/*
   MMC Driver
   Copyright (C) 2007,2008,2009,2010:
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

#ifndef __MMC_H
#define __MMC_H

// MMC commands
#define MMC_GO_IDLE_STATE						0
#define MMC_SEND_OP_COND						1
#define MMC_SEND_CSD								9
#define MMC_SEND_CID								10
#define MMC_SEND_STATUS							13
#define MMC_SET_BLOCKLEN						16
#define MMC_READ_SINGLE_BLOCK					17
#define MMC_WRITE_BLOCK							24
#define MMC_WRITE_MULTIPLE_BLOCK				25
#define MMC_PROGRAM_CSD							27
#define MMC_SET_WRITE_PROT						28
#define MMC_CLR_WRITE_PROT						29
#define MMC_SEND_WRITE_PROT					30
#define MMC_TAG_SECTOR_START					32
#define MMC_TAG_SECTOR_END						33
#define MMC_UNTAG_SECTOR						34
#define MMC_TAG_ERASE_GROUP_START			35
#define MMC_TAG_ERARE_GROUP_END				36
#define MMC_UNTAG_ERASE_GROUP					37
#define MMC_ERASE									38
#define MMC_CMD55 								55
#define MMC_READ_OCR								58
#define MMC_CRC_ON_OFF							59

#define MMC_ACMD41 								41

#define MMC_STARTBLOCK             			0xFE
#define MMC_STARTMULTIBLOCK             	0xFC
#define MMC_STOPMULTIBLOCK             	0xFD

#define MMC_TIMEOUT_ICOMMAND					1000			// Timeout to receive an answer to simple commands (ms)
#define MMC_TIMEOUT_READWRITE					1000			// Timeout to receive an answer to read/write commands (ms)
#define MMC_RETRY									25				// Number of times commands are retried



typedef struct _CID {
	unsigned char MID;
	unsigned short OID;
	unsigned char PNM[7];
	unsigned char Rev;
	unsigned long PSN;
	unsigned char MDT;
} CID;

typedef struct _CSD {
	unsigned char CSD;							// [127:126]
	unsigned char SPEC_VERS;					// [125:122]
	unsigned char TAAC;						// [119:112]
	unsigned char NSAC;						// [111:104]
	unsigned char TRAN_SPEED;					// [103:96]
	unsigned short CCC;						// [95:84]
	unsigned char READ_BL_LEN;					// [83:80]
	unsigned char READ_BL_PARTIAL;				// [79:79]
	unsigned char WRITE_BLK_MISALIGN;			// [78:78]
	unsigned char READ_BLK_MISALIGN;			// [77:77]
	unsigned char DSR_IMP;						// [76:76]
	unsigned short C_SIZE;					// [73:62]
	unsigned char VDD_R_CURR_MIN;				// [61:59]
	unsigned char VDD_R_CURR_MAX;				// [58:56]
	unsigned char VDD_W_CURR_MIN;				// [55:53]
	unsigned char VDD_W_CURR_MAX;				// [52:50]
	unsigned char C_SIZE_MULT;					// [49:47]
	unsigned char ERASE_GRP_SIZE;				// [46:42]
	unsigned char ERASE_GRP_MULT;				// [41:37]
	unsigned char WP_GRP_SIZE;					// [36:32]
	unsigned char WP_GRP_ENABLE;				// [31:31]
	unsigned char R2W_FACTOR;					// [28:26]
	unsigned char WRITE_BL_LEN;				// [25:22]
	unsigned char WRITE_BL_PARTIAL;			// [21:21]
	unsigned char CONTENT_PROT_APP;			// [16:16]
	unsigned char FILE_FORMAT_GRP;				// [15:15]
	unsigned char COPY;						// [14:14]
	unsigned char PERM_WRITE_PROTECT;			// [13:13]
	unsigned char TMP_WRITE_PROTECT;			// [12:12]
	unsigned char FILE_FORMAT;					// [11:10]
	unsigned char ECC;						// [9:8]
} CSD;


// Internal functions
char SPI_Transmit(unsigned char cData);
unsigned char MMC_iCommand(unsigned char cmd,unsigned char p1,unsigned char p2,unsigned char p3,unsigned char p4, unsigned char crc);
unsigned char MMC_iCommandResponse(unsigned char cmd,unsigned char p1,unsigned char p2,unsigned char p3,unsigned char p4, unsigned char crc,unsigned char *response,unsigned short n);
void MMC_SelectN(char ss);
unsigned char MMC_Command(unsigned char cmd,unsigned char p1,unsigned char p2,unsigned char p3,unsigned char p4, unsigned char crc);
unsigned char MMC_CommandResponse(unsigned char cmd,unsigned char p1,unsigned char p2,unsigned char p3,unsigned char p4, unsigned char crc,unsigned char *response,unsigned short n);
unsigned char MMC_CommandRead(unsigned char cmd,unsigned char p1,unsigned char p2,unsigned char p3,unsigned char p4, unsigned char crc,unsigned char *buffer,unsigned short n);

// Initialization functions
unsigned char mmc_init(CID *cid,CSD *csd,unsigned long *capacity);


// Block read/write functions
//unsigned char mmc_write_block(unsigned long addr,unsigned char *buffer,unsigned char debug);
unsigned char mmc_write_block(unsigned long addr,unsigned char *buffer);
unsigned char mmc_read_block(unsigned long addr,unsigned char *buffer);
unsigned char mmc_write_multipleblock(unsigned long addr,unsigned char *buffer,unsigned short repetition);

// Stream write functions
void mmc_write_stream_open(unsigned long addr);
unsigned char mmc_write_stream_write_partial(unsigned char *buffer, unsigned long size,unsigned long *written,unsigned char *blockcompleted,unsigned long *currentaddr);
unsigned char mmc_write_stream_close(unsigned char padchar,unsigned long *currentaddr);

// Stream write multiple block functions
void mmc_write_streammulti_open(unsigned long addr);
unsigned char mmc_write_streammulti_write_partial(unsigned char *buffer, unsigned long size,unsigned long *written,unsigned char *blockcompleted,unsigned long *currentaddr);
unsigned char mmc_write_streammulti_close(unsigned char padchar,unsigned long *currentaddr);

#endif
