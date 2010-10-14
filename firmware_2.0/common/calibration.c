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
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "wait.h"
#include "adc.h"
#include "calibration.h"
#include "main.h"

// Various
const char msg_capture[] PROGMEM = "Captured\r\n";

struct _CalibrationData CalibrationData;

/*
	Calibration data are stored in EEPROM starting at address 0 as follows (2 bytes per entry:
	Gm1X | G0X | Gp1X | Gm1Y | G0Y | Gp1Y | Gm1Z | G0Z | Gp1Z
*/
void PrintRealTime(FILE *file)
{
/*	_delay_ms(250);
	fprintf_P(file,PSTR("ADC X/Y/Z: %03u %03u %03u\r\n"),ReadADCAvg(PORT_ACCX),ReadADCAvg(PORT_ACCY),ReadADCAvg(PORT_ACCZ));*/
}

void StoreCalibrationData(void)
{
	unsigned char i;
	for(i=0;i<3;i++)
	{
		eeprom_write_word((uint16_t*)(0+i*6),CalibrationData.Gm1[i]);
		eeprom_write_word((uint16_t*)(2+i*6),CalibrationData.G0[i]);
		eeprom_write_word((uint16_t*)(4+i*6),CalibrationData.Gp1[i]);
	}
}
void LoadCalibrationData(void)
{
	unsigned char i;
	for(i=0;i<3;i++)
	{
		CalibrationData.Gm1[i] = eeprom_read_word((uint16_t*)(0+i*6));
		CalibrationData.G0[i] = eeprom_read_word((uint16_t*)(2+i*6));
		CalibrationData.Gp1[i] = eeprom_read_word((uint16_t*)(4+i*6));
	}
}
void PrintCalibrationData(FILE *file)
{
	unsigned char i;
	fprintf_P(file,PSTR("Calibration data: Axis/ -1g, 0g, 1g\r\n"));
	for(i=0;i<3;i++)
		fprintf_P(file,PSTR("%c/ %04d %04d %04d\r\n"),'X'+i,CalibrationData.Gm1[i],CalibrationData.G0[i],CalibrationData.Gp1[i]);	
}

void Calibrate(FILE *file)
{
	
	//unsigned char i;
	
	fprintf_P(file,PSTR("Calibration mode\r\n"));
	
	/*

	
	// Upside down: Z=-1, X,Y=0
	fprintf_P(file,PSTR("Place sensor with BT module up and press button...\r\n"));
	_delay_ms(500);
	while((PINB & 0x04) != 0) PrintRealTime(file);
		
	CalibrationData.Gm1[2] = ReadADCAvgLong(PORT_ACCZ);
	fputs_P(msg_capture,file);
	
	while((PINB & 0x04) == 0);

	// Horizontal: Z=1, X,Y=0.
	fprintf_P(file,PSTR("Place sensor with BT module down and press button...\r\n"));
	_delay_ms(500);
	while((PINB & 0x04) != 0) PrintRealTime(file);
	
	CalibrationData.G0[0] = ReadADCAvgLong(PORT_ACCX);
	CalibrationData.G0[1] = ReadADCAvgLong(PORT_ACCY);
	CalibrationData.Gp1[2] = ReadADCAvgLong(PORT_ACCZ);
	fputs_P(msg_capture,file);
	
	while((PINB & 0x04) == 0);
	

	// Buttons down: X=-1, Y=0, Z=0
	fprintf_P(file,PSTR("Place sensor with PWR-Switch down and press button...\r\n"));
	_delay_ms(500);
	while((PINB & 0x04) != 0) PrintRealTime(file);
	
	CalibrationData.Gm1[0] = ReadADCAvgLong(PORT_ACCX);
	CalibrationData.G0[2] = ReadADCAvgLong(PORT_ACCZ);
	fputs_P(msg_capture,file);
	
	while((PINB & 0x04) == 0);
	
	// Buttons up: X=+1, Y=0, Z=0
	fprintf_P(file,PSTR("Place sensor with PWR-Switch up and press button...\r\n"));
	_delay_ms(500);
	while((PINB & 0x04) != 0) PrintRealTime(file);
	
		CalibrationData.Gp1[0] = ReadADCAvgLong(PORT_ACCX);
	fputs_P(msg_capture,file);
	
	while((PINB & 0x04) == 0);

	// USB up: X=0, Y=-1, Z=0
	fprintf_P(file,PSTR("Place sensor with connector up and press button...\r\n"));
	_delay_ms(500);
	while((PINB & 0x04) != 0) PrintRealTime(file);
	
	CalibrationData.Gm1[1] = ReadADCAvgLong(PORT_ACCY);
	fputs_P(msg_capture,file);
	
	while((PINB & 0x04) == 0);
	
	// USB down: X=0, Y=+1, Z=0
	fprintf_P(file,PSTR("Place sensor with connector down and press button...\r\n"));
	_delay_ms(500);
	while((PINB & 0x04) != 0) PrintRealTime(file);
	
	CalibrationData.Gp1[1] = ReadADCAvgLong(PORT_ACCY);
	fputs_P(msg_capture,file);
	
	while((PINB & 0x04) == 0);
	
	PrintCalibrationData(file);
	
	fprintf_P(file,PSTR("Storing in calibration data\r\n"));

	StoreCalibrationData();
	
	// Test reread
	for(i=0;i<3;i++)
	{
		CalibrationData.Gm1[i]=0;
		CalibrationData.G0[i]=0;
		CalibrationData.Gp1[i]=0;
	}
	
	PrintCalibrationData(file);
	
	fprintf_P(file,PSTR("Load calibration data\r\n"));
	
	LoadCalibrationData();
	PrintCalibrationData(file);*/
}

/*
	Converts the LSB readings into mg using calibration data of sensor s.
*/
signed short LSB2mg(unsigned short lsb,unsigned char s)
{
	signed long int t;
	//unsigned short sens;
	signed short sens;
	signed short z;
	
	//printf("sensor %d. lsb %d. %d %d %d\r\n",s,lsb,CalibrationData.Gm1[s],CalibrationData.G0[s],CalibrationData.Gp1[s]);
	
	//sens = (CalibrationData.Gp1[s]-CalibrationData.Gm1[s])>>1;
	sens = ((signed short)CalibrationData.Gp1[s]-(signed short)CalibrationData.Gm1[s])/2;
	z = ((signed short)CalibrationData.Gp1[s]+(signed short)CalibrationData.Gm1[s])>>1;
	
	//printf("sensitivity: %u. zero: %u\r\n",sens,z);
	
	
	//t = lsb - CalibrationData.G0[s];
	t = lsb - (signed long int)z;
	//printf("%ld\r\n",t);
	t *= (signed long int)1000;
	//printf("%ld\r\n",t);
	t/=(signed long int)sens;
	//printf("%ld\r\n",t);
	return (signed short)t;
	//return 0;
	
	
}


