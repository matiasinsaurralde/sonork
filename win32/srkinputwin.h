#if !defined(SRKINPUTWIN_H)
#define SRKINPUTWIN_H

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL).

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	SSCL for more details.

	You should have received a copy of the SSCL	along with this program;
	if not, write to sscl@sonork.com.

	You may NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL).
*/

#if !defined(SRKWIN_H)
# include "srkwin.h"
#endif


#define SONORK_INPUT_F_LONG_TEXT	0x0001
#define SONORK_INPUT_F_PASSWORD		0x0002
class TSonorkInputWin
:public TSonorkWin
{
private:

	bool	OnCreate();

	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	bool	OnDrawItem(DRAWITEMSTRUCT*);

public:
	TSonorkInputWin(TSonorkWin*parent);

	TSonorkShortString	help;
	TSonorkShortString	prompt;
	TSonorkShortString	input;
	UINT			flags;
	SKIN_SIGN		sign;

	// Use UpdateForm if strings are changed after
	// the dialog was created.
	void	UpdateForm();
};

#endif
