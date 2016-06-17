#include "srk_defs.h"
#pragma hdrstop
#include "srk_sys_string.h"

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL) Version 1.

	You should have received a copy of the SSCL	along with this program;
	if not, write to sscl@sonork.com.

	You should NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL), doing so will indicate your agreement
	to the the terms which may be differ for each version of the software.

	This comment section, indicating the existence and requirement of
	acceptance of the SSCL may not be removed from the source code.
*/

#undef	_GSS
#define	_GSS(str)		{GSS_##str,#str}
static struct
{
	SONORK_SYS_STRING	index;
	SONORK_C_CSTR		lexeme;
}gu_sys_xlat_table[SONORK_SYS_STRINGS]=
{
	{GSS_NULL,""}
,	_GSS(NOTINIT)
,	_GSS(NOTCXTED)
,	_GSS(NOTAUTHED)
,	_GSS(NOPERMIT)
,	_GSS(NODATA)
,	_GSS(NOUTSLNK)
,	_GSS(QUOTAEXCEEDED)
,	_GSS(OUTOFRESOURCES)
,	_GSS(DUPDATA)
,	_GSS(ENGBUSY)
,	_GSS(SVRBUSY)
,	_GSS(APPBUSY)
,	_GSS(USRCANCEL)
,	_GSS(REQDENIED)
,	_GSS(REQTIMEOUT)
,	_GSS(NETERR)
,	_GSS(NETCXERR)
,	_GSS(NETWRERR)
,	_GSS(NETRDERR)
,	_GSS(NETCXLOST)
,	_GSS(NETCXSVRKILL)
,	_GSS(BADID)
,	_GSS(BADGUID)
,	_GSS(BADSID)
,	_GSS(BADALIAS)
,	_GSS(BADNAME)
,	_GSS(BADNOTES)
,	_GSS(BADPWD)
,	_GSS(BADUINFOLVL)
,	_GSS(BADCODEC)
,	_GSS(BADCRYPTTYPE)
,	_GSS(BADCRYPTSIGNATURE)
,	_GSS(BADNETPROTOCOL)
,	_GSS(BADSVRCFG)
,	_GSS(BADHANDLE)
,	_GSS(BADADDR)
,	_GSS(BADTYPE)
,	_GSS(DATATOOSMALL)
,	_GSS(SYNCHFAIL)
,	_GSS(CRYPTFAIL)
,	_GSS(PCKTOOSMALL)
,	_GSS(TASKNOTALLOC)
,	_GSS(FILEOPENERR)
,	_GSS(FILECREATEERR)
,	_GSS(FILERDERR)
,	_GSS(FILEWRERR)
,	_GSS(FILENEXISTS)
,	_GSS(DBWRERR)
,	_GSS(DBRDERR)
,	_GSS(OPNOTSUPPORT)
,	_GSS(OPNOTALLOWED)
,	_GSS(INTERNALERROR)
};

SONORK_C_CSTR	SONORK_SysIndexToLexeme(SONORK_SYS_STRING index)
{
	if(index>GSS_NULL && index<SONORK_SYS_STRINGS)
		return gu_sys_xlat_table[index].lexeme;
	return "";
}
SONORK_SYS_STRING SONORK_SysLexemeToIndex(SONORK_C_CSTR lexeme)
{
	UINT i;
	if( lexeme != NULL)
	for(i=1 ; i < SONORK_SYS_STRINGS; i++ )
	{
		if( *gu_sys_xlat_table[i].lexeme==*lexeme )
			if( !SONORK_StrNoCaseCmp(gu_sys_xlat_table[i].lexeme+1, lexeme+1) )
				return (SONORK_SYS_STRING)i;
	}
	return GSS_NULL;
}
