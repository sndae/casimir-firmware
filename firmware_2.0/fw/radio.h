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

#ifndef __RADIO_H
#define __RADIO_H

/*
	Notes on radio use

	RX mode
		Can wait for pin FFIT to be set to 1 or can wait for interrupt pin to be low, then read status register, to find what if the status of the FFIT pin

*/


/******************************************************************************
SI4420 Commands
******************************************************************************/
// STSREG: Status register
// Format:	0000 0000 0000 0000
//			Returns the status register: FFIT/RGIT POR FFOV/RGUR WKUP EXT LBD FFEM RSSI/ATS DQD CRL ATGL OFFS<6> OFFS<3> OFFS<2> OFFS<1> OFFS<0> 
//			FFIT: Bits received in RX FIFO reached limit. RGIT: TX register ready to receive next byte
//			POR: power-on reset (cleared after STSREG command)
//			FFOV: RX FIFO overflow. RGUR: TX register underrun
//			WKUP: wake-up timer overflow
//			EXT: logic level on interrupt pin changed to low
//			LBD: Low battery detect
//			FFEM: FIFO empty
//			RSSI: Strength of incoming signal above pre-programmed limit. ATS: Antenna tuning circuit detects strong enough RF signal
//			DQD: data quality detector output
//			CRL: clock recovery locked
//			ATGL: toggling in each AFC cycle
//			OFFS<6>: sign of the offset value
//			OFFS<3:0>: Offset value to add to the frequency control parameter
#define STSREG			0x0000	
// CSCREG:	Configuration Setting Command
// Format: 	1000  0000  el ef b1 b0  x3 x2 x1 x0
// 			b1|b0: 00=315MHz, 01=433MHz, 10=868MHz, 11=915MHz
// 			x3|x2|x1|x0: 0000=8.5pF, 0001=9.0pF, ..., 1111=16pF
// 			el: enable internal data register. If enabled FSK must be connected to logic high level.
// 			ef: enable the FIFO mode
#define CSCREG			0x8000		
// PMCREG:	Power Management Command
// Format:	1000  0010  er ebb et es  ex eb ew dc
// 			er: enable whole receiver chain (rf front end, baseband, synthesizer, oscillator)
//			ebb: receiver baseband circuit (baseband)
//			et: enable pll, power amplifier, start transmission if tx is enabled (power amplifier, synthesizer, oscillator)
//			es: turns on synthesizer
//			ex: turns on crystal oscillator
//			eb: enables low battery detector
//			ew: enable wake up timer
//			dc: disable the clock output
#define PMCREG			0x8200		
// FSCREG:	Frequency Setting Command
// Format:	1010  f11 f10 f9 f8  f7 f6 f5 f4  f3 f2 f1 f0
//			f11:f0: F in range [96;3903]. Synthesizer center frequency f0=10*C1*(C2+F/4000)MHz. C1,C2: 315MHz=1,31; 433MHz=1,43; 868MHz=2,43; 915MHz=3,30
#define FSCREG			0xA000
// DRCREG:	Data Rate Command
// Format:	1100 0110 cs r6 r5 r4 r3 r2 r1 r0
//			BR = 10000/29/(R+1)/(1+cs*7) kbps
//			R  = (10000/29/(1+cs*7)/BR)-1
#define DRCREG			0xC600		// 8b
// PSCREG:	Power Setting Command
// Format:	1001  0 p16 d1 d0  i2 i1 i0 g1  g0 r2 r1 r0
//			p16: function of pin nINT/VDI. p16=0: interrupt input. p16=1: VDI output
//			d1|d0: response time. 00=fast, 01=medium, 10=slow, 11=always on
//			i2|i1|i0: receiver baseband bandwidth. 001=400kHz, 010=340KHz, 011=270KHz, 100=200KHz, 101=134KHz, 110=67KHz
//			g1|g0: LNA gain: 00=0dB, 01=-6dB, 10=-14dB, 11=-20dB
//			r2|r1|r0: RSSI detector threshold: 000=-103dBm, 0001=-97dBm, 010=-91dBm, 011=-85dBm, 100=-79dBm, 101=-73dBm
#define PSCREG			0x9000
// DFCREG:	Data Filter Command
// Format:	1100  0010  al ml 1 s  1 f2 f1 f0
//			al: clock recovery auto lock
//			ml: 1: clock recovery fast mode (6-8 bit preamble recommended). 0: slow mode (12-16 bit preamble recommended)
//			s: 0:digital filter. 1:analog RC filter. In analog mode the internal clock recovery and FIFO cannot be used.
//			f2|f1f0: data quality detection (DQD) parameter used to report "good signal quality"
#define DFCREG			0xC200
// FRMCREG:	FIFO and Reset Mode Command
// Format:	1100  1010  f3 f2 f1 f0  0 al ff dr
//			f3|f2|f1|f0: FIFO interrupt level. Interrupt generated when F bits received
//			al:	FIFO fill start condition. 0=synchron pattern (0x2DD4). 1=always fill
//			ff: FIFO fill enabled after synchron pattern reception. FIFO fill stops when bit cleared.
//			dr: disables the highly sensitive reset
#define FRMCREG			0xCA00
// RFRCREG:	Receive FIFO Read Command
// Format:	1011  0000  0000 0000
//			Bit ef must be set in the Configuration Setting Command Register
//			No commands
//			Returns 16 bits : <15...8: FFIT in RX mode / RGIT otherwise><7...0: data>
#define RFRCREG		0xB000
// AFCCREG:	AFC Command
// Format:	1100  0100  a1 a0 rl1 rl0  st fi oe en
//			a1|a0: automatic operation mode selector. 00: auto mode off. 01: runs only once after powerup. 10: keep the foffset only during receiving. 11: keep the foffset value
//			rl1|rl0: 	range limit of the frequency offset register. 00: no restriction. 01: [+15fres;-16fres]. 10: [+7fres;-8fres]. 11: [+3fres;-4fres]. 
//						fres: 315MHz, 433MHz=2.5KHz. 868MHz=5KHz. 915MHz=7.5KHz
//			st:	strobe edge. when st goes high the last calculated frequency error is stored in the offset register of the AFC block
//			fi: high accuracy (fine) mode.
//			oe: enable the frequency offset register
//			en: enable the calculation of the offset frequency by the AFC circuit.
#define AFCCREG			0xC400
// TXCCCREG:TX Configuration Control Command
// Format:	1001  100 mp  m3 m2 m1 m0  0 p2 p1 p0
//			fout = f0+(-1)^sign*(M+1)*(15KHz)
//			M=<m3:m0>
//			f0=channel center frequency, see FSCREG
//			sign=mp xor fsk_input
//			p2|p1|p0: output power. 000=0dB. 001=-3dB. 010=-6dB. 011=-9dB. ...111=-21dB.
#define TXCCCREG		0x9800
// TRWCREG:	Transmit Register Write Command
//			1011  1000  t7 t6 t5 t4  t3 t2 t1 t0
//			Write <t7:t0> in the transmitter data register. Bit el must be set in the CSCREG
#define TRWCREG			0xB800
// WUTCREG:	Wake-Up Timer Command
// Format:	111 r4  r3 r2 r1 r0  m7 m6 m5 m4  m3 m2 m1 m0
//			Twakeup = M*2^R ms
//			Sending FE00 triggers a software reset
#define WUTCREG			0xE000
// LDCCREG:	Low Duty-Cycle Command
// Format:	1100  1000  d6 d5 d4 d3  d2 d1 d0 en
//			Duty-Cycle=(D*2+1)/M*100%.
//			M is a parameter in the wake-up timer command. 
//			The time cycle is defined by the wake-up timer command
//			en: enables the low duty-cycle mode. Wake-up timer interrupt not generated in this mode
#define LDCCREG			0xC800
// LBDMCDCREG:	Low Battery Detector and Microcontroller Clock Divider Command
// Format:	1100  0000  d2 d1 d0 v4  v3 v2 v1 v0
// 			Vlowbat = 2.25+V*0.1
//			d2|d1|d0: 000: 1MHz. 001: 1.25MHz. 010: 1.66MHz. 011: 2MHz. 100: 2.5MHz. 101: 3.33MHz. 110: 5MHz. 111: 10MHz.
//			Low battery detector and clock output can be disabled by bits eb and dc in the PMCREG.
#define LBDMCDCREG		0xC000


unsigned char mrf_irqlevel(void);
void mrf_setffs(unsigned char v);

void mrf_spi_select(char ss);
unsigned char mrf_spi_readwrite(unsigned char data);
void mrf_spi_init(void);

unsigned short mrf_icommand(unsigned short command);
unsigned short mrf_command(unsigned short command);
void mrf_icommands(unsigned short *commands,unsigned short n);
void mrf_commands(unsigned short *commands,unsigned short n);
void mrf_printstatus(void);
void mrf_printirq(void);
void mrf_txreg_write(unsigned char aByte);
unsigned char mrf_bitrate2div(unsigned long br);
unsigned long mrf_div2bitrate(unsigned char div);
void mrf_init433(unsigned long bitrate);
void mrf_setdatarate(unsigned long bps);
void mrf_setradiotx(void);
void mrf_setradiorx(void);
void mrf_setradioidle(void);
void mrf_fifoenable(void);
void mrf_fifodisable(void);
void mrf_settxpower(unsigned char power);
void mrf_setrxsens(unsigned char mode,unsigned char lna,unsigned char rssi);
void mrf_packet_transmit(unsigned char *buffer,unsigned char n);
unsigned char mrf_packet_receive(unsigned char *buffer,unsigned short n,unsigned short timeout);






#endif
