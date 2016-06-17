
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


const char * szSrkAllRegKeyRoot		= "SOFTWARE\\SONORK";
const char * szSrkServerRegKeyRoot	= "SOFTWARE\\SONORK\\Server\\CurrentVersion";
const char * szSrkClientRegKeyRoot	= "SOFTWARE\\SONORK\\Client\\CurrentVersion";
const char * szSrkMainServerSection	= "Sonork Main Server";
const char * szSrkDataServerSection	= "Sonork Data Server";
const char * szSrkMainServerImage	= "srksvr.exe";
const char * szSrkDataServerImage	= "srkdat.exe";
const char * szSrkClientImage		= "sonork.exe";
const char * szSrkServerConfigImage	= "srkcfg.exe";
const char * szSrkServiceDllImage	= "srksvc.dll";
const char * szSrkClientIniFile		= "sonork.ini";
const char * szSrkUninstallerImage	= "srkremov.exe";
#if defined(SONORK_SETUP_BUILD)
const char * szSrk_OLD_ServerIniFile	= "srksvr.ini";
#endif

