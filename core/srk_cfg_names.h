#if !defined(SONORK_CFG_NAMES_H)
#define SONORK_CFG_NAMES_H

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
enum SONORK_SERVER_PROFILE_FLAGS
{
  SONORK_SERVER_PROFILE_FLAG_HAS_MAIN	=0x0000001
, SONORK_SERVER_PROFILE_FLAG_HAS_DATA	=0x0000002
, SONORK_SERVER_PROFILE_FLAG_IS_REMOTE 	=0x1000000
};
#define SONORK_PROFILE_NAME_MAX_SIZE	48
extern const char * szSrkAllRegKeyRoot;
extern const char * szSrkServerRegKeyRoot;
extern const char * szSrkClientRegKeyRoot;
extern const char * szSrkMainServerSection;
extern const char * szSrkDataServerSection;
extern const char * szSrkMainServerImage;
extern const char * szSrkDataServerImage;
extern const char * szSrkServerConfigImage;
extern const char * szSrkServiceDllImage;
extern const char * szSrkClientImage;
extern const char * szSrkClientIniFile;
extern const char * szSrkUninstallerImage;
#if defined(SONORK_SETUP_BUILD)
extern const char * szSrk_OLD_ServerIniFile;
#endif
#endif
