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

#pragma once

#include <windows.h>

#include "tttypes_charset.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	IdKanjiCode coding;		// 内部のコード
	const char *CodeStrGUI;	// GUIで表示する文字列
	const char *CodeStrINI;	// iniファイル、コマンドラインの文字列
} TKanjiList;

const char *GetLanguageStr(int language);
int GetLanguageFromStr(const char *language_str);

const TKanjiList *GetKanjiList(int index);
const char *GetKanjiCodeStr(int kanji_code);
int GetKanjiCodeFromStr(const char *kanji_code_str);
int GetKanjiCodeFromStrW(const wchar_t *kanji_code_strW);

BOOL LangIsEnglish(WORD kanji_code);
BOOL LangIsJapanese(WORD kanji_code);

#ifdef __cplusplus
}
#endif
