#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkaboutwin.h"

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

#define SONORK_DM_LOGO_CX	170
#define SONORK_DM_LOGO_CY	78
#define LOGO_PADDING		2
#define TIMER_MSECS		100
#define SCROLL_STEP 		2
#define BG_COLOR		0x604040
#define FG_COLOR		0xf0f0f0
TSonorkAboutWin::TSonorkAboutWin(TSonorkWin*parent)
	:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_ABOUT
	,0)
{}

static const char *szAbout=
"\n\n\nSONORK\n"
"Work Group Collaboration System\n"
"http://www.sonork.com\n\n"
"The Sonork team is composed of\n\n"
"Miguel 'migs' Balsevich-Prieto\n"
"Sonork's Programmer\n"
"migs@sonork.com\n\n"
"Paloma 'palo' Tami\n"
"paloma@sonork.com\n\n"
"Emilio 'milo' Cerri Neto\n"
"emilio@sonork.com\n"
"Florianópolis, Brazil\n\n"
"Helmar 'harry' Rudolph\n"
"helmar@sonork.com\n"
"Cape Town, South Africa\n\n"
"Luis 'terere' Guerrero\n"
"Graphics to blame on him\n"
"luis@sonork.com\n"
"Asunción, Paraguay\n\n"
"Bob 'revenger' Johnson\n"
"bob@sonork.com\n"
"U.S.A.\n\n"
"Special thanks to\n\n"
"'NetSnooper' & 'Revenger'\n"
"(first users)\n\n"
"Revista Upgrade\n"
"www.upgrade.com.py\n\n"
"Planet Internet\n"
"www.pla.net.py\n"
"(hosting)\n\n"
"Nishad KA (Oracle scripts)\n"
"Gerard Niefergold & Mike Livingston\n"
"(French translation)\n"
"Fernando & Juan Balsevich (the Bros)\n"
"Oliver Schulze (Linux core port)\n"
"Inés Burró (whatafan!)\n"
"Daniel Mojoli (network consulting)\n"
"Ricardo Treithammer\n"
"Francisco Sánchez\n"
"Aníbal Acosta & Javier Omella (B&R)\n"
"Markus, Martin & Rikard\n"
"Olivier Mermod\n"
"Bob Paajanen\n"
"And a special, very special,\n"
"THANK YOU\n"
"to our favourite user:\n\n"
"%s!\n\n"
"it is great to have you on board!\n";

bool 	TSonorkAboutWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code == STN_DBLCLK && id == IDC_ABOUT_TEXT && (GetKeyState(VK_CONTROL) & 0x8000) )
	{
		SetCtrlText(IDC_ABOUT_TEXT,"ICQ was the start, Sonork is the summit");
	}
	return false;
}
bool 	TSonorkAboutWin::OnCreate()
{
	RECT 	rect;
	HBITMAP	tmp_bm;
	HDC		tmp_dc;
	TSonorkTempBuffer tmp(SONORK_MAX_PATH);
	img_hwnd=GetDlgItem(IDC_ABOUT_LOGO);
	int	len;
	char	*txt;

	bgBrush=CreateSolidBrush(BG_COLOR);
	SetWindowText( GLS_LB_HABOUT );
	len=wsprintf(tmp.CStr(),"Ver. %u.%02u.%02u"
			,SONORK_APP_VERSION_MAJOR
			,SONORK_APP_VERSION_MINOR
			,SONORK_APP_VERSION_BUILD);
	if( SONORK_APP_VERSION_PATCH!=0 )
		wsprintf(tmp.CStr()+len," SP%u",SONORK_APP_VERSION_PATCH);
	SetCtrlText(IDC_ABOUT_TEXT,tmp.CStr());

	::GetClientRect(img_hwnd,&rect);
	scr_size.cx = mem_size.cx = rect.right - rect.left;
	scr_size.cy = rect.bottom - rect.top;


	len = strlen(szAbout) + 128;
	txt = new char[len];
	len = sprintf(txt,szAbout
		,SonorkApp.IsProfileOpen()
			?SonorkApp.ProfileUser().alias.CStr()
			:"Guess who"
		);


	mem_dc = CreateCompatibleDC(NULL);

	SonorkApp.GetDirPath(tmp.CStr()
			,SONORK_APP_DIR_SKIN
			,"about.bmp");
	tmp_dc = CreateCompatibleDC(mem_dc);
	tmp_bm = (HBITMAP)LoadImage(NULL,tmp.CStr()
		,IMAGE_BITMAP
		,0
		,0
		,LR_LOADFROMFILE);
	logo_height_th = SONORK_DM_LOGO_CY + LOGO_PADDING*2;
	if(tmp_bm != NULL)
	{
		BITMAP	bmp;
		mem_bm = NULL;
		bmp.bmType = 0;
		if(GetObject(tmp_bm, sizeof(bmp), (LPSTR)&bmp))
		{
			bottom_margin   = scr_size.cy - SONORK_DM_LOGO_CY;
			mem_size.cy = DrawBitmapText( tmp_dc , txt, len, true);
			mem_bm = CreateBitmap(mem_size.cx
				,mem_size.cy
				,bmp.bmPlanes
				,bmp.bmBitsPixel
				,NULL);
			if(mem_bm != NULL )
			{
				::SelectObject( tmp_dc , tmp_bm );
				::SelectObject( mem_dc , mem_bm );
				DrawBitmapText( mem_dc , txt , len , false);
				::BitBlt(mem_dc
					,(mem_size.cx-SONORK_DM_LOGO_CX)/2
					,0
					,SONORK_DM_LOGO_CX
					,SONORK_DM_LOGO_CY
					,tmp_dc
					,0
					,0
					,SRCCOPY);
			}
		}
		else
			mem_bm = NULL;
	}
	else
	{
		mem_bm = NULL;
	}
	if( mem_bm == NULL )
	{
		logo_height_th=0;
		bottom_margin   = scr_size.cy;
		mem_size.cy = DrawBitmapText( tmp_dc ,txt, len, true);
		mem_bm = CreateCompatibleBitmap(mem_dc,mem_size.cx,mem_size.cy);
		::SelectObject( mem_dc , mem_bm );
		DrawBitmapText( mem_dc , txt , len , false);
	}
	::DeleteDC( tmp_dc );
	if(tmp_bm!=NULL)
		::DeleteObject( tmp_bm );


	delete[] txt;
	SetAuxTimer(TIMER_MSECS);
	scroll_y		  =0;
	scroll_rect.left  = 0;
	scroll_rect.right = scr_size.cx;
	scroll_rect.top	  = logo_height_th;
	scroll_rect.bottom= scr_size.cy;
	scr_dc = ::GetDC(img_hwnd);

	return true;
}
void	TSonorkAboutWin::DrawTextArea( HDC dc , UINT )
{
	::BitBlt(dc
		, 0
		, logo_height_th
		, scr_size.cx
		, scr_size.cy - logo_height_th
		, mem_dc
		, 0
		, logo_height_th + scroll_y
		, SRCCOPY);
}
void	TSonorkAboutWin::OnTimer(UINT id)
{
	if( id == SONORK_WIN_AUX_TIMER_ID )
	{
		scroll_y+=SCROLL_STEP;
		if(logo_height_th + scroll_y + bottom_margin > mem_size.cy )
			scroll_y=0;
		DrawTextArea(scr_dc, 0);
	}

}

int
 TSonorkAboutWin::DrawBitmapText(HDC dc, SONORK_C_CSTR str, int len, bool calc)
{
	int h;
	RECT rect;
	SaveDC(dc);
	::SelectObject(dc, sonork_skin.Font(SKIN_FONT_LARGE));
	if(!calc)
	{
		::SetBkMode(dc,TRANSPARENT);
		::SetTextColor(dc,FG_COLOR);
		rect.top 	= rect.left=0;
		rect.bottom	= mem_size.cy;
		rect.right	= mem_size.cx;
		::FillRect(dc,&rect,bgBrush);
	}
	rect.top	=
	rect.bottom	=logo_height_th;
	rect.left	=2;
	rect.right=scr_size.cx - 2;
	h=DrawText(dc
		,str
		,len
		,&rect
		,DT_CENTER
		|DT_EDITCONTROL
		|DT_NOPREFIX
		|DT_NOCLIP
		|DT_WORDBREAK
		|(calc?DT_CALCRECT:0));
	RestoreDC(dc,-1);
	return logo_height_th+h+bottom_margin;
}

void 	TSonorkAboutWin::OnDestroy()
{
	KillAuxTimer();
	::ReleaseDC(img_hwnd,scr_dc);
	::DeleteDC( mem_dc );
	::DeleteObject( mem_bm );
	DeleteObject(bgBrush);

}
bool	TSonorkAboutWin::OnDrawItem( DRAWITEMSTRUCT* S )
{
	if( S->CtlID == IDC_ABOUT_LOGO )
	{
		::FillRect(S->hDC,&S->rcItem,bgBrush);
		if(logo_height_th)
		{
			::BitBlt(S->hDC
				, 0
				, LOGO_PADDING
				, scr_size.cx
				, SONORK_DM_LOGO_CY
				, mem_dc
				, 0
				, 0
				, SRCCOPY);
		}
		DrawTextArea(S->hDC,0);
		return true;
	}
	return false;
}

