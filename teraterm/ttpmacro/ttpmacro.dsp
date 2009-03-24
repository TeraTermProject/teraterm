# Microsoft Developer Studio Project File - Name="ttpmacro" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ttpmacro - Win32 Release
!MESSAGE NMAKE /f "ttpmacro.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "ttpmacro.mak" CFG="ttpmacro - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ttpmacro - Win32 Release" ("Win32 (x86) Application")
!MESSAGE "ttpmacro - Win32 Debug" ("Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ttpmacro - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
# ADD BASE CPP /nologo /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\source\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX"teraterm.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\source\common" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "ttpmacro - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir "."
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir "."
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\source\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX"teraterm.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\source\common" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386

!ENDIF 

# Begin Target

# Name "ttpmacro - Win32 Release"
# Name "ttpmacro - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\source\ttmacro\errdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\inpdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\msgdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\statdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\Common\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttmacro.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttmdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttmmain.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\..\source\ttmacro\errdlg.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\inpdlg.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\msgdlg.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\statdlg.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttl.h
# End Source File
# Begin Source File

SOURCE=..\..\source\common\ttlib.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttmacro.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttmbuff.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttmdde.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttmdlg.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttmenc.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttmmain.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ttmacro\ttmparse.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\Source\Ttmacro\Ttmacro.ico
# End Source File
# Begin Source File

SOURCE=..\..\Source\Ttmacro\ttpmacro.rc
# ADD BASE RSC /l 0x411 /i "\DEV\TERATERM\Source\Ttmacro"
# SUBTRACT BASE RSC /i "..\..\source\common"
# ADD RSC /l 0x411 /i "\DEV\TERATERM\Source\Ttmacro" /i "C:\DEV\TERATERM\source\ttmacro"
# SUBTRACT RSC /i "..\..\source\common"
# End Source File
# End Group
# Begin Group "Source Files (C)"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=..\..\Source\Ttmacro\ttl.c
# End Source File
# Begin Source File

SOURCE=..\..\Source\Common\ttlib.c
# End Source File
# Begin Source File

SOURCE=..\..\Source\Ttmacro\ttmbuff.c
# End Source File
# Begin Source File

SOURCE=..\..\Source\Ttmacro\ttmdde.c
# End Source File
# Begin Source File

SOURCE=..\..\Source\Ttmacro\ttmenc.c
# End Source File
# Begin Source File

SOURCE=..\..\Source\Ttmacro\ttmlib.c
# End Source File
# Begin Source File

SOURCE=..\..\Source\Ttmacro\ttmparse.c
# End Source File
# End Group
# End Target
# End Project
