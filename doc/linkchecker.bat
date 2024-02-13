@echo off
setlocal
set CUR=%~dp0
cd /d %CUR%

set PERL=perl

if not exist linkchecker.pl (
   curl https://raw.githubusercontent.com/saoyagi2/linkchecker/master/linkchecker.pl -o linkchecker.pl
)

pushd ja\html
echo ja
%PERL% ../../linkchecker.pl .
popd

pushd en\html
echo en
%PERL% ../../linkchecker.pl .
popd

pause
