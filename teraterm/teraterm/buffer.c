/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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
#include <assert.h>

#include "tttypes.h"
#include "tttypes_charset.h"
#include "ttwinman.h"
#include "teraprn.h"
#include "vtdisp.h"
#include "telnet.h"
#include "ttplug.h" /* TTPLUG */
#include "codeconv.h"
#include "unicode.h"
#include "buffer.h"
#include "asprintf.h"
#include "ttcstd.h"
#include "vtdraw.h"

#define	ENABLE_CELL_INDEX	0

// バッファ内の半角1文字分の情報
typedef struct {
	char32_t u32;
	char32_t u32_last;
	char WidthProperty;				// 'W' or 'F' or 'H' or 'A' or 'n'(Narrow) or 'N'(Neutual) (文字の属性)
	char cell;			// 文字のcell数 1/2/3+=半角,全角,3以上
						// 2以上のとき、この文字の後ろにpaddingがcell-1個続く
	char Padding;					// TRUE = 全角の次の詰め物 or 行末の詰め物
	char Emoji;						// TRUE = 絵文字
	unsigned char CombinationCharCount16;	// character count
	unsigned char CombinationCharSize16;		// buffer size
	unsigned char CombinationCharCount32;
	unsigned char CombinationCharSize32;
	wchar_t *pCombinationChars16;
	char32_t *pCombinationChars32;
	wchar_t	wc2[2];
	unsigned char fg;
	unsigned char bg;
	unsigned char attr;
	unsigned char attr2;
	unsigned short ansi_char;
#if ENABLE_CELL_INDEX
	int idx;	// セル通し番号
#endif
} buff_char_t;

#define BuffXMax TermWidthMax
//#define BuffYMax 100000
//#define BuffSizeMax 8000000
// スクロールバッファの最大長を拡張 (2004.11.28 yutaka)
#define BuffYMax 500000
#define BuffSizeMax (BuffYMax * 80)

// 1文字あたりのコンビネーションバッファ最大サイズ
#define MAX_CHAR_SIZE	100

// status line
int StatusLine;	//0: none 1: shown
/* top, bottom, left & right margin */
int CursorTop, CursorBottom, CursorLeftM, CursorRightM;
BOOL Wrap;

static WORD TabStops[256];
static int NTabStops;

static WORD BuffLock = 0;

static buff_char_t *CodeBuffW;
static LONG LinePtr;
static LONG BufferSize;
static int NumOfLinesInBuff;
static int BuffStartAbs, BuffEndAbs;

// 選択
static BOOL Selected;			// TRUE = 領域選択されている
static BOOL Selecting;			// ?
static POINT SelectStart;
static POINT SelectEnd;
static POINT ClickCell;			// クリックした文字cell
static POINT SelectEndOld;
static DWORD SelectStartTime;
static BOOL BoxSelect;
static POINT DblClkStart, DblClkEnd;

// 描画
static int StrChangeStart;	// 描画開始 X (Y=CursorY)
static int StrChangeCount;	// 描画キャラクタ数(半角単位),0のとき描画するものがない
static BOOL UseUnicodeApi;

static BOOL SeveralPageSelect;  // add (2005.5.15 yutaka)

static TCharAttr CurCharAttr;

static char *SaveBuff = NULL;
static int SaveBuffX;
static int SaveBuffY;

// ANSI表示用に変換するときのCodePage
static int CodePage = 932;
vtdraw_t *vt_src;

static void BuffDrawLineI(vtdraw_t *vt, ttdc_t *dc, int SY, int IStart, int IEnd);

/**
 *	buff_char_t を relセル移動する
 *
 *	@param	CodeBuffW_	文字バッファの先頭ポインタ
 *	@param	BufferSize_	文字バッファのサイズ(buff_char_t単位)
 *	@param	p			移動させるポインタ
 *	@param	rel			移動量
 *	@retval	移動後のポインタ
 */
static buff_char_t *GetPtrRel(buff_char_t *CodeBuffW_, size_t BufferSize_, buff_char_t *p, int rel)
{
	ptrdiff_t idx = (ptrdiff_t)(p - CodeBuffW_) + rel;
	for (;;) {
		if (idx < 0) {
			idx += BufferSize_;
		}
		else if (idx >= (ptrdiff_t)BufferSize_) {
			idx -= BufferSize_;
		}
		else {
			break;
		}
	}
	p = &CodeBuffW_[(int)idx];
	return p;
}

static void FreeCombinationBuf(buff_char_t *b)
{
	if (b->pCombinationChars16 != NULL) {
		free(b->pCombinationChars16);
		b->pCombinationChars16 = NULL;
	}
	b->CombinationCharSize16 = 0;
	b->CombinationCharCount16 = 0;

	if (b->pCombinationChars32 != NULL) {
		free(b->pCombinationChars32);
		b->pCombinationChars32 = NULL;
	}
	b->CombinationCharSize32 = 0;
	b->CombinationCharCount32 = 0;
}

static void DupCombinationBuf(buff_char_t *b)
{
	size_t size;

	size = b->CombinationCharSize16;
	if (size > 0) {
		wchar_t *new_buf = malloc(sizeof(wchar_t) * size);
		memcpy(new_buf, b->pCombinationChars16, sizeof(wchar_t) * size);
		b->pCombinationChars16 = new_buf;
	}
	size = b->CombinationCharSize32;
	if (size > 0) {
		char32_t *new_buf = malloc(sizeof(char32_t) * size);
		memcpy(new_buf, b->pCombinationChars32, sizeof(char32_t) * size);
		b->pCombinationChars32 = new_buf;
	}
}

static void CopyCombinationBuf(buff_char_t *dest, const buff_char_t *src)
{
	FreeCombinationBuf(dest);

	// 構造体をコピーする
#if ENABLE_CELL_INDEX
	int idx = dest->idx;
#endif
	*dest = *src;
#if ENABLE_CELL_INDEX
	dest->idx = idx;
#endif

	DupCombinationBuf(dest);
}

static void BuffSetChar2(buff_char_t *buff, char32_t u32, char property, BOOL half_width, char emoji)
{
	size_t wstr_len;
	buff_char_t *p = buff;

	FreeCombinationBuf(p);
	p->WidthProperty = property;
	p->cell = half_width ? 1 : 2;
	p->u32 = u32;
	p->u32_last = u32;
	p->Padding = FALSE;
	p->Emoji = emoji;
	p->fg = AttrDefaultFG;
	p->bg = AttrDefaultBG;

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

	if (u32 < 0x80) {
		p->ansi_char = (unsigned short)u32;
	}
	else {
		if (u32 == 0x203e && CodePage == 932) {
			// U+203e OVERLINE 特別処理
			//	 U+203eは0x7e'~'に変換
			//p->ansi_char = 0x7e7e;
			p->ansi_char = 0x7e;
		}
		else {
			char strA[4];
			size_t lenA = UTF32ToMBCP(u32, CodePage, strA, sizeof(strA));
			switch (lenA) {
			case 0:
			default:
				p->ansi_char = '?';
				break;
			case 1:
				p->ansi_char = (unsigned char)strA[0];
				break;
			case 2:
				p->ansi_char = (unsigned char)strA[1] | ((unsigned char)strA[0] << 8);
				break;
			}
		}
	}
}

static void BuffSetChar4(buff_char_t *buff, char32_t u32, unsigned char fg, unsigned char bg, unsigned char attr, unsigned char attr2, char property)
{
	buff_char_t *p = buff;
	BuffSetChar2(p, u32, property, TRUE, FALSE);
	p->fg = fg;
	p->bg = bg;
	p->attr = attr;
	p->attr2 = attr2;
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
		if (new_size > MAX_CHAR_SIZE) {
			new_size = MAX_CHAR_SIZE;
		}
		if (p->CombinationCharSize16 != new_size) {
			p->pCombinationChars16 = realloc(p->pCombinationChars16, sizeof(wchar_t) * new_size);
			p->CombinationCharSize16 = (char)new_size;
		}
	}
	if (p->CombinationCharSize32 < p->CombinationCharCount32 + 2) {
		size_t new_size = p->CombinationCharSize32;
		new_size = new_size == 0 ? 5 : new_size * 2;
		if (new_size > MAX_CHAR_SIZE) {
			new_size = MAX_CHAR_SIZE;
		}
		if (p->CombinationCharSize32 != new_size) {
			p->pCombinationChars32 = realloc(p->pCombinationChars32, sizeof(char32_t) * new_size);
			p->CombinationCharSize32 = (char)new_size;
		}
	}

	// UTF-32
	if (p->CombinationCharCount32 < p->CombinationCharSize32) {
		p->u32_last = u32;
		p->pCombinationChars32[(size_t)p->CombinationCharCount32] = u32;
		p->CombinationCharCount32++;
	}

	// UTF-16
	if (p->CombinationCharCount16 < p->CombinationCharSize16) {
		wchar_t u16_str[2];
		size_t wlen = UTF32ToUTF16(u32, &u16_str[0], 2);
		if (p->CombinationCharCount16 + wlen <= p->CombinationCharSize16) {
			size_t i = (size_t)p->CombinationCharCount16;
			p->pCombinationChars16[i] = u16_str[0];
			if (wlen == 2) {
				i++;
				p->pCombinationChars16[i] = u16_str[1];
			}
			p->CombinationCharCount16 += (unsigned char)wlen;
		}
	}
}

static void memcpyW(buff_char_t *dest, const buff_char_t *src, size_t count)
{
	size_t i;

	if (dest == src || count == 0) {
		return;
	}

	for (i = 0; i < count; i++) {
		CopyCombinationBuf(dest, src);
		dest++;
		src++;
	}
}

static void memsetW(buff_char_t *dest, wchar_t ch, unsigned char fg, unsigned char bg, unsigned char attr, unsigned char attr2, size_t count)
{
	size_t i;
	for (i=0; i<count; i++) {
		BuffSetChar(dest, ch, 'H');
		dest->fg = fg;
		dest->bg = bg;
		dest->attr = attr;
		dest->attr2 = attr2;
		dest++;
	}
}

static void memmoveW(buff_char_t *dest, const buff_char_t *src, size_t count)
{
	size_t i;

	if (dest == src || count == 0) {
		return;
	}


	if (dest < src) {
		// 前からコピーする? -> memcpyW() でok
		memcpyW(dest, src, count);
	}
	else {
		// 後ろからコピーする
		dest += count - 1;
		src += count - 1;
		for (i = 0; i < count; i++) {
			CopyCombinationBuf(dest, src);
			dest--;
			src--;
		}
	}
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
	if (b->cell != 1)
		return TRUE;
	return FALSE;
}

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

/**
 * ポインタの位置から x,y を求める
 */
static void GetPosFromPtr(const buff_char_t *b, int *bx, int *by)
{
	size_t index = b - CodeBuffW;
	int x = (int)(index % NumOfColumns);
	int y = (int)(index / NumOfColumns);
	if (y >= BuffStartAbs) {
		y -= BuffStartAbs;
	}
	else {
		y = y - BuffStartAbs + NumOfLinesInBuff;
	}
	*bx = x;
	*by = y;
}

static BOOL ChangeBuffer(int Nx, int Ny)
{
	LONG NewSize;
	int NxCopy, NyCopy, i;
	LONG SrcPtr, DestPtr;
	WORD LockOld;
	buff_char_t *CodeDestW;

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

	CodeDestW = NULL;
	CodeDestW = malloc(NewSize * sizeof(buff_char_t));
	if (CodeDestW == NULL) {
		goto allocate_error;
	}

	memset(&CodeDestW[0], 0, NewSize * sizeof(buff_char_t));
#if ENABLE_CELL_INDEX
	{
		int i;
		for (i = 0; i < NewSize; i++) {
			CodeDestW[i].idx = i;
		}
	}
#endif
	memsetW(&CodeDestW[0], 0x20, AttrDefaultFG, AttrDefaultBG, AttrDefault, AttrDefault, NewSize);
	if ( CodeBuffW != NULL ) {
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
			memcpyW(&CodeDestW[DestPtr],&CodeBuffW[SrcPtr],NxCopy);
			if (CodeDestW[DestPtr+NxCopy-1].attr & AttrKanji) {
				BuffSetChar(&CodeDestW[DestPtr + NxCopy - 1], ' ', 'H');
				CodeDestW[DestPtr+NxCopy-1].attr ^= AttrKanji;
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

	CodeBuffW = CodeDestW;
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
	}
	else {
		;
	}
	BuffLock = LockOld;

	return TRUE;

allocate_error:
	if (CodeDestW)  free(CodeDestW);
	return FALSE;
}

void InitBuffer(IdVtDrawAPI draw_api)
{
	int Ny;

	UseUnicodeApi = draw_api == IdVtDrawAPIUnicode ? TRUE : FALSE;

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
}

void LockBuffer(void)
{
	BuffLock++;
	if (BuffLock>1) {
		return;
	}
	NewLine(PageStart+CursorY);
}

void UnlockBuffer(void)
{
	if (BuffLock==0) {
		return;
	}
	BuffLock--;
	if (BuffLock>0) {
		return;
	}
}

void FreeBuffer(void)
{
	int i;

	for (i = 0; i < NumOfColumns * NumOfLinesInBuff; i++) {
		FreeCombinationBuf(&CodeBuffW[i]);
	}

	BuffLock = 1;
	UnlockBuffer();
	if (CodeBuffW != NULL) {
		free(CodeBuffW);
		CodeBuffW = NULL;
	}
}

void BuffAllSelect(void)
{
	SelectStart.x = 0;
	SelectStart.y = 0;
	SelectEnd.x = 0;
	SelectEnd.y = BuffEnd;
//	SelectEnd.x = NumOfColumns;
//	SelectEnd.y = BuffEnd - 1;
	Selecting = TRUE;
}

void BuffScreenSelect(void)
{
	int X, Y;
	DispConvWinToScreen(vt_src, 0, 0, &X, &Y, NULL);
	SelectStart.x = X;
	SelectStart.y = Y + PageStart;
	SelectEnd.x = 0;
	SelectEnd.y = SelectStart.y + NumOfLines;
//	SelectEnd.x = X + NumOfColumns;
//	SelectEnd.y = Y + PageStart + NumOfLines - 1;
	Selecting = TRUE;
}

void BuffCancelSelection(void)
{
	SelectStart.x = 0;
	SelectStart.y = 0;
	SelectEnd.x = 0;
	SelectEnd.y = 0;
	Selecting = FALSE;
}

void BuffReset(void)
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

static void BuffScroll(int Count, int Bottom)
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
			memcpyW(&(CodeBuffW[DestPtr]),&(CodeBuffW[SrcPtr]),NumOfColumns);
			memsetW(&(CodeBuffW[SrcPtr]),0x20,CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, NumOfColumns);
			SrcPtr = PrevLinePtr(SrcPtr);
			DestPtr = PrevLinePtr(DestPtr);
			n--;
		}
	}
	for (i = 1 ; i <= n ; i++) {
		buff_char_t *b = &CodeBuffW[DestPtr];
		memsetW(b ,0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, NumOfColumns);
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

/**
 * If cursor is on left/right half of a Kanji, erase it.
 *	@param	LR	left(0)/right(1) flag
 *				0	カーソルが漢字の左側
 *				1	カーソルが漢字の右側
 *	@return		処理したcell数
 *				0	処理しなかった
 *				1,2	処理した
 */
static int EraseKanji(int LR)
{
	buff_char_t * CodeLineW = &CodeBuffW[LinePtr];
	buff_char_t *p;
	int bx;
	int cell = 0;

	if (CursorX < LR) {
		// 全角判定できない
		return 0;
	}
	bx = CursorX-LR;
	p = &CodeLineW[bx];
	if (IsBuffFullWidth(p)) {
		// 全角をつぶす
		BuffSetChar(p, ' ', 'H');
		p->attr = CurCharAttr.Attr;
		p->attr2 = CurCharAttr.Attr2;
		p->fg = CurCharAttr.Fore;
		p->bg = CurCharAttr.Back;
		if (bx+1 < NumOfColumns) {
			BuffSetChar(p + 1, ' ', 'H');
			(p+1)->attr = CurCharAttr.Attr;
			(p+1)->attr2 = CurCharAttr.Attr2;
			(p+1)->fg = CurCharAttr.Fore;
			(p+1)->bg = CurCharAttr.Back;
			cell = 2;
		}
		else {
			cell = 1;
		}
	}
	return cell;
}

static void EraseKanjiOnLRMargin(LONG ptr, int count)
{
	int i;
	LONG pos;

	if (count < 1)
		return;

	for (i=0; i<count; i++) {
		pos = ptr + CursorLeftM-1;
		if (CursorLeftM>0 && (CodeBuffW[pos].attr & AttrKanji)) {
			BuffSetChar(&CodeBuffW[pos], 0x20, 'H');
			CodeBuffW[pos].attr &= ~AttrKanji;
			pos++;
			BuffSetChar(&CodeBuffW[pos], 0x20, 'H');
			CodeBuffW[pos].attr &= ~AttrKanji;
		}
		pos = ptr + CursorRightM;
		if (CursorRightM < NumOfColumns-1 && (CodeBuffW[pos].attr & AttrKanji)) {
			BuffSetChar(&CodeBuffW[pos], 0x20, 'H');
			CodeBuffW[pos].attr &= ~AttrKanji;
			pos++;
			BuffSetChar(&CodeBuffW[pos], 0x20, 'H');
			CodeBuffW[pos].attr &= ~AttrKanji;
		}
		ptr = NextLinePtr(ptr);
	}
}

// Insert space characters at the current position
//   Count: Number of characters to be inserted
void BuffInsertSpace(int Count)
{
	buff_char_t * CodeLineW = &CodeBuffW[LinePtr];
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
		b->attr &= ~AttrKanji;
		sx--;
		extr++;
	}

	if (CursorRightM < NumOfColumns - 1 && (CodeLineW[CursorRightM].attr & AttrKanji)) {
		BuffSetChar(&CodeLineW[CursorRightM + 1], 0x20, 'H');
		CodeLineW[CursorRightM + 1].attr &= ~AttrKanji;
		extr++;
	}

	if (Count > CursorRightM + 1 - CursorX)
		Count = CursorRightM + 1 - CursorX;

	MoveLen = CursorRightM + 1 - CursorX - Count;

	if (MoveLen > 0) {
		memmoveW(&(CodeLineW[CursorX + Count]), &(CodeLineW[CursorX]), MoveLen);
	}
	memsetW(&(CodeLineW[CursorX]), 0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2, Count);
	/* last char in current line is kanji first? */
	if ((CodeLineW[CursorRightM].attr & AttrKanji) != 0) {
		/* then delete it */
		BuffSetChar(&CodeLineW[CursorRightM], 0x20, 'H');
		CodeLineW[CursorRightM].attr &= ~AttrKanji;
	}
	BuffUpdateRect(sx, CursorY, CursorRightM + extr, CursorY);
}

/**
 *	Erase characters from cursor to the end of screen
 */
void BuffEraseCurToEnd(void)
{
	LONG TmpPtr;
	int offset;
	int i, YEnd;
	int XStart = CursorX;
	int head = 0;

	NewLine(PageStart+CursorY);
	/* if cursor is on right half of a kanji, erase the kanji */
	if (EraseKanji(1) != 0) {
		head = 1;
	}
	offset = CursorX;
	TmpPtr = GetLinePtr(PageStart+CursorY);
	YEnd = NumOfLines-1;
	if (StatusLine && !isCursorOnStatusLine) {
		YEnd--;
	}
	for (i = CursorY ; i <= YEnd ; i++) {
		memsetW(&(CodeBuffW[TmpPtr+offset]),0x20,CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, NumOfColumns-offset);
		offset = 0;
		TmpPtr = NextLinePtr(TmpPtr);
	}

	/* update window */
	if (head == 1) {
		XStart--;
	}
	ttdc_t *dc = DispInitDC(vt_src);
	BuffDrawLineI(vt_src, dc, CursorY + PageStart, XStart, NumOfColumns);
	for (i = CursorY + 1; i <= YEnd; i++) {
		BuffDrawLineI(vt_src, dc, i + PageStart, 0, NumOfColumns);
	}
	DispReleaseDC(vt_src, dc);
}

/**
 * Erase characters from home to cursor
 */
void BuffEraseHomeToCur(void)
{
	LONG TmpPtr;
	int offset;
	int i, YHome;
	int tail = 0;
	int draw_len;

	NewLine(PageStart+CursorY);
	/* if cursor is on left half of a kanji, erase the kanji */
	if (EraseKanji(0) != 0) {
		tail++;
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
		memsetW(&(CodeBuffW[TmpPtr]),0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, offset);
		TmpPtr = NextLinePtr(TmpPtr);
	}

	/* update window */
	ttdc_t *dc = DispInitDC(vt_src);
	draw_len = tail == 0 ? CursorX : CursorX + 1;
	for (i = YHome; i < CursorY; i++) {
		BuffDrawLineI(vt_src, dc, i + PageStart, 0, NumOfColumns);
	}
	BuffDrawLineI(vt_src, dc, CursorY + PageStart, 0, draw_len);
	DispReleaseDC(vt_src, dc);
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
		memcpyW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
		SrcPtr = PrevLinePtr(SrcPtr);
		DestPtr = PrevLinePtr(DestPtr);
	}
	for (i = 1 ; i <= Count ; i++) {
		memsetW(&(CodeBuffW[DestPtr]), 0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, linelen);
		DestPtr = PrevLinePtr(DestPtr);
	}

	if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1 || !DispInsertLines(vt_src, Count, YEnd)) {
		BuffUpdateRect(CursorLeftM-extl, CursorY, CursorRightM+extr, YEnd);
	}
}

/**
 *	erase characters in the current line
 *	@param	start position of erasing
 *	@param Count: number of characters to be erased
 */
void BuffEraseCharsInLine(int XStart, int Count)
{
	buff_char_t * CodeLineW = &CodeBuffW[LinePtr];
	BOOL LineContinued=FALSE;
	int head = 0;
	int tail = 0;

	if (ts.EnableContinuedLineCopy && XStart == 0 && (CodeLineW[0].attr & AttrLineContinued)) {
		LineContinued = TRUE;
	}

	/* if cursor is on right half of a kanji, erase the kanji */
	if (EraseKanji(1) != 0) {
		head = 1;
	}

	if (XStart + Count < NumOfColumns) {
		int CursorXSave = CursorX;
		CursorX = XStart + Count;
		if (EraseKanji(1) != 0) {
			tail = 1;
		}
		CursorX = CursorXSave;
	}

	NewLine(PageStart+CursorY);
	memsetW(&(CodeLineW[XStart]),0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, Count);

	if (ts.EnableContinuedLineCopy) {
		if (LineContinued) {
			BuffLineContinued(TRUE);
		}

		if (XStart + Count >= NumOfColumns) {
			CodeBuffW[NextLinePtr(LinePtr)].attr &= ~AttrLineContinued;
		}
	}

	if (head != 0) {
		XStart -= 1;
		Count += 1;
	}
	if (tail != 0) {
		Count += 1;
	}
	ttdc_t *dc = DispInitDC(vt_src);
	BuffDrawLineI(vt_src, dc, CursorY + PageStart, XStart, XStart + Count);
	DispReleaseDC(vt_src, dc);
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
		memcpyW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
		SrcPtr = NextLinePtr(SrcPtr);
		DestPtr = NextLinePtr(DestPtr);
	}
	for (i = YEnd+1-Count ; i<=YEnd ; i++) {
		memsetW(&(CodeBuffW[DestPtr]), 0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, linelen);
		DestPtr = NextLinePtr(DestPtr);
	}

	if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1 || !DispDeleteLines(vt_src, Count,YEnd)) {
		BuffUpdateRect(CursorLeftM-extl, CursorY, CursorRightM+extr, YEnd);
	}
}

// Delete characters in current line from cursor
//   Count: number of characters to be deleted
void BuffDeleteChars(int Count)
{
	buff_char_t * CodeLineW = &CodeBuffW[LinePtr];
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

	if (CursorRightM < NumOfColumns - 1 && (CodeLineW[CursorRightM].attr & AttrKanji)) {
		BuffSetChar(&CodeLineW[CursorRightM], 0x20, 'H');
		CodeLineW[CursorRightM].attr &= ~AttrKanji;
		BuffSetChar(&CodeLineW[CursorRightM + 1], 0x20, 'H');
		CodeLineW[CursorRightM + 1].attr &= ~AttrKanji;
		extr = 1;
	}

	MoveLen = CursorRightM + 1 - CursorX - Count;

	if (MoveLen > 0) {
		memmoveW(&(CodeLineW[CursorX]), &(CodeLineW[CursorX + Count]), MoveLen);
	}
	memsetW(&(CodeLineW[CursorX + MoveLen]), ' ', CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, Count);

	BuffUpdateRect(CursorX, CursorY, CursorRightM + extr, CursorY);
}

/**
 * Erase characters in current line from cursor
 *	@param Count	number of characters to be deleted
 */
void BuffEraseChars(int Count)
{
	BuffEraseCharsInLine(CursorX, Count);
}

void BuffFillWithE(void)
// Fill screen with 'E' characters
{
	LONG TmpPtr;
	int i;

	TmpPtr = GetLinePtr(PageStart);
	for (i = 0 ; i <= NumOfLines-1-StatusLine ; i++) {
		memsetW(&(CodeBuffW[TmpPtr]),'E', AttrDefaultFG, AttrDefaultBG, AttrDefault, AttrDefault, NumOfColumns);
		TmpPtr = NextLinePtr(TmpPtr);
	}
	BuffUpdateRect(WinOrgX,WinOrgY,WinOrgX+WinWidth-1,WinOrgY+WinHeight-1);
}

void BuffDrawLine(const TCharAttr *Attr, int Direction, int C)
{ // IO-8256 terminal
	LONG Ptr;
	int i, X, Y;
	BYTE Attr_Attr = Attr->Attr;

	if (C==0) {
		return;
	}
	Attr_Attr |= AttrSpecial;

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
			memsetW(&(CodeBuffW[Ptr+CursorX]),'q', Attr->Fore, Attr->Back, Attr_Attr, Attr->Attr2, C);
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
				BuffSetChar4(&CodeBuffW[Ptr+X], 'x', Attr->Fore, Attr->Back, Attr_Attr | AttrSpecial, Attr->Attr2, 'H');
				Ptr = NextLinePtr(Ptr);
			}
			BuffUpdateRect(X,CursorY,X,CursorY+C-1);
			break;
	}
}

void BuffEraseBox(int XStart, int YStart, int XEnd, int YEnd)
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
		    ((CodeBuffW[Ptr+XStart-1].attr & AttrKanji) != 0)) {
			BuffSetChar4(&CodeBuffW[Ptr+XStart-1], 0x20, CurCharAttr.Fore, CurCharAttr.Back, CurCharAttr.Attr, CurCharAttr.Attr2, 'H');
		}
		if ((XStart+C<NumOfColumns) &&
		    ((CodeBuffW[Ptr+XStart+C-1].attr & AttrKanji) != 0)) {
			BuffSetChar4(&CodeBuffW[Ptr+XStart+C], 0x20, CurCharAttr.Fore, CurCharAttr.Back, CurCharAttr.Attr, CurCharAttr.Attr2, 'H');
		}
		memsetW(&(CodeBuffW[Ptr+XStart]),0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, C);
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
		    ((CodeBuffW[Ptr+XStart-1].attr & AttrKanji) != 0)) {
			BuffSetChar(&CodeBuffW[Ptr + XStart - 1], 0x20, 'H');
			CodeBuffW[Ptr+XStart-1].attr ^= AttrKanji;
		}
		if ((XStart+Cols<NumOfColumns) &&
		    ((CodeBuffW[Ptr+XStart+Cols-1].attr & AttrKanji) != 0)) {
			BuffSetChar(&CodeBuffW[Ptr + XStart + Cols], 0x20, 'H');
		}
		memsetW(&(CodeBuffW[Ptr+XStart]), ch, CurCharAttr.Fore, CurCharAttr.Back, CurCharAttr.Attr, CurCharAttr.Attr2, Cols);
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
	(void)SrcPage;		// warning: parameter 'SrcPage' set but not used
	(void)DstPage;		// warning: parameter 'DstPage' set but not used

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
			memcpyW(&(CodeBuffW[DPtr+DstX]), &(CodeBuffW[SPtr+SrcXStart]), C);
			SPtr = NextLinePtr(SPtr);
			DPtr = NextLinePtr(DPtr);
		}
	}
	else if (SrcXStart < DstX) {
		SPtr = GetLinePtr(PageStart+SrcYEnd);
		DPtr = GetLinePtr(PageStart+DstY+L-1);
		for (i=L; i>0; i--) {
			memcpyW(&(CodeBuffW[DPtr+DstX]), &(CodeBuffW[SPtr+SrcXStart]), C);
			SPtr = PrevLinePtr(SPtr);
			DPtr = PrevLinePtr(DPtr);
		}
	}
	else if (SrcYStart != DstY) {
		SPtr = GetLinePtr(PageStart+SrcYStart);
		DPtr = GetLinePtr(PageStart+DstY);
		for (i=0; i<L; i++) {
			memmoveW(&(CodeBuffW[DPtr+DstX]), &(CodeBuffW[SPtr+SrcXStart]), C);
			SPtr = NextLinePtr(SPtr);
			DPtr = NextLinePtr(DPtr);
		}
	}
	BuffUpdateRect(DstX,DstY,DstX+C-1,DstY+L-1);
}

void BuffChangeAttrBox(int XStart, int YStart, int XEnd, int YEnd, const PCharAttr attr, const PCharAttr mask)
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
			if (XStart>0 && (CodeBuffW[j].attr & AttrKanji)) {
				CodeBuffW[j].attr = (CodeBuffW[j].attr & ~mask->Attr) | attr->Attr;
				CodeBuffW[j].attr2 = (CodeBuffW[j].attr2 & ~mask->Attr2) | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { CodeBuffW[j].fg = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { CodeBuffW[j].bg = attr->Back; }
			}
			while (++j < Ptr+XStart+C) {
				CodeBuffW[j].attr = (CodeBuffW[j].attr & ~mask->Attr) | attr->Attr;
				CodeBuffW[j].attr2 = (CodeBuffW[j].attr2 & ~mask->Attr2) | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { CodeBuffW[j].fg = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { CodeBuffW[j].bg = attr->Back; }
			}
			if (XStart+C<NumOfColumns && (CodeBuffW[j-1].attr & AttrKanji)) {
				CodeBuffW[j].attr = (CodeBuffW[j].attr & ~mask->Attr) | attr->Attr;
				CodeBuffW[j].attr2 = (CodeBuffW[j].attr2 & ~mask->Attr2) | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { CodeBuffW[j].fg = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { CodeBuffW[j].bg = attr->Back; }
			}
			Ptr = NextLinePtr(Ptr);
		}
	}
	else { // DECRARA
		for (i=YStart; i<=YEnd; i++) {
			j = Ptr+XStart-1;
			if (XStart>0 && (CodeBuffW[j].attr & AttrKanji)) {
				CodeBuffW[j].attr ^= attr->Attr;
			}
			while (++j < Ptr+XStart+C) {
				CodeBuffW[j].attr ^= attr->Attr;
			}
			if (XStart+C<NumOfColumns && (CodeBuffW[j-1].attr & AttrKanji)) {
				CodeBuffW[j].attr ^= attr->Attr;
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

			if (XStart > 0 && (CodeBuffW[i].attr & AttrKanji)) {
				CodeBuffW[i].attr = (CodeBuffW[i].attr & ~mask->Attr) | attr->Attr;
				CodeBuffW[i].attr2 = (CodeBuffW[i].attr2 & ~mask->Attr2) | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { CodeBuffW[i].fg = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { CodeBuffW[i].bg = attr->Back; }
			}
			while (++i < endp) {
				CodeBuffW[i].attr = (CodeBuffW[i].attr & ~mask->Attr) | attr->Attr;
				CodeBuffW[i].attr2 = (CodeBuffW[i].attr2 & ~mask->Attr2) | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { CodeBuffW[i].fg = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { CodeBuffW[i].bg = attr->Back; }
			}
			if (XEnd < NumOfColumns-1 && (CodeBuffW[i-1].attr & AttrKanji)) {
				CodeBuffW[i].attr = (CodeBuffW[i].attr & ~mask->Attr) | attr->Attr;
				CodeBuffW[i].attr2 = (CodeBuffW[i].attr2 & ~mask->Attr2) | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { CodeBuffW[i].fg = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { CodeBuffW[i].bg = attr->Back; }
			}
		}
		else {
			i = Ptr + XStart - 1;
			endp = Ptr + NumOfColumns;

			if (XStart > 0 && (CodeBuffW[i].attr & AttrKanji)) {
				CodeBuffW[i].attr = (CodeBuffW[i].attr & ~mask->Attr) | attr->Attr;
				CodeBuffW[i].attr2 = (CodeBuffW[i].attr2 & ~mask->Attr2) | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { CodeBuffW[i].fg = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { CodeBuffW[i].bg = attr->Back; }
			}
			while (++i < endp) {
				CodeBuffW[i].attr = (CodeBuffW[i].attr & ~mask->Attr) | attr->Attr;
				CodeBuffW[i].attr2 = (CodeBuffW[i].attr2 & ~mask->Attr2) | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { CodeBuffW[i].fg = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { CodeBuffW[i].bg = attr->Back; }
			}

			for (j=0; j < YEnd-YStart-1; j++) {
				Ptr = NextLinePtr(Ptr);
				i = Ptr;
				endp = Ptr + NumOfColumns;

				while (i < endp) {
					CodeBuffW[i].attr = (CodeBuffW[i].attr & ~mask->Attr) | attr->Attr;
					CodeBuffW[i].attr2 = (CodeBuffW[i].attr2 & ~mask->Attr2) | attr->Attr2;
					if (mask->Attr2 & Attr2Fore) { CodeBuffW[i].fg = attr->Fore; }
					if (mask->Attr2 & Attr2Back) { CodeBuffW[i].bg = attr->Back; }
					i++;
				}
			}

			Ptr = NextLinePtr(Ptr);
			i = Ptr;
			endp = Ptr + XEnd + 1;

			while (i < endp) {
				CodeBuffW[i].attr = (CodeBuffW[i].attr & ~mask->Attr) | attr->Attr;
				CodeBuffW[i].attr2 = (CodeBuffW[i].attr2 & ~mask->Attr2) | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { CodeBuffW[i].fg = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { CodeBuffW[i].bg = attr->Back; }
				i++;
			}
			if (XEnd < NumOfColumns-1 && (CodeBuffW[i-1].attr & AttrKanji)) {
				CodeBuffW[i].attr = (CodeBuffW[i].attr & ~mask->Attr) | attr->Attr;
				CodeBuffW[i].attr2 = (CodeBuffW[i].attr2 & ~mask->Attr2) | attr->Attr2;
				if (mask->Attr2 & Attr2Fore) { CodeBuffW[i].fg = attr->Fore; }
				if (mask->Attr2 & Attr2Back) { CodeBuffW[i].bg = attr->Back; }
			}
		}
	}
	else { // DECRARA
		if (YStart == YEnd) {
			i = Ptr + XStart - 1;
			endp = Ptr + XEnd + 1;

			if (XStart > 0 && (CodeBuffW[i].attr & AttrKanji)) {
				CodeBuffW[i].attr ^= attr->Attr;
			}
			while (++i < endp) {
				CodeBuffW[i].attr ^= attr->Attr;
			}
			if (XEnd < NumOfColumns-1 && (CodeBuffW[i-1].attr & AttrKanji)) {
				CodeBuffW[i].attr ^= attr->Attr;
			}
		}
		else {
			i = Ptr + XStart - 1;
			endp = Ptr + NumOfColumns;

			if (XStart > 0 && (CodeBuffW[i].attr & AttrKanji)) {
				CodeBuffW[i].attr ^= attr->Attr;
			}
			while (++i < endp) {
				CodeBuffW[i].attr ^= attr->Attr;
			}

			for (j=0; j < YEnd-YStart-1; j++) {
				Ptr = NextLinePtr(Ptr);
				i = Ptr;
				endp = Ptr + NumOfColumns;

				while (i < endp) {
					CodeBuffW[i].attr ^= attr->Attr;
					i++;
				}
			}

			Ptr = NextLinePtr(Ptr);
			i = Ptr;
			endp = Ptr + XEnd + 1;

			while (i < endp) {
				CodeBuffW[i].attr ^= attr->Attr;
				i++;
			}
			if (XEnd < NumOfColumns-1 && (CodeBuffW[i-1].attr & AttrKanji)) {
				CodeBuffW[i].attr ^= attr->Attr;
			}
			Ptr = NextLinePtr(Ptr);
		}
	}
	BuffUpdateRect(0, YStart, NumOfColumns-1, YEnd);
}

/**
 *	(Line,CharPtr)位置が全角の右側(文字長さN cellの1cell目ではない)とき
 *	先頭の文字の位置を返す
 *
 *	If CharPtr is on the right half of a DBCS character
 *	return pointer to the left half
 *
 *	@param Line		points to a line in CodeBuff
 *	@param CharPtr	points to a char
 *	@return points to the left half of the DBCS
 */
static int LeftHalfOfDBCS(LONG Line, int CharPtr)
{
	int x = CharPtr;
	while(x > 0) {
		if ((CodeBuffW[Line+x].Padding) == FALSE) {
			// paddingではない
			break;
		}
		assert(x > 0);	// 行頭で padding?
		x--;
	}
	return x;
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
			if ((CodeBuffW[Line+*x].attr & AttrKanji) != 0) {
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
static wchar_t *BuffGetStringForCB(int sx, int sy, int ex, int ey, BOOL box_select, size_t *_str_len)
{
	wchar_t *str_w;
	size_t str_size;	// 確保したサイズ
	size_t k;
	LONG TmpPtr;
	int x, y;

	assert(sx >= 0);
	str_size = (NumOfColumns + 2) * (ey - sy + 1);
	str_w = malloc(sizeof(wchar_t) * str_size);

	LockBuffer();

	str_w[0] = 0;
	TmpPtr = GetLinePtr(sy);
	k = 0;
	for (y = sy; y<=ey ; y++) {
		int IStart;		// 開始
		int IEnd;		// 終了
		BOOL LineContinued;

		if (box_select) {
			IStart = sx;
			IEnd = ex - 1;
			LineContinued = FALSE;
		}
		else {
			// 行選択
			IStart = (y == sy) ? sx : 0;
			LineContinued = FALSE;
			if (y == ey) {
				// 1行選択時、又は
				// 複数行選択時の最後の行
				IEnd = ex - 1;
			}
			else {
				// 複数行選択時の途中の行
				// 行末まで選択されている
				IEnd = NumOfColumns - 1;

				// 継続行コピー設定
				if (ts.EnableContinuedLineCopy) {
					LONG NextTmpPtr = NextLinePtr(TmpPtr);
					if ((CodeBuffW[NextTmpPtr].attr & AttrLineContinued) != 0) {
						// 次の行に継続している
						LineContinued = TRUE;
					}
				}
			}
		}

		// IEnd=コピーが必要な最後の位置
		if (LineContinued) {
			// 行の一番最後までコピーする
			IEnd++;
		}
		else {
			// 次の行に継続していないなら、スペースを削除する
			while (IEnd >= IStart) {
				// コピー不要な" "(0x20)を削除
				const buff_char_t *b = &CodeBuffW[TmpPtr + IEnd];
				if (b->u32 != 0x20) {
					// スペース以外だった
					IEnd++;
					break;
				}
				if (IEnd == 0) {
					break;
				}
				// 切り詰める
				MoveCharPtr(TmpPtr,&IEnd,-1);
			}
		}

		// 1ライン文字列をコピーする
		//   IEnd=コピーが必要な最後の位置+1
		x = IStart;
		while (x < IEnd) {
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

/**
 *	1セル分をwchar_t文字列に展開する
 *	@param[in]		b			1セル分の文字情報へのポインタ
 *	@param[in,out]	buf			文字列展開先 NULLの場合は展開されない
 *	@param[in]		buf_size	bufの文字数(buff == NULLの場合は参照されない)
 *	@param[out]		too_small	NULL のとき情報を返さない
 *								TRUE	バッファサイズ不足
 *										戻り値は必要な文字数が返る
 *								FALSE	文字を展開できた
 *	@retrun			文字数		出力文字数
 *								0のとき、文字出力なし
 *
 *	TODO
 *		GetWCS() と同じ?
 */
static size_t expand_wchar(const buff_char_t *b, wchar_t *buf, size_t buf_size, BOOL *too_samll)
{
	size_t len;

	if (IsBuffPadding(b)) {
		if (too_samll != NULL) {
			*too_samll = FALSE;
		}
		return 0;
	}

	// 長さを測る
	len = 0;
	if (b->wc2[1] == 0) {
		// サロゲートペアではない
		len++;
	} else {
		// サロゲートペア
		len += 2;
	}
	// コンビネーション
	len += b->CombinationCharCount16;

	if (buf == NULL) {
		// 長さだけを返す
		return len;
	}

	// バッファに収まる?
	if (len > buf_size) {
		// バッファに収まらない
		if (too_samll != NULL) {
			*too_samll = TRUE;
		}
		return len;
	}
	if (too_samll != NULL) {
		*too_samll = FALSE;
	}

	// 展開していく
	*buf++ = b->wc2[0];
	if (b->wc2[1] != 0) {
		*buf++ = b->wc2[1];
	}
	if (b->CombinationCharCount16 != 0) {
		memcpy(buf, b->pCombinationChars16, b->CombinationCharCount16 * sizeof(wchar_t));
	}

	return len;
}

/**
 *	(x,y) の1文字が strと同一か調べる
 *		*注 1文字が複数のwchar_tから構成されている
 *
 *	@param		b
 *	@param		str		比較文字列(wchar_t)
 *	@param		len		比較文字列長
 *	@retval		マッチした文字列長
 *				0=マッチしなかった
 */
static size_t MatchOneStringPtr(const buff_char_t *b, const wchar_t *str, size_t len)
{
	int match_pos = 0;
	if (len == 0) {
		return 0;
	}
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
static size_t MatchOneString(int x, int y, const wchar_t *str, size_t len)
{
	int TmpPtr = GetLinePtr(y);
	const buff_char_t *b = &CodeBuffW[TmpPtr + x];
	return MatchOneStringPtr(b, str, len);
}

/**
 *	b から strと同一か調べる
 *
 *	@param		b		バッファへのポインタ、ここから検索する
 *	@param		LineCntinued	TRUE=行の継続を考慮する
 *	@retval		TRUE	マッチした
 *	@retval		FALSE	マッチしていない
 */
#if 0
static BOOL MatchStringPtr(const buff_char_t *b, const wchar_t *str, BOOL LineContinued)
{
	int x;
	int y;
	BOOL result;
	size_t len = wcslen(str);
	if (len == 0) {
		return FALSE;
	}
	GetPosFromPtr(b, &x, &y);
	for(;;) {
		size_t match_len;
		if (IsBuffPadding(b)) {
			b++;
			continue;
		}
		// 1文字同一か調べる
		match_len = MatchOneString(x, y, str, len);
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
			if (LineContinued && ((b->attr & AttrLineContinued) != 0)) {
				// 次の行へ
				y++;
				if (y == NumOfLines) {
					// バッファ最終端
					return 0;
				}
				x = 0;
				b = &CodeBuffW[GetLinePtr(y)];
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
 *	(x,y)から strと同一か調べる
 *
 *	@param		x		マイナスの時、上の行が対象になる
 *	@param		y		PageStart + CursorY
 *	@param		LineCntinued	TRUE=行の継続を考慮する
 *	@retval		TRUE	マッチした
 *	@retval		FALSE	マッチしていない
 */
static BOOL MatchString(int x, int y, const wchar_t *str, BOOL LineContinued)
{
	BOOL result;
	int TmpPtr = GetLinePtr(y);
	size_t len = wcslen(str);
	if (len == 0) {
		return FALSE;
	}
	while(x < 0) {
		if (LineContinued && (CodeBuffW[TmpPtr+0].attr & AttrLineContinued) == 0) {
			// 行が継続しているか考慮 & 継続していない
			x = 0;	// 行頭からとする
			break;
		}
		TmpPtr = PrevLinePtr(TmpPtr);
		x += NumOfColumns;
		y--;
	}
	while(x > NumOfColumns) {
		if (LineContinued && (CodeBuffW[TmpPtr+NumOfColumns-1].attr & AttrLineContinued) == 0) {
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
			if (LineContinued && (CodeBuffW[TmpPtr+NumOfColumns-1].attr & AttrLineContinued) != 0) {
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

/**
 *	(sx,sy)から(ex,ey)までで str にマッチする文字を探して
 *	位置を返す
 *
 *	@param		sy,ex	PageStart + CursorY
 *	@param[out]	x		マッチした位置
 *	@param[out]	y		マッチした位置
 *	@retval		TRUE	マッチした
 */
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


/**
 *	クリップボード用文字列取得
 *	@return		文字列
 *				使用後は free() すること
 */
wchar_t *BuffCBCopyUnicode(BOOL Table)
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
	return str_ptr;
}

void BuffPrint(BOOL ScrollRegion)
// Print screen or selected text
{
	int Id;
	POINT PrintStart, PrintEnd;
	int j;
	int IStart, IEnd;
	LONG TmpPtr;
	vtdraw_t *vt;
	ttdc_t *dc;

	if (ScrollRegion) {
		vt = VTPrintInit(IdPrnScrollRegion, &dc, &Id);
	}
	else if (Selected) {
		vt = VTPrintInit(IdPrnScreen | IdPrnSelectedText, &dc, &Id);
	}
	else {
		vt = VTPrintInit(IdPrnScreen, &dc, &Id);
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

	// スクロールしている分(スクロールバーで古いバッファを見ている等)ずらす
	PrintStart.y += WinOrgY;
	PrintEnd.y += WinOrgY;

	if (PrintEnd.y > BuffEnd-1) {
		PrintEnd.y = BuffEnd-1;
	}

	LockBuffer();

	int line_in_page = 0;
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

		int height;
		DispGetCellSize(vt, NULL, &height);
		DispSetDrawPos(vt, dc, 0, line_in_page * height);

		BuffDrawLineI(vt, dc, j, IStart, IEnd);
		TmpPtr = NextLinePtr(TmpPtr);
		line_in_page++;
		if (DispPrnIsNextPage(vt, dc)) {
			// 次のページへ
			line_in_page = 0;
			HDC hdc = DispDCGetRawDC(dc);
			EndPage(hdc);
			StartPage(hdc);
		}

		if (PrnCheckAbort()) {
			break;
		}
	}
	UnlockBuffer();
	VTPrintEnd(vt, dc);
}

// TODO とりあえず ANSI で実装
// Dumps current line to the file (for path through printing)
//   HFile: file handle
//   TERM: terminator character
//	= LF or VT or FF
void BuffDumpCurrentLine(PrintFile *handle, BYTE TERM)
{
	int i, j;
	buff_char_t *b = &CodeBuffW[LinePtr];
	char bufA[TermWidthMax+1];
	char *p = bufA;

	i = NumOfColumns;
	while ((i>0) && (b[i-1].ansi_char == 0x20)) {
		i--;
	}
	p = bufA;
	for (j=0; j<i; j++) {
		unsigned short c = b[j].ansi_char;
		*p++ = (c & 0xff);
		if (c > 0x100) {
			*p++ = (c & 0xff);
		}
	}
	p = bufA;
	for (j=0; j<i; j++) {
		WriteToPrnFile(handle, bufA[j],FALSE);
	}
	WriteToPrnFile(handle, 0,TRUE);
	if ((TERM>=LF) && (TERM<=FF)) {
		WriteToPrnFile(handle, 0x0d,FALSE);
		WriteToPrnFile(handle, TERM,TRUE);
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

static BOOL BuffIsHalfWidthFromPropery(const TTTSet *ts_, char width_property)
{
	switch (width_property) {
	case 'H':	// Halfwidth
	case 'n':	// Narrow
	case 'N':	// Neutral
	default:
		return TRUE;
	case 'A':	// Ambiguous 曖昧
		if (ts_->UnicodeAmbiguousWidth == 2) {
			// 全角として扱う
			return FALSE;
		}
		return TRUE;
	case 'W':
	case 'F':
		return FALSE;		// 全角
	}
}

static BOOL BuffIsHalfWidthFromCode(const TTTSet *ts_, unsigned int u32, char *width_property, char *emoji)
{
	int width;
	*width_property = UnicodeGetWidthProperty(u32);
	*emoji = (char)UnicodeIsEmoji(u32);
	if (ts_->KanjiCode == IdUTF8 && ts_->UnicodeOverrideCharWidthEnable != 0 &&
		UnicodeOverrideWidthCheck(u32, &width) == TRUE) {
		return width == 1 ? TRUE : FALSE;
	}
	if (ts_->UnicodeEmojiOverride) {
		if (*emoji) {
			// 絵文字だった場合
			if (u32 < 0x1f000) {
				if (ts_->UnicodeEmojiWidth == 2) {
					// 全角
					return FALSE;
				}
				else {
					// 半角
					return TRUE;
				}
			}
			else {
				// 常に全角
				return FALSE;
			}
		}
	}
	return BuffIsHalfWidthFromPropery(ts_, *width_property);
}

/**
 *	カーソル位置とのURLアトリビュートの先頭との距離を計算する
 */
static int get_url_len(int cur_x, int cur_y)
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
			if ((CodeBuffW[ptr].attr & AttrURL) == 0) {
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
			CodeBuffW[TmpPtr + x].attr |= AttrURL;
		}
	}
	*sx_match_e = rx;
	return TRUE;
}

/**
 *	カーソル位置の文字列をURL強調する
 *	この関数がコールされたとき、カーソル位置の1つ前はURL強調されている
 *
 *	@param cur_x	カーソル位置
 *	@param cur_y	カーソル位置(!バッファ位置)
 */
static void mark_url_line_w(int cur_x, int cur_y)
{
	int sx;
	int sy;
	int ex;
	int ey;
	LONG TmpPtr;
	const buff_char_t *b;

	// URL強調の先頭を探す
	TmpPtr = GetLinePtr(PageStart + cur_y) + cur_x - 1;	// カーソル位置をポインタへ
	while ((CodeBuffW[TmpPtr].attr & AttrURL) != 0) {
		if (TmpPtr == 0) {
			break;
		}
		TmpPtr--;
	}
	TmpPtr++;

	// ポインタをカーソル位置へ
	GetPosFromPtr(&CodeBuffW[TmpPtr], &sx, &sy);
	if (sy >= PageStart) {
		sy = sy - PageStart;
	} else {
		sy = sy - PageStart;
		sy = sy + NumOfLinesInBuff;
	}

	// 行末を探す
	ex = NumOfColumns - 1;
	ey = cur_y;
	if (cur_y <= NumOfLines - 1) {
		TmpPtr = GetLinePtr(PageStart + ey);
		while ((CodeBuffW[TmpPtr + NumOfColumns - 1].attr & AttrLineContinued) != 0) {
			ey++;
			TmpPtr = NextLinePtr(TmpPtr);
		}
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
			if (y == ey) {
				ex_i = ex;
			}
			TmpPtr = GetLinePtr(PageStart + y);
			for (x = sx_i; x < ex_i; x++) {
				CodeBuffW[TmpPtr + x].attr &= ~AttrURL;
			}
		}
	}

	// マークする
	ttdc_t *dc = DispInitDC(vt_src);
	{
	int sx_i = sx;
	int sy_i = sy;
	int i;
	for (i=0; i<1000000; i++) {
		int sx_match_s, sx_match_e;
		int sy_match_s, sy_match_e;
		BOOL match;

		if ((sy_i > ey) || (sy_i == ey && sx_i >= ex)) {
			break;
		}

		match = mark_url_w_sub(sx_i, ex, sy_i, ey, &sx_match_s, &sx_match_e, &sy_match_s, &sy_match_e);
		if (match) {
			if (sy_match_s == sy_match_e) {
				BuffDrawLineI(vt_src, dc, sy_match_s, sx_match_s, sx_match_e);
			}
			else {
				BuffDrawLineI(vt_src, dc, sy_match_s, sx_match_s, sx_match_e);
			}
			sx_i = sx_match_e;
			sy_i = sy_match_e;
		}

		// 次のセルへ
		if (sx_i == NumOfColumns - 1) {
			if (sy_i == NumOfLines - 1) {
				break;
			}
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
			BuffDrawLineI(vt_src, dc, y + PageStart, sx_i, ex_i);
		}
	}
	DispReleaseDC(vt_src, dc);
}

/**
 *	(cur_x, cur_y)位置からURL強調を行う
 */
static void mark_url_w(int cur_x, int cur_y)
{
	buff_char_t * CodeLineW = &CodeBuffW[LinePtr];
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
		if ((CodeLineW[0].attr & AttrLineContinued) != 0) {
			const LONG TmpPtr = GetLinePtr(PageStart + cur_y - 1);
			if ((CodeBuffW[TmpPtr + NumOfColumns - 1].attr & AttrURL) != 0) {
				prev = TRUE;
			}
		}
	}
	else {
		if (CodeLineW[x - 1].attr & AttrURL) {
			prev = TRUE;
		}
	}

	// 1つ後ろのセルがURL?
	if (x == NumOfColumns - 1) {
		// 現在xが一番右?
		if ((cur_y + 1) < NumOfLines) {
			if ((CodeLineW[x].attr & AttrLineContinued) != 0) {
				const LONG TmpPtr = GetLinePtr(PageStart + cur_y + 1);
				if ((CodeBuffW[TmpPtr + NumOfColumns - 1].attr & AttrURL) != 0) {
					next = TRUE;
				}
			}
		}
	}
	else {
		if (CodeLineW[x + 1].attr & AttrURL) {
			next = TRUE;
		}
	}

	if (prev == TRUE) {
		if (next == TRUE) {
			if (isURLchar(u32)) {
				// URLにはさまれていて、URLになりえるキャラクタ
				int ptr = GetLinePtr(PageStart + cur_y) + cur_x;
				CodeBuffW[ptr].attr |= AttrURL;
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
				CodeLineW[x].attr |= AttrURL;
			}
			return;
		}
		mark_url_line_w(cur_x, cur_y);
		return;
	}

	// '/' が入力されたら調べ始める
	if (u32 != '/') {
		return;
	}
	if (!MatchString(x - 2, PageStart + CursorY, L"://", TRUE)) {
		// "://" の一部ではない
		return;
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
			if ((CodeLineW[0].attr & AttrLineContinued) == 0) {
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
			CodeLineW[sx + i].attr |= AttrURL;
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
			CodeBuffW[TmpPtr + xx].attr |= AttrURL;
			xx++;
			if (xx == NumOfColumns) {
				int draw_x = sx;
				int draw_y = CursorY - 1;
				if (IsLineVisible(vt_src, &draw_x, &draw_y)) {
					ttdc_t *dc = DispInitDC(vt_src);
					BuffDrawLineI(vt_src, dc, PageStart + CursorY - 1, sx, NumOfColumns - 1);
					DispReleaseDC(vt_src, dc);
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

/**
 *	1セル分をwchar_t文字列に展開する
 *	@param[in]		b			1セル分の文字情報へのポインタ
 *	@retval			展開した文字列
 *
 *	TODO
 *		expand_wchar() と同じ?
 */
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
		*p++ = b->wc2[1];
	}
	for (i=0; i<b->CombinationCharCount16; i++) {
		*p++ = b->pCombinationChars16[i];
	}
	*p = L'\0';
	return strW;
}

/**
 *	(x,y)にu32を入れるとき、結合するか?
 *  @param[in]		wrap		TRUE wrap中
 *  @param[in]		u32			Unicode
 *	@param[in,out]	combine		u32の文字種を返す(NULL 結果を返さない)
 *								0	結合しない
 *								1	結合文字,Nonspacing Mark, カーソルは移動しない
 *								2	結合文字,Spacing Mark, カーソルが +1 移動する
 *	@return	結合する文字へのポインタ
 *								1(半角) or 2(全角) or N セル前
 *								現在のセル (x が行末で wrap == TRUE 時)
 *	@return	NULL	結合しない
 */
static buff_char_t *IsCombiningChar(int x, int y, BOOL wrap, unsigned int u32, int *combine)
{
	buff_char_t *p = NULL;  // NULLのとき、前の文字はない
	LONG LinePtr_ = GetLinePtr(PageStart+y);
	buff_char_t *CodeLineW = &CodeBuffW[LinePtr_];
	int combine_type;	// 0 or 1 or 2

	combine_type = (u32 == 0x200d) ? 1 : 0;		// U+200d = ゼロ幅接合子,ZERO WIDTH JOINER(ZWJ)
	if (combine_type == 0) {
		combine_type = UnicodeIsCombiningCharacter(u32);
	}
	if (combine != NULL) {
		*combine = combine_type;
	}

	if (x == NumOfColumns - 1 && wrap) {
		// 現在位置に結合する
		p = &CodeLineW[x];
		if (IsBuffPadding(p)){
			p--;
		}
	}
	else {
		if (x == 0) {
			// 前のもじはない
			p = NULL;
		}
		else {
			// paddingではないセルを探す
			x = LeftHalfOfDBCS(LinePtr_, x - 1);		// 1cell前から調べる
			if (!IsBuffPadding(&CodeLineW[x])) {
				p = &CodeLineW[x];
			}
			else {
				// 前のもじはない
				p = NULL;
			}
		}
	}

	// paddingではない前のセルあり?
	if (p == NULL) {
		// 前がないので結合できない
		return NULL;
	}

	// 結合する?
	// 		1つ前が ZWJ
	if (combine_type != 0 || (p->u32_last == 0x200d)) {
		return p;
	}

	// ヴィラーマ処理
	if (UnicodeIsVirama(p->u32_last) != 0) {
		// 1つ前のヴィラーマと同じ block の文字である
		int block_index_last = UnicodeBlockIndex(p->u32_last);
		int block_index = UnicodeBlockIndex(u32);
#if 0
		OutputDebugPrintf("U+%06x, %d, %s\n", p->u32_last, block_index_last, UnicodeBlockName(block_index_last));
		OutputDebugPrintf("U+%06x, %d, %s\n", u32, block_index, UnicodeBlockName(block_index));
#endif
		if (block_index_last == block_index) {
			return p;
		}
	}
	return NULL;
}

BOOL BuffIsCombiningCharacter(int x, int y, unsigned int u32)
{
	buff_char_t *p = IsCombiningChar(x, y, Wrap, u32, NULL);
	return p != NULL;
}

/**
 *	Unicodeから ANSI に変換する
 *		結合文字(combining character)も行う
 */
static unsigned short ConvertACPChar(const buff_char_t *b)
{
	char *strA;
	unsigned short chA;
	size_t lenA;
	size_t pool_lenW = 128;
	wchar_t *strW = (wchar_t *)malloc(pool_lenW * sizeof(wchar_t));
	BOOL too_small = FALSE;
	size_t lenW = expand_wchar(b, strW, pool_lenW, &too_small);
	if (too_small) {
		strW = (wchar_t *)realloc(strW, lenW * sizeof(wchar_t));
		expand_wchar(b, strW, lenW, &too_small);
	}

	if (lenW >= 2) {
		// WideCharToMultiByte() では結合処理は行われない
		// 自力で結合処理を行う。ただし、最初の2文字だけ
		//	 例1:
		//		U+307B(ほ) + U+309A(゜) は
		//		Shift jis の 0x82d9(ほ) と 0x814b(゜) に変換され
		//		0x82db(ぽ) には変換されない
		//		予め U+307D(ぽ)に正規化しておく
		//	 例2:
		//		U+0061 U+0302 -> U+00E2 (latin small letter a with circumflex) (a+^)
		unsigned short c = UnicodeCombining(strW[0], strW[1]);
		if (c != 0) {
			// 結合できた
			strW[0] = c;
			strW[1] = 0;
		}
	}
	strA = _WideCharToMultiByte(strW, lenW, CodePage, &lenA);
	chA = *(unsigned char *)strA;
	if (!IsDBCSLeadByte((BYTE)chA)) {
		// 1byte文字
		chA = strA[0];
	}
	else {
		// 2byte文字
		chA = (chA << 8) | ((unsigned char)strA[1]);
	}
	free(strA);
	free(strW);

	return chA;
}

/**
 *	ユニコードキャラクタを1文字バッファへ入力する
 *	@param[in]	u32		unicode character(UTF-32)
 *	@param[in]	Attr	attributes
 *	@param[in]	Insert	Insert flag
 *	@return		カーソル移動量(0 or 1 or 2)
 */
int BuffPutUnicode(unsigned int u32, const TCharAttr *Attr, BOOL Insert)
{
	buff_char_t * CodeLineW = &CodeBuffW[LinePtr];
	int move_x = 0;
	static BOOL show_str_change = FALSE;
	buff_char_t *p;
	int combining_type;
	BYTE Attr_Attr = Attr->Attr;

	assert(Attr_Attr == (Attr->AttrEx & 0xff));

#if 0
	OutputDebugPrintfW(L"BuffPutUnicode(U+%06x,(%d,%d)\n", u32, CursorX, CursorY);
#endif

	if (u32 < 0x20 || (0x80 <= u32 && u32 <= 0x9f)) {
		// C0/C1 Controls は文字として処理しない
		//assert(FALSE);	// 入ってくるのはおかしい
		return 0;
	}

	if (ts.EnableContinuedLineCopy && CursorX == 0 && (CodeLineW[0].attr & AttrLineContinued)) {
		Attr_Attr |= AttrLineContinued;
	}

	// 結合文字 or 1つ前の文字の影響で結合する?
	combining_type = 0;
	p = IsCombiningChar(CursorX, CursorY, Wrap, u32, &combining_type);
	if (p != NULL || combining_type != 0) {
		// 結合する
		BOOL add_base_char = FALSE;
		move_x = 0;  // カーソル移動量=0

		if (p == NULL) {
			// 前のもじ(基底文字)がないのに結合文字が出てきたとき
			// NBSP(non-breaking space) U+00A0 に結合させる
			add_base_char = TRUE;
			p = &CodeLineW[CursorX];
			BuffSetChar(p, 0xa0, 'H');

			move_x = 1;  // カーソル移動量=1
		}

		// 入力文字は、Nonspacing mark 以外?
		//		カーソルを+1, 文字幅を+1する
		if (p->u32_last != 0x200d && combining_type != 1) {
			// カーソル移動量は1
			move_x = 1;

			p->cell++;
			if(StrChangeCount == 0) {
				// 描画範囲がクリアされている、再度設定する
				StrChangeCount = p->cell;
				if (CursorX == 0) {
					// カーソルが左端の時
					StrChangeStart = 0;
				}
				else {
					StrChangeStart = CursorX - StrChangeCount + 1;
				}
			}
			else {
				// 描画範囲を1cell増やす
				StrChangeCount++;
			}

			// カーソル位置の文字は Paddingにする
			//	ただし次の時は Padding を入れない
			//	- 行末のとき (TODO この条件は不要?)
			//	- 基底文字がある状態で、Spacing Mark文字(カーソルが+1移動する結合文字)が入力されたとき
			if (CursorX < NumOfColumns - 1) {
				if (add_base_char == FALSE) {
					BuffSetChar(&CodeLineW[CursorX], 0, 'H');
					CodeLineW[CursorX].Padding = TRUE;
					CodeLineW[CursorX].attr = Attr_Attr;
					CodeLineW[CursorX].attr2 = Attr->Attr2;
					CodeLineW[CursorX].fg = Attr->Fore;
					CodeLineW[CursorX].bg = Attr->Back;
				}
			}
		}

		// 前の文字にくっつける
		BuffAddChar(p, u32);

		// 文字描画
		if (StrChangeCount == 0) {
			// 描画予定がない(StrChangeCount==0)のに、
			// 結合文字を受信した場合、描画する
			if (Wrap) {
				if (!BuffIsHalfWidthFromPropery(&ts, p->WidthProperty)) {
					// 行末に2セルの文字が描画済み、2セルの右側にカーソルがある状態
					StrChangeStart = CursorX - 1;
					StrChangeCount = 2;
				}
				else {
					// 行末に1セルの文字が描画されている、その上にカーソルがある状態
					StrChangeStart = CursorX;
					StrChangeCount = 1;
				}
			}
			else {
				StrChangeCount = p->cell;
				if (CursorX == 0) {
					// カーソルが左端の時
					StrChangeStart = 0;
				}
				else {
					StrChangeStart = CursorX - StrChangeCount;
				}
			}
		}

		// ANSI文字コードを更新
		p->ansi_char = ConvertACPChar(p);
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
			// 行末に全角(2cell)以上の文字が存在する可能性がある
			BuffSetChar(p, ' ', 'H');
			if (CursorX < NumOfColumns - 1) {
				BuffSetChar(p + 1, ' ', 'H');
			}
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

		{
			buff_char_t *p1 = GetPtrRel(CodeBuffW, BufferSize, p, 1);

			// 次の文字が全角 && 入力文字が全角 ?
			if (!Insert && !half_width && IsBuffFullWidth(p1)) {
				// 全角を潰す
				buff_char_t *p2 = GetPtrRel(CodeBuffW, BufferSize, p1, 1);
				BuffSetChar(p1, ' ', 'H');
				BuffSetChar(p2, ' ', 'H');
			}
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
			if (LineEnd <= NumOfColumns - 1 && (CodeLineW[LineEnd - 1].attr & AttrKanji)) {
				BuffSetChar(&CodeLineW[LineEnd - 1], 0x20, 'H');
				CodeLineW[LineEnd].attr &= ~AttrKanji;
				//				CodeLine[LineEnd+1] = 0x20;
				//				AttrLine[LineEnd+1] &= ~AttrKanji;
				extr = 1;
			}

			if (!half_width) {
				MoveLen = LineEnd - CursorX - 1;
				if (MoveLen > 0) {
					memmoveW(&(CodeLineW[CursorX + 2]), &(CodeLineW[CursorX]), MoveLen);
				}
			}
			else {
				MoveLen = LineEnd - CursorX;
				if (MoveLen > 0) {
					memmoveW(&(CodeLineW[CursorX + 1]), &(CodeLineW[CursorX]), MoveLen);
				}
			}

			BuffSetChar2(&CodeLineW[CursorX], u32, width_property, half_width, emoji);
			CodeLineW[CursorX].attr = Attr_Attr;
			CodeLineW[CursorX].attr2 = Attr->Attr2;
			CodeLineW[CursorX].fg = Attr->Fore;
			CodeLineW[CursorX].bg = Attr->Back;
			if (!half_width && CursorX < LineEnd) {
				BuffSetChar(&CodeLineW[CursorX + 1], 0, 'H');
				CodeLineW[CursorX + 1].Padding = TRUE;
				CodeLineW[CursorX + 1].attr = Attr_Attr;
				CodeLineW[CursorX + 1].attr2 = Attr->Attr2;
				CodeLineW[CursorX + 1].fg = Attr->Fore;
				CodeLineW[CursorX + 1].bg = Attr->Back;
			}
#if 0
			/* begin - ishizaki */
			markURL(CursorX);
			markURL(CursorX+1);
			/* end - ishizaki */
#endif

			/* last char in current line is kanji first? */
			if ((CodeLineW[LineEnd].attr & AttrKanji) != 0) {
				/* then delete it */
				BuffSetChar(&CodeLineW[LineEnd], 0x20, 'H');
				CodeLineW[LineEnd].attr = CurCharAttr.Attr;
				CodeLineW[LineEnd].attr2 = CurCharAttr.Attr2;
				CodeLineW[LineEnd].fg = CurCharAttr.Fore;
				CodeLineW[LineEnd].bg = CurCharAttr.Back;
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
			if ((Attr->AttrEx & AttrPadding) != 0) {
				// 詰め物
				buff_char_t *b = &CodeLineW[CursorX];
				BuffSetChar(b, u32, 'H');
				b->Padding = TRUE;
				b->attr = Attr_Attr;
				b->attr2 = Attr->Attr2;
				b->fg = Attr->Fore;
				b->bg = Attr->Back;
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

				BuffSetChar2(&CodeLineW[CursorX], u32, width_property, half_width, emoji);
				if (half_width) {
					CodeLineW[CursorX].attr = Attr_Attr;
				}
				else {
					// 全角
					CodeLineW[CursorX].attr = Attr_Attr | AttrKanji;
				}
				CodeLineW[CursorX].attr2 = Attr->Attr2;
				CodeLineW[CursorX].fg = Attr->Fore;
				CodeLineW[CursorX].bg = Attr->Back;

				if (!half_width) {
					// 全角の時は次のセルは詰め物
					if (CursorX < NumOfColumns - 1) {
						buff_char_t *b = &CodeLineW[CursorX + 1];
						BuffSetChar(b, 0, 'H');
						b->Padding = TRUE;
						b->attr = 0;
						b->attr2 = 0;
						b->fg = 0;
						b->bg = 0;
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
		}
	}

	if (show_str_change) {
		OutputDebugPrintf("StrChangeStart,Count %d,%d\n", StrChangeStart, StrChangeCount);
	}

	return move_x;
}

static BOOL CheckSelect(int x, int y)
//  subroutine called by BuffUpdateRect
{
	LONG L, L1, L2;

	if (BoxSelect) {
		return (Selected &&
				(((SelectStart.x <= x) && (x < SelectEnd.x)) || ((SelectEnd.x <= x) && (x < SelectStart.x))) &&
				(((SelectStart.y <= y) && (y <= SelectEnd.y)) || ((SelectEnd.y <= y) && (y <= SelectStart.y))));
	}
	else {
		L = MAKELONG(x, y);
		L1 = MAKELONG(SelectStart.x, SelectStart.y);
		L2 = MAKELONG(SelectEnd.x, SelectEnd.y);

		return (Selected && (((L1 <= L) && (L < L2)) || ((L2 <= L) && (L < L1))));
	}
}

static int TCharAttrCmp(const TCharAttr *a, const TCharAttr *b)
{
	if (a->Attr == b->Attr &&
		a->Attr2 == b->Attr2 &&
		a->Fore == b->Fore &&
		a->Back == b->Back)
	{
		return 0;
	}
	else {
		return 1;
	}
}

/**
 *	1行描画
 *
 *	@param	SY				スクリーン上の位置(Character)  !バッファ上の位置
 *							PageStart + YStart など
 *	@param	IStart,IEnd		スクリーン上の位置(Character)
 *							指定した間を描画する
 *  @param	disp_strW()		wchar_t 文字を描画用関数 (Unicode用)
 *  @param	disp_strA()		char 文字を描画用関数 (ANSI用)
 *  @param	disp_setup_dc()	アトリビュート設定関数
 */
static
void BuffGetDrawInfoW(vtdraw_t *vt, ttdc_t *dc, int SY, int IStart, int IEnd,
					  void (*disp_strW)(vtdraw_t *vt, ttdc_t *dc, const wchar_t *bufW, const char *width_info, int count),
					  void (*disp_strA)(vtdraw_t *vt, ttdc_t *dc, const char *buf, const char *width_info, int count))
{
	const LONG TmpPtr = GetLinePtr(SY);
	int istart = IStart;
	char bufA[TermWidthMax+1];
	char bufAW[TermWidthMax+1];
	wchar_t bufW[TermWidthMax+1];
	char bufWW[TermWidthMax+1];
	int lenW = 0;
	int lenA = 0;
	TCharAttr CurAttr;
	BOOL CurAttrEmoji;
	BOOL CurSelected;
	BOOL EndFlag = FALSE;
	int count = 0;		// 現在注目している文字,IStartから
#if 0
	OutputDebugPrintf("BuffGetDrawInfoW(%d,%d-%d)\n", SY, IStart, IEnd);
#endif
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
			if (IsBuffPadding(b)) {
				// 最初に表示しようとした文字が2cellの右側だった場合
				// assert(FALSE);
				ptr--;
			}
			CurAttr.Attr = CodeBuffW[ptr].attr & ~ AttrKanji;
			CurAttr.Attr2 = CodeBuffW[ptr].attr2;
			CurAttr.Fore = CodeBuffW[ptr].fg;
			CurAttr.Back = CodeBuffW[ptr].bg;
			CurAttrEmoji = b->Emoji;
			CurSelected = CheckSelect(istart+count,SY);
		}

		if (IsBuffPadding(b)) {
			// 2cellの次の文字,処理不要
		} else {
			if (count == 0) {
				// 最初の1文字目
				SetString = TRUE;
			} else {
				TCharAttr TempAttr;
				TempAttr.Attr = CodeBuffW[TmpPtr+istart+count].attr & ~ AttrKanji;
				TempAttr.Attr2 = CodeBuffW[TmpPtr+istart+count].attr2;
				TempAttr.Fore = CodeBuffW[TmpPtr + istart + count].fg;
				TempAttr.Back = CodeBuffW[TmpPtr + istart + count].bg;
				if (b->u32 != 0 &&
					((TCharAttrCmp(&CurAttr, &TempAttr) != 0 || CurAttrEmoji != b->Emoji) ||
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
				bufWW[lenW] = b->cell;
				lenW++;
			} else {
				// UTF-16でサロゲートペア
				bufW[lenW] = b->wc2[0];
				bufWW[lenW] = 0;
				lenW++;
				bufW[lenW] = b->wc2[1];
				bufWW[lenW] = b->cell;
				lenW++;
			}
			if (b->CombinationCharCount16 != 0) {
				// コンビネーション
				int i;
				const char cell_tmp = bufWW[lenW - 1];
				bufWW[lenW - 1] = 0;
				for (i = 0; i < (int)b->CombinationCharCount16; i++) {
					bufW[lenW + i] = b->pCombinationChars16[i];
					bufWW[lenW + i] = 0;
				}
				bufWW[lenW + b->CombinationCharCount16 - 1] = cell_tmp;
				lenW += b->CombinationCharCount16;
				DrawFlag = TRUE;  // コンビネーションがある場合はすぐ描画
			}

			// ANSI版
			{
				unsigned short ansi_char = b->ansi_char;
				int i;
				char cell = b->cell;
				int c = 0;
				if (ansi_char < 0x100) {
					bufA[lenA] = ansi_char & 0xff;
					bufAW[lenA] = cell;
					lenA++;
					c++;
				}
				else {
					bufA[lenA] = (ansi_char >> 8) & 0xff;
					bufAW[lenA] = cell;
					lenA++;
					c++;
					bufA[lenA] = ansi_char & 0xff;
					bufAW[lenA] = 0;
					lenA++;
					c++;
				}
				// ANSI文字列で表示できるのは 1or2cell(半角or全角)
				// 残りは '?' を表示する
				for (i = c; i < cell; i++) {
					bufA[lenA] = '?';
					bufAW[lenA] = 0;
					lenA++;
				}
			}

			if (b->WidthProperty == 'A' || b->WidthProperty == 'N') {
				DrawFlag = TRUE;
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

			DispSetupDC(vt, dc, &CurAttr, CurSelected);
			if (UseUnicodeApi) {
				disp_strW(vt, dc, bufW, bufWW, lenW);
			}
			else {
				disp_strA(vt, dc, bufA, bufAW, lenA);
			}

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

/**
 *	1行描画 画面用
 *
 *	@param	SY				スクリーン上の位置(Character)  !バッファ上の位置
 *							PageStart + YStart など
 *	@param	IStart,IEnd		スクリーン上の位置(Character)
 *							指定した間を描画する
 */
static void BuffDrawLineI(vtdraw_t *vt, ttdc_t *dc, int SY, int IStart, int IEnd)
{
	if (!DispDCIsPrinter(dc)) {
		// 画面描画
		//	カーソル位置、表示開始位置
		int X = IStart;
		int Y = SY - PageStart;
		//	画面上の描画位置(pixel)
		if (! IsLineVisible(vt, &X, &Y)) {
			// 描画不要行
			//assert(FALSE);
			return;
		}
		//	文字描画位置設定
		DispSetDrawPos(vt, dc, X, Y);
	} else {
		// プリンタの時
		//	何もしない(文字の位置は設定済み)
	}
	if (IEnd >= NumOfColumns) {
		IEnd = NumOfColumns - 1;
	}

	BuffGetDrawInfoW(vt, dc, SY, IStart, IEnd, DispStrW, DispStrA);
}

void BuffUpdateRect
  (int XStart, int YStart, int XEnd, int YEnd)
// Display text in a rectangular region in the screen
//   XStart: x position of the upper-left corner (screen cordinate)
//   YStart: y position
//   XEnd: x position of the lower-right corner (last character)
//   YEnd: y position
{
	int j;
	int IStart, IEnd;
	LONG TmpPtr;
	BOOL TempSel, Caret;

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

	ttdc_t *dc = DispInitDC(vt_src);

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

	TempSel = FALSE;

	Caret = IsCaretOn();
	if (Caret) {
		CaretOff(vt_src);
	}

	TmpPtr = GetLinePtr(PageStart+YStart);
	for (j = YStart+PageStart ; j <= YEnd+PageStart ; j++) {
		IStart = XStart;
		IEnd = XEnd;

		IStart = LeftHalfOfDBCS(TmpPtr,IStart);

		BuffDrawLineI(vt_src, dc, j, IStart, IEnd);
		TmpPtr = NextLinePtr(TmpPtr);
	}
	if (Caret) {
		CaretOn(vt_src);
	}

	DispReleaseDC(vt_src, dc);
}

void UpdateStr(void)
// Display not-yet-displayed string
{
	int X, Y;

	assert(StrChangeStart >= 0);
	if (StrChangeCount==0) {
		return;
	}
	X = StrChangeStart;
	Y = CursorY;
	if (! IsLineVisible(vt_src, &X, &Y)) {
		StrChangeCount = 0;
		return;
	}

	ttdc_t *dc = DispInitDC(vt_src);
	BuffDrawLineI(vt_src, dc, PageStart + CursorY, StrChangeStart, StrChangeStart + StrChangeCount - 1);
	DispReleaseDC(vt_src, dc);
	StrChangeCount = 0;
}

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

void MoveRight(void)
/* move cursor right, but dont update screen.
  this procedure must be called from DispChar&DispKanji only */
{
	CursorX++;
	/* 最下行でだけ自動スクロールする */
	if (ts.AutoScrollOnlyInBottomLine == 0 || WinOrgY == 0) {
		DispScrollToCursor(CursorX, CursorY);
	}
}

void BuffSetCaretWidth(void)
{
	buff_char_t *CodeLineW = &CodeBuffW[LinePtr];
	BOOL DW;

	/* check whether cursor on a DBCS character */
	DW = (((BYTE)(CodeLineW[CursorX]).attr & AttrKanji) != 0);
	DispSetCaretWidth(DW);
}

void ScrollUp1Line(void)
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
			memcpyW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
			DestPtr = SrcPtr;
		}
		memsetW(&(CodeBuffW[SrcPtr]), 0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, linelen);

		if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1)
			BuffUpdateRect(CursorLeftM-extl, CursorTop, CursorRightM+extr, CursorBottom);
		else
			DispScrollNLines(vt_src, CursorTop, CursorBottom, -1);
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
			DispCountScroll(vt_src, n);
			return;
		}
		else if (CursorY <= CursorBottom) {
			/* 最下行でだけ自動スクロールする */
			if (ts.AutoScrollOnlyInBottomLine != 0 && NewOrgY != 0) {
				/* スクロールさせない場合の処理 */
				WinOrgY = WinOrgY-n;
				NewOrgY = WinOrgY;
				BuffScroll(n,CursorBottom);
				DispCountScroll(vt_src, n);
			} else {
				BuffScroll(n,CursorBottom);
				DispScrollNLines(vt_src, WinOrgY, CursorBottom, n);
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
				memmoveW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
				SrcPtr = NextLinePtr(SrcPtr);
				DestPtr = NextLinePtr(DestPtr);
			}
		}
		else {
			n = CursorBottom-CursorTop+1;
		}
		for (i = CursorBottom+1-n ; i<=CursorBottom; i++) {
			memsetW(&(CodeBuffW[DestPtr]), 0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, linelen);
			DestPtr = NextLinePtr(DestPtr);
		}
		if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1)
			BuffUpdateRect(CursorLeftM-extl, CursorTop, CursorRightM+extr, CursorBottom);
		else
			DispScrollNLines(vt_src, CursorTop, CursorBottom, n);
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
			DispCountScroll(vt_src, n);
		}
		else {
			BuffScroll(n,CursorBottom);
			DispScrollNLines(vt_src, WinOrgY, CursorBottom, n);
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
				memmoveW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
				SrcPtr = NextLinePtr(SrcPtr);
				DestPtr = NextLinePtr(DestPtr);
			}
		}
		else {
			n = CursorBottom - CursorTop + 1;
		}
		for (i = CursorBottom+1-n ; i<=CursorBottom; i++) {
			memsetW(&(CodeBuffW[DestPtr]), 0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, linelen);
			DestPtr = NextLinePtr(DestPtr);
		}

		if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1) {
			BuffUpdateRect(CursorLeftM-extl, CursorTop, CursorRightM+extr, CursorBottom);
		}
		else {
			DispScrollNLines(vt_src, CursorTop, CursorBottom, n);
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
			memmoveW(&(CodeBuffW[DestPtr]), &(CodeBuffW[SrcPtr]), linelen);
			SrcPtr = PrevLinePtr(SrcPtr);
			DestPtr = PrevLinePtr(DestPtr);
		}
	}
	else {
		n = CursorBottom - CursorTop + 1;
	}
	for (i = CursorTop+n-1; i>=CursorTop; i--) {
		memsetW(&(CodeBuffW[DestPtr]), 0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, linelen);
		DestPtr = PrevLinePtr(DestPtr);
	}

	if (CursorLeftM > 0 || CursorRightM < NumOfColumns-1) {
		BuffUpdateRect(CursorLeftM-extl, CursorTop, CursorRightM+extr, CursorBottom);
	}
	else {
		DispScrollNLines(vt_src, CursorTop, CursorBottom, -n);
	}
}

void BuffClearScreen(void)
{ // clear screen
	if (isCursorOnStatusLine) {
		BuffScrollNLines(1); /* clear status line */
	}
	else { /* clear main screen */
		UpdateStr();
		BuffScroll(NumOfLines-StatusLine,NumOfLines-1-StatusLine);
		DispScrollNLines(vt_src, WinOrgY, NumOfLines - 1 - StatusLine, NumOfLines - StatusLine);
	}
}

void BuffUpdateScroll(void)
// Updates scrolling
{
	UpdateStr();
	DispUpdateScroll(vt_src);
}

void CursorUpWithScroll(void)
{
	if (((0 < CursorY) && (CursorY < CursorTop)) || (CursorTop < CursorY)) {
		MoveCursor(CursorX, CursorY - 1);
	}
	else if (CursorY == CursorTop) {
		ScrollUp1Line();
	}
}

/**
 * 単語区切り文字判定
 *
 *	@retval	TRUE	区切り文字
 *	@retval	FALSE	区切り文字ではない
 */
static BOOL IsDelimiter(LONG Line, int CharPtr)
{
	const buff_char_t *b = &CodeBuffW[Line+CharPtr];
	wchar_t *wcs = GetWCS(b);
	wchar_t *findp = wcsstr(ts.DelimListW, wcs);
	BOOL find = (findp != NULL);
	free(wcs);
	return find;
}

static void GetMinMax(int i1, int i2, int i3, int *min, int *max)
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

static void invokeBrowserWithUrl(const wchar_t *url)
{
	static const wchar_t *webbrowser_schemes[] = {
		L"http://",
		L"https://",
		L"ftp://",
	};
	BOOL use_browser = FALSE;
	for (int i = 0; i < _countof(webbrowser_schemes); i++) {
		const wchar_t *scheme = webbrowser_schemes[i];
		if (wcsncmp(url, scheme, wcslen(scheme)) == 0) {
			use_browser = TRUE;
			break;
		}
	}

	if (use_browser && ts.ClickableUrlBrowser[0] != 0) {
		// ブラウザを使用する
		wchar_t *browser = ToWcharA(ts.ClickableUrlBrowser);
		wchar_t *param;
		aswprintf(&param, L"%hs %s", ts.ClickableUrlBrowserArg, url);

		HINSTANCE r = ShellExecuteW(NULL, NULL, browser, param, NULL, SW_SHOWNORMAL);
		free(param);
		free(browser);
		if (r >= (HINSTANCE)32) {
			// 実行できた
			return;
		}
		// コマンドの実行に失敗した場合は通常と同じ処理をする
	}

	ShellExecuteW(NULL, NULL, url, NULL, NULL, SW_SHOWNORMAL);
}

/**
 *	文字を探す,前方
 *
 *	@param[in]	sx				スタート
 *	@param[in]	sy
 *	@param[in]	is_finish()		終了条件を満たすときにTRUEを返す関数
 *	@param[in]	work			関数へ渡すポインタ
 *	@param[in]	line_continue	FALSE=行頭で検索を打ち切る
 *	@param[out]	destx			見つけた位置
 *	@param[out]	dexty
 */
static void SearchCharPrev(
	int sx, int sy, BOOL line_continue,
	BOOL (*is_finish)(LONG ptr, int sx, void *work),
	void *work,
	int *destx, int *desty)
{
	LONG ptr = GetLinePtr(sy);
	const buff_char_t *b;

	sx = LeftHalfOfDBCS(ptr, sx);
	b = CodeBuffW + ptr + sx;
	assert(b->Padding == FALSE);

	for (;;) {
		// 移動前を記憶
		int px = sx;
		int py = sy;

		// 1つ前のキャラクタへ移動
		if (sx <= 0) {
			// 行頭なら上の行へ
			if (!line_continue) {
				// 上の行へ継続しない、検索打ち切り
				sx = 0;
				break;
			}
			else {
				// 上の行から継続している?
				if (sy > 0 && (b->attr & AttrLineContinued)) {
					// 継続している,前の行の最後へ
					sy--;
					ptr = GetLinePtr(sy);
					sx = NumOfColumns - 1;
					sx = LeftHalfOfDBCS(ptr, sx);
					b = CodeBuffW + ptr + sx;
				}
				else {
					// 継続していない,打ち切り
					sx = 0;
					break;
				}
			}
		}
		else {
			// 1char前へ
			sx--;
			b--;
			// 2cell以上の文字のとき頭出しする
			while (b->Padding == TRUE) {
				sx--;
				b--;
				if (sx < 0) {
					// 行頭が Padding はないはず
					assert(0);
					sx = 0;
					goto break_all;
				}
			}
		}

		// 終了?
		if (is_finish(ptr, sx, work)) {
		break_all:
			// 1つ前に進む前の位置を返す
			sx = px;
			sy = py;
			break;
		}
	}
	*destx = sx;
	*desty = sy;
}

/**
 *	文字を探す,後方
 *
 *	@param[in]	sx				スタート
 *	@param[in]	sy
 *	@param[in]	is_finish()		終了条件を満たすときにTRUEを返す関数
 *	@param[in]	work			関数へ渡すポインタ
 *	@param[in]	line_continue	FALSE=行末で検索を打ち切る
 *	@param[out]	destx			見つけた位置
 *	@param[out]	dexty
 */
static void SearchCharNext(
	int sx, int sy, BOOL line_continue,
	BOOL (*is_finish)(LONG ptr, int sx, void *work),
	void *work,
	int *destx, int *desty)
{
	LONG ptr = GetLinePtr(sy);
	const buff_char_t *b = CodeBuffW + ptr + sx;

	for (;;) {
		int px = sx;
		int py = sy;

		// 次のキャラクタへ
		if (sx + b->cell > NumOfColumns - 1) {
			// 次に進むと行末?
			if (!line_continue) {
				// 検索打ち切り
				sx = NumOfColumns - 1;
				break;
			}
			else {
				// 下の行が継続している?
				b += b->cell - 1;
				if (sy < BuffEnd && (b->attr & AttrLineContinued)) {
					// 継続している
					sx = 0;
					sy++;
					ptr = GetLinePtr(sy);
					b = CodeBuffW + ptr;
				}
				else {
					// 検索打ち切り
					sx = NumOfColumns - 1;
					break;
				}
			}
		}
		else {
			sx += b->cell;
			b += b->cell;
			if (sx > NumOfColumns - 1) {
				// はみ出すのはおかしい
				sx = NumOfColumns - 1;
			}
		}

		// 終了?
		if (is_finish(ptr, sx, work)) {
			// 1つ前に進む前の位置を返す
			sx = px;
			sy = py;
			break;
		}
	}
	*destx = sx;
	*desty = sy;
}

static BOOL IsNotURLChar(LONG ptr, int x, void *vwork)
{
	(void)vwork;
	const buff_char_t *b = CodeBuffW + ptr + x;
	return (b->attr & AttrURL) ? FALSE : TRUE;
}

static void SearchURLPrev(int sx, int sy, int *destx, int *desty)
{
	SearchCharPrev(sx, sy, TRUE, IsNotURLChar, NULL, destx, desty);
}

static void SearchURLNext(int sx, int sy, int *destx, int *desty)
{
	SearchCharNext(sx, sy, TRUE, IsNotURLChar, NULL, destx, desty);
}

static void invokeBrowserW(int x, int y)
{
	wchar_t *url;
	int sx, sy;
	int ex, ey;
	SearchURLPrev(x, y, &sx, &sy);
	SearchURLNext(x, y, &ex, &ey);

	url = BuffGetStringForCB(sx, sy, ex + 1, ey, FALSE, NULL);
	invokeBrowserWithUrl(url);
	free(url);
}

void ChangeSelectRegion(void)
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
			CaretOff(vt_src);
		}
		BuffUpdateRect(TempStart.x,TempStart.y-PageStart,
		               TempEnd.x,TempEnd.y-PageStart);
		if (Caret) {
			CaretOn(vt_src);
		}
		SelectEndOld = SelectEnd;
		return;
	}

	if ((SelectEndOld.y < SelectEnd.y) || ((SelectEndOld.y == SelectEnd.y) && (SelectEndOld.x <= SelectEnd.x))) {
		TempStart = SelectEndOld;
		TempEnd.x = SelectEnd.x - 1;
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
		CaretOff(vt_src);
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
			ttdc_t *dc = DispInitDC(vt_src);
			BuffUpdateRect(IStart,j-PageStart,IEnd,j-PageStart);
			DispReleaseDC(vt_src, dc);
		}
	}
	if (Caret) {
		CaretOn(vt_src);
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

	CaretOff(vt_src);

	DispConvWinToScreen(vt_src, Xw, Yw, &X, &Y, NULL);
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
		if (CodeBuffW[TmpPtr+X].attr & AttrURL) {
			BoxSelect = FALSE;
			SelectEnd = SelectStart;
			ChangeSelectRegion();

			url_invoked = TRUE;
			invokeBrowserW(X, Y);

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

typedef struct delimiter_work_tag {
	BOOL start_delimiter;
	wchar_t *start_char;
	int start_cell;
} delimiter_work;

/**
 *	区切り文字チェック
 *
 *	@retval	TRUE	区切り文字になった
 *	@retval	FALSE	区切り文字ではない
 */
static BOOL CheckDelimiterChar(LONG ptr, int x, void *vwork)
{
	delimiter_work *work = (delimiter_work *)vwork;
	const buff_char_t *b = CodeBuffW + ptr + x;

	if (work->start_delimiter) {
		// 区切り文字からスタートした場合
		wchar_t *wcs = GetWCS(b);
		int r = wcscmp(wcs, work->start_char);
		free(wcs);
		return r != 0;
	}
	else {
		// 非区切り文字からスタートした場合
		if (IsDelimiter(ptr, x) ||
			(ts.DelimDBCS != 0 && ((b->cell == 1) != work->start_cell))) {
			// 現在位置
			//		区切り文字になった
			//		スタート文字のセル数と異なるセル数(1orその他)となった
			// 終了
			return TRUE;
		}
	}
	return FALSE;
}

/**
 *	文字の区切り(Delimiter)を探す,前方
 *
 *	@param[in]	sx				スタート
 *	@param[in]	sy
 *	@param[in]	line_continue	FALSE=行の頭尾で区切る
 *	@param[out]	destx			見つけた位置
 *	@param[out]	dexty
 */
static void SearchDelimiterPrev(int sx, int sy, BOOL line_continue, int *destx, int *desty)
{
	LONG ptr = GetLinePtr(sy);
	sx = LeftHalfOfDBCS(ptr, sx);
	const buff_char_t *b = CodeBuffW + ptr + sx;
	assert(b->Padding == FALSE);

	delimiter_work work = {0};
	work.start_delimiter = IsDelimiter(ptr, sx);
	work.start_cell = b->cell == 1;
	work.start_char = GetWCS(b);

	SearchCharPrev(sx, sy, line_continue, CheckDelimiterChar, &work, destx, desty);

	free(work.start_char);
}

/**
 *	文字の区切り(Delimiter)を探す,後方
 *
 *	@param[in]	sx				スタート
 *	@param[in]	sy
 *	@param[in]	line_continue	FALSE=行の頭尾で区切る
 *	@param[out]	destx			見つけた位置
 *	@param[out]	dexty
 */
static void SearchDelimiterNext(
	int sx, int sy, BOOL line_continue,
	int *destx, int *desty)
{
	LONG ptr = GetLinePtr(sy);
	sx = LeftHalfOfDBCS(ptr, sx);
	const buff_char_t *b = CodeBuffW + ptr + sx;
	assert(b->Padding == FALSE);

	delimiter_work work = {0};
	work.start_delimiter = IsDelimiter(ptr, sx);
	work.start_cell = b->cell == 1;
	work.start_char = GetWCS(b);

	SearchCharNext(sx, sy, line_continue, CheckDelimiterChar, &work, destx, desty);

	free(work.start_char);
}

void BuffDblClk(int Xw, int Yw)
//  Select a word at (Xw, Yw) by mouse double click
//    Xw: horizontal position in window coordinate (pixels)
//    Yw: vertical
{
	int X, Y, YStart, YEnd;
	int IStart, IEnd;
	LONG TmpPtr;

	CaretOff(vt_src);

	if (!Selecting) {
		SelectStart = ClickCell;
		Selecting = TRUE;
	}

	DispConvWinToScreen(vt_src, Xw,Yw,&X,&Y,NULL);
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

		{
			int dest_x;
			int dest_y;

			// 前方の区切りを探す
			SearchDelimiterPrev(IStart, YStart, ts.EnableContinuedLineCopy, &dest_x, &dest_y);
			IStart = dest_x;
			YStart = dest_y;

			// 後方の区切りを探す
			SearchDelimiterNext(IEnd, YEnd, ts.EnableContinuedLineCopy, &dest_x, &dest_y);
			IEnd = dest_x + 1; // 終端の一つ後ろ
			YEnd = dest_y;
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

	CaretOff(vt_src);

	DispConvWinToScreen(vt_src, 0, Yw, NULL, &Y, NULL);
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

	DispConvWinToScreen(vt_src, Xw, Yw, &X, &Y, &Right);
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

/**
 *	文字の存在するcell位置を返す
 *
 *	VT Window上の(x,y)のcell位置から文字の存在するcell位置を返す
 *	文字のcell数と、cell上の右側か左側か(right)を判断してcell位置を返す
 *
 *	@param[in]	x		VT Windows上の文字(セル)位置
 *	@param[in]	y		バッファ上のy(PageStart加算済み)
 *	@param[in]	right	FALSE/TRUE = cell上の左側/右側
 *	@return		文字のcell位置
 */
static POINT GetCharCell(int x, int y, BOOL right)
{
//	y = y + PageStart;

	// clip
	x = x < 0 ? 0 : x > NumOfColumns ? NumOfColumns : x;
	y = y < 0 ? 0 : y > BuffEnd - 1 ? BuffEnd - 1 : y;

	int ptr = GetLinePtr(y);
	int x_char = LeftHalfOfDBCS(ptr, x);		// 文字の先頭

	const buff_char_t *b = CodeBuffW + ptr + x_char;
	int cell = b->cell;
	if ((cell & 1) == 0) {
		// 偶数長cell文字(2cellなど)
		int pos = x - x_char;
		if (pos >= (cell / 2)) {
			// 文字の中央より右側なので、一つ右の文字を選択
			if ((x_char + cell) < (NumOfColumns - 1)) {
				x = x_char + cell;
			}
		}
	} else {
		// 奇数長cell文字(1cell,3cellなど)
		int x_th = x_char + (cell + 1) / 2 - 1;
		if (x == x_th) {
			// ちょうど真ん中のcellだった時
			if (right == FALSE) {
				// cellの左側なので、文字の先頭
				x = x_char;
			} else {
				// cellの右側なので、一つ右の文字を選択
				x = x_char + cell;
			}
		} else if (x < x_th) {
			// 文字の左側なので、文字の先頭
			x = x_char;
		} else {
			// 文字の右側なので、一つ右の文字を選択
			x = x_char + cell;
		}
	}

	POINT pos;
	pos.x = x;
	pos.y = y;
	return pos;
}

void BuffStartSelect(int Xw, int Yw, BOOL Box)
//  Start text selection by mouse button down
//    Xw: horizontal position in window coordinate (pixels)
//    Yw: vertical
//    Box: Box selection if TRUE
{
	int X, Y;
	BOOL Right;

	DispConvWinToScreen(vt_src, Xw, Yw, &X, &Y, &Right);
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

	ClickCell = GetCharCell(X, Y, Right);

	SelectEnd = ClickCell;
	SelectEndOld = SelectEnd;
	CaretOff(vt_src);
	Selected = TRUE;
	BoxSelect = Box;
}

/**
 *  Change selection region by mouse move
 *	マウスが移動して選択領域が変化
 *
 *	@param Xw		horizontal position of the mouse cursor
 *					in window coordinate
 *	@param Yw		vertical
 *	@param NClick	1/2/3	ポインタ移動直前のクリック数
 */
void BuffChangeSelect(int Xw, int Yw, int NClick)
{
	int X, Y;
	BOOL Right;

	DispConvWinToScreen(vt_src, Xw, Yw, &X, &Y, &Right);

	if (!Selecting) {
		if (GetTickCount() - SelectStartTime < ts.SelectStartDelay) {
			return;
		}
		SelectStart = ClickCell;
		Selecting = TRUE;
	}

	Y = Y + PageStart;

	if (X<0) X = 0;
	if (X > NumOfColumns) {
		X = NumOfColumns;
	}
	if (Y < 0) Y = 0;
	if (Y >= BuffEnd) {
		Y = BuffEnd - 1;
	}

	LockBuffer();

	SelectEnd = GetCharCell(X, Y, Right);
	X = SelectEnd.x;
	Y = SelectEnd.y;

	if (NClick==2) { // drag after double click
		if ((DblClkStart.y > Y) ||
			(DblClkStart.y == Y && X < DblClkStart.x)) {
			// ダブルクリック選択領域より前を選択
			int dest_x;
			int dest_y;
			SearchDelimiterPrev(X, Y, ts.EnableContinuedLineCopy, &dest_x, &dest_y);
			SelectEnd.x = dest_x;
			SelectEnd.y = dest_y;
			SelectStart = DblClkEnd;
		} else if ((DblClkEnd.y < Y) ||
				   (DblClkEnd.y == Y && X > DblClkEnd.x)) {
			// ダブルクリック選択領域より後ろを選択
			int dest_x;
			int dest_y;
			SearchDelimiterNext(X - 1, Y, ts.EnableContinuedLineCopy, &dest_x, &dest_y);
			SelectEnd.x = dest_x + 1;
			SelectEnd.y = dest_y;
			SelectStart = DblClkStart;
		} else {
			// ダブルクリック選択領域上だった場合
			SelectStart = DblClkStart;
			SelectEnd = DblClkEnd;
		}
	}
	else if (NClick==3) { // drag after tripple click
		if ((SelectEnd.y > SelectStart.y) || ((SelectEnd.y == SelectStart.y) && (SelectEnd.x >= SelectStart.x))) {
			if (SelectStart.x == DblClkEnd.x) {
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

	ChangeSelectRegion();
	UnlockBuffer();
}

wchar_t *BuffEndSelect(void)
//  End text selection by mouse button up
{
	wchar_t *retval = NULL;
	if (!Selecting) {
		if (GetTickCount() - SelectStartTime < ts.SelectStartDelay) {
			SelectEnd = SelectStart;
			SelectEndOld = SelectEnd;
			LockBuffer();
			ChangeSelectRegion();
			UnlockBuffer();
			Selected = FALSE;
			return NULL;
		}
		SelectStart = ClickCell;
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
		else if ((SelectEnd.y < SelectStart.y) || ((SelectEnd.y == SelectStart.y) && (SelectEnd.x < SelectStart.x))) {
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
			retval = BuffCBCopyUnicode(FALSE);
			UnlockBuffer();
		}
	}
	return retval;
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
	DispChangeWinSize(vt_src, Nx, Ny);
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

	DispScrollHomePos(vt_src);

	if (cv.Ready && cv.TelFlag) {
		TelInformWinSize(NumOfColumns,NumOfLines-StatusLine);
	}

	TTXSetWinSize(NumOfLines-StatusLine, NumOfColumns); /* TTPLUG */
}

void ChangeWin(void)
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

	DispChangeWin(vt_src);
}

void ClearBuffer(void)
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
	memsetW(&CodeBuffW[0],0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, BufferSize);

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

	DispClearWin(vt_src);
}

void SetTabStop(void)
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
	DispScrollHomePos(vt_src);

	MoveCursor(CursorX,CursorY);
}

void BuffLineContinued(BOOL mode)
{
	buff_char_t* CodeLineW = &CodeBuffW[LinePtr];
	if (ts.EnableContinuedLineCopy) {
		if (mode) {
			CodeLineW[0].attr |= AttrLineContinued;
		} else {
			CodeLineW[0].attr &= ~AttrLineContinued;
		}
	}
}

void BuffSetCurCharAttr(const TCharAttr *Attr)
{
	CurCharAttr = *Attr;
}

void BuffSaveScreen(void)
{
	buff_char_t *CodeDestW;
	LONG ScrSize;
	LONG SrcPtr, DestPtr;
	int i;

	if (SaveBuff == NULL) {
		ScrSize = NumOfColumns * NumOfLines;	// 1画面分のバッファの保存数
		// 全画面分のバイト数
		SaveBuff = calloc(ScrSize, sizeof(buff_char_t));
		if (SaveBuff != NULL) {
			CodeDestW = (buff_char_t *)SaveBuff;

			SaveBuffX = NumOfColumns;
			SaveBuffY = NumOfLines;

			SrcPtr = GetLinePtr(PageStart);
			DestPtr = 0;

			for (i=0; i<NumOfLines; i++) {
				memcpyW(&CodeDestW[DestPtr], &CodeBuffW[SrcPtr], NumOfColumns);
				SrcPtr = NextLinePtr(SrcPtr);
				DestPtr += NumOfColumns;
			}
		}
	}
	_CrtCheckMemory();
}

void BuffRestoreScreen(void)
{
	buff_char_t *CodeSrcW;
	LONG SrcPtr, DestPtr;
	int i, CopyX, CopyY;

	_CrtCheckMemory();
	if (SaveBuff != NULL) {
		CodeSrcW = (buff_char_t*)SaveBuff;

		CopyX = (SaveBuffX > NumOfColumns) ? NumOfColumns : SaveBuffX;
		CopyY = (SaveBuffY > NumOfLines) ? NumOfLines : SaveBuffY;

		SrcPtr = 0;
		DestPtr = GetLinePtr(PageStart);

		for (i=0; i<CopyY; i++) {
			memcpyW(&CodeBuffW[DestPtr], &CodeSrcW[SrcPtr], CopyX);
			if (CodeBuffW[DestPtr+CopyX-1].attr & AttrKanji) {
				BuffSetChar(&CodeBuffW[DestPtr+CopyX-1], 0x20, 'H');
				CodeBuffW[DestPtr+CopyX-1].attr ^= AttrKanji;
			}
			SrcPtr += SaveBuffX;
			DestPtr = NextLinePtr(DestPtr);
		}
		BuffUpdateRect(WinOrgX,WinOrgY,WinOrgX+WinWidth-1,WinOrgY+WinHeight-1);

		BuffDiscardSavedScreen();
	}
	_CrtCheckMemory();
}

void BuffDiscardSavedScreen(void)
{
	if (SaveBuff != NULL) {
		int i;
		buff_char_t *b = (buff_char_t *)SaveBuff;

		for (i = 0; i < SaveBuffX * SaveBuffY; i++) {
			FreeCombinationBuf(&b[i]);
		}

		free(SaveBuff);
		SaveBuff = NULL;
	}
	_CrtCheckMemory();
}

void BuffSelectedEraseCurToEnd(void)
// Erase characters from cursor to the end of screen
{
	buff_char_t* CodeLineW = &CodeBuffW[LinePtr];
	LONG TmpPtr;
	int offset;
	int i, j, YEnd;

	NewLine(PageStart+CursorY);

	if (ts.KanjiCode == IdSJIS ||
		ts.KanjiCode == IdEUC ||
		ts.KanjiCode == IdJIS ||
		ts.KanjiCode == IdKoreanCP949 ||
		ts.KanjiCode == IdUTF8) {
		if (!(CodeLineW[CursorX].attr2 & Attr2Protect)) {
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
			if (!(CodeLineW[j].attr2 & Attr2Protect)) {
				BuffSetChar(&CodeBuffW[j], 0x20, 'H');
				CodeLineW[j].attr &= AttrSgrMask;
			}
		}
		offset = 0;
		TmpPtr = NextLinePtr(TmpPtr);
	}
	/* update window */
	BuffUpdateRect(0, CursorY, NumOfColumns, YEnd);
}

void BuffSelectedEraseHomeToCur(void)
// Erase characters from home to cursor
{
	buff_char_t* CodeLineW = &CodeBuffW[LinePtr];
	LONG TmpPtr;
	int offset;
	int i, j, YHome;

	NewLine(PageStart+CursorY);
	if (ts.KanjiCode == IdSJIS ||
		ts.KanjiCode == IdEUC ||
		ts.KanjiCode == IdJIS ||
		ts.KanjiCode == IdKoreanCP949 ||
		ts.KanjiCode == IdUTF8) {
		if (!(CodeLineW[CursorX].attr2 & Attr2Protect)) {
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
			if (!(CodeLineW[j].attr2 & Attr2Protect)) {
				BuffSetChar(&CodeBuffW[j], 0x20, 'H');
				CodeBuffW[j].attr &= AttrSgrMask;
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

void BuffSelectiveEraseBox(int XStart, int YStart, int XEnd, int YEnd)
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
		    ((CodeBuffW[Ptr+XStart-1].attr & AttrKanji) != 0) &&
		    ((CodeBuffW[Ptr+XStart-1].attr2 & Attr2Protect) == 0)) {
			BuffSetChar(&CodeBuffW[Ptr+XStart-1], 0x20, 'H');
			CodeBuffW[Ptr+XStart-1].attr &= AttrSgrMask;
		}
		if ((XStart+C<NumOfColumns) &&
		    ((CodeBuffW[Ptr+XStart+C-1].attr & AttrKanji) != 0) &&
		    ((CodeBuffW[Ptr+XStart+C-1].attr2 & Attr2Protect) == 0)) {
			BuffSetChar(&CodeBuffW[Ptr+XStart+C], 0x20, 'H');
			CodeBuffW[Ptr+XStart+C].attr &= AttrSgrMask;
		}
		for (j=Ptr+XStart; j<Ptr+XStart+C; j++) {
			if (!(CodeBuffW[j].attr2 & Attr2Protect)) {
				BuffSetChar(&CodeBuffW[j], 0x20, 'H');
				CodeBuffW[j].attr &= AttrSgrMask;
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
	buff_char_t * CodeLineW = &CodeBuffW[LinePtr];
	int i;
	BOOL LineContinued=FALSE;

	if (ts.EnableContinuedLineCopy && XStart == 0 && (CodeLineW[0].attr & AttrLineContinued)) {
		LineContinued = TRUE;
	}

	if (ts.KanjiCode == IdSJIS ||
		ts.KanjiCode == IdEUC ||
		ts.KanjiCode == IdJIS ||
		ts.KanjiCode == IdKoreanCP949 ||
		ts.KanjiCode == IdUTF8) {
		if (!(CodeLineW[CursorX].attr2 & Attr2Protect)) {
			EraseKanji(1); /* if cursor is on right half of a kanji, erase the kanji */
		}
	}

	NewLine(PageStart+CursorY);
	for (i=XStart; i < XStart + Count; i++) {
		if (!(CodeLineW[i].attr2 & Attr2Protect)) {
			BuffSetChar(&CodeLineW[i], 0x20, 'H');
			CodeLineW[i].attr &= AttrSgrMask;
		}
	}

	if (ts.EnableContinuedLineCopy) {
		if (LineContinued) {
			BuffLineContinued(TRUE);
		}

		if (XStart + Count >= NumOfColumns) {
			CodeBuffW[NextLinePtr(LinePtr)].attr &= ~AttrLineContinued;
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

		if (CodeBuffW[LPtr+CursorRightM].attr & AttrKanji) {
			BuffSetChar(&CodeBuffW[LPtr+CursorRightM], 0x20, 'H');
			CodeBuffW[LPtr+CursorRightM].attr &= ~AttrKanji;
			if (CursorRightM < NumOfColumns-1) {
				BuffSetChar(&CodeBuffW[LPtr+CursorRightM+1], 0x20, 'H');
			}
		}

		if (CodeBuffW[Ptr+count-1].attr & AttrKanji) {
			BuffSetChar(&CodeBuffW[Ptr+count], 0x20, 'H');
		}

		if (CursorLeftM > 0 && CodeBuffW[Ptr-1].attr & AttrKanji) {
			BuffSetChar(&CodeBuffW[Ptr-1], 0x20, 'H');
			CodeBuffW[Ptr-1].attr &= ~AttrKanji;
		}

		memmoveW(&(CodeBuffW[Ptr]),   &(CodeBuffW[Ptr+count]),   MoveLen);

		memsetW(&(CodeBuffW[Ptr+MoveLen]), 0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, count);

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

		if (CursorRightM < NumOfColumns-1 && CodeBuffW[LPtr+CursorRightM].attr & AttrKanji) {
			BuffSetChar(&CodeBuffW[LPtr+CursorRightM+1], 0x20, 'H');
		}

		if (CursorLeftM > 0 && CodeBuffW[Ptr-1].attr & AttrKanji) {
			BuffSetChar(&CodeBuffW[Ptr-1], 0x20, 'H');
			CodeBuffW[Ptr-1].attr &= ~AttrKanji;
			BuffSetChar(&CodeBuffW[Ptr], 0x20, 'H');
		}

		memmoveW(&(CodeBuffW[Ptr+count]),   &(CodeBuffW[Ptr]),   MoveLen);

		memsetW(&(CodeBuffW[Ptr]),   0x20, CurCharAttr.Fore, CurCharAttr.Back, AttrDefault, CurCharAttr.Attr2 & Attr2ColorMask, count);

		if (CodeBuffW[LPtr+CursorRightM].attr & AttrKanji) {
			BuffSetChar(&CodeBuffW[LPtr+CursorRightM], 0x20, 'H');
			CodeBuffW[LPtr+CursorRightM].attr &= ~AttrKanji;
		}

		LPtr = NextLinePtr(LPtr);
	}

	BuffUpdateRect(CursorLeftM-(CursorLeftM>0), CursorTop, CursorRightM+(CursorRightM<NumOfColumns-1), CursorBottom);
}

// 現在行をまるごとバッファに格納する。返り値は現在のカーソル位置(X)。
int BuffGetCurrentLineData(char *buf, int bufsize)
{
#if 0
	LONG Ptr;

	Ptr = GetLinePtr(PageStart + CursorY);
	memset(buf, 0, bufsize);
	memcpy(buf, &CodeBuff[Ptr], min(NumOfColumns, bufsize - 1));
	return (CursorX);
#endif
	BuffGetAnyLineData(CursorY, buf, bufsize);
	return CursorX;
}

/**
 * Sy行の一行を文字列にして返す
 *
 * @param[in]	Sy			Sy行の1行を返す
 * @param[in]	*Cx			文字の位置(カーソルなどの位置), NULLのとき無効
 * @param[out]	*Cx			左端からの文字数(先頭からカーソル位置までの文字数)
 * @param[out]	*lenght		文字数(ターミネータ含む)
 */
wchar_t *BuffGetLineStrW(int Sy, int *cx, size_t *lenght)
{
	size_t total_len = 0;
	LONG Ptr = GetLinePtr(PageStart + Sy);
	buff_char_t* b = &CodeBuffW[Ptr];
	int x;
	int cx_pos = cx != NULL ? *cx : 0;
	size_t idx;
	wchar_t *result;
	for(x = 0; x < NumOfColumns; x++) {
		size_t len;
		if (x == cx_pos) {
			if (cx != NULL) {
				*cx = (int)total_len;
			}
		}
		len = expand_wchar(b + x, NULL, 0, NULL);
		total_len += len;
	}
	total_len++;
	result = (wchar_t *)malloc(total_len * sizeof(wchar_t));
	idx = 0;
	for(x = 0; x < NumOfColumns; x++) {
		wchar_t *p = &result[idx];
		size_t len = expand_wchar(b + x, p, total_len - idx, NULL);
		idx += len;
	}
	result[idx] = 0;
	if (lenght != NULL) {
		*lenght = total_len;
	}
	return result;
}

// 全バッファから指定した行を返す。
int BuffGetAnyLineData(int offset_y, char *buf, int bufsize)
{
	LONG Ptr;
	int copysize = 0;
	int i;
	int idx;

	if (offset_y >= BuffEnd)
		return -1;

	Ptr = GetLinePtr(offset_y);
	memset(buf, 0, bufsize);
	copysize = min(NumOfColumns, bufsize - 1);
	idx = 0;
	for (i = 0; i<copysize; i++) {
		unsigned short c;
		buff_char_t *b = &CodeBuffW[Ptr];
		if (IsBuffPadding(b)) {
			continue;
		}
		c = b->ansi_char;
		buf[idx++] = c & 0xff;
		if (c >= 0x100) {
			buf[idx++] = (c >> 8) & 0xff;
		}
		if (idx >= bufsize - 1) {
			break;
		}
	}

	return (copysize);
}

/**
 * 全バッファから指定した行を返す。
 * filesys_log.cpp で使用される
 *
 * @param[in]	offset_y	取得する行(0...)
 * @param[in]	bufsize		文字数
 * @return		文字数
 *				-1	最終行以降を指定
 */
int BuffGetAnyLineDataW(int offset_y, wchar_t *buf, size_t bufsize)
{
	LONG Ptr;
	size_t copysize;
	size_t i;
	size_t idx;
	size_t left;
	buff_char_t *b;

	if (offset_y >= BuffEnd)
		return -1;

	memset(buf, 0, bufsize * sizeof(wchar_t));
	copysize = min((size_t)NumOfColumns, bufsize - 1);
	Ptr = GetLinePtr(offset_y);
	b = &CodeBuffW[Ptr];
	idx = 0;
	left = copysize;
	for (i = 0; i<copysize; i++) {
		BOOL too_small;
		size_t len;
		if (IsBuffPadding(b)) {
			continue;
		}
		len = expand_wchar(b, &buf[idx], left, &too_small);
		if (too_small) {
			break;
		}
		idx += len;
		left -= len;
		b++;
		if (idx >= bufsize - 1) {
			break;
		}
	}

	return (int)idx;
}

BOOL BuffCheckMouseOnURL(int Xw, int Yw)
{
	int X, Y;
	LONG TmpPtr;
	BOOL Result, Right;

	DispConvWinToScreen(vt_src, Xw, Yw, &X, &Y, &Right);
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

	if (CodeBuffW[TmpPtr+X].attr & AttrURL)
		Result = TRUE;
	else
		Result = FALSE;

	UnlockBuffer();

	return Result;
}

static wchar_t *UnicodeCodePointStr(char32_t u32)
{
	const wchar_t *format =
		u32 < 0x10000 ? L"U+%04X" :
		u32 < 0x100000 ? L"U+%05X" :  L"U+%06X";
	wchar_t *str;
	aswprintf(&str, format, u32);
	return str;
}

/**
 *	指定位置の文字情報を文字列で返す
 *	デバグ用途
 *
 *	@param		Xw, Yw	ウィンドウ上のX,Y(pixel),マウスポインタの位置
 *	@return		文字列(不要になったらfree()すること)
 */
wchar_t *BuffGetCharInfo(int Xw, int Yw)
{
	int X, Y;
	int ScreenY;
	LONG TmpPtr;
	BOOL Right;
	wchar_t *str_ptr;
	const buff_char_t *b;
	wchar_t *pos_str;
	wchar_t *attr_str;
	wchar_t *ansi_str;
    wchar_t *unicode_char_str;
    wchar_t *unicode_utf16_str;
    wchar_t *unicode_utf32_str;

	DispConvWinToScreen(vt_src, Xw, Yw, &X, &ScreenY, &Right);
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
	b = &CodeBuffW[TmpPtr+X];

	aswprintf(&pos_str,
			  L"ch(%d,%d(%d)) px(%d,%d)\n",
			  X, ScreenY, Y,
			  Xw, Yw);

	// アトリビュート
	{
		wchar_t *attr1_attr_str;
		wchar_t *attr1_str;
		wchar_t *attr2_str;
		wchar_t *attr2_attr_str;
		wchar_t *width_property;

		if (b->attr == 0) {
			attr1_attr_str = _wcsdup(L"");
		} else {
			const unsigned char attr = b->attr;
			aswprintf(&attr1_attr_str,
					  L"\n (%S%S%S%S%S%S%S%S)",
					  (attr & AttrBold) != 0 ? "AttrBold " : "",
					  (attr & AttrUnder) != 0 ? "AttrUnder " : "",
					  (attr & AttrSpecial) != 0 ? "AttrSpecial ": "",
					  (attr & AttrBlink) != 0 ? "AttrBlink ": "",
					  (attr & AttrReverse) != 0 ? "AttrReverse ": "",
					  (attr & AttrLineContinued) != 0 ? "AttrLineContinued ": "",
					  (attr & AttrURL) != 0 ? "AttrURL ": "",
					  (attr & AttrKanji) != 0 ? "AttrKanji ": "");
		}

		if (b->attr2 == 0) {
			attr2_attr_str = _wcsdup(L"");
		} else {
			const unsigned char attr2 = b->attr2;
			aswprintf(&attr2_attr_str,
					  L"\n (%S%S%S)",
					  (attr2 & Attr2Fore) != 0 ? "Attr2Fore " : "",
					  (attr2 & Attr2Back) != 0 ? "Attr2Back " : "",
					  (attr2 & Attr2Protect) != 0 ? "Attr2Protect ": "");
		}

		aswprintf(&attr1_str,
				  L"attr      0x%02x%s\n"
				  L"attr2     0x%02x%s\n",
				  b->attr, attr1_attr_str,
				  b->attr2, attr2_attr_str);
		free(attr1_attr_str);
		free(attr2_attr_str);

		width_property =
			b->WidthProperty == 'F' ? L"Fullwidth" :
			b->WidthProperty == 'H' ? L"Halfwidth" :
			b->WidthProperty == 'W' ? L"Wide" :
			b->WidthProperty == 'n' ? L"Narrow" :
			b->WidthProperty == 'A' ? L"Ambiguous" :
			b->WidthProperty == 'N' ? L"Neutral" :
			L"?";

		aswprintf(&attr2_str,
				  L"attrFore  0x%02x\n"
				  L"attrBack  0x%02x\n"
				  L"WidthProperty %s(%hc)\n"
				  L"cell %hd\n"
				  L"Padding %s\n"
				  L"Emoji %s\n",
				  b->fg, b->bg,
				  width_property, b->WidthProperty,
				  b->cell,
				  (b->Padding ? L"TRUE" : L"FALSE"),
				  (b->Emoji ? L"TRUE" : L"FALSE")
			);

		attr_str = NULL;
		awcscat(&attr_str, attr1_str);
		awcscat(&attr_str, attr2_str);
		free(attr1_str);
		free(attr2_str);
	}

	// ANSI
	{
		unsigned char mb[4];
		unsigned short c = b->ansi_char;
		if (c == 0) {
			mb[0] = 0;
		}
		else if (c < 0x100) {
			mb[0] = c < 0x20 ? '.' : (c & 0xff);
			mb[1] = 0;
		}
		else {
			mb[0] = ((c >> 8) & 0xff);
			mb[1] = (c & 0xff);
			mb[2] = 0;
		}

		aswprintf(&ansi_str,
				  L"ansi:\n"
				  L" '%hs'\n"
				  L" 0x%04x\n", mb, c);
	}

	// Unicode 文字
	{
		wchar_t *wcs = GetWCS(b);
		aswprintf(&unicode_char_str,
				  L"Unicode char:\n"
				  L" '%s'\n", wcs);
		free(wcs);
	}

	// Unicode UTF-16 文字コード
	{
		wchar_t *codes_ptr = NULL;
		wchar_t *code_str;
		int i;

		aswprintf(&code_str,
				  L"Unicode UTF-16:\n"
				  L" 0x%04x\n",
				  b->wc2[0]);
		awcscat(&codes_ptr, code_str);
		free(code_str);
		if (b->wc2[1] != 0 ) {
			wchar_t buf[32];
			swprintf(buf, _countof(buf), L" 0x%04x\n", b->wc2[1]);
			awcscat(&codes_ptr, buf);
		}
		for (i=0; i<b->CombinationCharCount16; i++) {
			wchar_t buf[32];
			swprintf(buf, _countof(buf), L" 0x%04x\n", b->pCombinationChars16[i]);
			awcscat(&codes_ptr, buf);
		}
		unicode_utf16_str = codes_ptr;
	}

	// Unicode UTF-32 文字コード
	{
		wchar_t *codes_ptr = NULL;
		wchar_t *code_str;
		int i;

		awcscat(&codes_ptr, L"Unicode UTF-32:\n");
		code_str = UnicodeCodePointStr(b->u32);
		awcscats(&codes_ptr, L" ", code_str, L"\n", NULL);
		free(code_str);
		for (i=0; i<b->CombinationCharCount32; i++) {
			code_str = UnicodeCodePointStr(b->pCombinationChars32[i]);
			awcscats(&codes_ptr, L" ", code_str, L"\n", NULL);
			free(code_str);
		}
		unicode_utf32_str = codes_ptr;
	}

	str_ptr = NULL;
	awcscat(&str_ptr, pos_str);
	awcscat(&str_ptr, attr_str);
	awcscat(&str_ptr, ansi_str);
	awcscat(&str_ptr, unicode_char_str);
	awcscat(&str_ptr, unicode_utf16_str);
	awcscat(&str_ptr, unicode_utf32_str);
	free(pos_str);
	free(attr_str);
	free(ansi_str);
	free(unicode_char_str);
	free(unicode_utf16_str);
	free(unicode_utf32_str);
	awcscat(&str_ptr, L"\nPress shift for sending to clipboard");
	return str_ptr;
}

void BuffSetCursorCharAttr(int x, int y, const TCharAttr *Attr)
{
	const LONG TmpPtr = GetLinePtr(PageStart+y);
	CodeBuffW[TmpPtr + x].attr = Attr->Attr;
	CodeBuffW[TmpPtr + x].attr2 = Attr->Attr2;
	CodeBuffW[TmpPtr + x].fg = Attr->Fore;
	CodeBuffW[TmpPtr + x].bg = Attr->Back;
}

TCharAttr BuffGetCursorCharAttr(int x, int y)
{
	const LONG TmpPtr = GetLinePtr(PageStart+y);
	TCharAttr Attr;
	Attr.Attr = CodeBuffW[TmpPtr + x].attr;
	Attr.Attr2 = CodeBuffW[TmpPtr + x].attr2;
	Attr.Fore = CodeBuffW[TmpPtr + x].fg;
	Attr.Back = CodeBuffW[TmpPtr + x].bg;

	return Attr;
}

void BuffSetDispAPI(BOOL unicode)
{
	UseUnicodeApi = unicode;
}

void BuffSetDispCodePage(int code_page)
{
	CodePage = code_page;
}

int BuffGetDispCodePage(void)
{
	return CodePage;
}

BOOL BuffIsSelected(void)
{
	return Selected;
}
