/*
   MEGALOL - ATmega LOw level Library
	Serial Module
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
/*
	Serial functions
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "serial.h"
#include "main.h"

volatile struct SerialData SerialData0;
volatile struct SerialData SerialData1;


/******************************************************************************
*******************************************************************************
*******************************************************************************
BUFFER MANAGEMENT   BUFFER MANAGEMENT   BUFFER MANAGEMENT   BUFFER MANAGEMENT  
*******************************************************************************
*******************************************************************************
******************************************************************************/

void buffer_put(volatile struct BUFFEREDIO *io, unsigned char c)
{
	io->buffer[io->wrptr]=c;
	io->wrptr=(io->wrptr+1)&SERIAL_BUFFERSIZE_MASK;
}
unsigned char buffer_get(volatile struct BUFFEREDIO *io)
{
	unsigned char c;
	c = io->buffer[io->rdptr];
	io->rdptr=(io->rdptr+1)&SERIAL_BUFFERSIZE_MASK;
	return c;
}
/*
Ungets a character.
In order to keep correct byte ordering, byte is unget at 'rdptr-1'.
(It is not possible to use 'buffer_put' for this purpose, as byte ordering would end up mixed).
*/
unsigned char buffer_unget(volatile struct BUFFEREDIO *io,unsigned char c)
{
	io->rdptr=(io->rdptr-1)&SERIAL_BUFFERSIZE_MASK;
	io->buffer[io->rdptr]=c;	
	return c;
}
unsigned char buffer_isempty(volatile struct BUFFEREDIO *io)
{
	if(io->rdptr==io->wrptr)
		return 1;
	return 0;
}
void buffer_clear(volatile struct BUFFEREDIO *io)
{
	io->wrptr=io->rdptr=0;
}
// We loose 1 character in the buffer because rd=wr means empty buffer, and 
// wr+1=rd means buffer full (whereas it would actually mean that one more byte can be stored).
unsigned char buffer_isfull(volatile struct BUFFEREDIO *io)
{
	if( ((io->wrptr+1)&SERIAL_BUFFERSIZE_MASK) == io->rdptr )
		return 1;
	return 0;
}
// Indicates how many bytes are in the buffer
unsigned char buffer_level(volatile struct BUFFEREDIO *io)
{
	return ((io->wrptr-io->rdptr)&SERIAL_BUFFERSIZE_MASK);
}
// Indicates how many free space is in the buffer - we still loose one byte.
unsigned char buffer_freespace(volatile struct BUFFEREDIO *io)
{
	return SERIAL_BUFFERSIZE-buffer_level(io)-1;
}




/******************************************************************************
*******************************************************************************
*******************************************************************************
UART MANAGEMENT   UART MANAGEMENT   UART MANAGEMENT   UART MANAGEMENT   
*******************************************************************************
******************************************************************************* 
******************************************************************************/
// Callbacks return 1 to put the data in the receive buffer, 0 if the data must be discarded 
unsigned char  (*uart0_rx_callback)(unsigned char)=0;
unsigned char  (*uart1_rx_callback)(unsigned char)=0;

/******************************************************************************
*******************************************************************************
INITIALIZATION   INITIALIZATION   INITIALIZATION   INITIALIZATION   INITIALIZATION   
*******************************************************************************
******************************************************************************/
int uart0_init(unsigned int ubrr, unsigned char u2x)
{
	buffer_clear(&SerialData0.rx);
	buffer_clear(&SerialData0.tx);

	// Setup the serial port
	UCSR0B = 0x00; 				  //disable while setting baud rate
	UCSR0C = 0x06; 				  // Asyn,NoParity,1StopBit,8Bit,

	if(u2x)
		UCSR0A = 0x02;				  // U2X = 1
	else
		UCSR0A = 0x00;
		
	UBRR0H = (unsigned char)((ubrr>>8)&0x000F);
	UBRR0L = (unsigned char)(ubrr&0x00FF);	

	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);			// RX/TX Enable, RX/TX Interrupt Enable

	return 1;
}

int uart1_init(unsigned int ubrr, unsigned char u2x)
{
	buffer_clear(&SerialData1.rx);
	buffer_clear(&SerialData1.tx);

	// Setup the serial port
	UCSR1B = 0x00; 				  //disable while setting baud rate
	UCSR1C = 0x06; 				  // Asyn,NoParity,1StopBit,8Bit,

	if(u2x)
		UCSR1A = 0x02;				  // U2X = 1
	else
		UCSR1A = 0x00;
		
	UBRR1H = (unsigned char)((ubrr>>8)&0x000F);
	UBRR1L = (unsigned char)(ubrr&0x00FF);			

	UCSR1B = (1<<RXCIE1)|(1<<RXEN1)|(1<<TXEN1);			// RX/TX Enable, RX/TX Interrupt Enable

	// If RTS-enabled communication, enable interrupt on pin change
#if ENABLE_BLUETOOTH_RTS
	PCMSK3 |= 1<<PCINT28;	// PCINT28 (PortD.4) pin change interrupt enable
	PCICR |= 1<<PCIE3;		// Enable pin change interrupt for PCINT31..24
#endif

	return 1;
}

/*
	Sets whether the read functions must be blocking or non blocking.
	In non-blocking mode, when no characters are available, read functions return EOF.
*/
void uart_setblocking(FILE *file,unsigned char blocking)
{
	fdev_set_udata(file,(void*)(blocking?0:1));
}

/******************************************************************************
*******************************************************************************
DIRECT ACCESS   DIRECT ACCESS   DIRECT ACCESS   DIRECT ACCESS   DIRECT ACCESS  
******************************************************************************* 
******************************************************************************/
int uart0_putchar(char ch)		// Placeholder
{
	return uart0_fputchar(ch,0);
}

int uart0_getchar()				// Placeholder
{
	return uart0_fgetchar(0);
}

int uart0_fputchar(char ch,FILE* stream)
{
	 while(!(UCSR0A & 0x20)); // wait for empty transmit buffer
	 UDR0 = ch;     	 		 // write char
	return 0;
}

int uart0_fgetchar(FILE *stream)
{
	char c;
	while(!(UCSR0A & 0x80)); // wait for receive complete
	c=UDR0;
	return c;
}

int uart1_putchar(char ch)		// Placeholder
{
	return uart1_fputchar(ch, 0);
}

int uart1_getchar()				// Placeholder
{
	return uart1_fgetchar(0);
}

int uart1_fputchar(char ch,FILE* stream)
{
	while(!(UCSR1A & 0x20)); // wait for empty transmit buffer
	UDR1 = ch;     	 		 // write char
	return 0;
}

int uart1_fgetchar(FILE *stream)
{
	char c;
	while(!(UCSR1A & 0x80)); // wait for receive complete
	c=UDR1;
	return c;
}

/******************************************************************************
*******************************************************************************
INTERRUPT-DRIVEN ACCESS   INTERRUPT-DRIVEN ACCESS   INTERRUPT-DRIVEN ACCESS   
*******************************************************************************
******************************************************************************/





int uart0_fputchar_int(char c, FILE*stream)
{
	// Wait until send buffer is free. 
	while( buffer_isfull(&SerialData0.tx) );
	// Store the character in the buffer
	buffer_put(&SerialData0.tx,c);
	
	// Trigger an interrupt
	UCSR0B|=(1<<UDRIE0);		// will generate interrupt when UDR is empty
	return 0;
}

int uart1_fputchar_int(char c, FILE*stream)
{
	// Wait until send buffer is free. 
	while( buffer_isfull(&SerialData1.tx) );
	// Store the character in the buffer
	buffer_put(&SerialData1.tx,c);
	
	// Trigger an interrupt
	UCSR1B|=(1<<UDRIE1);		// will generate interrupt when UDR is empty
	return 0;
}

int uart0_fgetchar_nonblock_int(FILE *stream)
{
	char c;
	if(buffer_isempty(&SerialData0.rx))
		return EOF;
	c = buffer_get(&SerialData0.rx);
	return ((int)c)&0xff;
}

int uart1_fgetchar_nonblock_int(FILE *stream)
{
	char c;
	if(buffer_isempty(&SerialData1.rx))
		return EOF;
	c = buffer_get(&SerialData1.rx);
	return ((int)c)&0xff;
}


int uart0_fgetchar_int(FILE *stream)
{
	int c;
	do{ c=uart0_fgetchar_nonblock_int(stream);}
	while(c==EOF && fdev_get_udata(stream)==0);
	return c;
}
int uart1_fgetchar_int(FILE *stream)
{
	int c;
	do{c=uart1_fgetchar_nonblock_int(stream);}
	while(c==EOF && fdev_get_udata(stream)==0);
	return c;
}


/******************************************************************************
Interrupt vectors
******************************************************************************/
ISR(USART0_UDRE_vect)
{
	if(buffer_isempty(&SerialData0.tx))					// No data to transmit
	{
		UCSR0B&=~(1<<UDRIE0);		// Deactivate interrupt otherwise we reenter continuously the loop
		return;
	}
	// Write data
	UDR0 = buffer_get(&SerialData0.tx);
}
ISR(USART1_UDRE_vect)
{
	// If RTS is enabled, and RTS is set, clear the interrupt flag and return (i.e. do nothing because the receiver is busy)
#if ENABLE_BLUETOOTH_RTS
	if(PIND&0x10)
	{
		UCSR1B&=~(1<<UDRIE1);								// Deactivate interrupt otherwise we reenter continuously the loop
		return;
	}
#endif
	
	if(buffer_isempty(&SerialData1.tx))					// No data to transmit
	{
		UCSR1B&=~(1<<UDRIE1);								// Deactivate interrupt otherwise we reenter continuously the loop
		return;
	}
	// Write data
	UDR1 = buffer_get(&SerialData1.tx);
}

/*
	Interrupt: data received
*/
ISR(USART0_RX_vect)
{
	unsigned char c=UDR0;
	if(uart0_rx_callback==0 || (*uart0_rx_callback)(c)==1)
		buffer_put(&SerialData0.rx,c);
	
}
ISR(USART1_RX_vect)
{
	unsigned char c=UDR1;
	if(uart1_rx_callback==0 || (*uart1_rx_callback)(c)==1)
		buffer_put(&SerialData1.rx,c);
}


/*
	RTS-enabled communication with the bluetooth chip
*/
#if ENABLE_BLUETOOTH_RTS
	/*
		Interrupt vector for RTS pin change on UART 1 (Bluetooth)
		We catch all the PCINT3, which corresponds to pins 31..24, but only PCINT28 is activated in this code.
	*/
	ISR(PCINT3_vect)				// PortD.4 interrupt on change
	{
		if(PIND&0x10)			// Trigger a bluetooth interrupt on high to low transitions. If we are high, we return.
			return;
		// todo check transition....
		UCSR1B|=(1<<UDRIE1);		// will generate interrupt when UDR is empty
	}
#endif


/******************************************************************************
	Stream manipulation
******************************************************************************/
unsigned char uart0_ischar_int(void)
{
	if(buffer_isempty(&SerialData0.rx))
		return 0;
	return 1;
}
unsigned char uart1_ischar_int(void)
{
	if(buffer_isempty(&SerialData1.rx))
		return 0;
	return 1;
}
unsigned char uart0_txbufferfree(void)
{
	return buffer_freespace(&SerialData0.tx);
}
unsigned char uart1_txbufferfree(void)
{
	return buffer_freespace(&SerialData1.tx);
}

int uart0_peek_int(void)
{
	return SerialData0.rx.buffer[SerialData0.rx.rdptr];
}

int uart1_peek_int(void)
{
	return SerialData1.rx.buffer[SerialData1.rx.rdptr];
}

void uart0_ungetch_int(unsigned char c)
{
	buffer_unget(&SerialData0.rx,c);
}
void uart1_ungetch_int(unsigned char c)
{
	buffer_unget(&SerialData1.rx,c);
}
/*
	Flushes f by reading until empty
*/
void flush(FILE *file)
{
	int c;
	// Non-blocking mode
	uart_setblocking(file,0);
	while((c=fgetc(file))!=EOF);
	// Return to blocking mode
	uart_setblocking(file,1);
}
