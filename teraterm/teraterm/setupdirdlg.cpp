/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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

#include "teraterm.h"
#include "tttypes.h"
#include "tttypes_key.h"

#include "ttcommon.h"
#include "ttdialog.h"
#include "commlib.h"
#include "ttlib.h"
#include "dlglib.h"

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>

#include <shlobj.h>
#include <windows.h>
#include <wchar.h>
#include <htmlhelp.h>

#include "tt_res.h"
#include "vtwin.h"
#include "compat_win.h"
#include "codeconv.h"
#include "asprintf.h"

#include "setupdirdlg.h"

// Virtual Storeが有効であるかどうかを判別する。
//
// [Windows 95-XP]
// return FALSE (always)
//
// [Windows Vista-10]
// return TRUE:  Virtual Store Enabled
//        FALSE: Virtual Store Disabled or Unknown
//
static BOOL GetVirtualStoreEnvironment(void)
{
#if _MSC_VER == 1400  // VSC2005(VC8.0)
	typedef struct _TOKEN_ELEVATION {
		DWORD TokenIsElevated;
	} TOKEN_ELEVATION, *PTOKEN_ELEVATION;
	int TokenElevation = 20;
#endif
	BOOL ret = FALSE;
	int flag = 0;
	HANDLE          hToken;
	DWORD           dwLength;
	TOKEN_ELEVATION tokenElevation;
	LONG lRet;
	HKEY hKey;
	char lpData[256];
	DWORD dwDataSize;
	DWORD dwType;
	BYTE bValue;

	// Windows Vista以前は無視する。
	if (!IsWindowsVistaOrLater())
		goto error;

	// UACが有効かどうか。
	// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\SystemのEnableLUA(DWORD値)が0かどうかで判断できます(0はUAC無効、1はUAC有効)。
	flag = 0;
	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
		NULL, KEY_QUERY_VALUE, &hKey
		);
	if (lRet == ERROR_SUCCESS) {
		dwDataSize = sizeof(lpData) / sizeof(lpData[0]);
		lRet = RegQueryValueEx(
			hKey,
			TEXT("EnableLUA"),
			0,
			&dwType,
			(LPBYTE)lpData,
			&dwDataSize);
		if (lRet == ERROR_SUCCESS) {
			bValue = ((LPBYTE)lpData)[0];
			if (bValue == 1)
				// UACが有効の場合、Virtual Storeが働く。
				flag = 1;
		}
		RegCloseKey(hKey);
	}
	if (flag == 0)
		goto error;

	// UACが有効時、プロセスが管理者権限に昇格しているか。
	flag = 0;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_DEFAULT, &hToken)) {
		if (GetTokenInformation(hToken, (TOKEN_INFORMATION_CLASS)TokenElevation, &tokenElevation, sizeof(TOKEN_ELEVATION), &dwLength)) {
			// (0は昇格していない、非0は昇格している)。
			if (tokenElevation.TokenIsElevated == 0) {
				// 管理者権限を持っていなければ、Virtual Storeが働く。
				flag = 1;
			}
		}
		CloseHandle(hToken);
	}
	if (flag == 0)
		goto error;

	ret = TRUE;
	return (ret);

error:
	return (ret);
}

//
// 指定したアプリケーションでファイルを開く。
//
// return TRUE: success
//        FALSE: failure
//
static BOOL openFileWithApplication(const wchar_t *filename, const char *editor, const char *UILanguageFile)
{
	wchar_t *commandW = NULL;
	BOOL ret = FALSE;

	if (GetFileAttributesW(filename) == INVALID_FILE_ATTRIBUTES) {
		// ファイルが存在しない
		DWORD no = GetLastError();
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"DLG_SETUPDIR_NOFILE_ERROR", L"File does not exist.(%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxA(NULL, &info, UILanguageFile, no);

		goto error;
	}

	aswprintf(&commandW, L"%hs \"%s\"", editor, filename);

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	GetStartupInfoW(&si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcessW(NULL, commandW, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) == 0) {
		// 起動失敗
		DWORD no = GetLastError();
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"DLG_SETUPDIR_OPENFILE_ERROR", L"Cannot open file.(%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxA(NULL, &info, UILanguageFile, no);

		goto error;
	}
	else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	ret = TRUE;

error:;
	free(commandW);

	return (ret);
}

//
// エクスプローラでパスを開く。
//
// return TRUE: success
//        FALSE: failure
//
static BOOL openDirectoryWithExplorer(const wchar_t *path, const char *UILanguageFile)
{
	BOOL ret;

	const DWORD attr = GetFileAttributesW(path);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		// ファイルが存在しない
		DWORD no = GetLastError();
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"DLG_SETUPDIR_NOFILE_ERROR", L"File does not exist.(%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxA(NULL, &info, UILanguageFile, no);
		ret = FALSE;
	} else if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		// フォルダを開く
		INT_PTR h = (INT_PTR)ShellExecuteW(NULL, L"open", L"explorer.exe", path, NULL, SW_NORMAL);
		ret = h > 32 ? TRUE : FALSE;
	} else {
		// フォルダを開く + ファイル選択
		wchar_t *param;
		aswprintf(&param, L"/select,%s", path);
		INT_PTR h = (INT_PTR)ShellExecuteW(NULL, L"open", L"explorer.exe", param, NULL, SW_NORMAL);
		free(param);
		ret = h > 32 ? TRUE : FALSE;
	}
	return ret;
}

/**
 *	フルパスファイル名を Virtual Storeパスに変換する。
 *	@param[in]		filename			変換前のファイル名
 *	@param[out]		vstore_filename		Virtual Storeのファイル名
 *	@retval			TRUE	変換した
 *					FALSE	変換しなかった(Virtual Storeに保存されていない)
 */
static BOOL convertVirtualStoreW(const wchar_t *filename, wchar_t **vstore_filename)
{
	wchar_t *path = ExtractDirNameW(filename);
	wchar_t *file = ExtractFileNameW(filename);

	// 不要なドライブレターを除去する。
	// ドライブレターは一文字とは限らない点に注意。(1文字では?)
	wchar_t *path_nodrive = wcsstr(path, L":\\");
	if (path_nodrive == NULL) {
		// フルパスではない, VSを考慮しなくてもok
		free(path);
		free(file);
		return FALSE;
	}
	path_nodrive++;

	BOOL ret = FALSE;
	static const wchar_t* virstore_env[] = {
		L"ProgramFiles",
		L"ProgramData",
		L"SystemRoot",
		NULL
	};
	LPITEMIDLIST pidl;
	int CSIDL;
	wchar_t shPath[1024];
	wchar_t *vs_file;
	const wchar_t** p = virstore_env;

	if (GetVirtualStoreEnvironment() == FALSE)
		goto error;

	// Virtual Store対象となるフォルダか。
	while (*p) {
		const wchar_t *s = _wgetenv(*p);
		if (s != NULL && wcsstr(path, s) != NULL) {
			break;
		}
		p++;
	}
	if (*p == NULL)
		goto error;

	CSIDL = CSIDL_LOCAL_APPDATA;
	if (SHGetSpecialFolderLocation(NULL, CSIDL, &pidl) != S_OK) {
		goto error;
	}
	SHGetPathFromIDListW(pidl, shPath);
	CoTaskMemFree(pidl);

	// Virtual Storeパスを作る。
	aswprintf(&vs_file, L"%s\\VirtualStore%s", shPath, path_nodrive, file);

	// 最後に、Virtual Storeにファイルがあるかどうかを調べる。
	if (GetFileAttributesW(vs_file) == INVALID_FILE_ATTRIBUTES) {
		free(vs_file);
		goto error;
	}

	*vstore_filename = vs_file;

	ret = TRUE;
	goto epilogue;

error:
	*vstore_filename = NULL;
	ret = FALSE;
epilogue:
	free(path);
	free(file);
	return ret;
}

static wchar_t *GetWindowTextAlloc(HWND hWnd)
{
	size_t length = GetWindowTextLengthW(hWnd);
	length++;
	wchar_t *buf = (wchar_t *)malloc(sizeof(wchar_t) * length);
	GetWindowTextW(hWnd, buf, (int)length);
	return buf;
}

static INT_PTR CALLBACK OnSetupDirectoryDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_SETUPDIR_TITLE" },
		{ IDC_INI_SETUPDIR_GROUP, "DLG_SETUPDIR_INIFILE" },
		{ IDC_KEYCNF_SETUPDIR_GROUP, "DLG_SETUPDIR_KEYBOARDFILE" },
		{ IDC_CYGTERM_SETUPDIR_GROUP, "DLG_SETUPDIR_CYGTERMFILE" },
		{ IDC_SSH_SETUPDIR_GROUP, "DLG_SETUPDIR_KNOWNHOSTSFILE" },
	};
	TTTSet *pts = (TTTSet *)GetWindowLongPtr(hDlgWnd, DWLP_USER);
	wchar_t *tmpbufW;
	HWND hWnd;

	switch (msg) {
	case WM_INITDIALOG: {
		BOOL ret;
		pts = (TTTSet *)lp;
		SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)pts);

		// I18N
		SetDlgTextsW(hDlgWnd, TextInfos, _countof(TextInfos), pts->UILanguageFileW);

		// 設定ファイル(teraterm.ini)のパスを取得する。
		/// (1)
		SetDlgItemTextW(hDlgWnd, IDC_INI_SETUPDIR_EDIT, pts->SetupFNameW);
		/// (2) Virutal Storeへの変換
		wchar_t *vs;
		ret = convertVirtualStoreW(pts->SetupFNameW, &vs);
		hWnd = GetDlgItem(hDlgWnd, IDC_INI_SETUPDIR_STATIC_VSTORE);
		EnableWindow(hWnd, ret);
		hWnd = GetDlgItem(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE);
		EnableWindow(hWnd, ret);
		if (ret) {
			SetDlgItemTextW(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE, vs);
			free(vs);
		}
		else {
			SetDlgItemTextA(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE, "");
		}

		// 設定ファイル(KEYBOARD.CNF)のパスを取得する。
		/// (1)
		SetDlgItemTextW(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT, pts->KeyCnfFNW);
		/// (2) Virutal Storeへの変換
		ret = convertVirtualStoreW(pts->KeyCnfFNW, &vs);
		hWnd = GetDlgItem(hDlgWnd, IDC_KEYCNF_SETUPDIR_STATIC_VSTORE);
		EnableWindow(hWnd, ret);
		hWnd = GetDlgItem(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE);
		EnableWindow(hWnd, ret);
		if (ret) {
			SetDlgItemTextW(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE, vs);
			free(vs);
		}
		else {
			SetDlgItemTextA(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE, "");
		}

		// cygterm.cfg は ttermpro.exe 配下に位置する。
		/// (1)
		aswprintf(&tmpbufW, L"%s\\cygterm.cfg", pts->HomeDirW);
		SetDlgItemTextW(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT, tmpbufW);
		/// (2) Virutal Storeへの変換
		ret = convertVirtualStoreW(tmpbufW, &vs);
		free(tmpbufW);
		hWnd = GetDlgItem(hDlgWnd, IDC_CYGTERM_SETUPDIR_STATIC_VSTORE);
		EnableWindow(hWnd, ret);
		hWnd = GetDlgItem(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE);
		EnableWindow(hWnd, ret);
		if (ret) {
			SetDlgItemTextW(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE, vs);
			free(vs);
		}
		else {
			SetDlgItemTextA(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE, "");
		}

		// ssh_known_hosts
		{
			typedef int (CALLBACK *PSSH_read_known_hosts_file)(char *, int);
			HMODULE h = NULL;
			PSSH_read_known_hosts_file func = NULL;
			if (((h = GetModuleHandle("ttxssh.dll")) != NULL)) {
				func = (PSSH_read_known_hosts_file)GetProcAddress(h, "TTXReadKnownHostsFile");
				if (func) {
					char temp[MAX_PATH];
					int ret = func(temp, sizeof(temp));
					if (ret) {
						char *s = strstr(temp, ":\\");

						if (s) { // full path
							tmpbufW = ToWcharA(temp);
						}
						else { // relative path
							aswprintf(&tmpbufW, L"%s\\%hs", pts->HomeDirW, temp);
						}

						SetDlgItemTextW(hDlgWnd, IDC_SSH_SETUPDIR_EDIT, tmpbufW);

						/// (2) Virutal Storeへの変換
						ret = convertVirtualStoreW(tmpbufW, &vs);
						free(tmpbufW);
						hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_STATIC_VSTORE);
						EnableWindow(hWnd, ret);
						hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE);
						EnableWindow(hWnd, ret);
						if (ret) {
							SetDlgItemTextW(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE, vs);
							free(vs);
						}
						else {
							SetDlgItemTextA(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE, "");
						}

					}
				}
			}
			else {
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT);
				EnableWindow(hWnd, FALSE);
				SetDlgItemTextA(hDlgWnd, IDC_SSH_SETUPDIR_EDIT, "");
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_BUTTON);
				EnableWindow(hWnd, FALSE);
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_BUTTON_FILE);
				EnableWindow(hWnd, FALSE);
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_STATIC_VSTORE);
				EnableWindow(hWnd, FALSE);
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE);
				EnableWindow(hWnd, FALSE);
				SetDlgItemTextA(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE, "");
			}
		}

		CenterWindow(hDlgWnd, GetParent(hDlgWnd));

		return TRUE;
	}

	case WM_COMMAND: {
		BOOL button_pressed = FALSE;
		BOOL open_dir = FALSE;
		int edit;
		int edit_vstore;
		switch (LOWORD(wp)) {
		case IDC_INI_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			edit = IDC_INI_SETUPDIR_EDIT;
			edit_vstore = IDC_INI_SETUPDIR_EDIT_VSTORE;
			open_dir = TRUE;
			button_pressed = TRUE;
			break;

		case IDC_INI_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			edit = IDC_INI_SETUPDIR_EDIT;
			edit_vstore = IDC_INI_SETUPDIR_EDIT_VSTORE;
			open_dir = FALSE;
			button_pressed = TRUE;
			break;

		case IDC_KEYCNF_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			edit = IDC_KEYCNF_SETUPDIR_EDIT;
			edit_vstore = IDC_KEYCNF_SETUPDIR_EDIT_VSTORE;
			open_dir = TRUE;
			button_pressed = TRUE;
			break;

		case IDC_KEYCNF_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			edit = IDC_KEYCNF_SETUPDIR_EDIT;
			edit_vstore = IDC_KEYCNF_SETUPDIR_EDIT_VSTORE;
			open_dir = FALSE;
			button_pressed = TRUE;
			break;

		case IDC_CYGTERM_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			edit = IDC_CYGTERM_SETUPDIR_EDIT;
			edit_vstore = IDC_CYGTERM_SETUPDIR_EDIT_VSTORE;
			open_dir = TRUE;
			button_pressed = TRUE;
			break;

		case IDC_CYGTERM_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			edit = IDC_CYGTERM_SETUPDIR_EDIT;
			edit_vstore = IDC_CYGTERM_SETUPDIR_EDIT_VSTORE;
			open_dir = FALSE;
			button_pressed = TRUE;
			break;

		case IDC_SSH_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			edit = IDC_SSH_SETUPDIR_EDIT;
			edit_vstore = IDC_SSH_SETUPDIR_EDIT_VSTORE;
			open_dir = TRUE;
			button_pressed = TRUE;
			break;

		case IDC_SSH_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			edit = IDC_SSH_SETUPDIR_EDIT;
			edit_vstore = IDC_SSH_SETUPDIR_EDIT_VSTORE;
			open_dir = FALSE;
			button_pressed = TRUE;
			break;

		case IDCANCEL:
			TTEndDialog(hDlgWnd, IDCANCEL);
			return TRUE;
			break;

		default:
			return FALSE;
		}

		if (button_pressed) {
			wchar_t *filename_p;
			if (!IsWindowEnabled(GetDlgItem(hDlgWnd, edit_vstore))) {
				filename_p = GetWindowTextAlloc(GetDlgItem(hDlgWnd, edit));
			} else {
				filename_p = GetWindowTextAlloc(GetDlgItem(hDlgWnd, edit_vstore));
			}

			const char *UILanguageFile = pts->UILanguageFile;
			if (open_dir) {
#if 0
				// フォルダを開く
				wchar_t *path_p = ExtractDirNameW(filename_p);
				openDirectoryWithExplorer(path_p, UILanguageFile);
				free(path_p);
#else
				// フォルダを開いて、ファイルを選択する
				openDirectoryWithExplorer(filename_p, UILanguageFile);
#endif
			}
			else {
				char *editor = pts->ViewlogEditor;
				openFileWithApplication(filename_p, editor, UILanguageFile);
			}

			free(filename_p);
			return TRUE;
		}
		return FALSE;
	}
	case WM_CLOSE:
		TTEndDialog(hDlgWnd, 0);
		return TRUE;

	default:
		return FALSE;
	}
	return TRUE;
}

void SetupDirectoryDialog(HINSTANCE hInst, HWND hWnd, TTTSet *pts)
{
	TTDialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SETUP_DIR_DIALOG),
					 hWnd, OnSetupDirectoryDlgProc, (LPARAM)pts);
}
