#if !defined(SRKDBMAINTWIN_H)
#define SRKDBMAINTWIN_H


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

class TSonorkDbMaintWin
:public TSonorkWin
{

private:

	TSonorkTempBuffer   app_msg_path;
	TSonorkTempBuffer	app_ext_path;
	TSonorkTempBuffer	tmp_msg_path;
	TSonorkTempBuffer	tmp_ext_path;
	char			*p_app_msg_path,*p_app_ext_path;
	char			*p_tmp_msg_path,*p_tmp_ext_path;
	DWORD			user_no;
	TSonorkListIterator	I;
	TSonorkMsg		msg;
	FILE			*arch_file;


	TSonorkAtomDb*	app_msg_db;
	TSonorkAtomDb*	app_ext_db;

	HWND	status_hwnd;
	HWND	pb1_hwnd;
	void	OnAfterCreate();
	void	OnDestroy();
	BOOL	OnQueryClose();
	void	LoadLabels();
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	void	DoMaintenance();
	bool 	OnAppEvent(UINT event, UINT param, void*data);
	void	OnTimer(UINT id);
	void	UpdateInterface();
	bool	OpenArchiveFile();
	void	ArchiveMsg();
	void	CmdBrowse();
public:
	TSonorkDbMaintWin(TSonorkWin*parent, TSonorkAtomDb*msg_db, TSonorkAtomDb*ext_db);
};


#endif
