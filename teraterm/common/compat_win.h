/*
 * (C) 2018 TeraTerm Project
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

/* compat_win */

/*
 * 使用している Windows SDK, Visual Studio の差をなくすためのファイル
 * windows.h などのファイルを include した後に include する
 */

#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE			((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE		((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2	((DPI_AWARENESS_CONTEXT)-4)
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#endif

#if !defined(WM_DPICHANGED)
#define WM_DPICHANGED					0x02E0
#endif
#if !defined(CF_INACTIVEFONTS)
#define CF_INACTIVEFONTS				0x02000000L
#endif
#if !defined(OPENFILENAME_SIZE_VERSION_400A)
#define OPENFILENAME_SIZE_VERSION_400A	76
#endif

extern BOOL (WINAPI *pAlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
extern BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPCRECT,MONITORENUMPROC,LPARAM);
extern DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
extern UINT (WINAPI *pGetDpiForWindow)(HWND hwnd);
extern BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
extern int (WINAPI *pAddFontResourceExA)(LPCSTR name, DWORD fl, PVOID res);
extern int (WINAPI *pAddFontResourceExW)(LPCWSTR name, DWORD fl, PVOID res);
extern BOOL (WINAPI *pRemoveFontResourceExA)(LPCSTR name, DWORD fl, PVOID pdv);
extern BOOL (WINAPI *pRemoveFontResourceExW)(LPCWSTR name, DWORD fl, PVOID pdv);

#ifdef UNICODE
#define pAddFontResourceEx		pAddFontResourceExW
#define pRemoveFontResourceEx	pRemoveFontResourceExW
#else
#define pAddFontResourceEx		pAddFontResourceExA
#define pRemoveFontResourceEx	pRemoveFontResourceExA
#endif // !UNICODE

void WinCompatInit();

#ifdef __cplusplus
}
#endif
