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

#include "tttypes_charset.h"

#include "ttlib_charset.h"

static const TKanjiList KanjiList[] = {
	{ /*IdUtf8,*/		IdUTF8,				"UTF-8",					"UTF-8" },
	{ /*IdEnglish,*/	IdISO8859_1,		"English/ISO8859-1",		"ISO8859-1" },
	{ /*IdEnglish,*/	IdISO8859_2,		"English/ISO8859-2",		"ISO8859-2" },
	{ /*IdEnglish,*/	IdISO8859_3,		"English/ISO8859-3",		"ISO8859-3" },
	{ /*IdEnglish,*/	IdISO8859_4,		"English/ISO8859-4",		"ISO8859-4" },
	{ /*IdEnglish,*/	IdISO8859_5,		"English/ISO8859-5",		"ISO8859-5" },
	{ /*IdEnglish,*/	IdISO8859_6,		"English/ISO8859-6",		"ISO8859-6" },
	{ /*IdEnglish,*/	IdISO8859_7,		"English/ISO8859-7",		"ISO8859-7" },
	{ /*IdEnglish,*/	IdISO8859_8,		"English/ISO8859-8",		"ISO8859-8" },
	{ /*IdEnglish,*/	IdISO8859_9,		"English/ISO8859-9",		"ISO8859-9" },
	{ /*IdEnglish,*/	IdISO8859_10,		"English/ISO8859-10",		"ISO8859-10" },
	{ /*IdEnglish,*/	IdISO8859_11,		"English/ISO8859-11",		"ISO8859-11" },
	{ /*IdEnglish,*/	IdISO8859_13,		"English/ISO8859-13",		"ISO8859-13" },
	{ /*IdEnglish,*/	IdISO8859_14,		"English/ISO8859-14",		"ISO8859-14" },
	{ /*IdEnglish,*/	IdISO8859_15,		"English/ISO8859-15",		"ISO8859-15" },
	{ /*IdEnglish,*/	IdISO8859_16,		"English/ISO8859-16",		"ISO8859-16" },
//	{ /*IdJapanese,*/	IdUTF8,				"Japanese/UTF-8",			"UTF-8" },
	{ /*IdJapanese,*/	IdSJIS,				"Japanese/SJIS (CP932)",	"SJIS" },
	{ /*IdJapanese,*/	IdEUC,				"Japanese/EUC",				"EUC" },
	{ /*IdJapanese,*/	IdJIS,				"Japanese/JIS",				"JIS" },
	{ /*IdRussian,*/	IdWindows,			"Russian/Windows (CP1251)",	"Windows(CP1251)" },
	{ /*IdRussian,*/	IdKOI8,				"Russian/KOI8-R",			"KOI8-R" },
	{ /*IdRussian,*/	Id866,				"Russian/CP866",			"CP866" },
	{ /*IdRussian,*/	IdISO,				"Russian/ISO 8859-5",		"ISO8859-5" },
	{ /*IdKorean,*/		IdKoreanCP949,		"Korean/KS5601 (CP949)",	"KS5601" },
//	{ /*IdKorean,*/		IdUTF8,				"Korean/UTF-8",				"UTF-8" },
	{ /*IdChinese,*/	IdCnGB2312,			"Chinese/GB2312 (CP936)",	"GB2312" },
	{ /*IdChinese,*/	IdCnBig5,			"Chinese/Big5 (CP950)",		"BIG5" },
//	{ /*IdChinese,*/	IdUTF8,				"Chinese/UTF-8",			"UTF-8" },
};

/**
 *	�\���̂�1�v�f���擾
 *	�͈͊O�ɂȂ�����NULL���Ԃ�
 */
const TKanjiList *GetKanjiList(int index)
{
	if (index < 0 || index >= _countof(KanjiList)) {
		return NULL;
	}
	return &KanjiList[index];
}

/**
 *	�����R�[�h��������擾
 *	ini�t�@�C���ɕۑ����銿���R�[�h������p
 *
 *	@param[in]	kanji_code (=ts.KanjiCode (receive) or ts.KanjiCodeSend)
 *						IdEUC, IdJIS, IdUTF8, ...
 *	@return	�����R�[�h������
 */
const char *GetKanjiCodeStr(int kanji_code)
{
	for (int i = 0; i < _countof(KanjiList); i++) {
		if (KanjiList[i].coding == kanji_code) {
			return KanjiList[i].KanjiCode;
		}
	}
	return "UTF-8";
}

/**
 *	�����R�[�h���擾
 *	ini�t�@�C���ɕۑ�����Ă��銿���R�[�h������������R�[�h�ɕϊ�����

 *	@param[in]	kanji_code_str
 *						�����R�[�h������
 *	@return	�����R�[�h
 */
int GetKanjiCodeFromStr(const char *kanji_code_str)
{
	if (kanji_code_str[0] == 0) {
		return IdUTF8;
	}

	for (int i = 0; i < _countof(KanjiList); i++) {
		if (strcmp(KanjiList[i].KanjiCode, kanji_code_str) == 0) {
			return KanjiList[i].coding;
		}
	}
	return IdUTF8;
}

/**
 *	KanjiCodeTranslate(KanjiCodeID(source)) returns KanjiCodeID
 *
 *	@param[in]	kcode (IdSJIS, IdEUC, ... IdKOI8 ... ) (IdKanjiCode)
 *	@return		���݂��銿���R�[�h��Ԃ�
 *
 *	�����R�[�h���Ó����`�F�b�N����
 *		- ini�t�@�C���̓ǂݍ��ݎ�
 *		- �ݒ��lang��؂�ւ�����
 */
int KanjiCodeTranslate(int kcode)
{
	// �g�ݍ��킹�����݂��Ă���?
	for (int i = 0; i < _countof(KanjiList); i++) {
		if (KanjiList[i].coding == kcode) {
			// ���݂��Ă���
			return kcode;
		}
	}

	assert(0);	// ���肦�Ȃ�
	return IdUTF8;
}

/**
 *	�����R�[�h���� ISO8859�̕��ԍ���Ԃ�
 *	@param	kanjicode	IdISO8859-1...16
 *	@return 1...16		ISO8859�Ɋ֌W�Ȃ������R�[�h�̏ꍇ��1��Ԃ�
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
