/*
 * Copyright (C) 2019-2020 TeraTerm Project
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

/*
 * W to A Wrapper
 *
 * API名はW版の頭に '_' を付けたものを使用する
 */

#pragma once

#include <windows.h>
#include <commdlg.h>	// for _GetOpenFileNameW()
#include <shlobj.h>	// for _SHBrowseForFolderW()

#ifdef __cplusplus
extern "C" {
#endif

/**
 *	NOTIFYICONDATA は define でサイズが変化する
 *	どんな環境でも変化しないよう定義
 *
 * Shlwapi.dll 5.0
 * 	Win98+,2000+
 */
/*  size 504 bytes */
typedef struct {
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	char   szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	char   szInfo[256];
	union {
		UINT  uTimeout;
		UINT  uVersion;	 // used with NIM_SETVERSION, values 0, 3 and 4
	} DUMMYUNIONNAME;
	char   szInfoTitle[64];
	DWORD dwInfoFlags;
	//GUID guidItem;		// XP+
	//HICON hBalloonIcon;	// Vista+
} TT_NOTIFYICONDATAA_V2;

/*  size 952 bytes */
typedef struct {
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	wchar_t	 szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	wchar_t	 szInfo[256];
	union {
		UINT  uTimeout;
		UINT  uVersion;	 // used with NIM_SETVERSION, values 0, 3 and 4
	} DUMMYUNIONNAME;
	wchar_t	 szInfoTitle[64];
	DWORD dwInfoFlags;
	//GUID guidItem;		// XP+
	//HICON hBalloonIcon;	// Vista+
} TT_NOTIFYICONDATAW_V2;

BOOL _SetWindowTextW(HWND hWnd, LPCWSTR lpString);
BOOL _SetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString);
UINT _GetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax);
LRESULT _SendDlgItemMessageW(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT _SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
HWND _CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y,
							 int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
							 LPVOID lpParam);
ATOM _RegisterClassW(const WNDCLASSW *lpWndClass);
int _DrawTextW(HDC hdc, LPCWSTR lpchText, int cchText, LPRECT lprc, UINT format);
int _MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
BOOL _InsertMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
BOOL _AppendMenuW(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
HWND _HtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);
int _GetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount);
int _GetWindowTextLengthW(HWND hWnd);
LONG_PTR _SetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LRESULT _CallWindowProcW(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// kernel32.dll
DWORD _GetFileAttributesW(LPCWSTR lpFileName);
DWORD _GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer);
BOOL _SetCurrentDirectoryW(LPCWSTR lpPathName);
void _OutputDebugStringW(LPCWSTR lpOutputString);
DWORD _GetPrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault,
								LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
BOOL _WritePrivateProfileStringW(LPCWSTR lpAppName,LPCWSTR lpKeyName,LPCWSTR lpString,LPCWSTR lpFileName);
BOOL _CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
					 LPSECURITY_ATTRIBUTES lpProcessAttributes,
					 LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
					 DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
					 LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

// gdi32.lib
int _AddFontResourceW(LPCWSTR lpFileName);
BOOL _RemoveFontResourceW(LPCWSTR lpFileName);
int _AddFontResourceExW(LPCWSTR name, DWORD fl, PVOID res);
BOOL _RemoveFontResourceExW(LPCWSTR name, DWORD fl, PVOID pdv);

// Comctl32.lib
HPROPSHEETPAGE _CreatePropertySheetPageW(LPCPROPSHEETPAGEW_V1 constPropSheetPagePointer);
INT_PTR _PropertySheetW(PROPSHEETHEADERW *constPropSheetHeaderPointer);
//INT_PTR _PropertySheetW(PROPSHEETHEADERW_V1 *constPropSheetHeaderPointer);

// Comdlg32.lib
BOOL _GetOpenFileNameW(LPOPENFILENAMEW ofnW);
BOOL _GetSaveFileNameW(LPOPENFILENAMEW ofnW);

// shell32.lib
UINT _DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);
BOOL _Shell_NotifyIconW(DWORD dwMessage, TT_NOTIFYICONDATAW_V2 *lpData);
LPITEMIDLIST _SHBrowseForFolderW(LPBROWSEINFOW lpbi);
BOOL _SHGetPathFromIDListW(LPITEMIDLIST pidl, LPWSTR pszPath);

HWND _CreateDialogIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate,
								 HWND hWndParent, DLGPROC lpDialogFunc,
								 LPARAM dwInitParam);
INT_PTR _DialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent,
								 DLGPROC lpDialogFunc, LPARAM lParamInit);

#ifdef __cplusplus
}
#endif
