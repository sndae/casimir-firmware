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

#ifndef __CALIBRATION_H
#define __CALIBRATION_H

void PrintRealTime(FILE *file);
void Calibrate(FILE *file);
void StoreCalibrationData(void);
void LoadCalibrationData(void);
void PrintCalibrationData(FILE *file);	
signed short LSB2mg(unsigned short lsb,unsigned char s);


/*
	Calibration structure
	Gm1: ADC reading for -1g
	Gp1: ADC reading for +1g
	G0: ADC reading for 0g
*/
struct _CalibrationData
{
	unsigned short Gm1[3];		
	unsigned short Gp1[3];
	unsigned short G0[3];
};

#endif


