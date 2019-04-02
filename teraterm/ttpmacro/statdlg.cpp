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

/* TTMACRO.EXE, status dialog box */

#include <assert.h>
#include <crtdbg.h>
#include "teraterm.h"
#include "ttlib.h"
#include "ttm_res.h"
#include "ttmlib.h"
#include "tmfc.h"
#include "tttypes.h"
#include "ttmacro.h"

#include "statdlg.h"

// CStatDlg dialog

BOOL CStatDlg::Create(PCHAR Text, PCHAR Title, int x, int y)
{
	TextStr = Text;
	TitleStr = Title;
	PosX = x;
	PosY = y;
	HINSTANCE hInst = GetInstance();
	return TTCDialog::Create(hInst, GetDesktopWindow(), CStatDlg::IDD);
}

void CStatDlg::Update(PCHAR Text, PCHAR Title, int x, int y)
{
	RECT R;

	if (Title!=NULL) {
		SetWindowText(Title);
		TitleStr = Title;
	}

	GetWindowRect(&R);
	PosX = R.left;
	PosY = R.top;
	WW = R.right-R.left;
	WH = R.bottom-R.top;

	if (Text!=NULL) {
		SIZE textSize;
		HWND hWnd = GetDlgItem(IDC_STATTEXT);
		CalcTextExtent(hWnd, NULL, Text, &textSize);
		TW = textSize.cx + textSize.cx/10;	// (cx * (1+0.1)) ?
		TH = textSize.cy;
		s = textSize;			// TODO s!?

		SetDlgItemText(IDC_STATTEXT,Text);
		TextStr = Text;
	}

	if (x!=32767) {
		PosX = x;
		PosY = y;
	}

	Relocation(TRUE, WW);
}

// CStatDlg message handler

BOOL CStatDlg::OnInitDialog()
{

	Update(TextStr,TitleStr,PosX,PosY);
	SetForegroundWindow(m_hWnd);
	return TRUE;
}

BOOL CStatDlg::OnOK()
{
	return TRUE;
}

BOOL CStatDlg::OnCancel()
{
	DestroyWindow();
	return TRUE;
}

BOOL CStatDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) {
	case IDOK:
		// Enter key押下で消えないようにする。(2010.8.25 yutaka)
		return TRUE;
	case IDCANCEL:
		if ((HWND)lParam!=NULL) { // ignore ESC key
			DestroyWindow();
		}
		return TRUE;
	default:
		return FALSE;
	}
}

BOOL CStatDlg::PostNcDestroy()
{
	delete this;
	return TRUE;
}

LRESULT CStatDlg::OnExitSizeMove(WPARAM wParam, LPARAM lParam)
{
	RECT R;

	::GetWindowRect(m_hWnd, &R);
	if (R.bottom-R.top == WH && R.right-R.left == WW) {
		// サイズが変わっていなければ何もしない
	}
	else if (R.bottom-R.top != WH || R.right-R.left < init_WW) {
		// 高さが変更されたか、最初より幅が狭くなった場合は元に戻す
		::SetWindowPos(m_hWnd, HWND_TOP, R.left,R.top,WW,WH,0);
	}
	else {
		// そうでなければ再配置する
		Relocation(FALSE, R.right-R.left);
	}

	return TRUE;
}

LRESULT CStatDlg::OnSetForceForegroundWindow(WPARAM wParam, LPARAM lParam)
{
	DWORD pid;
	DWORD targetid;
	DWORD currentActiveThreadId;
	HWND hwnd = (HWND)wParam;

	targetid = GetWindowThreadProcessId(hwnd, &pid);
	currentActiveThreadId = GetWindowThreadProcessId(::GetForegroundWindow(), &pid);

	::SetForegroundWindow(m_hWnd);
	if (targetid == currentActiveThreadId) {
		BringWindowToTop(m_hWnd);
	} else {
		AttachThreadInput(targetid, currentActiveThreadId, TRUE);
		BringWindowToTop(m_hWnd);
		AttachThreadInput(targetid, currentActiveThreadId, FALSE);
	}

	return TRUE;
}

void CStatDlg::Relocation(BOOL is_init, int new_WW)
{
	RECT R;
	HDC TmpDC;
	HWND HText;
	int CW, CH;

	if (TextStr != NULL) {
		HText = GetDlgItem(IDC_STATTEXT);

		GetClientRect(&R);
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
			WH = TH + 10 + (WH - CH);
			init_WW = WW;
		}
		else {
			TW = CW;
			WW = new_WW;
		}

		::MoveWindow(HText,(TW-s.cx)/2,5,TW,TH,TRUE);
	}

	if (PosX<=GetMonitorLeftmost(PosX, PosY)-100) {
		TmpDC = ::GetDC(GetSafeHwnd());
		PosX = (GetDeviceCaps(TmpDC,HORZRES)-WW) / 2;
		PosY = (GetDeviceCaps(TmpDC,VERTRES)-WH) / 2;
		::ReleaseDC(GetSafeHwnd(),TmpDC);
	}
	SetWindowPos(HWND_TOP,PosX,PosY,WW,WH,SWP_NOZORDER);

	InvalidateRect(NULL, TRUE);
}

void CStatDlg::Bringup()
{
	BringupWindow(m_hWnd);
}

BOOL CStatDlg::CheckAutoCenter()
{
	// CenterWindow() is called when x=0 && y=0
	// Don't call CenterWindow()
	return FALSE;
}

LRESULT CStatDlg::DlgProc(UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg) {
	case WM_USER_MSTATBRINGUP:
		return OnSetForceForegroundWindow(wp, lp);
	case WM_EXITSIZEMOVE :
		return OnExitSizeMove(wp, lp);
	}
	return FALSE;
}

