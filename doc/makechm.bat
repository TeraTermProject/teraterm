setlocal

set NOPAUSE=1
call ..\buildtools\install_sbapplocale.bat

if exist "%Programfiles(x86)%\HTML Help Workshop\hhc.exe" (
    set HELP_COMPILER="%Programfiles(x86)%\HTML Help Workshop\hhc.exe"
) else (
    set HELP_COMPILER=C:\progra~1\htmlhe~1\hhc.exe
)

REM for winxp installation on other drive or other language
if exist "%Programfiles%\HTML Help Workshop\hhc.exe" (
    set HELP_COMPILER="%Programfiles%\HTML Help Workshop\hhc.exe"
)

set SBAppLocale=..\buildtools\SBAppLocale\SBAppLocale.exe

CALL convtext.bat

:JA
for /f "delims=" %%i in ('perl htmlhelp_update_check.pl ja teratermj.chm') do @set updated=%%i
if "%updated%"=="updated" (
    perl htmlhelp_index_make.pl ja html -o ja\Index.hhk
    %SBAppLocale% 1041 %HELP_COMPILER% ja\teraterm.hhp
)

:English
for /f "delims=" %%i in ('perl htmlhelp_update_check.pl en teraterm.chm') do @set updated=%%i
if "%updated%"=="updated" (
    perl htmlhelp_index_make.pl en html -o en\Index.hhk
    %SBAppLocale% 1033 %HELP_COMPILER% en\teraterm.hhp
)

endlocal
