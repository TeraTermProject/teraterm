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
/* IPv6 modification is Copyright(C) 2000 Jun-ya Kato <kato@win6.jp> */

/* TERATERM.EXE, VT window */

// SDK7.0の場合、WIN32_IEが適切に定義されない
#if _MSC_VER == 1400	// VS2005の場合のみ
#if !defined(_WIN32_IE)
#define	_WIN32_IE 0x0501
#endif
#endif

#include "teraterm.h"
#include "tttypes.h"
#include "tttypes_key.h"

#include "ttcommon.h"
#include "ttwinman.h"
#include "ttsetup.h"
#include "keyboard.h"
#include "buffer.h"
#include "vtterm.h"
#include "vtdisp.h"
#include "ttdialog.h"
#include "ttime.h"
#include "commlib.h"
#include "clipboar.h"
#include "filesys.h"
#include "telnet.h"
#include "tektypes.h"
#include "ttdde.h"
#include "ttlib.h"
#include "dlglib.h"
#include "helpid.h"
#include "teraprn.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ttplug.h"  /* TTPLUG */
#include "teraterml.h"
#include "buffer.h"

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>
#include <io.h>
#include <errno.h>

#include <shlobj.h>
#include <windows.h>
#include <windowsx.h>
#include <imm.h>
#include <dbt.h>
#include <assert.h>
#include <wchar.h>
#include <htmlhelp.h>

#include "tt_res.h"
#include "vtwin.h"
#include "addsetting.h"
#include "winjump.h"
#include "sizetip.h"
#include "dnddlg.h"
#include "tekwin.h"
#include "compat_win.h"
#include "unicode_test.h"
#if UNICODE_DEBUG
#include "tipwin.h"
#endif
#include "codeconv.h"
#include "sendmem.h"
#include "sendfiledlg.h"
#include "setting.h"
#include "broadcast.h"
#include "asprintf.h"
#include "teraprn.h"
#include "setupdirdlg.h"

#include "initguid.h"
//#include "Usbiodef.h"
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, \
             0xC0, 0x4F, 0xB9, 0x51, 0xED);

#define VTClassName L"VTWin32"

// ウィンドウ最大化ボタンを有効にする (2005.1.15 yutaka)
#define WINDOW_MAXMIMUM_ENABLED 1

static BOOL TCPLocalEchoUsed = FALSE;
static BOOL TCPCRSendUsed = FALSE;

static BOOL IgnoreRelease = FALSE;

static HDEVNOTIFY hDevNotify = NULL;

static int AutoDisconnectedPort = -1;

#ifndef WM_IME_COMPOSITION
#define WM_IME_COMPOSITION              0x010F
#endif

UnicodeDebugParam_t UnicodeDebugParam;
extern "C" PrintFile *PrintFile_;

/////////////////////////////////////////////////////////////////////////////
// CVTWindow

// Tera Term起動時とURL文字列mouse over時に呼ばれる (2005.4.2 yutaka)
static void SetMouseCursor(const char *cursor)
{
	HCURSOR hc;
	LPCTSTR name = NULL;
	int i;

	for (i = 0 ; MouseCursor[i].name ; i++) {
		if (_stricmp(cursor, MouseCursor[i].name) == 0) {
			name = MouseCursor[i].id;
			break;
		}
	}
	if (name == NULL) {
		return;
	}

	hc = (HCURSOR)LoadImage(NULL, name, IMAGE_CURSOR,
	                        0, 0, LR_DEFAULTSIZE | LR_SHARED);

	if (hc != NULL) {
		SetClassLongPtr(HVTWin, GCLP_HCURSOR, (LONG_PTR)hc);
	}
}

/**
 * @param[in]	alpha	0-255
 */
void CVTWindow::SetWindowAlpha(BYTE alpha)
{
	if (pSetLayeredWindowAttributes == NULL) {
		return;	// レイヤードウインドウのサポートなし
	}
	if (Alpha == alpha) {
		return;	// 変化なしなら何もしない
	}
	LONG_PTR lp = GetWindowLongPtr(GWL_EXSTYLE);
	if (lp == 0) {
		return;
	}

	// 2006/03/16 by 337: BGUseAlphaBlendAPIがOnならばLayered属性とする
	//if (ts->EtermLookfeel.BGUseAlphaBlendAPI) {
	// アルファ値が255の場合、画面のちらつきを抑えるため何もしないこととする。(2006.4.1 yutaka)
	// 呼び出し元で、値が変更されたときのみ設定を反映する。(2007.10.19 maya)
	if (alpha < 255) {
		::SetWindowLongPtr(HVTWin, GWL_EXSTYLE, lp | WS_EX_LAYERED);
		pSetLayeredWindowAttributes(HVTWin, 0, alpha, LWA_ALPHA);
	}
	else {
		// アルファ値が 255 の場合、透明化属性を削除して再描画する。(2007.10.22 maya)
		::SetWindowLongPtr(HVTWin, GWL_EXSTYLE, lp & ~WS_EX_LAYERED);
		::RedrawWindow(HVTWin, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME);
	}
	Alpha = alpha;
}

void RegDeviceNotify(HWND hWnd)
{
	typedef HDEVNOTIFY (WINAPI *PRegisterDeviceNotification)(HANDLE hRecipient, LPVOID NotificationFilter, DWORD Flags);
	HMODULE h;
	PRegisterDeviceNotification pRegisterDeviceNotification;
	DEV_BROADCAST_DEVICEINTERFACE filter;

	if (((h = GetModuleHandle("user32.dll")) == NULL) ||
			((pRegisterDeviceNotification = (PRegisterDeviceNotification)GetProcAddress(h, "RegisterDeviceNotificationA")) == NULL)) {
		return;
	}

	ZeroMemory(&filter, sizeof(filter));
	filter.dbcc_size = sizeof(filter);
	filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	filter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
	hDevNotify = pRegisterDeviceNotification(hWnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
}

void UnRegDeviceNotify(HWND hWnd)
{
	typedef BOOL (WINAPI *PUnregisterDeviceNotification)(HDEVNOTIFY Handle);
	HMODULE h;
	PUnregisterDeviceNotification pUnregisterDeviceNotification;

	if (((h = GetModuleHandle("user32.dll")) == NULL) ||
			((pUnregisterDeviceNotification = (PUnregisterDeviceNotification)GetProcAddress(h, "UnregisterDeviceNotification")) == NULL)) {
		return;
	}

	pUnregisterDeviceNotification(hDevNotify);
}

void SetAutoConnectPort(int port)
{
	AutoDisconnectedPort = port;
}

/////////////////////////////////////////////////////////////////////////////
// CVTWindow constructor

CVTWindow::CVTWindow(HINSTANCE hInstance)
{
	WNDCLASSW wc;
	RECT rect;
	DWORD Style;
	char *Param;
	int CmdShow;
	int fuLoad = LR_DEFAULTCOLOR;
	BOOL isFirstInstance;
	m_hInst = hInstance;

	CommInit(&cv);
	isFirstInstance = StartTeraTerm(&ts);

	TTXInit(&ts, &cv); /* TTPLUG */

	MsgDlgHelp = RegisterWindowMessage(HELPMSGSTRING);

	if (isFirstInstance) {
		/* first instance */
		if (LoadTTSET()) {
			/* read setup info from "teraterm.ini" */
			(*ReadIniFile)(ts.SetupFName, &ts);
			FreeTTSET();
		}
		else {
			abort();
		}

	} else {
		// 2つめ以降のプロセスにおいても、ディスクから TERATERM.INI を読む。(2004.11.4 yutaka)
		if (LoadTTSET()) {
			/* read setup info from "teraterm.ini" */
			(*ReadIniFile)(ts.SetupFName, &ts);
			FreeTTSET();
		}
		else {
			abort();
		}
	}

	/* Parse command line parameters*/
	// 256バイト以上のコマンドラインパラメータ指定があると、BOF(Buffer Over Flow)で
	// 落ちるバグを修正。(2007.6.12 maya)
	Param = GetCommandLine();
	if (LoadTTSET()) {
		(*ParseParam)(Param, &ts, &(TopicName[0]));
	}
	FreeTTSET();

	// DPI Aware (高DPI対応)
	if (pIsValidDpiAwarenessContext != NULL && pSetThreadDpiAwarenessContext != NULL) {
		char Temp[4];
		GetPrivateProfileString("Tera Term", "DPIAware", NULL, Temp, sizeof(Temp), ts.SetupFName);
		if (_stricmp(Temp, "on") == 0) {
			if (pIsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == TRUE) {
				pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			}
		}
	}

	// duplicate sessionの指定があるなら、共有メモリからコピーする (2004.12.7 yutaka)
	if (ts.DuplicateSession == 1) {
		CopyShmemToTTSet(&ts);
	}

	InitKeyboard();
	SetKeyMap();

	/* window status */
	AdjustSize = TRUE;
	Minimized = FALSE;
	LButton = FALSE;
	MButton = FALSE;
	RButton = FALSE;
	DblClk = FALSE;
	AfterDblClk = FALSE;
	TplClk = FALSE;
	Hold = FALSE;
	FirstPaint = TRUE;
	ScrollLock = FALSE;  // 初期値は無効 (2006.11.14 yutaka)
	Alpha = 255;
	IgnoreSizeMessage = FALSE;
#if UNICODE_DEBUG
	TipWinCodeDebug = NULL;
#endif

	// UnicodeDebugParam
	{
#if _DEBUG
		UnicodeDebugParam.CodePopupEnable = TRUE;
#else
		UnicodeDebugParam.CodePopupEnable = FALSE;
#endif
		UnicodeDebugParam.CodePopupKey1 = VK_CONTROL;
		UnicodeDebugParam.CodePopupKey2 = VK_CONTROL;
		UnicodeDebugParam.UseUnicodeApi = FALSE;
        UnicodeDebugParam.CodePageForANSIDraw = 932;
	}

	/* Initialize scroll buffer */
	UnicodeDebugParam.UseUnicodeApi = IsWindowsNTKernel() ? TRUE : FALSE;
	InitBuffer(UnicodeDebugParam.UseUnicodeApi);
	BuffSetDispCodePage(UnicodeDebugParam.CodePageForANSIDraw);

	InitDisp();

	if (ts.HideTitle>0) {
		Style = WS_VSCROLL | WS_HSCROLL |
		        WS_BORDER | WS_THICKFRAME | WS_POPUP;

#ifdef ALPHABLEND_TYPE2
		if(BGNoFrame)
			Style &= ~(WS_BORDER | WS_THICKFRAME);
#endif
	}
	else
#ifdef WINDOW_MAXMIMUM_ENABLED
		Style = WS_VSCROLL | WS_HSCROLL |
		        WS_BORDER | WS_THICKFRAME |
		        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
#else
		Style = WS_VSCROLL | WS_HSCROLL |
		        WS_BORDER | WS_THICKFRAME |
		        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
#endif

	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)ProcStub;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	//wc.hCursor = LoadCursor(NULL,IDC_IBEAM);
	wc.hCursor = NULL; // マウスカーソルは動的に変更する (2005.4.2 yutaka)
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = VTClassName;

	RegisterClassW(&wc);
	m_hAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACC));

	if (ts.VTPos.x==CW_USEDEFAULT) {
		rect = rectDefault;
	}
	else {
		rect.left = ts.VTPos.x;
		rect.top = ts.VTPos.y;
		rect.right = rect.left + 100;
		rect.bottom = rect.top + 100;
	}
	CreateW(hInstance, VTClassName, L"Tera Term", Style, rect, NULL, NULL);

	/*--------- Init2 -----------------*/
	HVTWin = GetSafeHwnd();
	if (HVTWin == NULL) return;
	// register this window to the window list
	SerialNo = RegWin(HVTWin,NULL);

	logfile_lock_initialize();
	SetMouseCursor(ts.MouseCursorName);

#ifdef ALPHABLEND_TYPE2
//<!--by AKASI
	if(BGNoFrame && ts.HideTitle > 0) {
		DWORD ExStyle = (DWORD)::GetWindowLongPtr(HVTWin,GWL_EXSTYLE);
		ExStyle &= ~WS_EX_CLIENTEDGE;
		::SetWindowLongPtr(HVTWin,GWL_EXSTYLE,ExStyle);
	}
//-->
#endif

	// USBデバイス変化通知登録
	RegDeviceNotify(HVTWin);

	if (IsWindowsNT4()) {
		fuLoad = LR_VGACOLOR;
	}
	::PostMessage(HVTWin,WM_SETICON,ICON_SMALL,
	              (LPARAM)LoadImage(hInstance,
	                                MAKEINTRESOURCE((ts.VTIcon!=IdIconDefault)?ts.VTIcon:IDI_VT),
	                                IMAGE_ICON,16,16,fuLoad));
	// Vista の Aero において Alt+Tab 切り替えで表示されるアイコンが
	// 16x16 アイコンの拡大になってしまうので、大きいアイコンも
	// セットする (2008.9.3 maya)
	::PostMessage(HVTWin,WM_SETICON,ICON_BIG,
	              (LPARAM)LoadImage(hInstance,
	                                MAKEINTRESOURCE((ts.VTIcon!=IdIconDefault)?ts.VTIcon:IDI_VT),
	                                IMAGE_ICON, 0, 0, fuLoad));

	SetCustomNotifyIcon(
		(HICON)LoadImage(
			hInstance,
			MAKEINTRESOURCE((ts.VTIcon!=IdIconDefault)?ts.VTIcon:IDI_VT),
			IMAGE_ICON, 16, 16, LR_VGACOLOR|LR_SHARED));

	MainMenu = NULL;
	WinMenu = NULL;
	if ((ts.HideTitle==0) && (ts.PopupMenu==0)) {
		InitMenu(&MainMenu);
		::SetMenu(HVTWin,MainMenu);
	}

	/* Reset Terminal */
	ResetTerminal();

	if ((ts.PopupMenu>0) || (ts.HideTitle>0)) {
		::PostMessage(HVTWin,WM_USER_CHANGEMENU,0,0);
	}

	ChangeFont();

	ResetIME();

	BuffChangeWinSize(NumOfColumns,NumOfLines);

	ChangeTitle();
	/* Enable drag-drop */
	::DragAcceptFiles(HVTWin,TRUE);

	if (ts.HideWindow>0) {
		if (strlen(TopicName)>0) {
			InitDDE();
			SendDDEReady();
		}
		FirstPaint = FALSE;
		Startup();
		return;
	}
	CmdShow = SW_SHOWDEFAULT;
	if (ts.Minimize>0) {
		CmdShow = SW_SHOWMINIMIZED;
	}
	SetWindowAlpha(ts.AlphaBlendActive);
	ShowWindow(CmdShow);
	ChangeCaret();

	DropLists = NULL;
	DropListCount = 0;

#if UNICODE_DEBUG
	CtrlKeyState = 0;
#endif

	// TipWin
	TipWin = new CTipWin(hInstance);
	TipWin->Create(HVTWin);
}

/////////////////////////////////////////////////////////////////////////////
// CVTWindow destructor

CVTWindow::~CVTWindow()
{
	TipWin->Destroy();
	delete TipWin;
	TipWin = NULL;
}

/////////////////////////////////////////////////////////////////////////////

int CVTWindow::Parse()
{
	// added ScrollLock (2006.11.14 yutaka)
	if (LButton || MButton || RButton || ScrollLock)
		return 0;
	return (VTParse()); // Parse received characters
}

void CVTWindow::ButtonUp(BOOL Paste)
{
	BOOL disableBuffEndSelect = false;

	/* disable autoscrolling */
	::KillTimer(HVTWin,IdScrollTimer);
	ReleaseCapture();

	if (ts.SelectOnlyByLButton &&
	    (MButton || RButton)) {
		disableBuffEndSelect = true;
	}

	LButton = FALSE;
	MButton = FALSE;
	RButton = FALSE;
	DblClk = FALSE;
	TplClk = FALSE;
	CaretOn();

	// SelectOnlyByLButton が on で 中・右クリックしたときに
	// バッファが選択状態だったら、選択内容がクリップボードに
	// コピーされてしまう問題を修正 (2007.12.6 maya)
	if (!disableBuffEndSelect) {
		wchar_t *strW = BuffEndSelect();
		if (strW != NULL) {
			CBSetTextW(HVTWin, strW, 0);
		}
	}

	if (Paste) {
		CBStartPaste(HVTWin, FALSE, BracketedPasteMode());

		// スクロール位置をリセット
		if (WinOrgY != 0) {
			DispVScroll(SCROLL_BOTTOM, 0);
		}
	}
}

void CVTWindow::ButtonDown(POINT p, int LMR)
{
	HMENU PopupMenu, PopupBase;
	BOOL mousereport;

	if ((LMR==IdLeftButton) && ControlKey() && (MainMenu==NULL) &&
	    ((ts.MenuFlag & MF_NOPOPUP)==0)) {
		int i, numItems;

		InitMenu(&PopupMenu);

		PopupBase = CreatePopupMenu();
		numItems = GetMenuItemCount(PopupMenu);

		for (i = 0; i < numItems; i++) {
			wchar_t itemText[256];
			HMENU submenu = GetSubMenu(PopupMenu, i);

			if (submenu != NULL) {
				InitMenuPopup(submenu);
			}

			if (GetMenuStringW(PopupMenu, i, itemText, _countof(itemText), MF_BYPOSITION) != 0) {
				int state = GetMenuState(PopupMenu, i, MF_BYPOSITION) &
				            (MF_CHECKED | MF_DISABLED | MF_GRAYED | MF_HILITE |
				             MF_MENUBARBREAK | MF_MENUBREAK | MF_SEPARATOR);

				AppendMenuW(PopupBase,
							submenu != NULL ? LOBYTE(state) | MF_POPUP : state,
							submenu != NULL ? (UINT_PTR)submenu : GetMenuItemID(PopupMenu, i),
							itemText);
			}
		}

		::ClientToScreen(HVTWin, &p);
		TrackPopupMenu(PopupBase,TPM_LEFTALIGN | TPM_LEFTBUTTON,
		               p.x,p.y,0,HVTWin,NULL);
		if (WinMenu!=NULL) {
			DestroyMenu(WinMenu);
			WinMenu = NULL;
		}
		DestroyMenu(PopupBase);
		DestroyMenu(PopupMenu);
		PopupMenu = 0;
		return;
	}

	mousereport = MouseReport(IdMouseEventBtnDown, LMR, p.x, p.y);
	if (mousereport) {
		::SetCapture(m_hWnd);
		return;
	}

	// added ConfirmPasteMouseRButton (2007.3.17 maya)
	if ((LMR == IdRightButton) &&
		(ts.PasteFlag & CPF_DISABLE_RBUTTON) == 0 &&
		(ts.PasteFlag & CPF_CONFIRM_RBUTTON) != 0 &&
		cv.Ready &&
		!mousereport &&
		IsSendVarNULL() && IsFileVarNULL() &&
		(cv.PortType!=IdFile) &&
		(IsClipboardFormatAvailable(CF_TEXT) ||
		 IsClipboardFormatAvailable(CF_OEMTEXT))) {

		int i, numItems;

		InitPasteMenu(&PopupMenu);
		PopupBase = CreatePopupMenu();
		numItems = GetMenuItemCount(PopupMenu);

		for (i = 0; i < numItems; i++) {
			wchar_t itemText[256];
			if (GetMenuStringW(PopupMenu, i, itemText, _countof(itemText), MF_BYPOSITION) != 0) {
				int state = GetMenuState(PopupMenu, i, MF_BYPOSITION) &
				            (MF_CHECKED | MF_DISABLED | MF_GRAYED | MF_HILITE |
				             MF_MENUBARBREAK | MF_MENUBREAK | MF_SEPARATOR);

				AppendMenuW(PopupBase, state,
							GetMenuItemID(PopupMenu, i), itemText);
			}
		}

		::ClientToScreen(HVTWin, &p);
		TrackPopupMenu(PopupBase,TPM_LEFTALIGN | TPM_LEFTBUTTON,
		               p.x,p.y,0,HVTWin,NULL);
		if (WinMenu!=NULL) {
			DestroyMenu(WinMenu);
			WinMenu = NULL;
		}
		DestroyMenu(PopupBase);
		DestroyMenu(PopupMenu);
		PopupMenu = 0;
		return;
	}

	if (AfterDblClk && (LMR==IdLeftButton) &&
	    (abs(p.x-DblClkX)<=GetSystemMetrics(SM_CXDOUBLECLK)) &&
	    (abs(p.y-DblClkY)<=GetSystemMetrics(SM_CYDOUBLECLK))) {
		/* triple click */
		::KillTimer(HVTWin, IdDblClkTimer);
		AfterDblClk = FALSE;
		BuffTplClk(p.y);
		LButton = TRUE;
		TplClk = TRUE;
		/* for AutoScrolling */
		::SetCapture(HVTWin);
		::SetTimer(HVTWin, IdScrollTimer, 100, NULL);
	}
	else {
		if (! (LButton || MButton || RButton)) {
			BOOL box = FALSE;

			// select several pages of output from Tera Term window (2005.5.15 yutaka)
			if (LMR == IdLeftButton && ShiftKey()) {
				BuffSeveralPagesSelect(p.x, p.y);

			} else {
				// Select rectangular block with Alt Key. Delete Shift key.(2005.5.15 yutaka)
				if (LMR == IdLeftButton && AltKey()) {
					box = TRUE;
				}

				// Starting the selection only by a left button.(2007.11.20 maya)
				if (!ts.SelectOnlyByLButton ||
				    (ts.SelectOnlyByLButton && LMR == IdLeftButton) ) {
					BuffStartSelect(p.x,p.y, box);
					TplClk = FALSE;

					/* for AutoScrolling */
					::SetCapture(HVTWin);
					::SetTimer(HVTWin, IdScrollTimer, 100, NULL);
				}
			}
		}

		switch (LMR) {
			case IdRightButton:
				RButton = TRUE;
				break;
			case IdMiddleButton:
				MButton = TRUE;
				break;
			case IdLeftButton:
				LButton = TRUE;
				break;
		}
	}
}

// LogMeIn.exe -> LogMeTT.exe へリネーム (2005.2.21 yutaka)
static char LogMeTTMenuString[] = "Log&MeTT";
static char LogMeTT[MAX_PATH];

#define IS_LOGMETT_NOTFOUND     0
#define IS_LOGMETT_FOUND        1
#define IS_LOGMETT_UNKNOWN      2

static BOOL isLogMeTTExist()
{
	const char *LogMeTTexename = "LogMeTT.exe";
	LONG result;
	HKEY key;
	int inregist = 0;
	DWORD dwSize;
	DWORD dwType;
	DWORD dwDisposition;
	char *path;

	static int status = IS_LOGMETT_UNKNOWN;

	if (status != IS_LOGMETT_UNKNOWN) {
		return status == IS_LOGMETT_FOUND;
	}

	/* LogMeTT 2.9.6からはレジストリにインストールパスが含まれる。*/
	result = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\LogMeTT", 0, NULL,
				REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &key, &dwDisposition);
	if (result == ERROR_SUCCESS) {
		result = RegQueryValueEx(key, "InstallPath", NULL, &dwType, NULL, &dwSize);
		if (result == ERROR_SUCCESS) {
			path = (char *)malloc(dwSize);
			if (path != NULL) {
				result = RegQueryValueEx(key, "InstallPath", NULL, &dwType, (LPBYTE)path, &dwSize);
				if (result == ERROR_SUCCESS) {
					inregist = 1;
					strncpy_s(LogMeTT, sizeof(LogMeTT), path, _TRUNCATE);
				}
				free(path);
			}
		}
		RegCloseKey(key);
	}

	if (inregist == 0) {
		strncpy_s(LogMeTT, sizeof(LogMeTT), ts.HomeDir, _TRUNCATE);
		AppendSlash(LogMeTT, sizeof(LogMeTT));
		strncat_s(LogMeTT, sizeof(LogMeTT), LogMeTTexename, _TRUNCATE);
	}

	if (_access(LogMeTT, 0) == -1) {
		status = IS_LOGMETT_NOTFOUND;
		return FALSE;
	}
	status = IS_LOGMETT_FOUND;
	return TRUE;
}

void CVTWindow::InitMenu(HMENU *Menu)
{
	static const DlgTextInfo MenuTextInfo[] = {
		{ ID_FILE, "MENU_FILE" },
		{ ID_EDIT, "MENU_EDIT" },
		{ ID_SETUP, "MENU_SETUP" },
		{ ID_CONTROL, "MENU_CONTROL" },
		{ ID_HELPMENU, "MENU_HELP" },
	};
	static const DlgTextInfo FileMenuTextInfo[] = {
		{ ID_FILE_NEWCONNECTION, "MENU_FILE_NEW" },
		{ ID_FILE_DUPLICATESESSION, "MENU_FILE_DUPLICATE" },
		{ ID_FILE_CYGWINCONNECTION, "MENU_FILE_GYGWIN" },
		{ ID_FILE_LOG, "MENU_FILE_LOG" },
		{ ID_FILE_COMMENTTOLOG, "MENU_FILE_COMMENTLOG" },
		{ ID_FILE_VIEWLOG, "MENU_FILE_VIEWLOG" },
		{ ID_FILE_SHOWLOGDIALOG, "MENU_FILE_SHOWLOGDIALOG" },
		{ ID_FILE_PAUSELOG, "MENU_FILE_PAUSELOG" },
		{ ID_FILE_STOPLOG, "MENU_FILE_STOPLOG" },
		{ ID_FILE_SENDFILE, "MENU_FILE_SENDFILE" },
		{ ID_FILE_REPLAYLOG, "MENU_FILE_REPLAYLOG" },
		{ ID_FILE_CHANGEDIR, "MENU_FILE_CHANGEDIR" },
		{ ID_FILE_LOGMEIN, "MENU_FILE_LOGMETT" },
		{ ID_FILE_PRINT2, "MENU_FILE_PRINT" },
		{ ID_FILE_DISCONNECT, "MENU_FILE_DISCONNECT" },
		{ ID_FILE_EXIT, "MENU_FILE_EXIT" },
		{ ID_FILE_EXITALL, "MENU_FILE_EXITALL" },
		{ 11, "MENU_TRANS" },
		{ ID_FILE_KERMITRCV, "MENU_TRANS_KERMIT_RCV" },
		{ ID_FILE_KERMITGET, "MENU_TRANS_KERMIT_GET" },
		{ ID_FILE_KERMITSEND, "MENU_TRANS_KERMIT_SEND" },
		{ ID_FILE_KERMITFINISH, "MENU_TRANS_KERMIT_FINISH" },
		{ ID_FILE_XRCV, "MENU_TRANS_X_RCV" },
		{ ID_FILE_XSEND, "MENU_TRANS_X_SEND" },
		{ ID_FILE_YRCV, "MENU_TRANS_Y_RCV" },
		{ ID_FILE_YSEND, "MENU_TRANS_Y_SEND" },
		{ ID_FILE_ZRCV, "MENU_TRANS_Z_RCV" },
		{ ID_FILE_ZSEND, "MENU_TRANS_Z_SEND" },
		{ ID_FILE_BPRCV, "MENU_TRANS_BP_RCV" },
		{ ID_FILE_BPSEND, "MENU_TRANS_BP_SEND" },
		{ ID_FILE_QVRCV, "MENU_TRANS_QV_RCV" },
		{ ID_FILE_QVSEND, "MENU_TRANS_QV_SEND" },
	};
	static const DlgTextInfo EditMenuTextInfo[] = {
		{ ID_EDIT_COPY2, "MENU_EDIT_COPY" },
		{ ID_EDIT_COPYTABLE, "MENU_EDIT_COPYTABLE" },
		{ ID_EDIT_PASTE2, "MENU_EDIT_PASTE" },
		{ ID_EDIT_PASTECR, "MENU_EDIT_PASTECR" },
		{ ID_EDIT_CLEARSCREEN, "MENU_EDIT_CLSCREEN" },
		{ ID_EDIT_CLEARBUFFER, "MENU_EDIT_CLBUFFER" },
		{ ID_EDIT_CANCELSELECT, "MENU_EDIT_CANCELSELECT" },
		{ ID_EDIT_SELECTSCREEN, "MENU_EDIT_SELECTSCREEN" },
		{ ID_EDIT_SELECTALL, "MENU_EDIT_SELECTALL" },
	};
	static const DlgTextInfo SetupMenuTextInfo[] = {
		{ ID_SETUP_TERMINAL, "MENU_SETUP_TERMINAL" },
		{ ID_SETUP_WINDOW, "MENU_SETUP_WINDOW" },
		{ ID_SETUP_FONT, "MENU_SETUP_FONT" },
		{ ID_SETUP_DLG_FONT, "MENU_SETUP_DIALOG_FONT" },
		{ 2, "MENU_SETUP_FONT_SUBMENU" },
		{ ID_SETUP_KEYBOARD, "MENU_SETUP_KEYBOARD" },
		{ ID_SETUP_SERIALPORT, "MENU_SETUP_SERIALPORT" },
		{ ID_SETUP_TCPIP, "MENU_SETUP_TCPIP" },
		{ ID_SETUP_GENERAL, "MENU_SETUP_GENERAL" },
		{ ID_SETUP_ADDITIONALSETTINGS, "MENU_SETUP_ADDITION" },
		{ ID_SETUP_SAVE, "MENU_SETUP_SAVE" },
		{ ID_SETUP_RESTORE, "MENU_SETUP_RESTORE" },
		{ ID_OPEN_SETUP, "MENU_SETUP_OPENSETUP" },
		{ ID_SETUP_LOADKEYMAP, "MENU_SETUP_LOADKEYMAP" },
	};
	static const DlgTextInfo ControlMenuTextInfo[] = {
		{ ID_CONTROL_RESETTERMINAL, "MENU_CONTROL_RESET" },
		{ ID_CONTROL_RESETREMOTETITLE, "MENU_CONTROL_RESETTITLE" },
		{ ID_CONTROL_AREYOUTHERE, "MENU_CONTROL_AREYOUTHERE" },
		{ ID_CONTROL_SENDBREAK, "MENU_CONTROL_SENDBREAK" },
		{ ID_CONTROL_RESETPORT, "MENU_CONTROL_RESETPORT" },
		{ ID_CONTROL_BROADCASTCOMMAND, "MENU_CONTROL_BROADCAST" },
		{ ID_CONTROL_OPENTEK, "MENU_CONTROL_OPENTEK" },
		{ ID_CONTROL_CLOSETEK, "MENU_CONTROL_CLOSETEK" },
		{ ID_CONTROL_MACRO, "MENU_CONTROL_MACRO" },
		{ ID_CONTROL_SHOW_MACRO, "MENU_CONTROL_SHOW_MACRO" },
	};
	static const DlgTextInfo HelpMenuTextInfo[] = {
		{ ID_HELP_INDEX2, "MENU_HELP_INDEX" },
		{ ID_HELP_ABOUT, "MENU_HELP_ABOUT" },
	};

	HMENU hMenu = LoadMenu(m_hInst, MAKEINTRESOURCE(IDR_MENU));
	*Menu = hMenu;

	FileMenu = GetSubMenu(hMenu,ID_FILE);
	TransMenu = GetSubMenu(FileMenu,ID_TRANSFER);
	EditMenu = GetSubMenu(hMenu,ID_EDIT);
	SetupMenu = GetSubMenu(hMenu,ID_SETUP);
	ControlMenu = GetSubMenu(hMenu,ID_CONTROL);
	HelpMenu = GetSubMenu(hMenu,ID_HELPMENU);

	SetDlgMenuTextsW(hMenu, MenuTextInfo, _countof(MenuTextInfo), ts.UILanguageFileW);

	/* LogMeTT の存在を確認してメニューを追加する */
	if (isLogMeTTExist()) {
		::InsertMenu(FileMenu, ID_FILE_PRINT2, MF_STRING | MF_ENABLED | MF_BYCOMMAND,
		             ID_FILE_LOGMEIN, LogMeTTMenuString);
		::InsertMenu(FileMenu, ID_FILE_PRINT2, MF_SEPARATOR, NULL, NULL);
	}

	SetDlgMenuTextsW(FileMenu, FileMenuTextInfo, _countof(FileMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(EditMenu, EditMenuTextInfo, _countof(EditMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(SetupMenu, SetupMenuTextInfo, _countof(SetupMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(ControlMenu, ControlMenuTextInfo, _countof(ControlMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(HelpMenu, HelpMenuTextInfo, _countof(HelpMenuTextInfo), ts.UILanguageFileW);

	if ((ts.MenuFlag & MF_SHOWWINMENU) !=0) {
		wchar_t uimsg[MAX_UIMSG];
		WinMenu = CreatePopupMenu();
		get_lang_msgW("MENU_WINDOW", uimsg, _countof(uimsg),
					  L"&Window", ts.UILanguageFile);
		InsertMenuW(hMenu, ID_HELPMENU,
					MF_STRING | MF_ENABLED | MF_POPUP | MF_BYPOSITION,
					(UINT_PTR)WinMenu, uimsg);
	}

	TTXModifyMenu(hMenu); /* TTPLUG */
}

void CVTWindow::InitMenuPopup(HMENU SubMenu)
{
	if ( SubMenu == FileMenu )
	{
		if (ts.DisableMenuNewConnection) {
			if ( Connecting || cv.Open ) {
				EnableMenuItem(FileMenu,ID_FILE_NEWCONNECTION,MF_BYCOMMAND | MF_GRAYED);
			}
			else {
				EnableMenuItem(FileMenu,ID_FILE_NEWCONNECTION,MF_BYCOMMAND | MF_ENABLED);
			}
		}
		else {
			if ( Connecting ) {
				EnableMenuItem(FileMenu,ID_FILE_NEWCONNECTION,MF_BYCOMMAND | MF_GRAYED);
			}
			else {
				EnableMenuItem(FileMenu,ID_FILE_NEWCONNECTION,MF_BYCOMMAND | MF_ENABLED);
			}
		}

		if ( (! cv.Ready) || (!IsSendVarNULL()) ||
		     (!IsFileVarNULL()) || (cv.PortType==IdFile) ) {
			EnableMenuItem(FileMenu,ID_FILE_SENDFILE,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_TRANSFER,MF_BYPOSITION | MF_GRAYED); /* Transfer */
			EnableMenuItem(FileMenu,ID_FILE_CHANGEDIR,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_DISCONNECT,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_DUPLICATESESSION,MF_BYCOMMAND | MF_GRAYED);
		}
		else {
			EnableMenuItem(FileMenu,ID_FILE_SENDFILE,MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_TRANSFER,MF_BYPOSITION | MF_ENABLED); /* Transfer */
			EnableMenuItem(FileMenu,ID_FILE_CHANGEDIR,MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_DISCONNECT,MF_BYCOMMAND | MF_ENABLED);
			if (ts.DisableMenuDuplicateSession) {
				EnableMenuItem(FileMenu,ID_FILE_DUPLICATESESSION,MF_BYCOMMAND | MF_GRAYED);
			}
			else {
				EnableMenuItem(FileMenu,ID_FILE_DUPLICATESESSION,MF_BYCOMMAND | MF_ENABLED);
			}
		}

		// 新規メニューを追加 (2004.12.5 yutaka)
		EnableMenuItem(FileMenu,ID_FILE_CYGWINCONNECTION,MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(FileMenu,ID_FILE_TERATERMMENU,MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(FileMenu,ID_FILE_LOGMEIN,MF_BYCOMMAND | MF_ENABLED);

		// XXX: この位置にしないと、logがグレイにならない。 (2005.2.1 yutaka)
		if (FLogIsOpend()) { // ログ採取モードの場合
			EnableMenuItem(FileMenu,ID_FILE_LOG,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_COMMENTTOLOG, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_VIEWLOG, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_SHOWLOGDIALOG, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_PAUSELOG, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_STOPLOG, MF_BYCOMMAND | MF_ENABLED);
			if (FLogIsPause()) {
				CheckMenuItem(FileMenu,ID_FILE_PAUSELOG, MF_BYCOMMAND | MF_CHECKED);
			}
			else {
				CheckMenuItem(FileMenu,ID_FILE_PAUSELOG, MF_BYCOMMAND | MF_UNCHECKED);
			}
		} else {
			EnableMenuItem(FileMenu,ID_FILE_LOG,MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_COMMENTTOLOG, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_VIEWLOG, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_SHOWLOGDIALOG, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_PAUSELOG, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_STOPLOG, MF_BYCOMMAND | MF_GRAYED);

			CheckMenuItem(FileMenu,ID_FILE_PAUSELOG, MF_BYCOMMAND | MF_UNCHECKED);
		}

	}
	else if ( SubMenu == TransMenu )
	{
		if ((cv.PortType==IdSerial) &&
		    ((ts.DataBit==IdDataBit7) || (ts.Flow==IdFlowX))) {
			EnableMenuItem(TransMenu,1,MF_BYPOSITION | MF_GRAYED);  /* XMODEM */
			EnableMenuItem(TransMenu,4,MF_BYPOSITION | MF_GRAYED);  /* Quick-VAN */
		}
		else {
			EnableMenuItem(TransMenu,1,MF_BYPOSITION | MF_ENABLED); /* XMODEM */
			EnableMenuItem(TransMenu,4,MF_BYPOSITION | MF_ENABLED); /* Quick-VAN */
		}
		if ((cv.PortType==IdSerial) &&
		    (ts.DataBit==IdDataBit7)) {
			EnableMenuItem(TransMenu,2,MF_BYPOSITION | MF_GRAYED); /* ZMODEM */
			EnableMenuItem(TransMenu,3,MF_BYPOSITION | MF_GRAYED); /* B-Plus */
		}
		else {
			EnableMenuItem(TransMenu,2,MF_BYPOSITION | MF_ENABLED); /* ZMODEM */
			EnableMenuItem(TransMenu,3,MF_BYPOSITION | MF_ENABLED); /* B-Plus */
		}
	}
	else if (SubMenu == EditMenu)
	{
		if (Selected) {
			EnableMenuItem(EditMenu,ID_EDIT_COPY2,MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(EditMenu,ID_EDIT_COPYTABLE,MF_BYCOMMAND | MF_ENABLED);
		}
		else {
			EnableMenuItem(EditMenu,ID_EDIT_COPY2,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(EditMenu,ID_EDIT_COPYTABLE,MF_BYCOMMAND | MF_GRAYED);
		}
		if (cv.Ready &&
		    IsSendVarNULL() && IsFileVarNULL() &&
		    (cv.PortType!=IdFile) &&
		    (IsClipboardFormatAvailable(CF_TEXT) ||
		    IsClipboardFormatAvailable(CF_OEMTEXT))) {
			EnableMenuItem(EditMenu,ID_EDIT_PASTE2,MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(EditMenu,ID_EDIT_PASTECR,MF_BYCOMMAND | MF_ENABLED);
		}
		else {
			EnableMenuItem(EditMenu,ID_EDIT_PASTE2,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(EditMenu,ID_EDIT_PASTECR,MF_BYCOMMAND | MF_GRAYED);
		}
	}
	else if (SubMenu == SetupMenu)
		/*
		 * ネットワーク接続中(TCP/IPを選択して接続した状態)はシリアルポート
		 * (ID_SETUP_SERIALPORT)のメニューが選択できないようになっていたが、
		 * このガードを外し、シリアルポート設定ダイアログから新しい接続ができるようにする。
		 */
		if (!IsSendVarNULL() || !IsFileVarNULL() || Connecting) {
			EnableMenuItem(SetupMenu,ID_SETUP_SERIALPORT,MF_BYCOMMAND | MF_GRAYED);
		}
		else {
			EnableMenuItem(SetupMenu,ID_SETUP_SERIALPORT,MF_BYCOMMAND | MF_ENABLED);
		}

	else if (SubMenu == ControlMenu)
	{
		if (cv.Ready &&
		    IsSendVarNULL() && IsFileVarNULL()) {
			if (ts.DisableMenuSendBreak) {
				EnableMenuItem(ControlMenu,ID_CONTROL_SENDBREAK,MF_BYCOMMAND | MF_GRAYED);
			}
			else {
				EnableMenuItem(ControlMenu,ID_CONTROL_SENDBREAK,MF_BYCOMMAND | MF_ENABLED);
			}
			if (cv.PortType==IdSerial) {
				EnableMenuItem(ControlMenu,ID_CONTROL_RESETPORT,MF_BYCOMMAND | MF_ENABLED);
			}
			else {
				EnableMenuItem(ControlMenu,ID_CONTROL_RESETPORT,MF_BYCOMMAND | MF_GRAYED);
			}
		}
		else {
			EnableMenuItem(ControlMenu,ID_CONTROL_SENDBREAK,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(ControlMenu,ID_CONTROL_RESETPORT,MF_BYCOMMAND | MF_GRAYED);
		}

		if (cv.Ready && cv.TelFlag && IsFileVarNULL()) {
			EnableMenuItem(ControlMenu,ID_CONTROL_AREYOUTHERE,MF_BYCOMMAND | MF_ENABLED);
		}
		else {
			EnableMenuItem(ControlMenu,ID_CONTROL_AREYOUTHERE,MF_BYCOMMAND | MF_GRAYED);
		}

		if (HTEKWin==0) {
			EnableMenuItem(ControlMenu,ID_CONTROL_CLOSETEK,MF_BYCOMMAND | MF_GRAYED);
		}
		else {
			EnableMenuItem(ControlMenu,ID_CONTROL_CLOSETEK,MF_BYCOMMAND | MF_ENABLED);
		}

		if (DDELog || !IsFileVarNULL()) {
			EnableMenuItem(ControlMenu,ID_CONTROL_MACRO,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(ControlMenu,ID_CONTROL_SHOW_MACRO,MF_BYCOMMAND | MF_ENABLED);
		}
		else {
			EnableMenuItem(ControlMenu,ID_CONTROL_MACRO,MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(ControlMenu,ID_CONTROL_SHOW_MACRO,MF_BYCOMMAND | MF_GRAYED);
		}

	}
	else if (SubMenu == WinMenu)
	{
		SetWinMenu(WinMenu, ts.UIMsg, sizeof(ts.UIMsg), ts.UILanguageFile, 1);
	}

	TTXModifyPopupMenu(SubMenu); /* TTPLUG */
}

// added ConfirmPasteMouseRButton (2007.3.17 maya)
void CVTWindow::InitPasteMenu(HMENU *Menu)
{
	static const DlgTextInfo MenuTextInfo[] = {
		{ ID_EDIT_PASTE2, "MENU_EDIT_PASTE" },
		{ ID_EDIT_PASTECR, "MENU_EDIT_PASTECR" },
	};
	*Menu = LoadMenu(m_hInst,
	                 MAKEINTRESOURCE(IDR_PASTEMENU));
	SetDlgMenuTextsW(*Menu, MenuTextInfo, _countof(MenuTextInfo), ts.UILanguageFileW);
}

void CVTWindow::ResetSetup()
{
	ChangeFont();
	BuffChangeWinSize(WinWidth,WinHeight);
	ChangeCaret();

	if (cv.Ready) {
		ts.PortType = cv.PortType;
		if (cv.PortType==IdSerial) {
			/* if serial port, change port parameters */
			ts.ComPort = cv.ComPort;
			CommResetSerial(&ts, &cv, TRUE);
		}
	}

	/* setup terminal */
	SetupTerm();

	/* background and ANSI color */
#ifdef ALPHABLEND_TYPE2
	BGInitialize(FALSE);
	BGSetupPrimary(TRUE);
	// 2006/03/17 by 337 : Alpha値も即時変更
	// Layered窓になっていない場合は効果が無い
	//
	// AlphaBlend を即時反映できるようにする。
	// (2016.12.24 yutaka)
	SetWindowAlpha(ts.AlphaBlendActive);
#else
	DispApplyANSIColor();
#endif
	DispSetNearestColors(IdBack, IdFore+8, NULL);

	/* setup window */
	ChangeWin();

	/* Language & IME */
	ResetIME();

	/* change TEK window */
	if (pTEKWin != NULL)
		((CTEKWindow *)pTEKWin)->RestoreSetup();
}

void CVTWindow::RestoreSetup()
{
	char TempDir[MAXPATHLEN];
	char TempName[MAX_PATH];

	if ( strlen(ts.SetupFName)==0 ) {
		return;
	}

	ExtractFileName(ts.SetupFName,TempName,sizeof(TempName));
	ExtractDirName(ts.SetupFName,TempDir);
	if (TempDir[0]==0)
		strncpy_s(TempDir, sizeof(TempDir),ts.HomeDir, _TRUNCATE);
	FitFileName(TempName,sizeof(TempName),".INI");

	strncpy_s(ts.SetupFName, sizeof(ts.SetupFName),TempDir, _TRUNCATE);
	AppendSlash(ts.SetupFName,sizeof(ts.SetupFName));
	strncat_s(ts.SetupFName,sizeof(ts.SetupFName),TempName,_TRUNCATE);

	if (LoadTTSET()) {
		(*ReadIniFile)(ts.SetupFName,&ts);
	}
	FreeTTSET();

	ResetSetup();
}

/* called by the [Setup] Terminal command */
void CVTWindow::SetupTerm()
{
//	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		ResetCharSet();
//	}
	cv.CRSend = ts.CRSend;

	// for russian mode
	cv.RussHost = ts.RussHost;

	if (cv.Ready) {
		if (cv.TelFlag && (ts.TelEcho>0)) {
			TelChangeEcho();
		}
	}

	if ((ts.TerminalWidth!=NumOfColumns) ||
	    (ts.TerminalHeight!=NumOfLines-StatusLine)) {
		LockBuffer();
		HideStatusLine();
		ChangeTerminalSize(ts.TerminalWidth,
		                   ts.TerminalHeight);
		UnlockBuffer();
	}
	else if ((ts.TermIsWin>0) &&
	         ((ts.TerminalWidth!=WinWidth) ||
	          (ts.TerminalHeight!=WinHeight-StatusLine))) {
		BuffChangeWinSize(ts.TerminalWidth,ts.TerminalHeight+StatusLine);
	}

	ChangeTerminalID();
}

void CVTWindow::Startup()
{
	/* auto log */
	/* OnCommOpen で開始されるのでここでは開始しない (2007.5.14 maya) */

	if ((TopicName[0]==0) && (ts.MacroFN[0]!=0)) {
		// start the macro specified in the command line or setup file
		RunMacro(ts.MacroFN,TRUE);
		ts.MacroFN[0] = 0;
	}
	else {// start connection
		if (TopicName[0]!=0) {
			cv.NoMsg=1; /* suppress error messages */
		}
		::PostMessage(HVTWin,WM_USER_COMMSTART,0,0);
	}
}

void CVTWindow::OpenTEK()
{
	ActiveWin = IdTEK;
	if (HTEKWin==NULL) {
		pTEKWin = new CTEKWindow(m_hInst);
	}
	else {
		::ShowWindow(HTEKWin,SW_SHOWNORMAL);
		::SetFocus(HTEKWin);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CVTWindow message handler

BOOL CVTWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	WORD wID = LOWORD(wParam);
	WORD wNotifyCode = HIWORD(wParam);

	if (wNotifyCode==1) {
		switch (wID) {
			case ID_ACC_SENDBREAK:
				// added DisableAcceleratorSendBreak (2007.3.17 maya)
				if (!ts.DisableAcceleratorSendBreak)
					OnControlSendBreak();
				return TRUE;
			case ID_ACC_AREYOUTHERE:
				OnControlAreYouThere();
				return TRUE;
		}
		if (ActiveWin==IdVT) {
			switch (wID) {
				case ID_ACC_NEWCONNECTION:
					if (ts.AcceleratorNewConnection)
						OnFileNewConnection();
					return TRUE;
				case ID_ACC_DUPLICATESESSION:
					// added DisableAcceleratorDuplicateSession (2009.4.6 maya)
					if (!ts.DisableAcceleratorDuplicateSession)
						OnDuplicateSession();
					return TRUE;
				case ID_ACC_CYGWINCONNECTION:
					if (ts.AcceleratorCygwinConnection)
						OnCygwinConnection();
					return TRUE;
				case ID_ACC_DISCONNECT:
					Disconnect(TRUE);
					return TRUE;
				case ID_ACC_COPY:
					OnEditCopy();
					return TRUE;
				case ID_ACC_PASTECR:
					OnEditPasteCR();
					return TRUE;
				case ID_ACC_PASTE:
					OnEditPaste();
					return TRUE;
				case ID_ACC_PRINT:
					OnFilePrint();
					return TRUE;
				case ID_ACC_EXIT:
					OnFileExit();
					return TRUE;
			}
		}
		else { // transfer accelerator message to TEK win
			switch (wID) {
				case ID_ACC_COPY:
					::PostMessage(HTEKWin,WM_COMMAND,ID_TEKEDIT_COPY,0);
					return TRUE;
				case ID_ACC_PASTECR:
					::PostMessage(HTEKWin,WM_COMMAND,ID_TEKEDIT_PASTECR,0);
					return TRUE;
				case ID_ACC_PASTE:
					::PostMessage(HTEKWin,WM_COMMAND,ID_TEKEDIT_PASTE,0);
					return TRUE;
				case ID_ACC_PRINT:
					::PostMessage(HTEKWin,WM_COMMAND,ID_TEKFILE_PRINT,0);
					return TRUE;
				case ID_ACC_EXIT:
					::PostMessage(HTEKWin,WM_COMMAND,ID_TEKFILE_EXIT,0);
					return TRUE;
			}
		}
	}

	if ((wID>=ID_WINDOW_1) && (wID<ID_WINDOW_1+9)) {
		SelectWin(wID-ID_WINDOW_1);
		return TRUE;
	}
	else {
		if (TTXProcessCommand(HVTWin, wID)) {
			return TRUE;
		}
		else { /* TTPLUG */
			return TTCFrameWnd::OnCommand(wParam, lParam);
		}
	}
}

void CVTWindow::OnActivate(UINT nState, HWND pWndOther, BOOL bMinimized)
{
	DispSetActive(nState!=WA_INACTIVE);
	if (nState == WA_INACTIVE) {
		SetWindowAlpha(ts.AlphaBlendInactive);
	} else {
		SetWindowAlpha(ts.AlphaBlendActive);
	}
}

/**
 *	キーボードから1文字入力
 *	@param	nChar	UTF-16 char(wchar_t)	IsWindowUnicode() == TRUE 時
 *					ANSI char(char)			IsWindowUnicode() == FALSE 時
 */
void CVTWindow::OnChar(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
	unsigned int i;

	if (!KeybEnabled || (TalkStatus!=IdTalkKeyb)) {
		return;
	}

	if (MetaKey(ts.MetaKey)) {
		::PostMessage(HVTWin,WM_SYSCHAR,nChar,MAKELONG(nRepCnt,nFlags));
		return;
	}

	wchar_t u16;
	if (IsWindowUnicode(HVTWin) == TRUE) {
		// 入力は UTF-16
		u16 = (wchar_t)nChar;
	} else {
		// 入力は ANSI
		//		ANSI(ACP) -> UTF-32 -> UTF-16
		const char mb_str[2] = {(char)nChar, 0};
		unsigned int u32;
		size_t mb_len = MBCPToUTF32(mb_str, 1, CP_ACP, &u32);
		if (mb_len == 0) {
			return;
		}
		u16 = (wchar_t)u32;
	}

	// バッファへ出力、画面へ出力
	for (i=1 ; i<=nRepCnt ; i++) {
		CommTextOutW(&cv,&u16,1);
		if (ts.LocalEcho>0) {
			CommTextEchoW(&cv,&u16,1);
		}
	}

	// スクロール位置をリセット
	if (WinOrgY != 0) {
		DispVScroll(SCROLL_BOTTOM, 0);
	}
}

LRESULT CVTWindow::OnUniChar(WPARAM wParam, LPARAM lParam)
{
	if (wParam == UNICODE_NOCHAR) {
		// このメッセージをサポートしているかテストできるようにするため
		return TRUE;
	}

	char32_t u32 = (char32_t)wParam;
	wchar_t strW[2];
	size_t u16_len = UTF32ToUTF16(u32, strW, _countof(strW));
	CommTextOutW(&cv, strW, u16_len);
	if (ts.LocalEcho > 0) {
		CommTextEchoW(&cv, strW, u16_len);
	}

	return FALSE;
}

/* copy from ttset.c*/
static void WriteInt2(const char *Sect, const char *Key, const char *FName, int i1, int i2)
{
	char Temp[32];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d", i1, i2);
	WritePrivateProfileString(Sect, Key, Temp, FName);
}

static void SaveVTPos()
{
#define Section "Tera Term"
	if (ts.SaveVTWinPos) {
		/* VT win position */
		WriteInt2(Section, "VTPos", ts.SetupFName, ts.VTPos.x, ts.VTPos.y);

		/* VT terminal size  */
		WriteInt2(Section, "TerminalSize", ts.SetupFName,
		          ts.TerminalWidth, ts.TerminalHeight);
	}
}

void CVTWindow::OnClose()
{
	if ((HTEKWin!=NULL) && ! ::IsWindowEnabled(HTEKWin)) {
		MessageBeep(0);
		return;
	}

	if (cv.Ready && (cv.PortType==IdTCPIP) &&
	    ((ts.PortFlag & PF_CONFIRMDISCONN) != 0) &&
	    ! CloseTT) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			NULL, L"Tera Term",
			"MSG_DISCONNECT_CONF", L"Disconnect?",
			MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2
		};
		int result = TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
		if (result == IDCANCEL) {
			return;
		}
	}

	FLogClose();
	FileSendEnd();
	ProtoEnd();

	SaveVTPos();
	DestroyWindow();
}

// 全Tera Termの終了を指示する
void CVTWindow::OnAllClose()
{
	// 突然終了させると危険なので、かならずユーザに問い合わせを出すようにする。
	static const TTMessageBoxInfoW info = {
		"Tera Term",
		NULL, L"Tera Term",
		"MSG_ALL_TERMINATE_CONF", L"Terminate ALL Tera Term(s)?",
		MB_OKCANCEL | MB_ICONERROR | MB_DEFBUTTON2
	};
	int result = TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
	if (result == IDCANCEL)
		return;

	BroadcastClosingMessage(HVTWin);
}

// 終了問い合わせなしにTera Termを終了する。OnAllClose()受信用。
LRESULT CVTWindow::OnNonConfirmClose(WPARAM wParam, LPARAM lParam)
{
	// ここで ts の内容を意図的に書き換えても、終了時に自動セーブされるわけではないので、特に問題なし。
	ts.PortFlag &= ~PF_CONFIRMDISCONN;
	OnClose();
	return 1;
}

void CVTWindow::OnDestroy()
{
	// remove this window from the window list
	UnregWin(HVTWin);

	// USBデバイス変化通知解除
	UnRegDeviceNotify(HVTWin);

	EndKeyboard();

	/* Disable drag-drop */
	::DragAcceptFiles(HVTWin,FALSE);
	DropListFree();

	EndDDE();

	if (cv.TelFlag) {
		EndTelnet();
	}
	CommClose(&cv);

	FreeIME(HVTWin);
	FreeTTSET();
#if 0	// freeに失敗するまでfreeし続ける
	do { }
	while (FreeTTDLG());
#endif

#if 0
	do { }
	while (FreeTTFILE());
#endif

	if (HTEKWin != NULL) {
		::DestroyWindow(HTEKWin);
	}

	EndTerm();
	EndDisp();

	FreeBuffer();

	TTXEnd(); /* TTPLUG */

	DeleteNotifyIcon(&cv);
}

static void EscapeFilename(const wchar_t *src, wchar_t *dest)
{
#define ESCAPE_CHARS	L" ;&()$!`'[]{}#^~"
	const wchar_t *s = src;
	wchar_t *d = dest;
	while (*s) {
		wchar_t c = *s++;
		if (c == L'\\') {
			// パスの区切りを \ -> / へ
			*d = '/';
		} else if (wcschr(ESCAPE_CHARS, c) != NULL) {
			// エスケープが必要な文字
			*d++ = L'\\';
			*d = c;
		} else {
			*d = c;
		}
		d++;
	}
	*d = '\0'; // null-terminate
}

static wchar_t *GetPasteString(const wchar_t *str, bool escape)
{
	wchar_t *tmpbuf;
	if (!escape) {
		tmpbuf = _wcsdup(str);
	}
	else {
		const size_t len = wcslen(str) * sizeof(wchar_t) * 2;
		tmpbuf = (wchar_t *)malloc(len);
		EscapeFilename(str, tmpbuf);
	}
	return tmpbuf;
}

/* 入力はファイルのみ(フォルダは含まれない) */
static bool SendScp(wchar_t *Filenames[], int FileCount, const char *SendDir)
{
	typedef int (CALLBACK *PSSH_start_scp)(char *, char *);
	static PSSH_start_scp func = NULL;
	static HMODULE h = NULL;
	char msg[128];

	if (h == NULL) {
		if ( ((h = GetModuleHandle("ttxssh.dll")) == NULL) ) {
			_snprintf_s(msg, sizeof(msg), _TRUNCATE, "GetModuleHandle(\"ttxssh.dll\")) %d", GetLastError());
		scp_send_error:
			::MessageBox(NULL, msg, "Tera Term: scpsend command error", MB_OK | MB_ICONERROR);
			return false;
		}
	}
	if (func == NULL) {
		func = (PSSH_start_scp)GetProcAddress(h, "TTXScpSendfile");
		if (func == NULL) {
			_snprintf_s(msg, sizeof(msg), _TRUNCATE, "GetProcAddress(\"TTXScpSendfile\")) %d", GetLastError());
			goto scp_send_error;
		}
	}

	for (int i = 0; i < FileCount; i++) {
		char *FileName = ToU8W(Filenames[i]);
		func(FileName, ts.ScpSendDir);
		free(FileName);
	}
	return true;
}

void CVTWindow::DropListFree()
{
	if (DropListCount > 0) {
		for (int i = 0; i < DropListCount; i++) {
			free(DropLists[i]);
			DropLists[i] = NULL;
		}
		free(DropLists);
		DropLists = NULL;
		DropListCount = 0;
	}
}

LRESULT CVTWindow::OnDropNotify(WPARAM ShowDialog, LPARAM lParam)
{
	// iniに保存されない、今実行しているTera Termでのみ有効な設定
	static enum drop_type DefaultDropType = DROP_TYPE_CANCEL;
	static unsigned char DefaultDropTypePaste = DROP_TYPE_PASTE_ESCAPE;
	static bool DefaultShowDialog = ts.ConfirmFileDragAndDrop ? true : false;

	(void)lParam;
	int FileCount = 0;
	int DirectoryCount = 0;
	for (int i = 0; i < DropListCount; i++) {
		const wchar_t *FileName = DropLists[i];
		const DWORD attr = GetFileAttributesW(FileName);
		if (attr == INVALID_FILE_ATTRIBUTES) {
			FileCount++;
		} else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
			DirectoryCount++;
		} else {
			FileCount++;
		}
	}

	bool DoSameProcess = false;
	const bool isSSH = (cv.isSSH == 2);
	enum drop_type DropType;
	unsigned char DropTypePaste = DROP_TYPE_PASTE_ESCAPE;
	if (DefaultDropType == DROP_TYPE_CANCEL) {
		// default is not set
		if (!ShowDialog) {
			if (FileCount == 1 && DirectoryCount == 0) {
				if (ts.ConfirmFileDragAndDrop) {
					if (isSSH) {
						DropType = DROP_TYPE_SCP;
					} else {
						DropType = DROP_TYPE_SEND_FILE;
					}
					DoSameProcess = false;
				} else {
					DropType = DROP_TYPE_SEND_FILE;
					DoSameProcess = DefaultShowDialog ? false : true;
				}
			} else if (FileCount == 0 && DirectoryCount == 1) {
				DropType = DROP_TYPE_PASTE_FILENAME;
				DoSameProcess = DefaultShowDialog ? false : true;
			} else if (FileCount > 0 && DirectoryCount > 0) {
				DropType = DROP_TYPE_PASTE_FILENAME;
				DoSameProcess = false;
			} else if (FileCount > 0 && DirectoryCount == 0) {
				// filename only
				if (isSSH) {
					DropType = DROP_TYPE_SCP;
				} else {
					DropType = DROP_TYPE_SEND_FILE;
				}
				DoSameProcess = false;
			} else {
				// directory only
				DropType = DROP_TYPE_PASTE_FILENAME;
				DoSameProcess = ts.ConfirmFileDragAndDrop ? false : true;
			}
		} else {
			// show dialog
			if (DirectoryCount > 0) {
				DropType = DROP_TYPE_PASTE_FILENAME;
			} else {
				if (isSSH) {
					DropType = DROP_TYPE_SCP;
				} else {
					DropType = DROP_TYPE_SEND_FILE;
				}
			}
			DoSameProcess = false;
		}
	} else {
		if (DirectoryCount > 0 &&
			(DefaultDropType == DROP_TYPE_SEND_FILE ||
			 DefaultDropType == DROP_TYPE_SEND_FILE_BINARY ||
			 DefaultDropType == DROP_TYPE_SCP))
		{	// デフォルトのままでは処理できない組み合わせ
			DropType = DROP_TYPE_PASTE_FILENAME;
			DropTypePaste = DefaultDropTypePaste;
			DoSameProcess = false;
		} else {
			DropType = DefaultDropType;
			DropTypePaste = DefaultDropTypePaste;
			DoSameProcess = (ShowDialog || DefaultShowDialog) ? false : true;
		}
	}

	for (int i = 0; i < DropListCount; i++) {
		const wchar_t *FileName = DropLists[i];

		if (!DoSameProcess) {
			bool DoSameProcessNextDrop;
			bool DoNotShowDialog = !DefaultShowDialog;
			SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
						  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
			DropType =
				ShowDropDialogBox(m_hInst, HVTWin,
								  FileName, DropType,
								  DropListCount - i,
								  (DirectoryCount == 0 && isSSH) ? true : false,
								  DirectoryCount == 0 ? true : false,
								  &ts,
								  &DropTypePaste,
								  &DoSameProcess,
								  &DoSameProcessNextDrop,
								  &DoNotShowDialog);
			if (DropType == DROP_TYPE_CANCEL) {
				goto finish;
			}
			if (DoSameProcessNextDrop) {
				DefaultDropType = DropType;
				DefaultDropTypePaste = DropTypePaste;
			}
			if (!ts.ConfirmFileDragAndDrop) {
				DefaultShowDialog = !DoNotShowDialog;
			}
		}

		switch (DropType) {
		case DROP_TYPE_CANCEL:
		default:
			// cancel
			break;
		case DROP_TYPE_SEND_FILE:
			SendMemSendFile(FileName, FALSE, SENDMEM_DELAYTYPE_NO_DELAY, 0, 0);
			break;
		case DROP_TYPE_SEND_FILE_BINARY:
			SendMemSendFile(FileName, TRUE, SENDMEM_DELAYTYPE_NO_DELAY, 0, 0);
			break;
		case DROP_TYPE_PASTE_FILENAME:
		{
			const bool escape = (DropTypePaste & DROP_TYPE_PASTE_ESCAPE) ? true : false;

			TermSendStartBracket();

			wchar_t *str = GetPasteString(FileName, escape);
			TermPasteStringNoBracket(str, wcslen(str));
			free(str);
			if (DropListCount > 1 && i < DropListCount - 1) {
				if (DropTypePaste & DROP_TYPE_PASTE_NEWLINE) {
					TermPasteStringNoBracket(L"\x0d", 1);	// 改行(CR,0x0d)
				}
				else {
					TermPasteStringNoBracket(L" ", 1);		// space
				}
			}

			TermSendEndBracket();

			break;
		}
		case DROP_TYPE_SCP:
		{
			// send by scp
			wchar_t **FileNames = &DropLists[i];
			int FileCount = DoSameProcess ? DropListCount - i : 1;
			if (!SendScp(FileNames, FileCount, ts.ScpSendDir)) {
				goto finish;
			}
			i += FileCount - 1;
			break;
		}
		}
	}

finish:
	DropListFree();
	return 0;
}

void CVTWindow::OnDropFiles(HDROP hDropInfo)
{
	::SetForegroundWindow(HVTWin);
	if (cv.Ready && IsSendVarNULL())
	{
		const UINT ShowDialog =
			((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) ? 1 : 0;
		DropListCount = DragQueryFileW(hDropInfo, -1, NULL, 0);
		DropLists = (wchar_t **)malloc(sizeof(wchar_t *) * DropListCount);

		for (int i = 0; i < DropListCount; i++) {
			const UINT cch = DragQueryFileW(hDropInfo, i, NULL, 0);
			if (cch == 0) {
				continue;
			}
			wchar_t *FileName = (wchar_t *)malloc(sizeof(wchar_t) * (cch + 1));
			DragQueryFileW(hDropInfo,i,FileName,cch + 1);
			FileName[cch] = '\0';
			DropLists[i] = FileName;
		}

		::PostMessage(HVTWin, WM_USER_DROPNOTIFY, ShowDialog, 0);
	}
	DragFinish(hDropInfo);
}

void CVTWindow::OnGetMinMaxInfo(MINMAXINFO *lpMMI)
{
#ifndef WINDOW_MAXMIMUM_ENABLED
	lpMMI->ptMaxSize.x = 10000;
	lpMMI->ptMaxSize.y = 10000;
	lpMMI->ptMaxTrackSize.x = 10000;
	lpMMI->ptMaxTrackSize.y = 10000;
#endif
}

void CVTWindow::OnHScroll(UINT nSBCode, UINT nPos, HWND pScrollBar)
{
	int Func;

	switch (nSBCode) {
		case SB_BOTTOM:
			Func = SCROLL_BOTTOM;
			break;
		case SB_ENDSCROLL:
			return;
		case SB_LINEDOWN:
			Func = SCROLL_LINEDOWN;
			break;
		case SB_LINEUP:
			Func = SCROLL_LINEUP;
			break;
		case SB_PAGEDOWN:
			Func = SCROLL_PAGEDOWN;
			break;
		case SB_PAGEUP:
			Func = SCROLL_PAGEUP;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			Func = SCROLL_POS;
			break;
		case SB_TOP:
			Func = SCROLL_TOP;
			break;
		default:
			return;
	}
	DispHScroll(Func,nPos);
}

void CVTWindow::OnInitMenuPopup(HMENU hPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	InitMenuPopup(hPopupMenu);
}

void CVTWindow::OnKeyDown(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
#if UNICODE_DEBUG
	if (UnicodeDebugParam.CodePopupEnable)
	{
		const DWORD now = GetTickCount();
		switch(CtrlKeyState) {
		case 0:
			if (nChar == UnicodeDebugParam.CodePopupKey1) {
				CtrlKeyDownTick = now;
				CtrlKeyState = 1;
			}
			break;
		case 2:
			if (nChar != UnicodeDebugParam.CodePopupKey2) {
				CtrlKeyState = 0;
				break;
			}
			if (now - CtrlKeyDownTick < 500 && TipWinCodeDebug == NULL) {
				POINT pos;
				GetCursorPos(&pos);
				ScreenToClient(m_hWnd, &pos);
				CodePopup(pos.x, pos.y);
				CtrlKeyState = 3;
			} else {
				CtrlKeyDownTick = now;
				CtrlKeyState = 1;
			}
			break;
		case 3:
			break;
		default:
			CtrlKeyState = 0;
			break;
		}
	}
	if (TipWinCodeDebug != NULL && nChar == VK_SHIFT) {
		POINT pos;
		GetCursorPos(&pos);
		ScreenToClient(m_hWnd, &pos);
		wchar_t *buf = BuffGetCharInfo(pos.x, pos.y);
		CBSetTextW(HVTWin, buf, 0);
		free(buf);
		MessageBeep(MB_OK);
		TipWinDestroy(TipWinCodeDebug);
		TipWinCodeDebug = NULL;
		CtrlKeyState = 0;
	}
#endif
	switch (KeyDown(HVTWin,nChar,nRepCnt,nFlags & 0x1ff)) {
	case KEYDOWN_OTHER:
		break;
	case KEYDOWN_CONTROL:
		return;
	case KEYDOWN_COMMOUT:
		// スクロール位置をリセット
		if (WinOrgY != 0) {
			DispVScroll(SCROLL_BOTTOM, 0);
		}
		return;
	}

	if (MetaKey(ts.MetaKey) && (nFlags & 0x2000) != 0)
	{
		BYTE KeyState[256];
		MSG M;

		PeekMessage((LPMSG)&M,HVTWin,WM_CHAR,WM_CHAR,PM_REMOVE);
		/* for Ctrl+Alt+Key combination */
		GetKeyboardState(KeyState);
		KeyState[VK_MENU] = 0;
		SetKeyboardState(KeyState);
		M.hwnd = HVTWin;
		M.message = WM_KEYDOWN;
		M.wParam = nChar;
		M.lParam = MAKELONG(nRepCnt,nFlags & 0xdfff);
		TranslateMessage(&M);
	}

}

void CVTWindow::OnKeyUp(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
	KeyUp(nChar);
#if UNICODE_DEBUG
	if (CtrlKeyState == 1 && nChar == UnicodeDebugParam.CodePopupKey1) {
		CtrlKeyState++;
	} else {
		CtrlKeyState = 0;
	}
	if (nChar == UnicodeDebugParam.CodePopupKey2) {
		if (TipWinCodeDebug != NULL) {
			TipWinDestroy(TipWinCodeDebug);
			TipWinCodeDebug = NULL;
			CtrlKeyState = 0;
		}
	}
#endif
}

void CVTWindow::OnKillFocus(HWND hNewWnd)
{
	DispDestroyCaret();
	FocusReport(FALSE);
//	TTCFrameWnd::OnKillFocus(hNewWnd);		// TODO

	if (IsCaretOn()) {
		CaretKillFocus(TRUE);
	}
}

void CVTWindow::OnLButtonDblClk(WPARAM nFlags, POINTS point)
{
	if (LButton || MButton || RButton) {
		return;
	}

	DblClkX = point.x;
	DblClkY = point.y;

	if (MouseReport(IdMouseEventBtnDown, IdLeftButton, DblClkX, DblClkY)) {
		return;
	}

	if (BuffUrlDblClk(DblClkX, DblClkY)) { // ブラウザ呼び出しの場合は何もしない。 (2005.4.3 yutaka)
		return;
	}

	BuffDblClk(DblClkX, DblClkY);

	LButton = TRUE;
	DblClk = TRUE;
	AfterDblClk = TRUE;
	::SetTimer(HVTWin, IdDblClkTimer, GetDoubleClickTime(), NULL);

	/* for AutoScrolling */
	::SetCapture(HVTWin);
	::SetTimer(HVTWin, IdScrollTimer, 100, NULL);
}

void CVTWindow::OnLButtonDown(WPARAM nFlags, POINTS point)
{
	POINT p;

	p.x = point.x;
	p.y = point.y;
	ButtonDown(p,IdLeftButton);
}

void CVTWindow::OnLButtonUp(WPARAM nFlags, POINTS point)
{
	if (IgnoreRelease)
		IgnoreRelease = FALSE;
	else if (MouseReport(IdMouseEventBtnUp, IdLeftButton, point.x, point.y)) {
		ReleaseCapture();
	}

	if (! LButton) {
		return;
	}

	ButtonUp(FALSE);
}

void CVTWindow::OnMButtonDown(WPARAM nFlags, POINTS point)
{
	POINT p;

	p.x = point.x;
	p.y = point.y;
	ButtonDown(p,IdMiddleButton);
}

void CVTWindow::OnMButtonUp(WPARAM nFlags, POINTS point)
{
	if (IgnoreRelease)
		IgnoreRelease = FALSE;
	else if (MouseReport(IdMouseEventBtnUp, IdMiddleButton, point.x, point.y)) {
		ReleaseCapture();
	}

	if (! MButton) {
		return;
	}

	// added DisablePasteMouseMButton (2008.3.2 maya)
	if (ts.PasteFlag & CPF_DISABLE_MBUTTON) {
		ButtonUp(FALSE);
	}
	else {
		ButtonUp(TRUE);
	}
}

LRESULT CVTWindow::OnMouseActivate(HWND pDesktopWnd, UINT nHitTest, UINT message)
{
	if ((ts.SelOnActive==0) && (nHitTest==HTCLIENT)) { //disable mouse event for text selection
		IgnoreRelease = TRUE;
		return MA_ACTIVATEANDEAT; //     when window is activated
	}
	else {
		return MA_ACTIVATE;
	}
}


void CVTWindow::CodePopup(int client_x, int client_y)
{
	wchar_t *buf = BuffGetCharInfo(client_x, client_y);
	if (TipWinCodeDebug == NULL) {
		TipWinCodeDebug = TipWinCreate(m_hInst, m_hWnd);
	}
	POINT pos = { client_x, client_y };
	ClientToScreen(m_hWnd, &pos);
	TipWinSetPos(TipWinCodeDebug, pos.x, pos.y);
	TipWinSetTextW(TipWinCodeDebug, buf);
	TipWinSetVisible(TipWinCodeDebug, TRUE);
	free(buf);
}

void CVTWindow::OnMouseMove(WPARAM nFlags, POINTS point)
{
	int i;
	BOOL mousereport = FALSE;

#if UNICODE_DEBUG
	if (TipWinCodeDebug != NULL) {
		CodePopup(point.x, point.y);
	}
#endif

	if (!IgnoreRelease)
		mousereport = MouseReport(IdMouseEventMove, 0, point.x, point.y);

	if (! (LButton || MButton || RButton)) {
		if (BuffCheckMouseOnURL(point.x, point.y))
			SetMouseCursor("HAND");
		else
			SetMouseCursor(ts.MouseCursorName);
		return;
	}

	if (mousereport) {
		return;
	}

	if (DblClk) {
		i = 2;
	}
	else if (TplClk) {
		i = 3;
	}
	else {
		i = 1;
	}

	if (!ts.SelectOnlyByLButton ||
	    (ts.SelectOnlyByLButton && LButton) ) {
		// SelectOnlyByLButton == TRUE のときは、左ボタンダウン時のみ選択する (2007.11.21 maya)
		BuffChangeSelect(point.x, point.y,i);
	}
}

void CVTWindow::OnMove(int x, int y)
{
	DispSetWinPos();
}

// マウスホイールの回転
BOOL CVTWindow::OnMouseWheel(
	UINT nFlags,   // 仮想キー
	short zDelta,  // 回転距離
	POINTS pts     // カーソル位置
)
{
	POINT pt;
	pt.x = pts.x;
	pt.y = pts.y;

	int line, i;

	if (pSetLayeredWindowAttributes != NULL) {
		BOOL InTitleBar;
		POINT point = pt;
		GetPositionOnWindow(HVTWin, &point,
							NULL, NULL, &InTitleBar);
		if (InTitleBar) {
			int delta = zDelta < 0 ? -1 : 1;
			int newAlpha = Alpha;
			POINT tippos;

			newAlpha += delta * ts.MouseWheelScrollLine;
			if (newAlpha > 255)
				newAlpha = 255;
			else if (newAlpha < 0)
				newAlpha = 0;
			SetWindowAlpha(newAlpha);

			wchar_t *uimsg;
			GetI18nStrWA("Tera Term", "TOOLTIP_TITLEBAR_OPACITY", L"Opacity %.1f %%", ts.UILanguageFile, &uimsg);
			wchar_t *tipbuf;
			aswprintf(&tipbuf, uimsg, (newAlpha / 255.0) * 100);
			free(uimsg);

			tippos = TipWin->GetPos();
			if (tippos.x != pt.x ||
			    tippos.y != pt.y) {
					TipWin->SetVisible(FALSE);
			}

			TipWin->SetText(tipbuf);
			TipWin->SetPos(pt.x, pt.y);
			TipWin->SetHideTimer(1000);

			if(! TipWin->IsVisible()) {
				TipWin->SetVisible(TRUE);
			}

			free(tipbuf);

			return TRUE;
		}
	}

	::ScreenToClient(HVTWin, &pt);

	line = abs(zDelta) / WHEEL_DELTA; // ライン数
	if (line < 1) line = 1;

	// 一スクロールあたりの行数に変換する (2008.4.6 yutaka)
	if (line == 1 && ts.MouseWheelScrollLine > 0)
		line *= ts.MouseWheelScrollLine;

	if (MouseReport(IdMouseEventWheel, zDelta<0, pt.x, pt.y))
		return TRUE;

	if (WheelToCursorMode()) {
		if (zDelta < 0) {
			KeyDown(HVTWin, VK_DOWN, line, MapVirtualKey(VK_DOWN, 0) | 0x100);
			KeyUp(VK_DOWN);
		} else {
			KeyDown(HVTWin, VK_UP, line, MapVirtualKey(VK_UP, 0) | 0x100);
			KeyUp(VK_UP);
		}
	} else {
		for (i = 0 ; i < line ; i++) {
			if (zDelta < 0) {
				OnVScroll(SB_LINEDOWN, 0, NULL);
			} else {
				OnVScroll(SB_LINEUP, 0, NULL);
			}
		}
	}

	return (TRUE);
}

void CVTWindow::OnNcLButtonDblClk(UINT nHitTest, POINTS point)
{
	if (! Minimized && !ts.TermIsWin && (nHitTest == HTCAPTION)) {
		DispRestoreWinSize();
	}
}

void CVTWindow::OnNcRButtonDown(UINT nHitTest, POINTS point)
{
	if ((nHitTest==HTCAPTION) &&
	    (ts.HideTitle>0) &&
		AltKey()) {
		::CloseWindow(HVTWin); /* iconize */
	}
}

void CVTWindow::OnPaint()
{
	PAINTSTRUCT ps;
	HDC PaintDC;
	int Xs, Ys, Xe, Ye;

	// 表示されていなくてもWM_PAINTが発生するケース対策
	if (::IsWindowVisible(m_hWnd) == 0) {
		return;
	}

#ifdef ALPHABLEND_TYPE2
//<!--by AKASI
	BGSetupPrimary(FALSE);
//-->
#endif

	PaintDC = BeginPaint(&ps);

	PaintWindow(PaintDC,ps.rcPaint,ps.fErase, &Xs,&Ys,&Xe,&Ye);
	LockBuffer();
	BuffUpdateRect(Xs,Ys,Xe,Ye);
	UnlockBuffer();
	DispEndPaint();

	EndPaint(&ps);

	if (FirstPaint) {
		if (strlen(TopicName)>0) {
			InitDDE();
			SendDDEReady();
		}
		FirstPaint = FALSE;
		Startup();
	}
}

void CVTWindow::OnRButtonDown(UINT nFlags, POINTS point)
{
	POINT p;

	p.x = point.x;
	p.y = point.y;
	ButtonDown(p,IdRightButton);
}

void CVTWindow::OnRButtonUp(UINT nFlags, POINTS point)
{
	if (IgnoreRelease)
		IgnoreRelease = FALSE;
	else if (MouseReport(IdMouseEventBtnUp, IdRightButton, point.x, point.y)) {
		ReleaseCapture();
	}

	if (! RButton) {
		return;
	}

	/*
	 *  ペースト条件:
	 *  ・ts.PasteFlag & CPF_DISABLE_RBUTTON -> 右ボタンによるペースト無効
	 *  ・ts.PasteFlag & CPF_CONFIRM_RBUTTON -> 表示されたメニューからペーストを行うので、
	 *                                          右ボタンアップによるペーストは行わない
	 */
	if ((ts.PasteFlag & CPF_DISABLE_RBUTTON) || (ts.PasteFlag & CPF_CONFIRM_RBUTTON)) {
		ButtonUp(FALSE);
	} else {
		ButtonUp(TRUE);
	}
}

void CVTWindow::OnSetFocus(HWND hOldWnd)
{
	ChangeCaret();
	FocusReport(TRUE);
}

void CVTWindow::OnSize(WPARAM nType, int cx, int cy)
{
	if (IgnoreSizeMessage) {
		return;
	}
	RECT R;
	int w, h;

	Minimized = (nType==SIZE_MINIMIZED);

	if (FirstPaint && Minimized)
	{
		if (strlen(TopicName)>0)
		{
			InitDDE();
			SendDDEReady();
		}
		FirstPaint = FALSE;
		Startup();
		return;
	}
	if (Minimized || DontChangeSize) {
		return;
	}

	if (nType == SIZE_MAXIMIZED) {
		ts.TerminalOldWidth = ts.TerminalWidth;
		ts.TerminalOldHeight = ts.TerminalHeight;
	}

	::GetWindowRect(HVTWin,&R);
	w = R.right - R.left;
	h = R.bottom - R.top;
	if (AdjustSize) {
		ResizeWindow(R.left,R.top,w,h,cx,cy);
	}
	else {
		if (ts.FontScaling) {
			int NewFontWidth, NewFontHeight;
			BOOL FontChanged = FALSE;

			NewFontWidth = cx / ts.TerminalWidth;
			NewFontHeight = cy / ts.TerminalHeight;

			if (NewFontWidth - ts.FontDW < 3) {
				NewFontWidth = ts.FontDW + 3;
			}
			if (NewFontWidth != FontWidth) {
				ts.VTFontSize.x = ts.FontDW - NewFontWidth;
				FontWidth = NewFontWidth;
				FontChanged = TRUE;
			}

			if (NewFontHeight - ts.FontDH < 3) {
				NewFontHeight = ts.FontDH + 3;
			}
			if (NewFontHeight != FontHeight) {
				ts.VTFontSize.y = ts.FontDH - NewFontHeight;
				FontHeight = NewFontHeight;
				FontChanged = TRUE;
			}

			w = ts.TerminalWidth;
			h = ts.TerminalHeight;

			if (FontChanged) {
				ChangeFont();
			}
		}
		else {
			w = cx / FontWidth;
			h = cy / FontHeight;
		}

		HideStatusLine();
		BuffChangeWinSize(w,h);
	}

#ifdef WINDOW_MAXMIMUM_ENABLED
	if (nType == SIZE_MAXIMIZED) {
		AdjustSize = 0;
	}
#endif
}

// リサイズ中の処理として、以下の二つを行う。
// ・ツールチップで新しい端末サイズを表示する
// ・フォントサイズと端末サイズに合わせて、ウィンドウ位置・サイズを調整する
void CVTWindow::OnSizing(WPARAM fwSide, LPRECT pRect)
{
	int nWidth;
	int nHeight;
	RECT cr, wr;
	int margin_width, margin_height;
	int w, h;
	int fixed_width, fixed_height;

	::GetWindowRect(HVTWin, &wr);
	::GetClientRect(HVTWin, &cr);

	margin_width = wr.right - wr.left - cr.right + cr.left;
	margin_height = wr.bottom - wr.top - cr.bottom + cr.top;
	nWidth = (pRect->right) - (pRect->left) - margin_width;
	nHeight = (pRect->bottom) - (pRect->top) - margin_height;

	w = nWidth / FontWidth;
	h = nHeight / FontHeight;

	if (!ts.TermIsWin) {
		// TermIsWin=off の時はリサイズでは端末サイズが変わらないので
		// 現在の端末サイズを上限とする。
		if (w > ts.TerminalWidth)
			w = ts.TerminalWidth;
		if (h > ts.TerminalHeight)
			h = ts.TerminalHeight;
	}

	// 最低でも 1x1 の端末サイズを保障する。
	if (w <= 0)
		w = 1;
	if (h <= 0)
		h = 1;

	UpdateSizeTip(HVTWin, w, h, fwSide, pRect->left, pRect->top);

	fixed_width = w * FontWidth + margin_width;
	fixed_height = h * FontHeight + margin_height;

	switch (fwSide) { // 幅調整
	case 1: // 左
	case 4: // 左上
	case 7: // 左下
		pRect->left = pRect->right - fixed_width;
		break;
	case 2: // 右
	case 5: // 右上
	case 8: // 右下
		pRect->right = pRect->left + fixed_width;
		break;
	}

	switch (fwSide) { // 高さ調整
	case 3: // 上
	case 4: // 左上
	case 5: // 右上
		pRect->top = pRect->bottom - fixed_height;
		break;
	case 6: // 下
	case 7: // 左下
	case 8: // 右下
		pRect->bottom = pRect->top + fixed_height;
		break;
	}
}

void CVTWindow::OnSysChar(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
#ifdef WINDOW_MAXMIMUM_ENABLED
	// ALT + xを押下すると WM_SYSCHAR が飛んでくる。
	// ALT + Enterでウィンドウの最大化 (2005.4.24 yutaka)
	if ((nFlags&0x2000) != 0 && nChar == CR) {
		if (::IsZoomed(m_hWnd)) { // window is maximum
			ShowWindow(SW_RESTORE);
		} else {
			ShowWindow(SW_MAXIMIZE);
		}
	}
#endif

	if (MetaKey(ts.MetaKey)) {
		if (!KeybEnabled || (TalkStatus != IdTalkKeyb))
			return;

		char Code = nChar;
		wchar_t u16;
		switch (ts.Meta8Bit) {
		case IdMeta8BitRaw:
			Code = nChar;
			break;
		default:
			if (IsWindowUnicode(HVTWin) == TRUE) {
				u16 = nChar;
			}
			else {
				if (ts.Meta8Bit != IdMeta8BitRaw) {
					const char mb_str[2] = {(char)nChar, 0};
					unsigned int u32;
					size_t mb_len = MBCPToUTF32(mb_str, 1, CP_ACP, &u32);
					if (mb_len == 0) {
						return;
					}
					u16 = (wchar_t)u32;
				}
			}
		}
		for (unsigned int i = 1; i <= nRepCnt; i++) {
			switch (ts.Meta8Bit) {
				case IdMeta8BitRaw:
					Code |= 0x80;
					CommBinaryBuffOut(&cv, &Code, 1);
					if (ts.LocalEcho) {
						CommBinaryEcho(&cv, &Code, 1);
					}
					break;
				case IdMeta8BitText:
					u16 |= 0x80;
					CommTextOutW(&cv, &u16, 1);
					if (ts.LocalEcho) {
						CommTextEchoW(&cv, &u16, 1);
					}
					break;
				default: {
					const wchar_t e = ESC;
					CommTextOutW(&cv, &e, 1);
					CommTextOutW(&cv, &u16, 1);
					if (ts.LocalEcho) {
						CommTextEchoW(&cv, &e, 1);
						CommTextEchoW(&cv, &u16, 1);
					}
				}
			}
		}
		return;
	}

	TTCFrameWnd::DefWindowProc(WM_SYSCHAR, nChar, MAKELONG(nRepCnt, nFlags));
}

void CVTWindow::OnSysCommand(WPARAM nID, LPARAM lParam)
{
	if (nID==ID_SHOWMENUBAR) {
		ts.PopupMenu = 0;
		SwitchMenu();
	}
	else if (((nID & 0xfff0)==SC_CLOSE) && (cv.PortType==IdTCPIP) &&
	         cv.Open && ! cv.Ready && (cv.ComPort>0)) {
		// now getting host address (see CommOpen() in commlib.c)
		::PostMessage(HVTWin,WM_SYSCOMMAND,nID,lParam);
	}
#if 0
	else {
		TTCFrameWnd::OnSysCommand(nID,lParam);
	}
#endif
}

void CVTWindow::OnSysKeyDown(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
	if ((nChar==VK_F10) || MetaKey(ts.MetaKey) && ((MainMenu==NULL) || (nChar!=VK_MENU))) {
		KeyDown(HVTWin,nChar,nRepCnt,nFlags & 0x1ff);
		// OnKeyDown(nChar,nRepCnt,nFlags);
	}
	else {
		TTCFrameWnd::OnSysKeyDown(nChar,nRepCnt,nFlags);
	}
}

void CVTWindow::OnSysKeyUp(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_F10) {
		OnKeyUp(nChar,nRepCnt,nFlags);
	}
	else {
		TTCFrameWnd::OnSysKeyUp(nChar,nRepCnt,nFlags);
	}
}

void CVTWindow::OnTimer(UINT_PTR nIDEvent)
{
	POINT Point;
	WORD PortType;
	UINT T;

	if (nIDEvent==IdCaretTimer) {
		if (ts.NonblinkingCursor!=0) {
			T = GetCaretBlinkTime();
			SetCaretBlinkTime(T);
		}
		else {
			::KillTimer(HVTWin,IdCaretTimer);
		}
		return;
	}
	else if (nIDEvent==IdScrollTimer) {
		GetCursorPos(&Point);
		::ScreenToClient(HVTWin,&Point);
		DispAutoScroll(Point);
		if ((Point.x < 0) || (Point.x >= ScreenWidth) ||
			(Point.y < 0) || (Point.y >= ScreenHeight)) {
			::PostMessage(HVTWin,WM_MOUSEMOVE,MK_LBUTTON,MAKELONG(Point.x,Point.y));
		}
		return;
	}
	else if (nIDEvent == IdCancelConnectTimer) {
		// まだ接続が完了していなければ、ソケットを強制クローズ。
		// CloseSocket()を呼びたいが、ここからは呼べないので、直接Win32APIをコールする。
		if (!cv.Ready) {
			closesocket(cv.s);
			cv.s = INVALID_SOCKET;  /* ソケット無効の印を付ける。(2010.8.6 yutaka) */
			//::PostMessage(HVTWin, WM_USER_COMMNOTIFY, 0, FD_CLOSE);
		}
	}

	::KillTimer(HVTWin, nIDEvent);

	switch (nIDEvent) {
		case IdDelayTimer:
			cv.CanSend = TRUE;
			break;
		case IdProtoTimer:
			ProtoDlgTimeOut();
			break;
		case IdDblClkTimer:
			AfterDblClk = FALSE;
			break;
		case IdComEndTimer:
			if (! CommCanClose(&cv)) {
				// wait if received data remains
				::SetTimer(m_hWnd, IdComEndTimer,1,NULL);
				break;
			}
			cv.Ready = FALSE;
			if (cv.TelFlag) {
				EndTelnet();
			}
			PortType = cv.PortType;
			CommClose(&cv);
			SetDdeComReady(0);
			if ((PortType==IdTCPIP) &&
				((ts.PortFlag & PF_BEEPONCONNECT) != 0)) {
				MessageBeep(0);
			}
			if ((PortType==IdTCPIP) &&
				(ts.AutoWinClose>0) &&
				::IsWindowEnabled(HVTWin) &&
				((HTEKWin==NULL) || ::IsWindowEnabled(HTEKWin)) ) {
				OnClose();
			}
			else {
				ChangeTitle();
				if (ts.ClearScreenOnCloseConnection) {
					OnEditClearScreen();
				}
			}
			break;
		case IdPrnStartTimer:
			PrnFileStart(PrintFile_);
			break;
		case IdPrnProcTimer:
			PrnFileDirectProc(PrintFile_);
			break;
	}
}

void CVTWindow::OnVScroll(UINT nSBCode, UINT nPos, HWND pScrollBar)
{
	int Func;
	SCROLLINFO si;

	switch (nSBCode) {
	case SB_BOTTOM:
		Func = SCROLL_BOTTOM;
		break;
	case SB_ENDSCROLL:
		return;
	case SB_LINEDOWN:
		Func = SCROLL_LINEDOWN;
		break;
	case SB_LINEUP:
		Func = SCROLL_LINEUP;
		break;
	case SB_PAGEDOWN:
		Func = SCROLL_PAGEDOWN;
		break;
	case SB_PAGEUP:
		Func = SCROLL_PAGEUP;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		Func = SCROLL_POS;
		break;
	case SB_TOP:
		Func = SCROLL_TOP;
		break;
	default:
		return;
	}

	// スクロールレンジを 16bit から 32bit へ拡張した (2005.10.4 yutaka)
	ZeroMemory(&si, sizeof(SCROLLINFO));
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_TRACKPOS;
	if (::GetScrollInfo(HVTWin, SB_VERT, &si)) { // success
		nPos = si.nTrackPos;
	}

	DispVScroll(Func,nPos);
}

BOOL CVTWindow::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	PDEV_BROADCAST_HDR pDevHdr;
#ifdef DEBUG
	char port_name[8]; /* COMxxxx + NULL */
#endif

	pDevHdr = (PDEV_BROADCAST_HDR)dwData;

	switch (nEventType) {
	case DBT_DEVICEARRIVAL:
#ifdef DEBUG
		OutputDebugPrintf("DBT_DEVICEARRIVAL devicetype=%d PortType=%d AutoDisconnectedPort=%d\n", pDevHdr->dbch_devicetype, ts.PortType, AutoDisconnectedPort);
#endif
		// DBT_DEVTYP_PORT を投げず DBT_DEVTYP_DEVICEINTERFACE しか投げないドライバがあるため
		if ((pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT || pDevHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) &&
		    ts.PortType == IdSerial &&
		    ts.AutoComPortReconnect &&
		    AutoDisconnectedPort == ts.ComPort) {
#ifdef DEBUG
			if (pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT) {
				PDEV_BROADCAST_PORT pDevPort;
				pDevPort = (PDEV_BROADCAST_PORT)pDevHdr;
				strncpy_s(port_name, sizeof(port_name), pDevPort->dbcp_name, _TRUNCATE);
				OutputDebugPrintf("%s\n", port_name);
			}
#endif
			if (!cv.Open) {
				/* Tera Term 未接続 */
				if (CheckComPort(ts.ComPort) == 1) {
					/* ポートが有効 */
					AutoDisconnectedPort = -1;
					Connecting = TRUE;
					ChangeTitle();
					CommOpen(HVTWin, &ts, &cv);
				}
			}
		}
		break;
	case DBT_DEVICEREMOVECOMPLETE:
#ifdef DEBUG
		OutputDebugPrintf("DBT_DEVICEREMOVECOMPLETE devicetype=%d PortType=%d AutoDisconnectedPort=%d\n", pDevHdr->dbch_devicetype, ts.PortType, AutoDisconnectedPort);
#endif
		if ((pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT || pDevHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) &&
		    ts.PortType == IdSerial &&
		    ts.AutoComPortReconnect &&
		    AutoDisconnectedPort == -1) {
#ifdef DEBUG
			if (pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT) {
				PDEV_BROADCAST_PORT pDevPort;
				pDevPort = (PDEV_BROADCAST_PORT)pDevHdr;
				strncpy_s(port_name, sizeof(port_name), pDevPort->dbcp_name, _TRUNCATE);
				OutputDebugPrintf("%s\n", port_name);
			}
#endif
			if (cv.Open) {
				/* Tera Term 接続中 */
				if (CheckComPort(cv.ComPort) == 0) {
					/* ポートが無効 */
					AutoDisconnectedPort = cv.ComPort;
					Disconnect(TRUE);
				}
			}
		}
		break;
	}
	return TRUE;
}

//<!--by AKASI
LRESULT CVTWindow::OnWindowPosChanging(WPARAM wParam, LPARAM lParam)
{
#ifdef ALPHABLEND_TYPE2
	if(BGEnable && BGNoCopyBits) {
		((WINDOWPOS*)lParam)->flags |= SWP_NOCOPYBITS;
	}
#endif

	return TTCFrameWnd::DefWindowProc(WM_WINDOWPOSCHANGING,wParam,lParam);
}

LRESULT CVTWindow::OnSettingChange(WPARAM wParam, LPARAM lParam)
{
#ifdef ALPHABLEND_TYPE2
	BGOnSettingChange();
#endif
	return TTCFrameWnd::DefWindowProc(WM_SETTINGCHANGE,wParam,lParam);
}

LRESULT CVTWindow::OnEnterSizeMove(WPARAM wParam, LPARAM lParam)
{
	EnableSizeTip(1);

#ifdef ALPHABLEND_TYPE2
	BGOnEnterSizeMove();
#endif
	return TTCFrameWnd::DefWindowProc(WM_ENTERSIZEMOVE,wParam,lParam);
}

LRESULT CVTWindow::OnExitSizeMove(WPARAM wParam, LPARAM lParam)
{
#ifdef ALPHABLEND_TYPE2
	BGOnExitSizeMove();
#endif

	EnableSizeTip(0);

	return TTCFrameWnd::DefWindowProc(WM_EXITSIZEMOVE,wParam,lParam);
}
//-->

LRESULT CVTWindow::OnIMEStartComposition(WPARAM wParam, LPARAM lParam)
{
	IMECompositionState = TRUE;

	// 位置を通知する
	int CaretX = (CursorX-WinOrgX)*FontWidth;
	int CaretY = (CursorY-WinOrgY)*FontHeight;
	SetConversionWindow(HVTWin,CaretX,CaretY);

	return TTCFrameWnd::DefWindowProc(WM_IME_STARTCOMPOSITION,wParam,lParam);
}

LRESULT CVTWindow::OnIMEEndComposition(WPARAM wParam, LPARAM lParam)
{
	IMECompositionState = FALSE;
	return TTCFrameWnd::DefWindowProc(WM_IME_ENDCOMPOSITION,wParam,lParam);
}

LRESULT CVTWindow::OnIMEComposition(WPARAM wParam, LPARAM lParam)
{
	if (CanUseIME()) {
		size_t len;
		const wchar_t *lpstr = GetConvStringW(HVTWin, lParam, &len);
		if (lpstr != NULL) {
			const wchar_t *output_wstr = lpstr;
			if (len == 1 && ControlKey()) {
				const static wchar_t code_ctrl_space = 0;
				const static wchar_t code_ctrl_backslash = 0x1c;
				switch(*lpstr) {
				case 0x20:
					output_wstr = &code_ctrl_space;
					break;
				case 0x5c: // Ctrl-\ support for NEC-PC98
					output_wstr = &code_ctrl_backslash;
					break;
				}
			}
			if (ts.LocalEcho>0) {
				CommTextEchoW(&cv,output_wstr,len);
			}
			CommTextOutW(&cv,output_wstr,len);
			free((void *)lpstr);
			return 0;
		}
	}
	return TTCFrameWnd::DefWindowProc(WM_IME_COMPOSITION,wParam,lParam);
}

LRESULT CVTWindow::OnIMEInputChange(WPARAM wParam, LPARAM lParam)
{
	ChangeCaret();

	return TTCFrameWnd::DefWindowProc(WM_INPUTLANGCHANGE,wParam,lParam);
}

LRESULT CVTWindow::OnIMENotify(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
	case IMN_SETOPENSTATUS:
		// 入力コンテキストの開閉状態が更新される(IME On/OFF)

		// IMEのOn/Offを取得する
		IMEstat = GetIMEOpenStatus(HVTWin);
		if (IMEstat != 0) {
			// IME On

			// 状態を表示するIMEのために位置を通知する
			int CaretX = (CursorX-WinOrgX)*FontWidth;
			int CaretY = (CursorY-WinOrgY)*FontHeight;
			SetConversionWindow(HVTWin,CaretX,CaretY);

			// フォントを設定する
			ResetConversionLogFont(HVTWin);
		}

		// 描画
		ChangeCaret();

		break;

	// 候補ウィンドウの表示状況通知
	// IME_OPENCANDIDATE / IMN_CLOSECANDIDATE サポート状況
	//
	//  IME								status
	//  --------------------------------+----------
	//  MS IME 日本語(Windows 10 1809)	suport
	//  Google 日本語入力(2.24.3250.0)	not support
	//
	// WM_IME_STARTCOMPOSITION / WM_IME_ENDCOMPOSITION は
	// 漢字入力状態がスタートした / 終了したで発生する。
	// IME_OPENCANDIDATE / IMN_CLOSECANDIDATE は
	// 候補ウィンドウが表示された / 閉じたで発生する。
	case IMN_OPENCANDIDATE: {
		// 候補ウィンドウを開こうとしている

		// 状態を表示するIMEのために位置を通知する
		// 次の場合があるので、位置を再設定する
		// - 漢字変換候補を表示
		// - 次の文字を入力することで確定処理を行う
		// - 文字入力と未変換文字入力が発生する
		int CaretX = (CursorX-WinOrgX)*FontWidth;
		int CaretY = (CursorY-WinOrgY)*FontHeight;
		SetConversionWindow(HVTWin,CaretX,CaretY);

		// フォントを設定する
		ResetConversionLogFont(HVTWin);

		break;
	}

	case IMN_OPENSTATUSWINDOW:
		// ステータスウィンドウをオープン(未確定文字を表示?)しようとしている

		// IMEで未変換状態で、フォントダイアログをオープンしてクローズすると
		// IMEに設定していたフォントが別のものに変化しているらしい
		// ここでフォントの再設定を行う

		// フォントを設定する
		ResetConversionLogFont(HVTWin);
		break;
	default:
		break;
	}

	return TTCFrameWnd::DefWindowProc(WM_IME_NOTIFY,wParam,lParam);
}

static LRESULT ReplyIMERequestDocumentfeed(HWND hWnd, LPARAM lParam)
{
	static RECONVERTSTRING *pReconvPtrSave;		// TODO leak
	static size_t ReconvSizeSave;
	LRESULT result;

	if (lParam == 0)
	{  // 1回目の呼び出し サイズだけを返す
		if(IsWindowUnicode(hWnd) == FALSE) {
			// ANSI版
			char buf[512];			// 参照文字列を受け取るバッファ
			size_t str_len_count;
			int cx;

			// 参照文字列取得、1行取り出す
			{	// カーソルから後ろ、スペース以外が見つかったところを行末とする
				int x;
				int len;
				cx = BuffGetCurrentLineData(buf, sizeof(buf));
				len = cx;
				for (x=cx; x < NumOfColumns; x++) {
					const char c = buf[x];
					if (c != 0 && c != 0x20) {
						len = x+1;
					}
				}
				str_len_count = len;
			}

			// IMEに返す構造体を作成する
			if (pReconvPtrSave != NULL) {
				free(pReconvPtrSave);
			}
			pReconvPtrSave = (RECONVERTSTRING *)CreateReconvStringStA(
				hWnd, buf, str_len_count, cx, &ReconvSizeSave);
		}
		else {
			// UNICODE版
			size_t str_len_count;
			int cx;
			wchar_t *strW;

			// 参照文字列取得、1行取り出す
			{	// カーソルから後ろ、スペース以外が見つかったところを行末とする
				int x;
				size_t len = 0;
				cx = CursorX;
				strW = BuffGetLineStrW(CursorY, &cx, &len);
				len = cx;
				for (x=cx; x < NumOfColumns; x++) {
					const wchar_t c = strW[x];
					if (c == 0 || c == 0x20) {
						len = x;
						break;
					}
				}
				str_len_count = len;
			}

			// IMEに返す構造体を作成する
			if (pReconvPtrSave != NULL) {
				free(pReconvPtrSave);
			}
			pReconvPtrSave = (RECONVERTSTRING *)CreateReconvStringStW(
				hWnd, strW, str_len_count, cx, &ReconvSizeSave);
			free(strW);
		}

		// 1回目はサイズだけを返す
		result = ReconvSizeSave;
	}
	else {
		// 2回目の呼び出し 構造体を渡す
		if (pReconvPtrSave != NULL) {
			RECONVERTSTRING *pReconv = (RECONVERTSTRING*)lParam;
			result = 0;
			if (pReconv->dwSize >= ReconvSizeSave) {
				// 1回目のサイズが確保されてきているはず
				memcpy(pReconv, pReconvPtrSave, ReconvSizeSave);
				result = ReconvSizeSave;
			}
			free(pReconvPtrSave);
			pReconvPtrSave = NULL;
			ReconvSizeSave = 0;
		} else {
			// 3回目?
			result = 0;
		}
	}

	return result;
}

LRESULT CVTWindow::OnIMERequest(WPARAM wParam, LPARAM lParam)
{
	// "IME=off"の場合は、何もしない。
	if (ts.UseIME > 0) {
		switch(wParam) {
		case IMR_DOCUMENTFEED:
			return ReplyIMERequestDocumentfeed(HVTWin, lParam);
		default:
			break;
		}
	}
	return TTCFrameWnd::DefWindowProc(WM_IME_REQUEST,wParam,lParam);
}

LRESULT CVTWindow::OnAccelCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IdHold:
			if (TalkStatus==IdTalkKeyb) {
				Hold = ! Hold;
				CommLock(&ts,&cv,Hold);
			}
			 break;
		case IdPrint:
			OnFilePrint();
			break;
		case IdBreak:
			OnControlSendBreak();
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
		case IdCmdEditCLB:
			OnEditClearBuffer();
			break;
		case IdCmdCtrlOpenTEK:
			OnControlOpenTEK();
			break;
		case IdCmdCtrlCloseTEK:
			OnControlCloseTEK();
			break;
		case IdCmdLineUp:
			OnVScroll(SB_LINEUP,0,NULL);
			break;
		case IdCmdLineDown:
			OnVScroll(SB_LINEDOWN,0,NULL);
			break;
		case IdCmdPageUp:
			OnVScroll(SB_PAGEUP,0,NULL);
			break;
		case IdCmdPageDown:
			OnVScroll(SB_PAGEDOWN,0,NULL);
			break;
		case IdCmdBuffTop:
			OnVScroll(SB_TOP,0,NULL);
			break;
		case IdCmdBuffBottom:
			OnVScroll(SB_BOTTOM,0,NULL);
			break;
		case IdCmdNextWin:
			SelectNextWin(HVTWin,1,FALSE);
			break;
		case IdCmdPrevWin:
			SelectNextWin(HVTWin,-1,FALSE);
			break;
		case IdCmdNextSWin:
			SelectNextWin(HVTWin,1,TRUE);
			break;
		case IdCmdPrevSWin:
			SelectNextWin(HVTWin,-1,TRUE);
			break;
		case IdCmdLocalEcho:
			if (ts.LocalEcho==0) {
				ts.LocalEcho = 1;
			}
			else {
				ts.LocalEcho = 0;
			}
			if (cv.Ready && cv.TelFlag && (ts.TelEcho>0)) {
				TelChangeEcho();
			}
			break;
		case IdCmdDisconnect: // called by TTMACRO
			Disconnect(lParam);
			break;
		case IdCmdLoadKeyMap: // called by TTMACRO
			SetKeyMap();
			break;
		case IdCmdRestoreSetup: // called by TTMACRO
			RestoreSetup();
			break;
		case IdCmdScrollLock:
			ScrollLock = ! ScrollLock;
			break;
	}
	return 0;
}

LRESULT CVTWindow::OnChangeMenu(WPARAM wParam, LPARAM lParam)
{
	HMENU SysMenu;
	BOOL Show, B1, B2;

	Show = (ts.PopupMenu==0) && (ts.HideTitle==0);

// TTXKanjiMenu のために、メニューが表示されていても
// 再描画するようにした。 (2007.7.14 maya)
	if (Show != (MainMenu!=NULL)) {
		AdjustSize = TRUE;
	}

	if (MainMenu!=NULL) {
		DestroyMenu(MainMenu);
		MainMenu = NULL;
	}

	if (! Show) {
		if (WinMenu!=NULL) {
			DestroyMenu(WinMenu);
		}
		WinMenu = NULL;
	}
	else {
		InitMenu(&MainMenu);
	}

	::SetMenu(HVTWin, MainMenu);
	::DrawMenuBar(HVTWin);

	B1 = ((ts.MenuFlag & MF_SHOWWINMENU)!=0);
	B2 = (WinMenu!=NULL);
	if ((MainMenu!=NULL) &&
	    (B1 != B2)) {
		if (WinMenu==NULL) {
			WinMenu = CreatePopupMenu();
			get_lang_msg("MENU_WINDOW", ts.UIMsg, sizeof(ts.UIMsg),
			             "&Window", ts.UILanguageFile);
			::InsertMenu(MainMenu,ID_HELPMENU,
			             MF_STRING | MF_ENABLED | MF_POPUP | MF_BYPOSITION,
			             (UINT_PTR)WinMenu, ts.UIMsg);
		}
		else {
			RemoveMenu(MainMenu,ID_HELPMENU,MF_BYPOSITION);
			DestroyMenu(WinMenu);
			WinMenu = NULL;
		}
		::DrawMenuBar(HVTWin);
	}

	::GetSystemMenu(HVTWin,TRUE);
	if ((! Show) && ((ts.MenuFlag & MF_NOSHOWMENU)==0)) {
		SysMenu = ::GetSystemMenu(HVTWin,FALSE);
		AppendMenu(SysMenu, MF_SEPARATOR, 0, NULL);
		get_lang_msg("MENU_SHOW_MENUBAR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Show menu &bar", ts.UILanguageFile);
		AppendMenu(SysMenu, MF_STRING, ID_SHOWMENUBAR, ts.UIMsg);
	}
	return 0;
}

LRESULT CVTWindow::OnChangeTBar(WPARAM wParam, LPARAM lParam)
{
	BOOL TBar;
	DWORD Style,ExStyle;
	HMENU SysMenu;

	Style = (DWORD)::GetWindowLongPtr (HVTWin, GWL_STYLE);
	ExStyle = (DWORD)::GetWindowLongPtr (HVTWin, GWL_EXSTYLE);
	TBar = ((Style & WS_SYSMENU)!=0);
	if (TBar == (ts.HideTitle==0)) {
		return 0;
	}

#ifndef WINDOW_MAXMIMUM_ENABLED
	if (ts.HideTitle>0)
		Style = Style & ~(WS_SYSMENU | WS_CAPTION |
		                  WS_MINIMIZEBOX) | WS_BORDER | WS_POPUP;
	else
		Style = Style & ~WS_POPUP | WS_SYSMENU | WS_CAPTION |
	                     WS_MINIMIZEBOX;
#else
	if (ts.HideTitle>0) {
		Style = Style & ~(WS_SYSMENU | WS_CAPTION |
	                      WS_MINIMIZEBOX | WS_MAXIMIZEBOX) | WS_BORDER | WS_POPUP;

#ifdef ALPHABLEND_TYPE2
		if(BGNoFrame) {
			Style   &= ~(WS_THICKFRAME | WS_BORDER);
			ExStyle &= ~WS_EX_CLIENTEDGE;
		}else{
			ExStyle |=  WS_EX_CLIENTEDGE;
		}
#endif
	}
	else {
		Style = Style & ~WS_POPUP | WS_SYSMENU | WS_CAPTION |
		                 WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_BORDER;

		ExStyle |=  WS_EX_CLIENTEDGE;
	}
#endif

	AdjustSize = TRUE;
	::SetWindowLongPtr(HVTWin, GWL_STYLE, Style);
#ifdef ALPHABLEND_TYPE2
	::SetWindowLongPtr(HVTWin, GWL_EXSTYLE, ExStyle);
#endif
	::SetWindowPos(HVTWin, NULL, 0, 0, 0, 0,
	               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	::ShowWindow(HVTWin, SW_SHOW);

	if ((ts.HideTitle==0) && (MainMenu==NULL) &&
	    ((ts.MenuFlag & MF_NOSHOWMENU) == 0)) {
		SysMenu = ::GetSystemMenu(HVTWin,FALSE);
		AppendMenu(SysMenu, MF_SEPARATOR, 0, NULL);
		get_lang_msg("MENU_SHOW_MENUBAR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Show menu &bar", ts.UILanguageFile);
		AppendMenu(SysMenu, MF_STRING, ID_SHOWMENUBAR, ts.UIMsg);
	}
	return 0;
}

LRESULT CVTWindow::OnCommNotify(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(lParam)) {
		case FD_READ:  // TCP/IP
			CommProcRRQ(&cv);
			break;
		case FD_CLOSE:
			if (cv.PortType == IdTCPIP) {
				if (TCPLocalEchoUsed) {
					TCPLocalEchoUsed=FALSE;
					ts.LocalEcho = ts.LocalEcho_ini;
				}
				if (TCPCRSendUsed) {
					TCPCRSendUsed = FALSE;
					ts.CRSend = ts.CRSend_ini;
					cv.CRSend = ts.CRSend_ini;
				}
			}
			Connecting = FALSE;
			TCPIPClosed = TRUE;
			// disable transmition
			cv.OutBuffCount = 0;
			cv.LineModeBuffCount = 0;
			cv.FlushLen = 0;
			::SetTimer(m_hWnd, IdComEndTimer,1,NULL);
			break;
	}
	return 0;
}

LRESULT CVTWindow::OnCommOpen(WPARAM wParam, LPARAM lParam)
{
	AutoDisconnectedPort = -1;

	CommStart(&cv,lParam,&ts);
	if (ts.PortType == IdTCPIP && cv.RetryWithOtherProtocol == TRUE) {
		Connecting = TRUE;
	}
	else {
		Connecting = FALSE;
	}

	ChangeTitle();
	if (! cv.Ready) {
		return 0;
	}

	/* Auto start logging or /L= option */
	if (ts.LogAutoStart || ts.LogFN[0] != 0) {
		if (ts.LogFN == NULL || ts.LogFN[0] == 0) {
			ts.LogFNW = FLogGetLogFilename(NULL);
			WideCharToACP_t(ts.LogFNW, ts.LogFN, sizeof(ts.LogFN));
		}
		if (ts.LogFN[0]!=0) {
			FLogOpen(ts.LogFNW, LOG_UTF8, FALSE);
		}
	}

	if ((ts.PortType==IdTCPIP) &&
	    ((ts.PortFlag & PF_BEEPONCONNECT) != 0)) {
		MessageBeep(0);
	}

	if (cv.PortType==IdTCPIP) {
		InitTelnet();

		if ((cv.TelFlag) && (ts.TCPPort==ts.TelPort)) {
			// Start telnet option negotiation from this side
			//   if telnet flag is set and port#==default telnet port# (23)
			TelEnableMyOpt(TERMTYPE);

			TelEnableHisOpt(SGA);

			TelEnableMyOpt(SGA);

			if (ts.TelEcho>0)
				TelChangeEcho();
			else
				TelEnableHisOpt(ECHO);

			TelEnableMyOpt(NAWS);
			if (ts.TelBin>0) {
				TelEnableMyOpt(BINARY);
				TelEnableHisOpt(BINARY);
			}

			TelEnableMyOpt(TERMSPEED);

			TelStartKeepAliveThread();
		}
		else if (!ts.DisableTCPEchoCR) {
			if (ts.TCPCRSend>0) {
				TCPCRSendUsed = TRUE;
				ts.CRSend = ts.TCPCRSend;
				cv.CRSend = ts.TCPCRSend;
			}
			if (ts.TCPLocalEcho>0) {
				TCPLocalEchoUsed = TRUE;
				ts.LocalEcho = ts.TCPLocalEcho;
			}
		}
	}

	SetDdeComReady(1);

	return 0;
}

LRESULT CVTWindow::OnCommStart(WPARAM wParam, LPARAM lParam)
{
	// 自動接続が無効のときも接続ダイアログを出すようにした (2006.9.15 maya)
	if (((ts.PortType!=IdSerial) && (ts.HostName[0]==0)) ||
	    ((ts.PortType==IdSerial) && (ts.ComAutoConnect == FALSE))) {
		if (ts.HostDialogOnStartup) {
			OnFileNewConnection();
		}
		else {
			SetDdeComReady(0);
		}
	}
	else {
		Connecting = TRUE;
		ChangeTitle();
		if (ts.AutoComPortReconnect && ts.WaitCom && ts.PortType == IdSerial) {
			if (CheckComPort(ts.ComPort) == 0) {
				SetAutoConnectPort(ts.ComPort);
				return 0;
			}
		}
		CommOpen(HVTWin,&ts,&cv);
	}
	return 0;
}

LRESULT CVTWindow::OnDdeEnd(WPARAM wParam, LPARAM lParam)
{
	EndDDE();
	if (CloseTT) {
		OnClose();
	}
	return 0;
}

LRESULT CVTWindow::OnDlgHelp(WPARAM wParam, LPARAM lParam)
{
	DWORD help_id = (wParam == 0) ? HelpId : (DWORD)wParam;
	OpenHelp(HH_HELP_CONTEXT, help_id, ts.UILanguageFile);
	return 0;
}

#if 0
LRESULT CVTWindow::OnFileTransEnd(WPARAM wParam, LPARAM lParam)
{
	FileTransEnd(wParam);
	return 0;
}
#endif

LRESULT CVTWindow::OnGetSerialNo(WPARAM wParam, LPARAM lParam)
{
	return (LONG)SerialNo;
}

LRESULT CVTWindow::OnKeyCode(WPARAM wParam, LPARAM lParam)
{
	KeyCodeSend(wParam,(WORD)lParam);
	return 0;
}

LRESULT CVTWindow::OnProtoEnd(WPARAM wParam, LPARAM lParam)
{
	ProtoDlgCancel();
	return 0;
}

LRESULT CVTWindow::OnChangeTitle(WPARAM wParam, LPARAM lParam)
{
	ChangeTitle();
	return 0;
}

LRESULT CVTWindow::OnNotifyIcon(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 1) {
		switch (lParam) {
		  case WM_MOUSEMOVE:
		  case WM_LBUTTONUP:
		  case WM_LBUTTONDBLCLK:
		  case WM_RBUTTONUP:
		  case WM_RBUTTONDBLCLK:
		  case WM_CONTEXTMENU:
		  case NIN_BALLOONSHOW:
		  case NIN_BALLOONHIDE:
		  case NIN_KEYSELECT:
		  case NIN_SELECT:
			// nothing to do
			break;
		  case WM_LBUTTONDOWN:
		  case WM_RBUTTONDOWN:
			HideNotifyIcon(&cv);
			break;
		  case NIN_BALLOONTIMEOUT:
			HideNotifyIcon(&cv);
			break;
		  case NIN_BALLOONUSERCLICK:
			::SetForegroundWindow(HVTWin);
			HideNotifyIcon(&cv);
			break;
		}
	}

	return 0;
}

void CVTWindow::OnFileNewConnection()
{
//	char Command[MAXPATHLEN], Command2[MAXPATHLEN];
	char Command[MAXPATHLEN + HostNameMaxLength], Command2[MAXPATHLEN + HostNameMaxLength]; // yutaka
	TGetHNRec GetHNRec; /* record for dialog box */

	if (Connecting) return;

	HelpId = HlpFileNewConnection;
	GetHNRec.SetupFN = ts.SetupFName;
	GetHNRec.PortType = ts.PortType;
	GetHNRec.Telnet = ts.Telnet;
	GetHNRec.TelPort = ts.TelPort;
	GetHNRec.TCPPort = ts.TCPPort;
	GetHNRec.ProtocolFamily = ts.ProtocolFamily;
	GetHNRec.ComPort = ts.ComPort;
	GetHNRec.MaxComPort = ts.MaxComPort;

	strncpy_s(Command, sizeof(Command),"ttermpro ", _TRUNCATE);
	GetHNRec.HostName = &Command[9];

	if (! LoadTTDLG()) {
		return;
	}

	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	if ((*GetHostName)(HVTWin,&GetHNRec)) {
		if ((GetHNRec.PortType==IdTCPIP) && LoadTTSET()) {
			if (ts.HistoryList) {
				(*AddHostToList)(ts.SetupFName,GetHNRec.HostName);
			}
			if (ts.JumpList) {
				add_session_to_jumplist(GetHNRec.HostName, GetHNRec.SetupFN);
			}
			FreeTTSET();
		}

		if (! cv.Ready) {
			ts.PortType = GetHNRec.PortType;
			ts.Telnet = GetHNRec.Telnet;
			ts.TCPPort = GetHNRec.TCPPort;
			ts.ProtocolFamily = GetHNRec.ProtocolFamily;
			ts.ComPort = GetHNRec.ComPort;

			if ((GetHNRec.PortType==IdTCPIP) &&
				LoadTTSET()) {
				(*ParseParam)(Command, &ts, NULL);
				FreeTTSET();
			}
			SetKeyMap();
			if (ts.MacroFN[0]!=0) {
				RunMacro(ts.MacroFN,TRUE);
				ts.MacroFN[0] = 0;
			}
			else {
				Connecting = TRUE;
				ChangeTitle();
				CommOpen(HVTWin,&ts,&cv);
			}
			ResetSetup();
		}
		else {
			if (GetHNRec.PortType==IdSerial) {
				char comport[5];
				Command[8] = 0;
				strncat_s(Command,sizeof(Command)," /C=",_TRUNCATE);
				_snprintf_s(comport, sizeof(comport), _TRUNCATE, "%d", GetHNRec.ComPort);
				strncat_s(Command,sizeof(Command),comport,_TRUNCATE);
			}
			else {
				char tcpport[6];
				DeleteComment(Command2, sizeof(Command2), &Command[9]);
				Command[9] = 0;
				if (GetHNRec.Telnet==0)
					strncat_s(Command,sizeof(Command)," /T=0",_TRUNCATE);
				else
					strncat_s(Command,sizeof(Command)," /T=1",_TRUNCATE);
				if (GetHNRec.TCPPort != 0) {
					strncat_s(Command,sizeof(Command)," /P=",_TRUNCATE);
					_snprintf_s(tcpport, sizeof(tcpport), _TRUNCATE, "%d", GetHNRec.TCPPort);
					strncat_s(Command,sizeof(Command),tcpport,_TRUNCATE);
				}
				/********************************/
				/* ここにプロトコル処理を入れる */
				/********************************/
				if (GetHNRec.ProtocolFamily == AF_INET) {
					strncat_s(Command,sizeof(Command)," /4",_TRUNCATE);
				} else if (GetHNRec.ProtocolFamily == AF_INET6) {
					strncat_s(Command,sizeof(Command)," /6",_TRUNCATE);
				}
				strncat_s(Command,sizeof(Command)," ",_TRUNCATE);
				strncat_s(Command,sizeof(Command),Command2,_TRUNCATE);
			}
			TTXSetCommandLine(Command, sizeof(Command), &GetHNRec); /* TTPLUG */
			WinExec(Command,SW_SHOW);
		}
	}
	else {/* canceled */
		if (! cv.Ready) {
			SetDdeComReady(0);
		}
	}

	FreeTTDLG();
}


// すでに開いているセッションの複製を作る
// (2004.12.6 yutaka)
void CVTWindow::OnDuplicateSession()
{
	char Command[1024];
	const char *exec = "ttermpro";
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char cygterm_cfg[MAX_PATH];
	FILE *fp;
	char buf[256], *head, *body;
	int cygterm_PORT_START = 20000;
	int cygterm_PORT_RANGE = 40;
	int is_cygwin_port = 0;

	// 現在の設定内容を共有メモリへコピーしておく
	CopyTTSetToShmem(&ts);

	// cygterm.cfg を読み込む
	strncpy_s(cygterm_cfg, sizeof(cygterm_cfg), ts.HomeDir, _TRUNCATE);
	AppendSlash(cygterm_cfg, sizeof(cygterm_cfg));
	strncat_s(cygterm_cfg, sizeof(cygterm_cfg), "cygterm.cfg", _TRUNCATE);
	fopen_s(&fp, cygterm_cfg, "r");
	if (fp != NULL) {
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			size_t len = strlen(buf);

			if (buf[len - 1] == '\n')
				buf[len - 1] = '\0';

			split_buffer(buf, '=', &head, &body);
			if (head == NULL || body == NULL)
				continue;

			if (_stricmp(head, "PORT_START") == 0) {
				cygterm_PORT_START = atoi(body);
			} else if (_stricmp(head, "PORT_RANGE") == 0) {
				cygterm_PORT_RANGE = atoi(body);
			}
		}
		fclose(fp);
	}
	// Cygterm のポート範囲内かどうか
	if (ts.TCPPort >= cygterm_PORT_START &&
	    ts.TCPPort <= cygterm_PORT_START+cygterm_PORT_RANGE) {
		is_cygwin_port = 1;
	}

	if (is_cygwin_port && (strcmp(ts.HostName, "127.0.0.1") == 0 ||
	    strcmp(ts.HostName, "localhost") == 0)) {
		// localhostへの接続でポートがcygterm.cfgの範囲内の時はcygwin接続とみなす。
		OnCygwinConnection();
		return;
	} else if (cv.TelFlag) { // telnet
		_snprintf_s(Command, sizeof(Command), _TRUNCATE,
		            "%s %s:%d /DUPLICATE /nossh",
		            exec, ts.HostName, ts.TCPPort);

	} else if (cv.isSSH) { // SSH
		// ここの処理は TTSSH 側にやらせるべき (2004.12.7 yutaka)
		// TTSSH側でのオプション生成を追加。(2005.4.8 yutaka)
		_snprintf_s(Command, sizeof(Command), _TRUNCATE,
		            "%s %s:%d /DUPLICATE",
		            exec, ts.HostName, ts.TCPPort);

		TTXSetCommandLine(Command, sizeof(Command), NULL); /* TTPLUG */

	} else {
		// telnet/ssh/cygwin接続以外では複製を行わない。
		return;
	}

	// セッション複製を行う際、/K= があれば引き継ぎを行うようにする。
	// cf. http://sourceforge.jp/ticket/browse.php?group_id=1412&tid=24682
	// (2011.3.27 yutaka)
	if (strlen(ts.KeyCnfFN) > 0) {
		strncat_s(Command, sizeof(Command), " /K=", _TRUNCATE);
		strncat_s(Command, sizeof(Command), ts.KeyCnfFN, _TRUNCATE);
	}

	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(NULL, Command, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_EXEC_TT_ERROR", L"Can't execute Tera Term. (%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW, GetLastError());
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

static void __dupenv_s(char **envptr, size_t, const char* name)
{
#if defined(_MSC_VER)
	_dupenv_s(envptr, NULL, name);
#else
    const char* s = getenv(name);
	if (s == NULL) {
		*envptr = NULL;
		return;
	}
	*envptr = strdup(s);
#endif
}

//
// Connect to local cygwin
//
void CVTWindow::OnCygwinConnection()
{
	char file[MAX_PATH], *filename;
	char c, *envptr, *envbuff=NULL;
	size_t envbufflen;
	const char *exename = "cygterm.exe";
	char cygterm[MAX_PATH];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	if (strlen(ts.CygwinDirectory) > 0) {
		if (SearchPath(ts.CygwinDirectory, "bin\\cygwin1", ".dll", sizeof(file), file, &filename) > 0) {
			goto found_dll;
		}
	}

	if (SearchPath(NULL, "cygwin1", ".dll", sizeof(file), file, &filename) > 0) {
		goto found_path;
	}

	for (c = 'C' ; c <= 'Z' ; c++) {
		char tmp[MAX_PATH];
		sprintf_s(tmp, "%c:\\cygwin\\bin;%c:\\cygwin64\\bin", c, c);
		if (SearchPath(tmp, "cygwin1", ".dll", sizeof(file), file, &filename) > 0) {
			goto found_dll;
		}
	}

	{
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_FIND_CYGTERM_DIR_ERROR", L"Can't find Cygwin directory.",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
	}
	return;

found_dll:;
	__dupenv_s(&envptr, NULL, "PATH");
	file[strlen(file)-12] = '\0'; // delete "\\cygwin1.dll"
	if (envptr != NULL) {
		envbufflen = strlen(file) + strlen(envptr) + 7; // "PATH="(5) + ";"(1) + NUL(1)
		if ((envbuff = (char *)malloc(envbufflen)) == NULL) {
			static const TTMessageBoxInfoW info = {
				"Tera Term",
				"MSG_ERROR", L"ERROR",
				"MSG_CYGTERM_ENV_ALLOC_ERROR", L"Can't allocate memory for environment variable.",
				MB_OK | MB_ICONWARNING
			};
			TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
			free(envptr);
			return;
		}
		_snprintf_s(envbuff, envbufflen, _TRUNCATE, "PATH=%s;%s", file, envptr);
		free(envptr);
	} else {
		envbufflen = strlen(file) + 6; // "PATH="(5) + NUL(1)
		if ((envbuff = (char *)malloc(envbufflen)) == NULL) {
			static const TTMessageBoxInfoW info = {
				"Tera Term",
				"MSG_ERROR", L"ERROR",
				"MSG_CYGTERM_ENV_ALLOC_ERROR", L"Can't allocate memory for environment variable.",
				MB_OK | MB_ICONWARNING
			};
			TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
			return;
		}
		_snprintf_s(envbuff, envbufflen, _TRUNCATE, "PATH=%s", file);
	}
	_putenv(envbuff);
	if (envbuff) {
		free(envbuff);
		envbuff = NULL;
	}

found_path:;
	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	strncpy_s(cygterm, sizeof(cygterm), ts.HomeDir, _TRUNCATE);
	AppendSlash(cygterm, sizeof(cygterm));
	strncat_s(cygterm, sizeof(cygterm), exename, _TRUNCATE);

	if (CreateProcess(NULL, cygterm, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_EXEC_CYGTERM_ERROR", L"Can't execute Cygterm.",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}


//
// TeraTerm Menuの起動
//
void CVTWindow::OnTTMenuLaunch()
{
	const char *exename = "ttpmenu.exe";
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(exename, NULL, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_EXEC_TTMENU_ERROR", L"Can't execute TeraTerm Menu. (%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW, GetLastError());
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}


//
// LogMeTTの起動
//
void CVTWindow::OnLogMeInLaunch()
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	if (!isLogMeTTExist()) {
		return;
	}

	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(NULL, LogMeTT, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_EXEC_LOGMETT_ERROR", L"Can't execute LogMeTT. (%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW, GetLastError());
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}


void CVTWindow::OnFileLog()
{
	FLogDlgInfo_t info;
	info.filename = NULL;
	BOOL r = FLogOpenDialog(hInst, m_hWnd, &info);
	if (r) {
		const wchar_t *filename = info.filename;
		if (!info.append) {
			// ファイル削除
			DeleteFileW(filename);
		}
		BOOL r = FLogOpen(filename, info.code, info.bom);
		if (r != FALSE) {
			if (FLogIsOpendText()) {
				// 現在バッファにあるデータをすべて書き出してから、
				// ログ採取を開始する。
				// (2013.9.29 yutaka)
				if (ts.LogAllBuffIncludedInFirst) {
					FLogOutputAllBuffer();
				}
			}
		}
		else {
			// ログできない
			static const TTMessageBoxInfoW mbinfo = {
				"Tera Term",
				NULL, L"Tera Term: File open error",
				NULL, L"Can not create a `%s' file.",
				MB_OK | MB_ICONERROR
			};
			TTMessageBoxW(m_hWnd, &mbinfo, ts.UILanguageFileW, filename);
		}
		free(info.filename);
	}
}

void CVTWindow::OnCommentToLog()
{
	if (!FLogIsOpendText()) {
		// 選択できないので呼ばれないはず
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_COMMENT_LOG_OPEN_ERROR", L"It is not opened by the log file yet.",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
		return;
	}
	FLogAddCommentDlg(m_hInst, HVTWin);
}

// ログの閲覧 (2005.1.29 yutaka)
void CVTWindow::OnViewLog()
{
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	if(!FLogIsOpend()) {
		return;
	}

	const wchar_t *filename = FLogGetFilename();

	memset(&si, 0, sizeof(si));
	GetStartupInfoW(&si);
	memset(&pi, 0, sizeof(pi));

	wchar_t *command;
	wchar_t *ViewlogEditor = ToWcharA(ts.ViewlogEditor);
	aswprintf(&command, L"\"%s\" \"%s\"", ViewlogEditor, filename);
	free(ViewlogEditor);

	BOOL r = CreateProcessW(NULL, command, NULL, NULL, FALSE, 0,
							NULL, NULL, &si, &pi);
	free(command);
	if (r == 0) {
		DWORD error = GetLastError();
		static const TTMessageBoxInfoW mbinfo = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_VIEW_LOGFILE_ERROR", L"Can't view logging file. (%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(m_hWnd, &mbinfo, ts.UILanguageFileW, error);
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

// 隠しているログダイアログを表示する (2008.2.3 maya)
void CVTWindow::OnShowLogDialog()
{
	FLogShowDlg();
}

// ログ取得を中断/再開する
void CVTWindow::OnPauseLog()
{
	FLogPause(FLogIsPause() ? FALSE : TRUE);
}

// ログ取得を終了する
void CVTWindow::OnStopLog()
{
	FLogClose();
}

// ログの再生 (2006.12.13 yutaka)
void CVTWindow::OnReplayLog()
{
	OPENFILENAME ofn;
	char szFile[MAX_PATH];
	char Command[MAX_PATH] = "notepad.exe";
	const char *exec = "ttermpro";
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char uimsg[MAX_UIMSG];

	// バイナリモードで採取したログファイルを選択する
	memset(&ofn, 0, sizeof(OPENFILENAME));
	memset(szFile, 0, sizeof(szFile));
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner = HVTWin;
	get_lang_msg("FILEDLG_OPEN_LOGFILE_FILTER", ts.UIMsg, sizeof(ts.UIMsg),
	             "all(*.*)\\0*.*\\0\\0", ts.UILanguageFile);
	ofn.lpstrFilter = ts.UIMsg;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "log";
	get_lang_msg("FILEDLG_OPEN_LOGFILE_TITLE", uimsg, sizeof(uimsg),
	             "Select replay log file with binary mode", ts.UILanguageFile);
	ofn.lpstrTitle = uimsg;
	if(GetOpenFileName(&ofn) == 0)
		return;


	// "/R"オプション付きでTera Termを起動する（ログが再生される）
	_snprintf_s(Command, sizeof(Command), _TRUNCATE,
	            "%s /R=\"%s\"", exec, szFile);

	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(NULL, Command, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_EXEC_TT_ERROR", L"Can't execute Tera Term. (%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW, GetLastError());
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

void CVTWindow::OnFileSend()
{
	// new file send
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_TAHOMA_FONT");
	sendfiledlgdata data;
	data.UILanguageFile = ts.UILanguageFile;
	data.filesend_filter = ts.FileSendFilter;
	INT_PTR ok = sendfiledlg(m_hInst, m_hWnd, &data);
	if (ok != IDOK) {
		return;
	}

	wchar_t *filename = data.filename;
	if (!data.method_4) {
		SendMemSendFile(filename, data.binary, (SendMemDelayType)data.delay_type, data.delay_tick, data.send_size);
	}
	else {
		// file send same as teraterm 4
		HelpId = HlpFileSend;
		FileSendStart(filename, 0);
	}
	free(filename);
}

void CVTWindow::OnFileKermitRcv()
{
	KermitStartRecive(FALSE);
}

void CVTWindow::OnFileKermitGet()
{
	HelpId = HlpFileKmtGet;
	KermitGet(NULL);
}

void CVTWindow::OnFileKermitSend()
{
	HelpId = HlpFileKmtSend;
	KermitStartSend(NULL);
}

void CVTWindow::OnFileKermitFinish()
{
	KermitFinish(FALSE);
}

void CVTWindow::OnFileXRcv()
{
	HelpId = HlpFileXmodemRecv;
	XMODEMStartReceive(NULL, 0, 0);
}

void CVTWindow::OnFileXSend()
{
	HelpId = HlpFileXmodemSend;
	XMODEMStartSend(NULL, 0);
}

void CVTWindow::OnFileYRcv()
{
	HelpId = HlpFileYmodemRecv;
	YMODEMStartReceive(FALSE);
}

void CVTWindow::OnFileYSend()
{
	HelpId = HlpFileYmodemSend;
	YMODEMStartSend(NULL);
}

void CVTWindow::OnFileZRcv()
{
	ZMODEMStartReceive(FALSE, FALSE);
}

void CVTWindow::OnFileZSend()
{
	HelpId = HlpFileZmodemSend;
	ZMODEMStartSend(NULL, 0, FALSE);
}

void CVTWindow::OnFileBPRcv()
{
	BPStartReceive(TRUE, TRUE);
}

void CVTWindow::OnFileBPSend()
{
	HelpId = HlpFileBPlusSend;
	BPStartSend(NULL);
}

void CVTWindow::OnFileQVRcv()
{
	QVStartReceive(FALSE);
}

void CVTWindow::OnFileQVSend()
{
	HelpId = HlpFileQVSend;
	QVStartSend(NULL);
}

void CVTWindow::OnFileChangeDir()
{
	HelpId = HlpFileChangeDir;
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	(*ChangeDirectory)(HVTWin,ts.FileDir);
	FreeTTDLG();
}

void CVTWindow::OnFilePrint()
{
	HelpId = HlpFilePrint;
	BuffPrint(FALSE);
}

void CVTWindow::Disconnect(BOOL confirm)
{
	if (! cv.Ready) {
		return;
	}

	if ((cv.PortType==IdTCPIP) &&
	    ((ts.PortFlag & PF_CONFIRMDISCONN) != 0) &&
	    (confirm)) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			NULL, L"Tera Term",
			"MSG_DISCONNECT_CONF", L"Disconnect?",
			MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2
		};
		int result = TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
		if (result == IDCANCEL) {
			return;
		}
	}

	::PostMessage(HVTWin, WM_USER_COMMNOTIFY, 0, FD_CLOSE);
}

void CVTWindow::OnFileDisconnect()
{
	Disconnect(TRUE);
}

void CVTWindow::OnFileExit()
{
	OnClose();
}

void CVTWindow::OnEditCopy()
{
	// copy selected text to clipboard
	wchar_t *strW = BuffCBCopyUnicode(FALSE);
	CBSetTextW(HVTWin, strW, 0);
	free(strW);
}

void CVTWindow::OnEditCopyTable()
{
	// copy selected text to clipboard in Excel format
	wchar_t *strW = BuffCBCopyUnicode(TRUE);
	CBSetTextW(HVTWin, strW, 0);
	free(strW);
}

void CVTWindow::OnEditPaste()
{
	// add confirm (2008.2.4 yutaka)
	CBStartPaste(HVTWin, FALSE, BracketedPasteMode());

	// スクロール位置をリセット
	if (WinOrgY != 0) {
		DispVScroll(SCROLL_BOTTOM, 0);
	}
}

void CVTWindow::OnEditPasteCR()
{
	// add confirm (2008.3.11 maya)
	CBStartPaste(HVTWin, TRUE, BracketedPasteMode());

	// スクロール位置をリセット
	if (WinOrgY != 0) {
		DispVScroll(SCROLL_BOTTOM, 0);
	}
}

void CVTWindow::OnEditClearScreen()
{
	LockBuffer();
	BuffClearScreen();
	if (isCursorOnStatusLine) {
		MoveCursor(0,CursorY);
	}
	else {
		MoveCursor(0,0);
	}
	BuffUpdateScroll();
	BuffSetCaretWidth();
	UnlockBuffer();
}

void CVTWindow::OnEditClearBuffer()
{
	LockBuffer();
	ClearBuffer();
	UnlockBuffer();
}

void CVTWindow::OnEditSelectAllBuffer()
{
	// Select all of buffer
	POINT p = {0, 0};

	ButtonDown(p, IdLeftButton);
	BuffAllSelect();
	ButtonUp(FALSE);
	ChangeSelectRegion();
}

void CVTWindow::OnEditSelectScreenBuffer()
{
	// Select screen buffer
	POINT p = {0, 0};

	ButtonDown(p, IdLeftButton);
	BuffScreenSelect();
	ButtonUp(FALSE);
	ChangeSelectRegion();
}

void CVTWindow::OnEditCancelSelection()
{
	// Cancel selected buffer
	POINT p = {0, 0};

	ButtonDown(p, IdLeftButton);
	BuffCancelSelection();
	ButtonUp(FALSE);
	ChangeSelectRegion();
}

// Additional settings dialog
//
// (2004.9.5 yutaka) new added
// (2005.2.22 yutaka) changed to Tab Control
// (2008.5.12 maya) changed to PropertySheet
void CVTWindow::OnExternalSetup()
{
	BOOL old_use_unicode_api = UnicodeDebugParam.UseUnicodeApi;
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_TAHOMA_FONT");
	CAddSettingPropSheetDlg CAddSetting(m_hInst, HVTWin);
	INT_PTR ret = CAddSetting.DoModal();
	if (ret == IDOK) {
#ifdef ALPHABLEND_TYPE2
		BGInitialize(FALSE);
		BGSetupPrimary(TRUE);
#else
		DispApplyANSIColor();
#endif
		DispSetNearestColors(IdBack, IdFore+8, NULL);
		ChangeWin();
		ChangeFont();
		if (old_use_unicode_api != UnicodeDebugParam.UseUnicodeApi) {
			BuffSetDispAPI(UnicodeDebugParam.UseUnicodeApi);
		}

		// コーディングタブで設定が変化したときコールする必要がある
		SetupTerm();
	}
}

void CVTWindow::OnSetupTerminal()
{
	BOOL Ok;

	switch (ts.Language) {
	case IdJapanese:
		HelpId = HlpSetupTerminalJa;
		break;
	case IdEnglish:
		HelpId = HlpSetupTerminalEn;
		break;
	case IdKorean:
		HelpId = HlpSetupTerminalKo;
		break;
	case IdRussian:
		HelpId = HlpSetupTerminalRu;
		break;
	case IdUtf8:
		HelpId = HlpSetupTerminalUtf8;
		break;
	default:
		HelpId = HlpSetupTerminal;
		break;
	}
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	Ok = (*SetupTerminal)(HVTWin, &ts);
	FreeTTDLG();
	if (Ok) {
		SetupTerm();
	}
}

void CVTWindow::OnSetupWindow()
{
	BOOL Ok;
	char orgTitle[TitleBuffSize];

	HelpId = HlpSetupWindow;
	ts.VTFlag = 1;
	ts.SampleFont = VTFont[0];

	if (! LoadTTDLG()) {
		return;
	}

	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	strncpy_s(orgTitle, sizeof(orgTitle), ts.Title, _TRUNCATE);
	Ok = (*SetupWin)(HVTWin, &ts);
	FreeTTDLG();

	if (Ok) {
		// Eterm lookfeelの画面情報も更新することで、リアルタイムでの背景色変更が
		// 可能となる。(2006.2.24 yutaka)
#ifdef ALPHABLEND_TYPE2
		BGInitialize(FALSE);
		BGSetupPrimary(TRUE);
#endif

		// タイトルが変更されていたら、リモートタイトルをクリアする
		if ((ts.AcceptTitleChangeRequest == IdTitleChangeRequestOverwrite) &&
		    (strcmp(orgTitle, ts.Title) != 0)) {
			cv.TitleRemote[0] = '\0';
		}

		ChangeFont();
		ChangeWin();
	}

}

void CVTWindow::OnSetupFont()
{
	HelpId = HlpSetupFont;
	DispSetupFontDlg();
	// ANSI表示用のコードページを設定する
	BuffSetDispCodePage(UnicodeDebugParam.CodePageForANSIDraw);
}

static UINT_PTR CALLBACK TFontHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_INITDIALOG) {
		wchar_t uimsg[MAX_UIMSG];
		get_lang_msgW("DLG_CHOOSEFONT_STC6", uimsg, _countof(uimsg),
					  L"\"Font style\" selection here won't affect actual font appearance.", ts.UILanguageFile);
		SetDlgItemTextW(Dialog, stc6, uimsg);

		SetFocus(GetDlgItem(Dialog,cmb1));

		CenterWindow(Dialog, GetParent(Dialog));
	}
	return FALSE;
}

void CVTWindow::OnSetupDlgFont()
{
	LOGFONTA LogFont;
	CHOOSEFONTA cf;
	BOOL result;

	// LOGFONT準備
	memset(&LogFont, 0, sizeof(LogFont));
	strncpy_s(LogFont.lfFaceName, sizeof(LogFont.lfFaceName), ts.DialogFontName,  _TRUNCATE);
	LogFont.lfHeight = -GetFontPixelFromPoint(m_hWnd, ts.DialogFontPoint);
	LogFont.lfCharSet = ts.DialogFontCharSet;
	if (LogFont.lfFaceName[0] == 0) {
		GetMessageboxFont(&LogFont);
	}

	// ダイアログ表示
	memset(&cf, 0, sizeof(cf));
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = HVTWin;
	cf.lpLogFont = &LogFont;
	cf.Flags =
		CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT |
		CF_SHOWHELP | CF_NOVERTFONTS |
		CF_ENABLEHOOK;
	if (IsWindows7OrLater() && ts.ListHiddenFonts) {
		cf.Flags |= CF_INACTIVEFONTS;
	}
	cf.lpfnHook = TFontHook;
	cf.nFontType = REGULAR_FONTTYPE;
	cf.hInstance = m_hInst;
	HelpId = HlpSetupFont;
	result = ChooseFontA(&cf);

	if (result) {
		// 設定
		strncpy_s(ts.DialogFontName, sizeof(ts.DialogFontName), LogFont.lfFaceName, _TRUNCATE);
		ts.DialogFontPoint = cf.iPointSize / 10;
		ts.DialogFontCharSet = LogFont.lfCharSet;
	}
}

void CVTWindow::OnSetupKeyboard()
{
	BOOL Ok;

	if (ts.Language==IdRussian) {
		HelpId = HlpSetupKeyboardRuss;
	}
	else {
		HelpId = HlpSetupKeyboard;
	}
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	Ok = (*SetupKeyboard)(HVTWin, &ts);
	FreeTTDLG();

	if (Ok) {
//		ResetKeypadMode(TRUE);
		if ((ts.Language==IdJapanese) || (ts.Language==IdKorean) || (ts.Language==IdUtf8)) //HKS
			ResetIME();
	}
}

void CVTWindow::OnSetupSerialPort()
{
	BOOL Ok;
	char Command[MAXPATHLEN + HostNameMaxLength];
	char Temp[MAX_PATH], Str[MAX_PATH];

	HelpId = HlpSetupSerialPort;
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	Ok = (*SetupSerialPort)(HVTWin, &ts);
	FreeTTDLG();

	if (Ok && ts.ComPort > 0) {
		/*
		 * TCP/IPによる接続中の場合は新規プロセスとして起動する。
		 * New connectionからシリアル接続する動作と基本的に同じ動作となる。
		 */
		if ( cv.Ready && (cv.PortType != IdSerial) ) {
			_snprintf_s(Command, sizeof(Command),
				"ttermpro /C=%u /SPEED=%lu /CDELAYPERCHAR=%u /CDELAYPERLINE=%u ",
				ts.ComPort, ts.Baud, ts.DelayPerChar, ts.DelayPerLine);

			if (SerialPortConfconvertId2Str(COM_DATABIT, ts.DataBit, Temp, sizeof(Temp))) {
				_snprintf_s(Str, sizeof(Str), _TRUNCATE, "/CDATABIT=%s ", Temp);
				strncat_s(Command, sizeof(Command), Str, _TRUNCATE);
			}
			if (SerialPortConfconvertId2Str(COM_PARITY, ts.Parity, Temp, sizeof(Temp))) {
				_snprintf_s(Str, sizeof(Str), _TRUNCATE, "/CPARITY=%s ", Temp);
				strncat_s(Command, sizeof(Command), Str, _TRUNCATE);
			}
			if (SerialPortConfconvertId2Str(COM_STOPBIT, ts.StopBit, Temp, sizeof(Temp))) {
				_snprintf_s(Str, sizeof(Str), _TRUNCATE, "/CSTOPBIT=%s ", Temp);
				strncat_s(Command, sizeof(Command), Str, _TRUNCATE);
			}
			if (SerialPortConfconvertId2Str(COM_FLOWCTRL, ts.Flow, Temp, sizeof(Temp))) {
				_snprintf_s(Str, sizeof(Str), _TRUNCATE, "/CFLOWCTRL=%s ", Temp);
				strncat_s(Command, sizeof(Command), Str, _TRUNCATE);
			}

			WinExec(Command,SW_SHOW);
			return;
		}

		if (cv.Open) {
			if (ts.ComPort != cv.ComPort) {
				CommClose(&cv);
				CommOpen(HVTWin,&ts,&cv);
			}
			else {
				CommResetSerial(&ts, &cv, ts.ClearComBuffOnOpen);
			}
		}
		else {
			CommOpen(HVTWin,&ts,&cv);
		}
	}
}

void CVTWindow::OnSetupTCPIP()
{
	HelpId = HlpSetupTCPIP;
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	if ((*SetupTCPIP)(HVTWin, &ts)) {
		TelUpdateKeepAliveInterval();
	}
	FreeTTDLG();
}

void CVTWindow::OnSetupGeneral()
{
	HelpId = HlpSetupGeneral;
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	if ((*SetupGeneral)(HVTWin,&ts)) {
		ResetCharSet();
		ResetIME();
	}
	FreeTTDLG();
}

static wchar_t *_get_lang_msg(const char *key, const wchar_t *def, const char *iniFile)
{
	return TTGetLangStrW("Tera Term", key, def, iniFile);
}

/* GetSetupFname function id */
typedef enum {
	GSF_SAVE,		// Save setup
	GSF_RESTORE,	// Restore setup
	GSF_LOADKEY,	// Load key map
} GetSetupFnameFuncId;

static BOOL _GetSetupFname(HWND HWin, GetSetupFnameFuncId FuncId, PTTSet ts)
{
	wchar_t *FNameFilter;
	wchar_t TempDir[MAX_PATH];
	wchar_t DirW[MAX_PATH];
	wchar_t NameW[MAX_PATH];
	const char *UILanguageFile = ts->UILanguageFile;

	/* save current dir */
	GetCurrentDirectoryW(_countof(TempDir), TempDir);

	/* File name filter */
	if (FuncId==GSF_LOADKEY) {
		FNameFilter = _get_lang_msg("FILEDLG_KEYBOARD_FILTER", L"keyboard setup files (*.cnf)\\0*.cnf\\0\\0", UILanguageFile);
	}
	else {
		FNameFilter = _get_lang_msg("FILEDLG_SETUP_FILTER", L"setup files (*.ini)\\0*.ini\\0\\0", UILanguageFile);
	}

	if (FuncId==GSF_LOADKEY) {
		size_t i, j;
		wchar_t *KeyCnfFNW = ts->KeyCnfFNW;
		GetFileNamePosW(KeyCnfFNW,&i,&j);
		wcsncpy_s(NameW, _countof(NameW),&KeyCnfFNW[j], _TRUNCATE);
		memcpy(DirW, KeyCnfFNW, sizeof(wchar_t) * i);
		DirW[i] = 0;

		if ((wcslen(NameW) == 0) || (_wcsicmp(NameW, L"KEYBOARD.CNF") == 0)) {
			wcsncpy_s(NameW, _countof(NameW),L"KEYBOARD.CNF", _TRUNCATE);
		}
	}
	else {
		size_t i, j;
		wchar_t *SetupFNameW = ts->SetupFNameW;
		GetFileNamePosW(SetupFNameW,&i,&j);
		wcsncpy_s(NameW, _countof(NameW),&SetupFNameW[j], _TRUNCATE);
		memcpy(DirW, SetupFNameW, sizeof(wchar_t) * i);
		DirW[i] = 0;

		if ((wcslen(NameW) == 0) || (_wcsicmp(NameW, L"TERATERM.INI") == 0)) {
			wcsncpy_s(NameW, _countof(NameW), L"TERATERM.INI", _TRUNCATE);
		}
	}

	if (wcslen(DirW) == 0) {
		wchar_t *HomeDirW = ToWcharA(ts->HomeDir);
		wcsncpy_s(DirW, _countof(DirW), HomeDirW, _TRUNCATE);
		free(HomeDirW);
	}

	SetCurrentDirectoryW(DirW);

	/* OPENFILENAME record */
	OPENFILENAMEW ofn = {};
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = HWin;
	ofn.lpstrFile   = NameW;
	ofn.nMaxFile    = sizeof(NameW);
	ofn.lpstrFilter = FNameFilter;
	ofn.nFilterIndex = 1;
	ofn.hInstance = hInst;

	BOOL Ok;
	switch (FuncId) {
	case GSF_SAVE:
		ofn.lpstrDefExt = L"ini";
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_SHOWHELP;
		// 初期ファイルディレクトリをプログラム本体がある箇所に固定する (2005.1.6 yutaka)
		// 読み込まれたteraterm.iniがあるディレクトリに固定する。
		// これにより、/F= で指定された位置に保存されるようになる。(2005.1.26 yutaka)
		// Windows Vista ではファイル名まで指定すると NULL と同じ挙動をするようなので、
		// ファイル名を含まない形でディレクトリを指定するようにした。(2006.9.16 maya)
//		ofn.lpstrInitialDir = __argv[0];
//		ofn.lpstrInitialDir = ts->SetupFName;
		ofn.lpstrInitialDir = DirW;
		ofn.lpstrTitle = _get_lang_msg("FILEDLG_SAVE_SETUP_TITLE", L"Tera Term: Save setup", UILanguageFile);
		Ok = GetSaveFileNameW(&ofn);
		if (Ok) {
			free(ts->SetupFNameW);
			ts->SetupFNameW = _wcsdup(NameW);
			char *Name = ToCharW(NameW);
			strncpy_s(ts->SetupFName, sizeof(ts->SetupFName), Name, _TRUNCATE);
			free(Name);
		}
		break;
	case GSF_RESTORE:
		ofn.lpstrDefExt = L"ini";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHOWHELP;
		ofn.lpstrTitle = _get_lang_msg("FILEDLG_RESTORE_SETUP_TITLE", L"Tera Term: Restore setup", UILanguageFile);
		Ok = GetOpenFileNameW(&ofn);
		if (Ok) {
			free(ts->SetupFNameW);
			ts->SetupFNameW = _wcsdup(NameW);
			char *Name = ToCharW(NameW);
			strncpy_s(ts->SetupFName, sizeof(ts->SetupFName), Name, _TRUNCATE);
			free(Name);
		}
		break;
	case GSF_LOADKEY:
		ofn.lpstrDefExt = L"cnf";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHOWHELP;
		ofn.lpstrTitle = _get_lang_msg("FILEDLG_LOAD_KEYMAP_TITLE", L"Tera Term: Load key map", UILanguageFile);
		Ok = GetOpenFileNameW(&ofn);
		if (Ok) {
			free(ts->KeyCnfFNW);
			ts->KeyCnfFNW = _wcsdup(NameW);
			char *Name = ToCharW(NameW);
			strncpy_s(ts->KeyCnfFN, sizeof(ts->KeyCnfFN), Name, _TRUNCATE);
			free(Name);
		}
		break;
	default:
		assert(FALSE);
		Ok = FALSE;
		break;
	}

#if defined(_DEBUG)
	if (!Ok) {
		DWORD Err = GetLastError();
		DWORD DlgErr = CommDlgExtendedError();
		assert(Err == 0 && DlgErr == 0);
	}
#endif

	free(FNameFilter);
	free((void *)ofn.lpstrTitle);

	/* restore dir */
	SetCurrentDirectoryW(TempDir);

	return Ok;
}

void CVTWindow::OnSetupSave()
{
	wchar_t *PrevSetupFNW = _wcsdup(ts.SetupFNameW);
	HelpId = HlpSetupSave;
	BOOL Ok = _GetSetupFname(HVTWin,GSF_SAVE,&ts);
	if (! Ok) {
		free(PrevSetupFNW);
		return;
	}

	// 書き込みできるか?
	const DWORD attr = GetFileAttributesW(ts.SetupFNameW);
	if ((attr & FILE_ATTRIBUTE_DIRECTORY ) == 0 && (attr & FILE_ATTRIBUTE_READONLY) != 0) {
		// フォルダではなく、読み取り専用だった場合
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_TT_ERROR", L"Tera Term: ERROR",
			"MSG_SAVESETUP_PERMISSION_ERROR", L"TERATERM.INI file doesn't have the writable permission.",
			MB_OK|MB_ICONEXCLAMATION
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
		free(PrevSetupFNW);
		return;
	}

	if (LoadTTSET())
	{
		int w, h;

#ifdef WINDOW_MAXMIMUM_ENABLED
		if (::IsZoomed(m_hWnd)) {
			w = ts.TerminalWidth;
			h = ts.TerminalHeight;
			ts.TerminalWidth = ts.TerminalOldWidth;
			ts.TerminalHeight = ts.TerminalOldHeight;
		}
#endif

		CopyFileW(PrevSetupFNW, ts.SetupFNameW, TRUE);
		/* write current setup values to file */
		(*WriteIniFile)(ts.SetupFName, &ts);
		/* copy host list */
		char * PrevSetupFN = ToCharW(PrevSetupFNW);
		(*CopyHostList)(PrevSetupFN, ts.SetupFName);
		free(PrevSetupFN);
		FreeTTSET();
		free(PrevSetupFNW);
		PrevSetupFNW = NULL;

#ifdef WINDOW_MAXMIMUM_ENABLED
		if (::IsZoomed(m_hWnd)) {
			ts.TerminalWidth = w;
			ts.TerminalHeight = h;
		}
#endif
	}
}

void CVTWindow::OnSetupRestore()
{
	HelpId = HlpSetupRestore;
	BOOL Ok = _GetSetupFname(HVTWin, GSF_RESTORE, &ts);
	if (Ok) {
		RestoreSetup();
	}
}

//
// 現在読み込まれている teraterm.ini ファイルが格納されている
// フォルダをエクスプローラで開く。
//
void CVTWindow::OnOpenSetupDirectory()
{
	SetupDirectoryDialog(m_hInst, HVTWin, &ts);
}

void CVTWindow::OnSetupLoadKeyMap()
{
	HelpId = HlpSetupLoadKeyMap;
	BOOL Ok = _GetSetupFname(HVTWin,GSF_LOADKEY,&ts);
	if (! Ok) {
		return;
	}

	// load key map
	SetKeyMap();
}

void CVTWindow::OnControlResetTerminal()
{
	LockBuffer();
	HideStatusLine();
	DispScrollHomePos();
	ResetTerminal();
	DispResetColor(CS_ALL);
	UnlockBuffer();

	LButton = FALSE;
	MButton = FALSE;
	RButton = FALSE;

	Hold = FALSE;
	CommLock(&ts,&cv,FALSE);

	KeybEnabled  = TRUE;
}

void CVTWindow::OnControlResetRemoteTitle()
{
	cv.TitleRemote[0] = '\0';
	ChangeTitle();
}

void CVTWindow::OnControlAreYouThere()
{
	if (cv.Ready && (cv.PortType==IdTCPIP)) {
		TelSendAYT();
	}
}

void CVTWindow::OnControlSendBreak()
{
	if (cv.Ready)
		switch (cv.PortType) {
			case IdTCPIP:
				// SSH2接続の場合、専用のブレーク信号を送信する。(2010.9.28 yutaka)
				if (cv.isSSH == 2) {
					if (TTXProcessCommand(HVTWin, ID_CONTROL_SENDBREAK)) {
						break;
					}
				}

				TelSendBreak();
				break;
			case IdSerial:
				CommSendBreak(&cv, ts.SendBreakTime);
				break;
		}
}

void CVTWindow::OnControlResetPort()
{
	CommResetSerial(&ts, &cv, TRUE);
}

void CVTWindow::OnControlBroadcastCommand(void)
{
	BroadCastShowDialog(m_hInst, HVTWin);
}

// WM_COPYDATAの受信
LRESULT CVTWindow::OnReceiveIpcMessage(WPARAM wParam, LPARAM lParam)
{
	if (!cv.Ready) {
		return 0;
	}

	if (!ts.AcceptBroadcast) { // 337: 2007/03/20
		return 0;
	}

	// 未送信データがある場合は先に送信する
	// データ量が多い場合は送信しきれない可能性がある
	if (TalkStatus == IdTalkSendMem) {
		SendMemContinuously();	// TODO 必要?
	}

	// 送信可能な状態でなければエラー
	if (TalkStatus != IdTalkKeyb) {
		return 0;
	}

	const COPYDATASTRUCT *cds = (COPYDATASTRUCT *)lParam;
	BroadCastReceive(cds);

	return 1; // 送信できた場合は1を返す
}

void CVTWindow::OnControlOpenTEK()
{
	OpenTEK();
}

void CVTWindow::OnControlCloseTEK()
{
	if ((HTEKWin==NULL) ||
		! ::IsWindowEnabled(HTEKWin)) {
		MessageBeep(0);
	}
	else {
		::DestroyWindow(HTEKWin);
	}
}

void CVTWindow::OnControlMacro()
{
	RunMacro(NULL,FALSE);
}

void CVTWindow::OnShowMacroWindow()
{
	RunMacro(NULL,FALSE);
}

void CVTWindow::OnWindowWindow()
{
	BOOL Close;

	HelpId = HlpWindowWindow;
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	(*WindowWindow)(HVTWin,&Close);
	FreeTTDLG();
	if (Close) {
		OnClose();
	}
}

void CVTWindow::OnWindowMinimizeAll()
{
	ShowAllWin(SW_MINIMIZE);
}

void CVTWindow::OnWindowCascade()
{
	ShowAllWinCascade(HVTWin);
}

void CVTWindow::OnWindowStacked()
{
	ShowAllWinStacked(HVTWin);
}

void CVTWindow::OnWindowSidebySide()
{
	ShowAllWinSidebySide(HVTWin);
}

void CVTWindow::OnWindowRestoreAll()
{
	ShowAllWin(SW_RESTORE);
}

void CVTWindow::OnWindowUndo()
{
	UndoAllWin();
}

void CVTWindow::OnHelpIndex()
{
	OpenHelp(HH_DISPLAY_TOPIC, 0, ts.UILanguageFile);
}

void CVTWindow::OnHelpAbout()
{
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	(*AboutDialog)(HVTWin);
	FreeTTDLG();
}

LRESULT CVTWindow::OnDpiChanged(WPARAM wp, LPARAM)
{
	const UINT NewDPI = LOWORD(wp);
	// 現在のウィンドウサイズ
	RECT CurrentWindowRect;
	::GetWindowRect(m_hWnd, &CurrentWindowRect);
	const int CurrentWindowWidth = CurrentWindowRect.right - CurrentWindowRect.left;
	const int CurrentWindowHeight = CurrentWindowRect.bottom - CurrentWindowRect.top;

	// ポインタの位置
	POINT MouseCursorScreen;
	GetCursorPos(&MouseCursorScreen);
	POINT MouseCursorInWindow = MouseCursorScreen;
	MouseCursorInWindow.x -= CurrentWindowRect.left;
	MouseCursorInWindow.y -= CurrentWindowRect.top;

	// 新しいDPIに合わせてフォントを生成、
	// クライアント領域のサイズを決定する
	ChangeFont();
	ScreenWidth = WinWidth*FontWidth;
	ScreenHeight = WinHeight*FontHeight;
	//AdjustScrollBar();

	// スクリーンサイズ(=Client Areaのサイズ)からウィンドウサイズを算出
	int NewWindowWidth;
	int NewWindowHeight;
	if (pAdjustWindowRectExForDpi != NULL || pAdjustWindowRectEx != NULL) {
		const DWORD Style = (DWORD)::GetWindowLongPtr(m_hWnd, GWL_STYLE);
		const DWORD ExStyle = (DWORD)::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
		const BOOL bMenu = (ts.PopupMenu != 0) ? FALSE : TRUE;
		if (pGetSystemMetricsForDpi != NULL) {
			// スクロールバーが表示されている場合は、
			// スクリーンサイズ(クライアントエリアのサイズ)に追加する
			int min_pos;
			int max_pos;
			GetScrollRange(m_hWnd, SB_VERT, &min_pos, &max_pos);
			if (min_pos != max_pos) {
				ScreenWidth += pGetSystemMetricsForDpi(SM_CXVSCROLL, NewDPI);
			}
			GetScrollRange(m_hWnd, SB_HORZ, &min_pos, &max_pos);
			if (min_pos != max_pos) {
				ScreenHeight += pGetSystemMetricsForDpi(SM_CXHSCROLL, NewDPI);
			}
		}
		RECT Rect = {0, 0, ScreenWidth, ScreenHeight};
		if (pAdjustWindowRectExForDpi != NULL) {
			// Windows 10, version 1607+
			pAdjustWindowRectExForDpi(&Rect, Style, bMenu, ExStyle, NewDPI);
		}
		else {
			// Windows 2000+
			pAdjustWindowRectEx(&Rect, Style, bMenu, ExStyle);
		}
		NewWindowWidth = Rect.right - Rect.left;
		NewWindowHeight = Rect.bottom - Rect.top;
	}
	else {
		// WM_DPICHANGEDが発生しない環境のはず、念の為実装
		RECT WindowRect;
		GetWindowRect(&WindowRect);
		const int WindowWidth = WindowRect.right - WindowRect.left;
		const int WindowHeight = WindowRect.bottom - WindowRect.top;
		RECT ClientRect;
		GetClientRect(&ClientRect);
		const int ClientWidth =  ClientRect.right - ClientRect.left;
		const int ClientHeight = ClientRect.bottom - ClientRect.top;
		NewWindowWidth = WindowWidth - ClientWidth + ScreenWidth;
		NewWindowHeight = WindowHeight - ClientHeight + ScreenHeight;
	}

	// 新しいウィンドウ領域候補
	RECT NewWindowRect[5];

	// タイトルバー上のポインタ位置がなるべくずれない新しい位置
	int t1 = (int)MouseCursorInWindow.y * (int)NewWindowHeight / (int)CurrentWindowHeight;
	NewWindowRect[0].top =
		CurrentWindowRect.top -
		(t1 - (int)MouseCursorInWindow.y);
	t1 = (int)MouseCursorInWindow.x * (int)NewWindowWidth / (int)CurrentWindowWidth;
	NewWindowRect[0].left =
		CurrentWindowRect.left -
		(t1 - (int)MouseCursorInWindow.x);
	NewWindowRect[0].bottom = NewWindowRect[0].top + NewWindowHeight;
	NewWindowRect[0].right = NewWindowRect[0].left + NewWindowWidth;

	// 現在位置から上右寄せ
	NewWindowRect[1].top = CurrentWindowRect.top;
	NewWindowRect[1].bottom = CurrentWindowRect.top + NewWindowHeight;
	NewWindowRect[1].left = CurrentWindowRect.right - NewWindowWidth;
	NewWindowRect[1].right = CurrentWindowRect.right;

	// 現在位置から上左寄せ
	NewWindowRect[2].top = CurrentWindowRect.top;
	NewWindowRect[2].bottom = CurrentWindowRect.top + NewWindowHeight;
	NewWindowRect[2].left = CurrentWindowRect.left;
	NewWindowRect[2].right = CurrentWindowRect.left + NewWindowWidth;

	// 現在位置から下右寄せ
	NewWindowRect[3].top = CurrentWindowRect.bottom - NewWindowHeight;
	NewWindowRect[3].bottom = CurrentWindowRect.top;
	NewWindowRect[3].left = CurrentWindowRect.right - NewWindowWidth;
	NewWindowRect[3].right = CurrentWindowRect.right;

	// 現在位置から下左寄せ
	NewWindowRect[4].top = CurrentWindowRect.bottom - NewWindowHeight;
	NewWindowRect[4].bottom = CurrentWindowRect.top;
	NewWindowRect[4].left = CurrentWindowRect.left;
	NewWindowRect[4].right = CurrentWindowRect.left + NewWindowWidth;

	// 確認
	const RECT *NewRect = &NewWindowRect[0];
	for (size_t i=0; i < _countof(NewWindowRect); i++) {
		const RECT *r = &NewWindowRect[i];
		HMONITOR hMonitor = pMonitorFromRect(r, MONITOR_DEFAULTTONULL);
		UINT dpiX;
		UINT dpiY;
		pGetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
		if (NewDPI == dpiX) {
			NewRect = r;
			break;
		}
	}

	IgnoreSizeMessage = TRUE;
	::SetWindowPos(m_hWnd, NULL,
				   NewRect->left, NewRect->top,
				   NewWindowWidth, NewWindowHeight,
				   SWP_NOZORDER);
	IgnoreSizeMessage = FALSE;

	ChangeCaret();

	return TRUE;
}

LRESULT CVTWindow::Proc(UINT msg, WPARAM wp, LPARAM lp)
{
	LRESULT retval = 0;
	if (msg == MsgDlgHelp) {
		// HELPMSGSTRING message 時
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
	case WM_UNICHAR:
		retval = OnUniChar(wp, lp);
		break;
	case WM_CLOSE:
		OnClose();
		break;
	case WM_DESTROY:
		OnDestroy();
		PostQuitMessage(0);
		break;
	case WM_DROPFILES:
		OnDropFiles((HDROP)wp);
		break;
	case WM_GETMINMAXINFO:
		OnGetMinMaxInfo((MINMAXINFO *)lp);
		break;
	case WM_HSCROLL:
		OnHScroll(LOWORD(wp), HIWORD(wp), (HWND)lp);
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
	case WM_LBUTTONDBLCLK:
		OnLButtonDblClk(wp, MAKEPOINTS(lp));
		break;
	case WM_LBUTTONDOWN:
		OnLButtonDown(wp, MAKEPOINTS(lp));
		break;
	case WM_LBUTTONUP:
		OnLButtonUp(wp, MAKEPOINTS(lp));
		break;
	case WM_MBUTTONDOWN:
		OnMButtonDown(wp, MAKEPOINTS(lp));
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
	case WM_MOUSEWHEEL:
		OnMouseWheel(GET_KEYSTATE_WPARAM(wp), GET_WHEEL_DELTA_WPARAM(wp), MAKEPOINTS(lp));
		break;
	case WM_MOVE:
		OnMove(LOWORD(lp), HIWORD(lp));
		break;
	case WM_NCLBUTTONDBLCLK:
		OnNcLButtonDblClk((UINT)wp, MAKEPOINTS(lp));
		DefWindowProc(msg, wp, lp);
		break;
	case WM_NCRBUTTONDOWN:
		OnNcRButtonDown((UINT)wp, MAKEPOINTS(lp));
		break;
	case WM_PAINT:
		OnPaint();
		break;
	case WM_RBUTTONDOWN:
		OnRButtonDown((UINT)wp, MAKEPOINTS(lp));
		break;
	case WM_RBUTTONUP:
		OnRButtonUp((UINT)wp, MAKEPOINTS(lp));
		break;
	case WM_SETFOCUS:
#if !UNICODE_DEBUG_CARET_OFF
		OnSetFocus((HWND)wp);
#endif
		DefWindowProc(msg, wp, lp);
		break;
	case WM_SIZE:
		OnSize(wp, LOWORD(lp), HIWORD(lp));
		break;
	case WM_SIZING:
		OnSizing(wp, (LPRECT)lp);
		break;
	case WM_SYSCHAR:
		OnSysChar(wp, LOWORD(lp), HIWORD(lp));
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
	case WM_VSCROLL:
		OnVScroll(LOWORD(wp), HIWORD(wp), (HWND)lp);
		break;
	case WM_DEVICECHANGE:
		OnDeviceChange((UINT)wp, (DWORD_PTR)lp);
		DefWindowProc(msg, wp, lp);
		break;
	case WM_IME_STARTCOMPOSITION :
		OnIMEStartComposition(wp, lp);
		break;
	case WM_IME_ENDCOMPOSITION :
		OnIMEEndComposition(wp, lp);
		break;
	case WM_IME_COMPOSITION:
		OnIMEComposition(wp, lp);
		break;
	case WM_INPUTLANGCHANGE:
		OnIMEInputChange(wp, lp);
		break;
	case WM_IME_NOTIFY:
		OnIMENotify(wp, lp);
		break;
	case WM_IME_REQUEST:
		retval = OnIMERequest(wp, lp);
		break;
	case WM_WINDOWPOSCHANGING:
		OnWindowPosChanging(wp, lp);
		break;
	case WM_SETTINGCHANGE:
		OnSettingChange(wp, lp);
		break;
	case WM_ENTERSIZEMOVE:
		OnEnterSizeMove(wp, lp);
		break;
	case WM_EXITSIZEMOVE :
		OnExitSizeMove(wp, lp);
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
	case WM_USER_COMMNOTIFY:
		OnCommNotify(wp, lp);
		break;
	case WM_USER_COMMOPEN:
		OnCommOpen(wp, lp);
		break;
	case WM_USER_COMMSTART:
		OnCommStart(wp, lp);
		break;
	case WM_USER_DDEEND:
		OnDdeEnd(wp, lp);
		break;
	case WM_USER_DLGHELP2:
		OnDlgHelp(wp, lp);
		break;
#if 0
	case WM_USER_FTCANCEL:
		OnFileTransEnd(wp, lp);
		break;
#endif
	case WM_USER_GETSERIALNO:
		retval = OnGetSerialNo(wp, lp);
		break;
	case WM_USER_KEYCODE:
		OnKeyCode(wp, lp);
		break;
	case WM_USER_PROTOCANCEL:
		OnProtoEnd(wp, lp);
		break;
	case WM_USER_CHANGETITLE:
		OnChangeTitle(wp, lp);
		break;
	case WM_COPYDATA:
		SetTimer(m_hWnd, IdPasteDelayTimer, 0, NULL);  // idle処理を動作させるため
		OnReceiveIpcMessage(wp, lp);
		break;
	case WM_USER_NONCONFIRM_CLOSE:
		OnNonConfirmClose(wp, lp);
		break;
	case WM_USER_NOTIFYICON:
		OnNotifyIcon(wp, lp);
		break;
	case WM_USER_DROPNOTIFY:
		OnDropNotify(wp, lp);
		break;
	case WM_DPICHANGED:
		OnDpiChanged(wp, lp);
		break;
	case WM_COMMAND:
	{
		const WORD wID = GET_WM_COMMAND_ID(wp, lp);
		switch (wID) {
		case ID_FILE_NEWCONNECTION: OnFileNewConnection(); break;
		case ID_FILE_DUPLICATESESSION: OnDuplicateSession(); break;
		case ID_FILE_CYGWINCONNECTION: OnCygwinConnection(); break;
		case ID_FILE_TERATERMMENU: OnTTMenuLaunch(); break;
		case ID_FILE_LOGMEIN: OnLogMeInLaunch(); break;
		case ID_FILE_LOG: OnFileLog(); break;
		case ID_FILE_COMMENTTOLOG: OnCommentToLog(); break;
		case ID_FILE_VIEWLOG: OnViewLog(); break;
		case ID_FILE_SHOWLOGDIALOG: OnShowLogDialog(); break;
		case ID_FILE_PAUSELOG: OnPauseLog(); break;
		case ID_FILE_STOPLOG: OnStopLog(); break;
		case ID_FILE_REPLAYLOG: OnReplayLog(); break;
		case ID_FILE_SENDFILE: OnFileSend(); break;
		case ID_FILE_KERMITRCV: OnFileKermitRcv(); break;
		case ID_FILE_KERMITGET: OnFileKermitGet(); break;
		case ID_FILE_KERMITSEND: OnFileKermitSend(); break;
		case ID_FILE_KERMITFINISH: OnFileKermitFinish(); break;
		case ID_FILE_XRCV: OnFileXRcv(); break;
		case ID_FILE_XSEND: OnFileXSend(); break;
		case ID_FILE_YRCV: OnFileYRcv(); break;
		case ID_FILE_YSEND: OnFileYSend(); break;
		case ID_FILE_ZRCV: OnFileZRcv(); break;
		case ID_FILE_ZSEND: OnFileZSend(); break;
		case ID_FILE_BPRCV: OnFileBPRcv(); break;
		case ID_FILE_BPSEND: OnFileBPSend(); break;
		case ID_FILE_QVRCV: OnFileQVRcv(); break;
		case ID_FILE_QVSEND: OnFileQVSend(); break;
		case ID_FILE_CHANGEDIR: OnFileChangeDir(); break;
		case ID_FILE_PRINT2: OnFilePrint(); break;
		case ID_FILE_DISCONNECT: OnFileDisconnect(); break;
		case ID_FILE_EXIT: OnFileExit(); break;
		case ID_FILE_EXITALL: OnAllClose(); break;
		case ID_EDIT_COPY2: OnEditCopy(); break;
		case ID_EDIT_COPYTABLE: OnEditCopyTable(); break;
		case ID_EDIT_PASTE2: OnEditPaste(); break;
		case ID_EDIT_PASTECR: OnEditPasteCR(); break;
		case ID_EDIT_CLEARSCREEN: OnEditClearScreen(); break;
		case ID_EDIT_CLEARBUFFER: OnEditClearBuffer(); break;
		case ID_EDIT_CANCELSELECT: OnEditCancelSelection(); break;
		case ID_EDIT_SELECTALL: OnEditSelectAllBuffer(); break;
		case ID_EDIT_SELECTSCREEN: OnEditSelectScreenBuffer(); break;
		case ID_SETUP_ADDITIONALSETTINGS: OnExternalSetup(); break;
		case ID_SETUP_TERMINAL: OnSetupTerminal(); break;
		case ID_SETUP_WINDOW: OnSetupWindow(); break;
		case ID_SETUP_FONT: OnSetupFont(); break;
		case ID_SETUP_DLG_FONT: OnSetupDlgFont(); break;
		case ID_SETUP_KEYBOARD: OnSetupKeyboard(); break;
		case ID_SETUP_SERIALPORT: OnSetupSerialPort(); break;
		case ID_SETUP_TCPIP: OnSetupTCPIP(); break;
		case ID_SETUP_GENERAL: OnSetupGeneral(); break;
		case ID_SETUP_SAVE: OnSetupSave(); break;
		case ID_SETUP_RESTORE: OnSetupRestore(); break;
		case ID_OPEN_SETUP: OnOpenSetupDirectory(); break;
		case ID_SETUP_LOADKEYMAP: OnSetupLoadKeyMap(); break;
		case ID_CONTROL_RESETTERMINAL: OnControlResetTerminal(); break;
		case ID_CONTROL_RESETREMOTETITLE: OnControlResetRemoteTitle(); break;
		case ID_CONTROL_AREYOUTHERE: OnControlAreYouThere(); break;
		case ID_CONTROL_SENDBREAK: OnControlSendBreak(); break;
		case ID_CONTROL_RESETPORT: OnControlResetPort(); break;
		case ID_CONTROL_BROADCASTCOMMAND: OnControlBroadcastCommand(); break;
		case ID_CONTROL_OPENTEK: OnControlOpenTEK(); break;
		case ID_CONTROL_CLOSETEK: OnControlCloseTEK(); break;
		case ID_CONTROL_MACRO: OnControlMacro(); break;
		case ID_CONTROL_SHOW_MACRO: OnShowMacroWindow(); break;
		case ID_WINDOW_WINDOW: OnWindowWindow(); break;
		case ID_WINDOW_MINIMIZEALL: OnWindowMinimizeAll(); break;
		case ID_WINDOW_CASCADEALL: OnWindowCascade(); break;
		case ID_WINDOW_STACKED: OnWindowStacked(); break;
		case ID_WINDOW_SIDEBYSIDE: OnWindowSidebySide(); break;
		case ID_WINDOW_RESTOREALL: OnWindowRestoreAll(); break;
		case ID_WINDOW_UNDO: OnWindowUndo(); break;
		case ID_HELP_INDEX2: OnHelpIndex(); break;
		case ID_HELP_ABOUT: OnHelpAbout(); break;
		default:
			OnCommand(wp, lp);
			break;
		}
		break;
	}
	case WM_NCHITTEST: {
		retval = TTCFrameWnd::DefWindowProc(msg, wp ,lp);
		if (ts.HideTitle>0) {
			if ((retval == HTCLIENT) && AltKey()) {
#ifdef ALPHABLEND_TYPE2
			if(ShiftKey())
				retval = HTBOTTOMRIGHT;
			else
				retval = HTCAPTION;
#else
			retval = HTCAPTION;
#endif
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
