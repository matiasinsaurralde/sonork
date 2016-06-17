# Microsoft Developer Studio Project File - Name="Sonork" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Sonork - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Sonork.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Sonork.mak" CFG="Sonork - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Sonork - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Sonork - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Sonork - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\temp\Release"
# PROP Intermediate_Dir "..\temp\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /O2 /I "..\core" /I "..\win32" /I "..\ipc" /I "..\zip" /I "..\skrtpl" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "SONORK_APP_BUILD" /D "SONORK_CLIENT_BUILD" /D SONORK_CODEC_LEVEL=10 /D "STRICT" /YX"srk_defs.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wsock32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "Sonork - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\temp\Debug"
# PROP Intermediate_Dir "..\temp\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /ZI /Od /I "..\core" /I "..\win32" /I "..\ipc" /I "..\zip" /I "..\srktpl" /D "_DEBUG" /D "SONORK_APP_BU" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "SONORK_APP_BUILD" /D "SONORK_CLIENT_BUILD" /D SONORK_CODEC_LEVEL=10 /D "STRICT" /YX"srk_defs.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wsock32.lib comctl32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Sonork - Win32 Release"
# Name "Sonork - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\core\Sonork Source Code License.txt"
# End Source File
# Begin Source File

SOURCE=..\win32\sonork.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\sonork.rc
# End Source File
# Begin Source File

SOURCE=..\core\srk_app_base.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_atom_db.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_bf_file.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_ccache.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_cfg_names.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_client.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_codec_atom.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_codec_file.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_codec_io.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_crypt_context.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_data_lists.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_data_packet.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_data_types.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_defs.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_dragdrop.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_email_codec.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_event_handler.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_ext_user_data.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_file_io.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_language.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_netio.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_pairvalue.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_referrer_id.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_services.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_stream_lr.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_string_loader.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_svr_login_packet.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_sys_string.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_task_atom.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_tcpio.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_udpio.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_url_codec.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_uts.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_uts_login_packet.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_uts_packet.cpp
# End Source File
# Begin Source File

SOURCE=..\core\srk_winregkey.cpp
# End Source File
# Begin Source File

SOURCE=..\zip\srk_zip.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkaboutwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkappstr.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkauthreqwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkbicho.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkbitmap.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkchatwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkclipwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkconsole.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkdbmaintwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkdialogwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkeappcfgwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkerrorwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkfiletxeng.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkfiletxgui.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkgrpmsgwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkhistorywin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkinputwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srklistview.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkmaininfowin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkmainwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkmsgfilter.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkmsgwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkmyinfowin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srknetcfgwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkprofileswin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkremindwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srksetupwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srksidmodewin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkskin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkslidewin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srksmailwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srksnapshotwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srksysconwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srktaskwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srktextwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srktooltipwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srktrackerwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srktreeview.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkultraminwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkusearchwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkuserdatawin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwaitwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwappwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app_extapp.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app_ipc.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app_mfc.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app_service.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app_ui.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app_uts.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\srkwinctrl.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\srkcryptlibvccst.lib
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\core\srk_app_base.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_atom_db.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_bf_file.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_ccache.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_cfg_names.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_client.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_client_defs.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_codec_atom.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_codec_file.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_codec_io.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_crypt.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_crypt_context.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_data_lists.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_data_packet.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_data_types.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_defs.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_dragdrop.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_email_codec.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_event_handler.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_ext_user_data.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_file_io.h
# End Source File
# Begin Source File

SOURCE=..\ipc\srk_ipc_defs.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_language.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_netio.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_pairvalue.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_services.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_stream_lr.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_string_loader.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_svr_login_packet.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_sys_string.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_task_atom.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_tcpio.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_udpio.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_url_codec.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_uts.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_uts_packet.h
# End Source File
# Begin Source File

SOURCE=..\core\srk_winregkey.h
# End Source File
# Begin Source File

SOURCE=..\zip\srk_zip.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkaboutwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkappstr.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkauthreqwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkbicho.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkbicho_defs.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkbitmap.h
# End Source File
# Begin Source File

SOURCE=..\srktpl\srkbrowser_defs.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkchatwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkclipwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkconsole.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkdbmaintwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkdialogwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkeappcfgwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkerrorwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkfiletxeng.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkfiletxgui.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkgrpmsgwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkhistorywin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkinputwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srklistview.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkmaininfowin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkmainwin.h
# End Source File
# Begin Source File

SOURCE=..\srktpl\srkmfcapi.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkmsgfilter.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkmsgwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkmyinfowin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srknetcfgwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkprofileswin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkremindwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srksetupwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srksidmodewin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkskin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkslidewin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srksmailwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srksnapshotwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srksysconwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srktaskwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srktextwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srktooltipwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srktrackerwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srktreeview.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkultraminwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkusearchwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkuserdatawin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkwaitwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkwappwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app_defs.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app_extapp.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin32app_mfc.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkwin_defs.h
# End Source File
# Begin Source File

SOURCE=..\win32\srkwinctrl.h
# End Source File
# Begin Source File

SOURCE=..\zip\unzip.h
# End Source File
# Begin Source File

SOURCE=..\zip\zconf.h
# End Source File
# Begin Source File

SOURCE=..\zip\zip.h
# End Source File
# Begin Source File

SOURCE=..\zip\zlib.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\win32\harrow.cur
# End Source File
# Begin Source File

SOURCE=..\win32\main_ex_f.ICO
# End Source File
# Begin Source File

SOURCE=..\win32\main_ex_m.ICO
# End Source File
# Begin Source File

SOURCE=..\win32\main_in_f.ICO
# End Source File
# Begin Source File

SOURCE=..\win32\main_in_m.ICO
# End Source File
# Begin Source File

SOURCE=..\win32\sonork.ico
# End Source File
# Begin Source File

SOURCE=..\win32\tray0.ICO
# End Source File
# Begin Source File

SOURCE=..\win32\tray0_i.ICO
# End Source File
# Begin Source File

SOURCE=..\win32\tray1.ICO
# End Source File
# Begin Source File

SOURCE=..\win32\tray1_i.ICO
# End Source File
# Begin Source File

SOURCE=..\win32\tray2.ICO
# End Source File
# Begin Source File

SOURCE=..\win32\tray2_i.ICO
# End Source File
# End Group
# End Target
# End Project
