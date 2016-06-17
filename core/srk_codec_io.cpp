#include "srk_defs.h"
#pragma hdrstop
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

void
 TSonorkCodecLCStringAtom::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	assert( ptr!=NULL );
	CODEC.WriteDW(length_or_size);
	if(length_or_size)CODEC.WriteRaw(ptr,length_or_size);
}

void
 TSonorkCodecLCStringAtom::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD source_len;
	assert( ptr!=NULL );
	CODEC.ReadDW(&source_len);
	if( CODEC.CodecOk() && length_or_size>0 )
	{
		source_len&=0xfffff;
		if(source_len )
		{
			if( (UINT)length_or_size > source_len )
			{
				CODEC.ReadRaw(ptr,source_len);
				*(ptr+source_len)=0;
			}
			else
			{
				CODEC.ReadRaw(ptr,length_or_size);
				CODEC.Skip( source_len - length_or_size );
				*(ptr+length_or_size-1)=0;
			}
		}
		length_or_size = source_len;
		return;
	}
	length_or_size=0;
	*ptr=0;
}


// ----------------------------------------------------------------------------
// TSonorkCodecIO

TSonorkCodecIO::TSonorkCodecIO(const void*pPtr, DWORD pSize, SONORK_ATOM_TYPE pType)
{
	codec.index=0;
	codec.ptr.rd=(const BYTE*)pPtr;
	codec.limit=pSize;
	codec.type=pType;
	codec.result=SONORK_RESULT_OK;
}

void
 TSonorkCodecIO::SetBadCodecError(SONORK_C_CSTR module,DWORD module_line)
{
	SetError(SONORK_RESULT_CODEC_ERROR,module,module_line,0xBADC0DEC);
}

void
 TSonorkCodecIO::SetErrorLine(SONORK_C_CSTR module,DWORD line, DWORD )
{
	codec.error.module	= module;
	codec.error.line	= line;
}

void
 TSonorkCodecIO::SetError(SONORK_RESULT result,SONORK_C_CSTR module,DWORD line,DWORD ex_size)
{
	if( CodecOk() )
	{
		codec.result 		= result;
		SetErrorLine(module,line,ex_size);
	}
}


// ----------------------------------------------------------------------------
// TSonorkShortString

UINT
 CODEC_Size(const TSonorkShortString*pV)
{
	return sizeof(DWORD) + pV->Length();
}

void
 TSonorkCodecWriter::Write(const TSonorkShortString*S)
{
	if( CodecOk() )
	{
		TSonorkCodecLCStringAtom A;
		A.ptr 			= (char*)S->CStr();
		A.length_or_size	= S->Length();
		A.CODEC_WriteDataMem( *this );
	}
}


void	TSonorkCodecReader::Read(TSonorkShortString*S) // buffer physical size
{
	if( CodecOk() )
	{
		DWORD len;    // Short String can only read 1,048,575 characters
		ReadDW(&len);
		if( CodecOk() )
		{
			len&=0xfffff;
			if(len)
			{
				S->SetBufferSize(len + 1);
				ReadRaw(S->Buffer(),len);
				*( S->Buffer()+ len )=0;
				return;
			}
		}
	}
	S->Clear();
}


// ----------------------------------------------------------------------------
// TSonorkUserLocus2

UINT	CODEC_Size(const TSonorkUserLocus2*pV)
{
	UINT sz;
	sz	=	::CODEC_Size(&pV->userId)
		+	::CODEC_Size(&pV->sid);
	if(!pV->sid.IsZero())
	{
		sz	+=	::CODEC_Size(&pV->sidFlags)
			+	::CODEC_Size(&pV->physAddr);
	}
	return sz;
}

void TSonorkCodecWriter::WriteUL2(const TSonorkUserLocus2*pV)
{
	WriteDWN( (DWORD*)pV , SIZEOF_IN_DWORDS(pV->userId)+SIZEOF_IN_DWORDS(pV->sid) );
	if( !pV->sid.IsZero() )
	{
		WriteDW4(&pV->sidFlags);
		WritePA1(&pV->physAddr);
	}
}
void TSonorkCodecReader::ReadUL2(TSonorkUserLocus2*pV)
{
	ReadDWN((DWORD*)pV , SIZEOF_IN_DWORDS(pV->userId)+SIZEOF_IN_DWORDS(pV->sid) );
	if(!pV->sid.IsZero())
	{
		ReadDW4(&pV->sidFlags);
		ReadPA1(&pV->physAddr);
	}
	else
	{
		pV->sidFlags.Clear();
		pV->physAddr.Clear();
	}
}


// ----------------------------------------------------------------------------
// TSonorkUserLocus3

UINT	CODEC_Size(const TSonorkUserLocus3*pV)
{
	return	::CODEC_Size((const TSonorkUserLocus2*)pV)
		+   ::CODEC_Size(&pV->region)
		+	sizeof(pV->reserved);
}
void TSonorkCodecWriter::WriteUL3(const TSonorkUserLocus3*pV)
{
	WriteUL2(pV);
	WriteDW2(&pV->region);
	WriteDWN(pV->reserved, SIZEOF_IN_DWORDS(pV->reserved) );
}

void TSonorkCodecReader::ReadUL3(TSonorkUserLocus3*pV)
{
	ReadUL2(pV);
	ReadDW2(&pV->region);
	ReadDWN(pV->reserved, SIZEOF_IN_DWORDS(pV->reserved) );
}


// ----------------------------------------------------------------------------
// TSonorkUserInfo

UINT	CODEC_Size( SONORK_USER_INFO_LEVEL level )
{
	switch( level )
	{
		case SONORK_USER_INFO_LEVEL_0:
			return 0;
		case SONORK_USER_INFO_LEVEL_1:
			return sizeof(TSonorkUserInfo1);
		case SONORK_USER_INFO_LEVEL_2:
			return sizeof(TSonorkUserInfo2);
		case SONORK_USER_INFO_LEVEL_3:
		default:
			return sizeof(TSonorkUserInfo3);
	}
}
void TSonorkCodecWriter::WriteUI(const TSonorkUserInfo1*pV, SONORK_USER_INFO_LEVEL level)
{
	DWORD sz= ::CODEC_Size( level );
	assert((sz&0x3)==0);
	WriteDWN((const DWORD*)pV,sz>>2);

}
void TSonorkCodecReader::ReadUI(TSonorkUserInfo1*pV, SONORK_USER_INFO_LEVEL level)
{
	DWORD sz= ::CODEC_Size( level );
	assert((sz&0x3)==0);
	ReadDWN((DWORD*)pV,sz>>2);
}


// ----------------------------------------------------------------------------
// TSonorkPhysAddr

UINT	CODEC_Size(const TSonorkPhysAddr*pV)
{
	return sizeof(pV->header)
		+ (pV->header.type==SONORK_PHYS_ADDR_NONE?0:sizeof(pV->data));
}

void TSonorkCodecWriter::WritePA1(const TSonorkPhysAddr*pV)
{
	WriteRaw(&pV->header,sizeof(pV->header));
	if(pV->header.type!=SONORK_PHYS_ADDR_NONE)
		WriteRaw(&pV->data,sizeof(pV->data));
}

void TSonorkCodecReader::ReadPA1(TSonorkPhysAddr*pV)
{
	ReadRaw(&pV->header,sizeof(pV->header));
	if( pV->header.type == SONORK_PHYS_ADDR_NONE )
		pV->Clear();
	else
		ReadRaw(&pV->data,sizeof(pV->data));
}


// ----------------------------------------------------------------------------
// TSonorkUserAddr

UINT
 CODEC_Size(const TSonorkNewUserAddr*pV, bool v104_compatibility_mode)
{
	UINT sz;
	sz	=	::CODEC_Size(&pV->sid)
		+	::CODEC_Size(&pV->sidFlags );
	if(pV->sidFlags.SidMode()!=SONORK_SID_MODE_DISCONNECTED)
	{
		sz+=::CODEC_Size(&pV->physAddr);
		if( !v104_compatibility_mode )
		{
			sz+=::CODEC_Size(&pV->version);
		}
	}
	return sz;
}
/*
OBSOLETE
void
 TSonorkCodecWriter::WriteUA1(const TSonorkUserAddr*pV)
{
	WriteDW2( &pV->sid );
	WriteDW4( &pV->sid_flags );
	if(pV->sid_flags.SidMode()!=SONORK_SID_MODE_DISCONNECTED)
		WritePA1(&pV->phys_addr);
}
*/
void
 TSonorkCodecWriter::WriteUA2(const TSonorkNewUserAddr*pV
	, bool v104_compatibility_mode)
{
	WriteDW2( &pV->sid );
	WriteDW4( &pV->sidFlags );
	if(pV->sidFlags.SidMode()!=SONORK_SID_MODE_DISCONNECTED)
	{
		WritePA1(&pV->physAddr);
		if( !v104_compatibility_mode )
		{
			WriteDW4(&pV->version);
		}
	}
}

void
 TSonorkCodecReader::ReadOldUA1(TSonorkNewUserAddr*pV)
{
	ReadDW2( &pV->sid );
	ReadDW4( &pV->sidFlags );
	if(pV->sidFlags.SidMode()!=SONORK_SID_MODE_DISCONNECTED)
		ReadPA1(&pV->physAddr);
	else
		pV->physAddr.Clear();
	pV->version.Clear();
}
void
 TSonorkCodecReader::ReadUA2(TSonorkNewUserAddr*pV)
{
	ReadDW2( &pV->sid );
	ReadDW4( &pV->sidFlags );
	if(pV->sidFlags.SidMode()!=SONORK_SID_MODE_DISCONNECTED)
	{
		ReadPA1(&pV->physAddr);
		ReadDW4(&pV->version);
	}
	else
	{
		pV->physAddr.Clear();
		pV->version.Clear();
	}
}


// ----------------------------------------------------------------------------
// TSonorkOldSidNotification
// This is obsolete now and it is here just for backwards compatibility
// new versions use TSonorkSidNotificationAtom

UINT	CODEC_Size(const TSonorkOldSidNotification*pV)
{
	return	::CODEC_Size(&pV->locus)
		+   ::CODEC_Size(&pV->userSerial);
}
void TSonorkCodecWriter::WriteOSN1(const TSonorkOldSidNotification*pV)
{
	WriteUL3(&pV->locus);
	WriteDW2(&pV->userSerial);
}

void TSonorkCodecReader::ReadOSN1(TSonorkOldSidNotification*pV)
{
	ReadUL3(&pV->locus);
	ReadDW2(&pV->userSerial);
}



#if SONORK_CODEC_LEVEL > 1

UINT
 CODEC_Size(const TSonorkUserDataNotes*pV,UINT enc_flags)
{
	UINT sz;
	sz=sizeof(DWORD);
	sz+=::CODEC_Size(&pV->user_data);
	if(enc_flags&SONORK_UDN_ENCODE_NOTES)
		sz+=::CODEC_Size(&pV->notes);
	if(enc_flags&SONORK_UDN_ENCODE_AUTH)
		sz+=::CODEC_Size(&pV->auth);
	if(enc_flags&SONORK_UDN_ENCODE_PASSWORD)
		sz+=::CODEC_Size(&pV->password);

	return sz;
}
void
 TSonorkCodecWriter::WriteUDN(const TSonorkUserDataNotes*pV,UINT enc_flags)
{
	if( CodecOk() )
	{
		DWORD aux;
		// ENCODE_SIZE includes the DWORD <enc_flags> prefix
		aux=(::CODEC_Size(pV,enc_flags) )&SONORK_UDN_ENCODE_SIZE_MASK;
		aux|=(enc_flags&~SONORK_UDN_ENCODE_SIZE_MASK);
		WriteDW(aux);
		Write(&pV->user_data);
		if(enc_flags&SONORK_UDN_ENCODE_NOTES)
			Write(&pV->notes);
		if(enc_flags&SONORK_UDN_ENCODE_AUTH)
			WriteAU2(&pV->auth);
		if(enc_flags&SONORK_UDN_ENCODE_PASSWORD)
			Write(&pV->password);
	}
}
void
 TSonorkCodecReader::ReadUDN(TSonorkUserDataNotes*pV)
{
	if( CodecOk() )
	{
		DWORD	enc_flags;
		UINT	sent_size,my_size;

		my_size = Offset();
		ReadDW(&enc_flags);
		Read(&pV->user_data);

		if(enc_flags & SONORK_UDN_ENCODE_NOTES)
			Read(&pV->notes);
		else
			pV->notes.Clear();

		if(enc_flags&SONORK_UDN_ENCODE_AUTH)
			ReadAU2(&pV->auth);
		else
			pV->auth.Clear();

		if(enc_flags&SONORK_UDN_ENCODE_PASSWORD)
			Read(&pV->password);
		else
			pV->password.Clear();

		// ENCODE_SIZE includes the DWORD <enc_flags> prefix
		my_size		= Limit() - my_size;
		sent_size	= enc_flags&SONORK_UDN_ENCODE_SIZE_MASK;
		if( sent_size > my_size )
		{
			Skip(sent_size - my_size );
		}
	}
}
#endif





// ---------------------------------
// TSonorkCodecWriter


void
 TSonorkCodecWriter::Skip(DWORD bytes)
{
	if( CodecOk() && bytes > 0 )
	{
		if( Size()+bytes > Limit() )
		{
			SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
		}
		else
		{
			SONORK_ZeroMem(EofPtr(),bytes);
			codec.index+=bytes;
		}
	}
}

void
 TSonorkCodecWriter::WriteRaw(const void *data_ptr,UINT data_size)
{
	if( CodecOk() && data_size > 0)
	{
		if(Size()+data_size>Limit())
		{
			SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
		}
		else
		{
			if( data_ptr != NULL )
			    memcpy(EofPtr(),data_ptr,data_size);
			codec.index+=data_size;
		}
	}
}

void
 TSonorkCodecWriter::Write(const TSonorkCodecAtom*A)
{
	if( CodecOk() )
	{
		DWORD	work_codec_size=Limit() - Size();
		DWORD	atom_codec_size;
		TSonorkCodecNullAtom    nullAtom;
		if(!A)A=&nullAtom;
		atom_codec_size = A->CODEC_Size();
		if( work_codec_size <  atom_codec_size)
		{
			SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
		}
		else
		{
			codec.result=A->CODEC_WriteMem(EofPtr(),work_codec_size);
			if(codec.result!=SONORK_RESULT_OK)
				SetErrorLine( __FILE__ , SONORK_MODULE_LINE , work_codec_size );
			else
				codec.index+=work_codec_size;
		}
	}
}


// ---------------------------------
// TSonorkCodecReader

void
 TSonorkCodecReader::Skip(DWORD bytes)
{
	if( CodecOk() )
	{
		if(Size()+bytes>Limit())
		{
			SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
		}
		else
		{
			codec.index+=bytes;
		}
	}
}

void
 TSonorkCodecReader::ReadRaw(void *data_ptr,UINT data_size)
{
	if( CodecOk() && data_size > 0)
	{
		if( Size()+data_size > Limit() )
		{
			SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
		}
		else
		{
			memcpy(data_ptr,EofPtr(),data_size);
			codec.index+=data_size;
		}
	}
}

void
 TSonorkCodecReader::Read(TSonorkCodecAtom*A)
{
	if( CodecOk() )
	{
		DWORD	codec_size=Limit()-Size();
		codec.result=A->CODEC_ReadMem(EofPtr(),codec_size);
		if(codec.result==SONORK_RESULT_OK)
		{
			codec.index+=codec_size;
			return ;
		}
		SetErrorLine( __FILE__ , SONORK_MODULE_LINE , codec_size );
	}
	A->CODEC_Clear();

}

