#include "srk_defs.h"
#pragma hdrstop
#include "srk_atom_db.h"
#include "srk_codec_file.h"
#include "srk_crypt.h"

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



#define SONORK_DB_ENTRY_SIZE_FLAG_DELETED   0x80000000
#define SONORK_DB_ENTRY_SIZE_MASK           0x001fffff
SONORK_C_CSTR	SonorkAtomDbFileExtensions[SONORK_ATOM_DB_FILE_EXTENSIONS]=
{
	".idx"
,	".dat"
};
inline int TSonorkAtomDb::WriteEntry(TSonorkAtomDbEntry *E)
{
	return file.err_code=SONORK_IO_WriteFile(file.index,E,sizeof(*E));
}
inline int TSonorkAtomDb::ReadEntry(TSonorkAtomDbEntry *E)
{
	return file.err_code=SONORK_IO_ReadFile(file.index,E,sizeof(*E));
}


TSonorkAtomDb::TSonorkAtomDb()
{
	file.data=file.index=SONORK_INVALID_FILE_HANDLE;
	db.open=false;
}

TSonorkAtomDb::~TSonorkAtomDb()
{
	Close();
}

void TSonorkAtomDb::Clear()
{
	if(IsOpen())
	{
		db.header.items       	=
		db.header.data_size     =0;
		db.header.version	=SONORK_ATOM_DB_VERSION;
		db.header.signature	=SONORK_FILE_SIGNATURE;
		db.header.entry_size	=sizeof(TSonorkAtomDbEntry);
		SONORK_ZeroMem(db.header.value,sizeof(db.header.value));
		FlushIndexHeader();
		Flush();
	}
}

void
 TSonorkAtomDb::Close()
{
	int s_err_code;
	if(file.index!=SONORK_INVALID_FILE_HANDLE)
	{
		s_err_code = file.err_code;
		FlushIndexHeader();
		file.err_code = s_err_code;
		::SONORK_IO_CloseFile(file.index);
	}
	if(file.data!=SONORK_INVALID_FILE_HANDLE)
		::SONORK_IO_CloseFile(file.data);
	file.index=file.data=SONORK_INVALID_FILE_HANDLE;
	db.open=false;
}
SONORK_RESULT
	TSonorkAtomDb::RecomputeItems()
{
	DWORD	idx_phys_size;
	DWORD	dat_phys_size;
	if( file.index==SONORK_INVALID_FILE_HANDLE || file.data==SONORK_INVALID_FILE_HANDLE)
		return SONORK_RESULT_INVALID_OPERATION;
	if(SONORK_IO_GetFileSize(file.index,&idx_phys_size)!=0
	|| SONORK_IO_GetFileSize(file.data,&dat_phys_size)!=0)
	{
		return SONORK_RESULT_STORAGE_ERROR;
	}
	if( idx_phys_size > sizeof(db.header) )
	{
		idx_phys_size-=sizeof(db.header);
		db.header.items = idx_phys_size/sizeof(TSonorkAtomDbEntry);
	}
	else
		db.header.items = 0;
	if( dat_phys_size > sizeof(DWORD) )
	{
		db.header.data_size = dat_phys_size - sizeof(DWORD);
	}
	else
	{
		db.header.data_size = 0;
		db.header.items = 0;
	}

	return SONORK_RESULT_OK;
}

SONORK_RESULT
	TSonorkAtomDb::Open(const TSonorkId& owner
	, const char *	db_name
	, bool&		db_reset)
{
	TSonorkTempBuffer	path(SONORK_MAX_PATH+64);
	SONORK_OPEN_FILE_RESULT	of_index_result;
	SONORK_OPEN_FILE_RESULT	of_data_result;
	DWORD			phys_size;
	bool                	header_was_reseted=false;
	SONORK_RESULT		result;

	Close();
	sprintf(path.CStr(),"%s%s",db_name,SonorkAtomDbFileExtensions[SONORK_ATOM_DB_IDX_EXTENSION]);
	file.index=SONORK_IO_OpenFile(
		 path.CStr()
		,db_reset
			?SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS
			:SONORK_OPEN_FILE_METHOD_OPEN_ALWAYS
		,&file.err_code
		,&of_index_result);

	if(file.index==SONORK_INVALID_FILE_HANDLE)
		result=SONORK_RESULT_STORAGE_ERROR;
	else
	for(;;)
	{
		if(of_index_result==SONORK_OPEN_FILE_RESULT_NEW || db_reset==true)
		{
reset_header:
			header_was_reseted=true;
			SONORK_ZeroMem(&db.header,sizeof(db.header));
			db.header.version=SONORK_ATOM_DB_VERSION;
			db.header.signature=SONORK_FILE_SIGNATURE;
			db.header.entry_size=sizeof(TSonorkAtomDbEntry);
			db.header.owner.Set( owner );
			SONORK_ZeroMem(db.header.value,sizeof(db.header.value));
			if(FlushIndexHeader())
			{
				result=SONORK_RESULT_STORAGE_ERROR;
				break;
			}
		}
		else
		{
			if(SONORK_IO_GetFileSize(file.index,&phys_size)!=0)
			{
				result=SONORK_RESULT_STORAGE_ERROR;
				break;
			}
			if(phys_size<sizeof(db.header))
				goto reset_header;
			file.err_code=SONORK_IO_ReadFile(file.index,&db.header,sizeof(db.header));

			if(file.err_code||db.header.signature!=SONORK_FILE_SIGNATURE)
				goto reset_header;

			if(db.header.version>SONORK_ATOM_DB_VERSION
				||db.header.entry_size!=sizeof(TSonorkAtomDbEntry))
			{
				result=SONORK_RESULT_INVALID_VERSION;
				break;
			}
			if(db.header.owner != owner )
			{
				result=SONORK_RESULT_ACCESS_DENIED;
				break;
			}
		}
		if(file.data==SONORK_INVALID_FILE_HANDLE)
		{
			sprintf(path.CStr(),"%s%s",db_name,SonorkAtomDbFileExtensions[SONORK_ATOM_DB_DAT_EXTENSION]);
			file.data=SONORK_IO_OpenFile(
				path.CStr()
				,db_reset||header_was_reseted
					?SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS
					:SONORK_OPEN_FILE_METHOD_OPEN_ALWAYS
				,&file.err_code
				,&of_data_result);
			if(file.data==SONORK_INVALID_FILE_HANDLE)
			{
				result=SONORK_RESULT_STORAGE_ERROR;
				break;
			}
			if(SONORK_IO_GetFileSize(file.data,&phys_size)!=0)
			{
				result=SONORK_RESULT_STORAGE_ERROR;
				break;
			}
			if(of_data_result==SONORK_OPEN_FILE_RESULT_NEW
				||of_index_result==SONORK_OPEN_FILE_RESULT_NEW
				||header_was_reseted)
			{
write_signature:
				file.err_code=SONORK_IO_SetFilePointer(file.data,0,SONORK_FILE_SEEK_BEGIN);
				if(file.err_code!=0)
				{
					result=SONORK_RESULT_STORAGE_ERROR;
					break;
				}
				file.err_code=SONORK_IO_WriteFile(file.data,&db.header.signature,sizeof(db.header.signature));
				if(file.err_code!=0)
				{
					result=SONORK_RESULT_STORAGE_ERROR;
					break;
				}
				phys_size=sizeof(db.header.signature);
				if( !header_was_reseted )
				{
					goto reset_header;
				}
			}
			else
			{
				if(phys_size<sizeof(DWORD))
				{
					goto write_signature;
				}
				file.err_code=SONORK_IO_ReadFile(file.data,&db.header.signature,sizeof(db.header.signature));
				if(file.err_code!=0)
				{
					result=SONORK_RESULT_STORAGE_ERROR;
					break;
				}
				if(db.header.signature!=SONORK_FILE_SIGNATURE)
				{
					goto write_signature;
				}
			}
			if(db.header.data_size+sizeof(DWORD)>phys_size)
				goto write_signature;
		}
		result=SONORK_RESULT_OK;
		break;
	}
	if(result==SONORK_RESULT_OK)
	{
		db_reset = header_was_reseted;
		db.open=true;
	}
	else
		Close();
	return result;
}

int TSonorkAtomDb::FlushIndexHeader()
{
	if(file.index!=SONORK_INVALID_FILE_HANDLE)
	{
		file.err_code=SONORK_IO_SetFilePointer(file.index,0,SONORK_FILE_SEEK_BEGIN);
		if(file.err_code)
			return file.err_code;
		file.err_code=SONORK_IO_WriteFile(file.index,&db.header,sizeof(db.header));
	}
	else
		file.err_code=-1;
    return file.err_code;

}
int TSonorkAtomDb::SetIndexHandleAtEntry(UINT entry_no)
{
    DWORD offset;
	offset=sizeof(TSonorkAtomDbHeader)+sizeof(TSonorkAtomDbEntry)*entry_no;
	return SONORK_IO_SetFilePointer(file.index,offset,SONORK_FILE_SEEK_BEGIN);
}

int
 TSonorkAtomDb::SetHandleAtEof()
{
	file.err_code=SetIndexHandleAtEntry(db.header.items);
    if(file.err_code)return file.err_code;
	file.err_code=SONORK_IO_SetFilePointer(
	file.data,sizeof(DWORD)+db.header.data_size,SONORK_FILE_SEEK_BEGIN);
    return file.err_code;
}

int
 TSonorkAtomDb::LoadEntry(DWORD entry_no,TSonorkAtomDbEntry *E,bool set_data_pos)
{
	file.err_code=SetIndexHandleAtEntry(entry_no);
	if(file.err_code)
		return file.err_code;
	file.err_code=SONORK_IO_ReadFile(file.index,E,sizeof(db.entry));
	if(file.err_code)
		return file.err_code;
	if(set_data_pos)
	{
		file.err_code=SONORK_IO_SetFilePointer(file.data
			,sizeof(DWORD)+E->offset
			,SONORK_FILE_SEEK_BEGIN);
	}
	return file.err_code;
}
SONORK_RESULT
	TSonorkAtomDb::AddRaw(TSonorkDynData& DD,TSonorkTag& tag,DWORD& new_index)
{
	SONORK_RESULT 	result;
	new_index = SONORK_INVALID_INDEX;
	if(SetHandleAtEof()==0)
	{
		db.entry.offset     =db.header.data_size;
		db.entry.size       =DD.DataSize();
		if( db.entry.size > SONORK_DB_ENTRY_SIZE_MASK )
			result=SONORK_RESULT_CODEC_ERROR;
		else
		{
			db.entry.tag.Set(tag);
			if(WriteEntry(&db.entry)==0)
			{
				Sonork_SimpleEncrypt(DD.wBuffer(),db.entry.size,db.entry.offset+db.entry.size);
				if((file.err_code=SONORK_IO_WriteFile(file.data,DD.Buffer(),db.entry.size))==0)
				{
					db.header.items++;
					db.header.data_size+=db.entry.size;
					if(FlushIndexHeader()==0)
					{
						new_index=db.header.items-1;
						result=SONORK_RESULT_OK;
					}
					else
					{
						db.header.items--;
						db.header.data_size-=db.entry.size;
						result=SONORK_RESULT_STORAGE_ERROR;
					}
				}
				else
					result=SONORK_RESULT_STORAGE_ERROR;
			}
			else
				result=SONORK_RESULT_STORAGE_ERROR;
		}
	}
	else
		result=SONORK_RESULT_STORAGE_ERROR;

	return result;
}

SONORK_RESULT
	TSonorkAtomDb::GetEntryStatus(DWORD index, DWORD& size , BOOL& deleted )
{
	if(LoadEntry(index,&db.entry,false)!=0)
	{
		return SONORK_RESULT_STORAGE_ERROR;
	}
	deleted = db.entry.size&SONORK_DB_ENTRY_SIZE_FLAG_DELETED;
	size = db.entry.size&SONORK_DB_ENTRY_SIZE_MASK;
	return SONORK_RESULT_OK;
}

SONORK_RESULT
	TSonorkAtomDb::GetRaw(DWORD index, TSonorkDynData& DD,TSonorkTag& tag)
{
	if(LoadEntry(index,&db.entry,true)!=0)
	{
		return SONORK_RESULT_STORAGE_ERROR;
	}
	if(db.entry.size&SONORK_DB_ENTRY_SIZE_FLAG_DELETED)
	{
		return SONORK_RESULT_INVALID_PARAMETER;
	}
	tag.Set(db.entry.tag);
	db.entry.size&=SONORK_DB_ENTRY_SIZE_MASK;
	DD.SetDataSize( db.entry.size , true);
	if((file.err_code = SONORK_IO_ReadFile(file.data,DD.wBuffer(),db.entry.size)) != 0)
	{
		return SONORK_RESULT_STORAGE_ERROR;
	}
	Sonork_SimpleUncrypt(DD.wBuffer(),db.entry.size,db.entry.offset+db.entry.size);
	return SONORK_RESULT_OK;
}

SONORK_RESULT
	TSonorkAtomDb::Add(const TSonorkCodecAtom*ATOM,DWORD*p_new_index,TSonorkTag*tag)
{
	SONORK_RESULT 	result;
	DWORD		new_index = SONORK_INVALID_INDEX;
	if(SetHandleAtEof()==0)
	{
		db.entry.offset     =db.header.data_size;
		db.entry.size       =ATOM->CODEC_Size();
		if( db.entry.size > SONORK_DB_ENTRY_SIZE_MASK )
			result=SONORK_RESULT_CODEC_ERROR;
		else
		{
			if(tag != NULL )
				db.entry.tag.Set(*tag);
			else
				db.entry.tag.Clear();
			if(WriteEntry(&db.entry)==0)
			{
				if((result=CODEC_WriteFileStream(file.data
					, ATOM
					, db.entry.size
					, &file.err_code
					, db.entry.offset+db.entry.size))==SONORK_RESULT_OK)
				{
					db.header.items++;
					db.header.data_size+=db.entry.size;
					if(FlushIndexHeader()==0)
					{
						new_index=db.header.items-1;
						result=SONORK_RESULT_OK;
					}
					else
					{
						db.header.items--;
						db.header.data_size-=db.entry.size;
						result=SONORK_RESULT_STORAGE_ERROR;
					}
				}
			}
			else
				result=SONORK_RESULT_STORAGE_ERROR;
		}
	}
	else
		result=SONORK_RESULT_STORAGE_ERROR;

	if(p_new_index)*p_new_index=new_index;
	return result;
}

SONORK_RESULT
 TSonorkAtomDb::SetValue(SONORK_ATOM_DB_VALUE idx, DWORD value)
{
	db.header.value[idx] = value;
	return FlushIndexHeader()==0
				?SONORK_RESULT_OK
				:SONORK_RESULT_STORAGE_ERROR;
}

SONORK_RESULT
 TSonorkAtomDb::SetTag(DWORD index, TSonorkTag& tag)
{
	if(LoadEntry(index,&db.entry,false)!=0)
		return SONORK_RESULT_STORAGE_ERROR;

	if(db.entry.size&SONORK_DB_ENTRY_SIZE_FLAG_DELETED)
		return SONORK_RESULT_INVALID_PARAMETER;
		
	db.entry.tag.Set(tag);
	file.err_code=SetIndexHandleAtEntry(index);
	if(file.err_code == 0)
	{
		file.err_code=SONORK_IO_WriteFile(file.index,&db.entry,sizeof(db.entry));
		if(file.err_code==0)
			return SONORK_RESULT_OK;
	}
	return SONORK_RESULT_STORAGE_ERROR;
}

SONORK_RESULT
 TSonorkAtomDb::Get(DWORD index,TSonorkCodecAtom*ATOM,TSonorkTag*tag)
{
	if( !ATOM && !tag)
		return SONORK_RESULT_INVALID_PARAMETER;
	if(LoadEntry(index,&db.entry,ATOM!=NULL)!=0)
	{
		return SONORK_RESULT_STORAGE_ERROR;
	}
	if(db.entry.size&SONORK_DB_ENTRY_SIZE_FLAG_DELETED)
	{
		return SONORK_RESULT_INVALID_PARAMETER;
	}
	if(tag != NULL )
		tag->Set(db.entry.tag);
	if( ATOM != NULL )
	{
		db.entry.size&=SONORK_DB_ENTRY_SIZE_MASK;
		return CODEC_ReadFileStream(file.data
					,ATOM
					,db.entry.size
					,&file.err_code
					,db.entry.offset+db.entry.size);
	}
	else
		return SONORK_RESULT_OK;
}
SONORK_RESULT
	TSonorkAtomDb::Flush()
{
	if(!IsOpen())
		return SONORK_RESULT_INVALID_OPERATION;
	SONORK_IO_Flush(file.data);
	SONORK_IO_Flush(file.index);
	return SONORK_RESULT_OK;

}
SONORK_RESULT
	TSonorkAtomDb::Del(DWORD index)
{
	if(LoadEntry(index,&db.entry,true)!=0)
		return SONORK_RESULT_STORAGE_ERROR;
	if(!(db.entry.size&SONORK_DB_ENTRY_SIZE_FLAG_DELETED))
	{
		db.entry.tag.Clear();
		db.entry.size|=SONORK_DB_ENTRY_SIZE_FLAG_DELETED;
		file.err_code=SetIndexHandleAtEntry( index );
		if(file.err_code != 0)
			return SONORK_RESULT_STORAGE_ERROR;
		if(WriteEntry(&db.entry)==0)
		{
			db.entry.size&=SONORK_DB_ENTRY_SIZE_MASK;
			TSonorkTempBuffer buffer(db.entry.size);
			SONORK_ZeroMem(buffer.Ptr(),db.entry.size);
			SONORK_IO_WriteFile(file.data,buffer.Ptr(),db.entry.size);
			if( index == db.header.items - 1)
			{
				db.header.items--;
				db.header.data_size-=db.entry.size;
				FlushIndexHeader();
			}
			return SONORK_RESULT_OK;
		}
		else
			return SONORK_RESULT_STORAGE_ERROR;
	}
	return SONORK_RESULT_OK;
}

