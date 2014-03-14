/*******************************************************************************
File:			Tiny_Transmitter.c

				Main WhereAVR function library.

Functions:	extern int	main(void)
				extern void mainTransmit(void)
				extern void mainReceive(void)
				extern void ax25rxByte(unsigned char rxbyte)
				extern void mainDelay(unsigned int timeout)
				extern void Delay(unsigned int timeout)
				SIGNAL(SIG_OVERFLOW0)
				SIGNAL(SIG_OVERFLOW1)
				SIGNAL(SIG_OVERFLOW2)
				SIGNAL(SIG_COMPARATOR)

Created:		1.00	10/05/04	GND	Gary Dion

Revisions:	1.01	11/02/04	GND	Major modification on all fronts
				1.02	12/01/04	GND	Continued optimization
				1.03	06/23/05	GND	Converted to C++ comment style and cleaned up
				1.05	03/13/14	Rewritten to work with ATTINY
				
Copyright:		(c)2014, Justin D. Owen (justin.owen2@tulsacc.edu). All rights reserved.
				This software is available only for non-commercial amateur radio
				or educational applications.  All other uses are prohibited.
				This software may be modified only if the resulting code be
				made available publicly and the original author(s) given credit.

Copyright:	(c)2005, Gary N. Dion (me@garydion.com). All rights reserved.
				This software is available only for non-commercial amateur radio
				or educational applications.  ALL other uses are prohibited.
				This software may be modified only if the resulting code be
				made available publicly and the original author(s) given credit.

*******************************************************************************/

// OS headers
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/signal.h>

// General purpose include files
#include "Std_Defines.h"

// App required include files
#include "ax25.h"
#include "Tiny_Transmitter.h"
#include "Message_Create.h"
#include "GPS_Recieve.h"

#define	RXSIZE (256)

#define	WatchdogReset() asm("wdr")

// Static Functions and Variables
volatile unsigned char delay;				// State of Delay function
volatile unsigned char maindelay;		// State of mainDelay function
static unsigned char msg_start;			// Index of message start after header
static unsigned char	msg_end;				// Index of message ending character
static unsigned char	command;				// Used just for toggling
static unsigned char	transmit;			// Keeps track of TX/RX state
static unsigned short crc;					// Current checksum for incoming message
static unsigned short dcd;					// Carrier detect of sorts
volatile char busy;							// Carrier detect of sorts

/******************************************************************************/
extern int	main(void)
/*******************************************************************************
* ABSTRACT:	Main program entry function.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	static unsigned short loop;			// Generic loop variable
	static unsigned char	ones_seconds;	// Remembers tens digit of seconds
	static unsigned char seconds;			// Holds seconds calculated from GPS

	//Initialize serial communication functions
	SerInit();
	MsgInit();
	
	// PORT B - Sinewave Generation, and
	//	Bit/Pin 5 (out) connected to a 1k ohm resistor
	//	Bit/Pin 4 (out) connected to a 2k ohm resistor
	//	Bit/Pin 3 (out) connected to a 3.9k ohm resistor
	//	Bit/Pin 2 (out) connected to an 8.2k ohm resistor
	//	Bit/Pin 1 (out) connected to the PTT circuitry
	//	Bit/Pin 0 (out) DCD LED line
	PORTB = 0x00;							// Initial state is everything off
	DDRB  = 0x3F;							// Data direction register for port B

	//	Initialize the 8-bit Timer0 to clock at 1.8432 MHz
	TCCR0A = 0x07; 							// Timer0 clock prescale of 8. This takes the place of timer 2 in Gary's original code

	// Use the 16-bit Timer1 to measure frequency; set it to clock at 1.8432 MHz
	TCCR1B = 0x02;							// Timer2 clock prescale of 8. This is still going to be timer 1. The reciever timer is not needed. 

	
	// Enable Timer interrupts
	TIMSK = 1<<TOIE0 | 1<<TOIE1; // 

	// Enable the watchdog timer
	WDTCR	= (1<<WDCE) | (1<<WDE);		// Wake-up the watchdog register
	WDTCR	= (1<<WDE) | 7;				// Enable and timeout around 2.1s

	// Enable interrupts
	sei();

	// Reset watchdog
	WatchdogReset();
	
while (TRUE)
{
	//		txtone = SPACE;						// Debug tone for testing (MARK or SPACE)
	//		while(1) WatchdogReset();			// Debug with a single one tone
	//		while(1) ax25sendByte(0);			// Debug with a toggling tone
	Delay(250);
	Delay(250);
	Delay(250);
	Delay(250);
	Delay(250);
	//		while(busy)	Delay(250);			// Wait for break (not on balloons!!!)
	MsgPrepare();							// Prepare variables for APRS position
	mainTransmit();						// Enable transmitter

	if (command == 0)						// Default message to be sent
	{
		MsgSendPos();						// Send Position Report and comment
	}
	else
	{
		if (command == 'S')
		{
			ax25sendEEPROMString(48);	// Send ">See garydion.com"
		}

		if (command == 'T')
		{
			MsgSendTelem();				// Send Telemetry and comment
		}
return(1);
	} 
}
} // End Main

/******************************************************************************/
extern void mainTransmit(void)
/*******************************************************************************
* ABSTRACT:	Do all the setup to transmit.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	UCSRB &= ~((1<<RXCIE)|(1<<TXCIE));	// Disable the serial interrupts
	ACSR &= ~(1<<ACIE);						// Disable the comparator
	TCCR0A = 0x03; 								// Timer0 clock prescale of 64
	TCCR1B = 0x02;								// Timer1 clock prescale of 8
	transmit = TRUE;							// Enable the transmitter
	ax25sendHeader();							// Send APRS header
	return;

}		// End mainTransmit(void)

/******************************************************************************/
extern void mainDelay(unsigned char timeout)
/*******************************************************************************
* ABSTRACT:	This function sets "maindelay", programs the desired delay,
*				and takes care of incoming serial characters until it's cleared.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	maindelay = TRUE;							// Set the condition variable
	WatchdogReset();							// Kick the dog before we start
	TCNT0 = 255 - timeout;					// Set desired delay
	while(maindelay)
	{
		Serial_Processes();					// Do this until cleared by interrupt
	}

	return;

}		// End mainDelay(unsigned int timeout)

/******************************************************************************/
extern void Delay(unsigned char timeout)
/*******************************************************************************
* ABSTRACT:	This function sets "delay", programs the desired delay,
*				and takes care of incoming serial characters until it's cleared.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	delay = TRUE;							// Set the condition variable
	WatchdogReset();						// Kick the dog before we start
	TCNT0 = 255 - timeout;				// Set desired delay
	while(delay)
	{
		Serial_Processes();				// Do this until cleared by interrupt
	}

	return;

}		// End Delay(unsigned char timeout)

/******************************************************************************/
SIGNAL(SIG_OVERFLOW0)
/*******************************************************************************
* ABSTRACT:	This function handles the counter2 overflow interrupt.
*				Counter2 is used to generate a sine wave using resistors on
*				Pins B5-B1. Following are the sixteen 4-bit sinewave values:
*							7, 10, 13, 14, 15, 14, 13, 10, 8, 5, 2, 1, 0, 1, 2, 5.
*				If in receive mode, the counter is pre-loaded with a long delay
*				and the delay variable is cleared.
*
*		 !!!Important!!! This code is -optimized- for the least # of clock cycles.
*				If you modify it, PLEASE be sure you know what you're doing!
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	// This line is for if you followed the schematic:
	static char	sine[16] = {58,22,46,30,62,30,46,22,6,42,18,34,2,34,18,42};
	// This line is for if you installed the resistors in backwards order :-) :
	//	static char	sine[16] = {30,42,54,58,62,58,54,42,34,22,10,6,2,6,10,22};
	static unsigned char sine_index;		// Index for the D-to-A sequence

	if (transmit)
	{
		++sine_index;							// Increment index
		sine_index &= 15;						// And wrap to a max of 15
		PORTB = sine[sine_index];			// Load next D-to-A sinewave value
		TCNT0 = txtone;						// Preload counter based on freq.
	}
	else
	{
		delay = FALSE;							// Clear condition holding up Delay
		TCNT0 = 0;								// Make long as possible delay
	}
	

}		// End SIGNAL(SIG_OVERFLOW2)