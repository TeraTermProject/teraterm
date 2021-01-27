/*
 * Copyright (C) 2020- TeraTerm Project
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <wchar.h>
#include <locale.h>
#include <windows.h>
#include <stdbool.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../common/compat_win.h"

// 9x系などの古いmsvcrtが使えるようにする
// - _s() 系はセキュリティーの低い関数を呼び出す
// - W() 系関数は ANSI文字版を呼び出す
// - このファイルをリンクすると必ず置き換えられる
// TODO: msvcrtに存在するかチェックして、あればdll内の関数を使用する

int vsnprintf_s(char *buffer, size_t sizeOfBuffer, size_t count, const char *format, va_list ap)
{
	int truncate = count == _TRUNCATE ? 1 : 0;
	if (truncate) {
		count = sizeOfBuffer -1;
	}
	else if (count < sizeOfBuffer) {
		count = sizeOfBuffer;
	}
	int r = vsnprintf(buffer, count, format, ap);
	if (truncate && r == sizeOfBuffer) {
		buffer[sizeOfBuffer - 1] = 0;
		r = -1;
	}
	return r;
}

static int inner_vsnprintf_s(char *buffer, size_t sizeOfBuffer, size_t count, const char *format, va_list ap)
{
	return vsnprintf_s(buffer, sizeOfBuffer, count, format, ap);
}

int snprintf_s(char *buffer, size_t sizeOfBuffer, size_t count, const char *format, ...)
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = vsnprintf_s(buffer, sizeOfBuffer, count, format, ap);
	va_end(ap);
	return r;
}

static int inner_snprintf_s(char *buffer, size_t sizeOfBuffer, size_t count, const char *format, ...)
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = vsnprintf_s(buffer, sizeOfBuffer, count, format, ap);
	va_end(ap);
	return r;
}

/**
 *	TODO: locale無視
 */
static int inner_snprintf_s_l(char *buffer, size_t sizeOfBuffer, size_t count, const char *format, _locale_t locale, ...)
{
	int r;
	va_list ap;
	va_start(ap, locale);
	r = inner_vsnprintf_s(buffer, sizeOfBuffer, count, format, ap);
	va_end(ap);
	return r;
}

static int inner_vsnwprintf_s(wchar_t *buffer, size_t sizeOfBuffer, size_t count, const wchar_t *format, va_list ap)
{
	bool truncate = false;
	if (count == _TRUNCATE) {
		truncate = true;
		count = sizeOfBuffer;
	}
	else if (count < sizeOfBuffer) {
		count = sizeOfBuffer;
	}
	int r = _vsnwprintf(buffer, count, format, ap);
	if (r == -1) {
		// error or 切り捨て
		if (truncate) {
			buffer[sizeOfBuffer - 1] = 0;
		}
		r = -1;
	}
	else {
		// うまく書き込まれた
	}
	return r;
}

static int inner_snwprintf_s(wchar_t *buffer, size_t sizeOfBuffer, size_t count, const wchar_t *format, ...)
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = inner_vsnwprintf_s(buffer, sizeOfBuffer, count, format, ap);
	va_end(ap);
	return r;
}

int swprintf_s(wchar_t *buffer, size_t sizeOfBuffer, const wchar_t *format, ...)
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = inner_vsnwprintf_s(buffer, sizeOfBuffer, 1, format, ap);
	va_end(ap);
	return r;
}

static int inner_swprintf_s(wchar_t *buffer, size_t sizeOfBuffer, const wchar_t *format, ...)
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = inner_vsnwprintf_s(buffer, sizeOfBuffer, 1, format, ap);
	va_end(ap);
	return r;
}

__declspec(dllimport) int _vscprintf(const char *format, va_list ap)
{
	int r = vsnprintf(NULL, 0, format, ap);
	return r;
}

static int inner_vscprintf(const char *format, va_list ap)
{
	int r = vsnprintf(NULL, 0, format, ap);
	return r;
}

static errno_t inner_sscanf_s(const char *buffer, const char *format, ...)
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = vsscanf(buffer, format, ap);
	va_end(ap);
	return r;
}

static errno_t inner_strcat_s(char *strDestination, size_t numberOfElements, const char *strSource)
{
	size_t dest_len = strlen(strDestination);
	size_t src_len = strlen(strSource);

	if(dest_len + src_len + 1 > numberOfElements) {
		return EINVAL;
	}
	strcat(strDestination, strSource);
	return 0;
}

static errno_t inner_wcscat_s(wchar_t *strDestination, size_t numberOfElements, const wchar_t *strSource)
{
	size_t dest_len = wcslen(strDestination);
	size_t src_len = wcslen(strSource);

	if(dest_len + src_len + 1 > numberOfElements) {
		return EINVAL;
	}
	wcscat(strDestination, strSource);
	return 0;
}

static errno_t inner_strcpy_s(char *strDestination, size_t numberOfElements, const char *strSource)
{
	size_t src_len = strlen(strSource);

	if(src_len + 1 > numberOfElements) {
		return EINVAL;
	}
	strcpy(strDestination, strSource);
	return 0;
}

static errno_t inner_wcscpy_s(wchar_t *strDestination, size_t numberOfElements, const wchar_t *strSource)
{
	size_t src_len = wcslen(strSource);

	if(src_len + 1 > numberOfElements) {
		return EINVAL;
	}
	wcscpy(strDestination, strSource);
	return 0;
}

static errno_t inner_strncpy_s(char *strDestination, size_t numberOfElements, const char *strSource, size_t count)
{
	size_t src_len = strlen(strSource);

	if (count == _TRUNCATE) {
		size_t copy_len = numberOfElements - 1 < src_len ? numberOfElements - 1 : src_len;
		memcpy(strDestination, strSource, copy_len);
		strDestination[copy_len] = '\0';
	}
	else {
		if (numberOfElements - 1 < src_len) {
			size_t copy_len = numberOfElements - 1;
			memcpy(strDestination, strSource, copy_len);
			strDestination[copy_len] = '\0';
		}
		else {
			strcpy(strDestination, strSource);
		}
	}
	return 0;
}

static errno_t inner_wcsncpy_s(wchar_t *strDestination, size_t numberOfElements, const wchar_t *strSource, size_t count)
{
	size_t src_len = wcslen(strSource);

	if (count == _TRUNCATE) {
		size_t copy_len = numberOfElements - 1 < src_len ? numberOfElements - 1 : src_len;
		wmemcpy(strDestination, strSource, copy_len);
		strDestination[copy_len] = '\0';
	}
	else {
		if (numberOfElements - 1 < src_len) {
			size_t copy_len = numberOfElements - 1;
			wmemcpy(strDestination, strSource, copy_len);
			strDestination[copy_len] = '\0';
		}
		else {
			wcscpy(strDestination, strSource);
		}
	}
	return 0;
}

static errno_t inner_strncat_s(char *strDest, size_t numberOfElements, const char *strSource, size_t count)
{
	size_t dest_len = strlen(strDest);
	size_t left_len = numberOfElements - dest_len;

	char *d = strDest + dest_len;
	return inner_strncpy_s(d, left_len, strSource, count);
}

static errno_t inner_wcsncat_s(wchar_t *strDest, size_t numberOfElements, const wchar_t *strSource, size_t count)
{
	size_t dest_len = wcslen(strDest);
	size_t left_len = numberOfElements - dest_len;

	wchar_t *d = strDest + dest_len;
	return inner_wcsncpy_s(d, left_len, strSource, count);
}

static size_t inner_strnlen(const char *s, size_t maxlen)
{
	size_t len = 0;
	while(*s != 0 && maxlen != 0) {
		s++;
		maxlen--;
		len++;
	}
	return len;
}

static size_t inner_wcsnlen(const wchar_t *s, size_t maxlen)
{
	size_t len = 0;
	while(*s != 0 && maxlen != 0) {
		s++;
		maxlen--;
		len++;
	}
	return len;
}


static errno_t inner_fopen_s(FILE **f, const char *name, const char *mode)
{
	*f = fopen(name, mode);
	return errno;
}

static errno_t inner_freopen_s(FILE **f, const char *name, const char *mode)
{
	*f = freopen(name, mode, *f);
	return errno;
}

static errno_t inner__wfopen_s(FILE **f, const wchar_t *name, const char *mode)
{
	*f = fopen((char *)name, mode);		// TODO
	return errno;
}

static char *inner_strtok_s(char* str, const char* delimiters, char** context)
{
	(void)context;
	char *r = strtok(str, delimiters);
	return r;
}

static int inner__wstat64(const wchar_t *pathW, struct __stat64 *st)
{
	struct _stati64 st32;
	int r = _wstati64(pathW, &st32);
	if (r != 0) {
		char pathA[MAX_PATH];
		WideCharToMultiByte(CP_ACP, 0, pathW, -1, pathA, sizeof(pathA)-1, NULL, NULL);
		r = _stati64(pathA, &st32);
	}
	if (r == 0) {
		st->st_gid = st32.st_gid;
		st->st_atime = st32.st_atime;
		st->st_ctime = st32.st_ctime;
		st->st_dev = st32.st_dev;
		st->st_ino = st32.st_ino;
		st->st_mode = st32.st_mode;
		st->st_mtime = st32.st_mtime;
		st->st_nlink = st32.st_nlink;
		st->st_rdev = st32.st_rdev;
		st->st_size = st32.st_size;
		st->st_uid = st32.st_uid;
	}
	return r;
}

static errno_t inner_tmpnam_s(char * str, size_t sizeInChars)
{
	(void)sizeInChars;
	tmpnam(str);
	return 0;
}

static errno_t inner_itoa_s(int value, char * buffer, size_t size, int radix )
{
	(void)size;
	itoa(value, buffer, radix);
	return 0;
}

// テスト用関数
#if 0
static void not_implemented(void)
{
	MessageBox(NULL, "not_implemented", "tera trem msvcrt_wrapper", MB_OK | MB_ICONEXCLAMATION);
}
void *_imp___itoa_s = (void *)not_implemented;
#endif

void *_imp___vsnprintf_s = (void *)inner_vsnprintf_s;
void *_imp___snprintf_s = (void *)inner_snprintf_s;
void *_imp___snprintf_s_l = (void *)inner_snprintf_s_l;
void *_imp___vsnwprintf_s = (void *)inner_vsnwprintf_s;
void *_imp___snwprintf_s = (void *)inner_snwprintf_s;
void *_imp__swprintf_s = (void *)inner_swprintf_s;
void *_imp___vscprintf = (void *)inner_vscprintf;
void *_imp__sscanf_s = (void *)inner_sscanf_s;

void *_imp__strcat_s = (void *)inner_strcat_s;
void *_imp__wcscat_s = (void *)inner_wcscat_s;
void *_imp__strcpy_s = (void *)inner_strcpy_s;
void *_imp__wcscpy_s = (void *)inner_wcscpy_s;
void *_imp__strncat_s = (void *)inner_strncat_s;
void *_imp__wcsncat_s = (void *)inner_wcsncat_s;
void *_imp__strncpy_s = (void *)inner_strncpy_s;
void *_imp__wcsncpy_s = (void *)inner_wcsncpy_s;
void *_imp__strnlen = (void *)inner_strnlen;
void *_imp__wcsnlen = (void *)inner_wcsnlen;
void *_imp__strtok_s = (void *)inner_strtok_s;

void *_imp__fopen_s = (void *)inner_fopen_s;
void *_imp__freopen_s = (void *)inner_freopen_s;
void *_imp___wfopen_s = (void *)inner__wfopen_s;
void *_imp___wstat64 = (void *)inner__wstat64;
void *_imp__tmpnam_s = (void *)inner_tmpnam_s;

void *_imp___itoa_s = (void *)inner_itoa_s;
