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

#ifndef __LED_H
#define __LED_H



/******************************************************************************
	Legacy led functions.
******************************************************************************/
#define LED_TIME	200

void LifeSign(void);

/******************************************************************************
	Led manager functions
*******************************************************************************
	In order to use the led manager functions, the led_callback must be regularly
	called. Typically from an interrupt vector at 10Hz.
******************************************************************************/
// Internal use only
void _led_on(unsigned char led);
void _led_off(unsigned char led);
void _led_set(unsigned char led,unsigned char val);
void _led_enact(unsigned char led);
void _led_setmodestatic(unsigned char led);
void _led_setmodeblink(unsigned char led, unsigned char ton, unsigned char toff, unsigned char rep, unsigned char twait,unsigned char inf);
void _led_override(unsigned char led);
void _led_unoverride(unsigned char led);
// Public
void led_callback_10hz(void);
void led_enact(void);
void led_enact_nocli(void);
void led_on(unsigned char led);
void led_off(unsigned char led);
void led_set(unsigned char led,unsigned char val);
void led_toggle(unsigned char led);
void led_setmodestatic(unsigned char led);
void led_setmodeblink(unsigned char led, unsigned char ton, unsigned char toff, unsigned char rep, unsigned char twait,unsigned char inf);
void led_waitblink(unsigned char led);
// Legacy transition
void led_init(void);
void led_ok(unsigned char n);
void led_ok_repn(unsigned char n,unsigned char rep);
void led_fatal_error(unsigned char n);
void led_fatal_error_repn(unsigned char  n,unsigned char rep);
void led_fatal_error_block(unsigned char n);
void led_warning(unsigned char n);
void led_idle(void);
void led_idle_lowbat(void);

#endif 
