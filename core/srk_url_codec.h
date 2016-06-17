#if !defined(SONORK_URL_CODEC_H)
#define SONORK_URL_CODEC_H
#include "srk_defs.h"
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



struct TSonorkUrlNamedValue
{
private:
	DWORD		    flags;
	TSonorkShortString  name;
	TSonorkShortString  value;
public:
	TSonorkUrlNamedValue(SONORK_C_CSTR pName,UINT pNameLen, SONORK_C_CSTR pValue, UINT pValueLen);
	TSonorkUrlNamedValue(SONORK_C_CSTR pName, SONORK_C_CSTR pValue);
	void *operator new(size_t t)	{return SONORK_MEM_ALLOC(BYTE,t);}
	void operator delete(void*ptr)	{SONORK_MEM_FREE(ptr);}
	void		SetMark()  		{	flags|=1; 	}
	void		ClearMark()		{	flags&=~1; 	}
	BOOL		GetMark()	const 	{  	return flags&1; }
	SONORK_C_CSTR	Name()		const	{	return name.CStr(); }
	SONORK_C_CSTR	Value()		const	{	return value.CStr(); }
};
DeclareSonorkQueueClass(TSonorkUrlNamedValue);

class TSonorkUrlParams
{
	TSonorkShortString		qs;
	TSonorkUrlNamedValueQueue	queue;
public:
	~TSonorkUrlParams();

	UINT
		Items()	const
		{ return queue.Items(); }
		
	TSonorkUrlNamedValueQueue&
		Queue()
		{ return queue; }

	void
		Add(SONORK_C_CSTR var_name, SONORK_C_CSTR var_value);

	void
		Set(const TSonorkUrlParams&);

	 // Transfer: Loads <O> into <this> and then clears <O>
	 // (faster and uses less memory than Set)
	void
		Transfer(TSonorkUrlParams& O);

	TSonorkUrlNamedValue *
		GetValueNV(SONORK_C_CSTR var_name) const;

	SONORK_C_CSTR
		GetValueStr(SONORK_C_CSTR var_name) const;

	int
		GetValueNum(SONORK_C_CSTR var_name, int defValue) const;

	SONORK_C_CSTR
		QueryString() const
		{ return qs.CStr(); }

	bool	
		LoadQueryString(SONORK_C_CSTR);


	void	
		Clear();

	void
		BeginEnum(TSonorkListIterator&I) const
		{ queue.BeginEnum(I); }

	TSonorkUrlNamedValue*
		EnumNext(TSonorkListIterator&I) const
		{ return queue.EnumNext(I);}

	void	EndEnum(TSonorkListIterator&I)	const
		{ queue.EndEnum(I); }

};

inline UINT SONORK_UrlMaxEncodeSize(UINT sz){ return (sz*3)+1;}
inline UINT SONORK_UrlMaxDecodeSize(UINT sz){ return sz+1;}
SONORK_C_CSTR SONORK_UrlEncode(SONORK_C_STR pTgt, UINT pTgtSize, SONORK_C_CSTR str);
SONORK_C_CSTR SONORK_UrlDecode(SONORK_C_STR pTgt, UINT pTgtSize, SONORK_C_CSTR pSrc, UINT pSrcLen=(UINT)-1);
SONORK_C_CSTR SONORK_UrlEncode(TSonorkShortString* pTgt, SONORK_C_CSTR str);
SONORK_C_CSTR SONORK_UrlDecode(TSonorkShortString* pTgt, SONORK_C_CSTR pSrc, UINT pSrcLen=(UINT)-1);
SONORK_C_CSTR SONORK_UrlEncode(TSonorkTempBuffer*pTgt, SONORK_C_CSTR str);
SONORK_C_CSTR SONORK_UrlDecode(TSonorkTempBuffer*pTgt, SONORK_C_CSTR pSrc, UINT pSrcLen=((UINT)-1));
SONORK_C_CSTR SONORK_UrlParamEncode(TSonorkTempBuffer*pTgt, SONORK_C_CSTR name,const SONORK_DWORD2&value);
SONORK_C_CSTR SONORK_UrlParamEncode(TSonorkTempBuffer*pTgt, SONORK_C_CSTR name,SONORK_C_CSTR value);
SONORK_C_CSTR SONORK_UrlEncode(SONORK_C_STR tgt, UINT tgt_size, SONORK_C_CSTR str);
BOOL	  SONORK_IsUrl(const char *str);
void	  SONORK_HtmlPuts(FILE*,SONORK_C_CSTR);
#endif