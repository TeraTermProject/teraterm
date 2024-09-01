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
// ListDlg.cpp : �����t�@�C��
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

#define CONTROL_GAP_W	14		// �E�B���h�E�[�ƃR���g���[���ԂƂ̕�

// CListDlg �_�C�A���O

CListDlg::CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y, int ext, int width, int height)
{
	m_Text = Text;
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
	static const ResizeHelperInfo resize_info[] = {
		{ IDC_LISTBOX, RESIZE_HELPER_ANCHOR_LRTB },
		{ IDOK, RESIZE_HELPER_ANCHOR_RIGHT },
		{ IDCANCEL, RESIZE_HELPER_ANCHOR_RIGHT },
		{ IDC_LISTTEXT, RESIZE_HELPER_ANCHOR_LRB },
	};
	RECT R;
	HWND HList, HOk;
	int NonClientAreaWidth;
	int NonClientAreaHeight;

	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFileW);

	HList = ::GetDlgItem(m_hWnd, IDC_LISTBOX);
	InitList(HList);

	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), 0);
	if (m_ext & ExtListBoxMinmaxbutton) {
		ModifyStyle(0, WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
	}

	// �{���ƃ^�C�g��
	SetDlgItemTextW(IDC_LISTTEXT, m_Text);
	SetWindowTextW(m_Caption);

	CalcTextExtentW(GetDlgItem(IDC_LISTTEXT), NULL, m_Text,&s);
	TW = s.cx + s.cx/10;
	TH = s.cy;

	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	::GetWindowRect(HOk,&R);
	BW = R.right-R.left;
	BH = R.bottom-R.top;

	GetWindowRect(&R);
	WW = R.right-R.left;
	WH = R.bottom-R.top;

	if (m_ext & ExtListBoxSize) {
		LW = TH * m_width / 2;
		LH = TH * m_height;
		TW = LW;
		s.cx = s.cx + s.cx/10;
		::GetClientRect(m_hWnd, &R);
		NonClientAreaWidth = WW - (R.right - R.left);
		NonClientAreaHeight = WH - (R.bottom - R.top);
		WW = TW + NonClientAreaWidth + CONTROL_GAP_W * 4 + BW;
		WH = TH + LH + (int)(BH*1.5) + NonClientAreaHeight;
		SetWindowPos(HWND_TOP, 0, 0, WW, WH, SWP_NOMOVE);
	} else {
		::GetWindowRect(HList,&R);
		LW = R.right-R.left;
		LH = R.bottom-R.top;
	}

	ResizeHelper = ReiseHelperInit(m_hWnd, FALSE, resize_info, _countof(resize_info));
	Relocation(TRUE, WW);

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

void CListDlg::Relocation(BOOL is_init, int new_WW)
{
	RECT R;
	HWND HText, HOk, HCancel, HList;
	int CW, CH;
	int NonClientAreaWidth;
	int NonClientAreaHeight;

	::GetClientRect(m_hWnd, &R);
	CW = R.right-R.left;
	CH = R.bottom-R.top;
	NonClientAreaWidth = WW - CW;
	NonClientAreaHeight = WH - CH;

	// ����̂�
	if (is_init) {
		// �e�L�X�g�R���g���[���T�C�Y��␳
		if (TW < CW) {
			TW = CW;
		}
		// �E�C���h�E�T�C�Y�̌v�Z
		WW = TW + NonClientAreaWidth;
		CW = WW - NonClientAreaWidth;
		WH = TH + LH + (int)(BH*1.5) + NonClientAreaHeight;		// (�{�^���̍���/2) ���E�B���h�E�[�ƃR���g���[���ԂƂ̍���
		init_WW = WW;
		// ���X�g�{�b�N�X�T�C�Y�̌v�Z
		if (LW < CW - BW - CONTROL_GAP_W * 3) {
			LW = CW - BW - CONTROL_GAP_W * 3;
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
	::MoveWindow(HList,CONTROL_GAP_W,BH/2,LW,LH,TRUE);
	::MoveWindow(HOk,CONTROL_GAP_W+CONTROL_GAP_W+LW,BH/2,BW,BH,TRUE);
	::MoveWindow(HCancel,CONTROL_GAP_W+CONTROL_GAP_W+LW,BH*2,BW,BH,TRUE);

	SetDlgPos();

	::InvalidateRect(m_hWnd, NULL, TRUE);
}

LRESULT CListDlg::DlgProc(UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
		case WM_SIZE:
			if (wp == SIZE_MINIMIZED) {
				TTLShowMINIMIZE();
				ShowWindow(SW_MINIMIZE);
				return TRUE;
			}
			if (ResizeHelper != NULL) {
				ReiseDlgHelper_WM_SIZE(ResizeHelper);
			}
			break;
		case WM_GETMINMAXINFO:
			if (ResizeHelper != NULL) {
				ReiseDlgHelper_WM_GETMINMAXINFO(ResizeHelper, lp);
			}
			break;
		case WM_DESTROY:
			if (ResizeHelper != NULL) {
				ReiseDlgHelperDelete(ResizeHelper);
				ResizeHelper = NULL;
			}
			break;
		case WM_COMMAND:
			if (m_ext & ExtListBoxDoubleclick) {
				if (HIWORD(wp) == LBN_DBLCLK) {
					OnOK();
					return TRUE;
				}
				break;
			}
	}
	return (LRESULT)FALSE;
}
