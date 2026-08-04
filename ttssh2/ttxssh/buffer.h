/*
 * Copyright (C) 2004- TeraTerm Project
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
#ifndef BUFFER_H
#define BUFFER_H

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <zlib.h>

#if 1
typedef struct buffer {
	char *buf;	   /* バッファの先頭ポインタ。realloc()により変動する。*/
	size_t offset; /* 現在の読み出し位置 */
	size_t maxlen; /* バッファの最大サイズ */
	size_t len;	   /* バッファに含まれる有効なデータサイズ */
} buffer_t;
#else
typedef struct buffer buffer_t;
#endif

buffer_t *buffer_init(void);
void buffer_clear(buffer_t *buf);
void buffer_free(buffer_t *buf);

/* バッファ全体の長さ */
int buffer_len(buffer_t *buf);
/* まだ読み込んでいない残りのサイズを返す。OpenSSH の sshbuf_len() に相当 */
int buffer_remain_len(buffer_t *buf);
/* バッファの先頭のポインタを返す */
char *buffer_ptr(buffer_t *buf);
/* 現在のポインタを返す。OpenSSH の sshbuf_ptr() に相当 */
char *buffer_tail_ptr(buffer_t *buf);

void buffer_consume(buffer_t *buf, size_t shift_byte);
void buffer_consume_end(buffer_t *buf, size_t shift_byte);
void buffer_rewind(buffer_t *buf);
void *buffer_append_space(buffer_t * buf, size_t size);

int buffer_get(buffer_t *buf, void *v, size_t len);
int buffer_put(buffer_t *buf, const void *v, size_t len);

int buffer_get_int(buffer_t *buf, unsigned int *valp);
void buffer_put_int(buffer_t *buf, int val);

int buffer_get_char(buffer_t *buf, u_char *valp);
void buffer_put_char(buffer_t *buf, int val);

void *buffer_get_string(buffer_t *buf, int *lenp);
void buffer_put_string(buffer_t *buf, const char *v, size_t len);
void buffer_put_cstring(buffer_t *buf, const char *v);
void buffer_put_stringb(buffer_t *buf, buffer_t *v);

void buffer_put_bignum1(buffer_t *buf, const BIGNUM *v);

void buffer_get_bignum2(buffer_t *buf, BIGNUM *v);
void buffer_put_bignum2(buffer_t *buf, const BIGNUM *v);
int buffer_put_bignum2_bytes(buffer_t *buf, const void *v, size_t len);

void buffer_get_bignum_SECSH(buffer_t *buf, BIGNUM *v);

void buffer_get_ec(buffer_t *buf, EC_POINT *v, const EC_GROUP *g);
void buffer_put_ec(buffer_t *buf, const EC_POINT *v, const EC_GROUP *g);

int buffer_overflow_verify(buffer_t *buf, size_t len);
int buffer_compress(z_stream *zstream, char *payload, size_t len, buffer_t *compbuf);
int buffer_decompress(z_stream *zstream, char *payload, size_t len, buffer_t *compbuf);

#endif				/* BUFFER_H */
