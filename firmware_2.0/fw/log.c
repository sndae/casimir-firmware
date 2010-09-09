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
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "wait.h"
#include "serial.h"
#include "adc.h"
#include "init.h"
#include "core.h"
#include "helper.h"
#include "menu.h"
//#include "pkt.h"
#include "mmc.h"
#include "log.h"
#include "fs.h"
#include "pkt.h"
#include "global.h"
#include "calibration.h"
#include "test.h"
#include "radio.h"
#include "i2c.h"
#include "sample.h"
#include "helper2.h"
#include "led.h"


unsigned short _log_numerr;



/******************************************************************************
	Logs the acquired packets
*******************************************************************************

	Logging the acquired packets to the filesystem:
		log_start:			call to initialize the logging (new log in the filesystem)
		log_logpacket:		call at regular (but high frequency) intervals to store the packets to the filesystem
		log_stop:			call to terminate the log


	Return value:
		0:			completed normally (user interruption)
		1:			logging initialization error
		2:			logging error (mmc write error)
		3:			interruption due to no more free space
		other:	error
******************************************************************************/

/******************************************************************************
	Starts a new log in the file system
******************************************************************************/
unsigned char log_start(FS_STATE *fs_state,unsigned long deviceid,unsigned long time_absolute_offset,unsigned char *lognumber)
{
	unsigned char rv;
	unsigned char ln;

	rv=fs_log_start(fs_state,deviceid,time_absolute_offset,&ln);
	if(rv!=0)
	{
		printf_P(PSTR("log start failed: %d\r"),rv);
		return 1;
	}
	*lognumber=ln;
	return 0;
}
/******************************************************************************
	Starts a new log in the file system
******************************************************************************/
unsigned char log_stop(FS_STATE *fs_state)
{
	unsigned char rv;
	rv=fs_log_stop(fs_state);
	if(rv)
	{
		printf_P(PSTR("log stop failed: %d\r"),rv);
	}
	return rv;
}
/******************************************************************************
	Store all currently available packets to the stream
*******************************************************************************
	Return value:
		0:			Success
		Other		Number of calls to fs_log_write that failed.
******************************************************************************/
unsigned short log_logpacket(FS_STATE *fs_state)
{
	unsigned char rv;
	signed char rp;
	unsigned short numerr;

	numerr=0;

	// Audio
	rp=packetholder_read_next(&packetholder_audio);
	if(rp!=-1)
	{
		// Write data
		rv = fs_log_write(fs_state,packet_audio[rp],PACKET_SIZE_AUDIO);

		if(rv)
			numerr++;
		
		//printf_P(PSTR("Wrote audio %d. %02lu/%02lu\r"),rp,packetholder_audio._log_numerr,packetholder_audio.total);

		packetholder_read_done(&packetholder_audio);
	}
	
	// Acceleration
	rp=packetholder_read_next(&packetholder_acc);
	if(rp!=-1)
	{
		// Write data
		rv = fs_log_write(fs_state,packet_acc[rp],PACKET_SIZE_ACC);

		if(rv)
			numerr++;
		
		//printf_P(PSTR("Wrote acc %d. %02lu/%02lu\r"),rp,packetholder_acc._log_numerr,packetholder_acc.total);

		packetholder_read_done(&packetholder_acc);
	}
	// System
	rp=packetholder_read_next(&packetholder_system);
	if(rp!=-1)
	{
		// Write data
		rv = fs_log_write(fs_state,packet_system[rp],PACKET_SIZE_SYSTEM);

		if(rv)
			numerr++;
		
		//printf_P(PSTR("Wrote system %d. %02lu/%02lu\r"),rp,packetholder_system._log_numerr,packetholder_system.total);

		packetholder_read_done(&packetholder_system);
	}
	// Light
	rp=packetholder_read_next(&packetholder_light);
	if(rp!=-1)
	{
		// Write data
		rv = fs_log_write(fs_state,packet_light[rp],PACKET_SIZE_LIGHT);

		if(rv)
			numerr++;
		
		//printf_P(PSTR("Wrote light %d. %02lu/%02lu\r"),rp,packetholder_light._log_numerr,packetholder_light.total);

		packetholder_read_done(&packetholder_light);
	}
	// tmp
	rp=packetholder_read_next(&packetholder_tmp);
	if(rp!=-1)
	{
		// Write data
		rv = fs_log_write(fs_state,packet_tmp[rp],PACKET_SIZE_TMP);

		if(rv)
			numerr++;
		
		//printf_P(PSTR("Wrote tmp %d. %02lu/%02lu\r"),rp,packetholder_tmp._log_numerr,packetholder_tmp.total);

		packetholder_read_done(&packetholder_tmp);
	}
	// hmc
	rp=packetholder_read_next(&packetholder_hmc);
	if(rp!=-1)
	{
		// Write data
		rv = fs_log_write(fs_state,packet_hmc[rp],PACKET_SIZE_HMC);

		if(rv)
			numerr++;
		
		//printf_P(PSTR("Wrote hmc %d. %02lu/%02lu\r"),rp,packetholder_hmc._log_numerr,packetholder_hmc.total);

		packetholder_read_done(&packetholder_hmc);
	}
	
	return numerr;
}

/******************************************************************************
	log_dologging
*******************************************************************************
	Executes a continuous logging until interrupted by a key press.
	
	Return value:
		0:			Success
		Other		Error
******************************************************************************/

unsigned char log_dologging(FS_STATE *fs_state,unsigned long deviceid,unsigned long time_absolute_offset)
{
	unsigned char rv;
	unsigned long int t1,t2;
	unsigned char breakreason;						// Reason while the logging loop is interrupted. 0: user interruption. 1: log errors
	unsigned char ln;


	rv = log_start(fs_state,deviceid,time_absolute_offset,&ln);
	if(rv)
		return rv;
	
	printf_P(PSTR("Logging in entry %d\r"),ln);

	t1 = t2 = timer_ms_get();

	_log_numerr=0;
	uart_setblocking(file_usb,0);
	while(1)
	{
		// Log packets to MMC
		_log_numerr += log_logpacket(fs_state);

		// Display more detailed infos at regular intervals
		if(timer_ms_get()>t2)
		{		
			sample_printstat();
			pkt_printstat();
			printf_P(PSTR("Cumulative log errors: %03d"),_log_numerr);
			t2+=501;
		}

		if(_log_numerr>1000)
		{
			breakreason=1;
			break;
		}

		int c;
		if((c=fgetc(file_usb))!=EOF)
		{
			breakreason=0;
			break;
		}
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);

	log_stop(fs_state);
	

	if(breakreason==1)
	{
		printf_P(PSTR("Interrupted due to too many logging errors\r"));
		return 2;
	}
	if(breakreason==0)
		printf_P(PSTR("User stopped recording\r"));

	return 0;
}





