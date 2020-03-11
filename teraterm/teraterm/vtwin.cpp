/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004-2020 TeraTerm Project
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

#include "teraterm_conf.h"
#include "teraterm.h"
#include "tttypes.h"

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
#include "ttftypes.h"
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

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>
#include <locale.h>

#include <shlobj.h>
#include <io.h>
#include <errno.h>
#include <imagehlp.h>

#include <windowsx.h>
#include <imm.h>
#include <dbt.h>
#include <assert.h>
#include <wchar.h>

#include "tt_res.h"
#include "vtwin.h"
#include "addsetting.h"
#include "winjump.h"
#include "sizetip.h"
#include "dnddlg.h"
#include "tekwin.h"
#include <htmlhelp.h>
#include "compat_win.h"
#include "unicode_test.h"
#if UNICODE_DEBUG
#include "tipwin.h"
#endif
#include "codeconv.h"
#include "layer_for_unicode.h"
#include "sendmem.h"
#include "sendfiledlg.h"

#include "initguid.h"
//#include "Usbiodef.h"
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, \
             0xC0, 0x4F, 0xB9, 0x51, 0xED);

#define VTClassName L"VTWin32"

// ウィンドウ最大化ボタンを有効にする (2005.1.15 yutaka)
#define WINDOW_MAXMIMUM_ENABLED 1

// WM_COPYDATAによるプロセス間通信の種別 (2005.1.22 yutaka)
#define IPC_BROADCAST_COMMAND 1      // 全端末へ送信
#define IPC_MULTICAST_COMMAND 2      // 任意の端末群へ送信

#define BROADCAST_LOGFILE "broadcast.log"

static BOOL TCPLocalEchoUsed = FALSE;
static BOOL TCPCRSendUsed = FALSE;

static BOOL IgnoreRelease = FALSE;

static HDEVNOTIFY hDevNotify = NULL;

static int AutoDisconnectedPort = -1;

#ifndef WM_IME_COMPOSITION
#define WM_IME_COMPOSITION              0x010F
#endif

#if UNICODE_INTERNAL_BUFF
CUnicodeDebugParam UnicodeDebugParam;
#endif

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

//
// 例外ハンドラのフック（スタックトレースのダンプ）
//
// cf. http://svn.collab.net/repos/svn/trunk/subversion/libsvn_subr/win32_crashrpt.c
// (2007.9.30 yutaka)
//
// 例外コードを文字列へ変換する
#if !defined(_M_X64)
static const char *GetExceptionString(DWORD exception)
{
#define EXCEPTION(x) case EXCEPTION_##x: return (#x);
	static char buf[16];

	switch (exception)
	{
		EXCEPTION(ACCESS_VIOLATION)
		EXCEPTION(DATATYPE_MISALIGNMENT)
		EXCEPTION(BREAKPOINT)
		EXCEPTION(SINGLE_STEP)
		EXCEPTION(ARRAY_BOUNDS_EXCEEDED)
		EXCEPTION(FLT_DENORMAL_OPERAND)
		EXCEPTION(FLT_DIVIDE_BY_ZERO)
		EXCEPTION(FLT_INEXACT_RESULT)
		EXCEPTION(FLT_INVALID_OPERATION)
		EXCEPTION(FLT_OVERFLOW)
		EXCEPTION(FLT_STACK_CHECK)
		EXCEPTION(FLT_UNDERFLOW)
		EXCEPTION(INT_DIVIDE_BY_ZERO)
		EXCEPTION(INT_OVERFLOW)
		EXCEPTION(PRIV_INSTRUCTION)
		EXCEPTION(IN_PAGE_ERROR)
		EXCEPTION(ILLEGAL_INSTRUCTION)
		EXCEPTION(NONCONTINUABLE_EXCEPTION)
		EXCEPTION(STACK_OVERFLOW)
		EXCEPTION(INVALID_DISPOSITION)
		EXCEPTION(GUARD_PAGE)
		EXCEPTION(INVALID_HANDLE)

	default:
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "0x%x", exception);
		return buf;
		//return "UNKNOWN_ERROR";
	}
#undef EXCEPTION
}

/* 例外発生時に関数の呼び出し履歴を表示する、例外フィルタ関数 */
static LONG CALLBACK ApplicationFaultHandler(EXCEPTION_POINTERS *ExInfo)
{
	HGLOBAL gptr;
	STACKFRAME sf;
	BOOL bResult;
	PIMAGEHLP_SYMBOL pSym;
	DWORD Disp;
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	IMAGEHLP_MODULE	ih_module;
	IMAGEHLP_LINE	ih_line;
	int frame;
	char msg[3072], buf[256];
	HMODULE h, h2;
	char imagehlp_dll[MAX_PATH];

	// Windows98/Me/NT4では動かないためスキップする。(2007.10.9 yutaka)
	GetSystemDirectory(imagehlp_dll, sizeof(imagehlp_dll));
	strncat_s(imagehlp_dll, sizeof(imagehlp_dll), "\\imagehlp.dll", _TRUNCATE);
	h2 = LoadLibrary(imagehlp_dll);
	if (((h = GetModuleHandle(imagehlp_dll)) == NULL) ||
		(GetProcAddress(h, "SymGetLineFromAddr") == NULL)) {
			FreeLibrary(h2);
			goto error;
	}
	FreeLibrary(h2);

	/* シンボル情報格納用バッファの初期化 */
	gptr = GlobalAlloc(GMEM_FIXED, 10000);
	if (gptr == NULL) {
		goto error;
	}
	pSym = (PIMAGEHLP_SYMBOL)GlobalLock(gptr);
	ZeroMemory(pSym, sizeof(IMAGEHLP_SYMBOL));
	pSym->SizeOfStruct = 10000;
	pSym->MaxNameLength = 10000 - sizeof(IMAGEHLP_SYMBOL);

	/* スタックフレームの初期化 */
	ZeroMemory(&sf, sizeof(sf));
	sf.AddrPC.Offset = ExInfo->ContextRecord->Eip;
	sf.AddrStack.Offset = ExInfo->ContextRecord->Esp;
	sf.AddrFrame.Offset = ExInfo->ContextRecord->Ebp;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Mode = AddrModeFlat;

	/* シンボルハンドラの初期化 */
	SymInitialize(hProcess, NULL, TRUE);

	// レジスタダンプ
	msg[0] = '\0';
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "eax=%08X ebx=%08X ecx=%08X edx=%08X esi=%08X edi=%08X\r\n"
		   "ebp=%08X esp=%08X eip=%08X efl=%08X\r\n"
		   "cs=%04X ss=%04X ds=%04X es=%04X fs=%04X gs=%04X\r\n",
		   ExInfo->ContextRecord->Eax,
		   ExInfo->ContextRecord->Ebx,
		   ExInfo->ContextRecord->Ecx,
		   ExInfo->ContextRecord->Edx,
		   ExInfo->ContextRecord->Esi,
		   ExInfo->ContextRecord->Edi,
		   ExInfo->ContextRecord->Ebp,
		   ExInfo->ContextRecord->Esp,
		   ExInfo->ContextRecord->Eip,
		   ExInfo->ContextRecord->EFlags,
		   ExInfo->ContextRecord->SegCs,
		   ExInfo->ContextRecord->SegSs,
		   ExInfo->ContextRecord->SegDs,
		   ExInfo->ContextRecord->SegEs,
		   ExInfo->ContextRecord->SegFs,
		   ExInfo->ContextRecord->SegGs
	);
	strncat_s(msg, sizeof(msg), buf, _TRUNCATE);

	if (ExInfo->ExceptionRecord != NULL) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Exception: %s\r\n", GetExceptionString(ExInfo->ExceptionRecord->ExceptionCode));
		strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
	}

	/* スタックフレームを順に表示していく */
	frame = 0;
	for (;;) {
		/* 次のスタックフレームの取得 */
		bResult = StackWalk(
			IMAGE_FILE_MACHINE_I386,
			hProcess,
			hThread,
			&sf,
			NULL,
			NULL,
			SymFunctionTableAccess,
			SymGetModuleBase,
			NULL);

		/* 失敗ならば、ループを抜ける */
		if (!bResult || sf.AddrFrame.Offset == 0)
			break;

		frame++;

		/* プログラムカウンタ（仮想アドレス）から関数名とオフセットを取得 */
		bResult = SymGetSymFromAddr(hProcess, sf.AddrPC.Offset, &Disp, pSym);

		/* 取得結果を表示 */
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "#%d  0x%08x in ", frame, sf.AddrPC.Offset);
		strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		if (bResult) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s() + 0x%x ", pSym->Name, Disp);
			strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		} else {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, " --- ");
			strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		}

		// 実行ファイル名の取得
		ZeroMemory( &(ih_module), sizeof(ih_module) );
		ih_module.SizeOfStruct = sizeof(ih_module);
		bResult = SymGetModuleInfo( hProcess, sf.AddrPC.Offset, &(ih_module) );
		strncat_s(msg, sizeof(msg), "at ", _TRUNCATE);
		if (bResult) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s ", ih_module.ImageName );
			strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		} else {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s ", "<Unknown Module>" );
			strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		}

		// ファイル名と行番号の取得
		ZeroMemory( &(ih_line), sizeof(ih_line) );
		ih_line.SizeOfStruct = sizeof(ih_line);
		bResult = SymGetLineFromAddr( hProcess, sf.AddrPC.Offset, &Disp, &ih_line );
		if (bResult)
		{
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s:%lu", ih_line.FileName, ih_line.LineNumber );
			strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		}

		strncat_s(msg, sizeof(msg), "\n", _TRUNCATE);
	}

	/* 後処理 */
	SymCleanup(hProcess);
	GlobalUnlock(pSym);
	GlobalFree(pSym);

	// 例外処理中なので、APIを直接呼び出す
	::MessageBoxA(NULL, msg, "Tera Term: Application fault", MB_OK | MB_ICONEXCLAMATION);

error:
//	return (EXCEPTION_EXECUTE_HANDLER);  /* そのままプロセスを終了させる */
	return (EXCEPTION_CONTINUE_SEARCH);  /* 引き続き［アプリケーションエラー］ポップアップメッセージボックスを呼び出す */
}
#endif // !defined(_M_X64 )


// Virtual Storeが有効であるかどうかを判別する。
//
// [Windows 95-XP]
// return FALSE (always)
//
// [Windows Vista-10]
// return TRUE:  Virtual Store Enabled
//        FALSE: Virtual Store Disabled or Unknown
//
BOOL GetVirtualStoreEnvironment(void)
{
#if _MSC_VER == 1400  // VSC2005(VC8.0)
	typedef struct _TOKEN_ELEVATION {
		DWORD TokenIsElevated;
	} TOKEN_ELEVATION, *PTOKEN_ELEVATION;
	int TokenElevation = 20;
#endif
	BOOL ret = FALSE;
	int flag = 0;
	HANDLE          hToken;
	DWORD           dwLength;
	TOKEN_ELEVATION tokenElevation;
	LONG lRet;
	HKEY hKey;
	char lpData[256];
	DWORD dwDataSize;
	DWORD dwType;
	BYTE bValue;

	// Windows Vista以前は無視する。
	if (!IsWindowsVistaOrLater())
		goto error;

	// UACが有効かどうか。
	// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\SystemのEnableLUA(DWORD値)が0かどうかで判断できます(0はUAC無効、1はUAC有効)。
	flag = 0;
	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
		NULL, KEY_QUERY_VALUE, &hKey
		);
	if (lRet == ERROR_SUCCESS) {
		dwDataSize = sizeof(lpData) / sizeof(lpData[0]);
		lRet = RegQueryValueEx(
			hKey,
			TEXT("EnableLUA"),
			0,
			&dwType,
			(LPBYTE)lpData,
			&dwDataSize);
		if (lRet == ERROR_SUCCESS) {
			bValue = ((LPBYTE)lpData)[0];
			if (bValue == 1)
				// UACが有効の場合、Virtual Storeが働く。
				flag = 1;
		}
		RegCloseKey(hKey);
	}
	if (flag == 0)
		goto error;

	// UACが有効時、プロセスが管理者権限に昇格しているか。
	flag = 0;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_DEFAULT, &hToken)) {
		if (GetTokenInformation(hToken, (TOKEN_INFORMATION_CLASS)TokenElevation, &tokenElevation, sizeof(TOKEN_ELEVATION), &dwLength)) {
			// (0は昇格していない、非0は昇格している)。
			if (tokenElevation.TokenIsElevated == 0) {
				// 管理者権限を持っていなければ、Virtual Storeが働く。
				flag = 1;
			}
		}
		CloseHandle(hToken);
	}
	if (flag == 0)
		goto error;

	ret = TRUE;
	return (ret);

error:
	return (ret);
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
#ifdef SHARED_KEYMAP
	char Temp[MAX_PATH];
	PKeyMap tempkm;
#endif
	int fuLoad = LR_DEFAULTCOLOR;
	BOOL isFirstInstance;
	m_hInst = hInstance;

	// 例外ハンドラのフック (2007.9.30 yutaka)
#if !defined(_M_X64)
	SetUnhandledExceptionFilter(ApplicationFaultHandler);
#endif

	CommInit(&cv);
	isFirstInstance = StartTeraTerm(&ts);

	TTXInit(&ts, &cv); /* TTPLUG */

	MsgDlgHelp = RegisterWindowMessage(HELPMSGSTRING);

	if (isFirstInstance) {
		/* first instance */
		if (LoadTTSET()) {
			/* read setup info from "teraterm.ini" */
			(*ReadIniFile)(ts.SetupFName, &ts);
#ifdef SHARED_KEYMAP
			/* read keycode map from "keyboard.cnf" */
			tempkm = (PKeyMap)malloc(sizeof(TKeyMap));
			if (tempkm!=NULL) {
				strncpy_s(Temp, sizeof(Temp), ts.HomeDir, _TRUNCATE);
				AppendSlash(Temp,sizeof(Temp));
				strncat_s(Temp,sizeof(Temp),"KEYBOARD.CNF",_TRUNCATE);
				(*ReadKeyboardCnf)(Temp,tempkm,TRUE);
			}
#endif
			FreeTTSET();
#ifdef SHARED_KEYMAP
			/* store default sets in TTCMN */
#if 0
			ChangeDefaultSet(&ts,tempkm);
#else
			ChangeDefaultSet(NULL,tempkm);
#endif
			if (tempkm!=NULL) free(tempkm);
#endif
		}
		else {
			abort();
		}

	} else {
		// 2つめ以降のプロセスにおいても、ディスクから TERATERM.INI を読む。(2004.11.4 yutaka)
		if (LoadTTSET()) {
			/* read setup info from "teraterm.ini" */
			(*ReadIniFile)(ts.SetupFName, &ts);
#ifdef SHARED_KEYMAP
			/* read keycode map from "keyboard.cnf" */
			tempkm = (PKeyMap)malloc(sizeof(TKeyMap));
			if (tempkm!=NULL) {
				strncpy_s(Temp, sizeof(Temp), ts.HomeDir, _TRUNCATE);
				AppendSlash(Temp,sizeof(Temp));
				strncat_s(Temp,sizeof(Temp),"KEYBOARD.CNF",_TRUNCATE);
				(*ReadKeyboardCnf)(Temp,tempkm,TRUE);
			}
#endif
			FreeTTSET();
#ifdef SHARED_KEYMAP
			/* store default sets in TTCMN */
			if (tempkm!=NULL) {
				free(tempkm);
			}
#endif
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

	/* Initialize scroll buffer */
	InitBuffer();

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

	_RegisterClassW(&wc);
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
	// ロケールの設定
	// wctomb のため
	setlocale(LC_ALL, ts.Locale);

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

	// Tera Termの起動時、Virtual Storeが働くかどうかを覚えておく。
	// (2015.11.14 yutaka)
	cv.VirtualStoreEnabled = GetVirtualStoreEnvironment();

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
		BuffEndSelect();
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
		(SendVar==NULL) && (FileVar==NULL) &&
		(cv.PortType!=IdFile) &&
		(IsClipboardFormatAvailable(CF_TEXT) ||
		 IsClipboardFormatAvailable(CF_OEMTEXT))) {

		int i, numItems;
		char itemText[256];

		InitPasteMenu(&PopupMenu);
		PopupBase = CreatePopupMenu();
		numItems = GetMenuItemCount(PopupMenu);

		for (i = 0; i < numItems; i++) {
			if (GetMenuString(PopupMenu, i, itemText, sizeof(itemText), MF_BYPOSITION) != 0) {
				int state = GetMenuState(PopupMenu, i, MF_BYPOSITION) &
				            (MF_CHECKED | MF_DISABLED | MF_GRAYED | MF_HILITE |
				             MF_MENUBARBREAK | MF_MENUBREAK | MF_SEPARATOR);

				AppendMenu(PopupBase, state,
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

	SetDlgMenuTexts(hMenu, MenuTextInfo, _countof(MenuTextInfo), ts.UILanguageFile);

	/* LogMeTT の存在を確認してメニューを追加する */
	if (isLogMeTTExist()) {
		::InsertMenu(FileMenu, ID_FILE_PRINT2, MF_STRING | MF_ENABLED | MF_BYCOMMAND,
		             ID_FILE_LOGMEIN, LogMeTTMenuString);
		::InsertMenu(FileMenu, ID_FILE_PRINT2, MF_SEPARATOR, NULL, NULL);
	}

	SetDlgMenuTexts(FileMenu, FileMenuTextInfo, _countof(FileMenuTextInfo), ts.UILanguageFile);
	SetDlgMenuTexts(EditMenu, EditMenuTextInfo, _countof(EditMenuTextInfo), ts.UILanguageFile);
	SetDlgMenuTexts(SetupMenu, SetupMenuTextInfo, _countof(SetupMenuTextInfo), ts.UILanguageFile);
	SetDlgMenuTexts(ControlMenu, ControlMenuTextInfo, _countof(ControlMenuTextInfo), ts.UILanguageFile);
	SetDlgMenuTexts(HelpMenu, HelpMenuTextInfo, _countof(HelpMenuTextInfo), ts.UILanguageFile);

	if ((ts.MenuFlag & MF_SHOWWINMENU) !=0) {
		wchar_t uimsg[MAX_UIMSG];
		WinMenu = CreatePopupMenu();
		get_lang_msgW("MENU_WINDOW", uimsg, _countof(uimsg),
					  L"&Window", ts.UILanguageFile);
		_InsertMenuW(hMenu, ID_HELPMENU,
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

		if ( (! cv.Ready) || (SendVar!=NULL) ||
		     (FileVar!=NULL) || (cv.PortType==IdFile) ) {
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
		if (LogVar!=NULL) { // ログ採取モードの場合
			EnableMenuItem(FileMenu,ID_FILE_LOG,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_COMMENTTOLOG, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_VIEWLOG, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_SHOWLOGDIALOG, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_PAUSELOG, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_STOPLOG, MF_BYCOMMAND | MF_ENABLED);
			if (cv.FilePause & OpLog) {
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
		    (SendVar==NULL) && (FileVar==NULL) &&
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
		if ((SendVar!=NULL) || (FileVar!=NULL) || Connecting) {
			EnableMenuItem(SetupMenu,ID_SETUP_SERIALPORT,MF_BYCOMMAND | MF_GRAYED);
		}
		else {
			EnableMenuItem(SetupMenu,ID_SETUP_SERIALPORT,MF_BYCOMMAND | MF_ENABLED);
		}

	else if (SubMenu == ControlMenu)
	{
		if (cv.Ready &&
		    (SendVar==NULL) && (FileVar==NULL)) {
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

		if (cv.Ready && cv.TelFlag && (FileVar==NULL)) {
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

		if ((ConvH!=0) || (FileVar!=NULL)) {
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
	SetDlgMenuTexts(*Menu, MenuTextInfo, _countof(MenuTextInfo), ts.UILanguageFile);
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

#if 0
	ChangeDefaultSet(&ts,NULL);
#endif

	ResetSetup();
}

/* called by the [Setup] Terminal command */
void CVTWindow::SetupTerm()
{
	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		ResetCharSet();
	}
	cv.CRSend = ts.CRSend;

	// for russian mode
	cv.RussHost = ts.RussHost;
	cv.RussClient = ts.RussClient;

	if (cv.Ready) {
		if (cv.TelFlag && (ts.TelEcho>0)) {
			TelChangeEcho();
		}
		_free_locale(cv.locale);
		cv.locale = _create_locale(LC_ALL, cv.Locale);
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

#if UNICODE_INTERNAL_BUFF
	for (i=1 ; i<=nRepCnt ; i++) {
		wchar_t u16 = nChar;
		CommTextOutW(&cv,&u16,1);
		if (ts.LocalEcho>0) {
			CommTextEchoW(&cv,&u16,1);
		}
	}
#else
	{
		char Code = nChar;
		if ((ts.Language==IdRussian) &&
			((BYTE)Code>=128)) {
			Code = (char)RussConv(ts.RussKeyb,ts.RussClient,(BYTE)Code);
		}

		for (i=1 ; i<=nRepCnt ; i++) {
			CommTextOut(&cv,&Code,1);
			if (ts.LocalEcho>0) {
				CommTextEcho(&cv,&Code,1);
			}
		}
	}
#endif

	// スクロール位置をリセット
	if (WinOrgY != 0) {
		DispVScroll(SCROLL_BOTTOM, 0);
	}
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
		wchar_t uimsg[MAX_UIMSG];
		get_lang_msgW("MSG_DISCONNECT_CONF", uimsg, _countof(uimsg),
					  L"Disconnect?", ts.UILanguageFile);
		int result = _MessageBoxW(HVTWin, uimsg, L"Tera Term",
								  MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2);
		if (result == IDCANCEL) {
			return;
		}
	}

	FileTransEnd(0);
	ProtoEnd();

	SaveVTPos();
	DestroyWindow();
}

// 全Tera Termの終了を指示する
void CVTWindow::OnAllClose()
{
	// 突然終了させると危険なので、かならずユーザに問い合わせを出すようにする。
	// (2013.8.17 yutaka)
	wchar_t uimsg[MAX_UIMSG];

	get_lang_msgW("MSG_ALL_TERMINATE_CONF", uimsg, _countof(uimsg), L"Terminate ALL Tera Term(s)?", ts.UILanguageFile);
	if (_MessageBoxW(HVTWin, uimsg, L"Tera Term", MB_OKCANCEL | MB_ICONERROR | MB_DEFBUTTON2) == IDCANCEL)
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

	do { }
	while (FreeTTFILE());

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

static void PasteString(PComVar cv, const wchar_t *str, bool escape)
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

	SendMemPasteString(tmpbuf);
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
		const DWORD attr = _GetFileAttributesW(FileName);
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
			SendMemSendFile(FileName, FALSE);
			break;
		case DROP_TYPE_SEND_FILE_BINARY:
			SendMemSendFile(FileName, TRUE);
			break;
		case DROP_TYPE_PASTE_FILENAME:
		{
			const bool escape = (DropTypePaste & DROP_TYPE_PASTE_ESCAPE) ? true : false;

			TermSendStartBracket();

			PasteString(&cv, FileName, escape);
			if (DropListCount > 1 && i < DropListCount - 1) {
				const wchar_t *separator = (DropTypePaste & DROP_TYPE_PASTE_NEWLINE) ? L"\n" : L" ";
				PasteString(&cv, separator, false);
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
	if (cv.Ready && SendVar==NULL)
	{
		const UINT ShowDialog =
			((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) ? 1 : 0;
		DropListCount = _DragQueryFileW(hDropInfo, -1, NULL, 0);
		DropLists = (wchar_t **)malloc(sizeof(wchar_t *) * DropListCount);

		for (int i = 0; i < DropListCount; i++) {
			const UINT cch = _DragQueryFileW(hDropInfo, i, NULL, 0);
			if (cch == 0) {
				continue;
			}
			wchar_t *FileName = (wchar_t *)malloc(sizeof(wchar_t) * (cch + 1));
			_DragQueryFileW(hDropInfo,i,FileName,cch + 1);
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
	BYTE KeyState[256];
	MSG M;

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
		PeekMessage((LPMSG)&M,HVTWin,WM_CHAR,WM_CHAR,PM_REMOVE);
		/* for Ctrl+Alt+Key combination */
		GetKeyboardState((PBYTE)KeyState);
		KeyState[VK_MENU] = 0;
		SetKeyboardState((PBYTE)KeyState);
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

int CVTWindow::OnMouseActivate(HWND pDesktopWnd, UINT nHitTest, UINT message)
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
	BOOL mousereport;

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
			wchar_t tipbuf[32];
			wchar_t uimsg[MAX_UIMSG];
			POINT tippos;

			newAlpha += delta * ts.MouseWheelScrollLine;
			if (newAlpha > 255)
				newAlpha = 255;
			else if (newAlpha < 0)
				newAlpha = 0;
			SetWindowAlpha(newAlpha);

			get_lang_msgW("TOOLTIP_TITLEBAR_OPACITY", uimsg, sizeof(uimsg), L"Opacity %.1f %%", ts.UILanguageFile);
			_snwprintf_s(tipbuf, _countof(tipbuf), uimsg, (newAlpha / 255.0) * 100);

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
	char e = ESC;
	char Code;
	unsigned int i;

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
		if (!KeybEnabled || (TalkStatus!=IdTalkKeyb)) return;
		Code = nChar;
		for (i=1 ; i<=nRepCnt ; i++) {
			switch (ts.Meta8Bit) {
			  case IdMeta8BitRaw:
				Code |= 0x80;
				CommBinaryBuffOut(&cv, &Code, 1);
				if (ts.LocalEcho) {
					CommBinaryEcho(&cv, &Code, 1);
				}
				break;
			  case IdMeta8BitText:
				Code |= 0x80;
				CommTextOut(&cv, &Code, 1);
				if (ts.LocalEcho) {
					CommTextEcho(&cv, &Code, 1);
				}
				break;
			  default:
				CommTextOut(&cv, &e, 1);
				CommTextOut(&cv, &Code, 1);
				if (ts.LocalEcho) {
					CommTextEcho(&cv, &e, 1);
					CommTextEcho(&cv, &Code, 1);
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
			PrnFileStart();
			break;
		case IdPrnProcTimer:
			PrnFileDirectProc();
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
		char buf[512];			// 参照文字列を受け取るバッファ
		size_t str_len_count;
		int cx;
//		assert(IsWindowUnicode(hWnd) == FALSE);		// TODO UNICODE/ANSI切り替え

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

		// 1回目はサイズだけを返す
		result = ReconvSizeSave;
	}
	else {
		// 2回目の呼び出し 構造体を渡す
		if (pReconvPtrSave != NULL) {
			RECONVERTSTRING *pReconv = (RECONVERTSTRING*)lParam;
			memcpy(pReconv, pReconvPtrSave, ReconvSizeSave);
			result = ReconvSizeSave;
			free(pReconvPtrSave);
			pReconvPtrSave = NULL;
			ReconvSizeSave = 0;
		} else {
			// 3回目?
			result = 0;
		}
	}

#if 0
	OutputDebugPrintf("WM_IME_REQUEST,IMR_DOCUMENTFEED lp=%p LRESULT %d\n",
		lParam, result);
#endif

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

	/* Auto start logging (2007.5.31 maya) */
	if (ts.LogAutoStart && ts.LogFN[0]==0) {
		strncpy_s(ts.LogFN, sizeof(ts.LogFN), ts.LogDefaultName, _TRUNCATE);
	}
	/* ログ採取が有効で開始していなければ開始する (2006.9.18 maya) */
	if ((ts.LogFN[0]!=0) && (LogVar==NULL) && NewFileVar(&LogVar)) {
		LogVar->DirLen = 0;
		strncpy_s(LogVar->FullName, sizeof(LogVar->FullName), ts.LogFN, _TRUNCATE);
		HelpId = HlpFileLog;
		LogStart();
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

	if (DDELog || FileLog) {
		if (! CreateLogBuf()) {
			if (DDELog) {
				EndDDE();
			}
			if (FileLog) {
				FileTransEnd(OpLog);
			}
		}
	}

	if (BinLog) {
		if (! CreateBinBuf()) {
			FileTransEnd(OpLog);
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

LRESULT CVTWindow::OnFileTransEnd(WPARAM wParam, LPARAM lParam)
{
	FileTransEnd(wParam);
	return 0;
}

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
			int len = strlen(buf);

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
		wchar_t buf[80];
		wchar_t uimsg[MAX_UIMSG];
		wchar_t uimsg2[MAX_UIMSG];
		get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
		get_lang_msgW("MSG_EXEC_TT_ERROR", uimsg2, _countof(uimsg2),
					  L"Can't execute Tera Term. (%d)", ts.UILanguageFile);
		_snwprintf_s(buf, _countof(buf), _TRUNCATE, uimsg2, GetLastError());
		_MessageBoxW(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
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
	int envbufflen;
	const char *exename = "cygterm.exe";
	char cygterm[MAX_PATH];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	wchar_t uimsg[MAX_UIMSG];
	wchar_t uimsg2[MAX_UIMSG];

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

	get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
	get_lang_msgW("MSG_FIND_CYGTERM_DIR_ERROR", uimsg2, sizeof(uimsg2),
				  L"Can't find Cygwin directory.", ts.UILanguageFile);
	_MessageBoxW(NULL, uimsg2, uimsg, MB_OK | MB_ICONWARNING);
	return;

found_dll:;
	__dupenv_s(&envptr, NULL, "PATH");
	file[strlen(file)-12] = '\0'; // delete "\\cygwin1.dll"
	if (envptr != NULL) {
		envbufflen = strlen(file) + strlen(envptr) + 7; // "PATH="(5) + ";"(1) + NUL(1)
		if ((envbuff = (char *)malloc(envbufflen)) == NULL) {
			get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
			get_lang_msgW("MSG_CYGTERM_ENV_ALLOC_ERROR", uimsg2, _countof(uimsg2),
						  L"Can't allocate memory for environment variable.", ts.UILanguageFile);
			_MessageBoxW(NULL, uimsg2, uimsg, MB_OK | MB_ICONWARNING);
			free(envptr);
			return;
		}
		_snprintf_s(envbuff, envbufflen, _TRUNCATE, "PATH=%s;%s", file, envptr);
		free(envptr);
	} else {
		envbufflen = strlen(file) + 6; // "PATH="(5) + NUL(1)
		if ((envbuff = (char *)malloc(envbufflen)) == NULL) {
			get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
			get_lang_msgW("MSG_CYGTERM_ENV_ALLOC_ERROR", uimsg2, _countof(uimsg2),
						  L"Can't allocate memory for environment variable.", ts.UILanguageFile);
			_MessageBoxW(NULL, uimsg2, uimsg, MB_OK | MB_ICONWARNING);
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
		get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
		get_lang_msgW("MSG_EXEC_CYGTERM_ERROR", uimsg2, _countof(uimsg2),
		              L"Can't execute Cygterm.", ts.UILanguageFile);
		_MessageBoxW(NULL, uimsg2, uimsg, MB_OK | MB_ICONWARNING);
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
		wchar_t buf[80];
		wchar_t uimsg[MAX_UIMSG];
		wchar_t uimsg2[MAX_UIMSG];
		get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
		get_lang_msgW("MSG_EXEC_TTMENU_ERROR", uimsg2, _countof(uimsg2),
					  L"Can't execute TeraTerm Menu. (%d)", ts.UILanguageFile);
		_snwprintf_s(buf, _countof(buf), _TRUNCATE, uimsg2, GetLastError());
		_MessageBoxW(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
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
		wchar_t buf[80];
		wchar_t uimsg[MAX_UIMSG];
		wchar_t uimsg2[MAX_UIMSG];
		get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
		get_lang_msgW("MSG_EXEC_LOGMETT_ERROR", uimsg2, _countof(uimsg2),
					  L"Can't execute LogMeTT. (%d)", ts.UILanguageFile);
		_snwprintf_s(buf, _countof(buf), _TRUNCATE, uimsg2, GetLastError());
		_MessageBoxW(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}


void CVTWindow::OnFileLog()
{
	HelpId = HlpFileLog;
	LogStart();
}


static LRESULT CALLBACK OnCommentDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_COMMENT_TITLE" },
		{ IDOK, "BTN_OK" }
	};
	char buf[256];
	UINT ret;

	switch (msg) {
		case WM_INITDIALOG:
			//SetDlgItemText(hDlgWnd, IDC_EDIT_COMMENT, "サンプル");
			// エディットコントロールにフォーカスをあてる
			SetFocus(GetDlgItem(hDlgWnd, IDC_EDIT_COMMENT));
			SetDlgTexts(hDlgWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);
			return FALSE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					memset(buf, 0, sizeof(buf));
					ret = GetDlgItemTextA(hDlgWnd, IDC_EDIT_COMMENT, buf, sizeof(buf) - 1);
					if (ret > 0) { // テキスト取得成功
						//buf[sizeof(buf) - 1] = '\0';  // null-terminate
						CommentLogToFile(buf, ret);
					}
					TTEndDialog(hDlgWnd, IDOK);
					break;
				default:
					return FALSE;
			}
			break;
		case WM_CLOSE:
			TTEndDialog(hDlgWnd, 0);
			return TRUE;

		default:
			return FALSE;
	}
	return TRUE;
}

void CVTWindow::OnCommentToLog()
{
	// ログファイルへコメントを追加する (2004.8.6 yutaka)
	TTDialogBox(m_hInst, MAKEINTRESOURCE(IDD_COMMENT_DIALOG),
				HVTWin, (DLGPROC)OnCommentDlgProc);
}

// ログの閲覧 (2005.1.29 yutaka)
void CVTWindow::OnViewLog()
{
	char command[MAX_PATH*2+3]; // command "filename"
	char *file;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	if (LogVar == NULL || !LogVar->FileOpen) {
		return;
	}

	file = LogVar->FullName;

	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	_snprintf_s(command, sizeof(command), _TRUNCATE, "\"%s\" \"%s\"", ts.ViewlogEditor, file);

	if (CreateProcess(NULL, command, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
		wchar_t buf[80];
		wchar_t uimsgW[MAX_UIMSG];
		wchar_t uimsgW2[MAX_UIMSG];
		get_lang_msgW("MSG_ERROR", uimsgW, _countof(uimsgW), L"ERROR", ts.UILanguageFile);
		get_lang_msgW("MSG_VIEW_LOGFILE_ERROR", uimsgW2, _countof(uimsgW2),
					  L"Can't view logging file. (%d)", ts.UILanguageFile);
		_snwprintf_s(buf, _countof(buf), _TRUNCATE, uimsgW2, GetLastError());
		_MessageBoxW(NULL, buf, uimsgW, MB_OK | MB_ICONWARNING);
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}


// 隠しているログダイアログを表示する (2008.2.3 maya)
void CVTWindow::OnShowLogDialog()
{
	ShowFTDlg(OpLog);
}

// ログ取得を中断/再開する
void CVTWindow::OnPauseLog()
{
	FLogChangeButton(!(cv.FilePause & OpLog));
}

// ログ取得を終了する
void CVTWindow::OnStopLog()
{
	FileTransEnd(OpLog);
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
		wchar_t buf[80];
		wchar_t uimsgW[MAX_UIMSG];
		wchar_t uimsgW2[MAX_UIMSG];
		get_lang_msgW("MSG_ERROR", uimsgW, _countof(uimsgW), L"ERROR", ts.UILanguageFile);
		get_lang_msgW("MSG_EXEC_TT_ERROR", uimsgW2, _countof(uimsgW2),
		              L"Can't execute Tera Term. (%d)", ts.UILanguageFile);
		_snwprintf_s(buf, _countof(buf), _TRUNCATE, uimsgW2, GetLastError());
		_MessageBoxW(NULL, buf, uimsgW, MB_OK | MB_ICONWARNING);
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

void CVTWindow::OnFileSend()
{
#if !UNICODE_INTERNAL_BUFF
	HelpId = HlpFileSend;
	FileSendStart();
#else
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
	SendMemSendFile(filename, data.binary, (SendMemDelayType)data.delay_type, data.delay_tick, data.send_size);
	free(filename);
#endif
}

void CVTWindow::OnFileKermitRcv()
{
	KermitStart(IdKmtReceive);
}

void CVTWindow::OnFileKermitGet()
{
	HelpId = HlpFileKmtGet;
	KermitStart(IdKmtGet);
}

void CVTWindow::OnFileKermitSend()
{
	HelpId = HlpFileKmtSend;
	KermitStart(IdKmtSend);
}

void CVTWindow::OnFileKermitFinish()
{
	KermitStart(IdKmtFinish);
}

void CVTWindow::OnFileXRcv()
{
	HelpId = HlpFileXmodemRecv;
	XMODEMStart(IdXReceive);
}

void CVTWindow::OnFileXSend()
{
	HelpId = HlpFileXmodemSend;
	XMODEMStart(IdXSend);
}

void CVTWindow::OnFileYRcv()
{
	HelpId = HlpFileYmodemRecv;
	YMODEMStart(IdYReceive);
}

void CVTWindow::OnFileYSend()
{
	HelpId = HlpFileYmodemSend;
	YMODEMStart(IdYSend);
}

void CVTWindow::OnFileZRcv()
{
	ZMODEMStart(IdZReceive);
}

void CVTWindow::OnFileZSend()
{
	HelpId = HlpFileZmodemSend;
	ZMODEMStart(IdZSend);
}

void CVTWindow::OnFileBPRcv()
{
	BPStart(IdBPReceive);
}

void CVTWindow::OnFileBPSend()
{
	HelpId = HlpFileBPlusSend;
	BPStart(IdBPSend);
}

void CVTWindow::OnFileQVRcv()
{
	QVStart(IdQVReceive);
}

void CVTWindow::OnFileQVSend()
{
	HelpId = HlpFileQVSend;
	QVStart(IdQVSend);
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
		wchar_t uimsg[MAX_UIMSG];
		get_lang_msgW("MSG_DISCONNECT_CONF", uimsg, _countof(uimsg),
					  L"Disconnect?", ts.UILanguageFile);
		if (_MessageBoxW(HVTWin, uimsg, L"Tera Term",
		                 MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2)==IDCANCEL) {
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
#if	UNICODE_INTERNAL_BUFF
	BuffCBCopyUnicode(FALSE);
#else
	BuffCBCopy(FALSE);
#endif
}

void CVTWindow::OnEditCopyTable()
{
	// copy selected text to clipboard in Excel format
#if	UNICODE_INTERNAL_BUFF
	BuffCBCopyUnicode(TRUE);
#else
	BuffCBCopy(TRUE);
#endif
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
}

static BOOL CALLBACK TFontHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_INITDIALOG) {
		wchar_t uimsg[MAX_UIMSG];
		get_lang_msgW("DLG_CHOOSEFONT_STC6", uimsg, _countof(uimsg),
					  L"\"Font style\" selection here won't affect actual font appearance.", ts.UILanguageFile);
		_SetDlgItemTextW(Dialog, stc6, uimsg);

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
	cf.lpfnHook = (LPCFHOOKPROC)(&TFontHook);
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

void CVTWindow::OnSetupSave()
{
	BOOL Ok;
	char TmpSetupFN[MAX_PATH];
	int ret;

	strncpy_s(TmpSetupFN, sizeof(TmpSetupFN),ts.SetupFName, _TRUNCATE);
	if (! LoadTTFILE()) {
		return;
	}
	HelpId = HlpSetupSave;
	Ok = (*GetSetupFname)(HVTWin,GSF_SAVE,&ts);
	FreeTTFILE();
	if (! Ok) {
		return;
	}

	// 書き込みできるかの判別を追加 (2005.11.3 yutaka)
	if ((ret = _access(ts.SetupFName, 0x02)) != 0) {
		if (errno != ENOENT) {  // ファイルがすでに存在する場合のみエラーとする (2005.12.13 yutaka)
			wchar_t uimsg[MAX_UIMSG];
			wchar_t uimsg2[MAX_UIMSG];
			get_lang_msgW("MSG_TT_ERROR", uimsg, _countof(uimsg), L"Tera Term: ERROR", ts.UILanguageFile);
			get_lang_msgW("MSG_SAVESETUP_PERMISSION_ERROR", uimsg2, _countof(uimsg2),
						  L"TERATERM.INI file doesn't have the writable permission.", ts.UILanguageFile);
			_MessageBoxW(HVTWin, uimsg2, uimsg, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
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

		CopyFile(TmpSetupFN, ts.SetupFName, TRUE);
		/* write current setup values to file */
		(*WriteIniFile)(ts.SetupFName,&ts);
		/* copy host list */
		(*CopyHostList)(TmpSetupFN,ts.SetupFName);
		FreeTTSET();

#ifdef WINDOW_MAXMIMUM_ENABLED
		if (::IsZoomed(m_hWnd)) {
			ts.TerminalWidth = w;
			ts.TerminalHeight = h;
		}
#endif
	}

#if 0
	ChangeDefaultSet(&ts,NULL);
#endif
}

void CVTWindow::OnSetupRestore()
{
	BOOL Ok;

	HelpId = HlpSetupRestore;
	if (! LoadTTFILE()) {
		return;
	}
	Ok = (*GetSetupFname)(HVTWin,GSF_RESTORE,&ts);
	FreeTTFILE();
	if (Ok) {
		RestoreSetup();
	}
}


//
// 指定したアプリケーションでファイルを開く。
//
// return TRUE: success
//        FALSE: failure
//
static BOOL openFileWithApplication(char *pathname, char *filename, char *editor)
{
	char command[1024];
	char fullpath[1024];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	BOOL ret = FALSE;
	wchar_t buf[80];
	wchar_t uimsg[MAX_UIMSG];
	wchar_t uimsg2[MAX_UIMSG];

	SetLastError(NO_ERROR);

	_snprintf_s(fullpath, sizeof(fullpath), "%s\\%s", pathname, filename);
	if (_access(fullpath, 0) != 0) { // ファイルが存在しない
		DWORD no = GetLastError();
		get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
		get_lang_msgW("DLG_SETUPDIR_NOFILE_ERROR", uimsg2, _countof(uimsg2),
					  L"File does not exist.(%d)", ts.UILanguageFile);
		_snwprintf_s(buf, _countof(buf), _TRUNCATE, uimsg2, no);
		_MessageBoxW(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
		goto error;
	}

	_snprintf_s(command, sizeof(command), _TRUNCATE, "%s \"%s\"", editor, fullpath);

	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(NULL, command, NULL, NULL, FALSE, 0,
		NULL, NULL, &si, &pi) == 0) { // 起動失敗
		DWORD no = GetLastError();
		get_lang_msgW("MSG_ERROR", uimsg, _countof(uimsg), L"ERROR", ts.UILanguageFile);
		get_lang_msgW("DLG_SETUPDIR_OPENFILE_ERROR", uimsg2, _countof(uimsg2),
					  L"Cannot open file.(%d)", ts.UILanguageFile);
		_snwprintf_s(buf, _countof(buf), _TRUNCATE, uimsg2, no);
		_MessageBoxW(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
		goto error;
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	ret = TRUE;

error:;
	return (ret);
}


//
// エクスプローラでパスを開く。
//
// return TRUE: success
//        FALSE: failure
//
static BOOL openDirectoryWithExplorer(const wchar_t *path)
{
	LPSHELLFOLDER pDesktopFolder;
	LPMALLOC pMalloc;
	LPITEMIDLIST pIDL;
	SHELLEXECUTEINFO si;
	BOOL ret = FALSE;

	if (SHGetDesktopFolder(&pDesktopFolder) == S_OK) {
		if (SHGetMalloc(&pMalloc) == S_OK) {
			if (pDesktopFolder->ParseDisplayName(NULL, NULL, (LPWSTR)path, NULL, &pIDL, NULL) == S_OK) {
				::ZeroMemory(&si, sizeof(si));
				si.cbSize = sizeof(si);
				si.fMask = SEE_MASK_IDLIST;
				si.lpVerb = _T("open");
				si.lpIDList = pIDL;
				si.nShow = SW_SHOWNORMAL;
				::ShellExecuteEx(&si);
				pMalloc->Free((void *)pIDL);

				ret = TRUE;
			}

			pMalloc->Release();
		}
		pDesktopFolder->Release();
	}

	return (ret);
}

//
// フォルダもしくはファイルを開く。
//
static void openFileDirectory(char *path, char *filename, BOOL open_directory_only, char *open_editor)
{
	if (open_directory_only) {
		wchar_t *pathW = ToWcharA(path);
		openDirectoryWithExplorer(pathW);
		free(pathW);
	}
	else {
		openFileWithApplication(path, filename, open_editor);
	}
}


//
// Virtual Storeパスに変換する。
//
// path: IN
// filename: IN
// vstore_path: OUT
// vstore_pathlen: IN
//
// return TRUE: success
//        FALSE: failure
//
static BOOL convertVirtualStore(char *path, char *filename, char *vstore_path, int vstore_pathlen)
{
	BOOL ret = FALSE;
	const char *s, **p;
	const char *virstore_env[] = {
		"ProgramFiles",
		"ProgramData",
		"SystemRoot",
		NULL
	};
	char shPath[1024] = "";
	char shFullPath[1024] = "";
	LPITEMIDLIST pidl;
	int CSIDL;

	OutputDebugPrintf("[%s][%s]\n", path, filename);

	if (cv.VirtualStoreEnabled == FALSE)
		goto error;

	// Virtual Store対象となるフォルダか。
	p = virstore_env;
	while (*p) {
		s = getenv(*p);
		if (s != NULL && strstr(path, s) != NULL) {
			break;
		}
		p++;
	}
	if (*p == NULL)
		goto error;

	CSIDL = CSIDL_LOCAL_APPDATA;
	if (SHGetSpecialFolderLocation(NULL, CSIDL, &pidl) != S_OK) {
		goto error;
	}
	SHGetPathFromIDList(pidl, shPath);
	CoTaskMemFree(pidl);

	// Virtual Storeパスを作る。
	strncat_s(shPath, sizeof(shPath), "\\VirtualStore", _TRUNCATE);

	// 不要なドライブレターを除去する。
	// ドライブレターは一文字とは限らない点に注意。
	s = strstr(path, ":\\");
	if (s != NULL) {
		strncat_s(shPath, sizeof(shPath), s + 1, _TRUNCATE);
	}

	// 最後に、Virtual Storeにファイルがあるかどうかを調べる。
	_snprintf_s(shFullPath, sizeof(shFullPath), "%s\\%s", shPath, filename);
	if (_access(shFullPath, 0) != 0) {
		goto error;
	}

	strncpy_s(vstore_path, vstore_pathlen, shPath, _TRUNCATE);

	ret = TRUE;
	return (ret);

error:
	return (ret);
}



static LRESULT CALLBACK OnSetupDirectoryDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_SETUPDIR_TITLE" },
		{ IDC_INI_SETUPDIR_GROUP, "DLG_SETUPDIR_INIFILE" },
		{ IDC_KEYCNF_SETUPDIR_GROUP, "DLG_SETUPDIR_KEYBOARDFILE" },
		{ IDC_CYGTERM_SETUPDIR_GROUP, "DLG_SETUPDIR_CYGTERMFILE" },
		{ IDC_SSH_SETUPDIR_GROUP, "DLG_SETUPDIR_KNOWNHOSTSFILE" },
	};
	static char teratermexepath[MAX_PATH];
	static char inipath[MAX_PATH], inifilename[MAX_PATH], inipath_vstore[1024];
	static char keycnfpath[MAX_PATH], keycnffilename[MAX_PATH], keycnfpath_vstore[1024];
	static char cygtermpath[MAX_PATH], cygtermfilename[MAX_PATH], cygtermpath_vstore[1024];
//	static char eterm1path[MAX_PATH], eterm1filename[MAX_PATH], eterm1path_vstore[1024];
	char temp[MAX_PATH];
	char tmpbuf[1024];
	typedef int (CALLBACK *PSSH_read_known_hosts_file)(char *, int);
	PSSH_read_known_hosts_file func = NULL;
	HMODULE h = NULL;
	static char hostsfilepath[MAX_PATH], hostsfilename[MAX_PATH], hostsfilepath_vstore[1024];
	char *path_p, *filename_p;
	BOOL open_dir, ret;
	int button_pressed;
	HWND hWnd;

	switch (msg) {
	case WM_INITDIALOG:
		// I18N
		SetDlgTexts(hDlgWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

		if (GetModuleFileNameA(NULL, temp, sizeof(temp)) != 0) {
			ExtractDirName(temp, teratermexepath);
		}

		// 設定ファイル(teraterm.ini)のパスを取得する。
		/// (1)
		ExtractFileName(ts.SetupFName, inifilename, sizeof(inifilename));
		ExtractDirName(ts.SetupFName, inipath);
		//SetDlgItemText(hDlgWnd, IDC_INI_SETUPDIR_STATIC, inifilename);
		SetDlgItemText(hDlgWnd, IDC_INI_SETUPDIR_EDIT, ts.SetupFName);
		/// (2) Virutal Storeへの変換
		memset(inipath_vstore, 0, sizeof(inipath_vstore));
		ret = convertVirtualStore(inipath, inifilename, inipath_vstore, sizeof(inipath_vstore));
		if (ret) {
			hWnd = GetDlgItem(hDlgWnd, IDC_INI_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, TRUE);
			hWnd = GetDlgItem(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, TRUE);
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, "%s\\%s", inipath_vstore, inifilename);
			SetDlgItemText(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE, tmpbuf);
		}
		else {
			hWnd = GetDlgItem(hDlgWnd, IDC_INI_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, FALSE);
			hWnd = GetDlgItem(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, FALSE);
			SetDlgItemText(hDlgWnd, IDC_INI_SETUPDIR_EDIT_VSTORE, "");
		}

		// 設定ファイル(KEYBOARD.CNF)のパスを取得する。
		/// (1)
		ExtractFileName(ts.KeyCnfFN, keycnffilename, sizeof(keycnfpath));
		ExtractDirName(ts.KeyCnfFN, keycnfpath);
		//SetDlgItemText(hDlgWnd, IDC_KEYCNF_SETUPDIR_STATIC, keycnffilename);
		SetDlgItemText(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT, ts.KeyCnfFN);
		/// (2) Virutal Storeへの変換
		memset(keycnfpath_vstore, 0, sizeof(keycnfpath_vstore));
		ret = convertVirtualStore(keycnfpath, keycnffilename, keycnfpath_vstore, sizeof(keycnfpath_vstore));
		if (ret) {
			hWnd = GetDlgItem(hDlgWnd, IDC_KEYCNF_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, TRUE);
			hWnd = GetDlgItem(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, TRUE);
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, "%s\\%s", keycnfpath_vstore, keycnffilename);
			SetDlgItemText(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE, tmpbuf);
		}
		else {
			hWnd = GetDlgItem(hDlgWnd, IDC_KEYCNF_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, FALSE);
			hWnd = GetDlgItem(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, FALSE);
			SetDlgItemText(hDlgWnd, IDC_KEYCNF_SETUPDIR_EDIT_VSTORE, "");
		}


		// cygterm.cfg は ttermpro.exe 配下に位置する。
		/// (1)
		strncpy_s(cygtermfilename, sizeof(cygtermfilename), "cygterm.cfg", _TRUNCATE);
		strncpy_s(cygtermpath, sizeof(cygtermpath), teratermexepath, _TRUNCATE);
		//SetDlgItemText(hDlgWnd, IDC_CYGTERM_SETUPDIR_STATIC, cygtermfilename);
		_snprintf_s(temp, sizeof(temp), "%s\\%s", cygtermpath, cygtermfilename);
		SetDlgItemText(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT, temp);
		/// (2) Virutal Storeへの変換
		memset(cygtermpath_vstore, 0, sizeof(cygtermpath_vstore));
		ret = convertVirtualStore(cygtermpath, cygtermfilename, cygtermpath_vstore, sizeof(cygtermpath_vstore));
		if (ret) {
			hWnd = GetDlgItem(hDlgWnd, IDC_CYGTERM_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, TRUE);
			hWnd = GetDlgItem(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, TRUE);
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, "%s\\%s", cygtermpath_vstore, cygtermfilename);
			SetDlgItemText(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE, tmpbuf);
		}
		else {
			hWnd = GetDlgItem(hDlgWnd, IDC_CYGTERM_SETUPDIR_STATIC_VSTORE);
			EnableWindow(hWnd, FALSE);
			hWnd = GetDlgItem(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE);
			EnableWindow(hWnd, FALSE);
			SetDlgItemText(hDlgWnd, IDC_CYGTERM_SETUPDIR_EDIT_VSTORE, "");
		}

		// ssh_known_hosts
		if (func == NULL) {
			if (((h = GetModuleHandle("ttxssh.dll")) != NULL)) {
				func = (PSSH_read_known_hosts_file)GetProcAddress(h, "TTXReadKnownHostsFile");
				if (func) {
					int ret = func(temp, sizeof(temp));
					if (ret) {
						char *s = strstr(temp, ":\\");

						if (s) { // full path
							ExtractFileName(temp, hostsfilename, sizeof(hostsfilename));
							ExtractDirName(temp, hostsfilepath);
						}
						else { // relative path
							strncpy_s(hostsfilepath, sizeof(hostsfilepath), teratermexepath, _TRUNCATE);
							strncpy_s(hostsfilename, sizeof(hostsfilename), temp, _TRUNCATE);
							_snprintf_s(temp, sizeof(temp), "%s\\%s", hostsfilepath, hostsfilename);
						}

						SetDlgItemText(hDlgWnd, IDC_SSH_SETUPDIR_EDIT, temp);

						/// (2) Virutal Storeへの変換
						memset(hostsfilepath_vstore, 0, sizeof(hostsfilepath_vstore));
						ret = convertVirtualStore(hostsfilepath, hostsfilename, hostsfilepath_vstore, sizeof(hostsfilepath_vstore));
						if (ret) {
							hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_STATIC_VSTORE);
							EnableWindow(hWnd, TRUE);
							hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE);
							EnableWindow(hWnd, TRUE);
							_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, "%s\\%s", hostsfilepath_vstore, hostsfilename);
							SetDlgItemText(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE, tmpbuf);
						}
						else {
							hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_STATIC_VSTORE);
							EnableWindow(hWnd, FALSE);
							hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE);
							EnableWindow(hWnd, FALSE);
							SetDlgItemText(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE, "");
						}

					}
				}
			}
			else {
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT);
				EnableWindow(hWnd, FALSE);
				SetDlgItemText(hDlgWnd, IDC_SSH_SETUPDIR_EDIT, "");
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_BUTTON);
				EnableWindow(hWnd, FALSE);
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_BUTTON_FILE);
				EnableWindow(hWnd, FALSE);
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_STATIC_VSTORE);
				EnableWindow(hWnd, FALSE);
				hWnd = GetDlgItem(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE);
				EnableWindow(hWnd, FALSE);
				SetDlgItemText(hDlgWnd, IDC_SSH_SETUPDIR_EDIT_VSTORE, "");
			}
		}

		return TRUE;

	case WM_COMMAND:
		button_pressed = 0;
		switch (LOWORD(wp)) {
		case IDC_INI_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			open_dir = TRUE;
			path_p = inipath;
			if (inipath_vstore[0])
				path_p = inipath_vstore;
			filename_p = inifilename;
			button_pressed = 1;
			break;
		case IDC_INI_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			open_dir = FALSE;
			path_p = inipath;
			if (inipath_vstore[0])
				path_p = inipath_vstore;
			filename_p = inifilename;
			button_pressed = 1;
			break;

		case IDC_KEYCNF_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			open_dir = TRUE;
			path_p = keycnfpath;
			if (keycnfpath_vstore[0])
				path_p = keycnfpath_vstore;
			filename_p = keycnffilename;
			button_pressed = 1;
			break;
		case IDC_KEYCNF_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			open_dir = FALSE;
			path_p = keycnfpath;
			if (keycnfpath_vstore[0])
				path_p = keycnfpath_vstore;
			filename_p = keycnffilename;
			button_pressed = 1;
			break;

		case IDC_CYGTERM_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			open_dir = TRUE;
			path_p = cygtermpath;
			if (cygtermpath_vstore[0])
				path_p = cygtermpath_vstore;
			filename_p = cygtermfilename;
			button_pressed = 1;
			break;
		case IDC_CYGTERM_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			open_dir = FALSE;
			path_p = cygtermpath;
			if (cygtermpath_vstore[0])
				path_p = cygtermpath_vstore;
			filename_p = cygtermfilename;
			button_pressed = 1;
			break;

		case IDC_SSH_SETUPDIR_BUTTON | (BN_CLICKED << 16) :
			open_dir = TRUE;
			path_p = hostsfilepath;
			if (hostsfilepath_vstore[0])
				path_p = hostsfilepath_vstore;
			filename_p = hostsfilename;
			button_pressed = 1;
			break;
		case IDC_SSH_SETUPDIR_BUTTON_FILE | (BN_CLICKED << 16) :
			open_dir = FALSE;
			path_p = hostsfilepath;
			if (hostsfilepath_vstore[0])
				path_p = hostsfilepath_vstore;
			filename_p = hostsfilename;
			button_pressed = 1;
			break;

		case IDCANCEL:
			TTEndDialog(hDlgWnd, IDCANCEL);
			return TRUE;
			break;

		default:
			return FALSE;
		}

		if (button_pressed) {
			char *app = NULL;

			if (open_dir)
				app = NULL;
			else
				app = ts.ViewlogEditor;

			openFileDirectory(path_p, filename_p, open_dir, app);
			return TRUE;
		}
		return FALSE;

	case WM_CLOSE:
		TTEndDialog(hDlgWnd, 0);
		return TRUE;

	default:
		return FALSE;
	}
	return TRUE;
}

//
// 現在読み込まれている teraterm.ini ファイルが格納されている
// フォルダをエクスプローラで開く。
//
// (2015.2.28 yutaka)
//
void CVTWindow::OnOpenSetupDirectory()
{
	TTDialogBox(m_hInst, MAKEINTRESOURCE(IDD_SETUP_DIR_DIALOG),
	            HVTWin, (DLGPROC)OnSetupDirectoryDlgProc);
}

void CVTWindow::OnSetupLoadKeyMap()
{
	BOOL Ok;

	HelpId = HlpSetupLoadKeyMap;
	if (! LoadTTFILE()) {
		return;
	}
	Ok = (*GetSetupFname)(HVTWin,GSF_LOADKEY,&ts);
	FreeTTFILE();
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

void ApplyBroadCastCommandHisotry(HWND Dialog, char *historyfile)
{
	char EntName[13];
	char Command[HostNameMaxLength+1];
	int i = 1;

	SendDlgItemMessage(Dialog, IDC_COMMAND_EDIT, CB_RESETCONTENT, 0, 0);
	do {
		_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "Command%d", i);
		GetPrivateProfileString("BroadcastCommands",EntName,"",
		                        Command,sizeof(Command), historyfile);
		if (strlen(Command) > 0) {
			SendDlgItemMessage(Dialog, IDC_COMMAND_EDIT, CB_ADDSTRING,
			                   0, (LPARAM)Command);
		}
		i++;
	} while ((i <= ts.MaxBroadcatHistory) && (strlen(Command)>0));

	SendDlgItemMessage(Dialog, IDC_COMMAND_EDIT, EM_LIMITTEXT,
	                   HostNameMaxLength-1, 0);

	SendDlgItemMessage(Dialog, IDC_COMMAND_EDIT, CB_SETCURSEL,0,0);
}

// ドロップダウンの中のエディットコントロールを
// サブクラス化するためのウインドウプロシージャ
static WNDPROC OrigBroadcastEditProc; // Original window procedure
static HWND BroadcastWindowList;
static LRESULT CALLBACK BroadcastEditProc(HWND dlg, UINT msg,
                                          WPARAM wParam, LPARAM lParam)
{
	char buf[1024];
	int len;

	switch (msg) {
		case WM_CREATE:
			break;

		case WM_DESTROY:
			break;

		case WM_LBUTTONUP:
			// すでにテキストが入力されている場合は、カーソルを末尾へ移動させる。
			len = GetWindowText(dlg, buf, sizeof(buf));
			SendMessage(dlg, EM_SETSEL, len, len);
			SetFocus(dlg);
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			SetFocus(dlg);
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			{
				int i;
				HWND hd;
				int count;

				if (wParam == 0x0d) {  // Enter key
					SetWindowText(dlg, "");
					SendMessage(dlg, EM_SETSEL, 0, 0);
				}

				count = SendMessage(BroadcastWindowList, LB_GETCOUNT, 0, 0);
				for (i = 0 ; i < count ; i++) {
					if (SendMessage(BroadcastWindowList, LB_GETSEL, i, 0)) {
						hd = GetNthWin(i);
						if (hd) {
							PostMessage(hd, msg, wParam, lParam);
						}
					}
				}
			}
			break;

		case WM_CHAR:
			// 入力した文字がIDC_COMMAND_EDITに残らないように捨てる
			return FALSE;

		default:
			return CallWindowProc(OrigBroadcastEditProc, dlg, msg, wParam, lParam);
	}

	return FALSE;
}

static void UpdateBroadcastWindowList(HWND hWnd)
{
	int i, count;
	HWND hd;
	TCHAR szWindowText[256];

	SendMessage(hWnd, LB_RESETCONTENT, 0, 0);

	count = GetRegisteredWindowCount();
	for (i = 0 ; i < count ; i++) {
		hd = GetNthWin(i);
		if (hd == NULL) {
			break;
		}

		GetWindowText(hd, szWindowText, 256);
		SendMessage(hWnd, LB_INSERTSTRING, -1, (LPARAM)szWindowText);
	}
}

/*
 * ダイアログで選択されたウィンドウのみ、もしくは親ウィンドウのみに送るブロードキャストモード。
 * リアルタイムモードが off の時に利用される。
 */
void SendBroadcastMessageToSelected(HWND HVTWin, HWND hWnd, int parent_only, char *buf, int buflen)
{
	int i;
	int count;
	HWND hd;
	COPYDATASTRUCT cds;

	ZeroMemory(&cds, sizeof(cds));
	cds.dwData = IPC_BROADCAST_COMMAND;
	cds.cbData = buflen;
	cds.lpData = buf;

	if (parent_only) {
		// 親ウィンドウのみに WM_COPYDATA メッセージを送る
		SendMessage(GetParent(hWnd), WM_COPYDATA, (WPARAM)HVTWin, (LPARAM)&cds);
	}
	else {
		// ダイアログで選択されたウィンドウにメッセージを送る
		count = SendMessage(BroadcastWindowList, LB_GETCOUNT, 0, 0);
		for (i = 0 ; i < count ; i++) {
			// リストボックスで選択されているか
			if (SendMessage(BroadcastWindowList, LB_GETSEL, i, 0)) {
				if ((hd = GetNthWin(i)) != NULL) {
					// WM_COPYDATAを使って、プロセス間通信を行う。
					SendMessage(hd, WM_COPYDATA, (WPARAM)HVTWin, (LPARAM)&cds);
				}
			}
		}
	}
}

/*
 * 全 Tera Term へメッセージを送信するブロードキャストモード。
 * "sendbroadcast"マクロコマンドからのみ利用される。
 */
void SendBroadcastMessage(HWND HVTWin, HWND hWnd, char *buf, int buflen)
{
	int i, count;
	HWND hd;
	COPYDATASTRUCT cds;

	ZeroMemory(&cds, sizeof(cds));
	cds.dwData = IPC_BROADCAST_COMMAND;
	cds.cbData = buflen;
	cds.lpData = buf;

	count = GetRegisteredWindowCount();

	// 全 Tera Term へメッセージを送る。
	for (i = 0 ; i < count ; i++) {
		if ((hd = GetNthWin(i)) == NULL) {
			break;
		}
		// WM_COPYDATAを使って、プロセス間通信を行う。
		SendMessage(hd, WM_COPYDATA, (WPARAM)HVTWin, (LPARAM)&cds);
	}
}


/*
 * 任意の Tera Term 群へメッセージを送信するマルチキャストモード。厳密には、
 * ブロードキャスト送信を行い、受信側でメッセージを取捨選択する。
 * "sendmulticast"マクロコマンドからのみ利用される。
 */
void SendMulticastMessage(HWND hVTWin_, HWND hWnd, char *name, char *buf, int buflen)
{
	int i, count;
	HWND hd;
	COPYDATASTRUCT cds;
	char *msg = NULL;
	int msglen, nlen;

	/* 送信メッセージを構築する。
	 *
	 * msg
	 * +------+--------------+--+
	 * |name\0|buf           |\0|
	 * +------+--------------+--+
	 * <--------------------->
	 * msglen = strlen(name) + 1 + buflen
	 * bufの直後には \0 は付かない。
	 */
	nlen = strlen(name) + 1;
	msglen = nlen + buflen;
	if ((msg = (char *)malloc(msglen)) == NULL) {
		return;
	}
	strcpy_s(msg, msglen, name);
	memcpy_s(msg + nlen, msglen - nlen, buf, buflen);

	ZeroMemory(&cds, sizeof(cds));
	cds.dwData = IPC_MULTICAST_COMMAND;
	cds.cbData = msglen;
	cds.lpData = msg;

	count = GetRegisteredWindowCount();

	// すべてのTera Termにメッセージとデータを送る
	for (i = 0 ; i < count ; i++) {
		if ((hd = GetNthWin(i)) == NULL) {
			break;
		}

		// WM_COPYDATAを使って、プロセス間通信を行う。
		SendMessage(hd, WM_COPYDATA, (WPARAM)hVTWin_, (LPARAM)&cds);
	}

	free(msg);
}

void SetMulticastName(char *name)
{
	strncpy_s(ts.MulticastName, sizeof(ts.MulticastName), name, _TRUNCATE);
}

static int CompareMulticastName(char *name)
{
	return strcmp(ts.MulticastName, name);
}

//
// すべてのターミナルへ同一コマンドを送信するモードレスダイアログの表示
// (2005.1.22 yutaka)
//
static LRESULT CALLBACK BroadcastCommandDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_BROADCAST_TITLE" },
		{ IDC_HISTORY_CHECK, "DLG_BROADCAST_HISTORY" },
		{ IDC_ENTERKEY_CHECK, "DLG_BROADCAST_ENTER" },
		{ IDC_PARENT_ONLY, "DLG_BROADCAST_PARENTONLY" },
		{ IDC_REALTIME_CHECK, "DLG_BROADCAST_REALTIME" },
		{ IDOK, "DLG_BROADCAST_SUBMIT" },
		{ IDCANCEL, "BTN_CLOSE" },
	};
	char buf[256 + 3];
	UINT ret;
	LRESULT checked;
	LRESULT history;
	char historyfile[MAX_PATH];
	static HWND hwndBroadcast     = NULL; // Broadcast dropdown
	static HWND hwndBroadcastEdit = NULL; // Edit control on Broadcast dropdown
	// for resize
	RECT rc_dlg, rc, rc_ok;
	POINT p;
	static int ok2right, cancel2right, cmdlist2ok, list2bottom, list2right;
	// for update list
	const int list_timer_id = 100;
	const int list_timer_tick = 1000; // msec
	static int prev_instances = 0;
	// for status bar
	static HWND hStatus = NULL;
	static int init_width, init_height;

	switch (msg) {
		case WM_SHOWWINDOW:
			if (wp) {  // show
				// Tera Term window list
				UpdateBroadcastWindowList(GetDlgItem(hWnd, IDC_LIST));
				return TRUE;
			}
			break;

		case WM_INITDIALOG:
			// ラジオボタンのデフォルトは CR にする。
			SendMessage(GetDlgItem(hWnd, IDC_RADIO_CR), BM_SETCHECK, BST_CHECKED, 0);
			// デフォルトでチェックボックスを checked 状態にする。
			SendMessage(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), BM_SETCHECK, BST_CHECKED, 0);
			// history を反映する (2007.3.3 maya)
			if (ts.BroadcastCommandHistory) {
				SendMessage(GetDlgItem(hWnd, IDC_HISTORY_CHECK), BM_SETCHECK, BST_CHECKED, 0);
			}
			GetDefaultFName(ts.HomeDir, BROADCAST_LOGFILE, historyfile, sizeof(historyfile));
			ApplyBroadCastCommandHisotry(hWnd, historyfile);

			// エディットコントロールにフォーカスをあてる
			SetFocus(GetDlgItem(hWnd, IDC_COMMAND_EDIT));

			// サブクラス化させてリアルタイムモードにする (2008.1.21 yutaka)
			hwndBroadcast = GetDlgItem(hWnd, IDC_COMMAND_EDIT);
			hwndBroadcastEdit = GetWindow(hwndBroadcast, GW_CHILD);
			OrigBroadcastEditProc = (WNDPROC)GetWindowLongPtr(hwndBroadcastEdit, GWLP_WNDPROC);
			SetWindowLongPtr(hwndBroadcastEdit, GWLP_WNDPROC, (LONG_PTR)BroadcastEditProc);
			// デフォルトはon。残りはdisable。
			SendMessage(GetDlgItem(hWnd, IDC_REALTIME_CHECK), BM_SETCHECK, BST_CHECKED, 0);  // default on
			EnableWindow(GetDlgItem(hWnd, IDC_HISTORY_CHECK), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_PARENT_ONLY), FALSE);

			// Tera Term window list
			BroadcastWindowList = GetDlgItem(hWnd, IDC_LIST);
			UpdateBroadcastWindowList(BroadcastWindowList);

			// I18N
			SetDlgTexts(hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

			// ダイアログの初期サイズを保存
			GetWindowRect(hWnd, &rc_dlg);
			init_width = rc_dlg.right - rc_dlg.left;
			init_height = rc_dlg.bottom - rc_dlg.top;

			// 現在サイズから必要な値を計算
			GetClientRect(hWnd, &rc_dlg);
			p.x = rc_dlg.right;
			p.y = rc_dlg.bottom;
			ClientToScreen(hWnd, &p);

			GetWindowRect(GetDlgItem(hWnd, IDOK), &rc_ok);
			ok2right = p.x - rc_ok.left;

			GetWindowRect(GetDlgItem(hWnd, IDCANCEL), &rc);
			cancel2right = p.x - rc.left;

			GetWindowRect(GetDlgItem(hWnd, IDC_COMMAND_EDIT), &rc);
			cmdlist2ok = rc_ok.left - rc.right;

			GetWindowRect(GetDlgItem(hWnd, IDC_LIST), &rc);
			list2bottom = p.y - rc.bottom;
			list2right = p.x - rc.right;

			// リサイズアイコンを右下に表示させたいので、ステータスバーを付ける。
			InitCommonControls();
			hStatus = CreateStatusWindow(
				WS_CHILD | WS_VISIBLE |
				CCS_BOTTOM | SBARS_SIZEGRIP, NULL, hWnd, 1);

			// リスト更新タイマーの開始
			SetTimer(hWnd, list_timer_id, list_timer_tick, NULL);

			return FALSE;

		case WM_COMMAND:
			switch (wp) {
			case IDC_ENTERKEY_CHECK | (BN_CLICKED << 16):
				// チェックの有無により、ラジオボタンの有効・無効を決める。
				checked = SendMessage(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), BM_GETCHECK, 0, 0);
				if (checked & BST_CHECKED) { // 改行コードあり
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), TRUE);

				} else {
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), FALSE);
				}
				return TRUE;

			case IDC_REALTIME_CHECK | (BN_CLICKED << 16):
				checked = SendMessage(GetDlgItem(hWnd, IDC_REALTIME_CHECK), BM_GETCHECK, 0, 0);
				if (checked & BST_CHECKED) { // checkあり
					// new handler
					hwndBroadcast = GetDlgItem(hWnd, IDC_COMMAND_EDIT);
					hwndBroadcastEdit = GetWindow(hwndBroadcast, GW_CHILD);
					OrigBroadcastEditProc = (WNDPROC)GetWindowLongPtr(hwndBroadcastEdit, GWLP_WNDPROC);
					SetWindowLongPtr(hwndBroadcastEdit, GWLP_WNDPROC, (LONG_PTR)BroadcastEditProc);

					EnableWindow(GetDlgItem(hWnd, IDC_HISTORY_CHECK), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_PARENT_ONLY), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_LIST), TRUE);  // true
				} else {
					// restore old handler
					SetWindowLongPtr(hwndBroadcastEdit, GWLP_WNDPROC, (LONG_PTR)OrigBroadcastEditProc);

					EnableWindow(GetDlgItem(hWnd, IDC_HISTORY_CHECK), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_PARENT_ONLY), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_LIST), TRUE);  // true
				}
				return TRUE;
			}

			switch (LOWORD(wp)) {
				case IDOK:
					{
						memset(buf, 0, sizeof(buf));

						// realtime modeの場合、Enter keyのみ送る。
						// cf. http://logmett.com/forum/viewtopic.php?f=8&t=1601
						// (2011.3.14 hirata)
						checked = SendMessage(GetDlgItem(hWnd, IDC_REALTIME_CHECK), BM_GETCHECK, 0, 0);
						if (checked & BST_CHECKED) { // checkあり
							strncpy_s(buf, sizeof(buf), "\n", _TRUNCATE);
							SetDlgItemText(hWnd, IDC_COMMAND_EDIT, "");
							goto skip;
						}

						ret = GetDlgItemText(hWnd, IDC_COMMAND_EDIT, buf, 256 - 1);
						if (ret == 0) { // error
							memset(buf, 0, sizeof(buf));
						}

						// ブロードキャストコマンドの履歴を保存 (2007.3.3 maya)
						history = SendMessage(GetDlgItem(hWnd, IDC_HISTORY_CHECK), BM_GETCHECK, 0, 0);
						if (history) {
							GetDefaultFName(ts.HomeDir, BROADCAST_LOGFILE, historyfile, sizeof(historyfile));
							if (LoadTTSET()) {
								(*AddValueToList)(historyfile, buf, "BroadcastCommands", "Command",
								                  ts.MaxBroadcatHistory);
								FreeTTSET();
							}
							ApplyBroadCastCommandHisotry(hWnd, historyfile);
							ts.BroadcastCommandHistory = TRUE;
						}
						else {
							ts.BroadcastCommandHistory = FALSE;
						}
						checked = SendMessage(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), BM_GETCHECK, 0, 0);
						if (checked & BST_CHECKED) { // 改行コードあり
							if (SendMessage(GetDlgItem(hWnd, IDC_RADIO_CRLF), BM_GETCHECK, 0, 0) & BST_CHECKED) {
								strncat_s(buf, sizeof(buf), "\r\n", _TRUNCATE);

							} else if (SendMessage(GetDlgItem(hWnd, IDC_RADIO_CR), BM_GETCHECK, 0, 0) & BST_CHECKED) {
								strncat_s(buf, sizeof(buf), "\r", _TRUNCATE);

							} else if (SendMessage(GetDlgItem(hWnd, IDC_RADIO_LF), BM_GETCHECK, 0, 0) & BST_CHECKED) {
								strncat_s(buf, sizeof(buf), "\n", _TRUNCATE);

							} else {
								strncat_s(buf, sizeof(buf), "\r", _TRUNCATE);

							}
						}

skip:;
						// 337: 2007/03/20 チェックされていたら親ウィンドウにのみ送信
						checked = SendMessage(GetDlgItem(hWnd, IDC_PARENT_ONLY), BM_GETCHECK, 0, 0);

						SendBroadcastMessageToSelected(HVTWin, hWnd, checked, buf, strlen(buf));
					}

					// モードレスダイアログは一度生成されると、アプリケーションが終了するまで
					// 破棄されないので、以下の「ウィンドウプロシージャ戻し」は不要と思われる。(yutaka)
#if 0
					SetWindowLongPtr(hwndBroadcastEdit, GWLP_WNDPROC, (LONG_PTR)OrigBroadcastEditProc);
#endif

					//EndDialog(hDlgWnd, IDOK);
					return TRUE;

				case IDCANCEL:
					EndDialog(hWnd, 0);
					//DestroyWindow(hWnd);

					return TRUE;

				case IDC_COMMAND_EDIT:
					if (HIWORD(wp) == CBN_DROPDOWN) {
						GetDefaultFName(ts.HomeDir, BROADCAST_LOGFILE, historyfile, sizeof(historyfile));
						ApplyBroadCastCommandHisotry(hWnd, historyfile);
					}
					return FALSE;

				case IDC_LIST:
					// 一般的なアプリケーションと同じ操作感を持たせるため、
					// 「SHIFT+クリック」による連続的な選択をサポートする。
					// (2009.9.28 yutaka)
					if (HIWORD(wp) == LBN_SELCHANGE && ShiftKey()) {
						int i, cur, prev;

						cur = ListBox_GetCurSel(BroadcastWindowList);
						prev = -1;
						for (i = cur - 1 ; i >= 0 ; i--) {
							if (ListBox_GetSel(BroadcastWindowList, i)) {
								prev = i;
								break;
							}
						}
						if (prev != -1) {
							// すでに選択済みの箇所があれば、そこから連続選択する。
							for (i = prev ; i < cur ; i++) {
								ListBox_SetSel(BroadcastWindowList, TRUE, i);
							}
						}
					}

					return FALSE;

				default:
					return FALSE;
			}
			break;

		case WM_CLOSE:
			//DestroyWindow(hWnd);
			EndDialog(hWnd, 0);
			return TRUE;

		case WM_SIZE:
			{
				// 再配置
				int dlg_w, dlg_h;
				RECT rc_dlg;
				RECT rc;
				POINT p;

				// 新しいダイアログのサイズを得る
				GetClientRect(hWnd, &rc_dlg);
				dlg_w = rc_dlg.right;
				dlg_h = rc_dlg.bottom;

				// OK button
				GetWindowRect(GetDlgItem(hWnd, IDOK), &rc);
				p.x = rc.left;
				p.y = rc.top;
				ScreenToClient(hWnd, &p);
				SetWindowPos(GetDlgItem(hWnd, IDOK), 0,
				             dlg_w - ok2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// Cancel button
				GetWindowRect(GetDlgItem(hWnd, IDCANCEL), &rc);
				p.x = rc.left;
				p.y = rc.top;
				ScreenToClient(hWnd, &p);
				SetWindowPos(GetDlgItem(hWnd, IDCANCEL), 0,
				             dlg_w - cancel2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// Command Edit box
				GetWindowRect(GetDlgItem(hWnd, IDC_COMMAND_EDIT), &rc);
				p.x = rc.left;
				p.y = rc.top;
				ScreenToClient(hWnd, &p);
				SetWindowPos(GetDlgItem(hWnd, IDC_COMMAND_EDIT), 0,
				             0, 0, dlg_w - p.x - ok2right - cmdlist2ok, p.y,
				             SWP_NOMOVE | SWP_NOZORDER);

				// List Edit box
				GetWindowRect(GetDlgItem(hWnd, IDC_LIST), &rc);
				p.x = rc.left;
				p.y = rc.top;
				ScreenToClient(hWnd, &p);
				SetWindowPos(GetDlgItem(hWnd, IDC_LIST), 0,
				             0, 0, dlg_w - p.x - list2right , dlg_h - p.y - list2bottom,
				             SWP_NOMOVE | SWP_NOZORDER);

				// status bar
				SendMessage(hStatus , msg , wp , lp);
			}
			return TRUE;

		case WM_GETMINMAXINFO:
			{
				// ダイアログの初期サイズより小さくできないようにする
				LPMINMAXINFO lpmmi;
				lpmmi = (LPMINMAXINFO)lp;
				lpmmi->ptMinTrackSize.x = init_width;
				lpmmi->ptMinTrackSize.y = init_height;
			}
			return FALSE;

		case WM_TIMER:
			{
				int n;

				if (wp != list_timer_id)
					break;

				n = GetRegisteredWindowCount();
				if (n != prev_instances) {
					prev_instances = n;
					UpdateBroadcastWindowList(BroadcastWindowList);
				}
			}
			return TRUE;

		case WM_VKEYTOITEM:
			// リストボックスでキー押下(CTRL+A)されたら、全選択。
			if ((HWND)lp == BroadcastWindowList) {
				if (ControlKey() && LOWORD(wp) == 'A') {
					int i, n;

					//OutputDebugPrintf("msg %x wp %x lp %x\n", msg, wp, lp);
					n = GetRegisteredWindowCount();
					for (i = 0 ; i < n ; i++) {
						ListBox_SetSel(BroadcastWindowList, TRUE, i);
					}
				}
			}
			return TRUE;

		default:
			//OutputDebugPrintf("msg %x wp %x lp %x\n", msg, wp, lp);
			return FALSE;
	}
	return TRUE;
}

void CVTWindow::OnControlBroadcastCommand(void)
{
	// TODO: モードレスダイアログのハンドルは、親プロセスが DestroyWindow() APIで破棄する
	// 必要があるが、ここはOS任せとする。
	static HWND hDlgWnd = NULL;
	RECT prc, rc;
	LONG x, y;

	if (hDlgWnd != NULL) {
		goto activate;
	}

	hDlgWnd = TTCreateDialog(m_hInst, MAKEINTRESOURCE(IDD_BROADCAST_DIALOG),
							 HVTWin, (DLGPROC)BroadcastCommandDlgProc);

	if (hDlgWnd == NULL) {
		return;
	}

	// ダイアログをウィンドウの真上に配置する (2008.1.25 yutaka)
	GetWindowRect(&prc);
	::GetWindowRect(hDlgWnd, &rc);
	x = prc.left;
	y = prc.top - (rc.bottom - rc.top);
	if (y < 0) {
		y = 0;
	}
	::SetWindowPos(hDlgWnd, NULL, x, y,  0, 0, SWP_NOSIZE | SWP_NOZORDER);

activate:;
	::ShowWindow(hDlgWnd, SW_SHOW);
}

// WM_COPYDATAの受信
LRESULT CVTWindow::OnReceiveIpcMessage(WPARAM wParam, LPARAM lParam)
{
	COPYDATASTRUCT *cds;
	char *buf, *msg, *name;
	int buflen, msglen, nlen;
	int sending = 0;

	if (!cv.Ready) {
		return 0;
	}

	if (!ts.AcceptBroadcast) { // 337: 2007/03/20
		return 0;
	}

	// 未送信データがある場合は先に送信する
	// データ量が多い場合は送信しきれない可能性がある
#if	UNICODE_INTERNAL_BUFF
	if (TalkStatus == IdTalkSendMem) {
		SendMemContinuously();
	}
#endif
	if (TalkStatus == IdTalkCB) {
		CBSend();
	}
	// 送信可能な状態でなければエラー
	if (TalkStatus != IdTalkKeyb) {
		return 0;
	}

	cds = (COPYDATASTRUCT *)lParam;
	msglen = cds->cbData;
	msg = (char *)cds->lpData;
	if (cds->dwData == IPC_BROADCAST_COMMAND) {
		buf = msg;
		buflen = msglen;
		sending = 1;

	} else if (cds->dwData == IPC_MULTICAST_COMMAND) {
		name = msg;
		nlen = strlen(name) + 1;
		buf = msg + nlen;
		buflen = msglen - nlen;

		// マルチキャスト名をチェックする
		if (CompareMulticastName(name) == 0) {  // 同じ
			sending = 1;
		}
	}

	if (sending) {
		// 端末へ文字列を送り込む
		// DDE通信に使う関数に変更。(2006.2.7 yutaka)
		CBStartSend(buf, buflen, FALSE);
		// 送信データがある場合は送信する
		if (TalkStatus == IdTalkCB) {
			CBSend();
		}
	}

	// CBStartSend(), CBSend() では送信用バッファにデータを書き込むだけで、
	// 実際の送信は teraterm.cpp:OnIdle() で CommSend() が呼ばれる事に
	// よって行われる。
	// しかし非アクティブなウィンドウでは OnIdle() が呼ばれないので、
	// 空のメッセージを送って OnIdle() が呼ばれるようにする。
	::PostMessage(m_hWnd, WM_NULL, 0, 0);

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
		OnIMERequest(wp, lp);
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
	case WM_USER_FTCANCEL:
		OnFileTransEnd(wp, lp);
		break;
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


