rem Copyright (C) 2006- Tera Term Project
rem All rights reserved.
rem
rem  - build_common.bat でビルドされたバイナリ（Output/build/teraterm_common へコピー済み）
rem  - build_arch.bat でビルドされたバイナリ
rem  を次のフォルダにまとめる
rem  - Output/build/teraterm-%arch%
rem  - Output/build/teraterm-%arch%_pdb
setlocal


rem arch for package
if "%arch%" == "" (set arch=x86)


rem TARGET for package
if "%TARGET%" == "" (set TARGET=Win32)


rem Change the working directory to the location of this BAT file
pushd %~dp0


if not exist Output mkdir Output
if not exist Output\build mkdir Output\build

set common=%~dp0Output\build\teraterm_common
set dst=%~dp0Output\build\teraterm-%arch%
echo dst=%dst%


rem ファイルを %dst%, %dist%_pdb にコピーする
del /s /q %dst%\*.*
mkdir %dst%
del /s /q %dst%_pdb\*.*
mkdir %dst%_pdb

copy /y ..\teraterm\Release.%TARGET%\*.exe %dst%
copy /y ..\teraterm\Release.%TARGET%\*.dll %dst%
copy /y ..\teraterm\Release.%TARGET%\*.pdb %dst%_pdb
copy /y ..\ttssh2\ttxssh\Release.%TARGET%\ttxssh.dll %dst%
copy /y ..\ttssh2\ttxssh\Release.%TARGET%\ttxssh.pdb %dst%_pdb
copy /y ..\TTProxy\Release.%TARGET%\TTXProxy.dll %dst%
copy /y ..\TTProxy\Release.%TARGET%\TTXProxy.pdb %dst%_pdb
copy /y ..\TTXKanjiMenu\Release.%TARGET%\ttxkanjimenu.dll %dst%
copy /y ..\TTXKanjiMenu\Release.%TARGET%\ttxkanjimenu.pdb %dst%_pdb

copy /y ..\cygwin\Release.%TARGET%\cyglaunch.exe %dst%

copy /y %common%\cygterm.cfg %dst%
copy /y "%common%\cygterm+.tar.gz" %dst%
copy /y "%common%\cygterm.exe" %dst%

if not exist %common%\msys2term.exe goto msys2term_pass
copy /y %common%\msys2term.exe %dst%
copy /y %common%\msys2term.cfg %dst%
:msys2term_pass

copy /y ..\ttpmenu\Release.%TARGET%\ttpmenu.exe %dst%
copy /y ..\ttpmenu\Release.%TARGET%\ttpmenu.pdb %dst%_pdb
copy /y ..\ttpmenu\readme.txt %dst%\ttmenu_readme-j.txt

copy /y ..\TTXSamples\Release.%TARGET%\TTXAdditionalTitle.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXAdditionalTitle.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXAlwaysOnTop.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXAlwaysOnTop.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXCallSysMenu.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXCallSysMenu.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXCommandLineOpt.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXCommandLineOpt.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXCopyIniFile.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXCopyIniFile.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXFixedWinSize.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXFixedWinSize.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXKcodeChange.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXKcodeChange.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXOutputBuffering.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXOutputBuffering.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXRecurringCommand.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXRecurringCommand.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXResizeMenu.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXResizeMenu.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXResizeWin.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXResizeWin.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXShowCommandLine.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXShowCommandLine.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXViewMode.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXViewMode.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXtest.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXtest.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXttyplay.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXttyplay.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXttyrec.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXttyrec.pdb %dst%_pdb
copy /y ..\TTXSamples\Release.%TARGET%\TTXChangeFontSize.dll %dst%
copy /y ..\TTXSamples\Release.%TARGET%\TTXChangeFontSize.pdb %dst%_pdb

pushd %dst%
ren TTXFixedWinSize.dll _TTXFixedWinSize.dll
ren TTXFixedWinSize.dll _TTXFixedWinSize.dll
ren TTXOutputBuffering.dll _TTXOutputBuffering.dll
ren TTXResizeWin.dll _TTXResizeWin.dll
ren TTXResizeWin.dll _TTXResizeWin.dll
ren TTXShowCommandLine.dll _TTXShowCommandLine.dll
ren TTXtest.dll _TTXtest.dll
popd

copy /y %common%\teratermj.chm %dst%
copy /y %common%\teraterm.chm %dst%

copy /y release\*.* %dst%
copy /y release\IBMKEYB.CNF %dst%\KEYBOARD.CNF
xcopy /s /e /y /i /exclude:archive-exclude.txt release\theme %dst%\theme
xcopy /s /e /y /i /exclude:archive-exclude.txt release\plugin %dst%\plugin
xcopy /s /e /y /i /exclude:archive-exclude.txt %common%\lang %dst%\lang
xcopy /s /e /y /i /exclude:archive-exclude.txt %common%\lang_utf16le %dst%\lang_utf16le
del /f %dst%\lang\en_US.lng

perl setini.pl release\TERATERM.INI > %dst%\TERATERM.INI

copy nul %dst%\ttpmenu.ini
copy nul %dst%\portable.ini

popd
exit /b 0

:help
echo.
echo ex
echo   %0 rebuild ^>build.log 2^>^&1  ビルドログを採取する(Retrieve building log)
echo.
popd
endlocal
exit /b 0

:fail
@echo off
echo ===================================================
echo ================= E R R O R =======================
echo ===================================================
echo.
echo ビルドに失敗しました (Failed to build source code)
popd
endlocal
exit /b 1
