/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

/* TTMACRO.EXE, input dialog box */

#include "stdafx.h"
#include "teraterm.h"
#include "ttlib.h"
#include "ttmdef.h"
#include "ttm_res.h"
#include "ttmlib.h"

#include "inpdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CInpDlg dialog
CInpDlg::CInpDlg(PCHAR Input, PCHAR Text, PCHAR Title,
                 PCHAR Default, BOOL Paswd,
                 int x, int y) : CDialog(CInpDlg::IDD)
{
	//{{AFX_DATA_INIT(CInpDlg)
	//}}AFX_DATA_INIT
	InputStr = Input;
	TextStr = Text;
	TitleStr = Title;
	DefaultStr = Default;
	PaswdFlag = Paswd;
	PosX = x;
	PosY = y;
	DlgFont = NULL;
}

BEGIN_MESSAGE_MAP(CInpDlg, CDialog)
	//{{AFX_MSG_MAP(CInpDlg)
	ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CInpDlg message handler

// msgdlg のように、メッセージが長い場合にはダイアログを拡げるようにした (2006.7.29 maya)
BOOL CInpDlg::OnInitDialog()
{
	RECT R;
	HDC TmpDC;
	HWND HEdit, HOk;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font, tmpfont;

	CDialog::OnInitDialog();
	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_SYSTEM_FONT", m_hWnd, &logfont, &DlgFont, UILanguageFile)) {
		SendDlgItemMessage(IDC_INPTEXT, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_INPEDIT, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDOK, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
	}

	GetDlgItemText(IDOK, uimsg2, sizeof(uimsg2));
	get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
	SetDlgItemText(IDOK, uimsg);

	SetWindowText(TitleStr);
	SetDlgItemText(IDC_INPTEXT,TextStr);
	SetDlgItemText(IDC_INPEDIT,DefaultStr);

	TmpDC = ::GetDC(GetDlgItem(IDC_INPTEXT)->GetSafeHwnd());
	if (DlgFont) {
		tmpfont = (HFONT)SelectObject(TmpDC, DlgFont);
	}
	CalcTextExtent(TmpDC,TextStr,&s);
	if (DlgFont && tmpfont != NULL) {
		SelectObject(TmpDC, tmpfont);
	}
	::ReleaseDC(GetDlgItem(IDC_INPTEXT)->GetSafeHwnd(),TmpDC);
	TW = s.cx + s.cx/10;
	TH = s.cy;

	HEdit = ::GetDlgItem(GetSafeHwnd(), IDC_INPEDIT);
	::GetWindowRect(HEdit,&R);
	EW = R.right-R.left;
	EH = R.bottom-R.top;

	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	::GetWindowRect(HOk,&R);
	BW = R.right-R.left;
	BH = R.bottom-R.top;

	GetWindowRect(&R);
	WW = R.right-R.left;
	WH = R.bottom-R.top;

	Relocation(TRUE, WW);

	BringupWindow(this->m_hWnd);

	return TRUE;
}

void CInpDlg::OnOK()
{
	GetDlgItemText(IDC_INPEDIT,InputStr,MaxStrLen-1);
	EndDialog(IDOK);
}

LONG CInpDlg::OnExitSizeMove(UINT wParam, LONG lParam)
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

void CInpDlg::Relocation(BOOL is_init, int new_WW)
{
	RECT R;
	HDC TmpDC;
	HWND HText, HOk, HEdit;
	int CW, CH;

	GetClientRect(&R);
	CW = R.right-R.left;
	CH = R.bottom-R.top;

	// 初回のみ
	if (is_init) {
		// テキストコントロールサイズを補正
		if (TW < CW) {
			TW = CW;
		}
		if (EW < s.cx) {
			EW = s.cx;
		}
		// ウインドウサイズの計算
		WW = TW + (WW - CW);
		WH = TH + EH + BH + BH*2 + (WH - CH);
		init_WW = WW;
	}
	else {
		TW = CW;
		WW = new_WW;
	}

	HText = ::GetDlgItem(GetSafeHwnd(), IDC_INPTEXT);
	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	HEdit = ::GetDlgItem(GetSafeHwnd(), IDC_INPEDIT);

	::MoveWindow(HText,(TW-s.cx)/2,BH/2,TW,TH,TRUE);
	::MoveWindow(HEdit,(WW-EW)/2-4,TH+BH,EW,EH,TRUE);
	::MoveWindow(HOk,(TW-BW)/2,TH+EH+BH*3/2,BW,BH,TRUE);

	if (PaswdFlag) {
		SendDlgItemMessage(IDC_INPEDIT,EM_SETPASSWORDCHAR,(UINT)'*',0);
	}

	SendDlgItemMessage(IDC_INPEDIT, EM_LIMITTEXT, MaxStrLen, 0);

	if (PosX<=GetMonitorLeftmost(PosX, PosY)-100) {
		GetWindowRect(&R);
		TmpDC = ::GetDC(GetSafeHwnd());
		PosX = (GetDeviceCaps(TmpDC,HORZRES)-R.right+R.left) / 2;
		PosY = (GetDeviceCaps(TmpDC,VERTRES)-R.bottom+R.top) / 2;
		::ReleaseDC(GetSafeHwnd(),TmpDC);
	}
	SetWindowPos(&wndTop,PosX,PosY,WW,WH,0);

	InvalidateRect(NULL);
}
