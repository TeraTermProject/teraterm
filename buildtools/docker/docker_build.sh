CUR=$(cd "$(dirname "$0")" && pwd)
docker build --build-arg USER=${USER:-ttbuilder} -t teraterm_build:1.0 -t teraterm_build:latest .

MOUNTS="--mount type=bind,src=$CUR,dst=/mnt"
if [ -e "$CUR/../../.git" ]; then
    MOUNTS="$MOUNTS --mount type=bind,src=$CUR/../..,dst=/workspaces/teraterm/teraterm_host"
fi
docker run -it --rm $MOUNTS teraterm_build:latest bash
