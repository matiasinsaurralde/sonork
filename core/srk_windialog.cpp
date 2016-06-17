#include <windows.h>
#pragma hdrstop
#include "srk_windialog.h"

int		TSonorkDialog::Execute(HINSTANCE hInstance, HWND parent,UINT p_res_id)
{
	int x;
	if(p_res_id != 0)
		res_id = p_res_id;
	is_modal=true;
	x =  ::DialogBoxParam(
					hInstance
			,		MAKEINTRESOURCE(res_id)
			,		parent
			,		(DLGPROC)_DlgProc
			,		(LPARAM)this);
	if( x==-1 )
	{
		char tmp[64];
		wsprintf(tmp,"Failed, Error=%d",GetLastError());
		::MessageBox(parent,tmp,"Create",MB_OK);
	}
	return x;
}
HWND	TSonorkDialog::Create(HINSTANCE hInstance, HWND parent, UINT p_res_id)
{
	if(p_res_id != 0)
		res_id = p_res_id;
	is_modal=false;
	_hwnd = CreateDialogParam(
					hInstance
				, 	MAKEINTRESOURCE(res_id)
				, 	parent
				,   (DLGPROC)_DlgProc
				,	(LPARAM)this);
	return _hwnd;
}
BOOL	TSonorkDialog::DlgProc(UINT uMsg, WPARAM ,	LPARAM )
		{
			if(uMsg == WM_INITDIALOG)
				return true;
			return false;
		}


BOOL CALLBACK TSonorkDialog::_DlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TSonorkDialog*_this;
	if( uMsg == WM_INITDIALOG )
	{
		_this = (TSonorkDialog*)lParam;
		_this->_hwnd = hwndDlg;
		SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)_this);
	}
	else
	{
		if((_this = (TSonorkDialog*)GetWindowLong(hwndDlg,GWL_USERDATA))==NULL)
			return 0;
	}
	return _this->DlgProc(uMsg,wParam,lParam);
}
HWND TSonorkDialog::GetDlgItem(UINT cid)
{
	return ::GetDlgItem(_hwnd,cid);
}

int	TSonorkDialog::MessageBox(const char*text,const char*caption,UINT type)
{
	return ::MessageBox(_hwnd,text,caption,type);
}

UINT TSonorkDialog::SetTimer(UINT tid, UINT msecs)
{
	return ::SetTimer(_hwnd,tid,msecs,NULL);
}

void TSonorkDialog::KillTimer(UINT tid)
{
	::KillTimer(_hwnd,tid);
}

void  TSonorkDialog::EnableDlgItem( UINT cid, BOOL v)
{
	EnableWindow(GetDlgItem(cid),v?true:false);
}
void TSonorkDialog::ShowDlgItem(UINT cid, DWORD s)
{
	::ShowWindow(GetDlgItem( cid ) , s);
}
void TSonorkDialog::SetDlgItemText(UINT cid,const char*str)
{
	::SetDlgItemText(_hwnd,cid,str);
}
char* TSonorkDialog::GetDlgItemText(UINT cid,char*str,int max_cc)
{
	::GetDlgItemText(_hwnd,cid,str,max_cc);
	return str;
}
UINT 	TSonorkDialog::GetDlgItemInt(UINT cid)
{
	return ::GetDlgItemInt(_hwnd,cid,NULL,false);
}
void	TSonorkDialog::SetDlgItemInt(UINT cid,UINT v)
{
	::SetDlgItemInt(_hwnd,cid,v,false);
}
void	TSonorkDialog::SetDlgItemChecked(UINT cid,BOOL v)
{
	HWND c_hwnd = GetDlgItem(cid);
	if(c_hwnd)::SendMessage(c_hwnd,BM_SETCHECK,v?BST_CHECKED:BST_UNCHECKED,0);
}
BOOL	TSonorkDialog::GetDlgItemChecked(UINT cid)
{
	HWND c_hwnd = GetDlgItem(cid);
	if(c_hwnd)return ::SendMessage(c_hwnd,BM_GETCHECK,0,0)&BST_CHECKED;
	return false;
}
void	TSonorkDialog::EndDialog(UINT id)
{
	if(is_modal)::EndDialog(_hwnd,id);
	else	::DestroyWindow(_hwnd);
}

void	TSonorkDialog::UpdateDlgItem(UINT cid)
{
	HWND c_hwnd = GetDlgItem(cid);
	if(c_hwnd)::UpdateWindow(c_hwnd);
}

