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


#include <stdio.h>
#include <avr/pgmspace.h>

#include "main.h"
#include "fs.h"
#include "mmc.h"
#include "helper2.h"


//#define FSDBG

/******************************************************************************
	Variables
******************************************************************************/
FS_STATE fs_state;				// Filesystem state

/******************************************************************************
	fs_init
*******************************************************************************
	Low-level initialization of the storage.

	Initializes the MMC card, and places low-level card information in the
	FS_STATE structure

	Return value:
		0:			Success
		other:	Error
******************************************************************************/
unsigned char fs_init_physical(FS_STATE *fs_state)
{
	signed char rv;

	fs_state->phys_available = 0;
	fs_state->fs_available = 0;
	
	printf_P(PSTR("mmc_init...\r"));
	rv = mmc_init(&fs_state->cid,&fs_state->csd,&fs_state->capacity_byte);
	printf_P(PSTR("mmc_init returned: %d\r"),rv);
	if(rv!=0)
	{
		printf("MMC Failure (%d)\r",rv);
		return 1;
	}

	printf_P(PSTR("MMC card detected\r"));
	
	printf_P(PSTR("\rCID:\r"));
	printf_P(PSTR("\tMID: %X\r"),fs_state->cid.MID);
	printf_P(PSTR("\tOID: %X\r"),fs_state->cid.OID);
	printf_P(PSTR("\tProduct name: %s\r"),fs_state->cid.PNM);
	printf_P(PSTR("\tProduct revision: %X\r"),fs_state->cid.Rev);
	printf_P(PSTR("\tOID: %X\r"),fs_state->cid.PSN);
	printf_P(PSTR("\tMDT: %d/%d\r"),(fs_state->cid.MDT>>4),1997+(fs_state->cid.MDT&0xF));

	printf_P(PSTR("\nCSD:\r"));
	printf_P(PSTR("\tCSD: %X\r"),fs_state->csd.CSD);
	printf_P(PSTR("\tSPEC_VERS: %X\r"),fs_state->csd.SPEC_VERS);
	printf_P(PSTR("\tTAAC: %X\r"),fs_state->csd.TAAC);
	printf_P(PSTR("\tNSAC: %X\r"),fs_state->csd.NSAC);
	printf_P(PSTR("\tTRAN_SPEED: %X\r"),fs_state->csd.TRAN_SPEED);
	printf_P(PSTR("\tCCC: %X\r"),fs_state->csd.CCC);
	printf_P(PSTR("\tREAD_BL_LEN: %X\r"),fs_state->csd.READ_BL_LEN);
	printf_P(PSTR("\tREAD_BL_PARTIAL: %X\r"),fs_state->csd.READ_BL_PARTIAL);
	printf_P(PSTR("\tWRITE_BLK_MISALIGN: %X\r"),fs_state->csd.WRITE_BLK_MISALIGN);
	printf_P(PSTR("\tREAD_BLK_MISALIGN: %X\r"),fs_state->csd.READ_BLK_MISALIGN);
	printf_P(PSTR("\tDSR_IMP: %X\r"),fs_state->csd.DSR_IMP);
	printf_P(PSTR("\tC_SIZE: %X\r"),fs_state->csd.C_SIZE);
	printf_P(PSTR("\tVDD_R_CURR_MIN: %X\r"),fs_state->csd.VDD_R_CURR_MIN);
	printf_P(PSTR("\tVDD_R_CURR_MAX: %X\r"),fs_state->csd.VDD_R_CURR_MAX);
	printf_P(PSTR("\tVDD_W_CURR_MIN: %X\r"),fs_state->csd.VDD_W_CURR_MIN);
	printf_P(PSTR("\tVDD_W_CURR_MAX: %X\r"),fs_state->csd.VDD_W_CURR_MAX);
	printf_P(PSTR("\tC_SIZE_MULT: %X\r"),fs_state->csd.C_SIZE_MULT);
	printf_P(PSTR("\tERASE_GRP_SIZE: %X\r"),fs_state->csd.ERASE_GRP_SIZE);
	printf_P(PSTR("\tERASE_GRP_MULT: %X\r"),fs_state->csd.ERASE_GRP_MULT);
	printf_P(PSTR("\tWP_GRP_SIZE	: %X\r"),fs_state->csd.WP_GRP_SIZE);
	printf_P(PSTR("\tWP_GRP_ENABLE: %X\r"),fs_state->csd.WP_GRP_ENABLE);
	printf_P(PSTR("\tR2W_FACTOR: %X\r"),fs_state->csd.R2W_FACTOR);
	printf_P(PSTR("\tWRITE_BL_LEN: %X\r"),fs_state->csd.WRITE_BL_LEN);
	printf_P(PSTR("\tWRITE_BL_PARTIAL: %X\r"),fs_state->csd.WRITE_BL_PARTIAL);
	printf_P(PSTR("\tCONTENT_PROT_APP: %X\r"),fs_state->csd.CONTENT_PROT_APP);
	printf_P(PSTR("\tFILE_FORMAT_GRP: %X\r"),fs_state->csd.FILE_FORMAT_GRP);
	printf_P(PSTR("\tCOPY: %X\r"),fs_state->csd.COPY);
	printf_P(PSTR("\tPERM_WRITE_PROTECT: %X\r"),fs_state->csd.PERM_WRITE_PROTECT);
	printf_P(PSTR("\tTMP_WRITE_PROTECT: %X\r"),fs_state->csd.TMP_WRITE_PROTECT);
	printf_P(PSTR("\tFILE_FORMAT: %X\r"),fs_state->csd.FILE_FORMAT);
	printf_P(PSTR("\tECC: %X\r"),fs_state->csd.ECC);
	
	fs_state->capacity_block = fs_state->capacity_byte>>9;

	printf_P(PSTR("\rCapacity: %lu bytes. Maximum sector: %lu\r"),fs_state->capacity_byte,fs_state->capacity_block);

	// Understand the size of a write group and erase group

	// Size of erasable unit = (ERASE_GRP_SIZE + 1) * (ERASE_GRP_MULT + 1)
	// Read block len
	unsigned long eraseunit,readblen,writeblen;
	eraseunit = (fs_state->csd.ERASE_GRP_SIZE+1)*(fs_state->csd.ERASE_GRP_MULT+1);
	readblen = 1<<fs_state->csd.READ_BL_LEN;
	writeblen = 1<<fs_state->csd.WRITE_BL_LEN;
	printf_P(PSTR("Size of erasable unit: %lu blocks\r"),eraseunit);
	printf_P(PSTR("Maximum read block length: %lu bytes\r"),readblen);
	printf_P(PSTR("Maximum write block length: %lu bytes\r"),writeblen);

	fs_state->phys_available = 1;
	return 0;
}


/******************************************************************************
	fs_init_logical
*******************************************************************************
	Initialization of the log filesystem.

	Reads and process the root entry

	Returns:
		0					Success
		other				Error
******************************************************************************/
unsigned char fs_init_logical(FS_STATE *fs_state)
{
	unsigned char rv;
	unsigned char xsum1,xsum2;
	FS_LOGENTRY *logentry;
	unsigned char *buffer;

	fs_state->fs_available=0;

	if(!fs_state->phys_available)
		return 1;


	buffer = fs_state->root;
	fs_state->numentry=0;
	fs_state->firstfreeblock=0;

	rv = mmc_read_block(0,buffer);
	if(rv!=0)
	{
		printf_P(PSTR("Root sector error: reading error\n"));
		return 2;
	}

	// Checking the filesystem
	if(buffer[0]!='R' || buffer[1]!='O' || buffer[2]!='O' || buffer[3]!='T')
	{
		printf_P(PSTR("Root sector error: start marker\n"));
		return 3;
	}
	rv=0;
	for(unsigned char i=4;i<13;i++)
		if(buffer[i]!=0)
			rv=1;
	if(rv)
	{
		printf_P(PSTR("Root sector error: flags\n"));
		return 4;
	}

	data_checksum(buffer,510,&xsum1,&xsum2);

	
	if(xsum1!=buffer[510] || xsum2!=buffer[511])
	{
		printf_P(PSTR("Root sector error: invalid checksum  %X %X %X %X\n"),xsum1,xsum2,buffer[510],buffer[511]);
		return 6;
	}
	
	fs_state->numentry=buffer[13];
	if(fs_state->numentry>31)
	{
		printf_P(PSTR("Root sector error: invalid entry number\n"));
		return 5;
	}

	fs_state->fs_available=1;

	printf_P(PSTR("Number of log entries: %d\n"),fs_state->numentry);
	for(unsigned short i=0;i<fs_state->numentry;i++)
	{
		logentry = fs_getlogentry(fs_state,i);
		printf_P(PSTR("Log %d\t Device %lX\t StartTime %lu\t StartBlock %lu\t EndBlock %lu\n"),i,logentry->device,logentry->starttime,logentry->blockstart,logentry->blockend);
	}
	if(fs_state->numentry==0)
		fs_state->firstfreeblock=FS_FIRSTDATABLOCK;
	else
	{
		logentry = (FS_LOGENTRY*)(buffer + 14 + (fs_state->numentry-1)*16);
		fs_state->firstfreeblock=logentry->blockend+1;
	}

	

	return 0;
}
/******************************************************************************
	fs_getlogentry
*******************************************************************************
	Returns the specified log entry.

	Returns:
		pointer to the log entry
		0			Error (unavailable log entry)
******************************************************************************/
FS_LOGENTRY *fs_getlogentry(FS_STATE *fs_state,unsigned char n)
{
	// Filesystem not initialized
	if(!fs_state->fs_available)
	{
		return 0;
	}

	// Requested log entry beyond those available
	if(n>=fs_state->numentry)
	{
		return 0;	
	}
	
	return (FS_LOGENTRY*)(fs_state->root + 14 + n*16);
}




/******************************************************************************
	fs_format
*******************************************************************************
	Formats the custom logging file system
	
	
	Returns:
		0			Success
		other		Failure
******************************************************************************/
char fs_format(void)
{
	unsigned char sec[14]={'R','O','O','T',			// 4 ROOT sector
								0x00,0x00,0x00,0x00,			// 4 version
								0,0,0,0,0,						// 5 reserved
								0};								// 1 number of log files
	unsigned char zero=0;
	unsigned char xsum1,xsum2;
	unsigned char rv;
	unsigned long written;
	unsigned char blockcompleted;

	
	data_checksum(sec,14,&xsum1,&xsum2);
	

	// Root sector
	printf_P(PSTR("Writing root sector\n"));
	mmc_write_stream_open(0);
	mmc_write_stream_write_partial(sec,14,&written,&blockcompleted,0);
	for(unsigned short i=0;i<496;i++)
		mmc_write_stream_write_partial(&zero,1,&written,&blockcompleted,0);
	mmc_write_stream_write_partial(&xsum1,1,&written,&blockcompleted,0);
	mmc_write_stream_write_partial(&xsum2,1,&written,&blockcompleted,0);
	rv = mmc_write_stream_close(0x00,0);
	
	if(rv!=0)
	{
		printf_P(PSTR("Error finalizing write: %d\n"),rv);
		return 1;
	}

	// Reset the state mirroring the root.
	//fs_state->numentry=0;
	//fs_state->firstfreeblock=500;


	return 0;						
}




/******************************************************************************
	fs_writeentry
*******************************************************************************
	
	Writes the root with an updated entry.
	This can be used both to add a new entry, edit any entry, or erase the last entry:
		Add an entry by setting newnumentry to fs_state->numentry+1
		Erase the last entry by setting newnumentry to fs_state->numentry-1

	This function does not update the root memory structure if the write of the root on the MMC failed to keep consistency.

	Input:
		fs_state			Initialized file system
		editentry		Entry to edit
		newnumentry		Number of entries 
		device			Device number
		starttime		Start time 
		firstsector		First used sector
		lastsector		Last used sector

	Return:
		0					Success
		other				Failure
******************************************************************************/
char fs_writeentry(FS_STATE *fs_state,unsigned char editentry,unsigned char newnumentry,unsigned long device,unsigned long starttime,unsigned long firstsector,unsigned long lastsector)
{
	char rv;
	unsigned char xsum1,xsum2;
	FS_LOGENTRY *logentry;
	unsigned char *root;
	// Old state
	FS_LOGENTRY old_logentry;					// old log entry
	unsigned char old_xsum1,old_xsum2;		// old checksum
	unsigned char old_numentry;				// old number of entries
	
	#ifdef FSDBG
		printf_P(PSTR("fs_writeentry editentry %d newnumentry %d first/last sector %ld %ld\r"),editentry,newnumentry,firstsector,lastsector);
	#endif

	// No filesystem -> fail.
	if(!fs_state->fs_available)
		return 1;

	root = fs_state->root;

	// Backup the old values
	old_xsum1 = root[510];
	old_xsum2 = root[511];
	old_numentry = root[13];
	old_logentry = *(FS_LOGENTRY*)(root + 14 + editentry*16);


	// Set the new values

	// Set the number of logs
	fs_state->numentry = newnumentry;
	root[13] = newnumentry;

	// Select the entry
	logentry = (FS_LOGENTRY*)(root + 14 + editentry*16);

	// Set the values
	logentry->device=device;
	logentry->starttime=starttime;
	logentry->blockstart=firstsector;
	logentry->blockend=lastsector;
	
	// Update the checksum
	data_checksum(root,510,&xsum1,&xsum2);
	root[510] = xsum1;
	root[511] = xsum2;

	// Write root
	rv = mmc_write_block(0,root);

	
	if(rv)
	{
		// Failure: revert the root memory values to the old values
		root[510] = old_xsum1;
		root[511] = old_xsum2;
		root[13] = old_numentry;
		*(FS_LOGENTRY*)(root + 14 + editentry*16) = old_logentry;
		return rv;
	}

	return 0;
}


/******************************************************************************
	fs_log_start
*******************************************************************************
	Add a new log at the end of the file system.
	Starts logging at the first free block.

	In case of error 1: The application should abort or make space for new logs.
	In case of error 2: The start of the log failed, no new log entry was stored in the root. 
							  The application call fs_log_start a few more times.
							  If the problem is not transient, the application must give up starting a log as future changes to the root will likely also fail.
	

	Return value:
		0:			Success
		1:			No more root entries
		2:			Error writing the root entry
		other:	Error
******************************************************************************/
unsigned char fs_log_start(FS_STATE *fs_state,unsigned long deviceid,unsigned long timeoffset,unsigned char *lognumber)
{
	// Test if there is space to add an entry log...
	if(fs_state->numentry>=FS_MAXENTRY)
		return 1;												// No space

	// State variables
	fs_state->_log_lastusedsector=fs_state->firstfreeblock-1;		// Last used sector: one less than the first free block during initialization
	fs_state->_log_firstblock = fs_state->firstfreeblock;				// First used sector
	fs_state->_log_streamopen=0;												// Logging possible (i.e. sector open)
	fs_state->_log_writeerror=0;												// No write errors
	fs_state->_log_writeok=0;
	fs_state->_log_deviceid = deviceid;
	fs_state->_log_timeoffset = timeoffset;
	

	// Add a new entry to the root
	//printf_P(PSTR("log start write root\r"));
	unsigned char rv = fs_writeentry(fs_state,fs_state->numentry,fs_state->numentry+1,fs_state->_log_deviceid,fs_state->_log_timeoffset,fs_state->_log_firstblock,fs_state->_log_lastusedsector);

	// Update of the root failed.
	if(rv)
		return 2;
	
	*lognumber = fs_state->numentry;
	return 0;
}
	

/******************************************************************************
	fs_log_write
*******************************************************************************
	Writes n buffer bytes to the log.

	This function periodically updates the root entry with the number of sectors used so far.

	The root entry is updated when the partial write indicates a complete block is written, and when enough such blocks were written.
	In this case, the stream write is closed, the root updated, and then the stream write is opened anew.

	Error handling:

		The function does not attempt to repeat operations upon errors. 
		It prefers cancelling the operation, as data loss are not critical when using packet storage, and the underlying system is streaming store optimized

		When fs_log_write fails a consecutive number of times, e.g. in 10 calls it fails 10 times, then application should decide 
		to terminate the recording as there may be a fatal error.


	Return value:
		0:			Success
		other:	Error
******************************************************************************/
unsigned char fs_log_write(FS_STATE *fs_state,unsigned char *buffer, unsigned short n)
{
	unsigned char rv;
	unsigned long written;
	unsigned char blockcompleted;
	unsigned long currentaddr;
	unsigned char noerr;

	#ifdef FSDBG
		printf_P(PSTR("log: write %d\n"),n);
	#endif

	noerr=1;

	// Write n bytes
	while(n)
	{
		#ifdef FSDBG
		//	printf_P(PSTR("log: write left: %d\n"),n);
		#endif

		// Check if we need to open a new sector
		if(!fs_state->_log_streamopen)
		{
			#ifdef FSDBG
				printf_P(PSTR("log: open %d\n"),fs_state->_log_lastusedsector+1);
			#endif

			mmc_write_streammulti_open((fs_state->_log_lastusedsector+1)<<9);

			fs_state->_log_streamopen=1;										// Stream write is open
		}

		// Stream is open - let's write
		rv = mmc_write_streammulti_write_partial(buffer,n,&written,&blockcompleted,&currentaddr);
		
		// Write error
		if(rv!=0)
		{
			#ifdef FSDBG	
				printf_P(PSTR("mmc_write_streammulti_write error\r"));
			#endif

			// In case of error: 
			//		Log the error (_log_writeerror)
			//		Abort the rest of the logging. Streaming write functions do not support retries (optimized for fast streaming write)
			// log_write can called with the next data payload and the streaming write will be initialized again

			fs_state->_log_writeerror++;									// Error count increases

			return 1;
		}

		#ifdef FSDBG
			printf_P(PSTR("Wrote %ld of %d bytes (block completed: %d).\r"),written,n,blockcompleted);
		#endif

		// No error. Handle block completion and the remaining of the data transfer.
		
		// Update the counters and pointers
		n-=written;											
		buffer+=written;

		// Keep the last used sector
		fs_state->_log_lastusedsector = (currentaddr>>9);			// last used sector

		// A block has been completed - check if root must be updated
		if(blockcompleted)
		{
			// Update the root entry periodically
			// The number of blocks so far written is found with fs_state->firstfreeblock and currentaddr
			#ifdef FSDBG
				printf_P(PSTR("last: %ld first: %ld  (%d %d)\r"),fs_state->_log_lastusedsector,fs_state->firstfreeblock,LOG_UPDATEFS_SECTOR_INTERVAL,((fs_state->_log_lastusedsector-fs_state->firstfreeblock+1)&(LOG_UPDATEFS_SECTOR_INTERVAL-1)));
			#endif

			if ( ((fs_state->_log_lastusedsector-fs_state->firstfreeblock+1)&(LOG_UPDATEFS_SECTOR_INTERVAL-1))==0 )
			{
				// Update the root entry....

				// Terminate the stream write (the currentaddr does not change even with padding)
				rv = mmc_write_streammulti_close(0,0);
				if(rv!=0)
				{
					#ifdef FSDBG	
						printf_P(PSTR("mmc_write_streammulti_close error\r"));
					#endif

					fs_state->_log_writeerror++;													// Error count increases					
					noerr=0;
				}
				// Mark stream as closed
				fs_state->_log_streamopen=0;
							
				// Update the root
				#ifdef FSDBG
					printf_P(PSTR("Update root. Entry %d spans %ld - %ld\r"),fs_state->numentry-1,fs_state->firstfreeblock,fs_state->_log_lastusedsector);
				#endif
				rv = fs_writeentry(fs_state,fs_state->numentry-1,fs_state->numentry,fs_state->_log_deviceid,fs_state->_log_timeoffset,fs_state->_log_firstblock,fs_state->_log_lastusedsector);

				// Update the first free block
				fs_state->firstfreeblock = fs_state->_log_lastusedsector+1;


				if(rv!=0)
				{
					#ifdef FSDBG	
						printf_P(PSTR("fs_editentry error\r"));
					#endif

					fs_state->_log_writeerror++;													// Error count increases					
					noerr=0;
				}
			}
		}
		// Must check the capacity: if we reach the limit, we must abort
	}
	if(noerr==1)
		fs_state->_log_writeok++;
	return 0;
}
/******************************************************************************
	fs_log_stop
*******************************************************************************
	Return value:
		0:			Success
		other:	Error
******************************************************************************/
unsigned char fs_log_stop(FS_STATE *fs_state)
{
	unsigned char zero=0x5a;
	unsigned char rv;
	unsigned long currentaddr;

	if(fs_state->_log_streamopen)
	{
		rv = mmc_write_streammulti_close(zero,&currentaddr);
		if(rv!=0)
		{
			printf_P(PSTR("mmc_write_streammulti_close error\r"));
			fs_state->_log_writeerror++;										// Error count increases
		}
		fs_state->_log_streamopen=0;

		fs_state->_log_lastusedsector = (currentaddr>>9);			// last used sector
	}

	#ifdef FSDBG
		printf_P(PSTR("Update root. Entry %d spans %ld - %ld\r"),fs_state->numentry-1,fs_state->firstfreeblock,fs_state->_log_lastusedsector);
	#endif

	//printf_P(PSTR("log stop write root\r"));
	// If the sector was closed we should return the fs_state->_log_sectorwritten-1 as the closing in the write function increments the counter
	rv = fs_writeentry(fs_state,fs_state->numentry-1,fs_state->numentry,fs_state->_log_deviceid,fs_state->_log_timeoffset,fs_state->_log_firstblock,fs_state->_log_lastusedsector);

	// Update the first free block
	fs_state->firstfreeblock = fs_state->_log_lastusedsector+1;

	return 0;
}


