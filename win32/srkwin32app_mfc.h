#if !defined(SRKWIN32APP_MFC_H)
#define SRKWIN32APP_MFC_H

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

#include "SrkMfcApi.h"



TSrkMfcWindowHandle*  	SrkMfc_CreateWindow(TSonorkWin*,SRK_MFC_WIN_TYPE);
void					SrkMfc_DestroyWindow(TSrkMfcWindowHandle*);


#endif