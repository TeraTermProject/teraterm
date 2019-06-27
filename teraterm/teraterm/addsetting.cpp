/*
 * Copyright (C) 2008-2019 TeraTerm Project
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
#include "teraterm_conf.h"

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <commctrl.h>
#include <time.h>
#include <tchar.h>

#include "addsetting.h"
#include "teraterm.h"
#include "tttypes.h"
#include "ttwinman.h"	// for ts
#include "ttcommon.h"
#include "ttftypes.h"
#include "dlglib.h"
#include "compat_win.h"

#undef GetDlgItemText
#define GetDlgItemText GetDlgItemTextA
#undef SetDlgItemText
#define SetDlgItemText SetDlgItemTextA

const mouse_cursor_t MouseCursor[] = {
	{"ARROW", IDC_ARROW},
	{"IBEAM", IDC_IBEAM},
	{"CROSS", IDC_CROSS},
	{"HAND", IDC_HAND},
	{NULL, NULL},
};
#define MOUSE_CURSOR_MAX (sizeof(MouseCursor)/sizeof(MouseCursor[0]) - 1)

void CVisualPropPageDlg::SetupRGBbox(int index)
{
	BYTE c;

	c = GetRValue(ts.ANSIColor[index]);
	SetDlgItemNum(IDC_COLOR_RED, c);

	c = GetGValue(ts.ANSIColor[index]);
	SetDlgItemNum(IDC_COLOR_GREEN, c);

	c = GetBValue(ts.ANSIColor[index]);
	SetDlgItemNum(IDC_COLOR_BLUE, c);
}

// CGeneralPropPageDlg ダイアログ

CGeneralPropPageDlg::CGeneralPropPageDlg(HINSTANCE inst, TTCPropertySheet *sheet)
	: TTCPropertyPage(inst, CGeneralPropPageDlg::IDD, sheet)
{
	TCHAR UIMsg[MAX_UIMSG];
	get_lang_msgT("DLG_TABSHEET_TITLE_GENERAL", UIMsg, _countof(UIMsg),
	             _T("General"), ts.UILanguageFile);
	m_psp.pszTitle = _tcsdup(UIMsg);
	m_psp.dwFlags |= PSP_USETITLE;
}

CGeneralPropPageDlg::~CGeneralPropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CGeneralPropPageDlg メッセージ ハンドラ

void CGeneralPropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_CLICKABLE_URL, "DLG_TAB_GENERAL_CLICKURL" },
		{ IDC_DISABLE_SENDBREAK, "DLG_TAB_GENERAL_DISABLESENDBREAK" },
		{ IDC_ACCEPT_BROADCAST, "DLG_TAB_GENERAL_ACCEPTBROADCAST" },
		{ IDC_MOUSEWHEEL_SCROLL_LINE, "DLG_TAB_GENERAL_MOUSEWHEEL_SCROLL_LINE" },
		{ IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE, "DLG_TAB_GENERAL_AUTOSCROLL_ONLY_IN_BOTTOM_LINE" },
		{ IDC_CLEAR_ON_RESIZE, "DLG_TAB_GENERAL_CLEAR_ON_RESIZE" },
		{ IDC_CURSOR_CHANGE_IME, "DLG_TAB_GENERAL_CURSOR_CHANGE_IME" },
		{ IDC_LIST_HIDDEN_FONTS, "DLG_TAB_GENERAL_LIST_HIDDEN_FONTS" },
		{ IDC_TITLEFMT_GROUP, "DLG_TAB_GENERAL_TITLEFMT_GROUP" },
		{ IDC_TITLEFMT_DISPHOSTNAME, "DLG_TAB_GENERAL_TITLEFMT_DISPHOSTNAME" },
		{ IDC_TITLEFMT_DISPSESSION, "DLG_TAB_GENERAL_TITLEFMT_DISPSESSION" },
		{ IDC_TITLEFMT_DISPVTTEK, "DLG_TAB_GENERAL_TITLEFMT_DISPVTTEK" },
		{ IDC_TITLEFMT_SWAPHOSTTITLE, "DLG_TAB_GENERAL_TITLEFMT_SWAPHOSTTITLE" },
		{ IDC_TITLEFMT_DISPTCPPORT, "DLG_TAB_GENERAL_TITLEFMT_DISPTCPPORT" },
		{ IDC_TITLEFMT_DISPSERIALSPEED, "DLG_TAB_GENERAL_TITLEFMT_DISPSERIALSPEED" }
	};
	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

	// (1)DisableAcceleratorSendBreak
	SetCheck(IDC_DISABLE_SENDBREAK, ts.DisableAcceleratorSendBreak);

	// (2)EnableClickableUrl
	SetCheck(IDC_CLICKABLE_URL, ts.EnableClickableUrl);

	// (3)AcceptBroadcast 337: 2007/03/20
	SetCheck(IDC_ACCEPT_BROADCAST, ts.AcceptBroadcast);

	// (4)IDC_MOUSEWHEEL_SCROLL_LINE
	SetDlgItemNum(IDC_SCROLL_LINE, ts.MouseWheelScrollLine);

	// (5)IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE
	SetCheck(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE, ts.AutoScrollOnlyInBottomLine);

	// (6)IDC_CLEAR_ON_RESIZE
	SetCheck(IDC_CLEAR_ON_RESIZE, (ts.TermFlag & TF_CLEARONRESIZE) != 0);

	// (7)IDC_CURSOR_CHANGE_IME
	SetCheck(IDC_CURSOR_CHANGE_IME, (ts.WindowFlag & WF_IMECURSORCHANGE) != 0);

	// (8)IDC_LIST_HIDDEN_FONTS
	SetCheck(IDC_LIST_HIDDEN_FONTS, ts.ListHiddenFonts);

	// (9) Title Format
	SetCheck(IDC_TITLEFMT_DISPHOSTNAME, (ts.TitleFormat & 1) != 0);
	SetCheck(IDC_TITLEFMT_DISPSESSION, (ts.TitleFormat & (1<<1)) != 0);
	SetCheck(IDC_TITLEFMT_DISPVTTEK, (ts.TitleFormat & (1<<2)) != 0);
	SetCheck(IDC_TITLEFMT_SWAPHOSTTITLE, (ts.TitleFormat & (1<<3)) != 0);
	SetCheck(IDC_TITLEFMT_DISPTCPPORT, (ts.TitleFormat & (1<<4)) != 0);
	SetCheck(IDC_TITLEFMT_DISPSERIALSPEED, (ts.TitleFormat & (1<<5)) != 0);

	// ダイアログにフォーカスを当てる (2004.12.7 yutaka)
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_CLICKABLE_URL));
}

void CGeneralPropPageDlg::OnOK()
{
	TCHAR buf[64];
	int val;

	// (1)
	ts.DisableAcceleratorSendBreak = GetCheck(IDC_DISABLE_SENDBREAK);

	// (2)
	ts.EnableClickableUrl = GetCheck(IDC_CLICKABLE_URL);

	// (3) 337: 2007/03/20
	ts.AcceptBroadcast = GetCheck(IDC_ACCEPT_BROADCAST);

	// (4)IDC_MOUSEWHEEL_SCROLL_LINE
	GetDlgItemTextT(IDC_SCROLL_LINE, buf, _countof(buf));
	val = _tstoi(buf);
	if (val > 0)
		ts.MouseWheelScrollLine = val;

	// (5)IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE
	ts.AutoScrollOnlyInBottomLine = GetCheck(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE);

	// (6)IDC_CLEAR_ON_RESIZE
	if (((ts.TermFlag & TF_CLEARONRESIZE) != 0) != GetCheck(IDC_CLEAR_ON_RESIZE)) {
		ts.TermFlag ^= TF_CLEARONRESIZE;
	}

	// (7)IDC_CURSOR_CHANGE_IME
	if (((ts.WindowFlag & WF_IMECURSORCHANGE) != 0) != GetCheck(IDC_CURSOR_CHANGE_IME)) {
		ts.WindowFlag ^= WF_IMECURSORCHANGE;
	}

	// (8)IDC_LIST_HIDDEN_FONTS
	ts.ListHiddenFonts = GetCheck(IDC_LIST_HIDDEN_FONTS);

	// (9) Title Format
	ts.TitleFormat = GetCheck(IDC_TITLEFMT_DISPHOSTNAME) == BST_CHECKED;
	ts.TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPSESSION) == BST_CHECKED) << 1;
	ts.TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPVTTEK) == BST_CHECKED) << 2;
	ts.TitleFormat |= (GetCheck(IDC_TITLEFMT_SWAPHOSTTITLE) == BST_CHECKED) << 3;
	ts.TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPTCPPORT) == BST_CHECKED) << 4;
	ts.TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPSERIALSPEED) == BST_CHECKED) << 5;
}

// CSequencePropPageDlg ダイアログ

CSequencePropPageDlg::CSequencePropPageDlg(HINSTANCE inst, TTCPropertySheet *sheet)
	: TTCPropertyPage(inst, CSequencePropPageDlg::IDD, sheet)
{
	TCHAR UIMsg[MAX_UIMSG];
	get_lang_msgT("DLG_TABSHEET_TITLE_SEQUENCE", UIMsg, _countof(UIMsg),
	             _T("Control Sequence"), ts.UILanguageFile);
	m_psp.pszTitle = _tcsdup(UIMsg);
	m_psp.dwFlags |= PSP_USETITLE;
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
	};
	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

	TCHAR uimsg[MAX_UIMSG];
	get_lang_msgT("DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_OFF", uimsg, _countof(uimsg), _T("off"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_OVERWRITE", uimsg, _countof(uimsg), _T("overwrite"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_AHEAD", uimsg, _countof(uimsg), _T("ahead"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_LAST", uimsg, _countof(uimsg), _T("last"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)uimsg);

	get_lang_msgT("DLG_TAB_SEQUENCE_TITLE_REPORT_IGNORE", uimsg, _countof(uimsg), _T("ignore"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_TITLE_REPORT, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_SEQUENCE_TITLE_REPORT_ACCEPT", uimsg, _countof(uimsg), _T("accept"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_TITLE_REPORT, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_SEQUENCE_TITLE_REPORT_EMPTY", uimsg, _countof(uimsg), _T("empty"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_TITLE_REPORT, CB_ADDSTRING, 0, (LPARAM)uimsg);

	get_lang_msgT("DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_OFF", uimsg, _countof(uimsg), _T("off"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_CLIPBOARD_ACCESS, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_WRITE", uimsg, _countof(uimsg), _T("write only"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_CLIPBOARD_ACCESS, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_READ", uimsg, _countof(uimsg), _T("read only"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_CLIPBOARD_ACCESS, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_ON", uimsg, _countof(uimsg), _T("read/write"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_CLIPBOARD_ACCESS, CB_ADDSTRING, 0, (LPARAM)uimsg);

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
}


// CCopypastePropPageDlg ダイアログ

CCopypastePropPageDlg::CCopypastePropPageDlg(HINSTANCE inst, TTCPropertySheet *sheet)
	: TTCPropertyPage(inst, CCopypastePropPageDlg::IDD, sheet)
{
	TCHAR UIMsg[MAX_UIMSG];
	get_lang_msgT("DLG_TABSHEET_TITLE_COPYPASTE", UIMsg, _countof(UIMsg),
				  _T("Copy and Paste"), ts.UILanguageFile);
	m_psp.pszTitle = _tcsdup(UIMsg);
	m_psp.dwFlags |= PSP_USETITLE;
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
		{ IDC_NORMALIZE_LINEBREAK, "DLG_TAB_COPYPASTE_NORMALIZE_LINEBREAK" },
		{ IDC_CONFIRM_CHANGE_PASTE, "DLG_TAB_COPYPASTE_CONFIRM_CHANGE_PASTE" },
		{ IDC_CONFIRM_STRING_FILE_LABEL, "DLG_TAB_COPYPASTE_STRINGFILE" },
		{ IDC_DELIMITER, "DLG_TAB_COPYPASTE_DELIMITER" },
		{ IDC_PASTEDELAY_LABEL, "DLG_TAB_COPYPASTE_PASTEDELAY" },
		{ IDC_PASTEDELAY_LABEL2, "DLG_TAB_COPYPASTE_PASTEDELAY2" }
	};
	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

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

	// (7)NormalizeLineBreak
	SetCheck(IDC_NORMALIZE_LINEBREAK, (ts.PasteFlag & CPF_NORMALIZE_LINEBREAK)?BST_CHECKED:BST_UNCHECKED);

	// (8)ConfirmChangePaste
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

	// (9)delimiter characters
	SetDlgItemTextA(IDC_DELIM_LIST, ts.DelimList);

	// (10)PasteDelayPerLine
	char buf[64];
	_snprintf_s(buf, sizeof(buf), "%d", ts.PasteDelayPerLine);
	SetDlgItemNum(IDC_PASTEDELAY_EDIT, ts.PasteDelayPerLine);

	// ダイアログにフォーカスを当てる
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_LINECOPY));
}

BOOL CCopypastePropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	char uimsg[MAX_UIMSG];

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
				OPENFILENAMEA ofn;

				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = get_OPENFILENAME_SIZEA();
				ofn.hwndOwner = GetSafeHwnd();
				get_lang_msg("FILEDLG_SELECT_CONFIRM_STRING_APP_FILTER", ts.UIMsg, sizeof(ts.UIMsg),
				             "txt(*.txt)\\0*.txt\\0all(*.*)\\0*.*\\0\\0", ts.UILanguageFile);
				ofn.lpstrFilter = ts.UIMsg;
				ofn.lpstrFile = ts.ConfirmChangePasteStringFile;
				ofn.nMaxFile = sizeof(ts.ConfirmChangePasteStringFile);
				get_lang_msg("FILEDLG_SELECT_CONFIRM_STRING_APP_TITLE", uimsg, sizeof(uimsg),
				             "Choose a file including strings for ConfirmChangePaste", ts.UILanguageFile);
				ofn.lpstrTitle = uimsg;
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				if (GetOpenFileNameA(&ofn) != 0) {
					SetDlgItemTextA(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile);
				}
			}
			return TRUE;
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

void CCopypastePropPageDlg::OnOK()
{
	char buf[64];
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

	// (7)
	if (GetCheck(IDC_NORMALIZE_LINEBREAK)) {
		ts.PasteFlag |= CPF_NORMALIZE_LINEBREAK;
	}
	else {
		ts.PasteFlag &= ~CPF_NORMALIZE_LINEBREAK;
	}

	// (8)IDC_CONFIRM_CHANGE_PASTE
	if (GetCheck(IDC_CONFIRM_CHANGE_PASTE)) {
		ts.PasteFlag |= CPF_CONFIRM_CHANGEPASTE;
	}
	else {
		ts.PasteFlag &= ~CPF_CONFIRM_CHANGEPASTE;
	}
	GetDlgItemTextA(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile, sizeof(ts.ConfirmChangePasteStringFile));

	// (9)
	GetDlgItemTextA(IDC_DELIM_LIST, ts.DelimList, sizeof(ts.DelimList));

	// (10)
	GetDlgItemTextA(IDC_PASTEDELAY_EDIT, buf, sizeof(buf));
	val = atoi(buf);
	ts.PasteDelayPerLine =
		(val < 0) ? 0 :
		(val > 5000) ? 5000 : val;
}


// CVisualPropPageDlg ダイアログ

CVisualPropPageDlg::CVisualPropPageDlg(HINSTANCE inst, TTCPropertySheet *sheet)
	: TTCPropertyPage(inst, CVisualPropPageDlg::IDD, sheet)
{
	TCHAR UIMsg[MAX_UIMSG];
	get_lang_msgT("DLG_TABSHEET_TITLE_VISUAL", UIMsg, _countof(UIMsg),
	             _T("Visual"), ts.UILanguageFile);
	m_psp.pszTitle = _tcsdup(UIMsg);
	m_psp.dwFlags |= PSP_USETITLE;
}

CVisualPropPageDlg::~CVisualPropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CVisualPropPageDlg メッセージ ハンドラ

void CVisualPropPageDlg::OnInitDialog()
{
	char buf[MAXPATHLEN];

	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_ALPHABLEND, "DLG_TAB_VISUAL_ALPHA" },
		{ IDC_ALPHA_BLEND_ACTIVE_LABEL, "DLG_TAB_VISUAL_ALPHA_ACTIVE_LABEL" },
		{ IDC_ALPHA_BLEND_INACTIVE_LABEL, "DLG_TAB_VISUAL_ALPHA_INACTIVE_LABEL" },
		{ IDC_ETERM_LOOKFEEL, "DLG_TAB_VISUAL_ETERM" },
		{ IDC_BGIMG_CHECK, "DLG_TAB_VISUAL_BGIMG" },
		{ IDC_BGIMG_BRIGHTNESS, "DLG_TAB_VISUAL_BGIMG_BRIGHTNESS" },
		{ IDC_MOUSE, "DLG_TAB_VISUAL_MOUSE" },
		{ IDC_FONT_QUALITY_LABEL, "DLG_TAB_VISUAL_FONT_QUALITY" },
		{ IDC_ANSICOLOR, "DLG_TAB_VISUAL_ANSICOLOR" },
		{ IDC_RED, "DLG_TAB_VISUAL_RED" },
		{ IDC_GREEN, "DLG_TAB_VISUAL_GREEN" },
		{ IDC_BLUE, "DLG_TAB_VISUAL_BLUE" },
		{ IDC_ENABLE_ATTR_COLOR_BOLD, "DLG_TAB_VISUAL_BOLD" },
		{ IDC_ENABLE_ATTR_COLOR_BLINK, "DLG_TAB_VISUAL_BLINK" },
		{ IDC_ENABLE_ATTR_COLOR_REVERSE, "DLG_TAB_VISUAL_REVERSE" },
		{ IDC_ENABLE_URL_COLOR, "DLG_TAB_VISUAL_URL" },
		{ IDC_ENABLE_ANSI_COLOR, "DLG_TAB_VISUAL_ANSI" },
		{ IDC_URL_UNDERLINE, "DLG_TAB_VISUAL_URLUL" },
		{ IDC_RESTART, "DLG_TAB_VISUAL_RESTART" },
	};
	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

	TCHAR uimsg[MAX_UIMSG];
	get_lang_msgT("DLG_TAB_VISUAL_FONT_QUALITY_DEFAULT",
				  uimsg, _countof(uimsg), _T("Default"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_FONT_QUALITY, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_VISUAL_FONT_QUALITY_NONANTIALIASED",
				  uimsg, _countof(uimsg), _T("Non-Antialiased"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_FONT_QUALITY, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_VISUAL_FONT_QUALITY_ANTIALIASED",
				  uimsg, _countof(uimsg), _T("Antialiased"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_FONT_QUALITY, CB_ADDSTRING, 0, (LPARAM)uimsg);
	get_lang_msgT("DLG_TAB_VISUAL_FONT_QUALITY_CLEARTYPE",
				  uimsg, _countof(uimsg), _T("ClearType"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_FONT_QUALITY, CB_ADDSTRING, 0, (LPARAM)uimsg);

	// (1)AlphaBlend

	SetDlgItemNum(IDC_ALPHA_BLEND_ACTIVE, ts.AlphaBlendActive);

	SetDlgItemNum(IDC_ALPHA_BLEND_INACTIVE, ts.AlphaBlendInactive);

	// (2)[BG] BGEnable
	SetCheck(IDC_ETERM_LOOKFEEL, ts.EtermLookfeel.BGEnable);

	// Eterm look-feelの背景画像指定。
	SetDlgItemTextA(IDC_BGIMG_EDIT, ts.BGImageFilePath);

	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", ts.BGImgBrightness);
	SetDlgItemNum(IDC_EDIT_BGIMG_BRIGHTNESS, ts.BGImgBrightness);

	if (ts.EtermLookfeel.BGEnable) {
		EnableDlgItem(IDC_BGIMG_CHECK, TRUE);

		if (strcmp(ts.EtermLookfeel.BGThemeFile, BG_THEME_IMAGEFILE) == 0) {
			SetCheck(IDC_BGIMG_CHECK, BST_CHECKED);
			EnableDlgItem(IDC_BGIMG_EDIT, TRUE);
			EnableDlgItem(IDC_BGIMG_BUTTON, TRUE);

			EnableDlgItem(IDC_BGIMG_BRIGHTNESS, TRUE);
			EnableDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS, TRUE);
		} else {
			SetCheck(IDC_BGIMG_CHECK, BST_UNCHECKED);
			EnableDlgItem(IDC_BGIMG_EDIT, FALSE);
			EnableDlgItem(IDC_BGIMG_BUTTON, FALSE);

			EnableDlgItem(IDC_BGIMG_BRIGHTNESS, FALSE);
			EnableDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS, FALSE);
		}
	} else {
		EnableDlgItem(IDC_BGIMG_CHECK, FALSE);
		EnableDlgItem(IDC_BGIMG_EDIT, FALSE);
		EnableDlgItem(IDC_BGIMG_BUTTON, FALSE);

		EnableDlgItem(IDC_BGIMG_BRIGHTNESS, FALSE);
		EnableDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS, FALSE);
	}

	// (3)Mouse cursor type
	int sel = 0;
	for (int i = 0 ; MouseCursor[i].name ; i++) {
		const char *name = MouseCursor[i].name;
		SendDlgItemMessageA(IDC_MOUSE_CURSOR, CB_ADDSTRING, i, (LPARAM)name);
		if (strcmp(name, ts.MouseCursorName) == 0) {
			sel = i;
		}
	}
	SetCurSel(IDC_MOUSE_CURSOR, sel);

	// (4)Font quality
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

	// (5)ANSI color
	for (int i = 0 ; i < 16 ; i++) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", i);
		SendDlgItemMessageA(IDC_ANSI_COLOR, LB_INSERTSTRING, i, (LPARAM)buf);
	}
	SetupRGBbox(0);
	SendDlgItemMessage(IDC_ANSI_COLOR, LB_SETCURSEL, 0, 0);
	::InvalidateRect(GetDlgItem(IDC_SAMPLE_COLOR), NULL, TRUE);

	// (6)Bold Attr Color
	SetCheck(IDC_ENABLE_ATTR_COLOR_BOLD, (ts.ColorFlag&CF_BOLDCOLOR) != 0);

	// (7)Blink Attr Color
	SetCheck(IDC_ENABLE_ATTR_COLOR_BLINK, (ts.ColorFlag&CF_BLINKCOLOR) != 0);

	// (8)Reverse Attr Color
	SetCheck(IDC_ENABLE_ATTR_COLOR_REVERSE, (ts.ColorFlag&CF_REVERSECOLOR) != 0);

	// (9)URL Color
	SetCheck(IDC_ENABLE_URL_COLOR, (ts.ColorFlag&CF_URLCOLOR) != 0);

	// (10)Color
	SetCheck(IDC_ENABLE_ANSI_COLOR, (ts.ColorFlag&CF_ANSICOLOR) != 0);

	// (11)URL Underline
	SetCheck(IDC_URL_UNDERLINE, (ts.FontFlag&FF_URLUNDERLINE) != 0);

	// ダイアログにフォーカスを当てる
	::SetFocus(GetDlgItem(IDC_ALPHA_BLEND_ACTIVE));
}

BOOL CVisualPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int sel;

	switch (wParam) {
	case IDC_ETERM_LOOKFEEL:
			// チェックされたら Enable/Disable をトグルする。
			if (GetCheck(IDC_ETERM_LOOKFEEL)) {
				EnableDlgItem(IDC_BGIMG_CHECK, TRUE);
				if (GetCheck(IDC_BGIMG_CHECK)) {
					EnableDlgItem(IDC_BGIMG_EDIT, TRUE);
					EnableDlgItem(IDC_BGIMG_BUTTON, TRUE);

					EnableDlgItem(IDC_BGIMG_BRIGHTNESS, TRUE);
					EnableDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS, TRUE);
				} else {
					EnableDlgItem(IDC_BGIMG_EDIT, FALSE);
					EnableDlgItem(IDC_BGIMG_BUTTON, FALSE);

					EnableDlgItem(IDC_BGIMG_BRIGHTNESS, FALSE);
					EnableDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS, FALSE);
				}
			} else {
				EnableDlgItem(IDC_BGIMG_CHECK, FALSE);
				EnableDlgItem(IDC_BGIMG_EDIT, FALSE);
				EnableDlgItem(IDC_BGIMG_BUTTON, FALSE);

				EnableDlgItem(IDC_BGIMG_BRIGHTNESS, FALSE);
				EnableDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS, FALSE);

				// 無効化されたら、BGThemeFile を元に戻す。
				strncpy_s(ts.EtermLookfeel.BGThemeFile, BG_THEME_IMAGEFILE_DEFAULT, sizeof(ts.EtermLookfeel.BGThemeFile));
				// 背景画像も無効化する。
				SetDlgItemTextT(IDC_BGIMG_EDIT, _T(""));
				SetDlgItemInt(IDC_EDIT_BGIMG_BRIGHTNESS, BG_THEME_IMAGE_BRIGHTNESS_DEFAULT);
			}
			return TRUE;

		case IDC_BGIMG_CHECK:
			if (GetCheck(IDC_BGIMG_CHECK)) {
				EnableDlgItem(IDC_BGIMG_EDIT, TRUE);
				EnableDlgItem(IDC_BGIMG_BUTTON, TRUE);

				EnableDlgItem(IDC_BGIMG_BRIGHTNESS, TRUE);
				EnableDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS, TRUE);

				strncpy_s(ts.EtermLookfeel.BGThemeFile, BG_THEME_IMAGEFILE, sizeof(ts.EtermLookfeel.BGThemeFile));
			} else {
				EnableDlgItem(IDC_BGIMG_EDIT, FALSE);
				EnableDlgItem(IDC_BGIMG_BUTTON, FALSE);

				EnableDlgItem(IDC_BGIMG_BRIGHTNESS, FALSE);
				EnableDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS, FALSE);

				// 無効化されたら、BGThemeFile を元に戻す。
				strncpy_s(ts.EtermLookfeel.BGThemeFile, BG_THEME_IMAGEFILE_DEFAULT, sizeof(ts.EtermLookfeel.BGThemeFile));
				// 背景画像も無効化する。
				SetDlgItemTextT(IDC_BGIMG_EDIT, _T(""));
				SetDlgItemInt(IDC_EDIT_BGIMG_BRIGHTNESS, BG_THEME_IMAGE_BRIGHTNESS_DEFAULT);
			}
			return TRUE;

		case IDC_BGIMG_BUTTON | (BN_CLICKED << 16):
			// 背景画像をダイアログで指定する。
			{
				OPENFILENAMEA ofn;
				char szFile[MAX_PATH];

				memset(&ofn, 0, sizeof(ofn));
				memset(szFile, 0, sizeof(szFile));
				ofn.lStructSize = get_OPENFILENAME_SIZEA();
				ofn.hwndOwner = GetSafeHwnd();
				ofn.lpstrFilter = "Image Files(*.jpg;*.jpeg;*.bmp)\0*.jpg;*.jpeg;*.bmp\0All Files(*.*)\0*.*\0";
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = _countof(szFile);
				ofn.lpstrTitle = "select image file";
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				if (GetOpenFileNameA(&ofn) != 0) {
					SetDlgItemTextA(IDC_BGIMG_EDIT, szFile);
				}
			}
			return TRUE;

		case IDC_ANSI_COLOR | (LBN_SELCHANGE << 16):
			sel = SendDlgItemMessage(IDC_ANSI_COLOR, LB_GETCURSEL, 0, 0);
			if (sel != -1) {
				SetupRGBbox(sel);
				::InvalidateRect(GetDlgItem(IDC_SAMPLE_COLOR), NULL, TRUE);
			}
			return TRUE;

		case IDC_COLOR_RED | (EN_KILLFOCUS << 16):
		case IDC_COLOR_GREEN | (EN_KILLFOCUS << 16):
		case IDC_COLOR_BLUE | (EN_KILLFOCUS << 16):
			{
				BYTE r, g, b;
				char buf[8];

				sel = GetCurSel(IDC_ANSI_COLOR);
				if (sel < 0 && sel > sizeof(ts.ANSIColor)-1) {
					return TRUE;
				}

				GetDlgItemTextA(IDC_COLOR_RED, buf, sizeof(buf));
				r = atoi(buf);

				GetDlgItemTextA(IDC_COLOR_GREEN, buf, sizeof(buf));
				g = atoi(buf);

				GetDlgItemTextA(IDC_COLOR_BLUE, buf, sizeof(buf));
				b = atoi(buf);

				ts.ANSIColor[sel] = RGB(r, g, b);

				// 255を超えたRGB値は補正されるので、それをEditに表示する (2007.2.18 maya)
				SetupRGBbox(sel);

				::InvalidateRect(GetDlgItem(IDC_SAMPLE_COLOR), NULL, TRUE);
			}

			return TRUE;
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

HBRUSH CVisualPropPageDlg::OnCtlColor(HDC hDC, HWND hWnd)
{
	if ( hWnd == GetDlgItem(IDC_SAMPLE_COLOR) ) {
		BYTE r, g, b;
		char buf[8];

		GetDlgItemTextA(IDC_COLOR_RED, buf, sizeof(buf));
		r = atoi(buf);

		GetDlgItemTextA(IDC_COLOR_GREEN, buf, sizeof(buf));
		g = atoi(buf);

		GetDlgItemTextA(IDC_COLOR_BLUE, buf, sizeof(buf));
		b = atoi(buf);

		SetBkMode(hDC, TRANSPARENT);
		SetTextColor(hDC, RGB(r, g, b) );

		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
	return TTCPropertyPage::OnCtlColor(hDC, hWnd);
}

void CVisualPropPageDlg::OnOK()
{
	int sel;
	char buf[MAXPATHLEN];
	COLORREF TmpColor;
	int flag_changed = 0;

	// (1)
	GetDlgItemTextA(IDC_ALPHA_BLEND_ACTIVE, buf, sizeof(buf));
	if (isdigit(buf[0])) {
		int i = atoi(buf);
		ts.AlphaBlendActive =
			(i < 0) ? 0 :
			(i > 255) ? 255 : i;
	}
	GetDlgItemTextA(IDC_ALPHA_BLEND_INACTIVE, buf, sizeof(buf));
	if (isdigit(buf[0])) {
		int i = atoi(buf);
		ts.AlphaBlendInactive = 
			(i < 0) ? 0 :
			(i > 255) ? 255 : i;
	}

	// (2)
	// グローバル変数 BGEnable を直接書き換えると、プログラムが落ちることが
	// あるのでコピーを修正するのみとする。(2005.4.24 yutaka)
	if (ts.EtermLookfeel.BGEnable != GetCheck(IDC_ETERM_LOOKFEEL)) {
		flag_changed = 1;
		ts.EtermLookfeel.BGEnable = GetCheck(IDC_ETERM_LOOKFEEL);
	}
	if (ts.EtermLookfeel.BGEnable) {
		GetDlgItemTextA(IDC_BGIMG_EDIT, ts.BGImageFilePath, sizeof(ts.BGImageFilePath));
	} else {
		strncpy_s(ts.BGImageFilePath, sizeof(ts.BGImageFilePath), "%SystemRoot%\\Web\\Wallpaper\\*.bmp", _TRUNCATE);
	}

	GetDlgItemTextA(IDC_EDIT_BGIMG_BRIGHTNESS, buf, sizeof(buf));
	if (isdigit(buf[0])) {
		int i = atoi(buf);
		ts.BGImgBrightness = 
			(i < 0) ? 0 :
			(i > 255) ? 255 : i;
	}

	// (3)
	sel = GetCurSel(IDC_MOUSE_CURSOR);
	if (sel >= 0 && sel < MOUSE_CURSOR_MAX) {
		strncpy_s(ts.MouseCursorName, sizeof(ts.MouseCursorName), MouseCursor[sel].name, _TRUNCATE);
	}

	// (4)Font quality
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

	// (6) Attr Bold Color
	if (((ts.ColorFlag & CF_BOLDCOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_BOLD)) {
		ts.ColorFlag ^= CF_BOLDCOLOR;
	}

	// (7) Attr Blink Color
	if (((ts.ColorFlag & CF_BLINKCOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_BLINK)) {
		ts.ColorFlag ^= CF_BLINKCOLOR;
	}

	// (8) Attr Reverse Color
	if (ts.ColorFlag & CF_REVERSEVIDEO) { // Reverse Videoモード(DECSCNM)時は処理を変える
		if (ts.ColorFlag & CF_REVERSECOLOR) {
			if (!GetCheck(IDC_ENABLE_ATTR_COLOR_REVERSE)) {
				TmpColor = ts.VTColor[0];
				ts.VTColor[0] = ts.VTReverseColor[1];
				ts.VTReverseColor[1] = ts.VTColor[1];
				ts.VTColor[1] = ts.VTReverseColor[0];
				ts.VTReverseColor[0] = TmpColor;
				ts.ColorFlag ^= CF_REVERSECOLOR;
			}
		}
		else if (GetCheck(IDC_ENABLE_ATTR_COLOR_REVERSE)) {
			TmpColor = ts.VTColor[0];
			ts.VTColor[0] = ts.VTReverseColor[0];
			ts.VTReverseColor[0] = ts.VTColor[1];
			ts.VTColor[1] = ts.VTReverseColor[1];
			ts.VTReverseColor[1] = TmpColor;
			ts.ColorFlag ^= CF_REVERSECOLOR;
		}
	}
	else if (((ts.ColorFlag & CF_REVERSECOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_REVERSE)) {
		ts.ColorFlag ^= CF_REVERSECOLOR;
	}

	// (9) URL Color
	if (((ts.ColorFlag & CF_URLCOLOR) != 0) != GetCheck(IDC_ENABLE_URL_COLOR)) {
		ts.ColorFlag ^= CF_URLCOLOR;
	}

	// (10) Color
	if (((ts.ColorFlag & CF_ANSICOLOR) != 0) != GetCheck(IDC_ENABLE_ANSI_COLOR)) {
		ts.ColorFlag ^= CF_ANSICOLOR;
	}

	// (11) URL Underline
	if (((ts.FontFlag & FF_URLUNDERLINE) != 0) != GetCheck(IDC_URL_UNDERLINE)) {
		ts.FontFlag ^= FF_URLUNDERLINE;
	}

	if (flag_changed) {
		// re-launch
		// RestartTeraTerm(GetSafeHwnd(), &ts);
	}
}



// CLogPropPageDlg ダイアログ

CLogPropPageDlg::CLogPropPageDlg(HINSTANCE inst, TTCPropertySheet *sheet)
	: TTCPropertyPage(inst, CLogPropPageDlg::IDD, sheet)
{
	TCHAR UIMsg[MAX_UIMSG];
	get_lang_msgT("DLG_TABSHEET_TITLE_Log", UIMsg, _countof(UIMsg),
	             _T("Log"), ts.UILanguageFile);
	m_psp.pszTitle = _tcsdup(UIMsg);
	m_psp.dwFlags |= PSP_USETITLE;
}

CLogPropPageDlg::~CLogPropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CLogPropPageDlg メッセージ ハンドラ

#define LOG_ROTATE_SIZETYPE_NUM 3
static const char *LogRotateSizeType[] = {
	"Byte", "KB", "MB"
};

static const char *GetLogRotateSizeType(int val)
{
	if (val >= LOG_ROTATE_SIZETYPE_NUM)
		val = 0;

	return LogRotateSizeType[val];
}

void CLogPropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_VIEWLOG_LABEL, "DLG_TAB_LOG_EDITOR" },
		{ IDC_DEFAULTNAME_LABEL, "DLG_TAB_LOG_FILENAME" },
		{ IDC_DEFAULTPATH_LABEL, "DLG_TAB_LOG_FILEPATH" },
		{ IDC_AUTOSTART, "DLG_TAB_LOG_AUTOSTART" },
		// Log rotate
		{ IDC_LOG_ROTATE, "DLG_TAB_LOG_ROTATE" },
		{ IDC_ROTATE_SIZE_TEXT, "DLG_TAB_LOG_ROTATE_SIZE_TEXT" },
		{ IDC_ROTATE_STEP_TEXT, "DLG_TAB_LOG_ROTATESTEP" },
		// Log options
		// FIXME: メッセージカタログは既存のログオプションのものを流用したが、アクセラレータキーが重複するかもしれない。
		{ IDC_LOG_OPTION_GROUP, "DLG_FOPT" },
		{ IDC_OPT_BINARY, "DLG_FOPT_BINARY" },
		{ IDC_OPT_APPEND, "DLG_FOPT_APPEND" },
		{ IDC_OPT_PLAINTEXT, "DLG_FOPT_PLAIN" },
		{ IDC_OPT_HIDEDLG, "DLG_FOPT_HIDEDIALOG" },
		{ IDC_OPT_INCBUF, "DLG_FOPT_ALLBUFFINFIRST" },
		{ IDC_OPT_TIMESTAMP, "DLG_FOPT_TIMESTAMP" },
	};
	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

	TCHAR UIMsg[MAX_UIMSG];
	get_lang_msgT("DLG_FOPT_TIMESTAMP_LOCAL", UIMsg, _countof(UIMsg), _T("Local Time"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_OPT_TIMESTAMP_TYPE, CB_ADDSTRING, 0, (LPARAM)UIMsg);
	get_lang_msgT("DLG_FOPT_TIMESTAMP_UTC", UIMsg, _countof(UIMsg), _T("UTC"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_OPT_TIMESTAMP_TYPE, CB_ADDSTRING, 0, (LPARAM)UIMsg);
	get_lang_msgT("DLG_FOPT_TIMESTAMP_ELAPSED_LOGGING", UIMsg, _countof(UIMsg), _T("Elapsed Time (Logging)"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_OPT_TIMESTAMP_TYPE, CB_ADDSTRING, 0, (LPARAM)UIMsg);
	get_lang_msgT("DLG_FOPT_TIMESTAMP_ELAPSED_CONNECTION", UIMsg, _countof(UIMsg), _T("Elapsed Time (Connection)"), ts.UILanguageFile);
	SendDlgItemMessage(IDC_OPT_TIMESTAMP_TYPE, CB_ADDSTRING, 0, (LPARAM)UIMsg);


	// Viewlog Editor path (2005.1.29 yutaka)
	SetDlgItemTextA(IDC_VIEWLOG_EDITOR, ts.ViewlogEditor);

	// Log Default File Name (2006.8.28 maya)
	SetDlgItemTextA(IDC_DEFAULTNAME_EDITOR, ts.LogDefaultName);

	// Log Default File Path (2007.5.30 maya)
	SetDlgItemTextA(IDC_DEFAULTPATH_EDITOR, ts.LogDefaultPath);

	/* Auto start logging (2007.5.31 maya) */
	SetCheck(IDC_AUTOSTART, ts.LogAutoStart);

	// Log rotate
	SetCheck(IDC_LOG_ROTATE, ts.LogRotate != ROTATE_NONE);

	for (int i = 0 ; i < LOG_ROTATE_SIZETYPE_NUM ; i++) {
		SendDlgItemMessageA(IDC_ROTATE_SIZE_TYPE, CB_ADDSTRING, 0, (LPARAM)LogRotateSizeType[i]);
	}
	int TmpLogRotateSize = ts.LogRotateSize;
	for (int i = 0 ; i < ts.LogRotateSizeType ; i++)
		TmpLogRotateSize /= 1024;
	SetDlgItemInt(IDC_ROTATE_SIZE, TmpLogRotateSize, FALSE);
	SendDlgItemMessageA(IDC_ROTATE_SIZE_TYPE, CB_SELECTSTRING, -1, (LPARAM)GetLogRotateSizeType(ts.LogRotateSizeType));
	SetDlgItemInt(IDC_ROTATE_STEP, ts.LogRotateStep, FALSE);
	if (ts.LogRotate == ROTATE_NONE) {
		EnableDlgItem(IDC_ROTATE_SIZE_TEXT, FALSE);
		EnableDlgItem(IDC_ROTATE_SIZE, FALSE);
		EnableDlgItem(IDC_ROTATE_SIZE_TYPE, FALSE);
		EnableDlgItem(IDC_ROTATE_STEP_TEXT, FALSE);
		EnableDlgItem(IDC_ROTATE_STEP, FALSE);
	} else {
		EnableDlgItem(IDC_ROTATE_SIZE_TEXT, TRUE);
		EnableDlgItem(IDC_ROTATE_SIZE, TRUE);
		EnableDlgItem(IDC_ROTATE_SIZE_TYPE, TRUE);
		EnableDlgItem(IDC_ROTATE_STEP_TEXT, TRUE);
		EnableDlgItem(IDC_ROTATE_STEP, TRUE);
	}

	// Log options
	SetCheck(IDC_OPT_BINARY, ts.LogBinary != 0);
	if (ts.LogBinary) {
		EnableDlgItem(IDC_OPT_PLAINTEXT, FALSE);
		EnableDlgItem(IDC_OPT_TIMESTAMP, FALSE);
	} else {
		EnableDlgItem(IDC_OPT_PLAINTEXT, TRUE);
		EnableDlgItem(IDC_OPT_TIMESTAMP, TRUE);
	}
	SetCheck(IDC_OPT_APPEND, ts.Append != 0);
	SetCheck(IDC_OPT_PLAINTEXT, ts.LogTypePlainText != 0);
	SetCheck(IDC_OPT_HIDEDLG, ts.LogHideDialog != 0);
	SetCheck(IDC_OPT_INCBUF, ts.LogAllBuffIncludedInFirst != 0);
	SetCheck(IDC_OPT_TIMESTAMP, ts.LogTimestamp != 0);

	SetCurSel(IDC_OPT_TIMESTAMP_TYPE, ts.LogTimestampType);
	if (ts.LogBinary || !ts.LogTimestamp) {
		EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, FALSE);
	}
	else {
		EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, TRUE);
	}
/*
	switch (ts.LogTimestampType) {
		case CSF_CBRW:
			cmb->SetCurSel(3);
			break;
		case CSF_CBREAD:
			cmb->SetCurSel(2);
			break;
		case CSF_CBWRITE:
			cmb->SetCurSel(1);
			break;
		default: // off
			cmb->SetCurSel(0);
			break;
	}
*/

	// ダイアログにフォーカスを当てる
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_VIEWLOG_EDITOR));
}

BOOL CLogPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	char uimsg[MAX_UIMSG];
	char buf[MAX_PATH];
	char buf2[MAX_PATH];

	switch (wParam) {
		case IDC_VIEWLOG_PATH | (BN_CLICKED << 16):
			{
				OPENFILENAMEA ofn;

				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = get_OPENFILENAME_SIZEA();
				ofn.hwndOwner = GetSafeHwnd();
				get_lang_msg("FILEDLG_SELECT_LOGVIEW_APP_FILTER", ts.UIMsg, sizeof(ts.UIMsg),
				             "exe(*.exe)\\0*.exe\\0all(*.*)\\0*.*\\0\\0", ts.UILanguageFile);
				ofn.lpstrFilter = ts.UIMsg;
				ofn.lpstrFile = ts.ViewlogEditor;
				ofn.nMaxFile = sizeof(ts.ViewlogEditor);
				get_lang_msg("FILEDLG_SELECT_LOGVIEW_APP_TITLE", uimsg, sizeof(uimsg),
				             "Choose a executing file with launching logging file", ts.UILanguageFile);
				ofn.lpstrTitle = uimsg;
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				if (GetOpenFileNameA(&ofn) != 0) {
					SetDlgItemTextA(IDC_VIEWLOG_EDITOR, ts.ViewlogEditor);
				}
			}
			return TRUE;

		case IDC_DEFAULTPATH_PUSH | (BN_CLICKED << 16):
			// ログディレクトリの選択ダイアログ
			get_lang_msg("FILEDLG_SELECT_LOGDIR_TITLE", ts.UIMsg, sizeof(ts.UIMsg),
			             "Select log folder", ts.UILanguageFile);
			GetDlgItemText(IDC_DEFAULTPATH_EDITOR, buf, sizeof(buf));
			if (doSelectFolder(GetSafeHwnd(), buf2, sizeof(buf2), buf, ts.UIMsg)) {
				SetDlgItemText(IDC_DEFAULTPATH_EDITOR, buf2);
			}

			return TRUE;

		case IDC_LOG_ROTATE | (BN_CLICKED << 16):
			{
				if (GetCheck(IDC_LOG_ROTATE)) {
					EnableDlgItem(IDC_ROTATE_SIZE_TEXT, TRUE);
					EnableDlgItem(IDC_ROTATE_SIZE, TRUE);
					EnableDlgItem(IDC_ROTATE_SIZE_TYPE, TRUE);
					EnableDlgItem(IDC_ROTATE_STEP_TEXT, TRUE);
					EnableDlgItem(IDC_ROTATE_STEP, TRUE);
				} else {
					EnableDlgItem(IDC_ROTATE_SIZE_TEXT, FALSE);
					EnableDlgItem(IDC_ROTATE_SIZE, FALSE);
					EnableDlgItem(IDC_ROTATE_SIZE_TYPE, FALSE);
					EnableDlgItem(IDC_ROTATE_STEP_TEXT, FALSE);
					EnableDlgItem(IDC_ROTATE_STEP, FALSE);
				}

			}
			return TRUE;

		case IDC_OPT_BINARY | (BN_CLICKED << 16):
			{
				if (GetCheck(IDC_OPT_BINARY)) {
					EnableDlgItem(IDC_OPT_PLAINTEXT, FALSE);
					EnableDlgItem(IDC_OPT_TIMESTAMP, FALSE);
					EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, FALSE);
				} else {
					EnableDlgItem(IDC_OPT_PLAINTEXT, TRUE);
					EnableDlgItem(IDC_OPT_TIMESTAMP, TRUE);

					if (GetCheck(IDC_OPT_TIMESTAMP)) {
						EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, TRUE);
					}
				}
			}
			return TRUE;

		case IDC_OPT_TIMESTAMP | (BN_CLICKED << 16):
			{
				if (GetCheck(IDC_OPT_TIMESTAMP)) {
					EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, TRUE);
				} else {
					EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, FALSE);
				}
			}
			return TRUE;
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

void CLogPropPageDlg::OnOK()
{
	char buf[80], buf2[80];
	time_t time_local;
	struct tm *tm_local;
	TCHAR uimsg[MAX_UIMSG];
	TCHAR uimsg2[MAX_UIMSG];

	// Viewlog Editor path (2005.1.29 yutaka)
	GetDlgItemText(IDC_VIEWLOG_EDITOR, ts.ViewlogEditor, _countof(ts.ViewlogEditor));

	// Log Default File Name (2006.8.28 maya)
	GetDlgItemTextA(IDC_DEFAULTNAME_EDITOR, buf, sizeof(buf));
	if (isInvalidStrftimeChar(buf)) {
		get_lang_msgT("MSG_ERROR", uimsg, _countof(uimsg), _T("ERROR"), ts.UILanguageFile);
		get_lang_msgT("MSG_LOGFILE_INVALID_CHAR_ERROR", uimsg2, _countof(uimsg2),
		              _T("Invalid character is included in log file name."), ts.UILanguageFile);
		MessageBox(uimsg2, uimsg, MB_ICONEXCLAMATION);
		return;
	}

	// 現在時刻を取得
	time(&time_local);
	tm_local = localtime(&time_local);
	// 時刻文字列に変換
	if (strlen(buf) != 0 && strftime(buf2, sizeof(buf2), buf, tm_local) == 0) {
		get_lang_msgT("MSG_ERROR", uimsg, _countof(uimsg), _T("ERROR"), ts.UILanguageFile);
		get_lang_msgT("MSG_LOGFILE_TOOLONG_ERROR", uimsg2, _countof(uimsg2),
					  _T("The log file name is too long."), ts.UILanguageFile);
		MessageBox(uimsg2, uimsg, MB_ICONEXCLAMATION);
		return;
	}
	if (isInvalidFileNameChar(buf2)) {
		get_lang_msgT("MSG_ERROR", uimsg, _countof(uimsg), _T("ERROR"), ts.UILanguageFile);
		get_lang_msgT("MSG_LOGFILE_INVALID_CHAR_ERROR", uimsg2, _countof(uimsg2),
					  _T("Invalid character is included in log file name."), ts.UILanguageFile);
		MessageBox(uimsg2, uimsg, MB_ICONEXCLAMATION);
		return;
	}
	strncpy_s(ts.LogDefaultName, sizeof(ts.LogDefaultName), buf, _TRUNCATE);

	// Log Default File Path (2007.5.30 maya)
	GetDlgItemText(IDC_DEFAULTPATH_EDITOR, ts.LogDefaultPath, _countof(ts.LogDefaultPath));

	/* Auto start logging (2007.5.31 maya) */
	ts.LogAutoStart = GetCheck(IDC_AUTOSTART);

	/* Log Rotate */
	if (GetCheck(IDC_LOG_ROTATE)) {  /* on */
		ts.LogRotate = ROTATE_SIZE;
		GetDlgItemText(IDC_ROTATE_SIZE_TYPE, buf, _countof(buf));
		ts.LogRotateSizeType = 0;
		for (int i = 0 ; i < LOG_ROTATE_SIZETYPE_NUM ; i++) {
			if (strcmp(buf, LogRotateSizeType[i]) == 0) {
				ts.LogRotateSizeType = i;
				break;
			}
		}
		ts.LogRotateSize = GetDlgItemInt(IDC_ROTATE_SIZE);
		for (int i = 0 ; i < ts.LogRotateSizeType ; i++)
			ts.LogRotateSize *= 1024;

		ts.LogRotateStep = GetDlgItemInt(IDC_ROTATE_STEP);

	} else { /* off */
		ts.LogRotate = ROTATE_NONE;
		/* 残りのメンバーは意図的に設定を残す。*/
	}

	// Log Options
	if (GetCheck(IDC_OPT_BINARY)) {
		ts.LogBinary = 1;
	}
	else {
		ts.LogBinary = 0;
	}

	if (GetCheck(IDC_OPT_APPEND)) {
		ts.Append = 1;
	}
	else {
		ts.Append = 0;
	}

	if (GetCheck(IDC_OPT_PLAINTEXT)) {
		ts.LogTypePlainText = 1;
	}
	else {
		ts.LogTypePlainText = 0;
	}

	if (GetCheck(IDC_OPT_HIDEDLG)) {
		ts.LogHideDialog = 1;
	}
	else {
		ts.LogHideDialog = 0;
	}

	if (GetCheck(IDC_OPT_INCBUF)) {
		ts.LogAllBuffIncludedInFirst = 1;
	}
	else {
		ts.LogAllBuffIncludedInFirst = 0;
	}

	if (GetCheck(IDC_OPT_TIMESTAMP)) {
		ts.LogTimestamp = 1;
	}
	else {
		ts.LogTimestamp = 0;
	}

	ts.LogTimestampType = GetCurSel(IDC_OPT_TIMESTAMP_TYPE);
}

/////////////////////////////

// CCygwinPropPageDlg ダイアログ

CCygwinPropPageDlg::CCygwinPropPageDlg(HINSTANCE inst, TTCPropertySheet *sheet)
	: TTCPropertyPage(inst, CCygwinPropPageDlg::IDD, sheet)
{
	TCHAR UIMsg[MAX_UIMSG];
	get_lang_msgT("DLG_TABSHEET_TITLE_CYGWIN", UIMsg, _countof(UIMsg),
	             _T("Cygwin"), ts.UILanguageFile);
	m_psp.pszTitle = _tcsdup(UIMsg);
	m_psp.dwFlags |= PSP_USETITLE;
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
	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

	memcpy(&settings, &ts.CygtermSettings, sizeof(cygterm_t));

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
	char buf[MAX_PATH], buf2[MAX_PATH];

	switch (wParam) {
		case IDC_SELECT_FILE | (BN_CLICKED << 16):
			// Cygwin install ディレクトリの選択ダイアログ
			get_lang_msg("DIRDLG_CYGTERM_DIR_TITLE", ts.UIMsg, sizeof(ts.UIMsg),
			             "Select Cygwin directory", ts.UILanguageFile);
			GetDlgItemTextA(IDC_CYGWIN_PATH, buf, sizeof(buf));
			if (doSelectFolder(GetSafeHwnd(), buf2, sizeof(buf2), buf, ts.UIMsg)) {
				SetDlgItemTextA(IDC_CYGWIN_PATH, buf2);
			}
			return TRUE;
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

void CCygwinPropPageDlg::OnOK()
{
	// writing to CygTerm config file
	GetDlgItemTextA(IDC_TERM_EDIT, settings.term, sizeof(settings.term));
	GetDlgItemTextA(IDC_TERM_TYPE, settings.term_type, sizeof(settings.term_type));
	GetDlgItemTextA(IDC_PORT_START, settings.port_start, sizeof(settings.port_start));
	GetDlgItemTextA(IDC_PORT_RANGE, settings.port_range, sizeof(settings.port_range));
	GetDlgItemTextA(IDC_SHELL, settings.shell, sizeof(settings.shell));
	GetDlgItemTextA(IDC_ENV1, settings.env1, sizeof(settings.env1));
	GetDlgItemTextA(IDC_ENV2, settings.env2, sizeof(settings.env2));

	settings.login_shell = GetCheck(IDC_LOGIN_SHELL);
	settings.home_chdir = GetCheck(IDC_HOME_CHDIR);
	settings.agent_proxy = GetCheck(IDC_AGENT_PROXY);

	memcpy(&ts.CygtermSettings, &settings, sizeof(cygterm_t));

	// 設定を書き込むためフラグを立てておく。
	ts.CygtermSettings.update_flag = TRUE;

	// Cygwin install path
	GetDlgItemTextA(IDC_CYGWIN_PATH, ts.CygwinDirectory, sizeof(ts.CygwinDirectory));
}

// CAddSettingPropSheetDlg
CAddSettingPropSheetDlg::CAddSettingPropSheetDlg(
	HINSTANCE hInstance, LPCTSTR pszCaption, HWND hParentWnd) :
	TTCPropertySheet(hInstance, pszCaption, hParentWnd)
{
	m_GeneralPage = new CGeneralPropPageDlg(hInstance, this);
	m_SequencePage = new CSequencePropPageDlg(hInstance, this);
	m_CopypastePage = new CCopypastePropPageDlg(hInstance, this);
	m_VisualPage = new CVisualPropPageDlg(hInstance, this);
	m_LogPage = new CLogPropPageDlg(hInstance, this);
	m_CygwinPage = new CCygwinPropPageDlg(hInstance, this);
		
	hPsp[0] = CreatePropertySheetPage(m_GeneralPage->GetPropSheetPage());
	hPsp[1] = CreatePropertySheetPage(m_SequencePage->GetPropSheetPage());
	hPsp[2] = CreatePropertySheetPage(m_CopypastePage->GetPropSheetPage());
	hPsp[3] = CreatePropertySheetPage(m_VisualPage->GetPropSheetPage());
	hPsp[4] = CreatePropertySheetPage(m_LogPage->GetPropSheetPage());
	hPsp[5] = CreatePropertySheetPage(m_CygwinPage->GetPropSheetPage());
	m_psh.nPages = 6;
	m_psh.phpage = hPsp;

	TCHAR UIMsg[MAX_UIMSG];
	get_lang_msgT("DLG_TABSHEET_TITLE", UIMsg, _countof(UIMsg),
				 pszCaption, ts.UILanguageFile);
	m_psh.pszCaption = _tcsdup(UIMsg);
}

CAddSettingPropSheetDlg::~CAddSettingPropSheetDlg()
{
	free((void*)m_psh.pszCaption);
	delete m_GeneralPage;
	delete m_SequencePage;
	delete m_CopypastePage;
	delete m_VisualPage;
	delete m_LogPage;
	delete m_CygwinPage;
}

void CAddSettingPropSheetDlg::OnInitDialog()
{
	TTCPropertySheet::OnInitDialog();
	CenterWindow(m_hWnd, m_hParentWnd);
}
