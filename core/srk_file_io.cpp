#include "srk_defs.h"
#pragma hdrstop
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


// ----------------------------------------------------------------------------
// GU I/O functions
// ----------------------------------------------------------------------------


SONORK_C_CSTR   SONORK_IO_GetFileName(SONORK_C_CSTR path)
{
	SONORK_C_CSTR name;
	name=path;
	while(*path)
	{
		if( *path == '/' || *path=='\\')
			name=path+1;
		path++;
	}
	return name;
}
SONORK_C_CSTR   SONORK_IO_GetFileExt(SONORK_C_CSTR path)
{
	SONORK_C_CSTR extension;

	for(extension=NULL;*path;path++)
		if(*path=='.')
			extension=path;

	return !extension?path:extension;
}
bool            SONORK_IO_IsRelativePath(SONORK_C_CSTR path)
{
  const char *dd_ptr;
  const char *s_ptr;
  s_ptr=path;
  while((dd_ptr=strstr(s_ptr,".."))!=NULL)
  {
	s_ptr=dd_ptr+2;
	if(!*s_ptr||*s_ptr==SONORK_IO_DIR_SLASH)
		return true;
  }
  return false;
}
int SONORK_IO_CloseFile(SONORK_FILE_HANDLE H)
{

#if defined(SONORK_WIN32_BUILD)
	return CloseHandle(H);
#elif defined(SONORK_LINUX_BUILD)
	if(!fclose(H))
		return 1;	//ok
	else
		return 0;	//error
#else
#error NO IMPLEMENTATION
#endif
}
SONORK_FILE_HANDLE
	 SONORK_IO_OpenFile_ReadOnly(
		 SONORK_C_CSTR 				path
		,int*					p_err_code)
{
	SONORK_FILE_HANDLE 			H;
#if defined(SONORK_WIN32_BUILD)
	H = CreateFile(path
			,GENERIC_READ
			,FILE_SHARE_READ
			,NULL
			,OPEN_EXISTING
			,FILE_ATTRIBUTE_NORMAL
			,NULL);
	if(H==SONORK_INVALID_FILE_HANDLE)
		if(p_err_code)*p_err_code=GetLastError();
#elif defined(SONORK_LINUX_BUILD)
	H = fopen(path, "rb");
	if(H==SONORK_INVALID_FILE_HANDLE)
		if(p_err_code)
			*p_err_code = errno;
#else
#error NO IMPLEMENTATION
#endif
	return H;
}
SONORK_FILE_HANDLE
	 SONORK_IO_OpenFile(const char*		path
		,SONORK_OPEN_FILE_METHOD	method
		,int*				p_err_code
		,SONORK_OPEN_FILE_RESULT*   	p_result)
{
	SONORK_FILE_HANDLE 	H;
	SONORK_OPEN_FILE_RESULT	result;
	int 			err_code;
#if defined(SONORK_WIN32_BUILD)
	H=CreateFile(path
		,GENERIC_READ|GENERIC_WRITE
		,FILE_SHARE_READ
		,NULL
		,method
		,FILE_ATTRIBUTE_NORMAL
		,INVALID_HANDLE_VALUE);
	if(H==INVALID_HANDLE_VALUE)
	{
		err_code=GetLastError();
		result=SONORK_OPEN_FILE_RESULT_FAILED;
	}
	else
	{
		err_code=0;

		if(method==SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS)
			result=SONORK_OPEN_FILE_RESULT_NEW;
		else
		if(GetLastError()==ERROR_ALREADY_EXISTS||method==SONORK_OPEN_FILE_METHOD_OPEN_EXISTING)
			result=SONORK_OPEN_FILE_RESULT_EXISTING;
		else
			result=SONORK_OPEN_FILE_RESULT_NEW;

	}
	if(p_err_code)
		*p_err_code=err_code;
	if(p_result)
		*p_result=result;
	return H;
#elif defined(SONORK_LINUX_BUILD)

	char	existe;	// bandera par ver si existe el archivo
	int 	access_fd;

	// inocente hasta que se demuestre lo contrario
	existe = 0;
	// access(2) arma quilombo cuando el programa esta con suid
	access_fd = open(path, O_RDONLY);
	if(access_fd >= 0)	{ // si existe
		close(access_fd);
		existe = 1;
	}


	switch(method) {
		// setear H y result dentro de este switch
		// se llama solo una vez a fopen() dentro de este switch
		case SONORK_OPEN_FILE_METHOD_CREATE_NEW:
			if(existe) {
				result = SONORK_OPEN_FILE_RESULT_FAILED;
				H = SONORK_INVALID_FILE_HANDLE;
			}
			else {
				H = fopen(path, "w+b");
				if(H == SONORK_INVALID_FILE_HANDLE)
					result = SONORK_OPEN_FILE_RESULT_FAILED;
				else
					result = SONORK_OPEN_FILE_RESULT_NEW;
			}
		break;
		case SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS:
			H = fopen(path, "w+b");
			if(H == SONORK_INVALID_FILE_HANDLE)
				result = SONORK_OPEN_FILE_RESULT_FAILED;				
			else {
				if(existe)
					result = SONORK_OPEN_FILE_RESULT_EXISTING;
				else
					result = SONORK_OPEN_FILE_RESULT_NEW;
			}
		break;
		case SONORK_OPEN_FILE_METHOD_OPEN_EXISTING:
			if(existe) {
				H = fopen(path, "r+b");
				if(H == SONORK_INVALID_FILE_HANDLE)
					result = SONORK_OPEN_FILE_RESULT_FAILED;				
				else
					result = SONORK_OPEN_FILE_RESULT_EXISTING;
			}
			else {
				result = SONORK_OPEN_FILE_RESULT_FAILED;
				H = SONORK_INVALID_FILE_HANDLE;
			}
		break;
		case SONORK_OPEN_FILE_METHOD_OPEN_ALWAYS:
			if(existe) {	// abrir sin truncar
				H = fopen(path, "r+b");
				if(H == SONORK_INVALID_FILE_HANDLE)
					result = SONORK_OPEN_FILE_RESULT_FAILED;				
				else
					result = SONORK_OPEN_FILE_RESULT_EXISTING;
			}
			else {	//crear un archivo nuevo
				H = fopen(path, "w+b");
				if(H == SONORK_INVALID_FILE_HANDLE)
					result = SONORK_OPEN_FILE_RESULT_FAILED;				
				else
					result = SONORK_OPEN_FILE_RESULT_NEW;
			}
		break;
		default:
			result = SONORK_OPEN_FILE_RESULT_FAILED;
			H = SONORK_INVALID_FILE_HANDLE;
		break;
	}
	err_code = errno;

	if(p_err_code)
		*p_err_code = err_code;
	if(p_result)
		*p_result = result;
	return H;
#else
#	error NO IMPLEMENTATION
#endif
}
int	SONORK_IO_Flush(SONORK_FILE_HANDLE handle)
{
#if defined(SONORK_WIN32_BUILD)
	if( FlushFileBuffers( handle ) )
		return 0;
	return GetLastError();
#elif defined(SONORK_LINUX_BUILD)
	return fflush(handle);
#else
#	error NO IMPLEMENTATION
#endif
}

int SONORK_IO_GetFileSize(SONORK_FILE_HANDLE handle,DWORD*file_size)
{
#if defined(SONORK_WIN32_BUILD)
	*file_size=GetFileSize(handle,NULL);
	if(*file_size==0xFFFFFFFF)
		return GetLastError();
	return 0;
#elif defined(SONORK_LINUX_BUILD)

	// handle debe ser valido. No se revisa por SONORK_INVALID_FILE_HANDLE
	int fd;
	struct stat fs;

	fd = fileno(handle);
	if(fstat(fd, &fs) < 0)
	{
		int er=errno;
		return er;
	}
	*file_size = fs.st_size;
	return 0;
#else
#	error NO IMPLEMENTATION
#endif
}
int  SONORK_IO_SetFilePointer(SONORK_FILE_HANDLE handle,long pos,SONORK_FILE_SEEK_MODE seek_mode)
{
#if defined(SONORK_WIN32_BUILD)
	int rv;
	if(SetFilePointer(handle,(LONG)pos,NULL,seek_mode)==(DWORD)-1)
	{
		if((rv=GetLastError())==0)
			rv=-1;
		return rv;
	}
	return 0;
#elif defined(SONORK_LINUX_BUILD)

	int res;
	res = fseek(handle, pos, seek_mode);
	if(res < 0)
	{
//		printf("SONORK_IO_SetFilePointer(%u)= ERROR %u \n",pos, res);
		return -1;	//error
	}
	else
	{
//		printf("SONORK_IO_SetFilePointer(%u)= OK \n",pos);
		return 0;	//ok
	}
#else
#	error NO IMPLEMENTATION
#endif
}
int SONORK_IO_WriteFile(SONORK_FILE_HANDLE handle,const void*ptr,UINT size)
{
#if defined(SONORK_WIN32_BUILD)
	DWORD r_size;
	int rv;
	if( WriteFile(handle,ptr,size,&r_size,NULL) != 0 )
		if(r_size==size)
			return 0;
	if((rv=GetLastError())==0)
		rv=-1;
	return rv;
#elif defined(SONORK_LINUX_BUILD)

	DWORD w_size;
	w_size = fwrite(ptr, 1, size,  handle);	//escribir de a 1
	if(w_size != size)
		return -1;
	else
		return 0;
#else
#	error NO IMPLEMENTATION
#endif
}
int SONORK_IO_ReadFile(SONORK_FILE_HANDLE handle,void*ptr,UINT size)
{
#if defined(SONORK_WIN32_BUILD)
	DWORD r_size;
    int rv;
    if(ReadFile(handle,ptr,size,&r_size,NULL) != 0)
		if(r_size==size)
			return 0;
	if((rv=GetLastError())==0)
		rv=-1;
	return rv;
#elif defined(SONORK_LINUX_BUILD)

	DWORD r_size;
	r_size = fread(ptr, 1, size, handle);
	if(r_size != size)
		return -1;
	else
		return 0;
#else
#	error NO IMPLEMENTATION
#endif
}
TSonorkEnumDirHandle*
	SONORK_IO_EnumOpenDir(SONORK_C_CSTR path,SONORK_RESULT*p_result,SONORK_C_CSTR mask)
{

//Migs
  TSonorkEnumDirHandle*DH;
  SONORK_MEM_NEW(DH=new TSonorkEnumDirHandle);

#if defined(SONORK_WIN32_BUILD)
	TSonorkTempBuffer ff_path(SONORK_MAX_PATH*2);
	sprintf(ff_path.CStr(),"%s%c%s",path,SONORK_IO_DIR_SLASH,mask);
	DH->handle=FindFirstFile(ff_path.CStr(),&DH->find_data);

	if(DH->handle!=INVALID_HANDLE_VALUE)
	{
		DH->name=DH->find_data.cFileName;
		DH->ParseData();
		return DH;
	}
	if(p_result)*p_result=SONORK_RESULT_STORAGE_ERROR;
#elif defined(SONORK_LINUX_BUILD)

	SONORK_RESULT result;
	struct dirent *dir_entry;

	// aqui alloco memoria para DH->DIR
	DH->handle = opendir(path);
	if(DH->handle == SONORK_INVALID_DIR_HANDLE) {
		switch(errno) {
			case ENOENT: // no exite o path == NULL
				result=SONORK_RESULT_NO_DATA;
			break;
			case EACCES: // permisos
				result=SONORK_RESULT_ACCESS_DENIED;
			break;
			default: // otros
				result=SONORK_RESULT_STORAGE_ERROR;
			break;
		}
		if(p_result)*p_result=result;
	}
	else
	{
		// cargar el primer direntry
		dir_entry = readdir(DH->handle);
		if(dir_entry != SONORK_INVALID_DIR_HANDLE)
		{
			// completar DH->name
			SONORK_StrCopy(DH->name, sizeof(DH->name), dir_entry->d_name);
			// completar flags
		   	DH->ParseData();
		    return DH;
		}
		// la primera entrada del directorio se tiene que poder leer si o si
		result=SONORK_RESULT_STORAGE_ERROR;
		if(p_result)*p_result=result;
	};
#else
#	error NO IMPLEMENTATION
#endif
	// salir con error
//Migs
	SONORK_MEM_DELETE(DH);
	//return NULL;
    return SONORK_INVALID_DIR_HANDLE;
}
SONORK_RESULT  SONORK_IO_EnumNextDir(TSonorkEnumDirHandle*DH)
{
#if defined(SONORK_WIN32_BUILD)
	if(FindNextFile(DH->handle,&DH->find_data))
    {
    	DH->ParseData();
        return SONORK_RESULT_OK;
	}
    return SONORK_RESULT_NO_DATA;
#elif defined(SONORK_LINUX_BUILD)
	struct dirent *dir_entry;

	dir_entry = readdir(DH->handle);
	if(dir_entry == SONORK_INVALID_DIR_HANDLE) 
		return SONORK_RESULT_NO_DATA;

	// completar DH->name
	SONORK_StrCopy(DH->name, sizeof(DH->name), dir_entry->d_name);
	// completar flags
   	DH->ParseData();
	return SONORK_RESULT_OK;
#else
#	error NO IMPLEMENTATION
#endif
}
SONORK_RESULT  	SONORK_IO_EnumCloseDir(TSonorkEnumDirHandle*DH)
{
#if defined(SONORK_WIN32_BUILD)
	FindClose(DH->handle);
	SONORK_MEM_DELETE(DH);
	return SONORK_RESULT_OK;
#elif defined(SONORK_LINUX_BUILD)
	int res;
	res = closedir(DH->handle);
	if(!res)
		return SONORK_RESULT_OK;
	else
		return SONORK_RESULT_STORAGE_ERROR;
#else
#	error NO IMPLEMENTATION
#endif
}
SONORK_RESULT
	SONORK_IO_MakePath(SONORK_C_STR target,SONORK_C_CSTR p_source,SONORK_C_CSTR p_suffix)
{
//	int 		l;
	SONORK_RESULT 	result;
	SONORK_C_CSTR	source;
	SONORK_C_CSTR	suffix;
	source = (p_source==NULL)?"":p_source;
	suffix = (p_suffix==NULL)?"":p_suffix;
	if(strlen(source)+strlen(suffix)>=SONORK_MAX_PATH-2)
	{
		*target=0;
		result=SONORK_RESULT_INVALID_PARAMETER;
	}
	else
	{
		result=SONORK_RESULT_OK;
		if(*source)
		{
			if(*suffix)
			{
				sprintf(target,"%s%c%s",source,SONORK_IO_DIR_SLASH,suffix);
			}
			else
			{
				strcpy(target,source);
			}
		}
		else
		{
			if(*suffix)
			{
				strcpy(target,suffix);
			}
			else
				*target=0;
		}
	}
	return result;

}




#if defined(SONORK_WIN32_BUILD)
void	TSonorkEnumDirHandle::ParseData()
{
	size=find_data.nFileSizeLow;
	if(find_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		flags=SONORK_FILE_ATTR_DIRECTORY;
	else
		flags=0;
	if(find_data.dwFileAttributes&FILE_ATTRIBUTE_READONLY)
		flags|=SONORK_FILE_ATTR_READ_ONLY;
}
#elif defined(SONORK_LINUX_BUILD)
/*
 *	completar:
 *		DH->flags
 *		DH->size
 */
void	TSonorkEnumDirHandle::ParseData()
{
	struct stat file_stat;

	flags = 0;

	// completar flags
	if(stat(name, &file_stat) == 0) {
		if(S_ISDIR(file_stat.st_mode)) // revisar por dir
			flags = flags | SONORK_FILE_ATTR_DIRECTORY;
		else {
			// completar flags del archivo
			if(!access(name, W_OK)) // revisar readonly
				flags = flags | SONORK_FILE_ATTR_READ_ONLY;
		}
	}
}
#endif

