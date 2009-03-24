/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, file-transfer-protocol dialog box */

// CProtoDlg dialog
class CProtoDlg : public CDialog
{
private:
  PFileVar fv;
#ifndef NO_I18N
	HFONT DlgFont;
#endif

public:
#ifndef NO_I18N
	BOOL Create(PFileVar pfv, PTTSet pts);
#else
	BOOL Create(PFileVar pfv);
#endif

	//{{AFX_DATA(CProtoDlg)
	enum { IDD = IDD_PROTDLG };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CProtoDlg)
	protected:
	virtual void OnCancel( );
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

protected:

	//{{AFX_MSG(CProtoDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

typedef CProtoDlg *PProtoDlg;
