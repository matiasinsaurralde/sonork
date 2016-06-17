#include "srk_defs.h"
#pragma hdrstop
#include "srk_codec_io.h"
#include "srk_crypt.h"

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

DWORD	TSonorkCodecAtom_Count=0;

TSonorkCodecAtom::TSonorkCodecAtom()
{
	TSonorkCodecAtom_Count++;
}

TSonorkCodecAtom::~TSonorkCodecAtom()
{
	TSonorkCodecAtom_Count--;
}

// ----------------------------------------------------------------------------
// TSonorkCodecAtom

DWORD	TSonorkCodecAtom::CODEC_Size() const
{
	return CODEC_DataSize()+sizeof(SONORK_CODEC_DESCRIPTOR);
}
SONORK_RESULT	TSonorkCodecAtom::CODEC_Copy(const TSonorkCodecAtom*O)
{
	BYTE*		buffer;
	DWORD		codec_size;
	SONORK_RESULT   result;
	codec_size = O->CODEC_Size();
	buffer	= SONORK_MEM_ALLOC(BYTE,codec_size);
	result	= O->CODEC_WriteMem( buffer , codec_size );
	if(result==SONORK_RESULT_OK)
		result = this->CODEC_ReadMem(buffer,codec_size);
	SONORK_MEM_FREE(buffer);
	return result;
}

/*UINT	TSonorkCodecAtom::CODEC_MinSize() const
{
	return 0;//CODEC_MinDataSize()+sizeof(SONORK_CODEC_DESCRIPTOR);
}
UINT	TSonorkCodecAtom::CODEC_MaxSize() const
{
	return 0;//CODEC_MaxDataSize()+sizeof(SONORK_CODEC_DESCRIPTOR);

}
*/
char *TSonorkCodecAtom::CODEC_GetTextView(char*pTgt,DWORD size) const
{
	if(size>32)
	{
		sprintf(pTgt,"<ATOM T:%u S:%u>",CODEC_DataType(),CODEC_Size());
	}
	else
		*pTgt=0;
	return pTgt;
}

SONORK_RESULT
	TSonorkCodecAtom::CODEC_WriteMem( BYTE*ptr , DWORD& size ) const
{
	SONORK_RESULT	result;
	if(size<sizeof(SONORK_CODEC_DESCRIPTOR))
	{
		result = SONORK_RESULT_CODEC_ERROR; // SONORK_RESULT_BUFFER_TOO_SMALL;
	}
	else
	{
		SONORK_ATOM_TYPE		data_type;
		SONORK_CODEC_DESCRIPTOR*e_ptr;

		e_ptr=(SONORK_CODEC_DESCRIPTOR*)ptr;
		ptr+=sizeof(SONORK_CODEC_DESCRIPTOR);

		data_type=CODEC_DataType();
		if( CODEC_DataSize() > 0 )
		{
			TSonorkCodecWriter 	CODEC(ptr,size-sizeof(SONORK_CODEC_DESCRIPTOR),data_type);

			CODEC_WriteDataMem(CODEC);

			if( (result =CODEC.Result()) == SONORK_RESULT_OK)
			{
				*e_ptr=SONORK_DWORD( SONORK_CODEC_E_Descriptor(data_type,CODEC.Size()) );
				size=CODEC.Size()+sizeof(SONORK_CODEC_DESCRIPTOR);
			}
			else
			{
				size=0;
			}
		}
		else
		{
			size=sizeof(SONORK_CODEC_DESCRIPTOR);
			*e_ptr=SONORK_DWORD( SONORK_CODEC_E_Descriptor(data_type,0) );
			result = SONORK_RESULT_OK;
		}
	}
	return result;
}

// ReadMemNoSize:
//  Don't care about the size used up by CODEC_ReadMem
SONORK_RESULT
	TSonorkCodecAtom::CODEC_ReadMemNoSize(const BYTE*ptr,DWORD size)
{
	return CODEC_ReadMem(ptr,size);
}

SONORK_RESULT
	TSonorkCodecAtom::CODEC_ReadMem(const BYTE*ptr,DWORD& size)
{
	SONORK_RESULT	result;
	CODEC_Clear();
	if( size < sizeof(SONORK_CODEC_DESCRIPTOR) )
	{
		result = SONORK_RESULT_CODEC_ERROR; // SONORK_RESULT_BUFFER_TOO_SMALL;
	}
	else
	{
		SONORK_CODEC_DESCRIPTOR		descriptor;
		DWORD					data_size;
		SONORK_ATOM_TYPE			data_type;
		descriptor=SONORK_DWORD( *(SONORK_CODEC_DESCRIPTOR*)ptr );
		SONORK_CODEC_D_Descriptor(descriptor,data_type,data_size);
		if(data_size)
		{
			ptr+=sizeof(descriptor);
			if( data_size > size-sizeof(SONORK_CODEC_DESCRIPTOR) )
				result = SONORK_RESULT_CODEC_ERROR; // SONORK_RESULT_BUFFER_TOO_LARGE;
			else
			{
				TSonorkCodecReader CODEC(ptr,data_size, data_type);
				CODEC_ReadDataMem(CODEC);
				if( (result=CODEC.Result()) == SONORK_RESULT_OK)
				{
					size=data_size+sizeof(SONORK_CODEC_DESCRIPTOR);
				}
				else
				{
					CODEC_Clear();
				}
			}
		}
		else
		{
			size=sizeof(SONORK_CODEC_DESCRIPTOR);
			result = SONORK_RESULT_OK;
		}
	}
	return result;
}

// ----------------------------------------------------------------------------
// TSonorkCodecRawAtom
TSonorkCodecDW::TSonorkCodecDW(void *p_data, DWORD p_data_size,SONORK_ATOM_TYPE p_data_type)
{
	data=p_data;
	data_size=(data?p_data_size/sizeof(DWORD):0)*sizeof(DWORD);
	data_type=p_data_type;
}
void TSonorkCodecDW::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC) const
{
	CODEC.WriteDWN((DWORD*)data,data_size/sizeof(DWORD));
}
void TSonorkCodecDW::CODEC_ReadDataMem	(TSonorkCodecReader&CODEC)
{
	CODEC.ReadDWN((DWORD*)data,data_size/sizeof(DWORD));
}
// ----------------------------------------------------------------------------
// TSonorkCodecRawAtom

TSonorkCodecRawAtom::TSonorkCodecRawAtom(void *p_data, DWORD p_data_size,SONORK_ATOM_TYPE p_data_type)
{
	data=p_data;
	data_size=data?p_data_size:0;
	data_type=p_data_type;
}

TSonorkCodecRawAtom::TSonorkCodecRawAtom(SONORK_CODEC_DESCRIPTOR D,void*p_data)
{
	data=p_data;
	SONORK_CODEC_D_Descriptor(D,data_type,data_size);
}

void
 TSonorkCodecRawAtom::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC) const
{
	CODEC.WriteRaw(data,data_size);
}
void
 TSonorkCodecRawAtom::CODEC_ReadDataMem(TSonorkCodecReader&CODEC)
{
	CODEC.ReadRaw(data,data_size);
}




/*
SONORK_CODEC_DESCRIPTOR	CODEC_ReadDescriptor(TSonorkCodecReader&CODEC)
{
	SONORK_CODEC_DESCRIPTOR	descriptor;
	CODEC.ReadDW(&descriptor);
	return descriptor;
}
*/


SONORK_CODEC_DESCRIPTOR SONORK_CODEC_E_Descriptor(SONORK_ATOM_TYPE type,DWORD sz)
{
	SONORK_CODEC_DESCRIPTOR descriptor;
	descriptor=type;
	descriptor<<=SONORK_CODEC_DESCRIPTOR_TYPE_SHIFT;
	descriptor|=(sz&SONORK_CODEC_DESCRIPTOR_SIZE_MASK);
	return descriptor;
}
void  SONORK_CODEC_D_Descriptor(SONORK_CODEC_DESCRIPTOR descriptor,SONORK_ATOM_TYPE&type,DWORD& sz)
{
	sz=(UINT)(descriptor&SONORK_CODEC_DESCRIPTOR_SIZE_MASK);
	type=(SONORK_ATOM_TYPE)(descriptor>>SONORK_CODEC_DESCRIPTOR_TYPE_SHIFT);
}





#if SONORK_CODEC_LEVEL>5
SONORK_RESULT
	TSonorkCodecAtom::CODEC_Read(const TSonorkDynData*dyn_data)
{
	CODEC_Clear();
	if(dyn_data->DataSize() < sizeof(SONORK_CODEC_DESCRIPTOR)
		|| !dyn_data->Buffer())
	{
		return SONORK_RESULT_CODEC_ERROR; // SONORK_RESULT_BUFFER_TOO_SMALL;
	}
	else
	{
		SONORK_CODEC_DESCRIPTOR		descriptor;
		SONORK_ATOM_TYPE		data_type;
		DWORD				data_size;
		const BYTE		  *	ptr;
		ptr=dyn_data->Buffer();
		descriptor=*(SONORK_CODEC_DESCRIPTOR*)ptr;
		SONORK_CODEC_D_Descriptor(descriptor,data_type,data_size);
		if( data_size+sizeof(SONORK_CODEC_DESCRIPTOR) > dyn_data->DataSize() )
			return SONORK_RESULT_CODEC_ERROR; // SONORK_RESULT_BUFFER_TOO_SMALL;
		if(data_size)
		{
			ptr+=sizeof(descriptor);
			{
				TSonorkCodecReader CODEC(ptr,data_size,data_type);

				CODEC_ReadDataMem(CODEC);
				if(CODEC.Result() != SONORK_RESULT_OK)
				{
					CODEC_Clear();
					return CODEC.Result();
				}
			}
		}
		return SONORK_RESULT_OK;
	}
}

SONORK_RESULT	TSonorkCodecAtom::CODEC_Write(TSonorkDynData*dyn_data) const
{
	BYTE				*ptr;
	SONORK_CODEC_DESCRIPTOR	*e_ptr;
	SONORK_ATOM_TYPE		data_type;
	UINT 				data_size;

	data_type=CODEC_DataType();
	data_size=CODEC_DataSize();

	dyn_data->SetDataSize( data_size+sizeof(SONORK_CODEC_DESCRIPTOR) , true );

	ptr=dyn_data->wBuffer();
	e_ptr=(SONORK_CODEC_DESCRIPTOR*)ptr;
	ptr+=sizeof(SONORK_CODEC_DESCRIPTOR);

	dyn_data->SetDataType(data_type);

	if(data_size > 0)
	{
		TSonorkCodecWriter 	CODEC(ptr,data_size,data_type);
		CODEC_WriteDataMem(CODEC);

		if(CODEC.Result() != SONORK_RESULT_OK)
			return CODEC.Result();

		*e_ptr=SONORK_DWORD( SONORK_CODEC_E_Descriptor(data_type,CODEC.Size()) );
		dyn_data->SetDataSize( CODEC.Size()+sizeof(SONORK_CODEC_DESCRIPTOR) , false );
	}
	else
		*e_ptr=SONORK_DWORD( SONORK_CODEC_E_Descriptor(data_type,0) );

	return SONORK_RESULT_OK;
}

#endif

