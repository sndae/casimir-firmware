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

#include "wait.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>

/*
void wait_ms(unsigned int ms)
{

	unsigned int ocr, counter=0;
	unsigned char countL, countH;

	TCCR1A = 0x00;
	//if(ms > 65500/8)
	//	ms = 65500/8;
	//ocr = 8*ms; 		// Approx delay in ms with 1024 prescaler
	if(ms>65500/12)
		ms=65500/12;
	ocr = 12*ms;

	TCNT1H = 0;
	TCNT1L = 0;

	TIMSK1 = 0x00;		// no interrupt
	TCCR1B = 0x05;		// normal, prescaler=1024, start timer

	while(counter<ocr)
	{
		countL = TCNT1L;
		countH = TCNT1H;
		counter = (((unsigned int)countH) << 8) | (unsigned int)countL;
	}
	TCCR1B = 0x00;  	// disable timer
}
*/

/*
*/
/*
void timer_start(void)
{
	TCCR1A = 0x00;

	TCNT1H = 0;
	TCNT1L = 0;

	TIMSK1 = 0x00;		// no interrupt
	TCCR1B = 0x05;		// normal, prescaler=1024, start timer	
}
*/
/*
	Return the elapsed time since timer_start was called, in ~milliseconds (exactly 1.024ms/unit)
*/
/*
unsigned int timer_elapsed(void)
{
	unsigned char countL, countH;
	unsigned int counter;
	countL = TCNT1L;
	countH = TCNT1H;
	counter = (((unsigned int)countH) << 8) | (unsigned int)countL;
	return counter/12;	
}
*/

/*
	No prescaler, counts the CPU cycles.
*/
/*
void timer_start_fast(void)
{
	TCCR1A = 0x00;

	TCNT1H = 0;
	TCNT1L = 0;

	TIMSK1 = 0x00;		// no interrupt
	TCCR1B = 0x01;		// normal, prescaler=1024, start timer	
}
*/
/*
	Returns the elapsed time in CPU cycles
*/
/*
unsigned int timer_elapsed_fast(void)
{
	unsigned char countL, countH;
	unsigned int counter;
	countL = TCNT1L;
	countH = TCNT1H;
	counter = (((unsigned int)countH) << 8) | (unsigned int)countL;
	return counter;	
}
*/
/*void timer_rtc_start(void)
{
	TCCR2A = 0x00;

	ASSR=0x20;

	TCNT2H = 0;
	TCNT2L = 0;

	TIMSK1 = 0x00;		// no interrupt
	TCCR2B = 0x05;		// normal, prescaler=1024, start timer	
}*/
/*
	Return the elapsed time since timer_start was called, in ~milliseconds (exactly 1.024ms/unit)
*/
/*unsigned int timer_rtc_elapsed(void)
{
	unsigned char countL, countH;
	unsigned int counter;
	countL = TCNT1L;
	countH = TCNT1H;
	counter = (((unsigned int)countH) << 8) | (unsigned int)countL;
	return counter/8;	
}
*/



/*
	Interrupt-driven timer functions 
*/
volatile unsigned long int timer_millisecond;



/*
	Initializes timer 0 with approx a millisecond timer count.

	Override this function according to system clock
*/
void timer_ms_init(void)
{
	// Stop the timer
	TCCR0B = 0x00;		// Stop

	// Init the tick counter
	timer_millisecond = 0;

	// Set-up the timer and activate the timer interrupt		
	TCCR0A = 0x02;			// Clear timer on compare
	TCNT0 = 0;				// Clear timer
	OCR0A	=	46;			// CTC value, assuming 12MHz: 12MHz/(46+1)/256 -> 1.002ms
	TIMSK0 	= 0x02;		// Output compare A match interrupt
	TCCR0B 	= 0x04;		// Normal, prescaler = 256, start timer	
	

}
unsigned long int timer_ms_get(void)
{
	unsigned long int t;

	// disable interrupts
	TIMSK0	= 0x00;			// timer interrupt deactivated
	// Get the ticks
	t = timer_millisecond;
	// enable interrupts
	TIMSK0	= 0x02;			// timer interrupt activated

	return t;
}
/*
	Interrupt vector, timer 2 compare match
*/
ISR(TIMER0_COMPA_vect)
{
	timer_millisecond++;
}



