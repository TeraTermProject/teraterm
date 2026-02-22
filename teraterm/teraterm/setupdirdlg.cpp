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

#include "tttypes.h"
#include "ttcommon.h"
#include "ttdialog.h"
#include "ttlib.h"
#include "dlglib.h"
#include "tt_res.h"
#include "compat_win.h"
#include "asprintf.h"
#include "helpid.h"
#include "win32helper.h"
#include "tipwin2.h"
#include "scp.h"
#include "ttlib_types.h"
#include "resize_helper.h"

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

static wchar_t *GetExplorerFullPath(void)
{
	wchar_t *windows_dir;
	_SHGetKnownFolderPath(FOLDERID_Windows, 0, NULL, &windows_dir);
	wchar_t *explorer;
	aswprintf(&explorer, L"%s\\explorer.exe", windows_dir);
	free(windows_dir);
	return explorer;
}

//
// 指定したアプリケーションでファイルを開く。
//
// return TRUE: success
//        FALSE: failure
//
static BOOL openFileWithApplication(const wchar_t *filename,
									const wchar_t *editor, const wchar_t *arg,
									const wchar_t *UILanguageFile)
{
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

		return FALSE;
	}

	DWORD e = TTCreateProcess(editor, arg, filename);
	if (e != NO_ERROR) {
		// 起動失敗
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"DLG_SETUPDIR_OPENFILE_ERROR", L"Cannot open file.(%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(NULL, &info, UILanguageFile, e);
		return FALSE;
	}
	return TRUE;
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
		if ((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			// フォルダを開く
			INT_PTR h = (INT_PTR)ShellExecuteW(NULL, L"explore", dir, NULL, NULL, SW_NORMAL);
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
		INT_PTR h = (INT_PTR)ShellExecuteW(NULL, L"explore", file, NULL, NULL, SW_NORMAL);
		ret = h > 32 ? TRUE : FALSE;
	} else {
		// フォルダを開く + ファイル選択
		wchar_t *explorer = GetExplorerFullPath();
		wchar_t *param;
		aswprintf(&param, L"/select,%s", file);
		INT_PTR h = (INT_PTR)ShellExecuteW(NULL, L"open", explorer, param, NULL, SW_NORMAL);
		free(param);
		free(explorer);
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
 * @brief メニューを出して選択された処理を実行する
 * @param hWnd			親ウィンドウ
 * @param pointer_pos	ポインタの位置
 * @param path			フルパス,file or path
 * @param pts			*ts
 */
static void PopupAndExec(HWND hWnd, const POINT *pointer_pos, const wchar_t *path, const TTTSet *pts)
{
	const wchar_t *UILanguageFile = pts->UILanguageFileW;
	UINT menu_flag = (path == NULL || path[0] == 0) ? MF_DISABLED : MF_ENABLED;
	UINT menu_flag_open_file = MF_DISABLED;
	if (menu_flag == MF_ENABLED) {
		const DWORD file_stat = GetFileAttributesW(path);
		if ((file_stat == INVALID_FILE_ATTRIBUTES) || ((file_stat & FILE_ATTRIBUTE_DIRECTORY) != 0)) {
			menu_flag_open_file = MF_DISABLED;
		}
		else {
			menu_flag_open_file = MF_ENABLED;
		}
	}

	HMENU hMenu= CreatePopupMenu();
	AppendMenuW(hMenu, menu_flag_open_file | MF_STRING | 0, 1, L"&Open file");
	AppendMenuW(hMenu, menu_flag | MF_STRING | 0, 2, L"Open folder(with explorer)");
	AppendMenuW(hMenu, menu_flag | MF_STRING | 0, 3, L"Send path to clipboard");
	int result = TrackPopupMenu(hMenu, TPM_RETURNCMD, pointer_pos->x, pointer_pos->y, 0 , hWnd, NULL);
	DestroyMenu(hMenu);
	switch (result) {
	case 1: {
		// アプリで開く
		openFileWithApplication(path, pts->ViewlogEditorW, pts->ViewlogEditorArg, UILanguageFile);
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
	wchar_t *filename;
	BOOL r = TTXSSHGetKnownHostsFileName(&filename);
	if (!r) {
		return NULL;
	}
	assert(!IsRelativePathW(filename));

	if (list->data_ptr != 0) {
		wchar_t *virtual_store_path;
		BOOL ret = convertVirtualStoreW(filename, &virtual_store_path);
		free(filename);
		if (ret) {
			return virtual_store_path;
		} else {
			return NULL;
		}
	}

	return filename;
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
 *	current
 */
static wchar_t *GetCurrentPath(const SetupList *, const TTTSet *)
{
	wchar_t *path;
	hGetCurrentDirectoryW(&path);
	return path;
}

static wchar_t *_GetTermLogPath(const SetupList *list, const TTTSet *pts)
{
	if (list->data_ptr == 0) {
		// Default log save folder
		// LogDefaultNameW
		if (pts->LogDefaultPathW != NULL) {
			wchar_t *d = GetTermLogDir(pts);
			int r = wcscmp(d, pts->LogDefaultPathW);
			free(d);
			if (r == 0) {
				// ts->LogDefaultPathW と GetTermLogDir() が同じなら
				// 表示しない
				return NULL;
			}
			else {
				return _wcsdup(pts->LogDefaultPathW);
			}
		}
		else {
			return _wcsdup(L"");
		}
	}
	else {
		return GetTermLogDir(pts);
	}
}

static wchar_t *_GetFileDir(const SetupList *, const TTTSet *pts)
{
	return GetFileDir(pts);
}

static wchar_t *GetHistoryFileName(const SetupList *, const TTTSet *pts)
{
#define BROADCAST_LOGFILE L"broadcast.log"
	wchar_t *fname = NULL;
	awcscats(&fname, pts->HomeDirW, L"\\", BROADCAST_LOGFILE, NULL);
	return fname;
}

typedef struct {
	TComVar *pcv;
	TipWin2 *tipwin;
	ReiseDlgHelper_t *resize_helper;
} dlg_data_t;

// ★★★★★★★★★★
// 古いフォントを保持するためのグローバル変数
HFONT g_hCurrentFont = NULL;

// 子ウィンドウすべてにフォントを適用するためのコールバック
BOOL CALLBACK SetFontCallback(HWND hWndChild, LPARAM lParam) {
    SendMessage(hWndChild, WM_SETFONT, (WPARAM)lParam, MAKELPARAM(TRUE, 0));
    return TRUE;
}

void UpdateControlFont(HWND hWnd, UINT dpi) {
    NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };

    // 1. 新しいDPIに基づいたシステムフォント情報を取得
    if (SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0, dpi)) {

        // 2. 新しいフォントを作成 (lfMessageFontは標準のダイアログ用)
        HFONT hNewFont = CreateFontIndirect(&ncm.lfMessageFont);
        if (hNewFont) {
            // 3. 親ウィンドウおよびすべての子コントロールに適用
            SendMessage(hWnd, WM_SETFONT, (WPARAM)hNewFont, MAKELPARAM(TRUE, 0));
            EnumChildWindows(hWnd, SetFontCallback, (LPARAM)hNewFont);

            // 4. 古いフォントがあれば削除してメモリリークを防ぐ
            if (g_hCurrentFont) {
                DeleteObject(g_hCurrentFont);
            }
            g_hCurrentFont = hNewFont;
        }
    }
}
// ★★★★★★★★★★

static INT_PTR CALLBACK OnSetupDirectoryDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_SETUPDIR_TITLE" },
	};
	static const ResizeHelperInfo resize_info[] = {
		{ IDC_SETUP_DIR_LIST, RESIZE_HELPER_ANCHOR_LRTB },
		{ IDOK, RESIZE_HELPER_ANCHOR_RB },
		{ IDHELP, RESIZE_HELPER_ANCHOR_RB },
	};

	switch (msg) {
	case WM_INITDIALOG: {
		SetWindowLongPtrW(hDlgWnd, DWLP_USER, (LONG_PTR)lp);
		dlg_data_t *dlg_data = (dlg_data_t *)lp;
		TComVar *pcv = dlg_data->pcv;
		TTTSet *pts = pcv->ts;

		// I18N
		SetDlgTextsW(hDlgWnd, TextInfos, _countof(TextInfos), pts->UILanguageFileW);

		HWND hWndList = GetDlgItem(hDlgWnd, IDC_SETUP_DIR_LIST);
		ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

		LV_COLUMNA lvcol;
		lvcol.mask = LVCF_TEXT | LVCF_SUBITEM;
		lvcol.pszText = (LPSTR)"name";
		lvcol.iSubItem = 0;
		//ListView_InsertColumn(hWndList, 0, &lvcol);
		SendMessageA(hWndList, LVM_INSERTCOLUMNA, 0, (LPARAM)&lvcol);

		lvcol.pszText = (LPSTR)"path/file";
		lvcol.iSubItem = 1;
		//ListView_InsertColumn(hWndList, 1, &lvcol);
		SendMessageA(hWndList, LVM_INSERTCOLUMNA, 1, (LPARAM)&lvcol);

		// 設定一覧
		const SetupList setup_list[] = {
			{ "DLG_SETUPDIR_INIFILE", L"Tera Term Configuration File",
			  LIST_PARAM_STR, pts->SetupFNameW, NULL },
			{ NULL, L"  Virtual Store",
			  LIST_PARAM_FUNC, (void *)GetVirtualStorePath, pts->SetupFNameW },
			{ "DLG_SETUPDIR_KEYBOARDFILE", L"Keyboard Configuration File",
			  LIST_PARAM_STR, pts->KeyCnfFNW, NULL },
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
			  LIST_PARAM_FUNC, (void*)GetCurrentPath, NULL },
			{ NULL, L"HomeDir",
			  LIST_PARAM_STR, pts->HomeDirW, NULL },
			{ NULL, L"ExeDir",
			  LIST_PARAM_STR, pts->ExeDirW, NULL },
			{ NULL, L"File Transfer Dir",
			  LIST_PARAM_FUNC, (void*)_GetFileDir, NULL },
			{ NULL, L"LogDir(General)",
			  LIST_PARAM_STR, pts->LogDirW, NULL },
			{ NULL, L"Default log save folder",
			  LIST_PARAM_FUNC, (void*)_GetTermLogPath, (void *)0 },
			{ NULL, L"LogDir(Terminal)",
			  LIST_PARAM_FUNC, (void*)_GetTermLogPath, (void *)1 },
			{ NULL, L"Susie Plugin Path",
			  LIST_PARAM_STR, pts->EtermLookfeel.BGSPIPathW, NULL },
			{ NULL, L"UI language file",
			  LIST_PARAM_STR, pts->UILanguageFileW, NULL },
			{ NULL, L"Broadcast history file",
			  LIST_PARAM_FUNC, (void*)GetHistoryFileName, NULL },
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
			SendMessageW(hWndList, LVM_INSERTITEMW, 0, (LPARAM)&item);
			free(text);

			item.mask = LVIF_TEXT;
			item.iItem = y;
			item.iSubItem = 1;
			item.pszText = s;
			SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&item);

			y++;

			if (param & LIST_PARAM_FUNC) {
				// 表示用にもらった文字列を開放
				free(s);
			}
		}

		// 幅を調整
		for (int i = 0; i < 2; i++) {
			ListView_SetColumnWidth(hWndList, i, LVSCW_AUTOSIZE_USEHEADER);
		}

		dlg_data->resize_helper = ReiseDlgHelperInit(hDlgWnd, TRUE, resize_info, _countof(resize_info));

		CenterWindow(hDlgWnd, GetParent(hDlgWnd));

		static const wchar_t *str = L"Right click to open submenu";
		dlg_data->tipwin = TipWin2Create(NULL, hDlgWnd);
		TipWin2SetTextW(dlg_data->tipwin, IDC_SETUP_DIR_LIST, str);
		return TRUE;
	}

	case WM_COMMAND: {
		dlg_data_t *dlg_data = (dlg_data_t *)GetWindowLongPtrW(hDlgWnd, DWLP_USER);
		TComVar *pcv = dlg_data->pcv;
		switch (LOWORD(wp)) {
		case IDHELP:
			OpenHelpCV(pcv, HH_HELP_CONTEXT, HlpMenuSetupDir);
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

	case WM_DESTROY: {
		dlg_data_t *dlg_data = (dlg_data_t *)GetWindowLongPtrW(hDlgWnd, DWLP_USER);
		TipWin2Destroy(dlg_data->tipwin);
		dlg_data->tipwin = NULL;
		ReiseDlgHelperDelete(dlg_data->resize_helper);
		dlg_data->resize_helper = NULL;
		return TRUE;
	}

	case WM_NOTIFY: {
		dlg_data_t *dlg_data = (dlg_data_t *)GetWindowLongPtrW(hDlgWnd, DWLP_USER);
		NMHDR *nmhdr = (NMHDR *)lp;
		TComVar *pcv = dlg_data->pcv;
		TTTSet *pts = pcv->ts;
		if (nmhdr->code == TTN_POP) {
			// 1回だけ表示するため、閉じたら削除する
			TipWin2SetTextW(dlg_data->tipwin, IDC_SETUP_DIR_LIST, NULL);
		}
		else if (nmhdr->idFrom == IDC_SETUP_DIR_LIST) {
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
#if 0
	case WM_MOVE: {
		dlg_data_t *dlg_data = (dlg_data_t *)GetWindowLongPtrW(hDlgWnd, DWLP_USER);
		ReiseDlgHelper_WM_SIZE(dlg_data->resize_helper, wp, lp);
		// 幅を調整
		HWND hWndList = GetDlgItem(hDlgWnd, IDC_SETUP_DIR_LIST);
		for (int i = 0; i < 2; i++) {
			ListView_SetColumnWidth(hWndList, i, LVSCW_AUTOSIZE_USEHEADER);
		}
		break;
	}
#endif
	case WM_SIZE: {
		dlg_data_t *dlg_data = (dlg_data_t *)GetWindowLongPtrW(hDlgWnd, DWLP_USER);
		ReiseDlgHelper_WM_SIZE(dlg_data->resize_helper, wp, lp);
		// 幅を調整
		HWND hWndList = GetDlgItem(hDlgWnd, IDC_SETUP_DIR_LIST);
		for (int i = 0; i < 2; i++) {
			ListView_SetColumnWidth(hWndList, i, LVSCW_AUTOSIZE_USEHEADER);
		}
		break;
	}

	default: {
		dlg_data_t *dlg_data = (dlg_data_t *)GetWindowLongPtrW(hDlgWnd, DWLP_USER);
		if (dlg_data != NULL) {
			return ResizeDlgHelperProc(dlg_data->resize_helper, hDlgWnd, msg, wp, lp);
		}
		return FALSE;
	}
	}
	return TRUE;
}

void SetupDirectoryDialog(HINSTANCE hInst, HWND hWnd, TComVar *pcv)
{
	dlg_data_t* dlg_data = (dlg_data_t*)calloc(1, sizeof(dlg_data_t));
	dlg_data->pcv = pcv;
	TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SETUP_DIR_DIALOG),
					 hWnd, OnSetupDirectoryDlgProc, (LPARAM)dlg_data);
	free(dlg_data);
}
