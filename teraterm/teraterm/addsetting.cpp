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

void split_buffer(char *buffer, int delimiter, char **head, char **body)
{
	char *p1, *p2;

	*head = *body = NULL;

	if (!isalnum(*buffer) || (p1 = strchr(buffer, delimiter)) == NULL) {
		return;
	}

	*head = buffer;

	p2 = buffer;
	while (p2 < p1 && !isspace(*p2)) {
		p2++;
	}

	*p2 = '\0';

	p1++;
	while (*p1 && isspace(*p1)) {
		p1++;
	}

	*body = p1;
}


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
	}
	else {
		DlgSequenceFont = NULL;
	}

	GetDlgItemText(IDC_ACCEPT_MOUSE_EVENT_TRACKING, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQENCE_ACCEPT_MOUSE_EVENT_TRACKING", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ACCEPT_MOUSE_EVENT_TRACKING, ts.UIMsg);
	GetDlgItemText(IDC_DISABLE_MOUSE_TRACKING_CTRL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQENCE_DISABLE_MOUSE_TRACKING_CTRL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_DISABLE_MOUSE_TRACKING_CTRL, ts.UIMsg);
	GetDlgItemText(IDC_ACCEPT_TITLE_CHANGING_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQENCE_ACCEPT_TITLE_CHANGING", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_ACCEPT_TITLE_CHANGING_LABEL, ts.UIMsg);

	get_lang_msg("DLG_TAB_SEQENCE_ACCEPT_TITLE_CHANGING_OFF", ts.UIMsg, sizeof(ts.UIMsg), "off", ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQENCE_ACCEPT_TITLE_CHANGING_OVERWRITE", ts.UIMsg, sizeof(ts.UIMsg), "overwrite", ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQENCE_ACCEPT_TITLE_CHANGING_AHEAD", ts.UIMsg, sizeof(ts.UIMsg), "ahead", ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQENCE_ACCEPT_TITLE_CHANGING_LAST", ts.UIMsg, sizeof(ts.UIMsg), "last", ts.UILanguageFile);
	SendDlgItemMessage(IDC_ACCEPT_TITLE_CHANGING, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);

	GetDlgItemText(IDC_CURSOR_CTRL_SEQ, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQENCE_CURSOR_CTRL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CURSOR_CTRL_SEQ, ts.UIMsg);
	GetDlgItemText(IDC_WINDOW_CTRL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQENCE_WINDOW_CTRL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_WINDOW_CTRL, ts.UIMsg);
	GetDlgItemText(IDC_WINDOW_REPORT, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQENCE_WINDOW_REPORT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_WINDOW_REPORT, ts.UIMsg);
	GetDlgItemText(IDC_TITLE_REPORT_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_SEQENCE_TITLE_REPORT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_TITLE_REPORT_LABEL, ts.UIMsg);

	get_lang_msg("DLG_TAB_SEQENCE_TITLE_REPORT_IGNORE", ts.UIMsg, sizeof(ts.UIMsg), "ignore", ts.UILanguageFile);
	SendDlgItemMessage(IDC_TITLE_REPORT, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQENCE_TITLE_REPORT_ACCEPT", ts.UIMsg, sizeof(ts.UIMsg), "accept", ts.UILanguageFile);
	SendDlgItemMessage(IDC_TITLE_REPORT, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);
	get_lang_msg("DLG_TAB_SEQENCE_TITLE_REPORT_EMPTY", ts.UIMsg, sizeof(ts.UIMsg), "empty", ts.UILanguageFile);
	SendDlgItemMessage(IDC_TITLE_REPORT, CB_ADDSTRING, 0, (LPARAM)ts.UIMsg);

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

	// ダイアログにフォーカスを当てる (2004.12.7 yutaka)
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_CLICKABLE_URL));

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

	// (1)IDC_ACCEPT_MOUSE_EVENT_TRACKING
	btn = (CButton *)GetDlgItem(IDC_ACCEPT_MOUSE_EVENT_TRACKING);
	ts.MouseEventTracking = btn->GetCheck();

	// (2)IDC_DISABLE_MOUSE_TRACKING_CTRL
	btn = (CButton *)GetDlgItem(IDC_DISABLE_MOUSE_TRACKING_CTRL);
	ts.DisableMouseTrackingByCtrl = btn->GetCheck();

	// (3)IDC_ACCEPT_TITLE_CHANGING
	cmb = (CComboBox *)GetDlgItem(IDC_ACCEPT_TITLE_CHANGING);
	ts.AcceptTitleChangeRequest = cmb->GetCurSel();

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
		default: // 2
			ts.WindowFlag |= IdTitleReportEmpty;
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
		SendDlgItemMessage(IDC_CONFIRM_CHANGE_PASTE, WM_SETFONT, (WPARAM)DlgCopypasteFont, MAKELPARAM(TRUE,0));
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
	GetDlgItemText(IDC_CONFIRM_CHANGE_PASTE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_COPYPASTE_CONFIRM_CHANGE_PASTE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_CONFIRM_CHANGE_PASTE, ts.UIMsg);
	GetDlgItemText(IDC_CONFIRM_STRING_FILE_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_CONFIRM_STRING_FILE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
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
	btn->SetCheck(ts.DisablePasteMouseRButton);
	if (ts.DisablePasteMouseRButton) {
		btn2->EnableWindow(FALSE);
	} else {
		btn2->EnableWindow(TRUE);
	}

	// (3)ConfirmPasteMouseRButton
	btn2->SetCheck(ts.ConfirmPasteMouseRButton);

	// (4)DisablePasteMouseMButton
	btn = (CButton *)GetDlgItem(IDC_DISABLE_PASTE_MBUTTON);
	btn->SetCheck(ts.DisablePasteMouseMButton);

	// (5)SelectOnlyByLButton
	btn = (CButton *)GetDlgItem(IDC_SELECT_LBUTTON);
	btn->SetCheck(ts.SelectOnlyByLButton);

	// (6)ConfirmChangePaste 
	btn = (CButton *)GetDlgItem(IDC_CONFIRM_CHANGE_PASTE);
	btn->SetCheck(ts.ConfirmChangePaste);

	// ファイルパス
	SetDlgItemText(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile);
	edit = (CEdit *)GetDlgItem(IDC_CONFIRM_STRING_FILE);
	btn = (CButton *)GetDlgItem(IDC_CONFIRM_STRING_FILE_PATH);
	if (ts.ConfirmChangePaste) {
		edit->EnableWindow(TRUE);
		btn->EnableWindow(TRUE);
	} else {
		edit->EnableWindow(FALSE);
		btn->EnableWindow(FALSE);
	}

	// (7)delimiter characters
	SetDlgItemText(IDC_DELIM_LIST, ts.DelimList);

	// (8)PasteDelayPerLine
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
	ts.DisablePasteMouseRButton = btn->GetCheck();

	// (3)
	btn = (CButton *)GetDlgItem(IDC_CONFIRM_PASTE_RBUTTON);
	ts.ConfirmPasteMouseRButton = btn->GetCheck();

	// (4)
	btn = (CButton *)GetDlgItem(IDC_DISABLE_PASTE_MBUTTON);
	ts.DisablePasteMouseMButton = btn->GetCheck();

	// (5)
	btn = (CButton *)GetDlgItem(IDC_SELECT_LBUTTON);
	ts.SelectOnlyByLButton = btn->GetCheck();

	// (6)IDC_CONFIRM_CHANGE_PASTE
	btn = (CButton *)GetDlgItem(IDC_CONFIRM_CHANGE_PASTE);
	ts.ConfirmChangePaste = btn->GetCheck();
	GetDlgItemText(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile, sizeof(ts.ConfirmChangePasteStringFile));

	// (7)
	GetDlgItemText(IDC_DELIM_LIST, ts.DelimList, sizeof(ts.DelimList));

	// (8)
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
	GetDlgItemText(IDC_MOUSE, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_TAB_VISUAL_MOUSE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(IDC_MOUSE, ts.UIMsg);
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

	// (1)AlphaBlend
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", ts.AlphaBlend);
	SetDlgItemText(IDC_ALPHA_BLEND, buf);

	// (2)[BG] BGEnable
	btn = (CButton *)GetDlgItem(IDC_ETERM_LOOKFEEL);
	btn->SetCheck(ts.EtermLookfeel.BGEnable);

	// (3)Mouse cursor type
	listbox = (CListBox *)GetDlgItem(IDC_MOUSE_CURSOR);
	for (i = 0 ; MouseCursor[i].name ; i++) {
		listbox->InsertString(i, MouseCursor[i].name);
	}
	listbox->SelectString(0, ts.MouseCursorName);

	// (4)ANSI color
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

	// (5)Bold Attr Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_BOLD);
	btn->SetCheck((ts.ColorFlag&CF_BOLDCOLOR) != 0);

	// (6)Blink Attr Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_BLINK);
	btn->SetCheck((ts.ColorFlag&CF_BLINKCOLOR) != 0);

	// (7)Reverse Attr Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_REVERSE);
	btn->SetCheck((ts.ColorFlag&CF_REVERSECOLOR) != 0);

	// (8)URL Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_URL_COLOR);
	btn->SetCheck((ts.ColorFlag&CF_URLCOLOR) != 0);

	// (9)Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ANSI_COLOR);
	btn->SetCheck((ts.ColorFlag&CF_ANSICOLOR) != 0);

	// (10)URL Underline
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

	switch (wParam) {
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
	int sel;
	int beforeAlphaBlend;
	char buf[MAXPATHLEN];
	COLORREF TmpColor;

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
	ts.EtermLookfeel.BGEnable = btn->GetCheck();

	// (3)
	listbox = (CListBox *)GetDlgItem(IDC_MOUSE_CURSOR);
	sel = listbox->GetCurSel();
	if (sel >= 0 && sel < MOUSE_CURSOR_MAX) {
		strncpy_s(ts.MouseCursorName, sizeof(ts.MouseCursorName), MouseCursor[sel].name, _TRUNCATE);
	}

	// (5) Attr Bold Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_BOLD);
	if (((ts.ColorFlag & CF_BOLDCOLOR) != 0) != btn->GetCheck()) {
		ts.ColorFlag ^= CF_BOLDCOLOR;
	}

	// (6) Attr Blink Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ATTR_COLOR_BLINK);
	if (((ts.ColorFlag & CF_BLINKCOLOR) != 0) != btn->GetCheck()) {
		ts.ColorFlag ^= CF_BLINKCOLOR;
	}

	// (7) Attr Reverse Color
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

	// (8) URL Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_URL_COLOR);
	if (((ts.ColorFlag & CF_URLCOLOR) != 0) != btn->GetCheck()) {
		ts.ColorFlag ^= CF_URLCOLOR;
	}

	// (9) Color
	btn = (CButton *)GetDlgItem(IDC_ENABLE_ANSI_COLOR);
	if (((ts.ColorFlag & CF_ANSICOLOR) != 0) != btn->GetCheck()) {
		ts.ColorFlag ^= CF_ANSICOLOR;
	}

	// (10) URL Underline
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

BOOL CLogPropPageDlg::OnInitDialog()
{
	char uimsg[MAX_UIMSG];
	CButton *btn;

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

	// Viewlog Editor path (2005.1.29 yutaka)
	SetDlgItemText(IDC_VIEWLOG_EDITOR, ts.ViewlogEditor);

	// Log Default File Name (2006.8.28 maya)
	SetDlgItemText(IDC_DEFAULTNAME_EDITOR, ts.LogDefaultName);

	// Log Default File Path (2007.5.30 maya)
	SetDlgItemText(IDC_DEFAULTPATH_EDITOR, ts.LogDefaultPath);

	/* Auto start logging (2007.5.31 maya) */
	btn = (CButton *)GetDlgItem(IDC_AUTOSTART);
	btn->SetCheck(ts.LogAutoStart);

	// ダイアログにフォーカスを当てる
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_VIEWLOG_EDITOR));

	return FALSE;
}

BOOL CLogPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	char uimsg[MAX_UIMSG];

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
			doSelectFolder(GetSafeHwnd(), ts.LogDefaultPath, sizeof(ts.LogDefaultPath),
			               ts.UIMsg);
			SetDlgItemText(IDC_DEFAULTPATH_EDITOR, ts.LogDefaultPath);

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
	char *cfgfile = "cygterm.cfg"; // CygTerm configuration file
	char cfg[MAX_PATH];
	FILE *fp;
	char buf[256], *head, *body;
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

	// try to read CygTerm config file
	memset(&settings, 0, sizeof(settings));
	_snprintf_s(settings.term, sizeof(settings.term), _TRUNCATE, "ttermpro.exe %%s %%d /E /KR=SJIS /KT=SJIS /VTICON=CygTerm /nossh");
	_snprintf_s(settings.term_type, sizeof(settings.term_type), _TRUNCATE, "vt100");
	_snprintf_s(settings.port_start, sizeof(settings.port_start), _TRUNCATE, "20000");
	_snprintf_s(settings.port_range, sizeof(settings.port_range), _TRUNCATE, "40");
	_snprintf_s(settings.shell, sizeof(settings.shell), _TRUNCATE, "auto");
	_snprintf_s(settings.env1, sizeof(settings.env1), _TRUNCATE, "MAKE_MODE=unix");
	_snprintf_s(settings.env2, sizeof(settings.env2), _TRUNCATE, "");
	settings.login_shell = FALSE;
	settings.home_chdir = FALSE;
	settings.agent_proxy = FALSE;

	strncpy_s(cfg, sizeof(cfg), ts.HomeDir, _TRUNCATE);
	AppendSlash(cfg, sizeof(cfg));
	strncat_s(cfg, sizeof(cfg), cfgfile, _TRUNCATE);

	fp = fopen(cfg, "r");
	if (fp != NULL) { 
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			int len = strlen(buf);

			if (buf[len - 1] == '\n')
				buf[len - 1] = '\0';

			split_buffer(buf, '=', &head, &body);
			if (head == NULL || body == NULL)
				continue;

			if (_stricmp(head, "TERM") == 0) {
				_snprintf_s(settings.term, sizeof(settings.term), _TRUNCATE, "%s", body);

			} else if (_stricmp(head, "TERM_TYPE") == 0) {
				_snprintf_s(settings.term_type, sizeof(settings.term_type), _TRUNCATE, "%s", body);

			} else if (_stricmp(head, "PORT_START") == 0) {
				_snprintf_s(settings.port_start, sizeof(settings.port_start), _TRUNCATE, "%s", body);

			} else if (_stricmp(head, "PORT_RANGE") == 0) {
				_snprintf_s(settings.port_range, sizeof(settings.port_range), _TRUNCATE, "%s", body);

			} else if (_stricmp(head, "SHELL") == 0) {
				_snprintf_s(settings.shell, sizeof(settings.shell), _TRUNCATE, "%s", body);

			} else if (_stricmp(head, "ENV_1") == 0) {
				_snprintf_s(settings.env1, sizeof(settings.env1), _TRUNCATE, "%s", body);

			} else if (_stricmp(head, "ENV_2") == 0) {
				_snprintf_s(settings.env2, sizeof(settings.env2), _TRUNCATE, "%s", body);

			} else if (_stricmp(head, "LOGIN_SHELL") == 0) {
				if (strchr("YyTt", *body)) {
					settings.login_shell = TRUE;
				}

			} else if (_stricmp(head, "HOME_CHDIR") == 0) {
				if (strchr("YyTt", *body)) {
					settings.home_chdir = TRUE;
				}

			} else if (_stricmp(head, "SSH_AGENT_PROXY") == 0) {
				if (strchr("YyTt", *body)) {
					settings.agent_proxy = TRUE;
				}

			} else {
				// TODO: error check

			}
		}
		fclose(fp);
	}
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
	switch (wParam) {
		case IDC_SELECT_FILE | (BN_CLICKED << 16):
			// Cygwin install ディレクトリの選択ダイアログ
			get_lang_msg("DIRDLG_CYGTERM_DIR_TITLE", ts.UIMsg, sizeof(ts.UIMsg),
			             "Select Cygwin directory", ts.UILanguageFile);
			doSelectFolder(GetSafeHwnd(), ts.CygwinDirectory, sizeof(ts.CygwinDirectory),
			               ts.UIMsg);
			// Cygwin install path
			SetDlgItemText(IDC_CYGWIN_PATH, ts.CygwinDirectory);
			return TRUE;
	}

	return CPropertyPage::OnCommand(wParam, lParam);
}

void CCygwinPropPageDlg::OnOK()
{
	char *cfgfile = "cygterm.cfg"; // CygTerm configuration file
	char *tmpfile = "cygterm.tmp";
	char cfg[MAX_PATH];
	char tmp[MAX_PATH];
	FILE *fp;
	FILE *tmp_fp;
	char buf[256], *head, *body;
	char uimsg[MAX_UIMSG];
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

	strncpy_s(cfg, sizeof(cfg), ts.HomeDir, _TRUNCATE);
	AppendSlash(cfg, sizeof(cfg));
	strncat_s(cfg, sizeof(cfg), cfgfile, _TRUNCATE);

	strncpy_s(tmp, sizeof(tmp), ts.HomeDir, _TRUNCATE);
	AppendSlash(tmp, sizeof(tmp));
	strncat_s(tmp, sizeof(tmp), tmpfile, _TRUNCATE);

	fp = fopen(cfg, "r");
	tmp_fp = fopen(tmp, "w");
	if (tmp_fp == NULL) { 
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_CYGTERM_CONF_WRITEFILE_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Can't write CygTerm configuration file (%d).", ts.UILanguageFile);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts.UIMsg, GetLastError());
		MessageBox(buf, uimsg, MB_ICONEXCLAMATION);
	} else {
		if (fp != NULL) {
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				int len = strlen(buf);

				if (buf[len - 1] == '\n')
					buf[len - 1] = '\0';

				split_buffer(buf, '=', &head, &body);
				if (head == NULL || body == NULL) {
					fprintf(tmp_fp, "%s\n", buf);
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
			fclose(fp);
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

		if (remove(cfg) != 0 && errno != ENOENT) {
			get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
			get_lang_msg("MSG_CYGTERM_CONF_REMOVEFILE_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
			             "Can't remove old CygTerm configuration file (%d).", ts.UILanguageFile);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts.UIMsg, GetLastError());
			MessageBox(buf, uimsg, MB_ICONEXCLAMATION);
		}
		else if (rename(tmp, cfg) != 0) {
			get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
			get_lang_msg("MSG_CYGTERM_CONF_RENAMEFILE_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
			             "Can't rename CygTerm configuration file (%d).", ts.UILanguageFile);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts.UIMsg, GetLastError());
			MessageBox(buf, uimsg, MB_ICONEXCLAMATION);
		}
	}

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
