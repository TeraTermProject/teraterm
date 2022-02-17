set DIR=%~dp0
set PATH=C:\Program Files\Git\usr\bin;%PATH%
start "" "%DIR%\msys2term.exe" -t "%DIR%\ttermpro.exe %%s %%d /E /KR=UTF8 /KT=UTF8 /nossh /VTICON=TTERM"
