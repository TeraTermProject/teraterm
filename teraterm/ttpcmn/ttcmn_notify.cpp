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

/* TTCMN.DLL, notify icon */

// SDK7.0の場合、WIN32_IEが適切に定義されない
#if _MSC_VER == 1400	// VS2005の場合のみ
#if !defined(_WIN32_IE)
#define	_WIN32_IE 0x0501
//#define _WIN32_IE 0x0600
#endif
#endif

#include <string.h>
#include <windows.h>
#include <wchar.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttcommon.h"
#include "codeconv.h"
#include "compat_win.h"
#include "dlglib.h"

typedef struct {
	TT_NOTIFYICONDATAW_V2 notify_icon;
	int NotifyIconShowCount;
	HICON CustomIcon; // 未使用
} NotifyIcon;

/**
 *	Shell_NotifyIconW() wrapper
 *		- TT_NOTIFYICONDATAW_V2 は Windows 2000 以降で使用可能
 *		- タスクトレイにバルーンを出すことができるのは 2000 以降
 *			- NT4は使えない
 *			- HasBalloonTipSupport() で 2000以上としている
 *		- 必ず Unicode 版が利用可能 → 9xサポート不要?
 *			- Windows 98,ME で利用可能?
 *			- TT_NOTIFYICONDATAA_V2
 */
static BOOL Shell_NotifyIconW(DWORD dwMessage, TT_NOTIFYICONDATAW_V2 *lpData)
{
	return Shell_NotifyIconW(dwMessage, (NOTIFYICONDATAW*)lpData);
}

static NotifyIcon *NotifyCreate(HWND hWnd, HICON icon, UINT msg)
{
	NotifyIcon *ni = (NotifyIcon *)malloc(sizeof(NotifyIcon));
	memset(ni, 0, sizeof(*ni));

	TT_NOTIFYICONDATAW_V2 *p = &ni->notify_icon;
	p->cbSize = sizeof(*p);
	p->hWnd = hWnd;
	p->uID = 1;
	p->uFlags = NIF_ICON | NIF_MESSAGE;
	p->uCallbackMessage = msg;
	p->hIcon = icon;

	Shell_NotifyIconW(NIM_ADD, p);

	ni->NotifyIconShowCount = 0;

	return ni;
}

static void NotifyUpdateIcon(NotifyIcon *ni, HICON icon)
{
	TT_NOTIFYICONDATAW_V2 *NotifyIcon = &ni->notify_icon;
	NotifyIcon->uFlags = NIF_ICON;
	NotifyIcon->uID = 1;
	NotifyIcon->hIcon = icon;
	Shell_NotifyIconW(NIM_MODIFY, NotifyIcon);
}

static void NotifyDelete(NotifyIcon *ni)
{
	TT_NOTIFYICONDATAW_V2 *NotifyIcon = &ni->notify_icon;
	Shell_NotifyIconW(NIM_DELETE, NotifyIcon);
	ni->NotifyIconShowCount = 0;
	free(ni);
}

static void NotifyShowIcon(NotifyIcon *ni)
{
	TT_NOTIFYICONDATAW_V2 *NotifyIcon = &ni->notify_icon;
	NotifyIcon->uFlags = NIF_STATE;
	NotifyIcon->dwState = 0;
	NotifyIcon->dwStateMask = NIS_HIDDEN;
	Shell_NotifyIconW(NIM_MODIFY, NotifyIcon);
	ni->NotifyIconShowCount += 1;
}

static void NotifyHide(NotifyIcon *ni)
{
	if (ni->NotifyIconShowCount > 1) {
		ni->NotifyIconShowCount -= 1;
	}
	else {
		TT_NOTIFYICONDATAW_V2 *NotifyIcon = &ni->notify_icon;
		NotifyIcon->uFlags = NIF_STATE;
		NotifyIcon->dwState = NIS_HIDDEN;
		NotifyIcon->dwStateMask = NIS_HIDDEN;
		Shell_NotifyIconW(NIM_MODIFY, NotifyIcon);
		ni->NotifyIconShowCount = 0;
	}
}

static void NotifySetVersion(NotifyIcon *ni, unsigned int ver)
{
	TT_NOTIFYICONDATAW_V2 *NotifyIcon = &ni->notify_icon;
	NotifyIcon->uVersion = ver;
	Shell_NotifyIconW(NIM_SETVERSION, NotifyIcon);
}

static void NotifySetMessageW(NotifyIcon *ni, const wchar_t *msg, const wchar_t *title, DWORD flag)
{
	if (msg == NULL) {
		return;
	}

	if (! HasBalloonTipSupport()) {
		return;
	}

	TT_NOTIFYICONDATAW_V2 *NotifyIcon = &ni->notify_icon;
	NotifyIcon->uFlags = NIF_INFO | NIF_STATE;
	NotifyIcon->dwState = 0;
	NotifyIcon->dwStateMask = NIS_HIDDEN;

	if (title) {
		NotifyIcon->dwInfoFlags = flag;
		wcsncpy_s(NotifyIcon->szInfoTitle, _countof(NotifyIcon->szInfoTitle), title, _TRUNCATE);
	}
	else {
		NotifyIcon->dwInfoFlags = NIIF_NONE;
		NotifyIcon->szInfoTitle[0] = 0;
	}

	wcsncpy_s(NotifyIcon->szInfo, _countof(NotifyIcon->szInfo), msg, _TRUNCATE);

	Shell_NotifyIconW(NIM_MODIFY, NotifyIcon);

	ni->NotifyIconShowCount += 1;
}

static NotifyIcon *GetNotifyData(PComVar cv)
{
	NotifyIcon *p = (NotifyIcon *)cv->NotifyIcon;
	return p;
}

/*
 *	EXPORT API
 */

// SetCustomNotifyIcon(), GetCustomNotifyIcon() で操作できるが、内部からは使用されない
static HICON CustomIcon = NULL;

// 動作するが CustomIcon は使用されない
void WINAPI SetCustomNotifyIcon(HICON icon)
{
	CustomIcon = icon;
}

// 動作するが CustomIcon は使用されない
HICON WINAPI GetCustomNotifyIcon()
{
	return CustomIcon;
}

static HINSTANCE TTCustomIconInstance = NULL;
static WORD TTCustomIconID = 0;
static HINSTANCE CustomIconInstance = NULL;
static WORD CustomIconID = 0;

void WINAPI SetCustomNotifyIconID(HINSTANCE hInstance, WORD IconID, BOOL plugin)
{
	if (!plugin) {
		CustomIconInstance = hInstance;
		CustomIconID = IconID;

		// あとで外部からカスタム通知アイコンを消去されるときのために
		// Tera Term 本体から設定されたカスタム通知アイコンを保持しておく
		TTCustomIconInstance = hInstance;
		TTCustomIconID = IconID;
	}
	else {
		if (hInstance == NULL) {
			CustomIconInstance = TTCustomIconInstance;
		}
		else {
			CustomIconInstance = hInstance;
		}
		if (IconID == 0) {
			CustomIconID = TTCustomIconID;
		}
		else {
			CustomIconID = IconID;
		}
	}
}

void WINAPI CreateNotifyIcon(PComVar cv)
{
	NotifyIcon *ni = GetNotifyData(cv);
	HICON icon = NULL;

	// ウィンドウハンドルが必要になるので、接続中でないとアイコンは作成できない
	if (cv->HWin == NULL) {
		return;
	}

	if (CustomIconInstance != NULL && CustomIconID != 0) {
		const int dpi = GetMonitorDpiFromWindow(cv->HWin);
		icon = TTLoadIcon(CustomIconInstance, MAKEINTRESOURCEW(CustomIconID), 16 ,16, dpi, TRUE);
	}
	else {
		icon = (HICON)SendMessage(cv->HWin, WM_GETICON, ICON_SMALL, 0);
	}

	if (ni == NULL) {
		ni = NotifyCreate(cv->HWin, icon, WM_USER_NOTIFYICON);
	}
	else {
		NotifyUpdateIcon(ni, icon);
	}
	cv->NotifyIcon = ni;

	// 通知アイコンは渡したらコピーされるので保持しなくてよい
	// すぐに破棄する
	// https://docs.microsoft.com/ja-jp/windows/win32/shell/taskbar
	// https://stackoverflow.com/questions/23897103/how-to-properly-update-tray-notification-icon
	if (CustomIconInstance != NULL && CustomIconID != 0 &&
	    icon != NULL) {
		DestroyIcon(icon);
	}
}

void WINAPI DeleteNotifyIcon(PComVar cv)
{
	NotifyIcon* ni = GetNotifyData(cv);
	if (ni == NULL) {
		return;
	}
	NotifyDelete(ni);
}

void WINAPI ShowNotifyIcon(PComVar cv)
{
	NotifyIcon* ni;

	CreateNotifyIcon(cv);
	ni = GetNotifyData(cv);
	if (ni == NULL) {
		return;
	}

	NotifyShowIcon(ni);
}

void WINAPI HideNotifyIcon(PComVar cv)
{
	NotifyIcon *ni = GetNotifyData(cv);
	if (ni == NULL) {
		return;
	}
	NotifyHide(ni);
}

void WINAPI SetVerNotifyIcon(PComVar cv, unsigned int ver)
{
	NotifyIcon *ni = GetNotifyData(cv);
	if (ni == NULL) {
		return;
	}
	NotifySetVersion(ni, ver);
}

void WINAPI NotifyMessageW(PComVar cv, const wchar_t *msg, const wchar_t *title, DWORD flag)
{
	NotifyIcon *ni;

	CreateNotifyIcon(cv);
	ni = GetNotifyData(cv);
	if (ni == NULL) {
		return;
	}

	NotifySetMessageW(ni, msg, title, flag);
}

void WINAPI NotifyMessage(PComVar cv, const char *msg, const char *title, DWORD flag)
{
	wchar_t *titleW = ToWcharA(title);
	wchar_t *msgW = ToWcharA(msg);
	NotifyMessageW(cv, msgW, titleW, flag);
	free(titleW);
	free(msgW);
}
