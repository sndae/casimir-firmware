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
	Device specific elements here.
*/

//#include "pkt.h"


/******************************************************************************
*******************************************************************************
FUSE SETUP
*******************************************************************************
******************************************************************************/
// With crystal:

// Extended: FD
// High: D1
// Low: FF


// With internal RC oscillator:
// Extended: FD
// High: D1
// Low: E2


/******************************************************************************
*******************************************************************************
INITIAL SETUP
*******************************************************************************
******************************************************************************/
// ID is only written to Atmega-EEPROM and BNC-EEPROM if DO_FIRST_SETTINGS=1
#define ID 28
#define DEBUG 0

/******************************************************************************
*******************************************************************************
DEVICE PERIPHERAL CONFIGURATION   
*******************************************************************************
******************************************************************************/
#define PORT_LED1 6
#define PORT_LED2 7

/*#define PORT_ACCX 2
#define PORT_ACCY 3
#define PORT_ACCZ 4*/


/******************************************************************************
*******************************************************************************
DEVICE FEATURE CONFIGURATION
*******************************************************************************
******************************************************************************/

/******************************************************************************
*******************************************************************************
DEVICE PORT CONFIGURATION   
*******************************************************************************
******************************************************************************/

// Alternative 2 (unused pins as input with pull up) leads to lower power compared to output set to 0

// Port configuration
/*  
DDRx: 1=output; 0=input

                     I/O  PU
PortA.0 Vhalf         I    -
PortA.1 MICOUT        I    -
PortA.2 Acc_Xsig      I    -
PortA.3 Acc_Ysig      I    -
PortA.4 Acc_Zsig      I    -
PortA.5 in_photo      I    -
PortA.6 VBatHalf      I    -
PortA.7 XC_PLUGGEDIN  I    H

DDRA= 00000000= 0x00
PORTA=10000000= 0x80
*/
#define INIT_DDRA 0x00
#define INIT_PORTA 0x80

// Digital Input Disable Register (1=disable, 0=enable);
#define INIT_DIR0 0xFF		/* Deactivate the digital input buffer on the analog inputs */


/*
Radio connected or not connected:

PortB.0 X_CS          O    1		// chip not selected
PortB.1 PowerOnCtrl   O    1
PortB.2 UIButton      I    H
PortB.3 BUZZER        O    -
PortB.4 sd_cs#        O    -
PortB.5 sd_mosi       O    -
PortB.6 sd_miso       I    -
PortB.7 sd_clk        O    -

// radio connected:
DDRB= 10111011 = 0xBB;
PORTB=00000111 = 0x07;
*/
#define INIT_DDRB 0xBB
#define INIT_PORTB 0x07

// Alternative, wo radio, unused as output ground
// DDRB 11111011 = 0xfb
#define INIT_DDRB_ALT 0xFB
#define INIT_PORTB_ALT 0x07
// Alternative, wo radio, unused as input pull up
// DDRB 10110011 = 0xB3
// PORTB 00001111 0x0f
#define INIT_DDRB_ALT2 0xB3
#define INIT_PORTB_ALT2 0x0f

/*

Radio connected:
                     I/O  PU
PortC.0 S_SCL         O    -
PortC.1 S_SDA         I    -
PortC.2 PB0_X_X0      O    0				// FSK/DATA/FFS#
PortC.3 PB1_X_INT1    I    -
PortC.4 X_IRQ         I    -
PortC.5 X_INT0        I    -
PortC.6 Q0_VRegError  I    H
PortC.7 Q1_X_X1       I    -

DDRC= 00000101b = 0x05
PORTC=01000000b = 0x40
*/

// DIR WRONG on X0?
#define INIT_DDRC 0x01				
#define INIT_PORTC 0x40

// Alternative, wo radio, unused as output ground
// DDRC 11111101 = 0x02
#define INIT_DDRC_ALT 0x02
#define INIT_PORTC_ALT 0x00

// Alternative, wo radio, unused as input pull up
// DDRC 00000010 = 0x02
// PORTC 11111100 = 0xfc
#define INIT_DDRC_ALT2 0x02
#define INIT_PORTC_ALT2 0xfc
					
/*

// Radio connected
                       I/O  PU
PortD.0 XC_RXD          I    -
PortD.1 XC_TXD          O    -
PortD.2 X_MISO          I    -
PortD.3 X_MOSI          O    -	
PortD.4 X_SCK           O    -	
PortD.5 ADXLST_XC_SYNC  O    -
PortD.6 LED0            O    -
PortD.7 LED1            O    -

DDRB = 11111010 = 0xFA
PORTD= 00000000 = 0x00

// Radio not connected
                       I/O  PU
PortD.0 XC_RXD          I    -
PortD.1 XC_TXD          O    -
PortD.2 X_MISO          O    -
PortD.3 X_MOSI          O    -	
PortD.4 X_SCK           O    -	
PortD.5 ADXLST_XC_SYNC  O    -
PortD.6 LED0            O    -
PortD.7 LED1            O    -

DDRB = 11111110 = 0xFE
PORTD= 00000000 = 0x00

// Other settings:
DDRB = 11100010 = 0xE2	// ok
DDRB = 11111010 = 0xFA		// continuous reboot with radio module
DDRB = 11110010 = 0xF2		// continuous reboot with radio module
DDRB = 11101010 = 0xEA		// continuous reboot with radio module
PORTD= 00000000 = 0x00
*/
#define INIT_DDRD 0xFA;
#define INIT_PORTD 0x00;
//#define INIT_PORTD 0xC0;

// Alternative, wo radio, unused as output ground
// DDRD 11111111 = 0x0FE
#define INIT_DDRD_ALT 0xFF
#define INIT_PORTD_ALT 0x00

// Alternative, wo radio, unused as input pull up
// DDRD 11100000 = 0xE0
#define INIT_DDRD_ALT2 0xE0
#define INIT_PORTD_ALT2 0x1F



/******************************************************************************
*******************************************************************************
Command interface
*******************************************************************************

	The format of the received commands is as follows:
		STX CMD PAYLOAD[8] CKSUM ETX		: 12 bytes
	
	The format of the response commands is as follows:
		STX CMD PAYLOAD[8] CKSUM ETX		: 12 bytes

	For debug purposes: STX=S, ETX=E.

******************************************************************************/
#define XCOMMAND_SIZE 20
typedef struct
{
	unsigned char cmd;
	unsigned char payload[XCOMMAND_SIZE-4];
	unsigned char xsum;
} XCOMMAND;


/******************************************************************************
*******************************************************************************
STUFF
*******************************************************************************
******************************************************************************/


extern unsigned long int deviceid;



/******************************************************************************
*******************************************************************************
FUNCTIONS & CALLBACKS
*******************************************************************************
******************************************************************************/
void Configure(FILE *file);
unsigned char LoadConfiguration(void);
void SaveConfiguration(void);
void init_Specific(void);
//unsigned char BuildPacket(PACKET *packet);
unsigned ReadKeyboard(void);

void timer_callback_100hz(void);
void timer_callback_10hz(void);

extern unsigned char sharedbuffer[520];
extern volatile unsigned short batteryvoltage;

void main_loggingapplication(void);
void off(void);

