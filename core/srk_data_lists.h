#if !defined(SRK_DATA_LISTS_H)
#define SRK_DATA_LISTS_H

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


#include "srk_defs.h"

#define SONORK_SIMPLE_DATA_LIST_DEFAULT_ALLOC			8

#define StartSonorkSimpleDataListClass(name,type,alloc_size)\
class name \
:public TSonorkSimpleDataList{\
public:\
	name():TSonorkSimpleDataList(sizeof(type),alloc_size){}\
	type*	AddItem(type* I)		{ return (type*)w_AddItem(I);}\
	type*	ItemPtr(DWORD item_no) const	{ return (type*)w_ItemPtr(item_no);}\
	type*	EnumNext(TSonorkListIterator&i)	const { return (type*)w_EnumNext(i);}

#define EndSonorkSimpleDataListClass	};
struct  TSonorkListIterator
{
	union {
		const void 	*ptr;
		DWORD   	index;
	}d;
	DWORD		param;
	void
		Reset()
		{ d.index=0; }
};


class  TSonorkSimpleDataList
{
	DWORD items;
	DWORD item_size;
	DWORD items_per_block;
	DWORD allocated_items;
	BYTE *list;
	void AllocBlocks(UINT no);
public:
	TSonorkSimpleDataList(DWORD item_size,DWORD items_per_block=SONORK_SIMPLE_DATA_LIST_DEFAULT_ALLOC);
	virtual ~TSonorkSimpleDataList();

	void	InitEnum(TSonorkListIterator&i) const {i.Reset();}
	DWORD	Items()	const { return items;}
	void  	_Clear();
	void	PrepareFor(UINT items);
	virtual void	Clear(){ _Clear(); }
	DWORD   ItemSize()	const { return item_size;}
	void*	w_AddItem(const void*);
	void*	w_ItemPtr(DWORD item_no) const;
	void*	w_EnumNext(TSonorkListIterator&) const;
	const BYTE*
		Buffer() const
		{ return list; }
};



// FOR INTERNAL USE: DO NOT USE THIS VALUE!!
//#define SONORK_QUEUE_PRIORITY_DELETED	((UINT)-1)

enum SONORK_QUEUE_ITEM_PRIORITY
{
	SONORK_QUEUE_PRIORITY_LOWEST	=0
,	SONORK_QUEUE_PRIORITY_LOW
,	SONORK_QUEUE_PRIORITY_BELOW_NORMAL
,	SONORK_QUEUE_PRIORITY_NORMAL
,	SONORK_QUEUE_PRIORITY_DEFAULT=SONORK_QUEUE_PRIORITY_NORMAL
,	SONORK_QUEUE_PRIORITY_ABOVE_NORMAL
,	SONORK_QUEUE_PRIORITY_HIGH
,	SONORK_QUEUE_PRIORITY_HIGHEST
};
#define DeclareSonorkQueueClass(type)\
class type##Queue \
:public TSonorkQueue{\
public:\
	bool	Add(type* I,SONORK_QUEUE_ITEM_PRIORITY priority=SONORK_QUEUE_PRIORITY_NORMAL)\
				{ return w_Add(I,priority);}\
	type*	Peek() const \
				{ return (type*)w_Peek();}\
	type*	PeekNo(UINT no) const \
				{ return (type*)w_PeekNo(no);}\
	type* 	RemoveFirst()\
				{return (type*)w_RemoveFirst();}\
	bool  	Remove(type*I)\
				{return w_Remove(I);}\
	bool  	Exists(type*I)\
				{return w_Exists(I);}\
	type*	EnumNext(TSonorkListIterator&i)	const { return (type*)w_EnumNext(i);}\
}

#define StartSonorkQueueClass(name,type)\
class name \
:public TSonorkQueue{\
public:\
	bool	Add(type* I,SONORK_QUEUE_ITEM_PRIORITY priority=SONORK_QUEUE_PRIORITY_NORMAL)\
				{ return w_Add(I,priority);}\
	type*	Peek() const \
				{ return (type*)w_Peek();}\
	type*	PeekNo(UINT no) const \
				{ return (type*)w_PeekNo(no);}\
	type* 	RemoveFirst()\
				{return (type*)w_RemoveFirst();}\
	bool  	Remove(type*I)\
				{return w_Remove(I);}\
	bool	Exists(type*I)\
				{return w_Exists(I);}\
	type*	EnumNext(TSonorkListIterator&i)	const { return (type*)w_EnumNext(i);}
#define EndSonorkQueueClass	}
#define	NoExtensions

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,4)
#endif

struct  TSonorkQueueItem
{
friend class TSonorkQueue;
private:
	TSonorkQueueItem	*prev,*next;
	UINT			priority;
public:
	void  *	data;

	const TSonorkQueueItem*
		PrevItem() const
		{return prev;}

	const TSonorkQueueItem*
		NextItem() const
		{return next;}

	UINT	Priority() const
		{return priority;}
};

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif

class  TSonorkQueue
{
	UINT 			items;
	TSonorkQueueItem 	*head,*tail;
	UINT			lock_count;

protected:

	void
		Lock()
		{ lock_count++; }

	void
		Unlock()
		{ lock_count--; }


public:
	TSonorkQueue();
	virtual ~TSonorkQueue();

	bool
		w_Add(void*data,SONORK_QUEUE_ITEM_PRIORITY priority=SONORK_QUEUE_PRIORITY_NORMAL);

	void*
		w_Peek() const;

	void*
		w_PeekNo(UINT no) const;

	void*
		w_RemoveFirst();

	bool
		w_Remove(void*data);
		
	bool
		w_Exists(void*data);

	bool
		w_RemoveItem(TSonorkQueueItem *);

	void*
		w_EnumNext(TSonorkListIterator&)	const;

// NOTE:
//  The TSonorkQueue should NOT be modified between calls to BeginEnum and EndEnum()
//  IOW: The Add,Remove,RemoveFirst() methods are forbidden while
//   iterating the queue. The application SHOULD ALWAYS CALL EndEnum()
//   after finishing the iteration

	void
		BeginEnum(TSonorkListIterator&I)  const ;

	void
		EndEnum(TSonorkListIterator&I)    const ;

	TSonorkQueueItem *
		FirstItem() const
		{ return head; }

	TSonorkQueueItem *
		LastItem() const
		{ return tail; }

	UINT
		Items() const
		{return items;}

	void
		_Clear();

	virtual void
		Clear(){ _Clear(); }
};

class TSonorkDWORDQueue
:public TSonorkQueue{
public:
	bool	Add(DWORD I,SONORK_QUEUE_ITEM_PRIORITY priority=SONORK_QUEUE_PRIORITY_NORMAL)
				{ return w_Add((void*)I,priority);}
	DWORD	Peek() const
				{ return (DWORD)w_Peek();}
	DWORD	PeekNo(UINT no) const
				{ return (DWORD)w_PeekNo(no);}
	DWORD	RemoveFirst()
				{return (DWORD)w_RemoveFirst();}
	bool  	Remove(DWORD v)
				{return (DWORD)w_Remove((void*)v);}
	DWORD	EnumNext(TSonorkListIterator&i)	const { return (DWORD)w_EnumNext(i);}
};



StartSonorkQueueClass(TSonorkShortStringQueue,TSonorkShortString)
	bool	Add(SONORK_C_CSTR);
	void 	Clear();
	~TSonorkShortStringQueue();
EndSonorkQueueClass;

StartSonorkSimpleDataListClass(SONORK_DWORD2List, SONORK_DWORD2 , 60 )
	SONORK_DWORD2*	Get(const SONORK_DWORD2&);
EndSonorkSimpleDataListClass;

StartSonorkSimpleDataListClass( TSonorkOldMsgNotificationList, TSonorkOldMsgNotification, 32 );
	NoExtensions
EndSonorkSimpleDataListClass;

StartSonorkSimpleDataListClass( TSonorkOldSidNotificationList , TSonorkOldSidNotification , 32 )
	NoExtensions
EndSonorkSimpleDataListClass;


#endif