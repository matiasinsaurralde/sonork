#if !defined(SRKBICHO_H)
#define SRKBICHO_H

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

#if !defined(SRKWIN_H)
# include "srkwin.h"
#endif

#include "srkbicho_defs.h"


#define SONORK_ICON_SX	0
#define SONORK_ICON_SY	0
#define SONORK_ICON_SW	30
#define SONORK_ICON_SH	30

enum SONORK_SEQUENCE_ITEM_TYPE;



struct TSonorkSequenceItem
{
friend class TSonorkSequence;
private:
	SONORK_SEQUENCE_ITEM_TYPE	type;
	union{
	  UINT				frame;
	  SONORK_SEQUENCE		sequence;
	  int				item_no;
	}data;
	DWORD	param;
	TSonorkSequenceItem(){}
public:

	SONORK_SEQUENCE_ITEM_TYPE
		Type() const
		{ return type; }

	DWORD
		Param()	const
		{ return param;}

	SONORK_SEQUENCE
		Sequence() const
		{ return data.sequence;}

	UINT	Frame()	const
		{ return data.frame;}


};

class TSonorkSequence{
friend TSonorkBicho;
	int 			cur_item
				,max_items
				,items;
	SONORK_SEQUENCE 	type;
	TSonorkSequenceItem*	item;

	TSonorkSequence(SONORK_SEQUENCE p_type, int p_items);
	~TSonorkSequence();


	int
		GetItemNo() const
		{return cur_item;}

	void
		AddFrame(UINT,DWORD frames);

	void
		AddGotoItem(int item_no);

	void
		AddGotoSequence(SONORK_SEQUENCE);

	void
		AddCallSequence(SONORK_SEQUENCE);

	void
		Start(int no=0);

	bool
		NextItem();

	TSonorkSequenceItem *
		CurItem();
public:
	  SONORK_SEQUENCE
		Type()	const
		{return type;}

};

#define SEQUENCE_STACK_SIZE	10
struct TSonorkSequenceStackItem
{
	SONORK_SEQUENCE	sequence;
	int 		item_no;
};
#define	SONORK_ICON_F_DRAW_ENABLE	0x00001

class TSonorkBicho
:public TSonorkWin
{
	HDC		win_dc;

	HIMAGELIST	image_list;

	UINT		cur_gu_frame,
			scr_gu_frame;

	UINT		draw_count;
	int 		sequences;
	int 		sequence_stack_index;
	UINT		frame_count,
			sequence_count,
			idle_frame_count,
			idle_move_count;


	SONORK_SEQUENCE
			cur_sequence;

	TSonorkSequence*
			sequence[SONORK_SEQUENCES];

	TSonorkSequenceStackItem
			sequence_stack[SEQUENCE_STACK_SIZE];

	TSonorkSequence  *
		Sequence(SONORK_SEQUENCE S)
		{ return sequence[S];}

	UINT
		NextFrame();

	void
		SetSequence(SONORK_SEQUENCE,bool reset_idle, bool clear_stack,int start_item_no);

	void	_Draw(HDC);

	bool	OnBeforeCreate(TSonorkWinCreateInfo*);
	bool	OnCreate();
	void	OnBeforeDestroy();
	void 	OnPaint(HDC, RECT&, BOOL);
	bool 	OnEraseBG(HDC){return true;}
	void 	OnMouseMove(UINT keys,int x,int y);
	void 	OnLButtonDown(UINT keys,int x,int y);

public:
	TSonorkBicho(TSonorkWin*parent,HDC src_dc);
	~TSonorkBicho();

	void
		CallSequence(SONORK_SEQUENCE);

	void
		SetSequence( SONORK_SEQUENCE s, bool restart_if_already_set=false);

//   bool			SetBitmap(const char*);

	SONORK_SEQUENCE
		GetSequence() 	const
		{return cur_sequence;}
	UINT
		FrameCount() const
		{return frame_count;}

	UINT
		SequenceCount()	const
		{return sequence_count;}

	UINT
		IdleFrameCount() const
		{return idle_frame_count;}

	UINT
		DrawCount() const
		{return draw_count;}

	void
		ResetIdle();

	void
		TimeSlot();
};



#endif