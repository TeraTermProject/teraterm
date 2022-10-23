set DIR=%~dp0
set PATH=c:\msys64\usr\bin;%PATH%
start "" "%DIR%\msys2term.exe" -t "%DIR%\ttermpro.exe %%s %%d /E /KR=UTF8 /KT=UTF8 /nossh /VTICON=TTERM" -s ""C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe""
