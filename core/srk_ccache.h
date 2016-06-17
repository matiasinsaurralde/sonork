#if !defined(SRK_CCACHE_H)
#define SRK_CCACHE_H

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


class TSonorkCCacheLineOld;

#define SONORK_CCACHE_FLAG_HEADER_DIRTY			0x00000001
#define SONORK_CCACHE_FLAG_CACHE_DIRTY			0x00000002
#define SONORK_CCACHE_FLAG_BULK_MODE			0x10000000
#define SONORK_CCACHE_MASK_RUN_TIME			0xff000000
#define	SONORK_CCACHE_TAG_FLAGS				0
#define	SONORK_CCACHE_TAG_INDEX				1
#define SONORK_CCACHE_MAX_SELECTION_SIZE		256

#define SONORK_CCACHE_DEFAULT_LINKED_SCAN_LINES		512
#define SONORK_CCACHE_DEFAULT_MARKER_SCAN_LINES		2048
#define SONORK_CCACHE_MAXIMUM_LINKED_SCAN_LINES		262144
#define SONORK_CCACHE_MAXIMUM_MARKER_SCAN_LINES		1048576

#define SONORK_CCACHE_TEXT_STATUS_NOT_LOADED	((BYTE)0)
#define SONORK_CCACHE_TEXT_STATUS_SHORT		((BYTE)1)
#define SONORK_CCACHE_TEXT_STATUS_LONG		((BYTE)2)

typedef BYTE SONORK_CCACHE_TEXT_STATUS;

enum SONORK_CCACHE_LINE_FLAG
{
  SONORK_CCLF_INCOMMING 		= 0x00000001
, SONORK_CCLF_01 			= 0x00000002	// UNREAD
, SONORK_CCLF_02 			= 0x00000004	// DENIED
, SONORK_CCLF_03 			= 0x00000008	// PROCESSED
, SONORK_CCLF_04 			= 0x00000010
, SONORK_CCLF_05 			= 0x00000020
, SONORK_CCLF_06			= 0x00000040
, SONORK_CCLF_07			= 0x00000080
, SONORK_CCLF_RESERVED_01		= 0x00000100
, SONORK_CCLF_RESERVED_02		= 0x00000200
, SONORK_CCLF_RESERVED_03		= 0x00000400
, SONORK_CCLF_RESERVED_04		= 0x00000800
, SONORK_CCLF_08			= 0x00001000
, SONORK_CCLF_09			= 0x00002000
, SONORK_CCLF_10			= 0x00004000
, SONORK_CCLF_11			= 0x00008000
, SONORK_CCLF_THREAD_START		= 0x00010000
, SONORK_CCLF_IN_THREAD			= 0x00020000
, SONORK_CCLF_EXT_DATA			= 0x00040000
, SONORK_CCLF_RESERVED_05		= 0x00080000
, SONORK_CCLF_12			= 0x00100000
, SONORK_CCLF_13			= 0x00200000
, SONORK_CCLF_14			= 0x00400000
, SONORK_CCLF_15			= 0x00800000
, SONORK_CCLF_RESERVED_06		= 0x01000000
, SONORK_CCLF_RESERVED_07		= 0x02000000
, SONORK_CCLF_RESERVED_08		= 0x04000000
, SONORK_CCLF_RESERVED_09		= 0x08000000
};
enum SONORK_CCACHE_VALUE
{
  SONORK_CCACHE_VIEW_OFFSET
, SONORK_CCACHE_FOCUS_LINE
, SONORK_CCACHE_FIRST_UNREAD
, SONORK_CCACHE_USER_1
, SONORK_CCACHE_USER_2
, SONORK_CCACHE_USER_3
, SONORK_CCACHE_USER_4
, SONORK_CCACHE_USER_5
, SONORK_CCACHE_VALUES
};
enum SONORK_CCACHE_SCAN_DIRECTION
{
  SONORK_CCACHE_SD_BACKWARDS
, SONORK_CCACHE_SD_RANDOM
, SONORK_CCACHE_SD_FORWARD
};
#define CCACHE_MAX_LINK_DEPTH		16
enum SONORK_CCACHE_GET_LINKED_METHOD
{
  SONORK_CCACHE_GET_LINKED_DEFAULT		// default: goes up, then down
, SONORK_CCACHE_GET_LINKED_TOP_OF_THREAD		// searches top of the thread
, SONORK_CCACHE_GET_LINKED_FORWARD_ONLY		// Does not reverse
};

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,4)
#endif


struct  TSonorkCCacheEntry
{
	TSonorkTag    	tag;
	TSonorkTime   	time;
	SONORK_DWORD2	tracking_no;
	DWORD		dat_index;
	DWORD		ext_index;

	TSonorkTag&
		Tag()
		{ return tag;}

	TSonorkTime&
		Time()
		{ return time;}

	SONORK_DWORD2&
		TrackingNo()
		{ return tracking_no;}

	DWORD
		DatIndex() const
		{ return dat_index;}
	DWORD
		ExtIndex() const
		{ return ext_index;}

	void
		Set(TSonorkCCacheLineOld* CL);

	void
		Clear()
		{ SONORK_ZeroMem(this,sizeof(*this)); }

}__SONORK_PACKED;



#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif

struct TSonorkCCacheMark
{
	DWORD		line_no;
	SONORK_DWORD2	tracking_no;
	TSonorkTime	time;

	bool
		IsZero() const
		{ return tracking_no.IsZero();	}

	void
		Set(const TSonorkCCacheMark&O)
		{ memcpy(this,&O,sizeof(*this)); }

	void
		Set(const TSonorkCCacheEntry* CL) // NB! <line_no> is not loaded!
		{ tracking_no.Set(CL->tracking_no); time.Set(CL->time);	}

	void
		Set(DWORD p_line_no,const TSonorkCCacheEntry*CL)
		{ line_no = p_line_no;	Set( CL ); }

	void
		Clear()
		{ SONORK_ZeroMem(this,sizeof(*this)); }

	bool
		Equ( const TSonorkCCacheMark*O )
		{
			return  (tracking_no	== O->tracking_no)
			&&	(time		== O->time);
		}
};


#include "srk_data_types.h"
#include "srk_atom_db.h"

struct TSonorkCCacheSelection
{
private:
	DWORD			min,max;
	UINT			sel_items;
	TSonorkSimpleDataList	sel;
public:
	TSonorkCCacheSelection()
		:sel(sizeof(DWORD),128)
		{min=SONORK_INVALID_INDEX;sel_items=0;}


	bool
		Active() const
		{ return sel_items!=0;	}

	UINT
		Items()	const
		{ return sel_items;}

	bool
		Contains(DWORD) const;

	DWORD
		Min()	const
		{ return min;}

	DWORD
		Max()	const
		{ return max;}

	void
		Add(DWORD);

	void
		Del(DWORD);

	void
		Toggle(DWORD);

	void
		Clear();

	void
		InitEnum(TSonorkListIterator&) const;

	DWORD
		EnumNext(TSonorkListIterator&) const;

	void
		Sort();

};


struct TSonorkCCacheLinkContext
{
friend class TSonorkCCache;
private:

	DWORD	count;
	DWORD	tracking_no[CCACHE_MAX_LINK_DEPTH];
	DWORD	top_line_no;
	bool	search_next;
public:

	TSonorkCCacheLinkContext()
		{ Clear(); }

	void
		Clear();

	BOOL
		Contains(DWORD tracking_no);

	void
		Add( DWORD tracking_no );
};


// Callback must return <true> if text is longer that <size>

typedef BOOL SONORK_CALLBACK
	TSonorkCCacheCallback(void*,TSonorkCCacheEntry*,char*,UINT size);
	
typedef
	TSonorkCCacheCallback* TSonorkCCacheCallbackPtr;

class  TSonorkCCache
{
public:

	struct TSonorkCCacheHeader
	{
		DWORD	signature;
		DWORD	version;
		DWORD	entries;
		DWORD	entry_size;
		DWORD	reserved;
		DWORD	code;
		DWORD	value[SONORK_CCACHE_VALUES];
		DWORD	flags;
	} __SONORK_PACKED;

	struct _FILE
	{
		TSonorkCCacheHeader 		header;
		SONORK_FILE_HANDLE      	handle;
		int		    		err_code;

	}file;

	struct _CB
	{
		TSonorkCCacheCallbackPtr	ptr;
		void		    	*	tag;

	}cb;

	struct _CACHE
	{
		struct {
			DWORD offset,	entries;
		}loaded;
		TSonorkCCacheEntry* 	entry_buffer;
		char*			text_buffer;
		SONORK_CCACHE_TEXT_STATUS*
					text_status;
		UINT			entries;
		UINT			text_size;
		UINT                	loads,saves,hits,misses;
	}cache;

	TSonorkCCacheLinkContext	link_ctx;
	SONORK_RESULT  			last_result;

	void
		ResetHeader(bool clear_values);

	SONORK_RESULT
		OpenCacheFile(const char*path);

	void
		CloseCacheFile(bool flush);

	UINT
		ExpectedFileSize();

	SONORK_RESULT
		LoadCache(DWORD offset);

	UINT
		GetEntryFileOffset( UINT line_no ) const;

	UINT
		GetEofOffset() const
		{ return GetEntryFileOffset( file.header.entries ); }

	TSonorkCCacheEntry *
		CacheEntryPtr(UINT offset_within_cache );

	char*
		CacheTextPtr(UINT offset_within_cache );

	char*
		LoadText( UINT offset_within_cache );

	TSonorkCCacheEntry*
		LoadEntry(DWORD line_no
			, SONORK_CCACHE_SCAN_DIRECTION scan_dir);

	InvalidateCache();

public:
	TSonorkCCache( DWORD 			max_text_length
		     , DWORD		 	cache_size
		     , TSonorkCCacheCallbackPtr	cb_ptr
		     , void*			cb_tag);
	~TSonorkCCache();


	// IMPORTANT: (*1)
	// The TSonorkCCacheEntry* pointer returned by
	// the Get() and Getxxx() functions
	// IS VALID ONLY UNTIL ANY OF THE NON-CONST METHODS ARE CALLED AGAIN

	SONORK_RESULT
		Add(TSonorkCCacheEntry&,DWORD*line_no);

	TSonorkCCacheEntry*
		Get(  	  DWORD 			line_no
			, SONORK_C_CSTR* 		str_ptr
			, SONORK_CCACHE_TEXT_STATUS*	pstatus
			, SONORK_CCACHE_SCAN_DIRECTION 	scan_dir=SONORK_CCACHE_SD_RANDOM);

	TSonorkCCacheEntry*
		GetByMark(TSonorkCCacheMark*
			, SONORK_C_CSTR* str_ptr
			, DWORD max_scan_lines = SONORK_CCACHE_DEFAULT_MARKER_SCAN_LINES
			);

	// IMPORTANT: (*2)
	//  GetLinked() takes a TSonorkCCacheEntry* as parameter <CL>
	//  As stated under (*1), any call to Getxxx() functions
	//  invalidates any TSonorkCCacheEntry* that the caller may hold,
	//  so keep in mind that <CL> may NOT be used after this call.
	//  Before calling, set <start_line_no> to the line number of the <CL>
	//  entry and set <scan_dir> to the direction GetLinked() should
	//  scan first. If the linked line is not found in that direction,
	//  and <may_reverse_scan_dir> is true, GetLinked() will reverse
	//  the scan direction on try again. <scan_dir> MUST be either
	//  one of SONORK_CCACHE_SD_BACKWARDS or SONORK_CCACHE_SD_FORWARD.

	// if a NULL is returned, no linked line was found.
	// if a line is found, a new TSonorkCCacheEntry* pointer
	// is returned, the LinkContext().LineNo() contains the line number
	// if <found_line_text> is not-null, the text will be loaded and
	//  this parameter will point to the buffer where it was loaded.
	// <scan_dir> will return with the direction being scanned
	// when the line was found.

	TSonorkCCacheLinkContext&
		LinkContext()
		{ return link_ctx; }

	void
		ClearLinkContext()
		{ link_ctx.Clear(); }

	TSonorkCCacheEntry*
		GetLinked(DWORD				start_line_no
			, const TSonorkCCacheEntry* 	CL
			, DWORD&			found_line_no
			, SONORK_C_CSTR*		found_line_text
			, SONORK_CCACHE_GET_LINKED_METHOD method=SONORK_CCACHE_GET_LINKED_DEFAULT
			, DWORD max_scan_lines = SONORK_CCACHE_DEFAULT_LINKED_SCAN_LINES);

	TSonorkCCacheEntry*
		GetNext(  DWORD&			line_no
			, DWORD				set_flags
			, DWORD				flag_mask
			);

	SONORK_RESULT
		Set(DWORD line_no
			,TSonorkTag
			, DWORD*ext_entry_no);

	SONORK_RESULT
		SetDatIndex(DWORD line_no,DWORD);


	SONORK_RESULT
		Del(TSonorkCCacheSelection& i_sel);

	SONORK_RESULT
		Open(SONORK_C_CSTR path);

	void
		Flush(bool forced=false, bool buffers=true);

	int
		FlushHeader();

	void
		FlushCache();

	void
		Close();

	// SetBuldMode
	//  if true, CCache does not flush
	//  until SetBulkMode() is set to false
	void
		SetBulkMode(BOOL v);

	BOOL
		IsBulkMode()	const
		{ return file.header.flags&SONORK_CCACHE_FLAG_BULK_MODE;}

	bool
		IsOpen()   const
			{ return file.handle!=SONORK_INVALID_FILE_HANDLE;}
	SONORK_RESULT
		Clear(bool clear_values);

	UINT  	Lines()  const
		{ return file.header.entries;}

	SONORK_RESULT
		Result() const
		{ return last_result;}

	int  	FileErrCode()	const
		{ return file.err_code; }

	UINT
		CacheSize() const
		{ return cache.entries;}

	UINT	TextSize()  const
		{ return cache.text_size;}

	DWORD
		CacheHits() const
		{ return cache.hits;}

	DWORD 	CacheMisses() const
		{ return cache.misses;}

	DWORD 	CacheLoads() const
		{ return cache.loads;}

	DWORD 	CacheSaves() const
		{ return cache.saves;}

	DWORD 	GetValue(SONORK_CCACHE_VALUE n) const
		{ return file.header.value[n];}

	void  	SetValue(SONORK_CCACHE_VALUE n,DWORD value);
};


#endif