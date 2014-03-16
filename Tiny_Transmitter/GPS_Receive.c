/*******************************************************************************
File:			GPS_Recieve.c

				Serial I/O subsystem function library.

Functions:	extern void		SerInit(void)
				extern void		SendByte(unsigned char chr)
				extern void 	SendString(char *address)
				extern void 	Serial_Processes(void)
				SIGNAL(SIG_UART_RECV)
				SIGNAL(SIG_UART_TRANS)

Revisions:		1.00	04/14/03	GND	Original - Gary N. Dion
				1.01	11/01/04	GND	Modified for ISR based transmit
				1.02	11/02/04	GND	Optimized the ASCII routine (later removed)
				1.03	05/26/05	GND	Converted to C++ comment style
				

Copyright:	(c)2005, Gary N. Dion (me@garydion.com). All rights reserved.
				This software is available only for non-commercial amateur radio
				or educational applications.  ALL other uses are prohibited.
				This software may be modified only if the resulting code be
				made available publicly and the original author(s) given credit.

*******************************************************************************/

// OS headers
#include <avr/io.h>
#include <avr/interrupt.h>

// General purpose include files
#include "Std_Defines.h"

// App required include files
#include "Message_Create.h"
#include "GPS_Receive.h"

#define	BUF_SIZE		(96)					// Educated guess for a good buffer size

static unsigned char inbuf[BUF_SIZE];	// USART input buffer array
static unsigned char inhead;				// USART input buffer head pointer
static unsigned char intail;				// USART input buffer tail pointer
static unsigned char outbuf[BUF_SIZE];	// USART output buffer array
static unsigned char outhead;				// USART output buffer head pointer
static unsigned char outtail;				// USART output buffer tail pointer


/******************************************************************************/
extern void		SerInit(void)
/*******************************************************************************
* ABSTRACT:	This function initializes the serial USART.
*				It sets up the baud rate, data and stop bits, and parity
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	// Set baud rate of USART to 4800 baud at 14.7456 MHz
	UBRRH = 0;
	UBRRL = 191;

	// Set frame format to 8 data bits, no parity, and 1stop bit
	UCSRC = (1<<UMSEL)|(3<<UCSZ0);

	// Enable Receiver and Transmitter Interrupt, Receiver and Transmitter
	UCSRB = (1<<RXCIE)|(1<<TXCIE)|(1<<RXEN)|(1<<TXEN);
	return;

}		// End SerInit(void)


/******************************************************************************/
extern void		SendByte(unsigned char chr)
/*******************************************************************************
* ABSTRACT:	This function pushes a new character into the output buffer
*				then pre-advances the pointer to the next empty location.
*
* INPUT:		chr			byte to send
* OUTPUT:	None
* RETURN:	None
*/
{
	if (++outhead == BUF_SIZE) outhead = 0;	// Advance and wrap pointer
	outbuf[outhead] = chr;		 			// Transfer the byte to output buffer

	if (UCSRA & (1<<UDRE))					// If the transmit buffer is empty
	{
		if (++outtail == BUF_SIZE) outtail = 0;// Advance and wrap pointer
		UDR = outbuf[outtail];				// Place the byte in the output buffer
	}

	return;

}		// End SendByte(unsigned char chr)


/******************************************************************************/
extern void SendString(char *address)
/*******************************************************************************
* ABSTRACT:	This function sends a null-terminated string to the serial port.
*
* INPUT:		*szString	Pointer to string to send
* OUTPUT:	None
* RETURN:	None
*/
{
	while (*address != 0)					// While not to the string end yet
	{
		SendByte(*(address++));				// Send the byte and increment index
	}

	return;

}		// End SendString(char *address)


/******************************************************************************/
extern void 	Serial_Processes(void)
/*******************************************************************************
* ABSTRACT:	Called by main.c during idle time. Processes any waiting serial
*				characters coming in or going out both serial ports.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	if (intail != inhead)					// If there are incoming bytes pending
	{
		if (++intail == BUF_SIZE) intail = 0;	// Advance and wrap pointer
		MsgHandler(inbuf[intail]);		// And pass it to a handler
	}

	return;

}		// End Serial_Processes(void)


/******************************************************************************/
SIGNAL(SIG_UART_RECV)
/*******************************************************************************
* ABSTRACT:	Called by the receive ISR (interrupt). Saves the next serial
*				byte to the head of the RX buffer.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	if (++inhead == BUF_SIZE) inhead = 0;	// Advance and wrap buffer pointer
	inbuf[inhead] = UDR;	  					// Transfer the byte to the input buffer
	return;

}		// End SIGNAL(SIG_UART_RECV)


/******************************************************************************/
SIGNAL(SIG_UART_TRANS)
/*******************************************************************************
* ABSTRACT:	Called by the transmit ISR (interrupt). Puts the next serial
*				byte into the TX register.
*
* INPUT:		None
* OUTPUT:	None
* RETURN:	None
*/
{
	if (outtail != outhead)					// If there are outgoing bytes pending
	{
		if (++outtail == BUF_SIZE) outtail = 0;// Advance and wrap pointer
		UDR = outbuf[outtail];				// Place the byte in the output buffer
	}

	return;

}		// End SIGNAL(SIG_UART_TRANS)
