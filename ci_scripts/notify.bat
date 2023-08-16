rem perl‚Å WEBHOOK_URL ‚É’Ê’m‚·‚é

echo notify.bat %1
set result=%1

pushd %~dp0

if "%APPVEYOR%" == "" goto pass_discord
if "%WEBHOOK_URL%" == "" goto pass_discoard
if "%CYGWIN_PATH%" == "" call %~dp0..\buildtools\find_cygwin.bat
if "%CYGWIN_PATH%" == "" goto pass_discoard

rem NEED perl-JSON,perl-LWP-Protocol-https packages
%CYGWIN_PATH%\perl.exe notify_discord.pl %result%

:pass_discord
popd
