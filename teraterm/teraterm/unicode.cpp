/*
 * Copyright (C) 2019 TeraTerm Project
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

#pragma warning(push, 0)
#include <stdlib.h>
#include <stdio.h>
#pragma warning(pop)

#include "unicode.h"

/**
 *	East_Asian_Width 参考特性 取得
 *
 *	@retval	'F'		Fullwidth 全角
 *	@retval	'W'		Wide 広
 *	@retval	'A'		Ambiguous 曖昧
 *	@retval	'H'		半角扱い (H(Halfwidth 半角) or Na(Narrow 狭) or N(Neutral 中立))
 */
char UnicodeGetWidthProperty(unsigned long u32)
{
	typedef struct {
		unsigned long code_from;
		unsigned long code_to;
		char property;
	} east_asian_width_map_t;
	// W or F or A がテーブルに入っている (テーブル外は H)
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

#if 0
	if (result == 'A') {
		// キリル文字特別(TODO)
		// ?	0x500-0x520
		// 		0x2de0-0x2dff
		// 		0xa640-0xa69f
		if (0x400 <= u32 && u32 <= 0x4ff) {
			result = 'H';
		}
	}
#endif

	return result;
}

/*
 * 結合文字か検査する
 *		EMOJI MODIFIER も結合文字として扱う
 *
 *	@retval	0		結合文字ではない
 *	@retval	1		結合文字である
 */
int UnicodeIsCombiningCharacter(unsigned long u32)
{
	typedef struct {
		unsigned long code_from;
		unsigned long code_to;
	} CombiningCharacterList_t;
	const static CombiningCharacterList_t CombiningCharacterList[] = {
#include "unicode_combine.tbl"
	};
	const CombiningCharacterList_t *table = CombiningCharacterList;
	const size_t table_size = _countof(CombiningCharacterList);
	if (u32 < CombiningCharacterList[0].code_from) {
		return 0;
	}
	if (u32 > CombiningCharacterList[table_size-1].code_to) {
		return 0;
	}
	size_t low = 0;
	size_t high = table_size - 1;
	while (low <= high) {
		size_t mid = (low + high) / 2;
		if (table[mid].code_from <= u32 && u32 <= table[mid].code_to) {
			return 1;
		} else if (table[mid].code_to < u32) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}
	// テーブルの範囲外
	return 0;
}

/**
 *	異体字セレクタかチェックする
 *
 *	@retval	0		異体字セレクタではない
 *	@retval	1		異体字セレクタである
 */
int UnicodeIsVariationSelector(unsigned long u32)
{
	if ((0x00180b <= u32 && u32 <= 0x00180d) ||	// FVS (Mongolian Free Variation Selector)
		(0x00fe00 <= u32 && u32 <= 0x00fe0f) ||	// SVS VS1～VS16
		(0x0e0100 <= u32 && u32 <= 0x0e01ef))	// IVS VS17～VS256
	{
		return 1;
	}
	return 0;
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
