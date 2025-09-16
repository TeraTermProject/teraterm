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

/* TTMACRO.EXE, status dialog box */

#include <assert.h>
#include <crtdbg.h>
#include "compat_win.h"
#include "teraterm.h"
#include "ttlib.h"
#include "ttm_res.h"
#include "ttmlib.h"
#include "tmfc.h"
#include "tttypes.h"	// for WM_USER_MSTATBRINGUP
#include "ttmacro.h"
#include "dlglib.h"
#include "ttmdlg.h"

#include "statdlg.h"

// CStatDlg dialog

BOOL CStatDlg::Create(HINSTANCE hInst, const wchar_t *Text, const wchar_t *Title, int x, int y)
{
	wcscpy_s(TextStr, MaxStrLen, Text);
	TitleStr = Title;
	PosX = x;
	PosY = y;
	m_hInst = hInst;
	return TTCDialog::Create(hInst, NULL, CStatDlg::IDD);
}

void CStatDlg::Update(const wchar_t *Text, const wchar_t *Title, int x, int y)
{
	if (Title!=NULL) {
		SetWindowTextW(Title);
		TitleStr = Title;
	}

	if (Text!=NULL) {
		HWND hWnd = GetDlgItem(IDC_STATTEXT);
		CalcTextExtentW(hWnd, NULL, Text, &s);
		TW = s.cx + (int)(16 * dpi / 96.f);
		TH = s.cy + (int)(16 * dpi / 96.f);

		SetDlgItemTextW(IDC_STATTEXT,Text);
		wcscpy_s(TextStr, MaxStrLen, Text);
	}

	if (x!=32767) {
		PosX = x;
		PosY = y;
	}

	in_update = TRUE;
	Relocation(TRUE, 0, 0);
	in_update = FALSE;
}

// CStatDlg message handler

BOOL CStatDlg::OnInitDialog()
{
	dpi = GetMonitorDpiFromWindow(m_hWnd);
	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), dpi);
	// 閉じるボタンを無効化
	RemoveMenu(GetSystemMenu(m_hWnd, FALSE), SC_CLOSE, MF_BYCOMMAND);

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

void CStatDlg::Relocation(BOOL is_init, int new_WW, int new_WH)
{
	RECT R;
	int CW, CH;

	if (TextStr != NULL) {
		GetClientRect(&R);
		CW = R.right-R.left;
		CH = R.bottom-R.top;
		// 初回のみ
		if (is_init) {
			// ウインドウサイズの計算
			GetWindowRect(&R);
			WW = TW + R.right - R.left - CW;
			WH = TH + R.bottom - R.top - CH;
			// 実際のサイズを取得
			::SetWindowPos(m_hWnd, HWND_TOP, 0, 0, WW, WH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			GetWindowRect(&R);
			WW = R.right - R.left;
			WH = R.bottom - R.top;
			init_WW = WW;
			init_WH = WH;
			// CW,CHに反映
			GetClientRect(&R);
			CW = R.right-R.left;
			CH = R.bottom-R.top;
		} else {
			WW = new_WW;
			WH = new_WH;
		}
		::MoveWindow(GetDlgItem(IDC_STATTEXT), (CW - s.cx) / 2, (CH - s.cy) / 2, TW, TH, TRUE);
	}

	if (is_init) {
		if (SetDlgPosEX(GetSafeHwnd(), WW, WH, &PosX, &PosY) < 0) {
			SetDlgPos();
		}
	}

	InvalidateRect(NULL, TRUE);
}

void CStatDlg::Bringup()
{
	BringupWindow(m_hWnd);
}

/**
 * MFCのCWndの隠れメンバ関数
 *	この関数がFALSEを返すと
 *	CDialog::OnInitDialog()後に
 *	CWnd::CenterWindow() が呼び出されない
 */
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
	case WM_EXITSIZEMOVE:
		return OnExitSizeMove(wp, lp);
	case WM_DPICHANGED:
		int new_dpi, new_WW, new_WH, new_CW, new_CH;
		float mag;
		RECT R;

		new_dpi = HIWORD(wp);
		mag = new_dpi / (float)dpi;
		dpi = new_dpi;
		CalcTextExtentW(GetDlgItem(IDC_STATTEXT), NULL, TextStr, &s);
		TW = s.cx + (int)(16 * dpi / 96.f);
		TH = s.cy + (int)(16 * dpi / 96.f);
		R = *(RECT *)lp;
		new_WW = R.right - R.left;
		new_WH = R.bottom - R.top;
		GetClientRect(&R);
		new_CW = R.right-R.left;
		new_CH = R.bottom-R.top;
		init_WW = TW + new_WW - new_CW;
		init_WH = TH + new_WH - new_CH;

		TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), dpi);
		Relocation(in_update, WW, WH);
		return TRUE;
	}
	return FALSE;
}
