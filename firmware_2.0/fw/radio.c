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
#include "global.h"
#include "helper.h"
#include "wait.h"
#include "adc.h"
#include "main.h"
#include "radio.h"

/******************************************************************************
	Shadow registers
*******************************************************************************
	
	The shadow registers keep locally the value of the configuration registers of the radio.
	They are initialized upon calling mrf_initxxx. 
	
	These registers are used to modify some of the bits of the configuration registers
	without affecting the others by masking operations. This is used by functions
	such as mrf_setrxsens, etc.

******************************************************************************/
unsigned short _shadow_TXCCCREG;
unsigned short _shadow_PSCREG;

/******************************************************************************
	mrf_irqlevel
*******************************************************************************
	Returns the state of the radio IRQ pin.

	Return
		1: 		no interrupt
		0:		IRQ asserted
******************************************************************************/
unsigned char mrf_irqlevel(void)
{
	return (PINC>>4)&1;
}


/******************************************************************************
	mrf_spi_select
*******************************************************************************
	Selects the radio on the SPI bus

	Input:	
		ss=1: deselect
		ss=0: select
******************************************************************************/
void mrf_spi_select(char ss)
{
	ss=ss?1:0;
	PORTB=(PORTB&0xFE)|(ss);
}

/******************************************************************************
	mrf_spi_readwrite
*******************************************************************************
	Exchanges one data byte over the SPI bus on which the radio is connected

	Input:	
		data:	data sent to the radio
	Return:		data returned by the radio
******************************************************************************/
unsigned char mrf_spi_readwrite(unsigned char data)
{

	// Wait for empty transmit buffer
	while ( !( UCSR1A & (1<<UDRE1)) );
	// Put data into buffer, sends the data 
	UDR1 = data;
	// Wait for data to be received 
	while ( !(UCSR1A & (1<<RXC1)) );
	// Get and return received data from buffer 
	return UDR1;
}
/******************************************************************************
	mrf_spi_init
*******************************************************************************

	Initializes the SPI bus on which the radio is connected

******************************************************************************/
void mrf_spi_init(void)
{
	// MRF49XA:
	// clock must be < 2.5MHz
	// SPI mode 0,0: SCK 
	// SCK remain idle in low state (?)
	// Data is clocked in on the rising edge of SCK
	// Data is clocked out on the falling edge of SCK
	// MSB is sent first

	UBRR1=0;		// ??
	UCSR1C=0xC0;												// 0xC0: Master SPI, UDORD=0, UCPOL=0, UCPHA=0		// Correct setting for SI4420
	//UCSR1C=0xC4;												// 0xC4: Master SPI, UDORD=1, UCPOL=0, UCPHA=0
	//UCSR1C=0xC1;												// 0xC1: Master SPI, UDORD=0, UCPOL=1, UCPHA=0		
	//UCSR1C=0xC2;												// 0xC2: Master SPI, UDORD=0, UCPOL=0, UCPHA=1		// no data received (always 0)->likely phase wrong
	UCSR1B = (1<<RXEN1)|(1<<TXEN1);								// Enable receiver and transmitter
	// Set baud rate. Important: baud rate must be set after the transmitter is enabled (see datasheet)
	//UBRR1 = 255;												// UBRR1=255: 23KHz @ 12MHz
	UBRR1 = 3;													// UBRR1=3: 1.5MHz@12MHz
	//UBRR1 = 9;													// UBRR1=6: 600KHz@12MHz
}




/******************************************************************************
	mrf_icommand
*******************************************************************************

	Sends a command to the radio (16-bit) and returns the radio response.

	Note: assumes that the SPI select pin is asserted

******************************************************************************/
unsigned short mrf_icommand(unsigned short command)
{
	unsigned short data;
	
	//_delay_us(10);

	data=mrf_spi_readwrite(command>>8);
	data<<=8;
	data|=mrf_spi_readwrite(command);

	return data;
}


/******************************************************************************
	mrf_command
*******************************************************************************

	Sends a command to the radio (16-bit) and returns the radio response.
	Asserts ans deasserts the SPI select pin appropriately.

******************************************************************************/
unsigned short mrf_command(unsigned short command)
{
	unsigned short data;
	//unsigned short data2;
	mrf_spi_select(0);

	data=mrf_icommand(command);

	mrf_spi_select(1);
	
	return data;
}

/******************************************************************************
	mrf_icommands
*******************************************************************************

	Sends multiple commands to the radio.

	Note: assumes that the SPI select pin is asserted

******************************************************************************/
void mrf_icommands(unsigned short *commands,unsigned short n)
{
	unsigned short data;
	//unsigned short data2;
	for(unsigned short i=0;i<n;i++)
	{
		data=mrf_spi_readwrite(commands[i]>>8);
		data<<=8;
		data|=mrf_spi_readwrite(commands[i]);		
		printf_P(PSTR("CMD %X -> %X\r"),commands[i],data);
		//_delay_ms(1000);
	}
}

/******************************************************************************
	mrf_commands
*******************************************************************************

	Sends multiple commands to the radio.
	Asserts ans deasserts the SPI select pin appropriately.

******************************************************************************/
void mrf_commands(unsigned short *commands,unsigned short n)
{
	mrf_spi_select(0);

	mrf_icommands(commands,n);

	mrf_spi_select(1);

}


/*void mrf_reset_radio(void)
{
  mrf_command(FRMCREG | 0x0002);		
}*/




void mrf_printstatus(void)
{
	unsigned short data = mrf_command(0x0000);
	printf_P(PSTR("Status: %X\r"),data);

}
void mrf_printirq(void)
{
	printf_P(PSTR("MRF IRQ: %d\r"),(PINC&16)?1:0);
}




/******************************************************************************
	mrf_txreg_write
*******************************************************************************
	Transmits a data byte to the transmitter with the Transmit Register Write Command.

	Waits until the transmit buffer is empty, as signalled by the interrupt pin.
	
	TODO: possibly include a timeout. 
	However, this may not be needed if the transceiver is correctly configured (i.e. effectively in transmit mode).

	Input:	
		div:	8-bit parameter of the Data Rate Command. MSB=cs. bit 6:0=R
	Return:
		bitrate in bits per second		
******************************************************************************/
void mrf_txreg_write(unsigned char aByte)
{
	unsigned short status1;


	while(1)
	{
		// Wait for an interrupt to occur.
		while(mrf_irqlevel());		
		//printf_P(PSTR("%d "),mrf_irqlevel());
		// Read the status of the device
		status1 = mrf_command(0x0000);
		//printf_P(PSTR("%04X "),status1);

		// If the RGIT bit is set, send the byte and return
		if(status1&0x8000)
		{
			mrf_command(TRWCREG | aByte);
			return;
		}
		// Otherwise, the interrupt was triggered by another even than the RGIT... continue waiting.
	}
}






/******************************************************************************
	mrf_bitrate2div
*******************************************************************************
	Converts a bitrate into the divider value required by the Data Rate Command.

	Input:	
		br:		bitrate in bits per second
	Return:
		8-bit parameter of the Data Rate Command. MSB=cs. bit 6:0=R
******************************************************************************/
unsigned char mrf_bitrate2div(unsigned long br)
{
	unsigned long cs,r;
	unsigned char cfg;

	//R= (10000 / 29 / (1+cs*7) / BR) - 1, with BR in kbps

	// Quick sanity check. The device does not support speeds lower than 600 or higher than 115200.
	if(br<600 || br>115200)
		return 0;

	// Cutting point for cs is 4800bps. Speed lower than 4800 bps require cs=1. 
	if(br<4800)
		cs=1;
	else
		cs=0;

	// Do the conversion. br is in bit per second. Does round to the nearest integer with 20'000'000 and +1)/2
	r = ((20000000/29/(1+cs*7)/br)-1+1)/2;

	cfg = r;
	cfg |= cs<<7;

	//printf_P(PSTR("br: %ld -> r: %ld cs: %ld. %02X\r"),br,r,cs,cfg);
	
	return cfg;
}
/******************************************************************************
	mrf_div2bitrate
*******************************************************************************
	Converts the divider value of the Data Rate Command into a bitrate

	Input:	
		div:	8-bit parameter of the Data Rate Command. MSB=cs. bit 6:0=R
	Return:
		bitrate in bits per second		
******************************************************************************/
unsigned long mrf_div2bitrate(unsigned char div)
{
	unsigned long cs,br,r;
	

	// BR = 10000 / 29 / (R+1) / (1+cs*7) [kbps]

	cs = (div&0x80)?1:0;
	r=div&0x7f;

	// Convert into bit per second, and round to nearest.
	br = (20000000/29/(r+1)/(1+cs*7)+1)/2;

	
	//printf_P(PSTR("div: %02X -> br: %ld \r"),div,br);

	
	return br;
}

/******************************************************************************
	mrf_init433
*******************************************************************************
	Initializes the RFM12/SI4420 with the default parameters for 433MHz operation.

	Note: the transceiver is deactivated. Selection of transmit or receive mode 
	must be done after initialization with mrf_setradiotx or mrf_setradiorx;


	Input:
		bitrate:	data transmission speed in bits per second
******************************************************************************/
void mrf_init433(unsigned long bitrate)
{
	// Init the SPI interface
	mrf_spi_init();		
	// Set FFS# high.
	mrf_setffs(1);					


	// Setup the radio configuration

	mrf_command(CSCREG|0xD7);		// el ef 433MHz 12pF
	
	//PMCREG|0xD9,		// er ebb es ex dc (initial)
	mrf_command(FSCREG|0x640);		// 434MHz
	mrf_setdatarate(bitrate);
	
	_shadow_PSCREG = 0x4A0;			// nINT/VDI is VDI, fast, BW 134KHz, LNA 0dB, RSSI -103dBm
	mrf_command(PSCREG|_shadow_PSCREG);		
	mrf_command(DFCREG|0xAC);		// clock recovery auto lock, clock recovery slow, DQD=4
	mrf_command(FRMCREG|0x81);		// FIFO interrupt level 8, FIFO fill start condition synchron pattern, !ff, disable sensitive reset
	//AFCCREG|0x83,		// keep offset during receiving, no range restriction, !st, not fine mode, not enable frequency offset register, not enable calculation of offset (initial)
	mrf_command(AFCCREG|0x00);		// disable all AFC options

	_shadow_TXCCCREG = 0x50;		// !mp, 90KHz, 0dB
	mrf_command(TXCCCREG|_shadow_TXCCCREG);		

	mrf_command(WUTCREG|0x00);		// Wake-up timer unused
	mrf_command(LDCCREG|0x00);		// Low duty-cycle not enabled
	//AFCCREG|0x00,
	//FRMCREG|0x81
	
	mrf_setradioidle();


	mrf_command(0x0000);			// Reads the status register. This clears any possible interrupts.
}


/******************************************************************************
	mrf_setdatarate
*******************************************************************************
	Sets the data rate to the specified speed in bit per second. 
	This is done by writing to DRCREG.

******************************************************************************/
void mrf_setdatarate(unsigned long bps)
{
	mrf_command(DRCREG|mrf_bitrate2div(bps));
}

/******************************************************************************
	mrf_setradiotx
*******************************************************************************
	
	Switches the transceiver to transmit mode.

******************************************************************************/
void mrf_setradiotx(void)
{
	mrf_command(PMCREG|0x21);			// et dc: enable transmitter, disable clock output, disable lowbat, disable wake-up
}

/******************************************************************************
	mrf_setradiorx
*******************************************************************************
	
	Switches the transceiver to receive mode.

******************************************************************************/
void mrf_setradiorx(void)
{
	mrf_command(PMCREG|0x81);			// er dc: enable receiver, disable clock output, disable lowbat, disable wake-up
}

/******************************************************************************
	mrf_setradiotx
*******************************************************************************
	
	Switches the transceiver to idle (i.e. deactivate baseband, oscillator, etc)

******************************************************************************/
void mrf_setradioidle(void)
{
	mrf_command(PMCREG|0x01);			// dc: disable clock output, disable lowbat, disable wake-up
}

/******************************************************************************
	mrf_fifoenable
*******************************************************************************
	
	Enables the filling of the receive FIFO upon reception of the synchron pattern.
	This function must be called after setting the transceiver in receive mode to allow the FIFO
	to fill with data.

	In order to resynchronize on the synchron pattern the fifo must be first disabled then enabled
	with mrf_fifodisable and mrf_fifoenable.

******************************************************************************/
void mrf_fifoenable(void)
{
	// Activate the FIFO fill upon reception of the next synchron pattern.
	mrf_command(FRMCREG|0x83);
}
/******************************************************************************
	mrf_fifodisable
*******************************************************************************
	
	Disables the filling of the receive FIFO. 
	Typically called after the reception of a packet.

	In order to resynchronize on the synchron pattern the fifo must be first disabled then enabled
	with mrf_fifodisable and mrf_fifoenable.

******************************************************************************/
void mrf_fifodisable(void)
{
	// Reset the FIFO fill
	mrf_command(FRMCREG|0x81);
}
/******************************************************************************
	mrf_settxpower
*******************************************************************************
	
	Sets the transmit power.

	Input:
		p:		relative output power
				p		dB
				-       --
				0		  0
				1		 -3
				2		 -6
				3		 -9
				4		-12
				5		-15
				6		-18
				7		-21

******************************************************************************/
void mrf_settxpower(unsigned char power)
{
	_shadow_TXCCCREG &= 0xFFF8;						// Clear the lowest 3 bits (power value)
	_shadow_TXCCCREG |= power&0x7;					// Set the new power value

	mrf_command(TXCCCREG|_shadow_TXCCCREG);			// Set the new TXCCCREG
}
/******************************************************************************
	mrf_setrxsens
*******************************************************************************
	
	Sets the receive sensitivity.

	In slow mode, the RSSI threshold and LNA gain parameters can be used to set how strong the received signal should be.
	In fast mode, the RSSI threshold and LNA gain parameterss have no influence.

	Input:
		mode:	Valid data detection mode
				0:	fast
				1:	medium
				2:	slow
				3:	always on
		lna:	lna gain
				0:	0dB
				1:	-6dB
				2:	-14dB
				3:	-20dB
		rssi	rssi threshold
				0:	-103dBm
				1:	-97dBm
				2:	-91dBm
				3:	-85dBm
				4:	-79dBm
				5:	-73dBm

******************************************************************************/
void mrf_setrxsens(unsigned char mode,unsigned char lna,unsigned char rssi)
{
	_shadow_PSCREG &= 0xFCE0;						// Keep only the VDI/nINT and bandwidth parameter, clear mode, lna and rssi
	_shadow_PSCREG |= ((mode&3)<<8)|((lna&3)<<3)|(rssi&7);		// nINT/VDI is VDI, fast, BW 134KHz, LNA 0dB, RSSI -73dBm

	printf("rx sens: %X\r",PSCREG|_shadow_PSCREG);
	
	mrf_command(PSCREG|_shadow_PSCREG);			// Set the new PSCREG
}




/******************************************************************************
	mrf_packet_transmit
*******************************************************************************
	Broadcasts a data packet
	Does:
		- switch the radio to transmit mode, 
		- send the preamble: a number of 0xAAs to allow clock recovery, followed by the synchron pattern.
		- send the packet, 
		- send dummy bytes,
		- switch the radio to idle.
	
	Input:
		buffer:		data to transmit
		n:			number of bytes to transmit
******************************************************************************/
void mrf_packet_transmit(unsigned char *buffer,unsigned char n)
{
	// Goes in transmit mode
	mrf_setradiotx();

	// Reads the status register. This clears any possible interrupts.
	mrf_command(0x0000);

	// Clock recovery pattern
	mrf_txreg_write(0xAA);
	mrf_txreg_write(0xAA);
	mrf_txreg_write(0xAA);
	// Synchron pattern
	mrf_txreg_write(0x2D);
	mrf_txreg_write(0xD4);

	// Data
	for(unsigned char i=0;i<n;i++)
	{
		mrf_txreg_write(buffer[i]);
	}
	// Dummy bytes
	mrf_txreg_write(0xAA);
	mrf_txreg_write(0xAA);
	mrf_txreg_write(0xAA);
	// Go to idle mode
	mrf_setradioidle();

//	mrf_command(0x0000);

}


/******************************************************************************
	mrf_packet_receive
*******************************************************************************
	Receives over the air data
	Does:
		- switch the radio to receive mode, 
		- enable the FIFO fill,
		- receives the predefined number of bytes,
		- disables the FIFO fill,
		- switch the radio to idle.

	Input:
		buffer:		data buffer
		n:			number of bytes to receive
		timemout:	time in millisecond after which the function returns, even if no packet is received. Set to 0 to disable the timeout (wait forever)
		
	Return value:
		1:			success, a packet was received
		0:			failure, no packets were received before the timeout
******************************************************************************/
unsigned char mrf_packet_receive(unsigned char *buffer,unsigned short n,unsigned short timeout)
{
	unsigned long t1=timer_ms_get();		// Current time
	unsigned long t2=t1+timeout;			// Time at which we should bail out
	unsigned char success=1;
	
	// Sets the radio in receive mode
	mrf_setradiorx();


	// Reads the status register. This clears any possible interrupts.
	mrf_command(0x0000);			

	// Enable the FIFO fill.
	mrf_fifoenable();
		
	for(unsigned i=0;i<n;i++)
	{
		
		while(mrf_irqlevel() && (timeout==0 || timer_ms_get()<t2));

		// Check if we left the loop due to a timeout
		if(mrf_irqlevel())
		{
			success=0;
			break;
		}

		unsigned short data = mrf_command(RFRCREG);

		buffer[i]=data;
		
	}
	// Disable the FIFO fill
	mrf_fifodisable();

	// Return the radio to idle mode
	mrf_setradioidle();

	return success;
}



// Sets the FFS pin. Anyway it should always be high.
void mrf_setffs(unsigned char v)
{
	if(v)
		PORTC|=(1<<2);			// Set FFS# high when using TX register
	else
		PORTC&=~(1<<2);		// Set FFS# low when using TX register
}

