/*
 * Copyright (C) 2017- TeraTerm Project
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

#define TT_VERSION_MAJOR             5
#define TT_VERSION_MINOR             6
#define TT_VERSION_PATCH             0
#define TT_VERSION_SUBSTR            "RC"
//#undef TT_VERSION_SUBSTR
// SUBSTR が不要な時は undef する
//  使用例 "dev", "RC", "RC2"

#define TT_TOSTR(x)					TT_TOSTR_HELPER(x)
#define TT_TOSTR_HELPER(x)			#x
#define TT_VERSION_STR(sep)			TT_TOSTR(TT_VERSION_MAJOR) sep TT_TOSTR(TT_VERSION_MINOR) sep TT_TOSTR(TT_VERSION_PATCH)

#include "svnversion.h"

// TT_VERSION_SUBSTR_HASH
//		TT_VERSION_SUBSTR + (GITVERSION or SVNVERSION)
//		"914287eda" or "r9999"			for release
//		"dev 914287eda" or "dev r9999"	for snapshot
#if !defined(TT_VERSION_SUBSTR)
#if defined(GITVERSION)
// ex "914287eda"
#define TT_VERSION_SUBSTR_HASH GITVERSION
#elif defined(SVNVERSION)
// ex "r9999"
#define TT_VERSION_SUBSTR_HASH "r" TT_TOSTR(SVNVERSION)
#else
//
#undef TT_VERSION_SUBSTR_HASH
#endif
#else
#if defined(GITVERSION)
// ex "dev 914287eda"
#define TT_VERSION_SUBSTR_HASH TT_VERSION_SUBSTR " " GITVERSION
#elif defined(SVNVERSION)
// ex "5.1 dev r9999"
#define TT_VERSION_SUBSTR_HASH TT_VERSION_SUBSTR "r" TT_TOSTR(SVNVERSION)
#else
// ex "5.1 dev"
#define TT_VERSION_SUBSTR_HASH TT_VERSION_SUBSTR
#endif
#endif

// TT_RES_PRODUCT_VERSION_STR
//	リソースファイル(rcファイル) ProductVersion 用
#if defined(TT_VERSION_SUBSTR_HASH)
#define TT_RES_PRODUCT_VERSION_STR \
	TT_VERSION_STR(".") " " TT_VERSION_SUBSTR_HASH
#else
#define TT_RES_PRODUCT_VERSION_STR \
	TT_VERSION_STR(".")
#endif

// TT_RES_VERSION_STR
// 	リソースファイル(rcファイル) FileVersion 用
// 	ex "5, 1, 0, 0"
#undef TT_RES_VERSION_STR
#define TT_RES_VERSION_STR	TT_VERSION_STR(", ") ", 0"
