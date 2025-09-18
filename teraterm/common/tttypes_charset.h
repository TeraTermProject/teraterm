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
#if 0
	IdWindows,			// →IdCP1251
	IdKOI8,				// →IdKOI8_NEW
	Id866,				// →IdCP866
	IdISO,				// →IdISO8859_5
#endif
	// Korean
	IdKoreanCP949,		// CP949, KS5601
	// Chinese
	IdCnGB2312,			// CP936, GB2312
	IdCnBig5,			// CP950, Big5
	// Debug
	IdDebug,
	// CodePage (Single Byte Character Sets)
	IdCP437,
	IdCP737,
	IdCP775,
	IdCP850,
	IdCP852,
	IdCP855,
	IdCP857,
	IdCP860,
	IdCP861,
	IdCP862,
	IdCP863,
	IdCP864,
	IdCP865,
	IdCP866,
	IdCP869,
	IdCP874,
	IdCP1250,
	IdCP1251,
	IdCP1252,
	IdCP1253,
	IdCP1254,
	IdCP1255,
	IdCP1256,
	IdCP1257,
	IdCP1258,
	// Other ASCII-based
	IdKOI8_NEW,
} IdKanjiCode;

// DEC Special Graphics
typedef enum {
	IdDecSpecialUniToDec,		// 0 = Unicode -> DEC Special
	IdDecSpecialDecToUni,		// 1 = DEC Special -> Unicode
	IdDecSpecialDoNot,			// 2 = Do not map
} IdDecSpecial;
