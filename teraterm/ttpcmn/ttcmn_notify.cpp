/*
 * Copyright (C) 2020 TeraTerm Project
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

#define _WIN32_IE 0x0600
#include <string.h>
#include <windows.h>
#include <wchar.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "ttcommon.h"
#include "codeconv.h"
#include "layer_for_unicode.h"

typedef struct {
	TT_NOTIFYICONDATAW_V2 notify_icon;
	int NotifyIconShowCount;
	HICON CustomIcon;
} NotifyIcon;

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

	_Shell_NotifyIconW(NIM_ADD, p);

	ni->NotifyIconShowCount = 0;

	return ni;
}

static void NotifyDelete(NotifyIcon *ni)
{
	TT_NOTIFYICONDATAW_V2 *NotifyIcon = &ni->notify_icon;
	_Shell_NotifyIconW(NIM_DELETE, NotifyIcon);
	ni->NotifyIconShowCount = 0;
}

static void NotifyShowIcon(NotifyIcon *ni)
{
	TT_NOTIFYICONDATAW_V2 *NotifyIcon = &ni->notify_icon;
	NotifyIcon->uFlags = NIF_STATE;
	NotifyIcon->dwState = 0;
	NotifyIcon->dwStateMask = NIS_HIDDEN;
	_Shell_NotifyIconW(NIM_MODIFY, NotifyIcon);
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
		_Shell_NotifyIconW(NIM_MODIFY, NotifyIcon);
		ni->NotifyIconShowCount = 0;
	}
}

static void NotifySetVersion(NotifyIcon *ni, unsigned int ver)
{
	TT_NOTIFYICONDATAW_V2 *NotifyIcon = &ni->notify_icon;
	NotifyIcon->uVersion = ver;
	_Shell_NotifyIconW(NIM_SETVERSION, NotifyIcon);
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

	_Shell_NotifyIconW(NIM_MODIFY, NotifyIcon);

	ni->NotifyIconShowCount += 1;
}

/*
 *	EXPORT API
 */
static HICON CustomIcon = NULL;

static NotifyIcon *GetNotifyData(PComVar cv)
{
	NotifyIcon *p = (NotifyIcon *)cv->NotifyIcon;
	return p;
}

void WINAPI SetCustomNotifyIcon(HICON icon)
{
	CustomIcon = icon;
}

HICON WINAPI GetCustomNotifyIcon()
{
	return CustomIcon;
}

void WINAPI CreateNotifyIcon(PComVar cv)
{
	NotifyIcon *ni = GetNotifyData(cv);
	if (ni != NULL) {
		return;
	}
	HICON icon = CustomIcon;
	if (icon == NULL) {
		icon = (HICON)SendMessage(cv->HWin, WM_GETICON, ICON_SMALL, 0);
	}

	ni = NotifyCreate(cv->HWin, icon, WM_USER_NOTIFYICON);
	cv->NotifyIcon = ni;
}

void WINAPI DeleteNotifyIcon(PComVar cv)
{
	NotifyIcon* ni = GetNotifyData(cv);
	NotifyDelete(ni);
}

void WINAPI ShowNotifyIcon(PComVar cv)
{
	NotifyIcon* ni = GetNotifyData(cv);
	if (ni == NULL) {
		CreateNotifyIcon(cv);
		ni = GetNotifyData(cv);
	}

	NotifyShowIcon(ni);
}

void WINAPI HideNotifyIcon(PComVar cv)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifyHide(ni);
}

// Žg‚í‚ê‚Ä‚¢‚È‚¢
void WINAPI SetVerNotifyIcon(PComVar cv, unsigned int ver)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifySetVersion(ni, ver);
}

void WINAPI NotifyMessageW(PComVar cv, const wchar_t *msg, const wchar_t *title, DWORD flag)
{
	NotifyIcon *ni = GetNotifyData(cv);
	if (ni == NULL) {
		CreateNotifyIcon(cv);
		ni = GetNotifyData(cv);
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
