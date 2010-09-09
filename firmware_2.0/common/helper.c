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

#include "cpu.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include "wait.h"
#include "main.h"
#include "helper.h"
#include "serial.h"
#include "pkt.h"
#include "core.h"
#include "init.h"
#include "global.h"
#include "menu.h"

// Various
const char crlf[] PROGMEM = "\r\n";






/******************************************************************************
*******************************************************************************
VARIOUS   VARIOUS   VARIOUS   VARIOUS   VARIOUS   VARIOUS   VARIOUS   VARIOUS   
*******************************************************************************
******************************************************************************/



/*
	Displays a startup banner.
	Banner is in program memory.
*/
void Banner(void)
{
	fputs_P(crlf,stdout);
	fputs_P(device_name,stdout);
	fputs_P(PSTR(" "),stdout);
	fputs_P(device_version,stdout);
	fputs_P(crlf,stdout);
	for(int i=0;i<strlen_P(device_name)+strlen_P(device_version)+1;i++)
		fputs(PSTR("="),stdout);
	puts_P(PSTR("\n\r"));
}

/*
	Reads the ID from EEPROM, unless DO_FIRST_SETTINGS is true, in which case the ID is written to EEPROM.
*/
void ReadWriteID(void)
{
	// Set or read the device ID
	if( 1 )
	{
		eeprom_write_byte((uint8_t*)ID_ADDR, ID);
		printf("Setting new device ID... \n");
		
	}
	device_id = eeprom_read_byte((uint8_t*)ID_ADDR);
	printf("Device ID: %ld\n\n", device_id);
}

void PrintConfigReset(void)
{
	puts_P(PSTR("Config reset. Run setup again"));
}

/*
	Returns a pointer to a subset of string, removing any leading CR or LF or SPC 
	String is modified to remove any trailing CR or LF or SPC.
*/
char *trim(char *string)
{
	char *r;
	for(short i=strlen(string)-1;i>=0;i--)
   {
   	if(string[i]==32 || string[i]==0xA || string[i]==0xD)
			string[i]=0;
		else
			break;
	}
	r = string;
	for(short i=0;i<strlen(string);i++)
   {
   	if(string[i]==32 || string[i]==0xA || string[i]==0xD)
			r=string+i+1;
		else
			break;
	}
	return r;
}


const char str[] PROGMEM = "Streaming\r\n";
void DisplayStartStreaming(void)
{
	fputs_P(str,file_usb);
	//fputs_P(str,file_bt);
}


