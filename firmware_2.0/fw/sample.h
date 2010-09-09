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


#ifndef __SAMPLE_H
#define __SAMPLE_H

/******************************************************************************
*******************************************************************************
	Exported sample parameters and internal variables 
*******************************************************************************
******************************************************************************/

// Statistics about conversions executed
extern volatile unsigned long samplenum_adc[8];
extern volatile unsigned long samplenum_tmp_success,samplenum_tmp_fail;
extern volatile unsigned long samplenum_hmc_success[2],samplenum_hmc_fail[2];

/******************************************************************************
*******************************************************************************
	Packet encapsulation
*******************************************************************************
******************************************************************************/

// Type of packets
#define PACKET_TYPE_AUDIO				0
#define PACKET_TYPE_ACC					1
#define PACKET_TYPE_SYSTEM				2
#define PACKET_TYPE_LIGHT				3
#define PACKET_TYPE_TMP					4
#define PACKET_TYPE_HMC					5

// Number of packets
#define PACKET_NUM_AUDIO				16
#define PACKET_NUM_ACC					5
#define PACKET_NUM_SYSTEM				5
#define PACKET_NUM_LIGHT				5
#define PACKET_NUM_TMP					5
#define PACKET_NUM_HMC					5

// Size of packets in samples
#define PACKET_NUM_AUDIO_SAMPLE		192
#define PACKET_NUM_ACC_SAMPLE 		32
#define PACKET_NUM_SYSTEM_SAMPLE 	32
#define PACKET_NUM_LIGHT_SAMPLE 		32
#define PACKET_NUM_TMP_SAMPLE			32
#define PACKET_NUM_HMC_SAMPLE			32

// Size of packets in bytes (always adding size of PACKET_PREAMBLE and checksum (2)
#define PACKET_SIZE_AUDIO				(PACKET_PREAMBLE+PACKET_NUM_AUDIO_SAMPLE*2 + 2)		// Sized for 1 16-bit audio sample
#define PACKET_SIZE_ACC					(PACKET_PREAMBLE+3*PACKET_NUM_ACC_SAMPLE*2 + 2)		// Sized for 3 16-bit acceleration values
#define PACKET_SIZE_SYSTEM				(PACKET_PREAMBLE+1*PACKET_NUM_SYSTEM_SAMPLE*2 + 2)		// Sized for 1 16-bit battery values
#define PACKET_SIZE_LIGHT				(PACKET_PREAMBLE+1*PACKET_NUM_LIGHT_SAMPLE*2 + 2)		// Sized for 1 16-bit light values
#define PACKET_SIZE_TMP					(PACKET_PREAMBLE+1*PACKET_NUM_TMP_SAMPLE*2 + 2)		// Sized for 1 16-bit temperature value
#define PACKET_SIZE_HMC					(PACKET_PREAMBLE+3*PACKET_NUM_HMC_SAMPLE*2 + 2)		// Sized for 3 16-bit compass values

// Packet buffers
extern unsigned char packet_audio[PACKET_NUM_AUDIO][PACKET_SIZE_AUDIO];								// packet holding audio samples
extern unsigned char packet_acc[PACKET_NUM_ACC][PACKET_SIZE_ACC];										// packets holding acceleration samples
extern unsigned char packet_system[PACKET_NUM_SYSTEM][PACKET_SIZE_SYSTEM];							// packets holding system samples
extern unsigned char packet_light[PACKET_NUM_LIGHT][PACKET_SIZE_LIGHT];								// packets holding light samples
extern unsigned char packet_tmp[PACKET_NUM_TMP][PACKET_SIZE_TMP];										// packets holding temperature samples
extern unsigned char packet_hmc[PACKET_NUM_HMC][PACKET_SIZE_HMC];										// packets holding compass samples


// Packet buffer holder structure
extern PACKETHOLDER packetholder_audio;
extern PACKETHOLDER packetholder_acc;
extern PACKETHOLDER packetholder_system;
extern PACKETHOLDER packetholder_light;
extern PACKETHOLDER packetholder_tmp;
extern PACKETHOLDER packetholder_hmc;


/******************************************************************************
*******************************************************************************
	Sample methods
*******************************************************************************
******************************************************************************/



void sample_start(unsigned long deviceid,unsigned char entry);
void sample_stop(void);
unsigned char sample_isrunning(void);

void sample_monitor(void);
void sample_monitor_simwrite(void);
void sample_monitor_dumpdata(void);
void sample_printstat(void);
void pkt_printstat(void);

unsigned char sample_logpacket(FS_STATE *fs_state,unsigned long deviceid,unsigned long time_absolute_offset);


#endif
