#if !defined(SRKDIALOGWIN_H)
#define SRKDIALOGWIN_H


#include "srkwin.h"



struct TSonorkChildDialogNotify
{
	int			dialog_id;
	union {
		TSonorkWinNotify*	N;
		DRAWITEMSTRUCT*		draw;
	};
};
class TSonorkChildDialogWin
:public TSonorkWin
{

	virtual bool	OnDrawItem(DRAWITEMSTRUCT*);
	virtual bool	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	virtual LRESULT	OnNotify(WPARAM,TSonorkWinNotify*);

public:
	TSonorkChildDialogWin(TSonorkWin*parent, UINT res_id, UINT flags);

};


#endif