# Microsoft Developer Studio Project File - Name="ttpfile" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ttpfile - Win32 Release
!MESSAGE NMAKE /f "ttpfile.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "ttpfile.mak" CFG="ttpfile - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ttpfile - Win32 Release" ("Win32 (x86) Dynamic-Link Library")
!MESSAGE "ttpfile - Win32 Debug" ("Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ttpfile - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MT /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\source\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\source\common" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "ttpfile - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\source\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\source\common" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib /nologo /subsystem:windows /dll /debug /machine:I386

!ENDIF 

# Begin Target

# Name "ttpfile - Win32 Release"
# Name "ttpfile - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\..\source\ttfile\bplus.c
# End Source File
# Begin Source File

SOURCE=..\..\source\common\dlglib.c
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\ftlib.c
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\kermit.c
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\quickvan.c
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\ttfile.c
# End Source File
# Begin Source File

SOURCE=..\..\source\common\ttlib.c
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\xmodem.c
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\zmodem.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\..\source\ttfile\bplus.h
# End Source File
# Begin Source File

SOURCE=..\..\source\common\dlglib.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\ftlib.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\kermit.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\quickvan.h
# End Source File
# Begin Source File

SOURCE=..\..\source\common\ttlib.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\xmodem.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttfile\zmodem.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\Source\Ttfile\Ttpfile.rc
# ADD BASE RSC /l 0x411 /i "\DEV\TERATERM\Source\Ttfile"
# SUBTRACT BASE RSC /i "..\..\source\common"
# ADD RSC /l 0x411 /i "\DEV\TERATERM\Source\Ttfile" /i "C:\DEV\TERATERM\source\ttfile"
# SUBTRACT RSC /i "..\..\source\common"
# End Source File
# End Group
# Begin Group "Lib Files"

# PROP Default_Filter "lib"
# Begin Source File

SOURCE=..\bin\Release\ttpcmn.lib
# End Source File
# End Group
# Begin Group "Def File"

# PROP Default_Filter "def"
# Begin Source File

SOURCE=.\ttpfile.def
# End Source File
# End Group
# End Target
# End Project
