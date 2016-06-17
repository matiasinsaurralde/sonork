#if !defined(SONORK_BF_FILE_H)
#define SONORK_BF_FILE_H

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
#include "srk_codec_atom.h"


#define SONORK_BF_FILE_VERSION 				1
#define SONORK_BF_BLOCK_SIZE				256
#define SONORK_BF_DATA_SIZE					(SONORK_BF_BLOCK_SIZE-sizeof(TSonorkBfBlockHeader))
#define SONORK_BF_BLOCK_NAME_LENGTH     	48
#define SONORK_BF_INVALID_BLOCK_NO			((DWORD)-1)
#define SONORK_BF_INVALID_ENTRY_NO 			((DWORD)-1)

enum SONORK_BF_BLOCK_TYPE
{
	SONORK_BF_BLOCK_TYPE_NONE
,	SONORK_BF_BLOCK_TYPE_DIR
,	SONORK_BF_BLOCK_TYPE_DATA
};

#define SONORK_BF_ENUM_DIR	0x1
#define SONORK_BF_ENUM_DATA	0x2

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,2)
#endif

struct TSonorkBfBlockHeader
{
friend class TSonorkBfFile;
friend struct TSonorkBfBlock;
protected:
    DWORD seq_no;
    DWORD dir_entry_no;
    DWORD next_block_no;
public:
};

struct TSonorkBfFileHeader
{
	DWORD   version;
    DWORD   block_size;
	DWORD 	blocks;
    DWORD	allocated_blocks;
    DWORD   first_free_block;
    DWORD   entries;
    DWORD	allocated_entries;
    DWORD   salt;
};

struct TSonorkBfBlock
{
friend class 	TSonorkBfFile;
private:
	DWORD					dir_entry_no;
    DWORD					first_block_no;
    struct {
	    DWORD				alloc_size;
	    DWORD 				size;
		BYTE 				*ptr;
    }data;
    void	CalcAllocSize();
	TSonorkBfBlock(DWORD entry_no,struct TSonorkBfDirEntry*);
public:
	~TSonorkBfBlock();
    DWORD	DirEntryNo(){ return dir_entry_no;}

	BYTE *	Data()	{ return data.ptr; }
    DWORD 	DataSize(){ return data.size;}
    DWORD 	AllocSize(){ return data.alloc_size;}
    void 	Clear();
    void    AppendData(const void *data, DWORD length);
    void 	SetData(const void *data, DWORD length);
};

struct TSonorkBfDirEntry
{
friend class TSonorkBfFile;
friend struct TSonorkBfEnumHandle;
friend struct TSonorkBfBlock;
private:
    DWORD 				parent_entry_no;
    DWORD				first_block_no;
    DWORD				data_size;
    DWORD   			next_entry_no;
    TSonorkTime				time_stamp;
	DWORD				atom_type;
	DWORD				reserved;
	WORD    			type;
	WORD				blocks;
	char				name[SONORK_BF_BLOCK_NAME_LENGTH];
	void 				Clear();
	TSonorkBfDirEntry();
    TSonorkBfDirEntry(SONORK_BF_BLOCK_TYPE type
    		,const char *name
            ,DWORD parent_entry_no);
    DWORD   GetHashValue();

};
#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif

struct TSonorkBfEnumHandle
{
friend class TSonorkBfFile;
private:
    DWORD 				parent_entry_no;
    DWORD				cur_entry_no;
    DWORD				enum_flags;
	TSonorkBfDirEntry		entry;
	TSonorkBfEnumHandle(DWORD parent_entry_no,DWORD flags);
public:
	TSonorkBfEnumHandle(TSonorkBfBlock*,DWORD flags);
	DWORD				EntryNo()	const { return cur_entry_no;}
	SONORK_BF_BLOCK_TYPE 	Type()	   	const { return (SONORK_BF_BLOCK_TYPE)entry.type;}
	TSonorkTime& 			TimeStamp()	{ return entry.time_stamp;		}
	const char*			Name()		{ return entry.name;			}
	DWORD				DataSize()	const { return entry.data_size;		}
	void 				Reset()		{ cur_entry_no=0;}
	DWORD				AtomType()	const { return entry.atom_type;}
};

class TSonorkBfFile
{
//   	TSonorkBfBlockHeader		block_header;
    struct {
		SONORK_FILE_HANDLE		data_handle;
		SONORK_FILE_HANDLE		index_handle;
        TSonorkBfFileHeader		header;
    	int 				err_code;
    }file;
    SONORK_RESULT	last_result;
    int 			ReadHeader();
    int 			WriteHeader(bool flush);
    void 			CloseFile(bool flush);

    int			   	WriteEntry	(DWORD entry_no,TSonorkBfDirEntry*);
    int			   	ReadEntry	(DWORD entry_no,TSonorkBfDirEntry*);
    int				ReadBlockHeader(DWORD block_no,TSonorkBfBlockHeader*);
    int				WriteBlockHeader(DWORD block_no,TSonorkBfBlockHeader*);
    SONORK_RESULT 	GetEntryByName(DWORD&entry_no,TSonorkBfDirEntry*);
    SONORK_RESULT 	GetEntryByNo(DWORD entry_no,TSonorkBfDirEntry*);
    SONORK_RESULT 	AllocEntry(TSonorkBfDirEntry*,DWORD&entry_no);
    SONORK_RESULT 	DeleteEntry(DWORD entry_no,TSonorkBfDirEntry*);

    DWORD		   	AllocBlock(TSonorkBfBlockHeader*);
    SONORK_RESULT 	ReadBlockData(TSonorkBfDirEntry*,TSonorkBfBlock*);
    SONORK_RESULT 	WriteBlockData(TSonorkBfDirEntry*,TSonorkBfBlock*);
    SONORK_RESULT	DeleteBlockData(DWORD block_no);
    TSonorkBfBlock* 	CreateBlock(SONORK_BF_BLOCK_TYPE,DWORD parent_entry_no,const char*name,UINT type,bool overwrite);
    TSonorkBfBlock* 	OpenBlock(SONORK_BF_BLOCK_TYPE,DWORD parent_entry_no, const char*name,bool read_data,bool allow_create, UINT type,bool*created);
    SONORK_RESULT 	DeleteBlock(SONORK_BF_BLOCK_TYPE,DWORD parent_entry_no, const char*name, DWORD depth);
public:
	TSonorkBfFile();
    ~TSonorkBfFile();
	SONORK_RESULT Open(const char *dir,const char*,bool allow_create);
	SONORK_RESULT Create(const char *dir,const char*);
	void Close(){CloseFile(true);}
	bool IsOpen(){ return file.index_handle!=SONORK_INVALID_FILE_HANDLE;}
	SONORK_RESULT Result(){ return last_result;}
	SONORK_RESULT SetResult(SONORK_RESULT r){return (last_result=r);} 


    TSonorkBfBlock*		CreateDirBlock(TSonorkBfBlock* dir_block, const char*name,bool overwrite=false)
    {
    	return CreateBlock(SONORK_BF_BLOCK_TYPE_DIR
        			,dir_block?dir_block->dir_entry_no:SONORK_BF_INVALID_ENTRY_NO
                    ,name
                    ,SONORK_ATOM_NULL
                    ,overwrite);
    }
    TSonorkBfBlock*		CreateDataBlock(
    		TSonorkBfBlock* 		dir_block
        , 	const char*			name
        ,	UINT				atom_type
        ,	bool 				overwrite=false)
    {
    	return CreateBlock(SONORK_BF_BLOCK_TYPE_DATA
        			,dir_block?dir_block->dir_entry_no:SONORK_BF_INVALID_ENTRY_NO
                    ,name
                    ,atom_type
                    ,overwrite);
    }
    TSonorkBfBlock*		OpenDirBlock(TSonorkBfBlock* dir_block, const char*name, bool allow_create, bool *created=NULL)
    {
    	return OpenBlock(SONORK_BF_BLOCK_TYPE_DIR
        			,dir_block?dir_block->dir_entry_no:SONORK_BF_INVALID_ENTRY_NO
                    ,name
                    ,false
                    ,allow_create
                    ,SONORK_ATOM_NULL
                    ,created);
    }
    TSonorkBfBlock*		ReadDataBlock(TSonorkBfBlock* dir_block, const char*name)
    {
    	return OpenBlock(SONORK_BF_BLOCK_TYPE_DATA
        			,dir_block?dir_block->dir_entry_no:SONORK_BF_INVALID_ENTRY_NO
                    ,name
                    ,true
                    ,false
                    ,SONORK_ATOM_NULL
                    ,NULL);
    }
	SONORK_RESULT
		WriteDataBlock(TSonorkBfBlock*,UINT atom_type);

	SONORK_RESULT
		DeleteBlock(SONORK_BF_BLOCK_TYPE type,TSonorkBfBlock* dir_block, const char*name)
	{
		return DeleteBlock(type
						,dir_block?dir_block->dir_entry_no:SONORK_BF_INVALID_ENTRY_NO
						,name, 0);
	}

	SONORK_RESULT
		DeleteDataBlock(TSonorkBfBlock* dir_block, const char*name)
	{
		return DeleteBlock(SONORK_BF_BLOCK_TYPE_DATA,dir_block,name);
	}
	SONORK_RESULT 		DeleteDirBlock(TSonorkBfBlock* dir_block, const char*name)
	{
		return DeleteBlock(SONORK_BF_BLOCK_TYPE_DIR,dir_block,name);
	}

	SONORK_RESULT		WriteRaw(TSonorkBfBlock* dir_block, const char*name,const void*,UINT sz,SONORK_ATOM_TYPE data_type=SONORK_ATOM_RAW);
	SONORK_RESULT		ReadRaw(TSonorkBfBlock* dir_block	, const char*name,void*,UINT sz,SONORK_ATOM_TYPE  data_type=SONORK_ATOM_RAW);

	SONORK_RESULT		Read(TSonorkBfBlock* dir_block, const char*name, TSonorkShortString*);
    SONORK_RESULT		Write(TSonorkBfBlock* dir_block,const char*name, const TSonorkShortString*);

	SONORK_RESULT		Read(TSonorkBfBlock* dir_block, const char*name, TSonorkCodecAtom*);
	SONORK_RESULT		Write(TSonorkBfBlock* dir_block,const char*name, const TSonorkCodecAtom*);

    TSonorkBfEnumHandle*	OpenEnum(TSonorkBfBlock* dir_block,const char *name, DWORD flags=SONORK_BF_ENUM_DIR|SONORK_BF_ENUM_DATA);
    SONORK_RESULT			EnumNext(TSonorkBfEnumHandle*);
	void				Flush();
};
#endif