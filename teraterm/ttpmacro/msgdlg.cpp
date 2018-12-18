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

#include <windows.h>
#include <windowsx.h>
#include "teraterm.h"
#include "ttlib.h"
#include "ttm_res.h"
#include "ttmlib.h"
#include "tmfc.h"
#include "dlglib.h"
#include "ttmdlg.h"
#include "ttmacro.h"

#include "msgdlg.h"

// CMsgDlg dialog

CMsgDlg::CMsgDlg(const TCHAR *Text, const TCHAR *Title, BOOL YesNo,
                 int x, int y)
{
	TextStr = Text;
	TitleStr = Title;
	YesNoFlag = YesNo;
	PosX = x;
	PosY = y;
}

INT_PTR CMsgDlg::DoModal()
{
	HINSTANCE hInst = GetInstance();
	HWND hWndParent = GetHWND();
	return TTCDialog::DoModal(hInst, hWndParent, CMsgDlg::IDD);
}

BOOL CMsgDlg::OnInitDialog()
{
	static const DlgTextInfo TextInfosOk[] = {
		{ IDOK, "BTN_OK" },
	};
	static const DlgTextInfo TextInfosYesNo[] = {
		{ IDOK, "BTN_YES" },
		{ IDCANCEL, "BTN_NO" },
	};
	RECT R;
	HWND HOk;

	if (YesNoFlag) {
		SetDlgTexts(m_hWnd, TextInfosYesNo, _countof(TextInfosYesNo), UILanguageFile);
	} else {
		SetDlgTexts(m_hWnd, TextInfosOk, _countof(TextInfosOk), UILanguageFile);
	}

	SetWindowText(TitleStr);
	SetDlgItemText(IDC_MSGTEXT,TextStr);
	CalcTextExtent2(GetDlgItem(IDC_STATTEXT), NULL, TextStr, &s);
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
		// サイズが変わっていなければ何もしない
	}
	else if (R.bottom-R.top != WH || R.right-R.left < init_WW) {
		// 高さが変更されたか、最初より幅が狭くなった場合は元に戻す
		SetWindowPos(HWND_TOP,R.left,R.top,WW,WH,0);
	}
	else {
		// そうでなければ再配置する
		Relocation(FALSE, R.right-R.left);
	}

	return TRUE;
}

void CMsgDlg::Relocation(BOOL is_init, int new_WW)
{
	RECT R;
	HDC TmpDC;
	HWND HText, HOk, HNo;
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

	if (PosX<=GetMonitorLeftmost(PosX, PosY)-100) {
		TmpDC = ::GetDC(GetSafeHwnd());
		PosX = (GetDeviceCaps(TmpDC,HORZRES)-WW) / 2;
		PosY = (GetDeviceCaps(TmpDC,VERTRES)-WH) / 2;
		::ReleaseDC(GetSafeHwnd(),TmpDC);
	}
	SetWindowPos(HWND_TOP,PosX,PosY,WW,WH,0);
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
	}
	return FALSE;
}
