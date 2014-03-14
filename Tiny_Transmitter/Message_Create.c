/*******************************************************************************
File:			Message_Create.c

 				Messaging subsystem function library for the atmega8.

Functions:	extern void MsgInit (void)
				extern void MsgPrepare (void)
				extern void MsgSendPos (void)
				extern void MsgSendTelem (void)
		extern void MsgSendAck (unsigned char *rxbytes, unsigned char msg_start)
				extern void SerHandler (unsigned char newchar);

Revisions:	1.00	11/02/04	GND	Gary Dion
				1.01	11/28/04	GND	Added MsgSendAck routine
				1.02	06/23/05	GND	Converted to C++ comment style and cleaned up
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

// General purpose include files
#include "Std_Defines.h"

// App required include files.
#include "ax25.h"
#include "Message_Create.h"
#include "GPS_Recieve.h"

#define	GPRMC		(1)
#define	GPGGA		(2)

static unsigned char	Time[7];				// UTC time in HHMMSS format
static unsigned char	Latitude[10];		// Latitude in DDMM.MMMM format
static unsigned char	Longitude[11];		// Longitude in DDDMM.MMMM format
static unsigned char	Altitude[8];		// Altitude (meters) in MMM.MMM format
static unsigned char	Speed[6];			// Speed (knots) in kkk.kk format
static unsigned char	Course[6];			// Track angle (degrees) in ddd.dd format
static unsigned char	Satellites[3];		// Number of Satellites tracked

// Temporary variables used for decoding. Note: Time_Temp[7] is in the .h file.
static unsigned char	Latitude_Temp[10];
static unsigned char	Longitude_Temp[11];
static unsigned char	Altitude_Temp[8];
static unsigned char	Speed_Temp[6];
static unsigned char	Course_Temp[6];
static unsigned char	Satellites_Temp[3];

static unsigned char	Altifeet[7];		// Altitude (feet) in FFFFFF format

static unsigned char	sentence_type;		// GPRMC, GPGGA, or unrecognized


/******************************************************************************/
extern void MsgInit (void)
/*******************************************************************************
* ABSTRACT:	Initialize some of the fields in case we transmit before the GPS
*				has lock and sends us valid data.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	Speed_Temp[0] = Speed_Temp[1] = Speed_Temp[2] = '0';
	Course_Temp[0] = Course_Temp[1] = Course_Temp[2] = '0';
	Altitude_Temp[0] = Altitude_Temp[1] = '0';
	Altitude_Temp[2] = '.';
	return;

}		// End MsgInit

/******************************************************************************/
extern void MsgPrepare(void)
/*******************************************************************************
* ABSTRACT:	Call this function right before sending a position report for two
*				reasons. This copies all the temp strings into transmit strings so
*				they are not modified by the GPS receive handler.  Altitude is also
*				converted into feet from meters.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	static unsigned long	LongAltitude;	// Used to convert meters to feet
	static unsigned long	LongTemp;		// Just a long temp variable
	static unsigned char	index;			// For indexing local arrays
	static unsigned char	count;			// Keeps track of loops	in F-to-A

	for (index = 0; index < 6 ; index++)			// Grab latest Time
		Time[index] = Time_Temp[index];
	Time[index] = 0; 										// Terminate string

	for (index = 0; index < 9 ; index++)			// Grab latest Latitude
		Latitude[index] = Latitude_Temp[index];
	Latitude[index] = 0;									// Terminate string

	for (index = 0; index < 10 ; index++)			// Grab latest Longitude
		Longitude[index] = Longitude_Temp[index];
	Longitude[index] = 0;								// Terminate string

	for (index = 0; index < 7 ; index++)			// Grab latest Altitude
		Altitude[index] = Altitude_Temp[index];
	Altitude[index] = 0;									// Terminate string

	for (index = 0; index < 5 ; index++)			// Grab latest Speed
		Speed[index] = Speed_Temp[index];
	Speed[index] = 0;										// Terminate string

	for (index = 0; index < 5 ; index++)			// Grab latest Course
		Course[index] = Course_Temp[index];
	Course[index] = 0;									// Terminate string

	for (index = 0; index < 2 ; index++)			// Grab latest Satellites
		Satellites[index] = Satellites_Temp[index];
	Satellites[index] = 0;								// Terminate string

	index = 0;									// Reset index for this search
	Altitude[6] = '.';						// Force last character to a '.'
	while (Altitude[++index] != '.');	// Find the decimal in Alt string

	LongAltitude = 0;							// Begin with a blank slate
	while (index)			// This is Float-to-A, working from the decimal leftward
	{
		LongAltitude += (Altitude[--index] - 48) * LongTemp; // Right to left
		LongTemp *= 10;						// Each digit is worth 10x previous
	}
	// The LongAltitude variable now contains the altitude in meters.

	// The following is an approximation of 3.28 to convert Meters to Feet
	LongAltitude *= 3;						// Start by multiplying by 3
	LongAltitude += LongAltitude>>4;		// add to self/16  (3.1875)
	LongAltitude += LongAltitude>>6;		// add to self/64  (3.2373)
	LongAltitude += LongAltitude>>7;		// add to self/128 (3.2626)
	LongAltitude += LongAltitude>>8;		// add to self/256 (3.2758)
	LongAltitude += LongAltitude>>10;	// add to self/1024 (3.279)
	// The LongAltitude variable now contains the altitude in feet.

	// This converts a long to ASCII with six characters & leading zeros.
	count = 0;									// Convert to character each cycle
	index = 0;									// Start on left and work right
	for (LongTemp = 100000 ; LongTemp > 1; LongTemp /= 10)
	{
		while (LongAltitude >= LongTemp)	// Convert each order of 10 to a #
		{
			LongAltitude -= LongTemp;		// By looping, sub'ing each time
			count++;								// Keep track of iteration loops
		}
		Altifeet[index++] = count + 48;	// Save that count as a character
		count = 0;								// Reset count and start over
	}

	Altifeet[index++] = LongAltitude + 48;	// Last digit resides in LongAlt...
	Altifeet[index] = 0;						// Terminate string
	return;

}		// End MsgPrepare(void)


/******************************************************************************/
extern void MsgSendPos(void)
/*******************************************************************************
* ABSTRACT:	Send an APRS formatted message containing timestamped position data,
*				a symbol, the course, speed, altitude, and # of satellites received.
*				I also added two channels of analog data for temperature & voltage.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	ax25sendByte('@');						// The "@" Symbol means time stamp first
	ax25sendString(Time);					// Send the time
	ax25sendByte('z');						// Tag it as zulu
	Latitude[7] = 0;							// Truncate Latitude past 7th character
	ax25sendString(Latitude);				// Send it
	ax25sendByte('N');						// As degrees North
	ax25sendByte('/');						// Symbol Table Identifier
	Longitude[8] = 0;							// Truncate Longitude past 8th character
	ax25sendString(Longitude);				// Send it
	ax25sendByte('W');						// As degrees West
	ax25sendByte('O');						// Symbol Code for a balloon icon
	Course[3] = 0;								// Truncate course at 3 characters
	ax25sendString(Course);					// Transmit Course
	ax25sendByte('/');						// Just a separator with no meaning
	Speed[3] = 0;								// Truncate speed at 3 characters
	ax25sendString(Speed);					// Transmit Speed

	// Begin Comment - up to 36 characters are permissable
	ax25sendByte('/');						// A little more data...
	ax25sendByte('A');						// ...to be interpreted as altitude...
	ax25sendByte('=');						// ...starts right now
	ax25sendString(Altifeet);				// Send Altitude in feet
	ax25sendByte(' ');						// Space for formatting
	// Send number of Satellites tracked in HEX
	if (Satellites[0] == '1') Satellites[1] += 17; // '0' becomes 'A'...
	ax25sendByte(Satellites[1]);
	ax25sendByte(' ');						// Space for formatting
	//ax25sendASCIIebyte(ADCGet(0) + 7);	// Send analog channel 0
	//ax25sendByte(' ');						// Space for formatting
	//ax25sendASCIIebyte((ADCGet(1)<<1) + 33);	// Send analog channel 1
	return;

}		// End MsgSendPos(void)


/******************************************************************************/
extern void MsgSendTelem(void)
/*******************************************************************************
* ABSTRACT:	Send an APRS formatted message containing telemetry.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	static unsigned char	sequence;		// Telemetry sequence number
	static unsigned char	temp;				// To loop analog / digital channels

	ax25sendByte('T');						// Data type identifier for telemetry
	ax25sendByte('#');						// "#" for sequence number
	ax25sendASCIIebyte(sequence++);		// Transmit the sequence number...
	ax25sendByte(',');						// First comma preceding analog values

	for (temp = 1 ; temp < 6 ; temp++)	// For analog channels 1 through 5...
	{
		ax25sendASCIIebyte(ADCGet(temp));// Send their values in ASCII
		ax25sendByte(',');					// A comma following each analog value
	}

	// Send eight digital channels
	ax25sendByte((PIND & 1<<1)? '1' : '0');	// Check pin & choose character
	ax25sendByte((PIND & 1<<2)? '1' : '0');	// Check pin & choose character
	ax25sendByte((PIND & 1<<3)? '1' : '0');	// Check pin & choose character
	ax25sendByte((PIND & 1<<4)? '1' : '0');	// Check pin & choose character
	ax25sendByte((PIND & 1<<5)? '1' : '0');	// Check pin & choose character
	ax25sendByte((PIND & 1<<6)? '1' : '0');	// Check pin & choose character
	ax25sendByte((0)? '1' : '0');					// TBD & choose character
	ax25sendByte((0)? '1' : '0');					// TBD & choose character

	// Begin Comment - up to TBD characters are permissable
	ax25sendByte(',');						// Another comma so that I can...
	ax25sendASCIIebyte(ADCGet(0));		// Send analog channel 0 in ASCII
	ax25sendByte(',');						// Another comma so that I can...
	ax25sendString(Time);					// Send the time
	return;

}		// End MsgSendTelem(void)


/******************************************************************************/
extern void MsgHandler(unsigned char newchar)
/*******************************************************************************
* ABSTRACT:	Processes the characters coming in from USART.  In this case,
*				this is the port connected to the gps receiver.
*
* INPUT:		newchar	Next character from the serial port.
* OUTPUT:	None
* RETURN:	None
*/
{
	static unsigned char	commas;			// Number of commas for far in sentence
	static unsigned char	index;			// Individual array index

	if (newchar == 0)							// A NULL character resets GPS decoding
	{
		commas = 25;							// Set to an outrageous value
		sentence_type = FALSE;				// Clear local parse variable
		return;
	}

	if (newchar == '$')						// Start of Sentence character, reset
	{
		commas = 0;								// No commas detected in sentence for far
		sentence_type = FALSE;				// Clear local parse variable
		return;
	}

	if (newchar == ',')						// If there is a comma
	{
		commas += 1;							// Increment the comma count
		index = 0;								// And reset the field index
		return;
	}

	if (commas == 0)
	{
		switch(newchar)
		{
			case ('C'):							// Only the GPRMC sentence contains a "C"
				sentence_type = GPRMC;		// Set local parse variable
				break;
			case ('S'):							// Take note if sentence contains an "S"
				sentence_type = 'S';			// ...because we don't want to parse it
				break;
			case ('A'):							// The GPGGA sentence ID contains "A"
				if (sentence_type != 'S')	// As does GPGSA, which we will ignore
					sentence_type = GPGGA;	// Set local parse variable
				break;
		}

		return;
	}

	if (sentence_type == GPGGA)			// GPGGA sentence	decode initiated
	{
		switch (commas)
		{
			case (1):									// Time field, grab characters
				Time_Temp[index++] = newchar;
				return;
			case (2):									// Latitude field, grab chars
				Latitude_Temp[index++] = newchar;
				return;
			case (4):									// Longitude field, grab chars
				Longitude_Temp[index++] = newchar;
				return;
			case (7):									// Satellite field, grab chars
				Satellites_Temp[index++] = newchar;
				return;
			case (9):									// Altitude field, grab chars
				Altitude_Temp[index++] = newchar;
				return;
		}

		return;
	}		// end if (sentence_type == GPGGA)

	if (sentence_type == GPRMC)			// GPGGA sentence	decode initiated
	{
		switch (commas)
		{
			case (7):									// Speed field, grab characters
				Speed_Temp[index++] = newchar;
				return;
			case (8):									// Course field, grab characters
				Course_Temp[index++] = newchar;
				return;
		}

		return;
	}		// end if (sentence_type == GPRMC)

	return;

}		// End MsgHandler(unsigned char newchar)
