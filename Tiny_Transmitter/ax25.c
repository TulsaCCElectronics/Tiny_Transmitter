/*******************************************************************************

File:			ax25.C

				Routines for sending AX.25 Data.

Functions:	extern void ax25sendHeader(void);
				extern void ax25sendFooter(void);
				extern void ax25sendByte(char inbyte);
				extern void ax25crcBit(int txbyte);

				extern void ax25sendASCIIebyte(unsigned short value);
				extern void ax25sendString(char *address);
				extern void ax25sendEEPROMString(unsigned int address);

Revisions:		1.00	11/03/01 JAH	Original - John Hansen / Zack Clobes
				1.01	10/10/04	GND	Totally rewritten for AVR GNU GCC Compiler
				1.02	11/02/04	GND	Optimized for reading header out of EEPROM
				1.03	12/01/04	GND	Further optimized for tone generation
				1.04	06/23/05	GND	Converted to C++ comment style
				1.05	03/13/14	Rewritten to work with ATTINY
				
Copyright:		(c)2014, Justin D. Owen (justin.owen2@tulsacc.edu). All rights reserved.
				This software is available only for non-commercial amateur radio
				or educational applications.  All other uses are prohibited.
				This software may be modified only if the resulting code be
				made available publicly and the original author(s) given credit.				

Copyright:	(c)2005, Gary N. Dion (me@garydion.com). All rights reserved.
				This software is available only for non-commercial amateur radio
				or educational applications.  All other uses are prohibited.
				This software may be modified only if the resulting code be
				made available publicly and the original author(s) given credit.

				Many of the core functions were written for TAPR's PICe
				packet controller by John A. Hanson, W2FS (john@hansen.net).
				Some additions and modifications were made by Zack Clobes,
				W0ZC (zack@clobes.net).  I have further modified their code
				to compile on an Atmel ATmega8 using the freely available
				GNU GCC compiler, aka "WinAVR".

*******************************************************************************/

// Important! One global variable is used in this file: txtone

// OS headers
#include <avr/eeprom.h>
#include <avr/io.h>

// General purpose include files
#include "Std_Defines.h"

// App required include files
#include "ax25.h"
#include "Tiny_Transmitter.h"
#include "GPS_Recieve.h"

// Defines
#define BIT_DELAY 189					// Delay for 0.833 ms (189 for 14.7456 MHz)
#define TXDELAY 100						// Number of 6.7ms delay cycles (send flags)

// Global variables
static unsigned short	crc;

/******************************************************************************/
extern void ax25sendHeader(void)
/*******************************************************************************
* ABSTRACT:	This function keys the transmitter, sends the source and
*				destination address, and gets ready to send the actual data.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	static unsigned char	loop_delay;

	crc = 0xFFFF;							// Initialize the crc register

	// Transmit the Flag field to begin the UI-Frame
	// Adjust length for txdelay (each one takes 6.7ms)
	for (loop_delay = 0 ; loop_delay < TXDELAY ; loop_delay++)
	{
		(ax25sendByte(0x7E));
	}

	/* 		* * * THIS IS WHERE THE CALLSIGNS ARE DETERMINED * * *
	Each callsign character is shifted to use the high seven bits of the byte.
	Use the table below to determine the hex values for these characters.
	For callsigns less than six digits, pad the end of the callsign with spaces.
	For all bytes in header, except for the very last byte, bit0 must be clear.
	Yes, this means only the very last byte has bit0 set to 1 in the Station ID!

		Callsign byte lookup table
		 -----------------------------------------------------------
		| Letters:                    |   Numbers and SSID's:       |
		| A = 0x82       N = 0x9C     |   0 = 0x60       8 = 0x70   |
		| B = 0x84       O = 0x9E     |   1 = 0x62       9 = 0x72   |
		| C = 0x86       P = 0xA0     |   2 = 0x64       10 = 0x74  |
		| D = 0x88       Q = 0xA2     |   3 = 0x66       11 = 0x76  |
		| E = 0x8A       R = 0xA4     |   4 = 0x68       12 = 0x78  |
		| F = 0x8C       S = 0xA6     |   5 = 0x6A       13 = 0x7A  |
		| G = 0x8E       T = 0xA8     |   6 = 0x6C       14 = 0x7C  |
		| H = 0x90       U = 0xAA     |   7 = 0x6E       15 = 0x7E  |
		| I = 0x92       V = 0xAC     |   Space = 0x40              |
		| J = 0x94       W = 0xAE     |                             |
		| K = 0x96       X = 0xB0     |   REMEMBER!  Set bit0 in    |
		| L = 0x98       Y = 0xB2     |   the last SSID to one!     |
		| M = 0x9A       Z = 0xB4     |                             |
		 -----------------------------------------------------------
		End of lookup table */

/* To save on FLASH, the following was moved into EEPROM

	// Begin transmission of packet destination address (APxxxx0)
	ax25sendByte(0x82);						// A
	ax25sendByte(0xA0);						// P
	ax25sendByte(0x82);						// A
	ax25sendByte(0xAC);						// V
	ax25sendByte(0xA4);						// R
	ax25sendByte(0x60);						// 0
	ax25sendByte(0x60);						// SSID=0

	// Begin transmission of packet source address
	ax25sendByte(0x9C);						// Byte 1 (N)
	ax25sendByte(0x68);						// Byte 2 (4)
	ax25sendByte(0xA8);						// Byte 3 (T)
	ax25sendByte(0xB0);						// Byte 4 (X)
	ax25sendByte(0x92);						// Byte 5 (I)
	ax25sendByte(0x40);						// Byte 6 (Space)
	ax25sendByte(0x76);						// Station ID (11)

	ax25sendByte(0xA4);						// Byte 1 (R)
	ax25sendByte(0x8A);						// Byte 2 (E)
	ax25sendByte(0x98);						// Byte 3 (L)
	ax25sendByte(0x82);						// Byte 4 (A)
	ax25sendByte(0xB2);						// Byte 5 (Y)
	ax25sendByte(0x40);						// Byte 6 (Space)
	ax25sendByte(0x60);						// Station ID (0)

	ax25sendByte(0xAE);						// Byte 1 (W)
	ax25sendByte(0x92);						// Byte 2 (I)
	ax25sendByte(0x88);						// Byte 3 (D)
	ax25sendByte(0x8A);						// Byte 4 (E)
	ax25sendByte(0x64);						// Byte 5 (2)
	ax25sendByte(0x40);						// Byte 6 (Space)
	ax25sendByte(0x65);						// Station ID (2)

	// Finish out the header with two more bytes
	ax25sendByte(0x03);						// Control field - 0x03 is APRS UI-frame
	ax25sendByte(0xF0);						// Protocol ID - 0xF0 is no layer 3
*/

//	ax25sendEEPROMString(0);				// Send the header for use on 144.39 MHz
	ax25sendEEPROMString(31);				// Trimmed header for use in 144.34 MHz
	return;

}		// End ax25sendHeader(void)


/******************************************************************************/
extern void ax25sendFooter(void)
/*******************************************************************************
* ABSTRACT:	This function closes out the packet with the check-sum and a
*				final flag.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	static unsigned char	crchi;

	crchi = (crc >> 8)^0xFF;
	ax25sendByte(crc^0xFF); 				// Send the low byte of the crc
	ax25sendByte(crchi); 					// Send the high byte of the crc
	ax25sendByte(0x7E);			  			// Send a flag to end the packet
	return;

}		// End ax25sendFooter(void)


/******************************************************************************/
extern void ax25sendByte(unsigned char txbyte)
/*******************************************************************************
* ABSTRACT:	This function sends one byte by toggling the "tone" variable.
*
* INPUT:		txbyte	The byte to transmit
* OUTPUT:	None
* RETURN:	None
*/
{
	static char	loop;
	static char	bitbyte;
	static int	bit_zero;
	static unsigned char	sequential_ones;

	bitbyte = txbyte;							// Bitbyte will be rotated through

	for (loop = 0 ; loop < 8 ; loop++)	// Loop for eight bits in the byte
	{
		bit_zero = bitbyte & 0x01;			// Set aside the least significant bit

		if (txbyte == 0x7E)					// Is the transmit character a flag?
		{
			sequential_ones = 0;				// it is immune from sequential 1's
		}
		else										// The transmit character is not a flag
		{
			(ax25crcBit(bit_zero));			// So modify the checksum
		}

		if (!(bit_zero))						// Is the least significant bit low?
		{
			sequential_ones = 0;				// Clear the number of ones we have sent
			txtone = (txtone == MARK)? SPACE : MARK; // Toggle transmit tone
		}
		else										// Else, least significant bit is high
		{
			if (++sequential_ones == 5)	// Is this the 5th "1" in a row?
			{
				mainDelay(BIT_DELAY);		// Go ahead and send it
				txtone = (txtone == MARK)? SPACE : MARK; // Toggle transmit tone
				sequential_ones = 0;			// Clear the number of ones we have sent
			}

		}

		bitbyte >>= 1;							// Shift the reference byte one bit right
		mainDelay(BIT_DELAY);				// Pause for the bit to be sent
	}

	return;

}		// End ax25sendByte(unsigned char txbyte)


/******************************************************************************/
extern void ax25crcBit(int lsb_int)
/*******************************************************************************
* ABSTRACT:	This function takes the latest transmit bit and modifies the crc.
*
* INPUT:		lsb_int	An integer with its least significant bit set of cleared
* OUTPUT:	None
* RETURN:	None
*/
{
	static unsigned short	xor_int;

	xor_int = crc ^ lsb_int;				// XOR lsb of CRC with the latest bit
	crc >>= 1;									// Shift 16-bit CRC one bit to the right

	if (xor_int & 0x0001)					// If XOR result from above has lsb set
	{
		crc ^= 0x8408;							// Shift 16-bit CRC one bit to the right
	}

	return;

}		// End ax25crcBit(int lsb_int)


/******************************************************************************/
extern void ax25sendASCIIebyte(unsigned short value)
/*******************************************************************************
* ABSTRACT:	This function sends an unsigned "extended" byte using ASCII.
*				"Extended" means that it is 10-bits only (values up to 999.)
*
* INPUT:		value		The "ebyte" to be converted into ASCII and sent.
* OUTPUT:	None
* RETURN:	None
*/
{
	static char count;

	if (value > 999) value = 999;

	count = 0;
	while (value >= 100)
	{
		value -= 100;
		count++;
	}
	ax25sendByte(count + 48);

	count = 0;
	while (value >= 10)
	{
		value -= 10;
		count++;
	}
	ax25sendByte(count + 48);
	ax25sendByte(value + 48);
	return;

}		// End ax25sendASCIIebyte(unsigned short value)


/******************************************************************************/
extern void ax25sendString(char *address)
/*******************************************************************************
* ABSTRACT:	This function sends a null-terminated string to the packet
*
* INPUT:		*szString	Pointer to string to send
* OUTPUT:	None
* RETURN:	None
*/
{
	while (*address != 0)
	{
		ax25sendByte(*address);
		address++;
	}

	return;

}		// End ax25sendString(char *address)


/******************************************************************************/
extern void ax25sendEEPROMString(unsigned int address)
/*******************************************************************************
* ABSTRACT:	This function sends a null-terminated string found in EEPROM
*				at the address given
*
* INPUT:		address	Starting address for the string
* OUTPUT:	None
* RETURN:	None
*/
{
	static unsigned char temp_char;

	temp_char = eeprom_read_byte ((uint8_t *)(address));
	while (temp_char)
	{
		ax25sendByte(temp_char);
		temp_char = eeprom_read_byte ((uint8_t *)(++address));
	}

	return;

}		// End ax25sendEEPROMString(unsigned int address)
