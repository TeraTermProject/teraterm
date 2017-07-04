/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2017 TeraTerm Project
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

/* TTMACRO.EXE, message dialog box */

#include "stdafx.h"
#include "teraterm.h"
#include "ttlib.h"
#include "ttm_res.h"
#include "ttmlib.h"

#include "msgdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CMsgDlg dialog

CMsgDlg::CMsgDlg(PCHAR Text, PCHAR Title, BOOL YesNo,
                 int x, int y) : CDialog(CMsgDlg::IDD)
{
	//{{AFX_DATA_INIT(CMsgDlg)
	//}}AFX_DATA_INIT
	TextStr = Text;
	TitleStr = Title;
	YesNoFlag = YesNo;
	PosX = x;
	PosY = y;
	DlgFont = NULL;
}

BEGIN_MESSAGE_MAP(CMsgDlg, CDialog)
	//{{AFX_MSG_MAP(CMsgDlg)
	ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CMsgDlg message handler

BOOL CMsgDlg::OnInitDialog()
{
	RECT R;
	HDC TmpDC;
	HWND HOk, HNo;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font, tmpfont;

	CDialog::OnInitDialog();
	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_SYSTEM_FONT", m_hWnd, &logfont, &DlgFont, UILanguageFile)) {
		SendDlgItemMessage(IDC_MSGTEXT, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDOK, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDCLOSE, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
	}

	GetDlgItemText(IDOK, uimsg2, sizeof(uimsg2));
	get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
	SetDlgItemText(IDOK, uimsg);

	SetWindowText(TitleStr);
	SetDlgItemText(IDC_MSGTEXT,TextStr);

	TmpDC = ::GetDC(GetDlgItem(IDC_MSGTEXT)->GetSafeHwnd());
	if (DlgFont) {
		tmpfont = (HFONT)SelectObject(TmpDC, DlgFont);
	}
	CalcTextExtent(TmpDC,TextStr,&s);
	if (DlgFont && tmpfont != NULL) {
		SelectObject(TmpDC, tmpfont);
	}
	::ReleaseDC(GetDlgItem(IDC_MSGTEXT)->GetSafeHwnd(),TmpDC);
	TW = s.cx + s.cx/10;
	TH = s.cy;

	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	HNo = ::GetDlgItem(GetSafeHwnd(), IDCLOSE);
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

LONG CMsgDlg::OnExitSizeMove(UINT wParam, LONG lParam)
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

void CMsgDlg::Relocation(BOOL is_init, int new_WW)
{
	RECT R;
	HDC TmpDC;
	HWND HText, HOk, HNo;
	int CW, CH;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];

	GetClientRect(&R);
	CW = R.right-R.left;
	CH = R.bottom-R.top;

	// 初回のみ
	if (is_init) {
		// テキストコントロールサイズを補正
		if (TW < CW) {
			TW = CW;
		}
		if (YesNoFlag && (TW < 7*BW/2)) {
			TW = 7*BW/2;
		}
		// ウインドウサイズの計算
		WW = TW + (WW - CW);
		WH = TH + BH + BH*3/2 + (WH - CH);
		init_WW = WW;
	}
	else {
		TW = CW;
		WW = new_WW;
	}

	HText = ::GetDlgItem(GetSafeHwnd(), IDC_MSGTEXT);
	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	HNo = ::GetDlgItem(GetSafeHwnd(), IDCLOSE);

	::MoveWindow(HText,(TW-s.cx)/2, BH/2,TW,TH,TRUE);
	if (YesNoFlag) {
		if (is_init) {
			::SetWindowText(HOk,"&Yes");
			::SetWindowText(HNo,"&No");
			GetDlgItemText(IDOK, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_YES", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			::SetWindowText(HOk,uimsg);
			GetDlgItemText(IDCLOSE, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_NO", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			::SetWindowText(HNo,uimsg);
		}
		::MoveWindow(HOk,(2*TW-5*BW)/4,TH+BH,BW,BH,TRUE);
		::MoveWindow(HNo,(2*TW+BW)/4,TH+BH,BW,BH,TRUE);
		::ShowWindow(HNo,SW_SHOW);
	}
	else {
		::MoveWindow(HOk,(TW-BW)/2,TH+BH,BW,BH,TRUE);
	}

	if (PosX<=GetMonitorLeftmost(PosX, PosY)-100) {
		TmpDC = ::GetDC(GetSafeHwnd());
		PosX = (GetDeviceCaps(TmpDC,HORZRES)-WW) / 2;
		PosY = (GetDeviceCaps(TmpDC,VERTRES)-WH) / 2;
		::ReleaseDC(GetSafeHwnd(),TmpDC);
	}
	SetWindowPos(&wndTop,PosX,PosY,WW,WH,0);
	InvalidateRect(NULL);
}

BOOL CMsgDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) {
	case IDCANCEL:
		if( HIWORD(wParam) == BN_CLICKED ) {
			// メッセージボックスをキャンセルすると、マクロの終了とする。
			// (2008.8.5 yutaka)	
			int ret;
			char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
			get_lang_msg("MSG_MACRO_CONF", uimsg, sizeof(uimsg), "MACRO: confirmation", UILanguageFile);
			get_lang_msg("MSG_MACRO_HALT_SCRIPT", uimsg2, sizeof(uimsg2), "Are you sure that you want to halt this macro script?", UILanguageFile);
			ret = MessageBox(uimsg2, uimsg, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
			if (ret == IDYES) {
				if (YesNoFlag == TRUE) {
					EndDialog(IDCLOSE);
				} else {
					EndDialog(IDCANCEL);
				}
			}
			return TRUE;
		}
		return FALSE;

	case IDCLOSE:
		EndDialog(IDCANCEL);
		return TRUE;

	default:
		return (CDialog::OnCommand(wParam,lParam));
	}
}
