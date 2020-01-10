/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2020 TeraTerm Project
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

/* TERATERM.EXE, file-transfer-protocol dialog box */
#include "teraterm.h"
#include "tt_res.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "protodlg.h"

/////////////////////////////////////////////////////////////////////////////
// CProtoDlg dialog

BOOL CProtoDlg::Create(HINSTANCE hInstance, HWND hParent, PFileVar pfv, PTTSet pts)
{
	m_pts = pts;
	fv = pfv;

	BOOL Ok = TTCDialog::Create(hInstance, hParent, CProtoDlg::IDD);
	fv->HWin = GetSafeHwnd();

	return Ok;
}

/////////////////////////////////////////////////////////////////////////////
// CProtoDlg message handler

BOOL CProtoDlg::OnInitDialog()
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_PROT_FILENAME, "DLG_PROT_FILENAME" },
		{ IDC_PROT_PROT, "DLG_PROT_PROTO" },
		{ IDC_PROT_PACKET, "DLG_PROT_PACKET"},
		{ IDC_PROT_TRANS, "DLG_PROT_TRANS" },
		{ IDC_PROT_ELAPSED, "DLG_PROT_ELAPSED" },
		{ IDCANCEL, "BTN_CANCEL" },
	};
	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), m_pts->UILanguageFile);
	return TRUE;
}


BOOL CProtoDlg::OnCancel()
{
	::PostMessage(fv->HMainWin,WM_USER_PROTOCANCEL,0,0);
	return TRUE;
}

BOOL CProtoDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) {
		case IDCANCEL:
			::PostMessage(fv->HMainWin,WM_USER_PROTOCANCEL,0,0);
			return TRUE;
		default:
			return (TTCDialog::OnCommand(wParam,lParam));
	}
}

BOOL CProtoDlg::PostNcDestroy()
{
	delete this;
	return TRUE;
}
