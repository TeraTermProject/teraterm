/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

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

public:
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
