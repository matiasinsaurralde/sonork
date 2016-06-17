#include "srkwin32app.h"
#include "sonork.rh"
#pragma hdrstop
#include "srktextwin.h"

TSonorkTextWin::TSonorkTextWin(TSonorkWin*parent)
	:TSonorkWin(parent
	, SONORK_WIN_CLASS_NORMAL
	| SONORK_WIN_TYPE_NONE
	| SONORK_WIN_DIALOG
	| IDD_TEXT
	, 0
	)
{
}
bool
 TSonorkTextWin::OnCreate()
{
	SetWindowText(title.CStr() );
	SetCtrlText( IDC_TEXT_TEXT,text.CStr() );
	return true;
}

