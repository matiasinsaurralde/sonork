#if !defined(SRKPROFILESWIN_H)
#define SRKPROFILESWIN_H

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

# if !defined(SRKWIN_H)
#  include "srkwin.h"
# endif

class TSonorkProfilesWin
:public TSonorkWin
{
private:
	HWND				profile_list;
	TSonorkShortStringQueue         profile_queue;
	bool				add_profile_selected;
	bool	OnCreate();
	void	LoadLabels();
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	bool	OnDrawItem(DRAWITEMSTRUCT*S);
	bool	GetSelection(TSonorkExtUserData& UD
				, TSonorkProfileCtrlData&
				, TSonorkShortString*password
				, int*index);
	bool	OpenProfile();
	void	LoadProfileList(bool load_queue, bool load_listbox);
	bool	DelProfile();
public:
	TSonorkProfilesWin(TSonorkWin*parent);

	// DoExecute will not create the window
	// if there are no profiles, it will just
	// return <false>.
	// if the user selected "Create User"
	// it will also return false.
	// If the return value is <false> the calling
	// window should close and show instead the
	// Create/Recover window
	bool	DoExecute();

	// Make private to DoExecute() is called instead
private:
	int	Execute()
	{	return TSonorkWin::Execute(); }
};

#endif