/*
   pkt - Packet helper functions
   Copyright (C) 2008,2009:
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
   This file provides helper functions to create packets for streaming data transfer.
   In particular it provides functions to append data into a packet. Data can be of
   various sizes, including custom-sized bitfields.
   Checksumming is also supported.

   Usage:
      1. Declare a packet once (e.g. as a global variable)
      2. Before filling the packet, call packet_init
      3. Fill the packet with the various packet_add??? functions
      4. Optionally, append a checksum with packet_checksum_*** functions
      5. Stream the packet. The size of the packet in bytes is provided by packet_size.

*/

#ifndef PKT_H
#define PKT_H


#define __PKT_DATA_MAXSIZE 32

typedef struct
{
   unsigned char data[__PKT_DATA_MAXSIZE];
   unsigned short bitptr;

} PACKET;

void packet_init(PACKET *packet);
void packet_addbits(PACKET *packet,unsigned int data,unsigned short nbits);
void packet_addbits_little(PACKET *packet,unsigned int data,unsigned short nbits);
void packet_addchar(PACKET *packet,unsigned char data);
void packet_addshort(PACKET *packet,unsigned short data);
void packet_addshort_little(PACKET *packet,unsigned short data);
void packet_addint(PACKET *packet,unsigned int data);
void packet_addint_little(PACKET *packet,unsigned int data);
void packet_addchecksum_8(PACKET *packet);
void packet_addchecksum_fletcher16(PACKET *packet);
void packet_addchecksum_fletcher16_little(PACKET *packet);
unsigned short packet_size(PACKET *packet);
unsigned short packet_CheckSum(unsigned char *ptr,unsigned n);
unsigned short packet_fletcher16(unsigned char *data, int len );


#endif // PKT_H
