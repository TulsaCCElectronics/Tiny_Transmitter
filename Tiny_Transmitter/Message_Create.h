/*
 * Message_Create.h
 *
 * Created: 3/14/2014 12:03:11 PM
 *  Author: justin
 *
 * Messaging definitions/declarations for the AtTiny4313.
 *
 * Version		1.5
 */ 


unsigned char	Time_Temp[7]; 	// Temp variable here so main can read seconds

extern void MsgInit (void);
extern void MsgPrepare (void);
extern void MsgSendPos (void);
extern void MsgSendTelem (void);
extern void MsgSendAck (unsigned char *rxbytes, unsigned char msg_start);
extern void MsgHandler (unsigned char newchar);
