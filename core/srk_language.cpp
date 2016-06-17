#include "srk_defs.h"
#pragma hdrstop
#include "srk_language.h"
#include <stdio.h>
#define MAX_LINE_LENGTH	2046

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


TSonorkLangCodeTable::TSonorkLangCodeTable()
{
	table=NULL;
	items=0;
}
TSonorkLangCodeTable::~TSonorkLangCodeTable()
{
	Clear();
}
const	TSonorkLangCodeRecord	*TSonorkLangCodeTable::GetRecordByIndex(int index)
{
	if(IsLoaded())
	if(index>=0 && index<items)
		return table+index;
	return NULL;
}
const	TSonorkLangCodeRecord	*TSonorkLangCodeTable::GetRecordByCode(DWORD code, int *p_index)
{
	int i;
	TSonorkLangCodeRecord	*REC;
	if(IsLoaded())
	if(code)
		for(i=0,REC=table;i<items;i++,REC++)
			if( REC->code == code )
			{
				if(p_index!=NULL)*p_index = i;
				return REC;
			}
	return NULL;
}

void TSonorkLangCodeTable::Clear()
{
	if(table != NULL)
	{
		int i;
		TSonorkLangCodeRecord	*REC;
		for(i=0,REC=table;i<items;i++,REC++)
			SONORK_MEM_FREE(REC->name);
		SONORK_MEM_DELETE_ARRAY(table);
		table=NULL;
		items=0;
	}
}

SONORK_RESULT	TSonorkLangCodeTable::Load(TSonorkError& ERR, SONORK_C_CSTR file_name)
{
	FILE *F;

	Clear();

	F=fopen(file_name,"rt");
	if(!F)
	{
		ERR.SetSys(SONORK_RESULT_STORAGE_ERROR,GSS_FILEOPENERR,errno);
	}
	else
	{
		TSonorkTempBuffer line(MAX_LINE_LENGTH+2);
		int	scan_count;
		int len;
		TSonorkLangCodeRecord	*REC;

		scan_count=0;
		while( fgets( line.CStr() , MAX_LINE_LENGTH , F ) != NULL )
		{
			if(*line.CStr()!='.')continue;
			len=strlen(line.CStr());
			if( len<6 || *(line.CStr() + 3)!=':')continue;
			scan_count++;
		}
		if(scan_count > 1)
		{
			SONORK_MEM_NEW( table = new TSonorkLangCodeRecord[scan_count] );
			rewind(F);
			REC = table;
			while( fgets( line.CStr() , MAX_LINE_LENGTH , F ) != NULL
			 && items < scan_count)
			{
				if(*line.CStr()!='.')continue;
				len=strlen(line.CStr());
				if( len<6 || *(line.CStr() + 3)!=':')continue;
				scan_count++;
				*(line.CStr()+len-1)=0;	// Cut off ending new line char
				*(line.CStr()+1)= (char)tolower(*(line.CStr()+1));
				*(line.CStr()+2)= (char)tolower(*(line.CStr()+2));
				REC->code = *(WORD*)(line.CStr()+1);
				REC->name = SONORK_CStr( line.CStr() + 4 );
				items++;
				REC++;
			}
		}
		fclose(F);
	}
	return ERR.Result();
}

// ----------------------------------------------------------------------------
// TSonorkLanguageTable

void		TSonorkLanguageTable::Translate(TSonorkError& ERR)
{
	if(ERR.Result() != SONORK_RESULT_OK)
	{
		SONORK_C_CSTR str=ERR.Text().CStr();
		if(str != NULL )
		if(*(DWORD*)str == *(DWORD*)"!GU!")
		{
			SONORK_SYS_STRING sys_index;
			str+=4;
			if(*str)
			{
				sys_index = SONORK_SysLexemeToIndex( str );
				if( sys_index != GSS_NULL )
				{
					ERR.wText().Set( SysStr( sys_index) );
				}
			}
			else
				ERR.wText().Clear();
		}
	}


}
#define COMMAND_LINE_CHAR	':'
#define STRING_LINE_CHAR 	'.'
#define VALUE_PREFIX_CHAR	':'

enum SECTION
{
	SECTION_NONE
,	SECTION_APP
,	SECTION_SYS
};

SONORK_RESULT	TSonorkLanguageTable::Load(TSonorkError& 	ERR
						,SONORK_C_CSTR 				file_name
						,UINT					xlat_entries
						,SONORK_C_CSTR*				xlat_table)
{
	FILE *F;
	Clear();

	F=fopen(file_name,"rt");
	if(!F)
	{
		ERR.Set(SONORK_RESULT_STORAGE_ERROR,"File could not be open",errno,true);
	}
	else
	{

		app.xlat_table  = xlat_table;
		app.entries		= xlat_entries;

		if(_Load(F,ERR) == SONORK_RESULT_OK)
		{
			if(  sys.lang.code==0 )
			{
				ERR.Set(SONORK_RESULT_CONFIGURATION_ERROR
					,"Missing or invalid LANG_ID command"
					,0
					,true);
			}
		}
		fclose(F);
		if( ERR.Result() != SONORK_RESULT_OK )
			Clear();
	}
	return ERR.Result();
}

SONORK_RESULT
	TSonorkLanguageTable::_Load(void*fP,TSonorkError&ERR)
{
	FILE *F=(FILE*)fP;
	char 	 *value
			,cmd_char
			,*lexeme;
	SECTION	 section;
	UINT 	 index
			,cur_app_section_size
			,cur_sys_section_size
			,max_app_section_size
			,max_sys_section_size
			,app_section_offset
			,sys_section_offset
			,value_length
			,str_index;
	TSonorkTempBuffer line(MAX_LINE_LENGTH+2);

	assert( str_table == NULL );
	assert( app.str_index == NULL );
	assert( sys.str_index == NULL );

	max_app_section_size =
	max_sys_section_size = 0;

	// Lexeme is just after initial character because valid formats are:
	//	:lexeme :value
	//  .lexeme :value
	// There cannot be spaces between the initial char and <lexeme>
	// nor between the colon and <value>.
	// We load lexeme here because the position does not change.

	lexeme = line.CStr() + 1;

	section = SECTION_NONE;
	while( fgets( line.CStr() , MAX_LINE_LENGTH , F ) != NULL )
	{
		// All string lines should start with a dot. (.)
		// All commands should start with a colon. (:)
		cmd_char = _ParseLine(line.CStr() , &value, &value_length);

		if(cmd_char == 0)
			continue;

		if( cmd_char == COMMAND_LINE_CHAR )
		{
			section = (SECTION)_ParseCommand(section,lexeme,value);
			continue;
		}

		// String line: section must be defined
		if( section == SECTION_NONE )
			continue;

		if( section == SECTION_APP )
		{
			// we add '+1' to skip the starting dot
			// and get the index of the lexeme
			index=GetAppEntryIndex( lexeme );
			if( index == (UINT)-1 )
			{
				sonork_printf("LangLoad: '%s' not found",lexeme);
				continue;	// the lexeme does not exist in the xlat table
			}
			max_app_section_size += value_length+1;
		}
		else
		{
			// we add '+1' to skip the starting dot
			// and get the index of the lexeme
			index=SONORK_SysLexemeToIndex( lexeme );
			if( index == GSS_NULL )
				continue;	// the lexeme does not exist in the xlat table
			max_sys_section_size += value_length+1;
		}
	}

	if( max_sys_section_size == 0 || max_app_section_size == 0)
	{
		ERR.Set(SONORK_RESULT_CONFIGURATION_ERROR,"Invalid language file",0,true);
		return ERR.Result();
	}

	rewind( F );

	max_app_section_size+=8;
	max_sys_section_size+=8;

	str_table = SONORK_MEM_ALLOC( char , max_app_section_size + max_sys_section_size);

	SONORK_ZeroMem( str_table , max_app_section_size + max_sys_section_size);

	app.str_index	= SONORK_MEM_ALLOC( UINT , sizeof(UINT) * app.entries);
	SONORK_ZeroMem(app.str_index,sizeof(UINT) * app.entries);

	sys.str_index	= SONORK_MEM_ALLOC( UINT , sizeof(UINT) * SONORK_SYS_STRINGS );
	SONORK_ZeroMem(sys.str_index,sizeof(UINT)*SONORK_SYS_STRINGS);


	app_section_offset	 = 1;
	sys_section_offset   = max_app_section_size;

	cur_app_section_size = 0;
	cur_sys_section_size = 0;

	while( fgets( line.CStr() , MAX_LINE_LENGTH , F ) != NULL )
	{
		cmd_char = _ParseLine(line.CStr() , &value, &value_length);

		if(cmd_char == 0)
			continue;

		if( cmd_char == COMMAND_LINE_CHAR )
		{
			section = (SECTION)_ParseCommand(section,lexeme,value);
			continue;
		}

		if( section == SECTION_NONE )
			continue;

		value_length++;	// include terminating zero
		if( section == SECTION_APP )
		{
			index=GetAppEntryIndex( lexeme );
			if( index == (UINT)-1 )
				continue;

			if( cur_app_section_size + value_length > max_app_section_size)
				break;

			str_index = app_section_offset + cur_app_section_size;
			*(app.str_index + index) = str_index;
			SONORK_ParseCopyString(str_table + str_index , value);
			cur_app_section_size += value_length;
		}
		else
		{
			// we add '+1' to skip the starting dot
			// and get the index of the lexeme
			index=SONORK_SysLexemeToIndex( lexeme );
			if( index == GSS_NULL )
				continue;	// the lexeme does not exist in the xlat table

			if( cur_sys_section_size + value_length > max_sys_section_size)
				break;

			str_index = sys_section_offset + cur_sys_section_size;
			*(sys.str_index + index) = str_index;
			memcpy( str_table + str_index , value , value_length);
			cur_sys_section_size += value_length;
		}
	}

	// Check if all sys strings are loaded
	for( index = GSS_NULL + 1; index < SONORK_SYS_STRINGS; index ++ )
		if( *(sys.str_index+index) == 0 )
			break;
	if( index == SONORK_SYS_STRINGS )
	{
		// Yes: All sys strings loaded
		// Check if all app strings are loaded
		for( index = 0; index < app.entries; index ++ )
			if( *(app.str_index+index) == 0 )
				break;
		if( index == app.entries )
		{
			// Yes: All app strings loaded
			ERR.SetOk();
			return SONORK_RESULT_OK;
		}
		else
		{
			sprintf(line.CStr(),"Missing APP lexeme '%s'",*(app.xlat_table+index) );
		}
	}
	else
	{
		sprintf(line.CStr()
			,"Missing SYS lexeme '%s'"
			,SONORK_SysIndexToLexeme((SONORK_SYS_STRING)index) );
	}
	ERR.Set(SONORK_RESULT_CONFIGURATION_ERROR,line.CStr(),0,true);

	return ERR.Result();
}
UINT
	TSonorkLanguageTable::_ParseCommand(UINT section,SONORK_C_CSTR lexeme,SONORK_C_CSTR value)
{
	if(!SONORK_StrNoCaseCmp(lexeme,"APP_SECTION") )
	{
		if(!SONORK_StrNoCaseCmp(value,"BEGINS"))
		{
			section = SECTION_APP;
		}
		else
		if(!SONORK_StrNoCaseCmp(value,"ENDS"))
		{
			if(section == SECTION_APP)
				section = SECTION_NONE;
		}
	}
	else
	if(!SONORK_StrNoCaseCmp(lexeme,"SYS_SECTION") )
	{
		if(!SONORK_StrNoCaseCmp(value,"BEGINS"))
		{
			section = SECTION_SYS;
		}
		else
		if(!SONORK_StrNoCaseCmp(value,"ENDS"))
		{
			if(section == SECTION_SYS)
				section = SECTION_NONE;
		}
	}
	else
	if( !SONORK_StrNoCaseCmp(lexeme,"LANG_ID") )
	{
		sys.lang.lexeme[0]= (char)tolower(*(value+0));
		sys.lang.lexeme[1]= (char)tolower(*(value+1));
		sys.lang.lexeme[2]=sys.lang.lexeme[3]=0;
		sys.lang.code 	= *(WORD*)sys.lang.lexeme;
	}
	return section;
}
char
	TSonorkLanguageTable::_ParseLine(SONORK_C_STR pSrc,SONORK_C_STR*pValue,UINT*pValueLength)
{
	char 		cmd_char;
	SONORK_C_STR   	value,end_ptr;
	UINT		value_length;

	cmd_char = *pSrc++;
	// Look for the starting dot.
	if( cmd_char != STRING_LINE_CHAR )
	{
		// No dot, check if it starts with colon.
		if( cmd_char != COMMAND_LINE_CHAR )
			return 0;	// No colon,  ignore.
	}

	// Lookup value position, the value should be prefixed with ':'
	//  and there cannot be spaces between the colon and the value.
	// Note: <lexeme> is line+1 which is the character after <cmd_char>
	// (set before starting this loop)
	end_ptr = value = strchr( pSrc , VALUE_PREFIX_CHAR );
	if(value == NULL )
		return 0;	// No colon

	// Cut line at value start character
	//  all that remains before is the lexeme
	//  all that remains after is the value
	*value++ = 0;

	value_length=strlen( value );
	if( value_length<1 )
		return 0;

	// Cut value at the length - 1 if fgets()
	// appended a new line character
	if( *(value + value_length - 1) == '\n')
	{
		*(value + value_length - 1 )=0;
		if( --value_length <= 0)
			return 0;
	}

	// Now, <lexeme> may be separated from the :<value> with spaces,
	// so search backwards for either a non-space character or the
	// starting character <cmd_char>.
	// We are sure there IS a starting character because that's
	// what we check at the beginnig of this loop.
	end_ptr--;
	while( *end_ptr!=cmd_char && *end_ptr<=' ' )
		end_ptr--;

	if( *end_ptr == cmd_char )
	{
		// We've found the starting character!
		// there is no lexeme, ignore line
		return 0;
	}
	// We found a non-space character: skip it
	// and we're a the end of the lexeme
	end_ptr++;
	*end_ptr=0;
	*pValue 			= value;
	*pValueLength       = value_length;

	return cmd_char;

}


UINT	TSonorkLanguageTable::GetAppEntryIndex( SONORK_C_CSTR lexeme )
{
	UINT i;
	assert( app.xlat_table != NULL );
	for(i=0 ; i < app.entries; i++ )
	{
		if( *(DWORD*)*(app.xlat_table+i)==*(DWORD*)lexeme )
			if(!SONORK_StrNoCaseCmp(
				*(app.xlat_table+i)+sizeof(DWORD)
				, lexeme+sizeof(DWORD)))
				return i;
	}
	return (UINT)-1;
}

TSonorkLanguageTable::TSonorkLanguageTable()
{
	app.entries		= 0;
	sys.lang.code	= 0;

	str_table		= NULL;

	app.str_index	=
	sys.str_index	= NULL;


}

TSonorkLanguageTable::~TSonorkLanguageTable()
{
	Clear();
}
void TSonorkLanguageTable::Clear()
{
	if( str_table != NULL )
	{
		SONORK_MEM_FREE( str_table );
		str_table = NULL;
	}
	if( app.str_index != NULL )
	{
		SONORK_MEM_FREE( app.str_index );
		app.str_index = NULL;
	}
	if( sys.str_index != NULL )
	{
		SONORK_MEM_FREE( sys.str_index );
		sys.str_index = NULL;
	}
	app.xlat_table = NULL;
	app.entries =0;
	sys.lang.code=0;
}

void	  TSonorkLanguageTable::Transfer(TSonorkLanguageTable&O)
{
	Clear();	// this
	if( O.IsLoaded() )
	{
		str_table					= O.str_table;

		app.str_index				= O.app.str_index;
		app.xlat_table				= O.app.xlat_table;
		app.entries					= O.app.entries;

		sys.lang.code				= O.sys.lang.code;
		sys.str_index				= O.sys.str_index;
		*(DWORD*)sys.lang.lexeme	= *(DWORD*)O.sys.lang.lexeme;

		SONORK_ZeroMem(&O.app,sizeof(O.app));
		SONORK_ZeroMem(&O.sys,sizeof(O.sys));
		O.str_table = NULL;
	}
}

SONORK_C_CSTR TSonorkLanguageTable::AppStr(UINT index) const
{
	if( IsLoaded() )
	{
		if( index < app.entries)
			return str_table + *(app.str_index+index);
		return "";
	}
	return "AppStr:Not loaded!";
}
SONORK_C_CSTR TSonorkLanguageTable::SysStr(SONORK_SYS_STRING index)	const
{
	if( IsLoaded() )
		if( index < SONORK_SYS_STRINGS)
			return str_table + *(sys.str_index+index);
	return "SysStr:Not loaded!";
}
int SONORK_ParseCopyString(SONORK_C_STR target, SONORK_C_CSTR source)
{
	SONORK_C_STR	tp;
	SONORK_C_CSTR	sp;
	int  length;
	length = 0;
	tp=target;
	sp=source;
	while(*sp)
	{
		if(*sp=='\\')
		{
			char sc;
			sp++;
			switch(*sp)
			{
				case 'n':sc='\n';break;
				case 't':sc='\t';break;
				case 'r':sc='\r';break;
				default:
					sc=*sp;
					break;
			}
			if(sc)
			{
				*tp++=sc;
				length++;
				sp++;
			}
			continue;
		}
		else
		{
			*tp++=*sp++;
			length++;
		}
	}
	*tp=0;
	return length;

}


