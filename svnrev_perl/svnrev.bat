@echo off
setlocal
cd /d %~dp0

set SVNVERSION_H=../teraterm/ttpdlg/svnversion.h

if EXIST %SVNVERSION_H% goto finish

set PERL=..\libs\perl\perl\bin\perl.exe
if NOT EXIST %PERL% set PERL=perl.exe

%PERL% svnrev.pl --root .. --header %SVNVERSION_H%

:finish
