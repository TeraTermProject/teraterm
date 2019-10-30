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

/* TTMACRO.EXE, input dialog box */

#include "teraterm.h"
#include "ttlib.h"
#include "ttmdef.h"
#include "ttm_res.h"
#include "ttmlib.h"
#include "dlglib.h"
#include "ttmacro.h"
#include "compat_win.h"

#include "inpdlg.h"

// CInpDlg dialog
CInpDlg::CInpDlg(PCHAR Input, PCHAR Text, PCHAR Title,
                 PCHAR Default, BOOL Paswd,
                 int x, int y)
{
	InputStr = Input;
	TextStr = Text;
	TitleStr = Title;
	DefaultStr = Default;
	PaswdFlag = Paswd;
	PosX = x;
	PosY = y;
}

INT_PTR CInpDlg::DoModal()
{
	HINSTANCE hInst = GetInstance();
	HWND parent = GetHWND();
	return TTCDialog::DoModal(hInst, parent, CInpDlg::IDD);
}

// msgdlg のように、メッセージが長い場合にはダイアログを拡げるようにした (2006.7.29 maya)
BOOL CInpDlg::OnInitDialog()
{
	static const DlgTextInfo TextInfos[] = {
		{ IDOK, "BTN_OK" },
	};
	RECT R;
	HWND HEdit, HOk;

	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFile);
	SetWindowText(TitleStr);
	SetDlgItemText(IDC_INPTEXT,TextStr);
	SetDlgItemText(IDC_INPEDIT,DefaultStr);

	CalcTextExtent(GetDlgItem(IDC_INPTEXT), NULL, TextStr, &s);
	TW = s.cx + s.cx/10;
	TH = s.cy;

	HEdit = GetDlgItem(IDC_INPEDIT);
	::GetWindowRect(HEdit,&R);
	EW = R.right-R.left;
	EH = R.bottom-R.top;

	HOk = GetDlgItem(IDOK);
	::GetWindowRect(HOk,&R);
	BW = R.right-R.left;
	BH = R.bottom-R.top;

	GetWindowRect(&R);
	WW = R.right-R.left;
	WH = R.bottom-R.top;

	Relocation(TRUE, WW);

	BringupWindow(m_hWnd);

	return TRUE;
}

BOOL CInpDlg::OnOK()
{
	GetDlgItemText(IDC_INPEDIT,InputStr,MaxStrLen-1);
	EndDialog(IDOK);
	return TRUE;
}

LRESULT CInpDlg::OnExitSizeMove(WPARAM wParam, LPARAM lParam)
{
	RECT R;

	GetWindowRect(&R);
	if (R.bottom-R.top == WH && R.right-R.left == WW) {
		// サイズが変わっていなければ何もしない
	}
	else if (R.bottom-R.top != WH || R.right-R.left < init_WW) {
		// 高さが変更されたか、最初より幅が狭くなった場合は元に戻す
		SetWindowPos(HWND_TOP, R.left,R.top,WW,WH,0);
	}
	else {
		// そうでなければ再配置する
		Relocation(FALSE, R.right-R.left);
	}

	return TRUE;
}

void CInpDlg::Relocation(BOOL is_init, int new_WW)
{
	RECT R;
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

	HText = GetDlgItem(IDC_INPTEXT);
	HOk = GetDlgItem(IDOK);
	HEdit = GetDlgItem(IDC_INPEDIT);

	::MoveWindow(HText,(TW-s.cx)/2,BH/2,TW,TH,TRUE);
	::MoveWindow(HEdit,(WW-EW)/2-4,TH+BH,EW,EH,TRUE);
	::MoveWindow(HOk,(TW-BW)/2,TH+EH+BH*3/2,BW,BH,TRUE);

	if (PaswdFlag) {
		SendDlgItemMessage(IDC_INPEDIT,EM_SETPASSWORDCHAR,(UINT)'*',0);
	}

	SendDlgItemMessage(IDC_INPEDIT, EM_LIMITTEXT, MaxStrLen, 0);

	if (PosX<=GetMonitorLeftmost(PosX, PosY)-100) {
		// ウィンドウサイズをセット
		::SetWindowPos(m_hWnd, HWND_TOP,0,0,WW,WH,SWP_NOMOVE);
		// 中央に移動する
		CenterWindow(m_hWnd, m_hParentWnd);
		// 位置を保存
		RECT rcWnd;
		GetWindowRect(&rcWnd);
		PosX = rcWnd.left;
		PosY = rcWnd.top;
	} else {
		// ウィンドウサイズをセット + 指定位置へ移動
		::SetWindowPos(m_hWnd, HWND_TOP,PosX,PosY,WW,WH, 0);
	}

	InvalidateRect(NULL, TRUE);
}

LRESULT CInpDlg::DlgProc(UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg) {
	case WM_EXITSIZEMOVE:
		return OnExitSizeMove(wp, lp);
	case WM_DPICHANGED: {
		RECT rect;
		::GetWindowRect(m_hWnd, &rect);
		WW = rect.right - rect.left;
		WH = rect.bottom - rect.top;
		break;
	}
	}
	return FALSE;
}
