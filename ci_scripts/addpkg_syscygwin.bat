echo %~dp0\addpkg_syscygwin.bat
set CUR=%~dp0
call %CUR%..\buildtools\find_cygwin.bat

%CYGWIN_PATH%\..\setup-x86_64.exe --quiet-mode --no-desktop --root %CYGWIN_PATH%\.. --packages cmake,perl-JSON,perl-LWP-Protocol-https

