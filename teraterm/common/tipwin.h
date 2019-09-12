/*
 * Copyright (C) 2018-2019 TeraTerm Project
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

#include <windows.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	FRAME_WIDTH	6

typedef struct tagTipWinData TipWin;

TipWin *TipWinCreateW(HWND src, int cx, int cy, const wchar_t *str);
TipWin *TipWinCreateA(HWND src, int cx, int cy, const char *str);
void TipWinSetTextW(TipWin *tWin, const wchar_t *text);
void TipWinSetTextA(TipWin *tWin, const char *text);
void TipWinSetPos(TipWin *tWin, int x, int y);
void TipWinDestroy(TipWin *tWin);
void TipWinGetTextWidthHeight(HWND src, const TCHAR *str, int *width, int *height);
void TipWinSetPos(TipWin *tWin, int x, int y);

#if defined(UNICODE)
#define	TipWinCreate(p1, p2, p3, p4)	TipWinCreateW(p1, p2, p3, p4)
#define TipWinSetText(p1, p2)			TipWinSetTextW(p1, p2)
#else
#define	TipWinCreate(p1, p2, p3, p4)	TipWinCreateA(p1, p2, p3, p4)
#define TipWinSetText(p1, p2)			TipWinSetTextA(p1, p2)
#endif


#ifdef __cplusplus
}
#endif
