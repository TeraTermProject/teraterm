/*
 * Copyright (C) 2013- TeraTerm Project
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
//
// ListDlg.cpp : 実装ファイル
//

#include "tmfc.h"
#include "teraterm.h"
#include "ttlib.h"
#include "ttm_res.h"
#include "ttmlib.h"
#include "dlglib.h"
#include "ttmdlg.h"
#include "ttmacro.h"
#include "ttl.h"
#include "ttmparse.h"

#include "ListDlg.h"

// CListDlg ダイアログ

CListDlg::CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y, int ext, int width, int height)
{
	m_SelectItem = 0;
	CONTROL_GAP_W = 14;
	wcscpy_s(m_Text, MaxStrLen, Text);
	m_Caption = Caption;
	m_Lists = Lists;
	m_Selected = Selected;
	PosX = x;
	PosY = y;
	m_ext = ext;
	m_width = width;
	m_height = height;
}

INT_PTR CListDlg::DoModal(HINSTANCE hInst, HWND hWndParent)
{
	m_hInst = hInst;
	return TTCDialog::DoModal(hInst, hWndParent, IDD);
}

void CListDlg::InitList(HWND HList)
{
	wchar_t **p;
	int ListMaxWidth = 0;
	int ListCount = 0;
	HDC DC = ::GetDC(HList);
	HFONT hFontList = (HFONT)::SendMessage(HList,WM_GETFONT,0,0);
	HFONT hOldFont = (HFONT)SelectObject(DC,hFontList);
	p = m_Lists;
	while (*p) {
		SIZE size;
		int ListWidth;
		SendDlgItemMessageW(IDC_LISTBOX, LB_ADDSTRING, 0, (LPARAM)(*p));
		GetTextExtentPoint32W(DC, *p, (int)wcslen(*p), &size);
		ListWidth = size.cx;
		if (ListWidth > ListMaxWidth) {
			ListMaxWidth = ListWidth;
		}
		ListCount++;
		p++;
	}

	SendDlgItemMessage(IDC_LISTBOX, LB_SETHORIZONTALEXTENT, (ListMaxWidth + 5), 0);
	SelectObject(DC,hOldFont);
	::ReleaseDC(HList, DC);

	if (m_Selected < 0 || m_Selected >= ListCount) {
		m_Selected = 0;
	}
	SetCurSel(IDC_LISTBOX, m_Selected);
}

BOOL CListDlg::OnInitDialog()
{
	static const DlgTextInfo TextInfos[] = {
		{ IDOK, "BTN_YES" },
		{ IDCANCEL, "BTN_CANCEL" },
	};
	RECT R;
	HWND HList, HOk;
	int NonClientAreaWidth;
	int NonClientAreaHeight;

	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFileW);

	HList = ::GetDlgItem(m_hWnd, IDC_LISTBOX);
	InitList(HList);

	dpi = GetMonitorDpiFromWindow(m_hWnd);
	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), dpi);
	CONTROL_GAP_W = (int)(14 * dpi / 96.f);
	if (m_ext & ExtListBoxMinmaxbutton) {
		ModifyStyle(0, WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
	}

	// 本文とタイトル
	SetDlgItemTextW(IDC_LISTTEXT, m_Text);
	SetWindowTextW(m_Caption);

	CalcTextExtentW(GetDlgItem(IDC_LISTTEXT), NULL, m_Text,&s);
	TW = s.cx + (int)(16 * dpi / 96.f);
	TH = s.cy;

	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	::GetWindowRect(HOk,&R);
	BW = R.right-R.left;
	BH = R.bottom-R.top;

	GetWindowRect(&R);
	WW = R.right-R.left;
	WH = R.bottom-R.top;

	if (m_ext & ExtListBoxSize) {
		SIZE tmp_s;
		CalcTextExtentW(GetDlgItem(IDC_LISTBOX), NULL, L"A", &tmp_s);
		LW = tmp_s.cy * m_width / 2; // 横幅は縦の半分とみなす
		LH = tmp_s.cy * m_height;
		::GetClientRect(m_hWnd, &R);
		NonClientAreaWidth  = WW - (R.right - R.left);
		NonClientAreaHeight = WH - (R.bottom - R.top);
		WW = LW + NonClientAreaWidth + BW + CONTROL_GAP_W * 4;
		WH = LH + NonClientAreaHeight+ TH + (int)(BH*1.5);
		SetWindowPos(HWND_TOP, 0, 0, WW, WH, SWP_NOMOVE);
	} else {
		::GetWindowRect(HList,&R);
		LW = R.right-R.left;
		LH = R.bottom-R.top;
	}

	in_init = TRUE;
	Relocation(TRUE, WW, WH);
	in_init = FALSE;

	if (m_ext & ExtListBoxMinimize) {
		ShowWindow(SW_MINIMIZE);
	} else if (m_ext & ExtListBoxMaximize) {
		ShowWindow(SW_MAXIMIZE);
	}

	BringupWindow(m_hWnd);

	return TRUE;
}

BOOL CListDlg::OnOK()
{
	m_SelectItem = GetCurSel(IDC_LISTBOX);
	return TTCDialog::OnOK();
}

BOOL CListDlg::OnCancel()
{
	return TTCDialog::OnCancel();
}

BOOL CListDlg::OnClose()
{
	int ret = MessageBoxHaltScript(m_hWnd);
	if (ret == IDYES) {
		EndDialog(IDCLOSE);
	}
	return TRUE;
}

void CListDlg::Relocation(BOOL is_init, int new_WW, int new_WH)
{
	RECT R;
	HWND HText, HOk, HCancel, HList;
	int CW, CH;
	int NonClientAreaWidth;
	int NonClientAreaHeight;

	::GetClientRect(m_hWnd, &R);
	CW = R.right-R.left;
	CH = R.bottom-R.top;
	NonClientAreaWidth  = WW - CW;
	NonClientAreaHeight = WH - CH;

	// 初回のみ
	if (is_init) {
		// テキストコントロールサイズを補正
		if (TW < CW) {
			TW = CW;
			use_TW = FALSE;
		} else {
			use_TW = TRUE;
		}
		// ウインドウサイズの計算
		WW = TW + NonClientAreaWidth;
		CW = WW - NonClientAreaWidth;
		WH = TH + LH + (int)(BH*1.5) + NonClientAreaHeight;		// (ボタンの高さ/2) がウィンドウ端とコントロール間との高さ
		// リストボックスサイズの計算
		if (LW < CW - BW - CONTROL_GAP_W * 3) {
			LW = CW - BW - CONTROL_GAP_W * 3;
		}
		init_WW = WW;
		init_WH = WH;
		init_LW = LW;
		init_LH = LH;
	}
	else {
		TW = CW;
		WW = new_WW;
		WH = new_WH;
	}

	HText = ::GetDlgItem(GetSafeHwnd(), IDC_LISTTEXT);
	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	HCancel = ::GetDlgItem(GetSafeHwnd(), IDCANCEL);
	HList = ::GetDlgItem(GetSafeHwnd(), IDC_LISTBOX);

	::MoveWindow(HText,(TW-s.cx)/2,LH+BH,TW,TH,TRUE);
	::MoveWindow(HList,CONTROL_GAP_W,BH/2,LW,LH,TRUE);
	::MoveWindow(HOk,CONTROL_GAP_W+CONTROL_GAP_W+LW,BH/2,BW,BH,TRUE);
	::MoveWindow(HCancel,CONTROL_GAP_W+CONTROL_GAP_W+LW,BH*2,BW,BH,TRUE);

	if (is_init) {
		if (SetDlgPosEX(GetSafeHwnd(), WW, WH, &PosX, &PosY) < 0) {
			SetDlgPos();
		}
	}

	::InvalidateRect(m_hWnd, NULL, TRUE);
}

LRESULT CListDlg::DlgProc(UINT msg, WPARAM wp, LPARAM lp)
{
	RECT R;
	int CW, CH;
	LPMINMAXINFO pmmi = (LPMINMAXINFO)lp;

	switch (msg) {
		case WM_SIZE:
			if (GetMonitorDpiFromWindow(m_hWnd) == dpi) {
				if (wp == SIZE_MINIMIZED) {
					TTLShowMINIMIZE();
					ShowWindow(SW_MINIMIZE);
					return TRUE;
				}
				// 実際の位置とサイズを反映
				GetWindowRect(&R);
				PosX = R.left;
				PosY = R.top;
				WW = R.right-R.left;
				WH = R.bottom-R.top;
				// LWとLHを更新
				GetClientRect(&R);
				CW = R.right-R.left;
				CH = R.bottom-R.top;
				LW = CW - BW - CONTROL_GAP_W * 3;
				LH = CH - TH - (int)(BH*1.5);
				Relocation(FALSE, WW, WH);
				return TRUE;
			}
		case WM_GETMINMAXINFO:
			if (GetMonitorDpiFromWindow(m_hWnd) == dpi) {
				pmmi->ptMinTrackSize.x = init_WW;
				pmmi->ptMinTrackSize.y = init_WH;
				return TRUE;
			}
		case WM_COMMAND:
			if (m_ext & ExtListBoxDoubleclick) {
				if (HIWORD(wp) == LBN_DBLCLK) {
					OnOK();
					return TRUE;
				}
			}
			break;
		case WM_DPICHANGED:
			int new_dpi, NonClientAreaWidth, NonClientAreaHeight;
			float mag;
			RECT R;

			new_dpi = HIWORD(wp);
			mag = new_dpi / (float)dpi;
			dpi = new_dpi;
			CONTROL_GAP_W = (int)(14 * dpi / 96.f);
			GetWindowRect(&R);
			WW = R.right - R.left;
			WH = R.bottom - R.top;
			GetClientRect(&R);
			CW = R.right - R.left;
			CH = R.bottom - R.top;
			NonClientAreaWidth  = WW - CW;
			NonClientAreaHeight = WH - CH;
			::GetWindowRect(GetDlgItem(IDOK), &R);
			BW = R.right - R.left;
			BH = R.bottom - R.top;
			CalcTextExtentW(GetDlgItem(IDC_LISTTEXT), NULL, m_Text, &s);
			TW = s.cx + (int)(16 * dpi / 96.f);
			TH = s.cy;
			::GetWindowRect(GetDlgItem(IDC_LISTBOX), &R);
			LW = R.right - R.left;
			LH = R.bottom - R.top;
			if (m_ext & ExtListBoxSize) {
				SIZE tmp_s;
				CalcTextExtentW(GetDlgItem(IDC_LISTBOX), NULL, L"A", &tmp_s);
				init_LW = tmp_s.cy * m_width / 2; // 横幅は縦の半分とみなす
				init_LH = tmp_s.cy * m_height;
				init_WW = init_LW + NonClientAreaWidth  + BW + CONTROL_GAP_W * 4;
				init_WH = init_LH + NonClientAreaHeight + TH + (int)(BH*1.5);
			} else {
				init_LW = (int)(init_LW * mag);
				init_LH = (int)(init_LH * mag);
				if (use_TW) {
					// dpiによる拡大/縮小には変換誤差があるため、CW < IDC_LISTTEXT の場合は、IDC_LISTTEXTのサイズで計算する
					init_WW = TW + NonClientAreaWidth;
				} else {
					init_WW = init_LW + NonClientAreaWidth + BW + CONTROL_GAP_W * 3;
				}
				init_WH = init_LH + NonClientAreaHeight + TH + (int)(BH*1.5);
			}

			TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), dpi);
			::SetWindowPos(m_hWnd, HWND_TOP, 0, 0, WW, WH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

			if (in_init) {
				Relocation(FALSE, init_WW, init_WH);
				if (SetDlgPosEX(GetSafeHwnd(), WW, WH, &PosX, &PosY) < 0) {
					SetDlgPos();
				}
			} else {
				Relocation(FALSE, WW, WH);
			}
			return TRUE;
	}
	return (LRESULT)FALSE;
}
