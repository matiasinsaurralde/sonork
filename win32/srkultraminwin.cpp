#include "srkwin32app.h"
#include "srkappstr.h"
#pragma hdrstop
#include "srkultraminwin.h"

#define RESIZING_WIN	SONORK_WIN_F_USER_01

TSonorkUltraMinWin::TSonorkUltraMinWin(TSonorkWin*parent)
:TSonorkWin(NULL
		,SONORK_WIN_CLASS_NORMAL|SONORK_WIN_TYPE_NONE
		,SONORK_WIN_SF_NO_WIN_PARENT)
{
	owner=parent;
}
bool
 TSonorkUltraMinWin::Show(int x, int y)
{
	MoveWindow(x,y,ULTRA_MIN_FULL_WIDTH,ULTRA_MIN_FULL_HEIGHT);
	fullRECT.left=fullRECT.top=textRECT.top	=0;
	textRECT.left	=SKIN_ICON_SW+1;
	textRECT.right	=SKIN_ICON_SW+1+ULTRA_MIN_TEXT_WIDTH;
	fullRECT.right  =ULTRA_MIN_FULL_WIDTH;
	fullRECT.bottom =textRECT.bottom=ULTRA_MIN_FULL_HEIGHT;

	return Create();
}
void
 TSonorkUltraMinWin::OnPaint(HDC tDC, RECT& , BOOL )
{
	TSonorkUltraMinPaint	P;
	HBRUSH			brush;
	SaveDC(tDC);

	SetBkMode(tDC,TRANSPARENT);
	SelectObject(tDC,sonork_skin.Font(SKIN_FONT_MAIN_TREE));

	owner->SendPoke(SONORK_WIN_POKE_ULTRA_MIN_PAINT,(LPARAM)&P);

	SetTextColor(tDC,P.fg_color);
	brush 	= CreateSolidBrush(P.bg_color);
	::FillRect(tDC,&fullRECT,brush);
	::DrawText(tDC
		,P.text
		,strlen(P.text)
		,&textRECT
		,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
	::FrameRect(tDC
		,&fullRECT
		,(HBRUSH)GetStockObject(BLACK_BRUSH));
	sonork_skin.DrawIcon(tDC
		,P.icon
		,1
		,((ULTRA_MIN_FULL_HEIGHT-SKIN_ICON_SH)>>1)
		);
	sonork_skin.DrawIcon(tDC
		,SKIN_ICON_CLOSE_DOWN
		,textRECT.right+1
		,(ULTRA_MIN_FULL_HEIGHT-SKIN_ICON_SH)>>1);
	RestoreDC(tDC,-1);
	DeleteObject(brush);
}

bool
 TSonorkUltraMinWin::OnBeforeCreate(TSonorkWinCreateInfo*CI)
{
	CI->style=WS_VISIBLE|WS_POPUP;
	CI->ex_style=WS_EX_TOOLWINDOW|WS_EX_TOPMOST;
	return true;
}
void
 TSonorkUltraMinWin::OnLButtonDblClk(UINT ,int ,int )
{
	Destroy();
	/*
	TSonorkUltraMinMouse	M;
	M.point.x=x;
	M.point.y=y;
	M.keys=keys;
	owner->SendPoke(SONORK_WIN_POKE_ULTRA_MIN_MOUSE,(LPARAM)&M);
	*/
}
void
 TSonorkUltraMinWin::OnLButtonDown(UINT ,int x,int y)
{
	//TSonorkUltraMinMouse	M;
	if( x > textRECT.right || x < textRECT.left )
	{
		Destroy();
		/*
		M.point.x=x;
		M.point.y=y;
		M.keys=keys;
		owner->SendPoke(SONORK_WIN_POKE_ULTRA_MIN_MOUSE,(LPARAM)&M);
		*/
	}
	else
	{
		SetCapture(Handle());
		SetWinUsrFlag(RESIZING_WIN);
		sonork_win_move_origin.x = x;
		sonork_win_move_origin.y = y;
	}
}
void
 TSonorkUltraMinWin::OnLButtonUp(UINT ,int ,int )
{
	if(TestWinUsrFlag(RESIZING_WIN))
	{
		ReleaseCapture();
		ClearWinUsrFlag(RESIZING_WIN);
	}
}
void
 TSonorkUltraMinWin::OnMouseMove(UINT ,int x,int y)
{
	POINT	pt;
	if(TestWinUsrFlag(RESIZING_WIN))
	{
		pt.x=x - sonork_win_move_origin.x;
		pt.y=y - sonork_win_move_origin.y;
		ClientToScreen(&pt);
		SetWindowPos(NULL
			,pt.x
			,pt.y
			,0,0
			,SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
	}
}
bool
 TSonorkUltraMinWin::OnCreate()
{
	ShowWindow(SW_SHOW);
	return true;
}
void
  TSonorkUltraMinWin::OnBeforeDestroy()
{
	if( !owner->MultiTestWinUsrFlags(SONORK_WIN_SF_DESTROYING|SONORK_WIN_SF_DESTROYED))
		owner->SendPoke(SONORK_WIN_POKE_ULTRA_MIN_DESTROY,(LPARAM)this);
}
