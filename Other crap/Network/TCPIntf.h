#ifndef _TCP_H_
#define _TCP_H_

#include <AddressXlation.h>
#include <MacTCP.h>

/*====================================================================================*/


/*====================================================================================*/
/* INTERNAL TYPES */
/*====================================================================================*/

typedef char CStr255[256];		/* like Str255, except for C-format strings. */

/* network initialization ------------------------------------------------------*/

OSErr TCPInitNetwork(void);		/* opens the network driver */


/* connection stream creation/removal -------------------------------------------*/

OSErr TCPCreateStream(				/* creates a stream needed to establish a connection*/
	unsigned long *stream,			/* stream identifier (returned)					*/
	unsigned long recvLen);			/* stream buffer length to be allocated			*/
	
OSErr TCPReleaseStream(			/* disposes of an unused stream and its buffers		*/
	unsigned long stream);			/* stream identifier to dispose					*/


/* connection opening/closing calls ---------------------------------------------*/

OSErr TCPOpenConnection(			/* attempts to establish a connection w/remote host */
	unsigned long stream,			/* stream id to be used for connection			*/
	long remoteHost,				/* network number of remote host				*/
	short remotePort,				/* network port of remote port					*/
	Byte timeout);					/* timeout value for connection					*/
	
OSErr TCPWaitForConnection(		/* listens for a remote connection from a rem. port */
	unsigned long stream,			/* stream id to be used for connection			*/
	Byte timeout,					/* timeout value for open						*/
	short localPort,				/* local port to listen on						*/
	long *remoteHost,				/* remote host connected to (returned)			*/
	short *remotePort);				/* remote port connected to (returned)			*/

OSErr TCPAsyncWaitForConnection(	/* same as above, except executed asynchronously	*/
	unsigned long stream,			/* stream id to be used for connection			*/
	Byte timeout,					/* timeout value for open						*/
	short localPort,				/* local port to listen on						*/
	long remoteHost,				/* remote host to listen for					*/
	short remotePort,				/* remote port to listen for					*/
	TCPiopb **returnBlock);			/* parameter block for call (returned)			*/

OSErr TCPAsyncGetConnectionData(	/* retrieves connection data for above call			*/
	TCPiopb *returnBlock,			/* parameter block for asyncwait call			*/
	long *remoteHost,				/* remote host connected to (returned)			*/
	short *remotePort);				/* remote port connected to (returned)			*/

OSErr TCPCloseConnection(			/* closes an established connection					*/
	unsigned long stream,			/* stream id of stream used for connection		*/
	Boolean waitForOtherSideToCloseFirst);
	
OSErr TCPAbortConnection(			/* aborts a connection non-gracefully				*/
	unsigned long stream);			/* stream id of stream used for connection		*/


/* data sending calls ----------------------------------------------------------*/

OSErr TCPSendData(					/* sends data along an open connection				*/
	unsigned long stream,			/* stream used for connection					*/
	Ptr data,						/* pointer to data to send						*/
	unsigned short length,			/* length of data to send						*/
	Boolean retry);					/* if true, call continues until send successful*/
	
OSErr TCPSendMultiData(			/* sends multiple strings of data on a connection	*/
	unsigned long stream,			/* stream used for connection					*/
	CStr255 data[],					/* array of send strings						*/	
	short numData,					/* number of strings to send					*/
	Boolean retry);					/* if true, call continues until send successful*/

void TCPSendDataAsync(				/* sends data asynchronously						*/
	unsigned long stream,			/* stream used for connection					*/
	Ptr data,						/* pointer to data to send						*/
	unsigned short length,			/* length of data to send						*/
	TCPiopb **returnBlock);			/* pointer to parameter block (returned)		*/
	
OSErr TCPSendAsyncDone(			/* called when SendDataAsync call completes			*/
	TCPiopb *returnBlock);			/* parameter block to complete connection		*/


/* data receiving calls --------------------------------------------------------*/

OSErr TCPRecvData(					/* waits for data to be received on a connection	*/
	unsigned long stream,			/* stream used for connection					*/ 
	Ptr data,						/* pointer to memory used to hold incoming data	*/
	unsigned short *length,			/* length to data received (returned)			*/
	Boolean retry);					/* if true, call continues until successful		*/
	
void TCPRecvDataAsync(				/* receives data asynchronously						*/
	unsigned long stream,			/* stream used for connection					*/
	Ptr data,						/* pointer to memory used to hold incoming data	*/
	unsigned short length,			/* length of data requested						*/
	TCPiopb **returnBlock);			/* parameter block to complete connection		*/
	
OSErr TCPGetDataLength(			/* called when RecvDataAsync completes				*/
	TCPiopb *returnBlock,			/* parameter block used for receive				*/
	unsigned short *length);		/* length of data received (returned)			*/

/* other calls --------------------------------------------------------*/

/*	GetConnectionState gets the connection state of a stream. */
OSErr TCPGetConnectionState (unsigned long stream, Byte *state);

/*	IPNameToAddr invokes the domain name system to translate a domain name
	into an IP address. */
OSErr TCPIPNameToAddr (char *name, unsigned long *addr);

/*	IPAddrToName invokes the domain name system to translate an IP address
	into a domain name. */
OSErr TCPIPAddrToName (unsigned long addr, char *name);

/*	GetMyIPAddr returns the IP address of this Mac. */
OSErr TCPGetMyIPAddr (unsigned long *addr);

/*	GetMyIPAddrStr returns the IP address of this Mac as a dotted decimal
	string. */
OSErr TCPGetMyIPAddrStr (char *addrStr);

/*	GetMyIPName returns the domain name of this Mac. */
OSErr TCPGetMyIPName (char *name);

short TCPGetTCPRefNum (void);


/*====================================================================================*/
/* HTTP HIGH-LEVEL ROUTINE PROTOTYPES */
/*====================================================================================*/

// initialize TCP
OSErr HTTPInit(void);

OSErr HTTPGetFile(							// receive a remote HTTP file
			CStr255 url,						// full URL to receive (C string)
			Handle buffer,  					// buffer to fill (must be allocated)
			unsigned long *length, 				// max length to fill/returns actual length
			char *headers);						// ptr to buffer with headers to send (can be nil)

OSErr HTTPPutFile(							// send a local HTTP file
			CStr255 url,						// full URL location to put file (C string)
			Ptr buffer,  						// ptr to data to send (must be allocated)
			unsigned long length, 				// length to send
			char *headers);						// ptr to buffer with headers to send (can be nil)

/*====================================================================================*/
/* TCP CONSTANTS */
/*====================================================================================*/

#define TCP_BUFFER_LENGTH	0x7FFF		/* buffer size for TCP stream send/receives (32K) */

#define CR			'\r'
#define LF			'\n'
#define	CRSTR		"\r"
#define	LFSTR		"\n"
#define CRLF		"\r\n"
#define CRCR 		"\r\r"

#define NNTP_PORT			119
#define SMTP_PORT			25
#define FTP_PORT			21
#define HTTP_PORT			80

#define SLEEP_TIME		20

/*====================================================================================*/
/* APPLICATION-DEFINED GLOBALS / APPLICATION-DEFINED ROUTINES */
/*====================================================================================*/

extern Boolean gCancel;			/* flag set when user cancels an action */
extern Boolean GiveTime(const long sleepTime);

#endif // _TCP_H_