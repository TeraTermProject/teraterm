/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

/* TTMACRO.EXE, error dialog box */

#include "stdafx.h"
#include "teraterm.h"
#include "ttlib.h"
#include "ttm_res.h"

#include "tttypes.h"
#include "ttcommon.h"
#include "helpid.h"

#include "errdlg.h"
#include "ttmlib.h"
#include "ttmparse.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CErrDlg dialog
CErrDlg::CErrDlg(PCHAR Msg, PCHAR Line, int x, int y, int lineno, int start, int end, PCHAR FileName)
	: CDialog(CErrDlg::IDD)
{
	//{{AFX_DATA_INIT(CErrDlg)
	//}}AFX_DATA_INIT
	MsgStr = Msg;
	LineStr = Line;
	PosX = x;
	PosY = y;
	LineNo = lineno;
	StartPos = start;
	EndPos = end;
	MacroFileName = FileName;
}

BEGIN_MESSAGE_MAP(CErrDlg, CDialog)
	//{{AFX_MSG_MAP(CErrDlg)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_MACROERRHELP, &CErrDlg::OnBnClickedMacroerrhelp)
END_MESSAGE_MAP()

// CErrDlg message handler

BOOL CErrDlg::OnInitDialog()
{
	RECT R;
	HDC TmpDC;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;
	char buf[MaxLineLen*2], buf2[10];
	int i, len;

	CDialog::OnInitDialog();
	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_SYSTEM_FONT", m_hWnd, &logfont, &DlgFont, UILanguageFile)) {
		SendDlgItemMessage(IDC_ERRMSG, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_ERRLINE, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDOK, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDCANCEL, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
	}

	GetDlgItemText(IDOK, uimsg2, sizeof(uimsg2));
	get_lang_msg("BTN_STOP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
	SetDlgItemText(IDOK, uimsg);
	GetDlgItemText(IDCANCEL, uimsg2, sizeof(uimsg2));
	get_lang_msg("BTN_CONTINUE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
	SetDlgItemText(IDCANCEL, uimsg);
	GetDlgItemText(IDC_MACROERRHELP, uimsg2, sizeof(uimsg2));
	get_lang_msg("BTN_HELP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
	SetDlgItemText(IDC_MACROERRHELP, uimsg);

	SetDlgItemText(IDC_ERRMSG,MsgStr);

	// 行番号を先頭につける。
	// ファイル名もつける。
	// エラー箇所に印をつける。
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s:%d:", MacroFileName, LineNo);
	SetDlgItemText(IDC_ERRLINE, buf);

	len = strlen(LineStr);
	buf[0] = 0;
	for (i = 0 ; i < len ; i++) {
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

	if (PosX<=GetMonitorLeftmost(PosX, PosY)-100) {
		GetWindowRect(&R);
		TmpDC = ::GetDC(GetSafeHwnd());
		PosX = (GetDeviceCaps(TmpDC,HORZRES)-R.right+R.left) / 2;
		PosY = (GetDeviceCaps(TmpDC,VERTRES)-R.bottom+R.top) / 2;
		::ReleaseDC(GetSafeHwnd(),TmpDC);
	}
	SetWindowPos(&wndTop,PosX,PosY,0,0,SWP_NOSIZE);
	SetForegroundWindow();

	return TRUE;
}

void CErrDlg::OnBnClickedMacroerrhelp()
{
	OpenHelp(HH_HELP_CONTEXT, HlpMacroAppendixesError, UILanguageFile);
}
