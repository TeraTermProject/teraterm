
if exist "%Programfiles(x86)%\HTML Help Workshop\hhc.exe" (
	set HELP_COMPILER="%Programfiles(x86)%\HTML Help Workshop\hhc.exe"
) else (
	set HELP_COMPILER=C:\progra~1\htmlhe~1\hhc.exe
)

REM for winxp installation on other drive or other language
if exist "%Programfiles%\HTML Help Workshop\hhc.exe" (
set HELP_COMPILER="%Programfiles%\HTML Help Workshop\hhc.exe"
)

set updated=

CALL convtext.bat

REM Check Japanese version Windows
chcp | find "932" > NUL
if ERRORLEVEL 1 goto English

for /f "delims=" %%i in ('perl htmlhelp_update_check.pl ja teratermj.chm') do @set updated=%%i
if "%updated%"=="updated" (
perl htmlhelp_index_make.pl ja html > ja\Index.hhk
%HELP_COMPILER% ja\teraterm.hhp
)

:English
for /f "delims=" %%i in ('perl htmlhelp_update_check.pl en teraterm.chm') do @set updated=%%i
if "%updated%"=="updated" (
perl htmlhelp_index_make.pl en html > en\Index.hhk
%HELP_COMPILER% en\teraterm.hhp
)
