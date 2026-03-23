/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS networking abstraction (BSD sockets wrapper).
 * Replaces Windows Sockets (WinSock) API.
 */
#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize networking (no-op on POSIX, needed for API compat) */
int tt_mac_net_init(void);
void tt_mac_net_cleanup(void);

/* TCP connection */
int tt_mac_tcp_connect(const char* host, int port);
int tt_mac_tcp_connect_timeout(const char* host, int port, int timeout_ms);
void tt_mac_tcp_disconnect(int sock);

/* Async DNS resolution (replaces WSAAsyncGetAddrInfo) */
typedef void (*tt_mac_dns_callback)(void* ctx, const char* host, struct addrinfo* result, int error);
int tt_mac_dns_resolve_async(const char* host, const char* service,
                              tt_mac_dns_callback callback, void* ctx);

/* Socket options */
int tt_mac_socket_set_nonblocking(int sock, int nonblocking);
int tt_mac_socket_set_keepalive(int sock, int enable);
int tt_mac_socket_set_nodelay(int sock, int enable);

/* I/O */
int tt_mac_socket_send(int sock, const void* data, int len);
int tt_mac_socket_recv(int sock, void* buffer, int len);
int tt_mac_socket_can_read(int sock, int timeout_ms);
int tt_mac_socket_can_write(int sock, int timeout_ms);

#ifdef __cplusplus
}
#endif
