rem ●使用例
rem 通常のビルド
rem   makearchive.bat
rem リビルド
rem   makearchive.bat rebuild
rem デバッグ情報含む
rem   makearchive.bat debug
rem プラグイン含む
rem   makearchive.bat plugins

SET debug=no
SET plugins=no

if "%1"=="debug" SET debug=yes
if "%1"=="plugins" SET plugins=yes

rem "rebuild"を指定しない場合でも、SVNリビジョンを更新する。
if exist ..\teraterm\release\svnrev.exe goto svnrev
devenv /build release ..\teraterm\ttermpro.sln /project svnrev /projectconfig release
:svnrev
pushd ..\teraterm
release\svnrev.exe > ttpdlg\svnversion.h
popd

CALL makechm.bat
CALL build.bat %1

rem  for XP or later
set today=snapshot-%date:~0,4%%date:~5,2%%date:~8,2%

for %%a in (%today%, %today%_2, %today%_3, %today%_4, %today%_5) do (
set dst=%%a
if not exist %%a goto create
)

:create
del /s /q %dst%\*.*
mkdir %dst%

copy /y ..\teraterm\release\*.exe %dst%
copy /y ..\teraterm\release\*.dll %dst%
copy /y ..\ttssh2\ttxssh\Release\ttxssh.dll %dst%
copy /y ..\cygterm\cygterm.exe %dst%
copy /y ..\cygterm\cygterm.cfg %dst%
copy /y ..\cygterm\cyglaunch.exe %dst%
copy /y "..\cygterm\cygterm+.tar.gz" %dst%
copy /y ..\ttpmenu\Release\ttpmenu.exe %dst%
copy /y ..\TTProxy\Release\TTXProxy.dll %dst%
copy /y ..\TTXKanjiMenu\Release\ttxkanjimenu.dll %dst%
if "%plugins%"=="yes" copy /y ..\TTXSamples\Release\*.dll %dst%

rem Debug file
if "%debug%"=="yes" copy /y ..\teraterm\release\*.pdb %dst%
if "%debug%"=="yes" copy /y ..\ttssh2\ttxssh\Release\ttxssh.pdb %dst%
if "%debug%"=="yes" copy /y ..\ttpmenu\Release\ttxssh.pdb %dst%
if "%debug%"=="yes" copy /y ..\TTProxy\Release\TTXProxy.pdb %dst%
if "%debug%"=="yes" copy /y ..\TTXKanjiMenu\Release\ttxkanjimenu.pdb %dst%
if "%debug%"=="yes" if "%plugins%"=="yes" copy /y ..\TTXSamples\Release\*.pdb %dst%

if "%plugins%"=="yes" (
pushd %dst%
if exist TTXFixedWinSize.dll ren TTXFixedWinSize.dll _TTXFixedWinSize.dll
if exist TTXResizeWin.dll ren TTXResizeWin.dll _TTXResizeWin.dll
popd
)

copy /y ..\doc\jp\teratermj.chm %dst%
copy /y ..\doc\en\teraterm.chm %dst%

copy /y release\*.* %dst%
copy /y release\EDITOR.CNF %dst%\KEYBOARD.CNF
xcopy /s /e /y /i /exclude:archive-exclude.txt release\theme %dst%\theme
xcopy /s /e /y /i /exclude:archive-exclude.txt release\plugin %dst%\plugin
xcopy /s /e /y /i /exclude:archive-exclude.txt release\Collector %dst%\Collector
xcopy /s /e /y /i /exclude:archive-exclude.txt release\lang %dst%\lang
del /f %dst%\lang\English.lng

perl setini.pl release\TERATERM.INI > %dst%\TERATERM.INI
