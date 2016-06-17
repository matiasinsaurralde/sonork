#if !defined(SRKEAPPCFGWIN_H)
#define SRKEAPPCFGWIN_H

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

class TSonorkEappCfgWin
:public TSonorkWin
{
private:

	struct LIST
	{
		HWND				hwnd;
	}list;
	TSonorkShortString	ini_file_str;
	char			*ini_file;

	bool	OnCommand(UINT id,HWND hwnd, UINT code);
	void		CmdSave();
	void		CmdInstall();
	LRESULT	OnNotify(WPARAM,TSonorkWinNotify*);
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	void	LoadList();
	void		AddSetItem(SONORK_C_CSTR name , BOOL enabled, int iItem);
	void	LoadLabels();


protected:
	bool OnCreate();
	void OnBeforeDestroy();

public:
	TSonorkEappCfgWin();

};

class TSonorkServicesWin
:public TSonorkWin
{
private:

	struct LIST
	{
		HWND				hwnd;
	}list;

	bool	OnCommand(UINT id,HWND hwnd, UINT code);
	void	LoadList();
	void	LoadLabels();

protected:
	bool OnCreate();
	void OnBeforeDestroy();

public:
	TSonorkServicesWin();

};

#endif
