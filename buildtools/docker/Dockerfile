#FROM debian:trixie-20240701-slim
FROM debian:bookworm-20250113-slim

ARG USER=ttbuilder
ARG EMAIL=ttbuilder@example.com
ARG UID=1000
ARG GID=1000

ENV DEBIAN_FRONTEND=noninteractive

RUN \
	apt-get update && \
	apt-get install -y \
		cmake cmake-curses-gui \
		perl \
		gcc-mingw-w64-i686-win32 g++-mingw-w64-i686-win32 \
		fp-utils \
		git nasm

# locale
RUN \
	apt-get install -y locales && \
	sed -i 's/^# *\(en_US.UTF-8\)/\1/' /etc/locale.gen && \
	locale-gen en_US.UTF-8 && \
	update-locale LANG=en_US.UTF-8

# add user
RUN \
	apt-get install -y sudo && \
	groupadd -g ${GID} ${USER} && \
	useradd -u ${UID} -g ${GID} -m -s /bin/bash ${USER} && \
	usermod -aG sudo ${USER} && \
	echo "${USER} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USER} && \
	chmod 0440 /etc/sudoers.d/${USER} && \
	touch /home/${USER}/.sudo_as_admin_successful

# timezone
RUN \
	ln -snf /usr/share/zoneinfo/Asia/Tokyo /etc/localtime && \
	echo "Asia/Tokyo" > /etc/timezone

# cleanup
RUN \
	apt clean && \
	rm -rf /var/lib/apt/lists/*

# env
ENV USER=${USER}
ENV EMAIL=${USER}@example.com
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8
ENV BRANCH=main
ENV URL=https://github.com/TeraTermProject/teraterm.git

RUN mkdir -p /workspaces && chown ${USER}:${USER} /workspaces
USER ${USER}
WORKDIR /workspaces/teraterm

COPY build.sh .
RUN \
	git config --global user.name "${USER}" && \
	git config --global user.email "${EMAIL}"
