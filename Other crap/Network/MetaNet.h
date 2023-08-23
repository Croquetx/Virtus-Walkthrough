#ifndef _METANET_H_
#define _METANET_H_

#include "MetaLowTCP.h"
#include "MetaLowADSP.h"

/*====================================================================================*/
/* host id structure ------------------------------------------------------*/
/*====================================================================================*/

struct HostDescriptor {
	UInt8 network_type;						// network host type (ADSP or TCP)
	union {
		struct {
			Str32 hostName; 				// name of socket 
			Str32 hostType; 	 			// type of socket 
		} adsp;
		struct {
			long hostNum;					// network number of remote host
			short hostPort;					// network port of remote port
		} tcp;
	};
};

HostDescriptor MNMakeADSPHostDescriptor(Str32 hostName, Str32 hostType);
HostDescriptor MNMakeTCPHostDescriptor(long hostNum, short hostPort);
					
/*====================================================================================*/
/* network initialization ------------------------------------------------------*/
/*====================================================================================*/

OSErr MNInitNetwork(short network_type);		// opens the network driver 

OSErr MNSetADSPNetworkOptions(UInt16 sendBlocking,	// quantum for data packets 
							UInt8 badSeqMax,		// threshold for sending retransmit advice 
							UInt8 useCheckSum);		// use DDP packet checksum 

/*====================================================================================*/
/* connection stream creation/removal -------------------------------------------*/
/*====================================================================================*/

OSErr MNCreateADSPStream(			// creates a stream needed to establish a connection
	UInt32 *stream,					// stream identifier (returned)					
	UInt32 buffer_size,				// stream buffer length to be allocated			
	Str32 theName, 					// name to give this socket
	Str32 theType);					// type to give this socket

OSErr MNCreateTCPStream(			// creates a stream needed to establish a connection
	UInt32 *stream,					// stream identifier (returned)					
	UInt32 buffer_size,				// stream buffer length to be allocated		
	short local_port);				// preferred local port id	
	
OSErr MNReleaseStream(			// disposes of an unused stream and its buffers
	UInt32 stream);			// stream identifier to dispose


/*====================================================================================*/
/* connection opening/closing calls ---------------------------------------------*/
/*====================================================================================*/

OSErr MNOpenConnection(			// attempts to establish a connection w/remote host
	UInt32 stream,			// stream id to be used for connection
	HostDescriptor host,			// host to connect to
	UInt8 timeout);					// timeout value for connection

OSErr MNWaitForConnection(		// listens for a remote connection from a rem. port
	UInt32 stream,			// stream id to be used for connection
	UInt8 timeout);					// timeout value for open

OSErr MNCloseConnection(		// closes an established connection
	UInt32 stream);			// stream id of stream used for connection

/*====================================================================================*/
/* data sending calls ----------------------------------------------------------*/
/*====================================================================================*/

OSErr MNWriteData(					// routine to send data to other side
			UInt32 stream,					// stream id of stream used for connection 
			Ptr writeBuffer, 				// buffer to write (must not be nil)
			UInt16 writeBufferSize,			// number of bytes to write
			UInt8 endOfMessage,				// pass 1 if this is the end of this message
 			UInt8 flush,					// pass 1 if we are to send data immediately
			UInt16 *actualBytesWritten);	// returns number of bytes written this time (can be nil)

/*====================================================================================*/
/* data receiving calls --------------------------------------------------------*/
/*====================================================================================*/

OSErr MNReadData(					// routine to read incoming data
			UInt32 stream,					// stream id of stream used for connection 
			Ptr readBuffer, 			// buffer to fill (must not be nil)
			UInt16 readBufferSize,		// number of bytes we expect (i.e. size of read buffer)
			UInt16 *actualBytesRead,	// returns number of bytes read this time (can be nil)
			UInt8 *noMoreData);			// returns 1 if there is no more data to read (can be nil)
										// (i.e. it's the end of this message)

/*====================================================================================*/
/* NETWORK TYPES */
/*====================================================================================*/

enum {					// network types for MNInitNetwork()
	METANET_ADSP = 1,	// use AppleTalk Data Stream Protocol for network communication
	METANET_TCP = 2		// use TCP/IP for network communication	
};

#endif // _METANET_H_