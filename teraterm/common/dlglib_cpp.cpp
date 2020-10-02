/*
 * (C) 2005-2020 TeraTerm Project
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

/* Routines for dialog boxes */

#include <windows.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <wchar.h>

#include "dlglib.h"
#include "ttlib.h"
#include "layer_for_unicode.h"
#include "codeconv.h"

/**
 *	EndDialog() 互換関数
 */
BOOL TTEndDialog(HWND hDlgWnd, INT_PTR nResult)
{
	return EndDialog(hDlgWnd, nResult);
}

/**
 *	CreateDialogIndirectParam() 互換関数
 */
HWND TTCreateDialogIndirectParam(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,			// オーナーウィンドウのハンドル
	DLGPROC lpDialogFunc,		// ダイアログボックスプロシージャへのポインタ
	LPARAM lParamInit)			// 初期化値
{
	DLGTEMPLATE *lpTemplate = TTGetDlgTemplate(hInstance, lpTemplateName);
	HWND hDlgWnd = _CreateDialogIndirectParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
	free(lpTemplate);
	return hDlgWnd;
}

/**
 *	CreateDialogParam() 互換関数
 */
HWND TTCreateDialogParam(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,
	DLGPROC lpDialogFunc,
	LPARAM lParamInit)
{
	return TTCreateDialogIndirectParam(hInstance, lpTemplateName,
									   hWndParent, lpDialogFunc, lParamInit);
}

/**
 *	CreateDialog() 互換関数
 */
HWND TTCreateDialog(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,
	DLGPROC lpDialogFunc)
{
	return TTCreateDialogParam(hInstance, lpTemplateName,
							   hWndParent, lpDialogFunc, NULL);
}

/**
 *	DialogBoxParam() 互換関数
 *		EndDialog()ではなく、TTEndDialog()を使用すること
 */
INT_PTR TTDialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplateName,
						 HWND hWndParent,		// オーナーウィンドウのハンドル
						 DLGPROC lpDialogFunc,  // ダイアログボックスプロシージャへのポインタ
						 LPARAM lParamInit)		// 初期化値
{
	DLGTEMPLATE *lpTemplate = TTGetDlgTemplate(hInstance, lpTemplateName);
	INT_PTR DlgResult = _DialogBoxIndirectParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
	free(lpTemplate);
	return DlgResult;
}

/**
 *	DialogBox() 互換関数
 *		EndDialog()ではなく、TTEndDialog()を使用すること
 */
INT_PTR TTDialogBox(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc)
{
	return TTDialogBoxParam(hInstance, lpTemplateName, hWndParent, lpDialogFunc, NULL);
}

/**
 *	ダイアログフォントを設定する
 */
void SetDialogFont(const char *FontName, int FontPoint, int FontCharSet,
				   const char *UILanguageFile, const char *Section, const char *Key)
{
	LOGFONTA logfont;
	BOOL result;

	// 指定フォントをセット
	if (FontName != NULL && FontName[0] != 0) {
		// 存在チェック
		result = IsExistFontA(FontName, FontCharSet, TRUE);
		if (result == TRUE) {
			TTSetDlgFontA(FontName, FontPoint, FontCharSet);
			return;
		}
	}

	// .lngの指定
	if (UILanguageFile != NULL && Section != NULL && Key != NULL) {
		result = GetI18nLogfont(Section, Key, &logfont, 0, UILanguageFile);
		if (result == TRUE) {
			if (IsExistFontA(logfont.lfFaceName, logfont.lfCharSet, TRUE)) {
				TTSetDlgFontA(logfont.lfFaceName, logfont.lfHeight, logfont.lfCharSet);
				return;
			}
		}
	}

	// ini,lngで指定されたフォントが見つからなかったとき、
	// messagebox()のフォントをとりあえず選択しておく
	GetMessageboxFont(&logfont);
	if (logfont.lfHeight < 0) {
		logfont.lfHeight = GetFontPointFromPixel(NULL, -logfont.lfHeight);
	}
	TTSetDlgFontA(logfont.lfFaceName, logfont.lfHeight, logfont.lfCharSet);
}


/**
 *	pixel数をpoint数に変換する(フォント用)
 *		注 1point = 1/72 inch, フォントの単位
 *		注 ウィンドウの表示具合で倍率が変化するので hWnd が必要
 */
int GetFontPixelFromPoint(HWND hWnd, int pixel)
{
	if (hWnd == NULL) {
		hWnd = GetDesktopWindow();
	}
	HDC DC = GetDC(hWnd);
	int dpi = GetDeviceCaps(DC, LOGPIXELSY);	// dpi = dot per inch (96DPI)
	int point = MulDiv(pixel, dpi, 72);			// pixel = point / 72 * dpi
	ReleaseDC(hWnd, DC);
	return point;
}

/**
 *	point数をpixel数に変換する(フォント用)
 *		注 1point = 1/72 inch, フォントの単位
 */
int GetFontPointFromPixel(HWND hWnd, int point)
{
	HDC DC = GetDC(hWnd);
	int dpi = GetDeviceCaps(DC, LOGPIXELSY);	// dpi = dot per inch (96DPI)
	int pixel = MulDiv(point, 72, dpi);			// point = pixel / dpi * 72
	ReleaseDC(hWnd, DC);
	return pixel;
}

/**
 *	リストの横幅を拡張する(元の幅より狭くなることはない)
 *	@param[in]	dlg		ダイアログのハンドル
 *	@param[in]	ID		コンボボックスのID
 */
void ExpandCBWidth(HWND dlg, int ID)
{
	HWND hCtrlWnd = GetDlgItem(dlg, ID);
	int count = (int)SendMessage(hCtrlWnd, CB_GETCOUNT, 0, 0);
	HFONT hFont = (HFONT)SendMessage(hCtrlWnd, WM_GETFONT, 0, 0);
	int i, max_width = 0;
	HDC TmpDC = GetDC(hCtrlWnd);
	hFont = (HFONT)SelectObject(TmpDC, hFont);
	for (i=0; i<count; i++) {
		SIZE s;
		int len = (int)SendMessage(hCtrlWnd, CB_GETLBTEXTLEN, i, 0);
		char *lbl = (char *)calloc(len+1, sizeof(char));
		SendMessage(hCtrlWnd, CB_GETLBTEXT, i, (LPARAM)lbl);
		GetTextExtentPoint32(TmpDC, lbl, len, &s);
		if (s.cx > max_width)
			max_width = s.cx;
		free(lbl);
	}
	max_width += GetSystemMetrics(SM_CXVSCROLL);	// スクロールバーの幅も足し込んでおく
	SendMessage(hCtrlWnd, CB_SETDROPPEDWIDTH, max_width, 0);
	SelectObject(TmpDC, hFont);
	ReleaseDC(hCtrlWnd, TmpDC);
}

/**
 *	GetOpenFileName(), GetSaveFileName() 用フィルタ文字列取得
 *
 *	@param[in]	user_filter_mask	ユーザーフィルタ文字列
 *									"*.txt", "*.txt;*.log" など
 *									NULLのとき使用しない
 *	@param[in]	UILanguageFile
 *	@param[out]	len					生成した文字列長(wchar_t単位)
 *									NULLのときは返さない
 *	@retval		"User define(*.txt)\0*.txt\0All(*.*)\0*.*\0" など
 *				終端は "\0\0" となる
 */
wchar_t *GetCommonDialogFilterW(const char *user_filter_mask, const char *UILanguageFile, size_t *len)
{
	// "ユーザ定義(*.txt)\0*.txt"
	wchar_t *user_filter_str = NULL;
	size_t user_filter_len = 0;
	if (user_filter_mask != NULL && user_filter_mask[0] != 0) {
		wchar_t user_filter_name[MAX_UIMSG];
		GetI18nStrW("Tera Term", "FILEDLG_USER_FILTER_NAME", user_filter_name, sizeof(user_filter_name), L"User define",
					 UILanguageFile);
		size_t user_filter_name_len = wcslen(user_filter_name);
		wchar_t *user_filter_maskW = ToWcharA(user_filter_mask);
		size_t user_filter_mask_len = wcslen(user_filter_maskW);
		user_filter_len = user_filter_name_len + 1 + user_filter_mask_len + 1 + 1 + user_filter_mask_len + 1;
		user_filter_str = (wchar_t *)malloc(user_filter_len * sizeof(wchar_t));
		wchar_t *p = user_filter_str;
		wmemcpy(p, user_filter_name, user_filter_name_len);
		p += user_filter_name_len;
		*p++ = '(';
		wmemcpy(p, user_filter_maskW, user_filter_mask_len);
		p += user_filter_mask_len;
		*p++ = ')';
		*p++ = '\0';
		wmemcpy(p, user_filter_maskW, user_filter_mask_len);
		p += user_filter_mask_len;
		*p++ = '\0';
		free(user_filter_maskW);
	}

	// "すべてのファイル(*.*)\0*.*"
	wchar_t all_filter_str[MAX_UIMSG];
	GetI18nStrW("Tera Term", "FILEDLG_ALL_FILTER", all_filter_str, _countof(all_filter_str), L"All(*.*)\\0*.*", UILanguageFile);
	size_t all_filter_len;
	{
		size_t all_filter_title_len = wcsnlen(all_filter_str, _countof(all_filter_str));
		if (all_filter_title_len == 0 || all_filter_title_len == _countof(all_filter_str)) {
			all_filter_str[0] = 0;
			all_filter_len = 0;
		} else {
			size_t all_filter_mask_max = _countof(all_filter_str) - all_filter_title_len - 1;
			size_t all_filter_mask_len = wcsnlen(all_filter_str + all_filter_title_len + 1, all_filter_mask_max);
			if (all_filter_mask_len == 0 || all_filter_mask_len == _countof(all_filter_str)) {
				all_filter_str[0] = 0;
				all_filter_len = 0;
			} else {
				all_filter_len = all_filter_title_len + 1 + all_filter_mask_len + 1;
			}
		}
	}

	// フィルタ文字列を作る
	size_t filter_len = user_filter_len + all_filter_len;
	wchar_t* filter_str;
	if (filter_len != 0) {
		filter_len++;
		filter_str = (wchar_t*)malloc(filter_len * sizeof(wchar_t));
		wchar_t *p = filter_str;
		if (user_filter_len != 0) {
			wmemcpy(p, user_filter_str, user_filter_len);
			p += user_filter_len;
		}
		wmemcpy(p, all_filter_str, all_filter_len);
		p += all_filter_len;
		*p = '\0';
	} else {
		filter_len = 2;
		filter_str = (wchar_t*)malloc(filter_len * sizeof(wchar_t));
		filter_str[0] = 0;
		filter_str[1] = 0;
	}

	if (user_filter_len != 0) {
		free(user_filter_str);
	}

	if (len != NULL) {
		*len = filter_len;
	}

	return filter_str;
}

wchar_t *GetCommonDialogFilterW(const char *user_filter_mask, const char *UILanguageFile)
{
	return GetCommonDialogFilterW(user_filter_mask, UILanguageFile, NULL);
}

char *GetCommonDialogFilterA(const char *user_filter_mask, const char *UILanguageFile)
{
	size_t filterW_len;
	wchar_t *filterW_ptr = GetCommonDialogFilterW(user_filter_mask, UILanguageFile, &filterW_len);
	char *filterA_ptr = _WideCharToMultiByte(filterW_ptr, filterW_len, CP_ACP, NULL);
	free(filterW_ptr);
	return filterA_ptr;
}
