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

/* TTMACRO.EXE, main window */

#include "tmfc.h"
#include "ttmmsg.h"
#include "tttypes.h"
/////////////////////////////////////////////////////////////////////////////
// CCtrlWindow dialog

class CCtrlWindow : public TTCDialog
{
public:
	BOOL Pause;
	CCtrlWindow(HINSTANCE hInst);
	int Create();
	BOOL OnIdle();

	enum { IDD = IDD_CTRLWIN };

protected:
	virtual BOOL OnCancel();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL PostNcDestroy();
	virtual BOOL OnInitDialog();
	virtual LRESULT DlgProc(UINT msg, WPARAM wp, LPARAM lp);

protected:
	HICON m_hIcon;
	HFONT DlgFont;

	LONG m_init_width, m_init_height;
	LONG m_filename_ratio, m_lineno_ratio;
	HWND m_hStatus;

	BOOL OnClose();
	void OnDestroy();
	BOOL OnEraseBkgnd(HDC DC);
	void OnPaint();
	void OnSize(UINT nType, int cx, int cy);
	void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	HCURSOR OnQueryDragIcon();
	void OnSysColorChange();
	void OnTimer(UINT_PTR nIDEvent);
	LRESULT OnDdeCmndEnd(WPARAM wParam, LPARAM lParam);
	LRESULT OnDdeComReady(WPARAM wParam, LPARAM lParam);
	LRESULT OnDdeReady(WPARAM wParam, LPARAM lParam);
	LRESULT OnDdeEnd(WPARAM wParam, LPARAM lParam);
	LRESULT OnMacroBringup(WPARAM wParam, LPARAM lParam);
};

