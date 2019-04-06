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

/* TERATERM.EXE, main */

#include "teraterm_conf.h"

#include <crtdbg.h>
#include <tchar.h>
#include "teraterm.h"
#include "tttypes.h"
#include "commlib.h"
#include "ttwinman.h"
#include "buffer.h"
#include "vtterm.h"
#include "vtwin.h"
#include "clipboar.h"
#include "ttftypes.h"
#include "filesys.h"
#include "telnet.h"
#include "tektypes.h"
#include "tekwin.h"
#include "ttdde.h"
#include "keyboard.h"
#include "dllutil.h"
#include "compat_win.h"
#include "compat_w95.h"
#include "dlglib.h"
#include "teraterml.h"

#if 0
//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

static BOOL AddFontFlag;
static TCHAR TSpecialFont[MAX_PATH];

static void LoadSpecialFont()
{
	if (!IsExistFontA("Tera Special", SYMBOL_CHARSET, TRUE)) {
		int r;

		if (GetModuleFileName(NULL, TSpecialFont,_countof(TSpecialFont)) == 0) {
			AddFontFlag = FALSE;
			return;
		}
		*_tcsrchr(TSpecialFont, _T('\\')) = 0;
		_tcscat_s(TSpecialFont, _T("\\TSPECIAL1.TTF"));

		if (pAddFontResourceEx != NULL) {
			// teraterm.exeのみで有効なフォントとなる。
			// removeしなくても終了するとOSからなくなる
			r = pAddFontResourceEx(TSpecialFont, FR_PRIVATE, NULL);
		} else {
			// システム全体で使えるフォントとなる
			// removeしないとOSが掴んだままとなる
			r = AddFontResource(TSpecialFont);
		}
		if (r != 0) {
			AddFontFlag = TRUE;
		}
	}
}

static void UnloadSpecialFont()
{
	if (AddFontFlag) {
		if (pRemoveFontResourceEx != NULL) {
			pRemoveFontResourceEx(TSpecialFont, FR_PRIVATE, NULL);
		} else {
			RemoveFontResource(TSpecialFont);
		}
	}
}

static void init()
{
#ifdef _DEBUG
	::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	DLLInit();
	WinCompatInit();
#if defined(DPIAWARENESS)
	//SetProcessDPIAware();
	if (pSetThreadDpiAwarenessContext != NULL) {
		// Windows 10 Version 1703以降の場合?
		pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	}
#endif
	LoadSpecialFont();
}

// Tera Term main engine
static BOOL OnIdle(LONG lCount)
{
	static int Busy = 2;
	int Change, nx, ny;
	BOOL Size;

	if (lCount==0) Busy = 2;

	if (cv.Ready)
	{
		/* Sender */
		CommSend(&cv);

		/* Parser */
		if ((cv.HLogBuf!=NULL) && (cv.LogBuf==NULL))
			cv.LogBuf = (PCHAR)GlobalLock(cv.HLogBuf);

		if ((cv.HBinBuf!=NULL) && (cv.BinBuf==NULL))
			cv.BinBuf = (PCHAR)GlobalLock(cv.HBinBuf);

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
			if (cv.ProtoFlag) Change = ProtoDlgParse();
			else {
				switch (ActiveWin) {
				case IdVT:
					Change =  ((CVTWindow*)pVTWin)->Parse();
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
						((CVTWindow*)pVTWin)->OpenTEK();
						break;
				}
			}
		}

		if (cv.LogBuf!=NULL)
		{
			if (FileLog) {
				LogToFile();
			}
			if (DDELog && AdvFlag) {
				DDEAdv();
			}
			GlobalUnlock(cv.HLogBuf);
			cv.LogBuf = NULL;
		}

		if (cv.BinBuf!=NULL)
		{
			if (BinLog) {
				LogToFile();
			}
			GlobalUnlock(cv.HBinBuf);
			cv.BinBuf = NULL;
		}

		/* Talker */
		switch (TalkStatus) {
		case IdTalkCB:
			CBSend();
			break; /* clip board */
		case IdTalkFile:
			FileSend();
			break; /* file */
		}

		/* Receiver */
		if (DDELog && cv.DCount >0) {
			// ログバッファがまだDDEクライアントへ送られていない場合は、
			// TCPパケットの受信を行わない。
			// 連続して受信を行うと、ログバッファがラウンドロビンにより未送信のデータを
			// 上書きしてしまう可能性がある。(2007.6.14 yutaka)

		} else {
			CommReceive(&cv);
		}

	}

	if (cv.Ready &&
	    (cv.RRQ || (cv.OutBuffCount>0) || (cv.InBuffCount>0) || (cv.FlushLen>0) || (cv.LCount>0) || (cv.BCount>0) || (cv.DCount>0)) ) {
		Busy = 2;
	}
	else {
		Busy--;
	}

	return (Busy>0);
}

BOOL CallOnIdle(LONG lCount)
{
	return OnIdle(lCount);
}

HINSTANCE GetInstance()
{
	return hInst;
}

static HWND main_window;
HWND GetHWND()
{
	return main_window;
}

static HWND hModalWnd;

void AddModalHandle(HWND hWnd)
{
	hModalWnd = hWnd;
}

void RemoveModalHandle(HWND hWnd)
{
	hModalWnd = 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst,
                   LPSTR lpszCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	LONG lCount = 0;
	DWORD SleepTick = 1;
	init();
	hInst = hInstance;
	CVTWindow *m_pMainWnd = new CVTWindow();
	pVTWin = m_pMainWnd;
	main_window = m_pMainWnd->m_hWnd;
	// [Tera Term]セクションのDlgFont=がない場合は
	// [TTSSH]セクションのフォント設定を使用する
	SetDialogFont(ts.SetupFName, ts.UILanguageFile, "TTSSH");

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (hModalWnd != 0) {
			if (IsDialogMessage(hModalWnd, &msg)) {
				continue;
			}
		}

		bool message_processed = false;

		if (m_pMainWnd->m_hAccel != NULL) {
			if (!MetaKey(ts.MetaKey)) {
				// matakeyが押されていない
				if (TranslateAccelerator(m_pMainWnd->m_hWnd , m_pMainWnd->m_hAccel, &msg)) {
					// アクセラレーターキーを処理した
					message_processed = true;
				}
			}
		}

		if (!message_processed) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		while (!PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE)) {
			// メッセージがない
			if (!OnIdle(lCount)) {
				// idle不要
				if (SleepTick < 500) {	// 最大 501ms未満
					SleepTick += 2;
				}
				lCount = 0;
				Sleep(SleepTick);
			} else {
				// 要idle
				SleepTick = 0;
				lCount++;
			}
		}
	}
	delete m_pMainWnd;
	m_pMainWnd = NULL;

	UnloadSpecialFont();
	DLLExit();

    return msg.wParam;
}
