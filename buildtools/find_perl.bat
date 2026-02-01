if not "%PERL%" == "" exit /b 0

if "%PERL_PATH%" == "" goto perl_in_path
set PERL=%PERL_PATH%\perl.exe
if exist "%PERL%" exist /b 0

:perl_in_path
set PERL=perl.exe
where %PERL% > nul 2>&1
if %errorlevel% == 0 exit /b 0

set PERL=%~dp0cygwin64\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=%~dp0perl\perl\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\Strawberry\perl\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\Perl64\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\Perl\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\cygwin64\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\cygwin\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=

echo perl can not found
if not "%NOPAUSE%" == "1" pause
exit
