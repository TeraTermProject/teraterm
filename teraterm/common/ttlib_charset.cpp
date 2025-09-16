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
	// coding			GUI												INI
	{ IdUTF8,			"UTF-8",										"UTF-8" },
	{ IdISO8859_1,		"ISO8859-1 Latin-1 Western European",			"ISO8859-1" },
	{ IdISO8859_2,		"ISO8859-2 Latin-2 Central European",			"ISO8859-2" },
	{ IdISO8859_3,		"ISO8859-3 Latin-3 South European",				"ISO8859-3" },
	{ IdISO8859_4,		"ISO8859-4 Latin-4 North European",				"ISO8859-4" },
	{ IdISO8859_5,		"ISO8859-5 Latin/Cyrillic",						"ISO8859-5" },
	{ IdISO8859_6,		"ISO8859-6 Latin/Arabic",						"ISO8859-6" },
	{ IdISO8859_7,		"ISO8859-7 Latin/Greek",						"ISO8859-7" },
	{ IdISO8859_8,		"ISO8859-8 Latin/Hebrew",						"ISO8859-8" },
	{ IdISO8859_9,		"ISO8859-9 Latin-5 Turkish",					"ISO8859-9" },
	{ IdISO8859_10,		"ISO8859-10 Latin-6 Nordic",					"ISO8859-10" },
	{ IdISO8859_11,		"ISO8859-11 Latin/Thai",						"ISO8859-11" },
	{ IdISO8859_13,		"ISO8859-13 Latin-7 Baltic Rim",				"ISO8859-13" },
	{ IdISO8859_14,		"ISO8859-14 Latin-8 Celtic",					"ISO8859-14" },
	{ IdISO8859_15,		"ISO8859-15 Latin-9",							"ISO8859-15" },
	{ IdISO8859_16,		"ISO8859-16 Latin-10 South-Eastern European",	"ISO8859-16" },
	// 日本語
	{ IdSJIS,			"SJIS (CP932)",				"SJIS" },
	{ IdEUC,			NULL,						"EUC-JP" },
	{ IdEUC,			"EUC",						"EUC" },
	{ IdJIS,			"JIS",						"JIS" },
	// Russian
#if 0
	{ IdWindows,		"Windows-1251 (CP1251)",	"Windows-1251" },
	{ IdKOI8,			"KOI8-R",					"KOI8-R" },
	{ Id866,			"CP866",					"CP866" },
//	{ IdISO,			NULL,						"ISO8859-5" },
	{ IdISO8859_5,		NULL,						"ISO8859-5" },
#endif
	// Russian
#if 1
	{ IdCP1251,			"CP1251 Windows-1251",		"Windows-1251" },
	{ IdKOI8_NEW,		"KOI8-R (RFC 1489)",		"KOI8-R" },
	{ IdCP866,			"CP866 Cyrillic Russian",	"CP866" },
	{ IdISO8859_5,		"ISO8859-5 Latin/Cyrillic",	"ISO8859-5" },
#endif
	// Korean
	{ IdKoreanCP949,	"KS5601 (CP949)",			"KS5601" },
	{ IdCnGB2312,		"GB2312 (CP936)",			"GB2312" },
	{ IdCnBig5,			"Big5 (CP950)",				"BIG5" },
	// ↓https://www.unicode.org/L2/L1999/99325-E.htm
	{ IdCP437,			"CP437 Latin (US)",			"CP437" },
	{ IdCP737,			"CP737 Greek (A)",			"CP737" },
	{ IdCP775,			"CP775 BaltRim",			"CP775" },
	{ IdCP850,			"CP850 Latin (A)",			"CP850" },
	{ IdCP852,			"CP852 Latin (B)",			"CP852" },
	{ IdCP855,			"CP855 Cyrillic (A)",		"CP855" },
	{ IdCP857,			"CP857 Turkish",			"CP857" },
	{ IdCP860,			"CP860 Portuguese",			"CP860" },
	{ IdCP861,			"CP861 Icelandic",			"CP861" },
	{ IdCP862,			"CP862 Hebrew",				"CP862" },
	{ IdCP863,			"CP863 Canada F",			"CP863" },
	{ IdCP864,			"CP864 Arabic",				"CP864" },
	{ IdCP865,			"CP865 Nordic",				"CP865" },
	{ IdCP866,			"CP866 Cyrillic Russian",	"CP866" },
	{ IdCP869,			"CP869 Greek (B)",			"CP869" },
	{ IdCP874,			"CP874 Thai",				"CP874" },
	// ↓https://learn.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
	{ IdCP1250,			"CP1250 Central European",	"CP1250" },
	{ IdCP1251,			"CP1251 Cyrillic",			"CP1251" },
	{ IdCP1252,			"CP1252 Latin 1",			"CP1252" },
	{ IdCP1253,			"CP1253 Greek",				"CP1253" },
	{ IdCP1254,			"CP1254 Turkish",			"CP1254" },
	{ IdCP1255,			"CP1255 Hebrew",			"CP1255" },
	{ IdCP1256,			"CP1256 Arabic",			"CP1256" },
	{ IdCP1257,			"CP1257 Baltic",			"CP1257" },
	{ IdCP1258,			"CP1258 Vietnamese",		"CP1258" },
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
