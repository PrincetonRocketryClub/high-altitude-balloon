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
#include <SD.h>
#include <SPI.h>

#include "aprs.h"

// Ublox configuration code
#include "ublox.h"

// TinyGPSPlus from http://arduiniana.org/libraries/tinygpsplus/
#include "TinyGPS.h"
// Adafruit BMP180 Library
#include "adafruit_BMP085.h"

// ----- Pin Definitions ----- //
#define PTT_PIN 13 // Push to talk pin
#define THERMISTOR_PIN 36 // Thermistor Analog Input

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
#define SYMBOL_CHAR 'O'

// ----- Misc Constants ----- //
#define MAXSENDBUFFER 500 // Used to allocate a static buffer on the stack to build the AX25 buffer
#define APRSPERIOD 5000
#define LOGGINGPERIOD 2000
#define LOGGINGFILENAME "data.csv"


// ===== GLOBAL VARIABLES ==== //
char strbuf[MAXSENDBUFFER] = {};

struct PathAddress addresses[] = {
	{(char *)D_CALLSIGN, D_CALLSIGN_ID},	// Destination callsign
	{(char *)S_CALLSIGN, S_CALLSIGN_ID},	// Source callsign
	{(char *)NULL, 0}, // Digi1 (first digi in the chain)
	{(char *)NULL, 0}	 // Digi2 (second digi in the chain)
};

TinyGPSPlus gps; // GPS NEMA string decoder
Adafruit_BMP085 bmp180; // BMP180 Temperature/Pressure sensor

uint32_t timeOfAPRS = 0; // Time of last APRS transmission
uint32_t timeLogging = 0; // Time of last logging output
ublox_navmode_t currentUbloxMode = NAVMODE_LOW_PORTABLE;
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
	
	
	// Package the packet
	createAPRSStr(strbuf, gps, SYMBOL_TABLE, SYMBOL_CHAR, comment);
	
	// Send the packet
	aprs_send(addresses, nAddresses, strbuf);
}

// ----- Setup Code (Run on Startup) ----- //
void setup()
{
	Serial.begin(19200); // For debugging output over the USB port
	Serial.println("Starting...");
	
	// ----- GPS Setup ----- //
	Serial.println("----- GPS SETUP -----");
	GPS_SERIAL.begin(GPS_BAUDRATE);
	
	Serial.println("Configuring NEMA talker ID to GP");
	sendConfig(UBLOX_SET_NMEA_TALKER_GP, UBLOX_SET_NMEA_TALKER_GP_LEN);
	
	while (ublox_setRate(2000) == -1){
		Serial.println("Checksum mismatch, retrying...");
	}
	while (ublox_setNavMode(NAVMODE_LOW_PORTABLE) == -1){
		Serial.println("Checksum mismatch, retrying...");
	}
	
	// ----- BMP180 Setup ----- //
	Serial.println("----- BMP180 SETUP -----");
	if (bmp180.begin()) {
		Serial.println("BMP180 Setup Successful");
	} else {
		Serial.println("BMP180 Not Found");
	}
	
	// ----- APRS Setup ----- //
	Serial.println("----- APRS SETUP -----");
	aprs_setup(50, // number of preamble flags to send
		PTT_PIN, // Use PTT pin
		100, // ms to wait after PTT to transmit
		0, 0 // No VOX ton
	);
	
	// ----- SD Setup ----- //
	Serial.println("----- SD SETUP -----");
	if (SD.begin(BUILTIN_SDCARD)) {
		Serial.println("SD init succesful");
	} else {
		Serial.println("SD init failed, or card not present");
	}
	
	if (SD.exists(LOGGINGFILENAME)) {
		Serial.print(LOGGINGFILENAME);
		Serial.println(" already exists");
	} else {
		Serial.print(LOGGINGFILENAME);
		Serial.println(" not found, creating");
		File dataFile = SD.open(LOGGINGFILENAME, FILE_WRITE);
		dataFile.println("Timestamp (ms), Time, Latitude, Longitude, Altitude (cm), BMP180 Temperature, BMP180 Pressure, Thermistor");
		dataFile.close();
	}
	
	Serial.println("===== SETUP COMPLETE =====");
	
	// ----- Test REMOVE BEFORE LAUNCH ----- //
	/*
	char* test_gps = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n";
	for (int i = 0; i < 67; i++)
		gps.encode(test_gps[i]);
	*/
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
	if ((gps.location.isUpdated() || gps.satellites.isUpdated()) && gps.time.value() != 0) {
		Serial.printf("\r\nGPS Update %d:%d:%d T=+%ul\r\n", 
			gps.time.hour(), gps.time.minute(), gps.time.second(), millis());
		Serial.printf("Location: %f, %f altitude %f\r\n",
			gps.location.lat(), gps.location.lng(), gps.altitude.meters());
		Serial.printf("Satellites: %d\r\n", gps.satellites.value());
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
	if (gps.altitude.isValid()) {
		if (gps.altitude.meters() > 100 && currentUbloxMode != NAVMODE_AIRBORNE_1G) {
			ublox_setNavMode(NAVMODE_AIRBORNE_1G);
			currentUbloxMode = NAVMODE_AIRBORNE_1G;
		} else if(gps.altitude.meters() < 100 && currentUbloxMode != NAVMODE_LOW_PORTABLE) {
			ublox_setNavMode(NAVMODE_LOW_PORTABLE);
			currentUbloxMode = NAVMODE_LOW_PORTABLE;
		}
	}
	
	if (gps.location.isValid() && millis() - timeOfAPRS > APRSPERIOD) {
		Serial.println("===== APRS Transmission =====");
		char comment[36];
		snprintf(comment, 36, "%lu,%.1fC,%lu,%d", 
			millis()/1000,
			bmp180.readTemperature(), 
			bmp180.readPressure(), 
			analogRead(THERMISTOR_PIN));
		
		broadcastLocation(gps, comment);
		timeOfAPRS = millis();
	}
	
	if (millis() - timeLogging > LOGGINGPERIOD) {
		Serial.printf("Logging T = +%lu\r\n", millis()/1000);
		Serial.printf("Temperature: %.1fC, Pressure %d Pa\r\n", 
			bmp180.readTemperature(), bmp180.readPressure());
		Serial.print("Thermistor ADC: ");
		Serial.println(analogRead(THERMISTOR_PIN));
		
		File dataFile = SD.open(LOGGINGFILENAME, FILE_WRITE);
		if (dataFile) {
			dataFile.printf("%lu,%lu,%f,%f,%lu,%.1f,%lu,%d\r\n",
				millis(),
				gps.time.value(),
				gps.location.lat(),
				gps.location.lng(),
				gps.altitude.value(),
				bmp180.readTemperature(),
				bmp180.readPressure(),
				analogRead(THERMISTOR_PIN)
			);
			dataFile.close();
		} else {
			Serial.print("Error opening ");
			Serial.println(LOGGINGFILENAME);
		}
		
		timeLogging = millis();
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
