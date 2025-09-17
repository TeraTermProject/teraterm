/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2008- TeraTerm Project
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
#include "dlglib.h"

#include "i18n.h"
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <commctrl.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <wchar.h>
#include "ttlib.h"	// for get_lang_font()
#include "codeconv.h"

void EnableDlgItem(HWND HDlg, int FirstId, int LastId)
{
	int i;
	HWND HControl;

	for (i = FirstId ; i <= LastId ; i++) {
		HControl = GetDlgItem(HDlg, i);
		EnableWindow(HControl,TRUE);
	}
}

void DisableDlgItem(HWND HDlg, int FirstId, int LastId)
{
	int i;
	HWND HControl;

	for (i = FirstId ; i <= LastId ; i++) {
		HControl = GetDlgItem(HDlg, i);
		EnableWindow(HControl,FALSE);
	}
}

void ShowDlgItem(HWND HDlg, int FirstId, int LastId)
{
	int i;
	HWND HControl;

	for (i = FirstId ; i <= LastId ; i++) {
		HControl = GetDlgItem(HDlg, i);
		ShowWindow(HControl,SW_SHOW);
	}
}

void SetRB(HWND HDlg, int R, int FirstId, int LastId)
{
	HWND HControl;
	DWORD Style;

	if ( R<1 ) {
		return;
	}
	if ( FirstId+R-1 > LastId ) {
		return;
	}
	HControl = GetDlgItem(HDlg, FirstId + R - 1);
	SendMessage(HControl, BM_SETCHECK, 1, 0);
	Style = (DWORD)GetClassLongPtr(HControl, GCL_STYLE);
	SetClassLongPtr(HControl, GCL_STYLE, Style | WS_TABSTOP);
}

void GetRB(HWND HDlg, LPWORD R, int FirstId, int LastId)
{
	int i;

	*R = 0;
	for (i = FirstId ; i <= LastId ; i++) {
		if (SendDlgItemMessage(HDlg, i, BM_GETCHECK, 0, 0) != 0) {
			*R = (WORD)(i - FirstId + 1);
			return;
		}
	}
}

void SetDlgNum(HWND HDlg, int id_Item, LONG Num)
{
	wchar_t Temp[16];

	/* In Win16, SetDlgItemInt can not be used to display long integer. */
	_snwprintf_s(Temp, _countof(Temp), _TRUNCATE, L"%d", Num);
	SetDlgItemTextW(HDlg,id_Item,Temp);
}

void InitDlgProgress(HWND HDlg, int id_Progress, int *CurProgStat) {
	HWND HProg;
	HProg = GetDlgItem(HDlg, id_Progress);

	*CurProgStat = 0;

	SendMessage(HProg, PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, 100));
	SendMessage(HProg, PBM_SETSTEP, (WPARAM)1, 0);
	SendMessage(HProg, PBM_SETPOS, (WPARAM)0, 0);

	ShowWindow(HProg, SW_SHOW);
	return;
}

void SetDlgPercent(HWND HDlg, int id_Item, int id_Progress, LONG a, LONG b, int *p)
{
	// 20MB以上のファイルをアップロードしようとすると、buffer overflowで
	// 落ちる問題への対処。(2005.3.18 yutaka)
	// cf. http://sourceforge.jp/tracker/index.php?func=detail&aid=5713&group_id=1412&atid=5333
	double Num;
	wchar_t NumStr[10];

	if (b==0) {
		Num = 100.0;
	}
	else {
		Num = 100.0 * (double)a / (double)b;
	}
	_snwprintf_s(NumStr, _countof(NumStr),_TRUNCATE,L"%3.1f%%",Num);
	SetDlgItemTextW(HDlg, id_Item, NumStr);

	if (id_Progress != 0 && p != NULL && *p >= 0 && (double)*p < Num) {
		*p = (int)Num;
		SendMessage(GetDlgItem(HDlg, id_Progress), PBM_SETPOS, (WPARAM)*p, 0);
	}
}

void SetDlgTime(HWND HDlg, int id_Item, DWORD stime, int bytes)
{
	static int prev_elapsed;
	int elapsed, rate;
	wchar_t buff[64];

	elapsed = (GetTickCount() - stime) / 1000;

	if (elapsed == 0) {
		prev_elapsed = 0;
		SetDlgItemTextW(HDlg, id_Item, L"0:00");
		return;
	}

	if (elapsed == prev_elapsed) {
		return;
	}
	prev_elapsed = elapsed;

	rate = bytes / elapsed;
	if (rate < 1200) {
		_snwprintf_s(buff, _countof(buff), _TRUNCATE, L"%d:%02d (%dBytes/s)", elapsed / 60, elapsed % 60, rate);
	}
	else if (rate < 1200000) {
		_snwprintf_s(buff, _countof(buff), _TRUNCATE, L"%d:%02d (%d.%02dKB/s)", elapsed / 60, elapsed % 60, rate / 1000, rate / 10 % 100);
	}
	else {
		_snwprintf_s(buff, _countof(buff), _TRUNCATE, L"%d:%02d (%d.%02dMB/s)", elapsed / 60, elapsed % 60, rate / (1000 * 1000), rate / 10000 % 100);
	}

	SetDlgItemTextW(HDlg, id_Item, buff);
}

void SetDropDownList(HWND HDlg, int Id_Item, const char *List[], int nsel)
{
	int i;

	i = 0;
	while (List[i] != NULL) {
		SendDlgItemMessageA(HDlg, Id_Item, CB_ADDSTRING,
							0, (LPARAM)List[i]);
		i++;
	}
	SendDlgItemMessage(HDlg, Id_Item, CB_SETCURSEL,nsel-1,0);
}

void SetDropDownListW(HWND HDlg, int Id_Item, const wchar_t *List[], int nsel)
{
	int i;

	i = 0;
	while (List[i] != NULL) {
		SendDlgItemMessageW(HDlg, Id_Item, CB_ADDSTRING,
							0, (LPARAM)List[i]);
		i++;
	}
	SendDlgItemMessage(HDlg, Id_Item, CB_SETCURSEL,nsel-1,0);
}

LONG GetCurSel(HWND HDlg, int Id_Item)
{
	LRESULT n;

	n = SendDlgItemMessage(HDlg, Id_Item, CB_GETCURSEL, 0, 0);
	if (n==CB_ERR) {
		n = 0;
	}
	else {
		n++;
	}

	return (LONG)n;
}

typedef struct {
	WNDPROC OrigProc;	// Original window procedure
	LONG_PTR OrigUser;	// DWLP_USER
	BOOL IsComboBox;
} EditSubclassData;

/**
 *	サポートするキー
 *	C-n/C-p		コンボボックスのとき1つ上/下を選択
 *	C-b/C-f		カーソルを1文字前/後ろ
 *	C-a/C-e		カーソルを行頭/行末
 *	C-d			カーソル配下の1文字削除
 *	C-k			カーソルから行末まで削除
 *	C-u			カーソルより行頭まで削除
 */
static LRESULT CALLBACK HostnameEditProc(HWND dlg, UINT msg,
                                         WPARAM wParam, LPARAM lParam)
{
	EditSubclassData *data = (EditSubclassData *)GetWindowLongPtr(dlg, GWLP_USERDATA);
	LRESULT Result;
	DWORD select;
	DWORD max;
	DWORD len;

	switch (msg) {
		// キーが押されたのを検知する
		case WM_KEYDOWN:
			if (GetKeyState(VK_CONTROL) < 0) {
				switch (wParam) {
					case 0x50: // Ctrl+p ... up
						if (data->IsComboBox) {
							HWND parent = GetParent(dlg);
							select = (DWORD)SendMessageW(parent, CB_GETCURSEL, 0, 0);
							if (select == CB_ERR) {
								max = (DWORD)SendMessageW(parent, CB_GETCOUNT, 0, 0);
								PostMessageW(parent, CB_SETCURSEL, max - 1, 0);
							}
							else if (select > 0) {
								PostMessageW(parent, CB_SETCURSEL, select - 1, 0);
							}
							return 0;
						}
						break;
					case 0x4e: // Ctrl+n ... down
						if (data->IsComboBox) {
							HWND parent = GetParent(dlg);
							max = (DWORD)SendMessageW(parent, CB_GETCOUNT, 0, 0);
							select = (DWORD)SendMessageW(parent, CB_GETCURSEL, 0, 0);
							if (select == CB_ERR) {
								PostMessageW(parent, CB_SETCURSEL, 0, 0);
							} else if (select < max - 1) {
								PostMessageW(parent, CB_SETCURSEL, select + 1, 0);
							}
							return 0;
						}
						break;
					case 0x42: {
						// Ctrl+b ... left
						SendMessageW(dlg, EM_GETSEL, 0, (LPARAM)&select);
						if (select > 0) {
							wchar_t *str;
							max = GetWindowTextLengthW(dlg);
							max++; // '\0'
							str = (wchar_t *)malloc(sizeof(wchar_t) * max);
							if (str == NULL) {
								return 0;
							}
							GetWindowTextW(dlg, str, (int)max);
							select = select - 1;
							if (IsLowSurrogate(str[select])) {
								select = select - 1;
							}
							PostMessageW(dlg, EM_SETSEL, select, select);
							free(str);
						}
						return 0;
					}
					case 0x46: {
						// Ctrl+f ... right
						SendMessageW(dlg, EM_GETSEL, 0, (LPARAM)&select);
						max = GetWindowTextLengthW(dlg);
						if (select < max) {
							wchar_t *str;
							max++; // '\0'
							str = (wchar_t *)malloc(sizeof(wchar_t) * max);
							if (str == NULL) {
								return 0;
							}
							GetWindowTextW(dlg, str, (int)max);
							select = select + 1;
							if (IsLowSurrogate(str[select])) {
								select = select + 1;
							}
							PostMessageW(dlg, EM_SETSEL, select, select);
							free(str);
						}
						return 0;
					}
					case 0x41: // Ctrl+a ... home
						PostMessageW(dlg, EM_SETSEL, 0, 0);
						return 0;
					case 0x45: // Ctrl+e ... end
						max = GetWindowTextLengthW(dlg) ;
						PostMessageW(dlg, EM_SETSEL, max, max);
						return 0;

					case 0x44: // Ctrl+d
					case 0x4b: // Ctrl+k
					case 0x55: {
						// Ctrl+u
						wchar_t *str, *orgstr;
						SendMessageW(dlg, EM_GETSEL, 0, (LPARAM)&select);
						max = GetWindowTextLengthW(dlg);
						max++; // '\0'
						orgstr = str = (wchar_t *)malloc(sizeof(wchar_t) * max);
						if (str != NULL) {
							len = GetWindowTextW(dlg, str, (int)max);
							if (select < len) {
								if (wParam == 0x44) { // Ctrl+d カーソル配下の文字のみを削除する
									wmemmove(&str[select], &str[select + 1], len - select - 1);
									str[len - 1] = '\0';

								} else if (wParam == 0x4b) { // Ctrl+k カーソルから行末まで削除する
									str[select] = '\0';

								}
							}

							if (wParam == 0x55) { // Ctrl+uカーソルより左側をすべて消す
								if (select >= len) {
									str[0] = '\0';
								} else {
									str = &str[select];
								}
								select = 0;
							}

							SetWindowTextW(dlg, str);
							SendMessageW(dlg, EM_SETSEL, select, select);
							free(orgstr);
							return 0;
						}
						break;
					}
					default:
						break;
				}
			}
			break;

		// 上のキーを押した結果送られる文字で音が鳴るので捨てる
		case WM_CHAR:
			switch (wParam) {
				case 0x01:
				case 0x02:
				case 0x04:
				case 0x05:
				case 0x06:
				case 0x0b:
				case 0x0e:
				case 0x10:
				case 0x15:
					return 0;
			}
			break;
	}

	SetWindowLongPtrW(dlg, GWLP_WNDPROC, (LONG_PTR)data->OrigProc);
	SetWindowLongPtr(dlg, GWLP_USERDATA, (LONG_PTR)data->OrigUser);
	Result = CallWindowProcW(data->OrigProc, dlg, msg, wParam, lParam);
	data->OrigProc = (WNDPROC)GetWindowLongPtr(dlg, GWLP_WNDPROC);
	data->OrigUser = GetWindowLongPtr(dlg, GWLP_USERDATA);
	SetWindowLongPtrW(dlg, GWLP_WNDPROC, (LONG_PTR)HostnameEditProc);
	SetWindowLongPtr(dlg, GWLP_USERDATA, (LONG_PTR)data);

	switch (msg) {
		case WM_NCDESTROY:
			SetWindowLongPtrW(dlg, GWLP_WNDPROC, (LONG_PTR)data->OrigProc);
			SetWindowLongPtr(dlg, GWLP_USERDATA, (LONG_PTR)data->OrigUser);
			free(data);
			break;
	}

	return Result;
}

/**
 *	エディットボックス/コンボボックスのキー操作を emacs 風にする
 *		C-n/C-p のためにサブクラス化
 *	@praram		hDlg		ダイアログ
 *	@praram		nID			emacs風にするエディットボックス または コンボボックス
 */
void SetEditboxEmacsKeybind(HWND hDlg, int nID)
{
	EditSubclassData *data;
	HWND hWndEdit = GetDlgItem(hDlg, nID);
	BOOL IsCombobox = FALSE;
	char ClassName[32];
	GetClassNameA(hWndEdit, ClassName, _countof(ClassName));
	if (strcmp(ClassName, "ComboBox") == 0) {
		hWndEdit = GetWindow(hWndEdit, GW_CHILD);
		IsCombobox = TRUE;
	}
	data = (EditSubclassData *)malloc(sizeof(EditSubclassData));
	data->OrigProc = (WNDPROC)GetWindowLongPtrW(hWndEdit, GWLP_WNDPROC);
	data->OrigUser = (LONG_PTR)GetWindowLongPtr(hWndEdit, GWLP_USERDATA);
	data->IsComboBox = IsCombobox;
	SetWindowLongPtrW(hWndEdit, GWLP_WNDPROC, (LONG_PTR)HostnameEditProc);
	SetWindowLongPtr(hWndEdit, GWLP_USERDATA, (LONG_PTR)data);
}

typedef struct {
	BOOL found;
	const char *face;
	BYTE charset;
} IsExistFontInfoA;

static int CALLBACK IsExistFontSubA(
	const ENUMLOGFONTA* lpelf, const NEWTEXTMETRICA* lpntm,
	int nFontType, LPARAM lParam)
{
	IsExistFontInfoA *info = (IsExistFontInfoA *)lParam;
	(void)lpntm;
	if (nFontType != DEVICE_FONTTYPE &&
		_stricmp(lpelf->elfLogFont.lfFaceName, info->face) == 0 &&
		(info->charset == DEFAULT_CHARSET || lpelf->elfLogFont.lfCharSet == info->charset))
	{
		info->found = TRUE;
		return 0;
	}
	return 1;
}

/**
 *	IsExistFont
 *	フォントが存在しているかチェックする
 *
 *	@param[in]	face		フォント名(ファイル名ではない)
 *	@param[in]	charset		SHIFTJIS_CHARSETなど
 *	@param[in]	strict		TRUE	フォントリンクは検索に含めない
 *							FALSE	フォントリンクも検索に含める
 *	@retval		FALSE		フォントはしない
 *	@retval		TRUE		フォントは存在する
 *
 *	strict = FALSE時、存在しないフォントでも表示できるならTRUEが返る
 */
BOOL IsExistFontA(const char *face, BYTE charset, BOOL strict)
{
	HDC hDC = GetDC(NULL);
	LOGFONTA lf;
	IsExistFontInfoA info;
	memset(&lf, 0, sizeof(lf));
	lf.lfCharSet = !strict ? DEFAULT_CHARSET : charset;
	// ↑DEFAULT_CHARSETとするとフォントリンクも有効になるようだ
	lf.lfPitchAndFamily = 0;
	info.found = FALSE;
	info.face = face;
	info.charset = charset;
	EnumFontFamiliesExA(hDC, &lf, (FONTENUMPROCA)IsExistFontSubA, (LPARAM)&info, 0);
	ReleaseDC(NULL, hDC);
	return info.found;
}

typedef struct {
	BOOL found;
	const wchar_t *face;
	BYTE charset;
} IsExistFontInfoW;

static int CALLBACK IsExistFontSubW(
	const ENUMLOGFONTW* lpelf, const NEWTEXTMETRICW* lpntm,
	int nFontType, LPARAM lParam)
{
	IsExistFontInfoW *info = (IsExistFontInfoW *)lParam;
	(void)lpntm;
	if (nFontType != DEVICE_FONTTYPE &&
		_wcsicmp(lpelf->elfLogFont.lfFaceName, info->face) == 0 &&
		(info->charset == DEFAULT_CHARSET || lpelf->elfLogFont.lfCharSet == info->charset))
	{
		info->found = TRUE;
		return 0;
	}
	return 1;
}

/**
 *	フォントが存在しているかチェックする
 *
 *	@param[in]	face		フォント名(ファイル名ではない)
 *	@param[in]	charset		SHIFTJIS_CHARSETなど
 *	@param[in]	strict		TRUE	フォントリンクは検索に含めない
 *							FALSE	フォントリンクも検索に含める
 *	@retval		FALSE		フォントはしない
 *	@retval		TRUE		フォントは存在する
 *
 *	strict = FALSE時、存在しないフォントでも表示できるならTRUEが返る
 *	(charste = DEFAULT_CHARSET のときと同じ)
 *
 *	* 注
 *		- face は system locale とは異なるフォント名とはマッチしない
 *		- 理由 (EnumFontFamiliesExW() のドキュメントから)
 *		  - Englsh(ANSI) name と localized name の2つのフォント名を持っている
 *		  - localized name は system localと同じ時だけ取得できる
 *		    - EnumFontFamiliesExW() の仕様
 *		  - ハングルで指定された Gulim,Dotum は存在の確認できない
 *		- 解決案
 *		  - http://archives.miloush.net/michkap/archive/2006/02/13/530814.html
 */
BOOL IsExistFontW(const wchar_t *face, BYTE charset, BOOL strict)
{
	HDC hDC = GetDC(NULL);
	LOGFONTW lf;
	IsExistFontInfoW info;
	memset(&lf, 0, sizeof(lf));
	lf.lfCharSet = !strict ? DEFAULT_CHARSET : charset;
	// ↑DEFAULT_CHARSETとするとフォントリンクも有効になるようだ
	lf.lfPitchAndFamily = 0;
	info.found = FALSE;
	info.face = face;
	info.charset = charset;
	EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)IsExistFontSubW, (LPARAM)&info, 0);
	ReleaseDC(NULL, hDC);
	return info.found;
}
