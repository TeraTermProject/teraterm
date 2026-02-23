/*
 * Copyright (C) 2024- TeraTerm Project
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

#include "tt-version.h"

#define TTXKANJIMENU_VERSION_MAJOR             0
#define TTXKANJIMENU_VERSION_MINOR             1
#define TTXKANJIMENU_VERSION_PATCH             11
#define TTXKANJIMENU_VERSION_STR(sep)          TT_TOSTR(TTXKANJIMENU_VERSION_MAJOR) sep TT_TOSTR(TTXKANJIMENU_VERSION_MINOR) sep TT_TOSTR(TTXKANJIMENU_VERSION_PATCH)
#define TTXKANJIMENU_RES_VERSION_STR           TTXKANJIMENU_VERSION_STR(", ") ", 0"

// TTXKANJIMENU_RES_PRODUCT_VERSION_STR
//	リソースファイル(rcファイル) ProductVersion 用
#if defined(TT_VERSION_SUBSTR_HASH)
#define TTXKANJIMENU_RES_PRODUCT_VERSION_STR \
	TTXKANJIMENU_VERSION_STR(".") " " TT_VERSION_SUBSTR_HASH
#else
#define TTXKANJIMENU_RES_PRODUCT_VERSION_STR \
	TTXKANJIMENU_VERSION_STR(".")
#endif
