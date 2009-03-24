/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TEK window */
#include "stdafx.h"
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

#define TEKClassName "TEKWin32"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTEKWindow

CTEKWindow::CTEKWindow()
{
  WNDCLASS wc;
  RECT rect;
  DWORD Style;
  int fuLoad = LR_DEFAULTCOLOR;

  if (! LoadTTTEK()) return;
  TEKInit(&tk, &ts);

  if (ts.HideTitle>0)
    Style = WS_POPUP | WS_THICKFRAME | WS_BORDER;
  else
    Style = WS_CAPTION | WS_SYSMENU |
	    WS_MINIMIZEBOX | WS_BORDER | WS_THICKFRAME;

  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = AfxWndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = AfxGetInstanceHandle();
  wc.hIcon = NULL;
  wc.hCursor = LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = TEKClassName;

  RegisterClass(&wc);

  if (ts.TEKPos.x==CW_USEDEFAULT)
    rect = rectDefault;
  else {
    rect.left = ts.TEKPos.x;
    rect.top = ts.TEKPos.y;
    rect.right = rect.left + 640; //temporary width
    rect.bottom = rect.top + 400; //temporary height
  }
  Create(TEKClassName, "Tera Term", Style, rect, GetDesktopWindow(), NULL);
//--------------------------------------------------------
  HTEKWin = GetSafeHwnd();
  if (HTEKWin == NULL) return;
  tk.HWin = HTEKWin;
  // register this window to the window list
  RegWin(HVTWin,HTEKWin);

    if (is_NT4()) {
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
  if ((ts.HideTitle==0) && (ts.PopupMenu==0))
  {
    InitMenu(&MainMenu);
    ::SetMenu(HTEKWin,MainMenu);
  }

  ChangeTitle();

  ::GetWindowRect(tk.HWin,&rect);
  TEKResizeWindow(&tk,&ts,
     rect.right-rect.left, rect.bottom-rect.top);

  if ((ts.PopupMenu>0) || (ts.HideTitle>0))
    ::PostMessage(HTEKWin,WM_USER_CHANGEMENU,0,0);

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
  *Menu = ::LoadMenu(AfxGetInstanceHandle(),
					 MAKEINTRESOURCE(IDR_TEKMENU));
  EditMenu = GetSubMenu(MainMenu,1);
  FileMenu = GetSubMenu(MainMenu,0);
  SetupMenu = GetSubMenu(MainMenu,2);
  HelpMenu = GetSubMenu(MainMenu,4);
  char uimsg[MAX_UIMSG];

  GetMenuString(*Menu, 0, uimsg, sizeof(uimsg), MF_BYPOSITION);
  get_lang_msg("TEKMENU_FILE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(*Menu, 0, MF_BYPOSITION, 0, ts.UIMsg);
  GetMenuString(FileMenu, ID_TEKFILE_PRINT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_FILE_PRINT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(FileMenu, ID_TEKFILE_PRINT, MF_BYCOMMAND, ID_TEKFILE_PRINT, ts.UIMsg);
  GetMenuString(FileMenu, ID_TEKFILE_EXIT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_FILE_EXIT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(FileMenu, ID_TEKFILE_EXIT, MF_BYCOMMAND, ID_TEKFILE_EXIT, ts.UIMsg);

  GetMenuString(*Menu, 1, uimsg, sizeof(uimsg), MF_BYPOSITION);
  get_lang_msg("TEKMENU_EDIT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(*Menu, 1, MF_BYPOSITION, 1, ts.UIMsg);
  GetMenuString(EditMenu, ID_TEKEDIT_COPY, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_EDIT_COPY", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(EditMenu, ID_TEKEDIT_COPY, MF_BYCOMMAND, ID_TEKEDIT_COPY, ts.UIMsg);
  GetMenuString(EditMenu, ID_TEKEDIT_COPYSCREEN, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_EDIT_COPYSCREEN", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(EditMenu, ID_TEKEDIT_COPYSCREEN, MF_BYCOMMAND, ID_TEKEDIT_COPYSCREEN, ts.UIMsg);
  GetMenuString(EditMenu, ID_TEKEDIT_PASTE, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_EDIT_PASTE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(EditMenu, ID_TEKEDIT_PASTE, MF_BYCOMMAND, ID_TEKEDIT_PASTE, ts.UIMsg);
  GetMenuString(EditMenu, ID_TEKEDIT_PASTECR, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_EDIT_PASTECR", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(EditMenu, ID_TEKEDIT_PASTECR, MF_BYCOMMAND, ID_TEKEDIT_PASTECR, ts.UIMsg);
  GetMenuString(EditMenu, ID_TEKEDIT_CLEARSCREEN, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_EDIT_CLSCREEN", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(EditMenu, ID_TEKEDIT_CLEARSCREEN, MF_BYCOMMAND, ID_TEKEDIT_CLEARSCREEN, ts.UIMsg);

  GetMenuString(*Menu, 2, uimsg, sizeof(uimsg), MF_BYPOSITION);
  get_lang_msg("TEKMENU_SETUP", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(*Menu, 2, MF_BYPOSITION, 2, ts.UIMsg);
  GetMenuString(SetupMenu, ID_TEKSETUP_WINDOW, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_SETUP_WINDOW", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(SetupMenu, ID_TEKSETUP_WINDOW, MF_BYCOMMAND, ID_TEKSETUP_WINDOW, ts.UIMsg);
  GetMenuString(SetupMenu, ID_TEKSETUP_FONT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_SETUP_FONT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(SetupMenu, ID_TEKSETUP_FONT, MF_BYCOMMAND, ID_TEKSETUP_FONT, ts.UIMsg);

  GetMenuString(*Menu, ID_TEKVTWIN, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_VTWIN", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(*Menu, ID_TEKVTWIN, MF_BYCOMMAND, ID_TEKVTWIN, ts.UIMsg);

  GetMenuString(*Menu, 4, uimsg, sizeof(uimsg), MF_BYPOSITION);
  get_lang_msg("TEKMENU_HELP", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(*Menu, 4, MF_BYPOSITION, 4, ts.UIMsg);
  GetMenuString(HelpMenu, ID_TEKHELP_INDEX, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_HELP_INDEX", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(HelpMenu, ID_TEKHELP_INDEX, MF_BYCOMMAND, ID_TEKHELP_INDEX, ts.UIMsg);
  GetMenuString(HelpMenu, ID_TEKHELP_ABOUT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
  get_lang_msg("TEKMENU_HELP_ABOUT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  ModifyMenu(HelpMenu, ID_TEKHELP_ABOUT, MF_BYCOMMAND, ID_TEKHELP_ABOUT, ts.UIMsg);

  if ((ts.MenuFlag & MF_SHOWWINMENU) !=0)
  {
    WinMenu = CreatePopupMenu();
    get_lang_msg("TEKMENU_WINDOW", ts.UIMsg, sizeof(ts.UIMsg), "&Window", ts.UILanguageFile);
    ::InsertMenu(*Menu,4,
		 MF_STRING | MF_ENABLED |
		 MF_POPUP | MF_BYPOSITION,
		 (int)WinMenu, ts.UIMsg);
  }
}

void CTEKWindow::InitMenuPopup(HMENU SubMenu)
{
  if (SubMenu==EditMenu)
  {
    if (tk.Select)
      EnableMenuItem(EditMenu,ID_TEKEDIT_COPY,MF_BYCOMMAND | MF_ENABLED);
    else
      EnableMenuItem(EditMenu,ID_TEKEDIT_COPY,MF_BYCOMMAND | MF_GRAYED);

    if (cv.Ready &&
	(IsClipboardFormatAvailable(CF_TEXT) ||
	 IsClipboardFormatAvailable(CF_OEMTEXT)))
    {
      EnableMenuItem(EditMenu,ID_TEKEDIT_PASTE,MF_BYCOMMAND | MF_ENABLED);
      EnableMenuItem(EditMenu,ID_TEKEDIT_PASTECR,MF_BYCOMMAND | MF_ENABLED);
    }
    else {
      EnableMenuItem(EditMenu,ID_TEKEDIT_PASTE,MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(EditMenu,ID_TEKEDIT_PASTECR,MF_BYCOMMAND | MF_GRAYED);
    }
  }
  else if ( SubMenu == WinMenu )
  {
    SetWinMenu(WinMenu, ts.UIMsg, sizeof(ts.UIMsg), ts.UILanguageFile, 0);
  }
}

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

/////////////////////////////////////////////////////////////////////////////
// CTEKWindow message handler

LRESULT CTEKWindow::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result;

  if (message == MsgDlgHelp)
  {
    OnDlgHelp(wParam,lParam);
    return 0;
  }
  else if ((ts.HideTitle>0) &&
	   (message == WM_NCHITTEST))
  {
    Result = CFrameWnd::DefWindowProc(message,wParam,lParam);
    if ((Result==HTCLIENT) && AltKey())
      Result = HTCAPTION;
    return Result;
  }

  return (CFrameWnd::DefWindowProc(message,wParam,lParam));
}

BOOL CTEKWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
  if ((LOWORD(wParam)>=ID_WINDOW_1) && (LOWORD(wParam)<ID_WINDOW_1+9))
  {
    SelectWin(LOWORD(wParam)-ID_WINDOW_1);
    return TRUE;
  }
  else
    return CFrameWnd::OnCommand(wParam, lParam);
}

void CTEKWindow::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
  if (nState!=WA_INACTIVE)
  {
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

  if (!KeybEnabled || (TalkStatus!=IdTalkKeyb)) return;
  Code = nChar;

  if (tk.GIN) TEKReportGIN(&tk,&ts,&cv,Code);
  else
    for (i=1 ; i<=nRepCnt ; i++)
    {
      CommTextOut(&cv,&Code,1);
      if (ts.LocalEcho>0)
	CommTextEcho(&cv,&Code,1);
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

void CTEKWindow::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
  lpMMI->ptMaxSize.x = 10000;
  lpMMI->ptMaxSize.y = 10000;
  lpMMI->ptMaxTrackSize.x = 10000;
  lpMMI->ptMaxTrackSize.y = 10000;
}

void CTEKWindow::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
  InitMenuPopup(pPopupMenu->m_hMenu);
}

void CTEKWindow::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  KeyDown(HTEKWin,nChar,nRepCnt,nFlags & 0x1ff);
}

void CTEKWindow::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  KeyUp(nChar);
}

void CTEKWindow::OnKillFocus(CWnd* pNewWnd)
{
  TEKDestroyCaret(&tk,&ts);
  CFrameWnd::OnKillFocus(pNewWnd);
}

void CTEKWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
  POINT p;
  HMENU PopupMenu, PopupBase;

  p.x = point.x;
  p.y = point.y;

  // popup menu
  if (ControlKey() && (MainMenu==NULL))
  {
    InitMenu(&PopupMenu);
    InitMenuPopup(EditMenu);
    if (WinMenu!=NULL)
      InitMenuPopup(WinMenu);
    PopupBase = CreatePopupMenu();
    get_lang_msg("MENU_CONTROL", ts.UIMsg, sizeof(ts.UIMsg), "&File", ts.UILanguageFile);
    AppendMenu(PopupBase, MF_STRING | MF_ENABLED | MF_POPUP,
               (UINT)GetSubMenu(PopupMenu,0), ts.UIMsg);
    get_lang_msg("TEKMENU_EDIT", ts.UIMsg, sizeof(ts.UIMsg), "&Edit", ts.UILanguageFile);
    AppendMenu(PopupBase, MF_STRING | MF_ENABLED | MF_POPUP,
               (UINT)EditMenu, ts.UIMsg);
    get_lang_msg("TEKMENU_SETUP", ts.UIMsg, sizeof(ts.UIMsg), "&Setup", ts.UILanguageFile);
    AppendMenu(PopupBase, MF_STRING | MF_ENABLED | MF_POPUP,
               (UINT)GetSubMenu(PopupMenu,2), ts.UIMsg);
    get_lang_msg("TEKMENU_VTWIN", ts.UIMsg, sizeof(ts.UIMsg), "VT-Wind&ow", ts.UILanguageFile);
    AppendMenu(PopupBase, MF_STRING | MF_ENABLED,
               ID_TEKVTWIN, ts.UIMsg);
    if (WinMenu!=NULL) {
      get_lang_msg("TEKMENU_WINDOW", ts.UIMsg, sizeof(ts.UIMsg), "&Window", ts.UILanguageFile);
      AppendMenu(PopupBase, MF_STRING | MF_ENABLED | MF_POPUP,
                 (UINT)WinMenu, ts.UIMsg);
    }
    get_lang_msg("TEKMENU_HELP", ts.UIMsg, sizeof(ts.UIMsg), "&Help", ts.UILanguageFile);
    AppendMenu(PopupBase, MF_STRING | MF_ENABLED | MF_POPUP,
               (UINT)GetSubMenu(PopupMenu,4), ts.UIMsg);
    ::ClientToScreen(tk.HWin, &p);
    TrackPopupMenu(PopupBase,TPM_LEFTALIGN | TPM_LEFTBUTTON,
                   p.x,p.y,0,tk.HWin,NULL);
    if (WinMenu!=NULL)
    {
      DestroyMenu(WinMenu);
      WinMenu = NULL;
    }
    DestroyMenu(PopupBase);
    DestroyMenu(PopupMenu);
    return;
  }

  TEKWMLButtonDown(&tk,&ts,&cv,p);
}

void CTEKWindow::OnLButtonUp(UINT nFlags, CPoint point)
{
  TEKWMLButtonUp(&tk,&ts);
}

void CTEKWindow::OnMButtonUp(UINT nFlags, CPoint point)
{
  //OnRButtonUp(nFlags,point);
}

int CTEKWindow::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
  if ((ts.SelOnActive==0) &&
      (nHitTest==HTCLIENT))   //disable mouse event for text selection
    return MA_ACTIVATEANDEAT; //     when window is activated
  else
    return MA_ACTIVATE;
}

void CTEKWindow::OnMouseMove(UINT nFlags, CPoint point)
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
  CDC *cdc;
  HDC PaintDC;

  cdc = BeginPaint(&ps);
  PaintDC = cdc->GetSafeHdc();

  TEKPaint(&tk,&ts,PaintDC,&ps);

  EndPaint(&ps);
}

void CTEKWindow::OnRButtonUp(UINT nFlags, CPoint point)
{
  CBStartPaste(tk.HWin,FALSE,0,NULL,0);
}

void CTEKWindow::OnSetFocus(CWnd* pOldWnd)
{
  TEKChangeCaret(&tk,&ts);
  CFrameWnd::OnSetFocus(pOldWnd);	
}

void CTEKWindow::OnSize(UINT nType, int cx, int cy)
{
  int w, h;
  RECT R;

  if (tk.Minimized && (nType==SIZE_RESTORED))
  {
    tk.Minimized = FALSE;
    return;
  }
  tk.Minimized = (nType==SIZE_MINIMIZED);
  if (tk.Minimized) return;

  ::GetWindowRect(tk.HWin,&R);
  w = R.right - R.left;
  h = R.bottom - R.top;

  TEKWMSize(&tk,&ts,w,h,cx,cy);
}

void CTEKWindow::OnSysCommand(UINT nID, LPARAM lParam)
{
  if (nID==ID_SHOWMENUBAR)
  {
    ts.PopupMenu = 0;
    SwitchMenu();
  }
  else CFrameWnd::OnSysCommand(nID,lParam);
}

void CTEKWindow::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  if (nChar==VK_F10)
    OnKeyDown(nChar,nRepCnt,nFlags);
  else
    CFrameWnd::OnSysKeyDown(nChar,nRepCnt,nFlags);
}

void CTEKWindow::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  if (nChar==VK_F10)
    OnKeyUp(nChar,nRepCnt,nFlags);
  else
    CFrameWnd::OnSysKeyUp(nChar,nRepCnt,nFlags);
}

void CTEKWindow::OnTimer(UINT nIDEvent)
{
  UINT T;

  if (nIDEvent==IdCaretTimer)
  {
    if (ts.NonblinkingCursor!=0)
    {
      T = GetCaretBlinkTime();
      SetCaretBlinkTime(T);
    }
    else
      ::KillTimer(HTEKWin,IdCaretTimer);
    return;
  }
  else
    ::KillTimer(HTEKWin,nIDEvent);
}

LONG CTEKWindow::OnAccelCommand(UINT wParam, LONG lParam)
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
      SelectNextWin(HTEKWin,1);
      break;
    case IdCmdPrevWin:
      SelectNextWin(HTEKWin,-1);
      break;
    case IdBreak:
    case IdCmdRestoreSetup:
    case IdCmdLoadKeyMap:
      ::PostMessage(HVTWin,WM_USER_ACCELCOMMAND,wParam,0);
      break;
  }
  return 0;
}

LONG CTEKWindow::OnChangeMenu(UINT wParam, LONG lParam)
{
  HMENU SysMenu;
  BOOL Show, B1, B2;

  Show = (ts.PopupMenu==0) && (ts.HideTitle==0);
  if (Show != (MainMenu!=NULL))
  {
    if (! Show)
    {
      if (WinMenu!=NULL)
	DestroyMenu(WinMenu);
      WinMenu = NULL;
      DestroyMenu(MainMenu);
      MainMenu = NULL;
    }
    else
      InitMenu(&MainMenu);

    tk.AdjustSize = TRUE;
    ::SetMenu(tk.HWin, MainMenu);
    ::DrawMenuBar(HTEKWin);
  }

  B1 = ((ts.MenuFlag & MF_SHOWWINMENU)!=0);
  B2 = (WinMenu!=NULL);
  if ((MainMenu!=NULL) &&
      (B1 != B2))
  {
    if (WinMenu==NULL)
    {
      WinMenu = CreatePopupMenu();
      get_lang_msg("TEKMENU_WINDOW", ts.UIMsg, sizeof(ts.UIMsg), "&Window", ts.UILanguageFile);
      ::InsertMenu(MainMenu,4,
                   MF_STRING | MF_ENABLED |
                   MF_POPUP | MF_BYPOSITION,
                   (int)WinMenu, ts.UIMsg);
    }
    else {
      RemoveMenu(MainMenu,4,MF_BYPOSITION);
      DestroyMenu(WinMenu);
      WinMenu = NULL;
    }
    ::DrawMenuBar(HTEKWin);
  }

  ::GetSystemMenu(tk.HWin,TRUE);
  if ((! Show) && ((ts.MenuFlag & MF_NOSHOWMENU)==0))
  {
    SysMenu = ::GetSystemMenu(tk.HWin,FALSE);
    AppendMenu(SysMenu, MF_SEPARATOR, 0, NULL);
    get_lang_msg("TEKMENU_SHOW_MENUBAR", ts.UIMsg, sizeof(ts.UIMsg), "Show menu &bar", ts.UILanguageFile);
    AppendMenu(SysMenu, MF_STRING, ID_SHOWMENUBAR, ts.UIMsg);
  }
  return 0;
}

LONG CTEKWindow::OnChangeTBar(UINT wParam, LONG lParam)
{
  BOOL TBar;
  DWORD Style;
  HMENU SysMenu;

  Style = GetWindowLong (HTEKWin, GWL_STYLE);
  TBar = ((Style & WS_SYSMENU)!=0);
  if (TBar == (ts.HideTitle==0)) return 0;
  if (ts.HideTitle>0)
    Style = Style & ~(WS_SYSMENU | WS_CAPTION |
		      WS_MINIMIZEBOX) | WS_BORDER | WS_POPUP;
  else
    Style = Style & ~WS_POPUP | WS_SYSMENU | WS_CAPTION |
		     WS_MINIMIZEBOX;
  tk.AdjustSize = TRUE;
  SetWindowLong (HTEKWin, GWL_STYLE, Style);
  ::SetWindowPos (HTEKWin, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
		  SWP_NOZORDER | SWP_FRAMECHANGED);
  ::ShowWindow (HTEKWin, SW_SHOW);

  if ((ts.HideTitle==0) && (MainMenu==NULL) &&
      ((ts.MenuFlag & MF_NOSHOWMENU) == 0))
  {
    SysMenu = ::GetSystemMenu(HTEKWin,FALSE);
    AppendMenu(SysMenu, MF_SEPARATOR, 0, NULL);
    get_lang_msg("TEKMENU_SHOW_MENUBAR", ts.UIMsg, sizeof(ts.UIMsg), "Show menu &bar", ts.UILanguageFile);
    AppendMenu(SysMenu, MF_STRING, ID_SHOWMENUBAR, ts.UIMsg);
  }
  return 0;
}

LONG CTEKWindow::OnDlgHelp(UINT wParam, LONG lParam)
{
  OpenHelp(tk.HWin,HH_HELP_CONTEXT,HelpId);
  return 0;
}

LONG CTEKWindow::OnGetSerialNo(UINT wParam, LONG lParam)
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
  if (PrintDC==NULL) return;
  if (! PrnStart(ts.Title)) return;

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
  CBStartPaste(tk.HWin,FALSE,0,NULL,0);
}

void CTEKWindow::OnEditPasteCR()
{
  CBStartPaste(tk.HWin,TRUE,0,NULL,0);
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

  if (! LoadTTDLG()) return;
  OldEmu = ts.TEKColorEmu;
  Ok = (*SetupWin)(HTEKWin, &ts);
  FreeTTDLG();
  if (Ok)
  {
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
  if (! LoadTTDLG()) return;
  Ok = (*ChooseFontDlg)(HTEKWin,&tk.TEKlf,&ts);
  FreeTTDLG();
  if (! Ok) return;

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
  if (! LoadTTDLG()) return;
  (*WindowWindow)(HTEKWin,&Close);
  FreeTTDLG();
  if (Close) OnClose();
}

void CTEKWindow::OnHelpIndex()
{
  OpenHelp(tk.HWin,HH_DISPLAY_TOPIC,0);
}

void CTEKWindow::OnHelpAbout()
{
  if (! LoadTTDLG()) return;
  (*AboutDialog)(tk.HWin);
  FreeTTDLG();
}
