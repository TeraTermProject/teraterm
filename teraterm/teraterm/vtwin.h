/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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

#include "addsetting.h"

#ifdef __cplusplus
#include "tipwin.h"
#include "tmfc.h"
#include "unicode_test.h"
#include "tipwin.h"

class CVTWindow : public TTCFrameWnd
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
  void DropInit();
  void DropUninit();
  struct DropData_tag *DropData;
  wchar_t **DropLists;
  int DropListCount;
  void DropListFree();

  // window attribute
  BYTE Alpha;
  void SetWindowAlpha(BYTE alpha);

  // for debug
#if UNICODE_DEBUG
  TipWin *TipWinCodeDebug;
  int CtrlKeyState;			// 0:�J�n/1:����/2:����/3:����(�\�����)
  DWORD CtrlKeyDownTick;	// �ŏ��ɉ�����tick
#endif

  // TipWin
  CTipWin* TipWin;

  // for html help
  LONG HelpId;

  // �T�C�Y�ύX��(WM_SIZING �` WM_EXITSIZEMOVE)��TRUE
  BOOL isSizing;

public:
	CVTWindow(HINSTANCE hInstance);
	~CVTWindow();
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
	void Disconnect(BOOL confirm);

protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnActivate(UINT nState, HWND pWndOther, BOOL bMinimized);
	void OnChar(WPARAM nChar, UINT nRepCnt, UINT nFlags);
	LRESULT OnUniChar(WPARAM wParam, LPARAM lParam);
	void OnClose();
	void OnAllClose();
	void OnDestroy();
	void OnDropFiles(HDROP hDropInfo);
	void OnGetMinMaxInfo(MINMAXINFO *lpMMI);
	void OnHScroll(UINT nSBCode, UINT nPos, HWND pScrollBar);
	void OnInitMenuPopup(HMENU hPopupMenu, UINT nIndex, BOOL bSysMenu);
	void OnKeyDown(WPARAM nChar, UINT nRepCnt, UINT nFlags);
	void OnKeyUp(WPARAM nChar, UINT nRepCnt, UINT nFlags);
	void OnKillFocus(HWND hNewWnd);
	void OnLButtonDblClk(WPARAM nFlags, POINTS point);
	void OnLButtonDown(WPARAM nFlags, POINTS point);
	void OnLButtonUp(WPARAM nFlags, POINTS point);
	void OnMButtonDown(WPARAM nFlags, POINTS point);
	void OnMButtonUp(WPARAM nFlags, POINTS point);
	LRESULT OnMouseActivate(HWND pDesktopWnd, UINT nHitTest, UINT message);
	void OnMouseMove(WPARAM nFlags, POINTS point);
	void OnMove(int x, int y);
	BOOL OnMouseWheel(UINT nFlags, short zDelta, POINTS pt);
	void OnNcLButtonDblClk(UINT nHitTest, POINTS point);
	void OnNcRButtonDown(UINT nHitTest, POINTS point);
	void OnPaint();
	void OnRButtonDown(UINT nFlags, POINTS point);
	void OnRButtonUp(UINT nFlags, POINTS point);
	void OnSetFocus(HWND hOldWnd);
	void OnSize(WPARAM nType, int cx, int cy);
	void OnSizing(WPARAM fwSide, LPRECT pRect);
	void OnSysChar(WPARAM nChar, UINT nRepCnt, UINT nFlags);
	void OnSysCommand(WPARAM nID, LPARAM lParam);
	void OnSysKeyDown(WPARAM nChar, UINT nRepCnt, UINT nFlags);
	void OnSysKeyUp(WPARAM nChar, UINT nRepCnt, UINT nFlags);
	void OnTimer(UINT_PTR nIDEvent);
	void OnVScroll(UINT nSBCode, UINT nPos, HWND pScrollBar);
	BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	LRESULT OnWindowPosChanging(WPARAM wParam, LPARAM lParam);
	LRESULT OnSettingChange(WPARAM wParam, LPARAM lParam);
	LRESULT OnEnterSizeMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnExitSizeMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnIMEStartComposition(WPARAM wParam, LPARAM lParam);
	LRESULT OnIMEEndComposition(WPARAM wParam, LPARAM lParam);
	LRESULT OnIMEComposition(WPARAM wParam, LPARAM lParam);
	LRESULT OnIMEInputChange(WPARAM wParam, LPARAM lParam);
	LRESULT OnIMENotify(WPARAM wParam, LPARAM lParam);
	LRESULT OnIMERequest(WPARAM wParam, LPARAM lParam);
	LRESULT OnAccelCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnChangeMenu(WPARAM wParam, LPARAM lParam);
	LRESULT OnChangeTBar(WPARAM wParam, LPARAM lParam);
	LRESULT OnCommNotify(WPARAM wParam, LPARAM lParam);
	LRESULT OnCommOpen(WPARAM wParam, LPARAM lParam);
	LRESULT OnCommStart(WPARAM wParam, LPARAM lParam);
	LRESULT OnDdeEnd(WPARAM wParam, LPARAM lParam);
	LRESULT OnDlgHelp(WPARAM wParam, LPARAM lParam);
	LRESULT OnFileTransEnd(WPARAM wParam, LPARAM lParam);
	LRESULT OnGetSerialNo(WPARAM wParam, LPARAM lParam);
	LRESULT OnKeyCode(WPARAM wParam, LPARAM lParam);
	LRESULT OnProtoEnd(WPARAM wParam, LPARAM lParam);
	LRESULT OnChangeTitle(WPARAM wParam, LPARAM lParam);
	LRESULT OnReceiveIpcMessage(WPARAM wParam, LPARAM lParam);
	LRESULT OnNonConfirmClose(WPARAM wParam, LPARAM lParam);
	LRESULT OnNotifyIcon(WPARAM wParam, LPARAM lParam);
	void OnFileNewConnection();
	void OnDuplicateSession();
	void OnCygwinConnection();
	void OnTTMenuLaunch();
	void OnFileLog();
	void OnCommentToLog();
	void OnViewLog();
	void OnShowLogDialog();
	void OnPauseLog();
	void OnStopLog();
	void OnReplayLog();
	void OnExternalSetup();
	void OnFileSend();
	void OnFileKermitRcv();
	void OnFileKermitGet();
	void OnFileKermitSend();
	void OnFileKermitFinish();
	void OnFileXRcv();
	void OnFileXSend();
	void OnFileYRcv();
	void OnFileYSend();
	void OnFileZRcv();
	void OnFileZSend();
	void OnFileBPRcv();
	void OnFileBPSend();
	void OnFileQVRcv();
	void OnFileQVSend();
	void OnFileChangeDir();
	void OnFilePrint();
	void OnFileDisconnect();
	void OnFileExit();
	void OnEditCopy();
	void OnEditCopyTable();
	void OnEditPaste();
	void OnEditPasteCR();
	void OnEditClearScreen();
	void OnEditClearBuffer();
	void OnEditCancelSelection();
	void OnEditSelectScreenBuffer();
	void OnEditSelectAllBuffer();
	void OnSetupTerminal();
	void OnSetupWindow();
	void OnSetupFont();
	void OnSetupKeyboard();
	void OnSetupSerialPort();
	void OnSetupTCPIP();
	void OnSetupGeneral();
	void OnSetupSave();
	void OnSetupRestore();
	void OnOpenSetupDirectory();
	void OnSetupLoadKeyMap();
	void OnControlResetTerminal();
	void OnControlResetRemoteTitle();
	void OnControlBroadcastCommand();
	void OnControlAreYouThere();
	void OnControlSendBreak();
	void OnControlResetPort();
	void OnControlOpenTEK();
	void OnControlCloseTEK();
	void OnControlMacro();
	void OnShowMacroWindow();
	void OnWindowWindow();
	void OnWindowMinimizeAll();
	void OnWindowCascade();
	void OnWindowStacked();
	void OnWindowSidebySide();
	void OnWindowRestoreAll();
	void OnWindowUndo();
	void OnHelpIndex();
	void OnHelpAbout();
	LRESULT OnDropNotify(WPARAM ShowMenu, LPARAM lParam);
	LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam, BOOL calcOnly);
	virtual LRESULT Proc(UINT msg, WPARAM wp, LPARAM lp);

private:
	void CodePopup(int client_x, int client_y);
public:
	void SetColor();
};

extern CVTWindow *pVTWin;
#endif

#ifdef __cplusplus
extern "C" {
#endif

void VtwinSetHelpId(DWORD data);

#ifdef __cplusplus
}
#endif
