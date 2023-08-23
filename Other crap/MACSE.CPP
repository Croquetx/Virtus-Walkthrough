// NOTE:  THE RESOURCES ARE NOT MERGED INTO SOURCESAFE!!!!!


// BLURB CODE ON THE MAC:  (MERGED INTO SOURCESAFE)

Blurb(Handle parent)
{
	short rnd = Random();
	rnd = rnd > 0 ? rnd : -rnd;
	rnd = 5011 + (rnd % MAX_NUM_STRINGS);
	Handle strhandle = Get1Resource('STR ', rnd);
	if (strhandle == NULL)
		goto end;
	// Show a dialog with the string
	HLock(strhandle);
	ParamText((StringPtr)*strhandle, "\p", "\p", "\p");
	Alert(5000, nil);
	HUnlock(strhandle);
	ReleaseResource(strhandle);
end:
	return 0;
}


// GETSERIAL CODE ON THE MAC:  (MERGED INTO SOURCESAFE)

int
GetSerial(strPtr str,int num)
	{
		return 1;		// ABD-BLURB BOGUS THING FOR SPECIAL EDITION
	}


// ... various other things will need to be done to make SE compile on the Mac

// also, the blurb code needs the resources included in the file mac-se.rsrc.  Paste these
// into the resource file (THEY ARE NOT IN SOURCESAFE)