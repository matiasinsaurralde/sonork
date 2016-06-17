#if !defined(SRKSETUPWIN_H)
#define SRKSETUPWIN_H

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


#if !defined(SRKDIALOGWIN_H)
# include "srkdialogwin.h"
#endif

#if !defined(SRKLISTVIEW_H)
# include "srklistview.h"
#endif

enum SETUP_LABEL;
class TSonorkSetupWin
:public TSonorkWin
{
public:
	enum STEP
	{
		STEP_INVALID=-1
	,	STEP_WELCOME
	,	STEP_DECIDE
	,	STEP_NEW_1
	,	STEP_NEW_2
	,	STEP_NEW_3
	,	STEP_OLD_1
	,	SETUP_DONE_MARKER
	,	STEP_NEW_DONE
	,	STEP_OLD_DONE
	};
	
	enum PHASE
	{
		PHASE_INVALID	=-2
	,	PHASE_ERROR		=-1
	,	PHASE_NONE
	,	PHASE_RESOLVING
	,	PHASE_CONNECTING
	,	PHASE_AUTHORIZING
	,	PHASE_REQUESTING
	,	PHASE_RETRIEVING
	,	PHASE_DONE
	};

	enum OPERATION
	{
		OP_NONE
	,	OP_SEARCH_USER
	,	OP_CREATE_USER
	};

private:
	struct {
		HWND	hwnd;
	}status_bar;
	struct TSonorkSetupWinLang{
		TSonorkTempBuffer	buffer;
		TSonorkSetupWinLang():buffer(0){}
	}lang;
	TSonorkError			taskERR;
	HANDLE				resolve_handle;
	TSonorkPacketTcpEngine		tcp;
	TSonorkClientServerProfile 	SP;
	STEP				cur_step;
	PHASE				cur_phase;
	OPERATION			cur_op;
	TSonorkWin*			cur_dialog;
	TSonorkUserDataNotes		UDN;
	TSonorkShortString		aux_str;
	TSonorkListView			list;
	struct {
		int x,y,w,h;
	}dialog_pos;
	bool	OnCreate();
	void	OnDestroy();
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	void	OnTimer(UINT);
	LRESULT	OnChildDialogNotify(TSonorkChildDialogNotify*);


	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);


	BOOL	LoadLang(HWND  , const char*, bool update_combo);
	BOOL		_LoadLangLocal(const char*);
	BOOL		_LoadLangApp(const char*);
	void		_LoadLabels();	// _LoadLabels() to avoid conflict with TSonorkWin::LoadLabels
	const char*	GetLabel(TSonorkShortString&,SETUP_LABEL);
	bool	CheckForm();

	void	OnNextButton();
	STEP	NextStep(STEP);
	STEP	PrevStep(STEP);
	void	SetStep(STEP);
	void	SetPhase(PHASE);
	void	SetPhaseError(SETUP_LABEL,TSonorkError*);
	void	SetPhaseCxError(TSonorkError*);
	void	SetPhaseOpError(TSonorkError*pERR);
	void	SetCodecError(SONORK_SYS_STRING,TSonorkError&);
	void	SetLabel(UINT, SETUP_LABEL);
	void 	SetWelcomeLabels();
	void	SetDialogLabel(UINT, SETUP_LABEL);
	void	SetStatusLabel(SETUP_LABEL, SKIN_HICON);
	SONORK_RESULT	CreateProfile();
	void	UpdateTimeZone();
	void	InitializeUserSettings();

	void	SetupDialog();
	void	DoNextOld(bool is_search_button);
	void	StartConnectionPhase(OPERATION);
	void	ProcessPacket(TSonorkTcpPacket*);
	bool		ProcessConnectPacket(TSonorkError&,BYTE*, DWORD);
	void			OnConnect();
	bool		ProcessLoginPacket(TSonorkError&,BYTE*, DWORD);
	void			OnLogin();
	bool		OnReqData(TSonorkDataPacket*,UINT size);
	void		OnReqEnd(TSonorkError&);
public:
	TSonorkSetupWin(TSonorkWin*);



};
/*
class TSonorkSetupDialogWin
:public TSonorkDialogWin
{

	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
public:
	TSonorkSetupDialogWin(TSonorkSetupWin*,int id);

};
*/

#endif