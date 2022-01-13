set DIR=%~dp0
if exist c:\cygwin\bin set PATH=c:\cygwin\bin;%PATH%
if exist c:\cygwin64\bin set PATH=c:\cygwin64\bin;%PATH%
start "" "%DIR%\cygterm.exe" -t "%DIR%\ttermpro.exe %%s %%d /E /KR=UTF8 /KT=UTF8 /nossh /VTICON=TTERM" -s ""C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe""
