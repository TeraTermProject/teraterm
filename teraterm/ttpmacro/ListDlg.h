/*
 * Copyright (C) 2013- TeraTerm Project
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
#pragma once

#include "../common/tmfc.h"
#include "macrodlgbase.h"
#include "resize_helper.h"

// CListDlg �_�C�A���O
class CListDlg : public CMacroDlgBase
{
public:
	CListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int x, int y, int ext, int width, int height);
	INT_PTR DoModal(HINSTANCE hInst, HWND hWndParent);
	int m_SelectItem;

private:
	enum { IDD = IDD_LISTDLG };
	const wchar_t *m_Text;
	const wchar_t *m_Caption;
	wchar_t **m_Lists;
	int m_Selected;
	int init_WW, TW, TH, BH, BW, LW, LH;
	SIZE s;
	ReiseDlgHelper_t *ResizeHelper = NULL;
	HINSTANCE m_hInst;
	int m_ext = 0;
	int m_width;
	int m_height;

	void Relocation(BOOL is_init, int WW);
	void InitList(HWND HList);

	virtual BOOL OnInitDialog();
	virtual BOOL OnOK();
	virtual BOOL OnCancel();
	virtual BOOL OnClose();
	LRESULT DlgProc(UINT msg, WPARAM wp, LPARAM lp);
};
