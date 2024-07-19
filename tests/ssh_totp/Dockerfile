FROM debian:trixie-20240701-slim
RUN apt-get update
RUN apt-get install -y sudo
RUN apt-get install -y libpam-google-authenticator openssh-server
RUN echo >> /etc/pam.d/sshd
RUN echo "auth required pam_google_authenticator.so nullok" >> /etc/pam.d/sshd
RUN echo "auth required pam_permit.so >> /etc/pam.d/sshd" >> /etc/pam.d/sshd
RUN mv /etc/ssh/sshd_config /etc/ssh/sshd_config_orig
COPY sshd_config_pw_totp /etc/ssh/
RUN mv /etc/ssh/sshd_config_pw_totp /etc/ssh/sshd_config
#
RUN useradd -m -p $(perl -e 'print crypt("password", "\$6\$salt03")') test
RUN echo "sudo -u test google-authenticator --time-based --qr-mode=NONE" >> /root/.bash_history
RUN echo "/usr/sbin/sshd -d" >> /root/.bash_history
