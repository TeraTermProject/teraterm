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
#include "compat_win.h"
#include "ttlib.h"

#include "ttcommdlg.h"

#define	NEW_DIALOG_ENABLE	1		// Vista+
#define	OLD_DIALOG_ENABLE	1

// IFileOpenDialogが使えない場合
#if (defined(__MINGW64_VERSION_MAJOR) && (__MINGW64_VERSION_MAJOR <= 8)) || \
	(_MSC_VER == 1400)
//		VS2005
//		MinGW <= 8.0
#undef	NEW_DIALOG_ENABLE
#define	NEW_DIALOG_ENABLE	0
#undef	OLD_DIALOG_ENABLE
#define	OLD_DIALOG_ENABLE	1
#endif

#if OLD_DIALOG_ENABLE
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
static BOOL TTSHBrowseForFolderWAPI(const TTBROWSEINFOW *bi, const wchar_t *def, wchar_t **folder)
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
	// TODO
	//	SHGetPathFromIDListEx() を win32help へ持っていく
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
#endif	// OLD_DIALOG_ENABLE

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
BOOL TTSHBrowseForFolderW(const TTBROWSEINFOW *bi, const wchar_t *def, wchar_t **folder)
{
#if	!NEW_DIALOG_ENABLE
	return TTSHBrowseForFolderWAPI(bi, def, folder);
#else
	IFileOpenDialog *pDialog;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void **)&pDialog);
	if (FAILED(hr)) {
#if OLD_DIALOG_ENABLE
		return TTSHBrowseForFolderWAPI(bi, def, folder);
#else
		return FALSE;
#endif
	}

	DWORD options;
	pDialog->GetOptions(&options);
	pDialog->SetOptions(options | FOS_PICKFOLDERS);
	pDialog->SetTitle(bi->lpszTitle);
	if (def != NULL) {
		IShellItem *psi;
		hr = pSHCreateItemFromParsingName(def, NULL, IID_IShellItem, (void **)&psi);
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

static BOOL GetOpenSaveFileNameWAPI(const TTOPENFILENAMEW *ofn, bool save, wchar_t **filename)
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

	if (init_dir == NULL && ofn->lpstrInitialDir != NULL) {
		// 初期フォルダ指定あり
		init_dir = _wcsdup(ofn->lpstrInitialDir);
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
	o.Flags |= (OFN_EXPLORER | OFN_ENABLESIZING);
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

#if NEW_DIALOG_ENABLE
static COMDLG_FILTERSPEC *CreateFilterSpec(const wchar_t *filter, UINT *count)
{
	if (filter == NULL) {
		*count = 0;
		return NULL;
	}
	int n = 0;
	const wchar_t *p = filter;
	while (*p != 0) {
		p += wcslen(p);
		p++;
		p += wcslen(p);
		p++;
		if (p - filter > 32*1024) {
			// 何かおかしい
			*count = 0;
			return NULL;
		}
		n++;
	}
	*count = n;

	COMDLG_FILTERSPEC *spec = (COMDLG_FILTERSPEC *)malloc(sizeof(COMDLG_FILTERSPEC) * n);
	p = filter;
	for (int i = 0; i < n; i++) {
		spec[i].pszName = p;
		p += wcslen(p);
		p++;
		spec[i].pszSpec = p;
		p += wcslen(p);
		p++;
	}
	return spec;
}
#endif // NEW_DIALOG_ENABLE

static BOOL GetOpenSaveFileNameW(const TTOPENFILENAMEW *ofn, bool save, wchar_t **filename)
{
#if !NEW_DIALOG_ENABLE
	return GetOpenSaveFileNameWAPI(ofn, save, filename);
#else
	HRESULT hr = 0;
	IFileDialog *pFile;
	REFCLSID clsid = save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
	hr = CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_IFileDialog, (void**)&pFile);
	if (FAILED(hr)) {
#if OLD_DIALOG_ENABLE
		return GetOpenSaveFileNameWAPI(ofn, save, filename);
#else
		return FALSE;
#endif
	}

	DWORD options;
	pFile->GetOptions(&options);
	// 呼び出し側で未設定ならばクリア
	if (!(OFN_OVERWRITEPROMPT & ofn->Flags)) {
		options &= ~FOS_OVERWRITEPROMPT;
	}
	pFile->SetOptions(options | FOS_NOCHANGEDIR);

	// 初期フォルダ
	wchar_t* init_dir = NULL;
	if (ofn->lpstrFile != NULL) {
		// 初期ファイル指定あり
		if (!IsRelativePathW(ofn->lpstrFile)) {
			// 初期ファイルが絶対パスならパスを取り出して初期フォルダにする
			init_dir = _wcsdup(ofn->lpstrFile);
			wchar_t* p = wcsrchr(init_dir, L'\\');
			if (p != NULL) {
				*p = L'\0';
			}
		}
	}
	if (init_dir == NULL && ofn->lpstrInitialDir != NULL) {
		// 初期フォルダ指定あり
		init_dir = _wcsdup(ofn->lpstrInitialDir);
	}
	if (init_dir != NULL) {
		IShellItem* psi;
		hr = pSHCreateItemFromParsingName(init_dir, NULL, IID_IShellItem, (void**)&psi);
		if (SUCCEEDED(hr)) {
			hr = pFile->SetFolder(psi);
			psi->Release();
		}
		free(init_dir);
		init_dir = NULL;
	}

	// タイトル
	if (ofn->lpstrTitle != NULL) {
		pFile->SetTitle(ofn->lpstrTitle);
	}

	// (最初から入力されている)ファイル名
	if (ofn->lpstrFile != NULL) {
		// フルパスでいれるとだめらしい
		const wchar_t *base = wcsrchr(ofn->lpstrFile, L'\\');
		if (base != NULL) {
			base++;
		}
		else {
			// パス区切りがない
			base = ofn->lpstrFile;
		}
		pFile->SetFileName(base);
	}

	if (ofn->lpstrFilter != NULL) {
		UINT count;
		COMDLG_FILTERSPEC *filter_spec = CreateFilterSpec(ofn->lpstrFilter, &count);
		pFile->SetFileTypes(count, filter_spec);
		pFile->SetFileTypeIndex(ofn->nFilterIndex);
		free(filter_spec);
	}

	if (ofn->lpstrDefExt != NULL) {
		pFile->SetDefaultExtension(ofn->lpstrDefExt);
	}

	BOOL result = FALSE;
	hr = pFile->Show(ofn->hwndOwner);
	if (SUCCEEDED(hr)) {
		IShellItem *pItem;
		hr = pFile->GetResult(&pItem);
		if (SUCCEEDED(hr)) {
			PWSTR pFilename;
			hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pFilename);
			if (SUCCEEDED(hr)) {
				*filename = _wcsdup(pFilename);
				CoTaskMemFree(pFilename);
				result = TRUE;
			}
			pItem->Release();
		}
	}
	pFile->Release();

	if (!result) {
		*filename = NULL;
	}
	return result;
#endif
}

/**
 *	GetOpenFileNameW() 互換関数
 *	異なる点
 *		- フォルダが変更されない
 *		- 初期フォルダ設定
 *			- 初期ファイルのフルパスを初期フォルダにする
 *		- OFN_SHOWHELP をサポートしていない
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

void OpenFontFolder(void)
{
	HINSTANCE result = ShellExecuteA(NULL, "open", "ms-settings:fonts", NULL, NULL, SW_SHOWNORMAL);
	if (result <= (HINSTANCE)32) {
		wchar_t *font_folder;
		HRESULT r = _SHGetKnownFolderPath(FOLDERID_Fonts, KF_FLAG_DEFAULT, NULL, &font_folder);
		if (r == S_OK) {
			ShellExecuteW(NULL, L"explore", font_folder, NULL, NULL, SW_NORMAL);
			free(font_folder);
		}
	}
}
