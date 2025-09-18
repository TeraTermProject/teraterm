/*
 * (C) 2024- TeraTerm Project
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

// 内部文字コード(wchar_t) -> 出力文字コードへ変換する

#include <assert.h>

#include "tttypes.h"
#include "tttypes_charset.h"
#include "codeconv.h"
#include "../teraterm/unicode.h"
#include "codeconv_mb.h"

#include "makeoutputstring.h"

/* Character sets */
typedef enum {
	IdASCII,
	IdKatakana,
	IdKanji,
	IdSpecial,
} CharSet;

typedef struct OutputCharStateTag {
	IdKanjiCode KanjiCode;		// 出力文字コード(sjis,jisなど)

	// JIS 漢字IN/OUT/カナ
	WORD KanjiIn;		// IdKanjiInA / IdKanjiInB
	WORD KanjiOut;		// IdKanjiOutB / IdKanjiOutJ / IdKanjiOutH
	BOOL JIS7Katakana;	// (Kanji JIS)kana

	// state
	CharSet SendCode;		// [in,out](Kanji JIS)直前の送信コード Ascii/Kana/Kanji
} OutputCharState;

static const KanjiInOutSt KanjiInList[] = {
	{ IdKanjiInA, "^[$@", "@", 1 },
	{ IdKanjiInB, "^[$B", "B", 1 },
};

static const KanjiInOutSt KanjiOutList[] = {
	{ IdKanjiOutB, "^[(B", "B", 1 },
	{ IdKanjiOutJ, "^[(J", "J", 1 },
	{ IdKanjiOutH, "^[(H", "H", 0 },
};

WORD GetKanjiInCodeFromIni(const char *ini_str)
{
	for (int i = 0; i < _countof(KanjiInList); i++) {
		const KanjiInOutSt *p = &KanjiInList[i];
		if (strcmp(ini_str, p->ini_str) == 0) {
			return p->code;
		}
	}
	return IdKanjiInB;
}

WORD GetKanjiOutCodeFromIni(const char *ini_str)
{
	for (int i = 0; i < _countof(KanjiOutList); i++) {
		const KanjiInOutSt *p = &KanjiOutList[i];
		if (strcmp(ini_str, p->ini_str) == 0) {
			return p->code;
		}
	}
	return IdKanjiOutJ;
}

const KanjiInOutSt *GetKanjiInList(int index)
{
	if (index >= _countof(KanjiInList)) {
		return NULL;
	}
	return &KanjiInList[index];
}

const KanjiInOutSt *GetKanjiOutList(int index)
{
	if (index >= _countof(KanjiOutList)) {
		return NULL;
	}
	return &KanjiOutList[index];
}

/**
 *	@retval	true	日本語の半角カタカナ
 *	@retval	false	その他
 */
static BOOL IsHalfWidthKatakana(unsigned int u32)
{
	// Halfwidth CJK punctuation (U+FF61〜FF64)
	// Halfwidth Katakana variants (U+FF65〜FF9F)
	return (0xff61 <= u32 && u32 <= 0xff9f);
}

OutputCharState *MakeOutputStringCreate(void)
{
	OutputCharState *p = (OutputCharState *)calloc(1, sizeof(*p));
	return p;
}

void MakeOutputStringDestroy(OutputCharState *state)
{
	assert(state != NULL);

	memset(state, 0, sizeof(*state));
	free(state);
}

/**
 * 出力設定
 *
 *	@param	states
 *	@param	kanji_code	IdSJIS / IdEUC など
 *	@param	KanjiIn		IdKanjiInA / IdKanjiInB
 *	@param	KanjiOut	IdKanjiOutB / IdKanjiOutJ / IdKanjiOutH
 *	@param	jis7katakana
 */
void MakeOutputStringInit(
	OutputCharState *state,
	WORD kanji_code,
	WORD KanjiIn,
	WORD KanjiOut,
	BOOL jis7katakana)
{
	assert(state != NULL);

	state->KanjiCode = (IdKanjiCode)kanji_code;
	state->KanjiIn = KanjiIn;
	state->KanjiOut = KanjiOut;
	state->JIS7Katakana = jis7katakana;
	//
	state->SendCode = IdASCII;
//	state->ControlOut = OutControl;
}

/**
 * 出力用文字列を作成する
 *
 *	Unicode(UTF-16)文字列からUnicode(UTF-32)を1文字取り出して
 *	出力文字(TempStr)を作成する
 *
 *	@param	states
 *	@param	B			入力文字列(wchar_t)
 *	@param	C			入力文字列長
 *	@param	TempStr		出力文字ptr
 *	@param	TempLen_	出力文字長
 *	@param	ControlOut	文字検査/出力関数
 *	@param	data		ControlOut へ渡すデータ
 *	@retval	入力文字列から使用した文字数
 */
size_t MakeOutputString(
	OutputCharState *states,
	const wchar_t *B, size_t C,
	char *TempStr, size_t *TempLen_,
	BOOL (*ControlOut)(unsigned int u32, BOOL check_only, char *TempStr, size_t *StrLen, void *data),
	void *data)
{
	size_t TempLen = 0;
	size_t TempLen2;
	size_t output_char_count;	// 消費した文字数

	assert(states != NULL);

	// UTF-32 を1文字取り出す
	unsigned int u32;
	size_t u16_len = UTF16ToUTF32(B, C, &u32);
	if (u16_len == 0) {
		// デコードできない? あり得ないのでは?
		assert(FALSE);
		u32 = '?';
		u16_len = 1;
	}
	output_char_count = u16_len;

	// 各種シフト状態を通常に戻す
	if (u32 < 0x100 || (ControlOut != NULL && ControlOut(u32, TRUE, NULL, NULL, data))) {
		if (states->KanjiCode == IdJIS) {
			// 今のところ、日本語,JISしかない
			if (states->SendCode == IdKanji) {
				// 漢字ではないので、漢字OUT
				TempStr[TempLen++] = 0x1B;
				TempStr[TempLen++] = '(';
				switch (states->KanjiOut) {
				case IdKanjiOutJ:
					TempStr[TempLen++] = 'J';
					break;
				case IdKanjiOutH:
					TempStr[TempLen++] = 'H';
					break;
				default:
					TempStr[TempLen++] = 'B';
				}
			}

			if (states->JIS7Katakana == 1) {
				if (states->SendCode == IdKatakana) {
					TempStr[TempLen++] = SO;
				}
			}

			states->SendCode = IdASCII;
		}
	}

	// 1文字処理する
	if (ControlOut != NULL && ControlOut(u32, FALSE, &TempStr[TempLen], &TempLen2, data)) {
		// 特別な文字を処理した
		TempLen += TempLen2;
		output_char_count = 1;
	}
	else {
		switch (states->KanjiCode) {
		case IdUTF8: {
			// UTF-8 で出力
			size_t utf8_len = sizeof(TempStr);
			utf8_len = UTF32ToUTF8(u32, TempStr, utf8_len);
			TempLen += utf8_len;
			break;
		}
		case IdEUC:
		case IdJIS:
		case IdSJIS: {
			// 日本語
			// まず CP932(SJIS) に変換してから出力
			char mb_char[2];
			size_t mb_len = sizeof(mb_char);
			mb_len = UTF32ToMBCP(u32, 932, mb_char, mb_len);
			if (mb_len == 0) {
				// SJISに変換できない
				TempStr[TempLen++] = '?';
			} else {
				switch (states->KanjiCode) {
				case IdEUC:
					// TODO 半角カナ
					if (mb_len == 1) {
						TempStr[TempLen++] = mb_char[0];
					} else {
						WORD K;
						K = (((WORD)(unsigned char)mb_char[0]) << 8) +
							(WORD)(unsigned char)mb_char[1];
						K = CodeConvSJIS2EUC(K);
						TempStr[TempLen++] = HIBYTE(K);
						TempStr[TempLen++] = LOBYTE(K);
					}
					break;
				case IdJIS:
					if (u32 < 0x100) {
						// ASCII
						TempStr[TempLen++] = mb_char[0];
						states->SendCode = IdASCII;
					} else if (IsHalfWidthKatakana(u32)) {
						// 半角カタカナ
						if (states->JIS7Katakana==1) {
							if (states->SendCode != IdKatakana) {
								TempStr[TempLen++] = SI;
							}
							TempStr[TempLen++] = mb_char[0] & 0x7f;
						} else {
							TempStr[TempLen++] = mb_char[0];
						}
						states->SendCode = IdKatakana;
					} else {
						// 漢字
						WORD K;
						K = (((WORD)(unsigned char)mb_char[0]) << 8) +
							(WORD)(unsigned char)mb_char[1];
						K = CodeConvSJIS2JIS(K);
						if (states->SendCode != IdKanji) {
							// 漢字IN
							TempStr[TempLen++] = 0x1B;
							TempStr[TempLen++] = '$';
							if (states->KanjiIn == IdKanjiInB) {
								TempStr[TempLen++] = 'B';
							}
							else {
								TempStr[TempLen++] = '@';
							}
							states->SendCode = IdKanji;
						}
						TempStr[TempLen++] = HIBYTE(K);
						TempStr[TempLen++] = LOBYTE(K);
					}
					break;
				case IdSJIS:
					if (mb_len == 1) {
						TempStr[TempLen++] = mb_char[0];
					} else {
						TempStr[TempLen++] = mb_char[0];
						TempStr[TempLen++] = mb_char[1];
					}
					break;
				default:
					assert(FALSE);
					break;
				}
			}
			break;
		}
		case IdKoreanCP949:
		case IdCnGB2312:
		case IdCnBig5: {
			int code_page;
			char mb_char[2];
			size_t mb_len;
			switch (states->KanjiCode) {
			case IdKoreanCP949:
				code_page = 949;
				break;
			case IdCnGB2312:
				code_page = 936;
				break;
			case IdCnBig5:
				code_page = 950;
				break;
			default:
				assert(FALSE);
				code_page = 0;
				break;
			}
			/* code_page に変換して出力 */
			mb_len = sizeof(mb_char);
			mb_len = UTF32ToMBCP(u32, code_page, mb_char, mb_len);
			if (mb_len == 0) {
				TempStr[TempLen++] = '?';
			}
			else if (mb_len == 1) {
				TempStr[TempLen++] = mb_char[0];
			} else  {
				TempStr[TempLen++] = mb_char[0];
				TempStr[TempLen++] = mb_char[1];
			}
			break;
		}
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
		case IdISO8859_16: {
			// SBCS
			unsigned char byte;
			int r = UnicodeToISO8859(states->KanjiCode, u32, &byte);
			if (r == 0) {
				// 変換できない文字コードだった
				byte = '?';
			}
			TempStr[TempLen++] = byte;
			break;
		}
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
		case IdKOI8_NEW: {
			unsigned char byte;
			int r = UnicodeToCodePage(states->KanjiCode, u32, &byte);
			if (r == 0) {
				// 変換できない文字コードだった
				byte = '?';
			}
			TempStr[TempLen++] = byte;
			break;
		}
		case IdDebug:
			break;
#if !defined(__MINGW32__)
		default:
			// gcc/clangではswitchにenumのメンバがすべてないとき警告が出る
			assert(FALSE);
			break;
#endif
		}
	}

	*TempLen_ = TempLen;
	return output_char_count;
}

char *MakeOutputStringConvW(
	wchar_t const *strW,
	WORD kanji_code,
	WORD KanjiIn,
	WORD KanjiOut,
	BOOL jis7katakana,
	size_t *len)
{
	OutputCharState *h = MakeOutputStringCreate();
	MakeOutputStringInit(h, kanji_code, KanjiIn, KanjiOut, jis7katakana);

	size_t strW_len = wcslen(strW);
	size_t str_len = strW_len;
	char *str = (char *)malloc(str_len);
	size_t str_pos = 0;

	size_t i = 0;
	while (i < strW_len) {
		char tmp_str[8];
		size_t tmp_len;
		size_t output_char_count = MakeOutputString(h, &strW[i], strW_len - i, tmp_str, &tmp_len, NULL, NULL);
		if (str_pos + tmp_len + 1 > str_len) {
			str_len += 32;
			char *p = (char *)realloc(str, str_len);
			if (p == NULL) {
				break;
			}
			str = p;
		}
		assert(str_pos + tmp_len <= str_len);
		memcpy(&str[str_pos], &tmp_str, tmp_len);
		str_pos += tmp_len;
		i += output_char_count;
	}

	MakeOutputStringDestroy(h);

	assert(str_pos + 1 <= str_len);
	str[str_pos++] = 0;
	if (len != NULL) {
		*len = str_pos - 1;
	}

	return str;
}

char *MakeOutputStringConvU8(
	const char *strU8,
	WORD kanji_code,
	WORD KanjiIn,
	WORD KanjiOut,
	BOOL jis7katakana,
	size_t *len)
{
	wchar_t *strW = ToWcharU8(strU8);
	char *str = MakeOutputStringConvW(strW, kanji_code, KanjiIn, KanjiOut, jis7katakana, len);
	free(strW);
	return str;
}
