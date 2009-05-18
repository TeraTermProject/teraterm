/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */
/* IPv6 modification is Copyright(C) 2000 Jun-ya Kato <kato@win6.jp> */

/* TERATERM.EXE, VT window */

#include "stdafx.h"
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
#include "tekwin.h"
#include "ttdde.h"
#include "ttlib.h"
#include "helpid.h"
#include "teraprn.h"
#ifndef NO_INET6
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <winsock.h>
#endif /* NO_INET6 */
#include "ttplug.h"  /* TTPLUG */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <shlobj.h>
#include <io.h>
#include <errno.h>
#include <imagehlp.h>

#include <windowsx.h>

#include "tt_res.h"
#include "vtwin.h"
#include "addsetting.h"

#define VTClassName "VTWin32"

/* mouse buttons */
#define IdLeftButton 0
#define IdMiddleButton 1
#define IdRightButton 2

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ウィンドウ最大化ボタンを有効にする (2005.1.15 yutaka)
#define WINDOW_MAXMIMUM_ENABLED 1

// WM_COPYDATAによるプロセス間通信の種別 (2005.1.22 yutaka)
#define IPC_BROADCAST_COMMAND 1      // 全端末へ送信
#define IPC_MULTICAST_COMMAND 2      // 任意の端末群へ送信

#define BROADCAST_LOGFILE "broadcast.log"

static HFONT DlgBroadcastFont;
static HFONT DlgCommentFont;

// 本体は addsetting.cpp
extern mouse_cursor_t MouseCursor[];

/////////////////////////////////////////////////////////////////////////////
// CVTWindow

BEGIN_MESSAGE_MAP(CVTWindow, CFrameWnd)
	//{{AFX_MSG_MAP(CVTWindow)
	ON_WM_ACTIVATE()
	ON_WM_CHAR()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_DROPFILES()
	ON_WM_GETMINMAXINFO()
	ON_WM_HSCROLL()
	ON_WM_INITMENUPOPUP()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEACTIVATE()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOVE()
	ON_WM_NCLBUTTONDBLCLK()
	ON_WM_NCRBUTTONDOWN()
	ON_WM_PAINT()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_SYSCHAR()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_SYSCOMMAND()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_TIMER()
	ON_WM_VSCROLL()
	ON_MESSAGE(WM_IME_COMPOSITION,OnIMEComposition)
//<!--by AKASI
	ON_MESSAGE(WM_WINDOWPOSCHANGING,OnWindowPosChanging)
	ON_MESSAGE(WM_SETTINGCHANGE,OnSettingChange)
	ON_MESSAGE(WM_ENTERSIZEMOVE,OnEnterSizeMove)
	ON_MESSAGE(WM_EXITSIZEMOVE ,OnExitSizeMove)
//-->
	ON_MESSAGE(WM_USER_ACCELCOMMAND, OnAccelCommand)
	ON_MESSAGE(WM_USER_CHANGEMENU,OnChangeMenu)
	ON_MESSAGE(WM_USER_CHANGETBAR,OnChangeTBar)
	ON_MESSAGE(WM_USER_COMMNOTIFY,OnCommNotify)
	ON_MESSAGE(WM_USER_COMMOPEN,OnCommOpen)
	ON_MESSAGE(WM_USER_COMMSTART,OnCommStart)
	ON_MESSAGE(WM_USER_DDEEND,OnDdeEnd)
	ON_MESSAGE(WM_USER_DLGHELP2,OnDlgHelp)
	ON_MESSAGE(WM_USER_FTCANCEL,OnFileTransEnd)
	ON_MESSAGE(WM_USER_GETSERIALNO,OnGetSerialNo)
	ON_MESSAGE(WM_USER_KEYCODE,OnKeyCode)
	ON_MESSAGE(WM_USER_PROTOCANCEL,OnProtoEnd)
	ON_MESSAGE(WM_USER_CHANGETITLE,OnChangeTitle)
	ON_MESSAGE(WM_COPYDATA,OnReceiveIpcMessage)
	ON_COMMAND(ID_FILE_NEWCONNECTION, OnFileNewConnection)
	ON_COMMAND(ID_FILE_DUPLICATESESSION, OnDuplicateSession)
	ON_COMMAND(ID_FILE_CYGWINCONNECTION, OnCygwinConnection)
	ON_COMMAND(ID_FILE_TERATERMMENU, OnTTMenuLaunch)
	ON_COMMAND(ID_FILE_LOGMEIN, OnLogMeInLaunch)
	ON_COMMAND(ID_FILE_LOG, OnFileLog)
	ON_COMMAND(ID_FILE_COMMENTTOLOG, OnCommentToLog)
	ON_COMMAND(ID_FILE_VIEWLOG, OnViewLog)
	ON_COMMAND(ID_FILE_SHOWLOGDIALOG, OnShowLogDialog)
	ON_COMMAND(ID_FILE_REPLAYLOG, OnReplayLog)
	ON_COMMAND(ID_FILE_SENDFILE, OnFileSend)
	ON_COMMAND(ID_FILE_KERMITRCV, OnFileKermitRcv)
	ON_COMMAND(ID_FILE_KERMITGET, OnFileKermitGet)
	ON_COMMAND(ID_FILE_KERMITSEND, OnFileKermitSend)
	ON_COMMAND(ID_FILE_KERMITFINISH, OnFileKermitFinish)
	ON_COMMAND(ID_FILE_XRCV, OnFileXRcv)
	ON_COMMAND(ID_FILE_XSEND, OnFileXSend)
	ON_COMMAND(ID_FILE_YRCV, OnFileYRcv)
	ON_COMMAND(ID_FILE_YSEND, OnFileYSend)
	ON_COMMAND(ID_FILE_ZRCV, OnFileZRcv)
	ON_COMMAND(ID_FILE_ZSEND, OnFileZSend)
	ON_COMMAND(ID_FILE_BPRCV, OnFileBPRcv)
	ON_COMMAND(ID_FILE_BPSEND, OnFileBPSend)
	ON_COMMAND(ID_FILE_QVRCV, OnFileQVRcv)
	ON_COMMAND(ID_FILE_QVSEND, OnFileQVSend)
	ON_COMMAND(ID_FILE_CHANGEDIR, OnFileChangeDir)
	ON_COMMAND(ID_FILE_PRINT2, OnFilePrint)
	ON_COMMAND(ID_FILE_DISCONNECT, OnFileDisconnect)
	ON_COMMAND(ID_FILE_EXIT, OnFileExit)
	ON_COMMAND(ID_EDIT_COPY2, OnEditCopy)
	ON_COMMAND(ID_EDIT_COPYTABLE, OnEditCopyTable)
	ON_COMMAND(ID_EDIT_PASTE2, OnEditPaste)
	ON_COMMAND(ID_EDIT_PASTECR, OnEditPasteCR)
	ON_COMMAND(ID_EDIT_CLEARSCREEN, OnEditClearScreen)
	ON_COMMAND(ID_EDIT_CLEARBUFFER, OnEditClearBuffer)
	ON_COMMAND(ID_EDIT_CANCELSELECT, OnEditCancelSelection)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectAllBuffer)
	ON_COMMAND(ID_EDIT_SELECTSCREEN, OnEditSelectScreenBuffer)
	ON_COMMAND(ID_SETUP_ADDITIONALSETTINGS, OnExternalSetup)
	ON_COMMAND(ID_SETUP_TERMINAL, OnSetupTerminal)
	ON_COMMAND(ID_SETUP_WINDOW, OnSetupWindow)
	ON_COMMAND(ID_SETUP_FONT, OnSetupFont)
	ON_COMMAND(ID_SETUP_KEYBOARD, OnSetupKeyboard)
	ON_COMMAND(ID_SETUP_SERIALPORT, OnSetupSerialPort)
	ON_COMMAND(ID_SETUP_TCPIP, OnSetupTCPIP)
	ON_COMMAND(ID_SETUP_GENERAL, OnSetupGeneral)
	ON_COMMAND(ID_SETUP_SAVE, OnSetupSave)
	ON_COMMAND(ID_SETUP_RESTORE, OnSetupRestore)
	ON_COMMAND(ID_SETUP_LOADKEYMAP, OnSetupLoadKeyMap)
	ON_COMMAND(ID_CONTROL_RESETTERMINAL, OnControlResetTerminal)
	ON_COMMAND(ID_CONTROL_RESETREMOTETITLE, OnControlResetRemoteTitle)
	ON_COMMAND(ID_CONTROL_AREYOUTHERE, OnControlAreYouThere)
	ON_COMMAND(ID_CONTROL_SENDBREAK, OnControlSendBreak)
	ON_COMMAND(ID_CONTROL_RESETPORT, OnControlResetPort)
	ON_COMMAND(ID_CONTROL_BROADCASTCOMMAND, OnControlBroadcastCommand)
	ON_COMMAND(ID_CONTROL_OPENTEK, OnControlOpenTEK)
	ON_COMMAND(ID_CONTROL_CLOSETEK, OnControlCloseTEK)
	ON_COMMAND(ID_CONTROL_MACRO, OnControlMacro)
	ON_COMMAND(ID_WINDOW_WINDOW, OnWindowWindow)
	ON_COMMAND(ID_HELP_INDEX2, OnHelpIndex)
	ON_COMMAND(ID_HELP_ABOUT, OnHelpAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVTWindow constructor


static BOOL MySetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
	typedef BOOL (WINAPI *func)(HWND,COLORREF,BYTE,DWORD);
	static HMODULE g_hmodUser32 = NULL;
	static func g_pSetLayeredWindowAttributes = NULL;

	if (g_hmodUser32 == NULL) {
		g_hmodUser32 = LoadLibrary("user32.dll");
		if (g_hmodUser32 == NULL) {
			return FALSE;
		}

		g_pSetLayeredWindowAttributes =
			(func)GetProcAddress(g_hmodUser32, "SetLayeredWindowAttributes");
	}

	if (g_pSetLayeredWindowAttributes == NULL) {
		return FALSE;
	}

	return g_pSetLayeredWindowAttributes(hwnd, crKey, 
	                                     bAlpha, dwFlags);
}


// Tera Term起動時とURL文字列mouse over時に呼ばれる (2005.4.2 yutaka)
extern "C" void SetMouseCursor(char *cursor)
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

	hc = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(name), IMAGE_CURSOR,
	                        0, 0, LR_DEFAULTSIZE | LR_SHARED);

	if (hc != NULL) {
		SetClassLongPtr(HVTWin, GCLP_HCURSOR, (LONG_PTR)hc);
	}
}


void SetWindowStyle(TTTSet *ts)
{
	LONG_PTR lp;

	SetMouseCursor(ts->MouseCursorName);

	// 2006/03/16 by 337: BGUseAlphaBlendAPIがOnならばLayered属性とする
	//if (ts->EtermLookfeel.BGUseAlphaBlendAPI) {
	// アルファ値が255の場合、画面のちらつきを抑えるため何もしないこととする。(2006.4.1 yutaka)
	// 呼び出し元で、値が変更されたときのみ設定を反映する。(2007.10.19 maya)
	if (ts->AlphaBlend < 255) {
		lp = GetWindowLongPtr(HVTWin, GWL_EXSTYLE);
		if (lp != 0) {
			SetWindowLongPtr(HVTWin, GWL_EXSTYLE, lp | WS_EX_LAYERED);
			MySetLayeredWindowAttributes(HVTWin, 0, ts->AlphaBlend, LWA_ALPHA);
		}
	}
	// アルファ値が 255 の場合、透明化属性を削除して再描画する。(2007.10.22 maya)
	else {
		lp = GetWindowLongPtr(HVTWin, GWL_EXSTYLE);
		if (lp != 0) {
			SetWindowLongPtr(HVTWin, GWL_EXSTYLE, lp & ~WS_EX_LAYERED);
			RedrawWindow(HVTWin, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME);
		}
	}
}


//
// 例外ハンドラのフック（スタックトレースのダンプ）
//
// cf. http://svn.collab.net/repos/svn/trunk/subversion/libsvn_subr/win32_crashrpt.c
// (2007.9.30 yutaka)
//
// 例外コードを文字列へ変換する
static char *GetExceptionString(int exception)
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

	// Windows98/Me/NT4では動かないためスキップする。(2007.10.9 yutaka)
	h2 = LoadLibrary("imagehlp.dll");
	if (((h = GetModuleHandle("imagehlp.dll")) == NULL) ||
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

	MessageBox(NULL, msg, "Tera Term: Application fault", MB_OK | MB_ICONEXCLAMATION);

error:
//	return (EXCEPTION_EXECUTE_HANDLER);  /* そのままプロセスを終了させる */
	return (EXCEPTION_CONTINUE_SEARCH);  /* 引き続き［アプリケーションエラー］ポップアップメッセージボックスを呼び出す */
}


CVTWindow::CVTWindow()
{
	WNDCLASS wc;
	RECT rect;
	DWORD Style;
#ifdef ALPHABLEND_TYPE2
	DWORD ExStyle;
#endif
	char Temp[MAXPATHLEN];
	char *Param;
	int CmdShow;
	PKeyMap tempkm;
	int fuLoad = LR_DEFAULTCOLOR;

#ifdef _DEBUG
  ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// 例外ハンドラのフック (2007.9.30 yutaka)
	SetUnhandledExceptionFilter(ApplicationFaultHandler);

	TTXInit(&ts, &cv); /* TTPLUG */

	CommInit(&cv);

	MsgDlgHelp = RegisterWindowMessage(HELPMSGSTRING);

	if (StartTeraTerm(&ts)) {
		/* first instance */
		if (LoadTTSET()) {
			/* read setup info from "teraterm.ini" */
			(*ReadIniFile)(ts.SetupFName, &ts);
			/* read keycode map from "keyboard.cnf" */
			tempkm = (PKeyMap)malloc(sizeof(TKeyMap));
			if (tempkm!=NULL) {
				strncpy_s(Temp, sizeof(Temp), ts.HomeDir, _TRUNCATE);
				AppendSlash(Temp,sizeof(Temp));
				strncat_s(Temp,sizeof(Temp),"KEYBOARD.CNF",_TRUNCATE);
				(*ReadKeyboardCnf)(Temp,tempkm,TRUE);
			}
			FreeTTSET();
			/* store default sets in TTCMN */
#if 0
			ChangeDefaultSet(&ts,tempkm);
#else
			ChangeDefaultSet(NULL,tempkm);
#endif
			if (tempkm!=NULL) free(tempkm);
		}

	} else {
		// 2つめ以降のプロセスにおいても、ディスクから TERATERM.INI を読む。(2004.11.4 yutaka)
		if (LoadTTSET()) {
			/* read setup info from "teraterm.ini" */
			(*ReadIniFile)(ts.SetupFName, &ts);
			/* read keycode map from "keyboard.cnf" */
			tempkm = (PKeyMap)malloc(sizeof(TKeyMap));
			if (tempkm!=NULL) {
				strncpy_s(Temp, sizeof(Temp, ts.HomeDir), ts.HomeDir, _TRUNCATE);
				AppendSlash(Temp,sizeof(Temp));
				strncat_s(Temp,sizeof(Temp),"KEYBOARD.CNF",_TRUNCATE);
				(*ReadKeyboardCnf)(Temp,tempkm,TRUE);
			}
			FreeTTSET();
			/* store default sets in TTCMN */
			if (tempkm!=NULL) {
				free(tempkm);
			}
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

	// duplicate sessionの指定があるなら、共有メモリからコピーする (2004.12.7 yutaka)
	if (ts.DuplicateSession == 1) {
		CopyShmemToTTSet(&ts);
	}

	InitKeyboard();
	SetKeyMap();

	// コマンドラインでも設定ファイルでも変更しないのでここで初期化 (2008.1.25 maya)
	cv.isSSH = 0;
	cv.TitleRemote[0] = '\0';

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
	wc.lpfnWndProc = AfxWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = AfxGetInstanceHandle();
	if (is_NT4()) {
		fuLoad = LR_VGACOLOR;
	}
	wc.hIcon = NULL;
	//wc.hCursor = LoadCursor(NULL,IDC_IBEAM);
	wc.hCursor = NULL; // マウスカーソルは動的に変更する (2005.4.2 yutaka)
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = VTClassName;

	RegisterClass(&wc);
	LoadAccelTable(MAKEINTRESOURCE(IDR_ACC));

	if (ts.VTPos.x==CW_USEDEFAULT) {
		rect = rectDefault;
	}
	else {
		rect.left = ts.VTPos.x;
		rect.top = ts.VTPos.y;
		rect.right = rect.left + 100;
		rect.bottom = rect.top + 100;
	}
	Create(VTClassName, "Tera Term", Style, rect, NULL, NULL);

	/*--------- Init2 -----------------*/
	HVTWin = GetSafeHwnd();
	if (HVTWin == NULL) return;
	// register this window to the window list
	SerialNo = RegWin(HVTWin,NULL);

	logfile_lock_initialize();
	SetWindowStyle(&ts);
	// ロケールの設定
	// wctomb のため
	setlocale(LC_ALL, ts.Locale);

#ifdef ALPHABLEND_TYPE2
//<!--by AKASI
	if(BGNoFrame && ts.HideTitle > 0) {
		ExStyle  = GetWindowLong(HVTWin,GWL_EXSTYLE);
		ExStyle &= ~WS_EX_CLIENTEDGE;
		SetWindowLong(HVTWin,GWL_EXSTYLE,ExStyle);
	}
//-->
#endif

	if (is_NT4()) {
		fuLoad = LR_VGACOLOR;
	}
	::PostMessage(HVTWin,WM_SETICON,ICON_SMALL,
	              (LPARAM)LoadImage(AfxGetInstanceHandle(),
	                                MAKEINTRESOURCE((ts.VTIcon!=IdIconDefault)?ts.VTIcon:IDI_VT),
	                                IMAGE_ICON,16,16,fuLoad));
	// Vista の Aero において Alt+Tab 切り替えで表示されるアイコンが
	// 16x16 アイコンの拡大になってしまうので、大きいアイコンも
	// セットする (2008.9.3 maya)
	::PostMessage(HVTWin,WM_SETICON,ICON_BIG,
	              (LPARAM)LoadImage(AfxGetInstanceHandle(),
	                                MAKEINTRESOURCE((ts.VTIcon!=IdIconDefault)?ts.VTIcon:IDI_VT),
	                                IMAGE_ICON, 0, 0, fuLoad));

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
	ShowWindow(CmdShow);
	ChangeCaret();
}

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void CVTWindow::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CVTWindow::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

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
	BOOL pasteRButton = RButton && Paste;
	BOOL pasteMButton = MButton && Paste;

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

	// added ConfirmPasteMouseRButton (2007.3.17 maya)
	if (pasteRButton && !ts.ConfirmPasteMouseRButton) {
		if (CBStartPasteConfirmChange(HVTWin)) {
			CBStartPaste(HVTWin,FALSE,0,NULL,0);
			/* 最下行でだけ自動スクロールする設定の場合
			   ペースト処理でスクロールさせる */
			if (ts.AutoScrollOnlyInBottomLine != 0 && WinOrgY != 0) {
				DispVScroll(SCROLL_BOTTOM, 0);
			}
		}
	}
	else if (pasteMButton) {
		if (CBStartPasteConfirmChange(HVTWin)) {
			CBStartPaste(HVTWin,FALSE,0,NULL,0);
			/* 最下行でだけ自動スクロールする設定の場合
			   ペースト処理でスクロールさせる */
			if (ts.AutoScrollOnlyInBottomLine != 0 && WinOrgY != 0) {
				DispVScroll(SCROLL_BOTTOM, 0);
			}
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
				           submenu != NULL ? (UINT)submenu : GetMenuItemID(PopupMenu, i),
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

	// added ConfirmPasteMouseRButton (2007.3.17 maya)
	if ((LMR == IdRightButton) &&
		!ts.DisablePasteMouseRButton &&
		ts.ConfirmPasteMouseRButton &&
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
char *LogMeTTexename = "LogMeTT.exe";
static char LogMeTTMenuString[] = "Log&MeTT";
static BOOL isLogMeTTExist()
{
	char LogMeTT[MAX_PATH];
	
	strncpy_s(LogMeTT, sizeof(LogMeTT), ts.HomeDir, _TRUNCATE);
	AppendSlash(LogMeTT, sizeof(LogMeTT));
	strncat_s(LogMeTT, sizeof(LogMeTT), LogMeTTexename, _TRUNCATE);

	if (_access(LogMeTT, 0) == -1) {
		return FALSE;
	}
	return TRUE;
}

void CVTWindow::InitMenu(HMENU *Menu)
{
	*Menu = LoadMenu(AfxGetInstanceHandle(),
	                 MAKEINTRESOURCE(IDR_MENU));
	char uimsg[MAX_UIMSG];
	int ret;

	FileMenu = GetSubMenu(*Menu,ID_FILE);
	TransMenu = GetSubMenu(FileMenu,ID_TRANSFER);
	EditMenu = GetSubMenu(*Menu,ID_EDIT);
	SetupMenu = GetSubMenu(*Menu,ID_SETUP);
	ControlMenu = GetSubMenu(*Menu,ID_CONTROL);
	HelpMenu = GetSubMenu(*Menu,ID_HELPMENU);

	/* LogMeTT の存在を確認してメニューを追加する */
	if (isLogMeTTExist()) {
		::InsertMenu(FileMenu, ID_FILE_PRINT2, MF_STRING | MF_ENABLED | MF_BYCOMMAND,
		             ID_FILE_LOGMEIN, LogMeTTMenuString);
		::InsertMenu(FileMenu, ID_FILE_PRINT2, MF_SEPARATOR, NULL, NULL);
	}

	GetMenuString(*Menu, ID_FILE, uimsg, sizeof(uimsg), MF_BYPOSITION);
	get_lang_msg("MENU_FILE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(*Menu, ID_FILE, MF_BYPOSITION, ID_FILE, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_NEWCONNECTION, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_NEW", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_NEWCONNECTION, MF_BYCOMMAND, ID_FILE_NEWCONNECTION, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_DUPLICATESESSION, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_DUPLICATE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_DUPLICATESESSION, MF_BYCOMMAND, ID_FILE_DUPLICATESESSION, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_CYGWINCONNECTION, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_GYGWIN", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_CYGWINCONNECTION, MF_BYCOMMAND, ID_FILE_CYGWINCONNECTION, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_LOG, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_LOG", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_LOG, MF_BYCOMMAND, ID_FILE_LOG, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_COMMENTTOLOG, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_COMMENTLOG", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_COMMENTTOLOG, MF_BYCOMMAND, ID_FILE_COMMENTTOLOG, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_VIEWLOG, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_VIEWLOG", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_VIEWLOG, MF_BYCOMMAND, ID_FILE_VIEWLOG, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_SHOWLOGDIALOG, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_SHOWLOGDIALOG", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_SHOWLOGDIALOG, MF_BYCOMMAND, ID_FILE_SHOWLOGDIALOG, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_SENDFILE, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_SENDFILE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_SENDFILE, MF_BYCOMMAND, ID_FILE_SENDFILE, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_REPLAYLOG, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_REPLAYLOG", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_REPLAYLOG, MF_BYCOMMAND, ID_FILE_REPLAYLOG, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_CHANGEDIR, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_CHANGEDIR", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_CHANGEDIR, MF_BYCOMMAND, ID_FILE_CHANGEDIR, ts.UIMsg);
	ret = GetMenuString(FileMenu, ID_FILE_LOGMEIN, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	if (ret != 0) {
		get_lang_msg("MENU_FILE_LOGMETT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
		ModifyMenu(FileMenu, ID_FILE_LOGMEIN, MF_BYCOMMAND, ID_FILE_LOGMEIN, ts.UIMsg);
	}
	GetMenuString(FileMenu, ID_FILE_PRINT2, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_PRINT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_PRINT2, MF_BYCOMMAND, ID_FILE_PRINT2, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_DISCONNECT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_DISCONNECT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_DISCONNECT, MF_BYCOMMAND, ID_FILE_DISCONNECT, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_EXIT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_FILE_EXIT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_EXIT, MF_BYCOMMAND, ID_FILE_EXIT, ts.UIMsg);

	GetMenuString(FileMenu, 9, uimsg, sizeof(uimsg), MF_BYPOSITION);
	get_lang_msg("MENU_TRANS", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, 9, MF_BYPOSITION, 9, ts.UIMsg);

	GetMenuString(FileMenu, ID_FILE_KERMITRCV, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_KERMIT_RCV", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_KERMITRCV, MF_BYCOMMAND, ID_FILE_KERMITRCV, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_KERMITGET, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_KERMIT_GET", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_KERMITGET, MF_BYCOMMAND, ID_FILE_KERMITGET, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_KERMITSEND, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_KERMIT_SEND", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_KERMITSEND, MF_BYCOMMAND, ID_FILE_KERMITSEND, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_KERMITFINISH, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_KERMIT_FINISH", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_KERMITFINISH, MF_BYCOMMAND, ID_FILE_KERMITFINISH, ts.UIMsg);

	GetMenuString(FileMenu, ID_FILE_XRCV, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_X_RCV", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_XRCV, MF_BYCOMMAND, ID_FILE_XRCV, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_XSEND, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_X_SEND", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_XSEND, MF_BYCOMMAND, ID_FILE_XSEND, ts.UIMsg);

	// TBD: YMODEMはまだ未サポートなので、メニューは隠す。(2008.5.15 yutaka)
//#define YMODEM_TBD
#ifndef YMODEM_TBD
	DeleteMenu(TransMenu, 2, MF_BYPOSITION);
	DeleteMenu(FileMenu, ID_FILE_YRCV, MF_BYCOMMAND);
	DeleteMenu(FileMenu, ID_FILE_YSEND, MF_BYCOMMAND);
#endif
#undef YMODEM_TBD

	GetMenuString(FileMenu, ID_FILE_ZRCV, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_Z_RCV", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_ZRCV, MF_BYCOMMAND, ID_FILE_ZRCV, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_ZSEND, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_Z_SEND", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_ZSEND, MF_BYCOMMAND, ID_FILE_ZSEND, ts.UIMsg);

	GetMenuString(FileMenu, ID_FILE_BPRCV, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_BP_RCV", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_BPRCV, MF_BYCOMMAND, ID_FILE_BPRCV, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_BPSEND, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_BP_SEND", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_BPSEND, MF_BYCOMMAND, ID_FILE_BPSEND, ts.UIMsg);

	GetMenuString(FileMenu, ID_FILE_QVRCV, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_QV_RCV", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_QVRCV, MF_BYCOMMAND, ID_FILE_QVRCV, ts.UIMsg);
	GetMenuString(FileMenu, ID_FILE_QVSEND, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_TRANS_QV_SEND", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(FileMenu, ID_FILE_QVSEND, MF_BYCOMMAND, ID_FILE_QVSEND, ts.UIMsg);

	GetMenuString(*Menu, ID_EDIT, uimsg, sizeof(uimsg), MF_BYPOSITION);
	get_lang_msg("MENU_EDIT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(*Menu, ID_EDIT, MF_BYPOSITION, ID_EDIT, ts.UIMsg);
	GetMenuString(EditMenu, ID_EDIT_COPY2, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_COPY", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(EditMenu, ID_EDIT_COPY2, MF_BYCOMMAND, ID_EDIT_COPY2, ts.UIMsg);
	GetMenuString(EditMenu, ID_EDIT_COPYTABLE, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_COPYTABLE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(EditMenu, ID_EDIT_COPYTABLE, MF_BYCOMMAND, ID_EDIT_COPYTABLE, ts.UIMsg);
	GetMenuString(EditMenu, ID_EDIT_PASTE2, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_PASTE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(EditMenu, ID_EDIT_PASTE2, MF_BYCOMMAND, ID_EDIT_PASTE2, ts.UIMsg);
	GetMenuString(EditMenu, ID_EDIT_PASTECR, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_PASTECR", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(EditMenu, ID_EDIT_PASTECR, MF_BYCOMMAND, ID_EDIT_PASTECR, ts.UIMsg);
	GetMenuString(EditMenu, ID_EDIT_CLEARSCREEN, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_CLSCREEN", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(EditMenu, ID_EDIT_CLEARSCREEN, MF_BYCOMMAND, ID_EDIT_CLEARSCREEN, ts.UIMsg);
	GetMenuString(EditMenu, ID_EDIT_CLEARBUFFER, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_CLBUFFER", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(EditMenu, ID_EDIT_CLEARBUFFER, MF_BYCOMMAND, ID_EDIT_CLEARBUFFER, ts.UIMsg);
	GetMenuString(EditMenu, ID_EDIT_CANCELSELECT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_CANCELSELECT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(EditMenu, ID_EDIT_CANCELSELECT, MF_BYCOMMAND, ID_EDIT_CANCELSELECT, ts.UIMsg);
	GetMenuString(EditMenu, ID_EDIT_SELECTSCREEN, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_SELECTSCREEN", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(EditMenu, ID_EDIT_SELECTSCREEN, MF_BYCOMMAND, ID_EDIT_SELECTSCREEN, ts.UIMsg);
	GetMenuString(EditMenu, ID_EDIT_SELECTALL, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_SELECTALL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(EditMenu, ID_EDIT_SELECTALL, MF_BYCOMMAND, ID_EDIT_SELECTALL, ts.UIMsg);

	GetMenuString(*Menu, ID_SETUP, uimsg, sizeof(uimsg), MF_BYPOSITION);
	get_lang_msg("MENU_SETUP", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(*Menu, ID_SETUP, MF_BYPOSITION, ID_SETUP, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_TERMINAL, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_TERMINAL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_TERMINAL, MF_BYCOMMAND, ID_SETUP_TERMINAL, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_WINDOW, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_WINDOW", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_WINDOW, MF_BYCOMMAND, ID_SETUP_WINDOW, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_FONT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_FONT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_FONT, MF_BYCOMMAND, ID_SETUP_FONT, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_KEYBOARD, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_KEYBOARD", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_KEYBOARD, MF_BYCOMMAND, ID_SETUP_KEYBOARD, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_SERIALPORT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_SERIALPORT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_SERIALPORT, MF_BYCOMMAND, ID_SETUP_SERIALPORT, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_TCPIP, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_TCPIP", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_TCPIP, MF_BYCOMMAND, ID_SETUP_TCPIP, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_GENERAL, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_GENERAL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_GENERAL, MF_BYCOMMAND, ID_SETUP_GENERAL, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_ADDITIONALSETTINGS, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_ADDITION", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_ADDITIONALSETTINGS, MF_BYCOMMAND, ID_SETUP_ADDITIONALSETTINGS, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_SAVE, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_SAVE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_SAVE, MF_BYCOMMAND, ID_SETUP_SAVE, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_RESTORE, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_RESTORE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_RESTORE, MF_BYCOMMAND, ID_SETUP_RESTORE, ts.UIMsg);
	GetMenuString(SetupMenu, ID_SETUP_LOADKEYMAP, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_SETUP_LOADKEYMAP", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(SetupMenu, ID_SETUP_LOADKEYMAP, MF_BYCOMMAND, ID_SETUP_LOADKEYMAP, ts.UIMsg);

	GetMenuString(*Menu, ID_CONTROL, uimsg, sizeof(uimsg), MF_BYPOSITION);
	get_lang_msg("MENU_CONTROL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(*Menu, ID_CONTROL, MF_BYPOSITION, ID_CONTROL, ts.UIMsg);
	GetMenuString(ControlMenu, ID_CONTROL_RESETTERMINAL, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_CONTROL_RESET", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(ControlMenu, ID_CONTROL_RESETTERMINAL, MF_BYCOMMAND, ID_CONTROL_RESETTERMINAL, ts.UIMsg);
	GetMenuString(ControlMenu, ID_CONTROL_RESETREMOTETITLE, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_CONTROL_RESETTITLE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(ControlMenu, ID_CONTROL_RESETREMOTETITLE, MF_BYCOMMAND, ID_CONTROL_RESETREMOTETITLE, ts.UIMsg);
	GetMenuString(ControlMenu, ID_CONTROL_AREYOUTHERE, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_CONTROL_AREYOUTHERE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(ControlMenu, ID_CONTROL_AREYOUTHERE, MF_BYCOMMAND, ID_CONTROL_AREYOUTHERE, ts.UIMsg);
	GetMenuString(ControlMenu, ID_CONTROL_SENDBREAK, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_CONTROL_SENDBREAK", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(ControlMenu, ID_CONTROL_SENDBREAK, MF_BYCOMMAND, ID_CONTROL_SENDBREAK, ts.UIMsg);
	GetMenuString(ControlMenu, ID_CONTROL_RESETPORT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_CONTROL_RESETPORT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(ControlMenu, ID_CONTROL_RESETPORT, MF_BYCOMMAND, ID_CONTROL_RESETPORT, ts.UIMsg);
	GetMenuString(ControlMenu, ID_CONTROL_BROADCASTCOMMAND, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_CONTROL_BROADCAST", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(ControlMenu, ID_CONTROL_BROADCASTCOMMAND, MF_BYCOMMAND, ID_CONTROL_BROADCASTCOMMAND, ts.UIMsg);
	GetMenuString(ControlMenu, ID_CONTROL_OPENTEK, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_CONTROL_OPENTEK", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(ControlMenu, ID_CONTROL_OPENTEK, MF_BYCOMMAND, ID_CONTROL_OPENTEK, ts.UIMsg);
	GetMenuString(ControlMenu, ID_CONTROL_CLOSETEK, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_CONTROL_CLOSETEK", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(ControlMenu, ID_CONTROL_CLOSETEK, MF_BYCOMMAND, ID_CONTROL_CLOSETEK, ts.UIMsg);
	GetMenuString(ControlMenu, ID_CONTROL_MACRO, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_CONTROL_MACRO", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(ControlMenu, ID_CONTROL_MACRO, MF_BYCOMMAND, ID_CONTROL_MACRO, ts.UIMsg);

	GetMenuString(*Menu, ID_HELPMENU, uimsg, sizeof(uimsg), MF_BYPOSITION);
	get_lang_msg("MENU_HELP", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(*Menu, ID_HELPMENU, MF_BYPOSITION, ID_HELPMENU, ts.UIMsg);
	GetMenuString(HelpMenu, ID_HELP_INDEX2, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_HELP_INDEX", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(HelpMenu, ID_HELP_INDEX2, MF_BYCOMMAND, ID_HELP_INDEX2, ts.UIMsg);
	GetMenuString(HelpMenu, ID_HELP_ABOUT, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_HELP_ABOUT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(HelpMenu, ID_HELP_ABOUT, MF_BYCOMMAND, ID_HELP_ABOUT, ts.UIMsg);

	if ((ts.MenuFlag & MF_SHOWWINMENU) !=0) {
		WinMenu = CreatePopupMenu();
		get_lang_msg("MENU_WINDOW", ts.UIMsg, sizeof(ts.UIMsg),
		             "&Window", ts.UILanguageFile);
		::InsertMenu(*Menu,ID_HELPMENU,
		             MF_STRING | MF_ENABLED | MF_POPUP | MF_BYPOSITION,
		             (int)WinMenu, ts.UIMsg);
	}

	TTXModifyMenu(*Menu); /* TTPLUG */
}

void CVTWindow::InitMenuPopup(HMENU SubMenu)
{
	if ( SubMenu == FileMenu )
	{
		if ( Connecting ) {
			EnableMenuItem(FileMenu,ID_FILE_NEWCONNECTION,MF_BYCOMMAND | MF_GRAYED);
		} else {
			EnableMenuItem(FileMenu,ID_FILE_NEWCONNECTION,MF_BYCOMMAND | MF_ENABLED);
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
			EnableMenuItem(FileMenu,ID_FILE_DUPLICATESESSION,MF_BYCOMMAND | MF_ENABLED);
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
			if (ts.LogHideDialog) {
				EnableMenuItem(FileMenu,ID_FILE_SHOWLOGDIALOG, MF_BYCOMMAND | MF_ENABLED);
			}
			else {
				EnableMenuItem(FileMenu,ID_FILE_SHOWLOGDIALOG, MF_BYCOMMAND | MF_GRAYED);
			}
		} else {
			EnableMenuItem(FileMenu,ID_FILE_LOG,MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(FileMenu,ID_FILE_COMMENTTOLOG, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_VIEWLOG, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(FileMenu,ID_FILE_SHOWLOGDIALOG, MF_BYCOMMAND | MF_GRAYED);
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
		if (cv.Ready &&
		    ((cv.PortType==IdTCPIP) || (cv.PortType==IdFile)) ||
			(SendVar!=NULL) || (FileVar!=NULL) || Connecting) {
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
		}
		else {
			EnableMenuItem(ControlMenu,ID_CONTROL_MACRO,MF_BYCOMMAND | MF_ENABLED);
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
	char uimsg[MAX_UIMSG];

	*Menu = LoadMenu(AfxGetInstanceHandle(),
	                 MAKEINTRESOURCE(IDR_PASTEMENU));

	GetMenuString(*Menu, ID_EDIT_PASTE2, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_PASTE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(*Menu, ID_EDIT_PASTE2, MF_BYCOMMAND, ID_EDIT_PASTE2, ts.UIMsg);
	GetMenuString(*Menu, ID_EDIT_PASTECR, uimsg, sizeof(uimsg), MF_BYCOMMAND);
	get_lang_msg("MENU_EDIT_PASTECR", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	ModifyMenu(*Menu, ID_EDIT_PASTECR, MF_BYCOMMAND, ID_EDIT_PASTECR, ts.UIMsg);
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
	BGInitialize();
	BGSetupPrimary(TRUE);
	// 2006/03/17 by 337 : Alpha値も即時変更
	// Layered窓になっていない場合は効果が無い
	if (ts.EtermLookfeel.BGUseAlphaBlendAPI) {
		MySetLayeredWindowAttributes(HVTWin, 0, ts.AlphaBlend, LWA_ALPHA);
	}
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
	char TempName[MAXPATHLEN];

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

	if (cv.Ready && cv.TelFlag && (ts.TelEcho>0)) {
		TelChangeEcho();
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
		pTEKWin = new CTEKWindow();
	}
	else {
		::ShowWindow(HTEKWin,SW_SHOWNORMAL);
		::SetFocus(HTEKWin);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CVTWindow message handler

LRESULT CVTWindow::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result;

	if (message == MsgDlgHelp) {
		OnDlgHelp(wParam,lParam);
		return 0;
	}
	else if ((ts.HideTitle>0) &&
	         (message == WM_NCHITTEST)) {
		Result = CFrameWnd::DefWindowProc(message,wParam,lParam);
		if ((Result==HTCLIENT) && AltKey())
#ifdef ALPHABLEND_TYPE2
			if(ShiftKey())
				Result = HTBOTTOMRIGHT;
			else
				Result = HTCAPTION;
#else
			Result = HTCAPTION;
#endif
		return Result;
	}

	return (CFrameWnd::DefWindowProc(message,wParam,lParam));
}

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
			case ID_ACC_PASTECR:
				OnEditPasteCR();
				return TRUE;
			case ID_ACC_AREYOUTHERE:
				OnControlAreYouThere();
				return TRUE;
			case ID_ACC_PASTE:
				OnEditPaste();
				return TRUE;
			case ID_ACC_DISCONNECT:
				OnFileDisconnect();
				return TRUE;
			case ID_FILE_DUPLICATESESSION:
				// added DisableAcceleratorDuplicateSession (2009.4.6 maya)
				if (!ts.DisableAcceleratorDuplicateSession)
					OnDuplicateSession();
				return TRUE;
		}
		if (ActiveWin==IdVT) {
			switch (wID) {
				case ID_ACC_NEWCONNECTION:
					OnFileNewConnection();
					return TRUE;
				case ID_ACC_COPY:
					OnEditCopy();
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
			return CFrameWnd::OnCommand(wParam, lParam);
		}
	}
}

void CVTWindow::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	DispSetActive(nState!=WA_INACTIVE);
}

void CVTWindow::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	unsigned int i;
	char Code;

	if (!KeybEnabled || (TalkStatus!=IdTalkKeyb)) {
		return;
	}

	if ((ts.MetaKey>0) && AltKey()) {
		::PostMessage(HVTWin,WM_SYSCHAR,nChar,MAKELONG(nRepCnt,nFlags));
		return;
	}
	Code = nChar;

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

	/* 最下行でだけ自動スクロールする設定の場合
	   リモートへのキー入力送信でスクロールさせる */
	if (ts.AutoScrollOnlyInBottomLine != 0 && WinOrgY != 0) {
		DispVScroll(SCROLL_BOTTOM, 0);
	}
}

/* copy from ttset.c*/
static void WriteInt2(PCHAR Sect, PCHAR Key, PCHAR FName, int i1, int i2)
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
	get_lang_msg("MSG_DISCONNECT_CONF", ts.UIMsg, sizeof(ts.UIMsg),
	             "Disconnect?", ts.UILanguageFile);
	if (cv.Ready && (cv.PortType==IdTCPIP) &&
	    ((ts.PortFlag & PF_CONFIRMDISCONN) != 0) &&
	    ! CloseTT &&
	    (::MessageBox(HVTWin, ts.UIMsg, "Tera Term",
	     MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2)==IDCANCEL)) {
		return;
	}

	FileTransEnd(0);
	ProtoEnd();

	SaveVTPos();
	DestroyWindow();
}

void CVTWindow::OnDestroy()
{
	// remove this window from the window list
	UnregWin(HVTWin);

	EndKeyboard();

	/* Disable drag-drop */
	::DragAcceptFiles(HVTWin,FALSE);

	EndDDE();

	if (cv.TelFlag) {
		EndTelnet();
	}
	CommClose(&cv);

	OpenHelp(HVTWin,HH_CLOSE_ALL,0);

	FreeIME();
	FreeTTSET();
	do { }
	while (FreeTTDLG());

	do { }
	while (FreeTTFILE());

	if (HTEKWin != NULL) {
		::DestroyWindow(HTEKWin);
	}

	EndDisp();

	FreeBuffer();

	CFrameWnd::OnDestroy();
	TTXEnd(); /* TTPLUG */
}

// MessageBoxのボタン名変更用ハンドラ
static LRESULT CALLBACK MsgBoxHootProc( INT hc, WPARAM wParam, LPARAM lParam )
{
	if ( hc == HCBT_ACTIVATE ) {
		// &Send file や S&CP のようなアクセラレータキーは設けない
		get_lang_msg("FILEDLG_TRANS_TITLE_SENDFILE", ts.UIMsg, sizeof(ts.UIMsg),
		             "Send file", ts.UILanguageFile);
		SetDlgItemText( (HWND)wParam, IDOK, ts.UIMsg );
		SetDlgItemText( (HWND)wParam, IDYES, ts.UIMsg );
		SetDlgItemText( (HWND)wParam, IDNO, "SCP" );

		return FALSE;
	}

	return CallNextHookEx( NULL, hc, wParam, lParam );
}

void CVTWindow::OnDropFiles(HDROP hDropInfo)
{
	::SetForegroundWindow(HVTWin);
	if (cv.Ready && (SendVar==NULL) && NewFileVar(&SendVar))
	{
		if (DragQueryFile(hDropInfo,0,SendVar->FullName,
			sizeof(SendVar->FullName))>0)
		{
			DWORD attr;
			char *ptr, *q;
			char tmpbuf[_MAX_PATH * 2];

			// ディレクトリの場合はフルパス名を貼り付ける (2004.11.3 yutaka)
			attr = GetFileAttributes(SendVar->FullName);
			if (attr != -1 && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
				ptr = SendVar->FullName;
				// パスの区切りを \ -> / へ
				setlocale(LC_ALL, ts.Locale);
				while (*ptr) {
					if (isleadbyte(*ptr)) { // multi-byte
						ptr += 2;
						continue;
					}
					if (*ptr == '\\')
						*ptr = '/';
					ptr++;
				}

				// パスに空白があればエスケープする
				q = tmpbuf;
				ptr = SendVar->FullName;
				while (*ptr) {
					if (*ptr == ' ')
						*q++ = '\\';
					*q++ = *ptr;
					ptr++;
				}
				*q = '\0'; // null-terminate

				ptr = tmpbuf;

				// consoleへ送信
				while (*ptr) {
					FSOut1(*ptr);
					if (ts.LocalEcho > 0) {
						FSEcho1(*ptr);
					}
					ptr++;
				}
				FreeFileVar(&SendVar); // 解放を忘れずに

			} else {
				// Confirm send a file when drag and drop (2007.12.28 maya)
				if (ts.ConfirmFileDragAndDrop) {
					// いきなりファイルの内容を送り込む前に、ユーザに問い合わせを行う。(2006.1.21 yutaka)
					// MessageBoxでSCPも選択できるようにする。(2008.1.25 yutaka)
					char uimsg[MAX_UIMSG];
					HHOOK hook = NULL;
					DWORD dwThreadID = GetCurrentThreadId();
					int ret;

					get_lang_msg("MSG_DANDD_CONF_TITLE", uimsg, sizeof(uimsg),
					             "Tera Term: File Drag and Drop", ts.UILanguageFile);
					get_lang_msg("MSG_DANDD_CONF", ts.UIMsg, sizeof(ts.UIMsg),
					             "Are you sure that you want to send the file content?", ts.UILanguageFile);

					hook = SetWindowsHookEx( WH_CBT, MsgBoxHootProc, NULL, dwThreadID );
					if (cv.isSSH == 2) {
						ret = MessageBox(ts.UIMsg, uimsg, MB_YESNOCANCEL | MB_ICONINFORMATION | MB_DEFBUTTON3);
					}
					else {
						// SSH2 接続ではない場合には "SCP" を出さない (2008.1.25 maya)
						ret = MessageBox(ts.UIMsg, uimsg, MB_OKCANCEL | MB_ICONINFORMATION | MB_DEFBUTTON2);
					}
					UnhookWindowsHookEx( hook );

					if (ret == IDOK || ret == IDYES) {   // sendfile
						SendVar->DirLen = 0;
						ts.TransBin = 0;
						FileSendStart();

					} else if (ret == IDNO) {   // SCP
						typedef int (CALLBACK *PSSH_start_scp)(char *, char *);
						static PSSH_start_scp func = NULL;
						static HMODULE h = NULL, h2 = NULL;
						char msg[128];

						if (func == NULL) {
							h2 = LoadLibrary("ttxssh.dll");
							if ( ((h = GetModuleHandle("ttxssh.dll")) == NULL) ) {
								_snprintf_s(msg, sizeof(msg), _TRUNCATE, "GetModuleHandle(\"ttxssh.dll\")) %d", GetLastError());
								goto scp_send_error;
							}
							func = (PSSH_start_scp)GetProcAddress(h, "TTXScpSendfile");
							if (func == NULL) {
								_snprintf_s(msg, sizeof(msg), _TRUNCATE, "GetProcAddress(\"TTXScpSendfile\")) %d", GetLastError());
								goto scp_send_error;
							}
						}

						if (func != NULL) {
							func(SendVar->FullName, NULL);
							goto send_success;
						} 

scp_send_error:
						::MessageBox(NULL, msg, "Tera Term: scpsend command error", MB_OK | MB_ICONERROR);
						FreeLibrary(h2);
send_success:
						FreeFileVar(&SendVar);  // 解放を忘れずに

					} else {
						FreeFileVar(&SendVar);

					}
				}
				else {
					SendVar->DirLen = 0;
					ts.TransBin = 0;
					FileSendStart();

				}
			}
		}
		else
			FreeFileVar(&SendVar);
	}
	DragFinish(hDropInfo);
}

void CVTWindow::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
#ifndef WINDOW_MAXMIMUM_ENABLED
	lpMMI->ptMaxSize.x = 10000;
	lpMMI->ptMaxSize.y = 10000;
	lpMMI->ptMaxTrackSize.x = 10000;
	lpMMI->ptMaxTrackSize.y = 10000;
#endif
}

void CVTWindow::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
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

void CVTWindow::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	InitMenuPopup(pPopupMenu->m_hMenu);
}

void CVTWindow::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	BYTE KeyState[256];
	MSG M;

	switch (KeyDown(HVTWin,nChar,nRepCnt,nFlags & 0x1ff)) {
	case KEYDOWN_OTHER:
		break;
	case KEYDOWN_CONTROL:
		return;
	case KEYDOWN_COMMOUT:
		/* 最下行でだけ自動スクロールする設定の場合
		   リモートへのキー入力送信でスクロールさせる */
		if (ts.AutoScrollOnlyInBottomLine != 0 && WinOrgY != 0) {
			DispVScroll(SCROLL_BOTTOM, 0);
		}
		return;
	}


	if ((ts.MetaKey>0) && ((nFlags & 0x2000) != 0)) {
		/* for Ctrl+Alt+Key combination */
		GetKeyboardState((PBYTE)KeyState);
		KeyState[VK_MENU] = 0;
		SetKeyboardState((PBYTE)KeyState);
		M.hwnd = HVTWin;
		M.message = WM_KEYDOWN;
		M.wParam = nChar;
		M.lParam = MAKELONG(nRepCnt,nFlags & 0xdfff);
		TranslateMessage(&M);

	} else {
		// ScrollLockキーが点灯している場合は、マウスをクリックしっぱなし状態であると
		// 見なす。すなわち、パージング処理が一時停止する。
		// 当該キーを消灯させると、処理が再開される。(2006.11.14 yutaka)
#if 0
		GetKeyboardState((PBYTE)KeyState);
		if (KeyState[VK_SCROLL] == 0x81) { // on : scroll locked
			ScrollLock = TRUE;
		} else if (KeyState[VK_SCROLL] == 0x80) { // off : scroll unlocked
			ScrollLock = FALSE;
		} else {
			// do nothing
		}
#endif
	}

}

void CVTWindow::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	KeyUp(nChar);
}

void CVTWindow::OnKillFocus(CWnd* pNewWnd)
{
	DispDestroyCaret();
	FocusReport(FALSE);
	CFrameWnd::OnKillFocus(pNewWnd);

	if (IsCaretOn()) {
		CaretKillFocus(TRUE);
	}
}

void CVTWindow::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (LButton || MButton || RButton) {
		return;
	}

	DblClkX = point.x;
	DblClkY = point.y;

	if (! MouseReport(IdMouseEventBtnDown, IdLeftButton, DblClkX, DblClkY) &&
		BuffUrlDblClk(DblClkX, DblClkY)) { // ブラウザ呼び出しの場合は何もしない。 (2005.4.3 yutaka)
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

void CVTWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
	POINT p;

	p.x = point.x;
	p.y = point.y;
	ButtonDown(p,IdLeftButton);
}

void CVTWindow::OnLButtonUp(UINT nFlags, CPoint point)
{
	MouseReport(IdMouseEventBtnUp, IdLeftButton, point.x, point.y);

	if (! LButton) {
		return;
	}

	ButtonUp(FALSE);
}

void CVTWindow::OnMButtonDown(UINT nFlags, CPoint point)
{
	POINT p;

	p.x = point.x;
	p.y = point.y;
	ButtonDown(p,IdMiddleButton);
}

void CVTWindow::OnMButtonUp(UINT nFlags, CPoint point)
{
	BOOL mousereport;

	mousereport = MouseReport(IdMouseEventBtnUp, IdMiddleButton, point.x, point.y);

	if (! MButton) {
		return;
	}

	// added DisablePasteMouseMButton (2008.3.2 maya)
	if (ts.DisablePasteMouseMButton) {
		ButtonUp(FALSE);
	}
	else {
		ButtonUp(TRUE);
	}
}

int CVTWindow::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	if ((ts.SelOnActive==0) &&
	    (nHitTest==HTCLIENT))   //disable mouse event for text selection
		return MA_ACTIVATEANDEAT; //     when window is activated
	else
		return MA_ACTIVATE;
}

void CVTWindow::OnMouseMove(UINT nFlags, CPoint point)
{
	int i;

	if (! (LButton || MButton || RButton)) {
		// マウスカーソル直下にURL文字列があるかを走査する (2005.4.2 yutaka)
		BuffChangeSelect(point.x, point.y,0);
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
	CPoint pt      // カーソル位置
)
{
	int line, i;

	::ScreenToClient(HVTWin, &pt);

	line = abs(zDelta) / WHEEL_DELTA; // ライン数
	if (line < 1) line = 1;

	// 一スクロールあたりの行数に変換する (2008.4.6 yutaka)
	if (line == 1 && ts.MouseWheelScrollLine > 0)
		line *= ts.MouseWheelScrollLine;

	if (MouseReport(IdMouseEventWheel, zDelta<0, pt.x, pt.y))
		return TRUE;

	if (ts.TranslateWheelToCursor && AppliCursorMode && !ts.DisableAppCursor &&
	    !(ControlKey() && ts.DisableWheelToCursorByCtrl)) {
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


void CVTWindow::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
	if (! Minimized && (nHitTest == HTCAPTION)) {
		DispRestoreWinSize();
	}
	else {
		CFrameWnd::OnNcLButtonDblClk(nHitTest,point);
	}
}

void CVTWindow::OnNcRButtonDown(UINT nHitTest, CPoint point)
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
	CDC *cdc;
	HDC PaintDC;
	int Xs, Ys, Xe, Ye;

#ifdef ALPHABLEND_TYPE2
//<!--by AKASI
	BGSetupPrimary(FALSE);
//-->
#endif

	cdc = BeginPaint(&ps);
	PaintDC = cdc->GetSafeHdc();

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

void CVTWindow::OnRButtonDown(UINT nFlags, CPoint point)
{
	POINT p;

	p.x = point.x;
	p.y = point.y;
	ButtonDown(p,IdRightButton);
}

void CVTWindow::OnRButtonUp(UINT nFlags, CPoint point)
{
	BOOL mousereport;

	mousereport = MouseReport(IdMouseEventBtnUp, IdRightButton, point.x, point.y);

	if (! RButton) {
		return;
	}

	// 右ボタン押下でのペーストを禁止する (2005.3.16 yutaka)
	if (ts.DisablePasteMouseRButton || mousereport) {
		ButtonUp(FALSE);
	} else {
		ButtonUp(TRUE);
	}
}

void CVTWindow::OnSetFocus(CWnd* pOldWnd)
{
	ChangeCaret();
	FocusReport(TRUE);
	CFrameWnd::OnSetFocus(pOldWnd);
}


//
// リサイズツールチップ(based on PuTTY sizetip.c)
// 
static ATOM tip_class = 0;
static HFONT tip_font;
static COLORREF tip_bg;
static COLORREF tip_text;
static HWND tip_wnd = NULL;
static int tip_enabled = 0;

static LRESULT CALLBACK SizeTipWndProc(HWND hWnd, UINT nMsg,
                                       WPARAM wParam, LPARAM lParam)
{

	switch (nMsg) {
		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT:
			{
				HBRUSH hbr;
				HGDIOBJ holdbr;
				RECT cr;
				int wtlen;
				LPTSTR wt;
				HDC hdc;

				PAINTSTRUCT ps;
				hdc = BeginPaint(hWnd, &ps);

				SelectObject(hdc, tip_font);
				SelectObject(hdc, GetStockObject(BLACK_PEN));

				hbr = CreateSolidBrush(tip_bg);
				holdbr = SelectObject(hdc, hbr);

				GetClientRect(hWnd, &cr);
				Rectangle(hdc, cr.left, cr.top, cr.right, cr.bottom);

				wtlen = GetWindowTextLength(hWnd);
				wt = (LPTSTR) malloc((wtlen + 1) * sizeof(TCHAR));
				GetWindowText(hWnd, wt, wtlen + 1);

				SetTextColor(hdc, tip_text);
				SetBkColor(hdc, tip_bg);

				TextOut(hdc, cr.left + 3, cr.top + 3, wt, wtlen);

				free(wt);

				SelectObject(hdc, holdbr);
				DeleteObject(hbr);

				EndPaint(hWnd, &ps);
			}
			return 0;

		case WM_NCHITTEST:
			return HTTRANSPARENT;

		case WM_DESTROY:
			DeleteObject(tip_font);
			tip_font = NULL;
			break;

		case WM_SETTEXT:
			{
				LPCTSTR str = (LPCTSTR) lParam;
				SIZE sz;
				HDC hdc = CreateCompatibleDC(NULL);

				SelectObject(hdc, tip_font);
				GetTextExtentPoint32(hdc, str, _tcslen(str), &sz);

				SetWindowPos(hWnd, NULL, 0, 0, sz.cx + 6, sz.cy + 6,
				             SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
				InvalidateRect(hWnd, NULL, FALSE);

				DeleteDC(hdc);
			}
			break;
	}

	return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

static void UpdateSizeTip(HWND src, int cx, int cy)
{
	TCHAR str[32];

	if (!tip_enabled)
		return;

	if (!tip_wnd) {
		NONCLIENTMETRICS nci;

		/* First make sure the window class is registered */

		if (!tip_class) {
			WNDCLASS wc;
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = SizeTipWndProc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = hInst;
			wc.hIcon = NULL;
			wc.hCursor = NULL;
			wc.hbrBackground = NULL;
			wc.lpszMenuName = NULL;
			wc.lpszClassName = "SizeTipClass";

			tip_class = RegisterClass(&wc);
		}
#if 0
		/* Default values based on Windows Standard color scheme */

		tip_font = GetStockObject(SYSTEM_FONT);
		tip_bg = RGB(255, 255, 225);
		tip_text = RGB(0, 0, 0);
#endif

		/* Prepare other GDI objects and drawing info */

		tip_bg = GetSysColor(COLOR_INFOBK);
		tip_text = GetSysColor(COLOR_INFOTEXT);

		memset(&nci, 0, sizeof(NONCLIENTMETRICS));
		nci.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
			sizeof(NONCLIENTMETRICS), &nci, 0);
		tip_font = CreateFontIndirect(&nci.lfStatusFont);
	}

	/* Generate the tip text */

	sprintf(str, "%dx%d", cx, cy);

	if (!tip_wnd) {
		HDC hdc;
		SIZE sz;
		RECT wr;
		int ix, iy;

		/* calculate the tip's size */

		hdc = CreateCompatibleDC(NULL);
		GetTextExtentPoint32(hdc, str, _tcslen(str), &sz);
		DeleteDC(hdc);

		GetWindowRect(src, &wr);

		ix = wr.left;
		if (ix < 16) {
			ix = 16;
		}

		iy = wr.top - sz.cy;
		if (iy < 16) {
			iy = 16;
		}

		/* Create the tip window */

		tip_wnd =
			CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
			MAKEINTRESOURCE(tip_class), str, WS_POPUP, ix,
			iy, sz.cx, sz.cy, NULL, NULL, hInst, NULL);

		ShowWindow(tip_wnd, SW_SHOWNOACTIVATE);

	} else {

		/* Tip already exists, just set the text */

		SetWindowText(tip_wnd, str);
	}
}

static void EnableSizeTip(int bEnable)
{
	if (tip_wnd && !bEnable) {
		DestroyWindow(tip_wnd);
		tip_wnd = NULL;
	}

	tip_enabled = bEnable;
}

void CVTWindow::OnSize(UINT nType, int cx, int cy)
{
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

// 「ドラッグ中にウィンドウの内容を表示する」にチェックが入っている場合、
// リサイズ中は常に"WM_SIZING"が飛んでくるため、リサイズツールチップを
// 再描画する。
// (2008.8.1 yutaka)
void CVTWindow::OnSizing(UINT fwSide, LPRECT pRect)
{
	int nWidth;
	int nHeight;
	RECT cr, wr;
	int extra_width, extra_height;
	int w, h;

	::GetWindowRect(HVTWin, &wr);
	::GetClientRect(HVTWin, &cr);

	extra_width = wr.right - wr.left - cr.right + cr.left;
	extra_height = wr.bottom - wr.top - cr.bottom + cr.top;
	nWidth = (pRect->right) - (pRect->left) - extra_width;
	nHeight = (pRect->bottom) - (pRect->top) - extra_height;

	w = nWidth / FontWidth;
	h = nHeight / FontHeight;
	UpdateSizeTip(HVTWin, w, h);
}

void CVTWindow::OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	char e = ESC;
	char Code;
	unsigned int i;

#ifdef WINDOW_MAXMIMUM_ENABLED
	// ALT + xを押下すると WM_SYSCHAR が飛んでくる。
	// ALT + Enterでウィンドウの最大化 (2005.4.24 yutaka)
	if (AltKey() && nChar == 13) {
		if (IsZoomed()) { // window is maximum
			ShowWindow(SW_RESTORE);
		} else {
			ShowWindow(SW_MAXIMIZE);
		}
	}
#endif

	if (ts.MetaKey>0)
	{
		if (!KeybEnabled || (TalkStatus!=IdTalkKeyb)) return;
		Code = nChar;
		for (i=1 ; i<=nRepCnt ; i++)
		{
			CommTextOut(&cv,&e,1);
			CommTextOut(&cv,&Code,1);
			if (ts.LocalEcho>0)
			{
				CommTextEcho(&cv,&e,1);
				CommTextEcho(&cv,&Code,1);
			}
		}
		return;
	}
	CFrameWnd::OnSysChar(nChar, nRepCnt, nFlags);
}

void CVTWindow::OnSysColorChange()
{
	CFrameWnd::OnSysColorChange();
}

void CVTWindow::OnSysCommand(UINT nID, LPARAM lParam)
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
	else {
		CFrameWnd::OnSysCommand(nID,lParam);
	}
}

void CVTWindow::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ((nChar==VK_F10) ||
	    (ts.MetaKey>0) &&
	    ((MainMenu==NULL) || (nChar!=VK_MENU)) &&
		((nFlags & 0x2000) != 0)) {
		KeyDown(HVTWin,nChar,nRepCnt,nFlags & 0x1ff);
		// OnKeyDown(nChar,nRepCnt,nFlags);
	}
	else {
		CFrameWnd::OnSysKeyDown(nChar,nRepCnt,nFlags);
	}
}

void CVTWindow::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_F10) {
		OnKeyUp(nChar,nRepCnt,nFlags);
	}
	else {
		CFrameWnd::OnSysKeyUp(nChar,nRepCnt,nFlags);
	}
}

void CVTWindow::OnTimer(UINT nIDEvent)
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
		case IdDblClkTimer:
			AfterDblClk = FALSE;
			break;
		case IdComEndTimer:
			if (! CommCanClose(&cv)) {
				// wait if received data remains
				SetTimer(IdComEndTimer,1,NULL);
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

void CVTWindow::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
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

//<!--by AKASI
LONG CVTWindow::OnWindowPosChanging(UINT wParam, LONG lParam)
{
#ifdef ALPHABLEND_TYPE2
	if(BGEnable && BGNoCopyBits) {
		((WINDOWPOS*)lParam)->flags |= SWP_NOCOPYBITS;
	}
#endif

	return CFrameWnd::DefWindowProc(WM_WINDOWPOSCHANGING,wParam,lParam);
}

LONG CVTWindow::OnSettingChange(UINT wParam, LONG lParam)
{
#ifdef ALPHABLEND_TYPE2
	BGOnSettingChange();
#endif
	return CFrameWnd::DefWindowProc(WM_SETTINGCHANGE,wParam,lParam);
}

LONG CVTWindow::OnEnterSizeMove(UINT wParam, LONG lParam)
{
	EnableSizeTip(1);

#ifdef ALPHABLEND_TYPE2
	BGOnEnterSizeMove();
#endif
	return CFrameWnd::DefWindowProc(WM_ENTERSIZEMOVE,wParam,lParam);
}

LONG CVTWindow::OnExitSizeMove(UINT wParam, LONG lParam)
{
#ifdef ALPHABLEND_TYPE2
	BGOnExitSizeMove();
#endif

	EnableSizeTip(0);

	return CFrameWnd::DefWindowProc(WM_EXITSIZEMOVE,wParam,lParam);
}
//-->

LONG CVTWindow::OnIMEComposition(UINT wParam, LONG lParam)
{
	HGLOBAL hstr;
	//LPSTR lpstr;
	wchar_t *lpstr;
	int Len;
	char *mbstr;
	int mlen;

	if (CanUseIME()) {
		hstr = GetConvString(wParam, lParam);
	}
	else {
		hstr = NULL;
	}

	if (hstr!=NULL) {
		//lpstr = (LPSTR)GlobalLock(hstr);
		lpstr = (wchar_t *)GlobalLock(hstr);
		if (lpstr!=NULL) {
			mlen = wcstombs(NULL, lpstr, 0);
			mbstr = (char *)malloc(sizeof(char) * (mlen + 1));
			if (mbstr == NULL) {
				goto skip;
			}
			Len = wcstombs(mbstr, lpstr, mlen + 1);

			// add this string into text buffer of application
			Len = strlen(mbstr);
			if (Len==1) {
				switch (mbstr[0]) {
				case 0x20:
					if (ControlKey()) {
						mbstr[0] = 0; /* Ctrl-Space */
					}
					break;
				case 0x5c: // Ctrl-\ support for NEC-PC98
					if (ControlKey()) {
						mbstr[0] = 0x1c;
					}
					break;
				}
			}
			if (ts.LocalEcho>0) {
				CommTextEcho(&cv,mbstr,Len);
			}
			CommTextOut(&cv,mbstr,Len);

			free(mbstr);
			GlobalUnlock(hstr);
		}
skip:
		GlobalFree(hstr);
		return 0;
	}
	return CFrameWnd::DefWindowProc(WM_IME_COMPOSITION,wParam,lParam);
}

LONG CVTWindow::OnAccelCommand(UINT wParam, LONG lParam)
{
	switch (wParam) {
		case IdScrollLock:
			ScrollLock = ! ScrollLock;
			break;

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
			SelectNextWin(HVTWin,1);
			break;
		case IdCmdPrevWin:
			SelectNextWin(HVTWin,-1);
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
			OnFileDisconnect();
			break;
		case IdCmdLoadKeyMap: // called by TTMACRO
			SetKeyMap();
			break;
		case IdCmdRestoreSetup: // called by TTMACRO
			RestoreSetup();
			break;
	}
	return 0;
}

LONG CVTWindow::OnChangeMenu(UINT wParam, LONG lParam)
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
			             (int)WinMenu, ts.UIMsg);
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

LONG CVTWindow::OnChangeTBar(UINT wParam, LONG lParam)
{
	BOOL TBar;
	DWORD Style,ExStyle;
	HMENU SysMenu;

	Style = GetWindowLong (HVTWin, GWL_STYLE);
	ExStyle = GetWindowLong (HVTWin, GWL_EXSTYLE);
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
	SetWindowLong(HVTWin, GWL_STYLE, Style);
#ifdef ALPHABLEND_TYPE2
	SetWindowLong(HVTWin, GWL_EXSTYLE, ExStyle);
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

LONG CVTWindow::OnCommNotify(UINT wParam, LONG lParam)
{
	switch (LOWORD(lParam)) {
		case FD_READ:  // TCP/IP
			CommProcRRQ(&cv);
			break;
		case FD_CLOSE:
			Connecting = FALSE;
			TCPIPClosed = TRUE;
			// disable transmition
			cv.OutBuffCount = 0;
			SetTimer(IdComEndTimer,1,NULL);
			break;
	}
	return 0;
}

LONG CVTWindow::OnCommOpen(UINT wParam, LONG lParam)
{
	CommStart(&cv,lParam,&ts);
#ifndef NO_INET6
	if (ts.PortType == IdTCPIP && cv.RetryWithOtherProtocol == TRUE) {
		Connecting = TRUE;
	}
	else {
		Connecting = FALSE;
	}
#else
	Connecting = FALSE;
#endif /* NO_INET6 */
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

			// 直前の接続が非Telnet接続だった場合、Window を閉じていないと設定が残っている
			// (LocalEcho, CRSend が TCPLocalEcho, TCPCRSend の値で上書きされている)
			// Telnet のときには LocalEcho/CRSend を元の値に戻すために、
			// 読み込み時の設定を持ってくる (2008.4.22 maya)
			ts.CRSend = ts.CRSend_ini;
			cv.CRSend = ts.CRSend_ini;
			ts.LocalEcho = ts.LocalEcho_ini;

			TelStartKeepAliveThread();
		}
		// SSH, Cygwin などで接続するときに利用される
		else if (ts.DisableTCPEchoCR) {
			// 上と同じ理由で、読み込み時の設定を持ってくる (2008.4.22 maya)
			ts.CRSend = ts.CRSend_ini;
			cv.CRSend = ts.CRSend_ini;
			ts.LocalEcho = ts.LocalEcho_ini;
		}
		else {
			if (ts.TCPCRSend>0) {
				ts.CRSend = ts.TCPCRSend;
				cv.CRSend = ts.TCPCRSend;
			}
			if (ts.TCPLocalEcho>0) {
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

LONG CVTWindow::OnCommStart(UINT wParam, LONG lParam)
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
		CommOpen(HVTWin,&ts,&cv);
	}
	return 0;
}

LONG CVTWindow::OnDdeEnd(UINT wParam, LONG lParam)
{
	EndDDE();
	if (CloseTT) {
		OnClose();
	}
	return 0;
}

LONG CVTWindow::OnDlgHelp(UINT wParam, LONG lParam)
{
	OpenHelp(HVTWin,HH_HELP_CONTEXT,HelpId);
	return 0;
}

LONG CVTWindow::OnFileTransEnd(UINT wParam, LONG lParam)
{
	FileTransEnd(wParam);
	return 0;
}

LONG CVTWindow::OnGetSerialNo(UINT wParam, LONG lParam)
{
	return (LONG)SerialNo;
}

LONG CVTWindow::OnKeyCode(UINT wParam, LONG lParam)
{
	KeyCodeSend(wParam,(WORD)lParam);
	return 0;
}

LONG CVTWindow::OnProtoEnd(UINT wParam, LONG lParam)
{
	ProtoDlgCancel();
	return 0;
}

LONG CVTWindow::OnChangeTitle(UINT wParam, LONG lParam)
{
	ChangeTitle();
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
#ifndef NO_INET6
	GetHNRec.ProtocolFamily = ts.ProtocolFamily;
#endif /* NO_INET6 */
	GetHNRec.ComPort = ts.ComPort;
	GetHNRec.MaxComPort = ts.MaxComPort;

	strncpy_s(Command, sizeof(Command),"ttermpro ", _TRUNCATE);
	GetHNRec.HostName = &Command[9];

	if (! LoadTTDLG()) {
		return;
	}

	if ((*GetHostName)(HVTWin,&GetHNRec)) {
		if ((GetHNRec.PortType==IdTCPIP) &&
		    (ts.HistoryList>0) &&
		    LoadTTSET()) {
			(*AddHostToList)(ts.SetupFName,GetHNRec.HostName);
			FreeTTSET();
		}

		if (! cv.Ready) {
			ts.PortType = GetHNRec.PortType;
			ts.Telnet = GetHNRec.Telnet;
			ts.TCPPort = GetHNRec.TCPPort;
#ifndef NO_INET6
			ts.ProtocolFamily = GetHNRec.ProtocolFamily;
#endif /* NO_INET6 */
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
				char comport[4];
				Command[8] = 0;
				strncat_s(Command,sizeof(Command)," /C=",_TRUNCATE);
				_snprintf_s(comport, sizeof(comport), _TRUNCATE, "%d", GetHNRec.ComPort);
				strncat_s(Command,sizeof(Command),comport,_TRUNCATE);
			}
			else {
				char tcpport[6];
				strncpy_s(Command2, sizeof(Command2), &Command[9], _TRUNCATE);
				Command[9] = 0;
				if (GetHNRec.Telnet==0)
					strncat_s(Command,sizeof(Command)," /T=0",_TRUNCATE);
				else
					strncat_s(Command,sizeof(Command)," /T=1",_TRUNCATE);
				if (GetHNRec.TCPPort<65535) {
					strncat_s(Command,sizeof(Command)," /P=",_TRUNCATE);
					_snprintf_s(tcpport, sizeof(tcpport), _TRUNCATE, "%d", GetHNRec.TCPPort);
					strncat_s(Command,sizeof(Command),tcpport,_TRUNCATE);
				}
#ifndef NO_INET6
				/********************************/
				/* ここにプロトコル処理を入れる */
				/********************************/
				if (GetHNRec.ProtocolFamily == AF_INET) {
					strncat_s(Command,sizeof(Command)," /4",_TRUNCATE);
				} else if (GetHNRec.ProtocolFamily == AF_INET6) {
					strncat_s(Command,sizeof(Command)," /6",_TRUNCATE);
				}
#endif /* NO_INET6 */
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
	char Command[MAX_PATH];
	char *exec = "ttermpro";
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	
	// 現在の設定内容を共有メモリへコピーしておく
	CopyTTSetToShmem(&ts);

	if (ts.TCPPort != 23 && (strcmp(ts.HostName, "127.0.0.1") == 0 ||
	    strcmp(ts.HostName, "localhost") == 0)) {
		// localhostへの接続でポートが23以外の時はcygwin接続とみなす。
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

	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(NULL, Command, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
		char buf[80];
		char uimsg[MAX_UIMSG];
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_EXEC_TT_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Can't execute Tera Term. (%d)", ts.UILanguageFile);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts.UIMsg, GetLastError());
		::MessageBox(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
	}
}


//
// Connect to local cygwin
//
void CVTWindow::OnCygwinConnection()
{
	char file[MAX_PATH];
	char c, *envptr, *envbuff=NULL;
	int envbufflen;
	char *exename = "cygterm.exe";
	char cygterm[MAX_PATH];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char uimsg[MAX_UIMSG];

	envptr = getenv("PATH");
	if (strstr(envptr, "cygwin\\bin") != NULL) {
		goto found_path;
	}

	_snprintf_s(file, sizeof(file), _TRUNCATE, "%s\\bin", ts.CygwinDirectory);
	if (GetFileAttributes(file) == -1) { // open error
		for (c = 'C' ; c <= 'Z' ; c++) {
			file[0] = c;
			if (GetFileAttributes(file) != -1) { // open success
				goto found_dll;
			}
		}
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_FIND_CYGTERM_DIR_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Can't find Cygwin directory.", ts.UILanguageFile);
		::MessageBox(NULL, ts.UIMsg, uimsg, MB_OK | MB_ICONWARNING);
		return;
	}
found_dll:;
	if (envptr != NULL) {
		envbufflen = strlen(file) + strlen(envptr) + 7; // "PATH="(5) + ";"(1) + NUL(1)
		if ((envbuff = (char *)malloc(envbufflen)) == NULL) {
			get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
			get_lang_msg("MSG_CYGTERM_ENV_ALLOC_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
			             "Can't allocate memory for environment variable.", ts.UILanguageFile);
			::MessageBox(NULL, ts.UIMsg, uimsg, MB_OK | MB_ICONWARNING);
			return;
		}
		_snprintf_s(envbuff, envbufflen, _TRUNCATE, "PATH=%s;%s", file, envptr);
	} else {
		envbufflen = strlen(file) + 6; // "PATH="(5) + NUL(1)
		if ((envbuff = (char *)malloc(envbufflen)) == NULL) {
			get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
			get_lang_msg("MSG_CYGTERM_ENV_ALLOC_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
			             "Can't allocate memory for environment variable.", ts.UILanguageFile);
			::MessageBox(NULL, ts.UIMsg, uimsg, MB_OK | MB_ICONWARNING);
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
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_EXEC_CYGTERM_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Can't execute Cygterm.", ts.UILanguageFile);
		::MessageBox(NULL, ts.UIMsg, uimsg, MB_OK | MB_ICONWARNING);
	}
}


//
// TeraTerm Menuの起動
//
void CVTWindow::OnTTMenuLaunch()
{
	char *exename = "ttpmenu.exe";
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(NULL, exename, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
		char buf[80];
		char uimsg[MAX_UIMSG];
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_EXEC_TTMENU_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Can't execute TeraTerm Menu. (%d)", ts.UILanguageFile);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts.UIMsg, GetLastError());
		::MessageBox(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
	}
}


//
// LogMeTTの起動
//
void CVTWindow::OnLogMeInLaunch()
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char LogMeTT[MAX_PATH];

	if (!isLogMeTTExist()) {
		return;
	}

	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	strncpy_s(LogMeTT, sizeof(LogMeTT), ts.HomeDir, _TRUNCATE);
	AppendSlash(LogMeTT, sizeof(LogMeTT));
	strncat_s(LogMeTT, sizeof(LogMeTT), LogMeTTexename, _TRUNCATE);

	if (CreateProcess(NULL, LogMeTT, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
		char buf[80];
		char uimsg[MAX_UIMSG];
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_EXEC_LOGMETT_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Can't execute LogMeTT. (%d)", ts.UILanguageFile);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts.UIMsg, GetLastError());
		::MessageBox(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
	}
}


void CVTWindow::OnFileLog()
{
	HelpId = HlpFileLog;
	LogStart();
}


static LRESULT CALLBACK OnCommentDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	char buf[256];
	UINT ret;
	LOGFONT logfont;
	HFONT font;
	char uimsg[MAX_UIMSG];

	switch (msg) {
		case WM_INITDIALOG:
			//SetDlgItemText(hDlgWnd, IDC_EDIT_COMMENT, "サンプル");
			// エディットコントロールにフォーカスをあてる
			SetFocus(GetDlgItem(hDlgWnd, IDC_EDIT_COMMENT));

			font = (HFONT)SendMessage(hDlgWnd, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", hDlgWnd, &logfont, &DlgCommentFont, ts.UILanguageFile)) {
				SendDlgItemMessage(hDlgWnd, IDC_EDIT_COMMENT, WM_SETFONT, (WPARAM)DlgCommentFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hDlgWnd, IDOK, WM_SETFONT, (WPARAM)DlgCommentFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgCommentFont = NULL;
			}

			GetWindowText(hDlgWnd, uimsg, sizeof(uimsg));
			get_lang_msg("DLG_COMMENT_TITLE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetWindowText(hDlgWnd, ts.UIMsg);
			GetDlgItemText(hDlgWnd, IDOK, uimsg, sizeof(uimsg));
			get_lang_msg("BTN_OK", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetDlgItemText(hDlgWnd, IDOK, ts.UIMsg);

			return FALSE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					memset(buf, 0, sizeof(buf));
					ret = GetDlgItemText(hDlgWnd, IDC_EDIT_COMMENT, buf, sizeof(buf) - 1);
					if (ret > 0) { // テキスト取得成功
						//buf[sizeof(buf) - 1] = '\0';  // null-terminate
						CommentLogToFile(buf, ret);
					}
					if (DlgCommentFont != NULL) {
						DeleteObject(DlgCommentFont);
					}
					EndDialog(hDlgWnd, IDOK);
					break;
				default:
					return FALSE;
			}
		case WM_CLOSE:
			EndDialog(hDlgWnd, 0);
			return TRUE;

		default:
			return FALSE;
	}
	return TRUE;
}

void CVTWindow::OnCommentToLog()
{
	DWORD ret;

	// ログファイルへコメントを追加する (2004.8.6 yutaka)
	ret = DialogBox(hInst, MAKEINTRESOURCE(IDD_COMMENT_DIALOG),
	                HVTWin, (DLGPROC)OnCommentDlgProc);
	if (ret == 0 || ret == -1) {
		ret = GetLastError();
	}

}


// ログの閲覧 (2005.1.29 yutaka)
void CVTWindow::OnViewLog()
{
	char command[MAX_PATH];
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

	_snprintf_s(command, sizeof(command), _TRUNCATE, "%s %s", ts.ViewlogEditor, file);

	if (CreateProcess(NULL, command, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
		char buf[80];
		char uimsg[MAX_UIMSG];
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_VIEW_LOGFILE_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Can't view logging file. (%d)", ts.UILanguageFile);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts.UIMsg, GetLastError());
		::MessageBox(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
	}
}


// 隠しているログダイアログを表示する (2008.2.3 maya)
void CVTWindow::OnShowLogDialog()
{
	ShowFTDlg(OpLog);
}


// ログの再生 (2006.12.13 yutaka)
void CVTWindow::OnReplayLog()
{
	OPENFILENAME ofn;
	char szFile[MAX_PATH];
	char Command[MAX_PATH] = "notepad.exe";
	char *exec = "ttermpro";
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
		char buf[80];
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_EXEC_TT_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "Can't execute Tera Term. (%d)", ts.UILanguageFile);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts.UIMsg, GetLastError());
		::MessageBox(NULL, buf, uimsg, MB_OK | MB_ICONWARNING);
	}
}


void CVTWindow::OnFileSend()
{
	HelpId = HlpFileSend;
	FileSendStart();
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
	(*ChangeDirectory)(HVTWin,ts.FileDir);
	FreeTTDLG();
}

void CVTWindow::OnFilePrint()
{
	HelpId = HlpFilePrint;
	BuffPrint(FALSE);
}

void CVTWindow::OnFileDisconnect()
{
	if (! cv.Ready) {
		return;
	}

	get_lang_msg("MSG_DISCONNECT_CONF", ts.UIMsg, sizeof(ts.UIMsg),
	             "Disconnect?", ts.UILanguageFile);
	if ((cv.PortType==IdTCPIP) &&
	    ((ts.PortFlag & PF_CONFIRMDISCONN) != 0) &&
	    (::MessageBox(HVTWin, ts.UIMsg, "Tera Term",
	                  MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2)==IDCANCEL)) {
		return;
	}

	::PostMessage(HVTWin, WM_USER_COMMNOTIFY, 0, FD_CLOSE);
}

void CVTWindow::OnFileExit()
{
	OnClose();
}

void CVTWindow::OnEditCopy()
{
	// copy selected text to clipboard
	BuffCBCopy(FALSE);
}

void CVTWindow::OnEditCopyTable()
{
	// copy selected text to clipboard in Excel format
	BuffCBCopy(TRUE);
}

void CVTWindow::OnEditPaste()
{
	// add confirm (2008.2.4 yutaka)
	if (CBStartPasteConfirmChange(HVTWin)) {
		CBStartPaste(HVTWin,FALSE,0,NULL,0);
		/* 最下行でだけ自動スクロールする設定の場合
		   ペースト処理でスクロールさせる */
		if (ts.AutoScrollOnlyInBottomLine != 0 && WinOrgY != 0) {
			DispVScroll(SCROLL_BOTTOM, 0);
		}
	}
}

void CVTWindow::OnEditPasteCR()
{
	// add confirm (2008.3.11 maya)
	if (CBStartPasteConfirmChange(HVTWin)) {
		CBStartPaste(HVTWin,TRUE,0,NULL,0);
		/* 最下行でだけ自動スクロールする設定の場合
		   ペースト処理でスクロールさせる */
		if (ts.AutoScrollOnlyInBottomLine != 0 && WinOrgY != 0) {
			DispVScroll(SCROLL_BOTTOM, 0);
		}
	}
}

void CVTWindow::OnEditClearScreen()
{
	LockBuffer();
	BuffClearScreen();
	if ((StatusLine>0) && (CursorY==NumOfLines-1)) {
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
	DWORD ret;

	CAddSettingPropSheetDlg CAddSetting("", CWnd::FromHandle(HVTWin));
	ret = CAddSetting.DoModal();
	switch (ret) {
		case -1:
		case IDABORT:
			ret = GetLastError();
			break;
		case IDOK:
#ifdef ALPHABLEND_TYPE2
			BGInitialize();
			BGSetupPrimary(TRUE);
#else
			DispApplyANSIColor();
#endif
			DispSetNearestColors(IdBack, IdFore+8, NULL);
			ChangeWin();
			break;
		default:
			/* nothing to do */
			break;
	}
}

void CVTWindow::OnSetupTerminal()
{
	BOOL Ok;

	if (ts.Language==IdRussian) {
		HelpId = HlpSetupTerminalRuss;
	}
	else {
		HelpId = HlpSetupTerminal;
	}
	if (! LoadTTDLG()) {
		return;
	}
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

	strncpy_s(orgTitle, sizeof(orgTitle), ts.Title, _TRUNCATE);
	Ok = (*SetupWin)(HVTWin, &ts);
	FreeTTDLG();

	if (Ok) {
		// Eterm lookfeelの画面情報も更新することで、リアルタイムでの背景色変更が
		// 可能となる。(2006.2.24 yutaka)
#ifdef ALPHABLEND_TYPE2
		BGInitialize();
		BGSetupPrimary(TRUE);
#endif

		// タイトルが変更されていたら、リモートタイトルをクリアする
		if ((ts.AcceptTitleChangeRequest == IdTitleChangeRequestOverwrite) &&
		    (strcmp(orgTitle, ts.Title) != 0)) {
			cv.TitleRemote[0] = '\0';
		}

		ChangeWin();
	}

}

void CVTWindow::OnSetupFont()
{
	HelpId = HlpSetupFont;
	DispSetupFontDlg();
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
	HelpId = HlpSetupSerialPort;
	if (! LoadTTDLG()) {
		return;
	}
	Ok = (*SetupSerialPort)(HVTWin, &ts);
	FreeTTDLG();

	if (Ok && ts.ComPort > 0) {
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
	if ((*SetupGeneral)(HVTWin,&ts)) {
		ResetCharSet();
		ResetIME();
	}
	FreeTTDLG();
}

void CVTWindow::OnSetupSave()
{
	BOOL Ok;
	char TmpSetupFN[MAXPATHLEN];
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
			char uimsg[MAX_UIMSG];
			get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: ERROR", ts.UILanguageFile);
			get_lang_msg("MSG_SAVESETUP_PERMISSION_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
			             "TERATERM.INI file doesn't have the writable permission.", ts.UILanguageFile);
			MessageBox(ts.UIMsg, uimsg, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
	}

	if (LoadTTSET())
	{
		int w, h;

#ifdef WINDOW_MAXMIMUM_ENABLED
		if (IsZoomed()) {
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
		if (IsZoomed()) {
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
				TelSendBreak();
				break;
			case IdSerial:
				CommSendBreak(&cv);
				break;
		}
}

void CVTWindow::OnControlResetPort()
{
	CommResetSerial(&ts, &cv, TRUE);
}

void ApplyBoradCastCommandHisotry(HWND Dialog, char *historyfile)
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
static WNDPROC OrigHostnameEditProc; // Original window procedure
static HWND BroadcastWindowList;
static LRESULT CALLBACK HostnameEditProc(HWND dlg, UINT msg,
                                         WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_CREATE:
			break;

		case WM_DESTROY:
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
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
#if 0
				for (i = 0 ; i < MAXNWIN ; i++) { // 50 = MAXNWIN(@ ttcmn.c)
					hd = GetNthWin(i);
					if (hd == NULL)
						break;

					PostMessage(hd, msg, wParam, lParam);
					//PostMessage(hd, WM_SETFOCUS, NULL, 0);
				}
#else
				count = SendMessage(BroadcastWindowList, LB_GETCOUNT, 0, 0);
				for (i = 0 ; i < count ; i++) {
					if (SendMessage(BroadcastWindowList, LB_GETSEL, i, 0)) {
						hd = GetNthWin(i);
						if (hd) {
							PostMessage(hd, msg, wParam, lParam);
						}
					}
				}
#endif
			}
			break;

		default:
			return CallWindowProc(OrigHostnameEditProc, dlg, msg, wParam, lParam);
	}

	return FALSE;
}


static void UpdateBroadcastWindowList(HWND hWnd)
{
	int i;
	HWND hd;
    TCHAR szWindowText[256];

	SendMessage(hWnd, LB_RESETCONTENT, 0, 0);

	for (i = 0 ; i < MAXNWIN ; i++) { // 50 = MAXNWIN(@ ttcmn.c)
		hd = GetNthWin(i);
		if (hd == NULL) {
			break;
		}

		GetWindowText(hd, szWindowText, 256);
		SendMessage(hWnd, LB_INSERTSTRING, -1, (LPARAM)szWindowText);
	}

#if 0
	for (i = 0 ; i < SendMessage(BroadcastWindowList, LB_GETCOUNT, 0, 0) ; i++) {
		SendMessage(hWnd, LB_SETSEL, TRUE, i);
	}
#endif
}

/*
 * 全TeraTermへメッセージを送信するブロードキャストモード。
 * "sendbroadcast"マクロコマンドからも利用される。
 */
extern "C"
void SendAllBroadcastMessage(HWND HVTWin, HWND hWnd, int parent_only, char *buf, int buflen)
{
	int i;
	int count;
	HWND hd;
	COPYDATASTRUCT cds;

	// すべてのTera Termにメッセージとデータを送る
	count = SendMessage(BroadcastWindowList, LB_GETCOUNT, 0, 0);
	for (i = 0 ; i < count ; i++) { 
		if (parent_only) {
			hd = GetParent(hWnd);
			i = MAXNWIN;		// 337: 強引かつ直値 :P
		} else {
			// リストボックスで選択されているか
			if (SendMessage(BroadcastWindowList, LB_GETSEL, i, 0)) {
				hd = GetNthWin(i);
			}
		}
		if (hd == NULL) {
			break;
		}

		ZeroMemory(&cds, sizeof(cds));
		cds.dwData = IPC_BROADCAST_COMMAND;
		cds.cbData = buflen;
		cds.lpData = buf;

		// WM_COPYDATAを使って、プロセス間通信を行う。
		SendMessage(hd, WM_COPYDATA, (WPARAM)HVTWin, (LPARAM)&cds);

		// 送信先Tera Termウィンドウに適当なメッセージを送る。
		// これをしないと、送り込んだデータが反映されない模様。
		// (2006.2.7 yutaka)
		PostMessage(hd, WM_SETFOCUS, NULL, 0);
	}

}


/*
 * 任意のTeraTerm群へメッセージを送信するマルチキャストモード。厳密には、
 * ブロードキャスト送信を行い、受信側でメッセージを取捨選択する。
 * "sendmulticast"マクロコマンドからのみ利用される。
 */
extern "C"
void SendMulticastMessage(HWND HVTWin, HWND hWnd, char *name, char *buf, int buflen)
{
	int i;
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
	msg = (char *)malloc(msglen);
	if (msg == NULL) {
		goto error;
	}
	strcpy_s(msg, msglen, name);
	memcpy(msg + nlen, buf, buflen);

	// すべてのTera Termにメッセージとデータを送る
	for (i = 0 ; i < MAXNWIN ; i++) { // 50 = MAXNWIN(@ ttcmn.c)
		hd = GetNthWin(i);
		if (hd == NULL) {
			break;
		}

		ZeroMemory(&cds, sizeof(cds));
		cds.dwData = IPC_MULTICAST_COMMAND;
		cds.cbData = msglen;
		cds.lpData = msg;

		// WM_COPYDATAを使って、プロセス間通信を行う。
		SendMessage(hd, WM_COPYDATA, (WPARAM)HVTWin, (LPARAM)&cds);

		// 送信先Tera Termウィンドウに適当なメッセージを送る。
		// これをしないと、送り込んだデータが反映されない模様。
		// (2006.2.7 yutaka)
		PostMessage(hd, WM_SETFOCUS, NULL, 0);
	}

error:
	free(msg);
}


static char multicast_name[128];

extern "C"
void SetMulticastName(char *name)
{
	strncpy_s(multicast_name, sizeof(multicast_name), name, _TRUNCATE);
}

static int CompareMulticastName(char *name)
{
	return strcmp(multicast_name, name);
}

//
// すべてのターミナルへ同一コマンドを送信するモードレスダイアログの表示
// (2005.1.22 yutaka)
//
static LRESULT CALLBACK BroadcastCommandDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	char buf[256 + 3];
	UINT ret;
	LRESULT checked;
	LRESULT history;
	LOGFONT logfont;
	HFONT font;
	char uimsg[MAX_UIMSG];
	char historyfile[MAX_PATH];
	static HWND hwndHostname     = NULL; // HOSTNAME dropdown
	static HWND hwndHostnameEdit = NULL; // Edit control on HOSTNAME dropdown

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
			ApplyBoradCastCommandHisotry(hWnd, historyfile);

			// エディットコントロールにフォーカスをあてる
			SetFocus(GetDlgItem(hWnd, IDC_COMMAND_EDIT));

			// サブクラス化させてリアルタイムモードにする (2008.1.21 yutaka)
			hwndHostname = GetDlgItem(hWnd, IDC_COMMAND_EDIT);
			hwndHostnameEdit = GetWindow(hwndHostname, GW_CHILD);
			OrigHostnameEditProc = (WNDPROC)GetWindowLong(hwndHostnameEdit, GWL_WNDPROC);
			SetWindowLong(hwndHostnameEdit, GWL_WNDPROC, (LONG)HostnameEditProc);
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

			font = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", hWnd, &logfont, &DlgBroadcastFont, ts.UILanguageFile)) {
				SendDlgItemMessage(hWnd, IDC_COMMAND_EDIT, WM_SETFONT, (WPARAM)DlgBroadcastFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hWnd, IDC_HISTORY_CHECK, WM_SETFONT, (WPARAM)DlgBroadcastFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hWnd, IDC_RADIO_CRLF, WM_SETFONT, (WPARAM)DlgBroadcastFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hWnd, IDC_RADIO_CR, WM_SETFONT, (WPARAM)DlgBroadcastFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hWnd, IDC_RADIO_LF, WM_SETFONT, (WPARAM)DlgBroadcastFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hWnd, IDC_ENTERKEY_CHECK, WM_SETFONT, (WPARAM)DlgBroadcastFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hWnd, IDC_PARENT_ONLY, WM_SETFONT, (WPARAM)DlgBroadcastFont, MAKELPARAM(TRUE,0));	// 337: 2007/03/20
				SendDlgItemMessage(hWnd, IDC_REALTIME_CHECK, WM_SETFONT, (WPARAM)DlgBroadcastFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hWnd, IDOK, WM_SETFONT, (WPARAM)DlgBroadcastFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hWnd, IDCANCEL, WM_SETFONT, (WPARAM)DlgBroadcastFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgBroadcastFont = NULL;
			}
			GetWindowText(hWnd, uimsg, sizeof(uimsg));
			get_lang_msg("DLG_BROADCAST_TITLE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetWindowText(hWnd, ts.UIMsg);
			GetDlgItemText(hWnd, IDC_HISTORY_CHECK, uimsg, sizeof(uimsg));
			get_lang_msg("DLG_BROADCAST_HISTORY", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetDlgItemText(hWnd, IDC_HISTORY_CHECK, ts.UIMsg);
			GetDlgItemText(hWnd, IDC_ENTERKEY_CHECK, uimsg, sizeof(uimsg));
			get_lang_msg("DLG_BROADCAST_ENTER", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetDlgItemText(hWnd, IDC_ENTERKEY_CHECK, ts.UIMsg);
			GetDlgItemText(hWnd, IDC_PARENT_ONLY, uimsg, sizeof(uimsg));
			get_lang_msg("DLG_BROADCAST_PARENTONLY", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetDlgItemText(hWnd, IDC_PARENT_ONLY, ts.UIMsg);
			GetDlgItemText(hWnd, IDC_REALTIME_CHECK, uimsg, sizeof(uimsg));
			get_lang_msg("DLG_BROADCAT_REALTIME", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetDlgItemText(hWnd, IDC_REALTIME_CHECK, ts.UIMsg);
			GetDlgItemText(hWnd, IDOK, uimsg, sizeof(uimsg));
			get_lang_msg("DLG_BROADCAST_SUBMIT", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetDlgItemText(hWnd, IDOK, ts.UIMsg);
			GetDlgItemText(hWnd, IDCANCEL, uimsg, sizeof(uimsg));
			get_lang_msg("BTN_CLOSE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetDlgItemText(hWnd, IDCANCEL, ts.UIMsg);

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
					hwndHostname = GetDlgItem(hWnd, IDC_COMMAND_EDIT);
					hwndHostnameEdit = GetWindow(hwndHostname, GW_CHILD);
					OrigHostnameEditProc = (WNDPROC)GetWindowLong(hwndHostnameEdit, GWL_WNDPROC);
					SetWindowLong(hwndHostnameEdit, GWL_WNDPROC, (LONG)HostnameEditProc);

					EnableWindow(GetDlgItem(hWnd, IDC_HISTORY_CHECK), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_PARENT_ONLY), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_LIST), TRUE);  // true
				} else {
					// restore old handler
					SetWindowLong(hwndHostnameEdit, GWL_WNDPROC, (LONG)OrigHostnameEditProc);

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
							ApplyBoradCastCommandHisotry(hWnd, historyfile);
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

						// 337: 2007/03/20 チェックされていたら親ウィンドウにのみ送信
						checked = SendMessage(GetDlgItem(hWnd, IDC_PARENT_ONLY), BM_GETCHECK, 0, 0);

						SendAllBroadcastMessage(HVTWin, hWnd, checked, buf, strlen(buf));
					}

					// モードレスダイアログは一度生成されると、アプリケーションが終了するまで
					// 破棄されないので、以下の「ウィンドウプロシージャ戻し」は不要と思われる。(yutaka)
#if 0
					SetWindowLong(hwndHostnameEdit, GWL_WNDPROC, (LONG)OrigHostnameEditProc);
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
						ApplyBoradCastCommandHisotry(hWnd, historyfile);
					}
					return FALSE;

				case IDC_LIST:
					// リストボックスをダブルクリックされたら、全選択か全選択解除を行う。
					if (HIWORD(wp) == LBN_DBLCLK) {
						int i, n, max;
						BOOL flag;

						max = ListBox_GetCount(BroadcastWindowList);
						n = 0;
						for (i = 0 ; i < max ; i++) {
							if (ListBox_GetSel(BroadcastWindowList, i)) {
								n++;
							}
						}
						
						if (max == 2) {  // エントリが2個の場合は常に全選択とする。
							flag = TRUE;

						} else {
							if (n >= max - 1) { // all select
								flag = FALSE;
							}
							else {
								flag = TRUE;
							}

						}

						for (i = 0 ; i < max ; i++) {
							ListBox_SetSel(BroadcastWindowList, flag, i);
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
			if (DlgBroadcastFont != NULL) {
				DeleteObject(DlgBroadcastFont);
			}
			return TRUE;

		default:
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

	hDlgWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_BROADCAST_DIALOG),
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
LONG CVTWindow::OnReceiveIpcMessage(UINT wParam, LONG lParam)
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
		CBStartPaste(HVTWin, FALSE, TermWidthMax/*CBBufSize*/, buf, buflen);
		// 送信データがある場合は送信する
		if (TalkStatus == IdTalkCB) {
			CBSend();
		}
	}

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

void CVTWindow::OnWindowWindow()
{
	BOOL Close;

	HelpId = HlpWindowWindow;
	if (! LoadTTDLG()) {
		return;
	}
	(*WindowWindow)(HVTWin,&Close);
	FreeTTDLG();
	if (Close) {
		OnClose();
	}
}

void CVTWindow::OnHelpIndex()
{
	OpenHelp(HVTWin,HH_DISPLAY_TOPIC,0);
}

void CVTWindow::OnHelpAbout()
{
	if (! LoadTTDLG()) {
		return;
	}
	(*AboutDialog)(HVTWin);
	FreeTTDLG();
}
