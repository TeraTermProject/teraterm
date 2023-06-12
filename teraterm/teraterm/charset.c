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

#include "charset.h"

static BOOL KanjiIn;				// TRUE = MBCSの1byte目を受信している
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
BOOL Fallbacked;

typedef struct {
	/* GL, GR code group */
	int Glr[2];
	/* G0, G1, G2, G3 code group */
	int Gn[4];
} VttermKanjiWork;

static VttermKanjiWork KanjiWork;

// Unicodeベースに切り替え
static void PutChar(BYTE b)
{
	PutU32(b);
}

/**
 *	ISO2022用ワークを初期化する
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
 *	漢字関連ワークを初期化する
 */
void CharSetInit(void)
{
	CharSetInit2(&KanjiWork);
	SSflag = FALSE;

	KanjiIn = FALSE;
	EUCkanaIn = FALSE;
	EUCsupIn = FALSE;
	ConvJIS = FALSE;
	Fallbacked = FALSE;
}

/**
 *	1byte目チェック
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
 *	ts.Language == IdJapanese 時
 *	1byte目チェック
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
				// bはsjisの半角カタカナ
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
			PutChar('?');
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
			PutChar('?');
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
			// bはsjisの半角カタカナ
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
			if (ts.KanjiCode == IdKoreanCP51949) {
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
		PutChar('?');
	} else if ((b>=0x80) && (b<=0x9F)) {
		ParseControl(b);
	} else if (b>=0xA0) {
		PutU32(b);
	}
}

// UTF-8で受信データを処理する
// returns TRUE if b is processed
//  (actually allways returns TRUE)
static BOOL ParseFirstUTF8(BYTE b)
{
	static BYTE buf[4];
	static int count = 0;

	unsigned int code;
	int i;

	if (ts.FallbackToCP932 && Fallbacked) {
		return ParseFirstJP(b);
	}

	// UTF-8エンコード
	//	Unicode					1byte,		  2byte,	   3byte, 		  4byte
	//	U+0000  ... U+007f		0x00 .. 0x7f
	//	U+0080  ... U+07ff		0xc2 .. 0xdf, 0x80 .. 0xbf
	//	U+0800  ... U+ffff		0xe0 .. 0xef, 0x80 .. 0xbf, 0x80 .. 0xbf
	//	U+10000 ... U+10ffff	0xf0 .. 0xf4, 0x80 .. 0xbf, 0x80 .. 0xbf, 0x80 .. 0xbf
	// UTF-8でデコードできない場合
	//	- 1byte目
	//		- C1(0x80 - 0x9f)
	//		- 0xa0 - 0xc1
	//		- 0xf5 - 0xff
	//	- 2byte目以降
	//		- 0x00 - 0x7f
	//		- 0xc0 - 0xff

	// 1byte(7bit)
	if (count == 0) {
		if ((b & 0x80) == 0x00) {
			// 1byte(7bit)
			//		0x7f以下, のとき、そのまま出力
			ParseASCII(b);
			return TRUE;
		}
		if ((b & 0x40) == 0x00 || b >= 0xf6 ) {
			// UTF-8で1byteに出現しないコードのとき、そのまま出力
			//	0x40 = 0b1011_1111, 0b10xx_xxxxというbitパターンにはならない
			//  0xf6 以上のとき U+10FFFFより大きくなる
			PutU32(b);
			return TRUE;
		}
		// 1byte目保存
		buf[count++] = b;
		return TRUE;
	}

	// 2byte(11bit)
	if ((buf[0] & 0xe0) == 0xc0) {
		code = 0;
		if((b & 0xc0) == 0x80) {
			// 5bit + 6bit
			code = ((buf[0] & 0x1f) << 6) | (b & 0x3f);
			if (code < 0x80) {
				// 11bit使って7bit以下の時、UTF-8の冗長な表現
				code = 0;
			}
		}
		if (code == 0){
			// そのまま出力
			PutU32(buf[0]);
			PutU32(b);
			count = 0;
			return TRUE;
		}
		else {
			PutU32(code);
			count = 0;
			return TRUE;
		}
	}

	// 2byte目以降保存
	buf[count++] = b;

	// 3byte(16bit)
	if ((buf[0] & 0xf0) == 0xe0) {
		if(count < 3) {
			return TRUE;
		}
		code = 0;
		if ((buf[1] & 0xc0) == 0x80 && (buf[2] & 0xc0) == 0x80) {
			// 4bit + 6bit + 6bit
			code = ((buf[0] & 0xf) << 12);
			code |= ((buf[1] & 0x3f) << 6);
			code |= ((buf[2] & 0x3f));
			if (code < 0x800) {
				// 16bit使って11bit以下のとき、UTF-8の冗長な表現
				code = 0;
			}
		}
		if (code == 0) {
			// そのまま出力
			PutU32(buf[0]);
			PutU32(buf[1]);
			PutU32(buf[2]);
			count = 0;
			return TRUE;
		} else {
			PutU32(code);
			count = 0;
			return TRUE;
		}
	}

	// 4byte(21bit)
	if ((buf[0] & 0xf8) == 0xf0) {
		if(count < 4) {
			return TRUE;
		}
		code = 0;
		if ((buf[1] & 0xc0) == 0x80 && (buf[2] & 0xc0) == 0x80 && (buf[3] & 0xc0) == 0x80) {
			// 3bit + 6bit + 6bit + 6bit
			code = ((buf[0] & 0x07) << 18);
			code |= ((buf[1] & 0x3f) << 12);
			code |= ((buf[2] & 0x3f) << 6);
			code |= (buf[3] & 0x3f);
			if (code < 0x10000) {
				// 21bit使って16bit以下のとき、UTF-8の冗長な表現
				code = 0;
			}
		}
		if (code == 0) {
			// そのまま出力
			PutU32(buf[0]);
			PutU32(buf[1]);
			PutU32(buf[2]);
			PutU32(buf[3]);
			count = 0;
			return TRUE;
		} else {
			PutU32(code);
			count = 0;
			return TRUE;
		}
	}

	// ここには来ない
	assert(FALSE);

	for (i = 0; i < count; i++) {
		ParseASCII(buf[i]);
	}
	count = 0;
	return TRUE;
}

static BOOL ParseFirstRus(BYTE b)
// returns if b is processed
{
	// CP1251に変換
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
 *	指示(Designate)
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
 *	呼び出し(Invoke)
 *	@param	glr				0/1 = GL/GR (Locking shift時のみ有効)
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
 *	DEC特殊フォント(Tera Special font)
 *	0140(0x60) ... 0176(0x7f) に罫線でアサインされている
 *      (0xe0) ...     (0xff) も?
 *	<ESC>(0 という特殊なエスケープシーケンスで定義
 *	about/emulations.html
 *
 *	@param	b		コード
 *	@retval	TRUE	IdSpecial
 *	@retval	FALSE	IdSpecialではない
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
 *	状態を保存する
 */
void CharSetSaveState(CharSetState *state)
{
	VttermKanjiWork *w = &KanjiWork;
	CharSetSaveStateLow(state, w);
}

/**
 *	状態を復帰する
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
