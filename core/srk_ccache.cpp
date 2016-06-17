#include "srk_defs.h"
#pragma hdrstop
#include "srk_ccache.h"
#include "srk_file_io.h"
#define SONORK_CCACHE_VERSION	1

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


// ---------------------------------------------------------------------------
// TSonorkCCacheLinkContext

BOOL
 TSonorkCCacheLinkContext::Contains(DWORD p_tracking_no)
{
	DWORD i;
	if(p_tracking_no!=0)
	{
		for(i=0;i<count;i++)
			if(tracking_no[i] == p_tracking_no )
				return true;
	}
	return false;

}
void
 TSonorkCCacheLinkContext::Add( DWORD p_tracking_no )
{
	if(count<CCACHE_MAX_LINK_DEPTH && p_tracking_no!=0)
	{
		if( !Contains(p_tracking_no) )
			tracking_no[count++]=p_tracking_no;
	}
}
void
 TSonorkCCacheLinkContext::Clear()
{
	count=0;
	top_line_no=SONORK_INVALID_INDEX;
	search_next=false;
}
TSonorkCCacheEntry*
 TSonorkCCache::GetNext(  DWORD&	line_no
			, DWORD		set_flags
			, DWORD		flag_mask
			)
{
	DWORD end_line;
	TSonorkCCacheEntry* CL;
	end_line = Lines();
	if( end_line > 0)
	{
		if( line_no==SONORK_INVALID_INDEX || line_no >= end_line )
		{
			line_no = end_line - 1;
			return NULL;
		}
		for( ; line_no < end_line ; line_no++)
		{
			CL = Get( line_no , NULL , NULL , SONORK_CCACHE_SD_RANDOM );
			if(CL)
			{
				if( (CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & flag_mask) == set_flags )
				{
					return CL;
				}
			}
		}
	}
	else
		line_no=SONORK_INVALID_INDEX;
	return NULL;
}

TSonorkCCacheEntry*
 TSonorkCCache::GetLinked( DWORD			start_line_no
			, const TSonorkCCacheEntry* 	origCL
			, DWORD&			found_line_no
			, SONORK_C_CSTR*		str_ptr
			, SONORK_CCACHE_GET_LINKED_METHOD method
			, DWORD max_scan_lines )
{
	TSonorkCCacheEntry*    	pEntry;
	UINT		lines_scanned;
	DWORD		scan_line_no;
	DWORD		orig_tag;
	SONORK_DWORD2	orig_tracking_no;
	SONORK_DWORD2	tno;
	bool		searched_for_top;

	if( start_line_no == SONORK_INVALID_INDEX)
		start_line_no = 0;

	orig_tracking_no.Set(origCL->tracking_no);
	orig_tag=origCL->tag.v[SONORK_CCACHE_TAG_FLAGS];


	if( orig_tracking_no.v[1]==0
	|| (orig_tag & SONORK_CCLF_THREAD_START)
	||  start_line_no==0)
	{
		link_ctx.top_line_no=start_line_no;
		link_ctx.count=0;
		link_ctx.search_next=true;
		searched_for_top=true;
	}
	else
	{
		searched_for_top=false;
		for(;;)
		{
			if( method != SONORK_CCACHE_GET_LINKED_TOP_OF_THREAD )
			{
				if( link_ctx.Contains(orig_tracking_no.v[0]) )
					break;
				if(orig_tracking_no.v[1]!=0)
					if( link_ctx.Contains(orig_tracking_no.v[1]) )
						break;
			}
			link_ctx.Clear();
			break;
		}
	}

	found_line_no = SONORK_INVALID_INDEX;

restart_from_top:

	link_ctx.Add( orig_tracking_no.v[0] );
	link_ctx.Add( orig_tracking_no.v[1] );

	if( link_ctx.top_line_no == SONORK_INVALID_INDEX )
	{
		searched_for_top = true;
		for(scan_line_no=start_line_no-1,lines_scanned=0
		;  scan_line_no>=0 && lines_scanned < max_scan_lines
		; scan_line_no--, lines_scanned++)
		{
			pEntry = LoadEntry( scan_line_no, SONORK_CCACHE_SD_BACKWARDS);
			if( pEntry == NULL )
				return NULL;

			if(!(link_ctx.Contains(pEntry->tracking_no.v[0])
				||link_ctx.Contains(pEntry->tracking_no.v[1])) )
				continue;
			link_ctx.top_line_no = scan_line_no;
			link_ctx.Add(pEntry->tracking_no.v[0]);
			tno.Set(pEntry->tracking_no);
			if( pEntry->tracking_no.v[1] == 0)
			{
				// No related backwards item
				break;
			}
			link_ctx.Add(pEntry->tracking_no.v[1]);
			if( pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_CCLF_THREAD_START )
			{
				// Line already marked as thread start
				break;
			}
		}

		if( link_ctx.top_line_no == SONORK_INVALID_INDEX )
		{
			// Did not find any lines above this one,
			// we consider the start line as the top
			link_ctx.top_line_no = start_line_no;
			link_ctx.search_next = true;
			// found_line_no remains invalid
			// so next section searches forward
		}
		else
		{
			link_ctx.search_next = false;
		}
	}
	if( method == SONORK_CCACHE_GET_LINKED_TOP_OF_THREAD )
	{
		found_line_no = link_ctx.top_line_no;
	}
	else
	{
		if( link_ctx.search_next == false )
		{
			if( method == SONORK_CCACHE_GET_LINKED_FORWARD_ONLY)
			{
				return NULL;
			}
			for(scan_line_no=start_line_no-1,lines_scanned=0
			;  scan_line_no>=0 && lines_scanned < max_scan_lines
			; scan_line_no--, lines_scanned++)
			{
				pEntry = LoadEntry( scan_line_no , SONORK_CCACHE_SD_BACKWARDS);
				if( pEntry == NULL )
					return NULL;
				if(link_ctx.Contains(pEntry->tracking_no.v[0])
				|| link_ctx.Contains(pEntry->tracking_no.v[1]))
				{
					found_line_no = scan_line_no;
					break;
				}
			}
			if( found_line_no == SONORK_INVALID_INDEX )
			{
				link_ctx.search_next = true;
			}
		}

		if( link_ctx.search_next == true )
		{
			for( 	  scan_line_no = start_line_no+1 , lines_scanned = 0
				; scan_line_no < file.header.entries  && lines_scanned < max_scan_lines
				; scan_line_no++ , lines_scanned++)
			{
				pEntry = LoadEntry( scan_line_no , SONORK_CCACHE_SD_FORWARD);
				if( pEntry == NULL )
					return NULL;
				if(link_ctx.Contains(pEntry->tracking_no.v[0])
				|| link_ctx.Contains(pEntry->tracking_no.v[1]))
				{
					link_ctx.Add(pEntry->tracking_no.v[0]);
					link_ctx.Add(pEntry->tracking_no.v[1]);
					found_line_no = scan_line_no;
					break;
				}
			}
		}

		if( found_line_no == SONORK_INVALID_INDEX )
		{
			if( method != SONORK_CCACHE_GET_LINKED_FORWARD_ONLY)
			if( searched_for_top == false )
			{
				link_ctx.Clear();
				goto restart_from_top;
			}
			return NULL;
		}
	}
	pEntry = Get(found_line_no , str_ptr , NULL , SONORK_CCACHE_SD_RANDOM);
	if( pEntry != NULL )
	{
		if( !(pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_CCLF_IN_THREAD ) )
		{
			// Modify tag, and set CACHE DIRTY so it gets written
			pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS]|=SONORK_CCLF_IN_THREAD;
			file.header.flags|=SONORK_CCACHE_FLAG_CACHE_DIRTY;
		}
	}
	return pEntry;
}


SONORK_RESULT
 TSonorkCCache::Clear( bool clear_values )
{
	ResetHeader( clear_values );
	if( IsOpen() )
	{
		file.header.code = (DWORD)clock();
		file.err_code=FlushHeader();
		if( file.err_code != 0 )
			return SONORK_RESULT_STORAGE_ERROR;
		SONORK_IO_Flush( file.handle );
	}
	return SONORK_RESULT_OK;
}
SONORK_RESULT
 TSonorkCCache::Del(TSonorkCCacheSelection& i_sel)
{
	if( i_sel.Active() )
	{
		DWORD			start_line_no
					,	line_no
					,	del_count;
		DWORD			i_offset
					,	o_offset;


		Flush( true , true);

		cache.loaded.entries 	= 0;
		cache.loaded.offset 	= SONORK_INVALID_INDEX;
		SONORK_ZeroMem( cache.text_status
			, sizeof(SONORK_CCACHE_TEXT_STATUS) * cache.entries
			);

		// i_sel.Min() is always Contained in i_sel
		// i.e. i_sel.Contains( i_sel.Min() ) is always true
		start_line_no	= i_sel.Min();
		i_offset        = GetEntryFileOffset( start_line_no );
		o_offset	= i_offset;
		del_count	= 0;

		for( 	  line_no = start_line_no
				; line_no < file.header.entries
				; line_no++ )
		{
			if( i_sel.Contains( line_no ) )
			{
				i_sel.Del( line_no );
				del_count++;
			}
			else
			{
				// Check if the offsets are not the same
				if( o_offset != i_offset )
				{
					SONORK_IO_SetFilePointer(file.handle
						,i_offset
						,SONORK_FILE_SEEK_BEGIN);

					SONORK_IO_ReadFile(file.handle
						,cache.entry_buffer
						,sizeof(TSonorkCCacheEntry));

					SONORK_IO_SetFilePointer(file.handle
						,o_offset
						,SONORK_FILE_SEEK_BEGIN);

					SONORK_IO_WriteFile(file.handle
						,cache.entry_buffer
						,sizeof(TSonorkCCacheEntry));
				}
				o_offset += sizeof(TSonorkCCacheEntry);
			}
			i_offset += sizeof(TSonorkCCacheEntry);
		}

		// Ok, we're going to use the cache buffer
		// as temporal work buffer, flush it and
		// invalidate it.
		file.header.entries -= del_count;
		file.header.flags 	|= SONORK_CCACHE_FLAG_HEADER_DIRTY;
		file.header.flags 	&=~SONORK_CCACHE_FLAG_CACHE_DIRTY;
		FlushHeader();
		SONORK_IO_Flush(file.handle);
	}
	return SONORK_RESULT_OK;
}

void  TSonorkCCache::SetValue(SONORK_CCACHE_VALUE index,DWORD value)
{
	if( value != GetValue(index) )
	{
		file.header.flags|=SONORK_CCACHE_FLAG_HEADER_DIRTY;
		file.header.value[index]=value;
	}

}

SONORK_RESULT TSonorkCCache::Open(SONORK_C_CSTR path)
{
	if( path == NULL )
		last_result = SONORK_RESULT_INVALID_OPERATION;
	else
	{
		CloseCacheFile( true );
		last_result = OpenCacheFile( path );
	}
	return last_result;
}

void TSonorkCCache::Close()
{
	if(IsOpen())
	{
		CloseCacheFile(true);
		ResetHeader(true);
	}
}

void TSonorkCCache::CloseCacheFile(bool flush)
{
	if( file.handle != SONORK_INVALID_FILE_HANDLE )
	{
		if(flush)Flush();
		SONORK_IO_CloseFile(file.handle);
		file.handle = SONORK_INVALID_FILE_HANDLE;
	}
}


SONORK_RESULT TSonorkCCache::OpenCacheFile(const char *pPathBase )
{
	SONORK_OPEN_FILE_RESULT file_result;
	DWORD 			phys_size;
	TSonorkTempBuffer	path(SONORK_MAX_PATH);

	wsprintf(path.CStr(),"%s.gcc",pPathBase);
	file.handle=SONORK_IO_OpenFile(
				   path.CStr()
				  ,SONORK_OPEN_FILE_METHOD_OPEN_ALWAYS
				  ,&file.err_code
				  ,&file_result);

	if( file_result == SONORK_OPEN_FILE_RESULT_FAILED )
	{
		last_result=SONORK_RESULT_STORAGE_ERROR;
	}
	else
	{
		for(;;)
		{
			if(file_result==SONORK_OPEN_FILE_RESULT_NEW)
			{
				ResetHeader( true );
				file.header.code = (DWORD)clock();
				if(FlushHeader() != 0 )
					last_result = SONORK_RESULT_STORAGE_ERROR;
				else
					last_result = SONORK_RESULT_OK;
				break;
			}
			if((file.err_code=SONORK_IO_GetFileSize(file.handle,&phys_size)) != 0)
			{
				last_result = SONORK_RESULT_STORAGE_ERROR;
				break;
			}

			if( phys_size < sizeof(TSonorkCCacheHeader) )
			{
				// Reset the file
				file_result = SONORK_OPEN_FILE_RESULT_NEW;
				continue;
			}

			if((file.err_code=SONORK_IO_ReadFile(file.handle
				,&file.header
				,sizeof(file.header)))!=0)
			{
				last_result = SONORK_RESULT_STORAGE_ERROR;
				break;
			}

			if(	phys_size<ExpectedFileSize()
				||file.header.signature	!=SONORK_FILE_SIGNATURE
				||file.header.entry_size!=sizeof(TSonorkCCacheEntry)
				)
			{
				// Reset the file
				file_result = SONORK_OPEN_FILE_RESULT_NEW;
				continue;
			}

			file.header.flags&=~SONORK_CCACHE_MASK_RUN_TIME;
			last_result = SONORK_RESULT_OK;
			break;
		}
	}
	if(last_result != SONORK_RESULT_OK)
	{
		SONORK_RESULT saved_last_result;

		// CloseCacheFile modifies last_result
		saved_last_result 	= last_result;
		CloseCacheFile(false);
		last_result 		= saved_last_result;
	}
	return last_result;
}
void
	TSonorkCCache::SetBulkMode(BOOL v)
{
	if(v)
		file.header.flags|=SONORK_CCACHE_FLAG_BULK_MODE;
	else
	{
		file.header.flags&=~SONORK_CCACHE_FLAG_BULK_MODE;
		if(file.header.flags&SONORK_CCACHE_FLAG_HEADER_DIRTY)
		{
			FlushHeader();
			SONORK_IO_Flush(file.handle);
		}
	}
}

TSonorkCCacheEntry*
	TSonorkCCache::GetByMark(
			TSonorkCCacheMark*		REF
		, 	SONORK_C_CSTR* 			str_ptr
		,	DWORD 				p_max_scan_lines)
{
	if( file.header.entries > 0 )
	{
		DWORD 			line_no;
		DWORD 			scanned_lines;
		DWORD 			max_scan_lines;
		TSonorkCCacheEntry* 	pEntry;

		line_no = REF->line_no;
		if( line_no == SONORK_INVALID_INDEX)
		{
			max_scan_lines = p_max_scan_lines*2;
			line_no = file.header.entries-1;
		}
		else
		{
			max_scan_lines = p_max_scan_lines;
			if( line_no >= file.header.entries)
				line_no = file.header.entries-1;
		}

		for(	    scanned_lines = 0
			;   scanned_lines < max_scan_lines
			;   scanned_lines++)
		{
			if( (pEntry=LoadEntry(line_no , SONORK_CCACHE_SD_BACKWARDS)) == NULL )
				break;
			if( pEntry->tracking_no	== REF->tracking_no)
			if( pEntry->time	== REF->time )
			{
				REF->line_no = line_no;
				if(str_ptr != NULL )
				{
					*str_ptr = LoadText( line_no - cache.loaded.offset );
				}
				return pEntry;
			}
			if(line_no--==0)
				break;
		}
	}
	return NULL;

}

TSonorkCCacheEntry*
	TSonorkCCache::Get(DWORD 			line_no
			, SONORK_C_CSTR* 		str_ptr
			, SONORK_CCACHE_TEXT_STATUS*	pstatus
			, SONORK_CCACHE_SCAN_DIRECTION	scan_dir)
{
	TSonorkCCacheEntry	*pEntry;

	pEntry = LoadEntry( line_no , scan_dir);
	if( pEntry && str_ptr != NULL )
	{
		line_no-=cache.loaded.offset;
		*str_ptr = LoadText( line_no );
		if( pstatus )
		{
			*pstatus=*(cache.text_status + line_no);
		}
	}


	return pEntry;
}

SONORK_RESULT TSonorkCCache::LoadCache(DWORD new_offset)
{
	file.err_code=0;

	if( file.header.entry_size != sizeof(TSonorkCCacheEntry) )
		return (last_result=SONORK_RESULT_INTERNAL_ERROR);

	if( !IsOpen() )
		return (last_result=SONORK_RESULT_INVALID_OPERATION);

	if(file.header.flags&SONORK_CCACHE_FLAG_CACHE_DIRTY)
		FlushCache();

#if defined(SIMULATE_SLOW_MACHINE)
	SONORK_Sleep(SIMULATE_SLOW_MACHINE);
#endif

	if( new_offset >= file.header.entries )
	{
		last_result=SONORK_RESULT_INVALID_PARAMETER;
		return last_result;
	}

	cache.loads++;

	cache.loaded.offset		= new_offset;
	cache.loaded.entries	= cache.entries;

	if( cache.loaded.offset+cache.loaded.entries > file.header.entries)
		cache.loaded.entries = file.header.entries - cache.loaded.offset;

	SONORK_ZeroMem( cache.text_status
		, sizeof(SONORK_CCACHE_TEXT_STATUS) * cache.entries
		);

	// Set the pointer at the correct position
	if((file.err_code=
		SONORK_IO_SetFilePointer(file.handle
		,GetEntryFileOffset( cache.loaded.offset )
		,SONORK_FILE_SEEK_BEGIN))==0)
	{
		if((file.err_code=SONORK_IO_ReadFile(file.handle
			,cache.entry_buffer
			,cache.loaded.entries * sizeof(TSonorkCCacheEntry)))==0)
		{
			return (last_result = SONORK_RESULT_OK);
		}
	}
	cache.loaded.offset   	= SONORK_INVALID_INDEX;
	cache.loaded.entries 	= 0;
	return (last_result=SONORK_RESULT_STORAGE_ERROR);
}

TSonorkCCacheEntry*
 TSonorkCCache::CacheEntryPtr( UINT offset_within_cache )
{
	assert( offset_within_cache < cache.entries );
	return  cache.entry_buffer + offset_within_cache;
}

char *
 TSonorkCCache::CacheTextPtr(UINT offset_within_cache)
{
	assert( offset_within_cache < cache.entries );
	return  cache.text_buffer + (offset_within_cache * cache.text_size);
}


char*
 TSonorkCCache::LoadText( UINT offset_within_cache )
{
	TSonorkCCacheEntry	*pEntry;
	char			*pText;

	assert( offset_within_cache < cache.loaded.entries );
	pText 	= CacheTextPtr( offset_within_cache );
	if( *(cache.text_status + offset_within_cache)==SONORK_CCACHE_TEXT_STATUS_NOT_LOADED )
	{
		pEntry	= CacheEntryPtr( offset_within_cache );

		*(cache.text_status + offset_within_cache)=
			cb.ptr(cb.tag,pEntry,pText,cache.text_size)
			?SONORK_CCACHE_TEXT_STATUS_LONG
			:SONORK_CCACHE_TEXT_STATUS_SHORT;
		*(pText+cache.text_size-1)=0;
	}
	return pText;

}
TSonorkCCacheEntry*
 TSonorkCCache::LoadEntry(DWORD line_no
			, SONORK_CCACHE_SCAN_DIRECTION scan_dir)
{

	DWORD		new_offset;
	DWORD		delta;
	int			try_no;
	file.err_code=0;
	if(!IsOpen())
	{
		last_result=SONORK_RESULT_INVALID_OPERATION;
		return NULL;
	}
	if( line_no >= file.header.entries )
	{
		last_result=SONORK_RESULT_INVALID_PARAMETER;
		return NULL;
	}

	for(try_no=1;;try_no++)
	{
		if( 	line_no >= cache.loaded.offset
			&& 	line_no <  cache.loaded.offset+cache.loaded.entries)
		{
			if( try_no == 1)
				cache.hits++;
			return CacheEntryPtr( line_no  - cache.loaded.offset );
		}
		if(try_no==2)
		{
			last_result=SONORK_RESULT_INTERNAL_ERROR;
			break;
		}
		cache.misses++;

		if(scan_dir==SONORK_CCACHE_SD_RANDOM)
		{
			delta = (cache.entries>>1);
		}
		else
		if(scan_dir==SONORK_CCACHE_SD_FORWARD)
		{
			delta = (cache.entries>>3) + 1;
		}
		else
		{
			delta = cache.entries - 2;
		}
		if( line_no > delta )
			new_offset = line_no - delta;
		else
			new_offset = 0;

		if(LoadCache(new_offset)!=SONORK_RESULT_OK)
			break;
	}
	return NULL;
}

SONORK_RESULT
 TSonorkCCache::SetDatIndex(DWORD line_no,DWORD dat_index)
{
	TSonorkCCacheEntry *CL=LoadEntry( line_no , SONORK_CCACHE_SD_RANDOM);
	if(!CL)return last_result;
	CL->dat_index=dat_index;

	// Invalidate Text: Data points to another entry
	*(cache.text_status + line_no - cache.loaded.offset)=SONORK_CCACHE_TEXT_STATUS_NOT_LOADED;

	file.header.flags|=SONORK_CCACHE_FLAG_CACHE_DIRTY;
	if( !IsBulkMode() )FlushCache();

	return SONORK_RESULT_OK;
}

SONORK_RESULT
 TSonorkCCache::Set(DWORD line_no,TSonorkTag tag,DWORD*ext_index)
{
	TSonorkCCacheEntry *CL=LoadEntry( line_no , SONORK_CCACHE_SD_RANDOM);
	if(!CL)return last_result;
	CL->tag.Set( tag );
	if(ext_index!=NULL)
		CL->ext_index=*ext_index;
	file.header.flags|=SONORK_CCACHE_FLAG_CACHE_DIRTY;
	if( !IsBulkMode() )FlushCache();

	return SONORK_RESULT_OK;
}

TSonorkCCache::TSonorkCCache(
		DWORD 				text_size
	, 	DWORD 				cache_size
	, 	TSonorkCCacheCallbackPtr	cb_ptr
	,	void*				cb_tag)
{
	cb.ptr	= cb_ptr;
	cb.tag	= cb_tag;
	cache.text_size = text_size;
	if(cache.text_size<16)
		cache.text_size=16;
	else
	if(cache.text_size>16384)
		cache.text_size=16384;

	cache.entries=cache_size;
	if(cache.entries<2)cache.entries=2;
	else
	if(cache.entries>1024)cache.entries=1024;

	file.handle				= SONORK_INVALID_FILE_HANDLE;
	SONORK_MEM_NEW( cache.entry_buffer	= new TSonorkCCacheEntry[cache.entries] );
	SONORK_MEM_NEW( cache.text_buffer	= new char[cache.entries * cache.text_size] );
	SONORK_MEM_NEW( cache.text_status	= new SONORK_CCACHE_TEXT_STATUS[cache.entries] );
	ResetHeader(true);
	cache.loads=cache.saves=cache.hits=cache.misses=0;

}

TSonorkCCache::~TSonorkCCache()
{
	CloseCacheFile(true);
	SONORK_MEM_DELETE_ARRAY( cache.entry_buffer );
	SONORK_MEM_DELETE_ARRAY( cache.text_buffer );
	SONORK_MEM_DELETE_ARRAY( cache.text_status );
}

SONORK_RESULT
 TSonorkCCache::Add(TSonorkCCacheEntry& newEntry,DWORD* pLineNo)
{
	DWORD	offset;
	file.err_code=0;
	if(!IsOpen())
	{
		last_result=SONORK_RESULT_INVALID_OPERATION;
		return last_result;
	}
	offset = GetEofOffset();
	if((file.err_code=
		SONORK_IO_SetFilePointer(file.handle
		,offset
		,SONORK_FILE_SEEK_BEGIN))!=0)
		last_result=SONORK_RESULT_STORAGE_ERROR;
	else
	if((file.err_code=
		SONORK_IO_WriteFile(file.handle
		,&newEntry
		,sizeof(TSonorkCCacheEntry)))!=0)
		last_result=SONORK_RESULT_STORAGE_ERROR;
	else
	{
		last_result=SONORK_RESULT_OK;
		if(pLineNo)
			*pLineNo = file.header.entries;
		if(	cache.loaded.offset+cache.loaded.entries == file.header.entries
			&&
			cache.loaded.entries < cache.entries)
		{
			memcpy(CacheEntryPtr(cache.loaded.entries) , &newEntry, sizeof(TSonorkCCacheEntry) );
			cache.loaded.entries++;
			cache.hits++;
		}
		else
			cache.misses++;
		file.header.entries++;

		// we mark the HEADER as dirty, the cache remains intact
		// as we've writting the data directly to the disk
		if( IsBulkMode() )
		{
			file.header.flags|=SONORK_CCACHE_FLAG_HEADER_DIRTY;
		}
		else
		{
			FlushHeader();
		}
	}
	return last_result;
}

void
 TSonorkCCache::ResetHeader( bool clear_values )
{
	file.header.signature	= SONORK_FILE_SIGNATURE;
	file.header.version	= SONORK_CCACHE_VERSION;
	file.header.entry_size	= sizeof(TSonorkCCacheEntry);
	file.header.entries	=
	file.header.reserved	=
	file.header.flags  	=
	cache.loaded.entries 	= 0;
	cache.loaded.offset   	= SONORK_INVALID_INDEX;

	if(clear_values)
		SONORK_ZeroMem( &file.header.value, sizeof(file.header.value) );
}


void
 TSonorkCCache::Flush(bool forced, bool buffers)
{
	if( IsOpen() )
	{
		if((file.header.flags&SONORK_CCACHE_FLAG_CACHE_DIRTY) || forced)
			FlushCache();

		if((file.header.flags&SONORK_CCACHE_FLAG_HEADER_DIRTY) || forced)
			FlushHeader();
		if(  buffers )
			SONORK_IO_Flush( file.handle );
	}
}

int
 TSonorkCCache::FlushHeader()
{
	file.header.flags&=~SONORK_CCACHE_FLAG_HEADER_DIRTY;
	file.err_code = SONORK_IO_SetFilePointer(file.handle,0,SONORK_FILE_SEEK_BEGIN);
	if(file.err_code==0)
	{
		file.err_code = SONORK_IO_WriteFile(
			  file.handle
			,&file.header
			,sizeof(file.header)
			);
	}
	return file.err_code;
}

void TSonorkCCache::FlushCache()
{
	file.err_code=0;
	for(;;)
	{
		if(!IsOpen())
		{
			//last_result=SONORK_RESULT_INVALID_OPERATION;
			break;
		}

		if( !(file.header.flags&SONORK_CCACHE_FLAG_CACHE_DIRTY) || cache.loaded.entries == 0)
		{
			//last_result=SONORK_RESULT_OK;
			break;
		}
		if( cache.loaded.offset > file.header.entries || cache.loaded.entries > cache.entries)
		{
			//last_result=SONORK_RESULT_INTERNAL_ERROR;
			break;
		}
		file.header.flags&=~SONORK_CCACHE_FLAG_CACHE_DIRTY;
		cache.saves++;
		if((file.err_code=
				SONORK_IO_SetFilePointer(file.handle
				,GetEntryFileOffset( cache.loaded.offset )
				,SONORK_FILE_SEEK_BEGIN))==0)
		{
			if((file.err_code=SONORK_IO_WriteFile(file.handle
				  ,CacheEntryPtr(0)
				  ,cache.loaded.entries * sizeof(TSonorkCCacheEntry)
				  ))==0)
			{
				//last_result=SONORK_RESULT_OK;
				break;
			}
		}
		//last_result=SONORK_RESULT_STORAGE_ERROR;
		break;
	}
//	return last_result;
}
UINT TSonorkCCache::GetEntryFileOffset( UINT line_no ) const
{
	return sizeof(TSonorkCCacheHeader) + ( line_no *  sizeof(TSonorkCCacheEntry) );
}

UINT TSonorkCCache::ExpectedFileSize()
{
	return GetEntryFileOffset( file.header.entries );
}








//----------------------------------------------------------------------------
// TSonorkCCacheSelection

void	TSonorkCCacheSelection::InitEnum(TSonorkListIterator&I) const
{
	sel.InitEnum( I );
}
DWORD	TSonorkCCacheSelection::EnumNext(TSonorkListIterator& I) const
{
	DWORD   *pV;
	while( (pV=(DWORD*)sel.w_EnumNext(I)) != NULL)
		if(*pV!=SONORK_INVALID_INDEX)
			return *pV;
	return SONORK_INVALID_INDEX;
}
bool	TSonorkCCacheSelection::Contains(DWORD v) const
{
	if( v>=min && v<=max )
	{
		DWORD   no,items,*pV;
		if( v==min || v==max )
			return true;
		items=sel.Items();
		for( no=0,pV = (DWORD*)sel.w_ItemPtr(0)
			;no<items
			;no++,pV++)
		{
			if(*pV == v)
				return true;
		}
	}
	return false;
}
// NB: Add() and Del() are slighlty complex-er than strictly needed
//  in order to keep min and max updated so that Contains()
//  executes faster because Contains() is usually called much more times.
void	TSonorkCCacheSelection::Toggle(DWORD v)
{
	if(Contains(v))
		Del(v);
	else
		Add(v);
}

void	TSonorkCCacheSelection::Add(DWORD v)
{
	if(v!=SONORK_INVALID_INDEX && sel_items<SONORK_CCACHE_MAX_SELECTION_SIZE)
	{
		DWORD 	*pV, *sV,max_no,no;

		// if no items yet, set min,max to the new value
		if( min == SONORK_INVALID_INDEX )
			min=max=v;
		else
		if(v<min)
			min=v;
		else
		if(v>max)
			max=v;
		// Try to place the item towards the beginning of the list; the list
		// will have SONORK_INVALID_INDEX values inside if Del() was invoked
		max_no=sel.Items();
		for(no=0,pV = (DWORD*)sel.w_ItemPtr(0),sV=NULL
			;no<max_no
			;no++,pV++)
		{
			if(*pV == SONORK_INVALID_INDEX )
			{
				if(sV == NULL )
					sV=pV;
			}
			else
			if(*pV == v )
				return; 	// already exists
		}
		sel_items++;
		if( sV != NULL )
			*sV=v;
		else
			sel.w_AddItem(&v);
	}
}
void	TSonorkCCacheSelection::Del(DWORD v)
{
	DWORD 	*pV, no,max_no;

	if(min==SONORK_INVALID_INDEX||v<min||v>max||v==SONORK_INVALID_INDEX||!sel_items)
		return;
	// Recalculate min/max
	min=max=SONORK_INVALID_INDEX;
	max_no=sel.Items();
	for(no=0,pV = (DWORD*)sel.w_ItemPtr(0)
		;no<max_no
		;no++,pV++)
	{
		if(*pV==SONORK_INVALID_INDEX)
			continue;
		if(*pV == v)
		{
			*pV=SONORK_INVALID_INDEX;
			sel_items--;
			continue;
		}
		if( min == SONORK_INVALID_INDEX )
			min=max=*pV;
		else
		if( *pV < min )
			min=*pV;
		else
		if( *pV > max )
			max=*pV;
	}
	if(min == SONORK_INVALID_INDEX && sel.Items())
		Clear();
}
void	TSonorkCCacheSelection::Sort()
{
	DWORD	i,j,max_no;
	DWORD   *iV,*jV,tV;
	if(min==SONORK_INVALID_INDEX||sel_items<2)return;
	max_no=sel.Items();
	for( i = 0, iV = (DWORD*)sel.w_ItemPtr(0); i<max_no - 1 ; i++ , iV++)
		for( j = i+1, jV = (DWORD*)sel.w_ItemPtr(i+1); j<max_no ; j++ , jV++)
		{
			if( *iV > *jV )
			{
				tV = *iV;
				*iV = *jV;
				*jV = tV;
			}
		}
}
void	TSonorkCCacheSelection::Clear()
{
	sel_items=0;
	min=max=SONORK_INVALID_INDEX;
	sel.Clear();
}
