#if !defined(SONORK_APP_BASE_H)
#define SONORK_APP_BASE_H

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL) Version 1.

	You should have received a copy of the SSCL	along with this program;
	if not, write to sscl@sonork.com.

	You should NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL), doing so will indicate your agreement
	to the the terms which may be differ for each version of the software.

	This comment section, indicating the existence and requirement of
	acceptance of the SSCL may not be removed from the source code.
*/

#include "srk_event_handler.h"
#include "srk_bf_file.h"
#include "srk_ext_user_data.h"
#include "srk_client.h"




extern SONORK_C_CSTR	szUserData	;
extern SONORK_C_CSTR	szAuths		;
extern SONORK_C_CSTR	szUserGroups;
extern SONORK_C_CSTR	szAlias		;
extern SONORK_C_CSTR	szCtrlData	;
extern SONORK_C_CSTR	szOnlSound	;
extern SONORK_C_CSTR	szMsgSound	;
extern SONORK_C_CSTR	szHost		;
extern SONORK_C_CSTR	szNotes		;
extern SONORK_C_CSTR	szPassword	;
extern SONORK_C_CSTR	szData		;
extern SONORK_C_CSTR	szLang		;

enum SONORK_APP_BASE_SAVE_PROFILE_FLAGS
{
	SONORK_APP_BASE_SPF_SAVE_USER_DATA	= 0x00001
,	SONORK_APP_BASE_SPF_SAVE_PASSWORD	= 0x00002
,	SONORK_APP_BASE_SPF_SAVE_CTRL_DATA	= 0x00004
,	SONORK_APP_BASE_SPF_SAVE_SOUNDS		= 0x00008
,	SONORK_APP_BASE_SPF_SAVE_ALL		= 0x000ff

};
class TSonorkAppBase
:public TSonorkEventHandler
{
public:

private:
	TSonorkBfFile				bf_file;

protected:

	struct TSonorkExtAppData{
		TSonorkAppCtrlData		ctrl_data;
		TSonorkAppRunData		run_data;
		TSonorkVersion			version;
	}app;

	struct _PROFILE
	{
		bool				open;
		TSonorkProfileCtrlData		ctrl_data;
		TSonorkProfileRunData		run_data;
		TSonorkExtUserList    		user_list;
	}profile;


	struct TSonorkExtAppKeys{
		TSonorkBfBlock	 *    		app;
		TSonorkBfBlock   *    	        profile;
		TSonorkBfBlock   *    	        users;
	}bf_key;

	struct TSonorkExtAppSounds{
		TSonorkShortString	login;
		TSonorkShortString 	message;
	}sound;

	//virtual void SONORK_CALLBACK GutsEventHandler(void*,evGuts*);

	SONORK_RESULT	_OpenProfile(TSonorkId&, bool create, SONORK_C_CSTR password);
	void		_CloseProfile(bool save, bool gen_close_event);
	SONORK_RESULT	_LoadUserList();
	SONORK_RESULT	_SaveUserList();

private:

	void   	ProcessSonorkEvent(void*,TSonorkClientEvent*);
	void 	PSE_GlobalTask(SONORK_FUNCTION,const TSonorkError*);
	void 	PSE_StatusChange(SONORK_NETIO_STATUS,const TSonorkError*,DWORD);
	void	PSE_AddUser(TSonorkUserData*,const TSonorkAuth2*, const TSonorkDynString* , DWORD msg_sys_flags);
	void	PSE_SetUser(TSonorkUserData*,const TSonorkAuth2*,const TSonorkDynString*);
	void	PSE_UserSid(const TSonorkUserLocus3*
			,const TSonorkSerial*
			,const TSonorkVersion*
			,TSonorkDynString*);
	void	PSE_DelUser(const TSonorkId&);
	void	PSE_Monitor(BOOL idle);
	void	PSE_UserSynchEnd(const TSonorkId&, const TSonorkError*);
	void	PSE_LoadLoginRequest(TSonorkSvrLoginReqPacket*);

	virtual void
		OnSonorkStatusChange(SONORK_NETIO_STATUS,const TSonorkError*,DWORD){}

	virtual void
		OnSonorkUserAuth(TSonorkAuthReqData*){}

	virtual void
		OnSonorkAddUser(TSonorkExtUserData*, DWORD msg_sys_flags){}

	virtual void
		OnSonorkAddGroup( const TSonorkGroup* ){}

	virtual void
		OnSonorkSetUser(TSonorkExtUserData*,const TSonorkAuth2*,const TSonorkDynString*){}

	virtual void
		OnSonorkUserSid(TSonorkExtUserData*
			,const TSonorkUserLocus3&old_locus){}
	virtual void
		OnSonorkDelUser(const TSonorkId&,TSonorkExtUserData*){}

	virtual void
		OnSonorkGlobalTask(SONORK_FUNCTION,const TSonorkError*){}

	virtual void
		OnSonorkUserProfileOpen(bool open){}

	virtual void
		OnSonorkMsg(TSonorkMsg*){}

	virtual void
		OnSonorkCtrlMsg(const TSonorkUserLocus1*,const TSonorkCtrlMsg*,TSonorkDynData*){}

	virtual void
		OnSonorkMonitor(BOOL idle){}

	virtual SONORK_LOGIN_MODE
		OnSonorkLoadLoginRequest(DWORD& flags){return SONORK_LOGIN_MODE_INTRANET;}

	virtual void
		OnSonorkAddWapp( const TSonorkWappData*WD ){}

	virtual void
		OnSonorkSysMsg(DWORD index, TSonorkSysMsg*){}

protected:

	TSonorkClient 		sonork;
	TSonorkUserData		profile_user;
	TSonorkShortString  	profile_password;

	SONORK_RESULT  	_SaveUser(TSonorkExtUserData*UD);
	void		_MarkForRefresh(TSonorkExtUserData*UD,BOOL);

public:
	TSonorkAppBase();


// ---------------------------------------------------------------------------
// App profile management

	SONORK_RESULT
		OpenApp(  SONORK_C_CSTR dir
			, SONORK_C_CSTR config_file_name
			, DWORD 		app_version
			, DWORD			flags
			, SONORK_OS_TYPE        os_type
			, DWORD 		os_version);
	void
		CloseApp();

	SONORK_RESULT
		SaveAppCtrlData();

	TSonorkBfFile&
		ConfigFile()
		{ return bf_file;}

	TSonorkBfBlock*
		AppKey()
		{ return bf_key.app;}

	TSonorkBfBlock*
		ProfileKey()
		{ return bf_key.profile;}

	TSonorkBfBlock*
		UsersKey()
		{ return bf_key.users;}



// ---------------------------------------------------------------------------
// Global profile

	bool
		IsAppConfigOpen()	const
		{ return bf_key.app!=NULL; }

	TSonorkFlags&
		AppCtrlFlags()
		{ return app.ctrl_data.header.flags;	}

	TSonorkFlags&
		AppRunFlags()
		{ return app.run_data.flags;	}

	DWORD&
		AppCtrlValue(SONORK_APP_CTRL_VALUE v)
		{ return app.ctrl_data.header.value[v];}

	DWORD&
		AppRunValue(SONORK_APP_RUN_VALUE v)
		{ return app.run_data.value[v];}

	const TSonorkVersion&
		Version() const
		{ return app.version; }

	DWORD
		ServerVersionNumber()
		{ return sonork.ServerVersionNumber(); }
// ---------------------------------------------------------------------------
// User Profile

// Management

	SONORK_RESULT
		CreateProfile(TSonorkId& user_id
				, SONORK_C_CSTR alias
				, SONORK_C_CSTR pass);

	SONORK_RESULT
		OpenProfile(TSonorkId& user_id,SONORK_C_CSTR password);

	SONORK_RESULT
		DelProfile(TSonorkId& user_id,SONORK_C_CSTR password);

	SONORK_RESULT
		SaveCurrentProfile(DWORD flags); //use SONORK_APP_BASE_SAVE_PROFILE_FLAGS

	SONORK_RESULT
		GetProfileInfo(TSonorkUserData&UD,TSonorkProfileCtrlData*CD);

	// GuId in str
	SONORK_RESULT
		GetProfileInfo(SONORK_C_CSTR gu_id_str
							,TSonorkUserData*UD
							,TSonorkProfileCtrlData*CD);
	void
		CloseProfile()
		{ _CloseProfile(true,true);}

public:

// Information

	bool
		IsProfileOpen()	const
		{ return profile.open; }

	const TSonorkId&
		ProfileUserId() const
		{ return profile_user.userId;}

	const TSonorkSid&
		ProfileSid() 	const
		{ return profile_user.addr.sid;}

	const TSonorkShortString&
		ProfileAlias() 	const
		{ return profile_user.alias;}

	TSonorkShortString&
		wProfileAlias()
		{ return profile_user.alias;}

	const TSonorkRegion&
		ProfileRegion() const
		{ return profile_user.Region();}

	TSonorkRegion&
		wProfileRegion()
		{ return profile_user.wRegion();}

	const TSonorkUserData&
		ProfileUser() 	const
		{ return profile_user;}

	TSonorkUserData&
		wProfileUser()
		{ return profile_user;}

	const TSonorkShortString&
		ProfileUserAlias() const
		{ return profile_user.alias;}

	const 	TSonorkShortString&
		ProfilePassword() const
		{ return profile_password;}

	TSonorkShortString&
		wProfilePassword()
		{ return profile_password;}

	TSonorkFlags&
		ProfileCtrlFlags()
		{ return profile.ctrl_data.header.flags;	}

	TSonorkFlags&
		ProfileRunFlags()
		{ return profile.run_data.flags;	}

	DWORD&
		ProfileCtrlValue(SONORK_PROFILE_CTRL_VALUE v)
		{ return profile.ctrl_data.header.value[v];}

	DWORD&
		ProfileRunValue(SONORK_PROFILE_RUN_VALUE v)
		{ return profile.run_data.value[v];}

const	TSonorkShortString&
		ProfileServerProfile() const
		{ return profile.ctrl_data.serverProfile;}

	TSonorkShortString&
		wProfileServerProfile()
		{ return profile.ctrl_data.serverProfile;}

	// UserSidFlags() are the local sid flags, the TSonorkClient updates
	// the user.address and user.user_info->region flags
	SONORK_SID_MODE
		ProfileSidMode()	const
		{ return profile.ctrl_data.header.sidFlags.SidMode();	}

const TSonorkSidFlags&
		ProfileSidFlags()		const
		{ return profile.ctrl_data.header.sidFlags;	}

	TSonorkSidFlags&
		wProfileSidFlags()
		{ return profile.ctrl_data.header.sidFlags;	}


	// ConfigFile I/O

	SONORK_RESULT
		ReadProfileItem(SONORK_C_CSTR name, TSonorkCodecAtom*A);

	SONORK_RESULT
		ReadProfileItem(SONORK_C_CSTR name, TSonorkShortString*S);

	// ReadProfileItem SONORK_C_STR reads a TSonorkShortString
	SONORK_RESULT
		ReadProfileItem(SONORK_C_CSTR name, SONORK_C_STR buffer, DWORD buffer_size);

	SONORK_RESULT
		WriteProfileItem(SONORK_C_CSTR name, const TSonorkCodecAtom*A);

	SONORK_RESULT
		WriteProfileItem(SONORK_C_CSTR name, const TSonorkShortString*S);

	SONORK_RESULT
		DelProfileItem(SONORK_C_CSTR name, SONORK_BF_BLOCK_TYPE);

	SONORK_RESULT
		ReadProfileSubItem(SONORK_C_CSTR key,  SONORK_C_CSTR name, TSonorkCodecAtom*A);

	SONORK_RESULT
		ReadProfileSubItem(SONORK_C_CSTR key,  const SONORK_DWORD2&, TSonorkCodecAtom*A);

	SONORK_RESULT
		ReadProfileSubItem(SONORK_C_CSTR key,  SONORK_C_CSTR name, TSonorkShortString*S);

	SONORK_RESULT
		WriteProfileSubItem(SONORK_C_CSTR key, SONORK_C_CSTR name, const TSonorkCodecAtom*A);

	SONORK_RESULT
		WriteProfileSubItem(SONORK_C_CSTR key,  const SONORK_DWORD2&, const TSonorkCodecAtom*A);

	SONORK_RESULT
		WriteProfileSubItem(SONORK_C_CSTR key, SONORK_C_CSTR name, const TSonorkShortString*S);

	SONORK_RESULT
		DelProfileSubItem(SONORK_C_CSTR key, SONORK_C_CSTR name , SONORK_BF_BLOCK_TYPE);

	SONORK_RESULT
		DelProfileSubItem(SONORK_C_CSTR key, const SONORK_DWORD2& , SONORK_BF_BLOCK_TYPE);

// User

	TSonorkExtUserData*
		GetUser(const TSonorkId& user_id)
		{ return profile.user_list.Get(user_id);}

const TSonorkExtUserList&
		UserList()
		{	return profile.user_list; }

	SONORK_RESULT
		LoadUser(TSonorkExtUserData*);

	SONORK_RESULT
		SaveUser(const TSonorkId&);

	TSonorkExtUserData*
		AddRemoteUser(const TSonorkId&,SONORK_C_CSTR,const TSonorkAuth2*);

	void
		MarkForRefresh(TSonorkExtUserData*UD)
		{ _MarkForRefresh(UD,true);}

	SONORK_RESULT
		LoadUserNotes(const TSonorkId&
				,TSonorkDynString*
				,TSonorkExtUserData::NOTES_TYPE type);

	SONORK_RESULT
		SaveUserNotes(const TSonorkId&
				,const TSonorkDynString*
				,TSonorkExtUserData::NOTES_TYPE type);

	SONORK_RESULT
		ReadUserItem(const TSonorkId&, SONORK_C_CSTR name, TSonorkCodecAtom*A);

	SONORK_RESULT
		WriteUserItem(const TSonorkId&, SONORK_C_CSTR name, const TSonorkCodecAtom*A);

	SONORK_RESULT
		DelUser(const TSonorkId&, bool generate_del_event);

// Auths

	SONORK_RESULT
		LoadAuthReq(TSonorkAuthReqData*);

	SONORK_RESULT
		SaveAuthReq(const TSonorkAuthReqData*);

	SONORK_RESULT
		DelAuthReq(const TSonorkId&);

	SONORK_RESULT
		LoadAuthReqList(TSonorkAuthReqDataQueue&,bool incomming, bool outgoing);

// User Groups

	SONORK_RESULT
		SaveUserGroup(const TSonorkGroup*);

	SONORK_RESULT
		DelUserGroup(DWORD no); // if no=0 all groups are deleted

	SONORK_RESULT
		LoadUserGroups( TSonorkGroupQueue& );


// Server profile
protected:
	SONORK_RESULT
		_LoadServerProfile(SONORK_C_CSTR name, TSonorkClientServerProfile& prof);

	SONORK_RESULT
		_SaveServerProfile(SONORK_C_CSTR name, const TSonorkClientServerProfile& prof);
public:

	SONORK_RESULT
		DeleteServerProfile(SONORK_C_CSTR name);

// Misc

	void
		EncodePin64(SONORK_DWORD4& 		tgt_pin
					,const TSonorkId&	tgt_guid
					,SONORK_SERVICE_ID  tgt_service_id
					,DWORD			tgt_service_pin);

	DWORD
		GenTrackingNo(const TSonorkId& target)	const
		{ return SONORK_GenTrackingNo(profile_user.userId,target);}

	DWORD
		GenSelfTrackingNo()	const
		{ return SONORK_GenTrackingNo(profile_user.userId,profile_user.userId);}

protected:
	TSonorkExtUserList&
		wUserList()
		{	return profile.user_list; }


};



#endif
