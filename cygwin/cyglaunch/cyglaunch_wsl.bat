set DIR=%~dp0
start "" "%DIR%\cygterm.exe" -t "%DIR%\ttermpro.exe %%s %%d /E /KR=UTF8 /KT=UTF8 /nossh /VTICON=TTERM" -s "c:/windows/system32/wsl.exe"
