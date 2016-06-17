#if !defined(SRKBROWSERDEFS_H)
#define SRKBROWSERDEFS_H

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

#if !defined(SRKMFCAPI_H)
#include "SrkMfcApi.h"
#endif

#define SRK_MFC_BROWSER_EV_BEFORE_NAVIGATE		SRK_MFC_EVENT_01
#define SRK_MFC_BROWSER_EV_NAVIGATE_COMPLETE	SRK_MFC_EVENT_02
#define SRK_MFC_BROWSER_EV_DOWNLOAD_BEGIN		SRK_MFC_EVENT_03
#define SRK_MFC_BROWSER_EV_DOWNLOAD_COMPLETE	SRK_MFC_EVENT_04
#define SRK_MFC_BROWSER_EV_DOCUMENT_COMPLETE	SRK_MFC_EVENT_05
#define SRK_MFC_BROWSER_EV_PROGRESS_CHANGE		SRK_MFC_EVENT_06
#define SRK_MFC_BROWSER_EV_TITLE_CHANGE			SRK_MFC_EVENT_07

#define SRK_MFC_BROWSER_CM_NAVIGATE				SRK_MFC_CMD_01
#define SRK_MFC_BROWSER_CM_GET_FORM_INFO		SRK_MFC_CMD_10
#define SRK_MFC_BROWSER_CM_GET_FORM_ELEMENT		SRK_MFC_CMD_11
#define SRK_MFC_BROWSER_CM_GET_BROWSER_CTRL		SRK_MFC_CMD_12
#define SRK_MFC_BROWSER_CM_COMBINE_URL			SRK_MFC_CMD_13

struct TSrkMfcBrowserEvBeforeNavigate
{
	const char	*url;
	const char	*post_data;
	UINT		post_data_size;
	DWORD		flags;
	BOOL		*cancel;
};
struct TSrkMfcBrowserEvProgressChange
{
	long		cur_v,max_v;
};

struct TSrkMfcBrowserCmNavigate
{
	enum _CMD
	{
		CMD_NONE
	,	CMD_STOP
	,	CMD_URL
	,	CMD_HISTORY
	,	CMD_SUBMIT
	};
	DWORD		cmd;
	SONORK_C_CSTR   str;
	int			i_param;
	DWORD		reserved[8];
};
struct TSrkMfcBrowserCmGetFormInfo
{
	enum STR_INDEX
	{
	 	STR_NAME
	,	STR_VALUE
	};
	DWORD				reserved;
	IUnknown*			form;
	SONORK_C_CSTR			s_param;
	DWORD				d_param;
	TSonorkShortString*		str[4];
	DWORD				num[4];
};
struct TSrkMfcBrowserCmCombineUrl
{
	TSonorkShortString*		oURL;
	SONORK_C_CSTR			iURL[2];	// Set iURL[0] to NULL to combine with current
};
#endif