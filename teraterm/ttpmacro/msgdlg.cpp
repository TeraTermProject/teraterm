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
	TextStr = Text;
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

	// IDOK �̃f�t�H���g "OK", �\��
	// IDCANCEL �̃f�t�H���g "No", ��\��
	if (YesNoFlag) {
		static const DlgTextInfo TextInfosYesNo[] = {
			{ IDOK, "BTN_YES" },
			{ IDCANCEL, "BTN_NO" },
		};
		SetDlgItemTextA(IDOK, "Yes");	// lng �t�@�C���Ȃ��΍�
		SetDlgTextsW(m_hWnd, TextInfosYesNo, _countof(TextInfosYesNo), UILanguageFileW);
	} else {
		static const DlgTextInfo TextInfosOk[] = {
			{ IDOK, "BTN_OK" },
		};
		SetDlgTextsW(m_hWnd, TextInfosOk, _countof(TextInfosOk), UILanguageFileW);
	}
	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), 0);

	SetWindowTextW(TitleStr);
	SetDlgItemTextW(IDC_MSGTEXT,TextStr);
	CalcTextExtentW(GetDlgItem(IDC_MSGTEXT), NULL, TextStr, &s);
	TW = s.cx + s.cx/10;
	TH = s.cy;

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

LRESULT CMsgDlg::OnExitSizeMove(WPARAM wParam, LPARAM lParam)
{
	RECT R;

	GetWindowRect(&R);
		if (R.bottom-R.top == WH && R.right-R.left == WW) {
		// �T�C�Y���ς���Ă��Ȃ���Ή������Ȃ�
	}
	else if (R.bottom-R.top != WH || R.right-R.left < init_WW) {
		// �������ύX���ꂽ���A�ŏ���蕝�������Ȃ����ꍇ�͌��ɖ߂�
		SetWindowPos(HWND_TOP,R.left,R.top,WW,WH,0);
	}
	else {
		// �����łȂ���΍Ĕz�u����
		Relocation(FALSE, R.right-R.left);
	}

	return TRUE;
}

void CMsgDlg::Relocation(BOOL is_init, int new_WW)
{
	RECT R;
	HWND HText, HOk, HNo;
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
		if (YesNoFlag && (TW < 7*BW/2)) {
			TW = 7*BW/2;
		}
		// �E�C���h�E�T�C�Y�̌v�Z
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

	SetDlgPos();

	InvalidateRect(NULL);
}

BOOL CMsgDlg::OnCancel()
{
	if (!YesNoFlag) {
		// ok(yes)�����̂Ƃ��́Acancel�����͉������Ȃ�
		return TRUE;
	} else {
		// yes/no�̂Ƃ��́A�f�t�H���g����(�I��)
		return TTCDialog::OnCancel();
	}
}

// ���b�Z�[�W�{�b�N�X���L�����Z������(close�{�^��������)�ƁA�}�N���̏I���Ƃ���B
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
