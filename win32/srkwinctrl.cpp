#include "srkwin32app.h"
#pragma hdrstop
#include "srkwinctrl.h"

TSonorkWinCtrl::TSonorkWinCtrl()
	{
		v_hwnd	=NULL;
		v_parent=NULL;
		v_proc  =NULL;
	}
TSonorkWinCtrl::TSonorkWinCtrl(TSonorkWin*parent,UINT ctrl_id)
	{
		v_hwnd	=NULL;
		v_parent=NULL;
		v_proc  =NULL;
		AssignCtrl(parent,ctrl_id);
	}
TSonorkWinCtrl::~TSonorkWinCtrl()
	{
		ReleaseCtrl();
	}
void	TSonorkWinCtrl::ReleaseCtrl()
{
	if( v_hwnd != NULL )
	{
		SetWindowLong( v_hwnd , GWL_WNDPROC	, (LONG)v_proc);
		SetWindowLong( v_hwnd, GWL_USERDATA , (LONG)0);
		v_hwnd=NULL;
		v_id=0;
	}

}
bool TSonorkWinCtrl::AssignCtrl(TSonorkWin*parent, HWND c_hwnd, UINT control_id)
{
	assert( v_hwnd == NULL);
	v_hwnd 	 = c_hwnd;
	v_parent = parent;
	if(control_id == 0)
		control_id = GetWindowLong(v_hwnd,GWL_ID);
	v_id	 = control_id;
	v_proc = (WNDPROC)SetWindowLong( v_hwnd , GWL_WNDPROC	, (LONG)WinProc);
	SetWindowLong( v_hwnd, GWL_USERDATA , (LONG)this);
	return true;
}
bool TSonorkWinCtrl::AssignCtrl(TSonorkWin*parent, UINT control_id)
{
	assert( v_hwnd == NULL);
	v_hwnd = parent->GetDlgItem( control_id );
	if ( v_hwnd != NULL )
	{
		v_parent = parent;
		v_id	 = control_id;

		v_proc = (WNDPROC)SetWindowLong( v_hwnd , GWL_WNDPROC	, (LONG)WinProc);
		SetWindowLong( v_hwnd, GWL_USERDATA , (LONG)this);
		return true;
	}
	return false;
}

LRESULT CALLBACK TSonorkWinCtrl::WinProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	TSonorkWinCtrl*_this=(TSonorkWinCtrl*)::GetWindowLong(hwnd,GWL_USERDATA);
	if(_this != NULL)
	{
		if( _this->v_hwnd == hwnd )
			return _this->v_parent->OnCtlWinMsg(_this,uMsg,wParam,lParam);
		return CallWindowProc(_this->v_proc,hwnd,uMsg,wParam,lParam);
	}
	return	DefWindowProc( hwnd, uMsg, wParam, lParam );
}

LRESULT	TSonorkWinCtrl::DefaultProcessing(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return CallWindowProc(v_proc,v_hwnd,uMsg,wParam,lParam);
}

