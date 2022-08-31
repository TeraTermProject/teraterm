
if exist "%Programfiles(x86)%\HTML Help Workshop\hhc.exe" (
	set HELP_COMPILER="%Programfiles(x86)%\HTML Help Workshop\hhc.exe"
) else (
	set HELP_COMPILER=C:\progra~1\htmlhe~1\hhc.exe
)

REM for winxp installation on other drive or other language
if exist "%Programfiles%\HTML Help Workshop\hhc.exe" (
set HELP_COMPILER="%Programfiles%\HTML Help Workshop\hhc.exe"
)

if defined SystemRoot (
    set FIND=%SystemRoot%\system32\find.exe
    set CHCP=%SystemRoot%\system32\chcp.com
) else (
    set FIND=find.exe
    set CHCP=chcp.exe
)

set updated=

CALL convtext.bat

REM Check Japanese version Windows
if "%APPVEYOR%" == "True" goto JA
%CHCP% | %FIND% "932" > NUL
if NOT ERRORLEVEL 1 goto JA
%CHCP% | %FIND% "65001" > NUL
if NOT ERRORLEVEL 1 goto JA
goto English

:JA
for /f "delims=" %%i in ('perl htmlhelp_update_check.pl ja teratermj.chm') do @set updated=%%i
if "%updated%"=="updated" (
perl htmlhelp_index_make.pl ja html -o ja\Index.hhk
%HELP_COMPILER% ja\teraterm.hhp
)

:English
for /f "delims=" %%i in ('perl htmlhelp_update_check.pl en teraterm.chm') do @set updated=%%i
if "%updated%"=="updated" (
perl htmlhelp_index_make.pl en html -o en\Index.hhk
%HELP_COMPILER% en\teraterm.hhp
)
