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
/* IPv6 modification is Copyright(C) 2000 Jun-ya Kato <kato@win6.jp> */

/* TERATERM.EXE, VT window */

// SDK7.0の場合、WIN32_IEが適切に定義されない
#if _MSC_VER == 1400	// VS2005の場合のみ
#if !defined(_WIN32_IE)
#define	_WIN32_IE 0x0501
#endif
#endif

#include "teraterm.h"
#include "tttypes.h"
#include "tttypes_key.h"

#include "ttcommon.h"
#include "ttwinman.h"	//
//#include "ttsetup.h"
//#include "keyboard.h"
//#include "buffer.h"
//#include "vtterm.h"
//#include "vtdisp.h"
#include "ttdialog.h"
//#include "ttime.h"
#include "commlib.h"
//#include "clipboar.h"
//#include "filesys.h"
//#include "telnet.h"
//#include "tektypes.h"
//#include "ttdde.h"
#include "ttlib.h"
#include "dlglib.h"
#include "helpid.h"
//#include "teraprn.h"
//#include <winsock2.h>
//#include <ws2tcpip.h>
//#include "ttplug.h"  /* TTPLUG */
#include "teraterml.h"
//#include "buffer.h"

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>
#include <io.h>
#include <errno.h>

#include <shlobj.h>
#include <windows.h>
#include <windowsx.h>
//#include <imm.h>
#include <dbt.h>
#include <assert.h>
#include <wchar.h>
#include <htmlhelp.h>

#include "tt_res.h"
#include "vtwin.h"
//#include "addsetting.h"
//#include "winjump.h"
#include "sizetip.h"
//#include "dnddlg.h"
//#include "tekwin.h"
#include "compat_win.h"
//#include "unicode_test.h"
#if UNICODE_DEBUG
#include "tipwin.h"
#endif
#include "codeconv.h"
#include "sendmem.h"
//#include "sendfiledlg.h"
#include "setting.h"
//#include "broadcast.h"
#include "asprintf.h"
//#include "teraprn.h"

#include "setupdirdlg.h"

//
// 指定したアプリケーションでファイルを開く。
//
// return TRUE: success
//        FALSE: failure
//
static BOOL openFileWithApplication(char *pathname, char *filename, char *editor)
{
	char command[1024];
	char fullpath[1024];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	BOOL ret = FALSE;
	wchar_t buf[80];
	wchar_t uimsg[MAX_UIMSG];
	wchar_t uimsg2[MAX_UIMSG];

	SetLastError(NO_ERROR);

	_snprintf_s(fullpath, sizeof(fullpath), "%s\\%s", pathname, filename);
	if (_access(fullpath, 0) != 0) { // ファイルが存在しない
		DWORD no = GetLastError();
		get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
		get_lang_msgW("DLG_SETUPDIR_NOFILE_ERROR", uimsg2, _countof(uimsg2),
					  L"File does not exist.(%d)", ts.UILanguageFile);
		_snwprintf_s(buf, _countof(buf), _TRUNCATE, uimsg2, no);
		::MessageBoxW(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
		goto error;
	}

	_snprintf_s(command, sizeof(command), _TRUNCATE, "%s \"%s\"", editor, fullpath);

	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(NULL, command, NULL, NULL, FALSE, 0,
		NULL, NULL, &si, &pi) == 0) { // 起動失敗
		DWORD no = GetLastError();
		get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
		get_lang_msgW("DLG_SETUPDIR_OPENFILE_ERROR", uimsg2, _countof(uimsg2),
					  L"Cannot open file.(%d)", ts.UILanguageFile);
		_snwprintf_s(buf, _countof(buf), _TRUNCATE, uimsg2, no);
		::MessageBoxW(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
		goto error;
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	ret = TRUE;

error:;
	return (ret);
}

//
// エクスプローラでパスを開く。
//
// return TRUE: success
//        FALSE: failure
//
static BOOL openDirectoryWithExplorer(const wchar_t *path)
{
	LPSHELLFOLDER pDesktopFolder;
	LPMALLOC pMalloc;
	LPITEMIDLIST pIDL;
	SHELLEXECUTEINFO si;
	BOOL ret = FALSE;

	if (SHGetDesktopFolder(&pDesktopFolder) == S_OK) {
		if (SHGetMalloc(&pMalloc) == S_OK) {
			if (pDesktopFolder->ParseDisplayName(NULL, NULL, (LPWSTR)path, NULL, &pIDL, NULL) == S_OK) {
				::ZeroMemory(&si, sizeof(si));
				si.cbSize = sizeof(si);
				si.fMask = SEE_MASK_IDLIST;
				si.lpVerb = "open";
				si.lpIDList = pIDL;
				si.nShow = SW_SHOWNORMAL;
				::ShellExecuteEx(&si);
				pMalloc->Free((void *)pIDL);

				ret = TRUE;
			}

			pMalloc->Release();
		}
		pDesktopFolder->Release();
	}

	return (ret);
}

//
// フォルダもしくはファイルを開く。
//
static void openFileDirectory(char *path, char *filename, BOOL open_directory_only, char *open_editor)
{
	if (open_directory_only) {
		wchar_t *pathW = ToWcharA(path);
		openDirectoryWithExplorer(pathW);
		free(pathW);
	}
	else {
		openFileWithApplication(path, filename, open_editor);
	}
}

//
// Virtual Storeパスに変換する。
//
// path: IN
// filename: IN
// vstore_path: OUT
// vstore_pathlen: IN
//
// return TRUE: success
//        FALSE: failure
//
static BOOL convertVirtualStore(char *path, char *filename, char *vstore_path, int vstore_pathlen)
{
	BOOL ret = FALSE;
	const char *s, **p;
	const char *virstore_env[] = {
		"ProgramFiles",
		"ProgramData",
		"SystemRoot",
		NULL
	};
	char shPath[1024] = "";
	char shFullPath[1024] = "";
	LPITEMIDLIST pidl;
	int CSIDL;

	OutputDebugPrintf("[%s][%s]\n", path, filename);

	if (cv.VirtualStoreEnabled == FALSE)
		goto error;

	// Virtual Store対象となるフォルダか。
	p = virstore_env;
	while (*p) {
		s = getenv(*p);
		if (s != NULL && strstr(path, s) != NULL) {
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
	SHGetPathFromIDList(pidl, shPath);
	CoTaskMemFree(pidl);

	// Virtual Storeパスを作る。
	strncat_s(shPath, sizeof(shPath), "\\VirtualStore", _TRUNCATE);

	// 不要なドライブレターを除去する。
	// ドライブレターは一文字とは限らない点に注意。
	s = strstr(path, ":\\");
	if (s != NULL) {
		strncat_s(shPath, sizeof(shPath), s + 1, _TRUNCATE);
	}

	// 最後に、Virtual Storeにファイルがあるかどうかを調べる。
	_snprintf_s(shFullPath, sizeof(shFullPath), "%s\\%s", shPath, filename);
	if (_access(shFullPath, 0) != 0) {
		goto error;
	}

	strncpy_s(vstore_path, vstore_pathlen, shPath, _TRUNCATE);

	ret = TRUE;
	return (ret);

error:
	return (ret);
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
	static char teratermexepath[MAX_PATH];
	static char inipath[MAX_PATH], inifilename[MAX_PATH], inipath_vstore[1024];
	static char keycnfpath[MAX_PATH], keycnffilename[MAX_PATH], keycnfpath_vstore[1024];
	static char cygtermpath[MAX_PATH], cygtermfilename[MAX_PATH], cygtermpath_vstore[1024];
//	static char eterm1path[MAX_PATH], eterm1filename[MAX_PATH], eterm1path_vstore[1024];
	char temp[MAX_PATH];
	char tmpbuf[1024];
	typedef int (CALLBACK *PSSH_read_known_hosts_file)(char *, int);
	PSSH_read_known_hosts_file func = NULL;
	HMODULE h = NULL;
	static char hostsfilepath[MAX_PATH], hostsfilename[MAX_PATH], hostsfilepath_vstore[1024];
	char *path_p, *filename_p;
	BOOL open_dir, ret;
	int button_pressed;
	HWND hWnd;

	switch (msg) {
	case WM_INITDIALOG:
		// I18N
		SetDlgTexts(hDlgWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

		if (GetModuleFileNameA(NULL, temp, sizeof(temp)) != 0) {
			ExtractDirName(temp, teratermexepath);
		}

		// 設定ファイル(teraterm.ini)のパスを取得する。
		/// (1)
		ExtractFileName(ts.SetupFName, inifilename, sizeof(inifilename));
		ExtractDirName(ts.SetupFName, inipath);
		//SetDlgItemText(hDlgWnd, IDC_INI_SETUPDIR_STATIC, inifilename);
		SetDlgItemText(hDlgWnd, IDC_INI_SETUPDIR_EDIT, ts.SetupFName);
		/// (2) Virutal Storeへの変換
		memset(inipath_vstore, 0, sizeof(inipath_vstore));
		ret = convertVirtualStore(inipath, inifilename, inipath_vstore, sizeof(inipath_vstore));
		if (ret) {
			hWnd = GetDlgItem(hDlgWnd, IDC_INI_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, TRUE);
			hWnd = GetDlgItem(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, TRUE);
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, "%s\\%s", inipath_vstore, inifilename);
			SetDlgItemText(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE, tmpbuf);
		}
		else {
			hWnd = GetDlgItem(hDlgWnd, IDC_INI_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, FALSE);
			hWnd = GetDlgItem(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, FALSE);
			SetDlgItemText(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE, "");
		}

		// 設定ファイル(KEYBOARD.CNF)のパスを取得する。
		/// (1)
		ExtractFileName(ts.KeyCnfFN, keycnffilename, sizeof(keycnfpath));
		ExtractDirName(ts.KeyCnfFN, keycnfpath);
		//SetDlgItemText(hDlgWnd, IDC_KEYCNF_SETUPDIR_STATIC, keycnffilename);
		SetDlgItemText(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT, ts.KeyCnfFN);
		/// (2) Virutal Storeへの変換
		memset(keycnfpath_vstore, 0, sizeof(keycnfpath_vstore));
		ret = convertVirtualStore(keycnfpath, keycnffilename, keycnfpath_vstore, sizeof(keycnfpath_vstore));
		if (ret) {
			hWnd = GetDlgItem(hDlgWnd, IDC_KEYCNF_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, TRUE);
			hWnd = GetDlgItem(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, TRUE);
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, "%s\\%s", keycnfpath_vstore, keycnffilename);
			SetDlgItemText(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE, tmpbuf);
		}
		else {
			hWnd = GetDlgItem(hDlgWnd, IDC_KEYCNF_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, FALSE);
			hWnd = GetDlgItem(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, FALSE);
			SetDlgItemText(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE, "");
		}


		// cygterm.cfg は ttermpro.exe 配下に位置する。
		/// (1)
		strncpy_s(cygtermfilename, sizeof(cygtermfilename), "cygterm.cfg", _TRUNCATE);
		strncpy_s(cygtermpath, sizeof(cygtermpath), teratermexepath, _TRUNCATE);
		//SetDlgItemText(hDlgWnd, IDC_CYGTERM_SETUPDIR_STATIC, cygtermfilename);
		_snprintf_s(temp, sizeof(temp), "%s\\%s", cygtermpath, cygtermfilename);
		SetDlgItemText(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT, temp);
		/// (2) Virutal Storeへの変換
		memset(cygtermpath_vstore, 0, sizeof(cygtermpath_vstore));
		ret = convertVirtualStore(cygtermpath, cygtermfilename, cygtermpath_vstore, sizeof(cygtermpath_vstore));
		if (ret) {
			hWnd = GetDlgItem(hDlgWnd, IDC_CYGTERM_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, TRUE);
			hWnd = GetDlgItem(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, TRUE);
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, "%s\\%s", cygtermpath_vstore, cygtermfilename);
			SetDlgItemText(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE, tmpbuf);
		}
		else {
			hWnd = GetDlgItem(hDlgWnd, IDC_CYGTERM_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, FALSE);
			hWnd = GetDlgItem(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, FALSE);
			SetDlgItemText(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE, "");
		}

		// ssh_known_hosts
		if (func == NULL) {
			if (((h = GetModuleHandle("ttxssh.dll")) != NULL)) {
				func = (PSSH_read_known_hosts_file)GetProcAddress(h, "TTXReadKnownHostsFile");
				if (func) {
					int ret = func(temp, sizeof(temp));
					if (ret) {
						char *s = strstr(temp, ":\\");

						if (s) { // full path
							ExtractFileName(temp, hostsfilename, sizeof(hostsfilename));
							ExtractDirName(temp, hostsfilepath);
						}
						else { // relative path
							strncpy_s(hostsfilepath, sizeof(hostsfilepath), teratermexepath, _TRUNCATE);
							strncpy_s(hostsfilename, sizeof(hostsfilename), temp, _TRUNCATE);
							_snprintf_s(temp, sizeof(temp), "%s\\%s", hostsfilepath, hostsfilename);
						}

						SetDlgItemText(hDlgWnd, IDC_SSH_SETUPDIR_EDIT, temp);

						/// (2) Virutal Storeへの変換
						memset(hostsfilepath_vstore, 0, sizeof(hostsfilepath_vstore));
						ret = convertVirtualStore(hostsfilepath, hostsfilename, hostsfilepath_vstore, sizeof(hostsfilepath_vstore));
						if (ret) {
							hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_STATIC_VSTORE);
							EnableWindow(hWnd, TRUE);
							hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE);
							EnableWindow(hWnd, TRUE);
							_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, "%s\\%s", hostsfilepath_vstore, hostsfilename);
							SetDlgItemText(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE, tmpbuf);
						}
						else {
							hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_STATIC_VSTORE);
							EnableWindow(hWnd, FALSE);
							hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE);
							EnableWindow(hWnd, FALSE);
							SetDlgItemText(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE, "");
						}

					}
				}
			}
			else {
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT);
				EnableWindow(hWnd, FALSE);
				SetDlgItemText(hDlgWnd, IDC_SSH_SETUPDIR_EDIT, "");
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_BUTTON);
				EnableWindow(hWnd, FALSE);
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_BUTTON_FILE);
				EnableWindow(hWnd, FALSE);
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_STATIC_VSTORE);
				EnableWindow(hWnd, FALSE);
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE);
				EnableWindow(hWnd, FALSE);
				SetDlgItemText(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE, "");
			}
		}

		return TRUE;

	case WM_COMMAND:
		button_pressed = 0;
		switch (LOWORD(wp)) {
		case IDC_INI_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			open_dir = TRUE;
			path_p = inipath;
			if (inipath_vstore[0])
				path_p = inipath_vstore;
			filename_p = inifilename;
			button_pressed = 1;
			break;
		case IDC_INI_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			open_dir = FALSE;
			path_p = inipath;
			if (inipath_vstore[0])
				path_p = inipath_vstore;
			filename_p = inifilename;
			button_pressed = 1;
			break;

		case IDC_KEYCNF_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			open_dir = TRUE;
			path_p = keycnfpath;
			if (keycnfpath_vstore[0])
				path_p = keycnfpath_vstore;
			filename_p = keycnffilename;
			button_pressed = 1;
			break;
		case IDC_KEYCNF_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			open_dir = FALSE;
			path_p = keycnfpath;
			if (keycnfpath_vstore[0])
				path_p = keycnfpath_vstore;
			filename_p = keycnffilename;
			button_pressed = 1;
			break;

		case IDC_CYGTERM_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			open_dir = TRUE;
			path_p = cygtermpath;
			if (cygtermpath_vstore[0])
				path_p = cygtermpath_vstore;
			filename_p = cygtermfilename;
			button_pressed = 1;
			break;
		case IDC_CYGTERM_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			open_dir = FALSE;
			path_p = cygtermpath;
			if (cygtermpath_vstore[0])
				path_p = cygtermpath_vstore;
			filename_p = cygtermfilename;
			button_pressed = 1;
			break;

		case IDC_SSH_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			open_dir = TRUE;
			path_p = hostsfilepath;
			if (hostsfilepath_vstore[0])
				path_p = hostsfilepath_vstore;
			filename_p = hostsfilename;
			button_pressed = 1;
			break;
		case IDC_SSH_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			open_dir = FALSE;
			path_p = hostsfilepath;
			if (hostsfilepath_vstore[0])
				path_p = hostsfilepath_vstore;
			filename_p = hostsfilename;
			button_pressed = 1;
			break;

		case IDCANCEL:
			TTEndDialog(hDlgWnd, IDCANCEL);
			return TRUE;
			break;

		default:
			return FALSE;
		}

		if (button_pressed) {
			char *app = NULL;

			if (open_dir)
				app = NULL;
			else
				app = ts.ViewlogEditor;

			openFileDirectory(path_p, filename_p, open_dir, app);
			return TRUE;
		}
		return FALSE;

	case WM_CLOSE:
		TTEndDialog(hDlgWnd, 0);
		return TRUE;

	default:
		return FALSE;
	}
	return TRUE;
}

void SetupDirectoryDialog(HINSTANCE hInst, HWND hWnd)
{
	TTDialogBox(hInst, MAKEINTRESOURCE(IDD_SETUP_DIR_DIALOG),
	            hWnd, OnSetupDirectoryDlgProc);
}
