/*
 * Copyright (C) 2013-2019 TeraTerm Project
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
#include "tttypes.h"
#include "dlglib.h"
#include "ttmdlg.h"
#include "ttmacro.h"

#include "ListDlg.h"

// CListDlg ダイアログ

CListDlg::CListDlg(PCHAR Text, PCHAR Caption, CHAR **Lists, int Selected, int x, int y)
{
	m_Text = Text;
	m_Caption = Caption;
	m_Lists = Lists;
	m_Selected = Selected;
	PosX = x;
	PosY = y;
}

INT_PTR CListDlg::DoModal()
{
	HINSTANCE hInst = GetInstance();
	HWND hWndParent = GetHWND();
	return TTCDialog::DoModal(hInst, hWndParent, IDD);
}

BOOL CListDlg::OnInitDialog()
{
	static const DlgTextInfo TextInfos[] = {
		{ IDOK, "BTN_YES" },
		{ IDCANCEL, "BTN_CANCEL" },
	};
	char **p;
	int ListMaxWidth = 0;
	int ListWidth;
	int ListCount = 0;
	HDC DC;
	RECT R;
	HDC TmpDC;
	HWND HList, HOk;

	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFile);

	HList = ::GetDlgItem(m_hWnd, IDC_LISTBOX);
	DC = ::GetDC(HList);	// リストボックスを横スクロールできるように最大幅を取得

	p = m_Lists;
	while (*p) {
		SIZE size;
		SendDlgItemMessage(IDC_LISTBOX, LB_ADDSTRING, 0, (LPARAM)(*p));
		GetTextExtentPoint32(DC, *p, strlen(*p), &size);
		ListWidth = size.cx;
		if (ListWidth > ListMaxWidth) {
			ListMaxWidth = ListWidth;
		}
		ListCount++;
		p++;
	}

	SendDlgItemMessage(IDC_LISTBOX, LB_SETHORIZONTALEXTENT, (ListMaxWidth + 5), 0);
	::ReleaseDC(m_hWnd, DC);

	if (m_Selected < 0 || m_Selected >= ListCount) {
		m_Selected = 0;
	}
	SetCurSel(IDC_LISTBOX, m_Selected);

	// 本文とタイトル
	SetDlgItemText(IDC_LISTTEXT, m_Text);
	SetWindowText(m_Caption);

	TmpDC = ::GetDC(GetDlgItem(IDC_LISTTEXT));
	CalcTextExtent(TmpDC,m_Text,&s);
	::ReleaseDC(GetDlgItem(IDC_LISTTEXT), TmpDC);
	TW = s.cx + s.cx/10;
	TH = s.cy;

	::GetWindowRect(HList,&R);
	LW = R.right-R.left;
	LH = R.bottom-R.top;

	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
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

BOOL CListDlg::OnOK()
{
	m_SelectItem = GetCurSel(IDC_LISTBOX);
	return TTCDialog::OnOK();
}

BOOL CListDlg::OnCancel()
{
	return TTCDialog::OnCancel();
}

//int MessageBoxHaltScript(HWND hWnd);

BOOL CListDlg::OnClose()
{
	int ret = MessageBoxHaltScript(m_hWnd);
	if (ret == IDYES) {
		EndDialog(IDCLOSE);
	}
	return TRUE;
}

void CListDlg::Relocation(BOOL is_init, int new_WW)
{
	RECT R;
	HDC TmpDC;
	HWND HText, HOk, HCancel, HList;
	int CW, CH;

	::GetClientRect(m_hWnd, &R);
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
		WH = TH + LH + (int)(BH*1.5) + (WH - CH);
		init_WW = WW;
		// リストボックスサイズの計算
		if (LW < WW - BW - 14*3) {
			LW = WW - BW - 14*3;
		}
	}
	else {
		TW = CW;
		WW = new_WW;
	}

	HText = ::GetDlgItem(GetSafeHwnd(), IDC_LISTTEXT);
	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	HCancel = ::GetDlgItem(GetSafeHwnd(), IDCANCEL);
	HList = ::GetDlgItem(GetSafeHwnd(), IDC_LISTBOX);

	::MoveWindow(HText,(TW-s.cx)/2,LH+BH,TW,TH,TRUE);
	::MoveWindow(HList,14,BH/2,LW,LH,TRUE);
	::MoveWindow(HOk,14+14+LW,BH/2,BW,BH,TRUE);
	::MoveWindow(HCancel,14+14+LW,BH*2,BW,BH,TRUE);

	if (PosX<=GetMonitorLeftmost(PosX, PosY)-100) {
		::GetWindowRect(m_hWnd, &R);
		TmpDC = ::GetDC(GetSafeHwnd());
		PosX = (GetDeviceCaps(TmpDC,HORZRES)-R.right+R.left) / 2;
		PosY = (GetDeviceCaps(TmpDC,VERTRES)-R.bottom+R.top) / 2;
		::ReleaseDC(GetSafeHwnd(),TmpDC);
	}
	::SetWindowPos(m_hWnd, HWND_TOP,PosX,PosY,WW,WH,0);

	::InvalidateRect(m_hWnd, NULL, TRUE);
}

