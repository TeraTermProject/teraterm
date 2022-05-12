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
#include <assert.h>

#include <shlobj.h>
#include <windows.h>
#include <wchar.h>
#include <htmlhelp.h>

#include "tt_res.h"
#include "vtwin.h"
#include "compat_win.h"
#include "codeconv.h"
#include "asprintf.h"
#include "helpid.h"
#include "win32helper.h"

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
	lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
						 "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
						 0, KEY_QUERY_VALUE, &hKey);
	if (lRet == ERROR_SUCCESS) {
		dwDataSize = sizeof(lpData) / sizeof(lpData[0]);
		lRet = RegQueryValueExA(
			hKey,
			"EnableLUA",
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
static BOOL openFileWithApplication(const wchar_t *filename, const char *editor, const wchar_t *UILanguageFile)
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
		TTMessageBoxW(NULL, &info, UILanguageFile, no);

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
		TTMessageBoxW(NULL, &info, UILanguageFile, no);

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

/**
 *	エクスプローラで指定ファイルのフォルダを開く
 *	ファイルが存在する場合はファイルを選択した状態で開く
 *	ファイルが存在しない場合はフォルダを開く
 *
 *	@param	file	ファイル
 *	@retval TRUE: success
 *	@retval	FALSE: failure
 */
static BOOL openDirectoryWithExplorer(const wchar_t *file, const wchar_t *UILanguageFile)
{
	BOOL ret;

	DWORD attr = GetFileAttributesW(file);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		// ファイルが存在しない, ディレクトリをオープンする
		wchar_t *dir = ExtractDirNameW(file);
		attr = GetFileAttributesW(dir);
		if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			// フォルダを開く
			INT_PTR h = (INT_PTR)ShellExecuteW(NULL, L"open", L"explorer.exe", dir, NULL, SW_NORMAL);
			ret = h > 32 ? TRUE : FALSE;
		}
		else {
			// ファイルもフォルダも存在しない
			DWORD no = GetLastError();
			static const TTMessageBoxInfoW info = {
				"Tera Term",
				"MSG_ERROR", L"ERROR",
				"DLG_SETUPDIR_NOFILE_ERROR", L"File does not exist.(%d)",
				MB_OK | MB_ICONWARNING
			};
			TTMessageBoxW(NULL, &info, UILanguageFile, no);
			ret = FALSE;
		}
		free(dir);
	} else if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		// 指定されたのがフォルダだった、フォルダを開く
		INT_PTR h = (INT_PTR)ShellExecuteW(NULL, L"open", L"explorer.exe", file, NULL, SW_NORMAL);
		ret = h > 32 ? TRUE : FALSE;
	} else {
		// フォルダを開く + ファイル選択
		wchar_t *param;
		aswprintf(&param, L"/select,%s", file);
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


	// Virtual Storeパスを作る。
	wchar_t *local_appdata;
	_SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &local_appdata);
	wchar_t *vs_file;
	aswprintf(&vs_file, L"%s\\VirtualStore%s\\%s", local_appdata, path_nodrive, file);
	free(local_appdata);

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

/**
 *	リスト定義用構造体
 */
typedef enum {
	LIST_PARAM_STR = 1,			// 文字列
	LIST_PARAM_FUNC = 2,		// 関数
} SetupListParam;
typedef struct {
	const char *key;			// テキストのキー
	const wchar_t *def_text;	// デフォルトテキスト
	SetupListParam param;		// param_ptr の内容を示す
	void *param_ptr;			// paramが示す値
	void *data_ptr;				// 関数毎に使用するデータ
} SetupList;

/**
 *	メニューを出して選択された処理を実行する
 */
static void PopupAndExec(HWND hWnd, const POINT *pointer_pos, const wchar_t *path, const TTTSet *pts)
{
	const wchar_t *UILanguageFile = pts->UILanguageFileW;
	const DWORD file_stat = GetFileAttributesW(path);
	const BOOL dir = (file_stat & FILE_ATTRIBUTE_DIRECTORY) != 0 ? TRUE : FALSE;

	HMENU hMenu= CreatePopupMenu();
	AppendMenuW(hMenu, (dir ? MF_DISABLED : MF_ENABLED) | MF_STRING | 0, 1, L"&Open file");
	AppendMenuW(hMenu, MF_ENABLED | MF_STRING | 0, 2, L"Open folder(with explorer)");
	AppendMenuW(hMenu, MF_ENABLED | MF_STRING | 0, 3, L"Send path to clipboard");
	int result = TrackPopupMenu(hMenu, TPM_RETURNCMD, pointer_pos->x, pointer_pos->y, 0 , hWnd, NULL);
	DestroyMenu(hMenu);
	switch (result) {
	case 1: {
		// アプリで開く
		const char *editor = pts->ViewlogEditor;
		openFileWithApplication(path, editor, UILanguageFile);
		break;
	}
	case 2: {
		// フォルダを開いて、ファイルを選択する
		openDirectoryWithExplorer(path, UILanguageFile);
		break;
	}
	case 3: {
		// クリップボードへ文字列送信
		CBSetTextW(hWnd, path, 0);
		break;
	}
	default:
		break;
	}
}

static wchar_t *GetCygtermIni(const SetupList *list, const TTTSet *pts)
{
	wchar_t *cygterm_ini;
	aswprintf(&cygterm_ini, L"%s\\cygterm.cfg", pts->HomeDirW);
	if (list->data_ptr != 0) {
		wchar_t *virtual_store_path;
		BOOL ret = convertVirtualStoreW(cygterm_ini, &virtual_store_path);
		free(cygterm_ini);
		if (ret) {
			return virtual_store_path;
		} else {
			return NULL;
		}
	}
	return cygterm_ini;
}

static wchar_t *GetTTXSSHKwnownHostFile(const SetupList *list, const TTTSet *)
{
	HMODULE h = GetModuleHandle("ttxssh.dll");
	if (h == NULL) {
		return NULL;
	}

	size_t (CALLBACK *func)(wchar_t *, size_t) = NULL;
	void **pfunc = (void **)&func;
	*pfunc = (void *)GetProcAddress(h, "TTXReadKnownHostsFile");
	if (func == NULL) {
		return NULL;
	}

	size_t size = func(NULL, 0);
	if (size == 0) {
		return NULL;
	}

	wchar_t *temp = (wchar_t *)malloc(sizeof(wchar_t) * size);
	func(temp, size);
	assert(!IsRelativePathW(temp));

	if (list->data_ptr != 0) {
		wchar_t *virtual_store_path;
		BOOL ret = convertVirtualStoreW(temp, &virtual_store_path);
		if (ret) {
			return virtual_store_path;
		} else {
			return NULL;
		}
	}

	return temp;
}

/**
 *	Virtaul Storeへのパスを返す
 */
static wchar_t *GetVirtualStorePath(const SetupList *list, const TTTSet *)
{
	const wchar_t *path = (wchar_t *)list->data_ptr;
	wchar_t *virtual_store_path;
	BOOL ret = convertVirtualStoreW(path, &virtual_store_path);
	if (ret) {
		return virtual_store_path;
	} else {
		// virtual storeは使用されていない
		return NULL;
	}
}

/**
 *	Susie plug-in パス
 */
static wchar_t *GetSusiePluginPath(const SetupList *, const TTTSet *pts)
{
	const char *spi_path = pts->EtermLookfeel.BGSPIPath;
	wchar_t *path;
	aswprintf(&path, L"%s\\%hs", pts->ExeDirW, spi_path);
	return path;
}

/**
 *	Susie plug-in パス
 */
static wchar_t *GetCurrentPath(const SetupList *, const TTTSet *)
{
	wchar_t *path;
	hGetCurrentDirectoryW(&path);
	return path;
}

static INT_PTR CALLBACK OnSetupDirectoryDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_SETUPDIR_TITLE" },
	};
	TTTSet *pts = (TTTSet *)GetWindowLongPtr(hDlgWnd, DWLP_USER);

	switch (msg) {
	case WM_INITDIALOG: {
		pts = (TTTSet *)lp;
		SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)pts);

		// I18N
		SetDlgTextsW(hDlgWnd, TextInfos, _countof(TextInfos), pts->UILanguageFileW);

		HWND hWndList = GetDlgItem(hDlgWnd, IDC_SETUP_DIR_LIST);
		ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

		LV_COLUMNA lvcol;
		lvcol.mask = LVCF_TEXT | LVCF_SUBITEM;
		lvcol.pszText = (LPSTR)"name";
		lvcol.iSubItem = 0;
		//ListView_InsertColumn(hWndList, 0, &lvcol);
		SendMessage(hWndList, LVM_INSERTCOLUMNA, 0, (LPARAM)&lvcol);

		lvcol.pszText = (LPSTR)"path/file";
		lvcol.iSubItem = 1;
		//ListView_InsertColumn(hWndList, 1, &lvcol);
		SendMessage(hWndList, LVM_INSERTCOLUMNA, 1, (LPARAM)&lvcol);

		// 設定一覧
		static const SetupList setup_list[] = {
			{ "DLG_SETUPDIR_INIFILE", L"Tera Term Configuration File",
			  LIST_PARAM_STR, pts->SetupFNameW, },
			{ NULL, L"  Virtual Store",
			  LIST_PARAM_FUNC, (void *)GetVirtualStorePath, pts->SetupFNameW },
			{ "DLG_SETUPDIR_KEYBOARDFILE", L"Keyboard Configuration File",
			  LIST_PARAM_STR, pts->KeyCnfFNW },
			{ NULL, L"  Virtual Store",
			  LIST_PARAM_FUNC, (void*)GetVirtualStorePath, pts->KeyCnfFNW },
			{ "DLG_SETUPDIR_CYGTERMFILE", L"CygTerm Configuration File",
			  LIST_PARAM_FUNC, (void*)GetCygtermIni, (void *)0 },
			{ NULL, L"  Virtual Store",
			  LIST_PARAM_FUNC, (void*)GetCygtermIni, (void *)1 },
			{ "DLG_SETUPDIR_KNOWNHOSTSFILE", L"SSH known_hosts File",
			  LIST_PARAM_FUNC, (void*)GetTTXSSHKwnownHostFile, (void *)0 },
			{ NULL, L"  Virtual Store",
			  LIST_PARAM_FUNC, (void*)GetTTXSSHKwnownHostFile, (void *)1 },
			{ NULL, L"CurrentDirectry",
			  LIST_PARAM_FUNC, (void*)GetCurrentPath },
			{ NULL, L"HomeDir",
			  LIST_PARAM_STR, pts->HomeDirW },
			{ NULL, L"ExeDir",
			  LIST_PARAM_STR, pts->ExeDirW },
			{ NULL, L"LogDir",
			  LIST_PARAM_STR, pts->LogDirW },
			{ NULL, L"LogDefaultPathW",
			  LIST_PARAM_STR, pts->LogDefaultPathW },
			{ NULL, L"Download",
			  LIST_PARAM_STR, pts->FileDirW },
			{ NULL, L"Susie Plugin Path",
			  LIST_PARAM_FUNC, (void*)GetSusiePluginPath },
			{ NULL, L"UI language file",
			  LIST_PARAM_STR, pts->UILanguageFileW },
		};

		int y = 0;
		for (const SetupList *list = &setup_list[0]; list != &setup_list[_countof(setup_list)]; list++) {

			const SetupListParam param = list->param;
			wchar_t *s;
			if (param & LIST_PARAM_STR) {
				// 文字列へのポインタ
				s = (wchar_t *)list->param_ptr;
			} else if (param & LIST_PARAM_FUNC) {
				// 関数から文字列をもらう
				typedef wchar_t *(*func_t)(const SetupList *list, TTTSet *pts);
				func_t func = (func_t)list->param_ptr;
				s = func(list, pts);
			} else {
				assert("bad list?");
				s = NULL;
			}
			if (s == NULL) {
				// 表示不要
				continue;
			}

			const char *key = list->key;
			const wchar_t *def_text = list->def_text;
			wchar_t *text;
			GetI18nStrWW("Tera Term", key, def_text, pts->UILanguageFileW, &text);

			LVITEMW item;
			item.mask = LVIF_TEXT;
			item.iItem = y;
			item.iSubItem = 0;
			item.pszText = text;
			SendMessage(hWndList, LVM_INSERTITEMW, 0, (LPARAM)&item);
			free(text);

			item.mask = LVIF_TEXT;
			item.iItem = y;
			item.iSubItem = 1;
			item.pszText = s;
			SendMessage(hWndList, LVM_SETITEMW, 0, (LPARAM)&item);

			y++;

			if (param & LIST_PARAM_FUNC) {
				// 表示用にもらった文字列を開放
				free(s);
			}
		}

		// 幅を調整
		for (int i = 0; i < 2; i++) {
			ListView_SetColumnWidth(hWndList, i, LVSCW_AUTOSIZE);
		}

		CenterWindow(hDlgWnd, GetParent(hDlgWnd));

		return TRUE;
	}

	case WM_COMMAND: {
		switch (LOWORD(wp)) {
		case IDHELP:
			OpenHelp(HH_HELP_CONTEXT, HlpMenuSetupDir, pts->UILanguageFile);
			break;

		case IDOK:
			TTEndDialog(hDlgWnd, IDOK);
			return TRUE;
			break;

		case IDCANCEL:
			TTEndDialog(hDlgWnd, IDCANCEL);
			return TRUE;
			break;

		default:
			return FALSE;
		}
		return FALSE;
	}
	case WM_CLOSE:
		TTEndDialog(hDlgWnd, 0);
		return TRUE;

	case WM_NOTIFY: {
		NMHDR *nmhdr = (NMHDR *)lp;
		if (nmhdr->idFrom == IDC_SETUP_DIR_LIST) {
			NMLISTVIEW *nmlist = (NMLISTVIEW *)lp;
			switch (nmlist->hdr.code) {
//			case NM_CLICK:
			case NM_RCLICK: {
				POINT pointer_pos;
				GetCursorPos(&pointer_pos);
				LVHITTESTINFO ht = {};
				ht.pt = pointer_pos;
				ScreenToClient(nmlist->hdr.hwndFrom, &ht.pt);
				ListView_HitTest(nmlist->hdr.hwndFrom, &ht);
				if (ht.flags & LVHT_ONITEM) {
					int hit_item = ht.iItem;

					wchar_t path[MAX_PATH];
					LV_ITEMW item;
					item.mask = TVIF_TEXT;
					item.iItem = hit_item;
					item.iSubItem = 1;
					item.pszText = path;
					item.cchTextMax = _countof(path);
					SendMessageW(nmlist->hdr.hwndFrom, LVM_GETITEMW, 0, (LPARAM)&item);

					PopupAndExec(nmlist->hdr.hwndFrom, &pointer_pos, path, pts);
				}
				break;
			}
			}
		}
		break;
	}
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
