#if !defined(SONORK_WINDIALOG_H)
#define SONORK_WINDIALOG_H



class TSonorkDialog
{
private:
static BOOL CALLBACK
		_DlgProc(HWND hwndDlg, UINT uMsg , WPARAM wParam,	LPARAM lParam );

virtual BOOL	DlgProc(UINT uMsg , WPARAM wParam,	LPARAM lParam );

public:
	HWND	  	_hwnd;
	UINT		res_id;
	bool		is_modal;
	TSonorkDialog(UINT p_res_id=0){_hwnd=NULL;res_id=p_res_id;}
	HWND	Handle() 	const 	{ return _hwnd;}
	HWND	GetDlgItem(UINT cid);
	void	SetDlgItemText(UINT cid,const char*);
	char*	GetDlgItemText(UINT cid,char*,int);
	UINT 	GetDlgItemInt(UINT cid);
	void	SetDlgItemInt(UINT cid,UINT v);
	void	SetDlgItemChecked(UINT cid,BOOL v);
	BOOL	GetDlgItemChecked(UINT cid);
	BOOL	IsIconic()			{ return ::IsIconic(_hwnd);}
	BOOL	IsVisible()			{ return ::IsWindowVisible(_hwnd);}
	void	ShowWindow(UINT s)	{ ::ShowWindow(_hwnd,s);}
	void	SetForeground()		{ ::SetForegroundWindow(_hwnd); }
	void	UpdateWindow()      { ::UpdateWindow(_hwnd);}
	void	UpdateDlgItem(UINT);
	void	ShowDlgItem(UINT cid, DWORD s);

	void 	EnableDlgItem( UINT cid, BOOL v);
	int		MessageBox(const char*text,const char*caption,UINT type);
	UINT	SetTimer(UINT tid, UINT msecs);
	void	KillTimer(UINT tid);

	int		Execute(HINSTANCE hInstance, HWND parent, UINT id=0);
	HWND	Create(HINSTANCE hInstance, HWND parent, UINT id=0);
	void	EndDialog(UINT);
};

#endif
