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
#include "adc.h"
#include "main.h"
#include "wait.h"
#include "mmc.h"
#include "test_mmc.h"
#include "helper2.h"





void test_mmc_benchmarkwriteblock(void)
{
	unsigned long t1,t2;
	unsigned long tt1,tt2,tmax;
	unsigned long tb1,tb2;
	unsigned char rv;
	unsigned long i;
	unsigned long bins[33];		// bins of 8, the last bin is 'others'


	unsigned long startb[4]={0,   4096, 8192, 262144};
	unsigned long endb[4]={4096,8192,  12288, 262144+4096};


	printf_P(PSTR("Benchmarking sequential block write\r"));

	// Set something in the buffer
	for(i=0;i<512;i++)
		sharedbuffer[i]=i;

	for(int bi=0;bi<4;bi++)			// Iterate the regions of the card to test
	{
		// Clear the bins and statistics
		for(i=0;i<33;i++)
			bins[i]=0;
		tmax=0;

		printf_P(PSTR("Benchmarking blocks %ld->%ld\r"),startb[bi],endb[bi]);

		t1 = tb1 = timer_ms_get();
		tmax=0;
		for(i=startb[bi];i<endb[bi];i++)
		{
			sharedbuffer[0]=i;
			
			tt1=timer_ms_get();
			rv = mmc_write_block(i<<9,sharedbuffer);
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
				printf_P(PSTR("Write block failed on block %d\r"),i);
				break;
			}

			if( (i&1023)==1023)
			{
				tb2 = timer_ms_get();
				printf_P(PSTR("Block %lu Total time: %lu Average time per block: %lu Write speed: %lu KB/s\r"),i+1,tb2-tb1,(tb2-tb1)/1024,512L*1000/(tb2-tb1));
				tb1 = timer_ms_get();

			}
		}
		t2 = timer_ms_get();
		printf_P(PSTR("Total time: %lu Average time per block: %lu Maximum time per block: %lu\r"),t2-t1,(t2-t1)/(endb[bi]-startb[bi]),tmax);
		printf_P(PSTR("Average write speed: %lu KB/s\r"),(endb[bi]-startb[bi])*500L/(t2-t1));
		printf_P(PSTR("Histogram of times:\r"));
		for(i=0;i<32;i++)
		{
			printf_P(PSTR("[%lu-%lu) %lu   "),i<<3,(i<<3)+8,bins[i]);
		}
		printf_P(PSTR("[%lu-inf) %lu\r"),i<<3,bins[i]);
	}
}
void test_mmc_benchmarkwritestream(void)
{
	unsigned long t1,t2;
	unsigned long tt1,tt2,tmax;
	unsigned long tb1,tb2;
	unsigned char rv;
	unsigned long i;
	unsigned long bins[33];		// bins of 8, the last bin is 'others'

	unsigned long startb[4]={0,   4096, 8192, 262144};
	unsigned long endb[4]={4096,8192,  12288, 262144+4096};


	unsigned char blockcompleted;
	unsigned long written;
	unsigned long currentblockaddr;
	

	printf_P(PSTR("Benchmarking sequential stream write\r"));

	// Set something in the buffer
	for(i=0;i<512;i++)
		sharedbuffer[i]=i;

	for(int bi=0;bi<4;bi++)			// Iterate the regions of the card to test
	{
		// Clear the bins and statistics
		for(i=0;i<33;i++)
			bins[i]=0;
		tmax=0;

		printf_P(PSTR("Benchmarking blocks %ld->%ld\r"),startb[bi],endb[bi]);

		// Open the block
		mmc_write_stream_open(startb[bi]<<9);			// shift by 9 as startb is block, open requires bytes

		t1 = tb1 = timer_ms_get();
		tmax=0;
		for(i=startb[bi];i<endb[bi];i++)
		{
			sharedbuffer[0]=i;
			
			tt1=timer_ms_get();
			rv = mmc_write_stream_write_partial(sharedbuffer,512,&written,&blockcompleted,&currentblockaddr);
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
				printf_P(PSTR("Write failed on call %d\r"),i);
				break;
			}

			if( (i&1023)==1023)
			{
				tb2 = timer_ms_get();
				printf_P(PSTR("Block %lu Total time: %lu Average time per block: %lu Write speed: %lu KB/s\r"),i+1,tb2-tb1,(tb2-tb1)/1024,512L*1000/(tb2-tb1));
				tb1 = timer_ms_get();
			}
		}
		t2 = timer_ms_get();

		// Close the block
		rv = mmc_write_stream_close(0,&currentblockaddr);
		printf_P(PSTR("Stream write close: %d\r"),rv);
		if(rv!=0)
		{
			printf_P(PSTR("Stream write close failed: %d\r"),rv);
		}
		printf_P(PSTR("Last used block: %ld\r"),currentblockaddr>>9);
		printf_P(PSTR("Total time: %lu Average time per block: %lu Maximum time per block: %lu\r"),t2-t1,(t2-t1)/(endb[bi]-startb[bi]),tmax);
		printf_P(PSTR("Average write speed: %lu KB/s\r"),(endb[bi]-startb[bi])*500L/(t2-t1));
		printf_P(PSTR("Histogram of times:\r"));
		for(i=0;i<32;i++)
		{
			printf_P(PSTR("[%lu-%lu) %lu   "),i<<3,(i<<3)+8,bins[i]);
		}
		printf_P(PSTR("[%lu-inf) %lu\r"),i<<3,bins[i]);
	}
}
void test_mmc_benchmarkwritestreammulti(void)
{
	unsigned long t1,t2;
	unsigned long tt1,tt2,tmax;
	unsigned long tb1,tb2;
	unsigned short rv;
	unsigned long i;
	unsigned long bins[33];		// bins of 8, the last bin is 'others'
	
	unsigned long startb[4]={0,   4096, 8192, 262144};
	unsigned long endb[4]={4096,8192,  12288, 262144+4096};

	unsigned long written;
	unsigned char blockcompleted;
	unsigned long currentblockaddr;


	printf_P(PSTR("Benchmarking sequential stream write multiblock\r"));


// Set something in the buffer
	for(i=0;i<512;i++)
		sharedbuffer[i]=i;


	for(int bi=0;bi<4;bi++)			// Iterate the regions of the card to test
	{
		for(i=0;i<33;i++)
			bins[i]=0;

		printf_P(PSTR("Benchmarking blocks %ld->%ld\r"),startb[bi],endb[bi]);

		mmc_write_streammulti_open(startb[bi]<<9);			// shift by 9 as startb is block, open requires bytes
		printf_P(PSTR("Stream multiblock write open\r"));
		

		t1 = tb1 = timer_ms_get();
		tmax=0;
		for(i=startb[bi];i<endb[bi];i++)
		{
			sharedbuffer[0]=i;
			
			tt1=timer_ms_get();
			rv = mmc_write_streammulti_write_partial(sharedbuffer,512,&written,&blockcompleted,&currentblockaddr);
			tt2=timer_ms_get();
			if(rv!=0)
				printf_P(PSTR("Error writing %lu\r"),i);
			//printf_P(PSTR("%u\r"),tt2-tt1);
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
				printf_P(PSTR("Write block failed on block %d\r"),i);
				break;
			}

			if( (i&1023)==1023)
			{
				tb2 = timer_ms_get();
				printf_P(PSTR("Block %lu Total time: %lu Average time per block: %lu Write speed: %lu KB/s\r"),i+1,tb2-tb1,(tb2-tb1)/1024,512L*1000/(tb2-tb1));
				tb1 = timer_ms_get();
			}
		}
		rv = mmc_write_streammulti_close(0,&currentblockaddr);
		printf_P(PSTR("mmc_write_streammulti_close: %X\r"),rv);
		t2 = timer_ms_get();

		printf_P(PSTR("Last used block: %ld\r"),currentblockaddr>>9);
		printf_P(PSTR("Total time: %lu Average time per block: %lu Maximum time per block: %lu\r"),t2-t1,(t2-t1)/(endb[bi]-startb[bi]),tmax);
		printf_P(PSTR("Average write speed: %lu KB/s\r"),(endb[bi]-startb[bi])*500L/(t2-t1));
		printf_P(PSTR("Histogram of times:\r"));
		for(i=0;i<32;i++)
		{
			printf_P(PSTR("[%lu-%lu) %lu   "),i<<3,(i<<3)+8,bins[i]);
		}
		printf_P(PSTR("[%lu-inf) %lu\r"),i<<3,bins[i]);
	}

}


void benchmarkread(void)
{
	unsigned long t1,t2;
	unsigned long tt1,tt2,tmax;
	unsigned long tb1,tb2;
	unsigned char rv;
	unsigned long i;
	unsigned long bins[33];		// bins of 8, the last bin is 'others'

	// Clear the buffer
	memset(sharedbuffer,0,512);
	for(i=0;i<33;i++)
		bins[i]=0;

	printf_P(PSTR("Benchmarking sequential read\r"));

	t1 = tb1 = timer_ms_get();
	tmax=0;
	for(i=0L;i<100000L;i++)
	{
		if((i&1023)==0)
		{
			tb2 = timer_ms_get();
			printf_P(PSTR("Block %lu Total time: %lu Average time per block: %lu Read speed: %lu KB/s\r"),i,tb2-tb1,(tb2-tb1)/1024,512L*1000/(tb2-tb1));
			tb1 = timer_ms_get();

		}
		tt1=timer_ms_get();
		rv = mmc_read_block(i<<9,sharedbuffer);
		tt2=timer_ms_get();
		//printf_P(PSTR("%u\r"),tt2-tt1);
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
			printf_P(PSTR("Read block failed on block %d\r"),i);
			break;
		}
	}
	t2 = timer_ms_get();
	printf_P(PSTR("Total time: %lu Average time per block: %lu Maximum time per block: %lu\r"),t2-t1,(t2-t1)/i,tmax);
	printf_P(PSTR("Average read speed: %lu KB/s\r"),i*500L/(t2-t1));
	printf_P(PSTR("Histogram of times:\r"));
	for(i=0;i<32;i++)
	{
		printf_P(PSTR("[%lu-%lu) %lu   "),i<<3,(i<<3)+8,bins[i]);
	}
	printf_P(PSTR("[%lu-inf) %lu\r"),i<<3,bins[i]);
		for(i=0;i<33;i++)
	{
		printf_P(PSTR("%lu\t%lu\r"),i<<3,bins[i]);
	}	
}




void test_mmc_0(void)
{
	unsigned char rv;
	for(unsigned short i=0;i<512;i++)
		sharedbuffer[i]=i;

	rv = mmc_write_block(0,sharedbuffer);
	printf_P(PSTR("write block: %d\r"),rv);
}




void test_mmc_clear(void)
{
	unsigned char rv;
	unsigned char zero=0;
	unsigned long written;
	unsigned char blockcompleted;
	unsigned long startb[2]={0,8192};
	unsigned short maxblock = 16;


	for(unsigned char bi=0;bi<2;bi++)
	{

		printf_P(PSTR("Clearing %d blocks at %ld\r"),maxblock,startb[bi]);

		for(unsigned short j=0;j<maxblock;j++)
		{
			unsigned long block = startb[bi]+j;
			mmc_write_stream_open(block<<9);
	
			for(unsigned short i=0;i<512;i++)
				mmc_write_stream_write_partial(&zero,1,&written,&blockcompleted,0);

			rv = mmc_write_stream_close(0,0);
			printf_P(PSTR("Closing block %d: %d\r"),block,rv);
		}
	}
}

void test_mmc_writepattern(void)
{
	unsigned char rv;
	unsigned char pattern;
	unsigned long written;
	unsigned char blockcompleted;
	unsigned long startb[2]={0,8192};
	unsigned short maxblock = 16;


	for(unsigned char bi=0;bi<2;bi++)
	{

		printf_P(PSTR("Patterning %d blocks at %ld\r"),maxblock,startb[bi]);

		for(unsigned short j=0;j<maxblock;j++)
		{
			unsigned long block = startb[bi]+j;

			pattern = j;

			mmc_write_stream_open(block<<9);
		
	
			for(unsigned short i=0;i<512;i++)
			{
				mmc_write_stream_write_partial(&pattern,1,&written,&blockcompleted,0);
				pattern += j+1;
			}

			rv = mmc_write_stream_close(0,0);
			printf_P(PSTR("Closing block %d: %d\r"),block,rv);
		}
	}
}

void test_mmc_printblocks(unsigned long int startblock)
{
	unsigned char rv;
	unsigned long maxblock = 16;

	printf_P(PSTR("Reading blocks %ld - %ld\r"),startblock,startblock+maxblock-1);

	for(unsigned long int b=startblock;b<startblock+maxblock;b++)
	{
		printf_P(PSTR("Block %d\r"),b);
		rv = mmc_read_block(b<<9,sharedbuffer);
		if(rv==0)
			prettyprintblock(sharedbuffer,b<<9);
		else
			printf_P(PSTR("mmc_read_block failed: %d\r"),rv);
		
	}
}



