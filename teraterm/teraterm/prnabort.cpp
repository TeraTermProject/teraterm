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
// シンプルな中断ダイアログ

#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include "ttlib.h"
#include "dlglib.h"
#include "tt_res.h"
#include "prnabort.h"

INT_PTR CALLBACK CPrnAbortDlg::OnDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
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
		SetDlgTextsW(hDlgWnd, TextInfos, _countof(TextInfos), self->m_UILanguageFileW);
		return TRUE;
	}

	case WM_COMMAND:
	{
		CPrnAbortDlg *self = (CPrnAbortDlg *)GetWindowLongPtr(hDlgWnd, DWLP_USER);
		const WORD wID = GET_WM_COMMAND_ID(wp, lp);
		switch (wID) {
		case IDOK:
			return TRUE;
		case IDCANCEL:
			self->m_AbortFlag = TRUE;
			return TRUE;
		}
		return FALSE;
	}

	default:
		return FALSE;
	}
}

CPrnAbortDlg::CPrnAbortDlg()
{
	m_AbortFlag = FALSE;
}

/**
 *	ダイアログを生成して表示する
 *
 *	@param	hInstance
 *	@param	hParent
 *	@param	AbortFlag	中断ボタンが押されると TRUE となる(NULLのとき使用しない)
 *						IsAborted()でもチェック可
 *	@param	pts
 *	@retval	TRUE		ok
 *	@retval	FALSE		ダイアログ生成失敗(発生しないはず)
 */
BOOL CPrnAbortDlg::Create(HINSTANCE hInstance, HWND hParent, wchar_t *UILanguageFileW)
{
	m_hParentWnd = hParent;
	m_UILanguageFileW = _wcsdup(UILanguageFileW);

	HWND hWnd = TTCreateDialogParam(
		hInstance, MAKEINTRESOURCEW(IDD_PRNABORTDLG), hParent,
		OnDlgProc, (LPARAM)this);
	if (hWnd == NULL) {
		free(m_UILanguageFileW);
		return FALSE;
	}
	CenterWindow(hWnd, ::GetParent(hWnd));

	m_hWnd = hWnd;
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
	assert(m_hWnd != NULL);
	::EnableWindow(m_hParentWnd,TRUE);
	::SetFocus(m_hParentWnd);
	::DestroyWindow(m_hWnd);
	m_hWnd = NULL;
	free(m_UILanguageFileW);
	return TRUE;
}

/**
 *	Windowsメッセージの処理を行う
 *
 *	"Cancel(中断)" ボタンが押されたら後の処理は不要
 */
void CPrnAbortDlg::MessagePump()
{
	// すでに abort が押されている
	if (m_AbortFlag) {
		// メッセージの処理不要
		return;
	}

	MSG m;
	while (PeekMessage(&m, 0,0,0, PM_REMOVE)) {
		if (!IsDialogMessage(m_hWnd, &m)) {
			TranslateMessage(&m);
			DispatchMessage(&m);
		}
		if (m_AbortFlag) {
			break;
		}
	}
}

/**
 *	Abortボタンが押された?
 *	@retval	TRUE	押された
 *	@retval	FALSE	押されていない
 */
BOOL CPrnAbortDlg::IsAborted()
{
	return m_AbortFlag;
}
