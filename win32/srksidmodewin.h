#if !defined(SRKSIDMODEWIN_H)
#define SRKSIDMODEWIN_H

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




class TSonorkSidModeWin
:public TSonorkWin
{
public:
	enum TAB
	{
		TAB_VIS=0
	,	TAB_MSG
	,	TABS
	};

private:
	HWND			user_list;
	HWND			priv_check;
	HWND			pub_check;
	TSonorkSidFlags     	form_sid_flags;
	struct {
		HWND	hwnd;
		TAB	page;
	}tab;

	TSonorkWin*
		page[TABS];

	bool
		OnCreate();

	void
		OnBeforeDestroy();

	bool
		OnCommand(UINT id,HWND hwnd, UINT notify_code);

	LRESULT
		OnPoke(SONORK_WIN_POKE,LPARAM);

	LRESULT
		OnNotify(WPARAM,TSonorkWinNotify*N);

	LRESULT
		OnChildDialogNotify(struct TSonorkChildDialogNotify*);


	void
		LoadLabels();

	void
		EnableDisablePrivateMask();

	void
		EnableDisableCheckboxes();

	void
		LoadUserList();

	void
		SetTab(TAB tb , bool update_tab, bool forced );

public:
	TSonorkSidModeWin(TSonorkWin*parent);


	void	Transfer_Sid2Form();
	void	Transfer_Form2Sid( bool private_mask, bool update_ui );


};

#endif