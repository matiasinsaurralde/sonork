#if !defined(SONORK_UDPIO_H)
#define SONORK_UDPIO_H
#include "srk_defs.h"
#include "srk_netio.h"

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


#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,1)
#endif

#if !defined(SONORK_UDP_PACKET_FULL_ENTRY_SIZE)
#	define SONORK_UDP_PACKET_FULL_ENTRY_SIZE 1024
#endif



typedef DWORD 		SONORK_UDP_PACKET_QID;
typedef DWORD 		SONORK_UDP_PACKET_UID;
typedef BYTE   		SONORK_UDP_PACKET_ENTRY_FLAGS;


#define MAX_ENTRIES_PER_TIMESLOT	   	(SONORK_UDPIO_HFLAG_MAX_AKN_COUNT*3/2)//128//(SONORK_UDP_HFLAG_MAX_AKN_COUNT*3)

#define SONORK_UDP_PACKET_MAX_ENTRIES 		512
#define SONORK_UDP_PACKET_MAX_ENTRY_DATA_SIZE	(SONORK_UDP_PACKET_FULL_ENTRY_SIZE-sizeof(TSonorkUdpPacketEntry)-(sizeof(SONORK_UDP_PACKET_QID)*SONORK_UDPIO_HFLAG_MAX_AKN_COUNT))

#define SONORK_UDP_PACKET_QID_F_AKN_NEGATIVE	0x80000000
#define SONORK_UDP_PACKET_QID_F_AKN_COMPLETE	0x40000000
#define SONORK_UDP_PACKET_UID_MAX_VALUE		0xffff	//0xffff
#define SONORK_UDP_PACKET_UID_SHIFT   		12
#define SONORK_UDP_PACKET_QID_UID_MASK 		0x0ffff000
#define SONORK_UDP_PACKET_QID_ENTRY_MASK_2  	0x000001ff
#define SONORK_UDP_PACKET_VALUE_TO_UID(n)  	((SONORK_UDP_PACKET_QID)(((n)<<SONORK_UDP_PACKET_UID_SHIFT)&SONORK_UDP_PACKET_QID_UID_MASK))
#define SONORK_UDP_PACKET_UID_TO_VALUE(n)	((UINT) (((n)&SONORK_UDP_PACKET_QID_UID_MASK)>>SONORK_UDP_PACKET_UID_SHIFT))

#define SONORK_UDP_PACKET_INVALID_QID		((SONORK_UDP_PACKET_QID)0)
#define SONORK_UDP_PACKET_INVALID_UID		((SONORK_UDP_PACKET_UID)0)


#define QidToUid(q)			((q)&SONORK_UDP_PACKET_QID_UID_MASK)
#define QidEntryNo(q)			((q)&SONORK_UDP_PACKET_QID_ENTRY_MASK_2)
#define UidEntryToQid(uid,entry_no)	((SONORK_UDP_PACKET_QID)(QidToUid(uid)|((entry_no)&SONORK_UDP_PACKET_QID_ENTRY_MASK_2)))

#if defined(__BORLANDC__)
#	if SONORK_PACKET_MAX_FULL_SIZE > (SONORK_UDP_PACKET_MAX_ENTRY_DATA_SIZE*SONORK_UDP_PACKET_MAX_ENTRIES)
# 		error SONORK_MAX_PACKET_SIZE exceeds (SONORK_UDP_PACKET_MAX_ENTRY_DATA_SIZE*SONORK_UDP_PACKET_MAX_ENTRIES)
#	elif SONORK_PACKET_MAX_FULL_SIZE>0xfffff
#		error SONORK_PACKET_MAX_FULL_SIZE exceeds 1,048,575
#	endif
#endif


enum SONORK_UDP_AKN_RESULT
{
  SONORK_UDP_AKN_RESULT_INVALID
, SONORK_UDP_AKN_RESULT_NOP
, SONORK_UDP_AKN_RESULT_NEW
, SONORK_UDP_AKN_RESULT_ADD
, SONORK_UDP_AKN_RESULT_SET
, SONORK_UDP_AKN_RESULT_ALL
};



#define SONORK_UDP_ENTRY_F_SENT			0x01
#define SONORK_UDP_ENTRY_F_AKN			0x02
#define SONORK_UDP_ENTRY_F_RESEND		0x04
#define SONORK_UDP_ENTRY_F_LOADED		0x01

struct TSonorkUdpPacketHeader
{
	SONORK_NETIO_PACKET_HFLAG	hflag;
	SONORK_SID_ID			sid;

	DWORD
		Type() const
		{ return SONORK_NETIO_HFLAG_PACKET_TYPE(hflag);}

	DWORD
		Flags() const
		{ return SONORK_NETIO_HFLAG_PACKET_FLAGS(hflag);}

	SONORK_SID_ID
		SID() const
		{ return sid;}

	UINT
		AknCount() const
		{ return SONORK_UDPIO_HFLAG_AKN_COUNT(hflag) ;}

	UINT
		AknSize() const
		{ return sizeof(SONORK_UDP_PACKET_QID)*AknCount(); }

	BYTE*
		DataPtr()
		{ return ((BYTE*)this)+sizeof(*this);}

	void
		Normalize();

} __SONORK_PACKED ;


struct TSonorkUdpPacketEntry:public TSonorkUdpPacketHeader
{
	SONORK_UDP_PACKET_QID		qid;
	SONORK_NETIO_PACKET_SIZE	packet_size;
	SONORK_NETIO_PACKET_SIZE	entry_size;

	void
		Normalize();

	SONORK_UDP_PACKET_QID
		QID() const
		{ return qid;}

	SONORK_UDP_PACKET_QID
		UID() const
		{ return QidToUid(qid);}

	UINT
		EntryNo() const
		{ return QidEntryNo(qid);}

	UINT
		PacketSize() const
		{ return packet_size;}

	UINT
		EntrySize() const
		{ return entry_size;}

	SONORK_UDP_PACKET_QID*
		AknPtr()
		{ return (SONORK_UDP_PACKET_QID*)( ((BYTE*)this) + sizeof(*this) ); }

	BYTE*
		DataPtr()
		{ return ((BYTE*)AknPtr())+ AknSize(); 	}

	UINT
		CalcDataSize(UINT full_size) const
		{ return	full_size-(sizeof(*this)+AknSize()); }

	UINT
		FullSize(UINT data_size) const
		{ return data_size+sizeof(*this)+AknSize(); }

} __SONORK_PACKED ;




struct TSonorkUdpPacket
{
private:
	SONORK_UDP_PACKET_UID uid;
	UINT	size
		,entries
		,entry_size
		,akn_entries
		,full_entries
		,last_entry_size;
	BYTE*	buffer;

	SONORK_UDP_PACKET_ENTRY_FLAGS
		e_flags[SONORK_UDP_PACKET_MAX_ENTRIES];

	bool
		Rehash();

	SONORK_UDP_AKN_RESULT
		LoadEntry(DWORD entry_no,void*,DWORD);
public:
	TSonorkUdpPacket(SONORK_UDP_PACKET_UID uid,void*data,DWORD size);
	TSonorkUdpPacket(TSonorkUdpPacketEntry*E,DWORD E_size,SONORK_UDP_AKN_RESULT*akn_result);
	TSonorkUdpPacket();
	
	~TSonorkUdpPacket()
		{if(buffer!=NULL){SONORK_MEM_FREE(buffer);}}



	UINT
		UIDValue() const
		{ return SONORK_UDP_PACKET_UID_TO_VALUE(UID());}

	UINT
		UID() const
		{ return uid;}

	UINT
		Entries() const
		{ return entries; }

	UINT
		LastEntrySize()	const
		{ return last_entry_size; }

	UINT
		AknEntries() const
		{ return akn_entries;}
	bool
		TxComplete() const
		{ return entries==akn_entries;}

	void
		IncAknEntries()
		{ akn_entries++;}

	void
		DecAknEntries()
		{ akn_entries--;}

	void
		SetUID(SONORK_UDP_PACKET_UID v)
		{ uid=v;}

	void
		AssignBuffer(void*ptr,DWORD size);


	SONORK_UDP_AKN_RESULT
		ProcessAkn(SONORK_UDP_PACKET_QID);

	SONORK_UDP_AKN_RESULT
		ProcessEntry(TSonorkUdpPacketEntry*,DWORD entry_size);

	UINT
		PacketSize() const
		{ return size; }

	UINT
		PacketEntrySize() const
		{ return entry_size; }

	SONORK_UDP_PACKET_ENTRY_FLAGS*
		EntryTable()
		{ return e_flags;}

	BYTE*
		EntryPtr(UINT entry_no);

	UINT
		EntrySize(UINT entry_no) const;

	BYTE*
		Buffer()
		{ return buffer; }	//Same as EntryPtr(0);

	// RelinquishBuffer() returns the current buffer pointer
	// and sets this packet's buffer to NULL.
	BYTE*	RelinquishBuffer()
		{BYTE*rv=buffer;buffer=NULL;return rv;}

	SONORK_UDP_PACKET_ENTRY_FLAGS
		EntryFlags(UINT entry_no)	const
		{ return e_flags[entry_no]; }

	UINT
		TestEntryFlag(UINT entry_no,SONORK_UDP_PACKET_ENTRY_FLAGS flags) const
		{ return e_flags[entry_no]&flags;}

	void
		SetEntryFlag(UINT entry_no	,SONORK_UDP_PACKET_ENTRY_FLAGS flags)
		{ e_flags[entry_no]|=flags;}

	void
		ClearEntryFlag(UINT entry_no,SONORK_UDP_PACKET_ENTRY_FLAGS flags)
		{ e_flags[entry_no]&=(SONORK_UDP_PACKET_ENTRY_FLAGS)(~flags);}

} __SONORK_PACKED ;

StartSonorkQueueClass ( TSonorkUdpPacketQueue , TSonorkUdpPacket )

	TSonorkUdpPacket*
		Remove(SONORK_UDP_PACKET_UID);

EndSonorkQueueClass ;

class TSonorkUdpAknQueue:public TSonorkQueue
{
public:

	bool
		Add(SONORK_UDP_PACKET_QID P,SONORK_QUEUE_ITEM_PRIORITY priority=SONORK_QUEUE_PRIORITY_NORMAL)
		{ return w_Add((void*)P,priority);}

	SONORK_UDP_PACKET_QID
		Peek()
		{ return (SONORK_UDP_PACKET_QID)w_Peek();	}

	SONORK_UDP_PACKET_QID
		RemoveFirst()
		{ return (SONORK_UDP_PACKET_QID)w_RemoveFirst();	}

	bool
		Remove(SONORK_UDP_PACKET_QID P)
		{ return w_Remove((void*)P);}
} __SONORK_PACKED ;

#if defined(USE_PRAGMA_PUSH)
#pragma	pack( pop )
#endif


class TSonorkUdpEngine
:public TSonorkNetIO
{
public:
	TSonorkUdpEngine();

	int
		Listen(DWORD host, WORD port);

	int
		Connect(const TSonorkPhysAddr&addr, long send_sz, long recv_sz)
		{ return _Connect(addr,send_sz,recv_sz); }
		
#if defined(SONORK_SERVER_BUILD)
	int
		SendTo(const void*data,UINT size,sockaddr_in*target_address);

	UINT
		RecvFrom(DWORD wait_secs, sockaddr_in*sender_address);

#else
	UINT
		Recv(DWORD wait_msecs);

	int
		Send(const void*data,UINT size);
#endif
};


#endif
