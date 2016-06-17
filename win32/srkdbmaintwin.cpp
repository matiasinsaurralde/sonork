#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkdbmaintwin.h"
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

#define PROGRESS_SIZE(sz)	((sz)>>8)
#define AUX_TIMER_MSECS		100
#define WORKING			SONORK_WIN_F_USER_01

TSonorkDbMaintWin::TSonorkDbMaintWin(
	TSonorkWin*parent
	, TSonorkAtomDb *pMsgDb
	, TSonorkAtomDb *pExtDb)
	:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_DBMAINT
	,0)
	,app_msg_path(SONORK_MAX_PATH)
	,app_ext_path(SONORK_MAX_PATH)
	,tmp_msg_path(SONORK_MAX_PATH)
	,tmp_ext_path(SONORK_MAX_PATH)
{
	app_msg_db = pMsgDb;
	app_ext_db = pExtDb;
	SetEventMask(SONORK_APP_EM_PROFILE_AWARE|SONORK_APP_EM_CX_STATUS_AWARE);
}
bool
	TSonorkDbMaintWin::OnCommand(UINT id,HWND , UINT notify_code)
{
	if( notify_code == BN_CLICKED )
	{
		if(id==IDOK)
		{
			if(!TestWinUsrFlag( WORKING ))
				DoMaintenance();
			return true;
		}
		else
		if(id==IDC_DBMAINT_DELMSGS || id == IDC_DBMAINT_SAVMSGS)
		{
			UpdateInterface();
			return true;
		}
		else
		if(id==IDC_DBMAINT_BROWSE)
		{
			if(!TestWinUsrFlag( WORKING ))
				CmdBrowse();
			return true;
		}
	}
	return false;
}
void
	TSonorkDbMaintWin::OnTimer(UINT )
{
	TSonorkExtUserData	*UD;
	TSonorkCCache*		MC;
	TSonorkCCacheEntry	CH;
	DWORD				index,items;
	TSonorkTag			msg_tag;

	// we use prev_version as the user counter for the progress bar
	if( (UD=SonorkApp.UserList().EnumNext(I) ) == NULL )
	{
		SonorkApp.UserList().EndEnum(I);
		KillAuxTimer();
		ClearWinUsrFlag( WORKING );
		MessageBox(	GLS_DBMNTOK,GLS_DBMNT , MB_OK|MB_ICONINFORMATION);
		Destroy(IDOK);
		return;
	}

	user_no++;
	::SendMessage(pb1_hwnd,PBM_SETPOS,user_no,0);
	MC=SonorkApp.GrabSharedMsgCache(UD->userId);
	if(MC != NULL )
	{
		MC->Clear(false);
		MC->SetBulkMode( true );
		items=app_msg_db->Items();
		SetCtrlText(IDL_DBMAINT_ITEM , UD->display_alias.CStr() );
		for(index=0;index<items;index++)
		{
			if(app_msg_db->Get(index,&msg,&msg_tag) != SONORK_RESULT_OK)
			{
				continue;
			}
			if( UD->userId != msg.UserId() )
				continue;
			CH.dat_index = index;
			CH.ext_index = msg_tag.v[SONORK_DB_TAG_INDEX];
			CH.tag.v[SONORK_CCACHE_TAG_FLAGS] = msg_tag.v[SONORK_DB_TAG_FLAGS];
			CH.tag.v[SONORK_CCACHE_TAG_INDEX] = msg.DataServiceDescriptor() ;
			CH.tracking_no.Set(msg.header.trackingNo);
			CH.time.Set(msg.header.time);
			MC->Add( CH , NULL );
		}
		SonorkApp.ReleaseSharedMsgCache( MC );
	}
	else
		TRACE_DEBUG("LOAD: Cannot open ccache for %s",UD->display_alias.CStr());
}

enum MODIFY_DB_NAME_OP
{
	MODIFY_DB_NAME_DELETE
,	MODIFY_DB_NAME_COPY
,	MODIFY_DB_NAME_MOVE
};
void
 ModifyDbName(SONORK_C_CSTR old_db_path,SONORK_C_CSTR new_db_path, MODIFY_DB_NAME_OP op)
{
	TSonorkTempBuffer   oB(SONORK_MAX_PATH),nB(SONORK_MAX_PATH);
	char			*	old_path;
	char			*	new_path;
	old_path=oB.CStr();
	new_path=nB.CStr();
	if(	!new_db_path )*new_path=0;
	for(int ext=0;ext<SONORK_ATOM_DB_FILE_EXTENSIONS;ext++)
	{
		sprintf(old_path,"%s%s",old_db_path,SonorkAtomDbFileExtensions[ext]);
		if(	new_db_path )
			sprintf(new_path,"%s%s",new_db_path,SonorkAtomDbFileExtensions[ext]);
		switch( op )
		{
			case MODIFY_DB_NAME_DELETE:
				DeleteFile(old_path);
				// new_path is not used for DELETE
				break;
			case MODIFY_DB_NAME_COPY:
				CopyFile(old_path,new_path,false);
				break;
			case MODIFY_DB_NAME_MOVE:
				CopyFile(old_path,new_path,false);
				DeleteFile(old_path);
				break;
		}
	}
}
BOOL
	TSonorkDbMaintWin::OnQueryClose()
{
	return TestWinUsrFlag(WORKING);
}
bool
	TSonorkDbMaintWin::OpenArchiveFile()
{
	TSonorkShortString  archive_path;
	long int fsize;
	GetCtrlText( IDC_DBMAINT_SAVPATH , archive_path );
	arch_file = fopen( archive_path.CStr() , "r+b");
	if( !arch_file )
		arch_file = fopen( archive_path.CStr() , "wb");
	if( !arch_file )
	{
		TSonorkTempBuffer tmp(SONORK_MAX_PATH+128);
		SonorkApp.LangSprintf(tmp.CStr(),GLS_MS_NOOPENFILE , archive_path.CStr());
		MessageBox(tmp.CStr(),GLS_DBMNT,MB_ICONERROR|MB_OK);
		return false;
	}
	else
	{
		SYSTEMTIME	st;
		fseek(arch_file,0,SEEK_END);
		fsize = ftell(arch_file);
		if(fsize<64)
			fprintf(arch_file,
			"Time,UsrId1,UsrId2,Alias,Text,SysFlags,UsrFlags,SvcId,SvcDescr,Track0,Track1,DataType,DataSize,Data\n"
			);
		SonorkApp.CurrentTime().GetTime(&st);
		fprintf(arch_file,"\"%04u/%02u/%02u %02u:%02u:%02u\""
						",0,0,\"SONORK\",\"\""
						",0,0,0,0,0,0,0,0,\"\"\n"
					,st.wYear
					,st.wMonth
					,st.wDay
					,st.wHour
					,st.wMinute
					,st.wSecond);
	}

	return true;
}

void
	CSV_Output(FILE *file, const char *src)
{
	for(;*src;src++)
	{
		if(*src=='"')
			fputc('"',file);
		else
		if(*src=='\r')
			continue;
		fputc(*src,file);
	}
}
void
	CSV_Dump(FILE *file, const BYTE *data, DWORD data_size)
{
	char tmp[8];
	while(data_size--)
	{
		fprintf(file,ultoa(*data,tmp,16));
		data++;
	}
}

void
	TSonorkDbMaintWin::ArchiveMsg()
{
	TSonorkExtUserData *UD;
	SYSTEMTIME	st;
	const	char	*pAlias;
	UD=SonorkApp.UserList().Get( msg.header.userId );
	if(!UD)
		pAlias = "?";
	else
		pAlias = UD->display_alias.CStr();
	msg.header.time.GetTime(&st);
	fprintf(arch_file,"\"%04u/%02u/%02u %02u:%02u:%02u\",%u,%u,\""
		,st.wYear
		,st.wMonth
		,st.wDay
		,st.wHour
		,st.wMinute
		,st.wSecond
		,msg.header.userId.v[0]
		,msg.header.userId.v[1]);
	CSV_Output(arch_file,pAlias);
	fprintf(arch_file,"\",\"");
	CSV_Output( arch_file, msg.ToCStr() );
	fprintf(arch_file,"\",%u,%u,%u,%u,%u,%u,%u,\""
		, msg.header.sysFlags
		, msg.header.usrFlags
		, msg.DataServiceId()
		, msg.DataServiceDescriptor()
		, msg.header.trackingNo.v[0]
		, msg.header.trackingNo.v[1]
		, msg.data.Type()
		, msg.data.DataSize());
	CSV_Dump(arch_file,msg.data.Buffer(),msg.data.DataSize());
	fprintf(arch_file,"\"\n");
}
void
	TSonorkDbMaintWin::CmdBrowse()
{
	TSonorkShortString  	archive_path;
	SONORK_C_STR		file_name;
	GetCtrlText( IDC_DBMAINT_SAVPATH , archive_path );
	file_name = (SONORK_C_STR)SONORK_IO_GetFileName(archive_path.CStr());


	if(!SonorkApp.GetSavePath(Handle()
		, archive_path
		, file_name
		, GLS_OP_STORE
		, szSonorkOpenSaveDir_Export
		, "csv"
		,OFN_EXPLORER
		 | OFN_LONGNAMES
		 | OFN_NOCHANGEDIR
		 | OFN_NOREADONLYRETURN
		 | OFN_HIDEREADONLY
		 | OFN_PATHMUSTEXIST))
		return;
	SetCtrlText( IDC_DBMAINT_SAVPATH , archive_path.CStr() );
 }

#define SECS_IN_A_DAY			((double)(60*60*24))
void
 TSonorkDbMaintWin::DoMaintenance()
{
	TSonorkDynData		DD;
	TSonorkTag		msg_tag;
	TSonorkTag		ext_tag;
	DWORD			ext_index;
	DWORD			index,items;
	TSonorkAtomDb		tmp_msg_db;
	TSonorkAtomDb		tmp_ext_db;
	bool			failure,db_reset;
	DWORD			prev_version;
	DWORD			max_msg_days,cur_msg_days;
	double  		t_delta;
	static UINT	show_ctrls[8]=
	{	IDC_DBMAINT_DAYS
	,	IDL_DBMAINT_DAYS
	,	IDC_DBMAINT_DELMSGS
	,	IDC_DBMAINT_SAVMSGS
	,	IDC_DBMAINT_SAVPATH
	,	IDC_DBMAINT_BROWSE
	,	IDC_DBMAINT_PB1|0x80000000
	,	IDL_DBMAINT_ITEM|0x80000000
	};


	arch_file = NULL;
	if(GetCtrlChecked( IDC_DBMAINT_DELMSGS ))
		max_msg_days = GetCtrlUint( IDC_DBMAINT_DAYS );
	else
		max_msg_days = 0;
	if(max_msg_days)
	{
		if( GetCtrlChecked( IDC_DBMAINT_SAVMSGS ) )
		{
			if(!OpenArchiveFile())return;
		}
	}
	SetCtrlEnabled(IDOK,false);
	SetCtrlEnabled(IDCANCEL,false);
	SetCtrlText(IDL_DBMAINT_ITEM,GLS_DBMNCPTING);
	for(index=0;index<8;index++)
		SetCtrlVisible(show_ctrls[index]&0xffff
			,show_ctrls[index]&0x80000000);
	TSonorkWin::SetStatus_PleaseWait(status_hwnd);
	UpdateWindow();
	prev_version = app_msg_db->GetValue(SONORK_APP_ATOM_DB_VALUE_VERSION);


	db_reset=true;
	tmp_msg_db.Open(SonorkApp.ProfileUserId(),p_tmp_msg_path,db_reset);
	db_reset=true;
	tmp_ext_db.Open(SonorkApp.ProfileUserId(),p_tmp_ext_path,db_reset);
	tmp_msg_db.SetValue(SONORK_APP_ATOM_DB_VALUE_VERSION,SONORK_APP_CURRENT_ATOM_DB_VERSION);
	tmp_ext_db.SetValue(SONORK_APP_ATOM_DB_VALUE_VERSION,SONORK_APP_CURRENT_ATOM_DB_VERSION);

//	sonork_printf("max_msg_days=%u",max_msg_days);
	failure=false;
	app_msg_db->RecomputeItems();
	app_ext_db->RecomputeItems();
	items = app_msg_db->Items();
	::SendMessage(pb1_hwnd,PBM_SETRANGE,0,MAKELPARAM(0, PROGRESS_SIZE(items) ));

	for(index=0;index<items;index++)
	{
		if( (index&0xff) == 0 )
			::SendMessage(pb1_hwnd,PBM_SETPOS,PROGRESS_SIZE(index),0);

		if(app_msg_db->Get(index,&msg,&msg_tag) != SONORK_RESULT_OK)
		{
			continue;
		}
		if( max_msg_days )
		{
			if(SonorkApp.CurrentTime().DiffTime(msg.header.time,&t_delta))
			if( t_delta > 0 )
			{
				cur_msg_days = (DWORD)(t_delta/SECS_IN_A_DAY);
//				sonork_printf("%05u: Days=%u/%u",index,cur_msg_days,max_msg_days);
				if( cur_msg_days > max_msg_days && cur_msg_days<1080)
				{
					if(arch_file)
						ArchiveMsg();
					continue;
				}
			}
		}
		ext_index = msg_tag.v[SONORK_DB_TAG_INDEX];
		if( prev_version > 0 || (prev_version == 0 && ext_index != 0) )
		{
			if( ext_index != SONORK_INVALID_INDEX )
			{
				if( app_ext_db->GetRaw(ext_index , DD , ext_tag) == SONORK_RESULT_OK )
					tmp_ext_db.AddRaw(DD,ext_tag,ext_index);
				else
					ext_index = SONORK_INVALID_INDEX;
			}
		}
		else
		{
			// version 0 did not store the EXT index in the MSG db
			ext_index = SONORK_INVALID_INDEX;
		}
		msg_tag.v[SONORK_DB_TAG_INDEX] = ext_index;
		if(tmp_msg_db.Add(&msg,NULL,&msg_tag)!=SONORK_RESULT_OK)
		{
			failure=true;
			break;
		}
	}
	if( arch_file )
	{
		fclose(arch_file);
	}
	tmp_msg_db.Close();
	tmp_ext_db.Close();
	if( failure )
	{
		ModifyDbName(p_tmp_msg_path , NULL , MODIFY_DB_NAME_DELETE);
		ModifyDbName(p_tmp_ext_path , NULL , MODIFY_DB_NAME_DELETE);
		ErrorBox(GLS_DBMNRBLDCC,NULL);
		Destroy(IDCANCEL);
	}
	else
	{
		SetWinUsrFlag( WORKING );

		app_msg_db->Close();
		app_ext_db->Close();
		ModifyDbName(p_tmp_msg_path , p_app_msg_path , MODIFY_DB_NAME_MOVE);
		ModifyDbName(p_tmp_ext_path , p_app_ext_path , MODIFY_DB_NAME_MOVE);
		db_reset=false;
		app_msg_db->Open(SonorkApp.ProfileUserId(),p_app_msg_path,db_reset);
		db_reset=false;
		app_ext_db->Open(SonorkApp.ProfileUserId(),p_app_ext_path,db_reset);

		user_no = 0;
		items 	= SonorkApp.UserList().Items();
		::SendMessage(pb1_hwnd,PBM_SETRANGE,0,MAKELPARAM(0, items) );


		TSonorkWin::SetStatus(status_hwnd
			, GLS_DBMNRBLDCC
			, SKIN_HICON_BUSY);
		SonorkApp.UserList().BeginEnum(I);

		SetAuxTimer( AUX_TIMER_MSECS );
	}
}

void
	TSonorkDbMaintWin::UpdateInterface()
{
	BOOL dm=GetCtrlChecked(IDC_DBMAINT_DELMSGS);
	static UINT	ctrls[3]=
	{	IDC_DBMAINT_DAYS
	,	IDL_DBMAINT_DAYS
	,	IDC_DBMAINT_SAVMSGS};
	for(int i=0;i<3;i++)
		SetCtrlEnabled(ctrls[i],dm);
	SetCtrlEnabled(IDC_DBMAINT_SAVPATH
		,dm&&GetCtrlChecked(IDC_DBMAINT_SAVMSGS));
}
void
	TSonorkDbMaintWin::OnAfterCreate()
{
	int 	index;
	char	tmp[24];
	HWND	tHwnd = GetDlgItem( IDC_DBMAINT_DAYS );
	SetCtrlChecked(IDC_DBMAINT_DELMSGS,true);
	SetCtrlChecked(IDC_DBMAINT_SAVMSGS,true);
	UpdateInterface();
	for(index=30;index<=360;index+=30)
		ComboBox_AddString(tHwnd,ultoa(index,tmp,10));
	ComboBox_SetCurSel(tHwnd, 11);
	if( SonorkApp.CxActiveOrPending() )
	{
		ErrorBox(GLS_MS_NCXOP,NULL);
		Destroy(IDCANCEL);
		return;
	}

	SonorkApp.ProfileUserId().GetStr(tmp);

	p_app_msg_path = app_msg_path.CStr();
	p_app_ext_path = app_ext_path.CStr();
	p_tmp_msg_path = tmp_msg_path.CStr();
	p_tmp_ext_path = tmp_ext_path.CStr();

	SonorkApp.GetDirPath( p_app_msg_path ,SONORK_APP_DIR_DATA , tmp);
	strcpy( p_app_ext_path , p_app_msg_path );
	index=strlen( p_app_msg_path );

	sprintf( p_app_msg_path+index
		, "\\%s-msg(%s)-arc.csv"
		, tmp
		, szSonorkAppMode);
	SetCtrlText(IDC_DBMAINT_SAVPATH,p_app_msg_path);

	sprintf( p_app_msg_path+index ,"\\%s-msg(%s)" , tmp , szSonorkAppMode);
	sprintf( p_tmp_msg_path, "%s-tmp", p_app_msg_path);

	sprintf( p_app_ext_path+index ,"\\%s-ext(%s)" , tmp , szSonorkAppMode);
	sprintf( p_tmp_ext_path, "%s-tmp", p_app_ext_path);

	status_hwnd	= GetDlgItem( IDC_DBMAINT_STATUS );
	pb1_hwnd	= GetDlgItem( IDC_DBMAINT_PB1 );
}

void
	TSonorkDbMaintWin::OnDestroy()
{

	/*
	new_msg_db = new TSonorkAtomDb;
	new_ext_db = new TSonorkAtomDb;
	delete new_msg_db;
	delete new_msg_db;
	*/
}
void
	TSonorkDbMaintWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{ {IDG_DBMAINT|SONORK_WIN_CTF_BOLD	, GLS_LB_OPTS		}
	, {IDC_DBMAINT_DELMSGS			, GLS_OP_DELOLDERT	}
	, {IDL_DBMAINT_DAYS			, GLS_LB_DAYS		}
	, {IDC_DBMAINT_SAVMSGS			, GLS_OP_ARCDELITMS	}
	, {IDOK|SONORK_WIN_CTF_BOLD		, GLS_OP_START		}
	, {IDCANCEL|SONORK_WIN_CTF_BOLD		, GLS_OP_CANCEL		}
	, {-1					, GLS_DBMNT		}
	, {0					, GLS_NULL		}
	};
	LoadLangEntries( gls_table, false );

}

bool
	TSonorkDbMaintWin::OnAppEvent(UINT event, UINT , void*data)
{
	if( event == SONORK_APP_EVENT_MAINTENANCE )
	{
		// return true if we requested to enter "Maintenance" mode
		// because if false is returned, this window is destroyed  
		return data == (void*)this;
	}
	return false;
}
