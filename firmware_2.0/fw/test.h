/*
   CASIMIR - Firmware
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

#ifndef __TEST_H
#define __TEST_H

#include "fs.h"

void testanalog(void);
void testscani2c(void);
void teststreammike(unsigned char channel,unsigned char mode);
void teststreammikeGain(unsigned char gain,unsigned char mode);
void testtmp102(void);
void testmagn2(void);

void test_time(void);

void test_dummylog(FS_STATE *fs_state,unsigned long deviceid,unsigned long timeoffset);
void test_benchmarklog(FS_STATE *fs_state,unsigned long deviceid,unsigned long timeoffset);

void test_leds(void);

void test_ui(void);

#endif
