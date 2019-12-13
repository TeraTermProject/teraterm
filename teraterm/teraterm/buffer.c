/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004-2019 TeraTerm Project
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

#include "teraterm.h"
#include <string.h>
#include <stdio.h>
#include <windows.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <mbstring.h>
#include <assert.h>
#include <errno.h>

#include "tttypes.h"
#include "ttwinman.h"
#include "teraprn.h"
#include "vtdisp.h"
#include "clipboar.h"
#include "telnet.h"
#include "ttplug.h" /* TTPLUG */
#include "codeconv.h"
#include "unicode.h"
#include "buffer.h"
#include "unicode_test.h"

#if UNICODE_INTERNAL_BUFF

typedef unsigned long char32_t;		// C++11

// バッファ内の半角1文字分の情報
typedef struct {
	char32_t u32;
	char32_t u32_last;
	char WidthProperty;				// 'W' or 'F' or 'H' or 'A' (文字の属性)
	char HalfWidth;					// TRUE/FALSE = 半角/全角 (表示するときの文字幅)
	char Padding;					// TRUE = 全角の次の詰め物 or 行末の詰め物
	char Emoji;						// TRUE = 絵文字
	char CombinationCharCount16;	// charactor count
	char CombinationCharSize16;		// buffer size
	char CombinationCharCount32;
	char CombinationCharSize32;
	wchar_t *pCombinationChars16;
	char32_t *pCombinationChars32;
	wchar_t	wc2[2];
} buff_char_t;
#endif

#define BuffXMax TermWidthMax
//#define BuffYMax 100000
//#define BuffSizeMax 8000000
// スクロールバッファの最大長を拡張 (2004.11.28 yutaka)
#define BuffYMax 500000
#define BuffSizeMax (BuffYMax * 80)

// status line
int StatusLine;	//0: none 1: shown
/* top, bottom, left & right margin */
int CursorTop, CursorBottom, CursorLeftM, CursorRightM;
BOOL Selected, Selecting;
BOOL Wrap;

static WORD TabStops[256];
static int NTabStops;

static WORD BuffLock = 0;

static PCHAR CodeBuff;  /* Character code buffer */
static PCHAR AttrBuff;  /* Attribute buffer */
static PCHAR AttrBuff2; /* Color attr buffer */
static PCHAR AttrBuffFG; /* Foreground color attr buffer */
static PCHAR AttrBuffBG; /* Background color attr buffer */
static PCHAR CodeLine;
static PCHAR AttrLine;
static PCHAR AttrLine2;
static PCHAR AttrLineFG;
static PCHAR AttrLineBG;
#if UNICODE_INTERNAL_BUFF
static buff_char_t *CodeBuffW;
static buff_char_t *CodeLineW;
#endif
static LONG LinePtr;
static LONG BufferSize;
static int NumOfLinesInBuff;
static int BuffStartAbs, BuffEndAbs;
static POINT SelectStart, SelectStartTmp, SelectEnd, SelectEndOld;
static DWORD SelectStartTime;
static BOOL BoxSelect;
static POINT DblClkStart, DblClkEnd;

// 描画
static int StrChangeStart;	// 描画開始 X (Y=CursorY)
static int StrChangeCount;	// 描画キャラクタ数(半角単位),0のとき描画するものがない

static BOOL SeveralPageSelect;  // add (2005.5.15 yutaka)

static TCharAttr CurCharAttr;

static char *SaveBuff = NULL;
static int SaveBuffX;
static int SaveBuffY;

#if UNICODE_INTERNAL_BUFF
static void BuffDrawLineI(int DrawX, int DrawY, int SY, int IStart, int IEnd);
#endif

#if UNICODE_INTERNAL_BUFF
static void BuffSetChar2(buff_char_t *buff, char32_t u32, char property, BOOL half_width, char emoji)
{
	size_t wstr_len;
	buff_char_t *p = buff;
	if (p->pCombinationChars16 != NULL) {
		free(p->pCombinationChars16);
		p->pCombinationChars16 = NULL;
	}
	p->CombinationCharCount16 = 0;
	p->CombinationCharSize16 = 0;
	if (p->pCombinationChars32 != NULL) {
		free(p->pCombinationChars32);
		p->pCombinationChars32 = NULL;
	}
	p->CombinationCharCount32 = 0;
	p->CombinationCharSize32 = 0;
	p->WidthProperty = property;
	p->HalfWidth = (char)half_width;
	p->u32 = u32;
	p->u32_last = u32;
	p->Padding = FALSE;
	p->Emoji = emoji;

	//
	wstr_len = UTF32ToUTF16(u32, &p->wc2[0], 2);
	switch (wstr_len) {
	case 0:
	default:
		p->wc2[0] = 0;
		p->wc2[1] = 0;
		break;
	case 1:
		p->wc2[1] = 0;
		break;
	case 2:
		break;
	}
}

static void BuffSetChar(buff_char_t *buff, char32_t u32, char property)
{
	BuffSetChar2(buff, u32, property, TRUE, FALSE);
}

/**
 *	文字の追加、コンビネーション
 */
static void BuffAddChar(buff_char_t *buff, char32_t u32)
{
	buff_char_t *p = buff;
	assert(p->u32 != 0);
	// 後に続く文字領域を拡大する
	if (p->CombinationCharSize16 < p->CombinationCharCount16 + 2) {
		size_t new_size = p->CombinationCharSize16;
		new_size = new_size == 0 ? 5 : new_size * 2;
		p->pCombinationChars16 = realloc(p->pCombinationChars16, sizeof(wchar_t) * new_size);
		p->CombinationCharSize16 = (char)new_size;
	}
	if (p->CombinationCharSize32 < p->CombinationCharCount32 + 2) {
		size_t new_size = p->CombinationCharSize32;
		new_size = new_size == 0 ? 5 : new_size * 2;
		p->pCombinationChars32 = realloc(p->pCombinationChars32, sizeof(char32_t) * new_size);
		p->CombinationCharSize32 = (char)new_size;
	}

	// UTF-32
	p->u32_last = u32;
	p->pCombinationChars32[(size_t)p->CombinationCharCount32] = u32;
	p->CombinationCharCount32++;

	// UTF-16
	{
		wchar_t *u16 = &p->pCombinationChars16[(size_t)p->CombinationCharCount16];
		size_t wlen = UTF32ToUTF16(u32, u16, 2);
		p->CombinationCharCount16 += (char)wlen;
	}
}

static void memcpyW(buff_char_t *dest, const buff_char_t *src, size_t count)
{
	size_t i;
	memcpy(dest, src, count * sizeof(buff_char_t));
	for (i=0; i<count; i++) {
		buff_char_t *p = &dest[i];
		size_t size = p->CombinationCharSize16;
		if (size > 0) {
			wchar_t *new_buf = malloc(sizeof(wchar_t) * size);
			memcpy(new_buf, p->pCombinationChars16, sizeof(wchar_t) * size);
			p->pCombinationChars16 = new_buf;
		}
		size = p->CombinationCharSize32;
		if (size > 0) {
			char32_t *new_buf = malloc(sizeof(char32_t) * size);
			memcpy(new_buf, p->pCombinationChars32, sizeof(char32_t) * size);
			p->pCombinationChars32 = new_buf;
		}
	}
}

static void memsetW(buff_char_t *dest, wchar_t ch, size_t count)
{
	size_t i;
	for (i=0; i<count; i++) {
		BuffSetChar(dest, ch, 'H');
		dest++;
	}
}

static void memmoveW(buff_char_t *dest, const buff_char_t *src, size_t count)
{
	memmove(dest, src, count * sizeof(buff_char_t));
}

static BOOL IsBuffPadding(const buff_char_t *b)
{
	if (b->Padding == TRUE)
		return TRUE;
	if (b->u32 == 0)
		return TRUE;
	return FALSE;
}

static BOOL IsBuffFullWidth(const buff_char_t *b)
{
	if (b->HalfWidth == FALSE)
		return TRUE;
	return FALSE;
}

#endif

static LONG GetLinePtr(int Line)
{
	LONG Ptr;

	Ptr = (LONG)(BuffStartAbs + Line) * (LONG)(NumOfColumns);
	while (Ptr>=BufferSize) {
		Ptr = Ptr - BufferSize;
	}
	return Ptr;
}

static LONG NextLinePtr(LONG Ptr)
{
	Ptr = Ptr + (LONG)NumOfColumns;
	if (Ptr >= BufferSize) {
		Ptr = Ptr - BufferSize;
	}
	return Ptr;
}

static LONG PrevLinePtr(LONG Ptr)
{
	Ptr = Ptr - (LONG)NumOfColumns;
	if (Ptr < 0) {
		Ptr = Ptr + BufferSize;
	}
	return Ptr;
}

static BOOL ChangeBuffer(int Nx, int Ny)
{
	LONG NewSize;
	int NxCopy, NyCopy, i;
	PCHAR CodeDest, AttrDest, AttrDest2, AttrDestFG, AttrDestBG;
	LONG SrcPtr, DestPtr;
	WORD LockOld;
#if UNICODE_INTERNAL_BUFF
	buff_char_t *CodeDestW;
#endif

	if (Nx > BuffXMax) {
		Nx = BuffXMax;
	}
	if (ts.ScrollBuffMax > BuffYMax) {
		ts.ScrollBuffMax = BuffYMax;
	}
	if (Ny > ts.ScrollBuffMax) {
		Ny = ts.ScrollBuffMax;
	}

	if ( (LONG)Nx * (LONG)Ny > BuffSizeMax ) {
		Ny = BuffSizeMax / Nx;
	}

	NewSize = (LONG)Nx * (LONG)Ny;

	CodeDest = NULL;
	AttrDest = NULL;
	AttrDest2 = NULL;
	AttrDestFG = NULL;
	AttrDestBG = NULL;
#if UNICODE_INTERNAL_BUFF
	CodeDestW = NULL;
#endif

	CodeDest = malloc(NewSize);
	if (CodeDest == NULL) {
		goto allocate_error;
	}
	AttrDest = malloc(NewSize);
	if (AttrDest == NULL) {
		goto allocate_error;
	}
	AttrDest2 = malloc(NewSize);
	if (AttrDest2 == NULL) {
		goto allocate_error;
	}
	AttrDestFG = malloc(NewSize);
	if (AttrDestFG == NULL) {
		goto allocate_error;
	}
	AttrDestBG = malloc(NewSize);
	if (AttrDestBG == NULL) {
		goto allocate_error;
	}
#if UNICODE_INTERNAL_BUFF
	CodeDestW = malloc(NewSize * sizeof(buff_char_t));
	if (CodeDestW == NULL) {
		goto allocate_error;
	}
#endif

	memset(&CodeDest[0], 0x20, NewSize);
#if UNICODE_INTERNAL_BUFF
	memset(&CodeDestW[0], 0, NewSize * sizeof(buff_char_t));
	memsetW(&CodeDestW[0], 0x20, NewSize);
#endif
	memset(&AttrDest[0], AttrDefault, NewSize);
	memset(&AttrDest2[0], AttrDefault, NewSize);
	memset(&AttrDestFG[0], AttrDefaultFG, NewSize);
	memset(&AttrDestBG[0], AttrDefaultBG, NewSize);
	if ( CodeBuff != NULL ) {
		if ( NumOfColumns > Nx ) {
			NxCopy = Nx;
		}
		else {
			NxCopy = NumOfColumns;
		}

		if ( BuffEnd > Ny ) {
			NyCopy = Ny;
		}
		else {
			NyCopy = BuffEnd;
		}
		LockOld = BuffLock;
		LockBuffer();
		SrcPtr = GetLinePtr(BuffEnd-NyCopy);
		DestPtr = 0;
		for (i = 1 ; i <= NyCopy ; i++) {
			memcpy(&CodeDest[DestPtr],&CodeBuff[SrcPtr],NxCopy);
#if UNICODE_INTERNAL_BUFF
			memcpyW(&CodeDestW[DestPtr],&CodeBuffW[SrcPtr],NxCopy);
#endif
			memcpy(&AttrDest[DestPtr],&AttrBuff[SrcPtr],NxCopy);
			memcpy(&AttrDest2[DestPtr],&AttrBuff2[SrcPtr],NxCopy);
			memcpy(&AttrDestFG[DestPtr],&AttrBuffFG[SrcPtr],NxCopy);
			memcpy(&AttrDestBG[DestPtr],&AttrBuffBG[SrcPtr],NxCopy);
			if (AttrDest[DestPtr+NxCopy-1] & AttrKanji) {
				CodeDest[DestPtr+NxCopy-1] = ' ';
				AttrDest[DestPtr+NxCopy-1] ^= AttrKanji;
			}
			SrcPtr = NextLinePtr(SrcPtr);
			DestPtr = DestPtr + (LONG)Nx;
		}
		FreeBuffer();
	}
	else {
		LockOld = 0;
		NyCopy = NumOfLines;
		Selected = FALSE;
	}

	if (Selected) {
		SelectStart.y = SelectStart.y - BuffEnd + NyCopy;
		SelectEnd.y = SelectEnd.y - BuffEnd + NyCopy;
		if (SelectStart.y < 0) {
			SelectStart.y = 0;
			SelectStart.x = 0;
		}
		if (SelectEnd.y<0) {
			SelectEnd.x = 0;
			SelectEnd.y = 0;
		}

		Selected = (SelectEnd.y > SelectStart.y) ||
		           ((SelectEnd.y == SelectStart.y) &&
		            (SelectEnd.x > SelectStart.x));
	}

	CodeBuff = CodeDest;
	AttrBuff = AttrDest;
	AttrBuff2 = AttrDest2;
	AttrBuffFG = AttrDestFG;
	AttrBuffBG = AttrDestBG;
#if UNICODE_INTERNAL_BUFF
	CodeBuffW = CodeDestW;
#endif
	BufferSize = NewSize;
	NumOfLinesInBuff = Ny;
	BuffStartAbs = 0;
	BuffEnd = NyCopy;

	if (BuffEnd==NumOfLinesInBuff) {
		BuffEndAbs = 0;
	}
	else {
		BuffEndAbs = BuffEnd;
	}

	PageStart = BuffEnd - NumOfLines;

	LinePtr = 0;
	if (LockOld>0) {
		CodeLine = CodeBuff;
#if UNICODE_INTERNAL_BUFF
		CodeLineW = CodeBuffW;
#endif
		AttrLine = AttrBuff;
		AttrLine2 = AttrBuff2;
		AttrLineFG = AttrBuffFG;
		AttrLineBG = AttrBuffBG;
	}
	else {
		;
	}
	BuffLock = LockOld;

	return TRUE;

allocate_error:
	if (CodeDest)   free(CodeDest);
	if (AttrDest)   free(AttrDest);
	if (AttrDest2)  free(AttrDest2);
	if (AttrDestFG) free(AttrDestFG);
	if (AttrDestBG) free(AttrDestBG);
#if UNICODE_INTERNAL_BUFF
	if (CodeDestW)  free(CodeDestW);
#endif
	return FALSE;
}

void InitBuffer()
{
	int Ny;

	/* setup terminal */
	NumOfColumns = ts.TerminalWidth;
	NumOfLines = ts.TerminalHeight;

	if (NumOfColumns <= 0)
		NumOfColumns = 80;
	else if (NumOfColumns > TermWidthMax)
		NumOfColumns = TermWidthMax;

	if (NumOfLines <= 0)
		NumOfLines = 24;
	else if (NumOfLines > TermHeightMax)
		NumOfLines = TermHeightMax;

	/* setup window */
	if (ts.EnableScrollBuff>0) {
		if (ts.ScrollBuffSize < NumOfLines) {
			ts.ScrollBuffSize = NumOfLines;
		}
		Ny = ts.ScrollBuffSize;
	}
	else {
		Ny = NumOfLines;
	}

	if (! ChangeBuffer(NumOfColumns,Ny)) {
		PostQuitMessage(0);
	}

	if (ts.EnableScrollBuff>0) {
		ts.ScrollBuffSize = NumOfLinesInBuff;
	}

	StatusLine = 0;
}

static void NewLine(int Line)
{
	LinePtr = GetLinePtr(Line);
	CodeLine = &CodeBuff[LinePtr];
	AttrLine = &AttrBuff[LinePtr];
	AttrLine2 = &AttrBuff2[LinePtr];
	AttrLineFG = &AttrBuffFG[LinePtr];
	AttrLineBG = &AttrBuffBG[LinePtr];
#if UNICODE_INTERNAL_BUFF
	CodeLineW = &CodeBuffW[LinePtr];
#endif
}

void LockBuffer()
{
	BuffLock++;
	if (BuffLock>1) {
		return;
	}
	NewLine(PageStart+CursorY);
}

void UnlockBuffer()
{
	if (BuffLock==0) {
		return;
	}
	BuffLock--;
	if (BuffLock>0) {
		return;
	}
}

void FreeBuffer()
{
	BuffLock = 1;
	UnlockBuffer();
	if (CodeBuff!=NULL) {
		free(CodeBuff);
		CodeBuff = NULL;
	}
#if UNICODE_INTERNAL_BUFF
	if (CodeBuffW != NULL) {
		free(CodeBuffW);
		CodeBuffW = NULL;
	}
#endif
	if (AttrBuff!=NULL) {
		free(AttrBuff);
		AttrBuff = NULL;
	}
	if (AttrBuff2!=NULL) {
		free(AttrBuff2);
		AttrBuff2 = NULL;
	}
	if (AttrBuffFG!=NULL) {
		free(AttrBuffFG);
		AttrBuffFG = NULL;
	}
	if (AttrBuffBG!=NULL) {
		free(AttrBuffBG);
		AttrBuffBG = NULL;
	}
}

void BuffAllSelect()
{
	SelectStart.x = 0;
	SelectStart.y = 0;
	SelectEnd.x = 0;
	SelectEnd.y = BuffEnd;
//	SelectEnd.x = NumOfColumns;
//	SelectEnd.y = BuffEnd - 1;
	Selecting = TRUE;
}

void BuffScreenSelect()
{
	int X, Y;
	DispConvWinToScreen(0, 0, &X, &Y, NULL);
	SelectStart.x = X;
	SelectStart.y = Y + PageStart;
	SelectEnd.x = 0;
	SelectEnd.y = SelectStart.y + NumOfLines;
//	SelectEnd.x = X + NumOfColumns;
//	SelectEnd.y = Y + PageStart + NumOfLines - 1;
	Selecting = TRUE;
}

void BuffCancelSelection()
{
	SelectStart.x = 0;
	SelectStart.y = 0;
	SelectEnd.x = 0;
	SelectEnd.y = 0;
	Selecting = FALSE;
}

void BuffReset()
// Reset buffer status. don't update real display
//   called by ResetTerminal()
{
	int i;

	/* Cursor */
	NewLine(PageStart);
	WinOrgX = 0;
	WinOrgY = 0;
	NewOrgX = 0;
	NewOrgY = 0;

	/* Top/bottom margin */
	CursorTop = 0;
	CursorBottom = NumOfLines-1;
	CursorLeftM = 0;
	CursorRightM = NumOfColumns-1;

	/* Tab stops */
	NTabStops = (NumOfColumns-1) >> 3;
	for (i=1 ; i<=NTabStops ; i++) {
		TabStops[i-1] = (WORD)(i*8);
	}

	/* Initialize text selection region */
	SelectStart.x = 0;
	SelectStart.y = 0;
	SelectEnd = SelectStart;
	SelectEndOld = SelectStart;
	Selected = FALSE;

	StrChangeCount = 0;
	Wrap = FALSE;
	StatusLine = 0;

	SeveralPageSelect = FALSE; // yutaka

	/* Alternate Screen Buffer */
	BuffDiscardSavedScreen();
}

void BuffScroll(int Count, int Bottom)
{
	int i, n;
	LONG SrcPtr, DestPtr;
	int BuffEndOld;

	if (Count>NumOfLinesInBuff) {
		Count = NumOfLinesInBuff;
	}

	DestPtr = GetLinePtr(PageStart+NumOfLines-1+Count);
	n = Count;
	if (Bottom<NumOfLines-1) {
		SrcPtr = GetLinePtr(PageStart+NumOfLines-1);
		for (i=NumOfLines-1; i>=Bottom+1; i--) {
			memcpy(&(CodeBuff[DestPtr]),&(CodeBuff[SrcPtr]),NumOfColumns);
#if UNICODE_INTERNAL_BUFF
			memcpyW(&(CodeBuffW[DestPtr]),&(CodeBuffW[SrcPtr]),NumOfColumns);
#endif
			memcpy(&(AttrBuff[DestPtr]),&(AttrBuff[SrcPtr]),NumOfColumns);
			memcpy(&(AttrBuff2[DestPtr]),&(AttrBuff2[SrcPtr]),NumOfColumns);
			memcpy(&(AttrBuffFG[DestPtr]),&(AttrBuffFG[SrcPtr]),NumOfColumns);
			memcpy(&(AttrBuffBG[DestPtr]),&(AttrBuffBG[SrcPtr]),NumOfColumns);
			memset(&(CodeBuff[SrcPtr]),0x20,NumOfColumns);
#if UNICODE_INTERNAL_BUFF
			memsetW(&(CodeBuffW[SrcPtr]),0x20,NumOfColumns);
#endif
			memset(&(AttrBuff[SrcPtr]),AttrDefault,NumOfColumns);
			memset(&(AttrBuff2[SrcPtr]),CurCharAttr.Attr2 & Attr2ColorMask, NumOfColumns);
			memset(&(AttrBuffFG[SrcPtr]),CurCharAttr.Fore,NumOfColumns);
			memset(&(AttrBuffBG[SrcPtr]),CurCharAttr.Back,NumOfColumns);
			SrcPtr = PrevLinePtr(SrcPtr);
			DestPtr = PrevLinePtr(DestPtr);
			n--;
		}
	}
	for (i = 1 ; i <= n ; i++) {
		memset(&CodeBuff[DestPtr],0x20,NumOfColumns);
#if UNICODE_INTERNAL_BUFF
		memsetW(&CodeBuffW[DestPtr],0x20,NumOfColumns);
#endif
		memset(&AttrBuff[DestPtr],AttrDefault,NumOfColumns);
		memset(&AttrBuff2[DestPtr],CurCharAttr.Attr2 & Attr2ColorMask, NumOfColumns);
		memset(&AttrBuffFG[DestPtr],CurCharAttr.Fore,NumOfColumns);
		memset(&AttrBuffBG[DestPtr],CurCharAttr.Back,NumOfColumns);
		DestPtr = PrevLinePtr(DestPtr);
	}

	BuffEndAbs = BuffEndAbs + Count;
	if (BuffEndAbs >= NumOfLinesInBuff) {
		BuffEndAbs = BuffEndAbs - NumOfLinesInBuff;
	}
	BuffEndOld = BuffEnd;
	BuffEnd = BuffEnd + Count;
	if (BuffEnd >= NumOfLinesInBuff) {
		BuffEnd = NumOfLinesInBuff;
		BuffStartAbs = BuffEndAbs;
	}
	PageStart = BuffEnd-NumOfLines;

	if (Selected) {
		SelectStart.y = SelectStart.y - Count + BuffEnd - BuffEndOld;
		SelectEnd.y = SelectEnd.y - Count + BuffEnd - BuffEndOld;
		if ( SelectStart.y<0 ) {
			SelectStart.x = 0;
			SelectStart.y = 0;
		}
		if ( SelectEnd.y<0 ) {
			SelectEnd.x = 0;
			SelectEnd.y = 0;
		}
		Selected = (SelectEnd.y > SelectStart.y) ||
	               ((SelectEnd.y==SelectStart.y) &&
	                (SelectEnd.x > SelectStart.x));
	}

	NewLine(PageStart+CursorY);
}

void NextLine()
{
	LinePtr = NextLinePtr(LinePtr);
	CodeLine = &CodeBuff[LinePtr];
	AttrLine = &AttrBuff[LinePtr];
	AttrLine2 = &AttrBuff2[LinePtr];
	AttrLineFG = &AttrBuffFG[LinePtr];
	AttrLineBG = &AttrBuffBG[LinePtr];
#if UNICODE_INTERNAL_BUFF
	CodeLineW = &CodeBuffW[LinePtr];
#endif
}

void PrevLine()
{
	LinePtr = PrevLinePtr(LinePtr);
	CodeLine = &CodeBuff[LinePtr];
	AttrLine = &AttrBuff[LinePtr];
	AttrLine2 = &AttrBuff2[LinePtr];
	AttrLineFG = &AttrBuffFG[LinePtr];
	AttrLineBG = &AttrBuffBG[LinePtr];
#if UNICODE_INTERNAL_BUFF
	CodeLineW = &CodeBuffW[LinePtr];
#endif
}

// If cursor is on left/right half of a Kanji, erase it.
//   LR: left(0)/right(1) flag
//	LR	0	カーソルが漢字の左側
//		1	カーソルが漢字の右側
static void EraseKanji(int LR)
{
#if UNICODE_INTERNAL_BUFF
	buff_char_t *p;
	int bx;
	if (CursorX < LR) {
		// 全角判定できない
		return;
	}
	bx = CursorX-LR;
	p = &CodeLineW[bx];
	if (IsBuffFullWidth(p)) {
		// 全角をつぶす
		BuffSetChar(p, ' ', 'H');
		CodeLine[bx] = 0x20;
		AttrLine[bx] = CurCharAttr.Attr;
		AttrLine2[bx] = CurCharAttr.Attr2;
		AttrLineFG[bx] = CurCharAttr.Fore;
		AttrLineBG[bx] = CurCharAttr.Back;
		if (bx+1 < NumOfColumns) {
			BuffSetChar(p + 1, ' ', 'H');
			CodeLine[bx+1] = 0x20;
			AttrLine[bx+1] = CurCharAttr.Attr;
			AttrLine2[bx+1] = CurCharAttr.Attr2;
			AttrLineFG[bx+1] = CurCharAttr.Fore;
			AttrLineBG[bx+1] = CurCharAttr.Back;
		}
	}
#else
	if ((CursorX-LR>=0) &&
	    ((AttrLine[CursorX-LR] & AttrKanji) != 0)) {
		CodeLine[CursorX-LR] = 0x20;
		AttrLine[CursorX-LR] = CurCharAttr.Attr;
		AttrLine2[CursorX-LR] = CurCharAttr.Attr2;
		AttrLineFG[CursorX-LR] = CurCharAttr.Fore;
		AttrLineBG[CursorX-LR] = CurCharAttr.Back;
		if (CursorX-LR+1 < NumOfColumns) {
			CodeLine[CursorX-LR+1] = 0x20;
			AttrLine[CursorX-LR+1] = CurCharAttr.Attr;
			AttrLine2[CursorX-LR+1] = CurCharAttr.Attr2;
			AttrLineFG[CursorX-LR+1] = CurCharAttr.Fore;
			AttrLineBG[CursorX-LR+1] = CurCharAttr.Back;
		}
	}
#endif
}

void EraseKanjiOnLRMargin(LONG ptr, int count)
{
	int i;
	LONG pos;

	if (count < 1)
		return;

	for (i=0; i<count; i++) {
		pos = ptr + CursorLeftM-1;
		if (CursorLeftM>0 && (AttrBuff[pos] & AttrKanji)) {
			CodeBuff[pos] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[pos], 0x20, 'H');
#endif
			AttrBuff[pos] &= ~AttrKanji;
			pos++;
			CodeBuff[pos] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[pos], 0x20, 'H');
#endif
			AttrBuff[pos] &= ~AttrKanji;
		}
		pos = ptr + CursorRightM;
		if (CursorRightM < NumOfColumns-1 && (AttrBuff[pos] & AttrKanji)) {
			CodeBuff[pos] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[pos], 0x20, 'H');
#endif
			AttrBuff[pos] &= ~AttrKanji;
			pos++;
			CodeBuff[pos] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[pos], 0x20, 'H');
#endif
			AttrBuff[pos] &= ~AttrKanji;
		}
		ptr = NextLinePtr(ptr);
	}
}

// Insert space characters at the current position
//   Count: Number of characters to be inserted
#if UNICODE_INTERNAL_BUFF
void BuffInsertSpace(int Count)
{
	int MoveLen;
	int extr = 0;
	int sx;
	buff_char_t *b;

	if (CursorX < CursorLeftM || CursorX > CursorRightM)
		return;

	NewLine(PageStart + CursorY);

	sx = CursorX;
	b = &CodeLineW[CursorX];
	if (IsBuffPadding(b)) {
		/* if cursor is on right half of a kanji, erase the kanji */
		BuffSetChar(b - 1, ' ', 'H');
		BuffSetChar(b, ' ', 'H');
		AttrLine[CursorX] &= ~AttrKanji;
		sx--;
		extr++;
	}

	if (CursorRightM < NumOfColumns - 1 && (AttrLine[CursorRightM] & AttrKanji)) {
		CodeLine[CursorRightM + 1] = 0x20;
		BuffSetChar(&CodeLineW[CursorRightM + 1], 0x20, 'H');
		AttrLine[CursorRightM + 1] &= ~AttrKanji;
		extr++;
	}

	if (Count > CursorRightM + 1 - CursorX)
		Count = CursorRightM + 1 - CursorX;

	MoveLen = CursorRightM + 1 - CursorX - Count;

	if (MoveLen > 0) {
		memmove(&(CodeLine[CursorX + Count]), &(CodeLine[CursorX]), MoveLen);
		memmoveW(&(CodeLineW[CursorX + Count]), &(CodeLineW[CursorX]), MoveLen);
		memmove(&(AttrLine[CursorX + Count]), &(AttrLine[CursorX]), MoveLen);
		memmove(&(AttrLine2[CursorX + Count]), &(AttrLine2[CursorX]), MoveLen);
		memmove(&(AttrLineFG[CursorX + Count]), &(AttrLineFG[CursorX]), MoveLen);
		memmove(&(AttrLineBG[CursorX + Count]), &(AttrLineBG[CursorX]), MoveLen);
	}
	memset(&(CodeLine[CursorX]), 0x20, Count);
	memsetW(&(CodeLineW[CursorX]), 0x20, Count);
	memset(&(AttrLine[CursorX]), AttrDefault, Count);
	memset(&(AttrLine2[CursorX]), CurCharAttr.Attr2 & Attr2ColorMask, Count);
	memset(&(AttrLineFG[CursorX]), CurCharAttr.Fore, Count);
	memset(&(AttrLineBG[CursorX]), CurCharAttr.Back, Count);
	/* last char in current line is kanji first? */
	if ((AttrLine[CursorRightM] & AttrKanji) != 0) {
		/* then delete it */
		CodeLine[CursorRightM] = 0x20;
		BuffSetChar(&CodeLineW[CursorRightM], 0x20, 'H');
		AttrLine[CursorRightM] &= ~AttrKanji;
	}
	BuffUpdateRect(sx, CursorY, CursorRightM + extr, CursorY);
}
#else
void BuffInsertSpace(int Count)
// Insert space characters at the current position
//   Count: Number of characters to be inserted
{
	int MoveLen;
	int extr=0;

	if (CursorX < CursorLeftM || CursorX > CursorRightM)
		return;

	NewLine(PageStart+CursorY);

	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8)
		EraseKanji(1); /* if cursor is on right half of a kanji, erase the kanji */

	if (CursorRightM < NumOfColumns-1 && (AttrLine[CursorRightM] & AttrKanji)) {
		CodeLine[CursorRightM+1] = 0x20;
#if UNICODE_INTERNAL_BUFF
		BuffSetChar(&CodeLineW[CursorRightM + 1], 0x20, 'H');
#endif
		AttrLine[CursorRightM+1] &= ~AttrKanji;
		extr = 1;
	}

	if (Count > CursorRightM + 1 - CursorX)
		Count = CursorRightM + 1 - CursorX;

	MoveLen = CursorRightM + 1 - CursorX - Count;

	if (MoveLen > 0) {
		memmove(&(CodeLine[CursorX+Count]), &(CodeLine[CursorX]), MoveLen);
#if UNICODE_INTERNAL_BUFF
		memmoveW(&(CodeLineW[CursorX+Count]), &(CodeLineW[CursorX]), MoveLen);
#endif
		memmove(&(AttrLine[CursorX+Count]), &(AttrLine[CursorX]), MoveLen);
		memmove(&(AttrLine2[CursorX+Count]), &(AttrLine2[CursorX]), MoveLen);
		memmove(&(AttrLineFG[CursorX+Count]), &(AttrLineFG[CursorX]), MoveLen);
		memmove(&(AttrLineBG[CursorX+Count]), &(AttrLineBG[CursorX]), MoveLen);
	}
	memset(&(CodeLine[CursorX]), 0x20, Count);
#if UNICODE_INTERNAL_BUFF
	memsetW(&(CodeLineW[CursorX]), 0x20, Count);
#endif
	memset(&(AttrLine[CursorX]), AttrDefault, Count);
	memset(&(AttrLine2[CursorX]), CurCharAttr.Attr2 & Attr2ColorMask, Count);
	memset(&(AttrLineFG[CursorX]), CurCharAttr.Fore, Count);
	memset(&(AttrLineBG[CursorX]), CurCharAttr.Back, Count);
	/* last char in current line is kanji first? */
	if ((AttrLine[CursorRightM] & AttrKanji) != 0) {
		/* then delete it */
		CodeLine[CursorRightM] = 0x20;
#if UNICODE_INTERNAL_BUFF
		BuffSetChar(&CodeLineW[CursorRightM], 0x20, 'H');
#endif
		AttrLine[CursorRightM] &= ~AttrKanji;
	}
	BuffUpdateRect(CursorX, CursorY, CursorRightM+extr, CursorY);
}
#endif

void BuffEraseCurToEnd()
// Erase characters from cursor to the end of screen
{
	LONG TmpPtr;
	int offset;
	int i, YEnd;

	NewLine(PageStart+CursorY);
	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		EraseKanji(1); /* if cursor is on right half of a kanji, erase the kanji */
	}
	offset = CursorX;
	TmpPtr = GetLinePtr(PageStart+CursorY);
	YEnd = NumOfLines-1;
	if (StatusLine && !isCursorOnStatusLine) {
		YEnd--;
	}
	for (i = CursorY ; i <= YEnd ; i++) {
		memset(&(CodeBuff[TmpPtr+offset]),0x20,NumOfColumns-offset);
#if UNICODE_INTERNAL_BUFF
		memsetW(&(CodeBuffW[TmpPtr+offset]),0x20,NumOfColumns-offset);
#endif
		memset(&(AttrBuff[TmpPtr+offset]),AttrDefault,NumOfColumns-offset);
		memset(&(AttrBuff2[TmpPtr+offset]),CurCharAttr.Attr2 & Attr2ColorMask, NumOfColumns-offset);
		memset(&(AttrBuffFG[TmpPtr+offset]),CurCharAttr.Fore,NumOfColumns-offset);
		memset(&(AttrBuffBG[TmpPtr+offset]),CurCharAttr.Back,NumOfColumns-offset);
		offset = 0;
		TmpPtr = NextLinePtr(TmpPtr);
	}
	/* update window */
	DispEraseCurToEnd(YEnd);
}

void BuffEraseHomeToCur()
// Erase characters from home to cursor
{
	LONG TmpPtr;
	int offset;
	int i, YHome;

	NewLine(PageStart+CursorY);
	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		EraseKanji(0); /* if cursor is on left half of a kanji, erase the kanji */
	}
	offset = NumOfColumns;
	if (isCursorOnStatusLine) {
		YHome = CursorY;
	}
	else {
		YHome = 0;
	}
	TmpPtr = GetLinePtr(PageStart+YHome);
	for (i = YHome ; i <= CursorY ; i++) {
		if (i==CursorY) {
			offset = CursorX+1;
		}
		memset(&(CodeBuff[TmpPtr]),0x20,offset);
#if UNICODE_INTERNAL_BUFF
		memsetW(&(CodeBuffW[TmpPtr]),0x20,offset);
#endif
		memset(&(AttrBuff[TmpPtr]),AttrDefault,offset);
		memset(&(AttrBuff2[TmpPtr]),CurCharAttr.Attr2 & Attr2ColorMask, offset);
		memset(&(AttrBuffFG[TmpPtr]),CurCharAttr.Fore,offset);
		memset(&(AttrBuffBG[TmpPtr]),CurCharAttr.Back,offset);
		TmpPtr = NextLinePtr(TmpPtr);
	}

	/* update window */
	DispEraseHomeToCur(YHome);
}

void BuffInsertLines(int Count, int YEnd)
// Insert lines at current position
//   Count: number of lines to be inserted
//   YEnd: bottom line number of scroll region (screen coordinate)
{
	int i, linelen;
	int extl=0, extr=0;
	LONG SrcPtr, DestPtr;

	BuffUpdateScroll();

	if (CursorLeftM > 0)
		extl = 1;
	if (CursorRightM < NumOfColumns-1)
		extr = 1;
	if (extl || extr)
		EraseKanjiOnLRMargin(GetLinePtr(PageStart+CursorY), YEnd-CursorY+1);

	SrcPtr = GetLinePtr(PageStart+YEnd-Count) + CursorLeftM;
	DestPtr = GetLinePtr(PageStart+YEnd) + CursorLeftM;
	linelen = CursorRightM - CursorLeftM + 1;
	for (i= YEnd-Count ; i>=CursorY ; i--) {
		memcpy(&(CodeBuff[DestPtr]), &(CodeBuff[SrcPtr]), linelen);
#if UNICODE_INTERNAL_BUFF
		memcpyW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
#endif
		memcpy(&(AttrBuff[DestPtr]), &(AttrBuff[SrcPtr]), linelen);
		memcpy(&(AttrBuff2[DestPtr]), &(AttrBuff2[SrcPtr]), linelen);
		memcpy(&(AttrBuffFG[DestPtr]), &(AttrBuffFG[SrcPtr]), linelen);
		memcpy(&(AttrBuffBG[DestPtr]), &(AttrBuffBG[SrcPtr]), linelen);
		SrcPtr = PrevLinePtr(SrcPtr);
		DestPtr = PrevLinePtr(DestPtr);
	}
	for (i = 1 ; i <= Count ; i++) {
		memset(&(CodeBuff[DestPtr]), 0x20, linelen);
#if UNICODE_INTERNAL_BUFF
		memsetW(&(CodeBuffW[DestPtr]), 0x20, linelen);
#endif
		memset(&(AttrBuff[DestPtr]), AttrDefault, linelen);
		memset(&(AttrBuff2[DestPtr]), CurCharAttr.Attr2 & Attr2ColorMask, linelen);
		memset(&(AttrBuffFG[DestPtr]), CurCharAttr.Fore, linelen);
		memset(&(AttrBuffBG[DestPtr]), CurCharAttr.Back, linelen);
		DestPtr = PrevLinePtr(DestPtr);
	}

	if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1 || !DispInsertLines(Count, YEnd)) {
		BuffUpdateRect(CursorLeftM-extl, CursorY, CursorRightM+extr, YEnd);
	}
}

void BuffEraseCharsInLine(int XStart, int Count)
// erase characters in the current line
//  XStart: start position of erasing
//  Count: number of characters to be erased
{
	BOOL LineContinued=FALSE;

	if (ts.EnableContinuedLineCopy && XStart == 0 && (AttrLine[0] & AttrLineContinued)) {
		LineContinued = TRUE;
	}

	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		EraseKanji(1); /* if cursor is on right half of a kanji, erase the kanji */
	}

	NewLine(PageStart+CursorY);
	memset(&(CodeLine[XStart]),0x20,Count);
#if UNICODE_INTERNAL_BUFF
	memsetW(&(CodeLineW[XStart]),0x20,Count);
#endif
	memset(&(AttrLine[XStart]),AttrDefault,Count);
	memset(&(AttrLine2[XStart]),CurCharAttr.Attr2 & Attr2ColorMask, Count);
	memset(&(AttrLineFG[XStart]),CurCharAttr.Fore,Count);
	memset(&(AttrLineBG[XStart]),CurCharAttr.Back,Count);

	if (ts.EnableContinuedLineCopy) {
		if (LineContinued) {
			BuffLineContinued(TRUE);
		}

		if (XStart + Count >= NumOfColumns) {
			AttrBuff[NextLinePtr(LinePtr)] &= ~AttrLineContinued;
		}
	}

	DispEraseCharsInLine(XStart, Count);
}

void BuffDeleteLines(int Count, int YEnd)
// Delete lines from current line
//   Count: number of lines to be deleted
//   YEnd: bottom line number of scroll region (screen coordinate)
{
	int i, linelen;
	int extl=0, extr=0;
	LONG SrcPtr, DestPtr;

	BuffUpdateScroll();

	if (CursorLeftM > 0)
		extl = 1;
	if (CursorRightM < NumOfColumns-1)
		extr = 1;
	if (extl || extr)
		EraseKanjiOnLRMargin(GetLinePtr(PageStart+CursorY), YEnd-CursorY+1);

	SrcPtr = GetLinePtr(PageStart+CursorY+Count) + (LONG)CursorLeftM;
	DestPtr = GetLinePtr(PageStart+CursorY) + (LONG)CursorLeftM;
	linelen = CursorRightM - CursorLeftM + 1;
	for (i=CursorY ; i<= YEnd-Count ; i++) {
		memcpy(&(CodeBuff[DestPtr]), &(CodeBuff[SrcPtr]), linelen);
#if UNICODE_INTERNAL_BUFF
		memcpyW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
#endif
		memcpy(&(AttrBuff[DestPtr]), &(AttrBuff[SrcPtr]), linelen);
		memcpy(&(AttrBuff2[DestPtr]), &(AttrBuff2[SrcPtr]), linelen);
		memcpy(&(AttrBuffFG[DestPtr]), &(AttrBuffFG[SrcPtr]), linelen);
		memcpy(&(AttrBuffBG[DestPtr]), &(AttrBuffBG[SrcPtr]), linelen);
		SrcPtr = NextLinePtr(SrcPtr);
		DestPtr = NextLinePtr(DestPtr);
	}
	for (i = YEnd+1-Count ; i<=YEnd ; i++) {
		memset(&(CodeBuff[DestPtr]), 0x20, linelen);
#if UNICODE_INTERNAL_BUFF
		memsetW(&(CodeBuffW[DestPtr]), 0x20, linelen);
#endif
		memset(&(AttrBuff[DestPtr]), AttrDefault, linelen);
		memset(&(AttrBuff2[DestPtr]), CurCharAttr.Attr2 & Attr2ColorMask,  linelen);
		memset(&(AttrBuffFG[DestPtr]), CurCharAttr.Fore, linelen);
		memset(&(AttrBuffBG[DestPtr]), CurCharAttr.Back, linelen);
		DestPtr = NextLinePtr(DestPtr);
	}

	if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1 || ! DispDeleteLines(Count,YEnd)) {
		BuffUpdateRect(CursorLeftM-extl, CursorY, CursorRightM+extr, YEnd);
	}
}

// Delete characters in current line from cursor
//   Count: number of characters to be deleted
#if UNICODE_INTERNAL_BUFF
void BuffDeleteChars(int Count)
{
	int MoveLen;
	int extr = 0;
	buff_char_t *b;

	if (Count > CursorRightM + 1 - CursorX)
		Count = CursorRightM + 1 - CursorX;

	if (CursorX < CursorLeftM || CursorX > CursorRightM)
		return;

	NewLine(PageStart + CursorY);

	b = &CodeLineW[CursorX];

	if (IsBuffPadding(b)) {
		// 全角の右側、全角をスペースに置き換える
		BuffSetChar(b - 1, ' ', 'H');
		BuffSetChar(b, ' ', 'H');
	}
	if (IsBuffFullWidth(b)) {
		// 全角の左側、全角をスペースに置き換える
		BuffSetChar(b, ' ', 'H');
		BuffSetChar(b + 1, ' ', 'H');
	}
	if (Count > 1) {
		// 終端をチェック
		if (IsBuffPadding(b + Count)) {
			// 全角の右側、全角をスペースに置き換える
			BuffSetChar(b + Count - 1, ' ', 'H');
			BuffSetChar(b + Count, ' ', 'H');
		}
	}

	if (CursorRightM < NumOfColumns - 1 && (AttrLine[CursorRightM] & AttrKanji)) {
		CodeLine[CursorRightM] = 0x20;
		BuffSetChar(&CodeLineW[CursorRightM], 0x20, 'H');
		AttrLine[CursorRightM] &= ~AttrKanji;
		CodeLine[CursorRightM + 1] = 0x20;
		BuffSetChar(&CodeLineW[CursorRightM + 1], 0x20, 'H');
		AttrLine[CursorRightM + 1] &= ~AttrKanji;
		extr = 1;
	}

	MoveLen = CursorRightM + 1 - CursorX - Count;

	if (MoveLen > 0) {
		memmove(&(CodeLine[CursorX]), &(CodeLine[CursorX + Count]), MoveLen);
		memmoveW(&(CodeLineW[CursorX]), &(CodeLineW[CursorX + Count]), MoveLen);
		memmove(&(AttrLine[CursorX]), &(AttrLine[CursorX + Count]), MoveLen);
		memmove(&(AttrLine2[CursorX]), &(AttrLine2[CursorX + Count]), MoveLen);
		memmove(&(AttrLineFG[CursorX]), &(AttrLineFG[CursorX + Count]), MoveLen);
		memmove(&(AttrLineBG[CursorX]), &(AttrLineBG[CursorX + Count]), MoveLen);
	}
	memset(&(CodeLine[CursorX + MoveLen]), 0x20, Count);
	memsetW(&(CodeLineW[CursorX + MoveLen]), ' ', Count);
	memset(&(AttrLine[CursorX + MoveLen]), AttrDefault, Count);
	memset(&(AttrLine2[CursorX + MoveLen]), CurCharAttr.Attr2 & Attr2ColorMask, Count);
	memset(&(AttrLineFG[CursorX + MoveLen]), CurCharAttr.Fore, Count);
	memset(&(AttrLineBG[CursorX + MoveLen]), CurCharAttr.Back, Count);

	BuffUpdateRect(CursorX, CursorY, CursorRightM + extr, CursorY);
}
#else
void BuffDeleteChars(int Count)
{
	int MoveLen;
	int extr=0;

	if (CursorX < CursorLeftM || CursorX > CursorRightM)
		return;

	NewLine(PageStart+CursorY);

	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		EraseKanji(0); /* if cursor is on left harf of a kanji, erase the kanji */
		EraseKanji(1); /* if cursor on right half... */
	}

	if (CursorRightM < NumOfColumns-1 && (AttrLine[CursorRightM] & AttrKanji)) {
		CodeLine[CursorRightM] = 0x20;
		AttrLine[CursorRightM] &= ~AttrKanji;
		CodeLine[CursorRightM+1] = 0x20;
		AttrLine[CursorRightM+1] &= ~AttrKanji;
		extr = 1;
	}

	if (Count > CursorRightM + 1 - CursorX)
		Count = CursorRightM + 1 - CursorX;

	MoveLen = CursorRightM + 1 - CursorX - Count;

	if (MoveLen > 0) {
		memmove(&(CodeLine[CursorX]), &(CodeLine[CursorX+Count]), MoveLen);
		memmove(&(AttrLine[CursorX]), &(AttrLine[CursorX+Count]), MoveLen);
		memmove(&(AttrLine2[CursorX]), &(AttrLine2[CursorX+Count]), MoveLen);
		memmove(&(AttrLineFG[CursorX]), &(AttrLineFG[CursorX+Count]), MoveLen);
		memmove(&(AttrLineBG[CursorX]), &(AttrLineBG[CursorX+Count]), MoveLen);
	}
	memset(&(CodeLine[CursorX + MoveLen]), 0x20, Count);
	memset(&(AttrLine[CursorX + MoveLen]), AttrDefault, Count);
	memset(&(AttrLine2[CursorX + MoveLen]), CurCharAttr.Attr2 & Attr2ColorMask, Count);
	memset(&(AttrLineFG[CursorX + MoveLen]), CurCharAttr.Fore, Count);
	memset(&(AttrLineBG[CursorX + MoveLen]), CurCharAttr.Back, Count);

	BuffUpdateRect(CursorX, CursorY, CursorRightM+extr, CursorY);
}
#endif

// Erase characters in current line from cursor
//   Count: number of characters to be deleted
#if UNICODE_INTERNAL_BUFF
void BuffEraseChars(int Count)
{
	int extr = 0;
	int sx = CursorX;
	buff_char_t *b;
	NewLine(PageStart + CursorY);

	if (Count > NumOfColumns - CursorX) {
		Count = NumOfColumns - CursorX;
	}

	b = &CodeLineW[CursorX];
	if (IsBuffPadding(b)) {
		// 全角の右側、全角をスペースに置き換える
		BuffSetChar(b - 1, ' ', 'H');
		BuffSetChar(b, ' ', 'H');
		sx--;
		extr++;
	}
	if (IsBuffFullWidth(b)) {
		// 全角の左側、全角をスペースに置き換える
		BuffSetChar(b, ' ', 'H');
		BuffSetChar(b + 1, ' ', 'H');
		if (Count == 1) {
			extr++;
		}
	}
	if (Count > 1) {
		// 終端をチェック
		if (IsBuffPadding(b + Count)) {
			// 全角の右側、全角をスペースに置き換える
			BuffSetChar(b + Count - 1, ' ', 'H');
			BuffSetChar(b + Count, ' ', 'H');
			extr++;
		}
	}

	memset(&(CodeLine[CursorX]), 0x20, Count);
#if UNICODE_INTERNAL_BUFF
	memsetW(&(CodeLineW[CursorX]), 0x20, Count);
#endif
	memset(&(AttrLine[CursorX]), AttrDefault, Count);
	memset(&(AttrLine2[CursorX]), CurCharAttr.Attr2 & Attr2ColorMask, Count);
	memset(&(AttrLineFG[CursorX]), CurCharAttr.Fore, Count);
	memset(&(AttrLineBG[CursorX]), CurCharAttr.Back, Count);

	/* update window */
	DispEraseCharsInLine(sx, Count + extr);
}
#else
void BuffEraseChars(int Count)
{
	NewLine(PageStart+CursorY);

	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		EraseKanji(0); /* if cursor is on left harf of a kanji, erase the kanji */
		EraseKanji(1); /* if cursor on right half... */
	}

	if (Count > NumOfColumns-CursorX) {
		Count = NumOfColumns-CursorX;
	}
	memset(&(CodeLine[CursorX]),0x20,Count);
#if UNICODE_INTERNAL_BUFF
	memsetW(&(CodeLineW[CursorX]),0x20,Count);
#endif
	memset(&(AttrLine[CursorX]),AttrDefault,Count);
	memset(&(AttrLine2[CursorX]),CurCharAttr.Attr2 & Attr2ColorMask, Count);
	memset(&(AttrLineFG[CursorX]),CurCharAttr.Fore,Count);
	memset(&(AttrLineBG[CursorX]),CurCharAttr.Back,Count);

	/* update window */
	DispEraseCharsInLine(CursorX,Count);
}
#endif

void BuffFillWithE()
// Fill screen with 'E' characters
{
	LONG TmpPtr;
	int i;

	TmpPtr = GetLinePtr(PageStart);
	for (i = 0 ; i <= NumOfLines-1-StatusLine ; i++) {
		memset(&(CodeBuff[TmpPtr]),'E',NumOfColumns);
#if UNICODE_INTERNAL_BUFF
		memsetW(&(CodeBuffW[TmpPtr]),'E',NumOfColumns);
#endif
		memset(&(AttrBuff[TmpPtr]),AttrDefault,NumOfColumns);
		memset(&(AttrBuff2[TmpPtr]),AttrDefault,NumOfColumns);
		memset(&(AttrBuffFG[TmpPtr]),AttrDefaultFG,NumOfColumns);
		memset(&(AttrBuffBG[TmpPtr]),AttrDefaultBG,NumOfColumns);
		TmpPtr = NextLinePtr(TmpPtr);
	}
	BuffUpdateRect(WinOrgX,WinOrgY,WinOrgX+WinWidth-1,WinOrgY+WinHeight-1);
}

void BuffDrawLine(TCharAttr Attr, int Direction, int C)
{ // IO-8256 terminal
	LONG Ptr;
	int i, X, Y;

	if (C==0) {
		return;
	}
	Attr.Attr |= AttrSpecial;

	switch (Direction) {
		case 3:
		case 4:
			if (Direction==3) {
				if (CursorY==0) {
					return;
				}
				Y = CursorY-1;
			}
			else {
				if (CursorY==NumOfLines-1-StatusLine) {
					return;
				}
				Y = CursorY+1;
			}
			if (CursorX+C > NumOfColumns) {
				C = NumOfColumns-CursorX;
			}
			Ptr = GetLinePtr(PageStart+Y);
			memset(&(CodeBuff[Ptr+CursorX]),'q',C);
#if UNICODE_INTERNAL_BUFF
			memsetW(&(CodeBuffW[Ptr+CursorX]),'q',C);
#endif
			memset(&(AttrBuff[Ptr+CursorX]),Attr.Attr,C);
			memset(&(AttrBuff2[Ptr+CursorX]),Attr.Attr2,C);
			memset(&(AttrBuffFG[Ptr+CursorX]),Attr.Fore,C);
			memset(&(AttrBuffBG[Ptr+CursorX]),Attr.Back,C);
			BuffUpdateRect(CursorX,Y,CursorX+C-1,Y);
			break;
		case 5:
		case 6:
			if (Direction==5) {
				if (CursorX==0) {
					return;
				}
				X = CursorX - 1;
			}
			else {
				if (CursorX==NumOfColumns-1) {
					X = CursorX-1;
				}
				else {
					X = CursorX+1;
				}
			}
			Ptr = GetLinePtr(PageStart+CursorY);
			if (CursorY+C > NumOfLines-StatusLine) {
				C = NumOfLines-StatusLine-CursorY;
			}
			for (i=1; i<=C; i++) {
				CodeBuff[Ptr+X] = 'x';
#if UNICODE_INTERNAL_BUFF
				BuffSetChar(&CodeBuffW[Ptr+X], 0x20, 'H');
#endif
				AttrBuff[Ptr+X] = Attr.Attr;
				AttrBuff2[Ptr+X] = Attr.Attr2;
				AttrBuffFG[Ptr+X] = Attr.Fore;
				AttrBuffBG[Ptr+X] = Attr.Back;
				Ptr = NextLinePtr(Ptr);
			}
			BuffUpdateRect(X,CursorY,X,CursorY+C-1);
			break;
	}
}

void BuffEraseBox
  (int XStart, int YStart, int XEnd, int YEnd)
{
	int C, i;
	LONG Ptr;

	if (XEnd>NumOfColumns-1) {
		XEnd = NumOfColumns-1;
	}
	if (YEnd>NumOfLines-1-StatusLine) {
		YEnd = NumOfLines-1-StatusLine;
	}
	if (XStart>XEnd) {
		return;
	}
	if (YStart>YEnd) {
		return;
	}
	C = XEnd-XStart+1;
	Ptr = GetLinePtr(PageStart+YStart);
	for (i=YStart; i<=YEnd; i++) {
		if ((XStart>0) &&
		    ((AttrBuff[Ptr+XStart-1] & AttrKanji) != 0)) {
			CodeBuff[Ptr+XStart-1] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[Ptr+XStart-1], 0x20, 'H');
#endif
			AttrBuff[Ptr+XStart-1] = CurCharAttr.Attr;
			AttrBuff2[Ptr+XStart-1] = CurCharAttr.Attr2;
			AttrBuffFG[Ptr+XStart-1] = CurCharAttr.Fore;
			AttrBuffBG[Ptr+XStart-1] = CurCharAttr.Back;
		}
		if ((XStart+C<NumOfColumns) &&
		    ((AttrBuff[Ptr+XStart+C-1] & AttrKanji) != 0)) {
			CodeBuff[Ptr+XStart+C] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[Ptr+XStart+C], 0x20, 'H');
#endif
			AttrBuff[Ptr+XStart+C] = CurCharAttr.Attr;
			AttrBuff2[Ptr+XStart+C] = CurCharAttr.Attr2;
			AttrBuffFG[Ptr+XStart+C] = CurCharAttr.Fore;
			AttrBuffBG[Ptr+XStart+C] = CurCharAttr.Back;
		}
		memset(&(CodeBuff[Ptr+XStart]),0x20,C);
#if UNICODE_INTERNAL_BUFF
		memsetW(&(CodeBuffW[Ptr+XStart]),0x20,C);
#endif
		memset(&(AttrBuff[Ptr+XStart]),AttrDefault,C);
		memset(&(AttrBuff2[Ptr+XStart]),CurCharAttr.Attr2 & Attr2ColorMask, C);
		memset(&(AttrBuffFG[Ptr+XStart]),CurCharAttr.Fore,C);
		memset(&(AttrBuffBG[Ptr+XStart]),CurCharAttr.Back,C);
		Ptr = NextLinePtr(Ptr);
	}
	BuffUpdateRect(XStart,YStart,XEnd,YEnd);
}

void BuffFillBox(char ch, int XStart, int YStart, int XEnd, int YEnd)
{
	int Cols, i;
	LONG Ptr;

	if (XEnd>NumOfColumns-1) {
		XEnd = NumOfColumns-1;
	}
	if (YEnd>NumOfLines-1-StatusLine) {
		YEnd = NumOfLines-1-StatusLine;
	}
	if (XStart>XEnd) {
		return;
	}
	if (YStart>YEnd) {
		return;
	}
	Cols = XEnd-XStart+1;
	Ptr = GetLinePtr(PageStart+YStart);
	for (i=YStart; i<=YEnd; i++) {
		if ((XStart>0) &&
		    ((AttrBuff[Ptr+XStart-1] & AttrKanji) != 0)) {
			CodeBuff[Ptr+XStart-1] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeLineW[Ptr + XStart - 1], 0x20, 'H');
#endif
			AttrBuff[Ptr+XStart-1] ^= AttrKanji;
		}
		if ((XStart+Cols<NumOfColumns) &&
		    ((AttrBuff[Ptr+XStart+Cols-1] & AttrKanji) != 0)) {
			CodeBuff[Ptr+XStart+Cols] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeLineW[Ptr + XStart + Cols], 0x20, 'H');
#endif
		}
		memset(&(CodeBuff[Ptr+XStart]), ch, Cols);
#if UNICODE_INTERNAL_BUFF
		BuffSetChar(&CodeLineW[Ptr + XStart], 0x20, 'H');
#endif
		memset(&(AttrBuff[Ptr+XStart]), CurCharAttr.Attr, Cols);
		memset(&(AttrBuff2[Ptr+XStart]), CurCharAttr.Attr2, Cols);
		memset(&(AttrBuffFG[Ptr+XStart]), CurCharAttr.Fore, Cols);
		memset(&(AttrBuffBG[Ptr+XStart]), CurCharAttr.Back, Cols);
		Ptr = NextLinePtr(Ptr);
	}
	BuffUpdateRect(XStart, YStart, XEnd, YEnd);
}

//
// TODO: 1 origin になってるのを 0 origin に直す
//
void BuffCopyBox(
	int SrcXStart, int SrcYStart, int SrcXEnd, int SrcYEnd, int SrcPage,
	int DstX, int DstY, int DstPage)
{
	int i, C, L;
	LONG SPtr, DPtr;

	SrcXStart--;
	SrcYStart--;
	SrcXEnd--;
	SrcYEnd--;
	SrcPage--;
	DstX--;
	DstY--;
	DstPage--;

	if (SrcXEnd > NumOfColumns - 1) {
		SrcXEnd = NumOfColumns-1;
	}
	if (SrcYEnd > NumOfLines-1-StatusLine) {
		SrcYEnd = NumOfColumns-1;
	}
	if (SrcXStart > SrcXEnd ||
	    SrcYStart > SrcYEnd ||
	    DstX > NumOfColumns-1 ||
	    DstY > NumOfLines-1-StatusLine) {
		return;
	}

	C = SrcXEnd - SrcXStart + 1;
	if (DstX + C > NumOfColumns) {
		C = NumOfColumns - DstX;
	}
	L = SrcYEnd - SrcYStart + 1;
	if (DstY + C > NumOfColumns) {
		C = NumOfColumns - DstX;
	}

	if (SrcXStart > DstX) {
		SPtr = GetLinePtr(PageStart+SrcYStart);
		DPtr = GetLinePtr(PageStart+DstY);
		for (i=0; i<L; i++) {
			memcpy(&(CodeBuff[DPtr+DstX]), &(CodeBuff[SPtr+SrcXStart]), C);
#if UNICODE_INTERNAL_BUFF
			memcpyW(&(CodeBuffW[DPtr+DstX]), &(CodeBuffW[SPtr+SrcXStart]), C);
#endif
			memcpy(&(AttrBuff[DPtr+DstX]), &(AttrBuff[SPtr+SrcXStart]), C);
			memcpy(&(AttrBuff2[DPtr+DstX]), &(AttrBuff2[SPtr+SrcXStart]), C);
			memcpy(&(AttrBuffFG[DPtr+DstX]), &(AttrBuffFG[SPtr+SrcXStart]), C);
			memcpy(&(AttrBuffBG[DPtr+DstX]), &(AttrBuffBG[SPtr+SrcXStart]), C);
			SPtr = NextLinePtr(SPtr);
			DPtr = NextLinePtr(DPtr);
		}
	}
	else if (SrcXStart < DstX) {
		SPtr = GetLinePtr(PageStart+SrcYEnd);
		DPtr = GetLinePtr(PageStart+DstY+L-1);
		for (i=L; i>0; i--) {
			memcpy(&(CodeBuff[DPtr+DstX]), &(CodeBuff[SPtr+SrcXStart]), C);
#if UNICODE_INTERNAL_BUFF
			memcpyW(&(CodeBuffW[DPtr+DstX]), &(CodeBuffW[SPtr+SrcXStart]), C);
#endif
			memcpy(&(AttrBuff[DPtr+DstX]), &(AttrBuff[SPtr+SrcXStart]), C);
			memcpy(&(AttrBuff2[DPtr+DstX]), &(AttrBuff2[SPtr+SrcXStart]), C);
			memcpy(&(AttrBuffFG[DPtr+DstX]), &(AttrBuffFG[SPtr+SrcXStart]), C);
			memcpy(&(AttrBuffBG[DPtr+DstX]), &(AttrBuffBG[SPtr+SrcXStart]), C);
			SPtr = PrevLinePtr(SPtr);
			DPtr = PrevLinePtr(DPtr);
		}
	}
	else if (SrcYStart != DstY) {
		SPtr = GetLinePtr(PageStart+SrcYStart);
		DPtr = GetLinePtr(PageStart+DstY);
		for (i=0; i<L; i++) {
			memmove(&(CodeBuff[DPtr+DstX]), &(CodeBuff[SPtr+SrcXStart]), C);
#if UNICODE_INTERNAL_BUFF
			memmoveW(&(CodeBuffW[DPtr+DstX]), &(CodeBuffW[SPtr+SrcXStart]), C);
#endif
			memmove(&(AttrBuff[DPtr+DstX]), &(AttrBuff[SPtr+SrcXStart]), C);
			memmove(&(AttrBuff2[DPtr+DstX]), &(AttrBuff2[SPtr+SrcXStart]), C);
			memmove(&(AttrBuffFG[DPtr+DstX]), &(AttrBuffFG[SPtr+SrcXStart]), C);
			memmove(&(AttrBuffBG[DPtr+DstX]), &(AttrBuffBG[SPtr+SrcXStart]), C);
			SPtr = NextLinePtr(SPtr);
			DPtr = NextLinePtr(DPtr);
		}
	}
	BuffUpdateRect(DstX,DstY,DstX+C-1,DstY+L-1);
}

void BuffChangeAttrBox(int XStart, int YStart, int XEnd, int YEnd, PCharAttr attr, PCharAttr mask)
{
	int C, i, j;
	LONG Ptr;

	if (XEnd>NumOfColumns-1) {
		XEnd = NumOfColumns-1;
	}
	if (YEnd>NumOfLines-1-StatusLine) {
		YEnd = NumOfLines-1-StatusLine;
	}
	if (XStart>XEnd || YStart>YEnd) {
		return;
	}
	C = XEnd-XStart+1;
	Ptr = GetLinePtr(PageStart+YStart);

	if (mask) { // DECCARA
		for (i=YStart; i<=YEnd; i++) {
			j = Ptr+XStart-1;
			if (XStart>0 && (AttrBuff[j] & AttrKanji)) {
				AttrBuff[j] = AttrBuff[j] & ~mask->Attr | attr->Attr;
				AttrBuff2[j] = AttrBuff2[j] & ~mask->Attr2 | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { AttrBuffFG[j] = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { AttrBuffBG[j] = attr->Back; }
			}
			while (++j < Ptr+XStart+C) {
				AttrBuff[j] = AttrBuff[j] & ~mask->Attr | attr->Attr;
				AttrBuff2[j] = AttrBuff2[j] & ~mask->Attr2 | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { AttrBuffFG[j] = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { AttrBuffBG[j] = attr->Back; }
			}
			if (XStart+C<NumOfColumns && (AttrBuff[j-1] & AttrKanji)) {
				AttrBuff[j] = AttrBuff[j] & ~mask->Attr | attr->Attr;
				AttrBuff2[j] = AttrBuff2[j] & ~mask->Attr2 | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { AttrBuffFG[j] = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { AttrBuffBG[j] = attr->Back; }
			}
			Ptr = NextLinePtr(Ptr);
		}
	}
	else { // DECRARA
		for (i=YStart; i<=YEnd; i++) {
			j = Ptr+XStart-1;
			if (XStart>0 && (AttrBuff[j] & AttrKanji)) {
				AttrBuff[j] ^= attr->Attr;
			}
			while (++j < Ptr+XStart+C) {
				AttrBuff[j] ^= attr->Attr;
			}
			if (XStart+C<NumOfColumns && (AttrBuff[j-1] & AttrKanji)) {
				AttrBuff[j] ^= attr->Attr;
			}
			Ptr = NextLinePtr(Ptr);
		}
	}
	BuffUpdateRect(XStart, YStart, XEnd, YEnd);
}

void BuffChangeAttrStream(int XStart, int YStart, int XEnd, int YEnd, PCharAttr attr, PCharAttr mask)
{
	int i, j, endp;
	LONG Ptr;

	if (XEnd>NumOfColumns-1) {
		XEnd = NumOfColumns-1;
	}
	if (YEnd>NumOfLines-1-StatusLine) {
		YEnd = NumOfLines-1-StatusLine;
	}
	if (XStart>XEnd || YStart>YEnd) {
		return;
	}

	Ptr = GetLinePtr(PageStart+YStart);

	if (mask) { // DECCARA
		if (YStart == YEnd) {
			i = Ptr + XStart - 1;
			endp = Ptr + XEnd + 1;

			if (XStart > 0 && (AttrBuff[i] & AttrKanji)) {
				AttrBuff[i] = AttrBuff[i] & ~mask->Attr | attr->Attr;
				AttrBuff2[i] = AttrBuff2[i] & ~mask->Attr2 | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { AttrBuffFG[i] = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { AttrBuffBG[i] = attr->Back; }
			}
			while (++i < endp) {
				AttrBuff[i] = AttrBuff[i] & ~mask->Attr | attr->Attr;
				AttrBuff2[i] = AttrBuff2[i] & ~mask->Attr2 | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { AttrBuffFG[i] = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { AttrBuffBG[i] = attr->Back; }
			}
			if (XEnd < NumOfColumns-1 && (AttrBuff[i-1] & AttrKanji)) {
				AttrBuff[i] = AttrBuff[i] & ~mask->Attr | attr->Attr;
				AttrBuff2[i] = AttrBuff2[i] & ~mask->Attr2 | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { AttrBuffFG[i] = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { AttrBuffBG[i] = attr->Back; }
			}
		}
		else {
			i = Ptr + XStart - 1;
			endp = Ptr + NumOfColumns;

			if (XStart > 0 && (AttrBuff[i] & AttrKanji)) {
				AttrBuff[i] = AttrBuff[i] & ~mask->Attr | attr->Attr;
				AttrBuff2[i] = AttrBuff2[i] & ~mask->Attr2 | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { AttrBuffFG[i] = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { AttrBuffBG[i] = attr->Back; }
			}
			while (++i < endp) {
				AttrBuff[i] = AttrBuff[i] & ~mask->Attr | attr->Attr;
				AttrBuff2[i] = AttrBuff2[i] & ~mask->Attr2 | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { AttrBuffFG[i] = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { AttrBuffBG[i] = attr->Back; }
			}

			for (j=0; j < YEnd-YStart-1; j++) {
				Ptr = NextLinePtr(Ptr);
				i = Ptr;
				endp = Ptr + NumOfColumns;

				while (i < endp) {
					AttrBuff[i] = AttrBuff[i] & ~mask->Attr | attr->Attr;
					AttrBuff2[i] = AttrBuff2[i] & ~mask->Attr2 | attr->Attr2;
					if (mask->Attr2 & Attr2Fore) { AttrBuffFG[i] = attr->Fore; }
					if (mask->Attr2 & Attr2Back) { AttrBuffBG[i] = attr->Back; }
					i++;
				}
			}

			Ptr = NextLinePtr(Ptr);
			i = Ptr;
			endp = Ptr + XEnd + 1;

			while (i < endp) {
				AttrBuff[i] = AttrBuff[i] & ~mask->Attr | attr->Attr;
				AttrBuff2[i] = AttrBuff2[i] & ~mask->Attr2 | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { AttrBuffFG[i] = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { AttrBuffBG[i] = attr->Back; }
				i++;
			}
			if (XEnd < NumOfColumns-1 && (AttrBuff[i-1] & AttrKanji)) {
				AttrBuff[i] = AttrBuff[i] & ~mask->Attr | attr->Attr;
				AttrBuff2[i] = AttrBuff2[i] & ~mask->Attr2 | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { AttrBuffFG[i] = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { AttrBuffBG[i] = attr->Back; }
			}
		}
	}
	else { // DECRARA
		if (YStart == YEnd) {
			i = Ptr + XStart - 1;
			endp = Ptr + XEnd + 1;

			if (XStart > 0 && (AttrBuff[i] & AttrKanji)) {
				AttrBuff[i] ^= attr->Attr;
			}
			while (++i < endp) {
				AttrBuff[i] ^= attr->Attr;
			}
			if (XEnd < NumOfColumns-1 && (AttrBuff[i-1] & AttrKanji)) {
				AttrBuff[i] ^= attr->Attr;
			}
		}
		else {
			i = Ptr + XStart - 1;
			endp = Ptr + NumOfColumns;

			if (XStart > 0 && (AttrBuff[i] & AttrKanji)) {
				AttrBuff[i] ^= attr->Attr;
			}
			while (++i < endp) {
				AttrBuff[i] ^= attr->Attr;
			}

			for (j=0; j < YEnd-YStart-1; j++) {
				Ptr = NextLinePtr(Ptr);
				i = Ptr;
				endp = Ptr + NumOfColumns;

				while (i < endp) {
					AttrBuff[i] ^= attr->Attr;
					i++;
				}
			}

			Ptr = NextLinePtr(Ptr);
			i = Ptr;
			endp = Ptr + XEnd + 1;

			while (i < endp) {
				AttrBuff[i] ^= attr->Attr;
				i++;
			}
			if (XEnd < NumOfColumns-1 && (AttrBuff[i-1] & AttrKanji)) {
				AttrBuff[i] ^= attr->Attr;
			}
			Ptr = NextLinePtr(Ptr);
		}
	}
	BuffUpdateRect(0, YStart, NumOfColumns-1, YEnd);
}

static int LeftHalfOfDBCS(LONG Line, int CharPtr)
// If CharPtr is on the right half of a DBCS character,
// return pointer to the left half
//   Line: points to a line in CodeBuff
//   CharPtr: points to a char
//   return: points to the left half of the DBCS
{
	if ((CharPtr>0) &&
		((AttrBuff[Line+CharPtr-1] & AttrKanji) != 0)) {
		CharPtr--;
	}
	return CharPtr;
}

static int MoveCharPtr(LONG Line, int *x, int dx)
// move character pointer x by dx character unit
//   in the line specified by Line
//   Line: points to a line in CodeBuff
//   x: points to a character in the line
//   dx: moving distance in character unit (-: left, +: right)
//      One DBCS character is counted as one character.
//      The pointer stops at the beginning or the end of line.
// Output
//   x: new pointer. x points to a SBCS character or
//      the left half of a DBCS character.
//   return: actual moving distance in character unit
{
	int i;

	*x = LeftHalfOfDBCS(Line,*x);
	i = 0;
	while (dx!=0) {
		if (dx>0) { // move right
			if ((AttrBuff[Line+*x] & AttrKanji) != 0) {
				if (*x<NumOfColumns-2) {
					i++;
					*x = *x + 2;
				}
			}
			else if (*x<NumOfColumns-1) {
				i++;
				(*x)++;
			}
			dx--;
		}
		else { // move left
			if (*x>0) {
				i--;
				(*x)--;
			}
			*x = LeftHalfOfDBCS(Line,*x);
			dx++;
		}
	}
	return i;
}

/**
 *	(クリップボード用に)文字列を取得
 *	@param[in]	sx,sy,ex,ey	選択領域
 *	@param[in]	box_select	TRUE=箱型(矩形)選択
 *							FALSE=行選択
 *	@param[out] _str_len	文字列長(文字端L'\0'を含む)
 *				NULLのときは返さない
 *	@return		文字列
 *				使用後は free() すること
 */
#if UNICODE_INTERNAL_BUFF
static wchar_t *BuffGetStringForCB(int sx, int sy, int ex, int ey, BOOL box_select, size_t *_str_len)
{
	wchar_t *str_w;
	size_t str_size;	// 確保したサイズ
	size_t k;
	LONG TmpPtr;
	int IStart, IEnd;
	int x, y;
	BOOL LineContinued;

	str_size = NumOfColumns * (ey - sy + 1);
	str_w = malloc(sizeof(wchar_t) * str_size);

	LockBuffer();

	str_w[0] = 0;
	TmpPtr = GetLinePtr(sy);
	k = 0;
	for (y = sy; y<=ey ; y++) {
		if (box_select) {
			IStart = SelectStart.x;
			IEnd = SelectEnd.x-1;
		}
		else {
			IStart = 0;
			IEnd = NumOfColumns-1;
			if (y== sy) {
				IStart = sx;
			}
			if (y== ey) {
				IEnd = ex -1;
			}
		}

		// 次の行に続いてる?
		LineContinued = FALSE;
		if (!box_select) {
			// 行選択の場合のみ
			if (ts.EnableContinuedLineCopy && y!= ey ) {
				LONG NextTmpPtr = NextLinePtr(TmpPtr);
				if ((AttrBuff[NextTmpPtr] & AttrLineContinued) != 0) {
					LineContinued = TRUE;
				}
			}
		}

		if (!LineContinued) {
			while ((IEnd>0)) {
				// コピー不要分を削除
				const buff_char_t *b = &CodeBuffW[TmpPtr + IEnd];
				if (b->u32 == 0x20) {
					MoveCharPtr(TmpPtr,&IEnd,-1);	// 切り詰める
				}
				else {
					break;
				}
			}
		}

		x = IStart;
		while (x <= IEnd) {
			const buff_char_t *b = &CodeBuffW[TmpPtr + x];
			if (b->u32 != 0) {
				str_w[k++] = b->wc2[0];
				if (b->wc2[1] != 0) {
					str_w[k++] = b->wc2[1];
				}
				if (k + 2 >= str_size) {
					str_size *= 2;
					str_w = realloc(str_w, sizeof(wchar_t) * str_size);
				}
				{
					int i;
					// コンビネーション
					if (k + b->CombinationCharCount16 >= str_size) {
						str_size += + b->CombinationCharCount16;
						str_w = realloc(str_w, sizeof(wchar_t) * str_size);
					}
					for (i = 0 ; i < (int)b->CombinationCharCount16; i++) {
						str_w[k++] = b->pCombinationChars16[i];
					}
				}
			}
			x++;
		}

		if (y < ey) {
			// 改行を加える(最後の行以外の場合)
			if (!LineContinued) {
				str_w[k++] = 0x0d;
				str_w[k++] = 0x0a;
			}
		}

		TmpPtr = NextLinePtr(TmpPtr);
	}
	str_w[k++] = 0;

	UnlockBuffer();

	if (_str_len != NULL) {
		*_str_len = k;
	}
	return str_w;
}
#endif

/**
 *	(x,y) の1文字が strと同一か調べる
 *		*注 1文字が複数のwchar_tから構成されている
 *
 *	@param		y		PageStart + CursorY
 *	@param		str		1文字(wchar_t列)
 *	@param		len		文字列長
 *	@retval		0=マッチしなかった
 *				マッチした文字列長
 */
#if UNICODE_INTERNAL_BUFF
static size_t MatchOneString(int x, int y, const wchar_t *str, size_t len)
{
	int match_pos = 0;
	int TmpPtr = GetLinePtr(y);
	const buff_char_t *b = &CodeBuffW[TmpPtr + x];
	if (b->wc2[1] == 0) {
		// サロゲートペアではない
		if (str[match_pos] != b->wc2[0]) {
			return 0;
		}
		match_pos++;
		len--;
	} else {
		// サロゲートペア
		if (len < 2) {
			return 0;
		}
		if (str[match_pos+0] != b->wc2[0] ||
			str[match_pos+1] != b->wc2[1]) {
			return 0;
		}
		match_pos+=2;
		len-=2;
	}
	if (b->CombinationCharCount16 > 0) {
		// コンビネーション
		int i;
		if (len < b->CombinationCharCount16) {
			return 0;
		}
		for (i = 0 ; i < (int)b->CombinationCharCount16; i++) {
			if (str[match_pos++] != b->pCombinationChars16[i]) {
				return 0;
			}
		}
		len -= b->CombinationCharCount16;
	}
	return match_pos;
}
#endif

/**
 *	(x,y)から strと同一か調べる
 *
 *	@param		x		マイナスの時、上の行が対象になる
 *	@param		y		PageStart + CursorY
 *	@param		LineCntinued	TRUE=行の継続を考慮する
 *	@retval		TRUE	マッチした
 *	@retval		FALSE	マッチしていない
 */
#if UNICODE_INTERNAL_BUFF
static BOOL MatchString(int x, int y, const wchar_t *str, BOOL LineContinued)
{
	BOOL result;
	int TmpPtr = GetLinePtr(y);
	size_t len = wcslen(str);
	if (len == 0) {
		return FALSE;
	}
	while(x < 0) {
		if (LineContinued && (AttrBuff[TmpPtr+0] & AttrLineContinued) == 0) {
			// 行が継続しているか考慮 & 継続していない
			x = 0;	// 行頭からとする
			break;
		}
		TmpPtr = PrevLinePtr(TmpPtr);
		x += NumOfColumns;
		y--;
	}
	while(x > NumOfColumns) {
		if (LineContinued && (AttrBuff[TmpPtr+NumOfColumns-1] & AttrLineContinued) == 0) {
			// 行が継続しているか考慮 & 継続していない
			x = 0;	// 行頭からとする
			break;
		}
		TmpPtr = NextLinePtr(TmpPtr);
		x -= NumOfColumns;
	}

	for(;;) {
		// 1文字同一か調べる
		size_t match_len = MatchOneString(x, y, str, len);
		if (match_len == 0) {
			result = FALSE;
			break;
		}
		len -= match_len;
		if (len == 0) {
			// 全文字調べ終わった
			result = TRUE;
			break;
		}
		str += match_len;

		// 次の文字
		x++;
		if (x == NumOfColumns) {
			if (LineContinued && (AttrBuff[TmpPtr+NumOfColumns-1] & AttrLineContinued) != 0) {
				// 次の行へ
				x = 0;
				TmpPtr = NextLinePtr(TmpPtr);
				y++;
			} else {
				// 行末
				result = FALSE;
				break;
			}
		}
	}

	return result;
}
#endif

/**
 *	(sx,sy)から(ex,ey)までで str にマッチする文字を探して
 *	位置を返す
 *
 *	@param		sy,ex	PageStart + CursorY
 *	@param[out]	x		マッチした位置
 *	@param[out]	y		マッチした位置
 *	@retval		TRUE	マッチした
 */
#if UNICODE_INTERNAL_BUFF
static BOOL BuffGetMatchPosFromString(
	int sx, int sy, int ex, int ey, const wchar_t *str,
	int *match_x, int *match_y)
{
	int IStart, IEnd;
	int x, y;

	for (y = sy; y<=ey ; y++) {
		IStart = 0;
		IEnd = NumOfColumns-1;
		if (y== sy) {
			IStart = sx;
		}
		if (y== ey) {
			IEnd = ex;
		}

		x = IStart;
		while (x <= IEnd) {
			if (MatchString(x, y, str, TRUE)) {
				// マッチした
				if (match_x != NULL) {
					*match_x = x;
				}
				if (match_y != NULL) {
					*match_y = y;
				}
				return TRUE;
			}
			x++;
		}
	}
	return FALSE;
}
#endif


/**
 *	連続したスペースをタブ1つに置換する
 *	@param[out] _str_len	文字列長(L'\0'を含む)
 *	@return		文字列
 *				使用後は free() すること
 */
static wchar_t *ConvertTable(const wchar_t *src, size_t src_len, size_t *str_len)
{
	wchar_t *dest_top = malloc(sizeof(wchar_t) * src_len);
	wchar_t *dest = dest_top;
	BOOL WhiteSpace = FALSE;
	while (*src != '\0') {
		wchar_t c = *src++;
		if (c == 0x0d || c == 0x0a) {
			*dest++ = c;
			WhiteSpace = FALSE;
		} else if (c <= L' ') {
			if (!WhiteSpace) {
				// insert tab
				*dest++ = 0x09;
				WhiteSpace = TRUE;
			}
		} else {
			*dest++ = c;
			WhiteSpace = FALSE;
		}
	}
	*dest = L'\0';
	*str_len = dest - dest_top + 1;
	return dest_top;
}


#if UNICODE_INTERNAL_BUFF
void BuffCBCopyUnicode(BOOL Table)
{
	wchar_t *str_ptr;
	size_t str_len;
	str_ptr = BuffGetStringForCB(
		SelectStart.x, SelectStart.y,
		SelectEnd.x, SelectEnd.y, BoxSelect,
		&str_len);

	// テーブル形式へ変換
	if (Table) {
		size_t table_len;
		wchar_t *table_ptr = ConvertTable(str_ptr, str_len, &table_len);
		free(str_ptr);
		str_ptr = table_ptr;
		str_len = table_len;
	}
	OutputDebugPrintfW(L"BuffCBCopyUnicode()\n"
					   L"%d, '%s'\n", str_len, str_ptr);

	CBSetTextW(HVTWin, str_ptr, 0);
	free(str_ptr);
}
#endif

#if !UNICODE_INTERNAL_BUFF
void BuffCBCopy(BOOL Table)
// copy selected text to clipboard
{
	LONG MemSize;
	PCHAR CBPtr;
	LONG TmpPtr;
	int i, j, k, IStart, IEnd;
	BOOL Sp, FirstChar;
	BYTE b;
	BOOL LineContinued, PrevLineContinued;

	LineContinued = FALSE;

	if (TalkStatus==IdTalkCB) {
		return;
	}
	if (! Selected) {
		return;
	}

// --- open clipboard and get CB memory
	if (BoxSelect) {
		MemSize = (SelectEnd.x-SelectStart.x+3)*
		          (SelectEnd.y-SelectStart.y+1) + 1;
	}
	else {
		MemSize = (SelectEnd.y-SelectStart.y)*
		          (NumOfColumns+2) +
		          SelectEnd.x - SelectStart.x + 1;
	}
	CBPtr = CBOpen(MemSize);
	if (CBPtr==NULL) {
		return;
	}

// --- copy selected text to CB memory
	LockBuffer();

	CBPtr[0] = 0;
	TmpPtr = GetLinePtr(SelectStart.y);
	k = 0;
	for (j = SelectStart.y ; j<=SelectEnd.y ; j++) {
		if (BoxSelect) {
			IStart = SelectStart.x;
			IEnd = SelectEnd.x-1;
		}
		else {
			IStart = 0;
			IEnd = NumOfColumns-1;
			if (j==SelectStart.y) {
				IStart = SelectStart.x;
			}
			if (j==SelectEnd.y) {
				IEnd = SelectEnd.x-1;
			}
		}
		i = LeftHalfOfDBCS(TmpPtr,IStart);
		if (i!=IStart) {
			if (j==SelectStart.y) {
				IStart = i;
			}
			else {
				IStart = i + 2;
			}
		}

		// exclude right-side space characters
		IEnd = LeftHalfOfDBCS(TmpPtr,IEnd);
		PrevLineContinued = LineContinued;
		LineContinued = FALSE;
		if (ts.EnableContinuedLineCopy && j!=SelectEnd.y && !BoxSelect) {
			LONG NextTmpPtr = NextLinePtr(TmpPtr);
			if ((AttrBuff[NextTmpPtr] & AttrLineContinued) != 0) {
				LineContinued = TRUE;
			}
			if (IEnd == NumOfColumns-1 &&
				(AttrBuff[TmpPtr + IEnd] & AttrLineContinued) != 0) {
				MoveCharPtr(TmpPtr,&IEnd,-1);
			}
		}
		if (!LineContinued)
			while ((IEnd>0) && (CodeBuff[TmpPtr+IEnd]==0x20)) {
				MoveCharPtr(TmpPtr,&IEnd,-1);
			}
		if ((IEnd==0) && (CodeBuff[TmpPtr]==0x20)) {
			IEnd = -1;
		}
		else if ((AttrBuff[TmpPtr+IEnd] & AttrKanji) != 0) { /* DBCS first byte? */
			IEnd++;
		}

		Sp = FALSE;
		FirstChar = TRUE;
		i = IStart;
		while (i <= IEnd) {
			b = CodeBuff[TmpPtr+i];
			i++;
			if (! Sp) {
				if ((Table) && (b<=0x20)) {
					Sp = TRUE;
					b = 0x09;
				}
				if ((b!=0x09) || (! FirstChar) || PrevLineContinued) {
					FirstChar = FALSE;
					CBPtr[k] = b;
					k++;
				}
			}
			else {
				if (b>0x20) {
					Sp = FALSE;
					FirstChar = FALSE;
					CBPtr[k] = b;
					k++;
				}
			}
		}

		if (!LineContinued)
			if (j < SelectEnd.y) {
				CBPtr[k] = 0x0d;
				k++;
				CBPtr[k] = 0x0a;
				k++;
			}

		TmpPtr = NextLinePtr(TmpPtr);
	}
	CBPtr[k] = 0;
	LineContinued = FALSE;
	if (ts.EnableContinuedLineCopy && j!=SelectEnd.y && !BoxSelect && j<BuffEnd-1) {
		LONG NextTmpPtr = NextLinePtr(TmpPtr);
		if ((AttrBuff[NextTmpPtr] & AttrLineContinued) != 0) {
			LineContinued = TRUE;
		}
		if (IEnd == NumOfColumns-1 &&
			(AttrBuff[TmpPtr + IEnd] & AttrLineContinued) != 0) {
			MoveCharPtr(TmpPtr,&IEnd,-1);
		}
	}
	if (!LineContinued)
		UnlockBuffer();

// --- send CB memory to clipboard
	CBClose();
	return;
}
#endif

void BuffPrint(BOOL ScrollRegion)
// Print screen or selected text
{
	int Id;
	POINT PrintStart, PrintEnd;
	TCharAttr CurAttr, TempAttr;
	int i, j, count;
	int IStart, IEnd;
	LONG TmpPtr;

	TempAttr = DefCharAttr;

	if (ScrollRegion) {
		Id = VTPrintInit(IdPrnScrollRegion);
	}
	else if (Selected) {
		Id = VTPrintInit(IdPrnScreen | IdPrnSelectedText);
	}
	else {
		Id = VTPrintInit(IdPrnScreen);
	}
	if (Id==IdPrnCancel) {
		return;
	}

	/* set print region */
	if (Id==IdPrnSelectedText) {
		/* print selected region */
		PrintStart = SelectStart;
		PrintEnd = SelectEnd;
	}
	else if (Id==IdPrnScrollRegion) {
		/* print scroll region */
		PrintStart.x = 0;
		PrintStart.y = PageStart + CursorTop;
		PrintEnd.x = NumOfColumns;
		PrintEnd.y = PageStart + CursorBottom;
	}
	else {
		/* print current screen */
		PrintStart.x = 0;
		PrintStart.y = PageStart;
		PrintEnd.x = NumOfColumns;
		PrintEnd.y = PageStart + NumOfLines - 1;
	}
	if (PrintEnd.y > BuffEnd-1) {
		PrintEnd.y = BuffEnd-1;
	}

	LockBuffer();

	TmpPtr = GetLinePtr(PrintStart.y);
	for (j = PrintStart.y ; j <= PrintEnd.y ; j++) {
		if (j==PrintStart.y) {
			IStart = PrintStart.x;
		}
		else {
			IStart = 0;
		}
		if (j == PrintEnd.y) {
			IEnd = PrintEnd.x - 1;
		}
		else {
			IEnd = NumOfColumns - 1;
		}

		while ((IEnd>=IStart) &&
		       (CodeBuff[TmpPtr+IEnd]==0x20) &&
		       (AttrBuff[TmpPtr+IEnd]==AttrDefault) &&
		       (AttrBuff2[TmpPtr+IEnd]==AttrDefault)) {
			IEnd--;
		}

		i = IStart;
		while (i <= IEnd) {
			CurAttr.Attr = AttrBuff[TmpPtr+i] & ~ AttrKanji;
			CurAttr.Attr2 = AttrBuff2[TmpPtr+i];
			CurAttr.Fore = AttrBuffFG[TmpPtr+i];
			CurAttr.Back = AttrBuffBG[TmpPtr+i];

			count = 1;
			while ((i+count <= IEnd) &&
			       (CurAttr.Attr == (AttrBuff[TmpPtr+i+count] & ~ AttrKanji)) &&
			       (CurAttr.Attr2 == AttrBuff2[TmpPtr+i+count]) &&
			       (CurAttr.Fore == AttrBuffFG[TmpPtr+i+count]) &&
			       (CurAttr.Back == AttrBuffBG[TmpPtr+i+count]) ||
			       (i+count<NumOfColumns) &&
			       ((AttrBuff[TmpPtr+i+count-1] & AttrKanji) != 0)) {
				count++;
			}

			if (TCharAttrCmp(CurAttr, TempAttr) != 0) {
				PrnSetAttr(CurAttr);
				TempAttr = CurAttr;
			}
			PrnOutText(&(CodeBuff[TmpPtr+i]),count);
			i = i+count;
		}
		PrnNewLine();
		TmpPtr = NextLinePtr(TmpPtr);
	}

	UnlockBuffer();
	VTPrintEnd();
}

void BuffDumpCurrentLine(BYTE TERM)
// Dumps current line to the file (for path through printing)
//   HFile: file handle
//   TERM: terminator character
//	= LF or VT or FF
{
	int i, j;

	i = NumOfColumns;
	while ((i>0) && (CodeLine[i-1]==0x20)) {
		i--;
	}
	for (j=0; j<i; j++) {
		WriteToPrnFile(CodeLine[j],FALSE);
	}
	WriteToPrnFile(0,TRUE);
	if ((TERM>=LF) && (TERM<=FF)) {
		WriteToPrnFile(0x0d,FALSE);
		WriteToPrnFile(TERM,TRUE);
	}
}

static BOOL isURLchar(unsigned int u32)
{
	// RFC3986(Uniform Resource Identifier (URI): Generic Syntax)に準拠する
	// by sakura editor 1.5.2.1: etc_uty.cpp
	static const char	url_char[] = {
	  /* +0  +1  +2  +3  +4  +5  +6  +7  +8  +9  +A  +B  +C  +D  +E  +F */
	      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	/* +00: */
	      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	/* +10: */
	      0, -1,  0, -1, -1, -1, -1,  0,  0,  0,  0, -1, -1, -1, -1, -1,	/* +20: " !"#$%&'()*+,-./" */
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1,  0, -1,	/* +30: "0123456789:;<=>?" */
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* +40: "@ABCDEFGHIJKLMNO" */
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1,  0,  0, -1,	/* +50: "PQRSTUVWXYZ[\]^_" */
	      0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* +60: "`abcdefghijklmno" */
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0, -1,  0,	/* +70: "pqrstuvwxyz{|}~ " */
	    /* 0    : not url char
	     * -1   : url char
	     * other: url head char --> url_table array number + 1
	     */
	};

	if (u32 >= 0x80) {
		return FALSE;
	}
	return url_char[u32] == 0 ? FALSE : TRUE;
}

static void markURL(int x)
{
	LONG PrevCharPtr;
	CHAR PrevCharAttr, PrevCharCode;

	// RFC3986(Uniform Resource Identifier (URI): Generic Syntax)に準拠する
	// by sakura editor 1.5.2.1: etc_uty.cpp
	static const char	url_char[] = {
	  /* +0  +1  +2  +3  +4  +5  +6  +7  +8  +9  +A  +B  +C  +D  +E  +F */
	      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	/* +00: */
	      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	/* +10: */
	      0, -1,  0, -1, -1, -1, -1,  0,  0,  0,  0, -1, -1, -1, -1, -1,	/* +20: " !"#$%&'()*+,-./" */
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1,  0, -1,	/* +30: "0123456789:;<=>?" */
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* +40: "@ABCDEFGHIJKLMNO" */
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1,  0,  0, -1,	/* +50: "PQRSTUVWXYZ[\]^_" */
	      0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* +60: "`abcdefghijklmno" */
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0, -1,  0,	/* +70: "pqrstuvwxyz{|}~ " */
	    /* 0    : not url char
	     * -1   : url char
	     * other: url head char --> url_table array number + 1
	     */
	};
	static char *prefix[] = {
		"https://",
		"http://",
		"sftp://",
		"tftp://",
		"news://",
		"ftp://",
		"mms://",
		NULL
	};
	unsigned char ch = CodeLine[x];

	if (ts.EnableClickableUrl == FALSE &&
		(ts.ColorFlag & CF_URLCOLOR) == 0)
		return;

	// 直前の行から連結しているか。
	if (x == 0) {
		PrevCharPtr = PrevLinePtr(LinePtr) + NumOfColumns-1;
		PrevCharCode = CodeBuff[PrevCharPtr];
		PrevCharAttr = AttrBuff[PrevCharPtr];
		if ((PrevCharAttr & AttrURL) && !(AttrLine[0]&(AttrKanji|AttrSpecial)) && !(ch & 0x80) && url_char[ch]) {
			if ((AttrLine[0] & AttrLineContinued) || (ts.JoinSplitURL &&
			    (PrevCharCode == ts.JoinSplitURLIgnoreEOLChar || ts.JoinSplitURLIgnoreEOLChar == '\0' ))) {
				AttrLine[0] |= AttrURL;
			}
		}
		return;
	}

	if ((x-1>=0) && (AttrLine[x-1] & AttrURL) &&
	  !(AttrLine[x] & (AttrKanji|AttrSpecial)) &&
	  ((!(ch & 0x80) && url_char[ch]) || (x == NumOfColumns - 1 && ch == ts.JoinSplitURLIgnoreEOLChar))) {
		AttrLine[x] |= AttrURL;
		return;
	}

	if ((x-2>=0) && !strncmp(&CodeLine[x-2], "://", 3)) {
		int i;
		RECT rc;
		int CaretX, CaretY;
		char **p = prefix;

		while (*p) {
			size_t len = strlen(*p) - 1;
			if ((x-len>=0) && !strncmp(&CodeLine[x-len], *p, len)) {
				for (i = 0; i <= len; i++) {
					AttrLine[x-i] |= AttrURL;
				}
				break;
			}
			p++;
		}

		/* ハイパーリンクの色属性変更は、すでに画面へ出力後に、バッファを遡って URL 属性を
		 * 付け直すというロジックであるため、色が正しく描画されない場合がある。
		 * 少々強引だが、ハイパーリンクを発見したタイミングで、その行に再描画指示を出すことで、
		 * リアルタイムな色描画を実現する。
		 * (2009.8.26 yutaka)
		 */
		CaretX = (0-WinOrgX)*FontWidth;
		CaretY = (CursorY-WinOrgY)*FontHeight;
		rc.left = CaretX;
		rc.top = CaretY;
		rc.right = CaretX + NumOfColumns * FontWidth;
		rc.bottom = CaretY + FontHeight;
		InvalidateRect(HVTWin, &rc, FALSE);
	}
}

#if !UNICODE_INTERNAL_BUFF
void BuffPutChar(BYTE b, TCharAttr Attr, BOOL Insert)
// Put a character in the buffer at the current position
//   b: character
//   Attr: attributes
//   Insert: Insert flag
{
	int XStart, LineEnd, MoveLen;
	int extr = 0;

	if (ts.EnableContinuedLineCopy && CursorX == 0 && (AttrLine[0] & AttrLineContinued)) {
		Attr.Attr |= AttrLineContinued;
	}

	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		EraseKanji(1); /* if cursor is on right half of a kanji, erase the kanji */
		if (! Insert) {
			EraseKanji(0); /* if cursor on left half... */
		}
	}

	if (Insert) {
		if (CursorX > CursorRightM)
			LineEnd = NumOfColumns - 1;
		else
			LineEnd = CursorRightM;

		if (LineEnd < NumOfColumns - 1 && (AttrLine[LineEnd] & AttrKanji)) {
			CodeLine[LineEnd] = 0x20;
			AttrLine[LineEnd] &= ~AttrKanji;
			CodeLine[LineEnd+1] = 0x20;
			AttrLine[LineEnd+1] &= ~AttrKanji;
			extr = 1;
		}

		MoveLen = LineEnd - CursorX;
		if (MoveLen > 0) {
			memmove(&CodeLine[CursorX+1], &CodeLine[CursorX], MoveLen);
			memmove(&AttrLine[CursorX+1], &AttrLine[CursorX], MoveLen);
			memmove(&AttrLine2[CursorX+1], &AttrLine2[CursorX], MoveLen);
			memmove(&AttrLineFG[CursorX+1], &AttrLineFG[CursorX], MoveLen);
			memmove(&AttrLineBG[CursorX+1], &AttrLineBG[CursorX], MoveLen);
		}
		CodeLine[CursorX] = b;
		AttrLine[CursorX] = Attr.Attr;
		AttrLine2[CursorX] = Attr.Attr2;
		AttrLineFG[CursorX] = Attr.Fore;
		AttrLineBG[CursorX] = Attr.Back;
		/* last char in current line is kanji first? */
		if ((AttrLine[LineEnd] & AttrKanji) != 0) {
			/* then delete it */
			CodeLine[LineEnd] = 0x20;
			AttrLine[LineEnd] = CurCharAttr.Attr;
			AttrLine2[LineEnd] = CurCharAttr.Attr2;
			AttrLineFG[LineEnd] = CurCharAttr.Fore;
			AttrLineBG[LineEnd] = CurCharAttr.Back;
		}
		/* begin - ishizaki */
		markURL(CursorX+1);
		markURL(CursorX);
		/* end - ishizaki */

		if (StrChangeCount==0) {
			XStart = CursorX;
		}
		else {
			XStart = StrChangeStart;
		}
		StrChangeCount = 0;
		BuffUpdateRect(XStart, CursorY, LineEnd+extr, CursorY);
	}
	else {
		CodeLine[CursorX] = b;
		AttrLine[CursorX] = Attr.Attr;
		AttrLine2[CursorX] = Attr.Attr2;
		AttrLineFG[CursorX] = Attr.Fore;
		AttrLineBG[CursorX] = Attr.Back;
		/* begin - ishizaki */
		markURL(CursorX);
		/* end - ishizaki */

		if (StrChangeCount==0) {
			StrChangeStart = CursorX;
		}
		StrChangeCount++;
	}
}
#endif

void BuffPutKanji(WORD w, TCharAttr Attr, BOOL Insert)
// Put a kanji character in the buffer at the current position
//   b: character
//   Attr: attributes
//   Insert: Insert flag
{
	int XStart, LineEnd, MoveLen;
	int extr = 0;

	if (ts.EnableContinuedLineCopy && CursorX == 0 && (AttrLine[0] & AttrLineContinued)) {
		Attr.Attr |= AttrLineContinued;
	}

	EraseKanji(1); /* if cursor is on right half of a kanji, erase the kanji */

	if (Insert) {
		if (CursorX > CursorRightM)
			LineEnd = NumOfColumns - 1;
		else
			LineEnd = CursorRightM;

		if (LineEnd < NumOfColumns - 1 && (AttrLine[LineEnd] & AttrKanji)) {
			CodeLine[LineEnd] = 0x20;
			AttrLine[LineEnd] &= ~AttrKanji;
			CodeLine[LineEnd+1] = 0x20;
			AttrLine[LineEnd+1] &= ~AttrKanji;
			extr = 1;
		}

		MoveLen = LineEnd - CursorX - 1;
		if (MoveLen > 0) {
			memmove(&CodeLine[CursorX+2], &CodeLine[CursorX], MoveLen);
			memmove(&AttrLine[CursorX+2], &AttrLine[CursorX], MoveLen);
			memmove(&AttrLine2[CursorX+2], &AttrLine2[CursorX], MoveLen);
			memmove(&AttrLineFG[CursorX+2], &AttrLineFG[CursorX], MoveLen);
			memmove(&AttrLineBG[CursorX+2], &AttrLineBG[CursorX], MoveLen);
		}

		CodeLine[CursorX] = HIBYTE(w);
		AttrLine[CursorX] = Attr.Attr | AttrKanji; /* DBCS first byte */
		AttrLine2[CursorX] = Attr.Attr2;
		AttrLineFG[CursorX] = Attr.Fore;
		AttrLineBG[CursorX] = Attr.Back;
		if (CursorX < LineEnd) {
			CodeLine[CursorX+1] = LOBYTE(w);
			AttrLine[CursorX+1] = Attr.Attr;
			AttrLine2[CursorX+1] = Attr.Attr2;
			AttrLineFG[CursorX+1] = Attr.Fore;
			AttrLineBG[CursorX+1] = Attr.Back;
		}
		/* begin - ishizaki */
		markURL(CursorX);
		markURL(CursorX+1);
		/* end - ishizaki */

		/* last char in current line is kanji first? */
		if ((AttrLine[LineEnd] & AttrKanji) != 0) {
			/* then delete it */
			CodeLine[LineEnd] = 0x20;
			AttrLine[LineEnd] = CurCharAttr.Attr;
			AttrLine2[LineEnd] = CurCharAttr.Attr2;
			AttrLineFG[LineEnd] = CurCharAttr.Fore;
			AttrLineBG[LineEnd] = CurCharAttr.Back;
		}

		if (StrChangeCount==0) {
			XStart = CursorX;
		}
		else {
			XStart = StrChangeStart;
		}
		StrChangeCount = 0;
		BuffUpdateRect(XStart, CursorY, LineEnd+extr, CursorY);
	}
	else {
		CodeLine[CursorX] = HIBYTE(w);
		AttrLine[CursorX] = Attr.Attr | AttrKanji; /* DBCS first byte */
		AttrLine2[CursorX] = Attr.Attr2;
		AttrLineFG[CursorX] = Attr.Fore;
		AttrLineBG[CursorX] = Attr.Back;
		if (CursorX < NumOfColumns-1) {
			CodeLine[CursorX+1] = LOBYTE(w);
			AttrLine[CursorX+1] = Attr.Attr;
			AttrLine2[CursorX+1] = Attr.Attr2;
			AttrLineFG[CursorX+1] = Attr.Fore;
			AttrLineBG[CursorX+1] = Attr.Back;
		}
		/* begin - ishizaki */
		markURL(CursorX);
		markURL(CursorX+1);
		/* end - ishizaki */

		if (StrChangeCount==0) {
			StrChangeStart = CursorX;
		}
		StrChangeCount = StrChangeCount + 2;
	}
}

#if UNICODE_INTERNAL_BUFF
static BOOL BuffIsHalfWidthFromPropery(char width_property)
{
	switch (width_property) {
	case 'H':	// Halfwidth
	case 'n':	// Narrow
	case 'N':	// Neutral
	default:
		return TRUE;
	case 'A':	// Ambiguous 曖昧
		if (ts.Language == IdJapanese) {
			// 全角として扱う
			return FALSE;
		}
		return TRUE;
	case 'W':
	case 'F':
		return FALSE;		// 全角
	}
}

static BOOL BuffIsHalfWidthFromCode(TTTSet *ts_, unsigned int u32, char *width_property, char *emoji)
{
	*width_property = UnicodeGetWidthProperty(u32);
	*emoji = (char)UnicodeIsEmoji(u32);
	if (*emoji) {
		if (ts_->Language == IdJapanese) {
			// 全角
			return FALSE;
		} else {
			if (u32 >= 0x1f000) {
				return FALSE;
			}
			return TRUE;
		}
	}
	return BuffIsHalfWidthFromPropery(*width_property);
}

#endif

#if UNICODE_INTERNAL_BUFF
/**
 *	カーソル位置とのURLアトリビュートの先頭との距離を計算する
 */
int get_url_len(int cur_x, int cur_y)
{
	int sp = cur_x + cur_y * NumOfColumns;
	int cp;
	int dp;
	{
		int p = sp;
		p--;
		while (p > 0) {
			int sy = p / NumOfColumns;
			int sx = p % NumOfColumns;
			int ptr = GetLinePtr(PageStart + sy) + sx;
			if ((AttrBuff[ptr] & AttrURL) == 0) {
				break;
			}
			p--;
		}
		sp = p;
	}
	cp = cur_x + cur_y * NumOfColumns;
	dp = cp - sp;
	return dp;
}

static const struct schemes_t {
	const wchar_t *str;
	int len;
} schemes[] = {
	// clang-format off
	{L"https://", 8},
	{L"http://", 7},
	{L"sftp://", 7},
	{L"tftp://", 7},
	{L"news://", 7},
	{L"ftp://", 6},
	{L"mms://", 6},
	// clang-format on
};

static BOOL mark_url_w_sub(int sx_s, int sx_e, int sy_s, int sy_e, int *sx_match_s, int *sx_match_e, int *sy_match_s,
						   int *sy_match_e)
{
	int i;
	int match_x, match_y;
	int x;
	int y;
	int rx;
	LONG TmpPtr;

	if (sx_s >= sx_e && sy_s >= sy_e) {
		return FALSE;
	}

	for (i = 0; i < _countof(schemes); i++) {
		const wchar_t *prefix = schemes[i].str;
		// マッチするか?
		if (BuffGetMatchPosFromString(sx_s, PageStart + sy_s, sx_s, PageStart + sy_s, prefix, &match_x, &match_y)) {
			// マッチした
			break;
		}
	}

	if (i == _countof(schemes)) {
		// マッチしなかった
		return FALSE;
	}

	// マッチした
	*sx_match_s = match_x;
	*sy_match_s = match_y - PageStart;
	rx = match_x;
	for (y = match_y; y <= PageStart + sy_e; y++) {
		int sx_s_i = 0;
		int sx_e_i = NumOfColumns - 1;  // とにかく行末まで
		if (y == PageStart + sy_s) {
			sx_s_i = match_x;
		}
		*sy_match_e = y - PageStart;
		TmpPtr = GetLinePtr(y);
		for (x = sx_s_i; x <= sx_e_i; x++) {
			const buff_char_t *b = &CodeBuffW[TmpPtr + x];
			if (!isURLchar(b->u32)) {
				*sx_match_e = rx;
				return TRUE;
			}
			rx = x;
			AttrBuff[TmpPtr + x] |= AttrURL;
		}
	}
	*sx_match_e = rx;
	return TRUE;
}

// cur_x	カーソル位置
// cur_y	カーソル位置(!バッファ位置)
static void mark_url_line_w(int cur_x, int cur_y)
{
	int sx;
	int sy;
	int ex;
	int ey;
	LONG TmpPtr;
	const buff_char_t *b;

	// 行頭を探す
	sx = 0;
	sy = cur_y;
	TmpPtr = GetLinePtr(PageStart + sy);
	while ((AttrBuff[TmpPtr] & AttrLineContinued) != 0) {
		if (sy == 0) {
			break;
		}
		sy--;
		sx = 0;
		TmpPtr = PrevLinePtr(TmpPtr);
	}
	// 行末を探す
	ex = NumOfColumns - 1;
	ey = cur_y;
	TmpPtr = GetLinePtr(PageStart + ey);
	while ((AttrBuff[TmpPtr + NumOfColumns - 1] & AttrLineContinued) != 0) {
		ey++;
		TmpPtr = NextLinePtr(TmpPtr);
	}
	b = &CodeBuffW[TmpPtr + ex];
	for (;;) {
		if (b->u32 != ' ') {
			break;
		}
		b--;
		ex--;
	}

	// URLアトリビュートを落とす
	{
		int x;
		int y;
		for (y = sy; y <= ey; y++) {
			int sx_i = 0;
			int ex_i = NumOfColumns - 1;
			if (y == sy) {
				sx_i = sx;
			}
			else if (y == ey) {
				ex_i = ex;
			}
			TmpPtr = GetLinePtr(PageStart + y);
			for (x = sx_i; x < ex_i; x++) {
				AttrBuff[TmpPtr + x] &= ~AttrURL;
			}
		}
	}

	// マークする
	{
	int sx_i = sx;
	int sy_i = sy;
	for (;;) {
		int sx_match_s, sx_match_e;
		int sy_match_s, sy_match_e;
		BOOL match;

		if (sx_i >= ex && sy_i >= ey) {
			break;
		}

		match = mark_url_w_sub(sx_i, ex, sy_i, ey, &sx_match_s, &sx_match_e, &sy_match_s, &sy_match_e);
		if (match) {
			if (sy_match_s == sy_match_e) {
				BuffDrawLineI(-1, -1, sy_match_s, sx_match_s, sx_match_e);
			}
			else {
				BuffDrawLineI(-1, -1, sy_match_s, sx_match_s, sx_match_e);
			}
			sx_i = sx_match_e;
			sy_i = sy_match_e;
		}

		// 次のセルへ
		if (sx_i == NumOfColumns - 1) {
			sx_i = 0;
			sy_i++;
		}
		else {
			sx_i++;
		}
	}
	}

	// 描画する
	{
		int y;
		for (y = sy; y <= ey; y++) {
			int sx_i = 0;
			int ex_i = NumOfColumns - 1;
			if (y == sy) {
				sx_i = sx;
			}
			else if (y == ey) {
				ex_i = ex;
			}
			BuffDrawLineI(-1, -1, y + PageStart, sx_i, ex_i);
		}
	}
}

/**
 *	カーソル位置からURL強調を行う
 */
static void mark_url_w(int cur_x, int cur_y)
{
	buff_char_t *b = &CodeLineW[cur_x];
	const char32_t u32 = b->u32;
	int x = cur_x;
	BOOL prev = FALSE;
	BOOL next = FALSE;
	int i;
	BOOL match_flag = FALSE;
	int sx;
	int sy;
	int ey;
	int len;

	// 1つ前のセルがURL?
	if (x == 0) {
		// 一番左の時は、前の行から継続していて、前の行の最後がURLだった時
		if ((AttrLine[0] & AttrLineContinued) != 0) {
			const LONG TmpPtr = GetLinePtr(PageStart + cur_y - 1);
			if ((AttrBuff[TmpPtr + NumOfColumns - 1] & AttrURL) != 0) {
				prev = TRUE;
			}
		}
	}
	else {
		if (AttrLine[x - 1] & AttrURL) {
			prev = TRUE;
		}
	}

	// 1つ後ろのセルがURL?
	if (x == NumOfColumns - 1) {
		// 一番右
		if ((AttrLine[x] & AttrLineContinued) != 0) {
			const LONG TmpPtr = GetLinePtr(PageStart + cur_y + 1);
			if ((AttrBuff[TmpPtr + NumOfColumns - 1] & AttrURL) != 0) {
				next = TRUE;
			}
		}
	}
	else {
		if (AttrLine[x + 1] & AttrURL) {
			next = TRUE;
		}
	}

	if (prev == TRUE) {
		if (next == TRUE) {
			if (isURLchar(u32)) {
				// URLにはさまれていて、URLになりえるキャラクタ
				int ptr = GetLinePtr(PageStart + cur_y) + cur_x;
				AttrBuff[ptr] |= AttrURL;
				return;
			}
			// 1line検査
			mark_url_line_w(cur_x, cur_y);
			return;
		}

		len = get_url_len(cur_x, cur_y);
		if (len >= 9) {
			// URLアトリビュートがついている先頭から、
			// 9文字以上離れている場合は
			// 文字が上書きされてもURLが壊れることはない
			// → カーソル位置にURLアトリビュートをつける
			if (isURLchar(u32)) {
				// URLを伸ばす
				AttrLine[x] |= AttrURL;
			}
			return;
		}
		mark_url_line_w(cur_x, cur_y);
		return;
	}

	// '/' が入力されたら調べ始める
	if (u32 == '/') {
		if (!MatchString(x - 2, PageStart + CursorY, L"://", TRUE)) {
			// "://" の一部ではない
			return;
		}
	}

	// 本格的に探す
	for (i = 0; i < _countof(schemes); i++) {
		const wchar_t *prefix = schemes[i].str;
		len = schemes[i].len - 1;
		sx = x - len;
		sy = PageStart + CursorY;
		ey = sy;
		if (x < len) {
			// 短い
			if ((AttrLine[0] & AttrLineContinued) == 0) {
				// 前の行と連結していない
				continue;
			}
			// 前の行から検索かける
			sx = NumOfColumns + sx;
			sy = PageStart + CursorY - 1;
		}
		// マッチするか?
		if (BuffGetMatchPosFromString(sx, sy, x, ey, prefix, NULL, NULL)) {
			match_flag = TRUE;
			break;
		}
	}
	if (!match_flag) {
		return;
	}

	// マッチしたのでURL属性を付ける
	if (sy == ey) {
		for (i = 0; i <= len; i++) {
			AttrLine[sx + i] |= AttrURL;
		}
		if (StrChangeStart > sx) {
			StrChangeStart = sx;
			StrChangeCount += len;
		}
	}
	else {
		LONG TmpPtr = GetLinePtr(sy);
		int xx = sx;
		size_t left = len + 1;
		while (left > 0) {
			AttrBuff[TmpPtr + xx] |= AttrURL;
			xx++;
			if (xx == NumOfColumns) {
				int draw_x = sx;
				int draw_y = CursorY - 1;
				if (IsLineVisible(&draw_x, &draw_y)) {
					BuffDrawLineI(draw_x, draw_y, PageStart + CursorY - 1, sx, NumOfColumns - 1);
				}
				TmpPtr = NextLinePtr(TmpPtr);
				xx = 0;
			}
			left--;
		}
		StrChangeStart = 0;
		StrChangeCount = xx;
	}
}

static wchar_t *GetWCS(const buff_char_t *b)
{
	size_t len = (b->wc2[1] == 0) ? 2 : 3;
	wchar_t *strW;
	wchar_t *p;
	int i;

	len += b->CombinationCharCount16;
	strW = malloc(sizeof(wchar_t) * len);
	p = strW;
	*p++ = b->wc2[0];
	if (b->wc2[1] != 0) {
		*p++ = b->wc2[0];
	}
	for (i=0; i<b->CombinationCharCount16; i++) {
		*p++ = b->pCombinationChars16[i];
	}
	*p = L'\0';
	return strW;
}
#endif	// #if UNICODE_INTERNAL_BUFF

/**
 *	ユニコードキャラクタを1文字バッファへ入力する
 *	@param[in]	u32		unicode character(UTF-32)
 *	@param[in]	Attr	attributes
 *	@param[in]	Insert	Insert flag
 *	@return		カーソル移動量(0 or 1 or 2)
 */
#if UNICODE_INTERNAL_BUFF
int BuffPutUnicode(unsigned int u32, TCharAttr Attr, BOOL Insert)
{
	BYTE b1, b2;
	int move_x = 0;
	static BOOL show_str_change = FALSE;
	buff_char_t *p;

	assert(Attr.Attr == (Attr.AttrEx & 0xff));

	// TODO 入力文字を CP932 に変換しておく、廃止予定
	if (u32 < 0x80) {
		b1 = (BYTE)u32;
		b2 = 0;
	}
	else {
		char mbchar[2];
		size_t ret = UTF32ToCP932(u32, mbchar, 2);
		if (ret == 0) {
			b1 = '?';
			b2 = 0;
			ret = 1;
		}
		else if (ret == 1) {
			b1 = mbchar[0];
			b2 = 0;
		}
		else {  // ret == 2
			b1 = mbchar[0];
			b2 = mbchar[1];
			ret = 2;
		}
	}

#if 0
	OutputDebugPrintfW(L"BuffPutUnicode(U+%06x,(%d,%d)\n", u32, CursorX, CursorY);
#endif

	if (ts.EnableContinuedLineCopy && CursorX == 0 && (AttrLine[0] & AttrLineContinued)) {
		Attr.Attr |= AttrLineContinued;
	}

	p = NULL;  // NULLのとき、前の文字はない
	// 前の文字
	if (CursorX >= 1 && !IsBuffPadding(&CodeLineW[CursorX - 1])) {
		p = &CodeLineW[CursorX - 1];
	}
	else if (CursorX >= 2 && !IsBuffPadding(&CodeLineW[CursorX - 2])) {
		p = &CodeLineW[CursorX - 2];
	}

	if (UnicodeIsVariationSelector(u32) || UnicodeIsCombiningCharacter(u32) || (u32 == 0x200d) ||
		(p != NULL && p->u32_last == 0x200d)) {
		// Combining
		//		VariationSelector or
		//		CombiningCharacter or
		//		ゼロ幅接合子,ZERO WIDTH JOINER(ZWJ) (U+200d) or
		//		1つ前が ZWJ
		move_x = 0;  // カーソル移動量=0

		if (p == NULL) {
			// 前がないのにくっつく文字が出てきたとき
			// とりあえずスペースにくっつける
			p = &CodeLineW[CursorX];
			BuffSetChar(p, ' ', 'H');
		}

		// 前の文字にくっつける
		BuffAddChar(p, u32);
#if 0
		AttrLine[CursorX] = Attr.Attr;
		AttrLine2[CursorX] = Attr.Attr2;
		AttrLineFG[CursorX] = Attr.Fore;
		AttrLineBG[CursorX] = Attr.Back;
#endif
	}
	else {
		char width_property;
		char emoji;
		BOOL half_width = BuffIsHalfWidthFromCode(&ts, u32, &width_property, &emoji);

		p = &CodeLineW[CursorX];
		// 現在の位置が全角の右側?
		if (IsBuffPadding(p)) {
			// 全角の前半をスペースに置き換える
			assert(CursorX > 0);  // 行頭に全角の右側はない
			BuffSetChar(p - 1, ' ', 'H');
			BuffSetChar(p, ' ', 'H');
			if (StrChangeCount == 0) {
				StrChangeCount = 3;
				StrChangeStart = CursorX - 1;
			}
			else {
				if (StrChangeStart < CursorX) {
					StrChangeCount += (CursorX - StrChangeStart) + 1;
				}
				else {
					StrChangeStart = CursorX;
					StrChangeCount += CursorX - StrChangeStart;
				}
			}
		}
		// 現在の位置が全角の左側 && 入力文字が半角 ?
		if (half_width && IsBuffFullWidth(p)) {
			// 全角をスペースに置き換える
			assert(CursorX < NumOfColumns - 1);  // 行末に全角の左はこない
			BuffSetChar(p, ' ', 'H');
			BuffSetChar(p + 1, ' ', 'H');
			if (StrChangeCount == 0) {
				StrChangeCount = 3;
				StrChangeStart = CursorX;
			}
			else {
				if (CursorX < StrChangeStart) {
					assert(FALSE);
				}
				else {
					StrChangeCount += 2;
				}
			}
		}
		// 次の文字が全角 && 入力文字が全角 ?
		if (!Insert && !half_width && IsBuffFullWidth(p + 1)) {
			// 全角を潰す
			BuffSetChar(p + 1, ' ', 'H');
			BuffSetChar(p + 2, ' ', 'H');
		}

		if (Insert) {
			// 挿入モード
			// TODO 未チェック
			int XStart, LineEnd, MoveLen;
			int extr = 0;
			if (CursorX > CursorRightM)
				LineEnd = NumOfColumns - 1;
			else
				LineEnd = CursorRightM;

			if (half_width) {
				// 半角として扱う
				move_x = 1;
			}
			else {
				// 全角として扱う
				move_x = 2;
				if (CursorX + 1 > LineEnd) {
					// はみ出す
					return -1;
				}
			}

			// 一番最後の文字が全角の場合、
			if (LineEnd <= NumOfColumns - 1 && (AttrLine[LineEnd - 1] & AttrKanji)) {
				CodeLine[LineEnd - 1] = 0x20;
				BuffSetChar(&CodeLineW[LineEnd - 1], 0x20, 'H');
				AttrLine[LineEnd] &= ~AttrKanji;
				//				CodeLine[LineEnd+1] = 0x20;
				//				AttrLine[LineEnd+1] &= ~AttrKanji;
				extr = 1;
			}

			if (!half_width) {
				MoveLen = LineEnd - CursorX - 1;
				if (MoveLen > 0) {
					memmove(&CodeLine[CursorX + 2], &CodeLine[CursorX], MoveLen);
					memmoveW(&(CodeLineW[CursorX + 2]), &(CodeLineW[CursorX]), MoveLen);
					memmove(&AttrLine[CursorX + 2], &AttrLine[CursorX], MoveLen);
					memmove(&AttrLine2[CursorX + 2], &AttrLine2[CursorX], MoveLen);
					memmove(&AttrLineFG[CursorX + 2], &AttrLineFG[CursorX], MoveLen);
					memmove(&AttrLineBG[CursorX + 2], &AttrLineBG[CursorX], MoveLen);
				}
			}
			else {
				MoveLen = LineEnd - CursorX;
				if (MoveLen > 0) {
					memmove(&CodeLine[CursorX + 1], &CodeLine[CursorX], MoveLen);
					memmoveW(&(CodeLineW[CursorX + 1]), &(CodeLineW[CursorX]), MoveLen);
					memmove(&AttrLine[CursorX + 1], &AttrLine[CursorX], MoveLen);
					memmove(&AttrLine2[CursorX + 1], &AttrLine2[CursorX], MoveLen);
					memmove(&AttrLineFG[CursorX + 1], &AttrLineFG[CursorX], MoveLen);
					memmove(&AttrLineBG[CursorX + 1], &AttrLineBG[CursorX], MoveLen);
				}
			}

			CodeLine[CursorX] = b1;
			BuffSetChar2(&CodeLineW[CursorX], u32, width_property, half_width, emoji);
			AttrLine[CursorX] = Attr.Attr;
			AttrLine2[CursorX] = Attr.Attr2;
			AttrLineFG[CursorX] = Attr.Fore;
			AttrLineBG[CursorX] = Attr.Back;
			if (!half_width && CursorX < LineEnd) {
				CodeLine[CursorX + 1] = 0;
				BuffSetChar(&CodeLineW[CursorX + 1], 0, 'H');
				CodeLineW[CursorX + 1].Padding = TRUE;
				AttrLine[CursorX + 1] = Attr.Attr;
				AttrLine2[CursorX + 1] = Attr.Attr2;
				AttrLineFG[CursorX + 1] = Attr.Fore;
				AttrLineBG[CursorX + 1] = Attr.Back;
			}
#if 0
			/* begin - ishizaki */
			markURL(CursorX);
			markURL(CursorX+1);
			/* end - ishizaki */
#endif

			/* last char in current line is kanji first? */
			if ((AttrLine[LineEnd] & AttrKanji) != 0) {
				/* then delete it */
				CodeLine[LineEnd] = 0x20;
				BuffSetChar(&CodeLineW[LineEnd], 0x20, 'H');
				AttrLine[LineEnd] = CurCharAttr.Attr;
				AttrLine2[LineEnd] = CurCharAttr.Attr2;
				AttrLineFG[LineEnd] = CurCharAttr.Fore;
				AttrLineBG[LineEnd] = CurCharAttr.Back;
			}

			if (StrChangeCount == 0) {
				XStart = CursorX;
			}
			else {
				XStart = StrChangeStart;
			}
			StrChangeCount = 0;
			BuffUpdateRect(XStart, CursorY, LineEnd + extr, CursorY);
		}
		else {
			if ((Attr.AttrEx & AttrPadding) != 0) {
				// 詰め物
				buff_char_t *p = &CodeLineW[CursorX];
				BuffSetChar(p, u32, 'H');
				p->Padding = TRUE;
				AttrLine[CursorX] = Attr.Attr;
				AttrLine2[CursorX] = Attr.Attr2;
				AttrLineFG[CursorX] = Attr.Fore;
				AttrLineBG[CursorX] = Attr.Back;
				move_x = 1;
			}
			else {
				// 新しい文字追加

				if (half_width) {
					// 半角として扱う
					move_x = 1;
				}
				else {
					// 全角として扱う
					move_x = 2;
					if (CursorX + 2 > NumOfColumns) {
						// はみ出す
						return -1;
					}
				}

				CodeLine[CursorX] = b1;
				BuffSetChar2(&CodeLineW[CursorX], u32, width_property, half_width, emoji);
				if (half_width) {
					AttrLine[CursorX] = Attr.Attr;
				}
				else {
					// 全角
					AttrLine[CursorX] = Attr.Attr | AttrKanji;
				}
				AttrLine2[CursorX] = Attr.Attr2;
				AttrLineFG[CursorX] = Attr.Fore;
				AttrLineBG[CursorX] = Attr.Back;

				if (!half_width) {
					// 全角の時は次のセルは詰め物
					if (CursorX < NumOfColumns - 1) {
						buff_char_t *p = &CodeLineW[CursorX + 1];
						BuffSetChar(p, 0, 'H');
						p->Padding = TRUE;
						CodeLine[CursorX + 1] = b2;
						AttrLine[CursorX + 1] = 0;
						AttrLine2[CursorX + 1] = 0;
						AttrLineFG[CursorX + 1] = 0;
						AttrLineBG[CursorX + 1] = 0;
					}
				}
			}

			if (StrChangeCount == 0) {
				StrChangeStart = CursorX;
			}
			if (move_x == 0) {
				if (StrChangeCount == 0) {
					StrChangeCount = 1;
				}
			}
			else if (move_x == 1) {
				// 半角
				StrChangeCount = StrChangeCount + 1;
			}
			else /*if (move_x == 2)*/ {
				// 全角
				StrChangeCount = StrChangeCount + 2;
			}

			// URLの検出
			mark_url_w(CursorX, CursorY);

#if 0
			{
				char ba[128];
				memcpy(ba, &CodeLine[0], _countof(ba)-1);
				ba[127] = 0;
				OutputDebugPrintf("A '%s'\n", ba);
			}
#endif
		}
	}

	if (show_str_change) {
		OutputDebugPrintf("StrChangeStart,Count %d,%d\n", StrChangeStart, StrChangeCount);
	}

#if 0
	{
		wchar_t *wcs;
		p = &CodeLineW[CursorX];
		wcs = GetWCS(p);
		OutputDebugPrintf("BuffPutUnicode '%s' leave\n",wcs);
		free(wcs);
	}
#endif
	return move_x;
}
#endif

#if UNICODE_INTERNAL_BUFF
void BuffPutChar(BYTE b, TCharAttr Attr, BOOL Insert)
{
	BuffPutUnicode(b, Attr, Insert);
}
#endif

static BOOL CheckSelect(int x, int y)
//  subroutine called by BuffUpdateRect
{
	LONG L, L1, L2;

	if (BoxSelect) {
		return (Selected &&
		((SelectStart.x<=x) && (x<SelectEnd.x) ||
		 (SelectEnd.x<=x) && (x<SelectStart.x)) &&
		((SelectStart.y<=y) && (y<=SelectEnd.y) ||
		 (SelectEnd.y<=y) && (y<=SelectStart.y)));
	}
	else {
		L = MAKELONG(x,y);
		L1 = MAKELONG(SelectStart.x,SelectStart.y);
		L2 = MAKELONG(SelectEnd.x,SelectEnd.y);

		return (Selected &&
			((L1<=L) && (L<L2) || (L2<=L) && (L<L1)));
	}
}

#if UNICODE_INTERNAL_BUFF
/*
 *	1行描画
 *
 *	@param	DrawX,Y			Clint領域描画位置(pixel)
 *	@param	SY				スクリーン上の位置(Charactor)  !バッファ上の位置
 *							PageStart + YStart など
 *	@param	IStart,IEnd		スクリーン上の位置(Charactor)
 *							指定した間を描画する
 */
static void BuffDrawLineI(int DrawX, int DrawY, int SY, int IStart, int IEnd)
{
	int X = DrawX;
	int Y = DrawY;
	const LONG TmpPtr = GetLinePtr(SY);
	int istart = IStart;
	char bufA[TermWidthMax];
	wchar_t bufW[TermWidthMax];
	char bufWW[TermWidthMax];
	int lenW = 0;
	int lenA = 0;
	TCharAttr CurAttr;
	BOOL CurAttrEmoji;
	BOOL CurSelected;
	BOOL EndFlag = FALSE;
	int count = 0;		// 現在注目している文字,IStartから
#if 0
	OutputDebugPrintf("BuffDrawLineI(%d,%d, %d,%d-%d)\n", DrawX, DrawY, SY, IStart, IEnd);
#endif
	{
		// カーソル位置、表示開始位置から描画位置がわかるはず
		int X2 = IStart;
		int Y2 = SY - PageStart;
		if (! IsLineVisible(&X2, &Y2)) {
			// 描画不要行
			//assert(FALSE);
			return;
		}
		if (X != -1 && Y != -1) {
			assert(X == X2 && Y == Y2);
		}
		else {
			X = X2;
			Y = Y2;
		}
	}
	if (IEnd >= NumOfColumns) {
		IEnd = NumOfColumns - 1;
	}
	while (!EndFlag) {
		const buff_char_t *b = &CodeBuffW[TmpPtr + istart + count];

		BOOL DrawFlag = FALSE;
		BOOL SetString = FALSE;

		// アトリビュート取得
		if (count == 0) {
			// 最初の1文字目
			int ptr = TmpPtr + istart + count;
			if (b->u32 == 0) {	// TODO IsBuffPadding()
				// 最初に表示しようとした文字が全角の右だった場合
				ptr--;
			}
			CurAttr.Attr = AttrBuff[ptr] & ~ AttrKanji;
			CurAttr.Attr2 = AttrBuff2[ptr];
			CurAttr.Fore = AttrBuffFG[ptr];
			CurAttr.Back = AttrBuffBG[ptr];
			CurAttrEmoji = b->Emoji;
			CurSelected = CheckSelect(istart+count,SY);
		}

		bufA[lenA] = CodeBuff[TmpPtr + istart + count];
		lenA++;

		if (b->u32 == 0) {	// TODO IsBuffPadding()
			// 全角の次の文字,処理不要
		} else {
			if (count == 0) {
				// 最初の1文字目
				SetString = TRUE;
			} else {
				TCharAttr TempAttr;
				TempAttr.Attr = AttrBuff[TmpPtr+istart+count] & ~ AttrKanji;
				TempAttr.Attr2 = AttrBuff2[TmpPtr+istart+count];
				TempAttr.Fore = AttrBuffFG[TmpPtr+istart+count];
				TempAttr.Back = AttrBuffBG[TmpPtr+istart+count];
				if (b->u32 != 0 &&
					((TCharAttrCmp(CurAttr, TempAttr) != 0 || CurAttrEmoji != b->Emoji) ||
					 (CurSelected != CheckSelect(istart+count,SY)))){
					// この文字でアトリビュートが変化した → 描画
					DrawFlag = TRUE;
					count--;
				} else {
					SetString = TRUE;
				}
			}
		}

		if (SetString) {
			if (b->u32 < 0x10000) {
				bufW[lenW] = b->wc2[0];
				bufWW[lenW] = b->HalfWidth ? 'H' : 'W';
				lenW++;
			} else {
				// UTF-16でサロゲートペア
				bufW[lenW] = b->wc2[0];
				bufWW[lenW] = b->HalfWidth ? 'H' : 'W';
				lenW++;
				bufW[lenW] = b->wc2[1];
				bufWW[lenW] = '0';
				lenW++;
//				DrawFlag = TRUE;	// すぐに描画する
			}
			if (b->CombinationCharCount16 != 0)
			{
				// コンビネーション
				int i;
				for (i = 0 ; i < (int)b->CombinationCharCount16; i++) {
					bufW[lenW+i] = b->pCombinationChars16[i];
					bufWW[lenW + i] = '0';
				}
				lenW += b->CombinationCharCount16;
				DrawFlag = TRUE;	// コンビネーションがある場合はすぐ描画
			}
		}

		// 最後までスキャンした?
		if (istart + count >= IEnd) {
			DrawFlag = TRUE;
			EndFlag = TRUE;
		}

		if (DrawFlag) {
			// 描画する
			bufA[lenA] = 0;
			bufW[lenW] = 0;
			bufWW[lenW] = 0;

#if 0
			OutputDebugPrintf("A[%d] '%s'\n", lenA, bufA);
			OutputDebugPrintfW(L"W[%d] '%s'\n", lenW, bufW);
#endif

			DispSetupDC(CurAttr, CurSelected);
#if UNICODE_INTERNAL_BUFF && UNICODE_DISPLAY
			DispStrW(bufW, bufWW, lenW, Y, &X);
#else
			DispStr(bufA, lenA, Y, &X);
#endif

			lenA = 0;
			lenW = 0;
			DrawFlag = FALSE;
			istart += (count + 1);
			count = 0;
		} else {
			count++;
		}
	}
}
#endif

void BuffUpdateRect
  (int XStart, int YStart, int XEnd, int YEnd)
// Display text in a rectangular region in the screen
//   XStart: x position of the upper-left corner (screen cordinate)
//   YStart: y position
//   XEnd: x position of the lower-right corner (last character)
//   YEnd: y position
{
	int i, j;
#if !UNICODE_INTERNAL_BUFF
	int count;
#endif
	int IStart, IEnd;
	int X, Y;
	LONG TmpPtr;
	TCharAttr TempAttr;
#if !UNICODE_INTERNAL_BUFF
	TCharAttr CurAttr;
	BOOL CurSel;
#endif
	BOOL TempSel, Caret;
#if UNICODE_INTERNAL_BUFF
//	char bufA[256];
//	wchar_t bufW[256];
//	int lenW = 0;
//	int lenA = 0;
#endif

	if (XStart >= WinOrgX+WinWidth) {
		return;
	}
	if (YStart >= WinOrgY+WinHeight) {
		return;
	}
	if (XEnd < WinOrgX) {
		return;
	}
	if (YEnd < WinOrgY) {
		return;
	}

	if (XStart < WinOrgX) {
		XStart = WinOrgX;
	}
	if (YStart < WinOrgY) {
		YStart = WinOrgY;
	}
	if (XEnd >= WinOrgX+WinWidth) {
		XEnd = WinOrgX+WinWidth-1;
	}
	if (YEnd >= WinOrgY+WinHeight) {
		YEnd = WinOrgY+WinHeight-1;
	}

#if 0
	OutputDebugPrintf("BuffUpdateRect (%d,%d)-(%d,%d) [%d,%d]\n",
					  XStart, YStart,
					  XEnd, YEnd,
					  XEnd - XStart + 1, YEnd - YStart + 1);
#endif

	TempAttr = DefCharAttr;
	TempSel = FALSE;

	Caret = IsCaretOn();
	if (Caret) {
		CaretOff();
	}

	DispSetupDC(DefCharAttr, TempSel);

	Y = (YStart-WinOrgY)*FontHeight;
	TmpPtr = GetLinePtr(PageStart+YStart);
	for (j = YStart+PageStart ; j <= YEnd+PageStart ; j++) {
		IStart = XStart;
		IEnd = XEnd;

		IStart = LeftHalfOfDBCS(TmpPtr,IStart);

		X = (IStart-WinOrgX)*FontWidth;

		i = IStart;
#if !UNICODE_INTERNAL_BUFF
		do {
			CurAttr.Attr = AttrBuff[TmpPtr+i] & ~ AttrKanji;
			CurAttr.Attr2 = AttrBuff2[TmpPtr+i];
			CurAttr.Fore = AttrBuffFG[TmpPtr+i];
			CurAttr.Back = AttrBuffBG[TmpPtr+i];
			CurSel = CheckSelect(i,j);
			count = 1;
			while ( (i+count <= IEnd) &&
			        (CurAttr.Attr == (AttrBuff[TmpPtr+i+count] & ~ AttrKanji)) &&
			        (CurAttr.Attr2==AttrBuff2[TmpPtr+i+count]) &&
			        (CurAttr.Fore==AttrBuffFG[TmpPtr+i+count]) &&
			        (CurAttr.Back==AttrBuffBG[TmpPtr+i+count]) &&
			        (CurSel==CheckSelect(i+count,j)) ||
			        (i+count<NumOfColumns) &&
			        ((AttrBuff[TmpPtr+i+count-1] & AttrKanji) != 0) ) {
				count++;
			}

			if (TCharAttrCmp(CurAttr, TempAttr) != 0 || (CurSel != TempSel)) {
				DispSetupDC(CurAttr, CurSel);
				TempAttr = CurAttr;
				TempSel = CurSel;
			}
			DispStr(&CodeBuff[TmpPtr+i],count,Y, &X);
			i = i+count;
		}
		while (i<=IEnd);
#else
		{
			BuffDrawLineI(X, Y, j, IStart, IEnd);
		}
#endif
		Y = Y + FontHeight;
		TmpPtr = NextLinePtr(TmpPtr);
	}
	if (Caret) {
		CaretOn();
	}
}

void UpdateStr()
// Display not-yet-displayed string
{
	int X, Y;
#if !UNICODE_INTERNAL_BUFF
	TCharAttr TempAttr;
#endif
#if !UNICODE_INTERNAL_BUFF
	int pos, len;
#endif

	if (StrChangeCount==0) {
		return;
	}
	X = StrChangeStart;
	Y = CursorY;
	if (! IsLineVisible(&X, &Y)) {
		StrChangeCount = 0;
		return;
	}

#if !UNICODE_INTERNAL_BUFF
	TempAttr.Attr = AttrLine[StrChangeStart];
	TempAttr.Attr2 = AttrLine2[StrChangeStart];
	TempAttr.Fore = AttrLineFG[StrChangeStart];
	TempAttr.Back = AttrLineBG[StrChangeStart];

	/* これから描画する文字列の始まりが「URL構成文字属性」だった場合、
	 * 当該色で行末までペイントされないようにする。
	 * (2009.10.24 yutaka)
	 */
	if (TempAttr.Attr & AttrURL) {
		/* 開始位置からどこまでが AttrURL かをカウントする */
		len = 0;
		for (pos = 0 ; pos < StrChangeCount ; pos++) {
			if (TempAttr.Attr != AttrLine[StrChangeStart + pos])
				break;
			len++;
		}
		DispSetupDC(TempAttr, FALSE);
		DispStr(&CodeLine[StrChangeStart], len, Y, &X);

		/* 残りの文字列があれば、ふつうに描画を行う。*/
		if (len < StrChangeCount) {
			TempAttr.Attr = AttrLine[StrChangeStart + pos];
			TempAttr.Attr2 = AttrLine2[StrChangeStart + pos];
			TempAttr.Fore = AttrLineFG[StrChangeStart + pos];
			TempAttr.Back = AttrLineBG[StrChangeStart + pos];

			DispSetupDC(TempAttr, FALSE);
			DispStr(&CodeLine[StrChangeStart + pos], (StrChangeCount - len), Y, &X);
		}
	} else {
		DispSetupDC(TempAttr, FALSE);
		DispStr(&CodeLine[StrChangeStart],StrChangeCount,Y, &X);
	}
#else
	BuffDrawLineI(X, Y, PageStart + CursorY, StrChangeStart, StrChangeStart + StrChangeCount - 1);
#endif
	StrChangeCount = 0;
}

#if 0
void UpdateStrUnicode(void)
// Display not-yet-displayed string
{
  int X, Y;
  TCharAttr TempAttr;

  if (StrChangeCount==0) return;
  X = StrChangeStart;
  Y = CursorY;
  if (! IsLineVisible(&X, &Y))
  {
    StrChangeCount = 0;
    return;
  }

  TempAttr.Attr = AttrLine[StrChangeStart];
  TempAttr.Attr2 = AttrLine2[StrChangeStart];
  TempAttr.Fore = AttrLineFG[StrChangeStart];
  TempAttr.Back = AttrLineBG[StrChangeStart];
  DispSetupDC(TempAttr, FALSE);
  DispStr(&CodeLine[StrChangeStart],StrChangeCount,Y, &X);
  StrChangeCount = 0;
}
#endif

void MoveCursor(int Xnew, int Ynew)
{
	UpdateStr();

	if (CursorY!=Ynew) {
		NewLine(PageStart+Ynew);
	}

	CursorX = Xnew;
	CursorY = Ynew;
	Wrap = FALSE;

	/* 最下行でだけ自動スクロールする*/
	if (ts.AutoScrollOnlyInBottomLine == 0 || WinOrgY == 0) {
		DispScrollToCursor(CursorX, CursorY);
	}
}

void MoveRight()
/* move cursor right, but dont update screen.
  this procedure must be called from DispChar&DispKanji only */
{
	CursorX++;
	/* 最下行でだけ自動スクロールする */
	if (ts.AutoScrollOnlyInBottomLine == 0 || WinOrgY == 0) {
		DispScrollToCursor(CursorX, CursorY);
	}
}

void BuffSetCaretWidth()
{
	BOOL DW;

	/* check whether cursor on a DBCS character */
	DW = (((BYTE)(AttrLine[CursorX]) & AttrKanji) != 0);
	DispSetCaretWidth(DW);
}

void ScrollUp1Line()
{
	int i, linelen;
	int extl=0, extr=0;
	LONG SrcPtr, DestPtr;

	if ((CursorTop<=CursorY) && (CursorY<=CursorBottom)) {
		UpdateStr();

		if (CursorLeftM > 0)
			extl = 1;
		if (CursorRightM < NumOfColumns-1)
			extr = 1;
		if (extl || extr)
			EraseKanjiOnLRMargin(GetLinePtr(PageStart+CursorTop), CursorBottom-CursorTop+1);

		linelen = CursorRightM - CursorLeftM + 1;
		DestPtr = GetLinePtr(PageStart+CursorBottom) + CursorLeftM;
		for (i = CursorBottom-1 ; i >= CursorTop ; i--) {
			SrcPtr = PrevLinePtr(DestPtr);
			memcpy(&(CodeBuff[DestPtr]), &(CodeBuff[SrcPtr]), linelen);
#if UNICODE_INTERNAL_BUFF
			memcpyW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
#endif
			memcpy(&(AttrBuff[DestPtr]), &(AttrBuff[SrcPtr]), linelen);
			memcpy(&(AttrBuff2[DestPtr]), &(AttrBuff2[SrcPtr]), linelen);
			memcpy(&(AttrBuffFG[DestPtr]), &(AttrBuffFG[SrcPtr]), linelen);
			memcpy(&(AttrBuffBG[DestPtr]), &(AttrBuffBG[SrcPtr]), linelen);
			DestPtr = SrcPtr;
		}
		memset(&(CodeBuff[SrcPtr]), 0x20, linelen);
#if UNICODE_INTERNAL_BUFF
		memsetW(&(CodeBuffW[SrcPtr]), 0x20, linelen);
#endif
		memset(&(AttrBuff[SrcPtr]), AttrDefault, linelen);
		memset(&(AttrBuff2[SrcPtr]), CurCharAttr.Attr2 & Attr2ColorMask, linelen);
		memset(&(AttrBuffFG[SrcPtr]), CurCharAttr.Fore, linelen);
		memset(&(AttrBuffBG[SrcPtr]), CurCharAttr.Back, linelen);

		if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1)
			BuffUpdateRect(CursorLeftM-extl, CursorTop, CursorRightM+extr, CursorBottom);
		else
			DispScrollNLines(CursorTop, CursorBottom, -1);
	}
}

void BuffScrollNLines(int n)
{
	int i, linelen;
	int extl=0, extr=0;
	LONG SrcPtr, DestPtr;

	if (n<1) {
		return;
	}
	UpdateStr();

	if (CursorLeftM == 0 && CursorRightM == NumOfColumns-1 && CursorTop == 0) {
		if (CursorBottom == NumOfLines-1) {
			WinOrgY = WinOrgY-n;
			/* 最下行でだけ自動スクロールする */
			if (ts.AutoScrollOnlyInBottomLine != 0 && NewOrgY != 0) {
				NewOrgY = WinOrgY;
			}
			BuffScroll(n,CursorBottom);
			DispCountScroll(n);
			return;
		}
		else if (CursorY <= CursorBottom) {
			/* 最下行でだけ自動スクロールする */
			if (ts.AutoScrollOnlyInBottomLine != 0 && NewOrgY != 0) {
				/* スクロールさせない場合の処理 */
				WinOrgY = WinOrgY-n;
				NewOrgY = WinOrgY;
				BuffScroll(n,CursorBottom);
				DispCountScroll(n);
			} else {
				BuffScroll(n,CursorBottom);
				DispScrollNLines(WinOrgY,CursorBottom,n);
			}
			return;
		}
	}

	if ((CursorTop<=CursorY) && (CursorY<=CursorBottom)) {
		if (CursorLeftM > 0)
			extl = 1;
		if (CursorRightM < NumOfColumns-1)
			extr = 1;
		if (extl || extr)
			EraseKanjiOnLRMargin(GetLinePtr(PageStart+CursorTop), CursorBottom-CursorTop+1);

		linelen = CursorRightM - CursorLeftM + 1;
		DestPtr = GetLinePtr(PageStart+CursorTop) + (LONG)CursorLeftM;
		if (n<CursorBottom-CursorTop+1) {
			SrcPtr = GetLinePtr(PageStart+CursorTop+n) + (LONG)CursorLeftM;
			for (i = CursorTop+n ; i<=CursorBottom ; i++) {
				memmove(&(CodeBuff[DestPtr]), &(CodeBuff[SrcPtr]), linelen);
#if UNICODE_INTERNAL_BUFF
				memmoveW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
#endif
				memmove(&(AttrBuff[DestPtr]), &(AttrBuff[SrcPtr]), linelen);
				memmove(&(AttrBuff2[DestPtr]), &(AttrBuff2[SrcPtr]), linelen);
				memmove(&(AttrBuffFG[DestPtr]), &(AttrBuffFG[SrcPtr]), linelen);
				memmove(&(AttrBuffBG[DestPtr]), &(AttrBuffBG[SrcPtr]), linelen);
				SrcPtr = NextLinePtr(SrcPtr);
				DestPtr = NextLinePtr(DestPtr);
			}
		}
		else {
			n = CursorBottom-CursorTop+1;
		}
		for (i = CursorBottom+1-n ; i<=CursorBottom; i++) {
			memset(&(CodeBuff[DestPtr]), 0x20, linelen);
#if UNICODE_INTERNAL_BUFF
			memsetW(&(CodeBuffW[DestPtr]), 0x20, linelen);
#endif
			memset(&(AttrBuff[DestPtr]), AttrDefault, linelen);
			memset(&(AttrBuff2[DestPtr]), CurCharAttr.Attr2 & Attr2ColorMask, linelen);
			memset(&(AttrBuffFG[DestPtr]), CurCharAttr.Fore, linelen);
			memset(&(AttrBuffBG[DestPtr]), CurCharAttr.Back, linelen);
			DestPtr = NextLinePtr(DestPtr);
		}
		if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1)
			BuffUpdateRect(CursorLeftM-extl, CursorTop, CursorRightM+extr, CursorBottom);
		else
			DispScrollNLines(CursorTop, CursorBottom, n);
	}
}

void BuffRegionScrollUpNLines(int n) {
	int i, linelen;
	int extl=0, extr=0;
	LONG SrcPtr, DestPtr;

	if (n<1) {
		return;
	}
	UpdateStr();

	if (CursorLeftM == 0 && CursorRightM == NumOfColumns-1 && CursorTop == 0) {
		if (CursorBottom == NumOfLines-1) {
			WinOrgY = WinOrgY-n;
			BuffScroll(n,CursorBottom);
			DispCountScroll(n);
		}
		else {
			BuffScroll(n,CursorBottom);
			DispScrollNLines(WinOrgY,CursorBottom,n);
		}
	}
	else {
		if (CursorLeftM > 0)
			extl = 1;
		if (CursorRightM < NumOfColumns-1)
			extr = 1;
		if (extl || extr)
			EraseKanjiOnLRMargin(GetLinePtr(PageStart+CursorTop), CursorBottom-CursorTop+1);

		DestPtr = GetLinePtr(PageStart+CursorTop) + CursorLeftM;
		linelen = CursorRightM - CursorLeftM + 1;
		if (n < CursorBottom - CursorTop + 1) {
			SrcPtr = GetLinePtr(PageStart+CursorTop+n) + CursorLeftM;
			for (i = CursorTop+n ; i<=CursorBottom ; i++) {
				memmove(&(CodeBuff[DestPtr]), &(CodeBuff[SrcPtr]), linelen);
#if UNICODE_INTERNAL_BUFF
				memmoveW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
#endif
				memmove(&(AttrBuff[DestPtr]), &(AttrBuff[SrcPtr]), linelen);
				memmove(&(AttrBuff2[DestPtr]), &(AttrBuff2[SrcPtr]), linelen);
				memmove(&(AttrBuffFG[DestPtr]), &(AttrBuffFG[SrcPtr]), linelen);
				memmove(&(AttrBuffBG[DestPtr]), &(AttrBuffBG[SrcPtr]), linelen);
				SrcPtr = NextLinePtr(SrcPtr);
				DestPtr = NextLinePtr(DestPtr);
			}
		}
		else {
			n = CursorBottom - CursorTop + 1;
		}
		for (i = CursorBottom+1-n ; i<=CursorBottom; i++) {
			memset(&(CodeBuff[DestPtr]), 0x20, linelen);
#if UNICODE_INTERNAL_BUFF
			memsetW(&(CodeBuffW[DestPtr]), 0x20, linelen);
#endif
			memset(&(AttrBuff[DestPtr]), AttrDefault, linelen);
			memset(&(AttrBuff2[DestPtr]), CurCharAttr.Attr2 & Attr2ColorMask, linelen);
			memset(&(AttrBuffFG[DestPtr]), CurCharAttr.Fore, linelen);
			memset(&(AttrBuffBG[DestPtr]), CurCharAttr.Back, linelen);
			DestPtr = NextLinePtr(DestPtr);
		}

		if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1) {
			BuffUpdateRect(CursorLeftM-extl, CursorTop, CursorRightM+extr, CursorBottom);
		}
		else {
			DispScrollNLines(CursorTop, CursorBottom, n);
		}
	}
}

void BuffRegionScrollDownNLines(int n) {
	int i, linelen;
	int extl=0, extr=0;
	LONG SrcPtr, DestPtr;

	if (n<1) {
		return;
	}
	UpdateStr();

	if (CursorLeftM > 0)
		extl = 1;
	if (CursorRightM < NumOfColumns-1)
		extr = 1;
	if (extl || extr)
		EraseKanjiOnLRMargin(GetLinePtr(PageStart+CursorTop), CursorBottom-CursorTop+1);

	DestPtr = GetLinePtr(PageStart+CursorBottom) + CursorLeftM;
	linelen = CursorRightM - CursorLeftM + 1;
	if (n < CursorBottom - CursorTop + 1) {
		SrcPtr = GetLinePtr(PageStart+CursorBottom-n) + CursorLeftM;
		for (i=CursorBottom-n ; i>=CursorTop ; i--) {
			memmove(&(CodeBuff[DestPtr]), &(CodeBuff[SrcPtr]), linelen);
#if UNICODE_INTERNAL_BUFF
			memmoveW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
#endif
			memmove(&(AttrBuff[DestPtr]), &(AttrBuff[SrcPtr]), linelen);
			memmove(&(AttrBuff2[DestPtr]), &(AttrBuff2[SrcPtr]), linelen);
			memmove(&(AttrBuffFG[DestPtr]), &(AttrBuffFG[SrcPtr]), linelen);
			memmove(&(AttrBuffBG[DestPtr]), &(AttrBuffBG[SrcPtr]), linelen);
			SrcPtr = PrevLinePtr(SrcPtr);
			DestPtr = PrevLinePtr(DestPtr);
		}
	}
	else {
		n = CursorBottom - CursorTop + 1;
	}
	for (i = CursorTop+n-1; i>=CursorTop; i--) {
		memset(&(CodeBuff[DestPtr]), 0x20, linelen);
#if UNICODE_INTERNAL_BUFF
		memsetW(&(CodeBuffW[DestPtr]), 0x20, linelen);
#endif
		memset(&(AttrBuff[DestPtr]), AttrDefault, linelen);
		memset(&(AttrBuff2[DestPtr]), CurCharAttr.Attr2 & Attr2ColorMask, linelen);
		memset(&(AttrBuffFG[DestPtr]), CurCharAttr.Fore, linelen);
		memset(&(AttrBuffBG[DestPtr]), CurCharAttr.Back, linelen);
		DestPtr = PrevLinePtr(DestPtr);
	}

	if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1) {
		BuffUpdateRect(CursorLeftM-extl, CursorTop, CursorRightM+extr, CursorBottom);
	}
	else {
		DispScrollNLines(CursorTop, CursorBottom, -n);
	}
}

void BuffClearScreen()
{ // clear screen
	if (isCursorOnStatusLine) {
		BuffScrollNLines(1); /* clear status line */
	}
	else { /* clear main screen */
		UpdateStr();
		BuffScroll(NumOfLines-StatusLine,NumOfLines-1-StatusLine);
		DispScrollNLines(WinOrgY,NumOfLines-1-StatusLine,NumOfLines-StatusLine);
	}
}

void BuffUpdateScroll()
// Updates scrolling
{
	UpdateStr();
	DispUpdateScroll();
}

void CursorUpWithScroll()
{
	if ((0<CursorY) && (CursorY<CursorTop) ||
	    (CursorTop<CursorY)) {
		MoveCursor(CursorX,CursorY-1);
	}
	else if (CursorY==CursorTop) {
		ScrollUp1Line();
	}
}

// called by BuffDblClk
//   check if a character is the word delimiter
static BOOL IsDelimiter(LONG Line, int CharPtr)
{
	if ((AttrBuff[Line+CharPtr] & AttrKanji) !=0) {
		return (ts.DelimDBCS!=0);
	}
	return (strchr(ts.DelimList,CodeBuff[Line+CharPtr])!=NULL);
}

void GetMinMax(int i1, int i2, int i3,
               int *min, int *max)
{
	if (i1<i2) {
		*min = i1;
		*max = i2;
	}
	else {
		*min = i2;
		*max = i1;
	}
	if (i3<*min) {
		*min = i3;
	}
	if (i3>*max) {
		*max = i3;
	}
}

static void invokeBrowserWithUrl(const char *url)
{
	BOOL use_browser = FALSE;
	if (strncmp(url, "http://", strlen("http://")) == 0 || strncmp(url, "https://", strlen("https://")) == 0 ||
		strncmp(url, "ftp://", strlen("ftp://")) == 0) {
		use_browser = TRUE;
	}

	if (use_browser && ts.ClickableUrlBrowser[0] != 0) {
		// ブラウザを使用する
		char param[1024];
		_snprintf_s(param, sizeof(param), _TRUNCATE, "%s %s", ts.ClickableUrlBrowserArg, url);
		if (ShellExecuteA(NULL, NULL, ts.ClickableUrlBrowser, param, NULL, SW_SHOWNORMAL) >= (HINSTANCE)32) {
			// 実行できた
			return;
		}
		// コマンドの実行に失敗した場合は通常と同じ処理をする
	}

	ShellExecuteA(NULL, NULL, url, NULL, NULL, SW_SHOWNORMAL);
}

#if !UNICODE_INTERNAL_BUFF
static void invokeBrowser(LONG ptr)
{
	LONG i, start, end;
	char url[1024];
	char *uptr, ch;

	start = ptr;
	while (AttrBuff[start] & AttrURL) {
		start--;
	}
	start++;

	end = ptr;
	while (AttrBuff[end] & AttrURL) {
		end++;
	}
	end--;

	if (start + (LONG)sizeof(url) -1 <= end) {
		end = start + sizeof(url) - 2;
	}
	uptr = url;
	for (i = 0; i < end - start + 1; i++) {
		ch = CodeBuff[start + i];
		if ((start + i) % NumOfColumns == NumOfColumns - 1
			&& ch == ts.JoinSplitURLIgnoreEOLChar) {
			// 行末が行継続マーク用の文字の場合はスキップする
		} else {
			*uptr++ = ch;
		}
	}
	*uptr = '\0';

	invokeBrowserWithUrl(url);
}
#endif

#if UNICODE_INTERNAL_BUFF
static void invokeBrowserW(int x, int y)
{
	const LONG TmpPtr = GetLinePtr(y);
	int sx;
	int ex;

	sx = x;
	while (AttrBuff[TmpPtr + sx] & AttrURL) {
		sx--;
	}
	sx++;

	ex = x;
	while (AttrBuff[TmpPtr + ex] & AttrURL) {
		ex++;
	}

	{
		wchar_t *url_w = BuffGetStringForCB(sx, y, ex, y, FALSE, NULL);
		char *url = ToCharW(url_w);
		invokeBrowserWithUrl(url);
		free(url);
		free(url_w);
	}
}
#endif

void ChangeSelectRegion()
{
	POINT TempStart, TempEnd;
	int j, IStart, IEnd;
	BOOL Caret;

	if ((SelectEndOld.x==SelectEnd.x) &&
	    (SelectEndOld.y==SelectEnd.y)) {
		return;
	}

	if (BoxSelect) {
		GetMinMax(SelectStart.x,SelectEndOld.x,SelectEnd.x,
		          (int *)&TempStart.x,(int *)&TempEnd.x);
		GetMinMax(SelectStart.y,SelectEndOld.y,SelectEnd.y,
		          (int *)&TempStart.y,(int *)&TempEnd.y);
		TempEnd.x--;
		Caret = IsCaretOn();
		if (Caret) {
			CaretOff();
		}
		DispInitDC();
		BuffUpdateRect(TempStart.x,TempStart.y-PageStart,
		               TempEnd.x,TempEnd.y-PageStart);
		DispReleaseDC();
		if (Caret) {
			CaretOn();
		}
		SelectEndOld = SelectEnd;
		return;
	}

	if ((SelectEndOld.y < SelectEnd.y) ||
	    (SelectEndOld.y==SelectEnd.y) &&
	    (SelectEndOld.x<=SelectEnd.x)) {
		TempStart = SelectEndOld;
		TempEnd.x = SelectEnd.x-1;
		TempEnd.y = SelectEnd.y;
	}
	else {
		TempStart = SelectEnd;
		TempEnd.x = SelectEndOld.x-1;
		TempEnd.y = SelectEndOld.y;
	}
	if (TempEnd.x < 0) {
		TempEnd.x = NumOfColumns - 1;
		TempEnd.y--;
	}

	Caret = IsCaretOn();
	if (Caret) {
		CaretOff();
	}
	for (j = TempStart.y ; j <= TempEnd.y ; j++) {
		IStart = 0;
		IEnd = NumOfColumns-1;
		if (j==TempStart.y) {
			IStart = TempStart.x;
		}
		if (j==TempEnd.y) {
			IEnd = TempEnd.x;
		}

		if ((IEnd>=IStart) && (j >= PageStart+WinOrgY) &&
		    (j < PageStart+WinOrgY+WinHeight)) {
			DispInitDC();
			BuffUpdateRect(IStart,j-PageStart,IEnd,j-PageStart);
			DispReleaseDC();
		}
	}
	if (Caret) {
		CaretOn();
	}

	SelectEndOld = SelectEnd;
}

BOOL BuffUrlDblClk(int Xw, int Yw)
{
	int X, Y;
	LONG TmpPtr;
	BOOL url_invoked = FALSE;

	if (! ts.EnableClickableUrl) {
		return FALSE;
	}

	CaretOff();

	DispConvWinToScreen(Xw,Yw,&X,&Y,NULL);
	Y = Y + PageStart;
	if ((Y<0) || (Y>=BuffEnd)) {
		return 0;
	}
	if (X<0) X = 0;
	if (X>=NumOfColumns) {
		X = NumOfColumns-1;
	}

	if ((Y>=0) && (Y<BuffEnd)) {
		LockBuffer();
		TmpPtr = GetLinePtr(Y);
		/* start - ishizaki */
		if (AttrBuff[TmpPtr+X] & AttrURL) {
			BoxSelect = FALSE;
			SelectEnd = SelectStart;
			ChangeSelectRegion();

			url_invoked = TRUE;
#if UNICODE_INTERNAL_BUFF
			invokeBrowserW(X, Y);
#else
			invokeBrowser(TmpPtr+X);
#endif

			SelectStart.x = 0;
			SelectStart.y = 0;
			SelectEnd.x = 0;
			SelectEnd.y = 0;
			SelectEndOld.x = 0;
			SelectEndOld.y = 0;
			Selected = FALSE;
		}
		UnlockBuffer();
	}
	return url_invoked;
}

void BuffDblClk(int Xw, int Yw)
//  Select a word at (Xw, Yw) by mouse double click
//    Xw: horizontal position in window coordinate (pixels)
//    Yw: vertical
{
	int X, Y, YStart, YEnd;
	int IStart, IEnd, i;
	LONG TmpPtr;
	BYTE b;
	BOOL DBCS;

	CaretOff();

	if (!Selecting) {
		SelectStart = SelectStartTmp;
		Selecting = TRUE;
	}

	DispConvWinToScreen(Xw,Yw,&X,&Y,NULL);
	Y = Y + PageStart;
	if ((Y<0) || (Y>=BuffEnd)) {
		return;
	}
	if (X<0) X = 0;
	if (X>=NumOfColumns) X = NumOfColumns-1;

	BoxSelect = FALSE;
	LockBuffer();
	SelectEnd = SelectStart;
	ChangeSelectRegion();

	if ((Y>=0) && (Y<BuffEnd)) {
		TmpPtr = GetLinePtr(Y);

		IStart = X;
		IStart = LeftHalfOfDBCS(TmpPtr,IStart);
		IEnd = IStart;
		YStart = YEnd = Y;

		if (IsDelimiter(TmpPtr,IStart)) {
			b = CodeBuff[TmpPtr+IStart];
			DBCS = (AttrBuff[TmpPtr+IStart] & AttrKanji) != 0;
			while ((b==CodeBuff[TmpPtr+IStart]) ||
			       DBCS &&
			       ((AttrBuff[TmpPtr+IStart] & AttrKanji)!=0)) {
				MoveCharPtr(TmpPtr,&IStart,-1); // move left
				if (ts.EnableContinuedLineCopy) {
					if (IStart<=0) {
						// 左端の場合
						if (YStart>0 && AttrBuff[TmpPtr] & AttrLineContinued) {
							// 前の行に移動する
							YStart--;
							TmpPtr = GetLinePtr(YStart);
							IStart = NumOfColumns;
						}
						else {
							break;
						}
					}
				}
				else {
					if (IStart<=0) {
						// 左端の場合は終わり
						break;
					}
				}
			}
			if ((b!=CodeBuff[TmpPtr+IStart]) &&
			    ! (DBCS && ((AttrBuff[TmpPtr+IStart] & AttrKanji)!=0))) {
				// 最終位置が Delimiter でない場合にはひとつ右にずらす
				if (ts.EnableContinuedLineCopy && IStart == NumOfColumns-1) {
					// 右端の場合には次の行へ移動する
					YStart++;
					TmpPtr = GetLinePtr(YStart);
					IStart = 0;
				}
				else {
					MoveCharPtr(TmpPtr,&IStart,1);
				}
			}

			// 行が移動しているかもしれないので、クリックした行を取り直す
			TmpPtr = GetLinePtr(YEnd);
			i = 1;
			while (((b==CodeBuff[TmpPtr+IEnd]) ||
			        DBCS &&
			        ((AttrBuff[TmpPtr+IEnd] & AttrKanji)!=0))) {
				i = MoveCharPtr(TmpPtr,&IEnd,1); // move right
				if (ts.EnableContinuedLineCopy) {
					if (i==0) {
						// 右端の場合
						if (YEnd<BuffEnd &&
						    AttrBuff[TmpPtr+IEnd+1+DBCS] & AttrLineContinued) {
							// 次の行に移動する
							YEnd++;
							TmpPtr = GetLinePtr(YEnd);
							IEnd = 0;
						}
						else {
							break;
						}
					}
				}
				else {
					if (i==0) {
						// 右端の場合は終わり
						break;
					}
				}
			}
		}
		else {
			while (! IsDelimiter(TmpPtr,IStart)) {
				MoveCharPtr(TmpPtr,&IStart,-1); // move left
				if (ts.EnableContinuedLineCopy) {
					if (IStart<=0) {
						// 左端の場合
						if (YStart>0 && AttrBuff[TmpPtr] & AttrLineContinued) {
							// 前の行に移動する
							YStart--;
							TmpPtr = GetLinePtr(YStart);
							IStart = NumOfColumns;
						}
						else {
							break;
						}
					}
				}
				else {
					if (IStart<=0) {
						// 左端の場合は終わり
						break;
					}
				}
			}
			if (IsDelimiter(TmpPtr,IStart)) {
				// 最終位置が Delimiter の場合にはひとつ右にずらす
				if (ts.EnableContinuedLineCopy && IStart == NumOfColumns-1) {
					// 右端の場合には次の行へ移動する
					YStart++;
					TmpPtr = GetLinePtr(YStart);
					IStart = 0;
				}
				else {
					MoveCharPtr(TmpPtr,&IStart,1);
				}
			}

			// 行が移動しているかもしれないので、クリックした行を取り直す
			TmpPtr = GetLinePtr(YEnd);
			i = 1;
			while (! IsDelimiter(TmpPtr,IEnd)) {
				i = MoveCharPtr(TmpPtr,&IEnd,1); // move right
				if (ts.EnableContinuedLineCopy) {
					if (i==0) {
						// 右端の場合
						if (YEnd<BuffEnd && AttrBuff[TmpPtr+IEnd+1] & AttrLineContinued) {
							// 次の行に移動する
							YEnd++;
							TmpPtr = GetLinePtr(YEnd);
							IEnd = 0;
						}
						else {
							break;
						}
					}
				}
				else {
					if (i==0) {
						// 右端の場合は終わり
						break;
					}
				}
			}
		}
		if (ts.EnableContinuedLineCopy) {
			if (IEnd == 0) {
				// 左端の場合には前の行へ移動する
				YEnd--;
				IEnd = NumOfColumns;
			}
			else if (i==0) {
				IEnd = NumOfColumns;
			}
		}
		else {
			if (i==0)
				IEnd = NumOfColumns;
		}

		SelectStart.x = IStart;
		SelectStart.y = YStart;
		SelectEnd.x = IEnd;
		SelectEnd.y = YEnd;
		SelectEndOld = SelectStart;
		DblClkStart = SelectStart;
		DblClkEnd = SelectEnd;
		Selected = TRUE;
		ChangeSelectRegion();
	}
	UnlockBuffer();
	return;
}

void BuffTplClk(int Yw)
//  Select a line at Yw by mouse tripple click
//    Yw: vertical clicked position
//			in window coordinate (pixels)
{
	int Y;

	CaretOff();

	DispConvWinToScreen(0,Yw,NULL,&Y,NULL);
	Y = Y + PageStart;
	if ((Y<0) || (Y>=BuffEnd)) {
		return;
	}

	LockBuffer();
	SelectEnd = SelectStart;
	ChangeSelectRegion();
	SelectStart.x = 0;
	SelectStart.y = Y;
	SelectEnd.x = NumOfColumns;
	SelectEnd.y = Y;
	SelectEndOld = SelectStart;
	DblClkStart = SelectStart;
	DblClkEnd = SelectEnd;
	Selected = TRUE;
	ChangeSelectRegion();
	UnlockBuffer();
}


// The block of the text between old and new cursor positions is being selected.
// This function enables to select several pages of output from Tera Term window.
// add (2005.5.15 yutaka)
void BuffSeveralPagesSelect(int Xw, int Yw)
//  Start text selection by mouse button down
//    Xw: horizontal position in window coordinate (pixels)
//    Yw: vertical
{
	int X, Y;
	BOOL Right;

	DispConvWinToScreen(Xw,Yw, &X,&Y,&Right);
	Y = Y + PageStart;
	if ((Y<0) || (Y>=BuffEnd)) {
		return;
	}
	if (X<0) X = 0;
	if (X>=NumOfColumns) {
		X = NumOfColumns-1;
	}

	SelectEnd.x = X;
	SelectEnd.y = Y;
	//BoxSelect = FALSE; // box selecting disabled
	SeveralPageSelect = TRUE;
}

void BuffStartSelect(int Xw, int Yw, BOOL Box)
//  Start text selection by mouse button down
//    Xw: horizontal position in window coordinate (pixels)
//    Yw: vertical
//    Box: Box selection if TRUE
{
	int X, Y;
	BOOL Right;
	LONG TmpPtr;

	DispConvWinToScreen(Xw,Yw, &X,&Y,&Right);
	Y = Y + PageStart;
	if ((Y<0) || (Y>=BuffEnd)) {
		return;
	}
	if (X<0) X = 0;
	if (X>=NumOfColumns) {
		X = NumOfColumns-1;
	}

	SelectEndOld = SelectEnd;
	SelectEnd = SelectStart;

	LockBuffer();
	ChangeSelectRegion();
	UnlockBuffer();

	SelectStartTime = GetTickCount();
	Selecting = FALSE;

#define range_check(v, min, max)	((v)<(min) ? (min) : (v)>(max) ? (max) : (v))
	SelectStartTmp.x = range_check(X, 0, NumOfColumns);
	SelectStartTmp.y = range_check(Y, 0, BuffEnd-1);

	TmpPtr = GetLinePtr(SelectStart.y);
	// check if the cursor is on the right half of a character
	if ((SelectStartTmp.x>0) &&
	    ((AttrBuff[TmpPtr+SelectStartTmp.x-1] & AttrKanji) != 0) ||
	    ((AttrBuff[TmpPtr+SelectStartTmp.x] & AttrKanji) == 0) &&
	     Right) {
		SelectStartTmp.x++;
	}

	SelectEnd = SelectStartTmp;
	SelectEndOld = SelectEnd;
	CaretOff();
	Selected = TRUE;
	BoxSelect = Box;
}

void BuffChangeSelect(int Xw, int Yw, int NClick)
//  Change selection region by mouse move
//    Xw: horizontal position of the mouse cursor
//			in window coordinate
//    Yw: vertical
{
	int X, Y;
	BOOL Right;
	LONG TmpPtr;
	int i;
	BYTE b;
	BOOL DBCS;

	if (!Selecting) {
		if (GetTickCount() - SelectStartTime < ts.SelectStartDelay) {
			return;
		}
		SelectStart = SelectStartTmp;
		Selecting = TRUE;
	}

	DispConvWinToScreen(Xw,Yw,&X,&Y,&Right);
	Y = Y + PageStart;

	if (X<0) X = 0;
	if (X > NumOfColumns) {
		X = NumOfColumns;
	}
	if (Y < 0) Y = 0;
	if (Y >= BuffEnd) {
		Y = BuffEnd - 1;
	}

	TmpPtr = GetLinePtr(Y);
	LockBuffer();
	// check if the cursor is on the right half of a character
	if ((X>0) &&
	    ((AttrBuff[TmpPtr+X-1] & AttrKanji) != 0) ||
	    (X<NumOfColumns) &&
	    ((AttrBuff[TmpPtr+X] & AttrKanji) == 0) &&
	    Right) {
		X++;
	}

	if (X > NumOfColumns) {
		X = NumOfColumns;
	}

#if 0
	/* start - ishizaki */
	if (ts.EnableClickableUrl && (NClick == 2) && (AttrBuff[TmpPtr+X] & AttrURL)) {
		invokeBrowser(TmpPtr+X);

		SelectStart.x = 0;
		SelectStart.y = 0;
		SelectEnd.x = 0;
		SelectEnd.y = 0;
		SelectEndOld.x = 0;
		SelectEndOld.y = 0;
		Selected = FALSE;
		goto end;
	}
	/* end - ishizaki */
#endif

	SelectEnd.x = X;
	SelectEnd.y = Y;

	if (NClick==2) { // drag after double click
		if ((SelectEnd.y>SelectStart.y) ||
		    (SelectEnd.y==SelectStart.y) &&
		    (SelectEnd.x>=SelectStart.x)) {
			if (SelectStart.x==DblClkEnd.x) {
				SelectEnd = DblClkStart;
				ChangeSelectRegion();
				SelectStart = DblClkStart;
				SelectEnd.x = X;
				SelectEnd.y = Y;
			}
			MoveCharPtr(TmpPtr,&X,-1);
			if (X<SelectStart.x) {
				X = SelectStart.x;
			}

			i = 1;
			if (IsDelimiter(TmpPtr,X)) {
				b = CodeBuff[TmpPtr+X];
				DBCS = (AttrBuff[TmpPtr+X] & AttrKanji) != 0;
				while ((i!=0) &&
				       ((b==CodeBuff[TmpPtr+SelectEnd.x]) ||
				        DBCS &&
				        ((AttrBuff[TmpPtr+SelectEnd.x] & AttrKanji)!=0))) {
					i = MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,1); // move right
				}
			}
			else {
				while ((i!=0) &&
				       ! IsDelimiter(TmpPtr,SelectEnd.x)) {
					i = MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,1); // move right
				}
			}
			if (i==0) {
				SelectEnd.x = NumOfColumns;
			}
		}
		else {
			if (SelectStart.x==DblClkStart.x) {
				SelectEnd = DblClkEnd;
				ChangeSelectRegion();
				SelectStart = DblClkEnd;
				SelectEnd.x = X;
				SelectEnd.y = Y;
			}
			if (IsDelimiter(TmpPtr,SelectEnd.x)) {
				b = CodeBuff[TmpPtr+SelectEnd.x];
				DBCS = (AttrBuff[TmpPtr+SelectEnd.x] & AttrKanji) != 0;
				while ((SelectEnd.x>0) &&
				       ((b==CodeBuff[TmpPtr+SelectEnd.x]) ||
				       DBCS &&
				       ((AttrBuff[TmpPtr+SelectEnd.x] & AttrKanji)!=0))) {
					MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,-1); // move left
				}
				if ((b!=CodeBuff[TmpPtr+SelectEnd.x]) &&
				    ! (DBCS &&
				    ((AttrBuff[TmpPtr+SelectEnd.x] & AttrKanji)!=0))) {
					MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,1);
				}
			}
			else {
				while ((SelectEnd.x>0) &&
				       ! IsDelimiter(TmpPtr,SelectEnd.x)) {
					MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,-1); // move left
				}
				if (IsDelimiter(TmpPtr,SelectEnd.x)) {
					MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,1);
				}
			}
		}
	}
	else if (NClick==3) { // drag after tripple click
		if ((SelectEnd.y>SelectStart.y) ||
		    (SelectEnd.y==SelectStart.y) &&
		    (SelectEnd.x>=SelectStart.x)) {
			if (SelectStart.x==DblClkEnd.x) {
				SelectEnd = DblClkStart;
				ChangeSelectRegion();
				SelectStart = DblClkStart;
				SelectEnd.x = X;
				SelectEnd.y = Y;
			}
			SelectEnd.x = NumOfColumns;
		}
		else {
			if (SelectStart.x==DblClkStart.x) {
				SelectEnd = DblClkEnd;
				ChangeSelectRegion();
				SelectStart = DblClkEnd;
				SelectEnd.x = X;
				SelectEnd.y = Y;
			}
			SelectEnd.x = 0;
		}
	}

#if 0
	/* start - ishizaki */
end:
	/* end - ishizaki */
#endif

	ChangeSelectRegion();
	UnlockBuffer();
}

void BuffEndSelect()
//  End text selection by mouse button up
{
	if (!Selecting) {
		if (GetTickCount() - SelectStartTime < ts.SelectStartDelay) {
			SelectEnd = SelectStart;
			SelectEndOld = SelectEnd;
			LockBuffer();
			ChangeSelectRegion();
			UnlockBuffer();
			Selected = FALSE;
			return;
		}
		SelectStart = SelectStartTmp;
	}

	Selected = (SelectStart.x!=SelectEnd.x) ||
	           (SelectStart.y!=SelectEnd.y);
	if (Selected) {
		if (BoxSelect) {
			if (SelectStart.x>SelectEnd.x) {
				SelectEndOld.x = SelectStart.x;
				SelectStart.x = SelectEnd.x;
				SelectEnd.x = SelectEndOld.x;
			}
			if (SelectStart.y>SelectEnd.y) {
				SelectEndOld.y = SelectStart.y;
				SelectStart.y = SelectEnd.y;
				SelectEnd.y = SelectEndOld.y;
			}
		}
		else if ((SelectEnd.y < SelectStart.y) ||
		         (SelectEnd.y == SelectStart.y) &&
		          (SelectEnd.x < SelectStart.x)) {
			SelectEndOld = SelectStart;
			SelectStart = SelectEnd;
			SelectEnd = SelectEndOld;
		}

		if (SeveralPageSelect) { // yutaka
			// ページをまたぐ選択の場合、Mouse button up時にリージョンを塗り替える。
			LockBuffer();
			ChangeSelectRegion();
			UnlockBuffer();
			SeveralPageSelect = FALSE;
			InvalidateRect(HVTWin, NULL, TRUE); // ちょっと画面がちらつく
		}

		/* copy to the clipboard */
		if (ts.AutoTextCopy>0) {
			LockBuffer();
#if UNICODE_INTERNAL_BUFF
			BuffCBCopyUnicode(FALSE);
#else
			BuffCBCopy(FALSE);
#endif
			UnlockBuffer();
		}
	}
}

void BuffChangeWinSize(int Nx, int Ny)
// Change window size
//   Nx: new window width (number of characters)
//   Ny: new window hight
{
	if (Nx==0) {
		Nx = 1;
	}
	if (Ny==0) {
		Ny = 1;
	}

	if ((ts.TermIsWin>0) &&
	    ((Nx!=NumOfColumns) || (Ny!=NumOfLines))) {
		LockBuffer();
		BuffChangeTerminalSize(Nx,Ny-StatusLine);
		UnlockBuffer();
		Nx = NumOfColumns;
		Ny = NumOfLines;
	}
	if (Nx>NumOfColumns) {
		Nx = NumOfColumns;
	}
	if (Ny>BuffEnd) {
		Ny = BuffEnd;
	}
	DispChangeWinSize(Nx,Ny);
}

void BuffChangeTerminalSize(int Nx, int Ny)
{
	int i, Nb, W, H;
	BOOL St;

	Ny = Ny + StatusLine;
	if (Nx < 1) {
		Nx = 1;
	}
	if (Ny < 1) {
		Ny = 1;
	}
	if (Nx > BuffXMax) {
		Nx = BuffXMax;
	}
	if (ts.ScrollBuffMax > BuffYMax) {
		ts.ScrollBuffMax = BuffYMax;
	}
	if (Ny > ts.ScrollBuffMax) {
		Ny = ts.ScrollBuffMax;
	}

	St = isCursorOnStatusLine;
	if ((Nx!=NumOfColumns) || (Ny!=NumOfLines)) {
		if ((ts.ScrollBuffSize < Ny) ||
		    (ts.EnableScrollBuff==0)) {
			Nb = Ny;
		}
		else {
			Nb = ts.ScrollBuffSize;
		}

		if (! ChangeBuffer(Nx,Nb)) {
			return;
		}
		if (ts.EnableScrollBuff>0) {
			ts.ScrollBuffSize = NumOfLinesInBuff;
		}
		if (Ny > NumOfLinesInBuff) {
			Ny = NumOfLinesInBuff;
		}

		if ((ts.TermFlag & TF_CLEARONRESIZE) == 0 && Ny != NumOfLines) {
			if (Ny > NumOfLines) {
				CursorY += Ny - NumOfLines;
				if (Ny > BuffEnd) {
					CursorY -= Ny - BuffEnd;
					BuffEnd = Ny;
				}
			}
			else {
				if (Ny  > CursorY + StatusLine + 1) {
					BuffEnd -= NumOfLines - Ny;
				}
				else {
					BuffEnd -= NumOfLines - 1 - StatusLine - CursorY;
					CursorY = Ny - 1 - StatusLine;
				}
			}
		}

		NumOfColumns = Nx;
		NumOfLines = Ny;
		ts.TerminalWidth = Nx;
		ts.TerminalHeight = Ny-StatusLine;

		PageStart = BuffEnd - NumOfLines;
	}

	if (ts.TermFlag & TF_CLEARONRESIZE) {
		BuffScroll(NumOfLines,NumOfLines-1);
	}

	/* Set Cursor */
	if (ts.TermFlag & TF_CLEARONRESIZE) {
		CursorX = 0;
		CursorRightM = NumOfColumns-1;
		if (St) {
			CursorY = NumOfLines-1;
			CursorTop = CursorY;
			CursorBottom = CursorY;
		}
		else {
			CursorY = 0;
			CursorTop = 0;
			CursorBottom = NumOfLines-1-StatusLine;
		}
	}
	else {
		CursorRightM = NumOfColumns-1;
		if (CursorX >= NumOfColumns) {
			CursorX = NumOfColumns - 1;
		}
		if (St) {
			CursorY = NumOfLines-1;
			CursorTop = CursorY;
			CursorBottom = CursorY;
		}
		else {
			if (CursorY >= NumOfLines - StatusLine) {
				CursorY = NumOfLines - 1 - StatusLine;
			}
			CursorTop = 0;
			CursorBottom = NumOfLines - 1 - StatusLine;
		}
	}
	CursorLeftM = 0;

	SelectStart.x = 0;
	SelectStart.y = 0;
	SelectEnd = SelectStart;
	Selected = FALSE;

	/* Tab stops */
	NTabStops = (NumOfColumns-1) >> 3;
	for (i = 1 ; i <= NTabStops ; i++) {
		TabStops[i-1] = i*8;
	}

	if (ts.TermIsWin>0) {
		W = NumOfColumns;
		H = NumOfLines;
	}
	else {
		W = WinWidth;
		H = WinHeight;
		if ((ts.AutoWinResize>0) ||
		    (NumOfColumns < W)) {
			W = NumOfColumns;
		}
		if (ts.AutoWinResize>0) {
			H = NumOfLines;
		}
		else if (BuffEnd < H) {
			H = BuffEnd;
		}
	}

	NewLine(PageStart+CursorY);

	/* Change Window Size */
	BuffChangeWinSize(W,H);
	WinOrgY = -NumOfLines;

	DispScrollHomePos();

	if (cv.Ready && cv.TelFlag) {
		TelInformWinSize(NumOfColumns,NumOfLines-StatusLine);
	}

	TTXSetWinSize(NumOfLines-StatusLine, NumOfColumns); /* TTPLUG */
}

void ChangeWin()
{
	int Ny;

	/* Change buffer */
	if (ts.EnableScrollBuff>0) {
		if (ts.ScrollBuffSize < NumOfLines) {
			ts.ScrollBuffSize = NumOfLines;
		}
		Ny = ts.ScrollBuffSize;
	}
	else {
		Ny = NumOfLines;
	}

	if (NumOfLinesInBuff!=Ny) {
		ChangeBuffer(NumOfColumns,Ny);
		if (ts.EnableScrollBuff>0) {
			ts.ScrollBuffSize = NumOfLinesInBuff;
		}

		if (BuffEnd < WinHeight) {
			BuffChangeWinSize(WinWidth,BuffEnd);
		}
		else {
			BuffChangeWinSize(WinWidth,WinHeight);
		}
	}

	DispChangeWin();
}

void ClearBuffer()
{
	/* Reset buffer */
	PageStart = 0;
	BuffStartAbs = 0;
	BuffEnd = NumOfLines;
	if (NumOfLines==NumOfLinesInBuff) {
		BuffEndAbs = 0;
	}
	else {
		BuffEndAbs = NumOfLines;
	}

	SelectStart.x = 0;
	SelectStart.y = 0;
	SelectEnd = SelectStart;
	SelectEndOld = SelectStart;
	Selected = FALSE;

	NewLine(0);
	memset(&CodeBuff[0],0x20,BufferSize);
#if UNICODE_INTERNAL_BUFF
	memsetW(&CodeBuffW[0],0x20,BufferSize);
#endif
	memset(&AttrBuff[0],AttrDefault,BufferSize);
	memset(&AttrBuff2[0],CurCharAttr.Attr2 & Attr2ColorMask, BufferSize);
	memset(&AttrBuffFG[0],CurCharAttr.Fore,BufferSize);
	memset(&AttrBuffBG[0],CurCharAttr.Back,BufferSize);

	/* Home position */
	CursorX = 0;
	CursorY = 0;
	WinOrgX = 0;
	WinOrgY = 0;
	NewOrgX = 0;
	NewOrgY = 0;

	/* Top/bottom margin */
	CursorTop = 0;
	CursorBottom = NumOfLines - 1;
	CursorLeftM = 0;
	CursorRightM = NumOfColumns - 1;

	StrChangeCount = 0;

	DispClearWin();
}

void SetTabStop()
{
	int i,j;

	if (NTabStops<NumOfColumns) {
		i = 0;
		while ((TabStops[i]<CursorX) && (i<NTabStops)) {
			i++;
		}

		if ((i<NTabStops) && (TabStops[i]==CursorX)) {
			return;
		}

		for (j=NTabStops ; j>=i+1 ; j--) {
			TabStops[j] = TabStops[j-1];
		}
		TabStops[i] = CursorX;
		NTabStops++;
	}
}

void CursorForwardTab(int count, BOOL AutoWrapMode) {
	int i, LineEnd;
	BOOL WrapState;

	WrapState = Wrap;

	if (CursorX > CursorRightM || CursorY < CursorTop || CursorY > CursorBottom)
		LineEnd = NumOfColumns - 1;
	else
		LineEnd = CursorRightM;

	for (i=0; i<NTabStops && TabStops[i] <= CursorX; i++)
		;

	i += count - 1;

	if (i < NTabStops && TabStops[i] <= LineEnd) {
		MoveCursor(TabStops[i], CursorY);
	}
	else {
		MoveCursor(LineEnd, CursorY);
		if (!ts.VTCompatTab) {
			Wrap = AutoWrapMode;
		}
		else {
			Wrap = WrapState;
		}
	}
}

void CursorBackwardTab(int count) {
	int i, LineStart;

	if (CursorX < CursorLeftM || CursorY < CursorTop || CursorY > CursorBottom)
		LineStart = 0;
	else
		LineStart = CursorLeftM;

	for (i=0; i<NTabStops && TabStops[i] < CursorX; i++)
		;

	if (i < count || TabStops[i-count] < LineStart) {
		MoveCursor(LineStart, CursorY);
	}
	else {
		MoveCursor(TabStops[i-count], CursorY);
	}
}

void ClearTabStop(int Ps)
// Clear tab stops
//   Ps = 0: clear the tab stop at cursor
//      = 3: clear all tab stops
{
	int i,j;

	if (NTabStops>0) {
		switch (Ps) {
		case 0:
			if (ts.TabStopFlag & TABF_TBC0) {
				i = 0;
				while ((TabStops[i]!=CursorX) && (i<NTabStops-1)) {
					i++;
				}
				if (TabStops[i] == CursorX) {
					NTabStops--;
					for (j=i ; j<=NTabStops ; j++) {
						TabStops[j] = TabStops[j+1];
					}
				}
			}
			break;
		case 3:
			if (ts.TabStopFlag & TABF_TBC3)
				NTabStops = 0;
			break;
		}
	}
}

void ShowStatusLine(int Show)
// show/hide status line
{
	int Ny, Nb, W, H;

	BuffUpdateScroll();
	if (Show==StatusLine) {
		return;
	}
	StatusLine = Show;

	if (StatusLine==0) {
		NumOfLines--;
		BuffEnd--;
		BuffEndAbs=PageStart+NumOfLines;
		if (BuffEndAbs >= NumOfLinesInBuff) {
			BuffEndAbs = BuffEndAbs-NumOfLinesInBuff;
		}
		Ny = NumOfLines;
	}
	else {
		Ny = ts.TerminalHeight+1;
	}

	if ((ts.ScrollBuffSize < Ny) ||
	    (ts.EnableScrollBuff==0)) {
		Nb = Ny;
	}
	else {
		Nb = ts.ScrollBuffSize;
	}

	if (! ChangeBuffer(NumOfColumns,Nb)) {
		return;
	}
	if (ts.EnableScrollBuff>0) {
		ts.ScrollBuffSize = NumOfLinesInBuff;
	}
	if (Ny > NumOfLinesInBuff) {
		Ny = NumOfLinesInBuff;
	}

	NumOfLines = Ny;
	ts.TerminalHeight = Ny-StatusLine;

	if (StatusLine==1) {
		BuffScroll(1,NumOfLines-1);
	}

	if (ts.TermIsWin>0) {
		W = NumOfColumns;
		H = NumOfLines;
	}
	else {
		W = WinWidth;
		H = WinHeight;
		if ((ts.AutoWinResize>0) ||
		    (NumOfColumns < W)) {
			W = NumOfColumns;
		}
		if (ts.AutoWinResize>0) {
			H = NumOfLines;
		}
		else if (BuffEnd < H) {
			H = BuffEnd;
		}
	}

	PageStart = BuffEnd-NumOfLines;
	NewLine(PageStart+CursorY);

	/* Change Window Size */
	BuffChangeWinSize(W,H);
	WinOrgY = -NumOfLines;
	DispScrollHomePos();

	MoveCursor(CursorX,CursorY);
}

void BuffLineContinued(BOOL mode)
{
	if (ts.EnableContinuedLineCopy) {
		if (mode) {
			AttrLine[0] |= AttrLineContinued;
		} else {
			AttrLine[0] &= ~AttrLineContinued;
		}
	}
}

void BuffSetCurCharAttr(TCharAttr Attr) {
	CurCharAttr = Attr;
	DispSetCurCharAttr(Attr);
}

void BuffSaveScreen()
{
	PCHAR CodeDest, AttrDest, AttrDest2, AttrDestFG, AttrDestBG;
#if UNICODE_INTERNAL_BUFF
	buff_char_t *CodeDestW;
#endif
	LONG ScrSize;
	size_t AllocSize;
	LONG SrcPtr, DestPtr;
	int i;

	if (SaveBuff == NULL) {
		ScrSize = NumOfColumns * NumOfLines;	// 1画面分のバッファの保存数
#if UNICODE_INTERNAL_BUFF
		// 全画面分のバイト数
		AllocSize = ScrSize * (5+sizeof(buff_char_t));
#else
		AllocSize = ScrSize * 5;
#endif
		SaveBuff = malloc(AllocSize);
		if (SaveBuff != NULL) {
			CodeDest = SaveBuff;
			AttrDest = CodeDest + ScrSize;
			AttrDest2 = AttrDest + ScrSize;
			AttrDestFG = AttrDest2 + ScrSize;
			AttrDestBG = AttrDestFG + ScrSize;
#if UNICODE_INTERNAL_BUFF
			CodeDestW = (buff_char_t *)(AttrDestBG + ScrSize);
#endif

			SaveBuffX = NumOfColumns;
			SaveBuffY = NumOfLines;

			SrcPtr = GetLinePtr(PageStart);
			DestPtr = 0;

			for (i=0; i<NumOfLines; i++) {
				memcpy(&CodeDest[DestPtr], &CodeBuff[SrcPtr], NumOfColumns);
#if UNICODE_INTERNAL_BUFF
				memcpyW(&CodeDestW[DestPtr], &CodeBuffW[SrcPtr], NumOfColumns);
#endif
				memcpy(&AttrDest[DestPtr], &AttrBuff[SrcPtr], NumOfColumns);
				memcpy(&AttrDest2[DestPtr], &AttrBuff2[SrcPtr], NumOfColumns);
				memcpy(&AttrDestFG[DestPtr], &AttrBuffFG[SrcPtr], NumOfColumns);
				memcpy(&AttrDestBG[DestPtr], &AttrBuffBG[SrcPtr], NumOfColumns);
				SrcPtr = NextLinePtr(SrcPtr);
				DestPtr += NumOfColumns;
			}
		}
	}
	_CrtCheckMemory();
}

void BuffRestoreScreen()
{
	PCHAR CodeSrc, AttrSrc, AttrSrc2, AttrSrcFG, AttrSrcBG;
#if UNICODE_INTERNAL_BUFF
	buff_char_t *CodeSrcW;
#endif
	LONG ScrSize;
	LONG SrcPtr, DestPtr;
	int i, CopyX, CopyY;

	_CrtCheckMemory();
	if (SaveBuff != NULL) {
		CodeSrc = SaveBuff;
		ScrSize = SaveBuffX * SaveBuffY;

		AttrSrc = CodeSrc + ScrSize;
		AttrSrc2 = AttrSrc + ScrSize;
		AttrSrcFG = AttrSrc2 + ScrSize;
		AttrSrcBG = AttrSrcFG + ScrSize;
#if UNICODE_INTERNAL_BUFF
		CodeSrcW = (buff_char_t*)(AttrSrcBG + ScrSize);
#endif

		CopyX = (SaveBuffX > NumOfColumns) ? NumOfColumns : SaveBuffX;
		CopyY = (SaveBuffY > NumOfLines) ? NumOfLines : SaveBuffY;

#if UNICODE_INTERNAL_BUFF
		DestPtr = GetLinePtr(PageStart);
		for (i=0; i<CopyY; i++) {
			buff_char_t *p = &CodeBuffW[DestPtr];
			int j;
			for (j=0; j<CopyX; j++) {
				if (p->CombinationCharSize16 > 0) {
					free(p->pCombinationChars16);
				}
				if (p->CombinationCharSize32 > 0) {
					free(p->pCombinationChars32);
				}
			}
			DestPtr = NextLinePtr(DestPtr);
		}
#endif

		SrcPtr = 0;
		DestPtr = GetLinePtr(PageStart);

		for (i=0; i<CopyY; i++) {
			memcpy(&CodeBuff[DestPtr], &CodeSrc[SrcPtr], CopyX);
#if UNICODE_INTERNAL_BUFF
			memcpyW(&CodeBuffW[DestPtr], &CodeSrcW[SrcPtr], CopyX);
#endif
			memcpy(&AttrBuff[DestPtr], &AttrSrc[SrcPtr], CopyX);
			memcpy(&AttrBuff2[DestPtr], &AttrSrc2[SrcPtr], CopyX);
			memcpy(&AttrBuffFG[DestPtr], &AttrSrcFG[SrcPtr], CopyX);
			memcpy(&AttrBuffBG[DestPtr], &AttrSrcBG[SrcPtr], CopyX);
			if (AttrBuff[DestPtr+CopyX-1] & AttrKanji) {
				CodeBuff[DestPtr+CopyX-1] = ' ';
#if UNICODE_INTERNAL_BUFF
				BuffSetChar(&CodeBuffW[DestPtr+CopyX-1], 0x20, 'H');
#endif
				AttrBuff[DestPtr+CopyX-1] ^= AttrKanji;
			}
			SrcPtr += SaveBuffX;
			DestPtr = NextLinePtr(DestPtr);
		}
		BuffUpdateRect(WinOrgX,WinOrgY,WinOrgX+WinWidth-1,WinOrgY+WinHeight-1);

		free(SaveBuff);
		SaveBuff = NULL;
	}
	_CrtCheckMemory();
}

void BuffDiscardSavedScreen()
{
	if (SaveBuff != NULL) {
		free(SaveBuff);
		SaveBuff = NULL;
	}
	_CrtCheckMemory();
}

void BuffSelectedEraseCurToEnd()
// Erase characters from cursor to the end of screen
{
	LONG TmpPtr;
	int offset;
	int i, j, YEnd;

	NewLine(PageStart+CursorY);
	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		if (!(AttrLine2[CursorX] & Attr2Protect)) {
			EraseKanji(1); /* if cursor is on right half of a kanji, erase the kanji */
		}
	}
	offset = CursorX;
	TmpPtr = GetLinePtr(PageStart+CursorY);
	YEnd = NumOfLines-1;
	if (StatusLine && !isCursorOnStatusLine) {
		YEnd--;
	}
	for (i = CursorY ; i <= YEnd ; i++) {
		for (j = TmpPtr + offset; j < TmpPtr + NumOfColumns - offset; j++) {
			if (!(AttrBuff2[j] & Attr2Protect)) {
				CodeBuff[j] = 0x20;
#if UNICODE_INTERNAL_BUFF
				BuffSetChar(&CodeBuffW[j], 0x20, 'H');
#endif
				AttrBuff[j] &= AttrSgrMask;
			}
		}
		offset = 0;
		TmpPtr = NextLinePtr(TmpPtr);
	}
	/* update window */
	BuffUpdateRect(0, CursorY, NumOfColumns, YEnd);
}

void BuffSelectedEraseHomeToCur()
// Erase characters from home to cursor
{
	LONG TmpPtr;
	int offset;
	int i, j, YHome;

	NewLine(PageStart+CursorY);
	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		if (!(AttrLine2[CursorX] & Attr2Protect)) {
			EraseKanji(0); /* if cursor is on left half of a kanji, erase the kanji */
		}
	}
	offset = NumOfColumns;
	if (isCursorOnStatusLine) {
		YHome = CursorY;
	}
	else {
		YHome = 0;
	}
	TmpPtr = GetLinePtr(PageStart+YHome);
	for (i = YHome ; i <= CursorY ; i++) {
		if (i==CursorY) {
			offset = CursorX+1;
		}
		for (j = TmpPtr; j < TmpPtr + offset; j++) {
			if (!(AttrBuff2[j] & Attr2Protect)) {
				CodeBuff[j] = 0x20;
#if UNICODE_INTERNAL_BUFF
				BuffSetChar(&CodeBuffW[j], 0x20, 'H');
#endif
				AttrBuff[j] &= AttrSgrMask;
			}
		}
		TmpPtr = NextLinePtr(TmpPtr);
	}

	/* update window */
	BuffUpdateRect(0, YHome, NumOfColumns, CursorY);
}

void BuffSelectedEraseScreen() {
	BuffSelectedEraseHomeToCur();
	BuffSelectedEraseCurToEnd();
}

void BuffSelectiveEraseBox
  (int XStart, int YStart, int XEnd, int YEnd)
{
	int C, i, j;
	LONG Ptr;

	if (XEnd>NumOfColumns-1) {
		XEnd = NumOfColumns-1;
	}
	if (YEnd>NumOfLines-1-StatusLine) {
		YEnd = NumOfLines-1-StatusLine;
	}
	if (XStart>XEnd) {
		return;
	}
	if (YStart>YEnd) {
		return;
	}
	C = XEnd-XStart+1;
	Ptr = GetLinePtr(PageStart+YStart);
	for (i=YStart; i<=YEnd; i++) {
		if ((XStart>0) &&
		    ((AttrBuff[Ptr+XStart-1] & AttrKanji) != 0) &&
		    ((AttrBuff2[Ptr+XStart-1] & Attr2Protect) == 0)) {
			CodeBuff[Ptr+XStart-1] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[Ptr+XStart-1], 0x20, 'H');
#endif
			AttrBuff[Ptr+XStart-1] &= AttrSgrMask;
		}
		if ((XStart+C<NumOfColumns) &&
		    ((AttrBuff[Ptr+XStart+C-1] & AttrKanji) != 0) &&
		    ((AttrBuff2[Ptr+XStart+C-1] & Attr2Protect) == 0)) {
			CodeBuff[Ptr+XStart+C] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[Ptr+XStart+C], 0x20, 'H');
#endif
			AttrBuff[Ptr+XStart+C] &= AttrSgrMask;
		}
		for (j=Ptr+XStart; j<Ptr+XStart+C; j++) {
			if (!(AttrBuff2[j] & Attr2Protect)) {
				CodeBuff[j] = 0x20;
#if UNICODE_INTERNAL_BUFF
				BuffSetChar(&CodeBuffW[j], 0x20, 'H');
#endif
				AttrBuff[j] &= AttrSgrMask;
			}
		}
		Ptr = NextLinePtr(Ptr);
	}
	BuffUpdateRect(XStart,YStart,XEnd,YEnd);
}

void BuffSelectedEraseCharsInLine(int XStart, int Count)
// erase non-protected characters in the current line
//  XStart: start position of erasing
//  Count: number of characters to be erased
{
	int i;
	BOOL LineContinued=FALSE;

	if (ts.EnableContinuedLineCopy && XStart == 0 && (AttrLine[0] & AttrLineContinued)) {
		LineContinued = TRUE;
	}

	if (ts.Language==IdJapanese || ts.Language==IdKorean || ts.Language==IdUtf8) {
		if (!(AttrLine2[CursorX] & Attr2Protect)) {
			EraseKanji(1); /* if cursor is on right half of a kanji, erase the kanji */
		}
	}

	NewLine(PageStart+CursorY);
	for (i=XStart; i < XStart + Count; i++) {
		if (!(AttrLine2[i] & Attr2Protect)) {
			CodeLine[i] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeLineW[i], 0x20, 'H');
#endif
			AttrLine[i] &= AttrSgrMask;
		}
	}

	if (ts.EnableContinuedLineCopy) {
		if (LineContinued) {
			BuffLineContinued(TRUE);
		}

		if (XStart + Count >= NumOfColumns) {
			AttrBuff[NextLinePtr(LinePtr)] &= ~AttrLineContinued;
		}
	}

	BuffUpdateRect(XStart, CursorY, XStart+Count, CursorY);
}

void BuffScrollLeft(int count)
{
	int i, MoveLen;
	LONG LPtr, Ptr;

	UpdateStr();

	LPtr = GetLinePtr(PageStart + CursorTop);
	MoveLen = CursorRightM - CursorLeftM + 1 - count;
	for (i = CursorTop; i <= CursorBottom; i++) {
		Ptr = LPtr + CursorLeftM;

		if (AttrBuff[LPtr+CursorRightM] & AttrKanji) {
			CodeBuff[LPtr+CursorRightM] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[LPtr+CursorRightM], 0x20, 'H');
#endif
			AttrBuff[LPtr+CursorRightM] &= ~AttrKanji;
			if (CursorRightM < NumOfColumns-1) {
				CodeBuff[LPtr+CursorRightM+1] = 0x20;
#if UNICODE_INTERNAL_BUFF
				BuffSetChar(&CodeBuffW[LPtr+CursorRightM+1], 0x20, 'H');
#endif
			}
		}

		if (AttrBuff[Ptr+count-1] & AttrKanji) {
			CodeBuff[Ptr+count] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[Ptr+count], 0x20, 'H');
#endif
		}

		if (CursorLeftM > 0 && AttrBuff[Ptr-1] & AttrKanji) {
			CodeBuff[Ptr-1] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[Ptr-1], 0x20, 'H');
#endif
			AttrBuff[Ptr-1] &= ~AttrKanji;
		}

		memmove(&(CodeBuff[Ptr]),   &(CodeBuff[Ptr+count]),   MoveLen);
#if UNICODE_INTERNAL_BUFF
		memmoveW(&(CodeBuffW[Ptr]),   &(CodeBuffW[Ptr+count]),   MoveLen);
#endif
		memmove(&(AttrBuff[Ptr]),   &(AttrBuff[Ptr+count]),   MoveLen);
		memmove(&(AttrBuff2[Ptr]),  &(AttrBuff2[Ptr+count]),  MoveLen);
		memmove(&(AttrBuffFG[Ptr]), &(AttrBuffFG[Ptr+count]), MoveLen);
		memmove(&(AttrBuffBG[Ptr]), &(AttrBuffBG[Ptr+count]), MoveLen);

		memset(&(CodeBuff[Ptr+MoveLen]),   0x20,             count);
#if UNICODE_INTERNAL_BUFF
		memsetW(&(CodeBuffW[Ptr+MoveLen]),   0x20,             count);
#endif
		memset(&(AttrBuff[Ptr+MoveLen]),   AttrDefault,      count);
		memset(&(AttrBuff2[Ptr+MoveLen]),  CurCharAttr.Attr2 & Attr2ColorMask, count);
		memset(&(AttrBuffFG[Ptr+MoveLen]), CurCharAttr.Fore, count);
		memset(&(AttrBuffBG[Ptr+MoveLen]), CurCharAttr.Back, count);

		LPtr = NextLinePtr(LPtr);
	}

	BuffUpdateRect(CursorLeftM-(CursorLeftM>0), CursorTop, CursorRightM+(CursorRightM<NumOfColumns-1), CursorBottom);
}

void BuffScrollRight(int count)
{
	int i, MoveLen;
	LONG LPtr, Ptr;

	UpdateStr();

	LPtr = GetLinePtr(PageStart + CursorTop);
	MoveLen = CursorRightM - CursorLeftM + 1 - count;
	for (i = CursorTop; i <= CursorBottom; i++) {
		Ptr = LPtr + CursorLeftM;

		if (CursorRightM < NumOfColumns-1 && AttrBuff[LPtr+CursorRightM] & AttrKanji) {
			CodeBuff[LPtr+CursorRightM+1] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[LPtr+CursorRightM+1], 0x20, 'H');
#endif
		}

		if (CursorLeftM > 0 && AttrBuff[Ptr-1] & AttrKanji) {
			CodeBuff[Ptr-1] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[Ptr-1], 0x20, 'H');
#endif
			AttrBuff[Ptr-1] &= ~AttrKanji;
			CodeBuff[Ptr] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[Ptr], 0x20, 'H');
#endif
		}

		memmove(&(CodeBuff[Ptr+count]),   &(CodeBuff[Ptr]),   MoveLen);
#if UNICODE_INTERNAL_BUFF
		memmoveW(&(CodeBuffW[Ptr+count]),   &(CodeBuffW[Ptr]),   MoveLen);
#endif
		memmove(&(AttrBuff[Ptr+count]),   &(AttrBuff[Ptr]),   MoveLen);
		memmove(&(AttrBuff2[Ptr+count]),  &(AttrBuff2[Ptr]),  MoveLen);
		memmove(&(AttrBuffFG[Ptr+count]), &(AttrBuffFG[Ptr]), MoveLen);
		memmove(&(AttrBuffBG[Ptr+count]), &(AttrBuffBG[Ptr]), MoveLen);

		memset(&(CodeBuff[Ptr]),   0x20,             count);
#if UNICODE_INTERNAL_BUFF
		memsetW(&(CodeBuffW[Ptr]),   0x20,             count);
#endif
		memset(&(AttrBuff[Ptr]),   AttrDefault,      count);
		memset(&(AttrBuff2[Ptr]),  CurCharAttr.Attr2 & Attr2ColorMask, count);
		memset(&(AttrBuffFG[Ptr]), CurCharAttr.Fore, count);
		memset(&(AttrBuffBG[Ptr]), CurCharAttr.Back, count);

		if (AttrBuff[LPtr+CursorRightM] & AttrKanji) {
			CodeBuff[LPtr+CursorRightM] = 0x20;
#if UNICODE_INTERNAL_BUFF
			BuffSetChar(&CodeBuffW[LPtr+CursorRightM], 0x20, 'H');
#endif
			AttrBuff[LPtr+CursorRightM] &= ~AttrKanji;
		}

		LPtr = NextLinePtr(LPtr);
	}

	BuffUpdateRect(CursorLeftM-(CursorLeftM>0), CursorTop, CursorRightM+(CursorRightM<NumOfColumns-1), CursorBottom);
}

// 現在行をまるごとバッファに格納する。返り値は現在のカーソル位置(X)。
int BuffGetCurrentLineData(char *buf, int bufsize)
{
	LONG Ptr;

	Ptr = GetLinePtr(PageStart + CursorY);
	memset(buf, 0, bufsize);
	memcpy(buf, &CodeBuff[Ptr], min(NumOfColumns, bufsize - 1));
	return (CursorX);
}

// 全バッファから指定した行を返す。
int BuffGetAnyLineData(int offset_y, char *buf, int bufsize)
{
	LONG Ptr;
	int copysize = 0;

	if (offset_y >= BuffEnd)
		return -1;

	Ptr = GetLinePtr(offset_y);
	memset(buf, 0, bufsize);
	copysize = min(NumOfColumns, bufsize - 1);
	memcpy(buf, &CodeBuff[Ptr], copysize);

	return (copysize);
}


BOOL BuffCheckMouseOnURL(int Xw, int Yw)
{
	int X, Y;
	LONG TmpPtr;
	BOOL Result, Right;

	DispConvWinToScreen(Xw, Yw, &X, &Y, &Right);
	Y += PageStart;

	if (X < 0)
		X = 0;
	else if (X > NumOfColumns)
		X = NumOfColumns;
	if (Y < 0)
		Y = 0;
	else if (Y >= BuffEnd)
		Y = BuffEnd - 1;

	TmpPtr = GetLinePtr(Y);
	LockBuffer();

	if (AttrBuff[TmpPtr+X] & AttrURL)
		Result = TRUE;
	else
		Result = FALSE;

	UnlockBuffer();

	return Result;
}


/**
 *	領域を確保して、文字列をフォーマットして、ポインタ返す
 *	不要になったら free() すること
 *	@retval	出力文字数(終端の'\0'を含む)
 *			フォーマット文字列がおかしいときは"EILSEQ"
 *			その他エラー時は -1
 */
int vasprintf(char **strp, const char *fmt, va_list ap)
{
	char *tmp_ptr = NULL;
	size_t tmp_size = 128;
	for(;;) {
		int len;
		int err;
		tmp_ptr = realloc(tmp_ptr, tmp_size);
		assert(tmp_ptr != NULL);
		if (tmp_ptr == NULL) {
			*strp = NULL;
			return -1;
		}
		len = _vsnprintf_s(tmp_ptr, tmp_size, _TRUNCATE, fmt, ap);
		if (len != -1) {
			len++;	// +1 for '\0' (terminator)
			tmp_ptr = realloc(tmp_ptr, len);
			*strp = tmp_ptr;
			return len;
		}
		err = errno;
		if (err == EILSEQ) {
			*strp = _strdup("EILSEQ");
			return 7;
		}
		tmp_size *= 2;
	}
}

/**
 *	領域を確保して、文字列をフォーマットして、ポインタ返す
 *	不要になったら free() すること
 *	@retval	出力文字数(終端のL'\0'を含む)
 *			フォーマット文字列がおかしいときはL"EILSEQ"
 *			その他エラー時は -1
 */
int vaswprintf(wchar_t **strp, const wchar_t *fmt, va_list ap)
{
	wchar_t *tmp_ptr = NULL;
	size_t tmp_size = 128;
	for(;;) {
		int len;
		int err;
		tmp_ptr = realloc(tmp_ptr, sizeof(wchar_t) * tmp_size);
		assert(tmp_ptr != NULL);
		len = _vsnwprintf_s(tmp_ptr, tmp_size, _TRUNCATE, fmt, ap);
		if (len != -1) {
			len++;	// +1 for '\0' (terminator)
			tmp_ptr = realloc(tmp_ptr, sizeof(wchar_t) * len);
			*strp = tmp_ptr;
			return len;
		}
		err = errno;
		if (err == EILSEQ) {
			*strp = _wcsdup(L"EILSEQ");
			return 7;
		}
		tmp_size *= 2;
	}
}

/**
 *	領域を確保して、文字列をフォーマットして、ポインタ返す
 *	不要になったら free() すること
 *	@retval	出力文字数(終端の'\0'を含む)
 *			フォーマット文字列がおかしいときは"EILSEQ"
 *			その他エラー時は -1
 */
int asprintf(char **strp, const char *fmt, ...)
{
	int r;
	va_list ap;
	va_start(ap, fmt);
	r = vasprintf(strp, fmt, ap);
	va_end(ap);
	return r;
}

/**
 *	領域を確保して、文字列をフォーマットして、ポインタ返す
 *	不要になったら free() すること
 *	@retval	出力文字数(終端の'\0'を含む)
 *			フォーマット文字列がおかしいときはL"EILSEQ"
 *			その他エラー時は -1
 */
int aswprintf(wchar_t **strp, const wchar_t *fmt, ...)
{
	int r;
	va_list ap;
	va_start(ap, fmt);
	r = vaswprintf(strp, fmt, ap);
	va_end(ap);
	return r;
}

/**
 *	指定位置の文字情報を文字列で返す
 *	デバグ用途
 *
 *	@param	Xw, Yw	ウィンドウ上のX,Y(pixel),マウスポインタの位置
 *	@reterm	文字列(不要になったらfree()すること)
 */
wchar_t *BuffGetCharInfo(int Xw, int Yw)
{
	int X, Y;
	int ScreenY;
	LONG TmpPtr;
	BOOL Right;
	unsigned char c;
	char cs[4];
	wchar_t *str_ptr;
	size_t str_len;
	unsigned char mb[2];
	wchar_t *mb_str;

	DispConvWinToScreen(Xw, Yw, &X, &ScreenY, &Right);
	Y = PageStart + ScreenY;

	if (X < 0)
		X = 0;
	else if (X > NumOfColumns)
		X = NumOfColumns;
	if (Y < 0)
		Y = 0;
	else if (Y >= BuffEnd)
		Y = BuffEnd - 1;

	TmpPtr = GetLinePtr(Y);
	LockBuffer();

	c = (unsigned char)CodeBuff[TmpPtr+X];
	if (c < 0x20) {
		cs[0] = '.';
		cs[1] = 0;
		mb[0] = c;
		mb[1] = 0;
	} else if ((AttrBuff[TmpPtr+X] & AttrKanji) == 0) {
		// not DBCS(?TODO)
		if (_ismbblead(c)) {
			cs[0] = '*';
			cs[1] = 0;
		}
		else {
			cs[0] = c;
			cs[1] = 0;
		}
		mb[0] = c;
		mb[1] = 0;
	} else {
		if (X+1 < NumOfColumns) {
			char c2 = CodeBuff[TmpPtr+X+1];
			if (_ismbblead(c) && _ismbbtrail(c2)) {
				cs[0] = c;
				cs[1] = c2;
				cs[2] = 0;
				mb[0] = c;
				mb[1] = c2;
			} else {
				cs[0] = '*';
				cs[1] = 0;
				mb[0] = c;
				mb[1] = 0;
			}
		} else {
			// 一番右端
			mb[0] = c;
			mb[1] = 0;
			if (_ismbblead(c)) {
				cs[0] = '*';
				cs[1] = 0;
			} else {
				cs[0] = c;
				cs[1] = 0;
			}
		}
	}

	if (mb[1] == 0) {
		aswprintf(&mb_str, L"0x%02x", mb[0]);
	} else {
		aswprintf(&mb_str, L"0x%02x%02x", mb[0], mb[1]);
	}
	{
		const unsigned char attr = AttrBuff[TmpPtr+X];
		wchar_t *attr_str;
		if (attr == 0) {
			attr_str = _wcsdup(L"");
		} else {
			aswprintf(&attr_str,
					  L"(%S%S%S%S%S%S%S%S)\n",
					  (attr & AttrBold) != 0 ? "AttrBold " : "",
					  (attr & AttrUnder) != 0 ? "AttrUnder " : "",
					  (attr & AttrSpecial) != 0 ? "AttrSpecial ": "",
					  (attr & AttrBlink) != 0 ? "AttrBlink ": "",
					  (attr & AttrReverse) != 0 ? "AttrReverse ": "",
					  (attr & AttrLineContinued) != 0 ? "AttrLineContinued ": "",
					  (attr & AttrURL) != 0 ? "AttrURL ": "",
					  (attr & AttrKanji) != 0 ? "AttrKanji ": ""
				);
		}
		str_len =
			aswprintf(&str_ptr,
					  L"ch(%d,%d(%d)) px(%d,%d)\n"
					  L"attr      0x%02x%s%s\n"
					  L"attr2     0x%02x\n"
					  L"attrFore  0x%02x\n"
					  L"attrBack  0x%02x\n"
//					  L"CodeLine  %s('%hs')",
					  L"CodeLine  %s",
					  X, ScreenY, Y,
					  Xw, Yw,
					  attr, (attr != 0) ? L"\n " : L"", attr_str,
					  (unsigned char)AttrBuff2[TmpPtr+X],
					  (unsigned char)AttrBuffFG[TmpPtr+X],
					  (unsigned char)AttrBuffBG[TmpPtr+X],
//					  mb_str, cs
					  mb_str
				);
		free(attr_str);
	}
	free(mb_str);

#if UNICODE_INTERNAL_BUFF
	{
		const buff_char_t *b = &CodeBuffW[TmpPtr+X];
		wchar_t *wcs = GetWCS(b);
		wchar_t *codes_ptr;
		wchar_t *str2_ptr;
		size_t str2_len;
		int i;
		wchar_t *width_property;

		{
			size_t codes_len = 10 * (b->CombinationCharCount16 + 1);
			codes_ptr = malloc(sizeof(wchar_t) * codes_len);
			_snwprintf_s(codes_ptr, codes_len, _TRUNCATE, L"U+%06x", b->u32);
			for (i=0; i<b->CombinationCharCount32; i++) {
				wchar_t *code_str;
				aswprintf(&code_str, L"U+%06x", b->pCombinationChars16[i]);
				wcscat_s(codes_ptr, codes_len, L"\n");
				wcscat_s(codes_ptr, codes_len, code_str);
				free(code_str);
			}
		}

		width_property =
			b->WidthProperty == 'F' ? L"Fullwidth" :
			b->WidthProperty == 'H' ? L"Halfwidth" :
			b->WidthProperty == 'W' ? L"Wide" :
			b->WidthProperty == 'n' ? L"Narrow" :
			b->WidthProperty == 'A' ? L"Ambiguous" :
			b->WidthProperty == 'N' ? L"Neutral" :
			L"?";

		str2_len = aswprintf(&str2_ptr,
							 L"\n"
							 L"%s\n"
							 L"'%s'\n"
							 L"WidthProperty %s\n"
							 L"Half %s\n"
							 L"Padding %s",
							 codes_ptr,
							 wcs,
							 width_property,
							 (b->HalfWidth ? L"TRUE" : L"FALSE"),
							 (b->Padding ? L"TRUE" : L"FALSE")
			);
		free(codes_ptr);
		free(wcs);

		str_len = str_len + str2_len - 1;	// -1 = remove one '\0'
		str_ptr = realloc(str_ptr, sizeof(wchar_t) * str_len);
		wcscat_s(str_ptr, str_len, str2_ptr);
		free(str2_ptr);
	}
#endif

	UnlockBuffer();

	return str_ptr;
}

void BuffSetCursorCharAttr(int x, int y, TCharAttr Attr)
{
	const LONG TmpPtr = GetLinePtr(PageStart+y);
	AttrBuff[TmpPtr + x] = Attr.Attr;
	AttrBuff2[TmpPtr + x] = Attr.Attr2;
	AttrBuffFG[TmpPtr + x] = Attr.Fore;
	AttrBuffBG[TmpPtr + x] = Attr.Back;
}

TCharAttr BuffGetCursorCharAttr(int x, int y)
{
	const LONG TmpPtr = GetLinePtr(PageStart+y);
	TCharAttr Attr;
	Attr.Attr = AttrBuff[TmpPtr + x];
	Attr.Attr2 = AttrBuff2[TmpPtr + x];
	Attr.Fore = AttrBuffFG[TmpPtr + x];
	Attr.Back =AttrBuffBG[TmpPtr + x];

	return Attr;
}
