#FROM debian:trixie-20240701-slim
FROM debian:bookworm-20250113-slim
RUN apt-get update \
  && apt-get install -y cmake perl cmake-curses-gui \
  && apt-get install -y gcc-mingw-w64-i686-win32 \
  && apt-get install -y g++-mingw-w64-i686-win32 \
  && apt-get install -y fp-utils \
  && apt-get install -y git nasm
COPY build.sh /
