/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2020 TeraTerm Project
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

/* keycode.exe for Tera Term Pro */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>

#include "kc_res.h"
#define ClassName _T("KeyCodeWin32")

#include "compat_w95.h"

// Prototypes
LRESULT WINAPI MainWndProc( HWND, UINT, WPARAM, LPARAM );

// Global variables;
static HANDLE ghInstance;
static BOOL KeyDown = FALSE;
static BOOL Short;
static WORD Scan;

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine,
                   int nCmdShow)
{
	typedef BOOL (WINAPI *pSetDllDir)(LPCSTR);
	typedef BOOL (WINAPI *pSetDefDllDir)(DWORD);

	WNDCLASS wc;
	MSG msg;
	HWND hWnd;
	HMODULE module;
	pSetDllDir setDllDir;
	pSetDefDllDir setDefDllDir;

	(void)lpszCmdLine;
	DoCover_IsDebuggerPresent();

	if ((module = GetModuleHandleA("kernel32.dll")) != NULL) {
		FARPROC func_ptr = GetProcAddress(module, "SetDefaultDllDirectories");
		setDefDllDir = (pSetDefDllDir)func_ptr;
		if (setDefDllDir != NULL) {
			// SetDefaultDllDirectories() が使える場合は、検索パスを %WINDOWS%\system32 のみに設定する
			(*setDefDllDir)((DWORD)0x00000800); // LOAD_LIBRARY_SEARCH_SYSTEM32
		}
		else {
			func_ptr = GetProcAddress(module, "SetDllDirectoryA");
			setDllDir = (pSetDllDir)func_ptr;
			if (setDllDir != NULL) {
				// SetDefaultDllDirectories() が使えなくても、SetDllDirectory() が使える場合は
				// カレントディレクトリだけでも検索パスからはずしておく。
				(*setDllDir)("");
			}
		}
	}

	if(!hPrevInstance) {
		wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
		wc.lpfnWndProc = MainWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = ClassName;
		RegisterClass(&wc);
	}

	ghInstance = hInstance;

	hWnd = CreateWindow(ClassName,
	                    _T("Key code for Tera Term"),
	                    WS_OVERLAPPEDWINDOW,
	                    CW_USEDEFAULT,
	                    CW_USEDEFAULT,
	                    200,
	                    100,
	                    NULL,
	                    NULL,
	                    hInstance,
	                    NULL);

	ShowWindow(hWnd, nCmdShow);

	PostMessage(hWnd,WM_SETICON,ICON_SMALL,
	            (LPARAM)LoadImage(hInstance,
	                              MAKEINTRESOURCE(IDI_KEYCODE),
	                              IMAGE_ICON,16,16,0));
	PostMessage(hWnd,WM_SETICON,ICON_BIG,
	            (LPARAM)LoadImage(hInstance,
	                              MAKEINTRESOURCE(IDI_KEYCODE),
	                              IMAGE_ICON,0,0,0));

	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

void KeyDownProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if ((wParam==VK_SHIFT) ||
	    (wParam==VK_CONTROL) ||
	    (wParam==VK_MENU)) {
		return;
	}

	Scan = HIWORD(lParam) & 0x1ff;
	if ((GetKeyState(VK_SHIFT) & 0x80) != 0) {
		Scan = Scan | 0x200;
	}
	if ((GetKeyState(VK_CONTROL) & 0x80) != 0) {
		Scan = Scan | 0x400;
	}
	if ((GetKeyState(VK_MENU) & 0x80) != 0) {
		Scan = Scan | 0x800;
	}

	if (! KeyDown) {
		KeyDown = TRUE;
		Short = TRUE;
		SetTimer(hWnd,1,10,NULL);
		InvalidateRect(hWnd,NULL,TRUE);
	}
}

void KeyUpProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	(void)wParam;
	(void)lParam;
	if (! KeyDown) {
		return;
	}
	if (Short) {
		SetTimer(hWnd,2,500,NULL);
	}
	else {
		KeyDown = FALSE;
		InvalidateRect(hWnd,NULL,TRUE);
	}
}

void PaintProc(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC;
	char OutStr[30];

	hDC = BeginPaint(hWnd, &ps);

	if (KeyDown) {
		_snprintf_s(OutStr,sizeof(OutStr),_TRUNCATE,"Key code is %u.",Scan);
		TextOutA(hDC,10,10,OutStr, (int)strlen(OutStr));
	}
	else {
		TextOutA(hDC,10,10,"Push any key.",13);
	}

	EndPaint(hWnd, &ps);
}

void TimerProc(HWND hWnd, WPARAM wParam)
{
	KillTimer(hWnd,wParam);
	if (wParam==1) {
		Short = FALSE;
	}
	else if (wParam==2) {
		KeyDown = FALSE;
		InvalidateRect(hWnd,NULL,TRUE);
	}
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam,
  LPARAM lParam)
{
	switch( msg ) {
		case WM_KEYDOWN:
			KeyDownProc(hWnd, wParam, lParam);
			break;
		case WM_KEYUP:
			KeyUpProc(hWnd, wParam, lParam);
			break;
		case WM_SYSKEYDOWN:
			if (wParam==VK_F10) {
				KeyDownProc(hWnd, wParam, lParam);
			}
			else {
				return (DefWindowProc(hWnd, msg, wParam, lParam));
			}
			break;
		case WM_SYSKEYUP:
			if (wParam==VK_F10) {
				KeyUpProc(hWnd, wParam, lParam);
			}
			else {
				return (DefWindowProc(hWnd, msg, wParam, lParam));
			}
			break;
		case WM_PAINT:
			PaintProc(hWnd);
			break;
		case WM_TIMER:
			TimerProc(hWnd, wParam);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return (DefWindowProc(hWnd, msg, wParam, lParam));
	}

	return 0;
}
