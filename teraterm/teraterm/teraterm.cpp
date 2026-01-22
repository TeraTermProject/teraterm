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

/* TERATERM.EXE, main */

#include <stdio.h>
#include <crtdbg.h>
#include <windows.h>
#include <htmlhelp.h>
#include <locale.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "commlib.h"
#include "ttwinman.h"
#include "buffer.h"
#include "vtterm.h"
#include "vtwin.h"
#include "filesys.h"
#include "telnet.h"
#include "tekwin.h"
#include "ttdde.h"
#include "keyboard.h"
#include "dllutil.h"
#include "compat_win.h"
#include "dlglib.h"
#include "teraterml.h"
#include "sendmem.h"
#include "ttdebug.h"
#include "win32helper.h"
#include "asprintf.h"
#if ENABLE_GDIPLUS
#include "ttgdiplus.h"
#endif
#include "directx.h"
#include "unicode.h"

#if defined(_DEBUG) && defined(_MSC_VER)
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

static BOOL AddFontFlag;
static wchar_t *TSpecialFont;
CVTWindow* pVTWin;
static DWORD HtmlHelpCookie;
HANDLE hIdleTimer;

static void LoadSpecialFont(void)
{
	wchar_t *mod_path;
	if (IsExistFontW(L"Tera Special", SYMBOL_CHARSET, TRUE)) {
		// すでに存在するのでロードしない
		return;
	}

	if (hGetModuleFileNameW(NULL, &mod_path) != 0) {
		AddFontFlag = FALSE;
		return;
	}
	*wcsrchr(mod_path, L'\\') = 0;
	aswprintf(&TSpecialFont, L"%s\\TSPECIAL1.TTF", mod_path);
	free(mod_path);

	// teraterm.exeのみで有効なフォントとなる。
	// removeしなくても終了するとOSからなくなる
	int r = 0;
	if (pAddFontResourceExW != NULL) {
		r = pAddFontResourceExW(TSpecialFont, FR_PRIVATE, NULL);
	}
	if (r == 0) {
		// AddFontResourceEx() が使えなかった
		// システム全体で使えるフォントとなる
		// removeしないとOSが掴んだままとなる
		r = AddFontResourceW(TSpecialFont);
	}
	if (r != 0) {
		AddFontFlag = TRUE;
	}
}

static void UnloadSpecialFont(void)
{
	if (!AddFontFlag) {
		return;
	}
	int r = 0;
	if (pRemoveFontResourceExW != NULL) {
		r = pRemoveFontResourceExW(TSpecialFont, FR_PRIVATE, NULL);
	}
	if (r == 0) {
		RemoveFontResourceW(TSpecialFont);
	}
}

static void init(void)
{
	DLLInit();
	WinCompatInit();
	DebugSetException();
	LoadSpecialFont();
#if defined(DEBUG_OPEN_CONSOLE_AT_STARTUP)
	DebugConsoleOpen();
#endif
}

/**
 * Tera Term main engine
 *
 *	@retval	FALSE	すぐに処理する必要なし
 *	@retval	TRUE	引き続き処理する必要あり
 */
BOOL OnIdle(LONG lCount)
{
	int nx, ny;
	BOOL Size;
	(void)lCount;

	if (cv.Ready)
	{
		/* Sender */
		CommSend(&cv);

		/* Parser */
		if ((TelStatus==TelIdle) && cv.TelMode)
			TelStatus = TelIAC;

		if (TelStatus != TelIdle)
		{
			ParseTel(&Size,&nx,&ny);
			if (Size) {
				LockBuffer();
				ChangeTerminalSize(nx,ny);
				UnlockBuffer();
			}
		}
		else {
			int Change;
			if (ProtoGetProtoFlag()) Change = ProtoDlgParse();
			else {
				switch (ActiveWin) {
				case IdVT:
					Change = pVTWin->Parse();
					// TEK windowのアクティブ中に pause を使うと、CPU使用率100%となる
					// 現象への暫定対処。(2006.2.6 yutaka)
					// 待ち時間をなくし、コンテキストスイッチだけにする。(2006.3.20 yutaka)
					Sleep(0);
					break;

				case IdTEK:
					if (pTEKWin != NULL) {
						Change = ((CTEKWindow*)pTEKWin)->Parse();
						// TEK windowのアクティブ中に pause を使うと、CPU使用率100%となる
						// 現象への暫定対処。(2006.2.6 yutaka)
						Sleep(1);
					}
					else {
						Change = IdVT;
					}
					break;

				default:
					Change = 0;
				}

				switch (Change) {
					case IdVT:
						VTActivate();
						break;
					case IdTEK:
						pVTWin->OpenTEK();
						break;
				}
			}
		}

		FLogWriteFile();

		if (DDELog && AdvFlag) {
			DDEAdv();
		}

		/* Talker */
		switch (TalkStatus) {
		case IdTalkFile:
			FileSend();
			break; /* file */
		case IdTalkSendMem:
			SendMemContinuously();
			break;
		default:
			break;
		}

		/* Receiver */
		if (DDELog && DDEGetCount() > 0) {
			// ログバッファがまだDDEクライアントへ送られていない場合は、
			// TCPパケットの受信を行わない。
			// 連続して受信を行うと、ログバッファがラウンドロビンにより未送信のデータを
			// 上書きしてしまう可能性がある。(2007.6.14 yutaka)

		} else {
			if (cv.PortType == IdSerial) {
				// シリアル接続では、別スレッド CommThread() でCOMポートから読み出しを行っているため、
				// CommReceive() の呼び出しは不要。
			} else {
				CommReceive(&cv);
			}
		}

	}

	BOOL Busy = FALSE;
	if (cv.Ready &&
	    (cv.RRQ || (cv.OutBuffCount>0) || (cv.InBuffCount>0) || (cv.FlushLen>0) || FLogGetCount() > 0 || (DDEGetCount()>0)) ) {
		Busy = TRUE;
	}

	return Busy;
}

static HWND main_window;
HWND GetHWND(void)
{
	return main_window;
}

class ModelessDlg
{
public:
	ModelessDlg()
	{
		count_ = 0;
		hWnds_ = NULL;
	}
	void Add(HWND hWnd)
	{
		int index = FindHandle(hWnd);
		if (index != -1) {
			return;
		}
		hWnds_ = (HWND *)realloc(hWnds_, sizeof(HWND) * (count_ + 1));
		hWnds_[count_] = hWnd;
		count_++;
	}

	void Remove(HWND hWnd)
	{
		int index = FindHandle(hWnd);
		if (index == -1) {
			assert(index == -1);
			return;
		}
		int move_count = count_ - index;
		memmove(&hWnds_[index], &hWnds_[index + 1], move_count);
		count_--;
		assert(count_ >= 0);
		hWnds_ = (HWND *)realloc(hWnds_, sizeof(HWND) * count_);
	}

	BOOL HandleMessage(MSG *msg)
	{
		for (int i = 0; i < count_; i++) {
			HWND hWnd = hWnds_[i];
			if (::IsDialogMessageW(hWnd, msg) != FALSE) {
				return TRUE;
			}
		}
		return FALSE;
	}

private:
	int FindHandle(HWND hWnd)
	{
		for (int i = 0; i < count_; i++) {
			if (hWnds_[i] == hWnd) {
				return i;
			}
		}
		return -1;
	}

	int count_;
	HWND *hWnds_;
};

static ModelessDlg modeless_dlg;

void AddModelessHandle(HWND hWnd)
{
	modeless_dlg.Add(hWnd);
}

void RemoveModelessHandle(HWND hWnd)
{
	modeless_dlg.Remove(hWnd);
}

#if 0 // not used
static UINT nMsgLast;
static POINT ptCursorLast;

/**
 *	idle状態に入るか判定する
 */
static BOOL IsIdleMessage(const MSG* pMsg)
{
	if (pMsg->message == WM_MOUSEMOVE ||
		pMsg->message == WM_NCMOUSEMOVE)
	{
		if (pMsg->message == nMsgLast &&
			pMsg->pt.x == ptCursorLast.x &&
			pMsg->pt.y == ptCursorLast.y)
		{	// 同じ位置だったらidleにはいらない
			return FALSE;
		}

		ptCursorLast = pMsg->pt;
		nMsgLast = pMsg->message;
		return TRUE;
	}

	if (pMsg->message == WM_PAINT ||
		pMsg->message == 0x0118/*WM_SYSTIMER*/)
	{
		return FALSE;
	}

	return TRUE;
}
#endif

VOID CALLBACK IdleTimerProc(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	SendMessage(main_window, WM_USER_IDLETIMER, 0, 0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst,
                   LPSTR lpszCmdLine, int nCmdShow)
{
	(void)hPreInst;
	(void)lpszCmdLine;
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	srand((unsigned int)time(NULL));
	setlocale(LC_ALL, "");

	ts.TeraTermInstance = hInstance;
	ts.nCmdShow = nCmdShow;
	hInst = hInstance;
	init();
	_HtmlHelpW(NULL, NULL, HH_INITIALIZE, (DWORD_PTR)&HtmlHelpCookie);

#if ENABLE_GDIPLUS
	GDIPInit();
#endif
	DXInit();

	CVTWindow *m_pMainWnd = new CVTWindow(hInstance);
	pVTWin = m_pMainWnd;
	main_window = m_pMainWnd->m_hWnd;

	// [Tera Term]セクションのDLG_SYSTEM_FONTをとりあえずセットする
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");

	CreateTimerQueueTimer(&hIdleTimer, NULL, IdleTimerProc, 0, IdleTimerPeriod, 0, WT_EXECUTEDEFAULT);

	LONG lCount = 0;
	MSG msg;
	for (;;) {
		// idle状態でメッセージがない場合
		DWORD idle_enter_tick = GetTickCount();
		BOOL sleep_enable = FALSE;
		for (;;) {
			// idle処理を行う
			//		- GetMessage()でブロックすると、
			//		  ウィンドウ非表示時にidle処理ができない
			//		- メッセージ処理を素早く行わなければ
			//		  SCPのダイアログの処理が遅くなり転送が滞る
			//		- 高速で処理を行うとCPU時間を消費してしまう
			//	20msは高速に処理して、その後Sleep()することでバランスを取る
			const BOOL continue_idle = OnIdle(lCount++);
			if (!continue_idle) {
				// FALSEが戻ってきたらidle処理は不要
				if (!sleep_enable) {
					if (GetTickCount() - idle_enter_tick > 20) {
						// 20ms以上idle処理不要だったらSleep()を入れる
						sleep_enable = TRUE;
					}
				}
				if (sleep_enable) {
					Sleep(2);
				}
				lCount = 0;
			}
			else {
				idle_enter_tick = GetTickCount();
				sleep_enable = FALSE;
			}

			if (::PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE) != FALSE) {
				// メッセージが存在する
				break;
			}
		}

		// メッセージが空になるまで処理する
		for(;;) {
			// メッセージが何もない場合、GetMessage()でブロックすることがある
			if (::GetMessageW(&msg, NULL, 0, 0) == FALSE) {
				// WM_QUIT
				goto exit_message_loop;
			}

			if (modeless_dlg.HandleMessage(&msg) == FALSE) {
				bool message_processed = false;

				if (m_pMainWnd->m_hAccel != NULL) {
					if (!MetaKey(ts.MetaKey)) {
						// matakeyが押されていない
						if (::TranslateAcceleratorW(m_pMainWnd->m_hWnd, m_pMainWnd->m_hAccel, &msg)) {
							// アクセラレーターキーを処理した
							message_processed = true;
						}
					}
				}

				if (!message_processed) {
					::TranslateMessage(&msg);
					::DispatchMessageW(&msg);
				}
			}

//			// idle状態に入るか?
//			if (IsIdleMessage(&msg)) {
//				lCount = 0;
//			}

			if (::PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE) == FALSE) {
				// メッセージがなくなった
				break;
			}
		}
	}
exit_message_loop:

	DeleteTimerQueueTimer(NULL, hIdleTimer, NULL);

	delete m_pMainWnd;
	m_pMainWnd = NULL;

	DXUninit();
#if ENABLE_GDIPLUS
	GDIPUninit();
#endif

	_HtmlHelpW(NULL, NULL, HH_CLOSE_ALL, 0);
	_HtmlHelpW(NULL, NULL, HH_UNINITIALIZE, HtmlHelpCookie);

	free(TSpecialFont);
	TSpecialFont = NULL;
	UnloadSpecialFont();
	DLLExit();
	UnicodeOverrideWidthUninit();

    return (int)msg.wParam;
}
