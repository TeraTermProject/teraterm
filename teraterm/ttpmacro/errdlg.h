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

/* TTMACRO.EXE, error dialog box */

// CErrDlg dialog
#include "tmfc.h"
#include "macrodlgbase.h"

class CErrDlg : public CMacroDlgBase
{
public:
	CErrDlg(const char *Msg, const char *Line, int x, int y, int lineno, int start, int end, const char *FileName);
	INT_PTR DoModal(HINSTANCE hInst, HWND hWndParent);

private:
	enum { IDD = IDD_ERRDLG };

	const wchar_t *MsgStr;
	const wchar_t *LineStr;
	int LineNo;
	int StartPos, EndPos;
	const wchar_t *MacroFileName;
	BOOL in_init = FALSE;

	virtual BOOL OnInitDialog();
	virtual LRESULT DlgProc(UINT msg, WPARAM wp, LPARAM lp);
	BOOL OnCommand(WPARAM wp, LPARAM lp);
	void OnBnClickedMacroerrhelp();
};
