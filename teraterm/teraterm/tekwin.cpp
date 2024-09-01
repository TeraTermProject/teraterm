/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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
#include <windowsx.h>
#include "teraterm.h"
#include "tttypes.h"
#include "tttypes_key.h"
#include "tektypes.h"
#include "tttek.h"
#include "ttwinman.h"
#include "ttcommon.h"
#include "keyboard.h"
#include "clipboar.h"
#include "ttdialog.h"
#include "teraprn.h"
#include "helpid.h"
#include "tt_res.h"
#include "tekwin.h"
#include "ttlib.h"
#include <htmlhelp.h>
#include "dlglib.h"
#include "codeconv.h"
#include "compat_win.h"

#define TEKClassName L"TEKWin32"

/////////////////////////////////////////////////////////////////////////////
// CTEKWindow

CTEKWindow::CTEKWindow(HINSTANCE hInstance)
{
	WNDCLASSW wc;
	RECT rect;
	DWORD Style;
	m_hInst = hInstance;

	TEKInit(&tk, &ts);

	if (ts.HideTitle>0) {
		Style = WS_POPUP | WS_THICKFRAME | WS_BORDER;
	}
	else {
		Style = WS_CAPTION | WS_SYSMENU |
		        WS_MINIMIZEBOX | WS_BORDER | WS_THICKFRAME;
	}

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)ProcStub;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEKClassName;

	RegisterClassW(&wc);

	if (ts.TEKPos.x==CW_USEDEFAULT) {
		rect = rectDefault;
	}
	else {
		rect.left = ts.TEKPos.x;
		rect.top = ts.TEKPos.y;
		rect.right = rect.left + 640; //temporary width
		rect.bottom = rect.top + 400; //temporary height
	}
	CreateW(hInstance, TEKClassName, L"Tera Term", Style, rect, ::GetDesktopWindow(), NULL);

//--------------------------------------------------------
	HTEKWin = GetSafeHwnd();
	if (HTEKWin == NULL) {
		return;
	}

	// Windows 11 �ŃE�B���h�E�̊p���ۂ��Ȃ�Ȃ��悤�ɂ���
	if (ts.WindowCornerDontround && pDwmSetWindowAttribute != NULL) {
		DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_DONOTROUND;
		pDwmSetWindowAttribute(HTEKWin, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
	}

	tk.HWin = HTEKWin;
	// register this window to the window list
	RegWin(HVTWin,HTEKWin);

	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW((ts.TEKIcon!=IdIconDefault)?ts.TEKIcon:IDI_TEK), 0);

	MainMenu = NULL;
	WinMenu = NULL;
	if ((ts.HideTitle==0) && (ts.PopupMenu==0)) {
		InitMenu(&MainMenu);
		::SetMenu(HTEKWin,MainMenu);
	}

	ChangeTitle();

	::GetWindowRect(tk.HWin,&rect);
	TEKResizeWindow(&tk,&ts, rect.right-rect.left, rect.bottom-rect.top);

	if ((ts.PopupMenu>0) || (ts.HideTitle>0)) {
		::PostMessage(HTEKWin,WM_USER_CHANGEMENU,0,0);
	}

	::ShowWindow(tk.HWin, SW_SHOWNORMAL);
}

int CTEKWindow::Parse()
{
	return (TEKParse(&tk,&ts,&cv));
}

void CTEKWindow::RestoreSetup()
{
	TEKRestoreSetup(&tk,&ts);
	ChangeTitle();
}

void CTEKWindow::InitMenu(HMENU *Menu)
{
	static const DlgTextInfo MenuTextInfo[] = {
		{ 0, "TEKMENU_FILE" },
		{ 1, "TEKMENU_EDIT" },
		{ 2, "TEKMENU_SETUP" },
		{ ID_TEKVTWIN, "TEKMENU_VTWIN"},
		{ 4, "TEKMENU_HELP"},
	};
	static const DlgTextInfo FileMenuTextInfo[] = {
		{ ID_TEKFILE_PRINT, "TEKMENU_FILE_PRINT" },
		{ ID_TEKFILE_EXIT, "TEKMENU_FILE_EXIT" },
	};
	static const DlgTextInfo EditMenuTextInfo[] = {
		{ ID_TEKEDIT_COPY, "TEKMENU_EDIT_COPY" },
		{ ID_TEKEDIT_COPYSCREEN, "TEKMENU_EDIT_COPYSCREEN" },
		{ ID_TEKEDIT_PASTE, "TEKMENU_EDIT_PASTE" },
		{ ID_TEKEDIT_PASTECR, "TEKMENU_EDIT_PASTECR" },
		{ ID_TEKEDIT_CLEARSCREEN, "TEKMENU_EDIT_CLSCREEN" },
	};
	static const DlgTextInfo SetupMenuTextInfo[] = {
		{ ID_TEKSETUP_WINDOW, "TEKMENU_SETUP_WINDOW" },
		{ ID_TEKSETUP_FONT, "TEKMENU_SETUP_FONT" },
	};
	static const DlgTextInfo HelpMenuTextInfo[] = {
		{ ID_TEKHELP_INDEX, "TEKMENU_HELP_INDEX" },
		{ ID_TEKHELP_ABOUT, "TEKMENU_HELP_ABOUT" },
	};

	*Menu = ::LoadMenu(m_hInst,
	                   MAKEINTRESOURCE(IDR_TEKMENU));
	FileMenu = GetSubMenu(*Menu,0);
	EditMenu = GetSubMenu(*Menu,1);
	SetupMenu = GetSubMenu(*Menu,2);
	HelpMenu = GetSubMenu(*Menu,4);

	SetDlgMenuTextsW(*Menu, MenuTextInfo, _countof(MenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(FileMenu, FileMenuTextInfo, _countof(FileMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(EditMenu, EditMenuTextInfo, _countof(EditMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(SetupMenu, SetupMenuTextInfo, _countof(SetupMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(HelpMenu, HelpMenuTextInfo, _countof(HelpMenuTextInfo), ts.UILanguageFileW);

	if ((ts.MenuFlag & MF_SHOWWINMENU) !=0) {
		wchar_t *uimsg;
		WinMenu = CreatePopupMenu();
		GetI18nStrWW("Tera Term", "TEKMENU_WINDOW", L"&Window", ts.UILanguageFileW, &uimsg);
		::InsertMenuW(*Menu,4,MF_STRING | MF_ENABLED | MF_POPUP | MF_BYPOSITION,
					  (UINT_PTR)WinMenu, uimsg);
		free(uimsg);
	}
}

void CTEKWindow::InitMenuPopup(HMENU SubMenu)
{
	if (SubMenu==EditMenu) {
		if (tk.Select) {
			EnableMenuItem(EditMenu,ID_TEKEDIT_COPY,MF_BYCOMMAND | MF_ENABLED);
		}
		else {
			EnableMenuItem(EditMenu,ID_TEKEDIT_COPY,MF_BYCOMMAND | MF_GRAYED);
		}

		if (cv.Ready &&
		    (IsClipboardFormatAvailable(CF_TEXT) ||
		     IsClipboardFormatAvailable(CF_OEMTEXT))) {
			EnableMenuItem(EditMenu,ID_TEKEDIT_PASTE,MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(EditMenu,ID_TEKEDIT_PASTECR,MF_BYCOMMAND | MF_ENABLED);
		}
		else {
			EnableMenuItem(EditMenu,ID_TEKEDIT_PASTE,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(EditMenu,ID_TEKEDIT_PASTECR,MF_BYCOMMAND | MF_GRAYED);
		}
	}
	else if ( SubMenu == WinMenu ) {
		SetWinMenuW(WinMenu, ts.UILanguageFileW, 0);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTEKWindow message handler

BOOL CTEKWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if ((LOWORD(wParam)>=ID_WINDOW_1) && (LOWORD(wParam)<ID_WINDOW_1+9)) {
		SelectWin(LOWORD(wParam)-ID_WINDOW_1);
		return TRUE;
	}
	else {
		return TTCFrameWnd::OnCommand(wParam, lParam);
	}
}

void CTEKWindow::OnActivate(UINT nState, HWND pWndOther, BOOL bMinimized)
{
	if (nState!=WA_INACTIVE) {
		tk.Active = TRUE;
		ActiveWin = IdTEK;
	}
	else {
		tk.Active = FALSE;
	}
}

/**
 *	�L�[�{�[�h����1��������
 *	@param	nChar	UTF-16 char(wchar_t)	IsWindowUnicode() == TRUE ��
 *					ANSI char(char)			IsWindowUnicode() == FALSE ��
 */
void CTEKWindow::OnChar(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{

	if (!KeybEnabled || (TalkStatus!=IdTalkKeyb)) {
		return;
	}

	wchar_t u16;
	if (IsWindowUnicode(HTEKWin) == TRUE) {
		// ���͂� UTF-16
		u16 = (wchar_t)nChar;
	} else {
		// ���͂� ANSI
		//		ANSI(ACP) -> UTF-32 -> UTF-16
		const char mb_str[2] = {(char)nChar, 0};
		unsigned int u32;
		size_t mb_len = MBCPToUTF32(mb_str, 1, CP_ACP, &u32);
		if (mb_len == 0) {
			return;
		}
		u16 = (wchar_t)u32;
	}

	if (tk.GIN) {
		char Code = (char)nChar;
		TEKReportGIN(&tk, &ts, &cv, Code);
	}
	else {
		for (unsigned int i = 1; i <= nRepCnt; i++) {
			CommTextOutW(&cv, &u16, 1);
			if (ts.LocalEcho > 0) {
				CommTextEchoW(&cv, &u16, 1);
			}
		}
	}
}

void CTEKWindow::OnDestroy()
{
	// remove this window from the window list
	UnregWin(HTEKWin);

	TEKEnd(&tk);
	HTEKWin = NULL;
	pTEKWin = NULL;
	ActiveWin = IdVT;

	TTSetIcon(m_hInst, m_hWnd, NULL, 0);
}

void CTEKWindow::OnGetMinMaxInfo(MINMAXINFO *lpMMI)
{
	lpMMI->ptMaxSize.x = 10000;
	lpMMI->ptMaxSize.y = 10000;
	lpMMI->ptMaxTrackSize.x = 10000;
	lpMMI->ptMaxTrackSize.y = 10000;
}

void CTEKWindow::OnInitMenuPopup(HMENU hPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	InitMenuPopup(hPopupMenu);
}

void CTEKWindow::OnKeyDown(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
	KeyDown(HTEKWin,nChar,nRepCnt,nFlags & 0x1ff);
}

void CTEKWindow::OnKeyUp(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
	KeyUp(nChar);
}

void CTEKWindow::OnKillFocus(HWND hNewWnd)
{
	TEKDestroyCaret(&tk,&ts);
	TTCFrameWnd::OnKillFocus(hNewWnd);
}

void CTEKWindow::OnLButtonDown(WPARAM nFlags, POINTS point)
{
	POINT p;
	HMENU PopupMenu, PopupBase;

	p.x = point.x;
	p.y = point.y;

	// popup menu
	if (ControlKey() && (MainMenu==NULL))
	{
		int i, numItems;
		char itemText[256];

		InitMenu(&PopupMenu);

		PopupBase = CreatePopupMenu();
		numItems = GetMenuItemCount(PopupMenu);

		for (i = 0; i < numItems; i++) {
			HMENU submenu = GetSubMenu(PopupMenu, i);

			if (submenu != NULL) {
				InitMenuPopup(submenu);
			}

			if (GetMenuString(PopupMenu, i, itemText, sizeof(itemText), MF_BYPOSITION) != 0) {
				int state = GetMenuState(PopupMenu, i, MF_BYPOSITION) &
				            (MF_CHECKED | MF_DISABLED | MF_GRAYED | MF_HILITE |
				             MF_MENUBARBREAK | MF_MENUBREAK | MF_SEPARATOR);

				AppendMenu(PopupBase,
				           submenu != NULL ? LOBYTE(state) | MF_POPUP : state,
				           submenu != NULL ? (UINT_PTR)submenu : GetMenuItemID(PopupMenu, i),
				           itemText);
			}
		}

		::ClientToScreen(tk.HWin, &p);
		TrackPopupMenu(PopupBase,TPM_LEFTALIGN | TPM_LEFTBUTTON,
		               p.x,p.y,0,tk.HWin,NULL);
		if (WinMenu!=NULL) {
			DestroyMenu(WinMenu);
			WinMenu = NULL;
		}
		DestroyMenu(PopupBase);
		DestroyMenu(PopupMenu);
		PopupMenu = 0;
		return;
	}

	TEKWMLButtonDown(&tk,&ts,&cv,p);
}

void CTEKWindow::OnLButtonUp(WPARAM nFlags, POINTS point)
{
	TEKWMLButtonUp(&tk,&ts);
}

void CTEKWindow::OnMButtonUp(WPARAM nFlags, POINTS point)
{
	//OnRButtonUp(nFlags,point);
}

int CTEKWindow::OnMouseActivate(HWND pDesktopWnd, UINT nHitTest, UINT message)
{
	if ((ts.SelOnActive==0) &&
	    (nHitTest==HTCLIENT)) { //disable mouse event for text selection
		return MA_ACTIVATEANDEAT; //     when window is activated
	}
	else {
		return MA_ACTIVATE;
	}
}

void CTEKWindow::OnMouseMove(WPARAM nFlags, POINTS point)
{
	POINT p;

	p.x = point.x;
	p.y = point.y;
	TEKWMMouseMove(&tk,&ts,p);
}

void CTEKWindow::OnMove(int x, int y)
{
	RECT R;

	::GetWindowRect(HTEKWin,&R);
	ts.TEKPos.x = R.left;
	ts.TEKPos.y = R.top;
}

void CTEKWindow::OnPaint()
{
	PAINTSTRUCT ps;
	HDC PaintDC;

	PaintDC = BeginPaint(&ps);

	TEKPaint(&tk,&ts,PaintDC,&ps);

	EndPaint(&ps);
}

void CTEKWindow::OnRButtonUp(UINT nFlags, POINTS point)
{
	CBStartPaste(tk.HWin, FALSE, FALSE);
}

void CTEKWindow::OnSetFocus(HWND hOldWnd)
{
	TEKChangeCaret(&tk,&ts);
	TTCFrameWnd::OnSetFocus(hOldWnd);
}

void CTEKWindow::OnSize(WPARAM nType, int cx, int cy)
{
	int w, h;
	RECT R;

	if (tk.Minimized && (nType==SIZE_RESTORED)) {
		tk.Minimized = FALSE;
		return;
	}
	tk.Minimized = (nType==SIZE_MINIMIZED);
	if (tk.Minimized) {
		return;
	}

	::GetWindowRect(tk.HWin,&R);
	w = R.right - R.left;
	h = R.bottom - R.top;

	TEKWMSize(&tk,&ts,w,h,cx,cy);
}

void CTEKWindow::OnSysCommand(WPARAM nID, LPARAM lParam)
{
	if (nID==ID_SHOWMENUBAR) {
		ts.PopupMenu = 0;
		SwitchMenu();
	}
	else {
		TTCFrameWnd::OnSysCommand(nID,lParam);
	}
}

void CTEKWindow::OnSysKeyDown(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_F10) {
		OnKeyDown(nChar,nRepCnt,nFlags);
	}
	else {
		TTCFrameWnd::OnSysKeyDown(nChar,nRepCnt,nFlags);
	}
}

void CTEKWindow::OnSysKeyUp(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_F10) {
		OnKeyUp(nChar,nRepCnt,nFlags);
	}
	else {
		TTCFrameWnd::OnSysKeyUp(nChar,nRepCnt,nFlags);
	}
}

void CTEKWindow::OnTimer(UINT_PTR nIDEvent)
{
	UINT T;

	if (nIDEvent==IdCaretTimer) {
		if (ts.NonblinkingCursor!=0) {
			T = GetCaretBlinkTime();
			SetCaretBlinkTime(T);
		}
		else {
			::KillTimer(HTEKWin,IdCaretTimer);
		}
		return;
	}
	else {
		::KillTimer(HTEKWin,nIDEvent);
	}
}

LRESULT CTEKWindow::OnAccelCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IdPrint:
			OnFilePrint();
			break;
		case IdCmdEditCopy:
			OnEditCopy();
			break;
		case IdCmdEditPaste:
			OnEditPaste();
			break;
		case IdCmdEditPasteCR:
			OnEditPasteCR();
			break;
		case IdCmdEditCLS:
			OnEditClearScreen();
			break;
		case IdCmdCtrlCloseTEK:
			OnFileExit();
			break;
		case IdCmdNextWin:
			SelectNextWin(HTEKWin,1,FALSE);
			break;
		case IdCmdPrevWin:
			SelectNextWin(HTEKWin,-1,FALSE);
			break;
		case IdCmdNextSWin:
			SelectNextWin(HTEKWin,1,TRUE);
			break;
		case IdCmdPrevSWin:
			SelectNextWin(HTEKWin,-1,TRUE);
			break;
		case IdBreak:
		case IdCmdRestoreSetup:
		case IdCmdLoadKeyMap:
			::PostMessage(HVTWin,WM_USER_ACCELCOMMAND,wParam,0);
		break;
	}
	return 0;
}

LRESULT CTEKWindow::OnChangeMenu(WPARAM wParam, LPARAM lParam)
{
	HMENU SysMenu;
	BOOL Show, B1, B2;

	Show = (ts.PopupMenu==0) && (ts.HideTitle==0);
	if (Show != (MainMenu!=NULL)) {
		if (! Show) {
			if (WinMenu!=NULL) {
				DestroyMenu(WinMenu);
			}
			WinMenu = NULL;
			DestroyMenu(MainMenu);
			MainMenu = NULL;
		}
		else {
			InitMenu(&MainMenu);
		}

		tk.AdjustSize = TRUE;
		::SetMenu(tk.HWin, MainMenu);
		::DrawMenuBar(HTEKWin);
	}

	B1 = ((ts.MenuFlag & MF_SHOWWINMENU)!=0);
	B2 = (WinMenu!=NULL);
	if ((MainMenu!=NULL) &&
	    (B1 != B2)) {
		if (WinMenu==NULL) {
			wchar_t *uimsg;
			WinMenu = CreatePopupMenu();
			GetI18nStrWW("Tera Term", "TEKMENU_WINDOW", L"&Window", ts.UILanguageFileW, &uimsg);
			::InsertMenuW(MainMenu,4,
						  MF_STRING | MF_ENABLED | MF_POPUP | MF_BYPOSITION,
						  (UINT_PTR)WinMenu, uimsg);
			free(uimsg);
		}
		else {
			RemoveMenu(MainMenu,4,MF_BYPOSITION);
			DestroyMenu(WinMenu);
			WinMenu = NULL;
		}
		::DrawMenuBar(HTEKWin);
	}

	::GetSystemMenu(tk.HWin,TRUE);
	if ((! Show) && ((ts.MenuFlag & MF_NOSHOWMENU)==0)) {
		wchar_t *uimsg;
		SysMenu = ::GetSystemMenu(tk.HWin,FALSE);
		AppendMenuW(SysMenu, MF_SEPARATOR, 0, NULL);
		GetI18nStrWW("Tera Term", "TEKMENU_SHOW_MENUBAR", L"Show menu &bar", ts.UILanguageFileW, &uimsg);
		AppendMenuW(SysMenu, MF_STRING, ID_SHOWMENUBAR, uimsg);
		free(uimsg);
	}
	return 0;
}

LRESULT CTEKWindow::OnChangeTBar(WPARAM wParam, LPARAM lParam)
{
	BOOL TBar;
	DWORD Style;
	HMENU SysMenu;

	Style = GetWindowLongPtr (GWL_STYLE);
	TBar = ((Style & WS_SYSMENU)!=0);
	if (TBar == (ts.HideTitle==0)) {
		return 0;
	}
	if (ts.HideTitle>0) {
		Style = Style & ~(WS_SYSMENU | WS_CAPTION |
		                  WS_MINIMIZEBOX) | WS_BORDER | WS_POPUP;
	}
	else {
		Style = Style & ~WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
	}
	tk.AdjustSize = TRUE;
	SetWindowLongPtr (GWL_STYLE, Style);
	::SetWindowPos (HTEKWin, NULL, 0, 0, 0, 0,
	                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	::ShowWindow (HTEKWin, SW_SHOW);

	if ((ts.HideTitle==0) && (MainMenu==NULL) &&
	    ((ts.MenuFlag & MF_NOSHOWMENU) == 0)) {
		wchar_t *uimsg;
		SysMenu = ::GetSystemMenu(HTEKWin,FALSE);
		AppendMenuW(SysMenu, MF_SEPARATOR, 0, NULL);
		GetI18nStrWW("Tera Term", "TEKMENU_SHOW_MENUBAR", L"Show menu &bar", ts.UILanguageFileW, &uimsg);
		AppendMenuW(SysMenu, MF_STRING, ID_SHOWMENUBAR, uimsg);
		free(uimsg);
	}
	return 0;
}

LRESULT CTEKWindow::OnDlgHelp(WPARAM wParam, LPARAM lParam)
{
	DWORD help_id = (wParam == 0) ? HelpId : (DWORD)wParam;
	OpenHelpCV(&cv, HH_HELP_CONTEXT, help_id);
	return 0;
}

LRESULT CTEKWindow::OnGetSerialNo(WPARAM wParam, LPARAM lParam)
{
	return (LONG)SerialNo;
}

void CTEKWindow::OnFilePrint()
{
	BOOL Sel;
	HDC PrintDC;

	HelpId = HlpTEKFilePrint;

	Sel = tk.Select;
	PrintDC = PrnBox(tk.HWin,&Sel);
	if (PrintDC==NULL) {
		return;
	}
	if (! PrnStart(ts.Title)) {
		return;
	}

	(*TEKPrint)(&tk,&ts,PrintDC,Sel);

	PrnStop();
}

void CTEKWindow::OnFileExit()
{
	DestroyWindow();
}

void CTEKWindow::OnEditCopy()
{
	(*TEKCMCopy)(&tk,&ts);
}

void CTEKWindow::OnEditCopyScreen()
{
	(*TEKCMCopyScreen)(&tk,&ts);
}

void CTEKWindow::OnEditPaste()
{
	CBStartPaste(tk.HWin, FALSE, FALSE);
}

void CTEKWindow::OnEditPasteCR()
{
	CBStartPaste(tk.HWin, TRUE, FALSE);
}

void CTEKWindow::OnEditClearScreen()
{
	(*TEKClearScreen)(&tk,&ts);
}

void CTEKWindow::OnSetupWindow()
{
	BOOL Ok;
	WORD OldEmu;

	HelpId = HlpTEKSetupWindow;
	ts.VTFlag = 0;
	ts.SampleFont = tk.TEKFont[0];

	if (! LoadTTDLG()) {
		return;
	}
	OldEmu = ts.TEKColorEmu;
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	Ok = (*SetupWin)(HTEKWin, &ts);
	if (Ok) {
		(*TEKResetWin)(&tk,&ts,OldEmu);
		ChangeTitle();
		SwitchMenu();
		SwitchTitleBar();
	}
}

void CTEKWindow::OnSetupFont()
{
	BOOL Ok;

	HelpId = HlpTEKSetupFont;
	ts.VTFlag = 0;
	if (! LoadTTDLG()) {
		return;
	}
	Ok = (*ChooseFontDlg)(HTEKWin,&tk.TEKlf,&ts);
	if (! Ok) {
		return;
	}

	(*TEKSetupFont)(&tk,&ts);
}

void CTEKWindow::OnVTWin()
{
	VTActivate();
}

void CTEKWindow::OnWindowWindow()
{
	BOOL Close;

	HelpId = HlpWindowWindow;
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	(*WindowWindow)(HTEKWin,&Close);
	if (Close) {
		OnClose();
	}
}

void CTEKWindow::OnHelpIndex()
{
	OpenHelpCV(&cv, HH_DISPLAY_TOPIC, 0);
}

void CTEKWindow::OnHelpAbout()
{
	if (! LoadTTDLG()) {
		return;
	}
	(*AboutDialog)(tk.HWin);
}

LRESULT CTEKWindow::Proc(UINT msg, WPARAM wp, LPARAM lp)
{
	LRESULT retval = 0;
	if (msg == MsgDlgHelp) {
		// HELPMSGSTRING message ��
		//		wp = dialog handle
		//		lp = initialization structure
		OnDlgHelp(HelpId, 0);
		return 0;
	}
	switch(msg)
	{
	case WM_ACTIVATE:
		OnActivate(wp & 0xFFFF, (HWND)wp, (wp >> 16) & 0xFFFF);
		break;
	case WM_CHAR:
		OnChar(wp, LOWORD(lp), HIWORD(lp));
		break;
	case WM_DESTROY:
		OnDestroy();
		break;
	case WM_GETMINMAXINFO:
		OnGetMinMaxInfo((MINMAXINFO *)lp);
		break;
	case WM_INITMENUPOPUP:
		OnInitMenuPopup((HMENU)wp, LOWORD(lp), HIWORD(lp));
		break;
	case WM_KEYDOWN:
		OnKeyDown(wp, LOWORD(lp), HIWORD(lp));
		break;
	case WM_KEYUP:
		OnKeyUp(wp, LOWORD(lp), HIWORD(lp));
		break;
	case WM_KILLFOCUS:
		OnKillFocus((HWND)wp);
		break;
	case WM_LBUTTONDOWN:
		OnLButtonDown(wp, MAKEPOINTS(lp));
		break;
	case WM_LBUTTONUP:
		OnLButtonUp(wp, MAKEPOINTS(lp));
		break;
	case WM_MBUTTONUP:
		OnMButtonUp(wp, MAKEPOINTS(lp));
		break;
	case WM_MOUSEACTIVATE:
		retval = OnMouseActivate((HWND)wp, LOWORD(lp), HIWORD(lp));
		break;
	case WM_MOUSEMOVE:
		OnMouseMove(wp, MAKEPOINTS(lp));
		break;
	case WM_MOVE:
		OnMove(LOWORD(lp), HIWORD(lp));
		break;
	case WM_PAINT:
		OnPaint();
		break;
	case WM_RBUTTONUP:
		OnRButtonUp((UINT)wp, MAKEPOINTS(lp));
		break;
	case WM_SETFOCUS:
		OnSetFocus((HWND)wp);
		DefWindowProc(msg, wp, lp);
		break;
	case WM_SIZE:
		OnSize(wp, LOWORD(lp), HIWORD(lp));
		break;
	case WM_SYSCOMMAND:
		OnSysCommand(wp, lp);
		DefWindowProc(msg, wp, lp);
		break;
	case WM_SYSKEYDOWN:
		OnSysKeyDown(wp, LOWORD(lp), HIWORD(lp));
		break;
	case WM_SYSKEYUP:
		OnSysKeyUp(wp, LOWORD(lp), HIWORD(lp));
		break;
	case WM_TIMER:
		OnTimer(wp);
		break;
	case WM_USER_ACCELCOMMAND:
		OnAccelCommand(wp, lp);
		break;
	case WM_USER_CHANGEMENU:
		OnChangeMenu(wp, lp);
		break;
	case WM_USER_CHANGETBAR:
		OnChangeTBar(wp, lp);
		break;
	case WM_USER_DLGHELP2:
		OnDlgHelp(wp, lp);
		break;
	case WM_USER_GETSERIALNO:
		OnGetSerialNo(wp, lp);
		break;
	case WM_COMMAND:
	{
		const WORD wID = GET_WM_COMMAND_ID(wp, lp);
		switch (wID) {
		case ID_TEKFILE_PRINT: OnFilePrint(); break;
		case ID_TEKFILE_EXIT: OnFileExit(); break;
		case ID_TEKEDIT_COPY: OnEditCopy(); break;
		case ID_TEKEDIT_COPYSCREEN: OnEditCopyScreen(); break;
		case ID_TEKEDIT_PASTE: OnEditPaste(); break;
		case ID_TEKEDIT_PASTECR: OnEditPasteCR(); break;
		case ID_TEKEDIT_CLEARSCREEN: OnEditClearScreen(); break;
		case ID_TEKSETUP_WINDOW: OnSetupWindow(); break;
		case ID_TEKSETUP_FONT: OnSetupFont(); break;
		case ID_TEKVTWIN: OnVTWin(); break;
		case ID_TEKWINDOW_WINDOW: OnWindowWindow(); break;
		case ID_TEKHELP_INDEX: OnHelpIndex(); break;
		case ID_TEKHELP_ABOUT: OnHelpAbout(); break;
		default:
			OnCommand(wp, lp);
			break;
		}
		break;
	}
	case WM_NCHITTEST: {
		retval = TTCFrameWnd::DefWindowProc(msg, wp, lp);
		if (ts.HideTitle>0) {
			if ((retval ==HTCLIENT) && AltKey()) {
				retval = HTCAPTION;
			}
		}
		break;
	}
	case WM_DPICHANGED: {
		const UINT NewDPI = LOWORD(wp);
		TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW((ts.TEKIcon!=IdIconDefault)?ts.TEKIcon:IDI_TEK), NewDPI);
		break;
	}
	default:
		retval = DefWindowProc(msg, wp, lp);
		break;
	}

	return retval;
}
