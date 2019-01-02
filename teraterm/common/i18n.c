/*
 * Copyright (C) 2006-2018 TeraTerm Project
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

#include "i18n.h"
#include "ttlib.h"

#include <assert.h>
#include <tchar.h>

#if defined(UNICODE)
DllExport void GetI18nStrW(const char *section, const char *key, wchar_t *buf, int buf_len, const wchar_t *def, const char *iniFile)
{
	wchar_t sectionW[64];
	wchar_t keyW[128];
	wchar_t iniFileW[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, section, -1, sectionW, _countof(sectionW));
	MultiByteToWideChar(CP_ACP, 0, key, -1, keyW, _countof(keyW));
	MultiByteToWideChar(CP_ACP, 0, iniFile, -1, iniFileW, _countof(iniFileW));
	GetPrivateProfileStringW(sectionW, keyW, def, buf, buf_len, iniFileW);
	RestoreNewLineW(buf);
}
#endif

DllExport void GetI18nStr(const char *section, const char *key, PCHAR buf, int buf_len, const char *def, const char *iniFile)
{
	GetPrivateProfileStringA(section, key, def, buf, buf_len, iniFile);
	RestoreNewLine(buf);
}

// TODO: バッファ不足時の動作
DllExport void GetI18nStrU8(const char *section, const char *key, char *buf, int buf_len, const char *def, const char *iniFile)
{
	int r;
#if defined(UNICODE)
	wchar_t tmp[MAX_UIMSG];
	wchar_t defW[MAX_UIMSG];
	r = MultiByteToWideChar(CP_UTF8, 0, def, -1, defW, _countof(defW));
	assert(r != 0);
	GetI18nStrW(section, key, tmp, _countof(tmp), defW, iniFile);
	r = WideCharToMultiByte(CP_UTF8, 0,
							tmp, -1,
							buf, buf_len,
							NULL, NULL);
	assert(r != 0);
#else
	// ANSI -> Wide -> utf8
	char strA[MAX_UIMSG];
	wchar_t strW[MAX_UIMSG];
	GetI18nStr(section, key, strA, _countof(strA), def, iniFile);
	r = MultiByteToWideChar(CP_ACP, 0, strA, -1, strW, _countof(strW));
	assert(r != 0);
	r = WideCharToMultiByte(CP_UTF8, 0,
							strW, -1,
							buf, buf_len,
							NULL, NULL);
	assert(r != 0);
#endif
}


DllExport int GetI18nLogfont(const char *section, const char *key, PLOGFONTA logfont, int ppi, const char *iniFile)
{
	static char tmp[MAX_UIMSG];
	static char font[LF_FACESIZE];
	int hight, charset;
	GetPrivateProfileStringA(section, key, "", tmp, MAX_UIMSG, iniFile);
	if (tmp[0] == '\0') {
		return FALSE;
	}

	GetNthString(tmp, 1, LF_FACESIZE-1, font);
	GetNthNum(tmp, 2, &hight);
	GetNthNum(tmp, 3, &charset);

	strncpy_s(logfont->lfFaceName, sizeof(logfont->lfFaceName), font, _TRUNCATE);
	logfont->lfCharSet = (BYTE)charset;
	logfont->lfHeight = MulDiv(hight, -ppi, 72);
	logfont->lfWidth = 0;

	return TRUE;
}

DllExport
void SetI18DlgStrs(const char *section, HWND hDlgWnd,
				   const DlgTextInfo *infos, size_t infoCount, const char *UILanguageFile)
{
	size_t i;
	assert(hDlgWnd != NULL);
	assert(infoCount > 0);
	for (i = 0 ; i < infoCount; i++) {
		const char *key = infos[i].key;
		TCHAR uimsg[MAX_UIMSG];
		GetI18nStrT(section, key, uimsg, sizeof(uimsg), _T(""), UILanguageFile);
		if (uimsg[0] != _T('\0')) {
			const int nIDDlgItem = infos[i].nIDDlgItem;
			BOOL r;
			if (nIDDlgItem == 0) {
				r = SetWindowText(hDlgWnd, uimsg);
				assert(r != 0);
			} else {
				r = SetDlgItemText(hDlgWnd, nIDDlgItem, uimsg);
				assert(r != 0);
			}
			(void)r;
		}
	}
}

DllExport
void SetI18MenuStrs(const char *section, HMENU hMenu,
					const DlgTextInfo *infos, size_t infoCount, const char *UILanguageFile)
{
	size_t i;
	for (i = 0; i < infoCount; i++) {
		const int nIDDlgItem = infos[i].nIDDlgItem;
		const char *key = infos[i].key;
		TCHAR uimsg[MAX_UIMSG];
		GetI18nStrT(section, key, uimsg, sizeof(uimsg), _T(""), UILanguageFile);
		if (uimsg[0] != '\0') {
			if (nIDDlgItem < 1000) {
				ModifyMenu(hMenu, nIDDlgItem, MF_BYPOSITION, nIDDlgItem, uimsg);
			} else {
				ModifyMenu(hMenu, nIDDlgItem, MF_BYCOMMAND, nIDDlgItem, uimsg);
			}
		}
	}
}
