setlocal
set CUR=%~dp0
set PATH=C:\Program Files\Docker\Docker\resources;%PATH%
cd /d %CUR%
docker build --build-arg USER=%USERNAME% -t teraterm_build:1.0 -t teraterm_build:latest .

set MOUNTS=--mount type=bind,src=%CUR%,dst=/mnt
if exist "%CUR%\..\..\.git" set MOUNTS=%MOUNTS% --mount type=bind,src=%CUR%\..\..,dst=/workspaces/teraterm/teraterm_host
docker run -it --rm %MOUNTS% teraterm_build:latest bash
endlocal
