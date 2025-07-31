rem Copyright (C) 2025- Tera Term Project
rem All rights reserved.
rem
rem  次のフォルダにバイナリを作成
rem   - ./teraterm
rem   - ./teraterm_pdb

@echo off
rem この外で set された RELEASE を上書きしないために setlocal する
setlocal

if "%TARGET%" == "" (set TARGET=Win32)

if not exist Output mkdir Output
if not exist Output\build mkdir Output\build

SET rebuild=
SET release=

if "%1"=="/?" goto help
if "%1"=="rebuild" SET rebuild=rebuild&& goto build
if "%1"=="release" SET release=yes&& goto build
if "%1"=="" goto build
goto help

:build
@echo on
if "%release%"=="yes" SET rebuild=rebuild

CALL makechm.bat
CALL build.bat %rebuild%
if ERRORLEVEL 1 goto fail

set dst=%~dp0Output\build\teraterm
echo dst=%dst%
call :create

endlocal
exit /b


rem create archive
rem  ファイルを %dst%, %dist%_pdb にコピーする
rem
:create
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
copy /y ..\cygwin\cygterm\cygterm.cfg %dst%
copy /y "..\cygwin\cygterm\cygterm+.tar.gz" %dst%
copy /y "..\cygwin\cygterm\cygterm+-x86_64\cygterm.exe" %dst%

if not exist ..\cygwin\cygterm\msys2term\msys2term.exe goto msys2term_pass
copy /y ..\cygwin\cygterm\msys2term\msys2term.exe %dst%
copy /y ..\cygwin\cygterm\msys2term.cfg %dst%
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

copy /y ..\doc\ja\teratermj.chm %dst%
copy /y ..\doc\en\teraterm.chm %dst%

copy /y release\*.* %dst%
copy /y release\IBMKEYB.CNF %dst%\KEYBOARD.CNF
xcopy /s /e /y /i /exclude:archive-exclude.txt release\theme %dst%\theme
xcopy /s /e /y /i /exclude:archive-exclude.txt release\plugin %dst%\plugin
xcopy /s /e /y /i /exclude:archive-exclude.txt release\lang %dst%\lang
xcopy /s /e /y /i /exclude:archive-exclude.txt release\lang_utf16le %dst%\lang_utf16le
del /f %dst%\lang\English.lng

perl setini.pl release\TERATERM.INI > %dst%\TERATERM.INI

copy nul %dst%\ttpmenu.ini
copy nul %dst%\portable.ini

exit /b 0

:help
echo Tera Termをビルド。(build Tera Term)
echo.
echo   %0          通常のビルド(Normal building)
echo   %0 release  リリースビルド(same as rebuild)
echo   %0 rebuild  リビルド(Re-building)
echo.
echo   次のフォルダにバイナリを作成
echo     - %~dp0teraterm
echo     - %~dp0teraterm_pdb
echo.
echo ex
echo   %0 rebuild ^>build.log 2^>^&1  ビルドログを採取する(Retrieve building log)
echo.
endlocal
exit /b

:fail
@echo off
echo ===================================================
echo ================= E R R O R =======================
echo ===================================================
echo.
echo ビルドに失敗しました (Failed to build source code)
endlocal
exit 1
