#include <stdint.h>
#include <WProgram.h>
#include "ublox.h"

#define GPS_SERIAL Serial4
#define GPS_TIMEOUT 3000

// Set main talker ID to GP
byte UBLOX_SET_NMEA_TALKER_GP[] = {
	0xB5, 0x62, 0x06, 0x17, 0x14, 0x00, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x59
};
size_t UBLOX_SET_NMEA_TALKER_GP_LEN = sizeof(UBLOX_SET_NMEA_TALKER_GP);

// Calculates the checksum for the given message and writes the appropriate fields
void calcChecksum(byte *msg, size_t len) {
	byte CK_A = 0, CK_B = 0;
	for (size_t i = 2; i < len - 2; i++) {
		CK_A = CK_A + msg[i];
		CK_B = CK_B + CK_A;
	}
	msg[len - 2] = CK_A;
	msg[len - 1] = CK_B;
}

// Sends the given config message to the Ublox receiver and verifies response
int sendConfig(byte* msg, size_t len) {
	// Update checksum of message
	calcChecksum(msg, len);
	
	// Clean serial stream
	GPS_SERIAL.flush();
	
	/* Debug, prints out messages
	for (int i = 0; i < len; i++){
		Serial.print(msg[i], HEX);
		Serial.print(" ");
	}
	Serial.println();
	*/
	
	// Write the message
	GPS_SERIAL.write(msg, len);
	GPS_SERIAL.println();
	GPS_SERIAL.flush();
	
	// Verify ack packet
	unsigned long startTime = millis();
	byte byteCount = 0;
	byte inbyte = 0;
	
	while (millis() - startTime < GPS_TIMEOUT) {
		while (GPS_SERIAL.available()) {
			// Get next ACK byte
			inbyte = GPS_SERIAL.read();
			// Check if byte is correct
			//Serial.println(inbyte, HEX);
			switch (byteCount) {
			case 0:
				if (inbyte != 0xB5) {
					continue;
				}
				break;
			case 1:
				if (inbyte != 0x62) {
					return -1;
				}
				break;
			case 2:
				if (inbyte != 0x05) {
					return -1;
				}
				break;
			case 3:
				if (inbyte != 0x01) {
					return -1;
				}
				break;
			case 4:
				if (inbyte != 0x02) {
					return -1;
				}
				break;
			case 5:
				if (inbyte != 0x00) {
					return -1;
				}
				break;
			case 6:
				if (inbyte != msg[2]) {
					return -1;
				}
				break;
			case 7:
				if (inbyte != msg[3]) {
					return -1;
				}
				break;
			// Don't worry about checksums
			case 8:
			case 9:
				break;
			default:
				Serial.println("ERROR: Ublox config invalid ACK packet byteCount");
				return -2;
			}
			byteCount++;
			// Entire ACK received
			if (byteCount > 9) {
				Serial.println("Config success!");
				return 0;
			}
		}
	}
	Serial.println("ERROR: Ublox config timeout");
	return -2;
}

// Sets the navigation mode of the UBlox GPS to the given navmode
int ublox_setNavMode(ublox_navmode_t navmode) {
	byte msg[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 
		navmode, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 
		0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00};
	Serial.print("Setting navmode: ");
	Serial.println(navmode);
	return sendConfig(msg, sizeof(msg));
}

/* 
 * Sets the update period of the UBlox GPS to the given time in milliseconds.
 * Note the UBlox output will always be aligned to the top of the second.
 */
int ublox_setRate(uint16_t period) {
	byte msg[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 
		(byte)(period & 0x00FF), (byte)(period >> 8), 0x01, 0x00, 0x01, 0x00, 0x00, 0x00};
	Serial.print("Setting period: ");
	Serial.println(period);
	return sendConfig(msg, sizeof(msg));
}