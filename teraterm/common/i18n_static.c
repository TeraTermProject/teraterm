/*
 * Copyright (C) 2020 TeraTerm Project
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

#include "i18n.h"
#include "codeconv.h"
#include "compat_win.h"

#include <assert.h>

// TODO: バッファ不足時の動作
void GetI18nStrU8(const char *section, const char *key, char *buf, int buf_len, const char *def, const char *iniFile)
{
	size_t r;
	if (pGetPrivateProfileStringW != NULL) {
		// unicode base
		wchar_t tmp[MAX_UIMSG];
		wchar_t defW[MAX_UIMSG];
		r = UTF8ToWideChar(def, -1, defW, _countof(defW));
		assert(r != 0);
		GetI18nStrW(section, key, tmp, _countof(tmp), defW, iniFile);
		r = buf_len;
		WideCharToUTF8(tmp, NULL, buf, &r);
		assert(r != 0);
	}
	else {
		// ANSI -> Wide -> utf8
		char strA[MAX_UIMSG];
		wchar_t strW[MAX_UIMSG];
		GetI18nStr(section, key, strA, _countof(strA), def, iniFile);
		r = MultiByteToWideChar(CP_ACP, 0, strA, -1, strW, _countof(strW));
		assert(r != 0);
		r = buf_len;
		WideCharToUTF8(strW, NULL, buf, &r);
		assert(r != 0);
	}
}

/* vim: set ts=4 sw=4 ff=dos : */
