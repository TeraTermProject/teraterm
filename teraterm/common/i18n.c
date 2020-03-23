/*
 * Copyright (C) 2006-2020 TeraTerm Project
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
#include "codeconv.h"
#include "compat_win.h"

#include <assert.h>

/**
 *	GetI18nStr() の unicode版
 *	@param[in]	buf_len		文字数(\0含む)
 *	@reterm		バッファの文字数(L'\0'を含む)
 */
DllExport size_t WINAPI GetI18nStrW(const char *section, const char *key, wchar_t *buf, int buf_len, const wchar_t *def,
									const char *iniFile)
{
	size_t size;
	if (pGetPrivateProfileStringW != NULL) {
		wchar_t sectionW[64];
		wchar_t keyW[128];
		wchar_t iniFileW[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, section, -1, sectionW, _countof(sectionW));
		MultiByteToWideChar(CP_ACP, 0, key, -1, keyW, _countof(keyW));
		MultiByteToWideChar(CP_ACP, 0, iniFile, -1, iniFileW, _countof(iniFileW));
		// The return value is the number of characters copied to the buffer,
		// not including the terminating null character.
		size = pGetPrivateProfileStringW(sectionW, keyW, def, buf, buf_len, iniFileW);
		if (size == 0 && def == NULL) {
			buf[0] = 0;
		}
	}
	else {
		char tmp[MAX_UIMSG];
		char defA[MAX_UIMSG];
		WideCharToMultiByte(CP_ACP, 0, def, -1, defA, _countof(defA), NULL, NULL);
		size = GetPrivateProfileStringA(section, key, defA, tmp, _countof(tmp), iniFile);
		if (size == 0 && def == NULL) {
			buf[0] = 0;
		}
		MultiByteToWideChar(CP_ACP, 0, tmp, -1, buf, buf_len);
	}
	size = RestoreNewLineW(buf);
	return size;
}

/**
 *	section/keyの文字列をbufにセットする
 *	section/keyが見つからなかった場合、
 *		defの文字列をbufにセットする
 *		defがNULLの場合buf[0] = 0となる
 *	@param	buf_len		文字数(\0含む)
 */
DllExport void WINAPI GetI18nStr(const char *section, const char *key, PCHAR buf, int buf_len, const char *def, const char *iniFile)
{
	DWORD size = GetPrivateProfileStringA(section, key, def, buf, buf_len, iniFile);
	if (size == 0 && def == NULL) {
		// GetPrivateProfileStringA()の戻り値はbufにセットした文字数(終端含まず)
		// OSのバージョンによってはdefがNULLの時、bufが未設定となることがある
		buf[0] = 0;
	}
	RestoreNewLine(buf);
}

int WINAPI GetI18nLogfont(const char *section, const char *key, PLOGFONTA logfont, int ppi, const char *iniFile)
{
	char tmp[MAX_UIMSG];
	char font[LF_FACESIZE];
	int height, charset;
	assert(iniFile[0] != '\0');
	memset(logfont, 0, sizeof(*logfont));

	GetPrivateProfileStringA(section, key, "", tmp, MAX_UIMSG, iniFile);
	if (tmp[0] == '\0') {
		return FALSE;
	}

	GetNthString(tmp, 1, LF_FACESIZE-1, font);
	GetNthNum(tmp, 2, &height);
	GetNthNum(tmp, 3, &charset);

	if (font[0] != '\0') {
		strncpy_s(logfont->lfFaceName, sizeof(logfont->lfFaceName), font, _TRUNCATE);
	}
	logfont->lfCharSet = (BYTE)charset;
	if (ppi != 0) {
		logfont->lfHeight = MulDiv(height, -ppi, 72);
	} else {
		logfont->lfHeight = height;
	}
	logfont->lfWidth = 0;

	return TRUE;
}

/*
 * 言語ファイルからDialogのコンポーネントの文字列を変換する
 *
 * [return]
 *    言語ファイルで変換できた回数(infoCount以下の数になる)
 *
 */
int WINAPI SetI18nDlgStrs(const char *section, HWND hDlgWnd,
						 const DlgTextInfo *infos, size_t infoCount, const char *UILanguageFile)
{
	size_t i;
	int translatedCount = 0;

	assert(hDlgWnd != NULL);
	assert(infoCount > 0);
	for (i = 0 ; i < infoCount; i++) {
		const char *key = infos[i].key;
		BOOL r = FALSE;
		if (pGetPrivateProfileStringW == NULL) {
			// ANSI
			char uimsg[MAX_UIMSG];
			GetI18nStr(section, key, uimsg, sizeof(uimsg), NULL, UILanguageFile);
			if (uimsg[0] != '\0') {
				const int nIDDlgItem = infos[i].nIDDlgItem;
				if (nIDDlgItem == 0) {
					r = SetWindowTextA(hDlgWnd, uimsg);
					assert(r != 0);
				} else {
					r = SetDlgItemTextA(hDlgWnd, nIDDlgItem, uimsg);
					assert(r != 0);
				}
			}
		}
		else {
			// UNICODE
			wchar_t uimsg[MAX_UIMSG];
			GetI18nStrW(section, key, uimsg, _countof(uimsg), NULL, UILanguageFile);
			if (uimsg[0] != L'\0') {
				const int nIDDlgItem = infos[i].nIDDlgItem;
				if (nIDDlgItem == 0) {
					r = pSetWindowTextW(hDlgWnd, uimsg);
					assert(r != 0);
				} else {
					r = pSetDlgItemTextW(hDlgWnd, nIDDlgItem, uimsg);
					assert(r != 0);
				}
			}
		}
		if (r)
			translatedCount++;
	}

	return (translatedCount);
}

void WINAPI SetI18nMenuStrs(const char *section, HMENU hMenu, const DlgTextInfo *infos, size_t infoCount,
						   const char *UILanguageFile)
{
	const int id_position_threshold = 1000;
	size_t i;
	for (i = 0; i < infoCount; i++) {
		const int nIDDlgItem = infos[i].nIDDlgItem;
		const char *key = infos[i].key;
		if (pGetPrivateProfileStringW == NULL) {
			// ANSI
			char uimsg[MAX_UIMSG];
			GetI18nStr(section, key, uimsg, sizeof(uimsg), NULL, UILanguageFile);
			if (uimsg[0] != '\0') {
				UINT uFlags = (nIDDlgItem < id_position_threshold) ? MF_BYPOSITION : MF_BYCOMMAND;
				ModifyMenuA(hMenu, nIDDlgItem, uFlags, nIDDlgItem, uimsg);
			}
			else {
				if (nIDDlgItem < id_position_threshold) {
					// 一度ModifyMenu()しておかないとメニューの位置がずれる
					GetMenuStringA(hMenu, nIDDlgItem, uimsg, _countof(uimsg), MF_BYPOSITION);
					ModifyMenuA(hMenu, nIDDlgItem, MF_BYPOSITION, nIDDlgItem, uimsg);
				}
			}
		}
		else {
			// UNICODE
			wchar_t uimsg[MAX_UIMSG];
			GetI18nStrW(section, key, uimsg, _countof(uimsg), NULL, UILanguageFile);
			if (uimsg[0] != '\0') {
				UINT uFlags = (nIDDlgItem < id_position_threshold) ? MF_BYPOSITION : MF_BYCOMMAND;
				pModifyMenuW(hMenu, nIDDlgItem, uFlags, nIDDlgItem, uimsg);
			}
			else {
				if (nIDDlgItem < id_position_threshold) {
					// 一度ModifyMenu()しておかないとメニューの位置がずれる
					pGetMenuStringW(hMenu, nIDDlgItem, uimsg, _countof(uimsg), MF_BYPOSITION);
					pModifyMenuW(hMenu, nIDDlgItem, MF_BYPOSITION, nIDDlgItem, uimsg);
				}
			}
		}
	}
}

/* vim: set ts=4 sw=4 ff=dos : */
