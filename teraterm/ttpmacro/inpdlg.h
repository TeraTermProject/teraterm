/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2020 TeraTerm Project
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

/* TTMACRO.EXE, input dialog box */

#pragma once

#include "../common/tmfc.h"
#include "macrodlgbase.h"

class CInpDlg : public CMacroDlgBase
{
public:
	CInpDlg(wchar_t *Input, const wchar_t *Text, const wchar_t *Title,
	        const wchar_t *Default, BOOL Paswd,
	        int x, int y);
	INT_PTR DoModal(HINSTANCE hInst, HWND hWndParent);

private:
	enum { IDD = IDD_INPDLG };

	wchar_t *InputStr;
	const wchar_t *TextStr;
	const wchar_t *TitleStr;
	const wchar_t *DefaultStr;
	BOOL PaswdFlag;
	int init_WW, TW, TH, BH, BW, EW, EH;
	SIZE s;

	virtual BOOL OnInitDialog();
	virtual BOOL OnOK();
	virtual LRESULT DlgProc(UINT msg, WPARAM wp, LPARAM lp);
	LRESULT OnExitSizeMove(WPARAM wp, LPARAM lp);
 	void Relocation(BOOL is_init, int WW);
};

