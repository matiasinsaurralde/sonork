#include "srkwin32app.h"
#include "sonork.rh"
#pragma hdrstop
#include "srklistview.h"

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

void
 TSonorkListView::SetHandle(HWND phwnd
		, bool default_image_list
		, bool default_report)
{
	hwnd=phwnd;
	if( default_image_list )
		SetImageList(sonork_skin.Icons(),NULL);
	if( default_report )
	{
		SetStyle(
		  LVS_REPORT
		  |LVS_SHAREIMAGELISTS
		  |LVS_SINGLESEL
		  |LVS_SHOWSELALWAYS
		,LVS_TYPEMASK
		  |LVS_SHAREIMAGELISTS
		  |LVS_SINGLESEL
		  |LVS_SHOWSELALWAYS
		  |LVS_EDITLABELS
		  |LVS_NOSCROLL
		  |LVS_SORTASCENDING
		  |LVS_SORTDESCENDING

		,LVS_EX_FULLROWSELECT
		,LVS_EX_FULLROWSELECT
		 |LVS_EX_GRIDLINES
		 |LVS_EX_INFOTIP
		 |LVS_EX_HEADERDRAGDROP
		 |LVS_EX_MULTIWORKAREAS
		 |LVS_EX_FLATSB
		 |LVS_EX_SUBITEMIMAGES
		 |LVS_EX_CHECKBOXES
		 |LVS_EX_REGIONAL
		 |LVS_EX_ONECLICKACTIVATE);
	}
	UpdateSkin();
}
void
 TSonorkListView::SetStyle( DWORD st_flags
			, DWORD st_mask
			, DWORD ex_flags
			, DWORD ex_mask) const
{
	DWORD	aux;
	aux = ::GetWindowLong( hwnd,GWL_STYLE);
	aux&=~(st_mask);
	aux|= (st_flags&st_mask);
	::SetWindowLong( hwnd , GWL_STYLE, aux);

	SendMessage(
		  LVM_SETEXTENDEDLISTVIEWSTYLE
		, ex_mask
		, ex_flags
		);
}


void
 TSonorkListView::UpdateSkin() const
 {
	SendMessage(	  LVM_SETTEXTCOLOR
			, 0
			, sonork_skin.Color(SKIN_COLOR_MAIN,SKIN_CI_FG));

	SendMessage(	  LVM_SETTEXTBKCOLOR
			, 0
			, sonork_skin.Color(SKIN_COLOR_MAIN,SKIN_CI_BG));

	SendMessage(	  LVM_SETBKCOLOR
			, 0
			, sonork_skin.Color(SKIN_COLOR_MAIN,SKIN_CI_BG));

	SendMessage(	  WM_SETFONT
			, (WPARAM)sonork_skin.Font(SKIN_FONT_MAIN_TREE)
			, 0);
}
void
 TSonorkListView::SetImageList(HIMAGELIST himl_icons, HIMAGELIST himl_state) const
{
	ListView_SetImageList(hwnd,himl_icons,LVSIL_SMALL);
	ListView_SetImageList(hwnd,himl_icons,LVSIL_NORMAL);
	ListView_SetImageList(hwnd,himl_state,LVSIL_STATE);
}

LPARAM
 TSonorkListView::GetItem( int item ) const
{
	LV_ITEM	lv_item;
	lv_item.iItem	=item;
	lv_item.iSubItem=0;
	lv_item.mask 	= LVIF_PARAM;
	if( ListView_GetItem(hwnd,&lv_item) )
		return lv_item.lParam;
	return 0;
}
LPARAM
 TSonorkListView::HitTest( long	scr_x
			, long	scr_y
			, DWORD hit_flags
			, int*	item) const
{
	LPARAM		lParam;
	LVHITTESTINFO	info;
	info.pt.x = scr_x;
	info.pt.y = scr_y;
	::ScreenToClient(hwnd,&info.pt);

	if( ListView_HitTest(hwnd,&info) == -1 )
	{
		assert( info.iItem == -1 );
		lParam=0;
	}
	else
	{
		if(info.flags & hit_flags )
		{
			lParam = GetItem( info.iItem );
		}
		else
		{
			info.iItem=-1;
			lParam=0;
		}
	}
	if( item != NULL )
		*item=info.iItem;
	return lParam;
}

LPARAM
 TSonorkListView::GetSelectedItem(int*item) const
{
	int	index;
	LPARAM	lParam;
	index = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED);
	if( index != -1 )
		lParam = GetItem(index);
	else
		lParam=0;
	if( item != NULL )*item = index;
	return lParam;
}

void
 TSonorkListView::SelectItem(int item) const
{
	ListView_SetItemState(hwnd,item,LVNI_FOCUSED|LVNI_SELECTED,LVNI_FOCUSED|LVNI_SELECTED);
}

void
 TSonorkListView::SetDropTarget(int item, BOOL set) const
{
	ListView_SetItemState(hwnd
		,item
		,set?LVIS_DROPHILITED:0
		,LVIS_DROPHILITED );
}
void
 TSonorkListView::EnsureVisible(int item, BOOL partial_ok) const
{
	ListView_EnsureVisible(hwnd,item,partial_ok);
}



int
 TSonorkListView::FindItem( LPARAM lParam, int start) const
{
	LVFINDINFO	info;
	info.flags =LVFI_PARAM;
	info.lParam=lParam;
	return ListView_FindItem(hwnd , start, &info);
}

int
 TSonorkListView::GetCount() const
{
	return ListView_GetItemCount(hwnd);
}



int
 TSonorkListView::AddItem(SONORK_C_CSTR	text
			, SKIN_ICON	icon
			, LPARAM	lParam) const
{
	LV_ITEM	lv_item;
	lv_item.iItem	=ListView_GetItemCount(hwnd);
	lv_item.iSubItem=0;
	lv_item.mask 	= LVIF_TEXT| LVIF_IMAGE|LVIF_STATE|LVIF_PARAM;
	lv_item.iImage	=icon;
	lv_item.state	=0;
	lv_item.stateMask=0;
	lv_item.pszText	=(char*)text;
	lv_item.lParam  =lParam;
	return ListView_InsertItem(hwnd,&lv_item);
}
int
 TSonorkListView::AddItemState(SONORK_C_CSTR 	text
			, DWORD			state
			, DWORD			state_mask
			, LPARAM		lParam) const
{
	LV_ITEM	lv_item;
	lv_item.iItem	=ListView_GetItemCount(hwnd);
	lv_item.iSubItem=0;
	lv_item.mask 	= LVIF_TEXT|LVIF_STATE|LVIF_PARAM;
	lv_item.state	=state;
	lv_item.stateMask=state_mask;
	lv_item.pszText	=(char*)text;
	lv_item.lParam  =lParam;
	return ListView_InsertItem(hwnd,&lv_item);
}

BOOL
 TSonorkListView::SetItem( int		item
			, int		sub_item
			, SONORK_C_CSTR	text
			, SKIN_ICON	icon) const
{
	LV_ITEM	lv_item;
	lv_item.iItem	=item;
	lv_item.iSubItem=sub_item;
	lv_item.mask 	= LVIF_TEXT|LVIF_IMAGE|LVIF_STATE;
	lv_item.iImage	=icon;
	lv_item.state	=0;
	lv_item.stateMask=0;
	lv_item.pszText	=(char*)text;
	return ListView_SetItem(hwnd,&lv_item);
}

BOOL
 TSonorkListView::SetItemText(int	item
			, int		sub_item
			, SONORK_C_CSTR	text) const
{
	LV_ITEM	lv_item;
	lv_item.iItem	=item;
	lv_item.iSubItem=sub_item;
	lv_item.mask 	= LVIF_TEXT;
	lv_item.pszText	=(char*)text;
	return ListView_SetItem(hwnd,&lv_item);
}

BOOL
 TSonorkListView::SetItemState(int	item
			, DWORD		flags
			, DWORD		mask) const
{

	LVITEM	lv_item;
	lv_item.state	=flags;
	lv_item.stateMask=mask;
	return
	 SendMessage(LVM_SETITEMSTATE,(WPARAM)item,(LPARAM)&lv_item);
}

void
 TSonorkListView::AddColumns(int no
			, TSonorkListViewColumn*list
			, int start_pos) const
{
	for(int i=0;i<no;i++,list++,start_pos++)
		AddColumn( start_pos
			, list->text
			, list->width
			, LVCFMT_LEFT);
}
void
 TSonorkListView::AddColumn(int pos
			, SONORK_C_CSTR text
			, int width
			, DWORD fmt)  const
{
	LV_COLUMN	lv_col;
	lv_col.mask	= LVCF_TEXT | LVCF_WIDTH |LVCF_SUBITEM |LVCF_FMT;
	lv_col.fmt	= fmt;
	lv_col.iSubItem = 0;
	lv_col.cx	= width;
	lv_col.pszText	= (char*)text;
	ListView_InsertColumn(hwnd,pos,&lv_col);
}

void
 TSonorkListView::AddColumn(int 	pos
			, GLS_INDEX	text
			, int 		width
			, DWORD 	fmt) const
{
	AddColumn(pos,SonorkApp.LangString(text),width,fmt);
}


void
 TSonorkListView::DelItem(int index) const
{
	ListView_DeleteItem(hwnd,index);
}

void
 TSonorkListView::DelAllItems() const
{
	ListView_DeleteAllItems(hwnd);
}
