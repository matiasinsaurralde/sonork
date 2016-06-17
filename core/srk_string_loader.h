#if !defined(SONORK_STRING_LOADER_H)
#define SONORK_STRING_LOADER_H
#include "srk_defs.h"

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




// TSonorkStringLoader
//  A Helper for loading strings in a controlled manner:
//  Add() ignores characters when the maximum length is reached
//  so there is not needed to check for overflow each and every time
//  you add characters, only once per loop by checking Full()
struct TSonorkStringLoaderMark
{
friend class TSonorkStringLoader;
private:
	UINT 	len;
	UINT	flags;
	char 	*ptr;
};

class TSonorkStringLoader
{
	char *	cur_ptr;
	UINT    max_len;
	UINT	cur_len;
	DWORD	flags;
public:
	enum TGSL_FLAGS
	{
		TGSL_IGNORE_CR			= 0x00000001
	};
	TSonorkStringLoader(){ cur_ptr=NULL;flags=0;}


	BOOL	IsOpen()		const { return cur_ptr!=NULL; }

	// Translation flags
	void	SetTranslationFlags(DWORD f){ flags=f;}
	DWORD	GetTranslationFlags() const { return flags; }

	void	SetIgnoreCR(){ SetTranslationFlags(flags|TGSL_IGNORE_CR); }
	// Ignores '\r' characters on AddEncode/AddDecode, not used by Add()

	// buffer_size should be REAL size of the buffer
	// max_len is calculated as buffer_size - 1 (to make space for ending NULL)
	void	Open(char *buffer,UINT buffer_size);

	// Don't call the Add() functions before Open()!
	void 	Add(char c);

	void 	AddEncode(char c);	// Prefixes '\\' for special chars
	char	AddDecode(char c);	// Adds char after a prefixed '\\', returns decoded char
								// returns 0 if character is ignored 
	// Use AddDecode on next character in input stream, when '\' is found
	// i.e. if input stream is "Hi\n", Use Add() for 'H' and 'i' but not for '\'
	//      when '\' is found, skip it and instead call AddDecode('n')
	void 	AddStr(const char* str);
	void	AddStrEncode(const char* str);
	void	AddStrDecode(const char* str);

	// Used by AddDecode(), affected by translation flags
	char GetDecodeChar(char c);

	// Call Close() to append NULL character,
	// Call may be ommited it NULL char is not needed.
	UINT	Close();	// Returns CurLen()

	// Note: Full(),MaxLen(),CurLen() remain after Close()
	//  		until Open() is invoked again.
	bool	Full()	const 	{ return cur_len>=max_len;}
	UINT	MaxLen() const	{ return max_len; }
	UINT	CurLen() const	{ return cur_len; }
	char *	CurPtr() 		{ return cur_ptr;}

	void	Push(TSonorkStringLoaderMark&);	// Saves current position & flags
	// Restores flags and if <position> is true, also restores position
	void	Pop(TSonorkStringLoaderMark&, BOOL position);
};

#endif