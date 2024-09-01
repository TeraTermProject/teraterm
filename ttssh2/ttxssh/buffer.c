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

#include <winsock2.h>
#include <malloc.h>
#include "buffer.h"
#include "ttxssh.h"
#include "util.h"
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <zlib.h>

// �o�b�t�@�̃I�t�Z�b�g�����������A�܂��ǂ�ł��Ȃ���Ԃɂ���B
// Tera Term(TTSSH)�I���W�i���֐��B
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
	unsigned int size = 4096;

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
		// �Z�L�����e�B�΍� (2006.8.3 yutaka)
		int len =  buffer_len(buf);
		SecureZeroMemory(buf->buf, len);
		free(buf->buf);
		free(buf);
	}
}

// �o�b�t�@�̗̈�g�����s���B
// return: �g���O�̃o�b�t�@�|�C���^�[
void *buffer_append_space(buffer_t * buf, size_t size)
{
	unsigned int n;
	unsigned int newlen;
	void *p;

	n = buf->offset + size;
	if (n < buf->maxlen) {
		//
	} else {
		// �o�b�t�@������Ȃ��̂ŕ�[����B(2005.7.2 yutaka)
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

int buffer_append(buffer_t * buf, char *ptr, int size)
{
	unsigned int n;
	int ret = -1;
	unsigned int newlen;

	for (;;) {
		n = buf->offset + size;
		if (n < buf->maxlen) {
			memcpy(buf->buf + buf->offset, ptr, size);
			buf->offset += size;
			buf->len = buf->offset;
			ret = 0;
			break;

		} else {
			// �o�b�t�@������Ȃ��̂ŕ�[����B(2005.7.2 yutaka)
			newlen = buf->maxlen + size + BUFFER_INCREASE_MARGIN;
			if (newlen > BUFFER_SIZE_MAX) {
				goto panic;
			}
			buf->buf = realloc(buf->buf, newlen);
			if (buf->buf == NULL)
				goto panic;
			buf->maxlen = newlen;
		}
	}

	return (ret);

panic:
	abort();
	return (ret);
}

int buffer_append_length(buffer_t * msg, char *ptr, int size)
{
	char buf[4];
	int val;
	int ret = -1;

	val = htonl(size);
	memcpy(buf, &val, sizeof(val));
	ret = buffer_append(msg, buf, sizeof(buf));
	if (ptr != NULL) {
		ret = buffer_append(msg, ptr, size);
	}

	return (ret);
}

void buffer_put_raw(buffer_t *msg, char *ptr, int size)
{
	int ret = -1;

	ret = buffer_append(msg, ptr, size);
}

int buffer_get_ret(buffer_t *msg, void *buf, size_t len)
{
	if (len > msg->len - msg->offset) {
		// TODO: �G���[����
		OutputDebugPrintf("buffer_get_ret: trying to get more bytes %u than in buffer %u",
		    len, msg->len - msg->offset);
		return (-1);
	}
	memcpy(buf, msg->buf + msg->offset, len);
	msg->offset += len;
	return (0);
}

int buffer_get_int_ret(unsigned int *ret, buffer_t *msg)
{
	unsigned char buf[4];

	if (buffer_get_ret(msg, (char *) buf, 4) == -1)
		return (-1);
	if (ret != NULL)
		*ret = get_uint32(buf);
	return (0);
}

unsigned int buffer_get_int(buffer_t *msg)
{
	unsigned int ret = 0;

	if (buffer_get_int_ret(&ret, msg) == -1) {
		// TODO: �G���[����
		logprintf(LOG_LEVEL_ERROR, "buffer_get_int: buffer error");
	}
	return (ret);
}

int buffer_get_char_ret(char *ret, buffer_t *msg)
{
	if (buffer_get_ret(msg, ret, 1) == -1)
		return (-1);
	return (0);
}

int buffer_get_char(buffer_t *msg)
{
	char ch;

	if (buffer_get_char_ret(&ch, msg) == -1) {
		// TODO: �G���[����
		OutputDebugPrintf("buffer_get_char: buffer error");
	}
	return (unsigned char)ch;
}

// getting string buffer.
// NOTE: You should free the return pointer if it's unused.
// (2005.6.26 yutaka)
char *buffer_get_string(char **data_ptr, int *buflen_ptr)
{
	char *data = *data_ptr;
	char *ptr;
	unsigned int buflen;

	buflen = get_uint32_MSBfirst(data);
	data += 4;
	// buflen == 0�̏ꍇ�ł��A'\0'���͊m�ۂ��Adata_ptr��i�߁A���^�[������B
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

// buffer_get_string() �� buffer_t �ŁB�{���͂����炪 OpenSSH �X�^�C���B
// NOTE: You should free the return pointer if it's unused.
void *buffer_get_string_msg(buffer_t *msg, int *buflen_ptr)
{
	char *data, *olddata;
	void *ret = NULL;
	int off;
	int len, datalen;

	// Check size
	len = buffer_remain_len(msg);
	if (len < 4)
		goto error;

	data = olddata = buffer_tail_ptr(msg);
	datalen = get_uint32_MSBfirst(data);
	if (len - 4 < datalen)
		goto error;

	ret = buffer_get_string(&data, buflen_ptr);
	off = data - olddata;
	msg->offset += off;

error:;
	return (ret);
}

void buffer_put_string(buffer_t *msg, char *ptr, size_t size)
{
	char buf[4];
	int val;
	int ret = -1;

	// �u�T�C�Y�{������v�ŏ������ށB�T�C�Y��4byte��big-endian�B
	val = htonl(size);
	memcpy(buf, &val, sizeof(val));
	ret = buffer_append(msg, buf, sizeof(buf));
	if (ptr != NULL) {
		ret = buffer_append(msg, ptr, size);
	}
}

void buffer_put_cstring(buffer_t *msg, char *ptr)
{
	buffer_put_string(msg, ptr, strlen(ptr));
}

void buffer_put_char(buffer_t *msg, int value)
{
	char ch = value;

	buffer_append(msg, &ch, 1);
}

void buffer_put_padding(buffer_t *msg, int size)
{
	char ch = ' ';
	int i;

	for (i = 0 ; i < size ; i++) {
		buffer_append(msg, &ch, 1);
	}
}

void buffer_put_int(buffer_t *msg, int value)
{
	char buf[4];

	set_uint32_MSBfirst(buf, value);
	buffer_append(msg, buf, sizeof(buf));
}

int buffer_len(buffer_t *msg)
{
	return (msg->len);
}

// �܂��ǂݍ���ł��Ȃ��c��̃T�C�Y��Ԃ��B�{���͂����炪 OpenSSH �X�^�C���B
int buffer_remain_len(buffer_t *msg)
{
	return (msg->len - msg->offset);
}

// buffer_append() �� buffer_append_space() �Ń��b�Z�[�W�o�b�t�@�ɒǉ����s���ƁA
// ������ realloc() �ɂ��o�b�t�@�|�C���^���ς���Ă��܂����Ƃ�����B
// ���b�Z�[�W�o�b�t�@�̃|�C���^���擾����ۂ́A�o�b�t�@�ǉ��������������
// �s��Ȃ���΁ABOF�ŗ�����B
char *buffer_ptr(buffer_t *msg)
{
	return (msg->buf);
}

char *buffer_tail_ptr(buffer_t *msg)
{
	return (char *)(msg->buf + msg->offset);
}

int buffer_overflow_verify(buffer_t *msg, int len)
{
	if (msg->offset + len > msg->maxlen) {
		return -1;  // error
	}
	return 0; // no problem
}

// for SSH1
void buffer_put_bignum(buffer_t *buffer, BIGNUM *value)
{
	unsigned int bits, bin_size;
	unsigned char *buf;
	int oi;
	char msg[2];

	bits = BN_num_bits(value);
	bin_size = (bits + 7) / 8;
	buf = malloc(bin_size);
	if (buf == NULL) {
		*buf = 0;
		goto error;
	}

	buf[0] = '\0';
	/* Get the value of in binary */
	oi = BN_bn2bin(value, buf);
	if (oi != bin_size) {
		goto error;
	}

	/* Store the number of bits in the buffer in two bytes, msb first. */
	set_ushort16_MSBfirst(msg, bits);
	buffer_append(buffer, msg, 2);

	/* Store the binary data. */
	buffer_append(buffer, (char *)buf, oi);

error:
	free(buf);
}

// for SSH2
void buffer_put_bignum2(buffer_t *msg, BIGNUM *value)
{
	unsigned int bytes;
	unsigned char *buf;
	int oi;
	unsigned int hasnohigh = 0;

	bytes = BN_num_bytes(value) + 1; /* extra padding byte */
	buf = malloc(bytes);
	if (buf == NULL) {
		*buf = 0;
		goto error;
	}

	buf[0] = '\0';
	/* Get the value of in binary */
	oi = BN_bn2bin(value, buf+1);
	hasnohigh = (buf[1] & 0x80) ? 0 : 1;
	buffer_put_string(msg, buf+hasnohigh, bytes-hasnohigh);
	//memset(buf, 0, bytes);

error:
	free(buf);
}

void buffer_get_bignum2(char **data, BIGNUM *value)
{
	char *buf = *data;
	int len;

	len = get_uint32_MSBfirst(buf);
	buf += 4;
	BN_bin2bn(buf, len, value);
	buf += len;

	*data = buf;
}

void buffer_get_bignum2_msg(buffer_t *msg, BIGNUM *value)
{
	char *data, *olddata;
	int off;

	data = olddata = buffer_tail_ptr(msg);
	buffer_get_bignum2(&data, value);
	off = data - olddata;
	msg->offset += off;
}

void buffer_get_bignum_SECSH(buffer_t *buffer, BIGNUM *value)
{
	char *buf;
	unsigned int bits, bytes;

	bits = buffer_get_int(buffer);
	bytes = (bits + 7) / 8;

	if ((buffer->len - buffer->offset) < bytes) {
		return;
	}
	buf = buffer->buf + buffer->offset;
	if ((*buf & 0x80) != 0) {
		char *tmp = (char *)malloc(bytes + 1);
		tmp[0] = '\0';
		memcpy(tmp + 1, buf, bytes);
		BN_bin2bn(tmp, bytes + 1, value);
		free(tmp);
	}
	else {
		BN_bin2bn(buf, bytes, value);
	}

	buffer->offset += bytes;
}

void buffer_put_ecpoint(buffer_t *msg, const EC_GROUP *curve, const EC_POINT *point)
{
	unsigned char *buf = NULL;
	size_t len;
	BN_CTX *bnctx;

	/* Determine length */
	bnctx = BN_CTX_new();
	len = EC_POINT_point2oct(curve, point, POINT_CONVERSION_UNCOMPRESSED,
	    NULL, 0, bnctx);
	/* Convert */
	buf = malloc(len);
	if (buf == NULL) {
		*buf = 0;
		goto error;
	}
	if (EC_POINT_point2oct(curve, point, POINT_CONVERSION_UNCOMPRESSED,
	    buf, len, bnctx) != len) {
		goto error;
	}
	/* Append */
	buffer_put_string(msg, buf, len);

error:
	free(buf);
	BN_CTX_free(bnctx);
}

void buffer_get_ecpoint(char **data, const EC_GROUP *curve, EC_POINT *point)
{
	char *buf = *data;
	size_t len;
	BN_CTX *bnctx;

	bnctx = BN_CTX_new();

	len = get_uint32_MSBfirst(buf);
	buf += 4;
	EC_POINT_oct2point(curve, point, buf, len, bnctx);
	buf += len;

	*data = buf;

	BN_CTX_free(bnctx);
}

void buffer_get_ecpoint_msg(buffer_t *msg, const EC_GROUP *curve, EC_POINT *point)
{
	char *data, *olddata;
	int off;

	data = olddata = buffer_tail_ptr(msg);
	buffer_get_ecpoint(&data, curve, point);
	off = data - olddata;
	msg->offset += off;
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

// �o�b�t�@�̃I�t�Z�b�g��i�߂�B
void buffer_consume(buffer_t *buf, size_t shift_byte)
{
	if (shift_byte > buf->len - buf->offset) {
		// TODO: fatal error
	} else {
		buf->offset += shift_byte;
		// len�͕ς��Ȃ��B
	}
}

// �o�b�t�@�̖������k�ނ���B
void buffer_consume_end(buffer_t *buf, size_t shift_byte)
{
	if (shift_byte > buf->len - buf->offset) {
		// TODO: fatal error
	} else {
		buf->len -= shift_byte;
		// offset�͕ς��Ȃ��B
	}
}


// �p�P�b�g�̈��k
int buffer_compress(z_stream *zstream, char *payload, int len, buffer_t *compbuf)
{
	unsigned char buf[4096];
	int status;

	// input buffer
	zstream->next_in = payload;
	zstream->avail_in = len;

	do {
		// output buffer
		zstream->next_out = buf;
		zstream->avail_out = sizeof(buf);

		// �o�b�t�@�����k����B���k����ƁA�t�ɃT�C�Y���傫���Ȃ邱�Ƃ��l�����邱�ƁB
		status = deflate(zstream, Z_PARTIAL_FLUSH);
		if (status == Z_OK) {
			buffer_append(compbuf, buf, sizeof(buf) - zstream->avail_out);
		} else {
			return -1; // error
		}
	} while (zstream->avail_out == 0);

	return 0; // success
}

// �p�P�b�g�̓W�J
int buffer_decompress(z_stream *zstream, char *payload, int len, buffer_t *compbuf)
{
	unsigned char buf[4096];
	int status;

	// input buffer
	zstream->next_in = payload;
	zstream->avail_in = len;

	do {
		// output buffer
		zstream->next_out = buf;
		zstream->avail_out = sizeof(buf);

		// �o�b�t�@��W�J����B
		status = inflate(zstream, Z_PARTIAL_FLUSH);
		if (status == Z_OK) {
			buffer_append(compbuf, buf, sizeof(buf) - zstream->avail_out);

		} else if (status == Z_OK) {
			break;

		} else {
			return -1; // error
		}
	} while (zstream->avail_out == 0);

	return 0; // success
}
