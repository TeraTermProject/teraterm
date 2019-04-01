/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2017 TeraTerm Project
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

/* TTMACRO.EXE, main */

#include <stdio.h>
#include <crtdbg.h>
#include <windows.h>
#include <commctrl.h>
#include "teraterm.h"
#include "ttm_res.h"
#include "ttmmain.h"
#include "ttl.h"

#include "ttmacro.h"
#include "ttmlib.h"
#include "ttlib.h"

#include "compat_w95.h"
#include "compat_win.h"
#include "ttmdlg.h"
#include "tmfc.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

char UILanguageFile[MAX_PATH];
static BOOL Busy;
static HINSTANCE hInst;
static CCtrlWindow *pCCtrlWindow;

static void init()
{
	typedef BOOL (WINAPI *pSetDllDir)(LPCSTR);
	typedef BOOL (WINAPI *pSetDefDllDir)(DWORD);

	HMODULE module;
	pSetDllDir setDllDir;
	pSetDefDllDir setDefDllDir;

	if ((module = GetModuleHandle("kernel32.dll")) != NULL) {
		if ((setDefDllDir = (pSetDefDllDir)GetProcAddress(module, "SetDefaultDllDirectories")) != NULL) {
			// SetDefaultDllDirectories() が使える場合は、検索パスを %WINDOWS%\system32 のみに設定する
			(*setDefDllDir)((DWORD)0x00000800); // LOAD_LIBRARY_SEARCH_SYSTEM32
		}
		else if ((setDllDir = (pSetDllDir)GetProcAddress(module, "SetDllDirectoryA")) != NULL) {
			// SetDefaultDllDirectories() が使えなくても、SetDllDirectory() が使える場合は
			// カレントディレクトリだけでも検索パスからはずしておく。
			(*setDllDir)("");
		}
	}

	WinCompatInit();
	if (pSetThreadDpiAwarenessContext) {
		pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	}
}

// TTMACRO main engine
static BOOL OnIdle(LONG lCount)
{
	BOOL Continue;

	// Avoid multi entry
	if (Busy) {
		return FALSE;
	}
	Busy = TRUE;
	if (pCCtrlWindow != NULL) {
		Continue = pCCtrlWindow->OnIdle();
	}
	else {
		Continue = FALSE;
	}
	Busy = FALSE;
	return Continue;
}

/////////////////////////////////////////////////////////////////////////////

// CCtrlApp theApp;

static HWND CtrlWnd;

HINSTANCE GetInstance()
{
	return hInst;
}

HWND GetHWND()
{
	return CtrlWnd;
}

/////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst,
                   LPSTR lpszCmdLine, int nCmdShow)
{
	hInst = hInstance;
	static HMODULE HTTSET = NULL;
	LONG lCount = 0;
	DWORD SleepTick = 1;

#ifdef _DEBUG
	::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	init();
//	InitCommonControls();
	GetUILanguageFile(UILanguageFile, sizeof(UILanguageFile));

	Busy = TRUE;
	pCCtrlWindow = new CCtrlWindow();
	pCCtrlWindow->Create();
//	pCCtrlWindow->ShowWindow(SW_SHOW);
//	tmpWnd->ShowWindow(SW_SHOW);
	Busy = FALSE;  

	HWND hWnd = pCCtrlWindow->GetSafeHwnd();
	CtrlWnd = hWnd;

	//////////////////////////////////////////////////////////////////////
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
#if 0
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
#endif

		if (IsDialogMessage(hWnd, &msg) != 0) {
			/* 処理された*/
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#if 0
		if (!message_processed) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif

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

	//////////////////////////////////////////////////////////////////////

	// TODO すでに閉じられている、この処理不要?
	if (pCCtrlWindow) {
		pCCtrlWindow->DestroyWindow();
	}
	pCCtrlWindow = NULL;
	return ExitCode;
}
