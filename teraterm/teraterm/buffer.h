/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005- TeraTerm Project
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

/* TERATERM.EXE, scroll buffer routines */

#pragma once

#include "teraprnfile.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* Character attribute bit masks */
#define AttrDefault       0x00
#define AttrDefaultFG     0x00
#define AttrDefaultBG     0x00
#define AttrBold          0x01
#define AttrUnder         0x02
#define AttrSpecial       0x04
#define AttrFontMask      0x07
#define AttrBlink         0x08
#define AttrReverse       0x10
#define AttrLineContinued 0x20 /* valid only at the beggining or end of a line */
#define AttrURL           0x40
#define AttrKanji         0x80		// 1=�S�p(2cell)/0=���p(1cell)  (buffer.c,h �����̂�)
#define AttrPadding       0x100		// 1=padding(2cell�̎���1cell or �s��)  (vtterm - buffer �Ԃ̂�)

  /* Color attribute bit masks */
#define Attr2Fore         0x01
#define Attr2Back         0x02
#define AttrSgrMask       (AttrBold | AttrUnder | AttrBlink | AttrReverse)
#define AttrColorMask     (AttrBold | AttrBlink | AttrReverse)
#define Attr2ColorMask    (Attr2Fore | Attr2Back)

#define Attr2Protect      0x04

typedef struct {
	BYTE Attr;
	BYTE Attr2;
	WORD AttrEx;	// �A�g���r���[�g�𑝂₷�e�X�g
	BYTE Fore;
	BYTE Back;
} TCharAttr;

typedef TCharAttr *PCharAttr;

void InitBuffer(BOOL use_unicode_api);
void LockBuffer(void);
void UnlockBuffer(void);
void FreeBuffer(void);
void BuffReset(void);
void BuffAllSelect(void);
void BuffScreenSelect(void);
void BuffCancelSelection(void);
void ChangeSelectRegion(void);
void BuffInsertSpace(int Count);
void BuffEraseCurToEnd(void);
void BuffEraseHomeToCur(void);
void BuffInsertLines(int Count, int YEnd);
void BuffEraseCharsInLine(int XStart, int Count);
void BuffDeleteLines(int Count, int YEnd);
void BuffDeleteChars(int Count);
void BuffEraseChars(int Count);
void BuffFillWithE(void);
void BuffDrawLine(const TCharAttr *Attr, int Direction, int C);
void BuffEraseBox(int XStart, int YStart, int XEnd, int YEnd);
void BuffFillBox(char c, int XStart, int YStart, int XEnd, int YEnd);
void BuffCopyBox(int SrcXStart, int SrcYStart, int SrcXEnd, int SrcYEnd, int SrcPage, int DstX, int DstY, int DstPage);
void BuffChangeAttrBox(int XStart, int YStart, int XEnd, int YEnd, const PCharAttr attr, const PCharAttr mask);
void BuffChangeAttrStream(int XStart, int YStart, int XEnd, int YEnd, const PCharAttr attr, const PCharAttr mask);
wchar_t *BuffCBCopyUnicode(BOOL Table);
void BuffPrint(BOOL ScrollRegion);
void BuffDumpCurrentLine(PrintFile *handle, BYTE TERM);
int BuffPutUnicode(unsigned int uc, const TCharAttr *Attr, BOOL Insert);
void BuffUpdateRect(int XStart, int YStart, int XEnd, int YEnd);
void UpdateStr(void);
void UpdateStrUnicode(void);
void MoveCursor(int Xnew, int Ynew);
void MoveRight(void);
void BuffSetCaretWidth(void);
void BuffScrollNLines(int n);
void BuffClearScreen(void);
void BuffUpdateScroll(void);
void CursorUpWithScroll(void);
int BuffUrlDblClk(int Xw, int Yw);
void BuffDblClk(int Xw, int Yw);
void BuffTplClk(int Yw);
void BuffSeveralPagesSelect(int Xw, int Yw);
void BuffStartSelect(int Xw, int Yw, BOOL Box);
void BuffChangeSelect(int Xw, int Yw, int NClick);
wchar_t *BuffEndSelect(void);
void BuffChangeWinSize(int Nx, int Ny);
void BuffChangeTerminalSize(int Nx, int Ny);
void ChangeWin(void);
void ClearBuffer(void);
void SetTabStop(void);
void CursorForwardTab(int count, BOOL AutoWrapMode);
void CursorBackwardTab(int count);
void ClearTabStop(int Ps);
void ShowStatusLine(int Show);
void BuffLineContinued(BOOL mode);
#define SetLineContinued() BuffLineContinued(TRUE)
#define ClearLineContinued() BuffLineContinued(FALSE)
void BuffRegionScrollUpNLines(int n);
void BuffRegionScrollDownNLines(int n);
void BuffSetCurCharAttr(const TCharAttr *Attr);
void BuffSaveScreen(void);
void BuffRestoreScreen(void);
void BuffDiscardSavedScreen(void);
void BuffSelectedEraseCharsInLine(int XStart, int Count);
void BuffSelectedEraseCurToEnd(void);
void BuffSelectedEraseHomeToCur(void);
void BuffSelectedEraseScreen(void);
void BuffSelectiveEraseBox(int XStart, int YStart, int XEnd, int YEnd);
void BuffScrollLeft(int count);
void BuffScrollRight(int count);
int BuffGetCurrentLineData(char *buf, int bufsize);
wchar_t *BuffGetLineStrW(int Sy, int *cx, size_t *length);
int BuffGetAnyLineData(int offset_y, char *buf, int bufsize);
int BuffGetAnyLineDataW(int offset_y, wchar_t *buf, size_t bufsize);
BOOL BuffCheckMouseOnURL(int Xw, int Yw);
wchar_t *BuffGetCharInfo(int Xw, int Yw);
void BuffSetCursorCharAttr(int x, int y, const TCharAttr *Attr);
TCharAttr BuffGetCursorCharAttr(int x, int y);
BOOL BuffIsCombiningCharacter(int x, int y, unsigned int u32);
void BuffSetDispAPI(BOOL unicode);
void BuffSetDispCodePage(int CodePage);
int BuffGetDispCodePage(void);
BOOL BuffIsSelected(void);

extern int StatusLine;
extern int CursorTop, CursorBottom, CursorLeftM, CursorRightM;
extern BOOL Wrap;

#define isCursorOnStatusLine (StatusLine && CursorY == NumOfLines-1)

#ifdef __cplusplus
}
#endif
