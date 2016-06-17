#if !defined(SONORK_LANGUAGE_H)
#define SONORK_LANGUAGE_H

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

#include "srk_data_types.h"
#include "srk_sys_string.h"

#if defined(SONORK_CLIENT_BUILD)
struct TSonorkLangCodeRecord
{
	DWORD		code;
	SONORK_C_STR	name;
};

class TSonorkLangCodeTable
{
	TSonorkLangCodeRecord	*	table;
	int				items;
	void	Clear();
public:
	TSonorkLangCodeTable();
	~TSonorkLangCodeTable();

	SONORK_RESULT
		Load(TSonorkError& ERR, SONORK_C_CSTR 	file_name);

	bool
		IsLoaded() const
		{ return table!=NULL; }

	int
		Items()	const
		{ return items;}

const	TSonorkLangCodeRecord*
		Table() const
		{ return table;}

const	TSonorkLangCodeRecord*
		GetRecordByIndex(int index);

const	TSonorkLangCodeRecord*
		GetRecordByCode(DWORD code, int*p_index = NULL);

};

class TSonorkLanguageTable
{
private:

	struct {
		struct
		{
			DWORD	code;
			char	lexeme[4];
		}lang;
		UINT*		str_index;
	}sys;
	struct {
		UINT		entries;
		UINT*		str_index;
		SONORK_C_CSTR* 	xlat_table;
	}app;
	SONORK_C_STR		str_table;
	void		   	Clear();

	UINT
		GetAppEntryIndex( SONORK_C_CSTR lexeme );

	SONORK_RESULT
		_Load(void*,TSonorkError&);

	char
		_ParseLine(SONORK_C_STR,SONORK_C_STR*pValue,UINT*pValueLen);

	UINT
		_ParseCommand(UINT,SONORK_C_CSTR,SONORK_C_CSTR);

public:
	TSonorkLanguageTable();
	~TSonorkLanguageTable();
	
	SONORK_RESULT
		Load(	TSonorkError& 		ERR
			,SONORK_C_CSTR 		file_name
			,UINT			xlat_entries
			,SONORK_C_CSTR*		xlat_table);

	DWORD
		LangCode() const
		{ return sys.lang.code;}

const char*
		LangLexeme() const
		{ return sys.lang.lexeme;  }

	bool
		IsLoaded() const
		{ return sys.lang.code!=0; }


	// Transfer() loads the data of 'T' into 'this' and clears 'T'
	void
		Transfer(TSonorkLanguageTable&T);

	SONORK_C_CSTR
		AppStr(UINT index) const;

	SONORK_C_CSTR
		SysStr(SONORK_SYS_STRING index)	const;

	void	Translate(TSonorkError& ERR);
};
// Copies string replacing '\n','\r' with equivalent ascii values
int 		SONORK_ParseCopyString(SONORK_C_STR target, SONORK_C_CSTR source);
#endif

#endif

