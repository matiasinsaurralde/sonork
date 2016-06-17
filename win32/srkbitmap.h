#if !defined(SRKBITMAP_H)
#define SRKBITMAP_H

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


class TSonorkBitmap
{
	HDC	dc;
	HBITMAP	bm,saved_dc_bm;
	SIZE	size;

	void
		DeleteBM();
public:
	TSonorkBitmap();
	~TSonorkBitmap();

	HDC
		DC() const
		{ return dc; }

	HBITMAP
		Bitmap() const
		{ return bm; }

	const SIZE&
		Size() const
		{ return size;}
	BOOL
		Loaded() const
		{ return bm!=NULL;}
	BOOL
		Initialized() const
		{ return dc!=NULL;}
		
	BOOL
		InitHwnd(HWND);

	BOOL
		InitDc(HDC);

	BOOL
		LoadFile( SONORK_C_CSTR pFileName , DWORD cx, DWORD cy);

	BOOL
		Copy(HDC sDC, const RECT& rect);

	BOOL
		Copy(const TSonorkBitmap&);

	BOOL
		Transfer(TSonorkBitmap&);
		
	BOOL
		CreateBlank(DWORD cx, DWORD cy, HBRUSH init_brush);

	BOOL
		Save( SONORK_C_CSTR pFileName );

	BOOL
		BitBlt(HDC tDC, int tx, int ty, UINT op=SRCCOPY);

};
#endif
