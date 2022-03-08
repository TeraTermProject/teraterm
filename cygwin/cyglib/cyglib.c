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
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <errno.h>

#include "ttlib_static_dir.h"
#include "asprintf.h"

#include "cyglib.h"

/**
 *	cygwin1.dll / msys-2.0.dllを探す
 *
 *	@param[in]	dll_base		dllファイル名
 *	@param[in]	cygwin_dir		(存在するであろう)フォルダ(*1)
 *	@param[in]	search_paths	(*1)が見つからなかったときに探すパス
 *	@param[out]	find_dir		見つかったフォルダ free() すること
 *	@param[out]	find_in_path	環境変数 PATH 内に見つかった
 *
 *	@retval		TRUE	見つかった
 *	@retval		FALSE	見つからない
 */
static BOOL SearchDLL(const wchar_t *dll_base, const wchar_t *cygwin_dir, const wchar_t **search_paths, wchar_t **find_dir, BOOL *find_in_path)
{
	wchar_t file[MAX_PATH];
	wchar_t *filename;
	wchar_t c;
	wchar_t *dll;
	int i;
	DWORD r;

	*find_in_path = FALSE;
	*find_dir = NULL;

	// 指定されたフォルダに存在するか?
	if (cygwin_dir != NULL && cygwin_dir[0] != 0) {
		// SearchPathW() で探す
		dll = NULL;
		awcscats(&dll, L"bin\\", dll_base, NULL);
		r = SearchPathW(cygwin_dir, dll, L".dll", _countof(file), file, &filename);
		free(dll);
		if (r > 0) {
			goto found_dll;
		}

		// SearchPathW() だと "msys-2.0.dll" が見つけることができない (Windows 10)
		dll = NULL;
		awcscats(&dll, cygwin_dir, L"\\bin\\", dll_base, L".dll", NULL);
		r = GetFileAttributesW(dll);
		if (r != INVALID_FILE_ATTRIBUTES) {
			// 見つかった
			wcscpy_s(file, _countof(file), dll);
			free(dll);
			goto found_dll;
		}
		free(dll);
	}

	// PATH から探す
	if (SearchPathW(NULL, dll_base, L".dll", _countof(file), file, &filename) > 0) {
		*find_in_path = TRUE;
		goto found_dll;
	}

	// ありそうな場所を探す
	for (c = 'C' ; c <= 'Z' ; c++) {
		for (i = 0; search_paths[i] != NULL; i++) {
			// SearchPathW() で探す
			const wchar_t *search_path_base = search_paths[i];
			wchar_t *search_path;
			aswprintf(&search_path, search_path_base, c);
			r = SearchPathW(search_path, dll_base, L".dll", _countof(file), file, &filename);
			if (r > 0) {
				free(search_path);
				goto found_dll;
			}

			// ファイルが存在するか調べる
			dll = NULL;
			awcscats(&dll, search_path, L"\\", dll_base, L".dll", NULL);
			r = GetFileAttributesW(dll);
			free(search_path);
			if (r != INVALID_FILE_ATTRIBUTES) {
				wcscpy_s(file, _countof(file), dll);
				free(dll);
				goto found_dll;
			}
			free(dll);
		}
	}

	// 見つからなかった
	return FALSE;

found_dll:
	{
		// cut "cygwin1.dll", フォルダのみを返す
		wchar_t *p = wcsrchr(file, L'\\');
		*p = 0;
	}

	*find_dir = _wcsdup(file);
	return TRUE;
}

static errno_t __wdupenv_s(wchar_t** envptr, size_t* buf_size, const wchar_t* name)
{
#if defined(_MSC_VER)
	return _wdupenv_s(envptr, buf_size, name);
#else
    const wchar_t* s = _wgetenv(name);
	if (s == NULL) {
		// 存在しない
		*envptr = NULL;
		return EINVAL;
	}
	*envptr = _wcsdup(s);
	if (buf_size != NULL) {
		*buf_size = wcslen(*envptr);
	}
	return 0;
#endif
}

/**
 *	環境変数 PATH に add_path を追加
 */
static BOOL AddPath(const wchar_t *add_path)
{
	wchar_t *envptr;
	wchar_t *new_env;
	int r;
	errno_t e;

	e = __wdupenv_s(&envptr, NULL, L"PATH");
	if (e == 0) {
		aswprintf(&new_env, L"PATH=%s;%s", add_path, envptr);
		free(envptr);
	}
	else {
		// 環境変数 PATH が存在しない
		aswprintf(&new_env, L"PATH=%s", add_path);
	}
	r = _wputenv(new_env);
	free(new_env);
	return r == 0 ? TRUE : FALSE;
}

/**
 *	Connect to local cygwin
 *	cygtermを実行
 *
 *	@param[in]	CygwinDirectory		Cygwinがインストールしてあるフォルダ
 *									見つからなければデフォルトフォルダなどを探す
 *	@param[in]	cmdline				cygtermに渡すコマンドライン引数
 *									NULLのとき引数なし
 *	@retval		NO_ERROR					実行できた
 *	@retval		ERROR_FILE_NOT_FOUND		cygwinが見つからない(cygwin1.dllが見つからない)
 *	@retval		ERROR_NOT_ENOUGH_MEMORY		メモリ不足
 *	@retval		ERROR_OPEN_FAILED			実行できない
 */
static DWORD Connect(const wchar_t *cygterm_exe, const wchar_t *dll_base, const wchar_t *CygwinDirectory, const wchar_t **search_paths, const wchar_t *cmdline)
{
	BOOL find_cygwin;
	wchar_t *find_dir;
	BOOL find_in_path;
	wchar_t *ExeDirW;
	wchar_t *cygterm_cmd;
	DWORD e;

	find_cygwin = SearchDLL(dll_base, CygwinDirectory, search_paths, &find_dir, &find_in_path);
	if (find_cygwin == FALSE) {
		return ERROR_FILE_NOT_FOUND;
	}

	if (!find_in_path) {
		// 環境変数 PATH に追加
		// cygterm.exe を実行するときに cygwin1.dll をロードできるようにする
		BOOL r = AddPath(find_dir);
		if (r == FALSE) {
			free(find_dir);
			return ERROR_NOT_ENOUGH_MEMORY;
		}
	}
	free(find_dir);

	ExeDirW = GetExeDirW(NULL);
	cygterm_cmd = NULL;
	awcscats(&cygterm_cmd, ExeDirW, L"\\", cygterm_exe, NULL);
	if (cmdline != NULL && cmdline[0] != 0) {
		awcscats(&cygterm_cmd, L" ", cmdline, NULL);
	}

	e = TTWinExec(cygterm_cmd);
	free(cygterm_cmd);
	if (e != NO_ERROR) {
		return ERROR_OPEN_FAILED;
	}

	return NO_ERROR;
}

/**
 *	Connect to local cygwin
 *	cygtermを実行
 *
 *	@param[in]	CygwinDirectory		Cygwinがインストールしてあるフォルダ
 *									見つからなければデフォルトフォルダなどを探す
 *	@param[in]	cmdline				cygtermに渡すコマンドライン引数
 *									NULLのとき引数なし
 *	@retval		NO_ERROR					実行できた
 *	@retval		ERROR_FILE_NOT_FOUND		cygwinが見つからない(cygwin1.dllが見つからない)
 *	@retval		ERROR_NOT_ENOUGH_MEMORY		メモリ不足
 *	@retval		ERROR_OPEN_FAILED			実行できない
 */
DWORD CygwinConnect(const wchar_t *CygwinDirectory, const wchar_t *cmdline)
{
	static const wchar_t *cygterm_exe = L"cygterm.exe";
	static const wchar_t *dll_base = L"cygwin1";
	static const wchar_t *search_paths[] = {
		L"%c:\\cygwin\\bin",
		L"%c:\\cygwin64\\bin",
		NULL,
	};

	return Connect(cygterm_exe, dll_base, CygwinDirectory, search_paths, cmdline);
}

DWORD Msys2Connect(const wchar_t *Msys2Directory, const wchar_t *cmdline)
{
	static const wchar_t *cygterm_exe = L"msys2term.exe";
	static const wchar_t *dll_base = L"msys-2.0";
	static const wchar_t *search_paths[] = {
		L"%c:\\msys\\usr\\bin",
		L"%c:\\msys64\\usr\\bin",
		NULL,
	};

	return Connect(cygterm_exe, dll_base, Msys2Directory, search_paths, cmdline);
}
