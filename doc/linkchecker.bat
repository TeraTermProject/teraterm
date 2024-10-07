@echo off
setlocal
set CUR=%~dp0
cd /d %CUR%

set NOPAUSE=1
call ..\buildtools\install_linkchecker.bat
call ..\buildtools\find_perl.bat

pushd ja\html
echo ja
%PERL% %CUR%/../buildtools/linkchecker/linkchecker.pl .
popd

pushd en\html
echo en
%PERL% %CUR%/../buildtools/linkchecker/linkchecker.pl .
popd

pause
