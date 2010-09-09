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

/******************************************************************************
	Monitoring of sampling
******************************************************************************/
void sample_monitor(void)
{
	uart_setblocking(file_usb,0);
	while(1)
	{
		// Benchmark the number of instruction per seconds...
		unsigned long t1,ips=0;
		t1 = timer_ms_get();
		while(timer_ms_get()-t1<1000)
			ips++;
		printf_P(PSTR("Instructions per second: %05ld\r"),ips);

		printf_P(PSTR("ADC:"));
		for(unsigned char i=0;i<8;i++)
			printf_P(PSTR("%06ld "),samplenum_adc[i]);
		printf_P(PSTR(" TMP S|F: %05ld %05ld"),samplenum_tmp_success,samplenum_tmp_fail);
		printf_P(PSTR(" HMC_S S|F: %05ld %05ld HMC_R S|F: %05ld %05ld"),samplenum_hmc_success[0],samplenum_hmc_fail[0],samplenum_hmc_success[1],samplenum_hmc_fail[1]);
		printf_P(PSTR("\r"));

		// Audio packets
		printf_P(PSTR("audio: %02lu/%02lu w%d p%d n%d %03d\t"),
			packetholder_audio.numerr,packetholder_audio.total,
			(int)packetholder_audio.writeable,(int)packetholder_audio.wp,(int)packetholder_audio.nw,(int)packetholder_audio.dataptr);
		// Acceleration packets
		printf_P(PSTR("acc: %02lu/%02lu w%d p%d n%d %03d\t"),
			packetholder_acc.numerr,packetholder_acc.total,
			(int)packetholder_acc.writeable,(int)packetholder_acc.wp,(int)packetholder_acc.nw,(int)packetholder_acc.dataptr);
		// Analog aux packets
		printf_P(PSTR("aux: %02lu/%02lu w%d p%d n%d %03d\t"),
			packetholder_analogaux.numerr,packetholder_analogaux.total,
			(int)packetholder_analogaux.writeable,(int)packetholder_analogaux.wp,(int)packetholder_analogaux.nw,(int)packetholder_analogaux.dataptr);
		// tmp packets
		printf_P(PSTR("tmp: %02lu/%02lu w%d p%d n%d %03d\t"),
			packetholder_tmp.numerr,packetholder_tmp.total,
			(int)packetholder_tmp.writeable,(int)packetholder_tmp.wp,(int)packetholder_tmp.nw,(int)packetholder_tmp.dataptr);
		// hmc packets
			printf_P(PSTR("hmc: %02lu/%02lu w%d p%d n%d %03d\t"),
				packetholder_hmc.numerr,packetholder_hmc.total,
				(int)packetholder_hmc.writeable,(int)packetholder_hmc.wp,(int)packetholder_hmc.nw,(int)packetholder_hmc.dataptr);
			
		printf_P(PSTR("\r"));

		_delay_ms(20);
		int c;
		if((c=fgetc(file_usb))!=EOF)
			break;
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);
}
/******************************************************************************
	Monitoring of sampling with simulated data write
******************************************************************************/
void sample_monitor_simwrite(void)
{
	unsigned short simdelay=0;
	signed char rp;

	uart_setblocking(file_usb,0);
	while(1)
	{
		// Simulate writing packets to MMC
	
		// Audio
		rp=packetholder_read_next(&packetholder_audio);
		if(rp!=-1)
		{
			// Simulate taking time
			_delay_ms(simdelay);
			
			printf_P(PSTR("Wrote audio %d. %02lu/%02lu\r"),rp,packetholder_audio.numerr,packetholder_audio.total);

			packetholder_read_done(&packetholder_audio);
		}
		// Acceleration
		rp=packetholder_read_next(&packetholder_acc);
		if(rp!=-1)
		{
			// Simulate taking time
			_delay_ms(simdelay);
			
			printf_P(PSTR("Wrote acc %d. %02lu/%02lu\r"),rp,packetholder_acc.numerr,packetholder_acc.total);

			packetholder_read_done(&packetholder_acc);
		}
		// Analog aux
		rp=packetholder_read_next(&packetholder_analogaux);
		if(rp!=-1)
		{
			// Simulate taking time
			_delay_ms(simdelay);
			
			printf_P(PSTR("Wrote analogaux %d. %02lu/%02lu\r"),rp,packetholder_analogaux.numerr,packetholder_analogaux.total);

			packetholder_read_done(&packetholder_analogaux);
		}
		// tmp
		rp=packetholder_read_next(&packetholder_tmp);
		if(rp!=-1)
		{
			// Simulate taking time
			_delay_ms(simdelay);
			
			printf_P(PSTR("Wrote tmp %d. %02lu/%02lu\r"),rp,packetholder_tmp.numerr,packetholder_tmp.total);

			packetholder_read_done(&packetholder_tmp);
		}
		// hmc
		rp=packetholder_read_next(&packetholder_hmc);
		if(rp!=-1)
		{
			// Simulate taking time
			_delay_ms(simdelay);
			
			printf_P(PSTR("Wrote hmc %d. %02lu/%02lu\r"),rp,packetholder_hmc.numerr,packetholder_hmc.total);

			packetholder_read_done(&packetholder_hmc);
		}


		int c;
		if((c=fgetc(file_usb))!=EOF)
		{
			if(c=='+')
			{
				simdelay+=5;
			}
			if(c=='-')
			{
				if(simdelay>0)
					simdelay-=5;
			}
			printf_P(PSTR("Delay is %d. Change with + and -\r"),simdelay);
			if(c=='q' || c=='x')
				break;
		}
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);
}



/******************************************************************************
	Displays available recorded packets
******************************************************************************/
void sample_monitor_dumpdata(void)
{
	signed char rp;
	unsigned colaudio=16;
	unsigned colacc=4;



	// Audio
	while( (rp=packetholder_read_next(&packetholder_audio)) != -1)
	{
		printf_P(PSTR("Audio packet %d"),rp);
		unsigned short *data = ((unsigned short*)packet_audio[rp])+(PACKET_PREAMBLE>>1);
		for(unsigned i=0;i<PACKET_NUM_AUDIO_SAMPLE;i++)
		{
			if((i&(colaudio-1))==0)
				printf_P(PSTR("\r"));
			printf_P(PSTR("%04X "),*data);
			data++;
		}
		printf_P(PSTR("\r"));
		packetholder_read_done(&packetholder_audio);
	}
	
	// Acceleration
	while( (rp=packetholder_read_next(&packetholder_acc)) != -1)
	{
		printf_P(PSTR("Acceleration packet %d"),rp);
		unsigned short *data = ((unsigned short*)packet_acc[rp])+(PACKET_PREAMBLE>>1);
		for(unsigned i=0;i<PACKET_NUM_ACC_SAMPLE;i++)
		{
			if((i&(colacc-1))==0)
				printf_P(PSTR("\r"));
			printf_P(PSTR("%04X %04X %04X   "),data[0],data[1],data[2]);
			data+=3;
		}
		printf_P(PSTR("\r"));
		packetholder_read_done(&packetholder_acc);
	}
	// Analog auxiliary
	while( (rp=packetholder_read_next(&packetholder_analogaux)) != -1)
	{
		printf_P(PSTR("Analog auxiliary packet %d"),rp);
		unsigned short *data = ((unsigned short*)packet_analogaux[rp])+(PACKET_PREAMBLE>>1);
		for(unsigned i=0;i<PACKET_NUM_ANALOGAUX_SAMPLE;i++)
		{
			if((i&(colacc-1))==0)
				printf_P(PSTR("\r"));
			printf_P(PSTR("%04X %04X   "),data[0],data[1]);
			data+=2;
		}
		printf_P(PSTR("\r"));
		packetholder_read_done(&packetholder_analogaux);
	}
	// tmp
	while( (rp=packetholder_read_next(&packetholder_tmp)) != -1)
	{
		printf_P(PSTR("tmp packet %d"),rp);
		unsigned short *data = ((unsigned short*)packet_tmp[rp])+(PACKET_PREAMBLE>>1);
		for(unsigned i=0;i<PACKET_NUM_TMP_SAMPLE;i++)
		{
			if((i&(colaudio-1))==0)
				printf_P(PSTR("\r"));
			printf_P(PSTR("%04X "),*data);
			data++;
		}
		printf_P(PSTR("\r"));
		packetholder_read_done(&packetholder_tmp);
	}
	// hmc
	while( (rp=packetholder_read_next(&packetholder_hmc)) != -1)
	{
		printf_P(PSTR("hmc packet %d"),rp);
		unsigned char *data = packet_hmc[rp]+PACKET_PREAMBLE;
		for(unsigned i=0;i<PACKET_NUM_HMC_SAMPLE;i++)
		{
			if((i&(colacc-1))==0)
				printf_P(PSTR("\r"));
			unsigned short d[3];
			d[0] = (((unsigned short)data[0])<<8)|data[1];
			d[1] = (((unsigned short)data[2])<<8)|data[3];
			d[2] = (((unsigned short)data[4])<<8)|data[5];
			printf_P(PSTR("%04X %04X %04X   "),d[0],d[1],d[2]);
			data+=6;
		}
		printf_P(PSTR("\r"));
		packetholder_read_done(&packetholder_hmc);
	}


}



/******************************************************************************
	Logs the acquired packets
*******************************************************************************

	Logs the acquired packets to the filesystem

******************************************************************************/
unsigned char sample_logpacket(FS_STATE *fs_state,unsigned long deviceid,unsigned long time_absolute_offset)
{
	unsigned short simdelay=0;
	signed char rp;
	unsigned char rv;
	unsigned long int t1,t2;
	
	


	rv=fs_log_start(fs_state,deviceid,time_absolute_offset);
	if(rv!=0)
	{
		printf_P(PSTR("log start failed: %d\r"),rv);
		return;
	}
	
	t1 = t2 = timer_ms_get();

	uart_setblocking(file_usb,0);
	while(1)
	{
		// Simulate writing packets to MMC
	
		// Audio
		rp=packetholder_read_next(&packetholder_audio);
		if(rp!=-1)
		{
			// Write data
			rv = fs_log_write(fs_state,packet_audio[rp],PACKET_SIZE_AUDIO);
			
			//printf_P(PSTR("Wrote audio %d. %02lu/%02lu\r"),rp,packetholder_audio.numerr,packetholder_audio.total);

			packetholder_read_done(&packetholder_audio);
		}
		// Acceleration
		rp=packetholder_read_next(&packetholder_acc);
		if(rp!=-1)
		{
			// Write data
			rv = fs_log_write(fs_state,packet_acc[rp],PACKET_SIZE_ACC);
			
			//printf_P(PSTR("Wrote acc %d. %02lu/%02lu\r"),rp,packetholder_acc.numerr,packetholder_acc.total);

			packetholder_read_done(&packetholder_acc);
		}
		// Analog aux
		rp=packetholder_read_next(&packetholder_analogaux);
		if(rp!=-1)
		{
			// Write data
			rv = fs_log_write(fs_state,packet_analogaux[rp],PACKET_SIZE_ANALOGAUX);
			
			//printf_P(PSTR("Wrote analogaux %d. %02lu/%02lu\r"),rp,packetholder_analogaux.numerr,packetholder_analogaux.total);

			packetholder_read_done(&packetholder_analogaux);
		}
		// tmp
		rp=packetholder_read_next(&packetholder_tmp);
		if(rp!=-1)
		{
			// Write data
			rv = fs_log_write(fs_state,packet_tmp[rp],PACKET_SIZE_TMP);
			
			//printf_P(PSTR("Wrote tmp %d. %02lu/%02lu\r"),rp,packetholder_tmp.numerr,packetholder_tmp.total);

			packetholder_read_done(&packetholder_tmp);
		}
		// hmc
		rp=packetholder_read_next(&packetholder_hmc);
		if(rp!=-1)
		{
			// Write data
			rv = fs_log_write(fs_state,packet_hmc[rp],PACKET_SIZE_HMC);
			
			//printf_P(PSTR("Wrote hmc %d. %02lu/%02lu\r"),rp,packetholder_hmc.numerr,packetholder_hmc.total);

			packetholder_read_done(&packetholder_hmc);
		}


		// Display more detailed infos at regular intervals
		if(timer_ms_get()>t2)
		{
		
		printf_P(PSTR("ADC:"));
		for(unsigned char i=0;i<8;i++)
			printf_P(PSTR("%06ld "),samplenum_adc[i]);
		printf_P(PSTR(" TMP S|F: %05ld %05ld"),samplenum_tmp_success,samplenum_tmp_fail);
		printf_P(PSTR(" HMC_S S|F: %05ld %05ld HMC_R S|F: %05ld %05ld"),samplenum_hmc_success[0],samplenum_hmc_fail[0],samplenum_hmc_success[1],samplenum_hmc_fail[1]);
		printf_P(PSTR("\r"));

		// Log calls
		//printf_P(PSTR("LOG S|F: %04lu|%054lu\t"),fs_state->_log_writeok,fs_state->_log_writeerror);
		// Audio packets
		printf_P(PSTR("audio: %02lu/%02lu w%d p%d n%d %03d\t"),
			packetholder_audio.numerr,packetholder_audio.total,
			(int)packetholder_audio.writeable,(int)packetholder_audio.wp,(int)packetholder_audio.nw,(int)packetholder_audio.dataptr);
		// Acceleration packets
		printf_P(PSTR("acc: %02lu/%02lu w%d p%d n%d %03d\t"),
			packetholder_acc.numerr,packetholder_acc.total,
			(int)packetholder_acc.writeable,(int)packetholder_acc.wp,(int)packetholder_acc.nw,(int)packetholder_acc.dataptr);
		// Analog aux packets
		printf_P(PSTR("aux: %02lu/%02lu w%d p%d n%d %03d\t"),
			packetholder_analogaux.numerr,packetholder_analogaux.total,
			(int)packetholder_analogaux.writeable,(int)packetholder_analogaux.wp,(int)packetholder_analogaux.nw,(int)packetholder_analogaux.dataptr);
		// tmp packets
		printf_P(PSTR("tmp: %02lu/%02lu w%d p%d n%d %03d\t"),
			packetholder_tmp.numerr,packetholder_tmp.total,
			(int)packetholder_tmp.writeable,(int)packetholder_tmp.wp,(int)packetholder_tmp.nw,(int)packetholder_tmp.dataptr);
		// hmc packets
		printf_P(PSTR("hmc: %02lu/%02lu w%d p%d n%d %03d\t"),
			packetholder_hmc.numerr,packetholder_hmc.total,
			(int)packetholder_hmc.writeable,(int)packetholder_hmc.wp,(int)packetholder_hmc.nw,(int)packetholder_hmc.dataptr);
		printf_P(PSTR("\r"));

		t2+=250;

		}

		int c;
		if((c=fgetc(file_usb))!=EOF)
			break;
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);


	rv=rv=fs_log_stop(fs_state);
	if(rv!=0)
	{
		printf_P(PSTR("log stop failed: %d\r"),rv);
		return;
	}
}








/*
void sample_testi2cbuserrors
{
	// Initialize the I2C transactions
	// TMP102 Read transaction
	i2ctrans_tmp.address=0x48;
	i2ctrans_tmp.rw=I2C_READ;
	i2ctrans_tmp.dostart=1;
	i2ctrans_tmp.doaddress=1;
	i2ctrans_tmp.dodata=2;
	i2ctrans_tmp.dostop=1;
	i2ctrans_tmp.dataack[0]=1;
	i2ctrans_tmp.dataack[1]=1;
	i2ctrans_tmp.callback=sample_i2c_callback_tmp;
	
	//HMC Register select transaction 
	i2ctrans_hmc_select.address=0x1E;
	i2ctrans_hmc_select.rw=I2C_WRITE;
	i2ctrans_hmc_select.dostart=1;
	i2ctrans_hmc_select.doaddress=1;
	i2ctrans_hmc_select.dodata=1;
	i2ctrans_hmc_select.dostop=1;
	i2ctrans_hmc_select.data[0]=3;
	i2ctrans_hmc_select.dataack[0]=1;
	i2ctrans_hmc_select.callback=sample_i2c_callback_hmc;
	
	//HMC Register select transaction 
	i2ctrans_hmc_read.address=0x1E;
	i2ctrans_hmc_read.rw=I2C_READ;
	i2ctrans_hmc_read.dostart=1;
	i2ctrans_hmc_read.doaddress=1;
	i2ctrans_hmc_read.dodata=7;
	i2ctrans_hmc_read.dostop=1;
	for(unsigned char i=0;i<7;i++)
		i2ctrans_hmc_read.dataack[i]=(i==6)?0:1;			// Last data received must answer with not ack
	i2ctrans_hmc_read.callback=sample_i2c_callback_hmc;


	// Test the bug by changing the last acknowledge bit of the hmc read command: will hold the bus. Checks that the system self-recovers
	for(unsigned char ti=0;ti<3;ti++)
	{
		printf_P(PSTR("\rTest iteration %d\r\r"),ti);
		unsigned long t1;
		if(!i2c_transaction_idle())
		{
			printf_P(PSTR("Prior: transaction still running. Cancelling\r"));
			i2c_transaction_cancel();
			printf_P(PSTR("After cancel, transaction idle: %d\r"),i2c_transaction_idle());
		}
		printf_P(PSTR("Tmp: exec\r"));
		_delay_ms(100);
		i2c_transaction_execute(&i2ctrans_tmp,0);
		_delay_ms(100);
		t1 = timer_ms_get(); while(!i2c_transaction_idle() && timer_ms_get()-t1<1000);
		if(!i2c_transaction_idle())
		{
			printf_P(PSTR("Tmp: transaction still running!\r"));
		}
		else
		{
			printf_P(PSTR("Tmp: done. Status: %X I2C Error: %X\r"),i2ctrans_tmp.status,i2ctrans_tmp.i2cerror);
			printf_P(PSTR("Tmp: data %02X %02X\r"),i2ctrans_tmp.data[0],i2ctrans_tmp.data[1]);
		}

		printf_P(PSTR("Hmc: exec select\r"));
		if(!i2c_transaction_idle())
		{
			printf_P(PSTR("Prior: transaction still running. Cancelling\r"));
			i2c_transaction_cancel();
			printf_P(PSTR("After cancel, transaction idle: %d\r"),i2c_transaction_idle());
		}
		_delay_ms(100);
		i2c_transaction_execute(&i2ctrans_hmc_select,0);
		_delay_ms(100);
		t1 = timer_ms_get(); while(!i2c_transaction_idle() && timer_ms_get()-t1<1000);
		if(!i2c_transaction_idle())
		{
			printf_P(PSTR("Hmc: transaction still running!\r"));
		}
		else
		{
			printf_P(PSTR("Hmc: done. Select. Status: %X I2C Error: %X\r"),i2ctrans_hmc_select.status,i2ctrans_hmc_select.i2cerror);
			printf_P(PSTR("Hmc: data. Select. ")); for(int i=0;i<16;i++) printf_P(PSTR("%02X "),i2ctrans_hmc_select.data[i]); printf_P(PSTR("\r")); 
			printf_P(PSTR("Hmc: done. Read. Status: %X I2C Error: %X\r"),i2ctrans_hmc_read.status,i2ctrans_hmc_read.i2cerror);
			printf_P(PSTR("Hmc: data. Read. ")); for(int i=0;i<16;i++) printf_P(PSTR("%02X "),i2ctrans_hmc_read.data[i]); printf_P(PSTR("\r")); 
		}

	}
	return;
}
*/
