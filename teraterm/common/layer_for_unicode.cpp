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

#include <windows.h>

#include "codeconv.h"
#include "compat_win.h"
#include "dllutil.h"
#include "ttlib.h"		// for IsWindowsNTKernel()

#include "layer_for_unicode.h"

class Initializer {
public:
	Initializer() {
		DLLInit();
		WinCompatInit();
	}
	~Initializer() {
		DLLExit();
	}
};

static Initializer initializer;

BOOL _SetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString)
{
	if (pSetDlgItemTextW != NULL) {
		return pSetDlgItemTextW(hDlg, nIDDlgItem, lpString);
	}

	char *strA = ToCharW(lpString);
	BOOL retval = SetDlgItemTextA(hDlg, nIDDlgItem, strA);
	free(strA);
	return retval;
}

UINT _DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch)
{
	if (pDragQueryFileW != NULL) {
		return pDragQueryFileW(hDrop, iFile, lpszFile, cch);
	}

	UINT retval;
	if (iFile == 0xffffffff) {
		// ファイル数問い合わせ
		retval = DragQueryFileA(hDrop, iFile, NULL, 0);
	}
	else if (lpszFile == NULL) {
		// ファイル名の文字数問い合わせ
		char FileNameA[MAX_PATH];
		retval = DragQueryFileA(hDrop, iFile, FileNameA, MAX_PATH);
		if (retval != 0) {
			wchar_t *FileNameW = ToWcharA(FileNameA);
			retval = (UINT)(wcslen(FileNameW) + 1);
			free(FileNameW);
		}
	}
	else {
		// ファイル名取得
		char FileNameA[MAX_PATH];
		retval = DragQueryFileA(hDrop, iFile, FileNameA, MAX_PATH);
		if (retval != 0) {
			wchar_t *FileNameW = ToWcharA(FileNameA);
			wcscpy_s(lpszFile, cch, FileNameW);
			free(FileNameW);
		}
	}
	return retval;
}

DWORD _GetFileAttributesW(LPCWSTR lpFileName)
{
	if (pGetFileAttributesW != NULL) {
		return pGetFileAttributesW(lpFileName);
	}

	char *FileNameA;
	if (lpFileName == NULL) {
		FileNameA = NULL;
	} else {
		FileNameA = ToCharW(lpFileName);
	}
	const DWORD attr = GetFileAttributesA(FileNameA);
	free(FileNameA);
	return attr;
}

/**
 * hWnd に設定されている文字列を取得
 *
 * @param[in]		hWnd
 * @param[in,out]	lenW	文字数(L'\0'を含まない)
 * @return			文字列
 */
static wchar_t *SendMessageAFromW_WM_GETTEXT(HWND hWnd, size_t *lenW)
{
	// lenA = excluding the terminating null character.
	size_t lenA = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
	char *strA = (char *)malloc(lenA + 1);
	if (strA == NULL) {
		*lenW = 0;
		return NULL;
	}
	lenA = GetWindowTextA(hWnd, strA, (int)(lenA + 1));
	strA[lenA] = '\0';
	wchar_t *strW = ToWcharA(strA);
	free(strA);
	if (strW == NULL) {
		*lenW = 0;
		return NULL;
	}
	*lenW = wcslen(strW);
	return strW;
}

/**
 * hWnd(ListBox) に設定されている文字列を取得
 *
 * @param[in]		hWnd
 * @param[in]		wParam	項目番号(0～)
 * @param[in,out]	lenW	文字数(L'\0'を含まない)
 * @return			文字列
 */
static wchar_t *SendMessageAFromW_LB_GETTEXT(HWND hWnd, WPARAM wParam, size_t *lenW)
{
	// lenA = excluding the terminating null character.
	size_t lenA = SendMessageA(hWnd, LB_GETTEXTLEN, wParam, 0);
	char *strA = (char *)malloc(lenA + 1);
	if (strA == NULL) {
		*lenW = 0;
		return NULL;
	}
	lenA = SendMessageA(hWnd, LB_GETTEXT, wParam, (LPARAM)strA);
	wchar_t *strW = ToWcharA(strA);
	free(strA);
	if (strW == NULL) {
		*lenW = 0;
		return NULL;
	}
	*lenW = wcslen(strW);
	return strW;
}

int _GetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
	if (pGetWindowTextW != NULL) {
		return pGetWindowTextW(hWnd, lpString, nMaxCount);
	}

	size_t lenW;
	wchar_t *strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
	wchar_t *dest_ptr = (wchar_t *)lpString;
	size_t dest_len = (size_t)nMaxCount;
	wcsncpy_s(dest_ptr, dest_len, strW, _TRUNCATE);
	free(strW);
	return (int)(dest_len - 1);
}

int _GetWindowTextLengthW(HWND hWnd)
{
	if (pGetWindowTextLengthW != NULL) {
		return pGetWindowTextLengthW(hWnd);
	}

	size_t lenW;
	wchar_t *strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
	free(strW);
	return (int)(lenW - 1);
}

static LRESULT SendMessageAFromW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT retval;
	switch(Msg) {
	case CB_ADDSTRING:
	case CB_INSERTSTRING:
	case LB_ADDSTRING:
	case LB_INSERTSTRING: {
		char *strA = ToCharW((wchar_t *)lParam);
		retval = SendMessageA(hWnd, Msg, wParam, (LPARAM)strA);
		free(strA);
		return retval;
	}
	case WM_GETTEXTLENGTH:
	case LB_GETTEXTLEN: {
		size_t lenW;
		wchar_t *strW;
		if (Msg == WM_GETTEXTLENGTH) {
			strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
		}
		else {
			strW = SendMessageAFromW_LB_GETTEXT(hWnd, wParam, &lenW);
		}
		free(strW);
		return lenW;
	}
	case WM_GETTEXT:
	case LB_GETTEXT: {
		size_t lenW;
		wchar_t *strW;
		size_t dest_len;
		if (Msg == WM_GETTEXT) {
			strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
			dest_len = (size_t)wParam;
		}
		else {
			strW = SendMessageAFromW_LB_GETTEXT(hWnd, wParam, &lenW);
			dest_len = lenW + 1;
		}
		wchar_t *dest_ptr = (wchar_t *)lParam;
		wcsncpy_s(dest_ptr, dest_len, strW, _TRUNCATE);
		free(strW);
		return dest_len - 1 < lenW ? dest_len - 1 : lenW;
	}
	default:
		retval = SendMessageA(hWnd, Msg, wParam, lParam);
		break;
	}
	return retval;
}

LRESULT _SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (pSendMessageW != NULL) {
		return pSendMessageW(hWnd, Msg, wParam, lParam);
	}
	return SendMessageAFromW(hWnd, Msg, wParam, lParam);
}

LRESULT _SendDlgItemMessageW(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (pSendDlgItemMessageW != NULL) {
		return pSendDlgItemMessageW(hDlg, nIDDlgItem, Msg, wParam, lParam);
	}

	HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
	return SendMessageAFromW(hWnd, Msg, wParam, lParam);
}

HWND _CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y,
							 int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	if (pCreateWindowExW != NULL) {
		return pCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu,
								hInstance, lpParam);
	}

	char *lpClassNameA = ToCharW(lpClassName);
	char *lpWindowNameA = ToCharW(lpWindowName);
	HWND hWnd = CreateWindowExA(dwExStyle, lpClassNameA, lpWindowNameA, dwStyle, X, Y, nWidth, nHeight, hWndParent,
								hMenu, hInstance, lpParam);
	free(lpClassNameA);
	if (lpWindowNameA != NULL) {
		free(lpWindowNameA);
	}
	return hWnd;
}

ATOM _RegisterClassW(const WNDCLASSW *lpWndClass)
{
	if (pRegisterClassW != NULL) {
		return pRegisterClassW(lpWndClass);
	}

	char *menu_nameA = ToCharW(lpWndClass->lpszMenuName);
	char *class_nameA = ToCharW(lpWndClass->lpszClassName);

	WNDCLASSA WndClassA;
	WndClassA.style = lpWndClass->style;
	WndClassA.lpfnWndProc = lpWndClass->lpfnWndProc;
	WndClassA.cbClsExtra = lpWndClass->cbClsExtra;
	WndClassA.cbWndExtra = lpWndClass->cbWndExtra;
	WndClassA.hInstance = lpWndClass->hInstance;
	WndClassA.hIcon = lpWndClass->hIcon;
	WndClassA.hCursor = lpWndClass->hCursor;
	WndClassA.hbrBackground = lpWndClass->hbrBackground;
	WndClassA.lpszMenuName = menu_nameA;
	WndClassA.lpszClassName = class_nameA;
	ATOM atom = RegisterClassA(&WndClassA);

	if (menu_nameA != NULL) {
		free(menu_nameA);
	}
	if (class_nameA != NULL) {
		free(class_nameA);
	}
	return atom;
}

BOOL _SetWindowTextW(HWND hWnd, LPCWSTR lpString)
{
	if (pSetWindowTextW != NULL) {
		return pSetWindowTextW(hWnd, lpString);
	}

	char *strA = ToCharW(lpString);
	BOOL retval = SetWindowTextA(hWnd, strA);
	free(strA);
	return retval;
}

UINT _GetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax)
{
	if (pGetDlgItemTextW != NULL) {
		return pGetDlgItemTextW(hDlg, nIDDlgItem, lpString, cchMax);
	}

	if (cchMax <= 1) {
		return 0;
	}

	HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
	size_t lenW;
	wchar_t *strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
	wchar_t *dest_ptr = lpString;
	size_t dest_len = (size_t)cchMax;
	wcsncpy_s(dest_ptr, dest_len, strW, _TRUNCATE);
	free(strW);
	return (UINT)(dest_len - 1 < lenW ? dest_len - 1 : lenW);
}

/**
 *	@param[in]		hdc
 *	@param[in]		lpchText	文字列
 *	@param[in]		cchText		文字数(-1のときlpchTextの文字列長)
 *	@param[in]		lprc		表示rect
 *	@param[in]		format
 *
 *		TODO:9x系でDrawTextWが正しく動作する?
 */
int _DrawTextW(HDC hdc, LPCWSTR lpchText, int cchText, LPRECT lprc, UINT format)
{
	if (IsWindowsNTKernel()) {
		return DrawTextW(hdc, lpchText, cchText, lprc, format);
	}

	int strW_len = (cchText == -1) ? 0 : cchText;
	size_t strA_len;
	char *strA = _WideCharToMultiByte(lpchText, strW_len, CP_ACP, &strA_len);
	int result = DrawTextA(hdc, strA, (int)strA_len, lprc, format);
	free(strA);
	return result;
}

int _MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
	if (pMessageBoxW != NULL) {
		return pMessageBoxW(hWnd, lpText, lpCaption, uType);
	}

	char *textA = ToCharW(lpText);
	char *captionA = ToCharW(lpCaption);
	int result = MessageBoxA(hWnd, textA, captionA, uType);
	free(textA);
	free(captionA);
	return result;
}

BOOL _InsertMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	if (pInsertMenuW != NULL) {
		return pInsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
	}

	char *itemA = ToCharW(lpNewItem);
	int result = InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, itemA);
	free(itemA);
	return result;
}

HWND _HtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
	if (pHtmlHelpW != NULL) {
		return pHtmlHelpW(hwndCaller, pszFile, uCommand, dwData);
	}
	if (pHtmlHelpA != NULL) {
		char *fileA = ToCharW(pszFile);
		HWND result = pHtmlHelpA(hwndCaller, fileA, uCommand, dwData);
		free(fileA);
		return result;
	}
	return NULL;
}

BOOL _AppendMenuW(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	if (pAppendMenuW != NULL) {
		return pAppendMenuW(hMenu, uFlags, uIDNewItem, lpNewItem);
	}
	char *itemA = ToCharW(lpNewItem);
	BOOL result = AppendMenuA(hMenu, uFlags, uIDNewItem, itemA);
	free(itemA);
	return result;
}

int  _AddFontResourceExW(LPCWSTR name, DWORD fl, PVOID res)
{
	if (pAddFontResourceExW != NULL) {
		/* Windows 2000以降は使えるはず */
		return pAddFontResourceExW(name, fl, res);
	}
	return 0;
}

BOOL _RemoveFontResourceExW(LPCWSTR name, DWORD fl, PVOID pdv)
{
	if (pRemoveFontResourceExW != NULL) {
		/* Windows 2000以降は使えるはず */
		return pRemoveFontResourceExW(name, fl, pdv);
	}
	return FALSE;
}

int _AddFontResourceW(LPCWSTR lpFileName)
{
	char *filenameA = ToCharW(lpFileName);
	int result = AddFontResourceA(filenameA);
	free(filenameA);
	return result;
}

BOOL _RemoveFontResourceW(LPCWSTR lpFileName)
{
	char *filenameA = ToCharW(lpFileName);
	int result = RemoveFontResourceA(filenameA);
	free(filenameA);
	return result;
}

/*
 * lpData.cbSize == 952のときのみ ANSI関数で処理する
 */
BOOL _Shell_NotifyIconW(DWORD dwMessage, TT_NOTIFYICONDATAW_V2 *lpData)
{
	if (pShell_NotifyIconW != NULL) {
		return pShell_NotifyIconW(dwMessage, (PNOTIFYICONDATAW)lpData);
	}

	const TT_NOTIFYICONDATAW_V2 *w = lpData;
	if (w->cbSize != sizeof(TT_NOTIFYICONDATAW_V2)) {
		return FALSE;
	}

	TT_NOTIFYICONDATAA_V2 nid;
	TT_NOTIFYICONDATAA_V2 *p = &nid;
	p->cbSize = sizeof(nid);
	p->hWnd = w->hWnd;
	p->uID = w->uID;
	p->uFlags = w->uFlags;
	p->uCallbackMessage = w->uCallbackMessage;
	p->hIcon = w->hIcon;
	p->dwState = w->dwState;
	p->dwStateMask = w->dwStateMask;
	p->uTimeout = w->uTimeout;
	p->dwInfoFlags = w->dwInfoFlags;

	char *strA = ToCharW(w->szTip);
	strcpy_s(p->szTip, strA);
	free(strA);
	strA = ToCharW(w->szInfoTitle);
	strcpy_s(p->szInfoTitle, strA);
	free(strA);
	strA = ToCharW(w->szInfo);
	strcpy_s(p->szInfo, strA);
	free(strA);

	BOOL r = Shell_NotifyIconA(dwMessage, (PNOTIFYICONDATAA)p);
	return r;
}

HWND _CreateDialogIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc,
								 LPARAM dwInitParam)
{
	if (pCreateDialogIndirectParamW != NULL) {
		return pCreateDialogIndirectParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
	}
	return CreateDialogIndirectParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

INT_PTR _DialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent,
								 DLGPROC lpDialogFunc, LPARAM lParamInit)
{
	if (pDialogBoxIndirectParamW != NULL) {
		return pDialogBoxIndirectParamW(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParamInit);
	}
	return DialogBoxIndirectParamA(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParamInit);
}

LONG _SetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong)
{
	if (pSetWindowLongW != NULL) {
		return pSetWindowLongW(hWnd, nIndex, dwNewLong);
	}
	return SetWindowLongA(hWnd, nIndex, dwNewLong);
}

LONG_PTR _SetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
#ifdef _WIN64
	if (pSetWindowLongPtrW != NULL) {
		return pSetWindowLongPtrW(hWnd, nIndex, dwNewLong);
	}
	return SetWindowLongPtr(hWnd, nIndex, dwNewLong);
#else
	return _SetWindowLongW(hWnd, nIndex, dwNewLong);
#endif
}

LRESULT _CallWindowProcW(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (pCallWindowProcW != NULL) {
		return pCallWindowProcW(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
	}
	return CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}

void _OutputDebugStringW(LPCWSTR lpOutputString)
{
	if (pOutputDebugStringW != NULL) {
		return pOutputDebugStringW(lpOutputString);
	}

	char *strA = ToCharW(lpOutputString);
	OutputDebugStringA(strA);
	free(strA);
}

DWORD _GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	if (pGetCurrentDirectoryW != NULL) {
		return pGetCurrentDirectoryW(nBufferLength, lpBuffer);
	}
	char dir[MAX_PATH];
	DWORD len = GetCurrentDirectoryA(_countof(dir), dir);
	wchar_t *strW = ToWcharA(dir);
	wcsncpy_s(lpBuffer, nBufferLength, strW, _TRUNCATE);
	free(strW);
	DWORD r =  (DWORD)wcslen(lpBuffer);
	return r;
}

BOOL _SetCurrentDirectoryW(LPCWSTR lpPathName)
{
	if (pSetCurrentDirectoryW != NULL) {
		return pSetCurrentDirectoryW(lpPathName);
	}
	char *strA = ToCharW(lpPathName);
	BOOL r = SetCurrentDirectoryA(strA);
	free(strA);
	return r;
}

PIDLIST_ABSOLUTE _SHBrowseForFolderW(LPBROWSEINFOW lpbi)
{
	if (pSHBrowseForFolderW != NULL) {
		return pSHBrowseForFolderW(lpbi);
	}

	BROWSEINFOA biA;
	biA.hwndOwner = lpbi->hwndOwner;
	biA.pidlRoot = lpbi->pidlRoot;
	biA.pszDisplayName = ToCharW(lpbi->pszDisplayName);
	biA.lpszTitle = ToCharW(lpbi->lpszTitle);
	biA.ulFlags = lpbi->ulFlags;
	biA.lpfn = lpbi->lpfn;
	biA.lParam = lpbi->lParam;
	PIDLIST_ABSOLUTE pidlBrowse = SHBrowseForFolderA(&biA);
	free(biA.pszDisplayName);
	free((void *)biA.lpszTitle);

	return pidlBrowse;
}

BOOL _SHGetPathFromIDListW(PCIDLIST_ABSOLUTE pidl, LPWSTR pszPath)
{
	if (pSHGetPathFromIDListW != NULL) {
		return pSHGetPathFromIDListW(pidl, pszPath);
	}

	char pathA[MAX_PATH];
	BOOL r = SHGetPathFromIDListA(pidl, pathA);
	::MultiByteToWideChar(CP_ACP, 0, pathA, -1, pszPath, MAX_PATH);
	return r;
}

DWORD _GetPrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault,
								LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
	if (pGetPrivateProfileStringW != NULL) {
		return pGetPrivateProfileStringW(lpAppName, lpKeyName, lpDefault,
										 lpReturnedString, nSize, lpFileName);
	}

	char *buf = (char* )malloc(nSize);
	char *appA = ToCharW(lpAppName);
	char *keyA = ToCharW(lpKeyName);
	char *defA = ToCharW(lpDefault);
	char *fileA = ToCharW(lpFileName);
	DWORD r = GetPrivateProfileStringA(appA, keyA, defA, buf, nSize, fileA);
	::MultiByteToWideChar(CP_ACP, 0, buf, -1, lpReturnedString, nSize);
	r = (DWORD)wcslen(lpReturnedString);
	free(appA);
	free(keyA);
	free(defA);
	free(fileA);
	free(buf);
	return r;
}

BOOL _WritePrivateProfileStringW(LPCWSTR lpAppName,LPCWSTR lpKeyName,LPCWSTR lpString,LPCWSTR lpFileName)
{
	if (pWritePrivateProfileStringW != NULL) {
		return pWritePrivateProfileStringW(lpAppName, lpKeyName, lpString, lpFileName);
	}

	char *appA = ToCharW(lpAppName);
	char *keyA = ToCharW(lpKeyName);
	char *strA = ToCharW(lpString);
	char *fileA = ToCharW(lpFileName);
	BOOL r = WritePrivateProfileStringA(appA, keyA, strA, fileA);
	free(appA);
	free(keyA);
	free(strA);
	free(fileA);
	return r;
}
