#if !defined(SRKNETCFGWIN_H)
#define SRKNETCFGWIN_H

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

class TSonorkNetCfgWin
:public TSonorkWin
{
private:
	HWND			profile_list;
	bool	OnCreate();
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	void	CmdSave();
	void	CmdProfileAdd();
	void	CmdProfileDel();
	void	LoadLabels();
	void	LoadServerProfile();

	TSonorkClientServerProfile	SP;
public:
	TSonorkNetCfgWin(TSonorkWin*);
};
#endif