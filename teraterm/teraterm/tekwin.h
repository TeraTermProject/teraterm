/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TEK window */

/////////////////////////////////////////////////////////////////////////////
// CTEKWindow

class CTEKWindow : public CFrameWnd
{
private:
  TTEKVar tk;
#ifndef NO_I18N
  HMENU MainMenu, EditMenu, WinMenu,
    FileMenu, SetupMenu, HelpMenu;
#else
  HMENU MainMenu, EditMenu, WinMenu;
#endif

public:
	CTEKWindow();
	int Parse();
	void RestoreSetup();
	void InitMenu(HMENU *Menu);
	void InitMenuPopup(HMENU SubMenu);

protected:

public:
	//{{AFX_VIRTUAL(CTEKWindow)
	protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CTEKWindow)
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDestroy();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnPaint();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg LONG OnAccelCommand(UINT wParam, LONG lParam);
	afx_msg LONG OnChangeMenu(UINT wParam, LONG lParam);
	afx_msg LONG OnChangeTBar(UINT wParam, LONG lParam);
	afx_msg LONG OnDlgHelp(UINT wParam, LONG lParam);
	afx_msg LONG OnGetSerialNo(UINT wParam, LONG lParam);
	afx_msg void OnFilePrint();
	afx_msg void OnFileExit();
	afx_msg void OnEditCopy();
	afx_msg void OnEditCopyScreen();
	afx_msg void OnEditPaste();
	afx_msg void OnEditPasteCR();
	afx_msg void OnEditClearScreen();
	afx_msg void OnSetupWindow();
	afx_msg void OnSetupFont();
	afx_msg void OnVTWin();
	afx_msg void OnWindowWindow();
	afx_msg void OnHelpIndex();
	afx_msg void OnHelpUsing();
	afx_msg void OnHelpAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
