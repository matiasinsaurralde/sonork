#if !defined(SRK_EXT_USER_DATA_H)
#define SRK_EXT_USER_DATA_H


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

#if !defined(SRK_DATA_TYPES_H)
# include "srk_data_types.h"
#endif

enum SONORK_USER_TYPE
{
  SONORK_USER_TYPE_UNKNOWN		= 0x01000
, SONORK_USER_TYPE_AUTHORIZED		= 0x00000
, SONORK_USER_TYPE_NOT_AUTHORIZED	= 0x00001
, SONORK_USER_TYPE_MASK   		= 0x0ffff
, SONORK_USER_TYPE_ALL			= SONORK_USER_TYPE_MASK
};


enum SONORK_USER_CTRL_VALUE
{
    SONORK_UCV_UNREAD_MSG_COUNT
,   SONORK_UCV_FIRST_UNREAD
,   SONORK_UCV_1
,   SONORK_UCV_2
,   SONORK_UCV_3
,   SONORK_UCV_4
,   SONORK_UCV_5
,   SONORK_UCV_6
,   SONORK_USER_CTRL_VALUES
};
enum SONORK_USER_CTRL_FLAG
{
  SONORK_UCF_1				=0x00000010
, SONORK_UCF_2				=0x00000020
, SONORK_UCF_3				=0x00000040
, SONORK_UCF_4				=0x00000080
, SONORK_UCF_5				=0x00000100
, SONORK_UCF_6				=0x00000200
, SONORK_UCF_7              	   	=0x00000400
, SONORK_UCF_8                 		=0x00000800
, SONORK_UCF_9          	       	=0x00000100
, SONORK_UCF_10        	        	=0x00000200
, SONORK_UCF_11        	        	=0x00000400
, SONORK_UCF_12        	        	=0x00000800
, SONORK_UCF_13        	        	=0x00001000
, SONORK_UCF_14        	        	=0x00002000
, SONORK_UCF_15        	        	=0x00004000
, SONORK_UCF_16       	         	=0x00008000
, SONORK_UCF_RESERVED_01	 	=0x00000011	//ALT_ONL_SOUND
, SONORK_UCF_RESERVED_02	 	=0x00000021	//ALT_MSG_SOUND
, SONORK_UCF_MUT_ONL_SOUND 		=0x00000041
, SONORK_UCF_MUT_MSG_SOUND 		=0x00000081
, SONORK_UCF_FILE_NO_DIR     		=0x00000101
, SONORK_UCF_FILE_NO_READ    		=0x00000201
, SONORK_UCF_FILE_NO_WRITE   		=0x00000401
, SONORK_UCF_FILE_NO_DELETE  		=0x00000801
, SONORK_UCF_FILE_AUTO_ACCEPT		=0x00001001
, SONORK_UCF_FILE_NO_SHARING 		=0x00002001
, SONORK_UCF_SONORK_UTS_DISABLED  	=0x00100001
, SONORK_UCF_SONORK_UTS_NETMATCH	=0x00200001
, SONORK_UCF_SONORK_UTS_NO_AUTO		=0x00400001
, SONORK_UCF_SONORK_UTS_NO_CRYPT	=0x00800001
, SONORK_UCF_SONORK_UTS_NO_REVFW	=0x01000001
, SONORK_UCF_SONORK_UTS_USE_REVFW	=0x02000001
, SONORK_UCF_SONORK_UTS_ADFILES		=0x04000001
, SONORK_UCF_SONORK_UTS_AAFILES		=0x08000001
, SONORK_UCF_SERVER_SYNCH_PENDING	=0x10000001
, SONORK_UCF_SERVER_SYNCHING		=0x20000001
, SONORK_UCF_SERVER_SYNCH_ALIAS		=0x40000001	// Overwrite alias when SYNCH is done
, SONORK_UCF_SERVER_SYNCH_SKIP		=0x80000001
};
enum SONORK_USER_RUN_VALUE
{
    SONORK_URV_1
,   SONORK_URV_2
,   SONORK_URV_3
,   SONORK_URV_4
,   SONORK_URV_5
,   SONORK_URV_PROCESSED_SID_SEQ_NO
,   SONORK_URV_CURRENT_SID_SEQ_NO
,   SONORK_URV_REFRESH_FAIL_COUNT
,   SONORK_USER_RUN_VALUES
};
enum SONORK_USER_RUN_FLAG
{
  SONORK_URF_1	  			= 0x00000010
, SONORK_URF_2  			= 0x00000020
, SONORK_URF_3  			= 0x00000040
, SONORK_URF_4  			= 0x00000080
, SONORK_URF_5  			= 0x00000100
, SONORK_URF_6  			= 0x00000200
, SONORK_URF_7  			= 0x00000400
, SONORK_URF_8  			= 0x00000800
, SONORK_URF_9  			= 0x00001000
, SONORK_URF_10 			= 0x00002000
, SONORK_URF_11 			= 0x00004000
, SONORK_URF_12 			= 0x00008000
, SONORK_URF_13 			= 0x00010000
, SONORK_URF_14 			= 0x00020000
, SONORK_URF_15 			= 0x00040000
, SONORK_URF_16 			= 0x00080000
, SONORK_URF_17 			= 0x00100000
, SONORK_URF_18 			= 0x00200000
, SONORK_URF_19 			= 0x00400000
, SONORK_URF_20 			= 0x00800000
, SONORK_URF_21 			= 0x01000000
, SONORK_URF_22 			= 0x02000000
, SONORK_URF_23 			= 0x04000000
, SONORK_URF_24				= 0x08000000
, SONORK_URF_25				= 0x10000000
, SONORK_URF_26 			= 0x20000000
, SONORK_URF_27 			= 0x40000000
, SONORK_URF_REFRESH_MARK		= 0x80000000   // All users are marked before a full refresh
, SONORK_URF_DELETED			= 0x10000001
, SONORK_URF_CONSOLE_INIT		= 0x20000001
, SONORK_URF_INVALID_GROUP		= 0x40000001
};
struct TSonorkExtUserRunData
{
	TSonorkFlags	    	flags;
	UINT			value[SONORK_USER_RUN_VALUES];
	SONORK_UTS_LINK_ID	uts_link_id;
}__SONORK_PACKED;

struct  TSonorkExtUserCtrlData
{
	TSonorkFlags		flags;
	UINT			value[SONORK_USER_CTRL_VALUES];
	TSonorkAuth2		auth;
}__SONORK_PACKED;

struct  TSonorkExtUserSoundData
{
	TSonorkShortString	online;
	TSonorkShortString	message;
}__SONORK_PACKED;

struct TSonorkExtUserData
:public TSonorkUserData
{
private:
	SONORK_USER_TYPE	user_type;
public:
	enum NOTES_TYPE
	{
		LOCAL_NOTES
	,	REMOTE_NOTES
	,	NOTES_TYPE_COUNT
	};


	TSonorkShortString	display_alias;
	TSonorkExtUserSoundData	sound;
	TSonorkExtUserCtrlData 	ctrl_data;
	TSonorkExtUserRunData	run_data;
	TSonorkDynString	sid_text;

	TSonorkExtUserData(SONORK_USER_TYPE type);

	SONORK_USER_TYPE
		UserType() const
		{ return user_type; }
		
	bool	IsValidUserType() const;

	void	SetUserType(SONORK_USER_TYPE t)
		{ user_type = t; }

	TSonorkFlags&
		CtrlFlags()
		{return ctrl_data.flags;}

	TSonorkFlags&
		RunFlags()
		{return run_data.flags;}

	BOOL
		TestCtrlFlag(SONORK_USER_CTRL_FLAG f) const
		{ return ctrl_data.flags.Test(f);}

	void
		SetCtrlFlag(SONORK_USER_CTRL_FLAG f)
		{ CtrlFlags().Set(f);}

	void
		ClearCtrlFlag(SONORK_USER_CTRL_FLAG f)
		{ CtrlFlags().Clear(f);}

	void	SetClearCtrlFlag(SONORK_USER_CTRL_FLAG f,BOOL s)
		{ CtrlFlags().SetClear(f,s);}

	UINT&
		CtrlValue(SONORK_USER_CTRL_VALUE v)
		{return ctrl_data.value[v];}
	void
		ClearCtrlData()
		{SONORK_ZeroMem(&ctrl_data,sizeof(ctrl_data));}

	BOOL
		TestRunFlag(SONORK_USER_RUN_FLAG f)   const
		{ return run_data.flags.Test(f);}

	void
		SetRunFlag(SONORK_USER_RUN_FLAG f)
		{ RunFlags().Set(f);}

	void
		ClearRunFlag(SONORK_USER_RUN_FLAG f)
		{ RunFlags().Clear(f);}

	void
		SetClearRunFlag(SONORK_USER_RUN_FLAG f,BOOL s)
		{ RunFlags().SetClear(f,s);}
	UINT&
		RunValue(SONORK_USER_RUN_VALUE v)
		{return run_data.value[v];}

	void
		ClearRunData();

	TSonorkAuth2&
		Auth()
		{return ctrl_data.auth;}

	DWORD
		GetVisualGroupNo() const
		{ return ctrl_data.auth.GetGroupNo();}

	DWORD&
		AuthTag()
		{return ctrl_data.auth.tag;}
	void
		ClearExtData();

	SONORK_UTS_LINK_ID
		UtsLinkId() 	 const
		{return run_data.uts_link_id;}

	bool	UtsLinkReady()  const
		{return run_data.uts_link_id!=0;}

	void
		GetUserHandle(TSonorkUserHandle*) const;

	SONORK_C_CSTR
		SidText() const
		{ return sid_text.CStr(); }
};
typedef TSonorkExtUserData*	TSonorkExtUserDataPtr;


#if SONORK_CODEC_LEVEL > 5
#define SONORK_EXT_USER_LIST_HASH_SIZE	8
#define SONORK_EXT_USER_LIST_HASH_MASK	0x7
DeclareSonorkQueueClass(TSonorkExtUserData);
class TSonorkExtUserList
{
private:
	TSonorkExtUserDataQueue	enum_queue;
	TSonorkExtUserDataQueue	hash_queue[SONORK_EXT_USER_LIST_HASH_SIZE];
	TSonorkExtUserDataPtr
		GetQueue(const SONORK_DWORD2&,DWORD*) const;

public:
	TSonorkExtUserList(){}
	~TSonorkExtUserList();

	DWORD
		Items() const
		{ return enum_queue.Items();}
	TSonorkExtUserDataPtr
		Add(const TSonorkExtUserDataPtr pD) ;

	TSonorkExtUserDataPtr
		Get(const SONORK_DWORD2&) const;

	bool
		Del(const SONORK_DWORD2&);

	void
		BeginEnum( TSonorkListIterator& I ) const;

	TSonorkExtUserDataPtr
		EnumNext( TSonorkListIterator& I ) const;

	void
		EndEnum( TSonorkListIterator& I ) const;

	void
		Clear();
};

struct TSrkExtUserDataAtom
:public TSonorkCodecAtom
{
public:
	TSonorkExtUserData	*UD;
	
	TSrkExtUserDataAtom(TSonorkExtUserData*pUD)
		{ UD=pUD;}

	SONORK_ATOM_TYPE
		CODEC_DataType() const
		{ return SONORK_ATOM_USER_DATA_RESERVED_1; }

	void 	CODEC_Clear();

private:
	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;
	void	CODEC_ReadDataMem(TSonorkCodecReader&);
	DWORD	CODEC_DataSize() const;
};


#endif

#endif