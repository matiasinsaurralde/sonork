#include "srkwin32app.h"
#include "sonork.rh"
#pragma hdrstop
#include "srkdialogwin.h"


TSonorkChildDialogWin::TSonorkChildDialogWin(TSonorkWin*parent, UINT res_id, UINT sys_flags)
	:TSonorkWin(parent
		,SONORK_WIN_CLASS_NORMAL
		|SONORK_WIN_TYPE_NONE
		|SONORK_WIN_DIALOG
		|res_id
		,sys_flags)
{}

bool
 TSonorkChildDialogWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	TSonorkChildDialogNotify	N;
	N.dialog_id 	= WinResId();
	N.N		= (TSonorkWinNotify*)S;
	return Parent()->SendPoke(SONORK_WIN_POKE_CHILD_DRAW_ITEM
		,(LPARAM)&N);
}

bool
 TSonorkChildDialogWin::OnCommand(UINT id,HWND hwnd, UINT notify_code)
{
	TSonorkWinNotify N;
	N.hdr.idFrom 	= id;
	N.hdr.hwndFrom	= hwnd;
	N.hdr.code 	= notify_code;
	return OnNotify(0,&N);
}

LRESULT	TSonorkChildDialogWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	TSonorkChildDialogNotify dN;
	dN.dialog_id 	= WinResId();
	dN.N		= N;
	return Parent()->SendPoke(SONORK_WIN_POKE_CHILD_NOTIFY,(LPARAM)&dN);
}

