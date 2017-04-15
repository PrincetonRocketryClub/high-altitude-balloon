/*
* Copyright (C) 2017 by AG6GR
* 
* Module modified after being inheritted from KC3ARY Rich Nash and EA5HAV. The same license described below
* is in effect.
*
* The below copyright is kept intact.
*
*/

/* Credit to the trackuino project written by EA5HAV Javi
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <WProgram.h>
#include "afsk.h"
#include "ax25.h"
#include "aprs.h"
#include "TinyGPS.h"

#define MAXSENDBUFFER 500

uint16_t preambleFlags;

// Convert latitude from a float to a string DDMM.MM[N/S]
int latToStr(char * const s, const int size, RawDegrees latitude)
{
	char hemisphere = latitude.negative ? 'S' : 'N';
	// TODO verify if minutes includes whole degrees
	uint32_t milli_minutes = 60 * latitude.billionths / 10000000;
	return snprintf(s, size, "%02d%02lu.%02lu%c", latitude.deg, milli_minutes / 100, milli_minutes % 100, hemisphere);
}

// Convert latitude from a float to a string DDDMM.MM[W/E]
int lonToStr(char * const s, const int size, RawDegrees longitude)
{
	char hemisphere = longitude.negative ? 'W' : 'E';
	
	// TODO verify if minutes includes whole degrees
	uint32_t milli_minutes = 60 * longitude.billionths / 10000000;
	return snprintf(s, size, "%03d%02lu.%02lu%c", longitude.deg, milli_minutes / 100, milli_minutes % 100, hemisphere);
}
/*
 * Packages the given GPS and identification information into an APRS packet string
 * and stores it in the given buffer buf.
 * Returns the length of the string written, not including terminating /0
 */
int createAPRSStr( char * buf, TinyGPSPlus &gps, const char symbolTableIndicator, 
	const char symbol, const char* comment) {
	int index = 0;
	
	// ----- Begin Comment/Data section ----- //
	
	// Data type flag byte
	// '/': Report w/ timestamp, no APRS messaging. 
	// '$': NMEA raw data
	buf[0] = '/';
	index++;
	
	// Timestamp HHMMSS format, append h to indicate HMS format
	index += snprintf(&buf[index], MAXSENDBUFFER - index, "%02u%02u%02uh", (unsigned int) gps.time.hour(),
		(unsigned int) gps.time.minute(), (unsigned int) gps.time.second());
	
	// ----- Position Data ----- //
	
	// Example location block: "4903.50N/07201.75W-", using Symbol Table ID '/' and Code '-'
	
	// Latitude DDMM.MM
	index += latToStr(&buf[index], MAXSENDBUFFER - index, gps.location.rawLat());
	
	// Display Symbol Table ID
	buf[index] = symbolTableIndicator;
	index++;
	
	// Longitude DDDMM.MM
	index += lonToStr(&buf[index], MAXSENDBUFFER - index, gps.location.rawLng());
	
	// Display Symbol Code
	buf[index] = symbol;
	index++;
	
	// ----- APRS Data Extension CSE + '/' + SPD (7b) ----- //
	// Heading (degrees, 3b) / Speed (knots, 3b)
	index += snprintf(&buf[index], MAXSENDBUFFER - index, "%03u/%03d", 
		(unsigned int)gps.course.deg(), (unsigned int) gps.speed.knots());
	
	// Altitude /A=aaaaaa, must be in feet. 
	index += snprintf(&buf[index], MAXSENDBUFFER - index, "/A=%06d", (unsigned int)gps.altitude.feet());
	
	// ----- Additional Comments ----- //
	
	// Extra comments
	strncat(&buf[index], comment, MAXSENDBUFFER - index);
	index += sizeof(comment);
	
	Serial.println(buf);
	
	// Don't forget to count ending '\0'
	return index;
}

// Dump out the AX25 packet in a semi-readable way
// Note, the end of header and CRC are sent as ASCII characters, which they aren't
void logBuffer(const uint8_t * const buf, const int bitsSent)
{
	Serial.printf("Bits in packet %d: ", bitsSent);
	//Serial.print(dayOfMonth);
	//Serial.print(',');
	//Serial.print(hour);
	//Serial.print(',');
	//Serial.print(min);
	//Serial.print(',');
	
	uint8_t frameState = 0; // 0-No start, 1-start, 2-in header, 3-In info
	uint8_t bSoFar = 0x00;
	uint8_t gotBit = 0;
	int numOnes = 0;
	for (int onBit = 0; onBit < bitsSent; onBit++) {
		uint8_t bit = buf[onBit >> 3] & (1 << (onBit & 7));
		if (numOnes == 5) {
			// This may be a 0 due to bit stuffing
			if (bit) { // Maybe it's a 0x7e
				onBit++;
				bit = buf[onBit >> 3] & (1 << (onBit & 7));
				if (gotBit == 6 && bSoFar == 0x3e && !bit) {
					// Got 0x7e frame start/end
					if (frameState == 0) {
						frameState = 1;
						Serial.print('[');
					} else
					if (frameState == 3 || frameState == 2) {
						frameState = 0;
						Serial.print(']');
					}
					bSoFar = 0x00;
					gotBit = 0;
				} else {
					Serial.print('X'); // Error
				}
			}
			numOnes = 0;
			continue;
		}
		if (bit) {
			// Set the one bit
			bSoFar |= (1 << gotBit);
			numOnes++;
		} else {
			numOnes = 0;
		}
		gotBit++;
		if (gotBit == 8) {
			// Got a byte;
			if (frameState == 1) {
				frameState = 2;
			} else
			if (frameState == 2 && bSoFar == 0xf0) { // 0xf0 is the last byte of the header
				frameState = 3;
			}
			if (frameState == 2) {
				// In header
				Serial.print((char) (bSoFar >> 1));
			} else {
				// In info
				Serial.print((char) bSoFar);
			}
			bSoFar = 0x00;
			gotBit = 0;
		}
	}
	Serial.println();
}

void aprs_setup(const uint16_t p_preambleFlags, const uint8_t pttPin,
    const uint16_t pttDelay, const uint32_t toneLength,
    const uint32_t silenceLength)
{
	preambleFlags = p_preambleFlags;
	afsk_setup(pttPin, pttDelay, toneLength, silenceLength);
}

// This can operate in two modes, PTT or VOX
// In VOX mode send a toneLength and silenceLength. The audible tone will be sent for that
// many milliseconds to open up VOX, and then that many MS of silence, then the packet will
// begin.
// In PTT mode the pin given will be raised high, and then PTT_DELAY ms later, the packet will
// begin
void aprs_send(const PathAddress * const paths, const int nPaths,
	const char * dataStr)
{
	uint8_t buf[MAXSENDBUFFER];
	
	ax25_initBuffer(buf, sizeof(buf));
	
	ax25_send_header(paths, nPaths, preambleFlags);
	
	ax25_send_string(dataStr);
	
	// Footer
	ax25_send_footer();
	
	// ----- Begin transmission code ----- //
	// Make sure all Serial data (which is based on interrupts) is done before you start sending.
	Serial.flush(); 
	
	// Set the buffer of bits we are going to send
	afsk_set_buffer(buf, ax25_getPacketSize());
	
	// Double check all characters are sent
	Serial.flush(); 
	
	// Critical section, no more operations until this is done.
	afsk_start();
	while (afsk_busy())
		;
	
	// Debug logging
	//logBuffer(buf, ax25_getPacketSize());
}
