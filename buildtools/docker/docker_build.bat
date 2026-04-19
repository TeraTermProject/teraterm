setlocal
set CUR=%~dp0
set PATH=C:\Program Files\Docker\Docker\resources;%PATH%
cd /d %CUR%
docker build --build-arg USER=%USERNAME% -t teraterm_build:1.0 -t teraterm_build:latest .
docker run -it --rm --detach-keys="ctrl-t" --mount type=bind,src=%CUR%,dst=/mnt teraterm_build:latest bash
endlocal
