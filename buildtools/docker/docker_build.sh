docker build -t teraterm_build:1.0 -t teraterm_build:latest .
docker run -it --rm --detach-keys="ctrl-t" --mount type=bind,src=`pwd`/../..,dst=/mnt teraterm_build:latest bash
