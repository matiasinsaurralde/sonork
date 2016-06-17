#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srksnapshotwin.h"
#include "srkgrpmsgwin.h"
#include "srk_file_io.h"
#include "srk_zip.h"
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
#define POKE_SNAP_SHOT		SONORK_WIN_POKE_01
#define CROPPING		SONORK_WIN_F_USER_01
#define FOCUS_RECT_VISIBLE	SONORK_WIN_F_USER_02
#define STRETCH_MODE            SONORK_WIN_F_USER_03
#define CAPTURING		SONORK_WIN_F_USER_05
#define MIN_MEM_CX	8
#define MIN_MEM_CY	8
#define MIN_CROP_CX	4
#define MIN_CROP_CY	4
#define UPDATE_SHOULD_STRETCH_FLAG\
	ChangeWinUsrFlag(STRETCH_MODE,mem.sz.cx>view.sz.cx && mem.sz.cy>=view.sz.cy)
#define MIN_WIDTH	400
#define MIN_HEIGHT	200


HBITMAP
 Sonork_LoadBitmap(HDC tDC, const char*file_name)
{
	SONORK_FILE_HANDLE	file_handle;
	HBITMAP			loaded_bm=NULL;
	BITMAPFILEHEADER	bm_file_hdr;
	BITMAPINFOHEADER	bm_info_hdr;
	BITMAPINFO*		bm_info_ptr;
	BYTE*			bm_bits_ptr;
	DWORD			bm_color_sz;
	DWORD			bm_bits_sz;
	file_handle=SONORK_IO_OpenFile(
			file_name
			,SONORK_OPEN_FILE_METHOD_OPEN_EXISTING);
	if( file_handle == SONORK_INVALID_FILE_HANDLE)
		return NULL;

	if( SONORK_IO_ReadFile(file_handle
				, &bm_file_hdr
				, sizeof(bm_file_hdr)) != 0)
		goto exit_01_close_file;

	TRACE_DEBUG("*LOAD* %s",file_name);

	bm_color_sz = (bm_file_hdr.bfOffBits - sizeof(BITMAPFILEHEADER)-sizeof(BITMAPINFOHEADER));
	bm_bits_sz  = (bm_file_hdr.bfSize - bm_file_hdr.bfOffBits);

	TRACE_DEBUG("  FILE:HDR{%03u+%04u=%05u} DATA{%06u+%06u=%06u}"
		, sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)
		, bm_color_sz
		, sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+bm_color_sz
		, bm_file_hdr.bfOffBits
		, bm_bits_sz
		, bm_file_hdr.bfSize
		);

	if( SONORK_IO_ReadFile(file_handle
				, &bm_info_hdr
				, sizeof(bm_info_hdr)) != 0)
		goto exit_01_close_file;

	TRACE_DEBUG("  INFO:BT=%06u CP=%06u SZ=%06u"
		,bm_info_hdr.biBitCount
		,bm_info_hdr.biCompression
		,bm_info_hdr.biSizeImage);


	bm_info_ptr = SONORK_MEM_ALLOC(BITMAPINFO
				, sizeof(BITMAPINFOHEADER)
				  + bm_color_sz  );
	memcpy(&bm_info_ptr->bmiHeader
		,&bm_info_hdr
		,sizeof(bm_info_hdr));

	if( SONORK_IO_ReadFile(file_handle
				,  bm_info_ptr->bmiColors
				,  bm_color_sz ) != 0)
		goto exit_02_del_bm_info_ptr;
	bm_bits_ptr = SONORK_MEM_ALLOC(BYTE,bm_bits_sz);

	if( SONORK_IO_ReadFile(file_handle
				,  bm_bits_ptr
				,  bm_bits_sz ) != 0)
		goto exit_03_del_bm_bits_ptr;


	loaded_bm = CreateDIBitmap(tDC
				, &bm_info_hdr
				, CBM_INIT
				, bm_bits_ptr
				, bm_info_ptr
				, DIB_RGB_COLORS);

exit_03_del_bm_bits_ptr:
	SONORK_MEM_FREE(bm_bits_ptr);
exit_02_del_bm_info_ptr:
	SONORK_MEM_FREE(bm_info_ptr);
exit_01_close_file:
	SONORK_IO_CloseFile(file_handle);
	return loaded_bm;

}
//#define SAVE_BM_BITS	8
BOOL
 SaveAsBitmap(HDC sDC, const char*file_name, const RECT& rect)
{
	SONORK_FILE_HANDLE	file_handle;
	BOOL        		save_ok=false;
	BITMAPFILEHEADER	bm_file_hdr;
	BITMAPINFO  		bm_info,*bm_info_ptr;
	BYTE			*bm_bits_ptr;
	ULONG 			bm_info_size;
	ULONG			bm_bits_size;
	HDC			work_dc;
	HBITMAP			work_bm,temp_bm, original_work_dc_bm;
	SIZE			size;
	UINT			f_offset;

	size.cx=rect.right-rect.left;
	size.cy=rect.bottom-rect.top;


	work_bm=CreateCompatibleBitmap(sDC
		, size.cx
		, size.cy);
	if(!work_bm)
		return false;

	temp_bm=CreateCompatibleBitmap(sDC, 8 , 8 );

	if(!temp_bm)
		goto exit_label_01;

	work_dc=CreateCompatibleDC(sDC);
	if(!work_dc)
		goto exit_label_02_del_temp_bm;

	original_work_dc_bm=(HBITMAP)::SelectObject( work_dc , work_bm );
	::BitBlt(work_dc
		,0
		,0
		,size.cx
		,size.cy
		,sDC
		,rect.left
		,rect.top
		,SRCCOPY);
	::SelectObject( work_dc , temp_bm );

	memset(&bm_info.bmiHeader,0,sizeof(bm_info.bmiHeader));
	bm_info.bmiHeader.biSize = sizeof(bm_info.bmiHeader);
	if ((GetDIBits(work_dc
		, work_bm
		, 0, 0
		, (LPSTR)NULL
		, &bm_info
		, DIB_RGB_COLORS)) == 0)
	{
		goto exit_label_03_del_work_dc;
	}

	// Now that we know the size of the image, alloc enough memory to retrieve
	// the actual bits
	//
	if ((bm_bits_ptr = (PBYTE)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
		bm_info.bmiHeader.biSizeImage)) == NULL)
	{
		TRACE_DEBUG("  GetDIBBits() failed Err %u",GetLastError());
		goto exit_label_03_del_work_dc;
	}
	TRACE_DEBUG("*SAVE* %s",file_name);
	TRACE_DEBUG("0-INFO:BT=%06u CP=%06u SZ=%06u -> BT=%u"
			,bm_info.bmiHeader.biBitCount
			,bm_info.bmiHeader.biCompression
			,bm_info.bmiHeader.biSizeImage
			,0);//SAVE_BM_BITS);

	//bm_info.bmiHeader.biBitCount=SAVE_BM_BITS;
	bm_info.bmiHeader.biCompression=BI_RGB;
	switch(bm_info.bmiHeader.biBitCount)
	{

		case 24:
			bm_info_size = sizeof(BITMAPINFOHEADER);
			break;

		case 16:
		case 32:
			bm_info_size = sizeof(BITMAPINFOHEADER)
					+sizeof(DWORD)*3;
			break;

		default:

			bm_info_size = sizeof(BITMAPINFOHEADER)
					+(sizeof(RGBQUAD)
					*(1<<bm_info.bmiHeader.biBitCount));
			break;
	}

	if (bm_info_size != sizeof(BITMAPINFOHEADER))
	{
		bm_info_ptr = SONORK_MEM_ALLOC(BITMAPINFO,bm_info_size);
		memcpy(bm_info_ptr,&bm_info,sizeof(BITMAPINFOHEADER));
	}
	else
	{
		bm_info_ptr = &bm_info;
	}

	file_handle=SONORK_IO_OpenFile(
			file_name
			,SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS);

	if (file_handle == SONORK_INVALID_FILE_HANDLE)
	{
		goto exit_label_04_del_pointers;
	}
	
	bm_file_hdr.bfType = 0x4D42;	// 'BM'
	bm_file_hdr.bfSize =
			  sizeof(BITMAPFILEHEADER)
			+ bm_info_size
			+ bm_info_ptr->bmiHeader.biSizeImage;
	bm_file_hdr.bfReserved1 =
	bm_file_hdr.bfReserved2 = 0;

	bm_file_hdr.bfOffBits = sizeof(BITMAPFILEHEADER)
				+ bm_info_size;

	f_offset=0;
	TRACE_DEBUG("  FILE:HDR{%03u+%04u=%05u} DATA{%06u+%06u=%06u}"
		, sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)
		, bm_info_size-sizeof(BITMAPFILEHEADER)
		, bm_info_size+sizeof(BITMAPFILEHEADER)
		, bm_file_hdr.bfOffBits
		, bm_file_hdr.bfSize-bm_file_hdr.bfOffBits
		, bm_file_hdr.bfSize
		);

	if(SONORK_IO_WriteFile(file_handle
		, &bm_file_hdr
		, sizeof(BITMAPFILEHEADER)) != 0)
	{
		goto exit_label_05_close_file;
	}
	f_offset+=sizeof(BITMAPFILEHEADER);

	bm_bits_size = bm_info_ptr->bmiHeader.biSizeImage;
	if ((GetDIBits(work_dc, work_bm
		, 0
		, bm_info_ptr->bmiHeader.biHeight
		, (LPSTR)bm_bits_ptr
		, bm_info_ptr
		, DIB_RGB_COLORS))==0)
	{
		TRACE_DEBUG("  GetDIBBits() failed Err %u",GetLastError());
		goto exit_label_05_close_file;
	}
//	TRACE_DEBUG("  INFO@OFFSET=%05u",f_offset);
	bm_info_ptr->bmiHeader.biSizeImage=0;
	if(SONORK_IO_WriteFile(file_handle
		, bm_info_ptr
		, bm_info_size) == 0)
	{
		TRACE_DEBUG("  INFO:BT=%06u CP=%06u SZ=%06u"
			,bm_info_ptr->bmiHeader.biBitCount
			,bm_info_ptr->bmiHeader.biCompression
			,bm_info_ptr->bmiHeader.biSizeImage);
		f_offset+=bm_info_size;
//		TRACE_DEBUG("  BITS@OFFSET=%05u",f_offset);
		if(SONORK_IO_WriteFile(file_handle
			, bm_bits_ptr
			, bm_bits_size) == 0)
		{
			f_offset+=bm_bits_size;
			TRACE_DEBUG("  ENDS@OFFSET=%05u",f_offset);
			save_ok=true;
		}
	}


exit_label_05_close_file:
	SONORK_IO_CloseFile( file_handle );

exit_label_04_del_pointers:
	if( ((void*)(&bm_info)) != (void*)bm_info_ptr)
	{
		SONORK_MEM_FREE(bm_info_ptr);
	}
	GlobalFree(bm_bits_ptr);


exit_label_03_del_work_dc:
	::SelectObject( work_dc , original_work_dc_bm );
	::DeleteDC(work_dc);

exit_label_02_del_temp_bm:
	::DeleteObject(temp_bm);

exit_label_01:
	::DeleteObject(work_bm);

	return save_ok;
}

TSonorkSnapshotWin::TSonorkSnapshotWin()
	:TSonorkWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_SNAP
	,SONORK_WIN_SF_NO_WIN_PARENT)
{
//	SetEventMask(SONORK_APP_EM_USER_LIST_AWARE);
}
bool
 TSonorkSnapshotWin::OnCreate()
{
	static TSonorkWinGlsEntry gls_table[]=
	{	{IDC_SNAP_SHOT
		, GLS_OP_REFRESH	}
	,	{IDC_SNAP_SAVE
		,	GLS_OP_STORE	}
	,	{-1
		,	GLS_LB_SCRSHT	}
	,	{0
		,	GLS_NULL	}
	};
	char	tmp[128];
	LoadLangEntries( gls_table, true );
	sprintf(tmp
		,"%s+%s"
		,SonorkApp.LangString(GLS_OP_STORE)
		,SonorkApp.LangString(GLS_OP_SEND));
	SetCtrlText(IDC_SNAP_SAVE_SEND,tmp);

	hcursor=LoadCursor(NULL,IDC_CROSS);
	scr.dc = CreateDC("DISPLAY", (LPCSTR) NULL,
	    (LPCSTR) NULL, (CONST DEVMODE *) NULL);
	mem.dc = CreateCompatibleDC(scr.dc);

	// Retrieve the metrics for the bitmap associated with the
	// regular device context.

	bmp.bmBitsPixel = (BYTE) GetDeviceCaps(scr.dc, BITSPIXEL);
	bmp.bmPlanes	= (BYTE) GetDeviceCaps(scr.dc, PLANES);
	bmp.bmWidth	= GetDeviceCaps(scr.dc, HORZRES);
	bmp.bmHeight	= GetDeviceCaps(scr.dc, VERTRES);

	// The width must be byte-aligned.

	bmp.bmWidthBytes = ((bmp.bmWidth + 15) &~15)/8;

	// Create a bitmap for the compatible DC.

	mem.bm = CreateBitmap(bmp.bmWidth, bmp.bmHeight,
	    bmp.bmPlanes, bmp.bmBitsPixel, (CONST VOID *) NULL);

	// Select the bitmap for the compatible DC.

	SelectObject(mem.dc, mem.bm);

	viewCtrl.AssignCtrl(this,IDC_SNAP_BITMAP);
	view.dc=::GetDC(viewCtrl.Handle());

	GetCtrlSize(IDC_SNAP_SAVE_SEND,&button.size);
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_SNAP_SHOT,true,"SnapShot");
	Realign();
	mem.sz.cx=0;
	SendPoke(POKE_SNAP_SHOT,1);
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkSnapshotWin::OnBeforeDestroy()
{
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_SNAP_SHOT,false,"SnapShot");
}

// ----------------------------------------------------------------------------

bool
 TSonorkSnapshotWin::OnMinMaxInfo(MINMAXINFO*MMI)
{
	MMI->ptMinTrackSize.x=MIN_WIDTH;
	MMI->ptMinTrackSize.y=MIN_HEIGHT;
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkSnapshotWin::OnSize(UINT szt)
{
	if(szt==SIZE_RESTORED||szt==SIZE_MAXIMIZED)
	{
		Realign();
		::InvalidateRect(viewCtrl.Handle(),NULL,false);
	}

}
#define SPACING	2
#define BUTTON_SPACING	4
#define BUTTONS	4
void TSonorkSnapshotWin::Realign()
{
	RECT	rect;
	int 	button_y, button_w;
	HDWP defer_handle;
	static UINT button_id[BUTTONS]=
	{IDC_SNAP_SAVE_SEND,IDC_SNAP_SAVE,IDC_SNAP_SHOT,IDCANCEL};

	defer_handle = BeginDeferWindowPos( 2+BUTTONS );

	button_y=Height()-BUTTON_SPACING-button.size.cy;
	defer_handle=DeferWindowPos(defer_handle
		,viewCtrl.Handle()
		,NULL
		,SPACING
		,SPACING
		,Width()-SPACING
		,button_y - SPACING - BUTTON_SPACING
		,SWP_NOACTIVATE
		);

	// Use rect.right as our counter
	rect.left	= Width()-button.size.cx-SPACING;
	button_w	= button.size.cx;
	for(rect.right=0;rect.right<BUTTONS;rect.right++)
	{
		defer_handle=DeferWindowPos(defer_handle
			,GetDlgItem(button_id[rect.right])
			,NULL
			,rect.left
			,button_y
			,button_w
			,button.size.cy
			,SWP_NOZORDER|SWP_NOACTIVATE);
		if(rect.right==0)
			button_w-=(button_w/3);
		rect.left-=button_w+SPACING;
	}
	EndDeferWindowPos(defer_handle);

	::GetClientRect(viewCtrl.Handle(),&rect);
	view.sz.cx=rect.right;
	view.sz.cy=rect.bottom;
	UPDATE_SHOULD_STRETCH_FLAG;

}

void
 TSonorkSnapshotWin::OnDestroy()
{
	::ReleaseDC(viewCtrl.Handle(),view.dc);
	viewCtrl.ReleaseCtrl();
	DeleteDC(scr.dc);
	DeleteDC(mem.dc);
	DeleteObject(mem.bm);

}
void
 TSonorkSnapshotWin::ShowFocusRect( bool show )
{
	if(TestWinUsrFlag(FOCUS_RECT_VISIBLE))
	{
		DrawFocusRect(view.dc,&view.visible_focus_rect);
	}
	if( show )
	{
		view.visible_focus_rect=view.focus_rect;
		DrawFocusRect(view.dc,&view.visible_focus_rect);
		SetWinUsrFlag(FOCUS_RECT_VISIBLE);
	}
	else
	{
		ClearWinUsrFlag(FOCUS_RECT_VISIBLE);
	}
}
void
 TSonorkSnapshotWin::EndCrop(bool crop_to_focus_rect)
{
	SIZE	crop_sz;
	RECT	mapped_rect;
	HBRUSH	brush;
	double	map_dx,map_dy;

	if(!TestWinUsrFlag(CROPPING))return;
	ShowFocusRect( false );
	ReleaseCapture();
	ClearWinUsrFlag(CROPPING);
	SetCursor(LoadCursor(NULL,IDC_ARROW));
	// Test if we should crop and if
	// not at minimal size already
	if(!crop_to_focus_rect
	||mem.sz.cx<MIN_MEM_CX
	||mem.sz.cy<MIN_MEM_CY )
		return;

	crop_sz.cx=view.focus_rect.right-view.focus_rect.left;
	crop_sz.cy=view.focus_rect.bottom-view.focus_rect.top;

	// Don't crop if the mouse moved too little.
	if( crop_sz.cx<MIN_CROP_CX && crop_sz.cy<MIN_CROP_CY )
		return;

	if( TestWinUsrFlag(STRETCH_MODE) )
	{
		map_dx=(double)mem.sz.cx/(double)view.sz.cx;
		map_dy=(double)mem.sz.cy/(double)view.sz.cy;
		mapped_rect.left = (int)((double)view.focus_rect.left * map_dx);
		mapped_rect.right = (int)((double)view.focus_rect.right * map_dx);
		mapped_rect.top = (int)((double)view.focus_rect.top * map_dy);
		mapped_rect.bottom = (int)((double)view.focus_rect.bottom * map_dy);
	}
	else
	{
		mapped_rect=view.focus_rect;
	}
	if(mapped_rect.left>mem.sz.cx||mapped_rect.top>mem.sz.cy)
		return;
	if(mapped_rect.right>mem.sz.cx)
		mapped_rect.right=mem.sz.cx;
	if(mapped_rect.bottom>mem.sz.cy)
		mapped_rect.bottom=mem.sz.cy;

	crop_sz.cx=mapped_rect.right-mapped_rect.left;
	crop_sz.cy=mapped_rect.bottom-mapped_rect.top;
	// Test if the resultant size is not smaller than minimal size
	if(crop_sz.cx<MIN_MEM_CX ||crop_sz.cy<MIN_MEM_CY )
		return;
	ScrollDC(mem.dc
		, -mapped_rect.left
		, -mapped_rect.top
		, NULL
		, NULL
		, NULL
		, NULL);

	mem.sz=crop_sz;
	UPDATE_SHOULD_STRETCH_FLAG;
	view.focus_rect.left	=mem.sz.cx;
	view.focus_rect.right	=bmp.bmWidth-1;
	view.focus_rect.top 	=0;
	view.focus_rect.bottom 	=bmp.bmHeight-1;

	SetBkColor(mem.dc,GetSysColor(COLOR_3DFACE));
	brush = CreateHatchBrush(HS_DIAGCROSS,GetSysColor(COLOR_3DSHADOW));
	::FillRect(mem.dc,&view.focus_rect,brush);
	view.focus_rect.left	=0;
	view.focus_rect.top 	=mem.sz.cy;
	::FillRect(mem.dc,&view.focus_rect,brush);
	DeleteObject(brush);

	::InvalidateRect(viewCtrl.Handle(),NULL,false);


}
LRESULT
 TSonorkSnapshotWin::OnCtlWinMsg(TSonorkWinCtrl*CT,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	POINTS P;
	if(uMsg>=WM_MOUSEFIRST&&uMsg<=WM_MOUSELAST)
	{
		P=MAKEPOINTS(lParam);
		if(TestWinUsrFlag(CROPPING))
		{
			if(uMsg==WM_MOUSEMOVE)
			{
				view.focus_rect.right	= P.x;
				view.focus_rect.bottom	= P.y;
				ShowFocusRect(true);
			}
			else
			if(uMsg==WM_LBUTTONUP||uMsg==WM_RBUTTONDOWN)
			{
				EndCrop(uMsg==WM_LBUTTONUP);
			}
		}
		else
		if(uMsg==WM_LBUTTONDOWN)
		{
			if( mem.sz.cx>=MIN_MEM_CX && mem.sz.cy>=MIN_MEM_CY )
			{
				SetWinUsrFlag(CROPPING);
				SetCapture(viewCtrl.Handle());
				view.focus_rect.left	=
				view.focus_rect.right	= P.x;
				view.focus_rect.top	=
				view.focus_rect.bottom	= P.y;
				SetCursor(hcursor);
			}
		}
	}
	else
	if(uMsg==WM_CANCELMODE)
	{
		TRACE_DEBUG("CANCEL_MODE");
		EndCrop(false);
	}

	return CT->DefaultProcessing(uMsg,wParam,lParam);
}
void
 TSonorkSnapshotWin::OnTimer(UINT id)
{
	KillTimer(id);
	if(TestWinUsrFlag(CAPTURING))
		PostPoke(POKE_SNAP_SHOT,1);
}

LRESULT
 TSonorkSnapshotWin::OnPoke(SONORK_WIN_POKE poke,LPARAM lp)
{
	if(poke==POKE_SNAP_SHOT)
	{
		if( lp== 0)
		{
			// Leave some time so that
			// all windows are redrawn
			if(IsVisible())
			{
				TRACE_DEBUG("SNAP_HIDE");
				ShowWindow(SW_HIDE);
				Sleep(50);
				//SetAuxTimer(100);
//				SetWinUsrFlag(CAPTURING);
//				return 0;
			}
			TRACE_DEBUG("SNAP_NOT_HIDE");

		}
		TRACE_DEBUG("SNAP_EXEC");
		ClearWinUsrFlag(CAPTURING);
		mem.sz.cx=bmp.bmWidth;
		mem.sz.cy=bmp.bmHeight;
		UPDATE_SHOULD_STRETCH_FLAG;
		if(mem.sz.cx>=MIN_MEM_CX && mem.sz.cy>=MIN_MEM_CY)
		{
			BitBlt(mem.dc
				, 0
				, 0
				, mem.sz.cx
				, mem.sz.cy
				, scr.dc
				, 0
				, 0
				, SRCCOPY);
		}
		ShowWindow(SW_SHOW);
	}
	return 0;
}
bool
 TSonorkSnapshotWin::CmdSave(TSonorkShortString& path)
{
	RECT			rect;
	if(mem.sz.cx<MIN_MEM_CX || mem.sz.cy<MIN_MEM_CY )
		return false;
	if(!SonorkApp.GetSavePath(Handle()
		, path
		, "screenshot.bmp"
		, GLS_OP_STORE
		, "snapshot"
		, "bmp"
		,OFN_EXPLORER
		 | OFN_LONGNAMES
		 | OFN_NOCHANGEDIR
		 | OFN_NOREADONLYRETURN
		 | OFN_HIDEREADONLY
		 | OFN_PATHMUSTEXIST))
		return false;

	rect.left=rect.top=0;
	rect.right=mem.sz.cx;
	rect.bottom=mem.sz.cy;
	return SaveAsBitmap(mem.dc,path.CStr(),rect)!=0;
}

/*	TSonorkZipHandle	ZH;
		ZH=SonorkApp.ZipEngine()->OpenForDeflate(zipFile, false, 4096*4);
		if(!ZH)break;
		if( !SonorkApp.ZipEngine()->InitDeflateFile(
			  ZH
			, bmpFile
			, 9
			, "screenshot.bmp"))
			goto close_exit;
		while(SonorkApp.ZipEngine()->DoDeflateFile(ZH))
			if( ZH->OperationComplete() )
				break;

close_exit:
		SonorkApp.ZipEngine()->Close(ZH);
*/

bool
 TSonorkSnapshotWin::OnCommand(UINT id,HWND , UINT code)
{
	TSonorkGrpMsgWin*	GW;
	TSonorkShortString	path;
	if( code != BN_CLICKED )
		return false;
	switch(id)
	{
	case IDC_SNAP_SAVE:
	case IDC_SNAP_SAVE_SEND:
		if(CmdSave(path))
		{
			if( id == IDC_SNAP_SAVE_SEND )
			{
				GW=(TSonorkGrpMsgWin*)SonorkApp.RunSysDialog(SONORK_SYS_DIALOG_GRP_MSG);
				if(GW)
				{
					GW->AddFile(path.CStr());
					GW->SetCompress(true);
				}
			}
			Destroy(IDOK);
		}
		return true;

	case IDC_SNAP_SHOT:
		PostPoke(POKE_SNAP_SHOT,0);
		return true;
	}
	return false;

}
bool
 TSonorkSnapshotWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if(S->CtlID == IDC_SNAP_BITMAP )
	{
		if( mem.sz.cx>=MIN_MEM_CX && mem.sz.cy>=MIN_MEM_CY)
		{
			if(TestWinUsrFlag(STRETCH_MODE))
			{
			    StretchBlt(S->hDC
				, 0
				, 0
				, view.sz.cx
				, view.sz.cy
				, mem.dc
				, 0
				, 0
				, mem.sz.cx
				, mem.sz.cy
				, SRCCOPY);
			}
			else
			{
			    BitBlt(S->hDC
				, 0
				, 0
				, view.sz.cx
				, view.sz.cy
				, mem.dc
				, 0
				, 0
				, SRCCOPY);
			}
		}
		else
		{
			::FillRect(S->hDC
				,&S->rcItem
				,GetSysColorBrush(COLOR_3DFACE));

		}
	}
	return true;
}

