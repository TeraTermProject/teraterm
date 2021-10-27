/*
 * Copyright (C) 1994-1998 T. Teranishi
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

#include "tttypes_charset.h"

#include "ttlib_charset.h"

static const TLanguageList LanguageList[] = {
	{ IdUtf8,		"UTF-8" },
	{ IdJapanese,	"Japanese" },
	{ IdRussian,	"Russian" },
	{ IdEnglish,	"English" },
	{ IdKorean,		"Korean" },
	{ IdChinese,	"Chinese" },
};

static const TKanjiList KanjiList[] = {
	{ IdUtf8,		IdUTF8,				"UTF-8",					"UTF-8" },
	{ IdEnglish,	IdCodeEnglish,		"English",					"English" },
	{ IdJapanese,	IdSJIS,				"Japanese/SJIS (CP932)",	"SJIS" },
	{ IdJapanese,	IdEUC,				"Japanese/EUC",				"EUC" },
	{ IdJapanese,	IdJIS,				"Japanese/JIS",				"JIS" },
	{ IdJapanese,	IdUTF8,				"Japanese/UTF-8",			"UTF-8" },
	{ IdRussian,	IdWindows,			"Russian/Windows (CP1251)",	"CP1251" },
	{ IdRussian,	IdKOI8,				"Russian/KOI8-R",			"KOI8-R" },
	{ IdRussian,	Id866,				"Russian/CP 866",			"CP866" },
	{ IdRussian,	IdISO,				"Russian/ISO 8859-5",		"IS8859-5" },
	{ IdKorean,		IdKoreanCP51949,	"Korean/KS5601 (CP51949)",	"KS5601" },
	{ IdKorean,		IdUTF8,				"Korean/UTF-8",				"UTF-8" },
	{ IdChinese,	IdCnGB2312,			"China/GB2312 (CP936)",		"GB2312" },
	{ IdChinese,	IdCnBig5,			"China/Big5 (CP950)",		"BIG5" },
	{ IdChinese,	IdUTF8,				"China/UTF-8",				"UTF-8" },
};

const TLanguageList *GetLanguageList(int index)
{
	if (index < 0 || index >= _countof(LanguageList)) {
		return NULL;
	}
	return &LanguageList[index];
}

const char *GetLanguageStr(int language)
{
	for(int i = 0; i < _countof(LanguageList); i++) {
		if (LanguageList[i].language == language) {
			return LanguageList[i].str;
		}
	}
	return LanguageList[0].str;
}

int GetLanguageFromStr(const char *language_str)
{
	for(int i = 0; i < _countof(LanguageList); i++) {
		if (strcmp(language_str, LanguageList[i].str) == 0) {
			return LanguageList[i].language;
		}
	}
	return LanguageList[0].language;
}


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
 *	@param[in]	language (=ts.Language)
 *						IdJapanese, IdKorean, ...
 *	@param[in]	kanji_code (=ts.KanjiCode (receive) or ts.KanjiCodeSend)
 *						IdEUC, IdJIS, IdUTF8, ...
 *	@return	漢字コード文字列
 */
const char *GetKanjiCodeStr(int language, int kanji_code)
{
	for (int i = 0; i < sizeof(KanjiList); i++) {
		if (KanjiList[i].lang == language &&
			KanjiList[i].coding == kanji_code) {
			return KanjiList[i].KanjiCode;
		}
	}
	return "UTF-8";
}

/**
 *	漢字コードを取得
 *	@param[in]	language (=ts.Language)
 *						IdJapanese, IdKorean, ...
 *	@param[in]	kanji_code_str
 *						漢字コード文字列
 *	@return	漢字コード
 */
int GetKanjiCodeFromStr(int language, const char *kanji_code_str)
{
	if (kanji_code_str[0] == 0) {
		return IdUTF8;
	}

	for (int i = 0; i < sizeof(KanjiList); i++) {
		if (KanjiList[i].lang == language &&
			strcmp(KanjiList[i].KanjiCode, kanji_code_str) == 0) {
			return KanjiList[i].coding;
		}
	}
	return IdUTF8;
}

/**
 *	KanjiCodeTranslate(Language(dest), KanjiCodeID(source)) returns KanjiCodeID
 *	@param[in]	lang (IdEnglish, IdJapanese, IdRussian, ...)
 *	@param[in]	kcode (IdSJIS, IdEUC, ... IdKOI8 ... )
 *	@return		langに存在する漢字コードを返す
 *
 *	langに存在しない漢字コードを使用しないようこの関数を使用する
 *		- iniファイルの読み込み時
 *		- 設定でlangを切り替えた時
 */
int KanjiCodeTranslate(int lang, int kcode)
{
	static const int Table[][5] = {
		{1, 2, 3, 4, 5}, /* to English (dummy) */
		{1, 2, 3, 4, 5}, /* to Japanese(dummy) */
		{1, 2, 3, 4, 5}, /* to Russian (dummy) */
		{1, 1, 1, 4, 5}, /* to Korean */
		{4, 4, 4, 4, 5}, /* to Utf8 */
		{1, 2, 2, 2, 2}, /* to Chinese */
	};
	if (lang < 1 || lang > IdLangMax) lang = 1;
	if (kcode < 1 || kcode > 5) kcode = 1;
	lang--;
	kcode--;
	return Table[lang][kcode];
}
