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

#pragma once

#include "tttypes_charset.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	wchar_t *file;		// iniファイルフルパス
	wchar_t *section;	// セクション
	wchar_t *name;
} OverrideCharWidthInfoSet;

typedef struct {
	size_t count;		// 設定数, 0のとき設定なし他のメンバ―無効
	int enable;			// 0(FALSE)/1(TRUE)=disable/enable(上書き設定を行う)
	size_t selected;	// sets[N],  N=0..count-1
	OverrideCharWidthInfoSet *sets;
} OverrideCharWidthInfo;

char UnicodeGetWidthProperty(unsigned long u32);
int UnicodeIsCombiningCharacter(unsigned long u32);
#if 0
int UnicodeIsVariationSelector(unsigned long u32);
#endif
int UnicodeIsEmoji(unsigned long u32);
unsigned short UnicodeCombining(unsigned short first_code, unsigned short code);
int UnicodeIsVirama(unsigned long u32);
int UnicodeBlockIndex(unsigned long u32);
const char *UnicodeBlockName(int);
#if 1
int UnicodeFromISO8859(IdKanjiCode part, unsigned char b, unsigned short *u16);
int UnicodeToISO8859(IdKanjiCode part, unsigned long u32, unsigned char *b);
int UnicodeFromCodePage(IdKanjiCode kanji_code, unsigned char b, unsigned short *u16);
int UnicodeToCodePage(IdKanjiCode kanji_code, unsigned long u32, unsigned char *b);
#endif
int UnicodeFromSBCS(IdKanjiCode kanji_code, unsigned char b, unsigned short *u16);
int UnicodeToSBCS(IdKanjiCode kanji_code, unsigned long u32, unsigned char *b);

int UnicodeOverrideWidthInit(const wchar_t *ini, const wchar_t *section);
void UnicodeOverrideWidthUninit(void);
int UnicodeOverrideWidthCheck(unsigned int u32, int *width);
int UnicodeOverrideWidthAvailable(void);
void OverrideCharWidthInfoGet(const wchar_t *fname, OverrideCharWidthInfo *info);
void OverrideCharWidthInfoFree(OverrideCharWidthInfo *info);

#ifdef __cplusplus
}
#endif
