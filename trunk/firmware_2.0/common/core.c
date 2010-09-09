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


/******************************************************************************
*******************************************************************************
SAMPLING AND EVENT LOOP   SAMPLING AND EVENT LOOP   SAMPLING AND EVENT LOOP   
*******************************************************************************
******************************************************************************/

/******************************************************************************
	Interrupt vector for sample timer. Includes software downsampling logic.
******************************************************************************/
/*ISR(TIMER0_COMPA_vect)
{
	static unsigned short divider_software_sample_counter=0;
	static unsigned short divider_software_life_counter=0;
	
	divider_software_sample_counter++;
	if(divider_software_sample_counter>=divider_software_sample)
	{
		Sample();
		divider_software_sample_counter=0;
	}
	
	if(bluetooth_up)
	{
		divider_software_life_counter++;
		if(divider_software_life_counter>=divider_software_life)
		{
			LifeSign();
			divider_software_life_counter=0;
		}
	}
}*/

/*
	Sample is called at each software sample period.
	It updates the packet counters, builds, and sends packets.
*/
//void Sample(void)
//{
/*	// Continuously increment, even if not connected, to ensure continuity in the packet counter accross reconnections
	pktctr_abs++;					
	
	// Do not waste time when unconnected.
	if(!bluetooth_up)
		return;
	
	// If test data mode, send test packet
	unsigned char packettosend;
	if(config_testdata)
		packettosend=BuildTestPacket(&packet,pktctr_abs,config_counter,10,config_checksum);
	else
		packettosend=BuildPacket(&packet);

	if(packettosend)
	{
		pktctr_built++;
		if(PacketSend(&packet))
			pktctr_sent++;
	}
*/
//}

/*
	Send the packet if possible, i.e. if enough space in transmit buffer.
	Returns 1 if packet was placed in transmit buffer.
	Returns 0 otherwise.
*/
//unsigned char PacketSend(PACKET *packet)
//{
	/*
	// Output buffer large enough to send the packet
	unsigned short size = packet_size(packet);
	if(uart1_txbufferfree()>=size)
	{
		for(unsigned short i=0;i<size;i++)
			uart1_fputchar_int(packet->data[i],file_bt);		
		return 1;
	}
	*/
//	return 0;
//}

/******************************************************************************
	Main event loop
******************************************************************************/
/*void MainEventLoop(void)
{
	
	while(1)
	{
		sleep_enable();
		sleep_cpu();
		
		
	}
}

*/
