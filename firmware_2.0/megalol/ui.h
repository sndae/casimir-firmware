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


#ifndef __UI_H
#define __UI_H

//void ui_sethandler(void (*handler)(unsigned char,unsigned char));
//void ui_start(unsigned char mode);
void ui_start(unsigned char mode,unsigned char _first,unsigned char _count,void (*handler)(unsigned char,unsigned char));
void ui_stop(void);
//void ui_setoptioncount(unsigned char _first,unsigned char _count);
void ui_callback_10hz(void);


#endif
