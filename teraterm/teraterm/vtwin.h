/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004-2019 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* TERATERM.EXE, VT window */

#ifdef __cplusplus

class CVTWindow : public CFrameWnd
{
private:
  BOOL FirstPaint, Minimized;

  /* mouse status */
  BOOL LButton, MButton, RButton;
  BOOL DblClk, AfterDblClk, TplClk;
  int DblClkX, DblClkY;

  // "Hold" key status
  BOOL Hold;

  // ScrollLock key
  BOOL ScrollLock;

  HMENU MainMenu, FileMenu, TransMenu, EditMenu,
    SetupMenu, ControlMenu, WinMenu, HelpMenu;

  // drag and drop handle
  char **DropLists;
  int DropListCount;
  void DropListFree();
  bool DropWithLeftbutton;
  bool DropWithRightbutton;

  // window attribute
  BYTE Alpha;
  void SetWindowAlpha(BYTE alpha);

protected:

public:
	CVTWindow();
	int Parse();
	void ButtonUp(BOOL Paste);
	void ButtonDown(POINT p, int LMR);
	void InitMenu(HMENU *Menu);
	void InitMenuPopup(HMENU SubMenu);
	void InitPasteMenu(HMENU *Menu);
	void ResetSetup();
	void RestoreSetup();
	void SetupTerm();
	void Startup();
	void OpenTEK();

	//{{AFX_VIRTUAL(CVTWindow)
	protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	//{{AFX_MSG(CVTWindow)
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnClose();
	afx_msg void OnAllClose();
	afx_msg void OnDestroy();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO *lpMMI);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMove(int x, int y);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnNcCalcSize(BOOL valid, NCCALCSIZE_PARAMS *calcsize);
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg void OnNcRButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysColorChange();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
//<!--by AKASI
	afx_msg LONG OnWindowPosChanging(UINT wParam, LONG lParam);
	afx_msg LONG OnSettingChange(UINT wParam, LONG lParam);
	afx_msg LONG OnEnterSizeMove(UINT wParam, LONG lParam);
	afx_msg LONG  OnExitSizeMove(UINT wParam, LONG lParam);
//-->
	afx_msg LRESULT OnIMEStartComposition(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIMEEndComposition(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnIMEComposition(UINT wParam, LONG lParam);
	afx_msg LONG OnIMEInputChange(UINT wParam, LONG lParam);
	afx_msg LONG OnIMENotify(UINT wParam, LONG lParam);
	afx_msg LONG OnIMERequest(UINT wParam, LONG lParam);
	afx_msg LONG OnAccelCommand(UINT wParam, LONG lParam);
	afx_msg LONG OnChangeMenu(UINT wParam, LONG lParam);
	afx_msg LONG OnChangeTBar(UINT wParam, LONG lParam);
	afx_msg LONG OnCommNotify(UINT wParam, LONG lParam);
	afx_msg LONG OnCommOpen(UINT wParam, LONG lParam);
	afx_msg LONG OnCommStart(UINT wParam, LONG lParam);
	afx_msg LONG OnDdeEnd(UINT wParam, LONG lParam);
	afx_msg LONG OnDlgHelp(UINT wParam, LONG lParam);
	afx_msg LONG OnFileTransEnd(UINT wParam, LONG lParam);
	afx_msg LONG OnGetSerialNo(UINT wParam, LONG lParam);
	afx_msg LONG OnKeyCode(UINT wParam, LONG lParam);
	afx_msg LONG OnProtoEnd(UINT wParam, LONG lParam);
	afx_msg LONG OnChangeTitle(UINT wParam, LONG lParam);
	afx_msg LONG OnReceiveIpcMessage(UINT wParam, LONG lParam);
	afx_msg LONG OnNonConfirmClose(UINT wParam, LONG lParam);
	afx_msg LONG OnNotifyIcon(UINT wParam, LONG lParam);
	afx_msg void OnFileNewConnection();
	afx_msg void OnDuplicateSession();
	afx_msg void OnCygwinConnection();
	afx_msg void OnTTMenuLaunch();
	afx_msg void OnLogMeInLaunch();
	afx_msg void OnFileLog();
	afx_msg void OnCommentToLog();
	afx_msg void OnViewLog();
	afx_msg void OnShowLogDialog();
	afx_msg void OnReplayLog();
	afx_msg void OnExternalSetup();
	afx_msg void OnFileSend();
	afx_msg void OnFileKermitRcv();
	afx_msg void OnFileKermitGet();
	afx_msg void OnFileKermitSend();
	afx_msg void OnFileKermitFinish();
	afx_msg void OnFileXRcv();
	afx_msg void OnFileXSend();
	afx_msg void OnFileYRcv();
	afx_msg void OnFileYSend();
	afx_msg void OnFileZRcv();
	afx_msg void OnFileZSend();
	afx_msg void OnFileBPRcv();
	afx_msg void OnFileBPSend();
	afx_msg void OnFileQVRcv();
	afx_msg void OnFileQVSend();
	afx_msg void OnFileChangeDir();
	afx_msg void OnFilePrint();
	afx_msg void OnFileDisconnect();
	afx_msg void OnFileExit();
	afx_msg void OnEditCopy();
	afx_msg void OnEditCopyTable();
	afx_msg void OnEditPaste();
	afx_msg void OnEditPasteCR();
	afx_msg void OnEditClearScreen();
	afx_msg void OnEditClearBuffer();
	afx_msg void OnEditCancelSelection();
	afx_msg void OnEditSelectScreenBuffer();
	afx_msg void OnEditSelectAllBuffer();
	afx_msg void OnSetupTerminal();
	afx_msg void OnSetupWindow();
	afx_msg void OnSetupFont();
	afx_msg void OnSetupKeyboard();
	afx_msg void OnSetupSerialPort();
	afx_msg void OnSetupTCPIP();
	afx_msg void OnSetupGeneral();
	afx_msg void OnSetupSave();
	afx_msg void OnSetupRestore();
	afx_msg void OnOpenSetupDirectory();
	afx_msg void OnSetupLoadKeyMap();
	afx_msg void OnControlResetTerminal();
	afx_msg void OnControlResetRemoteTitle();
	afx_msg void OnControlBroadcastCommand();
	afx_msg void OnControlAreYouThere();
	afx_msg void OnControlSendBreak();
	afx_msg void OnControlResetPort();
	afx_msg void OnControlOpenTEK();
	afx_msg void OnControlCloseTEK();
	afx_msg void OnControlMacro();
	afx_msg void OnShowMacroWindow();
	afx_msg void OnWindowWindow();
	afx_msg void OnWindowMinimizeAll();
	afx_msg void OnWindowCascade();
	afx_msg void OnWindowStacked();
	afx_msg void OnWindowSidebySide();
	afx_msg void OnWindowRestoreAll();
	afx_msg void OnWindowUndo();
	afx_msg void OnHelpIndex();
	afx_msg void OnHelpUsing();
	afx_msg void OnHelpAbout();
	afx_msg LONG OnDropNotify(UINT ShowMenu, LONG lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
	void Disconnect(BOOL confirm);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

void SendBroadcastMessage(HWND HVTWin, HWND hWnd, char *buf, int buflen);
void SendMulticastMessage(HWND HVTWin, HWND hWnd, char *name, char *buf, int buflen);
void SetMulticastName(char *name);

#ifdef __cplusplus
}
#endif
