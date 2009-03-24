/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, error dialog box */

// CErrDlg dialog
class CErrDlg : public CDialog
{
public:
	CErrDlg(PCHAR Msg, PCHAR Line, int x, int y);

	//{{AFX_DATA(CErrDlg)
	enum { IDD = IDD_ERRDLG };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CErrDlg)
	//}}AFX_VIRTUAL

protected:
	PCHAR MsgStr, LineStr;
	int PosX, PosY;
	HFONT DlgFont;

	//{{AFX_MSG(CErrDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

typedef CErrDlg *PErrDlg;
