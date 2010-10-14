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
#include "serial.h"
#include "wait.h"
#include "global.h"
#include "helper2.h"
#include "main.h"
#include "radio.h"
#include "radio_test.h"

// Continously sends data, max power
void test_radio_continuoustx(void)
{
	printf_P(PSTR("Radio Test Continuous TX\r"));
	
	// Initialize the radio
	mrf_init433(4800);

	// Buffer in which we build our packets
	unsigned char buffer[13] = "TXTST ? ABCXX";

	while(1)
	{
		// Increement the value of byte 6 (packet counter)
		buffer[6]++;
		// Computes a checksum on the first 11 bytes, stores it at the end
		packet_fletcher16(buffer,11,&buffer[11],&buffer[12]);


		printf_P(PSTR("Sending\r"));

		// Send the packet
		mrf_packet_transmit(buffer,13);

		// Wait a bit
		_delay_ms(100);
	}
	
}

// Continously receives data, max sensitivity
void test_radio_continuousrx(void)
{
	// We on purpose receive a little bit more data than what is sent (n defined below, the tx function sends 13 bytes, we receive n for fun).
	unsigned char n=15;
	unsigned char buffer[n];
	unsigned char xsum1,xsum2;

	printf_P(PSTR("Radio Test RX\r"));

	// Initializes the radio
	mrf_init433(4800);


	while(1)
	{
		// Receive the data
		unsigned char success = mrf_packet_receive(buffer,n,500);
		if(!success)
		{
			printf_P(PSTR("X\r"));
			continue;
		}

		// Computes a checksum on the first 11 received bytes 
		packet_fletcher16(buffer,11,&xsum1,&xsum2);

		// Print the whole data
		for(unsigned i=0;i<n;i++)
			printf_P(PSTR("%02X "),(int)buffer[i]);

		// Print the computed checksum on the received data
		printf_P(PSTR("- X: %02X%02X "),xsum1,xsum2);

		// Print if the computed checksum matches the received checksum
		if(xsum1==buffer[11] && xsum2==buffer[12])
			printf_P(PSTR("CK OK"));
		else
			printf_P(PSTR("CK ERR"));
		
		// New line
		printf_P(PSTR("\r"));
	}
}


// Continously sends data, varies power
void test_radio_continuoustxpowervar(void)
{
	printf_P(PSTR("Radio Test Continuous TX with Power variation\r"));
	
	// Initialize the radio
	mrf_init433(4800);

	// Buffer in which we build our packets
	unsigned char buffer[13] = "TXPW ?? ABCXX";
	buffer[5]=0;
	buffer[6]=0;

	while(1)
	{
		for(unsigned i=0;i<4;i++)
		{
			// Set the transmit power
			mrf_settxpower(i*2);

			// Store in the packet the transmit power
			buffer[5]=i;

			// Computes a checksum on the first 11 bytes, stores it at the end
			packet_fletcher16(buffer,11,&buffer[11],&buffer[12]);


			printf_P(PSTR("Sending\r"));

			// Send the packet
			mrf_packet_transmit(buffer,13);
			_delay_ms(1);
		}
		// Increment the value of byte 6 (packet counter)
		buffer[6]++;

		// Wait a bit
		_delay_ms(100);
	}
	
}

// Continously receives data, varies sensitivity every 10 seconds
void test_radio_continuousrxsensvar(void)
{
	// We on purpose receive a little bit more data than what is sent (n defined below, the tx function sends 13 bytes, we receive n for fun).
	unsigned char n=15;
	unsigned char buffer[n];
	unsigned char xsum1,xsum2;
	unsigned long t1;

	printf_P(PSTR("Radio Test RX Sensitivity Variation\r"));

	// Initializes the radio
	mrf_init433(4800);


	unsigned char sens=0;
	
	while(1)
	{
		t1 = timer_ms_get()+10000;		// Time in millisecond of the next change of sensitivity
		switch(sens)
		{
			case 0:
				printf_P(PSTR("Sensitivity: fast mode\r"));
				mrf_setrxsens(0,0,0);
				break;
			case 1:
				printf_P(PSTR("Sensitivity: slow mode, LNA 0dB, RSSI -103dBm\r"));
				mrf_setrxsens(2,0,0);			
				break;
			case 2:
				printf_P(PSTR("Sensitivity: slow mode, LNA 0dB, RSSI -97dBm\r"));
				mrf_setrxsens(2,0,1);			
				break;
			case 3:
				printf_P(PSTR("Sensitivity: slow mode, LNA 0dB, RSSI -91dBm\r"));
				mrf_setrxsens(2,0,2);
				break;
			case 4:
				printf_P(PSTR("Sensitivity: slow mode, LNA 0dB, RSSI -73dBm\r"));
				mrf_setrxsens(2,0,5);
				break;
		}
		sens++;
		if(sens>4)
			sens=0;

		while(timer_ms_get()<t1)	// break the loop if the time has come to change the sensitivity
		{
			// Receive the data
			unsigned char success = mrf_packet_receive(buffer,n,500);
			if(!success)
			{
				// There has been a timeout in receiving a packet. We just continue to try again
				continue;
			}

			// Computes a checksum on the first 11 received bytes 
			packet_fletcher16(buffer,11,&xsum1,&xsum2);

			// Print the whole data
			for(unsigned i=0;i<n;i++)
				printf_P(PSTR("%02X "),(int)buffer[i]);

			// Print the computed checksum on the received data
			printf_P(PSTR("- X: %02X%02X "),xsum1,xsum2);

			// Print if the computed checksum matches the received checksum
			if(xsum1==buffer[11] && xsum2==buffer[12])
				printf_P(PSTR("CK OK"));
			else
				printf_P(PSTR("CK ERR"));
	
			// New line
			printf_P(PSTR("\r"));
		}
	}
}
