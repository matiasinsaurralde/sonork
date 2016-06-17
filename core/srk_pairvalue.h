#if !defined(SRK_PAIRVALUE_H)
#define SRK_PAIRVALUE_H
#include "srk_codec_atom.h"
#include "srk_data_lists.h"

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

enum SONORK_PAIR_VALUE_TYPE
{
	SONORK_PAIR_VALUE_ANY 	= -1
,	SONORK_PAIR_VALUE_NONE  = 0
,	SONORK_PAIR_VALUE_DWORD
,	SONORK_PAIR_VALUE_DWORD2
,	SONORK_PAIR_VALUE_SHORT_STRING
,	SONORK_PAIR_VALUE_DYN_DATA
};

struct TSonorkPairValue
{
private:
	SONORK_PAIR_VALUE_TYPE	type;
	TSonorkShortString		name;
	union
	{
		DWORD			dw;
		SONORK_DWORD2   	dw2;
		TSonorkShortString*	s_str;
		TSonorkDynData*     d_data;
	}value;
public:
	TSonorkPairValue();
	TSonorkPairValue(SONORK_C_CSTR name,SONORK_PAIR_VALUE_TYPE);
	~TSonorkPairValue();

	SONORK_PAIR_VALUE_TYPE	Type() const
		{ return type;}

	void SetType(SONORK_PAIR_VALUE_TYPE);
	void Clear();

		TSonorkShortString&	NameStr()		{ return name;}
		SONORK_C_CSTR		Name() const    { return name.CStr();}

		DWORD			DW() const;
		DWORD			SetDW(DWORD v);

		bool			GetDW2(SONORK_DWORD2&) const;
		SONORK_DWORD2&		SetDW2(const SONORK_DWORD2&);

		TSonorkShortString* SetStr(SONORK_C_CSTR str);
const	TSonorkShortString* GetStr() const;
		TSonorkShortString* wGetStr();
		SONORK_C_CSTR		CStr()	const;

		TSonorkDynData*		SetDynData(const void*ptr=NULL,UINT sz=0,SONORK_ATOM_TYPE tp=SONORK_ATOM_NONE);
		TSonorkDynData*		SetDynDataAtom(TSonorkCodecAtom*A);

// --------------
// PSEUDO CODEC (TSonorkShortString is not a TSonorkCodecAtom)

		UINT	CODEC_Size() const;

		void	CODEC_WriteDataMem	(TSonorkCodecWriter& CODEC) const;
		void	CODEC_ReadDataMem	(TSonorkCodecReader& CODEC);

};


class TSonorkPairValueQueue
:public TSonorkQueue, public TSonorkCodecAtom
{

public:
	TSonorkPairValueQueue();
	~TSonorkPairValueQueue();

	TSonorkPairValue *Add(TSonorkPairValue *PV)
					{ return (TSonorkPairValue*)w_Add(PV); }

	bool		  Remove(SONORK_C_CSTR name);
	bool		  Remove(TSonorkPairValue *PV)
					{ return w_Remove(PV); }


	TSonorkPairValue *Get(SONORK_C_CSTR name);
	TSonorkPairValue *GetIndexed(SONORK_C_CSTR name,UINT idx,SONORK_PAIR_VALUE_TYPE = SONORK_PAIR_VALUE_ANY);

	TSonorkPairValue *GetType(SONORK_C_CSTR name, SONORK_PAIR_VALUE_TYPE);

	TSonorkPairValue *EnumNext(TSonorkListIterator& I) const
					{ return (TSonorkPairValue *)w_EnumNext(I); }
	void Clear();
	
// ------------
// CODEC
public:

	void 	CODEC_Clear()
			{	Clear();	}

	SONORK_ATOM_TYPE
			CODEC_DataType()	const
			{	return SONORK_ATOM_PAIR_VALUE;}



private:

	void	CODEC_WriteDataMem	(TSonorkCodecWriter& CODEC) const;

	void	CODEC_ReadDataMem	(TSonorkCodecReader& CODEC);

	DWORD	CODEC_DataSize()	const;

};



#endif
