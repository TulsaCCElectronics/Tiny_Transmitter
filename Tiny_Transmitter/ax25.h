/*******************************************************************************
File:			ax25.h

				Routines for sending AX.25 Data.

Version:		1.5

*******************************************************************************/

#define	MARK (167)  						// 167 - works from 190 to 155 (1200 Hz.)
#define	SPACE (209) 						// 213 - works from 204 to 216 (2200 Hz.)

// external variables
unsigned char	txtone;						// Used in main.c SIGNAL(SIG_OVERFLOW0)

// external function prototypes
extern void ax25sendHeader(void);
extern void ax25sendFooter(void);
extern void ax25sendByte(unsigned char inbyte);
extern void ax25crcBit(int txbyte);

extern void ax25sendASCIIebyte(unsigned short value);
extern void ax25sendString(char *szString);
extern void ax25sendEEPROMString(unsigned int address);
