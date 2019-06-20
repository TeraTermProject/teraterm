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
#include <windowsx.h>
#include "teraterm.h"
#include "tttypes.h"
#include "tektypes.h"
#include "teklib.h"
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
#include <tchar.h>

#define CWnd	TTCWnd
#define CFrameWnd	TTCFrameWnd

#define TEKClassName _T("TEKWin32")

static HINSTANCE AfxGetInstanceHandle()
{
	return hInst;
}

/////////////////////////////////////////////////////////////////////////////
// CTEKWindow

CTEKWindow::CTEKWindow()
{
	WNDCLASS wc;
	RECT rect;
	DWORD Style;
	int fuLoad = LR_DEFAULTCOLOR;

	if (! LoadTTTEK()) {
		return;
	}
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
	wc.hInstance = AfxGetInstanceHandle();
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEKClassName;

	RegisterClass(&wc);

	if (ts.TEKPos.x==CW_USEDEFAULT) {
		rect = rectDefault;
	}
	else {
		rect.left = ts.TEKPos.x;
		rect.top = ts.TEKPos.y;
		rect.right = rect.left + 640; //temporary width
		rect.bottom = rect.top + 400; //temporary height
	}
	Create(hInst, TEKClassName, _T("Tera Term"), Style, rect, ::GetDesktopWindow(), NULL);
//--------------------------------------------------------
	HTEKWin = GetSafeHwnd();
	if (HTEKWin == NULL) {
		return;
	}
	tk.HWin = HTEKWin;
	// register this window to the window list
	RegWin(HVTWin,HTEKWin);

	if (IsWindowsNT4()) {
		fuLoad = LR_VGACOLOR;
	}
	::PostMessage(HTEKWin,WM_SETICON,ICON_SMALL,
	              (LPARAM)LoadImage(AfxGetInstanceHandle(),
	                                MAKEINTRESOURCE((ts.TEKIcon!=IdIconDefault)?ts.TEKIcon:IDI_TEK),
	                                IMAGE_ICON,16,16,fuLoad));
	::PostMessage(HTEKWin,WM_SETICON,ICON_BIG,
	              (LPARAM)LoadImage(AfxGetInstanceHandle(),
	                                MAKEINTRESOURCE((ts.TEKIcon!=IdIconDefault)?ts.TEKIcon:IDI_TEK),
	                                IMAGE_ICON, 0, 0, fuLoad));

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

	*Menu = ::LoadMenu(AfxGetInstanceHandle(),
	                   MAKEINTRESOURCE(IDR_TEKMENU));
	FileMenu = GetSubMenu(*Menu,0);
	EditMenu = GetSubMenu(*Menu,1);
	SetupMenu = GetSubMenu(*Menu,2);
	HelpMenu = GetSubMenu(*Menu,4);

	SetDlgMenuTexts(*Menu, MenuTextInfo, _countof(MenuTextInfo), ts.UILanguageFile);
	SetDlgMenuTexts(FileMenu, FileMenuTextInfo, _countof(FileMenuTextInfo), ts.UILanguageFile);
	SetDlgMenuTexts(EditMenu, EditMenuTextInfo, _countof(EditMenuTextInfo), ts.UILanguageFile);
	SetDlgMenuTexts(SetupMenu, SetupMenuTextInfo, _countof(SetupMenuTextInfo), ts.UILanguageFile);
	SetDlgMenuTexts(HelpMenu, HelpMenuTextInfo, _countof(HelpMenuTextInfo), ts.UILanguageFile);

	if ((ts.MenuFlag & MF_SHOWWINMENU) !=0) {
		TCHAR uimsg[MAX_UIMSG];
		WinMenu = CreatePopupMenu();
		get_lang_msgT("TEKMENU_WINDOW", uimsg, _countof(uimsg), _T("&Window"), ts.UILanguageFile);
		::InsertMenu(*Menu,4,MF_STRING | MF_ENABLED | MF_POPUP | MF_BYPOSITION,
		             (UINT_PTR)WinMenu, uimsg);
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
		SetWinMenu(WinMenu, ts.UIMsg, sizeof(ts.UIMsg), ts.UILanguageFile, 0);
	}
}

#if 0
BEGIN_MESSAGE_MAP(CTEKWindow, CFrameWnd)
	//{{AFX_MSG_MAP(CTEKWindow)
	ON_WM_ACTIVATE()
	ON_WM_CHAR()
	ON_WM_DESTROY()
	ON_WM_GETMINMAXINFO()
	ON_WM_INITMENUPOPUP()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEACTIVATE()
	ON_WM_MOUSEMOVE()
	ON_WM_MOVE()
	ON_WM_PAINT()
	ON_WM_RBUTTONUP()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_SYSCOMMAND()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_TIMER()
	ON_MESSAGE(WM_USER_ACCELCOMMAND, OnAccelCommand)
	ON_MESSAGE(WM_USER_CHANGEMENU,OnChangeMenu)
	ON_MESSAGE(WM_USER_CHANGETBAR,OnChangeTBar)
	ON_MESSAGE(WM_USER_DLGHELP2,OnDlgHelp)
	ON_MESSAGE(WM_USER_GETSERIALNO,OnGetSerialNo)
	ON_COMMAND(ID_TEKFILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_TEKFILE_EXIT, OnFileExit)
	ON_COMMAND(ID_TEKEDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_TEKEDIT_COPYSCREEN, OnEditCopyScreen)
	ON_COMMAND(ID_TEKEDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_TEKEDIT_PASTECR, OnEditPasteCR)
	ON_COMMAND(ID_TEKEDIT_CLEARSCREEN, OnEditClearScreen)
	ON_COMMAND(ID_TEKSETUP_WINDOW, OnSetupWindow)
	ON_COMMAND(ID_TEKSETUP_FONT, OnSetupFont)
	ON_COMMAND(ID_TEKVTWIN, OnVTWin)
	ON_COMMAND(ID_TEKWINDOW_WINDOW, OnWindowWindow)
	ON_COMMAND(ID_TEKHELP_INDEX, OnHelpIndex)
	ON_COMMAND(ID_TEKHELP_ABOUT, OnHelpAbout)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif

/////////////////////////////////////////////////////////////////////////////
// CTEKWindow message handler

BOOL CTEKWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if ((LOWORD(wParam)>=ID_WINDOW_1) && (LOWORD(wParam)<ID_WINDOW_1+9)) {
		SelectWin(LOWORD(wParam)-ID_WINDOW_1);
		return TRUE;
	}
	else {
		return CFrameWnd::OnCommand(wParam, lParam);
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

void CTEKWindow::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	unsigned int i;
	char Code;

	if (!KeybEnabled || (TalkStatus!=IdTalkKeyb)) {
		return;
	}
	Code = nChar;

	if (tk.GIN) {
		TEKReportGIN(&tk,&ts,&cv,Code);
	}
	else {
		for (i=1 ; i<=nRepCnt ; i++) {
			CommTextOut(&cv,&Code,1);
			if (ts.LocalEcho>0) {
				CommTextEcho(&cv,&Code,1);
			}
		}
	}
}

void CTEKWindow::OnDestroy()
{
	// remove this window from the window list
	UnregWin(HTEKWin);

	CFrameWnd::OnDestroy();

	TEKEnd(&tk);
	FreeTTTEK();
	HTEKWin = NULL;
	pTEKWin = NULL;
	ActiveWin = IdVT;
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

void CTEKWindow::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	KeyDown(HTEKWin,nChar,nRepCnt,nFlags & 0x1ff);
}

void CTEKWindow::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	KeyUp(nChar);
}

void CTEKWindow::OnKillFocus(HWND hNewWnd)
{
	TEKDestroyCaret(&tk,&ts);
	CFrameWnd::OnKillFocus(hNewWnd);
}

void CTEKWindow::OnLButtonDown(UINT nFlags, POINTS point)
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
				           submenu != NULL ? (UINT)submenu : GetMenuItemID(PopupMenu, i),
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

void CTEKWindow::OnLButtonUp(UINT nFlags, POINTS point)
{
	TEKWMLButtonUp(&tk,&ts);
}

void CTEKWindow::OnMButtonUp(UINT nFlags, POINTS point)
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

void CTEKWindow::OnMouseMove(UINT nFlags, POINTS point)
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
	CFrameWnd::OnSetFocus(hOldWnd);
}

void CTEKWindow::OnSize(UINT nType, int cx, int cy)
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

void CTEKWindow::OnSysCommand(UINT nID, LPARAM lParam)
{
	if (nID==ID_SHOWMENUBAR) {
		ts.PopupMenu = 0;
		SwitchMenu();
	}
	else {
		CFrameWnd::OnSysCommand(nID,lParam);
	}
}

void CTEKWindow::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_F10) {
		OnKeyDown(nChar,nRepCnt,nFlags);
	}
	else {
		CFrameWnd::OnSysKeyDown(nChar,nRepCnt,nFlags);
	}
}

void CTEKWindow::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_F10) {
		OnKeyUp(nChar,nRepCnt,nFlags);
	}
	else {
		CFrameWnd::OnSysKeyUp(nChar,nRepCnt,nFlags);
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
			TCHAR uimsg[MAX_UIMSG];
			WinMenu = CreatePopupMenu();
			get_lang_msgT("TEKMENU_WINDOW", uimsg, _countof(uimsg), _T("&Window"), ts.UILanguageFile);
			::InsertMenu(MainMenu,4,
			             MF_STRING | MF_ENABLED | MF_POPUP | MF_BYPOSITION,
			             (UINT_PTR)WinMenu, uimsg);
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
		TCHAR uimsg[MAX_UIMSG];
		SysMenu = ::GetSystemMenu(tk.HWin,FALSE);
		AppendMenu(SysMenu, MF_SEPARATOR, 0, NULL);
		get_lang_msgT("TEKMENU_SHOW_MENUBAR", uimsg, _countof(uimsg), _T("Show menu &bar"), ts.UILanguageFile);
		AppendMenu(SysMenu, MF_STRING, ID_SHOWMENUBAR, uimsg);
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
		TCHAR uimsg[MAX_UIMSG];
		SysMenu = ::GetSystemMenu(HTEKWin,FALSE);
		AppendMenu(SysMenu, MF_SEPARATOR, 0, NULL);
		get_lang_msgT("TEKMENU_SHOW_MENUBAR", uimsg, _countof(uimsg), _T("Show menu &bar"), ts.UILanguageFile);
		AppendMenu(SysMenu, MF_STRING, ID_SHOWMENUBAR, uimsg);
	}
	return 0;
}

LRESULT CTEKWindow::OnDlgHelp(WPARAM wParam, LPARAM lParam)
{
	OpenHelp(HH_HELP_CONTEXT, HelpId, ts.UILanguageFile);
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
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	Ok = (*SetupWin)(HTEKWin, &ts);
	FreeTTDLG();
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
	FreeTTDLG();
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
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	(*WindowWindow)(HTEKWin,&Close);
	FreeTTDLG();
	if (Close) {
		OnClose();
	}
}

void CTEKWindow::OnHelpIndex()
{
	OpenHelp(HH_DISPLAY_TOPIC, 0, ts.UILanguageFile);
}

void CTEKWindow::OnHelpAbout()
{
	if (! LoadTTDLG()) {
		return;
	}
	(*AboutDialog)(tk.HWin);
	FreeTTDLG();
}

LRESULT CTEKWindow::Proc(UINT msg, WPARAM wp, LPARAM lp)
{
	LRESULT retval = 0;
	if (msg == MsgDlgHelp) {
		OnDlgHelp(wp, lp);
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
		PostQuitMessage(0);
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
	default:
		retval = DefWindowProc(msg, wp, lp);
		break;
	}
				
	return retval;
}
