/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004-2017 TeraTerm Project
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

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500	// 2000
//#define _WIN32_WINNT 0x0501	// XP
#define _WIN32_IE _WIN32_WINNT

/* TTCMN.DLL, main */
#include <string.h>
#include <windows.h>
#include <tchar.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "ttcommon.h"

// Notify Icon ŠÖ˜A
static HICON CustomIcon = NULL;

typedef struct {
	NOTIFYICONDATA notify_icon;
	int NotifyIconShowCount;
//	HICON CustomIcon;
} CommonNotifyData;

static CommonNotifyData *GetNotifyData(PComVar cv)
{
	CommonNotifyData *p = cv->CmnNotifyData;
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
	if (cv->CmnNotifyData != NULL) {
		return;
	} else {
#if 0
		CommonNotifyData *CmnNotifyData = malloc(sizeof(CommonNotifyData));
#else
		static CommonNotifyData CmnNotifyDataPool;
		CommonNotifyData *CmnNotifyData = &CmnNotifyDataPool;
#endif
		NOTIFYICONDATA *notify_icon;
		memset(CmnNotifyData, 0, sizeof(CmnNotifyData));
		cv->CmnNotifyData = CmnNotifyData;

		notify_icon = &CmnNotifyData->notify_icon;
		notify_icon->cbSize = sizeof(*notify_icon);
		notify_icon->hWnd = cv->HWin;
		notify_icon->uID = 1;
		notify_icon->uFlags = NIF_ICON | NIF_MESSAGE;
		notify_icon->uCallbackMessage = WM_USER_NOTIFYICON;
		if (CustomIcon) {
			notify_icon->hIcon = CustomIcon;
		}
		else {
			notify_icon->hIcon = (HICON)SendMessage(cv->HWin, WM_GETICON, ICON_SMALL, 0);
		}
		notify_icon->szTip[0] = '\0';
		notify_icon->dwState = 0;
		notify_icon->dwStateMask = 0;
		notify_icon->szInfo[0] = '\0';
		notify_icon->uTimeout = 0;
		notify_icon->szInfoTitle[0] = '\0';
		notify_icon->dwInfoFlags = 0;

		Shell_NotifyIcon(NIM_ADD, notify_icon);

		CmnNotifyData->NotifyIconShowCount = 0;
	}
}

void WINAPI DeleteNotifyIcon(PComVar cv)
{
	CommonNotifyData *CmnNotifyData = GetNotifyData(cv);
	NOTIFYICONDATA *NotifyIcon = &CmnNotifyData->notify_icon;
	if (NotifyIcon) {
		Shell_NotifyIcon(NIM_DELETE, NotifyIcon);
		CmnNotifyData->NotifyIconShowCount = 0;
	}
}

void WINAPI ShowNotifyIcon(PComVar cv)
{
	CommonNotifyData *CmnNotifyData = GetNotifyData(cv);
	NOTIFYICONDATA *NotifyIcon;
	if (CmnNotifyData == NULL) {
		CreateNotifyIcon(cv);
		CmnNotifyData = GetNotifyData(cv);
	}

	NotifyIcon = &CmnNotifyData->notify_icon;
	NotifyIcon->uFlags = NIF_STATE;
	NotifyIcon->dwState = 0;
	NotifyIcon->dwStateMask = NIS_HIDDEN;
	Shell_NotifyIcon(NIM_MODIFY, NotifyIcon);
	CmnNotifyData->NotifyIconShowCount += 1;
}

void WINAPI HideNotifyIcon(PComVar cv)
{
	CommonNotifyData *CmnNotifyData = GetNotifyData(cv);
	if (CmnNotifyData->NotifyIconShowCount > 1) {
		CmnNotifyData->NotifyIconShowCount -= 1;
	}
	else {
		NOTIFYICONDATA *NotifyIcon = &CmnNotifyData->notify_icon;
		if (NotifyIcon) {
			NotifyIcon->uFlags = NIF_STATE;
			NotifyIcon->dwState = NIS_HIDDEN;
			NotifyIcon->dwStateMask = NIS_HIDDEN;
			Shell_NotifyIcon(NIM_MODIFY, NotifyIcon);
		}
		CmnNotifyData->NotifyIconShowCount = 0;
	}
}

void WINAPI SetVerNotifyIcon(PComVar cv, unsigned int ver)
{
	CommonNotifyData *CmnNotifyData = GetNotifyData(cv);
	NOTIFYICONDATA *NotifyIcon = &CmnNotifyData->notify_icon;
	if (NotifyIcon) {
		NotifyIcon->uVersion = ver;
		Shell_NotifyIcon(NIM_SETVERSION, NotifyIcon);
	}
}

void WINAPI NotifyMessage(PComVar cv, const TCHAR *msg, const TCHAR *title, DWORD flag)
{
	CommonNotifyData *CmnNotifyData;
	NOTIFYICONDATA *NotifyIcon;
	if (msg == NULL) {
		return;
	}

	if (! HasBalloonTipSupport()) {
		return;
	}

	CmnNotifyData = GetNotifyData(cv);
	if (CmnNotifyData == NULL) {
		CreateNotifyIcon(cv);
		CmnNotifyData = GetNotifyData(cv);
	}

	NotifyIcon = &CmnNotifyData->notify_icon;
	NotifyIcon->uFlags = NIF_INFO | NIF_STATE;
	NotifyIcon->dwState = 0;
	NotifyIcon->dwStateMask = NIS_HIDDEN;

	if (title) {
		NotifyIcon->dwInfoFlags = flag;
		_tcsncpy_s(NotifyIcon->szInfoTitle, _countof(NotifyIcon->szInfoTitle), title, _TRUNCATE);
	}
	else {
		NotifyIcon->dwInfoFlags = NIIF_NONE;
		NotifyIcon->szInfoTitle[0] = 0;
	}

	_tcsncpy_s(NotifyIcon->szInfo, _countof(NotifyIcon->szInfo), msg, _TRUNCATE);

	Shell_NotifyIcon(NIM_MODIFY, NotifyIcon);

	CmnNotifyData->NotifyIconShowCount += 1;
}
