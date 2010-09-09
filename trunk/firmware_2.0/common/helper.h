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

#ifndef __HELPER_H
#define __HELPER_H


extern const char crlf[] PROGMEM;

// Increment counter with wraparound. The wrapvalue is never reached.
#define INCREMENT_WRAP(variable,wrapvalue) (variable=(variable>=(wrapvalue-1))?0:(variable+1))


// CONFIGURATION MENU
void DefaultConfigure(FILE *file);
void DefaultSaveConfiguration(void);
unsigned char DefaultLoadConfiguration(void);



// VARIOUS
//unsigned char BuildTestPacket(PACKET *packet,unsigned short counter,unsigned char countertype, unsigned char payloadsize,unsigned char checksumtype);
void Banner(void);
void ReadWriteID(void);
void PrintConfigReset(void);
char *trim(char *string);
void DisplayStartStreaming(void);
void CheckEnterSetup(FILE *file);
char *trim(char *string);

#endif
