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

#ifndef __GLOBAL_H
#define __GLOBAL_H


// Connection
extern FILE *file_usb;


// Device
extern const char device_name[] PROGMEM;
extern const char device_version[] PROGMEM;
extern const char device_pin[] PROGMEM;
extern unsigned long device_id;



// DIGIREC
extern volatile unsigned long int time_absolute_tick;		// In audio units
extern volatile unsigned long int time_absolute_offset;	// In seconds, from Jan 1 1970 (UNIX time)


#endif
