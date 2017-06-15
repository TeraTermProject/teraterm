/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2017 TeraTerm Project
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

/* TTMACRO.EXE, input dialog box */

class CInpDlg : public CDialog
{
public:
	CInpDlg(PCHAR Input, PCHAR Text, PCHAR Title,
	        PCHAR Default, BOOL Paswd,
	        int x, int y);

	//{{AFX_DATA(CInpDlg)
	enum { IDD = IDD_INPDLG };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CInpDlg)
	//}}AFX_VIRTUAL

protected:
	PCHAR InputStr, TextStr, TitleStr, DefaultStr;
	BOOL PaswdFlag;
	int PosX, PosY, init_WW, WW, WH, TW, TH, BH, BW, EW, EH;
	SIZE s;
	HFONT DlgFont;

	//{{AFX_MSG(CInpDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg LONG OnExitSizeMove(UINT wParam, LONG lParam);
	//}}AFX_MSG
	void Relocation(BOOL is_init, int WW);
	DECLARE_MESSAGE_MAP()
};
typedef CInpDlg *PInpDlg;
