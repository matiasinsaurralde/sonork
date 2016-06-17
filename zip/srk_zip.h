#if !defined(SONORK_ZIP_H)
#define SONORK_ZIP_H
#include "zip.h"
#include "unzip.h"

typedef zipFile WINAPI t_zipOpen(const char *pathname, int append);
typedef int 	WINAPI t_zipClose(zipFile file, const char* global_comment);
typedef int 	WINAPI t_zipOpenNewFileInZip(zipFile file,
					   const char* filename,
					   const zip_fileinfo* zipfi,
					   const void* extrafield_local,
					   uInt size_extrafield_local,
					   const void* extrafield_global,
					   uInt size_extrafield_global,
					   const char* comment,
					   int method,
					   int level);
typedef int 	WINAPI t_zipWriteInFileInZip(zipFile file,
					   const voidp buf,
					   unsigned len);
typedef int  	WINAPI t_zipCloseFileInZip(zipFile file);

typedef unzFile WINAPI t_unzOpen(const char *path);

typedef int 	WINAPI t_unzGetGlobalInfo(unzFile file,
						unz_global_info *pglobal_info);
typedef int 	WINAPI t_unzGoToFirstFile(unzFile file);
typedef int 	WINAPI t_unzGoToNextFile(unzFile file);
typedef int 	WINAPI t_unzLocateFile(unzFile file,
					     const char *szFileName,
					     int iCaseSensitivity);
typedef int 	WINAPI t_unzOpenCurrentFile(unzFile file);
typedef int WINAPI t_unzGetCurrentFileInfo(unzFile file,
					     unz_file_info *pfile_info,
					     char *szFileName,
					     uLong fileNameBufferSize,
					     void *extraField,
					     uLong extraFieldBufferSize,
					     char *szComment,
					     uLong commentBufferSize);
typedef int 	WINAPI t_unzReadCurrentFile(unzFile file,
						  voidp buf,
						  unsigned len);
typedef int 	WINAPI t_unzCloseCurrentFile(unzFile file);
typedef int 	WINAPI t_unzClose(unzFile file);

enum SONORK_ZIP_OPERATION
{
  SONORK_ZIP_OP_DEFLATE
, SONORK_ZIP_OP_DEFLATE_DATA
, SONORK_ZIP_OP_INFLATE
, SONORK_ZIP_OP_INFLATE_DATA
, SONORK_ZIP_OP_INFLATE_FOLDER
};
#define SONORK_ZIP_OP_DEFLATE_FIRST	SONORK_ZIP_OP_DEFLATE
#define SONORK_ZIP_OP_DEFLATE_LAST	SONORK_ZIP_OP_DEFLATE_DATA

#define SONORK_ZIP_OP_INFLATE_FIRST	SONORK_ZIP_OP_INFLATE
#define SONORK_ZIP_OP_INFLATE_LAST	SONORK_ZIP_OP_INFLATE_FOLDER

class TSonorkZipStream
{
friend class TSonorkZip;
	union {
		void		*ptr;
		gzFile		deflate;
		unzFile		inflate;
	}z_file;
	DWORD			z_size;
	DWORD			z_count;
	bool			z_open;

	HANDLE			data_file;
	DWORD			data_size;
	DWORD			data_offset;

	SONORK_ZIP_OPERATION    op;
	BYTE*			buffer;
	DWORD			buffer_size;


	void CloseDataFile();
	TSonorkZipStream(void *ptr
		, SONORK_ZIP_OPERATION op
		, DWORD buffer_size
		, DWORD z_count);
	~TSonorkZipStream();
public:
	SONORK_ZIP_OPERATION
		Operation() const
		{
			return op;
		}
	DWORD
		ZipFiles() const
		{
			return z_count;
		}
		
	BOOL
		OperationComplete() const
		{
			return data_offset >= data_size;
		}

	DWORD
		DataOffset() const
		{
			return data_offset;
		}
	DWORD
		DataSize() const
		{
			return data_size;
		}
	// InflateFileName()
	// is loaded after a call to any of the InitDeflateXXXX method
	// and is valid only until another of the zip method are invoked
	// (after which the buffer gets overwritten)
	const char*
		InflateFileName() const
		{
			return (const char*)buffer;
		}
};
typedef TSonorkZipStream* TSonorkZipHandle;
class TSonorkZip
{
	HINSTANCE	hInstance;
	struct {
		t_zipOpen*		p_zipOpen;
		t_zipClose*		p_zipClose;
		t_zipOpenNewFileInZip*	p_zipOpenNewFileInZip;
		t_zipWriteInFileInZip*	p_zipWriteInFileInZip;
		t_zipCloseFileInZip*	p_zipCloseFileInZip;
		t_unzOpen*		p_unzOpen;
		t_unzGetGlobalInfo*	p_unzGetGlobalInfo;
		t_unzGoToFirstFile*	p_unzGoToFirstFile;
		t_unzGoToNextFile*	p_unzGoToNextFile;
		t_unzLocateFile*	p_unzLocateFile;
		t_unzOpenCurrentFile*	p_unzOpenCurrentFile;
		t_unzGetCurrentFileInfo*p_unzGetCurrentFileInfo;
		t_unzReadCurrentFile*	p_unzReadCurrentFile;
		t_unzCloseCurrentFile*	p_unzCloseCurrentFile;
		t_unzClose*		p_unzClose;
	}func;
	int		ref_count;
	char		*path;
	bool	Load();
	void	Unload();

	bool
		InitInflateFile( TSonorkZipHandle
			, const char *extract_path
			, const void *zip_data
			, bool zip_data_is_name);

public:
	TSonorkZip(const char *pPath);
	~TSonorkZip();
	bool IncRef();
	void DecRef();

	TSonorkZipHandle
		OpenForDeflate(const char *name, bool append, DWORD buffer_size=4096);

	TSonorkZipHandle
		OpenForInflate(const char *name, DWORD buffer_size=4096);
		
	int
		Close(TSonorkZipHandle);

	bool
		AddEntryToZip(TSonorkZipHandle
			, const char *zip_file_name
			, const zip_fileinfo& zip_file_info
			, DWORD	data_size
			, int compress_level);
	bool
		InitDeflateFile( TSonorkZipHandle
			, const char *file_path
			, int compress_level
			, const char *zip_file_name=NULL);

	bool
		DoDeflateFile( TSonorkZipHandle );

	bool
		EndDeflate( TSonorkZipHandle ZH );

	bool
		InitInflateFileByName(
			  TSonorkZipHandle ZH
			, const char *	zip_name
			, const char *	extract_path
			)
		{
			return InitInflateFile(ZH
				,extract_path
				,(void*)zip_name
				,true);
		}

	bool
		GetFileInfoByNumber( TSonorkZipHandle ZH
			, DWORD 	zip_number
			, char *	name
			, DWORD*	real_size
			, DWORD*	compressed_size
			);
	bool
		InitInflateFileByNumber( TSonorkZipHandle ZH
			, DWORD 	zip_number
			, const char *	extract_path
			)
		{
			return InitInflateFile(ZH
				,extract_path
				,(void*)zip_number
				,false);
		}


	bool
		DoInflateFile( TSonorkZipHandle );

	bool
		EndInflate( TSonorkZipHandle );



};
#endif