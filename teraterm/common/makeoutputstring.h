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

#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OutputCharStateTag OutputCharState;

  /* KanjiIn modes */
#define IdKanjiInA 1
#define IdKanjiInB 2
  /* KanjiOut modes */
#define IdKanjiOutB 1
#define IdKanjiOutJ 2
#define IdKanjiOutH 3

typedef struct {
	int code;
	const char *menu_str;
	const char *ini_str;
	int regular;
} KanjiInOutSt;

WORD GetKanjiInCodeFromIni(const char *ini_str);
WORD GetKanjiOutCodeFromIni(const char *ini_str);

const KanjiInOutSt *GetKanjiInList(int index);
const KanjiInOutSt *GetKanjiOutList(int index);

OutputCharState *MakeOutputStringCreate(void);
void MakeOutputStringDestroy(OutputCharState *state);
void MakeOutputStringInit(
	OutputCharState *state,
	WORD kanji_code,
	WORD KanjiIn,
	WORD KanjiOut,
	BOOL jis7katakana);
size_t MakeOutputString(
	OutputCharState *states,
	const wchar_t *B, size_t C,
	char *TempStr, size_t *TempLen_,
	BOOL (*ControlOut)(unsigned int u32, BOOL check_only, char *TempStr, size_t *StrLen, void *data),
	void *data);

/**
 *	Unicodeを指定文字コードに変換する
 *		codeconv.h ではできない JIS コードへの変換可
 *
 *	@param[in]	strW	元文字列
 *	@param[out]	len		変換文字列長 (NULLのとき値を返さない)
 *	@return				変換文字列
 */
char *MakeOutputStringConvW(
	const wchar_t *strW,
	WORD kanji_code, WORD KanjiIn, WORD KanjiOut,BOOL jis7katakana,
	size_t *len);

/**
 *	Unicodeを指定文字コードに変換する
 *		MakeOutputStringConvW()のUTF-8版
 */
char *MakeOutputStringConvU8(
	const char *strU8,
	WORD kanji_code, WORD KanjiIn, WORD KanjiOut,BOOL jis7katakana,
	size_t *len);

#ifdef __cplusplus
}
#endif
