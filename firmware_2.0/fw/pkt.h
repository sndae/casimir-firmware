/*
   CASIMIR - Firmware
   Copyright (C) 2010:
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


#ifndef __PKT_H
#define __PKT_H


// Size of the preamble in the data packet
#define PACKET_PREAMBLE				20			// This MUST be a multiple of 2



// Packet holding structure
typedef struct
{
	signed char numpkt;				// Total number of packet
	signed char writeable;			// Packet wp currently open for write: 1 or 0
	signed char wp;					// Write packet: packet in which data [is currently]/[was last] written.
	signed char nw;					// Number of packets that are written

	unsigned short dataptr;			// Data pointer in the currently open packet. Units: number of samples
	//unsigned short byteptr;			// Data pointer in the currently open packet. Units: number of bytes (complementary to dataptr, for optimization purposes)

	unsigned long int numerr;		// Number of attempts to open a packet without any free one.

	unsigned long int total;		// Total number of packets that were written (incremented upon packet_write_done)
} PACKETHOLDER;


// Packet holder functions
void packetholder_init(PACKETHOLDER *ph,unsigned char numpkt);
unsigned short packet_init(unsigned char *pkt,unsigned char type,unsigned long time,unsigned long deviceid,unsigned char entry,unsigned char ns,unsigned short dt,unsigned char opt);
signed char packetholder_write_next(PACKETHOLDER *ph);
void packetholder_write_done(PACKETHOLDER *ph);
signed char packetholder_read_next(PACKETHOLDER *ph);
void packetholder_read_done(PACKETHOLDER *ph);

#endif

