#if !defined(SRKTREEVIEW_H)
#define SRKTREEVIEW_H


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

enum SONORK_TREE_ITEM_TYPE
{
// Main view items
  SONORK_TREE_ITEM_NONE
, SONORK_TREE_ITEM_GROUP
, SONORK_TREE_ITEM_EXT_USER
, SONORK_TREE_ITEM_AUTH_REQ
, SONORK_TREE_ITEM_APP
, SONORK_TREE_ITEM_TRACKER_GROUP
, SONORK_TREE_ITEM_TRACKER_ROOM
, SONORK_TREE_ITEM_TRACKER_ROOM_SUBS
, SONORK_TREE_ITEM_TYPE_01
, SONORK_TREE_ITEM_TYPE_02
, SONORK_TREE_ITEM_TYPE_03
, SONORK_TREE_ITEM_TYPE_04
};

#define SONORK_TREE_ITEM_VALUE_F_HIDE_COUNT	0x00100000
#define SONORK_TREE_ITEM_VALUE_F_COUNT		0xf00fffff


struct TSonorkTreeItem
{
friend struct TSonorkTreeView;
private:
	SONORK_TREE_ITEM_TYPE	type;
	DWORD			visual_state_flags;
public:
	TSonorkTreeItem	*	parent;
	HTREEITEM		hitem;

protected:

	TSonorkTreeItem(SONORK_TREE_ITEM_TYPE nt);

	virtual LRESULT
		CustomDraw_Prepaint(NMTVCUSTOMDRAW*)	const;

public:
	virtual ~TSonorkTreeItem(){};

	SONORK_TREE_ITEM_TYPE
		Type() const
		{ return type;}

	HTREEITEM
		hItem()	const
		{ return hitem;}

	TSonorkTreeItem	*
		Parent() const
		{ return parent;}

	virtual DWORD
		SearchId() const =0;

	virtual DWORD
		GetStateFlags() const
		{ return 0; }


	virtual SONORK_C_CSTR
		GetLabel(BOOL expanded,SKIN_ICON&) const =0;

	int
		GetBoldCount() const
		{ return GetBoldValue() & SONORK_TREE_ITEM_VALUE_F_COUNT; }

	int
		GetEventCount() const
		{ return GetEventValue() & SONORK_TREE_ITEM_VALUE_F_COUNT; }

	virtual int
		GetBoldValue() const
		{ return 0; }

	virtual	int
		GetEventValue()	const
		{ return 0; }

	virtual	void
		IncEventCount(int){}

	virtual void
		IncBoldCount(int){}

	virtual UINT
		GetSortIndex()	const
		{ return 0; }

	SKIN_ICON
		GetDispInfoInto(BOOL expanded,char*buffer,int buffer_size) const;

	static LRESULT
		OnCustomDraw(NMTVCUSTOMDRAW*);

	static LRESULT
		OnGetDispInfo(TV_DISPINFO*);

};

enum SONORK_TREE_VIEW_UPDATE_FLAGS
{
  SONORK_TREE_VIEW_UPDATE_F_FORCE_PAINT	= 0x001
, SONORK_TREE_VIEW_UPDATE_F_SORT	= 0x002
, SONORK_TREE_VIEW_UPDATE_F_SORT_ITEM	= 0x004
, SONORK_TREE_VIEW_UPDATE_F_NO_PAINT	= 0x008
, SONORK_TREE_VIEW_UPDATE_iF_AUTO_PAINT	= 0x100	// Internal flag
, SONORK_TREE_VIEW_UPDATE_iF_PROPAGATE	= 0x200 // Internal flag
};

struct TSonorkTreeView
{
private:
	HWND	hwnd;

	static int CALLBACK
		SortCallback(LPARAM P1,LPARAM P2,LPARAM);
	void
		MassRepaint(HTREEITEM) const;

	void
		DoRepaint( TSonorkTreeItem* VI
			, DWORD* new_state_flags
			, BOOL invalidate_rect) const;
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
		SetHandle(HWND phwnd);

	void
		SetStyle(DWORD flags, DWORD mask) const;

	void
		SetImageList(HIMAGELIST) const;

	void
		SetIndent(int v) const;

	void
		UpdateSkin() const;

	// <mass_op> : See Notes on AfterMassOp()
	HTREEITEM
		AddItem(TSonorkTreeItem* PI
			, TSonorkTreeItem* VI
			, BOOL mass_op) const;

	// <mass_op> : See Notes on AfterMassOp()
	void
		DelItem(TSonorkTreeItem* VI , BOOL mass_op) const;

	void
		DelItem( HTREEITEM hItem , BOOL mass_op) const;

	TSonorkTreeItem*
		GetItem( HTREEITEM ) const;

	HTREEITEM
		GetParent( HTREEITEM hItem ) const;

	HTREEITEM
		GetParent( TSonorkTreeItem* VI) const
		{ return GetParent(VI->hitem); }

	TSonorkTreeItem*
		GetChild( HTREEITEM ) const;

	TSonorkTreeItem*
		GetChild( TSonorkTreeItem* ) const;

	TSonorkTreeItem*
		GetNextSibling( TSonorkTreeItem* ) const;

	TSonorkTreeItem*
		GetNextSibling( HTREEITEM ) const;

	TSonorkTreeItem*
		GetSelectedItem() const;

	HTREEITEM
		GetSelection() const;

	void
		DelItemChildren(HTREEITEM parent, BOOL mass_op) const;

	void
		DelItemChildren(TSonorkTreeItem* VI, BOOL mass_op) const
		{ DelItemChildren(VI->hitem, mass_op); }

	void
		DelAllItems() const;

	TSonorkTreeItem*
		FindItem(SONORK_TREE_ITEM_TYPE
			, DWORD search_id
			, HTREEITEM hItem=TVI_ROOT) const;

	TSonorkTreeItem*
		GetFirstItemWithEvents(HTREEITEM pHitem) const;

	TSonorkTreeItem*
		HitTest(  long	scr_x
			, long	scr_y
			, DWORD hit_flags
			, long*	client_y=NULL) const;


	void
		Repaint(TSonorkTreeItem*) const;

	void
		SortChildren(HTREEITEM hitem, bool recursive) const;

	void
		SortChildren(TSonorkTreeItem*VI, bool recursive) const
		{ SortChildren(VI->hitem,recursive); }

	void
		UpdateItemAttributes(
			  TSonorkTreeItem*	view_item
			, int 			bold_delta
			, int 			event_delta
			, DWORD 		update_flags ) const;

	void
		FocusItem(HTREEITEM hItem) const;

	void
		FocusItem(TSonorkTreeItem* VI) const
		{ FocusItem(VI->hitem); }

	void
		SelectItem(HTREEITEM hItem) const;

	void
		SelectItem(TSonorkTreeItem* VI) const
		{ SelectItem(VI->hitem);}

	void
		EnsureVisible(HTREEITEM hitem) const;

	void
		EnsureVisible(TSonorkTreeItem* VI) const
		{ EnsureVisible(VI->hitem); }

	void
		ExpandItemLevels(HTREEITEM hitem, DWORD expand_flags, int sub_levels) const;

	void
		ExpandItemLevels(TSonorkTreeItem* VI, DWORD expand_flags, int sub_levels=0) const
		{ ExpandItemLevels(VI->hitem,expand_flags,sub_levels);}

	void
		SelectDropTarget( HTREEITEM hItem ) const
		{ TreeView_SelectDropTarget(hwnd,hItem); }
		
	void
		HideToolTip() const;

	// Call AfterMassOp() after adding/deleting items
	// with <mass_op> set to <true> becase when <mass_op> is true,
	// the tree does minimial sorting/redrawing.
	// <sort_root>:
	//   if true, root and below are sorted
	//   if false, items INSIDE root and below are sorted (but not root)

	void
		AfterMassOp(bool sort_root=true) const;

	LRESULT
		SendMessage(UINT msg, WPARAM wParam, LPARAM lParam) const
		{ return ::SendMessage(hwnd,msg,wParam,lParam); }

};

#endif



