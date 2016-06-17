#include "srk_defs.h"
#pragma hdrstop
#include "srk_ext_user_data.h"
#include "srk_codec_io.h"

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

#define SONORK_EXT_USER_DATA_ATOM_VERSION	1

//--------------------------------------------------
// TSonorkExtUserData
// Holds all (RAM) information for a remote user in our auth list
//--------------------------------------------------
bool
 TSonorkExtUserData::IsValidUserType() const
{
	return user_type == SONORK_USER_TYPE_AUTHORIZED
		|| user_type == SONORK_USER_TYPE_NOT_AUTHORIZED;
}

TSonorkExtUserData::TSonorkExtUserData(SONORK_USER_TYPE ptype)
	:TSonorkUserData(SONORK_USER_INFO_LEVEL_1)
	{
		ClearExtData();
		user_type = ptype;
	}
void
 TSonorkExtUserData::ClearRunData()
{
	SONORK_ZeroMem(&run_data,sizeof(run_data));
	sid_text.Clear();
}
void
 TSonorkExtUserData::ClearExtData()
{
	display_alias.Set(alias);
	sound.online.Clear();
	sound.message.Clear();
	ClearRunData();
	ClearCtrlData();
}
void
 TSonorkExtUserData::GetUserHandle(TSonorkUserHandle*H) const
{
	GetLocus1(H);
	H->utsLinkId=UtsLinkId();
}




#if SONORK_CODEC_LEVEL > 5

//--------------------------------------------------
// TSonorkExtUserList
// Holds list of pointers to TSonorkExtUserData
// structures holding the information of the profile's
// user list.
//--------------------------------------------------

TSonorkExtUserDataPtr
 TSonorkExtUserList::GetQueue(const SONORK_DWORD2& userId,DWORD* p_queue_no) const
{
	TSonorkListIterator 	I;
	TSonorkExtUserData* 	pUD;
const	TSonorkExtUserDataQueue*pQ;
	DWORD			queue_no;

	queue_no=(userId.v[1]&SONORK_EXT_USER_LIST_HASH_MASK);
	pQ=&hash_queue[queue_no];
	if( p_queue_no!=NULL )*p_queue_no=queue_no;

	pQ->BeginEnum(I);
	while((pUD=pQ->EnumNext(I))!=NULL)
		if(pUD->userId == userId)
			break;
	pQ->EndEnum(I);
	return pUD;
}
TSonorkExtUserDataPtr
 TSonorkExtUserList::Get(const SONORK_DWORD2& user_id) const
{
	return GetQueue(user_id,NULL);
}

bool
 TSonorkExtUserList::Del(const SONORK_DWORD2& user_id)
{
	TSonorkExtUserDataPtr pUD;
	DWORD	queue_no;
	if((pUD=GetQueue(user_id,&queue_no))!=NULL)
	{
		assert( hash_queue[queue_no].Remove(pUD) );
		assert( enum_queue.Remove(pUD) );
		SONORK_MEM_DELETE(pUD);
		return true;
	}
	return false;
}

TSonorkExtUserDataPtr
 TSonorkExtUserList::Add(const TSonorkExtUserDataPtr newUD)
{

	TSonorkExtUserDataPtr	listUD;
	DWORD			queue_no;
	
	if(newUD == NULL)return NULL;
	if(newUD->userId.IsZero())return NULL;

	listUD =GetQueue(newUD->userId,&queue_no);

	if(listUD==NULL)
	{
		// Does not exist in list

		listUD=new TSonorkExtUserData( newUD->UserType() );
		hash_queue[queue_no].Add( listUD );
		enum_queue.Add( listUD );

		listUD->display_alias.Set(newUD->display_alias);
		listUD->sound.online.Set(newUD->sound.online);
		listUD->sound.message.Set(newUD->sound.message);
	}
	listUD->Set(*newUD);

	memcpy(&listUD->run_data
		,&newUD->run_data
		,sizeof(listUD->run_data));

	listUD->sid_text.Set(newUD->sid_text);

	memcpy(&listUD->ctrl_data
		,&newUD->ctrl_data
		,sizeof(listUD->ctrl_data));
	return listUD;
}

void
 TSonorkExtUserList::BeginEnum( TSonorkListIterator& I ) const
{
	enum_queue.BeginEnum(I);
}
TSonorkExtUserDataPtr
 TSonorkExtUserList::EnumNext( TSonorkListIterator& I ) const
{
	return enum_queue.EnumNext( I );
}

void
 TSonorkExtUserList::EndEnum( TSonorkListIterator& I ) const
{
	enum_queue.EndEnum(I);
}

void
 TSonorkExtUserList::Clear()
{
	UINT i;
	TSonorkExtUserDataPtr 	pUD;
	while((pUD=enum_queue.RemoveFirst())!=NULL)
	{
		SONORK_MEM_DELETE(pUD);
	}
	for(i=0;i<SONORK_EXT_USER_LIST_HASH_SIZE;i++)
		hash_queue[i].Clear();
}

TSonorkExtUserList::~TSonorkExtUserList()
{
	Clear();
}


// ---------------------------------------------------------------------------
// TSrkExtUserDataAtom

void TSrkExtUserDataAtom::CODEC_Clear()
{
	UD->Clear();
	UD->ClearExtData();
}


DWORD TSrkExtUserDataAtom::CODEC_DataSize() const
{
	return sizeof(DWORD)*8
		+ ::CODEC_Size( UD )
		+ ::CODEC_Size( &UD->display_alias )
		+ ::CODEC_Size( &UD->sound.online )
		+ ::CODEC_Size( &UD->sound.message )
		+ sizeof(UD->ctrl_data);
}
void TSrkExtUserDataAtom::CODEC_ReadDataMem( TSonorkCodecReader& CODEC)
{
	DWORD	version,type,sizeof_ctrl_data;

	CODEC.ReadDW(&version);
	CODEC.ReadDW(&type);
	CODEC.ReadDW(&sizeof_ctrl_data);
	CODEC.Skip(sizeof(DWORD)*5);


	if( version > SONORK_EXT_USER_DATA_ATOM_VERSION )
	{
		CODEC.SetBadCodecError(__FILE__ , SONORK_MODULE_LINE );
	}
	else
	{
		UD->SetUserType((SONORK_USER_TYPE)(type&SONORK_USER_TYPE_MASK));
		if( !UD->IsValidUserType() )
		{
			CODEC.SetBadCodecError(__FILE__ , SONORK_MODULE_LINE );
		}
		else
		{
			CODEC.Read( UD );
			CODEC.Read( &UD->display_alias );
			CODEC.Read( &UD->sound.online );
			CODEC.Read( &UD->sound.message );
			CODEC.ReadDWN((DWORD*)&UD->ctrl_data , SIZEOF_IN_DWORDS(UD->ctrl_data) );
		}
	}
}


void TSrkExtUserDataAtom::CODEC_WriteDataMem( TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(SONORK_EXT_USER_DATA_ATOM_VERSION);
	CODEC.WriteDW(UD->UserType());
	CODEC.WriteDW(sizeof(UD->ctrl_data));
	CODEC.Skip(sizeof(DWORD)*5);
	CODEC.Write( UD );
	CODEC.Write( &UD->display_alias );
	CODEC.Write( &UD->sound.online );
	CODEC.Write( &UD->sound.message );
	CODEC.WriteDWN((DWORD*)&UD->ctrl_data , SIZEOF_IN_DWORDS(UD->ctrl_data) );
}


#endif
