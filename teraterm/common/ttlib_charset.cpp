/*
 * (C) 2021- TeraTerm Project
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "codeconv.h"
#include "tttypes_charset.h"

#include "ttlib_charset.h"

static const TKanjiList KanjiList[] = {
	// coding			GUI							INI
	{ IdUTF8,			"UTF-8",					"UTF-8" },
	{ IdISO8859_1,		"ISO8859-1",				"ISO8859-1" },
	{ IdISO8859_2,		"ISO8859-2",				"ISO8859-2" },
	{ IdISO8859_3,		"ISO8859-3",				"ISO8859-3" },
	{ IdISO8859_4,		"ISO8859-4",				"ISO8859-4" },
	{ IdISO8859_5,		"ISO8859-5",				"ISO8859-5" },
	{ IdISO8859_6,		"ISO8859-6",				"ISO8859-6" },
	{ IdISO8859_7,		"ISO8859-7",				"ISO8859-7" },
	{ IdISO8859_8,		"ISO8859-8",				"ISO8859-8" },
	{ IdISO8859_9,		"ISO8859-9",				"ISO8859-9" },
	{ IdISO8859_10,		"ISO8859-10",				"ISO8859-10" },
	{ IdISO8859_11,		"ISO8859-11",				"ISO8859-11" },
	{ IdISO8859_13,		"ISO8859-13",				"ISO8859-13" },
	{ IdISO8859_14,		"ISO8859-14",				"ISO8859-14" },
	{ IdISO8859_15,		"ISO8859-15",				"ISO8859-15" },
	{ IdISO8859_16,		"ISO8859-16",				"ISO8859-16" },
	{ IdSJIS,			"SJIS (CP932)",				"SJIS" },
	{ IdEUC,			NULL,						"EUC-JP" },
	{ IdEUC,			"EUC",						"EUC" },
	{ IdJIS,			"JIS",						"JIS" },
	{ IdWindows,		"Windows-1251 (CP1251)",	"Windows-1251" },
	{ IdKOI8,			"KOI8-R",					"KOI8-R" },
	{ Id866,			"CP866",					"CP866" },
//	{ IdISO,			NULL,						"ISO8859-5" },
	{ IdISO8859_5,		NULL,						"ISO8859-5" },
	{ IdKoreanCP949,	"KS5601 (CP949)",			"KS5601" },
	{ IdCnGB2312,		"GB2312 (CP936)",			"GB2312" },
	{ IdCnBig5,			"Big5 (CP950)",				"BIG5" },
};

/**
 *	構造体の1要素を取得
 *	範囲外になったらNULLが返る
 */
const TKanjiList *GetKanjiList(int index)
{
	if (index < 0 || index >= _countof(KanjiList)) {
		return NULL;
	}
	return &KanjiList[index];
}

/**
 *	漢字コード文字列を取得
 *	iniファイルに保存する漢字コード文字列用
 *
 *	@param[in]	kanji_code (=ts.KanjiCode (receive) or ts.KanjiCodeSend)
 *						IdEUC, IdJIS, IdUTF8, ...
 *	@return	漢字コード文字列
 */
const char *GetKanjiCodeStr(int kanji_code)
{
	for (int i = 0; i < _countof(KanjiList); i++) {
		if (KanjiList[i].coding == kanji_code && KanjiList[i].CodeStrINI != NULL) {
			return KanjiList[i].CodeStrINI;
		}
	}
	return "UTF-8";
}

/**
 *	漢字コードを取得
 *	iniファイルに保存されている漢字コード文字列を漢字コードに変換する

 *	@param[in]	kanji_code_str
 *						漢字コード文字列
 *	@return	漢字コード
 */
int GetKanjiCodeFromStr(const char *kanji_code_str)
{
	if (kanji_code_str[0] == 0) {
		return IdUTF8;
	}

	for (int i = 0; i < _countof(KanjiList); i++) {
		if (KanjiList[i].CodeStrINI != NULL && strcmp(KanjiList[i].CodeStrINI, kanji_code_str) == 0) {
			return KanjiList[i].coding;
		}
	}
	return IdUTF8;
}

int GetKanjiCodeFromStrW(const wchar_t *kanji_code_strW)
{
	if (kanji_code_strW == NULL || kanji_code_strW[0] == 0) {
		return IdUTF8;
	}

	char *kanji_code_str = ToCharW(kanji_code_strW);
	int r = GetKanjiCodeFromStr(kanji_code_str);
	free(kanji_code_str);
	return r;
}

/**
 *	漢字コードから ISO8859の部番号を返す
 *	@param	kanjicode	IdISO8859-1...16
 *	@return 1...16		ISO8859に関係ない漢字コードの場合は1を返す
 */
int KanjiCodeToISO8859Part(int kanjicode)
{
	static const struct {
		IdKanjiCode kanji_code;
		int iso8859_part;
	} list[] = {
		{ IdISO8859_1, 1 },
		{ IdISO8859_2, 2 },
		{ IdISO8859_3, 3 },
		{ IdISO8859_4, 4 },
		{ IdISO8859_5, 5 },
		{ IdISO8859_6, 6 },
		{ IdISO8859_7, 7 },
		{ IdISO8859_8, 8 },
		{ IdISO8859_9, 9 },
		{ IdISO8859_10, 10 },
		{ IdISO8859_11, 11 },
		{ IdISO8859_13, 13 },
		{ IdISO8859_14, 14 },
		{ IdISO8859_15, 15 },
		{ IdISO8859_16, 16 },
	};
	for (size_t i = 0; i < _countof(list); i++) {
		if (list[i].kanji_code == kanjicode) {
			return list[i].iso8859_part;
		}
	}
	assert(0);
	return 1;
}

BOOL LangIsEnglish(WORD kanji_code)
{
	if (kanji_code == IdISO8859_1 ||
		kanji_code == IdISO8859_2 ||
		kanji_code == IdISO8859_3 ||
		kanji_code == IdISO8859_4 ||
		kanji_code == IdISO8859_5 ||
		kanji_code == IdISO8859_6 ||
		kanji_code == IdISO8859_7 ||
		kanji_code == IdISO8859_8 ||
		kanji_code == IdISO8859_9 ||
		kanji_code == IdISO8859_10 ||
		kanji_code == IdISO8859_11 ||
		kanji_code == IdISO8859_13 ||
		kanji_code == IdISO8859_14 ||
		kanji_code == IdISO8859_15 ||
		kanji_code == IdISO8859_16) {
		return TRUE;
	}
	return FALSE;
}

BOOL LangIsJapanese(WORD kanji_code)
{
	if (kanji_code == IdSJIS ||
		kanji_code == IdEUC ||
		kanji_code == IdJIS) {
		return TRUE;
	}
	return FALSE;
}
