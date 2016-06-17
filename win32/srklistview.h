#if !defined(SRKLISTVIEW_H)
#define SRKLISTVIEW_H


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

struct TSonorkListViewColumn
{
	GLS_INDEX       text;
	int		width;
};



struct TSonorkListView
{
private:
	HWND	hwnd;

public:

	HWND
		Handle() const
		{ return hwnd; }

	void
		ShowWindow(UINT sw) const
		{ ::ShowWindow(hwnd,sw); }

	void
		InvalidateRect(RECT* rect,bool erase_bg) const
		{ ::InvalidateRect(hwnd,rect,erase_bg);}

	void
		SetFocus() const
		{ ::SetFocus(hwnd);}

	void
		SetHandle(HWND phwnd
			, bool default_image_list
			, bool default_report);

	void
		UpdateSkin() const;

	void
		SetStyle( DWORD st_flags
			, DWORD st_mask
			, DWORD ex_flags
			, DWORD ex_mask) const;

	void
		SetImageList(HIMAGELIST icons, HIMAGELIST state) const;

	void
		AddColumns(int count
			, TSonorkListViewColumn*
			, int start_pos=0) const;

	void
		AddColumn(int 		pos
			, SONORK_C_CSTR text
			, int 		width
			, DWORD 	fmt=LVCFMT_LEFT)  const;

	void
		AddColumn(int 		pos
			, GLS_INDEX	text
			, int 		width
			, DWORD 	fmt=LVCFMT_LEFT) const;

	int
		AddItem(SONORK_C_CSTR	text
			, SKIN_ICON	icon
			, LPARAM	lParam) const;
	int
		AddItemState(SONORK_C_CSTR 	text
			, DWORD			state
			, DWORD			state_mask
			, LPARAM		lParam) const;

	BOOL
		SetItem(  int		item
			, int		sub_item
			, SONORK_C_CSTR	text
			, SKIN_ICON	icon) const;
	BOOL
		SetItemText(int		item
			, int		sub_item
			, SONORK_C_CSTR	text) const;

	BOOL
		SetItemState(int	item
			, DWORD		flags
			, DWORD		mask) const;
	int
		GetCount() const;

	LPARAM
		GetItem( int ) const;

	int
		FindItem( LPARAM , int start=-1) const;

	void
		DelItem( int ) const;

	void
		DelAllItems() const;

	LPARAM
		GetSelectedItem(int*item=NULL) const;

	void
		SelectItem(int item) const;

	void
		SetDropTarget(int item, BOOL) const;


	LPARAM
		HitTest(  long	scr_x
			, long	scr_y
			, DWORD hit_flags
			, int*	item=NULL) const;



	void
		EnsureVisible(int item, BOOL partial_ok) const;


	LRESULT
		SendMessage(UINT msg, WPARAM wParam, LPARAM lParam) const
		{ return ::SendMessage(hwnd,msg,wParam,lParam); }

};

#endif


