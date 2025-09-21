/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007- TeraTerm Project
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

/* TERATERM.EXE, print-abort dialog box */
#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "tt_res.h"
#include "prnabort.h"

HWND CPrnAbortDlg::m_hWnd_static;

LRESULT CALLBACK CPrnAbortDlg::OnDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_PRNABORT_PRINTING, "DLG_PRNABORT_PRINTING" },
		{ IDCANCEL, "BTN_CANCEL" },
	};

	switch (msg) {
	case WM_INITDIALOG:
	{
		CPrnAbortDlg *self = (CPrnAbortDlg *)lp;
		SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)self);
		SetDlgTextsW(hDlgWnd, TextInfos, _countof(TextInfos), self->m_ts->UILanguageFileW);
		return TRUE;
	}

	case WM_COMMAND:
	{
		CPrnAbortDlg *self = (CPrnAbortDlg *)GetWindowLongPtr(hDlgWnd, DWLP_USER);
		const WORD wID = GET_WM_COMMAND_ID(wp, lp);
		if (wID == IDOK) {
			self->DestroyWindow();
		}
		if (wID == IDCANCEL) {
			if (self->m_pAbort != NULL) {
				*self->m_pAbort = TRUE;
			}
			self->m_PrintAbortFlag = TRUE;
			self->DestroyWindow();
		}
		return FALSE;
	}

	default:
		return FALSE;
	}
	return TRUE;
}

CPrnAbortDlg::CPrnAbortDlg()
{
	m_PrintAbortFlag = FALSE;
}

/**
 *	ダイアログを表示する
 *
 *	@param	hInstance
 *	@param	hParent
 *	@param	AbortFlag	中断ボタンが押されると TRUE となる(NULLのとき使用しない)
 *	@param	pts
 *	@retval	TRUE		ok
 *	@retval	FALSE		ダイアログ生成失敗(ないはず)
 */
BOOL CPrnAbortDlg::Create(HINSTANCE hInstance, HWND hParent, PBOOL AbortFlag, PTTSet pts)
{
	assert(m_hWnd_static == NULL);
	if (m_hWnd_static != NULL) {
		return FALSE;
	}

	m_pAbort = AbortFlag;
	m_hParentWnd = hParent;
	m_ts = pts;
	if (m_pAbort != NULL) {
		*m_pAbort = FALSE;
	}

	SetDialogFont(m_ts->DialogFontNameW, m_ts->DialogFontPoint, m_ts->DialogFontCharSet,
				  m_ts->UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	HWND hWnd = TTCreateDialogParam(
		hInstance, MAKEINTRESOURCEW(IDD_PRNABORTDLG), hParent,
		OnDlgProc, (LPARAM)this);
	if (hWnd == NULL)
	{
		return FALSE;
	}

	m_hWnd = hWnd;
	m_hWnd_static = hWnd;
	::EnableWindow(hParent,FALSE);
	::ShowWindow(hWnd, SW_SHOW);
	::EnableWindow(m_hWnd,TRUE);
	return TRUE;
}

/**
 *	ダイアログを破棄
 */
BOOL CPrnAbortDlg::DestroyWindow()
{
	::EnableWindow(m_hParentWnd,TRUE);
	::SetFocus(m_hParentWnd);
	::DestroyWindow(m_hWnd);
	m_hWnd_static = NULL;
	return TRUE;
}

/**
 *	印刷用DCをセット
 *	@retval	0以上		成功
 *	@retval	SP_ERROR	失敗
 */
int CPrnAbortDlg::SetPrintDC(HDC hPrintDC)
{
	return ::SetAbortProc(hPrintDC, AbortProcStatic);
}

/**
 *
 *	@retval	TRUE	印刷ジョブを続行
 *	@retval	FALSE	印刷ジョブを取り消す
 */
BOOL CPrnAbortDlg::AbortProc(HDC hDC, int Error)
{
	MSG m;

	// すでに abort が押されている
	if (m_PrintAbortFlag) {
		return FALSE;
	}

	while (PeekMessage(&m, 0,0,0, PM_REMOVE)) {
		if (!IsDialogMessage(m_hWnd, &m)) {
			TranslateMessage(&m);
			DispatchMessage(&m);
		}
		if (m_PrintAbortFlag) {
			break;
		}
	}

	if (m_PrintAbortFlag) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}

BOOL CALLBACK CPrnAbortDlg::AbortProcStatic(HDC hDC, int Error)
{
	CPrnAbortDlg *self = (CPrnAbortDlg *)GetWindowLongPtr(m_hWnd_static, DWLP_USER);
	return self->AbortProc(hDC, Error);
}

/**
 *	Abortボタンが押された?
 *	@retval	TRUE	押された
 *	@retval	FALSE	押されていない
 */
BOOL CPrnAbortDlg::IsAborted()
{
	return m_PrintAbortFlag;
}
