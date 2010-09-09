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


/*$
Detect of memory overrun: avr-nm -n main.elf. 
_end is first free address in RAM. Must be <0x801600
*/
/*
	Device specific elements here.
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
#include "helper2.h"
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
#include "radio.h"
#include "test_mmc.h"
//#include "test_sample.h"
#include "led.h"
#include "ui.h"
#include "main_int.h"


/******************************************************************************
	Globals
******************************************************************************/
// Device ID
const char device_name[] PROGMEM = "DIGREC";
const char device_version[] PROGMEM = "HW 2 SW 1.0";
unsigned long int deviceid = 0x12345678;


unsigned char sharedbuffer[520];
volatile unsigned short batteryvoltage;

/******************************************************************************
	External commands
*******************************************************************************
	External commands, coming from USB or radio are provided in 
	_xcommand_int (for the command) and _xcommand_int_avail (to indicate whether 
	a command is available).

	The xcommand is received and stored by the communication interrupt routing. 
	Use getxcommand to read the command from the main application.
******************************************************************************/
XCOMMAND _xcommand_int;
unsigned char uart0_rx_callback_hook_data[XCOMMAND_SIZE];
unsigned char _xcommand_int_avail=0;

/******************************************************************************
	Get external command. Returns 0: no cmmand, 1: command.
******************************************************************************/
unsigned char getxcommand(XCOMMAND *cmd)
{
	unsigned char rv;
	
	cli();
	if(_xcommand_int_avail==0)
	{
		rv = 0;
	}
	else
	{
		rv = 1;
		*cmd = _xcommand_int;
		_xcommand_int_avail=0;
	}
	sei();
	return rv;
}
void sendxcommand(XCOMMAND *cmd)
{
	//fwrite(cmd,XCOMMAND_SIZE,1,file_usb);
	uart0_fputchar_int('S',file_usb);
	uart0_fputchar_int(cmd->cmd,file_usb);
	for(unsigned char i=0;i<XCOMMAND_SIZE-4;i++)
		uart0_fputchar_int(cmd->payload[i],file_usb);
	uart0_fputchar_int('X',file_usb);
	uart0_fputchar_int('E',file_usb);
	
}
/******************************************************************************
	UART callback. Bypasses the user interface for programmatic control.
******************************************************************************/
unsigned char  uart0_rx_callback_hook(unsigned char data)
{
	for(unsigned char i=0;i<XCOMMAND_SIZE-1;i++)
		uart0_rx_callback_hook_data[i]=uart0_rx_callback_hook_data[i+1];
	uart0_rx_callback_hook_data[XCOMMAND_SIZE-1]=data;

	// Detect commands
	if(uart0_rx_callback_hook_data[0]=='S' && 
		uart0_rx_callback_hook_data[XCOMMAND_SIZE-2]=='X' &&
		uart0_rx_callback_hook_data[XCOMMAND_SIZE-1]=='E')
	{
		// Some commands are processed immediately (e.g. synchronization). All ommands are placed in the XCOMMAND structure to allow the app to provide a feedback

		// Check commands requiring immediate processing

		// Synchronization command
		if(uart0_rx_callback_hook_data[1]=='S')
		{
			// Can ONLY set the time if there's no recording. Otherwise: drop command.
			if(!sample_isrunning())
			{
				time_absolute_offset = *((unsigned long*)(uart0_rx_callback_hook_data+2));	
				time_absolute_tick=0;
			}
		}
		_xcommand_int.cmd = uart0_rx_callback_hook_data[1];
		for(unsigned char i=0;i<XCOMMAND_SIZE-4;i++)
			_xcommand_int.payload[i] = uart0_rx_callback_hook_data[2+i];
		_xcommand_int_avail=1;
	}
	return 1;				// Indicates that the data should be placed in the receive buffer
}



/******************************************************************************
Test of the user interface, not interrupt driven
******************************************************************************/



void timer_callback_10hz(void)
{
	led_callback_10hz();
	ui_callback_10hz();
}





//unsigned char currenterror = 0;				// Blink code of the current error.
//unsigned char uimode = 0;			
volatile signed char uicommand = -1;		// Last ui command


/******************************************************************************
	UI Callback Handler
*******************************************************************************
	
	The UI callback handler waits for a user command.
	Upon reception of the user command it sets ui_command to the user command.

******************************************************************************/
void ui_callback_handler(unsigned char action, unsigned char option)
{

	if(action==3)
	{
		uicommand=option;
	}

}

/******************************************************************************
	Starts the UI 
******************************************************************************/
void startui(unsigned char firstoption,unsigned char numoption)
{
	uicommand=-1;
	ui_start(0,firstoption,numoption,ui_callback_handler);										// Initializes and starts the UI (0 is unused for now)
}
/******************************************************************************
	Stops the UI 
******************************************************************************/
void stopui(void)
{
	ui_stop();
}
/******************************************************************************
	Returns the current command from the UI, or -1 if no command was given
******************************************************************************/
signed char getcommand(void)
{
	signed char curcommand;

	// Get the command
	cli();
	curcommand = uicommand;
	uicommand = -1;
	sei();

	return curcommand;
}

/******************************************************************************
	Turns off the device....
******************************************************************************/
void off(void)
{
	printf_P(PSTR("Turning off\r\n"));
	_delay_ms(100);								// Time to get the message through
	//PORTB &= ~(1<<1);
	PORTB=0;
}


/******************************************************************************
	Test the sensors
*******************************************************************************
	Return value:
		0			not ok
		1			ok
******************************************************************************/
/******************************************************************************
	Accelerometer
******************************************************************************/
unsigned char sensor_isok_acc(void)
{
	unsigned long int d[3];
	unsigned char e[3];
	unsigned char noerr;

	// Read the acceleration 
	printf_P(PSTR("    Acceleration sensor: "));

	d[0]=d[1]=d[2]=0;
	for(short i=0;i<1024;i++)
	{
		d[0] += ReadADC(2);
		d[1] += ReadADC(3);
		d[2] += ReadADC(4);
	}
	d[0]>>=10;
	d[1]>>=10;
	d[2]>>=10;

	noerr=1;
	// Test the range. Tolerate +/- 3G when powering up.
	for(unsigned char i=0;i<3;i++)
	{
		if(d[i]<204 || d[i]>818)
		{
			e[i]=1;
			noerr=0;
		}
		else
			e[i]=0;
		printf_P(PSTR("%c: %c (%04ld). "),'X'+i,e[i]?'x':'v',d[i]);
	}
	printf_P(PSTR("                "));
	if(noerr)
		printf_P(PSTR("V\r"));
	else
		printf_P(PSTR("X\r"));
		
	if(noerr)
		return 1;
	return 0;
}
/******************************************************************************
	Microphone and vbat
******************************************************************************/
unsigned char sensor_isok_mic_bat(void)
{
	unsigned long int d[3];
	unsigned char e[3];
	unsigned char noerr;

	// Read the acceleration 
	printf_P(PSTR("    Microphone, VHalf, VBatHalf: "));

	d[0]=d[1]=d[2]=0;
	for(short i=0;i<1024;i++)
	{
		d[0] += ReadADC(1);
		d[1] += ReadADC(0);
		d[2] += ReadADC(6);
	}
	d[0]>>=10;
	d[1]>>=10;
	d[2]>>=10;

	noerr=1;
	if(d[0]<31 || d[0]>992)		// Micout: centered on VHalf... range volume and gain dependent... check for stuck at faults (0.1V from rail)...
	{
		e[0]=1;
		noerr=0;
	}
	else
		e[0]=0;

	if(d[1]<480 || d[1]>542)		// Vhalf: +/- 0.1V around VCC/2
	{
		e[1]=1;
		noerr=0;
	}
	else
		e[1]=0;

	if(d[2]<510 || d[2]>666)		// VBatHalf within Vreg/2 (512) and 4.3V (666) 
	{
		e[2]=1;
		noerr=0;
	}
	else
		e[2]=0;

	for(unsigned char i=0;i<3;i++)
		printf_P(PSTR("%c (%04ld). "),e[i]?'x':'v',d[i]);

	printf_P(PSTR("                 "));
	if(noerr)
		printf_P(PSTR("V\r"));
	else
		printf_P(PSTR("X\r"));
	
	if(noerr)
		return 1;
	return 0;	
}
/******************************************************************************
	Light sensor
******************************************************************************/
unsigned char sensor_isok_light(void)
{
	unsigned long int d;
	unsigned char noerr;

	// Read the acceleration 
	printf_P(PSTR("    Light sensor: "));

	d=0;
	for(short i=0;i<1024;i++)
	{
		d += ReadADC(5);
	}
	d>>=10;

	noerr=1;	
	if(d<31 || d>992)		// Range difficult to estimate. Check for stuck at faults (0.1V from rail)...
	{
		noerr=0;
	}

	printf_P(PSTR("%c (%04ld)."),noerr?'v':'x',d);
	for(unsigned char i=0;i<53;i++)
		printf_P(PSTR(" "));
	
	if(noerr)
		printf_P(PSTR("V\r"));
	else
		printf_P(PSTR("X\r"));
		
	if(noerr)
		return 1;
	return 0;
}
/******************************************************************************
	TMP102
******************************************************************************/
unsigned char sensor_isok_tmp(void)
{
	unsigned char noerr;
	I2C_TRANSACTION i2ctrans_tmp;

	// Read the acceleration 
	printf_P(PSTR("    TMP102: "));

		
	// TMP102 Read transaction
	i2ctrans_tmp.address=0x48;
	i2ctrans_tmp.rw=I2C_READ;
	i2ctrans_tmp.dostart=1;
	i2ctrans_tmp.doaddress=1;
	i2ctrans_tmp.dodata=2;
	i2ctrans_tmp.dostop=1;
	i2ctrans_tmp.dataack[0]=1;
	i2ctrans_tmp.dataack[1]=1;
	i2ctrans_tmp.callback=0;

	i2c_transaction_execute(&i2ctrans_tmp,1);		// Blocking transaction

	if(i2ctrans_tmp.status==0)
	{
		printf_P(PSTR("%02X%02X"),i2ctrans_tmp.data[0],i2ctrans_tmp.data[1]);
		noerr=1;
	}
	else
	{
		printf_P(PSTR("----"));
		noerr=0;
	}

	for(unsigned char i=0;i<64;i++)
		printf_P(PSTR(" "));
	if(noerr)
		printf_P(PSTR("V\r"));
	else
		printf_P(PSTR("X\r"));
	
	if(noerr)
		return 1;
	return 0;
	
}
/******************************************************************************
	HMC
******************************************************************************/
unsigned char sensor_isok_hmc(void)
{
	unsigned char noerr;
	I2C_TRANSACTION tran;

	// Read the acceleration 
	printf_P(PSTR("    HMC: "));
		
	//HMC Register select transaction 
	tran.address=0x1E;
	tran.rw=I2C_WRITE;
	tran.dostart=1;
	tran.doaddress=1;
	tran.dodata=1;
	tran.dostop=1;
	tran.data[0]=3;
	tran.dataack[0]=1;
	tran.callback=0;

	noerr=0;
	
	// Selecting continuous conversion
	tran.rw=I2C_WRITE;
	tran.dodata=2;
	tran.data[0]=2;		// Mode
	tran.data[1]=0;		// Continous
	//tran.data[1]=2;		// Idle
	i2c_transaction_execute(&tran,1);



	if(tran.status==0)
	{
		// Select the conversion speed
		tran.data[0]=0;		// CR A
		tran.data[1]=0x18;	// 50Hz
		//tran.data[1]=0x00;	// 0.5Hz
		i2c_transaction_execute(&tran,1);
		if(tran.status==0)
		{

			// Select register
			tran.rw=I2C_WRITE;
			tran.dodata=1;
			tran.data[0]=3;		

			i2c_transaction_execute(&tran,1);
			if(tran.status==0)
			{
				// Read data and status
				tran.rw=I2C_READ;
				tran.dodata=7;
				for(unsigned char i=0;i<7;i++)
					tran.dataack[i]=(i==6)?0:1;			// Last data received must answer with not ack

				i2c_transaction_execute(&tran,1);			// Blocking transaction
				if(tran.status==0)
				{
					for(unsigned char i=0;i<7;i++)
						printf_P(PSTR("%02X "),tran.data[i]);
					noerr=1;
				}
				else
					printf_P(PSTR("data read error      "));
			}
			else
				printf_P(PSTR("register select error"));
		}
		else
			printf_P(PSTR("configure CRA error  "));
	}
	else
		printf_P(PSTR("configure mode error "));


	for(unsigned char i=0;i<50;i++)
		printf_P(PSTR(" "));
	if(noerr)
		printf_P(PSTR("V\r"));
	else
		printf_P(PSTR("X\r"));
	
	if(noerr)
		return 1;
	return 0;
	
}

/******************************************************************************
	List of status messages
*******************************************************************************

	Upon user command
		Error:
			2		Command error (usually repeteated)

		Ok:
			2		Command success (usually repeated)

	Unattended

		Error:
			1		Accelerometer error
			2		Other analog signals error
			3		Light sensor error
			4		HMC error
			5		TMP error
			6		Disk initialization error
			7		Invalid file system
			8		Card full
			10		Logging was interrupted due to: too many write errors
			11		Logging was interrupted due to: capacity full
			12		Logging was interrupted due to: low battery
	

		ok:
			1(upon user command)
					Command ok
			(unattended)
			1		Initialization ok				
			2		Sensor initialization ok
			3		Disk initialization ok (disk present, readable with a filesystem)
			4		Ready


******************************************************************************/



/******************************************************************************
	Command: off
******************************************************************************/
unsigned char docommand_off(void)
{
	off();
	return 0;
}
/******************************************************************************
	Command: format
******************************************************************************/
unsigned char docommand_format(void)
{
	printf_P(PSTR("Formatting file system\r"));
	if(fs_format()==0 && fs_init_logical(&fs_state)==0)
	{
		// Success
		//led_ok(2);
		return 0;
	}
	
	// Failure: indicate error
	led_fatal_error_repn(2,5);		
	return 1;
}


/******************************************************************************
	waitexeccommanderror
*******************************************************************************
	Displays an error and allow one of two commands: off and/or format

	- Displays the error 'error'
	- Starts the UI with either only the off command (mode=0) or the off and format command (mode=1)
	- Waits for the command
	- Executes the command
	- Stops the UI

	error:		Error to display
	mode:			0: allow the off command. 1: allow the off and format commands.
		
	Returns:
		0:		success executing the UI command (i.e. the application may retry to execute a task if it had previously failed)
		1:		failure executing the UI command
******************************************************************************/
unsigned char waitexeccommanderror(unsigned char error,unsigned char mode)
{
	signed char curcommand;
	unsigned char rv;

	// Set the error to display
	led_fatal_error(error);
	
	// Start the ui with the appropriate numbers of options
	startui(1,1+mode);
	
	do
	{
		curcommand = -1;
		curcommand = getcommand();
		rv=0;
		switch(curcommand)
		{
			// Off command
			case 1:
				rv = docommand_off();
				break;

			// Format command
			case 2:
				rv = docommand_format();
				break;
		}
	}
	while(curcommand==-1);

	stopui();
	return rv;
}

/******************************************************************************
	Read device ID from flash
******************************************************************************/
void main_loggingapplication_readdeviceid(void)
{
	deviceid=eeprom_read_dword(0);
	printf_P(PSTR("Device ID: %08lX\r"),deviceid);
}

/******************************************************************************
	Main logging application - disk initialization
*******************************************************************************
	In case of error, indicate the error, and allow: turn off.
******************************************************************************/
unsigned char main_loggingapplication_diskinit(void)
{
	unsigned char rv;

	rv = fs_init_physical(&fs_state);
	if(!rv)
	{
		printf_P(PSTR("Disk initialization ok.\r"));
		return 0;
	}

	printf_P(PSTR("Disk initialization failed.\r"));
	
	return waitexeccommanderror(6,0);		// Error 6. Command: off

}


/******************************************************************************
	Check if the card is full
*******************************************************************************
	Return
		0:			Not full
		other:	Full
******************************************************************************/
unsigned char iscardentryfull(void)
{
	// Check the number of log entries
	if(fs_state.numentry>=FS_MAXENTRY)
		return 1;
	return 0;
}
unsigned char iscardcapafull(void)
{
	// Check the capacity
	if(fs_state.firstfreeblock>=fs_state.capacity_block-2*LOG_UPDATEFS_SECTOR_INTERVAL)		// The root is updated every LOG_UPDATEFS_SECTOR_INTERVAL - 2 is just to make sure
		return 1;

	return 0;
}
unsigned char isbatlow(void)
{
	return 0;
}

/******************************************************************************
	Main logging application - Scan and initializes sensors
*******************************************************************************
	In case of error, indicate the erroneous sensor and continue.
******************************************************************************/
void main_loggingapplication_sensorscaninit(void)
{
	unsigned char anyerr=0;

	printf_P(PSTR("Sensor initialization...\r"));
	// Test the accelerometer
	if(!sensor_isok_acc())
	{
		led_fatal_error_repn(1,3);		// Error
		anyerr=1;
	}

	// Test the aux analog signals (microphone, vhalf, vbathalf)
	if(!sensor_isok_mic_bat())
	{
		led_fatal_error_repn(2,3);		// Error
		anyerr=1;
	}
	// Test the aux light sensor
	//if(!sensor_isok_light())
	if(0)
	{
		led_fatal_error_repn(3,3);		// Error
		anyerr=1;
	}
	// Test the HMC
	if(!sensor_isok_hmc())
	{
		led_fatal_error_repn(4,3);		// Error
		anyerr=1;
	}
	// Test the TMP
	if(!sensor_isok_tmp())
	{
		led_fatal_error_repn(5,3);		// Error
		anyerr=1;
	}	
	// Sensor initialization ok
	if(!anyerr)
		led_ok(2);
}

/******************************************************************************
	adc2mv
******************************************************************************/
unsigned short adc2mv(unsigned short adc)
{
	return (unsigned)(6600L*(unsigned long)batteryvoltage/1023L);	
}

/******************************************************************************
	Main logging application
*******************************************************************************

	This is the master function that initializes and runs the whole 
	logging operating system



******************************************************************************/
extern unsigned _sample_togglet0,_sample_togglet1;

void main_loggingapplication(void)
{
	unsigned char rv;
	signed char curcommand;
	unsigned char ln;
	unsigned long int t1=0;
	unsigned long int rst=0;
	unsigned short numerror=0;
	unsigned char mustreadfs=1;						// Must read file system in main loop?
	unsigned char muststop=0;							// Indicate if logging must be stop. 0=no. 1=user stopped it. 2=error.
	unsigned char muststart=0;							// Indicate if logging must be started. 0=no. 1=yes.
	unsigned char mustformat=0;						// Indicate if format requested
	unsigned short filteredbattery=4000;			// Low-pass filtered battery voltage in mV (initialized to 4000 as this is an ok value.
	unsigned char idlelowbat=0;						// indicates the low-battery idle state
	unsigned char stopped=0;							// Indicates that logging was stopped and that the UI (battery indication) should not be touched
	unsigned char readystart=0;						// Indicates that the card is okay to start logging - i.e. capacity available.
	XCOMMAND xcommand;									// External commands

	// Read the device ID
	main_loggingapplication_readdeviceid();

	// Scan the sensors
	main_loggingapplication_sensorscaninit();
	
	// Initialize the MMC
	rv = main_loggingapplication_diskinit();
	
	// Indicate if the disk initialization suceeded
	if(rv==0)
		led_ok(3);

	t1 = timer_ms_get();

	uart_setblocking(file_usb,0);


	while(1)
	{
		// Read the file system, checks if capacity free, and shows idle signal if needed.
		if(mustreadfs)
		{
			mustreadfs=0;

			rv = fs_init_logical(&fs_state);
			if(!rv)
			{
				printf_P(PSTR("Filesystem initialization ok\r"));
				printf_P(PSTR("Number of entries: %d. Block to use: %lu. Number of available blocks: %lu\r"),fs_state.numentry,fs_state.firstfreeblock,fs_state.capacity_block-fs_state.firstfreeblock);

				if(iscardentryfull() || iscardcapafull())
				{
					// Display the blinking error code
					led_fatal_error(8);

					// Start the UI, allowing off and format commands
					startui(1,2);
				}
				else
				{
					led_ok(4);		// "Ready" signal

					// Set the LEDs
					led_idle();
					idlelowbat=0;

					// Start the UI, allowing all commands
					startui(0,3);
					readystart=1;
				}
			}
			else
			{
				printf_P(PSTR("Filesystem initialization error. Disk should be formatted\r"));
	
				// Display filesystem error
				led_fatal_error(7);

				// Start the UI, allowing off and format commands
				startui(1,2);
			}				
		}


		
		// ----------------------------------------------------------------------
		// Process the commands coming from the user interface
		// ----------------------------------------------------------------------
		// Get the commands
		curcommand = getcommand();

		if(curcommand!=-1)
			printf_P(PSTR("Got command %d. Sampling? %d\r"),curcommand,sample_isrunning());

		// Execute the idle commands
		switch(curcommand)
		{
			case 0:
				if(!sample_isrunning())
					muststart=1;
				else
					muststop=1;		
				break;
			case 1:
				docommand_off();
				break;
			case 2:
				mustformat=1;
				break;
		}	
		// Mark command as done.
		curcommand=-1;


			
		// ----------------------------------------------------------------------
		// Log the data
		// ----------------------------------------------------------------------		
		// Logging is active: do the logging, statistics update, error checking (battery, card full), etc.
		if(sample_isrunning())
		{
			// Log packets to MMC			
			numerror += log_logpacket(&fs_state);

			// Lots of write errors: MMC problem - stop and signal
			if(numerror>100)
				muststop=2;

			// Card is full - stop and signal
			if(iscardcapafull())
				muststop=3;
		}
		// ----------------------------------------------------------------------
		// Display infos at regular intervals
		// ----------------------------------------------------------------------				
		if(timer_ms_get()>t1)
		{		
			if(sample_isrunning())
			{
				// When sampling, dip
				sample_printstat();
				pkt_printstat();
				unsigned b = adc2mv(batteryvoltage);
				//b = 3400;
				filteredbattery = (filteredbattery*7+b)>>3;	
				printf_P(PSTR("Cumulative log errors: %03d. Recording time: %05ld sec. Capacity used (blk): %ld. Bat: %d (%d) mV\r"),numerror,(timer_ms_get()-rst)/1000,fs_state.firstfreeblock,b,filteredbattery);
				printf_P(PSTR("fs_state._log_writeok %ld fs_state._log_writeerror %ld\r"),fs_state._log_writeok,fs_state._log_writeerror);
			}
			else
			{
				// Get the battery voltage
				batteryvoltage = ReadADC(6);
				unsigned b = adc2mv(batteryvoltage);
				//b = 3400;
				filteredbattery = (filteredbattery*7+b)>>3;	

				if(idlelowbat)
					printf_P(PSTR("Bat: %d (%d) mV\r"),b,filteredbattery);
			}
			
			t1+=2000;
		}
		// ----------------------------------------------------------------------
		// Check the battery level
		// ----------------------------------------------------------------------				
		if(filteredbattery < 3450)
		{
			if(sample_isrunning())
			{
				// Interrupt the sampling
				muststop=4;
			}
			else
			{
				// Change the idle blinking to low battery mode.
				// Must call led_idle_lowbat only on the transition high-low (idlelowbat)
				// Must not set idle low bat if the recording is in 'stopped' mode, as an error message is displayed and should not be erased
				if(!idlelowbat && !stopped)
				{
					led_idle_lowbat();
					idlelowbat=1;
				}
			}
		}

		// ----------------------------------------------------------------------
		// Execute the format command
		// ----------------------------------------------------------------------
		if(mustformat)
		{
			readystart=0;
			docommand_format();
			mustreadfs=1;
			mustformat=0;
		}

		// ----------------------------------------------------------------------
		// Execute the start command
		// ----------------------------------------------------------------------
		if(muststart)
		{
			printf_P(PSTR("Starting logging\r"));
			// Card has space if we receive this command. (assumption not true anymore if xcommands are allowed)
			rv = log_start(&fs_state,deviceid,time_absolute_offset,&ln);
			if(rv)
			{
				printf_P(PSTR("Error starting the log\r"));
				led_fatal_error_repn(2,5);
			}
			else
			{
				startui(0,1);		// Only off

				sample_start(deviceid,ln);

				numerror=0;
				rst = t1 = timer_ms_get();
			}
			muststart=0;
		}

		// ----------------------------------------------------------------------
		// Execute the stop command
		// ----------------------------------------------------------------------
		if(muststop)
		{
			printf_P(PSTR("Stopping logging. Reason is %d\r"),muststop);

			readystart=0;		// Is set again after reading the fat


			sample_stop();
			rv = log_stop(&fs_state);
			if(rv)
			{
				printf_P(PSTR("Error stopping the log\r"));
				led_fatal_error_repn(2,5);
			}

			if(muststop!=1)
			{
				// If an error caused to stop logging, show this error, allowing only off.
				stopped=1;

				// Display the blinking error code
				led_fatal_error(10-2+muststop);

				// Start the UI, allowing off
				startui(1,1);
			}
			else
			{
				// If loging was stopped by the user, read the FS and  was stopped 

				mustreadfs = 1;
			}
			muststop=0;
		}

		// ----------------------------------------------------------------------
		// Execute external commands coming from the USB/radio
		// ----------------------------------------------------------------------
		rv = getxcommand(&xcommand);
		if(rv)
		{
			unsigned char fail=0;
			// Print the command (debug)
			printf_P(PSTR("XCOMMAND: %d"),xcommand.cmd);
			for(unsigned char i=0;i<8;i++)
				printf_P(PSTR("%02X "),xcommand.payload[i]);
			printf_P(PSTR("\r"));

			// Build some answer
			XCOMMAND answer;
			answer = xcommand;		// By default, sending this signifies that the command was successful.

			// Interpret the command
			if(xcommand.cmd=='S')
			{
				// Synchronization command: we do nothing  -  i.e. will automatically reply with success
			}
			if(xcommand.cmd=='I')
			{
				// Provide the device ID.
				*(unsigned long*)answer.payload=deviceid;
			}
			if(xcommand.cmd=='J')
			{
				deviceid = *(unsigned long*)xcommand.payload;
				printf_P(PSTR("Programming the device ID: %08lX\r"),deviceid);				
				eeprom_write_dword(0,deviceid);
				main_loggingapplication_readdeviceid();
			}
			if(xcommand.cmd=='T')
			{
				// Read out the device time

				// The answer contains:
				// time_offset    4
				// time_tick      4
				// Running / idle 1
				// Battery        2

				unsigned long tt;
				cli();				
				tt = time_absolute_tick;
				sei();

				*(unsigned long*)answer.payload = time_absolute_offset;
				*(unsigned long*)(answer.payload+4) = tt;
				answer.payload[8] = sample_isrunning();
				*(unsigned short*)(answer.payload+9) = filteredbattery;				
			}
			if(xcommand.cmd=='F')
			{
				// Read out the flash state

				// The answer contains:
				// card_capa      4
				// card_used      4
				// Number of logs 1
				// Running / idle 1				
				// Battery        2

				*(unsigned long*)answer.payload = fs_state.capacity_block;
				*(unsigned long*)(answer.payload+4) = fs_state.firstfreeblock;
				answer.payload[8] = fs_state.numentry;
				answer.payload[9] = sample_isrunning();
				*(unsigned short*)(answer.payload+10) = filteredbattery;
			}		
			if(xcommand.cmd=='L')
			{
				// Start logging.
				if(sample_isrunning() || readystart==0)
					fail=1;
				else
					muststart=1;
			}		
			if(xcommand.cmd=='E')
			{
				// Stop logging.
				if(!sample_isrunning())
					fail=1;
				else
					muststop=1;
			}
			if(xcommand.cmd=='K')
			{		
				// Format
				if(sample_isrunning())
					fail=1;
				else
					mustformat=1;
			}		
			if(fail)							// case of failure
				answer.cmd+=32;
			// send the answer
			sendxcommand(&answer);
		}


		
		// Do some sleeping
		/*ADCSRA=0;			// Must deactivate ADC before sleeping otherwise it automatically fires
		sleep_enable();
		sleep_cpu();*/


		// ----------------------------------------------------------------------
		// Some keyboard interaction - for debug/test
		// ----------------------------------------------------------------------
		/*int c;
		unsigned char mcr=0;
		if((c=fgetc(file_usb))!=EOF)
		{
			if(c=='+')
			{
				fs_state.firstfreeblock+=500000L;
				mcr=1;
			}
			if(c=='-')
			{
				fs_state.firstfreeblock-=400000L;
				mcr=1;
				
			}
			if(c=='e')
			{
				fs_state.firstfreeblock=fs_state.capacity_block-300;
				mcr=1;
				
			}
			if(mcr)
				printf_P(PSTR("firstfreeblock: %ld\r"),fs_state.firstfreeblock);
			if(c=='q' || c=='x')
				break;
		}*/

	}
	printf_P(PSTR("Broke from the main app.\r"));
	uart_setblocking(file_usb,1);
	_delay_ms(100);
	flush(file_usb);

}


/******************************************************************************
Device specific initialization
******************************************************************************/
void init_Specific(void)
{
	uart0_rx_callback = uart0_rx_callback_hook;
}

unsigned char main_checkenterui(void)
{
	unsigned long t1;
	unsigned char enter=0;
	
	// Non-blocking mode
	uart_setblocking(file_usb,0);
	
	printf_P(PSTR("Press return to enter test interface...\r"));
	t1 = timer_ms_get();
	while(timer_ms_get()-t1<2000)
	{
		int c;
		if((c=fgetc(file_usb))!=EOF)
		{
			if(c==0x0A)					// AVR stream functions (fgets) react on 0x0a -> wait until a 0A
			{
				enter=1;
				break;
			}
				
		}
	}
	// Return to blocking mode
	uart_setblocking(file_usb,1);
	return enter;
}








/******************************************************************************
Main program loop
******************************************************************************/
int main(void)
{
	unsigned char ui;
	// Initialization
	init_Module();

	
	ui = main_checkenterui();
	if(ui)
		main_ui();
	else
		main_loggingapplication();
		

	printf_P(PSTR("Infinite loop\r"));
	while(1);	
	return 0;
}
