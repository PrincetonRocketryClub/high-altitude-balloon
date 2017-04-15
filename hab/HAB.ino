// Example program for the APRS code in this repository

/*
* Copyright (C) 2014 by Richard Nash (KC3ARY)
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <WProgram.h>
#include "aprs.h"

// TinyGPSPlus from http://arduiniana.org/libraries/tinygpsplus/
#include "TinyGPS++.h"

// Ublox configuration code
#include "ublox.h"

// ----- Pin Definitions ----- //
#define PTT_PIN 13 // Push to talk pin
#define THERMISTOR_PIN 17 // Thermistor Analog Input

#define GPS_SERIAL Serial4 // GPS Serial port
#define GPS_BAUDRATE 9600

// ----- APRS Constants ----- //
// Callsign and SSID
#define S_CALLSIGN "AG6GR"
#define S_CALLSIGN_ID	11	 //  SSID is the attached ID number ex. W1AW-11. 11 is usually for balloons

// Destination callsign: APRS (with SSID=0) is usually okay.
#define D_CALLSIGN		"APRS"
#define D_CALLSIGN_ID	0

// Symbol Table: '/' is primary table '\' is secondary table
#define SYMBOL_TABLE '/' 
// Primary Table Symbols: /O=balloon, /-=House, /v=Blue Van, />=Red Car
#define SYMBOL_CHAR '0'

// ----- Misc Constants ----- //
#define MAXSENDBUFFER 500 // Used to allocate a static buffer on the stack to build the AX25 buffer

// ===== GLOBAL VARIABLES ==== //
char strbuf[MAXSENDBUFFER] = {};

struct PathAddress addresses[] = {
	{(char *)D_CALLSIGN, D_CALLSIGN_ID},	// Destination callsign
	{(char *)S_CALLSIGN, S_CALLSIGN_ID},	// Source callsign
	{(char *)NULL, 0}, // Digi1 (first digi in the chain)
	{(char *)NULL, 0}	 // Digi2 (second digi in the chain)
};

TinyGPSPlus gps; // GPS NEMA string decoder

uint32_t timeOfAPRS = 0; // Tracks time of last APRS transmission
bool gotGPS = false; // Flag if a valid GPS fix has been received

// ===== FUNCTIONS ===== //

/*
 * Sets up and transmits a APRS packet, using the given GPS instance and
 * comment string
 */
void broadcastLocation(TinyGPSPlus &gps, char *comment)
{
	int nAddresses;
	if (gps.altitude.meters() > 1500) {
		// APRS recomendations for > 5000 feet = 1500 m is:
		// Path: WIDE2-1 is acceptable, but no path is preferred.
		nAddresses = 3;
		addresses[2].callsign = "WIDE2";
		addresses[2].ssid = 1;
	} else {
		// Below 1500 meters use a much more generous path (assuming a mobile station)
		// Path is "WIDE1-1,WIDE2-2"
		nAddresses = 4;
		addresses[2].callsign = "WIDE1";
		addresses[2].ssid = 1;
		addresses[3].callsign = "WIDE2";
		addresses[3].ssid = 2;
	}
	
	// For debugging print out the path
	/*
	Serial.print("APRS(");
	Serial.print(nAddresses);
	Serial.print("): ");
	for (int i=0; i < nAddresses; i++) {
		Serial.print(addresses[i].callsign);
		Serial.print('-');
		Serial.print(addresses[i].ssid);
		if (i < nAddresses-1)
			Serial.print(',');
	}
	Serial.print(' ');
	Serial.print(SYMBOL_TABLE);
	Serial.print(SYMBOL_CHAR);
	Serial.println();
	*/
	
	// Package the packet
	createAPRSStr(strbuf, gps, SYMBOL_TABLE, SYMBOL_CHAR, comment);
	
	// Print for debugging
	Serial.println("APRS packet:");
	Serial.println(strbuf);
	
	// Send the packet
	aprs_send(addresses, nAddresses, strbuf);
}

// ----- Setup Code (Run on Startup) ----- //
void setup()
{
	Serial.begin(19200); // For debugging output over the USB port
	Serial.println("Starting...");
	
	// ----- GPS Setup ----- //
	delay(1500);
	GPS_SERIAL.begin(GPS_BAUDRATE);
	Serial.println("Configuring NEMA talker ID to GP");
	sendConfig(UBLOX_SET_NMEA_TALKER_GP, UBLOX_SET_NMEA_TALKER_GP_LEN);
	Serial.println("Configuring 2Hz refresh rate");
	sendConfig(UBLOX_SET_PM2_2SEC, UBLOX_SET_PM2_2SEC_LEN);
	delay(1000);
	
	// ----- APRS Setup ----- //
	aprs_setup(50, // number of preamble flags to send
		PTT_PIN, // Use PTT pin
		100, // ms to wait after PTT to transmit
		0, 0 // No VOX ton
	);
	
	// ----- Test REMOVE BEFORE LAUNCH ----- //
	char* test_gps = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n";
	for (int i = 0; i < 67; i++)
		gps.encode(test_gps[i]);
}

// -----  Main loop (run repeatedly) ----- //
void loop()
{
	// Read in new GPS data if available
	while (GPS_SERIAL.available() > 0) {
		char readbyte = GPS_SERIAL.read();
		Serial.print(readbyte);
		gps.encode(readbyte);
	}
	
	// Debug: Also read from USB Serial
	while (Serial.available() > 0)
		gps.encode(Serial.read());
	
	// GPS Debug logging
	if (gps.altitude.isUpdated()) {
		gotGPS = true; // @TODO: Should really check to see if the location data is still valid
		Serial.printf("Location: %f, %f altitude %f\r\n",
			gps.location.lat(), gps.location.lng(), gps.altitude.meters());
	} else {
		/*
		Serial.print("No GPS ");
		Serial.println(gps.charsProcessed());
		Serial.print("Pass: ");
		Serial.print(gps.passedChecksum());
		Serial.print(" Fail: ");
		Serial.println(gps.failedChecksum());
		Serial.printf("Location: %f, %f altitude %f\r\n",
			gps.location.lat(), gps.location.lng(), gps.altitude.meters());
		*/
	}
	
	if (gotGPS && timeOfAPRS + 1000 < millis()) {
		broadcastLocation(gps, "HELLO");
		timeOfAPRS = millis();
	}
}

// OLD CODE, MIGHT BE REMOVABLE Called from the powerup interrupt servicing routine.
int main(void)
{
	setup();
	while (true) {
		loop();
		yield();
	}
	return 0; // Never reached.
}
