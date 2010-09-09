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


#include <string.h>
#include <stdio.h>
#include "pkt.h"


/*
  Returns n bits from data, starting at firstbit.

  Bit/byte numbering:
  b0 b1 b2 b3 b4 b5 b6 b7   b8 b9 b10 b11 b12 b13 b14 b15   b16 b17 b18 b19 b20 b21 b22 b23
  -----------------------   -----------------------------   -------------------------------
            B0                           B1                                B2

  firstbit=0: bit 7 of byte 0
  firstbit=7: bit 0 of byte 0
  firstbit=8: bit 7 of byte 1

   The format is MSB first (big endian).

*/


void packet_init(PACKET *packet)
{
   //memset(packet->data,0,__PKT_DATA_MAXSIZE);
   for(unsigned short i=0;i<__PKT_DATA_MAXSIZE;i++)
   	packet->data[i]=0;
   packet->bitptr=0;
}
/*
  Add bits to packet in big endian format. Least significative byte comes last.
*/
void packet_addbits(PACKET *packet,unsigned int data,unsigned short size)
{
   unsigned char byteptr;
   unsigned char bitptr;
   unsigned int bitmask;
   unsigned short i;


   bitmask = 1<<(size-1);                 // Masks the current bit to add
   //printf("byteptr/bitptr: %d %d, data at byte: %X\n",byteptr,bitptr,data[byteptr]&0xff);

   for(i=0;i<size;i++)
   {
      byteptr=packet->bitptr>>3;             // bytenum is the byte in which to store data
      bitptr=7-(packet->bitptr&0x07);         // bytebit is the number in which to store data (7=MSB)
      if(data&bitmask)                       // bit to add is a 1 or 0
      {
         packet->data[byteptr] |= (1<<bitptr);
      }
      packet->bitptr++;

      bitmask>>=1;
   }

}
/*
  Add bits to packet in little endian format. Least significative byte comes first.
*/
void packet_addbits_little(PACKET *packet,unsigned int data,unsigned short size)
{
   unsigned short byteptr;
   unsigned short bitptr;
   unsigned int bitmask;
   unsigned short i;




   if(size<8)                                // Masks the current bit to add
      bitmask=1<<(size-1);
   else
     bitmask=0x80;

   for(i=0;i<size;i++)
   {
      byteptr=packet->bitptr>>3;             // byteptr is the byte in which to store data
      bitptr=7-(packet->bitptr&0x07);          // bitptr is the number in which to store data (7=MSB)

      if(data&bitmask)                       // bit to add is a 1 or 0
      {
         packet->data[byteptr] |= (1<<bitptr);
      }
      packet->bitptr++;

      if((i&0x07)==0x07)                     // Little endian: mask highest bit in next byte
      {
         if(size-i-1<8)
            bitmask<<=7+(size-i-1);          // Less than 8 bits to add: point inside the next byte
         else
            bitmask<<=7+8;                   // 8 or more bits to add: point to the MSB in the next byte
      }
      else
         bitmask>>=1;
   }
}


void packet_addchar(PACKET *packet,unsigned char data)
{
   packet_addbits(packet,data,8);
}
void packet_addshort(PACKET *packet,unsigned short data)
{
   packet_addbits(packet,data,16);
}
void packet_addshort_little(PACKET *packet,unsigned short data)
{
   packet_addbits_little(packet,data,16);
}
void packet_addint(PACKET *packet,unsigned int data)
{
   packet_addbits(packet,data,32);
}
void packet_addint_little(PACKET *packet,unsigned int data)
{
   packet_addbits_little(packet,data,32);
}
void packet_addchecksum_8(PACKET *packet)
{
   // Round up to the next byte
   if(packet->bitptr&0x07)
   {
      packet->bitptr=((packet->bitptr>>3)+1)<<3;
   }
   unsigned short check = packet_CheckSum(packet->data,packet_size(packet));
   packet_addchar(packet,check);
}
void packet_addchecksum_fletcher16(PACKET *packet)
{
   // Round up to the next byte
   if(packet->bitptr&0x07)
   {
      packet->bitptr=((packet->bitptr>>3)+1)<<3;
   }
   unsigned short check = packet_fletcher16(packet->data,packet_size(packet));
   packet_addshort(packet,check);
}
void packet_addchecksum_fletcher16_little(PACKET *packet)
{
   // Round up to the next byte
   if(packet->bitptr&0x07)
   {
      packet->bitptr=((packet->bitptr>>3)+1)<<3;
   }
   unsigned short check = packet_fletcher16(packet->data,packet_size(packet));
   packet_addshort_little(packet,check);
}
/*
  Returns the packet size in bytes, rounded up if a fractional number of bytes is used.
*/
unsigned short packet_size(PACKET *packet)
{
   unsigned short bytesize;
   bytesize=packet->bitptr>>3;
   if(packet->bitptr&0x07)
      bytesize++;
   return bytesize;
}

/*
  Longitudinal parity check
*/
unsigned short packet_CheckSum(unsigned char *ptr,unsigned n)
{
   unsigned short i;
   unsigned char lrc;

   lrc=0;
   for(i=0;i<n;i++)
   {
      lrc=lrc^ptr[i];
   }
   return lrc;
}

/*
  Fletcher's 16-bit checksum
*/
unsigned short packet_fletcher16(unsigned char *data, int len )
{
   unsigned short sum1,sum2;
   unsigned short check;
   sum1 = 0xff;
   sum2 = 0xff;

   while (len)
   {
      int tlen = len > 21 ? 21 : len;
      len -= tlen;
      do
      {
         sum1 += *data++;
         sum2 += sum1;
      }
      while (--tlen);
      sum1 = (sum1 & 0xff) + (sum1 >> 8);
      sum2 = (sum2 & 0xff) + (sum2 >> 8);
   }
   // Second reduction step to reduce sums to 8 bits
   sum1 = (sum1 & 0xff) + (sum1 >> 8);
   sum2 = (sum2 & 0xff) + (sum2 >> 8);


   check=sum1<<8|sum2;

   return check;
}
