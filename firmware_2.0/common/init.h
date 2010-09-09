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


#ifndef __INIT_H
#define __INIT_H



#define ID_ADDR 50  						// EEPROM address of the ID
#define CONFIG_ADDR_SAMPLERATE 60  	// EEPROM address of the sample rate
#define CONFIG_ADDR_CHECKSUM   61  	// EEPROM address of the checksum
#define CONFIG_ADDR_COUNTER    62	// EEPROM address of the counter format
#define CONFIG_ADDR_TESTDATA   63  	// EEPROM address of test-data enable
#define CONFIG_ADDR_FREE	 	 64	// EEPROM address for device specific config




void init_InitPort(void);
void init_Module(void);
unsigned char init_IsSRValid(unsigned char sr);
void init_SampleTimer(unsigned char config_samplerate);
void init_ApplyConfig(void);


//void init_timer1(void);


#endif