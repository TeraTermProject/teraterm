/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS networking implementation using BSD sockets.
 */

#include "macos_net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>

int tt_mac_net_init(void) {
    /* BSD sockets don't need initialization like WinSock */
    return 0;
}

void tt_mac_net_cleanup(void) {
    /* No-op on BSD */
}

int tt_mac_tcp_connect(const char* host, int port) {
    return tt_mac_tcp_connect_timeout(host, port, 30000);
}

int tt_mac_tcp_connect_timeout(const char* host, int port, int timeout_ms) {
    if (!host) return -1;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host, port_str, &hints, &result);
    if (err != 0) return -1;

    int sock = -1;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) continue;

        /* Set non-blocking for timeout-based connect */
        int flags = fcntl(sock, F_GETFL);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        int ret = connect(sock, rp->ai_addr, rp->ai_addrlen);
        if (ret == 0) {
            /* Connected immediately */
            fcntl(sock, F_SETFL, flags);  /* Restore blocking */
            break;
        }

        if (errno == EINPROGRESS) {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(sock, &wfds);

            struct timeval tv;
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;

            ret = select(sock + 1, NULL, &wfds, NULL, &tv);
            if (ret > 0) {
                int so_error;
                socklen_t len = sizeof(so_error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error == 0) {
                    fcntl(sock, F_SETFL, flags);  /* Restore blocking */
                    break;
                }
            }
        }

        close(sock);
        sock = -1;
    }

    freeaddrinfo(result);
    return sock;
}

void tt_mac_tcp_disconnect(int sock) {
    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
}

/* Async DNS resolution thread context */
typedef struct {
    char* host;
    char* service;
    tt_mac_dns_callback callback;
    void* ctx;
} TTDnsResolveCtx;

static void* dns_resolve_thread(void* arg) {
    TTDnsResolveCtx* dctx = (TTDnsResolveCtx*)arg;

    struct addrinfo hints, *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(dctx->host, dctx->service, &hints, &result);

    if (dctx->callback) {
        dctx->callback(dctx->ctx, dctx->host, err == 0 ? result : NULL, err);
    }

    if (err != 0 && result) {
        freeaddrinfo(result);
    }

    free(dctx->host);
    free(dctx->service);
    free(dctx);
    return NULL;
}

int tt_mac_dns_resolve_async(const char* host, const char* service,
                              tt_mac_dns_callback callback, void* ctx) {
    TTDnsResolveCtx* dctx = (TTDnsResolveCtx*)calloc(1, sizeof(TTDnsResolveCtx));
    if (!dctx) return -1;

    dctx->host = strdup(host ? host : "");
    dctx->service = service ? strdup(service) : NULL;
    dctx->callback = callback;
    dctx->ctx = ctx;

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int ret = pthread_create(&thread, &attr, dns_resolve_thread, dctx);
    pthread_attr_destroy(&attr);

    if (ret != 0) {
        free(dctx->host);
        free(dctx->service);
        free(dctx);
        return -1;
    }

    return 0;
}

int tt_mac_socket_set_nonblocking(int sock, int nonblocking) {
    int flags = fcntl(sock, F_GETFL);
    if (flags < 0) return -1;
    if (nonblocking)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;
    return fcntl(sock, F_SETFL, flags);
}

int tt_mac_socket_set_keepalive(int sock, int enable) {
    return setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
}

int tt_mac_socket_set_nodelay(int sock, int enable) {
    return setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
}

int tt_mac_socket_send(int sock, const void* data, int len) {
    return (int)send(sock, data, len, 0);
}

int tt_mac_socket_recv(int sock, void* buffer, int len) {
    return (int)recv(sock, buffer, len, 0);
}

int tt_mac_socket_can_read(int sock, int timeout_ms) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    return select(sock + 1, &rfds, NULL, NULL, &tv);
}

int tt_mac_socket_can_write(int sock, int timeout_ms) {
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    return select(sock + 1, NULL, &wfds, NULL, &tv);
}
