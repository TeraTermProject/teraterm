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

// SDK7.0�̏ꍇ�AWIN32_IE���K�؂ɒ�`����Ȃ�
#if _MSC_VER == 1400	// VS2005�̏ꍇ�̂�
#if !defined(_WIN32_IE)
#define	_WIN32_IE 0x0501
#endif
#endif

#include "teraterm.h"
#include "tttypes.h"
#include "tttypes_key.h"
#include "tttypes_charset.h"

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
#include "cyglib.h"
#include "theme.h"

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
#include "themedlg.h"
#include "ttcmn_static.h"
#include "ttcmn_notify2.h"
#include "scp.h"
#include "ttcommdlg.h"
#include "logdlg.h"
#include "makeoutputstring.h"
#include "ttlib_types.h"

#include <initguid.h>
#if _MSC_VER < 1600
// Visual Studio 2005,2008 �̂Ƃ��A2010���Â��o�[�W�����̂Ƃ�
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, \
             0xC0, 0x4F, 0xB9, 0x51, 0xED);
#else
#include <usbiodef.h>	// GUID_DEVINTERFACE_USB_DEVICE
#endif

#include "win32helper.h"

#define VTClassName L"VTWin32"

// �E�B���h�E�ő剻�{�^����L���ɂ��� (2005.1.15 yutaka)
#define WINDOW_MAXMIMUM_ENABLED 1

static BOOL TCPLocalEchoUsed = FALSE;
static BOOL TCPCRSendUsed = FALSE;

static BOOL IgnoreRelease = FALSE;

static HDEVNOTIFY hDevNotify = NULL;

static int AutoDisconnectedPort = -1;

UnicodeDebugParam_t UnicodeDebugParam;
typedef struct {
	char dbcs_lead_byte;
	UINT monitor_DPI;			// �E�B���h�E���\������Ă���f�B�X�v���C��DPI
	DWORD help_id;				// WM_HELP���b�Z�[�W���A�\������w���vID(0�ŕ\�����Ȃ�)
} vtwin_work_t;
static vtwin_work_t vtwin_work;

extern "C" PrintFile *PrintFile_;

static CVTWindow *pVTWin;

/////////////////////////////////////////////////////////////////////////////
// CVTWindow

// Tera Term�N������URL������mouse over���ɌĂ΂�� (2005.4.2 yutaka)
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
		return;	// ���C���[�h�E�C���h�E�̃T�|�[�g�Ȃ�
	}
	if (Alpha == alpha) {
		return;	// �ω��Ȃ��Ȃ牽�����Ȃ�
	}
	LONG_PTR lp = GetWindowLongPtr(GWL_EXSTYLE);
	if (lp == 0) {
		return;
	}

	// 2006/03/16 by 337: BGUseAlphaBlendAPI��On�Ȃ��Layered�����Ƃ���
	//if (ts->EtermLookfeel.BGUseAlphaBlendAPI) {
	// �A���t�@�l��255�̏ꍇ�A��ʂ̂������}���邽�߉������Ȃ����ƂƂ���B(2006.4.1 yutaka)
	// �Ăяo�����ŁA�l���ύX���ꂽ�Ƃ��̂ݐݒ�𔽉f����B(2007.10.19 maya)
	if (alpha < 255) {
		::SetWindowLongPtr(HVTWin, GWL_EXSTYLE, lp | WS_EX_LAYERED);
		pSetLayeredWindowAttributes(HVTWin, 0, alpha, LWA_ALPHA);
	}
	else {
		// �A���t�@�l�� 255 �̏ꍇ�A�������������폜���čĕ`�悷��B(2007.10.22 maya)
		::SetWindowLongPtr(HVTWin, GWL_EXSTYLE, lp & ~WS_EX_LAYERED);
		::RedrawWindow(HVTWin, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME);
	}
	Alpha = alpha;
}

static HDEVNOTIFY RegDeviceNotify(HWND hWnd)
{
	DEV_BROADCAST_DEVICEINTERFACE filter;
	HDEVNOTIFY h;

	if (pRegisterDeviceNotificationA == NULL) {
		return NULL;
	}

	ZeroMemory(&filter, sizeof(filter));
	filter.dbcc_size = sizeof(filter);
	filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	filter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
	h = pRegisterDeviceNotificationA(hWnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
	return h;
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
	int CmdShow;
	BOOL isFirstInstance;
	m_hInst = hInstance;

	CommInit(&cv);
	cv.ts = &ts;
	isFirstInstance = StartTeraTerm(m_hInst, &ts);

	TTXInit(&ts, &cv); /* TTPLUG */

	MsgDlgHelp = RegisterWindowMessage(HELPMSGSTRING);

	if (isFirstInstance) {
		/* first instance */
		if (LoadTTSET()) {
			/* read setup info from "teraterm.ini" */
			(*ReadIniFile)(ts.SetupFNameW, &ts);
			FreeTTSET();
		}
		else {
			abort();
		}

	} else {
		// 2�߈ȍ~�̃v���Z�X�ɂ����Ă��A�f�B�X�N���� TERATERM.INI ��ǂށB(2004.11.4 yutaka)
		if (LoadTTSET()) {
			/* read setup info from "teraterm.ini" */
			(*ReadIniFile)(ts.SetupFNameW, &ts);
			FreeTTSET();
		}
		else {
			abort();
		}
	}

	/* Parse command line parameters*/
	if (LoadTTSET()) {
		// GetCommandLineW() in MSDN remark
		//  The lifetime of the returned value is managed by the
		//  system, applications should not free or modify this value.
		wchar_t *ParamW = _wcsdup(GetCommandLineW());
		(*ParseParam)(ParamW, &ts, &(TopicName[0]));
		free(ParamW);
	}
	FreeTTSET();

	// DPI Aware (��DPI�Ή�)
	if (pIsValidDpiAwarenessContext != NULL && pSetThreadDpiAwarenessContext != NULL) {
		wchar_t Temp[4];
		GetPrivateProfileStringW(L"Tera Term", L"DPIAware", L"on", Temp, _countof(Temp), ts.SetupFNameW);
		if (_wcsicmp(Temp, L"on") == 0) {
			if (pIsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == TRUE) {
				pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			}
		}
	}

	// duplicate session�̎w�肪����Ȃ�A���L����������R�s�[���� (2004.12.7 yutaka)
	if (ts.DuplicateSession == 1) {
		// ���L�������̍��W�͕������̌��݂̃E�B���h�E���W�ɂȂ�B
		// ��œǂݍ��� TERATERM.INI �̒l���g�������̂ŁA�Ҕ����Ė߂��B
		POINT VTPos = ts.VTPos;
		POINT TEKPos = ts.TEKPos;

		CopyShmemToTTSet(&ts);

		ts.VTPos = VTPos;
		ts.TEKPos = TEKPos;
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
	ScrollLock = FALSE;  // �����l�͖��� (2006.11.14 yutaka)
	Alpha = 255;
#if UNICODE_DEBUG
	TipWinCodeDebug = NULL;
#endif
	vtwin_work.dbcs_lead_byte = 0;
	vtwin_work.monitor_DPI = 0;
	vtwin_work.help_id = 0;

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
        UnicodeDebugParam.CodePageForANSIDraw = GetACP();
	}

	/* Initialize scroll buffer */
	UnicodeDebugParam.UseUnicodeApi = IsWindowsNTKernel() ? TRUE : FALSE;
	InitBuffer(UnicodeDebugParam.UseUnicodeApi);
	BuffSetDispCodePage(UnicodeDebugParam.CodePageForANSIDraw);

	InitDisp();
	BGLoadThemeFile(&ts);

	if (ts.HideTitle>0) {
		Style = WS_VSCROLL | WS_HSCROLL |
		        WS_BORDER | WS_THICKFRAME | WS_POPUP;

		if (ts.EtermLookfeel.BGNoFrame)
			Style &= ~(WS_BORDER | WS_THICKFRAME);
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
	wc.hCursor = NULL; // �}�E�X�J�[�\���͓��I�ɕύX���� (2005.4.2 yutaka)
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
	cv.HWin = HVTWin;

	// Windows 11 �ŃE�B���h�E�̊p���ۂ��Ȃ�Ȃ��悤�ɂ���
	if (ts.WindowCornerDontround && pDwmSetWindowAttribute != NULL) {
		DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_DONOTROUND;
		pDwmSetWindowAttribute(HVTWin, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
	}

	// register this window to the window list
	SerialNo = RegWin(HVTWin,NULL);

	logfile_lock_initialize();
	SetMouseCursor(ts.MouseCursorName);

	if(ts.EtermLookfeel.BGNoFrame && ts.HideTitle > 0) {
		DWORD ExStyle = (DWORD)::GetWindowLongPtr(HVTWin,GWL_EXSTYLE);
		ExStyle &= ~WS_EX_CLIENTEDGE;
		::SetWindowLongPtr(HVTWin,GWL_EXSTYLE,ExStyle);
	}

	// USB�f�o�C�X�ω��ʒm�o�^
	hDevNotify = RegDeviceNotify(HVTWin);

	// �ʒm�̈揉����
	NotifyIcon *ni = Notify2Initialize();
	cv.NotifyIcon = ni;
	Notify2SetWindow(ni, m_hWnd, WM_USER_NOTIFYICON, m_hInst, (ts.VTIcon != IdIconDefault) ? ts.VTIcon: IDI_VT);
	Notify2SetSound(ni, ts.NotifySound);

	// VT �E�B���h�E�̃A�C�R��
	SetVTIconID(&cv, NULL, 0);

	MainMenu = NULL;
	WinMenu = NULL;
	if ((ts.HideTitle==0) && (ts.PopupMenu==0)) {
		InitMenu(&MainMenu);
		::SetMenu(HVTWin,MainMenu);
	}

	cv.StateEcho = MakeOutputStringCreate();
	cv.StateSend = MakeOutputStringCreate();

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

	DropInit();
	DropLists = NULL;
	DropListCount = 0;

#if UNICODE_DEBUG
	CtrlKeyState = 0;
#endif

	// TipWin
	if (ts.HideWindow==0) {
		TipWin = new CTipWin(hInstance);
		TipWin->Create(HVTWin);
	}

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

	pVTWin = this;
}

/////////////////////////////////////////////////////////////////////////////
// CVTWindow destructor

CVTWindow::~CVTWindow()
{
	if (ts.HideWindow==0) {
		TipWin->Destroy();
		delete TipWin;
		TipWin = NULL;
	}
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

	// SelectOnlyByLButton �� on �� ���E�E�N���b�N�����Ƃ���
	// �o�b�t�@���I����Ԃ�������A�I����e���N���b�v�{�[�h��
	// �R�s�[����Ă��܂������C�� (2007.12.6 maya)
	if (!disableBuffEndSelect) {
		// �I��̈�̕������擾�A�N���b�v�{�[�h�փZ�b�g����
		wchar_t *strW = BuffEndSelect();
		if (strW != NULL) {
			CBSetTextW(HVTWin, strW, 0);
			free(strW);
		}
	}

	if (Paste) {
		CBStartPaste(HVTWin, FALSE, BracketedPasteMode());

		// �X�N���[���ʒu�����Z�b�g
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

	SetDlgMenuTextsW(FileMenu, FileMenuTextInfo, _countof(FileMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(EditMenu, EditMenuTextInfo, _countof(EditMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(SetupMenu, SetupMenuTextInfo, _countof(SetupMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(ControlMenu, ControlMenuTextInfo, _countof(ControlMenuTextInfo), ts.UILanguageFileW);
	SetDlgMenuTextsW(HelpMenu, HelpMenuTextInfo, _countof(HelpMenuTextInfo), ts.UILanguageFileW);

	if ((ts.MenuFlag & MF_SHOWWINMENU) !=0) {
		wchar_t *uimsg;
		WinMenu = CreatePopupMenu();
		GetI18nStrWW("Tera Term", "MENU_WINDOW", L"&Window", ts.UILanguageFileW, &uimsg);
		InsertMenuW(hMenu, ID_HELPMENU,
					MF_STRING | MF_ENABLED | MF_POPUP | MF_BYPOSITION,
					(UINT_PTR)WinMenu, uimsg);
		free(uimsg);
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

		// �V�K���j���[��ǉ� (2004.12.5 yutaka)
		EnableMenuItem(FileMenu,ID_FILE_CYGWINCONNECTION,MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(FileMenu,ID_FILE_TERATERMMENU,MF_BYCOMMAND | MF_ENABLED);

		// XXX: ���̈ʒu�ɂ��Ȃ��ƁAlog���O���C�ɂȂ�Ȃ��B (2005.2.1 yutaka)
		if (FLogIsOpend()) { // ���O�̎惂�[�h�̏ꍇ
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
		if (BuffIsSelected()) {
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
		 * �l�b�g���[�N�ڑ���(TCP/IP��I�����Đڑ��������)�̓V���A���|�[�g
		 * (ID_SETUP_SERIALPORT)�̃��j���[���I���ł��Ȃ��悤�ɂȂ��Ă������A
		 * ���̃K�[�h���O���A�V���A���|�[�g�ݒ�_�C�A���O����V�����ڑ����ł���悤�ɂ���B
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
		SetWinMenuW(WinMenu, ts.UILanguageFileW, 1);
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

#if 0
	// �N�����݂̂ɓǂݍ��ރe�[�}�������ɂȂ��Ă��܂��̂ō폜
	BGInitialize(FALSE);
	BGSetupPrimary(TRUE);
#endif

	// 2006/03/17 by 337 : Alpha�l�������ύX
	// Layered���ɂȂ��Ă��Ȃ��ꍇ�͌��ʂ�����
	//
	// AlphaBlend �𑦎����f�ł���悤�ɂ���B
	// (2016.12.24 yutaka)
	SetWindowAlpha(ts.AlphaBlendActive);

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
		(*ReadIniFile)(ts.SetupFNameW, &ts);

		SetColor();
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
	/* OnCommOpen �ŊJ�n�����̂ł����ł͊J�n���Ȃ� (2007.5.14 maya) */

	if ((TopicName[0]==0) && (ts.MacroFNW != NULL)) {
		// start the macro specified in the command line or setup file
		RunMacroW(ts.MacroFNW, TRUE);
		free(ts.MacroFNW);
		ts.MacroFNW = NULL;
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
 *	�L�[�{�[�h����1��������
 *	@param	nChar	UTF-16 char(wchar_t)	IsWindowUnicode() == TRUE ��
 *					ANSI char(char)			IsWindowUnicode() == FALSE ��
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
		// ���͂� UTF-16
		u16 = (wchar_t)nChar;
	} else {
		// ���͂� ANSI
		const UINT acp = GetACP();
		if ((acp == 932) || (acp == 949) || (acp == 936) || (acp == 950)) {
			// CP932	���{�� shift jis
			// CP949	Korean
			// CP936	GB2312
			// CP950	Big5
			// CJK (2byte����)
			if (vtwin_work.dbcs_lead_byte == 0 && IsDBCSLeadByte(nChar)) {
				// ANSI 2�o�C�g������ 1byte�ڂ�����
				//	�ʏ�� WM_IME_* ���b�Z�[�W�ŏ��������
				//	���̏ꍇ�����ɓ����Ă���
				//		TERATERM.INI �� IME=off �̂Ƃ�
				//		imm32.dll �����[�h�ł��Ȃ������Ƃ�
				vtwin_work.dbcs_lead_byte = nChar;
				return;
			}
			else {
				// ANSI(ACP) -> UTF-32 -> UTF-16
				char mb_str[2];
				size_t mb_len;
				if (vtwin_work.dbcs_lead_byte == 0) {
					// 1�o�C�g����
					mb_str[0] = (char)nChar;
					mb_len = 1;
				}
				else {
					// 2�o�C�g����
					mb_str[0] = (char)vtwin_work.dbcs_lead_byte;
					mb_str[1] = (char)nChar;
					mb_len = 2;
					vtwin_work.dbcs_lead_byte = 0;
				}
				unsigned int u32;
				mb_len = MBCPToUTF32(mb_str, mb_len, CP_ACP, &u32);
				if (mb_len == 0) {
					return;
				}
				u16 = (wchar_t)u32;
			}
		}
		else if (acp == 1251) {
			// CP1251	Russian,Cyrillic �L����
			BYTE c;
			if (ts.RussKeyb == IdWindows) {
				// key = CP1251
				c = (char)nChar;
			}
			else {
				// key -> CP1251
				c = RussConv(ts.RussKeyb, IdWindows, nChar);
			}
			// CP1251 -> UTF-32 -> UTF-16
			unsigned long u32 = MBCP_UTF32(c, 1251);
			u16 = (wchar_t)u32;
		}
		else {
			u16 = (wchar_t)nChar;
		}
	}

	// �o�b�t�@�֏o�́A��ʂ֏o��
	for (i=1 ; i<=nRepCnt ; i++) {
		CommTextOutW(&cv,&u16,1);
		if (ts.LocalEcho>0) {
			CommTextEchoW(&cv,&u16,1);
		}
	}

	// �X�N���[���ʒu�����Z�b�g
	if (WinOrgY != 0) {
		DispVScroll(SCROLL_BOTTOM, 0);
	}
}

LRESULT CVTWindow::OnUniChar(WPARAM wParam, LPARAM lParam)
{
	if (wParam == UNICODE_NOCHAR) {
		// ���̃��b�Z�[�W���T�|�[�g���Ă��邩�e�X�g�ł���悤�ɂ��邽��
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
	Notify2UnsetWindow((NotifyIcon *)cv.NotifyIcon);

	// �A�v���P�[�V�����I�����ɃA�C�R����j������ƁA�E�B���h�E��������O��
	// �^�C�g���o�[�̃A�C�R���� "Windows �̎��s�t�@�C���̃A�C�R��" �ɕς��
	// ���Ƃ�����̂Ŕj�����Ȃ�
	// TTSetIcon(m_hInst, m_hWnd, NULL, 0);

	DestroyWindow();
}

// �STera Term�̏I�����w������
void CVTWindow::OnAllClose()
{
	// �ˑR�I��������Ɗ댯�Ȃ̂ŁA���Ȃ炸���[�U�ɖ₢���킹���o���悤�ɂ���B
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

// �I���₢���킹�Ȃ���Tera Term���I������BOnAllClose()��M�p�B
LRESULT CVTWindow::OnNonConfirmClose(WPARAM wParam, LPARAM lParam)
{
	// ������ ts �̓��e���Ӑ}�I�ɏ��������Ă��A�I�����Ɏ����Z�[�u�����킯�ł͂Ȃ��̂ŁA���ɖ��Ȃ��B
	ts.PortFlag &= ~PF_CONFIRMDISCONN;
	OnClose();
	return 1;
}

void CVTWindow::OnDestroy()
{
	MakeOutputStringDestroy((OutputCharState *)(cv.StateEcho));
	cv.StateEcho = NULL;
	MakeOutputStringDestroy((OutputCharState *)(cv.StateSend));
	cv.StateSend = NULL;

	// remove this window from the window list
	UnregWin(HVTWin);

	// USB�f�o�C�X�ω��ʒm����
	if (hDevNotify != NULL && pUnregisterDeviceNotification != NULL) {
		pUnregisterDeviceNotification(hDevNotify);
		hDevNotify = NULL;
	}

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

	TTSetUnInit(&ts);

	Notify2Uninitialize((NotifyIcon *)cv.NotifyIcon);
	cv.NotifyIcon = NULL;

	DropUninit();
}

static void EscapeFilename(const wchar_t *src, wchar_t *dest)
{
#define ESCAPE_CHARS	L" ;&()$!`'[]{}#^~"
	const wchar_t *s = src;
	wchar_t *d = dest;
	while (*s) {
		wchar_t c = *s++;
		if (c == L'\\') {
			// �p�X�̋�؂�� \ -> / ��
			*d = '/';
		} else if (wcschr(ESCAPE_CHARS, c) != NULL) {
			// �G�X�P�[�v���K�v�ȕ���
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

typedef struct DropData_tag {
	// ini�ɕۑ�����Ȃ��A�����s���Ă���Tera Term�ł̂ݗL���Ȑݒ�
	enum drop_type DefaultDropType;
	unsigned char DefaultDropTypePaste;
	bool DefaultShowDialog;
	bool TransBin;
	bool DoSameProcess;
	enum drop_type DropType;
	unsigned char DropTypePaste;

	HWND vtwin;
	UINT_PTR PollingTimerID;

	int FileCount;
	int DirectoryCount;
	int SendIndex;
} DropData_t;

/**
 *	������
 */
void CVTWindow::DropInit()
{
	DropData_t *data =(DropData_t *)calloc(1, sizeof(*data));
	data->DefaultDropType = DROP_TYPE_CANCEL;
	data->DefaultDropTypePaste = DROP_TYPE_PASTE_ESCAPE;
	data->DefaultShowDialog = ts.ConfirmFileDragAndDrop ? true : false;
	data->vtwin = HVTWin;

	DropData = data;
}

void CVTWindow::DropUninit()
{
	free(DropData);
	DropData = NULL;
}

/**
 *	���M�����R�[���o�b�N
 */
static void DropSendCallback(void *callback_data)
{
	DropData_t *data = (DropData_t *)callback_data;
	// ���̑��M���s��
	::PostMessage(data->vtwin, WM_USER_DROPNOTIFY, 0, 1);
}

/**
 *	�^�C�}�[��SCP�̑��M�󋵂��`�F�b�N
 */
static void CALLBACK DropSendTimerProc(HWND, UINT, UINT_PTR nIDEvent, DWORD)
{
	DropData_t *data = (DropData_t *)nIDEvent;

	// ���M��?
	if (ScpGetStatus() == TRUE) {
		// ���̃^�C�}�[�C���^�[�o���ōēx�`�F�b�N
		return;
	}

	assert(data->PollingTimerID != 0);
	BOOL r = KillTimer(HVTWin, data->PollingTimerID);
	assert(r == 1); (void)r;
	data->PollingTimerID = 0;

	// ���̑��M���s��
	::PostMessage(data->vtwin, WM_USER_DROPNOTIFY, 0, 1);
}

/**
 *  �t�@�C�����h���b�v�ʒm
 *	@param	lparam		0	�t�@�C�����h���b�v���ꂽ
 *							ShowDialog���Q�Ƃ����
 *						1	�t�@�C���̓]������������
 *	@param	ShowDialog	0	�\�����Ă��\�����Ȃ��Ă��ǂ�
 *						1	�K���\������
 */
LRESULT CVTWindow::OnDropNotify(WPARAM ShowDialog, LPARAM lparam)
{
	DropData_t *data = DropData;

	switch (lparam) {
	case 0: {
		data->FileCount = 0;
		data->DirectoryCount = 0;
		for (int i = 0; i < DropListCount; i++) {
			const wchar_t *FileName = DropLists[i];
			const DWORD attr = GetFileAttributesW(FileName);
			if (attr == INVALID_FILE_ATTRIBUTES) {
				data->FileCount++;
			} else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
				data->DirectoryCount++;
			} else {
				data->FileCount++;
			}
		}

		data->DoSameProcess = false;
		const bool isSSH = (cv.isSSH == 2);
		data->DropTypePaste = DROP_TYPE_PASTE_ESCAPE;
		if (data->DefaultDropType == DROP_TYPE_CANCEL) {
			// default is not set
			data->TransBin = ts.TransBin == 0 ? false : true;
			if (!ShowDialog) {
				if (data->FileCount == 1 && data->DirectoryCount == 0) {
					if (ts.ConfirmFileDragAndDrop) {
						if (isSSH) {
							data->DropType = DROP_TYPE_SCP;
						} else {
							data->DropType = DROP_TYPE_SEND_FILE;
						}
						data->DoSameProcess = false;
					} else {
						data->DropType = DROP_TYPE_SEND_FILE;
						data->DoSameProcess = data->DefaultShowDialog ? false : true;
					}
				}
				else if (data->FileCount == 0 && data->DirectoryCount == 1) {
					data->DropType = DROP_TYPE_PASTE_FILENAME;
					data->DoSameProcess = data->DefaultShowDialog ? false : true;
				}
				else if (data->FileCount > 0 && data->DirectoryCount > 0) {
					data->DropType = DROP_TYPE_PASTE_FILENAME;
					data->DoSameProcess = false;
				}
				else if (data->FileCount > 0 && data->DirectoryCount == 0) {
					// filename only
					if (isSSH) {
						data->DropType = DROP_TYPE_SCP;
					} else {
						data->DropType = DROP_TYPE_SEND_FILE;
					}
					data->DoSameProcess = false;
				} else {
					// directory only
					data->DropType = DROP_TYPE_PASTE_FILENAME;
					data->DoSameProcess = ts.ConfirmFileDragAndDrop ? false : true;
				}
			} else {
				// show dialog
				if (data->DirectoryCount > 0) {
					data->DropType = DROP_TYPE_PASTE_FILENAME;
				} else {
					if (isSSH) {
						data->DropType = DROP_TYPE_SCP;
					} else {
						data->DropType = DROP_TYPE_SEND_FILE;
					}
				}
				data->DoSameProcess = false;
			}
		} else {
			if (data->DirectoryCount > 0 &&
				(data->DefaultDropType == DROP_TYPE_SEND_FILE ||
				 data->DefaultDropType == DROP_TYPE_SCP))
			{	// �f�t�H���g�̂܂܂ł͏����ł��Ȃ��g�ݍ��킹
				data->DropType = DROP_TYPE_PASTE_FILENAME;
				data->DropTypePaste = data->DefaultDropTypePaste;
				data->DoSameProcess = false;
			} else {
				data->DropType = data->DefaultDropType;
				data->DropTypePaste = data->DefaultDropTypePaste;
				data->DoSameProcess = (ShowDialog || data->DefaultShowDialog) ? false : true;
			}
		}

		data->SendIndex = 0;
	}
		// break; �Ȃ�
		// FALLTHROUGH
	case 1:
	next_file: {
		if (data->SendIndex == DropListCount) {
			// ���ׂđ��M����
			goto finish;
		}
		int i = data->SendIndex;
		data->SendIndex++;
		const wchar_t *FileName = DropLists[i];

		if (!data->DoSameProcess) {
			const bool isSSH = (cv.isSSH == 2);
			bool DoSameProcessNextDrop;
			bool DoNotShowDialog = !data->DefaultShowDialog;
			SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
						  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
			data->DropType =
				ShowDropDialogBox(m_hInst, HVTWin, FileName, data->DropType,
								  DropListCount - i,
								  (data->DirectoryCount == 0 && isSSH) ? true : false, data->DirectoryCount == 0 ? true : false,
								  &data->TransBin,
								  &ts,
								  &data->DropTypePaste, &data->DoSameProcess,
								  &DoSameProcessNextDrop,
								  &DoNotShowDialog);
			if (data->DropType == DROP_TYPE_CANCEL) {
				goto finish;
			}
			if (DoSameProcessNextDrop) {
				data->DefaultDropType = data->DropType;
			}
			if (!ts.ConfirmFileDragAndDrop) {
				data->DefaultShowDialog = !DoNotShowDialog;
			}
		}

		switch (data->DropType) {
		case DROP_TYPE_CANCEL:
		default:
			// cancel
			break;
		case DROP_TYPE_SEND_FILE: {
			if (!data->TransBin) {
				SendMemSendFile2(FileName, FALSE, SENDMEM_DELAYTYPE_NO_DELAY, 0, 0, DropSendCallback, data);
			}
			else {
				SendMemSendFile2(FileName, TRUE, SENDMEM_DELAYTYPE_NO_DELAY, 0, 0, DropSendCallback, data);
			}
			break;
		}
		case DROP_TYPE_PASTE_FILENAME:
		{
			const bool escape = (data->DropTypePaste & DROP_TYPE_PASTE_ESCAPE) ? true : false;

			data->DefaultDropTypePaste = data->DropTypePaste;

			TermSendStartBracket();

			wchar_t *str = GetPasteString(FileName, escape);
			TermPasteStringNoBracket(str, wcslen(str));
			free(str);
			if (DropListCount > 1 && i < DropListCount - 1) {
				if (data->DropTypePaste & DROP_TYPE_PASTE_NEWLINE) {
					TermPasteStringNoBracket(L"\x0d", 1);	// ���s(CR,0x0d)
				}
				else {
					TermPasteStringNoBracket(L" ", 1);		// space
				}
			}

			TermSendEndBracket();

			// ���̃t�@�C���̏�����
			goto next_file;
		}
		case DROP_TYPE_SCP:
		{
			// send by scp
			const wchar_t *FileName = DropLists[i];
			wchar_t *SendDirW = ToWcharA(ts.ScpSendDir);
			BOOL r = ScpSend(FileName, SendDirW);
			free(SendDirW);
			if (!r) {
				// ���M�G���[
				::MessageBoxA(HVTWin, "scp send error", "Tera Term: error", MB_OK | MB_ICONERROR);
				goto finish;
			}

			data->PollingTimerID = SetTimer(HVTWin, (UINT_PTR)data, 100, DropSendTimerProc);
			break;

		}
		}
		break;
	}
	case 2:
	default:
	finish:
		// �I������
		DropListFree();
		break;
	}

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
		// �X�N���[���ʒu�����Z�b�g
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

	if (BuffUrlDblClk(DblClkX, DblClkY)) { // �u���E�U�Ăяo���̏ꍇ�͉������Ȃ��B (2005.4.3 yutaka)
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
	// eat mouse event for text selection
	// when window is activated by L button
	if ((ts.SelOnActive==0) && (nHitTest==HTCLIENT) && (message==WM_LBUTTONDOWN)) {
		IgnoreRelease = TRUE;
		return MA_ACTIVATEANDEAT;
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
		if (BuffCheckMouseOnURL(point.x, point.y) && ts.EnableClickableUrl)
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
		// SelectOnlyByLButton == TRUE �̂Ƃ��́A���{�^���_�E�����̂ݑI������ (2007.11.21 maya)
		BuffChangeSelect(point.x, point.y,i);
	}
}

void CVTWindow::OnMove(int x, int y)
{
	// �E�B���h�E�ʒu��ۑ�
	// 		�� x,y �̓N���C�A���g�̈�̍���̍��W
	RECT R;
	::GetWindowRect(HVTWin,&R);
	ts.VTPos.x = R.left;
	ts.VTPos.y = R.top;

	DispSetWinPos();

	if (vtwin_work.monitor_DPI == 0) {
		// �E�B���h�E�����߂ĕ\�����ꂽ
		//	���j�^��DPI��ۑ����Ă���
		vtwin_work.monitor_DPI = GetMonitorDpiFromWindow(m_hWnd);
	}
}

// �}�E�X�z�C�[���̉�]
BOOL CVTWindow::OnMouseWheel(
	UINT nFlags,   // ���z�L�[
	short zDelta,  // ��]����
	POINTS pts     // �J�[�\���ʒu
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
			GetI18nStrWW("Tera Term", "TOOLTIP_TITLEBAR_OPACITY", L"Opacity %.1f %%", ts.UILanguageFileW, &uimsg);
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

	line = abs(zDelta) / WHEEL_DELTA; // ���C����
	if (line < 1) line = 1;

	// ��X�N���[��������̍s���ɕϊ����� (2008.4.6 yutaka)
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

	// �\������Ă��Ȃ��Ă�WM_PAINT����������P�[�X�΍�
	if (::IsWindowVisible(m_hWnd) == 0) {
		return;
	}

	BGSetupPrimary(FALSE);

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
	 *  �y�[�X�g����:
	 *  �Ets.PasteFlag & CPF_DISABLE_RBUTTON -> �E�{�^���ɂ��y�[�X�g����
	 *  �Ets.PasteFlag & CPF_CONFIRM_RBUTTON -> �\�����ꂽ���j���[����y�[�X�g���s���̂ŁA
	 *                                          �E�{�^���A�b�v�ɂ��y�[�X�g�͍s��Ȃ�
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
	// �E�B���h�E�������̍ŏ���WM_SIZE��(monitor_DPI==0�̂Ƃ�)��
	// DPI�̃`�F�b�N���s��Ȃ�
	// �E�B���h�E�������AWM_SIZE, WM_MOVE �ƃ��b�Z�[�W����������
	if (vtwin_work.monitor_DPI != 0 &&
		GetMonitorDpiFromWindow(m_hWnd) != vtwin_work.monitor_DPI) {
		// DPI�̈قȂ�f�B�X�v���C���܂����� WM_DPICHANGE ����������
		//
		// �u�h���b�O���ɃE�B���h�E�̓��e��\������=OFF�v�ݒ莞
		// �}�E�X�̃{�^���𗣂����Ƃ��ɁA����2��ނ̔����p�^�[��������
		// 1. �E�B���h�E���ړ������Ƃ�
		//    WM_MOVE, WM_SIZE, WM_DPICHANGED �̏��Ń��b�Z�[�W����������
		// 2. �E�B���h�E�����T�C�Y�����Ƃ�
		//    (WM_MOVE,)WM_SIZE, WM_DPICHANGED �̏��Ń��b�Z�[�W����������
		//
		// ���b�Z�[�W����́A�Z�������t�H���g�T�C�Y���A
		// �ǂ����ύX���ׂ������f�ł��Ȃ�
		// �����ł� 1 ���Ó��ƂȂ�悤��������
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
		AdjustSize = FALSE;
	}
#endif
}

// ���T�C�Y���̏����Ƃ��āA�ȉ��̓���s���B
// �E�c�[���`�b�v�ŐV�����[���T�C�Y��\������
// �E�t�H���g�T�C�Y�ƒ[���T�C�Y�ɍ��킹�āA�E�B���h�E�ʒu�E�T�C�Y�𒲐�����
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
		// TermIsWin=off �̎��̓��T�C�Y�ł͒[���T�C�Y���ς��Ȃ��̂�
		// ���݂̒[���T�C�Y������Ƃ���B
		if (w > ts.TerminalWidth)
			w = ts.TerminalWidth;
		if (h > ts.TerminalHeight)
			h = ts.TerminalHeight;
	}

	// �Œ�ł� 1x1 �̒[���T�C�Y��ۏႷ��B
	if (w <= 0)
		w = 1;
	if (h <= 0)
		h = 1;

	UpdateSizeTip(HVTWin, w, h, fwSide, pRect->left, pRect->top);

	fixed_width = w * FontWidth + margin_width;
	fixed_height = h * FontHeight + margin_height;

	switch (fwSide) { // ������
	case 1: // ��
	case 4: // ����
	case 7: // ����
		pRect->left = pRect->right - fixed_width;
		break;
	case 2: // �E
	case 5: // �E��
	case 8: // �E��
		pRect->right = pRect->left + fixed_width;
		break;
	}

	switch (fwSide) { // ��������
	case 3: // ��
	case 4: // ����
	case 5: // �E��
		pRect->top = pRect->bottom - fixed_height;
		break;
	case 6: // ��
	case 7: // ����
	case 8: // �E��
		pRect->bottom = pRect->top + fixed_height;
		break;
	}
}

void CVTWindow::OnSysChar(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
#ifdef WINDOW_MAXMIMUM_ENABLED
	// ALT + x����������� WM_SYSCHAR �����ł���B
	// ALT + Enter�ŃE�B���h�E�̍ő剻 (2005.4.24 yutaka)
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
	if ((nChar==VK_F10) || (MetaKey(ts.MetaKey) && ((MainMenu==NULL) || (nChar!=VK_MENU)))) {
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
		// �܂��ڑ����������Ă��Ȃ���΁A�\�P�b�g�������N���[�Y�B
		// CloseSocket()���Ăт������A��������͌ĂׂȂ��̂ŁA����Win32API���R�[������B
		if (!cv.Ready) {
			closesocket(cv.s);
			cv.s = INVALID_SOCKET;  /* �\�P�b�g�����̈��t����B(2010.8.6 yutaka) */
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

	// �X�N���[�������W�� 16bit ���� 32bit �֊g������ (2005.10.4 yutaka)
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
		// DBT_DEVTYP_PORT �𓊂��� DBT_DEVTYP_DEVICEINTERFACE ���������Ȃ��h���C�o�����邽��
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
				/* Tera Term ���ڑ� */
				if (CheckComPort(ts.ComPort) == 1) {
					/* �|�[�g���L�� */
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
				/* Tera Term �ڑ��� */
				if (CheckComPort(cv.ComPort) == 0) {
					/* �|�[�g������ */
					AutoDisconnectedPort = cv.ComPort;
					Disconnect(TRUE);
				}
			}
		}
		break;
	}
	return TRUE;
}

LRESULT CVTWindow::OnWindowPosChanging(WPARAM wParam, LPARAM lParam)
{
	if(ts.EtermLookfeel.BGEnable && ts.EtermLookfeel.BGNoCopyBits) {
		((WINDOWPOS*)lParam)->flags |= SWP_NOCOPYBITS;
	}

	return TTCFrameWnd::DefWindowProc(WM_WINDOWPOSCHANGING,wParam,lParam);
}

LRESULT CVTWindow::OnSettingChange(WPARAM wParam, LPARAM lParam)
{
	BGOnSettingChange();
	return TTCFrameWnd::DefWindowProc(WM_SETTINGCHANGE,wParam,lParam);
}

LRESULT CVTWindow::OnEnterSizeMove(WPARAM wParam, LPARAM lParam)
{
	EnableSizeTip(1);

	BGOnEnterSizeMove();
	return TTCFrameWnd::DefWindowProc(WM_ENTERSIZEMOVE,wParam,lParam);
}

LRESULT CVTWindow::OnExitSizeMove(WPARAM wParam, LPARAM lParam)
{
	BGOnExitSizeMove();

	EnableSizeTip(0);

	return TTCFrameWnd::DefWindowProc(WM_EXITSIZEMOVE,wParam,lParam);
}

LRESULT CVTWindow::OnIMEStartComposition(WPARAM wParam, LPARAM lParam)
{
	IMECompositionState = TRUE;

	// �ʒu��ʒm����
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
		// ���̓R���e�L�X�g�̊J��Ԃ��X�V�����(IME On/OFF)

		// IME��On/Off���擾����
		IMEstat = GetIMEOpenStatus(HVTWin);
		if (IMEstat != 0) {
			// IME On

			// ��Ԃ�\������IME�̂��߂Ɉʒu��ʒm����
			int CaretX = (CursorX-WinOrgX)*FontWidth;
			int CaretY = (CursorY-WinOrgY)*FontHeight;
			SetConversionWindow(HVTWin,CaretX,CaretY);

			if (ts.IMEInline > 0) {
				// �t�H���g��ݒ肷��
				ResetConversionLogFont(HVTWin);
			}
		}

		// �`��
		ChangeCaret();

		break;

	// ���E�B���h�E�̕\���󋵒ʒm
	// IME_OPENCANDIDATE / IMN_CLOSECANDIDATE �T�|�[�g��
	//
	//  IME								status
	//  --------------------------------+----------
	//  MS IME ���{��(Windows 10 1809)	suport
	//  Google ���{�����(2.24.3250.0)	not support
	//
	// WM_IME_STARTCOMPOSITION / WM_IME_ENDCOMPOSITION ��
	// �������͏�Ԃ��X�^�[�g���� / �I�������Ŕ�������B
	// IME_OPENCANDIDATE / IMN_CLOSECANDIDATE ��
	// ���E�B���h�E���\�����ꂽ / �����Ŕ�������B
	case IMN_OPENCANDIDATE: {
		// ���E�B���h�E���J�����Ƃ��Ă���

		// ��Ԃ�\������IME�̂��߂Ɉʒu��ʒm����
		// ���̏ꍇ������̂ŁA�ʒu���Đݒ肷��
		// - �����ϊ�����\��
		// - ���̕�������͂��邱�ƂŊm�菈�����s��
		// - �������͂Ɩ��ϊ��������͂���������
		int CaretX = (CursorX-WinOrgX)*FontWidth;
		int CaretY = (CursorY-WinOrgY)*FontHeight;
		SetConversionWindow(HVTWin,CaretX,CaretY);

		// �t�H���g��ݒ肷��
		ResetConversionLogFont(HVTWin);

		break;
	}

	case IMN_OPENSTATUSWINDOW:
		// �X�e�[�^�X�E�B���h�E���I�[�v��(���m�蕶����\��?)���悤�Ƃ��Ă���

		// IME�Ŗ��ϊ���ԂŁA�t�H���g�_�C�A���O���I�[�v�����ăN���[�Y�����
		// IME�ɐݒ肵�Ă����t�H���g���ʂ̂��̂ɕω����Ă���炵��
		// �����Ńt�H���g�̍Đݒ���s��

		// �t�H���g��ݒ肷��
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
	{  // 1��ڂ̌Ăяo�� �T�C�Y������Ԃ�
		if(IsWindowUnicode(hWnd) == FALSE) {
			// ANSI��
			char buf[512];			// �Q�ƕ�������󂯎��o�b�t�@
			size_t str_len_count;
			int cx;

			// �Q�ƕ�����擾�A1�s���o��
			{	// �J�[�\��������A�X�y�[�X�ȊO�����������Ƃ�����s���Ƃ���
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

			// IME�ɕԂ��\���̂��쐬����
			if (pReconvPtrSave != NULL) {
				free(pReconvPtrSave);
			}
			pReconvPtrSave = (RECONVERTSTRING *)CreateReconvStringStA(
				hWnd, buf, str_len_count, cx, &ReconvSizeSave);
		}
		else {
			// UNICODE��
			size_t str_len_count;
			int cx;
			wchar_t *strW;

			// �Q�ƕ�����擾�A1�s���o��
			{	// �J�[�\��������A�X�y�[�X�ȊO�����������Ƃ�����s���Ƃ���
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

			// IME�ɕԂ��\���̂��쐬����
			if (pReconvPtrSave != NULL) {
				free(pReconvPtrSave);
			}
			pReconvPtrSave = (RECONVERTSTRING *)CreateReconvStringStW(
				hWnd, strW, str_len_count, cx, &ReconvSizeSave);
			free(strW);
		}

		// 1��ڂ̓T�C�Y������Ԃ�
		result = ReconvSizeSave;
	}
	else {
		// 2��ڂ̌Ăяo�� �\���̂�n��
		if (pReconvPtrSave != NULL) {
			RECONVERTSTRING *pReconv = (RECONVERTSTRING*)lParam;
			result = 0;
			if (pReconv->dwSize >= ReconvSizeSave) {
				// 1��ڂ̃T�C�Y���m�ۂ���Ă��Ă���͂�
				memcpy(pReconv, pReconvPtrSave, ReconvSizeSave);
				result = ReconvSizeSave;
			}
			free(pReconvPtrSave);
			pReconvPtrSave = NULL;
			ReconvSizeSave = 0;
		} else {
			// 3���?
			result = 0;
		}
	}

	return result;
}

LRESULT CVTWindow::OnIMERequest(WPARAM wParam, LPARAM lParam)
{
	// "IME=off"�̏ꍇ�́A�������Ȃ��B
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

// TTXKanjiMenu �̂��߂ɁA���j���[���\������Ă��Ă�
// �ĕ`�悷��悤�ɂ����B (2007.7.14 maya)
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
			wchar_t *uimsg;
			WinMenu = CreatePopupMenu();
			GetI18nStrWW("Tera Term", "MENU_WINDOW", L"&Window", ts.UILanguageFileW, &uimsg);
			::InsertMenuW(MainMenu,ID_HELPMENU,
						  MF_STRING | MF_ENABLED | MF_POPUP | MF_BYPOSITION,
						  (UINT_PTR)WinMenu, uimsg);
			free(uimsg);
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
		wchar_t *uimsg;
		SysMenu = ::GetSystemMenu(HVTWin,FALSE);
		AppendMenuA(SysMenu, MF_SEPARATOR, 0, NULL);
		GetI18nStrWW("Tera Term", "MENU_SHOW_MENUBAR", L"Show menu &bar", ts.UILanguageFileW, &uimsg);
		AppendMenuW(SysMenu, MF_STRING, ID_SHOWMENUBAR, uimsg);
		free(uimsg);
	}
	return 0;
}

LRESULT CVTWindow::OnChangeTBar(WPARAM wParam, LPARAM lParam)
{
	BOOL TBar,WFrame; // TitleBar, WindowFrame
	DWORD Style,ExStyle;
	HMENU SysMenu;

	Style = (DWORD)::GetWindowLongPtr (HVTWin, GWL_STYLE);
	ExStyle = (DWORD)::GetWindowLongPtr (HVTWin, GWL_EXSTYLE);
	TBar = ((Style & WS_SYSMENU)!=0);
	WFrame = ((Style & WS_BORDER) != 0);
	if (TBar == (ts.HideTitle==0) && WFrame == (ts.HideTitle==0 || ts.EtermLookfeel.BGNoFrame==0)) {
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
		Style = (Style & ~(WS_SYSMENU | WS_CAPTION |
						   WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) | WS_BORDER | WS_POPUP;

		if(ts.EtermLookfeel.BGNoFrame) {
			Style   &= ~(WS_THICKFRAME | WS_BORDER);
			ExStyle &= ~WS_EX_CLIENTEDGE;
		}else{
			Style   |=  WS_THICKFRAME;
			ExStyle |=  WS_EX_CLIENTEDGE;
		}
	}
	else {
		Style = (Style & ~WS_POPUP) | WS_SYSMENU | WS_CAPTION |
		                 WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_BORDER;

		ExStyle |=  WS_EX_CLIENTEDGE;
	}
#endif

	AdjustSize = TRUE;
	::SetWindowLongPtr(HVTWin, GWL_STYLE, Style);
	::SetWindowLongPtr(HVTWin, GWL_EXSTYLE, ExStyle);
	::SetWindowPos(HVTWin, NULL, 0, 0, 0, 0,
	               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	::ShowWindow(HVTWin, SW_SHOW);

	if ((ts.HideTitle==0) && (MainMenu==NULL) &&
	    ((ts.MenuFlag & MF_NOSHOWMENU) == 0)) {
		wchar_t *uimsg;
		SysMenu = ::GetSystemMenu(HVTWin,FALSE);
		AppendMenuA(SysMenu, MF_SEPARATOR, 0, NULL);
		GetI18nStrWW("Tera Term", "MENU_SHOW_MENUBAR", L"Show menu &bar", ts.UILanguageFileW, &uimsg);
		AppendMenuW(SysMenu, MF_STRING, ID_SHOWMENUBAR, uimsg);
		free(uimsg);
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
	if (ts.LogAutoStart || ts.LogFNW != NULL) {
		if (ts.LogFNW != NULL) {
			// "/L"= �Ŏw�肳��Ă���Ƃ�(Auto start logging ��������Ȃ�)
			//   �w��t�@�C������W�J����
			wchar_t *LogFNW = FLogGetLogFilename(ts.LogFNW);
			free(ts.LogFNW);
			ts.LogFNW = LogFNW;
		}
		else {
			// Auto start logging �̂Ƃ�("/L"�Ŏw�肳��Ă��Ȃ��Ƃ�)
			//   �f�t�H���g�̃t�@�C����
			ts.LogFNW = FLogGetLogFilename(NULL);
		}
		WideCharToACP_t(ts.LogFNW, ts.LogFN, sizeof(ts.LogFN));
		BOOL r = FLogOpen(ts.LogFNW, LOG_UTF8, FALSE);
		if (r != TRUE && (ts.HideWindow != 1 && ts.Minimize != 1)) {
			static const TTMessageBoxInfoW mbinfo = {
				"Tera Term",
				"MSG_TT_FILE_OPEN_ERROR", L"Tera Term: File open error",
				"MSG_LOGFILE_WRITE_ERROR", L"Cannot write log file.\n%s",
				MB_OK | MB_ICONERROR};
			TTMessageBoxW(m_hWnd, &mbinfo, ts.UILanguageFileW, ts.LogFNW);
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
	// �����ڑ��������̂Ƃ����ڑ��_�C�A���O���o���悤�ɂ��� (2006.9.15 maya)
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
	OpenHelpCV(&cv, HH_HELP_CONTEXT, help_id);
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
	Notify2Event((NotifyIcon *)cv.NotifyIcon, wParam, lParam);

	if (lParam == NIN_BALLOONUSERCLICK) {
		::SetForegroundWindow(m_hWnd);
	}

	return 0;
}

/**
 *	�V�����ڑ�
 *
 *	TODO
 *		���̕ϐ����R�}���h���C�����̏���̈���ƂȂ��Ă���
 *		- hostname[HostNameMaxLength] (�z�X�g���A�R�}���h���C��)
 *		- command[MAXPATHLEN + HostNameMaxLength]
 */
void CVTWindow::OnFileNewConnection()
{
	if (Connecting) return;
	if (! LoadTTDLG()) {
		return;
	}

	wchar_t hostname[HostNameMaxLength];
	hostname[0] = 0;
	TGetHNRec GetHNRec; /* record for dialog box */
	GetHNRec.SetupFN = ts.SetupFName;
	GetHNRec.SetupFNW = ts.SetupFNameW;
	GetHNRec.PortType = ts.PortType;
	GetHNRec.Telnet = ts.Telnet;
	GetHNRec.TelPort = ts.TelPort;
	GetHNRec.TCPPort = ts.TCPPort;
	GetHNRec.ProtocolFamily = ts.ProtocolFamily;
	GetHNRec.ComPort = ts.ComPort;
	GetHNRec.MaxComPort = ts.MaxComPort;
	GetHNRec.HostName = hostname;

	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	BOOL r = (*GetHostName)(HVTWin,&GetHNRec);

	if (r) {
		if ((GetHNRec.PortType==IdTCPIP) && LoadTTSET()) {
			if (ts.HistoryList) {
				(*AddHostToList)(ts.SetupFNameW, hostname);
			}
			if (ts.JumpList) {
				char *HostNameA = ToCharW(hostname);
				add_session_to_jumplist(HostNameA, GetHNRec.SetupFN);
				free(HostNameA);
			}
			FreeTTSET();
		}

		static const wchar_t *ttermpro = L"ttermpro";
		if (! cv.Ready) {
			ts.PortType = GetHNRec.PortType;
			ts.Telnet = GetHNRec.Telnet;
			ts.TCPPort = GetHNRec.TCPPort;
			ts.ProtocolFamily = GetHNRec.ProtocolFamily;
			ts.ComPort = GetHNRec.ComPort;

			if ((GetHNRec.PortType==IdTCPIP) && LoadTTSET()) {
				wchar_t *command = NULL;
				awcscats(&command, ttermpro, L" ", hostname, NULL);
				(*ParseParam)(command, &ts, NULL);
				free(command);
				FreeTTSET();
			}
			SetKeyMap();
			BGLoadThemeFile(&ts);
			if (ts.MacroFNW != NULL) {
				RunMacroW(ts.MacroFNW,TRUE);
				free(ts.MacroFNW);
				ts.MacroFNW = NULL;
			}
			else {
				Connecting = TRUE;
				ChangeTitle();
				CommOpen(HVTWin,&ts,&cv);
			}
			ResetSetup();
		}
		else {
			wchar_t Command[MAXPATHLEN + HostNameMaxLength];

			if (GetHNRec.PortType==IdSerial) {
				_snwprintf_s(Command, _countof(Command), _TRUNCATE, L"%s /C=%d", ttermpro, GetHNRec.ComPort);
			}
			else {
				wcsncpy_s(Command, _countof(Command), ttermpro, _TRUNCATE);
				wcsncat_s(Command, _countof(Command), L" ", _TRUNCATE);
				if (GetHNRec.Telnet==0)
					wcsncat_s(Command,_countof(Command), L" /T=0",_TRUNCATE);
				else
					wcsncat_s(Command,_countof(Command), L" /T=1",_TRUNCATE);
				if (GetHNRec.TCPPort != 0) {
					wchar_t tcpport[16];
					_snwprintf_s(tcpport, _countof(tcpport), _TRUNCATE, L" /P=%d", GetHNRec.TCPPort);
					wcsncat_s(Command,_countof(Command),tcpport,_TRUNCATE);
				}
				/********************************/
				/* �����Ƀv���g�R������������ */
				/********************************/
				if (GetHNRec.ProtocolFamily == AF_INET) {
					wcsncat_s(Command,_countof(Command), L" /4",_TRUNCATE);
				} else if (GetHNRec.ProtocolFamily == AF_INET6) {
					wcsncat_s(Command,_countof(Command), L" /6",_TRUNCATE);
				}
				wchar_t *hostname_nocomment = DeleteCommentW(hostname);
				wcsncat_s(Command,_countof(Command), L" ", _TRUNCATE);
				wcsncat_s(Command,_countof(Command), hostname_nocomment, _TRUNCATE);
				free(hostname_nocomment);
			}
			TTXSetCommandLine(Command, _countof(Command), &GetHNRec); /* TTPLUG */
			TTWinExec(Command);
		}
	}
	else {/* canceled */
		if (! cv.Ready) {
			SetDdeComReady(0);
		}
	}
}


// ���łɊJ���Ă���Z�b�V�����̕��������
// (2004.12.6 yutaka)
void CVTWindow::OnDuplicateSession()
{
	wchar_t Command[1024];
	const char *exec = "ttermpro";
	char cygterm_cfg[MAX_PATH];
	FILE *fp;
	char buf[256], *head, *body;
	int cygterm_PORT_START = 20000;
	int cygterm_PORT_RANGE = 40;
	int is_cygwin_port = 0;

	// ���݂̐ݒ���e�����L�������փR�s�[���Ă���
	CopyTTSetToShmem(&ts);

	// cygterm.cfg ��ǂݍ���
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
	// Cygterm �̃|�[�g�͈͓����ǂ���
	if (ts.TCPPort >= cygterm_PORT_START &&
	    ts.TCPPort <= cygterm_PORT_START+cygterm_PORT_RANGE) {
		is_cygwin_port = 1;
	}

	if (is_cygwin_port && (strcmp(ts.HostName, "127.0.0.1") == 0 ||
	    strcmp(ts.HostName, "localhost") == 0)) {
		// localhost�ւ̐ڑ��Ń|�[�g��cygterm.cfg�͈͓̔��̎���cygwin�ڑ��Ƃ݂Ȃ��B
		OnCygwinConnection();
		return;
	} else if (cv.TelFlag) { // telnet
		_snwprintf_s(Command, _countof(Command), _TRUNCATE,
					 L"%hs %hs:%d /DUPLICATE /nossh",
					 exec, ts.HostName, ts.TCPPort);

	} else if (cv.isSSH) { // SSH
		// �����̏����� TTSSH ���ɂ�点��ׂ� (2004.12.7 yutaka)
		// TTSSH���ł̃I�v�V����������ǉ��B(2005.4.8 yutaka)
		_snwprintf_s(Command, _countof(Command), _TRUNCATE,
					 L"%hs %hs:%d /DUPLICATE",
					 exec, ts.HostName, ts.TCPPort);

		TTXSetCommandLine(Command, _countof(Command), NULL); /* TTPLUG */

	} else {
		// telnet/ssh/cygwin�ڑ��ȊO�ł͕������s��Ȃ��B
		return;
	}

	// �Z�b�V�����������s���ہA/K= ������Έ����p�����s���悤�ɂ���B
	// cf. http://sourceforge.jp/ticket/browse.php?group_id=1412&tid=24682
	// (2011.3.27 yutaka)
	if (ts.KeyCnfFNW != NULL) {
		wcsncat_s(Command, _countof(Command), L" /K=", _TRUNCATE);
		wcsncat_s(Command, _countof(Command), ts.KeyCnfFNW, _TRUNCATE);
	}

	DWORD e = TTWinExec(Command);
	if (e != NO_ERROR) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_EXEC_TT_ERROR", L"Can't execute Tera Term. (%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW, e);
	}
}

//
// Connect to local cygwin
//
void CVTWindow::OnCygwinConnection()
{
	wchar_t *CygwinDirectory = ToWcharA(ts.CygwinDirectory);
	DWORD e = CygwinConnect(CygwinDirectory, NULL);
	free(CygwinDirectory);
	switch(e) {
	case NO_ERROR:
		break;
	case ERROR_FILE_NOT_FOUND: {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_FIND_CYGTERM_DIR_ERROR", L"Can't find Cygwin directory.",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
		break;
	}
	case ERROR_NOT_ENOUGH_MEMORY: {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_CYGTERM_ENV_ALLOC_ERROR", L"Can't allocate memory for environment variable.",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
		break;
	}
	case ERROR_OPEN_FAILED:
	default: {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_EXEC_CYGTERM_ERROR", L"Can't execute Cygterm.",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
		break;
	}
	}
}

//
// TeraTerm Menu�̋N��
//
void CVTWindow::OnTTMenuLaunch()
{
	static const wchar_t *exename = L"ttpmenu.exe";

	DWORD e = TTCreateProcess(exename, NULL, NULL);
	if (e != NO_ERROR) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_EXEC_TTMENU_ERROR", L"Can't execute TeraTerm Menu. (%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW, e);
	}
}

void CVTWindow::OnFileLog()
{
	FLogDlgInfo_t info;
	info.filename = NULL;
	info.pts = &ts;
	info.pcv = &cv;
	BOOL r = FLogOpenDialog(hInst, m_hWnd, &info);
	if (r) {
		const wchar_t *filename = info.filename;
		if (!info.append) {
			// �t�@�C���폜
			DeleteFileW(filename);
		}
		r = FLogOpen(filename, info.code, info.bom);
		if (r != FALSE) {
			if (FLogIsOpendText()) {
				// ���݃o�b�t�@�ɂ���f�[�^�����ׂď����o���Ă���A
				// ���O�̎���J�n����B
				// (2013.9.29 yutaka)
				if (ts.LogAllBuffIncludedInFirst) {
					FLogOutputAllBuffer();
				}
			}
		}
		else {
			// ���O�ł��Ȃ�
			static const TTMessageBoxInfoW mbinfo = {
				"Tera Term",
				"MSG_TT_FILE_OPEN_ERROR", L"Tera Term: File open error",
				"MSG_LOGFILE_WRITE_ERROR", L"Cannot write log file.\n%s",
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
		// �I���ł��Ȃ��̂ŌĂ΂�Ȃ��͂�
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

// ���O�̉{��
void CVTWindow::OnViewLog()
{
	if(!FLogIsOpend()) {
		return;
	}

	const wchar_t *filename = FLogGetFilename();
	DWORD e = TTCreateProcess(ts.ViewlogEditorW, ts.ViewlogEditorArg, filename);
	if (e != NO_ERROR) {
		DWORD error = GetLastError();
		static const TTMessageBoxInfoW mbinfo = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_VIEW_LOGFILE_ERROR", L"Can't view logging file. (%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(m_hWnd, &mbinfo, ts.UILanguageFileW, error);
	}
}

// �B���Ă��郍�O�_�C�A���O��\������ (2008.2.3 maya)
void CVTWindow::OnShowLogDialog()
{
	FLogShowDlg();
}

// ���O�擾�𒆒f/�ĊJ����
void CVTWindow::OnPauseLog()
{
	FLogPause(FLogIsPause() ? FALSE : TRUE);
}

// ���O�擾���I������
void CVTWindow::OnStopLog()
{
	FLogClose();
}

static wchar_t *_get_lang_msg(const char *key, const wchar_t *def, const wchar_t *iniFile)
{
	wchar_t *uimsg;
	GetI18nStrWW("Tera Term", key, def, iniFile, &uimsg);
	return uimsg;
}

// ���O�̍Đ�
void CVTWindow::OnReplayLog()
{
	wchar_t *szFile;
	static const wchar_t *exec = L"ttermpro.exe";

	// �o�C�i�����[�h�ō̎悵�����O�t�@�C����I������
	wchar_t *filter = _get_lang_msg("FILEDLG_OPEN_LOGFILE_FILTER", L"all(*.*)\\0*.*\\0\\0", ts.UILanguageFileW);
	wchar_t *title = _get_lang_msg("FILEDLG_OPEN_LOGFILE_TITLE", L"Select replay log file with binary mode", ts.UILanguageFileW);
	TTOPENFILENAMEW ofn = {};
	ofn.hwndOwner = HVTWin;
	ofn.lpstrFilter = filter;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"log";
	ofn.lpstrTitle = title;
	BOOL r = TTGetOpenFileNameW(&ofn, &szFile);
	free(filter);
	free(title);
	if (r == FALSE)
		return;

	// "/R"�I�v�V�����t����Tera Term���N������i���O���Đ������j
	wchar_t *exe_dir = GetExeDirW(NULL);
	wchar_t *Command;
	aswprintf(&Command, L"\"%s\\%s\" /R=\"%s\"", exe_dir, exec, szFile);
	free(exe_dir);
	free(szFile);

	DWORD e = TTWinExec(Command);
	free(Command);
	if (e != NO_ERROR) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_EXEC_TT_ERROR", L"Can't execute Tera Term. (%d)",
			MB_OK | MB_ICONWARNING
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW, e);
	}
}

void CVTWindow::OnFileSend()
{
	// new file send
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_TAHOMA_FONT");
	sendfiledlgdata data;
	data.UILanguageFileW = ts.UILanguageFileW;
	wchar_t *filterW = ToWcharA(ts.FileSendFilter);
	data.filesend_filter = filterW;
	INT_PTR ok = sendfiledlg(m_hInst, m_hWnd, &data);
	free(filterW);
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
	OpenExternalSetup(CAddSettingPropSheetDlg::DefaultPage);
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

	// �X�N���[���ʒu�����Z�b�g
	if (WinOrgY != 0) {
		DispVScroll(SCROLL_BOTTOM, 0);
	}
}

void CVTWindow::OnEditPasteCR()
{
	// add confirm (2008.3.11 maya)
	CBStartPaste(HVTWin, TRUE, BracketedPasteMode());

	// �X�N���[���ʒu�����Z�b�g
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

void CVTWindow::OpenExternalSetup(CAddSettingPropSheetDlg::Page page)
{
	BOOL old_use_unicode_api = UnicodeDebugParam.UseUnicodeApi;
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_TAHOMA_FONT");
	CAddSettingPropSheetDlg CAddSetting(m_hInst, HVTWin);
	CAddSetting.SetTreeViewMode(ts.ExperimentalTreeProprtySheetEnable);
	CAddSetting.SetStartPage(page);
	INT_PTR ret = CAddSetting.DoModal();
	if (ret == IDOK) {
		ChangeWin();
		ChangeFont();
		if (old_use_unicode_api != UnicodeDebugParam.UseUnicodeApi) {
			BuffSetDispAPI(UnicodeDebugParam.UseUnicodeApi);
		}

		// �R�[�f�B���O�^�u�Őݒ肪�ω������Ƃ��R�[������K�v������
		SetupTerm();
		TelUpdateKeepAliveInterval();
	}
}

/**
 *	�N���X�O���炻�̑��̐ݒ���J�����߂ɒǉ�
 */
void OpenExternalSetupOutside(CAddSettingPropSheetDlgPage page)
{
	pVTWin->OpenExternalSetup((CAddSettingPropSheetDlg::Page)page);
}

// Additional settings dialog
//
// (2004.9.5 yutaka) new added
// (2005.2.22 yutaka) changed to Tab Control
// (2008.5.12 maya) changed to PropertySheet
void CVTWindow::OnExternalSetup()
{
	OpenExternalSetup(CAddSettingPropSheetDlg::DefaultPage);
}

void CVTWindow::OnSetupTerminal()
{
	BOOL Ok;

	HelpId = HlpSetupTerminal;
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	Ok = (*SetupTerminal)(HVTWin, &ts);
	if (Ok) {
		SetupTerm();
	}
}

/**
 *  �F���Z�b�g����A�e�[�}���g�p����Ă���Ƃ��̓Z�b�g���Ă悢���₢���킹��
 */
void CVTWindow::SetColor()
{
	BOOL set_color = TRUE;
	if (ThemeIsEnabled()) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_TT_NOTICE", L"Tera Term: Notice",
			NULL, L"Theme is used\nDo you want to set color?",
			MB_ICONQUESTION | MB_YESNO };
		int r = TTMessageBoxW(m_hWnd, &info, ts.UILanguageFileW);
		if (r == IDNO) {
			set_color = FALSE;
		}
	}

	// �F��ݒ肷��
	if (set_color) {
		DispResetColor(CS_ALL);
	}

}

void CVTWindow::OnSetupWindow()
{
	BOOL Ok;

	HelpId = HlpSetupWindow;
	ts.VTFlag = 1;
	ts.SampleFont = VTFont[0];

	if (! LoadTTDLG()) {
		return;
	}

	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	char *orgTitle = _strdup(ts.Title);
	Ok = (*SetupWin)(HVTWin, &ts);

	if (Ok) {
		SetColor();

		// �^�C�g�����ύX����Ă�����A�����[�g�^�C�g�����N���A����
		if ((ts.AcceptTitleChangeRequest == IdTitleChangeRequestOverwrite) &&
		    (strcmp(orgTitle, ts.Title) != 0)) {
			free(cv.TitleRemoteW);
			cv.TitleRemoteW = NULL;
		}

		ChangeFont();
		ChangeWin();
	}

	free(orgTitle);
}

void CVTWindow::OnSetupFont()
{
	OpenExternalSetup(CAddSettingPropSheetDlg::FontPage);
}

static UINT_PTR CALLBACK TFontHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_INITDIALOG) {
		wchar_t *uimsg;
		GetI18nStrWW("Tera Term",
					 "DLG_CHOOSEFONT_STC6",
					 L"\"Font style\" selection here won't affect actual font appearance.",
					 ts.UILanguageFileW, &uimsg);
		SetDlgItemTextW(Dialog, stc6, uimsg);
		free(uimsg);

		SetFocus(GetDlgItem(Dialog,cmb1));

		CenterWindow(Dialog, GetParent(Dialog));
	}
	return FALSE;
}

void CVTWindow::OnSetupKeyboard()
{
	BOOL Ok;

	HelpId = HlpSetupKeyboard;
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	Ok = (*SetupKeyboard)(HVTWin, &ts);

	if (Ok) {
//		ResetKeypadMode(TRUE);
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
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	Ok = (*SetupSerialPort)(HVTWin, &ts);

	if (Ok && ts.ComPort > 0) {
		/*
		 * TCP/IP�ɂ��ڑ����̏ꍇ�͐V�K�v���Z�X�Ƃ��ċN������B
		 * New connection����V���A���ڑ����铮��Ɗ�{�I�ɓ�������ƂȂ�B
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

			TTWinExecA(Command);
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
	OpenExternalSetup(CAddSettingPropSheetDlg::TcpIpPage);
}

void CVTWindow::OnSetupGeneral()
{
	HelpId = HlpSetupGeneral;
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	if ((*SetupGeneral)(HVTWin,&ts)) {
		ResetCharSet();
		ResetIME();
	}
}

void CVTWindow::OnSetupSave()
{
	PTTSet pts = &ts;
	const wchar_t *UILanguageFileW = pts->UILanguageFileW;

	wchar_t *filter = _get_lang_msg("FILEDLG_SETUP_FILTER",
									L"setup files (*.ini)\\0*.ini\\0\\0", UILanguageFileW);
	wchar_t *title = _get_lang_msg("FILEDLG_SAVE_SETUP_TITLE",
								   L"Tera Term: Save setup", UILanguageFileW);

	/* OPENFILENAME record */
	TTOPENFILENAMEW ofn = {};
	ofn.hwndOwner   = m_hWnd;
	ofn.lpstrFile   = pts->SetupFNameW;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.hInstance = hInst;
	ofn.lpstrDefExt = L"ini";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_SHOWHELP;
	ofn.lpstrTitle = title;

	HelpId = HlpSetupSave;
	wchar_t *NameW;
	BOOL Ok = TTGetSaveFileNameW(&ofn, &NameW);

	free(filter);
	free(title);

	if (! Ok) {
		// �L�����Z��
		return;
	}

	// �t�@�C���������ւ���
	wchar_t *PrevSetupFNW = _wcsdup(ts.SetupFNameW);	// �O�̃t�@�C�����o���Ă���
	free(pts->SetupFNameW);
	pts->SetupFNameW = _wcsdup(NameW);
	WideCharToACP_t(pts->SetupFNameW, pts->SetupFName, sizeof(pts->SetupFName));

	// �������݂ł��邩?
	const DWORD attr = GetFileAttributesW(ts.SetupFNameW);
	if ((attr & FILE_ATTRIBUTE_DIRECTORY ) == 0 && (attr & FILE_ATTRIBUTE_READONLY) != 0) {
		// �t�H���_�ł͂Ȃ��A�ǂݎ���p�������ꍇ
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

		if (wcscmp(PrevSetupFNW, ts.SetupFNameW) == 0) {
			// �����t�@�C���֏�������(�㏑��)
			if (ts.IniAutoBackup) {
				// �o�b�N�A�b�v���쐬
				CreateBakupFile(ts.SetupFNameW, NULL);
			}
		}
		else {
			// �قȂ�t�@�C���֏�������
			CopyFileW(PrevSetupFNW, ts.SetupFNameW, TRUE);
		}

		if (GetFileAttributesW(ts.SetupFNameW) == INVALID_FILE_ATTRIBUTES) {
			// �t�@�C�����Ȃ�
			// UTF16LE BOM������ini�t�@�C�����쐬
			FILE *fp;
			_wfopen_s(&fp, ts.SetupFNameW, L"wb");
			if (fp != NULL) {
				fwrite("\xff\xfe", 2, 1, fp);	// BOM
				fclose(fp);
			}
		}
		else {
			// �t�@�C�������݂���
			// ini�t�@�C���̕����R�[�h��ύX����
			ConvertIniFileCharCode(ts.SetupFNameW, NULL);
		}

		/* write current setup values to file */
		(*WriteIniFile)(ts.SetupFNameW, &ts);
		/* copy host list */
		(*CopyHostList)(PrevSetupFNW, ts.SetupFNameW);
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
	const wchar_t *UILanguageFileW = ts.UILanguageFileW;
	wchar_t *FNameFilter = _get_lang_msg("FILEDLG_SETUP_FILTER", L"setup files (*.ini)\\0*.ini\\0\\0", UILanguageFileW);

	TTOPENFILENAMEW ofn = {};
	ofn.hwndOwner   = m_hWnd;
	ofn.lpstrFile   = ts.SetupFNameW;
	ofn.lpstrFilter = FNameFilter;
	ofn.nFilterIndex = 1;
	ofn.hInstance = hInst;
	ofn.lpstrDefExt = L"ini";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHOWHELP;
	ofn.lpstrTitle = _get_lang_msg("FILEDLG_RESTORE_SETUP_TITLE", L"Tera Term: Restore setup", UILanguageFileW);

	HelpId = HlpSetupRestore;
	wchar_t *filename;
	BOOL Ok = TTGetOpenFileNameW(&ofn, &filename);

	free(FNameFilter);
	free((void *)ofn.lpstrTitle);

	if (Ok) {
		free(ts.SetupFNameW);
		ts.SetupFNameW = filename;
		WideCharToACP_t(ts.SetupFNameW, ts.SetupFName, sizeof(ts.SetupFName));

		RestoreSetup();
	}
}

//
// ���ݓǂݍ��܂�Ă��� teraterm.ini �t�@�C�����i�[����Ă���
// �t�H���_���G�N�X�v���[���ŊJ���B
//
void CVTWindow::OnOpenSetupDirectory()
{
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_TAHOMA_FONT");
	SetupDirectoryDialog(m_hInst, HVTWin, &cv);
}

void CVTWindow::OnSetupLoadKeyMap()
{
	const wchar_t *UILanguageFileW = ts.UILanguageFileW;
	wchar_t *FNameFilter =
		_get_lang_msg("FILEDLG_KEYBOARD_FILTER", L"keyboard setup files (*.cnf)\\0*.cnf\\0\\0", UILanguageFileW);

	TTOPENFILENAMEW ofn = {};
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFile   = ts.KeyCnfFNW;
	ofn.lpstrFilter = FNameFilter;
	ofn.nFilterIndex = 1;
	ofn.hInstance = hInst;
	ofn.lpstrDefExt = L"cnf";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHOWHELP;
	ofn.lpstrTitle = _get_lang_msg("FILEDLG_LOAD_KEYMAP_TITLE", L"Tera Term: Load key map", UILanguageFileW);

	HelpId = HlpSetupLoadKeyMap;
	wchar_t *NameW;
	BOOL Ok = TTGetOpenFileNameW(&ofn, &NameW);

	free(FNameFilter);
	free((void *)ofn.lpstrTitle);

	if (Ok) {
		free(ts.KeyCnfFNW);
		ts.KeyCnfFNW = NameW;

		// load key map
		SetKeyMap();
	}
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
	free(cv.TitleRemoteW);
	cv.TitleRemoteW = NULL;
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
				// SSH2�ڑ��̏ꍇ�A��p�̃u���[�N�M���𑗐M����B(2010.9.28 yutaka)
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

// WM_COPYDATA�̎�M
LRESULT CVTWindow::OnReceiveIpcMessage(WPARAM wParam, LPARAM lParam)
{
	if (!cv.Ready) {
		return 0;
	}

	if (!ts.AcceptBroadcast) { // 337: 2007/03/20
		return 0;
	}

	// �����M�f�[�^������ꍇ�͐�ɑ��M����
	// �f�[�^�ʂ������ꍇ�͑��M������Ȃ��\��������
	if (TalkStatus == IdTalkSendMem) {
		SendMemContinuously();	// TODO �K�v?
	}

	// ���M�\�ȏ�ԂłȂ���΃G���[
	if (TalkStatus != IdTalkKeyb) {
		return 0;
	}

	const COPYDATASTRUCT *cds = (COPYDATASTRUCT *)lParam;
	BroadCastReceive(cds);

	return 1; // ���M�ł����ꍇ��1��Ԃ�
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
	RunMacroW(NULL,FALSE);
}

void CVTWindow::OnShowMacroWindow()
{
	RunMacroW(NULL,FALSE);
}

void CVTWindow::OnWindowWindow()
{
	BOOL Close;

	HelpId = HlpWindowWindow;
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	(*WindowWindow)(HVTWin,&Close);
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
	OpenHelpCV(&cv, HH_DISPLAY_TOPIC, 0);
}

void CVTWindow::OnHelpAbout()
{
	if (! LoadTTDLG()) {
		return;
	}
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	(*AboutDialog)(HVTWin);
}

LRESULT CVTWindow::OnDpiChanged(WPARAM wp, LPARAM lp)
{
	const UINT NewDPI = LOWORD(wp);
	const RECT SuggestedWindowRect = *(RECT *)lp;

	// �V����DPI�ɍ��킹�ăt�H���g�𐶐��A
	// �N���C�A���g�̈�̃T�C�Y�����肷��
	ChangeFont();
	ScreenWidth = WinWidth * FontWidth;
	ScreenHeight = WinHeight * FontHeight;
	//AdjustScrollBar();

	// �X�N���[���T�C�Y(=Client Area�̃T�C�Y)����E�B���h�E�T�C�Y���Z�o
	int NewWindowWidth;
	int NewWindowHeight;
	if (pAdjustWindowRectExForDpi != NULL || pAdjustWindowRectEx != NULL) {
		const DWORD Style = (DWORD)::GetWindowLongPtr(m_hWnd, GWL_STYLE);
		const DWORD ExStyle = (DWORD)::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
		const BOOL bMenu = (ts.PopupMenu != 0) ? FALSE : TRUE;
		if (pGetSystemMetricsForDpi != NULL) {
			// �X�N���[���o�[���\������Ă���ꍇ�́A
			// �X�N���[���T�C�Y(�N���C�A���g�G���A�̃T�C�Y)�ɒǉ�����
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
		// WM_DPICHANGED���������Ȃ����̂͂��A�O�̈׎���
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

	// �V�����E�B���h�E�̈���
	RECT NewWindowRect[4];

	// �����̈�ɉE���
	NewWindowRect[0].top = SuggestedWindowRect.top;
	NewWindowRect[0].bottom = SuggestedWindowRect.top + NewWindowHeight;
	NewWindowRect[0].left = SuggestedWindowRect.right - NewWindowWidth;
	NewWindowRect[0].right = SuggestedWindowRect.right;

	// �����̈�ɍ����
	NewWindowRect[1].top = SuggestedWindowRect.top;
	NewWindowRect[1].bottom = SuggestedWindowRect.top + NewWindowHeight;
	NewWindowRect[1].left = SuggestedWindowRect.left;
	NewWindowRect[1].right = SuggestedWindowRect.left + NewWindowWidth;

	// �����ʒu�ɉE����
	NewWindowRect[2].top = SuggestedWindowRect.bottom - NewWindowHeight;
	NewWindowRect[2].bottom = SuggestedWindowRect.top;
	NewWindowRect[2].left = SuggestedWindowRect.right - NewWindowWidth;
	NewWindowRect[2].right = SuggestedWindowRect.right;

	// �����ʒu�ɍ�����
	NewWindowRect[3].top = SuggestedWindowRect.bottom - NewWindowHeight;
	NewWindowRect[3].bottom = SuggestedWindowRect.top;
	NewWindowRect[3].left = SuggestedWindowRect.left;
	NewWindowRect[3].right = SuggestedWindowRect.left + NewWindowWidth;

	// �m�F
	const RECT *NewRect = &NewWindowRect[0];
	for (size_t i = 0; i < _countof(NewWindowRect); i++) {
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

	::SetWindowPos(m_hWnd, NULL,
				   NewRect->left, NewRect->top,
				   NewWindowWidth, NewWindowHeight,
				   SWP_NOZORDER);
	vtwin_work.monitor_DPI = NewDPI;

	ChangeCaret();

	{
		HINSTANCE inst;
		WORD icon_id;
		inst = (ts.PluginVTIconInstance != NULL) ? ts.PluginVTIconInstance : m_hInst;
		icon_id = (ts.PluginVTIconID != 0) ? ts.PluginVTIconID
		                                   : (ts.VTIcon != IdIconDefault) ? ts.VTIcon
		                                                                  : IDI_VT;
		TTSetIcon(inst, m_hWnd, MAKEINTRESOURCEW(icon_id), NewDPI);
	}

	return TRUE;
}

LRESULT CVTWindow::Proc(UINT msg, WPARAM wp, LPARAM lp)
{
	static const UINT WM_TASKBER_CREATED = RegisterWindowMessage("TaskbarCreated");

	LRESULT retval = 0;
	if (msg == MsgDlgHelp) {
		// HELPMSGSTRING message ��
		//		wp = dialog handle
		//		lp = initialization structure
		OnDlgHelp(HelpId, 0);
		return 0;
	}
	else if (msg == WM_TASKBER_CREATED) {
		// �^�X�N�o�[���ċN������
		NotifyIcon *ni = (NotifyIcon *)cv.NotifyIcon;
		Notify2Hide(ni);
		Notify2SetWindow(ni, m_hWnd, WM_USER_NOTIFYICON, m_hInst, (ts.VTIcon != IdIconDefault) ? ts.VTIcon: IDI_VT);
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
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_COMPOSITION:
	case WM_INPUTLANGCHANGE:
	case WM_IME_NOTIFY:
	case WM_IME_REQUEST:
	{
		if (ts.UseIME > 0) {
			switch(msg) {
			case WM_IME_STARTCOMPOSITION:
				OnIMEStartComposition(wp, lp);
				break;
			case WM_IME_ENDCOMPOSITION:
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
			default:
				assert(FALSE);
				break;
			}
		}
		else {
			retval = DefWindowProc(msg, wp, lp);
		}
		break;
	}
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
		SetTimer(m_hWnd, IdPasteDelayTimer, 0, NULL);  // idle�����𓮍삳���邽��
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
		case ID_SETUP_ADDITIONALSETTINGS_CODING: {
			OpenExternalSetup(CAddSettingPropSheetDlg::CodingPage);
			break;
		}
		case ID_SETUP_TERMINAL: OnSetupTerminal(); break;
		case ID_SETUP_WINDOW: OnSetupWindow(); break;
		case ID_SETUP_FONT: OnSetupFont(); break;
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
			if(ShiftKey())
				retval = HTBOTTOMRIGHT;
			else
				retval = HTCAPTION;
			}
		}
		break;
	}
	case WM_HELP: {
		// ���̏ꍇ�ɔ�������
		//		- F1 �L�[����
		//		- MessageBox() �� MB_HELP �� HELP �{�^������
		// - F1�L�[�������AWM_HELP, WM_KEYDOWN, KEYUP �ƃ��b�Z�[�W����������
		// - WM_HELP �ŉ������������Ă���� WM_KEYDOWN ���Ȃ��Ȃ�悤��
		vtwin_work_t *w = &vtwin_work;
		if (w->help_id != 0) {
			// �w���v���Z�b�g����Ă���
			OpenHelpCV(&cv, HH_HELP_CONTEXT, w->help_id);
			break;
		}
		goto default_proc;
	}
	default:
	default_proc:
		retval = DefWindowProc(msg, wp, lp);
		break;
	}
	return retval;
}

/**
 *	WM_HELP ���b�Z�[�W����M�����Ƃ��\������w���v�̃w���vID��ݒ�/��������
 *
 *	@param data		0		�w���v����
 *					0�ȊO	�w���vID
 *
 * - MessageBox() �� uType �� MB_HELP ��ݒ莞�AHELP�{�^�����\�������
 * - VTWindows ��e�ɂ��� MessageBox() �� HELP�{�^���������ꂽ�Ƃ��A
 *	 ����API�Őݒ肵���w���vID�̃w���v���\�������
 * - MessageBox() ������Ƃ��� 0���Z�b�g���邱��
 */
void VtwinSetHelpId(DWORD help_id)
{
	vtwin_work_t *w = &vtwin_work;
	w->help_id = help_id;
}
