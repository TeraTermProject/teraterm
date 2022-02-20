@echo off
setlocal
cd /d %~dp0
set BAT=%~n0%~x0

set SVNVERSION_H=../../teraterm/ttpdlg/svnversion.h
set SVNVERSION_H_DOS=..\..\teraterm\ttpdlg\svnversion.h
set DO_NOT_RUN_SVNREV=do_not_run_svnrev.txt

if NOT EXIST %SVNVERSION_H% goto env_perl
if NOT EXIST %DO_NOT_RUN_SVNREV% goto env_perl
@echo %BAT%: Find %DO_NOT_RUN_SVNREV%, Do not run svnrev.pl
goto finish

:env_perl
if NOT "%PERL%" == "" goto found_perl

:search_perl
set PERL=perl.exe
where %PERL% > nul 2>&1
if %errorlevel% == 0 goto found_perl
set PERL=%~dp0..\buildtools\perl\perl\bin\perl.exe
if exist %PERL% goto found_perl
set PERL=C:\Strawberry\perl\bin\perl.exe
if exist %PERL% goto found_perl
set PERL=C:\Perl64\bin\perl.exe
if exist %PERL% goto found_perl
set PERL=C:\Perl\bin\perl.exe
if exist %PERL% goto found_perl
set PERL=C:\cygwin64\usr\bin\perl.exe
if exist %PERL% goto found_perl
set PERL=C:\cygwin\usr\bin\perl.exe
if exist %PERL% goto found_perl
goto no_perl

:no_perl
@echo %BAT%: perl not found
@echo %BAT%: default svnversion.h is used
if exist sourcetree_info.bat del sourcetree_info.bat
type svnversion.default.h > %SVNVERSION_H_DOS%
goto finish

:found_perl
@echo %BAT%: perl=%PERL%
%PERL% svnrev.pl --root ../.. --header %SVNVERSION_H%
goto finish


:finish
