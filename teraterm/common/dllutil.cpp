/*
 * (C) 2019- TeraTerm Project
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
#include <assert.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <crtdbg.h>

#include "dllutil.h"

typedef struct {
	const wchar_t *fname;
	DLLLoadFlag LoadFlag;
	HMODULE handle;
	int refCount;
} HandleList_t;

static HandleList_t *HandleList;
static int HandleListCount;

static HMODULE GetHandle(const wchar_t *fname, DLLLoadFlag LoadFlag)
{
	wchar_t dllPath[MAX_PATH];
	HMODULE module;
	int i;
	HandleList_t *p;
	int r;

	if (LoadFlag == DLL_GET_MODULE_HANDLE) {
		module = GetModuleHandleW(fname);
		assert(module != NULL);
		return module;
	}

	// 以前にロードした?
	p = HandleList;
	for (i = 0; i < HandleListCount; i++) {
		if (wcscmp(p->fname, fname) == 0) {
			p->refCount++;
			return p->handle;
		}
		p++;
	}

	// 新たにロードする
	dllPath[0] = 0;
	switch (LoadFlag) {
	case DLL_LOAD_LIBRARY_SYSTEM:
		r = GetSystemDirectoryW(dllPath, _countof(dllPath));
		assert(r != 0);
		if (r == 0) return NULL;
		break;
	case DLL_LOAD_LIBRARY_CURRENT:
		r = GetModuleFileNameW(NULL, dllPath, _countof(dllPath));
		assert(r != 0);
		if (r == 0) return NULL;
		*wcsrchr(dllPath, L'\\') = 0;
		break;
	default:
		return NULL;
	}
	wcscat_s(dllPath, _countof(dllPath), L"\\");
	wcscat_s(dllPath, _countof(dllPath), fname);
	module = LoadLibraryW(dllPath);
	if (module == NULL) {
		// 存在しない,dllじゃない?
		return NULL;
	}

	// リストに追加
	HandleListCount++;
	HandleList = (HandleList_t *)realloc(HandleList, sizeof(HandleList_t)*HandleListCount);
	p = &HandleList[i];
	p->fname = _wcsdup(fname);
	p->handle = module;
	p->LoadFlag = LoadFlag;
	p->refCount = 1;
	return module;
}

static void DLLFree(HandleList_t *p)
{
	if (p->LoadFlag != DLL_GET_MODULE_HANDLE) {
		FreeLibrary(p->handle);
	}
	free((void *)p->fname);
}

static void DLLFreeFromList(int no)
{
	HandleList_t *p = &HandleList[no];
	p->refCount--;
	if (p->refCount > 0) {
		return;
	}

	// 開放する
	DLLFree(p);
	memcpy(p, p+1, sizeof(*p) + (HandleListCount - no - 1));
	HandleListCount--;
	HandleList = (HandleList_t *)realloc(HandleList, sizeof(HandleList_t)*HandleListCount);
}

/**
 *	dllをアンロードする
 *
 *	@param[in]	handle
 */
void DLLFreeByHandle(HANDLE handle)
{
	// リストから探す
	HandleList_t *p = HandleList;
	for (int i = 0; i < HandleListCount; i++) {
		if (p->handle == handle) {
			// 見つかった、削除
			DLLFreeFromList(i);
			return;
		}
		p++;
	}

	// リストになかった
}

/**
 *	dllをアンロードする
 *
 *	@param[in]	fname	ファイル名
 */
void DLLFreeByFileName(const wchar_t *fname)
{
	// リストから探す
	HandleList_t *p = HandleList;
	for (int i = 0; i < HandleListCount; i++) {
		if (wcscmp(p->fname, fname) == 0) {
			// 見つかった、削除
			DLLFreeFromList(i);
			return;
		}
		p++;
	}

	// リストになかった
}

/**
 * DLL内の関数へのアドレスを取得する
 * @param[in]		fname		DLLファイル名
 * @param[in] 		FuncFlag	関数が見つからないときの動作
 *					DLL_ACCEPT_NOT_EXIST	見つからなくてもok
 *					DLL_ERROR_NOT_EXIST		見つからない場合エラー
 * @param[in]		ApiName		API名
 * @param[in,out]	pFunc		関数へのアドレス
 *								見つからない時はNULLが代入される
 * @retval	NO_ERROR				エラーなし
 * @retval	ERROR_FILE_NOT_FOUND	DLLが見つからない(不正なファイル)
 * @retval	ERROR_PROC_NOT_FOUND	関数エントリが見つからない
 */
DWORD DLLGetApiAddress(const wchar_t *fname, DLLLoadFlag LoadFlag,
					   const char *ApiName, void **pFunc)
{
	HMODULE hDll = GetHandle(fname, LoadFlag);
	if (hDll == NULL) {
		*pFunc = NULL;
		return ERROR_FILE_NOT_FOUND;
	} else {
		*pFunc = (void *)GetProcAddress(hDll, ApiName);
		if (*pFunc == NULL) {
			return ERROR_PROC_NOT_FOUND;
		}
		return NO_ERROR; // = 0
	}
}

/**
 * DLL内の複数の関数へのアドレスを取得する
 * @param[in] fname		dllのファイル名
 * @param[in] LoadFlag	dllのロード先
 *				DLL_GET_MODULE_HANDLE,
 *				DLL_LOAD_LIBRARY_SYSTEM,
 *				DLL_LOAD_LIBRARY_CURRENT,
 * @param[in] FuncFlag	関数が見つからないときの動作
 * @param[out] handle	DLLハンドル
 *				NULLのときは値を返さない
 * @retval	NO_ERROR				エラーなし
 * @retval	ERROR_FILE_NOT_FOUND	DLLが見つからない(不正なファイル)
 * @retval	ERROR_PROC_NOT_FOUND	関数エントリが見つからない
 */
DWORD DLLGetApiAddressFromList(const wchar_t *fname, DLLLoadFlag LoadFlag,
							   DLLFuncFlag FuncFlag, const APIInfo *ApiInfo, HANDLE *handle)
{
	HMODULE hDll = GetHandle(fname, LoadFlag);
	if (hDll == NULL) {
		while(ApiInfo->ApiName != NULL) {
			void **func = ApiInfo->func;
			*func = NULL;
			ApiInfo++;
		}
		if (handle != NULL) {
			*handle = NULL;
		}
		return ERROR_FILE_NOT_FOUND;
	} else {
		BOOL exist_all = TRUE;
		const APIInfo *p = ApiInfo;

		// アドレス取得
		while(p->ApiName != NULL) {
			void **func = p->func;
			*func = (void *)GetProcAddress(hDll, p->ApiName);
			if (*func == NULL) {
				exist_all = FALSE;
			}
			p++;
		}

		// すべて見つかった or 見つからないAPIがあってもok
		if (exist_all || FuncFlag == DLL_ACCEPT_NOT_EXIST) {
			if (handle != NULL) {
				*handle = hDll;
			}
			return NO_ERROR;
		}

		// 見つからないAPIがあったのでエラー
		p = ApiInfo;
		while(p->ApiName != NULL) {
			void **func = p->func;
			*func = NULL;
			p++;
		}
		DLLFreeByFileName(fname);
		if (handle != NULL) {
			*handle = NULL;
		}
		return ERROR_PROC_NOT_FOUND;
	}
}

void DLLGetApiAddressFromLists(const DllInfo *dllInfos)
{
	while (dllInfos->DllName != NULL) {
		DLLGetApiAddressFromList(dllInfos->DllName,
								 dllInfos->LoadFlag,
								 dllInfos->FuncFlag,
								 dllInfos->APIInfoPtr, NULL);
		dllInfos++;
	}
}

static void SetupLoadLibraryPath(void)
{
	BOOL (WINAPI *pSetDefaultDllDirectories)(DWORD);
	BOOL (WINAPI *pSetDllDirectoryA)(LPCSTR);
	const wchar_t *kernel32 = L"kernel32.dll";

#if !defined(LOAD_LIBRARY_SEARCH_SYSTEM32)
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#endif

	// SetDefaultDllDirectories() が使える場合は、
	// 検索パスを %WINDOWS%\system32 のみに設定する
	DLLGetApiAddress(kernel32, DLL_GET_MODULE_HANDLE,
					 "SetDefaultDllDirectories", (void **)&pSetDefaultDllDirectories);
	if (pSetDefaultDllDirectories != NULL) {
		pSetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
		return;
	}

	// SetDefaultDllDirectories() が使えなくても
	// SetDllDirectory() が使える場合は
	// カレントディレクトリだけでも検索パスからはずしておく。
	// カレントを外す(""をセットする)だけなので、ANSI版でok
	DLLGetApiAddress(kernel32, DLL_GET_MODULE_HANDLE,
					 "SetDllDirectoryA", (void **)&pSetDllDirectoryA);
	if (pSetDllDirectoryA != NULL) {
		pSetDllDirectoryA("");
	}
}

void DLLInit()
{
	if (HandleListCount != 0) {
		return;		// 初期化済み
	}
	HandleList = NULL;
	HandleListCount = 0;
	SetupLoadLibraryPath();
}

void DLLExit()
{
	int i;
	if (HandleListCount == 0) {
		return;		// 未使用
	}
	for (i = 0; i < HandleListCount; i++) {
		HandleList_t *p = &HandleList[i];
		DLLFree(p);
	}
	free(HandleList);
	HandleList = NULL;
	HandleListCount = 0;
}
