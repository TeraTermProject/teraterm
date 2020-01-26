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
#include "ttlib.h"		// for IsWindowsNTKernel()

#include "layer_for_unicode.h"

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

LRESULT _SendDlgItemMessageW(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (pSendDlgItemMessageW != NULL) {
		return pSendDlgItemMessageW(hDlg, nIDDlgItem, Msg, wParam, lParam);
	}

	LRESULT retval;
	switch(Msg) {
	case CB_ADDSTRING:
	case LB_ADDSTRING: {
		char *strA = ToCharW((wchar_t *)lParam);
		retval = SendDlgItemMessageA(hDlg, nIDDlgItem, Msg, wParam, (LPARAM)strA);
		free(strA);
		break;
	}
	case WM_GETTEXTLENGTH: {
		retval = 0;
		LRESULT len = SendDlgItemMessageA(hDlg, nIDDlgItem, WM_GETTEXTLENGTH, 0, 0);
		len++;  // for '\0'
		char *strA = (char *)malloc(sizeof(char) * len);
		if (strA != NULL) {
			GetDlgItemTextA(hDlg, nIDDlgItem, strA, (int)len);
			strA[len-1] = '\0';
			wchar_t *strW = ToWcharA(strA);
			if (strW != NULL) {
				retval = (LRESULT)wcslen(strW);// '\0'を含まない長さを返す
				free(strW);
			}
			free(strA);
		}
		break;
	}
	default:
		retval = SendDlgItemMessageA(hDlg, nIDDlgItem, Msg, wParam, lParam);
		break;
	}
	return retval;
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

	// この文字列長は ANSI
	size_t len = SendDlgItemMessageA(hDlg, nIDDlgItem, WM_GETTEXTLENGTH, 0, 0);
	len++;  // for '\0'
	char *strA = (char *)malloc(sizeof(char) * len);
	if (strA != NULL) {
		GetDlgItemTextA(hDlg, nIDDlgItem, strA, (int)len);
		strA[len - 1] = '\0';
		wchar_t *strW = ToWcharA(strA);
		if (strW != NULL) {
			wcscpy_s(lpString, cchMax, strW);
			UINT len = (UINT)wcslen(strW); // '\0' を含まない長さを返す
			free(strW);
			return len;
		}
	}

	if (cchMax > 0)
		lpString[0] = 0;
	return 0;
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
