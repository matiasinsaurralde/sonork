#include "srk_defs.h"
#pragma hdrstop
#include "srk_bf_file.h"
#include "srk_codec_io.h"
#include "srk_crypt.h"
#include "srk_file_io.h"

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

#define ENTRY_HASH_TABLE_SIZE   512


#if defined(SONORK_LINUX_BUILD)
static const char *IdxNameFmt="%s/%s.bfidx";
static const char *DatNameFmt="%s/%s.bfdat";
#else
static const char *IdxNameFmt="%s\\%s.bfidx";
static const char *DatNameFmt="%s\\%s.bfdat";
#endif

TSonorkBfFile::TSonorkBfFile()
{
    file.index_handle=
    file.data_handle=SONORK_INVALID_FILE_HANDLE;
}
TSonorkBfFile::~TSonorkBfFile()
{
	Close();
}

TSonorkBfEnumHandle*
 TSonorkBfFile::OpenEnum(TSonorkBfBlock* dir_block,const char *name, DWORD flags)
{
	if(!IsOpen())
	{
		last_result=SONORK_RESULT_INVALID_OPERATION;
	}
	else
	{
		TSonorkBfBlock*B;
		TSonorkBfEnumHandle *E;
		if(dir_block||name)
		{
			B=OpenDirBlock(dir_block,name,false);
			if(B)
			{
				SONORK_MEM_NEW(E=new TSonorkBfEnumHandle(B,flags));
				SONORK_MEM_DELETE(B);
				return E;
			}
		}
		else
		{
			SONORK_MEM_NEW(E=new TSonorkBfEnumHandle(SONORK_BF_INVALID_ENTRY_NO,flags));
			return E;
		}
	}
	return NULL;
}
TSonorkBfBlock*
 TSonorkBfFile::OpenBlock(
	  SONORK_BF_BLOCK_TYPE block_type
	, DWORD parent_entry_no
	, const char*name
	, bool read_data
	, bool allow_create
	, UINT atom_type
	, bool*p_created)
{
	TSonorkBfDirEntry rE(block_type,name,parent_entry_no);
	TSonorkBfBlock*B;
	DWORD entry_no;

	if(!IsOpen())
	{
		last_result=SONORK_RESULT_INVALID_OPERATION;
		return NULL;
	}

	last_result=GetEntryByName(entry_no,&rE);
	if(last_result!=SONORK_RESULT_OK)
	{
		if(last_result==SONORK_RESULT_NO_DATA&&allow_create)
		{
			if(p_created)*p_created=true;
			return CreateBlock(block_type,parent_entry_no,name,atom_type,false);
		}
		return NULL;
	}
	if(p_created)
		*p_created=false;

	SONORK_MEM_NEW(B=new TSonorkBfBlock(entry_no,&rE));
	if(rE.type==SONORK_BF_BLOCK_TYPE_DATA && read_data)
		last_result=ReadBlockData(&rE,B);

	if(last_result!=SONORK_RESULT_OK)
	{
		SONORK_MEM_DELETE(B);
		B=NULL;
	}
	return B;
}

SONORK_RESULT
 TSonorkBfFile::Read(TSonorkBfBlock* dir_block, const char*name, TSonorkShortString*S)
{
	TSonorkCodecShortStringAtom tA(S);
	return Read(dir_block,name,&tA);
}
SONORK_RESULT
 TSonorkBfFile::Write(TSonorkBfBlock* dir_block,const char*name, const TSonorkShortString*S)
{
	TSonorkCodecShortStringAtom tA((TSonorkShortString*)S);
	return Write(dir_block,name,&tA);

}
SONORK_RESULT
 TSonorkBfFile::Read(TSonorkBfBlock* dir_block, const char *name, TSonorkCodecAtom*S)
{
	TSonorkBfBlock* B;
	B=ReadDataBlock(dir_block,name);
	if( B != NULL )
	{
		DWORD sz=B->DataSize();
		last_result=S->CODEC_ReadMem(B->Data(),sz);
		SONORK_MEM_DELETE(B);
	}
	else
		S->CODEC_Clear();
	return Result();
}

SONORK_RESULT
 TSonorkBfFile::Write(TSonorkBfBlock* dir_block, const char*name, const TSonorkCodecAtom*S)
{
	TSonorkBfBlock* B;
	B=CreateDataBlock(dir_block
		,name
		,S->CODEC_DataType()
		,true);
	if( B != NULL )
	{
		DWORD sz=S->CODEC_Size();
		B->SetData(NULL,sz);
		if((last_result=S->CODEC_WriteMem(B->Data(),sz))==SONORK_RESULT_OK)
		{
			WriteDataBlock(B,S->CODEC_DataType());
		}

		SONORK_MEM_DELETE(B);
	}
	return Result();
}

SONORK_RESULT
 TSonorkBfFile::WriteRaw(TSonorkBfBlock* dir_block, const char*name,const void*data,UINT sz, SONORK_ATOM_TYPE atom_type)
{
	TSonorkCodecRawAtom tA((void*)data,sz,atom_type);
	return Write(dir_block,name,&tA);
}
SONORK_RESULT
 TSonorkBfFile::ReadRaw(TSonorkBfBlock* dir_block, const char*name,void*data,UINT sz,SONORK_ATOM_TYPE atom_type)
{
	TSonorkCodecRawAtom tA(data,sz,atom_type);
	return Read(dir_block,name,&tA);
}


SONORK_RESULT TSonorkBfFile::Open(const char *dir,const char *name, bool allow_create)
{
	TSonorkTempBuffer path(SONORK_MAX_PATH);
	DWORD	real_entries;

	Close();

	sprintf(path.CStr(),IdxNameFmt,dir,name);
	file.index_handle=
		SONORK_IO_OpenFile(
			path.CStr()
			,SONORK_OPEN_FILE_METHOD_OPEN_EXISTING
			,&file.err_code
			,NULL);

	if(file.index_handle==SONORK_INVALID_FILE_HANDLE)
		last_result=SONORK_RESULT_STORAGE_ERROR;
	else
	if((file.err_code=ReadHeader())!=0)
		last_result=SONORK_RESULT_STORAGE_ERROR;
	else
	if(file.header.version>SONORK_BF_FILE_VERSION)
		last_result=SONORK_RESULT_INVALID_VERSION;
	else
	{
		if(SONORK_IO_GetFileSize( file.index_handle , &real_entries)==0)
		{
			real_entries = ( real_entries - sizeof(file.header) ) / sizeof(TSonorkBfDirEntry);
			if( real_entries < file.header.entries )
				file.header.entries = real_entries;
		}
		sprintf(path.CStr(),DatNameFmt,dir,name);

		file.data_handle=
			SONORK_IO_OpenFile(
				path.CStr()
				,SONORK_OPEN_FILE_METHOD_OPEN_EXISTING
				,&file.err_code
				,NULL);
		if(file.index_handle==SONORK_INVALID_FILE_HANDLE)
			last_result=SONORK_RESULT_STORAGE_ERROR;
		else
			last_result=SONORK_RESULT_OK;
	}

	if(last_result!=SONORK_RESULT_OK)
	{
		CloseFile(false);
		if(allow_create)
			last_result=Create(dir,name);
	}
	return last_result;
}

SONORK_RESULT
 TSonorkBfFile::Create(const char *dir,const char *name)
{
	TSonorkTempBuffer path(SONORK_MAX_PATH);
	Close();

	sprintf(path.CStr(),IdxNameFmt,dir,name);
	file.index_handle=SONORK_IO_OpenFile(
				 path.CStr()
				,SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS
				,&file.err_code
				,NULL);
	if(file.index_handle==SONORK_INVALID_FILE_HANDLE)
		last_result=SONORK_RESULT_STORAGE_ERROR;
	else
	{
		SONORK_ZeroMem(&file.header,sizeof(file.header));
		file.header.version	=SONORK_BF_FILE_VERSION;
		file.header.block_size	=SONORK_BF_BLOCK_SIZE;
		file.header.entries	=(ENTRY_HASH_TABLE_SIZE*2);
		file.header.salt        =time(NULL);

		// CHANGED FOR TESTS WriteHeader(true->false)
		if((file.err_code=WriteHeader( false ))!=0)
			last_result=SONORK_RESULT_STORAGE_ERROR;
		else
		{
			TSonorkBfDirEntry E;
			DWORD entry_no;
			last_result=SONORK_RESULT_OK;
			E.atom_type=SONORK_ATOM_NULL;
			for(entry_no=0;entry_no<(ENTRY_HASH_TABLE_SIZE*2);entry_no++)
				if((file.err_code=WriteEntry(entry_no,&E))!=0)
				{
					last_result=SONORK_RESULT_STORAGE_ERROR;
					break;
				}
		}
		if(last_result==SONORK_RESULT_OK)
		{
			sprintf(path.CStr(),DatNameFmt,dir,name);
			file.data_handle=SONORK_IO_OpenFile(
				path.CStr()
				,SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS
				,&file.err_code
				,NULL);
			if(file.index_handle==SONORK_INVALID_FILE_HANDLE)
				last_result=SONORK_RESULT_STORAGE_ERROR;
			else
				last_result=SONORK_RESULT_OK;
		}
	}
	if(last_result!=SONORK_RESULT_OK)
	CloseFile(false);
    return last_result;
}
int
 TSonorkBfFile::WriteEntry(DWORD entry_no,TSonorkBfDirEntry*E)
{
	int rv;
	rv=SONORK_IO_SetFilePointer(file.index_handle
			,sizeof(file.header)+(entry_no*sizeof(*E)) );

	if( rv == 0 )
	{
		rv=SONORK_IO_WriteFile(file.index_handle,E,sizeof(*E));
	}
	return rv;
}
int
 TSonorkBfFile::ReadEntry(DWORD entry_no,TSonorkBfDirEntry*E)
{
	int rv;
	rv=SONORK_IO_SetFilePointer(file.index_handle
			,sizeof(file.header)+(entry_no*sizeof(*E)) );
	if( rv == 0 )
	{
		rv=SONORK_IO_ReadFile(file.index_handle,E,sizeof(*E));
	}
    return rv;
}

int
 TSonorkBfFile::WriteHeader( bool flush )
{
	int rv;
	if((rv=SONORK_IO_SetFilePointer(file.index_handle,0))==0)
	{
		rv=SONORK_IO_WriteFile(file.index_handle,&file.header,sizeof(file.header));
		if( flush )
			Flush();
	}
	return rv;
}
void
 TSonorkBfFile::Flush()
{
	SONORK_IO_Flush(file.index_handle);
	SONORK_IO_Flush(file.data_handle);
}

int
 TSonorkBfFile::ReadHeader()
{
	int rv;
	if((rv=SONORK_IO_SetFilePointer(file.index_handle,0))==0)
		rv=SONORK_IO_ReadFile(file.index_handle,&file.header,sizeof(file.header));
	return rv;
}

void
 TSonorkBfFile::CloseFile(bool flush)
{
	if(file.index_handle!=SONORK_INVALID_FILE_HANDLE)
	{
		if(flush)WriteHeader( true );
		SONORK_IO_CloseFile(file.index_handle);
		file.index_handle=SONORK_INVALID_FILE_HANDLE;
	}
	if(file.data_handle!=SONORK_INVALID_FILE_HANDLE)
	{
		SONORK_IO_CloseFile(file.data_handle);
		file.data_handle=SONORK_INVALID_FILE_HANDLE;
	}
}

int
 TSonorkBfFile::ReadBlockHeader(DWORD block_no,TSonorkBfBlockHeader*H)
{
	int rv;
	rv=SONORK_IO_SetFilePointer(file.data_handle,file.header.block_size*block_no,SONORK_FILE_SEEK_BEGIN);
	if(rv!=0)return rv;
	return SONORK_IO_ReadFile(file.data_handle,H,sizeof(*H));
}

int
 TSonorkBfFile::WriteBlockHeader(DWORD block_no,TSonorkBfBlockHeader*H)
{
	int rv;
	rv=SONORK_IO_SetFilePointer(file.data_handle,file.header.block_size*block_no,SONORK_FILE_SEEK_BEGIN);
	if(rv!=0)return rv;
	return SONORK_IO_WriteFile(file.data_handle,H,sizeof(*H));
}


SONORK_RESULT
 TSonorkBfFile::ReadBlockData(TSonorkBfDirEntry*E,TSonorkBfBlock*B)
{
	DWORD 			cur_block_no;
	DWORD 			remaining_bytes,bytes;
	BYTE*			data_ptr;
	TSonorkBfBlockHeader	block_header;
	UINT			seq_no;

	remaining_bytes	=B->DataSize();
	data_ptr		=B->Data();
	cur_block_no	=E->first_block_no;
	for(seq_no=0;remaining_bytes>0;seq_no++)
	{
		if((file.err_code=ReadBlockHeader(cur_block_no,&block_header))!=0)
			return SONORK_RESULT_STORAGE_ERROR;
		if(block_header.dir_entry_no!=B->dir_entry_no)
		{
			file.err_code=SONORK_MODULE_LINE;
			return SONORK_RESULT_INTERNAL_ERROR;
		}
		if(block_header.seq_no!=seq_no)
		{
			file.err_code=SONORK_MODULE_LINE;
			return SONORK_RESULT_INTERNAL_ERROR;
		}
		bytes=SONORK_BF_DATA_SIZE;
		if(bytes>remaining_bytes)
			bytes=remaining_bytes;
		if((file.err_code=SONORK_IO_ReadFile(file.data_handle,data_ptr,bytes))!=0)
			return SONORK_RESULT_STORAGE_ERROR;
		Sonork_SimpleUncrypt(data_ptr,bytes,file.header.salt);
		data_ptr+=bytes;
		remaining_bytes-=bytes;
		cur_block_no=block_header.next_block_no;
	}
	return SONORK_RESULT_OK;
}

SONORK_RESULT
 TSonorkBfFile::DeleteBlockData(DWORD block_no)
{
	TSonorkBfBlockHeader	block_header;
	DWORD 				del_block_no;
	SONORK_RESULT 		result;
	TSonorkTempBuffer		w_buffer(SONORK_BF_DATA_SIZE);

	SONORK_ZeroMem(w_buffer.Ptr(),SONORK_BF_DATA_SIZE);
	result=SONORK_RESULT_OK;
	while(block_no!=SONORK_BF_INVALID_BLOCK_NO)
	{
		if((file.err_code=ReadBlockHeader(block_no,&block_header))!=0)
		{
			result=SONORK_RESULT_STORAGE_ERROR;
			break;
		}

		if(block_no<file.header.first_free_block)
			file.header.first_free_block=block_no;

		del_block_no=block_no;
		block_no=block_header.next_block_no;

		block_header.next_block_no	=SONORK_BF_INVALID_BLOCK_NO;
		block_header.dir_entry_no	=SONORK_BF_INVALID_ENTRY_NO;
		block_header.seq_no			=0;

		if((file.err_code=WriteBlockHeader(del_block_no,&block_header))!=0)
		{
			result=SONORK_RESULT_STORAGE_ERROR;
			break;
		}
		if((file.err_code=SONORK_IO_WriteFile(file.data_handle,w_buffer.Ptr(),SONORK_BF_DATA_SIZE))!=0)
		{
			result=SONORK_RESULT_STORAGE_ERROR;
			break;
		}

		file.header.allocated_blocks--;
	}
	return result;
}


SONORK_RESULT
 TSonorkBfFile::WriteBlockData(TSonorkBfDirEntry*E,TSonorkBfBlock*B)
{
	DWORD 			prev_block_no,cur_block_no;
	BYTE  			*data_ptr;
	TSonorkBfBlockHeader	cur_block_header;
	TSonorkBfBlockHeader	prev_block_header;
	UINT               	remaining_bytes;

	prev_block_no		=SONORK_BF_INVALID_BLOCK_NO;
	remaining_bytes		=B->AllocSize();
	data_ptr		=B->Data();
	cur_block_no		=E->first_block_no;
	cur_block_header.dir_entry_no =(WORD)B->dir_entry_no;

	last_result=SONORK_RESULT_OK;
	{
		TSonorkTempBuffer w_buffer(SONORK_BF_DATA_SIZE);
		for(UINT seq_no=0;;seq_no++)
		{

			if(cur_block_no==SONORK_BF_INVALID_BLOCK_NO)
			{
				cur_block_header.seq_no=seq_no;
				cur_block_no=AllocBlock(&cur_block_header);
				if(cur_block_no==SONORK_BF_INVALID_BLOCK_NO)
					break; // last_result loaded by AllocBlock

				if(seq_no==0)
				{
					E->first_block_no=cur_block_no;
				}
				if(prev_block_no==SONORK_BF_INVALID_BLOCK_NO)
				{
					goto write_data;
				}
				else
				{
					prev_block_header.next_block_no=cur_block_no;

					if((file.err_code=WriteBlockHeader(prev_block_no,&prev_block_header))!=0)
					{
						last_result=SONORK_RESULT_STORAGE_ERROR;
						break;
					}
				}

			}

			if((file.err_code=ReadBlockHeader(cur_block_no,&cur_block_header))!=0)
			{
				last_result=SONORK_RESULT_STORAGE_ERROR;
				break;
			}

write_data:

			if(cur_block_header.dir_entry_no!=B->dir_entry_no)
			{
				file.err_code=SONORK_MODULE_LINE;
				last_result=SONORK_RESULT_INTERNAL_ERROR;
				break;
			}

			if(cur_block_header.seq_no!=seq_no)
			{
				file.err_code=SONORK_MODULE_LINE;
				last_result=SONORK_RESULT_INTERNAL_ERROR;
				break;
			}
			if(remaining_bytes)
			{
				memcpy(w_buffer.Ptr(),data_ptr,SONORK_BF_DATA_SIZE);
				Sonork_SimpleEncrypt(w_buffer.Ptr(),SONORK_BF_DATA_SIZE,file.header.salt);
				if((file.err_code=SONORK_IO_WriteFile(file.data_handle,w_buffer.Ptr(),SONORK_BF_DATA_SIZE))!=0)
				{
					last_result=SONORK_RESULT_STORAGE_ERROR;
					break;
				}
			}
			data_ptr+=SONORK_BF_DATA_SIZE;
			remaining_bytes-=SONORK_BF_DATA_SIZE;

			prev_block_no=cur_block_no;
			cur_block_no=cur_block_header.next_block_no;
			if(!remaining_bytes)
			{
				if(cur_block_header.next_block_no!=SONORK_BF_INVALID_BLOCK_NO)
				{
					cur_block_header.next_block_no=SONORK_BF_INVALID_BLOCK_NO;
					if((file.err_code=WriteBlockHeader(prev_block_no,&cur_block_header))!=0)
					{
						last_result=SONORK_RESULT_STORAGE_ERROR;
						break;
					}
				}
				break;
			}
			else
			{
				memcpy(&prev_block_header,&cur_block_header,sizeof(cur_block_header));
			}

		}
	}

	if(last_result==SONORK_RESULT_OK)
	{
		if(cur_block_no!=SONORK_BF_INVALID_BLOCK_NO)
		{
			DeleteBlockData(cur_block_no);
		}

		E->data_size=B->DataSize();

		if((file.err_code=WriteEntry(B->dir_entry_no,E))!=0)
			last_result=SONORK_RESULT_STORAGE_ERROR;
		else
		if((file.err_code=WriteHeader( false ))!=0)
			last_result=SONORK_RESULT_STORAGE_ERROR;
		else
			last_result=SONORK_RESULT_OK;
	}
	return last_result;
}


SONORK_RESULT
 TSonorkBfFile::WriteDataBlock(TSonorkBfBlock*B,UINT atom_type)
{
	TSonorkBfDirEntry E;
	if(!IsOpen())
		return (last_result=SONORK_RESULT_INVALID_OPERATION);
	last_result=GetEntryByNo(B->dir_entry_no,&E);
	if(last_result!=SONORK_RESULT_OK)
		return last_result;
	if(B->first_block_no!=E.first_block_no || E.type!=SONORK_BF_BLOCK_TYPE_DATA)
		return (last_result=SONORK_RESULT_INVALID_PARAMETER);
	if(E.parent_entry_no==SONORK_BF_INVALID_ENTRY_NO)
		return (last_result=SONORK_RESULT_INTERNAL_ERROR);
	E.atom_type=(DWORD)atom_type;
	return (last_result=WriteBlockData(&E,B));
}

SONORK_RESULT
 TSonorkBfFile::DeleteBlock(SONORK_BF_BLOCK_TYPE type
		, DWORD parent_entry_no
		, const char*name
		, DWORD depth)
{
	TSonorkBfDirEntry nE(type,name,parent_entry_no);
	DWORD entry_no;
	if(!IsOpen())
	{
		return (last_result=SONORK_RESULT_INVALID_OPERATION);
	}
	if((last_result=GetEntryByName(entry_no,&nE))!=SONORK_RESULT_OK)
		return last_result;
	if(entry_no==parent_entry_no)
		return (last_result=SONORK_RESULT_INTERNAL_ERROR);

	if(nE.type==SONORK_BF_BLOCK_TYPE_DATA)
		last_result=DeleteBlockData(nE.first_block_no);
	else
	{
		TSonorkBfEnumHandle EN(entry_no,SONORK_BF_ENUM_DIR|SONORK_BF_ENUM_DATA);
		while((last_result=EnumNext(&EN))==SONORK_RESULT_OK)
		{
			if((last_result=DeleteBlock( EN.Type() , entry_no , EN.Name() , depth+1))!=SONORK_RESULT_OK)
				return last_result;
		}
		if(last_result!=SONORK_RESULT_NO_DATA)
			return last_result;
	}
	last_result=DeleteEntry(entry_no,&nE);

//	if( depth == 0 )	Flush();
	return last_result;
}


TSonorkBfBlock*
 TSonorkBfFile::CreateBlock(
	  SONORK_BF_BLOCK_TYPE 		type
	, DWORD 			parent_entry_no
	, const char*			name
	, UINT				atom_type
	, bool 				overwrite)
{
	TSonorkBfDirEntry nE(type,name,parent_entry_no);
	TSonorkBfDirEntry pE;
	TSonorkBfBlock*B=NULL;
	DWORD entry_no;
	if(!IsOpen())
	{
		last_result=SONORK_RESULT_INVALID_OPERATION;
		return NULL;
	}
	if(nE.type==SONORK_BF_BLOCK_TYPE_NONE)
	{
		last_result=SONORK_RESULT_INVALID_PARAMETER;
		return NULL;
	}
	last_result=GetEntryByName(entry_no,&nE);

	if(last_result==SONORK_RESULT_OK)
	{
		if(!overwrite||type!=nE.type)
		{
			last_result=SONORK_RESULT_DUPLICATE_DATA;
		}
		else
		{
			SONORK_MEM_NEW(B=new TSonorkBfBlock(entry_no,&nE));
		}
	}
	else
	if(last_result==SONORK_RESULT_NO_DATA)
	{
		nE.atom_type=atom_type;
		last_result=AllocEntry(&nE,entry_no);
		if(last_result==SONORK_RESULT_OK)
			SONORK_MEM_NEW(B=new TSonorkBfBlock(entry_no,&nE));
	}
	if( last_result!=SONORK_RESULT_OK && B!=NULL )
	{
		SONORK_MEM_DELETE(B);
		B=NULL;
	}
    return B;
}

SONORK_RESULT
 TSonorkBfFile::GetEntryByNo(DWORD entry_no,TSonorkBfDirEntry*E)
{
	if((file.err_code=ReadEntry(entry_no,E))!=0)
		return (last_result=SONORK_RESULT_STORAGE_ERROR);

	return (last_result=SONORK_RESULT_OK);
}

SONORK_RESULT
 TSonorkBfFile::GetEntryByName(DWORD&entry_no,TSonorkBfDirEntry*E)
{
	TSonorkBfDirEntry scanE;

	entry_no=E->GetHashValue();
	while(entry_no!=SONORK_BF_INVALID_ENTRY_NO)
	{
		if((file.err_code=ReadEntry(entry_no,&scanE))!=0)
		{
			return (last_result=SONORK_RESULT_STORAGE_ERROR);
		}

		if(scanE.type!=SONORK_BF_BLOCK_TYPE_NONE)
		{
#if defined(SONORK_LINUX_BUILD)
			if(!strncasecmp(E->name, scanE.name, sizeof(E->name))
#else
			if(!strnicmp(E->name,scanE.name,sizeof(E->name))
#endif
			&&  E->parent_entry_no==scanE.parent_entry_no)
			{
				memcpy(E,&scanE,sizeof(scanE));
				return (last_result=SONORK_RESULT_OK);
			}
		}
		entry_no=scanE.next_entry_no;
	}
	return (last_result=SONORK_RESULT_NO_DATA);
}

SONORK_RESULT
 TSonorkBfFile::DeleteEntry(DWORD entry_no,TSonorkBfDirEntry*E)
{
	if((file.err_code=ReadEntry(entry_no,E))!=0)
		return (last_result=SONORK_RESULT_STORAGE_ERROR);

	if(E->type==SONORK_BF_BLOCK_TYPE_NONE)
		return (last_result=SONORK_RESULT_NO_DATA);

	E->type=SONORK_BF_BLOCK_TYPE_NONE;
	E->parent_entry_no=SONORK_BF_INVALID_ENTRY_NO;

	if((file.err_code=WriteEntry(entry_no,E))!=0)
		return (last_result=SONORK_RESULT_STORAGE_ERROR);

	file.header.allocated_entries--;
	if((file.err_code=WriteHeader( false ))!=0)
		return (last_result=SONORK_RESULT_STORAGE_ERROR);

	return (last_result=SONORK_RESULT_OK);

}

SONORK_RESULT TSonorkBfFile::AllocEntry(TSonorkBfDirEntry*newE,DWORD&entry_no)
{
	DWORD prev_entry_no,new_entry_no;
	TSonorkBfDirEntry 	prevE,scanE;

	prev_entry_no=SONORK_BF_INVALID_ENTRY_NO;
	new_entry_no=newE->GetHashValue();
	if(new_entry_no==SONORK_BF_INVALID_ENTRY_NO)
	{
		return (last_result=SONORK_RESULT_INTERNAL_ERROR);
	}
	while(new_entry_no!=SONORK_BF_INVALID_ENTRY_NO)
	{
		if((file.err_code=ReadEntry(new_entry_no,&scanE))!=0)
			return (last_result=SONORK_RESULT_STORAGE_ERROR);
		if(scanE.type==SONORK_BF_BLOCK_TYPE_NONE)
			break;
		prev_entry_no=new_entry_no;
		memcpy(&prevE,&scanE,sizeof(prevE));
		new_entry_no=scanE.next_entry_no;
	}

	if(new_entry_no==SONORK_BF_INVALID_ENTRY_NO)
	{
		new_entry_no=file.header.entries;
		newE->next_entry_no	=SONORK_BF_INVALID_ENTRY_NO;
	}
	else
		newE->next_entry_no	=scanE.next_entry_no;

	newE->first_block_no=SONORK_BF_INVALID_BLOCK_NO;
	newE->data_size		=0;

	if((file.err_code=WriteEntry(new_entry_no,newE))!=0)
	return (last_result=SONORK_RESULT_STORAGE_ERROR);
	if(prev_entry_no!=SONORK_BF_INVALID_ENTRY_NO)
	{
		if(prevE.next_entry_no!=new_entry_no)
		{
			prevE.next_entry_no=new_entry_no;
			if((file.err_code=WriteEntry(prev_entry_no,&prevE))!=0)
				return (last_result=SONORK_RESULT_STORAGE_ERROR);
		}
	}
	if(new_entry_no==file.header.entries)
		file.header.entries++;
	file.header.allocated_entries++;

	// CHANGED FOR TESTS WriteHeader(true->false)
	if((file.err_code=WriteHeader( false ))!=0)
		return (last_result=SONORK_RESULT_STORAGE_ERROR);

	entry_no=new_entry_no;
	return SONORK_RESULT_OK;
}

DWORD
 TSonorkBfFile::AllocBlock(TSonorkBfBlockHeader*H)
{
	TSonorkBfBlockHeader tH;
	DWORD block_no;
	while(file.header.first_free_block<file.header.blocks)
	{
		if((file.err_code=ReadBlockHeader(file.header.first_free_block,&tH))!=0)
		{
			goto storage_error;
		}
		if(tH.dir_entry_no==SONORK_BF_INVALID_ENTRY_NO)
			break;
		file.header.first_free_block++;
	}
	block_no=file.header.first_free_block++;
	H->next_block_no=SONORK_BF_INVALID_BLOCK_NO;
	if((file.err_code=WriteBlockHeader(block_no,H))!=0)
	{
storage_error:
		last_result=SONORK_RESULT_STORAGE_ERROR;
		return SONORK_BF_INVALID_BLOCK_NO;
	}
	if(block_no==file.header.blocks)
		file.header.blocks++;
	file.header.allocated_blocks++;
	return block_no;
}


TSonorkBfBlock::TSonorkBfBlock(DWORD p_entry_no,TSonorkBfDirEntry*E)
{
	dir_entry_no	= p_entry_no;
	first_block_no	= E->first_block_no;
	data.ptr	= NULL;
	if( E->type == SONORK_BF_BLOCK_TYPE_DIR )
		data.size = 0;
	else
		SetData(NULL,E->data_size);

}

TSonorkBfBlock::~TSonorkBfBlock()
{
	Clear();
}

void
 TSonorkBfBlock::Clear()
{
	if(data.ptr)
    {
    	SONORK_MEM_FREE(data.ptr);
        data.ptr=NULL;
    }
    data.size=0;
}
void
 TSonorkBfBlock::CalcAllocSize()
{
	int l=data.size+1;
	data.alloc_size=((l/SONORK_BF_DATA_SIZE)*SONORK_BF_DATA_SIZE)+((l%SONORK_BF_DATA_SIZE)?SONORK_BF_DATA_SIZE:0);
}

void
 TSonorkBfBlock::AppendData(const void *D,DWORD l)
{
	UINT p_data_size,p_alloc_size;

	if(!l)return;

	p_data_size  = DataSize();
	p_alloc_size = AllocSize();
	data.size+=l;
	CalcAllocSize();

	if(AllocSize()>p_alloc_size)
	{
		BYTE *p_data=data.ptr;
		data.ptr=SONORK_MEM_ALLOC(BYTE,AllocSize());
		if(p_data)
		{
			if(p_data_size)
			{
				memcpy(data.ptr,p_data,p_data_size);
			}
			SONORK_MEM_FREE(p_data);
		}
	}

	if( D != NULL )
	{
		memcpy(data.ptr+p_data_size,D,l);
	}
}

void
 TSonorkBfBlock::SetData(const void *D,DWORD l)
{
	Clear();
	data.size=l;
	CalcAllocSize();
	data.ptr=SONORK_MEM_ALLOC(BYTE,AllocSize());
	if(data.size&&D)
	{
		memcpy(data.ptr,D,data.size);
	}

}

void
 TSonorkBfDirEntry::Clear()
{
	type=(WORD)SONORK_BF_BLOCK_TYPE_NONE;

	blocks		=0;
	data_size	=0;
	first_block_no	=SONORK_BF_INVALID_BLOCK_NO;

	parent_entry_no	=
	next_entry_no	=SONORK_BF_INVALID_ENTRY_NO;

	name[0]		=0;
}
TSonorkBfDirEntry::TSonorkBfDirEntry()
{
	Clear();
}
TSonorkBfDirEntry::TSonorkBfDirEntry(SONORK_BF_BLOCK_TYPE t,const char*n,DWORD pe)
{
	Clear();
	type=(WORD)t;
	parent_entry_no=pe;
	SONORK_StrCopy(name,sizeof(name),(SONORK_C_STR)n);
}

DWORD
 TSonorkBfDirEntry::GetHashValue()
{
	DWORD rv=0;
	int i=1;
	char *ptr=name;
	if(type==SONORK_BF_BLOCK_TYPE_NONE)
		return SONORK_BF_INVALID_ENTRY_NO;

	while(*ptr)
	{
		rv+=(*ptr-' ')*i;
		i++;ptr++;
	}
	rv+=parent_entry_no;
	return (rv%ENTRY_HASH_TABLE_SIZE)+(type==SONORK_BF_BLOCK_TYPE_DIR?0:ENTRY_HASH_TABLE_SIZE);
}
TSonorkBfEnumHandle::TSonorkBfEnumHandle(DWORD p_parent_entry_no, DWORD p_flags)
{
	cur_entry_no=0;
	parent_entry_no=p_parent_entry_no;
	enum_flags=p_flags;

}

TSonorkBfEnumHandle::TSonorkBfEnumHandle(TSonorkBfBlock*B, DWORD p_flags)
{
	if(B)
		parent_entry_no=B->DirEntryNo();
	else
		parent_entry_no=SONORK_BF_INVALID_ENTRY_NO;

	cur_entry_no=0;
	enum_flags=p_flags;
}
SONORK_RESULT
 TSonorkBfFile::EnumNext(TSonorkBfEnumHandle*EN)
{
	file.err_code=
		SONORK_IO_SetFilePointer(file.index_handle
			,sizeof(file.header)+EN->cur_entry_no*sizeof(TSonorkBfDirEntry));

	if( file.err_code !=0 )
		return SONORK_RESULT_STORAGE_ERROR;

	for(;EN->cur_entry_no<file.header.entries;EN->cur_entry_no++)
	{
		file.err_code=
			SONORK_IO_ReadFile(file.index_handle
			,&EN->entry
			,sizeof(TSonorkBfDirEntry));
		if( file.err_code !=0 )
			return SONORK_RESULT_STORAGE_ERROR;


		if(	EN->entry.type!=SONORK_BF_BLOCK_TYPE_NONE
		&&	EN->entry.parent_entry_no==EN->parent_entry_no)
		{
			if(
			   (EN->entry.type==SONORK_BF_BLOCK_TYPE_DIR
			&& (EN->enum_flags&SONORK_BF_ENUM_DIR)!=0)
			||
			   (EN->entry.type==SONORK_BF_BLOCK_TYPE_DATA
			&& (EN->enum_flags&SONORK_BF_ENUM_DATA)!=0) )
			{
				EN->cur_entry_no++;
				return SONORK_RESULT_OK;
			}
		}
	}
	return SONORK_RESULT_NO_DATA;
}





