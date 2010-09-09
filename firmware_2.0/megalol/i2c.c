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
#include "test.h"
#include "wait.h"
#include "i2c.h"


// Internal stuff
volatile unsigned char _i2c_transaction_idle=1;		// Indicates whether a transaction is in progress or idle.
I2C_TRANSACTION *_i2c_current_transaction;				// Current transaction for the interrupt routine.




/******************************************************************************
*******************************************************************************
DIRECT ACCESS   DIRECT ACCESS   DIRECT ACCESS   DIRECT ACCESS   DIRECT ACCESS   
*******************************************************************************
******************************************************************************/


void i2c_init(void)
{
	TWCR = 0;					// Deactivate
	//TWBR = 52; TWSR = 0;	// 100KHz @ 12 MHz
	//TWBR = 52; TWSR = 2;	// ~7KHz @ 12 MHz
	
	TWBR = 7; TWSR = 0;	// 400KHz @ 12 MHz
	TWCR = (1<<TWEN);			// Enable TWI
}

/*
	Starts a write by:
		- Sending a start condition
		- Sending the slave address
		- Returning the state of the acknowledge
	
	Upon success, the applicaton shall write data using i2c_writedata

	Return values:
		0x08:		Start transmitted
		0x10:		Repeat start transmitted
		0x18:		SLA+W transmitted, ack received
		0x20:		SLA+W transmitted, ack not received
		0x38:		data  transmitted, ack received
		0x30:		data  transmitted, ack not received
		0x38:		arbitration lost in SLA+W or data bytes
		0x00:		successful completion (start sent, SLA+W sent and ack received)
*/
unsigned char i2c_writestart(unsigned addr7,unsigned char debug)
{
	unsigned char twcr,twsr,twdr;

	// Transmit a start condition
	if(debug)
		printf_P(PSTR("        I2C START\r"));
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);						
	do
	{
		twcr = TWCR;
		twsr = TWSR;
		twdr = TWDR;
		if(debug)
			printf_P(PSTR("                TWCR: %X TWSR: %X TWDR: %X\r"),twcr,twsr,twdr);
	}
	while (!(twcr & (1<<TWINT)));										// Wait until TWINT is set
	twsr &= 0xF8;
	if((twsr!=0x08) && (twsr!=0x10))
	{
		if(debug)
			printf_P(PSTR("        I2C START error (%X)\r"),twsr);
		return twsr;
	}
	if(debug)
		printf_P(PSTR("        I2C START ok (%X)\r"),twsr);

	// Transmit the address
	if(debug)
		printf_P(PSTR("        I2C SLA+W\r"));
	TWDR = addr7<<1;														// I2C 7-bit address + write
	TWCR = (1<<TWINT) | (1<<TWEN);								// Send
	do
	{
		twcr = TWCR;
		twsr = TWSR;
		twdr = TWDR;
		if(debug)
			printf_P(PSTR("                TWCR: %X TWSR: %X TWDR: %X\r"),twcr,twsr,twdr);
	}
	while (!(twcr & (1<<TWINT)));										// Wait until TWINT is set
	twsr &= 0xF8;

	if(twsr != 0x18)
	{
		if(debug)
			printf_P(PSTR("        I2C address send error (%X)\r"),twsr);
		return twsr;
	}
	if(debug)
		printf_P(PSTR("        I2C address send ok (%X)\r"),twsr);

	return 0;
}
/*
	i2c_writedata
	Assumes a i2c_startwrite was successfully called, i.e. a slave has acknowledged being addressed and is waiting for data.

	Return values:
		See i2c_writestart

*/
unsigned char i2c_writedata(unsigned char data,unsigned char debug)
{
	unsigned char twcr,twsr,twdr;	

	if(debug)
		printf_P(PSTR("        I2C DATA Write\r"));
	
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN);
	do
	{
		twcr = TWCR;
		twsr = TWSR;
		twdr = TWDR;
		if(debug)
			printf_P(PSTR("                TWCR: %X TWSR: %X TWDR: %X\r"),twcr,twsr,twdr);
	}
	while (!(twcr & (1<<TWINT)));										// Wait until TWINT is set
	twsr &= 0xF8;

	if(twsr != 0x28)
	{
		if(debug)
			printf_P(PSTR("        I2C DATA Write error (%X)\r"),twsr);
		return twsr;
	}
	if(debug)
		printf_P(PSTR("        I2C DATA Write ok (%X)\r"),twsr);
	return 0;
}
/*

	Return values:
		0x08:		Start transmitted
		0x10:		Repeat start transmitted
		0x38:		Arbitration lost in SLA+R or not acknowledge
		0x40:		SLA+R transmitted, ack received
		0x48:		SLA+R transmitted, not ack received
		0x50:		data byte received, returned ack
		0x58: 	data byte received, returned not ack
*/
unsigned char i2c_readstart(unsigned addr7,unsigned char debug)
{
	unsigned char twcr,twsr,twdr;

	// Transmit a start condition
	if(debug)
		printf_P(PSTR("        I2C START\r"));
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);						
	do
	{
		twcr = TWCR;
		twsr = TWSR;
		twdr = TWDR;
		if(debug)
			printf_P(PSTR("                TWCR: %X TWSR: %X TWDR: %X\r"),twcr,twsr,twdr);
	}
	while (!(twcr & (1<<TWINT)));										// Wait until TWINT is set
	twsr &= 0xF8;
	if((twsr!=0x08) && (twsr!=0x10))		// Start and repeat start are okay
	{
		if(debug)
			printf_P(PSTR("        I2C START error (%X)\r"),twsr);
		return twsr;
	}
	if(debug)
		printf_P(PSTR("        I2C START ok (%X)\r"),twsr);

	// Transmit the address
	if(debug)
		printf_P(PSTR("        I2C SLA+R\r"));
	TWDR = (addr7<<1)+1;														// I2C 7-bit address + read
	TWCR = (1<<TWINT) | (1<<TWEN);						// Send
	do
	{
		twcr = TWCR;
		twsr = TWSR;
		twdr = TWDR;
		if(debug)
			printf_P(PSTR("                TWCR: %X TWSR: %X TWDR: %X\r"),twcr,twsr,twdr);
	}
	while (!(twcr & (1<<TWINT)));										// Wait until TWINT is set
	twsr &= 0xF8;

	if(twsr != 0x40)
	{
		if(debug)
			printf_P(PSTR("        I2C address send error (%X)\r"),twsr);
		return twsr;
	}
	if(debug)
		printf_P(PSTR("        I2C address send ok (%X)\r"),twsr);

	return 0;
}
/*
	i2c_readdata
	Assumes a i2c_startread was successfully called, i.e. a slave has acknowledged being addressed and is ready to send data.

	Return values:
		See i2c_readstart

*/
unsigned char i2c_readdata(unsigned char *data,unsigned char acknowledge,unsigned char debug)
{
	unsigned char twcr,twsr,twdr;	

	if(debug)
		printf_P(PSTR("        I2C DATA Read\r"));
	
	if(acknowledge)
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	else
		TWCR = (1<<TWINT) | (1<<TWEN);
	do
	{
		twcr = TWCR;
		twsr = TWSR;
		twdr = TWDR;
		if(debug)
			printf_P(PSTR("                TWCR: %X TWSR: %X TWDR: %X\r"),twcr,twsr,twdr);
	}
	while (!(twcr & (1<<TWINT)));										// Wait until TWINT is set
	twsr &= 0xF8;

	if((acknowledge && twsr != 0x50) | (acknowledge==0 && twsr != 0x58))
	{
		if(debug)
			printf_P(PSTR("        I2C DATA Read error (%X)\r"),twsr);
		return twsr;
	}
	if(debug)
		printf_P(PSTR("        I2C DATA Read ok (%X)\r"),twsr);
	*data = twdr;
	return 0;
}

void i2c_stop(unsigned char debug)
{
	unsigned char twcr,twsr,twdr;	
	twcr = TWCR;
	twsr = TWSR;
	twdr = TWDR;
	if(debug)
		printf_P(PSTR("STOP:                TWCR: %X TWSR: %X TWDR: %X\r"),twcr,twsr,twdr);
	TWCR = (1<<TWINT)|(1<<TWEN)| (1<<TWSTO);		// Transmit stop
	twcr = TWCR;
	twsr = TWSR;
	twdr = TWDR;
	if(debug)
		printf_P(PSTR("STOP:                TWCR: %X TWSR: %X TWDR: %X\r"),twcr,twsr,twdr);
	while(TWCR&0x10);							// Wait until the stop condition is transmitted (bit is cleared upon transmission)
	twcr = TWCR;
	twsr = TWSR;
	twdr = TWDR;
	if(debug)
		printf_P(PSTR("STOP:                TWCR: %X TWSR: %X TWDR: %X\r"),twcr,twsr,twdr);

}

/******************************************************************************
*******************************************************************************
INTERRUPT-DRIVEN ACCESS   INTERRUPT-DRIVEN ACCESS   INTERRUPT-DRIVEN ACCESS   
*******************************************************************************
******************************************************************************/

unsigned char hex2chr(unsigned char v)
{
	if(v>9)
		return 'A'+v-10;
	return '0'+v;
}


/******************************************************************************
Interrupt vector
******************************************************************************/

//#define I2CDBG

ISR(TWI_vect)
{
	unsigned char twsr;
	//PORTD ^=  _BV(PORT_LED1);												// Toggle - signal some interrupt is active.


	twsr = TWSR;																// Get the I2C status
	twsr &= 0xF8;

	#ifdef I2CDBG
		uart0_putchar('I');
		uart0_putchar(hex2chr(_i2c_current_transaction->_state));
		uart0_putchar(' ');
		uart0_putchar(hex2chr(_i2c_current_transaction->_n));
		uart0_putchar(' ');
		uart0_putchar('S');
		uart0_putchar(hex2chr((TWSR>>4)));
		uart0_putchar(hex2chr((TWSR&0xf)));
		uart0_putchar(' ');
		uart0_putchar('C');
		uart0_putchar(hex2chr((TWCR>>4)));
		uart0_putchar(hex2chr((TWCR&0xf)));
		uart0_putchar(' ');
		uart0_putchar('D');
		uart0_putchar(hex2chr((TWDR>>4)));
		uart0_putchar(hex2chr((TWDR&0xf)));
		uart0_putchar('\r');
	#endif	

	
	/*if(_i2c_transaction_idle)												
	{
		TWCR&=~(1<<TWIE);											// Deactivate interrupt
		return;
	}*/

	
	


	// Process the state machine.
	switch(_i2c_current_transaction->_state)							
	{
		// ------------------------------------------------------
		// State 0:	Always called when executing the transaction.
		// 			Setup interrupt and TWI enable
		// 			Generate the start condition if needed. Otherwise, go to next state.
		// ------------------------------------------------------
		case 0:	
			#ifdef I2CDBG	
			uart0_putchar('X');
			uart0_putchar('0');
			uart0_putchar('\r');
			#endif

			_i2c_transaction_idle = 0;										// Mark as non-idle
			_i2c_current_transaction->_state++;							// Set next state
			if(_i2c_current_transaction->dostart)						// Start must be generated
			{
				
				TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE);	// Generate the start condition.
				return;															// Return - will be called back upon completion.
			}
			// Start must not be generated: flows to next state (no break)

		// ------------------------------------------------------
		// State 1:	Address state
		//				Process the return value of the start condition (if executed)
		//				Sends the address (if needed)
		// ------------------------------------------------------
		case 1:				
			#ifdef I2CDBG													
			uart0_putchar('X');
			uart0_putchar('1');
			uart0_putchar('\r');
			#endif
	
			// If we previously executed a start, check the return value
			if(_i2c_current_transaction->dostart)
			{
				if((twsr!=0x08) && (twsr!=0x10))							// Errror code isn't start and repeat start -> error
				{
					// Error. 
					_i2c_transaction_idle=1;								// Return to idle
					_i2c_current_transaction->status = 1;				// Indicate failure in start condition
					_i2c_current_transaction->i2cerror = twsr;		// Indicate I2C failure code

					TWCR&=~(1<<TWIE);											// Deactivate interrupt

					if(_i2c_current_transaction->callback)
						_i2c_current_transaction->callback(_i2c_current_transaction);
					return;
				}
			}

			_i2c_current_transaction->_state++;							// Set next state

			if(_i2c_current_transaction->doaddress)					// Address must be sent
			{
				
				TWDR = (_i2c_current_transaction->address<<1)+_i2c_current_transaction->rw;	// I2C 7-bit address + r/w#
				#ifdef I2CDBG
				uart0_putchar('D');uart0_putchar('A');uart0_putchar(hex2chr(TWDR>>4));uart0_putchar(hex2chr(TWDR&0x0f));uart0_putchar('\r');
				#endif
				TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);					// Send address
				return;
			}
			// Address must not be sent: flows to the next state (no break)


		// ------------------------------------------------------
		// State 2:	Address return state
		//				Process the return value of the address condition (if executed).
		// ------------------------------------------------------
		case 2:		
			#ifdef I2CDBG
			uart0_putchar('X');
			uart0_putchar('2');
			uart0_putchar('\r');					
			#endif
			if(_i2c_current_transaction->doaddress)					// Check the error code of address sent
			{
				if((_i2c_current_transaction->rw==I2C_WRITE && twsr!=0x18) 
					|| (_i2c_current_transaction->rw==I2C_READ && twsr!=0x40))	// Errror code isn't an acknowledge -> error
				{
					// Error.
					_i2c_transaction_idle=1;								// Return to idle
					_i2c_current_transaction->status = 2;				// Indicate failure in address
					_i2c_current_transaction->i2cerror = twsr;		// Indicate I2C failure code

					TWCR&=~(1<<TWIE);											// Deactivate interrupt

					if(_i2c_current_transaction->callback)
						_i2c_current_transaction->callback(_i2c_current_transaction);

					return;
				}
			}
			_i2c_current_transaction->_state=4;							// Set next state - state 4: data transfer
			_i2c_current_transaction->_n=0;								// Setups the number of bytes currently transferred

			// Go to the next state
			goto case4;
			break;

		// ------------------------------------------------------
		// State 3:	Check data response state. This state is entered multiple times when multiple bytes must be transferred. 
		//				Checks the return value of the data transfer
		// ------------------------------------------------------
		case 3:
			#ifdef I2CDBG
			uart0_putchar('X');
			uart0_putchar('3');
			uart0_putchar('\r');
			#endif
			
			// Check the answer to the last byte transfer	
			if( (_i2c_current_transaction->rw==I2C_WRITE && twsr!=0x28) 
					|| (_i2c_current_transaction->rw==I2C_READ && twsr!=0x50 && twsr!=0x58))
			{
				// Error.
				_i2c_transaction_idle=1;																			// Return to idle
				_i2c_current_transaction->status = 3 | ((_i2c_current_transaction->_n-1)<<4);		// Indicate failure in data, indicates number of bytes sent before error
				_i2c_current_transaction->i2cerror = twsr;													// Indicate I2C failure code

				TWCR&=~(1<<TWIE);											// Deactivate interrupt

				if(_i2c_current_transaction->callback)
					_i2c_current_transaction->callback(_i2c_current_transaction);

				return;
			}
			// If read mode, then read the data
			if(_i2c_current_transaction->rw==I2C_READ)
			{
				_i2c_current_transaction->data[_i2c_current_transaction->_n-1]=TWDR;
			}

			
			// No error, flow to next state and transmit next byte

		
		// ------------------------------------------------------
		// State 4:	Transmit data state. This state is entered multiple times when multiple bytes must be transferred. 
		//				Transmits data (if any)
		// ------------------------------------------------------
		
		case 4:
		case4:
			#ifdef I2CDBG
			uart0_putchar('X');
			uart0_putchar('4');
			uart0_putchar('\r');
			#endif
			if(_i2c_current_transaction->_n<_i2c_current_transaction->dodata)				// If data to transmit data
			{
				_i2c_current_transaction->_n++;														// Indicate one data more procesed
				_i2c_current_transaction->_state=3;													// Set next state - state 3: check data transfer answer

				// Write mode
				if(_i2c_current_transaction->rw==I2C_WRITE)													
				{
					TWDR = _i2c_current_transaction->data[_i2c_current_transaction->_n-1];
					TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);										
					return;
				}
			
				// Read mode
				if(_i2c_current_transaction->dataack[_i2c_current_transaction->_n-1])	
					TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)|(1<<TWEA);							// Set acknowledge bit
				else
					TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);											// Clear acknowledge bit

				return;
			}
				
			// All bytes transferred, or no bytes to transfer... next state.
			_i2c_current_transaction->_state++;														// Set next state
			// Flow to next state (no break)

		// ------------------------------------------------------
		// State 5:	Stop state
		//				Transmits stop condition
		// ------------------------------------------------------
		case 5:
		default:
			#ifdef I2CDBG
			uart0_putchar('X');
			uart0_putchar('5');
			uart0_putchar('\r');
			#endif
			if(_i2c_current_transaction->dostop)
				TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);												// Transmit stop, no interrupt

			// Do not wait for any answer... we've completed successfully the task
			_i2c_transaction_idle=1;																	// Return to idle
			_i2c_current_transaction->status = 0;													// Indicate success
			_i2c_current_transaction->i2cerror = 0;												// Indicate success

			if(_i2c_current_transaction->callback)
				_i2c_current_transaction->callback(_i2c_current_transaction);
	}
}

/*
	Execute the transaction.
	Two execution modes are possible: wait for completion/error and return, or call the callback upon completion/error
	In both case interrupts are used to execute the transaction
*/
unsigned char i2c_transaction_execute(I2C_TRANSACTION *trans,unsigned char blocking)
{
	// should check if a transaction is in progress and return an error if it is.
	if(!_i2c_transaction_idle)
		return 0;

	// Initialize internal stuff
	trans->_n = 0;										// Number of data bytes transferred so far
	trans->_state = 0;								// State of the transaction
	_i2c_current_transaction = trans;			// Current transaction
	_i2c_transaction_idle=0;						// Non idle

	TWI_vect();											// call the interrupt vector to execute the transaction
	if(blocking)
		while(!_i2c_transaction_idle);				// Block until the transaction is completed

	return 1;
}
unsigned char i2c_transaction_idle(void)
{
	return _i2c_transaction_idle;
}
void i2c_transaction_cancel(void)
{
	cli();
	TWCR&=~(1<<TWIE);											// Deactivate interrupt
	sei();
	// Resets the hardware - to take care of some challenging error situations
	TWCR=0;														// Deactivate i2c
	TWCR=(1<<TWEN);											// Activate i2c
	if(!_i2c_transaction_idle)								// Transaction wasn't idle -> call the callback in cancel mode
	{
		TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);		// Deactivate interrupt, generate a stop condition.
		_i2c_current_transaction->status=255;			// Mark transaction canceled
		if(_i2c_current_transaction->callback)			// Call callback if available
			_i2c_current_transaction->callback(_i2c_current_transaction);
		
	}
	_i2c_transaction_idle = 1;
	
}




