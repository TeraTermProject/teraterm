FROM debian:unstable-20231120-slim
RUN apt-get update \
  && apt-get install -y cmake perl cmake-curses-gui \
  && apt-get install -y g++-mingw-w64 \
  && apt-get install -y fp-utils \
  && apt-get install -y git
COPY build.sh /
