#if !defined(SRKCLIPWIN_H)
#define SRKCLIPWIN_H

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

#if !defined(SRKCONSOLE_H)
# include "srkconsole.h"
#endif

class TSonorkClipWin
:public TSonorkWin
{

private:
	TSonorkShortString	dataListPath;
	TSonorkShortString	dataListName;
	

	struct {
		HWND		hwnd;
		int		height;
	}status;

	struct {
		HWND		hwnd;
		int		height;
	}toolbar;

	TSonorkClipData*	dta;
	TSonorkAtomDb		db;
	TSonorkConsole*		console;
	TSonorkCCache*		cache;
	TSonorkDropTarget*	drop_target;

	int			GetSelectedItem();

	bool			DL_Load();
	void			DL_Save();
	void			DL_Clear();
	SONORK_RESULT 		DL_Add(TSonorkClipData*DD);
	SONORK_RESULT 		DL_GetByDbIndex(DWORD db_index,TSonorkClipData*DD);
	void			DL_SetDefaultPath();

	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	LRESULT OnDragDrop(SONORK_DRAG_DROP_EVENT,LPARAM);
	bool	OnCommand(UINT id,HWND hwnd, UINT code);
	void	OnTimer(UINT);
	BOOL	OnQueryClose();
	bool 	OnCreate();
	void 	OnBeforeDestroy();
	void	OnInitMenu(HMENU);
	void OnSize(UINT);
	void 	RealignControls();


	SONORK_C_CSTR GetCleanFileName(SONORK_C_STR);
	void	UpdateCaption();
	void	OpenFocused( BOOL double_clicked );
	void 	DoExport();
	void	GetCDText(SONORK_C_STR,UINT max_length);
	void	GetCDText(TSonorkShortString&);

	static BOOL SONORK_CALLBACK
				CCacheCallback(void*,TSonorkCCacheEntry*,char*,UINT size);
				
	static DWORD SONORK_CALLBACK
				ConsoleCallback(void*
					,SONORK_CONSOLE_EVENT 	pEvent
					,DWORD	 		pIndex
					,void*	 		pData);

	DWORD	OnConsole_Export(TSonorkConsoleExportEvent*);
	DWORD	OnHistoryWin_Event(TSonorkHistoryWinEvent*);
	void	OnHistoryWin_LineDrag(DWORD);
	void	OnHistoryWin_LinePaint(const TSonorkCCacheEntry*, TSonorkHistoryWinPaintCtx*);
	void	OnHistoryWin_GetText(const TSonorkCCacheEntry*, TSonorkDynData*DD);

	void	CmdAdd();
	
	void	SetStatus_Count(GLS_INDEX,DWORD,SKIN_HICON hicon);
	void	SetStatus_RecordCount(SKIN_HICON hicon=SKIN_HICON_NONE);
	void	SetStatus_SelectCount();
public:
	TSonorkClipWin();
};
#endif
