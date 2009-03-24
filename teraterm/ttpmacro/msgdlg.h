/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, message dialog box */

class CMsgDlg : public CDialog
{
public:
	CMsgDlg(PCHAR Text, PCHAR Title,
	        BOOL YesNo, BOOL SPECIAL,
	        int x, int y);

	//{{AFX_DATA(CMsgDlg)
	enum { IDD = IDD_MSGDLG };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CMsgDlg)
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

protected:
	PCHAR TextStr, TitleStr;
	BOOL YesNoFlag;
	int  PosX, PosY, init_WW, WW, WH, TW, TH, BH, BW;
	SIZE s;
	HFONT DlgFont;

	//{{AFX_MSG(CMsgDlg)
	virtual BOOL OnInitDialog();
	afx_msg LONG OnExitSizeMove(UINT wParam, LONG lParam);
	//}}AFX_MSG
	void Relocation(BOOL is_init, int WW);
	DECLARE_MESSAGE_MAP()
};
