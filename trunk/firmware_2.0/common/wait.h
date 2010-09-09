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

#ifndef __WAIT_H
#define __WAIT_H

//void wait_ms(unsigned int ms);
//void timer_start(void);
//unsigned int timer_elapsed(void);
//void timer_start_fast(void);
//unsigned int timer_elapsed_fast(void);

/*
	Interrupt-driven timer functions 
*/
extern volatile unsigned long int timer_millisecond;
void timer_ms_init(void);
unsigned long int timer_ms_get(void);





#endif


