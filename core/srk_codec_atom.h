#if !defined(SONORK_CODEC_ATOM_H)
#define SONORK_CODEC_ATOM_H

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL)
	which is based on the GNU General Public License (GPL).

	You should have received a copy of the SSCL	along with this program;
	if not, write to sscl@sonork.com.

	You may NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL).

	This comment section, indicating the existence and requirement of
	acceptance of the SSCL may not be removed from the source code.
*/

#include "srk_defs.h"


#define SONORK_CODEC_DESCRIPTOR_SIZE_MASK	0x001fffff
#define SONORK_CODEC_DESCRIPTOR_TYPE_MASK	0xffe00000
#define	SONORK_CODEC_DESCRIPTOR_TYPE_SHIFT	21
#define SONORK_CODEC_ATOM_MIN_DATA_SIZE     0
#define SONORK_CODEC_ATOM_MAX_DATA_SIZE		SONORK_CODEC_DESCRIPTOR_SIZE_MASK
typedef DWORD SONORK_CODEC_DESCRIPTOR;

#if SONORK_CODEC_LEVEL>5
class TSonorkDynData;
#endif

struct TSonorkCodecAtom
{
friend	class TSonorkCodecReader;
friend	class TSonorkCodecWriter;
private:
	virtual void	CODEC_WriteDataMem	(TSonorkCodecWriter&) const =0;
	virtual void	CODEC_ReadDataMem	(TSonorkCodecReader&)=0;
	virtual DWORD	CODEC_DataSize() const =0;//{return 0;}

protected:
	TSonorkCodecAtom();
public:
	virtual ~TSonorkCodecAtom();
	virtual SONORK_ATOM_TYPE	CODEC_DataType() const =0;
	virtual void 			CODEC_Clear()=0;

	DWORD		CODEC_Size() 	const;
	SONORK_RESULT	CODEC_Copy(const TSonorkCodecAtom*);

	SONORK_RESULT 	CODEC_WriteMem(BYTE*	,DWORD&size) const;
	SONORK_RESULT 	CODEC_ReadMem(const 	BYTE*,DWORD&size);
	SONORK_RESULT 	CODEC_ReadMemNoSize(const BYTE*,DWORD size);

	// Returns string representation of atom, returns buffer
	virtual char* CODEC_GetTextView(char*buffer,DWORD size) const;

#if SONORK_CODEC_LEVEL>5
	SONORK_RESULT 	CODEC_Write(TSonorkDynData*) const;
	SONORK_RESULT 	CODEC_Read(const TSonorkDynData*);
#endif
};

struct TSonorkCodecNullAtom
:public TSonorkCodecAtom
{
   SONORK_ATOM_TYPE	CODEC_DataType()	const	{return SONORK_ATOM_NULL; }
   DWORD		CODEC_DataSize()	const	{return 0;}
   void			CODEC_WriteDataMem		(TSonorkCodecWriter&) const {}
   void			CODEC_ReadDataMem		(TSonorkCodecReader&){}
   void 		CODEC_Clear(){}
};

struct TSonorkCodecRawAtom
:public TSonorkCodecAtom
{
private:
	void *				data;
	DWORD				data_size;
	SONORK_ATOM_TYPE		data_type;

	void	CODEC_WriteDataMem	(TSonorkCodecWriter&) const;
	void	CODEC_ReadDataMem	(TSonorkCodecReader&);

public:
	TSonorkCodecRawAtom(void *p_data, DWORD p_data_size,SONORK_ATOM_TYPE p_data_type=SONORK_ATOM_RAW);
	TSonorkCodecRawAtom(SONORK_CODEC_DESCRIPTOR,void*data);

	SONORK_ATOM_TYPE	CODEC_DataType()	const	{	return data_type;	}
	DWORD 			CODEC_DataSize()	const	{	return data_size;	}

	void    CODEC_Clear(){	memset(data,0,data_size);	}

	void	*Data()				{	return data;	 	}
	DWORD	DataSize() 	const	{	return data_size;	}
};

struct TSonorkCodecDW
:public TSonorkCodecAtom
{
private:
	void *				data;
	DWORD 				data_size;
	SONORK_ATOM_TYPE		data_type;

	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;
	void	CODEC_ReadDataMem(TSonorkCodecReader&);

public:
	TSonorkCodecDW(void *p_data, DWORD p_data_size,SONORK_ATOM_TYPE p_data_type=SONORK_ATOM_RAW);

	SONORK_ATOM_TYPE CODEC_DataType()
			const	{	return data_type;	}

	DWORD	CODEC_DataSize()
			const	{	return data_size;	}

	void    CODEC_Clear()
			{	memset(data,0,data_size);	}

	void	*Data()
			{	return data;	 	}

	DWORD	DataSize() 	const
			{	return data_size;	}
};


SONORK_CODEC_DESCRIPTOR SONORK_CODEC_E_Descriptor(SONORK_ATOM_TYPE,DWORD sz);
void  				SONORK_CODEC_D_Descriptor(SONORK_CODEC_DESCRIPTOR descr,SONORK_ATOM_TYPE&,DWORD&sz);

#endif