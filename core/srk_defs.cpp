#include "srk_defs.h"
#pragma hdrstop
#include <ctype.h>
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

#if defined(SONORK_MEM_SERIALIZE)
//#error Don't use MEM_SERIALIZE
static CRITICAL_SECTION sonork_mem_cs;
void sonork_mem_lock()
{
	EnterCriticalSection(&sonork_mem_cs);
}
void sonork_mem_unlock()
{
	LeaveCriticalSection(&sonork_mem_cs);
}
void sonork_mem_init()
{
	InitializeCriticalSection(&sonork_mem_cs);
}
void sonork_mem_ext()
{
	DeleteCriticalSection(&sonork_mem_cs);
}
#pragma startup	sonork_mem_init	90
#pragma exit	sonork_mem_ext	90

BYTE*sonork_mem_locked_alloc(UINT sz)
{
	BYTE*rv;
	sonork_mem_lock();
	rv = sonork_mem_alloc(sz);
	sonork_mem_unlock();
	return rv;
}
void sonork_mem_locked_free(void* ptr)
{
	sonork_mem_lock();
	sonork_mem_free(ptr);
	sonork_mem_unlock();
}

#endif

// ----------------------------------------------------------------------------
// TSonorkVersion
// ----------------------------------------------------------------------------


#define SID_VERSION_APP_VERSION_MASK	0x0000ffff
#define SID_VERSION_OS_TYPE_MASK        0x00ff0000
#define SID_VERSION_OS_VERSION_MASK	0x0000ffff

SONORK_C_STR
 TSonorkVersion::GetStr(SONORK_C_STR str) const
{
	DWORD	vn=VersionNumber();
	if(!vn)
	{
		// Very old or version is blank
		*str=0;
	}
	else
	if( !(vn&0xf000) )
	{
		// Old version
		sprintf(str
		,"%u.%02u.%02u.00"
		,(vn>>8)&0xf
		,(vn>>4)&0xf
		,(vn>>0)&0xf);
	}
	else
	sprintf(str
		,"%u.%02u.%02u.%02u"
		,(vn>>12)&0xf
		,(vn>>8)&0xf
		,(vn>>4)&0xf
		,(vn>>0)&0xf);
	return str;
}

DWORD
 TSonorkVersion::VersionNumber() const
{
	return GetValueAt(V_VERSION)&SID_VERSION_APP_VERSION_MASK;
}
void
 TSonorkVersion::SetVersionNumber( DWORD v )
{
	SetValueAt(V_VERSION,v&SID_VERSION_APP_VERSION_MASK);
}

SONORK_OS_TYPE
 TSonorkVersion::OsType() const
{
	return (SONORK_OS_TYPE)((GetValueAt(V_OS)&SID_VERSION_OS_TYPE_MASK)>>16);
}
DWORD
	TSonorkVersion::OsVersion() const
{
	return (GetValueAt(V_OS)&SID_VERSION_OS_VERSION_MASK);
}
void
	TSonorkVersion::Load(
		  DWORD 		app_version
		, DWORD			flags
		, SONORK_OS_TYPE        os_type
		, DWORD 		os_version)
{
	SetValueAt(V_RESERVED,0);
	SetValueAt(V_OS,
		((os_type<<16)&SID_VERSION_OS_TYPE_MASK)
		|(os_version&SID_VERSION_OS_VERSION_MASK));
	SetValueAt(V_VERSION,app_version&SID_VERSION_APP_VERSION_MASK);
	SetValueAt(V_FLAGS,flags);
}


// ----------------------------------------------------------------------------
// TSonorkSidFlags
// ----------------------------------------------------------------------------


void
 TSonorkSidFlags::xCopyUserControlled(TSonorkSidFlags& O)
{
	v[0].v[0]=O.v[0].v[0];
	v[0].v[1]=(v[0].v[1]&~SONORK_AUTH_FM1_USER_CONTROLLED)|(O.v[0].v[1]&SONORK_AUTH_FM1_USER_CONTROLLED);
}
void
 TSonorkSidFlags::xClearServerControlled()
{
	v[0].v[1]&=SONORK_AUTH_FM1_USER_CONTROLLED;
	v[1].v[0]=v[1].v[0]=0;
}
void
 TSonorkSidFlags::xClearUserControlled()
{
	v[0].v[0]=0;
	v[0].v[1]&=~SONORK_AUTH_FM1_USER_CONTROLLED;
}

// ----------------------------------------------------------------------------
// SONORK_DWORD2/SONORK_DWORD4
// ----------------------------------------------------------------------------

bool SONORK_DWORD2::SetStr(SONORK_C_CSTR str,UINT base)
{
	char tmp[48],*c;
	for(;;)
	{
		if(!str)
			break;
		strncpy(tmp,str,46);
		tmp[47]=0;
		c=strchr(tmp,'.');
		if(c == NULL)
			break;
		*c++=0;
		if(*c == 0)
			break;
		v[0]=strtoul(tmp,NULL,base);
		v[1]=strtoul(c,NULL,base);
		return true;
	}
	v[0]=v[1]=0;
	return false;
}
SONORK_C_STR
 SONORK_DWORD2::GetStrEx(SONORK_C_STR str,SONORK_C_CSTR prefix,SONORK_C_CSTR sufix) const
{
	sprintf(str, "%s%u.%u%s", prefix?prefix:"",v[0], v[1], sufix?sufix:"");
	return str;
}

SONORK_C_STR
 SONORK_DWORD2::GetStr(SONORK_C_STR str,UINT base) const
{

#if defined(SONORK_WIN32_BUILD)
	int l;
	l=strlen(ultoa(v[0],str,base));
	*(str+l)='.';
	l++;
	ultoa(v[1],str+l,base);
	return str;
#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	if(base==16) {
		sprintf(str, "%x.%x", v[0], v[1]);
	}
	else
	{	//all other bases (FIX THIS!)
		sprintf(str, "%lu.%lu", v[0], v[1]);
	}
#else
#error NO IMPLEMENTATION
#endif
}

UINT	SONORK_DWORD2::AbsDiff( const SONORK_DWORD2& O ) const
{
	if(v[1]==O.v[1])
    {
    	if(v[0]>=O.v[0])
		{
        	return v[0]-O.v[0];
		}
		else
		{
			return O.v[0]-v[0];
		}
	}
    else
    {
     	return 0xffffffff;
    }
}


void SONORK_DWORD4::Set(const SONORK_DWORD4&O)
{
	v[0]=O.v[0];
	v[1]=O.v[1];
}
bool SONORK_DWORD4::operator ==(const SONORK_DWORD4 &O) const
{
	return v[0]==O.v[0] && v[1]==O.v[1] ;
}
bool SONORK_DWORD4::IsZero() const
{
	return v[0].IsZero() && v[1].IsZero();
}
void SONORK_DWORD4::Clear()
{
	v[0].v[0]=v[0].v[1]=v[1].v[0]=v[1].v[1]=0;
}


// ----------------------------------------------------------------------------
// TSonorkServiceInfo
// ----------------------------------------------------------------------------
void
 TSonorkServiceInfo::SetInfo(SONORK_SERVICE_ID svc_id,SONORK_SERVICE_TYPE svc_type, DWORD svc_hflags)
{
	v[V_ID] =
		svc_id&SONORK_SERVICE_ID_MASK ;

	v[V_DESCRIPTOR]
		=
		(svc_type&SONORK_SERVICE_TYPE_MASK)|(svc_hflags&SONORK_SERVICE_HFLAGS_MASK);
}
// ----------------------------------------------------------------------------
// TSonorkServiceServerExt
// ----------------------------------------------------------------------------
void
 TSonorkOldServiceServerExt::SetService(SONORK_SERVICE_TYPE svc_type, DWORD svc_hflags)
{
	v2[V2_DESCRIPTOR]=
		 (svc_type&SONORK_SERVICE_TYPE_MASK)
		|(svc_hflags&SONORK_SERVICE_HFLAGS_MASK);
	v2[V2_RESERVED]=0;
}

// ----------------------------------------------------------------------------
// TSonorkTime
// ----------------------------------------------------------------------------
#define SONORK_TIME_TYPE_MASK   0xf
#define SONORK_TIME_TYPE_SHIFT  28
#define SONORK_TIME_DST_MASK    0x1
#define SONORK_TIME_DST_SHIFT   27
#define SONORK_TIME_YEAR_MASK   0x7ff
#define SONORK_TIME_YEAR_SHIFT  9
#define SONORK_TIME_YEAR_BASE	1900
#define SONORK_TIME_YEAR_LIMIT	2500
#define SONORK_TIME_MONTH_MASK  0xf
#define SONORK_TIME_MONTH_SHIFT 5
#define SONORK_TIME_DAY_MASK    0x1f
#define SONORK_TIME_DAY_SHIFT   0
#define SONORK_TIME_HOUR_MASK       0x1f
#define SONORK_TIME_HOUR_SHIFT      27
#define SONORK_TIME_MINUTE_MASK     0x3f
#define SONORK_TIME_MINUTE_SHIFT    21
#define SONORK_TIME_SECOND_MASK     0x3f
#define SONORK_TIME_SECOND_SHIFT    15

DWORD	TSonorkTime::DateValue() const
{
	return v[V_DATE] &
			((SONORK_TIME_YEAR_MASK		<<	SONORK_TIME_YEAR_SHIFT)
			|(SONORK_TIME_MONTH_MASK	<<	SONORK_TIME_MONTH_SHIFT)
			|(SONORK_TIME_DAY_MASK		<<	SONORK_TIME_DAY_SHIFT) );
}
DWORD	TSonorkTime::TimeValue() const
{
	return	v[V_TIME] &
			((SONORK_TIME_HOUR_MASK		<<	SONORK_TIME_HOUR_SHIFT)
			|(SONORK_TIME_MINUTE_MASK	<<	SONORK_TIME_MINUTE_SHIFT));
}
DWORD	TSonorkTime::TimeValueSecs() const
{
	return	v[V_TIME] &
			((SONORK_TIME_HOUR_MASK		<<	SONORK_TIME_HOUR_SHIFT)
			|(SONORK_TIME_MINUTE_MASK	<<	SONORK_TIME_MINUTE_SHIFT)
			|(SONORK_TIME_SECOND_MASK	<<	SONORK_TIME_SECOND_SHIFT));
}

SONORK_TIME_RELATION
	TSonorkTime::RelDate(const TSonorkTime&T) const
{
	DWORD vM,vT;
	vM=DateValue();
	vT=T.DateValue();
	if(vM == vT)return SONORK_TIME_EQUAL;
	return vM>vT?SONORK_TIME_AFTER:SONORK_TIME_BEFORE;
}
SONORK_TIME_RELATION
	TSonorkTime::RelTime(const TSonorkTime&T) const
{
	DWORD vM,vT;
	vM=TimeValue();
	vT=T.TimeValue();
	if(vM == vT)return SONORK_TIME_EQUAL;
	return vM>vT?SONORK_TIME_AFTER:SONORK_TIME_BEFORE;
}
SONORK_TIME_RELATION
	TSonorkTime::RelDateTime(const TSonorkTime&T) const
{
	SONORK_TIME_RELATION	rel;
	rel = RelDate(T);
	if( rel == SONORK_TIME_EQUAL )
		return RelTime(T);
	return rel;
}

void TSonorkTime::SetTime_GMT()
{
	time_t lt=time(NULL);
	SetTime(gmtime(&lt));
}
void TSonorkTime::SetTime_Local()
{
	time_t lt=time(NULL);
	SetTime(localtime(&lt));
}
void TSonorkTime::ToLocal()
{
#if defined(SONORK_WIN32_BUILD)
	FILETIME 	t_local,t_utc;

	GetTime(&t_utc);
	FileTimeToLocalFileTime(&t_utc, &t_local);
	SetTime(&t_local);

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	struct tm t_tm;
	time_t	lt;
	GetTime(&t_tm);
	lt = mktime( &t_tm );
	SetTime(localtime(&lt));

#else
#	error NO IMPLENTATION
#endif
}
BOOL TSonorkTime::SetTime(const struct tm* st)
{
   UINT aux;
   aux =(SONORK_TIME_TYPE_E1&SONORK_TIME_TYPE_MASK)<<SONORK_TIME_TYPE_SHIFT;
   aux|=(st->tm_isdst?1:0)<<SONORK_TIME_DST_SHIFT;
   aux|=((st->tm_year)&SONORK_TIME_YEAR_MASK)<<SONORK_TIME_YEAR_SHIFT;
   aux|=((st->tm_mon+1)&SONORK_TIME_MONTH_MASK)<<SONORK_TIME_MONTH_SHIFT;
   aux|=((st->tm_mday)&SONORK_TIME_DAY_MASK)<<SONORK_TIME_DAY_SHIFT;
   v[V_DATE]=aux;

   aux =(st->tm_hour&SONORK_TIME_HOUR_MASK)<<SONORK_TIME_HOUR_SHIFT;
   aux|=(st->tm_min&SONORK_TIME_MINUTE_MASK)<<SONORK_TIME_MINUTE_SHIFT;
   aux|=(st->tm_sec&SONORK_TIME_SECOND_MASK)<<SONORK_TIME_SECOND_SHIFT;
   v[V_TIME]=aux;
   return true;
}

UINT
 TSonorkTime::Year()	const    // 1900...
{
	return SONORK_TIME_YEAR_BASE
		+ (v[V_DATE]>>SONORK_TIME_YEAR_SHIFT)&SONORK_TIME_YEAR_MASK;
}
UINT	TSonorkTime::Month()	const	// 1-12
{
	return (v[V_DATE]>>SONORK_TIME_MONTH_SHIFT)&SONORK_TIME_MONTH_MASK;
}
UINT	TSonorkTime::Day()	const	// 1-31
{
	return (v[V_DATE]>>SONORK_TIME_DAY_SHIFT)&SONORK_TIME_DAY_MASK;
}

UINT
 TSonorkTime::Hour()	const	// 0~23
{
	return ((v[V_TIME]>>SONORK_TIME_HOUR_SHIFT)&SONORK_TIME_HOUR_MASK);
}

UINT
 TSonorkTime::Minutes() const // 0-59
{
	return ((v[V_TIME]>>SONORK_TIME_MINUTE_SHIFT)&SONORK_TIME_MINUTE_MASK);
}

UINT
 TSonorkTime::Seconds() const // 0-59
{
	return ((v[V_TIME]>>SONORK_TIME_SECOND_SHIFT)&SONORK_TIME_SECOND_MASK);
}

BOOL  TSonorkTime::IsValid() const
{
	DWORD y;

	if( IsZero() )return false;
	y=(v[V_DATE]>>SONORK_TIME_TYPE_SHIFT)&SONORK_TIME_TYPE_MASK;
	if(y!=SONORK_TIME_TYPE_E1)return false;

	y=(v[V_DATE]>>SONORK_TIME_YEAR_SHIFT)&SONORK_TIME_YEAR_MASK;
	if(y>SONORK_TIME_YEAR_LIMIT-SONORK_TIME_YEAR_BASE)return false;

	y=(v[V_DATE]>>SONORK_TIME_MONTH_SHIFT)&SONORK_TIME_MONTH_MASK;
	if(y>12)return false;

	y=(v[V_DATE]>>SONORK_TIME_DAY_SHIFT)&SONORK_TIME_DAY_MASK;
	if(y<1||y>31)return false;

	y=(v[V_TIME]>>SONORK_TIME_HOUR_SHIFT)&SONORK_TIME_HOUR_MASK;
	if(y>=24)return false;

	y=(v[V_TIME]>>SONORK_TIME_MINUTE_SHIFT)&SONORK_TIME_MINUTE_MASK;
	if(y>=60)return false;

	y=(v[V_TIME]>>SONORK_TIME_SECOND_SHIFT)&SONORK_TIME_SECOND_MASK;
	if(y>=60)return false;

	return true;
}
void	TSonorkTime::ClearDate()
{
	v[V_DATE] = (SONORK_TIME_TYPE_E1&SONORK_TIME_TYPE_MASK)<<SONORK_TIME_TYPE_SHIFT;
}
void	TSonorkTime::ClearTime()
{
	v[V_TIME] = 0;
}

BOOL TSonorkTime::SetDate(UINT year, UINT month, UINT day)
{
	struct ::tm st;
	st.tm_year = year - 1900;
	st.tm_mon  = month - 1;
	st.tm_mday = day;
	st.tm_hour=
	st.tm_min=
	st.tm_sec=0;
	return SetTime(&st);
}

BOOL TSonorkTime::SetDate(SONORK_C_CSTR str)	// yyyy/mm/dd or yyyy.mm.dd
{
	struct ::tm st;
	char tmp[8];
	int l;
	SONORK_C_CSTR ptr;
	for(;;)
	{
		for(ptr=str,l=0	;l<4 && *ptr>='0' && *ptr<='9'; ptr++,l++)
			tmp[l]=*ptr;
		if(*ptr!='/' && *ptr!='.')break;
		tmp[l]=0;
		st.tm_year = atoi(tmp) - 1900;
		for(ptr++,l=0	;l<2 && *ptr>='0' && *ptr<='9'; ptr++,l++)
			tmp[l]=*ptr;
		if(*ptr!='/' && *ptr!='.')break;
		tmp[l]=0;
		st.tm_mon = atoi(tmp) - 1;
		for(ptr++,l=0	;l<2 && *ptr>='0' && *ptr<='9'; ptr++,l++)
			tmp[l]=*ptr;
		tmp[l]=0;
		st.tm_mday = atoi(tmp);
		st.tm_hour=
		st.tm_min=
		st.tm_sec=0;
		return SetTime(&st);

	}
    Clear();
	return false;

}
BOOL TSonorkTime::GetDate(SONORK_C_STR str) const	// yyyy/mm/dd
{

	struct ::tm st;

	if(GetTime(&st))
	{
		sprintf(str,"%04u/%02u/%02u"
			,st.tm_year+1900
			,st.tm_mon +1
			,st.tm_mday);
		return true;
	}
	*str=0;
	return false;

}
BOOL TSonorkTime::GetTime(struct tm* st) const
{
	UINT aux;
	aux=(v[V_DATE]>>SONORK_TIME_TYPE_SHIFT)&SONORK_TIME_TYPE_MASK;
	if(aux==SONORK_TIME_TYPE_E1)
	{
		aux=v[V_DATE];
		st->tm_mday	=(int)((aux>>SONORK_TIME_DAY_SHIFT)&SONORK_TIME_DAY_MASK);
		st->tm_mon	=(int)((aux>>SONORK_TIME_MONTH_SHIFT)&SONORK_TIME_MONTH_MASK)-1;
		st->tm_year	=(int)((aux>>SONORK_TIME_YEAR_SHIFT)&SONORK_TIME_YEAR_MASK);
		st->tm_isdst  =((aux>>SONORK_TIME_DST_SHIFT)&SONORK_TIME_DST_MASK);

		aux=v[V_TIME];
		st->tm_sec	=(int)((aux>>SONORK_TIME_SECOND_SHIFT)&SONORK_TIME_SECOND_MASK);
		st->tm_min	=(int)((aux>>SONORK_TIME_MINUTE_SHIFT)&SONORK_TIME_MINUTE_MASK);
		st->tm_hour	=(int)((aux>>SONORK_TIME_HOUR_SHIFT)&SONORK_TIME_HOUR_MASK);
		return
				st->tm_year>=0 && st->tm_year<(SONORK_TIME_YEAR_LIMIT-SONORK_TIME_YEAR_BASE)
			&&  st->tm_mon>=0  && st->tm_mon<=11
			&&  st->tm_mday>=1 && st->tm_mon<=31
			&&  st->tm_hour>=0 && st->tm_hour<24
			&&  st->tm_min>=0  && st->tm_min<60
			&&  st->tm_sec>=0  && st->tm_sec<60;
	}
	return false;
}


#if defined(SONORK_WIN32_BUILD)

BOOL  TSonorkTime::SetTime(const FILETIME*FT)
{
	SYSTEMTIME ST;
	if(!FileTimeToSystemTime(FT,&ST))
	{
		Clear();
		return false;
	}
	return SetTime(&ST);
}
BOOL  TSonorkTime::SetTime(const SYSTEMTIME*ST)
{
	struct tm st;
	st.tm_mday	=ST->wDay;
	st.tm_mon	=ST->wMonth-1;
	st.tm_year	=ST->wYear - 1900;
	st.tm_isdst =0;
	st.tm_sec	=ST->wSecond;
	st.tm_min	=ST->wMinute;
	st.tm_hour	=ST->wHour;
	SetTime(&st);
	return true;
}

BOOL  TSonorkTime::GetTime(SYSTEMTIME*ST) const
{
	UINT aux;
	aux=(v[V_DATE]>>SONORK_TIME_TYPE_SHIFT)&SONORK_TIME_TYPE_MASK;
	if(aux==SONORK_TIME_TYPE_E1)
	{
		aux=v[V_DATE];
		ST->wDay	=(WORD)(((aux>>SONORK_TIME_DAY_SHIFT)&SONORK_TIME_DAY_MASK));
		ST->wMonth  =(WORD)(((aux>>SONORK_TIME_MONTH_SHIFT)&SONORK_TIME_MONTH_MASK));
		ST->wYear	=(WORD)(((aux>>SONORK_TIME_YEAR_SHIFT)&SONORK_TIME_YEAR_MASK)+SONORK_TIME_YEAR_BASE);

		aux=v[V_TIME];
		ST->wSecond	=(WORD)((aux>>SONORK_TIME_SECOND_SHIFT)&SONORK_TIME_SECOND_MASK);
		ST->wMinute	=(WORD)((aux>>SONORK_TIME_MINUTE_SHIFT)&SONORK_TIME_MINUTE_MASK);
		ST->wHour	=(WORD)((aux>>SONORK_TIME_HOUR_SHIFT)&SONORK_TIME_HOUR_MASK);
		ST->wMilliseconds	=0;
		return
				ST->wYear	<SONORK_TIME_YEAR_LIMIT
			&&  ST->wMonth	>=1 && ST->wMonth<=12
			&&  ST->wDay	>=1 && ST->wDay<=31
			&&  ST->wHour 	<24
			&&  ST->wMinute	<60
			&&  ST->wSecond	<60;
	}
	return false;

}


BOOL  TSonorkTime::GetTime(FILETIME* FT) const
{
	SYSTEMTIME ST;
	if(!GetTime(&ST))
		return false;
	return SystemTimeToFileTime(&ST,FT);
}


#endif

// Time ellapsed since <t2> to <this> (seconds)
BOOL TSonorkTime::DiffTime(const TSonorkTime& t2,double*diff) const
{
	struct tm st1,st2;
	time_t tt1,tt2;
	if(this->GetTime(&st1))
		if(t2.GetTime(&st2))
		{
			tt1=mktime(&st1);
			tt2=mktime(&st2);
			if(tt1!=-1&&tt2!=-1)
			{
				*diff=difftime(tt1,tt2);
				return true;
			}
		}
	return false;
}

// ----------------------------------------------------------------------------
// TSonorkRegion
// ----------------------------------------------------------------------------

int
 TSonorkRegion::GetTimeZone() const
{
	return ((v[0]&SONORK_REGION_F0_TZ_VALUE)>>24)
		*((v[0]&SONORK_REGION_F0_TZ_NEGATIVE)?-1:1);
}

char*
 TSonorkRegion::GetTimeZoneStr(char*str) const
{
	int tz;
	int fract;
	tz=GetTimeZone();
	fract=tz<0?-tz:tz;
	sprintf(str,"GMT %+d.%02d"
		,(tz/SONORK_REGION_TZ_DIVISOR)
		,(fract%SONORK_REGION_TZ_DIVISOR)*SONORK_REGION_TZ_DIVISOR_MINS
		);
	return str;
}

void
 TSonorkRegion::SetTimeZone(int n)
{
	DWORD aux;
	if(n<0)
	{
		aux = (DWORD) ( (-n) | (SONORK_REGION_F0_TZ_NEGATIVE>>24) );
	}
	else
	{
		aux = (DWORD) (n);
	}
	aux<<=24;
	v[0]=( ( v[0]&~SONORK_REGION_F0_TZ ) | (aux&SONORK_REGION_F0_TZ) );
}


// ----------------------------------------------------------------------------
// TSonorkFlags
// ----------------------------------------------------------------------------

void TSonorkFlags::SetClear(UINT f,BOOL set)
{
	if(set)Set(f);
	else   Clear(f);
}

BOOL TSonorkFlags::Toggle(UINT f)
{
	if(Test(f))
	{
		Clear(f);
		return false;
	}
	else
	{
		Set(f);
		return true;
	}
}

// ----------------------------------------------------------------------------
// TSonorkUserLocus2
// ----------------------------------------------------------------------------

void TSonorkUserLocus2::Set(
	   const TSonorkId& 	  pUserId
	 , const TSonorkSid&      pSid
	 , const TSonorkPhysAddr& pPhysAddr
	 , const TSonorkSidFlags& pSidFlags)
   {
		userId.Set(pUserId);
		sid.Set(pSid);
		physAddr.Set(pPhysAddr);
		sidFlags.Set(pSidFlags);
   }

// ----------------------------------------------------------------------------
// TSonorkClock
// ----------------------------------------------------------------------------

void TSonorkClock::LoadCurrent()
{
#if defined(SONORK_WIN32_BUILD)

	GetSystemTimeAsFileTime(&T.ft);

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

/*	struct timeval tv;
	gettimeofday(&tv,NULL);
	T = (double)tv.tv_sec;
	T*= 1000.0;
	T+= (double)tv.tv_usec;
*/
	timeb	tv;
	ftime(&tv);

	T = (double)tv.time;
	T*= 1000.0;
	T+= tv.millitm;

	//sonork_printf("LC: %u.%u -> %f",tv.time,tv.millitm,T);

#else
#	error NO IMPLEMENTATION
#endif
}

void	TSonorkClock::Set(const TSonorkClock& O)
{
#if defined(SONORK_WIN32_BUILD)

	T.v64 =	O.T.v64;

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	this->T = O.T;

#else
#	error NO IMPLEMENTATION
#endif
}

void	TSonorkClock::LoadInterval(const TSonorkClock& O)
{
#if defined(SONORK_WIN32_BUILD)
	T.v64	-=	O.T.v64;

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	this->T -= O.T;

#else
#	error NO IMPLEMENTATION
#endif
}
void	TSonorkClock::LoadInterval2( const TSonorkClock& O1 , const TSonorkClock& O2)
{
#if defined(SONORK_WIN32_BUILD)

	T.v64	=	O1.T.v64 - O2.T.v64;

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	T		= 	O1.T - O2.T;
#else
#	error NO IMPLEMENTATION
#endif
}

UINT	TSonorkClock::IntervalSecsAfter( const TSonorkClock& O) const
{
#if defined(SONORK_WIN32_BUILD)

	return (UINT)( (T.v64 - O.T.v64) / SONORK_WIN32_TIME_SEC_DIVISOR);

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	return (UINT)( (T - O.T) / 1000.0 );

#else
#	error NO IMPLEMENTATION
#endif
}


UINT	TSonorkClock::IntervalMsecsAfter(const TSonorkClock&O) const
{
#if defined(SONORK_WIN32_BUILD)

	return (UINT)( (T.v64 - O.T.v64) / SONORK_WIN32_TIME_MSEC_DIVISOR);

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	return (UINT)( (T - O.T) );
#else
#	error NO IMPLEMENTATION
#endif
}
void	TSonorkClock::LoadIntervalSecs(UINT s)
{
#if defined(SONORK_WIN32_BUILD)

	T.v64 = s;
	T.v64*= SONORK_WIN32_TIME_SEC_DIVISOR;

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	T = s * 1000;

#else
#	error NO IMPLEMENTATION
#endif
}
void	TSonorkClock::LoadIntervalMsecs(UINT m)
{
#if defined(SONORK_WIN32_BUILD)

	T.v64 = m;
	T.v64*= SONORK_WIN32_TIME_MSEC_DIVISOR;

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	T = m;

#else
#	error NO IMPLEMENTATION
#endif
}


UINT	TSonorkClock::IntervalSecs() const
{
#if defined(SONORK_WIN32_BUILD)

	return (UINT)( T.v64  / SONORK_WIN32_TIME_SEC_DIVISOR );

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	return (UINT)( T  / 1000.0 );

#else
#	error NO IMPLEMENTATION
#endif
}

UINT	TSonorkClock::IntervalMsecs() const
{
#if defined(SONORK_WIN32_BUILD)

	return (UINT)( T.v64  / SONORK_WIN32_TIME_MSEC_DIVISOR );

#elif (defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD))

	return (UINT)( T );

#else
#	error NO IMPLEMENTATION
#endif
}

UINT	TSonorkClock::IntervalSecsCurrent() const
{
	TSonorkClock	O;
	O.LoadCurrent();
	return O.IntervalSecsAfter(*this);
}
UINT	TSonorkClock::IntervalMsecsCurrent() const
{
	TSonorkClock	O;
	O.LoadCurrent();
	return O.IntervalMsecsAfter(*this);
}



// ----------------------------------------------------------------------------
// TSonorkTempBuffer
// ----------------------------------------------------------------------------

TSonorkTempBuffer::TSonorkTempBuffer(UINT sz)
{
	if(sz<32)sz=32;
	size=sz;
	buffer=SONORK_MEM_ALLOC(BYTE,size);
}
TSonorkTempBuffer::~TSonorkTempBuffer()
{
	if(buffer)SONORK_MEM_FREE(buffer);

}
void TSonorkTempBuffer::Set(const char*str)
{
	if(str)
	{
		int l;
		l=strlen(str)+1;
		SetMinSize(l);
		memcpy(buffer,str,l);
	}
	else
		*buffer=0;

}
// Reallocates only if cur size < new_size
void TSonorkTempBuffer::SetMinSize(UINT new_size)
{
	if(size<new_size)
		SetSize(new_size);
}
void TSonorkTempBuffer::SetSize(UINT sz)
{
	if(buffer)SONORK_MEM_FREE(buffer);
	if(sz<32)sz=32;
	size=sz;
	buffer=SONORK_MEM_ALLOC(BYTE,size);

}

// ----------------------------------------------------------------------------
// TSonorkShortString
// ----------------------------------------------------------------------------

TSonorkShortString::TSonorkShortString(SONORK_C_CSTR p_str)
{
	str=NULL;
	Set(p_str);
}

UINT
 TSonorkShortString::Trim()
{
	if(!IsNull())
		return SONORK_StrTrim(Buffer());
	return 0;
}

SONORK_C_CSTR
	TSonorkShortString::Append( SONORK_C_CSTR str1 , SONORK_C_CSTR str2 )
{
	int l0,l1,l2;
	SONORK_C_STR	new_buffer;
	l0=Length();
	l1=str1?strlen(str1):0;
	l2=str2?strlen(str2):0;
	new_buffer = SONORK_MEM_ALLOC(char ,l0+l1+l2+1);
	if( l0 > 0 )
		strcpy(new_buffer , str );
	else
		*new_buffer=0;
	if( l1 > 0 )strcat(new_buffer + l0, str1);
	if( l2 > 0 )strcat(new_buffer + l0 + l1, str2);
	Clear();
	str=new_buffer;
	return str;
}

SONORK_C_CSTR
 TSonorkShortString::Assign(SONORK_C_STR new_buffer)
{
	Clear();
	str=new_buffer;
	return str;
}

SONORK_C_CSTR
 TSonorkShortString::CStr()     const
{
	return IsNull()?"":str;
}

SONORK_C_CSTR
 TSonorkShortString::Set(SONORK_C_CSTR p_str)
{
	UINT sz;
	if(str)Clear();
	if(p_str)
	{
		sz=strlen(p_str);
		str=SONORK_MEM_ALLOC(char,sz<8?8:sz+1);
		memcpy(str,p_str,sz+1);
	}
	return str;
}
SONORK_C_STR
 TSonorkShortString::SetBufferSize(UINT sz)
{
	Clear();
	if(sz)str=SONORK_MEM_ALLOC(char,sz<8?8:sz);
	return str;
}

UINT
 TSonorkShortString::CutAt( UINT sz )
{
	UINT csz;
	if(!str)return 0;
	csz=strlen(str);
	if(csz>sz)
	{
		*(str+sz)=0;
		return sz;
	}
	return csz;
}

void
 TSonorkShortString::Clear()
{
	if(str){SONORK_MEM_FREE(str);str=NULL;}
}

bool
 TSonorkShortString::operator==(SONORK_C_CSTR p_str)
{
	if(IsNull() || !p_str)
	{
		return false;
	}
	else
	{
		return !SONORK_StrNoCaseCmp(str,p_str);
	}
}

// ----------------------------------------------------------------------------
// TSonorkLock
// ----------------------------------------------------------------------------

TSonorkLock::TSonorkLock()
{
#if defined(SONORK_WIN32_BUILD)
	InitializeCriticalSection(&cs);
#endif
	count=0;
#if defined(SONORK_CLIENT_BUILD)
	file_name="";
#endif
}
TSonorkLock::~TSonorkLock()
{
#if defined(SONORK_WIN32_BUILD)
	DeleteCriticalSection(&cs);
#endif
}
void TSonorkLock::Lock(const char*name,UINT line)
{
#if defined(SONORK_WIN32_BUILD)
	EnterCriticalSection(&cs);
#elif defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD)
//  must add Thread mutex: Can be blank
//   if application is not multithreaded (like the client)
#else
#	error NO IMPLEMENTATION
#endif
	count++;

	file_name=name;
	file_line=line;
}
void TSonorkLock::Unlock()
{
	file_name="";
	if(count<=0)
	{
		assert(count>0);
	}
	count--;
#if defined(SONORK_WIN32_BUILD)
			LeaveCriticalSection(&cs);
#elif defined(SONORK_LINUX_BUILD) || defined(SONORK_BEOS_BUILD)
//  must add Thread mutex: Can be blank
//   if application is not multithreaded (like the client)
#else
#	error NO IMPLEMENTATION
#endif
}

// ----------------------------------------------------------------------------
// TSonorkCtrlMsg
// ----------------------------------------------------------------------------
void
 TSonorkCtrlMsg::LoadNewCmd(
			  SONORK_CTRLMSG_CMD	p_cmd
			, DWORD 		p_param
			, DWORD 		p_flags
			, DWORD			p_sys_flags)
{
	cmdFlags		= p_cmd;
	sysFlags		= p_sys_flags|SONORK_CTRLMSG_SF_NEW;
	body.n.usrParam		= p_param;
	body.n.usrFlags		= p_flags;

	target_instance 	=
	source_instance 	=
	body.n.systemId		=
	body.n.serviceId	= 0;

}

void
 TSonorkCtrlMsg::ClearOldParams()
{
	// Old Ctrl Msgs used <reserved> as <query_id>
	// and assumed the ClearParams() did not reset
	// the <query_id>
	SONORK_ZeroMem( &body.o.param , sizeof(body.o.param));
}
void
 TSonorkCtrlMsg::Clear()
{
	SONORK_ZeroMem( this , sizeof(*this));
}


// ----------------------------------------------------------------------------
// TSonorkUserAuth
// ----------------------------------------------------------------------------

UINT TSonorkUserAuth::Load(const SONORK_DWORD2& userId_A, const SONORK_DWORD2& userId_B)
{
	if(userId_A<=userId_B)
	{
		userId[0].Set(userId_A);
		userId[1].Set(userId_B);
		return 0;
	}
	else
	{
		userId[0].Set(userId_B);
		userId[1].Set(userId_A);
		return 1;
	}

}

UINT
 TSonorkUserAuth::GetIndex(const SONORK_DWORD2& pUserId) const
{
	if(userId[0]==pUserId)return 0;
	if(userId[1]==pUserId)return 1;
	return (UINT)-1;
}

void TSonorkUserAuth::SetAuth(UINT index,const TSonorkAuthFlags& flags,DWORD tag)
{
	assert(index<=1);
	auth[index].flags.Set( flags );
	auth[index].tag  =tag;
}
void TSonorkUserAuth::SetTag(UINT index,DWORD tag)
{
	assert(index<=1);
	auth[index].tag  =tag;
}
void TSonorkUserAuth::SetFlags(UINT index,const TSonorkAuthFlags&flags)
{
	assert(index<=1);
	auth[index].flags.Set(flags);
}
DWORD  TSonorkUserAuth::GetTag(UINT index) const
{
	assert(index<=1);
	return auth[index].tag;
}
const TSonorkAuthFlags& TSonorkUserAuth::GetFlags(UINT index) const 
{
	assert(index<=1);
	return auth[index].flags;
}

// ----------------------------------------------------------------------------
// TSonorkBitFlags
// ----------------------------------------------------------------------------

TSonorkBitFlags::TSonorkBitFlags(DWORD max_value)
{
	dwords 		= (max_value >> 5)+1;
	ptr = SONORK_MEM_ALLOC( DWORD , dwords*sizeof(DWORD) );
	ClearAll();
}
TSonorkBitFlags::~TSonorkBitFlags()
{
	SONORK_MEM_FREE(ptr);
}
void	TSonorkBitFlags::ClearAll()
{
	SONORK_ZeroMem(ptr,dwords*sizeof(DWORD) );
}
BOOL	TSonorkBitFlags::Test(UINT value) const
{
	DWORD	dword_offset, bit_offset;
	dword_offset 	= value >> 5;
	if( dword_offset < dwords )
	{
		bit_offset      = value & 0x1f;
		return *(ptr+dword_offset) & (1<<bit_offset);
	}
	return false;
}
void	TSonorkBitFlags::Clear(UINT value)
{
	DWORD	dword_offset, bit_offset;
	dword_offset 	= value >> 5;
	if( dword_offset < dwords )
	{
		bit_offset      = value & 0x1f;
		*(ptr+dword_offset) &= ~(1<<bit_offset);
	}
}
void	TSonorkBitFlags::Set(UINT value)
{
	DWORD	dword_offset, bit_offset;
	dword_offset 	= value >> 5;
	if( dword_offset < dwords )
	{
		bit_offset      = value & 0x1f;
		*(ptr+dword_offset) |= (1<<bit_offset);
	}
}


// ----------------------------------------------------------------------------
// SONORK_GenTrackingNo
// ----------------------------------------------------------------------------

DWORD	SONORK_GenTrackingNo(const TSonorkId& sender, const TSonorkId& target)
{
	static UINT salt=0;
	DWORD v;
	if(++salt>31)salt=0;

#if defined(SONORK_WIN32_BUILD)
	SYSTEMTIME	st;
	GetSystemTime(&st);

	v=(st.wYear-2000)%16;
	v<<=4;
	v|=(st.wMonth-(salt&0x3))&0xf;
	v<<=5;
	v|=(st.wDay)&0x1f;
	v<<=5;
	v|=(st.wHour+((salt>>2)&0x7))&0x1f;
	v<<=6;
	v|=(st.wMinute)&0x3f;
	v<<=6;
	v|=(st.wSecond)&0x3f;
#else
	time_t lt=time(NULL);
	struct tm *st;
	st = localtime(&lt);

	v=(st->tm_year-100)%16;
	v<<=4;
	v|=(st->tm_mon-(salt&0x3))&0xf;
	v<<=5;
	v|=(st->tm_mday)&0x1f;
	v<<=5;
	v|=(st->tm_hour+((salt>>2)&0x7))&0x1f;
	v<<=6;
	v|=(st->tm_min)&0x3f;
	v<<=6;
	v|=(st->tm_sec)&0x3f;
	
#endif
	if(sender<=target)
		v|=0x40000000;
	return (v&0x7ffffff);
}
// SONORK_EncodePin64
//  Builds a 64-bit pin (tgt_pin) mixing <src_pin>
//   with the src_guid, tgt_guid, tgt_service_id and tgt_service_pin;
//   the <src_pin> is diluted and cannot be derived from <tgt_pin>.
//  Used for when a user (src_guid) wants to connect to another
//   user (tgt_guid) to use a specific service.
//  Normally <src_pin> will be the src_guid's "sid_pin" which is known
//   only by the <src_guid> and the Sonork server, it should never be given
//   away and lasts only for one session (Sonork receives a new sid_pin every
//   time it connects).
//  The resultant <tgt_pin> is handed over to the target user (tgt_guid),
//   the target user can issue an IDENTIFY_USER on the server in order
//   to verify that the <tgt_pin> is valid.
//  On reception of an IDENTIFY_USER request, the server will invoke this
//   exact same function and compare the resultant <tgt_pin> with the
//   one in the request; the server can do that because it knows the
//   "sid_pin" on the src_guid.
//  If they don't match, the IDENTIFY_USER function fails: Probably because
//   <src_guid> is not REALLY <src_guid> and rather someone trying to
//   impersonate <src_guid>; but unless the impersonator has the correct
//   <sid_pin>, it won't be able to generate the same <tgt_pin>.

void  SONORK_EncodePin64(
			 SONORK_DWORD4&		tgt_pin
			,const 	TSonorkId& 	src_guid
			,const 	TSonorkPin& src_pin
			,const 	TSonorkId& 	tgt_guid
			,SONORK_SERVICE_ID  tgt_service_id
			,DWORD			tgt_service_pin)
{
	tgt_pin.v[0].v[0]=(DWORD)
	(
		(
			((tgt_guid.v[1] - src_guid.v[0] + 3) * ((src_pin.v[1]&0x3)+tgt_service_id+2))
			& 0xff00
		)
		+
		(
			( ((src_pin.v[0]>>16)+(src_pin.v[1]<<8)) & 0x0f0f0f0f )
			^
			( ((src_pin.v[0]<<8)+(src_pin.v[1]>>16)) & 0xf0f0f0f0 )
		)
		+
		(tgt_service_pin & 0xff)
	);

	tgt_pin.v[0].v[1]=(DWORD)
	(
		(
			((tgt_guid.v[0] - src_guid.v[1] + 2) * ((src_pin.v[0]&0x3)+tgt_service_id+3))
			& 0x00ff
		)
		+
		(
			( ((src_pin.v[1]>>16)+(src_pin.v[0]<<8)) & 0xf0f0f0f0 )
			^
			( ((src_pin.v[1]<<8)+(src_pin.v[0]>>16)) & 0xf0f0f0f0 )
		)
		+
		(tgt_service_pin >> 8)
	);
	tgt_pin.v[1].Clear();
}


// ----------------------------------------------------------------------------
// GU STRING MANIPULATION
// ----------------------------------------------------------------------------
UINT  SONORK_StrLen(SONORK_W_CSTR	str)
{
	if(!str)return 0;
#if defined(SONORK_WIN32_BUILD)
	return wcslen((const wchar_t *)str)-1;
#else
	UINT rv;
	for(rv=0;*str;rv++,str++);
	return rv;
#endif
}
SONORK_C_STR  SONORK_CStr(SONORK_C_CSTR source)
{
	int l;
	SONORK_C_STR target;
	if(source)
	{
		l=strlen(source)+1;
		target=SONORK_CStr(l);
		memcpy(target,source,l);
	}
	else
	{
		target=SONORK_CStr(4);
		*target=0;
	}
	return target;

}
SONORK_W_STR  SONORK_WStr(SONORK_C_CSTR source)
{
	int l;
    SONORK_W_STR target;
    l=SONORK_StrLen(source)+1;
    target=SONORK_WStr(l+4);
    SONORK_StrCopy(target,l,source,l-1);
	return target;
}
SONORK_W_STR  SONORK_WStr(SONORK_W_CSTR source)
{
	int l;
    SONORK_W_STR target;
	l=SONORK_StrLen(source)+1;
    target=SONORK_WStr(l+4);
    SONORK_StrCopy(target,l,source,l-1);
    return target;
}
UINT
 SONORK_StrCopyCut(SONORK_C_STR target,UINT target_length,SONORK_C_CSTR source)
{
	UINT		len;
	if(!target||!target_length||!source)
		return 0;
	target_length--;
	for(len=0;len < target_length && *source;len++)
	{
		if(*source>=0 && *source<=' ')break;
		*target++ = *source++;
	}
	*target=0;
	return len;
}
UINT   SONORK_StrCopy(SONORK_C_STR target,UINT target_length,const SONORK_C_CSTR source,UINT source_length)
{
	if(!target||target_length<1)
		return 0;
	if(source_length==(UINT)-1)
		source_length=SONORK_StrLen(source);
	if(source_length>target_length-1)
		source_length=target_length-1;
	if(source_length)
	{
		UINT bytes=source_length*sizeof(SONORK_C_CHAR);
		if(source)
			memcpy(target,source,bytes);
		else
			*target=0;
	}
	*(target+source_length)=0;
	return source_length;

}
UINT  SONORK_StrCopy(SONORK_W_STR target,UINT target_length,const SONORK_W_CSTR source,UINT source_length)
{
	if(!target||target_length<1)
    	return 0;
	if(source_length==(UINT)-1)
		source_length=SONORK_StrLen(source);
	if(source_length>target_length-1)
    	source_length=target_length-1;
	if(source_length)
    {
		UINT bytes=source_length*sizeof(SONORK_W_CHAR);
        if(source)
	    	memcpy(target,source,bytes);
		else
			*target=0;
	}
	*(target+source_length)=0;
    return source_length;

}

UINT  SONORK_StrCopy(SONORK_W_STR target,UINT target_length,SONORK_C_CSTR source,UINT source_length)
{

#if defined(SONORK_WIN32_BUILD)
	int code;
	UINT code_page=GetACP();
	if(!target||target_length<1)return 0;
	*target=0;
	code=MultiByteToWideChar(code_page
		,MB_PRECOMPOSED
		,source
	    ,source_length
    	,target
	    ,0);
	if(code)
	{
		if(code<=(int)target_length)
        {
			code=MultiByteToWideChar(code_page
                ,MB_PRECOMPOSED
				,source
                ,source_length
                ,target
                ,target_length);
        }
		else
		{
			TSonorkTempBuffer tmp( code*sizeof(SONORK_W_CHAR) );
			code=MultiByteToWideChar(code_page
				,MB_PRECOMPOSED
				,source
				,source_length
				,(SONORK_W_CHAR*)tmp.Ptr()
				,code);
			if(code)
			{
				if(target_length>1)
					memcpy(target
						,tmp.Ptr()
						,(target_length-1)*sizeof(SONORK_W_CHAR));
				*(target+target_length-1)=0;
			}
        }
		if(!code)return 0;
    }
	else
    	return 0;
	return code-1;
#elif defined(SONORK_LINUX_BUILD)

	SONORK_C_CHAR *s;
	SONORK_W_CHAR *t;

	s = (SONORK_C_CHAR *)source;
	t = (SONORK_W_CHAR *)target;

	while(*s != '\0') {
		*t = (SONORK_W_CHAR)(*s & 0xFF);
		t++;
		s++;
	};
	*t = (SONORK_W_CHAR)(*s & 0xFF);
#else
#error NO IMPLEMENTATION
#endif		
}
UINT  SONORK_StrCopy(SONORK_C_STR target,UINT target_length,SONORK_W_CSTR source,UINT source_length)
{

#if defined(SONORK_WIN32_BUILD)
	int code;
    UINT code_page=GetACP();
	if(!target||target_length<1)
		return 0;
	*target=0;
	code=WideCharToMultiByte(code_page
        ,WC_COMPOSITECHECK|WC_DISCARDNS
        ,source
        ,source_length
        ,target
        ,0
		,NULL
		,0);
	if(code)
	{
		if(code<=(int)target_length)
        {
			code=WideCharToMultiByte(code_page
                    ,WC_COMPOSITECHECK|WC_DISCARDNS
                    ,source
                    ,source_length
                    ,target
					,target_length
                    ,NULL
                    ,0);
		}
        else
        {
			TSonorkTempBuffer tmp(sizeof(SONORK_C_CHAR) * code);
			code=WideCharToMultiByte(code_page
					,WC_COMPOSITECHECK|WC_DISCARDNS
					,source
					,source_length
					,target
					,code
					,NULL
					,0);
			if(code)
			{
				if(target_length>1)
					memcpy(target
						,tmp.Ptr()
						,(target_length-1)*sizeof(SONORK_C_CHAR));
				*(target+target_length-1)=0;
			}
		}
		if(!code)return 0;
	}
	else
		return 0;
    return code-1;
#elif defined(SONORK_LINUX_BUILD)

	SONORK_W_CHAR *s;
	SONORK_C_CHAR *t;

	s = (SONORK_W_CHAR *)source;
	t = (SONORK_C_CHAR *)target;

	while((*s & 0xFF) != '\0') {
		*t = (SONORK_C_CHAR)(*s & 0xFF);
		t++;
		s++;
	};
	*t = '\0';
#else
#error NO IMPLEMENTATION
#endif		
}

SONORK_C_CSTR	SONORK_StrStr(SONORK_C_CSTR source,SONORK_C_CSTR target)
{
	SONORK_C_CSTR 	s_ptr,c_ptr;
	c_ptr = target;
	s_ptr = NULL;
	while(*source)
	{
		if(toupper(*source) == toupper(*c_ptr))
		{
			if(s_ptr==NULL)s_ptr = source;
			c_ptr++;
			if(*c_ptr==0)
			{
				return s_ptr;
			}
		}
		else
		{
			c_ptr=target;
			s_ptr=NULL;
		}
		source++;
	}
	return NULL;
}

void	SONORK_StrToLower(char *s){	while(*s){*s=(char)tolower(*s);s++;}}
void	SONORK_StrToUpper(char *s){	while(*s){*s=(char)toupper(*s);s++;}}

UINT
 SONORK_StrTrim(SONORK_C_STR str)
{
	int init_delta,l;
	char *sp,*ep;
	sp=str;
	init_delta=0;
	while(*sp>0&&*sp<=' '){sp++;init_delta++;}
	if(!*sp){*str=0;return 0;}
	l=strlen(str)-1;
	ep=str+l;
	while(l&&*ep>=0&&*ep<=' '){ep--;l--;}
	*(ep+1)=0;
	if(init_delta)
	{
		char *t=str;
		sp=str+init_delta;
		while(*sp){*t++=*sp++;}
		*t=0;
	}
	return l+1-init_delta;
}








#define TRACE_BUFFER_SIZE	800


void   sonork_printf(const char* fmt,...)
{
	DWORD len;
	char trace_buffer[TRACE_BUFFER_SIZE];
	va_list argptr;
	va_start(argptr, fmt);
		len = vsprintf(trace_buffer, fmt, argptr);
	va_end(argptr);
		if(len>= TRACE_BUFFER_SIZE)
		{
			FILE *file=fopen("T:\\dump.txt","wt");
			fprintf(file,"%s",trace_buffer);
			fflush(file);
			fclose(file);
			assert( len < TRACE_BUFFER_SIZE );
		}
	sonork_puts( trace_buffer );
}



#define R_CASE(n,f)	case SONORK_RESULT_##n: str=f;break;
SONORK_C_CSTR  SONORK_ResultName(SONORK_RESULT r)
{
	SONORK_C_CSTR  str;
	switch(r)
   {
		R_CASE(OK,"OK")
		R_CASE(NO_DATA,"NoData")
		R_CASE(DUPLICATE_DATA,"DuplicateData")
		R_CASE(CODEC_ERROR,"CodecError")
		R_CASE(PROTOCOL_ERROR,"ProtocolError")
		R_CASE(INVALID_PARAMETER,"InvalidParameter")
		R_CASE(INVALID_VERSION,"InvalidVersion")
		R_CASE(INVALID_OPERATION,"InvalidOperation")
		R_CASE(INVALID_ENCRYPTION,"InvalidEncryption")
		R_CASE(NOT_SUPPORTED,"NotSupported")
		R_CASE(TIMEOUT,"Timeout")
		R_CASE(SERVICE_BUSY,"ServiceBusy")
//		R_CASE(BUFFER_TOO_SMALL)
//		R_CASE(BUFFER_TOO_LARGE)
		R_CASE(QUOTA_EXCEEDED,"QuotaExceeded")
		R_CASE(OUT_OF_RESOURCES,"OutOfResources")
		R_CASE(NOT_ACCEPTED,"NotAccepted")
		R_CASE(NETWORK_ERROR,"NetworkError")
		R_CASE(STORAGE_ERROR,"StorageError")
		R_CASE(INTERNAL_ERROR,"InternalError")
		R_CASE(ACCESS_DENIED,"AccessDenied")
		R_CASE(DATABASE_ERROR,"DatabaseError")
		R_CASE(USER_TERMINATION,"UserTermination")
		R_CASE(CONFIGURATION_ERROR,"ConfigError")
		R_CASE(FORCED_TERMINATION,"ForcedTermination")
		R_CASE(INVALID_HANDLE,"InvalidHandle")
		R_CASE(NOT_AVAILABLE,"NotAvailable")
		R_CASE(OS_ERROR,"OpSysError")
		R_CASE(INVALID_MODE,"InvalidMode")
		R_CASE(INVALID_SERVER,"InvalidServer")
		R_CASE(OK_PENDING,"OkPending")
		R_CASE(NOT_READY,"NotReady")
		R_CASE(FUNCTION_DISABLED,"FunctionDisabled");
	  default:
		str="??";
      break;
	}

   return str;
}









