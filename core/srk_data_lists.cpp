#include "srk_defs.h"
#pragma hdrstop
#include "srk_data_lists.h"

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


SONORK_DWORD2* SONORK_DWORD2List::Get(const SONORK_DWORD2& dw2)
{
	TSonorkListIterator I;
	SONORK_DWORD2* pdw2;
	InitEnum(I);
	while((pdw2=EnumNext(I))!=NULL)
	{
		if(dw2==*pdw2)
			return pdw2;
	}
	return NULL;
}

// ----------------------------------------------------
// TSonorkSimpleDataList

void *TSonorkSimpleDataList::w_ItemPtr(DWORD item_no) const
{
	return list+(item_no*item_size);
}

TSonorkSimpleDataList::TSonorkSimpleDataList(DWORD p_item_size,DWORD p_items_per_block)
{
	list=NULL;
	items=0;
	item_size=p_item_size;
	items_per_block=p_items_per_block;
	allocated_items=0;

}
TSonorkSimpleDataList::~TSonorkSimpleDataList()
{
	if(list)SONORK_MEM_FREE(list);
}
void TSonorkSimpleDataList::_Clear()
{
	if(list)SONORK_MEM_FREE(list);
	list=NULL;items=allocated_items=0;
}

void TSonorkSimpleDataList::AllocBlocks(UINT n)
{
	BYTE *p_list=list;
	allocated_items += (items_per_block * n);
	list = SONORK_MEM_ALLOC(BYTE,allocated_items*item_size);
	if(p_list)
	{
		if(items)
			memcpy(list,p_list,items*item_size);
		SONORK_MEM_FREE(p_list);
	}
}
void	TSonorkSimpleDataList::PrepareFor(UINT items)
{
	UINT needed_blocks;
	if(items > allocated_items )
	{
		needed_blocks = ((items - allocated_items) / items_per_block) + 1;
		AllocBlocks(needed_blocks);
	}
}

void *TSonorkSimpleDataList::w_AddItem(const void*P)
{
    void *ptr;
	if(items>=allocated_items)AllocBlocks(1);
	ptr=w_ItemPtr(items++);
    if(P)memcpy(ptr,P,item_size);
    return ptr;
}
void *TSonorkSimpleDataList::w_EnumNext(TSonorkListIterator& iterator) const
{
    void *ptr;
    if(iterator.d.index>=items)return NULL;
    ptr=w_ItemPtr(iterator.d.index);
    iterator.d.index++;
    return ptr;
}


// ----------------------------------------------------
// TSonorkQueue

TSonorkQueue::TSonorkQueue(){items=0;lock_count=0;head=tail=NULL;}
TSonorkQueue::~TSonorkQueue(){_Clear();}
void*
	TSonorkQueue::w_Peek() const
{
	if( head != NULL)
		return head->data;
	return NULL;
}
void*
	TSonorkQueue::w_PeekNo(UINT no) const
{
	const TSonorkQueueItem *item;
	for(item=head ; item!=NULL ; no--,item=item->next )
		if( no == 0)
			return item->data;
	return NULL;
}

#if defined(__BORLANDC__)
#pragma option -w-ncf
#endif
void TSonorkQueue::BeginEnum(TSonorkListIterator&I) const
{
	I.d.ptr=FirstItem();
#if !defined(_MSC_VER)
	Lock();
#endif
}
void TSonorkQueue::EndEnum(TSonorkListIterator&I) const
{
#if !defined(_MSC_VER)
	assert( lock_count > 0 );
	Unlock();
#endif

	I.d.ptr=NULL;
}

#if defined(__BORLANDC__)
#pragma option -w+ncf
#endif

void* TSonorkQueue::w_EnumNext(TSonorkListIterator&I) const
{
	const TSonorkQueueItem *item;
	void *data;

#if !defined(_MSC_VER)
		assert( lock_count > 0 );
#endif

	item=(TSonorkQueueItem *)I.d.ptr;
	while(item != NULL)
	{
		data=item->data;
		I.d.ptr=item->NextItem();
		return data;
	}
	return NULL;
}

void	TSonorkQueue::_Clear()
{
	TSonorkQueueItem *item,*p_item;

#if !defined(_MSC_VER)
	assert( lock_count == 0 );
#endif

	item=head;
	while(item)
	{
		p_item=item->next;
		SONORK_MEM_FREE(item);
		item=p_item;
	}
	head=tail=NULL;
	items=0;
}
void*
	TSonorkQueue::w_RemoveFirst()
{
	void *rv=NULL;
#if defined( SONORK_DEBUG )
        assert( lock_count == 0 );
#endif
	if(head)
	{
		rv=head->data;
		w_RemoveItem(head);
	}
	return rv;
}
bool
 TSonorkQueue::w_RemoveItem(TSonorkQueueItem *item)
{
#if defined( SONORK_DEBUG )
	assert( lock_count == 0 );
#endif
	assert(items>0);
	if(items==1)
	{
		head=tail=NULL;
	}
	else
	{
		if(!item->prev)
		{
		    head=item->next;
		    head->prev=NULL;
		}
		else
		{
		    item->prev->next=item->next;
		}

		if(!item->next)
		{
		    tail=item->prev;
		    tail->next=NULL;
		}
		else
		    item->next->prev=item->prev;
		}
	SONORK_MEM_FREE(item);
	items--;
	return true;
}
bool
 TSonorkQueue::w_Remove(void*data)
{
    TSonorkQueueItem *scan;
    bool found=false;
#if defined( SONORK_DEBUG )
   assert( lock_count == 0 );
#endif
    scan=head;
    while(scan)
    {
	if(scan->data==data)
	{
		found=w_RemoveItem(scan);
		break;
	}
	else
		scan=scan->next;
    }
    return found;
}
bool
 TSonorkQueue::w_Exists(void*data)
{
    TSonorkQueueItem *scan;
#if defined( SONORK_DEBUG )
   assert( lock_count == 0 );
#endif
    scan=head;
    while(scan)
    {
	if(scan->data==data)
		return true;
	else
		scan=scan->next;
    }
    return false;
}

bool
	TSonorkQueue::w_Add(void*data,SONORK_QUEUE_ITEM_PRIORITY priority)
{
	TSonorkQueueItem *item;

	item=SONORK_MEM_ALLOC(TSonorkQueueItem,sizeof(TSonorkQueueItem));
	item->data=data;
	item->priority=(BYTE)priority;
	//Lock(TSonorkQueue)
	if(!items)
	{
		head=tail=item;
		item->next=item->prev=NULL;
	}
	else
	{
		TSonorkQueueItem *scan=tail;
		for(;;)
		{
			if(item->priority>scan->priority)
			{
				// item has higher priority than scan
				// try to follow the chain towards the head
				if(scan->prev)
				{
					scan=scan->prev;
					// There is another item.. follow it.
					continue;
				}
				else
				{
					// Can't follow the chain, no more items:
					// insert before scan item and exit
					item->prev=NULL;
					item->next=scan;
					scan->prev=item;
#if defined( SONORK_DEBUG )
					assert(scan==head);
					if(!scan->next)
						assert(scan==tail);
#endif
					head=item;
					break;
				}
			}
			else
			{
				// item has lower or equal priority than scan
				// allocate after scan.
				item->prev=scan;
				item->next=scan->next;
				scan->next=item;
				if(!item->next)
				{
#if defined( SONORK_DEBUG )
					assert(scan==tail);
#endif
					tail=item;
				}
				else
					item->next->prev=item;
				break;
			}
		}
	}
	items++;
	//Unlock(TSonorkQueue)
   return true;
}

TSonorkShortStringQueue::~TSonorkShortStringQueue()
{
	Clear();
}

bool TSonorkShortStringQueue::Add(SONORK_C_CSTR str)
{
	TSonorkShortString*S;
	S=new TSonorkShortString(str);
	return Add(S);
}
void TSonorkShortStringQueue::Clear()
{
	TSonorkShortString*S;
	while((S=RemoveFirst()) != NULL )
		delete S;

}

