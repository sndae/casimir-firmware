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
#ifndef __SERIAL_H
#define __SERIAL_H


#include <stdio.h>

#define SERIAL_BUFFERSIZE 64
#define SERIAL_BUFFERSIZE_MASK (SERIAL_BUFFERSIZE-1)

/******************************************************************************
*******************************************************************************
BUFFER MANAGEMENT   BUFFER MANAGEMENT   BUFFER MANAGEMENT   BUFFER MANAGEMENT  
*******************************************************************************
******************************************************************************/

struct BUFFEREDIO
{
	unsigned char volatile buffer[SERIAL_BUFFERSIZE];
	unsigned char volatile wrptr,rdptr;
};

void buffer_put(volatile struct BUFFEREDIO *io, unsigned char c);
unsigned char buffer_get(volatile struct BUFFEREDIO *io);
unsigned char buffer_isempty(volatile struct BUFFEREDIO *io);
void buffer_clear(volatile struct BUFFEREDIO *io);
unsigned char buffer_isfull(volatile struct BUFFEREDIO *io);
unsigned char buffer_level(volatile struct BUFFEREDIO *io);



/******************************************************************************
*******************************************************************************
UART MANAGEMENT   UART MANAGEMENT   UART MANAGEMENT   UART MANAGEMENT   
******************************************************************************* 
******************************************************************************/
struct SerialData
{
	volatile struct BUFFEREDIO rx;
	volatile struct BUFFEREDIO tx;
};

/*struct FILEUDEV {
	unsigned char blocking;
	void (*callback)(void);
};*/

// Define callbacks for hooking into the interrupt routines.
// The callback is called first with the received character.
// The callback must return 1 to place the character in the receive queue, 0 if the character was processed otherwise.
extern unsigned char  (*uart0_rx_callback)(unsigned char);
extern unsigned char  (*uart1_rx_callback)(unsigned char);


extern volatile struct SerialData SerialData0;
extern volatile struct SerialData SerialData1;

int uart0_init(unsigned int ubrr, unsigned char u2x);
int uart1_init(unsigned int ubrr, unsigned char u2x);
void uart_setblocking(FILE *file,unsigned char blocking);

/*
	Direct serial access
*/
int uart0_fputchar(char ch,FILE* stream);
int uart0_fgetchar(FILE *stream);
int uart0_putchar(char ch);
int uart0_getchar(void);

int uart1_fputchar(char ch,FILE* stream);
int uart1_fgetchar(FILE *stream);
int uart1_putchar(char ch);
int uart1_getchar(void);


/*
	Interrupt-driven serial access
*/
int uart0_fputchar_int(char c, FILE*stream);
int uart1_fputchar_int(char c, FILE*stream);
int uart0_fgetchar_nonblock_int(FILE *stream);
int uart1_fgetchar_nonblock_int(FILE *stream);
int uart0_fgetchar_int(FILE *stream);
int uart1_fgetchar_int(FILE *stream);

unsigned char uart0_ischar_int(void);
unsigned char uart1_ischar_int(void);
unsigned char uart0_txbufferfree(void);
unsigned char uart1_txbufferfree(void);
int uart0_peek_int(void);
int uart1_peek_int(void);
void uart0_ungetch_int(unsigned char c);
void uart1_ungetch_int(unsigned char c);
void flush(FILE *f);



#endif
