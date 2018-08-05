/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2018 TeraTerm Project
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
#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "tt_res.h"
#include "prnabort.h"

LRESULT CALLBACK CPrnAbortDlg::OnDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_PRNABORT_PRINTING, "DLG_PRNABORT_PRINTING" }, 
		{ IDCANCEL, "BTN_CANCEL" },
	};
	static const int FontIDs[] = {
		IDC_PRNABORT_PRINTING, IDCANCEL
	};

	CPrnAbortDlg *self = (CPrnAbortDlg *)GetWindowLongPtr(hDlgWnd, DWLP_USER);

	switch (msg) {
	case WM_INITDIALOG:
	{
		CPrnAbortDlg *self = (CPrnAbortDlg *)lp;
		SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)self);
		SetDlgTexts(hDlgWnd, TextInfos, _countof(TextInfos), self->m_ts->UILanguageFile);
		self->m_hNewFont =
			SetDlgFonts(hDlgWnd, FontIDs, _countof(FontIDs),
						self->m_ts->UILanguageFile, "DLG_SYSTEM_FONT");
		return TRUE;
	}

	case WM_COMMAND:
	{
		WORD wID = GET_WM_COMMAND_ID(wp, lp);
		const WORD wCMD = GET_WM_COMMAND_CMD(wp, lp);
		if (wID == IDOK) {
			self->DestroyWindow();
		}
		if (wID == IDCANCEL) {
			self->OnCancel();
		}
		return FALSE;
	}
	case WM_NCDESTROY:
		self->PostNcDestroy();
		return TRUE;

	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CPrnAbortDlg::Create(HINSTANCE hInstance, HWND hParent, PBOOL AbortFlag, PTTSet pts)
{
	m_pAbort = AbortFlag;
	m_hParentWnd = hParent;
	m_ts = pts;

	HRSRC hResource = ::FindResource(hInstance, MAKEINTRESOURCE(IDD_PRNABORTDLG), RT_DIALOG);
	HANDLE hDlgTemplate = ::LoadResource(hInstance, hResource);
	DLGTEMPLATE *lpTemplate = (DLGTEMPLATE *)::LockResource(hDlgTemplate);
	HWND hWnd = ::CreateDialogIndirectParam(	
		hInstance, lpTemplate, hParent,
		(DLGPROC)OnDlgProc, (LPARAM)this);
	if (hWnd == NULL)
	{
		return FALSE;
	}

	m_hWnd = hWnd;
	::EnableWindow(hParent,FALSE);
	::ShowWindow(hWnd, SW_SHOW);
	::EnableWindow(m_hWnd,TRUE);
	return TRUE;
}

void CPrnAbortDlg::OnCancel()
{
	*m_pAbort = TRUE;
	DestroyWindow();
}

void CPrnAbortDlg::PostNcDestroy()
{
	::DeleteObject(m_hNewFont);
	delete this;
}

BOOL CPrnAbortDlg::DestroyWindow()
{
	::EnableWindow(m_hParentWnd,TRUE);
	::SetFocus(m_hParentWnd);
	::DestroyWindow(m_hWnd);
	return TRUE;
}

