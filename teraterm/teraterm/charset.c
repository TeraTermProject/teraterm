/*
 * (C) 2023- TeraTerm Project
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

#include "teraterm.h"
#include "tttypes.h"
#include <stdio.h>
#include <string.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "buffer.h"	// for Wrap
#include "ttwinman.h"
#include "codeconv.h"
#include "unicode.h"
#include "language.h"	// for JIS2SJIS()
#include "ttcstd.h"

#include "charset.h"

// UTF-8���s���Ȓl���������ɕ\�����镶��
#define REPLACEMENT_CHARACTER	'?'
//#define REPLACEMENT_CHARACTER	0x2592
//#define REPLACEMENT_CHARACTER	0x20
//#define REPLACEMENT_CHARACTER	0xfffd

static BOOL KanjiIn;				// TRUE = MBCS��1byte�ڂ���M���Ă���
static BOOL EUCkanaIn, EUCsupIn;
static int  EUCcount;
#if 0
static BOOL Special;
#endif

/* GL for single shift 2/3 */
static int GLtmp;
/* single shift 2/3 flag */
static BOOL SSflag;
/* JIS -> SJIS conversion flag */
static BOOL ConvJIS;
static WORD Kanji;
static BOOL Fallbacked;

typedef struct {
	/* GL, GR code group */
	int Glr[2];
	/* G0, G1, G2, G3 code group */
	int Gn[4];
	//
	char32_t replacement_char;
	// UTF-8 work
	BYTE buf[4];
	int count;
} VttermKanjiWork;

static VttermKanjiWork KanjiWork;

// Unicode�x�[�X�ɐ؂�ւ�
static void PutChar(BYTE b)
{
	PutU32(b);
}

/**
 *	ISO2022�p���[�N������������
 */
static void CharSetInit2(VttermKanjiWork *w)
{
	if (ts.Language==IdJapanese) {
		w->Gn[0] = IdASCII;
		w->Gn[1] = IdKatakana;
		w->Gn[2] = IdKatakana;
		w->Gn[3] = IdKanji;
		w->Glr[0] = 0;
		if ((ts.KanjiCode==IdJIS) && (ts.JIS7Katakana==0))
			w->Glr[1] = 2;	// 8-bit katakana
		else
			w->Glr[1] = 3;
	}
	else {
		w->Gn[0] = IdASCII;
		w->Gn[1] = IdSpecial;
		w->Gn[2] = IdASCII;
		w->Gn[3] = IdASCII;
		w->Glr[0] = 0;
		w->Glr[1] = 0;
	}
}

/**
 *	�����֘A���[�N������������
 */
void CharSetInit(void)
{
	VttermKanjiWork *w = &KanjiWork;

	CharSetInit2(w);

	w->replacement_char = REPLACEMENT_CHARACTER;
	SSflag = FALSE;

	KanjiIn = FALSE;
	EUCkanaIn = FALSE;
	EUCsupIn = FALSE;
	ConvJIS = FALSE;
	Fallbacked = FALSE;
}

/**
 *	1byte�ڃ`�F�b�N
 */
static BOOL CheckFirstByte(BYTE b, int lang, int kanji_code)
{
	switch (lang) {
		case IdKorean:
			return __ismbblead(b, 51949);
		case IdChinese:
			if (kanji_code == IdCnGB2312) {
				return __ismbblead(b, 936);
			}
			else if (ts.KanjiCode == IdCnBig5) {
				return __ismbblead(b, 950);
			}
			break;
		default:
			assert(FALSE);
			break;
	}
	assert(FALSE);
	return FALSE;
}

/**
 *	Double-byte Character Sets
 *	SJIS��1byte��?
 *
 *	��1�o�C�g0x81...0x9F or 0xE0...0xEF
 *	��1�o�C�g0x81...0x9F or 0xE0...0xFC
 */
static BOOL ismbbleadSJIS(BYTE b)
{
	if (((0x80<b) && (b<0xa0)) || ((0xdf<b) && (b<0xfd))) {
		return TRUE;
	}
	return FALSE;
}

/**
 *	ts.Language == IdJapanese ��
 *	1byte�ڃ`�F�b�N
 */
static BOOL CheckKanji(BYTE b)
{
	VttermKanjiWork *w = &KanjiWork;
	BOOL Check;

	if (ts.Language!=IdJapanese)
		return FALSE;

	ConvJIS = FALSE;

	if (ts.KanjiCode==IdSJIS ||
	   (ts.FallbackToCP932 && ts.KanjiCode==IdUTF8)) {
		if (((0x80<b) && (b<0xa0)) || ((0xdf<b) && (b<0xfd))) {
			Fallbacked = TRUE;
			return TRUE; // SJIS kanji
		}
		if ((0xa1<=b) && (b<=0xdf)) {
			return FALSE; // SJIS katakana
		}
	}

	if ((b>=0x21) && (b<=0x7e)) {
		Check = (w->Gn[w->Glr[0]] == IdKanji);
		ConvJIS = Check;
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		Check = (w->Gn[w->Glr[1]] == IdKanji);
		if (ts.KanjiCode==IdEUC) {
			Check = TRUE;
		}
		else if (ts.KanjiCode==IdJIS && ((ts.TermFlag & TF_FIXEDJIS)!=0) && (ts.JIS7Katakana==0)) {
			Check = FALSE; // 8-bit katakana
		}
		ConvJIS = Check;
	}
	else {
		Check = FALSE;
	}

	return Check;
}

static BOOL ParseFirstJP(BYTE b)
// returns TRUE if b is processed
//  (actually allways returns TRUE)
{
	VttermKanjiWork *w = &KanjiWork;
	if (KanjiIn) {
		if (((! ConvJIS) && (0x3F<b) && (b<0xFD)) ||
			(ConvJIS && ( ((0x20<b) && (b<0x7f)) ||
						  ((0xa0<b) && (b<0xff)) )) )
		{
			unsigned long u32;
			Kanji = Kanji + b;
			if (ConvJIS) {
				// JIS -> Shift_JIS(CP932)
				Kanji = JIS2SJIS((WORD)(Kanji & 0x7f7f));
			}
			u32 = CP932ToUTF32(Kanji);
			PutU32(u32);
			KanjiIn = FALSE;
			return TRUE;
		}
		else if ((ts.TermFlag & TF_CTRLINKANJI)==0) {
			KanjiIn = FALSE;
		}
		else if ((b==CR) && Wrap) {
			CarriageReturn(FALSE);
			LineFeed(LF,FALSE);
			Wrap = FALSE;
		}
	}

	if (SSflag) {
		if (w->Gn[GLtmp] == IdKanji) {
			Kanji = b << 8;
			KanjiIn = TRUE;
			SSflag = FALSE;
			return TRUE;
		}
		else if (w->Gn[GLtmp] == IdKatakana) {
			b = b | 0x80;
		}

		PutChar(b);
		SSflag = FALSE;
		return TRUE;
	}

	if ((!EUCsupIn) && (!EUCkanaIn) && (!KanjiIn) && CheckKanji(b)) {
		Kanji = b << 8;
		KanjiIn = TRUE;
		return TRUE;
	}

	if (b<=US) {
		ParseControl(b);
	}
	else if (b==0x20) {
		PutChar(b);
	}
	else if ((b>=0x21) && (b<=0x7E)) {
		if (EUCsupIn) {
			EUCcount--;
			EUCsupIn = (EUCcount==0);
			return TRUE;
		}

		if ((w->Gn[w->Glr[0]] == IdKatakana) || EUCkanaIn) {
			b = b | 0x80;
			EUCkanaIn = FALSE;
			{
				// b��sjis�̔��p�J�^�J�i
				unsigned long u32 = CP932ToUTF32(b);
				PutU32(u32);
			}
			return TRUE;
		}
		PutChar(b);
	}
	else if (b==0x7f) {
		return TRUE;
	}
	else if ((b>=0x80) && (b<=0x8D)) {
		ParseControl(b);
	}
	else if (b==0x8E) { // SS2
		switch (ts.KanjiCode) {
		case IdEUC:
			if (ts.ISO2022Flag & ISO2022_SS2) {
				EUCkanaIn = TRUE;
			}
			break;
		case IdUTF8:
			PutU32(REPLACEMENT_CHARACTER);
			break;
		default:
			ParseControl(b);
		}
	}
	else if (b==0x8F) { // SS3
		switch (ts.KanjiCode) {
		case IdEUC:
			if (ts.ISO2022Flag & ISO2022_SS3) {
				EUCcount = 2;
				EUCsupIn = TRUE;
			}
			break;
		case IdUTF8:
			PutU32(REPLACEMENT_CHARACTER);
			break;
		default:
			ParseControl(b);
		}
	}
	else if ((b>=0x90) && (b<=0x9F)) {
		ParseControl(b);
	}
	else if (b==0xA0) {
		PutChar(0x20);
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		if (EUCsupIn) {
			EUCcount--;
			EUCsupIn = (EUCcount==0);
			return TRUE;
		}

		if ((w->Gn[w->Glr[1]] != IdASCII) ||
		    ((ts.KanjiCode==IdEUC) && EUCkanaIn) ||
		    (ts.KanjiCode==IdSJIS) ||
		    ((ts.KanjiCode==IdJIS) &&
			 (ts.JIS7Katakana==0) &&
			 ((ts.TermFlag & TF_FIXEDJIS)!=0))) {
			// b��sjis�̔��p�J�^�J�i
			unsigned long u32 = CP932ToUTF32(b);
			PutU32(u32);
		} else {
			if (w->Gn[w->Glr[1]] == IdASCII) {
				b = b & 0x7f;
			}
			PutChar(b);
		}
		EUCkanaIn = FALSE;
	}
	else {
		PutChar(b);
	}

	return TRUE;
}

static BOOL ParseFirstKR(BYTE b)
// returns TRUE if b is processed
//  (actually allways returns TRUE)
{
	VttermKanjiWork *w = &KanjiWork;
	if (KanjiIn) {
		if (((0x41<=b) && (b<=0x5A)) ||
			((0x61<=b) && (b<=0x7A)) ||
			((0x81<=b) && (b<=0xFE)))
		{
			unsigned long u32 = 0;
			if (ts.KanjiCode == IdKoreanCP949) {
				// CP51949
				Kanji = Kanji + b;
				u32 = MBCP_UTF32(Kanji, 51949);
			}
			else {
				assert(FALSE);
			}
			PutU32(u32);
			KanjiIn = FALSE;
			return TRUE;
		}
		else if ((ts.TermFlag & TF_CTRLINKANJI)==0) {
			KanjiIn = FALSE;
		}
		else if ((b==CR) && Wrap) {
			CarriageReturn(FALSE);
			LineFeed(LF,FALSE);
			Wrap = FALSE;
		}
	}

	if ((!KanjiIn) && CheckFirstByte(b, ts.Language, ts.KanjiCode)) {
		Kanji = b << 8;
		KanjiIn = TRUE;
		return TRUE;
	}

	if (b<=US) {
		ParseControl(b);
	}
	else if (b==0x20) {
		PutChar(b);
	}
	else if ((b>=0x21) && (b<=0x7E)) {
//		if (Gn[Glr[0]] == IdKatakana) {
//			b = b | 0x80;
//		}
		PutChar(b);
	}
	else if (b==0x7f) {
		return TRUE;
	}
	else if ((0x80<=b) && (b<=0x9F)) {
		ParseControl(b);
	}
	else if (b==0xA0) {
		PutChar(0x20);
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		if (w->Gn[w->Glr[1]] == IdASCII) {
			b = b & 0x7f;
		}
		PutChar(b);
	}
	else {
		PutChar(b);
	}

	return TRUE;
}

static BOOL ParseFirstCn(BYTE b)
// returns TRUE if b is processed
//  (actually allways returns TRUE)
{
	VttermKanjiWork *w = &KanjiWork;
	if (KanjiIn) {
		// TODO
		if (((0x40<=b) && (b<=0x7e)) ||
		    ((0xa1<=b) && (b<=0xFE)))
		{
			unsigned long u32 = 0;
			Kanji = Kanji + b;
			if (ts.KanjiCode == IdCnGB2312) {
				// CP936 GB2312
				u32 = MBCP_UTF32(Kanji, 936);
			}
			else if (ts.KanjiCode == IdCnBig5) {
				// CP950 Big5
				u32 = MBCP_UTF32(Kanji, 950);
			}
			else {
				assert(FALSE);
			}
			PutU32(u32);
			KanjiIn = FALSE;
			return TRUE;
		}
		else if ((ts.TermFlag & TF_CTRLINKANJI)==0) {
			KanjiIn = FALSE;
		}
		else if ((b==CR) && Wrap) {
			CarriageReturn(FALSE);
			LineFeed(LF,FALSE);
			Wrap = FALSE;
		}
	}

	if ((!KanjiIn) && CheckFirstByte(b, ts.Language, ts.KanjiCode)) {
		Kanji = b << 8;
		KanjiIn = TRUE;
		return TRUE;
	}

	if (b<=US) {
		ParseControl(b);
	}
	else if (b==0x20) {
		PutChar(b);
	}
	else if ((b>=0x21) && (b<=0x7E)) {
//		if (Gn[Glr[0]] == IdKatakana) {
//			b = b | 0x80;
//		}
		PutChar(b);
	}
	else if (b==0x7f) {
		return TRUE;
	}
	else if ((0x80<=b) && (b<=0x9F)) {
		ParseControl(b);
	}
	else if (b==0xA0) {
		PutChar(0x20);
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		if (w->Gn[w->Glr[1]] == IdASCII) {
			b = b & 0x7f;
		}
		PutChar(b);
	}
	else {
		PutChar(b);
	}

	return TRUE;
}

static void ParseASCII(BYTE b)
{
	if (SSflag) {
		PutChar(b);
		SSflag = FALSE;
		return;
	}

	if (b<=US) {
		ParseControl(b);
	} else if ((b>=0x20) && (b<=0x7E)) {
		PutU32(b);
	} else if ((b==0x8E) || (b==0x8F)) {
		PutU32(REPLACEMENT_CHARACTER);
	} else if ((b>=0x80) && (b<=0x9F)) {
		ParseControl(b);
	} else if (b>=0xA0) {
		PutU32(b);
	}
}

static void PutReplacementChr(VttermKanjiWork *w, const BYTE *ptr, size_t len, BOOL fallback)
{
	const char32_t replacement_char = w->replacement_char;
	int i;
	for (i = 0; i < len; i++) {
		BYTE c = *ptr++;
		if (fallback) {
			// fallback ISO8859-1
			PutU32(c);
		}
		else {
			// fallback���Ȃ�
			if (c < 0x80) {
				// �s����UTF-8������̂Ȃ���0x80����������΁A
				// 1������UTF-8�����Ƃ��Ă��̂܂ܕ\������
				ParseASCII(c);
			}
			else {
				PutU32(replacement_char);
			}
		}
	}
}

// UTF-8�Ŏ�M�f�[�^����������
// returns TRUE if b is processed
//  (actually allways returns TRUE)
static BOOL ParseFirstUTF8(BYTE b)
{
	VttermKanjiWork *w = &KanjiWork;
	char32_t code;

	if (Fallbacked) {
		BOOL r = ParseFirstJP(b);
		Fallbacked = FALSE;
		return r;
	}

	// UTF-8�G���R�[�h
	//	The Unicode Standard Chapter 3
	//	Table 3-7. Well-Formed UTF-8 Byte Sequences
	// | Code Points        | First Byte | Second Byte | Third Byte | Fourth Byte |
	// | U+0000..U+007F     | 00..7F     |             |            |             |
	// | U+0080..U+07FF     | C2..DF     | 80..BF      |            |             |
	// | U+0800..U+0FFF     | E0         | A0..BF      | 80..BF     |             |
	// | U+1000..U+CFFF     | E1..EC     | 80..BF      | 80..BF     |             |
	// | U+D000..U+D7FF     | ED         | 80..9F      | 80..BF     |             |
	// | U+E000..U+FFFF     | EE..EF     | 80..BF      | 80..BF     |             |
	// | U+10000..U+3FFFF   | F0         | 90..BF      | 80..BF     | 80..BF      |
	// | U+40000..U+FFFFF   | F1..F3     | 80..BF      | 80..BF     | 80..BF      |
	// | U+100000..U+10FFFF | F4         | 80..8F      | 80..BF     | 80..BF      |
	// UTF-8�Ńf�R�[�h�ł��Ȃ��ꍇ
	//	- 1byte��
	//		- 0x00 - 0x7f		ok
	//		- 0x80 - 0xc1		ng
	//		- 0xc2 - 0xf4		ok
	//		- 0xf5 - 0xff		ng
	//	- 2byte�ڈȍ~
	//		- 0x00 - 0x7f		ng
	//		- 0x80 - 0xbf		ok
	//		- 0xc0 - 0xff		ng
	//  - 2byte�ڗ�O
	//		- 1byte == 0xe0 �̂Ƃ� 0xa0 - 0xbf�̂�ok
	//		- 1byte == 0xed �̂Ƃ� 0x80 - 0x9f�̂�ok
	//		- 1byte == 0xf0 �̂Ƃ� 0x90 - 0xbf�̂�ok
	//		- 1byte == 0xf4 �̂Ƃ� 0x90 - 0x8f�̂�ok
recheck:
	// 1byte(7bit)
	if (w->count == 0) {
		if (b <= 0x7f) {
			// 1byte(7bit)
			//		0x7f�ȉ�, �̂Ƃ��A���̂܂܏o��
			ParseASCII(b);
			return TRUE;
		}
		if (0xc2 <= b && b <= 0xf4) {
			// 1byte�ڕۑ�
			w->buf[w->count++] = b;
			return TRUE;
		}

		// UTF-8��1byte�ɏo�����Ȃ��R�[�h�̂Ƃ�
		if (ts.FallbackToCP932) {
			// fallback����ꍇ
			if ((ts.Language == IdJapanese) && ismbbleadSJIS(b)) {
				// ���{��̏ꍇ && Shift_JIS 1byte��
				// Shift_JIS �� fallback
				Fallbacked = TRUE;
				ConvJIS = FALSE;
				Kanji = b << 8;
				KanjiIn = TRUE;
				return TRUE;
			}
			// fallback ISO8859-1
			PutU32(b);
			return TRUE;
		}
		else {
			// fallback���Ȃ�, �s���ȕ�������
			w->buf[0] = b;
			PutReplacementChr(w, w->buf, 1, FALSE);
		}
		return TRUE;
	}

	// 2byte�ȍ~����?
	if((b & 0xc0) != 0x80) {	// == (b <= 0x7f || 0xc0 <= b)
		// �s���ȕ���, (���2bit�� 0b10xx_xxxx �ł͂Ȃ�)
		PutReplacementChr(w, w->buf, w->count, ts.FallbackToCP932);
		w->count = 0;
		goto recheck;
	}

	// 2byte�ڈȍ~�ۑ�
	w->buf[w->count++] = b;

	// 2byte(11bit)
	if (w->count == 2) {
		if ((w->buf[0] & 0xe0) == 0xc0) {	// == (0xc2 <= w->buf[0] && w->buf[0] <= 0xdf)
			// 5bit + 6bit
			code = ((w->buf[0] & 0x1f) << 6) | (b & 0x3f);
			PutU32(code);
			w->count = 0;
			return TRUE;
		}
		return TRUE;
	}

	// 3byte(16bit)
	if (w->count == 3) {
		if ((w->buf[0] & 0xf0) == 0xe0) {
			if ((w->buf[0] == 0xe0 && (w->buf[1] < 0xa0 || 0xbf < w->buf[1])) ||
				(w->buf[0] == 0xed && (                 0x9f < w->buf[1]))) {
				// �s���� UTF-8
				PutReplacementChr(w, w->buf, 2, ts.FallbackToCP932);
				w->count = 0;
				goto recheck;
			}
			// 4bit + 6bit + 6bit
			code = ((w->buf[0] & 0xf) << 12);
			code |= ((w->buf[1] & 0x3f) << 6);
			code |= ((w->buf[2] & 0x3f));
			PutU32(code);
			w->count = 0;
			return TRUE;
		}
		return TRUE;
	}

	// 4byte(21bit)
	assert(w->count == 4);
	assert((w->buf[0] & 0xf8) == 0xf0);
	if ((w->buf[0] == 0xf0 && (w->buf[1] < 0x90 || 0x9f < w->buf[1])) ||
		(w->buf[0] == 0xf4 && (w->buf[1] < 0x80 || 0x8f < w->buf[1]))) {
		// �s���� UTF-8
		PutReplacementChr(w, w->buf, 3, ts.FallbackToCP932);
		w->count = 0;
		goto recheck;
	}
	// 3bit + 6bit + 6bit + 6bit
	code = ((w->buf[0] & 0x07) << 18);
	code |= ((w->buf[1] & 0x3f) << 12);
	code |= ((w->buf[2] & 0x3f) << 6);
	code |= (w->buf[3] & 0x3f);
	PutU32(code);
	w->count = 0;
	return TRUE;
}

static BOOL ParseFirstRus(BYTE b)
// returns if b is processed
{
	// CP1251�ɕϊ�
	BYTE c = RussConv(ts.KanjiCode, IdWindows, b);
	// CP1251->Unicode
	unsigned long u32 = MBCP_UTF32(c, 1251);
	PutU32(u32);
	return TRUE;
}

static BOOL ParseEnglish(BYTE b)
{
	unsigned short u16 = 0;
	int part = KanjiCodeToISO8859Part(ts.KanjiCode);
	int r = UnicodeFromISO8859(part, b, &u16);
	if (r == 0) {
		return FALSE;
	}
	if (u16 < 0x100) {
		ParseASCII((BYTE)u16);
	}
	else {
		PutU32(u16);
	}
	return TRUE;
}

void ParseFirst(BYTE b) {
	switch (ts.Language) {
	  case IdUtf8:
		  ParseFirstUTF8(b);
		return;

	  case IdJapanese:
		switch (ts.KanjiCode) {
		  case IdUTF8:
			  if (ParseFirstUTF8(b)) {
				return;
			}
			break;
		  default:
			if (ParseFirstJP(b))  {
				return;
			}
		}
		break;

	  case IdKorean:
		switch (ts.KanjiCode) {
		  case IdUTF8:
			if (ParseFirstUTF8(b)) {
				return;
			}
			break;
		  default:
			if (ParseFirstKR(b))  {
				return;
			}
		}
		break;

	  case IdRussian:
		if (ParseFirstRus(b)) {
			return;
		}
		break;

	case IdChinese:
		switch (ts.KanjiCode) {
		case IdUTF8:
			if (ParseFirstUTF8(b)) {
				return;
			}
			break;
		default:
			if (ParseFirstCn(b)) {
				return;
			}
		}
		break;
	case IdEnglish: {
		if (ParseEnglish(b)) {
			return;
		}
		break;
	}
	}

	if (SSflag) {
		PutChar(b);
		SSflag = FALSE;
		return;
	}

	if (b<=US)
		ParseControl(b);
	else if ((b>=0x20) && (b<=0x7E))
		PutChar(b);
	else if ((b>=0x80) && (b<=0x9F))
		ParseControl(b);
	else if (b>=0xA0)
		PutChar(b);
}

/**
 *	�w��(Designate)
 *
 *	@param	Gn			0/1/2/3 = G0/G1/G2/G3
 *	@param	codeset		IdASCII	   0
 *						IdKatakana 1
 *						IdKanji	   2
 *						IdSpecial  3
 */
void CharSet2022Designate(int gn, int cs)
{
	VttermKanjiWork *w = &KanjiWork;
	w->Gn[gn] = cs;
}

/**
 *	�Ăяo��(Invoke)
 *	@param	glr				0/1 = GL/GR (Locking shift���̂ݗL��)
 *	@param	gn				0/1/2/3 = G0/G1/G2/G3
 *	@param	single_shift	FALSE	Locking shift
 *							TRUE	Single shift
 */
void CharSet2022Invoke(int glr, int gn, BOOL single_shift)
{
	VttermKanjiWork *w = &KanjiWork;
	if (single_shift == FALSE) {
		// Locking shift
		w->Glr[glr] = gn;
	}
	else {
		// Single shift
		GLtmp = gn;
		SSflag = TRUE;
	}
}

/**
 *	DEC����t�H���g(Tera Special font)
 *	0140(0x60) ... 0176(0x7f) �Ɍr���ŃA�T�C������Ă���
 *      (0xe0) ...     (0xff) ��?
 *	<ESC>(0 �Ƃ�������ȃG�X�P�[�v�V�[�P���X�Œ�`
 *	about/emulations.html
 *
 *	@param	b		�R�[�h
 *	@retval	TRUE	IdSpecial
 *	@retval	FALSE	IdSpecial�ł͂Ȃ�
 */
BOOL CharSetIsSpecial(BYTE b)
{
	VttermKanjiWork *w = &KanjiWork;
	BOOL SpecialNew = FALSE;

	if ((b>0x5F) && (b<0x80)) {
		if (SSflag)
			SpecialNew = (w->Gn[GLtmp]==IdSpecial);
		else
			SpecialNew = (w->Gn[w->Glr[0]]==IdSpecial);
	}
	else if (b>0xDF) {
		if (SSflag)
			SpecialNew = (w->Gn[GLtmp]==IdSpecial);
		else
			SpecialNew = (w->Gn[w->Glr[1]]==IdSpecial);
	}

	return SpecialNew;
}

static void CharSetSaveStateLow(CharSetState *state, const VttermKanjiWork *w)
{
	int i;
	state->infos[0] = w->Glr[0];
	state->infos[1] = w->Glr[1];
	for (i=0 ; i<=3; i++) {
		state->infos[2 + i] = w->Gn[i];
	}
}

/**
 *	��Ԃ�ۑ�����
 */
void CharSetSaveState(CharSetState *state)
{
	VttermKanjiWork *w = &KanjiWork;
	CharSetSaveStateLow(state, w);
}

/**
 *	��Ԃ𕜋A����
 */
void CharSetLoadState(const CharSetState *state)
{
	VttermKanjiWork *w = &KanjiWork;
	int i;
	w->Glr[0] = state->infos[0];
	w->Glr[1] = state->infos[1];
	for (i=0 ; i<=3; i++) {
		w->Gn[i] = state->infos[2 + i];
	}
}

/**
 *	�t�H�[���o�b�N�̏I��
 *		��M�f�[�^UTF-8���ɁAShift_JIS�o�͒�(fallback���)�𒆒f����
 *
 */
void CharSetFallbackFinish(void)
{
	Fallbacked = FALSE;
}