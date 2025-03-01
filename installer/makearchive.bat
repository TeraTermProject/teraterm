@echo off
rem この外で set された RELEASE を上書きしないために setlocal する
setlocal

SET rebuild=
SET release=

if "%1"=="/?" goto help
if "%1"=="rebuild" SET rebuild=rebuild&& goto build
if "%1"=="release" SET release=yes&& goto build
if "%1"=="" goto build
goto help

:build
@echo on
if "%release%"=="yes" SET plugins=yes
if "%release%"=="yes" SET rebuild=rebuild

CALL makechm.bat
CALL build.bat %rebuild%
if ERRORLEVEL 1 goto fail
set release_bak=%release%
CALL ..\buildtools\svnrev\sourcetree_info.bat
set release=%release_bak%

rem  change folder name
if not "%release%"=="yes" goto snapshot
set dst=Output\teraterm-%VERSION%
goto create

:snapshot
set dst=snapshot-%DATE%_%TIME%

:create
del /s /q %dst%\*.*
mkdir %dst%
del /s /q %dst%_pdb\*.*
mkdir %dst%_pdb

copy /y ..\teraterm\release\*.exe %dst%
copy /y ..\teraterm\release\*.dll %dst%
copy /y ..\teraterm\release\*.pdb %dst%_pdb
copy /y ..\ttssh2\ttxssh\Release\ttxssh.dll %dst%
copy /y ..\ttssh2\ttxssh\Release\ttxssh.pdb %dst%_pdb
copy /y ..\TTProxy\Release\TTXProxy.dll %dst%
copy /y ..\TTProxy\Release\TTXProxy.pdb %dst%_pdb
copy /y ..\TTXKanjiMenu\Release\ttxkanjimenu.dll %dst%
copy /y ..\TTXKanjiMenu\Release\ttxkanjimenu.pdb %dst%_pdb

copy /y ..\cygwin\Release\cyglaunch.exe %dst%
copy /y ..\cygwin\cygterm\cygterm.cfg %dst%
copy /y "..\cygwin\cygterm\cygterm+.tar.gz" %dst%
copy /y "..\cygwin\cygterm\cygterm+-x86_64\cygterm.exe" %dst%

if not exist ..\cygwin\cygterm\msys2term\msys2term.exe goto msys2term_pass
copy /y ..\cygwin\cygterm\msys2term\msys2term.exe %dst%
copy /y ..\cygwin\cygterm\msys2term.cfg %dst%
:msys2term_pass

copy /y ..\ttpmenu\Release\ttpmenu.exe %dst%
copy /y ..\ttpmenu\Release\ttpmenu.pdb %dst%_pdb
copy /y ..\ttpmenu\readme.txt %dst%\ttmenu_readme-j.txt

copy /y ..\TTXSamples\Release\TTXAdditionalTitle.dll %dst%
copy /y ..\TTXSamples\Release\TTXAdditionalTitle.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXAlwaysOnTop.dll %dst%
copy /y ..\TTXSamples\Release\TTXAlwaysOnTop.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXCallSysMenu.dll %dst%
copy /y ..\TTXSamples\Release\TTXCallSysMenu.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXCommandLineOpt.dll %dst%
copy /y ..\TTXSamples\Release\TTXCommandLineOpt.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXCopyIniFile.dll %dst%
copy /y ..\TTXSamples\Release\TTXCopyIniFile.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXFixedWinSize.dll %dst%
copy /y ..\TTXSamples\Release\TTXFixedWinSize.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXKcodeChange.dll %dst%
copy /y ..\TTXSamples\Release\TTXKcodeChange.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXOutputBuffering.dll %dst%
copy /y ..\TTXSamples\Release\TTXOutputBuffering.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXRecurringCommand.dll %dst%
copy /y ..\TTXSamples\Release\TTXRecurringCommand.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXResizeMenu.dll %dst%
copy /y ..\TTXSamples\Release\TTXResizeMenu.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXResizeWin.dll %dst%
copy /y ..\TTXSamples\Release\TTXResizeWin.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXShowCommandLine.dll %dst%
copy /y ..\TTXSamples\Release\TTXShowCommandLine.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXViewMode.dll %dst%
copy /y ..\TTXSamples\Release\TTXViewMode.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXtest.dll %dst%
copy /y ..\TTXSamples\Release\TTXtest.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXttyplay.dll %dst%
copy /y ..\TTXSamples\Release\TTXttyplay.pdb %dst%_pdb
copy /y ..\TTXSamples\Release\TTXttyrec.dll %dst%
copy /y ..\TTXSamples\Release\TTXttyrec.pdb %dst%_pdb

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

if "%release%"=="yes" (
copy nul %dst%\ttpmenu.ini
copy nul %dst%\portable.ini
)

echo dst=%~dp0%dst%

endlocal
exit /b

:help
echo Tera Termをビルド。(build Tera Term)
echo.
echo   %0          通常のビルド(Normal building)
echo   %0 rebuild  リビルド(Re-building)
echo   %0 release  リリースビルド(rebuild + フォルダ名が特殊(Normal + Plugins building + unique folder naming))
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
