/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS PTY implementation using posix_openpt/forkpty.
 */

#include "macos_pty.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <util.h>    /* forkpty() on macOS */
#include <pwd.h>

typedef struct {
    int master_fd;
    pid_t child_pid;
} TTMacPtyImpl;

TTMacPty tt_mac_pty_create(const char* shell, int cols, int rows) {
    TTMacPtyImpl* impl = (TTMacPtyImpl*)calloc(1, sizeof(TTMacPtyImpl));
    if (!impl) return NULL;

    struct winsize ws;
    ws.ws_col = cols;
    ws.ws_row = rows;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    pid_t pid = forkpty(&impl->master_fd, NULL, NULL, &ws);
    if (pid < 0) {
        free(impl);
        return NULL;
    }

    if (pid == 0) {
        /* Child process */
        const char* sh = shell;
        if (!sh) sh = getenv("SHELL");
        if (!sh) {
            struct passwd* pw = getpwuid(getuid());
            if (pw) sh = pw->pw_shell;
        }
        if (!sh) sh = "/bin/zsh";  /* macOS default */

        /* Set TERM */
        setenv("TERM", "xterm-256color", 1);

        /* Execute shell */
        execlp(sh, sh, "--login", NULL);
        _exit(127);  /* exec failed */
    }

    /* Parent process */
    impl->child_pid = pid;

    /* Set master to non-blocking */
    int flags = fcntl(impl->master_fd, F_GETFL);
    fcntl(impl->master_fd, F_SETFL, flags | O_NONBLOCK);

    return (TTMacPty)impl;
}

void tt_mac_pty_destroy(TTMacPty pty) {
    if (!pty) return;
    TTMacPtyImpl* impl = (TTMacPtyImpl*)pty;

    if (impl->child_pid > 0) {
        kill(impl->child_pid, SIGHUP);
        waitpid(impl->child_pid, NULL, WNOHANG);
    }

    if (impl->master_fd >= 0) {
        close(impl->master_fd);
    }

    free(impl);
}

int tt_mac_pty_get_fd(TTMacPty pty) {
    if (!pty) return -1;
    return ((TTMacPtyImpl*)pty)->master_fd;
}

int tt_mac_pty_read(TTMacPty pty, void* buffer, int size) {
    if (!pty) return -1;
    TTMacPtyImpl* impl = (TTMacPtyImpl*)pty;
    ssize_t n = read(impl->master_fd, buffer, size);
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return 0;
    return (int)n;
}

int tt_mac_pty_write(TTMacPty pty, const void* buffer, int size) {
    if (!pty) return -1;
    TTMacPtyImpl* impl = (TTMacPtyImpl*)pty;
    return (int)write(impl->master_fd, buffer, size);
}

int tt_mac_pty_resize(TTMacPty pty, int cols, int rows) {
    if (!pty) return -1;
    TTMacPtyImpl* impl = (TTMacPtyImpl*)pty;

    struct winsize ws;
    ws.ws_col = cols;
    ws.ws_row = rows;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    return ioctl(impl->master_fd, TIOCSWINSZ, &ws);
}

int tt_mac_pty_is_alive(TTMacPty pty) {
    if (!pty) return 0;
    TTMacPtyImpl* impl = (TTMacPtyImpl*)pty;

    int status;
    pid_t ret = waitpid(impl->child_pid, &status, WNOHANG);
    if (ret == 0) return 1;  /* Still running */
    return 0;
}

int tt_mac_pty_get_pid(TTMacPty pty) {
    if (!pty) return -1;
    return (int)((TTMacPtyImpl*)pty)->child_pid;
}
