/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, VT terminal display routines */
#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
//<!--by AKASI
void BGInitialize(void);
void BGSetupPrimary(BOOL);

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
void DispSetANSIColor(int num, COLORREF color);
COLORREF DispGetANSIColor(int num);
void DispSetCurCharAttr(TCharAttr Attr);
void DispMoveWindow(int x, int y);
void DispShowWindow(int mode);
void DispResizeWin(int w, int h);
BOOL DispWindowIconified();
void DispGetWindowPos(int *x, int *y);

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
