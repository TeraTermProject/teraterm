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
void BuffFillBox(char c, int XStart, int YStart, int XEnd, int YEnd);
void BuffCopyBox(int SrcXStart, int SrcYStart, int SrcXEnd, int SrcYEnd, int SrcPage, int DstX, int DstY, int DstPage);
void BuffChangeAttrBox(int XStart, int YStart, int XEnd, int YEnd, PCharAttr attr, PCharAttr mask);
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
void BuffLineContinued(BOOL mode);
#define SetLineContinued() BuffLineContinued(TRUE)
#define ClearLineContinued() BuffLineContinued(FALSE)
void BuffRegionScrollUpNLines(int n);
void BuffRegionScrollDownNLines(int n);
void BuffSetCurCharAttr(TCharAttr Attr);
void BuffSaveScreen();
void BuffRestoreScreen();
void BuffDiscardSavedScreen();
void BuffSelectedEraseCharsInLine(int XStart, int Count);
void BuffSelectedEraseCurToEnd();
void BuffSelectedEraseHomeToCur();
void BuffSelectedEraseScreen();
void BuffSelectiveEraseBox(int XStart, int YStart, int XEnd, int YEnd);
void BuffScrollLeft(int count);
void BuffScrollRight(int count);
int BuffGetCurrentLineData(char *buf, int bufsize);
int BuffGetAnyLineData(int offset_y, char *buf, int bufsize);
BOOL BuffCheckMouseOnURL(int Xw, int Yw);

extern int StatusLine;
extern int CursorTop, CursorBottom, CursorLeftM, CursorRightM;
extern BOOL Selected;
extern BOOL Wrap;

#define isCursorOnStatusLine (StatusLine && CursorY == NumOfLines-1)

#ifdef __cplusplus
}
#endif
