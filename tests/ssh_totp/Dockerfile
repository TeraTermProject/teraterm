FROM debian:trixie-20240701-slim
RUN apt-get update
RUN apt-get install -y sudo vim nano
RUN apt-get install -y libpam-google-authenticator openssh-server
# /etc/pam.d/sshd
COPY pam-sshd /tmp
RUN mv /etc/pam.d/sshd /etc/pam.d/sshd_orig \
    && mv /tmp/pam-sshd /etc/pam.d/sshd
# /etc/sshd/sshd_config
COPY ssh-sshd_config /tmp
RUN mv /etc/ssh/sshd_config /etc/ssh/sshd_config_orig \
    && mv /tmp/ssh-sshd_config /etc/ssh/sshd_config
# user 'test'
RUN useradd -m -p $(perl -e 'print crypt("password", "\$6\$salt03")') test
RUN mkdir ~test/.ssh && chown test:test ~test/.ssh && chmod go-rwx ~test/.ssh
COPY tt_test_key.pub /tmp
RUN mv /tmp/tt_test_key.pub ~test/.ssh/authorized_keys
RUN chown test:test ~test/.ssh/authorized_keys && chmod go-rwx ~test/.ssh/authorized_keys
# history
COPY bash_history /tmp
RUN mv /tmp/bash_history /root/.bash_history
