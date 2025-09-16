/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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
#include "compat_win.h"
#include "dlglib.h"

#include "ttmdef.h"
#include "ttm_res.h"
#include "ttmlib.h"
#include "ttmdlg.h"

#include "inpdlg.h"

// CInpDlg dialog
CInpDlg::CInpDlg(wchar_t *Input, const wchar_t *Text, const wchar_t *Title,
                 const wchar_t *Default, BOOL Paswd,
                 int x, int y)
{
	InputStr = Input;
	wcscpy_s(TextStr, MaxStrLen, Text);
	TitleStr = Title;
	DefaultStr = Default;
	PaswdFlag = Paswd;
	PosX = x;
	PosY = y;
}

INT_PTR CInpDlg::DoModal(HINSTANCE hInst, HWND hWndParent)
{
	m_hInst = hInst;
	return TTCDialog::DoModal(hInst, hWndParent, CInpDlg::IDD);
}

// msgdlg のように、メッセージが長い場合にはダイアログを拡げるようにした (2006.7.29 maya)
BOOL CInpDlg::OnInitDialog()
{
	static const DlgTextInfo TextInfos[] = {
		{ IDOK, "BTN_OK" },
	};
	RECT R;
	HWND HEdit, HOk;

	dpi = GetMonitorDpiFromWindow(m_hWnd);
	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), dpi);

	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFileW);
	SetWindowTextW(TitleStr);
	SetDlgItemTextW(IDC_INPTEXT,TextStr);
	SetDlgItemTextW(IDC_INPEDIT,DefaultStr);

	CalcTextExtentW(GetDlgItem(IDC_INPTEXT), NULL, TextStr, &s);
	TW = s.cx + (int)(16 * dpi / 96.f);
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

	in_init = TRUE;
	Relocation(TRUE, WW, WH);
	in_init = FALSE;

	BringupWindow(m_hWnd);

	return TRUE;
}

BOOL CInpDlg::OnOK()
{
	GetDlgItemTextW(IDC_INPEDIT,InputStr,MaxStrLen-1);
	EndDialog(IDOK);
	return TRUE;
}

BOOL CInpDlg::OnClose()
{
	int ret = MessageBoxHaltScript(m_hWnd);
	if (ret == IDYES) {
		InputStr = NULL;
		EndDialog(IDCLOSE);
	}
	return TRUE;
}

LRESULT CInpDlg::OnExitSizeMove(WPARAM wParam, LPARAM lParam)
{
	RECT R;
	int current_WW, current_WH;

	::GetWindowRect(m_hWnd, &R);
	current_WW = R.right - R.left;
	current_WH = R.bottom - R.top;

	if (current_WW == WW && current_WH == WH) {
		// サイズが変わっていなければ何もしない
		PosX = R.left;
		PosY = R.top;
	}
	else {
		int new_WW;

		// 高さが変更されたか、最初より幅が狭くなった場合は元に戻す
		if (current_WW < init_WW) {
			new_WW = init_WW;
			if (PosX != R.left) {
				PosX = R.right - new_WW;
			} else {
				PosX = R.left;
			}
		} else {
			new_WW = current_WW;
			PosX = R.left;
		}
		if (current_WH < init_WH) {
			if (PosY != R.top) {
				PosY = R.bottom - init_WH;
			} else {
				PosY = R.top;
			}
		} else {
			if (PosY != R.top) {
				PosY = R.top;
			} else {
				PosY = R.bottom - init_WH;
			}
		}

		::SetWindowPos(m_hWnd, HWND_TOP, PosX, PosY, new_WW, init_WH, SWP_NOZORDER | SWP_NOACTIVATE);
		Relocation(FALSE, new_WW, init_WH);
	}

	return TRUE;
}

void CInpDlg::Relocation(BOOL is_init, int new_WW, int new_WH)
{
	RECT R;
	HWND HText, HOk, HEdit;
	int c_WW, c_WH, CW, CH, CONTROL_GAP_W, CONTROL_GAP_H;

	GetWindowRect(&R);
	c_WW = R.right - R.left;
	c_WH = R.bottom - R.top;
	GetClientRect(&R);
	CW = R.right - R.left;
	CH = R.bottom - R.top;
	CONTROL_GAP_W = c_WW - CW;
	CONTROL_GAP_H = c_WH - CH;

	// 初回のみ
	if (is_init) {
		// テキストコントロールサイズを補正
		if (TW < (int)(224 * dpi / 96.f)) {
			TW = (int)(224 * dpi / 96.f);
		}
		if (EW < s.cx) {
			EW = s.cx;
		}
		// ウインドウサイズの計算
		TW += (int)(26 * dpi / 96.f);
		WW = TW + CONTROL_GAP_W;
		WH = TH + CONTROL_GAP_H + EH + BH + BH*2;
		EW = WW - CONTROL_GAP_W - (int)(26 * dpi / 96.f);
		init_WW = WW;
		init_WH = WH;
	}
	else {
		TW = CW;
		EW = CW - (int)(26 * dpi / 96.f);
		WW = new_WW;
		WH = new_WH;
	}

	HText = GetDlgItem(IDC_INPTEXT);
	HOk = GetDlgItem(IDOK);
	HEdit = GetDlgItem(IDC_INPEDIT);

	::MoveWindow(HText,(TW-s.cx)/2,BH/2,TW,TH,TRUE);
	::MoveWindow(HEdit,(WW-EW)/2-CONTROL_GAP_W/2,TH+BH,EW,EH,TRUE);
	::MoveWindow(HOk,(TW-BW)/2,TH+EH+BH*3/2,BW,BH,TRUE);

	if (!PaswdFlag) {
		SendDlgItemMessage(IDC_INPEDIT,EM_SETPASSWORDCHAR, 0, 0);
	}

	SendDlgItemMessage(IDC_INPEDIT, EM_LIMITTEXT, MaxStrLen, 0);

	if (is_init) {
		if (SetDlgPosEX(GetSafeHwnd(), WW, WH, &PosX, &PosY) < 0) {
			SetDlgPos();
		}
	}

	InvalidateRect(NULL, TRUE);
}

LRESULT CInpDlg::DlgProc(UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg) {
	case WM_EXITSIZEMOVE:
		return OnExitSizeMove(wp, lp);
	case WM_DPICHANGED:
		int new_dpi, CW, CH, CONTROL_GAP_W, CONTROL_GAP_H;
		float mag;
		RECT R;

		new_dpi = HIWORD(wp);
		mag = new_dpi / (float)dpi;
		dpi = new_dpi;
		CalcTextExtentW(GetDlgItem(IDC_INPTEXT), NULL, TextStr, &s);
		TW = s.cx + (int)(16 * dpi / 96.f);
		TH = s.cy;
		::GetWindowRect(GetDlgItem(IDC_INPEDIT), &R);
		EW = R.right-R.left;
		EH = R.bottom-R.top;
		::GetWindowRect(GetDlgItem(IDOK), &R);
		BW = R.right-R.left;
		BH = R.bottom-R.top;
		R = *(RECT *)lp;
		WW = R.right - R.left;
		WH = R.bottom - R.top;
		GetClientRect(&R);
		CW = R.right-R.left;
		CH = R.bottom-R.top;
		CONTROL_GAP_W = WW - CW;
		CONTROL_GAP_H = WH - CH;
		if (TW < (int)(224 * dpi / 96.f)) {
			TW = (int)(224 * dpi / 96.f);
		}
		if (EW < s.cx) {
			EW = s.cx;
		}
		TW += (int)(26 * dpi / 96.f);
		EW += CONTROL_GAP_W;
		init_WW = TW + CONTROL_GAP_W;
		init_WH = TH + CONTROL_GAP_H + EH + BH + BH*2;
		EW = WW - CONTROL_GAP_W - (int)(26 * dpi / 96.f);

		TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), dpi);
		if (in_init) {
			::SetWindowPos(m_hWnd, HWND_TOP, 0, 0, WW, WH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		Relocation(in_init, WW, WH);
		return TRUE;
	}
	return FALSE;
}
