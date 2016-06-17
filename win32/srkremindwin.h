#if !defined(GUREMINDWIN_H)
#define GUREMINDWIN_H

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

#include "srkwinctrl.h"
#include "srk_task_atom.h"
#include "srktreeview.h"

#define REMIND_VIEW_ITEM_TIME	SONORK_TREE_ITEM_TYPE_01
#define REMIND_VIEW_ITEM_DATA	SONORK_TREE_ITEM_TYPE_02

struct TSonorkRemindData;

class TSonorkRemindWin
:public TSonorkWin
{
	TSonorkRemindData	*remind;
	HWND			hDatePicker;
	HWND			hAlarmPicker;
	HWND			hListView;
	HWND			hInput;

	bool OnCreate();
	void OnBeforeDestroy();
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	bool	OnCommand(UINT id,HWND hwnd, UINT code);
	LRESULT	OnNotify(WPARAM,TSonorkWinNotify*);
	LRESULT OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam);
	void	LoadLabels();
	void	LoadForm();
	void	SaveForm();

	void	UpdateRecurrentFields();


public:
	TSonorkRemindWin(TSonorkWin*,TSonorkRemindData*,bool new_item);

};
class TSonorkRemindAlarmWin
:public TSonorkWin
{
	HWND	minHwnd;
	bool 	OnCreate();
	void 	OnBeforeDestroy();
	bool	OnCommand(UINT id,HWND hwnd, UINT code);
	LRESULT OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam);
	void	LoadLabels();
	bool	OnDrawItem(DRAWITEMSTRUCT*);

public:
	TSonorkRemindDataHeader	header;
	TSonorkRemindAlarmWin();

};

struct TSonorkRemindViewItem;

class TSonorkRemindListWin
:public TSonorkWin
{
	struct
	{
		HWND	hwnd;
		int	height;
	}toolbar;

	struct unMONTHCALENDAR
	{

		TSonorkWinCtrl		ctrl;
		int			width;
	}mcal;

	struct unVIEW
	{
		TSonorkTreeView	tree;
		HTREEITEM	autoOpenHitem;
	}view;

	struct
	{
		HWND		hwnd;
		int		height;
	}status;

	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	bool	OnCommand(UINT id,HWND hwnd, UINT code);
	LRESULT	OnNotify(WPARAM,TSonorkWinNotify*);
	LRESULT	OnCtlWinMsg(TSonorkWinCtrl*,UINT,WPARAM,LPARAM);

	bool
		OnCreate();

	void
		OnAfterCreate();

	void
		OnBeforeDestroy();

	void
		OnSize(UINT);

	void
		RealignControls();

	void
		LoadLabels();

	void
		MakeFilePath(SONORK_C_STR);

	void
		DL_Load();

	void
		DL_Save();

	void
		_SaveItems(TSonorkAtomDb*,HTREEITEM pItem);

	void
		DL_Clear();

	void
		CmAddItem(const TSonorkTime*);
		
	void
		CmEditItem(TSonorkRemindViewItem*VI);

	TSonorkRemindViewItem*
		AddRemindItem(TSonorkRemindData*);

	void
		AddRemindItem(TSonorkRemindViewItem*);

	TSonorkRemindViewItem*
		AddTimeItem(const TSonorkTime&);

	void
		_AddViewItem(TSonorkRemindViewItem*,TSonorkRemindViewItem*item);

	TSonorkRemindViewItem*
		GetViewItem(HTREEITEM hItem);

	TSonorkRemindViewItem*
		GetDateItem(const TSonorkTime&);

	TSonorkRemindViewItem*
		GetRemindItem(HTREEITEM	,const TSonorkRemindDataHeader*HDR);

//	void    RefreshItemState(TSonorkRemindViewItem*);

//	void	UpdateAfterEditItem(TSonorkRemindViewItem*,int prevBoldCount);
public:
	TSonorkRemindListWin();


	bool
		MayCheckAlarms();

	bool
		AddItem(const TSonorkRemindData*);

	bool
		SetItemHeader(const TSonorkRemindDataHeader*, bool delete_it);

};


#endif
