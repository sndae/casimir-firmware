/*
   MEGALOL - ATmega LOw level Library
	One Button User Interface Module
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
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "led.h"
#include "led.h"
#include "ui.h"
#include "helper.h"



// Return 1 if the push button is pressed, 0 otherwise.
#define PB0PRESS ((PINB&4)?0:1)
unsigned char _ui_pb0;											// Debounced state of switch
unsigned char _ui_state,_ui_newstate;
unsigned char _ui_option;
unsigned char _ui_elapsedctr,_ui_elapsedctrbtn;
volatile unsigned char _ui_active;
unsigned char _ui_optioncount;
unsigned char _ui_optionfirst;								// Code of the first displayed option (usually 0, i.e. 1 blink)
//unsigned char _ui_ledbkpmode[2],_ui_ledbkpstate[2];	// Backup of LED state
unsigned char _ui_dounoverride;
void (*_ui_callback)(unsigned char action, unsigned char option)=0;

void ui_callback_10hz(void);


// Timout 0: when performing button operations (i.e. must relase button within timeout, otherwise it wasn't an intended button action)
#define UI_INACTIVITYTIMEOUT0		20				
// Timout 1: when performing selection operations (i.e. must decide to select an item, or go to the next item within this timeout)
#define UI_INACTIVITYTIMEOUT1		200
#define UI_SHORTPRESSTIME			5
#define UI_LONGPRESSTIME			30


/*void ui_sethandler(void (*handler)(unsigned char,unsigned char))
{
	_ui_callback = handler;
}*/
/*
	Mode 0:	start in 'wait for attention' mode - it is followed by user input once attention is obtained
	Mode 1:	start in user input mode (not available yet)

	_first:	Number of the first option to display (0-based)
	_count:	Total number of options availables
*/
//void ui_start(unsigned char mode,unsigned char _first,unsigned char _count)
void ui_start(unsigned char mode,unsigned char _first,unsigned char _count,void (*handler)(unsigned char,unsigned char))
{
	ui_stop();

	cli();
	_ui_callback = handler;
	_ui_state=-1;
	_ui_dounoverride=0;
	_ui_newstate=0;
	_ui_optioncount = _count;
	_ui_optionfirst = _first;
	_ui_active=1;
	sei();

}
void ui_stop(void)
{
	cli();
	_ui_active=0;
	if(_ui_dounoverride)
	{
		_led_unoverride(0);
		_led_unoverride(1);
		_ui_dounoverride=0;
	}
	sei();
}

/*void ui_setoptioncount(unsigned char _first,unsigned char _count)
{

}*/


/*
	User interface state machine.

	State 0	Reset
	
	State 1	Standby state. Wait as long as needed for push button release. Release should be at lease N cycles. Go in standby state 2 afterwards.
		Action: 	LED0 blinks with low duty cycle. LED 1 is off (led state 1)
		Event:	Push button is pressed continuously for N cycles -> state 2

	State 2	Standby state. Wait for a long button press to go in state 3.
		Action: 	LED0 blinks with low duty cycle. LED 1 is off (led state 1)
		Event:	Push button is pressed continuously for N cycles -> state 2

	State 3	Attention state - push button is still pressed, wait for release
		Action:	LED0 on, LED1 off (led state 0, manually set)
		Event:	If push button is released for N cycles -> state 4
					If pressed > 5 second -> state 1
	
	State 4	Command state - push button is released, wait for start of press to go to state 6
		Action:	none
		Event:	If push button pressed after 200 ms -> state 5
					If time > 5 second -> state 1

	State 5	Command state - push button is pressed, wait for a release. See how long the press lasted and interpret as short or long.
		Action:	LED0 unchanged, LED1 displays the command
		Event:	
					If time > 5 second -> state 1

		  
*/
void ui_callback_10hz(void)
{
	static unsigned char lastpb0;
	unsigned char curpb0;

	// Key debounce
	curpb0 = PB0PRESS;
	_ui_pb0 = ((lastpb0^curpb0)&lastpb0) + (lastpb0&curpb0);
	lastpb0 = curpb0;	


	if(!_ui_active)
		return;

	//printf_P(PSTR("pb %d. s %d->%d. opt: %d c %d bc %d\r"),_ui_pb0,_ui_state,_ui_newstate,_ui_option,_ui_elapsedctr,_ui_elapsedctrbtn);

	// We have a state transition
	if(_ui_state!=_ui_newstate)
	{
		// Execute the transient actions when entering the state
		switch(_ui_newstate)
		{
			// Transient reset 
			case 0:
				_ui_newstate=1;
				_ui_dounoverride=0;
				// No break to transit to state 1

			// Transient to state 1: standby, wait for release
			case 1:			
				_ui_elapsedctr=0;
				_ui_elapsedctrbtn=0;
				// Unoverride the LEDs
				if(_ui_dounoverride)
				{
					_led_unoverride(0);
					_led_unoverride(1);
					_ui_dounoverride=0;
				}
				// Call user initialization upon entering the non-attention
				_ui_callback(0,0);
				break;

			// Transient to state 2: standby, wait for long press
			case 2:
				_ui_elapsedctr=0;
				_ui_elapsedctrbtn=0;
				break;

			// Transient to state 3: attention, wait for release
			case 3:
				_ui_elapsedctr=0;
				_ui_elapsedctrbtn=0;
				_ui_option=0;
				// Setup the LEDs
				_led_set(0,0);						// LED 0 off
				_led_set(1,1);						// LED 1 on				
				// Call user initialization upon entering the attention mode
				if(_ui_callback)
					_ui_callback(1,0);
				break;

			// Transient to state 4: command state: wait for short or long press
			case 4:
				_ui_elapsedctr=0;
				_ui_elapsedctrbtn=0;
				// Setup the LEDs
				_led_setmodeblink(0,1,2,_ui_option+_ui_optionfirst+1,12-3*(_ui_option+_ui_optionfirst),1);			// LED 0 on low duty cycle, number of blinks=option
				_led_set(1,1);						// LED 1 on
				break;

			case 5:
				// Don't reset the timers here.
				// Setup the LEDs
				//led_setmodemanual(0,1);						// LED 1 on
				break;
				
		}
	}

	_ui_state=_ui_newstate;

	// Implement the state machine 
	switch(_ui_state)
	{
		// Reset state: nothing.
		case 0:
			break;

		// State 1: Standby, wait until the button is released for at least N cycles. Then go to state 3.
		case 1:
			if(_ui_pb0)												// Button pressed... reset the counter
			{
				_ui_elapsedctrbtn=0;
			}
			else
			{
				_ui_elapsedctrbtn++;								// Button released, increment counter, and go to state 3 if enough time elapsed.
				if(_ui_elapsedctrbtn>=2)
				{
					_ui_newstate=2;
				}
			}
			break;

		// State 2: Standby, wait for a long button press. Then go to state 3.
		case 2:
			if(!_ui_pb0)												// Button released... reset the counter
			{
				_ui_elapsedctrbtn=0;
			}
			else	
			{
				_ui_elapsedctrbtn++;								// Button pressed... increment counter, and go to state 4 if enough time elapsed
				if(_ui_elapsedctrbtn>=UI_LONGPRESSTIME)
				{
					_ui_newstate=3;
					// Override the 'application' mode
					_ui_dounoverride=1;
					_led_override(0);
					_led_override(1);
					_led_setmodestatic(0);
					_led_setmodestatic(1);
					_led_set(0,0);									// LED 0 off - blinks LED0 if it was on in non attention mode. The turning on occurs in the transition to state 3
					_led_set(1,0);									// LED 1 off
				}
			}
			break;

		// State 3: Attention, wait for release. Then go to state 4.
		case 3:
			_ui_elapsedctr++;										// Count the total time elapsed in this state
			if(_ui_elapsedctr>UI_INACTIVITYTIMEOUT0)		// If nothing happened during long time (i.e. no transition to the next state) return to state 1
			{
				_ui_newstate=1;
			}
			else
			{
				if(_ui_pb0)											// Button pressed... reset the counter
				{
					_ui_elapsedctrbtn=0;
				}
				else	
				{
					_ui_elapsedctrbtn++;							// Button released, increment counter, and go to state 4 if enough time elapsed.
					if(_ui_elapsedctrbtn>=2)							
					{
						_ui_newstate=4;
					}
				}
			}
			break;
			
		// State 4: command state: wait for the start of a button press
		case 4:
		
			_ui_elapsedctr++;										// Count the time elapsed
			if(_ui_elapsedctr>UI_INACTIVITYTIMEOUT1)		// If the time elapsed without any action, return to state 1
			{
				_ui_newstate=1;
			}
			else
			{
				if(!_ui_pb0)												// Button released... reset the counter
				{
					_ui_elapsedctrbtn=0;
				}
				else	
				{
					_ui_elapsedctrbtn++;								// Button pressed... increment counter, and go to state 4 if enough time for a short press elapsed
					if(_ui_elapsedctrbtn>=UI_SHORTPRESSTIME)
					{
						
						_ui_newstate=5;
					}
				}
			}
			break;


		
		// State 5: command state, button was pressed, need to distinguish short or long press. Wait until a) release check length, b) pressed & time greater than long press time.
		case 5:
			_ui_elapsedctr++;										// Count the time elapsed
			_ui_elapsedctrbtn++;
			if(_ui_elapsedctr>UI_INACTIVITYTIMEOUT1)		// If the time elapsed without any action, return to state 1
			{
				_ui_newstate=1;
			}
			else
			{
				if(!_ui_pb0)											// Button released - look how long it was pressed.
				{
					_led_set(0,0);						// LED 1 off to indicate a short was recognized
					_led_set(1,0);						// LED 1 off to indicate a short was recognized
					// short press
					INCREMENT_WRAP(_ui_option,_ui_optioncount);
					_ui_newstate=4;	
					if(_ui_callback)
						_ui_callback(2,_ui_option+_ui_optionfirst);
				}
				else												// Button pressed - check if long enough for a long press
				{
					if(_ui_elapsedctrbtn>(UI_LONGPRESSTIME+(UI_LONGPRESSTIME>>1)))
					{
						// Both LED on to indicate action recognized
						_led_set(0,1);						
						_led_set(1,1);
						_ui_newstate=1;
						if(_ui_callback)
							_ui_callback(3,_ui_option+_ui_optionfirst);
					}
				}
			}
			break;

			
		default:
			break;
			
	}

}
