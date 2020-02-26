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

#include "dlglib.h"
#include "ttlib.h"
#include "layer_for_unicode.h"

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
	TTSetDlgFont(logfont.lfFaceName, logfont.lfHeight, logfont.lfCharSet);
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
