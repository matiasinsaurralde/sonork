#if !defined(SONORK_TASK_ATOM_H)
#define SONORK_TASK_ATOM_H
#include "srk_data_types.h"
#include "srk_pairvalue.h"

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


#define SONORK_REMIND_F_ALARM			0x01000000
#define SONORK_REMIND_F_RECURRENT		0x02000000
#define SONORK_REMIND_F_PENDING			0x04000000
#define SONORK_REMIND_ALARM_MOD_DELAY_MIN	0x000003ff
#define SONORK_REMIND_ALARM_MOD_DAYS   		0x000ff000
#define SONORK_REMIND_ALARM_MOD_LAST_DAY	0x0ff00000
#define SONORK_REMIND_ALARM_MOD_DAY_01		0x00001000
#define SONORK_REMIND_ALARM_MOD_DAY_02		0x00002000
#define SONORK_REMIND_ALARM_MOD_DAY_03		0x00004000
#define SONORK_REMIND_ALARM_MOD_DAY_04		0x00008000
#define SONORK_REMIND_ALARM_MOD_DAY_05		0x00010000
#define SONORK_REMIND_ALARM_MOD_DAY_06		0x00020000
#define SONORK_REMIND_ALARM_MOD_DAY_07		0x00030000


struct TSonorkRemindDataHeader
{
	TSonorkId	owner;
	SONORK_DWORD2	key;
	TSonorkTime	time;
	DWORD		reserved_01;
	DWORD		alarm_mod;
	TSonorkTime	alarm_time;
	DWORD		flags;
	DWORD		reserved_2[4];

	bool equ(const TSonorkRemindDataHeader& O) const
			{return owner==O.owner && key==O.key;}

	bool equ(const TSonorkRemindDataHeader* O) const
			{return owner==O->owner && key==O->key;}

	BOOL		IsAlarmOn()	const
			{ return flags&SONORK_REMIND_F_ALARM;}

	BOOL		IsPending()	const
			{ return flags&SONORK_REMIND_F_PENDING;}

	BOOL		IsRecurrent()	const
			{ return flags&SONORK_REMIND_F_RECURRENT;}

	UINT		GetAlarmDelayMins()	const
			{ return alarm_mod&SONORK_REMIND_ALARM_MOD_DELAY_MIN;}

	void		SetAlarmDelayMins(UINT d)
			{  alarm_mod=
				(alarm_mod&~SONORK_REMIND_ALARM_MOD_DELAY_MIN)
				|(d&SONORK_REMIND_ALARM_MOD_DELAY_MIN);
			}
}__SONORK_PACKED;



struct TSonorkRemindData
:public TSonorkCodecAtom
{

public:
	TSonorkRemindDataHeader	header;
	TSonorkDynString	title;
	TSonorkDynString	notes;
	TSonorkPairValueQueue 	values;

	bool equ(const TSonorkRemindData& O) const
			{return header.equ(O.header);}

	bool equ(const TSonorkRemindData* O) const
			{return header.equ(O->header);}

	bool equ(const TSonorkRemindDataHeader& HDR) const
			{return header.equ(HDR);}

	bool equ(const TSonorkRemindDataHeader* pHDR) const
			{return header.equ(pHDR);}

	BOOL	IsPending()	const
		{ return header.IsPending();}

	BOOL
		IsAlarmOn()	const
		{ return header.IsAlarmOn();}

	BOOL
		IsRecurrent()	const
		{ return header.IsRecurrent();}

	DWORD
		Flags() 	const
		{ return header.flags;}

	UINT
		GetAlarmDelayMins()	const
		{ return header.GetAlarmDelayMins();}

	void	SetAlarmDelayMins(UINT d)
		{  header.SetAlarmDelayMins(d);	}

const	TSonorkTime&
		RemindTime()	const
		{ return header.time;}

	TSonorkTime&
		wRemindTime()
		{ return header.time;}

const	TSonorkTime&
		AlarmTime() const
		{ return header.alarm_time;}

	TSonorkTime&
		wAlarmTime()
		{ return header.alarm_time;}


	void
		Clear();
// ----------
// CODEC

public:

	void 	CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType() const
		{ return SONORK_ATOM_REMIND_DATA; }

private:

	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void	CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD	CODEC_DataSize() const;
};

struct TSonorkTaskDataHeader
{
	TSonorkId	gen_gu_id;
	TSonorkTime	gen_time;
	TSonorkId	sgn_gu_id;
	TSonorkTime	sgn_time;
	SONORK_DWORD2   tracking_no;
	TSonorkFlags	flags;
	DWORD		type;			// Type
	DWORD		stat_prio;		// Status & Priority
	DWORD		reserved[3];
}__SONORK_PACKED;

struct TSonorkTaskData
:public TSonorkCodecAtom
{
public:
	TSonorkTaskDataHeader	header;
	TSonorkDynString	notes;
	TSonorkPairValueQueue 	values;

// ----------
// CODEC

public:

	void 	CODEC_Clear();

	SONORK_ATOM_TYPE
			CODEC_DataType() const
					{ return SONORK_ATOM_TASK_DATA; }

private:
	void	CODEC_WriteDataMem	(TSonorkCodecWriter&) const;
	void	CODEC_ReadDataMem	(TSonorkCodecReader&);
	DWORD	CODEC_DataSize() const;
};

#endif