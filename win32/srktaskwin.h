#if !defined(SRKTASKWIN_H)
#define SRKTASKWIN_H

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

#include "srkwin.h"



#if !defined(SONORK_CLIENT_H)
#	include "srk_client.h"
#endif

#if !defined(SONORK_UTS_H)
#	include "srk_uts.h"
#endif

enum SONORK_TASKWIN_FLAGS
{
  SONORK_TASKWIN_DEFAULT_FLAGS		= 0
, SONORK_TASKWIN_F_NO_ERROR_BOX		= 0x000001
, SONORK_TASKWIN_F_NO_UTS		= 0x000010
, SONORK_TASKWIN_F_UTS_ONLY		= 0x000020
};

enum SONORK_WIN_TASK_TYPE
{
  SONORK_WIN_TASK_NONE
, SONORK_WIN_TASK_SONORK
, SONORK_WIN_TASK_UTS
};

class TSonorkTaskWin
:public TSonorkWin
{

private:
	struct TSonorkTaskWinInfo
	{
		SONORK_WIN_TASK_TYPE	type;
		DWORD					id;
		UINT					flags;
	}current_task;
	void	OnCleanup();

	static	void SONORK_CALLBACK SonorkClientHandler(void*,TSonorkClientRequestEvent*);
	static	void SONORK_CALLBACK SonorkUtsHandler(void *param, const SONORK_DWORD2& packet_tag, const TSonorkUTSLink*, const TSonorkError*);

#define OnSonorkTaskStart		f1(){ return 1; }
#define OnSonorkTaskEnd			f2(){ return 1; }

	virtual void OnTaskStart(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&){}
	virtual void OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size){}
	virtual void OnTaskEnd(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&,const TSonorkError*){}

protected:

/*
	SONORK_RESULT	TaskSendMsgToUser(TSonorkError&ERR
		, TSonorkExtUserData*		UD
		, const TSonorkMsg&			msg
		, DWORD						task_flags
		, const SONORK_DWORD2*		tag);

	SONORK_RESULT	TaskSendMsgToLocus(TSonorkError&ERR
		, const TSonorkUserLocus1& 	locus
		, const TSonorkMsg&			msg
		, DWORD						task_flags
		, const SONORK_DWORD2*		tag
		, SONORK_UTS_LINK_ID 		link_id=0);
*/
public:
	TSonorkTaskWin(TSonorkWin*parent,UINT def_flags,UINT sys_flags);

	bool
		MayStartTask(TSonorkError*ERR=NULL,SONORK_WIN_TASK_TYPE t=SONORK_WIN_TASK_NONE)
			const;

	DWORD
		CurrentTaskId()		const
		{ return current_task.id ; }

	SONORK_WIN_TASK_TYPE
		CurrentTaskType()	const
		{	return current_task.type; 	}

	BOOL
		IsTaskPending() 	const
		{ return current_task.type != SONORK_WIN_TASK_NONE; }

	UINT	CurrentTaskFlags()	const
		{ return current_task.flags;}

	SONORK_RESULT	StartSonorkTask(TSonorkError&
				, TSonorkDataPacket*
				, UINT 			size
				, DWORD			task_flags
				, GLS_INDEX 		gls_task
				, const SONORK_DWORD2*	tag);

	SONORK_RESULT	StartUtsTask(TSonorkError&   ERR
				, SONORK_UTS_LINK_ID 	id
				, TSonorkUTSPacket*	P
				, UINT 			P_size
				, DWORD			task_flags
				, GLS_INDEX 		gls_task
				, const SONORK_DWORD2*	tag);

	SONORK_RESULT	StartUtsTask(TSonorkError&	ERR
				, SONORK_UTS_LINK_ID 	id
				, DWORD 		cmd
				, BYTE 			version
				, const BYTE*		data
				, UINT 			data_size
				, DWORD			task_flags
				, GLS_INDEX 		gls_task
				, const SONORK_DWORD2*	tag);

	SONORK_RESULT
		StartUtsTask(TSonorkError&	ERR
			, SONORK_UTS_LINK_ID 	id
			, DWORD 		cmd
			, BYTE 			version
			, const TSonorkCodecAtom*
			, DWORD			task_flags
			, GLS_INDEX 		gls_task
			, const SONORK_DWORD2*	tag);

// Cancel Task is equivalent to EndTask with ERR=USER_TERMINATION
	void			EndCurrentTask(TSonorkError&, bool invoke_handlers);
	void			CancelCurrentTask(bool invoke_handlers);

};


#endif