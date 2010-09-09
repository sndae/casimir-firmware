/*
   MMC driver
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
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include "wait.h"
#include "mmc.h"




/******************************************************************************
*******************************************************************************
	All user MMC commands use the following convention:
		Return valus:	0			success
							other		error code
*******************************************************************************
******************************************************************************/

//#define MMCDBG

/******************************************************************************
*******************************************************************************
*******************************************************************************
INTERNALS   INTERNALS   INTERNALS   INTERNALS   INTERNALS   INTERNALS   
*******************************************************************************
*******************************************************************************
******************************************************************************/





unsigned long bit2val(unsigned char *data,unsigned int startbit,unsigned int n)
{
	unsigned long v,bit;
	unsigned int byteptr;
	unsigned int bitptr;

	v=0;

	byteptr = startbit>>3;
	bitptr = 7-(startbit&0x07);

	for(unsigned int i=0;i<n;i++)
	{
		bit = (data[byteptr]&(1<<bitptr))?1:0;    // read the bit from the input stream
      	v = (v<<1)+bit;
		bitptr = (bitptr-1)&0x07;
		if(bitptr==0x07)
			byteptr++;
	}

	return v;

}


char SPI_Transmit(unsigned char cData)		//send charakter over SPI char/ also used for reading!!!!
{
	SPDR = cData;
	while(!(SPSR & (1<<SPIF))); 				//the SPIF Bit ist set when transfer complete...
	//printf_P(PSTR("%X\r"),SPDR);
	return SPDR;
}
void testfewmore(void)
{
	for(unsigned i=0;i<10;i++)
	{
		_delay_ms(10);
		SPDR = 0xFF;
		while(!(SPSR & (1<<SPIF))); 				//the SPIF Bit ist set when transfer complete...
		printf_P(PSTR("   %X\r"),SPDR);
	}
}

/**
	MMC_iCommand
		
		Sends a low-level command to the MMC card. 
		Waits until the card answers, or until a timeout occurs.

		Returns:
			Nonzero		Failure
			Zero:			Success
**/
unsigned char MMC_iCommand(unsigned char cmd,unsigned char p1,unsigned char p2,unsigned char p3,unsigned char p4, unsigned char crc)
{

	return MMC_iCommandResponse(cmd,p1,p2,p3,p4,crc,0,0);
}

unsigned char MMC_iCommandResponse(unsigned char cmd,unsigned char p1,unsigned char p2,unsigned char p3,unsigned char p4, unsigned char crc,unsigned char *response,unsigned short n)
{
	unsigned char r1,r2;
	unsigned long int t1;

	// Send the command
	SPI_Transmit(cmd|0x40);
	SPI_Transmit(p1);
	SPI_Transmit(p2);
	SPI_Transmit(p3);
	SPI_Transmit(p4);				// Always 0
	SPI_Transmit(crc);			// Checksum, only used for command GO_IDLE_STATE

	// Wait until an answer is received, or a timeout occurs
	#ifdef MMCDBG
		printf_P(PSTR("MMC_iCommandResponse\r"));
	#endif
	t1 = timer_ms_get();
	do
	{
		r1 = SPI_Transmit(0xFF);
		#ifdef MMCDBG
			printf_P(PSTR("  cmdresp %X\r"),r1);
		#endif
	}
	//while(r1 == 0xFF && (timer_ms_get()-t1<MMC_TIMEOUT_ICOMMAND));
	while( (r1&0x80) && (timer_ms_get()-t1<MMC_TIMEOUT_ICOMMAND));

	
	if(r1&0x80)			// Timeout ocurrd: return 
	{
		#ifdef MMCDBG
			printf_P(PSTR("MMC_iCommandResponse: returning early\r"));
		#endif
		return r1;			// Returns nonzero indicating a failure.
	}

	// Read the remainder response
	for(unsigned int i=0;i<n;i++)
	{
		response[i] = SPI_Transmit(0xFF);
	}

	// Trailing byte requirement
	r2 = SPI_Transmit(0xFF);
	//printf_P(PSTR("   cmdresp trailing %X\n"),r2);
	
	#ifdef MMCDBG
		printf_P(PSTR("MMC_iCommandResponse: returning ok: %02X\r"),r1);
	#endif
	return r1; 		// Success
}

void MMC_SelectN(char ss)
{
	ss=ss?1:0;
	PORTB=(PORTB&0xEF)|(ss<<4);

}

unsigned char MMC_Command(unsigned char cmd,unsigned char p1,unsigned char p2,unsigned char p3,unsigned char p4, unsigned char crc)
{
	return MMC_CommandResponse(cmd,p1,p2,p3,p4,crc,0,0);
}
unsigned char MMC_CommandResponse(unsigned char cmd,unsigned char p1,unsigned char p2,unsigned char p3,unsigned char p4, unsigned char crc,unsigned char *response,unsigned short n)
{
	unsigned char r;
	MMC_SelectN(0);			//MMC Card is selected takes 0 as value!!
	//_delay_ms(1);
	r = MMC_iCommandResponse(cmd,p1,p2,p3,p4,crc,response,n);
	//_delay_ms(1);
	MMC_SelectN(1);
	return r;
}

/**
	MMC_CommandRead
		Sends any read-like command, and reads	the data block answer of the card.
		Implements read timeout.

		Returns:
			0		Success
			other	Failure
**/

unsigned char MMC_CommandRead(unsigned char cmd,unsigned char p1,unsigned char p2,unsigned char p3,unsigned char p4, unsigned char crc,unsigned char *buffer,unsigned short n)
{
	unsigned char r1;
	unsigned short i;
	unsigned long int t1;
	
	MMC_SelectN(0);				// Select the card
	
	r1=MMC_iCommand(cmd,p1,p2,p3,p4,crc);
	//printf_P(PSTR("MMC_CommandRead return from icommand. %X\r"),r1);
		
	t1 = timer_ms_get();
	do
	{
		r1 = SPI_Transmit(0xFF);
		//printf_P(PSTR("MMC_CommanRead wait start: %X\r"),r1);
	}
	while(r1!= MMC_STARTBLOCK && (timer_ms_get()-t1<MMC_TIMEOUT_READWRITE));
	if(r1 != MMC_STARTBLOCK)
	{
		//printf_P(PSTR("Failre before start block\r"));
		MMC_SelectN(1);			// Deselect the card
		return 1;					// Return, indicating a timeout waiting for MMC_STARTBLOCK
	}
	
	// read in data
	for(i=0; i<n; i++)
	{
		*buffer++ = SPI_Transmit(0xFF);
	}
	
	
	// Read 16-bit CRC
	SPI_Transmit(0xFF);
	SPI_Transmit(0xFF);

	// Trailing stuff
	SPI_Transmit(0xFF);	
	

	MMC_SelectN(1);
	return 0;	
}


/******************************************************************************
*******************************************************************************
INITIALIZATION   INITIALIZATION   INITIALIZATION   INITIALIZATION   
*******************************************************************************
******************************************************************************/


/*
	mmc_init
		Initializes the card, switch it to SPI mode, do a software reset (GO_IDLE_STATE)
	
	If successful, the function stores the mmc card informations at the location pointed by the non-null pointer parameters.



	Returns:
		0:	Success
		other:	Failure
*/
unsigned char mmc_init(CID *cid,CSD *csd,unsigned long *capacity)
{
	unsigned char c;
	int i;
	unsigned char retry;
	unsigned char response[32];
	

	// Native initialization
	MMC_SelectN(0);
	_delay_ms(100);
	for(i=0; i < 16; i++) SPI_Transmit(0xFF); // send 10*8=80 clock pulses
	MMC_SelectN(1);		// Make sure we are in SDC mode
	_delay_ms(100);

	for(i=0; i < 2; i++) SPI_Transmit(0xFF); // ? 2 bytes

	_delay_ms(100);



	retry = 0;
	printf_P(PSTR("MMC_GO_IDLE_STATE\n"));
	do
	{
		_delay_ms(100);
		c=MMC_Command(MMC_GO_IDLE_STATE,0,0,0,0,0x95);
		printf_P(PSTR("Result of MMC_GO_IDLE_STATE %X\r\n"),c);
		// do retry counter
	} while( c!=1 && retry++<MMC_RETRY);
	if(c!=1)
		return 1;
	
	//testfewmore();

	printf_P(PSTR("MMC_SEND_OP_COND\n"));
	retry=0;
	do
	{
		_delay_ms(100);
		c=MMC_CommandResponse(MMC_SEND_OP_COND,0,0,0,0,0x95,response,4);				// Response R3
		printf_P(PSTR("Result of MMC_SEND_OP_COND %X\r\n"),c);
		// do retry counter
	} while(c!=0 && retry++<MMC_RETRY);
	if(c!=0)
		return 2;

	printf_P(PSTR("OP_COND OCR: %X %X %X %X\r"),response[0],response[1],response[2],response[3]);
	




	/*printf_P(PSTR("CMD 55\n"));
	retry=0;
	do
	{
		_delay_ms(100);
		c=MMC_Command(MMC_CMD55,0,0,0,0,0x95);
		printf_P(PSTR("Result of CMD 55 %X\r\n"),c);
		// do retry counter
	} while( (c&0xFE)!=0x00 && c!=1 && retry++<MMC_RETRY);
	//if(c!=0 && c!=1)
		//return -2;
	if((c&0xFE)!=0x00)
		return -2;

	testfewmore();

	// Try acmd
	printf_P(PSTR("ACMD 41\n"));
	retry=0;
	do
	{
		_delay_ms(100);
		c=MMC_Command(MMC_ACMD41,0,0,0,0,0x95);
		printf_P(PSTR("Result of ACMD 41 %X\r\n"),c);
		// do retry counter
	//} while(c!=0 && c!=1 && retry++<MMC_RETRY);
	} while( (c&0xFE)!=0x00 && c!=1 && retry++<MMC_RETRY);
	//if(c!=0 && c!=1)
	//	return -2;
	if((c&0xFE)!=0x00)
		return -2;

	testfewmore();*/
	
	
	// Try cmd55/acmd
	
	/*printf_P(PSTR("CMD55/ACMD 41\n"));
	retry=0;
	do
	{
		_delay_ms(100);
		c=MMC_Command(MMC_CMD55,0,0,0,0,0x95);
		printf_P(PSTR("Result of CMD 55 %X\r\n"),c);
		//testfewmore();
		_delay_ms(100);
		c=MMC_Command(MMC_ACMD41,0,0,0,0,0x95);
		printf_P(PSTR("Result of ACMD 41 %X\r\n"),c);
		//testfewmore();
		// do retry counter
	//} while(c!=0 && c!=1 && retry++<MMC_RETRY);
	//} while( (c&0xFE)!=0x00 && c!=1 && retry++<MMC_RETRY);
	} while( c!=0x00  && retry++<MMC_RETRY);
	//if(c!=0 && c!=1)
	//	return -2;
	if((c&0xFE)!=0x00)
		return -2;*/

	



	// Try read_ocr
	printf_P(PSTR("MMC_READ_OCR\n"));
	retry=0;
	do
	{
		_delay_ms(100);
		c=MMC_CommandResponse(MMC_READ_OCR,0,0,0,0,0x95,response,4);					// Response R3
		printf_P(PSTR("Result of MMC_READ_OCR %X\r\n"),c);
		// do retry counter
	} while(c!=0 && retry++<MMC_RETRY);
	if(c!=0)
		return 3;

	printf_P(PSTR("OCR: %X %X %X %X\r"),response[0],response[1],response[2],response[3]);
	printf_P(PSTR("OCR status: %X\r"),bit2val(response,0,1));
	printf_P(PSTR("OCR reserved: %X\r"),bit2val(response,1,7));
	printf_P(PSTR("OCR 2.7-3.6V: %X\r"),bit2val(response,8,9));
	printf_P(PSTR("OCR 2.0-2.6V: %X\r"),bit2val(response,17,7));
	printf_P(PSTR("OCR 1.65-1.95V: %X\r"),bit2val(response,24,1));
	printf_P(PSTR("OCR reserved: %X\r"),bit2val(response,25,7));
	
	

	// Set block length to 512 bytes
	printf_P(PSTR("MMC_SET_BLOCKLEN\n"));
	c=MMC_Command(MMC_SET_BLOCKLEN,0,0,0x02,0x00,0x95);	
	printf_P(PSTR("Result of MMC_SET_BLOCKLEN: %X\r\n"),c);
	if(c!=0)
		return 4;

// Do the same with: MMC_SEND_CSD
	
	printf_P(PSTR("MMC_SEND_CSD\n"));
	c=MMC_CommandRead(MMC_SEND_CSD,0,0,0,0,0x95,response,30);
	printf_P(PSTR("Result of MMC_SEND_CSD: %X\r\n"),c);
	if(c!=0)
		return 5;

	
	if(csd)
	{
		csd->CSD						= bit2val(response,0,2);				// [127:126]
		csd->SPEC_VERS				= bit2val(response,2,4);				// [125:122]
		csd->TAAC					= bit2val(response,8,8);				// [119:112]
		csd->NSAC					= bit2val(response,16,8);				// [111:104]
		csd->TRAN_SPEED			= bit2val(response,24,8);				// [103:96]
		csd->CCC						= bit2val(response,32,12);				// [95:84]
		csd->READ_BL_LEN			= bit2val(response,44,4);				// [83:80]
		csd->READ_BL_PARTIAL		= bit2val(response,48,1);				// [79:79]
		csd->WRITE_BLK_MISALIGN	= bit2val(response,49,1);				// [78:78]
		csd->READ_BLK_MISALIGN	= bit2val(response,50,1);				// [77:77]
		csd->DSR_IMP				= bit2val(response,51,1);				// [76:76]
		csd->C_SIZE					= bit2val(response,54,12);				// [73:62]
		csd->VDD_R_CURR_MIN		= bit2val(response,66,3);				// [61:59]
		csd->VDD_R_CURR_MAX		= bit2val(response,69,3);				// [58:56]
		csd->VDD_W_CURR_MIN		= bit2val(response,72,3);				// [55:53]
		csd->VDD_W_CURR_MAX		= bit2val(response,75,3);				// [52:50]
		csd->C_SIZE_MULT			= bit2val(response,78,3);				// [49:47]
		csd->ERASE_GRP_SIZE		= bit2val(response,81,5);				// [46:42]
		csd->ERASE_GRP_MULT		= bit2val(response,86,5);				// [41:37]
		csd->WP_GRP_SIZE			= bit2val(response,91,5);				// [36:32]
		csd->WP_GRP_ENABLE		= bit2val(response,96,1);				// [31:31]
		csd->R2W_FACTOR			= bit2val(response,99,3);				// [28:26]
		csd->WRITE_BL_LEN			= bit2val(response,102,4);				// [25:22]
		csd->WRITE_BL_PARTIAL	= bit2val(response,106,1);				// [21:21]
		csd->CONTENT_PROT_APP	= bit2val(response,111,1);				// [16:16]
		csd->FILE_FORMAT_GRP		= bit2val(response,112,1);				// [15:15]
		csd->COPY					= bit2val(response,113,1);				// [14:14]
		csd->PERM_WRITE_PROTECT	= bit2val(response,114,1);				// [13:13]
		csd->TMP_WRITE_PROTECT	= bit2val(response,115,1);				// [12:12]
		csd->FILE_FORMAT			= bit2val(response,116,2);				// [11:10]
		csd->ECC						= bit2val(response,118,2);				// [9:8]
	}
	if(capacity)
	{
		*capacity = (bit2val(response,54,12)+1) * (1<<(bit2val(response,78,3)+2)) * (1<<bit2val(response,44,4));
	}


	//MMC_SEND_CID
	printf_P(PSTR("MMC_SEND_CID\n"));
	c=MMC_CommandRead(MMC_SEND_CID,0,0,0,0,0x95,response,30);
	printf_P(PSTR("Result of MMC_SEND_CID: %X\r\n"),c);
	if(c!=0)
		return 6;

	if(cid)
	{
		cid->MID=bit2val(response,0,8);
		cid->OID=bit2val(response,8,16);
		for(unsigned char i=0;i<6;i++)
			cid->PNM[i]=response[3+i];
		cid->PNM[6]=0;
		cid->Rev=bit2val(response,72,8);
		cid->PSN=bit2val(response,80,32);
		cid->MDT=bit2val(response,112,8);
	}

	
	return 0;
}








/******************************************************************************
*******************************************************************************
BLOCK READ/WRITE   BLOCK READ/WRITE   BLOCK READ/WRITE   BLOCK READ/WRITE   
*******************************************************************************
******************************************************************************/


/******************************************************************************
	_mmc_write_block_open
*******************************************************************************
	Internal use.
	Start a write command at the specified address.

	Return value:
		0:			Start of write ok
		other:	error
******************************************************************************/
unsigned char _mmc_write_block_open(unsigned long addr)
{
	unsigned char response;

	MMC_SelectN(0);				//	Select card
	response=MMC_iCommand(MMC_WRITE_BLOCK,addr>>24,addr>>16,addr>>8,addr,0x95);
	
	if (response!=0)				// Command failed
	{
		MMC_SelectN(1);			// Deselect card

		#ifdef MMCDBG
			printf_P(PSTR("_mmc_write_block_open fail %d\r"),response);
		#endif
		return response;				
	}
	#ifdef MMCDBG
		printf_P(PSTR("_mmc_write_block_open ok %d\r"),response);
	#endif

	SPI_Transmit(MMC_STARTBLOCK);			// Send Data Token

	return 0;
}

/******************************************************************************
	_mmc_write_block_stop
*******************************************************************************
	Internal use.
	
	Call after 512 bytes are written to complete the block with the checksum.
	Reads back the error codes.
	For block and multiblock writes.
			
	Return value:
		0:			ok
		other:	error
******************************************************************************/
unsigned char _mmc_write_block_stop(void)
{
	unsigned char response1,response2,dataresponse,response3,response;
	unsigned long int t1;
	// Send CRC
	response1 = SPI_Transmit(0xFF);
	response2 = SPI_Transmit(0xFF);

	
	// Upon sending the last byte (2nd CRC byte), the card:
	// 	- Immediately reads as 'data response' and indicates if the data is accepted: xxx00101
	// 	- Then reads as 0, aka not-idle
	// 	- Once write is completed reads as 1, aka idle
	// 	- Then reads as FF, aka transaction completed
	
	dataresponse = SPI_Transmit(0xFF);				// This should be the data response
	response3 = SPI_Transmit(0xFF);					// This should be 'non idle', not needed as we loop until idle

	#ifdef MMCDBG
		printf_P(PSTR("_mmc_write_block_stop: r1,r2,dr,d3: %X %X %X %X\r"),response1,response2,dataresponse,response3);
	#endif

	// Data response must be XXX00101 for data accepted
	if((dataresponse&0x1F)!=0x05)						// Failure
	{
		return 1;											// Return error
	}
	
	//printf_P(PSTR("end block response: %02X %02X %02X\r"),response1,response2,dataresponse);

	// Wait until ready for next block
	t1 = timer_ms_get();
	do
	{
		response = SPI_Transmit(0xFF);
		//printf_P(PSTR(" %X\r"),response);
	}
	while(response!=0xFF && (timer_ms_get()-t1<MMC_TIMEOUT_READWRITE));

	#ifdef MMCDBG
		printf_P(PSTR("_mmc_write_block_stop: success: %d\r"),response3==0xff?1:0);
	#endif

	if(response!=0xFF)									// Failure
	{
		return 2;											// Return error
	}
	return 0;
}


/******************************************************************************
	_mmc_write_block_writebuffer
*******************************************************************************
	Internal use.
	Writes size buffer bytes.

	It is the responsability of the caller to ensure that the number of bytes written 
	is equal to a complete block before calling _mmc_write_block_close. 
******************************************************************************/
void _mmc_write_block_writebuffer(unsigned char *buffer,unsigned short size)
{
	// Send data
	for(unsigned short k=0;k<size;k++)						
	{															
		SPI_Transmit(buffer[k]);
	}
}
/******************************************************************************
	_mmc_write_block_writeconst
*******************************************************************************
	Internal use.
	Writes size times byte b.

	It is the responsability of the caller to ensure that the number of bytes written 
	is equal to a complete block before calling _mmc_write_block_close. 
******************************************************************************/
void _mmc_write_block_writeconst(unsigned char b,unsigned short size)
{
	// Send data
	for(unsigned short k=0;k<size;k++)						
	{															
		SPI_Transmit(b);
	}
}
/******************************************************************************
	_mmc_write_block_close
*******************************************************************************
	Internal use.
	Finishes the writing transaction.

	It is the responsability of the application to call this function after one block of data has been provided.

	Return value:
		0:			End of write ok
		other:	error
******************************************************************************/
unsigned char _mmc_write_block_close(void)
{
	unsigned char rv;

	#ifdef MMCDBG
		printf_P(PSTR("_mmc_write_block_close\r"));
	#endif

	rv = _mmc_write_block_stop();

	// Deselect the card
	MMC_SelectN(1);

	return rv;					
}


/******************************************************************************
	mmc_write_block
*******************************************************************************
	Writes a block of data at byte address addr.
	The block size is fixed at 512 bytes.
	Uses internally the single block write function.
	
	Return value:
		0:			success
		nonzero:	error
******************************************************************************/
unsigned char mmc_write_block(unsigned long addr,unsigned char *buffer)			// shall be eliminated
{
	unsigned char response;

	response=_mmc_write_block_open(addr);

	#ifdef MMCDBG
		printf_P(PSTR("mmc_write_block, open returned: %X\r"),response);
	#endif

	if(response!=0)
	{
		printf_P(PSTR("mmc_write_block, failing\r"));
		return response;
	}
	

	// This operation never fails: it's SPI transmit only.
	_mmc_write_block_writebuffer(buffer,512);
	

	response = _mmc_write_block_close();

	#ifdef MMCDBG
		printf_P(PSTR("_mmc_write_block_close, returned %X\r"),response);
	#endif

	return response;
}

/******************************************************************************
	mmc_read_block
******************************************************************************/
unsigned char mmc_read_block(unsigned long addr,unsigned char *buffer)
{
	unsigned char response = MMC_CommandRead(MMC_READ_SINGLE_BLOCK,
							addr>>24,addr>>16,addr>>8,addr,
							0x95,
							buffer,
							512);
	return response;
}

unsigned char mmc_write_multipleblock(unsigned long addr,unsigned char *buffer,unsigned short repetition)
{
	unsigned char response1,response2,response3,response;
	unsigned char dataresponse;
	unsigned long int t1,t2;
	unsigned long int tt1,tt2;

	//printf_P(PSTR("in mmc_write_multipleblock\r"));

	MMC_SelectN(0);				//	Select card
	response=MMC_iCommand(MMC_WRITE_MULTIPLE_BLOCK,addr>>24,addr>>16,addr>>8,addr,0x95);
	//printf_P(PSTR("MMC_WRITE_MULTIPLE_BLOCK returns %X\r"),response);
	if (response!=0)				// Command failed
	{
		MMC_SelectN(1);			// Deselect card
		return response;				
	}

	tt1 = timer_ms_get();
	for(unsigned short rc=0;rc<repetition;rc++)
	{
		//printf_P(PSTR("Handling repetition %d\r"),rc);
		// Send data token MMC_STARTMULTIBLOCK
		SPI_Transmit(MMC_STARTMULTIBLOCK);								
		

		// Send data
		for(unsigned short k=0;k<512;k++)
		{															
			SPI_Transmit(buffer[k]);
		}																

		// Send CRC
		response1 = SPI_Transmit(0xFF);
		response2 = SPI_Transmit(0xFF);

		dataresponse = SPI_Transmit(0xFF);				// This should be the data response
		//printf_P(PSTR("dataresponse: %X\r"),dataresponse);
		if(dataresponse!=0xE5)
			printf_P(PSTR("dataresponse failed! %X\r"),dataresponse);
		response3 = SPI_Transmit(0xFF);					// This should be 'non idle'			
		//printf_P(PSTR("response3 (nonidle): %X\r"),response3);
	
	
		t1 = timer_ms_get();
		do
		{
			response = SPI_Transmit(0xFF);
			//printf_P(PSTR(" %X\r"),response);
		}
		while(response!=0xFF && (timer_ms_get()-t1<MMC_TIMEOUT_READWRITE));
		t2 = timer_ms_get();
		//printf_P(PSTR("Block %u busy time: %lu\r"),rc,t2-t1);

		//testfewmore();

		
	}
	tt2 = timer_ms_get();
	printf_P(PSTR("Data transfer time: %lu. KB/s: %lu\r"),tt2-tt1,500L*repetition/(tt2-tt1));
	
	printf_P(PSTR("Finalizing multiple block write\r"));
	response = SPI_Transmit(MMC_STOPMULTIBLOCK);
	printf_P(PSTR("response to stop multiblock %X\r"),response);
	t1 = timer_ms_get();
	do
	{
		response = SPI_Transmit(0xFF);
		printf_P(PSTR(" %X\r"),response);
	}
	while(response!=0xFF && (timer_ms_get()-t1<MMC_TIMEOUT_READWRITE));
	t2 = timer_ms_get();
	printf_P(PSTR("Final busy time: %lu\r"),t2-t1);

	testfewmore();




	
	MMC_SelectN(1);
	//printf_P(PSTR("writestream close dataresponse: %X %X %X %X. time. %lu\n"),response1,response2,dataresponse,response3,t2-t1);

	if(response!=0xFF)		// Time out before receiving answer -> failed.
		return 2;
	return 0;					// Write ok.
}




/******************************************************************************
*******************************************************************************
STREAM WRITE     STREAM WRITE     STREAM WRITE     STREAM WRITE     STREAM WRITE     
*******************************************************************************
******************************************************************************/
/*
	Stream write functions allow to initiate (open) a write operation to the memory card, 
	and write as few, or as much, data as needed. 
	
	The stream write function returns when the specified number of bytes has been written,
	or when a data block is completed (whichever occurs first).
	
	It returns the number of bytes effectively written and whether a block is completed.
	
	When the number of bytes effectively written is smaller than the write size, 
	the application must call multiple times the write function.

	
	
	Once the stream write is completed (closed) padding is executed to fill the write block.

	Internally, block write operations are used.
	

	Stream write consists of:
		1. Opening the block to write into with mmc_writestream_open
		2. Providing the block data with any number of calls to mmc_writestream_write
		3. Closing the block with mmc_writestream_close

*/

unsigned short _mmc_write_stream_numwritten;			// Number of bytes written within a block by a stream function
unsigned char _mmc_write_stream_open;					// Block open for writing
unsigned long _mmc_write_stream_address;				// Address to write to
/******************************************************************************
	mmc_write_stream_open
*******************************************************************************
	Start a stream write at the specified address using single block writes
	
******************************************************************************/
void mmc_write_stream_open(unsigned long addr)
{
	_mmc_write_stream_open=0;					// No opened block
	_mmc_write_stream_address=addr;			// Write address

}
/******************************************************************************
	mmc_write_stream_write_partial
*******************************************************************************
	Writes the lowest number of bytes between size, and the remaining space in the block. Returns the number of bytes effectively written.

	Improvement:	in case of errors, written indicates the number of bytes sent to the card, but not the number of bytes written to the card (that is always a multiple of blocksize)
						future version may return the number of bytes effectively written to the card.

						This version does not keep statistics about errors...

	Error handling:	In case of error return immediately. The data that was sent to the card is lost. The write sector increase.
							The application may continue calling this function - data write will proceed leaving a gap where there was the error.

	Return value:
		0:			ok
		other:	error
******************************************************************************/
unsigned char mmc_write_stream_write_partial(unsigned char *buffer,unsigned long size,unsigned long *written,unsigned char *blockcompleted)
{
	unsigned char response;
	unsigned long effw;

	*written=0;
	*blockcompleted=0;

	// if nothing to write, success
	if(size==0)
		return 0;

	// If the block isn't open, we open it.
	if(!_mmc_write_stream_open)
	{
		// Open block
		//printf_P(PSTR("Opening block %lu\r"),_mmc_write_stream_address);
		response=_mmc_write_block_open(_mmc_write_stream_address);
		// Return on error
		if(response!=0)
			return response;
		
		// Block successfully opened - initialize state vars
		_mmc_write_stream_open=1;				// block open
		_mmc_write_stream_numwritten=0;		// data written so far
	}

	// Write the data until: either all data is written, or a block is completed (whichever comes first)
	// Find the suitable write size: the smallest smallest of size or 512-_mmc_write_stream_numwritten
	effw = size;
	if(size<512-_mmc_write_stream_numwritten)
		effw=size;
	else
		effw=512-_mmc_write_stream_numwritten;

	// Write the data
	// This operation never fails: it's SPI transmit only.
	_mmc_write_block_writebuffer(buffer,effw);

	// Update the status
	_mmc_write_stream_numwritten+=effw;
	*written = effw;

	// Close the block if it is full
	if(_mmc_write_stream_numwritten>=512)
	{
		// Flag as closed, even if the close operation may fail, to ensure an open on next call.
		_mmc_write_stream_open=0;
		*blockcompleted=1;

		// Update the write address for the next write call. Update this even if the upcoming close my fail.
		_mmc_write_stream_address+=512;

		response=_mmc_write_block_close();
		if(response!=0)
			return response;
	}

	// Success
	return 0;								
}

/******************************************************************************
	mmc_write_stream_close
*******************************************************************************
	Finishes the write transaction.

	Pads the last open block with padchar.
		
	Return value:
		0:			ok
		other:	error
******************************************************************************/
unsigned char mmc_write_stream_close(unsigned char padchar)
{
	unsigned char response;

	// If the block is open, write padchar until the block is full.
	if(_mmc_write_stream_open)
	{
		// Number of bytes to pad
		unsigned short topad = 512-_mmc_write_stream_numwritten;

		// Write
		_mmc_write_block_writeconst(padchar,topad);

		// Update state 
		_mmc_write_stream_numwritten+=topad;

		// Flag as closed, even if the close operation may fail, to ensure an open on next call.
		_mmc_write_stream_open=0;
		
		response=_mmc_write_block_close();

		if(response!=0)
			return response;
	}

	return 0;					// Close ok
}

/******************************************************************************
*******************************************************************************
STREAM WRITE MULTIPLE BLOCK   STREAM WRITE MULTIPLE BLOCK   STREAM WRITE MULTIPLE BLOCK   
*******************************************************************************
******************************************************************************/
/*
	Stream write multiple block consists of:
		1. Opening the block to write into with mmc_write_streammulti_open
		2. Providing the block data with any number of calls to mmc_write_streammulti_write
		3. Closing the block with mmc_write_streammulti_close

	mmc_write_streammulti_write returns whenever a complete data block is written to the card, so that the application can decide e.g. to update a FAT 
	(mmc_write_streammulti_close must be called to release the card first).

	The number of bytes effectively written is made available, and it is the responsability of the application to call mmc_write_streammulti_write
	until all the data has been written.

	The following are the state variables:
		_mmc_write_stream_open;									// Block open for writing (.ie. multiblock command send)
		_mmc_write_stream_started								// Block is started (i.e. data token has been transmitted)
		_mmc_write_stream_numwritten;							// Number of bytes written within a block by a stream function
		_mmc_write_stream_address;								// Address to write to
*/


unsigned char _mmc_write_stream_started;					// Block is started (i.e. data token has been transmitted)


/******************************************************************************
	_mmc_write_multiblock_open
*******************************************************************************
	Internal use.
	
	Sensd a multiblock write command.
			
	Return value:
		0:			ok
		other:	error
******************************************************************************/
unsigned char _mmc_write_multiblock_open(unsigned long addr)
{
	unsigned char response;
	

	MMC_SelectN(0);				//	Select card
	response=MMC_iCommand(MMC_WRITE_MULTIPLE_BLOCK,addr>>24,addr>>16,addr>>8,addr,0x95);
	if (response!=0)				// Command failed
	{
		MMC_SelectN(1);			// Deselect card
		return response;				
	}
	return 0;
}
/******************************************************************************
	_mmc_write_multiblock_close
*******************************************************************************
	Internal use.			

	Terminates the multiblock write by sending MMC_STOPBLOCK and deselecting the card.

	Return value:
		0:			ok
		other:	error
******************************************************************************/
unsigned char _mmc_write_multiblock_close(void)
{
	unsigned char response;
	unsigned long int t1;

	// Send end of transfer token
	response = SPI_Transmit(MMC_STOPMULTIBLOCK);
	//printf_P(PSTR("response to stop multiblock %X\r"),response);
	t1 = timer_ms_get();
	do
	{
		response = SPI_Transmit(0xFF);
		//printf_P(PSTR(" %X\r"),response);
	}
	while(response!=0xFF && (timer_ms_get()-t1<MMC_TIMEOUT_READWRITE));

	//testfewmore();
	
	MMC_SelectN(1);										// Deselect card
	//printf_P(PSTR("writestream close dataresponse: %X %X %X %X. time. %lu\n"),response1,response2,dataresponse,response3,t2-t1);

	if(response!=0xFF)		// Time out before receiving answer -> failed.
		return 1;				// Return error
	return 0;					// Ok
}

/******************************************************************************
	mmc_write_streammulti_open
*******************************************************************************

	Start a stream write at the specified address using multiblock writes
			
******************************************************************************/
void mmc_write_streammulti_open(unsigned long addr)
{
	_mmc_write_stream_open=0;					// Command write multiblock not sent yet
	_mmc_write_stream_started = 0;			// Start data token not sent yet
	_mmc_write_stream_numwritten = 0;		// No bytes written yet
	_mmc_write_stream_address=addr;			// Write address
	
}


/******************************************************************************
	mmc_write_streammulti_write_partial
*******************************************************************************

	Writes the lowest number of bytes between size, and the remaining space in the block. Returns the number of bytes effectively written.
				
		buffer:		data to write
		size:			number of bytes to write
		written:		number of bytes effectively written when the function returns.


	It is the responsability of the application to call again if written!=size, passing the remainder of the data buffer.
	By guaranteeing to return on block boundaries, the application can decide to terminate the block write to release the bus or access the FAT, 
	or to resume the write operation on the next block by a subsequent call to this function.

	Improvement:	in case of errors, written indicates the number of bytes sent to the card, but not the number of bytes written to the card (that is always a multiple of blocksize)
						future version may return the number of bytes effectively written to the card.

	Error handling:	In case of error in write_partial, the stream write is terminated. This means:
								It is not necessary to call write_close - this is internally done.
								The stream is in closed state - it must be opened again by calling mmc_write_streammulti_open

	Return value:
		0:				success. In this case written indicates the number of bytes written to the card
		other:		failure.
******************************************************************************/
unsigned char mmc_write_streammulti_write_partial(unsigned char *buffer, unsigned long size,unsigned long *written,unsigned char *blockcompleted)
{
	// Find how many bytes can effectively be written until the sector is full.
	unsigned long effw;
	unsigned char response,response1,response2,dataresponse;
	unsigned long int t1;

	// Within this call: number of bytes sent to the card (errors not accounted for) and whether a block is completed
	*written=0;
	*blockcompleted=0;

	//	Write command not yet send.
	if(!_mmc_write_stream_open)
	{
		rv = _mmc_write_multiblock_open(_mmc_write_stream_address);
		
		if(rv)
			return 1;			// Abort if the open failed

		_mmc_write_stream_open = 1;			// Block write is now open,...
		_mmc_write_stream_started = 0;		// but start block has not yet been sent.
	}

	// Block start not yet sent
	if(_mmc_write_stream_started==0)
	{
		SPI_Transmit(MMC_STARTMULTIBLOCK);	// Send Data Token
		_mmc_write_stream_started=1;			// Block has started
	}

	// Write the data until: either all data is written, or a block is completed (whichever comes first)
	// Find the suitable write size: the smallest smallest of size or 512-_mmc_write_stream_numwritten
	effw = size;
	if(size<512-_mmc_write_stream_numwritten)
		effw=size;
	else
		effw=512-_mmc_write_stream_numwritten;


	// Send data
	_mmc_write_block_writebuffer(buffer,effw);
	

	// Update the counters
	_mmc_write_stream_numwritten+=effw;
	*written = effw;

	//printf_P(PSTR("write: numwritten: %u\r"),_mmc_write_stream_numwritten);

	// If a block is full, terminates it
	if(_mmc_write_stream_numwritten>=512)
	{
		//printf_P(PSTR("write: finishing block\r"));

		// Indicate a block was completed
		*blockcompleted=1;

		// Reset the internal state for the next block
		_mmc_write_stream_started=0;
		_mmc_write_stream_numwritten=0;


		rv = _mmc_write_block_stop()

		// Send CRC
		response1 = SPI_Transmit(0xFF);
		response2 = SPI_Transmit(0xFF);

		
		dataresponse = SPI_Transmit(0xFF);				// This should be the data response
		//response3 = SPI_Transmit(0xFF);					// This should be 'non idle', not needed as we loop until idle

		if((dataresponse&0x1F)!=0x05)						// Failure
		{
			testfewmore();

			// Terminate the transfer.
			_mmc_write_streammulti_terminate();

			return 1;											// Return error
		}
		
		//printf_P(PSTR("end block response: %02X %02X %02X\r"),response1,response2,dataresponse);

		// Wait until ready for next block
		t1 = timer_ms_get();
		do
		{
			response = SPI_Transmit(0xFF);
			//printf_P(PSTR(" %X\r"),response);
		}
		while(response!=0xFF && (timer_ms_get()-t1<MMC_TIMEOUT_READWRITE));
		if(response!=0xFF)									// Failure
		{
			// Terminate the transfer.
			// May not work if the communication with the card is really off. In this case terminate would also fail - in any case we return an error here.
			_mmc_write_streammulti_terminate();
			return 2;											// Return error
		}

		
		return 0;												// Success
	}
	return 0;
}


/******************************************************************************
	mmc_write_streammulti_close
*******************************************************************************
			
	Return value:
		0:			ok
		other:	error
******************************************************************************/
unsigned char mmc_write_streammulti_close(unsigned char padchar)
{
	unsigned char response;
	unsigned long int t1;
	unsigned long written;
	unsigned char blockcompleted;

	// If the block is open, but isn't completed, close takes care of this by writing padchar until the block is full.
	if(_mmc_write_stream_started)
	{
		unsigned short topad = 512-_mmc_write_stream_numwritten;
		//printf_P(PSTR("close: padding %u\r"),topad);
		for(unsigned short i=0;i<topad;i++)
		{
			mmc_write_streammulti_write_partial(&padchar,1L,&written,&blockcompleted);
			//printf_P(PSTR("writepad i=%d %lu %u\r"),i,written,blockcompleted);
		}
	}

	_mmc_write_streammulti_terminate();

}



