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

#include <windows.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include <string.h>
#include <wchar.h>

#include "teraterm.h"
#include "tttypes.h"
#include "codeconv.h"
#include "compat_win.h"
#include "dlglib.h"

#undef DllExport
#define DllExport __declspec(dllexport)

#include "ttcmn_notify.h"
#include "ttcmn_notify2.h"

typedef struct NotifyIconST {
	BOOL enable;					// FALSE 通知APIが使えないOS or 使えない状態
	int NotifyIconShowCount;
	HWND parent_wnd;
	BOOL created;
	UINT callback_msg;
	BOOL no_sound;
	HINSTANCE IconInstance;
	WORD IconID;
	HINSTANCE CustomIconInstance;
	WORD CustomIconID;
	BOOL BallonDontHide;
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
	BOOL r = Shell_NotifyIconW(dwMessage, (NOTIFYICONDATAW*)lpData);
	return r;
}

static HICON LoadIcon(NotifyIcon *ni)
{
	HINSTANCE IconInstance = ni->CustomIconInstance == NULL ? ni->IconInstance : ni->CustomIconInstance;
	WORD IconID = ni->CustomIconID == 0 ? ni->IconID : ni->CustomIconID;
	const int primary_monitor_dpi = GetMonitorDpiFromWindow(NULL);
	return TTLoadIcon(IconInstance, MAKEINTRESOURCEW(IconID), 16 ,16, primary_monitor_dpi, TRUE);
}

static void InitializeNotifyIconData(NotifyIcon *ni, TT_NOTIFYICONDATAW_V2 *p)
{
	assert(ni->parent_wnd != NULL);
	memset(p, 0, sizeof(*p));
	p->cbSize = sizeof(*p);
	p->hWnd = ni->parent_wnd;
	p->uID = 1;
	p->uCallbackMessage = ni->callback_msg;
	p->uFlags = (ni->callback_msg != 0) ? NIF_MESSAGE : 0;
}

void Notify2Hide(NotifyIcon *ni)
{
	if (ni->NotifyIconShowCount > 1) {
		ni->NotifyIconShowCount -= 1;
		return;
	}

	if (ni->BallonDontHide) {
		return;
	}

	TT_NOTIFYICONDATAW_V2 notify_icon;
	InitializeNotifyIconData(ni, &notify_icon);
	notify_icon.uFlags |= NIF_STATE;
	notify_icon.dwState = NIS_HIDDEN;
	notify_icon.dwStateMask = NIS_HIDDEN;
	BOOL r = Shell_NotifyIconW(NIM_MODIFY, &notify_icon);
	ni->NotifyIconShowCount = 0;
	if (r == FALSE) {
		// タスクバーからなくなっている
		ni->created = FALSE;
	}
}

/**
 *	タスクトレイにアイコン、バルーンツールチップを表示する
 *
 *	@param	msg		表示するメッセージ
 *					NULLのときバルーンツールチップは表示されない
 *	@param	title	NULLのときタイトルなし
 *	@param	flag	NOTIFYICONDATA.dwInfoFlags
 *					1		information icon (NIIF_INFO)
 *					2		warning icon (NIIF_WARNING)
 *					3		error icon (NIIF_ERROR)
 *					0x10	(NIIF_NOSOUND) XP+
 */
void Notify2SetMessageW(NotifyIcon *ni, const wchar_t *msg, const wchar_t *title, DWORD flag)
{
	if (!ni->enable) {
		return;
	}
	if (ni->parent_wnd == NULL) {
		// まだウィンドウをセットしていない
		return;
	}
	if (title == NULL) {
		flag = NIIF_NONE;
	}

	if (ni->no_sound) {
		// shell32.dll 6.00(XP)+
		flag |= NIIF_NOSOUND;
	}

	TT_NOTIFYICONDATAW_V2 notify_icon;
	InitializeNotifyIconData(ni, &notify_icon);
	notify_icon.uFlags |= NIF_INFO;
	notify_icon.uFlags |= NIF_STATE;
	notify_icon.dwState = 0;
	notify_icon.dwStateMask = NIS_HIDDEN;
	notify_icon.uFlags |= NIF_ICON;
	notify_icon.hIcon = LoadIcon(ni);

	if (title) {
		wcsncpy_s(notify_icon.szInfoTitle, _countof(notify_icon.szInfoTitle), title, _TRUNCATE);
	}
	else {
		notify_icon.szInfoTitle[0] = 0;
	}
	notify_icon.dwInfoFlags = flag;

	if (msg) {
		wcsncpy_s(notify_icon.szInfo, _countof(notify_icon.szInfo), msg, _TRUNCATE);
	}
	else {
		notify_icon.szInfo[0] = 0;
	}

	if (ni->created) {
		BOOL r = Shell_NotifyIconW(NIM_MODIFY, &notify_icon);
		if (r == FALSE) {
			// タスクバーからなくなっている?
			ni->created = FALSE;
		}
	}

	if (!ni->created) {
		if (Shell_NotifyIconW(NIM_ADD, &notify_icon) != FALSE) {
			ni->created = TRUE;
		}
		else {
			// タスクバーがなくなっているときADDに失敗する
		}
	}

	// 通知アイコンは渡したらコピーされるので保持しなくてよい
	// すぐに破棄してok
	// https://docs.microsoft.com/ja-jp/windows/win32/shell/taskbar
	// https://stackoverflow.com/questions/23897103/how-to-properly-update-tray-notification-icon
	DestroyIcon(notify_icon.hIcon);
	if (ni->created) {
		ni->NotifyIconShowCount += 1;
	}
}

void Notify2SetIconID(NotifyIcon *ni, HINSTANCE hInstance, WORD IconID)
{
	ni->CustomIconInstance = hInstance;
	ni->CustomIconID = IconID;
}

/**
 *	通知領域初期化
 *
 *	次に Notify2SetWindow() をコールする
 */
NotifyIcon *Notify2Initialize(void)
{
	NotifyIcon *ni = (NotifyIcon *)calloc(1, sizeof(NotifyIcon));
	return ni;
}

/**
 *	親ウィンドウと通知領域のアイコンをセットする
 *	このAPIをコールすると通知領域が使えるようになる
 *
 *	セットしたアイコンがデフォルトのアイコンとなる
 *
 *	@param	hWnd		通知領域に関連付けるウィンドウ(ここでは親ウィンドウと呼ぶ)
 *	@param	msg			親ウィンドウへ送られるメッセージID
 *	@param	hInstance	アイコンを持つモジュールのinstance
 *	@param	IconID		アイコンのリソースID
 */
void Notify2SetWindow(NotifyIcon *ni, HWND hWnd, UINT msg, HINSTANCE hInstance, WORD IconID)
{
	assert(hWnd != NULL);
	if (! HasBalloonTipSupport()) {
		ni->enable = FALSE;
		return;
	}
	ni->enable = TRUE;
	ni->parent_wnd = hWnd;
	ni->callback_msg = msg;
	ni->no_sound = FALSE;
	ni->IconInstance = hInstance;
	ni->IconID = IconID;
}

/**
 *	親ウィンドウを破棄する前に通知領域のアイコンを削除する
 *	このAPIをコールすると通知領域は使えなくなる
 */
void Notify2UnsetWindow(NotifyIcon *ni)
{
	if (ni->enable && ni->created) {
		TT_NOTIFYICONDATAW_V2 notify_icon;
		InitializeNotifyIconData(ni, &notify_icon);
		Shell_NotifyIconW(NIM_DELETE, &notify_icon);	// 戻り値チェックしない
		ni->created = FALSE;
		ni->enable = FALSE;
	}
}

/**
 *	通知領域終了
 */
void Notify2Uninitialize(NotifyIcon *ni)
{
	Notify2UnsetWindow(ni);
	free(ni);
}

void Notify2SetSound(NotifyIcon *ni, BOOL sound)
{
	ni->no_sound = sound == FALSE ? TRUE : FALSE;
}

BOOL Notify2GetSound(NotifyIcon *ni)
{
	return ni->no_sound;
}

void Notify2SetBallonDontHide(NotifyIcon *ni, BOOL dont_hide)
{
	ni->BallonDontHide = dont_hide;
}

/**
 *	Notify2SetWindow() で指定したウィンドウメッセージ処理
 */
void Notify2Event(NotifyIcon *ni, WPARAM wParam, LPARAM lParam)
{
	if (wParam != 1) {
		// NOTIFYICONDATA.uID, 1しかない
		return;
	}

	switch (lParam) {
		case WM_MOUSEMOVE:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_CONTEXTMENU:
		case NIN_BALLOONSHOW:
		case NIN_BALLOONHIDE:
		case NIN_KEYSELECT:
		case NIN_SELECT:
		default:
			// nothing to do
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case NIN_BALLOONTIMEOUT:
			Notify2Hide(ni);
			break;
		case NIN_BALLOONUSERCLICK:
			Notify2Hide(ni);
			break;
	}
}

////////////////////

static NotifyIcon *GetNotifyData(PComVar cv)
{
	assert(cv != NULL);
	NotifyIcon *p = (NotifyIcon *)cv->NotifyIcon;
	assert(p != NULL);
	return p;
}

/*
 *	for plug-in APIs
 */

void WINAPI NotifyMessageW(PComVar cv, const wchar_t *msg, const wchar_t *title, DWORD flag)
{
	NotifyIcon *ni = GetNotifyData(cv);
	Notify2SetMessageW(ni, msg, title, flag);
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
 *	通知領域で使用するアイコンをセットする
 *
 *	@param	hInstance	アイコンを持つモジュールのinstance
 *	@param	IconID		アイコンのリソースID
 *
 *	各々をNULL, 0 にすると NotifySetWindow() で設定したデフォルトのアイコンに戻る
 */
void WINAPI NotifySetIconID(PComVar cv, HINSTANCE hInstance, WORD IconID)
{
	NotifyIcon *ni = GetNotifyData(cv);
	Notify2SetIconID(ni, hInstance, IconID);
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
