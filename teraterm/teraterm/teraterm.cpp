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

#include "teraterm.h"
#include "tttypes.h"
#include "commlib.h"
#include "ttwinman.h"
#include "buffer.h"
#include "vtterm.h"
#include "vtwin.h"
#include "clipboar.h"
#include "filesys.h"
#include "telnet.h"
#include "tektypes.h"
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

#if defined(_DEBUG) && defined(_MSC_VER)
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

static BOOL AddFontFlag;
static wchar_t *TSpecialFont;
CVTWindow* pVTWin;
static DWORD HtmlHelpCookie;

static void LoadSpecialFont(void)
{
	wchar_t *mod_path;
	if (IsExistFontW(L"Tera Special", SYMBOL_CHARSET, TRUE)) {
		// ���łɑ��݂���̂Ń��[�h���Ȃ�
		return;
	}

	if (hGetModuleFileNameW(NULL, &mod_path) != 0) {
		AddFontFlag = FALSE;
		return;
	}
	*wcsrchr(mod_path, L'\\') = 0;
	aswprintf(&TSpecialFont, L"%s\\TSPECIAL1.TTF", mod_path);
	free(mod_path);

	// teraterm.exe�݂̂ŗL���ȃt�H���g�ƂȂ�B
	// remove���Ȃ��Ă��I�������OS����Ȃ��Ȃ�
	int r = 0;
	if (pAddFontResourceExW != NULL) {
		r = pAddFontResourceExW(TSpecialFont, FR_PRIVATE, NULL);
	}
	if (r == 0) {
		// AddFontResourceEx() ���g���Ȃ�����
		// �V�X�e���S�̂Ŏg����t�H���g�ƂȂ�
		// remove���Ȃ���OS���͂񂾂܂܂ƂȂ�
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

// Tera Term main engine
static BOOL OnIdle(LONG lCount)
{
	static int Busy = 2;
	int nx, ny;
	BOOL Size;

	if (lCount==0) Busy = 2;

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
					// TEK window�̃A�N�e�B�u���� pause ���g���ƁACPU�g�p��100%�ƂȂ�
					// ���ۂւ̎b��Ώ��B(2006.2.6 yutaka)
					// �҂����Ԃ��Ȃ����A�R���e�L�X�g�X�C�b�`�����ɂ���B(2006.3.20 yutaka)
					Sleep(0);
					break;

				case IdTEK:
					if (pTEKWin != NULL) {
						Change = ((CTEKWindow*)pTEKWin)->Parse();
						// TEK window�̃A�N�e�B�u���� pause ���g���ƁACPU�g�p��100%�ƂȂ�
						// ���ۂւ̎b��Ώ��B(2006.2.6 yutaka)
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
			// ���O�o�b�t�@���܂�DDE�N���C�A���g�֑����Ă��Ȃ��ꍇ�́A
			// TCP�p�P�b�g�̎�M���s��Ȃ��B
			// �A�����Ď�M���s���ƁA���O�o�b�t�@�����E���h���r���ɂ�薢���M�̃f�[�^��
			// �㏑�����Ă��܂��\��������B(2007.6.14 yutaka)

		} else {
			CommReceive(&cv);
		}

	}

	if (cv.Ready &&
	    (cv.RRQ || (cv.OutBuffCount>0) || (cv.InBuffCount>0) || (cv.FlushLen>0) || FLogGetCount() > 0 || (DDEGetCount()>0)) ) {
		Busy = 2;
	}
	else {
		Busy--;
	}

	return (Busy>0);
}

static HWND main_window;
HWND GetHWND(void)
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
 *	idle��Ԃɓ��邩���肷��
 */
static BOOL IsIdleMessage(const MSG* pMsg)
{
	if (pMsg->message == WM_MOUSEMOVE ||
		pMsg->message == WM_NCMOUSEMOVE)
	{
		if (pMsg->message == nMsgLast &&
			pMsg->pt.x == ptCursorLast.x &&
			pMsg->pt.y == ptCursorLast.y)
		{	// �����ʒu��������idle�ɂ͂���Ȃ�
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

	CVTWindow *m_pMainWnd = new CVTWindow(hInstance);
	pVTWin = m_pMainWnd;
	main_window = m_pMainWnd->m_hWnd;

	if (ts.Minimize>0) {
		nCmdShow = SW_SHOWMINIMIZED;
	}
	ShowWindow(main_window, nCmdShow);

	// [Tera Term]�Z�N�V������DLG_SYSTEM_FONT���Ƃ肠�����Z�b�g����
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");

	BOOL bIdle = TRUE;	// idle��Ԃ�?
	LONG lCount = 0;
	MSG msg;
	for (;;) {
		// idle��ԂŃ��b�Z�[�W���Ȃ��ꍇ
		while (bIdle) {
			if (::PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE) != FALSE) {
				// ���b�Z�[�W�����݂���
				break;
			}

			const BOOL continue_idle = OnIdle(lCount++);
			if (!continue_idle) {
				// FALSE���߂��Ă�����idle�����͕s�v
				bIdle = FALSE;
				break;
			}
		}

		// ���b�Z�[�W����ɂȂ�܂ŏ�������
		for(;;) {
			// ���b�Z�[�W�������Ȃ��ꍇ�AGetMessage()�Ńu���b�N���邱�Ƃ�����
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
						// matakey��������Ă��Ȃ�
						if (::TranslateAccelerator(m_pMainWnd->m_hWnd , m_pMainWnd->m_hAccel, &msg)) {
							// �A�N�Z�����[�^�[�L�[����������
							message_processed = true;
						}
					}
				}

				if (!message_processed) {
					::TranslateMessage(&msg);
					::DispatchMessage(&msg);
				}
			}

			// idle��Ԃɓ��邩?
			if (IsIdleMessage(&msg)) {
				bIdle = TRUE;
				lCount = 0;
			}

			if (::PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE) == FALSE) {
				// ���b�Z�[�W���Ȃ��Ȃ���
				break;
			}
		}
	}
exit_message_loop:

	delete m_pMainWnd;
	m_pMainWnd = NULL;

#if ENABLE_GDIPLUS
	GDIPUninit();
#endif

	_HtmlHelpW(NULL, NULL, HH_CLOSE_ALL, 0);
	_HtmlHelpW(NULL, NULL, HH_UNINITIALIZE, HtmlHelpCookie);

	free(TSpecialFont);
	TSpecialFont = NULL;
	UnloadSpecialFont();
	DLLExit();

    return (int)msg.wParam;
}
