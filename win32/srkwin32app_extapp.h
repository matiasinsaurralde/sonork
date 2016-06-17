#if !defined(GUWIN32APP_EXTAPP_H)
#define GUWIN32APP_EXTAPP_H

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL).

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	SSCL for more details.

	You should have received a copy of the SSCL	along with this program;
	if not, write to sscl@sonork.com.

	You may NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL).
*/

// This include file is intended ONLY for the guwin32app_extapp.cpp module and
//  any other module that loads/configures external applications, it should NOT
//  be included otherwise.

#define LEA_SECTION_MAX_SIZE		(1024*2)
#define LEA_LINE_MAX_SIZE		380
#define LEA_MAX_APP_NAME_SIZE		32
#define LEA_MAX_APP_SECTION_NAME_SIZE	(LEA_MAX_APP_NAME_SIZE+8)
#define LEA_MAX_PARAM_NAME_SIZE		32
#define LEA_MAX_VALUE_SIZE		64
#define	LEA_MAX_VIEW_NAME_SIZE		LEA_MAX_VALUE_SIZE
#define LEA_MAX_PARAMS_SIZE		(SONORK_MAX_PATH+256)
#define LEA_MAX_PARAMS			4
#define IS_NOT_FOUND_MARK(n)		( *(DWORD*)(n) == *(DWORD*)(szNotFoundMark) )

// ----------------------------------------------------------------------------

struct TSonorkExtAppLoadSection
{
	UINT		flags;
	char		*type;
	char		*attr;
	char 		*cmd;
	char		*extra_params;	// extra_params are appended to the params contained in cmd
};

// ----------------------------------------------------------------------------

struct TSonorkExtAppLoadInfo
{
	char	*			app_name;
	char	*			ini_file;
	char				section_name[LEA_MAX_APP_SECTION_NAME_SIZE];
	TSonorkExtAppLoadSection	sec[SONORK_EXT_APP_CONTEXTS];
};

// ----------------------------------------------------------------------------

struct TSonorkExtApp
{
friend class TSonorkWin32App;
private:
	SONORK_EXT_APP_TYPE 		type;
	SKIN_ICON			icon;
	UINT				flags;
	TSonorkShortString		name;
	TSonorkShortString		cmd;
	UINT				cmd_len;	// Excluding parameters
	char			*	view_name;
	TSonorkExtApp(SONORK_EXT_APP_TYPE t);
	~TSonorkExtApp();
	// The <p_cmd> may already have parameters (after the first space)
	// <p_extra_params> are appended after those
	void	SetCmd(const char *p_cmd, const char* p_extra_params, bool no_quote_parsing);
public:
	SONORK_EXT_APP_CONTEXT
		Context() const
		{ return (SONORK_EXT_APP_CONTEXT)(flags&SONORK_APP_EA_CONTEXT_MASK);}

	SONORK_EXT_APP_TYPE
		Type() 	const
		{ return type; }

	UINT	Flags()	const
		{ return flags; }

	SONORK_C_CSTR
		Name()	const
		{ return name.CStr();}

	SONORK_C_CSTR
		Cmd() const
		{ return cmd.CStr();}

	UINT
		CmdLen() const
		{ return cmd_len;}

	SONORK_C_CSTR
		ViewName() const
		{ return view_name;}

	SKIN_ICON
		AppIcon() const
		{ return icon;}

};

// ----------------------------------------------------------------------------

struct TSonorkExtAppReqAtom
:public TSonorkCodecAtom
{
	TSonorkShortString	app_name;
	TSonorkShortString	alias;
	TSonorkPhysAddr		phys_addr;
	TSonorkDynData		data;
	DWORD			params[15]  __SONORK_PACKED;
	TSonorkExtAppReqAtom();

// ------------
// CODEC

public:
	void	CODEC_Clear();

	SONORK_ATOM_TYPE
			CODEC_DataType() const
			{ return SONORK_ATOM_EXTERNAL_APP_REQ_1; }

private:

	void	CODEC_WriteDataMem	(TSonorkCodecWriter&) const;
	void	CODEC_ReadDataMem	(TSonorkCodecReader&);
	DWORD	CODEC_DataSize() const;
};

// ----------------------------------------------------------------------------

// Structure where we store information
// about about a request we've sent or received
// from/to the other side.

struct TSonorkExtAppPendingReq
{
	class TSonorkWaitWin*		owner;
	union {
		TSonorkUserHandle	user;
		TSonorkServiceHandle	service;
		TSonorkServiceCircuit	circuit;
		TSonorkServiceQuery	query;
	}remote;
	DWORD			local_instance;
	TSonorkExtAppReqAtom 	request;
	const TSonorkExtApp*	EA;
	char			view_name[LEA_MAX_VIEW_NAME_SIZE];
	bool			processed;
	bool			using_circuit;
	TSonorkExtAppPendingReq()
	{EA=NULL;owner=NULL;local_instance=0;using_circuit=processed=false;}
};

// ----------------------------------------------------------------------------

extern SONORK_C_CSTR
 sonork_ext_app_context_name[SONORK_EXT_APP_CONTEXTS];



#endif
