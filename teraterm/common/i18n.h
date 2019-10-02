/*
 * Copyright (C) 2006-2019 TeraTerm Project
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

#ifndef __I18N_H
#define __I18N_H

#include <windows.h>

#define MAX_UIMSG	1024

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DllExport)
#define DllExport __declspec(dllexport)
#endif

typedef struct {
	int nIDDlgItem;
	const char *key;
} DlgTextInfo;

DllExport void WINAPI GetI18nStrW(const char *section, const char *key, wchar_t *buf, int buf_len, const wchar_t *def, const char *iniFile);
DllExport void WINAPI GetI18nStr(const char *section, const char *key, PCHAR buf, int buf_len, const char *def, const char *iniFile);
DllExport int WINAPI GetI18nLogfont(const char *section, const char *key, PLOGFONTA logfont, int ppi, const char *iniFile);
DllExport int WINAPI SetI18DlgStrs(const char *section, HWND hDlgWnd,
							 const DlgTextInfo *infos, size_t infoCount, const char *UILanguageFile);
DllExport void WINAPI SetI18MenuStrs(const char *section, HMENU hMenu,
							  const DlgTextInfo *infos, size_t infoCount, const char *UILanguageFile);

#if defined(_UNICODE)
#define	GetI18nStrT(p1, p2, p3, p4, p5, p6) GetI18nStrW(p1, p2, p3, p4, p5, p6)
#else
#define	GetI18nStrT(p1, p2, p3, p4, p5, p6) GetI18nStr(p1, p2, p3, p4, p5, p6)
#endif

#ifdef __cplusplus
}
#endif

#endif
