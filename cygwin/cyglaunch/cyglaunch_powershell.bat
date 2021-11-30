set DIR=%~dp0
start "" "%DIR%\cygterm.exe" -t "%DIR%\ttermpro.exe %%s %%d /E /KR=UTF8 /KT=UTF8 /nossh /VTICON=TTERM" -s ""C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe""
