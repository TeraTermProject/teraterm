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

#include <stdlib.h>

#include "codeconv.h"
#include "ttlib.h"

#include "layer_for_unicode_crt.h"

/**
 *	fopen の wchar_t版
 */
FILE *__wfopen(const wchar_t *fname, const wchar_t *mode)
{
	FILE *fp = _wfopen(fname, mode);
	if (fp != NULL) {
		return fp;
	}
	char *fnameA = ToCharW(fname);
	char *modeA = ToCharW(mode);
	fp = fopen(fnameA, modeA);
	free(fnameA);
	free(modeA);
	return fp;
}

void __wfopen_s(FILE **fp, wchar_t const* filename, wchar_t const* mode)
{
	if (IsWindowsNTKernel()) {
		// 多分内部で CreateFileW() を使用している
		// NTでのみ使用する
		_wfopen_s(fp, filename, mode);
		if (fp != NULL) {
			return;
		}
		// 念の為 ANSI でもオープンする
	}
	// ANSI でオープン
	char *filenameA = ToCharW(filename);
	char *modeA = ToCharW(mode);
	fopen_s(fp, filenameA, modeA);
	free(filenameA);
	free(modeA);
}
