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

void main_ui(void)
{
	// Get user input
	unsigned char base=0;
	while(1)
	{
		printf_P(PSTR("\r\rSelect what to do: \r"));
		printf_P(PSTR(" 0. Filesystem physical initialization (MMC card)\r"));
		printf_P(PSTR(" 1. Filesystem logical initialization (read root)\r"));
		printf_P(PSTR(" 2. Format\r"));
		printf_P(PSTR(" 3. Write 0 to first blocks and data section\r"));
		printf_P(PSTR(" 4. Write pattern to first blocks and data section\r"));
		printf_P(PSTR(" 5. Read and prettyprint blocks starting from 0\r"));
		printf_P(PSTR(" 6. Read and prettyprint blocks in the data section\r"));
		printf_P(PSTR(" 7. Write data\r"));
		printf_P(PSTR(" 8. Read data\r"));
		printf_P(PSTR("----------------------------------------\r"));
		printf_P(PSTR("10. Log dummy data\r"));
		printf_P(PSTR("11. Log benchmark\r"));
		printf_P(PSTR("12. Log packets (sampling must be active)\r"));
		printf_P(PSTR("13. Add log\r"));
		printf_P(PSTR("----------------------------------------\r"));
		printf_P(PSTR("22. Benchmark read speed\r"));
		printf_P(PSTR("24. Log data with multiple block write\r"));
		printf_P(PSTR("25. Benchmark write speed (block)\r"));
		printf_P(PSTR("26. Benchmark write speed (stream)\r"));
		printf_P(PSTR("27. Benchmark write speed (stream, multiblock)\r"));		
		printf_P(PSTR("28. test mmc 0\r"));		
		printf_P(PSTR("----------------------------------------\r"));
		printf_P(PSTR("30. Stream microphone (format \"é;c\", 7 MSB bits)\r"));
		printf_P(PSTR("31. Stream microphone (format \"é;c\", truncated 7 LSB bits)\r"));
		printf_P(PSTR("32. Stream microphone G10x (format \"é;c\", 7 MSB bits)\r"));
		printf_P(PSTR("33. Stream microphone G10x (format \"é;c\", truncated 7 LSB bits)\r"));
		printf_P(PSTR("34. Stream microphone G200x (format \"é;c\", 7 MSB bits)\r"));
		printf_P(PSTR("35. Stream microphone G200x (format \"é;c\", truncated 7 LSB bits)\r"));
		printf_P(PSTR("36. Test analog inputs\r"));
		printf_P(PSTR("37. Scan I2C bus\r"));
		printf_P(PSTR("38. Test TMP102\r"));
		printf_P(PSTR("39. Test HMC5843\r"));
		printf_P(PSTR("----------------------------------------\r"));
		printf_P(PSTR("40. Print time\r"));
		printf_P(PSTR("----------------------------------------\r"));
		printf_P(PSTR("50. Test radio presence\r"));
		printf_P(PSTR("51. Test radio reset\r"));
		printf_P(PSTR("52. Test radio reset 2\r"));
		printf_P(PSTR("53. Test radio RX\r"));
		printf_P(PSTR("54. Test radio TX\r"));
		printf_P(PSTR("----------------------------------------\r"));
		printf_P(PSTR("60. Test timer 1\r"));
		printf_P(PSTR("61. Start sampling\r"));
		printf_P(PSTR("62. Stop sampling\r"));
		printf_P(PSTR("63. Monitor sampling\r"));
		printf_P(PSTR("64. Monitor sampling with simulated data write\r"));
		printf_P(PSTR("65. Dump acquired packets\r"));
		printf_P(PSTR("66. Start sample and log\r"));
		printf_P(PSTR("67. Start sample and simulated log\r"));
		printf_P(PSTR("----------------------------------------\r"));
		printf_P(PSTR("70. Test UI\r"));
		printf_P(PSTR("72. Led under led manager control\r"));
		printf_P(PSTR("73. Test led blink pattern for capacity\r"));
		printf_P(PSTR("74. Test software I2C interrupt trigger\r"));
		printf_P(PSTR("----------------------------------------\r"));
		printf_P(PSTR("80. Launch the logging application\r"));
		printf_P(PSTR("81. Off\r"));
		printf_P(PSTR("82. Sleep\r"));
		printf_P(PSTR("83. Deactivate digital input buffer on ADC pins\r"));
		printf_P(PSTR("84. Activate digital input buffer on ADC pins\r"));
		printf_P(PSTR("85. Primary port setting\r"));
		printf_P(PSTR("86. Secondary port setting\r"));
		printf_P(PSTR("87. Secondary 2 port setting\r"));
		printf_P(PSTR("88. Test leds\r"));
		printf_P(PSTR("89. Sleep idle led off\r"));
		printf_P(PSTR("90. Sleep power down led off\r"));
		printf_P(PSTR("91. Sleep adc led off\r"));
		printf_P(PSTR("93. Disable analog comparator\r"));
		printf_P(PSTR("94. Enable analog comparator\r"));
		printf_P(PSTR("95. Shut down peripherals\r"));

		
		char userinput[16];
		char rv;
		char *r;
		char *trimmedinput;
		unsigned long t1,t2;

		r = fgets(userinput,16,file_usb);
		trimmedinput = trim(userinput);

		printf_P(PSTR("Got input: '%s'->'%s'\r"),userinput,trimmedinput);
	

		if(strcmp(trimmedinput,"0")==0)
		{
			printf_P(PSTR("Initializing physical filesystem...\r"));
			
			rv = fs_init_physical(&fs_state);
			if(rv)
				printf_P(PSTR("Initialization failed.\r"));
			else
				printf_P(PSTR("Initialization ok.\r"));
		}
		if(strcmp(trimmedinput,"1")==0)
		{

			unsigned char rv = fs_init_logical(&fs_state);
			if(rv)
			{
				printf_P(PSTR("Invalid file system. Format card\r"));
			}
			else
			{
				printf_P(PSTR("Number of entries: %d. Block to use: %lu. Number of available blocks: %lu\r"),fs_state.numentry,fs_state.firstfreeblock,fs_state.capacity_block-fs_state.firstfreeblock);
			}

		}
		if(strcmp(trimmedinput,"2")==0)
		{
			printf_P(PSTR("Formatting\r"));
			fs_format();
		}
		if(strcmp(trimmedinput,"3")==0)
		{
			test_mmc_clear();
		}
		if(strcmp(trimmedinput,"4")==0)
		{
			test_mmc_writepattern();
		}
		if(strcmp(trimmedinput,"5")==0)
		{
			test_mmc_printblocks(0);
		}
		if(strcmp(trimmedinput,"6")==0)
		{
			test_mmc_printblocks(8192);
		}
		


		if(strcmp(trimmedinput,"7")==0)
		{
			printf_P(PSTR("Writing\r"));

			for(unsigned int i=0;i<512;i++)
				sharedbuffer[i]=base+i;

			
			printf_P(PSTR("Start\r"));
			t1 = timer_ms_get();
			for(unsigned int i=0;i<10;i++)			
				rv = mmc_write_block(i<<9,sharedbuffer);
			t2 = timer_ms_get();
			printf_P(PSTR("Stop\r"));
			printf_P(PSTR("Write time: %ld\r"),t2-t1);


			printf_P(PSTR("writeblock returns %d\r"),rv);
			
		}
		if(strcmp(trimmedinput,"8")==0)
		{
			
			printf_P(PSTR("Reading\r"));
			t1 = timer_ms_get();
			//for(unsigned int i=0;i<1000;i++)
				rv = mmc_read_block(0,sharedbuffer);
			t2 = timer_ms_get();
			printf_P(PSTR("readblock returns %d\r"),rv);
			prettyprintblock(sharedbuffer,0);
			printf_P(PSTR("Read time: %ld\r"),t2-t1);
			
		}
		
		
		
		if(strcmp(trimmedinput,"10")==0)
		{
			test_dummylog(&fs_state,deviceid,time_absolute_offset);
		}
		if(strcmp(trimmedinput,"11")==0)
		{
			test_benchmarklog(&fs_state,deviceid,time_absolute_offset);
		}
		if(strcmp(trimmedinput,"12")==0)
		{
			rv = log_dologging(&fs_state,deviceid,time_absolute_offset);
			printf_P(PSTR("log_dologging returned %d\r"),rv);
		}
		if(strcmp(trimmedinput,"13")==0)
		{
			printf_P(PSTR("Ediding entry %d with nextsector: %lu\r"),fs_state.numentry,fs_state.firstfreeblock);
			rv = fs_writeentry(&fs_state,fs_state.numentry,fs_state.numentry,deviceid,0x00,fs_state.firstfreeblock,fs_state.firstfreeblock+100);
			if(rv)
				printf_P(PSTR("Add log failed\r"));
			else
				printf_P(PSTR("Add log ok.\r"));
		}
		if(strcmp(trimmedinput,"24")==0)
		{
			//testlog(nextsector,numentry,1);
		}
		
		if(strcmp(trimmedinput,"22")==0)
		{
			benchmarkread();
		}
		if(strcmp(trimmedinput,"25")==0)
		{
			test_mmc_benchmarkwriteblock();
		}
		if(strcmp(trimmedinput,"26")==0)
		{
			test_mmc_benchmarkwritestream();
		}
		if(strcmp(trimmedinput,"27")==0)
		{
			test_mmc_benchmarkwritestreammulti();
		}
		if(strcmp(trimmedinput,"28")==0)
		{
			test_mmc_0();
		}
		
		if(strcmp(trimmedinput,"30")==0)
		{
			teststreammike(1,0);
		}
		if(strcmp(trimmedinput,"31")==0)
		{
			teststreammike(1,1);
		}
		if(strcmp(trimmedinput,"32")==0)
		{
			teststreammikeGain(0,0);
		}
		if(strcmp(trimmedinput,"33")==0)
		{
			teststreammikeGain(0,1);
		}
		if(strcmp(trimmedinput,"34")==0)
		{
			teststreammikeGain(1,0);
		}
		if(strcmp(trimmedinput,"35")==0)
		{
			teststreammikeGain(1,1);
		}
		if(strcmp(trimmedinput,"36")==0)
		{
			testanalog();
		}
		if(strcmp(trimmedinput,"37")==0)
		{
			testscani2c();
		}
		if(strcmp(trimmedinput,"38")==0)
		{
			testtmp102();
		}
		if(strcmp(trimmedinput,"39")==0)
		{
			testmagn2();
		}
		if(strcmp(trimmedinput,"40")==0)
		{
			test_time();
		}
		if(strcmp(trimmedinput,"50")==0)
		{
			testradio_presence();
		}
		if(strcmp(trimmedinput,"51")==0)
		{
			testradio_reset();
		}
		if(strcmp(trimmedinput,"52")==0)
		{
			testradio_reset2();
		}
		if(strcmp(trimmedinput,"53")==0)
		{
			testradio_rx();
		}
		if(strcmp(trimmedinput,"54")==0)
		{
			testradio_tx();
		}
		if(strcmp(trimmedinput,"60")==0)
		{
			while(1)
			{
				unsigned short cnt;
				cnt = TCNT1;
				printf_P(PSTR("%ld\r"),cnt);
			}

		}
		if(strcmp(trimmedinput,"61")==0)
		{
			sample_start(deviceid,0);
		}
		if(strcmp(trimmedinput,"62")==0)
		{
			sample_stop();
		}
		if(strcmp(trimmedinput,"63")==0)
		{
			sample_monitor();
		}
		if(strcmp(trimmedinput,"64")==0)
		{
			sample_monitor_simwrite();
		}
		if(strcmp(trimmedinput,"65")==0)
		{
			sample_monitor_dumpdata();
		}
		if(strcmp(trimmedinput,"66")==0)
		{
			sample_start(deviceid,0);
			rv = log_dologging(&fs_state,deviceid,time_absolute_offset);
			printf_P(PSTR("log_dologging returned %d\r"),rv);
		}
		if(strcmp(trimmedinput,"67")==0)
		{
			sample_start(deviceid,0);
			sample_monitor_simwrite();
		}
		
		if(strcmp(trimmedinput,"70")==0)
		{
			test_ui();
		}
		if(strcmp(trimmedinput,"72")==0)
		{
			led_setmodeblink(0,1,19,1,0,1);			// LED 0 on low duty cycle
			led_setmodestatic(1);
			led_set(1,0);									// LED 1 off
		}
		if(strcmp(trimmedinput,"73")==0)
		{
			test_led_capacity();
		}
		if(strcmp(trimmedinput,"74")==0)
		{
			test_i2cinttrigger();
		}
		if(strcmp(trimmedinput,"80")==0)
		{
			printf_P(PSTR("Calling the main logging application...\r"));
			main_loggingapplication();
		}
		if(strcmp(trimmedinput,"81")==0)
		{
			off();
		}
		if(strcmp(trimmedinput,"82")==0)
		{
			printf_P(PSTR("Sleeping\r"));
			while(1)
			{
				sleep_enable();
				sleep_cpu();
			}
		}
		if(strcmp(trimmedinput,"83")==0)
		{
			printf_P(PSTR("Deactivate digital input buffer on analog inputs (old value: %02X)\r"),DIDR0);
			DIDR0 = 0xff;			// Disactivate the digital input buffer on the analog inputs
		}
		if(strcmp(trimmedinput,"84")==0)
		{
			printf_P(PSTR("Activate digital input buffer on analog inputs (old value: %02X)\r"),DIDR0);
			DIDR0 = 0x00;			// Disactivate the digital input buffer on the analog inputs
		}
		if(strcmp(trimmedinput,"85")==0)
		{
			printf_P(PSTR("Primary port setting\r"));
			init_InitPort();
		}
		if(strcmp(trimmedinput,"86")==0)
		{
			printf_P(PSTR("Secondary port setting\r"));
			cli();
			DDRB  = INIT_DDRB_ALT;
			PORTB = INIT_PORTB_ALT;	
			DDRC  = INIT_DDRC_ALT;
			PORTC = INIT_PORTC_ALT;	
			DDRD  = INIT_DDRD_ALT;
			PORTD = INIT_PORTD_ALT;
			sei();
		}
		if(strcmp(trimmedinput,"87")==0)
		{
			printf_P(PSTR("Secondary 2 port setting\r"));
			cli();
			DDRB  = INIT_DDRB_ALT2;
			PORTB = INIT_PORTB_ALT2;	
			DDRC  = INIT_DDRC_ALT2;
			PORTC = INIT_PORTC_ALT2;	
			DDRD  = INIT_DDRD_ALT2;
			PORTD = INIT_PORTD_ALT2;
			sei();
		}
		if(strcmp(trimmedinput,"88")==0)
		{
			test_leds();
		}
		if(strcmp(trimmedinput,"89")==0)
		{
			printf_P(PSTR("SLEEP_MODE_IDLE\r"));
			/*led_setmodestatic(0);
			led_setmodestatic(1);
			led_off(0);
			led_off(1);*/
			set_sleep_mode(SLEEP_MODE_IDLE);
			while(1)
			{
				sleep_enable();
				sleep_cpu();
			}
		}
		if(strcmp(trimmedinput,"90")==0)
		{
			printf_P(PSTR("SLEEP_MODE_PWR_DOWN\r"));
			/*led_setmodestatic(0);
			led_setmodestatic(1);
			led_off(0);
			led_off(1);*/
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			
			while(1)
			{
				sleep_enable();
				sleep_cpu();
			}
		}
		if(strcmp(trimmedinput,"91")==0)
		{
			printf_P(PSTR("SLEEP_MODE_ADC\r"));
			/*led_setmodestatic(0);
			led_setmodestatic(1);
			led_off(0);
			led_off(1);*/
			set_sleep_mode(SLEEP_MODE_ADC);
			
			while(1)
			{
				sleep_enable();
				sleep_cpu();
			}
		}
		if(strcmp(trimmedinput,"92")==0)
		{
			printf_P(PSTR("SLEEP_MODE_EXT_STANDBY\r"));
			/*led_setmodestatic(0);
			led_setmodestatic(1);
			led_off(0);
			led_off(1);*/
			set_sleep_mode(SLEEP_MODE_EXT_STANDBY);
			
			while(1)
			{
				sleep_enable();
				sleep_cpu();
			}
		}
		if(strcmp(trimmedinput,"93")==0)
		{
			printf_P(PSTR("Disable analog comparator\r"));
			ACSR = 0x80;
		}
		if(strcmp(trimmedinput,"94")==0)
		{
			printf_P(PSTR("Enable analog comparator\r"));
			ACSR = 0x80;
		}
		if(strcmp(trimmedinput,"94")==0)
		{
			printf_P(PSTR("Shut down peripherals\r"));
			PRR0 = 0xFF;
		}
		
		
		base++;

	}

	
		
}
