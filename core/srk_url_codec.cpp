#include "srk_defs.h"
#pragma hdrstop
#include "srk_url_codec.h"
#include "srk_string_loader.h"

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


#define SONORK_URLCODEC_BASE	10
#define SONORK_URLCODEC_LIMIT	256
extern SONORK_C_CSTR gu_urlcodec_table[];
#define ENCODE_STR(n)	(gu_urlcodec_table[n]+1)
#define DECODE_CHR(n)	(*gu_urlcodec_table[n])



TSonorkUrlNamedValue::TSonorkUrlNamedValue(SONORK_C_CSTR pName, SONORK_C_CSTR pValue)
{
	name.Set(	pName	);
	value.Set(	pValue	);
	flags=0;
}

TSonorkUrlNamedValue::TSonorkUrlNamedValue(SONORK_C_CSTR pName,UINT pNameLen, SONORK_C_CSTR pValue, UINT pValueLen)
{
	TSonorkTempBuffer buffer(SONORK_UrlMaxDecodeSize(pValueLen));
	name.Set(	SONORK_UrlDecode(&buffer, pName	, pNameLen)	);
	value.Set(	SONORK_UrlDecode(&buffer, pValue, pValueLen)	);
	flags=0;

}

bool	TSonorkUrlParams::LoadQueryString(SONORK_C_CSTR str)
{
	SONORK_C_CSTR	n_ptr;
	SONORK_C_CSTR	v_ptr;
	SONORK_C_CSTR	e_ptr;
	UINT		n_len;
	UINT		v_len;
	TSonorkUrlNamedValue *NV;

	Clear();

	qs.Set( str );
	n_ptr=str;
	while( n_ptr != NULL )
	{
		v_ptr=strchr(n_ptr,'=');
		if(!v_ptr)break;

		n_len = v_ptr - n_ptr;
		if(!n_len)break;
		v_ptr++;

		e_ptr=strchr(v_ptr,'&');
		if(e_ptr)
		{
			v_len = e_ptr - v_ptr;
			e_ptr++;
		}
		else
			v_len = strlen(v_ptr);

		NV=new TSonorkUrlNamedValue(n_ptr,n_len,v_ptr,v_len);
		queue.Add(NV);
		n_ptr = e_ptr;
	}
 	return true;
}
void
	TSonorkUrlParams::Add(SONORK_C_CSTR var_name, SONORK_C_CSTR var_value)
{
	TSonorkUrlNamedValue *NV;
	NV=new TSonorkUrlNamedValue(var_name,var_value);
	queue.Add(NV);
}
void
	TSonorkUrlParams::Set(const TSonorkUrlParams& O)
{
	TSonorkListIterator LI;
	TSonorkUrlNamedValue *NV;
	Clear();
	this->qs.Set(O.qs);
	O.queue.BeginEnum(LI);
	while((NV=O.queue.EnumNext(LI))!=NULL)
	{
		this->queue.Add( new TSonorkUrlNamedValue(NV->Name(),NV->Value() ) );
	}
	O.queue.EndEnum(LI);
}
void
	TSonorkUrlParams::Transfer(TSonorkUrlParams& O )
{
	TSonorkUrlNamedValue *NV;
	Clear();
	this->qs.Set(O.qs);
	while((NV=O.queue.RemoveFirst())!=NULL)
		queue.Add(NV);
	O.Clear();
}

int
	TSonorkUrlParams::GetValueNum(SONORK_C_CSTR name, int defValue) const
{
	TSonorkUrlNamedValue *NV;
	SONORK_C_CSTR	szValue;
	if((NV=GetValueNV(name))!=NULL)
	{
		szValue=NV->Value();
		if(*szValue)
			return atoi(szValue);
	}
	return defValue;
}

SONORK_C_CSTR
	TSonorkUrlParams::GetValueStr(SONORK_C_CSTR name) const
{
	TSonorkUrlNamedValue *NV;
	if((NV=GetValueNV(name))!=NULL)
		return NV->Value();
	return NULL;
}
TSonorkUrlNamedValue *TSonorkUrlParams::GetValueNV(SONORK_C_CSTR name) const
{
	TSonorkUrlNamedValue *NV;
	TSonorkListIterator LI;
	queue.BeginEnum(LI);
	while((NV=queue.EnumNext(LI))!=NULL)
	{
		if( !stricmp(name,NV->Name()) )
			break;
	}
	queue.EndEnum(LI);
	return NV;
}
TSonorkUrlParams::~TSonorkUrlParams()
{
	Clear();
}

void	TSonorkUrlParams::Clear()
{
	TSonorkUrlNamedValue *NV;
	while((NV=queue.RemoveFirst())!=NULL)
		delete NV;
	qs.Clear();
}

SONORK_C_CSTR SONORK_UrlParamEncode(TSonorkTempBuffer*pTgt, SONORK_C_CSTR name,const SONORK_DWORD2&value)
{
	char tmp[40];
	value.GetStr(tmp);
	return SONORK_UrlParamEncode(pTgt,name,tmp);
}

SONORK_C_CSTR SONORK_UrlParamEncode(TSonorkTempBuffer*pTgt, SONORK_C_CSTR name,SONORK_C_CSTR value)
{
	TSonorkTempBuffer tBuff( SONORK_UrlMaxEncodeSize(strlen(value)) );
	char	*tgt;
	pTgt->SetMinSize(tBuff.GetSize() + strlen(name) + 2);
	tgt = pTgt->CStr();
	wsprintf(tgt,"%s=%s",name,SONORK_UrlEncode(&tBuff,value));
	return tgt;
}

SONORK_C_CSTR SONORK_UrlEncode(TSonorkShortString* pTgt, SONORK_C_CSTR pSrc)
{
	UINT bSz=SONORK_UrlMaxEncodeSize(strlen(pSrc));
	pTgt->SetBufferSize( bSz );
	return SONORK_UrlEncode(pTgt->Buffer() , bSz , pSrc);
}
SONORK_C_CSTR SONORK_UrlDecode(TSonorkShortString* pTgt, SONORK_C_CSTR pSrc, UINT pSrcLen)
{
	UINT bSz;
	if(pSrcLen==(UINT)-1)pSrcLen = strlen(pSrc);
	bSz =SONORK_UrlMaxDecodeSize(pSrcLen);
	pTgt->SetBufferSize( bSz );
	return SONORK_UrlDecode( pTgt->Buffer() , bSz, pSrc, pSrcLen );
}

SONORK_C_CSTR SONORK_UrlEncode(TSonorkTempBuffer*pTgt, SONORK_C_CSTR pSrc)
{
	pTgt->SetMinSize( SONORK_UrlMaxEncodeSize(strlen(pSrc)) );
	return SONORK_UrlEncode(pTgt->CStr() , pTgt->GetSize() , pSrc);
}

SONORK_C_CSTR SONORK_UrlDecode(TSonorkTempBuffer*pTgt, SONORK_C_CSTR pSrc, UINT pSrcLen)
{
	if(pSrcLen==(UINT)-1)pSrcLen = strlen(pSrc);
	pTgt->SetMinSize( SONORK_UrlMaxDecodeSize(pSrcLen) );
	return SONORK_UrlDecode( pTgt->CStr() , pTgt->GetSize(), pSrc, pSrcLen );
}

SONORK_C_CSTR SONORK_UrlEncode(SONORK_C_STR pTgt, UINT pTgtSize, SONORK_C_CSTR pSrc)
{
	int  c;
	SONORK_C_CSTR enc_str;
	TSonorkStringLoader	SL;

	SL.Open(pTgt , pTgtSize);
	while( *pSrc && !SL.Full() )
	{
		c=*pSrc++;
		if( 	(c>='A'&&c<='Z')
			||	(c>='a'&&c<='z')
			||	(c>='0'&&c<='9'))
		{
			SL.Add((char)c);
		}
		else
		if(c>=SONORK_URLCODEC_BASE && c<SONORK_URLCODEC_LIMIT)
		{
			c-=SONORK_URLCODEC_BASE;
			enc_str = ENCODE_STR(c);
			while(*enc_str)
				SL.Add(*enc_str++);
		}
	}
	SL.Close();
	return pTgt;
}

SONORK_C_CSTR SONORK_UrlDecode(SONORK_C_STR pTgt, UINT pTgtSize, SONORK_C_CSTR pSrc, UINT pSrcLen)
{
	char 			val_str[3];
	int  			c;
	TSonorkStringLoader	SL;

	val_str[2]=0;
	if(pSrcLen==(UINT)-1)
		pSrcLen = strlen(pSrc);

	SL.Open(pTgt,pTgtSize);
	while( *pSrc && pSrcLen )
	{
		c=*pSrc++;
		pSrcLen--;

		if(c=='%')
		{
			if(!(*pSrc!=0 && pSrcLen))break;
			val_str[0]=*pSrc++;
			pSrcLen--;
			if(!(*pSrc!=0 && pSrcLen))break;
			val_str[1]=*pSrc++;
			pSrcLen--;
			c = (int)strtol(val_str,NULL,16);
			if(c<SONORK_URLCODEC_BASE || c>=SONORK_URLCODEC_LIMIT)
				continue;
			SL.Add( DECODE_CHR(c-SONORK_URLCODEC_BASE) );
		}
		else
			if(c=='+')
				SL.Add(' ');
		else
			SL.Add( (char)c );
	}
	SL.Close();
	return pTgt;
}


static SONORK_C_CSTR
gu_urlcodec_table[SONORK_URLCODEC_LIMIT - SONORK_URLCODEC_BASE]=
{
{"\n%0a"},	//	10
{" %0b"},
{" %0c"},
{"\r%0d"},
{" %0e"},
{" %0f"},
{" %10"},
{" %11"},
{" %12"},
{" %13"},
{" %14"},	// 20
{" %15"},
{" %16"},
{" %17"},
{" %18"},
{" %19"},
{" %1a"},
{" %1b"},
{" %1c"},
{" %1d"},
{" %1e"},	// 30
{" %1f"},
{" +"},
{"!%21"},
{"\"%22"},
{"#%23"},
{"$%24"},
{"%%25"},
{"&%26"},
{"\'%27"},
{"(%28"},	// 40
{")%29"},
{"*%2a"},
{"+%2b"},
{",%2c"},
{"-%2d"},
{".%2e"},
{"/%2f"},
{"00"},
{"11"},
{"22"},		// 50
{"33"},
{"44"},
{"55"},
{"66"},
{"77"},
{"88"},
{"99"},
{":%3A"},
{";%3B"},
{"<%3C"},    // 60
{"=%3D"},
{">%3E"},
{"?%3F"},
{"@%40"},
{"AA"},
{"BB"},
{"CC"},
{"DD"},
{"EE"},
{"FF"},		// 70
{"GG"},
{"HH"},
{"II"},
{"JJ"},
{"KK"},
{"LL"},
{"MM"},
{"NN"},
{"OO"},
{"PP"},		// 80
{"QQ"},
{"RR"},
{"SS"},
{"TT"},
{"UU"},
{"VV"},
{"WW"},
{"XX"},
{"YY"},
{"ZZ"},      // 90
{"[%5B"},
{"\\%5C"},
{"]%5D"},
{"^%5E"},
{"_%5F"},
{"`%60"},
{"aa"},
{"bb"},
{"cc"},
{"dd"},		// 100
{"ee"},
{"ff"},
{"gg"},
{"hh"},
{"ii"},
{"jj"},
{"kk"},
{"ll"},
{"mm"},
{"nn"},		// 110
{"oo"},
{"pp"},
{"qq"},
{"rr"},
{"ss"},
{"tt"},
{"uu"},
{"vv"},
{"ww"},
{"xx"},      // 120
{"yy"},
{"zz"},
{"{%7B"},
{"|%7C"},
{"}%7D"},
{"~%7E"},
{"%7F"},
{"€%80"},
{"%81"},
{"‚%82"},	// 130
{"ƒ%83"},
{"„%84"},
{"…%85"},
{"†%86"},
{"‡%87"},
{"ˆ%88"},
{"‰%89"},
{"Š%8A"},
{"‹%8B"},
{"Œ%8C"},	// 140
{"%8D"},
{"Ž%8E"},
{"%8F"},
{"%90"},
{"‘%91"},
{"’%92"},
{"“%93"},
{"”%94"},
{"•%95"},
{"–%96"},	// 150
{"—%97"},
{"˜%98"},
{"™%99"},
{"š%9A"},
{"›%9B"},
{"œ%9C"},
{"%9D"},
{"ž%9E"},
{"Ÿ%9F"},
{" %A0"},
{"¡%A1"},
{"¢%A2"},
{"£%A3"},
{"¤%A4"},
{"¥%A5"},
{"¦%A6"},
{"§%A7"},
{"¨%A8"},
{"©%A9"},
{"ª%AA"},
{"«%AB"},
{"¬%AC"},
{"­%AD"},
{"®%AE"},
{"¯%AF"},
{"°%B0"},
{"±%B1"},
{"²%B2"},
{"³%B3"},
{"´%B4"},
{"µ%B5"},
{"¶%B6"},
{"·%B7"},
{"¸%B8"},
{"¹%B9"},
{"º%BA"},
{"»%BB"},
{"¼%BC"},
{"½%BD"},
{"¾%BE"},
{"¿%BF"},
{"À%C0"},
{"Á%C1"},
{"Â%C2"},
{"Ã%C3"},
{"Ä%C4"},
{"Å%C5"},
{"Æ%C6"},
{"Ç%C7"},
{"È%C8"},
{"É%C9"},
{"Ê%CA"},
{"Ë%CB"},
{"Ì%CC"},
{"Í%CD"},
{"Î%CE"},
{"Ï%CF"},
{"Ð%D0"},
{"Ñ%D1"},
{"Ò%D2"},
{"Ó%D3"},
{"Ô%D4"},
{"Õ%D5"},
{"Ö%D6"},
{"×%D7"},
{"Ø%D8"},
{"Ù%D9"},
{"Ú%DA"},
{"Û%DB"},
{"Ü%DC"},
{"Ý%DD"},
{"Þ%DE"},
{"ß%DF"},
{"à%e0"},
{"á%e1"},
{"â%e2"},
{"ã%e3"},
{"ä%e4"},
{"å%e5"},
{"æ%e6"},
{"ç%e7"},
{"è%e8"},
{"é%e9"},
{"ê%ea"},
{"ë%eB"},
{"ì%eC"},
{"í%eD"},
{"î%eE"},
{"ï%eF"},
{"ð%f0"},
{"ñ%f1"},
{"ò%f2"},
{"ó%f3"},
{"ô%f4"},
{"õ%f5"},
{"ö%f6"},
{"÷%f7"},
{"ø%f8"},
{"ù%f9"},
{"ú%fa"},
{"û%fb"},
{"ü%fc"},
{"ý%fd"},
{"þ%fe"},
{"ÿ%ff"}};

BOOL SONORK_IsUrl(const char *str)
{
	if(!strnicmp(str,"http://",7))
		return true;
	if(!strnicmp(str,"ftp://",6))
		return true;
	if(!strnicmp(str,"mailto:",7))
		return true;
	return false;
}
void	  SONORK_HtmlPuts(FILE* file ,SONORK_C_CSTR src)
{
	char c;
	while(*src)
	{
		c=*src++;
		if(!( 	(c>='a'&&c<='z')
		||		(c>='A'&&c<='Z')
		||		(c>='9'&&c<='0') ) )
		{
			switch(c)
			{
				case 'á':	c='a';	goto acute;
				case 'é':	c='e';	goto acute;
				case 'í':	c='i';	goto acute;
				case 'ó':	c='o';	goto acute;
				case 'ú':	c='u';
acute:
				fprintf(file,"&%cacute;",c);
				break;

				case 'ñ':	c='n'; goto tilde;
				case 'Ñ':	c='N'; ;
tilde:
				fprintf(file,"&%ctilde;",c);
				break;

				case '>':	fputs("&gt;",file);break;
				case '<':	fputs("&lt;",file);break;
				default:
					goto no_translation;

			}
			continue;
		}
no_translation:
		fputc( c , file );

	}

}

