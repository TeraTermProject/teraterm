/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, print-abort dialog box */

// CPrnAbortDlg dialog
class CPrnAbortDlg : public CDialog
{
#ifndef NO_I18N
private:
	HFONT DlgFont;
#endif

public:
#ifndef NO_I18N
	BOOL Create(CWnd* p_Parent, PBOOL AbortFlag, PTTSet pts);
#else
	BOOL Create(CWnd* p_Parent, PBOOL AbortFlag);
#endif

	//{{AFX_DATA(CPrnAbortDlg)
	enum { IDD = IDD_PRNABORTDLG };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CPrnAbortDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void OnCancel( );
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	protected:
	CWnd* m_pParent;
	BOOL *Abort;

	//{{AFX_MSG(CPrnAbortDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
