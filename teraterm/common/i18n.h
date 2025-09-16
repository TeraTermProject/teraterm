/*
 * Copyright (C) 2006- TeraTerm Project
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

typedef struct {
	const char *key;				// NULLの場合は常にdefault_text が使用される
	const wchar_t *default_text;	// key == NULL && default_text == NULLの場合終端
	uintptr_t data;
} I18nTextInfo;

DllExport size_t WINAPI GetI18nStrW(const char *section, const char *key, wchar_t *buf, int buf_len, const wchar_t *def, const char *iniFile);
DllExport void WINAPI GetI18nStr(const char *section, const char *key, PCHAR buf, int buf_len, const char *def, const char *iniFile);
DllExport int WINAPI GetI18nLogfont(const char *section, const char *key, PLOGFONTA logfont, int ppi, const char *iniFile);
DllExport int WINAPI SetI18nDlgStrs(const char *section, HWND hDlgWnd,
							 const DlgTextInfo *infos, size_t infoCount, const char *UILanguageFile);
DllExport void WINAPI SetI18nMenuStrs(const char *section, HMENU hMenu,
							  const DlgTextInfo *infos, size_t infoCount, const char *UILanguageFile);

// i18n_static.c
size_t GetI18nStrWW(const char *section, const char *key, const wchar_t *def, const wchar_t *iniFile, wchar_t **buf);
size_t GetI18nStrWA(const char *section, const char *key, const wchar_t *def, const char *iniFile, wchar_t **buf);
size_t GetI18nStrU8W(const char *section, const char *key, const char *def, const wchar_t *iniFile, char **buf);
size_t GetI18nStrU8A(const char *section, const char *key,  const char *def, const char *iniFile, char **buf);

void GetI18nStrU8(const char *section, const char *key, char *buf, int buf_len, const char *def, const char *iniFile);

int SetI18nDlgStrsW(HWND hDlgWnd, const char *section, const DlgTextInfo *infos, size_t infoCount,
					const wchar_t *UILanguageFile);
int SetI18nDlgStrsA(HWND hDlgWnd, const char *section, const DlgTextInfo *infos, size_t infoCount,
					const char *UILanguageFile);
void SetI18nMenuStrsW(HMENU hMenu, const char *section, const DlgTextInfo *infos, size_t infoCount,
					  const wchar_t *UILanguageFile);
void SetI18nMenuStrsA(HMENU hMenu, const char *section, const DlgTextInfo *infos, size_t infoCount,
					  const char *UILanguageFile);
void SetI18nListW(const char *section, HWND hDlg, int nIDDlgItem, const I18nTextInfo *infos, size_t infoCount,
				  const wchar_t *UILanguageFile, uintptr_t nsel);
void SetI18nList(const char *section, HWND hDlg, int nIDDlgItem, const I18nTextInfo *infos, size_t infoCount,
				 const char *UILanguageFile, int nsel);
int GetI18nLogfontAW(const char *section, const char *key, PLOGFONTA logfont, int ppi, const wchar_t *iniFile);
int GetI18nLogfontAA(const char *section, const char *key, PLOGFONTA logfont, int ppi, const char *iniFile);
int GetI18nLogfontW(const wchar_t *section, const wchar_t *key, PLOGFONTW logfont, int ppi, const wchar_t *iniFile);
wchar_t *TTGetLangStrW(const char *section, const char *key, const wchar_t *def, const wchar_t *UILanguageFile);

#ifdef __cplusplus
}
#endif

#endif
/* vim: set ts=4 sw=4 ff=dos : */
