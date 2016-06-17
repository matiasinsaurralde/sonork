#include "srkwin32app.h"
#include "sonork.rh"
#pragma hdrstop
#include "srkinputwin.h"

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

TSonorkInputWin::TSonorkInputWin(TSonorkWin*parent)
	:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_INPUT
	, 0)
{
	flags=0;
	sign=SKIN_SIGN_QUERY;
}


bool 	TSonorkInputWin::OnCommand(UINT id,HWND , UINT )
{
	if(id==IDOK)
	{
		int rcid;
		if(flags & SONORK_INPUT_F_LONG_TEXT )
			rcid=IDC_INPUT_LONGTEXT;
		else
			rcid=IDC_INPUT_SHORTTEXT;
		GetCtrlText(rcid,input);
		EndDialog(IDOK);
		return true;
	}
	return false;
}

bool
 TSonorkInputWin::OnCreate()
{
	HWND hwnd = GetDlgItem(IDC_INPUT_SIGN);
	SetWindowText(szSONORK);
	if( flags & SONORK_INPUT_F_PASSWORD )
	{
		::SendMessage(GetDlgItem(IDC_INPUT_SHORTTEXT),EM_SETPASSWORDCHAR,(WPARAM)'*',0);
	}
	::SetWindowPos(hwnd,NULL,0,0,SKIN_SIGN_SW,SKIN_SIGN_SH,SWP_NOMOVE|SWP_NOACTIVATE);
	UpdateForm();
	LoadDefLangEntries();
	return true;
}
void	TSonorkInputWin::UpdateForm()
{
	int id0,id1;
	SetCtrlText(IDC_INPUT_HELP|SONORK_WIN_CTF_BOLD,help.CStr());
	SetCtrlText(IDC_INPUT_PROMPT|SONORK_WIN_CTF_BOLD,prompt.CStr());
	if(flags & SONORK_INPUT_F_LONG_TEXT )
	{
		id1=IDC_INPUT_LONGTEXT;
		id0=IDC_INPUT_SHORTTEXT;
	}
	else
	{
		id1=IDC_INPUT_SHORTTEXT;
		id0=IDC_INPUT_LONGTEXT;
	}
	SetCtrlVisible(id0,false);
	SetCtrlText(id1,input.CStr());
	SetCtrlVisible(id1,true);

}
bool	TSonorkInputWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if( S->CtlID == IDC_INPUT_SIGN )
	{
		//HBRUSH brush;brush = (HBRUSH)GetClassLong(Handle(),GCL_HBRBACKGROUND);

		//::FillRect(S->hDC,&S->rcItem,brush);
		sonork_skin.DrawSign(S->hDC,sign,S->rcItem.left,S->rcItem.top);
		return true;
	}

	return false;
}

