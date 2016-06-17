#include <windows.h>
#pragma hdrstop
#include <stdio.h>
#include <assert.h>
#include "srk_zip.h"
static const char *file_name_alone(const char *file_path);

bool
 TSonorkZip::InitInflateFile( TSonorkZipHandle	ZH
			, const char *extract_path
			, const void *zip_data
			, bool  zip_data_is_name)
{
	unz_file_info zi;
	union {
		const void *	data;
		DWORD		number;
		const char * 	name;
	}zd;
	const char	*zip_name;

	if(ref_count==0 || ZH==NULL)
		return false;

	if( ZH->op != SONORK_ZIP_OP_INFLATE )
		return false;

	ZH->CloseDataFile();
	zd.data = zip_data;

	if( zip_data_is_name )
	{
		if( func.p_unzLocateFile(ZH->z_file.deflate
					     ,zd.name
					     ,2)!=UNZ_OK)
			return false;
	}
	else
	{
		if( func.p_unzGoToFirstFile(ZH->z_file.deflate) != UNZ_OK )
			return false;
		while( zd.number-- )
			if( func.p_unzGoToNextFile(ZH->z_file.deflate) != UNZ_OK )
				return false;
	}

	if( func.p_unzOpenCurrentFile(ZH->z_file.deflate) != UNZ_OK)
		return false;

	ZH->z_open=true;

	if( func.p_unzGetCurrentFileInfo(ZH->z_file.deflate
		, &zi
		, (char*)ZH->buffer
		, ZH->buffer_size
		, NULL  , 0
		, NULL  , 0) != UNZ_OK)
		return false;

	zip_name = file_name_alone( (const char*)ZH->buffer );
	if( *zip_name )
	{
		ZH->data_file = CreateFile(
				 extract_path
				,GENERIC_WRITE
				,0
				,NULL
				,CREATE_ALWAYS
				,FILE_ATTRIBUTE_NORMAL
				,NULL);
		if( ZH->data_file == INVALID_HANDLE_VALUE )
			return false;
		ZH->op	= SONORK_ZIP_OP_INFLATE_DATA;
	}
	else
	{
		ZH->op	= SONORK_ZIP_OP_INFLATE_FOLDER;
	}
	ZH->z_size 	= zi.compressed_size;
	ZH->data_size   = zi.uncompressed_size;
	ZH->data_offset	= 0;

	return true;
}
bool
 TSonorkZip::GetFileInfoByNumber( TSonorkZipHandle ZH
			, DWORD 	zip_number
			, char *	name
			, DWORD*	real_size
			, DWORD*	compressed_size
			)
{
	unz_file_info zi;
	if(ref_count==0 || ZH==NULL)
		return false;
	if( func.p_unzGoToFirstFile(ZH->z_file.deflate) != UNZ_OK )
			return false;
	while( zip_number-- )
		if( func.p_unzGoToNextFile(ZH->z_file.deflate) != UNZ_OK )
			return false;
	if( func.p_unzGetCurrentFileInfo(ZH->z_file.deflate
		, &zi
		, (char*)ZH->buffer
		, ZH->buffer_size
		, NULL  , 0
		, NULL  , 0) != UNZ_OK)
		return false;
	if(name)
		strcpy(name , file_name_alone( (const char*)ZH->buffer ) );
	if(real_size)
		*real_size=zi.uncompressed_size;
	if(compressed_size)
		*compressed_size=zi.compressed_size;

	return true;
}

bool
 TSonorkZip::InitDeflateFile( TSonorkZipHandle ZH
	, const char *file_path
	, int compress_level
	, const char *zip_file_name)
{
	zip_fileinfo 	zi;
	FILETIME	local_file_time;
	SYSTEMTIME	sys_time;
	BY_HANDLE_FILE_INFORMATION	fi;

	if(ref_count==0 || ZH==NULL)
		return false;

	if( ZH->op != SONORK_ZIP_OP_DEFLATE )
		return false;

	ZH->CloseDataFile();

	ZH->data_file = CreateFile(
			 file_path
			,GENERIC_READ
			,FILE_SHARE_READ|FILE_SHARE_WRITE
			,NULL
			,OPEN_EXISTING
			,FILE_ATTRIBUTE_NORMAL
			,NULL);
	if( ZH->data_file == INVALID_HANDLE_VALUE )
		return false;

	if( !GetFileInformationByHandle(ZH->data_file,&fi ) )
		return false;

	zi.dosDate = 0;
	zi.internal_fa = 0;
	zi.external_fa = 0;

	FileTimeToLocalFileTime(&fi.ftLastWriteTime,&local_file_time);
	FileTimeToSystemTime( &local_file_time , &sys_time);
	zi.tmz_date.tm_sec = sys_time.wSecond;
	zi.tmz_date.tm_min = sys_time.wMinute;
	zi.tmz_date.tm_hour= sys_time.wHour;
	zi.tmz_date.tm_mday= sys_time.wDay;
	zi.tmz_date.tm_mon = sys_time.wMonth;
	zi.tmz_date.tm_year= sys_time.wYear;

	if(zip_file_name == NULL )
		zip_file_name = file_name_alone( file_path );

	return AddEntryToZip( ZH
		, zip_file_name
		, zi
		, fi.nFileSizeLow
		, compress_level );
}
bool
 TSonorkZip::DoInflateFile( TSonorkZipHandle ZH)
{
	DWORD	io_size,aux;

	if(ref_count==0 || ZH==NULL)
		return false;
	if( ZH->op != SONORK_ZIP_OP_INFLATE_DATA
	 || ZH->data_file == INVALID_HANDLE_VALUE )
		return false;
	io_size = ZH->data_size - ZH->data_offset;
	if( io_size )
	{
		if( io_size > ZH->buffer_size )
			io_size=ZH->buffer_size;
			
		aux = (DWORD)func.p_unzReadCurrentFile(ZH->z_file.inflate,ZH->buffer,io_size);

		if( aux != io_size )
			return false;

		if(!WriteFile(ZH->data_file , ZH->buffer , io_size , &aux , NULL ))
			return false;

		ZH->data_offset+=io_size;
	}
	return true;
}

bool
 TSonorkZip::DoDeflateFile(  TSonorkZipHandle ZH )
{
	DWORD	io_size,aux;

	if(ref_count==0 || ZH==NULL)
		return false;

	if(  ZH->op != SONORK_ZIP_OP_DEFLATE_DATA
	  || ZH->data_file == INVALID_HANDLE_VALUE )
		return false;

	io_size = ZH->data_size - ZH->data_offset;
	if( io_size )
	{
		if( io_size > ZH->buffer_size )
			io_size=ZH->buffer_size;
		if(!ReadFile(ZH->data_file , ZH->buffer , io_size , &aux , NULL ))
			return false;
		if(func.p_zipWriteInFileInZip(ZH->z_file.deflate,ZH->buffer,io_size)!=ZIP_OK)
			return false;
		ZH->data_offset+=io_size;
	}
	return true;
}

bool
 TSonorkZip::AddEntryToZip(TSonorkZipHandle	ZH
			, const char *zip_file_name
			, const zip_fileinfo& zip_file_info
			, DWORD data_size
			, int compress_level)
{
	if(ref_count==0 || ZH==NULL)
		return false;
	if( ZH->op != SONORK_ZIP_OP_DEFLATE )
		return false;
	ZH->data_offset	=
	ZH->data_size	=
	ZH->z_size	= 0;
	if(func.p_zipOpenNewFileInZip(ZH->z_file.deflate
			 ,zip_file_name
			 ,&zip_file_info
			 ,NULL
			 ,0
			 ,NULL
			 ,0
			 ,NULL /* comment*/
			 ,(compress_level != 0) ? Z_DEFLATED : 0
			 , compress_level)==ZIP_OK)
	{
		ZH->op 		= SONORK_ZIP_OP_DEFLATE_DATA;
		ZH->data_size	= data_size;
		ZH->z_open	= true;
		return true;
	}
	return false;
}



TSonorkZipHandle
 TSonorkZip::OpenForDeflate(const char *name,bool append, DWORD buffer_size)
{
	gzFile		z_file;
	if(!IncRef())
	{
		return NULL;
	}
	if((z_file = func.p_zipOpen(name,append))==NULL)
	{
		DecRef();
		return NULL;
	}
	return new TSonorkZipStream(z_file,SONORK_ZIP_OP_DEFLATE,buffer_size,0);
}
TSonorkZipHandle
 TSonorkZip::OpenForInflate(const char *name,DWORD buffer_size)
{
	unzFile			z_file;
	unz_global_info		gi;
	if(!IncRef())
		return NULL;
	if((z_file = func.p_unzOpen(name))==NULL)
	{
		DecRef();
		return NULL;
	}
	if( func.p_unzGetGlobalInfo(z_file,&gi)!=UNZ_OK)
	{
		DecRef();
		return NULL;
	}
	return new TSonorkZipStream(z_file,SONORK_ZIP_OP_INFLATE,buffer_size,gi.number_entry);
}
int
 TSonorkZip::Close( TSonorkZipHandle ZH )
 {
	int rv;
	if(ZH==NULL || ref_count == 0)
	{
		rv = -1;
	}
	else
	{
		if( ZH->op >= SONORK_ZIP_OP_DEFLATE_FIRST
		&&  ZH->op <= SONORK_ZIP_OP_DEFLATE_LAST)
		{
			EndDeflate(ZH);
			rv = func.p_zipClose( ZH->z_file.deflate ,"");
		}
		else
		{
			EndInflate(ZH);
			rv = func.p_unzClose( ZH->z_file.deflate );
		}
		delete ZH;
		DecRef();
	}
	return rv;
}

bool
 TSonorkZip::EndDeflate( TSonorkZipHandle ZH )
{
	if(ref_count==0 || ZH==NULL)
		return false;

	if( !(ZH->op >= SONORK_ZIP_OP_DEFLATE_FIRST && ZH->op <= SONORK_ZIP_OP_DEFLATE_LAST) )
		return false;

	ZH->op = SONORK_ZIP_OP_DEFLATE;
	ZH->CloseDataFile();

	if(ZH->z_open)
	{
		ZH->z_open=false;
		if( ZH->OperationComplete() )
			ZH->z_count++;
		if(func.p_zipCloseFileInZip(ZH->z_file.deflate)!=ZIP_OK)
			return false;
	}
	return true;
}

bool
 TSonorkZip::EndInflate( TSonorkZipHandle ZH )
{
	if(ref_count==0 || ZH==NULL)
		return false;

	if( !(ZH->op >= SONORK_ZIP_OP_INFLATE_FIRST && ZH->op <= SONORK_ZIP_OP_INFLATE_LAST) )
		return false;

	ZH->op = SONORK_ZIP_OP_INFLATE;
	ZH->CloseDataFile();
	if( ZH->z_open )
	{
		ZH->z_open = false;
		if(func.p_unzCloseCurrentFile(ZH->z_file.inflate)!=ZIP_OK)
			return false;
	}
	return true;
}


TSonorkZip::TSonorkZip(const char *pPath)
{
	int l=strlen(pPath)+1;
	ref_count=0;
	hInstance=NULL;
	path = new char[l];
	memcpy(path,pPath,l);
}
TSonorkZip::~TSonorkZip()
{
	Unload();
	delete[] path;
}
bool
 TSonorkZip::IncRef()
{
	if( ref_count == 0)
		if(!Load())
			return false;
	ref_count++;
	return true;
}
void
 TSonorkZip::DecRef()
{
	assert(ref_count>0);
	if(--ref_count==0)
		Unload();
}

bool
 TSonorkZip::Load()
{
#define GZ_MAP(name)\
	if((func.p_##name=(t_##name*)GetProcAddress(hInstance,#name))==NULL)\
		break
	assert( hInstance == NULL );
	for(;;)
	{
		if( ( hInstance = LoadLibrary(path) ) == NULL )
			break;
		GZ_MAP( zipOpen );
		GZ_MAP( zipClose );
		GZ_MAP( zipOpenNewFileInZip );
		GZ_MAP( zipWriteInFileInZip );
		GZ_MAP( zipCloseFileInZip );
		GZ_MAP( unzOpen );
		GZ_MAP( unzGetGlobalInfo );
		GZ_MAP( unzGoToFirstFile );
		GZ_MAP( unzGoToNextFile );
		GZ_MAP( unzLocateFile );
		GZ_MAP( unzOpenCurrentFile );
		GZ_MAP( unzGetCurrentFileInfo );
		GZ_MAP( unzReadCurrentFile );
		GZ_MAP( unzCloseCurrentFile );
		GZ_MAP( unzClose );
		return true;
	}
	Unload();
	return false;
#undef GZ_MAP
}
void
 TSonorkZip::Unload()
{
	if(hInstance!=NULL)
	{
		FreeLibrary(hInstance);
		hInstance=NULL;
	}
}
void TSonorkZipStream::CloseDataFile()
{
	if( data_file != INVALID_HANDLE_VALUE )
	{
		CloseHandle( data_file );
		data_file=INVALID_HANDLE_VALUE;
	}
}
TSonorkZipStream::TSonorkZipStream(void *ptr
	,SONORK_ZIP_OPERATION p_op
	, DWORD p_size
	, DWORD p_count)
{
	if( p_size < 256 )p_size=256;
	z_file.ptr	= ptr;
	z_count		= p_count;
	z_open		= false;
	op		= p_op;
	buffer_size 	= p_size;
	buffer 		= new BYTE[buffer_size];
	data_file 	= INVALID_HANDLE_VALUE;
}
TSonorkZipStream::~TSonorkZipStream()
{
	delete[] buffer;
	CloseDataFile();
}

static const char *file_name_alone(const char *file_path)
{
	const char *file_name,*ptr;
	ptr=file_name=file_path;
	while(*ptr!=NULL)
	{
		if(*ptr=='/'||*ptr=='\\')
		{
			file_name=++ptr;
		}
		else
			ptr++;
	}
	return file_name;
}
