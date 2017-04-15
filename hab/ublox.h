#ifndef UBLOX_H
#define UBLOX_H

#include <stdint.h>

enum ublox_navmode_t {
	NAVMODE_LOW_PORTABLE = 0,
	NAVMODE_STATIONARY = 2,
	NAVMODE_PEDESTRIAN = 3,
	NAVMODE_AUTOMOTIVE = 4,
	NAVMODE_SEA = 5,
	NAVMODE_AIRBORNE_1G = 6,
	NAVMODE_AIRBORNE_2G = 7,
	NAVMODE_AIRBORNE_4G = 8
};

// Set main talker ID to GP
extern byte UBLOX_SET_NMEA_TALKER_GP[];
extern size_t UBLOX_SET_NMEA_TALKER_GP_LEN;

// Sends the given config message to the Ublox receiver
int sendConfig(byte* msg, size_t len);

// Sets the navigation mode of the UBlox GPS to the given navmode
int ublox_setNavMode(ublox_navmode_t navmode);

/* 
 * Sets the update period of the UBlox GPS to the given time in milliseconds.
 * Note the UBlox output will always be aligned to the top of the second.
 */
int ublox_setRate(uint16_t period);
#endif