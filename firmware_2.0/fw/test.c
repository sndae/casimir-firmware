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
#include "serial.h"
#include "global.h"
#include "helper.h"
#include "helper2.h"
#include "led.h"
#include "adc.h"
#include "main.h"
#include "test.h"
#include "i2c.h"
#include "wait.h"
#include "ui.h"

void testanalog(void)
{
	unsigned short adc;

	uart_setblocking(file_usb,0);
	while(1)
	{
		for(unsigned char i=0;i<8;i++)
		{
			adc=ReadADC(i);
			printf_P(PSTR("%04u "),adc);
		}
		printf_P(PSTR("\r"));
		_delay_ms(100);
		int c;
		if((c=fgetc(file_usb))!=EOF)
			break;
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);
}







/*
	mode=0: streams 7 MSB bits [10...3]
	mode=1: streams 7 LSB bits [7...0]
*/
void teststreammike(unsigned char channel,unsigned char mode)
{
	int c;

	uart_setblocking(file_usb,0);	
	while(1)
	{
		uart0_fputchar_int('é',0);
		unsigned short v = ReadADCNR(channel);
		//unsigned short v = ReadADCNR(6);
		if(mode==0)
		{
			v>>=3;		// 7 MSB bits
		}
		else
		{
			v=v-448;		// Remove offset
			if(v>127)
				v=127;	// 7 LSB bits
		}
		uart0_fputchar_int(v,0);
		_delay_us(400);
		//_delay_us(500);
		
		if((c=fgetc(file_usb))!=EOF)		
			break;
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);
}

/*
	gain=0: 10x
	gain=1: 200x
*/
void teststreammikeGain(unsigned char gain,unsigned char mode)
{
	int c;
	
	if(gain==0)
		ADMUX = 0x40 | 0x09;
	else
		ADMUX = 0x40 | 0x0B;

	uart_setblocking(file_usb,0);	
	while(1)
	{
		uart0_fputchar_int('é',0);

		
		ADCSRA = 0x8C;		// Start conversion, prescaler 16
		set_sleep_mode(SLEEP_MODE_IDLE);
		do
		{
			sleep_mode();
		}
		while(ADCSRA&0x40);

		

		signed short v = ADCW;
		if(v&0x200)
			v|=0xFC00;

		v+=512;

		//printf_P(PSTR("v: %d\r"),v);
		//_delay_ms(10);
		
		
		
		
		
		//unsigned short v = ReadADCNR(6);
		if(mode==0)
		{
			v>>=3;		// 7 MSB bits
		}
		else
		{
			v=v-448;		// Remove offset
			if(v>127)
				v=127;	// 7 LSB bits
		}
		uart0_fputchar_int(v,0);
		_delay_us(400);
		//_delay_us(500);
		
		if((c=fgetc(file_usb))!=EOF)		
			break;
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);
}





void testscani2c(void)
{
	unsigned char rv;

	printf_P(PSTR("\rScanning I2C bus in write mode\r"));
	for(unsigned char address=0;address<128;address++)
	{

		printf_P(PSTR("Address %02X... "),address);

		// Start a write 
		rv = i2c_writestart(address,0);
		
		if(rv==0)
		{
			// Success
			printf_P(PSTR("ok        <----\r"));
			
		}
		else
		{
			printf_P(PSTR("error (%X)\r"),rv);
		}
		i2c_stop(1);
	
	}		
/*
	printf_P(PSTR("\rScanning I2C bus in read mode\r"));
	for(unsigned char address=28;address<34;address++)
	{

		printf_P(PSTR("Address %02X... "),address);

		// Start a write 
		rv = i2c_readstart(address,1);
		
		if(rv==0)
		{
			// Success
			printf_P(PSTR("ok        <----\r"));
			unsigned char data;
			for(unsigned i=0;i<5;i++)
			{
				rv = i2c_readdata(&data,(i==4)?0:1,1);
				printf_P(PSTR("Read data: rv: %X, data: %X\r"),rv,data);			
			}
			
		}
		else
		{
			printf_P(PSTR("error (%X)\r"),rv);
		}
		i2c_stop(1);
	}*/
	


}

void testtmp102_direct(void)
{
	unsigned char rv,data[10];

	printf_P(PSTR("Testing TMP102 - direct access\r"));

	rv = i2c_writestart(0x48,0);
	i2c_writedata(1,0);	// configuration register
	i2c_stop(1);

	rv = i2c_readstart(0x48,0);
	rv = i2c_readdata(&data[0],1,0);
	rv = i2c_readdata(&data[1],1,0);
	i2c_stop(1);
	printf_P(PSTR("TMP102 configuration register: %X %X\r"),data[0],data[1]);


	rv = i2c_writestart(0x48,0);
	i2c_writedata(0,0);	// temperature register
	i2c_stop(1);

	uart_setblocking(file_usb,0);
	while(1)
	{
		rv = i2c_readstart(0x48,0);
		rv = i2c_readdata(&data[0],1,0);	
		rv = i2c_readdata(&data[1],1,0);	
		i2c_stop(1);
		unsigned short datas = (((unsigned short)data[0])<<4) + (data[1]>>4);
		printf_P(PSTR("Temp: %03X. %d.%04d°C\r"),datas,datas/16,(datas%16)*625);
		_delay_ms(200);

		int c;
		if((c=fgetc(file_usb))!=EOF)
			break;
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);
}
void testtmp102_int(void)
{
	//unsigned char rv,data[10];
	unsigned long idle;
	I2C_TRANSACTION tran;

	printf_P(PSTR("Testing TMP102 - interrupt driven\r"));

	
	// Select configuration register
	printf_P(PSTR("Selecting configuration register\r"));

	tran.address=0x48;
	tran.rw=I2C_WRITE;
	tran.dostart=1;
	tran.doaddress=1;
	tran.dodata=1;
	tran.dostop=1;
	tran.data[0]=1;
	tran.dataack[0]=1;
	tran.callback=0;
	
	idle=i2c_transaction_execute(&tran,1);
	
	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
	
	
	// Read configuration register
	//for(int i=0;i<3;i++)
	//{
		printf_P(PSTR("Reading configuration register\r"));

		tran.rw=I2C_READ;
		tran.dodata=2;
		tran.dataack[0]=1;
		tran.dataack[1]=1;
		idle=i2c_transaction_execute(&tran,1);

		printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
		printf_P(PSTR("Data %X %X\r"),tran.data[0],tran.data[1]);
	//}

	// Select temperature register
	//for(int i=0;i<3;i++)
	//{
		printf_P(PSTR("Select temperature register\r"));

		tran.rw=I2C_WRITE;
		tran.dodata=1;
		tran.dataack[0]=1;
		idle=i2c_transaction_execute(&tran,1);

		printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
	//}


	uart_setblocking(file_usb,0);
	while(1)
	{
		tran.rw=I2C_READ;
		tran.dodata=2;
		tran.dataack[0]=1;
		tran.dataack[1]=1;
		idle=i2c_transaction_execute(&tran,1);

		printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
		printf_P(PSTR("Data %X %X\r"),tran.data[0],tran.data[1]);

		unsigned short datas = (((unsigned short)tran.data[0])<<4) + (tran.data[1]>>4);
		printf_P(PSTR("Temp: %03X. %d.%04d°C\r"),datas,datas/16,(datas%16)*625);
		_delay_ms(200);

		int c;
		if((c=fgetc(file_usb))!=EOF)
			break;
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);
}


void test_benchmarki2c_int(void)
{
	unsigned char rv;
	unsigned char data[2];
	unsigned short i;
	unsigned long t1,t2;

	printf_P(PSTR("Benchmarking direct access read (TMP102)\r"));
	t1 = timer_ms_get();
	for(i=0;i<1000;i++)
	{
		rv = i2c_readstart(0x48,0);
		rv = i2c_readdata(&data[0],1,0);	
		rv = i2c_readdata(&data[1],1,0);	
		i2c_stop(0);
	}
	t2 = timer_ms_get();
	printf_P(PSTR("Time: %ld for %d calls\r"),t2-t1,i);
	printf_P(PSTR("Data %X %X\r"),data[0],data[1]);


	I2C_TRANSACTION tran;
	tran.address=0x48;
	tran.rw=I2C_READ;
	tran.dostart=1;
	tran.doaddress=1;
	tran.dodata=3;
	tran.dostop=1;
	tran.dataack[0]=1;
	tran.dataack[1]=1;
	tran.data[0]=0x55;
	tran.data[1]=0x55;
	tran.callback=0;
	
	printf_P(PSTR("Benchmarking interrupt access read (TMP102)\r"));
	unsigned long idle=0;
	t1 = timer_ms_get();
	for(i=0;i<1000;i++)
	{
		idle+=i2c_transaction_execute(&tran,1);
	}
	t2 = timer_ms_get();
	printf_P(PSTR("Time: %ld for %d calls. Idle : %ld\r"),t2-t1,i,idle);
	printf_P(PSTR("Data %X %X\r"),tran.data[0],tran.data[1]);


}


void testtmp102(void)
{
	
	testtmp102_int();
	test_benchmarki2c_int();
}


void testmagn2_direct(void)
{
	unsigned char rv,data[12];

	i2c_init();
	

	printf_P(PSTR("Testing HMC5843\r"));
	rv = i2c_writestart(0x00,1);
	i2c_stop(1);

	rv = i2c_writestart(0x1E,1);
	if(rv!=0)
		return;
	rv = i2c_writedata(2,1);			// mode
	if(rv!=0)
		return;
	rv = i2c_writedata(0,1);				// continuous
	if(rv!=0)
		return;
	i2c_stop(1);



	while(1)
	{
		_delay_ms(500);

		// Write address pointer
		/*rv = i2c_writestart(0x1E,1);
		if(rv!=0)
			continue;
		rv = i2c_writedata(0x0,1);
		if(rv!=0)
			continue;
		i2c_stop(1);*/


		rv = i2c_readstart(0x1E,1);
		if(rv!=0)
			continue;
		for(int i=0;i<7;i++)
		{
			rv=i2c_readdata(&data[i],(i==6)?0:1,1);
			if(rv!=0)
				continue;
		}
		i2c_stop(1);

		for(int i=0;i<7;i++)
			printf_P(PSTR("%02X "),data[i]);
		signed short x,y,z;
		x = (((unsigned short)data[0])<<8) + ((unsigned short)data[1]);
		y = (((unsigned short)data[2])<<8) + ((unsigned short)data[3]);
		z = (((unsigned short)data[4])<<8) + ((unsigned short)data[5]);
		printf_P(PSTR("%d %d %d"),x,y,z);
		printf_P(PSTR("\r"));
	}
	
	
}



void testmagn2_int(void)
{
	unsigned long idle;
	I2C_TRANSACTION tran;

	
	printf_P(PSTR("Testing HMC5843 - interrupt driven\r"));


	// Selecting configuration register A
	printf_P(PSTR("Selecting configuration register A\r"));

	tran.address=0x1E;
	tran.rw=I2C_WRITE;
	tran.dostart=1;
	tran.doaddress=1;
	tran.dodata=1;
	tran.dostop=1;
	tran.data[0]=0;
	tran.dataack[0]=1;
	tran.callback=0;
	
	idle=i2c_transaction_execute(&tran,1);

	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);

	
	// Read configuration and mode registers
	printf_P(PSTR("Read configuration and mode registers\r"));

	tran.rw=I2C_READ;
	tran.dodata=3;
	for(unsigned char i=0;i<3;i++)
	{
		tran.dataack[i]=(i==2)?0:1;			// Last data received must answer with not ack
		tran.data[i]=0x55;
	}	

	idle=i2c_transaction_execute(&tran,1);

	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
	printf_P(PSTR("Config A: %02X Config B: %02X Mode: %02X\r"),tran.data[0],tran.data[1],tran.data[2]);
	
	// Selecting identification register A
	printf_P(PSTR("Selecting identification register A\r"));

	tran.rw=I2C_WRITE;
	tran.dodata=1;
	tran.data[0]=10;
	idle=i2c_transaction_execute(&tran,1);

	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);

	// Reading identification registers
	printf_P(PSTR("Reading identification registers\r"));

	tran.rw=I2C_READ;
	tran.dodata=3;
	for(unsigned char i=0;i<3;i++)
	{
		tran.dataack[i]=(i==2)?0:1;			// Last data received must answer with not ack
		tran.data[i]=0x55;
	}	

	idle=i2c_transaction_execute(&tran,1);

	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
	printf_P(PSTR("ID A: %02X ID B: %02X ID C: %02X\r"),tran.data[0],tran.data[1],tran.data[2]);

	
	// Selecting continuous conversion
	printf_P(PSTR("Selecting continuous conversion\r"));

	tran.rw=I2C_WRITE;
	tran.dodata=2;
	tran.data[0]=2;		// Mode
	tran.data[1]=0;		// Continous

	idle=i2c_transaction_execute(&tran,1);

	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);


	// Reread the new setting
	printf_P(PSTR("Selecting configuration register A\r"));

	tran.address=0x1E;
	tran.rw=I2C_WRITE;
	tran.dostart=1;
	tran.doaddress=1;
	tran.dodata=1;
	tran.dostop=1;
	tran.data[0]=0;
	tran.dataack[0]=1;
	tran.callback=0;
	
	idle=i2c_transaction_execute(&tran,1);

	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);

	
	// Read configuration and mode registers
	printf_P(PSTR("Read configuration and mode registers\r"));

	tran.rw=I2C_READ;
	tran.dodata=3;
	for(unsigned char i=0;i<3;i++)
	{
		tran.dataack[i]=(i==2)?0:1;			// Last data received must answer with not ack
		tran.data[i]=0x55;
	}	

	idle=i2c_transaction_execute(&tran,1);

	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
	printf_P(PSTR("Config A: %02X Config B: %02X Mode: %02X\r"),tran.data[0],tran.data[1],tran.data[2]);
	

	tran.rw=I2C_WRITE;
	tran.dodata=1;
	tran.data[0]=3;		// First conversion register

	idle=i2c_transaction_execute(&tran,1);

	printf_P(PSTR("Set read pointer. Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);

	printf_P(PSTR("\rBenchmarking read\r"));
	unsigned long t1,t2;
	tran.rw=I2C_READ;
	unsigned n;
	n=7;
	tran.dodata=n;
	for(unsigned char i=0;i<n;i++)
	{
			tran.dataack[i]=(i==(n-1))?0:1;			// Last data received must answer with not ack
	}
	idle=0;
	t1=timer_ms_get();
	for(unsigned short i=0;i<1000;i++)
	{
		idle+=i2c_transaction_execute(&tran,1);
		//printf_P(PSTR("Read data. Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
	}
	t2=timer_ms_get();
	printf_P(PSTR("Time: %ld. Idle: %ld\r\r"),t2-t1,idle);


	uart_setblocking(file_usb,0);
	while(1)
	{
		_delay_ms(500);

		


		tran.rw=I2C_READ;
		tran.dodata=7;
		for(unsigned char i=0;i<7;i++)
		{
			tran.dataack[i]=(i==6)?0:1;			// Last data received must answer with not ack
			tran.data[i]=0x55;
		}	
		idle=i2c_transaction_execute(&tran,1);
		printf_P(PSTR("Read data. Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
		for(unsigned char i=0;i<7;i++)
			printf_P(PSTR("%02X "),tran.data[i]);
		printf_P(PSTR("\r"));


		int c;
		if((c=fgetc(file_usb))!=EOF)
			break;
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);
	
}


void test_benchmarki2c_magn(void)
{
	unsigned short i;
	unsigned long t1,t2;

	printf_P(PSTR("Benchmarking magnetic field\r"));
	


	I2C_TRANSACTION tran;
	tran.address=0x1E;
	tran.rw=I2C_WRITE;
	tran.dostart=1;
	tran.doaddress=1;
	tran.dodata=1;
	tran.dostop=1;
	tran.data[0]=3;
	tran.dataack[0]=1;
	tran.callback=0;
	
	printf_P(PSTR("Benchmarking setting register pointer\r"));
	unsigned long idle=0;
	t1 = timer_ms_get();
	for(i=0;i<1000;i++)
	{
		idle+=i2c_transaction_execute(&tran,1);
	}
	t2 = timer_ms_get();
	printf_P(PSTR("Time: %ld for %d calls. Idle : %ld\r"),t2-t1,i,idle);
	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
	printf_P(PSTR("Data %X %X\r"),tran.data[0],tran.data[1]);

	printf_P(PSTR("Selecting continuous conversion\r"));

	tran.rw=I2C_WRITE;
	tran.dodata=2;
	tran.data[0]=2;		// Mode
	tran.data[1]=0;		// Continous

	idle=i2c_transaction_execute(&tran,1);
	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);

	
	printf_P(PSTR("Benchmarking reading data\r"));
	tran.rw=I2C_READ;
	tran.dodata=7;
	t1 = timer_ms_get();
	for(unsigned char i=0;i<7;i++)
	{
		tran.dataack[i]=(i==6)?0:1;			// Last data received must answer with not ack
		tran.data[i]=0x55;
	}	
	for(i=0;i<1000;i++)
	{
		idle+=i2c_transaction_execute(&tran,1);
	}
	t2 = timer_ms_get();
	printf_P(PSTR("Time: %ld for %d calls. Idle : %ld\r"),t2-t1,i,idle);
	printf_P(PSTR("Status: %X I2C Error: %X Idle: %ld\r"),tran.status,tran.i2cerror,idle);
	for(unsigned char i=0;i<7;i++)
			printf_P(PSTR("%02X "),tran.data[i]);
		printf_P(PSTR("\r"));

		
		
		


}


void testmagn2(void)
{
	testmagn2_int();
	//testmagn2_direct();
	//test_benchmarki2c_magn();
}



/******************************************************************************
	test_time
*******************************************************************************
	Prints the time offset, relative time tick, and absolute time
******************************************************************************/
void test_time(void)
{
	printf_P(PSTR("\rTest time\r"));
	printf_P(PSTR("Time offset [s]   Relative tick [au]   Current time [s]\r"));
	uart_setblocking(file_usb,0);
	while(1)
	{	
		unsigned long cts;
		cts = time_absolute_offset + time_absolute_tick/8000;
		printf_P(PSTR("%010ld        %010ld           %010ld\r"),time_absolute_offset,time_absolute_tick,cts);

		_delay_ms(100);

		int c;
		if((c=fgetc(file_usb))!=EOF)
			break;
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);
}


/******************************************************************************
	test_dummylog
*******************************************************************************
	Dummy logging
******************************************************************************/

void test_dummylog(FS_STATE *fs_state,unsigned long deviceid,unsigned long timeoffset)
{
	unsigned char rv;
	unsigned short sz[4]={128,256,64,64};
	unsigned char szi;
	unsigned char ctr;
	unsigned long totalw;
	unsigned char ln;

	printf_P(PSTR("Testing dummy logging\r"));

	rv=fs_log_start(fs_state,deviceid,timeoffset,&ln);

	if(rv!=0)
	{
		printf_P(PSTR("log start failed: %d\r"),rv);
		return;
	}


	szi=0;
	ctr=0;
	totalw=0;

	uart_setblocking(file_usb,0);
	while(1)
	{	
		for(unsigned short i=0;i<sz[szi];i++)
			sharedbuffer[i] = sz[szi]+ctr;
		sharedbuffer[0] = ctr;

		rv = fs_log_write(fs_state,sharedbuffer,sz[szi]);
		printf_P(PSTR("log_write: %d\r"),rv);
		
		totalw+=sz[szi];

		szi++;
		if(szi>=4)
			szi=0;

		ctr++;

		//_delay_ms(100);
		printf_P(PSTR("Attempted to write : %ld bytes. Log ok/errors: %ld/%ld\r"),totalw,fs_state->_log_writeok,fs_state->_log_writeerror);

		int c;
		if((c=fgetc(file_usb))!=EOF)
			break;
	}
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);


	rv=fs_log_stop(fs_state);
	printf_P(PSTR("Log closed: %d\r"),rv);
	
	
	printf_P(PSTR("Done aquiring\r"));
}

void test_benchmarklog(FS_STATE *fs_state,unsigned long deviceid,unsigned long timeoffset)
{
	unsigned long t1,t2;
	unsigned long tt1,tt2,tmax;
	unsigned long tb1,tb2;
	unsigned short rv;
	unsigned long i;
	unsigned long bins[33];		// bins of 8, the last bin is 'others'
	unsigned char ln;
	
	unsigned long payloadsize[4]={64,128,256,512};
	unsigned long testsize = 1024L*1024*2;


	printf_P(PSTR("Benchmarking log functions\r"));


// Set something in the buffer
	for(i=0;i<512;i++)
		sharedbuffer[i]=i;


	for(int bi=0;bi<4;bi++)			// Iterate the regions of the card to test
	{

		printf_P(PSTR("Benchmarking log with payload size %ld\r"),payloadsize[bi]);

		
		rv=fs_log_start(fs_state,deviceid,timeoffset,&ln);
		if(rv!=0)
		{
			printf_P(PSTR("log start failed: %d\r"),rv);
			continue;
		}

		// Log opened - now write lots of data

		// Number of log calls 
		unsigned long nlc = testsize/payloadsize[bi];

		printf_P(PSTR("Writing %ld bytes in %ld log calls\r"),testsize,nlc);

		t1 = tb1 = timer_ms_get();
		tmax=0;
		for(i=0;i<nlc;i++)
		{
			sharedbuffer[0]=i;
			
			tt1=timer_ms_get();
			rv = fs_log_write(fs_state,sharedbuffer,payloadsize[bi]);
			tt2=timer_ms_get();

			if(tt2-tt1>tmax)
				tmax=tt2-tt1;
			if(((tt2-tt1)>>3) < 32)
			{
				bins[(tt2-tt1)>>3]++;
			}
			else
				bins[32]++;

			if(rv!=0)
			{
				printf_P(PSTR("Error in log call %ld\r"),i);
				break;
			}

			if( (i&1023)==1023)
			{
				tb2 = timer_ms_get();
				printf_P(PSTR("Call %lu Total time: %lu Average time per call: %lu Write speed: %lu KB/s\r"),i,tb2-tb1,(tb2-tb1)/1024,payloadsize[bi]*1000/(tb2-tb1));
				tb1 = timer_ms_get();
			}
		}
		rv=fs_log_stop(fs_state);
		printf_P(PSTR("Log closed: %d\r"),rv);
		t2 = timer_ms_get();

		printf_P(PSTR("Total time: %lu Average time per call: %lu Maximum time per call: %lu\r"),t2-t1,(t2-t1)/nlc,tmax);
		printf_P(PSTR("Average write speed: %lu KB/s\r"),nlc*payloadsize[bi]*125L/128L/(t2-t1));
		printf_P(PSTR("Histogram of times:\r"));
		for(i=0;i<32;i++)
		{
			printf_P(PSTR("[%lu-%lu) %lu   "),i<<3,(i<<3)+8,bins[i]);
		}
		printf_P(PSTR("[%lu-inf) %lu\r"),i<<3,bins[i]);
	}
}

// Test the led manager...
void test_leds(void)
{
	unsigned int delay=4000;

	printf_P(PSTR("Testing LED manager\r"));
/*
	for(unsigned char o=0;o<4;o++)
	{
		if((o&1)==0)
		{
			printf_P(PSTR("Unoverride\r"));
			_led_unoverride(0);
			_led_unoverride(1);
		}
		if((o&1)==1)
		{
			printf_P(PSTR("Override\r"));
			_led_override(0);
			_led_override(1);
			if(o&2)
			{
				_led_setmodestatic(0);
				_led_set(0,1);	
				_led_setmodeblink(1,4,1,1,0,1);
			}
			else
			{
				_led_setmodestatic(1);
				_led_set(1,1);	
				_led_setmodeblink(0,4,1,1,0,1);
			}
		}


		printf_P(PSTR("Static\r"));
		led_setmodestatic(0);
		led_setmodestatic(1);
	
		for(unsigned i=0;i<2;i++)
		{
			for(unsigned j=0;j<2;j++)
			{
				led_set(0,i);
				led_set(1,j);
				led_enact();
				_delay_ms(1000);
			}
		}

		printf_P(PSTR("Blink\r"));

	
		for(unsigned i=0;i<2;i++)
		{
			for(unsigned j=0;j<2;j++)
			{
				printf_P(PSTR("%d %d\r"),i,j);
				led_setmodeblink(0,i+1,j+1,3,10,1);
				led_setmodeblink(1,i+1,j+1,2,10+i+1+j+1,1);
				led_enact();
				_delay_ms(4000);
			}
		}
	}
*/
	printf_P(PSTR("Unoverride - should restore last prior state which is blink\r"));
	_led_unoverride(0);
	_led_unoverride(1);
	_delay_ms(delay);
	
	printf_P(PSTR("Overriding with blink\r"));
	_led_override(0);
	_led_override(1);
	_led_setmodeblink(0,4,1,1,0,1);
	_led_setmodeblink(1,4,1,1,0,1);
	_delay_ms(delay);
	printf_P(PSTR("Setting 1,0\r"));
	led_setmodestatic(0);
	led_setmodestatic(1);
	led_set(0,1);
	led_set(1,0);
	led_enact();
	_delay_ms(delay);
	printf_P(PSTR("Unoverriding\r"));
	_led_unoverride(0);
	_led_unoverride(1);
	_delay_ms(delay);

	printf_P(PSTR("Overriding with blink\r"));
	_led_override(0);
	_led_override(1);
	_led_setmodeblink(0,4,1,1,0,1);
	_led_setmodeblink(1,4,1,1,0,1);
	_delay_ms(delay);
	printf_P(PSTR("Setting 0,1\r"));
	led_setmodestatic(0);
	led_setmodestatic(1);
	led_set(0,0);
	led_set(1,1);
	led_enact();
	_delay_ms(delay);
	printf_P(PSTR("Unoverriding\r"));
	_led_unoverride(0);
	_led_unoverride(1);
	_delay_ms(delay);



	printf_P(PSTR("Overriding with blink\r"));
	_led_override(0);
	_led_override(1);
	_led_setmodeblink(0,4,1,1,0,1);
	_led_setmodeblink(1,4,1,1,0,1);
	_delay_ms(delay);
	printf_P(PSTR("Setting some blink \r"));
	led_setmodestatic(0);
	led_setmodestatic(1);
	led_setmodeblink(0,1,1,3,10,1);
	led_setmodeblink(1,1,1,2,10+2,1);
	led_enact();
	_delay_ms(delay);
	printf_P(PSTR("Unoverriding\r"));
	_led_unoverride(0);
	_led_unoverride(1);
	_delay_ms(delay);


	printf_P(PSTR("Overriding with blink\r"));
	_led_override(0);
	_led_override(1);
	_led_setmodeblink(0,4,1,1,0,1);
	_led_setmodeblink(1,4,1,1,0,1);
	_delay_ms(delay);
	printf_P(PSTR("Setting some blink \r"));
	led_setmodestatic(0);
	led_setmodestatic(1);
	led_setmodeblink(0,1,1,2,10+2,1);
	led_setmodeblink(1,1,1,3,10,1);
	led_enact();
	_delay_ms(delay);
	printf_P(PSTR("Unoverriding\r"));
	_led_unoverride(0);
	_led_unoverride(1);
	_delay_ms(delay);

}




void _delay_s(unsigned int s)
{
	for(unsigned i=0;i<s;i++)
		_delay_ms(1000);
}

void test_ui_callback(unsigned char action, unsigned char option)
{
	uart0_fputchar_int('A'+action,0);
	uart0_fputchar_int('0'+option,0);
	uart0_fputchar_int('\r',0);
}
void test_ui(void)
{
	printf_P(PSTR("Test the UI\r"));
	
	printf_P(PSTR("Setting some blink and starting the UI\r"));
	led_setmodeblink(0,1,1,2,10+2,1);
	led_setmodeblink(1,1,1,3,10,1);
	led_enact();
	ui_start(0,0,3,test_ui_callback);
	_delay_s(60);

	printf_P(PSTR("Stopping the UI\r"));
	ui_stop();
	_delay_ms(4000);

	printf_P(PSTR("Setting some blink and starting the UI\r"));
	led_setmodeblink(0,1,1,3,10,1);
	led_setmodeblink(1,1,1,2,10+2,1);
	led_enact();
	ui_start(0,0,3,test_ui_callback);
	_delay_s(60);

	printf_P(PSTR("Stopping the UI\r"));
	ui_stop();
	_delay_ms(4000);

	printf_P(PSTR("Setting static / blink and starting the UI\r"));
	led_setmodestatic(0);
	led_on(0);
	led_setmodeblink(1,1,1,3,10,1);
	led_enact();
	ui_start(0,0,3,test_ui_callback);
	_delay_s(60);



	printf_P(PSTR("Stopping the UI\r"));
	ui_stop();
	_delay_ms(4000);
}
