#if !defined(SRK_WINREGKEY_H)
#define SRK_WINREGKEY_H

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

class TSonorkRegKey
{
	HKEY	hkey;
	bool	hkey_is_mine;
public:
	TSonorkRegKey(){	hkey=NULL;}
	TSonorkRegKey(HKEY pk){ hkey=pk;hkey_is_mine=false;}
	~TSonorkRegKey();

	HKEY
		Handle() const
		{ return hkey; }

	operator
		HKEY()	const
		{ return hkey; }

	bool
		IsOpen() const
		{ return hkey!=NULL;}

	LONG	Open(HKEY, const char *name,   bool create=false, REGSAM sam=KEY_ALL_ACCESS);
	LONG	QueryValue(const char*name,char *value, DWORD value_size, DWORD*ptype=NULL);
	LONG	QueryValue(const char*name,DWORD* value);
	LONG	SetValue(const char*name,const char *value);
	LONG	SetValue(const char*name,DWORD value);
	LONG	SetValue(const char*name,const void*value,DWORD value_size,DWORD value_type);
	LONG	DelValue(const char*name);
	LONG	DelKey(const char*name);

	LONG	EnumKey(DWORD index,char *name, DWORD name_size);
	LONG	EnumValue(DWORD index
				, char *name
				, DWORD name_size
				, char *value
				, DWORD value_size
				, DWORD*type=NULL);
	void 	Close();
};
#endif
