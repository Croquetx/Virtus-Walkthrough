#include <AppleTalk.h>
#include <ADSP.h>
#include "MetaADSP.h"		// our prototypes and error codes

/*====================================================================================*/
/*
	AppleTalk Data Stream Protocol Interface
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	by Andrew B. Davidson
	April 1995
	
	This code uses ADSP to do network communication.  
	
	More info on AppleTalk networking can be found in:
	
	"Inside Macintosh: Networking"  (aka IM-Networking)
		Chapter 1: Introduction to AppleTalk
			especially page 1-24 for ADSP info
		Chapter 5: AppleTalk Data Stream Protocol (ADSP)
			especially pages 5-13 through 5-17, and 5-43 through 5-76
		Chapter 3: Name-Binding Protocol (NBP)
		
	This protocol (ADSP) has many advantages.  ADSP is high-level, network-independent,
	full-duplex, supports attention messages outside the main data stream, provides
	error checking and reliable, proper-sequence delivery of data.  ADSP also provides
	for a sub-protocol ASDSP, which allows secure connections but is not implemented
	here.
		
	From IM-Networking, page 1-24:
			Your application can use ADSP to set up and maintain a connection with 
			another application over an internet. Through this connection, both 
			applications can send and receive streams of data at any time. Because 
			ADSP allows for the continuous exchange of data, any application that 
			needs to support the exchange of more than a small amount of data should 
			use ADSP. In addition to providing for a duplex data stream, ADSP also 
			provides an application with a means of sending attention messages to 
			pass control information between the two communicating applications 
			without disrupting the main flow of data.  
			
			In most cases, ADSP is the protocol that Apple recommends applications 
			use for sending and receiving data. In addition to ensuring reliable 
			delivery of data, ADSP provides a peer-to-peer connection, that is, 
			both ends of the connection can exert equal control over the exchange 
			of data. 
		
	From IM-Networking, page 1-13:
			The AppleTalk Data Stream Protocol (ADSP) is a connection-oriented 
			protocol that supports sessions over which applications and processes 
			that are socket clients can exchange full-duplex streams of data across 
			an AppleTalk internet. ADSP is a symmetrical protocol; the socket 
			clients at either end of the connection have equal control over the 
			ADSP session and the data exchange. Through attention messages, ADSP 
			also provides for out-of-band signaling, a process of sending data 
			outside the normal session dialog so as not to interrupt the data flow. 

	From IM-Networking, page 5-4:
			ADSP manages and controls the data flow between the two sockets 
			throughout the session to ensure that:
			-- the data is delivered and received in the order in which it was sent
			-- duplicate data is not sent
			-- the application or process at the receiving end of the connection has 
			   the buffer capacity to accept the data

	From IM-Networking, page 1-28:
			If you write an application that uses one of the high-level AppleTalk 
			protocols, such as ADSP or ATP, your program will run over any link type. 
			A user running your application can switch between link types, for 
			example, move from one type of network, such as token ring, to another, 
			such as Ethernet, without affecting your program. 
			
*/
/*====================================================================================*/
/*
	IMPORTANT NOTE ON CURRENT CODE
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	This code uses globals to store things like ref nums and stuff, so all of these
	routines can only operate on one connection at a time.  Do not call StartNetwork()
	twice or things won't work!!
	
	Stuff to come (not yet implemented):
		buffer status routines
		better connection handling (connection listeners)
		multiple names
		multiple connections
*/
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
/*
	Example Code
	~~~~~~~~~~~~
	Set one or the other of these to compile example main() routines below,
	or set both of them to zero for no example code.  See comments in example code
	for more info.
*/
#define COMPILE_EXAMPLE_SERVER    0		// set to 1 to compile example server code below
#define COMPILE_EXAMPLE_CLIENT    0		// set to 1 to compile example client code below
/*====================================================================================*/
/*
	Sanity Checks
	~~~~~~~~~~~~~
	Set this value to 1 to compile in sanity checks, that do some rudimentary checking
	of parameter values, and make sure that routines are called in a good ordering.
*/
#define SANITY_CHECKS    1		// set to 1 to compile sanity check code
/*====================================================================================*/
/*
	Beeps
	~~~~~
	To beep when a connection is made or broken, set this value to one.
*/
#define BEEP    1		// set to 1 to beep
/*====================================================================================*/


/*====================================================================================*/
/* GLOBALS */
/*====================================================================================*/
static TRCCB gMyCCB;						// our CCB, defined in <ADSP.h>
											// see IM-Networking page 5-6
static short gADSPDrvrRefNum;				// ADSP driver reference number (refnum)
static short gMPPRefNum;					// MPP driver reference number (refnum)
static short gCCBRefNum;					// our CCB reference number (refnum)
static Ptr gDspSendQPtr = nil;				// ADSP use only
static Ptr gDspRecvQPtr = nil;				// ADSP use only
static Ptr gDspAttnBufPtr = nil;			// ADSP use only
static Boolean gStartedNetwork = false;		// true if started network; mainly used for sanity checks
static Boolean gOpenedConnection = false;	// true if opened connection; mainly used for sanity checks
static NamesTableEntry gMyNTEName;			// our name table entry, defined in <AppleTalk.h>

/*====================================================================================*/
/* INTERNAL ROUTINE PROTOTYPES */
/* note that internal routine names begin with a lowercase letter */
/*====================================================================================*/
/* internal routine for looking up names */
OSErr findASocket(Str32 theName, Str32 theType, AddrBlock *myAddrBlk);

/* internal routine for receiving attention messages */
OSErr receiveAttentionMessage(UInt16 *attentionCode, UInt16 *attentionSize, Ptr *attentionData);		// Ptr to attention message data


/*====================================================================================*/
/* IMPLEMENTATION */
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
Boolean IsNetworkStarted() { return gStartedNetwork; }
Boolean IsConnectionOpened() { return gOpenedConnection; }

/*====================================================================================*/
/*
	This routine initializes the network code.  It should be called once at startup.
	Pass in Pascal strings describing the name and type of this network process.
	These strings are looked for in the OpenNetworkConnection routine below.  Other than
	that they have no significance.  Note that the strings must be unique across the zone, 
	so the server will register its names and call WaitForNetworkConnection, while the 
	client will register *different* names and call OpenNetworkConnection with the 
	*server's* names in order to link up with the server.
	
	theName and theType must be *AT MOST* 32 bytes long.  That is, they must be 
	Str32's.  They must also be *Pascal* strings.  If the strings do not fit
	these descriptions, bad things will most likely happen.
	
	This routine can take several seconds to execute.  This is because the 
	Apple routine PRegisterName() is extremely slow.
	
	If an error occurs, it returns the error code.  If we get an error on this routine,
	then forget about calling the other network routines.
*/
OSErr StartNetwork(Str32 theName, 			// name to give this socket 
					Str32 theType,			// type to give this socket
					UInt32 queue_size)		// size of the queue to use
{
DSPParamBlock myDSPPB;		// our ADSP parameter block, defined in <ADSP.h>
MPPParamBlock myMPPPB;		// our MPP parameter block, defined in <AppleTalk.h>
OSErr err = noErr;                                                                                                                    

#if SANITY_CHECKS
	if (theName == nil)		
		return ERR_BOGUS_PARAMETER;
	if (theType == nil)		
		return ERR_BOGUS_PARAMETER;
	if (queue_size < minDSPQueueSize)	
		return ERR_BOGUS_PARAMETER;
#endif // SANITY_CHECKS

	/*------*/
	// (Numbers) in parens are based on list in IM-Networking, Page 5-13
	/*------*/
	
	/*------*/
	// (1)
	// Open drivers

	// Open AppleTalk drivers.
	// Most of the time, these drivers will already be open.  Calling OpenDriver() in
	// that case will simply give us the ref nums that we need to have.

	err = OpenDriver("\p.MPP", &gMPPRefNum);		// open .MPP driver & get refnum
	if (err != noErr) return err;					// check error
	err = OpenDriver("\p.DSP", &gADSPDrvrRefNum);	// open .DSP driver & get refnum
	if (err != noErr) return err;					// check error
	
	/*------*/
	// (2)
	// Allocate nonrelocatable memory for a CCB, send/receive/attention queues

	// Allocate memory for data buffers.
	gDspSendQPtr = nil;
	gDspRecvQPtr = nil;
	gDspAttnBufPtr = nil;
	
	// make the send and receive queue buffers.  We can make these any size larger than
	// minDSPQueueSize, which is defined in <ADSP.h>.  The larger the queue, the better
	// the network performance generally.  See IM-Networking page 5-13, number (2).  
	
	gDspSendQPtr = NewPtr(queue_size);			// create queue for ADSP use only 
	if (gDspSendQPtr == nil) { err = memFullErr; goto ERROR; }
	
	gDspRecvQPtr = NewPtr(queue_size);			// create queue for ADSP use only 
	if (gDspRecvQPtr == nil) { err = memFullErr; goto ERROR; }
	
	// make attention buffer -- this must be exactly attnBufSize bytes large. 
	gDspAttnBufPtr = NewPtr(attnBufSize);	// create queue for ADSP use only 
											// (attnBufSize defined in <ADSP.h>)
	if (gDspAttnBufPtr == nil) { err = memFullErr; goto ERROR; }

	/*------*/
	// (3)
	// Use dspInit to establish the connection end
	// Provide pointers to the CCB, send queue, receive queue, and attention queue,
	// and a user routine to call when connection end receives a connection event
	// We can specify a socket number in localSocket, or use 0 to let ADSP choose a socket
	
	// set up dspInit parameters
	
	myDSPPB.ioCompletion = nil;							// no completion routine
	myDSPPB.ioCRefNum = gADSPDrvrRefNum;				// ADSP driver ref num
	myDSPPB.csCode = dspInit;							// issue an init call
	myDSPPB.u.initParams.ccbPtr = (TPCCB)&gMyCCB;		// pointer to CCB
	myDSPPB.u.initParams.userRoutine = nil;				// no user completion routine
	myDSPPB.u.initParams.sendQSize = queue_size;		// size of send queue
	myDSPPB.u.initParams.sendQueue = gDspSendQPtr;		// send-queue buffer
	myDSPPB.u.initParams.recvQSize = queue_size;		// size of receive queue
	myDSPPB.u.initParams.recvQueue = gDspRecvQPtr;		// receive-queue buffer
	myDSPPB.u.initParams.attnPtr = gDspAttnBufPtr;		// receive-attention buffer
	myDSPPB.u.initParams.localSocket = 0;				// let ADSP assign socket
	err = PBControl((ParmBlkPtr)&myDSPPB, FALSE);

	// use the PBControl err if there is one; otherwise use ioResult err code	
	if (err == noErr)
		err = myDSPPB.ioResult;

	if (err != noErr) goto ERROR;						// check error

	// store our global CCB ref num
	gCCBRefNum = myDSPPB.ccbRefNum;						// save CCB ref num for later

	/*------*/
	// (4)
	// use the Name-Binding Protocol (NBP) routines to add the name and address 
	// of our connection end to the node�s names table.  We do this so the client
	// end can use PLookupName to find us.  It then will have an AddrBlock with our
	// address that it can pass when it calls dspOpen.
	
	// set up PRegisterName parameters
	// register our name
	NBPSetNTE((Ptr)&gMyNTEName, theName, theType,
				 "\p*", myDSPPB.u.initParams.localSocket);	// set up NBP names table entry
	
	myMPPPB.NBPinterval = 7;						// retransmit every 7*8=56 ticks
	myMPPPB.NBPcount = 3;							// retry 3 times
	myMPPPB.NBPentityPtr = (Ptr)&gMyNTEName;		// name to register
	myMPPPB.NBPverifyFlag = 1;						// verify this name
	err = PRegisterName((MPPPBPtr)&myMPPPB, FALSE);	// register this socket
	
	if (err == noErr)
		err = myMPPPB.MPPioResult;					// check error

	if (err != noErr) goto ERROR;					// check error

	/*------*/
	// (5)
	// (Optional) use dspOptions to set several parameters (blocking factor, etc.)
	// We don't do this (for now), we use the defaults.

	/*------*/
	// if we get here, there was no error and we have started the network
	gStartedNetwork = true;
	return noErr;
	
	// return if no error, otherwise....
		
ERROR:
	if (gDspSendQPtr != nil) DisposPtr(gDspSendQPtr);
	if (gDspRecvQPtr != nil) DisposPtr(gDspRecvQPtr);
	if (gDspAttnBufPtr != nil) DisposPtr(gDspAttnBufPtr);
	gDspSendQPtr = nil;
	gDspRecvQPtr = nil;
	gDspAttnBufPtr = nil;
	return err;
}


/*====================================================================================*/
/*
	This routine closes down the network code.  It should be called once at the end
	of the program.  
	If an error occurs, it returns the error code.
*/
OSErr StopNetwork()
{
MPPParamBlock myMPPPB;		// our MPP parameter block (defined in <AppleTalk.h>) 
OSErr err = noErr;

#if SANITY_CHECKS
	if (!gStartedNetwork)
		return ERR_NETWORK_NOT_STARTED;
#endif // SANITY_CHECKS

	// unregister the name 
	myMPPPB.MPPioCompletion = nil;					// no completion routine
	myMPPPB.MPPioRefNum = gMPPRefNum;				// the .MPP driver refnum
	// myMPPPB.MPPcsCode = filled in automatically	
	
	// fill in the name to remove
	
	myMPPPB.NBPentityPtr = (Ptr)&gMyNTEName.nt.entityData;		// name to remove
	err = PRemoveName(&myMPPPB, false);
	
	if (err == noErr)
		err = myMPPPB.MPPioResult;					// check error
	
	/*------*/
	// Dispose of our queues etc.
	if (gDspSendQPtr != nil) DisposPtr(gDspSendQPtr);
	if (gDspRecvQPtr != nil) DisposPtr(gDspRecvQPtr);
	if (gDspAttnBufPtr != nil) DisposPtr(gDspAttnBufPtr);
	
	gStartedNetwork = false;
	
	// IMPORTANT NOTE:  Note that we do *NOT* call MPPClose() or any other routine to
	// close down AppleTalk.  This would be a bad thing, because other processes
	// might be using AppleTalk.  See IM-Networking, pages 2-18 through 2-20.
	
	return err;
}

/*====================================================================================*/
/*
	Server Open-Connection Routine
	This routine halts and waits for a connection from another process.  It will wait
	for secondsToWait seconds at most.  secondsToWait must be between 1 and 255.  If
	secondsToWait is 255, then this routine will wait indefinitely (or use the constant 
	WAIT_FOREVER).  If a connection is made, this routine returns 0 (noErr).  If an 
	error occurs (or no connection is made), it returns the error code.
*/
OSErr WaitForNetworkConnection(UInt8 secondsToWait)	// seconds to wait for connection, or WAIT_FOREVER
{
DSPParamBlock myDSPPB;					// our ADSP parameter block (defined in <ADSP.h>) 
AddrBlock any_address = { 0, 0, 0 };	// a filter address that doesn't filter at all
OSErr err = noErr;

#if SANITY_CHECKS
	if (!gStartedNetwork)
		return ERR_NETWORK_NOT_STARTED;
#endif // SANITY_CHECKS
	
	// fix up secondsToWait to be a valid value	
	if ((secondsToWait <= 0) || (secondsToWait > 255))
		secondsToWait = WAIT_FOREVER;

	/*------*/
	// (6)
	// Open (actually wait) for a connection
	/*
		Use the ocPassive mode when you expect to receive an open-connection request from
		a remote socket. You can specify a value for the filterAddress parameter to
		restrict the network number, node ID, or socket number from which you will accept
		an open-connection request. When your connection end receives an open-connection
		request that meets the restrictions of the filterAddress parameter, it
		acknowledges the request and ADSP completes the connection. You can poll the
		state field in the CCB to determine when the connection end is waiting to receive
		an open-connection request, when the connection end is waiting to receive an
		acknowledgment of an open-connection request, and when the connection is open.
		See the section �The ADSP Connection Control Block Record� beginning on page 5-35
		for a description of the CCB fields. Alternatively, you can check the result code
		for the dspOpen routine when the routine completes execution. If the routine
		returns the noErr result code, then the connection is open.

		The ocPassive mode, in which the connection end waits to receive an open-
		connection request from a remote connection end. You can use the filterAddress 
		parameter to restrict the addresses from which you will accept a connection request.
		The dspOpen routine completes execution in the ocPassive mode when ADSP establishes 
		a connection or when either connection end receives a connection denial.

		For ocPassive mode, we need to pass in the following:
			ioCompletion
			ioCRefNum
			csCode
			ccbRefNum
			filterAddress
			ocMode
			ocInterval
			ocMaximum
			
			and we get back:
			
			ioResult
			localCID
			remoteCID
			remoteAddress
			sendSeq
			sendWindow
			attnSendSeq
	*/
	
	// note that we pass a blank (i.e. zeroed-out) filterAddress, meaning don't filter
	// anything -- accept a connection from any address.  We could try to find our client
	// if we wanted to and filter for that specific address here.
	
	myDSPPB.ioCompletion = nil;							// no completion routine
	myDSPPB.ioCRefNum = gADSPDrvrRefNum;				// ADSP driver ref num
	myDSPPB.csCode = dspOpen;							// try to open a connection
	myDSPPB.ccbRefNum = gCCBRefNum;						// connection ref num
	myDSPPB.u.openParams.filterAddress = any_address;	// address filter
	myDSPPB.u.openParams.ocMode = ocPassive; 			// open connection mode (wait)
	myDSPPB.u.openParams.ocInterval = 0; 				// use default retry interval = 1 second
	myDSPPB.u.openParams.ocMaximum = secondsToWait; 	// retry maximum (255 == indefinitely)
	
	/*
		open a connection -- note that the asynch parameter is FALSE, which means that
		this routine will not return until a connection is made or secondsToWait seconds
		have elapsed.  (note if secondsToWait == 255 (the constant WAIT_FOREVER), this routine 
		won't return until connection is established)
	*/
	err = PBControl((ParmBlkPtr)&myDSPPB, FALSE);		

	// use the PBControl err if there is one; otherwise use ioResult err code	
	if (err == noErr)
		err = myDSPPB.ioResult;
	
	if (err == noErr)
	{
		// we got no error, so we got a connection
		gOpenedConnection = true;
		
		#if BEEP
		SysBeep(2);
		#endif
	}
	
	// if err is noErr here, then we got a connection
	
	return err;
}

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
							UInt8 secondsToWait)	// seconds to wait for connection, or WAIT_FOREVER
{
DSPParamBlock myDSPPB;		// our ADSP parameter block, defined in <ADSP.h>
AddrBlock myAddrBlk;		// net/node/socket address specifier, defined in <AppleTalk.h>
OSErr err = noErr;

#if SANITY_CHECKS
	if (!gStartedNetwork)
		return ERR_NETWORK_NOT_STARTED;
	if (theName == nil)
		return ERR_BOGUS_PARAMETER;
	if (theType == nil)
		return ERR_BOGUS_PARAMETER;
#endif // SANITY_CHECKS
	
	// fix up secondsToWait to be a valid value	
	if ((secondsToWait <= 0) || (secondsToWait > 255))
		secondsToWait = WAIT_FOREVER;

	/*------*/
	// (6)
	// Open a connection with the server
	/*
		To use the ocRequest mode, you must know the complete internet address of the 
		remote socket, and the ADSP client at that address must either be a connection 
		listener or have executed the dspOpen routine in the ocPassive mode. You can use 
		the NBP routines to obtain a list of names of objects on the internet and to 
		determine the internet address of a socket when you know its name. See the 
		chapter �Name-Binding Protocol (NBP)� in this book for information on the NBP 
		routines.
	*/
	
	err = findASocket(theName, theType, &myAddrBlk);	// find the server with NBP routines
	if (err != noErr) return err;
	
	// if we get here, myAddrBlk should be filled with the address of our server
	
	// Open a connection with the selected socket.
	// set up dspOpen parameters
	
	/*
		For ocPassive mode, we need to pass in the following:
			ioCompletion
			ioCRefNum
			csCode
			ccbRefNum
			remoteAddress		(which we got from findASocket)
			filterAddress
			ocMode
			ocInterval
			ocMaximum
			
			and we get back:
			
			ioResult
			localCID
			remoteCID
			remoteAddress
			sendSeq
			sendWindow
			attnSendSeq
	*/
	
	myDSPPB.ioCompletion = nil;							// no completion routine
	myDSPPB.ioCRefNum = gADSPDrvrRefNum;				// ADSP driver ref num
	myDSPPB.csCode = dspOpen;							// issue an open call
	myDSPPB.ccbRefNum = gCCBRefNum;						// connection ref num
	myDSPPB.u.openParams.remoteAddress = myAddrBlk;		// address of remote socket
														// from findASocket function
	myDSPPB.u.openParams.filterAddress = myAddrBlk;		// address filter; use specified	
														// socket address only
	myDSPPB.u.openParams.ocMode = ocRequest; 			// open connection mode
	myDSPPB.u.openParams.ocInterval = 0; 				// use default retry interval = 1 second
	myDSPPB.u.openParams.ocMaximum = secondsToWait; 	// retry maximum (255 == indefinitely)
	err = PBControl((ParmBlkPtr)&myDSPPB, FALSE);		// open a connection

	// use the PBControl err if there is one; otherwise use ioResult err code	
	if (err == noErr)
		err = myDSPPB.ioResult;
	
	if (err == noErr) 
	{
		// we got no error, so we got a connection
		gOpenedConnection = true;

		#if BEEP
		SysBeep(2);
		#endif
	}
	
	return err;	
}

/*====================================================================================*/
/*
	This routine permanently closes an open connection.  
	All data to be read/written is aborted immediately.
	If an error occurs, it returns the error code.
*/
OSErr CloseNetworkConnection()
{
DSPParamBlock myDSPPB;		// our ADSP parameter block, defined in <ADSP.h>
OSErr err = noErr;

#if SANITY_CHECKS
	if (!gStartedNetwork)
		return ERR_NETWORK_NOT_STARTED;
	if (!gOpenedConnection)
		return ERR_CONNECTION_NOT_OPENED;
#endif // SANITY_CHECKS

	/*------*/
	// (8)
	// Remove the connection
	// We're finished with the connection, so remove it.
	// set up dspRemove parameters
	
	myDSPPB.ioCompletion = nil;							// no completion routine
	myDSPPB.ioCRefNum = gADSPDrvrRefNum;				// ADSP driver ref num
	myDSPPB.csCode = dspRemove;							// remove the connection
	myDSPPB.ccbRefNum = gCCBRefNum;						// connection ref num
	myDSPPB.u.closeParams.abort = 1;					// close immediately (�DEBUG�)
	err = PBControl((ParmBlkPtr)&myDSPPB, FALSE);		// close and remove the connection

	// use the PBControl err if there is one; otherwise use ioResult err code	
	if (err == noErr)
		err = myDSPPB.ioResult;

	if (err != noErr) 									// check error
		return err;			
	
	gOpenedConnection = false;
	return err;		// err is noErr here
}


/*====================================================================================*/
/*
	Routine to check for spurious network events (i.e attention messages, closed 
	connections, etc).  This routine should be called often (like once every main
	event loop).  Most of the time it is fast -- it just checks a few flags.  If there
	are any pending events though, it will receive them and possibly close down the
	connection if necessary.

	Returns TRUE if an attention message was received.  If this is the case, then
	attentionCode, attentionSize, attentionData are all filled in (if they are not
	nil -- all those parameters are optional).
*/
Boolean NetworkIdle(UInt16 *attentionCode, 		// attention message code (can be nil)
					UInt16 *attentionSize,		// size of attention message data (can be nil)
					Ptr *attentionData)			// Ptr to attention message data (can be nil)
{
UInt8 flags = gMyCCB.userFlags;

	if (!gStartedNetwork)
		return false;
	if (!gOpenedConnection)
		return false;

	if (flags & eAttention)
	{
	OSErr err = receiveAttentionMessage(attentionCode, attentionSize, attentionData);

		// clear out the user flag to show that we've received the message
		BitClr(&gMyCCB.userFlags, 2);
		
		return true;
	}
	else if (flags & eClosed)
	{	
		#if BEEP
		SysBeep(2);
		SysBeep(2);
		#endif

		// we must clear the bit after we have used it or we wont get any more messages 
		BitClr(&gMyCCB.userFlags, 0);
		CloseNetworkConnection();
	}
	else if (flags & eTearDown)
	{
		// after two minutes or so if the connection has broken we'll be notified by 
		// the .DSP driver with an eTearDown message 
		#if BEEP
		SysBeep(2);
		SysBeep(2);
		#endif

		// we must clear the bit after we have used it or we wont get any more messages 
		BitClr(&gMyCCB.userFlags, 1);
		CloseNetworkConnection();
	}

	return false;
}

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
OSErr ReadData(Ptr readBuffer, 			// buffer to fill
			UInt16 readBufferSize,		// number of bytes we expect (i.e. size of read buffer)
			UInt16 *actualBytesRead,	// returns number of bytes read this time (can be nil)
			UInt8 *noMoreData)			// returns 1 if there is no more data to read (can be nil)
										// (i.e. it's the end of this message)
{
DSPParamBlock myDSPPB;		// our ADSP parameter block, defined in <ADSP.h>
OSErr err = noErr;

	// fill in these values first in case there is an error, the caller will know
	// that he received no data
	if (actualBytesRead)
		*actualBytesRead = 0;
	if (noMoreData)
		*noMoreData = 1;

#if SANITY_CHECKS
	if (!gStartedNetwork)
		return ERR_NETWORK_NOT_STARTED;
	if (!gOpenedConnection)
		return ERR_CONNECTION_NOT_OPENED;
	if (readBuffer == nil)
		return ERR_BOGUS_PARAMETER;
#endif // SANITY_CHECKS

	// check gMyCCB.state here to make sure we have an open connection
	if (gMyCCB.state != sOpen)
		return ERR_CONNECTION_NOT_OPENED;

	// check gMyCCB.userFlags here to make sure we have an open connection
	// if userFlags & eClosed, then the connection was just closed by remote connection
	// theoretically we could still read, but this is stopgap measure (�DEBUG�)
	if (gMyCCB.userFlags & eClosed)
		return ERR_CONNECTION_NOT_OPENED;

	// (7)
	// Read Data 
	// Use the dspRead routine to read data that your connection end has received from 
	// the remote connection end. 
	
	// issue a status call, and only read if there are bytes to be read
	myDSPPB.ioCompletion = nil;							// no completion routine
	myDSPPB.ioCRefNum = gADSPDrvrRefNum;				// ADSP driver ref num
	myDSPPB.csCode = dspStatus;							// get connection status
	myDSPPB.ccbRefNum = gCCBRefNum;						// connection ref num
	err = PBControl((ParmBlkPtr)&myDSPPB, FALSE);		// get connection status

	// use the PBControl err if there is one; otherwise use ioResult err code	
	if (err == noErr)
		err = myDSPPB.ioResult;
		
	// if there was an error, go to error
	if (err != noErr)
		goto ERROR;
	
	// only issue a read call if there is data to be read 
	if (myDSPPB.u.statusParams.recvQPending > 0)
	{
		myDSPPB.ioCompletion = nil;						// no completion routine
		myDSPPB.ioCRefNum = gADSPDrvrRefNum;			// ADSP driver ref num
		myDSPPB.csCode = dspRead;						// issue a read
		myDSPPB.ccbRefNum = gCCBRefNum;					// connection ref num
		myDSPPB.u.ioParams.reqCount = readBufferSize;	// read this number of bytes
		myDSPPB.u.ioParams.dataPtr = readBuffer;		// pointer to read buffer
		err = PBControl((ParmBlkPtr)&myDSPPB, FALSE);	// read data from the remote connection
	
		// use the PBControl err if there is one; otherwise use ioResult err code	
		if (err == noErr)
			err = myDSPPB.ioResult;
		
		if (err != noErr)
		{
			if (actualBytesRead != nil)
				*actualBytesRead = 0;
			if (noMoreData != nil)
				*noMoreData = 1;
			goto ERROR;
		}
	
		if (actualBytesRead != nil)
			*actualBytesRead = myDSPPB.u.ioParams.actCount;
		if (noMoreData != nil)
			*noMoreData = myDSPPB.u.ioParams.eom;
	}
	else	// no data to be read
	{
		// no bytes to be read, so fill in these values
		if (actualBytesRead)
			*actualBytesRead = 0;
		if (noMoreData)
			*noMoreData = 1;
	}

ERROR:	
	return err;
}

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
OSErr WriteData(Ptr writeBuffer, 		// buffer to write
			UInt16 writeBufferSize,		// number of bytes to write
			UInt8 endOfMessage,			// pass 1 if this is the end of this message
			UInt8 flush,				// pass 1 if we are to send data immediately
			UInt16 *actualBytesWritten)	// returns number of bytes written this time (can be nil)
{
DSPParamBlock myDSPPB;		// our ADSP parameter block, defined in <ADSP.h>
OSErr err = noErr;

#if SANITY_CHECKS
	if (!gStartedNetwork)
		return ERR_NETWORK_NOT_STARTED;
	if (!gOpenedConnection)
		return ERR_CONNECTION_NOT_OPENED;
	if (writeBuffer == nil)
		return ERR_BOGUS_PARAMETER;
#endif // SANITY_CHECKS

	// check gMyCCB.state here to make sure we have an open connection
	if (gMyCCB.state != sOpen)
		return ERR_CONNECTION_NOT_OPENED;

	// check gMyCCB.userFlags here to make sure we have an open connection
	// if userFlags & eClosed, then the connection was just closed by remote connection
	// theoretically we could still read, but this is stopgap measure (�DEBUG�)
	if (gMyCCB.userFlags & eClosed)
		return ERR_CONNECTION_NOT_OPENED;
	
	myDSPPB.ioCompletion = nil;							// no completion routine
	myDSPPB.ioCRefNum = gADSPDrvrRefNum;				// ADSP driver ref num
	myDSPPB.csCode = dspWrite;							// issue a write call
	myDSPPB.ccbRefNum = gCCBRefNum;						// connection ref num
	myDSPPB.u.ioParams.reqCount = writeBufferSize;		// write this number of bytes
	myDSPPB.u.ioParams.dataPtr = writeBuffer;			// pointer to send queue
	myDSPPB.u.ioParams.eom = endOfMessage;				// 1 means last byte is
														// logical end-of-message
	myDSPPB.u.ioParams.flush = flush;					// 1 means send data now
	err = PBControl((ParmBlkPtr)&myDSPPB, FALSE);		// send data to the remote connection

	// use the PBControl err if there is one; otherwise use ioResult err code	
	if (err == noErr)
		err = myDSPPB.ioResult;

	if (actualBytesWritten != nil)
		*actualBytesWritten = myDSPPB.u.ioParams.actCount;

ERROR:	
	return err;
}

/*====================================================================================*/
/*
	Routine to reset/resynchronize the connection.  
	This routine does not operate asychronously.  
	This routine clears all data that the remote client has not already read and
	resynchronizes the connection. 
*/
OSErr ResetNetwork()
{
DSPParamBlock myDSPPB;		// our ADSP parameter block, defined in <ADSP.h>
OSErr err = noErr;

#if SANITY_CHECKS
	if (!gStartedNetwork)
		return ERR_NETWORK_NOT_STARTED;
	if (!gOpenedConnection)
		return ERR_CONNECTION_NOT_OPENED;
#endif // SANITY_CHECKS
	
	myDSPPB.ioCompletion = nil;							// no completion routine
	myDSPPB.ioCRefNum = gADSPDrvrRefNum;				// ADSP driver ref num
	myDSPPB.csCode = dspReset;							// issue a reset call
	myDSPPB.ccbRefNum = gCCBRefNum;						// connection ref num
	err = PBControl((ParmBlkPtr)&myDSPPB, FALSE);		// reset remote connection

	// use the PBControl err if there is one; otherwise use ioResult err code	
	if (err == noErr)
		err = myDSPPB.ioResult;
	
	return err;
}


/*====================================================================================*/
/*
	Routine to send an attention message.  
	attentionCode may be any value from 0x0000 through 0xEFFF.  Attention code values
	0xF000 through 0xFFFF are reserved by ADSP.  What these codes mean is totally
	application-dependent.
	attentionSize is the size of the data pointed to by attentionData, up to 570 bytes.
	attentionData is a pointer to the attention message data, which can be up to 570
	bytes in length.  (Actually, it can be any size, but only the first 570 bytes will
	be sent.)
*/
OSErr SendAttentionMessage(UInt16 attentionCode, 	// attention message code
							UInt16 attentionSize,	// size of attention message data
							Ptr attentionData)		// Ptr to attention message data
{
DSPParamBlock myDSPPB;		// our ADSP parameter block, defined in <ADSP.h>
OSErr err = noErr;

#if SANITY_CHECKS
	if (!gStartedNetwork)
		return ERR_NETWORK_NOT_STARTED;
	if (!gOpenedConnection)
		return ERR_CONNECTION_NOT_OPENED;
	if (attentionSize > 570)
		return ERR_BOGUS_PARAMETER;
	if ((attentionCode >= (UInt16)0xF000) && 
		(attentionCode <= (UInt16)0xFFFF))
		return ERR_BOGUS_PARAMETER;		
#endif // SANITY_CHECKS

	// check gMyCCB.state here to make sure we have an open connection
	if (gMyCCB.state != sOpen)
		return ERR_CONNECTION_NOT_OPENED;
	
	myDSPPB.ioCompletion = nil;							// no completion routine
	myDSPPB.ioCRefNum = gADSPDrvrRefNum;				// ADSP driver ref num
	myDSPPB.csCode = dspAttention;						// issue an attention call
	myDSPPB.ccbRefNum = gCCBRefNum;						// connection ref num
	myDSPPB.u.attnParams.attnCode = attentionCode;		// attention message code
	myDSPPB.u.attnParams.attnSize = attentionSize;		// attention message data size
	myDSPPB.u.attnParams.attnData = attentionData;		// attention message data ptr

	err = PBControl((ParmBlkPtr)&myDSPPB, FALSE);		// send data to the remote connection

	// use the PBControl err if there is one; otherwise use ioResult err code	
	if (err == noErr)
		err = myDSPPB.ioResult;
	
	return err;
}

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
*/
OSErr SetNetworkOptions(UInt16 sendBlocking,	// quantum for data packets 
						UInt8 badSeqMax,		// threshold for sending retransmit advice 
						UInt8 useCheckSum)		// set to 1 to use DDP packet checksum 
{
DSPParamBlock myDSPPB;		// our ADSP parameter block, defined in <ADSP.h>
OSErr err = noErr;

#if SANITY_CHECKS
	if (!gStartedNetwork)
		return ERR_NETWORK_NOT_STARTED;
	if (gOpenedConnection)
		return ERR_TOO_LATE_TO_SET_OPTIONS;
#endif // SANITY_CHECKS
	
	myDSPPB.ioCompletion = nil;							// no completion routine
	myDSPPB.ioCRefNum = gADSPDrvrRefNum;				// ADSP driver ref num
	myDSPPB.csCode = dspOptions;						// issue an options call
	myDSPPB.ccbRefNum = gCCBRefNum;						// connection ref num
	myDSPPB.u.optionParams.sendBlocking = sendBlocking;	// quantum for data packets
	myDSPPB.u.optionParams.badSeqMax = badSeqMax;		// threshold for sending retransmit advice
	myDSPPB.u.optionParams.useCheckSum = useCheckSum;	// use DDP packet checksum

	err = PBControl((ParmBlkPtr)&myDSPPB, FALSE);		// set the options

	// use the PBControl err if there is one; otherwise use ioResult err code	
	if (err == noErr)
		err = myDSPPB.ioResult;
	
	return err;
}


/*====================================================================================*/
/* INTERNAL CONSTANTS */
/*====================================================================================*/
enum {
	BIG_BUFFER 	= 10000		// big, ugly buffer - for NBP lookups
};

/*====================================================================================*/
/* INTERNAL-USE-ONLY ROUTINES BELOW */
/*====================================================================================*/

/*====================================================================================*/
/*
	Routine to receive and acknowledge an attention message.  
	This routine *MUST* be called if IsAttentionRequested() returns true
*/
OSErr receiveAttentionMessage(UInt16 *attentionCode, 	// attention message code
							UInt16 *attentionSize,		// size of attention message data
							Ptr *attentionData)			// Ptr to attention message data
{
#if SANITY_CHECKS
	if (!gStartedNetwork)
		return ERR_NETWORK_NOT_STARTED;
	if (!gOpenedConnection)
		return ERR_CONNECTION_NOT_OPENED;
#endif // SANITY_CHECKS

	
	if ((gMyCCB.state == sOpen) && 			// make sure we have an open connection
		(gMyCCB.userFlags & eAttention))	// make sure that someone requested our attention
	{
		if (attentionCode != nil)					// optional parameter, check for nil
			*attentionCode = gMyCCB.attnCode;
		if (attentionSize != nil)					// optional parameter, check for nil
			*attentionSize = gMyCCB.attnSize;
		if (attentionData != nil)					// optional parameter, check for nil
			*attentionData = (Ptr)gMyCCB.attnPtr;
	}
	else
	{
		return ERR_NO_ATTENTION_MESSAGE;
	}
	
	return noErr;
}

/*====================================================================================*/
/*
	Internal routine for finding a socket (i.e. filling out an AddrBlock) with the
	specified name and type.  This routine only returns the *first* such socket 
	found, so don't call this routine if you want more than one. 
	
	theName and theType must be *AT MOST* 32 bytes long.  That is, they must be 
	Str32's.  They must also be *Pascal* strings.  If the strings do not fit
	these descriptions, bad things will most likely happen.
*/
OSErr findASocket(Str32 theName, Str32 theType, AddrBlock *myAddrBlk)
{
Str32 NBPZone = "\p*";			// search local zone only
NamesTableEntry lookupEntity;	// the name to look up, struct defined in <AppleTalk.h> 
Ptr NBPLookupBuffer = nil; 		// totally gross mondo buffer for returned names
MPPParamBlock pbLKP;			// our MPP parameter block, struct defined in <AppleTalk.h> 
EntityName abEntity;			// name we will fill in, struct defined in <AppleTalk.h> 
OSErr resultCode;				// result returned by NBPExtract()
OSErr err = noErr;

#if SANITY_CHECKS
	if (!gStartedNetwork) 	
		return ERR_NETWORK_NOT_STARTED;
	if (theName == nil) 	
		return ERR_BOGUS_PARAMETER;
	if (theType == nil)	 	
		return ERR_BOGUS_PARAMETER;
	if (myAddrBlk == nil) 	
		return ERR_BOGUS_PARAMETER;
#endif // SANITY_CHECKS

	NBPLookupBuffer = nil;
	NBPLookupBuffer = NewPtr(BIG_BUFFER);
	if (NBPLookupBuffer == nil) { err = memFullErr; goto ERROR; }

	NBPSetEntity((Ptr)&lookupEntity.nt.entityData, theName, theType, NBPZone);
	
	pbLKP.MPPioCompletion = nil;
	// pbLKP.MPPioRefNum = filled in automatically
	// pbLKP.MPPcsCode = filled in automatically
	pbLKP.NBPinterval = 3;				// retry interval in 8-tick units (3 * 8 = 24 ticks)
	pbLKP.NBPcount = 3;				// max # of retries
	pbLKP.NBPentityPtr = (Ptr)&lookupEntity.nt.entityData;
	pbLKP.NBPretBuffSize = BIG_BUFFER;
	pbLKP.NBPretBuffPtr = NBPLookupBuffer;
	pbLKP.NBPmaxToGet = (BIG_BUFFER / sizeof(NTElement));

	err = PLookupName(&pbLKP, false);
	if (err == noErr)
	{
		// if we found any...
		if (pbLKP.NBPnumGotten > 0)
		{
			// just get the first one -- the 1 should be a loop with i = 1 to pbLKP.NBPnumGotten
			// if we want more than one, and we should fill in multiple address blocks 
			
			// this fills in myAddrBlk
			resultCode = NBPExtract(NBPLookupBuffer, pbLKP.NBPnumGotten, 1, &abEntity, myAddrBlk);
		}
		else
			return ERR_COULD_NOT_FIND_SERVER;		// we didn't find any!
	}
	else
		return err;

ERROR: 
	if (NBPLookupBuffer != nil) DisposPtr(NBPLookupBuffer);
	return err;
}



/*====================================================================================*/
/* EXAMPLE ROUTINES BELOW */
/*====================================================================================*/

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
	
	// server starts network with its unique name
	err = StartNetwork(SERVER_NAME_STR32, SERVER_TYPE_STR32, DEFAULT_QUEUE_SIZE);
	if (err) goto ERROR;
	
	// server waits for connection
	err = WaitForNetworkConnection(WAIT_FOREVER);
	if (err) goto ERROR;

	// NOTE: client should call OpenNetworkConnection() here
	
	// if we get here, we have a connection

	// NOTE: client should call WriteData() here
	
	// read the sample data
	err = ReadData(myData2ReadPtr, 		// buffer to fill
			BUFFER_SIZE,				// number of bytes we expect (i.e. size of read buffer)
			&actualBytesRead,			// returns number of bytes read this time (can be nil)
			&noMoreData);				// returns 1 if there is no more data to read (can be nil)
										// (i.e. it's the end of this message)
	if (err) goto ERROR;

ERROR:
	// close the connection and stop network
	err = CloseNetworkConnection();
	err = StopNetwork();
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
	
	// client starts network with its unique name
	err = StartNetwork(CLIENT_NAME_STR32, CLIENT_TYPE_STR32, DEFAULT_QUEUE_SIZE);
	if (err) goto ERROR;
	
	// NOTE: server should call WaitForNetworkConnection() here
	
	// client opens connection with the waiting server's name and type
	err = OpenNetworkConnection(SERVER_NAME_STR32, SERVER_TYPE_STR32, WAIT_FOREVER);
	if (err) goto ERROR;
	
	// if we get here, we have a connection
	
	// write out some sample data
	err = WriteData(myData2WritePtr, 	// buffer to write
			13,							// number of bytes to write
			1,							// pass 1 if this is the end of this message
			1,							// pass 1 if we are to send data immediately
			&actualBytesWritten);		// returns number of bytes written this time (can be nil)
	if (err) goto ERROR;

	// NOTE: server should call ReadData() here

ERROR:
	// close the connection and stop network
	err = CloseNetworkConnection();
	err = StopNetwork();
	Debugger();
}
#endif // COMPILE_EXAMPLE_CLIENT