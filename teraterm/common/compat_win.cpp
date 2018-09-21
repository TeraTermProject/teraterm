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

#include "compat_win.h"

HINSTANCE hDll_msimg32;
HMODULE hDll_user32;

BOOL (WINAPI *pAlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPCRECT,MONITORENUMPROC,LPARAM);
DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
UINT (WINAPI *pGetDpiForWindow)(HWND hwnd);
BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

typedef struct {
	const char *ApiName;
	void **func;
} APIInfo;

typedef struct {
	const char *DllName;
	HINSTANCE *hDll;
	const APIInfo *APIInfoPtr;
	size_t APIInfoCount;
} DllInfo;

static const APIInfo Lists_user32[] = {
	{ "SetLayeredWindowAttributes", (void **)&pSetLayeredWindowAttributes },
	{ "SetThreadDpiAwarenessContext", (void **)&pSetThreadDpiAwarenessContext },
	{ "GetDpiForWindow", (void **)&pGetDpiForWindow }
};
	
static const APIInfo Lists_msimg32[] = {
	{ "AlphaBlend", (void **)&pAlphaBlend },
};
	
static const DllInfo DllInfos[] = {
	{ "user32.dll", &hDll_user32, Lists_user32, _countof(Lists_user32) },
	{ "msimg32.dll", &hDll_msimg32, Lists_msimg32, _countof(Lists_msimg32) },
};

void WinCompatInit()
{
	static BOOL done = FALSE;
	if (done) return;
	done = TRUE;

	for (size_t i = 0; i < _countof(DllInfos); i++) {
		const DllInfo *pDllInfo = &DllInfos[i];

		char dllName[MAX_PATH];
		GetSystemDirectory(dllName, sizeof(dllName));
		strcat_s(dllName, sizeof(dllName), "/");
		strcat_s(dllName, sizeof(dllName), pDllInfo->DllName);

		HINSTANCE hDll = LoadLibrary(dllName);
		*pDllInfo->hDll = hDll;

		if (hDll != NULL) {
			const APIInfo *pApiInfo = pDllInfo->APIInfoPtr;
			for (size_t j = 0; j < pDllInfo->APIInfoCount; j++) {
				void **func = pApiInfo->func;
				*func = (void *)GetProcAddress(hDll, pApiInfo->ApiName);
				pApiInfo++;
			}
		}
	}
}
