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

/* TERATERM.EXE, file transfer dialog box */

/////////////////////////////////////////////////////////////////////////////
// CFileTransDlg dialog

class CFileTransDlg : public CDialog
{
private:
	PFileVar fv;
	PComVar cv;
	BOOL Pause;
#ifndef NO_I18N
	PTTSet ts;
	HFONT DlgFont;
#endif
	HANDLE SmallIcon;
	HANDLE BigIcon;

public:
	CFileTransDlg() {
		DlgFont = NULL;
		SmallIcon = NULL;
		BigIcon = NULL;
	}

#ifndef NO_I18N
	BOOL Create(PFileVar pfv, PComVar pcv, PTTSet pts);
#else
	BOOL Create(PFileVar pfv, PComVar pcv);
#endif
	void ChangeButton(BOOL PauseFlag);
	void RefreshNum();

	//{{AFX_DATA(CFileTransDlg)
	enum { IDD = IDD_FILETRANSDLG };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CFileTransDlg)
	protected:
	virtual void OnCancel( );
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

protected:

	//{{AFX_MSG(CFileTransDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

typedef CFileTransDlg *PFileTransDlg;
