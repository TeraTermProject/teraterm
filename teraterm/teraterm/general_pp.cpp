/*
 * Copyright (C) 2024- TeraTerm Project
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

/* General Propaty Page */

#include <stdio.h>
#include <windows.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "ttwinman.h"	// for ts
#include "helpid.h"
#include "i18n.h"
#include "win32helper.h"
#include "ttcmn_notify2.h"
#include "compat_win.h"
#include "asprintf.h"
#include "tipwin2.h"

#include "general_pp.h"

typedef struct {
	wchar_t* fullname;
	wchar_t* filename;
	wchar_t* language;
	wchar_t* date;
	wchar_t* contributor;
} LangInfo;

static const wchar_t* get_lang_folder()
{
	return (IsWindowsNTKernel()) ? L"lang_utf16le" : L"lang";
}

static void LangFree(LangInfo* infos, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++) {
		LangInfo* p = infos + i;
		free(p->filename);
		free(p->fullname);
		free(p->language);
		free(p->date);
		free(p->contributor);
	}
	free(infos);
}

/**
 *	ファイル名をリストする(ファイル名のみ)
 *	infosに追加してreturnする
 */
static LangInfo* LangAppendFileList(const wchar_t* folder, LangInfo* infos, size_t* infos_size)
{
	wchar_t* fullpath;
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	size_t count = *infos_size;

	aswprintf(&fullpath, L"%s\\*.lng", folder);
	hFind = FindFirstFileW(fullpath, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				LangInfo* p = (LangInfo*)realloc(infos, sizeof(LangInfo) * (count + 1));
				if (p != NULL) {
					infos = p;
					p = infos + count;
					count++;
					memset(p, 0, sizeof(*p));
					p->filename = _wcsdup(fd.cFileName);
					aswprintf(&p->fullname, L"%s\\%s", folder, fd.cFileName);
				}
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
	free(fullpath);

	*infos_size = count;
	return infos;
}

/**
 *	lngファイルの Info セクションを読み込む
 */
static void LangRead(LangInfo* infos, size_t infos_size)
{
	size_t i;

	for (i = 0; i < infos_size; i++) {
		LangInfo* p = infos + i;
		const wchar_t* lng = p->fullname;
		wchar_t* s;
		hGetPrivateProfileStringW(L"Info", L"language", NULL, lng, &s);
		if (s[0] == 0) {
			free(s);
			p->language = _wcsdup(p->filename);
		}
		else {
			p->language = s;
		}
		hGetPrivateProfileStringW(L"Info", L"date", NULL, lng, &s);
		if (s[0] == 0) {
			free(s);
			p->date = _wcsdup(L"-");
		}
		else {
			p->date = s;
		}
		hGetPrivateProfileStringW(L"Info", L"contributor", NULL, lng, &s);
		if (s[0] == 0) {
			free(s);
			p->contributor = _wcsdup(L"-");
		}
		else {
			p->contributor = s;
		}
	}
}

static wchar_t *LangInfoText(const LangInfo *p)
{
	wchar_t *info;
	aswprintf(&info,
			  L"language\r\n"
			  L"  %s\r\n"
			  L"filename\r\n"
			  L"  %s\r\n"
			  L"date\r\n"
			  L"  %s\r\n"
			  L"contributor\r\n"
			  L"  %s",
			  p->language, p->filename, p->date, p->contributor);
	return info;
}

typedef struct DlgDataTag {
	TTTSet* ts;
	LangInfo* lng_infos;
	size_t lng_size;
	size_t selected_lang;	// 選ばれていたlngファイル番号
	TipWin2 *tipwin2;
} DlgData;


CGeneralPropPageDlg::CGeneralPropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CGeneralPropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_GENERAL",
				 L"General", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
	data = (DlgData *)calloc(1, sizeof(*data));
	data->ts = &ts;

	// UI Language, 読み込み
	TTTSet *pts = data->ts;
	LangInfo* infos = NULL;
	size_t infos_size = 0;
	wchar_t* folder;
	aswprintf(&folder, L"%s\\%s", pts->ExeDirW, get_lang_folder());
	infos = LangAppendFileList(folder, infos, &infos_size);
	free(folder);
	if (wcscmp(pts->ExeDirW, pts->HomeDirW) != 0) {
		aswprintf(&folder, L"%s\\%s", pts->HomeDirW, get_lang_folder());
		infos = LangAppendFileList(folder, infos, &infos_size);
		free(folder);
	}
	LangRead(infos, infos_size);

	data->lng_infos = infos;
	data->lng_size = infos_size;
}

CGeneralPropPageDlg::~CGeneralPropPageDlg()
{
	TipWin2Destroy(data->tipwin2);
	data->tipwin2 = NULL;
	LangFree(data->lng_infos, data->lng_size);
	data->lng_infos = NULL;
	data->lng_size = 0;
	free((void *)m_psp.pszTitle);
	free(data);
	data = NULL;
}

// CGeneralPropPageDlg メッセージ ハンドラ

void CGeneralPropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_DISABLE_SENDBREAK, "DLG_TAB_GENERAL_DISABLESENDBREAK" },
		{ IDC_ACCEPT_BROADCAST, "DLG_TAB_GENERAL_ACCEPTBROADCAST" },
		{ IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE, "DLG_TAB_GENERAL_AUTOSCROLL_ONLY_IN_BOTTOM_LINE" },
		{ IDC_CLEAR_ON_RESIZE, "DLG_TAB_GENERAL_CLEAR_ON_RESIZE" },
		{ IDC_CURSOR_CHANGE_IME, "DLG_TAB_GENERAL_CURSOR_CHANGE_IME" },
		// { IDC_LIST_HIDDEN_FONTS, "DLG_TAB_GENERAL_LIST_HIDDEN_FONTS" },
		{ IDC_GENUILANG_LABEL, "DLG_GEN_UILANG" },
		{ IDC_GENPORT_LABEL, "DLG_GEN_PORT" },
		{ IDC_TITLEFMT_GROUP, "DLG_TAB_GENERAL_TITLEFMT_GROUP" },
		{ IDC_TITLEFMT_DISPHOSTNAME, "DLG_TAB_GENERAL_TITLEFMT_DISPHOSTNAME" },
		{ IDC_TITLEFMT_DISPSESSION, "DLG_TAB_GENERAL_TITLEFMT_DISPSESSION" },
		{ IDC_TITLEFMT_DISPVTTEK, "DLG_TAB_GENERAL_TITLEFMT_DISPVTTEK" },
		{ IDC_TITLEFMT_SWAPHOSTTITLE, "DLG_TAB_GENERAL_TITLEFMT_SWAPHOSTTITLE" },
		{ IDC_TITLEFMT_DISPTCPPORT, "DLG_TAB_GENERAL_TITLEFMT_DISPTCPPORT" },
		{ IDC_TITLEFMT_DISPSERIALSPEED, "DLG_TAB_GENERAL_TITLEFMT_DISPSERIALSPEED" },
		{ IDC_NOTIFICATION_TITLE, "DLG_TAB_GENERAL_NOTIFICATION_TITLE" },
		{ IDC_NOTIFY_SOUND, "DLG_TAB_GENERAL_NOTIFIY_SOUND" },
		{ IDC_NOTIFICATION_TEST_POPUP, "DLG_TAB_GENERAL_NOTIFICATION_TEST_NOTIFY" },
		{ IDC_NOTIFICATION_TEST_TRAY, "DLG_TAB_GENERAL_NOTIFICATION_TEST_TRAY" },
	};
	TTTSet *pts = data->ts;

	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), pts->UILanguageFileW);

	// (1)DisableAcceleratorSendBreak
	SetCheck(IDC_DISABLE_SENDBREAK, pts->DisableAcceleratorSendBreak);

	// (2)EnableClickableUrl
	SetCheck(IDC_CLICKABLE_URL, pts->EnableClickableUrl);

	// (3)AcceptBroadcast 337: 2007/03/20
	SetCheck(IDC_ACCEPT_BROADCAST, pts->AcceptBroadcast);

	// (5)IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE
	SetCheck(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE, pts->AutoScrollOnlyInBottomLine);

	// (6)IDC_CLEAR_ON_RESIZE
	SetCheck(IDC_CLEAR_ON_RESIZE, (pts->TermFlag & TF_CLEARONRESIZE) != 0);

	// (7)IDC_CURSOR_CHANGE_IME
	SetCheck(IDC_CURSOR_CHANGE_IME, (pts->WindowFlag & WF_IMECURSORCHANGE) != 0);

	// (8)IDC_LIST_HIDDEN_FONTS
	// SetCheck(IDC_LIST_HIDDEN_FONTS, pts->ListHiddenFonts);

	// (9) Title Format
	SetCheck(IDC_TITLEFMT_DISPHOSTNAME, (pts->TitleFormat & 1) != 0);
	SetCheck(IDC_TITLEFMT_DISPSESSION, (pts->TitleFormat & (1<<1)) != 0);
	SetCheck(IDC_TITLEFMT_DISPVTTEK, (pts->TitleFormat & (1<<2)) != 0);
	SetCheck(IDC_TITLEFMT_SWAPHOSTTITLE, (pts->TitleFormat & (1<<3)) != 0);
	SetCheck(IDC_TITLEFMT_DISPTCPPORT, (pts->TitleFormat & (1<<4)) != 0);
	SetCheck(IDC_TITLEFMT_DISPSERIALSPEED, (pts->TitleFormat & (1<<5)) != 0);

	// Notify
	SetCheck(IDC_NOTIFY_SOUND, pts->NotifySound);

	// UI Language用 tipwin
	data->tipwin2 = TipWin2Create(NULL, m_hWnd);

	// UI Language, 選択
	{
		data->selected_lang = 0;
		LangInfo* infos = data->lng_infos;
		size_t info_size = data->lng_size;
		for (size_t i = 0; i < info_size; i++) {
			const LangInfo* p = infos + i;
			SendDlgItemMessageW(IDC_GENUILANG, CB_ADDSTRING, 0, (LPARAM)p->language);
			if (wcscmp(p->fullname, ts.UILanguageFileW) == 0) {
				data->selected_lang = i;
				wchar_t *info_text = LangInfoText(p);
				TipWin2SetTextW(data->tipwin2, IDC_GENUILANG, info_text);
				free(info_text);
			}
		}
		SendDlgItemMessage(IDC_GENUILANG, CB_SETCURSEL, data->selected_lang, 0);
	}

	// default port
	{
		WORD w;
		SendDlgItemMessageA(IDC_GENPORT, CB_ADDSTRING, 0, (LPARAM) "TCP/IP");
		for (w=1;w<=pts->MaxComPort;w++) {
			char Temp[8];
			_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "COM%d", w);
			SendDlgItemMessageA(IDC_GENPORT, CB_ADDSTRING, 0, (LPARAM)Temp);
		}
		if (pts->PortType==IdSerial) {
			if (pts->ComPort <= pts->MaxComPort) {
				w = pts->ComPort;
			}
			else {
				w = 1; // COM1
			}
		}
		else {
			w = 0; // TCP/IP
		}
		SendDlgItemMessageA(IDC_GENPORT, CB_SETCURSEL, w, 0);
	}

	// Download dir
	SetDlgItemTextW(IDC_DOWNLOAD_DIR, pts->FileDirW);

#if 0
	// ダイアログにフォーカスを当てる (2004.12.7 yutaka)
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_CLICKABLE_URL));
#endif
}

void CGeneralPropPageDlg::OnOK()
{
	TTTSet *pts = data->ts;

	// (1)
	pts->DisableAcceleratorSendBreak = GetCheck(IDC_DISABLE_SENDBREAK);

	// (3) 337: 2007/03/20
	pts->AcceptBroadcast = GetCheck(IDC_ACCEPT_BROADCAST);

	// (5)IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE
	pts->AutoScrollOnlyInBottomLine = GetCheck(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE);

	// (6)IDC_CLEAR_ON_RESIZE
	if (((pts->TermFlag & TF_CLEARONRESIZE) != 0) != GetCheck(IDC_CLEAR_ON_RESIZE)) {
		pts->TermFlag ^= TF_CLEARONRESIZE;
	}

	// (7)IDC_CURSOR_CHANGE_IME
	if (((pts->WindowFlag & WF_IMECURSORCHANGE) != 0) != GetCheck(IDC_CURSOR_CHANGE_IME)) {
		pts->WindowFlag ^= WF_IMECURSORCHANGE;
	}

	// (8)IDC_LIST_HIDDEN_FONTS
	// pts->ListHiddenFonts = GetCheck(IDC_LIST_HIDDEN_FONTS);

	// (9) Title Format
	pts->TitleFormat = GetCheck(IDC_TITLEFMT_DISPHOSTNAME) == BST_CHECKED;
	pts->TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPSESSION) == BST_CHECKED) << 1;
	pts->TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPVTTEK) == BST_CHECKED) << 2;
	pts->TitleFormat |= (GetCheck(IDC_TITLEFMT_SWAPHOSTTITLE) == BST_CHECKED) << 3;
	pts->TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPTCPPORT) == BST_CHECKED) << 4;
	pts->TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPSERIALSPEED) == BST_CHECKED) << 5;

	// Notify
	{
		BOOL notify_sound = (BOOL)GetCheck(IDC_NOTIFY_SOUND);
		if (notify_sound != pts->NotifySound) {
			pts->NotifySound = notify_sound;
			Notify2SetSound((NotifyIcon *)cv.NotifyIcon, notify_sound);
		}
	}

	{
		LRESULT w = SendDlgItemMessageA(IDC_GENUILANG, CB_GETCURSEL, 0, 0);
		if (w != data->selected_lang) {
			const LangInfo* p = data->lng_infos + w;
			free(pts->UILanguageFileW);
			pts->UILanguageFileW = _wcsdup(p->fullname);

			// タイトルの更新を行う。(2014.2.23 yutaka)
			PostMessage(HVTWin, WM_USER_CHANGETITLE, 0, 0);
		}

		// TTXKanjiMenu は Language を見てメニューを表示するので、変更の可能性がある
		// OK 押下時にメニュー再描画のメッセージを飛ばすようにした。 (2007.7.14 maya)
		// 言語ファイルの変更時にメニューの再描画が必要 (2012.5.5 maya)
		PostMessage(HVTWin, WM_USER_CHANGEMENU, 0, 0);
	}

	// default port
	{
		WORD w = (WORD)GetCurSel(IDC_GENPORT);
		if (w > 0) {
			pts->PortType = IdSerial;
			pts->ComPort = w;
		}
		else {
			pts->PortType = IdTCPIP;
		}
	}

	// Download dir
	free(pts->FileDirW);
	hGetDlgItemTextW(m_hWnd, IDC_DOWNLOAD_DIR, &pts->FileDirW);
	if (pts->FileDirW != NULL && pts->FileDirW[0] == 0) {
		free(pts->FileDirW);
		pts->FileDirW = NULL;
	}
}

void CGeneralPropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalGeneral, 0);
}

BOOL CGeneralPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_NOTIFICATION_TEST_POPUP | (BN_CLICKED << 16): {
			// popupを出すテスト
			NotifyIcon *ni = (NotifyIcon *)cv.NotifyIcon;
			const wchar_t *msg = L"Test button was pushed";
			BOOL prev_sound = Notify2GetSound(ni);
			BOOL notify_sound = (BOOL)GetCheck(IDC_NOTIFY_SOUND);
			Notify2SetSound(ni, notify_sound);
			Notify2SetMessageW(ni, msg, NULL, 1);
			Notify2SetSound(ni, prev_sound);
			break;
		}
		case IDC_NOTIFICATION_TEST_TRAY | (BN_CLICKED << 16): {
			// trayにiconを出す(自動で消えない)
			NotifyIcon *ni = (NotifyIcon *)cv.NotifyIcon;
			BOOL prev_sound = Notify2GetSound(ni);
			BOOL notify_sound = (BOOL)GetCheck(IDC_NOTIFY_SOUND);
			Notify2SetSound(ni, notify_sound);
			Notify2SetBallonDontHide(ni, TRUE);
			Notify2SetMessageW(ni, NULL, NULL, 1);
			Notify2SetSound(ni, prev_sound);

			static const wchar_t *msg =
				L"Now icon is displayed in the notification area (task tray),\n"
				L"and can be turned on or off in Windows notification setting.";
			static const TTMessageBoxInfoW info = {
				"Tera Term",
				"MSG_TT_NOTICE", L"Tera Term: Notice",
				"DLG_TAB_GENERAL_NOTIFICATION_TEST_MESSAGE", msg,
				MB_OK };
			TTMessageBoxW(m_hWnd, &info, ts.UILanguageFileW);

			// 通知領域のアイコンを消す
			Notify2SetBallonDontHide(ni, FALSE);
			Notify2Hide(ni);
			break;
		}
		case IDC_DOWNLOAD_DIR_SELECT | (BN_CLICKED << 16): {
			wchar_t *uimsgW;
			wchar_t *src;
			wchar_t *dest;
			GetI18nStrWW("Tera Term", "DLG_SELECT_DIR_TITLE", L"Select new directory", ts.UILanguageFileW, &uimsgW);

			hGetDlgItemTextW(m_hWnd, IDC_DOWNLOAD_DIR, &src);
			if (src != NULL && src[0] == 0) {
				free(src);
				// Windowsのデフォルトのダウンロードフォルダ
				_SHGetKnownFolderPath(FOLDERID_Downloads, KF_FLAG_CREATE, NULL, &src);
			}
			else {
				wchar_t *FileDirExpanded;
				hExpandEnvironmentStringsW(src, &FileDirExpanded);
				free(src);
				src = FileDirExpanded;
			}
			if (doSelectFolderW(m_hWnd, src, uimsgW, &dest)) {
				SetDlgItemTextW(IDC_DOWNLOAD_DIR, dest);
				free(dest);
			}
			free(src);
			free(uimsgW);
			break;
		}
		case IDC_GENUILANG | (CBN_SELCHANGE << 16): {
			size_t ui_sel = (size_t)SendDlgItemMessageA(IDC_GENUILANG, CB_GETCURSEL, 0, 0);
			wchar_t *info_text = LangInfoText(data->lng_infos + ui_sel);
			TipWin2SetTextW(data->tipwin2, IDC_GENUILANG, info_text);
			free(info_text);
			break;
		}
		default:
			break;
	}
	return TTCPropertyPage::OnCommand(wParam, lParam);
}
