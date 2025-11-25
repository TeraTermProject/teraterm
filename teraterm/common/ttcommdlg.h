/*
 * Copyright (C) 2020- TeraTerm Project
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

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DllExport)
#define DllExport __declspec(dllexport)
#endif

typedef struct {
	HWND hwndOwner;
	HINSTANCE hInstance;
	LPCWSTR lpstrFilter;
	DWORD nFilterIndex;
	LPCWSTR lpstrFile;	// 初期ファイル名
	LPCWSTR lpstrInitialDir;	// 初期フォルダ
	LPCWSTR lpstrTitle;
	DWORD Flags;
	LPCWSTR lpstrDefExt;
} TTOPENFILENAMEW;

BOOL TTGetOpenFileNameW(const TTOPENFILENAMEW *ofn, wchar_t **filename);
BOOL TTGetSaveFileNameW(const TTOPENFILENAMEW *ofn, wchar_t **filename);

typedef struct _TTbrowseinfoW {
	HWND	hwndOwner;
	LPCWSTR lpszTitle;
	UINT	ulFlags;
} TTBROWSEINFOW;

BOOL TTSHBrowseForFolderW(const TTBROWSEINFOW *bi, const wchar_t *def, wchar_t **folder);

DllExport BOOL doSelectFolder(HWND hWnd, char *path, int pathlen, const char *def, const char *msg);
BOOL doSelectFolderW(HWND hWnd, const wchar_t *def, const wchar_t *msg, wchar_t **folder);
void OpenFontFolder(void);

#ifdef __cplusplus
}
#endif
