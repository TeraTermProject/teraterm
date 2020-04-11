/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2019 TeraTerm Project
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

/* TTMACRO.EXE, error dialog box */

#include <windowsx.h>
#include <stdio.h>
#include "tmfc.h"
#include "teraterm.h"
#include "ttlib.h"
#include "ttm_res.h"
#include "tttypes.h"
#include "ttcommon.h"
#include "helpid.h"
#include "ttmlib.h"
#include "ttmparse.h"
#include "htmlhelp.h"
#include "dlglib.h"
#include "ttmacro.h"
#include "codeconv.h"

#include "errdlg.h"

// CErrDlg dialog

CErrDlg::CErrDlg(const char *Msg, const char *Line, int x, int y, int lineno, int start, int end, const char *FileName)
{
	MsgStr = _tcsdup((tc)Msg);
	LineStr = _tcsdup((tc)Line);
	PosX = x;
	PosY = y;
	LineNo = lineno;
	StartPos = start;
	EndPos = end;
	MacroFileName = _tcsdup((tc)FileName);
}

INT_PTR CErrDlg::DoModal(HINSTANCE hInst, HWND hWndParent)
{
	return TTCDialog::DoModal(hInst, hWndParent, CErrDlg::IDD);
}

BOOL CErrDlg::OnInitDialog()
{
	static const DlgTextInfo TextInfos[] = {
		{ IDOK, "BTN_STOP" },
		{ IDCANCEL, "BTN_CONTINUE" },
		{ IDC_MACROERRHELP, "BTN_HELP" },
	};
	char buf[MaxLineLen*2], buf2[10];

	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFile);

	SetDlgItemText(IDC_ERRMSG,MsgStr);

	// 行番号を先頭につける。
	// ファイル名もつける。
	// エラー箇所に印をつける。
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s:%d:", MacroFileName, LineNo);
	SetDlgItemText(IDC_ERRLINE, buf);

	size_t len = strlen(LineStr);
	buf[0] = 0;
	for (size_t i = 0 ; i < len ; i++) {
		if (i == StartPos)
			strncat_s(buf, sizeof(buf), "<<<", _TRUNCATE);
		if (i == EndPos)
			strncat_s(buf, sizeof(buf), ">>>", _TRUNCATE);
		buf2[0] = LineStr[i];
		buf2[1] = 0;
		strncat_s(buf, sizeof(buf), buf2, _TRUNCATE);
	}
	if (EndPos == len)
		strncat_s(buf, sizeof(buf), ">>>", _TRUNCATE);
	SetDlgItemText(IDC_EDIT_ERRLINE, buf);

	SetDlgPos();

	::SetForegroundWindow(m_hWnd);

	return TRUE;
}

void CErrDlg::OnBnClickedMacroerrhelp()
{
	OpenHelp(HH_HELP_CONTEXT, HlpMacroAppendixesError, UILanguageFile);
}

BOOL CErrDlg::OnCommand(WPARAM wp, LPARAM lp)
{
	const WORD wID = GET_WM_COMMAND_ID(wp, lp);
	if (wID == IDC_MACROERRHELP) {
		OnBnClickedMacroerrhelp();
		return TRUE;
	}
	return FALSE;
}
