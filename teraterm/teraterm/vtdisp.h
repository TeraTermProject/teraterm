/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2008- TeraTerm Project
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

#pragma once

#include "buffer.h"		// for TCharAttr

#define VTDISP_DEBUG_DISABLE 1

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
  /* for DispSetColor() / DispGetColor() */
// ANSIColor -- 0-255
#define CS_VT_NORMALFG     256
#define CS_VT_NORMALBG     257
#define CS_VT_BOLDFG       258
#define CS_VT_BOLDBG       259
#define CS_VT_BLINKFG      260
#define CS_VT_BLINKBG      261
#define CS_VT_REVERSEFG    262
#define CS_VT_REVERSEBG    263
#define CS_VT_URLFG        264
#define CS_VT_URLBG        265
#define CS_VT_UNDERFG      266
#define CS_VT_UNDERBG      267
#define CS_TEK_FG          268
#define CS_TEK_BG          269
#define CS_ANSICOLOR_ALL   270
#define CS_SP_ALL          271
#define CS_UNSPEC          0xffffffff
#define CS_ALL             0xfffffffe	// DispResetColor() だけで使用

// HDC の wapper
typedef struct {
	HDC VTDC;			// 描画に使用するDC
	HWND HVTWin;		// DCの元になったHWND(プリンタの場合はNULL)
	HFONT DCPrevFont;	// DCに最初にセレクトされていたfont
	int PrnX, PrnY;		// 文字の描画位置(pixel), 描画するとPrnXが+方向に増加する
	BOOL IsPrinter;		// TRUEのとき、プリンタ
	BYTE DCBackAlpha;
} ttdc_t;

// VT Winの描画情報、ワーク
typedef struct vtdraw_tag {
	HWND hVTWin;					// VT Win Window handle(ウィドウの時)
	HFONT VTFont[AttrFontMask+1];	// 各フォント
	int FontWidth, FontHeight;		// フォントサイズ(pixel/1cell)
	BOOL IsPrinter;					// TRUEのとき、プリンタ
	BOOL IsColorPrinter;			// TRUEのとき、プリンタでカラー印刷(実験的実装)
	RECT Margin;					// 文字描画時の余白(pixel)、プリンタ用
	COLORREF White, Black;			// プリンタ用
	BOOL debug_drawbox_text;		// 文字描画毎にboxを描画する
	HBRUSH Background;				// background paintbrush
	TCharAttr CurCharAttr;			// ディスプレイ用
} vtdraw_t;

// VTWinの描画情報
//   TODO 移動する vtwinあたりか?
extern vtdraw_t *vt_src;

/* prototypes */
void BGInitialize(BOOL initialize_once);
void BGLoadThemeFile(const TTTSet *pts);
void BGSetupPrimary(vtdraw_t *vt, BOOL forceSetup);

void BGOnSettingChange(vtdraw_t *vt);
void BGOnEnterSizeMove(void);
void BGOnExitSizeMove(vtdraw_t *vt);

vtdraw_t *InitDisp(HWND hVTWin);
void EndDisp(vtdraw_t *vt);
void DispReset(vtdraw_t *vt);
void DispConvWinToScreen(vtdraw_t *vt, int Xw, int Yw, int *Xs, int *Ys, PBOOL Right);
void DispConvScreenToWin(vtdraw_t *vt, int Xs, int Ys, int *Xw, int *Yw);
void ChangeFont(vtdraw_t *vt, unsigned int dpi);
void ResetIME(vtdraw_t *vt);
void ChangeCaret(vtdraw_t *vt);
void CaretKillFocus(vtdraw_t *vt, BOOL show);
void UpdateCaretPosition(vtdraw_t *vt, BOOL enforce);
void CaretOn(vtdraw_t *vt);
void CaretOff(vtdraw_t *vt);
void DispDestroyCaret(void);
BOOL IsCaretOn(void);
void DispEnableCaret(vtdraw_t *vt, BOOL On);
BOOL IsCaretEnabled(void);
void DispSetCaretWidth(BOOL DW);
void DispChangeWinSize(vtdraw_t *vt, int Nx, int Ny);
void ResizeWindow(vtdraw_t *vt, int x, int y, int w, int h, int cw, int ch);
ttdc_t *PaintWindow(vtdraw_t *vt, HDC PaintDC, RECT PaintRect, BOOL fBkGnd,
		 int* Xs, int* Ys, int* Xe, int* Ye);
void DispEndPaint(ttdc_t *dc);
void DispClearWin(vtdraw_t *vt);
void DispChangeBackground(vtdraw_t *vt);
void DispChangeWin(vtdraw_t *vt);
void DispSetupDC(vtdraw_t *vt, ttdc_t *dc, const TCharAttr *Attr, BOOL Reverse);
BOOL DispDeleteLines(vtdraw_t *vt, int Count, int YEnd);
BOOL DispInsertLines(vtdraw_t *vt, int Count, int YEnd);
BOOL IsLineVisible(vtdraw_t *vt, int *X, int *Y);
void AdjustScrollBar(void);
void DispScrollToCursor(int CurX, int CurY);
void DispScrollNLines(vtdraw_t *vt, int Top, int Bottom, int Direction);
void DispCountScroll(vtdraw_t *vt, int n);
void DispUpdateScroll(vtdraw_t *vt);
void DispScrollHomePos(vtdraw_t *vt);
void DispAutoScroll(vtdraw_t *vt, POINT p);
void DispHScroll(vtdraw_t *vt, int Func, int Pos);
void DispVScroll(vtdraw_t *vt, int Func, int Pos);
void DispRestoreWinSize(vtdraw_t *vt);
void DispSetWinPos(vtdraw_t *vt);
void DispSetActive(vtdraw_t *vt, BOOL ActiveFlag);
int TCharAttrCmp(TCharAttr a, TCharAttr b);
void DispSetColor(vtdraw_t *vt, unsigned int num, COLORREF color);
void DispResetColor(vtdraw_t *vt, unsigned int num);
COLORREF DispGetColor(unsigned int num);
void DispSetCurCharAttr(vtdraw_t *vt, const TCharAttr *Attr);
void DispMoveWindow(int x, int y);
void DispShowWindow(int mode);
void DispResizeWin(int w, int h);
BOOL DispWindowIconified(void);
void DispGetWindowPos(int *x, int *y, BOOL client);
void DispGetWindowSize(int *width, int *height, BOOL client);
void DispGetRootWinSize(vtdraw_t *vt, int *x, int *y, BOOL inPixels);
int DispFindClosestColor(int red, int green, int blue);
void DispEnableResizedFont(BOOL enable);
BOOL DispIsResizedFont();

ttdc_t *DispInitDC(vtdraw_t *vt);
ttdc_t *DispInitDCDebug(vtdraw_t *vt, const char *file, int line);
void DispReleaseDC(vtdraw_t *vt, ttdc_t *dc);
void DispReleaseDCDebug(vtdraw_t *vt, ttdc_t *dc, const char *file, int line);
#ifndef VTDISP_DEBUG_DISABLE
#define DispInitDC(p1)	DispInitDCDebug(p1, __FILE__, __LINE__)
#define DispReleaseDC(p1, p2) DispReleaseDCDebug(p1, p2, __FILE__, __LINE__)
#endif
void DispStrA(vtdraw_t *vt, ttdc_t *dc, const char *Buff, const char *WidthInfo, int Count);
void DispStrW(vtdraw_t *vt, ttdc_t *dc, const wchar_t *StrW, const char *WidthInfo, int Count);
BOOL DispIsPrinter(vtdraw_t *vt);
BOOL DispDCIsPrinter(ttdc_t *dc);
void DispSetDrawPos(vtdraw_t *vt, ttdc_t *dc, int x, int y);

void DispFontCreate(vtdraw_t *vt, ttdc_t *dc, LOGFONTW VTlf);
void DispFontDelete(vtdraw_t *vt);

extern int WinWidth, WinHeight;
extern int ScreenWidth, ScreenHeight;
extern BOOL AdjustSize, DontChangeSize;
extern int CursorX, CursorY;
extern int WinOrgX, WinOrgY, NewOrgX, NewOrgY;
extern int NumOfLines, NumOfColumns;
extern int PageStart, BuffEnd;
extern TCharAttr DefCharAttr;

extern BOOL IMEstat;
extern BOOL IMECompositionState;

// for DispHScroll(), DispVScroll()
#define SCROLL_BOTTOM	1
#define SCROLL_LINEDOWN	2
#define SCROLL_LINEUP	3
#define SCROLL_PAGEDOWN	4
#define SCROLL_PAGEUP	5
#define SCROLL_POS	6
#define SCROLL_TOP	7

// for DispShowWindow()
#define WINDOW_MINIMIZE 1
#define WINDOW_MAXIMIZE 2
#define WINDOW_RESTORE  3
#define WINDOW_RAISE    4
#define WINDOW_LOWER    5
#define WINDOW_REFRESH  6
#define WINDOW_TOGGLE_MAXIMIZE 7

#ifdef __cplusplus
}
#endif
