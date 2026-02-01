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

/**
 *	lngファイル(iniファイル)から文字列を取得
 *
 *	@param[in]		section
 *	@param[in]		key			キー
 *								NULLのときは str = def が返る
 *	@param[in]		def			デフォルト文字列
 *								NULL=未指定
 *	@param[in]		iniFile		iniファイル
 *	@param[in,out]	str			取得文字列
 *								不要になったらfree()する
 *	@return						文字数
 *
 *		strには必ず文字列が返ってくるので、free() が必要
 *		(メモリがない時 str=NULL が返る)
 */
size_t GetI18nStrWW(const char *section, const char *key, const wchar_t *def, const wchar_t *iniFile, wchar_t **str)
{
	size_t size;
	if (key == NULL) {
		assert(def != NULL);
		*str = _wcsdup(def);
	}
	else {
		wchar_t sectionW[64];
		wchar_t keyW[128];
		MultiByteToWideChar(CP_ACP, 0, section, -1, sectionW, _countof(sectionW));
		MultiByteToWideChar(CP_ACP, 0, key, -1, keyW, _countof(keyW));
		hGetPrivateProfileStringW(sectionW, keyW, def, iniFile, str);
	}
	assert(*str != NULL);		// メモリがない時 NULL が返ってくる
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

wchar_t *TTGetLangStrW(const char *section, const char *key, const wchar_t *def, const wchar_t *UILanguageFile)
{
	wchar_t *str;
	GetI18nStrWW(section, key, def, UILanguageFile, &str);
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
 *	@param[in]	nsel			I18nTextInfo の data メンバがセットされていないとき
 *									CB_SETCURSEL の引数と同じ
 *										-1	未選択
 *										0〜	選択項目
 *								I18nTextInfo の data メンバがセットされているとき
 *									data の値と同じアイテムが選択される
 *									同じ値がないとき、最初の値が選択される
 */
void SetI18nListW(const char *section, HWND hDlg, int nIDDlgItem, const I18nTextInfo *infos, size_t infoCount,
				  const wchar_t *UILanguageFile, uintptr_t nsel)
{
	UINT ADDSTRING;
	UINT SETCURSEL;
	UINT SETITEMDATA;
	size_t i;
	char ClassName[32];
	int r = GetClassNameA(GetDlgItem(hDlg, nIDDlgItem), ClassName, _countof(ClassName));
	assert(r != 0);
	(void)r;
	BOOL data_available = TRUE;

	if (strcmp(ClassName, "ListBox") == 0) {
		ADDSTRING = LB_ADDSTRING;
		SETITEMDATA = LB_SETITEMDATA;
		SETCURSEL = LB_SETCURSEL;
	}
	else {
		// "ComboBox"
		ADDSTRING = CB_ADDSTRING;
		SETITEMDATA = CB_SETITEMDATA;
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

	// dataに値が入っている?
	for (i = 0; i < infoCount; i++) {
		if (infos[i].data != 0) {
			data_available = TRUE;
		}
	}

	for (i = 0; i < infoCount; i++) {
		LRESULT index;
		if (infos[i].key != NULL) {
			wchar_t *uimsg;
			GetI18nStrWW(section, infos[i].key, infos[i].default_text, UILanguageFile, &uimsg);
			index = SendDlgItemMessageW(hDlg, nIDDlgItem, ADDSTRING, 0, (LPARAM)uimsg);
			free(uimsg);
		}
		else {
			index = SendDlgItemMessageW(hDlg, nIDDlgItem, ADDSTRING, 0, (LPARAM)infos[i].default_text);
		}
		SendDlgItemMessageA(hDlg, nIDDlgItem, SETITEMDATA, index, infos[i].data);
	}

	if (data_available) {
		uintptr_t sel = 0;
		for (i = 0; i < infoCount; i++) {
			if (infos[i].data == nsel) {
				sel = i;
				break;
			}
		}
		nsel = sel;
	}

	SendDlgItemMessageA(hDlg, nIDDlgItem, SETCURSEL, (WPARAM)nsel, 0);
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
		if (uimsg[0] != 0) {
			BOOL r = FALSE;
			const int nIDDlgItem = infos[i].nIDDlgItem;
			if (nIDDlgItem == 0) {
				r = SetWindowTextW(hDlgWnd, uimsg);
				assert(r != 0);
			} else {
				r = SetDlgItemTextW(hDlgWnd, nIDDlgItem, uimsg);
				assert(r != 0);
			}
			if (r)
				translatedCount++;
		}
		free(uimsg);
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
		if (uimsg[0] != 0) {
			UINT uFlags = (nIDDlgItem < id_position_threshold) ? MF_BYPOSITION : MF_BYCOMMAND;
			ModifyMenuW(hMenu, nIDDlgItem, uFlags, nIDDlgItem, uimsg);
		}
		else {
			if (nIDDlgItem < id_position_threshold) {
				// 一度ModifyMenu()しておかないとメニューの位置がずれる
				wchar_t *s;
				hGetMenuStringW(hMenu, nIDDlgItem, MF_BYPOSITION, &s);
				ModifyMenuW(hMenu, nIDDlgItem, MF_BYPOSITION, nIDDlgItem, s);
				free(s);
			}
		}
		free(uimsg);
	}
}

void SetI18nMenuStrsA(HMENU hMenu, const char *section, const DlgTextInfo *infos, size_t infoCount,
					  const char *UILanguageFile)
{
	wchar_t *UILanguageFileW = ToWcharA(UILanguageFile);
	SetI18nMenuStrsW(hMenu, section, infos, infoCount, UILanguageFileW);
	free(UILanguageFileW);
}

/**
 *	フォント(LOGFONTW)を取得する
 *
 *	@param[in]	section			UILanguageFile のセクション名
 *	@param[in]	key				キー
 *	@param[out]	logfont			font
 *	@param[in]	ppi				Pixel Per Inch(=Dot Per Inch)
 *	@param[in]	iniFile			lng file
 *	@retval		TRUE			フォントを取得した,logfontが設定される
 *	@retval		FALSE			フォントを取得できなかった
 *
 *	TODO
 *		- 戻り値を BOOL に変更
 *		- logfont(戻り値) を引数の一番最後に変更
 */
int GetI18nLogfontW(const wchar_t *section, const wchar_t *key, PLOGFONTW logfont, int ppi, const wchar_t *iniFile)
{
	wchar_t *tmp;
	wchar_t font[LF_FACESIZE];
	int height, charset;
	assert(iniFile[0] != '\0');
	memset(logfont, 0, sizeof(*logfont));

	hGetPrivateProfileStringW(section, key, L"", iniFile, &tmp);
	if (tmp[0] == L'\0') {
		free(tmp);
		return FALSE;
	}

	GetNthStringW(tmp, 1, LF_FACESIZE-1, font);
	GetNthNumW(tmp, 2, &height);
	GetNthNumW(tmp, 3, &charset);

	if (font[0] != '\0') {
		wcsncpy_s(logfont->lfFaceName, _countof(logfont->lfFaceName), font, _TRUNCATE);
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

static void LOGFONTWA(const LOGFONTW *logfontW, LOGFONTA *logfontA)
{
	// メンバが全く同じ部分(lfFaceName以外)は単純にコピーする
	size_t copy_size = sizeof(*logfontA) - sizeof(logfontA->lfFaceName);
	memcpy(logfontA, logfontW, copy_size);

	// face name Unicode -> ANSI
	WideCharToACP_t(logfontW->lfFaceName, logfontA->lfFaceName,
					_countof(logfontA->lfFaceName));
}

int GetI18nLogfontAW(const char *section, const char *key, PLOGFONTA logfont, int ppi, const wchar_t *iniFile)
{
	LOGFONTW logfontW;
	wchar_t *sectionW = ToWcharA(section);
	wchar_t *keyW = ToWcharA(key);
	int r = GetI18nLogfontW(sectionW, keyW, &logfontW, ppi, iniFile);
	free(sectionW);
	free(keyW);
	if (r == TRUE) {
		LOGFONTWA(&logfontW, logfont);
	}
	else {
		memset(logfont, 0, sizeof(*logfont));
	}
	return r;
}

int GetI18nLogfontAA(const char *section, const char *key, PLOGFONTA logfont, int ppi, const char *iniFile)
{
	wchar_t *iniFileW = ToWcharA(iniFile);
	int r = GetI18nLogfontAW(section, key, logfont, ppi, iniFileW);
	free(iniFileW);
	return r;
}

/* vim: set ts=4 sw=4 ff=dos : */
