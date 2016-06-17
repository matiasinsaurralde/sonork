#include <windows.h>
#pragma hdrstop
#include "srk_winregkey.h"

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


LONG TSonorkRegKey::Open(HKEY pKey, const char*name, bool create, REGSAM sam)
{
	LONG r;

	Close();

	if( create )
	{
		DWORD disp;
		r=RegCreateKeyEx(
			pKey
			,name
			,0
			,NULL
			,REG_OPTION_NON_VOLATILE
			,sam
			,NULL
			,&hkey
			,&disp 	// address of disposition value buffer
		   );
	}
	else
		r = RegOpenKeyEx(
			pKey
			,name
			,0
			,sam
			,&hkey);
	if(r!=ERROR_SUCCESS)
		hkey=NULL;
	return r;
}
void 	TSonorkRegKey::Close()
{
	if( hkey != NULL && hkey_is_mine)
		RegCloseKey(hkey);
	hkey=NULL;
}
LONG	TSonorkRegKey::EnumValue(DWORD index
				, char *name
				, DWORD name_size
				, char *value
				, DWORD value_size
				, DWORD*type)
{
	 return RegEnumValue(
	  hkey
	 ,index
	 ,name
	 ,&name_size
	 ,0
	 ,type
	 ,(BYTE*)value
	 ,&value_size);
}
LONG	TSonorkRegKey::EnumKey(DWORD index,char *name, DWORD name_size)
{
	return RegEnumKeyEx(hkey
				,index
				,name
				,&name_size
				,NULL
				,0
				,NULL
				,NULL);

}

LONG
 TSonorkRegKey::DelValue(const char*name)
{
	return RegDeleteValue(hkey,name);
}

LONG
 TSonorkRegKey::DelKey(const char*name)
{
	return RegDeleteKey(hkey,name);
}


LONG	TSonorkRegKey::SetValue(const char*name,DWORD value)
{
	return RegSetValueEx(
	 hkey
	 ,name
	,0
	,REG_DWORD
	,(const BYTE*)&value
	,sizeof(DWORD));
}
LONG	TSonorkRegKey::SetValue(const char*name,const void*value,DWORD value_size,DWORD value_type)
{
	return RegSetValueEx(
	 hkey
	 ,name
	,0
	,value_type
	,(const BYTE*)value
	,value_size);
}
LONG	TSonorkRegKey::SetValue(const char*name,const char *value)
{
	return RegSetValueEx(
	 hkey
	 ,name
	,0
	,REG_SZ
	,(const BYTE*)value
	,strlen(value)+1);
}

LONG	TSonorkRegKey::QueryValue(const char*name,DWORD* value)
{
	DWORD value_size;
	DWORD type;
	LONG r;
	value_size=sizeof(DWORD);
	r=RegQueryValueEx(
		hkey
		,name
		,NULL
		,&type
		,(BYTE*)value
		,&value_size) ;
	if( r== ERROR_SUCCESS && type!=REG_DWORD)
		return ERROR_BAD_FORMAT;
	return r;
		
}

LONG TSonorkRegKey::QueryValue(const char*name,char *value, DWORD value_size, DWORD *type)
{
	LONG r=RegQueryValueEx(
		hkey
		,name
		,NULL
		,type
		,(BYTE*)value
		,&value_size) ;
	if( r != ERROR_SUCCESS && value!=NULL)
	{
		*value=0;
	}
	return r;
}
TSonorkRegKey::~TSonorkRegKey()
{
	Close();
}

