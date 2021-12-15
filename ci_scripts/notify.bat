echo notify.bat %1
set result=%1
if "%APPVEYOR%" == "" goto pass_discord

set PATH=c:\cygwin64\bin;%PATH%
if exist c:\cygwin64\setup-x86_64.exe (
  c:\cygwin64\setup-x86_64.exe --quiet-mode --packages perl --packages perl-JSON --packages perl-LWP-Protocol-https
  )
perl ci_scripts/notify_discord.pl %result%
:pass_discord
