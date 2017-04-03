/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, status dialog box */

#include "stdafx.h"
#include "teraterm.h"
#include "ttlib.h"
#include "ttm_res.h"
#include "ttmlib.h"

#include "statdlg.h"
#include "tttypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CStatDlg dialog

BEGIN_MESSAGE_MAP(CStatDlg, CDialog)
	//{{AFX_MSG_MAP(CStatDlg)
	ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
	ON_MESSAGE(WM_USER_MSTATBRINGUP, OnSetForceForegroundWindow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CStatDlg::Create(PCHAR Text, PCHAR Title, int x, int y)
{
	TextStr = Text;
	TitleStr = Title;
	PosX = x;
	PosY = y;
	DlgFont = NULL;
	return CDialog::Create(CStatDlg::IDD, GetDesktopWindow());
}

void CStatDlg::Update(PCHAR Text, PCHAR Title, int x, int y)
{
	RECT R;
	HDC TmpDC;
	HFONT tmpfont;

	if (Title!=NULL) {
		SetWindowText(Title);
		TitleStr = Title;
	}

	GetWindowRect(&R);
	PosX = R.left;
	PosY = R.top;
	WW = R.right-R.left;
	WH = R.bottom-R.top;

	if (Text!=NULL) {
		TmpDC = ::GetDC(GetDlgItem(IDC_STATTEXT)->GetSafeHwnd());
		if (DlgFont) {
			tmpfont = (HFONT)SelectObject(TmpDC, DlgFont);
		}
		CalcTextExtent(TmpDC,Text,&s);
		if (DlgFont && tmpfont != NULL) {
			SelectObject(TmpDC, tmpfont);
		}
		::ReleaseDC(GetDlgItem(IDC_STATTEXT)->GetSafeHwnd(),TmpDC);
		TW = s.cx + s.cx/10;
		TH = s.cy;

		SetDlgItemText(IDC_STATTEXT,Text);
		TextStr = Text;
	}

	if (x!=32767) {
		PosX = x;
		PosY = y;
	}

	Relocation(TRUE, WW);
}

// CStatDlg message handler

BOOL CStatDlg::OnInitDialog()
{
	LOGFONT logfont;
	HFONT font;

	CDialog::OnInitDialog();
	Update(TextStr,TitleStr,PosX,PosY);
	SetForegroundWindow();

	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_SYSTEM_FONT", m_hWnd, &logfont, &DlgFont, UILanguageFile)) {
		SendDlgItemMessage(IDC_STATTEXT, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
	}

	return TRUE;
}

void CStatDlg::OnCancel()
{
	DestroyWindow();
}

BOOL CStatDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) {
		case IDOK:  // Enter key押下で消えないようにする。(2010.8.25 yutaka)
			return TRUE;
		case IDCANCEL:
			if ((HWND)lParam!=NULL) { // ignore ESC key
				DestroyWindow();
			}
			return TRUE;
		default:
		return (CDialog::OnCommand(wParam,lParam));
	}
}

void CStatDlg::PostNcDestroy()
{
	// statusboxとclosesboxを繰り返すと、GDIリソースリークとなる問題を修正した。
	//   - CreateFontIndirect()で作成した論理フォントを削除する。
	// (2016.10.5 yutaka)
	if (DlgFont) {
		DeleteObject(DlgFont);
		DlgFont = NULL;
	}

	delete this;
}

LONG CStatDlg::OnExitSizeMove(UINT wParam, LONG lParam)
{
	RECT R;

	GetWindowRect(&R);
	if (R.bottom-R.top == WH && R.right-R.left == WW) {
		// サイズが変わっていなければ何もしない
	}
	else if (R.bottom-R.top != WH || R.right-R.left < init_WW) {
		// 高さが変更されたか、最初より幅が狭くなった場合は元に戻す
		SetWindowPos(&wndTop,R.left,R.top,WW,WH,0);
	}
	else {
		// そうでなければ再配置する
		Relocation(FALSE, R.right-R.left);
	}

	return CDialog::DefWindowProc(WM_EXITSIZEMOVE,wParam,lParam);
}

LONG CStatDlg::OnSetForceForegroundWindow(UINT wParam, LONG lParam)
{
	DWORD pid;
	DWORD targetid;
	DWORD currentActiveThreadId;
	HWND hwnd = (HWND)wParam;

	targetid = GetWindowThreadProcessId(hwnd, &pid);
	currentActiveThreadId = GetWindowThreadProcessId(::GetForegroundWindow(), &pid);

	SetForegroundWindow();
	if (targetid == currentActiveThreadId) {
		BringWindowToTop();
	} else {
		AttachThreadInput(targetid, currentActiveThreadId, TRUE);
		BringWindowToTop();
		AttachThreadInput(targetid, currentActiveThreadId, FALSE);
	}

	return TRUE;
}

void CStatDlg::Relocation(BOOL is_init, int new_WW)
{
	RECT R;
	HDC TmpDC;
	HWND HText;
	int CW, CH;

	if (TextStr != NULL) {
		HText = ::GetDlgItem(GetSafeHwnd(), IDC_STATTEXT);

		GetClientRect(&R);
		CW = R.right-R.left;
		CH = R.bottom-R.top;

		// 初回のみ
		if (is_init) {
			// テキストコントロールサイズを補正
			if (TW < CW) {
				TW = CW;
			}
			// ウインドウサイズの計算
			WW = TW + (WW - CW);
			WH = TH + 10 + (WH - CH);
			init_WW = WW;
		}
		else {
			TW = CW;
			WW = new_WW;
		}

		::MoveWindow(HText,(TW-s.cx)/2,5,TW,TH,TRUE);
	}

	if (PosX<=GetMonitorLeftmost(PosX, PosY)-100) {
		TmpDC = ::GetDC(GetSafeHwnd());
		PosX = (GetDeviceCaps(TmpDC,HORZRES)-WW) / 2;
		PosY = (GetDeviceCaps(TmpDC,VERTRES)-WH) / 2;
		::ReleaseDC(GetSafeHwnd(),TmpDC);
	}
	SetWindowPos(&wndTop,PosX,PosY,WW,WH,SWP_NOZORDER);

	InvalidateRect(NULL);
}

void CStatDlg::Bringup()
{
	BringupWindow(this->m_hWnd);
}

BOOL CStatDlg::CheckAutoCenter()
{
	// CenterWindow() is called when x=0 && y=0
	// Don't call CenterWindow()
	return FALSE;
}
