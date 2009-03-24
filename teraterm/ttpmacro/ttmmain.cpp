/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, main */

#include "stdafx.h"
#include "teraterm.h"
#include "ttm_res.h"
#include "ttmdlg.h"
#include "ttl.h"
#include "ttmparse.h"
#include "ttmdde.h"

#include "ttmmain.h"
#include "ttmbuff.h"
#include "ttmlib.h"

#include "ttlib.h"

#include "wait4all.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CCtrlWindow dialog
CCtrlWindow::CCtrlWindow()
	: CDialog()
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_TTMACRO);
}

BOOL CCtrlWindow::Create()
{
	if (! CDialog::Create(CCtrlWindow::IDD, NULL)) {
		PostQuitMessage(0);
		return FALSE;
	}
	return TRUE;
}

// TTMACRO main engine
BOOL CCtrlWindow::OnIdle()
{
	int ResultCode;
	char Temp[2];

	if (TTLStatus==IdTTLEnd) {
		DestroyWindow();
		return FALSE;
	}

	SendSync(); // for sync mode

	if (OutLen>0) {
		DDESend();

		// Windows2000/XP（シングルプロセッサ環境）においてマクロ実行中にCPU使用率が100%になってしまう
		// 現象への暫定処置。ttermpro.exeへのDDE送信後に無条件に100ミリ秒のスリープを追加。
		// (2005.5.25 yutaka)
		// 以下の暫定処置は解除。
		// (2006.10.13 yutaka)
		// 以下の暫定処置は復活。Tera Term本体側のOnIdle()において、XTYP_ADVREQが頻繁に飛ぶことが
		// CPUをストールさせてしまう要因と思われるのだが、根本的原因はいまだ不明。
		// (2006.11.4 yutaka)
		// 以下の暫定処置は DDESend() で行うため、削除。
		// (2006.11.6 yutaka)
#if 0
		Sleep(100);
#endif
		return TRUE;
	}
	else if (! Pause && (TTLStatus==IdTTLRun)) {
		Exec();

		// 更新対象のマクロコマンドの場合のみ、ウィンドウに更新指示を出す。
		// 毎度 WM_PAINT を送っているとマクロの動作が遅くなるため。(2006.2.24 yutaka)
		if (IsUpdateMacroCommand()) {
			Invalidate(TRUE);
		}
		return TRUE;
	}
	else if (TTLStatus==IdTTLWait) {
		ResultCode = Wait();
		if (ResultCode>0) {
			KillTimer(IdTimeOutTimer);
			TTLStatus = IdTTLRun;
			LockVar();
			SetResult(ResultCode);
			UnlockVar();
			ClearWait();
			return TRUE;
		}
		else if (ComReady==0) {
			SetTimer(IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLWaitLn) {
		ResultCode = Wait();
		if (ResultCode>0) {
			LockVar();
			SetResult(ResultCode);
			UnlockVar();
			Temp[0] = 0x0a;
			Temp[1] = 0;
			if (CmpWait(ResultCode,Temp)==0) { // new-line is received
				KillTimer(IdTimeOutTimer);
				ClearWait();
				TTLStatus = IdTTLRun;
				LockVar();
				SetInputStr(GetRecvLnBuff());
				UnlockVar();
			}
			else { // wait new-line
				ClearWait();
				SetWait(1,Temp);
				TTLStatus = IdTTLWaitNL;
			}
			return TRUE;
		}
		else if (ComReady==0) {
			SetTimer(IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLWaitNL) {
		ResultCode = Wait();
		if (ResultCode>0) {
			KillTimer(IdTimeOutTimer);
			TTLStatus = IdTTLRun;
			LockVar();
			SetInputStr(GetRecvLnBuff());
			UnlockVar();
			return TRUE;
		}
		else if (ComReady==0) {
			SetTimer(IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLWait2) {
		if (Wait2()) {
			KillTimer(IdTimeOutTimer);
			TTLStatus = IdTTLRun;
			LockVar();
			SetInputStr(Wait2Str);
			SetResult(1);
			UnlockVar();
			return TRUE;
		}
		else if (ComReady==0) {
			SetTimer(IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLWaitN) {
		if (WaitN()) {
			KillTimer(IdTimeOutTimer);
			TTLStatus = IdTTLRun;
			LockVar();
			SetResult(1);
			SetInputStr(GetRecvLnBuff());
			UnlockVar();
			ClearWaitN();
			return TRUE;
		}
		else if (ComReady==0) {
			SetTimer(IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLWait4all) {
		ResultCode = Wait4all();
		if (ResultCode>0) {
			KillTimer(IdTimeOutTimer);
			TTLStatus = IdTTLRun;
			LockVar();
			SetResult(ResultCode);
			UnlockVar();
			ClearWait();
			return TRUE;
		}
		else if (ComReady==0) {
			SetTimer(IdTimeOutTimer,0, NULL);
		}
	}

	return FALSE;
}

BEGIN_MESSAGE_MAP(CCtrlWindow, CDialog)
	//{{AFX_MSG_MAP(CCtrlWindow)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_TIMER()
	ON_MESSAGE(WM_USER_DDECMNDEND,OnDdeCmndEnd)
	ON_MESSAGE(WM_USER_DDECOMREADY,OnDdeComReady)
	ON_MESSAGE(WM_USER_DDEREADY,OnDdeReady)
	ON_MESSAGE(WM_USER_DDEEND,OnDdeEnd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CCtrlWindow message handler

BOOL CCtrlWindow::OnInitDialog()
{
	HDC TmpDC;
	int CRTWidth, CRTHeight;
	RECT Rect;
	char Temp[MAXPATHLEN];
	BOOL IOption, VOption;
	int CmdShow;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;
	int fuLoad = LR_DEFAULTCOLOR;

	CDialog::OnInitDialog();

	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_SYSTEM_FONT", m_hWnd, &logfont, &DlgFont, UILanguageFile)) {
		SendDlgItemMessage(IDC_CTRLPAUSESTART, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_CTRLEND, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
	}

	GetDlgItemText(IDC_CTRLPAUSESTART, uimsg2, sizeof(uimsg2));
	get_lang_msg("BTN_PAUSE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
	SetDlgItemText(IDC_CTRLPAUSESTART, uimsg);
	GetDlgItemText(IDC_CTRLEND, uimsg2, sizeof(uimsg2));
	get_lang_msg("BTN_END", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
	SetDlgItemText(IDC_CTRLEND, uimsg);

	Pause = FALSE;

	TmpDC = ::GetDC(GetSafeHwnd());
	CRTWidth = GetDeviceCaps(TmpDC,HORZRES);
	CRTHeight = GetDeviceCaps(TmpDC,VERTRES);
	GetWindowRect(&Rect);
	::ReleaseDC(GetSafeHwnd(), TmpDC);
	::SetWindowPos(GetSafeHwnd(),HWND_TOP,
	               (CRTWidth-Rect.right+Rect.left) / 2,
	               (CRTHeight-Rect.bottom+Rect.top) / 2,
	               0,0,SWP_NOSIZE | SWP_NOZORDER);

	if (is_NT4()) {
		fuLoad = LR_VGACOLOR;
	}
	::PostMessage(GetSafeHwnd(),WM_SETICON,ICON_SMALL,
	              (LPARAM)LoadImage(AfxGetInstanceHandle(),
	                                MAKEINTRESOURCE(IDI_TTMACRO),
	                                IMAGE_ICON,16,16,fuLoad));
	::PostMessage(GetSafeHwnd(),WM_SETICON,ICON_BIG,
	              (LPARAM)LoadImage(AfxGetInstanceHandle(),
	                                MAKEINTRESOURCE(IDI_TTMACRO),
	                                IMAGE_ICON,0,0,fuLoad));

	ParseParam(&IOption,&VOption);

	if (TopicName[0] != 0) {
		InitDDE(GetSafeHwnd());
	}

	if ((FileName[0]==0) && (! GetFileName(GetSafeHwnd()))) {
		EndDDE();
		PostQuitMessage(0);
		return TRUE;
	}

	if (! InitTTL(GetSafeHwnd())) {
		EndDDE();
		PostQuitMessage(0);
	}

	// wait4all
	register_macro_window(GetSafeHwnd());

	strncpy_s(Temp, sizeof(Temp), "MACRO - ", _TRUNCATE);
	strncat_s(Temp, sizeof(Temp), ShortName, _TRUNCATE);
	SetWindowText(Temp);

	// send the initialization signal to TT
	SendCmnd(CmdInit,0);

	if (VOption) {
		return TRUE;
	}
	if (IOption) {
		CmdShow = SW_SHOWMINIMIZED;
	}
	else {
		CmdShow = SW_SHOWDEFAULT;
	}
	ShowWindow(CmdShow);
	return TRUE;
}

void CCtrlWindow::OnCancel( )
{
	DestroyWindow();
}

BOOL CCtrlWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	char uimsg[MAX_UIMSG];

	switch (LOWORD(wParam)) {
		case IDC_CTRLPAUSESTART:
			if (Pause) {
				get_lang_msg("BTN_PAUSE", uimsg, sizeof(uimsg),  "Pau&se", UILanguageFile);
				SetDlgItemText(IDC_CTRLPAUSESTART, uimsg);
			}
			else {
				get_lang_msg("BTN_START", uimsg, sizeof(uimsg),  "&Start", UILanguageFile);
				SetDlgItemText(IDC_CTRLPAUSESTART, uimsg);
			}
			Pause = ! Pause;
			return TRUE;
		case IDC_CTRLEND:
			TTLStatus = IdTTLEnd;
			return TRUE;
		default:
			return (CDialog::OnCommand(wParam,lParam));
	}
}

void CCtrlWindow::OnClose()
{
	EndTTL();
	EndDDE();
	CDialog::OnClose();
}

void CCtrlWindow::OnDestroy()
{
	unregister_macro_window(GetSafeHwnd());

	EndTTL();
	EndDDE();
	CDialog::OnDestroy();
}

// for icon drawing in Win NT 3.5
BOOL CCtrlWindow::OnEraseBkgnd(CDC* pDC)
{
	if (IsIconic()) {
		return TRUE;
	}
	else {
		return CDialog::OnEraseBkgnd(pDC);
	}
}

// for icon drawing in Win NT 3.5
void CCtrlWindow::OnPaint()
{
	int OldMapMode;
	CPaintDC dc(this);
	char buf[128];

	// line number (2005.7.18 yutaka)
	// added line buffer (2005.7.22 yutaka)
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d:%s", GetLineNo(), GetLineBuffer());
	SetDlgItemText(IDC_LINENO, buf);

	OldMapMode = dc.GetMapMode();
	dc.SetMapMode(MM_TEXT);

	if (!IsIconic()) return;
	SendMessage(WM_ICONERASEBKGND,(UINT)(dc.m_hDC));
	dc.DrawIcon(0, 0, m_hIcon);
	dc.SetMapMode(OldMapMode);
}

// for icon drawing in Win NT 3.5
HCURSOR CCtrlWindow::OnQueryDragIcon()
{
	return m_hIcon;
}

void CCtrlWindow::OnSysColorChange()
{
	CDialog::OnSysColorChange();
}

void CCtrlWindow::OnTimer(UINT nIDEvent)
{
	BOOL TimeOut;

	KillTimer(nIDEvent);
	if (nIDEvent!=IdTimeOutTimer) {
		return;
	}
	if (TTLStatus==IdTTLRun) {
		return;
	}

	TimeOut = CheckTimeout();
	LockVar();

	if ((TTLStatus == IdTTLWait) ||
	    (TTLStatus == IdTTLWaitLn) ||
	    (TTLStatus == IdTTLWaitNL)) {
		if ((! Linked) || (ComReady==0)|| TimeOut) {
			SetResult(0);
			if (TTLStatus==IdTTLWaitNL) {
				SetInputStr(GetRecvLnBuff());
			}
			ClearWait();
			TTLStatus = IdTTLRun;
		}
	}
	else if (TTLStatus == IdTTLWait2) {
		if ((! Linked) || (ComReady==0) || TimeOut) {
			if (Wait2Found) {
				SetInputStr(Wait2Str);
				SetResult(-1);
			}
			else {
				SetInputStr("");
				SetResult(0);
			}
			TTLStatus = IdTTLRun;
		}
	}
	else if (TTLStatus == IdTTLWaitN) {
		if ((! Linked) || (ComReady==0) || TimeOut) {
			PCHAR p = GetRecvLnBuff();
			SetInputStr(p);
			if (strlen(p) > 0) {
				SetResult(-1);
			}
			else {
				SetResult(0);
			}
			TTLStatus = IdTTLRun;
		}
	}
	else if (TTLStatus==IdTTLPause) {
		if (TimeOut) {
			TTLStatus = IdTTLRun;
		}
	}
	else if (TTLStatus==IdTTLSleep) {
		if ((TimeOut) && TestWakeup(IdWakeupTimeout)) {
			SetResult(IdWakeupTimeout);
			TTLStatus = IdTTLRun;
		}
	}
	else {
		TTLStatus = IdTTLRun;
	}

	UnlockVar();

	if (TimeOut || (TTLStatus==IdTTLRun)) {
		return;
	}

	SetTimer(IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
}

void CCtrlWindow::PostNcDestroy()
{
	delete this;
	PostQuitMessage(0);
}

BOOL CCtrlWindow::PreTranslateMessage(MSG* pMsg)
{
	if ((pMsg->message==WM_KEYDOWN) && (pMsg->wParam==VK_ESCAPE)) { // ignore ESC key
		return FALSE;
	}
	return CDialog::PreTranslateMessage(pMsg);
}

LONG CCtrlWindow::OnDdeCmndEnd(UINT wParam, LONG lParam)
{
	if (TTLStatus == IdTTLWaitCmndResult) {
		LockVar();
		SetResult(wParam);
		UnlockVar();
	}

	if ((TTLStatus == IdTTLWaitCmndEnd) ||
		(TTLStatus == IdTTLWaitCmndResult)) {
		TTLStatus = IdTTLRun;
	}
	return 0;
}

LONG CCtrlWindow::OnDdeComReady(UINT wParam, LONG lParam)
{
	ComReady = wParam;
	if ((TTLStatus == IdTTLWait) ||
	    (TTLStatus == IdTTLWaitLn) ||
	    (TTLStatus == IdTTLWaitNL) ||
	    (TTLStatus == IdTTLWait2) ||
	    (TTLStatus == IdTTLWaitN)) {
		if (ComReady==0) {
			SetTimer(IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLSleep) {
		LockVar();
		if (TestWakeup(IdWakeupInit)) {
			if (ComReady!=0) {
				SetResult(2);
			}
			else {
				SetResult(1);
			}
			TTLStatus = IdTTLRun;
		}
		else if ((ComReady!=0) && TestWakeup(IdWakeupConnect)) {
			SetResult(IdWakeupConnect);
			TTLStatus = IdTTLRun;
		}
		else if ((ComReady==0) && TestWakeup(IdWakeupDisconn)) {
			SetResult(IdWakeupDisconn);
			TTLStatus = IdTTLRun;
		}
		UnlockVar();
	}
	return 0;
}

LONG CCtrlWindow::OnDdeReady(UINT wParam, LONG lParam)
{
	if (TTLStatus != IdTTLInitDDE) {
		return 0;
	}
	SetWakeup(IdWakeupInit);
	TTLStatus = IdTTLSleep;

	if (! InitDDE(GetSafeHwnd())) {
		LockVar();
		SetResult(0);
		UnlockVar();
		TTLStatus = IdTTLRun;
	}
	return 0;
}

LONG CCtrlWindow::OnDdeEnd(UINT wParam, LONG lParam)
{
	EndDDE();
	if ((TTLStatus == IdTTLWaitCmndEnd) ||
		(TTLStatus == IdTTLWaitCmndResult)) {
		TTLStatus = IdTTLRun;
	}
	else if ((TTLStatus == IdTTLWait) ||
	         (TTLStatus == IdTTLWaitLn) ||
	         (TTLStatus == IdTTLWaitNL) ||
	         (TTLStatus == IdTTLWait2) ||
	         (TTLStatus == IdTTLWaitN)) {
		SetTimer(IdTimeOutTimer,0, NULL);
	}
	else if (TTLStatus==IdTTLSleep) {
		LockVar();
		if (TestWakeup(IdWakeupInit)) {
			SetResult(0);
			TTLStatus = IdTTLRun;
		}
		else if (TestWakeup(IdWakeupUnlink)) {
			SetResult(IdWakeupUnlink);
			TTLStatus = IdTTLRun;
		}
		UnlockVar();
	}
	return 0;
}
