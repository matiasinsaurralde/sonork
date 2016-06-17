#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop

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

#include "srknetcfgwin.h"
#include "srkinputwin.h"


// ----------------------------------------------------------------------------

TSonorkNetCfgWin::TSonorkNetCfgWin(TSonorkWin*parent)
	:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_NETCFG
	, 0)
{
}

// ----------------------------------------------------------------------------

void
 TSonorkNetCfgWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{	{IDL_NETCFG_SOCKS_HOST	, GLS_LB_SVR_HOST	}
	,	{IDL_NETCFG_SONORK_HOST , GLS_LB_SVR_HOST	}
	,	{IDL_NETCFG_SOCKS_PORT  , GLS_LB_PORT		}
	,	{IDC_NETCFG_SOCKS_ENABLE, GLS_LB_ENA		}
	,	{IDC_NETCFG_PROFILE_ADD	, GLS_OP_ADD		}
	,	{IDC_NETCFG_PROFILE_DEL	, GLS_OP_DEL		}
	,	{IDG_NETCFG_SOCKS
		|SONORK_WIN_CTF_BOLD	, GLS_LB_SOCKS		}
	,	{IDG_NETCFG_PROFILE
		|SONORK_WIN_CTF_BOLD	, GLS_LB_PROFILES	}
	,	{IDG_NETCFG_SONORK
		|SONORK_WIN_CTF_BOLD	, GLS_NULL		}
	,	{IDOK
		|SONORK_WIN_CTF_BOLD	, GLS_OP_ACCEPT		}
	,	{IDCANCEL
		|SONORK_WIN_CTF_BOLD	, GLS_OP_CLOSE		}
	,	{-1			, GLS_LB_NETCFG 	}
	,	{0			, GLS_NULL		}
	};
	LoadLangEntries( gls_table, false );
	SetCtrlText(IDG_NETCFG_SONORK,szSONORK);

}

// ----------------------------------------------------------------------------

void
 TSonorkNetCfgWin::LoadServerProfile()
{
	int i;
	SetWinUsrFlag(SONORK_WIN_F_UPDATING);

	SonorkApp.LoadServerProfile(SonorkApp.ProfileServerProfile().CStr()
		,SP
		,true
		,&SonorkApp.wProfileServerProfile());

	if(!SP.socks.TcpPort())SP.socks.SetTcpPort(1080);
	SetCtrlUint(IDC_NETCFG_SOCKS_PORT,SP.socks.TcpPort());
	SetCtrlText(IDC_NETCFG_SOCKS_HOST,SP.socks.HostName());
	SetCtrlChecked(IDC_NETCFG_SOCKS_ENABLE,SP.socks.IsEnabled());

	if(!SP.sonork.TcpPort())SP.sonork.SetTcpPort(1504);
	if(!SP.sonork.UdpPort())SP.sonork.SetUdpPort(1503);

	SetCtrlUint(IDC_NETCFG_SONORK_TCPPORT,SP.sonork.TcpPort());
	SetCtrlUint(IDC_NETCFG_SONORK_UDPPORT,SP.sonork.UdpPort());
	SetCtrlText(IDC_NETCFG_SONORK_HOST,SP.sonork.HostName());
	SetCtrlChecked(IDC_NETCFG_SONORK_TCP,SP.sonork.AddrType() == SONORK_PHYS_ADDR_TCP_1);
	SetCtrlChecked(IDC_NETCFG_SONORK_UDP,SP.sonork.AddrType() == SONORK_PHYS_ADDR_UDP_1);
	SetCtrlEnabled(IDC_NETCFG_SONORK_UDP,!SP.socks.IsEnabled());
	SetCtrlUint(IDC_NETCFG_UTS_PORT1,SP.nat.range.v[0]);
	SetCtrlUint(IDC_NETCFG_UTS_PORT2,SP.nat.range.v[1]);

	i=ComboBox_FindStringExact(profile_list,-1,SonorkApp.ProfileServerProfile().CStr());
	if(i==CB_ERR)
		i=0;
	ComboBox_SetCurSel(profile_list,i);
	GetCtrlText(IDC_NETCFG_PROFILE , SonorkApp.wProfileServerProfile());
	// Save ctrl data only
	SonorkApp.SaveCurrentProfile( SONORK_APP_BASE_SPF_SAVE_CTRL_DATA );
	SetCtrlEnabled(IDC_NETCFG_PROFILE_DEL
		, SONORK_StrNoCaseCmp(SonorkApp.ProfileServerProfile().CStr(),szDefault));

	ClearWinUsrFlag(SONORK_WIN_F_UPDATING);

}

// ----------------------------------------------------------------------------

bool
 TSonorkNetCfgWin::OnCreate()
{
	TSonorkBfEnumHandle *E;

	profile_list = GetDlgItem( IDC_NETCFG_PROFILE );

	ComboBox_Clear(profile_list);
	ComboBox_AddString( profile_list , szDefault );
	E=SonorkApp.ConfigFile().OpenEnum(SonorkApp.AppKey()
		,"Servers"
		, SONORK_BF_ENUM_DATA);
	if( E != NULL)
	{
		while(SonorkApp.ConfigFile().EnumNext(E) == SONORK_RESULT_OK )
		{
			if(!SONORK_StrNoCaseCmp(E->Name(),szDefault))
			{
				// Don't add the "Default" profile:
				// we add it manually
				// because it must always exist
				continue;
			}
			ComboBox_AddString( profile_list ,E->Name() );
		}
		SONORK_MEM_DELETE(E);
	}
	LoadServerProfile();

	return true;

}

// ----------------------------------------------------------------------------

void
 TSonorkNetCfgWin::CmdSave()
{
	if(GetCtrlChecked(IDC_NETCFG_SOCKS_ENABLE))
		SP.socks.Enable();
	else
		SP.socks.Disable();


	SP.nat.range.v[0]=GetCtrlUint(IDC_NETCFG_UTS_PORT1);
	SP.nat.range.v[1]=GetCtrlUint(IDC_NETCFG_UTS_PORT2);
	if(SP.nat.range.v[0]==0||SP.nat.range.v[1]==0
	||SP.nat.range.v[0]>SP.nat.range.v[1])
		SP.nat.range.Clear();
	SP.socks.SetTcpPort( GetCtrlUint(IDC_NETCFG_SOCKS_PORT) );
	GetCtrlText(IDC_NETCFG_SOCKS_HOST,SP.socks.Host());
	SP.socks.SetAddrType( SONORK_PHYS_ADDR_TCP_1  );

	SP.sonork.SetTcpPort( GetCtrlUint(IDC_NETCFG_SONORK_TCPPORT) );
	SP.sonork.SetUdpPort( GetCtrlUint(IDC_NETCFG_SONORK_UDPPORT) );
	GetCtrlText(IDC_NETCFG_SONORK_HOST,SP.sonork.Host());
	SP.sonork.SetAddrType(
		GetCtrlChecked(IDC_NETCFG_SONORK_TCP)
		?SONORK_PHYS_ADDR_TCP_1
		:SONORK_PHYS_ADDR_UDP_1);

	SP.sonork.Enable();
	SonorkApp.SaveServerProfile(SonorkApp.ProfileServerProfile().CStr(),SP);
}

// ----------------------------------------------------------------------------

void
 TSonorkNetCfgWin::CmdProfileDel()
{
	int i;
	if( !SONORK_StrNoCaseCmp(SonorkApp.ProfileServerProfile().CStr(),szDefault) )
	{
		// Cannot delete "default" profile
		return;
	}
	SonorkApp.DeleteServerProfile(SonorkApp.ProfileServerProfile().CStr());
	i=ComboBox_FindStringExact(profile_list,-1,SonorkApp.ProfileServerProfile().CStr());
	if( i!=CB_ERR )
	{
		ComboBox_DelString(profile_list,i);
	}
	SonorkApp.wProfileServerProfile().Set(szDefault);
	LoadServerProfile();
	
}

// ----------------------------------------------------------------------------

void
 TSonorkNetCfgWin::CmdProfileAdd()
{
	TSonorkInputWin W(this);
	int i;
	SONORK_C_CSTR new_profile_name;
	CmdSave();
	W.sign=SKIN_SIGN_TOOLS;
	W.help.SetBufferSize(128);
	sprintf(W.help.Buffer()
		,"%s\n%s\n%s"
		,SonorkApp.LangString(GLS_LB_NETCFG)
		,SonorkApp.LangString(GLS_LB_PROFILES)
		,SonorkApp.LangString(GLS_OP_ADD));
	W.prompt.Set(SonorkApp.LangString(GLS_LB_NAME));
	if(W.Execute() == IDOK && W.input.Length() > 2)
	{
		SetWinUsrFlag(SONORK_WIN_F_UPDATING);
		new_profile_name=W.input.CStr();
		i=ComboBox_FindStringExact(profile_list,-1,new_profile_name);
		if(i==CB_ERR)
		{
			if(SonorkApp.LoadServerProfile(szDefault,SP,true,NULL)==SONORK_RESULT_OK)
			if(SonorkApp.SaveServerProfile(new_profile_name,SP)==SONORK_RESULT_OK)
			{
				i=ComboBox_AddString( profile_list ,new_profile_name );
				SonorkApp.wProfileServerProfile().Set( new_profile_name );
			}
		}
		ComboBox_SetCurSel(profile_list,i);
		ClearWinUsrFlag(SONORK_WIN_F_UPDATING);
	}
}

// ----------------------------------------------------------------------------

bool
 TSonorkNetCfgWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code == BN_CLICKED)
	{
		switch(id)
		{
			case IDC_NETCFG_PROFILE_ADD:
				CmdProfileAdd();
				break;
				
			case IDC_NETCFG_PROFILE_DEL:
				CmdProfileDel();
				break;

			case IDC_NETCFG_SOCKS_ENABLE:
				code = GetCtrlChecked(IDC_NETCFG_SOCKS_ENABLE);
				SetCtrlChecked(IDC_NETCFG_SONORK_TCP,code);
				SetCtrlChecked(IDC_NETCFG_SONORK_UDP,!code);
				SetCtrlEnabled(IDC_NETCFG_SONORK_UDP,!code);
				break;

			case IDOK:
				GetCtrlText(IDC_NETCFG_PROFILE,SonorkApp.wProfileServerProfile());
				CmdSave();
				Destroy();
				break;
			default:
				return false;
		}
		return true;
	}
	else
	if( code == CBN_SELENDOK && id==IDC_NETCFG_PROFILE)
	{
		if(TestWinSysFlag(SONORK_WIN_SF_INITIALIZED)
		&& !TestWinUsrFlag(SONORK_WIN_F_UPDATING) )
		{
			CmdSave();
			GetCtrlText(IDC_NETCFG_PROFILE,SonorkApp.wProfileServerProfile());
			LoadServerProfile();
		}
	}
	return false;
}

// ----------------------------------------------------------------------------

