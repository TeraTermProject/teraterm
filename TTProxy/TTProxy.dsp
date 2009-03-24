# Microsoft Developer Studio Project File - Name="TTProxy" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TTProxy - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "TTProxy.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "TTProxy.mak" CFG="TTProxy - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "TTProxy - Win32 Release" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE "TTProxy - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TTProxy - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TTX_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "../teraterm/source/teraterm" /I "../teraterm/source/common" /I "YCL/include" /I "../openssl/inc32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TTX_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wsock32.lib /nologo /dll /map /machine:I386 /def:".\TTX.def" /out:"TTXProxy.dll" /implib:"Release/TTXProxy.lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "TTProxy - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TTX_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /I "../teraterm/source/teraterm" /I "../teraterm/source/common" /I "YCL/include" /I "../openssl/inc32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TTX_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wsock32.lib /nologo /dll /debug /machine:I386 /out:"Debug/TTXProxy.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "TTProxy - Win32 Release"
# Name "TTProxy - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TTProxy.cpp
# End Source File
# Begin Source File

SOURCE=.\TTProxy.rc
# End Source File
# Begin Source File

SOURCE=.\TTX.def

!IF  "$(CFG)" == "TTProxy - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "TTProxy - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Hooker.h
# End Source File
# Begin Source File

SOURCE=.\Logger.h
# End Source File
# Begin Source File

SOURCE=.\ProxyWSockHook.h
# End Source File
# Begin Source File

SOURCE=.\SSLSocket.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TTProxy.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "YCL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\YCL\include\YCL\Array.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\ComboBoxCtrl.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\common.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\Dialog.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\DynamicLinkLibrary.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\EditBoxCtrl.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\Enumeration.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\FileVersion.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\HASHCODE.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\Hashtable.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\IniFile.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\Integer.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\libc.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\Object.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\Pointer.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\Resource.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\StringBuffer.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\StringUtil.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\ValueCtrl.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\Vector.h
# End Source File
# Begin Source File

SOURCE=.\YCL\include\YCL\Window.h
# End Source File
# End Group
# Begin Group "TeraTermPro"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ttsrcp23\source\common\teraterm.h
# End Source File
# Begin Source File

SOURCE=..\ttsrcp23\source\teraterm\ttdialog.h
# End Source File
# Begin Source File

SOURCE=..\ttsrcp23\source\common\ttplugin.h
# End Source File
# Begin Source File

SOURCE=..\ttsrcp23\source\teraterm\ttsetup.h
# End Source File
# Begin Source File

SOURCE=..\ttsrcp23\source\common\tttypes.h
# End Source File
# Begin Source File

SOURCE=..\ttsrcp23\source\teraterm\ttwsk.h
# End Source File
# End Group
# End Target
# End Project
