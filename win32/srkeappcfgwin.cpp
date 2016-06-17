#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkeappcfgwin.h"
#include "srkwin32app_extapp.h"

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

#define EAPP_POKE_EDIT_ITEM	SONORK_WIN_POKE_01
static const char *szApps="Apps";


// ----------------------------------------------------------------------------
// TSonorkServicesWin
// ----------------------------------------------------------------------------

TSonorkServicesWin::TSonorkServicesWin()
	:TSonorkWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_EAPPCFG
	,SONORK_WIN_SF_NO_WIN_PARENT
	)
{

}

// ----------------------------------------------------------------------------

#define SERVICES_COLS	4
bool
 TSonorkServicesWin::OnCreate()
{
	union {
		LV_COLUMN	lv_col;
	}D;
	RECT	rect;
	int	tcx,i;
	static struct{
		int 		cx;
		GLS_INDEX	gls;
	}col[SERVICES_COLS]=
	{{70	, GLS_LB_APP}
	,{170	, GLS_LB_NAME}
	,{50	, GLS_LB_TYPE}
	,{-1	, GLS_LB_INFO}
	};

	list.hwnd = GetDlgItem(IDC_EAPPCFG_LIST);
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_SERVICES,true,NULL);


	ListView_SetImageList(list.hwnd,sonork_skin.Icons(),LVSIL_SMALL);

	::GetClientRect( list.hwnd, &rect );
	D.lv_col.mask=LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
	D.lv_col.fmt =LVCFMT_LEFT;

	for(tcx=i=0;i<SERVICES_COLS;i++)
	{
		D.lv_col.cx  = i==SERVICES_COLS-1?rect.right-tcx:col[i].cx;
		D.lv_col.pszText=(char*)SonorkApp.LangString(col[i].gls);
		ListView_InsertColumn(list.hwnd,i,&D.lv_col);
		tcx+=D.lv_col.cx;
	}


	LoadList();
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkServicesWin::OnBeforeDestroy()
{
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_SERVICES,false,NULL);
}


// ----------------------------------------------------------------------------

void
 TSonorkServicesWin::LoadList()
{
	TSonorkListIterator	I;
const 	TSonorkServiceEntry*	E;
	ListView_DeleteAllItems(list.hwnd);
	int			index;
	LV_ITEM			lv_item;
	TSonorkShortString	svcName;
	SONORK_C_CSTR		svcDesc;
	SONORK_C_CSTR		svcType;
	char			tmp[32];



	lv_item.lParam	= 0;

	SonorkApp.Service_BeginEnum(I);

	for(	  index=0
		; (E=SonorkApp.Service_EnumNext(I,&svcName)) != NULL
		; index++
		)
	{

		svcDesc=SonorkApp.Service_GetDescription(
				  E->sii
				, &svcType
				, NULL
				);
		lv_item.iItem	= index;
		lv_item.mask 	= LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lv_item.iSubItem=0;
		lv_item.pszText = ultoa(E->sii.ServiceInstance(),tmp,16);
		lv_item.iImage  = E->icon;
		lv_item.iItem   = ListView_InsertItem( list.hwnd, &lv_item);

		lv_item.mask 	= LVIF_TEXT;

		lv_item.iSubItem= 1;
		lv_item.pszText = (char*)svcName.CStr();
		ListView_SetItem( list.hwnd, &lv_item);

		lv_item.iSubItem= 2;
		lv_item.pszText = (char*)svcType;
		ListView_SetItem( list.hwnd, &lv_item);

		lv_item.iSubItem= 3;
		lv_item.pszText = (char*)svcDesc;
		ListView_SetItem( list.hwnd, &lv_item);



	}

	SonorkApp.Service_EndEnum(I);
}

// ----------------------------------------------------------------------------

bool
 TSonorkServicesWin::OnCommand(UINT id,HWND , UINT  code)
{

	if(code==BN_CLICKED)
	{
		switch( id )
		{
			case IDOK:
				// Break ommited, fall through to case IDCANCEL:EndDialog(id)
			case IDCANCEL:
				EndDialog(id);
				break;

			case IDC_EAPPCFG_INSTALL:
				LoadList();
				break;

			default:
				return false;
		}
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkServicesWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{	{IDC_EAPPCFG_INSTALL
		, GLS_OP_REFRESH	}
	,	{-1
		,	GLS_LB_APPS	}
	,	{0
		,	GLS_NULL	}
	};
	LoadLangEntries( gls_table, true );
}


// ----------------------------------------------------------------------------
// TSonorkEappItemWin (declaration)
// ----------------------------------------------------------------------------

class TSonorkEappItemWin
:public TSonorkWin
{
private:
	char		app_section[LEA_MAX_APP_SECTION_NAME_SIZE];
	SONORK_C_STR    app_name;
	LPARAM*		enabled;
	SONORK_C_CSTR 	ini_file;

	bool	OnCommand(UINT id,HWND hwnd, UINT code);
	void	LoadLabels();

protected:
	bool OnCreate();

public:
	TSonorkEappItemWin(TSonorkWin*parent,SONORK_C_STR , LPARAM*enabled, SONORK_C_CSTR ini_file);
};

// ----------------------------------------------------------------------------
// TSonorkEappCfgWin
// ----------------------------------------------------------------------------

TSonorkEappCfgWin::TSonorkEappCfgWin()
	:TSonorkWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_EAPPCFG
	,SONORK_WIN_SF_NO_WIN_PARENT
	)
{

}

// ----------------------------------------------------------------------------

bool
 TSonorkEappCfgWin::OnCommand(UINT id,HWND , UINT  code)
{

	if(code==BN_CLICKED)
	{
		switch( id )
		{
			case IDOK:
				CmdSave();
				// Break ommited, fall through to case IDCANCEL:EndDialog(id)
			case IDCANCEL:
				EndDialog(id);
				break;

			case IDC_EAPPCFG_INSTALL:
				CmdInstall();
				break;

			default:
				return false;
		}
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkEappCfgWin::CmdInstall()
{
	TSonorkShortString	install_ini_str;
	TSonorkTempBuffer	buffer(LEA_SECTION_MAX_SIZE);
	char			app_name[LEA_MAX_APP_NAME_SIZE];
	char			section_name[LEA_MAX_APP_SECTION_NAME_SIZE];
	char			*install_ini,*buffer_ptr;
	bool			install_ok=false;
	UINT			aux,context;
	buffer_ptr = buffer.CStr();
	if(!SonorkApp.GetLoadPath(Handle()
				, install_ini_str
				, NULL
				, GLS_OP_OPEN
				, NULL
				, "ini"
				, OFN_EXPLORER
				  | OFN_LONGNAMES
				  | OFN_NOCHANGEDIR
				  | OFN_PATHMUSTEXIST
				  | OFN_FILEMUSTEXIST
				))
	{
		return;
	}
	install_ini = install_ini_str.Buffer();
	for(;;)
	{
		if(!stricmp(install_ini,ini_file))
			break;
		GetPrivateProfileString(szApps,"Name","",app_name,LEA_MAX_APP_NAME_SIZE,install_ini);
		if(!*app_name)
			break;
		GetPrivateProfileString(szApps,"IniFileVersion","0",buffer_ptr,8,install_ini);
		aux=strtoul(buffer_ptr,NULL,10);
		if(aux<1||aux>8)
			break;

		GetPrivateProfileString(szApps,app_name,"",buffer_ptr,LEA_LINE_MAX_SIZE,ini_file);
		if(*buffer_ptr)
		{
			SonorkApp.LangSprintf(buffer_ptr,GLS_QU_RPLCSTR,app_name);
			if(MessageBox(buffer_ptr,szSONORK,MB_ICONQUESTION|MB_YESNO)!=IDYES)
				return;
		}
		for( context=0
			;context<SONORK_EXT_APP_CONTEXTS+1
			;context++)
		{
			wsprintf(section_name,"%s.%-.8s"
				,app_name
				,context<SONORK_EXT_APP_CONTEXTS
					?sonork_ext_app_context_name[context]
					:"name");
			if(GetPrivateProfileSection(section_name,buffer_ptr,LEA_SECTION_MAX_SIZE-4,install_ini)<8)
				continue;
			WritePrivateProfileSection(section_name,buffer_ptr,ini_file);
		}
		WritePrivateProfileString(szApps,app_name,"1",ini_file);
		install_ok=true;
		break;

	}
	if(!install_ok)
	{
		MessageBox(GLS_ERR_BADFILFMT,szSONORK,MB_ICONSTOP|MB_OK);
	}
	else
	{
		MessageBox(GLS_MS_SAVED,szSONORK,MB_ICONASTERISK|MB_OK);
		LoadList();
	}
}

// ----------------------------------------------------------------------------

void
	TSonorkEappCfgWin::CmdSave()
{
	int		items;
	LV_ITEM	lv;
	char	app_name[LEA_MAX_APP_NAME_SIZE];
	char	value[2];

	items=ListView_GetItemCount( list.hwnd );
	lv.mask 	= LVIF_TEXT|LVIF_PARAM;
	lv.pszText	= app_name;
	lv.cchTextMax = LEA_MAX_APP_NAME_SIZE;
	lv.iSubItem	=0;
	value[1]=0;
	for(lv.iItem =0 ; lv.iItem<items ; lv.iItem++)
	{
		if(!ListView_GetItem(list.hwnd,&lv))continue;
		value[0]=lv.lParam!=0?'1':'0';
		WritePrivateProfileString("Apps" , app_name , value ,ini_file );
	}
	WritePrivateProfileString(NULL,NULL,NULL,ini_file );
	SonorkApp.PostAppCommand(SONORK_APP_COMMAND_RELOAD_EXT_APPS,0);
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkEappCfgWin::OnPoke(SONORK_WIN_POKE poke, LPARAM )
{
	LV_ITEM	lv;
	char	app_name[LEA_MAX_APP_NAME_SIZE];
	if( poke == EAPP_POKE_EDIT_ITEM )
	for(;;)
	{
		lv.iItem = ListView_GetNextItem(list.hwnd, -1, LVNI_FOCUSED|LVNI_SELECTED);
		if( lv.iItem == -1)break;
		lv.mask 	= LVIF_TEXT|LVIF_PARAM;
		lv.pszText	= app_name;
		lv.cchTextMax = LEA_MAX_APP_NAME_SIZE;
		lv.iSubItem	=0;
		if(!ListView_GetItem(list.hwnd,&lv))break;
		if(TSonorkEappItemWin(this,app_name, &lv.lParam, ini_file ).Execute()==IDOK)
			AddSetItem(app_name,lv.lParam,lv.iItem);
		break;
	}
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkEappCfgWin::AddSetItem(SONORK_C_CSTR name , BOOL enabled , int iItem)
{
	LV_ITEM			lv_item;
	lv_item.iItem   =
		(iItem == -1?(ListView_GetItemCount( list.hwnd )):(iItem));

	lv_item.lParam	= enabled;
	lv_item.mask 	= LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
	lv_item.iSubItem=0;
	lv_item.pszText = (char*)name;
	lv_item.iImage  = SKIN_ICON_PLUGIN_ERROR;
	if( enabled )
	if(SonorkApp.GetExtApp(name,SONORK_EXT_APP_MAIN_CONTEXT,NULL)!=NULL)
		lv_item.iImage  = SKIN_ICON_PLUGIN_OK;

	if( iItem == -1 )
		lv_item.iItem 	= ListView_InsertItem( list.hwnd, &lv_item);
	else
		ListView_SetItem( list.hwnd, &lv_item);

	lv_item.mask 	= LVIF_TEXT;
	lv_item.iSubItem= 1;
	lv_item.pszText = (char*)SonorkApp.LangString(enabled?GLS_LB_YES:GLS_LB_NO);
	ListView_SetItem( list.hwnd, &lv_item);

}

// ----------------------------------------------------------------------------

void
 TSonorkEappCfgWin::LoadList()
{
	TSonorkTempBuffer	buffer(LEA_SECTION_MAX_SIZE);
	int				i;
	char			*equ,*app_name;

	ini_file_str.SetBufferSize(SONORK_MAX_PATH);
	ini_file=ini_file_str.Buffer();
	SonorkApp.GetExtAppsIniFile( ini_file );
	ListView_DeleteAllItems(list.hwnd);
	i = GetPrivateProfileSection("Apps",buffer,LEA_SECTION_MAX_SIZE-4,ini_file);
	*(DWORD*)(buffer.CStr()+i)=0;
	if(i > 2)
	{
		for(app_name=buffer.CStr()
			;*app_name !=0
			;app_name +=  strlen(app_name) + 1)
		{
			equ = strchr(app_name,'=');
			if(equ == NULL)continue;
			*equ++=0;
			if(strlen(app_name) >= LEA_MAX_APP_NAME_SIZE )
				continue;
			AddSetItem(app_name,*equ=='1',-1);
		}

	}
}

// ----------------------------------------------------------------------------

bool
 TSonorkEappCfgWin::OnCreate()
{
	union {
		LV_COLUMN	lv_col;
	}D;
	RECT	rect;
	list.hwnd = GetDlgItem(IDC_EAPPCFG_LIST);
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_EXT_APP_CONFIG,true,NULL);

	::GetClientRect( list.hwnd, &rect );
	D.lv_col.mask=LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
	D.lv_col.fmt =LVCFMT_LEFT;
	D.lv_col.cx  = rect.right - 64;
	D.lv_col.pszText=(char*)SonorkApp.LangString(GLS_LB_APP);
	ListView_InsertColumn(list.hwnd,0,&D.lv_col);
	D.lv_col.cx  = 64;
	D.lv_col.pszText=(char*)SonorkApp.LangString(GLS_LB_ENA);
	ListView_InsertColumn(list.hwnd,1,&D.lv_col);

	ListView_SetImageList(list.hwnd,sonork_skin.Icons(),LVSIL_SMALL);

	LoadList();
	return true;
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkEappCfgWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	if(!TestWinSysFlag(SONORK_WIN_SF_INITIALIZED))return 0;

	if( N->hdr.hwndFrom == list.hwnd)
	{
		if( N->hdr.code == NM_DBLCLK )
		{
			PostPoke( EAPP_POKE_EDIT_ITEM , 0);
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkEappCfgWin::OnBeforeDestroy()
{
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_EXT_APP_CONFIG,false,NULL);
}

// ----------------------------------------------------------------------------

void
 TSonorkEappCfgWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{	{IDC_EAPPCFG_INSTALL,	GLS_OP_INSTALL	}
	,	{-1					,	GLS_LB_APPS	}
	,	{0					,	GLS_NULL		}
	};
	LoadLangEntries( gls_table, true );
}

// ----------------------------------------------------------------------------
// TSonorkEappItemWin (definition)
// ----------------------------------------------------------------------------

TSonorkEappItemWin::TSonorkEappItemWin(TSonorkWin*parent,SONORK_C_STR papp_name, LPARAM*penabled, SONORK_C_CSTR pini_file)
	:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_EAPPITEM
	,0
	)
{
	app_name	=papp_name;
	enabled     =penabled;
	ini_file	=pini_file;
	wsprintf(app_section,"%s.%s"
		,app_name
		,sonork_ext_app_context_name[SONORK_EXT_APP_MAIN_CONTEXT]);
		
}

// ----------------------------------------------------------------------------

bool
 TSonorkEappItemWin::OnCreate()
{
	TSonorkTempBuffer tmp(LEA_LINE_MAX_SIZE);
	SetCtrlChecked(IDC_EAPPITEM_ENABLE,*enabled!=0);
	SetCtrlChecked(IDC_EAPPITEM_DISABLE,*enabled==0);
	SetWindowText(app_name);
	GetPrivateProfileString(app_section
			,"WebSite"
			,SonorkApp.LangString(GLS_LB_NA)
			,tmp.CStr()
			,LEA_LINE_MAX_SIZE
			,ini_file);
	SetCtrlText(IDC_EAPPITEM_URL,tmp.CStr());
	GetPrivateProfileString(app_section
			,"Cmd"
			,""
			,tmp.CStr()
			,LEA_LINE_MAX_SIZE
			,ini_file);
	SetCtrlText(IDC_EAPPITEM_IMAGE,tmp.CStr());
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkEappItemWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{
		{IDL_EAPPITEM_IMAGE		,	GLS_LB_INFO	}
	,	{IDC_EAPPITEM_ENABLE	,	GLS_LB_ENA }
	,	{IDC_EAPPITEM_DISABLE	,	GLS_LB_DIS }
	,	{0						,	GLS_NULL	}
	};
	LoadLangEntries(gls_table,true);

}

// ----------------------------------------------------------------------------

bool
 TSonorkEappItemWin::OnCommand(UINT id,HWND , UINT  code)
{
	if((id==IDOK||id==IDCANCEL)&&code==BN_CLICKED)
	{
		*enabled=GetCtrlChecked(IDC_EAPPITEM_ENABLE);
		EndDialog(id);
		return true;
	}
	return false;
}
// ----------------------------------------------------------------------------

