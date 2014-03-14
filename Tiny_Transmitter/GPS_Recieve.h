/*******************************************************************************
File:			GPS_Recieve.h

 				Serial functions module definitions/declarations.

Version:		1.05

*******************************************************************************/

// external function prototypes
extern void		SerInit(void);
extern void		SendByte(unsigned char chr);
extern void 	SendString(char *address);
extern void 	Serial_Processes (void);
