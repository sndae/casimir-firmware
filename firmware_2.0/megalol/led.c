/*
   MEGALOL - ATmega LOw level Library
	LED Manager
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
#include <util/delay.h>
#include <stdio.h>
#include "main.h"
#include "core.h"
#include "init.h"
#include "global.h"
#include "led.h"
#include "helper.h"


/******************************************************************************
*******************************************************************************
LED Legacy   LED Legacy   LED Legacy   LED Legacy   LED Legacy   LED Legacy   
*******************************************************************************
******************************************************************************/

void LifeSign(void)
{
	PORTD ^=  _BV(PORT_LED1);
	PORTD ^=  _BV(PORT_LED2);
}

/******************************************************************************
LED Manager   LED Manager   LED Manager   LED Manager   LED Manager   LED Manager   
*******************************************************************************

	Convention:
		Functions prefixed by _led_ are for internal use
		Functions prefixed by led_ are public

	Principles:
		Public functions to set the led state are executed only when calling led_enact. 
		This ensures synchronous, race-condition free update of the LED state or blink sequences (interrupts are deactivated during the update).

		functions led_setmodexxx and led_on/off only set a change flag and store the desired state in _led_app_xxxx

		Upon calling led_enact, the _led_app_xxx states are synchronously copied to the _led_next_xxx states
		If the LED isn't overridden, this is effected immediately.
		If the LED is overridden, the state stays in _led_next until the led is unoverridden.



	The LED manager performs according to two fundamentally different states:
		The default/normal/direct state (_led_isoverride=0) allows the LEDs to be controlled through the led_on, led_off, led_toggle, blink commands
		In "override" mode (_led_isoverride=1) the led commands (on, off, toggle, blink) used by the application are not affecting the leds. 
			They are buffered, and affect the led upon returning to the normal state.

	In normal state, the LED manager offers different modes:

		Mode 0: manual setting of the led
	
		Mode 1: blink sequence
			The blink sequence is: (On[TOn]; Off[Toff]) x rep times; Wait[Twait]) x repeat
	
			This sequence is implemented by a states machine with 3 states: 1=on, 2=off, 3=wait

			All the states last a minimum of 100 ms, even if the parameter is set to 0, except for the wait time that can be 0.

******************************************************************************/


// Buffering of the application command (_led_app_ prefix)
unsigned char _led_app_mode[2];															// Application desired mode. 0: controlled by the application. 1: blink sequence
unsigned char _led_app_state[2];																// Is the LED on or off.
unsigned char _led_app_ton[2],_led_app_toff[2],_led_app_rep[2],_led_app_w[2],_led_app_inf[2];	// Blink: Application-desired led on-time, off-time, repetition of the on-off blink, wait time before repeating the sequence

// Latched the next command upon calling led_enact (_led_next_ prefix)
unsigned char _led_next_mode[2];															// Application desired mode. 0: controlled by the application. 1: blink sequence
unsigned char _led_next_state[2];																// Is the LED on or off.
unsigned char _led_next_ton[2],_led_next_toff[2],_led_next_rep[2],_led_next_w[2],_led_next_inf[2];	// Blink: Application-desired led on-time, off-time, repetition of the on-off blink, wait time before repeating the sequence

// Currently active parameters
unsigned char _led_mode[2];																// Effective mode. 0: controlled by the application. 1: blink sequence
unsigned char _led_isoverride[2];														// 0: controlled by application. 1: override (non-internal commands have no direct effect on LEDs or blink sequences)
unsigned char _led_ton[2],_led_toff[2],_led_rep[2],_led_w[2],_led_inf[2];	// Blink: Effective led on-time, off-time, repetition of the on-off blink, wait time before repeating the sequence
unsigned char _led_change[2];																// Indicate whether there has been a change in the config
unsigned char _led_do[2];																	// Indicate whether changes are to be enacted



// Interrupt vector variables prefixed by _led_i
unsigned char _led_istate[2],_led_istate_ctr[2],_led_istate_rctr[2];			// Deeper internal state for blink sequence
unsigned char _led_istatedotrans[2];													// Execute the transient code.
volatile unsigned char _led_idone[2];													// Indicates the sequence is done




/******************************************************************************
	Internal led control
*******************************************************************************
	These functions have an immediate effect on the LED.

	They do not deactivate/reactivate interrupts. They can be called from interrupt vectors.

	They are not meant to be called by the user application. 
	Exception: the UI manager can call these functions within it's interrupt routine.

******************************************************************************/
void _led_on(unsigned char led)
{
	if(led==0)
		PORTD &=  ~_BV(PORT_LED1);				// On
	if(led==1)
		PORTD &=  ~_BV(PORT_LED2);				// On
}
void _led_off(unsigned char led)
{
	if(led==0)
		PORTD |=  _BV(PORT_LED1);				// On
	if(led==1)
		PORTD |=  _BV(PORT_LED2);				// On
}
void _led_toggle(unsigned char led)
{
	if(led==0)
		PORTD ^=  _BV(PORT_LED1);				// On
	if(led==1)
		PORTD ^=  _BV(PORT_LED2);				// On
}
void _led_set(unsigned char led,unsigned char val)
{
	if(val==1)
		_led_on(led);
	else
		_led_off(led);
}

void _led_setmodestatic(unsigned char led)
{
	_led_mode[led]=0;
}
/*
	Set a blink sequence
*/
void _led_setmodeblink(unsigned char led, unsigned char ton, unsigned char toff, unsigned char rep, unsigned char twait,unsigned char inf)
{
	_led_ton[led]=ton;
	_led_toff[led]=toff;
	_led_rep[led]=rep;
	_led_w[led]=twait;	
	_led_inf[led]=inf;
	_led_idone[led]=0;

	_led_istate[led]=0;					// Start internal state is reset
	_led_istatedotrans[led]=1;			// Execute

	_led_mode[led]=1;
}
void _led_override(unsigned char led)
{
	_led_isoverride[led]=1;
}
void _led_unoverride(unsigned char led)
{
	_led_isoverride[led]=0;		
	_led_enact(led);				// Setup the led according to buffered application request
}
// Internal led enactment: setup the led according to 'next' states
void _led_enact(unsigned char led)
{
	// This led must be enacted.
	if(_led_next_mode[led]==0)
	{
		// Static mode
		_led_setmodestatic(led);
		_led_set(led,_led_next_state[led]);
	}
	else
	{
		// Blink mode
		_led_setmodeblink(led,_led_next_ton[led],_led_next_toff[led],_led_next_rep[led],_led_next_w[led],_led_next_inf[led]);
	}
}
/******************************************************************************
	Public direct led control: overriden by non-manual mode
*******************************************************************************
	


******************************************************************************/


/******************************************************************************
	led_enact: executes the LED commands.
*******************************************************************************
	The led commands are executed when _led_override==0 and _led_change==1.
	
	Interrupts are deactivated to ensure synchronous updates (important for the blink mode)
	
	the _nocli variant does not disable/reenable interrupts, and thus can be used 
	in an interrupt vector to avoid enabling nested interrupts
		
*******************************************************************************/
 
void led_enact_nocli(void)
{
	// Iterate the LEDs
	for(unsigned char i=0;i<2;i++)
	{
		// Copy all the application parameters to the 'next' parameters
		_led_next_mode[i]=_led_app_mode[i];
		_led_next_state[i]=_led_app_state[i];
		_led_next_ton[i]=_led_app_ton[i];
		_led_next_toff[i]=_led_app_toff[i];
		_led_next_rep[i]=_led_app_rep[i];
		_led_next_w[i]=_led_app_w[i];
		_led_next_inf[i]=_led_app_inf[i];

		// call enact if not overriden and desire to change
		if(!_led_isoverride[i] && _led_change[i])
		{
			_led_enact(i);
			_led_change[i]=0;
		}
	}
}

void led_enact(void)
{
	cli();					// No interrupts
	led_enact_nocli();
	sei();					// Interrupts
}
// Buffer the led command when the mode matches.
void led_on(unsigned char led)
{

	_led_app_state[led]=1;
	_led_change[led]=1;
}
// Buffer the led command when the mode matches.
void led_off(unsigned char led)
{
	_led_app_state[led]=0;
	_led_change[led]=1;
}
// Buffer the led command when the mode matches.
void led_set(unsigned char led,unsigned char val)
{
	_led_app_state[led]=val;
	_led_change[led]=1;
}
void led_toggle(unsigned char led)
{
	_led_app_state[led]=_led_app_state[led]?0:1;	
	_led_change[led]=1;
}

/*
	Set the led in manual mode
*/
void led_setmodestatic(unsigned char led)
{
	_led_app_mode[led]=0;
	_led_change[led]=1;
}

/*
	Set a blink sequence
*/
void led_setmodeblink(unsigned char led, unsigned char ton, unsigned char toff, unsigned char rep, unsigned char twait,unsigned char inf)
{
	_led_app_mode[led]=1;

	_led_app_ton[led]=ton;
	_led_app_toff[led]=toff;
	_led_app_rep[led]=rep;
	_led_app_w[led]=twait;	
	_led_app_inf[led]=inf;

	_led_change[led]=1;
}

/*
	Wait until a (non infinite) blink sequence has completed.
	If in override mode, waits that both the blink sequence and override mode complete
*/
void led_waitblink(unsigned char led)
{
	while(!_led_idone[led] || _led_isoverride[led]);
}

/******************************************************************************
Legacy transition led control
******************************************************************************/

void led_init(void)
{
	led_setmodestatic(0);
	led_setmodestatic(1);
	led_set(0,1);
	led_set(1,0);
	led_enact();
}
void led_idle(void)
{
	led_setmodeblink(0,1,19,1,0,1);			// LED 0 on low duty cycle
	led_setmodestatic(1);						// LED 1 off
	led_set(1,0);									// LED 1 off
	led_enact();
}
void led_idle_lowbat(void)
{
	led_setmodeblink(0,1,19,1,0,1);			// LED 0 on low duty cycle
	led_setmodeblink(1,1,39,1,0,1);			// LED 1 on low duty cycle
	led_enact();
}
void led_ok(unsigned char n)
{
	led_init();

	for(unsigned char  i=0;i<n;i++)
	{
		_delay_ms(LED_TIME);
		led_set(1,1);
		led_enact();
		_delay_ms(LED_TIME);
		led_set(1,0);
		led_enact();
	}
	_delay_ms(LED_TIME);
}
void led_ok_repn(unsigned char n,unsigned char rep)
{
	led_init();

	for(unsigned char r=0;r<rep;r++)
	{

		for(unsigned char  i=0;i<n;i++)
		{
			_delay_ms(LED_TIME);
			led_set(1,1);
			led_enact();
			_delay_ms(LED_TIME);
			led_set(1,0);
			led_enact();
		}
		_delay_ms(5*LED_TIME);
	}
}
// Blocking fatal error 
void led_fatal_error_block(unsigned char  n)		
{
	led_init();

	while(1)
	{
		for(unsigned char  i=0;i<n;i++)
		{
			_delay_ms(LED_TIME);
			led_set(0,1);
			led_set(1,1);
			led_enact();
			_delay_ms(LED_TIME);		
			led_set(0,0);
			led_set(1,0);
			led_enact();
		}
		_delay_ms(1000);
	}
}
// Non blobking fatal error 
void led_fatal_error(unsigned char  n)		
{
	led_init();
	// Set the blink code
	led_setmodeblink(0,LED_TIME/100,LED_TIME/100,n,1000/100,1);
	led_setmodeblink(1,LED_TIME/100,LED_TIME/100,n,1000/100,1);
	led_enact();
}
// Non blobking fatal error, non-infinitum repeat rep, wait till complete
void led_fatal_error_repn(unsigned char  n,unsigned char rep)
{
	_delay_ms(LED_TIME);
	led_init();

	for(unsigned r=0;r<rep;r++)
	{
		// Set the blink code
		led_setmodeblink(0,LED_TIME/100,LED_TIME/100,n,500/100,0);
		led_setmodeblink(1,LED_TIME/100,LED_TIME/100,n,500/100,0);
		led_enact();

		led_waitblink(0);
		led_waitblink(1);
	}
}
void led_warning(unsigned char n)
{
	led_init();
	for(unsigned char  i=0;i<n;i++)
	{
		_delay_ms(LED_TIME);
		led_set(0,0);
		led_enact();
		_delay_ms(LED_TIME);
		led_set(0,1);
		led_enact();
	}
}
/******************************************************************************
	Led manager callback
*******************************************************************************
	Must be called at regular intervals. Typicalls from an interrupt vector at about 10Hz.
******************************************************************************/
void led_callback_10hz(void)
{

	// Process the 2 leds
	for(unsigned char l=0;l<2;l++)
	{
		switch(_led_mode[l])
		{
			// Case 0: LED in static mode. 
			case 0:
				break;
			// Case 1: LED in blink sequence mode
			case 1:
				//printf_P(PSTR("LED %d\r"),l);
				//printf_P(PSTR("B dotrans %d istate %d ctr %d rctr %d inf %d\r"),_led_istatedotrans[l],_led_istate[l],_led_istate_ctr[l],_led_istate_rctr[l],_led_inf[l]);


				// Transient code (entering a state)
				if(_led_istatedotrans[l])
				{
					_led_istatedotrans[l]=0;
					switch(_led_istate[l])
					{
						// Reset transition
						case 0:		
							_led_istate[l]=1;				// First state after reset
						
							// Reset initialization
							_led_istate_rctr[l]=0;
						

							// No break on purpose to ensure initialization, and entering in the first state upon power up.

						// Transition to LED on
						case 1:
							_led_on(l);
							_led_istate_ctr[l]=0;
							break;
						// Transition to LED off
						case 2:
							_led_off(l);
							_led_istate_ctr[l]=0;
							break;
						// Transition to wait
						case 3:
							_led_istate_ctr[l]=0;
							_led_istate_rctr[l]=0;
							break;
						// Transition to end of sequence repetition
						case 4:
							_led_idone[l]=1;
					}
				}

				// State code (in a state)
				switch(_led_istate[l])
				{
					// State LED on
					case 1:		
						INCREMENT_WRAP(_led_istate_ctr[l],_led_ton[l]);
						if(!_led_istate_ctr[l])										// Time elapsed
						{
							_led_istate[l]=2;	
							_led_istatedotrans[l]=1;
						}
						break;		
					// State LED off		
					case 2:
						INCREMENT_WRAP(_led_istate_ctr[l],_led_toff[l]);
					
						if(!_led_istate_ctr[l])										// Time elapsed
						{
							// Go to wait, or repeat?
							INCREMENT_WRAP(_led_istate_rctr[l],_led_rep[l]);


							// We blink on again if: the sequence is not complete, or the sequence is complete but we should not wait at the end of the sequence, and we want infinite blinks
							if(_led_istate_rctr[l] || (_led_w[l]==0 && _led_inf[l]==1))
							{							
								_led_istate[l]=1;
								_led_istatedotrans[l]=1;
							}
							else
							{
								if(_led_inf[l]==0 && _led_w[l]==0)			// no need to wait and no infinite loop -> end
								{
									_led_istate[l]=4;
									_led_istatedotrans[l]=1;
								}
								else													// need to wait, state 3: wait
								{
									_led_istate[l]=3;
									_led_istatedotrans[l]=1;
								}
							}
						}
						break;
					// State wait
					case 3:
						INCREMENT_WRAP(_led_istate_ctr[l],_led_w[l]);
						if(!_led_istate_ctr[l])										// Time elapsed
						{
							// wait is done
							if(_led_inf[l]==1)										// infinite blinks: state1
							{
								_led_istate[l]=1;					
								_led_istatedotrans[l]=1;
							}
							else
							{
								_led_istate[l]=4;					
								_led_istatedotrans[l]=1;
							}
						}
						break;
					
						
				}

				//printf_P(PSTR("B t%d s%d c%d r %d\r\r"),_led_istate0dotrans,_led_istate0,_led_istate0_ctr,_led_istate0_rctr);
				break;
		}
	}
}
