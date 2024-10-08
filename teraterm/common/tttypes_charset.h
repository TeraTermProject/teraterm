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

#pragma once

typedef enum {
	IdUTF8,
	// English
	IdISO8859_1,
	IdISO8859_2,
	IdISO8859_3,
	IdISO8859_4,
	IdISO8859_5,
	IdISO8859_6,
	IdISO8859_7,
	IdISO8859_8,
	IdISO8859_9,
	IdISO8859_10,
	IdISO8859_11,
	IdISO8859_13,
	IdISO8859_14,
	IdISO8859_15,
	IdISO8859_16,
	// Japanese
	IdSJIS,
	IdEUC,
	IdJIS,
	// Russian code sets
	IdWindows,			// CP1251
	IdKOI8,
	Id866,
	IdISO,
	// Korean
	IdKoreanCP949,		// CP949, KS5601
	// Chinese
	IdCnGB2312,			// CP936, GB2312
	IdCnBig5,			// CP950, Big5
	// Debug
	IdDebug,
} IdKanjiCode;
