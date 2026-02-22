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
#include "recvfiledlg.h"
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
#include "externalsetup.h"
#include "tslib.h"
#include "../ttpset/ttset.h"
#include "commentdlg.h"
#include "ttdup.h"

#include <initguid.h>
#if _MSC_VER < 1600
// Visual Studio 2005,2008 のとき、2010より古いバージョンのとき
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, \
             0xC0, 0x4F, 0xB9, 0x51, 0xED);
#else
#include <usbiodef.h>	// GUID_DEVINTERFACE_USB_DEVICE
#endif

#include "win32helper.h"

#define VTClassName L"VTWin32"

// ウィンドウ最大化ボタンを有効にする (2005.1.15 yutaka)
#define WINDOW_MAXMIMUM_ENABLED 1

static BOOL TCPLocalEchoUsed = FALSE;
static BOOL TCPCRSendUsed = FALSE;

static BOOL IgnoreRelease = FALSE;

UnicodeDebugParam_t UnicodeDebugParam;
typedef struct {
	char dbcs_lead_byte;
	UINT monitor_DPI;			// ウィンドウが表示されているディスプレイのDPI
	DWORD help_id;				// WM_HELPメッセージ時、表示するヘルプID(0で表示しない)
} vtwin_work_t;
static vtwin_work_t vtwin_work;

extern "C" PrintFile *PrintFile_;

/////////////////////////////////////////////////////////////////////////////
// CVTWindow

// Tera Term起動時とURL文字列mouse over時に呼ばれる (2005.4.2 yutaka)
static void SetMouseCursor(const char *cursor)
{
	HCURSOR hc;
	LPCWSTR name = NULL;
	int i;

	for (i = 0 ; MouseCursor[i].name ; i++) {
		if (_stricmp(cursor, MouseCursor[i].name) == 0) {
			// ANSI版のリソースIDをUnicode版に変換
			LPCSTR nameA = MouseCursor[i].id;
			assert(IS_INTRESOURCE(nameA));
			name = (LPCWSTR)nameA;
			break;
		}
	}
	if (name == NULL) {
		return;
	}

	hc = (HCURSOR)LoadImageW(NULL, name, IMAGE_CURSOR,
							 0, 0, LR_DEFAULTSIZE | LR_SHARED);

	if (hc != NULL) {
		SetClassLongPtrW(HVTWin, GCLP_HCURSOR, (LONG_PTR)hc);
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

/**
 *	シリアルの再接続/切断を行う
 */
class SerialReconnect {

#define DEBUG_WM_DEVICECHANGE	0
#define RECONNECT_DELAY_NORMAL	500		// (ms)
#define RECONNECT_DELAY_ILLEGAL	2000	// (ms)
#define RECONNECT_RETRY			3
#define RECONNECT_INTERVAL		1000	// (ms)

public:
	void Init(CVTWindow *vtwin)
	{
		hDevNotify = NULL;
		state_ = NONE;
		AutoDisconnectedPort = -1;
		vtwin_ = vtwin;
		if (pRegisterDeviceNotificationA == NULL) {
			return;
		}

		/*
		 *	USBデバイス変化通知登録
		 *	WM_DEVICECHANGE で通知を受けることができる
		 */
		DEV_BROADCAST_DEVICEINTERFACE_A filter = {};
		filter.dbcc_size = sizeof(filter);
		filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		filter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;	// USBマウスなどにも反応
		//filter.dbcc_classguid = GUID_DEVINTERFACE_COMPORT;	// シリアルポートだけに反応

		hDevNotify = pRegisterDeviceNotificationA(vtwin->m_hWnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
	}

	void Exit()
	{
		if (hDevNotify != NULL && pUnregisterDeviceNotification != NULL) {
			pUnregisterDeviceNotification(hDevNotify);
			hDevNotify = NULL;
		}
	}

	/**
	 *	WM_DEVICECHANGE が発生したときコールする
	 */
	BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
	{
		PDEV_BROADCAST_HDR pDevHdr;
		int comport = 0;

		pDevHdr = (PDEV_BROADCAST_HDR)dwData;
		if (nEventType == DBT_DEVICEARRIVAL || nEventType == DBT_DEVICEREMOVECOMPLETE) {
			if (pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT) {
				// メッセージからポートを取得
				PDEV_BROADCAST_PORT_A pDevPortA = (PDEV_BROADCAST_PORT_A)pDevHdr;
				if (pDevPortA->dbcp_name[1] != 0) {
					// ANSI版
					(void)sscanf(&pDevPortA->dbcp_name[3], "%d", &comport);
				}
				else {
					PDEV_BROADCAST_PORT_W pDevPortW = (PDEV_BROADCAST_PORT_W)pDevHdr;
					(void)swscanf(&pDevPortW->dbcp_name[3], L"%d", &comport);
				}
			}
		}
#if DEBUG_WM_DEVICECHANGE
		OutputDebugPrintf(
			"nEventType=%s(0x%x)\n",
			nEventType == DBT_DEVICEARRIVAL ? "DBT_DEVICEARRIVAL" :
			nEventType == DBT_DEVICEREMOVECOMPLETE ? "DBT_DEVICEREMOVECOMPLETE" :
			nEventType == DBT_DEVNODES_CHANGED ? "DBT_DEVNODES_CHANGED" :
			"-",
			nEventType);
		if (nEventType == DBT_DEVICEARRIVAL || nEventType == DBT_DEVICEREMOVECOMPLETE) {
			OutputDebugPrintf(
				" devicetype=%s(%ld)\n",
				pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT ? "DBT_DEVTYP_PORT" :
				pDevHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE ? "DBT_DEVTYP_DEVICEINTERFACE" :
				"-",
				pDevHdr->dbch_devicetype);
			if (pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT) {
				PDEV_BROADCAST_PORT_A pDevPortA = (PDEV_BROADCAST_PORT_A)pDevHdr;
				if (pDevPortA->dbcp_name[1] != 0) {
					OutputDebugPrintf(" %s (%d)\n", pDevPortA->dbcp_name, comport);
				}
				else {
					PDEV_BROADCAST_PORT_W pDevPortW = (PDEV_BROADCAST_PORT_W)pDevHdr;
					OutputDebugPrintfW(L" %s (%d)\n", pDevPortW->dbcp_name, comport);
				}
			}
		}
#endif
		switch (nEventType) {
		case DBT_DEVICEARRIVAL:
			// デバイスまたはメディアの一部が挿入されて使用可能になった
#if DEBUG_WM_DEVICECHANGE
			OutputDebugPrintf(" PortType=%d AutoDisconnectedPort=%d state_=%d\n",
							  ts.PortType, AutoDisconnectedPort, state_);
#endif
			// - 正しくは次の順で2メッセージが送られてくるようだ
			// 		DBT_DEVTYP_DEVICEINTERFACE
			// 		DBT_DEVTYP_PORT
			// - DBT_DEVTYP_PORT を投げず DBT_DEVTYP_DEVICEINTERFACE しか投げないドライバがあるらしい
			// - DBT_DEVTYP_PORT しか投げてこない場合は Windows 2000 以前 or 古いドライバ?
			//   - DBT_DEVTYP_DEVICEINTERFACE は WINVER >= 0x040A(Windows2000?) のときだけ発生するらしい
			if ((pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT ||
				 pDevHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) &&
				ts.PortType == IdSerial &&
				ts.AutoComPortReconnect &&
				AutoDisconnectedPort == ts.ComPort &&
				(!cv.Open)) {
				switch(state_) {
				case NONE:
					break;
				case WAIT_DEVTYP_DEVICEINTERFACE: {
					Connecting = TRUE;	// ポートが分からない、とりあえず接続中にする
					UINT delay = ts.AutoComPortReconnectDelayIllegal;
					SetTimer(delay);
					if (pDevHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
						state_ = WAIT_DEVTYP_PORT;
					}
					else {
						// DEVTYP_DEVICEINTERFACE のはずが DBT_DEVTYP_PORT 等が発生した
						state_ = WAIT_TIMER;
					}
#if DEBUG_WM_DEVICECHANGE
					OutputDebugPrintf(" state %d timer illegal %d ms\n", state_, delay);
#endif
					break;
				}
				case WAIT_DEVTYP_PORT: {
					if (pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT) {
						if (comport == AutoDisconnectedPort) {
							// 接続する
							UINT delay = ts.AutoComPortReconnectDelayNormal;
							Connecting = TRUE;	// 接続中
							SetTimer(delay);
							state_ = WAIT_TIMER;
#if DEBUG_WM_DEVICECHANGE
							OutputDebugPrintf(" state %d timer normal %d ms\n", state_, delay);
#endif
						}
					}
					else {
						state_ = WAIT_TIMER;
					}
				}
				case WAIT_TIMER:
					// タイマーで接続する
					break;
				case RECONNECTING:
					// 再接続中
					break;
				}
			}
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			// デバイスまたはメディアが削除された
#if DEBUG_WM_DEVICECHANGE
			OutputDebugPrintf(" PortType=%d AutoDisconnectedPort=%d\n", ts.PortType, AutoDisconnectedPort);
#endif
			if ((pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT ||
				 pDevHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) &&
				ts.PortType == IdSerial &&
				ts.AutoComPortReconnect) {
				if (state_ == NONE) {
					// 切断された、再接続準備
					if (cv.Open) {
						BOOL disconnected = FALSE;
						if (pDevHdr->dbch_devicetype == DBT_DEVTYP_PORT) {
							if (comport == cv.ComPort) {
								SetAutoConnectPort(ts.ComPort);
								vtwin_->Disconnect(TRUE);
								disconnected = TRUE;
							}
						}
						if (disconnected == FALSE) {
							// DBT_DEVTYP_PORT が発生しない場合対応
							//   HHD Software Virtual Serial Port Tool 6.20.00.1466 では
							//		ポートをオープンしていると DBT_DEVTYP_PORT を投げてこないようだ
							//
							// 次の順でメッセージが来るため事実上 DBT_DEVTYP_PORT のコードは使用されない
							//  - DBT_DEVTYP_DEVICEINTERFACE
							//  - DBT_DEVTYP_PORT
							if (CheckComPort(cv.ComPort) == 0) {
								/* オープンしているポートが無効になった,クローズする */
								vtwin_->Disconnect(TRUE);
								SetAutoConnectPort(ts.ComPort);
							}
						}
					}
				}
				else {
					// 再接続中に抜かれた?
					state_ = NONE;
					if (timer_id_ != 0) {
						KillTimer(vtwin_->m_hWnd, timer_id_);
					}
					SetAutoConnectPort(ts.ComPort);
					Connecting = FALSE;		// 通常状態(接続中ではない)
					ChangeTitle();
				}
			}
			break;
		}
		return TRUE;
	}

	void SetAutoConnectPort(int port)
	{
#if DEBUG_WM_DEVICECHANGE
		OutputDebugPrintf("%s() %d\n", __FUNCTION__, state_);
#endif
		if (state_ != NONE) {
			return;
		}
		AutoDisconnectedPort = port;
		state_ = WAIT_DEVTYP_DEVICEINTERFACE;
		retry_left_ = ts.AutoComPortReconnectRetryCount;
		ChangeTitle();
	}

	void ResetAutoConnectPort()
	{
#if DEBUG_WM_DEVICECHANGE
		OutputDebugPrintf("%s() %d\n", __FUNCTION__, state_);
#endif
		AutoDisconnectedPort = -1;
	}

	BOOL IsReconnecting()
	{
		return state_ == RECONNECTING;
	}

private:
	void SetTimer(UINT uElapse)
	{
		if (timer_id_ != 0) {
			KillTimer(vtwin_->m_hWnd, timer_id_);
		}
		timer_id_ = ::SetTimer(vtwin_->m_hWnd, (UINT_PTR)this, uElapse, OpenSerialTimerEntry);
	}

	/**
	 *	シリアルをオープンする
	 */
	void OpenSerial()
	{
		state_ = RECONNECTING;
#if DEBUG_WM_DEVICECHANGE
		OutputDebugPrintf("%s() %d %d\n", __FUNCTION__, AutoDisconnectedPort, state_);
#endif
		if (timer_id_ != 0) {
			KillTimer(vtwin_->m_hWnd, timer_id_);
			timer_id_ = 0;
		}
		BOOL try_open = FALSE;
		if (CheckComPort(AutoDisconnectedPort) != 0) {
			// ポートが存在している、オープンを試みる
			int NoMsg_prev = cv.NoMsg;
			try_open = TRUE;
			if (retry_left_ != 0) {
				cv.NoMsg = 1;	// ポップアップを出さない
			}
			else {
				state_ = NONE;
			}
			CommOpen(vtwin_->m_hWnd, &ts, &cv);
			cv.NoMsg = NoMsg_prev;
#if DEBUG_WM_DEVICECHANGE
			OutputDebugPrintf("%s() return from CommOpen()\n", __FUNCTION__);
#endif
		}
		if (cv.Open == TRUE) {
			// シリアルオープン成功
#if DEBUG_WM_DEVICECHANGE
			OutputDebugPrintf("%s() Open success\n", __FUNCTION__);
#endif
			state_ = NONE;
			AutoDisconnectedPort = -1;
		}
		else if (retry_left_ ==0) {
			// シリアルオープン失敗
#if DEBUG_WM_DEVICECHANGE
			OutputDebugPrintf("%s() Open fail\n", __FUNCTION__);
#endif
			state_ = NONE;
			AutoDisconnectedPort = -1;
			if (try_open == FALSE) {
				Connecting = FALSE;
			}
		}
		else {
			// シリアルオープン失敗,リトライする
			UINT delay = ts.AutoComPortReconnectRetryInterval;
#if DEBUG_WM_DEVICECHANGE
			OutputDebugPrintf("%s() Open fail, retry left %d interval %dms\n", __FUNCTION__, retry_left_, delay);
#endif
			SetTimer(delay);
			retry_left_--;
		}

		ChangeTitle();
	}

	static void CALLBACK OpenSerialTimerEntry(HWND, UINT, UINT_PTR data, DWORD)
	{
		SerialReconnect *ptr = (SerialReconnect *)data;
#if DEBUG_WM_DEVICECHANGE
		OutputDebugPrintf("%s()\n", __FUNCTION__);
#endif
		ptr->OpenSerial();
	}

private:
	CVTWindow *vtwin_;
	UINT_PTR timer_id_;
	enum {
		NONE,
		WAIT_DEVTYP_DEVICEINTERFACE,
		WAIT_DEVTYP_PORT,
		WAIT_TIMER,
		RECONNECTING,
	} state_;
	HDEVNOTIFY hDevNotify;
	int AutoDisconnectedPort;
	int retry_left_;
};

static SerialReconnect *serail_reconnect;

/////////////////////////////////////////////////////////////////////////////
// CVTWindow constructor

CVTWindow::CVTWindow(HINSTANCE hInstance)
{
	WNDCLASSW wc;
	RECT rect;
	DWORD Style;
	BOOL isFirstInstance;
	m_hInst = hInstance;

	CommInit(&cv);
	cv.ts = &ts;
	isFirstInstance = StartTeraTerm(m_hInst, &ts);

	TTXInit(&ts, &cv); /* TTPLUG */

	MsgDlgHelp = RegisterWindowMessage(HELPMSGSTRING);

	ParseFOption(&ts);

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
		// 2つめ以降のプロセスにおいても、ディスクから TERATERM.INI を読む。(2004.11.4 yutaka)
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

	// DPI Aware (高DPI対応)
	if (pIsValidDpiAwarenessContext != NULL && pSetThreadDpiAwarenessContext != NULL) {
		wchar_t Temp[4];
		GetPrivateProfileStringW(L"Tera Term", L"DPIAware", L"on", Temp, _countof(Temp), ts.SetupFNameW);
		if (_wcsicmp(Temp, L"on") == 0) {
			if (pIsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == TRUE) {
				pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			}
		}
	}

	// duplicate sessionの指定があるなら、共有メモリからコピーする (2004.12.7 yutaka)
	if (ts.DuplicateSession == 1) {
		// 共有メモリの座標は複製元の現在のウィンドウ座標になる。
		// 上で読み込んだ TERATERM.INI の値を使いたいので、待避して戻す。
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
	ScrollLock = FALSE;  // 初期値は無効 (2006.11.14 yutaka)
	Alpha = 255;
#if UNICODE_DEBUG
	TipWinCodeDebug = NULL;
#endif
	vtwin_work.dbcs_lead_byte = 0;
	vtwin_work.monitor_DPI = 0;
	vtwin_work.help_id = 0;
	isSizing = FALSE;

	// UnicodeDebugParam
	{
#if _DEBUG
		UnicodeDebugParam.CodePopupEnable = TRUE;
#else
		UnicodeDebugParam.CodePopupEnable = FALSE;
#endif
		UnicodeDebugParam.CodePopupKey1 = VK_CONTROL;
		UnicodeDebugParam.CodePopupKey2 = VK_CONTROL;
	}

	/* Initialize scroll buffer */
	InitBuffer((IdVtDrawAPI)ts.VTDrawAPI);
	BuffSetDispCodePage(ts.VTDrawAnsiCodePage);

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
	cv.HWin = HVTWin;
	vt_src = InitDisp(HVTWin);
	BGLoadThemeFile(vt_src, &ts);

	// Windows 11 でウィンドウの角が丸くならないようにする
	if (ts.WindowCornerDontround && pDwmSetWindowAttribute != NULL) {
		DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_DONOTROUND;
		pDwmSetWindowAttribute(HVTWin, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
	}

	// register this window to the window list
	SerialNo = RegWin(HVTWin,NULL);

	SetMouseCursor(ts.MouseCursorName);

	if(ts.EtermLookfeel.BGNoFrame && ts.HideTitle > 0) {
		DWORD ExStyle = (DWORD)::GetWindowLongPtr(HVTWin,GWL_EXSTYLE);
		ExStyle &= ~WS_EX_CLIENTEDGE;
		::SetWindowLongPtr(HVTWin,GWL_EXSTYLE,ExStyle);
	}

	// USBデバイス変化通知登録
	serail_reconnect = new SerialReconnect();
	serail_reconnect->Init(this);

	// 通知領域初期化
	NotifyIcon *ni = Notify2Initialize();
	cv.NotifyIcon = ni;
	Notify2SetWindow(ni, m_hWnd, WM_USER_NOTIFYICON, m_hInst, (ts.VTIcon != IdIconDefault) ? ts.VTIcon: IDI_VT);
	Notify2SetSound(ni, ts.NotifySound);

	// VT ウィンドウのアイコン
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

	ChangeFont(vt_src, 0);

	ResetIME(vt_src);

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

	if (ts.nCmdShow == SW_HIDE || ts.HideWindow > 0) {
		if (strlen(TopicName)>0) {
			InitDDE();
			SendDDEReady();
		}
		FirstPaint = FALSE;
		Startup();
		return;
	}
	SetWindowAlpha(ts.AlphaBlendActive);
	int CmdShow = ts.nCmdShow;
	if (ts.Minimize > 0) {
		CmdShow = SW_SHOWMINIMIZED;
	}
	ShowWindow(CmdShow);
	ChangeCaret(vt_src);
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
	CaretOn(vt_src);

	// SelectOnlyByLButton が on で 中・右クリックしたときに
	// バッファが選択状態だったら、選択内容がクリップボードに
	// コピーされてしまう問題を修正 (2007.12.6 maya)
	if (!disableBuffEndSelect) {
		// 選択領域の文字を取得、クリップボードへセットする
		wchar_t *strW = BuffEndSelect();
		if (strW != NULL) {
			CBSetTextW(HVTWin, strW, 0);
			free(strW);
		}
	}

	if (Paste) {
		CBStartPaste(HVTWin, FALSE, BracketedPasteMode());

		// スクロール位置をリセット
		if (WinOrgY != 0) {
			DispVScroll(vt_src, SCROLL_BOTTOM, 0);
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
		{ ID_FILE_RECVFILE, "MENU_FILE_RECVFILE" },
		{ ID_FILE_REPLAYLOG, "MENU_FILE_REPLAYLOG" },
		{ ID_FILE_CHANGEDIR, "MENU_FILE_CHANGEDIR" },
		{ ID_FILE_PRINT2, "MENU_FILE_PRINT" },
		{ ID_FILE_DISCONNECT, "MENU_FILE_DISCONNECT" },
		{ ID_FILE_EXIT, "MENU_FILE_EXIT" },
		{ ID_FILE_EXITALL, "MENU_FILE_EXITALL" },
		{ ID_TRANSFER, "MENU_TRANS" },
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
			EnableMenuItem(FileMenu,ID_FILE_RECVFILE,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_TRANSFER,MF_BYPOSITION | MF_GRAYED); /* Transfer */
			EnableMenuItem(FileMenu,ID_FILE_DISCONNECT,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_DUPLICATESESSION,MF_BYCOMMAND | MF_GRAYED);
		}
		else {
			EnableMenuItem(FileMenu,ID_FILE_SENDFILE,MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_RECVFILE,MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_TRANSFER,MF_BYPOSITION | MF_ENABLED); /* Transfer */
			EnableMenuItem(FileMenu,ID_FILE_DISCONNECT,MF_BYCOMMAND | MF_ENABLED);
			if (ts.DisableMenuDuplicateSession || cv.PortType==IdSerial) {
				EnableMenuItem(FileMenu,ID_FILE_DUPLICATESESSION,MF_BYCOMMAND | MF_GRAYED);
			}
			else {
				EnableMenuItem(FileMenu,ID_FILE_DUPLICATESESSION,MF_BYCOMMAND | MF_ENABLED);
			}
		}

		// 新規メニューを追加 (2004.12.5 yutaka)
		EnableMenuItem(FileMenu,ID_FILE_CYGWINCONNECTION,MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(FileMenu,ID_FILE_TERATERMMENU,MF_BYCOMMAND | MF_ENABLED);

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
			EnableMenuItem(TransMenu,2,MF_BYPOSITION | MF_GRAYED);  /* YMODEM */
			EnableMenuItem(TransMenu,5,MF_BYPOSITION | MF_GRAYED);  /* Quick-VAN */
		}
		else {
			EnableMenuItem(TransMenu,1,MF_BYPOSITION | MF_ENABLED); /* XMODEM */
			EnableMenuItem(TransMenu,2,MF_BYPOSITION | MF_ENABLED); /* YMODEM */
			EnableMenuItem(TransMenu,5,MF_BYPOSITION | MF_ENABLED); /* Quick-VAN */
		}
		if ((cv.PortType==IdSerial) &&
		    (ts.DataBit==IdDataBit7)) {
			EnableMenuItem(TransMenu,3,MF_BYPOSITION | MF_GRAYED); /* ZMODEM */
			EnableMenuItem(TransMenu,4,MF_BYPOSITION | MF_GRAYED); /* B-Plus */
		}
		else {
			EnableMenuItem(TransMenu,3,MF_BYPOSITION | MF_ENABLED); /* ZMODEM */
			EnableMenuItem(TransMenu,4,MF_BYPOSITION | MF_ENABLED); /* B-Plus */
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
	ChangeFont(vt_src,0);
	BuffChangeWinSize(WinWidth,WinHeight);
	ChangeCaret(vt_src);

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
	// 起動時のみに読み込むテーマが無効になってしまうので削除
	BGInitialize(FALSE);
	BGSetupPrimary(TRUE);
#endif

	// 2006/03/17 by 337 : Alpha値も即時変更
	// Layered窓になっていない場合は効果が無い
	//
	// AlphaBlend を即時反映できるようにする。
	// (2016.12.24 yutaka)
	SetWindowAlpha(ts.AlphaBlendActive);

	/* setup window */
	ChangeWin();

	/* Language & IME */
	ResetIME(vt_src);

	/* change TEK window */
	if (pTEKWin != NULL)
		((CTEKWindow *)pTEKWin)->RestoreSetup();
}

void CVTWindow::RestoreSetup()
{
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
	/* OnCommOpen で開始されるのでここでは開始しない (2007.5.14 maya) */

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
	DispSetActive(vt_src, nState != WA_INACTIVE);
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
		const UINT acp = GetACP();
		if ((acp == 932) || (acp == 949) || (acp == 936) || (acp == 950)) {
			// CP932	日本語 shift jis
			// CP949	Korean
			// CP936	GB2312
			// CP950	Big5
			// CJK (2byte文字)
			if (vtwin_work.dbcs_lead_byte == 0 && IsDBCSLeadByte(nChar)) {
				// ANSI 2バイト文字の 1byte目だった
				//	通常は WM_IME_* メッセージで処理される
				//	次の場合ここに入ってくる
				//		TERATERM.INI で IME=off のとき
				//		imm32.dll がロードできなかったとき
				vtwin_work.dbcs_lead_byte = nChar;
				return;
			}
			else {
				// ANSI(ACP) -> UTF-32 -> UTF-16
				char mb_str[2];
				size_t mb_len;
				if (vtwin_work.dbcs_lead_byte == 0) {
					// 1バイト文字
					mb_str[0] = (char)nChar;
					mb_len = 1;
				}
				else {
					// 2バイト文字
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
			// CP1251	Russian,Cyrillic キリル
			UINT code_page = CP_ACP;
			if (ts.RussKeyb != /*IdWindows*/0) {
				code_page = 20866; /*koi8-r*/
			}
			char mb_str[2];
			size_t mb_len;
			mb_str[0] = (char)nChar;
			unsigned int u32;
			mb_len = MBCPToUTF32(mb_str, 1, code_page, &u32);
			if (mb_len == 0) {
				return;
			}
			u16 = (wchar_t)u32;
		}
		else {
			u16 = (wchar_t)nChar;
		}
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
		DispVScroll(vt_src, SCROLL_BOTTOM, 0);
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

	SaveVTPos(&ts);
	Notify2UnsetWindow((NotifyIcon *)cv.NotifyIcon);

	// アプリケーション終了時にアイコンを破棄すると、ウィンドウが消える前に
	// タイトルバーのアイコンが "Windows の実行ファイルのアイコン" に変わる
	// ことがあるので破棄しない
	// TTSetIcon(m_hInst, m_hWnd, NULL, 0);

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
	MakeOutputStringDestroy((OutputCharState *)(cv.StateEcho));
	cv.StateEcho = NULL;
	MakeOutputStringDestroy((OutputCharState *)(cv.StateSend));
	cv.StateSend = NULL;

	// remove this window from the window list
	UnregWin(HVTWin);

	// USBデバイス変化通知解除
	serail_reconnect->Exit();
	delete serail_reconnect;
	serail_reconnect = NULL;


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
	EndDisp(vt_src);
	vt_src = NULL;
	sendfiledlgUnInit();
	recvfiledlgUnInit();
	FLogOpenDialogUnInit();

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
	// iniに保存されない、今実行しているTera Termでのみ有効な設定
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
 *	初期化
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
 *	送信完了コールバック
 */
static void DropSendCallback(void *callback_data)
{
	DropData_t *data = (DropData_t *)callback_data;
	// 次の送信を行う
	::PostMessage(data->vtwin, WM_USER_DROPNOTIFY, 0, 1);
}

/**
 *	タイマーでSCPの送信状況をチェック
 */
static void CALLBACK DropSendTimerProc(HWND, UINT, UINT_PTR nIDEvent, DWORD)
{
	DropData_t *data = (DropData_t *)nIDEvent;

	// 送信中?
	if (ScpGetStatus() == TRUE) {
		// 次のタイマーインターバルで再度チェック
		return;
	}

	assert(data->PollingTimerID != 0);
	BOOL r = KillTimer(HVTWin, data->PollingTimerID);
	assert(r == 1); (void)r;
	data->PollingTimerID = 0;

	// 次の送信を行う
	::PostMessage(data->vtwin, WM_USER_DROPNOTIFY, 0, 1);
}

/**
 *  ファイルがドロップ通知
 *	@param	lparam		0	ファイルがドロップされた
 *							ShowDialogが参照される
 *						1	ファイルの転送が完了した
 *	@param	ShowDialog	0	表示しても表示しなくても良い
 *						1	必ず表示する
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
			{	// デフォルトのままでは処理できない組み合わせ
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
		// break; なし
		// FALLTHROUGH
	case 1:
	next_file: {
		if (data->SendIndex == DropListCount) {
			// すべて送信完了
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
			const BOOL binary = data->TransBin ? TRUE : FALSE;
			SendMemSendFile2(FileName, binary, (SendMemDelayType)ts.SendfileDelayType, ts.SendfileDelayTick,
							 ts.SendfileSize, ts.LocalEcho, DropSendCallback, data);
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
					TermPasteStringNoBracket(L"\x0d", 1);	// 改行(CR,0x0d)
				}
				else {
					TermPasteStringNoBracket(L" ", 1);		// space
				}
			}

			TermSendEndBracket();

			// 次のファイルの処理へ
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
				// 送信エラー
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
		// 終了処理
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
	DispHScroll(vt_src, Func, nPos);
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
			DispVScroll(vt_src, SCROLL_BOTTOM, 0);
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
		CaretKillFocus(vt_src, TRUE);
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
		// SelectOnlyByLButton == TRUE のときは、左ボタンダウン時のみ選択する (2007.11.21 maya)
		BuffChangeSelect(point.x, point.y,i);
	}
}

void CVTWindow::OnMove(int x, int y)
{
	// ウィンドウ位置を保存
	// 		注 x,y はクライアント領域の左上の座標
	RECT R;
	::GetWindowRect(HVTWin,&R);
	ts.VTPos.x = R.left;
	ts.VTPos.y = R.top;

	DispSetWinPos(vt_src);

	if (vtwin_work.monitor_DPI == 0) {
		// ウィンドウが初めて表示された
		//	モニタのDPIを保存しておく
		vtwin_work.monitor_DPI = GetMonitorDpiFromWindow(m_hWnd);
	}
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
		DispRestoreWinSize(vt_src);
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

	BGSetupPrimary(vt_src, FALSE);

	PaintDC = BeginPaint(&ps);

	ttdc_t *vt = PaintWindow(vt_src, PaintDC, ps.rcPaint, ps.fErase, &Xs, &Ys, &Xe, &Ye);
	LockBuffer();
	BuffUpdateRect(Xs,Ys,Xe,Ye);
	UnlockBuffer();
	DispEndPaint(vt);

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
	ChangeCaret(vt_src);
	FocusReport(TRUE);
}

void CVTWindow::OnSize(WPARAM nType, int cx, int cy)
{
	// ウィンドウ生成時の最初のWM_SIZE時(monitor_DPI==0のとき)は
	// DPIのチェックを行わない
	// ウィンドウ生成時、WM_SIZE, WM_MOVE とメッセージが発生する
	if (vtwin_work.monitor_DPI != 0 &&
		GetMonitorDpiFromWindow(m_hWnd) != vtwin_work.monitor_DPI &&
		isSizing == FALSE) {
		// DPIの異なるディスプレイをまたぐと WM_DPICHANGE が発生する
		//
		// 「ドラッグ中にウィンドウの内容を表示する=OFF」設定時
		// マウスのボタンを離したときに、次の2種類の発生パターンがある
		// 1. ウィンドウが移動したとき
		//    WM_MOVE, WM_SIZE, WM_DPICHANGED の順でメッセージが発生する
		// 2. ウィンドウがリサイズしたとき
		//    (WM_MOVE,)WM_SIZE, WM_DPICHANGED の順でメッセージが発生する
		//
		// WM_SIZEの処理のため、2 が妥当となるよう実装した
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
		ResizeWindow(vt_src, R.left, R.top, w, h, cx, cy);
	}
	else {
		int CellWidth, CellHeight;
		DispGetCellSize(vt_src, &CellWidth, &CellHeight);
#if 0
		if (ts.FontScaling) {
			int NewCellWidth, NewCellHeight;
			BOOL FontChanged = FALSE;

			NewCellWidth = cx / ts.TerminalWidth;
			NewCellHeight = cy / ts.TerminalHeight;

			if (NewCellWidth - ts.FontDW < 3) {
				NewCellWidth = ts.FontDW + 3;
			}
			if (NewCellWidth != CellWidth) {
				ts.VTFontSize.x = ts.FontDW - NewCellWidth;
				CellWidth = NewCellWidth;
				FontChanged = TRUE;
			}

			if (NewCellHeight - ts.FontDH < 3) {
				NewCellHeight = ts.FontDH + 3;
			}
			if (NewCellHeight != CellHeight) {
				ts.VTFontSize.y = ts.FontDH - NewCellHeight;
				CellHeight = NewCellHeight;
				FontChanged = TRUE;
			}

			w = ts.TerminalWidth;
			h = ts.TerminalHeight;

			if (FontChanged) {
				DispSetFontSize(vt_src, CellWidth, CellHeight);
				ChangeFont(vt_src, 0);
			}
		}
		else
#endif
		{
			w = cx / CellWidth;
			h = cy / CellHeight;
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

	int CellWidth, CellHeight;
	DispGetCellSize(vt_src, &CellWidth, &CellHeight);
	w = nWidth / CellWidth;
	h = nHeight / CellHeight;

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

	fixed_width = w * CellWidth + margin_width;
	fixed_height = h * CellHeight + margin_height;

	switch (fwSide) {		   // 幅調整
		case WMSZ_LEFT:		   // 左
		case WMSZ_TOPLEFT:	   // 左上
		case WMSZ_BOTTOMLEFT:  // 左下
			pRect->left = pRect->right - fixed_width;
			break;
		case WMSZ_RIGHT:		// 右
		case WMSZ_TOPRIGHT:		// 右上
		case WMSZ_BOTTOMRIGHT:	// 右下
			pRect->right = pRect->left + fixed_width;
			break;
	}

	switch (fwSide) {		 // 高さ調整
		case WMSZ_TOP:		 // 上
		case WMSZ_TOPLEFT:	 // 左上
		case WMSZ_TOPRIGHT:	 // 右上
			pRect->top = pRect->bottom - fixed_height;
			break;
		case WMSZ_BOTTOM:		// 下
		case WMSZ_BOTTOMLEFT:	// 左下
		case WMSZ_BOTTOMRIGHT:	// 右下
			pRect->bottom = pRect->top + fixed_height;
			break;
	}
	isSizing = TRUE;
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
		int ScreenWidth;
		int ScreenHeight;
		DispGetScreenSize(vt_src, &ScreenWidth, &ScreenHeight);
		GetCursorPos(&Point);
		::ScreenToClient(HVTWin,&Point);
		DispAutoScroll(vt_src, Point);
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

	DispVScroll(vt_src, Func, nPos);
}

BOOL CVTWindow::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	return serail_reconnect->OnDeviceChange(nEventType, dwData);
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
	BGOnSettingChange(vt_src);
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
	BGOnExitSizeMove(vt_src);

	EnableSizeTip(0);
	isSizing = FALSE;
	return TTCFrameWnd::DefWindowProc(WM_EXITSIZEMOVE,wParam,lParam);
}

// 変換位置を通知する
void CVTWindow::SetConversionWindowPos()
{
	int CellWidth, CellHeight;
	DispGetCellSize(vt_src, &CellWidth, &CellHeight);
	int CaretX = (CursorX - WinOrgX) * CellWidth + ts.FontDX;
	int CaretY = (CursorY - WinOrgY) * CellHeight + ts.FontDY;
	SetConversionWindow(m_hWnd, CaretX, CaretY);
}

LRESULT CVTWindow::OnIMEStartComposition(WPARAM wParam, LPARAM lParam)
{
	IMECompositionState = TRUE;
	SetConversionWindowPos();
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
	ChangeCaret(vt_src);

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
			SetConversionWindowPos();

			if (ts.IMEInline > 0) {
				// フォントを設定する
				ResetConversionLogFont(HVTWin);
			}
		}

		// 描画
		ChangeCaret(vt_src);

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
		SetConversionWindowPos();

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
			if (!serail_reconnect->IsReconnecting()) {
				Connecting = FALSE;
			}
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
	serail_reconnect->ResetAutoConnectPort();

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
			// "/L"= で指定されているとき(Auto start logging かもしれない)
			//   指定ファイル名を展開する
			wchar_t *LogFNW = FLogGetLogFilename(ts.LogFNW);
			free(ts.LogFNW);
			ts.LogFNW = LogFNW;
		}
		else {
			// Auto start logging のとき("/L"で指定されていないとき)
			//   デフォルトのファイル名
			ts.LogFNW = FLogGetLogFilename(NULL);
		}
		WideCharToACP_t(ts.LogFNW, ts.LogFN, sizeof(ts.LogFN));
		if (!FLogIsOpend()) {
			BOOL r = FLogOpen(ts.LogFNW, LOG_UTF8, FALSE);
			if (r != TRUE && (ts.HideWindow != 1 && ts.Minimize != 1)) {
				static const TTMessageBoxInfoW mbinfo = {
					"Tera Term",
					"MSG_TT_FILE_OPEN_ERROR", L"Tera Term: File open error",
					"MSG_LOGFILE_WRITE_ERROR", L"Cannot write log file.\n%s",
					MB_OK | MB_ICONERROR };
				TTMessageBoxW(m_hWnd, &mbinfo, ts.UILanguageFileW, ts.LogFNW);
			}
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
				serail_reconnect->SetAutoConnectPort(ts.ComPort);
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

LRESULT CVTWindow::OnIdleTimer(WPARAM wParam, LPARAM lParam)
{
	// TalkStatus が IdTalkSendMem の状態で連続して OnIdle() を呼びだすと
	// ファイル転送ダイアログと VTwindow の操作がままならなくなるため、
	// TalkStatus が IdTalkKeyb か IdTalkQuiet の場合のみ OnIdle() を呼びだす。
	if (cv.PortType == IdSerial && (TalkStatus == IdTalkKeyb || TalkStatus == IdTalkQuiet)) {
		int i = IdleLoopCount;
		while (OnIdle(0)) {
			if (--i <= 0) {
				break;
			}
		}
	}
	DeleteTimerQueueTimer(NULL, hIdleTimer, NULL);
	CreateTimerQueueTimer(&hIdleTimer, NULL, IdleTimerProc, 0, IdleTimerPeriod, 0, WT_EXECUTEDEFAULT);
	return 0;
}

/**
 *	新しい接続
 *
 *	TODO
 *		次の変数がコマンドライン長の上限の一因となっている
 *		- hostname[HostNameMaxLength] (ホスト名、コマンドライン)
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
				add_session_to_jumplist(hostname, GetHNRec.SetupFNW);
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
			BGLoadThemeFile(vt_src, &ts);
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
				/* ここにプロトコル処理を入れる */
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

static void CygtermPortRange(int *start, int *range)
{
	wchar_t *cygterm_cfg;
	FILE *fp;
	char buf[256], *head, *body;
	int cygterm_PORT_START = 20000;
	int cygterm_PORT_RANGE = 40;

	aswprintf(&cygterm_cfg, L"%s\\cygterm.cfg", ts.HomeDirW);
	_wfopen_s(&fp, cygterm_cfg, L"r");
	free(cygterm_cfg);
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

	*start = cygterm_PORT_START;
	*range = cygterm_PORT_RANGE;
}

static BOOL IsCygterm()
{
	int cygterm_PORT_START = 20000;
	int cygterm_PORT_RANGE = 40;
	int is_cygwin_port = 0;

	CygtermPortRange(&cygterm_PORT_START, &cygterm_PORT_RANGE);

	// Cygterm のポート範囲内かどうか
	if (ts.TCPPort < cygterm_PORT_START ||
	    ts.TCPPort > cygterm_PORT_START+cygterm_PORT_RANGE) {
		return 0;
	}

	if ((strcmp(ts.HostName, "127.0.0.1") == 0 ||
		 strcmp(ts.HostName, "localhost") == 0)) {
		// localhostへの接続でポートがcygterm.cfgの範囲内の時はcygwin接続とみなす。
		return 1;
	}

	return 0;
}

// すでに開いているセッションの複製を作る
void CVTWindow::OnDuplicateSession()
{
	// 現在の設定内容を共有メモリへコピーしておく
	CopyTTSetToShmem(&ts);

	if (IsCygterm()) {
		// cygwin接続
		OnCygwinConnection();
		return;
	}

	if (!cv.TelFlag && !cv.isSSH) {
		// telnet/ssh/cygwin接続以外では複製を行わない。
		return;
	}

	const char *exec = "ttermpro";	// 仮実行ファイル名
	wchar_t Command[1024];
	Command[0] = 0;

	if (cv.TelFlag) {
		// telnet
		_snwprintf_s(Command, _countof(Command), _TRUNCATE,
					 L"%hs /DUPLICATE /nossh", exec);

	} else if (cv.isSSH) {
		// SSH
		_snwprintf_s(Command, _countof(Command), _TRUNCATE,
					 L"%hs /DUPLICATE", exec);

		// telnt以外の時は、プラグインにオプション生成してもらう
		// プラグインからコマンドラインを返す
		TTXSetCommandLine(Command, _countof(Command), NULL); /* TTPLUG */
	} else {
		// 来ないはず
		assert(FALSE);
	}

	if (ts.KeyCnfFNW != NULL) {
		wcsncat_s(Command, _countof(Command), L" /K=", _TRUNCATE);
		wcsncat_s(Command, _countof(Command), ts.KeyCnfFNW, _TRUNCATE);
	}

	if (ParseFOption(&ts)) {
		wcsncat_s(Command, _countof(Command), L" /F=", _TRUNCATE);
		wcsncat_s(Command, _countof(Command), ts.SetupFNameW, _TRUNCATE);
	}

	wchar_t *hostnameW = ToWcharA(ts.HostName);
	const wchar_t *commandline = wcschr(Command, L' ') + 1;	// 実行ファイル名以降
	TTDupInfo info = {};
	info.szHostName = hostnameW;
	info.port = ts.TCPPort;
	info.szOption = commandline;
	info.mode = TTDUP_COMMANDLINE;
	DWORD e = ConnectHost(m_hInst, m_hWnd, &info);
	free(hostnameW);
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
// TeraTerm Menuの起動
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
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	FLogDlgInfo_t info;
	info.filename = NULL;
	info.pts = &ts;
	info.pcv = &cv;
	BOOL r = FLogOpenDialog(hInst, m_hWnd, &info);
	if (r) {
		const wchar_t *filename = info.filename;
		if (!info.append) {
			// ファイル削除
			DeleteFileW(filename);
		}
		r = FLogOpen(filename, info.code, info.bom);
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
	if (!FLogIsOpend()) {
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

	wchar_t *comment;
	INT_PTR r = CommentDlg(m_hInst, HVTWin, &ts, &comment);
	if (r == IDOK && comment != NULL && comment[0] != 0) {
		FLogWriteStr(comment);
		FLogWriteStr(L"\n");		// TODO 改行コード
		free(comment);
	}
}

// ログの閲覧
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

static wchar_t *_get_lang_msg(const char *key, const wchar_t *def, const wchar_t *iniFile)
{
	wchar_t *uimsg;
	GetI18nStrWW("Tera Term", key, def, iniFile, &uimsg);
	return uimsg;
}

// ログの再生
void CVTWindow::OnReplayLog()
{
	wchar_t *szFile;
	static const wchar_t *exec = L"ttermpro.exe";

	// バイナリモードで採取したログファイルを選択する
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

	// "/R"オプション付きでTera Termを起動する（ログが再生される）
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
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_TAHOMA_FONT");
	sendfiledlgdata data = {};
	data.UILanguageFileW = ts.UILanguageFileW;
	wchar_t *filterW = ToWcharA(ts.FileSendFilter);
	data.filesend_filter = filterW;
	wchar_t *initial_dir = GetFileDir(&ts);
	data.initial_dir = initial_dir;
	data.skip_dialog = (BOOL)ts.SendfileSkipOptionDialog;
	data.binary = ts.TransBin;
	data.delay_type = (SendMemDelayType)ts.SendfileDelayType;
	data.delay_tick = ts.SendfileDelayTick;
	data.send_size = ts.SendfileSize;
	data.sequential_read = ts.SendfileSequential;

	INT_PTR ok = sendfiledlg(m_hInst, m_hWnd, &data);
	free(initial_dir);
	free(filterW);
	if (ok != IDOK) {
		return;
	}
	ts.SendfileSkipOptionDialog = data.skip_dialog;
	ts.TransBin = data.binary;
	ts.SendfileDelayType = data.delay_type;
	ts.SendfileDelayTick = data.delay_tick;
	ts.SendfileSize = data.send_size;
	ts.SendfileSequential = data.sequential_read;

	wchar_t *filename = data.filename;
	if (!data.sequential_read) {
		// new file send
		SendMemSendFile(filename, data.binary, data.delay_type, data.delay_tick, data.send_size, ts.LocalEcho);
	}
	else {
		// file send same as teraterm 4
		HelpId = HlpFileSend;
		FileSendStart(filename, data.binary);
	}
	free(filename);
}

void CVTWindow::OnFileRecv()
{
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_TAHOMA_FONT");
	recvfiledlgdata data = {};
	data.UILanguageFileW = ts.UILanguageFileW;
	wchar_t *filterW = ToWcharA(ts.FileReceiveFilter);
	data.filerecv_filter = filterW;
	wchar_t *initial_dir = GetFileDir(&ts);
	data.initial_dir = initial_dir;
	data.skip_dialog = (BOOL)ts.ReceivefileSkipOptionDialog;
	data.binary = TRUE;											// バイナリモード固定
	data.autostop_sec = ts.ReceivefileAutoStopWaitTime;

	INT_PTR ok = recvfiledlg(m_hInst, m_hWnd, &data);
	free(initial_dir);
	free(filterW);
	if (ok != IDOK) {
		return;
	}
	ts.ReceivefileSkipOptionDialog = data.skip_dialog;
	ts.TransBin = data.binary;
	ts.ReceivefileAutoStopWaitTime = data.autostop_sec;

	wchar_t *filename = data.filename;
	RawStartReceive(filename, ts.ReceivefileAutoStopWaitTime, FALSE);
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
	OpenSetupGeneral(m_hWnd);
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
		DispVScroll(vt_src, SCROLL_BOTTOM, 0);
	}
}

void CVTWindow::OnEditPasteCR()
{
	// add confirm (2008.3.11 maya)
	CBStartPaste(HVTWin, TRUE, BracketedPasteMode());

	// スクロール位置をリセット
	if (WinOrgY != 0) {
		DispVScroll(vt_src, SCROLL_BOTTOM, 0);
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

/**
 * Additional settings dialog
 */
void CVTWindow::OnExternalSetup()
{
	OpenExternalSetup(m_hWnd);
}

void CVTWindow::OnSetupTerminal()
{
	OpenSetupTerminal(m_hWnd);
}


/**
 *  色をセットする、テーマが使用されているときはセットしてよいか問い合わせる
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

	// 色を設定する
	if (set_color) {
		DispResetColor(vt_src, CS_ALL);
	}
}

void CVTWindow::OnSetupWindow()
{
	OpenSetupWin(m_hWnd);
}

void CVTWindow::OnSetupFont()
{
	OpenSetupFont(m_hWnd);
}

void CVTWindow::OnSetupKeyboard()
{
	OpenSetupKeyboard(m_hWnd);
}

/*
 *	シリアル接続で新しいプロセスを起動
 *	 New connectionからシリアル接続する動作と基本的に同じ動作
 */
#if 0
static void OpenNewComport(const TTTSet *pts)
{
	char Command[MAXPATHLEN + HostNameMaxLength];
	char Temp[MAX_PATH], Str[MAX_PATH];

	_snprintf_s(Command, sizeof(Command),
				"ttermpro /C=%u /SPEED=%lu /CDELAYPERCHAR=%u /CDELAYPERLINE=%u ",
				pts->ComPort, pts->Baud, pts->DelayPerChar, pts->DelayPerLine);

	if (SerialPortConfconvertId2Str(COM_DATABIT, pts->DataBit, Temp, sizeof(Temp))) {
		_snprintf_s(Str, sizeof(Str), _TRUNCATE, "/CDATABIT=%s ", Temp);
		strncat_s(Command, sizeof(Command), Str, _TRUNCATE);
	}
	if (SerialPortConfconvertId2Str(COM_PARITY, pts->Parity, Temp, sizeof(Temp))) {
		_snprintf_s(Str, sizeof(Str), _TRUNCATE, "/CPARITY=%s ", Temp);
		strncat_s(Command, sizeof(Command), Str, _TRUNCATE);
	}
	if (SerialPortConfconvertId2Str(COM_STOPBIT, pts->StopBit, Temp, sizeof(Temp))) {
		_snprintf_s(Str, sizeof(Str), _TRUNCATE, "/CSTOPBIT=%s ", Temp);
		strncat_s(Command, sizeof(Command), Str, _TRUNCATE);
	}
	if (SerialPortConfconvertId2Str(COM_FLOWCTRL, pts->Flow, Temp, sizeof(Temp))) {
		_snprintf_s(Str, sizeof(Str), _TRUNCATE, "/CFLOWCTRL=%s ", Temp);
		strncat_s(Command, sizeof(Command), Str, _TRUNCATE);
	}

	TTWinExecA(Command);
}
#endif

void CVTWindow::OnSetupSerialPort()
{
	OpenSetupSerialPort(m_hWnd);
}

void CVTWindow::OnSetupTCPIP()
{
	OpenSetupTCPIP(m_hWnd);
}

void CVTWindow::OnSetupGeneral()
{
	OpenSetupGeneral(m_hWnd);
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
		// キャンセル
		return;
	}

	// ファイル名を入れ替える
	wchar_t *PrevSetupFNW = _wcsdup(ts.SetupFNameW);	// 前のファイルを覚えておく
	free(pts->SetupFNameW);
	pts->SetupFNameW = _wcsdup(NameW);
	WideCharToACP_t(pts->SetupFNameW, pts->SetupFName, sizeof(pts->SetupFName));

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
		int w = 0, h = 0;

#ifdef WINDOW_MAXMIMUM_ENABLED
		if (::IsZoomed(m_hWnd)) {
			w = ts.TerminalWidth;
			h = ts.TerminalHeight;
			ts.TerminalWidth = ts.TerminalOldWidth;
			ts.TerminalHeight = ts.TerminalOldHeight;
		}
#endif

		if (wcscmp(PrevSetupFNW, ts.SetupFNameW) == 0) {
			// 同名ファイルへ書き込み(上書き)
			if (ts.IniAutoBackup) {
				// バックアップを作成
				CreateBakupFile(ts.SetupFNameW, NULL);
			}
		}
		else {
			// 異なるファイルへ書き込み
			CopyFileW(PrevSetupFNW, ts.SetupFNameW, TRUE);
		}

		if (GetFileAttributesW(ts.SetupFNameW) == INVALID_FILE_ATTRIBUTES) {
			// ファイルがない
			// UTF16LE BOMだけのiniファイルを作成
			FILE *fp;
			_wfopen_s(&fp, ts.SetupFNameW, L"wb");
			if (fp != NULL) {
				fwrite("\xff\xfe", 2, 1, fp);	// BOM
				fclose(fp);
			}
		}
		else {
			// ファイルが存在する
			// iniファイルの文字コードを変更する
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
// 現在読み込まれている teraterm.ini ファイルが格納されている
// フォルダをエクスプローラで開く。
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
	DispScrollHomePos(vt_src);
	ResetTerminal();
	DispResetColor(vt_src, CS_ALL);
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
	if (cv.Ready && (cv.PortType==IdTCPIP) && cv.TelFlag) {
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

LRESULT CVTWindow::OnDpiChanged(WPARAM wp, LPARAM lp, BOOL calcOnly)
{
	// BOOL calcOnly
	//   TRUE  = WM_GETDPISCALEDSIZE
	//   FALSE = WM_DPICHANGED

	const UINT NewDPI = LOWORD(wp);
	const RECT SuggestedWindowRect = *(RECT *)lp;
	SIZE *sz = (SIZE *)lp;
	RECT NewWindowRect[4]; // 新しいウィンドウ領域候補
	const RECT *NewRect;
	int NewWindowWidth;
	int NewWindowHeight;

	if (isSizing && (calcOnly == FALSE)) {
		NewRect = &SuggestedWindowRect;
		NewWindowWidth  = NewRect->right  - NewRect->left;
		NewWindowHeight = NewRect->bottom - NewRect->top;
	} else {
		int tmpScreenWidth;
		int tmpScreenHeight;

		if (calcOnly) {
			// 新DPIのフォントのサイズからスクリーンサイズを算出
			LOGFONTW VTlfDefault;
			TSGetLogFont(m_hWnd, &ts, 0, NewDPI, &VTlfDefault); // Normal Font
			HFONT VTFontDefault = CreateFontIndirectW(&VTlfDefault);
			HDC TmpDC = GetDC(m_hWnd);
			SelectObject(TmpDC, VTFontDefault);
			TEXTMETRIC Metrics;
			GetTextMetrics(TmpDC, &Metrics);
			int tmpFontWidth = Metrics.tmAveCharWidth + ts.FontDW;
			int tmpFontHeight = Metrics.tmHeight + ts.FontDH;
			DeleteObject(VTFontDefault);
			ReleaseDC(m_hWnd, TmpDC);

			tmpScreenWidth = WinWidth * tmpFontWidth;
			tmpScreenHeight = WinHeight * tmpFontHeight;
		} else {
			// 新しいDPIに合わせてフォントを生成、
			// クライアント領域のサイズを決定する
			ChangeFont(vt_src, NewDPI);
			int CellWidth, CellHeight;
			DispGetCellSize(vt_src, &CellWidth, &CellHeight);
			tmpScreenWidth = WinWidth * CellWidth;
			tmpScreenHeight = WinHeight * CellHeight;
			//AdjustScrollBar();
		}

		// スクリーンサイズ(=Client Areaのサイズ)からウィンドウサイズを算出
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
					tmpScreenWidth += pGetSystemMetricsForDpi(SM_CXVSCROLL, NewDPI);
				}
				GetScrollRange(m_hWnd, SB_HORZ, &min_pos, &max_pos);
				if (min_pos != max_pos) {
					tmpScreenHeight += pGetSystemMetricsForDpi(SM_CXHSCROLL, NewDPI);
				}
			}
			RECT Rect = {0, 0, tmpScreenWidth, tmpScreenHeight};
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
			NewWindowWidth = WindowWidth - ClientWidth + tmpScreenWidth;
			NewWindowHeight = WindowHeight - ClientHeight + tmpScreenHeight;
		}

		if (calcOnly) {
			sz->cx = NewWindowWidth;
			sz->cy = NewWindowHeight;
			return TRUE;
		}

		// 推奨領域に左上寄せ
		NewWindowRect[0].top = SuggestedWindowRect.top;
		NewWindowRect[0].bottom = SuggestedWindowRect.top + NewWindowHeight;
		NewWindowRect[0].left = SuggestedWindowRect.left;
		NewWindowRect[0].right = SuggestedWindowRect.left + NewWindowWidth;

		// 推奨領域に右上寄せ
		NewWindowRect[1].top = SuggestedWindowRect.top;
		NewWindowRect[1].bottom = SuggestedWindowRect.top + NewWindowHeight;
		NewWindowRect[1].left = SuggestedWindowRect.right - NewWindowWidth;
		NewWindowRect[1].right = SuggestedWindowRect.right;

		// 推奨位置に左下寄せ
		NewWindowRect[2].top = SuggestedWindowRect.bottom - NewWindowHeight;
		NewWindowRect[2].bottom = SuggestedWindowRect.bottom;
		NewWindowRect[2].left = SuggestedWindowRect.left;
		NewWindowRect[2].right = SuggestedWindowRect.left + NewWindowWidth;

		// 推奨位置に右下寄せ
		NewWindowRect[3].top = SuggestedWindowRect.bottom - NewWindowHeight;
		NewWindowRect[3].bottom = SuggestedWindowRect.bottom;
		NewWindowRect[3].left = SuggestedWindowRect.right - NewWindowWidth;
		NewWindowRect[3].right = SuggestedWindowRect.right;

		// 確認
		NewRect = &NewWindowRect[0];
		for (size_t i = 0; i < _countof(NewWindowRect); i++) {
			const RECT *r = &NewWindowRect[i];
			HWND tmphWnd = CreateWindowExW(0, WC_STATICW, (LPCWSTR)NULL, 0,
									r->left, r->top, NewWindowWidth, NewWindowHeight,
									NULL, (HMENU)0x00, m_hInst, (LPVOID)NULL);
			if (tmphWnd) {
				assert(pGetDpiForWindow); // GetDpiForWindow()は、Windows 10 v1607 Red Stone 1 (RS1)以降で使用可能_
				int myDPI = pGetDpiForWindow(tmphWnd);
				/*
				  ・Tera Term の高 DPI(Per-Monitor V2) 対応環境は、Windows 10 v1703 以降。
				    Windows 10 v1703 未満、及び Windows 8.1、Windows 7環境では、
				    WM_DPICHANGED と WM_GETDPISCALEDSIZE は送信されない。
				  ・CVTWindow::OnDpiChanged() 内では、GetDpiForWindow() は常に使用可能だが、
				    他の箇所では GetDpiForWindow() の存在を確認するコードとすること。

				  機能                 最小要件                             リリース日 備考
				  ------------------------------------------------------------------------------------------
				  WM_DPICHANGED        Windows 8.1                          2013/10/18
				  GetDpiForMonitor()   Windows 8.1                          2013/10/18 Per-Monitor V2 非対応
				  GetDpiForWindow()    Windows 10 v1607  Red Stone 1 (RS1)  2016/08/XX
				  ------------------------------------------------------------------------------------------
				  Per-Monitor V2       Windows 10 v1703  Red Stone 2 (RS2)  2017/04/XX
				  WM_GETDPISCALEDSIZE  Windows 10 v1703  Red Stone 2 (RS2)  2017/04/XX
				  ------------------------------------------------------------------------------------------
				*/
				::DestroyWindow(tmphWnd);
				if (NewDPI == myDPI) {
					NewRect = r;
					break;
				}
			}
		}
	}

#ifdef WINDOW_MAXMIMUM_ENABLED
	if (IsZoomed(m_hWnd)) {
		ts.TerminalOldWidth = NewWindowWidth;
		ts.TerminalOldHeight = NewWindowHeight;
		GetDesktopRect(m_hWnd, &NewWindowRect[0]);
		NewRect = &NewWindowRect[0];
		NewWindowWidth  = NewRect->right  - NewRect->left;
		NewWindowHeight = NewRect->bottom - NewRect->top;
	}
#endif

	::SetWindowPos(m_hWnd, NULL,
				   NewRect->left, NewRect->top,
				   NewWindowWidth, NewWindowHeight,
				   SWP_NOZORDER);
	vtwin_work.monitor_DPI = NewDPI;

	ChangeCaret(vt_src);

	{
		HINSTANCE inst;
		WORD icon_id;
		inst = (ts.PluginVTIconInstance != NULL) ? ts.PluginVTIconInstance : m_hInst;
		icon_id = (ts.PluginVTIconID != 0) ? ts.PluginVTIconID
		                                   : (ts.VTIcon != IdIconDefault) ? ts.VTIcon
		                                                                  : IDI_VT;
		TTSetIcon(inst, m_hWnd, MAKEINTRESOURCEW(icon_id), NewDPI);
	}

	return 0;
}

LRESULT CVTWindow::Proc(UINT msg, WPARAM wp, LPARAM lp)
{
	static const UINT WM_TASKBER_CREATED = RegisterWindowMessage("TaskbarCreated");

	LRESULT retval = 0;
	if (msg == MsgDlgHelp) {
		// HELPMSGSTRING message 時
		//		wp = dialog handle
		//		lp = initialization structure
		OnDlgHelp(HelpId, 0);
		return 0;
	}
	else if (msg == WM_TASKBER_CREATED) {
		// タスクバーが再起動した
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
	case WM_ERASEBKGND:
		// 背景消去を DefWindowProc() に行わせない(DefWindowProc()を呼ばない)
		//		DefWindowProc() は背景を塗りつぶすが、
		//		WNDCLASSW.hbrBackground = NULL なので何も行わない
		// 0を返すと背景が塗りつぶしが行われていない、と判定される
		//		WM_PAINTで PAINTSTRUCT.fErase = TRUE となる
		retval = 0;
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
	case WM_USER_IDLETIMER:
		OnIdleTimer(wp, lp);
		break;
	case WM_GETDPISCALEDSIZE:
		retval = OnDpiChanged(wp, lp, TRUE);
		break;
	case WM_DPICHANGED:
		OnDpiChanged(wp, lp, FALSE);
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
		case ID_FILE_RECVFILE: OnFileRecv(); break;
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
		// 次の場合に発生する
		//		- F1 キー押下
		//		- MessageBox() の MB_HELP の HELP ボタン押下
		// - F1キー押下時、WM_HELP, WM_KEYDOWN, KEYUP とメッセージが発生する
		// - WM_HELP で何か処理をしていると WM_KEYDOWN がなくなるようだ
		vtwin_work_t *w = &vtwin_work;
		if (w->help_id != 0) {
			// ヘルプがセットされている
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
 *	WM_HELP メッセージを受信したとき表示するヘルプのヘルプIDを設定/解除する
 *
 *	@param data		0		ヘルプ解除
 *					0以外	ヘルプID
 *
 * - MessageBox() の uType に MB_HELP を設定時、HELPボタンが表示される
 * - VTWindows を親にした MessageBox() の HELPボタンが押されたとき、
 *	 このAPIで設定したヘルプIDのヘルプが表示される
 * - MessageBox() を閉じたときに 0をセットすること
 */
void VtwinSetHelpId(DWORD help_id)
{
	vtwin_work_t *w = &vtwin_work;
	w->help_id = help_id;
}
