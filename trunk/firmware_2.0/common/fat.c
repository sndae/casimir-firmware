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
/*
	Create a volume boot record, etc
*/
void mmc_format(void)
{
	char rv;
	// This is a typical volume boot record for a 2GB memory card
	/*unsigned char boot[90]={0xEB, 0x00, 0x90,							// Executable code
									'r','e','c','o','r','d','e','r',		// OEM name
									0x00, 0x02,									// bytes per sector (0x200)
									0x08,											// sector per cluster
									0x20, 0x00,									// reserved sectors
									0x02,											// num fat copies
									0x00,0x00,									// max root dir entries (NA)
									0x00,0x00,									// number of sector in partition smaller 32MB (NA)
									0xf8,											// media descriptor: f8=hard disk
									0x00,0x00,									// sector per fat (NA)
									0x3f,0x00,									// sector per track
									0x40,0x00,									// number of head
									0x3f,0x00,0x00,0x00,						// number of hidden sectors in partition (hs)
									0xc1,0x9f,0x3a,0x00,						// number of sectors in partition (ns). ns+hs times sector size is equal to the size of the card reported by the CSD command
									0xa5,0x0e,0x00,0x00,						// number of sectors per fat
									0x00,0x00,									// flags
									0x00,0x00,									// fat version
									0x02,0x00,0x00,0x00,						// cluster number start of root directory
									0x01,0x00,									// Sector Number of the File System Information Sector
									0x06,0x00,									// Sector Number of the Backup Boot Sector
									0,0,0,0,0,0,0,0,0,0,0,0,				// reserved
									0x00,											// logical drive number of partitions
									0x00,											// unused
									0x29,											// extended signature
									0x38,0x5c,0xdb,0xef,						// serial number of partition
									'd','i','g','r','e','c','o','r','d','e','r',	// volume name of partition
									'F','A','T','3','2',' ',' ',' '		// fat32
	};*/
	unsigned char boot[90]={0xEB, 0x00, 0x90,							// Executable code
									'M','S','D','O','S','5','.','0',		// OEM name
									0x00, 0x02,									// bytes per sector (0x200)
									0x08,											// sector per cluster
									0xC0, 0x02,									// reserved sectors
									0x02,											// num fat copies
									0x00,0x00,									// max root dir entries (NA)
									0x00,0x00,									// number of sector in partition smaller 32MB (NA)
									0xf8,											// media descriptor: f8=hard disk
									0x00,0x00,									// sector per fat (NA)
									0x3f,0x00,									// sector per track
									0xFF,0x00,									// number of head
									0x3f,0x00,0x00,0x00,						// number of hidden sectors in partition (hs)
									0xc1,0x9f,0x3a,0x00,						// number of sectors in partition (ns). ns+hs times sector size is equal to the size of the card reported by the CSD command
									0xa0,0x0e,0x00,0x00,						// number of sectors per fat
									0x00,0x00,									// flags
									0x00,0x00,									// fat version
									0x02,0x00,0x00,0x00,						// cluster number start of root directory
									0x01,0x00,									// Sector Number of the File System Information Sector
									0x06,0x00,									// Sector Number of the Backup Boot Sector
									0,0,0,0,0,0,0,0,0,0,0,0,				// reserved
									//0x00,											// logical drive number of partitions
									0x80,											// logical drive number of partitions
									0x00,											// unused
									0x29,											// extended signature
									0x2B,0xEA,0xDA,0x3C,						// serial number of partition
									'N','O',' ','N','A','M','E',' ',' ',' ',' ',	// volume name of partition
									'F','A','T','3','2',' ',' ',' '		// fat32
	};
	unsigned char fsis1[4]={ 0x52,0x52,0x61,0x41};					// first signature
	unsigned char fsis2[28]={0x72,0x72,0x41,0x61,						// second signature
									0xff,0xff,0xff,0xff,						// number of free clusters
									0x02,0x00,0x00,0x00,						// cluster number of most recently allocated cluster
									0,0,0,0,0,0,0,0,0,0,0,0,				// reserved
									0x00,0x00,									// unknown or null
									0x55,0xaa};									// boot record signature


									

	unsigned char zero=0;
	unsigned char marker[2]={0x55,0xaa};


	// Primary boot
	printf_P(PSTR("Writing primary boot\n"));
	rv = mmc_writestream_open(0);
	if(rv!=0)
	{
		printf_P(PSTR("Error opening card\n"));
		return;
	}
	mmc_writestream_write(boot,90);
	for(unsigned short i=0;i<420;i++)
		mmc_writestream_write(&zero,1);
	mmc_writestream_write(marker,2);
	rv = mmc_writestream_close();
	printf_P(PSTR("close: %d\n"),rv);

	// Primary FSIS
	printf_P(PSTR("Writing primary FSIS\n"));
	rv = mmc_writestream_open(1*512);
	if(rv!=0)
	{
		printf_P(PSTR("Error opening card\n"));
		return;
	}
	mmc_writestream_write(fsis1,4);
	for(unsigned short i=0;i<480;i++)
		mmc_writestream_write(&zero,1);
	mmc_writestream_write(fsis2,28);
	rv = mmc_writestream_close();
	printf_P(PSTR("close: %d\n"),rv);



	// Secondary boot
	printf_P(PSTR("Writing secondary boot\n"));
	rv = mmc_writestream_open(6*512);
	if(rv!=0)
	{
		printf_P(PSTR("Error opening card\n"));
		return;
	}
	mmc_writestream_write(boot,90);
	for(unsigned short i=0;i<420;i++)
		mmc_writestream_write(&zero,1);
	mmc_writestream_write(marker,2);
	rv = mmc_writestream_close();
	printf_P(PSTR("close: %d\n"),rv);

	// Secondary FSIS
	printf_P(PSTR("Writing secondary FSIS\n"));
	rv = mmc_writestream_open(7*512);
	if(rv!=0)
	{
		printf_P(PSTR("Error opening card\n"));
		return;
	}
	mmc_writestream_write(fsis1,4);
	for(unsigned short i=0;i<480;i++)
		mmc_writestream_write(&zero,1);
	mmc_writestream_write(fsis2,28);
	rv = mmc_writestream_close();
	printf_P(PSTR("close: %d\n"),rv);
}
