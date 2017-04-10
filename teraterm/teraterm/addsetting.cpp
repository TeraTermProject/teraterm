/*
 * Additional settings dialog
 */

#include <afxwin.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <commctrl.h>

#include "addsetting.h"
#include "teraterm.h"
#include "tttypes.h"
#include "ttwinman.h"
#include "ttcommon.h"
#include "ttftypes.h"

mouse_cursor_t MouseCursor[] = {
	{"ARROW", IDC_ARROW},
	{"IBEAM", IDC_IBEAM},
	{"CROSS", IDC_CROSS},
	{"HAND", IDC_HAND},
	{NULL, NULL},
};
#define MOUSE_CURSOR_MAX (sizeof(MouseCursor)/sizeof(MouseCursor[0]) - 1)

// 本体は vtwin.cpp
extern void SetWindowStyle(TTTSet *ts);


static void SetupRGBbox(HWND hDlgWnd, int index)
{
	HWND hWnd;
	BYTE c;
	char buf[10];

	hWnd = GetDlgItem(hDlgWnd, IDC_COLOR_RED);
	c = GetRValue(ts.ANSIColor[index]);
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", c);
	SendMessage(hWnd, WM_SETTEXT , 0, (LPARAM)buf);

	hWnd = GetDlgItem(hDlgWnd, IDC_COLOR_GREEN);
	c = GetGValue(ts.ANSIColor[index]);
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", c);
	SendMessage(hWnd, WM_SETTEXT , 0, (LPARAM)buf);

	hWnd = GetDlgItem(hDlgWnd, IDC_COLOR_BLUE);
	c = GetBValue(ts.ANSIColor[index]);
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", c);
	SendMessage(hWnd, WM_SETTEXT , 0, (LPARAM)buf);
}



// CGeneralPropPageDlg ダイアログ

IMPLEMENT_DYNAMIC(CGeneralPropPageDlg, CPropertyPage)

CGeneralPropPageDlg::CGeneralPropPageDlg()
	: CPropertyPage(CGeneralPropPageDlg::IDD)
{
}

CGeneralPropPageDlg::~CGeneralPropPageDlg()
{
	if (DlgGeneralFont != NULL) {
		DeleteObject(DlgGeneralFont);
	}
}

BEGIN_MESSAGE_MAP(CGeneralPropPageDlg, CPropertyPage)
END_MESSAGE_MAP()

// CGeneralPropPageDlg メッセージ ハンドラ

BOOL CGeneralPropPageDlg::OnInitDialog()
{
	char uimsg[MAX_UIMSG];
	char buf[64];
	CButton *btn;

	CPropertyPage::OnInitDialog();

	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_TAHOMA_FONT", GetSafeHwnd(), &logfont, &DlgGeneralFont, ts.UILanguageFile)) {
		SendDlgItemMessage(IDC_CLICKABLE_URL, WM_SETFONT, (WPARAM)DlgGeneralFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DISABLE_SENDBREAK, WM_SETFONT, (WPARAM)DlgGeneralFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ACCEPT_BROADCAST, WM_SETFONT, (WPARAM)DlgGeneralFont, MAKELPARAM(TRUE,0)); // 337: 2007/03/20
		SendDlgItemMessage(IDC_MOUSEWHEEL_SCROLL_LINE, WM_SETFONT, (WPARAM)DlgGeneralFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_SCROLL_LINE, WM_SETFONT, (WPARAM)DlgGeneralFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE, WM_SETFONT, (WPARAM)DlgGeneralFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CLEAR_ON_RESIZE, WM_SETFONT, (WPARAM)DlgGeneralFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CURSOR_CHANGE_IME, WM_SETFONT, (WPARAM)DlgGeneralFont, MAKELPARAM(TRUE,0));
	}
	else {
		DlgGeneralFont = NULL;
	}

	GetDlgItemText(IDC_CLICKABLE_URL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_GENERAL_CLICKURL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CLICKABLE_URL, ts.UIMsg);
	GetDlgItemText(IDC_DISABLE_SENDBREAK, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_GENERAL_DISABLESENDBREAK", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_DISABLE_SENDBREAK, ts.UIMsg);
	GetDlgItemText(IDC_ACCEPT_BROADCAST, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_GENERAL_ACCEPTBROADCAST", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ACCEPT_BROADCAST, ts.UIMsg);
	GetDlgItemText(IDC_MOUSEWHEEL_SCROLL_LINE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_GENERAL_MOUSEWHEEL_SCROLL_LINE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_MOUSEWHEEL_SCROLL_LINE, ts.UIMsg);
	GetDlgItemText(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_GENERAL_AUTOSCROLL_ONLY_IN_BOTTOM_LINE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE, ts.UIMsg);
	GetDlgItemText(IDC_CLEAR_ON_RESIZE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_GENERAL_CLEAR_ON_RESIZE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CLEAR_ON_RESIZE, ts.UIMsg);
	GetDlgItemText(IDC_CURSOR_CHANGE_IME, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_GENERAL_CURSOR_CHANGE_IME", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CURSOR_CHANGE_IME, ts.UIMsg);

	// (1)DisableAcceleratorSendBreak
	btn = (CButton *)GetDlgItem(IDC_DISABLE_SENDBREAK);
	btn->SetCheck(ts.DisableAcceleratorSendBreak);

	// (2)EnableClickableUrl
	btn = (CButton *)GetDlgItem(IDC_CLICKABLE_URL);
	btn->SetCheck(ts.EnableClickableUrl);

	// (3)AcceptBroadcast 337: 2007/03/20
	btn = (CButton *)GetDlgItem(IDC_ACCEPT_BROADCAST);
	btn->SetCheck(ts.AcceptBroadcast);

	// (4)IDC_MOUSEWHEEL_SCROLL_LINE
	_snprintf_s(buf, sizeof(buf), "%d", ts.MouseWheelScrollLine);
	SetDlgItemText(IDC_SCROLL_LINE, buf);

	// (5)IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE
	btn = (CButton *)GetDlgItem(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE);
	btn->SetCheck(ts.AutoScrollOnlyInBottomLine);

	// (6)IDC_CLEAR_ON_RESIZE
	btn = (CButton *)GetDlgItem(IDC_CLEAR_ON_RESIZE);
	btn->SetCheck((ts.TermFlag & TF_CLEARONRESIZE) != 0);

	// (7)IDC_CURSOR_CHANGE_IME
	btn = (CButton *)GetDlgItem(IDC_CURSOR_CHANGE_IME);
	btn->SetCheck((ts.WindowFlag & WF_IMECURSORCHANGE) != 0);

	// ダイアログにフォーカスを当てる (2004.12.7 yutaka)
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_CLICKABLE_URL));

	return FALSE;
}

void CGeneralPropPageDlg::OnOK()
{
	CButton *btn;
	char buf[64];
	int val;

	// (1)
	btn = (CButton *)GetDlgItem(IDC_DISABLE_SENDBREAK);
	ts.DisableAcceleratorSendBreak = btn->GetCheck();

	// (2)
	btn = (CButton *)GetDlgItem(IDC_CLICKABLE_URL);
	ts.EnableClickableUrl = btn->GetCheck();

	// (3) 337: 2007/03/20
	btn = (CButton *)GetDlgItem(IDC_ACCEPT_BROADCAST);
	ts.AcceptBroadcast = btn->GetCheck();

	// (4)IDC_MOUSEWHEEL_SCROLL_LINE
	GetDlgItemText(IDC_SCROLL_LINE, buf, sizeof(buf));
	val = atoi(buf);
	if (val > 0)
		ts.MouseWheelScrollLine = val;

	// (5)IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE
	btn = (CButton *)GetDlgItem(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE);
	ts.AutoScrollOnlyInBottomLine = btn->GetCheck();

	// (6)IDC_CLEAR_ON_RESIZE
	btn = (CButton *)GetDlgItem(IDC_CLEAR_ON_RESIZE);
	if (((ts.TermFlag & TF_CLEARONRESIZE) != 0) != btn->GetCheck()) {
		ts.TermFlag ^= TF_CLEARONRESIZE;
	}

	// (7)IDC_CURSOR_CHANGE_IME
	btn = (CButton *)GetDlgItem(IDC_CURSOR_CHANGE_IME);
	if (((ts.WindowFlag & WF_IMECURSORCHANGE) != 0) != btn->GetCheck()) {
		ts.WindowFlag ^= WF_IMECURSORCHANGE;
	}
}



// CSequencePropPageDlg ダイアログ

IMPLEMENT_DYNAMIC(CSequencePropPageDlg, CPropertyPage)

CSequencePropPageDlg::CSequencePropPageDlg()
	: CPropertyPage(CSequencePropPageDlg::IDD)
{
}

CSequencePropPageDlg::~CSequencePropPageDlg()
{
	if (DlgSequenceFont != NULL) {
		DeleteObject(DlgSequenceFont);
	}
}

BEGIN_MESSAGE_MAP(CSequencePropPageDlg, CPropertyPage)
END_MESSAGE_MAP()

// CSequencePropPageDlg メッセージ ハンドラ

BOOL CSequencePropPageDlg::OnInitDialog()
{
	char uimsg[MAX_UIMSG];
	CButton *btn, *btn2;
	CComboBox *cmb;

	CPropertyPage::OnInitDialog();

	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_TAHOMA_FONT", GetSafeHwnd(), &logfont, &DlgSequenceFont, ts.UILanguageFile)) {
		SendDlgItemMessage(IDC_ACCEPT_MOUSE_EVENT_TRACKING, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DISABLE_MOUSE_TRACKING_CTRL, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING_LABEL, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_TITLE_REPORT_LABEL, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_TITLE_REPORT, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_WINDOW_CTRL, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_WINDOW_REPORT, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CURSOR_CTRL_SEQ, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CLIPBOARD_ACCESS_LABEL, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CLIPBOARD_ACCESS, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CLIPBOARD_NOTIFY, WM_SETFONT, (WPARAM)DlgSequenceFont, MAKELPARAM(TRUE,0));
	}
	else {
		DlgSequenceFont = NULL;
	}

	GetDlgItemText(IDC_ACCEPT_MOUSE_EVENT_TRACKING, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQUENCE_ACCEPT_MOUSE_EVENT_TRACKING", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ACCEPT_MOUSE_EVENT_TRACKING, ts.UIMsg);
	GetDlgItemText(IDC_DISABLE_MOUSE_TRACKING_CTRL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQUENCE_DISABLE_MOUSE_TRACKING_CTRL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_DISABLE_MOUSE_TRACKING_CTRL, ts.UIMsg);
	GetDlgItemText(IDC_ACCEPT_TITLE_CHANGING_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ACCEPT_TITLE_CHANGING_LABEL, ts.UIMsg);

	get_lang_msg("DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_OFF", ts.UIMsg, sizeof(ts.UIMsg), "off", ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_OVERWRITE", ts.UIMsg, sizeof(ts.UIMsg), "overwrite", ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_AHEAD", ts.UIMsg, sizeof(ts.UIMsg), "ahead", ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_LAST", ts.UIMsg, sizeof(ts.UIMsg), "last", ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);

	GetDlgItemText(IDC_CURSOR_CTRL_SEQ, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQUENCE_CURSOR_CTRL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CURSOR_CTRL_SEQ, ts.UIMsg);
	GetDlgItemText(IDC_WINDOW_CTRL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQUENCE_WINDOW_CTRL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_WINDOW_CTRL, ts.UIMsg);
	GetDlgItemText(IDC_WINDOW_REPORT, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQUENCE_WINDOW_REPORT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_WINDOW_REPORT, ts.UIMsg);
	GetDlgItemText(IDC_TITLE_REPORT_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQUENCE_TITLE_REPORT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_TITLE_REPORT_LABEL, ts.UIMsg);

	get_lang_msg("DLG_TAB_SEQUENCE_TITLE_REPORT_IGNORE", ts.UIMsg, sizeof(ts.UIMsg), "ignore", ts.UILanguageFile);
	SendDlgItemMessage(IDC_TITLE_REPORT, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQUENCE_TITLE_REPORT_ACCEPT", ts.UIMsg, sizeof(ts.UIMsg), "accept", ts.UILanguageFile);
	SendDlgItemMessage(IDC_TITLE_REPORT, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQUENCE_TITLE_REPORT_EMPTY", ts.UIMsg, sizeof(ts.UIMsg), "empty", ts.UILanguageFile);
	SendDlgItemMessage(IDC_TITLE_REPORT, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);

	GetDlgItemText(IDC_CLIPBOARD_ACCESS_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CLIPBOARD_ACCESS_LABEL, ts.UIMsg);

	get_lang_msg("DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_OFF", ts.UIMsg, sizeof(ts.UIMsg), "off", ts.UILanguageFile);
	SendDlgItemMessage(IDC_CLIPBOARD_ACCESS, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_WRITE", ts.UIMsg, sizeof(ts.UIMsg), "write only", ts.UILanguageFile);
	SendDlgItemMessage(IDC_CLIPBOARD_ACCESS, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_READ", ts.UIMsg, sizeof(ts.UIMsg), "read only", ts.UILanguageFile);
	SendDlgItemMessage(IDC_CLIPBOARD_ACCESS, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_ON", ts.UIMsg, sizeof(ts.UIMsg), "read/write", ts.UILanguageFile);
	SendDlgItemMessage(IDC_CLIPBOARD_ACCESS, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);

	GetDlgItemText(IDC_CLIPBOARD_NOTIFY, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQUENCE_CLIPBOARD_NOTIFY", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CLIPBOARD_NOTIFY, ts.UIMsg);

	// (1)IDC_ACCEPT_MOUSE_EVENT_TRACKING
	btn = (CButton *)GetDlgItem(IDC_ACCEPT_MOUSE_EVENT_TRACKING);
	btn2 = (CButton *)GetDlgItem(IDC_DISABLE_MOUSE_TRACKING_CTRL);
	btn->SetCheck(ts.MouseEventTracking);
	if (ts.MouseEventTracking) {
		btn2->EnableWindow(TRUE);
	} else {
		btn2->EnableWindow(FALSE);
	}

	// (2)IDC_DISABLE_MOUSE_TRACKING_CTRL
	btn2->SetCheck(ts.DisableMouseTrackingByCtrl);

	// (3)IDC_ACCEPT_TITLE_CHANGING
	cmb = (CComboBox *)GetDlgItem(IDC_ACCEPT_TITLE_CHANGING);
	cmb->SetCurSel(ts.AcceptTitleChangeRequest);

	// (4)IDC_TITLE_REPORT
	cmb = (CComboBox *)GetDlgItem(IDC_TITLE_REPORT);
	switch (ts.WindowFlag & WF_TITLEREPORT) {
		case IdTitleReportIgnore:
			cmb->SetCurSel(0);
			break;
		case IdTitleReportAccept:
			cmb->SetCurSel(1);
			break;
		default: // IdTitleReportEmpty
			cmb->SetCurSel(2);
			break;
	}

	// (5)IDC_WINDOW_CTRL
	btn = (CButton *)GetDlgItem(IDC_WINDOW_CTRL);
	btn->SetCheck((ts.WindowFlag & WF_WINDOWCHANGE) != 0);

	// (6)IDC_WINDOW_REPORT
	btn = (CButton *)GetDlgItem(IDC_WINDOW_REPORT);
	btn->SetCheck((ts.WindowFlag & WF_WINDOWREPORT) != 0);

	// (7)IDC_CURSOR_CTRL_SEQ
	btn = (CButton *)GetDlgItem(IDC_CURSOR_CTRL_SEQ);
	btn->SetCheck((ts.WindowFlag & WF_CURSORCHANGE) != 0);

	// (8)IDC_CLIPBOARD_ACCESS
	cmb = (CComboBox *)GetDlgItem(IDC_CLIPBOARD_ACCESS);
	switch (ts.CtrlFlag & CSF_CBRW) {
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

	// (9)IDC_CLIPBOARD_NOTIFY
	btn = (CButton *)GetDlgItem(IDC_CLIPBOARD_NOTIFY);
	btn->SetCheck(ts.NotifyClipboardAccess);

	// ダイアログにフォーカスを当てる (2004.12.7 yutaka)
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_ACCEPT_MOUSE_EVENT_TRACKING));

	return FALSE;
}

BOOL CSequencePropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	CButton *btn, *btn2;

	switch (wParam) {
		case IDC_ACCEPT_MOUSE_EVENT_TRACKING | (BN_CLICKED << 16):
			btn = (CButton *)GetDlgItem(IDC_ACCEPT_MOUSE_EVENT_TRACKING);
			btn2 = (CButton *)GetDlgItem(IDC_DISABLE_MOUSE_TRACKING_CTRL);
			if (btn->GetCheck()) {
				btn2->EnableWindow(TRUE);
			} else {
				btn2->EnableWindow(FALSE);
			}
			return TRUE;
	}
	return CPropertyPage::OnCommand(wParam, lParam);
}

void CSequencePropPageDlg::OnOK()
{
	CButton *btn;
	CComboBox *cmb;
	int sel;

	// (1)IDC_ACCEPT_MOUSE_EVENT_TRACKING
	btn = (CButton *)GetDlgItem(IDC_ACCEPT_MOUSE_EVENT_TRACKING);
	ts.MouseEventTracking = btn->GetCheck();

	// (2)IDC_DISABLE_MOUSE_TRACKING_CTRL
	btn = (CButton *)GetDlgItem(IDC_DISABLE_MOUSE_TRACKING_CTRL);
	ts.DisableMouseTrackingByCtrl = btn->GetCheck();

	// (3)IDC_ACCEPT_TITLE_CHANGING
	cmb = (CComboBox *)GetDlgItem(IDC_ACCEPT_TITLE_CHANGING);
	sel = cmb->GetCurSel();
	if (0 <= sel && sel <= IdTitleChangeRequestMax) {
		ts.AcceptTitleChangeRequest = sel;
	}

	// (4)IDC_TITLE_REPORT
	cmb = (CComboBox *)GetDlgItem(IDC_TITLE_REPORT);
	switch (cmb->GetCurSel()) {
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
	btn = (CButton *)GetDlgItem(IDC_WINDOW_CTRL);
	if (((ts.WindowFlag & WF_WINDOWCHANGE) != 0) != btn->GetCheck()) {
		ts.WindowFlag ^= WF_WINDOWCHANGE;
	}

	// (6)IDC_WINDOW_REPORT
	btn = (CButton *)GetDlgItem(IDC_WINDOW_REPORT);
	if (((ts.WindowFlag & WF_WINDOWREPORT) != 0) != btn->GetCheck()) {
		ts.WindowFlag ^= WF_WINDOWREPORT;
	}

	// (7)IDC_CURSOR_CTRL_SEQ
	btn = (CButton *)GetDlgItem(IDC_CURSOR_CTRL_SEQ);
	if (((ts.WindowFlag & WF_CURSORCHANGE) != 0) != btn->GetCheck()) {
		ts.WindowFlag ^= WF_CURSORCHANGE;
	}

	// (8)IDC_CLIPBOARD_ACCESS
	cmb = (CComboBox *)GetDlgItem(IDC_CLIPBOARD_ACCESS);
	switch (cmb->GetCurSel()) {
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
	btn = (CButton *)GetDlgItem(IDC_CLIPBOARD_NOTIFY);
	ts.NotifyClipboardAccess = btn->GetCheck();
}



// CCopypastePropPageDlg ダイアログ

IMPLEMENT_DYNAMIC(CCopypastePropPageDlg, CPropertyPage)

CCopypastePropPageDlg::CCopypastePropPageDlg()
	: CPropertyPage(CCopypastePropPageDlg::IDD)
{
}

CCopypastePropPageDlg::~CCopypastePropPageDlg()
{
	if (DlgCopypasteFont != NULL) {
		DeleteObject(DlgCopypasteFont);
	}
}

BEGIN_MESSAGE_MAP(CCopypastePropPageDlg, CPropertyPage)
END_MESSAGE_MAP()

// CCopypastePropPageDlg メッセージ ハンドラ

BOOL CCopypastePropPageDlg::OnInitDialog()
{
	char uimsg[MAX_UIMSG];
	CButton *btn, *btn2;
	CEdit *edit;
	char buf[64];

	CPropertyPage::OnInitDialog();

	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_TAHOMA_FONT", GetSafeHwnd(), &logfont, &DlgCopypasteFont, ts.UILanguageFile)) {
		SendDlgItemMessage(IDC_LINECOPY, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DISABLE_PASTE_RBUTTON, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CONFIRM_PASTE_RBUTTON, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DISABLE_PASTE_MBUTTON, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_SELECT_LBUTTON, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_TRIMNLCHAR, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_NORMALIZE_LINEBREAK, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CONFIRM_CHANGE_PASTE, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CONFIRM_STRING_FILE_LABEL, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CONFIRM_STRING_FILE, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CONFIRM_STRING_FILE_PATH, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DELIMITER, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DELIM_LIST, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PASTEDELAY_LABEL, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PASTEDELAY_EDIT, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PASTEDELAY_LABEL2, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
	}
	else {
		DlgCopypasteFont = NULL;
	}

	GetDlgItemText(IDC_LINECOPY, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_CONTINUE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_LINECOPY, ts.UIMsg);
	GetDlgItemText(IDC_DISABLE_PASTE_RBUTTON, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_MOUSEPASTE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_DISABLE_PASTE_RBUTTON, ts.UIMsg);
	GetDlgItemText(IDC_CONFIRM_PASTE_RBUTTON, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_CONFIRMPASTE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CONFIRM_PASTE_RBUTTON, ts.UIMsg);
	GetDlgItemText(IDC_DISABLE_PASTE_MBUTTON, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_MOUSEPASTEM", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_DISABLE_PASTE_MBUTTON, ts.UIMsg);
	GetDlgItemText(IDC_SELECT_LBUTTON, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_SELECTLBUTTON", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_SELECT_LBUTTON, ts.UIMsg);
	GetDlgItemText(IDC_TRIMNLCHAR, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_TRIM_TRAILING_NL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_TRIMNLCHAR, ts.UIMsg);
	GetDlgItemText(IDC_NORMALIZE_LINEBREAK, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_NORMALIZE_LINEBREAK", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_NORMALIZE_LINEBREAK, ts.UIMsg);
	GetDlgItemText(IDC_CONFIRM_CHANGE_PASTE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_CONFIRM_CHANGE_PASTE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CONFIRM_CHANGE_PASTE, ts.UIMsg);
	GetDlgItemText(IDC_CONFIRM_STRING_FILE_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_STRINGFILE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CONFIRM_STRING_FILE_LABEL, ts.UIMsg);
	GetDlgItemText(IDC_DELIMITER, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_DELIMITER", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_DELIMITER, ts.UIMsg);
	GetDlgItemText(IDC_PASTEDELAY_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_PASTEDELAY", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_PASTEDELAY_LABEL, ts.UIMsg);
	GetDlgItemText(IDC_PASTEDELAY_LABEL2, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_PASTEDELAY2", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_PASTEDELAY_LABEL2, ts.UIMsg);

	// (1)Enable continued-line copy
	btn = (CButton *)GetDlgItem(IDC_LINECOPY);
	btn->SetCheck(ts.EnableContinuedLineCopy);

	// (2)DisablePasteMouseRButton
	btn = (CButton *)GetDlgItem(IDC_DISABLE_PASTE_RBUTTON);
	btn2 = (CButton *)GetDlgItem(IDC_CONFIRM_PASTE_RBUTTON);
	if (ts.PasteFlag & CPF_DISABLE_RBUTTON) {
		btn->SetCheck(BST_CHECKED);
		btn2->EnableWindow(FALSE);
	} else {
		btn->SetCheck(BST_UNCHECKED);
		btn2->EnableWindow(TRUE);
	}

	// (3)ConfirmPasteMouseRButton
	btn2->SetCheck((ts.PasteFlag & CPF_CONFIRM_RBUTTON)?BST_CHECKED:BST_UNCHECKED);

	// (4)DisablePasteMouseMButton
	btn = (CButton *)GetDlgItem(IDC_DISABLE_PASTE_MBUTTON);
	btn->SetCheck((ts.PasteFlag & CPF_DISABLE_MBUTTON)?BST_CHECKED:BST_UNCHECKED);

	// (5)SelectOnlyByLButton
	btn = (CButton *)GetDlgItem(IDC_SELECT_LBUTTON);
	btn->SetCheck(ts.SelectOnlyByLButton);

	// (6)TrimTrailingNLonPaste
	btn = (CButton *)GetDlgItem(IDC_TRIMNLCHAR);
	btn->SetCheck((ts.PasteFlag & CPF_TRIM_TRAILING_NL)?BST_CHECKED:BST_UNCHECKED);

	// (7)NormalizeLineBreak
	btn = (CButton *)GetDlgItem(IDC_NORMALIZE_LINEBREAK);
	btn->SetCheck((ts.PasteFlag & CPF_NORMALIZE_LINEBREAK)?BST_CHECKED:BST_UNCHECKED);

	// (8)ConfirmChangePaste
	btn = (CButton *)GetDlgItem(IDC_CONFIRM_CHANGE_PASTE);
	btn->SetCheck((ts.PasteFlag & CPF_CONFIRM_CHANGEPASTE)?BST_CHECKED:BST_UNCHECKED);

	// ファイルパス
	SetDlgItemText(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile);
	edit = (CEdit *)GetDlgItem(IDC_CONFIRM_STRING_FILE);
	btn = (CButton *)GetDlgItem(IDC_CONFIRM_STRING_FILE_PATH);
	if (ts.PasteFlag & CPF_CONFIRM_CHANGEPASTE) {
		edit->EnableWindow(TRUE);
		btn->EnableWindow(TRUE);
	} else {
		edit->EnableWindow(FALSE);
		btn->EnableWindow(FALSE);
	}

	// (9)delimiter characters
	SetDlgItemText(IDC_DELIM_LIST, ts.DelimList);

	// (10)PasteDelayPerLine
	_snprintf_s(buf, sizeof(buf), "%d", ts.PasteDelayPerLine);
	SetDlgItemText(IDC_PASTEDELAY_EDIT, buf);

	// ダイアログにフォーカスを当てる
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_LINECOPY));

	return FALSE;
}

BOOL CCopypastePropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	CButton *btn, *btn2;
	CEdit *edit;
	char uimsg[MAX_UIMSG];

	switch (wParam) {
		case IDC_DISABLE_PASTE_RBUTTON | (BN_CLICKED << 16):
			btn = (CButton *)GetDlgItem(IDC_DISABLE_PASTE_RBUTTON);
			btn2 = (CButton *)GetDlgItem(IDC_CONFIRM_PASTE_RBUTTON);
			if (btn->GetCheck()) {
				btn2->EnableWindow(FALSE);
			} else {
				btn2->EnableWindow(TRUE);
			}
			return TRUE;

		case IDC_CONFIRM_CHANGE_PASTE | (BN_CLICKED << 16):
			btn2 = (CButton *)GetDlgItem(IDC_CONFIRM_CHANGE_PASTE);
			btn = (CButton *)GetDlgItem(IDC_CONFIRM_STRING_FILE_PATH);
			edit = (CEdit *)GetDlgItem(IDC_CONFIRM_STRING_FILE);
			if (btn2->GetCheck()) {
				edit->EnableWindow(TRUE);
				btn->EnableWindow(TRUE);
			} else {
				edit->EnableWindow(FALSE);
				btn->EnableWindow(FALSE);
			}
			return TRUE;

		case IDC_CONFIRM_STRING_FILE_PATH | (BN_CLICKED << 16):
			{
				OPENFILENAME ofn;

				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = get_OPENFILENAME_SIZE();
				ofn.hwndOwner = GetSafeHwnd();
				get_lang_msg("FILEDLG_SELECT_CONFIRM_STRING_APP_FILTER", ts.UIMsg, sizeof(ts.UIMsg),
				             "txt(*.txt)\\0*.txt\\0all(*.*)\\0*.*\\0\\0", ts.UILanguageFile);
				ofn.lpstrFilter = ts.UIMsg;
				ofn.lpstrFile = ts.ConfirmChangePasteStringFile;
				ofn.nMaxFile = sizeof(ts.ConfirmChangePasteStringFile);
				get_lang_msg("FILEDLG_SELECT_CONFIRM_STRING_APP_TITLE", uimsg, sizeof(uimsg),
				             "Choose a file including strings for ConfirmChangePaste", ts.UILanguageFile);
				ofn.lpstrTitle = uimsg;
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_FORCESHOWHIDDEN | OFN_HIDEREADONLY;
				if (GetOpenFileName(&ofn) != 0) {
					SetDlgItemText(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile);
				}
			}
			return TRUE;
	}

	return CPropertyPage::OnCommand(wParam, lParam);
}

void CCopypastePropPageDlg::OnOK()
{
	CButton *btn;
	char buf[64];
	int val;

	// (1)
	btn = (CButton *)GetDlgItem(IDC_LINECOPY);
	ts.EnableContinuedLineCopy = btn->GetCheck();

	// (2)
	btn = (CButton *)GetDlgItem(IDC_DISABLE_PASTE_RBUTTON);
	if (btn->GetCheck()) {
		ts.PasteFlag |= CPF_DISABLE_RBUTTON;
	}
	else {
		ts.PasteFlag &= ~CPF_DISABLE_RBUTTON;
	}

	// (3)
	btn = (CButton *)GetDlgItem(IDC_CONFIRM_PASTE_RBUTTON);
	if (btn->GetCheck()) {
		ts.PasteFlag |= CPF_CONFIRM_RBUTTON;
	}
	else {
		ts.PasteFlag &= ~CPF_CONFIRM_RBUTTON;
	}

	// (4)
	btn = (CButton *)GetDlgItem(IDC_DISABLE_PASTE_MBUTTON);
	if (btn->GetCheck()) {
		ts.PasteFlag |= CPF_DISABLE_MBUTTON;
	}
	else {
		ts.PasteFlag &= ~CPF_DISABLE_MBUTTON;
	}

	// (5)
	btn = (CButton *)GetDlgItem(IDC_SELECT_LBUTTON);
	ts.SelectOnlyByLButton = btn->GetCheck();

	// (6)
	btn = (CButton *)GetDlgItem(IDC_TRIMNLCHAR);
	if (btn->GetCheck()) {
		ts.PasteFlag |= CPF_TRIM_TRAILING_NL;
	}
	else {
		ts.PasteFlag &= ~CPF_TRIM_TRAILING_NL;
	}

	// (7)
	btn = (CButton *)GetDlgItem(IDC_NORMALIZE_LINEBREAK);
	if (btn->GetCheck()) {
		ts.PasteFlag |= CPF_NORMALIZE_LINEBREAK;
	}
	else {
		ts.PasteFlag &= ~CPF_NORMALIZE_LINEBREAK;
	}

	// (8)IDC_CONFIRM_CHANGE_PASTE
	btn = (CButton *)GetDlgItem(IDC_CONFIRM_CHANGE_PASTE);
	if (btn->GetCheck()) {
		ts.PasteFlag |= CPF_CONFIRM_CHANGEPASTE;
	}
	else {
		ts.PasteFlag &= ~CPF_CONFIRM_CHANGEPASTE;
	}
	GetDlgItemText(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile, sizeof(ts.ConfirmChangePasteStringFile));

	// (9)
	GetDlgItemText(IDC_DELIM_LIST, ts.DelimList, sizeof(ts.DelimList));

	// (10)
	GetDlgItemText(IDC_PASTEDELAY_EDIT, buf, sizeof(buf));
	val = atoi(buf);
	ts.PasteDelayPerLine = min(max(0, val), 5000);
}



// CVisualPropPageDlg ダイアログ

IMPLEMENT_DYNAMIC(CVisualPropPageDlg, CPropertyPage)

CVisualPropPageDlg::CVisualPropPageDlg()
	: CPropertyPage(CVisualPropPageDlg::IDD)
{

}

CVisualPropPageDlg::~CVisualPropPageDlg()
{
	if (DlgVisualFont != NULL) {
		DeleteObject(DlgVisualFont);
	}
}

BEGIN_MESSAGE_MAP(CVisualPropPageDlg, CPropertyPage)
END_MESSAGE_MAP()

// CVisualPropPageDlg メッセージ ハンドラ

BOOL CVisualPropPageDlg::OnInitDialog()
{
	char buf[MAXPATHLEN];
	char uimsg[MAX_UIMSG];
	CListBox *listbox;
	CButton *btn;
	CComboBox *cmb;
	int i;

	CPropertyPage::OnInitDialog();

	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_TAHOMA_FONT", GetSafeHwnd(), &logfont, &DlgVisualFont, ts.UILanguageFile)) {
		SendDlgItemMessage(IDC_ALPHABLEND, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ALPHA_BLEND, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ETERM_LOOKFEEL, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_MOUSE, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_MOUSE_CURSOR, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_FONT_QUALITY_LABEL, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_FONT_QUALITY, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ANSICOLOR, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ANSI_COLOR, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_RED, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_COLOR_RED, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_GREEN, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_COLOR_GREEN, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_BLUE, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_COLOR_BLUE, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_SAMPLE_COLOR, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ENABLE_ATTR_COLOR_BOLD, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ENABLE_ATTR_COLOR_BLINK, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ENABLE_ATTR_COLOR_REVERSE, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ENABLE_URL_COLOR, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ENABLE_ANSI_COLOR, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_URL_UNDERLINE, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_BGIMG_CHECK, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_BGIMG_EDIT, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_BGIMG_BUTTON, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_RESTART, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_BGIMG_BRIGHTNESS, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE, 0));
		SendDlgItemMessage(IDC_EDIT_BGIMG_BRIGHTNESS, WM_SETFONT, (WPARAM)DlgVisualFont, MAKELPARAM(TRUE, 0));
	}
	else {
		DlgVisualFont = NULL;
	}

	GetDlgItemText(IDC_ALPHABLEND, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_ALHPA", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ALPHABLEND, ts.UIMsg);
	GetDlgItemText(IDC_ETERM_LOOKFEEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_ETERM", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ETERM_LOOKFEEL, ts.UIMsg);
	GetDlgItemText(IDC_BGIMG_CHECK, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_BGIMG", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_BGIMG_CHECK, ts.UIMsg);
	GetDlgItemText(IDC_BGIMG_BRIGHTNESS, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_BGIMG_BRIGHTNESS", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_BGIMG_BRIGHTNESS, ts.UIMsg);
	GetDlgItemText(IDC_MOUSE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_MOUSE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_MOUSE, ts.UIMsg);
	GetDlgItemText(IDC_FONT_QUALITY_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_FONT_QUALITY", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_FONT_QUALITY_LABEL, ts.UIMsg);
	GetDlgItemText(IDC_ANSICOLOR, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_ANSICOLOR", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ANSICOLOR, ts.UIMsg);
	GetDlgItemText(IDC_RED, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_RED", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_RED, ts.UIMsg);
	GetDlgItemText(IDC_GREEN, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_GREEN", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_GREEN, ts.UIMsg);
	GetDlgItemText(IDC_BLUE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_BLUE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_BLUE, ts.UIMsg);
	GetDlgItemText(IDC_ENABLE_ATTR_COLOR_BOLD, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_BOLD", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ENABLE_ATTR_COLOR_BOLD, ts.UIMsg);
	GetDlgItemText(IDC_ENABLE_ATTR_COLOR_BLINK, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_BLINK", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ENABLE_ATTR_COLOR_BLINK, ts.UIMsg);
	GetDlgItemText(IDC_ENABLE_ATTR_COLOR_REVERSE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_REVERSE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ENABLE_ATTR_COLOR_REVERSE, ts.UIMsg);
	GetDlgItemText(IDC_ENABLE_URL_COLOR, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_URL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ENABLE_URL_COLOR, ts.UIMsg);
	GetDlgItemText(IDC_ENABLE_ANSI_COLOR, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_ANSI", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ENABLE_ANSI_COLOR, ts.UIMsg);
	GetDlgItemText(IDC_URL_UNDERLINE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_URLUL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_URL_UNDERLINE, ts.UIMsg);
	GetDlgItemText(IDC_RESTART, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_RESTART", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_RESTART, ts.UIMsg);

	get_lang_msg("DLG_TAB_VISUAL_FONT_QUALITY_DEFAULT", ts.UIMsg, sizeof(ts.UIMsg), "Default", ts.UILanguageFile);
	SendDlgItemMessage(IDC_FONT_QUALITY, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_VISUAL_FONT_QUALITY_NONANTIALIASED", ts.UIMsg, sizeof(ts.UIMsg), "Non-Antialiased", ts.UILanguageFile);
	SendDlgItemMessage(IDC_FONT_QUALITY, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_VISUAL_FONT_QUALITY_ANTIALIASED", ts.UIMsg, sizeof(ts.UIMsg), "Antialiased", ts.UILanguageFile);
	SendDlgItemMessage(IDC_FONT_QUALITY, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_VISUAL_FONT_QUALITY_CLEARTYPE", ts.UIMsg, sizeof(ts.UIMsg), "ClearType", ts.UILanguageFile);
	SendDlgItemMessage(IDC_FONT_QUALITY, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);

	// (1)AlphaBlend
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", ts.AlphaBlend);
	SetDlgItemText(IDC_ALPHA_BLEND, buf);

	// (2)[BG] BGEnable
	btn = (CButton *)GetDlgItem(IDC_ETERM_LOOKFEEL);
	btn->SetCheck(ts.EtermLookfeel.BGEnable);

	// Eterm look-feelの背景画像指定。
	SetDlgItemText(IDC_BGIMG_EDIT, ts.BGImageFilePath);

	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", ts.BGImgBrightness);
	SetDlgItemText(IDC_EDIT_BGIMG_BRIGHTNESS, buf);

	if (ts.EtermLookfeel.BGEnable) {
		GetDlgItem(IDC_BGIMG_CHECK)->EnableWindow(TRUE);

		btn = (CButton *)GetDlgItem(IDC_BGIMG_CHECK);
		if (strcmp(ts.EtermLookfeel.BGThemeFile, BG_THEME_IMAGEFILE) == 0) {
			btn->SetCheck(BST_CHECKED);
			GetDlgItem(IDC_BGIMG_EDIT)->EnableWindow(TRUE);
			GetDlgItem(IDC_BGIMG_BUTTON)->EnableWindow(TRUE);

			GetDlgItem(IDC_BGIMG_BRIGHTNESS)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS)->EnableWindow(TRUE);
		} else {
			btn->SetCheck(BST_UNCHECKED);
			GetDlgItem(IDC_BGIMG_EDIT)->EnableWindow(FALSE);
			GetDlgItem(IDC_BGIMG_BUTTON)->EnableWindow(FALSE);

			GetDlgItem(IDC_BGIMG_BRIGHTNESS)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS)->EnableWindow(FALSE);
		}
	} else {
		GetDlgItem(IDC_BGIMG_CHECK)->EnableWindow(FALSE);
		GetDlgItem(IDC_BGIMG_EDIT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BGIMG_BUTTON)->EnableWindow(FALSE);

		GetDlgItem(IDC_BGIMG_BRIGHTNESS)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS)->EnableWindow(FALSE);
	}

	// (3)Mouse cursor type
	listbox = (CListBox *)GetDlgItem(IDC_MOUSE_CURSOR);
	for (i = 0 ; MouseCursor[i].name ; i++) {
		listbox->InsertString(i, MouseCursor[i].name);
	}
	listbox->SelectString(0, ts.MouseCursorName);

	// (4)Font quality
	cmb = (CComboBox *)GetDlgItem(IDC_FONT_QUALITY);
	switch (ts.FontQuality) {
		case DEFAULT_QUALITY:
			cmb->SetCurSel(0);
			break;
		case NONANTIALIASED_QUALITY:
			cmb->SetCurSel(1);
			break;
		case ANTIALIASED_QUALITY:
			cmb->SetCurSel(2);
			break;
		default: // CLEARTYPE_QUALITY
			cmb->SetCurSel(3);
			break;
	}

	// (5)ANSI color
	listbox = (CListBox *)GetDlgItem(IDC_ANSI_COLOR);
	for (i = 0 ; i < 16 ; i++) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", i);
		listbox->InsertString(i, buf);
	}
	SetupRGBbox(GetSafeHwnd(), 0);
#if 0
	SendMessage(WM_CTLCOLORSTATIC,
	            (WPARAM)label_hdc,
	            (LPARAM)GetDlgItem(IDC_SAMPLE_COLOR));
#endif

	// (6)Bold Attr Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_BOLD);
	btn->SetCheck((ts.ColorFlag&CF_BOLDCOLOR) != 0);

	// (7)Blink Attr Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_BLINK);
	btn->SetCheck((ts.ColorFlag&CF_BLINKCOLOR) != 0);

	// (8)Reverse Attr Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_REVERSE);
	btn->SetCheck((ts.ColorFlag&CF_REVERSECOLOR) != 0);

	// (9)URL Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_URL_COLOR);
	btn->SetCheck((ts.ColorFlag&CF_URLCOLOR) != 0);

	// (10)Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ANSI_COLOR);
	btn->SetCheck((ts.ColorFlag&CF_ANSICOLOR) != 0);

	// (11)URL Underline
	btn = (CButton *)GetDlgItem(IDC_URL_UNDERLINE);
	btn->SetCheck((ts.FontFlag&FF_URLUNDERLINE) != 0);

	// ダイアログにフォーカスを当てる
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_ALPHA_BLEND));

	return FALSE;
}

BOOL CVisualPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	CListBox *listbox;
	int sel;
	char buf[MAXPATHLEN];
	CButton *btn;

	switch (wParam) {
		case IDC_ETERM_LOOKFEEL:
			// チェックされたら Enable/Disable をトグルする。
			btn = (CButton *)GetDlgItem(IDC_ETERM_LOOKFEEL);
			if (btn->GetCheck()) {
				GetDlgItem(IDC_BGIMG_CHECK)->EnableWindow(TRUE);
				btn = (CButton *)GetDlgItem(IDC_BGIMG_CHECK);
				if (btn->GetCheck()) {
					GetDlgItem(IDC_BGIMG_EDIT)->EnableWindow(TRUE);
					GetDlgItem(IDC_BGIMG_BUTTON)->EnableWindow(TRUE);

					GetDlgItem(IDC_BGIMG_BRIGHTNESS)->EnableWindow(TRUE);
					GetDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS)->EnableWindow(TRUE);
				} else {
					GetDlgItem(IDC_BGIMG_EDIT)->EnableWindow(FALSE);
					GetDlgItem(IDC_BGIMG_BUTTON)->EnableWindow(FALSE);

					GetDlgItem(IDC_BGIMG_BRIGHTNESS)->EnableWindow(FALSE);
					GetDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS)->EnableWindow(FALSE);
				}
			} else {
				GetDlgItem(IDC_BGIMG_CHECK)->EnableWindow(FALSE);
				GetDlgItem(IDC_BGIMG_EDIT)->EnableWindow(FALSE);
				GetDlgItem(IDC_BGIMG_BUTTON)->EnableWindow(FALSE);

				GetDlgItem(IDC_BGIMG_BRIGHTNESS)->EnableWindow(FALSE);
				GetDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS)->EnableWindow(FALSE);

				// 無効化されたら、BGThemeFile を元に戻す。
				strncpy_s(ts.EtermLookfeel.BGThemeFile, BG_THEME_IMAGEFILE_DEFAULT, sizeof(ts.EtermLookfeel.BGThemeFile));
				// 背景画像も無効化する。
				SetDlgItemText(IDC_BGIMG_EDIT, "");
				SetDlgItemInt(IDC_EDIT_BGIMG_BRIGHTNESS, BG_THEME_IMAGE_BRIGHTNESS_DEFAULT);
			}
			return TRUE;

		case IDC_BGIMG_CHECK:
			btn = (CButton *)GetDlgItem(IDC_BGIMG_CHECK);
			if (btn->GetCheck()) {
				GetDlgItem(IDC_BGIMG_EDIT)->EnableWindow(TRUE);
				GetDlgItem(IDC_BGIMG_BUTTON)->EnableWindow(TRUE);

				GetDlgItem(IDC_BGIMG_BRIGHTNESS)->EnableWindow(TRUE);
				GetDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS)->EnableWindow(TRUE);

				strncpy_s(ts.EtermLookfeel.BGThemeFile, BG_THEME_IMAGEFILE, sizeof(ts.EtermLookfeel.BGThemeFile));
			} else {
				GetDlgItem(IDC_BGIMG_EDIT)->EnableWindow(FALSE);
				GetDlgItem(IDC_BGIMG_BUTTON)->EnableWindow(FALSE);

				GetDlgItem(IDC_BGIMG_BRIGHTNESS)->EnableWindow(FALSE);
				GetDlgItem(IDC_EDIT_BGIMG_BRIGHTNESS)->EnableWindow(FALSE);

				// 無効化されたら、BGThemeFile を元に戻す。
				strncpy_s(ts.EtermLookfeel.BGThemeFile, BG_THEME_IMAGEFILE_DEFAULT, sizeof(ts.EtermLookfeel.BGThemeFile));
				// 背景画像も無効化する。
				SetDlgItemText(IDC_BGIMG_EDIT, "");
				SetDlgItemInt(IDC_EDIT_BGIMG_BRIGHTNESS, BG_THEME_IMAGE_BRIGHTNESS_DEFAULT);
			}
			return TRUE;

		case IDC_BGIMG_BUTTON | (BN_CLICKED << 16):
			// 背景画像をダイアログで指定する。
			{
				CString         filter("Image Files(*.jpg;*.jpeg;*.bmp)|*.jpg;*.jpeg;*.bmp|All Files(*.*)|*.*||");
				CFileDialog     selDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, filter);
				if (selDlg.DoModal() == IDOK) {
					// 背景画像指定が意図的に行われたら、BGThemeFile を固定化する。
					SetDlgItemText(IDC_BGIMG_EDIT, selDlg.GetPathName());
				}
			}
			return TRUE;

		case IDC_ANSI_COLOR | (LBN_SELCHANGE << 16):
			listbox = (CListBox *)GetDlgItem(IDC_ANSI_COLOR);
			sel = listbox->GetCurSel();
			if (sel != -1) {
				SetupRGBbox(GetSafeHwnd(), sel);
#if 0
				SendMessage(WM_CTLCOLORSTATIC,
				            (WPARAM)label_hdc,
				            (LPARAM)GetDlgItem(IDC_SAMPLE_COLOR));
#endif
			}
			return TRUE;

		case IDC_COLOR_RED | (EN_KILLFOCUS << 16):
		case IDC_COLOR_GREEN | (EN_KILLFOCUS << 16):
		case IDC_COLOR_BLUE | (EN_KILLFOCUS << 16):
			{
				BYTE r, g, b;

				listbox = (CListBox *)GetDlgItem(IDC_ANSI_COLOR);
				sel = listbox->GetCurSel();
				if (sel < 0 && sel > sizeof(ts.ANSIColor)-1) {
					return TRUE;
				}

				GetDlgItemText(IDC_COLOR_RED, buf, sizeof(buf));
				r = atoi(buf);

				GetDlgItemText(IDC_COLOR_GREEN, buf, sizeof(buf));
				g = atoi(buf);

				GetDlgItemText(IDC_COLOR_BLUE, buf, sizeof(buf));
				b = atoi(buf);

				ts.ANSIColor[sel] = RGB(r, g, b);

				// 255を超えたRGB値は補正されるので、それをEditに表示する (2007.2.18 maya)
				SetupRGBbox(GetSafeHwnd(), sel);
#if 0
				SendMessage(WM_CTLCOLORSTATIC,
				            (WPARAM)label_hdc,
				            (LPARAM)GetDlgItem(IDC_SAMPLE_COLOR));
#endif
			}

			return TRUE;
#if 0
		case WM_CTLCOLORSTATIC:
			{
				HDC hDC = (HDC)wp;
				HWND hWnd = (HWND)lp;

				//if (label_hdc == NULL) {
					hDC = GetWindowDC(GetDlgItem(hDlgWnd, IDC_SAMPLE_COLOR));
				//	label_hdc = hDC;
				//}

				if ( hWnd == GetDlgItem(hDlgWnd, IDC_SAMPLE_COLOR) ) {
					BYTE r, g, b;

					hWnd = GetDlgItem(hDlgWnd, IDC_COLOR_RED);
					SendMessage(hWnd, WM_GETTEXT , sizeof(buf), (LPARAM)buf);
					r = atoi(buf);

					hWnd = GetDlgItem(hDlgWnd, IDC_COLOR_GREEN);
					SendMessage(hWnd, WM_GETTEXT , sizeof(buf), (LPARAM)buf);
					g = atoi(buf);

					hWnd = GetDlgItem(hDlgWnd, IDC_COLOR_BLUE);
					SendMessage(hWnd, WM_GETTEXT , sizeof(buf), (LPARAM)buf);
					b = atoi(buf);

					OutputDebugPrintf("%06x\n", RGB(r, g, b));

					SetBkMode(hDC, TRANSPARENT);
					SetTextColor(hDC, RGB(r, g, b) );
					ReleaseDC(hDlgWnd, hDC);

					return (BOOL)(HBRUSH)GetStockObject(NULL_BRUSH);
				}
				return FALSE;
			}
			break ;
#endif
	}

	return CPropertyPage::OnCommand(wParam, lParam);
}

void CVisualPropPageDlg::OnOK()
{
	CListBox *listbox;
	CButton *btn;
	CComboBox *cmb;
	int sel;
	int beforeAlphaBlend;
	char buf[MAXPATHLEN];
	COLORREF TmpColor;
	int flag_changed = 0;

	// (1)
	beforeAlphaBlend = ts.AlphaBlend;
	GetDlgItemText(IDC_ALPHA_BLEND, buf, sizeof(buf));
	if (isdigit(buf[0])) {
		ts.AlphaBlend = atoi(buf);
		ts.AlphaBlend = max(0, ts.AlphaBlend);
		ts.AlphaBlend = min(255, ts.AlphaBlend);
	}

	// (2)
	// グローバル変数 BGEnable を直接書き換えると、プログラムが落ちることが
	// あるのでコピーを修正するのみとする。(2005.4.24 yutaka)
	btn = (CButton *)GetDlgItem(IDC_ETERM_LOOKFEEL);
	if (ts.EtermLookfeel.BGEnable != btn->GetCheck()) {
		flag_changed = 1;
		ts.EtermLookfeel.BGEnable = btn->GetCheck();
	}

	if (ts.EtermLookfeel.BGEnable) {
		GetDlgItemText(IDC_BGIMG_EDIT, ts.BGImageFilePath, sizeof(ts.BGImageFilePath));
	} else {
		strncpy_s(ts.BGImageFilePath, sizeof(ts.BGImageFilePath), "%SystemRoot%\\Web\\Wallpaper\\*.bmp", _TRUNCATE);
	}

	GetDlgItemText(IDC_EDIT_BGIMG_BRIGHTNESS, buf, sizeof(buf));
	if (isdigit(buf[0])) {
		ts.BGImgBrightness = atoi(buf);
		ts.BGImgBrightness = max(0, ts.BGImgBrightness);
		ts.BGImgBrightness = min(255, ts.BGImgBrightness);
	}

	// (3)
	listbox = (CListBox *)GetDlgItem(IDC_MOUSE_CURSOR);
	sel = listbox->GetCurSel();
	if (sel >= 0 && sel < MOUSE_CURSOR_MAX) {
		strncpy_s(ts.MouseCursorName, sizeof(ts.MouseCursorName), MouseCursor[sel].name, _TRUNCATE);
	}

	// (4)Font quality
	cmb = (CComboBox *)GetDlgItem(IDC_FONT_QUALITY);
	switch (cmb->GetCurSel()) {
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
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_BOLD);
	if (((ts.ColorFlag & CF_BOLDCOLOR) != 0) != btn->GetCheck()) {
		ts.ColorFlag ^= CF_BOLDCOLOR;
	}

	// (7) Attr Blink Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_BLINK);
	if (((ts.ColorFlag & CF_BLINKCOLOR) != 0) != btn->GetCheck()) {
		ts.ColorFlag ^= CF_BLINKCOLOR;
	}

	// (8) Attr Reverse Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_REVERSE);
	if (ts.ColorFlag & CF_REVERSEVIDEO) { // Reverse Videoモード(DECSCNM)時は処理を変える
		if (ts.ColorFlag & CF_REVERSECOLOR) {
			if (!btn->GetCheck()) {
				TmpColor = ts.VTColor[0];
				ts.VTColor[0] = ts.VTReverseColor[1];
				ts.VTReverseColor[1] = ts.VTColor[1];
				ts.VTColor[1] = ts.VTReverseColor[0];
				ts.VTReverseColor[0] = TmpColor;
				ts.ColorFlag ^= CF_REVERSECOLOR;
			}
		}
		else if (btn->GetCheck()) {
			TmpColor = ts.VTColor[0];
			ts.VTColor[0] = ts.VTReverseColor[0];
			ts.VTReverseColor[0] = ts.VTColor[1];
			ts.VTColor[1] = ts.VTReverseColor[1];
			ts.VTReverseColor[1] = TmpColor;
			ts.ColorFlag ^= CF_REVERSECOLOR;
		}
	}
	else if (((ts.ColorFlag & CF_REVERSECOLOR) != 0) != btn->GetCheck()) {
		ts.ColorFlag ^= CF_REVERSECOLOR;
	}

	// (9) URL Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_URL_COLOR);
	if (((ts.ColorFlag & CF_URLCOLOR) != 0) != btn->GetCheck()) {
		ts.ColorFlag ^= CF_URLCOLOR;
	}

	// (10) Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ANSI_COLOR);
	if (((ts.ColorFlag & CF_ANSICOLOR) != 0) != btn->GetCheck()) {
		ts.ColorFlag ^= CF_ANSICOLOR;
	}

	// (11) URL Underline
	btn = (CButton *)GetDlgItem(IDC_URL_UNDERLINE);
	if (((ts.FontFlag & FF_URLUNDERLINE) != 0) != btn->GetCheck()) {
		ts.FontFlag ^= FF_URLUNDERLINE;
	}

	// 2006/03/11 by 337 : Alpha値も即時変更
	// Layered窓になっていない場合は効果が無い
	if (ts.EtermLookfeel.BGUseAlphaBlendAPI) {
		// 起動時に半透明レイヤにしていない場合でも、即座に半透明となるようにする。(2006.4.1 yutaka)
		//MySetLayeredWindowAttributes(HVTWin, 0, (ts.AlphaBlend > 255) ? 255: ts.AlphaBlend, LWA_ALPHA);
		// 値が変更されたときのみ設定を反映する。(2007.10.19 maya)
		if (ts.AlphaBlend != beforeAlphaBlend) {
			SetWindowStyle(&ts);
		}
	}

	if (flag_changed) {
		// re-launch
		// RestartTeraTerm(GetSafeHwnd(), &ts);
	}
}



// CLogPropPageDlg ダイアログ

IMPLEMENT_DYNAMIC(CLogPropPageDlg, CPropertyPage)

CLogPropPageDlg::CLogPropPageDlg()
	: CPropertyPage(CLogPropPageDlg::IDD)
{

}

CLogPropPageDlg::~CLogPropPageDlg()
{
	if (DlgLogFont != NULL) {
		DeleteObject(DlgLogFont);
	}
}

BEGIN_MESSAGE_MAP(CLogPropPageDlg, CPropertyPage)
END_MESSAGE_MAP()

// CLogPropPageDlg メッセージ ハンドラ

#define LOG_ROTATE_SIZETYPE_NUM 3
static char *LogRotateSizeType[] = {
	"Byte", "KB", "MB"
};

static char *GetLogRotateSizeType(int val)
{
	if (val >= LOG_ROTATE_SIZETYPE_NUM)
		val = 0;

	return LogRotateSizeType[val];
}

BOOL CLogPropPageDlg::OnInitDialog()
{
	char uimsg[MAX_UIMSG];
	CButton *btn;
	CComboBox *combo;
	int i, TmpLogRotateSize;

	CPropertyPage::OnInitDialog();

	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_TAHOMA_FONT", GetSafeHwnd(), &logfont, &DlgLogFont, ts.UILanguageFile)) {
		SendDlgItemMessage(IDC_VIEWLOG_LABEL, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_VIEWLOG_EDITOR, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_VIEWLOG_PATH, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DEFAULTNAME_LABEL, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DEFAULTNAME_EDITOR, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DEFAULTPATH_LABEL, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DEFAULTPATH_EDITOR, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_DEFAULTPATH_PUSH, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_AUTOSTART, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));

		// Log rotate
		SendDlgItemMessage(IDC_LOG_ROTATE, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ROTATE_SIZE_TEXT, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ROTATE_SIZE, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ROTATE_SIZE_TYPE, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ROTATE_STEP_TEXT, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ROTATE_STEP, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));

		// Log options
		SendDlgItemMessage(IDC_LOG_OPTION_GROUP, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_OPT_BINARY, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_OPT_APPEND, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_OPT_PLAINTEXT, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_OPT_TIMESTAMP, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_OPT_HIDEDLG, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_OPT_INCBUF, WM_SETFONT, (WPARAM)DlgLogFont, MAKELPARAM(TRUE,0));
	}
	else {
		DlgLogFont = NULL;
	}

	GetDlgItemText(IDC_VIEWLOG_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_LOG_EDITOR", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_VIEWLOG_LABEL, ts.UIMsg);
	GetDlgItemText(IDC_DEFAULTNAME_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_LOG_FILENAME", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_DEFAULTNAME_LABEL, ts.UIMsg);
	GetDlgItemText(IDC_DEFAULTPATH_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_LOG_FILEPATH", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_DEFAULTPATH_LABEL, ts.UIMsg);
	GetDlgItemText(IDC_AUTOSTART, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_LOG_AUTOSTART", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_AUTOSTART, ts.UIMsg);
	// Log rotate
	GetDlgItemText(IDC_LOG_ROTATE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_LOG_ROTATE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_LOG_ROTATE, ts.UIMsg);
	GetDlgItemText(IDC_ROTATE_SIZE_TEXT, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_LOG_ROTATE_SIZE_TEXT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ROTATE_SIZE_TEXT, ts.UIMsg);
	GetDlgItemText(IDC_ROTATE_STEP_TEXT, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_LOG_ROTATESTEP", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ROTATE_STEP_TEXT, ts.UIMsg);
	// Log options
	// FIXME: メッセージカタログは既存のログオプションのものを流用したが、アクセラレータキーが重複するかもしれない。
	GetDlgItemText(IDC_LOG_OPTION_GROUP, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FOPT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_LOG_OPTION_GROUP, ts.UIMsg);
	GetDlgItemText(IDC_OPT_BINARY, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FOPT_BINARY", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_OPT_BINARY, ts.UIMsg);
	GetDlgItemText(IDC_OPT_APPEND, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FOPT_APPEND", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_OPT_APPEND, ts.UIMsg);
	GetDlgItemText(IDC_OPT_PLAINTEXT, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FOPT_PLAIN", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_OPT_PLAINTEXT, ts.UIMsg);
	GetDlgItemText(IDC_OPT_TIMESTAMP, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FOPT_TIMESTAMP", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_OPT_TIMESTAMP, ts.UIMsg);
	GetDlgItemText(IDC_OPT_HIDEDLG, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FOPT_HIDEDIALOG", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_OPT_HIDEDLG, ts.UIMsg);
	GetDlgItemText(IDC_OPT_INCBUF, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FOPT_ALLBUFFINFIRST", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_OPT_INCBUF, ts.UIMsg);


	// Viewlog Editor path (2005.1.29 yutaka)
	SetDlgItemText(IDC_VIEWLOG_EDITOR, ts.ViewlogEditor);

	// Log Default File Name (2006.8.28 maya)
	SetDlgItemText(IDC_DEFAULTNAME_EDITOR, ts.LogDefaultName);

	// Log Default File Path (2007.5.30 maya)
	SetDlgItemText(IDC_DEFAULTPATH_EDITOR, ts.LogDefaultPath);

	/* Auto start logging (2007.5.31 maya) */
	btn = (CButton *)GetDlgItem(IDC_AUTOSTART);
	btn->SetCheck(ts.LogAutoStart);

	// Log rotate
	btn = (CButton *)GetDlgItem(IDC_LOG_ROTATE);
	btn->SetCheck(ts.LogRotate != ROTATE_NONE);

	combo = (CComboBox *)GetDlgItem(IDC_ROTATE_SIZE_TYPE);
	for (i = 0 ; i < LOG_ROTATE_SIZETYPE_NUM ; i++) {
		combo->AddString(LogRotateSizeType[i]);
	}

	TmpLogRotateSize = ts.LogRotateSize;
	for (i = 0 ; i < ts.LogRotateSizeType ; i++)
		TmpLogRotateSize /= 1024;
	SetDlgItemInt(IDC_ROTATE_SIZE, TmpLogRotateSize, FALSE);
	combo->SelectString(-1, GetLogRotateSizeType(ts.LogRotateSizeType));
	SetDlgItemInt(IDC_ROTATE_STEP, ts.LogRotateStep, FALSE);
	if (ts.LogRotate == ROTATE_NONE) {
		GetDlgItem(IDC_ROTATE_SIZE_TEXT)->EnableWindow(FALSE);
		GetDlgItem(IDC_ROTATE_SIZE)->EnableWindow(FALSE);
		GetDlgItem(IDC_ROTATE_SIZE_TYPE)->EnableWindow(FALSE);
		GetDlgItem(IDC_ROTATE_STEP_TEXT)->EnableWindow(FALSE);
		GetDlgItem(IDC_ROTATE_STEP)->EnableWindow(FALSE);
	} else {
		GetDlgItem(IDC_ROTATE_SIZE_TEXT)->EnableWindow(TRUE);
		GetDlgItem(IDC_ROTATE_SIZE)->EnableWindow(TRUE);
		GetDlgItem(IDC_ROTATE_SIZE_TYPE)->EnableWindow(TRUE);
		GetDlgItem(IDC_ROTATE_STEP_TEXT)->EnableWindow(TRUE);
		GetDlgItem(IDC_ROTATE_STEP)->EnableWindow(TRUE);
	}

	// Log options
	btn = (CButton *)GetDlgItem(IDC_OPT_BINARY);
	btn->SetCheck(ts.LogBinary != 0);
	if (ts.LogBinary) {
		GetDlgItem(IDC_OPT_PLAINTEXT)->EnableWindow(FALSE);
		GetDlgItem(IDC_OPT_TIMESTAMP)->EnableWindow(FALSE);
	} else {
		GetDlgItem(IDC_OPT_PLAINTEXT)->EnableWindow(TRUE);
		GetDlgItem(IDC_OPT_TIMESTAMP)->EnableWindow(TRUE);
	}
	btn = (CButton *)GetDlgItem(IDC_OPT_APPEND);
	btn->SetCheck(ts.Append != 0);
	btn = (CButton *)GetDlgItem(IDC_OPT_PLAINTEXT);
	btn->SetCheck(ts.LogTypePlainText != 0);
	btn = (CButton *)GetDlgItem(IDC_OPT_TIMESTAMP);
	btn->SetCheck(ts.LogTimestamp != 0);
	btn = (CButton *)GetDlgItem(IDC_OPT_HIDEDLG);
	btn->SetCheck(ts.LogHideDialog != 0);
	btn = (CButton *)GetDlgItem(IDC_OPT_INCBUF);
	btn->SetCheck(ts.LogAllBuffIncludedInFirst != 0);


	// ダイアログにフォーカスを当てる
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_VIEWLOG_EDITOR));

	return FALSE;
}

BOOL CLogPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	char uimsg[MAX_UIMSG];
	char buf[MAX_PATH], buf2[MAX_PATH];

	switch (wParam) {
		case IDC_VIEWLOG_PATH | (BN_CLICKED << 16):
			{
				OPENFILENAME ofn;

				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = get_OPENFILENAME_SIZE();
				ofn.hwndOwner = GetSafeHwnd();
				get_lang_msg("FILEDLG_SELECT_LOGVIEW_APP_FILTER", ts.UIMsg, sizeof(ts.UIMsg),
				             "exe(*.exe)\\0*.exe\\0all(*.*)\\0*.*\\0\\0", ts.UILanguageFile);
				ofn.lpstrFilter = ts.UIMsg;
				ofn.lpstrFile = ts.ViewlogEditor;
				ofn.nMaxFile = sizeof(ts.ViewlogEditor);
				get_lang_msg("FILEDLG_SELECT_LOGVIEW_APP_TITLE", uimsg, sizeof(uimsg),
				             "Choose a executing file with launching logging file", ts.UILanguageFile);
				ofn.lpstrTitle = uimsg;
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_FORCESHOWHIDDEN | OFN_HIDEREADONLY;
				if (GetOpenFileName(&ofn) != 0) {
					SetDlgItemText(IDC_VIEWLOG_EDITOR, ts.ViewlogEditor);
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
				CButton *btn;
				btn = (CButton *)GetDlgItem(IDC_LOG_ROTATE);
				if (btn->GetCheck()) {
					GetDlgItem(IDC_ROTATE_SIZE_TEXT)->EnableWindow(TRUE);
					GetDlgItem(IDC_ROTATE_SIZE)->EnableWindow(TRUE);
					GetDlgItem(IDC_ROTATE_SIZE_TYPE)->EnableWindow(TRUE);
					GetDlgItem(IDC_ROTATE_STEP_TEXT)->EnableWindow(TRUE);
					GetDlgItem(IDC_ROTATE_STEP)->EnableWindow(TRUE);
				} else {
					GetDlgItem(IDC_ROTATE_SIZE_TEXT)->EnableWindow(FALSE);
					GetDlgItem(IDC_ROTATE_SIZE)->EnableWindow(FALSE);
					GetDlgItem(IDC_ROTATE_SIZE_TYPE)->EnableWindow(FALSE);
					GetDlgItem(IDC_ROTATE_STEP_TEXT)->EnableWindow(FALSE);
					GetDlgItem(IDC_ROTATE_STEP)->EnableWindow(FALSE);
				}

			}
			return TRUE;

		case IDC_OPT_BINARY | (BN_CLICKED << 16):
			{
				CButton *btn;
				// バイナリオプションが有効の場合、FixLogOption() で無効化している
				// オプションを、ここでも無効にしなければならない。
				btn = (CButton *)GetDlgItem(IDC_OPT_BINARY);
				if (btn->GetCheck()) {
					ts.LogBinary = 1;

					ts.LogTypePlainText = 0;
					ts.LogTimestamp = 0;
					GetDlgItem(IDC_OPT_PLAINTEXT)->EnableWindow(FALSE);
					GetDlgItem(IDC_OPT_TIMESTAMP)->EnableWindow(FALSE);
				} else {
					ts.LogBinary = 0;

					GetDlgItem(IDC_OPT_PLAINTEXT)->EnableWindow(TRUE);
					GetDlgItem(IDC_OPT_TIMESTAMP)->EnableWindow(TRUE);
				}
			}
			return TRUE;

		case IDC_OPT_APPEND | (BN_CLICKED << 16):
			{
				CButton *btn;
				btn = (CButton *)GetDlgItem(IDC_OPT_APPEND);
				if (btn->GetCheck()) {
					ts.Append = 1;
				} else {
					ts.Append = 0;
				}
			}
			return TRUE;

		case IDC_OPT_PLAINTEXT | (BN_CLICKED << 16):
			{
				CButton *btn;
				btn = (CButton *)GetDlgItem(IDC_OPT_PLAINTEXT);
				if (btn->GetCheck()) {
					ts.LogTypePlainText = 1;
				} else {
					ts.LogTypePlainText = 0;
				}
			}
			return TRUE;

		case IDC_OPT_TIMESTAMP | (BN_CLICKED << 16):
			{
				CButton *btn;
				btn = (CButton *)GetDlgItem(IDC_OPT_TIMESTAMP);
				if (btn->GetCheck()) {
					ts.LogTimestamp = 1;
				} else {
					ts.LogTimestamp = 0;
				}
			}
			return TRUE;

		case IDC_OPT_HIDEDLG | (BN_CLICKED << 16):
			{
				CButton *btn;
				btn = (CButton *)GetDlgItem(IDC_OPT_HIDEDLG);
				if (btn->GetCheck()) {
					ts.LogHideDialog = 1;
				} else {
					ts.LogHideDialog = 0;
				}
			}
			return TRUE;

		case IDC_OPT_INCBUF | (BN_CLICKED << 16):
			{
				CButton *btn;
				btn = (CButton *)GetDlgItem(IDC_OPT_INCBUF);
				if (btn->GetCheck()) {
					ts.LogAllBuffIncludedInFirst = 1;
				} else {
					ts.LogAllBuffIncludedInFirst = 0;
				}
			}
			return TRUE;
	}

	return CPropertyPage::OnCommand(wParam, lParam);
}

void CLogPropPageDlg::OnOK()
{
	char buf[80], buf2[80];
	time_t time_local;
	struct tm *tm_local;
	char uimsg[MAX_UIMSG];
	CButton *btn;
	CString str;
	int i;

	// Viewlog Editor path (2005.1.29 yutaka)
	GetDlgItemText(IDC_VIEWLOG_EDITOR, ts.ViewlogEditor, sizeof(ts.ViewlogEditor));

	// Log Default File Name (2006.8.28 maya)
	GetDlgItemText(IDC_DEFAULTNAME_EDITOR, buf, sizeof(buf));
	if (isInvalidStrftimeChar(buf)) {
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_LOGFILE_INVALID_CHAR_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Invalid character is included in log file name.", ts.UILanguageFile);
		MessageBox(ts.UIMsg, uimsg, MB_ICONEXCLAMATION);
		return;
	}
	// 現在時刻を取得
	time(&time_local);
	tm_local = localtime(&time_local);
	// 時刻文字列に変換
	if (strlen(buf) != 0 && strftime(buf2, sizeof(buf2), buf, tm_local) == 0) {
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_LOGFILE_TOOLONG_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "The log file name is too long.", ts.UILanguageFile);
		MessageBox(ts.UIMsg, uimsg, MB_ICONEXCLAMATION);
		return;
	}
	if (isInvalidFileNameChar(buf2)) {
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_LOGFILE_INVALID_CHAR_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Invalid character is included in log file name.", ts.UILanguageFile);
		MessageBox(ts.UIMsg, uimsg, MB_ICONEXCLAMATION);
		return;
	}
	strncpy_s(ts.LogDefaultName, sizeof(ts.LogDefaultName), buf, _TRUNCATE);

	// Log Default File Path (2007.5.30 maya)
	GetDlgItemText(IDC_DEFAULTPATH_EDITOR, ts.LogDefaultPath, sizeof(ts.LogDefaultPath));

	/* Auto start logging (2007.5.31 maya) */
	btn = (CButton *)GetDlgItem(IDC_AUTOSTART);
	ts.LogAutoStart = btn->GetCheck();

	/* Log Rotate */
	btn = (CButton *)GetDlgItem(IDC_LOG_ROTATE);
	if (btn->GetCheck()) {  /* on */
		ts.LogRotate = ROTATE_SIZE;

		((CComboBox*)GetDlgItem(IDC_ROTATE_SIZE_TYPE))->GetWindowText(str);
		for (i = 0 ; i < LOG_ROTATE_SIZETYPE_NUM ; i++) {
			if (strcmp(str, LogRotateSizeType[i]) == 0)
				break;
		}
		if (i >= LOG_ROTATE_SIZETYPE_NUM)
			i = 0;
		ts.LogRotateSizeType = i;

		ts.LogRotateSize = GetDlgItemInt(IDC_ROTATE_SIZE);
		for (i = 0 ; i < ts.LogRotateSizeType ; i++)
			ts.LogRotateSize *= 1024;

		ts.LogRotateStep = GetDlgItemInt(IDC_ROTATE_STEP);

	} else { /* off */
		ts.LogRotate = ROTATE_NONE;
		/* 残りのメンバーは意図的に設定を残す。*/
	}

}



// CCygwinPropPageDlg ダイアログ

IMPLEMENT_DYNAMIC(CCygwinPropPageDlg, CPropertyPage)

CCygwinPropPageDlg::CCygwinPropPageDlg()
	: CPropertyPage(CCygwinPropPageDlg::IDD)
{
}

CCygwinPropPageDlg::~CCygwinPropPageDlg()
{
	if (DlgCygwinFont != NULL) {
		DeleteObject(DlgCygwinFont);
	}
}

BEGIN_MESSAGE_MAP(CCygwinPropPageDlg, CPropertyPage)
END_MESSAGE_MAP()

// CCygwinPropPageDlg メッセージ ハンドラ

BOOL CCygwinPropPageDlg::OnInitDialog()
{
	char uimsg[MAX_UIMSG];
	CButton *btn;

	CPropertyPage::OnInitDialog();

	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_TAHOMA_FONT", GetSafeHwnd(), &logfont, &DlgCygwinFont, ts.UILanguageFile)) {
		SendDlgItemMessage(IDC_CYGWIN_PATH_LABEL, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CYGWIN_PATH, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_SELECT_FILE, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CYGWIN, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_TERM_LABEL, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_TERM_EDIT, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_TERMTYPE_LABEL, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_TERM_TYPE, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PORTSTART_LABEL, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PORT_START, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PORTRANGE_LABEL, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PORT_RANGE, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_SHELL_LABEL, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_SHELL, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ENV1_LABEL, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ENV1, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ENV2_LABEL, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ENV2, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_LOGIN_SHELL, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_HOME_CHDIR, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_AGENT_PROXY, WM_SETFONT, (WPARAM)DlgCygwinFont, MAKELPARAM(TRUE,0));
	}
	else {
		DlgCygwinFont = NULL;
	}

	GetDlgItemText(IDC_CYGWIN_PATH_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_CYGWIN_PATH", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CYGWIN_PATH_LABEL, ts.UIMsg);

	memcpy(&settings, &ts.CygtermSettings, sizeof(cygterm_t));

	SetDlgItemText(IDC_TERM_EDIT, settings.term);
	SetDlgItemText(IDC_TERM_TYPE, settings.term_type);
	SetDlgItemText(IDC_PORT_START, settings.port_start);
	SetDlgItemText(IDC_PORT_RANGE, settings.port_range);
	SetDlgItemText(IDC_SHELL, settings.shell);
	SetDlgItemText(IDC_ENV1, settings.env1);
	SetDlgItemText(IDC_ENV2, settings.env2);
	btn = (CButton *)GetDlgItem(IDC_LOGIN_SHELL);
	btn->SetCheck(settings.login_shell);
	btn = (CButton *)GetDlgItem(IDC_HOME_CHDIR);
	btn->SetCheck(settings.home_chdir);
	btn = (CButton *)GetDlgItem(IDC_AGENT_PROXY);
	btn->SetCheck(settings.agent_proxy);

	// Cygwin install path
	SetDlgItemText(IDC_CYGWIN_PATH, ts.CygwinDirectory);

	// ダイアログにフォーカスを当てる
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_CYGWIN_PATH));

	return FALSE;
}

BOOL CCygwinPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	char buf[MAX_PATH], buf2[MAX_PATH];

	switch (wParam) {
		case IDC_SELECT_FILE | (BN_CLICKED << 16):
			// Cygwin install ディレクトリの選択ダイアログ
			get_lang_msg("DIRDLG_CYGTERM_DIR_TITLE", ts.UIMsg, sizeof(ts.UIMsg),
			             "Select Cygwin directory", ts.UILanguageFile);
			GetDlgItemText(IDC_CYGWIN_PATH, buf, sizeof(buf));
			if (doSelectFolder(GetSafeHwnd(), buf2, sizeof(buf2), buf, ts.UIMsg)) {
				SetDlgItemText(IDC_CYGWIN_PATH, buf2);
			}
			return TRUE;
	}

	return CPropertyPage::OnCommand(wParam, lParam);
}

void CCygwinPropPageDlg::OnOK()
{
	CButton *btn;

	// writing to CygTerm config file
	GetDlgItemText(IDC_TERM_EDIT, settings.term, sizeof(settings.term));
	GetDlgItemText(IDC_TERM_TYPE, settings.term_type, sizeof(settings.term_type));
	GetDlgItemText(IDC_PORT_START, settings.port_start, sizeof(settings.port_start));
	GetDlgItemText(IDC_PORT_RANGE, settings.port_range, sizeof(settings.port_range));
	GetDlgItemText(IDC_SHELL, settings.shell, sizeof(settings.shell));
	GetDlgItemText(IDC_ENV1, settings.env1, sizeof(settings.env1));
	GetDlgItemText(IDC_ENV2, settings.env2, sizeof(settings.env2));
	btn = (CButton *)GetDlgItem(IDC_LOGIN_SHELL);
	settings.login_shell = btn->GetCheck();
	btn = (CButton *)GetDlgItem(IDC_HOME_CHDIR);
	settings.home_chdir = btn->GetCheck();
	btn = (CButton *)GetDlgItem(IDC_AGENT_PROXY);
	settings.agent_proxy = btn->GetCheck();

	memcpy(&ts.CygtermSettings, &settings, sizeof(cygterm_t));

	// 設定を書き込むためフラグを立てておく。
	ts.CygtermSettings.update_flag = TRUE;

	// Cygwin install path
	GetDlgItemText(IDC_CYGWIN_PATH, ts.CygwinDirectory, sizeof(ts.CygwinDirectory));
}



// CAddSettingPropSheetDlg

IMPLEMENT_DYNAMIC(CAddSettingPropSheetDlg, CPropertySheet)

CAddSettingPropSheetDlg::CAddSettingPropSheetDlg(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	AddPage(&m_GeneralPage);
	AddPage(&m_SequencePage);
	AddPage(&m_CopypastePage);
	AddPage(&m_VisualPage);
	AddPage(&m_LogPage);
	AddPage(&m_CygwinPage);

	m_psh.dwFlags |= PSH_NOAPPLYNOW;
}

CAddSettingPropSheetDlg::CAddSettingPropSheetDlg(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	AddPage(&m_GeneralPage);
	AddPage(&m_SequencePage);
	AddPage(&m_CopypastePage);
	AddPage(&m_VisualPage);
	AddPage(&m_LogPage);
	AddPage(&m_CygwinPage);

	m_psh.dwFlags |= PSH_NOAPPLYNOW;
}

CAddSettingPropSheetDlg::~CAddSettingPropSheetDlg()
{
}


BEGIN_MESSAGE_MAP(CAddSettingPropSheetDlg, CPropertySheet)
END_MESSAGE_MAP()


// CAddSettingPropSheetDlg メッセージ ハンドラ

BOOL CAddSettingPropSheetDlg::OnInitDialog()
{
	CPropertySheet::OnInitDialog();

	get_lang_msg("DLG_TABSHEET_TITLE", ts.UIMsg, sizeof(ts.UIMsg),
	             "Tera Term: Additional settings", ts.UILanguageFile);
	SetWindowText(ts.UIMsg);

	CTabCtrl *tab = GetTabControl();
	TCITEM tc;
	tc.mask = TCIF_TEXT;

	get_lang_msg("DLG_TABSHEET_TITLE_GENERAL", ts.UIMsg, sizeof(ts.UIMsg),
	             "General", ts.UILanguageFile);
	tc.pszText = ts.UIMsg;
	tab->SetItem(0, &tc);

	get_lang_msg("DLG_TABSHEET_TITLE_SEQUENCE", ts.UIMsg, sizeof(ts.UIMsg),
	             "Control Sequence", ts.UILanguageFile);
	tc.pszText = ts.UIMsg;
	tab->SetItem(1, &tc);

	get_lang_msg("DLG_TABSHEET_TITLE_COPYPASTE", ts.UIMsg, sizeof(ts.UIMsg),
	             "Copy and Paste", ts.UILanguageFile);
	tc.pszText = ts.UIMsg;
	tab->SetItem(2, &tc);

	get_lang_msg("DLG_TABSHEET_TITLE_VISUAL", ts.UIMsg, sizeof(ts.UIMsg),
	             "Visual", ts.UILanguageFile);
	tc.pszText = ts.UIMsg;
	tab->SetItem(3, &tc);

	get_lang_msg("DLG_TABSHEET_TITLE_Log", ts.UIMsg, sizeof(ts.UIMsg),
	             "Log", ts.UILanguageFile);
	tc.pszText = ts.UIMsg;
	tab->SetItem(4, &tc);

	get_lang_msg("DLG_TABSHEET_TITLE_CYGWIN", ts.UIMsg, sizeof(ts.UIMsg),
	             "Cygwin", ts.UILanguageFile);
	tc.pszText = ts.UIMsg;
	tab->SetItem(5, &tc);

	return FALSE;
}
