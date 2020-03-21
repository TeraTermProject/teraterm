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

#if !defined(DPI_ENUMS_DECLARED)
typedef enum MONITOR_DPI_TYPE {
    MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI = 1,
    MDT_RAW_DPI = 2,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
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

extern ATOM (WINAPI *pRegisterClassW)(const WNDCLASSW *lpWndClass);
extern HWND (WINAPI *pCreateWindowExW)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X,
									   int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
									   LPVOID lpParam);
extern LRESULT (WINAPI *pDefWindowProcW)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern HPROPSHEETPAGE (WINAPI * pCreatePropertySheetPageW)(LPCPROPSHEETPAGEW constPropSheetPagePointer);
extern INT_PTR (WINAPI *pPropertySheetW)(LPCPROPSHEETHEADERW constPropSheetHeaderPointer);
extern LRESULT (WINAPI *pSendDlgItemMessageW)(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
extern BOOL(WINAPI *pModifyMenuW)(HMENU hMnu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
extern int(WINAPI *pGetMenuStringW)(HMENU hMenu, UINT uIDItem, LPWSTR lpString, int cchMax, UINT flags);
extern BOOL(WINAPI *pSetWindowTextW)(HWND hWnd, LPCWSTR lpString);
extern DWORD(WINAPI *pGetPrivateProfileStringW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault,
												LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
extern UINT(WINAPI *pDragQueryFileW)(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);
extern DWORD (WINAPI *pGetFileAttributesW)(LPCWSTR lpFileName);
extern BOOL (WINAPI *pSetDlgItemTextW)(HWND hDlg, int nIDDlgItem, LPCWSTR lpString);
extern BOOL (WINAPI *pGetDlgItemTextW)(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax);
extern BOOL (WINAPI *pAlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
extern BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPCRECT,MONITORENUMPROC,LPARAM);
extern HMONITOR (WINAPI *pMonitorFromRect)(LPCRECT lprc, DWORD dwFlags);
extern DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
extern BOOL (WINAPI *pIsValidDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
extern UINT (WINAPI *pGetDpiForWindow)(HWND hwnd);
extern HRESULT (WINAPI *pGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
extern BOOL (WINAPI *pAdjustWindowRectEx)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
extern BOOL (WINAPI *pAdjustWindowRectExForDpi)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
extern BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
extern int (WINAPI *pAddFontResourceExW)(LPCWSTR name, DWORD fl, PVOID res);
extern BOOL (WINAPI *pRemoveFontResourceExW)(LPCWSTR name, DWORD fl, PVOID pdv);
extern HWND (WINAPI *pGetConsoleWindow)(void);
extern int (WINAPI *pMessageBoxW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
extern INT_PTR (WINAPI *pDialogBoxIndirectParamW)(HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
extern HWND (WINAPI *pCreateDialogIndirectParamW)(HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate,
												  HWND hWndParent, DLGPROC lpDialogFunc,
												  LPARAM dwInitParam);
extern HWND (WINAPI *pHtmlHelpW)(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);
extern HWND (WINAPI *pHtmlHelpA)(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);
extern BOOL (WINAPI *pInsertMenuW)(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
extern BOOL (WINAPI *pAppendMenuW)(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
extern HMONITOR (WINAPI *pMonitorFromWindow)(HWND hwnd, DWORD dwFlags);
extern HMONITOR (WINAPI *pMonitorFromPoint)(POINT pt, DWORD dwFlags);
extern HMONITOR (WINAPI *pMonitorFromRect)(LPCRECT lprc, DWORD dwFlags);
extern BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);
extern LRESULT (WINAPI *pSendMessageW)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern int (WINAPI *pGetWindowTextW)(HWND hWnd, LPWSTR lpString, int nMaxCount);
extern int (WINAPI *pGetWindowTextLengthW)(HWND hWnd);
extern BOOL (WINAPI *pShell_NotifyIconW)(DWORD dwMessage, NOTIFYICONDATAW *lpData);
extern LONG (WINAPI *pSetWindowLongW)(HWND hWnd, int nIndex, LONG dwNewLong);
#ifdef _WIN64
extern LONG_PTR (WINAPI *pSetWindowLongPtrW)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
#endif
extern LRESULT (WINAPI *pCallWindowProcW)(WNDPROC lpPrevWndFunc,
										  HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern void (WINAPI *pOutputDebugStringW)(LPCWSTR lpOutputString);

void WinCompatInit();

#ifdef __cplusplus
}
#endif
