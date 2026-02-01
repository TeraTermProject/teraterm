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

/* TTMACRO.EXE, message dialog box */

#include <windows.h>
#include <windowsx.h>
#include "teraterm.h"
#include "tmfc.h"
#include "dlglib.h"
#include "compat_win.h"

#include "ttlib.h"
#include "ttm_res.h"
#include "ttmlib.h"
#include "ttmdlg.h"

#include "msgdlg.h"

// CMsgDlg dialog

CMsgDlg::CMsgDlg(const wchar_t *Text, const wchar_t *Title, BOOL YesNo,
                 int x, int y)
{
	wcscpy_s(TextStr, MaxStrLen, Text);
	TitleStr = Title;
	YesNoFlag = YesNo;
	PosX = x;
	PosY = y;
}

INT_PTR CMsgDlg::DoModal(HINSTANCE hInst, HWND hWndParent)
{
	m_hInst = hInst;
	return TTCDialog::DoModal(hInst, hWndParent, CMsgDlg::IDD);
}

BOOL CMsgDlg::OnInitDialog()
{
	RECT R;
	HWND HOk;

	// IDOK のデフォルト "OK", 表示
	// IDCANCEL のデフォルト "No", 非表示
	if (YesNoFlag) {
		static const DlgTextInfo TextInfosYesNo[] = {
			{ IDOK, "BTN_YES" },
			{ IDCANCEL, "BTN_NO" },
		};
		SetDlgItemTextA(IDOK, "Yes");	// lng ファイルなし対策
		SetDlgTextsW(m_hWnd, TextInfosYesNo, _countof(TextInfosYesNo), UILanguageFileW);
	} else {
		static const DlgTextInfo TextInfosOk[] = {
			{ IDOK, "BTN_OK" },
		};
		SetDlgTextsW(m_hWnd, TextInfosOk, _countof(TextInfosOk), UILanguageFileW);
	}
	dpi = GetMonitorDpiFromWindow(m_hWnd);
	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), dpi);

	SetWindowTextW(TitleStr);
	SetDlgItemTextW(IDC_MSGTEXT,TextStr);
	CalcTextExtentW(GetDlgItem(IDC_MSGTEXT), NULL, TextStr, &s);
	TW = s.cx + (int)(16 * dpi / 96.f);
	TH = s.cy;

	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	::GetWindowRect(HOk,&R);
	BW = R.right-R.left;
	BH = R.bottom-R.top;

	GetWindowRect(&R);
	WW = R.right-R.left;
	WH = R.bottom-R.top;

	in_init = TRUE;
	Relocation(TRUE, WW, WH);
	in_init = FALSE;

	BringupWindow(this->m_hWnd);

	return TRUE;
}

LRESULT CMsgDlg::OnExitSizeMove(WPARAM wParam, LPARAM lParam)
{
	RECT R;
	int current_WW, current_WH;

	GetWindowRect(&R);
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

void CMsgDlg::Relocation(BOOL is_init, int new_WW, int new_WH)
{
	RECT R;
	HWND HText, HOk, HNo;
	int CW, CH;

	GetClientRect(&R);
	CW = R.right-R.left;
	CH = R.bottom-R.top;

	// 初回のみ
	if (is_init) {
		// テキストコントロールサイズを補正
		if (TW < BW) {
			TW = BW * 2;
		}
		if (YesNoFlag && (TW < 7*BW/2)) {
			TW = 7*BW/2;
		}
		// ウインドウサイズの計算
		GetWindowRect(&R);
		WW = TW + (R.right - R.left - CW);
		WH = TH + (R.bottom - R.top - CH) + BH + BH*3/2;
		init_WW = WW;
		init_WH = WH;
		// 実際のサイズを取得
		::SetWindowPos(m_hWnd, HWND_TOP, 0, 0, WW, WH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		GetClientRect(&R);
		TW = R.right - R.left;
		GetWindowRect(&R);
		WW = R.right - R.left;
		WH = R.bottom - R.top;
	}
	else {
		TW = CW;
		WW = new_WW;
		WH = new_WH;
	}

	HText = ::GetDlgItem(GetSafeHwnd(), IDC_MSGTEXT);
	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	HNo = ::GetDlgItem(GetSafeHwnd(), IDCANCEL);

	::MoveWindow(HText,(TW-s.cx)/2, BH/2,TW,TH,TRUE);
	if (YesNoFlag) {
		::MoveWindow(HOk,(2*TW-5*BW)/4,TH+BH,BW,BH,TRUE);
		::MoveWindow(HNo,(2*TW+BW)/4,TH+BH,BW,BH,TRUE);
		::ShowWindow(HNo,SW_SHOW);
	}
	else {
		::MoveWindow(HOk,(TW-BW)/2,TH+BH,BW,BH,TRUE);
	}

	if (is_init) {
		if (SetDlgPosEX(GetSafeHwnd(), WW, WH, &PosX, &PosY) < 0) {
			SetDlgPos();
		}
	}

	InvalidateRect(NULL);
}

BOOL CMsgDlg::OnCancel()
{
	if (!YesNoFlag) {
		// ok(yes)だけのときは、cancel処理は何もしない
		return TRUE;
	} else {
		// yes/noのときは、デフォルト処理(終了)
		return TTCDialog::OnCancel();
	}
}

// メッセージボックスをキャンセルする(closeボタンを押す)と、マクロの終了とする。
// (2008.8.5 yutaka)
BOOL CMsgDlg::OnClose()
{
	const int ret = MessageBoxHaltScript(m_hWnd);
	if (ret == IDYES) {
		if (YesNoFlag == TRUE) {
			EndDialog(IDCLOSE);
		} else {
			EndDialog(IDCANCEL);
		}
	}
	return TRUE;
}

LRESULT CMsgDlg::DlgProc(UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg) {
	case WM_EXITSIZEMOVE:
		return OnExitSizeMove(wp, lp);
	case WM_DPICHANGED:
		int new_dpi, CW, CH;
		float mag;
		RECT R;

		new_dpi = HIWORD(wp);
		mag = new_dpi / (float)dpi;
		dpi = new_dpi;
		CalcTextExtentW(GetDlgItem(IDC_MSGTEXT), NULL, TextStr, &s);
		TW = s.cx + (int)(16 * dpi / 96.f);
		TH = s.cy;
		::GetWindowRect(GetDlgItem(IDOK), &R);
		BW = R.right - R.left;
		BH = R.bottom - R.top;
		R = *(RECT *)lp;
		WW = R.right - R.left;
		WH = R.bottom - R.top;
		GetClientRect(&R);
		CW = R.right - R.left;
		CH = R.bottom - R.top;
		if (TW < BW) {
			TW = BW * 2;
		}
		if (YesNoFlag && (TW < 7*BW/2)) {
			TW = 7*BW/2;
		}
		init_WW = TW + WW - CW;
		init_WH = TH + WH - CH + BH + BH*3/2;

		TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), dpi);
		if (in_init) {
			::SetWindowPos(m_hWnd, HWND_TOP, 0, 0, WW, WH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		Relocation(in_init, WW, WH);
		return TRUE;
	}
	return FALSE;
}
