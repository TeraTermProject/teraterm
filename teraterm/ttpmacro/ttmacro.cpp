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

/* TTMACRO.EXE, main */

#include <stdio.h>
#include <crtdbg.h>
#include <windows.h>
#include <commctrl.h>

#include "teraterm.h"
#include "compat_win.h"
#include "ttmdlg.h"
#include "tmfc.h"
#include "dlglib.h"
#include "dllutil.h"
#include "codeconv.h"
#include "win32helper.h"
#include "ttdebug.h"

#include "ttm_res.h"
#include "ttmmain.h"
#include "ttl.h"
#include "ttmacro.h"
#include "ttmlib.h"
#include "ttlib.h"
#include "ttmdlg.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

wchar_t *UILanguageFileW;
static wchar_t *SetupFNameW;
static HWND CtrlWnd;
static HINSTANCE hInst;

static BOOL Busy;
static CCtrlWindow *pCCtrlWindow;

DPI_AWARENESS_CONTEXT DPIAware;

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
	HomeDirW = GetHomeDirW(hInst);
	SetupFNameW = GetDefaultFNameW(HomeDirW, L"TERATERM.INI");

	UILanguageFileW = GetUILanguageFileFullW(SetupFNameW);

	DLLInit();
	WinCompatInit();

	// DPI Aware (��DPI�Ή�)
	DPIAware = DPI_AWARENESS_CONTEXT_UNAWARE;
	if (pIsValidDpiAwarenessContext != NULL && pSetThreadDpiAwarenessContext != NULL) {
		wchar_t Temp[4];
		GetPrivateProfileStringW(L"Tera Term", L"DPIAware", NULL, Temp, _countof(Temp), SetupFNameW);
		if (_wcsicmp(Temp, L"on") == 0) {
			if (pIsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == TRUE) {
				pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
				DPIAware = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
			}
		}
	}

	// UILanguageFile�� "Tera Term" �Z�N�V���� "DLG_SYSTEM_FONT" �̃t�H���g�ɐݒ肷��
	LOGFONTW logfont;
	GetI18nLogfontW(L"Tera Term", L"DlgFont", &logfont, 0, SetupFNameW);
	SetDialogFont(logfont.lfFaceName, logfont.lfHeight, logfont.lfCharSet,
				  UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
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
	DebugSetException();

//	InitCommonControls();
	init();

	Busy = TRUE;
	pCCtrlWindow = new CCtrlWindow(hInst);
	pCCtrlWindow->Create();

	if (!VOption) {
		if (IOption) {
			nCmdShow = SW_SHOWMINIMIZED;
		}
		ShowWindow(pCCtrlWindow->m_hWnd, nCmdShow);
	}

	Busy = FALSE;

	HWND hWnd = pCCtrlWindow->GetSafeHwnd();
	CtrlWnd = hWnd;

	// message pump
	MSG msg;
	for (;;) {
		// Windows�̃��b�Z�[�W���Ȃ��ꍇ�̃��[�v
		for(;;) {
			if (PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE) != FALSE) {
				// ���b�Z�[�W�����݂����A���[�v�𔲂���
				break;
			}

			if (!OnIdle(lCount)) {
				// idle�s�v
				if (SleepTick < 500) {	// �ő� 501ms����
					SleepTick += 2;
				}
				lCount = 0;
				Sleep(SleepTick);
			} else {
				// �vidle
				SleepTick = 0;
				lCount++;
			}
		}

		// Windows�̃��b�Z�[�W����ɂȂ�܂ŏ�������
		for(;;) {
			if (GetMessageW(&msg, NULL, 0, 0) == FALSE) {
				// WM_QUIT
				goto exit_message_loop;
			}
			if (IsDialogMessageW(hWnd, &msg) != 0) {
				/* �������ꂽ*/
			} else {
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}

			if (PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE) == FALSE) {
				// ���b�Z�[�W���Ȃ��Ȃ����A���[�v�𔲂���
				break;
			}
		}
	}
exit_message_loop:

	pCCtrlWindow->DestroyWindow();
	delete pCCtrlWindow;
	pCCtrlWindow = NULL;

	free(UILanguageFileW);
	free(SetupFNameW);
	free(HomeDirW);
	DLLExit();
	return ExitCode;
}
