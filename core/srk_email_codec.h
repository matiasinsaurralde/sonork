#if !defined(SRK_EMAIL_CODEC_H)
#define SRK_EMAIL_CODEC_H

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

// ----------------------------------------------------------------------------

enum SONORK_EMAIL_ACCOUNT_STRING
{
  SONORK_EMAIL_ACCOUNT_STR_NAME
, SONORK_EMAIL_ACCOUNT_STR_REAL_NAME
, SONORK_EMAIL_ACCOUNT_STR_RETURN_ADDRESS
, SONORK_EMAIL_ACCOUNT_STR_LOGIN_NAME
, SONORK_EMAIL_ACCOUNT_STR_LOGIN_PASS
, SONORK_EMAIL_ACCOUNT_STR_INCOMMING_SERVER
, SONORK_EMAIL_ACCOUNT_STR_OUTGOING_SERVER
, SONORK_EMAIL_ACCOUNT_STR_FIRST_UIDL
, SONORK_EMAIL_ACCOUNT_STR_LAST_UIDL
, SONORK_EMAIL_ACCOUNT_STRINGS
};
enum SONORK_EMAIL_ACCOUNT_FLAG
{
  SONORK_EMAIL_ACCOUNT_F_CHECK_MAIL	= 0x00010000
, SONORK_EMAIL_ACCOUNT_F_PENDING		= 0x10000000
};


enum SONORK_EMAIL_EXCEPT_FIELD
{
  SONORK_EMAIL_EXCEPT_FIELD_NONE
, SONORK_EMAIL_EXCEPT_FIELD_TO
, SONORK_EMAIL_EXCEPT_FIELD_FROM
, SONORK_EMAIL_EXCEPT_FIELD_SUBJECT
, SONORK_EMAIL_EXCEPT_FIELDS
};

#define SONORK_EMAIL_EXCEPT_FIELD_MASK		0x000000ff
#define SONORK_EMAIL_EXCEPT_OP_NEGATIVE		0x10000000
#define SONORK_EMAIL_EXCEPT_OP_MASK		0x100000ff
enum SONORK_EMAIL_EXCEPT_OP
{
  SONORK_EMAIL_EXCEPT_OP_NONE		= 0
, SONORK_EMAIL_EXCEPT_OP_CONTAINS       = 1
, SONORK_EMAIL_EXCEPT_OP_NOT_CONTAINS	= (SONORK_EMAIL_EXCEPT_OP_CONTAINS|SONORK_EMAIL_EXCEPT_OP_NEGATIVE)
};

#if SONORK_CODEC_LEVEL>5

struct TSonorkEmailAccount
:public TSonorkCodecAtom
{
	struct HEADER
	{
		DWORD				type;
		DWORD				flags;
		DWORD				uid;
		DWORD				last_msg_no;
		DWORD				reserved[4];
	}__SONORK_PACKED;
	HEADER					header;
	TSonorkShortString			str[SONORK_EMAIL_ACCOUNT_STRINGS];

	void	Clear();

	DWORD
		UID() const
		{	return header.uid; }

	void
		SetUID(DWORD uid)
		{	header.uid=uid;}

	DWORD
		LastMsgNo() const
		{	return header.last_msg_no; }

	void
		SetLastMsgNo( DWORD no )
		{	header.last_msg_no = no ; }

	SONORK_EMAIL_ACCOUNT_TYPE
		AccountType() const
		{	return (SONORK_EMAIL_ACCOUNT_TYPE)header.type; }

	SONORK_C_CSTR const
		Name()
		{	return str[SONORK_EMAIL_ACCOUNT_STR_NAME].CStr(); }

	SONORK_C_CSTR	const
		ReturnAddress()
		{  	return str[SONORK_EMAIL_ACCOUNT_STR_RETURN_ADDRESS].CStr();}

	SONORK_C_CSTR
		FirstUIDL() const
		{ 	return str[SONORK_EMAIL_ACCOUNT_STR_FIRST_UIDL].CStr();}

	SONORK_C_CSTR
		LastUIDL() const
		{ 	return str[SONORK_EMAIL_ACCOUNT_STR_LAST_UIDL].CStr();}

	SONORK_C_CSTR
		OutgoingServer() const
		{	return str[SONORK_EMAIL_ACCOUNT_STR_OUTGOING_SERVER].CStr(); }

	void
		SetFirstUIDL( SONORK_C_CSTR pstr )
		{	str[SONORK_EMAIL_ACCOUNT_STR_FIRST_UIDL].Set( pstr ); }

	void
		SetLastUIDL( SONORK_C_CSTR pstr )
		{	str[SONORK_EMAIL_ACCOUNT_STR_LAST_UIDL].Set( pstr ); }

// -----------------------
// CODEC

public:

	void	CODEC_Clear()
			{ Clear(); }

	SONORK_ATOM_TYPE
			CODEC_DataType()	const
			{ 	return SONORK_ATOM_EMAIL_ACCOUNT_1; 	}

private:

	void
			CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
			CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
			CODEC_DataSize()	const 	;
};

// ----------------------------------------------------------------------------

struct TSonorkEmailExceptEntry
:public TSonorkCodecAtom
{
	struct HEADER
	{
		DWORD				field;
		DWORD				op;
		DWORD				flags;
		DWORD				reserved[3];
	}__SONORK_PACKED;
	HEADER					header;
	TSonorkShortString			string;

	void	Clear();


	SONORK_EMAIL_EXCEPT_FIELD
		ExceptField() const
		{
			return (SONORK_EMAIL_EXCEPT_FIELD)
					(header.field&SONORK_EMAIL_EXCEPT_FIELD_MASK);
		}

	SONORK_EMAIL_EXCEPT_OP
		ExceptOp() const
		{
			return (SONORK_EMAIL_EXCEPT_OP)
					(header.op&SONORK_EMAIL_EXCEPT_OP_MASK);
		}

	SONORK_C_CSTR
		ExceptStr() const
		{
			return string.CStr();
		}

	void
		SetExceptField( SONORK_EMAIL_EXCEPT_FIELD f)
		{
			header.field=(header.field&~SONORK_EMAIL_EXCEPT_FIELD_MASK)
							|   (f&SONORK_EMAIL_EXCEPT_FIELD_MASK);
		}

	void
		SetExceptOp( SONORK_EMAIL_EXCEPT_OP op)
		{
			header.op=(header.op&~SONORK_EMAIL_EXCEPT_OP_MASK)
							|   (op&SONORK_EMAIL_EXCEPT_OP_MASK);
		}
// -----------------------
// CODEC

public:

	void	CODEC_Clear()
			{ Clear(); }

	SONORK_ATOM_TYPE
			CODEC_DataType()	const
			{ 	return SONORK_ATOM_EMAIL_EXCEPT_ENTRY; 	}

private:

	void
			CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
			CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
			CODEC_DataSize()	const 	;
};

StartSonorkAtomQueueClass(TSonorkEmailAccountQueue,TSonorkEmailAccount)
	NoExtensions
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkEmailExceptEntryQueue,TSonorkEmailExceptEntry)
	NoExtensions
EndSonorkAtomQueueClass;

struct TSonorkEmailExcept
:public TSonorkCodecAtom
{
	struct HEADER
	{
		DWORD				flags;
		DWORD				reserved[3];
	}__SONORK_PACKED;
	HEADER						header;
	TSonorkShortString				name;
	TSonorkEmailExceptEntryQueue    queue;

	void	Clear();


	SONORK_C_CSTR
		Name() const
		{
			return name.CStr();
		}
		
	BOOL
		Contains( SONORK_C_CSTR from, SONORK_C_CSTR to , SONORK_C_CSTR subject);

// -----------------------
// CODEC

public:

	void	CODEC_Clear()
			{ Clear(); }

	SONORK_ATOM_TYPE
			CODEC_DataType()	const
			{ 	return SONORK_ATOM_EMAIL_EXCEPT; 	}

private:

	void
			CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
			CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
			CODEC_DataSize()	const 	;
};

StartSonorkAtomQueueClass(TSonorkEmailExceptQueue,TSonorkEmailExcept)
	BOOL
		Contains( SONORK_C_CSTR from, SONORK_C_CSTR to , SONORK_C_CSTR subject);
EndSonorkAtomQueueClass;

#endif
#endif
