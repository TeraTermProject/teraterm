/*
 * (C) 2018-2020 TeraTerm Project
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
#include "ttlib.h"

ATOM (WINAPI *pRegisterClassW)(const WNDCLASSW *lpWndClass);
HWND(WINAPI *pCreateWindowExW)
(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
 HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HPROPSHEETPAGE (WINAPI * pCreatePropertySheetPageW)(LPCPROPSHEETPAGEW constPropSheetPagePointer);
INT_PTR (WINAPI *pPropertySheetW)(LPCPROPSHEETHEADERW constPropSheetHeaderPointer);
LRESULT (WINAPI *pSendDlgItemMessageW)(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL (WINAPI *pModifyMenuW)(HMENU hMnu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
int(WINAPI *pGetMenuStringW)(HMENU hMenu, UINT uIDItem, LPWSTR lpString, int cchMax, UINT flags);
BOOL(WINAPI *pSetWindowTextW)(HWND hWnd, LPCWSTR lpString);
DWORD (WINAPI *pGetPrivateProfileStringW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
UINT (WINAPI *pDragQueryFileW)(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);
DWORD (WINAPI *pGetFileAttributesW)(LPCWSTR lpFileName);
BOOL (WINAPI *pSetDlgItemTextW)(HWND hDlg, int nIDDlgItem, LPCWSTR lpString);
BOOL (WINAPI *pGetDlgItemTextW)(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax);
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
HRESULT (WINAPI *pGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
BOOL (WINAPI *pAdjustWindowRectEx)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
BOOL (WINAPI *pAdjustWindowRectExForDpi)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
HWND (WINAPI *pGetConsoleWindow)(void);
int (WINAPI *pMessageBoxW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
INT_PTR (WINAPI *pDialogBoxIndirectParamW)(HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);

HWND (WINAPI *pHtmlHelpW)(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);
HWND (WINAPI *pHtmlHelpA)(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);

BOOL (WINAPI *pInsertMenuW)(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
BOOL (WINAPI *pAppendMenuW)(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);

HMONITOR (WINAPI *pMonitorFromWindow)(HWND hwnd, DWORD dwFlags);
HMONITOR (WINAPI *pMonitorFromPoint)(POINT pt, DWORD dwFlags);
HMONITOR (WINAPI *pMonitorFromRect)(LPCRECT lprc, DWORD dwFlags);
BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);

/**
 *	GetConsoleWindow() と同じ動作をする
 *	 https://support.microsoft.com/ja-jp/help/124103/how-to-obtain-a-console-window-handle-hwnd
 */
static HWND WINAPI GetConsoleWindowLocal(void)
{
#define MY_BUFSIZE 1024					 // Buffer size for console window titles.
	HWND hwndFound;						 // This is what is returned to the caller.
	char pszNewWindowTitle[MY_BUFSIZE];  // Contains fabricated WindowTitle.
	char pszOldWindowTitle[MY_BUFSIZE];  // Contains original WindowTitle.

	// Fetch current window title.
	DWORD size = GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);
	if (size == 0) {
		DWORD err = GetLastError();
		if (err == ERROR_INVALID_HANDLE) {
			// コンソールが開いていない
			return NULL;
		}
	}

	// Format a "unique" NewWindowTitle.
	wsprintf(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

	// Change current window title.
	SetConsoleTitle(pszNewWindowTitle);

	// Ensure window title has been updated.
	Sleep(40);

	// Look for NewWindowTitle.
	hwndFound = FindWindow(NULL, pszNewWindowTitle);

	// Restore original window title.
	SetConsoleTitle(pszOldWindowTitle);

	return hwndFound;
}

static const APIInfo Lists_user32[] = {
	{ "SetLayeredWindowAttributes", (void **)&pSetLayeredWindowAttributes },
	{ "SetThreadDpiAwarenessContext", (void **)&pSetThreadDpiAwarenessContext },
	{ "IsValidDpiAwarenessContext", (void **)&pIsValidDpiAwarenessContext },
	{ "GetDpiForWindow", (void **)&pGetDpiForWindow },
	{ "MonitorFromRect", (void **)&pMonitorFromRect },
	{ "AdjustWindowRectEx", (void **)&pAdjustWindowRectEx },
	{ "AdjustWindowRectExForDpi", (void **)&pAdjustWindowRectExForDpi },
	{ "SetDlgItemTextW", (void **)&pSetDlgItemTextW },
	{ "GetDlgItemTextW", (void **)&pGetDlgItemTextW },
	{ "SetWindowTextW", (void **)&pSetWindowTextW },
	{ "ModifyMenuW", (void **)&pModifyMenuW },
	{ "GetMenuStringW", (void **)&pGetMenuStringW },
	{ "SendDlgItemMessageW", (void **)&pSendDlgItemMessageW },
	{ "MessageBoxW", (void **)&pMessageBoxW },
	{ "DialogBoxIndirectParamW", (void **)&pDialogBoxIndirectParamW },
	{ "InsertMenuW", (void **)&pInsertMenuW },
	{ "AppendMenuW", (void **)&pAppendMenuW },
	{ "MonitorFromWindow", (void **)&pMonitorFromWindow },
	{ "MonitorFromPoint", (void **)&pMonitorFromPoint },
	{ "GetMonitorInfoA", (void **)&pGetMonitorInfoA },
	{},
};

static const APIInfo Lists_msimg32[] = {
	{ "AlphaBlend", (void **)&pAlphaBlend },
	{},
};

static const APIInfo Lists_gdi32[] = {
	{ "AddFontResourceExA", (void **)&pAddFontResourceExA },
	{ "RemoveFontResourceExA", (void **)&pRemoveFontResourceExA },
	{ "AddFontResourceExW", (void **)&pAddFontResourceExW },
	{ "RemoveFontResourceExW", (void **)&pRemoveFontResourceExW },
	{},
};

static const APIInfo Lists_Shcore[] = {
	{ "GetDpiForMonitor", (void **)&pGetDpiForMonitor },
	{},
};

static const APIInfo Lists_kernel32[] = {
	{ "GetFileAttributesW", (void **)&pGetFileAttributesW },
	{ "GetPrivateProfileStringW", (void **)&pGetPrivateProfileStringW },
	{ "GetConsoleWindow", (void **)&pGetConsoleWindow },
	{},
};

static const APIInfo Lists_shell32[] = {
	{ "DragQueryFileW", (void **)&pDragQueryFileW },
	{},
};

static const APIInfo Lists_comctl32[] = {
	{ "CreatePropertySheetPageW", (void **)&pCreatePropertySheetPageW },
	{ "PropertySheetW", (void **)&pPropertySheetW },
	{},
};

static const APIInfo Lists_hhctrl[] = {
	{ "HtmlHelpW", (void **)&pHtmlHelpW },
	{ "HtmlHelpA", (void **)&pHtmlHelpA },
	{},
};

static const DllInfo DllInfos[] = {
	{ _T("user32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_user32 },
	{ _T("msimg32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_msimg32 },
	{ _T("gdi32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_gdi32 },
	{ _T("Shcore.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_Shcore },
	{ _T("kernel32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_kernel32 },
	{ _T("shell32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_shell32 },
	{ _T("Comctl32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_comctl32 },
	{ _T("hhctrl.ocx"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_hhctrl },
	{},
};

void WinCompatInit()
{
	static BOOL done = FALSE;
	if (done) return;
	done = TRUE;

	DLLGetApiAddressFromLists(DllInfos);

	// 9x特別処理
	if (!IsWindowsNTKernel()) {
		// Windows 9x に存在しているAPI(環境依存?)
		// 正しく動作しないので無効とする
		pGetPrivateProfileStringW = NULL;
		pSetWindowTextW = NULL;
		pSetDlgItemTextW = NULL;
		pGetDlgItemTextW = NULL;
		pDialogBoxIndirectParamW = NULL;
		pCreateWindowExW = NULL;
		pRegisterClassW = NULL;
	}

	// GetConsoleWindow特別処理
	if (pGetConsoleWindow == NULL) {
		pGetConsoleWindow = GetConsoleWindowLocal;
	}
}
