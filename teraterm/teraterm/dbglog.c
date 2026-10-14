/*
 * (C) 2026- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>

#include "ttlib_static_dir.h"
#include "dbglog.h"

#define DBGLOG_MAX_SIZE		(5 * 1024 * 1024)

static CRITICAL_SECTION dbglog_lock;
static INIT_ONCE dbglog_once = INIT_ONCE_STATIC_INIT;
static wchar_t *dbglog_file;
static wchar_t *dbglog_old;

static BOOL CALLBACK DbgLogInit(PINIT_ONCE once, PVOID param, PVOID *ctx)
{
	(void)once;
	(void)param;
	(void)ctx;
	InitializeCriticalSection(&dbglog_lock);
	wchar_t *dir = GetLogDirW(NULL);
	if (dir != NULL) {
		size_t len = wcslen(dir) + 32;
		dbglog_file = (wchar_t *)malloc(len * sizeof(wchar_t));
		dbglog_old = (wchar_t *)malloc(len * sizeof(wchar_t));
		_snwprintf_s(dbglog_file, len, _TRUNCATE, L"%ls\\debug.log", dir);
		_snwprintf_s(dbglog_old, len, _TRUNCATE, L"%ls\\debug.old.log", dir);
		CreateDirectoryW(dir, NULL);
		free(dir);
	}
	return TRUE;
}

/**
 *	add a line to debug.log (thread safe)
 */
void DbgLog(const char *fmt, ...)
{
	InitOnceExecuteOnce(&dbglog_once, DbgLogInit, NULL, NULL);
	if (dbglog_file == NULL) {
		return;
	}

	char msg[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf_s(msg, sizeof(msg), _TRUNCATE, fmt, ap);
	va_end(ap);

	SYSTEMTIME t;
	GetLocalTime(&t);
	char line[1200];
	int len = _snprintf_s(line, sizeof(line), _TRUNCATE,
						  "%04u-%02u-%02u %02u:%02u:%02u.%03u [%5lu] %s\r\n",
						  t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
						  GetCurrentThreadId(), msg);
	if (len < 0) {
		len = (int)strlen(line);
	}

	EnterCriticalSection(&dbglog_lock);
	HANDLE h = CreateFileW(dbglog_file, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						   NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER size;
		if (GetFileSizeEx(h, &size) && size.QuadPart > DBGLOG_MAX_SIZE) {
			// keep one old file
			CloseHandle(h);
			MoveFileExW(dbglog_file, dbglog_old, MOVEFILE_REPLACE_EXISTING);
			h = CreateFileW(dbglog_file, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		if (h != INVALID_HANDLE_VALUE) {
			DWORD written;
			WriteFile(h, line, (DWORD)len, &written, NULL);
			CloseHandle(h);
		}
	}
	LeaveCriticalSection(&dbglog_lock);
}
