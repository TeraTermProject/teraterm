/*
 * Copyright (C) 2020- TeraTerm Project
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

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>

#include "checkeol.h"

// tttypes.h
#define CR   0x0D
#define LF   0x0A

struct CheckEOLData_st {
	BOOL cr_hold;
};

CheckEOLData_t *CheckEOLCreate(void)
{
	CheckEOLData_t *self = (CheckEOLData_t *)calloc(sizeof(CheckEOLData_t), 1);
	return self;
}

void CheckEOLDestroy(CheckEOLData_t *self)
{
	free(self);
}

void CheckEOLClear(CheckEOLData_t *self)
{
	self->cr_hold = FALSE;
}

/**
 *	次にEOL(改行), u32 を出力するか調べる
 *
 *	戻り値は CheckEOLRet の OR で返る
 *
 *	@retval	CheckEOLNoOutput	何も出力しない
 *	@retval	CheckEOLOutputEOL	改行コードを出力する
 *	@retval	CheckEOLOutputChar	u32をそのまま出力する
 */
CheckEOLRet CheckEOLCheck(CheckEOLData_t *self, unsigned int u32)
{
   	// 入力が改行(CR or LF)の場合、
	// 改行の種類(CR or LF or CR+LF)を自動で判定する
	//		入力    CR hold     改行出力   	CR hold 変更
   	// 		+-------+-----------+-----------+------------
	//		CR      なし        しない		セットする
	//		LF      なし        する		変化なし
	//		その他  なし        しない		変化なし
	//		CR      あり        する		変化なし(ホールドしたまま)
	//		LF      あり        する		クリアする
	//		その他  あり        する		クリアする
	if (self->cr_hold == FALSE) {
		if (u32 == CR) {
			self->cr_hold = TRUE;
			return CheckEOLNoOutput;
		}
		else if (u32 == LF) {
			return CheckEOLOutputEOL;
		}
		else {
			return CheckEOLOutputChar;
		}
	}
	else {
		if (u32 == CR) {
			return CheckEOLOutputEOL;
		}
		else if (u32 == LF) {
			self->cr_hold = FALSE;
			return CheckEOLOutputEOL;
		}
		else {
			self->cr_hold = FALSE;
			return (CheckEOLRet)(CheckEOLOutputEOL | CheckEOLOutputChar);
		}
	}
}
