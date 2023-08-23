#include "ParseURL.h"
#include <string.h>		// for strlen(), etc.
#include <ctype.h>		// for tolower()

/*====================================================================================*/
#define URL_PREFIX				"url:"
#define URL_SP_PREFIX			"url: "
#define LESS_THAN_PREFIX		"<"
#define HTTP_PREFIX				"http://"
#define FTP_PREFIX				"ftp://"
#define FTP_ALT_PREFIX			"file://"			// [NONSTANDARD?]
#define AFS_PREFIX				"afs://"
#define GOPHER_PREFIX			"gopher://"
#define MAILTO_PREFIX			"mailto:"
#define NEWS_PREFIX				"news:"
#define NNTP_PREFIX				"nntp:"
#define TELNET_PREFIX			"telnet://"
#define WAIS_PREFIX				"wais:"
#define MID_PREFIX				"mid:"
#define CID_PREFIX				"cid:"
#define PROSPERO_PREFIX			"prospero:"
#define CHAR_NULL				'\0'
#define CHAR_SLASH				'/'
#define CHAR_COLON				':'
#define CHAR_GREATER_THAN		'>'
#define CHAR_HASH				'#'
#define CHAR_SPACE				' '
#define CHAR_CR					'\r'
#define CHAR_LF					'\n'
#define CHAR_TAB				'\t'


/*====================================================================================*/
/* INTERNAL PROTOTYPES */
/*====================================================================================*/
Boolean stringMatchPrefix(			// returns true if strings match first parts
			CStr255 str1,
			CStr255 str2);
Boolean is_whitespace(char ch);

/*====================================================================================*/
/* INTERNAL ROUTINES */
/*====================================================================================*/
/*
	Returns TRUE if str1 is a prefix of str2, or if str2 is a prefix of str1, or if
	str1 equals str2.  Basically, returns TRUE if strings match their first parts.
	Keep in mind that this will return TRUE if either string is empty!  However, if
	str1 or str2 is nil, it will return FALSE.
*/
Boolean stringMatchPrefix(			// returns true if strings match
			CStr255 str1,
			CStr255 str2)
{
char *local_str1 = str1, *local_str2 = str2;

	if ((str1 == nil) || (str2 == nil))
		return FALSE;
	do
	{
		if (*local_str1 == '\0')
			return TRUE;
		else if (*local_str2 == '\0')
			return TRUE;	
		else if (*local_str1 != *local_str2)
			return FALSE;
		else
		{
			local_str1++;
			local_str2++;
		}
	}
	while (TRUE);
}

/*====================================================================================*/
/*
	Returns TRUE if str1 is a prefix of str2, or if str2 is a prefix of str1, or if
	str1 equals str2.  Basically, returns TRUE if strings match their first parts.
	Keep in mind that this will return TRUE if either string is empty!  However, if
	str1 or str2 is nil, it will return FALSE.
*/
Boolean is_whitespace(char ch)
{
	return ((ch == CHAR_SPACE) ||
			(ch == CHAR_CR) ||
			(ch == CHAR_LF) ||
			(ch == CHAR_TAB));
}

/*====================================================================================*/
/* EXTERNAL ROUTINES */
/*====================================================================================*/
/*
	Things to do:
		see еее TO-BE-IMPLEMENTED marks -- should probably make separate routines for
		parsing URLs of different types
		check for invalid chars (like spaces) and put %20 or whatever in their place
		for mailto, should the address be returned in host or username or what?
		similarly for other protocols: make a list of what is returned and where
		should do more error checking -- make sure we don't read off the "end" of the
		  input string, if it is bogus or something
		
	Notes: 
		file may be return an empty string (e.g. for URL "http://host.name" )
		file may be return just a slash (e.g. for URL "http://host.name/" )
*/
OSErr ParseURL(								// parse a URL into protocol/host/path
			CStr255 url,						// INPUT : URL to receive (C string -- must not be nil)
			Byte *protocol,  					// OUTPUT: protocol (can be nil)
			CStr255 host, 						// OUTPUT: host name/IP number as a string (can be nil)
			CStr255 port, 						// OUTPUT: host port as a string (can be nil)
			CStr255 file,						// OUTPUT: path and filename (can be nil)
			CStr255 section,					// OUTPUT: section of file -- usually empty string (can be nil)
			CStr255 username, 					// OUTPUT: username -- usually empty string (can be nil)
			CStr255 password) 					// OUTPUT: password -- usually empty string (can be nil)
{
OSErr err = noErr;
char *url_ptr;
int i;
int length;
Boolean found_less_than_prefix = FALSE;
Boolean found_a_hash = FALSE;
Boolean found_a_colon = FALSE;
char *last_part;

	// NULL out all the strings
	if (protocol != nil) 
		*protocol = PROTOCOL_UNKNOWN;
	if (host != nil) 
		host[0] = CHAR_NULL;
	if (port != nil) 
		port[0] = CHAR_NULL;
	if (file != nil) 
		file[0] = CHAR_NULL;
	if (section != nil) 
		section[0] = CHAR_NULL;
	if (username != nil) 
		username[0] = CHAR_NULL;
	if (password != nil) 
		password[0] = CHAR_NULL;

	url_ptr = url;
	if (url_ptr == nil)		// url cannot be nil
	{
		err = -1;
		goto ERROR;
	}
	
	//------
	// 
	length = strlen(url_ptr);
	if (length == 0)
	{
		// zero-length URL? return an err 
		err = -1;
		goto ERROR;
	}

	//------
	// convert entire url to lowercase
	// This assumes plain ASCII; if we really wanted to be robust, we might have to worry 
	// about alternate char sets or something
	for ( i = 0 ; i < length ; i++ )
		url_ptr[i] = tolower(url[i]);

	//------
	// remove white space at the beginning [NONSTANDARD]
	while (is_whitespace(*url_ptr))
		url_ptr++;

	//------
	// remove white space at the end [NONSTANDARD]
	length = strlen(url_ptr);
	while (is_whitespace(url_ptr[length-1]))
	{	
		url_ptr[length-1] = CHAR_NULL;
		length--;
		if (length < 0)		// it's all whitespace!!
		{
			err = -1;
			goto ERROR;
		}
	}
	
	//------
	// skip past "<" at the beginning, if it is there [NONSTANDARD]
	if (stringMatchPrefix(LESS_THAN_PREFIX, url_ptr))
	{
		// remember that we found a greater than prefix
		found_less_than_prefix = TRUE;
		
		// update url_ptr
		url_ptr += strlen(LESS_THAN_PREFIX);
	}

	//------
	// skip past "URL:" at the beginning, if it is there
	if (stringMatchPrefix(URL_PREFIX, url_ptr))
	{
		// update url_ptr
		url_ptr += strlen(URL_PREFIX);
	}
	else if (stringMatchPrefix(URL_SP_PREFIX, url_ptr))		// [NONSTANDARD]
	{
		// update url_ptr
		url_ptr += strlen(URL_SP_PREFIX);
	}
	
	//------
	// look for protocol at the beginning
	if (stringMatchPrefix(HTTP_PREFIX, url_ptr))
	{
		if (protocol != nil) 
			*protocol = PROTOCOL_HTTP;

		// update url_ptr
		url_ptr += strlen(HTTP_PREFIX);
	}
	else if (stringMatchPrefix(FTP_PREFIX, url_ptr))
	{
		if (protocol != nil) 
			*protocol = PROTOCOL_FTP;

		// update url_ptr
		url_ptr += strlen(FTP_PREFIX);
	}
	else if (stringMatchPrefix(FTP_ALT_PREFIX, url_ptr))
	{
		if (protocol != nil) 
			*protocol = PROTOCOL_FTP;

		// update url_ptr
		url_ptr += strlen(FTP_ALT_PREFIX);
	}
	else if (stringMatchPrefix(GOPHER_PREFIX, url_ptr))
	{
		if (protocol != nil) 
			*protocol = PROTOCOL_GOPHER;

		// update url_ptr
		url_ptr += strlen(GOPHER_PREFIX);
	}
	else if (stringMatchPrefix(MAILTO_PREFIX, url_ptr))
	{
		if (protocol != nil) 
			*protocol = PROTOCOL_MAILTO;

		// update url_ptr
		url_ptr += strlen(MAILTO_PREFIX);
	}
	else if (stringMatchPrefix(NEWS_PREFIX, url_ptr))
	{
		if (protocol != nil) 
			*protocol = PROTOCOL_NEWS;

		// update url_ptr
		url_ptr += strlen(NEWS_PREFIX);
	}
	else if (stringMatchPrefix(NNTP_PREFIX, url_ptr))
	{
		if (protocol != nil) 
			*protocol = PROTOCOL_NNTP;

		// update url_ptr
		url_ptr += strlen(NNTP_PREFIX);
	}
	else if (stringMatchPrefix(TELNET_PREFIX, url_ptr))
	{
		if (protocol != nil) 
			*protocol = PROTOCOL_TELNET;

		// update url_ptr
		url_ptr += strlen(TELNET_PREFIX);
	}
	else if (stringMatchPrefix(WAIS_PREFIX, url_ptr))
	{
		if (protocol != nil) 
			*protocol = PROTOCOL_WAIS;

		// update url_ptr
		url_ptr += strlen(WAIS_PREFIX);
	}
	else
	{
		// TO-BE-IMPLEMENTED!! ееее
		// Need to handle cases where the protocol matches, but a username/password is
		// specified!!!
	
		// some bizarre URL we can't handle, return an err
		err = -1;
		if (protocol != nil) 
			*protocol = PROTOCOL_UNKNOWN;
		goto ERROR;
	}	
	
	//------
	// get the host name/email address/newsgroup/whatever (the second part)
	i = 0;
	do
	{
		if (host != nil)
			host[i++] = *(url_ptr++);
		if (*url_ptr == CHAR_SLASH)
			break;
		else if (*url_ptr == CHAR_COLON)
		{
			// found the port number
			found_a_colon = TRUE;
			break;
		}
		else if (*url_ptr == CHAR_NULL)
		{
			last_part = host;
			goto END_OF_STRING;
		}
	}
	while (true);
	if (host != nil)
		host[i] = CHAR_NULL;		// terminate the string
		
	if (found_a_colon)
	{
		url_ptr++;		// skip past the actual colon
		
		i = 0;
		do
		{
			if (port != nil)
				port[i++] = *(url_ptr++);
			if (*url_ptr == CHAR_SLASH)
				break;
			else if (*url_ptr == CHAR_NULL)
				break;
		}
		while (true);
	}
	
	//------
	// get the file/pathname
	// TO-BE-IMPLEMENTED!! should only do this for certain URL protocols???
	i = 0;
	do
	{
		if (file != nil)
			file[i++] = *(url_ptr++);
		if (*url_ptr == CHAR_NULL)
			break;
		else if (*url_ptr == CHAR_HASH)
		{
			found_a_hash = TRUE;
			break;
		}
	}
	while (true);
	
	//------
	// get the section of the file 
	if (found_a_hash)
	{
		url_ptr++;		// skip past the actual hash mark
		
		i = 0;
		do
		{
			if (section != nil)
				section[i++] = *(url_ptr++);
			if (*url_ptr == CHAR_NULL)
				break;
		}
		while (true);
	}
	
	last_part = file;

END_OF_STRING:
	
	// if this URL is surrounded by <...> then handle that [NONSTANDARD]
	if ((found_less_than_prefix) && (last_part != nil) && (last_part[i-1] == CHAR_GREATER_THAN))
		i--;	// back up one character to overwrite the '>'
	
	if (last_part != nil)
		last_part[i] = CHAR_NULL;		// terminate the string

ERROR:
	return err;
}



/*====================================================================================*/
/* TEST ROUTINE */
/*====================================================================================*/
#define COMPILE_TEST_ROUTINE 0
#if COMPILE_TEST_ROUTINE

#define TEST_URL1		"http://this.is.the.host/path1/path2/file.html"
#define TEST_URL2		"http://this.is.the.host:80/path1/path2/file.html"
#define TEST_URL3		"ftp://this.is.the.host:80/path1/path2/file.html"
#define TEST_URL4		"ftp://this.is.the.host:80/"
#define TEST_URL5		"http://this.is.the.host"
#define TEST_URL6		"mailto:user@hostname.org"
#define TEST_URL7		"URL:http://this.is.the.host"
#define TEST_URL8		"<http://this.is.the.host:80/path1/path2/file.html>"
#define TEST_URL9		"http://this.is.the.host:80/path1/path2/file.html#section1"
#define TEST_URL10		"news:rec.arts.moronity"
#define TEST_URL11		"   http://this.is.the.host/path1/path2/file.html   "
#define TEST_URL12		"asdf"
#define TEST_URL13		"asdf"
#define TEST_URL14		"asdf"
#define TEST_URL15		"asdf"
#define TEST_URL16		"asdf"

void main()
{
Byte protocol;
CStr255 host;
CStr255 port;
CStr255 file;
CStr255 section;
CStr255 username;
CStr255 password;
OSErr err = noErr;

	err = ParseURL(TEST_URL1, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL2, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL3, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL4, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL5, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL6, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL7, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL8, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL9, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL10, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL11, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL12, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL13, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL14, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL15, &protocol, host, port, file, section, username, password);
	err = ParseURL(TEST_URL16, &protocol, host, port, file, section, username, password);
}
#endif // COMPILE_TEST_ROUTINE
