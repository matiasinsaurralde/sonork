#if !defined(SRKUSEARCHWIN_H)
#define SRKUSEARCHWIN_H

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

#if !defined(SRKTREEVIEW_H)
# include "srktreeview.h"
#endif

#if !defined(SRKLISTVIEW_H)
# include "srklistview.h"
#endif

#if !defined(SRKTASKWIN_H)
# include "srktaskwin.h"
#endif

struct TSonorkTrackerTreeItem
:public TSonorkTreeItem
{

protected:
	TSonorkTrackerTreeItem(SONORK_TREE_ITEM_TYPE tp)
	:TSonorkTreeItem(tp){}
public:

	BOOL
		IsGroupOrRoom() const
		{ return Type() ==  SONORK_TREE_ITEM_TRACKER_GROUP
			|| IsRoom(); }
	BOOL
		IsRoom() const
		{ return Type() ==  SONORK_TREE_ITEM_TRACKER_ROOM
		  ||	 Type() ==  SONORK_TREE_ITEM_TRACKER_ROOM_SUBS; }


};

struct TSonorkTrackerRoomItem
:public TSonorkTrackerTreeItem
{
private:
	int	e_count,b_count;

	SONORK_C_CSTR
		GetLabel(BOOL expanded,SKIN_ICON&) const;

public:

	TSonorkTrackerRoom* 	room;

	TSonorkTrackerRoomItem(SONORK_TREE_ITEM_TYPE,TSonorkTrackerRoom*);
	~TSonorkTrackerRoomItem();

	DWORD
		SearchId() const
		{ return room->header.id.v[0];}

	DWORD	GetStateFlags() const
		{ return GetBoldValue()?TVIS_BOLD:0; }

	virtual int
		GetBoldValue()	const
		{return b_count;}

	virtual int
		GetEventValue()	const
		{return e_count;}

	void
		IncBoldCount(int v)
		{b_count+=v;}

	void
		IncEventCount(int v)
		{e_count+=v;}

};


struct TSonorkTrackerRoomSubsItem
:public TSonorkTrackerRoomItem
{

public:
	TSonorkTrackerData*	data;

	int
		GetBoldValue()	const;

	int
		GetEventValue()	const;

	TSonorkTrackerRoomSubsItem(TSonorkTrackerRoom* room);

	~TSonorkTrackerRoomSubsItem();

};


union TSonorkTrackerItemPtrs
{
	LPARAM				lParam;
	TSonorkTreeItem*		ptr;
	TSonorkTrackerTreeItem*		item;
	TSonorkTrackerRoomItem*		room;
	TSonorkTrackerRoomSubsItem*	subs;
};



class TSonorkUSearchWin
:public TSonorkTaskWin
{
	enum PAGE
	{
	  PAGE_USERS
	, PAGE_ROOMS
	};
	enum TASK
	{
		LOAD_ROOM_TREE
	,	LOAD_ROOM_LIST
	,	SEARCH_USERS
	,	ADD_USER
	};

private:

	TSonorkError		taskERR;
	TASK			current_task;
	struct
	{
		TSonorkTreeView	tree;
		TSonorkListView	list;
	}track;

	struct
	{
		TSonorkListView	list;
	}users;

	struct
	{
		HWND	hwnd;
		PAGE	page;
	}tab;

	HWND	status_bar;

	struct {
		HWND type;
		HWND text;
		HWND sex;

		HWND mode;

		HWND lang;

	}search;


	
	DWORD				addGroupNo;
	TSonorkShortString	addGroupName;


	bool	OnCreate();
	void	OnBeforeDestroy();
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);

	LRESULT
		OnNotify(WPARAM,TSonorkWinNotify*);

	
	void
		OnTaskStart(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&);

	void
		OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size);

	void
		OnTaskEnd(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&,const TSonorkError*);

	void
		SetTab( PAGE p, bool forced, bool update_tab);

	static void
		SaveTrackerCacheItem(TSonorkAtomDb&,const TSonorkTreeView&,HTREEITEM);

	static void SONORK_CALLBACK
		SearchUserCallback(
		 const TSonorkListView*		TV
		,TSonorkUserDataNotes*		UDN);

	void
		EnableDisableItems();

	void
		LoadTrackerTree(BOOL auto_load);
		
	void
		LoadTrackerList(const TSonorkTrackerRoom*);

	void
		SearchOnlineUsers();

	void
		ClearRoomItems();

	void
		SetSelectedUser(TSonorkUserDataNotes*);

	void
		SetSelectedMember(TSonorkTrackerMember*);

	void
		ClearSelectedUser();

	void
		OnCommand_Refresh();

	void
		OnCommand_User(UINT id, LPARAM);

	void
		DoDrag(const TSonorkUserData&);

	void
		UpdateAddGroupStatus(BOOL local_mode);

	void
		UpdateUsersFoundStatus(UINT n);

public:
	TSonorkUSearchWin(DWORD group_no, SONORK_C_CSTR group_name, BOOL request_auth);

	void
		SetAddGroup(DWORD group_no, SONORK_C_CSTR group_name, BOOL request_auth);


	static void
		SetupTrackerListView(const TSonorkListView& LV);
		
	static BOOL
		StartTrackerTreeTask(TSonorkTaskWin*
			, TSonorkError&
			, const TSonorkTreeView&
			, SONORK_TREE_ITEM_TYPE // Set to NONE to avoid using cache
			, BOOL& task_pending
			, SONORK_DWORD2*tag);

	static BOOL
		StartTrackerListTask(TSonorkTaskWin*
			, TSonorkError&
			, const TSonorkListView&
			, const TSonorkTrackerId&
			, BOOL& task_pending
			);

	static void
		ProcessTrackerTreeTaskData(const TSonorkTreeView&
			, SONORK_TREE_ITEM_TYPE
			, TSonorkDataPacket*P
			, UINT P_size);

	static void
		AfterTrackerTreeLoad(const TSonorkTreeView&
			, BOOL from_cache
			, BOOL remove_empty_groups);

	static void
		ProcessTrackerListTaskData(const TSonorkListView&
			, TSonorkDataPacket*P
			, UINT P_size);


	static void
		InvalidateTrackerListCache();

	static void
		SaveTrackerListToCache(const TSonorkTreeView&);


	static BOOL
		LoadTrackerListFromCache(const TSonorkTreeView&
			, SONORK_TREE_ITEM_TYPE	room_item_type);


};


#endif