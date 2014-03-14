/*
 * Sine_Gen.c
 *
 * Created: 3/8/2014 9:04:11 PM
 *  Author: justin
 */ 

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

volatile unsigned char delay;
volatile char busy;	

/******************************************************************************/
extern int	main(void)
/*******************************************************************************
* ABSTRACT:	Main program entry function.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/

PORTB = 0x00;

// Initialize Timer0 for 1.8432MHz
TCCR0A = 0x02;

// Enable Timer0 interrupt
TIMSK0 = (1<<TOIE0);
sei();

	while (1)
	{
		//	Might Need Delay Here			// Wait for break (not on balloons!!!)
		MsgPrepare();							// Prepare variables for APRS position
		mainTransmit();
		
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

		}
		
	return(1);
}

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
	TCCR0 = 0x03; 								// Timer0 clock prescale of 64
	TCCR1B = 0x02;								// Timer1 clock prescale of 8
	TCCR2 = 0x02; 								// Timer2 clock prescale of 8
	transmit = TRUE;							// Enable the transmitter
	ax25sendHeader();							// Send APRS header
	return;

}		// End mainTransmit(void)

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
	TCNT2 = 255 - timeout;				// Set desired delay
	while(delay)
	{
		Serial_Processes();				// Do this until cleared by interrupt
	}

	return;

}		// End Delay(unsigned char timeout)


/******************************************************************************/
SIGNAL(SIG_OVERFLOW2)
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
	static char	sine[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	static unsigned char sine_index;		// Index for the D-to-A sequence

	if (transmit)
	{
		++sine_index;							// Increment index
		sine_index &= 15;						// And wrap to a max of 15
		PORTB = sine[sine_index];			// Load next D-to-A sinewave value
		TCNT2 = txtone;						// Preload counter based on freq.
	}
	else
	{
		delay = FALSE;							// Clear condition holding up Delay
		TCNT2 = 0;								// Make long as possible delay
	}

}		// End SIGNAL(SIG_OVERFLOW2)
