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

	printf("Benchmark\n");
	unsigned int t;
	timer_start();
	for(int i=0;i<100;i++)
	{
		packet_init(&packet);
		packet_addchar(&packet,'D');
		packet_addchar(&packet,'K');
		packet_addchar(&packet,'B');
		packet_addchar(&packet,device_id);
		packet_addshort_little(&packet,pktctr_abs);
		packet_addshort_little(&packet,12);
		packet_addshort_little(&packet,24);
		packet_addshort_little(&packet,36);
		packet_addshort_little(&packet,12);
		packet_addshort_little(&packet,24);
		packet_addshort_little(&packet,36);
		packet_addshort_little(&packet,50);
		packet_addshort_little(&packet,55);
		packet_addshort_little(&packet,60);
		packet_addshort_little(&packet,66);
		packet_addshort_little(&packet,99);
		/*packet_addshort(&packet,pktctr_abs);
		packet_addshort(&packet,12);
		packet_addshort(&packet,24);
		packet_addshort(&packet,36);
		packet_addshort(&packet,12);
		packet_addshort(&packet,24);
		packet_addshort(&packet,36);
		packet_addshort(&packet,50);
		packet_addshort(&packet,55);
		packet_addshort(&packet,60);
		packet_addshort(&packet,66);
		packet_addshort(&packet,99);*/
		packet_addchecksum_fletcher16_little(&packet);
	}
	t=timer_elapsed();
	printf("time: %d\n",t);
	timer_start();
	_delay_ms(1000);
	t=timer_elapsed();
	printf("time: %d\n",t);