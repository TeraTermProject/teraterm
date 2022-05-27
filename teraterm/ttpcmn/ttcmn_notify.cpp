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
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "codeconv.h"
#include "compat_win.h"
#include "dlglib.h"

#define	TTCMN_NOTIFY_INTERNAL 1
#include "ttcmn_notify.h"

typedef struct {
	BOOL enable;
	int NotifyIconShowCount;
	HWND parent_wnd;
	BOOL created;
	UINT callback_msg;
	BOOL no_sound;
	HINSTANCE IconInstance;
	WORD IconID;
	HINSTANCE CustomIconInstance;
	WORD CustomIconID;
} NotifyIcon;

// https://nyaruru.hatenablog.com/entry/20071008/p3

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
	BOOL r = Shell_NotifyIconW(dwMessage, (NOTIFYICONDATAW*)lpData);
#if _DEBUG
	DWORD e = GetLastError();
	assert(r != FALSE && e == 0);
#endif
	return r;
}

static HICON LoadIcon(NotifyIcon *ni)
{
	HINSTANCE IconInstance = ni->CustomIconInstance == NULL ? ni->IconInstance : ni->CustomIconInstance;
	WORD IconID = ni->CustomIconID == 0 ? ni->IconID : ni->CustomIconID;
	const int primary_monitor_dpi = GetMonitorDpiFromWindow(NULL);
	//const int primary_monitor_dpi = 96;
	return TTLoadIcon(IconInstance, MAKEINTRESOURCEW(IconID), 16 ,16, primary_monitor_dpi, TRUE);
}

static void CreateNotifyIconData(NotifyIcon *ni, TT_NOTIFYICONDATAW_V2 *p)
{
	assert(ni->parent_wnd != NULL);
	memset(p, 0, sizeof(*p));
	p->cbSize = sizeof(*p);
	p->hWnd = ni->parent_wnd;
	p->uID = 1;
	p->uCallbackMessage = ni->callback_msg;
	p->hIcon = LoadIcon(ni);

	p->uFlags =
		((p->uCallbackMessage != 0) ? NIF_MESSAGE : 0) |
		((p->hIcon != NULL) ? NIF_ICON : 0);
}

static void NotifyHide(NotifyIcon *ni)
{
	if (ni->NotifyIconShowCount > 1) {
		ni->NotifyIconShowCount -= 1;
	}
	else {
		TT_NOTIFYICONDATAW_V2 notify_icon;
		CreateNotifyIconData(ni, &notify_icon);
		notify_icon.uFlags |= NIF_STATE;
		notify_icon.dwState = NIS_HIDDEN;
		notify_icon.dwStateMask = NIS_HIDDEN;
		Shell_NotifyIconW(NIM_MODIFY, &notify_icon);
		ni->NotifyIconShowCount = 0;
		DestroyIcon(notify_icon.hIcon);
	}
}

/**
 *
 *	@param	flag	NOTIFYICONDATA.dwInfoFlags
 *					1	information icon
 *					2	warning icon
 *					3	error icon
 */
static void NotifySetMessageW(NotifyIcon *ni, const wchar_t *msg, const wchar_t *title, DWORD flag)
{
	if (msg == NULL) {
		return;
	}
	if (title == NULL) {
		flag = NIIF_NONE;
	}

	if (ni->no_sound) {
		flag |= NIIF_NOSOUND;
	}

	TT_NOTIFYICONDATAW_V2 notify_icon;
	CreateNotifyIconData(ni, &notify_icon);
	notify_icon.uFlags |= NIF_INFO;
	notify_icon.uFlags |= NIF_STATE;
	notify_icon.dwState = 0;
	notify_icon.dwStateMask = NIS_HIDDEN;

	if (title) {
		wcsncpy_s(notify_icon.szInfoTitle, _countof(notify_icon.szInfoTitle), title, _TRUNCATE);
	}
	else {
		notify_icon.szInfoTitle[0] = 0;
	}
	notify_icon.dwInfoFlags = flag;

	wcsncpy_s(notify_icon.szInfo, _countof(notify_icon.szInfo), msg, _TRUNCATE);

	if (!ni->created) {
		if (Shell_NotifyIconW(NIM_ADD, &notify_icon) != FALSE) {
			ni->created = TRUE;
		}
	}
	else {
		Shell_NotifyIconW(NIM_MODIFY, &notify_icon);
	}

	// 通知アイコンは渡したらコピーされるので保持しなくてよい
	// すぐに破棄してok
	// https://docs.microsoft.com/ja-jp/windows/win32/shell/taskbar
	// https://stackoverflow.com/questions/23897103/how-to-properly-update-tray-notification-icon
	DestroyIcon(notify_icon.hIcon);
	ni->NotifyIconShowCount += 1;
}

static void NotifySetIconID(NotifyIcon *ni, HINSTANCE hInstance, WORD IconID)
{
	ni->CustomIconInstance = hInstance;
	ni->CustomIconID = IconID;
}

static NotifyIcon *NotifyInitialize(HWND hWnd, UINT msg, HINSTANCE hInstance, WORD IconID)
{
	NotifyIcon *ni = (NotifyIcon *)calloc(sizeof(NotifyIcon) ,1);
	assert(hWnd != NULL);
	if (! HasBalloonTipSupport()) {
		ni->enable = FALSE;
		return ni;
	}
	ni->enable = TRUE;
	ni->parent_wnd = hWnd;
	ni->callback_msg = msg;
	ni->no_sound = FALSE;
	ni->IconInstance = hInstance;
	ni->IconID = IconID;
	return ni;
}

static void NotifyUninitialize(NotifyIcon *ni)
{
	if (ni->enable && ni->created) {
		TT_NOTIFYICONDATAW_V2 notify_icon;
		CreateNotifyIconData(ni, &notify_icon);
		Shell_NotifyIconW(NIM_DELETE, &notify_icon);
		DestroyIcon(notify_icon.hIcon);
	}
	free(ni);
}

static NotifyIcon *GetNotifyData(PComVar cv)
{
	assert(cv != NULL);
	NotifyIcon *p = (NotifyIcon *)cv->NotifyIcon;
	assert(p != NULL);
	return p;
}

/*
 *	EXPORT API
 */
void WINAPI HideNotifyIcon(PComVar cv)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifyHide(ni);
}

void WINAPI NotifyMessageW(PComVar cv, const wchar_t *msg, const wchar_t *title, DWORD flag)
{
	NotifyIcon *ni = GetNotifyData(cv);
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

/**
 *	通知領域初期化
 *
 *	@param	hWnd		通知領域の親ウィンドウ
 *	@param	msg			親ウィンドウへ送られるメッセージID
 *	@param	hInstance	アイコンを持つモジュールのinstance
 *	@param	IconID		アイコンのリソースID
 */
void WINAPI NotifyInitialize(PComVar cv, HWND hWnd, UINT msg, HINSTANCE hInstance, WORD IconID)
{
	assert (cv->NotifyIcon == NULL);	// 2重初期化
	NotifyIcon *ni = NotifyInitialize(hWnd, msg, hInstance, IconID);
	cv->NotifyIcon = ni;
}

/**
 *	通知領域完了
 */
void WINAPI NotifyUninitialize(PComVar cv)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifyUninitialize(ni);
	cv->NotifyIcon = NULL;
}

/**
 *	通知領域で使用するアイコンをセットする
 *
 *	@param	hInstance	アイコンを持つモジュールのinstance
 *	@param	IconID		アイコンのリソースID
 *
 *	各々をNULL, 0 にすると NotifyInitialize() で設定したアイコンに戻る
 */
void WINAPI NotifySetIconID(PComVar cv, HINSTANCE hInstance, WORD IconID)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifySetIconID(ni, hInstance, IconID);
}

/**
 * Tera Termの通知領域の使い方
 * - 通知するときに、アイコンとバルーンを表示
 *   - 初めての通知時に、通知アイコン作成
 * - 通知タイムアウト
 *   - 通知アイコンを隠す
 *   - vtwin.cpp の CVTWindow::OnNotifyIcon() を参照
 * - 通知をクリック
 *   - 通知アイコンを隠す + vtwinのZオーダーを最前面に
 *   - vtwin.cpp の CVTWindow::OnNotifyIcon() を参照
 * - 終了時
 *   - アイコンを削除
 */
