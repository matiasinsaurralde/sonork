#include "srkwin32app.h"
#include "srkappstr.h"
#pragma hdrstop

#if !defined(SONORK_APP_BUILD)
#error This module is for SONORK_APP_BUILD
#endif

#include "srktooltipwin.h"
#include "srkwinctrl.h"
#include "srkerrorwin.h"
#include "srk_dragdrop.h"

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



// ===================================================
// TSonorkWin Constructor/Destructor
// ===================================================

POINT		sonork_win_move_origin;
TSonorkWin*	sonork_active_win=NULL;


TSonorkWin::TSonorkWin(TSonorkWin*parent
	, DWORD def_flags
	, DWORD sys_flags)
{
	// The application window is not constructed
	// using TSonorkWin, it is constructed from TSonorkApp
	win.defFlags	=def_flags;
	win.hwnd	=NULL;
	win.sysFlags	=sys_flags;
	win.sysFlags  &=~(SONORK_WIN_SF_ACTIVE|SONORK_WIN_SF_DESTROYING);
	if( !(win.defFlags & SONORK_WIN_DIALOG_MASK) )
		win.defFlags&=~SONORK_WIN_RES_ID_MASK;
	win.parent	=parent;
	win.eventMask	=win.usrFlags	=0;


	assert( WinClass() != SONORK_WIN_CLASS_APP );
};

TSonorkWin::~TSonorkWin()
{
}


// ===================================================
// TSonorkWin interface methods
// ===================================================


// --------------------------------
// Window appearance control

void TSonorkWin::ShowWindow( int show )
{
	::ShowWindow(Handle(),show);
}
void TSonorkWin::SetWindowPos(HWND hWndInsertAfter,int x,int y,int w,int h,UINT f)
{
	::SetWindowPos(Handle(),hWndInsertAfter,x,y,w,h,f);

}

void TSonorkWin::MoveWindow(int x, int y, int w, int h, bool redraw)
{
	if( Handle() != NULL )
		::MoveWindow(Handle(),x,y,w,h,redraw);
	else
	{
		win.rect.left	=x;
		win.rect.top	=y;
		win.rect.right	=w;
		win.rect.bottom	=h;
	}
}


void TSonorkWin::SetFont(HFONT font, bool redraw)
{
	::SendMessage(Handle(),WM_SETFONT,(WPARAM)font,MAKELPARAM(redraw,0));
}
void TSonorkWin::SetCtrlsFont(UINT*pPtr,HFONT font)
{
	while(*pPtr!=0)
	{
		SetCtrlFont(*pPtr++,font);
	}
}
void TSonorkWin::SetCtrlFont(UINT id, HFONT font)
{
	HWND hWnd = GetDlgItem(id);
	if(hWnd)::SendMessage(hWnd,WM_SETFONT,WPARAM(font),0);
}
void TSonorkWin::SetWindowText(SONORK_C_CSTR str)
{
	::SetWindowText(Handle(),str);
}
void TSonorkWin::SetWindowText(GLS_INDEX gls)
{
	SetWindowText(SonorkApp_LangString(gls));
}

void TSonorkWin::GetClientRect(RECT*rect)
{	::GetClientRect(Handle(),rect);	}
void TSonorkWin::GetWindowRect(RECT*rect)
{	::GetWindowRect(Handle(),rect);	}

void TSonorkWin::GetWindowSize(SIZE*sz)
{
	RECT rect;
	::GetWindowRect(Handle(),&rect);
	sz->cx=rect.right 	- rect.left;
	sz->cy=rect.bottom  - rect.top;
}

void TSonorkWin::UpdateWindow(){	::UpdateWindow(Handle());}
void TSonorkWin::InvalidateRect(RECT*rect,bool erase_bg){ ::InvalidateRect(Handle(),rect,erase_bg);}

void TSonorkWin::ClientToScreen(POINT*p){ ::ClientToScreen(Handle(),p);}
void TSonorkWin::ScreenToClient(POINT*p){ ::ScreenToClient(Handle(),p);}
void TSonorkWin::ScreenToClient(RECT*rect)
{
	POINT p;
	p.x	= rect->left;
	p.y	= rect->top;
	::ScreenToClient(Handle(),&p);
	rect->left	= p.x;
	rect->top	= p.y;

	p.x	= rect->right;
	p.y	= rect->bottom;
	::ScreenToClient(Handle(),&p);
	rect->right	= p.x;
	rect->bottom= p.y;
}
void TSonorkWin::ClientToScreen(RECT*rect)
{
	POINT p;
	p.x	= rect->left;
	p.y	= rect->top;
	::ClientToScreen(Handle(),&p);
	rect->left	= p.x;
	rect->top	= p.y;

	p.x	= rect->right;
	p.y	= rect->bottom;
	::ClientToScreen(Handle(),&p);
	rect->right	= p.x;
	rect->bottom= p.y;
}

// --------------------------------
// Helpers
int			TSonorkWin::MessageBox(const char *title, const char *text, UINT style)
{
	return ::MessageBox(Handle()
		,title
		,text!=NULL?text:szSONORK
		,style);
}

LRESULT
 TSonorkWin::SendMessage(UINT msg, WPARAM wParam,LPARAM lParam)
{
	return	::SendMessage(Handle(),msg,wParam,lParam);
}

BOOL
 TSonorkWin::PostMessage(UINT msg, WPARAM wParam,LPARAM lParam)
{
	return	::PostMessage(Handle(),msg,wParam,lParam);
}

LRESULT
 TSonorkWin::SendPoke(SONORK_WIN_POKE wParam,LPARAM lParam) const
{
	return	::SendMessage(Handle(),WM_SONORK_POKE, wParam ,lParam);
}

BOOL
 TSonorkWin::PostPoke(SONORK_WIN_POKE wParam,LPARAM lParam) const
{
	return	::PostMessage(Handle(),WM_SONORK_POKE, wParam ,lParam);
}

// --------------------------------
// Controls

HWND
 TSonorkWin::GetDlgItem(UINT id) const
{
	return ::GetDlgItem(Handle(),id);
}

BOOL
 TSonorkWin::IsCtrlEnabled(UINT id) const
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)return ::IsWindowEnabled(hwnd);
	return false;

}

bool
 TSonorkWin::GetCtrlPoint(UINT id, POINT*pt) const
{
	RECT rect;
	if(GetCtrlRect(id,&rect))
	{
		pt->x=rect.left;
		pt->y=rect.top;
		::ScreenToClient(Handle(),pt);
		return true;
	}
	return false;
}

bool
 TSonorkWin::GetCtrlRect(UINT id, RECT*rect) const
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)
	{
		::GetWindowRect(hwnd , rect);
		return true;
	}
	return false;
}

UINT
 TSonorkWin::GetCtrlHeight(UINT id) const
{
	RECT rect;
	if( GetCtrlRect(id, &rect) )
		return rect.bottom - rect.top  ;
	return 0;
}

UINT
 TSonorkWin::GetCtrlWidth(UINT id) const
{
	RECT rect;
	if( GetCtrlRect(id, &rect) )
		return rect.right - rect.left  ;
    return 0;
}

bool
 TSonorkWin::GetCtrlSize(UINT id, SIZE*size) const
{
	RECT rect;
	if( GetCtrlRect(id, &rect) )
	{
		size->cx = rect.right - rect.left  ;
		size->cy = rect.bottom- rect.top  ;
		return true;
	}
	return false;
}


bool
 TSonorkWin::GetCtrlText( UINT id, TSonorkShortString& S )
{
	HWND hwnd= GetDlgItem(id);
    if(hwnd)
	{
    	int l=::GetWindowTextLength(hwnd)+1;
        if(l>1)
        {
			S.SetBufferSize( l+1 );
			::GetWindowText(hwnd , S.Buffer() , l);
        	S.Trim();
		}
		else
        	S.Clear();            
        return true;
	}
    S.Clear();
    return false;
}
bool TSonorkWin::GetCtrlText(UINT id, TSonorkDynString& S)
{
	HWND hwnd= GetDlgItem(id);
    if(hwnd)
    {
    	int l=::GetWindowTextLength(hwnd)+1;
		if(l>1)
        {
        	TSonorkTempBuffer tmp(l+1);
	    	::GetWindowText(hwnd , tmp.CStr() , l);
	        SONORK_StrTrim(tmp.CStr());
            S.Set(tmp.CStr());
		}
        else
			S.Clear();
        return true;
	}
    S.Clear();
    return false;

}
bool TSonorkWin::GetCtrlText(UINT id, SONORK_C_STR buffer, int buffer_size)
{
	HWND hwnd= GetDlgItem(id);
    if(hwnd)
    {
    	int l=::GetWindowTextLength(hwnd)+1;
		if(l<=buffer_size)
        {
	    	::GetWindowText(hwnd , buffer , buffer_size);
            SONORK_StrTrim(buffer);
		}
        else
        {
			TSonorkTempBuffer S(l);
			::GetWindowText(hwnd , S.CStr() , l);
			SONORK_StrTrim(S.CStr());
            lstrcpyn(buffer,S.CStr(),buffer_size);

		}
        return true;
	}
    return false;
}

void TSonorkWin::SetEditCtrlMaxLength(UINT id, int v)
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)::SendMessage(hwnd,EM_SETLIMITTEXT,v,0);
}

void TSonorkWin::SetEditCtrlModified(UINT id,BOOL v)
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)::SendMessage(hwnd,EM_SETMODIFY,v!=0,0);
}

bool TSonorkWin::GetEditCtrlModified(UINT id) const
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)
		return ::SendMessage(hwnd,EM_GETMODIFY,0,0)!=0;
	return false;
}

void TSonorkWin::SetCtrlVisible(UINT id, BOOL v)
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)::ShowWindow(hwnd,v!=0?SW_SHOW:SW_HIDE);
}

void TSonorkWin::SetCtrlEnabled(UINT id, BOOL v)
{
	HWND hwnd= GetDlgItem(id);
    if(hwnd)::EnableWindow(hwnd,v!=0?true:false);
}

bool TSonorkWin::SetCtrlSize(UINT id, int w, int h)
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)
	{
		::SetWindowPos(hwnd
		,NULL,0,0,w,h,SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE);
	return true;
	}
	return false;

}
bool TSonorkWin::SetCtrlPoint(UINT id, int x, int y)
{
	HWND hwnd= GetDlgItem(id);
    if(hwnd)
    {
    	::SetWindowPos( hwnd , NULL
        	, x , y , 0 , 0
            , SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

		return true;
	}
    return false;

}
bool TSonorkWin::SetCtrlRect(UINT id, int x, int y, int w, int h)
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)
    {
    	::SetWindowPos( hwnd , NULL
        	, x , y , w , h
            , SWP_NOZORDER | SWP_NOACTIVATE);

		return true;
	}
	return false;

}


void TSonorkWin::SetCtrlTime(UINT id, const TSonorkTime& time , DWORD flags)
{
	char tmp[80];
	TSonorkWin32App::MakeTimeStr( time , tmp, flags);
	SetCtrlText(id,tmp);
	
}

void
	TSonorkWin::AppendCtrlText(UINT id, SONORK_C_CSTR str1 , SONORK_C_CSTR str2)
{
	TSonorkShortString pS;
	GetCtrlText(id, pS );
	pS.Append(str1,str2);
	SetCtrlText( id, pS.CStr() );
}
void
	TSonorkWin::SetCtrlDropText(UINT id, TSonorkClipData*CD,SONORK_C_CSTR append_prefix)
{
	TSonorkShortString S;
	SONORK_C_CSTR		src;
	if(CD->HasTextFormat())
	{
		if(CD->GetTextFormat(&S))
		{
			src = S.CStr();
			if( GetKeyState(VK_CONTROL) & 0x8000 )
				AppendCtrlText(id , append_prefix, src);
			else
				SetCtrlText( id , src );
		}
	}
}
void TSonorkWin::SetCtrlText(UINT id, GLS_INDEX index)
{
	SetCtrlText(id, SonorkApp_LangString(index));
}
void TSonorkWin::SetCtrlUint(UINT id, DWORD v)
{
	::SetDlgItemInt(Handle() , id, v, false);
}

DWORD	TSonorkWin::GetCtrlUint(UINT id)
{
	return ::GetDlgItemInt(Handle(), id , NULL, false);
}


void TSonorkWin::SetCtrlText(UINT id, SONORK_C_CSTR text)
{
//	::SetDlgItemText( Handle() , id , text?text:"");
	HWND hwnd= GetDlgItem(id&0xffff);
	if(!hwnd)return;
	::SetWindowText(hwnd,text?text:"");
	if(id&SONORK_WIN_CTF_BOLD)
		::SendMessage(hwnd
			,WM_SETFONT
			,WPARAM(sonork_skin.Font(SKIN_FONT_BOLD))
			,0);
}
void TSonorkWin::InvalidateCtrl( UINT id, RECT*rect, bool erase_bg )
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)::InvalidateRect( hwnd, rect, erase_bg);
}

void TSonorkWin::LoadLangEntries(const TSonorkWinGlsEntry* E, bool load_def_entries)
{
	if(load_def_entries)LoadDefLangEntries();
	while( E->id != 0 )
	{
		if( E->id == -1 )
			SetWindowText(E->gls_index);
		else
			SetCtrlText(E->id,E->gls_index);
		E++;
	}
}

void TSonorkWin::LoadDefLangEntries()
{
	TSonorkWinGlsEntry E[]=
	{	{IDOK			,GLS_OP_ACCEPT	}
	,	{IDCANCEL		,GLS_OP_CANCEL	}
	,	{IDHELP  		,GLS_LB_HELP	}
	,	{0	     		,GLS_NULL	}};
	LoadLangEntries(E, false);
	
}
void TSonorkWin::SetCtrlCheckState(UINT id, UINT check)
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)::SendMessage(hwnd,BM_SETCHECK,(WPARAM)check,0);
}

void
 TSonorkWin::SetCtrlChecked(UINT id, BOOL set)
{
	SetCtrlCheckState( id, set?BST_CHECKED:BST_UNCHECKED);
}

UINT
 TSonorkWin::GetCtrlCheckState(UINT id) const
{
	HWND hwnd= GetDlgItem(id);
	if(hwnd)return ::SendMessage(hwnd,BM_GETCHECK,0,0);
	return 0;
}
BOOL
 TSonorkWin::GetCtrlChecked(UINT id) const
{
	return GetCtrlCheckState(id) == BST_CHECKED;
}



// --------------------------------
// Window creation/destruction

bool
 TSonorkWin::InitCreateInfo(TSonorkWinCreateInfo*CI)
{
	if( Handle() != NULL )
		return false;
	ClearWinSysFlag(SONORK_WIN_SF_DESTROYING);

	if( TestWinSysFlag(SONORK_WIN_SF_NO_WIN_PARENT) || Parent() == NULL )
	{
		CI->style		=0;
		CI->parent		=NULL;
	}
	else
	{
		CI->parent		=Parent()->Handle();
		CI->style		=WS_CHILD;
	}

	CI->ex_style	=0;
	CI->resource_id	=WinResId();
	CI->hinstance	=SonorkApp_Instance();
	CI->sys_flags	=win.sysFlags;

	if(!OnBeforeCreate(CI))
		return false;

	win.sysFlags	=CI->sys_flags;
	win.defFlags  &=~SONORK_WIN_RES_ID_MASK;
	win.defFlags  |=(CI->resource_id&SONORK_WIN_RES_ID_MASK);

	return true;
}

int
 TSonorkWin::Execute()
{

	TSonorkWinCreateInfo CI;

	if( !IsDialog() )
		return -1;


	if(!InitCreateInfo(&CI))
		return -1;

	if( WinResId()==0 )
		return -1;

	SetWinSysFlag(SONORK_WIN_SF_MODAL);
	return DialogBoxParam(
		 CI.hinstance
		,MAKEINTRESOURCE(WinResId())
		,CI.parent
		,(DLGPROC)Win_Modal_Proc
		,(LPARAM)this);
}

bool	TSonorkWin::Create()
{
	TSonorkWinCreateInfo CI;

	if(!InitCreateInfo(&CI))
		return false;

	// Modal dialogs are created by caling Execute(),
	//  Create() is only for modeless dialogs/windows
	ClearWinSysFlag( SONORK_WIN_SF_MODAL );

	// If the interface object does have a parent, the memory object
	//   MUST also have one;
	// The interface object may or may not have a parent
	//   regardless of whether the memory object has a parent

	if( !TestWinSysFlag( SONORK_WIN_SF_NO_WIN_PARENT ) )
		assert( CI.parent!=NULL );

	if( IsDialog() )
	{
		// Modeless dialogs
		if(WinResId() == 0 )
			return false;
		win.hwnd=::CreateDialogParam(
			 CI.hinstance
			,MAKEINTRESOURCE( WinResId() )
			,CI.parent
			,(DLGPROC)Win_Modeless_Proc
			,(LPARAM)this);

	}
	else
	{
		// Normal window

		win.hwnd=::CreateWindowEx(
				 CI.ex_style
				,WinClassName()
				,""
				,CI.style
				,Left()
				,Top()
				,Width()
				,Height()
				,CI.parent
				,NULL
				,CI.hinstance
				,this);
	}
	if(win.hwnd)
	{
		SetWindowLong(GWL_USERDATA	,(LONG)this);
		SetWinSysFlag(SONORK_WIN_SF_WAS_CREATED);
		::SetCursor(LoadCursor(NULL,IDC_ARROW));

//		if(!TestWinSysFlag(SONORK_WIN_SF_NO_DEFAULT_FONT))
//		SetFont( guskin.font.main ,false);

		if( IsDialog() )
		{
			RECT rect;
			GetClientRect(&rect);
			win.rect.right	=rect.right-rect.left;
			win.rect.bottom	=rect.bottom-rect.top;
			GetWindowRect(&rect);
			win.rect.left	=rect.left;
			win.rect.top	=rect.top;
		}
		SonorkApp_ReportWinCreate(this,true);
		OnCreate();
		LoadLabels();

		//GuApp.PostAppCommand(SONORK_APP_COMMAND_WIN_CREATED,(LPARAM)this);
		return true;
	}
//	TRACE_DEBUG("CreateWindowEx(%x) failed Error=%u",this,GetLastError());
	return false;
}

void	TSonorkWin::EndDialog(int code)
{
	if(Handle() != NULL && !MultiTestWinSysFlags(SONORK_WIN_SF_DESTROYING|SONORK_WIN_SF_DESTROYED))
	{
		SetWinSysFlag(SONORK_WIN_SF_DESTROYING);
		if( IsModal() )
			::EndDialog(Handle() , code );
		else
			::DestroyWindow( Handle() );
	}
}

void	TSonorkWin::Destroy(int code)
{
	if(Handle() != NULL && !MultiTestWinSysFlags(SONORK_WIN_SF_DESTROYING|SONORK_WIN_SF_DESTROYED))
	{
		SetWinSysFlag(SONORK_WIN_SF_DESTROYING);
		if( IsModal() )
			::EndDialog(Handle() , code);
		else
			::DestroyWindow( Handle() );
	}

}
BOOL	TSonorkWin::GetStayOnTop()
{
	return GetWindowLong(GWL_EXSTYLE)&WS_EX_TOPMOST;
}
void	TSonorkWin::SetStayOnTop(BOOL s)
{
	BOOL is_top = GetStayOnTop();
	if(s)
	{
		if(is_top)return;
	}
	else
	{
		if(!is_top)return;
	}
	::SetWindowPos(Handle()
		,s?HWND_TOPMOST:HWND_NOTOPMOST
		,0,0,0,0
		,SWP_NOSIZE|SWP_NOMOVE);//|(is_top?SWP_NOREDRAW:0));
}

// --------------------------------
// ToolTips
// See gutooltipwin module

void
 TSonorkWin::AddToolTipCtrl(UINT count,UINT*id_list)
{
	SetWinSysFlag(SONORK_WIN_SF_TOOL_TIPS);
	SONORK_TT_AddCtrl(Handle(),count,id_list);
}
void
 TSonorkWin::AddToolTipCtrl(HWND tt_hwnd)
{
	SetWinSysFlag(SONORK_WIN_SF_TOOL_TIPS);
	SONORK_TT_AddCtrl(Handle(),tt_hwnd);
}
void	TSonorkWin::AddToolTipRect(UINT id, int x1, int y1, int x2, int y2)
{
	RECT rect;
	rect.left = x1;
	rect.top  = y1;
	rect.right= x2;
	rect.bottom=y2;
	SONORK_TT_AddRect(Handle(),id,&rect);

	// Set the flag that indicates we've got tooltips.
	// The flag is used in OnDestroy() to avoid calling
	// uselessly the tool tip cleanup function.
	// No harm is done if the cleanup function is called
	// without tooltips, it is just to improve perfomance
	SetWinSysFlag(SONORK_WIN_SF_TOOL_TIPS);
}

// ===================================================
// TSonorkWin default handlers
// ===================================================

LRESULT TSonorkWin::OnCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if( IsDialog() )
	{
		if(TestWinSysFlag(SONORK_WIN_SF_APP_COLORS))
			return sonork_skin.OnCtlColorDialog( wParam );
		if( IsModal() )
			return 0; 	// DefWindowProc is not called for dialogs
		else
			return ::DefDlgProc( Handle() , uMsg, wParam, lParam );
	}
	else
		return	::DefWindowProc( Handle() , uMsg, wParam, lParam );
}


// ===================================================
// TSonorkWin protected methods
// ===================================================


// ===================================================
// TSonorkWin internal handlers
// ===================================================


LRESULT CALLBACK
 TSonorkWin::Win_Proc(HWND hwnd,UINT uMsg ,WPARAM wParam ,LPARAM lParam)
{
	// The GWL_USERDATA is set in Create() to the TSonorkWin's pointer
	TSonorkWin *win=(TSonorkWin *)::GetWindowLong(hwnd,GWL_USERDATA);
	if(win != NULL)
		return win->WinProc(uMsg,wParam,lParam);
	return	DefWindowProc( hwnd, uMsg, wParam, lParam );
}


LRESULT CALLBACK
 TSonorkWin::Win_Modeless_Proc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if(uMsg == WM_INITDIALOG)
	{
		::SetWindowLong(hwnd,GWL_WNDPROC,(LONG)Win_Proc);
		return Win_Modal_Proc(hwnd,uMsg,wParam,lParam);
	}
	return 0L;
}
LRESULT CALLBACK
 TSonorkWin::Win_Modal_Proc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	TSonorkWin *win;
	LRESULT	result;
	if(uMsg == WM_INITDIALOG)
	{
		// WM_INITDIALOG is the first message
		// with lParam set to the TSonorkWin's pointer
		// we passed to CreateDialogBoxParam;
		// We store it in GWL_USERDATA for
		// posterior messages
		win=(TSonorkWin *)lParam;
		if(win)
		{
			::SetWindowLong(hwnd,GWL_USERDATA,lParam);
			win->SetWinSysFlag(SONORK_WIN_SF_WAS_CREATED);
			assert(win->win.hwnd == NULL);
			win->win.hwnd = hwnd;
//			win->SetFont( guskin.font.main,false);
		}
	}
	else
		win=(TSonorkWin *)::GetWindowLong(hwnd,GWL_USERDATA);

	if(win != NULL)
	{
		result = win->WinProc(uMsg,wParam,lParam);
		::SetWindowLong(hwnd,DWL_MSGRESULT,result);
		return result;

	}
	return false;
}


BOOL
 TSonorkWin::PreTranslateMessage(MSG*msg)
{
	if( IsDialog() )
		return ::IsDialogMessage(Handle(),msg);
	else
		return false;
}
void	TSonorkWin::FillBg(HDC tDC,HBRUSH tBrush)
{
	RECT rect;
	rect.left 	= rect.top = 0;
	rect.right  = Width();
	rect.bottom	= Height();
	::FillRect(tDC,&rect,tBrush);
}

LRESULT
 TSonorkWin::WinProc(UINT uMsg ,WPARAM wParam ,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_GETFONT:
			//if(!TestWinSysFlag(SONORK_WIN_SF_NO_DEFAULT_FONT))
			//return (LRESULT)sonork_skin.Font(SKIN_F O N T _ M A I N );
			break;

		case WM_COMMAND:
			return WmCommand(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			if(OnKeyDown(uMsg==WM_SYSKEYDOWN,wParam,lParam))
				return 0;
			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			if(OnKeyUp(uMsg==WM_SYSKEYUP,wParam,lParam))
				return 0;
			break;


		case WM_SYSCHAR:
		case WM_CHAR:
			if(OnChar(uMsg==WM_SYSCHAR,(TCHAR)wParam,lParam))
				return 0;
			break;

		case WM_VSCROLL:
			return !OnVScroll((int)LOWORD(wParam),(int)HIWORD(wParam),(HWND)lParam);

		case WM_TIMER:
			OnTimer( wParam );
			return 0;

		case WM_GETDLGCODE:
			return OnGetDlgCode((MSG*)lParam);


		case WM_SETFOCUS:
			return !OnSetFocus(true);

		case WM_KILLFOCUS:
			return !OnSetFocus(false);

		case WM_SYSCOMMAND:
			if ( !WmSysCommand(wParam&0xfff0) )
				break;
			return 0L;

		case WM_CANCELMODE:
			OnCancelMode();
			break;

		case WM_INITDIALOG:
			return WmInitDialog(wParam,lParam);

//		case WM_WINDOWPOSCHANGED:WmWindowPosChanged((WINDOWPOS*)lParam);return true;

		case WM_INITMENU:
			OnInitMenu((HMENU)wParam);
			break;

		case WM_INITMENUPOPUP :
			OnInitMenuPopup( (HMENU)wParam
				, (UINT)LOWORD(lParam)
				, (BOOL)HIWORD(lParam));
			break;

		case WM_PARENTNOTIFY:
			{
				int event	=LOWORD(wParam);
				UINT id  	=HIWORD(wParam);
				WmParentNotify(event,id,lParam);
			}
			break;

		case WM_MEASUREITEM:
			{
				MEASUREITEMSTRUCT*MI=(MEASUREITEMSTRUCT*)lParam;
				return WmMeasureItem(MI);
			}

		case WM_DRAWITEM:
			{
				DRAWITEMSTRUCT*DI=(DRAWITEMSTRUCT*)lParam;
				return WmDrawItem(DI);

			}
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLORSCROLLBAR:
			return OnCtlColor(uMsg,wParam,lParam);

		case WM_ERASEBKGND:
			if( TestWinSysFlag(SONORK_WIN_SF_APP_COLORS) )
			{
				FillBg((HDC)wParam,sonork_skin.Brush(SKIN_BRUSH_DIALOG));
				return true;
			}
			if(OnEraseBG((HDC)wParam))
				return true;
			break;

		case WM_PAINT:
			if(!IsDialog())
			{
				WmPaint( (HDC)wParam );
				return 0L;
			}
			if( TestWinSysFlag(SONORK_WIN_SF_APP_COLORS) )
			{
				HDC t_dc;
				PAINTSTRUCT ps;
				t_dc = BeginPaint(Handle(),&ps);
				if(t_dc)
				{
					FillRect(t_dc, &ps.rcPaint, sonork_skin.Brush(SKIN_BRUSH_DIALOG));
					EndPaint(Handle(),&ps);
				}
				return 0L;
			}
			break;



		case WM_NOTIFY:
			return WmNotify(wParam,(NMHDR*)lParam);

		case WM_SHOWWINDOW:
			OnShowWindow((BOOL)wParam, (int)lParam);
		break;


		case WM_ACTIVATE:
		{
			DWORD flags=LOWORD(wParam);
			if( flags == WA_INACTIVE )
			{
				if(sonork_active_win==this)
					sonork_active_win = NULL;
				if(!TestWinSysFlag(SONORK_WIN_SF_ACTIVE))
					break;
				ClearWinSysFlag(SONORK_WIN_SF_ACTIVE);
			}
			else
			if( flags == WA_ACTIVE || flags==WA_CLICKACTIVE )
			{
				if( IsDialog() )
					sonork_active_win=this;
				if(TestWinSysFlag(SONORK_WIN_SF_ACTIVE))
					break;
				SetWinSysFlag(SONORK_WIN_SF_ACTIVE);
			}
			OnActivate(flags,HIWORD(wParam));
		}
		break;

		case WM_CLOSE:
			if( !( TestWinSysFlag(SONORK_WIN_SF_NO_CLOSE) || OnQueryClose() ))
				Destroy();
		break;

		case WM_DESTROY:
			WmDestroy();
		break;

		case WM_MOVE:
				WmMove((POINTS*)&lParam);
			break;

		case WM_SIZE:
				WmSize(wParam,(POINTS*)&lParam);
			break;
		case WM_GETMINMAXINFO:
				// Return 0 if processed
				return 	!OnMinMaxInfo((MINMAXINFO*)lParam);

		case WM_MOUSEMOVE:

			{
				POINTS P=MAKEPOINTS(lParam);
				OnMouseMove(wParam,P.x,P.y);
			}
			break;

		case WM_LBUTTONDOWN:
			{
				POINTS P=MAKEPOINTS(lParam);
				OnLButtonDown(wParam,P.x,P.y);
			}
			break;
		case WM_LBUTTONDBLCLK:
			{
				POINTS P=MAKEPOINTS(lParam);
				OnLButtonDblClk(wParam,P.x,P.y);
			}
			break;
		case WM_LBUTTONUP:
			{
				POINTS P=MAKEPOINTS(lParam);
				OnLButtonUp(wParam,P.x,P.y);
			}
			break;
		case WM_RBUTTONDOWN:
			{
				POINTS P=MAKEPOINTS(lParam);
				OnRButtonDown(wParam,P.x,P.y);
			}
			break;
		case WM_RBUTTONUP:
			{
				POINTS P=MAKEPOINTS(lParam);
				OnRButtonUp(wParam,P.x,P.y);
			}
			break;

  //		case WM_SONORK_APP_MESSAGE:			return 0;


		case WM_SONORK_POKE:
			return OnPoke( (SONORK_WIN_POKE)wParam , lParam );

		case WM_SONORK_MFC:
			return OnMfcEvent( (SRK_MFC_EVENT)wParam , lParam);

		case WM_SONORK_MODAL_CREATE:
			assert( IsModal() );
			_OnAfterCreate();
			break;

		case WM_SONORK_DRAG_DROP:
			return OnDragDrop( (SONORK_DRAG_DROP_EVENT)wParam , lParam);

	}
	if( IsDialog() )
	{
		if( IsModal() )return 0L;
		return ::DefDlgProc(Handle() , uMsg, wParam, lParam );
	}
	return	::DefWindowProc( Handle() , uMsg, wParam, lParam );
}

void	TSonorkWin::_OnAfterCreate()
{
	win.sysFlags|=SONORK_WIN_SF_INITIALIZED;
	OnAfterCreate();
}

// WmMove: Store our position when we've been moved
void	TSonorkWin::WmMove(POINTS*P)
{
	win.rect.left=P->x;
	win.rect.top=P->y;
	OnMove();
}

// WmSize: Store our size when we've been resize,
// also generates OnMinimize() event
void	TSonorkWin::WmSize(UINT sizeType,POINTS*P)
{
	if( sizeType == SIZE_RESTORED || sizeType==SIZE_MAXIMIZED )
	{
		win.rect.right	= P->x;
		win.rect.bottom	= P->y;
		if(TestWinSysFlag(SONORK_WIN_SF_MINIMIZED))
		{
			ClearWinSysFlag(SONORK_WIN_SF_MINIMIZED);
			OnMinimize(false);
		}
	}
	else
	if( sizeType == SIZE_MINIMIZED )
	{
		if(!TestWinSysFlag(SONORK_WIN_SF_MINIMIZED))
		{
			SetWinSysFlag(SONORK_WIN_SF_MINIMIZED);
			OnMinimize(true);
		}
	}
	OnSize(sizeType);
}

void	TSonorkWin::WmPaint( HDC )
{
	HDC t_dc;
	PAINTSTRUCT ps;
	t_dc = BeginPaint(Handle(),&ps);
	if(t_dc)
	{
		OnPaint(t_dc, ps.rcPaint, ps.fErase);
		EndPaint(Handle(),&ps);
	}
	else
	{
		// BeginPaint failed !(??)
	}
}

bool 	TSonorkWin::WmMeasureItem(MEASUREITEMSTRUCT*S)
{
	return OnMeasureItem(S);
}


bool 	TSonorkWin::WmDrawItem	( DRAWITEMSTRUCT*S)
{
	return OnDrawItem(S);
}

// ---------------------------------------------
// NOTIFICATIONS

void TSonorkWin::WmParentNotify(int event, UINT id, LPARAM lParam)
{
    switch(event)
    {
        case WM_CREATE:
        {
            OnChildCreate(id,(HWND)lParam);
        }
        break;

        case WM_DESTROY:
        {
            OnChildDestroy(id,(HWND)lParam);
        }
        break;

        case WM_LBUTTONDOWN:
        {
			POINTS P=MAKEPOINTS(lParam);
            OnChildLButton(id,P.x,P.y);
        }
		break;
    }
}


	/*
void	TSonorkWin::WmWindowPosChanged(WINDOWPOS*WP)
{
	UINT F=WP->flags;
	if(TestWinUsrFlag(SONORK_WIN_F_TRACE_DEBUG))
	{
		TRACE_DEBUG("[%x]:WmWPC(%x,%d,%d,%d,%d %04x=%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u)"
			,Handle()
			,WP->hwndInsertAfter
			,WP->x
			,WP->y
			,WP->cx
			,WP->cy
			,F
			,F&0x8000?1:0
			,F&0x4000?1:0
			,F&0x2000?1:0
			,F&0x1000?1:0
			,F&0x800?1:0
			,F&0x400?1:0
			,F&0x200?1:0
			,F&0x100?1:0
			,F&0x80?1:0
			,F&0x40?1:0
			,F&0x20?1:0
			,F&0x10?1:0
			,F&0x8?1:0
			,F&0x4?1:0
			,F&0x2?1:0
			,F&0x1?1:0
			);
	}
	if( !(F&SWP_NOMOVE) )
	{
		win.pos.x=WP->x;
		win.pos.y=WP->y;
		if(TestWinUsrFlag(SONORK_WIN_F_TRACE_DEBUG))
			TRACE_DEBUG("TSonorkWin[%x]::WmWPC:OnMove(%d,%d)",Handle(),win.pos.x,win.pos.y);
		OnMove();

	}
	
	if( !(F&SWP_NOSIZE) )
	{
		RECT rect;
		GetClientRect(&rect);
		win.pos.w=rect.right;
		win.pos.h=rect.bottom;
		if(TestWinSysFlag(SONORK_WIN_SF_MINIMIZED))
		{
			if(!IsIconic())
			{
				ClearWinSysFlag(SONORK_WIN_SF_MINIMIZED);
				if(TestWinUsrFlag(SONORK_WIN_F_TRACE_DEBUG))
					TRACE_DEBUG("TSonorkWin[%x]::WmWPC:OnMinimize(FALSE)",Handle());
				OnMinimize(false);
			}
		}
		else
		{
			if(IsIconic())
			{
				if(TestWinUsrFlag(SONORK_WIN_F_TRACE_DEBUG))
					TRACE_DEBUG("TSonorkWin[%x]::WmWPC:OnMinimize(TRUE)",Handle());
				SetWinSysFlag(SONORK_WIN_SF_MINIMIZED);
				OnMinimize(true);
			}
		}
		if(TestWinUsrFlag(SONORK_WIN_F_TRACE_DEBUG))
			TRACE_DEBUG("TSonorkWin[%x]::WmWPC:OnSize(RESTORE,%d,%d)",Handle(),win.pos.w,win.pos.h);
		OnSize( SIZE_RESTORED );
	}

}
	*/

LRESULT TSonorkWin::WmNotify(WPARAM wParam, NMHDR *hdr)
{
	switch(hdr->code)
	{
		case TTN_NEEDTEXT:
			{
				TOOLTIPTEXT *I=(TOOLTIPTEXT *)hdr;
				TSonorkWinToolTipText TTT;
				TTT.gls=GLS_NULL;
				TTT.str[0]=0;
				OnToolTipText( hdr->idFrom
					, hdr->hwndFrom, TTT );
				if( *TTT.str !=0 )
					I->lpszText=(char*)TTT.str;
				else
					I->lpszText=(char*)SonorkApp_LangString(TTT.gls);

				return 0L;
			}
		default:
			// Call OnNotify for all other unhandled/unknown notifications.
			return OnNotify(wParam,(TSonorkWinNotify*)hdr);
	}
//	return 0;
}

void TSonorkWin::QueryClose()
{
	if( !OnQueryClose() )
	{
		Destroy();
	}
}
// ---------------------
// WM_COMMAND

bool  TSonorkWin::WmCommand(UINT id, UINT notify_code, HWND sender)
{
	bool processed;
	processed=OnCommand(id,sender,notify_code);
	if(sender != NULL)
	{
		// Sender is a control
		if(!processed && notify_code == BN_CLICKED)
		{
			// If the ID of the control is IDCANCEL,
			// and the window does not process it nor
			// does it have the NO_CLOSE flag, we close it.
			if(id==IDCANCEL)
			{
				if( !(TestWinSysFlag(SONORK_WIN_SF_NO_CLOSE) || OnQueryClose()) )
				{
					Destroy();
				}
				processed=true;
			}
		}
	}
	return !processed;	// WM_COMMAND: return 0 if processed
}

// ---------------------
// WM_SYS_COMMAND

bool  TSonorkWin::WmSysCommand(UINT id)
{
	bool processed=OnSysCommand(id);

	if(!processed)
	{
		switch(id)
		{
			case SC_CLOSE:
				if( !( TestWinSysFlag(SONORK_WIN_SF_NO_CLOSE) || OnQueryClose() ))
					Destroy();
				processed=true;
				break;
			case SC_RESTORE:
				::ShowWindow(Handle(),SW_RESTORE);
				processed=true;
				break;
			case SC_MOVE:
				break;
		}

	}
	return processed;
}



// -------------------
//  Creation/destruction

LRESULT TSonorkWin::WmInitDialog(WPARAM ,LPARAM )
{
	BOOL rv;
	if( IsModal() )
	{
		RECT rect;
		GetClientRect(&rect);
		win.rect.left	=rect.left;
		win.rect.top	=rect.top;
		win.rect.right	=rect.right-rect.left;
		win.rect.bottom	=rect.bottom-rect.top;
		rv = OnCreate();
		LoadLabels();
		PostMessage(WM_SONORK_MODAL_CREATE,0,0);
	}
	else
		rv = true;

	// WM_INIT_DIALOG: Should Return true if windows should set focus.
	return rv;
}


void	TSonorkWin::WmDestroy()
{
	if( Handle()!= NULL && !TestWinSysFlag(SONORK_WIN_SF_DESTROYED) )
	{
		win.sysFlags|=SONORK_WIN_SF_DESTROYED|SONORK_WIN_SF_DESTROYING;

		if(sonork_active_win==this) // (Just to make absolutely sure)
			sonork_active_win=NULL;

		// OnBeforeDestroy is for cleaning up stuff
		// before we start doing it for the window;
		// the Handle() is valid.
		OnBeforeDestroy();
		KillAuxTimer();

		if(TestWinSysFlag(SONORK_WIN_SF_TOOL_TIPS))
			SONORK_TT_Del( Handle() );



		OnDestroy();
		// OnDestroy is invoked after the window handle
		// has been destroyed and is no longer valid.
		win.hwnd = NULL;
		
		OnCleanup();	// Special _OnDestroy() for subclassing

		if( !IsModal() )
		{
			// The application will call the OnAfterDestroy()
			// just before our class instance is deleted.
			// The OnAfterDestroy() will be called when no
			// other events are being processed.
			SonorkApp_ReportWinCreate(this,false);
		}
	}
}

static TSonorkWinClassDef
	sonork_win_class[SONORK_WIN_CLASSES]=
{
	 {NULL		,0,NULL}
	,{"SonorkMain"	,0,NULL}
	,{"SonorkWin"	,CS_DBLCLKS,NULL}
};


const char*
	TSonorkWin::Win_ClassName( SONORK_WIN_CLASS classId )
{
	if( classId == SONORK_WIN_CLASS_NORMAL )
		return sonork_win_class[2].name;

	if( classId == SONORK_WIN_CLASS_MAIN   )
		return sonork_win_class[1].name;
	return "??";
}



bool
 TSonorkWin::InitModule(SONORK_C_CSTR app_win_class)
{
	UINT 		i;
	BOOL		bFlag;
	WNDCLASS	C;
	INITCOMMONCONTROLSEX ic;


	bFlag 		= false;

	ic.dwSize	= sizeof(ic);
	ic.dwICC	= ICC_LISTVIEW_CLASSES
				| ICC_TAB_CLASSES
				| ICC_TREEVIEW_CLASSES
				| ICC_UPDOWN_CLASS
				| ICC_PROGRESS_CLASS
				| ICC_BAR_CLASSES

				| ICC_DATE_CLASSES;

#if defined(__BORLANDC__ ) && __BORLANDC__  < 0x0540
typedef BOOL WINAPI tInitCommonControlsEx(LPINITCOMMONCONTROLSEX);
typedef tInitCommonControlsEx* pInitCommonControlsEx;

	HINSTANCE				TH;
	pInitCommonControlsEx	ptr=NULL;
	TH = LoadLibrary("COMCTL32.DLL");
	if(TH)
	{
		ptr = (pInitCommonControlsEx)GetProcAddress(TH,"InitCommonControlsEx");
		if( ptr )
			bFlag = ptr(&ic);
		FreeLibrary(TH);
	}
	if(!ptr)
	{
		InitCommonControls();
		bFlag=true;
	}
#else
	bFlag = InitCommonControlsEx(&ic);
#endif

	if(!bFlag)return false;

	//g_rich_edit_dll = LoadLibrary("RICHED32.DLL");
	SONORK_TT_Init();
	for( i=0 ; i < SONORK_WIN_CLASSES ; i++)
	{
		C.style		=sonork_win_class[i].style;
		C.cbClsExtra	=0;
		C.cbWndExtra	=sizeof(void*);
		C.hInstance	=SonorkApp_Instance();
		C.hIcon		=NULL;
		C.hCursor	=LoadCursor(NULL,IDC_ARROW);
		C.hbrBackground	=(HBRUSH)(COLOR_WINDOW+1);
		C.lpszMenuName 	=NULL;
		if( i==0 )sonork_win_class[i].name=app_win_class;
		C.lpszClassName	=sonork_win_class[i].name;
		C.lpfnWndProc	=TSonorkWin::Win_Proc;
		sonork_win_class[i].atom=::RegisterClass(&C);
		if(!sonork_win_class[i].atom)
			return false;
	}
	return true;

}
void
 TSonorkWin::ExitModule()
{
	UINT i;
	for( i=0 ; i < SONORK_WIN_CLASSES ; i++)
		if( sonork_win_class[i].atom != NULL )
			::UnregisterClass( sonork_win_class[i].name , SonorkApp_Instance() );
	SONORK_TT_Exit();
//	if( g_rich_edit_dll != NULL )FreeLibrary(g_rich_edit_dll);
}


HDC
 TSonorkWin::GetDC()
{
	return ::GetDC(Handle());
}

int
 TSonorkWin::ReleaseDC(HDC hdc)
{
	return ::ReleaseDC(Handle(), hdc);
}

UINT
 TSonorkWin::SetAuxTimer(UINT msecs)
{
	KillAuxTimer();
	SetWinSysFlag(SONORK_WIN_SF_AUX_TIMER);
	return SetTimer(SONORK_WIN_AUX_TIMER_ID, msecs);
}

UINT
 TSonorkWin::SetTimer(UINT id, UINT msecs)
{
	return ::SetTimer(Handle(),id,msecs,NULL);
}

void
 TSonorkWin::KillAuxTimer()
{
	if(TestWinSysFlag(SONORK_WIN_SF_AUX_TIMER))
	{
		KillTimer(SONORK_WIN_AUX_TIMER_ID);
		ClearWinSysFlag(SONORK_WIN_SF_AUX_TIMER);
	}
}

BOOL
 TSonorkWin::KillTimer(UINT id)
{
	return ::KillTimer(Handle(),id);
}

// Subclasses are recommend to call TSonorkWin::TransferStartInfo before
// processing this method
void	TSonorkWin::TransferStartInfo(TSonorkWinStartInfo*SI, BOOL load)
{
	if( load )
	{
		DWORD style = GetWindowLong(GWL_STYLE);
		DWORD swp   = SWP_NOZORDER|SWP_NOACTIVATE;
		RECT rect;
		POINT origin;
		POINT limit;
		::SystemParametersInfo(SPI_GETWORKAREA, 0 , &rect , 0);
		origin.x	= rect.left;
		origin.y	= rect.top;
		limit.x		= rect.right;
		limit.y		= rect.bottom;
		rect.right -= rect.left;
		rect.bottom-= rect.top;


		if( SI->pos.pt.x<0 || SI->pos.pt.x >= limit.x)
			SI->pos.pt.x=0;
		if( SI->pos.pt.y<0 || SI->pos.pt.y >= limit.y)
			SI->pos.pt.y=0;

		if( style & WS_THICKFRAME )
		{
			if( SI->pos.sz.cx > rect.right  )
				SI->pos.sz.cx = rect.right;
			if( SI->pos.sz.cy > rect.bottom  )
				SI->pos.sz.cy = rect.bottom;

		}
		else
		{
			swp|=SWP_NOSIZE;
			SI->pos.sz.cx = Width();
			SI->pos.sz.cy = Height();
		}
		if( SI->pos.pt.x + SI->pos.sz.cx > limit.x )
			SI->pos.pt.x = limit.x - SI->pos.sz.cx;
		if( SI->pos.pt.y + SI->pos.sz.cy > limit.y )
			SI->pos.pt.y = limit.y - SI->pos.sz.cy;
		SetWindowPos(NULL
				,SI->pos.pt.x
				,SI->pos.pt.y
				,SI->pos.sz.cx
				,SI->pos.sz.cy
				,swp);
	}
	else
	{
		RECT rect;
		GetWindowRect(&rect);
		SI->pos.pt.x = rect.left;
		SI->pos.pt.y = rect.top;
		SI->pos.sz.cx = rect.right - rect.left;
		SI->pos.sz.cy = rect.bottom - rect.top;
		SI->win_flags=0;
		SONORK_ZeroMem(SI->reserved,sizeof(SI->reserved));
	}
}



	// Used to send messages from the application to the windows
void TSonorkWin::DispatchAppEvent(UINT event, UINT param,void*data)
{
	if(!OnAppEvent( event, param, data ))
	switch(event)
	{
		case SONORK_APP_EVENT_MAINTENANCE:
		case SONORK_APP_EVENT_SHUTDOWN:
			Destroy();
			break;
		case SONORK_APP_EVENT_SET_LANGUAGE:
			LoadLabels();
			InvalidateRect(NULL,true);
			break;
	}
}
LRESULT	TSonorkWin::OnCtlWinMsg(TSonorkWinCtrl*WC,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return WC->DefaultProcessing(uMsg,wParam,lParam);
}

void	TSonorkWin::SetCaptionIcon(HICON hicon, UINT type)
{
	::SendMessage(Handle(),WM_SETICON,type,(LPARAM)hicon);
}
void	TSonorkWin::SetCaptionIcon(SKIN_HICON hicon, UINT type)
{
	SetCaptionIcon(sonork_skin.Hicon(hicon) , type );
}

BOOL	TSonorkWin::ChangeWinUsrFlag(SONORK_WIN_USR_FLAG flag, BOOL v)
{
	BOOL ov = TestWinUsrFlag(flag);
	if(v)
	{
		if(ov)return false;
		SetWinUsrFlag(flag);
	}
	else
	{
		if(!ov)return false;
		ClearWinUsrFlag(flag);
	}
	return true;
}



int
 TSonorkWin::MessageBox(const char *text, GLS_INDEX	title, UINT flags)
{
	return ::MessageBox(Handle(),text,SonorkApp_LangString(title),flags);
}

int
 TSonorkWin::MessageBox(GLS_INDEX text, const char *title, UINT flags)
{
	return ::MessageBox(Handle(),SonorkApp_LangString(text),title,flags);
}

int
 TSonorkWin::MessageBox(GLS_INDEX text, GLS_INDEX title, UINT flags)
{
	return ::MessageBox(Handle(),SonorkApp_LangString(text),SonorkApp_LangString(title),flags);
}

void
 TSonorkWin::ErrorBox(SONORK_C_CSTR text, TSonorkError*pERR, SKIN_SIGN sign)
{
	TSonorkErrorWin EW(this,text,pERR,sign);
	EW.Execute();
}

void
 TSonorkWin::ErrorBox(GLS_INDEX text, TSonorkError*pERR, SKIN_SIGN sign)
{
	ErrorBox(SonorkApp_LangString(text),pERR,sign);
}

void
 TSonorkWin::TaskErrorBox(GLS_INDEX gls_task, TSonorkError*pERR, SKIN_SIGN sign)
{
	TSonorkShortString str;
	MkTaskErrorString(str,gls_task);
	ErrorBox(str.CStr(),pERR,sign);
}
void
	TSonorkWin::SetHintModeLang(GLS_INDEX gls, bool update_window)
{
	SetHintMode(SonorkApp_LangString(gls),update_window);
}

void
 TSonorkWin::SetStatus(HWND hwnd,SONORK_C_CSTR str, SKIN_HICON ic)
{
	LPARAM lp;
	::SendMessage(hwnd
		,SB_SETTEXT
		,0
		,(LPARAM)str);
	lp=(LPARAM)sonork_skin.Hicon(ic);
	::SendMessage(hwnd
		,SB_SETICON
		,0
		,lp);
}
void
 TSonorkWin::SetStatus(HWND hwnd,GLS_INDEX str, SKIN_HICON ic)
{
	TSonorkWin::SetStatus(hwnd,SonorkApp_LangString(str),ic);
}
void
 TSonorkWin::SetStatus_PleaseWait(HWND hwnd)
{
	TSonorkWin::SetStatus(hwnd,GLS_MS_PWAIT,SKIN_HICON_BUSY);
}
void
 TSonorkWin::SetStatus_None(HWND hwnd)
{
	TSonorkWin::SetStatus(hwnd,GLS_NULL,SKIN_HICON_NONE);
}

HWND
 TSonorkWin::CreateToolBar(HWND pHwnd
		,UINT id
		,UINT style
		,UINT btn_count
		,const TSonorkWinToolBarButton*sB
		,SIZE*	size)
{

	HWND hwnd;
	hwnd=CreateWindowEx(0
			, TOOLBARCLASSNAME
			, (LPSTR) NULL
			,	WS_CHILD
				| style
			,  0, 0 , 100, 26
			, pHwnd
			, (HMENU)(id)
			, SonorkApp_Instance()
			, NULL);

	// TB_BUTTONSTRUCTSIZE:
	//  Needed for backward compatibility
	::SendMessage(hwnd
		,TB_BUTTONSTRUCTSIZE
		,(WPARAM) sizeof(TBBUTTON), 0);
	::SendMessage(hwnd,TB_SETBITMAPSIZE
		,0,(LPARAM)MAKELONG(SKIN_ICON_SW,SKIN_ICON_SH) );
	::SendMessage(hwnd
		,TB_SETIMAGELIST
		,0
		,(LPARAM)sonork_skin.Icons());
	::SendMessage(hwnd
		, TB_SETEXTENDEDSTYLE
		, 0
		, TBSTYLE_EX_DRAWDDARROWS);
	if( btn_count>0 && sB!=NULL )
	{
		LoadToolBar(hwnd,btn_count,sB,size);
	}

	return hwnd;
}
void
 TSonorkWin::LoadToolBar(HWND hwnd,UINT btn_count,const TSonorkWinToolBarButton*sB,SIZE*size)
{
	TBBUTTON 	TB;
	RECT		rect;
	UINT		btn_no,index;
	LONG		style,ex_style;
//	int			i;
	char		tmp[64];


	btn_no=0;
	// Add dummy double-zero terminated string
	//SendMessage(hwnd, TB_ADDSTRING, 0, (LPARAM) &btn_no);
	for(index=0;btn_no < btn_count;btn_no++,sB++)
	{
		if(sB->cmd&SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR)
		{
			TB.iString	= 0;
			TB.idCommand= 0;
			TB.iBitmap	= 0;
			TB.fsState  = 0 ;
			TB.fsStyle  = TBSTYLE_SEP;
			::SendMessage(hwnd,TB_INSERTBUTTON,index++,(LPARAM)&TB);
		}
		TB.idCommand= (sB->cmd&0xffff);
		TB.iBitmap	= (sB->icon&0xffff);
		TB.fsState  = (BYTE)sB->state ;
		TB.fsStyle  = (BYTE)sB->style;
		if( sB->gls!=GLS_NULL )
		{
			SONORK_ZeroMem(tmp,sizeof(tmp));
			lstrcpyn( tmp , SonorkApp_LangString(sB->gls) , sizeof(tmp)-4);
			TB.iString = ::SendMessage(hwnd,TB_ADDSTRING,0,(LPARAM)tmp);
		}
		else
			TB.iString = -1;
		::SendMessage(hwnd,TB_INSERTBUTTON,index++,(LPARAM)&TB);
	}
	::SendMessage(hwnd,TB_AUTOSIZE,0,0);
	if( size != NULL )
	{
		size->cx	= 0;
		for(btn_no=0 ; btn_no < index ; btn_no++)
		{
			::SendMessage(hwnd,TB_GETITEMRECT,btn_no,(LPARAM)&rect);
			size->cx+=(rect.right - rect.left);
		}
		size->cy	= (rect.bottom - rect.top);
		rect.left 	= rect.top = 0;
		rect.right  = size->cx+1;
		rect.bottom = size->cy+1;
		style 		= ::GetWindowLong(hwnd,GWL_STYLE);
		ex_style	= ::GetWindowLong(hwnd,GWL_EXSTYLE);
		::AdjustWindowRectEx(&rect,style,false,ex_style);
		size->cy = rect.right;
		size->cy = rect.bottom;

	}

}


void
 TSonorkWin::CenterWin(TSonorkWin*win , RECT& rect , UINT flags)
{
	SIZE	rsz;
	SIZE	wsz;
	rsz.cx = rect.right - rect.left + 1;
	rsz.cy = rect.bottom- rect.top + 1;

	if(flags & SONORK_CENTER_WIN_F_CREATE)
		win->Create();
	wsz.cx = win->Width();
	wsz.cy = win->Height();
	if(wsz.cy>rsz.cy)wsz.cy=rsz.cy;
	if(wsz.cx>rsz.cx)wsz.cx=rsz.cx;

	win->MoveWindow( rect.left + (rsz.cx - wsz.cx)/2
					,rect.top  + (rsz.cy - wsz.cy)/2
					,wsz.cx
					,wsz.cy
					,flags & SONORK_CENTER_WIN_F_REDRAW?true:false);
}

void
 TSonorkWin::SetMenuText(HMENU hmenu, UINT id, GLS_INDEX gls)
{
	MENUITEMINFO  MI;
	MI.cbSize		= sizeof(MI);
	MI.fMask 		= MIIM_DATA|MIIM_TYPE;
	if( id&SONORK_WIN_CTF_OWNER_DRAW)
	{
		MI.fType 	= MFT_OWNERDRAW;
		MI.dwTypeData	= NULL;
		MI.dwItemData	= (gls&SONORK_WIN_CTRL_TEXT_ID)
				| (id&SONORK_WIN_CTRL_TEXT_FLAGS);
		SetMenuItemInfo(hmenu
			,(id&SONORK_WIN_CTRL_TEXT_ID)
			,(id&SONORK_WIN_CTF_BYPOSITION)?true:false
			,&MI);
	}
	else
		SetMenuText(hmenu, id, SonorkApp_LangString(gls));
}
void
 TSonorkWin::SetMenuText(HMENU hmenu, UINT id, SONORK_C_CSTR str)
{
	MENUITEMINFO  	MI;
	char		tmp[SONORK_WIN_MAX_MENU_LENGTH+4];

	assert( !(id&SONORK_WIN_CTF_OWNER_DRAW));

	MI.cbSize	= sizeof(MI);
	MI.fMask 	= MIIM_DATA|MIIM_TYPE;
	MI.dwItemData	= (id&SONORK_WIN_CTRL_TEXT_FLAGS);
	MI.fType 	= MFT_STRING;

	if( id&SONORK_WIN_CTF_ELLIPSIS )
	{
		wsprintf(tmp,"%-.92s..",str);
		MI.dwTypeData	= (char*)tmp;
	}
	else
	{
		MI.dwTypeData	= (char*)str;
	}
	SetMenuItemInfo(hmenu
		,(id&SONORK_WIN_CTRL_TEXT_ID)
		,(id&SONORK_WIN_CTF_BYPOSITION)?true:false
		,&MI);
}
void
 TSonorkWin::AppendMenu(HMENU hmenu
	, UINT insert_before_id
	, UINT id
	, GLS_INDEX gls)
{
	MENUITEMINFO  	MI;
	char		tmp[SONORK_WIN_MAX_MENU_LENGTH+4];

	MI.wID		= (id&SONORK_WIN_CTRL_TEXT_ID);
	MI.cbSize	= sizeof(MI);
	MI.fMask 	= MIIM_DATA|MIIM_TYPE|MIIM_ID;
	MI.fType 	= MFT_STRING;

	if( id&SONORK_WIN_CTF_OWNER_DRAW)
	{
		MI.fType 	= MFT_OWNERDRAW;
		MI.dwTypeData	= NULL;
		MI.dwItemData	= (gls&SONORK_WIN_CTRL_TEXT_ID)
				| (id&SONORK_WIN_CTRL_TEXT_FLAGS);
	}
	else
	{
		MI.dwItemData	= (id&SONORK_WIN_CTRL_TEXT_FLAGS);
		MI.dwTypeData   = (char*)SonorkApp_LangString(gls);
		if( id&SONORK_WIN_CTF_ELLIPSIS )
		{
			wsprintf(tmp,"%-.92s..",MI.dwTypeData);
			MI.dwTypeData   =tmp;
		}
	}
	if(insert_before_id==0)
	{
		insert_before_id= GetMenuItemCount(hmenu)|SONORK_WIN_CTF_BYPOSITION;
	}
	InsertMenuItem(hmenu
		, (insert_before_id&SONORK_WIN_CTRL_TEXT_ID)
		, (insert_before_id&SONORK_WIN_CTF_BYPOSITION)?true:false
		, &MI);
	/*
	AppendMenu(menu
		, MF_STRING
		, (id&SONORK_WIN_CTRL_TEXT_ID)
		, label);
	*/
}
void
 TSonorkWin::AppendMenuNoData(HMENU menu)
{
	char tmp[128];
	wsprintf(tmp,"(%s)",SonorkApp.SysString(GSS_NODATA));
	::AppendMenu(menu,MF_STRING|MF_GRAYED, 0, tmp );
}

void
 TSonorkWin::SetMenuText(HMENU pMenu,const TSonorkWinGlsEntry* E)
{
	UINT			id;
	while( (id=E->id) != 0 )
	{
		SetMenuText(pMenu,id,E->gls_index);
		E++;
	}
}
TSonorkWin*
 TSonorkWin::Handle_to_SonorkWin(HWND hwnd)
{
	return (TSonorkWin *)::GetWindowLong(hwnd,GWL_USERDATA);
}


// -----------------------------------
// Helper definitions

int
 ComboBox_AddString(HWND hwnd , SONORK_C_CSTR str)
{
	return (int)::SendMessage(hwnd,CB_ADDSTRING, 0, (LPARAM)str);
}

int
 ComboBox_GetString(HWND hwnd , int index , SONORK_C_STR str)
{
	return (int)::SendMessage(hwnd,CB_GETLBTEXT , index, (LPARAM)str);
}

int
 ComboBox_DelString(HWND hwnd , int index)
{
	return (int)::SendMessage(hwnd,CB_DELETESTRING, index, 0);
}
void
 ComboBox_Clear(HWND hwnd)
{
	::SendMessage(hwnd,CB_RESETCONTENT, 0, 0);
}

int
 ComboBox_SetCurSel(HWND hwnd, int index)
{
	return (int)::SendMessage(hwnd,CB_SETCURSEL, (WPARAM)index, 0);
}

int
 ComboBox_GetCurSel(HWND hwnd)
{
	return (int)::SendMessage(hwnd,CB_GETCURSEL, 0, 0);
}

int
 ComboBox_FindStringExact(HWND hwnd,int index,SONORK_C_CSTR str)
{
	return (int)::SendMessage(hwnd,CB_FINDSTRINGEXACT,index,(LPARAM)str);
}

int
 ComboBox_FindString(HWND hwnd,int index,SONORK_C_CSTR str)
{
	return (int)::SendMessage(hwnd,CB_FINDSTRING,index,(LPARAM)str);
}

int
 ComboBox_SetItemData(HWND hwnd,int index,DWORD data)
{
	return (int)::SendMessage(hwnd,CB_SETITEMDATA ,index, (LPARAM)data);
}
DWORD
 ComboBox_GetItemData(HWND hwnd,int index)
{
	return (DWORD)::SendMessage(hwnd,CB_GETITEMDATA ,index, 0);
}
int
 ComboBox_GetCount(HWND hwnd)
{
	return (int)::SendMessage(hwnd,CB_GETCOUNT ,0,0);
}



int 	ListBox_AddString(HWND hwnd , SONORK_C_CSTR str){
	return (int)::SendMessage(hwnd,LB_ADDSTRING, 0, (LPARAM)str);
}

int 	ListBox_GetString(HWND hwnd , int index , SONORK_C_STR str){
	return (int)::SendMessage(hwnd,LB_GETTEXT , index, (LPARAM)str);
}

int 	ListBox_DelString(HWND hwnd , int index){
	return (int)::SendMessage(hwnd,LB_DELETESTRING, index, 0);
}
void    ListBox_Clear(HWND hwnd){
	::SendMessage(hwnd,LB_RESETCONTENT, 0, 0);
}

int		ListBox_SetCurSel(HWND hwnd, int index){
	return (int)::SendMessage(hwnd,LB_SETCURSEL, (WPARAM)index, 0);
}

int		ListBox_GetCurSel(HWND hwnd){
	return (int)::SendMessage(hwnd,LB_GETCURSEL, 0, 0);
}
int		ListBox_FindStringExact(HWND hwnd,int index,SONORK_C_CSTR str){
	return (int)::SendMessage(hwnd,LB_FINDSTRINGEXACT,index,(LPARAM)str);
}

int		ListBox_FindString(HWND hwnd,int index,SONORK_C_CSTR str){
	return (int)::SendMessage(hwnd,LB_FINDSTRING,index,(LPARAM)str);
}

int		ListBox_SetItemData(HWND hwnd,int index,DWORD data){
	return (int)::SendMessage(hwnd,LB_SETITEMDATA ,index, (LPARAM)data);
}
DWORD	ListBox_GetItemData(HWND hwnd,int index){
	return (DWORD)::SendMessage(hwnd,LB_GETITEMDATA ,index, 0);
}
int		ListBox_GetCount(HWND hwnd){
	return (int)::SendMessage(hwnd,LB_GETCOUNT ,0,0);
}

DWORD	ToolBar_GetButtonState(HWND hwnd,UINT b)
{
	return ::SendMessage(hwnd,TB_GETSTATE,b,0);
}
BOOL	ToolBar_SetButtonState(HWND hwnd,UINT b,DWORD state)
{
	return (BOOL)::SendMessage(hwnd,TB_SETSTATE,b,MAKELONG(state,0));
}


