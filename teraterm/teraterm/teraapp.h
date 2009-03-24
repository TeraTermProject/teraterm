/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, Main application class */

class CTeraApp : public CWinApp
{
public:
	CTeraApp();

	//{{AFX_VIRTUAL(CTeraApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CTeraApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
