@echo off
setlocal
set CUR=%~dp0
cd /d %CUR%

set NOPAUSE=1
call ..\buildtools\install_linkchecker.bat
call ..\buildtools\find_perl.bat
rem PATH ã‚Ì perl ‚Å HTML::Parser ‚ªŽg‚¦‚È‚¢‚Æ‚«‚Í buildtools ‚Ì perl ‚ðŽg—p‚·‚é
%PERL% -MHTML::Parser -e 1 > nul 2>&1
if %errorlevel% == 0 goto perl_found
set PERL=%CUR%..\buildtools\cygwin64\bin\perl.exe
:perl_found

pushd ja\html
echo ja
%PERL% %CUR%/../buildtools/linkchecker/linkchecker.pl .
popd

pushd en\html
echo en
%PERL% %CUR%/../buildtools/linkchecker/linkchecker.pl .
popd

pause
