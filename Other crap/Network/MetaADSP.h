#ifndef _APPLETALK_NETWORK_H_
#define _APPLETALK_NETWORK_H_

#include <Errors.h>		// for network error codes

/*====================================================================================*/
/*
	Calling Sequence Example
	~~~~~~~~~~~~~~~~~~~~~~~~	
	// client and server start network with their unique names
	Client & Server: 	StartNetwork(SERVER_NAME_STR32, SERVER_TYPE_NAME_STR32);
						StartNetwork(CLIENT_NAME_STR32, CLIENT_TYPE_NAME_STR32);

	Client & Server (optional): SetNetworkOptions(...);	// optional call

	// server waits for connection from client
	// This routine *must* be called before the client calls OpenNetworkConnection
	Server: 	WaitForNetworkConnection(WAIT_FOREVER);
	
	// client opens connection with the *server's* name and type
	// note that server must previously have called WaitForNetworkConnection()
	Client: 	OpenNetworkConnection(SERVER_NAME_STR32, SERVER_TYPE_NAME_STR32);
		
	// in main event loop
	Client & Server: 	NetworkIdle(...);

	// connection now opened, read and write data back and forth
	Client & Server: 	ReadData(...); WriteData(...); ReadData(...); WriteData(...)

	// client and/or server closes the connection and stops network
	Client & Server: 	CloseNetworkConnection();
	Client & Server: 	StopNetwork();
*/

/*====================================================================================*/
/* ROUTINE PROTOTYPES */
/*====================================================================================*/


/*====================================================================================*/
/*
	These routines return true if the network has been started (i.e. StartNetwork()
	has been called, and completed successfully, and StopNetwork() has not been called) 
	and the connection has been opened (i.e. WaitForConnection()/OpenNetworkConnection() 
	has been called and completed successfully, and CloseConnection() has not been
	called).  These may not reflect the true state of the connection, especially since
	these are merely globals set by the routines named, and a connection can close
	underneath us for a variety of reasons, but they are good for fast tests of the
	connection.
*/
Boolean IsNetworkStarted(void);
Boolean IsConnectionOpened(void);


/*====================================================================================*/
/*
	This routine initializes the network code.  It should be called once at startup.
	Pass in Pascal strings describing the name and type of this network process.
	These strings are looked for in the OpenNetworkConnection routine below.  Note that
	the strings must be unique across the zone, so the server will register its
	names and call WaitForNetworkConnection, while the client will register *different*
	names and call OpenNetworkConnection with the *server's* names.  
	
	theName and theType must be *AT MOST* 32 bytes long.  That is, they must be 
	Str32's.  They must also be *Pascal* strings.  If the strings do not fit
	these descriptions, bad things will most likely happen.
	
	Pass DEFAULT_QUEUE_SIZE for queue_size if you want the default send/receive 
	queue size, appropriate for most applications.  Pass larger values or smaller 
	depending on your needs.  See IM-Networking page 5-13, number (2).  Note that
	queue_size must be at least minDSPQueueSize, which is defined in <ADSP.h>. 
	
	This routine can take several seconds to execute.  This is because the 
	Apple routine PRegisterName() is extremely slow.
	
	If an error occurs, it returns the error code.
*/
OSErr StartNetwork(Str32 theName, 			// name to give this socket 
					Str32 theType,			// type to give this socket
					UInt32 queue_size);		// size of the queue to use


/*====================================================================================*/
/*
	This routine closes down the network code.  It should be called once at the end
	of the program.  
	If an error occurs, it returns the error code.
*/
OSErr StopNetwork(void);


/*====================================================================================*/
/*
	Look For Others
	This routine returns a list of others on the network with the specified type.  
*/


/*====================================================================================*/
/*
	Connect to Others
	This routine attempts to connect to others on the network with the specified
	type.  
*/


/*====================================================================================*/
/*
	Server Open-Connection Routine
	This routine halts and waits for a connection from another process.  It will wait
	for secondsToWait seconds at most.  secondsToWait must be between 1 and 255.  If
	secondsToWait is 255, then this routine will wait indefinitely (or use the constant 
	WAIT_FOREVER).  If a connection is made, this routine returns 0 (noErr).  If an 
	error occurs (or no connection is made), it returns the error code.
*/
OSErr WaitForNetworkConnection(UInt8 secondsToWait);	// seconds to wait for connection, or WAIT_FOREVER


/*====================================================================================*/
/* 
	Client Open-Connection Routine
	This routine attempts to open a connection with a process that has registered
	a name and type of theName and theType.  This routine will wait
	for secondsToWait seconds at most.  secondsToWait must be between 1 and 255.  If
	secondsToWait is 255, then this routine will wait indefinitely (or use the 
	constant WAIT_FOREVER).  If a connection is made, it returns 0 (noErr).  If an 
	error occurs, or no connection is made, it returns a non-zero error code.
	
	theName and theType must be *AT MOST* 32 bytes long.  That is, they must be 
	Str32's.  They must also be *Pascal* strings.  If the strings do not fit
	these descriptions, bad things will most likely happen.
	
	This routine can take several seconds to execute.  This is because the 
	Apple routines for searching for registered names is extremely slow.
*/
OSErr OpenNetworkConnection(Str32 theName, 			// name of socket to open connection with
							Str32 theType, 	 		// type of socket to open connection with
							UInt8 secondsToWait);	// seconds to wait for connection, or WAIT_FOREVER


/*====================================================================================*/
/*
	This routine permanently closes an open connection.  
	All data to be read/written is aborted immediately.
	If an error occurs, it returns the error code.
*/
OSErr CloseNetworkConnection(void);


/*====================================================================================*/
/*
	Routine to check for spurious network events (i.e attention messages, closed 
	connections, etc).  This routine should be called often (like once every main
	event loop).  Most of the time it is fast -- it just checks a few flags.  If there
	are any pending events though, it will receive them and possibly close down the
	connection if necessary.  Another good time to call this routine is in a loop
	with ReadData(), so attention messages can be received while reading a long data
	stream.

	Returns TRUE if an attention message was received.  If this is the case, then
	attentionCode, attentionSize, attentionData are all filled in (if they are not
	nil -- all those parameters are optional).
*/
Boolean NetworkIdle(UInt16 *attentionCode, 		// attention message code (can be nil)
					UInt16 *attentionSize,		// size of attention message data (can be nil)
					Ptr *attentionData);		// Ptr to attention message data (can be nil)


/*====================================================================================*/
/*
	Routine to read data.  
	This routine does not operate asychronously.  It reads whatever data is available
	up to readBufferSize bytes.  It returns the acutal number of bytes read, and
	a flag indicating if there is more data to be read.  
	
	If actualBytesRead is zero, and noMoreData is 1, then there was nothing to be
	read and the routine returns noErr.
	
	IMPORTANT NOTE (paraphrased from IM-Networking page 5-16):
		If there is less data in the receive queue than the amount you specify with the 
		readBufferSize parameter, the command does not complete execution until there 
		is enough data available to satisfy the request. There are three exceptions 
		to this rule: 
		-  If the end-of-message bit in the ADSP packet header is set, the dspRead 
		   command reads the data in the receive queue, returns the actual amount of data 
		   read in the actualBytesRead parameter, and returns with the noMoreData parameter 
		   set to 1. 
		-  If you have closed the connection end before calling the dspRead routine 
		   (that is, the connection is half open), the command reads whatever data is 
		   available and returns the actual amount of data read in the actualBytesRead 
		   parameter. 
		-  If ADSP has closed the connection before you call the dspRead routine and 
		   there is no data in the receive queue, the routine returns the noErr result 
		   code with the actCount parameter set to 0 and the eom parameter set to 0.
		   
	So, in general, ReadData() will *NOT RETURN* unless:
		(1) it fills up readBuffer by reading readBufferSize bytes, and noMoreData is 0
		    (and there is still more data to be read in this message, so clear out readBuffer
		    and call ReadData() again.)
		or:
		(2) is doesn't fill up readBuffer, but it reads the entire message and noMoreData is 1
		or:
		(3) there is no data to be read in the first place
		
	So, it is a bad thing to call WriteData() with half a message, because the ReadData() on
	the other end will hang until it gets the whole message, slowing things down
	considerably.  Try to send whole messages at a time.  
*/
OSErr ReadData(Ptr readBuffer, 			// buffer to fill (must not be nil)
			UInt16 readBufferSize,		// number of bytes we expect (i.e. size of read buffer)
			UInt16 *actualBytesRead,	// returns number of bytes read this time (can be nil)
			UInt8 *noMoreData);			// returns 1 if there is no more data to read (can be nil)
										// (i.e. it's the end of this message)

/*====================================================================================*/
/*
	Routine to write data.  
	This routine does not operate asychronously.  It writes the given data and sends it
	if flush is 1.  If endOfMessage is 1, then the end of message signal is sent so the
	reader of this data will know that there is no more data to be sent in this message.
	This end-of-message signal allows the receiver to receive messages that are longer
	than its read buffer, and facilitates arbitrary-length messages without having to
	send a length ahead of time.  
	
	See important note under ReadData() for important information.
*/
OSErr WriteData(Ptr writeBuffer, 			// buffer to write (must not be nil)
			UInt16 writeBufferSize,			// number of bytes to write
			UInt8 endOfMessage,				// pass 1 if this is the end of this message
			UInt8 flush,					// pass 1 if we are to send data immediately
			UInt16 *actualBytesWritten);	// returns number of bytes written this time (can be nil)

/*====================================================================================*/
/*
	Routine to reset/resynchronize the connection.  
	This routine does not operate asychronously.  
	This routine clears all data that the remote client has not already read and
	resynchronizes the connection. 
*/
OSErr ResetNetwork(void);

/*====================================================================================*/
/*
	Routine to send an attention message.  Attention messages are handled separately 
	from the main data stream. 
	
	attentionCode may be any value from 0x0000 through 0xEFFF.  Attention code values
	0xF000 through 0xFFFF are reserved by ADSP.  What attentionCode really means is 
	totally application-dependent.
	attentionSize is the size of the data pointed to by attentionData, up to 570 bytes.
	attentionData is a pointer to the attention message data, which can be up to 570
	bytes in length.  (Actually, it can be any size, but only the first attentionSize 
	bytes will be sent.)
*/
OSErr SendAttentionMessage(UInt16 attentionCode, 	// attention message code
							UInt16 attentionSize,	// size of attention message data
							Ptr attentionData);		// Ptr to attention message data

/*====================================================================================*/
/*
	Routine to set the options for a connection.  
	This routine should be called after StartNetwork() but before 
	WaitForNetworkConnection() or OpenNetworkConnection().  
	
	Under most circumstances, there is no need to call this routine at all, because
	the default values are OK.  However, this routine can be called to fine-tune for
	specific situations.  
	
	See IM-Networking page 5-14 for more information about the parameters for this 
	routine.  In general:
		sendBlocking is the max number of bytes that can accumulate in the send queue before
			ADSP sends a packet.  Under most circumstances, the default value of 16 bytes
			performs the best.
		badSeqMax is the max number of out-of-sequence data backets that the the local
			connection can receive before requesting the missing data.   Under most 
			circumstances, the default value of 3 performs the best.
		useCheckSum tells DDP (the protocol underlying ADSP) to use checksums for each
			packet.  This transparent error checking is needed only for highly 
			unreliable networks, as ADSP and DDP perform error checking anyway.
			The default is 0 (no checksum).
*/
OSErr SetNetworkOptions(UInt16 sendBlocking,	// quantum for data packets 
						UInt8 badSeqMax,		// threshold for sending retransmit advice 
						UInt8 useCheckSum);		// use DDP packet checksum 

/*====================================================================================*/
/* ERROR CODES */
/*====================================================================================*/

// our internal error messages (some of which returned when SANITY_CHECKS set to 1)
// other AppleTalk (specifically ADSP) return codes (errors) are generally
// from -91 to -98, -1024 to -1029, and -1273 to -1280.  See header file <Errors.h>

enum {
	ERR_NETWORK_NOT_STARTED = 5000,		// forgot to call StartNetwork()
	ERR_CONNECTION_NOT_OPENED = 5001,	// forgot to call OpenNetworkConnection() and/or
										// WaitForNetworkConnection(); i.e. called a network 
										// routine without a connection being established
	ERR_COULD_NOT_FIND_SERVER = 5002,	// could not find specified server name on network
	ERR_NO_ATTENTION_MESSAGE = 5003,	// receiveAttentionMessage() called without a pending
										// attention message (internal error)
	ERR_TOO_LATE_TO_SET_OPTIONS = 5004,	// SetNetworkOptions() called too late to take effect --
										// i.e. after a connection was already made
	ERR_BOGUS_PARAMETER = 6666			// bad parameter or nil pointer passed
};


/*====================================================================================*/
/* CONSTANTS */
/*====================================================================================*/

enum {
	WAIT_FOREVER = 255,			// use for secondsToWait to mean wait forever
	DEFAULT_QUEUE_SIZE = 600	// use for queue_size for default queue size
};


#endif // _APPLETALK_NETWORK_H_
