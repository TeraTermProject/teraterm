/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, scroll buffer routines */

#ifdef __cplusplus
extern "C" {
#endif

void InitBuffer();
void LockBuffer();
void UnlockBuffer();
void FreeBuffer();
void BuffReset();
void BuffAllSelect();
void BuffScreenSelect();
void BuffCancelSelection();
void ChangeSelectRegion();
void BuffScroll(int Count, int Bottom);
void BuffInsertSpace(int Count);
void BuffEraseCurToEnd();
void BuffEraseHomeToCur();
void BuffInsertLines(int Count, int YEnd);
void BuffEraseCharsInLine(int XStart, int Count);
void BuffDeleteLines(int Count, int YEnd);
void BuffDeleteChars(int Count);
void BuffEraseChars(int Count);
void BuffFillWithE();
void BuffDrawLine(TCharAttr Attr, int Direction, int C);
void BuffEraseBox(int XStart, int YStart, int XEnd, int YEnd);
void BuffCBCopy(BOOL Table);
void BuffPrint(BOOL ScrollRegion);
void BuffDumpCurrentLine(BYTE TERM);
void BuffPutChar(BYTE b, TCharAttr Attr, BOOL Insert);
void BuffPutKanji(WORD w, TCharAttr Attr, BOOL Insert);
void BuffUpdateRect(int XStart, int YStart, int XEnd, int YEnd);
void UpdateStr();
void UpdateStrUnicode(void);
void MoveCursor(int Xnew, int Ynew);
void MoveRight();
void BuffSetCaretWidth();
void BuffScrollNLines(int n);
void BuffClearScreen();
void BuffUpdateScroll();
void CursorUpWithScroll();
int BuffUrlDblClk(int Xw, int Yw);
void BuffDblClk(int Xw, int Yw);
void BuffTplClk(int Yw);
void BuffSeveralPagesSelect(int Xw, int Yw);
void BuffStartSelect(int Xw, int Yw, BOOL Box);
void BuffChangeSelect(int Xw, int Yw, int NClick);
void BuffEndSelect();
void BuffChangeWinSize(int Nx, int Ny);
void BuffChangeTerminalSize(int Nx, int Ny);
void ChangeWin();
void ClearBuffer();
void SetTabStop();
void CursorForwardTab(int count, BOOL AutoWrapMode);
void CursorBackwardTab(int count);
void ClearTabStop(int Ps);
void ShowStatusLine(int Show);
#ifndef NO_COPYLINE_FIX
void BuffLineContinued(BOOL mode);
#define SetLineContinued() BuffLineContinued(TRUE)
#define ClearLineContinued() BuffLineContinued(FALSE)
#endif /* NO_COPYLINE_FIX */
void BuffRegionScrollUpNLines(int n);
void BuffRegionScrollDownNLines(int n);
void BuffSetCurCharAttr(TCharAttr Attr);
void BuffSaveScreen();
void BuffRestoreScreen();

extern int StatusLine;
extern int CursorTop, CursorBottom;
extern BOOL Selected;
extern BOOL Wrap;

#ifdef __cplusplus
}
#endif
