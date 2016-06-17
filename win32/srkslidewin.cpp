#include "srkwin32app.h"
#include "srkappstr.h"
#pragma hdrstop
#include "srkslidewin.h"


TSonorkSlideWin::TSonorkSlideWin()
	:TSonorkWin(NULL
		,SONORK_WIN_CLASS_NORMAL
		|SONORK_WIN_TYPE_NONE
		,SONORK_WIN_SF_NO_WIN_PARENT)
{
	event.type = SONORK_UI_EVENT_NONE;
}

bool
 TSonorkSlideWin::OnBeforeCreate(TSonorkWinCreateInfo*CI)
{
	CI->style=WS_BORDER|WS_POPUP	;
//	CI->style&=~(WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_VSCROLL|WS_HSCROLL);
	CI->ex_style=WS_EX_TOPMOST|WS_EX_TOOLWINDOW;
	return true;
}
#define SCREEN_MARGIN_CX	2
#define SCREEN_MARGIN_CY	2
#define ICON_WIDTH		(SKIN_ICON_SW)
#define ICON_HEIGHT		(SKIN_ICON_SH)
#define PADDING			3
#define MIN_TEXT_WIDTH		100
#define MAX_TEXT_WIDTH 		180
#define MIN_TEXT_HEIGHT		ICON_HEIGHT
#define MAX_TEXT_HEIGHT		88
#define LEFT_ICON_MARGIN	(PADDING)
#define TOP_ICON_MARGIN		(PADDING)

#define TEXT_LEFT_MARGIN	(PADDING)
#define TEXT_RIGHT_MARGIN       (PADDING+ICON_WIDTH/2)
#define TEXT_TOP_MARGIN		(PADDING)
#define TEXT_BOTTOM_MARGIN	(PADDING+3)

#define TEXT_TOP_OFFSET		(TEXT_TOP_MARGIN)
#define TEXT_LEFT_OFFSET	(LEFT_ICON_MARGIN+ICON_WIDTH+TEXT_LEFT_MARGIN)
#define DT_FLAGS   		(DT_LEFT|DT_NOPREFIX|DT_WORDBREAK|DT_EDITCONTROL|DT_END_ELLIPSIS)
void
 TSonorkSlideWin::OnPaint(HDC tDC, RECT& rect, BOOL)
{
	HBRUSH	brush;
	if( event.type == SONORK_UI_EVENT_NONE)
	{
		::FillRect(tDC,&rect,GetSysColorBrush(COLOR_INFOBK));
		return;
	}

	brush = CreateSolidBrush(event.bg_color);

	SaveDC(tDC);
	SelectObject(tDC , sonork_skin.Font(SKIN_FONT_BOLD));
	SetTextColor(tDC , event.fg_color);
	SetBkMode(tDC, TRANSPARENT);

	::FillRect(tDC,&rect,brush);
	::DrawText(tDC
		, event.text.CStr()
		, event.text.Length()
		,&text_rect
		, DT_FLAGS );
	sonork_skin.DrawIcon(tDC
			, event.icon
			, LEFT_ICON_MARGIN
			, TOP_ICON_MARGIN
			, ILD_NORMAL);
	RestoreDC(tDC,-1);

	DeleteObject(brush);
}
void
 TSonorkSlideWin::OnLButtonDown(UINT ,int ,int )
{
	if(!SonorkApp.IsControlKeyDown())
	switch( EventType() )
	{
		default:
			break;

		case SONORK_UI_EVENT_USER_CONNECT:
		case SONORK_UI_EVENT_USER_DISCONNECT:
		case SONORK_UI_EVENT_ADD_USER:
		case SONORK_UI_EVENT_USER_MSG:
			SonorkApp.OpenMsgWindow( SonorkApp.UserList().Get( event.data.gu_id ) , SONORK_MSG_WIN_OPEN_FOREGROUND);
			break;

		case SONORK_UI_EVENT_INCOMMING_EMAIL:
			SonorkApp.OpenMailReader( NULL );
			break;
			
		case SONORK_UI_EVENT_EVENT_COUNT:
			SonorkApp.OpenNextEvent( false );
			break;
	}
	ClearEvent();
}

void
 TSonorkSlideWin::ClearEvent()
{
	if(event.type!= SONORK_UI_EVENT_NONE)
		DoClearEvent();
}

// 1318 276
// 

void
 TSonorkSlideWin::SetEvent(SONORK_UI_EVENT_TYPE	type
			, SONORK_C_CSTR		str
			, SKIN_ICON		icon
			, COLORREF 		fg_color
			, COLORREF 		bg_color
			, DWORD	   		ttl)
 {
	HDC	tDC;
	POINT	win_pt;
	SIZE	win_sz;
//	TRACE_DEBUG("SLIDE:SetEvent(%u,%s)",type,str?str:"<NULL>");
	if( type == SONORK_UI_EVENT_NONE || str==NULL )
	{
		ClearEvent();
		return;
	}
	event.age  = 0;
	event.ttl  = ttl;
	event.type = type;

	event.text.Set(str);
	event.icon = icon;
	event.fg_color = fg_color;
	event.bg_color = bg_color;


	text_rect.top  =  text_rect.left =0;
	text_rect.right = MIN_TEXT_WIDTH;
	text_rect.bottom= MIN_TEXT_HEIGHT;

	tDC=GetDC();
	SaveDC(tDC);
	SelectObject(tDC , sonork_skin.Font(SKIN_FONT_BOLD));
	::DrawText(tDC
		, event.text.CStr()
		, event.text.Length()
		, &text_rect
		,  DT_FLAGS|DT_CALCRECT);
	RestoreDC(tDC,-1);
	ReleaseDC(tDC);

	if(text_rect.right > MAX_TEXT_WIDTH)
		text_rect.right = MAX_TEXT_WIDTH;

	if(text_rect.bottom > MAX_TEXT_HEIGHT)
		text_rect.bottom = MAX_TEXT_HEIGHT;



	win_sz.cx	= text_rect.right
			+ LEFT_ICON_MARGIN
			+ ICON_WIDTH
			+ TEXT_LEFT_MARGIN
			+ TEXT_RIGHT_MARGIN;

	win_sz.cy	= text_rect.bottom
			+ TEXT_TOP_MARGIN
			+ TEXT_BOTTOM_MARGIN;

	switch( SonorkApp.ProfileCtrlValue(SONORK_PCV_SLIDER_POS) )
	{
		case SONORK_SLIDER_WIN_BOTTOM_LEFT:
			win_pt.x= SCREEN_MARGIN_CX;
			win_pt.y=SonorkApp.DesktopLimit().y - win_sz.cy - SCREEN_MARGIN_CY;
		break;
		case SONORK_SLIDER_WIN_TOP_LEFT:
			win_pt.x=SCREEN_MARGIN_CX;
			win_pt.y=SCREEN_MARGIN_CY;
		break;

		case SONORK_SLIDER_WIN_TOP_RIGHT:
			win_pt.y=SCREEN_MARGIN_CY;
			goto place_right;

		case SONORK_SLIDER_WIN_BOTTOM_RIGHT:
		default:
			win_pt.y=SonorkApp.DesktopLimit().y - win_sz.cy - SCREEN_MARGIN_CY;
place_right:
			win_pt.x=SonorkApp.DesktopLimit().x - win_sz.cx - SCREEN_MARGIN_CX;
		break;
	}

	text_rect.left	 = TEXT_LEFT_OFFSET;
	text_rect.right += TEXT_LEFT_OFFSET;
	text_rect.top	 = TEXT_TOP_OFFSET;
	text_rect.bottom+= TEXT_TOP_OFFSET;

	MoveWindow(	win_pt.x
			,win_pt.y
			,win_sz.cx
			,win_sz.cy);

	if( IsVisible() )
	{
		InvalidateRect(NULL,false);
	}
	else
	{
		ShowWindow(SW_SHOWNOACTIVATE);
	}
}

void
 TSonorkSlideWin::TimeSlot(UINT msecs)
{
	if( event.type!= SONORK_UI_EVENT_NONE )
	{
		event.age+=msecs;
		if(event.age > event.ttl)
		{
			DoClearEvent();
		}
	}
}
void
 TSonorkSlideWin::DoClearEvent()
{
	event.type = SONORK_UI_EVENT_NONE;
	event.text.Clear();
	ShowWindow(SW_HIDE);
}

