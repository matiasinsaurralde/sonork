#if !defined(SONORK_ATOM_DB_H)
#define SONORK_ATOM_DB_H
#include "srk_codec_atom.h"

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

#define SONORK_DB_TAG_FLAGS		 		0
#define SONORK_DB_TAG_INDEX		 		1



#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,4)
#endif

#define SONORK_ATOM_DB_VERSION				1
enum SONORK_ATOM_DB_VALUE
{
	SONORK_ATOM_DB_VALUE_1
,	SONORK_ATOM_DB_VALUE_2
,	SONORK_ATOM_DB_VALUE_3
,	SONORK_ATOM_DB_VALUE_4
,	SONORK_ATOM_DB_VALUE_5
,	SONORK_ATOM_DB_VALUE_6
,	SONORK_ATOM_DB_VALUES
};

struct TSonorkAtomDbHeader
{
	DWORD 		signature;
	DWORD 		version;
	DWORD 		items;
	DWORD 		entry_size;
	DWORD 		data_size;
	TSonorkId   owner;
	DWORD		value[SONORK_ATOM_DB_VALUES];
};
struct  TSonorkAtomDbEntry
{
friend class TSonorkAtomDb;
private:
	DWORD		offset;
	DWORD		size;
public:
	TSonorkTag	tag;
};

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif

class  TSonorkAtomDb
{
	struct  _DB
	{
		TSonorkAtomDbHeader header;
		TSonorkAtomDbEntry  entry;
		bool open;
	}db;
	struct {
    	SONORK_FILE_HANDLE	index;
	    SONORK_FILE_HANDLE	data;
		int				err_code;
    }file;

    int FlushIndexHeader();
	int LoadEntry(DWORD entry_no,TSonorkAtomDbEntry *E, bool set_data_pos);
	int SetIndexHandleAtEntry(UINT entry_no);
	int SetHandleAtEof();
	int WriteEntry(TSonorkAtomDbEntry *E);
	int ReadEntry(TSonorkAtomDbEntry *E);
public:
	TSonorkAtomDb();
	~TSonorkAtomDb();
	bool            	IsOpen()	const		{ return db.open;}
	SONORK_RESULT 		Open(const TSonorkId&, const char *name, bool& reset);
	void	     		Close();
	void	     		Clear();
	DWORD           	Items()		const	{ return db.header.items;}
	DWORD	     		DataSize()	const	{ return db.header.data_size;}
	int	     		FileErrCode()	const	{ return file.err_code;}
	SONORK_RESULT		RecomputeItems();

	DWORD			GetValue(SONORK_ATOM_DB_VALUE idx) const
				{	return db.header.value[idx]; }

	SONORK_RESULT 		SetValue(SONORK_ATOM_DB_VALUE idx, DWORD value);


	SONORK_RESULT 		Add(const TSonorkCodecAtom*,DWORD*new_index=NULL,TSonorkTag*tag=NULL);
	SONORK_RESULT 		SetTag(DWORD index, TSonorkTag&tag);
	SONORK_RESULT 		Get(DWORD index, TSonorkCodecAtom*A=NULL,TSonorkTag*tag=NULL);
	SONORK_RESULT 		Del(DWORD index);
	SONORK_RESULT		GetEntryStatus(DWORD index, DWORD& size , BOOL& deleted );

	SONORK_RESULT 		AddRaw(TSonorkDynData& DD,TSonorkTag& tag,DWORD& new_index);
	SONORK_RESULT 		GetRaw(DWORD index, TSonorkDynData& DD,TSonorkTag& tag);
	SONORK_RESULT		Flush();
};
enum SONORK_ATOM_DB_FILE_EXTENSION
{
	SONORK_ATOM_DB_IDX_EXTENSION
,	SONORK_ATOM_DB_DAT_EXTENSION
,	SONORK_ATOM_DB_FILE_EXTENSIONS
};
// Extensions include de dot
extern SONORK_C_CSTR
	SonorkAtomDbFileExtensions[SONORK_ATOM_DB_FILE_EXTENSIONS];
#endif