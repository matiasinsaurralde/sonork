#include "srkwin32app.h"
#pragma hdrstop
#include "srkbicho.h"

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

#if !defined(SONORK_APP_TIMER_MSECS)
#	error must defined SONORK_APP_TIMER_MSECS
#endif

#define DRAW_ENABLED		SONORK_WIN_F_USER_01


#define IDLE_BLINK_SECONDS	5
#define IDLE_MOVE_SECONDS	10
#define SLEEP_SECONDS		(IDLE_MOVE_SECONDS<<2)
#define SLEEP_MOVE_SECONDS	35
#define LOOK_SIDE_MSECS		1000
#define LOOK_CENTER_MSECS	(LOOK_SIDE_MSECS/2)

#define _GX(n)			((n)*SONORK_ICON_SW + SONORK_ICON_SX)
#define _GY(n)			((n)*SONORK_ICON_SH + SONORK_ICON_SY)
#define SONORK_ICON_ROWS	6
#define SONORK_ICON_ROW_SW	(19 * SONORK_ICON_SW)
#define	MASK_DY			(SONORK_ICON_ROWS*SONORK_ICON_SH)



#define C_MS(n)	  		(((UINT)(n))/SONORK_APP_TIMER_MSECS)
#define C_SEC(n)   		C_MS((n)*1000)
#define C_MIN(n)		C_SEC((n)*60)
#define C_UNIT(n)		C_MS(SONORK_APP_TIMER_MSECS*(n))


enum SONORK_SEQUENCE_ITEM_TYPE
{
  SONORK_SIT_NULL
, SONORK_SIT_FRAME
, SONORK_SIT_GOTO_ITEM
, SONORK_SIT_GOTO_SEQUENCE
, SONORK_SIT_CALL_SEQUENCE
, SONORK_SITS
};

enum SONORK_FRAME
{
  SONORK_FRAME_NULL				=-1

, SONORK_FRAME_NORMAL
, SONORK_FRAME_CLOSE_TWO_EYES
, SONORK_FRAME_CLOSE_ONE_EYE
, SONORK_FRAME_WAKEUP
, SONORK_FRAME_LOOK_UP
, SONORK_FRAME_LOOK_DOWN
, SONORK_FRAME_BOOK_CENTER
, SONORK_FRAME_BOOK_LEFT
, SONORK_FRAME_BOOK_RIGHT
, SONORK_FRAME_BOOK_TURNPAGE1
, SONORK_FRAME_BOOK_TURNPAGE2
, SONORK_FRAME_HAPPY
, SONORK_FRAME_SAINT
, SONORK_FRAME_A01
, SONORK_FRAME_A02
, SONORK_FRAME_A03
, SONORK_FRAME_A04
, SONORK_FRAME_A05
, SONORK_FRAME_A06

, SONORK_FRAME_ANGRY
, SONORK_FRAME_LOOK_LEFT
, SONORK_FRAME_LOOK_RIGHT
, SONORK_FRAME_YAWN1
, SONORK_FRAME_YAWN2
, SONORK_FRAME_MGLASS1
, SONORK_FRAME_MGLASS2
, SONORK_FRAME_MGLASS3
, SONORK_FRAME_MGLASS3_BLINK
, SONORK_FRAME_EMAIL1
, SONORK_FRAME_EMAIL2
, SONORK_FRAME_EMAIL3
, SONORK_FRAME_EMAIL4
, SONORK_FRAME_EMAIL5
, SONORK_FRAME_B01
, SONORK_FRAME_B02
, SONORK_FRAME_B03
, SONORK_FRAME_B04
, SONORK_FRAME_B05

, SONORK_FRAME_ZOOM1
, SONORK_FRAME_ZOOM2
, SONORK_FRAME_ZOOM3
, SONORK_FRAME_ZOOM4
, SONORK_FRAME_ZOOM5
, SONORK_FRAME_ZOOM5_BLINK
, SONORK_FRAME_UNUSED_1_1
, SONORK_FRAME_UNUSED_1_2
, SONORK_FRAME_UNUSED_1_3
, SONORK_FRAME_UNUSED_1_4
, SONORK_FRAME_UNUSED_1_5
, SONORK_FRAME_UNUSED_1_6
, SONORK_FRAME_UNUSED_1_7
, SONORK_FRAME_UNUSED_1_8
, SONORK_FRAME_UNUSED_1_9
, SONORK_FRAME_UNUSED_1_10
, SONORK_FRAME_UNUSED_1_11
, SONORK_FRAME_UNUSED_1_12
, SONORK_FRAME_UNUSED_1_13

, SONORK_FRAME_WORK1
, SONORK_FRAME_WORK2
, SONORK_FRAME_WORK3
, SONORK_FRAME_WORK4
, SONORK_FRAME_WORK5
, SONORK_FRAME_WINDOW1
, SONORK_FRAME_WINDOW2
, SONORK_FRAME_WINDOW3
, SONORK_FRAME_WINDOW4
, SONORK_FRAME_WINDOW5
, SONORK_FRAME_WINDOW6
, SONORK_FRAME_WINDOW6_BLINK
, SONORK_FRAME_UNUSED_2_1
, SONORK_FRAME_UNUSED_2_2
, SONORK_FRAME_UNUSED_2_3
, SONORK_FRAME_UNUSED_2_4
, SONORK_FRAME_UNUSED_2_5
, SONORK_FRAME_UNUSED_2_6
, SONORK_FRAME_UNUSED_2_7

, SONORK_FRAME_ERROR1
, SONORK_FRAME_ERROR2
, SONORK_FRAME_ERROR3
, SONORK_FRAME_CALL1
, SONORK_FRAME_CALL2
, SONORK_FRAME_RADAR1
, SONORK_FRAME_RADAR2
, SONORK_FRAME_RADAR3
, SONORK_FRAME_RADAR4
, SONORK_FRAME_RADAR5
, SONORK_FRAME_RADAR6
, SONORK_FRAME_RADAR7
, SONORK_FRAME_RADAR8
, SONORK_FRAME_RADAR9
, SONORK_FRAME_RADAR10
, SONORK_FRAME_RADAR11
, SONORK_FRAME_RADAR12
, SONORK_FRAME_UNUSED_3_1
, SONORK_FRAME_UNUSED_3_2

, SONORK_FRAME_F01
, SONORK_FRAME_F02
, SONORK_FRAME_F03
, SONORK_FRAME_F04
, SONORK_FRAME_F05
, SONORK_FRAME_F06
, SONORK_FRAME_F07
, SONORK_FRAME_F08
, SONORK_FRAME_F09
, SONORK_FRAME_F10
, SONORK_FRAME_F11
, SONORK_FRAME_F12
, SONORK_FRAME_F13
, SONORK_FRAME_F14
, SONORK_FRAME_F15
, SONORK_FRAME_F16
, SONORK_FRAME_F17
, SONORK_FRAME_F18
, SONORK_FRAME_F20
, SONORK_FRAMES
};

extern  int GuIconData[];


void TSonorkBicho::OnPaint(HDC t_dc , RECT& rect, BOOL)
{
	if( TestWinUsrFlag( DRAW_ENABLED ) )
		_Draw(t_dc);
	else
		::FillRect(t_dc,&rect, sonork_skin.Brush(SKIN_BRUSH_DIALOG) );
}
void	TSonorkBicho::TimeSlot()
{
	if( TestWinUsrFlag( DRAW_ENABLED ) )
	{
		cur_gu_frame = NextFrame();
		if(cur_gu_frame!=scr_gu_frame)
		{
			_Draw(win_dc);
		}
	}
}

void TSonorkBicho::_Draw(HDC t_dc)
{
	ImageList_Draw(image_list , cur_gu_frame , t_dc, 0, 0, ILD_NORMAL );
	draw_count++;
	scr_gu_frame = cur_gu_frame;
}

bool TSonorkBicho::OnBeforeCreate(TSonorkWinCreateInfo*CI)
{
	RECT rect;
	CI->style|=WS_VISIBLE;
	CI->style&=~WS_BORDER;
	rect.top	=
	rect.left	= 0;
	rect.right	= SONORK_ICON_SW;
	rect.bottom	= SONORK_ICON_SH;
	::AdjustWindowRectEx(&rect,CI->style,false,CI->ex_style);
	MoveWindow(0,0,rect.right,rect.bottom);
	return true;
}
void TSonorkBicho::OnMouseMove(UINT ,int ,int )
{
	::SetCursor( sonork_skin.Cursor(SKIN_CURSOR_HAND) );
}
void TSonorkBicho::OnLButtonDown(UINT,int,int)
{
	if(Parent())
		Parent()->PostMessage(WM_COMMAND
				,MAKEWPARAM(1,STN_CLICKED)
				,(LPARAM)Handle() );
}
bool TSonorkBicho::OnCreate()
{
	win_dc = GetDC();
	if( image_list != NULL )
		SetWinUsrFlag(DRAW_ENABLED);
	return true;
}
void	TSonorkBicho::OnBeforeDestroy()
{
	ClearWinUsrFlag(DRAW_ENABLED);
	ReleaseDC( win_dc );
}
void TSonorkBicho::SetSequence(SONORK_SEQUENCE S, bool restart_if_already_set)
{
	if( restart_if_already_set || S!=GetSequence())
		SetSequence(S,true,true,0);
}
void TSonorkBicho::SetSequence(SONORK_SEQUENCE S, bool reset_idle, bool clear_stack, int start_item_no)
{
	if(clear_stack)
		sequence_stack_index=0;
	cur_sequence = S;
	Sequence(cur_sequence)->Start(start_item_no);
	frame_count=sequence_count=0;
	if(reset_idle)
		idle_frame_count=idle_move_count=0;
}
void TSonorkBicho::ResetIdle()
{
	if(cur_sequence==SONORK_SEQUENCE_SLEEP)
		SetSequence(SONORK_SEQUENCE_WAKEUP,true,false,0);
	else
		idle_frame_count=0;
}
void TSonorkBicho::CallSequence(SONORK_SEQUENCE S)
{
	sequence_stack[sequence_stack_index].sequence=cur_sequence;
	sequence_stack[sequence_stack_index].item_no = Sequence(cur_sequence)->GetItemNo() + 1;
	sequence_stack_index++;
	SetSequence(S,false,false,0);
}



UINT TSonorkBicho::NextFrame()
{
	TSonorkSequenceItem *I;
	if(cur_sequence==SONORK_SEQUENCE_IDLE)
	{
		idle_frame_count++;
		if(sequence_count>=C_SEC(IDLE_MOVE_SECONDS))
		{
			if(idle_frame_count<C_SEC(SLEEP_SECONDS))
			{
				if(idle_frame_count<C_SEC(SLEEP_SECONDS-IDLE_MOVE_SECONDS))
				{
					idle_move_count++;
					if(idle_move_count&1)
						SetSequence(SONORK_SEQUENCE_LOOK_LR,false,false,0);
					else
						SetSequence(SONORK_SEQUENCE_LOOK_UPDN,false,false,0);
				}
				else
					SetSequence(SONORK_SEQUENCE_YAWN,false,false,0);
			}
			else
			{
				SetSequence(SONORK_SEQUENCE_SLEEP,false,false,0);
			}
		}
	}
	for(;;)
	{
		I=sequence[cur_sequence]->CurItem();
		switch(I->Type())
		{
			case SONORK_SIT_FRAME:
			if( frame_count >= I->Param() )
			{
				if(!sequence[cur_sequence]->NextItem())
				{
					if(sequence_stack_index)
					{
						sequence_stack_index--;
						SetSequence(
							sequence_stack[sequence_stack_index].sequence
							,false
							,false
							,sequence_stack[sequence_stack_index].item_no);
					}
					else
						SetSequence(SONORK_SEQUENCE_IDLE,false,false,0);
				}
				else
					frame_count=0;
				break;//goto process_item;
			}
			frame_count++;
			sequence_count++;
			return I->Frame();

			case SONORK_SIT_GOTO_SEQUENCE:
				SetSequence(I->Sequence(),false,true,0);
				break;//goto process_item;

			case SONORK_SIT_CALL_SEQUENCE:
				CallSequence(I->Sequence());
				break;//goto process_item;

			default:
				break;//goto process_item;
		}
	}
}


TSonorkSequence::TSonorkSequence(SONORK_SEQUENCE p_type, int p_items)
{
	type 		= p_type;
	max_items	= p_items;
	item 		= new TSonorkSequenceItem[max_items];
	items 		= cur_item = 0;
}
TSonorkSequence::~TSonorkSequence()
{
	if(item)
	{
		delete[] item;
		item=NULL;
	}
}
void TSonorkSequence::AddFrame(UINT frame,DWORD frame_count)
{
	(item+items)->type 		= SONORK_SIT_FRAME;
	(item+items)->data.frame= frame;
	(item+items)->param		= frame_count;
	items++;
}
void TSonorkSequence::AddGotoItem(int item_no)
{
	(item+items)->type = SONORK_SIT_GOTO_ITEM;
	(item+items)->data.item_no = item_no;
	items++;
}
void TSonorkSequence::AddGotoSequence(SONORK_SEQUENCE sequence)
{
	(item+items)->type = SONORK_SIT_GOTO_SEQUENCE;
	(item+items)->data.sequence = sequence;
	items++;
}
void TSonorkSequence::AddCallSequence(SONORK_SEQUENCE sequence)
{
	(item+items)->type = SONORK_SIT_CALL_SEQUENCE;
	(item+items)->data.sequence = sequence;
	items++;
}

void TSonorkSequence::Start(int no)
{
	if(no>=0&&no<items)
		cur_item=no;
	else
		cur_item=0;
}
TSonorkSequenceItem *TSonorkSequence::CurItem()
{
	return item+cur_item;
}
bool TSonorkSequence::NextItem()
{
	TSonorkSequenceItem *I;

	if(cur_item>=items-1)
		return false;

	cur_item++;

	for(;;)
	{
		I=(item+cur_item);
		if(I->type == SONORK_SIT_GOTO_ITEM)
		{
			cur_item = I->data.item_no;
			continue;//goto goto_item;
		}
		return true;
	}
}




/*int GuIconFrame[SONORK_FRAMES][3]=
{{SONORK_FRAME_NULL				,_GX(0)	,_GY(0)}
,{SONORK_FRAME_NORMAL			,_GX(0)	,_GY(0)}
,{SONORK_FRAME_CLOSE_TWO_EYES	,_GX(1)	,_GY(0)}
,{SONORK_FRAME_CLOSE_ONE_EYE	,_GX(2)	,_GY(0)}
,{SONORK_FRAME_WAKEUP			,_GX(3)	,_GY(0)}
,{SONORK_FRAME_LOOK_UP			,_GX(4)	,_GY(0)}
,{SONORK_FRAME_LOOK_DOWN		,_GX(5)	,_GY(0)}
,{SONORK_FRAME_BOOK_CENTER		,_GX(6)	,_GY(0)}
,{SONORK_FRAME_BOOK_LEFT		,_GX(7)	,_GY(0)}
,{SONORK_FRAME_BOOK_RIGHT		,_GX(8)	,_GY(0)}
,{SONORK_FRAME_BOOK_TURNPAGE1	,_GX(10),_GY(0)}
,{SONORK_FRAME_BOOK_TURNPAGE2	,_GX(9)	,_GY(0)}
,{SONORK_FRAME_ANGRY			,_GX(0)	,_GY(1)}
,{SONORK_FRAME_LOOK_LEFT	 	,_GX(1)	,_GY(1)}
,{SONORK_FRAME_LOOK_RIGHT 		,_GX(2)	,_GY(1)}
,{SONORK_FRAME_YAWN1			,_GX(3)	,_GY(1)}
,{SONORK_FRAME_YAWN2			,_GX(4)	,_GY(1)}
,{SONORK_FRAME_MGLASS1			,_GX(5)	,_GY(1)}
,{SONORK_FRAME_MGLASS2			,_GX(6)	,_GY(1)}
,{SONORK_FRAME_MGLASS3			,_GX(7)	,_GY(1)}
,{SONORK_FRAME_MGLASS3_BLINK	,_GX(8)	,_GY(1)}
,{SONORK_FRAME_ZOOM1			,_GX(0)	,_GY(2)}
,{SONORK_FRAME_ZOOM2			,_GX(1)	,_GY(2)}
,{SONORK_FRAME_ZOOM3			,_GX(2)	,_GY(2)}
,{SONORK_FRAME_ZOOM4			,_GX(3)	,_GY(2)}
,{SONORK_FRAME_ZOOM5			,_GX(4)	,_GY(2)}
,{SONORK_FRAME_ZOOM6			,_GX(5)	,_GY(2)}
,{SONORK_FRAME_ZOOM7			,_GX(6)	,_GY(2)}
,{SONORK_FRAME_ZOOM7_BLINK		,_GX(7)	,_GY(2)}
,{SONORK_FRAME_WORK1			,_GX(0)	,_GY(3)}
,{SONORK_FRAME_WORK2			,_GX(1)	,_GY(3)}
,{SONORK_FRAME_WORK3			,_GX(2)	,_GY(3)}
,{SONORK_FRAME_WORK4			,_GX(3)	,_GY(3)}
,{SONORK_FRAME_WORK5			,_GX(4)	,_GY(3)}
,{SONORK_FRAME_WINDOW1 			,_GX(5)	,_GY(3)}
,{SONORK_FRAME_WINDOW2 			,_GX(6)	,_GY(3)}
,{SONORK_FRAME_WINDOW3 			,_GX(7)	,_GY(3)}
,{SONORK_FRAME_WINDOW4 			,_GX(8)	,_GY(3)}
,{SONORK_FRAME_WINDOW5 			,_GX(9)	,_GY(3)}
,{SONORK_FRAME_WINDOW6 			,_GX(10),_GY(3)}
,{SONORK_FRAME_WINDOW6_BLINK	,_GX(11),_GY(3)}
,{SONORK_FRAME_EMAIL1			,_GX(9)	,_GY(1)}
,{SONORK_FRAME_EMAIL2			,_GX(10),_GY(1)}
,{SONORK_FRAME_EMAIL3			,_GX(11),_GY(1)}
,{SONORK_FRAME_EMAIL4			,_GX(12),_GY(1)}
,{SONORK_FRAME_EMAIL5			,_GX(13),_GY(1)}
,{SONORK_FRAME_ERROR1			,_GX(0)	,_GY(4)}
,{SONORK_FRAME_ERROR2			,_GX(1)	,_GY(4)}
,{SONORK_FRAME_ERROR3			,_GX(2)	,_GY(4)}
,{SONORK_FRAME_CALL1			,_GX(3)	,_GY(4)}
,{SONORK_FRAME_CALL2			,_GX(4)	,_GY(4)}
,{SONORK_FRAME_ICQ_ENTER1		,_GX(8)	,_GY(2)}
,{SONORK_FRAME_ICQ_ENTER2		,_GX(9)	,_GY(2)}
,{SONORK_FRAME_ICQ_ENTER3		,_GX(10),_GY(2)}
,{SONORK_FRAME_ICQ_CROSS1		,_GX(11),_GY(2)}
,{SONORK_FRAME_ICQ_CROSS2		,_GX(12),_GY(2)}
,{SONORK_FRAME_ICQ_EAT1			,_GX(13),_GY(2)}
,{SONORK_FRAME_ICQ_EAT2			,_GX(14),_GY(2)}
,{SONORK_FRAME_ICQ_EAT3			,_GX(15),_GY(2)}
,{SONORK_FRAME_ICQ_EAT4			,_GX(16),_GY(2)}
,{SONORK_FRAME_ICQ_EAT5			,_GX(17),_GY(2)}
,{SONORK_FRAME_SAINT			,_GX(12),_GY(0)}
,{SONORK_FRAME_RADAR1			,_GX(5)	,_GY(4)}
,{SONORK_FRAME_RADAR2			,_GX(6)	,_GY(4)}
,{SONORK_FRAME_RADAR3			,_GX(7)	,_GY(4)}
,{SONORK_FRAME_RADAR4			,_GX(8)	,_GY(4)}
,{SONORK_FRAME_RADAR5			,_GX(9)	,_GY(4)}
,{SONORK_FRAME_RADAR6			,_GX(10),_GY(4)}
,{SONORK_FRAME_RADAR7			,_GX(11),_GY(4)}
,{SONORK_FRAME_RADAR8			,_GX(12),_GY(4)}
,{SONORK_FRAME_RADAR9			,_GX(13),_GY(4)}
,{SONORK_FRAME_RADAR10 			,_GX(14),_GY(4)}
,{SONORK_FRAME_RADAR11 			,_GX(15),_GY(4)}
,{SONORK_FRAME_RADAR12 			,_GX(16),_GY(4)}
,{SONORK_LOGO_01         		,_GX( 0),_GY(5)}
,{SONORK_LOGO_02         		,_GX( 1),_GY(5)}
,{SONORK_LOGO_03         		,_GX( 2),_GY(5)}
,{SONORK_LOGO_04         		,_GX( 3),_GY(5)}
,{SONORK_LOGO_05         		,_GX( 4),_GY(5)}
,{SONORK_LOGO_06         		,_GX( 5),_GY(5)}
,{SONORK_LOGO_07         		,_GX( 6),_GY(5)}
,{SONORK_LOGO_08         		,_GX( 7),_GY(5)}
,{SONORK_LOGO_09         		,_GX( 8),_GY(5)}
,{SONORK_LOGO_10         		,_GX( 9),_GY(5)}
,{SONORK_LOGO_11         		,_GX(10),_GY(5)}
,{SONORK_LOGO_12         		,_GX(11),_GY(5)}
,{SONORK_LOGO_13         		,_GX(12),_GY(5)}
,{SONORK_LOGO_14         		,_GX(13),_GY(5)}
,{SONORK_LOGO_15         		,_GX(14),_GY(5)}
};
*/

int GuIconData[]={

		SONORK_SEQUENCE_IDLE
   ,	SONORK_SIT_FRAME			, SONORK_FRAME_NORMAL		  		, C_SEC(IDLE_BLINK_SECONDS)
   ,	SONORK_SIT_FRAME			, SONORK_FRAME_CLOSE_TWO_EYES 		, C_UNIT(1)
   ,	SONORK_SIT_GOTO_ITEM		, 0
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_SLEEP
   ,	SONORK_SIT_CALL_SEQUENCE	,	SONORK_SEQUENCE_YAWN
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_YAWN1				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_SEC(SLEEP_MOVE_SECONDS)
   ,	SONORK_SIT_GOTO_ITEM		,	C_UNIT(2)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_WAKEUP
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WAKEUP				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_NORMAL				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_UNIT(1)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_YAWN
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_YAWN1		  		, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_YAWN2		  		, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_YAWN1		  		, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_YAWN2		  		, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_YAWN1		  		, C_UNIT(1)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_UNIT(2)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_UNIT(2)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_LOOK_LR
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_LOOK_LEFT			, C_MS(LOOK_SIDE_MSECS)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_MS(LOOK_CENTER_MSECS)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_LOOK_RIGHT			, C_MS(LOOK_SIDE_MSECS)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_LOOK_UPDN
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_LOOK_UP			, C_MS(LOOK_SIDE_MSECS)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_NORMAL				, C_MS(LOOK_CENTER_MSECS)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_LOOK_DOWN			, C_MS(LOOK_SIDE_MSECS)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_ZOOM
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM1				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM2				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM3				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM4				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM5				, C_UNIT(3)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM5_BLINK		, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM5				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM4				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM3				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM2				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ZOOM1				, C_UNIT(1)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_READ
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_RIGHT			, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_CENTER		, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_LEFT			, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_RIGHT			, C_UNIT(8)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_CENTER		, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_LEFT			, C_UNIT(15)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_RIGHT			, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_CENTER		, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_LEFT			, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_TURNPAGE1		, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_BOOK_TURNPAGE2		, C_UNIT(1)
   ,	SONORK_SIT_GOTO_ITEM		, 0
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_SEARCHING
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS1			, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS2			, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3			, C_UNIT(15)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3_BLINK		, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3			, C_UNIT(15)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS2			, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS1			, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS2			, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3			, C_UNIT(15)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3_BLINK		, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3			, C_UNIT(15)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS2			, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3			, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3_BLINK		, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3			, C_UNIT(15)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3_BLINK		, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3			, C_UNIT(10)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS2			, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS1			, C_UNIT(10)
   ,	SONORK_SIT_GOTO_ITEM		,	C_UNIT(1)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_WORK
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WORK1				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WORK2				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WORK3				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WORK4				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WORK5				, C_UNIT(3)
   ,	SONORK_SIT_GOTO_ITEM		,	0
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_OPEN_WINDOW
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW1			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW1			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW2			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW3			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW4			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW5			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW6			, C_UNIT(6)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW6_BLINK		, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW6			, C_UNIT(8)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_CLOSE_WINDOW
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW6			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW5			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW4			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW3			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW2			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_WINDOW1			, C_UNIT(1)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_EMAIL
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_EMAIL1				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_EMAIL2				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_EMAIL3				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_EMAIL4				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_EMAIL5				, C_UNIT(1)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_ERROR
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ERROR1				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ERROR2				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ERROR3				, C_UNIT(3)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_ERROR1				, C_UNIT(2)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_CALL
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CALL1				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CALL2				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CALL1				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CALL2				, C_UNIT(3)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CALL1				, C_UNIT(2)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_SEARCH_START
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS1			, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS2			, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3			, C_UNIT(15)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3_BLINK		, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3			, C_UNIT(15)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_SEARCH_END
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS3			, C_UNIT(5)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS2			, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_MGLASS1			, C_UNIT(2)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_RADAR
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR1				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR2				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR3				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR4				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR5				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR6				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR7				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR8				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR9				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR10			, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR11			, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_RADAR12			, C_UNIT(2)
   ,	SONORK_SIT_GOTO_ITEM		,	0
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_LOOK_DN
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_LOOK_DOWN			, C_UNIT(8)
   ,	SONORK_SIT_NULL

   ,  SONORK_SEQUENCE_SURPRISED_LEFT
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_LOOK_LEFT			, C_UNIT(3)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_LOOK_LEFT			, C_UNIT(5)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_UNIT(2)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_UNIT(1)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_UNIT(1)
   ,	SONORK_SIT_NULL

   , SONORK_SEQUENCE_SURPRISED_RIGHT
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_LOOK_RIGHT 		, C_UNIT(3)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_LOOK_RIGHT 		, C_UNIT(5)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_UNIT(2)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_UNIT(2)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_UNIT(1)
   ,	SONORK_SIT_FRAME			, 	SONORK_FRAME_NORMAL				, C_UNIT(1)
   ,	SONORK_SIT_FRAME			,	SONORK_FRAME_CLOSE_TWO_EYES		, C_UNIT(1)
   ,	SONORK_SIT_NULL

   ,	SONORK_SEQUENCE_NULL
};


TSonorkBicho::TSonorkBicho(TSonorkWin*parent, HDC pSrcDC)
	:TSonorkWin(parent,SONORK_WIN_CLASS_NORMAL|SONORK_WIN_TYPE_NONE,0)
{
	int 	*ci,*si;
	int		i;
	SONORK_SEQUENCE			sequence_type;
	SONORK_SEQUENCE_ITEM_TYPE item_type;
	TSonorkSequence *S;


	ci=GuIconData;
	SONORK_ZeroMem(sequence,sizeof(sequence));
	sequences=0;
	sequence_stack_index=0;
	while(*ci!=SONORK_SEQUENCE_NULL)
	{
		sequence_type=(SONORK_SEQUENCE)*ci;
		ci++;
		si=ci;
		i=0;

		while((item_type=(SONORK_SEQUENCE_ITEM_TYPE)*si)!=SONORK_SIT_NULL)
		{
			si++;
			switch(item_type)
			{
				case SONORK_SIT_FRAME:
					si+=2;
				break;

				case SONORK_SIT_GOTO_ITEM:
				case SONORK_SIT_GOTO_SEQUENCE:
				case SONORK_SIT_CALL_SEQUENCE:
					si++;
				break;
			
				default:
					break;
			}
			i++;
		}

		S = sequence[sequence_type] = new TSonorkSequence(sequence_type,i);
		sequences++;

		while((item_type=(SONORK_SEQUENCE_ITEM_TYPE)*ci)!=SONORK_SIT_NULL)
		{
			ci++;
			switch(item_type)
			{
				case SONORK_SIT_FRAME:
					S->AddFrame((UINT)*ci,*(ci+1));
					ci+=2;
				break;

				case SONORK_SIT_GOTO_ITEM:
					S->AddGotoItem((UINT)*ci);
					ci++;
				break;

				case SONORK_SIT_GOTO_SEQUENCE:
					S->AddGotoSequence((SONORK_SEQUENCE)*ci);
					ci++;
				break;

				case SONORK_SIT_CALL_SEQUENCE:
					S->AddCallSequence((SONORK_SEQUENCE)*ci);
					ci++;
				break;

				default:
				break;
			}
		}
		ci++;
	}

	scr_gu_frame	= SONORK_FRAME_NULL;
	draw_count		= 0;
	SetSequence(SONORK_SEQUENCE_IDLE,true,false,0);


	win_dc	= NULL;

	if( pSrcDC != NULL )
	{
		int			icon_sy,mask_sy;
		HDC			temp_dc;
		HBITMAP 	icon_bm,mask_bm,prev_bm;
		image_list=ImageList_Create(SONORK_ICON_SW,SONORK_ICON_SH,ILC_COLORDDB|ILC_MASK,SONORK_FRAMES,0);

		temp_dc = CreateCompatibleDC(pSrcDC);
		icon_sy = SONORK_ICON_SY;
		mask_sy = icon_sy + MASK_DY;
		icon_bm=::CreateCompatibleBitmap(pSrcDC,SONORK_ICON_ROW_SW,SONORK_ICON_SH);
		mask_bm=::CreateCompatibleBitmap(pSrcDC,SONORK_ICON_ROW_SW,SONORK_ICON_SH);

		for(i=0;i<SONORK_ICON_ROWS;i++)
		{
			prev_bm = (HBITMAP)::SelectObject(temp_dc,icon_bm);
				::BitBlt(temp_dc
					,0,0,SONORK_ICON_ROW_SW,SONORK_ICON_SH
					,pSrcDC
					,SONORK_ICON_SX
					,icon_sy
					,SRCCOPY);
			::SelectObject(temp_dc,mask_bm);
				::BitBlt(temp_dc
					,0,0,SONORK_ICON_ROW_SW,SONORK_ICON_SH
					,pSrcDC
					,SONORK_ICON_SX
					,mask_sy
					,SRCCOPY);
			::SelectObject(temp_dc,prev_bm);

			ImageList_Add(image_list,icon_bm,mask_bm);
			//int rv=TRACE_DEBUG("ImageList_Add(%u)=%u",i,rv);

			icon_sy += SONORK_ICON_SH;
			mask_sy += SONORK_ICON_SH;
		}
		::DeleteObject(icon_bm);
		::DeleteObject(mask_bm);
		::DeleteDC( temp_dc );
		ImageList_SetBkColor(image_list
			,sonork_skin.Color(SKIN_COLOR_DIALOG,SKIN_CI_BG));
	}
	else
		image_list=NULL;

}


TSonorkBicho::~TSonorkBicho()
{

	for(sequences=0;sequences<SONORK_SEQUENCES;sequences++)
		if(sequence[sequences])
			delete sequence[sequences];
	if( image_list )
		ImageList_Destroy(image_list);
}

