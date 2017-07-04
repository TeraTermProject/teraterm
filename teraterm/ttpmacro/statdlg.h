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

/* TTMACRO.EXE, status dialog box */

class CStatDlg : public CDialog
{
public:
	BOOL Create(PCHAR Text, PCHAR Title, int x, int y);
	void Update(PCHAR Text, PCHAR Title, int x, int y);
	void Bringup();
	virtual BOOL CheckAutoCenter();

	//{{AFX_DATA(CStatDlg)
	enum { IDD = IDD_STATDLG };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CStatDlg)
	protected:
	virtual void OnCancel( );
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

protected:
	PCHAR TextStr, TitleStr;
	int  PosX, PosY, init_WW, WW, WH, TW, TH;
	SIZE s;
	HFONT DlgFont;

	//{{AFX_MSG(CStatDlg)
	virtual BOOL OnInitDialog();
	afx_msg LONG OnExitSizeMove(UINT wParam, LONG lParam);
	afx_msg LONG OnSetForceForegroundWindow(UINT wParam, LONG lParam);
	//}}AFX_MSG
	void Relocation(BOOL is_init, int WW);
	DECLARE_MESSAGE_MAP()
};

typedef CStatDlg *PStatDlg;
