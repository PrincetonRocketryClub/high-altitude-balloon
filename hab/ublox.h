#ifndef __UBLOX_H__
#define __UBLOX_H__

#include <stdint.h>

// GPS Mode 0 (Low altitude, Portable)
extern uint8_t UBLOX_SET_GPS_MODE_0[];
// GPS Mode 6 (Airborne, High Altitude)
extern uint8_t UBLOX_SET_GPS_MODE_6[];

// Set main talker ID to GP
extern uint8_t UBLOX_SET_NMEA_TALKER_GP[];

// Set PM2 to 1 sec refresh
extern uint8_t UBLOX_SET_PM2_1SEC[];
// Set PM2 to 2 sec refresh
extern uint8_t UBLOX_SET_PM2_2SEC[];
// Set PM2 to 10 sec refresh
extern uint8_t UBLOX_SET_PM2_10SEC[];

// Sends the given config message to the Ublox receiver
int sendConfig(uint8_t* msg);

#endif
