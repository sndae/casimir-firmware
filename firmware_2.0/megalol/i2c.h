/*
   MEGALOL - ATmega LOw level Library
	I2C Module
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

#ifndef __I2CSENSOR_H
#define __I2CSENSOR_H


#define I2C_READ			1
#define I2C_WRITE			0

struct _I2C_TRANSACTION;

typedef unsigned char(*I2C_CALLBACK)(struct _I2C_TRANSACTION *);

typedef struct  _I2C_TRANSACTION {
	unsigned char address;
	unsigned char rw;											// write: 0. read: 1.
	unsigned char data[16];									// data to send/receive
	unsigned char dataack[16];								// acknowledge to send/receive
	unsigned char doaddress;								// Send the address (otherwise, only the rest is done)
	unsigned char dodata;									// Send/read the data (otherwise, only the rest is done): specifies the number of bytes, or 0.
	unsigned char dostart;									// Send I2C start 
	unsigned char dostop;									// Send I2C stop 
	//void (*callback)(struct _I2C_TRANSACTION *);		// Callback to call upon completion or error
	I2C_CALLBACK callback;
	
	// Status information
	unsigned char status;									// Status:	Lower 4 bit indicate status. 
																	//				0: success. 
																	//				1: failed in start condition.
																	//				2: failed in address.
																	//				3: failed in data (high 4 bit indicated how many bytes were transmitted successfully before the error)
	unsigned char i2cerror;									// Indicate the AVR I2C error code, when status is non-null

	// Internal stuff
	unsigned char _n;
	unsigned char _state;

} I2C_TRANSACTION;


/******************************************************************************
*******************************************************************************
DIRECT ACCESS   DIRECT ACCESS   DIRECT ACCESS   DIRECT ACCESS   DIRECT ACCESS   
*******************************************************************************
******************************************************************************/

void i2c_init(void);
unsigned char i2c_readstart(unsigned addr7,unsigned char debug);
unsigned char i2c_readdata(unsigned char *data,unsigned char acknowledge,unsigned char debug);
unsigned char i2c_writestart(unsigned addr7,unsigned char debug);
unsigned char i2c_writedata(unsigned char data,unsigned char debug);
void i2c_stop(unsigned char debug);

/******************************************************************************
*******************************************************************************
INTERRUPT-DRIVEN ACCESS   INTERRUPT-DRIVEN ACCESS   INTERRUPT-DRIVEN ACCESS   
*******************************************************************************
******************************************************************************/

unsigned char i2c_transaction_execute(I2C_TRANSACTION *trans,unsigned char blocking);
unsigned char i2c_transaction_idle(void);
void i2c_transaction_cancel(void);
void i2c_int_tst(void);

#endif



