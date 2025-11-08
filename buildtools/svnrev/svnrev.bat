rem @echo off
setlocal
cd /d %~dp0
set BAT=%~n0%~x0

set SVNVERSION_H=../../teraterm/common/svnversion.h
set SVNVERSION_H_DOS=..\..\teraterm\common\svnversion.h
set SVNVERSION_H_OLD=..\..\teraterm\ttpdlg\svnversion.h
set DO_NOT_RUN_SVNREV=do_not_run_svnrev.txt

if NOT "%1" == "" set ARCHITECTURE=%1
if "%ARCHITECTURE%" == "" set ARCHITECTURE=Win32

if NOT EXIST %SVNVERSION_H% goto env_perl
if NOT EXIST %DO_NOT_RUN_SVNREV% goto env_perl
@echo %BAT%: Find %DO_NOT_RUN_SVNREV%, Do not run svnrev.pl
goto finish

:env_perl
if NOT "%PERL%" == "" goto found_perl

:search_perl
call ..\find_perl.bat
if not "%PERL%" == "" goto found_perl

:no_perl
@echo %BAT%: perl not found
@echo %BAT%: default svnversion.h is used
if exist sourcetree_info.bat del sourcetree_info.bat
type svnversion.default.h > %SVNVERSION_H_DOS%
goto finish

:found_perl
@echo %BAT%: perl=%PERL%
if EXIST %SVNVERSION_H_OLD% del %SVNVERSION_H_OLD%
%PERL% svnrev.pl --root ../.. --architecture %ARCHITECTURE% --header %SVNVERSION_H% --cmake build_config.cmake --json build_config.json --isl ../../installer/build_config.isl
goto finish


:finish
