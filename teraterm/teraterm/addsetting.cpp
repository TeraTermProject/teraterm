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

/*
 * Additional settings dialog
 */

#include <stdio.h>
#include <windows.h>
//#include <dwmapi.h>	// compat_win.h 内の定義を使用するため include しない
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "ttwinman.h"	// for ts
#include "dlglib.h"
#include "compat_win.h"
#include "helpid.h"
#include "tipwin.h"
#include "i18n.h"
#include "asprintf.h"
#include "win32helper.h"
#include "theme.h"

// proparty page
#include "debug_pp.h"
#include "coding_pp.h"
#include "font_pp.h"
#include "theme_pp.h"
#include "general_pp.h"
#include "keyboard_pp.h"
#include "mouse_pp.h"
#include "log_pp.h"
#include "tcpip_pp.h"
#include "term_pp.h"
#include "win_pp.h"
#include "serial_pp.h"
#include "ui_pp.h"
#include "tekfont_pp.h"
#include "plugin_pp.h"

#include "addsetting.h"

const mouse_cursor_t MouseCursor[] = {
	{"ARROW", IDC_ARROW},
	{"IBEAM", IDC_IBEAM},
	{"CROSS", IDC_CROSS},
	{"HAND", IDC_HAND},
	{NULL, NULL},
};
#define MOUSE_CURSOR_MAX (sizeof(MouseCursor)/sizeof(MouseCursor[0]) - 1)

// CSequencePropPageDlg ダイアログ
class CSequencePropPageDlg : public TTCPropertyPage
{
public:
	CSequencePropPageDlg(HINSTANCE inst);
	virtual ~CSequencePropPageDlg();
private:
	void OnInitDialog();
	void OnOK();
	enum { IDD = IDD_TABSHEET_SEQUENCE };
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnHelp();
};

CSequencePropPageDlg::CSequencePropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CSequencePropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_SEQUENCE",
				 L"Control Sequence", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
}

CSequencePropPageDlg::~CSequencePropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CSequencePropPageDlg メッセージ ハンドラ

void CSequencePropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_ACCEPT_MOUSE_EVENT_TRACKING, "DLG_TAB_SEQUENCE_ACCEPT_MOUSE_EVENT_TRACKING" },
		{ IDC_DISABLE_MOUSE_TRACKING_CTRL, "DLG_TAB_SEQUENCE_DISABLE_MOUSE_TRACKING_CTRL" },
		{ IDC_ACCEPT_TITLE_CHANGING_LABEL, "DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING" },
		{ IDC_CURSOR_CTRL_SEQ, "DLG_TAB_SEQUENCE_CURSOR_CTRL" },
		{ IDC_WINDOW_CTRL, "DLG_TAB_SEQUENCE_WINDOW_CTRL" },
		{ IDC_WINDOW_REPORT, "DLG_TAB_SEQUENCE_WINDOW_REPORT" },
		{ IDC_TITLE_REPORT_LABEL, "DLG_TAB_SEQUENCE_TITLE_REPORT" },
		{ IDC_CLIPBOARD_ACCESS_LABEL, "DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS" },
		{ IDC_CLIPBOARD_NOTIFY, "DLG_TAB_SEQUENCE_CLIPBOARD_NOTIFY" },
		{ IDC_ACCEPT_CLEAR_SBUFF, "DLG_TAB_SEQUENCE_ACCEPT_CLEAR_SBUFF" },
		{ IDC_DISABLE_PRINT_START, "DLG_TAB_SEQUENCE_PRINT_START" },
		{ IDC_BEEP_LABEL, "DLG_TAB_SEQUENCE_BEEP_LABEL" },
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	const static I18nTextInfo accept_title_changing[] = {
		{ "DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_OFF", L"off" },
		{ "DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_OVERWRITE", L"overwrite" },
		{ "DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_AHEAD", L"ahead" },
		{ "DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_LAST", L"last" },
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_ACCEPT_TITLE_CHANGING, accept_title_changing, _countof(accept_title_changing),
				 ts.UILanguageFileW, 0);

	const static I18nTextInfo sequence_title_report[] = {
		{ "DLG_TAB_SEQUENCE_TITLE_REPORT_IGNORE", L"ignore" },
		{ "DLG_TAB_SEQUENCE_TITLE_REPORT_ACCEPT", L"accept" },
		{ "DLG_TAB_SEQUENCE_TITLE_REPORT_EMPTY", L"empty" },
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_TITLE_REPORT, sequence_title_report, _countof(sequence_title_report),
				 ts.UILanguageFileW, 0);

	const static I18nTextInfo sequence_clipboard_access[] = {
		{ "DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_OFF", L"off" },
		{ "DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_WRITE", L"write only" },
		{ "DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_READ", L"read only" },
		{ "DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_ON", L"read/write" },
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_CLIPBOARD_ACCESS, sequence_clipboard_access,
				 _countof(sequence_clipboard_access), ts.UILanguageFileW, 0);

	const static I18nTextInfo beep_type[] = {
		{"DLG_TAB_SEQUENCE_BEEP_DISABLE", L"disable"},		  // IdBeepOff = 0
		{"DLG_TAB_SEQUENCE_BEEP_SOUND", L"sound"},			  // IdBeepOn = 1
		{"DLG_TAB_SEQUENCE_BEEP_VISUALBELL", L"visualbell"},  // IdBeepVisual = 2
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_BEEP_DROPDOWN, beep_type, _countof(beep_type), ts.UILanguageFileW, ts.Beep);

	// (1)IDC_ACCEPT_MOUSE_EVENT_TRACKING
	SetCheck(IDC_ACCEPT_MOUSE_EVENT_TRACKING, ts.MouseEventTracking);
	EnableDlgItem(IDC_DISABLE_MOUSE_TRACKING_CTRL, ts.MouseEventTracking ? TRUE : FALSE);

	// (2)IDC_DISABLE_MOUSE_TRACKING_CTRL
	SetCheck(IDC_DISABLE_MOUSE_TRACKING_CTRL, ts.DisableMouseTrackingByCtrl);

	// (3)IDC_ACCEPT_TITLE_CHANGING
	SetCurSel(IDC_ACCEPT_TITLE_CHANGING, ts.AcceptTitleChangeRequest);

	// (4)IDC_TITLE_REPORT
	SetCurSel(IDC_TITLE_REPORT,
			  (ts.WindowFlag & WF_TITLEREPORT) == IdTitleReportIgnore ? 0 :
			  (ts.WindowFlag & WF_TITLEREPORT) == IdTitleReportAccept ? 1
			  /*(ts.WindowFlag & WF_TITLEREPORT) == IdTitleReportEmptye ? */ : 2);

	// (5)IDC_WINDOW_CTRL
	SetCheck(IDC_WINDOW_CTRL, (ts.WindowFlag & WF_WINDOWCHANGE) != 0);

	// (6)IDC_WINDOW_REPORT
	SetCheck(IDC_WINDOW_REPORT, (ts.WindowFlag & WF_WINDOWREPORT) != 0);

	// (7)IDC_CURSOR_CTRL_SEQ
	SetCheck(IDC_CURSOR_CTRL_SEQ, (ts.WindowFlag & WF_CURSORCHANGE) != 0);

	// (8)IDC_CLIPBOARD_ACCESS
	SetCurSel(IDC_CLIPBOARD_ACCESS,
			  (ts.CtrlFlag & CSF_CBRW) == CSF_CBRW ? 3 :
			  (ts.CtrlFlag & CSF_CBRW) == CSF_CBREAD ? 2 :
			  (ts.CtrlFlag & CSF_CBRW) == CSF_CBWRITE ? 1 :
			  0);	// off

	// (9)IDC_CLIPBOARD_NOTIFY
	SetCheck(IDC_CLIPBOARD_NOTIFY, ts.NotifyClipboardAccess);
	EnableDlgItem(IDC_CLIPBOARD_NOTIFY, HasBalloonTipSupport() ? TRUE : FALSE);

	// (10)IDC_ACCEPT_CLEAR_SBUFF
	SetCheck(IDC_ACCEPT_CLEAR_SBUFF, (ts.TermFlag & TF_REMOTECLEARSBUFF) != 0);

	// IDC_DISABLE_PRINT_START
	SetCheck(IDC_DISABLE_PRINT_START, (ts.TermFlag & TF_PRINTERCTRL) == 0);

	// IDC_BEEP_DROPDOWN
	SetCurSel(IDC_BEEP_DROPDOWN, ts.Beep);

	// ダイアログにフォーカスを当てる (2004.12.7 yutaka)
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_ACCEPT_MOUSE_EVENT_TRACKING));
}

BOOL CSequencePropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_ACCEPT_MOUSE_EVENT_TRACKING | (BN_CLICKED << 16):
			EnableDlgItem(IDC_DISABLE_MOUSE_TRACKING_CTRL,
						  GetCheck(IDC_ACCEPT_MOUSE_EVENT_TRACKING) ? TRUE : FALSE);
			return TRUE;
	}
	return TTCPropertyPage::OnCommand(wParam, lParam);
}

void CSequencePropPageDlg::OnOK()
{
	// (1)IDC_ACCEPT_MOUSE_EVENT_TRACKING
	ts.MouseEventTracking = GetCheck(IDC_ACCEPT_MOUSE_EVENT_TRACKING);

	// (2)IDC_DISABLE_MOUSE_TRACKING_CTRL
	ts.DisableMouseTrackingByCtrl = GetCheck(IDC_DISABLE_MOUSE_TRACKING_CTRL);

	// (3)IDC_ACCEPT_TITLE_CHANGING
	int sel = GetCurSel(IDC_ACCEPT_TITLE_CHANGING);
	if (0 <= sel && sel <= IdTitleChangeRequestMax) {
		ts.AcceptTitleChangeRequest = sel;
	}

	// (4)IDC_TITLE_REPORT
	switch (GetCurSel(IDC_TITLE_REPORT)) {
		case 0:
			ts.WindowFlag &= ~WF_TITLEREPORT;
			break;
		case 1:
			ts.WindowFlag &= ~WF_TITLEREPORT;
			ts.WindowFlag |= IdTitleReportAccept;
			break;
		case 2:
			ts.WindowFlag |= IdTitleReportEmpty;
			break;
		default: // Invalid value.
			break;
	}

	// (5)IDC_WINDOW_CTRL
	if (((ts.WindowFlag & WF_WINDOWCHANGE) != 0) != GetCheck(IDC_WINDOW_CTRL)) {
		ts.WindowFlag ^= WF_WINDOWCHANGE;
	}

	// (6)IDC_WINDOW_REPORT
	if (((ts.WindowFlag & WF_WINDOWREPORT) != 0) != GetCheck(IDC_WINDOW_REPORT)) {
		ts.WindowFlag ^= WF_WINDOWREPORT;
	}

	// (7)IDC_CURSOR_CTRL_SEQ
	if (((ts.WindowFlag & WF_CURSORCHANGE) != 0) != GetCheck(IDC_CURSOR_CTRL_SEQ)) {
		ts.WindowFlag ^= WF_CURSORCHANGE;
	}

	// (8)IDC_CLIPBOARD_ACCESS
	switch (GetCurSel(IDC_CLIPBOARD_ACCESS)) {
		case 0: // off
			ts.CtrlFlag &= ~CSF_CBRW;
			break;
		case 1: // write only
			ts.CtrlFlag &= ~CSF_CBRW;
			ts.CtrlFlag |= CSF_CBWRITE;
			break;
		case 2: // read only
			ts.CtrlFlag &= ~CSF_CBRW;
			ts.CtrlFlag |= CSF_CBREAD;
			break;
		case 3: // read/write
			ts.CtrlFlag |= CSF_CBRW;
			break;
		default: // Invalid value.
			break;
	}

	// (9)IDC_CLIPBOARD_ACCESS
	ts.NotifyClipboardAccess = GetCheck(IDC_CLIPBOARD_NOTIFY);

	// (10)IDC_ACCEPT_CLEAR_SBUFF
	if (((ts.TermFlag & TF_REMOTECLEARSBUFF) != 0) != GetCheck(IDC_ACCEPT_CLEAR_SBUFF)) {
		ts.TermFlag ^= TF_REMOTECLEARSBUFF;
	}

	if (GetCheck(IDC_DISABLE_PRINT_START) == 0) {
		ts.TermFlag |= TF_PRINTERCTRL;
	} else {
		ts.TermFlag &= ~TF_PRINTERCTRL;
	}

	ts.Beep = GetCurSel(IDC_BEEP_DROPDOWN);
}

void CSequencePropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalCtrlSeq, 0);
}

// CCopypastePropPageDlg ダイアログ
class CCopypastePropPageDlg : public TTCPropertyPage
{
public:
	CCopypastePropPageDlg(HINSTANCE inst);
	virtual ~CCopypastePropPageDlg();
private:
	void OnInitDialog();
	void OnOK();
	enum { IDD = IDD_TABSHEET_COPYPASTE };
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnHelp();
};

CCopypastePropPageDlg::CCopypastePropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CCopypastePropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_COPYPASTE",
				 L"Copy and Paste", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
}

CCopypastePropPageDlg::~CCopypastePropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CCopypastePropPageDlg メッセージ ハンドラ

void CCopypastePropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_LINECOPY, "DLG_TAB_COPYPASTE_CONTINUE" },
		{ IDC_DISABLE_PASTE_RBUTTON, "DLG_TAB_COPYPASTE_MOUSEPASTE" },
		{ IDC_CONFIRM_PASTE_RBUTTON, "DLG_TAB_COPYPASTE_CONFIRMPASTE" },
		{ IDC_DISABLE_PASTE_MBUTTON, "DLG_TAB_COPYPASTE_MOUSEPASTEM" },
		{ IDC_SELECT_LBUTTON, "DLG_TAB_COPYPASTE_SELECTLBUTTON" },
		{ IDC_TRIMNLCHAR, "DLG_TAB_COPYPASTE_TRIM_TRAILING_NL" },
		{ IDC_CONFIRM_CHANGE_PASTE, "DLG_TAB_COPYPASTE_CONFIRM_CHANGE_PASTE" },
		{ IDC_CONFIRM_STRING_FILE_LABEL, "DLG_TAB_COPYPASTE_STRINGFILE" },
		{ IDC_DELIMITER, "DLG_TAB_COPYPASTE_DELIMITER" },
		{ IDC_PASTEDELAY_LABEL, "DLG_TAB_COPYPASTE_PASTEDELAY" },
		{ IDC_PASTEDELAY_LABEL2, "DLG_TAB_COPYPASTE_PASTEDELAY2" },
		{ IDC_SELECT_ON_ACTIVATE, "DLG_TAB_COPYPASTE_SELECT_ON_ACTIVATE" },
		{ IDC_AUTO_TEXT_COPY, "DLG_TAB_COPYPASTE_AUTO_TEXT_COPY" },
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	// (1)Enable continued-line copy
	SetCheck(IDC_LINECOPY, ts.EnableContinuedLineCopy);

	// (2)DisablePasteMouseRButton
	if (ts.PasteFlag & CPF_DISABLE_RBUTTON) {
		SetCheck(IDC_DISABLE_PASTE_RBUTTON, BST_CHECKED);
		EnableDlgItem(IDC_CONFIRM_PASTE_RBUTTON, FALSE);
	} else {
		SetCheck(IDC_DISABLE_PASTE_RBUTTON, BST_UNCHECKED);
		EnableDlgItem(IDC_CONFIRM_PASTE_RBUTTON, TRUE);
	}

	// (3)ConfirmPasteMouseRButton
	SetCheck(IDC_CONFIRM_PASTE_RBUTTON, (ts.PasteFlag & CPF_CONFIRM_RBUTTON)?BST_CHECKED:BST_UNCHECKED);

	// (4)DisablePasteMouseMButton
	SetCheck(IDC_DISABLE_PASTE_MBUTTON, (ts.PasteFlag & CPF_DISABLE_MBUTTON)?BST_CHECKED:BST_UNCHECKED);

	// (5)SelectOnlyByLButton
	SetCheck(IDC_SELECT_LBUTTON, ts.SelectOnlyByLButton);

	// (6)TrimTrailingNLonPaste
	SetCheck(IDC_TRIMNLCHAR, (ts.PasteFlag & CPF_TRIM_TRAILING_NL)?BST_CHECKED:BST_UNCHECKED);

	// (7)ConfirmChangePaste
	SetCheck(IDC_CONFIRM_CHANGE_PASTE, (ts.PasteFlag & CPF_CONFIRM_CHANGEPASTE)?BST_CHECKED:BST_UNCHECKED);

	// ファイルパス
	SetDlgItemTextA(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile);
	if (ts.PasteFlag & CPF_CONFIRM_CHANGEPASTE) {
		EnableDlgItem(IDC_CONFIRM_STRING_FILE, TRUE);
		EnableDlgItem(IDC_CONFIRM_STRING_FILE_PATH, TRUE);
	} else {
		EnableDlgItem(IDC_CONFIRM_STRING_FILE, FALSE);
		EnableDlgItem(IDC_CONFIRM_STRING_FILE_PATH, FALSE);
	}

	// (8)delimiter characters
	SetDlgItemTextW(IDC_DELIM_LIST, ts.DelimListW);

	// (9)PasteDelayPerLine
	SetDlgItemNum(IDC_PASTEDELAY_EDIT, ts.PasteDelayPerLine);

	// (10) SelectOnActivate
	SetCheck(IDC_SELECT_ON_ACTIVATE, ts.SelOnActive ? BST_CHECKED : BST_UNCHECKED);

	// (11) auto text copy
	SetCheck(IDC_AUTO_TEXT_COPY, ts.AutoTextCopy ? BST_CHECKED : BST_UNCHECKED);

	// ダイアログにフォーカスを当てる
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_LINECOPY));
}

BOOL CCopypastePropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_DISABLE_PASTE_RBUTTON | (BN_CLICKED << 16):
			EnableDlgItem(IDC_CONFIRM_PASTE_RBUTTON,
						  GetCheck(IDC_DISABLE_PASTE_RBUTTON) ? FALSE : TRUE);
			return TRUE;

		case IDC_CONFIRM_CHANGE_PASTE | (BN_CLICKED << 16):
			if (GetCheck(IDC_CONFIRM_CHANGE_PASTE)) {
				EnableDlgItem(IDC_CONFIRM_STRING_FILE, TRUE);
				EnableDlgItem(IDC_CONFIRM_STRING_FILE_PATH, TRUE);
			} else {
				EnableDlgItem(IDC_CONFIRM_STRING_FILE, FALSE);
				EnableDlgItem(IDC_CONFIRM_STRING_FILE_PATH, FALSE);
			}
			return TRUE;

		case IDC_CONFIRM_STRING_FILE_PATH | (BN_CLICKED << 16):
			{
				wchar_t *def;
				hGetDlgItemTextW(m_hWnd, IDC_CONFIRM_STRING_FILE, &def);

				TTOPENFILENAMEW ofn = {};
				ofn.hwndOwner = m_hWnd;GetSafeHwnd();
				ofn.lpstrFilter = TTGetLangStrW("Tera Term", "FILEDLG_SELECT_CONFIRM_STRING_APP_FILTER", L"txt(*.txt)\\0*.txt\\0all(*.*)\\0*.*\\0\\0", ts.UILanguageFileW);
				ofn.lpstrFile = def;
				ofn.lpstrTitle = TTGetLangStrW("Tera Term", "FILEDLG_SELECT_CONFIRM_STRING_APP_TITLE", L"Choose a file including strings for ConfirmChangePaste", ts.UILanguageFileW);
				ofn.lpstrInitialDir = ts.HomeDirW;
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				wchar_t *filename;
				BOOL ok = TTGetOpenFileNameW(&ofn, &filename);
				if (ok) {
					SetDlgItemTextW(IDC_CONFIRM_STRING_FILE, filename);
					free(filename);
				}
				free(def);
				free((void *)ofn.lpstrFilter);
				free((void *)ofn.lpstrTitle);
			}
			return TRUE;
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

void CCopypastePropPageDlg::OnOK()
{
	int val;

	// (1)
	ts.EnableContinuedLineCopy = GetCheck(IDC_LINECOPY);

	// (2)
	if (GetCheck(IDC_DISABLE_PASTE_RBUTTON)) {
		ts.PasteFlag |= CPF_DISABLE_RBUTTON;
	}
	else {
		ts.PasteFlag &= ~CPF_DISABLE_RBUTTON;
	}

	// (3)
	if (GetCheck(IDC_CONFIRM_PASTE_RBUTTON)) {
		ts.PasteFlag |= CPF_CONFIRM_RBUTTON;
	}
	else {
		ts.PasteFlag &= ~CPF_CONFIRM_RBUTTON;
	}

	// (4)
	if (GetCheck(IDC_DISABLE_PASTE_MBUTTON)) {
		ts.PasteFlag |= CPF_DISABLE_MBUTTON;
	}
	else {
		ts.PasteFlag &= ~CPF_DISABLE_MBUTTON;
	}

	// (5)
	ts.SelectOnlyByLButton = GetCheck(IDC_SELECT_LBUTTON);

	// (6)
	if (GetCheck(IDC_TRIMNLCHAR)) {
		ts.PasteFlag |= CPF_TRIM_TRAILING_NL;
	}
	else {
		ts.PasteFlag &= ~CPF_TRIM_TRAILING_NL;
	}

	// (7)IDC_CONFIRM_CHANGE_PASTE
	if (GetCheck(IDC_CONFIRM_CHANGE_PASTE)) {
		ts.PasteFlag |= CPF_CONFIRM_CHANGEPASTE;
	}
	else {
		ts.PasteFlag &= ~CPF_CONFIRM_CHANGEPASTE;
	}
	GetDlgItemTextA(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile, sizeof(ts.ConfirmChangePasteStringFile));

	// (8)
	free(ts.DelimListW);
	hGetDlgItemTextW(this->m_hWnd, IDC_DELIM_LIST, &ts.DelimListW);

	// (9)
	val = GetDlgItemInt(IDC_PASTEDELAY_EDIT);
	ts.PasteDelayPerLine =
		(val < 0) ? 0 :
		(val > 5000) ? 5000 : val;

	// (10) SelectOnActivate
	ts.SelOnActive = (GetCheck(IDC_SELECT_ON_ACTIVATE) == BST_CHECKED);

	// (11) auto text copy
	ts.AutoTextCopy = (GetCheck(IDC_AUTO_TEXT_COPY) == BST_CHECKED);
}

void CCopypastePropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalCopyAndPaste, 0);
}

// CVisualPropPageDlg ダイアログ
class CVisualPropPageDlg : public TTCPropertyPage
{
public:
	CVisualPropPageDlg(HINSTANCE inst);
	virtual ~CVisualPropPageDlg();
private:
	void OnInitDialog();
	void OnOK();
	HBRUSH OnCtlColor(HDC hDC, HWND hWnd);
	enum { IDD = IDD_TABSHEET_VISUAL };
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnHScroll(UINT nSBCode, UINT nPos, HWND pScrollBar);
	void SetupRGBbox(int index);
	void OnHelp();
	BOOL CheckColorChanged();
	BOOL CheckThemeColor();
	CTipWin* TipWin;
	COLORREF ANSIColor[16];
};

CVisualPropPageDlg::CVisualPropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CVisualPropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_VISUAL",
				 L"Visual", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
	TipWin = new CTipWin(inst);
}

CVisualPropPageDlg::~CVisualPropPageDlg()
{
	free((void *)m_psp.pszTitle);
	TipWin->Destroy();
	delete TipWin;
	TipWin = NULL;
}

void CVisualPropPageDlg::SetupRGBbox(int index)
{
	COLORREF Color = ANSIColor[index];
	BYTE c;

	c = GetRValue(Color);
	SetDlgItemNum(IDC_COLOR_RED, c);

	c = GetGValue(Color);
	SetDlgItemNum(IDC_COLOR_GREEN, c);

	c = GetBValue(Color);
	SetDlgItemNum(IDC_COLOR_BLUE, c);
}

// CVisualPropPageDlg メッセージ ハンドラ

void CVisualPropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_ALPHABLEND, "DLG_TAB_VISUAL_ALPHA" },
		{ IDC_ALPHA_BLEND_ACTIVE_LABEL, "DLG_TAB_VISUAL_ALPHA_ACTIVE" },
		{ IDC_ALPHA_BLEND_INACTIVE_LABEL, "DLG_TAB_VISUAL_ALPHA_INACTIVE" },
		{ IDC_MOUSE, "DLG_TAB_VISUAL_MOUSE" },
		{ IDC_FONT_QUALITY_LABEL, "DLG_TAB_VISUAL_FONT_QUALITY" },
		{ IDC_ANSICOLOR, "DLG_TAB_VISUAL_ANSICOLOR" },
		{ IDC_RED, "DLG_TAB_VISUAL_RED" },
		{ IDC_GREEN, "DLG_TAB_VISUAL_GREEN" },
		{ IDC_BLUE, "DLG_TAB_VISUAL_BLUE" },
		{ IDC_CHECK_CORNERDONTROUND, "DLG_TAB_VISUAL_CORNER_DONT_ROUND" },
		{ IDC_ENABLE_ATTR_COLOR_BOLD, "DLG_TAB_VISUAL_BOLD_COLOR" },		// SGR 1
		{ IDC_ENABLE_ATTR_FONT_BOLD, "DLG_TAB_VISUAL_BOLD_FONT" },
		{ IDC_ENABLE_ATTR_COLOR_UNDERLINE, "DLG_TAB_VISUAL_UNDERLINE_COLOR" },	// SGR 4
		{ IDC_ENABLE_ATTR_FONT_UNDERLINE, "DLG_TAB_VISUAL_UNDERLINE_FONT" },
		{ IDC_ENABLE_ATTR_COLOR_BLINK, "DLG_TAB_VISUAL_BLINK" },			// SGR 5
		{ IDC_ENABLE_ATTR_COLOR_REVERSE, "DLG_TAB_VISUAL_REVERSE" },		// SGR 7
		{ IDC_ENABLE_ATTR_COLOR_URL, "DLG_TAB_VISUAL_URL_COLOR" },			// URL Attribute
		{ IDC_ENABLE_ATTR_FONT_URL, "DLG_TAB_VISUAL_URL_FONT" },
		{ IDC_ENABLE_ANSI_COLOR, "DLG_TAB_VISUAL_ANSI" },
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	const static I18nTextInfo visual_font_quality[] = {
		{ "DLG_TAB_VISUAL_FONT_QUALITY_DEFAULT", L"Default" },
		{ "DLG_TAB_VISUAL_FONT_QUALITY_NONANTIALIASED", L"Non-Antialiased" },
		{ "DLG_TAB_VISUAL_FONT_QUALITY_ANTIALIASED", L"Antialiased" },
		{ "DLG_TAB_VISUAL_FONT_QUALITY_CLEARTYPE", L"ClearType" },
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_FONT_QUALITY, visual_font_quality, _countof(visual_font_quality),
				 ts.UILanguageFileW, 0);

	// (1)AlphaBlend

	SetDlgItemNum(IDC_ALPHA_BLEND_ACTIVE, ts.AlphaBlendActive);
	SendDlgItemMessage(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
	SendDlgItemMessage(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR, TBM_SETPOS, TRUE, ts.AlphaBlendActive);

	SetDlgItemNum(IDC_ALPHA_BLEND_INACTIVE, ts.AlphaBlendInactive);
	SendDlgItemMessage(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
	SendDlgItemMessage(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR, TBM_SETPOS, TRUE, ts.AlphaBlendInactive);

	BOOL isLayeredWindowSupported = (pSetLayeredWindowAttributes != NULL);
	EnableDlgItem(IDC_ALPHA_BLEND_ACTIVE, isLayeredWindowSupported);
	EnableDlgItem(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR, isLayeredWindowSupported);
	EnableDlgItem(IDC_ALPHA_BLEND_INACTIVE, isLayeredWindowSupported);
	EnableDlgItem(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR, isLayeredWindowSupported);

	// (2)Mouse cursor type
	int sel = 0;
	for (int i = 0 ; MouseCursor[i].name ; i++) {
		const char *name = MouseCursor[i].name;
		SendDlgItemMessageA(IDC_MOUSE_CURSOR, CB_ADDSTRING, i, (LPARAM)name);
		if (_stricmp(name, ts.MouseCursorName) == 0) {
			sel = i;
		}
	}
	SetCurSel(IDC_MOUSE_CURSOR, sel);

	// (3)Font quality
	switch (ts.FontQuality) {
		case DEFAULT_QUALITY:
			SetCurSel(IDC_FONT_QUALITY, 0);
			break;
		case NONANTIALIASED_QUALITY:
			SetCurSel(IDC_FONT_QUALITY, 1);
			break;
		case ANTIALIASED_QUALITY:
			SetCurSel(IDC_FONT_QUALITY, 2);
			break;
		default: // CLEARTYPE_QUALITY
			SetCurSel(IDC_FONT_QUALITY, 3);
			break;
	}

	// (4)ANSI color
	for (int i = 0; i < 16; i++) {
		ANSIColor[i] = ts.ANSIColor[i];
	}
	for (int i = 0 ; i < 16 ; i++) {
		char buf[4];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", i);
		SendDlgItemMessageA(IDC_ANSI_COLOR, LB_INSERTSTRING, i, (LPARAM)buf);
	}
	SetupRGBbox(0);
	SendDlgItemMessage(IDC_ANSI_COLOR, LB_SETCURSEL, 0, 0);
	::InvalidateRect(GetDlgItem(IDC_SAMPLE_COLOR), NULL, TRUE);

	// (5)Bold Attr Color
	SetCheck(IDC_ENABLE_ATTR_COLOR_BOLD, (ts.ColorFlag&CF_BOLDCOLOR) != 0);
	SetCheck(IDC_ENABLE_ATTR_FONT_BOLD, (ts.FontFlag&FF_BOLD) != 0);

	// (6)Blink Attr Color
	SetCheck(IDC_ENABLE_ATTR_COLOR_BLINK, (ts.ColorFlag&CF_BLINKCOLOR) != 0);

	// (7)Reverse Attr Color
	SetCheck(IDC_ENABLE_ATTR_COLOR_REVERSE, (ts.ColorFlag&CF_REVERSECOLOR) != 0);

	// Underline Attr
	SetCheck(IDC_ENABLE_ATTR_COLOR_UNDERLINE, (ts.ColorFlag&CF_UNDERLINE) != 0);
	SetCheck(IDC_ENABLE_ATTR_FONT_UNDERLINE, (ts.FontFlag&FF_UNDERLINE) != 0);

	// URL Underline Attr
	SetCheck(IDC_ENABLE_ATTR_COLOR_URL, (ts.ColorFlag&CF_URLCOLOR) != 0);
	SetCheck(IDC_ENABLE_ATTR_FONT_URL, (ts.FontFlag&FF_URLUNDERLINE) != 0);

	// Color
	SetCheck(IDC_ENABLE_ANSI_COLOR, (ts.ColorFlag&CF_ANSICOLOR) != 0);

	SetCheck(IDC_CHECK_FLICKER_LESS_MOVE, ts.EtermLookfeel.BGNoCopyBits != 0);

	// ウィンドウの角を丸くしない
	SetCheck(IDC_CHECK_CORNERDONTROUND, (ts.WindowCornerDontround) != 0);
	{
		DWM_WINDOW_CORNER_PREFERENCE preference;
		if (pDwmGetWindowAttribute == NULL ||
			pDwmGetWindowAttribute(HVTWin, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference)) != S_OK) {
			// ウィンドウの角を丸くしないに対応していないなら disable にする
			//		DwmGetWindowAttribute() API がない or
			//		DwmGetWindowAttribute(DWMWA_WINDOW_CORNER_PREFERENCE) で S_OK が返らない
			EnableDlgItem(IDC_CHECK_CORNERDONTROUND, FALSE);
		}
	}

	// ダイアログにフォーカスを当てる
	::SetFocus(GetDlgItem(IDC_ALPHA_BLEND_ACTIVE));

	// ツールチップ作成
	TipWin->Create(m_hWnd);
}

void CVisualPropPageDlg::OnHScroll(UINT nSBCode, UINT nPos, HWND pScrollBar)
{
	int pos;
	if ( pScrollBar == GetDlgItem(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR) ) {
		switch (nSBCode) {
			case SB_TOP:
			case SB_BOTTOM:
			case SB_LINEDOWN:
			case SB_LINEUP:
			case SB_PAGEDOWN:
			case SB_PAGEUP:
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				pos = (int)SendDlgItemMessage(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR, TBM_GETPOS, NULL, NULL);
				SetDlgItemNum(IDC_ALPHA_BLEND_ACTIVE, pos);
				break;
			case SB_ENDSCROLL:
			default:
				return;
		}
	}
	else if ( pScrollBar == GetDlgItem(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR) ) {
		switch (nSBCode) {
			case SB_TOP:
			case SB_BOTTOM:
			case SB_LINEDOWN:
			case SB_LINEUP:
			case SB_PAGEDOWN:
			case SB_PAGEUP:
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				pos = (int)SendDlgItemMessage(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR, TBM_GETPOS, NULL, NULL);
				SetDlgItemNum(IDC_ALPHA_BLEND_INACTIVE, pos);
				break;
			case SB_ENDSCROLL:
			default:
				return;
		}
	}
}

static void OpacityTooltip(CTipWin* tip, HWND hDlg, int trackbar, int pos, const wchar_t *UILanguageFile)
{
	wchar_t *uimsg;
	GetI18nStrWW("Tera Term", "TOOLTIP_TITLEBAR_OPACITY", L"Opacity %.1f %%", UILanguageFile, &uimsg);
	wchar_t *tipbuf;
	aswprintf(&tipbuf, uimsg, (pos / 255.0) * 100);
	RECT rc;
	::GetWindowRect(::GetDlgItem(hDlg, trackbar), &rc);
	tip->SetText(tipbuf);
	tip->SetPos(rc.right, rc.bottom);
	tip->SetHideTimer(1000);
	if (! tip->IsVisible()) {
		tip->SetVisible(TRUE);
	}
	free(tipbuf);
	free(uimsg);
}

BOOL CVisualPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_ANSI_COLOR | (LBN_SELCHANGE << 16): {
			int sel = (int)SendDlgItemMessage(IDC_ANSI_COLOR, LB_GETCURSEL, 0, 0);
			if (sel != -1) {
				SetupRGBbox(sel);
				::InvalidateRect(GetDlgItem(IDC_SAMPLE_COLOR), NULL, TRUE);
			}
			return TRUE;
		}

		case IDC_COLOR_RED | (EN_CHANGE << 16) :
		case IDC_COLOR_GREEN | (EN_CHANGE << 16) :
		case IDC_COLOR_BLUE | (EN_CHANGE << 16) :
			{
				int r, g, b;
				int sel;

				sel = GetCurSel(IDC_ANSI_COLOR);
				if (sel < 0 || sel > _countof(ANSIColor)-1) {
					return TRUE;
				}

				r = GetDlgItemInt(IDC_COLOR_RED);
				if (r < 0) {
					r = 0;
					SetDlgItemNum(IDC_COLOR_RED, r);
				}
				else if (r > 255) {
					r = 255;
					SetDlgItemNum(IDC_COLOR_RED, r);
				}

				g = GetDlgItemInt(IDC_COLOR_GREEN);
				if (g < 0) {
					g = 0;
					SetDlgItemNum(IDC_COLOR_GREEN, g);
				}
				else if (g > 255) {
					g = 255;
					SetDlgItemNum(IDC_COLOR_GREEN, g);
				}

				b = GetDlgItemInt(IDC_COLOR_BLUE);
				if (b < 0) {
					b = 0;
					SetDlgItemNum(IDC_COLOR_BLUE, b);
				}
				else if (b > 255) {
					b = 255;
					SetDlgItemNum(IDC_COLOR_BLUE, b);
				}

				ANSIColor[sel] = RGB(r, g, b);

				::InvalidateRect(GetDlgItem(IDC_SAMPLE_COLOR), NULL, TRUE);
			}
			return TRUE;
		case IDC_ALPHA_BLEND_ACTIVE | (EN_CHANGE << 16):
			{
				int pos;
				pos = GetDlgItemInt(IDC_ALPHA_BLEND_ACTIVE);
				if(pos < 0) {
					pos = 0;
					SetDlgItemNum(IDC_ALPHA_BLEND_ACTIVE, pos);
				}
				else if(pos > 255) {
					pos = 255;
					SetDlgItemNum(IDC_ALPHA_BLEND_ACTIVE, pos);
				}
				SendDlgItemMessage(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR, TBM_SETPOS, TRUE, pos);
				OpacityTooltip(TipWin, m_hWnd, IDC_ALPHA_BLEND_ACTIVE, pos, ts.UILanguageFileW);
				return TRUE;
			}
		case IDC_ALPHA_BLEND_INACTIVE | (EN_CHANGE << 16):
			{
				int pos;
				pos = GetDlgItemInt(IDC_ALPHA_BLEND_INACTIVE);
				if(pos < 0) {
					pos = 0;
					SetDlgItemNum(IDC_ALPHA_BLEND_INACTIVE, pos);
				}
				else if(pos > 255) {
					pos = 255;
					SetDlgItemNum(IDC_ALPHA_BLEND_INACTIVE, pos);
				}
				SendDlgItemMessage(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR, TBM_SETPOS, TRUE, pos);
				OpacityTooltip(TipWin, m_hWnd, IDC_ALPHA_BLEND_INACTIVE, pos, ts.UILanguageFileW);
				return TRUE;
			}
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

HBRUSH CVisualPropPageDlg::OnCtlColor(HDC hDC, HWND hWnd)
{
	if ( hWnd == GetDlgItem(IDC_SAMPLE_COLOR) ) {
		BYTE r = (BYTE)GetDlgItemInt(IDC_COLOR_RED);
		BYTE g = (BYTE)GetDlgItemInt(IDC_COLOR_GREEN);
		BYTE b = (BYTE)GetDlgItemInt(IDC_COLOR_BLUE);
		SetBkMode(hDC, TRANSPARENT);
		SetTextColor(hDC, RGB(r, g, b) );

		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
	return TTCPropertyPage::OnCtlColor(hDC, hWnd);
}

/**
 *	色の設定を変更したかチェックする
 *	@retval	TRUE	変更した
 *	@retval	FALSE	変更していない
 */
BOOL CVisualPropPageDlg::CheckColorChanged()
{
	for (int i = 0; i < 16; i++) {
		if (ts.ANSIColor[i] != ANSIColor[i]) {
			return TRUE;
		}
	}
	return FALSE;
}

/**
 *	テーマカラーが設定してあるかチェックする
 *	@retval	TRUE	設定されている
 *	@retval	FALSE	設定されていない
 */
BOOL CVisualPropPageDlg::CheckThemeColor()
{
	TColorTheme def;	// default color (=ts.ANSIColor[])
	ThemeGetColorDefault(&def);
	TColorTheme disp;	// 今表示されている色
	ThemeGetColor(vt_src, &disp);
	for (int i = 0; i < 16; i++) {
		if (disp.ansicolor.color[i] != def.ansicolor.color[i]) {
			return TRUE;
		}
	}
	return FALSE;
}

void CVisualPropPageDlg::OnOK()
{
	int sel;
	int i;

	// (1)
	i = GetDlgItemInt(IDC_ALPHA_BLEND_ACTIVE);
	ts.AlphaBlendActive =
		(i < 0) ? 0 :
		(BYTE)((i > 255) ? 255 : i);
	i = GetDlgItemInt(IDC_ALPHA_BLEND_INACTIVE);
	ts.AlphaBlendInactive =
		(i < 0) ? 0 :
		(BYTE)((i > 255) ? 255 : i);

	// (2)
	sel = GetCurSel(IDC_MOUSE_CURSOR);
	if (sel >= 0 && sel < MOUSE_CURSOR_MAX) {
		strncpy_s(ts.MouseCursorName, sizeof(ts.MouseCursorName), MouseCursor[sel].name, _TRUNCATE);
	}

	// (3)Font quality
	switch (GetCurSel(IDC_FONT_QUALITY)) {
		case 0:
			ts.FontQuality = DEFAULT_QUALITY;
			break;
		case 1:
			ts.FontQuality = NONANTIALIASED_QUALITY;
			break;
		case 2:
			ts.FontQuality = ANTIALIASED_QUALITY;
			break;
		case 3:
			ts.FontQuality = CLEARTYPE_QUALITY;
			break;
		default: // Invalid value.
			break;
	}

	// (5) Attr Bold Color
	if (((ts.ColorFlag & CF_BOLDCOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_BOLD)) {
		ts.ColorFlag ^= CF_BOLDCOLOR;
	}
	if (((ts.FontFlag & FF_BOLD) != 0) != GetCheck(IDC_ENABLE_ATTR_FONT_BOLD)) {
		ts.FontFlag ^= FF_BOLD;
	}

	// (6) Attr Blink Color
	if (((ts.ColorFlag & CF_BLINKCOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_BLINK)) {
		ts.ColorFlag ^= CF_BLINKCOLOR;
	}

	// (7) Attr Reverse Color
	if (((ts.ColorFlag & CF_REVERSECOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_REVERSE)) {
		ts.ColorFlag ^= CF_REVERSECOLOR;
	}

	// Underline Attr
	if (((ts.FontFlag & FF_UNDERLINE) != 0) != GetCheck(IDC_ENABLE_ATTR_FONT_UNDERLINE)) {
		ts.FontFlag ^= FF_UNDERLINE;
	}
	if (((ts.ColorFlag & CF_UNDERLINE) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_UNDERLINE)) {
		ts.ColorFlag ^= CF_UNDERLINE;
	}

	// URL Underline Attr
	if (((ts.FontFlag & FF_URLUNDERLINE) != 0) != GetCheck(IDC_ENABLE_ATTR_FONT_URL)) {
		ts.FontFlag ^= FF_URLUNDERLINE;
	}
	if (((ts.ColorFlag & CF_URLCOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_URL)) {
		ts.ColorFlag ^= CF_URLCOLOR;
	}

	// Color
	if (((ts.ColorFlag & CF_ANSICOLOR) != 0) != GetCheck(IDC_ENABLE_ANSI_COLOR)) {
		ts.ColorFlag ^= CF_ANSICOLOR;
	}

	ts.EtermLookfeel.BGNoCopyBits = GetCheck(IDC_CHECK_FLICKER_LESS_MOVE);

	// ウィンドウの角を丸くしない
	if (ts.WindowCornerDontround != GetCheck(IDC_CHECK_CORNERDONTROUND)) {
		ts.WindowCornerDontround = GetCheck(IDC_CHECK_CORNERDONTROUND);
		if (pDwmSetWindowAttribute != NULL) {
			DWM_WINDOW_CORNER_PREFERENCE preference = ts.WindowCornerDontround ? DWMWCP_DONOTROUND : DWMWCP_DEFAULT;
			pDwmSetWindowAttribute(HVTWin, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
		}
	}

	// ANSI Color
	if (CheckColorChanged()) {
		// 色の変更が行われた
		bool set_color = TRUE;

		// カラーテーマを使って色が変更されている?
		if (CheckThemeColor()) {
			static const TTMessageBoxInfoW info = {
				"Tera Term",
				"MSG_TT_NOTICE", L"Tera Term: Notice",
				NULL, L"Color settings have been changed.\nDo you want to display this?",
				MB_ICONQUESTION | MB_YESNO };
			int r = TTMessageBoxW(m_hWnd, &info, ts.UILanguageFileW);
			if (r == IDNO) {
				set_color = FALSE;
			}
		}

		// 色を設定(デフォルト色)に反映
		for (i = 0; i < 16; i++) {
			ts.ANSIColor[i] = ANSIColor[i];
		}

		// 設定された色を表示に反映
		if (set_color) {
			TColorTheme color;
			// デフォルト色を設定する
			ThemeGetColorDefault(&color);
			ThemeSetColor(vt_src, &color);
		}
	}
}

void CVisualPropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalVisual, 0);
}

/////////////////////////////
// cygterm.cfg 読み書き

#define CYGTERM_FILE "cygterm.cfg"  // CygTerm configuration file
#define CYGTERM_FILE_MAXLINE 100

typedef struct cygterm {
	char term[128];
	char term_type[80];
	char port_start[80];
	char port_range[80];
	char shell[80];
	char env1[128];
	char env2[128];
	BOOL login_shell;
	BOOL home_chdir;
	BOOL agent_proxy;
} cygterm_t;

void ReadCygtermConfFile(const char *homedir, cygterm_t *psettings)
{
	const char *cfgfile = CYGTERM_FILE; // CygTerm configuration file
	char cfg[MAX_PATH];
	FILE *fp;
	char buf[256], *head, *body;
	cygterm_t settings;

	// try to read CygTerm config file
	memset(&settings, 0, sizeof(settings));
	_snprintf_s(settings.term, sizeof(settings.term), _TRUNCATE, "ttermpro.exe %%s %%d /E /KR=UTF8 /KT=UTF8 /VTICON=CygTerm /nossh");
	_snprintf_s(settings.term_type, sizeof(settings.term_type), _TRUNCATE, "vt100");
	_snprintf_s(settings.port_start, sizeof(settings.port_start), _TRUNCATE, "20000");
	_snprintf_s(settings.port_range, sizeof(settings.port_range), _TRUNCATE, "40");
	_snprintf_s(settings.shell, sizeof(settings.shell), _TRUNCATE, "auto");
	_snprintf_s(settings.env1, sizeof(settings.env1), _TRUNCATE, "");
	_snprintf_s(settings.env2, sizeof(settings.env2), _TRUNCATE, "");
	settings.login_shell = FALSE;
	settings.home_chdir = FALSE;
	settings.agent_proxy = FALSE;

	strncpy_s(cfg, sizeof(cfg), homedir, _TRUNCATE);
	AppendSlash(cfg, sizeof(cfg));
	strncat_s(cfg, sizeof(cfg), cfgfile, _TRUNCATE);

	fp = fopen(cfg, "r");
	if (fp != NULL) {
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			size_t len = strlen(buf);

			if (buf[len - 1] == '\n')
				buf[len - 1] = '\0';

			split_buffer(buf, '=', &head, &body);
			if (head == NULL || body == NULL)
				continue;

			if (_stricmp(head, "TERM") == 0) {
				_snprintf_s(settings.term, sizeof(settings.term), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "TERM_TYPE") == 0) {
				_snprintf_s(settings.term_type, sizeof(settings.term_type), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "PORT_START") == 0) {
				_snprintf_s(settings.port_start, sizeof(settings.port_start), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "PORT_RANGE") == 0) {
				_snprintf_s(settings.port_range, sizeof(settings.port_range), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "SHELL") == 0) {
				_snprintf_s(settings.shell, sizeof(settings.shell), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "ENV_1") == 0) {
				_snprintf_s(settings.env1, sizeof(settings.env1), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "ENV_2") == 0) {
				_snprintf_s(settings.env2, sizeof(settings.env2), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "LOGIN_SHELL") == 0) {
				if (strchr("YyTt", *body)) {
					settings.login_shell = TRUE;
				}

			}
			else if (_stricmp(head, "HOME_CHDIR") == 0) {
				if (strchr("YyTt", *body)) {
					settings.home_chdir = TRUE;
				}

			}
			else if (_stricmp(head, "SSH_AGENT_PROXY") == 0) {
				if (strchr("YyTt", *body)) {
					settings.agent_proxy = TRUE;
				}

			}
			else {
				// TODO: error check

			}
		}
		fclose(fp);
	}

	memcpy(psettings, &settings, sizeof(cygterm_t));
}

BOOL WriteCygtermConfFile(const char *homedir, cygterm_t *psettings)
{
	const char *cfgfile = CYGTERM_FILE; // CygTerm configuration file
	const char *tmpfile = "cygterm.tmp";
	char cfg[MAX_PATH];
	char tmp[MAX_PATH];
	FILE *fp;
	FILE *tmp_fp;
	char buf[256], *head, *body;
	cygterm_t settings;
	char *line[CYGTERM_FILE_MAXLINE];
	int i, linenum;

	memcpy(&settings, psettings, sizeof(cygterm_t));

	strncpy_s(cfg, sizeof(cfg), homedir, _TRUNCATE);
	AppendSlash(cfg, sizeof(cfg));
	strncat_s(cfg, sizeof(cfg), cfgfile, _TRUNCATE);

	strncpy_s(tmp, sizeof(tmp), homedir, _TRUNCATE);
	AppendSlash(tmp, sizeof(tmp));
	strncat_s(tmp, sizeof(tmp), tmpfile, _TRUNCATE);

	// cygterm.cfg が存在すれば、いったんメモリにすべて読み込む。
	memset(line, 0, sizeof(line));
	linenum = 0;
	fp = fopen(cfg, "r");
	if (fp) {
		i = 0;
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			size_t len = strlen(buf);
			if (buf[len - 1] == '\n')
				buf[len - 1] = '\0';
			if (i < CYGTERM_FILE_MAXLINE)
				line[i++] = _strdup(buf);
			else
				break;
		}
		linenum = i;
		fclose(fp);
	}

	tmp_fp = fopen(cfg, "w");
	if (tmp_fp == NULL) {
		return FALSE;
	}
	else {
		if (linenum > 0) {
			for (i = 0; i < linenum; i++) {
				split_buffer(line[i], '=', &head, &body);
				if (head == NULL || body == NULL) {
					fprintf(tmp_fp, "%s\n", line[i]);
				}
				else if (_stricmp(head, "TERM") == 0) {
					fprintf(tmp_fp, "TERM = %s\n", settings.term);
					settings.term[0] = '\0';
				}
				else if (_stricmp(head, "TERM_TYPE") == 0) {
					fprintf(tmp_fp, "TERM_TYPE = %s\n", settings.term_type);
					settings.term_type[0] = '\0';
				}
				else if (_stricmp(head, "PORT_START") == 0) {
					fprintf(tmp_fp, "PORT_START = %s\n", settings.port_start);
					settings.port_start[0] = '\0';
				}
				else if (_stricmp(head, "PORT_RANGE") == 0) {
					fprintf(tmp_fp, "PORT_RANGE = %s\n", settings.port_range);
					settings.port_range[0] = '\0';
				}
				else if (_stricmp(head, "SHELL") == 0) {
					fprintf(tmp_fp, "SHELL = %s\n", settings.shell);
					settings.shell[0] = '\0';
				}
				else if (_stricmp(head, "ENV_1") == 0) {
					fprintf(tmp_fp, "ENV_1 = %s\n", settings.env1);
					settings.env1[0] = '\0';
				}
				else if (_stricmp(head, "ENV_2") == 0) {
					fprintf(tmp_fp, "ENV_2 = %s\n", settings.env2);
					settings.env2[0] = '\0';
				}
				else if (_stricmp(head, "LOGIN_SHELL") == 0) {
					fprintf(tmp_fp, "LOGIN_SHELL = %s\n", (settings.login_shell == TRUE) ? "yes" : "no");
					settings.login_shell = FALSE;
				}
				else if (_stricmp(head, "HOME_CHDIR") == 0) {
					fprintf(tmp_fp, "HOME_CHDIR = %s\n", (settings.home_chdir == TRUE) ? "yes" : "no");
					settings.home_chdir = FALSE;
				}
				else if (_stricmp(head, "SSH_AGENT_PROXY") == 0) {
					fprintf(tmp_fp, "SSH_AGENT_PROXY = %s\n", (settings.agent_proxy == TRUE) ? "yes" : "no");
					settings.agent_proxy = FALSE;
				}
				else {
					fprintf(tmp_fp, "%s = %s\n", head, body);
				}
			}
		}
		else {
			fputs("# CygTerm setting\n", tmp_fp);
			fputs("\n", tmp_fp);
		}
		if (settings.term[0] != '\0') {
			fprintf(tmp_fp, "TERM = %s\n", settings.term);
		}
		if (settings.term_type[0] != '\0') {
			fprintf(tmp_fp, "TERM_TYPE = %s\n", settings.term_type);
		}
		if (settings.port_start[0] != '\0') {
			fprintf(tmp_fp, "PORT_START = %s\n", settings.port_start);
		}
		if (settings.port_range[0] != '\0') {
			fprintf(tmp_fp, "PORT_RANGE = %s\n", settings.port_range);
		}
		if (settings.shell[0] != '\0') {
			fprintf(tmp_fp, "SHELL = %s\n", settings.shell);
		}
		if (settings.env1[0] != '\0') {
			fprintf(tmp_fp, "ENV_1 = %s\n", settings.env1);
		}
		if (settings.env2[0] != '\0') {
			fprintf(tmp_fp, "ENV_2 = %s\n", settings.env2);
		}
		if (settings.login_shell) {
			fprintf(tmp_fp, "LOGIN_SHELL = yes\n");
		}
		if (settings.home_chdir) {
			fprintf(tmp_fp, "HOME_CHDIR = yes\n");
		}
		if (settings.agent_proxy) {
			fprintf(tmp_fp, "SSH_AGENT_PROXY = yes\n");
		}
		fclose(tmp_fp);

		// ダイレクトにファイルに書き込むようにしたので、下記処理は不要。
#if 0
		if (remove(cfg) != 0 && errno != ENOENT) {
			get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts->UILanguageFile);
			get_lang_msg("MSG_CYGTERM_CONF_REMOVEFILE_ERROR", ts->UIMsg, sizeof(ts->UIMsg),
				"Can't remove old CygTerm configuration file (%d).", ts->UILanguageFile);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts->UIMsg, GetLastError());
			MessageBox(NULL, buf, uimsg, MB_ICONEXCLAMATION);
		}
		else if (rename(tmp, cfg) != 0) {
			get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts->UILanguageFile);
			get_lang_msg("MSG_CYGTERM_CONF_RENAMEFILE_ERROR", ts->UIMsg, sizeof(ts->UIMsg),
				"Can't rename CygTerm configuration file (%d).", ts->UILanguageFile);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts->UIMsg, GetLastError());
			MessageBox(NULL, buf, uimsg, MB_ICONEXCLAMATION);
		}
		else {
			// cygterm.cfg ファイルへの保存が成功したら、メッセージダイアログを表示する。
			// 改めて、Save setupを実行する必要はないことを注意喚起する。
			// (2012.5.1 yutaka)
			// Save setup 実行時に、CygTermの設定を保存するようにしたことにより、
			// ダイアログ表示が不要となるため、削除する。
			// (2015.11.12 yutaka)
			get_lang_msg("MSG_TT_NOTICE", uimsg, sizeof(uimsg), "MSG_TT_NOTICE", ts->UILanguageFile);
			get_lang_msg("MSG_CYGTERM_CONF_SAVED_NOTICE", ts->UIMsg, sizeof(ts->UIMsg),
				"%s has been saved. Do not do save setup.", ts->UILanguageFile);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts->UIMsg, CYGTERM_FILE);
			MessageBox(NULL, buf, uimsg, MB_OK | MB_ICONINFORMATION);
		}
#endif
	}

	// 忘れずにメモリフリーしておく。
	for (i = 0; i < linenum; i++) {
		free(line[i]);
	}

	return TRUE;
}

/////////////////////////////

// CCygwinPropPageDlg ダイアログ
void ReadCygtermConfFile(const char *homedir, cygterm_t *psettings);
BOOL WriteCygtermConfFile(const char *homedir, cygterm_t *psettings);
BOOL CmpCygtermConfFile(const cygterm_t *a, const cygterm_t *b);

class CCygwinPropPageDlg : public TTCPropertyPage
{
public:
	CCygwinPropPageDlg(HINSTANCE inst);
	virtual ~CCygwinPropPageDlg();
private:
	void OnInitDialog();
	void OnOK();
	enum { IDD = IDD_TABSHEET_CYGWIN };
	cygterm_t settings;
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnHelp();
};

CCygwinPropPageDlg::CCygwinPropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CCygwinPropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_CYGWIN",
				 L"Cygwin", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
}

CCygwinPropPageDlg::~CCygwinPropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CCygwinPropPageDlg メッセージ ハンドラ

void CCygwinPropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_CYGWIN_PATH_LABEL, "DLG_TAB_CYGWIN_PATH" }
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	ReadCygtermConfFile(ts.HomeDir, &settings);

	SetDlgItemTextA(IDC_TERM_EDIT, settings.term);
	SetDlgItemTextA(IDC_TERM_TYPE, settings.term_type);
	SetDlgItemTextA(IDC_PORT_START, settings.port_start);
	SetDlgItemTextA(IDC_PORT_RANGE, settings.port_range);
	SetDlgItemTextA(IDC_SHELL, settings.shell);
	SetDlgItemTextA(IDC_ENV1, settings.env1);
	SetDlgItemTextA(IDC_ENV2, settings.env2);

	SetCheck(IDC_LOGIN_SHELL, settings.login_shell);
	SetCheck(IDC_HOME_CHDIR, settings.home_chdir);
	SetCheck(IDC_AGENT_PROXY, settings.agent_proxy);

	// Cygwin install path
	SetDlgItemTextA(IDC_CYGWIN_PATH, ts.CygwinDirectory);

	// ダイアログにフォーカスを当てる
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_CYGWIN_PATH));
}

BOOL CCygwinPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_SELECT_FILE | (BN_CLICKED << 16):
			// Cygwin install ディレクトリの選択ダイアログ
			wchar_t *title = TTGetLangStrW("Tera Term", "DIRDLG_CYGTERM_DIR_TITLE", L"Select Cygwin directory", ts.UILanguageFileW);
			wchar_t *buf;
			hGetDlgItemTextW(m_hWnd, IDC_CYGWIN_PATH, &buf);
			wchar_t *path;
			if (doSelectFolderW(GetSafeHwnd(), buf, title, &path)) {
				SetDlgItemTextW(IDC_CYGWIN_PATH, path);
				free(path);
			}
			free(buf);
			free(title);
			return TRUE;
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

/**
 * @brief	cygterm_tを比較する
 * @param	a	cygterm_t
 * @param	b	cygterm_t
 * @retval	TRUE	同一
 * @retval	FALSE	異なる
 */
BOOL CmpCygtermConfFile(const cygterm_t *a, const cygterm_t *b)
{
	if ((strcmp(a->term, b->term) != 0) ||
		(strcmp(a->term_type, b->term_type) != 0) ||
		(strcmp(a->port_start, b->port_start) != 0) ||
		(strcmp(a->port_range, b->port_range) != 0) ||
		(strcmp(a->shell, b->shell) != 0) ||
		(strcmp(a->env1, b->env1) != 0) ||
		(strcmp(a->env2, b->env2) != 0) ||
		(a->login_shell != b->login_shell) ||
		(a->home_chdir != b->home_chdir) ||
		(a->agent_proxy != b->agent_proxy)) {
		return FALSE;
	}
	return TRUE;
}

void CCygwinPropPageDlg::OnOK()
{
	cygterm_t settings_prop;

	// プロパティーシートから値を取り込む
	GetDlgItemTextA(IDC_TERM_EDIT, settings_prop.term, sizeof(settings_prop.term));
	GetDlgItemTextA(IDC_TERM_TYPE, settings_prop.term_type, sizeof(settings_prop.term_type));
	GetDlgItemTextA(IDC_PORT_START, settings_prop.port_start, sizeof(settings_prop.port_start));
	GetDlgItemTextA(IDC_PORT_RANGE, settings_prop.port_range, sizeof(settings_prop.port_range));
	GetDlgItemTextA(IDC_SHELL, settings_prop.shell, sizeof(settings_prop.shell));
	GetDlgItemTextA(IDC_ENV1, settings_prop.env1, sizeof(settings_prop.env1));
	GetDlgItemTextA(IDC_ENV2, settings_prop.env2, sizeof(settings_prop.env2));

	settings_prop.login_shell = GetCheck(IDC_LOGIN_SHELL);
	settings_prop.home_chdir = GetCheck(IDC_HOME_CHDIR);
	settings_prop.agent_proxy = GetCheck(IDC_AGENT_PROXY);

	// 変更されている場合 cygterm.cfg へ書き込む
	if (CmpCygtermConfFile(&settings_prop, &settings) == FALSE) {
		if (WriteCygtermConfFile(ts.HomeDir, &settings_prop) == FALSE) {
			static const TTMessageBoxInfoW info = {
				"Tera Term",
				"MSG_ERROR", L"ERROR",
				"MSG_CYGTERM_CONF_WRITEFILE_ERROR", L"Can't write CygTerm configuration file (%d).",
				MB_ICONEXCLAMATION };
			DWORD e = GetLastError();
			TTMessageBoxW(m_hWnd, &info, ts.UILanguageFileW, e);
		}
	}

	// Cygwin install path
	GetDlgItemTextA(IDC_CYGWIN_PATH, ts.CygwinDirectory, sizeof(ts.CygwinDirectory));
}

void CCygwinPropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalCygwin, 0);
}

//////////////////////////////////////////////////////////////////////////////

// タブ
// この順にタブが表示される
enum {
	PP_GENERAL,
	PP_UI,
	PP_SEQUENCE,
	PP_COPY_PASTE,
	PP_VISUAL,
	PP_LOG,
	PP_CYGWIN,
	PP_ENCODING,
	PP_FONT,
	PP_THEME,
	PP_KEYBOARD,
	PP_MOUSE,
	PP_TCPIP,
	PP_TERMINAL,
	PP_SERIAL,
	PP_WINDOW,
	PP_TEK_WINDOW,
	PP_TEK_FONT,
	PP_PLUGIN,
	PP_DEBUG,
	PP_MAX,
};

static int GetStartPageNo(CAddSettingPropSheetDlg::Page page)
{
	int start_page = PP_GENERAL;
	switch (page) {
	case DefaultPage:
		start_page = PP_GENERAL;
		break;
	case CodingPage:
		start_page = PP_ENCODING;
		break;
	case FontPage:
		start_page = PP_FONT;
		break;
	case KeyboardPage:
		start_page = PP_KEYBOARD;
		break;
	case TcpIpPage:
		start_page = PP_TCPIP;
		break;
	case TermPage:
		start_page = PP_TERMINAL;
		break;
	case WinPage:
		start_page = PP_WINDOW;
		break;
	case SerialPortPage:
		start_page = PP_SERIAL;
		break;
	case TekFontPage:
		start_page = PP_TEK_FONT;
		break;
	default:
		start_page = PP_GENERAL;
		break;
	}
	return start_page;
}

// CAddSettingPropSheetDlg
CAddSettingPropSheetDlg::CAddSettingPropSheetDlg(
	HINSTANCE hInstance, HWND hParentWnd,
	CAddSettingPropSheetDlg::Page StartPage):
	TTCPropSheetDlg(hInstance, hParentWnd, ts.UILanguageFileW)
{
	HPROPSHEETPAGE pages[PP_MAX] = {0};

	AddsettingWin parent_win = AddsettingCheckWin(hParentWnd);

	// CPP,tmfcのTTCPropertyPage派生クラスから生成
	int i = 0;
	if (parent_win == ADDSETTING_WIN_VT) {
		m_Page[i] = new CGeneralPropPageDlg(hInstance, hParentWnd, &cv);
		pages[PP_GENERAL] = m_Page[i++]->CreatePropertySheetPage();
		m_Page[i] = new CSequencePropPageDlg(hInstance);
		pages[PP_SEQUENCE] = m_Page[i++]->CreatePropertySheetPage();
		m_Page[i] = new CCopypastePropPageDlg(hInstance);
		pages[PP_COPY_PASTE] = m_Page[i++]->CreatePropertySheetPage();
		m_Page[i] = new CVisualPropPageDlg(hInstance);
		pages[PP_VISUAL] = m_Page[i++]->CreatePropertySheetPage();
		m_Page[i] = new CLogPropPageDlg(hInstance);
		pages[PP_LOG] = m_Page[i++]->CreatePropertySheetPage();
		m_Page[i] = new CCygwinPropPageDlg(hInstance);
		pages[PP_CYGWIN] = m_Page[i++]->CreatePropertySheetPage();
	}
	if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
		(GetKeyState(VK_SHIFT) & 0x8000) != 0 ) {
		m_Page[i] = new CDebugPropPage(hInstance);
		pages[PP_DEBUG] = m_Page[i++]->CreatePropertySheetPage();
	}
	m_PageCountCPP = i;

	// TTCPropertyPage を使用しない PropertyPage
	if (parent_win == ADDSETTING_WIN_VT) {
		pages[PP_ENCODING] = CodingPageCreate(hInstance, &ts);
		pages[PP_FONT] = FontPageCreate(hInstance, &ts);
		pages[PP_THEME] = ThemePageCreate(hInstance, &ts);
		pages[PP_KEYBOARD] = KeyboardPageCreate(hInstance, &ts);
		pages[PP_MOUSE] = MousePageCreate(hInstance, &ts);
		pages[PP_TCPIP] = TcpIPPageCreate(hInstance, &ts);
		pages[PP_TERMINAL] = CreateTerminalPP(hInstance, hParentWnd, &ts);
		pages[PP_SERIAL] = SerialPageCreate(hInstance, &ts);
		pages[PP_UI] = UIPageCreate(hInstance, &ts);
		pages[PP_WINDOW] = CreateWinVTPP(hInstance, hParentWnd, &ts);
	}
	if (parent_win == ADDSETTING_WIN_TEK) {	// TEKから呼び出されたときだけ表示する
//	{										// いつも表示する
		pages[PP_TEK_WINDOW] = CreateWinTEKPP(hInstance, hParentWnd, &ts);
		pages[PP_TEK_FONT] = TEKFontPageCreate(hInstance, hParentWnd, &ts);
	}
	pages[PP_PLUGIN] = PluginPageCreate(hInstance, &ts);

	const int start_page_no = GetStartPageNo(StartPage);
	int available_page_count = 0;
	int start_page_index = 0;
	for (int i = 0; i < PP_MAX; i++) {
		HPROPSHEETPAGE page = pages[i];
		if (page != 0) {
			AddPage(page);
			if (i == start_page_no) {
				start_page_index = available_page_count;
			}
			available_page_count++;
		}
	}
	TTCPropSheetDlg::SetStartPage(start_page_index);

	wchar_t *title = TTGetLangStrW("Tera Term", "DLG_TABSHEET_TITLE", L"Tera Term: Additional settings", ts.UILanguageFileW);
	SetCaption(title);
	free(title);
}

CAddSettingPropSheetDlg::~CAddSettingPropSheetDlg()
{
	for (int i = 0; i < m_PageCountCPP; i++) {
		delete m_Page[i];
	}
}

/**
 *	hWnd がどのウィンドウか調べる
 *		hWnd は VTWINかTEKWINのはず
 */
AddsettingWin AddsettingCheckWin(HWND hWnd)
{
	if (hWnd == NULL) {
		// ActiveWin が使える?
		assert(false);
		if (ActiveWin == IdTEK)
			hWnd = HTEKWin;
		else
			hWnd = HVTWin;
	}
	if (hWnd == HVTWin) {
		return ADDSETTING_WIN_VT;
	}
	else if (hWnd == HTEKWin) {
		return ADDSETTING_WIN_TEK;
	}
	assert(false);
	return ADDSETTING_WIN_VT;
}
