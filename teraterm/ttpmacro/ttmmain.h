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

/* TTMACRO.EXE, main window */

#include "ttmmsg.h"
#include "tttypes.h"
/////////////////////////////////////////////////////////////////////////////
// CCtrlWindow dialog

class CCtrlWindow : public CDialog
{
public:
	BOOL Pause;
	CCtrlWindow();
	int Create();
	BOOL OnIdle();

// Dialog Data
	//{{AFX_DATA(CCtrlWindow)
	enum { IDD = IDD_CTRLWIN };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CCtrlWindow)
	protected:
	virtual void OnCancel( );
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

protected:
	HICON m_hIcon;
	HFONT DlgFont;

	LONG m_init_width, m_init_height;
	LONG m_filename_ratio, m_lineno_ratio;
	HWND m_hStatus;

	//{{AFX_MSG(CCtrlWindow)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSysColorChange();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg LONG OnDdeCmndEnd(UINT wParam, LONG lParam);
	afx_msg LONG OnDdeComReady(UINT wParam, LONG lParam);
	afx_msg LONG OnDdeReady(UINT wParam, LONG lParam);
	afx_msg LONG OnDdeEnd(UINT wParam, LONG lParam);
	afx_msg LONG OnMacroBringup(UINT wParam, LONG lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

typedef CCtrlWindow *PCtrlWindow;
