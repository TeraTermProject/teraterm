/*
 * Copyright (C) 2021- TeraTerm Project
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

#include <windows.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "win32helper.h"

/**
 *	GetModuleFileNameW() の動的バッファ版
 *
 *	@param buf	文字列を格納するバッファ
 *				不要になったらfree()する
 *	@return	エラーコード,0(=NO_ERROR)のときエラーなし
 */
DWORD hGetModuleFileNameW(HMODULE hModule, wchar_t **buf)
{
	size_t size = MAX_PATH;
	wchar_t *b = (wchar_t*)malloc(sizeof(wchar_t) * size);
	DWORD error;
	if (b == NULL) {
		error = ERROR_NOT_ENOUGH_MEMORY;
		goto error_return;
	}

	for(;;) {
		DWORD r = GetModuleFileNameW(hModule, b, (DWORD)size);
		if (r == 0) {
			// 関数が失敗
			error = GetLastError();
			break;
		} else if (r < size - 1) {
			// 取得成功
			size = r + 1;
			b = (wchar_t*)realloc(b, sizeof(wchar_t) * size);
			*buf = b;
			return NO_ERROR;
		} else {
			size *= 2;
			wchar_t *p = (wchar_t*)realloc(b, sizeof(wchar_t) * size);
			if (p == NULL) {
				free(b);
				error = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}
			b = p;
		}
    }

	// error
	free(b);
error_return:
	*buf = NULL;
	return error;
}

/**
 *	GetPrivateProfileStringW() の動的バッファ版
 *
 *	@param str	文字列を格納するバッファ
 *				不要になったらfree()する
 *	@return	エラーコード,0(=NO_ERROR)のときエラーなし
 */
DWORD hGetPrivateProfileStringW(const wchar_t *section, const wchar_t *key, const wchar_t *def, const wchar_t *ini, wchar_t **str)
{
	size_t size = 256;
	wchar_t *b = (wchar_t*)malloc(sizeof(wchar_t) * size);
	DWORD error;
	if (b == NULL) {
		error = ERROR_NOT_ENOUGH_MEMORY;
		goto error_return;
	}
	b[0] = 0;
	for(;;) {
		DWORD r = GetPrivateProfileStringW(section, key, def, b, (DWORD)size, ini);
		if (r == 0 || b[0] == L'\0') {
			// 次の場合ここに入る
			//   iniに'key='と記述 ("="の後に何も書いていない)
			//   iniに'key=...' が存在しない かつ def=NULL
			free(b);
			*str = NULL;
			return NO_ERROR;
		} else if (r < size - 2) {
			size = r + 1;
			b = (wchar_t *)realloc(b, sizeof(wchar_t) * size);
			*str = b;
			return NO_ERROR;
		} else {
			wchar_t *p;
			size *= 2;
			p = (wchar_t*)realloc(b, sizeof(wchar_t) * size);
			if (p == NULL) {
				free(b);
				error = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}
			b = p;
		}
	}

	// error
	free(b);
error_return:
	*str = NULL;
	return error;
}

/**
 *	GetFullPathNameW() の動的バッファ版
 *
 *	@param fullpath		fullpathを格納するバッファ
 *						不要になったらfree()する
 *	@return	エラーコード,0(=NO_ERROR)のときエラーなし
 */
DWORD hGetFullPathNameW(const wchar_t *lpFileName, wchar_t **fullpath, wchar_t **filepart)
{
	size_t len = GetFullPathNameW(lpFileName, 0, NULL, NULL);		// include L'\0'
	if (len == 0) {
		*fullpath = NULL;
		*filepart = NULL;
		return GetLastError();
	}
	wchar_t *path = (wchar_t *)malloc(sizeof(wchar_t) * len);
	wchar_t *file;
	len = GetFullPathNameW(lpFileName, (DWORD)len, path, &file);
	if (len == 0) {
		free(path);
		return GetLastError();
	}
	if (filepart != NULL) {
		*filepart = file;
	}
	*fullpath = path;
	return NO_ERROR;
}

/**
 *	GetCurrentDirectoryW() の動的バッファ版
 *
 *	@param[out]	dir		フォルダ
 *						不要になったらfree()する
 *	@return	エラーコード,0(=NO_ERROR)のときエラーなし
 */
DWORD hGetCurrentDirectoryW(wchar_t **dir)
{
	DWORD len = GetCurrentDirectoryW(0, NULL);
	if (len == 0) {
		*dir = NULL;
		return GetLastError();
	}
	wchar_t *d = (wchar_t *)malloc(sizeof(wchar_t) * len);
	len = GetCurrentDirectoryW(len, d);
	if (len == 0) {
		free(d);
		*dir = NULL;
		return GetLastError();
	}
	*dir = d;
	return 0;
}

/**
 *	hWndの文字列を取得する
 *	不要になったら free() すること
 *
 *	@param[out]	text	設定されている文字列
 *						不要になったらfree()する
 *	@return	エラーコード,0(=NO_ERROR)のときエラーなし
 */
DWORD hGetWindowTextW(HWND hWnd, wchar_t **text)
{
	int len = GetWindowTextLength(hWnd);
	if (len == 0) {
		DWORD err = GetLastError();
		if (err != 0) {
			*text = NULL;
			return err;
		}
		*text = _wcsdup(L"");
		return 0;
	}

	wchar_t *strW = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
	if (strW == NULL) {
		*text = NULL;
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	GetWindowTextW(hWnd, strW, len + 1);
	strW[len] = 0;
	*text = strW;
	return 0;
}

DWORD hGetDlgItemTextW(HWND hDlg, int id, wchar_t **text)
{
	HWND hWnd = GetDlgItem(hDlg, id);
	return hGetWindowTextW(hWnd, text);
}
