setlocal
set CUR=%~dp0
set PATH=C:\Program Files\Docker\Docker\resources;%PATH%
cd /d %CUR%
docker build -t teraterm_totp:1.0 -t teraterm_totp:latest .
docker run -it --rm --detach-keys="ctrl-t" --mount type=bind,src=%CUR%../..,dst=/mnt -p22:22 teraterm_totp:latest bash
pause
endlocal
