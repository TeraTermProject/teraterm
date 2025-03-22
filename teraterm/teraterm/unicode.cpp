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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "unicode.h"

/**
 *	East_Asian_Width �Q�l���� �擾
 *
 *	@retval	'F'		Fullwidth �S�p
 *	@retval	'H'		Halfwidth ���p
 *	@retval	'W'		Wide �L
 *	@retval	'n'		Na,Narrow ��
 *	@retval	'A'		Ambiguous �B��
 *					�����ɂ���ĕ��������قȂ镶���B
 *					���A�W�A�̑g�łƂ���ȊO�̑g�ł̗����ɏo�����A
 *					���A�W�A�̏]�������R�[�h�ł͂�����S�p�Ƃ��Ĉ����邱�Ƃ�����B
 *					�M���V�A������L���������ȂǁB
 *	@retval	'N'		Neutral ����
 *					���A�W�A�̑g�łɂ͒ʏ�o�������A�S�p�ł����p�ł��Ȃ��B�A���r�A�����ȂǁB
 */
char UnicodeGetWidthProperty(unsigned long u32)
{
	typedef struct {
		unsigned long code_from;
		unsigned long code_to;
		char property;
	} east_asian_width_map_t;
	// �e�[�u���ɓ����Ă��Ȃ��ꍇ�� H
	const static east_asian_width_map_t east_asian_width_map[] = {
#include "unicode_asian_width.tbl"
	};
	const east_asian_width_map_t *table = east_asian_width_map;
	const size_t table_size = _countof(east_asian_width_map);
	char result;

	// �e�[�u���O�`�F�b�N
	if (u32 < east_asian_width_map[0].code_from) {
		return 'H';
	}
	if (east_asian_width_map[table_size-1].code_to < u32) {
		return 'H';
	}

	// �e�[�u������
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
 * u32���e�[�u���̃f�[�^�Ɋ܂܂�Ă��邩���ׂ�
 *
 *	@retval		�e�[�u����index
 *	@retval		-1 �e�[�u���ɑ��݂��Ȃ�
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
	// �e�[�u���͈̔͊O
	return -1;
}

/**
 *	SearchTableSimple() �Ɠ���
 *	�e�[�u���̌^���قȂ�
 *
 *	@retval		�e�[�u����index
 *	@retval		-1 �e�[�u���ɑ��݂��Ȃ�
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
	// �e�[�u���͈̔͊O
	return -1;
}

/**
 *	SearchTableSimple() �Ɠ���
 *	�e�[�u���̌^���قȂ�
 *
 *	@retval		�e�[�u����index
 *	@retval		-1 �e�[�u���ɑ��݂��Ȃ�
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
	// �e�[�u���͈̔͊O
	return -1;
}

/*
 * ������������������
 *		���̕��������������Ƃ��Ĉ���
 *			EMOJI MODIFIER
 *				= Nonspacing Mark
 *			VARIATION SELECTOR (�ّ̎��Z���N�^)
 *				= Nonspacing Mark
 *
 *	@retval	0		���������ł͂Ȃ�
 *	@retval	1		��������,Nonspacing Mark, �J�[�\���͈ړ����Ȃ�
 *	@retval	2		��������,Spacing Mark, �J�[�\���� +1 �ړ�����
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
 *	�G����?
 *
 *	@retval	0	�G�����ł͂Ȃ�
 *	@retval	1	�G�����ł���
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
 *	�ّ̎��Z���N�^���`�F�b�N����
 *
 *	UnicodeIsCombiningCharacter() �œ����Ƀ`�F�b�N�ł���̂Ŏg�p���Ȃ��Ȃ���
 *
 *	@retval	0		�ّ̎��Z���N�^�ł͂Ȃ�
 *	@retval	1		�ّ̎��Z���N�^�ł���
 */
#if 0
int UnicodeIsVariationSelector(unsigned long u32)
{
	if ((0x00180b <= u32 && u32 <= 0x00180d) ||	// FVS (Mongolian Free Variation Selector)
		(0x00fe00 <= u32 && u32 <= 0x00fe0f) ||	// SVS VS1�`VS16
		(0x0e0100 <= u32 && u32 <= 0x0e01ef))	// IVS VS17�`VS256
	{
		return 1;
	}
	return 0;
}
#endif

/**
 *	���B���[�}?
 *
 *	@retval	0	���B���[�}�ł͂Ȃ�
 *	@retval	1	���B���[�}�ł���
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
 *	Unicode block �� index �𓾂�
 *
 *	@retval	-1	block ��������Ȃ�
 *	@retval		block �� index
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
		if (table[i].first_code != first_code) { // 1�����ڂ��قȂ�Ȃ�A�ȍ~�͂������ׂȂ��Ă悢�B
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
 *	Unicode�̌����������s��
 *	@param[in]	first_code
 *	@param[in]	code
 *	@retval		0		�����ł��Ȃ�
 *	@retval		�ȊO	��������Unicode
 *
 *		��
 *			first_code
 *				U+307B(��)
 *			code
 *				U+309A(�K)
 *			retval
 *				U+307D(��)
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
		// �����s�v
		*u16 = table_ptr[b].unicode;
		return 1;
	}
	// �e�[�u��������
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
		// �ϊ���ɑ��݂��Ȃ��R�[�h
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
 *	SBCS�����R�[�h����Unicode�֕ϊ�
 *
 *	@param[in]	kanji_code	SBCS�̕����R�[�henum
 *	@param[in]	b			SBCS�����R�[�h
 *	@param[out]	u16			�ϊ�����Unicode
 *	@retval		0			�ϊ��ł��Ȃ�
 *	@retval		1			�ϊ��ł���
 */
int UnicodeFromSBCS(IdKanjiCode kanji_code, unsigned char b, unsigned short *u16)
{
	if (kanji_code == IdISO8859_1) {
		// ISO8859-1 �� unicode �Ɠ���
		*u16 = b;
		return 1;
	}
	int table_size;
	const SBCSTable_t *table_ptr = GetSBCSTable(kanji_code, &table_size);
	return UnicodeFromSBCSTable(table_ptr, table_size, b, u16);
}

/**
 *	Unicode����SBCS�����R�[�h�֕ϊ�
 *
 *	@param[in]	kanji_code	SBCS�̕����R�[�henum
 *	@param[in]	u32			Unicode
 *	@param[out]	*b			�ϊ�����SBCS�̕����R�[�h
 *							�ϊ��ł��Ȃ��������� 0
 *	@retval		0			�ϊ��ł��Ȃ�
 *	@retval		1			�ϊ��ł���
 */
int UnicodeToSBCS(IdKanjiCode kanji_code, unsigned long u32, unsigned char *b)
{
	if (kanji_code == IdISO8859_1) {
		// ISO8859-1 �� unicode �Ɠ���
		*b = (unsigned char)u32;
		return 1;
	}
	if (u32 >= 0x10000) {
		// �ϊ���ɑ��݂��Ȃ��R�[�h
		*b = 0;
		return 0;
	}
	int table_size = 0;
	const SBCSTable_t *table_ptr = GetSBCSTable(kanji_code, &table_size);
	return UnicodeToSBCTable(table_ptr, table_size, u32, b);
}

/**
 *	ISO8859����Unicode�֕ϊ�
 */
int UnicodeFromISO8859(IdKanjiCode part, unsigned char b, unsigned short *u16)
{
	return UnicodeFromSBCS(part, b, u16);
}

/**
 *	Unicode����ISO8859�֕ϊ�
 */
int UnicodeToISO8859(IdKanjiCode part, unsigned long u32, unsigned char *b)
{
	return UnicodeToSBCS(part, u32, b);
}

/**
 *	CodePage����Unicode�֕ϊ�
 */
int UnicodeFromCodePage(IdKanjiCode kanji_code, unsigned char b, unsigned short *u16)
{
	return UnicodeFromSBCS(kanji_code, b, u16);
}

/**
 *	Unicode����CodePage�֕ϊ�
 *
 */
int UnicodeToCodePage(IdKanjiCode kanji_code, unsigned long u32, unsigned char *b)
{
	return UnicodeToSBCS(kanji_code, u32, b);
}
