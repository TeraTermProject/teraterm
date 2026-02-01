/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2024- TeraTerm Project
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

#include "ddelib.h"

/**
 *	DDE用 バイナリエンコード
 *	DDE通信できるようバイナリをエスケープされたバイナリにエンコードする
 *
 *	0x00 -> 0x01 + 0x01
 *	0x01 -> 0x01 + 0x02
 *
 *	@param[in]	src			入力バイナリ ptr
 *	@param[in]	sec_len		入力バイナリ長
 *	@param[out]	dest_len	出力バイナリ(エンコード済み)長
 *	@return					出力バイナリ(エンコード済み)ptr,不要になったらfree()する
 */
uint8_t *EncodeDDEBinary(const uint8_t *src, size_t src_len, size_t *dest_len)
{
	uint8_t *d = (uint8_t *)malloc(src_len * 2);	// 最大2倍のデータ長
	if (d == NULL) {
		*dest_len = 0;
		return NULL;
	}
	uint8_t *p = d;
	size_t i;
	size_t dlen = 0;
	for (i = 0; i < src_len; i++) {
		const uint8_t c = *src++;
		if (c <= 0x01) {
			*p++ = 0x01;
			*p++ = c + 1;
			dlen += 2;
		}
		else {
			*p++ = c;
			dlen++;
		}
	}
	*dest_len = dlen;
	return d;
}

/**
 *	DDE用 バイナリデコード
 *	DDE通信できるようエスケープされたバイナリをデコードする
 *
 *	0x01 + (x+1) -> x
 *
 *	@param[in]	src			入力バイナリ ptr
 *	@param[in]	sec_len		入力バイナリ長
 *	@param[out]	dest_len	出力バイナリ(デコード済み)長
 *	@return					出力バイナリ(デコード済み)ptr,不要になったらfree()する
 */
uint8_t *DecodeDDEBinary(const uint8_t *src, size_t src_len, size_t *dest_len)
{
	uint8_t *d = (uint8_t *)malloc(src_len);
	if (d == NULL) {
		*dest_len = 0;
		return NULL;
	}
	uint8_t *p = d;
	size_t i;
	size_t dlen = 0;
	for (i = 0; i < src_len; i++) {
		if (*src == 0x01) {
			src++;
			*p++ = (*src++) - 1;
			i++;
		}
		else {
			*p++ = *src++;
		}
		dlen++;
	}
	*dest_len = dlen;
	return d;
}
