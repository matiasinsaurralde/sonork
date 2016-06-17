#if !defined(SRK_DATA_TYPES_H)
#define SRK_DATA_TYPES_H

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


#if !defined(SONORK_SERVER_BUILD) && !defined(SONORK_CLIENT_BUILD)
#   error MUST define either SONORK_SERVER_BUILD or SONORK_CLIENT_BUILD
#endif

#include "srk_defs.h"
#include "srk_codec_atom.h"
#include "srk_sys_string.h"
#include "srk_data_lists.h"

#if SONORK_CODEC_LEVEL>5



struct TSonorkSimpleDataListAtom
:public TSonorkCodecAtom
{
private:
	SONORK_ATOM_TYPE	type;
	TSonorkSimpleDataList	*DL;
	bool 			force_type_match;
public:

	TSonorkSimpleDataListAtom(TSonorkSimpleDataList	*pDL
		,SONORK_ATOM_TYPE pType
		,bool ftm)
		{DL=pDL;type=pType;force_type_match=ftm;}

	TSonorkSimpleDataListAtom()
		{DL=NULL;type=SONORK_ATOM_NONE;force_type_match=false;}

	void
		SetSource(TSonorkSimpleDataList	*pDL
			,SONORK_ATOM_TYPE pType
			,bool ftm)
		{ DL=pDL;type=pType; force_type_match=ftm;}

// -----------------------
// CODEC

public:

	void
		CODEC_Clear()
		{ DL->Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ return type; }

private:
	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize() const;
};

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,1)
#endif


struct  TSonorkDynString
:public TSonorkCodecAtom
{
private:
	DWORD	flags;
	union  _S
	{
		SONORK_C_STR		c;
		SONORK_W_STR		w;
	}__SONORK_PACKED;
	_S	s;

public:
	TSonorkDynString(){flags=0;s.c=NULL;}
	TSonorkDynString(TSonorkDynString&);
	TSonorkDynString(SONORK_C_CSTR);
	TSonorkDynString(SONORK_W_CSTR);
	~TSonorkDynString(){Clear();}
	void  				Clear();

	UINT
		CharByteSize() 	 const;
	UINT
		Length() const
		{ return flags&SONORK_DYN_STRING_FM_LENGTH;	}

	UINT  	StringByteSize() const
		{ return Length()*CharByteSize();		}

	SONORK_DYN_STRING_TYPE
		Type()	const
		{ return (SONORK_DYN_STRING_TYPE)(flags&SONORK_DYN_STRING_FM_TYPE);}

	bool  IsNull() const
	{
		return s.c==NULL||Type()==SONORK_DYN_STRING_TYPE_NULL;
	}

	bool  IsNullOrCStr() const
	{
		return Type()==SONORK_DYN_STRING_TYPE_NULL||Type()==SONORK_DYN_STRING_TYPE_C;
	}

	UINT  Get(SONORK_C_STR buff,UINT buff_length) const;
	UINT  Get(SONORK_W_STR buff,UINT buff_length) const;
	UINT  Get(SONORK_C_STR*buff) const;
	UINT  Get(SONORK_W_STR*buff) const;

	SONORK_C_CSTR	CStr() const ;
	SONORK_W_CSTR	WStr() const ;

	SONORK_C_CSTR	ToCStr();
	SONORK_W_CSTR	ToWStr();

	void  Set(SONORK_C_CSTR,bool do_a_copy=true);
	void  Set(SONORK_W_CSTR,bool do_a_copy=true);
	void  Set( const TSonorkDynString&);
	bool  operator==(TSonorkDynString&S);
	bool  operator!=(TSonorkDynString&S){ return !(*this==S);}
	void  operator=(SONORK_C_CSTR		str)	{ Set(str,true);}
	void  operator=(SONORK_W_CSTR		str)	{ Set(str,true);}
	void  operator=(const TSonorkDynString&	str)	{ Set(str);}
	bool  operator==(SONORK_C_CSTR      str);
	bool  operator!=(SONORK_C_CSTR      str){ return !(*this==str); }

	void
		Append(SONORK_C_CSTR str1,SONORK_C_CSTR str2=NULL);
	void
		CutAt(UINT);

// -----------------------
// CODEC

public:
	void
		CODEC_Clear()
		{	Clear();	}

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ return SONORK_ATOM_DYN_STRING; }

	char*
		CODEC_GetTextView(char*buffer,DWORD size) const;

private:
	void
		CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const
		{
			return sizeof(DWORD)+StringByteSize();
		}

} __SONORK_PACKED;

struct TSonorkSidNotificationAtom
:public TSonorkCodecAtom
{
	struct HEADER{
		TSonorkVersion		version;
		TSonorkSerial		user_serial;
		DWORD			reserved[4];
	}__SONORK_PACKED;


	HEADER			header;
	TSonorkUserLocus3	locus;
	TSonorkDynString      	text;

	void	Clear();

// -----------------------
// CODEC
public:
	void
		CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType() const
		{ return SONORK_ATOM_SID_NOTIFICATION_1; }

private:
	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize() const;


} ;


class  TSonorkDynData
:public TSonorkCodecAtom
{
	UINT 			buffer_size;
	UINT 			block_size;

	SONORK_ATOM_TYPE	data_type;
	UINT 			data_size;
	BYTE *			buffer;

public:
	TSonorkDynData( SONORK_ATOM_TYPE data_type=SONORK_ATOM_NONE );
	~TSonorkDynData(){Clear();}
	void SetDataType(SONORK_ATOM_TYPE p_data_type){data_type=p_data_type;}


	// SetBufferSize() modifies buffer_size> and adjusts the buffer.
	//  If <preserve_data> is true, all data (up to <buffer_size>) is preserved
	//  and <data_size> is adjusted if the new <buffer_size> is less than the
	//  value of <data_size> when the method was invoked.
	//  If <preserve_data> is false, all data is discarded and <data_size>
	//  is reset to zero.
	void SetBufferSize(UINT size,bool preserve_data);

	// SetDataSize() modifies the <data_size>
	//  and if <adjust_buffer> is true, it also sets <buffer_size> and <buffer>
	//  to match <data_size>. If the buffer is adjusted (adjust_buffer is true)
	//  the current buffer contents are discarded.
	// If <adjust_buffer> is false, the caller is responsible for ensuring
	//  that the new <data_size> does not exceed <buffer_size>: the situation
	//  where <data_size> is larger than <buffer_size> is an error.
	void SetDataSize(UINT size, bool adjust_buffer);
	void SetBlockSize(UINT size);
	void Set(const TSonorkDynData&O);
	void Clear();
	void Append(const void*,UINT size);

const	BYTE
		*Buffer()	const
		{ return buffer;}
	BYTE
		*wBuffer()
		{ return buffer;}
	BOOL
		IsNull() const
		{ return data_size==0 && data_type == SONORK_ATOM_NONE;}

	UINT 	DataSize()	const
		{ return data_size;}

	SONORK_ATOM_TYPE
		Type()		const
		{ return data_type; }

	SONORK_C_CSTR
		CStr()
		{ return buffer!=NULL?(SONORK_C_CSTR)buffer:"";}

	UINT 	BlockSize()	 const
		{ return block_size;}

	UINT
		BufferSize() const
		{ return buffer_size;}
	void
		SetStr(SONORK_C_CSTR);

	void
		AppendStr(SONORK_C_CSTR, BOOL append_null);

	void
		SetDWN(const DWORD*dword_ptr,UINT dword_count);

	void
		SetRaw(const void*,UINT size,SONORK_ATOM_TYPE type);


	void
		Assign(BYTE*,UINT);

	UINT
		GenerateSignature();

// -----------------------
// CODEC

public:
	void
		CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType() const
		{ return Type(); }

private:
	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize() const
		{ return DataSize(); }


} __SONORK_PACKED;



struct  TSonorkText
:public TSonorkCodecAtom
{

public:
	struct  HEADER {
		TSonorkRegion	region;
		DWORD		flags;
	}__SONORK_PACKED;
	HEADER			header;
private:
	TSonorkDynString	string;
public:

const 	DWORD
		Flags()	 const
		{ return header.flags;}
const	TSonorkRegion&
		Region() const
		{ return header.region; }

const	TSonorkDynString&
		String() const
			{ return string;}

	TSonorkDynString&
		wString()
		{ return string;}

	TSonorkRegion&
		wRegion() 		{ return header.region; }

	void
		SetRegion(const	TSonorkRegion& R)
		{ header.region.Set(R);}

	void
		SetFlags(DWORD f)
		{ header.flags=f;}

	void
		SetStr(DWORD flags,SONORK_C_CSTR str);

	void
		SetString(DWORD flags,const TSonorkDynString& S);

	UINT
		Length()	const
		{ return string.Length();}

	SONORK_C_CSTR
		CStr() 	 	const
		{ return string.CStr();}

	SONORK_C_CSTR
		ToCStr()
		{ return string.ToCStr();}

	void
		Clear();
		
	void
		Set(const TSonorkText&T);



// -----------------------
// CODEC

public:
	void 	CODEC_Clear()
		{Clear();}

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ return SONORK_ATOM_TEXT; }

	char* 	CODEC_GetTextView(char*buffer,DWORD size) const
		{ return string.CODEC_GetTextView(buffer,size);}

private:

	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void	CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD	CODEC_DataSize() const;

} __SONORK_PACKED;

#define SONORK_MSG_VERSION		1
struct  TSonorkMsg
:public TSonorkCodecAtom
{
	TSonorkMsgHeader	header;
	TSonorkText		text;
	TSonorkDynData		data;

	TSonorkMsg();

const 	TSonorkId&
		UserId() const
		{ return header.userId;}

	TSonorkId&
		wUserId()
		{ return header.userId;}

	TSonorkMsgHeader&
		MsgHeader()
		{ return header;}

const	TSonorkTime&
		Time() const
		{ return header.time;}

	TSonorkTime&
		wTime()
		{ return header.time;}

	DWORD
		SysFlags()  const
		{ return header.sysFlags;}

	DWORD
		UsrFlags()	const
		{ return header.usrFlags;}

const	TSonorkMsgTarget&
		Target() const
		{ return header.target; }

	TSonorkMsgTarget&
		wTarget()
		{ return header.target; }


const	TSonorkServiceInfo&
		DataServiceInfo() 	const
		{ return header.DataServiceInfo();}

	TSonorkServiceInfo&
		wDataServiceInfo()
		{ return header.wDataServiceInfo();}

	SONORK_SERVICE_ID
		DataServiceId() const
		{ return header.DataServiceId();}

	DWORD
		DataServiceDescriptor()	const
		{ return header.DataServiceDescriptor();	}

	SONORK_SERVICE_TYPE
		DataServiceType()	const
		{ return header.DataServiceType();}
	DWORD
		DataServiceHFlags()	const
		{ return header.DataServiceHFlags();}

	void
		SetDataServiceInfo(SONORK_SERVICE_ID psi,SONORK_SERVICE_TYPE pst,DWORD psf)
		{	header.SetDataServiceInfo(psi,pst,psf); }

	SONORK_DWORD2&
		TrackingNo()
		{ return header.trackingNo;}

	TSonorkText&
		Text()
		{ return text;}

	TSonorkText*
		TextPtr()
		{ return &text;}

	UINT	TextLength() const
		{ return text.Length();}

	TSonorkDynData&
		ExtData()
		{ return data;}

	TSonorkDynData*
		ExtDataPtr()
		{ return &data;}

	UINT
		ExtDataSize() const
		{ return data.DataSize();}
		
	DWORD
		ExtDataType() const
		{ return data.Type(); }

	SONORK_C_CSTR
		CStr() 	 	const
		{ return text.CStr();}

	SONORK_C_CSTR
		ToCStr()
		{ return text.ToCStr();}

	void
		SetStr(DWORD flags,SONORK_C_CSTR str)
		{ text.SetStr(flags,str);}

	void
		SetString(DWORD flags,const TSonorkDynString& S)
														{ text.SetString(flags,S);}

	void
		ClearText()
		{text.Clear();}

	void
		ClearData()
		{data.Clear();}

	void
		Clear();

// -----------------------
// CODEC

public:

	void 	CODEC_Clear()
		{Clear();}

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{	return SONORK_ATOM_MSG;		}

	char*
		CODEC_GetTextView(char*buffer,DWORD size) const
		{ return text.CODEC_GetTextView(buffer,size);}
private:

	void
		CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize() const;

};

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif

struct  TSonorkError
:public TSonorkCodecAtom
{
public:
	SONORK_RESULT		result;
	DWORD			code;
	TSonorkShortString 	text;
	bool			local;

	const char      *
		ResultName() const;

	SONORK_RESULT
		SetResult(SONORK_RESULT r)
		{return (result=r);}

	SONORK_RESULT
		Result() const
		{ return result;}

	DWORD
		SetCode(DWORD c)
		{return (code=c);}

	SONORK_RESULT
		Set(const TSonorkError&O);

	SONORK_RESULT
		SetOk();

	DWORD	Code() const
		{ return code;}

const	TSonorkShortString&
		Text()		const
		{ return text;}

	TSonorkShortString&
		wText()
		{ return text;}

	bool	IsLocal()	const
		{ return local;}

	void	SetLocal(bool v)
		{ local=v;}

#if defined(SONORK_SERVER_BUILD)
	SONORK_RESULT
		Set(SONORK_RESULT,SONORK_C_CSTR,DWORD code);
#else
	SONORK_RESULT
		Set(SONORK_RESULT,SONORK_C_CSTR,DWORD code,bool local);
#endif

#if !defined(SONORK_IPC_BUILD)
	void
		SetSys(SONORK_RESULT,SONORK_SYS_STRING str_index,DWORD code);
	void
		SetSysExtInfo(SONORK_SYS_STRING str_index,DWORD code);
#endif

	void
		SetExtendedInfo(SONORK_C_CSTR,DWORD code);

// -----------------------
// CODEC

public:

	void
		CODEC_Clear()
		{SetOk();}

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ return SONORK_ATOM_ERROR; }

	char* 	CODEC_GetTextView(char*buffer,DWORD size) 	const;

private:

	void
		CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const;

};

struct  TSonorkErrorOk
:public TSonorkError
{
	TSonorkErrorOk()
	:TSonorkError()
	{
		SetOk();
	}
};

struct  TSonorkErrorInfo
:public TSonorkError
{
#if defined(SONORK_SERVER_BUILD)
	TSonorkErrorInfo(SONORK_RESULT p_result
	, const char*p_msg
	, DWORD p_code):TSonorkError()
	{
		Set(p_result,p_msg,p_code);
	}
#else
	TSonorkErrorInfo(SONORK_RESULT p_result
	, const char*p_msg
	, DWORD p_code
	, bool p_is_local):TSonorkError()
	{
		Set(p_result,p_msg,p_code,p_is_local);
	}
#endif
};

class  TSonorkUserData
:public TSonorkCodecAtom
{

private:
	SONORK_USER_INFO_LEVEL	info_level;
	TSonorkUserInfo3*	user_info;

public:
	TSonorkId		userId;
	TSonorkShortString	alias;
	TSonorkShortString	name;
	TSonorkShortString	email;
	TSonorkNewUserAddr	addr;
#if defined(SONORK_CLIENT_BUILD)
	TSonorkUserData	(SONORK_USER_INFO_LEVEL level=SONORK_USER_INFO_LEVEL_1);
#else
	TSonorkUserData	();
#endif
	virtual ~TSonorkUserData();

	void	Clear(bool reset_info_level=true);

const	TSonorkId&
		UserId()	const
		{ return userId;}

	TSonorkId&
		wUserId()
		{ return userId;}


	bool	IsSystemUser() const
		{ return userId.IsSystemUser();}

const 	TSonorkRegion&
		Region() 	const
		{ return user_info->region;}

	TSonorkRegion&
		wRegion()
		{ return user_info->region;}

const 	TSonorkSerial&
		Serial() 	const
		{ return user_info->serial;}
		
	TSonorkSerial&
		wSerial()
		{ return user_info->serial;}

const 	TSonorkUserInfoFlags&
		InfoFlags() const
		{ return user_info->infoFlags;}

	TSonorkUserInfoFlags&
		wInfoFlags()
		{ return user_info->infoFlags;}

	DWORD	PublicAuthPin()	const
		{ return user_info->pubAuthPin;}

const 	TSonorkAuthFlags&
		PublicAuthFlags() const
		{ return user_info->pubAuthFlags;}
	TSonorkAuthFlags&
		wPublicAuthFlags()
		{ return user_info->pubAuthFlags;}

const	TSonorkTime&
		BornTime()	const
		{ return user_info->bornTime;}

	TSonorkTime&
		wBornTime()
		{ return user_info->bornTime;}

	BOOL
		IsAdministrator() const
		{ return user_info->IsAdministrator(); }

	BOOL
		IsRobot() const
		{ return user_info->IsRobot();	}

	BOOL
		IsBroadcaster() const
		{ return user_info->IsBroadcaster();	}

const	TSonorkShortString&
		Alias()		const
		{ return alias;		}

	TSonorkShortString&
		wAlias()
		{ return alias;		}

const 	TSonorkShortString&
		Name()		const
		{ return name;		}

	TSonorkShortString&
		wName()
		{ return name;		}

const	TSonorkShortString&
		Email()		const
		{ return email;		}

	TSonorkShortString&
		wEmail()
		{ return email;		}

const 	TSonorkNewUserAddr&
		Address()	const
		{ return addr;		}

	TSonorkNewUserAddr&
		wAddress()
		{ return addr;		}

	SONORK_SID_MODE
		SidMode()	const
		{ return addr.SidMode();}

	BOOL
		IsOnline()	const
		{ return addr.sidFlags.IsOnline();}

	BOOL
		IsOffline()	const
		{ return addr.sidFlags.IsOffline();}

const TSonorkSid&
		Sid()		const
		{ return addr.sid;}

	TSonorkSid&
		wSid()
		{ return addr.sid;}

const TSonorkSidFlags&
		SidFlags()	const
		{ return addr.sidFlags;}

	TSonorkSidFlags&
		wSidFlags()
		{ return addr.sidFlags;}

	BOOL
		TrackerEnabled() const
		{ return addr.sidFlags.TrackerEnabled(); }

const	TSonorkVersion&
		SidVersion() const
		{ return addr.version;}

	TSonorkVersion&
		wSidVersion()
		{ return addr.version;}

	void
		Set(const TSonorkUserData&);

	void
		Set(const TSonorkUserInfo1*,SONORK_USER_INFO_LEVEL);

	// Copies user info from other into ours, without modifying our size:
	//  If the <other_level> is larger that <this->info_level>,
	//    we copy the bytes given by <this->info_level>.
	//  If <this->info_level> is larger that <other_level>,
	//    it copies the bytes given by <other_level>
	void
		CopyUserInfo(const TSonorkUserInfo1*,SONORK_USER_INFO_LEVEL);

	void
		CopyUserInfo(const TSonorkUserData* UD)
		{ CopyUserInfo(UD->UserInfo() , UD->GetUserInfoLevel() ); }

	SONORK_USER_INFO_LEVEL
		GetUserInfoLevel()	const
		{return info_level; }

	static  DWORD
		GetUserInfoSize(SONORK_USER_INFO_LEVEL level, BOOL for_codec) ;

	DWORD
		GetUserInfoSize(BOOL for_codec) const
		{ return GetUserInfoSize(info_level,for_codec); }

	void
		SetUserInfoLevel(SONORK_USER_INFO_LEVEL l, bool preserve_data);

const 	TSonorkUserInfo3*
		UserInfo()	const
		{return user_info;}

	TSonorkUserInfo3*
		wUserInfo()
		{return user_info;}

	void
		GetLocus1(TSonorkUserLocus1*) const;

	void
		GetLocus2(TSonorkUserLocus2*) const;

	void
		GetLocus3(TSonorkUserLocus3*) const;

virtual void
		GetUserHandle(TSonorkUserHandle*) const;

	void
		ClearUnusedFields();

	char*
		CODEC_GetTextView(char*buffer,DWORD size) const;

// -----------------------
// CODEC

public:

	void 	CODEC_Clear()
		{Clear();}

	SONORK_ATOM_TYPE
		CODEC_DataType() 	const
		{return SONORK_ATOM_USER_DATA;}

private:
	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const;
};

struct TSonorkUserDataNotes
{
	TSonorkUserData		user_data;
	TSonorkDynString	notes;
	TSonorkAuth2		auth;
	TSonorkShortString	password;

	void
		Clear();
	bool
		IsSystemUser() const
		{ return user_data.userId.IsSystemUser();}

	DWORD
		CODEC_Size(DWORD codec_flags) const;
};

typedef TSonorkUserData* 	TSonorkUserDataPtr;
typedef TSonorkUserDataNotes*   TSonorkUserDataNotesPtr;


struct  TSonorkServiceData
:public TSonorkCodecAtom
{
public:
	// V1.04.06: header was duplicated because it
	//  was wrongfully "__SONORK_PACKET" instead of "__SONORK_PACKED"

	TSonorkDynServerHeader		header;
	TSonorkPhysAddr			physAddr;
	TSonorkShortString		notes;

	void
		Set(const TSonorkServiceData&O);

	void
		Clear();

// -----------------------
// From TSonorkDynServerHeader
// -----------------------

const	TSonorkId&
		UserId() const
		{	return header.locus.userId;}

	TSonorkId&
		wUserId()
		{	return header.locus.userId;}

const	TSonorkSid&
		Sid()  const
		{	return header.locus.sid;}

	TSonorkSid&
		wSid()
		{	return header.locus.sid;}

const   TSonorkServiceInstanceInfo&
		ServiceInstanceInfo() const
		{ return header.ServiceInstanceInfo(); }

	TSonorkServiceInstanceInfo&
		wServiceInstanceInfo()
		{ return header.wServiceInstanceInfo(); }
		
	SONORK_SERVICE_ID
		ServiceId()	const
		{ return header.ServiceId();	}

	SONORK_SERVICE_TYPE
		ServiceType()	const
		{ return header.ServiceType();	}

	DWORD
		ServiceDescriptor()	const
		{ return header.ServiceDescriptor();	}

	DWORD
		ServiceInstance()	const
		{ return header.ServiceInstance();}

	DWORD
		ServiceVersionNumber()	const
		{ return header.ServiceVersionNumber(); }

	BOOL
		IsSystemWideService() const
		{ return header.IsSystemWideService();}

	BOOL
		IsLocatorService() const
		{ return header.IsLocatorService(); }

	SONORK_SERVICE_PRIORITY
		ServicePriority() const
		{ return header.ServicePriority();}

	SONORK_SERVICE_STATE
		ServiceState() const
		{ return header.ServiceState();}

	void
		SetInstanceInfo(
			  SONORK_SERVICE_ID	service_id
			, SONORK_SERVICE_TYPE 	service_type
			, DWORD 		service_hflags
			, DWORD 		server_instance
			, DWORD 		server_version
			)
		{
			header.SetInstanceInfo(service_id
				, service_type
				, service_hflags
				, server_instance
				, server_version);
		}

	void
		SetInstanceInfo( const TSonorkServiceInstanceInfo& O )
		{
			header.SetInstanceInfo( O );
		}

	void
		SetInfo(SONORK_SERVICE_ID si,SONORK_SERVICE_TYPE st,DWORD sf)
		{
			header.SetInfo(si,st,sf);
		}

	void	SetInstance(DWORD instance,DWORD version)
		{
			header.SetInstance(instance,version);
		}


	void
		SetState(SONORK_SERVICE_STATE s,SONORK_SERVICE_PRIORITY p=SONORK_SERVICE_PRIORITY_NORMAL)
		{
			header.SetState(s,p);
		}


// -----------------------
// CODEC

public:
	void	CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_SERVICE_DATA; }

private:

	void	CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void	CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD	CODEC_DataSize()	const 	;

}__SONORK_PACKED;


struct TSonorkGroup
:public TSonorkCodecAtom
{
	struct HEADER
	{
		DWORD	group_no;
		DWORD	parent_no;
		DWORD	depth;
		DWORD	group_type;
		DWORD	reserved[2];
	}__SONORK_PACKED;


	HEADER			header;
	TSonorkDynString	name;

	void
		Set(const TSonorkGroup&O);
		
	void
		Clear();

	DWORD
		GroupNo() const
		{ return header.group_no; 		}

	DWORD
		ParentNo() const
		{ return header.parent_no; 	}

	SONORK_GROUP_TYPE
		GroupType() const
		{ return (SONORK_GROUP_TYPE)header.group_type;}

// -----------------------
// CODEC

public:
	void
		CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_USER_GROUP_1; 	}

private:

	void
		CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;
};


struct TSonorkWappData
:public TSonorkCodecAtom
{
public:
	struct HEADER
	{
		DWORD	app_id;
		DWORD	group_no;
		DWORD	app_type;
		DWORD	app_flags;
		DWORD	url_id;
		DWORD	reserved[3];
	}__SONORK_PACKED;
	HEADER			header;
	TSonorkShortString	name;

	void	Clear();

	void	Set(const TSonorkWappData&);


	DWORD
		AppId2() const	// used to be app_no
		{ return header.app_id;}
	DWORD
		UrlId() const   // used to be app_id
		{ return header.url_id;}

	DWORD
		AppFlags() const
		{ return header.app_flags;}

	SONORK_WAPP_TYPE
		AppType() const
		{ return (SONORK_WAPP_TYPE)(header.app_type&0xffff);}

	SONORK_C_CSTR
		AppName() const
		{ return name.CStr();}
		
	DWORD
		GroupNo() const
		{ return header.group_no; }

// -----------------------
// CODEC

public:

	void
		CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_WAPP_DATA_1; 		}


private:
	void
		CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;
};


struct TSonorkSysMsg
:public TSonorkCodecAtom
{
	struct HEADER
	{
		DWORD		id;
		DWORD		flags;
		TSonorkTime	time;
		DWORD		reserved[4];
	}__SONORK_PACKED;


	HEADER			header;
	TSonorkText		text;

	void	Clear();


	DWORD			MsgId()		const	{ return header.id; 			}
	DWORD			MsgFlags()	const	{ return header.flags;			}
	const TSonorkTime&	Time()		const	{ return header.time;				}
	TSonorkTime&		wTime()			{ return header.time;				}


// -----------------------
// CODEC

public:

	void	CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_SYS_MSG_1; 	}

private:

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;
};


// ----------------------------------------------------------------------------

struct  TSonorkServerAddr
:public TSonorkCodecAtom
{
	TSonorkShortString		host;
	struct {
		DWORD	tcp_port;
		DWORD	udp_port;
		DWORD	xx1_port;
		DWORD	xx2_port;
		DWORD	addr_type;  // SONORK_PHYS_ADDR_TCP_1/SONORK_PHYS_ADDR_UDP_1
		DWORD	param1;
		DWORD	param2;
		DWORD	flags;		// Low bit to 0 to disable
		DWORD	reserved[2];
	}ctrl_data;
	void
		Clear();

	BOOL
		IsEnabled()	const
		{	return ctrl_data.flags&1; 	}

	void
		Enable()
		{	ctrl_data.flags|=1;		}

	void	Disable()
		{	ctrl_data.flags&=~1;		}

	SONORK_C_CSTR
		HostName()	const
		{	return host.CStr();	}

	TSonorkShortString&
		Host()
		{	return host;	}

	void
		SetTcpPort(DWORD port)
		{	ctrl_data.tcp_port = port;	}

	void
		SetUdpPort(DWORD port)
		{	ctrl_data.udp_port = port;	}
	DWORD
		TcpPort()	const
		{       return ctrl_data.tcp_port;	}

	DWORD
		UdpPort()	const
		{      	return ctrl_data.udp_port;	}

	SONORK_PHYS_ADDR_TYPE
		AddrType() const
		{	return	(SONORK_PHYS_ADDR_TYPE)ctrl_data.addr_type;}

	void	SetAddrType(SONORK_PHYS_ADDR_TYPE type)
		{	ctrl_data.addr_type = type;	}

	void
		SetDefaultPort(DWORD port)
		{
			if(ctrl_data.addr_type == SONORK_PHYS_ADDR_TCP_1)
				ctrl_data.tcp_port=port;
			else
				ctrl_data.udp_port=port;
		}
	DWORD
		DefaultPort()	const
		{
			return ctrl_data.addr_type == SONORK_PHYS_ADDR_TCP_1
					?ctrl_data.tcp_port
					:ctrl_data.udp_port;
		}

// -----------------------
// CODEC

public:

	void
		CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_SERVER_ADDRESS; 	}
private:

	void
		CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;

};


// ----------------------------------------------------------------------------

struct 	TSonorkClientServerProfile
:public TSonorkCodecAtom
{
	struct NAT
	{
		DWORD		flags;
		SONORK_DWORD2	range;
	};

	TSonorkServerAddr	sonork;
	TSonorkServerAddr	socks;
	TSonorkServerAddr	reserved;
	NAT			nat;

	void Clear();

// -----------------------
// CODEC

public:

	void	CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_CLIENT_SERVER_PROFILE; 	}
private:

	void
		CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;

};


// ----------------------------------------------------------------------------

struct  TSonorkAuthReqData
:public TSonorkCodecAtom
{
	struct HEADER
	{
		TSonorkId		requestor_id;
		TSonorkTime		time;
		TSonorkAuth2		auth;
	}__SONORK_PACKED;

	HEADER			header;
	TSonorkUserData		user_data;
	TSonorkDynString	notes;

	const TSonorkId&
			RequestorId()	const
			{ return header.requestor_id;}

	TSonorkId&
			wRequestorId()
			{ return header.requestor_id;}

	const TSonorkTime&
			RequestTime()	const
			{ return header.time;}

	TSonorkTime&
			wRequestTime()
			{ return header.time;}


	const TSonorkAuth2&
			RequestAuth()	const
			{ return header.auth;}

	TSonorkAuth2&
			wRequestAuth()
			{ return header.auth;}

//----------
// CODEC

public:


	void	CODEC_Clear();

	SONORK_ATOM_TYPE
			CODEC_DataType()	const
			{ 	return SONORK_ATOM_AUTH_REQ_DATA_1; 	}

private:

	void
			CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
			CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
			CODEC_DataSize()	const 	;

};

// ----------------------------------------------------------------------------

struct  TSonorkAppCtrlData
:public TSonorkCodecAtom
{
	struct HEADER
	{
		TSonorkFlags	  flags;
		DWORD             value[SONORK_APP_CTRL_VALUES];
		SONORK_DWORD2	  reserved;
	}__SONORK_PACKED;

	HEADER			header;

//----------
// CODEC

public:


	void	CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_APP_CTRL_DATA_1; 	}

private:

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;


};

// ----------------------------------------------------------------------------

struct  TSonorkProfileCtrlData
:public TSonorkCodecAtom
{
	enum CODEC_FLAGS
	{
		CODEC_F_HAS_SERVER_PROFILE	=0x00001000
	,	CODEC_M_VERSION                 =0x000f0000
	,	CODEC_V_VERSION_V10400		=0x00000000
	,	CODEC_V_VERSION_V10500		=0x00010000
	};

	struct HEADER
	{
		TSonorkFlags	flags;
		TSonorkSidFlags	sidFlags;
		TSonorkTime	lastSysMsgTime;
		DWORD		value[SONORK_PROFILE_V1500_CTRL_VALUES];
	}__SONORK_PACKED;


	HEADER 			header;
	TSonorkShortString      serverProfile;

//----------
// CODEC

public:

	const TSonorkFlags&
		Flags()		const
		{	return header.flags;	}

	TSonorkFlags&
		wFlags()
		{	return header.flags;	}

	const TSonorkSidFlags&
		SidFlags()	const
		{	return header.sidFlags; }

	TSonorkSidFlags&
		wSidFlags()
		{	return header.sidFlags; }

	const TSonorkTime&
		LastSysMsgTime() const
		{	return header.lastSysMsgTime; }

	TSonorkTime&
		wLastSysMsgTime()
		{	return header.lastSysMsgTime; }


	void
		CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_PROFILE_CTRL_DATA_1; 	}

private:

	void
		CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;


};

// ----------------------------------------------------------------------------


struct  TSonorkUserServer
:public TSonorkCodecAtom
{
	TSonorkDynServerHeader	header;
	TSonorkPhysAddr		physAddr;
	TSonorkDynString	name;
	TSonorkDynString	text;
	TSonorkDynData		data;


	void
		Clear();

// -----------------------
// From TSonorkDynServerHeader
// -----------------------

const	TSonorkId&
		UserId() const
		{ return header.locus.userId;}

	TSonorkId&
		wUserId()
		{ return header.locus.userId;}

const	TSonorkSid&
		Sid()  const
		{ return header.locus.sid;}

	TSonorkSid&
		wSid()
		{ return header.locus.sid;}

const   TSonorkServiceInstanceInfo&
		ServiceInstanceInfo() const
		{ return header.ServiceInstanceInfo(); }

	TSonorkServiceInstanceInfo&
		wServiceInstanceInfo()
		{ return header.wServiceInstanceInfo(); }

	SONORK_SERVICE_TYPE
		ServiceType()	const
		{ return header.ServiceType();	}

	DWORD
		ServiceInstance() const
		{ return header.ServiceInstance();	}

	SONORK_SERVICE_ID
		ServiceId()	const
		{ return header.ServiceId();	}

	DWORD
		ServiceVersionNumber()	const
		{ return header.ServiceVersionNumber(); }

	DWORD
		ServiceDescriptor()	const
		{ return header.ServiceDescriptor();	}

	SONORK_SERVICE_STATE
		ServiceState() const
		{ return header.ServiceState();}

	SONORK_SERVICE_PRIORITY
		ServicePriority() const
		{ return header.ServicePriority();}

	void
		SetInstanceInfo(
			  SONORK_SERVICE_ID	service_id
			, SONORK_SERVICE_TYPE 	service_type
			, DWORD 		service_hflags
			, DWORD 		server_instance
			, DWORD 		server_version
			)
		{
			header.SetInstanceInfo(service_id
				, service_type
				, service_hflags
				, server_instance
				, server_version);
		}
		
	void
		SetInstanceInfo( const TSonorkServiceInstanceInfo& O )
		{
			header.SetInstanceInfo( O );
		}


	void
		SetInfo(SONORK_SERVICE_ID si,SONORK_SERVICE_TYPE st,DWORD sf)
		{ header.SetInfo(si,st,sf); }

	void	SetInstance(DWORD instance,DWORD version)
		{ header.SetInstance(instance,version);	}

	void
		SetState(SONORK_SERVICE_STATE s,SONORK_SERVICE_PRIORITY p=SONORK_SERVICE_PRIORITY_NORMAL)
		{ header.SetState(s,p); }


const	TSonorkServiceInstanceInfo&
		InstanceInfo() const
		{ return header.locus.info; }

	TSonorkServiceInstanceInfo&
		wInstanceInfo()
		{ return header.locus.info; }

// -----------------------
const	TSonorkRegion&
		Region() const
		{ return header.user.region; }

	TSonorkRegion&
		wRegion()
		{ 	return header.user.region; }

	DWORD	UtsFlags() const
		{	return header.user.utsFlags; }

	DWORD	UtsInstance() const	// Same as ServerNo()
		{	return ServiceInstance(); }

	void
		SetUtsFlags( DWORD utsFlags )
		{	header.user.utsFlags = utsFlags;	}

	DWORD
		Param(UINT i) const
		{ return header.user.data[i]; }

	void	SetParam(UINT i, DWORD v)
		{
			header.user.data[i]=v;
		}

// -----------------------

	void
		CODEC_Clear()
		{	Clear();	}

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_USER_SERVER_1; 	}

private:

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;


};

// ----------------------------------------------------------------------------

struct  TSonorkTrackerRoom
:public TSonorkCodecAtom
{
	TSonorkTrackerHeader	header;
	TSonorkDynString	name; // SONORK_TRACKER_ROOM_NAME_MAX_SIZE
	TSonorkDynString	text; // SONORK_TRACKER_ROOM_TEXT_MAX_SIZE
	TSonorkDynString	data; // SONORK_TRACKER_ROOM_DATA_MAX_SIZE

	BOOL
		IsActive() const
		{ return header.IsActive(); }

	BOOL
		IsAppDetector() const
		{ return header.IsAppDetector(); }

	BOOL
		IsHelper() const
		{ return header.IsHelper();}

	DWORD
		HelpLevel() const
		{ return header.HelpLevel(); }

	void
		SetHelpLevel(DWORD v)
		{ header.SetHelpLevel(v);}

//----------
// CODEC

public:
	void
		Clear();

	void
		CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_TRACKER_ROOM_1; 	}

private:

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;

};

// ----------------------------------------------------------------------------

struct  TSonorkTrackerData
:public TSonorkCodecAtom
{
	TSonorkTrackerHeader	header;
	TSonorkDynString	text; 	// SONORK_TRACKER_DATA_TEXT_MAX_SIZE
	TSonorkDynString	data;   // SONORK_TRACKER_DATA_DATA_MAX_SIZE

	BOOL
		IsActive() const
		{ return header.IsActive(); }

	BOOL
		IsAppDetector() const
		{ return header.IsAppDetector(); }

	BOOL
		IsHelper() const
		{ return header.IsHelper();}

	DWORD
		HelpLevel() const
		{ return header.HelpLevel(); }

	void
		SetHelpLevel(DWORD v)
		{ header.SetHelpLevel(v);}

//----------
// CODEC

public:
	void
		Clear();

	void
		CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_TRACKER_DATA_1; 	}

private:

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;

};

// ----------------------------------------------------------------------------

struct  TSonorkTrackerMember
:public TSonorkCodecAtom
{
	TSonorkUserData		user_data;
	TSonorkTrackerData	track_data;


	BOOL
		IsActive() const
		{ return track_data.IsActive(); }

	BOOL
		IsAppDetector() const
		{ return track_data.IsAppDetector(); }

	BOOL
		IsHelper() const
		{ return track_data.IsHelper();}

	DWORD
		HelpLevel() const
		{ return track_data.HelpLevel(); }


	void
		Set(const TSonorkTrackerHeader& O)
		{ memcpy(this,&O,sizeof(*this)); }

//----------
// CODEC

public:
	void
		Clear();

	void
		CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_TRACKER_MEMBER_1; 	}

private:

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;

};

// ----------------------------------------------------------------------------

class TSonorkAtomQueue
:public TSonorkQueue
{
public:
	~TSonorkAtomQueue()
	{
		Clear();
	}
	TSonorkCodecAtom*
		RemoveFirstAtom()
			{ return	(TSonorkCodecAtom*)w_RemoveFirst(); }

	bool
		RemoveAtom(TSonorkCodecAtom*I)
			{ return (TSonorkCodecAtom*)w_Remove(I);}

	bool
		AddAtom(TSonorkCodecAtom* A,SONORK_QUEUE_ITEM_PRIORITY priority=SONORK_QUEUE_PRIORITY_NORMAL)
			{ 	return w_Add(A,priority);}

	TSonorkCodecAtom*
		EnumNextAtom(TSonorkListIterator&i)	const
			{ return (TSonorkCodecAtom*)w_EnumNext(i);}

	TSonorkCodecAtom*
		PeekAtom() const
			{ return (TSonorkCodecAtom*)w_Peek();}

	TSonorkCodecAtom*
		PeekAtomNo(UINT no) const
			{ return (TSonorkCodecAtom*)w_PeekNo(no);}

	DWORD
		SumAtomsCodecSize() const;

	void
		Clear();
};


#define StartSonorkAtomQueueClass(queue_name,type)\
class queue_name \
:public TSonorkAtomQueue{\
public:\
	bool	Add(type* I,SONORK_QUEUE_ITEM_PRIORITY priority=SONORK_QUEUE_PRIORITY_NORMAL)\
				{ return w_Add(I,priority);}\
	type*	Peek() const \
				{ return (type*)w_Peek();}\
	type*	PeekNo(UINT no) const \
				{ return (type*)w_PeekNo(no);}\
	type* 	RemoveFirst()\
				{return (type*)w_RemoveFirst();}\
	bool  	Remove(type*I)\
				{return (type*)w_Remove(I);}\
	type*	EnumNext(TSonorkListIterator&i)	const { return (type*)w_EnumNext(i);}

#define EndSonorkAtomQueueClass	}

StartSonorkAtomQueueClass(TSonorkSidNotificationQueue,TSonorkSidNotificationAtom)
	NoExtensions;
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkUserDataNotesQueue,TSonorkUserDataNotes)
	NoExtensions;
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkDynStringQueue,TSonorkDynString)
	bool	Add(SONORK_C_CSTR str)
        	{
                	bool rv;
                	sonork_mem_lock();
                	rv = Add( new TSonorkDynString(str) );
                        sonork_mem_unlock();
                        return rv;
                }

	bool	Add(SONORK_W_CSTR str)
        	{
                	bool rv;
                	sonork_mem_lock();
                	rv = Add( new TSonorkDynString(str) );
                        sonork_mem_unlock();
                        return rv;
                }
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkTextQueue,TSonorkText)
	NoExtensions;
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkSysMsgQueue,TSonorkSysMsg)
	NoExtensions;
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkServiceDataQueue,TSonorkServiceData)
	NoExtensions
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkUserServerQueue,TSonorkUserServer)
	NoExtensions
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkWappDataQueue,TSonorkWappData)
	NoExtensions
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkAuthReqDataQueue,TSonorkAuthReqData)
	NoExtensions
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkTrackerRoomQueue,TSonorkTrackerRoom)
	NoExtensions;
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkTrackerDataQueue,TSonorkTrackerData)
	NoExtensions;
EndSonorkAtomQueueClass;

StartSonorkAtomQueueClass(TSonorkTrackerMemberQueue,TSonorkTrackerMember)
	NoExtensions;
EndSonorkAtomQueueClass;

// TSonorkGroupQueue's AddSorted() is used by the client which will normally need
// to have the queue sorted in a "Parents before Children" fashion as to
// easily load them into a tree-like view.
// The sorting on the client is done by letting <depth> act
// as the reversed priority where higher depths (children) get lower priorities
// and are added towards the end and lower depths (parents) get higher
// priorities and are added towards the beginning.
// The server always sends the queues sorted by depth, so AddSorted()
// is only used when loading from a file or another source

StartSonorkAtomQueueClass(TSonorkGroupQueue,TSonorkGroup)
	bool	AddSorted(TSonorkGroup*GRP)
			{
				return Add(GRP,(SONORK_QUEUE_ITEM_PRIORITY)(SONORK_GROUP_MAX_DEPTH - GRP->header.depth));
			}
EndSonorkAtomQueueClass;


extern bool sonork_codec_io_v104_compatibility_mode;
#endif  // #if SONORK_CODEC_LEVEL > 5

#endif
