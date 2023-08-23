/*----------------------------------------------------------------------------

	ftp.c

	This handle all transactions with FTP servers.
	
	Portions copyright © 1990, Apple Computer.
	Portions copyright © 1993, Northwestern University.

----------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>

#include "TCPIntf.h"
#include "ftp.h"

static unsigned long gControlConnectionId;
static unsigned long gDataConnectionId;
static Ptr gBuffer;
static Boolean gAlreadyGot226;


void ErrorMessage(char *str);
void MailOrFTPServerErrorMessage(Ptr p);
void UnexpectedErrorMessage(OSErr err);
void StatusString(char *str);


/*=====================================================================================*/
void ErrorMessage(char *str) { }
void MailOrFTPServerErrorMessage(Ptr p) { }
void UnexpectedErrorMessage(OSErr err) { }
void StatusString(char *str) { }


/*=====================================================================================*/
/* Abort aborts a transaction with an FTP server. */

static void FTPAbort (void)
{
	if (gControlConnectionId != 0) {
		TCPAbortConnection(gControlConnectionId);
		TCPReleaseStream(gControlConnectionId);
	}
	if (gDataConnectionId != 0) {
		TCPAbortConnection(gDataConnectionId);
		TCPReleaseStream(gDataConnectionId);
	}
	DisposPtr(gBuffer);
}


/*=====================================================================================*/
/* Init initializes a transaction with an FTP server. */

static Boolean FTPInit (char *host, char *user, char *pswd,
	char *file, char *cmd)
{
	CStr255 sendData[4];
	unsigned long addr;
	unsigned long myAddr;
	unsigned short length;
	OSErr err;
	TCPiopb *pBlock;
	unsigned short localPort;
	Ptr p,pEnd;
	char *serverCommand = nil;
	CStr255 str; 
	
	gControlConnectionId = gDataConnectionId = 0;
	gBuffer = nil;
	pBlock = nil;
	
	//----
		StatusString("Getting host address...");
	//----
	/* Get host address. */
	if (TCPIPNameToAddr(host, &addr) != noErr) {
		if (err != -1) ErrorMessage("Could not get host address.");
		goto exit3;
	}
	
	/* Allocate data buffer. */
	
	gBuffer = NewPtr(TCP_BUFFER_LENGTH);
	if ((err = MemError()) != noErr) 
		goto exit1;
	if (gBuffer == nil)
		goto exit1;
	
	/* Create and open control stream connecton. */
	
	//----
		StatusString("Creating TCP control stream...");
	//----
	if ((err = TCPCreateStream(&gControlConnectionId,TCP_BUFFER_LENGTH)) != noErr) goto exit1;

	//----
		sprintf(str, "Opening connection to %s...", host);
		StatusString(str);
	//----
	if ((err = TCPOpenConnection(gControlConnectionId,addr,FTP_PORT,20)) != noErr) {
		if (err != -1) ErrorMessage("Could not open connection to host.");
		goto exit3;
	}
	length = TCP_BUFFER_LENGTH;
	if ((err = TCPRecvData(gControlConnectionId,gBuffer,&length,true)) != noErr) goto exit1;
	if (length < 3 || *gBuffer != '2') goto exit2;
	
	//----
		sprintf(str, "USER %s", user);
		StatusString(str);
	//----
	/* Send USER command. */	
	strcpy(sendData[0],"USER ");
	strcpy(sendData[1],user);
	strcpy(sendData[2],CRLF);
	if ((err = TCPSendMultiData(gControlConnectionId,sendData,3,true)) != noErr) goto exit1;
	length = TCP_BUFFER_LENGTH;
	if ((err = TCPRecvData(gControlConnectionId,gBuffer,&length,true)) != noErr) goto exit1;
	if (length < 3 || *gBuffer != '3') goto exit4;
	
	//----
		StatusString("PASS");
	//----
	/* Send PASS command. */
	
	strcpy(sendData[0],"PASS ");
	strcpy(sendData[1],pswd);
	strcpy(sendData[2],CRLF);
	if ((err = TCPSendMultiData(gControlConnectionId,sendData,3,true)) != noErr) goto exit1;
	length = TCP_BUFFER_LENGTH;
	if ((err = TCPRecvData(gControlConnectionId,gBuffer,&length,true)) != noErr) goto exit1;
	if (length < 3 || *gBuffer != '2') goto exit4;
	
	//----
		StatusString("Creating TCP data stream...");
	//----
	/* Create and open data stream connection. */
	
	if ((err = TCPCreateStream(&gDataConnectionId,TCP_BUFFER_LENGTH)) != noErr) goto exit1;

	//----
		StatusString("Waiting for connection...");
	//----
	if ((err = TCPAsyncWaitForConnection(gDataConnectionId,0,0,0,0,&pBlock)) != noErr) 
		goto exit1;
		
	/* Wait for MacTCP to assign port number. */
		
	while ((localPort = pBlock->csParam.open.localPort) == 0 && GiveTime(SLEEP_TIME));
	if (gCancel) goto exit3;

	//----
		StatusString("PORT");
	//----
	/* Send PORT command. */	
	if ((err = TCPGetMyIPAddr(&myAddr)) != noErr) goto exit1;
	sprintf(sendData[0],"PORT %hu,%hu,%hu,%hu,%hu,%hu",
		(unsigned short)((myAddr>>24)&0xff), 
		(unsigned short)((myAddr>>16)&0xff), 
		(unsigned short)((myAddr>>8)&0xff), 
		(unsigned short)(myAddr&0xff),
		(unsigned short)((localPort>>8)&0xff), 
		(unsigned short)(localPort&0xff));
	strcpy(sendData[1],CRLF);
	serverCommand = "PORT";
	if ((err = TCPSendMultiData(gControlConnectionId,sendData,2,true)) != noErr) goto exit1;
	length = TCP_BUFFER_LENGTH;
	if ((err = TCPRecvData(gControlConnectionId,gBuffer,&length,true)) != noErr) goto exit1;
	if (length < 3 || *gBuffer != '2') goto exit2;
	
	/* Send RETR or STOR command. */
	
	//----
		sprintf(str, "%s \"%s\"", cmd, file);
		StatusString(str);
	//----
	strcpy(sendData[0],cmd);
	strcpy(sendData[1]," ");
	strcpy(sendData[2],file);
	strcpy(sendData[3],CRLF);
	serverCommand = cmd;
	if ((err = TCPSendMultiData(gControlConnectionId,sendData,4,true)) != noErr) goto exit1;
	length = TCP_BUFFER_LENGTH;
	if ((err = TCPRecvData(gControlConnectionId,gBuffer,&length,true)) != noErr) goto exit1;
	if (length < 3 || *gBuffer != '1') goto exit2;
	
	/* If the timing is just right, it is possible that the final 226 "data transfer
	   complete" message arrived as part of the buffer just received. We must check 
	   for this. */
	   
	p = gBuffer;
	pEnd = gBuffer+length-3;
	gAlreadyGot226 = false;
	while (p < pEnd ) {
		if (*p == CR && *(p+1) == LF) {
			p += 2;
			length = pEnd - p;
			if (length < 3 || *p != '2') {
				MailOrFTPServerErrorMessage(p);
				goto exit3;
			}
			gAlreadyGot226 = true;
			break;
		}
		p++;
	}
	
	/* Wait for server to open its end of the data stream connection. */
	
	while (pBlock->ioResult > 0 && GiveTime(SLEEP_TIME));
	err = gCancel ? -1 : pBlock->ioResult;
	if (err != noErr) goto exit1;
	DisposPtr((Ptr)pBlock);
	
	return true;
	
exit1:

	UnexpectedErrorMessage(err);
	goto exit3;
	
exit2:

	MailOrFTPServerErrorMessage(gBuffer);
	
exit3:

	FTPAbort();
	DisposPtr((Ptr)pBlock);
	return false;
	
exit4:

	ErrorMessage("Invalid username or password.");
	goto exit3;
}


/* Term terminates a transaction with an FTP server. */

static Boolean Term (char *cmd, Boolean get)
{
	CStr255 commStr;
	unsigned short length;
	OSErr err;
	char *serverCommand;
	
	/* Close data stream. */
	
	if ((err = TCPCloseConnection(gDataConnectionId, get)) != noErr) goto exit1;
	if ((err = TCPReleaseStream(gDataConnectionId)) != noErr) goto exit1;
	
	/* Check for final 226 "data transfer complete" reply to RETR or STOR command. */
	
	serverCommand = cmd;
	if (!gAlreadyGot226) {
		length = TCP_BUFFER_LENGTH;
		if ((err = TCPRecvData(gControlConnectionId,gBuffer,&length,true)) != noErr) goto exit1;
		if (length < 3 || *gBuffer != '2') goto exit2;
	}
	
	/* Send QUIT command on control stream. */
	
	//----
		StatusString("QUIT");
	//----
	strcpy(commStr,"QUIT");
	strcat(commStr,CRLF);
	serverCommand = "QUIT";
	if ((err = TCPSendData(gControlConnectionId,commStr,6,true)) != noErr) goto exit1;
	length = TCP_BUFFER_LENGTH;
	if ((err = TCPRecvData(gControlConnectionId,gBuffer,&length,true)) != noErr) goto exit1;
	if (length < 3 || *gBuffer != '2') goto exit2;
	
	//----
		StatusString("Closing connection...");
	//----
	/* Close control stream. */
	if ((err = TCPCloseConnection(gControlConnectionId, true)) != noErr) goto exit1;

	//----
		StatusString("Releasing TCP data stream...");
	//----
	if ((err = TCPReleaseStream(gControlConnectionId)) != noErr) goto exit1;

	/* Dispose the buffer. */
	
	//----
		StatusString("Cleaning up...");
	//----
	DisposPtr(gBuffer);
	return true;
	
exit1:

	UnexpectedErrorMessage(err);
	goto exit3;
	
exit2:

	MailOrFTPServerErrorMessage(gBuffer);
	
exit3:

	FTPAbort();
	return false;
}


/*=====================================================================================*/
/* FTPGetFile gets a file from a host. */	
	
Boolean FTPGetFile (char *host, char *user, char *pswd, char *file, Handle data)
{
	unsigned short length;
	long size, allocated;
	OSErr err;
	Ptr p,pEnd,q;

	if (!FTPInit(host,user,pswd,file,"RETR")) return false;
	
	SetHandleSize(data, TCP_BUFFER_LENGTH);
	if (err = MemError())
		goto exit1;
	if (GetHandleSize(data) != TCP_BUFFER_LENGTH)
		goto exit1;
	size = 0;
	allocated = TCP_BUFFER_LENGTH;
	
	while (true) 
	{
		length = TCP_BUFFER_LENGTH;
		if ((err = TCPRecvData(gDataConnectionId,gBuffer,&length,true)) != noErr) break;
		if (size + length > allocated) 
		{
			allocated += TCP_BUFFER_LENGTH;
			SetHandleSize(data, allocated);
			if (err = MemError())
				goto exit1;
			if (GetHandleSize(data) != allocated)
				goto exit1;
		}
		BlockMove(gBuffer, *data + size, length);
		size += length;
	}
	if (err != connectionClosing && err != connectionTerminated) goto exit1;
	
	p = q = *data;;
	pEnd = *data + size;
	while (p < pEnd) {
		if (*p == CR && *(p+1) == LF) {
			*q++ = CR;
			p += 2;
		} else {
			*q++ = *p++;
		}
	}
	size = q - *data;
	SetHandleSize(data, size + 1);
	if (err = MemError())
		goto exit1;
	if (GetHandleSize(data) != size + 1)
		goto exit1;
	*(*data+size) = 0;

	return Term("RETR", true);
	
exit1:

	UnexpectedErrorMessage(err);
	FTPAbort();
	return false;
}


/*=====================================================================================*/
/* FTPPutFile sends a file to a host. */	
	
Boolean FTPPutFile (char *host, char *user, char *pswd, char *file, Ptr data, long size)
{
	OSErr err;
	Ptr p,pEnd,q,qEnd;
	CStr255 str;

	if (!FTPInit(host,user,pswd,file,"STOR")) return false;
	
	p = data;
	pEnd = data + size;
	while (p < pEnd) {
		q = gBuffer;
		qEnd = q + TCP_BUFFER_LENGTH - 1;
		while (p < pEnd && q < qEnd) {
			if (*p == CR) {
				*q++ = CR;
				*q++ = LF;
				p++;
			} else {
				*q++ = *p++;
			}
		}
		if (q > gBuffer) {
			if ((err = TCPSendData(gDataConnectionId,gBuffer,q-gBuffer,true)) != noErr) goto exit1;
		}
	}
	
	return Term("STOR", false);
	
exit1:

	UnexpectedErrorMessage(err);
	FTPAbort();
	return false;
}
