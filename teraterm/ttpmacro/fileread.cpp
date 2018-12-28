/*
 * Copyright (C) 2018 TeraTerm Project
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

#include <stdio.h>
#include <stdlib.h>
#if (defined(_MSC_VER) && (_MSC_VER >= 1600)) || !defined(_MSC_VER)
#include <stdint.h>
#endif
#include <windows.h>
#include <tchar.h>
#include "codeconv.h"
#include "fileread.h"

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned char uint8_t;
#endif

/**
 *	ファイルをメモリに読み込む
 *	@param[out]	*_len	サイズ(最後に付加される"\0\0"を含む)
 *	@retval		ファイルの中身へのポインタ(使用後free()すること)
 *				NULL=エラー
 */
static void *LoadRawFile(FILE *fp, size_t *_len)
{
    fseek(fp, 0L, SEEK_END);
	fpos_t pos;
	fgetpos(fp, &pos);
    fseek(fp, 0L, SEEK_SET);
	size_t len = (size_t)pos;
	char *buf = (char *)malloc(len + 2);
	buf[len] = 0;
	buf[len+1] = 0;		// UTF-16対策
	fread(buf, 1, len, fp);
	len += 2;
	*_len = len;
	return buf;
}

/**
 *	ファイルをメモリに読み込む
 *	中身はUTF-8に変換される
 *	ファイルの最後は '\0'でターミネートされている
 *
 *	@param[out]	*_len	サイズ(最後に付加される"\0"を含む)
 *	@retval		ファイルの中身へのポインタ(使用後free()すること)
 *				NULL=エラー
 */
char *LoadFileU8(FILE *fp, size_t *_len)
{
	size_t len;
	void *vbuf = LoadRawFile(fp, &len);
	if (vbuf == NULL) {
		*_len = 0;
		return NULL;
	}

	uint8_t *buf = (uint8_t *)vbuf;
	if (len >= 3 && (buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)) {
		// UTF-8 BOM
		//		trim BOM
		len -= 3;
		memmove(&buf[0], &buf[3], len);
	} else if(len >= 2 && (buf[0] == 0xff && buf[1] == 0xfe)) {
		// UTF-16LE BOM
		//		UTF-16LE -> UTF-8
		const char *u8 = ToU8W((wchar_t *)&buf[2]);

		free(buf);
		buf = (uint8_t *)u8;
	} else if(len >= 2 && (buf[0] == 0xfe && buf[1] == 0xff)) {
		// UTF-16BE BOM
		//		UTF-16BE -> UTF-16LE
		{
			uint8_t *p = &buf[2];
			len -= 2;
			len /= 2;
			for (size_t i=0; i<len; i++) {
				uint8_t t = *p;
				*p = *(p+1);
				*(p+1) = t;
				p += 2;
			};
		}
		//		UTF-16LE -> UTF-8
		const char *u8 = ToU8W((wchar_t *)&buf[2]);

		free(buf);
		buf = (uint8_t *)u8;
	} else {
		// ACP? -> UTF-8
		const char *u8 = ToU8A((char *)buf);
		if (u8 != NULL) {
			// ACP -> UTF-8
			free(buf);
			buf = (uint8_t *)u8;
		}
	}

	*_len = strlen((char *)buf)+1;	// 改めて長さを計る
	return (char *)buf;
}

/**
 *	ファイルをメモリに読み込む
 *	中身はUTF-8に変換される
 *
 *	@param[out]	*_len	サイズ(最後に付加される"\0"を含む)
 *	@retval		ファイルの中身へのポインタ(使用後free()すること)
 *				NULL=エラー
 */
char *LoadFileU8A(const char *FileName, size_t *_len)
{
	*_len = 0;
	FILE *fp = fopen(FileName, "rb");
	if (fp == NULL) {
		return NULL;
	}
	size_t len;
	char *u8 = LoadFileU8(fp, &len);
	fclose(fp);
	if (u8 == NULL) {
		return NULL;
	}
	*_len = len;
	return u8;
}

char *LoadFileAA(const char *FileName, size_t *_len)
{
	*_len = 0;
	size_t len;
	char *u8 = LoadFileU8A(FileName, &len);
	if (u8 == NULL) {
		return NULL;
	}
	char *strA = (char *)ToCharU8(u8);
	free(u8);
	if (strA == NULL) {
		return NULL;
	}
	len = strlen(strA);
	*_len = len;
	return strA;
}
