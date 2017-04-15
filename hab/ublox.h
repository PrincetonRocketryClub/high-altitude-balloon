#ifndef __UBLOX_H__
#define __UBLOX_H__

#include <stdint.h>

// GPS Mode 0 (Low altitude, Portable)
extern byte UBLOX_SET_GPS_MODE_0[];
extern size_t UBLOX_SET_GPS_MODE_0_LEN;
// GPS Mode 6 (Airborne, High Altitude)
extern byte UBLOX_SET_GPS_MODE_6[];
extern size_t UBLOX_SET_GPS_MODE_6_LEN;

// Set main talker ID to GP
extern byte UBLOX_SET_NMEA_TALKER_GP[];
extern size_t UBLOX_SET_NMEA_TALKER_GP_LEN;

// Set PM2 to 1 sec refresh
extern byte UBLOX_SET_PM2_1SEC[];
extern size_t UBLOX_SET_PM2_1SEC_LEN;
// Set PM2 to 2 sec refresh
extern byte UBLOX_SET_PM2_2SEC[];
extern size_t UBLOX_SET_PM2_2SEC_LEN;
// Set PM2 to 10 sec refresh
extern byte UBLOX_SET_PM2_10SEC[];
extern size_t UBLOX_SET_PM2_10SEC_LEN;

// Sends the given config message to the Ublox receiver
int sendConfig(byte* msg, size_t len);

#endif
