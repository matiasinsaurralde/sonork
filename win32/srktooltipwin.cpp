#include "srkwin32app.h"
#pragma hdrstop
#include "srktooltipwin.h"

static HWND TT_handle;

HWND SONORK_TT_Handle()
{
	return TT_handle;
}
bool SONORK_TT_Init()
{

	TT_handle = CreateWindow(TOOLTIPS_CLASS
		, (LPSTR) NULL
        , TTS_ALWAYSTIP
        , CW_USEDEFAULT
        , CW_USEDEFAULT
        , CW_USEDEFAULT
        , CW_USEDEFAULT
        , NULL
        , (HMENU) NULL
        , SonorkApp_Instance()
        , NULL);
	if(TT_handle!=NULL)
    {

		//TT_delay=::SendMessage(TT_handle,TTM_GETDELAYTIME,TTDT_INITIAL,0);
		::SendMessage(TT_handle,TTM_SETDELAYTIME,TTDT_INITIAL,(LPARAM)(MAKELONG(200,0)));
		::SetWindowPos(TT_handle
			,HWND_TOPMOST
			,0,0,0,0,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);

		return true;
	}
	return false;
}

bool
 SONORK_TT_AddCtrl(HWND owner_hwnd,UINT count,UINT*id_list)
{
	TOOLINFO tool_info;
	UINT i;
	//BOOL result;
	if( TT_handle==NULL  )
		return false;
	for(i=0;i<count;i++,id_list++)
	{
		tool_info.cbSize = sizeof(TOOLINFO);
		tool_info.uFlags = TTF_IDISHWND	| TTF_SUBCLASS;
		tool_info.hwnd   = owner_hwnd;
		tool_info.uId    = (UINT)GetDlgItem(owner_hwnd,*id_list);
		tool_info.hinst = 0;
		tool_info.lpszText = LPSTR_TEXTCALLBACK;
		SendMessage(TT_handle, TTM_ADDTOOL, 0,
		    (LPARAM) (LPTOOLINFO) &tool_info);
		//_T("TT: + %u of %x = %u",tool_info.uId,tool_info.hwnd,result);
	}
	return true;
}
bool
 SONORK_TT_AddCtrl(HWND owner_hwnd,HWND ctrl_hwnd)
{
	TOOLINFO tool_info;
	if( TT_handle==NULL  )return false;
	tool_info.cbSize = sizeof(TOOLINFO);
	tool_info.uFlags = TTF_IDISHWND	| TTF_SUBCLASS;
	tool_info.hwnd   = owner_hwnd;
	tool_info.uId    = (UINT)ctrl_hwnd;
	tool_info.hinst = 0;
	tool_info.lpszText = LPSTR_TEXTCALLBACK;
	SendMessage(TT_handle, TTM_ADDTOOL, 0,  (LPARAM) (LPTOOLINFO) &tool_info);
	return true;

}
bool SONORK_TT_AddRect(HWND owner_hwnd,UINT id,RECT*rect)
{
	TOOLINFO tool_info;
	if( TT_handle==NULL  )return false;
	tool_info.cbSize = sizeof(TOOLINFO);
	tool_info.uFlags = TTF_SUBCLASS;
	tool_info.hwnd   = owner_hwnd;
	tool_info.uId    = id;
	tool_info.hinst  = 0;
	tool_info.lpszText = LPSTR_TEXTCALLBACK;
	tool_info.rect   = *rect;
	SendMessage(TT_handle, TTM_ADDTOOL, 0,
	(LPARAM) (LPTOOLINFO) &tool_info);
	return true;
}


void SONORK_TT_Del(HWND owner_hwnd)
{
	UINT 			tool_no,tools,remove_items;
	TOOLINFO 		tool_info;
	char			tmp[64];
	UINT			*remove_list;

	if( TT_handle==NULL  ) return;

	tools=::SendMessage(TT_handle,TTM_GETTOOLCOUNT,0,0);
//	TRACE_DEBUG("SONORK_TT_Del(%x), Tools=%u",owner_hwnd,tools);
	if(tools > 0 )
	{
		SONORK_MEM_NEW(remove_list=new UINT[tools+1]);
		remove_items=0;
		for(tool_no=0;tool_no<tools;tool_no++)
		{
			tool_info.cbSize=sizeof(tool_info);
			tool_info.lpszText=tmp;
			if(!::SendMessage(TT_handle
				,TTM_ENUMTOOLS
				,(WPARAM)tool_no,(LPARAM)&tool_info))
					continue;
/*
			TRACE_DEBUG("[%04u]  TT_Test(%u)=%x"
				,tool_no
				,tool_info.uId
				,tool_info.hwnd
				);
*/
			if(tool_info.hwnd != owner_hwnd)
				continue;

			*(remove_list+remove_items)=tool_info.uId;
			remove_items++;
		}
		if( remove_items > 0 )
		{
			UINT *pId = remove_list;
			for(tool_no=0;tool_no<remove_items;tool_no++,pId++)
			{
				tool_info.cbSize=sizeof(tool_info);
				tool_info.uId   =*pId;
				tool_info.hwnd	= owner_hwnd;
				::SendMessage(TT_handle,TTM_DELTOOL,0,(LPARAM)&tool_info);
//				TRACE_DEBUG("[%04u]  TT_Del(%u)",tool_no,tool_info.uId);
			}
		}
		SONORK_MEM_DELETE_ARRAY(remove_list);
	}
}

void SONORK_TT_Exit()
{
	if(TT_handle!=NULL)
	{
		DestroyWindow(TT_handle);
		TT_handle=NULL;
	}
}

/*
BOOL CALLBACK _SONORK_TT_InstallControls(HWND hwndCtrl, LPARAM lParam)
{
    char szClass[64];

    // Skip static controls.
    GetClassName(hwndCtrl, szClass, sizeof(szClass));
    if (lstrcmpi(szClass, "STATIC") )
    {
    }
    return TRUE;
}
*/
