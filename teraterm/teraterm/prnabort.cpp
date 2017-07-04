/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
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
#include "stdafx.h"
#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "tt_res.h"
#include "prnabort.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CPrnAbortDlg dialog
BEGIN_MESSAGE_MAP(CPrnAbortDlg, CDialog)
	//{{AFX_MSG_MAP(CPrnAbortDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CPrnAbortDlg message handler
BOOL CPrnAbortDlg::Create(CWnd* p_Parent, PBOOL AbortFlag, PTTSet pts)
{
	BOOL Ok;
	HWND HParent;
	LOGFONT logfont;
	HFONT font;

	m_pParent = p_Parent;
	if (p_Parent!=NULL) {
		HParent = p_Parent->GetSafeHwnd();
	}
	else {
		HParent = NULL;
	}
	Abort = AbortFlag;
	Ok = (CDialog::Create(CPrnAbortDlg::IDD, m_pParent));
	if (Ok) {
		::EnableWindow(HParent,FALSE);
		::EnableWindow(GetSafeHwnd(),TRUE);
	}

	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_SYSTEM_FONT", GetSafeHwnd(), &logfont, &DlgFont, pts->UILanguageFile)) {
		SendDlgItemMessage(IDC_PRNABORT_PRINTING, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDCANCEL, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
	}

	return Ok;
}

void CPrnAbortDlg::OnCancel()
{
	*Abort = TRUE;
	DestroyWindow();
}

BOOL CPrnAbortDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	*Abort = TRUE;
	DestroyWindow();

	return CDialog::OnCommand(wParam, lParam);
}

void CPrnAbortDlg::PostNcDestroy()
{
	delete this;
}

BOOL CPrnAbortDlg::DestroyWindow()
{
	HWND HParent;

	HParent = m_pParent->GetSafeHwnd();
	::EnableWindow(HParent,TRUE);
	::SetFocus(HParent);
	return CDialog::DestroyWindow();
}
