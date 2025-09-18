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

#pragma once

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(_Printf_format_string_)
// 定義されていないときは何もしないように定義しておく
#define _Printf_format_string_
#endif

#if defined(_MSC_VER)
int asprintf(char **strp, _Printf_format_string_ const char *fmt, ...);
int aswprintf(wchar_t **strp, _Printf_format_string_ const wchar_t *fmt, ...);
#elif defined(__GNUC__)
int asprintf(char **strp, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
int aswprintf(wchar_t **strp, const wchar_t *fmt, ...); // __attribute__ ((format (wprintf, 2, 3)));
#else
int asprintf(char **strp, const char *fmt, ...);
int aswprintf(wchar_t **strp, const wchar_t *fmt, ...);
#endif
int vasprintf(char **strp, const char *fmt, va_list ap);
int vaswprintf(wchar_t **strp, const wchar_t *fmt, va_list ap);

void awcscat(wchar_t **dest, const wchar_t *add);
void awcscats(wchar_t **dest, const wchar_t *add, ...);

#ifdef __cplusplus
}
#endif
