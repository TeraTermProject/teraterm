/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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

// TTCMN.DLL character code conversion

#include "teraterm.h"
#include "tttypes_charset.h"
#include "codemap.h"
#include "codeconv_mb.h"

#define DllExport __declspec(dllexport)
#include "language.h"

// exportされている
DllExport unsigned short ConvertUnicode(unsigned short code, const codemap_t *table, int tmax)
{
	int low, mid, high;
	unsigned short result;

	low = 0;
	high = tmax - 1;
	result = 0; // convert error

	// binary search
	while (low < high) {
		mid = (low + high) / 2;
		if (table[mid].from_code < code) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}

	if (table[low].from_code == code) {
		result = table[low].to_code;
	}

	return (result);
}

// Japanese SJIS -> JIS
DllExport WORD PASCAL SJIS2JIS(WORD KCode)
{
	return CodeConvSJIS2JIS(KCode);
}

// Japanese SJIS -> EUC
DllExport WORD PASCAL SJIS2EUC(WORD KCode)
{
	return CodeConvSJIS2EUC(KCode);
}

// Japanese JIS -> SJIS
DllExport WORD PASCAL JIS2SJIS(WORD KCode)
{
	return CodeConvJIS2SJIS(KCode);
}

// Russian character set conversion
DllExport BYTE PASCAL RussConv(int cin, int cout, BYTE b)
// cin: input character set (IdWindows/IdKOI8/Id866/IdISO)
// cin: output character set (IdWindows/IdKOI8/Id866/IdISO)
{
#if 0
	return CodeConvRussConv(cin, cout, b);
#else
	return 0;
#endif
}

// Russian character set conversion for a character string
DllExport void PASCAL RussConvStr(int cin, int cout, PCHAR Str, int count)
// cin: input character set (IdWindows/IdKOI8/Id866/IdISO)
// cin: output character set (IdWindows/IdKOI8/Id866/IdISO)
{
	int i;

	if (count<=0) {
		return;
	}

	for (i=0; i<=count-1; i++) {
		if ((BYTE)Str[i]>=128) {
			Str[i] = (char)CodeConvRussConv(cin, cout, (BYTE)Str[i]);
		}
	}
}
