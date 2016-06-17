#include "srkwin32app.h"
#pragma hdrstop

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

#include "srkbitmap.h"
#include "srk_file_io.h"

BOOL
 TSonorkBitmap::InitHwnd(HWND hwnd)
{

	BOOL rv;
	HDC  wDC;
	if( !dc )
	{
		wDC=::GetDC(hwnd);
		rv=InitDc(wDC);
		::ReleaseDC(hwnd,wDC);
		return rv;
	}
	return true;
}

BOOL
 TSonorkBitmap::InitDc(HDC wDC)
{
	if( wDC != NULL )
	{
		if(!dc)
		{
			if((dc=CreateCompatibleDC(wDC)) == NULL )
				return false;
			bm=saved_dc_bm=NULL;
		}
		return true;
	}
	return false;
}


void
  TSonorkBitmap::DeleteBM()
{
	if( bm != NULL && dc!=NULL )
	{
		if( saved_dc_bm )
			::SelectObject( dc, saved_dc_bm );
		::DeleteObject( bm );
		bm=NULL;
	}

}


BOOL
 TSonorkBitmap::BitBlt(HDC tDC, int tx, int ty, UINT op)
{
	RECT	rect;
	if( Loaded() )
	{
		return ::BitBlt(  tDC
				, tx, ty, size.cx,size.cy
				, dc , 0 , 0 , op);
	}
	else
	{
		rect.left=rect.top=0;
		rect.right=size.cx;
		rect.bottom=size.cy;
		return ::FillRect(  tDC
				, &rect
				, GetSysColorBrush(COLOR_3DFACE));
	}

}

BOOL
 TSonorkBitmap::Copy( const TSonorkBitmap& oBM)
{
	RECT rect;
	if( Initialized() && oBM.Loaded() )
	{
		rect.left=rect.top=0;
		rect.right=oBM.size.cx;
		rect.bottom=oBM.size.cy;
		return Copy(oBM.dc , rect );
	}
	return false;

}
BOOL
 TSonorkBitmap::Transfer( TSonorkBitmap& oBM)
{
	if( oBM.Loaded() )
	{
		if( dc )
		{
			DeleteBM();
			::DeleteDC( dc );
		}


		dc = oBM.dc;
		bm = oBM.bm;
		saved_dc_bm=oBM.saved_dc_bm;
		size = oBM.size;

		oBM.dc=NULL;
		oBM.saved_dc_bm=oBM.bm=NULL;
		return true;

	}
	return false;

}

BOOL
 TSonorkBitmap::Copy(HDC sDC, const RECT& rect)
{
	if( dc != NULL )
	{
		DeleteBM();
		if(CreateBlank(rect.right - rect.left
			, rect.bottom- rect.top
			, NULL ) )
		{
			return ::BitBlt( dc
				, 0 , 0
				, size.cx,size.cy
				, sDC
				, rect.left , rect.top
				, SRCCOPY);
		}

	}
	return false;
}

BOOL
 TSonorkBitmap::CreateBlank(DWORD cx, DWORD cy, HBRUSH init_brush)
{
	RECT	rect;
	if( dc != NULL )
	{
		DeleteBM();
		size.cx=cx;
		size.cy=cy;
		bm=CreateCompatibleBitmap( dc , size.cx, size.cy);
		if( bm != NULL )
		{
			saved_dc_bm=(HBITMAP)::SelectObject( dc, bm );
			if( init_brush )
			{
				rect.left=rect.top=0;
				rect.right=size.cx;
				rect.bottom=size.cy;
				::FillRect( dc
					, &rect
					, init_brush);

			}
			return true;
		}
	}
 	return false;
}

BOOL
 TSonorkBitmap::LoadFile( SONORK_C_CSTR pFileName , DWORD cx, DWORD cy)
{
	if( dc != NULL )
	{

		DeleteBM();

		size.cx=cx;
		size.cy=cy;
		bm = (HBITMAP)LoadImage(NULL
				,pFileName
				,IMAGE_BITMAP
				,cx
				,cy
				,LR_LOADFROMFILE);
		if( bm != NULL )
		{
			saved_dc_bm=(HBITMAP)::SelectObject( dc, bm );
			return true;
		}

	}
	return false;

}

BOOL
 TSonorkBitmap::Save( SONORK_C_CSTR pFileName )
{
	SONORK_FILE_HANDLE	file_handle;
	BOOL        		save_ok=false;
	BITMAPFILEHEADER	bm_file_hdr;
	BITMAPINFO  		bm_info,*bm_info_ptr;
	BYTE			*bm_bits_ptr;
	ULONG 			bm_info_size;
	ULONG			bm_bits_size;
	HBITMAP			temp_bm;

	if( !dc || !bm)
		return false;
		
	temp_bm=CreateCompatibleBitmap(dc, 8 , 8 );

	if(!temp_bm)
		return false;

	if(!::SelectObject( dc , temp_bm ))
		goto exit_label_01_del_temp_bm;

	memset(&bm_info.bmiHeader,0,sizeof(bm_info.bmiHeader));
	bm_info.bmiHeader.biSize = sizeof(bm_info.bmiHeader);
	if ((GetDIBits(dc
		, bm
		, 0, 0
		, (LPSTR)NULL
		, &bm_info
		, DIB_RGB_COLORS)) == 0)
	{
		goto exit_label_02_restore_dc;
	}

	// Now that we know the size of the image, alloc enough memory to retrieve
	// the actual bits
	//
	if ((bm_bits_ptr = (PBYTE)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
		bm_info.bmiHeader.biSizeImage)) == NULL)
	{
		TRACE_DEBUG("  GetDIBBits() failed Err %u",GetLastError());
		goto exit_label_02_restore_dc;
	}
	TRACE_DEBUG("*SAVE* %s",pFileName);
	TRACE_DEBUG("0-INFO:BT=%06u CP=%06u SZ=%06u"
			,bm_info.bmiHeader.biBitCount
			,bm_info.bmiHeader.biCompression
			,bm_info.bmiHeader.biSizeImage);

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
			 pFileName
			,SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS);

	if (file_handle == SONORK_INVALID_FILE_HANDLE)
	{
		goto exit_label_03_del_pointers;
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
		goto exit_label_04_close_file;
	}


	bm_bits_size = bm_info_ptr->bmiHeader.biSizeImage;
	if ((GetDIBits(dc
		, bm
		, 0
		, bm_info_ptr->bmiHeader.biHeight
		, (LPSTR)bm_bits_ptr
		, bm_info_ptr
		, DIB_RGB_COLORS))==0)
	{
		TRACE_DEBUG("  GetDIBBits() failed Err %u",GetLastError());
		goto exit_label_04_close_file;
	}

	bm_info_ptr->bmiHeader.biSizeImage=0;
	if(SONORK_IO_WriteFile(file_handle
		, bm_info_ptr
		, bm_info_size) == 0)
	{
		TRACE_DEBUG("  INFO:BT=%06u CP=%06u SZ=%06u"
			,bm_info_ptr->bmiHeader.biBitCount
			,bm_info_ptr->bmiHeader.biCompression
			,bm_info_ptr->bmiHeader.biSizeImage);
		if(SONORK_IO_WriteFile(file_handle
			, bm_bits_ptr
			, bm_bits_size) == 0)
		{
			save_ok=true;
		}
	}


exit_label_04_close_file:
	SONORK_IO_CloseFile( file_handle );

exit_label_03_del_pointers:
	if( ((void*)(&bm_info)) != (void*)bm_info_ptr)
	{
		SONORK_MEM_FREE(bm_info_ptr);
	}
	GlobalFree(bm_bits_ptr);


exit_label_02_restore_dc:
	::SelectObject( dc , bm );

exit_label_01_del_temp_bm:
	::DeleteObject(temp_bm);


	return save_ok;
}
TSonorkBitmap::TSonorkBitmap()
{
	dc=NULL;
}
TSonorkBitmap::~TSonorkBitmap()
{
	if( dc )
	{
		DeleteBM();
		::DeleteDC( dc );
	}
}
