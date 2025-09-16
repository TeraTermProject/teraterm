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

#include <stdio.h>
#include <string.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include <windows.h>

#include "ttwinman.h"	// for ts
#include "codeconv.h"
#include "codeconv_mb.h"
#include "unicode.h"
#include "ttcstd.h"
#include "vtterm.h"
#include "tttypes_charset.h"
#include "ttlib_charset.h"

#include "charset.h"

// UTF-8が不正な値だった時に表示する文字
#define REPLACEMENT_CHARACTER	0xfffd		// REPLACEMENT CHARACTER
//#define REPLACEMENT_CHARACTER	0x2e2e		// Reversed Question Mark (VT382)

typedef struct CharSetDataTag {
	/* GL, GR code group */
	int Glr[2];
	/* G0, G1, G2, G3 code group */
	CharSetCS Gn[4];
	/* GL for single shift 2/3 */
	int GLtmp;
	/* single shift 2/3 flag */
	BOOL SSflag;
	//
	char32_t replacement_char;
	// UTF-8 work
	BYTE buf[4];
	int count;
	BOOL Fallbacked;

	// MBCS
	BOOL KanjiIn;				// TRUE = MBCSの1byte目を受信している
	WORD Kanji;

	// EUC
	BOOL EUCkanaIn;
	BOOL EUCsupIn;
	int  EUCcount;

	/* JIS -> SJIS conversion flag */
	BOOL ConvJIS;
	BYTE DebugFlag;

	// Operations
	CharSetOp Op;
	void *ClientData;
} CharSetData;

static BOOL IsC0(char32_t b)
{
	return (b <= US);
}

static BOOL IsC1(char32_t b)
{
	return ((b>=0x80) && (b<=0x9F));
}

/**
 *	ISO2022用ワークを初期化する
 */
static void CharSetInit2(CharSetData *w)
{
	if (LangIsJapanese(ts.KanjiCode)) {
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
CharSetData *CharSetInit(const CharSetOp *op, void *client_data)
{
	CharSetData *w = (CharSetData *)calloc(1, sizeof(*w));
	if (w == NULL) {
		return NULL;
	}

	w->Op = *op;
	w->ClientData = client_data;

	CharSetInit2(w);
	w->GLtmp = 0;
	w->SSflag = FALSE;

	w->DebugFlag = DEBUG_FLAG_NONE;

	w->replacement_char = REPLACEMENT_CHARACTER;
	w->SSflag = FALSE;

	w->KanjiIn = FALSE;
	w->EUCkanaIn = FALSE;
	w->EUCsupIn = FALSE;
	w->ConvJIS = FALSE;
	w->Fallbacked = FALSE;

	return w;
}

void CharSetFinish(CharSetData *w)
{
	assert(w != NULL);
	free(w);
}

/**
 *	1byte目チェック
 */
static BOOL CheckFirstByte(BYTE b, int kanji_code)
{
	switch (kanji_code) {
		case IdKoreanCP949:
			return __ismbblead(b, 949);
		case IdCnGB2312:
			return __ismbblead(b, 936);
		case IdCnBig5:
			return __ismbblead(b, 950);
		default:
			assert(FALSE);
			return FALSE;
	}
}

/**
 *	Double-byte Character Sets
 *	SJISの1byte目?
 *
 *	第1バイト0x81...0x9F or 0xE0...0xEF
 *	第1バイト0x81...0x9F or 0xE0...0xFC
 */
static BOOL ismbbleadSJIS(BYTE b)
{
	if (((0x80<b) && (b<0xa0)) || ((0xdf<b) && (b<0xfd))) {
		return TRUE;
	}
	return FALSE;
}

/**
 *	ts.Language == IdJapanese 時
 *	1byte目チェック
 */
static BOOL CheckKanji(CharSetData *w, BYTE b)
{
	BOOL Check;

	if (!LangIsJapanese(ts.KanjiCode) && ts.KanjiCode != IdUTF8) {
		return FALSE;
	}

	w->ConvJIS = FALSE;

	if (ts.KanjiCode==IdSJIS ||
	   (ts.FallbackToCP932 && ts.KanjiCode==IdUTF8)) {
		if (((0x80<b) && (b<0xa0)) || ((0xdf<b) && (b<0xfd))) {
			w->Fallbacked = TRUE;
			return TRUE; // SJIS kanji
		}
		if ((0xa1<=b) && (b<=0xdf)) {
			return FALSE; // SJIS katakana
		}
	}

	if ((b>=0x21) && (b<=0x7e)) {
		Check = (w->Gn[w->Glr[0]] == IdKanji);
		w->ConvJIS = Check;
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		Check = (w->Gn[w->Glr[1]] == IdKanji);
		if (ts.KanjiCode==IdEUC) {
			Check = TRUE;
		}
		else if (ts.KanjiCode==IdJIS && ((ts.TermFlag & TF_FIXEDJIS)!=0) && (ts.JIS7Katakana==0)) {
			Check = FALSE; // 8-bit katakana
		}
		w->ConvJIS = Check;
	}
	else {
		Check = FALSE;
	}

	return Check;
}

static BOOL ParseFirstJP(CharSetData *w, BYTE b)
// returns TRUE if b is processed
//  (actually allways returns TRUE)
{
	if (w->KanjiIn) {
		if (((! w->ConvJIS) && (0x3F<b) && (b<0xFD)) ||
			(w->ConvJIS && ( ((0x20<b) && (b<0x7f)) ||
						  ((0xa0<b) && (b<0xff)) )) )
		{
			unsigned long u32;
			w->Kanji = w->Kanji + b;
			if (w->ConvJIS) {
				// JIS -> Shift_JIS(CP932)
				w->Kanji = CodeConvJIS2SJIS((WORD)(w->Kanji & 0x7f7f));
			}
			u32 = CP932ToUTF32(w->Kanji);
			w->Op.PutU32(u32, w->ClientData);
			w->KanjiIn = FALSE;
			return TRUE;
		}
		else if ((ts.TermFlag & TF_CTRLINKANJI)==0) {
			w->KanjiIn = FALSE;
		}
	}

	if (w->SSflag) {
		if (w->Gn[w->GLtmp] == IdKanji) {
			w->Kanji = b << 8;
			w->KanjiIn = TRUE;
			w->SSflag = FALSE;
			return TRUE;
		}
		else if (w->Gn[w->GLtmp] == IdKatakana) {
			b = b | 0x80;
		}

		w->Op.PutU32(b, w->ClientData);
		w->SSflag = FALSE;
		return TRUE;
	}

	if ((!w->EUCsupIn) && (!w->EUCkanaIn) && (!w->KanjiIn) && CheckKanji(w, b)) {
		w->Kanji = b << 8;
		w->KanjiIn = TRUE;
		return TRUE;
	}

	if (b<=US) {
		w->Op.ParseControl(b, w->ClientData);
	}
	else if (b==0x20) {
		w->Op.PutU32(b, w->ClientData);
	}
	else if ((b>=0x21) && (b<=0x7E)) {
		if (w->EUCsupIn) {
			w->EUCcount--;
			w->EUCsupIn = (w->EUCcount==0);
			return TRUE;
		}

		if ((w->Gn[w->Glr[0]] == IdKatakana) || w->EUCkanaIn) {
			b = b | 0x80;
			w->EUCkanaIn = FALSE;
			{
				// bはsjisの半角カタカナ
				unsigned long u32 = CP932ToUTF32(b);
				w->Op.PutU32(u32, w->ClientData);
			}
			return TRUE;
		}
		w->Op.PutU32(b, w->ClientData);
	}
	else if (b==0x7f) {
		return TRUE;
	}
	else if ((b>=0x80) && (b<=0x8D)) {
		w->Op.ParseControl(b, w->ClientData);
	}
	else if (b==0x8E) { // SS2
		switch (ts.KanjiCode) {
		case IdEUC:
			if (ts.ISO2022Flag & ISO2022_SS2) {
				w->EUCkanaIn = TRUE;
			}
			break;
		case IdUTF8:
			w->Op.PutU32(w->replacement_char, w->ClientData);
			break;
		default:
			w->Op.ParseControl(b, w->ClientData);
		}
	}
	else if (b==0x8F) { // SS3
		switch (ts.KanjiCode) {
		case IdEUC:
			if (ts.ISO2022Flag & ISO2022_SS3) {
				w->EUCcount = 2;
				w->EUCsupIn = TRUE;
			}
			break;
		case IdUTF8:
			w->Op.PutU32(w->replacement_char, w->ClientData);
			break;
		default:
			w->Op.ParseControl(b, w->ClientData);
		}
	}
	else if ((b>=0x90) && (b<=0x9F)) {
		w->Op.ParseControl(b, w->ClientData);
	}
	else if (b==0xA0) {
		w->Op.PutU32(0x20, w->ClientData);
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		if (w->EUCsupIn) {
			w->EUCcount--;
			w->EUCsupIn = (w->EUCcount==0);
			return TRUE;
		}

		if ((w->Gn[w->Glr[1]] != IdASCII) ||
		    ((ts.KanjiCode==IdEUC) && w->EUCkanaIn) ||
		    (ts.KanjiCode==IdSJIS) ||
		    ((ts.KanjiCode==IdJIS) &&
			 (ts.JIS7Katakana==0) &&
			 ((ts.TermFlag & TF_FIXEDJIS)!=0))) {
			// bはsjisの半角カタカナ
			unsigned long u32 = CP932ToUTF32(b);
			w->Op.PutU32(u32, w->ClientData);
		} else {
			if (w->Gn[w->Glr[1]] == IdASCII) {
				b = b & 0x7f;
			}
			w->Op.PutU32(b, w->ClientData);
		}
		w->EUCkanaIn = FALSE;
	}
	else {
		w->Op.PutU32(b, w->ClientData);
	}

	return TRUE;
}

static BOOL ParseFirstKR(CharSetData *w, BYTE b)
// returns TRUE if b is processed
//  (actually allways returns TRUE)
{
	if (w->KanjiIn) {
		if (((0x41<=b) && (b<=0x5A)) ||
			((0x61<=b) && (b<=0x7A)) ||
			((0x81<=b) && (b<=0xFE)))
		{
			unsigned long u32 = 0;
			if (ts.KanjiCode == IdKoreanCP949) {
				// CP949
				w->Kanji = w->Kanji + b;
				u32 = MBCP_UTF32(w->Kanji, 949);
			}
			else {
				assert(FALSE);
			}
			w->Op.PutU32(u32, w->ClientData);
			w->KanjiIn = FALSE;
			return TRUE;
		}
		else if ((ts.TermFlag & TF_CTRLINKANJI)==0) {
			w->KanjiIn = FALSE;
		}
	}

	if ((!w->KanjiIn) && CheckFirstByte(b, ts.KanjiCode)) {
		w->Kanji = b << 8;
		w->KanjiIn = TRUE;
		return TRUE;
	}

	if (b<=US) {
		w->Op.ParseControl(b, w->ClientData);
	}
	else if (b==0x20) {
		w->Op.PutU32(b, w->ClientData);
	}
	else if ((b>=0x21) && (b<=0x7E)) {
//		if (Gn[Glr[0]] == IdKatakana) {
//			b = b | 0x80;
//		}
		w->Op.PutU32(b, w->ClientData);
	}
	else if (b==0x7f) {
		return TRUE;
	}
	else if ((0x80<=b) && (b<=0x9F)) {
		w->Op.ParseControl(b, w->ClientData);
	}
	else if (b==0xA0) {
		w->Op.PutU32(0x20, w->ClientData);
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		if (w->Gn[w->Glr[1]] == IdASCII) {
			b = b & 0x7f;
		}
		w->Op.PutU32(b, w->ClientData);
	}
	else {
		w->Op.PutU32(b, w->ClientData);
	}

	return TRUE;
}

static BOOL ParseFirstCn(CharSetData *w, BYTE b)
// returns TRUE if b is processed
//  (actually allways returns TRUE)
{
	if (w->KanjiIn) {
		// TODO
		if (((0x40<=b) && (b<=0x7e)) ||
		    ((0xa1<=b) && (b<=0xFE)))
		{
			unsigned long u32 = 0;
			w->Kanji = w->Kanji + b;
			if (ts.KanjiCode == IdCnGB2312) {
				// CP936 GB2312
				u32 = MBCP_UTF32(w->Kanji, 936);
			}
			else if (ts.KanjiCode == IdCnBig5) {
				// CP950 Big5
				u32 = MBCP_UTF32(w->Kanji, 950);
			}
			else {
				assert(FALSE);
			}
			w->Op.PutU32(u32, w->ClientData);
			w->KanjiIn = FALSE;
			return TRUE;
		}
		else if ((ts.TermFlag & TF_CTRLINKANJI)==0) {
			w->KanjiIn = FALSE;
		}
	}

	if ((!w->KanjiIn) && CheckFirstByte(b, ts.KanjiCode)) {
		w->Kanji = b << 8;
		w->KanjiIn = TRUE;
		return TRUE;
	}

	if (b<=US) {
		w->Op.ParseControl(b, w->ClientData);
	}
	else if (b==0x20) {
		w->Op.PutU32(b, w->ClientData);
	}
	else if ((b>=0x21) && (b<=0x7E)) {
//		if (Gn[Glr[0]] == IdKatakana) {
//			b = b | 0x80;
//		}
		w->Op.PutU32(b, w->ClientData);
	}
	else if (b==0x7f) {
		return TRUE;
	}
	else if ((0x80<=b) && (b<=0x9F)) {
		w->Op.ParseControl(b, w->ClientData);
	}
	else if (b==0xA0) {
		w->Op.PutU32(0x20, w->ClientData);
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		if (w->Gn[w->Glr[1]] == IdASCII) {
			b = b & 0x7f;
		}
		w->Op.PutU32(b, w->ClientData);
	}
	else {
		w->Op.PutU32(b, w->ClientData);
	}

	return TRUE;
}

static void ParseASCII(CharSetData *w, BYTE b)
{
	if (w->SSflag) {
		w->Op.PutU32(b, w->ClientData);
		w->SSflag = FALSE;
		return;
	}

	if (b<=US) {
		w->Op.ParseControl(b, w->ClientData);
	} else if ((b>=0x20) && (b<=0x7E)) {
		w->Op.PutU32(b, w->ClientData);
	} else if ((b==0x8E) || (b==0x8F)) {
		w->Op.PutU32(w->replacement_char, w->ClientData);
	} else if ((b>=0x80) && (b<=0x9F)) {
		w->Op.ParseControl(b, w->ClientData);
	} else if (b>=0xA0) {
		w->Op.PutU32(b, w->ClientData);
	}
}

/**
 *	REPLACEMENT_CHARACTER の表示
 *	UTF-8 デコードから使用
 */
static void PutReplacementChr(CharSetData *w, const BYTE *ptr, size_t len, BOOL fallback)
{
	const char32_t replacement_char = w->replacement_char;
	int i;
	for (i = 0; i < len; i++) {
		BYTE c = *ptr++;
		assert(!IsC0(c));
		if (fallback) {
			// fallback ISO8859-1
			w->Op.PutU32(c, w->ClientData);
		}
		else {
			// fallbackしない
			if (c < 0x80) {
				// 不正なUTF-8文字列のなかに0x80未満があれば、
				// 1文字のUTF-8文字としてそのまま表示する
				w->Op.PutU32(c, w->ClientData);
			}
			else {
				w->Op.PutU32(replacement_char, w->ClientData);
			}
		}
	}
}

/**
 * UTF-8で受信データを処理する
 *
 * returns TRUE if b is processed
 */
static BOOL ParseFirstUTF8(CharSetData *w, BYTE b)
{
	char32_t code;

	if (w->Fallbacked) {
		BOOL r = ParseFirstJP(w, b);
		w->Fallbacked = FALSE;
		return r;
	}

	// UTF-8エンコード
	//	The Unicode Standard Chapter 3
	//	Table 3-7. Well-Formed UTF-8 Byte Sequences
	// | Code Points        | 1st Byte | 2nd Byte | 3rd Byte | 4th Byte |
	// | U+0000..U+007F     | 00..7F   |          |          |          |
	// | U+0080..U+07FF     | C2..DF   | 80..BF   |          |          |
	// | U+0800..U+0FFF     | E0       | A0..BF   | 80..BF   |          |
	// | U+1000..U+CFFF     | E1..EC   | 80..BF   | 80..BF   |          |
	// | U+D000..U+D7FF     | ED       | 80..9F   | 80..BF   |          |
	// | U+E000..U+FFFF     | EE..EF   | 80..BF   | 80..BF   |          |
	// | U+10000..U+3FFFF   | F0       | 90..BF   | 80..BF   | 80..BF   |
	// | U+40000..U+FFFFF   | F1..F3   | 80..BF   | 80..BF   | 80..BF   |
	// | U+100000..U+10FFFF | F4       | 80..8F   | 80..BF   | 80..BF   |
	//	- 1byte目
	//		- 0x00 - 0x7f		ok
	//		- 0x80 - 0xc1		ng
	//		- 0xc2 - 0xf4		ok
	//		- 0xf5 - 0xff		ng
	//	- 2byte目以降
	//		- 0x00 - 0x7f		ng
	//		- 0x80 - 0xbf		ok
	//		- 0xc0 - 0xff		ng
	//  - 2byte目例外
	//		- 1byte == 0xe0 のとき 0xa0 - 0xbfのみok
	//		- 1byte == 0xed のとき 0x80 - 0x9fのみok
	//		- 1byte == 0xf0 のとき 0x90 - 0xbfのみok
	//		- 1byte == 0xf4 のとき 0x90 - 0x8fのみok
recheck:
	// 1byte(7bit)
	if (w->count == 0) {
		if (IsC0(b)) {
			// U+0000 .. U+001f
			// C0制御文字, C0 Coontrols
			w->Op.ParseControl(b, w->ClientData);
			return TRUE;
		}
		else if (b <= 0x7f) {
			// 0x7f以下, のとき、そのまま出力
			w->Op.PutU32(b, w->ClientData);
			return TRUE;
		}
		else if (0xc2 <= b && b <= 0xf4) {
			// 1byte目保存
			w->buf[w->count++] = b;
			return TRUE;
		}

		// 0x80 - 0xc1, 0xf5 - 0xff
		// UTF-8で1byteに出現しないコードのとき
		if (ts.FallbackToCP932) {
			// fallbackする場合
			if (LangIsJapanese(ts.KanjiCode) && ismbbleadSJIS(b)) {
				// 日本語の場合 && Shift_JIS 1byte目
				// Shift_JIS に fallback
				w->Fallbacked = TRUE;
				w->ConvJIS = FALSE;
				w->Kanji = b << 8;
				w->KanjiIn = TRUE;
				return TRUE;
			}
		}
		// fallbackしない, 不正な文字入力
		w->buf[0] = b;
		PutReplacementChr(w, w->buf, 1, FALSE);
		return TRUE;
	}

	// 2byte以降正常?
	if((b & 0xc0) != 0x80) {	// == (b <= 0x7f || 0xc0 <= b)
		// 不正な文字, (上位2bitが 0b10xx_xxxx ではない)
		PutReplacementChr(w, w->buf, w->count, ts.FallbackToCP932);
		w->count = 0;
		goto recheck;
	}

	// 2byte目以降保存
	w->buf[w->count++] = b;

	// 2byte(11bit)
	if (w->count == 2) {
		if ((w->buf[0] & 0xe0) == 0xc0) {	// == (0xc2 <= w->buf[0] && w->buf[0] <= 0xdf)
			// 5bit + 6bit
			code = ((w->buf[0] & 0x1f) << 6) | (b & 0x3f);
			if (IsC1(code)) {
				// U+0080 .. u+009f
				// C1制御文字, C1 Controls
				w->Op.ParseControl((BYTE)code, w->ClientData);
			}
			else {
				w->Op.PutU32(code, w->ClientData);
			}
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
				// 不正な UTF-8
				PutReplacementChr(w, w->buf, 2, ts.FallbackToCP932);
				w->count = 0;
				goto recheck;
			}
			// 4bit + 6bit + 6bit
			code = ((w->buf[0] & 0xf) << 12);
			code |= ((w->buf[1] & 0x3f) << 6);
			code |= ((w->buf[2] & 0x3f));
			w->Op.PutU32(code, w->ClientData);
			w->count = 0;
			return TRUE;
		}
		return TRUE;
	}

	// 4byte(21bit)
	assert(w->count == 4);
	assert((w->buf[0] & 0xf8) == 0xf0);
	if ((w->buf[0] == 0xf0 && (w->buf[1] < 0x90 || 0xbf < w->buf[1])) ||
		(w->buf[0] == 0xf4 && (w->buf[1] < 0x80 || 0x8f < w->buf[1]))) {
		// 不正な UTF-8
		PutReplacementChr(w, w->buf, 3, ts.FallbackToCP932);
		w->count = 0;
		goto recheck;
	}
	// 3bit + 6bit + 6bit + 6bit
	code = ((w->buf[0] & 0x07) << 18);
	code |= ((w->buf[1] & 0x3f) << 12);
	code |= ((w->buf[2] & 0x3f) << 6);
	code |= (w->buf[3] & 0x3f);
	w->Op.PutU32(code, w->ClientData);
	w->count = 0;
	return TRUE;
}

static BOOL ParseEnglish(CharSetData *w, BYTE b)
{
	unsigned short u16 = 0;
	int r = UnicodeFromISO8859((IdKanjiCode)ts.KanjiCode, b, &u16);
	if (r == 0) {
		return FALSE;
	}
	if (u16 < 0x100) {
		ParseASCII(w, (BYTE)u16);
	}
	else {
		w->Op.PutU32(u16, w->ClientData);
	}
	return TRUE;
}

static BOOL ParseCodePage(IdKanjiCode kanji_code, CharSetData *w, BYTE b)
{
	unsigned short u16 = 0;
	int r = UnicodeFromCodePage(kanji_code, b, &u16);
	if (r == 0) {
		return FALSE;
	}
	if (u16 < 0x100) {
		ParseASCII(w, (BYTE)u16);
	}
	else {
		w->Op.PutU32(u16, w->ClientData);
	}
	return TRUE;
}

static void PutDebugChar(CharSetData *w, BYTE b)
{
	int i;
	BOOL svInsertMode, svAutoWrapMode;
	TCharAttr svCharAttr;
	TCharAttr char_attr;

	svInsertMode = TermGetInsertMode();
	TermSetInsertMode(FALSE);
	svAutoWrapMode = TermGetAutoWrapMode();
	TermSetAutoWrapMode(TRUE);

	TermGetAttr(&svCharAttr);
	char_attr = svCharAttr;
	char_attr.Attr = AttrDefault;
	TermSetAttr(&char_attr);

	if (w->DebugFlag==DEBUG_FLAG_HEXD) {
		char buff[3];
		_snprintf(buff, 3, "%02X", (unsigned int) b);

		for (i=0; i<2; i++)
			w->Op.PutU32(buff[i], w->ClientData);
		w->Op.PutU32(' ', w->ClientData);
	}
	else if (w->DebugFlag==DEBUG_FLAG_NORM) {

		if ((b & 0x80) == 0x80) {
			//UpdateStr();
			char_attr.Attr = AttrReverse;
			TermSetAttr(&char_attr);
			b = b & 0x7f;
		}

		if (b<=US) {
			w->Op.PutU32('^', w->ClientData);
			w->Op.PutU32((char)(b + 0x40), w->ClientData);
		}
		else if (b==DEL) {
			w->Op.PutU32('<', w->ClientData);
			w->Op.PutU32('D', w->ClientData);
			w->Op.PutU32('E', w->ClientData);
			w->Op.PutU32('L', w->ClientData);
			w->Op.PutU32('>', w->ClientData);
		}
		else
			w->Op.PutU32(b, w->ClientData);
	}

	TermSetAttr(&char_attr);
	TermSetInsertMode(svInsertMode);
	TermSetAutoWrapMode(svAutoWrapMode);
}

void ParseFirst(CharSetData *w, BYTE b)
{
	IdKanjiCode kanji_code = (IdKanjiCode)ts.KanjiCode;
	if (w->DebugFlag != DEBUG_FLAG_NONE) {
		kanji_code = IdDebug;
	}

	switch (kanji_code) {
	case IdSJIS:
	case IdEUC:
	case IdJIS:
		if (ParseFirstJP(w, b))  {
			return;
		}
		break;

	case IdISO8859_1:
	case IdISO8859_2:
	case IdISO8859_3:
	case IdISO8859_4:
	case IdISO8859_5:
	case IdISO8859_6:
	case IdISO8859_7:
	case IdISO8859_8:
	case IdISO8859_9:
	case IdISO8859_10:
	case IdISO8859_11:
	case IdISO8859_13:
	case IdISO8859_14:
	case IdISO8859_15:
	case IdISO8859_16:
		if (ParseEnglish(w, b)) {
			return;
		}
		break;

	case IdUTF8:
		ParseFirstUTF8(w, b);
		return;

	case IdKoreanCP949:
		if (ParseFirstKR(w, b))  {
			return;
		}
		break;

	case IdCnGB2312:
	case IdCnBig5:
		if (ParseFirstCn(w, b)) {
			return;
		}
		break;

	case IdDebug:
		PutDebugChar(w, b);
		return;

	case IdCP437:
	case IdCP737:
	case IdCP775:
	case IdCP850:
	case IdCP852:
	case IdCP855:
	case IdCP857:
	case IdCP860:
	case IdCP861:
	case IdCP862:
	case IdCP863:
	case IdCP864:
	case IdCP865:
	case IdCP866:
	case IdCP869:
	case IdCP874:
	case IdCP1250:
	case IdCP1251:
	case IdCP1252:
	case IdCP1253:
	case IdCP1254:
	case IdCP1255:
	case IdCP1256:
	case IdCP1257:
	case IdCP1258:
	case IdKOI8_NEW:
		if (ParseCodePage(kanji_code, w, b)) {
			return;
		}
		return;
#if !defined(__MINGW32__)
	default:
		// gcc/clangではswitchにenumのメンバがすべてないとき警告が出る
		assert(FALSE);
		break;
#endif
	}

	if (w->SSflag) {
		w->Op.PutU32(b, w->ClientData);
		w->SSflag = FALSE;
		return;
	}

	if (b<=US)
		w->Op.ParseControl(b, w->ClientData);
	else if ((b>=0x20) && (b<=0x7E))
		w->Op.PutU32(b, w->ClientData);
	else if ((b>=0x80) && (b<=0x9F))
		w->Op.ParseControl(b, w->ClientData);
	else if (b>=0xA0)
		w->Op.PutU32(b, w->ClientData);
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
void CharSet2022Designate(CharSetData *w, int gn, CharSetCS cs)
{
	w->Gn[gn] = cs;
}

/**
 *	呼び出し(Invoke)
 *	@param	shift
 */
void CharSet2022Invoke(CharSetData *w, CharSet2022Shift shift)
{
	switch (shift) {
	case CHARSET_LS0:
		// Locking Shift 0 (G0->GL)
		w->Glr[0] = 0;
		break;
	case CHARSET_LS1:
		// Locking Shift 1 (G1->GL)
		w->Glr[0] = 1;
		break;
	case CHARSET_LS2:
		// Locking Shift 2 (G2->GL)
		w->Glr[0] = 2;
		break;
	case CHARSET_LS3:
		// Locking Shift 3 (G3->GL)
		w->Glr[0] = 3;
		break;
	case CHARSET_LS1R:
		// Locking Shift 1 (G1->GR)
		w->Glr[1] = 1;
		break;
	case CHARSET_LS2R:
		// Locking Shift 2 (G2->GR)
		w->Glr[1] = 2;
		break;
	case CHARSET_LS3R:
		// Locking Shift 3 (G3->GR)
		w->Glr[1] = 3;
		break;
	case CHARSET_SS2:
		// Single Shift 2
		w->GLtmp = 2;
		w->SSflag = TRUE;
		break;
	case CHARSET_SS3:
		// Single Shift 3
		w->GLtmp = 3;
		w->SSflag = TRUE;
		break;
	default:
		assert(FALSE);
		break;
	}
}

/**
 *	DEC特殊フォント(DEC Special Graphics, DSG)
 *	0137(0x5f) ... 0176(0x7e) に罫線でアサインされている
 *      (0xdf) ...     (0xfe) も?
 *	<ESC>(0 という特殊なエスケープシーケンスで定義
 *	about/emulations.html
 *
 *	@param	b		コード
 *	@retval	TRUE	IdSpecial
 *	@retval	FALSE	IdSpecialではない
 */
BOOL CharSetIsSpecial(CharSetData *w, BYTE b)
{
	BOOL SpecialNew = FALSE;

	if ((b >= 0x5F) && (b < 0x7f)) {
		if (w->SSflag) {
			SpecialNew = (w->Gn[w->GLtmp] == IdSpecial);
		}
		else {
			SpecialNew = (w->Gn[w->Glr[0]] == IdSpecial);
		}
	}
	else if ((b >= 0xDF) && (b < 0xff)) {
		if (w->SSflag) {
			SpecialNew = (w->Gn[w->GLtmp] == IdSpecial);
		}
		else {
			SpecialNew = (w->Gn[w->Glr[1]] == IdSpecial);
		}
	}

	return SpecialNew;
}

static void CharSetSaveStateLow(CharSetState *state, const CharSetData *w)
{
	int i;
	state->infos[0] = w->Glr[0];
	state->infos[1] = w->Glr[1];
	for (i=0 ; i<=3; i++) {
		state->infos[2 + i] = (int)w->Gn[i];
	}
}

/**
 *	状態を保存する
 */
void CharSetSaveState(CharSetData *w, CharSetState *state)
{
	CharSetSaveStateLow(state, w);
}

/**
 *	状態を復帰する
 */
void CharSetLoadState(CharSetData *w, const CharSetState *state)
{
	int i;
	w->Glr[0] = state->infos[0];
	w->Glr[1] = state->infos[1];
	for (i=0 ; i<=3; i++) {
		w->Gn[i] = (CharSetCS)state->infos[2 + i];
	}
}

/**
 *	フォールバックの終了
 *		受信データUTF-8時に、Shift_JIS出力中(fallback状態)を中断する
 *
 */
void CharSetFallbackFinish(CharSetData *w)
{
	w->Fallbacked = FALSE;
}

/**
 *	デバグ出力を次のモードに変更する
 */
void CharSetSetNextDebugMode(CharSetData *w)
{
	// ts.DebugModes には tttypes.h の DBGF_* が OR で入ってる
	do {
		w->DebugFlag = (w->DebugFlag + 1) % DEBUG_FLAG_MAXD;
	} while (w->DebugFlag != DEBUG_FLAG_NONE && !((ts.DebugModes >> (w->DebugFlag - 1)) & 1));
}

BYTE CharSetGetDebugMode(CharSetData *w)
{
	return w->DebugFlag;
}

void CharSetSetDebugMode(CharSetData *w, BYTE mode)
{
	w->DebugFlag = mode % DEBUG_FLAG_MAXD;
}
