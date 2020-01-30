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

#include <stdio.h>
#include <crtdbg.h>
#include <tchar.h>
#include <io.h>			// for access()
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
#include "unicode_test.h"
#if UNICODE_INTERNAL_BUFF
#include "sendmem.h"
#endif
#include "layer_for_unicode.h"

#if defined(_DEBUG) && defined(_MSC_VER)
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

static BOOL AddFontFlag;
static wchar_t TSpecialFont[MAX_PATH];
static CVTWindow* pVTWin;

static void LoadSpecialFont()
{
	if (IsExistFontA("Tera Special", SYMBOL_CHARSET, TRUE)) {
		// すでに存在するのでロードしない
		return;
	}

	if (GetModuleFileNameW(NULL, TSpecialFont, _countof(TSpecialFont)) == 0) {
		AddFontFlag = FALSE;
		return;
	}
	*wcsrchr(TSpecialFont, L'\\') = 0;
	wcscat_s(TSpecialFont, L"\\TSPECIAL1.TTF");

	// teraterm.exeのみで有効なフォントとなる。
	// removeしなくても終了するとOSからなくなる
	int r = _AddFontResourceExW(TSpecialFont, FR_PRIVATE, NULL);
	if (r == 0) {
		// AddFontResourceEx() が使えなかった
		// システム全体で使えるフォントとなる
		// removeしないとOSが掴んだままとなる
		r = _AddFontResourceW(TSpecialFont);
	}
	if (r != 0) {
		AddFontFlag = TRUE;
	}
}

static void UnloadSpecialFont()
{
	if (!AddFontFlag) {
		return;
	}
	int r = _RemoveFontResourceExW(TSpecialFont, FR_PRIVATE, NULL);
	if (r == 0) {
		_RemoveFontResourceW(TSpecialFont);
	}
}

static void init()
{
	DLLInit();
	WinCompatInit();
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
		case IdTalkSendMem:
			SendMemContinuously();
			break;
		default:
			break;
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

static HWND main_window;
HWND GetHWND()
{
	return main_window;
}

static HWND hModelessDlg;

void AddModelessHandle(HWND hWnd)
{
	hModelessDlg = hWnd;
}

void RemoveModelessHandle(HWND hWnd)
{
	(void)hWnd;
	hModelessDlg = 0;
}

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst,
                   LPSTR lpszCmdLine, int nCmdShow)
{
	(void)hPreInst;
	(void)lpszCmdLine;
	(void)nCmdShow;
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	init();
	hInst = hInstance;
	CVTWindow *m_pMainWnd = new CVTWindow(hInstance);
	pVTWin = m_pMainWnd;
	main_window = m_pMainWnd->m_hWnd;
	// [Tera Term]セクションのDLG_SYSTEM_FONTをとりあえずセットする
	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");

	BOOL bIdle = TRUE;	// idle状態か?
	LONG lCount = 0;
	MSG msg;
	for (;;) {
		// idle状態でメッセージがない場合
		while (bIdle) {
			if (::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE) != FALSE) {
				// メッセージが存在する
				break;
			}

			const BOOL continue_idle = OnIdle(lCount++);
			if (!continue_idle) {
				// FALSEが戻ってきたらidle処理は不要
				bIdle = FALSE;
				break;
			}
		}

		// メッセージが空になるまで処理する
		for(;;) {
			// メッセージが何もない場合、GetMessage()でブロックすることがある
			if (::GetMessage(&msg, NULL, 0, 0) == FALSE) {
				// WM_QUIT
				goto exit_message_loop;
			}

			if (hModelessDlg == 0 ||
				::IsDialogMessage(hModelessDlg, &msg) == FALSE)
			{
				bool message_processed = false;

				if (m_pMainWnd->m_hAccel != NULL) {
					if (!MetaKey(ts.MetaKey)) {
						// matakeyが押されていない
						if (::TranslateAccelerator(m_pMainWnd->m_hWnd , m_pMainWnd->m_hAccel, &msg)) {
							// アクセラレーターキーを処理した
							message_processed = true;
						}
					}
				}

				if (!message_processed) {
					::TranslateMessage(&msg);
					::DispatchMessage(&msg);
				}
			}

			// idle状態に入るか?
			if (IsIdleMessage(&msg)) {
				bIdle = TRUE;
				lCount = 0;
			}

			if (::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE) == FALSE) {
				// メッセージがなくなった
				break;
			}
		}
	}
exit_message_loop:

	delete m_pMainWnd;
	m_pMainWnd = NULL;

	UnloadSpecialFont();
	DLLExit();

    return (int)msg.wParam;
}
