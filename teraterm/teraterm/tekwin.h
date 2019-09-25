/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2019 TeraTerm Project
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

/* TERATERM.EXE, TEK window */

/////////////////////////////////////////////////////////////////////////////
// CTEKWindow
#include "tmfc.h"
class CTEKWindow : public TTCFrameWnd
{
private:
  TTEKVar tk;
  HMENU MainMenu, EditMenu, WinMenu,
    FileMenu, SetupMenu, HelpMenu;
  LONG HelpId;

public:
	CTEKWindow(HINSTANCE hInstance);
	int Parse();
	void RestoreSetup();
	void InitMenu(HMENU *Menu);
	void InitMenuPopup(HMENU SubMenu);

protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnActivate(UINT nState, HWND pWndOther, BOOL bMinimized);
	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnDestroy();
	void OnGetMinMaxInfo(MINMAXINFO *lpMMI);
	void OnInitMenuPopup(HMENU hPopupMenu, UINT nIndex, BOOL bSysMenu);
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnKillFocus(HWND hNewWnd);
	void OnLButtonDown(UINT nFlags, POINTS point);
	void OnLButtonUp(UINT nFlags, POINTS point);
	void OnMButtonUp(UINT nFlags, POINTS point);
	int OnMouseActivate(HWND hDesktopWnd, UINT nHitTest, UINT message);
	void OnMouseMove(UINT nFlags, POINTS point);
	void OnMove(int x, int y);
	void OnPaint();
	void OnRButtonUp(UINT nFlags, POINTS point);
	void OnSetFocus(HWND hOldWnd);
	void OnSize(UINT nType, int cx, int cy);
	void OnSysCommand(UINT nID, LPARAM lParam);
	void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnTimer(UINT_PTR nIDEvent);
	LRESULT OnAccelCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnChangeMenu(WPARAM wParam, LPARAM lParam);
	LRESULT OnChangeTBar(WPARAM wParam, LPARAM lParam);
	LRESULT OnDlgHelp(WPARAM wParam, LPARAM lParam);
	LRESULT OnGetSerialNo(WPARAM wParam, LPARAM lParam);
	void OnFilePrint();
	void OnFileExit();
	void OnEditCopy();
	void OnEditCopyScreen();
	void OnEditPaste();
	void OnEditPasteCR();
	void OnEditClearScreen();
	void OnSetupWindow();
	void OnSetupFont();
	void OnVTWin();
	void OnWindowWindow();
	void OnHelpIndex();
	void OnHelpAbout();
	virtual LRESULT Proc(UINT msg, WPARAM wp, LPARAM lp);
};
