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
#include "helper2.h"
#include "led.h"



/******************************************************************************
*******************************************************************************
	Sample parameters and internal variables 
*******************************************************************************
******************************************************************************/

unsigned char _sample_acquire_enabled=0;									// Sampling running?

// When opening packets the device and entry 
unsigned long _sample_deviceid;
unsigned long _sample_entry;

unsigned short _sample_audio_ctr;								// Low-resulution high-frequency counter. Units: audio samples
//const unsigned short _sample_acceleration_mask = 255;	//	Acceleration sample mask
const unsigned short _sample_acceleration_mask = 63;		//	Acceleration sample mask 125Hz@8KHz (default)
//const unsigned short _sample_acceleration_mask = 4095;		//	Acceleration sample mask 1.95@8KHz
const unsigned short _sample_system_mask = 511;				//	System (battery) sample mask 15.625Hz @ 8KHz (default)
//const unsigned short _sample_system_mask = 1023;				//	System (battery) sample mask 7.8125@ 8KHz
//const unsigned short _sample_system_mask = 2047;				//	System (battery) sample mask 4Hz@ 8KHz
const unsigned short _sample_light_mask = 127;				//	Light sample mask 62.5Hz @ 8KHz (default)
//const unsigned short _sample_light_mask = 255;				//	Light sample mask 32Hz @ 8KHz
//const unsigned short _sample_light_mask = 511;				//	Light sample mask 16Hz @ 8KHz
//const unsigned short _sample_light_mask = 1023;				//	Light sample mask 8Hz @ 8KHz
//const unsigned short _sample_light_mask = 2047;				//	Light sample mask 4Hz @ 8KHz
const unsigned short _sample_tmp_mask = 1023;				//	Temperature sample mask 7.8125 @ 8KHz (default)
//const unsigned short _sample_hmc_mask = 63;					//	Compass sample mask 125Hz@8KHz (default)
const unsigned short _sample_hmc_mask = 127;					//	Compass sample mask 62.5Hz@8KHz (new default, since in HW we are at 50Hz max)
//const unsigned short _sample_hmc_mask = 1023;					//	Compass sample mask 7.8125Hz@8KHz


// Sample time 
// Contains the time when the sample was triggered
// - used when opening a new packet
// Does not contain the phase shift 
unsigned long sampletime_audio;
unsigned long sampletime_acc;
unsigned long sampletime_system;
unsigned long sampletime_light;
unsigned long sampletime_tmp;
unsigned long sampletime_hmc;


// Statistics about conversions executed
volatile unsigned long samplenum_adc[8];
volatile unsigned long samplenum_tmp_success,samplenum_tmp_fail;
volatile unsigned long samplenum_hmc_success[2],samplenum_hmc_fail[2];

// Led toggling
unsigned _sample_togglet0,_sample_togglet1;

// I2C transactions
I2C_TRANSACTION i2ctrans_tmp,i2ctrans_hmc_select,i2ctrans_hmc_read;
// HMC internal vars
unsigned char hmc_data_error[6]={0xff,0xff,0xff,0xff,0xff,0xff};


/* Chaining of ADC conversions
	adcconv_type:
		0:	audio
		1:	Acc X
		2:	Acc Y
		3:	Acc Z
		4:	Light
		5:	Voltage
		
*/
// ADC: 15ksps at max resolution (need here 16...)
// Input clock: 50KHz-200KHz
// Normal conversion: 13 clock. First conversion: 25 clock.
// Due to the 25 clocks when enabling the ADC, and the fact that to sleep one needs to disable the ADC to avoid it starting a conversion, we must choose between:
// - sleep mode and ADCDiv=16
// - No sleep mode and ADCDiv=32
//#define ADCCONV_START 0xCC								// Prescaler 16
//#define ADCCONV_START 0xC8								// Prescaler 2. 
// For no sleep mode: 32
#define ADCCONV_START 0xCD								// Prescaler 32. Conversion time: 12MHz/32/13=28846Hz (34uS per conv. 2 conv=69uS. 8KHz: time avail is 125uS). Note clock is 375'000, above the 200KHz for max resolution
// For sleep mode: 16
//#define ADCCONV_START 0xCC									// Prescaler 16. Conversion time: 12MHz/16/25=30000Hz (33uS per conv. 2 conv=66uS. 8KHz: time avail is 125uS). Note clock is 750'000, above the 200KHz for max resolution
//#define ADCCONV_START 0xCB									// Prescaler 16. Conversion time: 12MHz/16/25=30000Hz (33uS per conv. 2 conv=66uS. 8KHz: time avail is 125uS). Note clock is 750'000, above the 200KHz for max resolution

unsigned char adcconv_chain_type=0;
unsigned char adcconv_chain_config=0;
unsigned char adcconv_type=0;



/******************************************************************************
*******************************************************************************
	Packet encapsulation
*******************************************************************************
******************************************************************************/

// Packet buffers
unsigned char packet_audio[PACKET_NUM_AUDIO][PACKET_SIZE_AUDIO];								// packet holding audio samples
unsigned char packet_acc[PACKET_NUM_ACC][PACKET_SIZE_ACC];										// packets holding acceleration samples
unsigned char packet_system[PACKET_NUM_SYSTEM][PACKET_SIZE_SYSTEM];							// packets holding system samples
unsigned char packet_light[PACKET_NUM_LIGHT][PACKET_SIZE_LIGHT];								// packets holding light samples
unsigned char packet_tmp[PACKET_NUM_TMP][PACKET_SIZE_TMP];										// packets holding temperature samples
unsigned char packet_hmc[PACKET_NUM_HMC][PACKET_SIZE_HMC];										// packets holding compass samples


// Packet buffer holder structure
PACKETHOLDER packetholder_audio;
PACKETHOLDER packetholder_acc;
PACKETHOLDER packetholder_system;
PACKETHOLDER packetholder_light;
PACKETHOLDER packetholder_tmp;
PACKETHOLDER packetholder_hmc;


unsigned short tcnt1;

/******************************************************************************
*******************************************************************************
	Interrupts&callbacks
*******************************************************************************
******************************************************************************/

unsigned char time_tick_100hz;																			// counter used to generate a 100Hz interrupt (can be char)
unsigned short time_tick_10hz;																			// counter used to generate a 100Hz interrupt (must be short)


/******************************************************************************
	ADC Conversion Complete.
******************************************************************************/
ISR(ADC_vect)
{
	unsigned short adcw = ADCW; 
	unsigned char curadcconv_type;

	samplenum_adc[adcconv_type]++;

	// Immediately trigger the new conversion, so that the conversion occurs while processing the rest of this interrupt
	curadcconv_type = adcconv_type;				// Copy the conversion type for use in the rest of the routine
	if(adcconv_chain_config)						// Chain ADC conversions?
	{
		ADMUX = adcconv_chain_config;				// Setup next conversion
		adcconv_chain_config=0;						// None afterwards
		adcconv_type = adcconv_chain_type;		// Type of next conversion
		ADCSRA = ADCCONV_START;						// Convert
	}

	
	switch(curadcconv_type)
	{
		// ------------------------------------------------------
		// Audio 
		// ------------------------------------------------------
		case 0:	// Audio
			if(!packetholder_audio.writeable)
			{
				// Not writeable - attempts to open a packet
				if(packetholder_write_next(&packetholder_audio)!=-1)
				{
					// If successful, init the packet with the packet sample time
					packet_init(packet_audio[packetholder_audio.wp],PACKET_TYPE_AUDIO,sampletime_audio,_sample_deviceid,_sample_entry,PACKET_NUM_AUDIO_SAMPLE,1,0);	
				}
			}
			// Store the data in the packet if it is open
			if(packetholder_audio.writeable)		// Packet is open
			{
				// Store the data at position (in bytes) PACKET_PREAMBLE + dataptr*2
				((unsigned short*)packet_audio[packetholder_audio.wp])[(PACKET_PREAMBLE>>1)+packetholder_audio.dataptr] = adcw;
				//((unsigned short*)packet_audio[packetholder_audio.wp])[(PACKET_PREAMBLE>>1)+packetholder_audio.dataptr] = tcnt1;
				packetholder_audio.dataptr++;
				// Is packet full?
				if(packetholder_audio.dataptr>=PACKET_NUM_AUDIO_SAMPLE)
					packetholder_write_done(&packetholder_audio);
			}
			
			break;

		// ------------------------------------------------------
		// Acceleration
		// ------------------------------------------------------
		case 1:	// X
			if(!packetholder_acc.writeable)
			{
				// Not writeable - attempts to open a packet
				if(packetholder_write_next(&packetholder_acc)!=-1)
				{
					// If successful, init the packet with the packet sample time
					packet_init(packet_acc[packetholder_acc.wp],PACKET_TYPE_ACC,sampletime_acc,_sample_deviceid,_sample_entry,PACKET_NUM_ACC_SAMPLE,1*(_sample_acceleration_mask+1),0);	
				}
			}
			// Flows to the next case to actually store the data
		case 2:	// Y
		case 3:	// Z
			// Store the data in the packet if it is open
			if(packetholder_acc.writeable)		// Packet is open
			{
				// Store the data at position (in bytes) PACKET_PREAMBLE + dataptr*2
				((unsigned short*)packet_acc[packetholder_acc.wp])[(PACKET_PREAMBLE>>1)+packetholder_acc.dataptr*3+curadcconv_type-1] = adcw;
			
				if(curadcconv_type==3)
				{
					packetholder_acc.dataptr++;
					// Is packet full?
					if(packetholder_acc.dataptr>=PACKET_NUM_ACC_SAMPLE)
						packetholder_write_done(&packetholder_acc);
				}
			}
			break;

		// ------------------------------------------------------
		// Light
		// ------------------------------------------------------
		case 4:
			if(!packetholder_light.writeable)
			{
				// Not writeable - attempts to open a packet
				if(packetholder_write_next(&packetholder_light)!=-1)
				{
					// If successful, init the packet with the packet sample time
					packet_init(packet_light[packetholder_light.wp],PACKET_TYPE_LIGHT,sampletime_light,_sample_deviceid,_sample_entry,PACKET_NUM_LIGHT_SAMPLE,_sample_light_mask+1,0);
				}
			}
			if(packetholder_light.writeable)		// Packet is open
			{
				// Store the data at position (in bytes) PACKET_PREAMBLE + dataptr*2
				((unsigned short*)packet_light[packetholder_light.wp])[(PACKET_PREAMBLE>>1)+packetholder_light.dataptr] = adcw;		
				packetholder_light.dataptr++;
				// Is packet full?
				if(packetholder_light.dataptr>=PACKET_NUM_LIGHT_SAMPLE)
					packetholder_write_done(&packetholder_light);
			}
			break;
		// ------------------------------------------------------
		// System
		// ------------------------------------------------------
		case 5:
			if(!packetholder_system.writeable)
			{
				// Not writeable - attempts to open a packet
				if(packetholder_write_next(&packetholder_system)!=-1)
				{
					// If successful, init the packet with the packet sample time
					packet_init(packet_system[packetholder_system.wp],PACKET_TYPE_SYSTEM,sampletime_system,_sample_deviceid,_sample_entry,PACKET_NUM_SYSTEM_SAMPLE,_sample_system_mask+1,0);
				}
			}
			if(packetholder_system.writeable)		// Packet is open
			{
				// Store the data at position (in bytes) PACKET_PREAMBLE + dataptr*2
				((unsigned short*)packet_system[packetholder_system.wp])[(PACKET_PREAMBLE>>1)+packetholder_system.dataptr] = adcw;		
				packetholder_system.dataptr++;
				// Is packet full?
				if(packetholder_system.dataptr>=PACKET_NUM_SYSTEM_SAMPLE)
					packetholder_write_done(&packetholder_system);
			}
			batteryvoltage = adcw;
			break;
		default:
			;
	}

}


/******************************************************************************
	I2C TMP callback
******************************************************************************/
unsigned char sample_i2c_callback_tmp(I2C_TRANSACTION *tran)
{
	if(tran->status==0)
		samplenum_tmp_success++;
	else
	{
		samplenum_tmp_fail++;
		tran->data[0]=0xFF;			// Mark data as NAN
		tran->data[1]=0xFF;
	}

	if(!packetholder_tmp.writeable)
	{
		// Not writeable - attempts to open a packet
		if(packetholder_write_next(&packetholder_tmp)!=-1)
		{
			// If successful, init the packet with the packet sample time
			packet_init(packet_tmp[packetholder_tmp.wp],PACKET_TYPE_TMP,sampletime_tmp,_sample_deviceid,_sample_entry,PACKET_NUM_TMP_SAMPLE,1*(_sample_tmp_mask+1),0);	
		}
	}
	if(packetholder_tmp.writeable)		// Packet is open
	{
		unsigned short d;
		d=(((unsigned short)tran->data[0])<<8) | tran->data[1];

		// Store the data at position (in bytes) PACKET_PREAMBLE + dataptr*2
		((unsigned short*)packet_tmp[packetholder_tmp.wp])[(PACKET_PREAMBLE>>1)+packetholder_tmp.dataptr] = d;
		packetholder_tmp.dataptr++;
		// Is packet full?
		if(packetholder_tmp.dataptr>=PACKET_NUM_TMP_SAMPLE)
			packetholder_write_done(&packetholder_tmp);
	}
	return 0;
}
/******************************************************************************
	I2C HMC callback
******************************************************************************/
unsigned char sample_i2c_callback_hmc(I2C_TRANSACTION *tran)
{
	unsigned char *data,store;

	store=0;
	// If the transaction failed we log the failure, and store NAN data (the transaction that generated the error is irrelevant)
	if(tran->status)
	{
		// Log failure
		if(tran == &i2ctrans_hmc_select)
			samplenum_hmc_fail[0]++;
		else
			samplenum_hmc_fail[1]++;

		// Store dummy data....
		store=1;
		data = hmc_data_error;
	}
	else	
	// If the transaction successful we log the success, see wether to initiate the read transaction, and store the read data
	{
		if(tran == &i2ctrans_hmc_select)
		{
			// Select transaction was successful. Initiate a read transaction.
			samplenum_hmc_success[0]++;

			i2c_transaction_execute(&i2ctrans_hmc_read,0);
		}
		else
		{
			// Read transaction was successful. Store the data
			samplenum_hmc_success[1]++;

			// Store the successfully read data
			store=1;
			data=tran->data;
		}
	}
	if(store)
	{
		if(!packetholder_hmc.writeable)
		{
			// Not writeable - attempts to open a packet
			if(packetholder_write_next(&packetholder_hmc)!=-1)
			{
				// If successful, init the packet with the packet sample time
				packet_init(packet_hmc[packetholder_hmc.wp],PACKET_TYPE_HMC,sampletime_hmc,_sample_deviceid,_sample_entry,PACKET_NUM_HMC_SAMPLE,_sample_hmc_mask+1,0);
			}
		}
		if(packetholder_hmc.writeable)		// Packet is open
		{
			// Store the data at position (in bytes) PACKET_PREAMBLE + dataptr*6
			unsigned char *ptr = packet_hmc[packetholder_hmc.wp] + PACKET_PREAMBLE+6*packetholder_hmc.dataptr;
			for(unsigned char i=0;i<6;i++)
				ptr[i] = tran->data[i];

			packetholder_hmc.dataptr++;
			// Is packet full?
			if(packetholder_hmc.dataptr>=PACKET_NUM_HMC_SAMPLE)
				packetholder_write_done(&packetholder_hmc);
		}
	}
	return 0;
}


/******************************************************************************
	Sample timer interrupt vector 
******************************************************************************/
/*
	Sampling is triggered synchronously to the audio stuff.
	Care is taken to ensure that the time consuming sampling start operation do not start at the same time with a phase shift: ...(_sample_audio_ctr-PSHIFT)&mask...
*/
ISR(TIMER1_COMPA_vect)
{
	// This is the system's clocks - must always be executed
	time_absolute_tick++;						// Absolute time. must ALWAYS tick. Audio units (defined according to TIMER1 interrupt frequency). Typ: 8KHz

	/*INCREMENT_WRAP(time_tick_100hz,8000/100);		//	100Hz timer. Must always be executed
	if(!time_tick_100hz)
		timer_callback_100hz();*/
	INCREMENT_WRAP(time_tick_10hz,8000/10);		//	10Hz timer. Must always be executed
	if(!time_tick_10hz)
		timer_callback_10hz();
	

	// Below this, only executed when sampling data.
	if(!_sample_acquire_enabled)
		return;

	

	// ------------------------------------------------------
	// Audio sampling
	// 			Flag the current conversion as HF/audio
	//				Trigger audio sample
	// 			Always executed
	// ------------------------------------------------------
	adcconv_type = 0;
	ADMUX = 0x41;										// Vref=VCC, channel audio, gain 1
	//ADMUX = 0x49;										// Vref=VCC, channel audio, gain 10
	//ADMUX = 0x4B;										// Vref=VCC, channel audio, gain 200
	ADCSRA = ADCCONV_START;							// Start conversion, perscaler 16, Interrup on conversion end. Two conversions must be executable within one HF sample interval.
	sampletime_audio=time_absolute_tick;		// Time of audio sample request


	// ------------------------------------------------------
	// Acceleration sampling
	// 			Executed every _sample_acceleration_mask+1 audio sample
	//				Chained on the audio sample conversion
	// ------------------------------------------------------
	// X acceleration
	if(((_sample_audio_ctr-1)&_sample_acceleration_mask)==0)
	{
		adcconv_chain_config=0x42;	// Vref=VCC, channel 2
		adcconv_chain_type=1;		// X	
		sampletime_acc=time_absolute_tick-1;	// Time of acc sample request (only for X - packets are only opened on an X)
	}
	// Y acceleration
	if(((_sample_audio_ctr-2)&_sample_acceleration_mask)==0)
	{
		adcconv_chain_config=0x43;	// Vref=VCC, channel 3
		adcconv_chain_type=2;		// Y
	}
	// Z acceleration
	if(((_sample_audio_ctr-3)&_sample_acceleration_mask)==0)
	{
		adcconv_chain_config=0x44;	// Vref=VCC, channel 4
		adcconv_chain_type=3;		// Z
	}

	// ------------------------------------------------------
	// Light 
	// ------------------------------------------------------
	//		Chained on the audio sample conversion
	// ------------------------------------------------------
	if(((_sample_audio_ctr-4)&_sample_light_mask)==0)
	{
		adcconv_chain_config=0x45;	// Vref=VCC, channel 5
		adcconv_chain_type=4;		// Light
		sampletime_light=time_absolute_tick-4;		// Time of system sample request
	}
	// ------------------------------------------------------
	// System/Voltage
	// ------------------------------------------------------
	//		Chained on the audio sample conversion
	// ------------------------------------------------------
	if(((_sample_audio_ctr-5)&_sample_system_mask)==0)
	{
		adcconv_chain_config=0x46;	// Vref=VCC, channel 6
		adcconv_chain_type=5;		// Voltage
		sampletime_system=time_absolute_tick-5;		// Time of light sample request
	}
	// ------------------------------------------------------
	// Temperature
	// 			Executed every _sample_tmp_mask+1 audio sample
	//				Temperature conversion time: 
	//					444/1000 ms @ 100KHz
	//					167/1000 ms @ 400KHz
	//					Worst case: 0.5ms/conversion.
	//					In audio units: 4u @ 8KHz, 6u @ 10KHz
	//						Wait ~10 u until I2C bus is likely available for the next I2C transaction
	// ------------------------------------------------------
	if(((_sample_audio_ctr-6)&_sample_tmp_mask)==0)
	{
		// Check if bus is idle. If not, likely an error occured and the transaction is cancelled to issue the new one.
		if(!i2c_transaction_idle())
		{
			i2c_transaction_cancel();
		}

		// Initiate the I2C reading...
		i2c_transaction_execute(&i2ctrans_tmp,0);
		sampletime_tmp = time_absolute_tick-6;
	}
	// ------------------------------------------------------
	// Compass
	// 			Executed every _sample_hmc_mask+1 audio sample
	//				Chained on the audio sample conversion
	//				Select register time:
	//					250/1000 ms @ 100KHz
	//					 98/1000 ms @ 400KHz
	//				Read data register time:
	//					852/1000 ms @ 100KHz
	//					308/1000 ms @ 400KHz
	//					Worst case total: 1.6ms/conversion.
	//					In audio units: 13u @ 8KHz, 20u @ 10KHz
	//						Wait ~25 u until I2C bus is likely available for the next I2C transaction
	// ------------------------------------------------------
	if(((_sample_audio_ctr-16)&_sample_hmc_mask)==0)
	{
		// Check if bus is idle. If not, likely an error occured and the transaction is cancelled to issue the new one.
		if(!i2c_transaction_idle())
		{
			i2c_transaction_cancel();
		}
		// Initiate the I2C reading...
		i2c_transaction_execute(&i2ctrans_hmc_select,0);
		sampletime_hmc = time_absolute_tick-16;
	}

	// ------------------------------------------------------
	// LED lifesigns and card capacity indicator
	// ------------------------------------------------------

	if(_sample_audio_ctr==0)				// Periodically recompute when to toggle the led - this is about every 8 seconds
	{
		_sample_togglet0 = (unsigned)(448L*fs_state.firstfreeblock/(fs_state.capacity_block>>4))+1023;
		_sample_togglet1 = _sample_togglet0+8192;
	}
	if((_sample_audio_ctr&16383)==0)
	{
		led_toggle(0);
		led_enact_nocli();
	}
	if((_sample_audio_ctr&16383)==_sample_togglet0)
	{
		led_toggle(0);
		led_enact_nocli();
	}
	if((_sample_audio_ctr&16383)==8192)
	{
		led_toggle(1);
		led_enact_nocli();
	}
	if((_sample_audio_ctr&16383)==_sample_togglet1)
	{
		led_toggle(1);
		led_enact_nocli();
	}

	/*if((_sample_audio_ctr&8191)==0)			// Periodically indicates aliveness and recording.
	{
		led_toggle(0);
		led_toggle(1);
		led_enact_nocli();
	}*/
	_sample_audio_ctr++;

	tcnt1 = TCNT1;	
}


/******************************************************************************
*******************************************************************************
	Control methods
*******************************************************************************
******************************************************************************/

/******************************************************************************
	Starts data sampling
******************************************************************************/
void sample_start(unsigned long deviceid,unsigned char entry)
{
	// Keep the device ID and 
	_sample_deviceid = deviceid;
	_sample_entry=entry;

	// Initialize the low-resolution helper counters
	_sample_audio_ctr=0;

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
	
	//HMC Register read transaction 
	i2ctrans_hmc_read.address=0x1E;
	i2ctrans_hmc_read.rw=I2C_READ;
	i2ctrans_hmc_read.dostart=1;
	i2ctrans_hmc_read.doaddress=1;
	i2ctrans_hmc_read.dodata=7;
	i2ctrans_hmc_read.dostop=1;
	for(unsigned char i=0;i<7;i++)
		i2ctrans_hmc_read.dataack[i]=(i==6)?0:1;			// Last data received must answer with not ack
	i2ctrans_hmc_read.callback=sample_i2c_callback_hmc;

	// Initialize the ADC: must make at least ONE conversion to initialize the ADC and obtain 13 clock per conversion
	ADCSRA = ADCCONV_START&(~8);						// Convert, but does not generate an interrupt
	while(ADCSRA&0x40);									// Wait for the conversion to complete
	

	// Initialize the packet holders
	packetholder_init(&packetholder_audio,PACKET_NUM_AUDIO);
	packetholder_init(&packetholder_acc,PACKET_NUM_ACC);
	//packetholder_init(&packetholder_analogaux,PACKET_NUM_ANALOGAUX);
	packetholder_init(&packetholder_system,PACKET_NUM_SYSTEM);
	packetholder_init(&packetholder_light,PACKET_NUM_LIGHT);
	packetholder_init(&packetholder_tmp,PACKET_NUM_TMP);
	packetholder_init(&packetholder_hmc,PACKET_NUM_HMC);

	// Initialize the statistics
	memset((void*)samplenum_adc,0,8*sizeof(unsigned long));
	samplenum_tmp_success=0;
	samplenum_tmp_fail=0;
	samplenum_hmc_success[0]=samplenum_hmc_success[1]=0;
	samplenum_hmc_fail[0]=samplenum_hmc_fail[1]=0;



	led_setmodestatic(0);
	led_setmodestatic(1);
	led_off(0);
	led_off(1);
	led_enact();
	_sample_audio_ctr=0;
	_sample_acquire_enabled=1;
	
}
/******************************************************************************
	Stop data sampling
******************************************************************************/
void sample_stop(void)
{
	printf_P(PSTR("In sample stop\r"));
	_sample_acquire_enabled=0;
	led_warning(1);
	led_idle();
}

/******************************************************************************
	Returns if sampling is running
******************************************************************************/
unsigned char sample_isrunning(void)
{
	return _sample_acquire_enabled;
}

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

		sample_printstat();

		pkt_printstat();
		
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
	unsigned long int t1,t2;

	t1 = t2 = timer_ms_get();
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
			
			//printf_P(PSTR("Wrote audio %d. %02lu/%02lu\r"),rp,packetholder_audio.numerr,packetholder_audio.total);

			packetholder_read_done(&packetholder_audio);
		}
		// Acceleration
		rp=packetholder_read_next(&packetholder_acc);
		if(rp!=-1)
		{
			// Simulate taking time
			_delay_ms(simdelay);
			
			//printf_P(PSTR("Wrote acc %d. %02lu/%02lu\r"),rp,packetholder_acc.numerr,packetholder_acc.total);

			packetholder_read_done(&packetholder_acc);
		}
		// System
		rp=packetholder_read_next(&packetholder_system);
		if(rp!=-1)
		{
			// Simulate taking time
			_delay_ms(simdelay);
			
			//printf_P(PSTR("Wrote system %d. %02lu/%02lu\r"),rp,packetholder_system.numerr,packetholder_system.total);

			packetholder_read_done(&packetholder_system);
		}
		// Light
		rp=packetholder_read_next(&packetholder_light);
		if(rp!=-1)
		{
			// Simulate taking time
			_delay_ms(simdelay);
			
			//printf_P(PSTR("Wrote light %d. %02lu/%02lu\r"),rp,packetholder_light.numerr,packetholder_light.total);

			packetholder_read_done(&packetholder_light);
		}
		// tmp
		rp=packetholder_read_next(&packetholder_tmp);
		if(rp!=-1)
		{
			// Simulate taking time
			_delay_ms(simdelay);
			
			//printf_P(PSTR("Wrote tmp %d. %02lu/%02lu\r"),rp,packetholder_tmp.numerr,packetholder_tmp.total);

			packetholder_read_done(&packetholder_tmp);
		}
		// hmc
		rp=packetholder_read_next(&packetholder_hmc);
		if(rp!=-1)
		{
			// Simulate taking time
			_delay_ms(simdelay);
			
			//printf_P(PSTR("Wrote hmc %d. %02lu/%02lu\r"),rp,packetholder_hmc.numerr,packetholder_hmc.total);

			packetholder_read_done(&packetholder_hmc);
		}

		// Display more detailed infos at regular intervals
		if(timer_ms_get()>t2)
		{		
			sample_printstat();
			pkt_printstat();
			t2+=501;
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
	// System
	while( (rp=packetholder_read_next(&packetholder_system)) != -1)
	{
		printf_P(PSTR("System packet %d"),rp);
		unsigned short *data = ((unsigned short*)packet_system[rp])+(PACKET_PREAMBLE>>1);
		for(unsigned i=0;i<PACKET_NUM_SYSTEM_SAMPLE;i++)
		{
			if((i&(colacc-1))==0)
				printf_P(PSTR("\r"));
			printf_P(PSTR("%04X "),*data);
			data++;
		}
		printf_P(PSTR("\r"));
		packetholder_read_done(&packetholder_system);
	}
	// Light
	while( (rp=packetholder_read_next(&packetholder_light)) != -1)
	{
		printf_P(PSTR("Light packet %d"),rp);
		unsigned short *data = ((unsigned short*)packet_light[rp])+(PACKET_PREAMBLE>>1);
		for(unsigned i=0;i<PACKET_NUM_LIGHT_SAMPLE;i++)
		{
			if((i&(colacc-1))==0)
				printf_P(PSTR("\r"));
			printf_P(PSTR("%04X "),*data);
			data++;
		}
		printf_P(PSTR("\r"));
		packetholder_read_done(&packetholder_light);
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


void sample_printstat(void)
{
	printf_P(PSTR("ADC:"));
	for(unsigned char i=0;i<8;i++)
		printf_P(PSTR("%06ld "),samplenum_adc[i]);
	printf_P(PSTR(" TMP S|F: %05ld %05ld"),samplenum_tmp_success,samplenum_tmp_fail);
	printf_P(PSTR(" HMC_S S|F: %05ld %05ld HMC_R S|F: %05ld %05ld"),samplenum_hmc_success[0],samplenum_hmc_fail[0],samplenum_hmc_success[1],samplenum_hmc_fail[1]);
	printf_P(PSTR("\r"));
}


/******************************************************************************
	Print stats about stored packets
******************************************************************************/
void pkt_printstat(void)
{
	// Log calls
	//printf_P(PSTR("LOG S|F: %04lu|%054lu\t"),fs_state->_log_writeok,fs_state->_log_writeerror);
	// Audio packets
	printf_P(PSTR("audio: %02lu/%02lu w%d p%02d n%02d %03d\t"),
		packetholder_audio.numerr,packetholder_audio.total,
		(int)packetholder_audio.writeable,(int)packetholder_audio.wp,(int)packetholder_audio.nw,(int)packetholder_audio.dataptr);
	// Acceleration packets
	printf_P(PSTR("acc: %02lu/%02lu w%d p%02d n%02d %03d\t"),
		packetholder_acc.numerr,packetholder_acc.total,
		(int)packetholder_acc.writeable,(int)packetholder_acc.wp,(int)packetholder_acc.nw,(int)packetholder_acc.dataptr);
	// System packets
	printf_P(PSTR("sys: %02lu/%02lu w%d p%02d n%02d %03d\t"),
		packetholder_system.numerr,packetholder_system.total,
		(int)packetholder_system.writeable,(int)packetholder_system.wp,(int)packetholder_system.nw,(int)packetholder_system.dataptr);
	// Light packets
	printf_P(PSTR("light: %02lu/%02lu w%d p%02d n%02d %03d\t"),
		packetholder_light.numerr,packetholder_light.total,
		(int)packetholder_light.writeable,(int)packetholder_light.wp,(int)packetholder_light.nw,(int)packetholder_light.dataptr);
	// tmp packets
	printf_P(PSTR("tmp: %02lu/%02lu w%d p%02d n%02d %03d\t"),
		packetholder_tmp.numerr,packetholder_tmp.total,
		(int)packetholder_tmp.writeable,(int)packetholder_tmp.wp,(int)packetholder_tmp.nw,(int)packetholder_tmp.dataptr);
	// hmc packets
	printf_P(PSTR("hmc: %02lu/%02lu w%d p%02d n%02d %03d\t"),
		packetholder_hmc.numerr,packetholder_hmc.total,
		(int)packetholder_hmc.writeable,(int)packetholder_hmc.wp,(int)packetholder_hmc.nw,(int)packetholder_hmc.dataptr);
	printf_P(PSTR("\r"));
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

