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

/* Multi Byte, Double Byte, Single Byte 関連の文字コード変換 */

#include <windows.h>
#include <assert.h>

#include "codeconv_mb.h"
#include "tttypes_charset.h"

// Japanese SJIS -> JIS
WORD CodeConvSJIS2JIS(WORD KCode)
{
	WORD x0,x1,x2,y0;
	BYTE b = LOBYTE(KCode);

	if ((b>=0x40) && (b<=0x7f)) {
		x0 = 0x8140;
		y0 = 0x2121;
	}
	else if ((b>=0x80) && (b<=0x9e)) {
		x0 = 0x8180;
		y0 = 0x2160;
	}
	else {
		x0 = 0x819f;
		y0 = 0x2221;
	}
	if (HIBYTE(KCode) >= 0xe0) {
		x0 = x0 + 0x5f00;
		y0 = y0 + 0x3e00;
	}
	x1 = (KCode-x0) / 0x100;
	x2 = (KCode-x0) % 0x100;
	return (y0 + x1*0x200 + x2);
}

// Japanese JIS -> SJIS
WORD CodeConvJIS2SJIS(WORD KCode)
{
	WORD n1, n2, SJIS;

	n1 = (KCode-0x2121) / 0x200;
	n2 = (KCode-0x2121) % 0x200;

	if (n1<=0x1e) {
		SJIS = 0x8100 + n1*256;
	}
	else {
		SJIS = 0xC100 + n1*256;
	}

	if (n2<=0x3e) {
		return (SJIS + n2 + 0x40);
	}
	else if ((n2>=0x3f) && (n2<=0x5d)) {
		return (SJIS + n2 + 0x41);
	}
	else {
		return (SJIS + n2 - 0x61);
	}
}

// Japanese SJIS -> EUC
WORD CodeConvSJIS2EUC(WORD KCode)
{
	return (CodeConvSJIS2JIS(KCode) | 0x8080);
}

// Russian character set conversion
BYTE CodeConvRussConv(int cin, int cout, BYTE b)
{
	assert(FALSE);
	(void)cin;
	(void)cout;
	(void)b;
	return 0;
}
