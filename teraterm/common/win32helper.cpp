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
#include <stdio.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "asprintf.h"
#include "compat_win.h"

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
 *	@param section
 *	@param key
 *	@param def		デフォルト値
 *					NULLのときは L"" を返す
 *	@param ini		iniファイルのパス
 *					NULLのときはファイル指定なし
 *	@param str		文字列を格納するバッファ
 *					不要になったらfree()する
 *	@return	エラーコード,0(=NO_ERROR)のときエラーなし
 *
 *		次の場合 str = L"" が返る (free()すること)
 *			ini=NULLの場合
 *				戻り値 NO_ERROR
 *			keyに空が設定されている 'key='と記述 ("="の後に何も書いていない)
 *				戻り値 NO_ERROR
 *			keyが存在せずdef=NULL 場合
 *				戻り値 ERROR_FILE_NOT_FOUND
 *			GetPrivateProfileStringW() でエラーが発生した場合
 *				戻り値 エラー値
 *			メモリが確保できなかった場合
 *				戻り値 ERROR_NOT_ENOUGH_MEMORY
 */
DWORD hGetPrivateProfileStringW(const wchar_t *section, const wchar_t *key, const wchar_t *def, const wchar_t *ini, wchar_t **str)
{
	size_t size;
	wchar_t *b;
	DWORD error;

	if (ini == NULL) {
		// iniファイル指定なしのときデフォルト値を返す
		//		GetPrivateProfileStringW(A)() の仕様には
		//		ファイル名にNULLを渡して良いとの記述はない
		//			NULLを渡したとき、
		//			Windows10,11 ではファイルがないときと同じ動作
		//			Windows95 は仕様外の動作
		//				戻り値=バッファサイズ+2が返ってくる
		*str = _wcsdup(def != NULL ? def : L"");  // 引数がNULLのときの動作は未定義
		if (*str == NULL) {
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		return NO_ERROR;
	}
	size = 256;
	b = (wchar_t*)malloc(sizeof(wchar_t) * size);
	if (b == NULL) {
		error = ERROR_NOT_ENOUGH_MEMORY;
		goto error_return;
	}
	b[0] = 0;
	for(;;) {
		DWORD r = GetPrivateProfileStringW(section, key, def, b, (DWORD)size, ini);
		if (r == 0 || b[0] == L'\0') {
			error = GetLastError();
			free(b);
			*str = _wcsdup(L"");
			return error;
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
		if (filepart != NULL) {
			*filepart = NULL;
		}
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
	// GetWindowTextLengthW() が 0 を返したとき、
	// エラーならエラーがセットされるが、
	// エラーではないとき(正常終了時)、エラーをクリアしない(エラーなしをセットしない)
	// ここでエラーをクリアしておく
	SetLastError(NO_ERROR);
	int len = GetWindowTextLengthW(hWnd);
	if (len == 0) {
		DWORD err = GetLastError();
		if (err != NO_ERROR) {
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
	assert(hWnd != NULL);
	return hGetWindowTextW(hWnd, text);
}

DWORD hExpandEnvironmentStringsW(const wchar_t *src, wchar_t **expanded)
{
	size_t len = (size_t)ExpandEnvironmentStringsW(src, NULL, 0);
	wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * len);
	if (dest == NULL) {
		*expanded = NULL;
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	ExpandEnvironmentStringsW(src, dest, (DWORD)len);
	*expanded = dest;
	return NO_ERROR;
}

/**
 *	RegQueryValueExW の動的バッファ版
 *
 *	lpData が malloc() を使って確保される
 *	不要になったら free() すること
 */
LSTATUS hRegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, void **lpData,
						  LPDWORD lpcbData)
{
	BYTE *p;
	DWORD len = 0;
	LSTATUS r;
	r = RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, NULL, &len);
	if (r != ERROR_SUCCESS) {
		*lpData = NULL;
		goto finish;
	}
	p = (BYTE *)malloc(len);
	r = RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, p, &len);
	if (r != ERROR_SUCCESS) {
		free(p);
		*lpData = NULL;
		goto finish;
	}
	*lpData = p;
finish:
	if (lpcbData != NULL) {
		*lpcbData = len;
	}
	return r;
}

DWORD hGetMenuStringW(HMENU hMenu, UINT uIDItem, UINT flags, wchar_t **text)
{
	size_t len = (size_t)GetMenuStringW(hMenu, uIDItem, NULL, 0, flags);
	len++;
	wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * len);
	if (dest == NULL) {
		*text = NULL;
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	GetMenuStringW(hMenu, uIDItem, dest, (int)len, flags);
	*text = dest;
	return NO_ERROR;
}

DWORD hDragQueryFileW(HDROP hDrop, UINT iFile, wchar_t **filename)
{
	const UINT len = DragQueryFileW(hDrop, iFile, NULL, 0);
	if (len == 0) {
		*filename = NULL;
		return NO_ERROR;
	}
	wchar_t *fname = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
	if (fname == NULL) {
		*filename = NULL;
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	DragQueryFileW(hDrop, iFile, fname, len + 1);
	fname[len] = '\0';
	*filename = fname;
	return NO_ERROR;
}

DWORD hFormatMessageW(DWORD error, wchar_t **message)
{
	LPWSTR lpMsgBuf;
	DWORD r =
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_MAX_WIDTH_MASK,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf,
		0,
		NULL);
	if (r == 0) {
		*message = NULL;
		return GetLastError();
	}
	*message = _wcsdup(lpMsgBuf);
	LocalFree(lpMsgBuf);
	if (*message == NULL) {
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	return NO_ERROR;
}

BOOL hSetupDiGetDevicePropertyW(
	HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
	const DEVPROPKEY *PropertyKey,
	void **buf, size_t *buf_size)
{
	BOOL r;
	DEVPROPTYPE ulPropertyType;
	DWORD size;
	r = _SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, PropertyKey,
								   &ulPropertyType, NULL, 0, &size, 0);
	_CrtCheckMemory();
	if (r == FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		*buf = NULL;
		if (buf_size != NULL) {
			*buf_size = 0;
		}
		return FALSE;
	}

	BYTE *b = (BYTE *)malloc(size);
	r = _SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, PropertyKey,
								   &ulPropertyType, b, size, &size, 0);
	if (r == FALSE) {
		return FALSE;
	}
	_CrtCheckMemory();
	if (ulPropertyType == DEVPROP_TYPE_STRING) {
		// ポインタをそのまま返せばok (文字列)
		*buf = b;
		if (buf_size != NULL) {
			*buf_size = size;
		}
		return TRUE;
	} else if (ulPropertyType ==  DEVPROP_TYPE_FILETIME) {
		// buf = FILETIME 構造体の8バイト
		SYSTEMTIME stFileTime = {};
		FileTimeToSystemTime((FILETIME *)b, &stFileTime);
		free(b);
		int wbuflen = 64;
		int buflen = sizeof(wchar_t) * wbuflen;
		wchar_t *prop = (wchar_t *)malloc(buflen);
		_snwprintf_s(prop, wbuflen, _TRUNCATE, L"%d-%d-%d",
					 stFileTime.wMonth, stFileTime.wDay, stFileTime.wYear
			);
		*buf = prop;
		if (buf_size != NULL) {
			*buf_size = buflen;
		}
		return TRUE;
	}
	else if (ulPropertyType == DEVPROP_TYPE_GUID) {
		memcpy(buf, b, size);
		free(b);
		if (buf_size != NULL) {
			*buf_size = 0;
		}
		return TRUE;
	} else {
		assert(FALSE);
		free(b);
		*buf = NULL;
		if (buf_size != NULL) {
			*buf_size = 0;
		}
	}
	return FALSE;
}

/**
 *	リストボックス(コンボボックス)の文字列を取得する
 *	不要になったら free() すること
 *
 *	@param		hDlg	ダイアログのハンドル
 *	@param		id		コントロール(コンボボックス)のID
 *	@param		index	リストの通し番号(0-)
 *	@param[out]	text	設定されている文字列
 *						不要になったらfree()する
 *	@return	エラーコード,0(=NO_ERROR)のときエラーなし
 */
DWORD hGetDlgItemCBTextW(HWND hDlg, int id, int index, wchar_t **text)
{
	HWND hWnd = GetDlgItem(hDlg, id);
	assert(hWnd != NULL);
	LRESULT len = SendMessageW(hWnd, CB_GETLBTEXTLEN, index, 0);
	wchar_t *strW = (wchar_t *)malloc((size_t(len) + 1) * sizeof(wchar_t));
	if (strW == NULL) {
		*text = NULL;
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	SendMessageW(hWnd, CB_GETLBTEXT, index, (LPARAM)strW);
	strW[len] = 0;
	*text = strW;
	return NO_ERROR;
}
