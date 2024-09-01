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

#include "inpdlg.h"

// CInpDlg dialog
CInpDlg::CInpDlg(wchar_t *Input, const wchar_t *Text, const wchar_t *Title,
                 const wchar_t *Default, BOOL Paswd,
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

INT_PTR CInpDlg::DoModal(HINSTANCE hInst, HWND hWndParent)
{
	m_hInst = hInst;
	return TTCDialog::DoModal(hInst, hWndParent, CInpDlg::IDD);
}

// msgdlg �̂悤�ɁA���b�Z�[�W�������ꍇ�ɂ̓_�C�A���O���g����悤�ɂ��� (2006.7.29 maya)
BOOL CInpDlg::OnInitDialog()
{
	static const DlgTextInfo TextInfos[] = {
		{ IDOK, "BTN_OK" },
	};
	RECT R;
	HWND HEdit, HOk;

	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), 0);

	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFileW);
	SetWindowTextW(TitleStr);
	SetDlgItemTextW(IDC_INPTEXT,TextStr);
	SetDlgItemTextW(IDC_INPEDIT,DefaultStr);

	CalcTextExtentW(GetDlgItem(IDC_INPTEXT), NULL, TextStr, &s);
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

	GetWindowRect(&R);
	if (R.bottom-R.top == WH && R.right-R.left == WW) {
		// �T�C�Y���ς���Ă��Ȃ���Ή������Ȃ�
	}
	else if (R.bottom-R.top != WH || R.right-R.left < init_WW) {
		// �������ύX���ꂽ���A�ŏ���蕝�������Ȃ����ꍇ�͌��ɖ߂�
		SetWindowPos(HWND_TOP, R.left,R.top,WW,WH,0);
	}
	else {
		// �����łȂ���΍Ĕz�u����
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

	// ����̂�
	if (is_init) {
		// �e�L�X�g�R���g���[���T�C�Y��␳
		if (TW < CW) {
			TW = CW;
		}
		if (EW < s.cx) {
			EW = s.cx;
		}
		// �E�C���h�E�T�C�Y�̌v�Z
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

	if (!PaswdFlag) {
		SendDlgItemMessage(IDC_INPEDIT,EM_SETPASSWORDCHAR, 0, 0);
	}

	SendDlgItemMessage(IDC_INPEDIT, EM_LIMITTEXT, MaxStrLen, 0);

	SetDlgPos();

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
