#include "srkwin32app.h"
#include "sonork.rh"
#pragma hdrstop
#include "srkskin.h"

#define SKIN_SIGN_SX		0
#define SKIN_SIGN_SY		SKIN_ICON_LAST_ROW_SY
#define SKIN_SIGN_MASK_SY(y)	((y)+SKIN_SIGN_SH)
#define SKIN_SIGN_ROW_SW	(12 * SKIN_SIGN_SW)
#define SKIN_SIGN_ROWS		1

#define SKIN_MICO_SX		SKIN_ICON_LAST_ROW_SX
#define SKIN_MICO_SY		(SKIN_ICON_LAST_ROW_SY+SKIN_ICON_SH*2)
#define SKIN_MICO_MASK_SY(y)	((y)+SKIN_MICO_SH)

TSonorkSkin
	sonork_skin;
LRESULT
 TSonorkSkin::OnCtlColorDialog(WPARAM wParam) const
{
	::SetTextColor((HDC)wParam
		, Color(SKIN_COLOR_DIALOG, SKIN_CI_FG));
	::SetBkColor((HDC)wParam
		, Color(SKIN_COLOR_DIALOG, SKIN_CI_BG));
	return (LRESULT)Brush(SKIN_BRUSH_DIALOG);
}

LRESULT
 TSonorkSkin::OnCtlColorMsgView(WPARAM wParam) const
{
	::SetTextColor((HDC)wParam
		, Color(SKIN_COLOR_MSG_VIEW, SKIN_CI_FG));
	::SetBkColor((HDC)wParam
		, Color(SKIN_COLOR_MSG_VIEW, SKIN_CI_BG));
	return (LRESULT)Brush(SKIN_BRUSH_MSG_VIEW);

}
static void _LoadHicon(SKIN_HICON hic, SKIN_ICON sic);
static void _ClearHicons();


#define SONORK_COLOR_MSG_VIEW_FG		RGB(0x00,0x00,0x00)
#define SONORK_COLOR_MSG_VIEW_BG		RGB(0xff,0xff,0xfa)
#define SONORK_COLOR_MSG_VIEW_SP		RGB(0xE8,0xE8,0xE8)

#define SONORK_COLOR_MSG_VIEW_FOCUS_FG		RGB(0x22,0x00,0xA8)
#define SONORK_COLOR_MSG_VIEW_FOCUS_BG		RGB(0xBB,0xD0,0xE8)

#define SONORK_COLOR_MSG_VIEW_SELECT_FG		RGB(0x00,0x00,0x00)
#define SONORK_COLOR_MSG_VIEW_SELECT_BG		RGB(0xff,0xff,0x80)

#define SONORK_COLOR_MSG_I_OLD_FG		RGB(0x00,0x00,0x00)	//RGB(0x00,0x40,0x00)
#define SONORK_COLOR_MSG_I_OLD_BG		RGB(0xf6,0xff,0xf6)

#define SONORK_COLOR_MSG_I_NEW_FG		RGB(0x00,0x00,0x00)	//RGB(0x00,0x40,0x00)
#define SONORK_COLOR_MSG_I_NEW_BG		RGB(0xff,0xff,0xE0)

#define SONORK_COLOR_MSG_O_FG			RGB(0x00,0x00,0x00)		//RGB(0x00,0x00,0x40)
#define SONORK_COLOR_MSG_O_BG			RGB(0xf8,0xf8,0xff)

#define SONORK_COLOR_CLIP_FG			SONORK_COLOR_MSG_VIEW_FG
#define SONORK_COLOR_CLIP_BG			SONORK_COLOR_MSG_VIEW_BG
#define SONORK_COLOR_CLIP_SP			RGB(0xE0,0xE4,0xf0)


#define SONORK_COLOR_CHAT_FG			SONORK_COLOR_MSG_VIEW_FG
#define SONORK_COLOR_CHAT_BG			SONORK_COLOR_MSG_VIEW_BG
#define SONORK_COLOR_CHAT_SP			SONORK_COLOR_MSG_VIEW_SP
#define SONORK_COLOR_CHAT_SYS_FG 		RGB(0x00,0xa0,0x00)
#define SONORK_COLOR_CHAT_ACT_FG		RGB(0x00,0x00,0xA0)

#define SONORK_COLOR_HINT_FG			RGB(0xff,0xff,0xff)
#define SONORK_COLOR_HINT_BG			RGB(0xa0,0xa0,0xa0)

#define SONORK_COLOR_MAIN_FG			RGB(0x00,0x00,0x00)
#define SONORK_COLOR_MAIN_BG			RGB(0xe8,0xe8,0xe8)
#define SONORK_COLOR_MAIN_SP			RGB(0x00,0x00,0xff)
#define SONORK_COLOR_MAIN_ONLINE		SONORK_COLOR_MAIN_FG
#define SONORK_COLOR_MAIN_OFFLINE		RGB(0x80,0x80,0x80)
#define SONORK_COLOR_MAIN_EVENT			RGB(0xa0,0x00,0x00)

#define MAIN_TREE_FONT_SIZE			14
#define BIG_FONT_SIZE				16
#define CONSOLE_FONT_SIZE			15 // 15
#define NEWS_FONT_SIZE				14 //13
#define SMALL_FONT_SIZE				-11 //9
#define MAIN_B_FONT_SIZE			-11//9
#define MAIN_B_FONT_WEIGHT			FW_DEMIBOLD
TSonorkSkin::TSonorkSkin()
{
	initialized = false;
}
TSonorkSkin::~TSonorkSkin()
{
	Clear();
}
struct ColorScheme
{
	SKIN_COLOR	cl;
	COLORREF	bg,fg,sp;
};
static ColorScheme default_color[]=
{{SKIN_COLOR_MSG_VIEW		,SONORK_COLOR_MSG_VIEW_BG	,SONORK_COLOR_MSG_VIEW_FG	,SONORK_COLOR_MSG_VIEW_SP}
,{SKIN_COLOR_MSG_SELECT		,SONORK_COLOR_MSG_VIEW_SELECT_BG,SONORK_COLOR_MSG_VIEW_SELECT_FG,0}
,{SKIN_COLOR_MSG_FOCUS		,SONORK_COLOR_MSG_VIEW_FOCUS_BG	,SONORK_COLOR_MSG_VIEW_FOCUS_FG	,0}
,{SKIN_COLOR_MSG_IN_NEW		,SONORK_COLOR_MSG_I_NEW_BG	,SONORK_COLOR_MSG_I_NEW_FG	,0}
,{SKIN_COLOR_MSG_IN_OLD		,SONORK_COLOR_MSG_I_OLD_BG	,SONORK_COLOR_MSG_I_OLD_FG	,0}
,{SKIN_COLOR_MSG_OUT		,SONORK_COLOR_MSG_O_BG		,SONORK_COLOR_MSG_O_FG		,0}
,{SKIN_COLOR_HINT		,SONORK_COLOR_HINT_BG		,SONORK_COLOR_HINT_FG		,0}
,{SKIN_COLOR_CLIP		,SONORK_COLOR_CLIP_BG		,SONORK_COLOR_CLIP_FG		,SONORK_COLOR_CLIP_SP}
,{SKIN_COLOR_CHAT		,SONORK_COLOR_CHAT_BG		,SONORK_COLOR_CHAT_FG		,SONORK_COLOR_CHAT_SP}
,{SKIN_COLOR_CHAT_EXT		,SONORK_COLOR_CHAT_SYS_FG	,SONORK_COLOR_CHAT_ACT_FG,0}
,{SKIN_COLOR_MAIN		,SONORK_COLOR_MAIN_BG		,SONORK_COLOR_MAIN_FG		,SONORK_COLOR_MAIN_SP}
,{SKIN_COLOR_MAIN_EXT	        ,SONORK_COLOR_MAIN_OFFLINE	,SONORK_COLOR_MAIN_ONLINE	,SONORK_COLOR_MAIN_EVENT}
,{SKIN_COLOR_SYSCON		,SONORK_COLOR_MSG_VIEW_BG	,SONORK_COLOR_MSG_VIEW_FG	,SONORK_COLOR_MSG_VIEW_SP}
,{SKIN_COLOR_SYSCON_NEW		,SONORK_COLOR_MSG_I_NEW_BG	,SONORK_COLOR_MSG_I_NEW_FG	,0}
,{SKIN_COLOR_NULL,0,0}
};

static ColorScheme lo_res_color[]=
{{SKIN_COLOR_MSG_VIEW		,COLOR_WINDOW			,COLOR_WINDOWTEXT,COLOR_3DHIGHLIGHT}
,{SKIN_COLOR_MSG_SELECT		,COLOR_INFOBK			,COLOR_INFOTEXT,0}
,{SKIN_COLOR_MSG_FOCUS		,COLOR_HIGHLIGHT		,COLOR_HIGHLIGHTTEXT,0}
,{SKIN_COLOR_MSG_IN_NEW		,COLOR_3DFACE			,COLOR_BTNTEXT,0}
,{SKIN_COLOR_MSG_IN_OLD		,COLOR_WINDOW			,COLOR_WINDOWTEXT,0}
,{SKIN_COLOR_MSG_OUT		,COLOR_WINDOW			,COLOR_GRAYTEXT,0}
,{SKIN_COLOR_HINT		,COLOR_INFOBK			,COLOR_INFOTEXT	,0}
,{SKIN_COLOR_CLIP		,COLOR_WINDOW			,COLOR_WINDOWTEXT,COLOR_3DHIGHLIGHT}
,{SKIN_COLOR_CHAT		,COLOR_WINDOW			,COLOR_WINDOWTEXT,COLOR_3DHIGHLIGHT}
,{SKIN_COLOR_CHAT_EXT		,COLOR_GRAYTEXT			,COLOR_WINDOWTEXT,0}
,{SKIN_COLOR_MAIN		,COLOR_WINDOW			,COLOR_WINDOWTEXT,COLOR_3DHIGHLIGHT}
,{SKIN_COLOR_MAIN_EXT	        ,COLOR_GRAYTEXT			,COLOR_WINDOWTEXT,COLOR_WINDOWTEXT}
,{SKIN_COLOR_SYSCON		,COLOR_WINDOW			,COLOR_WINDOWTEXT,COLOR_3DHIGHLIGHT}
,{SKIN_COLOR_SYSCON_NEW		,COLOR_3DFACE			,COLOR_BTNTEXT,0}
,{SKIN_COLOR_NULL,0,0}
};

void
 TSonorkSkin::SetColor(SKIN_COLOR c , SKIN_COLOR_INDEX i, COLORREF cr)
{
	if(c<=0)return;//Can't change color of dialog
	color[c][i] = cr;
	CreateColorBrushes();
}

void
 TSonorkSkin::SetDefaultColors(BOOL lo_res, BOOL create_brushes)
{
	int i;
	color[SKIN_COLOR_DIALOG][SKIN_CI_FG] = GetSysColor(COLOR_BTNTEXT);
	color[SKIN_COLOR_DIALOG][SKIN_CI_BG] = GetSysColor(COLOR_BTNFACE );
	if(lo_res)
	{
		for(i=0;default_color[i].cl!=SKIN_COLOR_NULL;i++)
		{
			color[default_color[i].cl][SKIN_CI_BG]=GetSysColor(lo_res_color[i].bg);
			color[default_color[i].cl][SKIN_CI_FG]=GetSysColor(lo_res_color[i].fg);
			color[default_color[i].cl][SKIN_CI_SP]=GetSysColor(lo_res_color[i].sp);
		}
	}
	else
	{
		for(i=0;default_color[i].cl!=SKIN_COLOR_NULL;i++)
		{
			color[default_color[i].cl][SKIN_CI_FG]=default_color[i].fg;
			color[default_color[i].cl][SKIN_CI_BG]=default_color[i].bg;
			color[default_color[i].cl][SKIN_CI_SP]=default_color[i].sp;
		}
	}
	if( create_brushes )
		CreateColorBrushes();
}
void
 TSonorkSkin::CreateColorBrushes()
{
	DeleteColorBrushes();
	// Warning!: Brush #0 should not be deleted!
	brush[SKIN_BRUSH_DIALOG]=GetSysColorBrush(COLOR_BTNFACE);
	brush[SKIN_BRUSH_MSG_VIEW]=CreateSolidBrush( Color(SKIN_COLOR_MSG_VIEW,SKIN_CI_BG) );
	brush[SKIN_BRUSH_MAIN_VIEW]=CreateSolidBrush( Color(SKIN_COLOR_MAIN,SKIN_CI_BG) );
}
void
 TSonorkSkin::DeleteColorBrushes()
{
	if(brush[0]!=NULL)
	{
		// Warning!: Brush #0 should not be deleted!
		for(int i=1;i<SKIN_BRUSHES;i++)
			DeleteObject(brush[i]);
		brush[0]=NULL;
	}
}
bool
 TSonorkSkin::Initialize(HINSTANCE hInstance
	, const char* icons_file
	, const char* logo_file
	, BOOL intranet_mode)
{
	UINT base;

	Clear();
	initialized	= true;

	base = intranet_mode?IDI_TRAY_0_INTRA:IDI_TRAY_0;
	for(int i=0;i<3;i++)
	{
		hicon.tray[i] =
			(HICON)LoadImage(hInstance
				, MAKEINTRESOURCE(base+i)
				, IMAGE_ICON
				, 16
				, 16
				, LR_DEFAULTCOLOR|LR_SHARED	);
	}
	image_list.icon=ImageList_Create(SKIN_ICON_SW,SKIN_ICON_SH,ILC_COLORDDB|ILC_MASK,0,SKIN_ICONS);
	image_list.sign=ImageList_Create(SKIN_SIGN_SW,SKIN_SIGN_SH,ILC_COLORDDB|ILC_MASK,0,SKIN_SIGNS);
	image_list.mico=ImageList_Create(SKIN_MICO_SW,SKIN_MICO_SH,ILC_COLORDDB|ILC_MASK,0,SKIN_MICOS);
	image_list.dyn_icons=0;

	cursor[SKIN_CURSOR_HAND] 	= LoadCursor(hInstance,MAKEINTRESOURCE(IDCURSOR_HAND));
	cursor[SKIN_CURSOR_ARROW]	= LoadCursor(NULL,MAKEINTRESOURCE(IDC_ARROW));
	cursor[SKIN_CURSOR_SIZE_ALL]	= LoadCursor(NULL,MAKEINTRESOURCE(IDC_SIZEALL));
	cursor[SKIN_CURSOR_SIZE_NS]	= LoadCursor(NULL,MAKEINTRESOURCE(IDC_SIZENS));
	cursor[SKIN_CURSOR_SIZE_WE]	= LoadCursor(NULL,MAKEINTRESOURCE(IDC_SIZEWE));
	SONORK_ZeroMem(hicon.skin,sizeof(hicon.skin));

	font[SKIN_FONT_MAIN_TREE]
			= CreateFont(MAIN_TREE_FONT_SIZE
				,0,0,0
				,FW_NORMAL
				,0 // italic attribute flag
				,0	// underline attribute flag
				,0	// strikeout attribute flag
				,DEFAULT_CHARSET	// character set identifier
				,OUT_DEFAULT_PRECIS			// output precision
				,CLIP_DEFAULT_PRECIS	// clipping precision
				,DRAFT_QUALITY	//DEFAULT_QUALITY		// output quality
				,FF_DONTCARE|DEFAULT_PITCH	//pitch and family
				,"Arial"	//"MS Sans Serif"//"Arial" //
				);	// pointer to typeface name string

	font[SKIN_FONT_BOLD]
			= CreateFont(MAIN_B_FONT_SIZE
				,0,0,0
				,MAIN_B_FONT_WEIGHT
				,0 // italic attribute flag
				,0	// underline attribute flag
				,0	// strikeout attribute flag
				,DEFAULT_CHARSET	// character set identifier
				,OUT_DEFAULT_PRECIS			// output precision
				,CLIP_DEFAULT_PRECIS	// clipping precision
				,DRAFT_QUALITY	//DEFAULT_QUALITY		// output quality
				,FF_DONTCARE|DEFAULT_PITCH	//pitch and family
				,"Arial"	//"MS Sans Serif" //"MS Sans Serif"
				);	// pointer to typeface name string



	font[SKIN_FONT_CONSOLE]
			= CreateFont(CONSOLE_FONT_SIZE
				,0,0,0
				,FW_MEDIUM
				,0 // italic attribute flag
				,0	// underline attribute flag
				,0	// strikeout attribute flag
				,DEFAULT_CHARSET	// character set identifier
				,OUT_DEFAULT_PRECIS			// output precision
				,CLIP_DEFAULT_PRECIS	// clipping precision
				,DRAFT_QUALITY	//DEFAULT_QUALITY		// output quality
				,FF_DONTCARE|DEFAULT_PITCH	//pitch and family
				,"Arial"//"MS Sans Serif"
				);	// pointer to typeface name string

	font[SKIN_FONT_HINT]
			= CreateFont(NEWS_FONT_SIZE
				,0,0,0
				,FW_MEDIUM
				,0 // italic attribute flag
				,0	// underline attribute flag
				,0	// strikeout attribute flag
				,DEFAULT_CHARSET	// character set identifier
				,OUT_DEFAULT_PRECIS			// output precision
				,CLIP_DEFAULT_PRECIS	// clipping precision
				,DRAFT_QUALITY	//DEFAULT_QUALITY		// output quality
				,FF_DONTCARE|DEFAULT_PITCH	//pitch and family
				,"Arial"//"MS Sans Serif"
				);	// pointer to typeface name string

	font[SKIN_FONT_SMALL]
			= CreateFont(SMALL_FONT_SIZE
				,0,0,0
				,FW_NORMAL
				,0 // italic attribute flag
				,0	// underline attribute flag
				,0	// strikeout attribute flag
				,DEFAULT_CHARSET	// character set identifier
				,OUT_DEFAULT_PRECIS			// output precision
				,CLIP_DEFAULT_PRECIS	// clipping precision
				,DRAFT_QUALITY	//DEFAULT_QUALITY		// output quality
				,FF_DONTCARE|DEFAULT_PITCH	//pitch and family
				,"Arial"	//"MS Sans Serif"
				);	// pointer to typeface name string
	font[SKIN_FONT_LARGE]
			= CreateFont(BIG_FONT_SIZE
				,0,0,0
				,FW_NORMAL
				,0 // italic attribute flag
				,0	// underline attribute flag
				,0	// strikeout attribute flag
				,DEFAULT_CHARSET	// character set identifier
				,OUT_DEFAULT_PRECIS			// output precision
				,CLIP_DEFAULT_PRECIS	// clipping precision
				,DRAFT_QUALITY	//DEFAULT_QUALITY		// output quality
				,FF_DONTCARE|DEFAULT_PITCH	//pitch and family
				,"Arial" //"MS Sans Serif"
				);	// pointer to typeface name string

	SetDefaultColors(false,true);
	return LoadBitmaps(icons_file,logo_file);
}

void
 TSonorkSkin::DeleteFonts()
{
	if(font[0]!=NULL)
	{
		for(int i=0;i<SKIN_FONTS;i++)
			DeleteObject(font[i]);
		font[0]=NULL;
	}
}
void
 TSonorkSkin::Clear()
{
	if( initialized )
	{
		DeleteFonts();
		DeleteColorBrushes();
		DeletePredefinedHicons();
		ImageList_Destroy(image_list.icon);
		ImageList_Destroy(image_list.sign);
		image_list.icon=image_list.sign=NULL;
		if(logo.dc)
			DeleteDC(logo.dc);
		if(logo.bm)
			DeleteObject(logo.bm);
		if(logo.sm.mask)
			DeleteObject(logo.sm.mask);
	}
	logo.dc=NULL;
	logo.bm=NULL;
	logo.sm.mask=NULL;
	font[0]=NULL;

	initialized=false;
}


bool
 TSonorkSkin::LoadBitmaps(const char *icons_file, const char *logo_file)
{
	int		i;
	HBITMAP 	src_bm;
	HDC		src_dc;

	if( !initialized )
		return false;

	DeletePredefinedHicons();


	src_bm = (HBITMAP)LoadImage(NULL,icons_file,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
	if(src_bm == NULL)
		return false;

	src_dc	= CreateCompatibleDC(NULL);
	SelectObject(src_dc,src_bm);

	ImageList_RemoveAll(image_list.icon);
	ImageList_RemoveAll(image_list.sign);
	ImageList_RemoveAll(image_list.mico);

	logo.dc	= CreateCompatibleDC( src_dc );
	{
		HBITMAP prev_bm,icon_bm,mask_bm;
		int		icon_sy;

		icon_sy = 0;

		icon_bm=::CreateCompatibleBitmap(src_dc,SKIN_ICON_FULL_ROW_SW,SKIN_ICON_SH);
		mask_bm=::CreateCompatibleBitmap(src_dc,SKIN_ICON_FULL_ROW_SW,SKIN_ICON_SH);

		for(i=0;i<SKIN_ICON_FULL_ROWS;i++)
		{

			prev_bm = (HBITMAP)::SelectObject(logo.dc,icon_bm);

				::BitBlt(logo.dc
					,0,0
					,SKIN_ICON_FULL_ROW_SW
					,SKIN_ICON_SH
					,src_dc
					,SKIN_ICON_FULL_ROW_SX
					,icon_sy,SRCCOPY);

			::SelectObject(logo.dc,mask_bm);

				::BitBlt(logo.dc
					,0,0
					,SKIN_ICON_FULL_ROW_SW
					,SKIN_ICON_SH
					,src_dc
					,SKIN_ICON_FULL_ROW_SX
					,SKIN_ICON_MASK_SY(icon_sy)
					,SRCCOPY);

			::SelectObject(logo.dc,prev_bm);

			ImageList_Add(image_list.icon,icon_bm,mask_bm);

			icon_sy += SKIN_ICON_SH*2;
		}

		::DeleteObject(icon_bm);
		::DeleteObject(mask_bm);


		icon_bm=::CreateCompatibleBitmap(src_dc,SKIN_ICON_LAST_ROW_SW,SKIN_ICON_SH);
		mask_bm=::CreateCompatibleBitmap(src_dc,SKIN_ICON_LAST_ROW_SW,SKIN_ICON_SH);
		{
			prev_bm = (HBITMAP)::SelectObject(logo.dc,icon_bm);
				::BitBlt(logo.dc
					,0,0
					,SKIN_ICON_LAST_ROW_SW
					,SKIN_ICON_SH
					,src_dc
					,SKIN_ICON_LAST_ROW_SX
					,icon_sy,SRCCOPY);
			::SelectObject(logo.dc,mask_bm);
				::BitBlt(logo.dc
					,0,0
					,SKIN_ICON_LAST_ROW_SW
					,SKIN_ICON_SH
					,src_dc
					,SKIN_ICON_LAST_ROW_SX
					,SKIN_ICON_MASK_SY(icon_sy)
					,SRCCOPY);

			::SelectObject(logo.dc,prev_bm);

			ImageList_Add(image_list.icon,icon_bm,mask_bm);
		}
		::DeleteObject(icon_bm);
		::DeleteObject(mask_bm);
		icon_sy = SKIN_SIGN_SY;

		icon_bm=::CreateCompatibleBitmap(src_dc,SKIN_SIGN_ROW_SW,SKIN_SIGN_SH);
		mask_bm=::CreateCompatibleBitmap(src_dc,SKIN_SIGN_ROW_SW,SKIN_SIGN_SH);

		for(i=0;i<SKIN_SIGN_ROWS;i++)
		{
			prev_bm = (HBITMAP)::SelectObject(logo.dc,icon_bm);
				::BitBlt(logo.dc
					,0,0,SKIN_SIGN_ROW_SW,SKIN_SIGN_SH
					,src_dc
					,SKIN_SIGN_SX,icon_sy,SRCCOPY);
			::SelectObject(logo.dc,mask_bm);
				::BitBlt(logo.dc
					,0,0,SKIN_SIGN_ROW_SW,SKIN_SIGN_SH
					,src_dc
					,SKIN_SIGN_SX,SKIN_SIGN_MASK_SY(icon_sy),SRCCOPY);

			::SelectObject(logo.dc,prev_bm);
			ImageList_Add(image_list.sign,icon_bm,mask_bm);

			icon_sy += SKIN_SIGN_SH*2;
		}
		::DeleteObject(icon_bm);
		::DeleteObject(mask_bm);

		icon_bm=::CreateCompatibleBitmap(src_dc,SKIN_MICOS*SKIN_MICO_SW,SKIN_MICO_SH);
		mask_bm=::CreateCompatibleBitmap(src_dc,SKIN_MICOS*SKIN_MICO_SW,SKIN_MICO_SH);

		prev_bm = (HBITMAP)::SelectObject(logo.dc,icon_bm);
			::BitBlt(logo.dc
				,0,0,SKIN_MICOS*SKIN_MICO_SW,SKIN_MICO_SH
				,src_dc
				,SKIN_MICO_SX
				,SKIN_MICO_SY
				,SRCCOPY);
		::SelectObject(logo.dc,mask_bm);
			::BitBlt(logo.dc
				,0,0
				,SKIN_MICOS*SKIN_MICO_SW,SKIN_MICO_SH
				,src_dc
				,SKIN_MICO_SX
				,SKIN_MICO_MASK_SY(SKIN_MICO_SY)
				,SRCCOPY);

		::SelectObject(logo.dc,prev_bm);
		ImageList_Add(image_list.mico,icon_bm,mask_bm);

		::DeleteObject(icon_bm);
		::DeleteObject(mask_bm);
	}

	ImageList_SetBkColor(image_list.icon,CLR_NONE);
#define LHICON(name)	MakePredefinedHicon(SKIN_HICON_##name,SKIN_ICON_##name)
	LHICON(CONNECTED);
	LHICON(DISCONNECTED);
	LHICON(BUSY);
	LHICON(ERROR);
	LHICON(ALERT);
	LHICON(INFO);
	LHICON(CANCEL);
	LHICON(FILE_DOWNLOAD);
	LHICON(FILE_UPLOAD);
	LHICON(FILE_DELETE);

	LHICON(MODE_M_ONLINE);
	LHICON(MODE_M_BUSY);
	LHICON(MODE_M_AT_WORK);
	LHICON(MODE_M_FRIENDLY);
	LHICON(MODE_M_AWAY);

	LHICON(MODE_F_ONLINE);
	LHICON(MODE_F_BUSY);
	LHICON(MODE_F_AT_WORK);
	LHICON(MODE_F_FRIENDLY);
	LHICON(MODE_F_AWAY);

	LHICON(SEX_MALE);
	LHICON(SEX_FEMALE);
	LHICON(SONORK);
	LHICON(EVENT);
	LHICON(CHAT);
	LHICON(TRACKER);
	LHICON(NOTES);
	LHICON(TEMPLATE);
	LHICON(REPLY_TEMPLATE);
	LHICON(TIME);
	LHICON(EMAIL);

	
	logo.sm.mask=CreateBitmap(SKIN_SMALL_LOGO_SW
				,SKIN_SMALL_LOGO_SH
				,1
				,1
				,NULL);

	SelectObject(src_dc,logo.sm.mask);
	DeleteObject(src_bm);

	logo.bm = (HBITMAP)LoadImage(NULL
		,logo_file
		,IMAGE_BITMAP
		,0
		,0
		,LR_LOADFROMFILE);
	if(logo.bm == NULL)
	{
		DeleteDC(src_dc);
		return false;
	}
	SelectObject(logo.dc,logo.bm);
	logo.lg.l_bg = GetPixel( logo.dc
				, SKIN_LARGE_LOGO_SX
				, SKIN_LARGE_LOGO_SY );
	logo.lg.r_bg = GetPixel( logo.dc
				, SKIN_LARGE_LOGO_SX + SKIN_LARGE_LOGO_SW -1
				, SKIN_LARGE_LOGO_SY );
	BitBlt( src_dc
		, 0
		, 0
		, SKIN_SMALL_LOGO_SW
		, SKIN_SMALL_LOGO_SH
		, logo.dc
		, SKIN_SMALL_LOGO_SX
		, SKIN_SMALL_LOGO_SY+SKIN_SMALL_LOGO_SH
		, SRCCOPY);
	DeleteDC(src_dc);
	return true;
}

void
 TSonorkSkin::DrawLargeLogo(HDC tDC, const RECT* scrRECT, bool center)
{
	HBRUSH lBrush,rBrush;
	RECT	tmpRECT;
	UINT	X;
	if( logo.dc )
	{
		tmpRECT.top	=scrRECT->top;
		tmpRECT.bottom	=scrRECT->bottom;
		rBrush = CreateSolidBrush( logo.lg.r_bg );
		if( center )
		{
			X=(scrRECT->right-scrRECT->left-SKIN_LARGE_LOGO_SW)>>1;
			tmpRECT.left	= 0;
			tmpRECT.right 	= X;
			lBrush = CreateSolidBrush( logo.lg.l_bg );
			FillRect(tDC,&tmpRECT,lBrush);
			DeleteObject( lBrush );
			tmpRECT.left  = tmpRECT.right+SKIN_LARGE_LOGO_SW;
		}
		else
		{
			X=0;
			tmpRECT.left=SKIN_LARGE_LOGO_SW;
		}
		tmpRECT.right  = scrRECT->right;
		FillRect(tDC,&tmpRECT,rBrush);
		BitBlt( tDC
			, X
			, 0
			, SKIN_LARGE_LOGO_SW
			, SKIN_LARGE_LOGO_SH
			, logo.dc
			, SKIN_LARGE_LOGO_SX
			, SKIN_LARGE_LOGO_SY
			, SRCCOPY);
		DeleteObject( rBrush );

	}
	else
		FillRect(tDC
		,scrRECT
		,GetSysColorBrush(COLOR_3DFACE));

}

void
 TSonorkSkin::DrawSmallLogo(HDC tDC, const RECT* scrRECT, bool center)
{
	UINT	X;

	if( center )
	{
		X=(scrRECT->right-scrRECT->left-SKIN_SMALL_LOGO_SW)>>1;
	}
	else
	{
		X=0;
	}

	FillRect(tDC
		,scrRECT
		,GetSysColorBrush(COLOR_3DFACE));
	if( logo.dc )
	{
		MaskBlt( tDC
			, X
			, 1
			, SKIN_SMALL_LOGO_SW
			, SKIN_SMALL_LOGO_SH
			, logo.dc
			, SKIN_SMALL_LOGO_SX
			, SKIN_SMALL_LOGO_SY
			, logo.sm.mask
			, 0
			, 0
			, MAKEROP4(0x00AA0029,SRCCOPY));//0x00A000C9
	}

}

void
 TSonorkSkin::MakePredefinedHicon(SKIN_HICON hic, SKIN_ICON sic)
{
	hicon.skin[hic]=CreateHicon(sic);
}
void
 TSonorkSkin::DeletePredefinedHicons()
{
	int i;
	for(i=0;i<SKIN_HICONS;i++)
		if( hicon.skin[i] != NULL )
			::DestroyIcon(hicon.skin[i]);
	SONORK_ZeroMem(hicon.skin,sizeof(hicon.skin));
}
HICON
 TSonorkSkin::CreateHicon(SKIN_ICON icon)
{
	return ImageList_GetIcon(image_list.icon,icon,ILD_NORMAL);
}

