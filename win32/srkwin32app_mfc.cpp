#include "srkwin32app.h"
#pragma hdrstop
#include "srkwin32app_mfc.h"

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

static HINSTANCE			 		srk_mfc_hinstance=NULL;
static UINT 				 		srk_mfc_ref_count;
static TSrkMfcContext				 srk_mfc_context;
static fnSrkMfcCreateWindow			*pSrkMfcCreateWindow;
static fnSrkMfcDestroyWindow		*pSrkMfcDestroyWindow;
static fnSrkMfcPreTranslateMessage	*pSrkMfcPreTranslateMessage;

static UINT	SrkMfc_IncRef();
static UINT	SrkMfc_DecRef();

TSrkMfcWindowHandle*  	SrkMfc_CreateWindow(TSonorkWin*OW,SRK_MFC_WIN_TYPE type)
{
	TSrkMfcWindowCreateInfo CI;
	TSrkMfcWindowHandle*   	WH;
	if(!SrkMfc_IncRef())
		return NULL;
	CI.owner.hwnd 		= OW->Handle();
	WH = pSrkMfcCreateWindow(type,&CI);
	if(WH==NULL)
		SrkMfc_DecRef();
	return WH;
}

void	SrkMfc_DestroyWindow( TSrkMfcWindowHandle* WH )
{
	if( srk_mfc_hinstance!=NULL && WH!=NULL )
	{
		pSrkMfcDestroyWindow(WH);
		SrkMfc_DecRef();
	}
}

static
	bool
		SrkMfc_LoadLibrary();
UINT	SrkMfc_IncRef()
{
	if( srk_mfc_hinstance==NULL )
	{
		if( !SrkMfc_LoadLibrary() )
			return 0;
	}
	srk_mfc_ref_count++;
	return srk_mfc_ref_count;
}
UINT	SrkMfc_DecRef()
{
	if( srk_mfc_hinstance!=NULL )
	{
		assert(srk_mfc_ref_count>0);
		if(--srk_mfc_ref_count == 0)
		{
			FreeLibrary(srk_mfc_hinstance);
//			TRACE_DEBUG("SONORK_MFC Library Unloaded (%x)",srk_mfc_hinstance);
			srk_mfc_hinstance=NULL;
		}
		return srk_mfc_ref_count;
	}
	return 0;
}
void WINAPI	SrkMfc_Trace(const char*msg)
{
	sonork_puts(msg);
}
BYTE* PASCAL WINAPI	SrkMfc_MemAlloc(UINT sz)
{
	BYTE* rv;
	rv = SONORK_MEM_ALLOC(BYTE,sz);
//	TRACE_DEBUG("SrkMfc_MemAlloc(%u) >> %x",sz,rv);
	return rv;
}
void PASCAL WINAPI	SrkMfc_MemFree(void*ptr)
{
//	TRACE_DEBUG("SrkMfc_MemFree(%x)",ptr);
	SONORK_MEM_FREE(ptr);
}


bool 	SrkMfc_LoadLibrary()
{
	char	path[SONORK_MAX_PATH];
	assert( srk_mfc_hinstance==NULL );
	SonorkApp.GetDirPath(path,SONORK_APP_DIR_BIN,"SrkTpl.dll");
	srk_mfc_hinstance = LoadLibrary(path);
	if(srk_mfc_hinstance != NULL )
	{
		fnSrkMfcInitialize *init;
		*(void**)&init = GetProcAddress(srk_mfc_hinstance,"SrkInit");
		if(init != NULL )
		{
			srk_mfc_context.cb_alloc	= SrkMfc_MemAlloc;
			srk_mfc_context.cb_free 	= SrkMfc_MemFree;
			srk_mfc_context.cb_trace	= SrkMfc_Trace;
			srk_mfc_context.ev_msg		= WM_SONORK_MFC;
			if(init(0,&srk_mfc_context))
			{
				*(void**)&pSrkMfcCreateWindow = GetProcAddress(srk_mfc_hinstance
					,"SrkCreateWin");
				if(pSrkMfcCreateWindow != NULL )
				{
					*(void**)&pSrkMfcDestroyWindow = GetProcAddress(srk_mfc_hinstance
						,"SrkDestroyWin");
					if(pSrkMfcDestroyWindow != NULL )
					{
						*(void**)&pSrkMfcPreTranslateMessage = GetProcAddress(srk_mfc_hinstance
							,"SrkPreTranslateMsg");
						if(pSrkMfcPreTranslateMessage!=NULL)
						{
//							TRACE_DEBUG("SRK_MFC Library Loaded (%x)",srk_mfc_hinstance);
							srk_mfc_ref_count=0;
							return true;
						}
					}
				}
			}
		}
		FreeLibrary(srk_mfc_hinstance);
		srk_mfc_hinstance=NULL;
	}
	return false;
}
LRESULT TSrkMfcWindowHandle::Exec(SRK_MFC_CMD cmd,LPARAM lParam)
{
	return ::SendMessage(hwnd,srk_mfc_context.cm_msg,cmd,lParam);
}
BOOL TSrkMfcWindowHandle::PreTranslateMessage(MSG*msg)
{
	return ::pSrkMfcPreTranslateMessage(this,msg);
}

