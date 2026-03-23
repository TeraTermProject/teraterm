/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS PTY (pseudo-terminal) support.
 * Enables local shell sessions on macOS (replacing Cygwin CygTerm on Windows).
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void* TTMacPty;

/* Create a PTY with the given shell (NULL = default shell from $SHELL) */
TTMacPty tt_mac_pty_create(const char* shell, int cols, int rows);
void tt_mac_pty_destroy(TTMacPty pty);

/* Get the file descriptor for I/O (for select/poll) */
int tt_mac_pty_get_fd(TTMacPty pty);

/* Read/write */
int tt_mac_pty_read(TTMacPty pty, void* buffer, int size);
int tt_mac_pty_write(TTMacPty pty, const void* buffer, int size);

/* Resize terminal */
int tt_mac_pty_resize(TTMacPty pty, int cols, int rows);

/* Check if child process is still running */
int tt_mac_pty_is_alive(TTMacPty pty);

/* Get child process PID */
int tt_mac_pty_get_pid(TTMacPty pty);

#ifdef __cplusplus
}
#endif
