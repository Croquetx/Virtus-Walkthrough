#include "TCPIntf.h"
#include "ParseURL.h"
#include <string.h>
#include <stdio.h>


/*====================================================================================*/
/* DEBUG ONLY */
/*====================================================================================*/
Boolean gCancel;		// set to TRUE to cancel
Boolean GiveTime(const long sleepTime) 
{ 
KeyMap theKeys;
	GetKeys(theKeys);
	if (((theKeys[1] & 4) == 4) &&	// option
		((theKeys[1] & 8) == 8) &&	// control
		((theKeys[1] & 1) == 1))	// shift
		return FALSE;
	else
		return TRUE; 
}		// return FALSE to abort

/*====================================================================================*/
/* REQUIRED EXTERNAL ROUTINES AND VARIABLES */
/*
Boolean gCancel;		// set to TRUE to cancel
Boolean GiveTime(const long sleepTime) { return TRUE; }		// return FALSE to abort
*/
/*====================================================================================*/

extern Boolean GiveTime(const long sleepTime);	// return FALSE to abort
extern Boolean gCancel;							// set to TRUE to cancel

/*====================================================================================*/
/* ACTUAL TCP CODE -- INCLUDE FOR EITHER MAC OR WINDOWS */
/*====================================================================================*/
#if 1		// MACINTOSH
#include "TCPMac.c"
#else
#include "TCPWin.c"
#endif		// MACINTOSH


/*====================================================================================*/
/* EXAMPLE ROUTINES BELOW */
/*====================================================================================*/
#include "ClientServer.h"

#if ((COMPILE_EXAMPLE_SERVER) && (COMPILE_EXAMPLE_CLIENT))
#error Can't compile example code for both client and server.
#endif

// required constants for example code
#if ((COMPILE_EXAMPLE_SERVER) || (COMPILE_EXAMPLE_CLIENT))
#define SERVER_DOMAIN_NAME		"davidson-mac.virtus.com"
#define SERVER_IP_ADDRESS_NUM	0xC748542A	// 199.72.84.42

#define CLIENT_DOMAIN_NAME		"marketing-mac.virtus.com"
#define CLIENT_IP_ADDRESS_NUM	0xC748542B	// 199.72.84.43

#define OUR_TCP_PORT			33			// preferred port number
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
Ptr gBuffer = nil;
void main()
{
OSErr err = noErr;						// error code
UInt16 actualBytesRead = TCP_BUFFER_LENGTH;		// number of bytes read
UInt32 streamRefNum;
unsigned long my_ip;
long remoteHost;
short remotePort;

	gBuffer = NewPtr(TCP_BUFFER_LENGTH);
	if ((err = MemError()) != noErr) 
		goto ERROR;
	if (gBuffer == nil)
		goto ERROR;

	err = TCPInitNetwork();

	err = TCPGetMyIPAddr (&my_ip);		// should equal 199.72.84.42 = 0xC748542A = 3343406122
	if (my_ip != SERVER_IP_ADDRESS_NUM)
		Debugger();
	
	// server starts network with its unique name
	err = TCPCreateStream(&streamRefNum, TCP_BUFFER_LENGTH);
	if (err) goto ERROR;
	
	// server waits for connection
	err = TCPWaitForConnection(streamRefNum, 120, OUR_TCP_PORT, &remoteHost, &remotePort);
	if (err) goto ERROR;

	// NOTE: client should call TCPOpenConnection() here
	
	// if we get here, we have a connection

	// NOTE: client should call TCPWriteData() here
	
	// read the sample data
	err = TCPRecvData(streamRefNum, gBuffer, &actualBytesRead, FALSE);
	if (err) goto ERROR;

ERROR:
	// close the connection and stop network
	err = TCPCloseConnection(streamRefNum, FALSE);
	err = TCPReleaseStream(streamRefNum);
	Debugger();
}

#elif COMPILE_EXAMPLE_CLIENT

/*====================================================================================*/
/*
	Example routine for a client.  See comment above on example routine for a server
	for more information.
*/
Ptr gBuffer = nil;
void main()
{
char myData[32] = "Hello world!";		// data to write (13 bytes)
OSErr err = noErr;						// error code
UInt32 streamRefNum;
unsigned long my_ip;
unsigned long server_ip;

	gBuffer = NewPtr(TCP_BUFFER_LENGTH);
	if ((err = MemError()) != noErr) 
		goto ERROR;
	if (gBuffer == nil)
		goto ERROR;

	err = TCPInitNetwork();

	err = TCPGetMyIPAddr (&my_ip);		// should equal 199.72.84.43 = 0xC748542B = 3343406123
	if (my_ip != CLIENT_IP_ADDRESS_NUM)
		Debugger();
	
	// client starts network with its unique name
	err = TCPCreateStream(&streamRefNum, TCP_BUFFER_LENGTH);
	if (err) goto ERROR;
	
	// NOTE: server should call TCPWaitForConnection() here
	
	/*---- begin domain name server stuff ----*/
	err = OpenResolver(nil);		// nil means use default Hosts file
	if (err) goto ERROR;
	err = StrToAddr(SERVER_DOMAIN_NAME, &host_info, NewResultProc(nil), nil);
	if (err) goto ERROR;
	
	if (host_info.rtnCode == noErr)		// are the fields valid?
	{
		server_ip = host_info.addr[0];
	}
	else
	{
		// we could have a cache fault error here, in which case our result proc will
		// be called when it really gets the name... but for now we pass nil as the
		// result proc
	}

	err = CloseResolver();
	if (err) goto ERROR;
	/*---- end domain name server stuff ----*/
	
	// client opens connection with the waiting server's name and type
	err = TCPOpenConnection(streamRefNum, SERVER_IP_ADDRESS_NUM, OUR_TCP_PORT, 120);
	if (err) goto ERROR;
	
	// if we get here, we have a connection
	
	// copy data to write to the buffer
	for ( i = 0 ; i < 32 ; i++ ) gBuffer[i] = myData[i];
	
	// write out some sample data
	TCPSendData(streamRefNum, myData2WritePtr, 13, FALSE);
	if (err) goto ERROR;

	// NOTE: server should call TCPReadData() here

ERROR:
	// close the connection and stop network
	err = TCPCloseConnection(streamRefNum, FALSE);
	err = TCPReleaseStream(streamRefNum);
	Debugger();
}
#endif // COMPILE_EXAMPLE_CLIENT


/*====================================================================================*/
/* DOMAIN NAME RESOLVER CODE */
/*====================================================================================*/
// include the DNR code... for some bizarre reason this is in a header file!
#include <dnr.c>



/*====================================================================================*/
/* HTTP CODE */
/*====================================================================================*/

#define GET_STRING				"GET"
#define PUT_STRING				"PUT"
#define CONTENT_TRANSFER_ENCODING_STRING	"Content-Transfer-Encoding: %s\r\n"
#define CONTENT_ENCODING_STRING				"Content-Encoding: %s\r\n"
#define CONTENT_TYPE_STRING					"Content-Type: %s\r\n"
#define CONTENT_LENGTH_STRING				"Content-Length: %d\r\n"
#define SPACE					" "
#define HTTP_VERSION_STRING		"HTTP/1.0"

/*====================================================================================*/
OSErr HTTPInit()
{
OSErr err = noErr;

	err = TCPInitNetwork();

ERROR:
	return err;
}

/*====================================================================================*/
OSErr tcp_sendString(							// send a string along a stream
			unsigned long stream_refnum,		// TCP stream refnum
			CStr255 str); 						// string to send (C string)
OSErr tcp_sendString(							// send a string along a stream
			unsigned long stream_refnum,		// TCP stream refnum
			CStr255 str) 						// string to send (C string)
{
OSErr err = noErr;
unsigned short length;

	length = strlen(str);
	err = TCPSendData(stream_refnum, str, length, TRUE);
	return err;
}

/*====================================================================================*/
/*
	Get a file with HTTP.
	This routine opens a TCP/IP data stream, and gets a remote file using an HTTP "GET"
	command.
*/
OSErr HTTPGetFile(							// receive a remote HTTP file
			CStr255 url,						// full URL to receive (C string)
			Handle buffer,  					// buffer to fill (must be allocated)
			unsigned long *length, 				// max length to fill/returns actual length
			char *headers)						// ptr to buffer with headers to send (can be nil)
{
OSErr err = noErr;
Byte protocol;
unsigned long stream_refnum;
unsigned long server_ip;
unsigned short short_length;
CStr255 host, file;
Byte oldstate;

	err = TCPInitNetwork();
	if (err) goto ERROR;
	
	// client starts network with its unique name
	err = TCPCreateStream(&stream_refnum, TCP_BUFFER_LENGTH);
	if (err) goto ERROR;

	err = ParseURL(url, &protocol, host, nil, file, 
				nil, nil, nil);		// don't care about port, section, username, or password ��� TO-BE-IMPLEMENTED
	if (protocol != PROTOCOL_HTTP)
	{	
		err = -1; 
		goto ERROR;
	}

	err = convertStringToAddr(host, &server_ip);
	if (err) goto ERROR;
	
	// client opens connection with the waiting server's name and type
	err = TCPOpenConnection(stream_refnum, server_ip, HTTP_PORT, 120);
	if (err) goto ERROR;
	
	// send "GET" SP
	err = tcp_sendString(stream_refnum, GET_STRING);
	if (err) goto ERROR;
	err = tcp_sendString(stream_refnum, SPACE);
	if (err) goto ERROR;

	// send path & filename SP
	err = tcp_sendString(stream_refnum, file);
	if (err) goto ERROR;
	err = tcp_sendString(stream_refnum, SPACE);
	if (err) goto ERROR;

	// send "HTTP/1.0" CRLF
	err = tcp_sendString(stream_refnum, HTTP_VERSION_STRING);
	if (err) goto ERROR;
	err = tcp_sendString(stream_refnum, CRLF);
	if (err) goto ERROR;

	// send header buffer (must be formatted correctly and have CR/LF's!)
	if (headers != nil)
	{
		tcp_sendString(stream_refnum, headers);
		if (err) goto ERROR;
	}
	
	// send final CRLF to end this request
	err = tcp_sendString(stream_refnum, CRLF);
	if (err) goto ERROR;

{
Ptr responsePtr = nil;
int i;

	// write it to a file
	FILE *f = fopen("data.received", "w");

	// receive the response
	responsePtr = nil;
	responsePtr = NewPtr(TCP_BUFFER_LENGTH);
	if ((err = MemError()) != noErr) 
		goto ERROR;
	if (responsePtr == nil)
		goto ERROR;

	short_length = TCP_BUFFER_LENGTH;

	do	
	{
		short_length = TCP_BUFFER_LENGTH;
		err = TCPRecvData(stream_refnum, responsePtr, &short_length, TRUE);
		if (err) break;
		if (short_length == 0) break;
		
		for ( i = 0 ; i < short_length ; i++ )
		{
			fputc(responsePtr[i], f);
		}
	}
	while (TRUE);
	fflush(f);
	fclose(f);	
}

	#if 0
	// receive the response -- since we can only receive 0x7FFF bytes at a time, we need
	// to call TCPRecvData and repeatedly expand the handle to read all the data
	// ����� TO-BE-IMPLEMENTED need to call TCPRecvData repeatedly
	short_length = TCP_BUFFER_LENGTH;
	oldstate = HGetState(buffer);
	HLock(buffer);
	err = TCPRecvData(stream_refnum, *buffer, &short_length, TRUE);
	if (length) 
		*length = (unsigned long)short_length;
	HSetState(buffer, oldstate);
	if (err) goto ERROR;
	#endif

ERROR:
	// Close the connection and release the stream -- this may not be very fast, and it
	// may be better to open/close connections outside this routine?
	err = TCPCloseConnection(stream_refnum, FALSE);	
	err = TCPReleaseStream(stream_refnum);
	return err;
}

/*====================================================================================*/
/*
	Put a file with HTTP.
	This routine opens a TCP/IP data stream, and sends the file using an HTTP "PUT"
	command.
*/
OSErr HTTPPutFile(							// send a local HTTP file
			CStr255 url,						// full URL location to put file (C string)
			Ptr buffer,  						// ptr to data to send (must be allocated)
			unsigned long length, 				// length to send
			char *headers)						// ptr to buffer with headers to send (can be nil)
{
OSErr err = noErr;
Byte protocol;
unsigned long stream_refnum;
unsigned long server_ip;
unsigned short short_length;
CStr255 host, file, str;
Ptr responsePtr = nil;
int i;

	err = TCPInitNetwork();
	if (err) goto ERROR;
	
	// client starts network with its unique name
	err = TCPCreateStream(&stream_refnum, TCP_BUFFER_LENGTH);
	if (err) goto ERROR;

	err = ParseURL(url, &protocol, host, nil, file, 
				nil, nil, nil);		// don't care about port, section, username, or password ��� TO-BE-IMPLEMENTED
	if (protocol != PROTOCOL_HTTP)
	{	
		err = -1; 
		goto ERROR;
	}

	err = convertStringToAddr(host, &server_ip);
	if (err) goto ERROR;
	
	// client opens connection with the waiting server's name and type
	err = TCPOpenConnection(stream_refnum, server_ip, HTTP_PORT, 120);
	if (err) goto ERROR;
	
#define PUT_THIS_STRING "PUT /virtus/TestPut.html HTTP/1.0\r\nContent-Length: 10\r\n\r\n1234567890\r\n"

	// send "GET" SP
	err = tcp_sendString(stream_refnum, PUT_THIS_STRING);
	if (err) goto ERROR;
	
	#if 0
	// send "GET" SP
	err = tcp_sendString(stream_refnum, PUT_STRING);
	if (err) goto ERROR;
	err = tcp_sendString(stream_refnum, SPACE);
	if (err) goto ERROR;

	// send path & filename SP
	err = tcp_sendString(stream_refnum, file);
	if (err) goto ERROR;
	err = tcp_sendString(stream_refnum, SPACE);
	if (err) goto ERROR;

	// send "HTTP/1.0" CRLF
	err = tcp_sendString(stream_refnum, HTTP_VERSION_STRING);
	if (err) goto ERROR;
	err = tcp_sendString(stream_refnum, CRLF);
	if (err) goto ERROR;

	//----
	// send Content Transfer Encoding
	sprintf(str, CONTENT_TRANSFER_ENCODING_STRING, "none");
	err = tcp_sendString(stream_refnum, str);
	if (err) goto ERROR;

	//----
	// send Content Encoding
	sprintf(str, CONTENT_ENCODING_STRING, "none");
	err = tcp_sendString(stream_refnum, str);
	if (err) goto ERROR;

	//----
	// send Content Type
	sprintf(str, CONTENT_TYPE_STRING, "text/plain");
	err = tcp_sendString(stream_refnum, str);
	if (err) goto ERROR;

	//----
	// send Content Length
	sprintf(str, CONTENT_LENGTH_STRING, "20");
	err = tcp_sendString(stream_refnum, str);
	if (err) goto ERROR;

	// send aux header buffer (must be formatted correctly and have CR/LF's!)
	if (headers != nil)
	{
		tcp_sendString(stream_refnum, headers);
		if (err) goto ERROR;
	}
	
	// send buffer
	err = TCPSendData(stream_refnum, buffer, length, TRUE);
	
	// send final CRLF to end this request
	err = tcp_sendString(stream_refnum, CRLF);
	if (err) goto ERROR;
	#endif

	// receive the response
	responsePtr = nil;
	responsePtr = NewPtr(TCP_BUFFER_LENGTH);
	if ((err = MemError()) != noErr) 
		goto ERROR;
	if (responsePtr == nil)
		goto ERROR;

	short_length = TCP_BUFFER_LENGTH;
	err = TCPRecvData(stream_refnum, responsePtr, &short_length, TRUE);
	if (err) goto ERROR;

	// write it to a file
	FILE *f = fopen("data.received", "w");
	for ( i = 0 ; i < short_length ; i++ )
	{
		fputc(responsePtr[i], f);
	}
	fflush(f);
	fclose(f);	

ERROR:
	if (responsePtr) DisposPtr(responsePtr);

	// Close the connection and release the stream -- this may not be very fast, and it
	// may be better to open/close connections outside this routine?
	err = TCPCloseConnection(stream_refnum, FALSE);	
	err = TCPReleaseStream(stream_refnum);
	return err;
}

/*====================================================================================*/
/* HTTP CLIENT CODE */
/*====================================================================================*/

#define COMPILE_HTTP_CLIENT 1
#if COMPILE_HTTP_CLIENT

#define TEST_GET_URL	"http://www.webpress.net//"
#define TEST_PUT_URL0	"http://marketing-mac.virtus.com/TestPut.html"
#define TEST_PUT_URL1	"http://www.webpress.net/"
#define TEST_PUT_URL	"http://titan.interpath.net/"

#include <stdio.h>

/*====================================================================================*/
/*
	Example routine for a HTTP client.  
*/
void main()
{
OSErr err = noErr;						// error code
unsigned long i = 0;
unsigned long length = TCP_BUFFER_LENGTH;
Handle bufferH = nil;
char buffer[255] = "Hello World!\r\n";

	bufferH = nil;
	bufferH = NewHandle(TCP_BUFFER_LENGTH);
	if ((err = MemError()) != noErr) 
		goto ERROR;
	if (bufferH == nil)
		goto ERROR;
		
	#if 1
	err = HTTPGetFile(							// receive a remote HTTP file
			TEST_GET_URL,						// full URL to receive (C string)
			bufferH,  							// buffer to fill (must be allocated)
			&length, 							// max length to fill/returns actual length
			nil);								// headers to send

	#else
		
	HLock(bufferH);
	strcpy(*bufferH, "12345678901234567890");
	length = 20;
	err = HTTPPutFile(	
			TEST_PUT_URL,
			*bufferH,
			length,
			nil);
	HUnlock(bufferH);
	#endif

ERROR:
	// close the connection and stop network
	if (bufferH) DisposHandle(bufferH);
	Debugger();
}

#endif // COMPILE_HTTP_CLIENT



#if 0
#include "ftp.h"
void main ()
{
OSErr err = noErr;						// error code
Handle bufferH = nil;

	err = TCPInitNetwork();

	bufferH = nil;
	bufferH = NewHandle(TCP_BUFFER_LENGTH);
	if ((err = MemError()) != noErr) 
		goto ERROR;
	if (bufferH == nil)
		goto ERROR;
		
	HLock(bufferH);
	strcpy(*bufferH, "1234567890");
	err = FTPPutFile("marketing-mac.virtus.com", "davidson", "fool4www", 
			"Macintosh HD:ftpdirectory:testputfile2.txt", *bufferH, 10);
	HUnlock(bufferH);
	
ERROR:
	// close the connection and stop network
	if (bufferH) DisposHandle(bufferH);
	Debugger();
}
#endif
