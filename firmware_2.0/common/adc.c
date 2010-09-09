/*
   MEGALOL - ATmega LOw level Library
	ADC Module
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
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "serial.h"
#include "wait.h"



unsigned short ReadADC(unsigned char channel)
{
	ADMUX = 0x40 | channel;		// Vref=VCC
	//timer_start_fast();
	//ADMUX = 0xC0 | channel;			// Vref=2.56V
	//ADMUX = 0x80 | channel;			// Vref=1.1V
	
	//ADCSRA = 0xC7;		// Start conversion, prescaler 128
	//ADCSRA = 0xC6;		// Start conversion, prescaler 64
	ADCSRA = 0xC4;		// Start conversion, prescaler 16
	//ADCSRA = 0xC3;		// Start conversion, prescaler 8
	while(ADCSRA&0x40);
	//unsigned int t = timer_elapsed_fast();
	//printf("conv t: %d\r\n",t);
	return ADCW;
}
/*
	Read ADC with noise reduction
*/
unsigned short ReadADCNR(unsigned char channel)
{
	ADMUX = 0x40 | channel;
	
	//ADCSRA = 0x8F;		// Start conversion, prescaler 128
	//ADCSRA = 0x8E;		// Start conversion, prescaler 64
	ADCSRA = 0x8C;		// Start conversion, prescaler 16
	//ADCSRA = 0x8B;			// Interrupt enable, prescaler 8
	//ADCSRA = 0x89;			// Interrupt enable, prescaler 2
	set_sleep_mode(SLEEP_MODE_IDLE);
	//set_sleep_mode(SLEEP_MODE_ADC);
	do
	{
		sleep_mode();
	}
	while(ADCSRA&0x40);
		
	return ADCW;
}

unsigned short ReadADCAvg(unsigned char channel)
{
	unsigned short r=0;
	unsigned short i=0;

	ADMUX = 0x40 | channel;
	

	//for(i=0;i<8;i++)
	//for(i=0;i<4;i++)
	for(i=0;i<2;i++)
	//for(i=0;i<1;i++)
	{
		ADCSRA = 0xC6;		// Start conversion, prescaler 64
		//ADCSRA = 0xC7;			// Start conversion, prescaler 128
		//ADCSRA = 0xC4;			// Start conversion, prescaler 16
		while(ADCSRA&0x40);
		r += ADCW;
	}
	//return (r>>=6);
	//return (r>>=3);
	//return (r>>=2);
	return (r>>=1);
}


unsigned short ReadADCAvgLong(unsigned char channel)
{
	unsigned long r=0;

	ADMUX = 0x40 | channel;
	
	for(unsigned short i=0;i<1024;i++)
	{
		ADCSRA = 0xC6;		// Start conversion
		while(ADCSRA&0x40);
		r += ADCW;
	}
	
	return (r>>=10);
}


