#if !defined(SRKMAINWIN_H)
#define SRKMAINWIN_H

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
#if !defined(SRKWINCTRL_H)
# include "srkwinctrl.h"
#endif

#if !defined(SRKTASKWIN_H)
# include "srktaskwin.h"
#endif

#if !defined(SRKTREEVIEW_H)
# include "srktreeview.h"
#endif

#if !defined(SRK_DRAGDROP_H)
# include "srk_dragdrop.h"
#endif

#define MAIN_WIN_PINNED_WIDTH		180
#define MAIN_WIN_MIN_WIDTH		134             //164
#define MAIN_WIN_MIN_HEIGHT		280	//348		//400
#define MAIN_WIN_MAX_WIDTH		(MAIN_WIN_MIN_WIDTH*2)	//164*2;


#define SONORK_VIEW_ITEM_F_SELECTED	0x000001

enum SONORK_VIEW_TAB
{
  SONORK_VIEW_TAB_ALL			= -1
, SONORK_VIEW_TAB_FIRST			= 0
, SONORK_VIEW_TAB_1			= SONORK_VIEW_TAB_FIRST
, SONORK_VIEW_TAB_2
, SONORK_VIEW_TAB_LAST			= SONORK_VIEW_TAB_2
, SONORK_VIEW_TAB_COUNT			= (SONORK_VIEW_TAB_LAST-SONORK_VIEW_TAB_FIRST+1)
, SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS	= SONORK_VIEW_TAB_1
, SONORK_VIEW_TAB_TREE_TOOLS		= SONORK_VIEW_TAB_1
, SONORK_VIEW_TAB_TREE_AUTHS		= SONORK_VIEW_TAB_1
, SONORK_VIEW_TAB_TREE_NOT_AUTHORIZED_USERS	= SONORK_VIEW_TAB_1
, SONORK_VIEW_TAB_TREE_APPS		= SONORK_VIEW_TAB_2
, SONORK_VIEW_TAB_TREE_MTPL		= SONORK_VIEW_TAB_2
};

enum SONORK_VIEW_GROUP
{
  SONORK_VIEW_GROUP_NONE 		= -2
, SONORK_VIEW_GROUP_ROOT 		= -1

, SONORK_VIEW_SYS_GROUP_LOCAL_USERS	= 0x0000
, SONORK_VIEW_USER_GROUP_FIRST		= 0x0000
, SONORK_VIEW_USER_GROUP_LAST		= 0x00ff
, SONORK_VIEW_SYS_GROUP_AUTHS		= 0x0101
, SONORK_VIEW_SYS_GROUP_REMOTE_USERS	= 0x0200
, SONORK_VIEW_SYS_GROUP_EXT_APPS	= 0x0300
, SONORK_VIEW_SYS_GROUP_WEB_APPS	= 0x0400
, SONORK_VIEW_WAPP_GROUP_FIRST		= 0x0400
, SONORK_VIEW_WAPP_GROUP_LAST		= 0x04ff
};

// ----------------------------------------------------------------------------
// TSonorkMainViewItem
// Base class for all type of View items that appear in the main window

struct TSonorkMainViewItem
:public TSonorkTreeItem
{

private:

	LRESULT
		CustomDraw_Prepaint(NMTVCUSTOMDRAW*) const;

protected:

	TSonorkMainViewItem(SONORK_TREE_ITEM_TYPE tp)
	:TSonorkTreeItem(tp)
	{v_flags= 0;}


public:
	DWORD	v_flags;

	virtual ~TSonorkMainViewItem()
	{}
	
	virtual SONORK_VIEW_TAB
		GetTree() const =0;

	virtual SONORK_VIEW_GROUP
		GetDispGroup() const =0;
};

struct TSonorkAppViewItem
:public TSonorkMainViewItem
{
private:

	SONORK_APP_TYPE		app_type;
	DWORD			app_menu_cmd;

protected:

	TSonorkAppViewItem(SONORK_APP_TYPE type,DWORD menu_cmd);

public:
	~TSonorkAppViewItem(){}

	SONORK_VIEW_TAB
		GetTree() const
		{ return SONORK_VIEW_TAB_TREE_APPS;}

	SONORK_APP_TYPE
		AppType() const
		{ return app_type;}

	DWORD
		AppMenuCmd() const
		{ return app_menu_cmd;}

	DWORD
		SearchId() const
		{ return app_menu_cmd;}

	virtual SONORK_C_CSTR
		AppName() const =0;

	virtual DWORD
		AppFlags() const =0;

	virtual BOOL
		MayExecute() const =0;

	DWORD
		GetStateFlags() const
		{ return 0;}
};

struct TSonorkIntAppViewItem
:public TSonorkAppViewItem
{
private:
	GLS_INDEX		app_name;
public:
	TSonorkIntAppViewItem(DWORD cmd , GLS_INDEX name );

	SONORK_VIEW_GROUP
		GetDispGroup() 	const
		{ return SONORK_VIEW_SYS_GROUP_EXT_APPS;}
		
	SONORK_C_CSTR
		GetLabel(BOOL expanded,SKIN_ICON&)	const ;

	SONORK_C_CSTR
		AppName()		const ;

	DWORD
		AppFlags()		const ;

	BOOL
		MayExecute()		const;
};

struct TSonorkExtAppViewItem
:public TSonorkAppViewItem
{
private:
	TSonorkShortString	app_name;
	DWORD			app_flags;
	SKIN_ICON		app_icon;
public:
	TSonorkExtAppViewItem(DWORD cmd,SONORK_C_CSTR pname, DWORD pflags , SKIN_ICON picon);

	SONORK_VIEW_GROUP
		GetDispGroup() const
		{ return SONORK_VIEW_SYS_GROUP_EXT_APPS;}
		
	SONORK_C_CSTR
		GetLabel(BOOL expanded,SKIN_ICON&)	const ;

	SONORK_C_CSTR
		AppName() const
		{ return app_name.CStr();}

	DWORD
		AppFlags() const
		{ return app_flags;}

	BOOL
		MayExecute() const;
};

struct TSonorkWebAppViewItem
:public TSonorkAppViewItem
{
	TSonorkWappData		wapp_data;
public:
	TSonorkWebAppViewItem(DWORD cmd,const TSonorkWappData*WD);

	SONORK_VIEW_GROUP
		GetDispGroup() const;

	SONORK_C_CSTR
		GetLabel(BOOL expanded,SKIN_ICON&) const ;

	SONORK_C_CSTR
		AppName() const
		{ return wapp_data.AppName();}

	DWORD
		AppFlags() const
		{ return wapp_data.AppFlags();}

	BOOL
		MayExecute() const
		{ return true; }

	const TSonorkWappData*
		WappData() const
		{ return &wapp_data;}

	SONORK_WAPP_TYPE
		WappType() const
		{ return wapp_data.AppType();	}

	DWORD
		WappFlags() const
		{ return wapp_data.AppFlags();	}

	DWORD
		WappAppId() const
		{ return wapp_data.AppId2();}

	DWORD
		WappUrlId() const
		{ return wapp_data.UrlId();}

	int
		GetBoldValue()
		const { return 1; }
};

struct TSonorkMainGroupViewItem
:public TSonorkMainViewItem
{
	SONORK_VIEW_GROUP	group;
	UINT			depth;
	int			e_count,f_count,b_count;

	TSonorkMainGroupViewItem(SONORK_VIEW_GROUP group);

	SONORK_VIEW_TAB
		GetTree() const;

	SONORK_VIEW_GROUP
		Group() const
		{ return group;}

	DWORD
		SearchId() const
		{ return group;}

	UINT	Depth() const
		{ return depth;}

	int	SubFolders() const
		{ return f_count;}

	int	GetEventValue()	const
		{ return e_count;}

	int	GetBoldValue() const
		{ return b_count;}

	void	IncEventCount(int d)
		{ e_count+=d;}

	void	IncBoldCount(int d)
		{ b_count+=d;}

	bool	IsUserGroup() const
		{
			return IsLocalUserGroup()||IsRemoteUserGroup();
		}

	bool	IsLocalUserGroup()	const
		{
			return group>=SONORK_VIEW_USER_GROUP_FIRST && group<=SONORK_VIEW_USER_GROUP_LAST;
		}

	bool	IsRemoteUserGroup()	const
		{
			return group==SONORK_VIEW_SYS_GROUP_REMOTE_USERS;
		}

	bool	IsCustomLocalUserGroup() const
		{
			return group>SONORK_VIEW_USER_GROUP_FIRST && group<=SONORK_VIEW_USER_GROUP_LAST;
		}



	bool	IsAnyWappGroup()	const
		{
			return group>=SONORK_VIEW_WAPP_GROUP_FIRST && group<=SONORK_VIEW_WAPP_GROUP_LAST;
		}

	bool	IsCustomWappGroup() const
		{
			return group>SONORK_VIEW_WAPP_GROUP_FIRST && group<=SONORK_VIEW_WAPP_GROUP_LAST;
		}

};


struct TSonorkSysGroupViewItem
:public TSonorkMainGroupViewItem
{
	TSonorkSysGroupViewItem(SONORK_VIEW_GROUP group):TSonorkMainGroupViewItem(group){}

	SONORK_C_CSTR
		GetLabel(BOOL expanded,SKIN_ICON&)	const;

	SONORK_VIEW_GROUP
		GetDispGroup() const
		{ return SONORK_VIEW_GROUP_ROOT; }

	DWORD
		GetStateFlags()	const
		{ return TVIS_BOLD; }
};

struct TSonorkCustomGroupViewItem
:public TSonorkMainGroupViewItem
{
	DWORD			base_no;
	DWORD			parent_no;
	TSonorkShortString	name;
	// Does not take ownership of UG (Does not delete it)
	TSonorkCustomGroupViewItem(DWORD base_no, const TSonorkGroup*UG);


	SONORK_C_CSTR
		GetLabel(BOOL expanded,SKIN_ICON&) const;

	SONORK_VIEW_GROUP
		GetDispGroup()		const
		{ return (SONORK_VIEW_GROUP)(base_no+parent_no); }

	UINT
		GetSortIndex()	const;

	DWORD
		GetStateFlags()	const;

};

struct TSonorkExtUserViewItem
:public TSonorkMainViewItem
{
	TSonorkExtUserData	*UD;

	TSonorkExtUserViewItem(TSonorkExtUserData*pUD)
		:TSonorkMainViewItem(SONORK_TREE_ITEM_EXT_USER){UD=pUD;}

	TSonorkExtUserData*
		UserData()
		{ return UD;}

	DWORD
		SearchId() const
		{ return (DWORD)UD; }

	DWORD
		GetStateFlags()	const;

	int	GetEventValue()	const
		{ return (int)UD->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT); }

	int	GetBoldValue()	const
		{ return UD->addr.sidFlags.SidMode()!=SONORK_SID_MODE_DISCONNECTED?1:0; }

	SONORK_VIEW_TAB
		GetTree() 	const;
		
	SONORK_C_CSTR
		GetLabel(BOOL expanded,SKIN_ICON&)	const;

	SONORK_VIEW_GROUP
		GetDispGroup() const;

	UINT
		GetSortIndex()const;
};

struct TSonorkAuthReqViewItem
:public TSonorkMainViewItem
{

	TSonorkId		userId;
	TSonorkShortString   	alias;
	bool			sent_by_us;
	TSonorkUserInfoFlags    user_info_flags;

	TSonorkAuthReqViewItem()
	:TSonorkMainViewItem(SONORK_TREE_ITEM_AUTH_REQ){}

	DWORD
		SearchId() const
		{ return Incomming()?1:0; }

	SONORK_C_CSTR
		GetLabel(BOOL expanded,SKIN_ICON&) const;

	SONORK_VIEW_TAB
		GetTree() const
		{ return SONORK_VIEW_TAB_TREE_AUTHS;}

	SONORK_VIEW_GROUP
		GetDispGroup() const
		{ return SONORK_VIEW_SYS_GROUP_AUTHS;}

	bool
		Outgoing() const
		{ return sent_by_us;}

	bool
		Incomming() const
		{ return !sent_by_us;}

	DWORD

		GetStateFlags()	const
		{ return Incomming()?TVIS_BOLD:0; }

	int
		GetBoldValue() const
		{ return Incomming()?1:0;}

	int
		GetEventValue()	const
		{ return Incomming()?(SONORK_TREE_ITEM_VALUE_F_HIDE_COUNT|1):0;}

	UINT
		GetSortIndex() const
		{ return Incomming()?1:0;}

};
// TSonorkViewItemPtrs
// utility union used to cast from one view item type to another
union TSonorkViewItemPtrs
{
	TSonorkTreeItem		*ptr;
	TSonorkMainViewItem 	*item;
	TSonorkSysGroupViewItem	*sys;
	TSonorkExtUserViewItem  *user;
	TSonorkMainGroupViewItem* group;
	TSonorkCustomGroupViewItem*custom_group;
	TSonorkAuthReqViewItem	*auth;
	TSonorkAppViewItem	*app;
	TSonorkExtAppViewItem	*extapp;
	TSonorkWebAppViewItem	*webapp;
};
#define SONORK_MAINWIN_POKE_SHOW_LOGIN_DIALOG		SONORK_WIN_POKE_01
#define SONORK_MAINWIN_POKE_EXEC_ITEM			SONORK_WIN_POKE_02
#define SONORK_MAINWIN_POKE_EXEC_ITEM_NO_CTRL		SONORK_WIN_POKE_03
#define SONORK_MAINWIN_POKE_TOGGLE_EXPAND_ITEM		SONORK_WIN_POKE_04
#define SONORK_MAINWIN_POKE_CONNECT_DISCONNECT		SONORK_WIN_POKE_05
#define SONORK_MAINWIN_POKE_TOGGLE_SELECT_ITEM		SONORK_WIN_POKE_06
#define SONORK_MAINWIN_POKE_CHECK_MAIL			SONORK_WIN_POKE_07




// TASKS
struct TSonorkPopCheckState;
struct TSonorkEmailAccount;
class  TSonorkEmailExceptQueue;

class TSonorkMainWin
:public TSonorkTaskWin
{
public:
	enum TASK_TYPE
	{
	  TASK_NONE
	, TASK_SET_USER
	, TASK_ADD_GROUP
	, TASK_SET_GROUP
	, TASK_DEL_GROUP
	, TASK_DEL_USER
	, TASK_REFRESH_PROFILE
	};
	enum MAIN_WIN_SELECT
	{
	  MAIN_WIN_SELECT_SET
	, MAIN_WIN_SELECT_CLEAR
	, MAIN_WIN_SELECT_TOGGLE
	};


private:
	struct TOOLBAR
	{
		HWND	hwnd;
		SIZE	size;
	}tool_bar;

	struct INFO
	{
		HWND	hwnd;
		int		height;
	}info;

	struct BICHO
	{
		HWND	hwnd;
	}bicho;

	struct LOGO
	{
		HWND		hwnd;
		int	  	height;
	}logo;

	struct SID_MODE
	{
		HWND	hwnd;
		int	height;
	}sid_mode_combo;

	struct _VIEW
	{
		HWND	      		tabHwnd;
		HWND	      		titleHwnd;
		TSonorkTreeView		tree[SONORK_VIEW_TAB_COUNT];
		TSonorkWinCtrl		ctrl[SONORK_VIEW_TAB_COUNT];
		struct {
			TSonorkSysGroupViewItem	*local_users;
			TSonorkSysGroupViewItem	*remote_users;
			TSonorkSysGroupViewItem	*auths;
			TSonorkSysGroupViewItem	*extapps;
			TSonorkSysGroupViewItem	*webapps;
		}group;
		int			update_count;
		int	      		tab_height;
		int	      		top;
		int	      		height;
		SONORK_VIEW_TAB		ctxTab;
		TSonorkViewItemPtrs	ctxVP;
		TSonorkDropTarget	drop_target;
		HTREEITEM		dropHitem;
		UINT			sel_count;
	}view;

	struct _TASK
	{
		TASK_TYPE		type;
		TSonorkError		ERR;
		TSonorkPopCheckState*	pop;
	}task;


	bool	OnCreate();
	void 	OnAfterCreate();
	void    OnBeforeDestroy();
	void	OnDestroy();
	void	OnAfterDestroy();
	bool    OnSysCommand(UINT);
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	LRESULT	OnCtlWinMsg(class TSonorkWinCtrl*,UINT,WPARAM,LPARAM);

	bool	OnMeasureItem(MEASUREITEMSTRUCT*);
	bool	OnDrawItem(DRAWITEMSTRUCT*);
	void 	OnSize(UINT);
	bool	OnMinMaxInfo(MINMAXINFO*);
	LRESULT	OnNotify(WPARAM,TSonorkWinNotify*);
	void	OnToolTipText(UINT id, HWND, TSonorkWinToolTipText&TTT );
	void	OnInitMenu(HMENU);
	void	OnInitMenuPopup(HMENU ,UINT,BOOL);
	void	OnActivate(DWORD flags, BOOL minimized);
	void	OnTimer(UINT);

	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	LRESULT OnDragDrop(SONORK_DRAG_DROP_EVENT,LPARAM);

	bool	OnAppEvent(UINT event, UINT param, void*data);


	void 	OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size);
	void		OnSetGroupTaskData(TSonorkDataPacket*P, UINT P_size);
	void		OnSetUserTaskData(TSonorkDataPacket*P, UINT P_size);
	void 	OnTaskEnd(SONORK_WIN_TASK_TYPE, const SONORK_DWORD2&, const TSonorkError*);

	void	ShowLoginDialog();
	void	CmdConnect(BOOL);
	void	CmdCfgNetwork();
	void	CmdAddUser(bool show_err_msg);
	void	CmdUserAuth(TSonorkExtUserData*);

	void	CmdDelete();
	void	CmdModifyGroup(UINT);
	void	CmdSidModeSelected();
	void	SetSidMode( SONORK_SID_MODE mode );
	void 	CmdUserAuth(UINT, bool visibility);
//	void	CmdTellAFriend();

	void	CmdSwitchMode();
	void	CmdGroupMsg(TSonorkClipData*);
	void	OnViewRClick(POINT&);


	void	RealignControls();


	// Sets the menu labels to the proper language strings
	// The main menu is in the resource file; the popup menus
	// are created in LoadMenuLabels
	//void	LoadMenuLabels();

	void	SetDropTarget(HTREEITEM, int y_pos);

	// Updates the interface to reflect current connection status
	void	OnAppEventCxStatus(UINT old_status);


// ----------------------
// UpdateSidModeIndicator()
// Updates the user mode indicator to reflect current user mode (sid mode)

	void	UpdateSidModeIndicator();
	void	UpdateConnectIndicators();
	void	UpdateCaption();


	void 	DrawSidMode( DRAWITEMSTRUCT*S );
	void	DoDrag(NM_TREEVIEW*);
public:
	TSonorkMainWin();



// ----------------------
// UpdateInterfaceFunctions()
// Enables/disables the interface functions.
// + It is invoked by TSonorkWin32App::OnGlobalTask() when a
//   global task starts/ends.
// + and by OnAppEventCxStatus() to update those items
//   that work only on certain connection statuses.
// + also by OnCreate() on initialization
	void	UpdateInterfaceFunctions();


// --------------------------------------------------------------
// Menu/Toolbar

private:

	void	SetToolBarIcon( UINT, SKIN_ICON );
	void	SetToolBarState(UINT, UINT);
	void	GetToolBarRect( RECT* rect ){ ::GetWindowRect(tool_bar.hwnd, rect); }
	void	SetupToolBar();
	void	EnableCommand( UINT , bool );
	void	CheckCommand( UINT , bool );
	void	UpdateSkinDependantItems();


// --------------------------------------------------------------
// Tree/View info

public:

const	TSonorkTreeView&
		GetTree(SONORK_VIEW_TAB t) const;

const	TSonorkTreeView&
		GetActiveTree() const
		{
			return GetTree(view.ctxTab);
		}
	void
		SetActiveTab(SONORK_VIEW_TAB tab, bool forced, bool update_tab);


// ---------------------------------------------------------------------------
// Add/Del items
// ---------------------------------------------------------------------------

public:

	void
		AddViewItem(TSonorkMainViewItem*);

	void
		DelViewItem(TSonorkMainViewItem*);

// ---------------------------------------------------------------------------
// Get tree items
// ---------------------------------------------------------------------------

public:

	TSonorkAppViewItem*
		GetAppViewItem(DWORD cmd);

	TSonorkMainGroupViewItem*
		GetGroupViewItem(SONORK_VIEW_GROUP grp, bool use_default) const;

	TSonorkExtUserViewItem*
		GetExtUserViewItem(const TSonorkId&);

	TSonorkExtUserViewItem*
		GetExtUserViewItem(const TSonorkExtUserData*);

	TSonorkAuthReqViewItem*
		GetAuthReqViewItem(const TSonorkId&);

private:

static 	TSonorkMainViewItem*
		GetViewItem(const TSonorkTreeView& TV,HTREEITEM hItem)
		{ return (TSonorkMainViewItem*)TV.GetItem(hItem); }


	TSonorkMainViewItem*
		GetViewItemIntendedParent(TSonorkMainViewItem*) const;

	bool
		ContextViewItemIs(SONORK_TREE_ITEM_TYPE) const;

	TSonorkExtUserData*
		GetContextUser() const;

// ---------------------------------------------------------------------------
// Find tree items
// ---------------------------------------------------------------------------

public:
	TSonorkAuthReqViewItem*
		FindFirstIncommingAuthReqViewItem();

private:
	TSonorkMainViewItem*
		FindFirstItemWithEvents(const TSonorkTreeView&,HTREEITEM);

// ---------------------------------------------------------------------------
// Count/Load items
// ---------------------------------------------------------------------------

public:
	int	CountAuthReqViewItems(bool incomming, bool outgoing);

	HMENU	LoadWappTemplatesMenuFromViewItems(HTREEITEM pHitem=NULL);

	DWORD	GetFreeGroupNo();
private:

	bool	_GetFreeGroupNo(HTREEITEM, DWORD&, UINT level);


// ---------------------------------------------------------------------------
// Focus/expansion
// ---------------------------------------------------------------------------

public:
	void	ExpandGroupViewItemLevels(SONORK_VIEW_GROUP grp
			, BOOL expand
			, int  sub_levels=0);
	void	FocusViewItem(TSonorkMainViewItem*);
	void	EnsureVisibleViewItem(TSonorkMainViewItem*, BOOL force_view_visible);
	void	EnsureVisibleGroupViewItem(SONORK_VIEW_GROUP grp, BOOL force_view_visible);



// ---------------------------------------------------------------------------
// Clear items
// ---------------------------------------------------------------------------

public:
	void	ClearViewGroup(SONORK_VIEW_GROUP);

private:
	void	ClearViewItemChildren(HWND,HTREEITEM);

// ---------------------------------------------------------------------------
// Redraw/Update
// ---------------------------------------------------------------------------

public:

	void	RepaintViewItem(TSonorkMainViewItem*);


	void	UpdateViewItemAttributes(TSonorkMainViewItem*
				, int	b_delta
				, int 	e_delta
				, UINT	flags);

	void	UpdateUserViewItemAttributes(const TSonorkExtUserData*UD
				, int	sid_delta
				, int 	msg_delta
				, UINT	flags);

	void
		AfterMassUpdate(SONORK_VIEW_TAB);

private:
	void
		RecalcViewItemParent( TSonorkMainViewItem* );


// ---------------------------------------------------------------------------
// Sorting
// ---------------------------------------------------------------------------

public:
	void
		SortViewGroup(SONORK_VIEW_GROUP , bool recursive);

private:

// --------------------------------------------------------------
// Misc. Interface

public:

	void
		PinWindow(BOOL left, UINT width);

	void
		CloseApp(bool interactive);
		
private:

	void
		HideTreeToolTips();

// --------------------------------------------------------------
// Hint/Info

public:
	void
		SetHintMode(SONORK_C_CSTR, bool update_window);
		// Set to NULL to leave hint mode

	TSonorkMainInfoWin*
		InfoWin()
		{
			return (TSonorkMainInfoWin*)Handle_to_SonorkWin(info.hwnd);
		}
// --------------------------------------------------------------

public:

	TSonorkBicho*
		Bicho()
		{
			return (TSonorkBicho*)Handle_to_SonorkWin(bicho.hwnd);
		}

// --------------------------------------------------------------
// Selection

public:

	UINT    GetSelection(SONORK_DWORD2List*);
	void    SetSelection(TSonorkMainViewItem*, bool clear_prev);
	void    ClearSelection(BOOL redraw);

private:
static	void	_GetSelection(const TSonorkTreeView&,SONORK_DWORD2List*,HTREEITEM);
static	void	_ClearSelection(const TSonorkTreeView&,HTREEITEM);
	UINT	_ToggleSelectItem(TSonorkMainViewItem*);
	void	_BroadcastSelectionEvent();

// ----------------------------------------------------------------------------
//	ASYNC TASKS

public:
	bool
		Task_IsCheckingMail() const
		{ return task.pop!=NULL; }

	bool
		Task_StartMailCheck();

	void
		Task_StopMailCheck( bool abort);

	SONORK_RESULT
		Task_AssignGroupToUser(const TSonorkExtUserData*,DWORD group_no);

	SONORK_RESULT
		Task_SetUserAuth(const TSonorkExtUserData*, const TSonorkAuth2&);

	SONORK_RESULT
		Task_SetGroup(TSonorkGroup*,bool add);

	SONORK_RESULT
		Task_DelGroup(SONORK_GROUP_TYPE,DWORD group_no);

	SONORK_RESULT
		Task_AddUser(const TSonorkId& gu_id,const TSonorkText&);

	SONORK_RESULT
		Task_DelUser(const TSonorkId& gu_id);

	SONORK_RESULT
		Task_RefreshUserProfile( DWORD taskFlags );


private:
	SONORK_RESULT
		Task_Start(TASK_TYPE
			,TSonorkDataPacket*
			,UINT
			,GLS_INDEX
			,DWORD taskFlags);
	void
		Task_UpdateStatus();
		
	void
		Pop_Start();

	bool
		Pop_Connect(const TSonorkPhysAddr&);

	void
		Pop_ProcessLine(SONORK_C_CSTR);

	void
		Pop_StartUIDL(SONORK_C_STR);

	void
		Pop_ProcessUIDL(SONORK_C_CSTR);

	void
		Pop_StartNextTopList(SONORK_C_STR);

	void
		Pop_ProcessTopList(SONORK_C_CSTR);

	void
		Pop_StopTopList();

	void
		Pop_StartQuit(SONORK_C_STR);

	void
		Pop_Stop(bool abort);

	void
		Pop_SaveAccounts();

	UINT
		Pop_AddNotifications(bool abort);

static	void
		Pop_LoadNotificationExceptions(TSonorkEmailExceptQueue*);

	UINT
		Pop_AddAccountNotifications(TSonorkEmailExceptQueue*);

	static UINT
		Pop_ProcessValue(SONORK_C_CSTR str);
};

#endif


