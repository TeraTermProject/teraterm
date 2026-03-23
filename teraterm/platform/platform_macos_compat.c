/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS implementation of basic Windows API compatibility functions.
 */

#include "platform_macos_types.h"
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <wctype.h>

static __thread DWORD tls_last_error = 0;

DWORD GetTickCount(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (DWORD)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void Sleep(DWORD dwMilliseconds)
{
    usleep(dwMilliseconds * 1000);
}

DWORD GetLastError(void)
{
    return tls_last_error;
}

void SetLastError(DWORD dwErrCode)
{
    tls_last_error = dwErrCode;
}

DWORD GetCurrentThreadId(void)
{
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return (DWORD)tid;
}

DWORD GetCurrentProcessId(void)
{
    return (DWORD)getpid();
}

void* dlopen_wide(const wchar_t* name)
{
    if (!name) return NULL;

    /* Convert wide string to UTF-8 for dlopen */
    size_t len = wcslen(name);
    size_t buf_size = len * 4 + 1;
    char* utf8 = (char*)malloc(buf_size);
    if (!utf8) return NULL;

    wcstombs(utf8, name, buf_size);
    void* handle = dlopen(utf8, RTLD_LAZY);
    free(utf8);
    return handle;
}
