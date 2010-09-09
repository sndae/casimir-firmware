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

#include "pkt.h"


/*
	Initializes a packet holder. 
	Sets all packets as unfilled, and data pointers to 0.
*/
void packetholder_init(PACKETHOLDER *ph,unsigned char numpkt)
{
	ph->numpkt=numpkt;
	ph->writeable=0;
	ph->wp=-1;
	ph->nw=0;
	ph->dataptr=0;
	ph->numerr=0;
	ph->total=0;
}




/*
	Finds a free packet, starting from the one after the most recently written packet.
	Returns -1 if failed, or the packet number that was open.
*/
signed char packetholder_write_next(PACKETHOLDER *ph)
{
	if(ph->nw>=ph->numpkt)
	{
		ph->numerr++;				// Log the attempt to open as an error
		return -1;
	}

	ph->wp++;						// Get the the next packet
	if(ph->wp>=ph->numpkt)		// Wrap around if needed
		ph->wp=0;
	ph->writeable=1;
	ph->dataptr=0;
		
	return ph->wp;
}
/*
	Mark the packet as filled and indicate it is closed.
*/
void packetholder_write_done(PACKETHOLDER *ph)
{
	if(ph->writeable==1)
	{
		ph->nw++;
		ph->writeable=0;
		ph->total++;				// Log the number of packets written
	}
}

/*
	Finds the next packet available for read, starting from the oldest packets.
	Return -1 if no packets are available for reading.
	Returns packet number otherwise.
*/
signed char packetholder_read_next(PACKETHOLDER *ph)
{
	if(ph->nw==0)
		return -1;			// No packet to read
	signed char tmp;

	tmp = ph->wp-ph->nw;	// Packet is nw before the writein packet
	if(!ph->writeable)		// If there is no writein packet, the packet wp is ready to read and is accounted for here
		tmp++;
	if(tmp<0)		
		tmp+=ph->numpkt;	// Wrap around if needed
	return tmp;
}
void packetholder_read_done(PACKETHOLDER *ph)
{
	if(ph->nw>0)
		ph->nw--;
}


/*
	Packet data structure
	+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------------
	|   55   |   AA   |   00   |   FF   |  type  |  time  |  time  |  time  |  time  |   id   |   id   |   id   |   id   | entry# |   ns   |   dt   |   dt   |   opt  |   00   |   00   |  ... data ...
	+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------------

	-------------+--------+--------+
	... Data ... |  XSUM  |  XSUM  |
	-------------+--------+--------+


	type: 		type of packet
	time: 		time of the first sample of the packet
	dt:   		delta time between samples
	ns:			number of samples in packet (meaning specific to packet type) 
	id:			device id
	entry#:		log entry
	opt:			optional (reserved, 0)

*/


unsigned short packet_init(unsigned char *pkt,unsigned char type,unsigned long time,unsigned long deviceid,unsigned char entry,unsigned char ns,unsigned short dt,unsigned char opt)
{
	pkt[0]=0x55;
	pkt[1]=0xAA;
	pkt[2]=0x00;
	pkt[3]=0xFF;
	pkt[4]=type;
	pkt[5]=time;
	pkt[6]=time>>8;
	pkt[7]=time>>16;
	pkt[8]=time>>24;
	pkt[9]=deviceid;
	pkt[10]=deviceid>>8;
	pkt[11]=deviceid>>16;
	pkt[12]=deviceid>>24;
	pkt[13]=entry;
	pkt[14]=ns;
	pkt[15]=dt;
	pkt[16]=dt>>8;
	pkt[17]=opt;
	pkt[18]=0;
	pkt[19]=0;
	return PACKET_PREAMBLE;
}
