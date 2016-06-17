#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkinputwin.h"
#include "srkprofileswin.h"
#include "srk_file_io.h"

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

TSonorkProfilesWin::TSonorkProfilesWin(TSonorkWin*parent)
	:TSonorkWin(parent
		,SONORK_WIN_CLASS_NORMAL
		|SONORK_WIN_TYPE_NONE
		|SONORK_WIN_DIALOG
		|IDD_PROFILES
		, 0)
{
}


bool
 TSonorkProfilesWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if(S->CtlID == IDC_PROFILES_ICON )
	{
		sonork_skin.DrawSign(S->hDC
			, SKIN_SIGN_ALERT
			, S->rcItem.left
			, S->rcItem.top  );
		return true;
	}
	return false;
}

bool	TSonorkProfilesWin::GetSelection(TSonorkExtUserData&UD
		, TSonorkProfileCtrlData& 	CD
		, TSonorkShortString*		p_password
		, int*					p_index)
{
	int 	index;
	char	*c,tmp[128];
	index = ListBox_GetCurSel(profile_list);
	if(index != -1 )
	{
		ListBox_GetString(profile_list,index,tmp);
		c=strchr(tmp,':');
		if(!c)index=-1;
		else
		{
			*c=0;
			if(!UD.userId.SetStr(tmp))
				index=-1;
			else
			{
				if(p_index)*p_index=index;
			}
		}
	}
	if( index == -1 )
	{
		MessageBox(GLS_PROF_NSEL,csNULL,MB_OK);
		return false;
	}
	if(SonorkApp.GetProfileInfo(UD,&CD)!=SONORK_RESULT_OK)
	{
		MessageBox(GLS_PROF_RERR,csNULL,MB_OK);
		return false;
	}
	if( CD.Flags().Test(SONORK_PCF_PASS_PROTECT) )
	{
		TSonorkInputWin W(this);
		W.sign=SKIN_SIGN_SECURITY;
		W.flags=SONORK_INPUT_F_PASSWORD;
		W.help.Set(SonorkApp.LangString(GLS_PROF_LOCK));
		W.prompt.Set(SonorkApp.LangString(GLS_LB_PWD));
		if(W.Execute()!=IDOK)return false;
		if(p_password)p_password->Set(W.input);
	}
	return true;
}

bool
 TSonorkProfilesWin::DelProfile()
{
	TSonorkExtUserData 		UD(SONORK_USER_TYPE_UNKNOWN);
	TSonorkProfileCtrlData     	CD;
	TSonorkShortString		password;
	TSonorkTempBuffer		tmp(SONORK_MAX_PATH);
	SONORK_RESULT			result;

	if(!GetSelection(UD,CD,&password,NULL))
		return false;

	SonorkApp.LangSprintf(tmp.CStr(),GLS_MS_SURE_TK,SonorkApp.LangString(GLS_TK_DELUSR));
	if(MessageBox(tmp.CStr(),csNULL,MB_YESNO)!=IDYES)return false;
	if( SonorkApp.IsProfileOpen()
	&&	SonorkApp.ProfileUserId() == UD.userId)
		SonorkApp.CloseProfile();
	result=SonorkApp.DelProfile(UD.userId,password.CStr());
	if(result==SONORK_RESULT_ACCESS_DENIED)
		MessageBox(SonorkApp.SysString(GSS_BADID),csNULL,MB_OK);
	else
	if(result==SONORK_RESULT_OK)
	{
		char gu_id_str[64];
		TSonorkEnumDirHandle*DH;

		wsprintf(gu_id_str,"%u.%u_*.*",UD.userId.v[0],UD.userId.v[1]);
		SonorkApp.GetDirPath(tmp.CStr(),SONORK_APP_DIR_DATA,"");
		DH=SONORK_IO_EnumOpenDir(tmp.CStr(),NULL,gu_id_str);
		if(DH)
		{
			do{
				if(DH->flags&SONORK_FILE_ATTR_DIRECTORY)
					continue;
				SonorkApp.GetDirPath(tmp.CStr(),SONORK_APP_DIR_DATA,DH->name);
				::DeleteFile(tmp.CStr());

			}while( SONORK_IO_EnumNextDir(DH)==SONORK_RESULT_OK );
			SONORK_IO_EnumCloseDir(DH);
		}
		return true;
	}
	return false;
}
bool	TSonorkProfilesWin::OpenProfile()
{
	TSonorkExtUserData 		UD(SONORK_USER_TYPE_UNKNOWN);
	TSonorkProfileCtrlData  CD;
	TSonorkShortString		password;
	SONORK_RESULT			result;
	if(!GetSelection(UD,CD,&password,NULL))
	{
		return false;
	}
	result=SonorkApp.OpenProfile(UD.userId,password.CStr());

	if(result==SONORK_RESULT_OK)
	{
		return true;
	}
	if(result==SONORK_RESULT_ACCESS_DENIED)
		MessageBox(SonorkApp.SysString(GSS_BADID),csNULL,MB_OK);
	return false;
}
bool	TSonorkProfilesWin::OnCommand(UINT id,HWND , UINT code)
{

	// if user double-clicks a string in a list box
	if( id == IDC_PROFILES_LIST && code == LBN_DBLCLK )
	{
		// Simulate the OK button being pressed
		id=IDOK;
		code=BN_CLICKED;
	}
	if( code == BN_CLICKED )
	{
		switch(id)
		{
			case IDOK:
				if(OpenProfile())
					EndDialog(IDOK);
			break;
			case IDC_PROFILES_ADD:
				{
					add_profile_selected=true;
					Destroy(IDCANCEL);
				}
			break;
			case IDC_PROFILES_DEL:
				{
					if(DelProfile())
					{
						LoadProfileList(true,true);
						if(!profile_queue.Items())
						{
							add_profile_selected=true;
							Destroy(IDCANCEL);
						}
					}
				}
			break;
			default:
				return false;
		}
		return true;
	}
	return false;
}

bool
 TSonorkProfilesWin::DoExecute()
{
	LoadProfileList(true,false);
	if(!profile_queue.Items())
		add_profile_selected=true;
	else
		Execute();

	return add_profile_selected;
}

bool
 TSonorkProfilesWin::OnCreate()
{
	add_profile_selected=false;
	profile_list = GetDlgItem(IDC_PROFILES_LIST);
	LoadProfileList(false, true);
	return true;
}
void
 TSonorkProfilesWin::LoadProfileList(bool load_queue, bool load_listbox)
{
	TSonorkBfEnumHandle *E;
	TSonorkExtUserData 	UD(SONORK_USER_TYPE_UNKNOWN);
	TSonorkListIterator	I;
	TSonorkTempBuffer   	tmp(128);
	TSonorkShortString	*S;

	SetWinUsrFlag(SONORK_WIN_F_UPDATING);
	if( load_queue )
	{
		profile_queue.Clear();
		E=SonorkApp.ConfigFile().OpenEnum(NULL,"Profiles", SONORK_BF_ENUM_DIR);
		if( E != NULL)
		{
			while(SonorkApp.ConfigFile().EnumNext(E) == SONORK_RESULT_OK )
			{
				if(SonorkApp.GetProfileInfo(E->Name(),&UD,NULL)!=SONORK_RESULT_OK)
						continue;
				wsprintf(tmp.CStr(),"%s:%s ",E->Name(),UD.Alias().CStr());
				profile_queue.Add(tmp.CStr());
			}
			SONORK_MEM_DELETE(E);
		}

	}
	if( load_listbox )
	{
		ListBox_Clear(profile_list);

		profile_queue.BeginEnum(I);
		while((S=profile_queue.EnumNext(I))!=NULL)
		{
			ListBox_AddString( profile_list ,S->CStr() );
		}
		profile_queue.EndEnum(I);
		if( profile_queue.Items() == 1 )
			ListBox_SetCurSel( profile_list, 0);
	}
	ClearWinUsrFlag(SONORK_WIN_F_UPDATING);
}
void
 TSonorkProfilesWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{	{IDC_PROFILES_DEL		,	GLS_OP_DEL   	}
	,	{IDC_PROFILES_ADD		,	GLS_OP_ADD   	}
	,	{IDC_PROFILES_HELP|SONORK_WIN_CTF_BOLD, GLS_PROF_HELP	}
	,	{IDOK|SONORK_WIN_CTF_BOLD	,	GLS_OP_ACCEPT	}
	,	{IDCANCEL|SONORK_WIN_CTF_BOLD	,	GLS_OP_CANCEL	}
	,	{IDC_PROFILES_LIST|SONORK_WIN_CTF_BOLD,	GLS_NULL	}
	,	{-1				,	GLS_LB_PROFILES	}
	,	{0				,	GLS_NULL	}
	};
	LoadLangEntries( gls_table, true );
}

