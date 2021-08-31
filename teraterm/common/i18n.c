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

#include "i18n.h"
#include "ttlib.h"
#include "codeconv.h"
#include "compat_win.h"

#include <wchar.h>
#include <assert.h>

/**
 *	GetI18nStr() の unicode版
 *	@param[in]	buf_len		文字数(\0含む)
 *	@reterm		バッファの文字数(L'\0'を含む)
 */
DllExport size_t WINAPI GetI18nStrW(const char *section, const char *key, wchar_t *buf, int buf_len, const wchar_t *def,
									const char *iniFile)
{
	wchar_t *str;
	size_t size = GetI18nStrWA(section, key, def, iniFile, &str);
	if (size <= (size_t)buf_len) {
		// sizeは L'\0' を含む
		wmemcpy(buf, str, size);
	}
	else {
		wmemcpy(buf, str, buf_len - 1);
		buf[buf_len - 1] = '\0';
		assert(("buffer too small",0));
	}
	free(str);
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
	wchar_t *strW;
	wchar_t *defW = ToWcharA(def);
	size_t size = GetI18nStrWA(section, key, defW, iniFile, &strW);
	size_t lenA;
	char *strA = _WideCharToMultiByte(strW, size, CP_ACP, &lenA);
	if ((size_t)buf_len >= lenA) {
		memcpy(buf, strA, lenA);
	}
	else {
		memcpy(buf, strA, buf_len - 1);
		buf[buf_len - 1] = '\0';
		assert(("buffer too small",0));
	}
	free(defW);
	free(strA);
	free(strW);
}

int WINAPI GetI18nLogfont(const char *section, const char *key, PLOGFONTA logfont, int ppi, const char *iniFile)
{
	return GetI18nLogfontAA(section, key, logfont, ppi, iniFile);
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
	return SetI18nDlgStrsA(hDlgWnd, section, infos, infoCount, UILanguageFile);
}

void WINAPI SetI18nMenuStrs(const char *section, HMENU hMenu, const DlgTextInfo *infos, size_t infoCount,
							const char *UILanguageFile)
{
	SetI18nMenuStrsA(hMenu, section, infos, infoCount, UILanguageFile);
}

/* vim: set ts=4 sw=4 ff=dos : */
