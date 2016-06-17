#if !defined(SRK_DEFS_H)
#define SRK_DEFS_H

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL) Version 1.

	You should have received a copy of the SSCL along with this program;
	if not, write to sscl@sonork.com.

	You should NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL), doing so will indicate your agreement
	to the the terms which may be differ for each version of the software.

	This comment section, indicating the existence and requirement of
	acceptance of the SSCL may not be removed from the source code.
*/

// Client
#define SONORK_CLIENT_VERSION_MAJOR		1
#define SONORK_CLIENT_VERSION_MINOR		5
#define SONORK_CLIENT_VERSION_BUILD		1
#define SONORK_CLIENT_VERSION_PATCH		3

// Server
#define SONORK_SERVER_VERSION_MAJOR		1
#define SONORK_SERVER_VERSION_MINOR		5
#define SONORK_SERVER_VERSION_BUILD		1
#define SONORK_SERVER_VERSION_PATCH		3

#define	SONORK_MODULE_LINE		MAKE_SONORK_MODULE_LINE(__LINE__)
#define MAKE_VERSION_NUMBER(a,b,c,d)	( (((a)<<12)&0xf000) | (((b)<<8)&0x0f00) | (((c)<<4)&0x00f0) | (((d)<<0)&0x000f))
#define	SONORK_CLIENT_VERSION_NUMBER	MAKE_VERSION_NUMBER(SONORK_CLIENT_VERSION_MAJOR,SONORK_CLIENT_VERSION_MINOR,SONORK_CLIENT_VERSION_BUILD,SONORK_CLIENT_VERSION_PATCH)
#define	SONORK_SERVER_VERSION_NUMBER	MAKE_VERSION_NUMBER(SONORK_SERVER_VERSION_MAJOR,SONORK_SERVER_VERSION_MINOR,SONORK_SERVER_VERSION_BUILD,SONORK_CLIENT_VERSION_PATCH)

#if defined(SONORK_SERVER_BUILD)
#define MAKE_SONORK_MODULE_LINE(line)	( (SONORK_SERVER_VERSION_NUMBER<<16) | ((line)&0xffff) )
#else
#define MAKE_SONORK_MODULE_LINE(line)	( (SONORK_CLIENT_VERSION_NUMBER<<16) | ((line)&0xffff) )
#endif

#if !defined(SONORK_CODEC_LEVEL)
#	define SONORK_CODEC_LEVEL 10
#endif

#if defined(_WIN32)||defined(__WIN32__)
#	define SONORK_WIN32_BUILD
#elif defined(__linux__)
#	define SONORK_LINUX_BUILD
#elif defined(__BEOS__)
#	define SONORK_BEOS_BUILD
#else
#	error NO IMPLEMENTATION
#endif

// these definitions to be changed if byte are reversed
#undef SONORK_BYTE_REVERSE

#	define SONORK_REV_WORD(n)	( (((n)<<8)&0xff00) | (((n)>>8)&0x00ff) )
#	define SONORK_REV_DWORD(n)	( \
		  SONORK_REV_WORD( ( ( (n)<<16 )&0xffff0000 ) ) \
		| SONORK_REV_WORD( ( ( (n)>>16 )&0x0000ffff ) ) \
		)

#if defined(SONORK_BYTE_REVERSE)	
#	define SONORK_SET_WORD(ptr,n)	*(WORD*)(ptr)=(WORD)SONORK_REV_WORD(n)
#	define SONORK_SET_DWORD(ptr,n)	*(DWORD*)(ptr)=(DWORD)SONORK_REV_DWORD(n)
#	define SONORK_WORD(n)			((WORD)SONORK_REV_WORD( ((WORD)(n) ))
#	define SONORK_DWORD(n)			((DWORD)SONORK_REV_DWORD( (DWORD)(n) ))
#else
#	define SONORK_SET_WORD(ptr,n)	*(WORD*)(ptr)=(WORD)(n)
#	define SONORK_SET_DWORD(ptr,n)	*(DWORD*)(ptr)=(DWORD)(n)
#	define SONORK_WORD(n)			((WORD)(n))
#	define SONORK_DWORD(n)			((DWORD)(n))
#endif





#define SONORK_FILE_SIGNATURE    		0x5347494D
#define SONORK_MAX_PATH 			320
#define SONORK_CRYPT_DATA_SIZE			128
#define SONORK_USER_NOTES_SIZE 			(1024-sizeof(DWORD))
#define SONORK_ID_MAX_STR_SIZE	 		22
#define SONORK_USER_OLD_ALIAS_MAX_SIZE  	16
#define SONORK_USER_ALIAS_MAX_SIZE  		24
#define SONORK_USER_NAME_MAX_SIZE   		80
#define SONORK_USER_PASS_MAX_SIZE		24
#define SONORK_WAPP_NAME_MAX_SIZE		24
#define SONORK_WAPP_PATH_MAX_SIZE		126
#define SONORK_WAPP_URL_MAX_SIZE		(SONORK_WAPP_PATH_MAX_SIZE*2)
#define SONORK_MAX_USER_GROUPS			64
#define SONORK_USER_GROUP_NAME_MAX_SIZE		24
#define SONORK_GROUP_MAX_DEPTH			64
#define SONORK_SERVICE_NOTES_MAX_SIZE  		250
#define SONORK_EMAIL_MAX_SIZE			128
#define SONORK_INVALID_INDEX        		((DWORD)-1)

#define SONORK_TRACKER_ROOM_NAME_MAX_SIZE	48
#define SONORK_TRACKER_ROOM_TEXT_MAX_SIZE	512
#define SONORK_TRACKER_ROOM_DATA_MAX_SIZE	4096

#define SONORK_TRACKER_DATA_TEXT_MAX_SIZE	250
#define SONORK_TRACKER_DATA_DATA_MAX_SIZE	250

#define SONORK_INVALID_SID_ID			((DWORD)0)
#define SONORK_INVALID_TASK_ID			((DWORD)0)
#define SONORK_INVALID_LINK_ID			((DWORD)0)

#define SONORK_USER_SERVER_NAME_MAX_SIZE  	250
#define SONORK_USER_SERVER_TEXT_MAX_SIZE  	16384
#define SONORK_USER_SERVER_DATA_MAX_SIZE  	65536

#define	SONORK_SID_MSG_MAX_SIZE			250

#define	SONORK_ERROR_MAX_TEXT_LENGTH    	512
#define SONORK_ERROR_MAX_DATA_SIZE		( SONORK_ERROR_MAX_TEXT_LENGTH + sizeof(DWORD)*2 )

#define SONORK_PACKET_MAX_DATA_SIZE 		( 65536*4 )
#define SONORK_PACKET_MAX_FULL_SIZE		( SONORK_PACKET_MAX_DATA_SIZE + 64 )

#define SONORK_USER_MSG_MAX_TEXT_SIZE		((SONORK_PACKET_MAX_DATA_SIZE/2)-256)
#define SONORK_USER_MSG_MAX_DATA_SIZE		((SONORK_PACKET_MAX_DATA_SIZE/2)-256)
#define SONORK_CTRL_MSG_MAX_DATA_SIZE		( SONORK_PACKET_MAX_DATA_SIZE - 1024 )


#define SONORK_USER_INFO_MAX_LEVEL  		SONORK_USER_INFO_LEVEL_3
#define SONORK_AUTH_INFO_MAX_LEVEL 		SONORK_USER_INFO_MAX_LEVEL
#define SONORK_AUTH_TAG_GROUP_MASK		0x000000ff
#define TSonorkUserInfoMax			TSonorkUserInfo3

#define SIZEOF_DWORD				sizeof(DWORD)
#define SIZE_IN_DWORDS(n)			((n)/SIZEOF_DWORD)
#define SIZEOF_IN_BYTES(n)			(sizeof(n))
#define SIZEOF_IN_DWORDS(n)			( SIZE_IN_DWORDS( sizeof(n) ) )

#define csNULL					((SONORK_C_STR)0)
#define wsNULL					((SONORK_W_STR)0)

enum SONORK_GROUP_TYPE
{
  SONORK_GROUP_TYPE_NONE
, SONORK_GROUP_TYPE_USER
, SONORK_GROUP_TYPE_WAPP
, SONORK_GROUP_TYPE_MTPL_obsolete
, SONORK_GROUP_TYPE_NEWS
, SONORK_GROUP_TYPE_TRACKER_GROUP
, SONORK_GROUP_TYPE_TRACKER_ROOM
};

// SONORK_WAPP_TYPE is a sub-set of SONORK_GROUP_TYPE
enum SONORK_WAPP_TYPE
{
  SONORK_WAPP_TYPE_NONE		= 0
, SONORK_WAPP_TYPE_WAPP		= SONORK_GROUP_TYPE_WAPP
, SONORK_WAPP_TYPE_MTPL		= SONORK_GROUP_TYPE_MTPL_obsolete
};
enum SONORK_WAPP_FLAGS
{
  SONORK_WAPP_F_USER_CONTEXT			=0x00000100
};
enum SONORK_EMAIL_ACCOUNT_TYPE
{
  SONORK_EMAIL_ACCOUNT_NONE
, SONORK_EMAIL_ACCOUNT_POP3
};

enum SONORK_SERVICE_STATE
{
  SONORK_SERVICE_STATE_NONE	// Not registered, not servicing
, SONORK_SERVICE_STATE_PAUSED	// Registered, not servicing
, SONORK_SERVICE_STATE_AVAILABLE	// Registered, servicing but not accepting new requests
, SONORK_SERVICE_STATE_READY	// Registered, servicing and acccepting new requests
};

enum SONORK_SERVICE_PRIORITY
{
  SONORK_SERVICE_PRIORITY_IDLE		=0
, SONORK_SERVICE_PRIORITY_1		=1
, SONORK_SERVICE_PRIORITY_LOWEST	=SONORK_SERVICE_PRIORITY_1
, SONORK_SERVICE_PRIORITY_2
, SONORK_SERVICE_PRIORITY_3
, SONORK_SERVICE_PRIORITY_LOW		=SONORK_SERVICE_PRIORITY_3
, SONORK_SERVICE_PRIORITY_4
, SONORK_SERVICE_PRIORITY_5
, SONORK_SERVICE_PRIORITY_MEDIUM	=SONORK_SERVICE_PRIORITY_5
, SONORK_SERVICE_PRIORITY_NORMAL	=SONORK_SERVICE_PRIORITY_5
, SONORK_SERVICE_PRIORITY_6
, SONORK_SERVICE_PRIORITY_7
, SONORK_SERVICE_PRIORITY_8
, SONORK_SERVICE_PRIORITY_HIGH		=SONORK_SERVICE_PRIORITY_8
, SONORK_SERVICE_PRIORITY_9
, SONORK_SERVICE_PRIORITY_10
, SONORK_SERVICE_PRIORITY_HIGHEST	=SONORK_SERVICE_PRIORITY_10
};

enum SONORK_MSG_SYS_FLAGS
{
  SONORK_MSG_SF_SERVER_ORIGINATED	=0x00010000
, SONORK_MSG_SF_SELF_TRIGGERED		=0x00100000
, SONORK_MSG_SF_SERVER_AUTOFIX 		=0x00200000
, SONORK_MSG_SF_READ			=0x00400000
, SONORK_MSG_SF_CURRENT_SID		=0x00000001
, SONORK_MSG_SF_LOGGED			=0x00000002
, SONORK_MSG_SFM_USER_CONTROLLED	=0x00000fff
, SONORK_MSG_SFM_SERVER_CONTROLLED	=0xfffff000
};

enum SONORK_MSG_USR_FLAGS
{
  SONORK_MSG_UF_QUERY			=0x00000001	// Should reply
, SONORK_MSG_UF_REPLY			=0x00000010	// Is a reply
, SONORK_MSG_UF_NEGATIVE		=0x00000020	// Reply/Query is negative
, SONORK_MSG_UF_URGENT			=0x00000100 // Urgent flag
, SONORK_MSG_UF_AUTOMATIC		=0x00000200 // Not generated by human
, SONORK_MSG_UF_GROUP			=0x00000400 // Sent to a group of users
};
#define SONORK_TEXT_F_RTF		0x00010000

enum SONORK_OS_TYPE
{
  SONORK_OS_UNKNOWN		= 0x00000000
, SONORK_OS_WIN    		= 0x00000001
, SONORK_OS_LINUX      		= 0x00000002
, SONORK_OS_BEOS      		= 0x00000003
};

enum SONORK_LANG_CODE
{
  SONORK_LANG_CODE_EN		= 0x6e65
, SONORK_LANG_CODE_ES		= 0x7365
, SONORK_LANG_CODE_DE		= 0x6564
, SONORK_LANG_CODE_FR		= 0x7266
, SONORK_LANG_CODE_PT		= 0x7470
, SONORK_LANG_CODE_IT		= 0x7469
};

enum SONORK_USER_INFO_LEVEL
{
  SONORK_USER_INFO_LEVEL_0
, SONORK_USER_INFO_LEVEL_1
, SONORK_USER_INFO_LEVEL_2
, SONORK_USER_INFO_LEVEL_3
};

enum SONORK_RESULT
{
  SONORK_RESULT_OK		= 0
, SONORK_RESULT_NO_DATA
, SONORK_RESULT_DUPLICATE_DATA
, SONORK_RESULT_CODEC_ERROR
, SONORK_RESULT_PROTOCOL_ERROR
, SONORK_RESULT_INVALID_PARAMETER
, SONORK_RESULT_INVALID_VERSION
, SONORK_RESULT_INVALID_OPERATION
, SONORK_RESULT_INVALID_ENCRYPTION
, SONORK_RESULT_NOT_SUPPORTED
, SONORK_RESULT_SERVICE_BUSY
, SONORK_RESULT_QUOTA_EXCEEDED
, SONORK_RESULT_TIMEOUT
, SONORK_RESULT_OUT_OF_RESOURCES
, SONORK_RESULT_NOT_ACCEPTED
, SONORK_RESULT_NETWORK_ERROR
, SONORK_RESULT_STORAGE_ERROR
, SONORK_RESULT_INTERNAL_ERROR
, SONORK_RESULT_ACCESS_DENIED
, SONORK_RESULT_USER_TERMINATION
, SONORK_RESULT_FORCED_TERMINATION
, SONORK_RESULT_CONFIGURATION_ERROR
, SONORK_RESULT_INVALID_HANDLE
, SONORK_RESULT_NOT_AVAILABLE
, SONORK_RESULT_OS_ERROR
, SONORK_RESULT_INVALID_MODE
, SONORK_RESULT_INVALID_SERVER
, SONORK_RESULT_NOT_READY
, SONORK_RESULT_FUNCTION_DISABLED
, SONORK_RESULT_DATABASE_ERROR	= 100
, SONORK_RESULT_OK_PENDING	= 200
};

enum SONORK_DYN_STRING_TYPE
{
  SONORK_DYN_STRING_TYPE_NULL	= 0
, SONORK_DYN_STRING_TYPE_C	= 0x10000000
, SONORK_DYN_STRING_TYPE_W	= 0x20000000
};

#define SONORK_DYN_STRING_FM_TYPE	0xf0000000
#define SONORK_DYN_STRING_FM_LENGTH	0x0000ffff



#if defined(SONORK_WIN32_BUILD)

// Compiler-specific flags and bug fixes

# if defined(_MSC_VER)					// VISUAL C++ COMPILER (6.0 AND ABOVE)
#   define USE_PRAGMA_PUSH
# elif defined(__BORLANDC__)
#   if (__BORLANDC__  >= 0x0540)		// NEW BORLAND C++ COMPILER (>5.2)
#     define USE_PRAGMA_PUSH
#   else                       			// OLD BORLAND C++ COMPILER	(<=5.2)
#     if defined(__MT__)
#       define SONORK_MEM_SERIALIZE
#     endif
#   endif
#endif

#include <winsock.h>
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>



#if defined(__CONSOLE__)
# undef  wsprintf
# define wsprintf 	__DONT_USE_WSPRINTF__	// Don't allow wsprintf on non-GUI
#else
# undef	 printf
# define printf 	__DONT_USE_PRINTF__	// Don't allow printf on GUI applications
#endif


typedef HANDLE 				SONORK_PROCESS_HANDLE;
typedef HANDLE 				SONORK_FILE_HANDLE;
#define SONORK_CALLBACK 		WINAPI
#define SONORK_IO_DIR_SLASH		'\\'
#define SONORK_INVALID_FILE_HANDLE  	INVALID_HANDLE_VALUE


#define __SONORK_PACKED

#define	SONORK_WIN32_TIME_MSEC_DIVISOR	(10000i64)
#define SONORK_WIN32_TIME_SEC_DIVISOR	(10000000i64)

union SONORK_WIN32_TIME
{
	__int64			v64;
	DWORD			v32;
	FILETIME		ft;
};

// end of if defined(SONORK_WIN32_BUILD)
// ---------------------------------

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))
// Linux specific

#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h> // sockaddr_in
#include <stdlib.h>
#include <sys/types.h>	//accept(2)
#include <sys/socket.h>	//accept(2)

#include <sys/time.h>	//fd_set
#include <sys/types.h>	//fd_set
#include <dirent.h> // DIR* //fd_set
#include <unistd.h>	// pid_t //fd_set

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h> // stat(2)
#include <arpa/inet.h> // inet_addr(3)
#include <netdb.h>	//gethostbyname(3)

#include <sys/timeb.h>	//struct timeb
#include <assert.h>

#if defined(__GNUC__)
#define __SONORK_PACKED	__attribute__ ((packed))
#endif

#define SONORK_CALLBACK
#define INVALID_SOCKET 				-1
#define SOCKET_ERROR   				-1
#define SONORK_IO_DIR_SLASH			'/'
#define SONORK_INVALID_FILE_HANDLE  		NULL

int		GetLastError();
int		WSAGetLastError();

typedef unsigned int	UINT;
typedef unsigned long	DWORD;
typedef unsigned short	WORD;
typedef unsigned char	BYTE;
typedef FILE*		HANDLE;
typedef bool		BOOL;
typedef int 		SOCKET;
typedef fd_set		FD_SET;
typedef pid_t* 		SONORK_PROCESS_HANDLE;
typedef FILE* 		SONORK_FILE_HANDLE;


#else	// if !defined(SONORK_WIN32_BUILD|SONORK_LINUX_BUILD)

#error NO IMPLEMENTATION

#endif


typedef char			SONORK_C_CHAR;
#if defined(SONORK_WIN32_BUILD)
typedef wchar_t			SONORK_W_CHAR;
#else
typedef unsigned short		SONORK_W_CHAR;
#endif


typedef SONORK_C_CHAR*		SONORK_C_STR;
typedef SONORK_W_CHAR*		SONORK_W_STR;
typedef const SONORK_C_CHAR* 	SONORK_C_CSTR;
typedef const SONORK_W_CHAR*	SONORK_W_CSTR;

typedef DWORD 			SONORK_SID_ID;
typedef BYTE  			SONORK_CRYPT_METHOD;
typedef WORD  			SONORK_CRYPT_ID;
typedef DWORD 			SONORK_UTS_LINK_ID;

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,1)
#endif

// ----------------------------------------------------------------------------
// APP DEFINED FUNCTIONS:
// These functions *must* be implented in the host application, they are NOT
//  implemented in the Sonork library. Debug messages and the the result of
//  <sonork_printf> will be sent to sonork_puts, the host application may ignore
//  or do whatever it pleases with  those messages.
// *DO NOT* invoke Sonork functions from any of these functions.

#if defined(SONORK_EXTERNAL_MEM_ALLOCATOR)
extern BYTE*sonork_mem_alloc(UINT n);
extern void sonork_mem_free(void*ptr);
#else
inline BYTE*sonork_mem_alloc(UINT n){ return new BYTE[n+2]; }
inline void sonork_mem_free(void*ptr){ delete[] (BYTE*)ptr;}
#endif

extern void sonork_puts(const char *);

#if defined(SONORK_MEM_SERIALIZE)
extern	void sonork_mem_lock();
extern	void sonork_mem_unlock();
extern	BYTE*sonork_mem_locked_alloc(UINT sz);
extern  void sonork_mem_locked_free(void*);
# define SONORK_MEM_ALLOC(type,sz)		(type*)sonork_mem_locked_alloc(sz)
# define SONORK_MEM_FREE(ptr)			sonork_mem_locked_free(ptr)
# define SONORK_MEM_NEW(expression)  		{sonork_mem_lock();expression;sonork_mem_unlock();}
# define SONORK_MEM_DELETE(var)			{sonork_mem_lock();delete (var);sonork_mem_unlock();}
# define SONORK_MEM_DELETE_ARRAY(var)		{sonork_mem_lock();delete[] (var);sonork_mem_unlock();}
#else
# define sonork_mem_lock()
# define sonork_mem_unlock()
# define SONORK_MEM_ALLOC(type,sz)		(type*)sonork_mem_alloc(sz)
# define SONORK_MEM_FREE(ptr)			sonork_mem_free(ptr)
# define SONORK_MEM_NEW(expression)  		expression
# define SONORK_MEM_DELETE(var)			delete(var)
# define SONORK_MEM_DELETE_ARRAY(var)		delete[] (var)
#endif

enum SONORK_SERVICE_ID
{
// Notes: Base: Inclusive, Limit: Exclusive

  SONORK_SERVICE_ID_INVALID			=-1
, SONORK_SERVICE_ID_NONE			= 0
, SONORK_SERVICE_ID_SONORK			= 1
, SONORK_SERVICE_ID_IPC				= 2
, SONORK_SERVICE_ID_EXTERNAL_APP		= 3
, SONORK_SERVICE_ID_WAPP			= 10
, SONORK_SERVICE_ID_SONORK_CHAT			= 1000
, SONORK_SERVICE_ID_SONORK_TRACKER		= 1001
, SONORK_SERVICE_ID_DATA_SERVER			= 2049
, SONORK_SERVICE_ID_INTERNAL_LIMIT		= 4096
, SONORK_SERVICE_ID_SYSTEM_LIMIT		= 32384

, SONORK_SERVICE_ID_PRIVATE_BASE		= 32384
, SONORK_SERVICE_ID_PRIVATE_LIMIT		= 32896

, SONORK_SERVICE_ID_SHARED_BASE			= 32896
, SONORK_SERVICE_ID_SHARED_LIMIT		= 98944UL

, SONORK_SERVICE_ID_3RD_PARTY_BASE		= 1048576UL
, SONORK_SERVICE_ID_3RD_PARTY_LIMIT		= 1073741823UL
};

// SONORK_PRIVATE_SERVICES
//  This are services that are specific to a service
#define	SONORK_PRIVATE_SERVICE_BASE		SONORK_SHARED_SERVICE_TYPE_LIMIT
#define	SONORK_PRIVATE_SERVICE_LIMIT		(SONORK_PRIVATE_SERVICE_BASE + 8192)
#define SONORK_SERVICE_ID_DEMO_APP 		((SONORK_SERVICE_ID)(SONORK_SERVICE_ID_PRIVATE_BASE+1))

// SONORK_SHARED_SERVICES
//  This are services that can be provided by all services.
enum SONORK_SERVICE_TYPE
{
  SONORK_SHARED_SERVICE_TYPE_BASE		= 0
, SONORK_SERVICE_TYPE_NONE			= 0
, SONORK_SERVICE_TYPE_AUTHS			= 1
, SONORK_SERVICE_TYPE_URL			= 2
, SONORK_SERVICE_TYPE_MSGTPL			= 3
, SONORK_SERVICE_TYPE_EMAIL			= 4
, SONORK_SERVICE_TYPE_SONORK_CHAT		= 1000
, SONORK_SERVICE_TYPE_SONORK_FILE		= 2049
, SONORK_SERVICE_TYPE_TELNET			= 2050
, SONORK_SHARED_SERVICE_TYPE_LIMIT_NEW		= 8192	// Was 4096
, SONORK_PRIVATE_SERVICE_TYPE_BASE		= SONORK_SHARED_SERVICE_TYPE_LIMIT_NEW
, SONORK_SERVICE_TYPE_01
, SONORK_SERVICE_TYPE_02
, SONORK_SERVICE_TYPE_03
, SONORK_SERVICE_TYPE_04
, SONORK_SERVICE_TYPE_05
, SONORK_SERVICE_TYPE_06
, SONORK_SERVICE_TYPE_07
, SONORK_SERVICE_TYPE_08
, SONORK_SERVICE_TYPE_09
, SONORK_SERVICE_TYPE_10
, SONORK_SERVICE_TYPE_11
, SONORK_SERVICE_TYPE_12
, SONORK_SERVICE_TYPE_13
, SONORK_SERVICE_TYPE_14
, SONORK_SERVICE_TYPE_15
, SONORK_SERVICE_TYPE_16
, SONORK_SERVICE_TYPE_17
, SONORK_SERVICE_TYPE_18
, SONORK_SERVICE_TYPE_19
, SONORK_SERVICE_TYPE_20
, SONORK_PRIVATE_SERVICE_TYPE_LIMIT	=(SONORK_PRIVATE_SERVICE_TYPE_BASE+2048)
};
enum SONORK_SERVICE_FLAGS
{
  SONORK_SERVICE_FLAG_RESERVED_01		= 0x00002000
, SONORK_SERVICE_FLAG_RESERVED_02		= 0x00004000
, SONORK_SERVICE_FLAG_RESERVED_03		= 0x00008000
, SONORK_SERVICE_FLAG_RESERVED_04		= 0x00010000
, SONORK_SERVICE_FLAG_RESERVED_05		= 0x00020000
, SONORK_SERVICE_FLAG_RESERVED_06		= 0x00040000
, SONORK_SERVICE_FLAG_RESERVED_07		= 0x00080000
, SONORK_SERVICE_FLAG_01			= 0x00100000
, SONORK_SERVICE_FLAG_02			= 0x00200000
, SONORK_SERVICE_FLAG_03			= 0x00400000
, SONORK_SERVICE_FLAG_04			= 0x00800000
, SONORK_SERVICE_FLAG_05			= 0x01000000
, SONORK_SERVICE_FLAG_06			= 0x02000000
, SONORK_SERVICE_FLAG_07			= 0x04000000
, SONORK_SERVICE_FLAG_08			= 0x08000000
, SONORK_SERVICE_FLAG_RESERVED_08		= 0x10000000
, SONORK_SERVICE_FLAG_RESERVED_09		= 0x20000000
, SONORK_SERVICE_FLAG_RESERVED_10		= 0x40000000
, SONORK_SERVICE_PRIVATE_FLAG_MASK		= 0x0ff00000
, SONORK_SERVICE_RESERVED_FLAG_MASK		= 0x300fe000
};

#define SONORK_IDENTIFY_F_VALID_PIN_TYPE	0x00000001
#define SONORK_IDENTIFY_F_SID_EXISTS		0x00000010
#define SONORK_IDENTIFY_F_PIN_MATCH		0x00000020

#define SONORK_IDENTIFY_MATCH_OK(flags)		( flags & SONORK_IDENTIFY_F_PIN_MATCH )

struct SONORK_DWORD2
{
	DWORD	v[2];

	void
		Set(DWORD v1,DWORD v2)
		{ v[0]=v1;v[1]=v2;}
	void
		Set(const SONORK_DWORD2&O)
		{ v[0]=O.v[0];v[1]=O.v[1];}

	bool
		operator ==(const SONORK_DWORD2 &O) const
		{return v[0]==O.v[0]&&v[1]==O.v[1];}

	bool
		operator !=(const SONORK_DWORD2 &O) const
		{return v[0]!=O.v[0]||v[1]!=O.v[1];}

	bool
		operator < (const SONORK_DWORD2 &O) const
		{return ((v[0]<O.v[0]) || (v[0]==O.v[0]&&v[1]< O.v[1]));}

	bool
		operator > (const SONORK_DWORD2 &O) const
		{return ((v[0]>O.v[0]) || (v[0]==O.v[0]&&v[1]> O.v[1]));}

	bool
		operator <=(const SONORK_DWORD2 &O) const
		{return ((v[0]<O.v[0]) || (v[0]==O.v[0]&&v[1]<=O.v[1]));}

	bool
		operator >=(const SONORK_DWORD2 &O) const
		{return ((v[0]>O.v[0]) || (v[0]==O.v[0]&&v[1]>=O.v[1]));}

	bool
		IsZero() const
		{ return !v[0]&&!v[1];}

	void
		Clear()
		{v[0]=v[1]=0;}

	void
		SetZero()
		{v[0]=v[1]=0;}

	bool
		SetStr(SONORK_C_CSTR str,UINT base=10);

	SONORK_C_STR
		GetStr(SONORK_C_STR str,UINT  base=10) const;

	SONORK_C_STR
		GetStrEx(SONORK_C_STR str,SONORK_C_CSTR prefix,SONORK_C_CSTR sufix) const;

	void
		operator++()
		{ if(++v[0]==0)v[1]++; }

	void
		operator++(int)
		{ if(++v[0]==0)v[1]++; }

	// CheckNeg returns true if either one of the values
	// are negative (We use this function to prevent
	// adding negative values to the database because
	// the ODBC driver adds them OK, but generates an
	// error when retrieving).
	
	BOOL
		CheckNeg() const
		{ return (v[0]|v[1])&0x80000000; }

	void
		ClearNeg()
		{ v[0]&=~0x80000000;v[1]&=~0x80000000;}



	// AbsDiff: returns absolute difference with other value (up to 0xffffffff)
	UINT
		AbsDiff(const SONORK_DWORD2&) const;


} __SONORK_PACKED;

struct SONORK_DWORD4
{
	SONORK_DWORD2	v[2];

	void
		Set(const SONORK_DWORD4&O);
		
	bool
		operator ==(const SONORK_DWORD4 &O) const ;

	bool
		operator !=(const SONORK_DWORD4 &O) const
		{return !(*this==O);}

	bool
		IsZero() const ;

	void
		Clear();

	void
		SetZero()
		{ Clear(); }

	SONORK_DWORD2&
		LoTag()
		{ return v[0];}

	SONORK_DWORD2&
		HiTag()
		{ return v[1];}

	DWORD
		GetValueAt(DWORD ix) const
		{ return *( ((DWORD*)v) + ix );	}

	void
		SetValueAt(DWORD ix, DWORD nv)
		{ *( ((DWORD*)v) + ix ) =nv; }


} __SONORK_PACKED;


#define SONORK_FLAG_LIST_SELECTOR    0x00000007
#define SONORK_FLAG_LIST_FLAGS       0xfffffff0
enum SONORK_LO_FLAGS
{
  SONORK_LO_FLAG_01		= 0x00000010
, SONORK_LO_FLAG_02		= 0x00000020
, SONORK_LO_FLAG_03		= 0x00000040
, SONORK_LO_FLAG_04		= 0x00000080
, SONORK_LO_FLAG_05		= 0x00000100
, SONORK_LO_FLAG_06		= 0x00000200
, SONORK_LO_FLAG_07		= 0x00000400
, SONORK_LO_FLAG_08		= 0x00000800
, SONORK_LO_FLAG_09		= 0x00001000
, SONORK_LO_FLAG_10		= 0x00002000
, SONORK_LO_FLAG_11		= 0x00004000
, SONORK_LO_FLAG_12		= 0x00008000
, SONORK_LO_FLAG_13		= 0x00010000
, SONORK_LO_FLAG_14		= 0x00020000
, SONORK_LO_FLAG_15		= 0x00040000
, SONORK_LO_FLAG_16		= 0x00080000
, SONORK_LO_FLAG_17		= 0x00100000
, SONORK_LO_FLAG_18		= 0x00200000
, SONORK_LO_FLAG_19		= 0x00400000
, SONORK_LO_FLAG_20		= 0x00800000
};
enum SONORK_HI_FLAGS
{
  SONORK_HI_FLAG_01		= 0x00000011
, SONORK_HI_FLAG_02		= 0x00000021
, SONORK_HI_FLAG_03		= 0x00000041
, SONORK_HI_FLAG_04		= 0x00000081
, SONORK_HI_FLAG_05		= 0x00000101
, SONORK_HI_FLAG_06		= 0x00000201
, SONORK_HI_FLAG_07		= 0x00000401
, SONORK_HI_FLAG_08		= 0x00000801
, SONORK_HI_FLAG_09		= 0x00001001
, SONORK_HI_FLAG_10		= 0x00002001
, SONORK_HI_FLAG_11		= 0x00004001
, SONORK_HI_FLAG_12		= 0x00008001
, SONORK_HI_FLAG_13		= 0x00010001
, SONORK_HI_FLAG_14		= 0x00020001
, SONORK_HI_FLAG_15		= 0x00040001
, SONORK_HI_FLAG_16		= 0x00080001
, SONORK_HI_FLAG_17		= 0x00100001
, SONORK_HI_FLAG_18		= 0x00200001
, SONORK_HI_FLAG_19		= 0x00400001
, SONORK_HI_FLAG_20		= 0x00800001
};

struct  TSonorkFlags
:public SONORK_DWORD2
{
public:
	void ClearAll(){v[0]=v[1]=0;}

	BOOL
		Test(UINT F) const
		{ return (v[F&SONORK_FLAG_LIST_SELECTOR]&(F&SONORK_FLAG_LIST_FLAGS)); }

	void
		Clear(UINT F)
		{ v[F&SONORK_FLAG_LIST_SELECTOR]&=~(F&SONORK_FLAG_LIST_FLAGS);	}

	void
		Set(UINT F)
		{ v[F&SONORK_FLAG_LIST_SELECTOR]|=(F&SONORK_FLAG_LIST_FLAGS); }

	BOOL
		Toggle(UINT f);

	void
		SetClear(UINT f,BOOL set);
};

struct TSonorkBitFlags
{
	DWORD*	ptr;
	DWORD	dwords;
public:
	TSonorkBitFlags(DWORD pmax_value);
	~TSonorkBitFlags();

	BOOL
		Test(UINT value) const;

	void
		Clear(UINT value);

	void
		Set(UINT value);

	void
		ClearAll();
};



#define SONORK_SID_FM0_MODE		0x0000001f
#define SONORK_SID_FM0_PRIVATE_MASK	0x00ffff00
#define SONORK_SID_FM1_USER_CONTROLLED	0x00ffffff
#define IS_VALID_SID_MODE(m) \
	((m)>=SONORK_SID_MODE_ONLINE && (m)<SONORK_SID_MODES)

enum SONORK_SID_FLAG0
{
  SONORK_SID_F0_PRIVATE_1		=0x00000100
, SONORK_SID_F0_PRIVATE_2		=0x00000200
, SONORK_SID_F0_PRIVATE_3		=0x00000400
, SONORK_SID_F0_PRIVATE_4		=0x00000800
, SONORK_SID_F0_PRIVATE_5		=0x00001000
, SONORK_SID_F0_PRIVATE_6		=0x00002000
, SONORK_SID_F0_PRIVATE_7		=0x00004000
, SONORK_SID_F0_PRIVATE_8		=0x00008000
, SONORK_SID_F0_PRIVATE_9		=0x00010000
, SONORK_SID_F0_PRIVATE_10		=0x00020000
, SONORK_SID_F0_PRIVATE_11		=0x00040000
, SONORK_SID_F0_PRIVATE_12		=0x00080000
, SONORK_SID_F0_PRIVATE_13		=0x00100000
, SONORK_SID_F0_PRIVATE_14		=0x00200000
, SONORK_SID_F0_PRIVATE_15		=0x00400000
, SONORK_SID_F0_PRIVATE_16		=0x00800000
};

enum SONORK_SID_FLAG1
{
   SONORK_SID_F1_SONORK_UTS_CLIENT	=0x00001000
 , SONORK_SID_F1_SONORK_UTS_SERVER	=0x00002000
 , SONORK_SID_F1_USING_SOCKS		=0x00004000
 , SONORK_SID_F1_TRACKER_ENABLED	=0x00010000
 , SONORK_SID_F1_PRIVATE		=0x00100000
 , SONORK_SID_F1_HIDE_PUBLIC		=0x00200000
 , SONORK_SID_F1_SERVER			=0x10000000
 , SONORK_SID_F1_ROBOT			=0x20000000
 , SONORK_SID_F1_ADMINISTRATOR		=0x40000000
};


enum SONORK_SID_MODE
{
  SONORK_SID_MODE_DISCONNECTED		= 0
, SONORK_SID_MODE_ONLINE		= 1
, SONORK_SID_MODE_BUSY			= 2
, SONORK_SID_MODE_AT_WORK		= 3
, SONORK_SID_MODE_FRIENDLY		= 4
, SONORK_SID_MODE_AWAY			= 5
, SONORK_SID_MODE_AWAY_AUTO		= 6
, SONORK_SID_MODE_INVISIBLE		= 7
, SONORK_SID_MODE_ONLINE_02		= 8
, SONORK_SID_MODE_ONLINE_03		= 9
, SONORK_SID_MODE_BUSY_02		= 10
, SONORK_SID_MODE_BUSY_03		= 11
, SONORK_SID_MODE_AT_WORK_02		= 12
, SONORK_SID_MODE_AT_WORK_03		= 13
, SONORK_SID_MODE_FRIENDLY_02		= 14
, SONORK_SID_MODE_FRIENDLY_03		= 15
, SONORK_SID_MODE_AWAY_HOLD		= 16
, SONORK_SID_MODE_AWAY_PHONE		= 17
, SONORK_SID_MODE_AWAY_02		= 18
, SONORK_SID_MODE_AWAY_03		= 19
, SONORK_SID_MODE_INVISIBLE_02		= 20
, SONORK_SID_MODE_INVISIBLE_03		= 21
, SONORK_SID_MODES
};

#define SONORK_AUTH_FM0_INFO_LEVEL		0x0000001f
#define SONORK_AUTH_FM0_PRIVATE_MASK		0x00ffff00
#define SONORK_AUTH_FM1_USER_CONTROLLED		0x00ffffff  // Flags controlled by the user

enum SONORK_AUTH_FLAG0
{
  SONORK_AUTH_F0_INFO_LEVEL_0		=0x00000000
, SONORK_AUTH_F0_INFO_LEVEL_1		=0x00000001
, SONORK_AUTH_F0_INFO_LEVEL_2		=0x00000002
, SONORK_AUTH_F0_INFO_LEVEL_3		=0x00000003
, SONORK_AUTH_F0_PRIVATE_1		=0x00000100
, SONORK_AUTH_F0_PRIVATE_2		=0x00000200
, SONORK_AUTH_F0_PRIVATE_3		=0x00000400
, SONORK_AUTH_F0_PRIVATE_4		=0x00000800
, SONORK_AUTH_F0_PRIVATE_5		=0x00001000
, SONORK_AUTH_F0_PRIVATE_6		=0x00002000
, SONORK_AUTH_F0_PRIVATE_7		=0x00004000
, SONORK_AUTH_F0_PRIVATE_8		=0x00008000
, SONORK_AUTH_F0_PRIVATE_9		=0x00010000
, SONORK_AUTH_F0_PRIVATE_10		=0x00020000
, SONORK_AUTH_F0_PRIVATE_11		=0x00040000
, SONORK_AUTH_F0_PRIVATE_12		=0x00080000
, SONORK_AUTH_F0_PRIVATE_13		=0x00100000
, SONORK_AUTH_F0_PRIVATE_14		=0x00200000
, SONORK_AUTH_F0_PRIVATE_15		=0x00400000
, SONORK_AUTH_F0_PRIVATE_16		=0x00800000
, SONORK_AUTH_F0_PIN_REQUIRED_OBSOLETE	=0x10000000
, SONORK_AUTH_F0_HIDE_PUBLIC		=0x20000000
};

enum SONORK_AUTH_FLAG1
{
  SONORK_AUTH_F1_HIDE_EMAIL		=0x00000100
, SONORK_AUTH_F1_HIDE_ADDR		=0x00000200
, SONORK_AUTH_F1_BUSY        		=0x00001000	// Show as Busy (don't disturb)
, SONORK_AUTH_F1_NOT_BUSY    		=0x00002000	// Don't show as busy (when mode is busy)
, SONORK_AUTH_F1_FRIENDLY     		=0x00004000	// Show as friendly
, SONORK_AUTH_F1_NOT_FRIENDLY		=0x00008000	// Don't show as friendly (when mode is friendly)
, SONORK_AUTH_F1_AWAY          		=0x00010000	// Show as away (N/A)
, SONORK_AUTH_F1_NOT_AWAY      		=0x00020000	// Don't show as away (when mode is away)
, SONORK_AUTH_F1_DISCONNECTED  		=0x00100000	// Always show as disconnected
};

// SONORK_AUTH_F0_PIN_REQUIRED: (OBSOLETE)
// It indicates that users that are not in our user list
// can access our user data as long as they have the correct PIN;
// is is ignored when the requestor is already in our list.
// This flag is only valid for the <priv> and <pub> flags
// of the user information, it is ignored in the user list

// SONORK_AUTH_F0_HIDE_PUBLIC:
// Indicates that our data should not be listed in public lists
// This flag is only valid for the <priv> and <pub> flags
// of the user information, it is ignored in the user list

// If Neither PIN_REQUIRED or DENY_PUBLIC are set,
// any user can access our information.
enum SONORK_PIN_TYPE
{
  SONORK_PIN_TYPE_NONE
, SONORK_PIN_TYPE_64
};


enum SONORK_SEX
{
    SONORK_SEX_NA
,   SONORK_SEX_M
,   SONORK_SEX_F
};

// ----------------------------------------------------------------------------
enum SONORK_UINFO_FLAG0
{
   SONORK_UINFO_F0_SEX			=0x00000007
,  SONORK_UINFO_F0_RESERVED_1		=0x00000010
,  SONORK_UINFO_F0_RESERVED_2		=0x00000020
};

enum SONORK_UINFO_FLAG1
{
   SONORK_UINFO_F1_BROADCASTER		=0x10000000
 , SONORK_UINFO_F1_ROBOT		=0x20000000
 , SONORK_UINFO_F1_ADMINISTRATOR	=0x40000000
};

enum SONORK_USER_LOGIN_FLAGS
{
  SONORK_ULF_REFRESH_USER_LIST		=0x00000010
, SONORK_ULF_DISABLE_UTS		=0x00000020
, SONORK_ULF_DISABLED			=0x10000000
};
enum SONORK_LOGIN_REQ_FLAGS
{
	// SONORK_LOGIN_CF_GUEST:
	// A pseudo login: We can invoke only a very limited subset of functions
	// and we won't get a SESSION information data (i.e: We can't select
	// a user mode and hence we effectively don't exist for other users)
	// It is used to create a new user or to search for other users.

	// SONORK_LOGIN_CF_NO_USER_DATA_UPDATE:
	// The server always updates some fields of our user data uppon login
	// (like the region & language) unless the SONORK_LOGIN_CF_NO_USER_DATA_UPDATE
	// is set.

	// SONORK_LOGIN_CF_INTERACTIVE
	// The login is being executed because of an user request, this flag
	// should be cleared when a non-interactive login is being executed.
	// (for example: Automatic re-connection after the link was lost)
  SONORK_LOGIN_RF_GUEST			=0x00000010
, SONORK_LOGIN_RF_INTERACTIVE		=0x00000020
, SONORK_LOGIN_RF_ROBOT			=0x00000100
, SONORK_LOGIN_RF_ADMINISTRATOR		=0x00000200
, SONORK_LOGIN_RF_NO_USER_DATA_UPDATE	=0x00001000
, SONORK_LOGIN_RF_INTERNET_MODE		=0x00000000
, SONORK_LOGIN_RF_INTRANET_MODE		=0x00000001
, SONORK_LOGIN_RF_MONITOR_MODE		=0x00000002
, SONORK_LOGIN_RF_MODE_MASK		=0x0000000f
};
enum SONORK_LOGIN_MODE
{
  SONORK_LOGIN_MODE_INTERNET		=SONORK_LOGIN_RF_INTERNET_MODE
, SONORK_LOGIN_MODE_INTRANET		=SONORK_LOGIN_RF_INTRANET_MODE
, SONORK_LOGIN_MODE_MONITOR		=SONORK_LOGIN_RF_MONITOR_MODE
};

//  for the auth flags in the user list

#if SONORK_SID_FM0_PRIVATE_MASK != SONORK_AUTH_FM0_PRIVATE_MASK
#	error SONORK_SID_FM0_PRIVATE_MASK != SONORK_AUTH_FM0_PRIVATE_MASK
#endif

struct TSonorkId
:public SONORK_DWORD2
{
	bool
		IsSystemUser() const
		{ return v[1]==1; }
};


typedef SONORK_DWORD2 TSonorkObjId;
typedef SONORK_DWORD2 TSonorkPin;
typedef SONORK_DWORD2 TSonorkSerial;


struct  TSonorkAuthFlags
:public SONORK_DWORD2
{

	DWORD&
		Flags0()
		{ return v[0];	}

	DWORD&
		Flags1()
		{ return v[1];	}

	void
		Set(const SONORK_DWORD2&f )
		{ SONORK_DWORD2::Set( f ); }

	void
		Clear( void )
		{ SONORK_DWORD2::Clear(); }

	void
		Set(SONORK_AUTH_FLAG0 f)
		{ v[0]|=f;}

	void
		Clear(SONORK_AUTH_FLAG0 f)
		{ v[0]&=~f;}

	BOOL
		Test(SONORK_AUTH_FLAG0 f)	const
		{ return v[0]&f; }

	void
		Toggle(SONORK_AUTH_FLAG0 f)
		{ v[0]^=f; }
	void
		SetClear(SONORK_AUTH_FLAG0 f,BOOL s)
		{ s?v[0]|=f:v[0]&=~f;}

	void
		Set(SONORK_AUTH_FLAG1 f)
		{ v[1]|=f;}

	void
		Clear(SONORK_AUTH_FLAG1 f)
		{ v[1]&=~f;}

	BOOL
		Test(SONORK_AUTH_FLAG1 f)	const
		{ return v[1]&f; }

	void
		Toggle(SONORK_AUTH_FLAG1 f)
		{ v[1]^=f; }

	void
		SetClear(SONORK_AUTH_FLAG1 f,BOOL s)
		{ s?v[1]|=f:v[1]&=~f;}

	SONORK_USER_INFO_LEVEL
		UserInfoLevel() const
		{ return (SONORK_USER_INFO_LEVEL)(v[0]&SONORK_AUTH_FM0_INFO_LEVEL);}

	void
		SetUserInfoLevel(SONORK_USER_INFO_LEVEL level)
		{ v[0]=(v[0]&~SONORK_AUTH_FM0_INFO_LEVEL)|(level&SONORK_AUTH_FM0_INFO_LEVEL); }

	DWORD
		PrivateMask() const
		{ return v[0]&SONORK_AUTH_FM0_PRIVATE_MASK; }

	void
		SetPrivateMask(DWORD mask)
		{ v[0]=(v[0]&~SONORK_AUTH_FM0_PRIVATE_MASK)|(mask&SONORK_AUTH_FM0_PRIVATE_MASK); }

	BOOL
		HideEmail() const
		{ return Test(SONORK_AUTH_F1_HIDE_EMAIL);}

	BOOL
		HideAddr() const
		{ return Test(SONORK_AUTH_F1_HIDE_ADDR);}

	BOOL
		ShowDisconnected() const
		{ return Test(SONORK_AUTH_F1_DISCONNECTED);}

	BOOL
		ShowBusy() const
		{ return Test(SONORK_AUTH_F1_BUSY);		}

	BOOL
		ShowNotBusy() const
		{ return Test(SONORK_AUTH_F1_NOT_BUSY);	}

	BOOL
		ShowFriendly() const
		{ return Test(SONORK_AUTH_F1_FRIENDLY);	}

	BOOL
		ShowNotFriendly() const
		{ return Test(SONORK_AUTH_F1_NOT_FRIENDLY);}

	BOOL
		ShowAway() const
		{ return Test(SONORK_AUTH_F1_AWAY);		}

	BOOL
		ShowNotAway() const
		{ return Test(SONORK_AUTH_F1_NOT_AWAY);	}

	BOOL
		IsPublic() const
		{ return !Test(SONORK_AUTH_F0_HIDE_PUBLIC);}

	void
		SetPublic()
		{ Clear(SONORK_AUTH_F0_HIDE_PUBLIC);	}

	void
		ClearPublic()
		{ Set(SONORK_AUTH_F0_HIDE_PUBLIC);		}


	void
		CopyUserControlled(TSonorkAuthFlags& O)
		{
		 v[0]=O.v[0];
		 v[1]=(v[1]&~SONORK_AUTH_FM1_USER_CONTROLLED)|(O.v[1]&SONORK_AUTH_FM1_USER_CONTROLLED);
		}

	void
		ClearUserControlled()
		{ v[1]&=~SONORK_AUTH_FM1_USER_CONTROLLED; }

	void
		ClearServerControlled()
		{ v[1]&=SONORK_AUTH_FM1_USER_CONTROLLED; }


}__SONORK_PACKED;

struct  TSonorkSidFlags
:public SONORK_DWORD4
{
	DWORD&
		Flags0()
		{ return v[0].v[0];	}

	DWORD&
		Flags1()
		{ return v[0].v[1];	}

	void
		Set(const SONORK_DWORD4& f )
		{ SONORK_DWORD4::Set( f ); }

	void
		Clear( void )
		{ SONORK_DWORD4::Clear(); }

	void
		Set(SONORK_SID_FLAG0 f)
		{ v[0].v[0]|=f;}

	void
		Clear(SONORK_SID_FLAG0 f)
		{ v[0].v[0]&=~f;}

	BOOL
		Test(SONORK_SID_FLAG0 f) const
		{ return v[0].v[0]&f; }

	void
		Toggle(SONORK_AUTH_FLAG0 f)
		{ v[0].v[0]^=f; }

	void
		Set(SONORK_SID_FLAG1 f)
		{ v[0].v[1]|=f;}

	void
		Clear(SONORK_SID_FLAG1 f)
		{ v[0].v[1]&=~f;}

	BOOL
		Test(SONORK_SID_FLAG1 f) const
		{ return v[0].v[1]&f; }

	void
		Toggle(SONORK_SID_FLAG1 f)
		{ v[0].v[1]^=f; }

	SONORK_SID_MODE
		SidMode() const
		{ return (SONORK_SID_MODE)(v[0].v[0]&SONORK_SID_FM0_MODE); }

	void
		SetSidMode(SONORK_SID_MODE mode)
		{ v[0].v[0]=(v[0].v[0]&~SONORK_SID_FM0_MODE)|(mode&SONORK_SID_FM0_MODE); }

	DWORD
		PrivateMask() const
		{ return v[0].v[0]&SONORK_SID_FM0_PRIVATE_MASK;	}

	void
		SetPrivateMask(DWORD mask)
		{ v[0].v[0]=(v[0].v[0]&~SONORK_SID_FM0_PRIVATE_MASK)|(mask&SONORK_SID_FM0_PRIVATE_MASK); }

	void
		SetPrivate()
		{ Set(SONORK_SID_F1_PRIVATE); }

	void
		ClearPrivate()
		{ Clear(SONORK_SID_F1_PRIVATE); }

	BOOL
		IsPublic() const
		{ return !Test(SONORK_SID_F1_HIDE_PUBLIC);	}

	void
		SetPublic()
		{ Clear(SONORK_SID_F1_HIDE_PUBLIC);		}

	void
		ClearPublic()
		{ Set(SONORK_SID_F1_HIDE_PUBLIC);		}

	void
		SetUsingSocks()
		{ Set(SONORK_SID_F1_USING_SOCKS);		}

	void
		ClearUsingSocks()
		{ Clear(SONORK_SID_F1_USING_SOCKS);		}

	void
		EnableUtsServer()
		{ Set(SONORK_SID_F1_SONORK_UTS_SERVER);	}

	void
		EnableUtsClient()
		{ Set(SONORK_SID_F1_SONORK_UTS_CLIENT);	}

	void
		DisableUtsServer()
		{ Clear(SONORK_SID_F1_SONORK_UTS_SERVER);	}

	void
		DisableUtsClient()
		{ Clear(SONORK_SID_F1_SONORK_UTS_CLIENT);	}

	void
		DisableUts()
		{ DisableUtsServer();DisableUtsClient();	}

	void
		EnableTracker()
		{ Set(SONORK_SID_F1_TRACKER_ENABLED);	}

	void
		DisableTracker()
		{ Clear(SONORK_SID_F1_TRACKER_ENABLED);	}

	BOOL
		IsPrivate() const
		{ return Test(SONORK_SID_F1_PRIVATE);	}

	BOOL
		UsingSocks() const
		{ return Test(SONORK_SID_F1_USING_SOCKS);	}

	BOOL
		UtsServerEnabled() const
		{ return Test(SONORK_SID_F1_SONORK_UTS_SERVER); }

	BOOL
		UtsClientEnabled() const
		{ return Test(SONORK_SID_F1_SONORK_UTS_CLIENT); }

	BOOL
		UtsEnabled() const
		{ return UtsClientEnabled(); }

	BOOL
		TrackerEnabled() const
		{ return Test(SONORK_SID_F1_TRACKER_ENABLED);}

	BOOL
		IsRobot() const
		{ return Test(SONORK_SID_F1_ROBOT);	}

	BOOL
		IsHuman() const
		{ return !IsRobot();}
		
	BOOL
		IsAdministrator() const
		{ return Test(SONORK_SID_F1_ADMINISTRATOR);	}

	BOOL
		IsOnline() const
		{ return SidMode()!=SONORK_SID_MODE_DISCONNECTED;}

	BOOL
		IsOffline() const
		{ return SidMode()==SONORK_SID_MODE_DISCONNECTED;}

	void
		xCopyUserControlled(TSonorkSidFlags&);

	void
		xClearServerControlled();

	void
		xClearUserControlled();

}__SONORK_PACKED;

enum SONORK_SID_VERSION_FLAGS
{
  SONORK_SID_VF_SUPPORTS_EXT_SID	= 0x00010000
, SONORK_SID_VF_SUPPORTS_UTS		= 0x00020000
, SONORK_SID_VF_SUPPORTS_EXT_MSG	= 0x00040000
};
struct  TSonorkVersion
:public SONORK_DWORD4
{
	enum V_INDEX
	{
		V_VERSION		= 0
	,	V_FLAGS			= 1
	,	V_OS			= 2
	,	V_RESERVED		= 3
	};

	DWORD
		VersionNumber() const;

	void
		SetVersionNumber( DWORD v );

	DWORD
		OldVersionNumber() const
		{ return VersionNumber()>>4; }

	SONORK_OS_TYPE
		OsType() const;

	DWORD
		OsVersion() const;

	DWORD
		Flags() const
		{	return GetValueAt(V_FLAGS);}
	BOOL
		SupportsExtendedSidData() const
		{ return Flags()&SONORK_SID_VF_SUPPORTS_EXT_SID;}

	BOOL
		SupportsUTS() const
		{ return Flags()&SONORK_SID_VF_SUPPORTS_UTS;}

	void
		Load(	  DWORD			app_version
			, DWORD			flags
			, SONORK_OS_TYPE        os_type
			, DWORD 		os_version);

	SONORK_C_STR
		GetStr(SONORK_C_STR) const;

	BOOL
		UsesOldCtrlMsg() const
		{ return VersionNumber()<MAKE_VERSION_NUMBER(1,5,0,7);}
		
}__SONORK_PACKED;

struct  TSonorkTag
:public SONORK_DWORD2
{
	enum V_INDEX
	{
		V_FLAGS			= 0
	,	V_VALUE			= 1
	};
	DWORD
		Flags()	const
		{ return v[V_FLAGS];}

	DWORD
		Value()	const
		{ return v[V_VALUE];}

}__SONORK_PACKED;

struct  TSonorkTag2	// double tag
:public SONORK_DWORD4
{
	DWORD&
		lFlags()
		{ return v[0].v[0]; }

	DWORD&
		lValue()
		{ return v[0].v[1]; }

	DWORD&
		hFlags()
		{ return v[1].v[0]; }

	DWORD&
		hValue()
		{ return v[1].v[1]; }

}__SONORK_PACKED;



struct  TSonorkTask
:public SONORK_DWORD2
{
	enum V_INDEX
	{
		V_SERVER		= 0
	,	V_CLIENT		= 1
	};

	DWORD	Server()	const	{ return v[V_SERVER];	}
	DWORD 	Client()	const	{ return v[V_CLIENT];	}

}__SONORK_PACKED;



struct  TSonorkSid
:public SONORK_DWORD2
{
	enum V_INDEX
	{
		V_SERVER_NO	= 0
	,	V_SESSION_ID	= 1
	};

	SONORK_SID_ID
		ServerNo() const
		{ return v[V_SERVER_NO];}

	SONORK_SID_ID
		SessionId() const
		{ return v[V_SESSION_ID];}
		
}__SONORK_PACKED;




struct  TSonorkUserInfoFlags
:public SONORK_DWORD2
{
	void
		Set(const SONORK_DWORD2& f)
		{ SONORK_DWORD2::Set(f);}

	void
		Clear( void )
		{ SONORK_DWORD2::Clear();}

	void
		Set(SONORK_UINFO_FLAG0 f)
		{ v[0]|=f;}

	void
		Clear(SONORK_UINFO_FLAG0 f)
		{ v[0]&=~f;}

	BOOL
		Test(SONORK_UINFO_FLAG0 f)	const
		{ return v[0]&f; }

	void
		Toggle(SONORK_AUTH_FLAG0 f)
		{ v[0]^=f; }

	void
		Set(SONORK_UINFO_FLAG1 f)
		{ v[1]|=f;}

	void
		Clear(SONORK_UINFO_FLAG1 f)
		{ v[1]&=~f;}

	BOOL
		Test(SONORK_UINFO_FLAG1 f)	const
		{ return v[1]&f; }

	void
		Toggle(SONORK_UINFO_FLAG1 f)
		{ v[1]^=f; }

	BOOL
		IsAdministrator() const
		{ return Test(SONORK_UINFO_F1_ADMINISTRATOR); }

	BOOL
		IsRobot() const
		{ return Test(SONORK_UINFO_F1_ROBOT);	}

	BOOL
		IsBroadcaster() const
		{ return Test(SONORK_UINFO_F1_BROADCASTER); }


	SONORK_SEX
		GetSex() const
		{ return (SONORK_SEX)(v[0]&SONORK_UINFO_F0_SEX); }

	void
		SetSex(SONORK_SEX s)
		{ v[0]= (v[0]&~SONORK_UINFO_F0_SEX) | (s&SONORK_UINFO_F0_SEX); }

};
/*
// OLD WAY
#define SONORK_REGION_F0_TZ_VALUE		0x000f0000
#define SONORK_REGION_F0_TZ_SIGN		0x00100000
#define SONORK_REGION_F0_TZ			(SONORK_REGION_F0_TZ_VALUE|SONORK_REGION_F0_TZ_SIGN)
*/

#define SONORK_REGION_F0_LANGUAGE		0x0000ffff
#define SONORK_REGION_TZ_DIVISOR		4
#define SONORK_REGION_TZ_DIVISOR_MINS		15
#define SONORK_REGION_TZ_MAX_VALUE		(SONORK_REGION_TZ_DIVISOR*(12))
#define SONORK_REGION_TZ_MIN_VALUE		(SONORK_REGION_TZ_DIVISOR*(-12))
#define SONORK_REGION_F0_TZ_VALUE		0x3f000000
#define SONORK_REGION_F0_TZ_NEGATIVE		0x40000000
#define SONORK_REGION_F0_TZ			(SONORK_REGION_F0_TZ_VALUE|SONORK_REGION_F0_TZ_NEGATIVE)
#define SONORK_REGION_F1_COUNTRY		0x0000ffff

struct  TSonorkRegion
:public SONORK_DWORD2
{

	DWORD
		GetLanguage() const
		{ return (DWORD)(v[0]&SONORK_REGION_F0_LANGUAGE); }

	void
		SetLanguage(DWORD l)
		{ v[0]= (v[0]&~SONORK_REGION_F0_LANGUAGE) | (l&SONORK_REGION_F0_LANGUAGE); }

	DWORD
		GetCountry() const
		{ return (DWORD)(v[1]&SONORK_REGION_F1_COUNTRY); }

	void
		SetCountry(DWORD c)
		{ v[1]=(v[1]&~SONORK_REGION_F1_COUNTRY) | (c&SONORK_REGION_F1_COUNTRY); }


	// TZ: Ranges from -12*SONORK_REGION_TZ_DIVISOR to 12*SONORK_REGION_TZ_DIVISOR

	int
		GetTimeZone() const;

	void
		SetTimeZone(int n);

	char*
		GetTimeZoneStr(char*) const;
};


#define SONORK_TIME_TYPE_E1		0
enum SONORK_TIME_RELATION
{
  SONORK_TIME_BEFORE	=-1
, SONORK_TIME_EQUAL	=0
, SONORK_TIME_AFTER	=1
};

struct  TSonorkTime
:public SONORK_DWORD2
{
	enum V_INDEX
	{
		V_TIME		= 0
	,	V_DATE		= 1
	};
	void    SetTime_GMT();
	void    SetTime_Local();
	void	ToLocal();

	// IMPORTANT:
	// do not use inherited operators <,>,<=,>= to
	// check if one date/time is after another:
	//  they won't work correctly, instead use
	//  IsAfter(<TSonorkTime>) IsBefore(<TSonorkTime>), IsSameDate(<TSonorkTime>)
	// SONORK_DWORD2::operator == will work, but the dates must be exactly
	//   the same, including seconds.
	// Additionally, DateValue() and DateTime() may be used with normal
	// logical operators (> <) to relate two TSonorkTimes

	DWORD	DateValue() 	const;
	DWORD	TimeValue() 	const;	// Does not include seconds
	DWORD	TimeValueSecs() const;	// Includes seconds

	SONORK_TIME_RELATION
		RelDate(const TSonorkTime&T)	const ;	 // Compares Date (No time)

	SONORK_TIME_RELATION
		RelTime(const TSonorkTime&T)	const ;  // Compares Time (No Secs,No date)

	SONORK_TIME_RELATION
		RelDateTime(const TSonorkTime&T)const ;  // Comapres Date & Time (No Secs)


	BOOL
		SetTime(const struct tm*);

	BOOL
		GetTime(struct tm*) const;

	BOOL
		GetDate(SONORK_C_STR) const;	// yyyy/mm/dd or yyyy.mm.dd

	BOOL
		SetDate(SONORK_C_CSTR);		// yyyy/mm/dd or yyyy.mm.dd

	BOOL
		DiffTime(const TSonorkTime&,double*) const ;

#if defined(SONORK_WIN32_BUILD)

	// FILETIME: Implemented in WIN32 version only.
	//  Many Win32 API functions take a FILETIME as arguments

	BOOL
		SetTime(const FILETIME*);

	BOOL
		GetTime(FILETIME*) const ;

	BOOL
		SetTime(const SYSTEMTIME*);

	BOOL
		GetTime(SYSTEMTIME*) const ;
#endif

	BOOL
		SetDate(UINT year, UINT month, UINT day);

	void
		ClearDate();

	void
		ClearTime();


	UINT
		Year()	const;	// 1900...

	UINT
		Month()	const;	// 1-12

	UINT
		Day()	const;	// 1-31

	BOOL
		IsValid() const;
		
	UINT
		Hour()	const;	// 0~23

	UINT
		Minutes() const; // 0-59

	UINT
		Seconds() const; // 0-59
}__SONORK_PACKED;



// TYPE OF ADDRESS
// TSonorkPhysAddr.type
enum SONORK_PHYS_ADDR_TYPE
{
  SONORK_PHYS_ADDR_ANY	=-1
, SONORK_PHYS_ADDR_NONE
, SONORK_PHYS_ADDR_TCP_1
, SONORK_PHYS_ADDR_UDP_1
, SONORK_PHYS_ADDR_TYPES
};

struct  TSonorkPhysAddr
{
	struct _HEADER{
		BYTE		type;
		BYTE		reserved;
	}__SONORK_PACKED;

	struct _INET1  {
			sockaddr_in	addr;
	}__SONORK_PACKED;

	union _DATA{
		_INET1	inet1;
		BYTE 	raw[10];
	}__SONORK_PACKED;

	_HEADER		header;
	_DATA		data;

	SONORK_PHYS_ADDR_TYPE
		Type() const
		{ return (SONORK_PHYS_ADDR_TYPE)header.type;	}

	void
		SetStr(SONORK_C_CSTR );

	SONORK_C_STR
		GetStr(SONORK_C_STR ) const;

	void
		SetType(SONORK_PHYS_ADDR_TYPE t, UINT family=AF_INET);

	// SetInet1 return 0 if Ok, O/S error code if failed
	sockaddr_in*
		Inet1()
		{ return &data.inet1.addr; }

	bool
		IsZero() const
		{ return header.type == SONORK_PHYS_ADDR_NONE; }

	bool
		IsValid(SONORK_PHYS_ADDR_TYPE t)	const
		{
			return  data.inet1.addr.sin_port!=0
				&& ( header.type == t || t == SONORK_PHYS_ADDR_ANY)
				&&  data.inet1.addr.sin_addr.s_addr!=0;
		}

	WORD
		GetInet1Port() const
		{ return ntohs(data.inet1.addr.sin_port); }
	void
		SetInet1Port(WORD port)
		{ data.inet1.addr.sin_port=htons(port);	}

	void
		SetInet1Addr(unsigned long addr)
		{ data.inet1.addr.sin_addr.s_addr=addr;	}

	unsigned long
		GetInet1Addr() const
		{ return data.inet1.addr.sin_addr.s_addr; }

	int
		SetInet1(SONORK_PHYS_ADDR_TYPE,const sockaddr_in*);

	int
		SetInet1(SONORK_PHYS_ADDR_TYPE,SONORK_C_CSTR host,WORD port);

	int
		SetInet1(SONORK_PHYS_ADDR_TYPE,unsigned long addr,WORD port);

	SONORK_PHYS_ADDR_TYPE
		GetInet1(sockaddr_in*);

	bool operator==(const TSonorkPhysAddr&O);
	bool operator!=(const TSonorkPhysAddr&O){ return !(*this==O);}

	bool
		IsEqual(sockaddr_in*) const;
	void
		Set(const TSonorkPhysAddr&O)
		{ memcpy(this , &O , sizeof(*this) );	}

	void
		Clear()
		{ memset(this,0,sizeof(*this));	}


}__SONORK_PACKED;


struct  TSonorkHostInfo
{
	TSonorkPhysAddr		physAddr;
	DWORD			version;
	DWORD			reserved;

	bool
		Enabled() const
		{ return version!=0; }

	void
		Disable()
		{ version=0; }

	void
		Set(const TSonorkHostInfo&O)
		{ memcpy(this,&O,sizeof(*this));}

	void
		Clear()
		{ memset(this,0,sizeof(*this));	}


} __SONORK_PACKED;

struct  TSonorkUserLocus1
{
	TSonorkId 	userId;
	TSonorkSid	sid;


	void
		Set(const TSonorkId&puid, const TSonorkSid&psid)
		{
			userId.Set(puid);
			sid.Set(psid);
		}
	void
		Set(const TSonorkUserLocus1& O)
		{ memcpy(this,&O,sizeof(*this)); }

	void
		Clear()
		{ memset(this,0,sizeof(*this));	}



}__SONORK_PACKED;



struct  TSonorkUserLocus2
:public TSonorkUserLocus1
{
	TSonorkSidFlags		sidFlags;
	TSonorkPhysAddr		physAddr;

	void
		Set( const TSonorkId& pUserId , const TSonorkSid& pSid ,const TSonorkPhysAddr& pPhysAddr,const TSonorkSidFlags& pSidFlags);

	void
		Set(const TSonorkUserLocus2 &O)
		{ memcpy(this,&O,sizeof(*this)); }

	void
		Clear()
		{ memset(this,0,sizeof(*this));	}

	SONORK_SID_MODE
		SidMode() const
		{ return sidFlags.SidMode(); }

	BOOL
		UsingSocks() const
		{ return sidFlags.UsingSocks();}

	BOOL    UtsServerEnabled()	const {	return sidFlags.UtsServerEnabled();}
	BOOL    UtsClientEnabled()	const {	return sidFlags.UtsClientEnabled();}
	BOOL	UtsEnabled()		const {	return sidFlags.UtsEnabled();}
	BOOL	TrackerEnabled() 	const { return sidFlags.TrackerEnabled();}

	BOOL	IsPrivate()		const {	return sidFlags.IsPrivate();	}
	BOOL	IsRobot()		const {	return sidFlags.IsRobot();	}
	BOOL	IsHuman()		const {	return sidFlags.IsHuman();	}
	BOOL	IsAdministrator()	const {	return sidFlags.IsAdministrator();	}


}__SONORK_PACKED;

struct  TSonorkUserLocus3
:public TSonorkUserLocus2
{
	TSonorkRegion	 	region;
	DWORD		 	reserved[4];

	void
		Set(const TSonorkUserLocus3 &O)
		{ memcpy(this,&O,sizeof(*this)); }

	void 	Clear()
		{ memset(this,0,sizeof(*this));	}

}__SONORK_PACKED;

struct TSonorkUserHandle
:public TSonorkUserLocus1
{
	DWORD	utsLinkId;

	void
		Set(const TSonorkUserHandle&O)
		{ memcpy(this,&O,sizeof(*this)); }

	void
		Clear()
		{ memset(this,0,sizeof(*this));}

};

struct TSonorkServiceHandle
:public TSonorkUserHandle
{
	DWORD	instance;

	void
		Set(const TSonorkServiceHandle&O)
		{ memcpy(this,&O,sizeof(*this)); }

	void
		Clear()
		{ memset(this,0,sizeof(*this));}
};

struct TSonorkServiceSystem
:public TSonorkServiceHandle
{
	DWORD	serviceId;
	DWORD	systemId;

	void
		Set(const TSonorkServiceSystem&O)
		{ memcpy(this,&O,sizeof(*this)); }

	SONORK_SERVICE_ID
		ServiceId() const
		{ return (SONORK_SERVICE_ID)serviceId;}

	DWORD
		QueryId() const
		{ return systemId; }

	DWORD
		CircuitId() const
		{ return systemId; }

	// NB! Clears QueryId()/CircuitId()
	void
		LoadTarget(SONORK_SERVICE_ID p_service_id
			, DWORD p_instance)
		{
			serviceId=p_service_id;
			instance  =p_instance;
			systemId =0;
		}

	void
		Clear()
		{ memset(this,0,sizeof(*this));}
};

typedef TSonorkServiceSystem TSonorkServiceQuery;
typedef TSonorkServiceSystem TSonorkServiceCircuit;

struct  TSonorkOldUserAddr
{
	TSonorkSid	 	sid;
	TSonorkSidFlags		sidFlags;
	TSonorkPhysAddr		physAddr;

	void
		OldClear()
		{ memset(this,0,sizeof(*this));	}

	void
		OldSet(const TSonorkOldUserAddr &O)
		{ memcpy(this,&O,sizeof(TSonorkOldUserAddr)); }


}__SONORK_PACKED;

struct  TSonorkNewUserAddr
:public TSonorkOldUserAddr
{
	TSonorkVersion 	version;

	SONORK_SID_MODE
		SidMode()	const
		{ return sidFlags.SidMode(); }

	bool operator ==(TSonorkNewUserAddr&O)
	{
		return     sidFlags==O.sidFlags
				&& sid      ==O.sid
				&& physAddr==O.physAddr;
	}

	void
		Set(const TSonorkNewUserAddr &O)
		{
			memcpy(this,&O,sizeof(TSonorkNewUserAddr));
		}
	void
		SetOld(const TSonorkOldUserAddr &O)
		{
			memcpy(this,&O,sizeof(TSonorkOldUserAddr));
		}

	void
		Clear()
		{ memset(this,0,sizeof(*this));	}

}__SONORK_PACKED;

struct  TSonorkOldSidNotification
{
	TSonorkUserLocus3	locus;
	TSonorkSerial		userSerial;
};
struct  TSonorkAuth1
{
	DWORD      		tag;
	TSonorkAuthFlags	flags;

	void 	Set(const TSonorkAuth1&O)
	{
		memcpy(this,&O,sizeof(*this));
	}

	void Clear()
	{
		memset(this,0,sizeof(*this));
	}


	BOOL	HideEmail()		const {	return flags.HideEmail();}
	BOOL	HideAddr()		const {	return flags.HideAddr();}
	BOOL	ShowDisconnected()	const {	return flags.ShowDisconnected();}
	BOOL	ShowBusy()		const {	return flags.ShowBusy();		}
	BOOL	ShowNotBusy()		const {	return flags.ShowNotBusy();		}
	BOOL	ShowFriendly()		const {	return flags.ShowFriendly();	}
	BOOL	ShowNotFriendly()	const {	return flags.ShowNotFriendly();	}
	BOOL	ShowAway()		const {	return flags.ShowAway();		}
	BOOL	ShowNotAway()		const {	return flags.ShowNotAway();		}

	BOOL	IsPublic()		const {	return flags.IsPublic();	}
	void	SetPublic()		{	flags.SetPublic();	 	}
	void	ClearPublic()		{	flags.ClearPublic(); 	}

	DWORD
		GetGroupNo() const
		{ return tag&SONORK_AUTH_TAG_GROUP_MASK;}

	void
		SetGroupNo(DWORD group)
		{ tag=(tag&~SONORK_AUTH_TAG_GROUP_MASK)|(group&SONORK_AUTH_TAG_GROUP_MASK);}

	void
		SetFlag(SONORK_AUTH_FLAG0 f)
		{ flags.Set(f);		}

	void
		ClearFlag(SONORK_AUTH_FLAG0 f)
		{ flags.Clear(f);	}

	BOOL
		TestFlag(SONORK_AUTH_FLAG0 f)	const
		{ return flags.Test(f); }

	void
		SetClearFlag(SONORK_AUTH_FLAG0 f,BOOL s)
		{ flags.SetClear(f,s);}

	void
		SetFlag(SONORK_AUTH_FLAG1 f)
		{ flags.Set(f);	}

	void
		ClearFlag(SONORK_AUTH_FLAG1 f)
		{ flags.Clear(f); }

	BOOL	TestFlag(SONORK_AUTH_FLAG1 f)	const
	{
		return flags.Test(f);
	}

	void
		SetClearFlag(SONORK_AUTH_FLAG1 f,BOOL s)
		{ flags.SetClear(f,s); }

	SONORK_USER_INFO_LEVEL
		UserInfoLevel()	const
		{ return flags.UserInfoLevel();	}

	void
		SetUserInfoLevel(SONORK_USER_INFO_LEVEL level)
		{ flags.SetUserInfoLevel(level); }

	DWORD
		PrivateMask()			const
		{ return flags.PrivateMask(); }

	void
		SetPrivateMask(DWORD mask)
		{ flags.SetPrivateMask(mask); }

}__SONORK_PACKED;

struct  TSonorkAuth2
:public TSonorkAuth1
{
	DWORD	pin;

	void Set(const TSonorkAuth2&O)
	{
		memcpy(this,&O,sizeof(*this));
	}

	void Clear()
	{
		memset(this,0,sizeof(*this));
	}


}__SONORK_PACKED;

struct  TSonorkUserAuth
{
	DWORD		pin;
	TSonorkId	userId[2];
	TSonorkAuth1	auth[2];

	void
		Clear()
		{ memset(this,0,sizeof(*this)); }

	void
		Set(const TSonorkUserAuth& O)
		{ memcpy(this,&O,sizeof(*this));}

	bool
		IsZero() const
		{ return userId[0].IsZero() || userId[1].IsZero(); }
		 
	// Load returns index of userId_A;
	UINT
		Load(	const SONORK_DWORD2& userId_A, const SONORK_DWORD2& userId_B);

	UINT
		GetIndex(const SONORK_DWORD2& userId) const ;

	void
		SetAuth(UINT index,const TSonorkAuthFlags& flags,DWORD tag);

	void
		SetTag(UINT index,DWORD tag);

	void
		SetFlags(UINT index,const TSonorkAuthFlags& flags);

	DWORD
		GetTag(UINT index) const ;

	const TSonorkAuthFlags&
		GetFlags(UINT index) const;


}__SONORK_PACKED;



struct  TSonorkUserInfo1
{
	DWORD			_Reserved0;
	TSonorkTime		bornTime;
	TSonorkUserInfoFlags	infoFlags;
	TSonorkRegion		region;
	SONORK_DWORD2		_Reserved1;

	DWORD			pubAuthPin;
	TSonorkAuthFlags	pubAuthFlags;

	TSonorkSerial		serial;

	void
		Clear()
		{ memset(this,0,sizeof(*this));	}

	void	ClearUnusedFields()
		{ _Reserved0=0;_Reserved1.Clear(); }

	TSonorkUserInfoFlags&
		InfoFlags()
		{ return infoFlags;}

	TSonorkRegion&
		Region()
		{ return region; 	}

	TSonorkTime&
		BornTime()
		{ return bornTime; }


	TSonorkSerial&
		Serial()
		{ return serial;}

	DWORD
		PublicAuthPin()	const
		{ return pubAuthPin;}

	TSonorkAuthFlags&
		PublicAuthFlags()
		{ return pubAuthFlags;}

	SONORK_SEX
		GetSex() const
		{ return infoFlags.GetSex();}

	BOOL
		IsAdministrator() const
		{ return infoFlags.IsAdministrator(); }

	BOOL
		IsRobot() const
		{ return infoFlags.IsRobot();	}

	BOOL
		IsBroadcaster() const
		{ return infoFlags.IsBroadcaster();	}


}__SONORK_PACKED;

struct  TSonorkUserInfo2
:public TSonorkUserInfo1
{
	TSonorkTime		lastLogin;
	DWORD			loginCount;
	DWORD			_Reserved3;
	DWORD			_Reserved4;

	void
		Clear()
		{ memset(this,0,sizeof(*this));	}

	void
		ClearUnusedFields()
		{ TSonorkUserInfo1::ClearUnusedFields(); _Reserved3=_Reserved4=0;}


}__SONORK_PACKED;

struct  TSonorkUserInfo3
:public TSonorkUserInfo2
{
	TSonorkAuthFlags	privAuthFlags;
	DWORD			loginFlags;
	DWORD			_Reserved5;

const TSonorkAuthFlags&		PrivateAuthFlags() const
				{ return privAuthFlags;	}

	TSonorkAuthFlags&	wPrivateAuthFlags()
				{ return privAuthFlags;	}

	void	Clear()
	{
		memset(this,0,sizeof(*this));
	}

	void	ClearUnusedFields()
	{
		TSonorkUserInfo2::ClearUnusedFields();
		_Reserved5=0;
	}
} __SONORK_PACKED;


#define SONORK_SERVICE_ID_MASK			0x00ffffff
#define SONORK_SERVICE_TYPE_MASK		0x000fffff
#define SONORK_SERVICE_HFLAGS_MASK		0x7ff00000

enum SONORK_SERVICE_HFLAGS
{
  SONORK_SERVICE_HFLAG_SYSTEMWIDE	=	0x00100000
, SONORK_SERVICE_HFLAG_LOCATOR		=	0x00200000
};

struct TSonorkServiceId
:public SONORK_DWORD2
{
	enum V_INDEX
	{
		V_ID	    	= 0
	,	V_INSTANCE	= 1
	};

	SONORK_SERVICE_ID
		ServiceId()	const
		{ return (SONORK_SERVICE_ID)( v[V_ID]&SONORK_SERVICE_ID_MASK );	}
		
	DWORD
		ServiceInstance() const
		{ return v[V_INSTANCE];	}

};

struct	TSonorkServiceInstance
:public SONORK_DWORD2
{
	enum V_INDEX
	{
		V_INSTANCE		= 0
	,	V_VERSION		= 1
	};
	DWORD
		ServiceInstance() const
		{ return v[V_INSTANCE];	}

	DWORD
		ServiceVersionNumber()	const
		{ return v[V_VERSION];	}

	void	SetInstance(DWORD instance,DWORD version)
		{ v[V_INSTANCE]=instance;v[V_VERSION]=version;}


}__SONORK_PACKED;

struct TSonorkServiceInfo
:public SONORK_DWORD2
{
public:
	enum V_INDEX
	{
		V_ID	    	= 0
	,	V_DESCRIPTOR	= 1
	};

	SONORK_SERVICE_ID
		ServiceId()	const
		{ return (SONORK_SERVICE_ID)( v[V_ID]&SONORK_SERVICE_ID_MASK );	}

	DWORD
		ServiceDescriptor() const
		{ return v[V_DESCRIPTOR];	}

	SONORK_SERVICE_TYPE
		ServiceType()	const
		{ return (SONORK_SERVICE_TYPE)(v[V_DESCRIPTOR]&SONORK_SERVICE_TYPE_MASK); }

	DWORD
		ServiceHFlags()	const
		{ return (v[V_DESCRIPTOR]&SONORK_SERVICE_HFLAGS_MASK);	}

	void
		SetInfo(SONORK_SERVICE_ID svc_id
			,SONORK_SERVICE_TYPE svc_type
			,DWORD svc_flags);
	BOOL
		IsSystemWideService() const
		{ return v[V_DESCRIPTOR]&SONORK_SERVICE_HFLAG_SYSTEMWIDE; }

	BOOL
		IsLocatorService() const
		{ return v[V_DESCRIPTOR]&SONORK_SERVICE_HFLAG_LOCATOR; }

}__SONORK_PACKED;

struct	TSonorkServiceState
:public SONORK_DWORD2
{
	enum V_INDEX
	{
		V_STATE_PRIO		= 0
	,	V_RESERVED		= 1
	};

	SONORK_SERVICE_STATE
		ServiceState() const
		{
			return (SONORK_SERVICE_STATE)
			((v[V_STATE_PRIO]&0xff00)>>8);
		}

	SONORK_SERVICE_PRIORITY
		ServicePriority()	const
		{
			return (SONORK_SERVICE_PRIORITY)
			((v[V_STATE_PRIO]&0x00ff)>>0);
		}

	void	SetState(SONORK_SERVICE_STATE		state
			,SONORK_SERVICE_PRIORITY 	priority)
		{
			v[V_STATE_PRIO]=( ((state&0xff)<<8)|(priority&0xff) );
			v[V_RESERVED]=0;
		}

}__SONORK_PACKED;

struct
 TSonorkServiceInstanceInfo
{
	TSonorkServiceInstance		inst;
	TSonorkServiceInfo		info;

	SONORK_SERVICE_ID
		ServiceId()	const
		{ return info.ServiceId();	}

	SONORK_SERVICE_TYPE
		ServiceType()	const
		{ return info.ServiceType(); }

	DWORD
		ServiceHFlags()	const
		{ return info.ServiceHFlags();	}

	DWORD
		ServiceDescriptor()	const
		{ return info.ServiceDescriptor();	}

	BOOL
		IsSystemWideService() const
		{ return info.IsSystemWideService(); }

	BOOL
		IsLocatorService() const
		{ return info.IsLocatorService(); }

	DWORD
		ServiceInstance()	const
		{ return inst.ServiceInstance();	}

	DWORD
		ServiceVersionNumber()	const
		{ return inst.ServiceVersionNumber();	}

	void
		Set(const TSonorkServiceInstanceInfo& O)
		{ memcpy(this,&O,sizeof(*this)); }

	void
		SetInstanceInfo(
			  SONORK_SERVICE_ID	service_id
			, SONORK_SERVICE_TYPE 	service_type
			, DWORD 		service_hflags
			, DWORD 		server_instance
			, DWORD 		server_version
			)
		{
			SetInfo(service_id
				,service_type
				,service_hflags);
			SetInstance(server_instance
				,server_version);
		}

	void
		SetInfo(SONORK_SERVICE_ID svc_id
			,SONORK_SERVICE_TYPE svc_type
			,DWORD svc_flags)
		{ info.SetInfo(svc_id,svc_type,svc_flags);}

	void
		SetInstance(DWORD instance,DWORD version)
		{ inst.SetInstance(instance,version);}

	void
		Clear()
		{ memset(this,0,sizeof(*this)); }

};

struct	TSonorkOldServiceServerExt
:public TSonorkServiceInstance
{
	enum V2_INDEX
	{
		V2_DESCRIPTOR	= 0
	,	V2_RESERVED	= 1
	};
	DWORD	v2[2];

	SONORK_SERVICE_TYPE
		ServiceType()	const
		{ return (SONORK_SERVICE_TYPE)(v2[V2_DESCRIPTOR]&SONORK_SERVICE_TYPE_MASK); }

	DWORD
		ServiceHFlags()	const
		{ return (v2[V2_DESCRIPTOR]&SONORK_SERVICE_HFLAGS_MASK);	}

	DWORD
		ServiceDescriptor()	const
		{ return v2[V2_DESCRIPTOR];	}

	void
		SetService(SONORK_SERVICE_TYPE svc_type, DWORD hflags);
};


struct  TSonorkServiceLocus1
:public TSonorkUserLocus1
{
	TSonorkServiceInstanceInfo	info;

	SONORK_SERVICE_ID
		ServiceId()	const
		{ return info.ServiceId();	}

	SONORK_SERVICE_TYPE
		ServiceType()	const
		{ return info.ServiceType();	}

	DWORD
		ServiceHFlags()	const
		{ return info.ServiceHFlags();	}

	DWORD
		ServiceInstance()	const
		{ return info.ServiceInstance();	}

	DWORD
		ServiceVersionNumber()	const
		{ return info.ServiceVersionNumber(); }

	DWORD
		ServiceDescriptor()	const
		{ return info.ServiceDescriptor();	}

	BOOL
		IsSystemWideService() const
		{ return info.IsSystemWideService(); }

	BOOL
		IsLocatorService() const
		{ return info.IsLocatorService(); }

	void
		SetInstanceInfo(
			  SONORK_SERVICE_ID	service_id
			, SONORK_SERVICE_TYPE 	service_type
			, DWORD 		service_hflags
			, DWORD 		server_instance
			, DWORD 		server_version
			)
		{
			info.SetInstanceInfo(service_id
				, service_type
				, service_hflags
				, server_instance
				, server_version);
		}

	void
		SetInstanceInfo( const TSonorkServiceInstanceInfo& O )
		{
			info.Set( O );
		}

	void
		SetInfo(SONORK_SERVICE_ID si,SONORK_SERVICE_TYPE st,DWORD sf)
		{
			info.SetInfo(si,st,sf);
		}

	void	SetInstance(DWORD instance,DWORD version)
		{
			info.SetInstance(instance,version);
		}

}__SONORK_PACKED;


struct TSonorkMsgTarget
{
	TSonorkId		userId;
	TSonorkServiceId	service;

	void
		Set(const TSonorkMsgTarget&O)
		{ memcpy(this,&O,sizeof(*this)); }
	void
		Clear()
		{ memset(this,0,sizeof(*this)); }

	SONORK_SERVICE_ID
		ServiceId()	const
		{ return service.ServiceId();	}

	DWORD
		ServiceInstance() const
		{ return service.ServiceInstance();	}
};

struct  TSonorkOldMsgHeader
{
	TSonorkId		userId;
	DWORD    		usrFlags;	// Old reserved
	DWORD			sysFlags;
	TSonorkServiceInfo	dataSvcInfo;
	SONORK_DWORD2		trackingNo;
	TSonorkTime  		time;

	void Set(const TSonorkOldMsgHeader&O)
	{
		memcpy(this,&O,sizeof(*this));
	}

	DWORD
		SysFlags() const
		{ return sysFlags;}

	DWORD
		UsrFlags() const
		{ return usrFlags;}

	SONORK_DWORD2&
		TrackingNo()
		{ return trackingNo;}

const	TSonorkId&
		SenderUserId() const
		{ return userId; }

const	TSonorkServiceInfo&
		DataServiceInfo() 	const
		{ return dataSvcInfo;}

	TSonorkServiceInfo&
		wDataServiceInfo()
		{ return dataSvcInfo;}

	SONORK_SERVICE_ID
		DataServiceId() const
		{ return dataSvcInfo.ServiceId();}

	DWORD
		DataServiceDescriptor()	const
		{ return dataSvcInfo.ServiceDescriptor();}

	SONORK_SERVICE_TYPE
		DataServiceType()	const
		{ return dataSvcInfo.ServiceType();}

	DWORD
		DataServiceHFlags()	const
		{ return dataSvcInfo.ServiceHFlags();}

	void
		SetDataServiceInfo(
			SONORK_SERVICE_ID 	serviceId
			,SONORK_SERVICE_TYPE 	serviceType
			,DWORD 			serviceHflags)
		{
			dataSvcInfo.SetInfo(serviceId
				,serviceType
				,serviceHflags);
		}


} __SONORK_PACKED;

struct  TSonorkMsgHeader
:public TSonorkOldMsgHeader
{
	TSonorkMsgTarget	target;
	TSonorkServiceId	sourceService;


const	TSonorkId&
		TargetUserId() const
		{ return target.userId; }

const	SONORK_SERVICE_ID
		TargetServiceId() const
		{ return target.ServiceId(); }

const	DWORD
		TargetServiceInstance() const
		{ return target.ServiceInstance(); }

const	SONORK_SERVICE_ID
		SourceServiceId() const
		{ return sourceService.ServiceId(); }

const	DWORD
		SourceServiceInstance() const
		{ return sourceService.ServiceInstance(); }

	void SetOld(const TSonorkOldMsgHeader&O)
	{
		memcpy(this,&O,sizeof(TSonorkOldMsgHeader));
		target.Clear();

	}
	void Set(const TSonorkMsgHeader&O)
	{
		memcpy(this,&O,sizeof(*this));
	}
} __SONORK_PACKED;

struct TSonorkOldMsgNotification
{
	TSonorkObjId		msg_id;
	TSonorkOldMsgHeader	header;
	DWORD			msg_size;


	void Set(const TSonorkOldMsgNotification&O)
	{
		memcpy(this,&O,sizeof(*this));
	}

} __SONORK_PACKED;

struct TSonorkShortString
{
private:
	SONORK_C_STR	str;
public:

	TSonorkShortString(){str=NULL;}
	TSonorkShortString(SONORK_C_CSTR p_str);
	~TSonorkShortString(){Clear();}

	void *operator new(size_t t)	{return SONORK_MEM_ALLOC(BYTE,t);}
	void operator delete(void*ptr)	{SONORK_MEM_FREE(ptr);}
	void *operator new[](size_t t)	{return SONORK_MEM_ALLOC(BYTE,t);}
	void operator delete[](void*ptr){SONORK_MEM_FREE(ptr);}

	SONORK_C_CSTR
		CStr()       const ;

	bool  	IsNull()      const
		{ return str==NULL;}

	UINT  	Length()      const
		{ return IsNull()?0:strlen(str);	}

	UINT	CODEC_Size()  const
		{ return sizeof(DWORD)+Length();		}

	SONORK_C_CSTR
		Set(SONORK_C_CSTR);

	SONORK_C_CSTR
		Set(const TSonorkShortString& O)
		{ return Set(O.CStr()); }

	UINT
		Trim();		// Returns resultant length

	UINT
		CutAt(UINT sz);	// Returns resultant length

	SONORK_C_STR
		SetBufferSize(UINT sz);

	// Assign: Discards current buffer, points to <new_buffer>
	// <new_buffer> must have been created with SONORK_MEM_ALLOC()
	SONORK_C_CSTR
		Assign(SONORK_C_STR new_buffer);

	SONORK_C_STR
		Buffer()
		{ return str; }

	bool  	operator==(SONORK_C_CSTR p_str);

	bool  	operator!=(SONORK_C_CSTR p_str)
		{ return !((*this)==p_str);}

	void
		Clear();

	SONORK_C_CSTR
		Append(SONORK_C_CSTR str1, SONORK_C_CSTR str2=NULL);
};




class  TSonorkTempBuffer
{
	BYTE*buffer;
	UINT size;
public:
	TSonorkTempBuffer(UINT sz);
	~TSonorkTempBuffer();

	void*	operator new(size_t t)
		{return SONORK_MEM_ALLOC(BYTE,t);}

	void  	operator delete(void*ptr)
		{SONORK_MEM_FREE(ptr);}

	void*	operator new[](size_t t)
		{return SONORK_MEM_ALLOC(BYTE,t);}

	void	operator delete[](void*ptr)
		{SONORK_MEM_FREE(ptr);}

	void
		Set(const char*);
	UINT
		GetSize() const
		{ return size;}
	void
		SetSize(UINT);

	// SetMinSize reallocates only if cur size < new_size
	void
		SetMinSize(UINT new_size);
	char*
		CStr()
		{ return (char*)buffer;}

	BYTE*
		Ptr()
		{ return buffer;}

	operator char*()
		{ return (char*)buffer;}

	operator BYTE*()
		{ return (BYTE*)buffer;}

	operator void*()
		{ return (void*)buffer;}
};

// ----------------------------------------------------------------------------

enum SONORK_PROFILE_CTRL_FLAG
{
    SONORK_PCF_AUTO_AUTH		=0x00000010	// SONORK_PCF_DEFAULT
,   SONORK_PCF_INITIALIZED		=0x00000020
,   SONORK_PCF_NO_AUTO_CONNECT		=0x00000040
,   SONORK_PCF_NO_SAVE_PASS		=0x00000080
,   SONORK_PCF_PASS_PROTECT		=0x00000100
,   SONORK_PCF_NO_PUBLIC_UTS		=0x00001000
,   SONORK_PCF_SONORK_UTS_USE_REVFW	=0x00002000
,   SONORK_PCF_USING_PROXY		=0x00004000
,   SONORK_PCF_MUST_REFRESH_DATA	=0x00010000
,   SONORK_PCF_MUST_REFRESH_LIST	=0x00020000
,   SONORK_PCF_1			=0x00000011
,   SONORK_PCF_2			=0x00000021
,   SONORK_PCF_3			=0x00000041
,   SONORK_PCF_4			=0x00000081
,   SONORK_PCF_5			=0x00000101
,   SONORK_PCF_6			=0x00000201
,   SONORK_PCF_7			=0x00000401
,   SONORK_PCF_8			=0x00000801
,   SONORK_PCF_9			=0x00001001
,   SONORK_PCF_10			=0x00002001
,   SONORK_PCF_11			=0x00004001
,   SONORK_PCF_12			=0x00008001
,   SONORK_PCF_13			=0x00010001
,   SONORK_PCF_14			=0x00020001
,   SONORK_PCF_15			=0x00040001
,   SONORK_PCF_16			=0x00080001
,   SONORK_PCF_17			=0x00100001
,   SONORK_PCF_18			=0x00200001
,   SONORK_PCF_19			=0x00400001
,   SONORK_PCF_20			=0x00800001
};
enum SONORK_PROFILE_RUN_FLAG
{
    SONORK_PRF_REFRESH_PENDING		=0x00000010
,   SONORK_PRF_DIRTY			=0x00000020
,   SONORK_PRF_1			=0x00000021
,   SONORK_PRF_2			=0x00000041
,   SONORK_PRF_3			=0x00000081
,   SONORK_PRF_4			=0x00000101
,   SONORK_PRF_5			=0x00000201
,   SONORK_PRF_6			=0x00000401
,   SONORK_PRF_7			=0x00000801
};
enum SONORK_PROFILE_CTRL_VALUE
{
    SONORK_PCV_1 			// 0
,   SONORK_PCV_2                        // 1
,   SONORK_PCV_3			// 2
,   SONORK_PCV_4			// 3
,   SONORK_PCV_EMAIL_CHECK_MINS		// 4
,   SONORK_PCV_EMAIL_ACCOUNT_UID	// 5
,   SONORK_PCV_USER_LIST_SIZE		// 6
,   SONORK_PCV_MAX_APP_VERSION		// 7
,   SONORK_PROFILE_V1400_VALUES		// 8
,   SONORK_PCV_5			= SONORK_PROFILE_V1400_VALUES // 8
,   SONORK_PCV_6			// 9
,   SONORK_PCV_7			// 10
,   SONORK_PCV_8			// 11
,   SONORK_PCV_9			// 12
,   SONORK_PCV_10			// 13
,   SONORK_PCV_11			// 14
,   SONORK_PCV_12			// 15
,   SONORK_PROFILE_V1500_CTRL_VALUES	// 16
};
enum SONORK_PROFILE_RUN_VALUE
{
    SONORK_PRV_1
,   SONORK_PRV_2
,   SONORK_PRV_3
,   SONORK_PRV_4
,   SONORK_PROFILE_RUN_VALUES
};

enum SONORK_APP_CTRL_VALUE
{
    SONORK_APP_CV_1
,   SONORK_APP_CV_2
,   SONORK_APP_CV_3
,   SONORK_APP_CV_4
,   SONORK_APP_CV_5
,   SONORK_APP_CV_6
,   SONORK_APP_CTRL_VALUES
};
enum SONORK_APP_CTRL_FLAG
{
    SONORK_ACF_1			=0x00000010
,   SONORK_ACF_2			=0x00000020
,   SONORK_ACF_3			=0x00000040
,   SONORK_ACF_4			=0x00000080
,   SONORK_ACF_5			=0x00000100
,   SONORK_ACF_6			=0x00000200
,   SONORK_ACF_7			=0x00000400
,   SONORK_ACF_8			=0x00000800
,   SONORK_ACF_9			=0x00001000
,   SONORK_ACF_10			=0x00002000
,   SONORK_ACF_11			=0x00004000
,   SONORK_ACF_12			=0x00008000
,   SONORK_ACF_13			=0x00010000
,   SONORK_ACF_14			=0x00020000
,   SONORK_ACF_15			=0x00040000	// NO_AUTO_CONNECT
,   SONORK_ACF_16	     		=0x00080000
};
enum SONORK_APP_RUN_VALUE
{
    SONORK_ARV_1
,   SONORK_ARV_2
,   SONORK_ARV_3
,   SONORK_ARV_4
,   SONORK_ARV_MONITOR_TASKS
,   SONORK_APP_RUN_VALUES
};
enum SONORK_APP_RUN_FLAG
{
    SONORK_ARF_1       =0x00000010
,   SONORK_ARF_2       =0x00000020
,   SONORK_ARF_3       =0x00000040
,   SONORK_ARF_4       =0x00000080
,   SONORK_ARF_5       =0x00000100
,   SONORK_ARF_6       =0x00000200
,   SONORK_ARF_7       =0x00000400
,   SONORK_ARF_8       =0x00000800
,   SONORK_ARF_9       =0x00001000
,   SONORK_ARF_10      =0x00002000
,   SONORK_ARF_11      =0x00004000
,   SONORK_ARF_12      =0x00008000
,   SONORK_ARF_13      =0x00010000
,   SONORK_ARF_14      =0x00020000
,   SONORK_ARF_15      =0x00040000
,   SONORK_ARF_16      =0x00080000
,   SONORK_ARF_17      =0x00100000

,   SONORK_ARF_18      =0x00200000
,   SONORK_ARF_19      =0x00400000
,   SONORK_ARF_20      =0x00800000
,   SONORK_ARF_21      =0x01000000

,   SONORK_ARF_22      =0x02000000
,   SONORK_ARF_23      =0x04000000
,   SONORK_ARF_24      =0x08000000
,   SONORK_ARF_25      =0x10000000
,   SONORK_ARF_26      =0x20000000
,   SONORK_ARF_27      =0x40000000

,   SONORK_ARF_28      =0x80000000
};


struct  TSonorkProfileRunData
{
	TSonorkFlags	flags;
	DWORD		value[SONORK_PROFILE_RUN_VALUES];
};

struct  TSonorkAppRunData
{
	TSonorkFlags	flags;
	DWORD           value[SONORK_APP_RUN_VALUES];
};


struct TSonorkUTSDescriptor
{
	DWORD			serviceId;
	DWORD			instance;
	DWORD			flags;
	TSonorkUserLocus1	locus;

	SONORK_SERVICE_ID
		ServiceId()		const
		{ return (SONORK_SERVICE_ID)serviceId; }

	DWORD
		Flags()  const
		{ return flags;}

}__SONORK_PACKED;

#define SONORK_DYN_SERVER_RAW_DWORDS	16

struct  TSonorkDynServerHeader
{
	struct T_USERVER{
		TSonorkRegion   region;
		DWORD		utsFlags;
		DWORD		data[5];
	}__SONORK_PACKED;


	TSonorkServiceLocus1	locus;
	TSonorkServiceState	state;

	union{
		DWORD		raw_data[SONORK_DYN_SERVER_RAW_DWORDS];
		T_USERVER	user;
	};

const	TSonorkId&
		UserId() const
		{ return locus.userId;}

const	TSonorkSid&
		Sid() const
		{ return locus.sid;}

const   TSonorkServiceInstanceInfo&
		ServiceInstanceInfo() const
		{ return locus.info; }

	TSonorkServiceInstanceInfo&
		wServiceInstanceInfo()
		{ return locus.info; }

	SONORK_SERVICE_ID
		ServiceId()	const
		{ return locus.ServiceId();	}

	SONORK_SERVICE_TYPE
		ServiceType()	const
		{ return locus.ServiceType();	}

	DWORD
		ServiceInstance() const
		{ return locus.ServiceInstance();	}

	DWORD
		ServiceVersionNumber()	const
		{ return locus.ServiceVersionNumber(); }

	DWORD
		ServiceDescriptor()	const
		{ return locus.ServiceDescriptor();	}

	BOOL
		IsSystemWideService() const
		{ return locus.IsSystemWideService();}

	BOOL
		IsLocatorService() const
		{ return locus.IsLocatorService(); }

	SONORK_SERVICE_STATE
		ServiceState() const
		{ return state.ServiceState();}

	SONORK_SERVICE_PRIORITY
		ServicePriority() const
		{ return state.ServicePriority();}


	void
		SetInstanceInfo(
			  SONORK_SERVICE_ID	service_id
			, SONORK_SERVICE_TYPE 	service_type
			, DWORD 		service_hflags
			, DWORD 		server_instance
			, DWORD 		server_version
			)
		{
			locus.SetInstanceInfo(service_id
				, service_type
				, service_hflags
				, server_instance
				, server_version);
		}

	void
		SetInstanceInfo( const TSonorkServiceInstanceInfo& O )
		{
			locus.info.Set( O );
		}

	void
		SetInfo(SONORK_SERVICE_ID si,SONORK_SERVICE_TYPE st,DWORD sf)
		{
			locus.SetInfo(si,st,sf);
		}

	void	SetInstance(DWORD instance,DWORD version)
		{
			locus.SetInstance(instance,version);
		}

	void
		SetState(SONORK_SERVICE_STATE s,SONORK_SERVICE_PRIORITY p=SONORK_SERVICE_PRIORITY_NORMAL)
		{
			state.SetState(s,p);
		}



}__SONORK_PACKED;	// V1.04.06: this was wrong as "__SONORK_PACKET";

enum SONORK_TRACKER_FLAG0
{
// Flags used for TSonorkTrackerData
  SONORK_TRACKER_DATA_F0_HELP_LEVEL	=0x0000000f
, SONORK_TRACKER_DATA_F0_ACTIVE		=0x00000100

// Flags used for TSonorkTrackerRoom
, SONORK_TRACKER_ROOM_F0_ACTIVE		=SONORK_TRACKER_DATA_F0_ACTIVE

};

enum SONORK_TRACKER_FLAG1
{
// Flags used for TSonorkTrackerData
  SONORK_TRACKER_DATA_F1_APP_DETECTOR	=0x10000000

// Flags used for TSonorkTrackerRoom
, SONORK_TRACKER_ROOM_F1_APP_DETECTOR	=SONORK_TRACKER_DATA_F1_APP_DETECTOR
};

struct TSonorkTrackerId
:public SONORK_DWORD2
{
	enum V_INDEX
	{
		V_ROOM_ID	= 0
	,	V_GROUP_ID	= 0
	,	V_PARENT	= 1
	};
};

struct  TSonorkTrackerFlags
:public SONORK_DWORD4
{
	DWORD&
		Flags0()
		{ return v[0].v[0];	}

	DWORD&
		Flags1()
		{ return v[0].v[1];	}

	void
		Set(const SONORK_DWORD4& f)
		{ SONORK_DWORD4::Set(f);}

	void
		Clear( void )
		{ SONORK_DWORD4::Clear();}

	void
		Set(SONORK_TRACKER_FLAG0 f)
		{ v[0].v[0]|=f;}

	void
		Clear(SONORK_TRACKER_FLAG0 f)
		{ v[0].v[0]&=~f;}

	BOOL
		Test(SONORK_TRACKER_FLAG0 f) const
		{ return v[0].v[0]&f; }

	void
		Toggle(SONORK_AUTH_FLAG0 f)
		{ v[0].v[0]^=f; }

	void
		Set(SONORK_TRACKER_FLAG1 f)
		{ v[0].v[1]|=f;}

	void
		Clear(SONORK_TRACKER_FLAG1 f)
		{ v[0].v[1]&=~f;}

	BOOL
		Test(SONORK_TRACKER_FLAG1 f) const
		{ return v[0].v[1]&f; }

	void
		Toggle(SONORK_TRACKER_FLAG1 f)
		{ v[0].v[1]^=f; }

	BOOL
		IsActive() const
		{ return Test(SONORK_TRACKER_DATA_F0_ACTIVE); }

	BOOL
		IsAppDetector() const
		{ return Test(SONORK_TRACKER_DATA_F1_APP_DETECTOR); }

	BOOL
		IsHelper() const
		{ return HelpLevel() != 0;}

	DWORD
		HelpLevel() const
		{ return GetValueAt(0)&SONORK_TRACKER_DATA_F0_HELP_LEVEL; }

	void
		SetHelpLevel(DWORD v)
		{ SetValueAt(0
			,(GetValueAt(0)&~SONORK_TRACKER_DATA_F0_HELP_LEVEL)
			|(v&SONORK_TRACKER_DATA_F0_HELP_LEVEL));}

};

struct  TSonorkTrackerHeader
{
	TSonorkTrackerId	id;
	TSonorkTrackerFlags	flags;
	DWORD			reserved[6];


	BOOL
		IsActive() const
		{ return flags.IsActive(); }

	BOOL
		IsAppDetector() const
		{ return flags.IsAppDetector(); }

	BOOL
		IsHelper() const
		{ return flags.IsHelper();}

	DWORD
		HelpLevel() const
		{ return flags.HelpLevel(); }

	void
		SetHelpLevel(DWORD v)
		{ flags.SetHelpLevel(v);}

	void
		Set(const TSonorkTrackerHeader& O)
		{ memcpy(this,&O,sizeof(*this)); }

}__SONORK_PACKED;


enum SONORK_OLD_CTRLMSG_CMD
{
  SONORK_OLD_CTRLMSG_CMD_NONE		=0
, SONORK_OLD_CTRLMSG_CMD_PROBE		=1
, SONORK_OLD_CTRLMSG_CMD_EXEC_START	=10


, SONORK_OLD_CTRLMSG_CMD_QUERY_REQUEST  =1024
, SONORK_OLD_CTRLMSG_CMD_QUERY_RESULT
, SONORK_OLD_CTRLMSG_CMD_QUERY_END
, SONORK_OLD_CTRLMSG_CMD_EXEC_STATUS
, SONORK_OLD_CTRLMSG_CMD_EXEC_RESULT

};

enum SONORK_CTRLMSG_CMD
{
  SONORK_CTRLMSG_CMD_NONE		= 0
, SONORK_CTRLMSG_CMD_PROBE		= 1
, SONORK_CTRLMSG_CMD_OPEN		= 2
, SONORK_CTRLMSG_CMD_CLOSE		= 3
, SONORK_CTRLMSG_CMD_UTS_LINK		= 4
, SONORK_CTRLMSG_CMD_SYSTEM_LIMIT	= 0x10000
, SONORK_CTRLMSG_CMD_GET_SERVER_DATA
, SONORK_CTRLMSG_CMD_START
, SONORK_CTRLMSG_CMD_USER_BASE		= 0x20000
, SONORK_CTRLMSG_CMD_01
, SONORK_CTRLMSG_CMD_02
, SONORK_CTRLMSG_CMD_03
, SONORK_CTRLMSG_CMD_04
, SONORK_CTRLMSG_CMD_05
, SONORK_CTRLMSG_CMD_06
, SONORK_CTRLMSG_CMD_07
, SONORK_CTRLMSG_CMD_08
, SONORK_CTRLMSG_CMD_09
, SONORK_CTRLMSG_CMD_10
, SONORK_CTRLMSG_CMD_11
, SONORK_CTRLMSG_CMD_12
, SONORK_CTRLMSG_CMD_13
, SONORK_CTRLMSG_CMD_14
, SONORK_CTRLMSG_CMD_15
, SONORK_CTRLMSG_CMD_16
, SONORK_CTRLMSG_CMD_17
, SONORK_CTRLMSG_CMD_18
, SONORK_CTRLMSG_CMD_19
, SONORK_CTRLMSG_CMD_20
};
enum SONORK_CTRLMSG_CMD_FLAGS
{
  SONORK_CTRLMSG_CF_01			= 0x00100000
, SONORK_CTRLMSG_CF_02			= 0x00200000
, SONORK_CTRLMSG_CF_03			= 0x00400000
, SONORK_CTRLMSG_CF_04			= 0x00800000
, SONORK_CTRLMSG_CF_05			= 0x01000000
, SONORK_CTRLMSG_CF_06			= 0x02000000
, SONORK_CTRLMSG_CF_07			= 0x04000000
, SONORK_CTRLMSG_CF_08			= 0x08000000
// Some commonly used aliases for standard commands
, SONORK_CTRLMSG_CF_ACCEPT		= SONORK_CTRLMSG_CF_01
, SONORK_CTRLMSG_CF_DATA		= SONORK_CTRLMSG_CF_02
, SONORK_CTRLMSG_CF_RESULT		= SONORK_CTRLMSG_CF_03
};

enum SONORK_CTRLMSG_SYS_FLAGS
{
  SONORK_CTRLMSG_SF_TYPE_NONE		= 0x00000000
, SONORK_CTRLMSG_SF_TYPE_QUERY		= 0x00000100
, SONORK_CTRLMSG_SF_TYPE_CIRCUIT	= 0x00000200
, SONORK_CTRLMSG_SF_FLOW_NONE		= 0x00000000
, SONORK_CTRLMSG_SF_FLOW_REQ		= 0x00001000
, SONORK_CTRLMSG_SF_FLOW_AKN		= 0x00002000
, SONORK_CTRLMSG_SF_FLOW_MASK		= 0x00007000
, SONORK_CTRLMSG_SF_TYPE_MASK		= 0x00000f00
, SONORK_CTRLMSG_SF_VERSION_MASK	= 0x000000ff
, SONORK_CTRLMSG_SF_NEW			= 0x80000000
};
#define SONORK_OLD_CTRLMSG_SYS_FLAGS_MASK	0xffff0000
#define SONORK_OLD_CTRLMSG_USER_FLAGS_MASK	0x0000ffff


#define	SONORK_SERVICE_CTRLMSG_CMD_V_MASK	0x000fffff
#define SONORK_SERVICE_CTRLMSG_CMD_F_MASK	0x0ff00000
#define SONORK_SERVICE_INSTANCE_V_MIN_VALUE	0x00010000
#define SONORK_SERVICE_INSTANCE_V_MAX_VALUE	0x000fffff
#define SONORK_SERVICE_INSTANCE_V_MASK		0x000fffff
#define SONORK_SERVICE_INSTANCE_F_SERVER	0x00100000
#define SONORK_SERVICE_INSTANCE_F_LOCATOR	0x00200000
#define SONORK_SERVICE_INSTANCE_MASK_SERVER	(SONORK_SERVICE_INSTANCE_F_SERVER|SONORK_SERVICE_INSTANCE_V_MASK)
#define SONORK_SERVICE_INSTANCE_MASK_LOCATOR	(SONORK_SERVICE_INSTANCE_F_LOCATOR)
#define SONORK_SERVICE_LOCATOR_INSTANCE(n)	(SONORK_SERVICE_INSTANCE_F_LOCATOR|((n)&SONORK_SERVICE_INSTANCE_V_MASK))
#define SONORK_SERVICE_SERVER_INSTANCE(n)	(SONORK_SERVICE_INSTANCE_F_SERVER|((n)&SONORK_SERVICE_INSTANCE_V_MASK))


struct  TSonorkOldCtrlMsgParams
{
	DWORD	param[4];
}__SONORK_PACKED;

struct  TSonorkNewCtrlMsgParams
{
	DWORD	usrParam;
	DWORD	usrFlags;
	DWORD	serviceId;
	DWORD	systemId;
}__SONORK_PACKED;

union TSonorkCtrlMsgBody
{
	TSonorkOldCtrlMsgParams	o;
	TSonorkNewCtrlMsgParams	n;
}__SONORK_PACKED;

struct TSonorkCtrlGetServerDataReq
{
	DWORD		sizeof_this;
	TSonorkVersion	sonork_version;
	DWORD		sender_version;
}__SONORK_PACKED;


inline SONORK_CTRLMSG_CMD
 MakeSonorkCtrlMsgCmd(SONORK_CTRLMSG_CMD cmd,DWORD flags)
{
	return (SONORK_CTRLMSG_CMD)
		((cmd&SONORK_SERVICE_CTRLMSG_CMD_V_MASK)
		|(flags&SONORK_SERVICE_CTRLMSG_CMD_F_MASK));
}


struct  TSonorkCtrlMsg
{
public:
	DWORD			cmdFlags;
	DWORD			sysFlags;
	DWORD			target_instance;
	DWORD			source_instance;
	TSonorkCtrlMsgBody	body;

	void
		Clear();

	SONORK_CTRLMSG_CMD
		Cmd() const
		{ return (SONORK_CTRLMSG_CMD)(cmdFlags&SONORK_SERVICE_CTRLMSG_CMD_V_MASK);}

	DWORD
		CmdFlags() const
		{ return (cmdFlags&SONORK_SERVICE_CTRLMSG_CMD_F_MASK);}

	DWORD
		SysFlags() const
		{ return (sysFlags);}

	BOOL
		IsSystemCmd() const
		{ return Cmd()<SONORK_CTRLMSG_CMD_SYSTEM_LIMIT; }

	BOOL
		TargetIsBroadcast() const
		{ return (target_instance&SONORK_SERVICE_INSTANCE_V_MASK)==0;}

	DWORD
		Type() const
		{ return sysFlags&SONORK_CTRLMSG_SF_TYPE_MASK;}

	DWORD
		Flow() const
		{ return sysFlags&SONORK_CTRLMSG_SF_FLOW_MASK;}

	DWORD
		Version() const
		{ return sysFlags&SONORK_CTRLMSG_SF_VERSION_MASK;}

	DWORD
		UserFlags() const
		{ return body.n.usrFlags; }

	DWORD
		UserParam() const
		{ return body.n.usrParam; }

	SONORK_SERVICE_ID
		ServiceId() const
		{ return (SONORK_SERVICE_ID)(body.n.serviceId); }

	DWORD
		QueryId() const
		{ return body.n.systemId; }

	DWORD
		CircuitId() const
		{ return body.n.systemId; }


	// NB: LoadNewCmd() resets ALL members of this structure
	void
		LoadNewCmd(
			  SONORK_CTRLMSG_CMD
			, DWORD usr_param
			, DWORD usr_flags
			, DWORD sys_flags
			);


	// Compatibility with old version ---------------------------------

	SONORK_OLD_CTRLMSG_CMD
		OldCmd() const
		{ return (SONORK_OLD_CTRLMSG_CMD)cmdFlags ;}
	BOOL
		IsOldCtrlMsg() const
		{ return !(sysFlags&SONORK_CTRLMSG_SF_NEW);}

	void
		ClearOldParams();


	void *operator new(size_t t)	{return SONORK_MEM_ALLOC(BYTE,t);}
	void operator delete(void*ptr)	{SONORK_MEM_FREE(ptr);}
	void *operator new[](size_t t)	{return SONORK_MEM_ALLOC(BYTE,t);}
	void operator delete[](void*ptr){SONORK_MEM_FREE(ptr);}

	// Compatibility with V1.04 notes:
	// V1.04 and before used to send
	// <service_id> as <instance>
	// and <query_id> as <reserved>.
	// We know when it is an old CtrlMsg
	// because the flags don't have the
	// SONORK_CTRLMSG_SF_NEW flag set.
	DWORD
		OldServiceId() const
		{ return target_instance; }

	DWORD
		OldQueryId() const
		{ return source_instance; }

	void
		SetOldServiceId( DWORD pv )
		{ target_instance=pv; }

	void
		SetOldQueryId( DWORD pv)
		{ source_instance=pv; }

}__SONORK_PACKED;

#if defined(USE_PRAGMA_PUSH)
#pragma	pack( pop )
#endif


// ----------------------------------------------------------------------------
// Clock for short intervals (Up to 3 months approx)

struct  TSonorkClock
{

private:
#if defined(SONORK_WIN32_BUILD)
	SONORK_WIN32_TIME	T;
#else
	double		 	T;
#endif

public:
	void 			LoadCurrent();
#if defined(SONORK_WIN32_BUILD)
	__int64			GetClockValue()	const
	{
		return T.v64;
	}
#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))
	double			GetClockValue() const
	{
		return T;
	}
#endif

	void			Set(const TSonorkClock& O);

	// Loads the difference between 'this' - 'O' into 'this'
	void			LoadInterval(const TSonorkClock& O);

	// Loads the difference between 'O1' - 'O2' into 'this'
	void			LoadInterval2(const TSonorkClock& O1 , const TSonorkClock& O2);

    void			LoadIntervalSecs(UINT secs);

	void			LoadIntervalMsecs(UINT msecs);


	// Returns the equivalent, in seconds, of the difference in 'this'
	// (one of the 'LoadInterval' methods should have been called before)
	UINT			IntervalSecs() const;

	// returns diference, in seconds, between this and O ( 'this' - 'O' )
	UINT			IntervalSecsAfter( const TSonorkClock&O ) const ;

	// returns diference, in seconds, between this and the current time
	UINT			IntervalSecsCurrent() const ;

	// Returns the equivalent, in milli-seconds, of the difference in 'this'
	// (one of the 'LoadInterval' methods should have been called before)
	UINT			IntervalMsecs() const;

	// returns diference, in milli seconds, between this and O ( 'this' - 'O' )
	UINT			IntervalMsecsAfter( const TSonorkClock&O) const ;

	// returns diference, in msecs, between this and the current time
	UINT			IntervalMsecsCurrent	()  const ;
};



// ----------------------------------------------------------------------------
// Mutex/Semaphore

#define SONORK_TRACE_LOCKS
struct  TSonorkLock
{
	private:
		UINT		count;
		const char *	file_name;
		UINT		file_line;

#if defined(SONORK_WIN32_BUILD)
		CRITICAL_SECTION cs;
#endif

	public:
		TSonorkLock();
		~TSonorkLock();
		
		UINT
			LockCount() const
			{ return count; }
		void
			Lock(const char*name,UINT line);

		BOOL
			TryLock();

		void
			Unlock();
};



enum SONORK_ATOM_TYPE
{
		SONORK_ATOM_NONE				// 0
	,	SONORK_SYSTEM_ATOMS_BASE			// 1
	,	SONORK_SIMPLE_ATOMS_BASE=SONORK_SYSTEM_ATOMS_BASE	// Base is inclusive
	, 	SONORK_ATOM_NULL	=SONORK_SYSTEM_ATOMS_BASE	// 1

	, 	SONORK_ATOM_GENERIC    				// 2

	,	SONORK_ATOM_DWORD_1    				// 3
	,	SONORK_ATOM_DWORD_2				// 4
	,	SONORK_ATOM_SONORK_ID	=SONORK_ATOM_DWORD_2
	,	SONORK_ATOM_SID		=SONORK_ATOM_DWORD_2
	,	SONORK_ATOM_TAG		=SONORK_ATOM_DWORD_2
	,	SONORK_ATOM_OBJ_ID	=SONORK_ATOM_DWORD_2
	,	SONORK_ATOM_DWORD_4				// 5
	,	SONORK_ATOM_DWORD_V				// 6

	,	SONORK_ATOM_CSTR		// 7 NULL terminated byte character string
	,	SONORK_ATOM_WSTR		// 8 NULL terminated wide character string
	,	SONORK_ATOM_STR_RESERVED_1	// 9 Len-prefixed byte character string
	,	SONORK_ATOM_STR_RESERVED_2	// 10
	,	SONORK_ATOM_STR_RESERVED_3	// 11
	,	SONORK_ATOM_STR_RESERVED_4	// 12

	,	SONORK_ATOM_PASS_1		// 13
	,	SONORK_ATOM_PASS_2		// 14

	,	SONORK_SIMPLE_ATOMS_LIMIT	=64	// Limit is exclusive

	,	SONORK_ATOM_CRYPT_INFO_1        =SONORK_SIMPLE_ATOMS_LIMIT	// 64
	,	SONORK_ATOM_CRYPT_INFO_2	// 65
	,	SONORK_ATOM_CRYPT_INFO_3        // 66
	,	SONORK_ATOM_CRYPT_INFO_4	// 67

	,	SONORK_ATOM_SERVER_LOGIN_REQ_1	// 68
	,	SONORK_ATOM_SERVER_LOGIN_REQ_2	// 69
	,	SONORK_ATOM_SERVER_LOGIN_REQ_3	// 70
	,	SONORK_ATOM_SERVER_LOGIN_REQ_4	// 71

	,	SONORK_ATOM_SERVER_LOGIN_AKN_1	// 72
	,	SONORK_ATOM_SERVER_LOGIN_AKN_2	// 73
	,	SONORK_ATOM_SERVER_LOGIN_AKN_3	// 74
	,	SONORK_ATOM_SERVER_LOGIN_AKN_4	// 75

	,	SONORK_ATOM_SID_INFO_1		// 76
	,	SONORK_ATOM_SID_INFO_2		// 77
	,	SONORK_ATOM_SID_INFO_3		// 78
	,	SONORK_ATOM_SID_INFO_4		// 79

	,	SONORK_ATOM_LOGOUT_AKN_1	// 80
	,	SONORK_ATOM_LOGOUT_AKN_2	// 81
	,	SONORK_ATOM_LOGOUT_AKN_3	// 82
	,	SONORK_ATOM_LOGOUT_AKN_4	// 83

	,	SONORK_ATOM_SHORT_STRING	=SONORK_SIMPLE_ATOMS_LIMIT // 64
	,	SONORK_ATOM_STRING_RESERVED_1	// 65
	,	SONORK_ATOM_STRING_RESERVED_2	// 66
	,	SONORK_ATOM_STRING_RESERVED_3	// 67

	,	SONORK_ATOM_DYN_STRING			// 68
	,	SONORK_ATOM_DYN_STRING_RESERVED_1	// 69
	,	SONORK_ATOM_DYN_STRING_RESERVED_2	// 70
	,	SONORK_ATOM_DYN_STRING_RESERVED_3	// 71

	,	SONORK_ATOM_TEXT		// 72
	,	SONORK_ATOM_TEXT_RESERVED_1	// 73
	,	SONORK_ATOM_TEXT_RESERVED_2	// 74
	,	SONORK_ATOM_TEXT_RESERVED_3	// 75

	,	SONORK_ATOM_MSG			// 76
	,	SONORK_ATOM_MSG_RESERVED_1	// 77
	,	SONORK_ATOM_MSG_RESERVED_2	// 78
	,	SONORK_ATOM_MSG_RESERVED_3	// 79

	,	SONORK_ATOM_CTRL_MSG		// 80
	,	SONORK_ATOM_CTRL_MSG_RESERVED_1	// 81
	,	SONORK_ATOM_CTRL_MSG_RESERVED_2	// 82
	,	SONORK_ATOM_CTRL_MSG_RESERVED_3	// 83

	,	SONORK_ATOM_ERROR			// 84
	,	SONORK_ATOM_ERROR_RESERVED_1	// 85
	,	SONORK_ATOM_ERROR_RESERVED_2	// 86
	,	SONORK_ATOM_ERROR_RESERVED_3	// 88

	,	SONORK_ATOM_USER_DATA			// 89
	,	SONORK_ATOM_USER_DATA_RESERVED_1	// 90
	,	SONORK_ATOM_USER_DATA_RESERVED_2	// 91
	,	SONORK_ATOM_USER_DATA_RESERVED_3	// 92

	,	SONORK_ATOM_PACKET			// 93
	,	SONORK_ATOM_PACKET_RESERVED_1		// 94
	,	SONORK_ATOM_PACKET_RESERVED_2		// 95
	,	SONORK_ATOM_PACKET_RESERVED_3		// 96
	,	SONORK_ATOM_PACKET_RESERVED_4		// 97

	,	SONORK_ATOM_SERVER_ADDRESS			// 98
	,	SONORK_ATOM_CLIENT_SERVER_PROFILE	// 99

	,	SONORK_ATOM_SERVICE_DATA		// 100
	,	SONORK_ATOM_SERVICE_DATA_RESERVED_1	// 101
	,	SONORK_ATOM_SERVICE_DATA_RESERVED_2	// 102
	,	SONORK_ATOM_SERVICE_DATA_RESERVED_3	// 103

	,	SONORK_ATOM_SONORK_UTS_LOGIN_PACKET_1	// 104
	,	SONORK_ATOM_SONORK_UTS_LOGIN_PACKET_2	// 105
	,	SONORK_ATOM_SONORK_UTS_LOGIN_PACKET_3	// 106
	,	SONORK_ATOM_SONORK_UTS_LOGIN_PACKET_4	// 107

	,	SONORK_ATOM_SONORK_UTS_LOGIN_REQ_1	// 108
	,	SONORK_ATOM_SONORK_UTS_LOGIN_REQ_2	// 109
	,	SONORK_ATOM_SONORK_UTS_LOGIN_REQ_3	// 110

	,	SONORK_ATOM_SONORK_UTS_LOGIN_AKN_1	// 111
	,	SONORK_ATOM_SONORK_UTS_LOGIN_AKN_2	// 112
	,	SONORK_ATOM_SONORK_UTS_LOGIN_AKN_3	// 113

	,	SONORK_ATOM_EXTERNAL_APP_REQ_1	// 114
	,	SONORK_ATOM_EXTERNAL_APP_REQ_2	// 115
	,	SONORK_ATOM_EXTERNAL_APP_REQ_3	// 116

	,	SONORK_ATOM_USER_GROUP_1		// 117
	,	SONORK_ATOM_USER_GROUP_2		// 118
	,	SONORK_ATOM_USER_GROUP_3		// 119

	,	SONORK_ATOM_WAPP_DATA_1			// 120
	,	SONORK_ATOM_WAPP_DATA_2			// 121

	,	SONORK_ATOM_NEWS_DATA_1			// 122
	,	SONORK_ATOM_NEWS_DATA_2			// 123

	,	SONORK_ATOM_SYS_MSG_1			// 124
	,	SONORK_ATOM_SYS_MSG_2			// 125

	,	SONORK_ATOM_EMAIL_ACCOUNT_1		// 126
	,	SONORK_ATOM_EMAIL_ACCOUNT_2		// 127

	,	SONORK_ATOM_AUTH_REQ_DATA_1		// 128
	,	SONORK_ATOM_AUTH_REQ_DATA_2		// 129

	,	SONORK_ATOM_PROFILE_CTRL_DATA_1		// 130
	,	SONORK_ATOM_PROFILE_CTRL_DATA_2		// 131

	,	SONORK_ATOM_APP_CTRL_DATA_1		// 132
	,	SONORK_ATOM_APP_CTRL_DATA_2		// 133

	,	SONORK_ATOM_EMAIL_EXCEPT_ENTRY		// 134
	,	SONORK_ATOM_EMAIL_EXCEPT		// 135

	,	SONORK_ATOM_USER_SERVER_1		// 136
	,	SONORK_ATOM_USER_SERVER_2		// 137

	,	SONORK_ATOM_SID_NOTIFICATION_1		// 138
	,	SONORK_ATOM_SID_NOTIFICATION_2		// 139
	,	SONORK_ATOM_SID_NOTIFICATION_3		// 140

	,	SONORK_ATOM_TRACKER_ROOM_1		// 141
	,	SONORK_ATOM_TRACKER_ROOM_2		// 142

	,	SONORK_ATOM_TRACKER_DATA_1		// 143
	,	SONORK_ATOM_TRACKER_DATA_2		// 144

	,	SONORK_ATOM_TRACKER_MEMBER_1		// 145
	,	SONORK_ATOM_TRACKER_MEMBER_2		// 146

	,	SONORK_SYSTEM_ATOMS_LIMIT	=512    // Limit is exclusive


	,	SONORK_PUBLIC_ATOMS_BASE	=SONORK_SYSTEM_ATOMS_LIMIT
	,	SONORK_ATOM_URL			=SONORK_PUBLIC_ATOMS_BASE // 512
	,	SONORK_ATOM_RAW			// 513
	,	SONORK_ATOM_MTPL_INFO		// 514	String, Message template information
	,	SONORK_ATOM_PAIR_VALUE		// 515
	,	SONORK_ATOM_TASK_DATA		// 516
	,	SONORK_ATOM_REMIND_DATA		// 517
	,	SONORK_PUBLIC_ATOMS_LIMIT	=1535

	// SERVICE SPECIFIC ATOMS
	,	SONORK_SERVICE_ATOMS_BASE	=SONORK_PUBLIC_ATOMS_LIMIT
	,	SONORK_SERVICE_ATOM_001		=SONORK_SERVICE_ATOMS_BASE
	,	SONORK_SERVICE_ATOM_002
	,	SONORK_SERVICE_ATOM_003
	,	SONORK_SERVICE_ATOM_004
	,	SONORK_SERVICE_ATOM_005
	,	SONORK_SERVICE_ATOM_006
	,	SONORK_SERVICE_ATOM_007
	,	SONORK_SERVICE_ATOM_008
	,	SONORK_SERVICE_ATOM_009
	,	SONORK_SERVICE_ATOM_010
	,	SONORK_SERVICE_ATOM_011
	,	SONORK_SERVICE_ATOM_012
	,	SONORK_SERVICE_ATOM_013
	,	SONORK_SERVICE_ATOM_014
	,	SONORK_SERVICE_ATOM_015
	,	SONORK_SERVICE_ATOM_016
	,	SONORK_SERVICE_ATOM_017
	,	SONORK_SERVICE_ATOM_018
	,	SONORK_SERVICE_ATOM_019
	,	SONORK_SERVICE_ATOM_020
	,	SONORK_SERVICE_ATOM_021
	,	SONORK_SERVICE_ATOM_022
	,	SONORK_SERVICE_ATOM_023
	,	SONORK_SERVICE_ATOM_024
	,	SONORK_SERVICE_ATOM_025
	,	SONORK_SERVICE_ATOM_026
	,	SONORK_SERVICE_ATOM_027
	,	SONORK_SERVICE_ATOM_028
	,	SONORK_SERVICE_ATOM_029
	,	SONORK_SERVICE_ATOM_030
	,	SONORK_SERVICE_ATOMS_LIMIT	=2048

	,	SONORK_ATOM_CLIPDATA_01		=SONORK_SERVICE_ATOMS_LIMIT
	,	SONORK_ATOM_CLIPDATA_02
	,	SONORK_ATOM_CLIPDATA_03
	,	SONORK_ATOM_LIMIT
};



// ----------------------------------------------------------------------------
// Function wrappers for each O/S

inline void   SONORK_Sleep(UINT milli_secs)
{
#if defined(SONORK_WIN32_BUILD)
	Sleep(milli_secs);
#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))
	usleep(milli_secs*1000);
#else
#	error no implementation
#endif
}



inline UINT   SONORK_Random(UINT max_value)
{
#if defined(SONORK_WIN32_BUILD)
#	if defined(_MSC_VER)
		return rand()%(max_value+1);
#	else
		return random(max_value+1);
#	endif
#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))
	return random()%(max_value+1);
#else
#	error no implementation
#endif
}


#if defined(SONORK_WIN32_BUILD)
#	define SONORK_StrNoCaseCmp	stricmp
#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))
#	define SONORK_StrNoCaseCmp	strcasecmp
#else
#error NO IMPLEMENTATION
#endif

// ----------------------------------------------------------------------------
// Helper functions

SONORK_C_CSTR	 SONORK_ResultName(SONORK_RESULT   result);

inline bool
  SONORK_IsEmptyStr(const char *str)
{
	return str==NULL?true:(*str==0);
}

inline SONORK_C_STR
  SONORK_CStr(int chars)
{
	return SONORK_MEM_ALLOC(SONORK_C_CHAR,chars*sizeof(SONORK_C_CHAR));
}

inline SONORK_W_STR
  SONORK_WStr(int chars)
{
	return SONORK_MEM_ALLOC(SONORK_W_CHAR,chars*sizeof(SONORK_W_CHAR));
}

// SONORK_CStr() , SONORK_WStr()
// Creates a string and loads the contents.
// The owner should later delete these strings with SONORK_MEM_FREE()
SONORK_C_STR	SONORK_CStr(SONORK_C_CSTR   str);
SONORK_W_STR	SONORK_WStr(SONORK_C_CSTR   str);
SONORK_W_STR	SONORK_WStr(SONORK_W_CSTR   str);
// SONORK_StrStr(): Find target in source (case insensitive)
SONORK_C_CSTR	SONORK_StrStr(SONORK_C_CSTR source,SONORK_C_CSTR target);
inline UINT  SONORK_StrLen(SONORK_C_CSTR	str){ return str?strlen(str):0;}
UINT	SONORK_StrLen(SONORK_W_CSTR	str);
UINT	SONORK_StrCopyCut(SONORK_C_STR target,UINT target_length,SONORK_C_CSTR source);
UINT	SONORK_StrCopy(SONORK_C_STR target,UINT target_length,SONORK_C_CSTR source, UINT source_length=(UINT)-1);
UINT	SONORK_StrCopy(SONORK_C_STR target,UINT target_length,SONORK_W_CSTR source, UINT source_length=(UINT)-1);
UINT	SONORK_StrCopy(SONORK_W_STR target,UINT target_length,SONORK_C_CSTR source, UINT source_length=(UINT)-1);
UINT	SONORK_StrCopy(SONORK_W_STR target,UINT target_length,SONORK_W_CSTR source, UINT source_length=(UINT)-1);
UINT	SONORK_StrTrim(SONORK_C_STR str);
void	SONORK_StrToLower(char *s);
void	SONORK_StrToUpper(char *s);


void	SONORK_EncodePin64(
			 SONORK_DWORD4&			tgtPin
			,const 	TSonorkId& 		srcUserId
			,const 	TSonorkPin& 		srcPin
			,const 	TSonorkId& 		tgtUserId
			,SONORK_SERVICE_ID  		tgtServiceId
			,DWORD				tgtServicePin
			);
DWORD	SONORK_GenTrackingNo(const TSonorkId& sender, const TSonorkId& target);

extern void sonork_printf(const char* fmt,...);


inline void SONORK_ZeroMem(void *ptr, int sz)
{
	memset(ptr,0,sz);
}

// ----------------------------------------------------------------------------
// Sonork File I/O


enum SONORK_OPEN_FILE_METHOD
{
#if defined(SONORK_WIN32_BUILD)
	SONORK_OPEN_FILE_METHOD_CREATE_NEW		=CREATE_NEW
,	SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS	=CREATE_ALWAYS
,	SONORK_OPEN_FILE_METHOD_OPEN_EXISTING	=OPEN_EXISTING
,	SONORK_OPEN_FILE_METHOD_OPEN_ALWAYS		=OPEN_ALWAYS
#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))
	SONORK_OPEN_FILE_METHOD_CREATE_NEW		=0 //"r+"	// crear solo si no existe
,	SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS	=1 //"w+"	// crear siempre
,	SONORK_OPEN_FILE_METHOD_OPEN_EXISTING	=2 //""		// abrir solo si existe
,	SONORK_OPEN_FILE_METHOD_OPEN_ALWAYS		=3 //""		// abrir siempre
#endif
};

enum SONORK_FILE_SEEK_MODE
{
#if defined(SONORK_WIN32_BUILD)
	SONORK_FILE_SEEK_BEGIN		=FILE_BEGIN
,	SONORK_FILE_SEEK_CURRENT	=FILE_CURRENT
,	SONORK_FILE_SEEK_END		=FILE_END
#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))
	SONORK_FILE_SEEK_BEGIN		=SEEK_SET
,	SONORK_FILE_SEEK_CURRENT	=SEEK_CUR
,	SONORK_FILE_SEEK_END		=SEEK_END
#endif
};

enum SONORK_OPEN_FILE_RESULT
{
	SONORK_OPEN_FILE_RESULT_FAILED
,	SONORK_OPEN_FILE_RESULT_NEW
,	SONORK_OPEN_FILE_RESULT_EXISTING
};
enum SONORK_FILE_ATTRIBUTE
{
  SONORK_FILE_ATTR_DIRECTORY	=   0x00000010
, SONORK_FILE_ATTR_READ_ONLY	=   0x00000020
, SONORK_FILE_ATTR_TEMPORARY	=   0x00000040
};

#define SONORK_INVALID_DIR_HANDLE				NULL

enum SONORK_CRYPT_ENGINE
{
	SONORK_CRYPT_ENGINE_NONE
,	SONORK_CRYPT_ENGINE_SIMPLE
,	SONORK_CRYPT_ENGINE_SINGLE_BUFFERED=5000
,	SONORK_CRYPT_ENGINE_RIJNDAEL
};


enum SONORK_UTS_CMD
{
  SONORK_UTS_CMD_NONE
, SONORK_UTS_CMD_MSG
, SONORK_UTS_CMD_CTRL_MSG
, SONORK_UTS_CMD_LIMIT			=4096
};



#endif
