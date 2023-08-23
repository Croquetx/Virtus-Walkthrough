#include "MetaTCP.h"
#include <string.h>

/*====================================================================================*/
/* REQUIRED EXTERNAL ROUTINES AND VARIABLES */
/*
Boolean gCancel;		// define, and set to true to cancel
Boolean GiveTime(const long sleepTime) { return FALSE; }
*/
/*====================================================================================*/

extern Boolean GiveTime(const long sleepTime);
extern Boolean gCancel;		// define, and set to true to cancel




/*====================================================================================*/
/* global variables */
/*====================================================================================*/

static short 		gRefNum;

/*====================================================================================*/
/* INTERNAL PROTOTYPES */
/*====================================================================================*/

OSErr lowTCPOpenDriver(void);				/* opens the MacTCP driver */

OSErr lowTCPKill(						/* kills a pending MacTCP driver call		*/
	TCPiopb *pBlock);

OSErr lowTCPCreateStream(				/* creates a MacTCP stream 					*/
	StreamPtr *streamPtr,					/* stream created (returned)			*/
	Ptr connectionBuffer,					/* pointer to connection buffer			*/
	unsigned long connBufferLen,			/* length of connection buffer			*/
	TCPNotifyUPP notifPtr);				/* address of Async. Notification Rtn.	*/
	
OSErr lowTCPWaitForConnection(			/* listens for a TCP connection to be opened*/
	StreamPtr streamPtr,					/* stream pointer for connection		*/
	byte timeout,							/* timeout for listen					*/
	ip_addr *remoteHost,					/* remote host to listen for (returned)	*/
	tcp_port *remotePort,					/* remote port to listen on (returned)	*/
	ip_addr *localHost,						/* local host number (returned)			*/
	tcp_port *localPort,					/* local port to listen on (returned)	*/
	Boolean async,							/* true if call to be made async		*/
	TCPiopb **returnBlock);					/* address of parameter block used		*/

OSErr lowTCPFinishWaitForConn(			/* called when lowTCPWaitForConn. completes	*/
	TCPiopb *pBlock,						/* parameter block used for wait call	*/
	ip_addr *remoteHost,					/* remote host connected to (returned)	*/
	tcp_port *remotePort,					/* remote port connected to (returned)	*/
	ip_addr *localHost,						/* local host- our ip number (returned)	*/
	tcp_port *localPort);					/* local port connected to (returned)	*/
	
OSErr lowTCPOpenConnection(				/* actively attempts to connect to a host	*/
	StreamPtr streamPtr,					/* stream to use for this connection	*/
	byte timeout,							/* timeout value for open				*/
	ip_addr remoteHost,						/* ip number of host to connect to		*/
	tcp_port remotePort,					/* tcp port number of host to connect to*/
	ip_addr *localHost,						/* local host ip number (returned)		*/
	tcp_port *localPort);					/* local port number used for connection*/

OSErr lowTCPSendData(					/* send data along an open connection		*/
	StreamPtr streamPtr,					/* stream identifier to send data on	*/
	byte timeout,							/* timeout for this send				*/
	Boolean push,							/* true if "push" bit to be set			*/
	Boolean urgent,							/* true if "urgent" bit to be set		*/
	Ptr wdsPtr,								/* write data structure (len/data pairs)*/
	Boolean async,							/* true if call is to be asynchronous	*/
	TCPiopb **returnBlock);					/* parameter block used (returned)		*/
	
OSErr lowTCPFinishSend(					/* called when TCPSendData completes		*/
	TCPiopb *pBlock);						/* parameter block used in call			*/

OSErr lowTCPRecvData(					/* waits for data to be send on a TCP stream*/
	StreamPtr	streamPtr,					/* stream waiting for data on			*/
	byte		timeout,					/* timeout for receive					*/
	Boolean		*urgent,					/* value of urgent flag (returned)		*/
	Boolean		*mark,						/* value of mark flag (returned)		*/
	Ptr			rcvBuff,					/* buffer to store received data		*/
	unsigned short *rcvLen,					/* length of data received				*/
	Boolean 	async,						/* true if call is asynchronous			*/
	TCPiopb		**returnBlock);				/* parameter block used (returned)		*/
	
OSErr lowFinishTCPRecv(					/* called when lowTCPRecvData completes		*/
	TCPiopb		*pBlock,					/* parameter block used in call			*/
	Boolean		*urgent,					/* value of urgent flag (returned)		*/
	Boolean		*mark,						/* value of mark flag (returned)		*/
	unsigned short *rcvLen);				/* length of data received				*/
	
OSErr lowTCPNoCopyRcv(					/* receives data & doesn't copy to ext. buf.*/
	StreamPtr	streamPtr,					/* stream waiting for data on			*/
	byte		timeout,					/* timeout for receive					*/
	Boolean		*urgent,					/* value of urgent flag (returned)		*/
	Boolean		*mark,						/* value of mark flag (returned)		*/
	Ptr			rdsPtr,						/* pointer to read data struct data/len */
	short		numEntry,					/* number of entries in read data struct*/
	Boolean		async,						/* true if call is asynchronous			*/
	TCPiopb		**returnBlock);				/* parameter block used (returned)		*/
	
OSErr lowTCPFinishNoCopyRcv(			/* called when lowTCPNoCopyRcv finishes		*/
	TCPiopb *pBlock,						/* parameter block used	in call			*/
	Boolean *urgent,						/* value of urgent flag (returned)		*/
	Boolean *mark);							/* value of mark flag (returned)		*/

OSErr lowTCPBfrReturn(					/* returns a buffer used in lowTCPNoCopyRcv	*/
	StreamPtr	streamPtr,					/* stream on which data was received	*/
	Ptr			rdsPtr);					/* pointer to buffer (read data struct)	*/
	
OSErr lowTCPClose(						/* closes a connection on a given TCP stream*/
	StreamPtr	streamPtr,					/* stream identifier for connection		*/
	byte		timeout);					/* timeout for close command			*/
	
OSErr lowTCPAbort(						/* aborts a TCP connection non-gracefully	*/
	StreamPtr streamPtr);					/* stream identifier for connection		*/
	
OSErr lowTCPRelease(					/* releases a TCP connection stream			*/
	StreamPtr	streamPtr,					/* stream to be released				*/
	Ptr			*recvPtr,					/* connection buffer (returned)			*/
	unsigned long *recvLen);

OSErr lowTCPStatus(						/* returns status information for a stream	*/
	StreamPtr	streamPtr,					/* stream to get status of				*/
	TCPStatusPB	*theStatus);				/* status (see MacTCP manual for info)	*/
	
OSErr lowTCPGlobalInfo(					/* returns status information for all MacTCP*/
	Ptr	*tcpParam,							/* TCP parameters(returned) (see manual)*/
	Ptr	*tcpStat);							/* TCP statistics(returned) (see manual)*/

OSErr lowTCPGetMyIPAddr(							/* get local IP address */
	ip_addr *ipnum);						/* ip address (returned) */

OSErr lowTCPIPNameToAddr (char *name, unsigned long *addr);
OSErr lowTCPIPAddrToName (unsigned long addr, char *name);
OSErr lowTCPGetMyIPAddrStr (char *addrStr);

static Boolean IsAUX (void);

/*====================================================================================*/
/*	This is the completion routine used for name-resolver calls.
	It sets the userDataPtr flag to indicate the call has completed.
*/
pascal void DNRResultProc(struct hostInfo *hInfoPtr,char *userDataPtr);
pascal void DNRResultProc(struct hostInfo *hInfoPtr,char *userDataPtr)
{
	*userDataPtr = 0xff;	/* setting the use data to non-zero means we're done */
}

/*====================================================================================*/
OSErr ConvertStringToAddr(char *name,unsigned long *netNum);
OSErr ConvertStringToAddr(char *name,unsigned long *netNum)
{
struct hostInfo hInfo;
OSErr result;
char done = 0x00;
extern Boolean gCancel;

	if ((result = OpenResolver(nil)) == noErr) {
		result = StrToAddr(name,&hInfo, NewResultProc(DNRResultProc),&done);
		if (result == cacheFault)
			while (!done)
				; /* wait for cache fault resolver to be called by interrupt */
		CloseResolver();
		if ((hInfo.rtnCode == noErr) || (hInfo.rtnCode == cacheFault)) {
			*netNum = hInfo.addr[0];
			strcpy(name,hInfo.cname);
			name[strlen(name)-1] = '\0';
			return noErr;
		}
	}
	*netNum = 0;
	
	return result;
}

/*====================================================================================*/	
/* InitNetwork opens the network driver
*/

OSErr TCPInitNetwork(void)
{
	return lowTCPOpenDriver();
}


/*====================================================================================*/	
/* CreateStream() creates an unconnected network stream to be
   used later by OpenConnection.  The length of the receive
   buffer must be specified in the call */
   
OSErr TCPCreateStream(unsigned long *stream,unsigned long recvLen)
{
	Ptr recvPtr;
	OSErr err;
	
	recvPtr = NewPtr(recvLen);
	err = MemError();
	if (err==noErr)
		err = lowTCPCreateStream(stream,recvPtr,recvLen, NewTCPNotifyProc(nil));
	return err;
}


/*====================================================================================*/	
/* OpenConnection() initiates a connection to a remote machine,
   given that machine's network number and connection port.  A timeout
   value for the call must be given, and the stream identifier is returned. */

OSErr TCPOpenConnection(unsigned long stream,long remoteHost,short remotePort,Byte timeout)
{
	ip_addr localHost;
	tcp_port localPort = 0;
	
	return lowTCPOpenConnection(stream,timeout,remoteHost,remotePort,&localHost,
								&localPort);
}


/*====================================================================================*/	
/* WaitForConnection() listens for a connection on a particular port from a
	particular host.  It returns when a connection has been established */

OSErr TCPWaitForConnection(unsigned long stream,Byte timeout,short localPort,
						long *remoteHost,short *remotePort)
{
	ip_addr localHost;
	
	return lowTCPWaitForConnection(stream,timeout,(ip_addr *)remoteHost,
				(tcp_port *)remotePort,&localHost,(tcp_port *)&localPort,false,nil);
}


/*====================================================================================*/	
/* AsyncWaitForConnection() listens for a connection on a particular port from a
	particular host.  It is executed asynchronously and returns immediately */

OSErr TCPAsyncWaitForConnection(unsigned long stream,Byte timeout,short localPort,
				long remoteHost,short remotePort,TCPiopb **returnBlock)
{
	ip_addr localHost;
	
	return lowTCPWaitForConnection(stream,timeout,(ip_addr *)&remoteHost,
				(tcp_port *)&remotePort,&localHost,(tcp_port *)&localPort,true,returnBlock);
}


/*====================================================================================*/	
/* AsyncGetConnectionData() should be called when a call to AsyncWaitForConnection
	completes (when returnBlock->ioResult <= 0).  This call retrieves the information
	about this new connection and disposes the parameter block. */
	
OSErr TCPAsyncGetConnectionData(TCPiopb *returnBlock,long *remoteHost,short *remotePort)
{
	ip_addr localHost;
	tcp_port localPort;
	
	return lowTCPFinishWaitForConn(returnBlock,(ip_addr *)remoteHost,
						(tcp_port *)remotePort,&localHost,&localPort);
}


/*====================================================================================*/	
/* CloseConnection() terminates a connection to a remote host, given the
   stream identifier of the connection. It waits for the remote host to also
   close its end of the connection. */
   
OSErr TCPCloseConnection (unsigned long stream, Boolean waitForOtherSideToCloseFirst)
{
	unsigned short length;
	CStr255 data;
	OSErr err;
	
	if (IsAUX()) {
		err = lowTCPClose(stream, 10);
		if (err != noErr) goto exit;
		return noErr;
	}
	
	if (!waitForOtherSideToCloseFirst) {
		err = lowTCPClose(stream, 10);
		if (err != noErr && err != connectionDoesntExist &&
			err != connectionClosing && err != connectionTerminated) goto exit;
	}

	while (true) {
		length = 256;
		err = RecvData(stream, data, &length, true);
		if (err != noErr) break;
	}
	
	if (waitForOtherSideToCloseFirst) {
		err = lowTCPClose(stream, 10);
		if (err != noErr) goto exit;
	}
	
	return noErr;

exit:

	AbortConnection(stream);
	return err;
}


/*====================================================================================*/	
/* AbortConnection() aborts a connection to a remote host, given the
   stream identifier of the connection */
   
OSErr TCPAbortConnection(unsigned long stream)
{
	return lowTCPAbort(stream);
}
	

/*====================================================================================*/	
/* ReleaseStream() frees the allocated buffer space for a given connection
   stream.  This call should be made after CloseConnection. */
   
OSErr TCPReleaseStream(unsigned long stream)
{
	OSErr err;
	Ptr recvPtr;
	unsigned long recvLen;
	
	if ((err = lowTCPRelease(stream,&recvPtr,&recvLen)) == noErr)
			DisposPtr(recvPtr);
	
	return err;
}


/*====================================================================================*/	
/* SendData() sends data along a connection stream to a remote host. */

OSErr TCPSendData(unsigned long stream, Ptr data, unsigned short length, Boolean retry)
{	
	OSErr err;
	struct wdsEntry myWDS[2];	/* global write data structure */

	myWDS[0].length = length;
	myWDS[0].ptr = data;
	myWDS[1].length = 0;
	myWDS[1].ptr = nil;
	do
		err = lowTCPSendData(stream,20,false,false,(Ptr) myWDS,false,nil);
	while (retry && err==commandTimeout);
	return err;
}

/*====================================================================================*/	
/* SendMultiData() is similar to SendData, but takes an array of strings to send
   to the remote host. */

OSErr TCPSendMultiData(unsigned long stream, Str255 data[], short numData, Boolean retry)
{
#if 0
struct wdsEntry *theWDS;
short i;
OSErr err;
	
	theWDS = (wdsEntry *)NewPtr((numData+1) * sizeof(wdsEntry));
	if (MemError())
		return MemError();	
	theWDS[numData].length = 0;
	theWDS[numData].ptr = nil;
	for (i=0; i<numData; i++) {
		theWDS[i].ptr = data[i];
		theWDS[i].length = strlen((char *)data[i]);
	}
	do
		err = lowTCPSendData(stream,20,false,false,(Ptr) theWDS,false,nil);
	while (retry && err==commandTimeout);
	DisposPtr((Ptr)theWDS);
	return err;
#endif
return noErr;
}


/*====================================================================================*/	
/* SendDataAsync() sends data to a remote host asynchronously.  The ioResult
   parameter block variable should be checked, and SendDataDone() called when
   this flag is zero or negative */

void TCPSendDataAsync(unsigned long stream, Ptr data,unsigned short length,TCPiopb **returnBlock)
{	
struct wdsEntry *theWDS;

	// InitNetCursor();
	theWDS = (wdsEntry *)NewPtr( (2*sizeof(wdsEntry)) );
	theWDS[0].length = length;
	theWDS[0].ptr = data;
	theWDS[1].length = 0;
	theWDS[1].ptr = 0;
	lowTCPSendData(stream,20,false,false,(Ptr) theWDS,true,returnBlock);
	// TrashNetCursor();
}


/*====================================================================================*/	
/* SendDataDone() should be called in response to the completion of a SendDataAsync
   call.  It returns any error which occurred in the send. */

OSErr TCPSendAsyncDone(TCPiopb *returnBlock)
{
	DisposPtr((Ptr)returnBlock->csParam.send.wdsPtr);
	return lowTCPFinishSend(returnBlock);
}


/*====================================================================================*/	
/* RecvData() waits for data to be received on a connection stream.  When data
   arrives, it is copied into the data buffer and the call terminates. */

OSErr TCPRecvData(unsigned long stream, Ptr data, unsigned short *length, Boolean retry)
{
	Boolean	urgent,mark;
	OSErr	err;
	unsigned short recvLength;

	do {
		recvLength = *length;
		err = lowTCPRecvData(stream,20,&urgent,&mark,data,&recvLength,false,nil);
	}
	while (retry && err==commandTimeout);
	*length = recvLength;
	if (err == noErr) {
		*(data+*length) = '\0';
	}
	return err;
}


/*====================================================================================*/	
/* RecvDataAsync() is identical to RecvData above, but in this case, the call is
   made asynchronously. */

void TCPRecvDataAsync(unsigned long stream,Ptr data,unsigned short length,TCPiopb **returnBlock)
{
	Boolean urgent,mark;
	
	lowTCPRecvData(stream,20,&urgent,&mark,data,&length,true,returnBlock);
}


/*====================================================================================*/	
/* GetDataLength() should be called in response to the completion of the
   RecvDataAsync call. */

OSErr TCPGetDataLength(TCPiopb *returnBlock,unsigned short *length)
{
	Boolean urgent,mark;
	
	return lowFinishTCPRecv(returnBlock,&urgent,&mark,length);
}




/*	GetConnectionState gets the connection state of a stream. */

OSErr TCPGetConnectionState (unsigned long stream, byte *state)
{
	TCPStatusPB theStatus;
	OSErr err;
	
	err = lowTCPStatus(stream,&theStatus);
	if (err == connectionDoesntExist) {
		*state = 0;
		return noErr;
	}
	*state = theStatus.connectionState;
	return err;
}

/*	IPNameToAddr invokes the domain name system to translate a domain name
	into an IP address. */
	
OSErr TCPIPNameToAddr (char *name, unsigned long *addr)
{
	OSErr err;
	short i;
	static struct {
		CStr255 name;
		unsigned long addr;
	} cache[10];
	static short numCache=0;
	
	for (i=0; i<numCache; i++) {
		if (strcmp(name, cache[i].name) == 0) {
			*addr = cache[i].addr;
			return noErr;
		}
	}
	if ((err = lowTCPIPNameToAddr(name, addr)) != noErr) return err;
	if (numCache < 10) {
		strcpy(cache[numCache].name, name);
		cache[numCache].addr = *addr;
		numCache++;
	}
	return noErr;
}


/*	IPAddrToName invokes the domain name system to translate an IP address
	into a domain name. */
	
OSErr TCPIPAddrToName (unsigned long addr, char *name)
{
	return lowTCPIPAddrToName(addr, name);
}


/*	GetMyIPAddr returns the IP address of this Mac. */

OSErr TCPGetMyIPAddr (unsigned long *addr)
{
	return lowTCPGetMyIPAddr(addr);
}


/*	GetMyIPAddrStr returns the IP address of this Mac as a dotted decimal
	string. */
	
OSErr TCPGetMyIPAddrStr (char *addrStr)
{
	return lowTCPGetMyIPAddrStr(addrStr);
}

/*	GetMyIPName returns the domain name of this Mac. */

OSErr TCPGetMyIPName (char *name)
{
	unsigned long addr;
	short len;
	static OSErr err;
	static Boolean gotIt=false;
	static CStr255 theName;
	
	if (!gotIt) {
		if ((err = lowTCPGetMyIPAddr(&addr)) != noErr) return err;
		err = lowTCPIPAddrToName(addr,theName);
		gotIt = true;
		len = strlen(theName);
		if (theName[len-1] == '.') theName[len-1] = 0;
	}
	if (err != noErr) return err;
	strcpy(name,theName);
	return noErr;
}
	

/*====================================================================================*/
/* LOW-LEVEL ROUTINES BELOW */
/*====================================================================================*/

OSErr newBlock(TCPiopb **pBlock);

/*====================================================================================*/
short GetTCPRefNum (void)
{
	return gRefNum;
}


/*====================================================================================*/
/* 	IsAUX is a helper function used by CloseConnection to figure out whether 
	we are running under A/UX. */
	
static Boolean IsAUX (void)
{
	return ((*(short*)0xB22) & (1<<9)) != 0;
}

/*====================================================================================*/
/* Opens the MacTCP driver.
   This routine must be called prior to any of the below functions. */

OSErr lowTCPOpenDriver()
{
	OSErr	err;
	
	err = OpenDriver("\p.IPP",&gRefNum);
	return(err);
}


/*====================================================================================*/
OSErr newBlock(TCPiopb **pBlock)
{
	*pBlock = (TCPiopb *)NewPtr(sizeof(TCPiopb));
	if (MemError() != noErr)
		return MemError();
	(*pBlock)->ioCompletion = 0L;
	(*pBlock)->ioCRefNum = gRefNum;
	return noErr;
}


/*====================================================================================*/
/* kills any pending calls to the TCP driver */

OSErr lowTCPKill(TCPiopb *pBlock)
{
	return(PBKillIO((ParmBlkPtr)pBlock,false));
}


/*====================================================================================*/
/* Creates a new TCP stream in preparation for initiating a connection.
   A buffer must be provided for storing incoming data waiting to be processed */

OSErr lowTCPCreateStream(StreamPtr *streamPtr,Ptr connectionBuffer,
			unsigned long connBufferLen, TCPNotifyUPP notifPtr)
{
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
		
	pBlock->csCode = TCPCreate;
	pBlock->ioResult = 1;
	pBlock->csParam.create.rcvBuff = connectionBuffer;
	pBlock->csParam.create.rcvBuffLen = connBufferLen;
	pBlock->csParam.create.notifyProc = notifPtr;
	PBControl((ParmBlkPtr)pBlock,true);
	while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
		;
	if (gCancel)
		return -1;
		
	*streamPtr = pBlock->tcpStream;
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}


/*====================================================================================*/
/* If TCPWaitForConnection is called asynchronously, this command retrieves the 
   result of the call.  It should be called when the above command completes. */

OSErr lowTCPFinishWaitForConn(TCPiopb *pBlock,ip_addr *remoteHost,tcp_port *remotePort,
							ip_addr *localHost,tcp_port *localPort)
{	
	OSErr err;
	
	*remoteHost = pBlock->csParam.open.remoteHost;
	*remotePort = pBlock->csParam.open.remotePort;
	*localHost = pBlock->csParam.open.localHost;
	*localPort = pBlock->csParam.open.localPort;
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}


/*====================================================================================*/
/* Waits for a connection to be opened on a specified port from a specified address.
   It completes when a connection is made, or a timeout value is reached.  This call
   may be made asynchronously. */

OSErr lowTCPWaitForConnection(StreamPtr streamPtr,byte timeout,ip_addr *remoteHost,
			tcp_port *remotePort,ip_addr *localHost,tcp_port *localPort,
			Boolean async,TCPiopb **returnBlock)
{
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPPassiveOpen;
	pBlock->ioResult = 1;
	pBlock->ioCompletion = nil;
	pBlock->tcpStream = streamPtr;
	pBlock->csParam.open.ulpTimeoutValue = timeout;
	pBlock->csParam.open.ulpTimeoutAction = 1;
	pBlock->csParam.open.validityFlags = 0xC0;
	pBlock->csParam.open.commandTimeoutValue = timeout;
	pBlock->csParam.open.remoteHost = *remoteHost;
	pBlock->csParam.open.remotePort = *remotePort;
	pBlock->csParam.open.localPort = *localPort;
	pBlock->csParam.open.tosFlags = 0;
	pBlock->csParam.open.precedence = 0;
	pBlock->csParam.open.dontFrag = 0;
	pBlock->csParam.open.timeToLive = 0;
	pBlock->csParam.open.security = 0;
	pBlock->csParam.open.optionCnt = 0;
	PBControl((ParmBlkPtr)pBlock,true);
	if (!async) {
		while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
			;
		if (gCancel)
			return -1;
		return(lowTCPFinishWaitForConn(pBlock,remoteHost,remotePort,localHost,localPort));
	}
	
	*returnBlock = pBlock;
	return noErr;
}


/*====================================================================================*/
/* Attempts to initiate a connection with a host specified by host and port. */

OSErr lowTCPOpenConnection(StreamPtr streamPtr,byte timeout,ip_addr remoteHost,
			tcp_port remotePort,ip_addr *localHost,tcp_port *localPort)
{
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPActiveOpen;
	pBlock->ioResult = 1;
	pBlock->tcpStream = streamPtr;
	pBlock->csParam.open.ulpTimeoutValue = timeout;
	pBlock->csParam.open.ulpTimeoutAction = 1;
	pBlock->csParam.open.validityFlags = 0xC0;
	pBlock->csParam.open.commandTimeoutValue = timeout;
	pBlock->csParam.open.remoteHost = remoteHost;
	pBlock->csParam.open.remotePort = remotePort;
	pBlock->csParam.open.localPort = *localPort;
	pBlock->csParam.open.tosFlags = 0;
	pBlock->csParam.open.precedence = 0;
	pBlock->csParam.open.dontFrag = 0;
	pBlock->csParam.open.timeToLive = 0;
	pBlock->csParam.open.security = 0;
	pBlock->csParam.open.optionCnt = 0;
	PBControl((ParmBlkPtr)pBlock,true);
	while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
		;
	if (gCancel)
		return -1;
	*localHost = pBlock->csParam.open.localHost;
	*localPort = pBlock->csParam.open.localPort;
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}


/*====================================================================================*/
/* This routine should be called when a TCPSendData call completes.  It returns the
   error code generated upon completion of the CallTCPSend. */

OSErr lowTCPFinishSend(TCPiopb *pBlock)
{
OSErr err;
	
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}


/*====================================================================================*/
/* Sends data through an open connection stream.  Note that the connection must be
   open before any data is sent. This call may be made asynchronously. */

OSErr lowTCPSendData(StreamPtr streamPtr,byte timeout,Boolean push,Boolean urgent,
					Ptr wdsPtr,Boolean async,TCPiopb **returnBlock)
{	
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPSend;
	pBlock->ioResult = 1;
	pBlock->tcpStream = streamPtr;
	pBlock->ioCompletion = nil;
	pBlock->csParam.send.ulpTimeoutValue = timeout;
	pBlock->csParam.send.ulpTimeoutAction = 1;
	pBlock->csParam.send.validityFlags = 0xC0;
	pBlock->csParam.send.pushFlag = push;
	pBlock->csParam.send.urgentFlag = urgent;
	pBlock->csParam.send.wdsPtr = wdsPtr;
	PBControl((ParmBlkPtr)pBlock,true);
	if (!async) {
		while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
			;
		if (gCancel)
			return -1;
		return lowTCPFinishSend(pBlock);
		return err;
	}
	
	*returnBlock = pBlock;
	return noErr;
}


/*====================================================================================*/
OSErr lowTCPFinishNoCopyRcv(TCPiopb *pBlock,Boolean *urgent,Boolean *mark)
{
OSErr err;
	
	*urgent = pBlock->csParam.receive.urgentFlag;
	*mark = pBlock->csParam.receive.markFlag;
	
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}


/*====================================================================================*/
OSErr lowTCPNoCopyRcv(StreamPtr streamPtr,byte timeout,Boolean *urgent,Boolean *mark,
				Ptr rdsPtr,short numEntry,Boolean async,TCPiopb **returnBlock)
{
OSErr	err = noErr;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPNoCopyRcv;
	pBlock->ioResult = 1;
	pBlock->tcpStream = streamPtr;
	pBlock->ioCompletion = nil;
	pBlock->csParam.receive.commandTimeoutValue = timeout;
	pBlock->csParam.receive.rdsPtr = rdsPtr;
	pBlock->csParam.receive.rdsLength = numEntry;
	PBControl((ParmBlkPtr)pBlock,true);
	if (!async) {
		while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
			;
		if (gCancel)
			return -1;
		return lowTCPFinishNoCopyRcv(pBlock,urgent,mark);
	}
	
	*returnBlock = pBlock;
	return noErr;
}


/*====================================================================================*/
OSErr lowTCPBfrReturn(StreamPtr streamPtr,Ptr rdsPtr)
{
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPRcvBfrReturn;
	pBlock->ioResult = 1;
	pBlock->tcpStream = streamPtr;
	pBlock->csParam.receive.rdsPtr = rdsPtr;
	PBControl((ParmBlkPtr)pBlock,true);
	while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
		;
	if (gCancel)
		return -1;
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}


/*====================================================================================*/
/* If the above is called asynchronously, this routine returns the data that was
   received from the remote host. */
   
OSErr lowFinishTCPRecv(TCPiopb *pBlock,Boolean *urgent,Boolean *mark,
					unsigned short *rcvLen)
{
OSErr err;
	
	*rcvLen = pBlock->csParam.receive.rcvBuffLen;
	*urgent = pBlock->csParam.receive.urgentFlag;
	*mark = pBlock->csParam.receive.markFlag;
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}


/*====================================================================================*/
/* Attempts to pull data out of the incoming stream for a connection. If data is
   not present, the routine waits a specified amout of time before returning with
   a timeout error.  This call may be made asynchronously. */
   
OSErr lowTCPRecvData(StreamPtr streamPtr,byte timeout,Boolean *urgent,Boolean *mark,
				Ptr rcvBuff,unsigned short *rcvLen,Boolean async,TCPiopb **returnBlock)
{
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPRcv;
	pBlock->ioResult = 1;
	pBlock->ioCompletion = nil;
	pBlock->tcpStream = streamPtr;
	pBlock->csParam.receive.commandTimeoutValue = timeout;
	pBlock->csParam.receive.rcvBuff = rcvBuff;
	pBlock->csParam.receive.rcvBuffLen = *rcvLen;
	PBControl((ParmBlkPtr)pBlock,true);
	if (!async) {
		while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
			;
		if (gCancel)
			return -1;
		return(lowFinishTCPRecv(pBlock,urgent,mark,rcvLen));
	}
	
	*returnBlock = pBlock;
	return noErr;
}
	

/*====================================================================================*/
/* Gracefully closes a connection with a remote host.  This is not always possible,
   and the programmer might have to resort to CallTCPAbort, described next. */

OSErr lowTCPClose(StreamPtr streamPtr,byte timeout)
{
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPClose;
	pBlock->ioResult = 1;
	pBlock->tcpStream = streamPtr;
	pBlock->csParam.close.ulpTimeoutValue = timeout;
	pBlock->csParam.close.validityFlags = 0xC0;
	pBlock->csParam.close.ulpTimeoutAction = 1;
	PBControl((ParmBlkPtr)pBlock,true);
	while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
		;
	if (gCancel)
		return -1;
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}


/*====================================================================================*/
/* Should be called if a CallTCPClose fails to close a connection properly.
   This call should not normally be used to terminate connections. */
   
OSErr lowTCPAbort(StreamPtr streamPtr)
{
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPAbort;
	pBlock->ioResult = 1;
	pBlock->tcpStream = streamPtr;
	PBControl((ParmBlkPtr)pBlock,true);
	while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
		;
	if (gCancel)
		return -1;
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}

/*====================================================================================*/
OSErr lowTCPStatus(StreamPtr streamPtr,TCPStatusPB *theStatus)
{
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPStatus;
	pBlock->ioResult = 1;
	pBlock->tcpStream = streamPtr;
	PBControl((ParmBlkPtr)pBlock,true);
	while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
		;
	if (gCancel)
		return -1;
	theStatus = &(pBlock->csParam.status);
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}


/*====================================================================================*/
/* Deallocates internal buffers used to hold connection data. This should be
   called after a connection has been closed. */

OSErr lowTCPRelease(StreamPtr streamPtr,Ptr *recvPtr,unsigned long *recvLen)
{
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPRelease;
	pBlock->ioResult = 1;
	pBlock->tcpStream = streamPtr;
	PBControl((ParmBlkPtr)pBlock,true);
	while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
		;
	if (gCancel)
		return -1;
	*recvPtr = pBlock->csParam.create.rcvBuff;
	*recvLen = pBlock->csParam.create.rcvBuffLen;
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}

/*====================================================================================*/
OSErr lowTCPGlobalInfo(Ptr *tcpParam,Ptr *tcpStat)
{
OSErr err;
TCPiopb *pBlock;
	
	if ((err = newBlock(&pBlock)) != noErr)
		return err;
	
	pBlock->csCode = TCPGlobalInfo;
	pBlock->ioResult = 1;
	PBControl((ParmBlkPtr)pBlock,true);
	while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME))
		;
	if (gCancel)
		return -1;
	*tcpParam = (Ptr) pBlock->csParam.globalInfo.tcpParamPtr;
	*tcpStat = (Ptr) pBlock->csParam.globalInfo.tcpStatsPtr;
	err = pBlock->ioResult;
	DisposPtr((Ptr)pBlock);
	return err;
}
	

/*====================================================================================*/
/*	lowTCPIPNameToAddr invokes the domain name system to translate a domain name
	into an IP address. */
	
OSErr lowTCPIPNameToAddr (char *name, unsigned long *addr)
{
struct hostInfo hInfo;
OSErr err;
Boolean done=false;
	
	if ((err = OpenResolver(nil)) != noErr) return err;
	err = StrToAddr(name, &hInfo, NewResultProc(DNRResultProc), (char*)&done);
	if (err == cacheFault) {
		while (!done) GiveTime(SLEEP_TIME);
		err = hInfo.rtnCode;
	}
	CloseResolver();
	*addr = hInfo.addr[0];
	return gCancel ? -1 : err;
}


/*====================================================================================*/
/*	lowTCPIPAddrToName invokes the domain name system to translate an IP address
	into a domain name. */
	
OSErr lowTCPIPAddrToName (unsigned long addr, char *name)
{
struct hostInfo hInfo;
OSErr err;
Boolean done=false;
	
	if ((err = OpenResolver(nil)) != noErr) return err;
	err = AddrToName(addr, &hInfo, NewResultProc(DNRResultProc), (char*)&done);
	if (err == cacheFault) {
		while (!done) GiveTime(SLEEP_TIME);
		err = hInfo.rtnCode;
	}
	CloseResolver();
	hInfo.cname[254] = 0;
	strcpy(name,hInfo.cname);
	return gCancel ? -1 : err;
}


/*====================================================================================*/
/* lowTCPGetMyIPAddr returns the IP address of this Mac. */

OSErr lowTCPGetMyIPAddr (unsigned long *addr)
{
struct	IPParamBlock	IPBlock;
	
	memset(&IPBlock, 0, sizeof(IPBlock));
	IPBlock.ioResult = 1;
	IPBlock.csCode = ipctlGetAddr;
	IPBlock.ioCRefNum = gRefNum;
	PBControl((ParmBlkPtr)&IPBlock,true);
	while (IPBlock.ioResult > 0) GiveTime(SLEEP_TIME);
	// *addr = IPBlock.ourAddress;
	*addr = IPBlock.csParam.IPEchoPB.dest;		// �� IS THIS RIGHT? ��
	return gCancel ? -1 : IPBlock.ioResult;
}


/*====================================================================================*/
/*	lowTCPGetMyIPAddrStr returns the IP address of this Mac as a dotted decimal
	string. */
	
OSErr lowTCPGetMyIPAddrStr (char *addrStr)
{
unsigned long addr;
OSErr err;
static char theAddrStr[16];
static Boolean gotIt=false;
	
	if (!gotIt) {
		if ((err = lowTCPGetMyIPAddr(&addr)) != noErr) return err;
		if ((err = OpenResolver(nil)) != noErr) return err;
		err = AddrToStr(addr,theAddrStr);
		CloseResolver();
		if (err != noErr) return err;
		gotIt = true;
	}
	strcpy(addrStr,theAddrStr);
	return noErr;
}

