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

/* ttlib_static_cpp から分離 */
/* フォルダに関する関数、*/
/* cyglaunchで使う TTWinExec() */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include <wchar.h>
#include <shlobj.h>

// compat_win を利用する
//		cyglaunch の単独ビルドのとき、compatwin を使用せずビルドする
#if !defined(ENABLE_COMAPT_WIN)
#define ENABLE_COMAPT_WIN	1
#endif

#include "asprintf.h"
#include "win32helper.h"	// for hGetModuleFileNameW()
#include "ttknownfolders.h"

#if ENABLE_COMAPT_WIN
#include "compat_win.h"
#endif

#include "ttlib_static_dir.h"

// ポータブル版として動作するかを決めるファイル
#define PORTABLE_FILENAME L"portable.ini"

/**
 *	AppDataフォルダの取得
 *	環境変数 APPDATA のフォルダ
 *
 *	@retval	AppDataフォルダ
 */
#if ENABLE_COMAPT_WIN
static wchar_t *GetAppdataDir(void)
{
	wchar_t *path;
	_SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &path);
	return path;
}
#else
static wchar_t *GetAppdataDir(void)
{
#if _WIN32_WINNT > 0x0600
	wchar_t *appdata;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &appdata);
	wchar_t *retval = _wcsdup(appdata);
	CoTaskMemFree(appdata);
	return retval;
#else
	LPITEMIDLIST pidl;
	HRESULT r = SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
	if (r == NOERROR) {
		wchar_t appdata[MAX_PATH];
		SHGetPathFromIDListW(pidl, appdata);
		wchar_t *retval = wcsdup(appdata);
		CoTaskMemFree(pidl);
		return retval;
	}
	char *env = getenv("APPDATA");
	if (env == NULL) {
		// もっと古い windows ?
		abort();
	}
	wchar_t *appdata = ToWcharA(env);
	return appdata;
#endif
}
#endif

/*
 * Get Exe(exe,dll) directory
 *	ttermpro.exe, プラグインがあるフォルダ
 *	ttypes.ExeDirW と同一
 *	もとは GetHomeDirW() だった
 *
 * @param[in]		hInst		WinMain()の HINSTANCE または NULL
 * @return			ExeDir		不要になったら free() すること
 *								文字列の最後にパス区切り('\')はついていない
 */
wchar_t *GetExeDirW(HINSTANCE hInst)
{
	wchar_t *dir;
	DWORD error = hGetModuleFileNameW(hInst, &dir);
	if (error != NO_ERROR) {
		// パスの取得に失敗した。致命的、abort() する。
		abort();
	}
	wchar_t *sep = wcsrchr(dir, L'\\');
	*sep = 0;
	return dir;
}

/**
 *	ポータブル版として動作するか
 *
 *	@retval		TRUE		ポータブル版
 *	@retval		FALSE		通常インストール版
 */
BOOL IsPortableMode(void)
{
	static BOOL called = FALSE;
	static BOOL ret_val = FALSE;
	if (called == FALSE) {
		called = TRUE;
		wchar_t *exe_dir = GetExeDirW(NULL);
		wchar_t *portable_ini = NULL;
		awcscats(&portable_ini, exe_dir, L"\\", PORTABLE_FILENAME, NULL);
		free(exe_dir);
		DWORD r = GetFileAttributesW(portable_ini);
		free(portable_ini);
		if (r == INVALID_FILE_ATTRIBUTES) {
			//ファイルが存在しない
			ret_val = FALSE;
		}
		else {
			ret_val = TRUE;
		}
	}
	return ret_val;
}

/*
 * Get home directory
 *		個人用設定ファイルフォルダ取得
 *		ttypes.HomeDirW と同一
 *		TERATERM.INI などがおいてあるフォルダ
 *		ttermpro.exe があるフォルダは GetHomeDirW() ではなく GetExeDirW() で取得できる
 *		ExeDirW に portable.ini がある場合
 *			ExeDirW
 *		ExeDirW に portable.ini がない場合
 *			%APPDATA%\teraterm5 (%USERPROFILE%\AppData\Roaming\teraterm5)
 *
 * @param[in]		hInst		WinMain()の HINSTANCE または NULL
 * @return			HomeDir		不要になったら free() すること
 *								文字列の最後にパス区切り('\')はついていない
 */
wchar_t *GetHomeDirW(HINSTANCE hInst)
{
	if (IsPortableMode()) {
		return GetExeDirW(hInst);
	}
	else {
		wchar_t *path = GetAppdataDir();
		wchar_t *ret = NULL;
		awcscats(&ret, path, L"\\teraterm5", NULL);
		free(path);
		CreateDirectoryW(ret, NULL);
		return ret;
	}
}

/*
 * Get log directory
 *		ログ保存フォルダ取得
 *		ttypes.LogDirW と同一
 *		ExeDirW に portable.ini がある場合
 *			ExeDirW\log
 *		ExeDirW に portable.ini がない場合
 *			%LOCALAPPDATA%\teraterm5 (%USERPROFILE%\AppData\Local\teraterm5)
 *
 * @param[in]		hInst		WinMain()の HINSTANCE または NULL
 * @return			LogDir		不要になったら free() すること
 *								文字列の最後にパス区切り('\')はついていない
 */
wchar_t* GetLogDirW(HINSTANCE hInst)
{
	wchar_t *ret = NULL;
	if (IsPortableMode()) {
		wchar_t *ExeDirW = GetExeDirW(hInst);
		awcscats(&ret, ExeDirW, L"\\log", NULL);
		free(ExeDirW);
	}
	else {
		wchar_t *path = GetAppdataDir();
		awcscats(&ret, path, L"\\teraterm5", NULL);
		free(path);
	}
	CreateDirectoryW(ret, NULL);
	return ret;
}

/**
 *	プログラムを実行する
 *
 *	@param[in]	command		実行するコマンドライン
 *							CreateProcess() にそのまま渡される
 * 	@retval		NO_ERROR	エラーなし
 *	@retval		エラーコード	(NO_ERROR以外)
 *
 *	シンプルにプログラムを起動するだけの関数
 *		CreateProcess() は CloseHandle() を忘れてハンドルリークを起こしやすい
 *		単純なプログラム実行ではこの関数を使用すると安全
 */
DWORD TTWinExec(const wchar_t *command)
{
	STARTUPINFOW si = {};
	PROCESS_INFORMATION pi = {};

	GetStartupInfoW(&si);

	BOOL r = CreateProcessW(NULL, (LPWSTR)command, NULL, NULL, FALSE, 0,
							NULL, NULL, &si, &pi);
	if (r == 0) {
		// error
		return GetLastError();
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return NO_ERROR;
}
