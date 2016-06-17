#if !defined(SONORK_FILE_IO_H)
#define SONORK_FILE_IO_H
#include "srk_defs.h"

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




SONORK_FILE_HANDLE
	 SONORK_IO_OpenFile(
		 SONORK_C_CSTR 			path
		,SONORK_OPEN_FILE_METHOD	method
		,int*				err_code=NULL
		,SONORK_OPEN_FILE_RESULT*   	result=NULL);
SONORK_FILE_HANDLE
	 SONORK_IO_OpenFile_ReadOnly(
		 SONORK_C_CSTR 		path
		,int*			err_code=NULL);

int
	SONORK_IO_GetFileSize(SONORK_FILE_HANDLE,DWORD*size);

int
	SONORK_IO_CloseFile(SONORK_FILE_HANDLE);

int
	SONORK_IO_SetFilePointer(
		 SONORK_FILE_HANDLE	handle
		,long 			pos
		,SONORK_FILE_SEEK_MODE	seek_mode=SONORK_FILE_SEEK_BEGIN);

int
	SONORK_IO_WriteFile(SONORK_FILE_HANDLE,const void*,UINT size);

int
	SONORK_IO_ReadFile(SONORK_FILE_HANDLE,void*,UINT size);

bool
	SONORK_IO_IsRelativePath(SONORK_C_CSTR path);

int
	SONORK_IO_Flush(SONORK_FILE_HANDLE);

SONORK_RESULT
	SONORK_IO_MakePath(SONORK_C_STR target
		,SONORK_C_CSTR base_path
		,SONORK_C_CSTR ext_path);

SONORK_C_CSTR
	SONORK_IO_GetFileExt(SONORK_C_CSTR file_path); // DOES (YES) include the dot

// SONORK_IO_GetFileName
//   Returns the first character of the file name in a full path
//  	It does NOT include the bar (\\) before the file name
//      <file_path> CANNOT be NULL , and it does NOT return NULL
//		if no bar is existent in the file name, it returns <file_path>
SONORK_C_CSTR
	SONORK_IO_GetFileName(SONORK_C_CSTR file_path);
// SONORK_IO_EnumOpenDir
// CREATES a SONORK_ENUM_DIR_HANDLE structure and initializes it with whatever
// the underling OS needs and loads the appropriate fields(size,flags,name)
// with the first file in the "path" directory.
// On error returns NULL (SONORK_INVALID_DIR_HANDLE)
// and if "result" is provided (not null) loads the error code into it.
struct  TSonorkEnumDirHandle
{
	 UINT					flags;
	 UINT					size;
#if defined(SONORK_WIN32_BUILD)
	 char *					name;
	 HANDLE					handle;
	 WIN32_FIND_DATA		find_data;
	 void					ParseData();
#elif defined(SONORK_LINUX_BUILD)
	 char  					name[SONORK_MAX_PATH];
	 DIR* 					handle;
	 void					ParseData();
#endif
};

TSonorkEnumDirHandle*	 SONORK_IO_EnumOpenDir(SONORK_C_CSTR subdir,SONORK_RESULT*,SONORK_C_CSTR mask);

// SONORK_IO_EnumNextDir
// Loads into the hande the next file in the directory.
// On success, returns SONORK_RESULT_OK if file load
// Returns SONORK_RESULT_NO_DATA if no more files found.
// returns SONORK_RESULT_STORAGE_ERROR on failure
SONORK_RESULT  	 SONORK_IO_EnumNextDir(TSonorkEnumDirHandle*);

// SONORK_IO_EnumCloseDir
// Cleans up anything open during SONORK_IO_EnumOpenDir
// (including the SONORK_ENUM_DIR_HANDLE structure)
SONORK_RESULT  	SONORK_IO_EnumCloseDir(TSonorkEnumDirHandle*);


#endif