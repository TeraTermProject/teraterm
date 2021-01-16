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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"

#include "codeconv.h"
#include "dlglib.h"
#include "i18n.h"
#include "layer_for_unicode.h"
#include "asprintf.h"
#include "tt-version.h"		// for TT_VERSION_MAJOR, TT_VERSION_MINOR
#include "svnversion.h"		// for SVNVERSION

#include "resource.h"
#include "parse.h"
#include "getcontent.h"

#define ORDER 4000
#define ID_MENUITEM 55900	// see reference/develop.txt

typedef struct {
	HANDLE hInst;
	PTTSet ts;
	PComVar cv;
	version_one_t *versions;
	size_t versions_count;
} TInstVar;

static TInstVar InstVar;
static TInstVar *pvar;

/**
 *	ドロップダウンにバージョン情報一覧の一覧をセットする
 *
 *	最初に見つかった
 *	running_version のメジャーバージョンと同じ
 *	バージョン情報を選択する
 */
static int SetDropdown(HWND hDlg, int running_version)
{
	const int version_major = running_version / 10000;
	const int version_minor = running_version % 10000;
	char *str;
	int cursor = -1;
	size_t i;
	char version_label[32];

	GetDlgItemText(hDlg, IDC_VERSION_LABEL, version_label, sizeof(version_label));
#if defined(SVNVERSION)
	asprintf(&str, "%s (current version teraterm:%d.%d, ttxcheckupdate:%d.%d %s r%d)", version_label, version_major,
			 version_minor, TT_VERSION_MAJOR, TT_VERSION_MINOR, BRANCH_NAME, SVNVERSION);
#else
	asprintf(&str, "%s (current version teraterm:%d.%d, ttxcheckupdate:%d.%d)", version_label, version_major,
			 version_minor, TT_VERSION_MAJOR, TT_VERSION_MINOR);
#endif
	SetDlgItemTextA(hDlg, IDC_VERSION_LABEL, str);
	free(str);

	_SendDlgItemMessageW(hDlg, IDC_VERSION_DROPDOWN, CB_RESETCONTENT, 0, 0);
	for (i = 0; i < pvar->versions_count; i++) {
		const version_one_t *v = &pvar->versions[i];
		wchar_t *strW = ToWcharU8(v->version_text);
		_SendDlgItemMessageW(hDlg, IDC_VERSION_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)strW);
		free(strW);
		if (cursor == -1 && v->version_major == version_major) {
			cursor = (int)i;
		}
	}
	if (cursor == -1) {
		cursor = 0;
	}
	return cursor;
}

/**
 *	version_one_t の情報をダイアログに表示する
 */
static void SetTexts(HWND hDlg, const version_one_t *version)
{
	const version_one_t *v = version;

	wchar_t *strW = ToWcharU8(v->text);
	_SetWindowTextW(GetDlgItem(hDlg, IDC_DETAIL_EDIT), strW);
	free(strW);

	if (v->url == NULL) {
		EnableWindow(GetDlgItem(hDlg, IDC_OPEN), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_URL_EDIT), FALSE);
		SetWindowTextA(GetDlgItem(hDlg, IDC_URL_EDIT), "");
	}
	else {
		EnableWindow(GetDlgItem(hDlg, IDC_OPEN), TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_URL_EDIT), TRUE);
		SetWindowTextA(GetDlgItem(hDlg, IDC_URL_EDIT), v->url);
	}
}

static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	(void)lParam;
	switch (msg) {
		case WM_INITDIALOG: {
			int cursor = SetDropdown(hDlg, pvar->ts->RunningVersion);
			SendDlgItemMessage(hDlg, IDC_VERSION_DROPDOWN, CB_SETCURSEL, cursor, 0);
			SetTexts(hDlg, &pvar->versions[cursor]);
			PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDOK), TRUE);
			CenterWindow(hDlg, GetParent(hDlg));
			break;
		}

		case WM_COMMAND: {
			switch (wParam) {
				case IDOK | (BN_CLICKED << 16):
					EndDialog(hDlg, 1);
					break;
				case IDCANCEL | (BN_CLICKED << 16):
					EndDialog(hDlg, 0);
					break;
				case IDC_OPEN | (BN_CLICKED << 16): {
					int cursor = (int)SendDlgItemMessage(hDlg, IDC_VERSION_DROPDOWN, CB_GETCURSEL, 0, 0);
					const char *url = pvar->versions[cursor].url;
					ShellExecuteA(NULL, NULL, url, NULL, NULL, SW_SHOWNORMAL);
					break;
				}
				case IDC_VERSION_DROPDOWN | (CBN_SELCHANGE << 16): {
					int cursor = (int)SendDlgItemMessage(hDlg, IDC_VERSION_DROPDOWN, CB_GETCURSEL, 0, 0);
					SetTexts(hDlg, &pvar->versions[cursor]);
					break;
				}
				default:
					break;
			}
			break;
		}

		default:
			break;
	}
	return FALSE;
}

static void ShowDialog(HWND hWnd)
{
	const wchar_t *update_info_url_default =
		L"https://osdn.dl.osdn.net/storage/g/t/tt/ttssh2/snapshot/teraterm_version.json";
	const wchar_t *agent_base = L"teraterm_updatechecker";
	wchar_t agent[128];
	int result_mb;
	char *json_raw_ptr;
	size_t json_raw_size;
	BOOL result_bool;
	size_t json_size;
	char *json_ptr;
	const char *UILanguageFile = pvar->ts->UILanguageFile;
	wchar_t update_info_url[MAX_UIMSG];
	static const TTMessageBoxInfoW info = {
		"TTXCheckUpdate",
		NULL, L"Tera Term",
		"MSG_CHECKUPDATE", L"Do you want to check update?\n%s" };

	/* 更新情報を取得してもok? */
	GetI18nStrW("TTXCheckUpdate", "JSON_URL", update_info_url, _countof(update_info_url), update_info_url_default,
				UILanguageFile);
	result_mb = TTMessageBoxW(hWnd, &info, MB_YESNO | MB_ICONEXCLAMATION, UILanguageFile, update_info_url);
	if (result_mb == IDNO) {
		return;
	}

	/* 更新情報取得、'\0'を追加する→ json文字列を作成 */
	swprintf(agent, _countof(agent), L"%s_%d", agent_base, pvar->ts->RunningVersion);
	result_bool = GetContent(update_info_url, agent, (void**)&json_raw_ptr, &json_raw_size);
	if (!result_bool) {
		_MessageBoxW(hWnd, L"access error?", L"Tera Term", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	json_size = json_raw_size + 1;
	json_ptr = realloc(json_raw_ptr, json_size);
	if (json_ptr == NULL) {
		free(json_raw_ptr);
		return;
	}
	json_raw_ptr = NULL;
	json_ptr[json_size - 1] = '\0';

	/* jsonをパースする, versionsに情報が入る */
	pvar->versions = ParseJson(json_ptr, &pvar->versions_count);
	if (pvar->versions == NULL) {
		_MessageBoxW(hWnd, L"parse error?", L"Tera Term", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	/* ダイアログを出す */
	SetDialogFont(pvar->ts->DialogFontName, pvar->ts->DialogFontPoint, pvar->ts->DialogFontCharSet,
				  pvar->ts->UILanguageFile, "Tera Term", "DLG_TAHOMA_FONT");
	TTDialogBoxParam(pvar->hInst, MAKEINTRESOURCE(IDD_CHECK_UPDATE_DIALOG), hWnd, DlgProc, (LPARAM)pvar);

	/* 終了 */
	free(json_ptr);
	ParseFree(pvar->versions, pvar->versions_count);
	pvar->versions = NULL;
}

static void WINAPI TTXInit(PTTSet ts, PComVar cv)
{
	pvar->ts = ts;
	pvar->cv = cv;
}

static void WINAPI TTXModifyMenu(HMENU menu)
{
	static const DlgTextInfo MenuTextInfo[] = {
		{ ID_MENUITEM, "MENU_CHECKUPDATE" },
	};
	const UINT ID_HELP_INDEX2 = 50910;

	const char *UILanguageFile = pvar->ts->UILanguageFile;

	TTInsertMenuItemA(menu, ID_HELP_INDEX2, MF_ENABLED, ID_MENUITEM, "Check &Update...", FALSE);
	TTInsertMenuItemA(menu, ID_HELP_INDEX2, MF_SEPARATOR, 0, NULL, FALSE);

	SetI18nMenuStrs("TTXCheckUpdate", menu, MenuTextInfo, _countof(MenuTextInfo), UILanguageFile);
}

static int WINAPI TTXProcessCommand(HWND hWin, WORD cmd)
{
	if (cmd == ID_MENUITEM) {
		ShowDialog(hWin);
		return 1;
	}
	return 0;
}

static TTXExports Exports = {
	sizeof(TTXExports),
	ORDER,

	TTXInit,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	TTXModifyMenu,
	NULL,
	TTXProcessCommand,
	NULL,
	NULL,
	NULL,
	NULL,
};

BOOL __declspec(dllexport) WINAPI TTXBind(WORD Version, TTXExports *exports)
{
	size_t size;

	if (Version != TTVERSION) {
		return FALSE;
	}

#if 0
	if (!IsWindowsNTKernel()) {
		// TODO Windows10以外、未検証
		return FALSE;
	}
#endif

	size = sizeof(Exports) - sizeof(exports->size);
	if ((int)size > exports->size) {
		size = exports->size;
	}
	memcpy((char *)exports + sizeof(exports->size), (char *)&Exports + sizeof(exports->size), size);
	return TRUE;
}

BOOL WINAPI DllMain(HANDLE hInstance, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	(void)lpReserved;
	switch (ul_reason_for_call) {
		case DLL_THREAD_ATTACH:
			/* do thread initialization */
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF) | _CRTDBG_LEAK_CHECK_DF);
			break;
		case DLL_THREAD_DETACH:
			/* do thread cleanup */
			break;
		case DLL_PROCESS_ATTACH:
			/* do process initialization */
			pvar = &InstVar;
			pvar->hInst = hInstance;
			break;
		case DLL_PROCESS_DETACH:
			/* do process cleanup */
			break;
	}
	return TRUE;
}
