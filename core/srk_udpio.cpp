#include "srk_defs.h"
#pragma hdrstop
#include "srk_udpio.h"

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






TSonorkUdpEngine::TSonorkUdpEngine()
:TSonorkNetIO(SONORK_NETIO_PROTOCOL_UDP,SONORK_UDP_PACKET_FULL_ENTRY_SIZE*2)
{}



#if defined(SONORK_SERVER_BUILD)
int
 TSonorkUdpEngine::SendTo(const void*data,UINT size,sockaddr_in*target_address)
{
	UINT bytes;
	int rv;
	bytes=(UINT)::sendto(Socket()
		,(const char*)data
		,size
		,0
		,(sockaddr*)target_address
		,sizeof(*target_address));
	if(bytes!=size)
	{
		rv=WSAGetLastError();
		return rv;
	}
	return 0;
}

UINT
 TSonorkUdpEngine::RecvFrom(DWORD select_msecs, sockaddr_in *sender_address)
{
	int   sender_address_len;
	if(_Select(select_msecs))
	{
		if(ReadEvent())
		{
			sender_address_len=sizeof(*sender_address);
			recv_bytes=::recvfrom(Socket()
				,(char*)Buffer()
				,BufferSize()
				,0
				,(sockaddr*)sender_address
				,&sender_address_len);
			if(recv_bytes==(UINT)SOCKET_ERROR)
			{
				return 0;
			}
			return recv_bytes;
		}
	}
	return 0;
}
#else
int   TSonorkUdpEngine::Send(const void*data,UINT size)
{
	UINT dummy;
	return _Send(data,size,dummy);
}

UINT
 TSonorkUdpEngine::Recv(DWORD select_msecs)
{
	if(_Select(select_msecs)>0)
	    if(ReadEvent())
    		if(!_Recv())
	        	return RecvBytes();
    return 0;
}
#endif

int
 TSonorkUdpEngine::Listen(DWORD host, WORD port)
{
	int rv;
	rv=_Bind(host, port);
	if(!rv)
	{
		SetStatus(SONORK_NETIO_STATUS_LISTENING);
		EnableSelectRead(true);
		EnableSelectWrite(false);
	}
	else
		sonork_printf("UDP:_Bind failed (Err %u)\n",rv);
    return rv;
}


/////////////////////
// Constructor #1
//  Creates a packet with no data.
//  Should call AssignBuffer() to set up the buffer, before using this packet.
//  See notes on AssignBuffer().
TSonorkUdpPacket::TSonorkUdpPacket()
{
    uid=SONORK_UDP_PACKET_INVALID_UID;
    size=entries=last_entry_size=0;
    entry_size=SONORK_UDP_PACKET_MAX_ENTRY_DATA_SIZE;
    buffer=NULL;
}

/////////////////////
// Constructor #2
//  Creates a packet with a given size and (optionaly) loads the buffer
//  Used when creating a buffer for transmitting data.
TSonorkUdpPacket::TSonorkUdpPacket(SONORK_UDP_PACKET_UID p_uid,void*data,DWORD data_size)
{
    uid=p_uid;
    size=data_size;
    buffer=SONORK_MEM_ALLOC(BYTE,size);
    if(data)memcpy(buffer,data,size);
    entry_size=SONORK_UDP_PACKET_MAX_ENTRY_DATA_SIZE;
    Rehash();
}

/////////////////////
// Constructor #3
//  Creates a packet from a TSonorkUdpPacketEntry
//  Used when creating a buffer for receiving data
//  The UDP_PACKET has space enough to hold all the incoming
//  data, but at creation it will only have data in the segment
//  pointed by the TSonorkUdpPacketEntry.
TSonorkUdpPacket::TSonorkUdpPacket(TSonorkUdpPacketEntry*E,DWORD E_size,SONORK_UDP_AKN_RESULT*akn_result)
{
    uid			=E->UID();
    size		=E->PacketSize();
    entry_size	=E->EntrySize();
    if(!Rehash())
    	*akn_result=SONORK_UDP_AKN_RESULT_INVALID;
    else
    {
	    buffer=SONORK_MEM_ALLOC(BYTE,size);
	    *akn_result=LoadEntry(E->EntryNo(),E->DataPtr(),E->CalcDataSize(E_size));
    }
}



/////////////////////
//  Rehash()
//   Private. Determines the amount of entries needed to transmit
//   the buffer and resets all entry flags.
bool	TSonorkUdpPacket::Rehash()
{
   if(size>entry_size)
   {
	   	full_entries  		= size/entry_size;
   		last_entry_size   = size%entry_size;
   }
   else
   {
      full_entries    = 0;
      last_entry_size = size;
   }
   akn_entries=0;
   entries 				= full_entries;
   if(last_entry_size)entries++;
   if(entries)
   {
	   if(entries>SONORK_UDP_PACKET_MAX_ENTRIES)return false;
	   SONORK_ZeroMem(e_flags,sizeof(SONORK_UDP_PACKET_ENTRY_FLAGS)*entries);
   }
   return true;

}


/////////////////////
//  AssignBuffer()
//   Public. Assigns a buffer.
//   The data is NOT copied, instead the data's ownership
//   is transferred to the TSonorkUdpPacket. After assigning
//   a buffer to the UDP_PACKET, the caller should NOT use
//   that buffer because it will be deleted when the packet
//   is deleted.
void	TSonorkUdpPacket::AssignBuffer(void*data,DWORD data_size)
{
	assert(buffer==NULL);
	assert(data!=NULL);
	size=data_size;
	buffer=(BYTE*)data;
   Rehash();

}

/////////////////////
//  GetEntryPtr()
//   Returns a pointer to the beginning of the buffer segment
//   that belongs to the entry.
BYTE  *  TSonorkUdpPacket::EntryPtr(UINT entry_no)
{
	if(entry_no>=entries)return NULL;
	return buffer+entry_no*entry_size;
}

/////////////////////
//  GetEntrySize()
//   Returns the size of the buffer segment that belongs to the entry.
UINT   TSonorkUdpPacket::EntrySize(UINT entry_no) const
{
   if(entry_no>=entries)return 0;
   if(entry_no<full_entries)return entry_size;
   return last_entry_size;
}

/////////////////////
//  ProcessAkn()
//   Public. Used when transmitting.
//   It processes an AKN for one of the packet's entries.
//   Tipically the sender will send the entries of the outgoing
//   UDP_PACKET along with the entry's QID and wait for AKN on
//   each of those entries. When the receptor receives an entry
//   it will answer with the entry's QID. When the transmitter
//   gets back the QID it calls ProcessAkn(QID) to mark the
//   entries as [un]successfully transmitted.
//   Returns:
//   SONORK_UDP_AKN_RESULT_INVALID if the QID does not correspond to this packet.
//   SONORK_UDP_AKN_RESULT_ADD if the entry has been marked as transmitted.
//   SONORK_UDP_AKN_RESULT_SET if the entry was already marked as transmitted (no harm)
//   SONORK_UDP_AKN_RESULT_ALL if ALL of the entries have been marked as transmitted
//                         and hence, the whole packet is marked as transmitted.
//   SONORK_UDP_AKN_RESULT_NOP if the QID is valid but negative (i.e. the receiver
//                         requests that the entry be retransmitted)

SONORK_UDP_AKN_RESULT TSonorkUdpPacket::ProcessAkn(SONORK_UDP_PACKET_QID qid)
{
	SONORK_UDP_AKN_RESULT result;
	// Check if the UID matches
	if(UID()==QidToUid(qid))
	{
		DWORD no=QidEntryNo(qid);
		// Check if entry no is valid
		if(no>=Entries())
		{
			result=SONORK_UDP_AKN_RESULT_INVALID;
		}
		else
		{
			if(qid&SONORK_UDP_PACKET_QID_F_AKN_NEGATIVE)
			{
				if(TestEntryFlag(no,SONORK_UDP_ENTRY_F_AKN))
				{
					ClearEntryFlag(no,SONORK_UDP_ENTRY_F_AKN);
					DecAknEntries();
				}
				else
				{
					SetEntryFlag(no,SONORK_UDP_ENTRY_F_RESEND);
				}
				ClearEntryFlag(no,SONORK_UDP_ENTRY_F_SENT);
				result=SONORK_UDP_AKN_RESULT_NOP;
			}
			else
			{
				if(akn_entries<entries)
				{
					if(!TestEntryFlag(no,SONORK_UDP_ENTRY_F_AKN))
					{
						SetEntryFlag(no,SONORK_UDP_ENTRY_F_AKN);
						IncAknEntries();
						if(akn_entries<entries)
						{
							if(qid&SONORK_UDP_PACKET_QID_F_AKN_COMPLETE)
							{
								akn_entries=entries;
								result=SONORK_UDP_AKN_RESULT_ALL;
							}
							else
								result=SONORK_UDP_AKN_RESULT_ADD;
						}
						else
							result=SONORK_UDP_AKN_RESULT_ALL;
					}
					else
					{
						if(akn_entries<entries)
						{
							if(qid&SONORK_UDP_PACKET_QID_F_AKN_COMPLETE)
							{
								akn_entries=entries;
								result=SONORK_UDP_AKN_RESULT_ALL;
							}
							else
								result=SONORK_UDP_AKN_RESULT_SET;
						}
						else
							result=SONORK_UDP_AKN_RESULT_ALL;
					}
				}
				else
					result=SONORK_UDP_AKN_RESULT_ALL;
			}
		}
	}
	else
		result=SONORK_UDP_AKN_RESULT_INVALID;
	return result;
}

/////////////////////
//  ProcessEntry
//   Public. Used when receiving.
//   Checks that the ENTRY received corresponds to this packet,
//   if it does not:
//     Returns SONORK_UDP_AKN_RESULT_NEW meaning that a new packet has started
//     because the ENTRY's information does not correspond to this one.
//   if it does:
//     Returns the result of LoadEntry()
SONORK_UDP_AKN_RESULT TSonorkUdpPacket::ProcessEntry(TSonorkUdpPacketEntry*E, DWORD full_entry_size)
{
   if(UID()==E->UID())
   {
	 if(PacketSize()==(DWORD)E->packet_size && PacketEntrySize()==(DWORD)E->entry_size)
     {
		return LoadEntry(E->EntryNo(),E->DataPtr(),E->CalcDataSize(full_entry_size));
     }
   }
   return SONORK_UDP_AKN_RESULT_NEW;
}


/////////////////////
//  LoadEntry
//   Private. Used when receiving, called by constructor and ProcessEntry().
//   Loads the portion of the buffer that corresponds to the entry number.
//   It returns:
//   SONORK_UDP_AKN_RESULT_INVALID if the entry number or the size is invalid.
//   SONORK_UDP_AKN_RESULT_ADD if the entry was successfully been loaded.
//   SONORK_UDP_AKN_RESULT_SET if the entry was already loaded.(no harm)
//   SONORK_UDP_AKN_RESULT_ALL if ALL of the entries have been loaded
//                         and hence, the whole packet is loaded.
SONORK_UDP_AKN_RESULT
	TSonorkUdpPacket::LoadEntry(DWORD entry_no,void*data,DWORD data_size)
{
	void *ptr;
	DWORD ex_size;
	SONORK_UDP_AKN_RESULT akn_result;
	ptr=EntryPtr(entry_no);
	if(ptr)
	{
		ex_size=EntrySize(entry_no);
		if(data_size==ex_size)
		{
			if(TestEntryFlag(entry_no,SONORK_UDP_ENTRY_F_LOADED))
			{
				akn_result=(akn_entries<entries?SONORK_UDP_AKN_RESULT_SET:SONORK_UDP_AKN_RESULT_ALL);
			}
			else
			{
				akn_entries++;
				memcpy(ptr,data,data_size);
				SetEntryFlag(entry_no,SONORK_UDP_ENTRY_F_LOADED);
				akn_result=(akn_entries<entries?SONORK_UDP_AKN_RESULT_ADD:SONORK_UDP_AKN_RESULT_ALL);
			}
		}
		else
		{
			akn_result=SONORK_UDP_AKN_RESULT_INVALID;
		}
	}
	else
	{
	akn_result=SONORK_UDP_AKN_RESULT_INVALID;
	}

	return akn_result;
}


///////////////////////////
// TSonorkUdpPacketQueue
//

TSonorkUdpPacket*TSonorkUdpPacketQueue::Remove(SONORK_UDP_PACKET_UID uid)
{
    TSonorkUdpPacket*P;
    const TSonorkQueueItem* qItem;
	qItem=FirstItem();
	while( qItem!=NULL )
	{
		P=(TSonorkUdpPacket*)qItem->data;
		if(P!=NULL )
        {
            if(P->UID()==uid)
            {
                if(w_RemoveItem((TSonorkQueueItem*)qItem))
                    return P;
                else
                    break;
            }
		}            
        qItem=qItem->NextItem();
    }
    return NULL;
}





void TSonorkUdpPacketHeader::Normalize(){}
void TSonorkUdpPacketEntry::Normalize(){}

