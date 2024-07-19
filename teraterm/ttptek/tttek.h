/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2018- TeraTerm Project
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

/* TTTEK.DLL, TEK escape sequences */

#ifdef __cplusplus
extern "C" {
#endif

void WINAPI TEKInit(PTEKVar tk, PTTSet ts);
void WINAPI TEKChangeCaret(PTEKVar tk, PTTSet ts);
void WINAPI TEKDestroyCaret(PTEKVar tk, PTTSet ts);
void WINAPI TEKResizeWindow(PTEKVar tk, PTTSet ts, int W, int H);
int WINAPI TEKParse(PTEKVar tk, PTTSet ts, PComVar cv);
void WINAPI TEKReportGIN(PTEKVar tk, PTTSet ts, PComVar cv, BYTE KeyCode);
void WINAPI TEKPaint(PTEKVar tk, PTTSet ts, HDC PaintDC, PAINTSTRUCT *PaintInfo);
void WINAPI TEKWMLButtonDown(PTEKVar tk, PTTSet ts, PComVar cv, POINT pos);
void WINAPI TEKWMLButtonUp(PTEKVar tk, PTTSet ts);
void WINAPI TEKWMMouseMove(PTEKVar tk, PTTSet ts, POINT p);
void WINAPI TEKWMSize(PTEKVar tk, PTTSet ts, int W, int H, int cx, int cy);
void WINAPI TEKCMCopy(PTEKVar tk, PTTSet ts);
void WINAPI TEKCMCopyScreen(PTEKVar tk, PTTSet ts);
void WINAPI TEKPrint(PTEKVar tk, PTTSet ts, HDC PrintDC, BOOL SelFlag);
void WINAPI TEKClearScreen(PTEKVar tk, PTTSet ts);
void WINAPI TEKSetupFont(PTEKVar tk, PTTSet ts);
void WINAPI TEKResetWin(PTEKVar tk, PTTSet ts, WORD EmuOld);
void WINAPI TEKRestoreSetup(PTEKVar tk, PTTSet ts);
void WINAPI TEKEnd(PTEKVar tk);

#ifdef __cplusplus
}
#endif
