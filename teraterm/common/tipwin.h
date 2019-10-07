/*
 * Copyright (C) 2008-2019 TeraTerm Project
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

#ifndef _H_TIPWIN
#define _H_TIPWIN
#include <windows.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	TIP_WIN_FRAME_WIDTH	6

#define TipWinClassName _T("TeraTermTipWinClass")

typedef struct tagTipWinData TipWin;

TipWin *TipWinCreate(HINSTANCE hInstance, HWND src, int cx, int cy, const TCHAR *str);
void TipWinSetText(TipWin *tWin, TCHAR *text);
void TipWinDestroy(TipWin *tWin);
void TipWinGetTextWidthHeight(HWND src, const TCHAR *str, int *width, int *height);
void TipWinGetPos(TipWin *tWin, int *x, int *y);
void TipWinSetPos(TipWin *tWin, int x, int y);
void TipWinSetHideTimer(TipWin *tWin, int ms);
void TipWinSetVisible(TipWin *tWin, int visible);
int TipWinIsExists(TipWin *tWin);
int TipWinIsVisible(TipWin *tWin);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class CTipWin
{
public:
	CTipWin(HINSTANCE hInstance);
	~CTipWin();
	VOID Create(HWND pHwnd);
	VOID Destroy();
	VOID SetText(TCHAR *str);
	POINT GetPos();
	VOID SetPos(int x, int y);
	VOID SetHideTimer(int ms);
	BOOL IsExists();
	VOID SetVisible(BOOL bVisible);
	BOOL IsVisible();
private:
	UINT_PTR timerid;
	TipWin* tWin;
	HINSTANCE hInstance;
	TCHAR class_name[32];
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
	VOID CalcStrRect();
	ATOM RegisterClass();
	BOOL UnregisterClass();
	BOOL IsClassRegistered();
};
#endif
#endif
