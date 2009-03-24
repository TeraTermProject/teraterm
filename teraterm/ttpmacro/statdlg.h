/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, status dialog box */

class CStatDlg : public CDialog
{
public:
	BOOL Create(PCHAR Text, PCHAR Title, BOOL SPECIAL, int x, int y);
	void Update(PCHAR Text, PCHAR Title, BOOL SPECIAL, int x, int y);

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
	BOOL _SPECIAL;

	//{{AFX_MSG(CStatDlg)
	virtual BOOL OnInitDialog();
	afx_msg LONG OnExitSizeMove(UINT wParam, LONG lParam);
	//}}AFX_MSG
	void Relocation(BOOL is_init, int WW);
	DECLARE_MESSAGE_MAP()
};

typedef CStatDlg *PStatDlg;
