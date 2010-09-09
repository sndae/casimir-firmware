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


#ifndef __FS_H
#define __FS_H

#include "mmc.h"


/******************************************************************************
Filesystem organization
*******************************************************************************
	
The block 0 is laid out as follows:

	The first 14 bytes contain the root identifier, parameters, and the number of log entries
	+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+-----
	|   R    |   O    |   O    |   T    |   v0   |   v1   |   v2   |   v3   | res(0) | res(0) | res(0) | res(0) | res(0) | numlog | ...
	+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+-----
	Successive bytes contain the log entries (16 byte per log entry):
			-----+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+-----
			 ... |  DID0  |  DID1  |  DID2  |  DID3  |  TIM0  |  TIM1  |  TIM2  |  TIM3  | START0 | START1 | START2 | START3 |  END0  |  END0  |  END0  |  END0  | ...
			-----+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+-----
	The last two bytes contain a 16-bit simple checksum on first 510 bytes:
																		-----+--------+--------+
																		 ... | XSUM0  | XSUM1  | 
																		-----+--------+--------+

******************************************************************************/

/******************************************************************************
Constants
******************************************************************************/
#define FS_MAXENTRY		30


#define FS_FIRSTDATABLOCK		8192						// First block where data is stored. Bypass the 'fast write' area of the MMC card. Anything before FS_FIRSTDATABLOCK is for the root/fat
//#define LOG_UPDATEFS_SECTOR_INTERVAL	8				// This must be 2^n
#define LOG_UPDATEFS_SECTOR_INTERVAL	64				// This must be 2^n

/******************************************************************************
Filesystem data structures
******************************************************************************/
// Log entry
typedef struct {
	unsigned long int device;
	unsigned long int starttime;
	unsigned long int blockstart;
	unsigned long int blockend;
} FS_LOGENTRY;

// Filesytem
typedef struct {
	// Physical
	unsigned char phys_available;							// Indicates whether the physical device is available. i.e. a MMC card has been found and successfully initialized
	CID cid;														// MMC low-level stuff
	CSD csd;														// MMC low-level stuff

	unsigned long capacity_byte;							// Capacity of the MMC card in bytes
	unsigned long capacity_block;							// Capacity of the MMC card in blocks
	unsigned long blocknext;

	// Logical (filesystem)
	unsigned char fs_available;							// Indicates whether a formatted filesystem is found
	unsigned char root[512];								// Root entry - for convenience
	unsigned char numentry;									// Number of log entries found
	unsigned long firstfreeblock;							// First unused block

	// Logging
	// Used by the log_xxx functions
	unsigned char _log_streamopen;						// Is a write stream open?
	unsigned long int _log_lastusedsector;				// Last sector where data was stored
	unsigned long int _log_writeerror;					// Number of log write errors
	unsigned long int _log_writeok;						// Number of log write successes
	unsigned long int _log_deviceid;						// Device ID to store in log entry
	unsigned long int _log_timeoffset;					// Time offset to store in log entry
	unsigned long int _log_firstblock;					// First block used by the currently recorded log

} FS_STATE;

/******************************************************************************
	Variables
******************************************************************************/
extern FS_STATE fs_state;									// Filesystem state


/******************************************************************************
Filesystem functions
******************************************************************************/
unsigned char fs_init_physical(FS_STATE *fs_state);
unsigned char fs_init_logical(FS_STATE *fs_state);
void fs_checksum(unsigned char *buffer,unsigned short n,unsigned char *x1,unsigned char *x2);
char fs_format(void);
char fs_writeentry(FS_STATE *fs_state,unsigned char editentry,unsigned char newnumentry,unsigned long device,unsigned long starttime,unsigned long firstsector,unsigned long lastsector);
FS_LOGENTRY *fs_getlogentry(FS_STATE *fs_state,unsigned char n);

// Logging functions
unsigned char fs_log_start(FS_STATE *fs_state,unsigned long deviceid,unsigned long timeoffset,unsigned char *lognumber);
unsigned char fs_log_write(FS_STATE *fs_state,unsigned char *buffer, unsigned short n);
unsigned char fs_log_stop(FS_STATE *fs_state);




#endif

