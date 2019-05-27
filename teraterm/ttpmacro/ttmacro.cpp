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

/* TTMACRO.EXE, main */

#include <stdio.h>
#include <crtdbg.h>
#include <windows.h>
#include <commctrl.h>

#include "teraterm.h"
#include "compat_w95.h"
#include "compat_win.h"
#include "ttmdlg.h"
#include "tmfc.h"
#include "dlglib.h"
#include "dllutil.h"

#include "ttm_res.h"
#include "ttmmain.h"
#include "ttl.h"
#include "ttmacro.h"
#include "ttmlib.h"
#include "ttlib.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

char UILanguageFile[MAX_PATH];
static char SetupFName[MAX_PATH];
static HWND CtrlWnd;
static HINSTANCE hInst;

static BOOL Busy;
static CCtrlWindow *pCCtrlWindow;

HINSTANCE GetInstance()
{
	return hInst;
}

HWND GetHWND()
{
	return CtrlWnd;
}

static void init()
{
	char UILanguageFileRel[MAX_PATH];
	LOGFONTA logfont;

	GetHomeDir(hInst, HomeDir, sizeof(HomeDir));
	GetDefaultFName(HomeDir, "TERATERM.INI", SetupFName, sizeof(SetupFName));
	GetPrivateProfileString("Tera Term", "UILanguageFile", "lang\\Default.lng",
	                        UILanguageFileRel, sizeof(UILanguageFileRel), SetupFName);
	GetUILanguageFileFull(HomeDir, UILanguageFileRel,
						  UILanguageFile, sizeof(UILanguageFile));

	DLLInit();
	WinCompatInit();

	// DPI Aware (高DPI対応)
	if (pIsValidDpiAwarenessContext != NULL && pSetThreadDpiAwarenessContext != NULL) {
		char Temp[4];
		GetPrivateProfileString("Tera Term", "DPIAware", NULL, Temp, sizeof(Temp), SetupFName);
		if (_stricmp(Temp, "on") == 0) {
			if (pIsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == TRUE) {
				pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			}
		}
	}

	// UILanguageFileの "Tera Term" セクション "DLG_SYSTEM_FONT" のフォントに設定する
	GetI18nLogfont("Tera Term", "DlgFont", &logfont, 0, SetupFName);
	SetDialogFont(logfont.lfFaceName, logfont.lfHeight, logfont.lfCharSet,
				  UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst,
                   LPSTR lpszCmdLine, int nCmdShow)
{
	hInst = hInstance;
	LONG lCount = 0;
	DWORD SleepTick = 1;

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

//	InitCommonControls();
	init();

	Busy = TRUE;
	pCCtrlWindow = new CCtrlWindow();
	pCCtrlWindow->Create();
	Busy = FALSE;

	HWND hWnd = pCCtrlWindow->GetSafeHwnd();
	CtrlWnd = hWnd;

	// message pump
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {

		if (IsDialogMessage(hWnd, &msg) != 0) {
			/* 処理された*/
		} else {
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

	pCCtrlWindow->DestroyWindow();
	delete pCCtrlWindow;
	pCCtrlWindow = NULL;

	DLLExit();
	return ExitCode;
}
