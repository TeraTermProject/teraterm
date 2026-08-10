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
//
// buffer.c
//

#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <zlib.h>

#include "ttxssh.h"	// for logprintf()
#include "openbsd-compat.h"

#include "buffer.h"
#include "ssherr.h"

/* buffer_t.buf の拡張の上限値 (16MB) */
#define BUFFER_SIZE_MAX 0x1000000

/* buffer_t.buf の拡張時に追加で確保する量 (32KB) */
#define BUFFER_INCREASE_MARGIN (32*1024)

#if 0
typedef struct buffer {
	char *buf;      /* バッファの先頭ポインタ。realloc()により変動する。*/
	size_t offset;     /* 現在の読み出し位置 */
	size_t maxlen;     /* バッファの最大サイズ */
	size_t len;        /* バッファに含まれる有効なデータサイズ */
} buffer_t;
#endif

// バッファのオフセットを初期化し、まだ読んでいない状態にする。
// Tera Term(TTSSH)オリジナル関数。
void buffer_rewind(buffer_t *buf)
{
	buf->offset = 0;
}

void buffer_clear(buffer_t *buf)
{
	buf->offset = 0;
	buf->len = 0;
}

buffer_t *buffer_init(void)
{
	void *ptr;
	buffer_t *buf;
	size_t size = 4096;

	buf = malloc(sizeof(buffer_t));
	ptr = malloc(size);
	if (buf && ptr) {
		memset(buf, 0, sizeof(buffer_t));
		memset(ptr, 0, size);
		buf->buf = ptr;
		buf->maxlen = size;
		buf->len = 0;
		buf->offset = 0;

	} else {
		ptr = NULL; *(char *)ptr = 0;
	}

	return (buf);
}

void buffer_free(buffer_t * buf)
{
	if (buf != NULL) {
		// セキュリティ対策 (2006.8.3 yutaka)
		int len =  buffer_len(buf);
		SecureZeroMemory(buf->buf, len);
		free(buf->buf);
		free(buf);
	}
}

// バッファの領域拡張を行う。
// return: 拡張前のバッファポインター
void *buffer_append_space(buffer_t * buf, size_t size)
{
	size_t n;
	size_t newlen;
	void *p;

	n = buf->offset + size;
	if (n < buf->maxlen) {
		//
	} else {
		// バッファが足りないので補充する。(2005.7.2 yutaka)
		newlen = buf->maxlen + size + BUFFER_INCREASE_MARGIN;
		if (newlen > BUFFER_SIZE_MAX) {
			goto panic;
		}
		buf->buf = realloc(buf->buf, newlen);
		if (buf->buf == NULL)
			goto panic;
		buf->maxlen = newlen;
	}

	p = buf->buf + buf->offset;
	//buf->offset += size;
	buf->len = buf->offset + size;

	return (p);

panic:
	abort();
	return (NULL);
}

// from OpenSSH 10.4p1 sshbuf.c
static int sshbuf_check_sanity(buffer_t *buf)
{
	if (buf == NULL || buf->maxlen > BUFFER_SIZE_MAX || buf->len > buf->maxlen || buf->offset > buf->len) {
		return SSH_ERR_INTERNAL_ERROR;
	}
	return 0;
}

// from OpenSSH 10.4p1 sshbuf.c
int buffer_check_reserve(buffer_t *buf, size_t len)
{
	int r;

	if ((r = sshbuf_check_sanity(buf)) != 0)
		return r;

	/* Check that len is reasonable and that max_size + available < len */
	if (len > BUFFER_SIZE_MAX || BUFFER_SIZE_MAX - len < buf->len - buf->offset)
		return SSH_ERR_NO_BUFFER_SPACE;
	return 0;
}

// from OpenSSH 10.4p1 sshbuf.c
int buffer_allocate(buffer_t *buf, size_t len)
{
	size_t rlen, need;
	u_char *dp;
	int r;

	if ((r = buffer_check_reserve(buf, len)) != 0)
		return r;

	if (len + buf->len <= buf->maxlen)
		return 0; /* already have it. */

	/*
	 * Prefer to alloc in SSHBUF_SIZE_INC units, but
	 * allocate less if doing so would overflow max_size.
	 */
	need = len + buf->len - buf->maxlen;
	rlen = buf->maxlen + need;

	if (rlen > BUFFER_SIZE_MAX)
		rlen = buf->maxlen + need;

	if ((dp = realloc(buf->buf, rlen)) == NULL) {
		return SSH_ERR_ALLOC_FAIL;
	}
	buf->maxlen = rlen;
	buf->buf = dp;
	if ((r = buffer_check_reserve(buf, len)) < 0) {
		/* shouldn't fail */
		return r;
	}
	return 0;
}

// from OpenSSH 10.4p1 sshbuf.c
// buf->len は len バイト（拡張した長さ） 進むので、
// この関数を呼んだら dpp に len バイト書き込まなければならない
int buffer_reserve(buffer_t *buf, size_t len, u_char **dpp)
{
	u_char *dp;
	int r;

	if (dpp != NULL)
		*dpp = NULL;

	if ((r = buffer_allocate(buf, len)) != 0)
		return r;

	dp = buf->buf + buf->len; // 拡張前のデータ末尾のポインタ
	buf->len += len;
	if (dpp != NULL)
		*dpp = dp;
	return 0;
}

int buffer_put(buffer_t * buf, const void *v, size_t len)
{
	size_t n;
	int ret = SSH_ERR_INTERNAL_ERROR;
	size_t newlen;

	for (;;) {
		n = buf->offset + len;
		if (n < buf->maxlen) {
			memcpy(buf->buf + buf->offset, v, len);
			buf->offset += len;
			buf->len = buf->offset;
			ret = 0;
			break;

		} else {
			// バッファが足りないので補充する。(2005.7.2 yutaka)
			newlen = buf->maxlen + len + BUFFER_INCREASE_MARGIN;
			if (newlen > BUFFER_SIZE_MAX) {
				return SSH_ERR_NO_BUFFER_SPACE;
			}
			buf->buf = realloc(buf->buf, newlen);
			if (buf->buf == NULL)
				return SSH_ERR_ALLOC_FAIL;
			buf->maxlen = newlen;
		}
	}

	return ret;
}

int buffer_get(buffer_t *buf, void *v, size_t len)
{
	if (len > buf->len - buf->offset) {
		// TODO: エラー処理
		OutputDebugPrintf("buffer_get: trying to get more bytes %u than in buffer %u",
		                  len, buf->len - buf->offset);
		return SSH_ERR_MESSAGE_INCOMPLETE;
	}
	memcpy(v, buf->buf + buf->offset, len);
	buf->offset += len;
	return 0;
}

int buffer_get_int(buffer_t *buf, unsigned int *valp)
{
	unsigned char tmp[4];
	int r;

	if ((r = buffer_get(buf, (char *)tmp, 4)) != 0)
		return r;
	if (valp != NULL)
		*valp = get_uint32(tmp);
	return 0;
}

int buffer_get_char(buffer_t *buf, u_char *valp)
{
	int r;

	if ((r = buffer_get(buf, valp, 1)) != 0)
		return r;
	return 0;
}

// NOTE: You should free the return pointer if it's unused.
static char *buffer_get_string_internal(char **data_ptr, int *buflen_ptr)
{
	char *data = *data_ptr;
	char *ptr;
	unsigned int buflen;

	buflen = get_uint32_MSBfirst(data);
	data += 4;
	// buflen == 0の場合でも、'\0'分は確保し、data_ptrを進め、リターンする。
//	if (buflen <= 0)
//		return NULL;

	ptr = malloc(buflen + 1);
	if (ptr == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: malloc failed.", __FUNCTION__);
		if (buflen_ptr != NULL)
			*buflen_ptr = 0;
		return NULL;
	}
	memcpy(ptr, data, buflen);
	ptr[buflen] = '\0'; // null-terminate
	data += buflen;

	*data_ptr = data;
	if (buflen_ptr != NULL)
		*buflen_ptr = buflen;

	return(ptr);
}

// NOTE: You should free the return pointer if it's unused.
void *buffer_get_string_(buffer_t *buf, int *lenp)
{
	char *data, *olddata;
	void *ret = NULL;
	size_t off;
	int len, datalen;

	// Check size
	len = buffer_remain_len(buf);
	if (len < 4)
		goto error;

	data = olddata = buffer_tail_ptr(buf);
	datalen = get_uint32_MSBfirst(data);
	if (len - 4 < datalen)
		goto error;

	ret = buffer_get_string_internal(&data, lenp);
	off = data - olddata;
	buf->offset += off;

error:;
	return (ret);
}


// from OpenSSH 10.4p1 sshbuf-getput-basic.c
int buffer_get_string(buffer_t *buf, u_char **valp, size_t *lenp)
{
	const u_char *val;
	size_t len;
	int r;

	if (valp != NULL)
		*valp = NULL;
	if (lenp != NULL)
		*lenp = 0;
	if ((r = buffer_get_string_direct(buf, &val, &len)) < 0)
			return r;
		if (valp != NULL) {
		if ((*valp = malloc(len + 1)) == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: malloc failed.", __FUNCTION__);
			return SSH_ERR_ALLOC_FAIL;
		}
		if (len != 0)
			memcpy(*valp, val, len);
		(*valp)[len] = '\0';
	}
	if (lenp != NULL)
		*lenp = len;
	return 0;
}

// from OpenSSH 10.4p1 sshbuf-getput-basic.c
int buffer_get_string_direct(buffer_t *buf, const u_char **valp, size_t *lenp)
{
	size_t len;
	const u_char *p;
	int r;

	if (valp != NULL)
		*valp = NULL;
	if (lenp != NULL)
		*lenp = 0;
	if ((r = buffer_peek_string_direct(buf, &p, &len)) < 0)
		return r;
	if (valp != NULL)
		*valp = p;
	if (lenp != NULL)
		*lenp = len;
	if (buffer_consume(buf, len + 4) != 0) {
		/* Shouldn't happen */
		logprintf(LOG_LEVEL_ERROR, "%s: SSH_ERR_INTERNAL_ERROR", __FUNCTION__);
		return SSH_ERR_INTERNAL_ERROR;
	}
	return 0;
}

// from OpenSSH 10.4p1 sshbuf-getput-basic.c
int buffer_peek_string_direct(buffer_t *buf, const u_char **valp, size_t *lenp)
{
	uint32_t len;
	const u_char *p = buffer_tail_ptr(buf);

	if (valp != NULL)
		*valp = NULL;
	if (lenp != NULL)
		*lenp = 0;
	if (buffer_remain_len(buf) < 4) {
		logprintf(LOG_LEVEL_ERROR, "%s: SSH_ERR_MESSAGE_INCOMPLETE", __FUNCTION__);
		return SSH_ERR_MESSAGE_INCOMPLETE;
	}
	len = PEEK_U32(p);
	if (len > BUFFER_SIZE_MAX - 4) {
		logprintf(LOG_LEVEL_ERROR, "%s: SSH_ERR_STRING_TOO_LARGE", __FUNCTION__);
		return SSH_ERR_STRING_TOO_LARGE;
	}
	if (buffer_remain_len(buf) - 4 < len) {
		logprintf(LOG_LEVEL_ERROR, "%s: SSH_ERR_MESSAGE_INCOMPLETE", __FUNCTION__);
		return SSH_ERR_MESSAGE_INCOMPLETE;
	}
	if (valp != NULL)
		*valp = p + 4;
	if (lenp != NULL)
		*lenp = len;
	return 0;
}

// from OpenSSH 10.4p1 sshbuf-getput-basic.c
int buffer_get_cstring(buffer_t *buf, char **valp, size_t *lenp)
{
	size_t len;
	const u_char *p, *z;
	int r;

	if (valp != NULL)
		*valp = NULL;
	if (lenp != NULL)
		*lenp = 0;
	if ((r = buffer_peek_string_direct(buf, &p, &len)) != 0)
		return r;
	/* Allow a \0 only at the end of the string */
	if (len > 0 && (z = memchr(p, '\0', len)) != NULL && z < p + len - 1) {
		logprintf(LOG_LEVEL_ERROR, "%s: SSH_ERR_INVALID_FORMAT", __FUNCTION__);
		return SSH_ERR_INVALID_FORMAT;
	}
	if ((r = buffer_skip_string(buf)) != 0)
		return -1;
	if (valp != NULL) {
		if ((*valp = malloc(len + 1)) == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: SSH_ERR_ALLOC_FAIL", __FUNCTION__);
			return SSH_ERR_ALLOC_FAIL;
		}
		if (len != 0)
			memcpy(*valp, p, len);
		(*valp)[len] = '\0';
	}
	if (lenp != NULL)
		*lenp = (size_t)len;
	return 0;
}

// from OpenSSH 10.4p1 sshbuf-getput-basic.c
int buffer_get_stringb(buffer_t *buf, buffer_t *v)
{
	uint32_t len;
	u_char *p;
	int r;

	/*
	 * Use sshbuf_peek_string_direct() to figure out if there is
	 * a complete string in 'buf' and copy the string directly
	 * into 'v'.
	 */
	if ((r = buffer_peek_string_direct(buf, NULL, NULL)) != 0 ||
	    (r = buffer_get_int(buf, &len)) != 0 ||
	    (r = buffer_reserve(v, len, &p)) != 0 ||
	    (r = buffer_get(buf, p, len)) != 0)
		return r;
	return 0;
}

int buffer_put_string(buffer_t *msg, const char *v, size_t len)
{
	char buf[4];
	int val;
	int ret = SSH_ERR_INTERNAL_ERROR;

	assert(len == (size_t)(int)len);
	// 「サイズ＋文字列」で書き込む。サイズは4byteのbig-endian。
	val = htonl((int)len);
	memcpy(buf, &val, sizeof(val));
	ret = buffer_put(msg, buf, sizeof(buf));
	if (v != NULL) {
		ret = buffer_put(msg, v, len);
	}

	return ret;
}

int buffer_put_cstring(buffer_t *buf, const char *v)
{
	return buffer_put_string(buf, v, strlen(v));
}

int buffer_put_stringb(buffer_t *buf, buffer_t *v)
{
	return buffer_put_string(buf, buffer_ptr(v), buffer_len(v));
}

int buffer_put_char(buffer_t *buf, int val)
{
	char ch = (char)val;

	return buffer_put(buf, &ch, 1);
}

int buffer_put_int(buffer_t *buf, int val)
{
	char tmp[4];

	set_uint32_MSBfirst(tmp, val);
	return buffer_put(buf, tmp, sizeof(tmp));
}

int buffer_len(buffer_t *buf)
{
	return (int)(buf->len);
}

int buffer_remain_len(buffer_t *buf)
{
	return (int)(buf->len - buf->offset);
}

// buffer_put() や buffer_append_space() でメッセージバッファに追加を行うと、
// 内部で realloc() によりバッファポインタが変わってしまうことがある。
// メッセージバッファのポインタを取得する際は、バッファ追加が完了した後に
// 行わなければ、BOFで落ちる。
char *buffer_ptr(buffer_t *buf)
{
	return (buf->buf);
}

char *buffer_tail_ptr(buffer_t *buf)
{
	return (char *)(buf->buf + buf->offset);
}

int buffer_overflow_verify(buffer_t *buf, size_t len)
{
	if (buf->offset + len > buf->maxlen) {
		return -1;  // error
	}
	return 0; // no problem
}

// for SSH1
int buffer_put_bignum1(buffer_t *buf, const BIGNUM *v)
{
	unsigned int bits, bin_size;
	unsigned char *d;
	int oi;
	char msg[2];
	int r = 0;

	bits = BN_num_bits(v);
	bin_size = (bits + 7) / 8;
	d = malloc(bin_size);
	if (d == NULL) {
		*d = 0;
		return SSH_ERR_ALLOC_FAIL;
	}

	d[0] = '\0';
	/* Get the value of in binary */
	oi = BN_bn2bin(v, d);
	if (oi != bin_size) {
		r = SSH_ERR_INTERNAL_ERROR;
		goto error;
	}

	/* Store the number of bits in the buffer in two bytes, msb first. */
	set_ushort16_MSBfirst(msg, bits);
	if ((r = buffer_put(buf, msg, 2)) != 0)
		goto error;

	/* Store the binary data. */
	if ((r = buffer_put(buf, (char *)d, oi)) != 0)
		goto error;

error:
	free(d);
	return r;
}

// for SSH2
int buffer_put_bignum2(buffer_t *buf, const BIGNUM *v)
{
	unsigned int bytes;
	unsigned char *d;
	int oi;
	unsigned int hasnohigh = 0;
	int r = 0;

	bytes = BN_num_bytes(v) + 1; /* extra padding byte */
	d = malloc(bytes);
	if (d == NULL) {
		*d = 0;
		return SSH_ERR_ALLOC_FAIL;
	}

	d[0] = '\0';
	/* Get the value of in binary */
	oi = BN_bn2bin(v, d+1);
	hasnohigh = (d[1] & 0x80) ? 0 : 1;
	if ((r = buffer_put_string(buf, d + hasnohigh, bytes - hasnohigh)) != 0)
		goto error;
	//memset(buf, 0, bytes);

error:
	free(d);
	return r;
}

static void buffer_get_bignum2_internal(char **data, BIGNUM *value)
{
	char *buf = *data;
	int len;

	len = get_uint32_MSBfirst(buf);
	buf += 4;
	BN_bin2bn(buf, len, value);
	buf += len;

	*data = buf;
}

int buffer_get_bignum2(buffer_t *buf, BIGNUM *v)
{
	char *data, *olddata;
	size_t off;

	data = olddata = buffer_tail_ptr(buf);
	buffer_get_bignum2_internal(&data, v);
	off = data - olddata;
	buf->offset += off;

	return 0;
}

int buffer_get_bignum_SECSH(buffer_t *buf, BIGNUM *v)
{
	char *d;
	unsigned int bits, bytes;
	int r;

	if ((r = buffer_get_int(buf, &bits)) != 0) {
		return r;
	}
	bytes = (bits + 7) / 8;

	if ((buf->len - buf->offset) < bytes) {
		return SSH_ERR_NO_BUFFER_SPACE;
	}
	d = buf->buf + buf->offset;
	if ((*d & 0x80) != 0) {
		char *tmp = (char *)malloc(bytes + 1);
		tmp[0] = '\0';
		memcpy(tmp + 1, d, bytes);
		BN_bin2bn(tmp, bytes + 1, v);
		free(tmp);
	}
	else {
		BN_bin2bn(d, bytes, v);
	}

	buf->offset += bytes;

	return 0;
}

int buffer_put_bignum2_bytes(buffer_t *buf, const void *v, size_t len)
{
	u_char *d;
	const u_char *s = (const u_char *)v;
	int prepend;

	if (len > BUFFER_SIZE_MAX - 5)
		return SSH_ERR_NO_BUFFER_SPACE;

	/* Skip leading zero bytes */
	for (; len > 0 && *s == 0; len--, s++)
		;
	/*
	 * If most significant bit is set then prepend a zero byte to
	 * avoid interpretation as a negative number.
	 */
	prepend = len > 0 && (s[0] & 0x80) != 0;

	d = buffer_append_space(buf, len + 4 + prepend);
	POKE_U32(d, len + prepend);
	if (prepend)
		d[4] = 0;
	memcpy(d + 4 + prepend, s, len);
	return 0;
}

int buffer_put_ec(buffer_t *buf, const EC_POINT *v, const EC_GROUP *g)
{
	unsigned char *d = NULL;
	size_t len;
	int r = 0;

	/* Determine length */
	len = EC_POINT_point2oct(g, v, POINT_CONVERSION_UNCOMPRESSED,
	    NULL, 0, NULL);
	/* Convert */
	d = malloc(len);
	if (d == NULL) {
		*d = 0;
		return SSH_ERR_ALLOC_FAIL;
	}
	if (EC_POINT_point2oct(g, v, POINT_CONVERSION_UNCOMPRESSED,
	    d, len, NULL) != len) {
		return SSH_ERR_INTERNAL_ERROR;
		goto error;
	}
	/* Append */
	if ((r = buffer_put_string(buf, d, len)) != 0)
		goto error;

error:
	free(d);
	return r;
}

static void buffer_get_ec_internal(char **data, EC_POINT *v, const EC_GROUP *g)
{
	char *buf = *data;
	size_t len;

	len = get_uint32_MSBfirst(buf);
	buf += 4;
	EC_POINT_oct2point(g, v, buf, len, NULL);
	buf += len;

	*data = buf;
}

int buffer_get_ec(buffer_t *buf, EC_POINT *v, const EC_GROUP *g)
{
	char *data, *olddata;
	size_t off;

	data = olddata = buffer_tail_ptr(buf);
	buffer_get_ec_internal(&data, v, g);
	off = data - olddata;
	buf->offset += off;

	return 0;
}

void buffer_dump(FILE *fp, buffer_t *buf)
{
	int i;
	char *ch = buffer_ptr(buf);

	for (i = 0 ; i < buffer_len(buf) ; i++) {
		fprintf(fp, "%02x", ch[i] & 0xff);
		if (i % 16 == 15)
			fprintf(fp, "\n");
		else if (i % 2 == 1)
			fprintf(fp, " ");
	}
	fprintf(fp, "\n");
}

// バッファのオフセットを進める。
int buffer_consume(buffer_t *buf, size_t len)
{
	if (len > buf->len - buf->offset) {
		return SSH_ERR_MESSAGE_INCOMPLETE;
	} else {
		buf->offset += len;
		// lenは変えない。
	}
	return 0;
}

// バッファの末尾を縮退する。
int buffer_consume_end(buffer_t *buf, size_t len)
{
	if (len > buf->len - buf->offset) {
		return SSH_ERR_MESSAGE_INCOMPLETE;
	} else {
		buf->len -= len;
		// offsetは変えない。
	}
	return 0;
}


// パケットの圧縮
int buffer_compress(z_stream *zstream, char *payload, size_t len, buffer_t *compbuf)
{
	unsigned char buf[4096];
	int status;

	// input buffer
	zstream->next_in = payload;
	zstream->avail_in = (uInt)len;
	assert(len == (size_t)(uInt)len);

	do {
		// output buffer
		zstream->next_out = buf;
		zstream->avail_out = sizeof(buf);

		// バッファを圧縮する。圧縮すると、逆にサイズが大きくなることも考慮すること。
		status = deflate(zstream, Z_PARTIAL_FLUSH);
		if (status == Z_OK) {
			if (buffer_put(compbuf, buf, sizeof(buf) - zstream->avail_out) != 0) {
				return -1; // error
			}
		} else {
			return -1; // error
		}
	} while (zstream->avail_out == 0);

	return 0; // success
}

// パケットの展開
int buffer_decompress(z_stream *zstream, char *payload, size_t len, buffer_t *compbuf)
{
	unsigned char buf[4096];
	int status;

	// input buffer
	zstream->next_in = payload;
	zstream->avail_in = (uInt)len;
	assert(len == (size_t)(uInt)len);

	do {
		// output buffer
		zstream->next_out = buf;
		zstream->avail_out = sizeof(buf);

		// バッファを展開する。
		status = inflate(zstream, Z_PARTIAL_FLUSH);
		if (status == Z_OK) {
			if (buffer_put(compbuf, buf, sizeof(buf) - zstream->avail_out) != 0) {
				return -1; // error
			}

		} else if (status == Z_OK) {
			break;

		} else {
			return -1; // error
		}
	} while (zstream->avail_out == 0);

	return 0; // success
}
