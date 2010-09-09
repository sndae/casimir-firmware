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
#include <util/delay.h>

#include "init.h"
#include "serial.h"
#include "main.h"
#include "helper.h"
#include "core.h"
#include "global.h"
#include "wait.h"
#include "i2c.h"
#include "led.h"



// Setup ports
void init_InitPort(void)
{
	cli();          	// disable all interrupts

	DDRA  = INIT_DDRA;
	PORTA = INIT_PORTA;
	
	DDRB  = INIT_DDRB;
	PORTB = INIT_PORTB;
	
	DDRC  = INIT_DDRC;
	PORTC = INIT_PORTC;
	
	DDRD  = INIT_DDRD;
	PORTD = INIT_PORTD;
	
	sei();				// enable all interrupts
}

// Setup the sampling timer
//void init_SampleTimer(unsigned char config_samplerate)
//{
	// Setup the software divider
//	divider_software_sample = sr_software_divider_list[config_samplerate];
//	divider_software_life = life_timer_divider_list[config_samplerate];
	
	// Setup Timer 0 64,32 x pro sekunde 
	/*
	TCCR0A = 0x02;		// clear timer on compare
	OCR0A = sr_timer_divider_list[config_samplerate];
	TIMSK0 = 0x02;		// output compare A match interrupt
	TCCR0B = 0x05;		// prescaler=1024, start timer 0
	*/
//}



void init_Module(void)
{
	// Port initialization
	init_InitPort();

	// Specific module initialization
	init_Specific();


	DIDR0 = INIT_DIR0;			



	/*
		Timers

		Timer 0:		Millisecond timer interrupt. Used for timing purposes
		Timer 1:		Sample timer
		Timer 2:		Asynchronous timer, currently unused
		
	*/
	// Init the asynchronous clock
	/*
	TCCR2B = 0x00;		// Stop
	TCCR2A = 0x02;		// Clear timer on compare
	ASSR=0x20;
	TCNT2 = 0;
	//OCR2A	=	63;		// clear timer value
	//OCR2A	=	255;		// clear timer value
	OCR2A	=	3;		// clear timer value
	TIMSK2 	= 0x02;		// output compare A match interrupt
	TCCR2B 	= 0x01;		// normal, no prescaler, start timer	
	//TCCR2B 	= 0x02;		// normal, prescaler = 8, start timer
	*/
	// Init the timer1 (16-bit) as the audio sample
	TCCR1B = 0;											// Stop timer
	TCNT1 = 0;											// Clear counter
	//OCR1A = 29999;										// Top value. 400Hz @ 12MHz, No prescaler
	//OCR1A = 2999;										// Top value. 4KHz @ 12MHz, No prescaler
	OCR1A = 1999;										// Top value. 6KHz @ 12MHz, No prescaler
	//OCR1A = 1499;										// Top value. 8KHz @ 12MHz, No prescaler
	//OCR1A = 1332;										// Top value. 9KHz @ 12MHz, No prescaler
	//OCR1A = 1199;										// Top value. 10KHz @ 12MHz, No prescaler
	//OCR1A = 1090;										// Top value. 11KHz @ 12MHz, No prescaler
	//OCR1A = 999;										// Top value. 12KHz @ 12MHz, No prescaler
	//OCR1A = 856;										// Top value. 14KHz @ 12MHz, No prescaler
	//OCR1A = 749;										// Top value. 16KHz @ 12MHz, No prescaler
	//OCR1A = 374;										// Top value. 32KHz @ 12MHz, No prescaler
	//OCR1A = 272;										// Top value. 44KHz @ 12MHz, No prescaler
	TIMSK1 = (1<<OCIE1A);							// Output compare A interrupt enable
	TCCR1A = 0;
	TCCR1C = 0;
	TCCR1B = (1<<WGM12)|(1<<CS10);				// Clear timer on compare, no prescaler
	//TCCR1B = (1<<WGM12)|(1<<CS12)|(1<<CS10);// Clear timer on compare, 1024 prescaler
	//TCCR1B = (1<<WGM12)|(1<<CS12);				// Clear timer on compare, 256 prescaler
	


	// Init the millisecond timer
	timer_ms_init();

	// Init the SPI interface	
	//Remember to set MOSI and SCK output, MISO as input (see port init above)
	SPCR = 0x50;												// Enable SPI, master, clock=fosc/4 (spi2x=false) or clock=fosc/2 (spi2x=true)
	SPSR = 1;													// SPI2X
	//SPCR = 0x53;												// Enable SPI, master, clock=fosc/128 (spi2x=false) or clock=fosc/64 (spi2x=true)

	// Init the I2C interface
	i2c_init();
	
	// First life sign
	//LedInit();
	//LedOk(1);
	led_init();
	led_ok(1);

		


	// Communication setup
	// (8,1) 115200bps  @ 8Mhz
	// (16,1) 57600bps  @ 8Mhz
	// (3,0) 115200bps  @ 7.37 Mhz
	// (12,1) 115200bps @ 12MHz
	//uart0_init(16,1);		// 57k6 @ 8MHz
	//uart0_init(3,0);	// 115k2 @ 7.37MHz
	uart0_init(12,1);	// 115k2 @ 12MHz

	// Initialize the streams
	file_usb = fdevopen(uart0_fputchar_int,uart0_fgetchar_int);
	
	// Additional life sign
	Banner();
	

	

	
	//set_sleep_mode(SLEEP_MODE_IDLE);	
}



/*
	Applies the configuration 
*/
//void init_ApplyConfig(void)
//{
	// Setup streaming timer, packet counter, life sign
//	init_SampleTimer(config_samplerate);
//}

