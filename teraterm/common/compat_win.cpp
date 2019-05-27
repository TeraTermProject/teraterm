/*
 * (C) 20018-2019 TeraTerm Project
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

#include <windows.h>
#include <tchar.h>
#include "compat_win.h"

#include "dllutil.h"

BOOL (WINAPI *pAlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPCRECT,MONITORENUMPROC,LPARAM);
DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
BOOL (WINAPI *pIsValidDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
UINT (WINAPI *pGetDpiForWindow)(HWND hwnd);
BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
int (WINAPI *pAddFontResourceExA)(LPCSTR name, DWORD fl, PVOID res);
int (WINAPI *pAddFontResourceExW)(LPCWSTR name, DWORD fl, PVOID res);
BOOL (WINAPI *pRemoveFontResourceExA)(LPCSTR name, DWORD fl, PVOID pdv);
BOOL (WINAPI *pRemoveFontResourceExW)(LPCWSTR name, DWORD fl, PVOID pdv);

static const APIInfo Lists_user32[] = {
	{ "SetLayeredWindowAttributes", (void **)&pSetLayeredWindowAttributes },
	{ "SetThreadDpiAwarenessContext", (void **)&pSetThreadDpiAwarenessContext },
	{ "IsValidDpiAwarenessContext", (void **)&pIsValidDpiAwarenessContext },
	{ "GetDpiForWindow", (void **)&pGetDpiForWindow },
	{ NULL },
};

static const APIInfo Lists_msimg32[] = {
	{ "AlphaBlend", (void **)&pAlphaBlend },
	{ NULL },
};

static const APIInfo Lists_gdi32[] = {
	{ "AddFontResourceExA", (void **)&pAddFontResourceExA },
	{ "RemoveFontResourceExA", (void **)&pRemoveFontResourceExA },
	{ "AddFontResourceExW", (void **)&pAddFontResourceExW },
	{ "RemoveFontResourceExW", (void **)&pRemoveFontResourceExW },
	{ NULL },
};

static const DllInfo DllInfos[] = {
	{ _T("user32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_user32 },
	{ _T("msimg32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_msimg32 },
	{ _T("gdi32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_gdi32 },
	{ NULL },
};

void WinCompatInit()
{
	static BOOL done = FALSE;
	if (done) return;
	done = TRUE;

	DLLGetApiAddressFromLists(DllInfos);
}
