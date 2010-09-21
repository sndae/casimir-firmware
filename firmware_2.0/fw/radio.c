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
#include "adc.h"
#include "main.h"
#include "radio.h"

/*
	ss=1: deselect
	ss=0: select
*/
void mrf_select(char ss)
{
	ss=ss?1:0;
	PORTB=(PORTB&0xFE)|(ss);
}
unsigned char mrf_readwrite(unsigned char data)
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
/*
	Establish some communication with the radio module
*/

#define STSREG			0x0000		//
#define GENCREG		0x8000		// 8b
#define AFCCREG		0xC400		// 8b
#define TXCREG			0x9800		// 9b
#define TXBREG			0xB800		// 8b
#define CFSREG			0xA000		// 12b
#define RXCREG			0x9000		// 11b
#define BBFCREG		0xC200		// 8b
#define RXFIFOREG		0xB000		// 8b
#define FIFORSTREG	0xCA00		// 8b
#define SYNBREG		0xCE00		// 8b
#define DRSREG			0xC600		// 8b
#define PMCREG			0x8200		// 8b
#define WTSREG			0xE000		// 13b
#define DCSREG			0xC800		// 8b
#define BCSREG			0xC000		// 8b
#define PLLCREG		0xCC00		// 8b




unsigned short mrf_icommand(unsigned short command)
{
	unsigned short data;
	//unsigned short data2;
	

	data=mrf_readwrite(command>>8);
	data<<=8;
	data|=mrf_readwrite(command);

	//data2=mrf_readwrite(0);
	//data2=mrf_readwrite(command>>8);
	//data2<<=8;
	//data2|=mrf_readwrite(0);
	//data2|=mrf_readwrite(command);

	
	printf_P(PSTR("mrf_command %X. Read %X\r"),command,data);
	return data;
}

unsigned short mrf_command(unsigned short command)
{
	unsigned short data;
	//unsigned short data2;
	mrf_select(0);

	data=mrf_icommand(command);

	mrf_select(1);
	
	return data;
}

void mrf_icommands(unsigned short *commands,unsigned short n)
{
	unsigned short data;
	//unsigned short data2;
	for(unsigned short i=0;i<n;i++)
	{
		data=mrf_readwrite(commands[i]>>8);
		data<<=8;
		data|=mrf_readwrite(commands[i]);		
		printf_P(PSTR("CMD %X -> %X\r"),commands[i],data);
		//_delay_ms(1000);
	}
}

void mrf_commands(unsigned short *commands,unsigned short n)
{
	mrf_select(0);

	mrf_icommands(commands,n);

	mrf_select(1);

}


void mrf_reset_radio(void)
{
  mrf_command(FIFORSTREG | 0x0002);		
}
void mrf_power_down(void)
{
	mrf_command(0x8201);
}
void mrf_initspi(void)
{
	// MRF49XA:
	// clock must be < 2.5MHz
	// SPI mode 0,0: SCK 
	// SCK remain idle in low state (?)
	// Data is clocked in on the rising edge of SCK
	// Data is clocked out on the falling edge of SCK
	// MSB is sent first

	UBRR1=0;		// ??
	UCSR1C=0xC0;												// 0xC0: Master SPI, UDORD=0, UCPOL=0, UCPHA=0		// ok
	//UCSR1C=0xC4;												// 0xC4: Master SPI, UDORD=1, UCPOL=0, UCPHA=0
	//UCSR1C=0xC1;												// 0xC1: Master SPI, UDORD=0, UCPOL=1, UCPHA=0		
	//UCSR1C=0xC2;												// 0xC2: Master SPI, UDORD=0, UCPOL=0, UCPHA=1		// no data received (always 0)->likely phase wrong
	UCSR1B = (1<<RXEN1)|(1<<TXEN1);						// Enable receiver and transmitter
	UBRR1 = 255;												// Set baud rate. Important: baud rate must be set after the transmitter is enabled (see datasheet)
	// UBRR1=255: 1.5KHz @ 12MHz
}

unsigned short commands[17]={STSREG,GENCREG,AFCCREG,TXCREG,TXBREG,CFSREG,RXCREG,BBFCREG,RXFIFOREG,FIFORSTREG,SYNBREG,DRSREG,PMCREG,WTSREG,DCSREG,BCSREG,PLLCREG};
	unsigned short cmd_send[]={0x80d7,0x8239,0xa640,0xc647,
										0x94a0,0xc2ac,0xca81,0xc483,
										0x9850,0xe000,0xc800,0xc400,
										0x0000,0x8239,0xaaaa,0xaa2d,
										0xd400,0x8201};
void testradio_presence(void)
{
	// Configuration of the USART
	
	printf_P(PSTR("Setting up USART in SPI mode\r"));

	mrf_initspi();
	

	//unsigned short data;
	//mrf_select(0);

	

	//mrf_commands(cmd_send,18);


	uart_setblocking(file_usb,0);
	//unsigned char address=0;


	/*unsigned char eo=0;
	while(1)
	{
		if(eo&1)
		{
			//mrf_reset_radio();
			//mrf_command(0xfe00);
			printf_P(PSTR("Reset radio\r"));
		}
		eo++;
		for(int i=0;i<17;i++)
		{
			data = mrf_command(commands[i]);
			printf_P(PSTR("mrf command: %X %X\r"),i,data);
		}

		int c;
		if((c=fgetc(file_usb))!=EOF)
			break;
		_delay_ms(200);
	}*/
	//mrf_select(1);

	_delay_ms(2000);

	// Try to generate various clock outputs
	for(int i=0;i<8;i++)
	{
		printf_P(PSTR("Setting frequency %d\r"),i);
		mrf_command(BCSREG|(i<<5));
		_delay_ms(2000);
	}
	
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);	
}

void testradio_reset(void)
{
	mrf_initspi();

	printf_P(PSTR("Setting setup commands\r"));

	mrf_commands(cmd_send,18);
}

void testradio_reset2(void)
{
	mrf_initspi();

	printf_P(PSTR("Reseting\r"));

	mrf_reset_radio();
	//mrf_command(0xfe00);
}

void mrf_initradio_rx(void)
{
	
	/*
	// Microchip code
//---- Send init cmd
  mrf_command( FIFORSTREG | 0x81);							//	FIFO fill 8 bits, wordlong sync, fifo fill start: sync, fifo sync fill: stop, sensitive reset disabled
  mrf_command( FIFORSTREG | 0x83);							//	FIFO fill 8 bits, wordlong sync, fifo fill start: sync, fifo sync fill: sync, sensitive reset disabled
  mrf_command( GENCREG | 0x38);								// 0x8038
  mrf_command( CFSREG);
  mrf_command( PMCREG);
  mrf_command( RXCREG);
  mrf_command( TXCREG);	
//---- antenna tunning
  mrf_command( PMCREG | 0x0020);		// turn on tx
  _delay_ms(4);
//---- end of antenna tunning
  mrf_command( PMCREG | 0x0080);		// turn off Tx, turn on receiver
  mrf_command( GENCREG | 0x0040);		// enable the FIFO
  mrf_command( FIFORSTREG);
  mrf_command( FIFORSTREG | 0x0002);	// enable syncron latch	
  //RF_FSEL=1;

  */
	mrf_command(0x80D7);//EL,EF,11.5pF
	mrf_command(0x82D9);//!er,!ebb,ET,ES,EX,!eb,!ew,DC
	mrf_command(0xA640);//434MHz
	mrf_command(0xC647);//4.8kbps
	mrf_command(0x94A0);//VDI,FAST,134kHz,0dBm,-103dBm
	mrf_command(0xC2AC);//AL,!ml,DIG,DQD4
	mrf_command(0xCA81);//FIFO8,SYNC,!ff,DR
	mrf_command(0xC483);//@PWR,NO RSTRIC,!st,!fi,OE,EN
	mrf_command(0x9850);//!mp,9810=30kHz,MAX OUT
	mrf_command(0xE000);//NOT USE
	mrf_command(0xC800);//NOT USE
	mrf_command(0xC400);//1.66MHz,2.2V
}
void mrf_initradio_tx(void)
{
	mrf_command(0x80D7);//EL,EF,11.5pF
	mrf_command(0x8239);//!er,!ebb,ET,ES,EX,!eb,!ew,DC
	mrf_command(0xA640);//434MHz
	mrf_command(0xC647);//4.8kbps
	mrf_command(0x94A0);//VDI,FAST,134kHz,0dBm,-103dBm
	mrf_command(0xC2AC);//AL,!ml,DIG,DQD4
	mrf_command(0xCA81);//FIFO8,SYNC,!ff,DR
	mrf_command(0xC483);//@PWR,NO RSTRIC,!st,!fi,OE,EN
	mrf_command(0x9850);//!mp,9810=30kHz,MAX OUT
	mrf_command(0xE000);//NOT USE
	mrf_command(0xC800);//NOT USE
	mrf_command(0xC400);//1.66MHz,2.2V
}


void mrf_printstatus(void)
{
	unsigned short data = mrf_command(0x0000);
	printf_P(PSTR("Status: %X\r"),data);

}
void mrf_printirq(void)
{
	printf_P(PSTR("MRF IRQ: %d\r"),(PINC&16)?1:0);
}
unsigned char mrf_receive(void)
{
  unsigned short FIFO_data;

	
  while(PINC&(1<<4));					// WAIT_IRQ_LOW();
  
  mrf_command(0x0000);
  FIFO_data=mrf_command(RXFIFOREG);
  printf_P(PSTR("received %X\r"),FIFO_data);
  return FIFO_data;
}
unsigned char mrf_ireceive(void)
{
  unsigned short FIFO_data;

	
  while(PINC&(1<<4));					// WAIT_IRQ_LOW();
  
  mrf_icommand(0x0000);
  FIFO_data=mrf_icommand(RXFIFOREG);
  printf_P(PSTR("received %X\r"),FIFO_data);
  return FIFO_data;
}

void mrf_itransmit(unsigned char aByte)
{
	mrf_printirq();
	while(PINC&(1<<4));					// WAIT_IRQ_LOW();
	mrf_icommand(TXBREG | aByte);
}
void mrf_transmit(unsigned char aByte)
{
	mrf_select(0);
	mrf_itransmit(aByte);
	mrf_select(1);
}
void mrf_waitirq(void)
{
	while(PINC&(1<<4));					// WAIT_IRQ_LOW();
}

void testradio_rx1(void)
{
	unsigned char i;
	unsigned char ChkSum;

	printf_P(PSTR("Radio Test RX\r"));
	mrf_initspi();

	PORTC|=(1<<2);			// Set FFS# high when using TX register


	mrf_printstatus();
	mrf_printstatus();
	mrf_printstatus();
	mrf_printirq();

	mrf_initradio_rx();

	mrf_printstatus();
	mrf_printstatus();
	mrf_printstatus();
	mrf_printirq();

	
	
	
	mrf_command(FIFORSTREG|0x81);	//Init FIFO
	while(1)
	{
		//Enable FIFO
		mrf_command(FIFORSTREG|0x83);
		ChkSum=0;
		//Receive payload data
		for(i=0;i<16;i++)
		{
			unsigned char rx;
			printf_P(PSTR("Receiving byte %d\r"),i);
			mrf_printirq();
			rx = mrf_ireceive();
			printf_P(PSTR("Received %X\r"),rx);

			
			PORTD |=  _BV(PORT_LED1);			// Off
			_delay_ms(100);
			PORTD &=  ~(_BV(PORT_LED1));		// On
			


			ChkSum+=rx;
		}
		//Receive Check sum
		printf_P(PSTR("Receiving checksum...\r"));
		i=mrf_ireceive();
		PORTD |=  _BV(PORT_LED1);			// Off
		_delay_ms(100);
		PORTD &=  ~(_BV(PORT_LED1));		// On

		//Disable FIFO
		mrf_icommand(FIFORSTREG|0x81);
		//Package chkeck
		if(ChkSum==i)
		{
			printf_P(PSTR("Checksum matches!\r"));
		}
		_delay_ms(100);
	}

	
}


void testradio_tx1(void)
{
	unsigned char ChkSum;

	printf_P(PSTR("Radio Test TX\r"));
	mrf_initspi();

	mrf_printstatus();
	mrf_printstatus();
	mrf_printstatus();
	mrf_printirq();

	mrf_initradio_tx();

	mrf_printstatus();
	mrf_printstatus();
	mrf_printstatus();
	mrf_printirq();


	
	PORTC|=(1<<2);			// Set FFS# high when using TX register
	
	

	while(1)
	{
		printf_P(PSTR("Sending...\r"));
	
		mrf_select(0);
		
		mrf_icommand(0x0000);//read status register
		mrf_icommand(PMCREG|0x39);//!er,!ebb,ET,ES,EX,!eb,!ew,DC

		ChkSum=0;
		mrf_itransmit(0xAA);//PREAMBLE
		mrf_itransmit(0xAA);//PREAMBLE
		mrf_itransmit(0xAA);//PREAMBLE
		mrf_itransmit(0x2D);//SYNC HI BYTE
		mrf_itransmit(0xD4);//SYNC LOW BYTE
		mrf_itransmit(0x30);//DATA BYTE 0
		ChkSum+=0x30;
		mrf_itransmit(0x31);//DATA BYTE 1
		ChkSum+=0x31;
		mrf_itransmit(0x32);
		ChkSum+=0x32;
		mrf_itransmit(0x33);
		ChkSum+=0x33;
		mrf_itransmit(0x34);
		ChkSum+=0x34;
		mrf_itransmit(0x35);
		ChkSum+=0x35;
		mrf_itransmit(0x36);
		ChkSum+=0x36;
		mrf_itransmit(0x37);
		ChkSum+=0x37;
		mrf_itransmit(0x38);
		ChkSum+=0x38;
		mrf_itransmit(0x39);
		ChkSum+=0x39;
		mrf_itransmit(0x3A);
		ChkSum+=0x3A;
		mrf_itransmit(0x3B);
		ChkSum+=0x3B;
		mrf_itransmit(0x3C);
		ChkSum+=0x3C;
		mrf_itransmit(0x3D);
		ChkSum+=0x3D;
		mrf_itransmit(0x3E);
		ChkSum+=0x3E;
		mrf_itransmit(0x3F); //DATA BYTE 15
		ChkSum+=0x3F;
		mrf_itransmit(ChkSum); //send chk sum
		mrf_itransmit(0xAA);//DUMMY BYTE
		mrf_itransmit(0xAA);//DUMMY BYTE
		mrf_itransmit(0xAA);//DUMMY BYTE
		mrf_icommand(PMCREG|0x01);
		
		mrf_select(1);
	 	_delay_ms(1000);
	};

}

// TX2 and RX2 kinda work and transmit a byte from time to time

void testradio_tx2(void)
{
	unsigned short cmd_send[]={0x80d7,0x8239,0xa640,0xc647,
										0x94a0,0xc2ac,0xca81,0xc483,
										0x9850,0xe000,0xc800,0xc400,
										0x0000,0x8239};
	
	printf_P(PSTR("Radio Test TX\r"));
	mrf_initspi();

	mrf_printstatus();
	mrf_printstatus();
	mrf_printstatus();
	mrf_printirq();

		
	PORTC|=(1<<2);			// Set FFS# high when using TX register	
	
	while(1)
	{
		printf_P(PSTR("Sending\r"));
		mrf_select(0);
		mrf_icommands(cmd_send,14);

		mrf_itransmit(0xAA);
		mrf_itransmit(0xAA);
		mrf_itransmit(0xAA);
		mrf_itransmit(0x2D);
		mrf_itransmit(0xD4);

		mrf_itransmit('T');
		mrf_itransmit('D');
		mrf_itransmit('O');
	
		mrf_icommand(0x8201);
		mrf_select(1);
		//_delay_ms(1000);
	}
	
}
void testradio_rx2(void)
{
	unsigned short cmd_recv[]={0x80d7,0x82D9,0xa640,0xc647,
										0x94a0,0xc2ac,0xca81,0xc483,
										0x9850,0xe000,0xc800,0xc400,
										0xCA81};
	unsigned short FIFO_data;
	//unsigned char data;


	printf_P(PSTR("Radio Test RX\r"));
	mrf_initspi();



	mrf_setffs(1);					// Set FFS# high when using TX register



	mrf_printstatus();
	mrf_printstatus();
	mrf_printstatus();
	mrf_printirq();



	mrf_commands(cmd_recv,13);


	while(1)
	{
		mrf_select(0);
		mrf_icommand(0xCA83);			// FIFO activate
		while(1)
		{
			printf_P(PSTR("PINC: %02X PIND: %02X\r"),PINC,PIND);
			mrf_command(0x0000);
	  		FIFO_data=mrf_command(RXFIFOREG);
			printf("RCV: %X\n",FIFO_data);
			_delay_ms(100);
		}


		

		mrf_select(1);
		_delay_ms(100);

		/*mrf_select(0);
		mrf_icommand(0xCA83);			// FIFO activate
		while(1)
		{
			printf_P(PSTR("Receiving...\r"));
			data = mrf_ireceive();
			printf_P(PSTR("Received %X\r"),data);
		}
		mrf_icommand(0xCA81);			// FIFO activate
		mrf_icommand(0x0000);			
		mrf_select(1);
		_delay_ms(20);*/
	}

	mrf_select(1);
}


/**************
*//////////////
/*
#define XGENCREG 		0x8038		// Cload=12.5pF; TX registers & FIFO are disabled
#define XPMCREG 		0x8200		// Everything off, uC clk enabled
#define XRXCREG 		0x94A1		// BW=135kHz, DRSSI=-97dBm, pin8=VDI, fast VDI
#define XTXBREG 		0xB800
#define XFIFORSTREG	0xCA81		// Sync. latch cleared, limit=8bits, disable sensitive reset
#define XBBFCREG 		0xC22C		// Digital LPF (default)
#define XAFCCREG		0xC4D7		// Auto AFC (default)
//#define XCFSREG 		0xA7D0		// Fo=915.000MHz (default)
#define XCFSREG 		0xA640		// Fo=434.000MHz 
#define XTXCREG		0x9830		// df=60kHz, Pmax, normal modulation polarity 
#define XDRSREG 		0xC623		// 9579Baud (default)
*/
#define XGENCREG 		0x8038		// Cload=12.5pF; TX registers & FIFO are disabled
#define XPMCREG 		0x8200		// Everything off, uC clk enabled
#define XRXCREG 		0x94A0		
#define XTXBREG 		0xB800
#define XFIFORSTREG	0xCA81		
#define XBBFCREG 		0xC2AC		
//#define XAFCCREG		0xC4D7		// Auto AFC (default)
#define XAFCCREG		0xC483		
#define XCFSREG 		0xA640		// Fo=434.000MHz 
#define XTXCREG		0x9850		
#define XDRSREG 		0xC647		


void mrf_setffs(unsigned char v)
{
	if(v)
		PORTC|=(1<<2);			// Set FFS# high when using TX register
	else
		PORTC&=~(1<<2);		// Set FFS# low when using TX register
}

void testradio_tx3_sendpacket(void)
{
	//---- turn off receiver , enable Tx register
	mrf_command(XPMCREG);				// turn off the transmitter and receiver
	mrf_command(XGENCREG | 0x0080);		// Enable the Tx register
	//---- Packet transmission
	// Reset value of the Tx regs are [AA AA], we can start transmission
	//---- Enable Tx
	mrf_command(XPMCREG |0x0020);		// turn on tx

		mrf_select(0);							// chip select low
		mrf_waitirq();
			mrf_icommand(XTXBREG | 0xAA);	// preamble 
		mrf_waitirq();
			mrf_icommand(XTXBREG | 0x2D);	// sync pattern 1st byte
		mrf_waitirq();
			mrf_icommand(XTXBREG | 0xD4);	// sync pattern 2nd byte
		
		mrf_waitirq();
			mrf_icommand(XTXBREG | 66);	// length
		
		
		// payload

		mrf_waitirq();
		mrf_icommand(XTXBREG | 'T'); // write a byte to tx register
		mrf_waitirq();
		mrf_icommand(XTXBREG | 'D'); // write a byte to tx register
		mrf_waitirq();
		mrf_icommand(XTXBREG | 'O'); // write a byte to tx register
		mrf_waitirq();
		mrf_icommand(XTXBREG | 0x00); // write a dummy byte since the previous byte is still in buffer 		
		mrf_waitirq();
		
		
		mrf_select(1);						// chip select high, end transmission
		
	//---- Turn off Tx disable the Tx register
	mrf_command(XPMCREG | 0x0080);		// turn off Tx, turn on the receiver
	mrf_command(XGENCREG | 0x0040);		// disable the Tx register, Enable the FIFO
}


void testradio_tx3(void)
{
	mrf_initspi();
	mrf_command( XFIFORSTREG );
	mrf_command( XFIFORSTREG | 0x0002);
	mrf_command( XGENCREG);
	mrf_command( XCFSREG);
	mrf_command( XPMCREG);
	mrf_command( XRXCREG);
	mrf_command( XTXCREG);	
	//---- antenna tunning
	mrf_command( XPMCREG | 0x0020);		// turn on tx
	_delay_ms(4);
	//---- end of antenna tunning
	mrf_command( XPMCREG | 0x0080);		// turn off Tx, turn on receiver
	mrf_command( XGENCREG | 0x0040);		// enable the FIFO
	mrf_command( XFIFORSTREG);
	mrf_command( XFIFORSTREG | 0x0002);	// enable syncron latch	
	mrf_setffs(1);


	while(1)
	{
		testradio_tx3_sendpacket();
		_delay_ms(100);
	}
	


}
void testradio_rx3_receivepacket(void)
{
	

	while(1)
	{
		mrf_select(0);
		printf_P(PSTR("PINC: %X PIND: %X\r"),PINC&0x10,PIND&0x04);
		mrf_select(1);
		_delay_ms(100);
	}

	
}
void testradio_rx3(void)
{
	mrf_initspi();
	mrf_command( XFIFORSTREG );
	mrf_command( XFIFORSTREG | 0x0002);
	mrf_command( XGENCREG);
	mrf_command( XCFSREG);
	mrf_command( XPMCREG);
	mrf_command( XRXCREG);
	mrf_command( XTXCREG);	
	//---- antenna tunning
	mrf_command( XPMCREG | 0x0020);		// turn on tx
	_delay_ms(4);
	//---- end of antenna tunning
	mrf_command( XPMCREG | 0x0080);		// turn off Tx, turn on receiver
	mrf_command( XGENCREG | 0x0040);		// enable the FIFO
	mrf_command( XFIFORSTREG);
	mrf_command( XFIFORSTREG | 0x0002);	// enable syncron latch	
	mrf_setffs(1);

	testradio_rx3_receivepacket();

}

void testradio_rx(void)
{
	//testradio_rx1();
	testradio_rx2();
	//testradio_rx3();
}
void testradio_tx(void)
{
	//testradio_tx1();
	testradio_tx2();
	//testradio_tx3();
}

