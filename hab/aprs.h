/*
 * Copyright (C) 2014 by KC3ARY Rich Nash
 * 
 * Module modified from code by EA5HAV.
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

#ifndef __APRS_H__
#define __APRS_H__

#include "TinyGPS.h"

struct PathAddress
{
  const char *callsign;
  uint8_t ssid;
};

void aprs_setup(const uint16_t preambleFlags, // number of preambleFlags to send, must be at least 1 to frame packet
    const uint8_t pttPin, // Use PTT pin, 0 = do not use PTT
    const uint16_t pttDelay, // ms to wait after PTT to transmit
    const uint32_t toneLength, const uint32_t silenceLength // Emit sub-audio tone before packet to trigger VOX
    );

void aprs_send(const PathAddress * const paths, const int nPaths,
	const char * dataStr); // Synchronized ... won't return until the entire packet is sent

/*
 * Packages the given GPS and identification information into an APRS packet string
 * and stores it in the given buffer buf.
 * Returns the length of the string written, not including terminating /0
 */
int createAPRSStr( char * buf, TinyGPSPlus &gps, const char symbolTableIndicator, 
	const char symbol, const char* comment);

#endif
