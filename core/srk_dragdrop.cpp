#include "srk_defs.h"
#pragma hdrstop
#include "srk_dragdrop.h"

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



static TSonorkDropSource*	_GuDropSource=NULL;
static UINT					_GuClipData_cc;
static UINT					_GuDropSourceData_cc;
static UINT					_GuDropSourceDataEnum_cc;
static UINT					_GuDropCallbackMessage;
CLIPFORMAT		cfSonorkClipData;
CLIPFORMAT		cfFileName;
CLIPFORMAT		cfFileDescr;
CLIPFORMAT		cfShellUrl;
LONG SONORK_DoDragDrop(IDataObject * pDataObject,DWORD dwOKEffect,DWORD * pdwEffect)
{
	assert( _GuDropSource != NULL );
	return DoDragDrop(pDataObject,_GuDropSource,dwOKEffect,pdwEffect);
}

void	TSonorkClipData::CODEC_WriteDataMem	(TSonorkCodecWriter& CODEC) const
{
	DWORD aux=(DataType()&0xfff);
	CODEC.WriteDW(aux);
	switch( D_type )
	{
		case SONORK_CLIP_DATA_TEXT:
		case SONORK_CLIP_DATA_URL:
		case SONORK_CLIP_DATA_FILE_NAME:
			CODEC.Write(D.sh_str_ptr);
			break;

		case SONORK_CLIP_DATA_USER_ID:
			CODEC.WriteDW2(&D.user_id);
			break;

		case SONORK_CLIP_DATA_USER_DATA:
			CODEC.Write(D.user_data);
			break;

		case SONORK_CLIP_DATA_FILE_LIST:
			{
				TSonorkListIterator I;
				TSonorkShortString	*s;
				CODEC.WriteDW(D.sh_str_que->Items());
				D.sh_str_que->BeginEnum(I);
				while((s=D.sh_str_que->EnumNext(I)) != NULL )
					CODEC.Write(s);
				D.sh_str_que->EndEnum(I);
			}
			break;
		case SONORK_CLIP_DATA_LIST:
			{
				TSonorkListIterator I;
				TSonorkClipData		*CD;
				CODEC.WriteDW(D.clip_que->Items());
				D.clip_que->BeginEnum(I);
				while((CD=D.clip_que->EnumNext(I)) != NULL )
					CODEC.Write(CD);
				D.clip_que->EndEnum(I);
			}
			break;

		default:
		case SONORK_CLIP_DATA_NONE:
			break;
	}

}
void
 TSonorkClipData::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD aux;
	CODEC.ReadDW(&aux);
	aux&=0xfff;
	D_type = (SONORK_CLIP_DATA_TYPE)aux;
	switch( D_type )
	{
		case SONORK_CLIP_DATA_TEXT:
		case SONORK_CLIP_DATA_URL:
		case SONORK_CLIP_DATA_FILE_NAME:
			{
				D.sh_str_ptr = new TSonorkShortString();
				CODEC.Read(D.sh_str_ptr);
			}
			break;

		case SONORK_CLIP_DATA_USER_ID:
			CODEC.ReadDW2(&D.user_id);
			break;
			
		case SONORK_CLIP_DATA_USER_DATA:
			D.user_data = new TSonorkUserData;
			CODEC.Read(D.user_data);
			break;

		case SONORK_CLIP_DATA_FILE_LIST:
			D.sh_str_que  = new TSonorkShortStringQueue();
			{
				TSonorkShortString	*s;
				CODEC.ReadDW(&aux);
				while( aux-- )
				{
					s=new TSonorkShortString;
					CODEC.Read(s);
					D.sh_str_que->Add( s );
				}
			}
			break;
		case SONORK_CLIP_DATA_LIST:
			D.clip_que  = new TSonorkClipDataQueue();
			{
				TSonorkClipData	*CD;
				CODEC.ReadDW(&aux);
				while( aux-- )
				{
					CD=new TSonorkClipData;
					CODEC.Read(CD);
					D.clip_que->Add( CD );
				}
			}
			break;
		default:
			D_type = SONORK_CLIP_DATA_NONE;
		case SONORK_CLIP_DATA_NONE:
			break;
	}

}

DWORD	TSonorkClipData::CODEC_DataSize()	const
{
	DWORD rv;
	rv = sizeof(DWORD);
	switch( D_type )
	{
		case SONORK_CLIP_DATA_TEXT:
		case SONORK_CLIP_DATA_URL:
		case SONORK_CLIP_DATA_FILE_NAME:
			rv += ::CODEC_Size(D.sh_str_ptr);
			break;

		case SONORK_CLIP_DATA_USER_ID:
			rv += ::CODEC_Size(&D.user_id);
			break;

		case SONORK_CLIP_DATA_USER_DATA:
			rv += D.user_data->CODEC_Size();
			break;
			
		case SONORK_CLIP_DATA_FILE_LIST:
			{
				TSonorkListIterator I;
				TSonorkShortString	*s;
				rv+=sizeof(DWORD); 	// count
				D.sh_str_que->BeginEnum(I);
				while((s=D.sh_str_que->EnumNext(I)) != NULL )
					rv+=::CODEC_Size(s);
				D.sh_str_que->EndEnum(I);
			}
			break;
			
		case SONORK_CLIP_DATA_LIST:
			{
				TSonorkListIterator I;
				TSonorkClipData		*CD;
				rv+=sizeof(DWORD); 	// count
				D.clip_que->BeginEnum(I);
				while((CD=D.clip_que->EnumNext(I)) != NULL )
					rv+=CD->CODEC_Size();
				D.clip_que->EndEnum(I);
			}
			break;

		default:
		case SONORK_CLIP_DATA_NONE:
			break;
	}
	return rv;

}

TSonorkClipData::TSonorkClipData()
{
	ref_count = 1;
	D_type 	= SONORK_CLIP_DATA_NONE;
	D.ptr 	= NULL;
	_GuClipData_cc++;
}
TSonorkClipData::~TSonorkClipData()
{
	Clear();
	_GuClipData_cc--;
}
void
 TSonorkClipData::AddRef(void)
{
	++ref_count;

}
void
 TSonorkClipData::Release(void)
{
	--ref_count;
	if(ref_count == 0)
	{
		delete this;
	}
}

bool	TSonorkClipData::HasTextFormat() const
{
	if( D_type != SONORK_CLIP_DATA_NONE
	&&  D_type != SONORK_CLIP_DATA_FILE_LIST)
		return true;
	return false;
}

CLIPFORMAT
  TSonorkClipData::GetExtClipFormat()		const
{
	if( D_type == SONORK_CLIP_DATA_URL )
		return cfShellUrl;
	else
	if( D_type == SONORK_CLIP_DATA_FILE_NAME )
		return cfFileName; 
	else
		return 0;
}

UINT
 TSonorkClipData::GetClipFormatCount()	const
{
	UINT rv;
	if( D_type == SONORK_CLIP_DATA_NONE ) return 0;
	rv=1;
	if( HasTextFormat() ) rv++;
	if( HasExtFormat()  ) rv++;
	return rv;
}

bool
 TSonorkClipData::HasClipFormat(CLIPFORMAT cf)	const
{
	if( D_type != SONORK_CLIP_DATA_NONE && cf!= 0)
	{
		if( cf == cfSonorkClipData )
			return true;
		if( cf == CF_TEXT && HasTextFormat() )
			return true;
		if( cf == GetExtClipFormat() )
			return true;
	}
	return false;
}

UINT
 TSonorkClipData::GetFormatSize(CLIPFORMAT cf)	const
{
	if( cf == cfSonorkClipData )
		return CODEC_Size();
	if( cf == CF_TEXT)
		return GetTextFormatMinSize();
	return GetExtFormatSize();
}

CLIPFORMAT
 TSonorkClipData::GetClipFormat(UINT index)		const
{
	if( index == 0 )
		return cfSonorkClipData;
	if( index == 1 && HasTextFormat() )
		return CF_TEXT;
	return GetExtClipFormat();
}


UINT
 TSonorkClipData::GetTextFormatMinSize() const
{
	UINT	rv;
	switch( D_type )
	{
		case SONORK_CLIP_DATA_TEXT:
		case SONORK_CLIP_DATA_URL:
		case SONORK_CLIP_DATA_FILE_NAME:
			return D.sh_str_ptr->Length() + 1;

		case SONORK_CLIP_DATA_USER_ID:
			return 24;

		case SONORK_CLIP_DATA_USER_DATA:
			return 26 + SONORK_USER_ALIAS_MAX_SIZE;

		case SONORK_CLIP_DATA_FILE_LIST:
			{
				TSonorkListIterator I;
				TSonorkShortString	*s;
				rv=1;						// NULL terminator
				D.sh_str_que->BeginEnum(I);
				while((s=D.sh_str_que->EnumNext(I)) != NULL )
					rv+=s->Length() + 2;	// +2 = '\r\n';
				D.sh_str_que->EndEnum(I);
			}
			return rv;

		case SONORK_CLIP_DATA_LIST:
			{
				TSonorkListIterator I;
				TSonorkClipData		*CD;
				rv=1;						// NULL terminator
				D.clip_que->BeginEnum(I);
				while((CD=D.clip_que->EnumNext(I)) != NULL )
					rv+=CD->GetTextFormatMinSize() + 2 ; // +2 = '\r\n';
				D.clip_que->EndEnum(I);
			}
			return rv;


	}
	return 0;
}

UINT
 TSonorkClipData::GetExtFormatSize()	const
{
	CLIPFORMAT cf= GetExtClipFormat();
	if( cf == cfFileName ||  cf == cfShellUrl)
		return GetTextFormatMinSize();
	return 0;
}

UINT
 TSonorkClipData::GetTextFormat(TSonorkShortString* S) const
{
	UINT sz;
	if((sz = GetTextFormatMinSize()) == 0 )
	{
		S->Clear();
		return 0;
	}
	else
	{
		S->SetBufferSize(sz);
		return GetTextFormat(S->Buffer(),sz);
	}
}

BOOL
 TSonorkClipData::GetExtFormat(void *buffer, DWORD& buffer_size)
{
	CLIPFORMAT cf;
	cf = GetExtClipFormat();
	if( cf == cfFileName || cf == cfShellUrl)
		return GetTextFormat((char*)buffer,buffer_size);
	buffer_size =0;
	return false;
}

UINT
 TSonorkClipData::GetTextFormat(char *tgt_str,DWORD tgt_size) const
{
	UINT	len,loaded_size;
	switch( D_type )
	{
		case SONORK_CLIP_DATA_TEXT:
		case SONORK_CLIP_DATA_URL:
		case SONORK_CLIP_DATA_FILE_NAME:
			lstrcpyn(tgt_str,D.sh_str_ptr->CStr(),tgt_size);
			return strlen(tgt_str);

		case SONORK_CLIP_DATA_USER_ID:
			if(tgt_size >= 24)
			{
				D.user_id.GetStr(tgt_str);
				return strlen(tgt_str);
			}
			break;
		case SONORK_CLIP_DATA_USER_DATA:
			if(tgt_size >= 26 + SONORK_USER_ALIAS_MAX_SIZE)
			{
				return sprintf(tgt_str
					, "%u.%u %s"
					, D.user_data->userId.v[0]
					, D.user_data->userId.v[1]
					, D.user_data->alias.CStr());
			}
			break;


		case SONORK_CLIP_DATA_FILE_LIST:	// TSonorkShortStringQueue*
			if( tgt_size > 2)
			{
				TSonorkListIterator I;
				TSonorkShortString	*s;
				loaded_size=0;
				tgt_size--; 	// substract char needed for terminating NULL
				D.sh_str_que->BeginEnum(I);
				while((s=D.sh_str_que->EnumNext(I)) != NULL )
				{
					len = s->Length() + 2;
					if( loaded_size + len > tgt_size )break;
					wsprintf(tgt_str,"%s\r\n" , s->CStr() );
					tgt_str+=len;
					loaded_size+=len;
				}
				D.sh_str_que->EndEnum(I);
				*tgt_str=0;
				return loaded_size;
			}
			break;
		case SONORK_CLIP_DATA_LIST:	// TSonorkClipDataQueue*
			if( tgt_size > 2)
			{
				TSonorkListIterator I;
				TSonorkClipData		*CD;

				loaded_size=0;
				tgt_size--; 	// substract char needed for terminating NULL
				D.clip_que->BeginEnum(I);
				while((CD=D.clip_que->EnumNext(I)) != NULL )
				{
					len = CD->GetTextFormatMinSize()+2;
					if( loaded_size + len > tgt_size )
						break;
					len = CD->GetTextFormat( tgt_str , tgt_size - loaded_size );
					tgt_str+=len;
					*tgt_str++='\r';
					*tgt_str++='\n';
					loaded_size += len+2;
				}
				D.clip_que->EndEnum(I);
				*tgt_str=0;
				return loaded_size;
			}
			break;
		default:
			break;
	}
	*tgt_str=0;

	return 0;
}

void
 TSonorkClipData::Clear()
{
	if( D.ptr != NULL )
	{
		switch(D_type)
		{
			default:
			case SONORK_CLIP_DATA_NONE:		// NULL
			case SONORK_CLIP_DATA_USER_ID:		// TSonorkId
				break;

			case SONORK_CLIP_DATA_URL:		// TSonorkShortString*
			case SONORK_CLIP_DATA_TEXT:		// TSonorkShortString*
			case SONORK_CLIP_DATA_FILE_NAME:	// TSonorkShortString*
				SONORK_MEM_DELETE(D.sh_str_ptr);
				break;
				
			case SONORK_CLIP_DATA_USER_DATA:	// TSonorkShortStringQueue*
				SONORK_MEM_DELETE(D.user_data);
				break;


			case SONORK_CLIP_DATA_FILE_LIST:	// TSonorkShortStringQueue*
				SONORK_MEM_DELETE(D.sh_str_que);
				break;

			case SONORK_CLIP_DATA_LIST:	// TSonorkShortStringQueue*
				{
					TSonorkClipData  *CD;
					while( (CD=D.clip_que->RemoveFirst()) != NULL )
						CD->Release();
					SONORK_MEM_DELETE(D.clip_que);
				}
				break;
		}
		D.ptr  = NULL;
	}
	D_type = SONORK_CLIP_DATA_NONE;
	//D_size = 0;

}

TSonorkUserData*
 TSonorkClipData::SetUserData(const TSonorkUserData* pUD)
{
	Clear();
	D_type = SONORK_CLIP_DATA_USER_DATA;
	SONORK_MEM_NEW( D.user_data = new TSonorkUserData() );
	if( pUD != NULL )
		D.user_data->Set( *pUD );
	else
		D.user_data->Clear();
	return D.user_data;
}

TSonorkId&
 TSonorkClipData::SetUserId(const TSonorkId*user_id)
{
	Clear();
	D_type = SONORK_CLIP_DATA_USER_ID;
	if( user_id != NULL )
		D.user_id.Set(*user_id);
	else
		D.user_id.Clear();
	return D.user_id;
}

const	TSonorkUserData*
 TSonorkClipData::GetUserData() const
{
	if( D_type == SONORK_CLIP_DATA_USER_DATA )
		return D.user_data;
	return NULL;
}

SONORK_C_CSTR
 TSonorkClipData::GetCStr()	const
{
	switch( D_type )
	{
		case SONORK_CLIP_DATA_TEXT:
		case SONORK_CLIP_DATA_URL:
		case SONORK_CLIP_DATA_FILE_NAME:
			return D.sh_str_ptr->CStr();

		case SONORK_CLIP_DATA_USER_DATA:
			return D.user_data->alias.CStr();

	}
	return "";
}

bool
 TSonorkClipData::SetCStr(SONORK_CLIP_DATA_TYPE dt,SONORK_C_CSTR str)
{
	Clear();
	if( str != NULL )
	{
		D_type	= dt;
		D.sh_str_ptr = new TSonorkShortString( str );
		return true;
	}
	return false;
}

void*
 TSonorkClipData::ReleaseData()
{
	void*    RV;
	RV	= D.ptr;
	D_type	= SONORK_CLIP_DATA_NONE;
	D.ptr	= NULL;
	return RV;
}

TSonorkShortStringQueue*
 TSonorkClipData::SetShortStringQueue(SONORK_CLIP_DATA_TYPE dt)
{
	Clear();
	D_type	= dt;
	SONORK_MEM_NEW( D.sh_str_que  = new TSonorkShortStringQueue() );
	return D.sh_str_que;
}

void
 TSonorkClipData::SetSonorkClipDataQueue()
{
	Clear();
	D_type	= SONORK_CLIP_DATA_LIST;
	SONORK_MEM_NEW( D.clip_que  = new TSonorkClipDataQueue() );
}

bool
 TSonorkClipData::AddSonorkClipData(TSonorkClipData*CD)
{
	if( D_type == SONORK_CLIP_DATA_LIST && CD!=NULL )
	{
		CD->AddRef();
		D.clip_que->Add( CD );
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------
// T_GuDropSourceData

TSonorkDropSourceData::TSonorkDropSourceData()
{
	ref_count 		= 1;
	DD			= new TSonorkClipData;
	_GuDropSourceData_cc++;
	//TRACE_DEBUG("T_GuDropSourceData[%x]::ALLOC (DD=%08x) Items=%u",this,DD,_GuDropSourceData_cc);
}
TSonorkDropSourceData::TSonorkDropSourceData(const TSonorkId&user_id)	// starts with ref_count = 1
{
	ref_count 		= 1;
	DD			= new TSonorkClipData;
	DD->SetUserId(&user_id);
	_GuDropSourceData_cc++;
	//TRACE_DEBUG("T_GuDropSourceData[%x]::ALLOC (DD=%08x) Items=%u",this,DD,_GuDropSourceData_cc);
}
TSonorkDropSourceData::TSonorkDropSourceData(TSonorkClipData*pDD)
{
	ref_count 		= 1;
	DD 			= pDD;
	DD->AddRef();
	_GuDropSourceData_cc++;
//	TRACE_DEBUG("T_GuDropSourceData[%x]::ALLOC (DD=%08x) Items=%u",this,DD,_GuDropSourceData_cc);
}

TSonorkDropSourceData::~TSonorkDropSourceData()
{
	_GuDropSourceData_cc--;
//	TRACE_DEBUG("T_GuDropSourceData[%x]::FREE (DD=%x) Items=%u",this,DD,_GuDropSourceData_cc);
	Clear();
}

ULONG __stdcall TSonorkDropSourceData::AddRef(void)
{
	//TRACE_DEBUG("TSonorkDropSourceData::AddRef(%u)",ref_count+1);
	return ++ref_count;
}
ULONG __stdcall TSonorkDropSourceData::Release(void)
{
	DWORD tempRef = --ref_count;
	//TRACE_DEBUG("TSonorkDropSourceData::Release(%u)",tempRef);
	if(tempRef == 0)
		delete this;
	return tempRef;
}

HRESULT __stdcall TSonorkDropSourceData::EnumFormatEtc(DWORD dwDir,  IEnumFORMATETC ** ppFE)
{
//	TRACE_DEBUG("TSonorkDropSourceData::EnumFormatEtc(dir=%u / GET=%u)",dwDir, DATADIR_GET);
	if( dwDir == DATADIR_GET )
	{
		TSonorkDropSourceDataEnum *E;
		E= new TSonorkDropSourceDataEnum( DD );
		*ppFE = E;
		E->AddRef();
		return S_OK;
	}
	return E_NOTIMPL ;
}
HRESULT __stdcall TSonorkDropSourceData::QueryGetData(FORMATETC * pFE)
{
	HRESULT rv;
	if( DD == NULL )
		rv = E_UNEXPECTED ;
	else
	if( !DD->HasClipFormat(pFE->cfFormat) )
	{
		rv = DV_E_FORMATETC ;
	}
	else
	{
		if( pFE->lindex		!= -1)
			rv = DV_E_LINDEX ;
		else
		if( pFE->tymed		!= TYMED_HGLOBAL)
			rv = DV_E_TYMED;
		else
		if( pFE->dwAspect 	!= DVASPECT_CONTENT)
			rv = DV_E_DVASPECT ;
		else
			rv = S_OK;
	}
	if( rv != S_OK )
	{
		char cf_name[48];
		if(!GetClipboardFormatName(pFE->cfFormat,cf_name,32))
			strcpy(cf_name,"<internal>");
	}
	return rv;
}
HRESULT __stdcall
 TSonorkDropSourceData::SetData(FORMATETC * ,  STGMEDIUM * ,  BOOL )
{
	return E_NOTIMPL;
}

void
 TSonorkDropSourceData::Clear()
{
	if(DD != NULL )
	{
		DD->Release();
		DD = NULL;
	}
}

BOOL
 TSonorkDropSourceData::GetData(CLIPFORMAT cf,void*buffer,DWORD& buffer_size)
{
	BOOL rv;
	if( DD != NULL )
	{
		if( cf == CF_TEXT)
			rv = DD->GetTextFormat((char*)buffer,buffer_size);
		else
		if( cf == cfSonorkClipData)
		{
			rv = DD->CODEC_WriteMem((BYTE*)buffer,buffer_size) == SONORK_RESULT_OK;
		}
		else
		if( cf == DD->GetExtClipFormat() )
		{
			rv = DD->GetExtFormat(buffer,buffer_size);
		}
		else
			rv=false;
	}
	else
		rv=false;
	return rv;
}

HRESULT __stdcall
 TSonorkDropSourceData::GetData(FORMATETC * pFE,STGMEDIUM * pMD )
{
	HRESULT rv;
	DWORD	data_size;
	if( (rv = QueryGetData(pFE)) == S_OK )
	{
		pMD->tymed			= TYMED_HGLOBAL;
		pMD->pUnkForRelease	= NULL;
		data_size = DD->GetFormatSize(pFE->cfFormat);
		if( data_size == 0 )
			rv = E_UNEXPECTED ;
		else
		{
			void	*ptr;
			ptr = (void*)pMD->hGlobal 	= GlobalAlloc(GPTR	, data_size );
			if( ptr == NULL )
				rv = E_OUTOFMEMORY;
			else
			{
				if(GetData( pFE->cfFormat, ptr , data_size ))
					rv = S_OK;
				else
				{
					rv = E_UNEXPECTED ;
					GlobalFree( ptr );
					pMD->hGlobal = NULL;
				}
			}
		}
	}
	return rv;
}
HRESULT __stdcall
 TSonorkDropSourceData::GetDataHere(FORMATETC * pFE,STGMEDIUM * pMD )
{
	HRESULT rv;
	void 	*ptr;
	DWORD	data_size;

	if( (rv = QueryGetData(pFE)) == S_OK )
	{
		if( pMD->tymed	 != TYMED_HGLOBAL)
			rv = DV_E_FORMATETC ;
		else
		if( GlobalFlags(pMD->hGlobal) & GMEM_INVALID_HANDLE )
			rv = E_INVALIDARG ;
		else
		{
			data_size = DD->GetFormatSize(pFE->cfFormat);
			if( data_size == 0 )
				rv = E_UNEXPECTED ;
			else
			if(GlobalSize(pMD->hGlobal) < data_size )
				rv = STG_E_MEDIUMFULL ;
			else
			if( (ptr = GlobalLock(pMD->hGlobal)) == NULL )
				rv = E_UNEXPECTED ;
			else
			{
				if( GetData( pFE->cfFormat, ptr , data_size ) )
					rv = S_OK;
				else
					rv = E_UNEXPECTED ;
				GlobalUnlock( pMD->hGlobal );
			}
		}
	}
	return rv;

}
HRESULT __stdcall
 TSonorkDropSourceData::GetCanonicalFormatEtc(FORMATETC * ,FORMATETC * )
{
	return DATA_S_SAMEFORMATETC;
}

HRESULT __stdcall
 TSonorkDropSourceData::DAdvise(FORMATETC* ,DWORD ,IAdviseSink *,DWORD * )
{
	return OLE_E_ADVISENOTSUPPORTED ;
}
HRESULT __stdcall
 TSonorkDropSourceData::DUnadvise(DWORD )
{
	return OLE_E_ADVISENOTSUPPORTED ;
}
HRESULT __stdcall
 TSonorkDropSourceData::EnumDAdvise(IEnumSTATDATA ** )
{
	return OLE_E_ADVISENOTSUPPORTED ;
}
HRESULT __stdcall
 TSonorkDropSourceData::QueryInterface(
			REFIID	iid
			,void __RPC_FAR *__RPC_FAR *O)
{
	*O=NULL;
	if (iid == IID_IUnknown || iid == IID_IDataObject)
	{
		*O = this;
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE ;
}

// ----------------------------------------------------------------------
// TSonorkDropSource

TSonorkDropSource::TSonorkDropSource()
{
	ref_count=1;
	//TRACE_DEBUG("\t + %08x T_GuDropSource",this);
}
TSonorkDropSource::~TSonorkDropSource()
{
	//TRACE_DEBUG("\t - %08x T_GuDropSource",this);
}

HRESULT __stdcall TSonorkDropSource::QueryInterface(
			REFIID 	iid
			,void __RPC_FAR *__RPC_FAR *O)
{
//	TRACE_DEBUG("TSonorkDropSource::QueryInterface");
	*O=NULL;
	// interfaces
	if (iid == IID_IUnknown || iid == IID_IDropSource)
	{
		*O = this;
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE ;
}
ULONG __stdcall TSonorkDropSource::AddRef(void)
{
	return ++ref_count;
}
ULONG __stdcall TSonorkDropSource::Release(void)
{
	return --ref_count;
}
HRESULT __stdcall TSonorkDropSource::QueryContinueDrag(BOOL fEscapePressed,DWORD grfKeyState)
{
	if(fEscapePressed)return DRAGDROP_S_CANCEL;
	if(grfKeyState & MK_LBUTTON)return S_OK;
	return DRAGDROP_S_DROP ;
}
HRESULT __stdcall TSonorkDropSource::GiveFeedback(DWORD )
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

// ----------------------------------------------------------------------
// T_GuDropSourceDataEnum

TSonorkDropSourceDataEnum::TSonorkDropSourceDataEnum(TSonorkClipData	*pDD)
{
	DD	= pDD;
	DD->AddRef();
	ref_count	= 0;
	index		= 0;
	max_index 	= DD->GetClipFormatCount();
	_GuDropSourceDataEnum_cc++;
	//TRACE_DEBUG("SourceDataEnum[%x]:ALLOC DD=%08x Items=%u",this,DD,	_GuDropSourceDataEnum_cc);
}
TSonorkDropSourceDataEnum::~TSonorkDropSourceDataEnum()
{
	_GuDropSourceDataEnum_cc--;
	//TRACE_DEBUG("SourceDataEnum[%x]:FREE  DD=%08x Items=%u",this,DD,	_GuDropSourceDataEnum_cc);
	DD->Release();
}

HRESULT __stdcall TSonorkDropSourceDataEnum::QueryInterface(
			REFIID 	iid
			,void __RPC_FAR *__RPC_FAR *O)
{
//	TRACE_DEBUG("TSonorkDropSourceDataEnum::QueryInterface");
	*O=NULL;
	// interfaces
	if (iid == IID_IUnknown || iid == IID_IEnumFORMATETC)
	{
		*O = this;
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE ;
}

ULONG __stdcall TSonorkDropSourceDataEnum::AddRef(void)
{
//	TRACE_DEBUG("TSonorkDropSourceDataEnum::AddRef(%u)",ref_count+1);
	return ++ref_count;
}
ULONG __stdcall TSonorkDropSourceDataEnum::Release(void)
{
	DWORD tempRef = --ref_count;
//	TRACE_DEBUG("TSonorkDropSourceDataEnum::Release(%u)",tempRef);
	if(tempRef == 0)
		delete this;
	return tempRef;
}
HRESULT __stdcall TSonorkDropSourceDataEnum::Next(ULONG celt, FORMATETC * rgelt,ULONG * pceltFetched)
{
	ULONG fetched;

	fetched = 0;
	if( index < max_index )
	{
		ULONG items_to_fetch;
		FORMATETC * pFE;
//		char		cf_name[32];
		pFE = rgelt;
		items_to_fetch = max_index - index;
		if( items_to_fetch > celt )
			items_to_fetch = celt;
		while( fetched < items_to_fetch )
		{
			pFE->cfFormat 	= DD->GetClipFormat( index );
			pFE->ptd		= NULL;
			pFE->lindex		= -1;
			pFE->tymed		= TYMED_HGLOBAL;
			pFE->dwAspect 	= DVASPECT_CONTENT;
			/*
			if(!GetClipboardFormatName(pFE->cfFormat,cf_name,32))
				strcpy(cf_name,"<internal>");
			TRACE_DEBUG("SRC_ENUM[%u(%u)/%u]: %u=%s",index,celt,max_index,pFE->cfFormat,cf_name);
			*/
			fetched ++;
			index   ++;
			pFE++;
		}
	}
	if(pceltFetched!=NULL)
		*pceltFetched=fetched;
	return fetched==celt?S_OK:S_FALSE;
}
HRESULT __stdcall TSonorkDropSourceDataEnum::Skip(ULONG celt)
{
	if(index + celt < max_index)
	{
		index+=celt;
		return S_OK;
	}
	return S_FALSE;
}
HRESULT __stdcall TSonorkDropSourceDataEnum::Reset(void)
{
	//TRACE_DEBUG("TSonorkDropSourceDataEnum::Reset(index=%u)",index);
	index=0;
	return S_OK;
}
HRESULT __stdcall TSonorkDropSourceDataEnum::Clone(IEnumFORMATETC ** ppFE)
{
	TSonorkDropSourceDataEnum *E;
	E=new TSonorkDropSourceDataEnum( DD );
	E->index = this->index;
	E->AddRef();
	*ppFE = E;
	return S_OK;
}


// ----------------------------------------------------------------------
// TSonorkDropTarget

ULONG __stdcall TSonorkDropTarget::AddRef(void)
{
	return ++drag.ref_count;
}
ULONG __stdcall TSonorkDropTarget::Release(void)
{
	return --drag.ref_count;
}

TSonorkDropTarget::TSonorkDropTarget()
{
	drag.flags		=0;
	drag.owner_hwnd	=NULL;
	drag.ctrl_hwnd	=NULL;//target;
}
TSonorkDropTarget::TSonorkDropTarget(HWND owner,UINT id)
{
	drag.flags		=0;
	drag.owner_hwnd	=NULL;
	drag.ctrl_hwnd	=NULL;//target;
	AssignCtrl(owner,id);
}
TSonorkDropTarget::TSonorkDropTarget(HWND owner,HWND ctrl_hwnd)
{
	drag.flags		=0;
	drag.owner_hwnd	=NULL;
	drag.ctrl_hwnd	=NULL;//target;
	AssignCtrl(owner,ctrl_hwnd);
}

void	TSonorkDropTarget::AssignCtrl(HWND owner,UINT id)
{
	AssignCtrl(owner,::GetDlgItem(owner,id));
}

void	TSonorkDropTarget::AssignCtrl(HWND owner,HWND ctrl_hwnd)
{
	if( ctrl_hwnd != NULL )
	{
		drag.ctrl_hwnd		=ctrl_hwnd; //ow->GetDlgItem( id );
		drag.owner_hwnd		=owner;
		Enable( true );
	}
}

TSonorkDropTarget::~TSonorkDropTarget()
{
	Enable(false);
}



void TSonorkDropTarget::Enable(BOOL v)
{
	UINT r;
	if( drag.owner_hwnd == NULL || drag.ctrl_hwnd == NULL )return;
	if(v)
	{
		if(IsEnabled())return;
		r=RegisterDragDrop(drag.ctrl_hwnd,this);
		if(r==S_OK)
		{
			drag.flags|=SONORK_DT_ENABLED;
		}
	}
	else
	{
		if(!IsEnabled())return;
		RevokeDragDrop( drag.ctrl_hwnd );
		drag.flags&=~SONORK_DT_ENABLED;
	}
}

void TSonorkDropTarget::SndQueryAccept(TSonorkDropQuery *Q)
{
	::SendMessage(drag.owner_hwnd,_GuDropCallbackMessage,SONORK_DRAG_DROP_QUERY,(LPARAM)Q);
/*	TRACE_DEBUG("QueryAccept(CP_%x,%s) -> %x (%u)"
		,Q->CpFormat()
		,Q->FormatName()
		,Q->Accepted()
		,Q->PrefLevel());
*/
}

HRESULT __stdcall
	TSonorkDropTarget::DragEnter(IDataObject __RPC_FAR *pDataObj
		,DWORD grfKeyState,POINTL pt,DWORD __RPC_FAR *pwdEffect)
{
	IEnumFORMATETC *E;
	FORMATETC F;
	ULONG aux;
	TSonorkDropQuery Q(this,pwdEffect,grfKeyState,&pt);

	drag.list.Clear();
	::SendMessage(drag.owner_hwnd,_GuDropCallbackMessage,SONORK_DRAG_DROP_INIT,0);
	if(pDataObj->EnumFormatEtc(DATADIR_GET,&E)==S_OK)
	{
		if(E)
		{
			*pwdEffect=DROPEFFECT_COPY;
			Q.data.pDataObj = pDataObj;
			while(E->Next(1,&F,&aux)==S_OK)
			{
                if(!aux)break;
				Q.data.SetFormat(F.cfFormat);
				if(GetClipboardFormatName(
					 Q.data.CpFormat()
					,Q.data.name
					,sizeof(Q.data.name)-1) == false)
					Q.data.name[0]=0;
				SndQueryAccept(&Q);
				if(Q.Accepted())
				{
					drag.list.AddItem( &Q.data );
					if(Q.data.PrefLevel()>=SONORK_DROP_ACCEPT_MAX_LEVEL)
						break;
				}
			}
			E->Release();
		}
	}
	if( !drag.list.Items() )
		*pwdEffect	   = DROPEFFECT_NONE;
	return S_OK;
}

HRESULT __stdcall TSonorkDropTarget::DragLeave( void)
{
	//TRACE_DEBUG("TSonorkDropTarget::DragLeave(Items=%u)",DataList().Items());
	if(  drag.list.Items() )
	{
		drag.list.Clear();
		::SendMessage(drag.owner_hwnd,_GuDropCallbackMessage,SONORK_DRAG_DROP_CANCEL,0);
	}

	return S_OK;
}

HRESULT __stdcall TSonorkDropTarget::DragOver(
	 DWORD	grfKeyState
	,POINTL	pt
	,DWORD __RPC_FAR *pwdEffect)
{
	if(drag.list.Items()==0)
		*pwdEffect=DROPEFFECT_NONE;
	else
		::SendMessage(drag.owner_hwnd,_GuDropCallbackMessage,SONORK_DRAG_DROP_UPDATE, (LPARAM)&TSonorkDropMsg(this,pwdEffect,grfKeyState,&pt));
	return S_OK;
}

HRESULT __stdcall TSonorkDropTarget::Drop(
  IDataObject __RPC_FAR *pDataObj
  ,DWORD grfKeyState
  ,POINTL pt
  ,DWORD __RPC_FAR *pdwEffect)
{
//	TRACE_DEBUG("TSonorkDropTarget::Drop(Items=%u,EFF=%u)",DataList().Items(),*pdwEffect);
	if( drag.list.Items()>0 )
	{
		TSonorkDropExecute	E(this,pDataObj,pdwEffect,grfKeyState,&pt);

		/*
		TSonorkDropTargetData	*fmt;
		TSonorkListIterator I;
		drag.list.InitEnum(I);

		while( (fmt=drag.list.EnumNext(I)) != NULL)
		{
			TRACE_DEBUG("InDropFormat: Cp:%04x Pref:%02u Name:%s"
				,fmt->CpFormat()
				,fmt->PrefLevel()
				,fmt->FormatName());
		}
		*/
		::SendMessage(drag.owner_hwnd,_GuDropCallbackMessage,SONORK_DRAG_DROP_EXECUTE,(LPARAM)&E);
		drag.list.Clear();
	}
  return S_OK;
}

HRESULT __stdcall TSonorkDropTarget::QueryInterface(
			REFIID 	iid
			,void __RPC_FAR *__RPC_FAR *O)
{
	//TRACE_DEBUG("TSonorkDropTarget::QueryInterface");
	*O=NULL;
	// interfaces
	if (iid == IID_IUnknown || iid == IID_IDropTarget)
	{
		*O = this;
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE ;
}
void TSonorkDropDataList::Clear()
{
	TSonorkListIterator I;
	TSonorkDropTargetData  *fmt;
    InitEnum(I);
	while( (fmt=EnumNext(I)) != NULL)
		fmt->ReleaseData();
    TSonorkSimpleDataList::Clear();

}
TSonorkDropTargetData*	TSonorkDropDataList::GetFormat(CLIPFORMAT cf)
{
	TSonorkListIterator I;
    TSonorkDropTargetData  *fmt;
    InitEnum(I);
    while( (fmt=EnumNext(I)) != NULL)
      if(fmt->CpFormat()==cf)
      	return fmt;
	return NULL;
}
TSonorkDropTargetData*	TSonorkDropDataList::GetFormat(const char *name)
{
	TSonorkListIterator I;
    TSonorkDropTargetData  *fmt;
	InitEnum(I);
    while( (fmt=EnumNext(I)) != NULL)
      if(!stricmp(name,fmt->FormatName()))
      	return fmt;
	return NULL;
}
TSonorkDropTargetData*	TSonorkDropDataList::GetBestFormat()
{
	TSonorkListIterator I;
    TSonorkDropTargetData  *fmt,*b_fmt;
    b_fmt=NULL;
    InitEnum(I);
    while( (fmt=EnumNext(I)) != NULL)
    {
		if(b_fmt)
        {
        	if(b_fmt->PrefLevel()>=fmt->PrefLevel())
            	continue;
        }
        b_fmt=fmt;
	}
	return b_fmt;
}
void TSonorkDropTargetData::SetFormat(CLIPFORMAT cf)
{
	cp_format=cf;
    pref_level=0;
	storage_medium.tymed=TYMED_NULL;
	data_ptr=NULL;
}
TSonorkDropTargetData::TSonorkDropTargetData()
{
	pDataObj = NULL ;
    SetFormat(0);
}

void TSonorkDropTargetData::ReleaseData()
{
    if(storage_medium.tymed!=TYMED_NULL)
    {
	    //TRACE_DEBUG("FREE_DATA(%04x,%s)",CpFormat(),FormatName());
	    if( data_ptr!=NULL )
        {
		    GlobalUnlock(storage_medium.hGlobal);
            data_ptr=NULL;
		}
		ReleaseStgMedium(&storage_medium);
		storage_medium.tymed=TYMED_NULL;

    }
}
const BYTE*TSonorkDropTargetData::GetData(int lindex,DWORD*d_size)
{
	FORMATETC		FETC;
	DWORD 			aux;

	ReleaseData();

	//TRACE_DEBUG("GRAB_DATA(%04x,%s)",CpFormat(),FormatName());
	FETC.cfFormat	=CpFormat();
	FETC.ptd		=NULL;
	FETC.dwAspect	=DVASPECT_CONTENT;
	FETC.lindex		=lindex;
	FETC.tymed		=TYMED_HGLOBAL;
	aux				=pDataObj->GetData(&FETC,&storage_medium);
	if(aux==S_OK)
	{
		if(storage_medium.tymed==TYMED_HGLOBAL)
		{
			//TRACE_DEBUG("GlobalFlags()=%x",GlobalFlags(storage_medium.hGlobal));
			if((data_ptr=GlobalLock(storage_medium.hGlobal))!=NULL)
			{
				if(d_size!=NULL)
					*d_size=GlobalSize(storage_medium.hGlobal);
				return (const BYTE*)data_ptr;
			}
		}
	}
	if(d_size!=NULL)*d_size=0;
	ReleaseData();
	return NULL;
}

// ----------------------------------------------------------------------
// TSonorkDropMsg

TSonorkDropMsg::TSonorkDropMsg(TSonorkDropTarget*T,DWORD __RPC_FAR *p_pdwEffect,DWORD p_grfKeyState,POINTL*p_pt)
{
	target 		=T;
	drop_effect	=p_pdwEffect;
	key_state	=p_grfKeyState;
	point		=p_pt;
}



void SONORK_DragDropInit(UINT pMsg)
{
	OleInitialize( NULL );
	_GuDropCallbackMessage	= pMsg;
	cfSonorkClipData	= (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SONORKCLIPDATA);
	cfFileName			= (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILENAME);
	cfFileDescr			= (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
	cfShellUrl			= (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLURL);
	_GuClipData_cc			=
	_GuDropSourceData_cc		=
	_GuDropSourceDataEnum_cc	=0;
	_GuDropSource  = new TSonorkDropSource;
}
void SONORK_DragDropExit()
{
	if( _GuDropSource != NULL )
	{
		delete _GuDropSource;
		_GuDropSource=NULL;
	}
	OleUninitialize();
}

/*
	FILEGROUPDESCRIPTOR	F;
	FILEDESCRIPTOR *	fd;
	F.cItems = 1;
	fd = &F.fgd[0];
	fd->dwFlags = 0;
	lstrcpyn(fd->cFileName,D.cstr,sizeof(fd->cFileName));
	buffer_size =	sizeof(FILEGROUPDESCRIPTOR);
	memcpy(buffer , &F , buffer_size);
*/

