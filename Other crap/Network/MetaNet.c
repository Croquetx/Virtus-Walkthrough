#include "MetaNet.h"
#include <AppleTalk.h>
#include <ADSP.h>

/*====================================================================================*/
/*
	MetaNet
	~~~~~~~
	by Andrew B. Davidson
	May 1995
	
	
*/
/*====================================================================================*/


/*====================================================================================*/
/* TYPEDEFS */
/*====================================================================================*/

struct ConnectionStruct {
	Boolean inUse;
	UInt8 network_type;			// network host type (ADSP or TCP)
	union {
		struct {
			TRCCB ccb;
			short ccbRefNum;
			Ptr dspSendQPtr;
			Ptr dspRecvQPtr;
			Ptr dspAttnBufPtr;
			Boolean started, opened;
			NamesTableEntry nteName;
		} adsp;
		struct {
			UInt32 tcpStream;		// tcp stream id
			long remoteHost;		// network number of remote host
			short remotePort;		// network port of remote port
			short localPort;		// network port of local port
		} tcp;
	};
};

/*====================================================================================*/
/* GLOBALS */
/*====================================================================================*/

ConnectionStruct gConnections[64];
Boolean gMetaNetInited = false;

/*====================================================================================*/
/* INTERNAL ROUTINE PROTOTYPES */
/*====================================================================================*/

void initMetaNet(void);
UInt32 findUnusedConnection(void);

/*====================================================================================*/
/* ROUTINES */
/*====================================================================================*/


/*====================================================================================*/
HostDescriptor MNMakeADSPHostDescriptor(Str32 hostName, Str32 hostType)
{
HostDescriptor host;
int i;
	host.network_type = METANET_ADSP;
	for (i=0;i<32;i++) { host.adsp.hostName[i] = hostName[i]; }
	for (i=0;i<32;i++) { host.adsp.hostType[i] = hostType[i]; }
	return host;
}

/*====================================================================================*/
HostDescriptor MNMakeTCPHostDescriptor(long hostNum, short hostPort)
{
HostDescriptor host;
	host.network_type = METANET_ADSP;
	host.tcp.hostNum = hostNum;
	host.tcp.hostPort = hostPort;
	return host;
}

/*====================================================================================*/
void initMetaNet()
{
	// set all connections to not in use
}

/*====================================================================================*/
OSErr MNInitNetwork(short network_type)		// opens the network driver 
{
	if (!gMetaNetInited)
		initMetaNet();
	
	switch (network_type)
	{
		case METANET_ADSP:
			break;
		case METANET_TCP:
			return TCPInitNetwork();
			break;
		default:
			Debugger();
			break;
	}
	return noErr;
}

/*====================================================================================*/
UInt32 findUnusedConnection()
{
	// return connection number not in use
	return 0;
}

/*====================================================================================*/
OSErr MNCreateADSPStream(			// creates a stream needed to establish a connection
	UInt32 *stream,					// stream identifier (returned)					
	UInt32 buffer_size,				// stream buffer length to be allocated			
	Str32 theName, 					// name to give this socket
	Str32 theType)					// type to give this socket
{
OSErr err = noErr;
	return err;
}

/*====================================================================================*/
OSErr MNCreateTCPStream(			// creates a stream needed to establish a connection
	UInt32 *stream,					// stream identifier (returned)					
	UInt32 buffer_size,				// stream buffer length to be allocated		
	short local_port)				// preferred local port id	
{
OSErr err = noErr;
UInt32 connect;

	if (stream == nil)
		return -1;

	connect = findUnusedConnection();
	gConnections[connect].tcp.localPort = local_port;
	err = TCPCreateStream(&gConnections[connect].tcp.tcpStream, buffer_size);
	*stream = connect;
	return err;
}

/*====================================================================================*/
OSErr MNReleaseStream(			// disposes of an unused stream and its buffers
	UInt32 stream)			// stream identifier to dispose
{
	switch (gConnections[stream].network_type)
	{
		case METANET_ADSP:
			break;
		case METANET_TCP:
			return TCPReleaseStream(gConnections[stream].tcp.tcpStream);
			break;
		default:
			Debugger();
			break;
	}
	return noErr;
}

/*====================================================================================*/
OSErr MNOpenConnection(			// attempts to establish a connection w/remote host
	UInt32 stream,					// stream id to be used for connection
	HostDescriptor host,			// host to connect to
	UInt8 timeout)					// timeout value for connection
{
	switch (gConnections[stream].network_type)
	{
		case METANET_ADSP:
			break;
		case METANET_TCP:
			return TCPOpenConnection(gConnections[stream].tcp.tcpStream,
					host.tcp.hostNum, host.tcp.hostPort, timeout);
			break;
		default:
			Debugger();
			break;
	}
	return noErr;
}

/*====================================================================================*/
OSErr MNWaitForConnection(		// listens for a remote connection from a rem. port
	UInt32 stream,				// stream id to be used for connection
	UInt8 timeout)				// timeout value for open
{
	switch (gConnections[stream].network_type)
	{
		case METANET_ADSP:
			break;
		case METANET_TCP:
			return TCPWaitForConnection(gConnections[stream].tcp.tcpStream,
					timeout, 
					gConnections[stream].tcp.localPort, 
					&gConnections[stream].tcp.remoteHost, 
					&gConnections[stream].tcp.remotePort);
			break;
		default:
			Debugger();
			break;
	}
	return noErr;
}

/*====================================================================================*/
OSErr MNCloseConnection(		// closes an established connection
	UInt32 stream)			// stream id of stream used for connection
{
	switch (gConnections[stream].network_type)
	{
		case METANET_ADSP:
			break;
		case METANET_TCP:
			return TCPCloseConnection(gConnections[stream].tcp.tcpStream, false);
			break;
		default:
			Debugger();
			break;
	}
	return noErr;
}

/*====================================================================================*/
OSErr MNWriteData(					// routine to send data to other side
			UInt32 stream,					// stream id of stream used for connection 
			Ptr writeBuffer, 				// buffer to write (must not be nil)
			UInt16 writeBufferSize,			// number of bytes to write
			UInt8 endOfMessage,				// pass 1 if this is the end of this message
 			UInt8 flush,					// pass 1 if we are to send data immediately
			UInt16 *actualBytesWritten)		// returns number of bytes written this time (can be nil)
{
	switch (gConnections[stream].network_type)
	{
		case METANET_ADSP:
			break;
		case METANET_TCP:
			return TCPSendData(gConnections[stream].tcp.tcpStream,
				writeBuffer,
				writeBufferSize,
				true);
			break;
		default:
			Debugger();
			break;
	}
	return noErr;
}

/*====================================================================================*/
OSErr MNReadData(					// routine to read incoming data
			UInt32 stream,				// stream id of stream used for connection 
			Ptr readBuffer, 			// buffer to fill (must not be nil)
			UInt16 readBufferSize,		// number of bytes we expect (i.e. size of read buffer)
			UInt16 *actualBytesRead,	// returns number of bytes read this time (can be nil)
			UInt8 *noMoreData)			// returns 1 if there is no more data to read (can be nil)
										// (i.e. it's the end of this message)
{
OSErr err = noErr;

	switch (gConnections[stream].network_type)
	{
		case METANET_ADSP:
			break;
		case METANET_TCP:
		{
		unsigned short recvLength;
		
			recvLength = readBufferSize;
			err = TCPRecvData(gConnections[stream].tcp.tcpStream, readBuffer, 
					&recvLength, false);
			
			// if length is zero, set actualBytesRead/noMoreData
		}
			break;
		default:
			Debugger();
			break;
	}
	return noErr;
}

/*====================================================================================*/
/* LOW-LEVEL ROUTINES BELOW */
/*====================================================================================*/


/*====================================================================================*/
/* EXAMPLE ROUTINES BELOW */
/*====================================================================================*/
#define COMPILE_EXAMPLE_SERVER    0		// set to 1 to compile example server code below
#define COMPILE_EXAMPLE_CLIENT    1		// set to 1 to compile example client code below

#if ((COMPILE_EXAMPLE_SERVER) && (COMPILE_EXAMPLE_CLIENT))
#error Can't compile example code for both client and server.
#endif

// required constants for example code
#if ((COMPILE_EXAMPLE_SERVER) || (COMPILE_EXAMPLE_CLIENT))
#define SERVER_NAME_STR32 	"\pmyServer"	// example name of server
#define SERVER_TYPE_STR32 	"\pmyType"		// example type of server (can equal name of client)
#define CLIENT_NAME_STR32	"\pmyClient"	// example name of client
#define CLIENT_TYPE_STR32	"\pmyType"		// example type of client (can equal name of server)
#define BUFFER_SIZE			256				// example size of read & write buffers
#define LOCAL_TCP_PORT		30				// local preferred port number
#endif

#if COMPILE_EXAMPLE_SERVER
/*====================================================================================*/
/*
	Example routine for a server.  This routine is most effectively executed under
	a source-level debugger along with the client on another machine.  This will
	serve no purpose if it is just executed at full speed; you have you manually
	step through to make sure that the server's WaitForNetworkConnection() is
	called before the client's OpenNetworkConnection, and that the server does
	a ReadData() after the client calls WriteData(), etc.  This code is mainly here 
	for quick-n-dirty testing.  
*/

void main()
{
char myData2ReadPtr[BUFFER_SIZE];		// buffer to read data into
OSErr err = noErr;						// error code
UInt16 actualBytesRead;					// number of bytes read
UInt8 noMoreData;						// flag for no more data
UInt32 streamRefNum;

	err = MNInitNetwork(METANET_ADSP);
	
	// server starts network with its unique name
	err = MNCreateTCPStream(&streamRefNum, DEFAULT_QUEUE_SIZE, LOCAL_TCP_PORT);
	if (err) goto ERROR;
	
	// server waits for connection
	err = MNWaitForConnection(streamRefNum, WAIT_FOREVER);
	if (err) goto ERROR;

	// NOTE: client should call MNOpenConnection() here
	
	// if we get here, we have a connection

	// NOTE: client should call MNWriteData() here
	
	// read the sample data
	err = MNReadData(streamRefNum,		// stream id
			myData2ReadPtr, 			// buffer to fill
			BUFFER_SIZE,				// number of bytes we expect (i.e. size of read buffer)
			&actualBytesRead,			// returns number of bytes read this time (can be nil)
			&noMoreData);				// returns 1 if there is no more data to read (can be nil)
										// (i.e. it's the end of this message)
	if (err) goto ERROR;

ERROR:
	// close the connection and stop network
	err = MNCloseConnection(streamRefNum);
	Debugger();
}

#elif COMPILE_EXAMPLE_CLIENT

/*====================================================================================*/
/*
	Example routine for a client.  See comment above on example routine for a server
	for more information.
*/

void main()
{
char myData2WritePtr[BUFFER_SIZE] = "Hello world!";		// data to write (13 bytes)
OSErr err = noErr;										// error code
UInt16 actualBytesWritten;								// number of bytes written
UInt32 streamRefNum;
HostDescriptor host;

	err = MNInitNetwork(METANET_ADSP);
	
	// client starts network with its unique name
	err = MNCreateTCPStream(&streamRefNum, DEFAULT_QUEUE_SIZE, LOCAL_TCP_PORT);
	if (err) goto ERROR;
	
	// NOTE: server should call MNWaitForConnection() here
	
	// client opens connection with the waiting server's name and type
	host = MNMakeADSPHostDescriptor(SERVER_NAME_STR32, SERVER_TYPE_STR32);
	err = MNOpenConnection(streamRefNum, host, WAIT_FOREVER);
	if (err) goto ERROR;
	
	// if we get here, we have a connection
	
	// write out some sample data
	err = MNWriteData(streamRefNum,		// stream id
			myData2WritePtr, 			// buffer to write
			13,							// number of bytes to write
			1,							// pass 1 if this is the end of this message
			1,							// pass 1 if we are to send data immediately
			&actualBytesWritten);		// returns number of bytes written this time (can be nil)
	if (err) goto ERROR;

	// NOTE: server should call MNReadData() here

ERROR:
	// close the connection and stop network
	err = MNCloseConnection(streamRefNum);
	Debugger();
}
#endif // COMPILE_EXAMPLE_CLIENT