/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, input dialog box */

class CInpDlg : public CDialog
{
public:
	CInpDlg(PCHAR Input, PCHAR Text, PCHAR Title,
	        PCHAR Default, BOOL Paswd, BOOL SPECIAL,
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
