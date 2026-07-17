/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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

/* misc. routines  */

#include <sys/stat.h>
#include <sys/utime.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <shlobj.h>
#include <ctype.h>
#include <assert.h>

#include "compat_win.h"

#include "ttlib.h"

/**
 *	ウィンドウ上の位置を取得する
 *	@Param[in]		hWnd
 *	@Param[in]		point		位置(x,y)
 *	@Param[in,out]	InWindow	ウィンドウ上
 *	@Param[in,out]	InClient	クライアント領域上
 *	@Param[in,out]	InTitleBar	タイトルバー上
 *	@retval			FALSE		無効なhWnd
 */
BOOL GetPositionOnWindow(
	HWND hWnd, const POINT *point,
	BOOL *InWindow, BOOL *InClient, BOOL *InTitleBar)
{
	const int x = point->x;
	const int y = point->y;
	RECT winRect;
	RECT clientRect;

	if (InWindow != NULL) *InWindow = FALSE;
	if (InClient != NULL) *InClient = FALSE;
	if (InTitleBar != NULL) *InTitleBar = FALSE;

	if (!GetWindowRect(hWnd, &winRect)) {
		return FALSE;
	}

	if ((x < winRect.left) || (winRect.right < x) ||
		(y < winRect.top) || (winRect.bottom < y))
	{
		return TRUE;
	}
	if (InWindow != NULL) *InWindow = TRUE;

	{
		POINT pos;
		GetClientRect(hWnd, &clientRect);
		pos.x = clientRect.left;
		pos.y = clientRect.top;
		ClientToScreen(hWnd, &pos);
		clientRect.left = pos.x;
		clientRect.top = pos.y;

		pos.x = clientRect.right;
		pos.y = clientRect.bottom;
		ClientToScreen(hWnd, &pos);
		clientRect.right = pos.x;
		clientRect.bottom = pos.y;
	}

	if ((clientRect.left <= x) && (x < clientRect.right) &&
		(clientRect.top <= y) && (y < clientRect.bottom))
	{
		if (InClient != NULL) *InClient = TRUE;
		if (InTitleBar != NULL) *InTitleBar = FALSE;
		return TRUE;
	}
	if (InClient != NULL) *InClient = FALSE;

	if (InTitleBar != NULL) {
		*InTitleBar = (y < clientRect.top) ? TRUE : FALSE;
	}

	return TRUE;
}

int SetDlgTextsW(HWND hDlgWnd, const DlgTextInfo *infos, int infoCount, const wchar_t *UILanguageFile)
{
	return SetI18nDlgStrsW(hDlgWnd, "Tera Term", infos, infoCount, UILanguageFile);
}

int SetDlgTexts(HWND hDlgWnd, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile)
{
	return SetI18nDlgStrsA(hDlgWnd, "Tera Term", infos, infoCount, UILanguageFile);
}

void SetDlgMenuTextsW(HMENU hMenu, const DlgTextInfo *infos, int infoCount, const wchar_t *UILanguageFile)
{
	SetI18nMenuStrsW(hMenu, "Tera Term", infos, infoCount, UILanguageFile);
}

void SetDlgMenuTexts(HMENU hMenu, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile)
{
	SetI18nMenuStrs("Tera Term", hMenu, infos, infoCount, UILanguageFile);
}

/**
 *	ウィンドウ表示されているディスプレイのデスクトップの範囲を取得する
 *	@param[in]		hWnd	ウィンドウのハンドル
 *	@param[out]		rect	デスクトップ
 */
void GetDesktopRect(HWND hWnd, RECT *rect)
{
	if (pMonitorFromWindow != NULL) {
		// マルチモニタがサポートされている場合
		MONITORINFO monitorInfo;
		HMONITOR hMonitor = pMonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		monitorInfo.cbSize = sizeof(MONITORINFO);
		pGetMonitorInfoA(hMonitor, &monitorInfo);
		*rect = monitorInfo.rcWork;
	} else {
		// マルチモニタがサポートされていない場合
		SystemParametersInfo(SPI_GETWORKAREA, 0, rect, 0);
	}
}

/**
 *	pointが存在するディスプレイのデスクトップの範囲を取得する
 */
void GetDesktopRectFromPoint(const POINT *p, RECT *rect)
{
	if (pMonitorFromPoint == NULL) {
		// NT4.0, 95 はマルチモニタAPIに非対応
		SystemParametersInfo(SPI_GETWORKAREA, 0, rect, 0);
	}
	else {
		// マルチモニタがサポートされている場合
		HMONITOR hm;
		POINT pt;
		MONITORINFO mi;

		pt.x = p->x;
		pt.y = p->y;
		hm = pMonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(MONITORINFO);
		pGetMonitorInfoA(hm, &mi);
		*rect = mi.rcWork;
	}
}

/**
 *	ウィンドウを領域からはみ出さないように移動する
 *	はみ出ていない場合は移動しない
 *
 *	@param[in]	hWnd		位置を調整するウィンドウ
 */
void MoveWindowToDisplayRect(HWND hWnd, const RECT *rect)
{
	RECT desktop = *rect;
	RECT win_rect;
	int win_width;
	int win_height;
	int win_x;
	int win_y;
	BOOL modify = FALSE;

	GetDesktopRect(hWnd, &desktop);

	GetWindowRect(hWnd, &win_rect);
	win_x = win_rect.left;
	win_y = win_rect.top;
	win_height = win_rect.bottom - win_rect.top;
	win_width = win_rect.right - win_rect.left;
	if (win_y < desktop.top) {
		win_y = desktop.top;
		modify = TRUE;
	}
	else if (win_y + win_height > desktop.bottom) {
		win_y = desktop.bottom - win_height;
		modify = TRUE;
	}
	if (win_x < desktop.left) {
		win_x = desktop.left;
		modify = TRUE;
	}
	else if (win_x + win_width > desktop.right) {
		win_x = desktop.right - win_width;
		modify = TRUE;
	}

	if (modify) {
		SetWindowPos(hWnd, NULL, win_x, win_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
}

/**
 *	ウィンドウをディスプレイからはみ出さないように移動する
 *	ディスプレイはウィンドウが属しているディスプレイ
 *	またいでいるときは面積の広い方
 *	はみ出ていない場合は移動しない
 *
 *	@param[in]	hWnd		位置を調整するウィンドウ
 */
void MoveWindowToDisplay(HWND hWnd)
{
	RECT desktop;
	GetDesktopRect(hWnd, &desktop);
	MoveWindowToDisplayRect(hWnd, &desktop);
}

/**
 *	ウィンドウをディスプレイからはみ出さないように移動する
 *	ディスプレイはpointが属しているディスプレイ
 *	はみ出ていない場合は移動しない
 *
 *	@param[in]	hWnd		位置を調整するウィンドウ
 *	@param[in]	point		位置
 */
void MoveWindowToDisplayPoint(HWND hWnd, const POINT *point)
{
	RECT desktop;
	GetDesktopRectFromPoint(point, &desktop);
	MoveWindowToDisplayRect(hWnd, &desktop);
}

/**
 *	ウィンドウをディスプレイの中央に配置する
 *
 *	@param[in]	hWnd		位置を調整するウィンドウ
 *	@param[in]	hWndParent	このウィンドウの中央に移動する
 *							(NULLの場合ディスプレイの中央)
 *
 *	hWndParentの指定がある場合
 *		hWndParentが表示状態の場合
 *			- hWndParentの中央に配置
 *			- ただし表示されているディスプレイからはみ出す場合は調整される
 *		hWndParentが非表示状態の場合
 *			- hWndが表示されているディスプレイの中央に配置される
 *	hWndParentがNULLの場合
 *		- hWndが表示されているディスプレイの中央に配置される
 */
void CenterWindow(HWND hWnd, HWND hWndParent)
{
	RECT rcWnd;
	LONG WndWidth;
	LONG WndHeight;
	int NewX;
	int NewY;
	RECT rcDesktop;
	BOOL r;

	r = GetWindowRect(hWnd, &rcWnd);
	assert(r != FALSE); (void)r;
	WndWidth = rcWnd.right - rcWnd.left;
	WndHeight = rcWnd.bottom - rcWnd.top;

	if (hWndParent == NULL || !IsWindowVisible(hWndParent) || IsIconic(hWndParent)) {
		// 親が設定されていない or 表示されていない or icon化されている 場合
		// ウィンドウの表示されているディスプレイの中央に表示する
		GetDesktopRect(hWnd, &rcDesktop);

		// デスクトップ(表示されているディスプレイ)の中央
		NewX = (rcDesktop.left + rcDesktop.right) / 2 - WndWidth / 2;
		NewY = (rcDesktop.top + rcDesktop.bottom) / 2 - WndHeight / 2;
	} else {
		RECT rcParent;
		r = GetWindowRect(hWndParent, &rcParent);
		assert(r != FALSE); (void)r;

		// hWndParentの中央
		NewX = (rcParent.left + rcParent.right) / 2 - WndWidth / 2;
		NewY = (rcParent.top + rcParent.bottom) / 2 - WndHeight / 2;

		GetDesktopRect(hWndParent, &rcDesktop);
	}

	// デスクトップからはみ出す場合、調整する
	if (NewX + WndWidth > rcDesktop.right)
		NewX = rcDesktop.right - WndWidth;
	if (NewX < rcDesktop.left)
		NewX = rcDesktop.left;

	if (NewY + WndHeight > rcDesktop.bottom)
		NewY = rcDesktop.bottom - WndHeight;
	if (NewY < rcDesktop.top)
		NewY = rcDesktop.top;

	// 移動する
	SetWindowPos(hWnd, NULL, NewX, NewY, 0, 0,
				 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/**
 *	hWndの表示されているモニタのDPIを取得する
 *	Per-monitor DPI awareness対応
 *
 *	@param	hWnd	(NULLのときPrimary monitor)
 *	@retval	DPI値(通常のDPIは96)
 */
int GetMonitorDpiFromWindow(HWND hWnd)
{
	if (pGetDpiForWindow == NULL || hWnd == NULL) {
		int dpiY;
		HDC hDC = GetDC(hWnd);
		dpiY = GetDeviceCaps(hDC,LOGPIXELSY);
		ReleaseDC(hWnd, hDC);
		return dpiY;
	}
	else {
		int dpi = 96;
		RECT r;
		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		if (GetWindowPlacement(hWnd, &wp)) {
			if (wp.showCmd == SW_MINIMIZE ||
				wp.showCmd == SW_SHOWMINIMIZED ||
				wp.showCmd == SW_SHOWMINNOACTIVE) {
				r = wp.rcNormalPosition;
			} else {
				GetWindowRect(hWnd, &r);
			}
		} else {
			GetWindowRect(hWnd, &r);
		}
		HINSTANCE hInst = (HINSTANCE)GetWindowLongW(hWnd, GWLP_HINSTANCE);
		HWND tmphWnd = CreateWindowExW(0, WC_STATICW, (LPCWSTR)NULL, 0,
							r.left, r.top, r.right - r.left, r.bottom - r.top,
							NULL, (HMENU)0x00, hInst, (LPVOID)NULL);
		if (tmphWnd) {
			dpi = pGetDpiForWindow(tmphWnd);
			DestroyWindow(tmphWnd);
		}
		return dpi;
	}
}

void OutputDebugPrintfW(const wchar_t *fmt, ...)
{
	wchar_t tmp[1024];
	va_list arg;
	va_start(arg, fmt);
	_vsnwprintf_s(tmp, _countof(tmp), _TRUNCATE, fmt, arg);
	va_end(arg);
	OutputDebugStringW(tmp);
}

/**
 *
 *	現在の時間を文字列にして返す
 *
 *	@param	format		strftime likeなフォーマット
 *	@param	utc_flag	TRUE / FALSE = UTCで出力 / ローカルタイムで出力
 *	@return	経過時刻文字列
 *			不要になったらfree()すること
 */
wchar_t *ttstrftime(const wchar_t *format, BOOL utc_flag)
{
	// 現在時刻
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft); // 1601/1/1 (UTC) から 100ns単位
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;

	// 100ns単位から秒単位(time_t)に変換  1970/1/1から
	long long now_ms = (ull.QuadPart - 116444736000000000LL) / (10*1000); // nano->micro->milli
	time_t now = now_ms / 1000;

	struct tm tm;
	if (utc_flag) {
		// UTC
		gmtime_s(&tm, &now);
	} else {
		// local
		localtime_s(&tm, &now);
	}

	// 独自フォーマットで展開する
	// 未対応の指定子はそのまま出力する
	size_t format_len = wcslen(format);

	// 出力バッファ
	// 独自フォーマット展開時の最大展開率は %Y(2文字->4文字) 等で 2 倍程度。
	// 余裕を持って format 長の 4 倍 + α を確保する。
	// (それでも溢れる場合は wcsncat_s が切り詰めるためオーバーフローしない)
	size_t sizeof_strtime = format_len * 4 + 1;
	wchar_t *strtime = malloc(sizeof(wchar_t) * sizeof_strtime);
	if (strtime == NULL) {
		return NULL;
	}

	static const wchar_t week[][4] = {
		L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"
	};
	static const wchar_t month[][4] = {
		L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
		L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
	};
	size_t i;

	strtime[0] = L'\0';
	for (i = 0; i < format_len; i++) {
		if (format[i] == L'%') {
			wchar_t tmp[5];
			wchar_t c = format[i + 1];
			switch (c) {
			case 'a':
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%s", week[tm.tm_wday]);
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			// 'A' 未実装
			case 'b':
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%s", month[tm.tm_mon]);
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			// 'c' 未実装
			case 'd':
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%02d", tm.tm_mday);
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			case 'e':
				// マニュアルに記載なし
				// %Hの0埋めなし("01"ではなく"1"など)、
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%2d", tm.tm_mday);
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			case 'H':
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%02d", tm.tm_hour);
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			// 'I' 未実装
			case 'N':
				// Tera Term オリジナル実装
				// ミリ秒 3桁
				// マニュアルに記載なし
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%03d", (int)(now_ms % 1000));
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			case 'm':
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%02d", tm.tm_mon + 1);
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			case 'M':
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%02d", tm.tm_min);
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			// 'p' 未実装
			case 'S':
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%02d", tm.tm_sec);
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			// 'U' 未実装
			case 'w':
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%d", tm.tm_wday);
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			// 'W' 未実装
			// 'x' 未実装
			// 'X' 未実装
			case 'y':
				// 年 2桁
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%02d", (tm.tm_year + 1900) % 100);
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			case 'Y':
				// 年 4桁
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L"%04d", (tm.tm_year + 1900));
				wcsncat_s(strtime, sizeof_strtime, tmp, _TRUNCATE);
				i++;
				break;
			// 'z' 未実装
			// 'Z' 未実装
			case '%':
				wcsncat_s(strtime, sizeof_strtime, L"%", _TRUNCATE);
				i++;
				break;
			case '\0':
			default:
				// 未対応の指定子は '%' を残してそのまま出力する
				// (i は進めないので、次ループで指定子文字がリテラルとして連結される)
				wcsncat_s(strtime, sizeof_strtime, L"%", _TRUNCATE);
				break;
			}
		}
		else {
			const wchar_t lit[2] = { format[i], L'\0' };
			wcsncat_s(strtime, sizeof_strtime, lit, _TRUNCATE);
		}
	}

	return strtime;
}

/*
 *	現在までの経過時間を文字列にして返す
 *
 *	@return	経過時間の文字列
 *			不要になったらfree()すること
 */
wchar_t *strelapsedW(DWORD start_time)
{
	size_t sizeof_strtime = 20;
	wchar_t *strtime = malloc(sizeof(wchar_t) * sizeof_strtime);
	int days, hours, minutes, seconds, msecs;
	DWORD delta = GetTickCount() - start_time;

	msecs = delta % 1000;
	delta /= 1000;

	seconds = delta % 60;
	delta /= 60;

	minutes = delta % 60;
	delta /= 60;

	hours = delta % 24;
	days = delta / 24;

	_snwprintf_s(strtime, sizeof_strtime, _TRUNCATE,
				 L"%d %02d:%02d:%02d.%03d",
				 days, hours, minutes, seconds, msecs);

	return strtime;
}

/* vim: set ts=4 sw=4 ff=dos : */
