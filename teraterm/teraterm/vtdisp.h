/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2008-2017 TeraTerm Project
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

/* TERATERM.EXE, VT terminal display routines */
#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
//<!--by AKASI
void BGInitialize(void);
void BGSetupPrimary(BOOL);

void BGExchangeColor(void);

void BGOnSettingChange(void);
void BGOnEnterSizeMove(void);
void BGOnExitSizeMove(void);

extern BOOL BGEnable;
extern BOOL BGNoFrame;
extern BOOL BGNoCopyBits;
//-->

void InitDisp();
void EndDisp();
void DispReset();
void DispConvWinToScreen
  (int Xw, int Yw, int *Xs, int *Ys, PBOOL Right);
void DispConvScreenToWin
  (int Xs, int Ys, int *Xw, int *Yw);
void SetLogFont();
void ChangeFont();
void ResetIME();
void ChangeCaret();
void CaretKillFocus(BOOL show);
void UpdateCaretPosition(BOOL enforce);
void CaretOn();
void CaretOff();
void DispDestroyCaret();
BOOL IsCaretOn();
void DispEnableCaret(BOOL On);
BOOL IsCaretEnabled();
void DispSetCaretWidth(BOOL DW);
void DispChangeWinSize(int Nx, int Ny);
void ResizeWindow(int x, int y, int w, int h, int cw, int ch);
void PaintWindow(HDC PaintDC, RECT PaintRect, BOOL fBkGnd,
		 int* Xs, int* Ys, int* Xe, int* Ye);
void DispEndPaint();
void DispClearWin();
void DispChangeBackground();
void DispChangeWin();
void DispInitDC();
void DispReleaseDC();
void DispSetupDC(TCharAttr Attr, BOOL Reverse);
void DispStr(PCHAR Buff, int Count, int Y, int* X);
void DispEraseCurToEnd(int YEnd);
void DispEraseHomeToCur(int YHome);
void DispEraseCharsInLine(int XStart, int Count);
BOOL DispDeleteLines(int Count, int YEnd);
BOOL DispInsertLines(int Count, int YEnd);
BOOL IsLineVisible(int* X, int* Y);
void AdjustScrollBar();
void DispScrollToCursor(int CurX, int CurY);
void DispScrollNLines(int Top, int Bottom, int Direction);
void DispCountScroll();
void DispUpdateScroll();
void DispScrollHomePos();
void DispAutoScroll(POINT p);
void DispHScroll(int Func, int Pos);
void DispVScroll(int Func, int Pos);
void DispSetupFontDlg();
void DispRestoreWinSize();
void DispSetWinPos();
void DispSetActive(BOOL ActiveFlag);
void InitColorTable();
void DispApplyANSIColor();
void DispSetNearestColors(int start, int end, HDC DispCtx);
int TCharAttrCmp(TCharAttr a, TCharAttr b);
void DispSetColor(unsigned int num, COLORREF color);
void DispResetColor(unsigned int num);
COLORREF DispGetColor(unsigned int num);
void DispSetCurCharAttr(TCharAttr Attr);
void DispMoveWindow(int x, int y);
void DispShowWindow(int mode);
void DispResizeWin(int w, int h);
BOOL DispWindowIconified();
void DispGetWindowPos(int *x, int *y);
void DispGetWindowSize(int *width, int *height, BOOL client);
void DispGetRootWinSize(int *x, int *y);
int DispFindClosestColor(int red, int green, int blue);
void UpdateBGBrush(void);

extern int WinWidth, WinHeight;
extern HFONT VTFont[AttrFontMask+1];
extern int FontHeight, FontWidth, ScreenWidth, ScreenHeight;
extern BOOL AdjustSize, DontChangeSize;
extern int CursorX, CursorY;
extern int WinOrgX, WinOrgY, NewOrgX, NewOrgY;
extern int NumOfLines, NumOfColumns;
extern int PageStart, BuffEnd;
extern TCharAttr DefCharAttr;

#define SCROLL_BOTTOM	1
#define SCROLL_LINEDOWN	2
#define SCROLL_LINEUP	3
#define SCROLL_PAGEDOWN	4
#define SCROLL_PAGEUP	5
#define SCROLL_POS	6
#define SCROLL_TOP	7

#define WINDOW_MINIMIZE 1
#define WINDOW_MAXIMIZE 2
#define WINDOW_RESTORE  3
#define WINDOW_RAISE    4
#define WINDOW_LOWER    5
#define WINDOW_REFRESH  6

#ifdef __cplusplus
}
#endif
