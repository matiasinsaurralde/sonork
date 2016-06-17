#if !defined(SRKTRACKERWIN_H)
#define SRKTRACKERWIN_H

#if !defined(SRKWAITWIN_H)
# include "srkwaitwin.h"
#endif

#if !defined(SRKCONSOLE_H)
# include "srkconsole.h"
#endif


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

class	TSonorkUltraMinWin;

#define SONORK_TRACKER_VERSION_NUMBER		MAKE_VERSION_NUMBER(1,0,0,0)
#define SONORK_TRACKER_INVITE_DATA_ATOM		SONORK_SERVICE_ATOM_001
#define SONORK_TRACKER_TEXT_DATA_ATOM		SONORK_SERVICE_ATOM_002
#define SONORK_TRACKER_TEXT_FLAG_SYSTEM		SONORK_CCLF_01
#define SONORK_TRACKER_TEXT_FLAG_URL		SONORK_CCLF_02
// --------------------------------------------------------------------------

struct TSonorkTrackerInviteData
:public TSonorkCodecAtom
{
	struct HEADER
	{
		DWORD			reserved[8];
	} __SONORK_PACKED;
	HEADER				header;
	TSonorkUserData			user_data;
	TSonorkText             	text;

//----------

// CODEC

public:

	void
		CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_TRACKER_INVITE_DATA_ATOM; 	}

private:

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;

}__SONORK_PACKED;



// --------------------------------------------------------------------------

#define SONORK_TRACKER_TEXT_TAG_FLAG		0
#define SONORK_TRACKER_TEXT_TAG_ATTR		1

struct TSonorkTrackerTextData
:public TSonorkCodecAtom
{

	struct HEADER
	{
		TSonorkTag	tag;
		SONORK_DWORD2	tracking_no;
		SONORK_DWORD2	reserved;
	}__SONORK_PACKED;

	HEADER			header;
	TSonorkDynString	text;
	TSonorkDynData		data;

	DWORD
		TextFlags()
		const { return header.tag.v[SONORK_TRACKER_TEXT_TAG_FLAG];}

	DWORD
		TextAttrs()
		const { return header.tag.v[SONORK_TRACKER_TEXT_TAG_ATTR];}
//----------
// CODEC

public:

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_TRACKER_TEXT_DATA_ATOM; 	}

private:

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;

}__SONORK_PACKED;

// --------------------------------------------------------------------------

struct TSonorkTrackerPendingInvitation
{
	TSonorkWaitWin*		wait_win;
	bool			processed;
	TSonorkTrackerInviteData	invite_data;
	DWORD			service_instance;
	TSonorkServiceCircuit	circuit;
};

// --------------------------------------------------------------------------

class TSonorkTrackerWin
:public TSonorkWin
{

private:
	TSonorkAtomDb		db;
	TSonorkCCache*		cache;
	TSonorkConsole*		console;
	TSonorkUserData		user_data;
	TSonorkTrackerTextData 	temp_TD;
	struct
	{
		HWND	hwnd;
		int	height;
	}toolbar;


	struct unINPUT
	{
		HWND			hwnd;
		int			height;
		TSonorkWinCtrl		ctrl;
		TSonorkDropTarget	drop_target;
	}input;

	struct {
		int x,y;
		TSonorkUltraMinWin*	win;
	}ultra_min;

	DWORD				service_instance;
	TSonorkServiceCircuit		circuit;
	TSonorkTime			last_sound_time;

	BOOL	OnQueryClose();

	bool	OnCreate();
	void	OnBeforeDestroy();
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	LRESULT OnDragDrop(SONORK_DRAG_DROP_EVENT,LPARAM);
	bool	OnDrawItem(DRAWITEMSTRUCT*);
	bool	OnMinMaxInfo(MINMAXINFO*);
	void	OnActivate(DWORD flags,BOOL);

	void 	OnSize(UINT);
	bool	OnAppEvent(UINT event, UINT param,void*data);

	void	RealignControls();
	void	CmdSave();
	void	CmdAddUser();
	void	CmdSearch();

	void	LoadLabels();

	static DWORD SONORK_CALLBACK
		ServiceCallback(
			  SONORK_DWORD2&		handler_tag
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*		event_tag
			, TSonorkAppServiceEvent*	event_data);

	void
		OnCircuitData(SONORK_CTRLMSG_CMD
			, DWORD user_param
			, DWORD user_flags
			, const BYTE* data
			, DWORD data_size);
	void

		OnCircuitData_Text(const BYTE* data , DWORD data_size);


	void

		OnCircuitData_User(const BYTE* data , DWORD data_size);


	void

		OnCircuitClose(SONORK_RESULT);

	static void SONORK_CALLBACK
		UserResponseWaitCallback(
			  TSonorkWaitWin*	WW
			, LPARAM 		cb_param
			, LPVOID 		cb_data
			, SONORK_WAIT_WIN_EVENT event
			, LPARAM  		event_param);

	static	BOOL SONORK_CALLBACK
		CCacheCallback(void*
			,TSonorkCCacheEntry*
			,char*
			,UINT size);

	static DWORD SONORK_CALLBACK
		ConsoleCallback(void*
			,SONORK_CONSOLE_EVENT 	pEvent
			,DWORD				pIndex
			,void*				pData);
	DWORD
		OnHistory_Event(TSonorkHistoryWinEvent*E);

	void
		OnHistory_LinePaint(const TSonorkCCacheEntry*, TSonorkHistoryWinPaintCtx*);

	void
		OnHistory_LineDrag(DWORD );

	DWORD
		OnConsole_Export(TSonorkConsoleExportEvent*);

	void
		OnHistoryWin_GetText(DWORD,TSonorkDynData*DD);

	void
		SendInputText(BOOL reply);

	void
		SendProfileData();

	void
		SendFiles(TSonorkDropTarget*,TSonorkClipData* );

	void
		AddUserText( TSonorkTrackerTextData* );

	void
		AddSysLine(GLS_INDEX);

	void
		AddSysLine(SONORK_C_CSTR);

	void
		AddText(SONORK_C_CSTR
			, const SONORK_DWORD2& tag
			, const SONORK_DWORD2& tracking_no);

	void
		Stop();

	static DWORD
		Service_Register(TSonorkAppServiceCallbackPtr cb_ptr,void*cb_dat);

	void
		UltraMinimizedPaint(struct TSonorkUltraMinPaint*);

	void
		OpenLine(DWORD line_no);

public:
	TSonorkTrackerWin();
	~TSonorkTrackerWin();

	BOOL
		IsConnected() const;

	BOOL
		Call(const TSonorkUserData*UD, SONORK_C_CSTR text);

	BOOL
		Accept(const TSonorkTrackerPendingInvitation*);

	static void
		Init(const TSonorkUserData*UD, SONORK_C_CSTR text);	// UD may NOT be NULL


// New V1.5 service locator messages
	static DWORD SONORK_CALLBACK
		LocatorServiceCallback(
			  SONORK_DWORD2&		handler_tag
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*		event_tag
			, TSonorkAppServiceEvent*	event_data);
	BOOL
		IsUltraMinimized() const
		{ return ultra_min.win != NULL; }

	void
		ConditionalUltraMinimize();
		
	void
		DoSound( bool forced );

};

#endif