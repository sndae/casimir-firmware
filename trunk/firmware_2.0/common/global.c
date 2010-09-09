/*
   CASIMIR - Firmware
   Copyright (C) 2009,2010:
         Daniel Roggen, droggen@gmail.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
	Global variable declarations are here
*/

#include <stdio.h>
#include "pkt.h"


// Connection
FILE *file_usb;

// Device
unsigned long device_id = 0x12345678;


// DIGIREC
volatile unsigned long int time_absolute_tick;		// In audio units
volatile unsigned long int time_absolute_offset;	// In seconds, from Jan 1 1970 (UNIX time)



