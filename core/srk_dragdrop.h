#if !defined(SRK_DRAGDROP_H)
#define SRK_DRAGDROP_H

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


#include <shlobj.h>


#include "srk_codec_io.h"

enum SONORK_DRAG_DROP_EVENT
{
  SONORK_DRAG_DROP_INIT
, SONORK_DRAG_DROP_QUERY
, SONORK_DRAG_DROP_UPDATE
, SONORK_DRAG_DROP_EXECUTE
, SONORK_DRAG_DROP_CANCEL
};


#if !defined(CFSTR_SHELLURL)
#define CFSTR_SHELLURL	 		"UniformResourceLocator"
#endif
#define CFSTR_SONORKCLIPDATA	"SonorkClipData"


#define SONORK_DROP_ACCEPT_MAX_LEVEL		1000

#define SONORK_DROP_ACCEPT_F_SONORKCLIPDATA	0x0001
#define SONORK_DROP_ACCEPT_F_FILES		0x0002
#define SONORK_DROP_ACCEPT_F_URL		0x0004
#define SONORK_DROP_ACCEPT_F_TEXT		0x0008


enum SONORK_CLIP_DATA_TYPE
{
   SONORK_CLIP_DATA_NONE	// NULL
,  SONORK_CLIP_DATA_TEXT	// char*
,  SONORK_CLIP_DATA_URL		// char*
,  SONORK_CLIP_DATA_FILE_NAME	// char*
,  SONORK_CLIP_DATA_USER_ID	// TSonorkId
,  SONORK_CLIP_DATA_USER_DATA	// TSonorkUserData*
,  SONORK_CLIP_DATA_FILE_LIST	// TSonorkShortStringQueue*
,  SONORK_CLIP_DATA_LIST	// TSonorkClipDataQueue*
};
class TSonorkClipDataQueue;

struct TSonorkClipData
:public TSonorkCodecAtom
{
private:
	SONORK_CLIP_DATA_TYPE 		D_type;
	UINT				ref_count;
	union {
		void*			ptr;
		TSonorkId   		user_id;
		TSonorkUserData*	user_data;
		TSonorkShortString*	sh_str_ptr;
		TSonorkShortStringQueue*sh_str_que;
		TSonorkClipDataQueue*	clip_que;
	}D;
//	UINT	D_size;
	~TSonorkClipData();

public:
	// NB! Constructors starts with ref_count = 1
	TSonorkClipData();
	void	Clear();

	SONORK_CLIP_DATA_TYPE
		DataType()
		const { return D_type; }

	const BYTE*
		DataPtr()
		const {	return (const BYTE*)D.ptr;	}

	const TSonorkId&
		GetUserId()
		const { return D.user_id;}

const	TSonorkUserData*
		GetUserData() const;

	TSonorkId&
		SetUserId(const TSonorkId*user_id);

	TSonorkUserData*
		SetUserData(const TSonorkUserData* );

	SONORK_C_CSTR
		GetCStr()	const;
	bool
		SetCStr(SONORK_CLIP_DATA_TYPE,SONORK_C_CSTR);

	TSonorkShortStringQueue*
		SetShortStringQueue(SONORK_CLIP_DATA_TYPE);
		
const	TSonorkShortStringQueue*
		GetShortStringQueue()
		const	{ return D.sh_str_que;}

	void
		SetSonorkClipDataQueue();

const 	TSonorkClipDataQueue*
		GetSonorkClipDataQueue()	const
		{	return D.clip_que;}

	bool
		AddSonorkClipData(TSonorkClipData*);

	// ReleaseData() return data.ptr and sets
	// data.ptr to NULL and data_type to none
	void*    	ReleaseData();

	void		AddRef( void);
	void		Release( void);
	UINT		GetClipFormatCount()				const;
	CLIPFORMAT	GetClipFormat(UINT index)			const;
	bool		HasClipFormat(CLIPFORMAT)			const;
	UINT		GetFormatSize(CLIPFORMAT)			const;

	bool
		HasTextFormat() const;

	UINT
		GetTextFormatMinSize() const ;

	UINT
		GetTextFormat(char*,DWORD max_size)	const;

	UINT
		GetTextFormat(TSonorkShortString*) const;


	CLIPFORMAT
		GetExtClipFormat() const;

	BOOL
		HasExtFormat()   const
		{ return GetExtClipFormat() != 0; }

	UINT
		GetExtFormatSize() const ;

	BOOL
		GetExtFormat(void* ,DWORD& max_size);

// -----------------
// CODEC

public:

	void 	CODEC_Clear()
		{	Clear();	}

	SONORK_ATOM_TYPE
		CODEC_DataType() const
		{	return SONORK_ATOM_CLIPDATA_01;}
private:

	void	CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const;
	void	CODEC_ReadDataMem(TSonorkCodecReader& CODEC);
	DWORD	CODEC_DataSize()	const;
	
};

DeclareSonorkQueueClass( TSonorkClipData );

struct TSonorkDropTargetData
{
private:
friend class  		TSonorkDropTarget;

	CLIPFORMAT		cp_format;
	UINT			pref_level;
	STGMEDIUM 		storage_medium;
	IDataObject __RPC_FAR *pDataObj;
	char			name[48];
	const void		*data_ptr;

	void SetFormat(CLIPFORMAT cf);

public:
	TSonorkDropTargetData();

	CLIPFORMAT
		CpFormat() const
		{ return cp_format; }

	UINT
		PrefLevel() const
		{ return pref_level;}

	void
		Accept(int level)
		{ pref_level=level;}

	BOOL
		Accepted()	const
		{ return pref_level>0;}

	SONORK_C_CSTR
		FormatName() const
		{ return name;}

	const BYTE*
		GetData(int lindex=-1,DWORD*size=NULL);

	void
		ReleaseData();


	HDROP			GetHDrop()	{ return (HDROP)GetData(-1); }
	const char*		GetStr(int lindex=-1)
					{ return (const char*)GetData(lindex);}
	FILEGROUPDESCRIPTOR*GetFileGroup(int lindex)
					{ return (FILEGROUPDESCRIPTOR*)GetData(lindex);}
};




StartSonorkSimpleDataListClass(TSonorkDropDataList,TSonorkDropTargetData,8)
	void Clear();
	TSonorkDropTargetData*	GetFormat(CLIPFORMAT);
	TSonorkDropTargetData*	GetFormat(const char *name);
	TSonorkDropTargetData*	GetBestFormat();

EndSonorkSimpleDataListClass;




#define SONORK_DT_ENABLED	0x0001
class TSonorkDropTarget: public IDropTarget
{
	struct _DRAG{
		HWND			owner_hwnd;
		HWND		   	ctrl_hwnd;
		ULONG          		ref_count;
		DWORD			flags;
		TSonorkDropDataList   	list;
   }drag;

	void SndQueryAccept(struct TSonorkDropQuery*);
	void SndQuerySelect();

	HRESULT __stdcall QueryInterface(REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject);

	ULONG __stdcall AddRef( void);
	ULONG __stdcall Release( void);

	HRESULT __stdcall DragEnter(IDataObject __RPC_FAR *pDataObj,DWORD grfKeyState,POINTL pt,DWORD __RPC_FAR *pdwEffect);
	HRESULT __stdcall DragOver(DWORD grfKeyState,POINTL pt,DWORD __RPC_FAR *pdwEffect);
	HRESULT __stdcall QueryContinueDrag(BOOL fEscapePressed,DWORD grfKeyState)
					{return S_OK;}
	HRESULT __stdcall DragLeave( void);
	HRESULT __stdcall Drop(IDataObject __RPC_FAR *pDataObj,DWORD grfKeyState,POINTL pt,DWORD __RPC_FAR *pdwEffect);

public:
	TSonorkDropTarget();
	TSonorkDropTarget(HWND owner,UINT ctl_id);
	TSonorkDropTarget(HWND owner,HWND ctl_hwnd);
	~TSonorkDropTarget();

	void
		AssignCtrl(HWND owner,HWND ctl_hwnd);
	void
		AssignCtrl(HWND owner,UINT ctl_id);
	void
		Enable(BOOL);
	BOOL
		IsEnabled()	const
		{ return drag.flags&SONORK_DT_ENABLED;}

	HWND	CtrlHandle() const
		{ return drag.ctrl_hwnd;}

	TSonorkDropDataList&
		DataList()
		{ return drag.list;}
};

class TSonorkDropSourceDataEnum: public IEnumFORMATETC
{
	ULONG		ref_count;
	UINT		index,max_index;
	TSonorkClipData	*DD;
	HRESULT __stdcall QueryInterface(REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject);
	HRESULT __stdcall Next(ULONG celt, FORMATETC * rgelt,ULONG * pceltFetched);
	HRESULT __stdcall Skip(ULONG celt);
	HRESULT __stdcall Reset(void);
	HRESULT __stdcall Clone(IEnumFORMATETC ** ppenum);
	~TSonorkDropSourceDataEnum();
public:
	// NB! Constructors starts with ref_count = 1
	TSonorkDropSourceDataEnum(TSonorkClipData*);
	ULONG	__stdcall AddRef(void);
	ULONG	__stdcall Release(void);
};

class TSonorkDropSourceData: public IDataObject
{
	ULONG		ref_count;
	//CLIPFORMAT 	cfFormat;
	TSonorkClipData	*DD;
	HRESULT __stdcall QueryInterface(REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject);
	HRESULT	__stdcall DAdvise(FORMATETC* pFormatetc,DWORD advf,IAdviseSink * pAdvSink,DWORD * pdwConnection);
	HRESULT __stdcall DUnadvise(DWORD dwConnection);
	HRESULT __stdcall EnumDAdvise(IEnumSTATDATA ** );
	HRESULT __stdcall QueryGetData(FORMATETC * pFormatetc);
	HRESULT __stdcall GetData(FORMATETC * pFormatetc,STGMEDIUM * pmedium );
	HRESULT __stdcall GetDataHere(FORMATETC * pFormatetc,STGMEDIUM * pmedium );
	HRESULT __stdcall SetData(FORMATETC * pFormatetc,  STGMEDIUM * pmedium,  BOOL fRelease);
	HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC * pFormatetcIn,FORMATETC * pFormatetcOut);
	HRESULT __stdcall EnumFormatEtc(DWORD dwDirection,  IEnumFORMATETC ** ppenumFormatetc);

	void	Clear();
	~TSonorkDropSourceData();
public:
	// NB! Constructors starts with ref_count = 1
	TSonorkDropSourceData();				// Creates a new TSonorkClipData*
	TSonorkDropSourceData(const TSonorkId&);	// starts with ref_count = 1
	TSonorkDropSourceData(TSonorkClipData*);
	ULONG	__stdcall AddRef(void);
	ULONG	__stdcall Release(void);
	BOOL	GetData(CLIPFORMAT,void*,DWORD& size);
	TSonorkClipData*	GetClipData(){	return DD;	}
};

class TSonorkDropSource: public IDropSource
{
	ULONG          		ref_count;
	HRESULT __stdcall QueryInterface(REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject);

	HRESULT __stdcall QueryContinueDrag(BOOL fEscapePressed,DWORD grfKeyState);
	HRESULT __stdcall GiveFeedback(DWORD dwEffect);

public:
	TSonorkDropSource();
	~TSonorkDropSource();
	ULONG __stdcall AddRef( void);
	ULONG __stdcall Release( void);

};


struct TSonorkDropMsg
{
friend TSonorkDropTarget;
protected:
	TSonorkDropMsg(TSonorkDropTarget*T,DWORD __RPC_FAR *pdwEffect,DWORD grfKeyState,POINTL *pt);
public:
	TSonorkDropTarget	*	target;
	POINTL*				point;
	DWORD				key_state;
	DWORD __RPC_FAR *	drop_effect;
	TSonorkDropDataList&	DataList()	{ return target->DataList();}
};

struct TSonorkDropQuery
:public	TSonorkDropMsg
{
friend TSonorkDropTarget;
private:
	TSonorkDropTargetData		data;
	TSonorkDropQuery(TSonorkDropTarget*T,DWORD __RPC_FAR *pdwEffect,DWORD grfKeyState,POINTL*pt)
		:TSonorkDropMsg(T,pdwEffect,grfKeyState,pt){}
public:
	UINT			PrefLevel()			{ return data.PrefLevel();}
	void			Accept(UINT level)	{ data.Accept(level);}
	BOOL			Accepted()          { return PrefLevel()>0;}
	CLIPFORMAT     	CpFormat()			{ return data.CpFormat();}
	const char*		FormatName()		{ return data.FormatName();}
};


struct TSonorkDropExecute
:public	TSonorkDropMsg
{
friend TSonorkDropTarget;
private:
	IDataObject __RPC_FAR 	*pDataObj;
	TSonorkDropExecute(TSonorkDropTarget*T,IDataObject __RPC_FAR * p_pDataObj, DWORD __RPC_FAR *pdwEffect,DWORD grfKeyState,POINTL*pt)
		:TSonorkDropMsg(T,pdwEffect,grfKeyState,pt){pDataObj=p_pDataObj;}
public:
	IDataObject __RPC_FAR *	DataObject(){ return pDataObj;}

};

extern CLIPFORMAT		cfSonorkClipData;
extern CLIPFORMAT		cfFileName;
extern CLIPFORMAT		cfFileDescr;
extern CLIPFORMAT		cfShellUrl;
extern void SONORK_DragDropInit(UINT cbDragMessage);
extern LONG SONORK_DoDragDrop(IDataObject * pDataObject,DWORD dwOKEffect,DWORD * pdwEffect);
extern void SONORK_DragDropExit();

#endif
