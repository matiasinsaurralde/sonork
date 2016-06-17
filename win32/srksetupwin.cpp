#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "setup.rh"
#include "srksetupwin.h"
#include "srknetcfgwin.h"
#include "srk_winregkey.h"
#include "srk_file_io.h"
#include "srk_svr_login_packet.h"
#include "srk_cfg_names.h"


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


#define TIMER_MSECS			250
#undef _GLS
#define _GLS(n)	SL_##n
enum SETUP_LABEL
{
	SL_NULL		=-1
,	_GLS(WC1) 	= 0
,	_GLS(EXWC)
,	_GLS(INWC)

// -------------

,	_GLS(EX2NEW)
,	_GLS(EX2OLD)
,	_GLS(IN2NEW)
,	_GLS(IN2OLD)

// -------------

,	_GLS(NEW1HLP)
,	_GLS(NEW2HLPEX)
,	_GLS(NEW2HLPIN)
,	_GLS(NEW3HLPEX1)
,	_GLS(NEW3HLPEX2)
,	_GLS(NEW3HLPIN1)
,	_GLS(NEW3HLPIN2)

// -------------

,	_GLS(OLD1HLP)
,	_GLS(SRES_RS)
,	_GLS(SRES_RN)

// -------------

,	_GLS(SBOLD1_RN)
,	_GLS(SBOLD1_RS)
,	_GLS(SBNEWOK)
,	_GLS(SBNEWERR)
,	_GLS(SBWC)
,	_GLS(SBDE)
,	_GLS(SBOLD1)
,	_GLS(SBNEW1)
,	_GLS(SBNEW2)
,	_GLS(SBNEW3)
,	_GLS(SBRSING)
,	_GLS(SBCXING)
,	_GLS(SBRQING)
,	_GLS(SBRCING)
,	_GLS(BNEW)
,	_GLS(BOLD)
,	_GLS(BNEXT)
,	_GLS(BPREV)
,	_GLS(BEND)
,	_GLS(TINET)
,	_GLS(TINTRA)

// -------------

,	_GLS(SNOINPUT)
,	_GLS(GUIDINV)
,	_GLS(NAMETSHORT)
,	_GLS(PASSTSHORTNEW)
,	_GLS(PASSTSHORTOLD)
,	_GLS(PASSNMATCH)
,	_GLS(EMAILTSHORT)
,	_GLS(NOEMAIL)

// -------------

,	_GLS(MDERREX)
,	_GLS(MDERRIN)

// -------------

,	_GLS(CXERREX)
,	_GLS(CXERRIN)
,	_GLS(CXLOST)
,	_GLS(RTIMEOUT)

// -------------

,	_GLS(RNEWOKEX)
,	_GLS(RNEWOKIN)
,	_GLS(RNEWDENIEDEX)
,	_GLS(RNEWDENIEDIN)
,	_GLS(RNEWERREX)
,	_GLS(RNEWERRIN)

// -------------

,	_GLS(ROLDOKEX)
,	_GLS(ROLDOKIN)
,	_GLS(ROLDERR)
,	_GLS(ROLDDENIED)
,	_GLS(ROLDERREX)
,	_GLS(ROLDERRIN)

// -------------

,	SL_COUNT
};
#undef _GLS(n)

#define UWF_USER_INFO_INITIALIZED	SONORK_WIN_F_USER_01


// ---------------------------------------------------------------------------

TSonorkSetupWin::TSonorkSetupWin(TSonorkWin*parent)
	:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_SETUP
	,0)
{

	cur_step		= STEP_INVALID;
	cur_phase		= PHASE_INVALID;
	cur_dialog		= NULL;
	resolve_handle	= NULL;

	UDN.user_data.SetUserInfoLevel(SONORK_USER_INFO_LEVEL_3,false);

}

// ---------------------------------------------------------------------------

TSonorkSetupWin::STEP TSonorkSetupWin::NextStep(TSonorkSetupWin::STEP S)
{
	switch(S)
	{
		case STEP_WELCOME:		return STEP_DECIDE;
		case STEP_NEW_1:		return STEP_NEW_2;
		case STEP_NEW_2:		return STEP_NEW_3;
		case STEP_NEW_3:		return STEP_NEW_DONE;
		case STEP_OLD_1:		return STEP_OLD_DONE;
	}
	return S;
}

TSonorkSetupWin::STEP TSonorkSetupWin::PrevStep(TSonorkSetupWin::STEP S)
{
	switch(S)
	{
		case STEP_DECIDE:	return STEP_WELCOME;
		case STEP_NEW_1:
		case STEP_OLD_1:	return STEP_DECIDE;
		case STEP_NEW_2:	return STEP_NEW_1;
		case STEP_NEW_3:	return STEP_NEW_2;
	}
	return S;
}

// ---------------------------------------------------------------------------

void	TSonorkSetupWin::SetStep( STEP new_step )
{
	int	 	new_dialog_id;
	BOOL	done;

	if( new_step == STEP_NEW_3 || cur_step == STEP_NEW_3)
		SetPhase(PHASE_NONE);
		
	done =(new_step==STEP_NEW_DONE || new_step==STEP_OLD_DONE);
	SetCtrlEnabled( IDCANCEL		, !done);
	SetCtrlEnabled( IDOK			, new_step != STEP_DECIDE);
	if(new_step<=STEP_WELCOME)
		SetCtrlText(IDCANCEL,GLS_OP_CANCEL);
	else
		SetLabel(IDCANCEL,SL_BPREV);
	SetLabel(IDOK, done ? SL_BEND : SL_BNEXT );

	new_dialog_id =-1;
	switch(new_step)
	{
		case STEP_WELCOME:
			new_dialog_id =IDD_SETUPWC;
			break;

		case STEP_DECIDE:
			new_dialog_id =IDD_SETUPD;
			ClearWinUsrFlag(UWF_USER_INFO_INITIALIZED);
			break;

		case STEP_NEW_1:
			new_dialog_id =IDD_SETNEW1;
			break;

		case STEP_NEW_2:
			new_dialog_id =IDD_SETNEW2;
			break;

		case STEP_NEW_3:
			new_dialog_id =IDD_SETUPWC;
			break;

		case STEP_OLD_1:
			new_dialog_id =IDD_SETOLD1;
			break;
	}

	if( cur_step != new_step )
	{
		if(cur_dialog)
		{
			if(cur_step == STEP_OLD_1)
				ListView_DeleteAllItems( cur_dialog->GetDlgItem(IDC_SETOLD1_LIST) );
			cur_dialog->Destroy();
			cur_dialog=NULL;
		}
		cur_step = new_step;

		if(new_dialog_id!=-1)
		{
			cur_dialog=new TSonorkChildDialogWin(this,new_dialog_id,SONORK_WIN_SF_NO_CLOSE);
			SetupDialog();
		}
	}
	SetStatusLabel( SL_NULL , SKIN_HICON_NONE );
}

// ---------------------------------------------------------------------------

void	TSonorkSetupWin::SetupDialog()
{
	TSonorkWinGlsEntry *GE=NULL;
	HWND 	tHwnd;
	int		i;
	UINT	dialog_id,focus_id;
	union {
		SONORK_SEX sex;
		RECT	rect;
		const TSonorkLangCodeRecord	*REC;
	}D;
	static TSonorkWinGlsEntry gls_new1[]=
	{	{IDL_SETNEW1_ALIAS	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_ALIAS	}
	,	{IDL_SETNEW1_NAME	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_NAME		}
	,	{IDL_SETNEW1_NOTES	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_NOTES	}
	,	{IDL_SETNEW1_COUNTRY	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_COUNTRY	}
	,	{IDL_SETNEW1_SEX	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_SEX		}
	,	{IDC_SETNEW1_SEXM	,	GLS_LB_SEXM		}
	,	{IDC_SETNEW1_SEXF	,	GLS_LB_SEXF		}
	,	{IDC_SETNEW1_SEXN	,	GLS_LB_SEXN		}
	,	{0					,	GLS_NULL		}
	};
	static TSonorkWinGlsEntry gls_new2[]=
	{	{IDL_SETNEW2_EMAIL	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_EMAIL	}
	,	{IDL_SETNEW2_PASS1	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_PWD		}
	,	{IDL_SETNEW2_PASS2	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_PWD		}
	,	{0,GLS_NULL}
	};
	static TSonorkWinGlsEntry gls_old1[]=
	{       {IDL_SETOLD1_GUID	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_SRKID	}
	,	{IDL_SETOLD1_ALIAS  	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_ALIAS	}
	,	{IDL_SETOLD1_NAME	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_NAME		}
	,	{IDL_SETOLD1_EMAIL	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_EMAIL	}
	,	{IDC_SETOLD1_CLEAR	|SONORK_WIN_CTF_BOLD
		,	GLS_OP_CLEAR	}
	,	{IDC_SETOLD1_SEARCH	|SONORK_WIN_CTF_BOLD
		,	GLS_OP_SEARCH	}
	,	{0			,	GLS_NULL		}
	};
	static UINT IDD_SETUPD_bold[]=
	{	IDC_SETUPD_NEW
	,	IDC_SETUPD_OLD
	,	0
	};


	SetWinUsrFlag(SONORK_WIN_F_UPDATING);

	cur_dialog->Create();
	cur_dialog->GetWindowRect(&D.rect);
	D.rect.right 	-= D.rect.left;
	D.rect.bottom -= D.rect.top;
	cur_dialog->MoveWindow(
		 dialog_pos.x + (dialog_pos.w - D.rect.right)/2
		,dialog_pos.y + (dialog_pos.h - D.rect.bottom)/2
		,D.rect.right
		,D.rect.bottom
		,false);

	focus_id  = 0;
	dialog_id = cur_dialog->WinResId();
	switch( cur_step )
	{

// -----------

		case STEP_WELCOME:

			assert( dialog_id == IDD_SETUPWC );
			tHwnd = cur_dialog->GetDlgItem(IDC_SETUPWC_LANG);
			ComboBox_Clear( tHwnd );
			SonorkApp.EnumLanguagesIntoComboBox( tHwnd );
			i = ComboBox_FindStringExact(tHwnd,-1,SonorkApp.LangName());
			ComboBox_SetCurSel( tHwnd , i );
			cur_dialog->SetCtrlFont(IDC_SETUPWC_HLP1,sonork_skin.Font(SKIN_FONT_BOLD));
			SetWelcomeLabels();
			cur_dialog->SetCtrlVisible( IDC_SETUPWC_LANG , true );

			
		break;

// -----------

		case STEP_DECIDE:

			assert( dialog_id == IDD_SETUPD );

			cur_dialog->SetCtrlsFont( IDD_SETUPD_bold
				, sonork_skin.Font(SKIN_FONT_BOLD));
			SetDialogLabel(IDC_SETUPD_NEW, SL_BNEW);
			SetDialogLabel(IDC_SETUPD_OLD, SL_BOLD);
			SetDialogLabel(IDL_SETUPD_NEW
				,SonorkApp.IntranetMode()
					?SL_IN2NEW:SL_EX2NEW);

			SetDialogLabel(IDL_SETUPD_OLD
				,SonorkApp.IntranetMode()
					?SL_IN2OLD:SL_EX2OLD);

		break;


// -----------

		case STEP_NEW_1:

			assert( dialog_id == IDD_SETNEW1 );
			InitializeUserSettings();
			GE=gls_new1;

			SetDialogLabel(IDC_SETNEW1_HELP	,SL_NEW1HLP);
			
			focus_id =IDC_SETNEW1_ALIAS;

			cur_dialog->SetCtrlText(IDC_SETNEW1_ALIAS
				,UDN.user_data.alias.CStr());

			cur_dialog->SetCtrlText(IDC_SETNEW1_NAME
				,UDN.user_data.name.CStr());

			cur_dialog->SetCtrlText(IDC_SETNEW1_NOTES
				,UDN.notes.ToCStr());

			D.sex = UDN.user_data.InfoFlags().GetSex();
			cur_dialog->SetCtrlChecked(IDC_SETNEW1_SEXM
				,D.sex == SONORK_SEX_M);
			cur_dialog->SetCtrlChecked(IDC_SETNEW1_SEXF
				,D.sex == SONORK_SEX_F);
			cur_dialog->SetCtrlChecked(IDC_SETNEW1_SEXN
				,D.sex != SONORK_SEX_F && D.sex!= SONORK_SEX_M);


			tHwnd = cur_dialog->GetDlgItem( IDC_SETNEW1_COUNTRY );
			for(i=0,D.REC = SonorkApp.CountryCodeTable().Table()
				;i<SonorkApp.CountryCodeTable().Items()
				;i++,D.REC++)
				ComboBox_AddString( tHwnd , D.REC->name);
			if(SonorkApp.CountryCodeTable().GetRecordByCode(UDN.user_data.Region().GetCountry(), &i)==NULL)
				i = LB_ERR;
			ComboBox_SetCurSel( tHwnd , i );
			UpdateTimeZone();

		break;

// -----------

		case STEP_NEW_2:

			assert( dialog_id == IDD_SETNEW2 );
			GE=gls_new2;
			SetDialogLabel(IDC_SETNEW2_NOEMAIL
				,SL_NOEMAIL);
			SetDialogLabel(IDC_SETNEW2_HELP
				,SonorkApp.IntranetMode()
					?SL_NEW2HLPIN
					:SL_NEW2HLPEX);
			focus_id =IDC_SETNEW2_PASS1;

			cur_dialog->SetCtrlText(IDC_SETNEW2_PASS1
				,UDN.password.CStr());
			cur_dialog->SetCtrlText(IDC_SETNEW2_PASS2
				,UDN.password.CStr());
			cur_dialog->SetCtrlText(IDC_SETNEW2_EMAIL
				,UDN.user_data.email.CStr());
			cur_dialog->SetCtrlVisible(IDC_SETNEW2_NOEMAIL
				,!SonorkApp.IntranetMode());

		break;

// -----------

		case STEP_NEW_3:

			assert( dialog_id == IDD_SETUPWC );

			cur_dialog->SetCtrlFont(IDC_SETUPWC_HLP1
						,sonork_skin.Font(SKIN_FONT_BOLD));

			SetDialogLabel(IDC_SETUPWC_HLP1
						, SonorkApp.IntranetMode()
							?SL_NEW3HLPIN1
							:SL_NEW3HLPEX1);
			SetDialogLabel(IDC_SETUPWC_HLP2
						, SonorkApp.IntranetMode()
							?SL_NEW3HLPIN2
							:SL_NEW3HLPEX2);

			break;

// -----------

		case STEP_OLD_1:

			assert( dialog_id == IDD_SETOLD1 );

			InitializeUserSettings();
			GE=gls_old1;
			focus_id =IDC_SETOLD1_GUID;

			SetDialogLabel( IDC_SETOLD1_HELP , SL_OLD1HLP );
			list.SetHandle(cur_dialog->GetDlgItem( IDC_SETOLD1_LIST )
				, true
				, true);
			SonorkApp.SetupSearchUserList( &list );

		break;

// -----------

		default:
			break;
	}
	if(GE)cur_dialog->LoadLangEntries(GE,false);
	cur_dialog->ShowWindow(SW_SHOW);
	if(focus_id)
		::SetFocus(cur_dialog->GetDlgItem(focus_id));
	ClearWinUsrFlag(SONORK_WIN_F_UPDATING);
}


// ---------------------------------------------------------------------------

void	TSonorkSetupWin::OnNextButton()
{
	switch(cur_step)
	{

// -----------------

		case STEP_NEW_3:
			StartConnectionPhase(OP_CREATE_USER);
		break;


// -----------------

		default:
			SetStep(NextStep(cur_step));
			break;
	}
}

bool
 TSonorkSetupWin::CheckForm()
{
	bool 			rv=false;
	TSonorkShortString 	es;
	HWND 			hwnd;
	int		       	i;
	union {
		SONORK_SEX 	sex;
		const TSonorkLangCodeRecord	*REC;
	}D;


	switch(cur_step)
	{

// -----------------

		case STEP_OLD_1:
			assert(false);
			

// -----------------

		case STEP_NEW_1:

			cur_dialog->GetCtrlText(IDC_SETNEW1_NOTES,UDN.notes);
			cur_dialog->GetCtrlText(IDC_SETNEW1_ALIAS,UDN.user_data.alias);
			if(UDN.user_data.alias.Length()<2)
			{
				es.Set(SonorkApp.LangString(GLS_MS_INVALIAS));
				break;
			}
			cur_dialog->GetCtrlText(IDC_SETNEW1_NAME,UDN.user_data.name);
			if(UDN.user_data.name.Length()<2)
			{
				GetLabel(es,SL_NAMETSHORT);
				break;
			}
			if(cur_dialog->GetCtrlChecked(IDC_SETNEW1_SEXM))
				D.sex = SONORK_SEX_M;
			else
			if(cur_dialog->GetCtrlChecked(IDC_SETNEW1_SEXF))
				D.sex = SONORK_SEX_F;
			else
				D.sex = SONORK_SEX_NA;
			UDN.user_data.wInfoFlags().SetSex(D.sex);

			hwnd = cur_dialog->GetDlgItem( IDC_SETNEW1_COUNTRY );
			i = ComboBox_GetCurSel( hwnd );
			D.REC = SonorkApp.CountryCodeTable().GetRecordByIndex( i );
			if( D.REC != NULL )
				UDN.user_data.wRegion().SetCountry( D.REC->code );
			rv=true;
			break;

// -----------------

		case STEP_NEW_2:

			cur_dialog->GetCtrlText(IDC_SETNEW2_PASS1,aux_str);
			if(aux_str.Length()<2 )
			{
				GetLabel(es,SL_PASSTSHORTNEW);
				break;
			}
			cur_dialog->GetCtrlText(IDC_SETNEW2_PASS2,UDN.password);
			if(strcmp(aux_str.CStr(),UDN.password.CStr()))
			{
				GetLabel(es,SL_PASSNMATCH);
				break;
			}
			cur_dialog->GetCtrlText(IDC_SETNEW2_EMAIL,UDN.user_data.email);
			if(!SonorkApp.IntranetMode() && !cur_dialog->GetCtrlChecked(IDC_SETNEW2_NOEMAIL))
			{
				if(UDN.user_data.email.Length()<2)
				{
					GetLabel(es,SL_EMAILTSHORT);
					break;
				}
			}
			else
				UDN.user_data.email.Clear();
			rv=true;

			break;


// -----------------

		case STEP_NEW_3:
		default:
			rv=true;
			break;
			
	}
	if(!rv)
		MessageBox(es.CStr(),szSONORK,MB_OK|MB_ICONERROR);
	return rv;
}

bool 	TSonorkSetupWin::OnCommand(UINT id,HWND , UINT )
{
	switch(id)
	{
		case IDCANCEL:
			if(cur_step>STEP_WELCOME)
				SetStep(PrevStep(cur_step));
			else
				Destroy(IDCANCEL);
		break;

		case IDC_SETUP_NETCONFIG:
			TSonorkNetCfgWin(this).Execute();
			break;

		case IDOK:
			if( cur_step == STEP_OLD_1 )
			{
				DoNextOld(false);
			}
			else
			{
				if(CheckForm())
					OnNextButton();
			}
		break;

		default:
			return false;

	}
	return true;
}
LRESULT	TSonorkSetupWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	if( wParam == SONORK_WIN_POKE_CHILD_NOTIFY )
	{
		return OnChildDialogNotify((TSonorkChildDialogNotify*)lParam);
	}
	else
	if( wParam == SONORK_WIN_POKE_NET_RESOLVE_RESULT )
	{
		TSonorkAppNetResolveResult*RR =(TSonorkAppNetResolveResult*)lParam;
		if( RR->ERR.Result() == SONORK_RESULT_OK )
		{
			TSonorkError ERR;
			int err;
			if(SP.socks.IsEnabled())
			{
				TSonorkPhysAddr socks_phys_addr;
				if(!socks_phys_addr.SetInet1(
						SONORK_PHYS_ADDR_TCP_1
						,SP.socks.HostName()
						,(WORD)SP.socks.TcpPort()
					))
					tcp.SetSocksV4(socks_phys_addr);
			}
			err=tcp.Connect(*RR->physAddr);
			if(err==0)
			{
				SetPhase(PHASE_CONNECTING);
				return 0L;
			}
			ERR.SetSys(SONORK_RESULT_NETWORK_ERROR,GSS_NETCXERR,err);

		}
		SetPhaseCxError(&RR->ERR);
	}
	return 0L;
}


void	TSonorkSetupWin::SetPhase( PHASE new_phase )
{
	SETUP_LABEL		label;
	bool			active;
	SKIN_HICON	hicon;

	active = (new_phase>=PHASE_RESOLVING && new_phase<PHASE_DONE);
//	TRACE_DEBUG("TSonorkSetupWin::SetPhase( %u,a:%u )  old=%u",new_phase,active,cur_phase);
	SetCtrlEnabled( IDC_SETUP_NETCONFIG , !active ) ;
	SetCtrlEnabled( IDCANCEL , !active ) ;
	SetCtrlEnabled( IDOK	 , !active ) ;
	if( cur_dialog != NULL )
		cur_dialog->EnableWindow(!active ) ;

	if(new_phase == cur_phase)
		return;
	active = (new_phase>=PHASE_CONNECTING && new_phase<PHASE_DONE);
	if( active )
	{
		SetAuxTimer(TIMER_MSECS);
	}
	else
	{
		tcp.Shutdown();
		KillAuxTimer();
		if(new_phase<PHASE_RESOLVING)
		{
			if(resolve_handle)
			{
				SonorkApp.CancelAsyncResolve(Handle());
				resolve_handle=NULL;
			}
		}
	}
	hicon=SKIN_HICON_BUSY;
	switch(new_phase)
	{
		default:
		case PHASE_NONE:		label=SL_NULL;break;
		case PHASE_RESOLVING: 	label=SL_SBRSING;break;
		case PHASE_AUTHORIZING: hicon=SKIN_HICON_CONNECTED;//break ommited to CONNECTING
		case PHASE_CONNECTING:  label=SL_SBCXING;break;
		case PHASE_REQUESTING:	label=SL_SBRQING;hicon=SKIN_HICON_CONNECTED;break;
		case PHASE_RETRIEVING:	label=SL_SBRCING;hicon=SKIN_HICON_CONNECTED;break;
	}
	cur_phase=new_phase;
	if(label != SL_NULL)
		SetStatusLabel( label , hicon);
}

BOOL
 TSonorkSetupWin::LoadLang(HWND tHwnd,const char *name, bool update_combo)
{
	BOOL rv;
	int index;
	SetWinUsrFlag(SONORK_WIN_F_UPDATING);
	if(_LoadLangApp(name))
		rv= _LoadLangLocal(name);
	else
		rv=false;
	if(rv)
	{
		_LoadLabels();
	}
	else
	{
		ErrorBox("Cannot load/No se puede cargar");
		update_combo=true;
	}
	if( update_combo && tHwnd != NULL )
	{
		index = ComboBox_FindStringExact(tHwnd,-1,SonorkApp.LangName());
		ComboBox_SetCurSel(tHwnd,index);
	}
	ClearWinUsrFlag(SONORK_WIN_F_UPDATING);
	return rv;
}

BOOL
   TSonorkSetupWin::_LoadLangApp(const char*lang_name)
{
	TSonorkError ERR;
	if(!SonorkApp.LangLoad( ERR, lang_name , true ))
	if(ERR.Result() != SONORK_RESULT_OK)
	{
		return false;
	}
	return true;
}
BOOL
 TSonorkSetupWin::_LoadLangLocal(const char *name)
{
	SONORK_FILE_HANDLE 	handle;
	TSonorkTempBuffer	file_path( SONORK_MAX_PATH+8 );
	char			tmp[96];
	DWORD			sz;
	sprintf(tmp,"lang\\%s\\setup.txt",name);
	SonorkApp.GetDirPath(file_path.CStr(), SONORK_APP_DIR_ROOT , tmp);
	handle = SONORK_IO_OpenFile(file_path.CStr(),SONORK_OPEN_FILE_METHOD_OPEN_EXISTING);
	if(handle != SONORK_INVALID_FILE_HANDLE)
	{
		SONORK_IO_GetFileSize(handle,&sz);
		lang.buffer.SetSize(sz+64);
		SONORK_IO_ReadFile(handle,lang.buffer.CStr(),sz);
		*(lang.buffer.CStr() + sz)=0;
		SONORK_IO_CloseFile(handle);
		sz=true;
	}
	else
		sz=false;
	return sz;

}
void
 TSonorkSetupWin::DoNextOld(bool is_search_button)
{
	DWORD 	aux;

	UDN.user_data.SetUserInfoLevel(SONORK_USER_INFO_MAX_LEVEL,true);

	cur_dialog->GetCtrlText(IDC_SETOLD1_GUID,aux_str);
	UDN.user_data.userId.SetStr(aux_str.CStr());
	cur_dialog->GetCtrlText(IDC_SETOLD1_ALIAS,UDN.user_data.alias);
	cur_dialog->GetCtrlText(IDC_SETOLD1_NAME,UDN.user_data.name);
	cur_dialog->GetCtrlText(IDC_SETOLD1_EMAIL,UDN.user_data.email);

	if( UDN.user_data.userId.IsZero()
	||  UDN.user_data.alias.Length()<2
	||  UDN.user_data.name.Length()<2
	||  is_search_button)
	{
		ListView_DeleteAllItems( cur_dialog->GetDlgItem(IDC_SETOLD1_LIST) );

		if(UDN.user_data.userId.IsZero()
		&& UDN.user_data.alias.Length()<2
		&& UDN.user_data.name.Length()<2
		&& UDN.user_data.email.Length()<2)
		{
			GetLabel(aux_str,SL_SNOINPUT);
			MessageBox(aux_str.CStr(),szSONORK,MB_OK|MB_ICONSTOP);
		}
		else
		{
			StartConnectionPhase(OP_SEARCH_USER);
		}
	}
	else
	{
		TSonorkTempBuffer	buffer(512);
		GetLabel(aux_str,SL_SBOLD1);
		sprintf(buffer.CStr()
			, "%s?\n%u.%u '%s'\n%s"
			,aux_str.CStr()
			,UDN.user_data.userId.v[0]
			,UDN.user_data.userId.v[1]
			,UDN.user_data.alias.CStr()
			,UDN.user_data.name.CStr());
		if(MessageBox(buffer.CStr(),szSONORK,MB_YESNO|MB_ICONQUESTION)!=IDYES)
			return;

		aux=CreateProfile();
		if( aux == SONORK_RESULT_OK )
		{
			SonorkApp.CloseProfile();
			GetLabel(aux_str
				,SonorkApp.IntranetMode()
					?SL_ROLDOKIN:SL_ROLDOKEX );
			aux=MB_ICONINFORMATION;
		}
		else
		{
			GetLabel(aux_str
				,aux==SONORK_RESULT_ACCESS_DENIED
					?SL_ROLDDENIED:SL_ROLDERR);
			aux=MB_ICONERROR;
		}
		MessageBox(aux_str.CStr()
			,szSONORK
			,MB_OK|aux);
		if(aux != MB_ICONERROR)
		{
			Destroy(IDOK);
			SonorkApp.ShowLoginDialog();
		}
	}


}

LRESULT	TSonorkSetupWin::OnChildDialogNotify(TSonorkChildDialogNotify*DN)
{
	int    index;
	char   lang_name[48];
	TSonorkUserDataNotes*pUDN;
	TSonorkWinNotify*N;

	if( cur_dialog == NULL )
		return false;
	N=DN->N;

	if( DN->dialog_id == IDD_SETUPWC )
	{
		if( N->hdr.code == CBN_SELENDOK )
		{
			if(N->hdr.idFrom == IDC_SETUPWC_LANG )
			if(!TestWinUsrFlag(SONORK_WIN_F_UPDATING))
			{
				index=ComboBox_GetCurSel( N->hdr.hwndFrom );
				if(index!=-1)
				{
					ComboBox_GetString(N->hdr.hwndFrom,index,lang_name);
					LoadLang(N->hdr.hwndFrom,lang_name,false);
					SetWelcomeLabels();
				}
			}
			return true;
		}
		return false;
	}

	if( DN->dialog_id == IDD_SETOLD1 )
	{
		if( N->hdr.idFrom == IDC_SETOLD1_LIST)
		{
			if( N->hdr.code == LVN_DELETEITEM )
			{
				pUDN=(TSonorkUserDataNotes*)N->lview.lParam;
				TRACE_DEBUG("LVN_DELETEITEM(%x)",pUDN);
				if(pUDN)SONORK_MEM_DELETE(pUDN);
			}
			else
			if( N->hdr.code == LVN_ITEMCHANGED )
			{
				if( N->lview.uNewState & LVIS_SELECTED )
				{
					pUDN=(TSonorkUserDataNotes*)N->lview.lParam;
					if(pUDN)
					{
						UDN.user_data.Set(pUDN->user_data);
						UDN.notes.Set(pUDN->notes);
						UDN.user_data.userId.GetStr(lang_name);
						cur_dialog->SetCtrlText(IDC_SETOLD1_GUID,lang_name);
						cur_dialog->SetCtrlText(IDC_SETOLD1_ALIAS,UDN.user_data.alias.CStr());
						cur_dialog->SetCtrlText(IDC_SETOLD1_NAME,UDN.user_data.name.CStr());
					}
				}
			}
		}
		else
		if( N->hdr.code == BN_CLICKED )
		{
			if(N->hdr.idFrom == IDC_SETOLD1_SEARCH )
			{
				DoNextOld( true );
			}
			else
			if(N->hdr.idFrom == IDC_SETOLD1_CLEAR )
			{
				cur_dialog->ClrCtrlText(IDC_SETOLD1_GUID);
				cur_dialog->ClrCtrlText(IDC_SETOLD1_NAME);
				cur_dialog->ClrCtrlText(IDC_SETOLD1_ALIAS);
				cur_dialog->ClrCtrlText(IDC_SETOLD1_EMAIL);
				//cur_dialog->ClrCtrlText(IDC_SETOLD1_PASS);
			}
			else
				return false;
			return true;
		}
		return false;
	}


	if( DN->dialog_id 	== IDD_SETNEW1 )
	{
		if( N->hdr.idFrom == IDC_SETNEW1_TZ_UDN && N->hdr.code == UDN_DELTAPOS)
		{
			index = UDN.user_data.Region().GetTimeZone();
			index -= N->updown.iDelta;

			if( index<SONORK_REGION_TZ_MIN_VALUE
			 || index>SONORK_REGION_TZ_MAX_VALUE )
				return true;      // Prevent change
			UDN.user_data.wRegion().SetTimeZone(index);
			UpdateTimeZone();
		}
		return false;
	}

	if( DN->dialog_id == IDD_SETUPD )
	{
		if( N->hdr.code == BN_CLICKED )
		{
			if( N->hdr.idFrom == IDC_SETUPD_NEW )
				SetStep(STEP_NEW_1);
			else
			if( N->hdr.idFrom == IDC_SETUPD_OLD )
				SetStep(STEP_OLD_1);
			else
				return false;
			return true;
		}
		return false;
	}
	return false;
}

void	TSonorkSetupWin::_LoadLabels()
{
//	TRACE_DEBUG("TSonorkSetupWin::_LoadLabels()");
	GetLabel(aux_str,SonorkApp.IntranetMode()?SL_TINTRA:SL_TINET);
	SetWindowText(aux_str.CStr());
	SetCtrlText(IDC_SETUP_NETCONFIG,GLS_LB_NETCFG);
	SetStep(cur_step);
}

void	TSonorkSetupWin::SetDialogLabel(UINT id, SETUP_LABEL label)
{
	if(cur_dialog)
	{
		GetLabel( aux_str , label );
		cur_dialog->SetCtrlText(id,aux_str.CStr());
	}
}
void	TSonorkSetupWin::SetLabel( UINT id, SETUP_LABEL label )
{
	GetLabel( aux_str , label );
	SetCtrlText(id,aux_str.CStr());

}
void	TSonorkSetupWin::SetStatusLabel(SETUP_LABEL label, SKIN_HICON hicon)
{
	if(label == NULL )
	{
		switch(cur_step)
		{
			case STEP_WELCOME:
				label=SL_SBWC;
				break;

			case STEP_DECIDE:
				label=SL_SBDE;
				break;

//			case STEP_2:label="SB2";break;

//			case STEP_3:label="SB3";break;


			case STEP_NEW_1:
				label=SL_SBNEW1;
				break;

			case STEP_NEW_2:
				label=SL_SBNEW2;
				break;

			case STEP_NEW_3:
				label=SL_SBNEW3;
				break;

			case STEP_OLD_1:
				label=SL_SBOLD1;
				break;

			default:
				label=SL_NULL;
				break;
		}
	}
	if( label != SL_NULL )
	{
		if(hicon == SKIN_HICON_NONE)
			hicon=SKIN_HICON_INFO;
		GetLabel(aux_str,label);
	}
	else
		aux_str.Clear();
	TSonorkWin::SetStatus(status_bar.hwnd,aux_str.CStr(),hicon);
}

// ----------------------------------------------------------------------------

int SONORK_ExecuteSetupDialog(TSonorkWin*W)
{
	return TSonorkSetupWin(W).Execute();
}

// ---------------------------------------------------------------------------

void TSonorkSetupWin::StartConnectionPhase(OPERATION op)
{
	TSonorkError ERR;

	SonorkApp.LoadServerProfile(SonorkApp.ProfileServerProfile().CStr(),SP,true);
	resolve_handle = SonorkApp.AsyncResolve(ERR
		, Handle()
		, SONORK_PHYS_ADDR_TCP_1
		, SP.sonork.HostName()
		, SP.sonork.TcpPort()
		);
	if( resolve_handle )
	{
		cur_op = op;
		SetPhase(PHASE_RESOLVING);
	}
	else
	{
		cur_op = OP_NONE;
		SetPhaseCxError(&ERR);
	}
}

void
 TSonorkSetupWin::OnTimer(UINT id)
{
	if( id==SONORK_WIN_AUX_TIMER_ID)
	{
		if(cur_phase > PHASE_NONE )
		{
			TSonorkTcpPacket *P;
			while((P=tcp.Recv(0)) != NULL )
			{
				ProcessPacket(P);
				Sonork_FreeTcpPacket(P);
			}
			if(tcp.Status() == SONORK_NETIO_STATUS_DISCONNECTED)
			{
				if(cur_phase < PHASE_DONE)
				{
					TSonorkError ERR;
					ERR.SetSys(SONORK_RESULT_NETWORK_ERROR
						,cur_phase==PHASE_CONNECTING?GSS_NETCXERR:GSS_NETCXLOST
						,0);
					if(cur_phase == PHASE_CONNECTING)
						SetPhaseCxError(&ERR);
					else
						SetPhaseError(SL_CXLOST,&ERR);
				}
			}
		}
		else
			KillAuxTimer();
	}
}

bool
 TSonorkSetupWin::OnReqData(TSonorkDataPacket*P,UINT P_size)
{
//	TRACE_DEBUG("TSonorkSetupWin::OnReqData(%s,%u)",SONORK_FunctionName(P->Function()),P_size);
	SetPhase( PHASE_RETRIEVING );
	if( cur_op == OP_SEARCH_USER )
	{
		if( P->Function() == SONORK_FUNCTION_SEARCH_USER_OLD )
		{
			SonorkApp.ProcessSearchUserTaskData(&list,P,P_size,false, NULL);
		}
	}
	else
	if( cur_op == OP_CREATE_USER)
	{
		if( P->Function() == SONORK_FUNCTION_CREATE_USER )
		if( P->SubFunction() == 0)
		{
			if( !P->D_CreateUser_A(P_size,UDN.user_data.userId) )
				return false;

		}
	}
	return 	true;
}

void
 TSonorkSetupWin::OnConnect()
{
	TSonorkSvrLoginReqPacket  REQ;
	DWORD	P_size;

	REQ.Clear();
	REQ.Prepare2(NULL
		,NULL
		,SonorkApp.IntranetMode()
		?SONORK_LOGIN_RF_GUEST|SONORK_LOGIN_RF_INTRANET_MODE
		:SONORK_LOGIN_RF_GUEST|SONORK_LOGIN_RF_INTERNET_MODE
		,SonorkApp.Version());

	P_size = REQ.CODEC_Size()+32;
	{

		TSonorkTempBuffer tmp(P_size);
		REQ.CODEC_WriteMem(tmp.Ptr(),P_size);
		tcp.SendPacket(SONORK_NETIO_HFLAG_TYPE_LOGIN, tmp.Ptr(), P_size );
	}
}
void
 TSonorkSetupWin::OnLogin()
{
	TSonorkDataPacket	*P;
	TSonorkDynData		DD;
	UINT A_size,P_size;

	if( cur_op == OP_SEARCH_USER )
	{
		A_size	= ::CODEC_Size(&UDN,0);
		P	= SONORK_AllocDataPacket( A_size );
		P_size 	= P->E_SearchUser_R_OLD(A_size
			, UDN.user_data.userId
			, UDN.user_data.alias
			, UDN.user_data.name
			, UDN.user_data.email
			, 10);
	}
	else
	if( cur_op == OP_CREATE_USER)
	{
		DD.Clear();
		UDN.user_data.userId.Clear();
		A_size	=
				sizeof(DWORD)
				+ ::CODEC_Size(&UDN,SONORK_UDN_ENCODE_NOTES|SONORK_UDN_ENCODE_PASSWORD)
				+ ::CODEC_Size(&DD)
				+ 32;
		P		= SONORK_AllocDataPacket( A_size );
		P_size 	= P->E_CreateUser1_R(A_size
					, SonorkApp.ReferrerId()
					, UDN
					, DD);
	}
	else
		return;
	tcp.SendPacket(SONORK_NETIO_HFLAG_TYPE_PACKET , P, P_size);
	SONORK_FreeDataPacket( P );
}
void	TSonorkSetupWin::OnReqEnd(TSonorkError&ERR)
{
	SETUP_LABEL	sb;
//	TRACE_DEBUG("TSonorkSetupWin::OnReqEnd(%s,%u)",ERR.ResultName(),ERR.Text().CStr());
	if( ERR.Result() == SONORK_RESULT_OK)
	{
		bool 		bErrFlag;
		bool		bEndFlag=false;
		SetPhase( PHASE_DONE );
		TSonorkShortString	msg;
		if(cur_op == OP_SEARCH_USER )
		{
			bErrFlag=list.GetCount()<=0;
			sb = bErrFlag?SL_SBOLD1_RN:SL_SBOLD1_RS;
			GetLabel(msg,bErrFlag?SL_SRES_RN:SL_SRES_RS);
		}
		else
		if(cur_op == OP_CREATE_USER )
		{
			bErrFlag = UDN.user_data.userId.IsZero();
			if( bErrFlag )
			{
				GetLabel(msg,SonorkApp.IntranetMode()?SL_RNEWERRIN:SL_RNEWERREX);
				sb=SL_SBNEWERR;
			}
			else
			{
				GetLabel(aux_str,SonorkApp.IntranetMode()?SL_RNEWOKIN:SL_RNEWOKEX);
				msg.SetBufferSize(aux_str.Length() + 48 );
				wsprintf(msg.Buffer()
					, aux_str.CStr()
					, UDN.user_data.userId.v[0]
					, UDN.user_data.userId.v[1]);
				bEndFlag=true;
				sb=SL_SBNEWOK;
				if(CreateProfile() == SONORK_RESULT_OK)
				{
					SonorkApp.wProfileUser().Set(UDN.user_data);
					SonorkApp.wProfilePassword().Set(UDN.password);
					SonorkApp.WriteProfileItem("Notes",&UDN.notes);
					SonorkApp.CloseProfile();
				}
				// FIX_WARNING:
				//  No handler for when CreateProfile() fails
				//  (Anyway, if THAT fails, I don't understand how we're here:
				//   the whole application should be collapsing)
			}

		}
		SetStatusLabel( sb , SKIN_HICON_ALERT );
		if(msg.Length())
		{
			MessageBox( msg.CStr()
				, szSONORK
				, MB_OK|(bErrFlag?MB_ICONWARNING:MB_ICONINFORMATION));
		}
		if( bEndFlag )
		{
			Destroy(IDOK);
			SonorkApp.ShowLoginDialog();
		}
		return ;

	}
	SetPhaseOpError(&ERR);
}

void	TSonorkSetupWin::ProcessPacket(TSonorkTcpPacket *tcp_packet)
{
	UINT P_size=tcp_packet->DataSize();
	TSonorkError 	ERR;

	if( tcp_packet->Type() == SONORK_NETIO_HFLAG_TYPE_PACKET  )
	{
		TSonorkDataPacket	*P;
		if(cur_phase != PHASE_REQUESTING && cur_phase != PHASE_RETRIEVING)
			return;

		P=(TSonorkDataPacket*)tcp_packet->DataPtr();
		if( P_size < sizeof(TSonorkDataPacket) )
		{
			// Not a TSonorkDataPacket
			return;
		}
		if( !P->TestMarks( SONORK_PCTRL_MARK_ERROR | SONORK_PCTRL_MARK_NO_DATA ) )
		{
			if( OnReqData(P,P_size) )
				ERR.SetOk();
			else
				SetCodecError(GSS_BADCODEC,ERR);
		}
		else
		{
			P->D_Error_A(P_size,ERR);
			ERR.SetLocal(false);
		}
		if( P->IsAknEnd() || ERR.Result()!=SONORK_RESULT_OK)
			OnReqEnd( ERR );
		return;
	}
	else
	if( tcp_packet->Type() == SONORK_NETIO_HFLAG_TYPE_LOGIN )
	{
		if(cur_phase != PHASE_AUTHORIZING)
			return;
		if(ProcessLoginPacket(ERR,tcp_packet->DataPtr(),P_size))
		{
			SetPhase( PHASE_REQUESTING );
			OnLogin();
			return;
		}
		SetPhaseOpError(&ERR);
	}
	else
	if( tcp_packet->Type() == SONORK_NETIO_HFLAG_TYPE_CONNECT )
	{
		if(cur_phase != PHASE_CONNECTING)
			return;
		if(ProcessConnectPacket(ERR,tcp_packet->DataPtr(),P_size))
		{
			SetPhase(PHASE_AUTHORIZING);
			OnConnect();
			return;
		}
		SetPhaseCxError(&ERR);

	}
}

bool	TSonorkSetupWin::ProcessLoginPacket(TSonorkError& ERR, BYTE*D,DWORD D_size)
{
	TSonorkSvrLoginAknPacket	AKN;
	if( AKN.CODEC_ReadMem(D,D_size) != SONORK_RESULT_OK)
		SetCodecError(GSS_BADCODEC,ERR);
	else
	{
		if( AKN.Result() == SONORK_RESULT_OK )
			return true;
		ERR.SetSys(AKN.Result(),GSS_REQDENIED,SONORK_MODULE_LINE);
	}
	return false;
}

bool	TSonorkSetupWin::ProcessConnectPacket(TSonorkError& ERR, BYTE*D,DWORD D_size)
{
	TSonorkTcpConnectAknPacket*AKN;
	if( D_size < sizeof(TSonorkTcpConnectAknPacket) )
		SetCodecError(GSS_PCKTOOSMALL,ERR);
	else
	{
		AKN = (TSonorkTcpConnectAknPacket*)D;
		if( AKN->result == SONORK_RESULT_OK )
			return true;
		ERR.Set((SONORK_RESULT)AKN->result,AKN->message,SONORK_MODULE_LINE,false);
	}
	return false;
}

// ---------------------------------------------------------------------------

void	TSonorkSetupWin::InitializeUserSettings()
{
	DWORD			aux;
	char			buffer[16];
	TSonorkUserInfo3*	UI;
	union {
	const	TSonorkLangCodeRecord*	REC;
		TIME_ZONE_INFORMATION	TZI;
	}D;

	if( TestWinUsrFlag(UWF_USER_INFO_INITIALIZED) )
		return;
	SetWinUsrFlag(UWF_USER_INFO_INITIALIZED);
	UDN.user_data.SetUserInfoLevel(SONORK_USER_INFO_MAX_LEVEL,false);
	UI = UDN.user_data.wUserInfo();
	SONORK_ZeroMem(UI,UDN.user_data.GetUserInfoSize(false));

	UI->region.SetLanguage(SonorkApp.LangCode());
	UI->pubAuthFlags.SetUserInfoLevel( SONORK_USER_INFO_LEVEL_1 );

	if(GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SABBREVCTRYNAME , buffer, sizeof(buffer))
		!= 0)
	{
		WORD code;
		buffer[0]=(char)tolower(buffer[0]);
		buffer[1]=(char)tolower(buffer[1]);
		code = *(WORD*)buffer;
		// USA (0x7375) is returned in almost all machines
		// but if the language is not EN (0x6e65), then ignore
		// as to not mistreat the non-USA internet minority
		// by defaulting to USA.
		if( !(code == 0x7375 && SonorkApp.LangCode() != 0x6e65) )
		{
			D.REC = SonorkApp.CountryCodeTable().GetRecordByCode( code );
			if(D.REC != NULL )
				UDN.user_data.wRegion().SetCountry( code );
		}
	}

	aux = GetTimeZoneInformation(&D.TZI);
	if(  aux !=(DWORD)-1 )
	{
		int 	tz;
		tz = -((int)D.TZI.Bias);
		tz/=SONORK_REGION_TZ_DIVISOR_MINS;
		UI->region.SetTimeZone(tz);
	}
}

// ---------------------------------------------------------------------------

SONORK_RESULT	TSonorkSetupWin::CreateProfile()
{
	SONORK_RESULT result;

	SonorkApp.CreateProfile(UDN.user_data.userId
			,UDN.user_data.alias.CStr()
			,UDN.password.CStr());
//	TRACE_DEBUG("SW::SonorkApp.CreateProfile() = %s",SONORK_ResultName(result));
	result = SonorkApp.OpenProfile(UDN.user_data.userId,UDN.password.CStr());
//	TRACE_DEBUG("SW::SonorkApp.OpenUserProfile() = %s",SONORK_ResultName(result));

	if( result == SONORK_RESULT_OK )
	{
		TSonorkRegKey regKEY;
		TSonorkRegKey subKEY;
		char tmp[48];
		SonorkApp.ProfileCtrlFlags().Clear(SONORK_PCF_INITIALIZED);
//		if(SonorkApp.IntranetMode())
		SonorkApp.ProfileCtrlFlags().Set(SONORK_PCF_AUTO_AUTH);
		if(regKEY.Open(HKEY_CURRENT_USER,szSrkClientRegKeyRoot,true) == ERROR_SUCCESS)
		{
			if(subKEY.Open(regKEY
				,SonorkApp.ConfigKeyName()
				,true)==ERROR_SUCCESS)
			{
				subKEY.SetValue(szGuId,UDN.user_data.userId.GetStr(tmp));
				subKEY.Close();
			}
			regKEY.Close();
		}
	}
	return result;
}


// ---------------------------------------------------------------------------

bool
 TSonorkSetupWin::OnCreate()
{
	char lang_name[48];
	static UINT main_bold[]=
	{	IDCANCEL
	,	IDOK
	,	0
	};

	SonorkApp.Disconnect();
	SonorkApp.CloseProfile();
	SonorkApp.SetRunFlag(SONORK_WAPP_RF_NO_PROFILE_EVENTS);

	// Status bar
	{
		status_bar.hwnd=GetDlgItem(IDC_SETUP_STATUS);
	}

	{
		RECT R;
		GetCtrlRect( IDC_SETUP_FRAME , &R );
		ScreenToClient( &R );
		dialog_pos.x=R.left;
		dialog_pos.y=R.top;
		dialog_pos.w=R.right-R.left;
		dialog_pos.h=R.bottom-R.top;
	}

	SetCtrlsFont( main_bold	, sonork_skin.Font(SKIN_FONT_BOLD));
	strcpy(lang_name,SonorkApp.LangName());
	LoadLang(NULL,lang_name,true);


	SetPhase( PHASE_NONE );
	SetStep( STEP_WELCOME );
	return true;
}

void	TSonorkSetupWin::OnDestroy()
{
	SonorkApp.ClearRunFlag(SONORK_WAPP_RF_NO_PROFILE_EVENTS);
	SetPhase(PHASE_NONE);
}

// ---------------------------------------------------------------------------

void	TSonorkSetupWin::SetPhaseCxError(TSonorkError*pERR)
{
	SetPhaseError( SonorkApp.IntranetMode()?SL_CXERRIN:SL_CXERREX , pERR );
}
void	TSonorkSetupWin::SetPhaseOpError(TSonorkError*pERR)
{
	SETUP_LABEL sb;
	if( pERR->Result() == SONORK_RESULT_INVALID_MODE )
	{
		sb = SonorkApp.IntranetMode()?SL_MDERRIN:SL_MDERREX;
		SetPhase(PHASE_NONE);
		SetStatusLabel(SL_NULL, SKIN_HICON_NONE );
		GetLabel(aux_str,sb);
		if(MessageBox(aux_str.CStr(),szSONORK,MB_YESNO|MB_ICONQUESTION)==IDYES)
		{
			SonorkApp.PostAppCommand(SONORK_APP_COMMAND_SWITCH_MODE,(LPARAM)"-ShowLogin");
			Destroy(IDOK);
		}
		return;
	}

	if( cur_op == OP_CREATE_USER )
	{
		if( pERR->Result() == SONORK_RESULT_ACCESS_DENIED)
			sb = SonorkApp.IntranetMode()?SL_RNEWDENIEDIN:SL_RNEWDENIEDEX;
		else
			sb = SonorkApp.IntranetMode()?SL_RNEWERRIN:SL_RNEWERREX;
	}
	else
	{
		sb = SonorkApp.IntranetMode()?SL_ROLDERRIN:SL_ROLDERREX;
	}
	SetPhaseError(sb,pERR);
}
void	TSonorkSetupWin::SetPhaseError(SETUP_LABEL label,TSonorkError*pERR)
{
	SetPhase(PHASE_NONE);
	SetStatusLabel(SL_NULL, SKIN_HICON_NONE );
	GetLabel(aux_str,label);
	{
		TSonorkTempBuffer tmp(aux_str.Length()+SP.sonork.host.Length()+64);
		sprintf(tmp.CStr()
			,aux_str.CStr()
			,SP.sonork.HostName()
			,SP.sonork.TcpPort());
		ErrorBox(tmp.CStr(),pERR);
	}
}

void	TSonorkSetupWin::SetCodecError(SONORK_SYS_STRING sys_string,TSonorkError&ERR)
{
	ERR.SetSys(SONORK_RESULT_CODEC_ERROR,sys_string,SONORK_MODULE_LINE);
}


void TSonorkSetupWin::UpdateTimeZone()
{
	char tmp[32];
	cur_dialog->SetCtrlText(IDC_SETNEW1_TZ
	, UDN.user_data.Region().GetTimeZoneStr(tmp));
}

void TSonorkSetupWin::SetWelcomeLabels()
{
	SetDialogLabel(IDC_SETUPWC_HLP1, SL_WC1);
	SetDialogLabel(IDC_SETUPWC_HLP2
				,SonorkApp.IntranetMode()?SL_INWC:SL_EXWC);
}

// ---------------------------------------------------------------------------


const char *
 TSonorkSetupWin::GetLabel(TSonorkShortString&S,SETUP_LABEL label_index)
{
	char tmp[64];
	char *seek,*eptr;
	int	 len;
#define _GLS(n)	#n
static
	const char*
		label_name[SL_COUNT]={
	_GLS(WC1)
,	_GLS(EXWC)
,	_GLS(INWC)

// -------------

,	_GLS(EX2NEW)
,	_GLS(EX2OLD)
,	_GLS(IN2NEW)
,	_GLS(IN2OLD)

// -------------

,	_GLS(NEW1HLP)
,	_GLS(NEW2HLPEX)
,	_GLS(NEW2HLPIN)
,	_GLS(NEW3HLPEX1)
,	_GLS(NEW3HLPEX2)
,	_GLS(NEW3HLPIN1)
,	_GLS(NEW3HLPIN2)

// -------------

,	_GLS(OLD1HLP)
,	_GLS(SRES_RS)
,	_GLS(SRES_RN)

// -------------

,	_GLS(SBOLD1_RN)
,	_GLS(SBOLD1_RS)
,	_GLS(SBNEWOK)
,	_GLS(SBNEWERR)
,	_GLS(SBWC)
,	_GLS(SBDE)
,	_GLS(SBOLD1)
,	_GLS(SBNEW1)
,	_GLS(SBNEW2)
,	_GLS(SBNEW3)
,	_GLS(SBRSING)
,	_GLS(SBCXING)
,	_GLS(SBRQING)
,	_GLS(SBRCING)
,	_GLS(BNEW)
,	_GLS(BOLD)
,	_GLS(BNEXT)
,	_GLS(BPREV)
,	_GLS(BEND)
,	_GLS(TINET)
,	_GLS(TINTRA)

// -------------

,	_GLS(SNOINPUT)
,	_GLS(GUIDINV)
,	_GLS(NAMETSHORT)
,	_GLS(PASSTSHORTNEW)
,	_GLS(PASSTSHORTOLD)
,	_GLS(PASSNMATCH)
,	_GLS(EMAILTSHORT)
,	_GLS(NOEMAIL)

// -------------

,	_GLS(MDERREX)
,	_GLS(MDERRIN)

// -------------

,	_GLS(CXERREX)
,	_GLS(CXERRIN)
,	_GLS(CXLOST)
,	_GLS(RTIMEOUT)

// -------------

,	_GLS(RNEWOKEX)
,	_GLS(RNEWOKIN)
,	_GLS(RNEWDENIEDEX)
,	_GLS(RNEWDENIEDIN)
,	_GLS(RNEWERREX)
,	_GLS(RNEWERRIN)

// -------------

,	_GLS(ROLDOKEX)
,	_GLS(ROLDOKIN)
,	_GLS(ROLDERR)
,	_GLS(ROLDDENIED)
,	_GLS(ROLDERREX)
,	_GLS(ROLDERRIN)
};

	if( label_index < 0 || label_index >= SL_COUNT )
		return "";

	len=sprintf(tmp,"[#%s|",label_name[label_index]);
	seek=strstr(lang.buffer.CStr(),tmp);
	if(seek != NULL)
	{
		seek+=len;
		eptr=strstr(seek,"#]");
		if(eptr != NULL)
		{
			len=(eptr - seek) ;
			S.SetBufferSize(len + 1);
			if(len)memcpy(S.Buffer(),seek,len);
			*(S.Buffer()+len)=0;
		}
		else
			S.Set(label_name[label_index]);
	}
	else
		S.Set(label_name[label_index]);
	return S.CStr();
}

