#include "srkwin32app.h"
#include "sonork.rh"
#pragma hdrstop
#include "srkerrorwin.h"

TSonorkErrorWin::TSonorkErrorWin(TSonorkWin*parent, SONORK_C_CSTR pText, TSonorkError*ERR, SKIN_SIGN psign)
	:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_ERROR
	, 0)
{
	sign=psign;
	pERR=ERR;
	text.Set(pText);
}


bool
 TSonorkErrorWin::OnCommand(UINT id,HWND , UINT )
{
	if(id==IDOK)
	{
		EndDialog(IDOK);
		return true;
	}
	return false;
}

bool
 TSonorkErrorWin::OnCreate()
{
	if(pERR)SonorkApp.LangTranslate(*pERR);
	LoadDefLangEntries();
	UpdateForm();
	::SetWindowPos(GetDlgItem(IDC_ERROR_SIGN)
		,NULL,0,0,SKIN_SIGN_SW,SKIN_SIGN_SH,SWP_NOMOVE|SWP_NOACTIVATE);
	::SendMessage(GetDlgItem(IDC_ERROR_CODE)
		,WM_SETFONT
		,(WPARAM)sonork_skin.Font(SKIN_FONT_SMALL)
		,0);
	::SendMessage(GetDlgItem(IDC_ERROR_TEXT)
		,WM_SETFONT
		,(WPARAM)sonork_skin.Font(SKIN_FONT_BOLD)
		,0);
	return true;
}
void
 TSonorkErrorWin::UpdateForm()
{
	char tmp[80];
	char *str;
	int l;
	for(;;)
	{
		if( pERR != NULL )
		if( pERR->Result() != SONORK_RESULT_OK )
		{

			sprintf(tmp,"%s (%u:%x:%u)"
				,pERR->ResultName()
				,pERR->Result()
				,pERR->Code()>>16
				,pERR->Code()&0xffff
				);
			SONORK_StrToLower(tmp);
			SetCtrlText(IDC_ERROR_CODE, tmp);
			l=pERR->Text().Length();
			str=new char[text.Length() + l + 16];
			if(l)
				sprintf(str,"%s\r\n%s",text.CStr(),pERR->Text().CStr());
			else
				sprintf(str,"%s",text.CStr());
			SetCtrlText(IDC_ERROR_TEXT,str);
			delete[] str;
			break;
		}
		SetCtrlText(IDC_ERROR_TEXT,text.CStr());
		ClrCtrlText(IDC_ERROR_CODE);
		break;
	}

}
#define MARGIN	4
bool
 TSonorkErrorWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if( S->CtlID == IDC_ERROR_SIGN )
	{
		sonork_skin.DrawSign(S->hDC,sign,S->rcItem.left,S->rcItem.top);
		return true;
	}
	return false;
}

	/*if( S->CtlID == IDC_ERROR_TEXT )
	{
		if(o_text_ptr != NULL)
		{
			RECT rect;
			HFONT pFont;
			pFont = (HFONT)::SelectObject(S->hDC, SonorkApp.MainFont() );
			if( recalc_text_rect )
			{
				int h;
				HWND hwnd;
				hwnd = GetDlgItem(IDC_ERROR_TEXT);
				::GetClientRect( hwnd,&o_text_rect );
				rect=o_text_rect;
				rect.right -= MARGIN*2;
				h=::DrawTextEx(S->hDC,o_text_ptr,o_text_len
					, &rect
					, DT_CALCRECT|DT_NOPREFIX|DT_TABSTOP|DT_WORDBREAK
					, NULL);
				rect.top = (o_text_rect.bottom - h)/2;
				rect.bottom=rect.top+h;
				rect.left  +=MARGIN;
				rect.right +=MARGIN;

				o_text_rect=rect;
				recalc_text_rect=false;
			}
			else
				rect=o_text_rect;
			::DrawTextEx(S->hDC,o_text_ptr,o_text_len-1
				, &rect
				, 0
				| DT_CENTER|DT_NOPREFIX|DT_TABSTOP|DT_WORDBREAK
				, NULL);
			::SelectObject(S->hDC, pFont);
		}
		return true;
	}
	else
	*/

