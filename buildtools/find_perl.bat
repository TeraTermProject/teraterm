if not "%PERL%" == "" exit /b 0

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

echo perl can not found
pause
exit
