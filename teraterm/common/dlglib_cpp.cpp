/*
 * (C) 2005- TeraTerm Project
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
#include <assert.h>

#include "dlglib.h"
#include "ttlib.h"
#include "codeconv.h"
#include "asprintf.h"
#include "compat_win.h"
#include "win32helper.h"

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
	HWND hDlgWnd = CreateDialogIndirectParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
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
	INT_PTR DlgResult = DialogBoxIndirectParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
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
wchar_t *GetCommonDialogFilterWW(const wchar_t *user_filter_mask, const wchar_t *UILanguageFile, size_t *len)
{
	// "ユーザ定義(*.txt)\0*.txt"
	wchar_t *user_filter_str = NULL;
	size_t user_filter_len = 0;
	if (user_filter_mask != NULL && user_filter_mask[0] != 0) {
		wchar_t *user_filter_name;
		GetI18nStrWW("Tera Term", "FILEDLG_USER_FILTER_NAME", L"User define", UILanguageFile, &user_filter_name);
		size_t user_filter_name_len = wcslen(user_filter_name);
		size_t user_filter_mask_len = wcslen(user_filter_mask);
		user_filter_len = user_filter_name_len + 1 + user_filter_mask_len + 1 + 1 + user_filter_mask_len + 1;
		user_filter_str = (wchar_t *)malloc(user_filter_len * sizeof(wchar_t));
		wchar_t *p = user_filter_str;
		wmemcpy(p, user_filter_name, user_filter_name_len);
		p += user_filter_name_len;
		*p++ = '(';
		wmemcpy(p, user_filter_mask, user_filter_mask_len);
		p += user_filter_mask_len;
		*p++ = ')';
		*p++ = '\0';
		wmemcpy(p, user_filter_mask, user_filter_mask_len);
		p += user_filter_mask_len;
		*p++ = '\0';
		free(user_filter_name);
	}

	// "すべてのファイル(*.*)\0*.*"
	wchar_t *all_filter_str;
	size_t all_filter_len;
	GetI18nStrWW("Tera Term", "FILEDLG_ALL_FILTER", L"All(*.*)\\0*.*", UILanguageFile, &all_filter_str);
	{
		// check all_filter_str
		size_t all_filter_title_len = wcslen(all_filter_str);
		if (all_filter_title_len == 0) {
			free(all_filter_str);
			all_filter_str = NULL;
			all_filter_len = 0;
		} else {
			size_t all_filter_mask_len = wcslen(all_filter_str + all_filter_title_len + 1);
			if (all_filter_mask_len == 0) {
				free(all_filter_str);
				all_filter_str = NULL;
				all_filter_len = 0;
			} else {
				// ok
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
	if (all_filter_len != 0) {
		free(all_filter_str);
	}

	if (len != NULL) {
		*len = filter_len;
	}

	return filter_str;
}

wchar_t *GetCommonDialogFilterWW(const wchar_t *user_filter_mask, const wchar_t *UILanguageFile)
{
	return GetCommonDialogFilterWW(user_filter_mask, UILanguageFile, NULL);
}

wchar_t *GetCommonDialogFilterW(const char *user_filter_mask, const char *UILanguageFile)
{
	wchar_t *UILanguageFileW = ToWcharA(UILanguageFile);
	wchar_t * user_filter_maskW = ToWcharA(user_filter_mask);
	wchar_t *ret = GetCommonDialogFilterWW(user_filter_maskW, UILanguageFileW);
	free(user_filter_maskW);
	free(UILanguageFileW);
	return ret;
}

/**
 *	アイコンをロードする
 *	@param[in]	hinst
 *	@param[in]	name
 *	@param[in]	cx	アイコンサイズ(96dpi時)
 *	@param[in]	cy	アイコンサイズ
 *	@param[in]	dpi	アイコンサイズ(cx,cy)はdpi/96倍のサイズで読み込まれる
 *	@return		HICON
 *
 *		cx == 0 && cy == 0 のときデフォルトのアイコンサイズで読み込む
 *		DestroyIcon()すること
 */
static HICON TTLoadIcon(HINSTANCE hinst, const wchar_t *name, int cx, int cy, UINT dpi)
{
	if (cx == 0 && cy == 0) {
		// 100%(96dpi?)のとき、GetSystemMetrics(SM_CXICON)=32
		if (pGetSystemMetricsForDpi != NULL) {
			cx = pGetSystemMetricsForDpi(SM_CXICON, dpi);
			cy = pGetSystemMetricsForDpi(SM_CYICON, dpi);
		}
		else {
			cx = GetSystemMetrics(SM_CXICON);
			cy = GetSystemMetrics(SM_CYICON);
		}
	}
	else {
		cx = cx * dpi / 96;
		cy = cy * dpi / 96;
	}
	HICON hIcon;
	HRESULT hr = _LoadIconWithScaleDown(hinst, name, cx, cy, &hIcon);
	if(FAILED(hr)) {
		hIcon = NULL;
	}
	return hIcon;
}

typedef struct {
	wchar_t *icon_name;
	HICON icon;
	WNDPROC prev_proc;
	int cx;
	int cy;
} IconSubclassData;

static void SetIcon(HWND hwnd, HICON icon)
{
	char class_name[32];
	int r = GetClassNameA(hwnd, class_name, _countof(class_name));
	if (strcmp(class_name, "Button") == 0) {
		SendMessage(hwnd, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)icon);
	}
	else if (strcmp(class_name, "Static") == 0) {
		SendMessage(hwnd, STM_SETICON, (WPARAM)icon, 0);
	}
	else {
		assert(("not support", FALSE));
	}
}

static LRESULT IconProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	IconSubclassData *data = (IconSubclassData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg) {
	case WM_DPICHANGED: {
		const HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
		const UINT new_dpi = LOWORD(wp);
		HICON icon = TTLoadIcon(hinst, data->icon_name, data->cx, data->cy, new_dpi);
		if (icon != NULL) {
			DestroyIcon(data->icon);
			data->icon = icon;
			SetIcon(hwnd, icon);
		}
		break;
	}
	case WM_NCDESTROY: {
		LRESULT result = CallWindowProc(data->prev_proc, hwnd, msg, wp, lp);
		DestroyIcon(data->icon);
		if (HIWORD(data->icon_name) != 0) {
			free(data->icon_name);
		}
		free(data);
		return result;
	}
	default:
		break;
	}
	return CallWindowProc(data->prev_proc, hwnd, msg, wp, lp);
}

/**
 *	ダイアログのコントロールにアイコンをセットする
 *
 *	@param	dlg		ダイアログ
 *	@param	nID		コントロールID
 *	@param	name	アイコン
 *	@param	cx		アイコンサイズ
 *	@param	cy		アイコンサイズ
 *
 *	cx == 0 && cy == 0 のときデフォルトのアイコンサイズで読み込む
 *
 *	セットする例
 *		SetDlgItemIcon(Dialog, IDC_TT_ICON, MAKEINTRESOURCEW(IDI_TTERM), 0, 0);
 *	DPIが変化したときにアイコンのサイズを変更する例
 *		case WM_DPICHANGED:
 *			SendDlgItemMessage(Dialog, IDC_TT_ICON, WM_DPICHANGED, wParam, lParam);
 */
void SetDlgItemIcon(HWND dlg, int nID, const wchar_t *name, int cx, int cy)
{
	IconSubclassData *data = (IconSubclassData *)malloc(sizeof(IconSubclassData));
	data->icon_name = (HIWORD(name) == 0) ? (wchar_t *)name : _wcsdup(name);
	data->cx = cx;
	data->cy = cy;

	const HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(dlg, GWLP_HINSTANCE);
	const UINT dpi = GetMonitorDpiFromWindow(dlg);
	data->icon = TTLoadIcon(hinst, name, cx, cy, dpi);

	const HWND hwnd = GetDlgItem(dlg, nID);
	SetIcon(hwnd, data->icon);

	data->prev_proc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)IconProc);
	SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)data);
}

/**
 *	接続したホスト履歴をコンボボックスにセットする
 */
void SetComboBoxHostHistory(HWND dlg, int dlg_item, int maxhostlist, const wchar_t *SetupFNW)
{
	int i = 1;
	do {
		wchar_t EntNameW[128];
		wchar_t *TempHostW;
		_snwprintf_s(EntNameW, _countof(EntNameW), _TRUNCATE, L"host%d", i);
		hGetPrivateProfileStringW(L"Hosts", EntNameW, L"", SetupFNW, &TempHostW);
		if (TempHostW[0] != 0) {
			SendDlgItemMessageW(dlg, dlg_item, CB_ADDSTRING,
								0, (LPARAM) TempHostW);
		}
		free(TempHostW);
		i++;
	} while (i <= maxhostlist);
}
