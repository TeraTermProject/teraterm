/*
 * Copyright (C) 2008- TeraTerm Project
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

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>
#include <shlobj.h>
#include <assert.h>

#include "win32helper.h"
#include "ttlib.h"

#include "ttcommdlg.h"

static int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch(uMsg) {
	case BFFM_INITIALIZED: {
		// 初期化時
		const wchar_t *folder = (wchar_t *)lpData;
		if (folder != NULL && folder[0] != 0) {
			// フォルダを選択状態にする
			//		フォルダが存在しないときは 0 が返ってくる
			SendMessageW(hwnd, BFFM_SETSELECTIONW, (WPARAM)TRUE, (LPARAM)folder);
		}
		break;
	}
	default:
		break;
	}
	return 0;
}

/**
 *	API版,古い
 */
static BOOL TTSHBrowseForFolderWAPI(TTBROWSEINFOW *bi, const wchar_t *def, wchar_t **folder)
{
	wchar_t buf[MAX_PATH];	// PIDL形式で受け取るのでダミー,一応MAX_PATH長で受ける
	BROWSEINFOW b = {};
	b.hwndOwner = bi->hwndOwner;
	b.pidlRoot = 0;	// 0 = from desktop
	b.pszDisplayName = buf;
	b.lpszTitle = bi->lpszTitle;
	b.ulFlags = bi->ulFlags;
	if (def != NULL && def[0] != 0) {
		if (GetFileAttributesW(def) == INVALID_FILE_ATTRIBUTES) {
			MessageBoxA(bi->hwndOwner, "Not found folder", "Tera Term", MB_OK | MB_ICONERROR);
		}
		else {
			b.lpfn = BrowseCallback;
			b.lParam = (LPARAM)def;
		}
	}
	LPITEMIDLIST pidl = SHBrowseForFolderW(&b);
	if (pidl == NULL) {
		*folder = NULL;
		return FALSE;
	}

	// PIDL形式の戻り値のファイルシステムのパスに変換
#if _MSC_VER > 1400
	// VS2005で使えるSDKにはSHGetPathFromIDListEx()が入っていない
	if (true) {
		size_t len = MAX_PATH / 2;
		wchar_t *path = NULL;
		do {
			wchar_t *p;
			len *= 2;
			if (len >= SHRT_MAX) {
				free(path);
				return FALSE;
			}
			p = (wchar_t *)realloc(path, len);
			if (p == NULL) {
				free(path);
				return FALSE;
			}
			path = p;
		} while (!SHGetPathFromIDListEx(pidl, path, (DWORD)len, GPFIDL_DEFAULT));
		*folder = path;
	}
	else
#endif
	{
		wchar_t buf[MAX_PATH];
		if (!SHGetPathFromIDListW(pidl, buf)) {
			return FALSE;
		}
		*folder = _wcsdup(buf);
	}
	CoTaskMemFree(pidl);
	return TRUE;
}

/**
 *	SHBrowseForFolderW() ほぼ互換関数
 *
 *	@param	TTBROWSEINFOW
 *		- BROWSEINFOW の代わりに TTBROWSEINFOW を使う
 *		- 次のメンバがない
 *  	  - BROWSEINFOW.lpfn
 *		  - BROWSEINFOW.lParam
 *		  - BROWSEINFOW.iImage
 *		- folder 引数に選択フォルダが入る
 *	@param	def		デフォルトフォルダ
 *					指定しないときはNULL
 *	@param	folder	指定されたフォルダ
 *					不要になったら free() すること
 *					MAX_PATH上限なし
 */
BOOL TTSHBrowseForFolderW(TTBROWSEINFOW *bi, const wchar_t *def, wchar_t **folder)
{
#if _MSC_VER == 1400 // VS2005の場合
	// 2005で使えるSDKにはIFileOpenDialogがない
	return TTSHBrowseForFolderWAPI(bi, def, folder);
#else
	IFileOpenDialog *pDialog;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void **)&pDialog);
	if (FAILED(hr)) {
		// IFileOpenDialog が使えないOS, Vista以前
		return TTSHBrowseForFolderWAPI(bi, def, folder);
	}

	DWORD options;
	pDialog->GetOptions(&options);
	pDialog->SetOptions(options | FOS_PICKFOLDERS);
	pDialog->SetTitle(bi->lpszTitle);
	{
		IShellItem *psi;
		hr = SHCreateItemFromParsingName(def, NULL, IID_IShellItem, (void **)&psi);
		if (SUCCEEDED(hr)) {
			hr = pDialog->SetFolder(psi);
			psi->Release();
		}
	}
	hr = pDialog->Show(bi->hwndOwner);

	BOOL result = FALSE;
	if (SUCCEEDED(hr)) {
		IShellItem *pItem;
		hr = pDialog->GetResult(&pItem);
		if (SUCCEEDED(hr)) {
			PWSTR pPath;
			hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);
			if (SUCCEEDED(hr)) {
				*folder = _wcsdup(pPath);
				CoTaskMemFree(pPath);
				result = TRUE;
			}
		}
	}

	if (!result) {
		// cancel(or some error)
		*folder = NULL;
	}
	pDialog->Release();
	return result;
#endif
}

/**
 *	フォルダを選択する
 *	SHBrowseForFolderW() をコールする
 *
 *	@param[in]	def			選択フォルダの初期値(特に指定しないときは NULL or "")
 *	@param[out]	**folder	選択したフォルダのフルパス(キャンセル時はセットされない)
 *							不要になったら free() すること(キャンセル時はfree()不要)
 *	@retval	TRUE	選択した
 *	@retval	FALSE	キャンセルした
 *
 */
BOOL doSelectFolderW(HWND hWnd, const wchar_t *def, const wchar_t *msg, wchar_t **folder)
{
	TTBROWSEINFOW bi = {};
	bi.hwndOwner = hWnd;
	bi.lpszTitle = msg;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_NEWDIALOGSTYLE;

	return TTSHBrowseForFolderW(&bi, def, folder);
}

static BOOL GetOpenSaveFileNameW(const TTOPENFILENAMEW *ofn, bool save, wchar_t **filename)
{
	// GetSaveFileNameW(), GetOpenFileNameW() がカレントフォルダを変更してしまうため
	wchar_t *curdir;
	hGetCurrentDirectoryW(&curdir);

	// 初期フォルダ
	wchar_t *init_dir = NULL;
	if (ofn->lpstrFile != NULL) {
		// 初期ファイル指定あり
		if (!IsRelativePathW(ofn->lpstrFile)) {
			// 初期ファイルが絶対パスならパスを取り出して初期フォルダにする
			init_dir = _wcsdup(ofn->lpstrFile);
			wchar_t *p = wcsrchr(init_dir, L'\\');
			if (p != NULL) {
				*p = L'\0';
			}
		}
	}
	else {
		if (ofn->lpstrInitialDir != NULL) {
			// 初期フォルダ指定あり
			init_dir = _wcsdup(ofn->lpstrInitialDir);
		}
	}

	wchar_t File[MAX_PATH];
	if (GetFileAttributesW(ofn->lpstrFile) != INVALID_FILE_ATTRIBUTES) {
		wcsncpy_s(File, _countof(File), ofn->lpstrFile, _TRUNCATE);
	}
	else {
		File[0] = 0;
	}

	OPENFILENAMEW o = {};
	o.lStructSize = get_OPENFILENAME_SIZEW();
	o.hwndOwner = ofn->hwndOwner;
	o.lpstrFilter = ofn->lpstrFilter;
	o.lpstrFile = File;
	o.nMaxFile = _countof(File);
	o.lpstrTitle = ofn->lpstrTitle;
	o.lpstrInitialDir = init_dir;
	o.Flags = ofn->Flags;
	BOOL ok = save ? GetSaveFileNameW(&o) : GetOpenFileNameW(&o);
#if defined(_DEBUG)
	if (!ok) {
		DWORD Err = GetLastError();
		DWORD DlgErr = CommDlgExtendedError();
		assert(Err == 0 && DlgErr == 0);
	}
#endif
	*filename = ok ? _wcsdup(File) : NULL;

	free(init_dir);
	SetCurrentDirectoryW(curdir);
	free(curdir);

	return ok;
}

/**
 *	GetOpenFileNameW() 互換関数
 *	異なる点
 *		- フォルダが変更されない
 *		- 初期フォルダ設定
 *			- 初期ファイルのフルパスを初期フォルダにする
 *
 *	@param	filename	選択されたファイル名(戻り値が TRUEの時)
 *						MAX_PATH制限なし、不要になったらfree()すること
 *	@retval	TRUE		okが押された
 *	@retval	FALSE		cancelが押された
 */
BOOL TTGetOpenFileNameW(const TTOPENFILENAMEW *ofn, wchar_t **filename)
{
	return GetOpenSaveFileNameW(ofn, false, filename);
}

BOOL TTGetSaveFileNameW(const TTOPENFILENAMEW *ofn, wchar_t **filename)
{
	return GetOpenSaveFileNameW(ofn, true, filename);
}
