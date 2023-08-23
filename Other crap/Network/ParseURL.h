#ifndef _PARSE_URL_H_
#define _PARSE_URL_H_

#include "TCPIntf.h"		// for CStr255, HTTP_PORT, FTP_PORT, etc.

// parse a url into its component parts
OSErr ParseURL(								// parse a URL into protocol/host/path
			CStr255 url,						// INPUT : URL to receive (C string -- must not be nil)
			Byte *protocol,  					// OUTPUT: protocol (can be nil)
			CStr255 host, 						// OUTPUT: host name/IP number as a string (can be nil)
			CStr255 port, 						// OUTPUT: host port as a string (can be nil)
			CStr255 file,						// OUTPUT: path and filename (can be nil)
			CStr255 section,					// OUTPUT: section of file -- usually empty string (can be nil)
			CStr255 username, 					// OUTPUT: username -- usually empty string (can be nil)
			CStr255 password); 					// OUTPUT: password -- usually empty string (can be nil)

enum {						// list of protocols
	PROTOCOL_HTTP = 0,
	PROTOCOL_FTP,
	PROTOCOL_GOPHER,
	PROTOCOL_MAILTO,
	PROTOCOL_NEWS,
	PROTOCOL_NNTP,
	PROTOCOL_TELNET,
	PROTOCOL_WAIS,
	PROTOCOL_MID,
	PROTOCOL_CID,
	PROTOCOL_PROSPERO,
	NUM_PROTOCOLS,
	PROTOCOL_UNKNOWN = 444
};

#endif // _PARSE_URL_H_