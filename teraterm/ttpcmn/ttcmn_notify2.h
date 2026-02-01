/*
 * Copyright (C) 2022- TeraTerm Project
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

// ttermpro.exe 内で使用する通知関連のAPI
// プラグインから使用するAPIは ttcmn_notify.h を参照

#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DllExport)
#define DllExport __declspec(dllimport)
#endif

typedef struct NotifyIconST NotifyIcon;

DllExport NotifyIcon *Notify2Initialize(void);
DllExport void Notify2Uninitialize(NotifyIcon *ni);
DllExport void Notify2SetWindow(NotifyIcon *ni, HWND hWnd, UINT msg, HINSTANCE hInstance, WORD IconID);
DllExport void Notify2Hide(NotifyIcon *ni);
DllExport void Notify2SetMessageW(NotifyIcon *ni, const wchar_t *msg, const wchar_t *title, DWORD flag);
DllExport void Notify2SetIconID(NotifyIcon *ni, HINSTANCE hInstance, WORD IconID);
DllExport void Notify2UnsetWindow(NotifyIcon *ni);
DllExport void Notify2SetSound(NotifyIcon *ni, BOOL sound);
DllExport BOOL Notify2GetSound(NotifyIcon *ni);
DllExport void Notify2Event(NotifyIcon *ni, WPARAM wParam, LPARAM lParam);
DllExport void Notify2SetBallonDontHide(NotifyIcon *ni, BOOL dont_hide);

#ifdef __cplusplus
}
#endif
