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

#include <assert.h>

#include "i18n.h"
#include "codeconv.h"
#include "compat_win.h"
#include "win32helper.h"
#include "ttlib.h"

size_t GetI18nStrWW(const char *section, const char *key, const wchar_t *def, const wchar_t *iniFile, wchar_t **str)
{
	wchar_t sectionW[64];
	wchar_t keyW[128];
	DWORD error;
	size_t size;
	MultiByteToWideChar(CP_ACP, 0, section, -1, sectionW, _countof(sectionW));
	MultiByteToWideChar(CP_ACP, 0, key, -1, keyW, _countof(keyW));
	error = hGetPrivateProfileStringW(sectionW, keyW, def, iniFile, str);
	if (error != NO_ERROR) {
		*str = NULL;
		return 0;
	}
	if (*str == NULL) {
		return 0;
	}
	size = RestoreNewLineW(*str);
	return size;
}

size_t GetI18nStrWA(const char *section, const char *key, const wchar_t *def, const char *iniFile, wchar_t **buf)
{
	wchar_t *iniFileW = ToWcharA(iniFile);
	size_t size = GetI18nStrWW(section, key, def, iniFileW, buf);
	free(iniFileW);
	return size;
}

wchar_t *TTGetLangStrW(const char *section, const char *key, const wchar_t *def, const char *UILanguageFile)
{
	wchar_t *str;
	GetI18nStrWA(section, key, def, UILanguageFile, &str);
	return str;
}

size_t GetI18nStrU8W(const char *section, const char *key, const char *def, const wchar_t *iniFile, char **buf)
{
	wchar_t *defW = ToWcharU8(def);
	wchar_t *strW;
	size_t size = GetI18nStrWW(section, key, defW, iniFile, &strW);
	*buf = ToU8W(strW);
	free(defW);
	free(strW);
	return size;
}

size_t GetI18nStrU8A(const char *section, const char *key, const char *def, const char *iniFile, char **buf)
{
	wchar_t *iniFileW  = ToWcharA(iniFile);
	size_t size = GetI18nStrU8W(section, key, def, iniFileW, buf);
	free(iniFileW);
	return size;
}

void GetI18nStrU8(const char *section, const char *key, char *buf, int buf_len, const char *def, const char *iniFile)
{
	char *str;
	GetI18nStrU8A(section, key, def, iniFile, &str);
	strncpy_s(buf, buf_len, str, _TRUNCATE);
	free(str);
}

/**
 *	リストを設定する
 *	SetDropDownList() の他言語版
 *
 *	@param[in]	section			UILanguageFile のセクション名
 *	@param[in]	hDlg			ダイアログ
 *	@param[in]	nIDDlgItem		id
 *	@param[in]	I18nTextInfo	テキスト情報
 *	@param[in]	infoCount		テキスト情報数
 *	@param[in]	UILanguageFile	lng file
 *	@param[in]	nsel			CB_SETCURSEL の引数と同じ
 *								-1	未選択
 *								0～	選択項目
 */
void SetI18nListW(const char *section, HWND hDlg, int nIDDlgItem, const I18nTextInfo *infos, size_t infoCount,
				  const wchar_t *UILanguageFile, int nsel)
{
	UINT ADDSTRING;
	UINT SETCURSEL;
	size_t i;
	char ClassName[32];
	int r = GetClassNameA(GetDlgItem(hDlg, nIDDlgItem), ClassName, _countof(ClassName));
	assert(r != 0);
	(void)r;

	if (strcmp(ClassName, "ListBox") == 0) {
		ADDSTRING = LB_ADDSTRING;
		SETCURSEL = LB_SETCURSEL;
	}
	else {
		// "ComboBox"
		ADDSTRING = CB_ADDSTRING;
		SETCURSEL = CB_SETCURSEL;
	}

	if (infoCount == 0) {
		// 0 のときは、終端を探す
		i = 0;
		while (infos[i].key != NULL && infos[i].default_text != NULL) {
			i++;
		}
		infoCount = i;
	}

	for (i = 0; i < infoCount; i++) {
		if (infos->key != NULL) {
			wchar_t *uimsg;
			GetI18nStrWW(section, infos->key, infos->default_text, UILanguageFile, &uimsg);
			SendDlgItemMessageW(hDlg, nIDDlgItem, ADDSTRING, 0, (LPARAM)uimsg);
			free(uimsg);
		}
		else {
			SendDlgItemMessageW(hDlg, nIDDlgItem, ADDSTRING, 0, (LPARAM)infos->default_text);
		}
		infos++;
	}
	SendDlgItemMessageA(hDlg, nIDDlgItem, SETCURSEL, nsel, 0);
}

void SetI18nList(const char *section, HWND hDlg, int nIDDlgItem, const I18nTextInfo *infos, size_t infoCount,
				 const char *UILanguageFile, int nsel)
{
	wchar_t *UILanguageFileW = ToWcharA(UILanguageFile);
	SetI18nListW(section, hDlg, nIDDlgItem, infos, infoCount, UILanguageFileW, nsel);
	free(UILanguageFileW);
}

/**
 * 言語ファイルからDialogのコンポーネントの文字列を変換する
 *
 *	@return 言語ファイルで変換できた回数(infoCount以下の数になる)
 */
int SetI18nDlgStrsW(HWND hDlgWnd, const char *section, const DlgTextInfo *infos, size_t infoCount,
					const wchar_t *UILanguageFile)
{
	size_t i;
	int translatedCount = 0;

	assert(hDlgWnd != NULL);
	assert(infoCount > 0);
	for (i = 0 ; i < infoCount; i++) {
		wchar_t *uimsg;
		GetI18nStrWW(section, infos[i].key, NULL, UILanguageFile, &uimsg);
		if (uimsg != NULL) {
			BOOL r = FALSE;
			const int nIDDlgItem = infos[i].nIDDlgItem;
			if (nIDDlgItem == 0) {
				r = SetWindowTextW(hDlgWnd, uimsg);
				assert(r != 0);
			} else {
				r = SetDlgItemTextW(hDlgWnd, nIDDlgItem, uimsg);
				assert(r != 0);
			}
			free(uimsg);
			if (r)
				translatedCount++;
		}
	}

	return translatedCount;
}

int SetI18nDlgStrsA(HWND hDlgWnd, const char *section, const DlgTextInfo *infos, size_t infoCount,
					const char *UILanguageFile)
{
	wchar_t *UILanguageFileW = ToWcharA(UILanguageFile);
	int r = SetI18nDlgStrsW(hDlgWnd, section, infos, infoCount, UILanguageFileW);
	free(UILanguageFileW);
	return r;
}

void SetI18nMenuStrsW(HMENU hMenu, const char *section, const DlgTextInfo *infos, size_t infoCount,
					  const wchar_t *UILanguageFile)
{
	const int id_position_threshold = 1000;
	size_t i;
	for (i = 0; i < infoCount; i++) {
		const int nIDDlgItem = infos[i].nIDDlgItem;
		const char *key = infos[i].key;
		// UNICODE
		wchar_t *uimsg;
		GetI18nStrWW(section, key, NULL, UILanguageFile, &uimsg);
		if (uimsg != NULL) {
			UINT uFlags = (nIDDlgItem < id_position_threshold) ? MF_BYPOSITION : MF_BYCOMMAND;
			ModifyMenuW(hMenu, nIDDlgItem, uFlags, nIDDlgItem, uimsg);
			free(uimsg);
		}
		else {
			if (nIDDlgItem < id_position_threshold) {
				// 一度ModifyMenu()しておかないとメニューの位置がずれる
				wchar_t s[MAX_UIMSG];
				GetMenuStringW(hMenu, nIDDlgItem, s, _countof(s), MF_BYPOSITION);
				ModifyMenuW(hMenu, nIDDlgItem, MF_BYPOSITION, nIDDlgItem, s);
			}
		}
	}
}

void SetI18nMenuStrsA(HMENU hMenu, const char *section, const DlgTextInfo *infos, size_t infoCount,
					  const char *UILanguageFile)
{
	wchar_t *UILanguageFileW = ToWcharA(UILanguageFile);
	SetI18nMenuStrsW(hMenu, section, infos, infoCount, UILanguageFileW);
	free(UILanguageFileW);
}

int GetI18nLogfontAW(const char *section, const char *key, PLOGFONTA logfont, int ppi, const wchar_t *iniFile)
{
	wchar_t sectionW[64];
	wchar_t keyW[128];
	wchar_t tmpW[MAX_UIMSG];
	char *tmp;
	char font[LF_FACESIZE];
	int height, charset;
	assert(iniFile[0] != '\0');
	memset(logfont, 0, sizeof(*logfont));

	MultiByteToWideChar(CP_ACP, 0, section, -1, sectionW, _countof(sectionW));
	MultiByteToWideChar(CP_ACP, 0, key, -1, keyW, _countof(keyW));
	GetPrivateProfileStringW(sectionW, keyW, L"", tmpW, MAX_UIMSG, iniFile);
	if (tmpW[0] == L'\0') {
		return FALSE;
	}
	tmp = ToCharW(tmpW);

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

	free(tmp);
	return TRUE;
}

int GetI18nLogfontAA(const char *section, const char *key, PLOGFONTA logfont, int ppi, const char *iniFile)
{
	wchar_t *iniFileW = ToWcharA(iniFile);
	int r = GetI18nLogfontAW(section, key, logfont, ppi, iniFileW);
	free(iniFileW);
	return r;
}

/* vim: set ts=4 sw=4 ff=dos : */
