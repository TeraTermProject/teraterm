/*
 * Copyright (C) 2019- TeraTerm Project
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
#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <assert.h>
#include <wchar.h>

#include "unicode.h"

#include "win32helper.h"
#include "asprintf.h"
#include "inifile_com.h"	// GetPrivateProfileOnOffW()
#include "ttlib.h"			// IsRelativePathW()

/**
 *	East_Asian_Width 参考特性 取得
 *
 *	@retval	'F'		Fullwidth 全角
 *	@retval	'H'		Halfwidth 半角
 *	@retval	'W'		Wide 広
 *	@retval	'n'		Na,Narrow 狭
 *	@retval	'A'		Ambiguous 曖昧
 *					文脈によって文字幅が異なる文字。
 *					東アジアの組版とそれ以外の組版の両方に出現し、
 *					東アジアの従来文字コードではいわゆる全角として扱われることがある。
 *					ギリシア文字やキリル文字など。
 *	@retval	'N'		Neutral 中立
 *					東アジアの組版には通常出現せず、全角でも半角でもない。アラビア文字など。
 */
char UnicodeGetWidthProperty(unsigned long u32)
{
	typedef struct {
		unsigned long code_from;
		unsigned long code_to;
		char property;
	} east_asian_width_map_t;
	// テーブルに入っていない場合は H
	const static east_asian_width_map_t east_asian_width_map[] = {
#include "unicode_asian_width.tbl"
	};
	const east_asian_width_map_t *table = east_asian_width_map;
	const size_t table_size = _countof(east_asian_width_map);
	char result;

	// テーブル外チェック
	if (u32 < east_asian_width_map[0].code_from) {
		return 'H';
	}
	if (east_asian_width_map[table_size-1].code_to < u32) {
		return 'H';
	}

	// テーブル検索
	result = 'H';
	size_t low = 0;
	size_t high = table_size - 1;
	while (low < high) {
		size_t mid = (low + high) / 2;
		if (table[mid].code_from <= u32 && u32 <= table[mid].code_to) {
			result = table[mid].property;
			break;
		} else if (table[mid].code_to < u32) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}

	return result;
}

typedef struct {
	unsigned long code_from;
	unsigned long code_to;
} UnicodeTable_t;

typedef struct {
	unsigned long code_from;
	unsigned long code_to;
	unsigned char category;
} UnicodeTableCombine_t;

typedef struct {
	unsigned long code_from;
	unsigned long code_to;
	const char *block_name;
} UnicodeTableBlock_t;

static const UnicodeTableBlock_t UnicodeBlockList[] = {
#include "unicode_block.tbl"
};

/**
 * u32がテーブルのデータに含まれているか調べる
 *
 *	@retval		テーブルのindex
 *	@retval		-1 テーブルに存在しない
 */
static int SearchTableSimple(
	const UnicodeTable_t *table, size_t table_size,
	unsigned long u32)
{
	if (u32 < table[0].code_from) {
		return -1;
	}
	if (u32 > table[table_size-1].code_to) {
		return -1;
	}
	size_t low = 0;
	size_t high = table_size - 1;
	while (low <= high) {
		size_t mid = (low + high) / 2;
		if (table[mid].code_from <= u32 && u32 <= table[mid].code_to) {
			return (int)mid;
		} else if (table[mid].code_to < u32) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}
	// テーブルの範囲外
	return -1;
}

/**
 *	SearchTableSimple() と同じ
 *	テーブルの型が異なる
 *
 *	@retval		テーブルのindex
 *	@retval		-1 テーブルに存在しない
 */
static int SearchTableCombine(
	const UnicodeTableCombine_t *table, size_t table_size,
	unsigned long u32)
{
	if (u32 < table[0].code_from) {
		return -1;
	}
	if (u32 > table[table_size-1].code_to) {
		return -1;
	}
	size_t low = 0;
	size_t high = table_size - 1;
	while (low <= high) {
		size_t mid = (low + high) / 2;
		if (table[mid].code_from <= u32 && u32 <= table[mid].code_to) {
			return (int)mid;
		} else if (table[mid].code_to < u32) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}
	// テーブルの範囲外
	return -1;
}

/**
 *	SearchTableSimple() と同じ
 *	テーブルの型が異なる
 *
 *	@retval		テーブルのindex
 *	@retval		-1 テーブルに存在しない
 */
static int SearchTableBlock(
	const UnicodeTableBlock_t *table, size_t table_size,
	unsigned long u32)
{
	if (u32 < table[0].code_from) {
		return -1;
	}
	if (u32 > table[table_size-1].code_to) {
		return -1;
	}
	size_t low = 0;
	size_t high = table_size - 1;
	while (low <= high) {
		size_t mid = (low + high) / 2;
		if (table[mid].code_from <= u32 && u32 <= table[mid].code_to) {
			return (int)mid;
		} else if (table[mid].code_to < u32) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}
	// テーブルの範囲外
	return -1;
}

/*
 * 結合文字か検査する
 *		次の文字も結合文字として扱う
 *			EMOJI MODIFIER
 *				= Nonspacing Mark
 *			VARIATION SELECTOR (異体字セレクタ)
 *				= Nonspacing Mark
 *
 *	@retval	0		結合文字ではない
 *	@retval	1		結合文字,Nonspacing Mark, カーソルは移動しない
 *	@retval	2		結合文字,Spacing Mark, カーソルが +1 移動する
 */
int UnicodeIsCombiningCharacter(unsigned long u32)
{
#define Mn 1  // Nonspacing_Mark	a nonspacing combining mark (zero advance width)
#define Mc 2  // Spacing_Mark		a spacing combining mark (positive advance width)
#define Me 1  // Enclosing_Mark		an enclosing combining mark
#define Sk 1  // Modifier_Symbol	a non-letterlike modifier symbol
	const static UnicodeTableCombine_t CombiningCharacterList[] = {
#include "unicode_combine.tbl"
	};
	const int index = SearchTableCombine(CombiningCharacterList, _countof(CombiningCharacterList), u32);
	if (index == -1) {
		return 0;
	}
	return (int)CombiningCharacterList[index].category;
}

/**
 *	絵文字?
 *
 *	@retval	0	絵文字ではない
 *	@retval	1	絵文字である
 */
int UnicodeIsEmoji(unsigned long u32)
{
	const static UnicodeTable_t EmojiList[] = {
#include "unicode_emoji.tbl"
	};
	const int index = SearchTableSimple(EmojiList, _countof(EmojiList), u32);
	return index != -1 ? 1 : 0;
}

/**
 *	異体字セレクタかチェックする
 *
 *	UnicodeIsCombiningCharacter() で同時にチェックできるので使用しなくなった
 *
 *	@retval	0		異体字セレクタではない
 *	@retval	1		異体字セレクタである
 */
#if 0
int UnicodeIsVariationSelector(unsigned long u32)
{
	if ((0x00180b <= u32 && u32 <= 0x00180d) ||	// FVS (Mongolian Free Variation Selector)
		(0x00fe00 <= u32 && u32 <= 0x00fe0f) ||	// SVS VS1〜VS16
		(0x0e0100 <= u32 && u32 <= 0x0e01ef))	// IVS VS17〜VS256
	{
		return 1;
	}
	return 0;
}
#endif

/**
 *	ヴィラーマ?
 *
 *	@retval	0	ヴィラーマではない
 *	@retval	1	ヴィラーマである
 */
int UnicodeIsVirama(unsigned long u32)
{
	const static UnicodeTable_t ViramaList[] = {
#include "unicode_virama.tbl"
	};
	const int index = SearchTableSimple(ViramaList, _countof(ViramaList), u32);
	return index != -1 ? 1 : 0;
}

/**
 *	Unicode block の index を得る
 *
 *	@retval	-1	block が見つからない
 *	@retval		block の index
 */
int UnicodeBlockIndex(unsigned long u32)
{
	return SearchTableBlock(UnicodeBlockList, _countof(UnicodeBlockList), u32);
}

const char *UnicodeBlockName(int index)
{
	if (index == -1) {
		return "";
	}
	return UnicodeBlockList[index].block_name;
}

#if 0
int main(int, char *[])
{
	static const unsigned long codes[] = {
#if 0
		0, 1, 0x7f,
		0x80,
		0x0e00ff,
		0x0e0100,
		0x10fffd,
#endif
		0x10fffe,
	};

	for (size_t i = 0; i < _countof(codes); i++) {
		unsigned long code = codes[i];
		printf("U+%06lx %c\n", code, UnicodeGetWidthProperty(code));
	}
	return 0;
}
#endif

//
// Unicode Combining Character Support
//
#include "uni_combining.map"

static unsigned short UnicodeGetPrecomposedChar(int start_index, unsigned short first_code, unsigned short code)
{
	const combining_map_t *table = mapCombiningToPrecomposed;
	int tmax = _countof(mapCombiningToPrecomposed);
	unsigned short result = 0;
	int i;

	for (i = start_index ; i < tmax ; i++) {
		if (table[i].first_code != first_code) { // 1文字目が異なるなら、以降はもう調べなくてよい。
			break;
		}

		if (table[i].second_code == code) {
			result = table[i].precomposed;
			break;
		}
	}

	return (result);
}

static int UnicodeGetIndexOfCombiningFirstCode(unsigned short code)
{
	const combining_map_t *table = mapCombiningToPrecomposed;
	int tmax = _countof(mapCombiningToPrecomposed);
	int low, mid, high;
	int index = -1;

	low = 0;
	high = tmax - 1;

	// binary search
	while (low < high) {
		mid = (low + high) / 2;
		if (table[mid].first_code < code) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}

	if (table[low].first_code == code) {
		while (low >= 0 && table[low].first_code == code) {
			index = low;
			low--;
		}
	}

	return (index);
}

/**
 *	Unicodeの結合処理を行う
 *	@param[in]	first_code
 *	@param[in]	code
 *	@retval		0		結合できない
 *	@retval		以外	結合したUnicode
 *
 *		例
 *			first_code
 *				U+307B(ほ)
 *			code
 *				U+309A(゜)
 *			retval
 *				U+307D(ぽ)
 */
unsigned short UnicodeCombining(unsigned short first_code, unsigned short code)
{
	int first_code_index = UnicodeGetIndexOfCombiningFirstCode(first_code);
	if (first_code_index == -1) {
		return 0;
	}
	unsigned short cset = UnicodeGetPrecomposedChar(first_code_index, first_code, code);
	return cset;
}

typedef struct {
	unsigned char code;			// 0x00...0xff SBCS
	unsigned short unicode;		// Unicode (...U+ffff)
} SBCSTable_t;

const SBCSTable_t *GetSBCSTable(IdKanjiCode kanji_code, int *table_size)
{
	static const SBCSTable_t iso8859_2[] = {
#include "mapping/iso8859-2.map"
	};
	static const SBCSTable_t iso8859_3[] = {
#include "mapping/iso8859-3.map"
	};
	static const SBCSTable_t iso8859_4[] = {
#include "mapping/iso8859-4.map"
	};
	static const SBCSTable_t iso8859_5[] = {
#include "mapping/iso8859-5.map"
	};
	static const SBCSTable_t iso8859_6[] = {
#include "mapping/iso8859-6.map"
	};
	static const SBCSTable_t iso8859_7[] = {
#include "mapping/iso8859-7.map"
	};
	static const SBCSTable_t iso8859_8[] = {
#include "mapping/iso8859-8.map"
	};
	static const SBCSTable_t iso8859_9[] = {
#include "mapping/iso8859-9.map"
	};
	static const SBCSTable_t iso8859_10[] = {
#include "mapping/iso8859-10.map"
	};
	static const SBCSTable_t iso8859_11[] = {
#include "mapping/iso8859-11.map"
	};
	static const SBCSTable_t iso8859_13[] = {
#include "mapping/iso8859-13.map"
	};
	static const SBCSTable_t iso8859_14[] = {
#include "mapping/iso8859-14.map"
	};
	static const SBCSTable_t iso8859_15[] = {
#include "mapping/iso8859-15.map"
	};
	static const SBCSTable_t iso8859_16[] = {
#include "mapping/iso8859-16.map"
	};
	static const SBCSTable_t cp1250[] = {
#include "mapping/cp1250.map"
	};
	static const SBCSTable_t cp1251[] = {
#include "mapping/cp1251.map"
	};
	static const SBCSTable_t cp1252[] = {
#include "mapping/cp1252.map"
	};
	static const SBCSTable_t cp1253[] = {
#include "mapping/cp1253.map"
	};
	static const SBCSTable_t cp1254[] = {
#include "mapping/cp1254.map"
	};
	static const SBCSTable_t cp1255[] = {
#include "mapping/cp1255.map"
	};
	static const SBCSTable_t cp1256[] = {
#include "mapping/cp1256.map"
	};
	static const SBCSTable_t cp1257[] = {
#include "mapping/cp1257.map"
	};
	static const SBCSTable_t cp1258[] = {
#include "mapping/cp1258.map"
	};
	static const SBCSTable_t cp437[] = {
#include "mapping/cp437.map"
	};
	static const SBCSTable_t cp737[] = {
#include "mapping/cp737.map"
	};
	static const SBCSTable_t cp775[] = {
#include "mapping/cp775.map"
	};
	static const SBCSTable_t cp850[] = {
#include "mapping/cp850.map"
	};
	static const SBCSTable_t cp852[] = {
#include "mapping/cp852.map"
	};
	static const SBCSTable_t cp855[] = {
#include "mapping/cp855.map"
	};
	static const SBCSTable_t cp857[] = {
#include "mapping/cp857.map"
	};
	static const SBCSTable_t cp860[] = {
#include "mapping/cp860.map"
	};
	static const SBCSTable_t cp861[] = {
#include "mapping/cp861.map"
	};
	static const SBCSTable_t cp862[] = {
#include "mapping/cp862.map"
	};
	static const SBCSTable_t cp863[] = {
#include "mapping/cp863.map"
	};
	static const SBCSTable_t cp864[] = {
#include "mapping/cp864.map"
	};
	static const SBCSTable_t cp865[] = {
#include "mapping/cp865.map"
	};
	static const SBCSTable_t cp866[] = {
#include "mapping/cp866.map"
	};
	static const SBCSTable_t cp869[] = {
#include "mapping/cp869.map"
	};
	static const SBCSTable_t cp874[] = {
#include "mapping/cp874.map"
	};
	static const SBCSTable_t koi8r[] = {
#include "mapping/koi8-r.map"
	};

	static const struct {
		IdKanjiCode kanji_code;
		const SBCSTable_t *table;
		int size;
	} tables[] = {
		// https://www.unicode.org/L2/L1999/99325-E.htm
		// ISO 8859
		{ IdISO8859_1, NULL, 0 },	// ISO8859-1
		{ IdISO8859_2, iso8859_2, _countof(iso8859_2) },
		{ IdISO8859_3, iso8859_3, _countof(iso8859_3) },
		{ IdISO8859_4, iso8859_4, _countof(iso8859_4) },
		{ IdISO8859_5, iso8859_5, _countof(iso8859_5) },
		{ IdISO8859_6, iso8859_6, _countof(iso8859_6) },
		{ IdISO8859_7, iso8859_7, _countof(iso8859_7) },
		{ IdISO8859_8, iso8859_8, _countof(iso8859_8) },
		{ IdISO8859_9, iso8859_9, _countof(iso8859_9) },
		{ IdISO8859_10, iso8859_10, _countof(iso8859_10) },
		{ IdISO8859_11, iso8859_11, _countof(iso8859_11) },
		{ IdISO8859_13, iso8859_13, _countof(iso8859_13) },
		{ IdISO8859_14, iso8859_14, _countof(iso8859_14) },
		{ IdISO8859_15, iso8859_15, _countof(iso8859_15) },
		{ IdISO8859_16, iso8859_16, _countof(iso8859_16) },
		// DOS
		{ IdCP437, cp437, _countof(cp437) },
		{ IdCP737, cp737, _countof(cp737) },
		{ IdCP775, cp775, _countof(cp775) },
		{ IdCP850, cp850, _countof(cp850) },
		{ IdCP852, cp852, _countof(cp852) },
		{ IdCP855, cp855, _countof(cp855) },
		{ IdCP857, cp857, _countof(cp857) },
		{ IdCP860, cp860, _countof(cp860) },
		{ IdCP861, cp861, _countof(cp861) },
		{ IdCP862, cp862, _countof(cp862) },
		{ IdCP863, cp863, _countof(cp863) },
		{ IdCP864, cp864, _countof(cp864) },
		{ IdCP865, cp865, _countof(cp865) },
		{ IdCP866, cp866, _countof(cp866) },
		{ IdCP869, cp869, _countof(cp869) },
		// Windows
		{ IdCP874, cp874, _countof(cp874) },
		{ IdCP1250, cp1250, _countof(cp1250) },
		{ IdCP1251, cp1251, _countof(cp1251) },
		{ IdCP1252, cp1252, _countof(cp1252) },
		{ IdCP1253, cp1253, _countof(cp1253) },
		{ IdCP1254, cp1254, _countof(cp1254) },
		{ IdCP1255, cp1255, _countof(cp1255) },
		{ IdCP1256, cp1256, _countof(cp1256) },
		{ IdCP1257, cp1257, _countof(cp1257) },
		{ IdCP1258, cp1258, _countof(cp1258) },
		// Other ASCII-based
		{ IdKOI8_NEW, koi8r, _countof(koi8r) },
	};
	for (int i = 0; i < _countof(tables); i++) {
		if (kanji_code == tables[i].kanji_code) {
			if (table_size != NULL) {
				*table_size = tables[i].size;
			}
			return tables[i].table;
		}
	}
	assert(0);
	if (table_size != NULL) {
		*table_size = 0;
	}
	return NULL;
}

static int UnicodeFromSBCSTable(const SBCSTable_t *table_ptr, int table_size, unsigned char b,
								unsigned short *u16)
{
	if (table_ptr == NULL || table_size == 0) {
		*u16 = 0;
		return 0;
	}
	if (table_size == 0x100) {
		// 検索不要
		*u16 = table_ptr[b].unicode;
		return 1;
	}
	// テーブルを検索
	for (int i = 0; i < 0xff; i++ ){
		if (table_ptr[i].code == b) {
			*u16 = table_ptr[i].unicode;
			return 1;
		}
	}
	*u16 = 0;
	return 0;
}

static int UnicodeToSBCTable(const SBCSTable_t *table_ptr, int table_size, unsigned long u32,
							 unsigned char *b)
{
	if (u32 >= 0x10000) {
		// 変換先に存在しないコード
		*b = 0;
		return 0;
	}
	const unsigned short u16 = (unsigned short)u32;
	for (int i = 0; i < table_size; i++ ){
		if (table_ptr[i].unicode == u16) {
			*b = table_ptr[i].code;
			return 1;
		}
	}
	*b = 0;
	return 0;
}

/**
 *	SBCS文字コードからUnicodeへ変換
 *
 *	@param[in]	kanji_code	SBCSの文字コードenum
 *	@param[in]	b			SBCS文字コード
 *	@param[out]	u16			変換したUnicode
 *	@retval		0			変換できない
 *	@retval		1			変換できた
 */
int UnicodeFromSBCS(IdKanjiCode kanji_code, unsigned char b, unsigned short *u16)
{
	if (kanji_code == IdISO8859_1) {
		// ISO8859-1 は unicode と同一
		*u16 = b;
		return 1;
	}
	int table_size;
	const SBCSTable_t *table_ptr = GetSBCSTable(kanji_code, &table_size);
	return UnicodeFromSBCSTable(table_ptr, table_size, b, u16);
}

/**
 *	UnicodeからSBCS文字コードへ変換
 *
 *	@param[in]	kanji_code	SBCSの文字コードenum
 *	@param[in]	u32			Unicode
 *	@param[out]	*b			変換したSBCSの文字コード
 *							変換できなかった時は 0
 *	@retval		0			変換できない
 *	@retval		1			変換できた
 */
int UnicodeToSBCS(IdKanjiCode kanji_code, unsigned long u32, unsigned char *b)
{
	if (kanji_code == IdISO8859_1) {
		// ISO8859-1 は unicode と同一
		*b = (unsigned char)u32;
		return 1;
	}
	if (u32 >= 0x10000) {
		// 変換先に存在しないコード
		*b = 0;
		return 0;
	}
	int table_size = 0;
	const SBCSTable_t *table_ptr = GetSBCSTable(kanji_code, &table_size);
	return UnicodeToSBCTable(table_ptr, table_size, u32, b);
}

/**
 *	ISO8859からUnicodeへ変換
 */
int UnicodeFromISO8859(IdKanjiCode part, unsigned char b, unsigned short *u16)
{
	return UnicodeFromSBCS(part, b, u16);
}

/**
 *	UnicodeからISO8859へ変換
 */
int UnicodeToISO8859(IdKanjiCode part, unsigned long u32, unsigned char *b)
{
	return UnicodeToSBCS(part, u32, b);
}

/**
 *	CodePageからUnicodeへ変換
 */
int UnicodeFromCodePage(IdKanjiCode kanji_code, unsigned char b, unsigned short *u16)
{
	return UnicodeFromSBCS(kanji_code, b, u16);
}

/**
 *	UnicodeからCodePageへ変換
 *
 */
int UnicodeToCodePage(IdKanjiCode kanji_code, unsigned long u32, unsigned char *b)
{
	return UnicodeToSBCS(kanji_code, u32, b);
}

typedef struct {
	unsigned int start;
	unsigned int end;
	int width;
} UnicodeWidthList_t;

static UnicodeWidthList_t *unicode_width_list_ptr;
static size_t unicode_width_list_count = 0;

int UnicodeOverrideWidthInit(const wchar_t *ini, const wchar_t *section)
{
	UnicodeOverrideWidthUninit();

	UnicodeWidthList_t *p = NULL;
	size_t c = 0;

	for (size_t i = 1;; i++) {
		wchar_t *key;
		aswprintf(&key, L"Range%d", (int)i);
		wchar_t *value;
		hGetPrivateProfileStringW(section, key, L"", ini, &value);
		free(key);
		if (value[0] == 0) {
			free(value);
			break;
		}
		unsigned int start;
		unsigned int end;
		int width;
		int r = 0;
		r = swscanf(value, L"U+%x , U+%x , %d", &start, &end, &width);
		if (r == 3) {
			if (width == 1 || width == 2) {
				UnicodeWidthList_t *p2 = (UnicodeWidthList_t *)realloc(p, sizeof(UnicodeWidthList_t) * (c + 1));
				if (p2 != NULL) {
					UnicodeWidthList_t item;
					item.start = start;
					item.end = end;
					item.width = width;

					p = p2;
					p[c] = item;
					c++;
				}
			}
			free(value);
			continue;
		}
		r = swscanf(value, L"U+%x , %d", &start, &width);
		if (r == 2) {
			if (width == 1 || width == 2) {
				UnicodeWidthList_t *p2 = (UnicodeWidthList_t *)realloc(p, sizeof(UnicodeWidthList_t) * (c + 1));
				if (p2 != NULL) {
					UnicodeWidthList_t item;
					item.start = start;
					item.end = start;
					item.width = width;

					p = p2;
					p[c] = item;
					c++;
				}
			}
			free(value);
			continue;
		}
	}

	unicode_width_list_ptr = p;
	unicode_width_list_count = c;

	return 1;
}

void UnicodeOverrideWidthUninit(void)
{
	if (unicode_width_list_ptr != NULL) {
		free(unicode_width_list_ptr);
		unicode_width_list_ptr = NULL;
		unicode_width_list_count = 0;
	}
}

/**
 *	文字幅Overrideする?
 *
 *	@param		u32		コードポイント
 *	@param[out]	width	文字幅(セル数) 1 or 2
 *	@retval		TRUE	u32はOverrideする
 *	@retval		FALSE	u32はOverrideしない
 */
int UnicodeOverrideWidthCheck(unsigned int u32, int *width)
{
	if (unicode_width_list_count == 0) {
		*width = 0;
		return 0;
	}
	const UnicodeWidthList_t *p = unicode_width_list_ptr;
	for (size_t i = 0; i < unicode_width_list_count; p++,i++) {
		if (p->start <= u32 && u32 <= p->end) {
			*width = p->width;
			return 1;
		}
	}
	return 0;
}

int UnicodeOverrideWidthAvailable(void)
{
	return unicode_width_list_count == 0 ? 0 : 1;
}

/**
 * iniファイルから、文字幅情報の設定一覧を読み出す
 *
 * @param		fname	TERATERM.INI
 * @param[out]	info	情報一覧
 *						OverrideCharWidthInfoFree() を使って開放すること
 */
void OverrideCharWidthInfoGet(const wchar_t *fname, OverrideCharWidthInfo *info)
{
	const wchar_t *tt_section = L"UnicodeOverrideCharWidth";

	size_t count = 0;
	OverrideCharWidthInfoSet *ptr = NULL;

	int n = 0;
	for (;;) {
		wchar_t *key;
		aswprintf(&key, L"List%d", n + 1);
		wchar_t *list;
		hGetPrivateProfileStringW(tt_section, key, L"", fname, &list);
		free(key);
		if (list == NULL || list[0] == L'\0') {
			free(list);
			break;
		}
		size_t len = wcslen(list) + 1;
		wchar_t *ini = (wchar_t *)malloc(len * sizeof(wchar_t));
		wchar_t *section = (wchar_t *)malloc(len * sizeof(wchar_t));
		int r = swscanf_s(list, L"%[^,] , %s", ini, (unsigned int)len, section, (unsigned int)len);
		if (r != 2) {
			int r = swscanf_s(list, L"%s", section, (unsigned int)len);
			if (r == 1) {
				free(ini);
				ini = wcsdup(fname);
			} else {
				r = 0;
			}
		}
		free(list);
		if (r != 0) {
			OverrideCharWidthInfoSet *p = (OverrideCharWidthInfoSet *)
				realloc(ptr, sizeof(OverrideCharWidthInfoSet) * (count + 1));
			if (p != NULL) {
				ptr = p;
				p = &ptr[count];
				count += 1;
				if (IsRelativePathW(ini)) {
					p->file = NULL;
					wchar_t *path = _wcsdup(fname);
					wchar_t *sep = wcsrchr(path, L'\\') + 1;
					*sep = 0;
					awcscats(&p->file, path, ini, NULL);
					free(path);
					free(ini);
				} else {
					p->file = ini;
				}
				p->section = section;
				wchar_t *name;
				hGetPrivateProfileStringW(section, L"name", L"", p->file, &name);
				if (name != NULL && name[0] != L'\0') {
					p->name = name;
				} else {
					p->name = wcsdup(p->section);
					free(name);
				}
			}
		} else {
			// 行無効
			free(ini);
			free(section);
			continue;
		}
		n++;
	}

	info->count = count;
	info->sets = ptr;
	info->enable = FALSE;
	info->selected = 0;

	if (count != 0) {
		GetPrivateProfileOnOffW(tt_section, L"Enable", fname, FALSE, &info->enable);
		info->selected = GetPrivateProfileIntW(tt_section, L"Selected", 0, fname);
		if (info->selected >= info->count) {
			info->selected = 0;
		}
	}
}

/**
 * 文字幅情報の設定一覧を開放する
 */
void OverrideCharWidthInfoFree(OverrideCharWidthInfo *info)
{
	for (size_t i = 0; i< info->count; i++) {
		free(info->sets[i].file);
		info->sets[i].file = NULL;
		free(info->sets[i].section);
		info->sets[i].section = NULL;
		free(info->sets[i].name);
		info->sets[i].name = NULL;
	}
	free(info->sets);
	info->sets = NULL;
	info->count = 0;
}
