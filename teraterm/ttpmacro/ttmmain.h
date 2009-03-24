/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, main window */

#include "ttmmsg.h"
#include "tttypes.h"
/////////////////////////////////////////////////////////////////////////////
// CCtrlWindow dialog

class CCtrlWindow : public CDialog
{
public:
	BOOL Pause;
	CCtrlWindow();
	int Create();
	BOOL OnIdle();

// Dialog Data
	//{{AFX_DATA(CCtrlWindow)
	enum { IDD = IDD_CTRLWIN };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CCtrlWindow)
	protected:
	virtual void OnCancel( );
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

protected:
	HICON m_hIcon;
	HFONT DlgFont;

	//{{AFX_MSG(CCtrlWindow)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSysColorChange();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg LONG OnDdeCmndEnd(UINT wParam, LONG lParam);
	afx_msg LONG OnDdeComReady(UINT wParam, LONG lParam);
	afx_msg LONG OnDdeReady(UINT wParam, LONG lParam);
	afx_msg LONG OnDdeEnd(UINT wParam, LONG lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

typedef CCtrlWindow *PCtrlWindow;
