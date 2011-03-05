/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, main */

/////////////////////////////////////////////////////////////////////////////
// CCtrlApp:

class CCtrlApp : public CWinApp
{
public:
	CCtrlApp();
	BOOL Busy;

	//{{AFX_VIRTUAL(CCtrlApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CCtrlApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
