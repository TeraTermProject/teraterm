/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2020 TeraTerm Project
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
#include <mbctype.h>	// for _ismbblead
#include <assert.h>

#include "teraterm_conf.h"
#include "teraterm.h"
#include "tttypes.h"
#include "compat_win.h"

#include "../teraterm/unicode_test.h"


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

int SetDlgTexts(HWND hDlgWnd, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile)
{
	return SetI18nDlgStrs("Tera Term", hDlgWnd, infos, infoCount, UILanguageFile);
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
 *	ウィンドウをディスプレイからはみ出さないように移動する
 *	はみ出ていない場合は移動しない
 *
 *	@param[in]	hWnd		位置を調整するウィンドウ
 */
void MoveWindowToDisplay(HWND hWnd)
{
	RECT desktop;
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
 *	@retval	DPI値(通常のDPIは96)
 */
int GetMonitorDpiFromWindow(HWND hWnd)
{
	if (pGetDpiForMonitor == NULL) {
		// ダイアログ内では自動スケーリングが効いているので
		// 常に96を返すようだ
		int dpiY;
		HDC hDC = GetDC(hWnd);
		dpiY = GetDeviceCaps(hDC,LOGPIXELSY);
		ReleaseDC(hWnd, hDC);
		return dpiY;
	} else {
		UINT dpiX;
		UINT dpiY;
		HMONITOR hMonitor = pMonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		pGetDpiForMonitor(hMonitor, 0 /*0=MDT_EFFECTIVE_DPI*/, &dpiX, &dpiY);
		return (int)dpiY;
	}
}

/* vim: set ts=4 sw=4 ff=dos : */
