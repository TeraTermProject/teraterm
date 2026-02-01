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

#pragma once

#include <windows.h>
#include <setupapi.h>

// VS2005(SDK7.1以下)のとき,LSTATUSがない
#if defined(_MSC_VER) && !defined(__MINGW32__) && _MSC_VER == 1400
typedef LONG LSTATUS;
#endif

#ifdef __cplusplus
extern "C" {
#endif

DWORD hGetModuleFileNameW(HMODULE hModule, wchar_t **buf);
DWORD hGetPrivateProfileStringW(const wchar_t *section, const wchar_t *key, const wchar_t *def, const wchar_t *ini, wchar_t **str);
DWORD hGetFullPathNameW(const wchar_t *lpFileName, wchar_t **fullpath, wchar_t **filepart);
DWORD hGetCurrentDirectoryW(wchar_t **dir);
DWORD hGetWindowTextW(HWND hWnd, wchar_t **text);
DWORD hGetDlgItemTextW(HWND hDlg, int id, wchar_t **text);
DWORD hExpandEnvironmentStringsW(const wchar_t *src, wchar_t **expanded);
LSTATUS hRegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, void **lpData,
						  LPDWORD lpcbData);
DWORD hGetMenuStringW(HMENU hMenu, UINT uIDItem, UINT flags, wchar_t **text);
DWORD hDragQueryFileW(HDROP hDrop, UINT iFile, wchar_t **filename);
DWORD hFormatMessageW(DWORD error, wchar_t **message);
BOOL hSetupDiGetDevicePropertyW(
	HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
	const DEVPROPKEY *PropertyKey,
	void **buf, size_t *buf_size);
DWORD hGetDlgItemCBTextW(HWND hDlg, int id, int index, wchar_t **text);

#ifdef __cplusplus
}
#endif
