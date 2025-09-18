/*
 * Import from OpenSSH  (TeraTerm Project 2008.11.18)
 */
/* $OpenBSD: cipher-ctr.c,v 1.10 2006/08/03 03:34:42 deraadt Exp $ */
/*
 * Copyright (c) 2003 Markus Friedl <markus@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/types.h>
#include <malloc.h>
#include <string.h>
#include <windows.h>

#include "config.h"

#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/des.h>
#include <openssl/blowfish.h>
#include <openssl/cast.h>
#include <openssl/camellia.h>

#include "cipher-ctr.h"

struct ssh_aes_ctr_ctx
{
	AES_KEY		aes_ctx;
	unsigned char	aes_counter[AES_BLOCK_SIZE];
};

#define DES_BLOCK_SIZE sizeof(DES_cblock)
struct ssh_des3_ctr_ctx
{
	DES_key_schedule des3_ctx[3];
	unsigned char	des3_counter[DES_BLOCK_SIZE];
};

struct ssh_blowfish_ctr_ctx
{
	BF_KEY		blowfish_ctx;
	unsigned char	blowfish_counter[BF_BLOCK];
};

struct ssh_cast5_ctr_ctx
{
	CAST_KEY	cast5_ctx;
	unsigned char	cast5_counter[CAST_BLOCK];
};

struct ssh_camellia_ctr_ctx
{
	CAMELLIA_KEY	camellia_ctx;
	unsigned char	camellia_counter[CAMELLIA_BLOCK_SIZE];
};

static void
ssh_ctr_inc(unsigned char *ctr, unsigned int len)
{
	int i;

	for ( i = len - 1; i>= 0; i--)
		if (++ctr[i])
			return;
}

//============================================================================
// AES
//============================================================================
static int
ssh_aes_ctr(EVP_CIPHER_CTX *ctx, unsigned char *dest, const unsigned char *src, size_t len)
{
	struct ssh_aes_ctr_ctx *c;
	unsigned int n = 0;
	unsigned char buf[AES_BLOCK_SIZE];

	if (len == 0)
		return (1);
	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	while ((len--) > 0) {
		if (n == 0) {
			AES_encrypt(c->aes_counter, buf, &c->aes_ctx);
			ssh_ctr_inc(c->aes_counter, AES_BLOCK_SIZE);
		}
		*(dest++) = *(src++) ^ buf[n];
		n = (n + 1) % AES_BLOCK_SIZE;
	}
	return (1);
}

static int
ssh_aes_ctr_init(EVP_CIPHER_CTX *ctx, const unsigned char *key, const unsigned char *iv, int enc)
{
	struct ssh_aes_ctr_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = malloc(sizeof(*c));
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		AES_set_encrypt_key(key, EVP_CIPHER_CTX_key_length(ctx) * 8, &c->aes_ctx);
	if (iv != NULL)
		memcpy(c->aes_counter, iv, AES_BLOCK_SIZE);
	return (1);
}

static int
ssh_aes_ctr_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_aes_ctr_ctx *c;

	if((c = EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		SecureZeroMemory(c, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

#if !defined(LIBRESSL_VERSION_NUMBER) || LIBRESSL_VERSION_NUMBER <= 0x3040300fL || LIBRESSL_VERSION_NUMBER >= 0x3070100fL
const EVP_CIPHER *
evp_aes_128_ctr(void)
{
#if !defined(LIBRESSL_VERSION_NUMBER) || LIBRESSL_VERSION_NUMBER >= 0x3070100fL
	static EVP_CIPHER *p = NULL;

	if (p == NULL) {
		p = EVP_CIPHER_meth_new(NID_undef, /*block_size*/AES_BLOCK_SIZE, /*key_len*/16);
		/*** TODO: OPENSSL1.1.1 ERROR CHECK(ticket#39335で処置予定) ***/
	}
	if (p) {
		EVP_CIPHER_meth_set_iv_length(p, AES_BLOCK_SIZE);
		EVP_CIPHER_meth_set_init(p, ssh_aes_ctr_init);
		EVP_CIPHER_meth_set_cleanup(p, ssh_aes_ctr_cleanup);
		EVP_CIPHER_meth_set_do_cipher(p, ssh_aes_ctr);
		EVP_CIPHER_meth_set_flags(p, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
	}
	return (p);
#else
	static EVP_CIPHER aes_ctr;

	memset(&aes_ctr, 0, sizeof(EVP_CIPHER));
	aes_ctr.nid = NID_undef;
	aes_ctr.block_size = AES_BLOCK_SIZE;
	aes_ctr.iv_len = AES_BLOCK_SIZE;
	aes_ctr.key_len = 16;
	aes_ctr.init = ssh_aes_ctr_init;
	aes_ctr.cleanup = ssh_aes_ctr_cleanup;
	aes_ctr.do_cipher = ssh_aes_ctr;
#ifndef SSH_OLD_EVP
	aes_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;
#endif
	return (&aes_ctr);
#endif
}
#endif

//============================================================================
// Triple-DES
//============================================================================
static int
ssh_des3_ctr(EVP_CIPHER_CTX *ctx, unsigned char *dest, const unsigned char *src, size_t len)
{
	struct ssh_des3_ctr_ctx *c;
	unsigned int n = 0;
	unsigned char buf[DES_BLOCK_SIZE];

	if (len == 0)
		return (1);
	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	while ((len--) > 0) {
		if (n == 0) {
			memcpy(buf, (unsigned char *)(c->des3_counter), DES_BLOCK_SIZE);
			DES_encrypt3((DES_LONG *)buf, &c->des3_ctx[0], &c->des3_ctx[1], &c->des3_ctx[2]);
			ssh_ctr_inc(c->des3_counter, DES_BLOCK_SIZE);
		}
		*(dest++) = *(src++) ^ buf[n];
		n = (n + 1) % DES_BLOCK_SIZE;
	}
	return (1);
}

static int
ssh_des3_ctr_init(EVP_CIPHER_CTX *ctx, const unsigned char *key, const unsigned char *iv, int enc)
{
	struct ssh_des3_ctr_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = malloc(sizeof(*c));
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL) {
		DES_set_key((const_DES_cblock *)key, &c->des3_ctx[0]);
		DES_set_key((const_DES_cblock *)(key + 8), &c->des3_ctx[1]);
		DES_set_key((const_DES_cblock *)(key + 16), &c->des3_ctx[2]);
	}

	if (iv != NULL)
		memcpy(c->des3_counter, iv, DES_BLOCK_SIZE);
	return (1);
}

static int
ssh_des3_ctr_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_des3_ctr_ctx *c;

	if((c = EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		SecureZeroMemory(c, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

#if !defined(LIBRESSL_VERSION_NUMBER) || LIBRESSL_VERSION_NUMBER <= 0x3040300fL || LIBRESSL_VERSION_NUMBER >= 0x3070100fL
const EVP_CIPHER *
evp_des3_ctr(void)
{
#if !defined(LIBRESSL_VERSION_NUMBER) || LIBRESSL_VERSION_NUMBER >= 0x3070100fL
	static EVP_CIPHER *p = NULL;

	if (p == NULL) {
		p = EVP_CIPHER_meth_new(NID_undef, /*block_size*/DES_BLOCK_SIZE, /*key_len*/24);
		/*** TODO: OPENSSL1.1.1 ERROR CHECK(ticket#39335で処置予定) ***/
	}
	if (p) {
		EVP_CIPHER_meth_set_iv_length(p, DES_BLOCK_SIZE);
		EVP_CIPHER_meth_set_init(p, ssh_des3_ctr_init);
		EVP_CIPHER_meth_set_cleanup(p, ssh_des3_ctr_cleanup);
		EVP_CIPHER_meth_set_do_cipher(p, ssh_des3_ctr);
		EVP_CIPHER_meth_set_flags(p, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
	}
	return (p);
#else
	static EVP_CIPHER des3_ctr;

	memset(&des3_ctr, 0, sizeof(EVP_CIPHER));
	des3_ctr.nid = NID_undef;
	des3_ctr.block_size = DES_BLOCK_SIZE;
	des3_ctr.iv_len = DES_BLOCK_SIZE;
	des3_ctr.key_len = 24;
	des3_ctr.init = ssh_des3_ctr_init;
	des3_ctr.cleanup = ssh_des3_ctr_cleanup;
	des3_ctr.do_cipher = ssh_des3_ctr;
#ifndef SSH_OLD_EVP
	des3_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;
#endif
	return (&des3_ctr);
#endif
}
#endif

//============================================================================
// Blowfish
//============================================================================
static int
ssh_bf_ctr(EVP_CIPHER_CTX *ctx, unsigned char *dest, const unsigned char *src, size_t len)
{
	struct ssh_blowfish_ctr_ctx *c;
	unsigned int n = 0;
	unsigned char buf[BF_BLOCK];
	int i, j;
	BF_LONG tmp[(BF_BLOCK + 3) / 4];
	unsigned char *p;

	if (len == 0)
		return (1);
	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	while ((len--) > 0) {
		if (n == 0) {
			for (i = j = 0, p = c->blowfish_counter; i < BF_BLOCK; i += 4, j++) {
				tmp[j]  = ((BF_LONG)*p++) << 24;
				tmp[j] |= ((BF_LONG)*p++) << 16;
				tmp[j] |= ((BF_LONG)*p++) << 8;
				tmp[j] |= ((BF_LONG)*p++);
			}

			BF_encrypt(tmp, &c->blowfish_ctx);

			for (i = j = 0, p = buf; i < BF_BLOCK; i += 4, j++) {
				*p++ = (unsigned char)(tmp[j] >> 24);
				*p++ = (unsigned char)(tmp[j] >> 16);
				*p++ = (unsigned char)(tmp[j] >> 8);
				*p++ = (unsigned char)tmp[j];
			}

			ssh_ctr_inc(c->blowfish_counter, BF_BLOCK);
		}
		*(dest++) = *(src++) ^ buf[n];
		n = (n + 1) % BF_BLOCK;
	}
	return (1);
}

static int
ssh_bf_ctr_init(EVP_CIPHER_CTX *ctx, const unsigned char *key, const unsigned char *iv, int enc)
{
	struct ssh_blowfish_ctr_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = malloc(sizeof(*c));
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL) {
		BF_set_key(&c->blowfish_ctx, EVP_CIPHER_CTX_key_length(ctx), key);
	}

	if (iv != NULL)
		memcpy(c->blowfish_counter, iv, BF_BLOCK);
	return (1);
}

static int
ssh_bf_ctr_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_blowfish_ctr_ctx *c;

	if((c = EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		SecureZeroMemory(c, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

#if !defined(LIBRESSL_VERSION_NUMBER) || LIBRESSL_VERSION_NUMBER <= 0x3040300fL || LIBRESSL_VERSION_NUMBER >= 0x3070100fL
const EVP_CIPHER *
evp_bf_ctr(void)
{
#if !defined(LIBRESSL_VERSION_NUMBER) || LIBRESSL_VERSION_NUMBER >= 0x3070100fL
	static EVP_CIPHER *p = NULL;

	if (p == NULL) {
		p = EVP_CIPHER_meth_new(NID_undef, /*block_size*/BF_BLOCK, /*key_len*/16);
		/*** TODO: OPENSSL1.1.1 ERROR CHECK(ticket#39335で処置予定) ***/
	}
	if (p) {
		EVP_CIPHER_meth_set_iv_length(p, BF_BLOCK);
		EVP_CIPHER_meth_set_init(p, ssh_bf_ctr_init);
		EVP_CIPHER_meth_set_cleanup(p, ssh_bf_ctr_cleanup);
		EVP_CIPHER_meth_set_do_cipher(p, ssh_bf_ctr);
		EVP_CIPHER_meth_set_flags(p, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
	}
	return (p);
#else
	static EVP_CIPHER blowfish_ctr;

	memset(&blowfish_ctr, 0, sizeof(EVP_CIPHER));
	blowfish_ctr.nid = NID_undef;
	blowfish_ctr.block_size = BF_BLOCK;
	blowfish_ctr.iv_len = BF_BLOCK;
	blowfish_ctr.key_len = 16;
	blowfish_ctr.init = ssh_bf_ctr_init;
	blowfish_ctr.cleanup = ssh_bf_ctr_cleanup;
	blowfish_ctr.do_cipher = ssh_bf_ctr;
#ifndef SSH_OLD_EVP
	blowfish_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;
#endif
	return (&blowfish_ctr);
#endif
}
#endif

//============================================================================
// CAST-128
//============================================================================
static int
ssh_cast5_ctr(EVP_CIPHER_CTX *ctx, unsigned char *dest, const unsigned char *src, size_t len)
{
	struct ssh_cast5_ctr_ctx *c;
	unsigned int n = 0;
	unsigned char buf[CAST_BLOCK];
	int i, j;
	CAST_LONG tmp[(CAST_BLOCK + 3) / 4];
	unsigned char *p;

	if (len == 0)
		return (1);
	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	while ((len--) > 0) {
		if (n == 0) {
			for (i = j = 0, p = c->cast5_counter; i < CAST_BLOCK; i += 4, j++) {
				tmp[j]  = ((CAST_LONG)*p++) << 24;
				tmp[j] |= ((CAST_LONG)*p++) << 16;
				tmp[j] |= ((CAST_LONG)*p++) << 8;
				tmp[j] |= ((CAST_LONG)*p++);
			}

			CAST_encrypt(tmp, &c->cast5_ctx);

			for (i = j = 0, p = buf; i < CAST_BLOCK; i += 4, j++) {
				*p++ = (unsigned char)(tmp[j] >> 24);
				*p++ = (unsigned char)(tmp[j] >> 16);
				*p++ = (unsigned char)(tmp[j] >> 8);
				*p++ = (unsigned char)tmp[j];
			}

			ssh_ctr_inc(c->cast5_counter, CAST_BLOCK);
		}
		*(dest++) = *(src++) ^ buf[n];
		n = (n + 1) % CAST_BLOCK;
	}
	return (1);
}

static int
ssh_cast5_ctr_init(EVP_CIPHER_CTX *ctx, const unsigned char *key, const unsigned char *iv, int enc)
{
	struct ssh_cast5_ctr_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = malloc(sizeof(*c));
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL) {
		CAST_set_key(&c->cast5_ctx, EVP_CIPHER_CTX_key_length(ctx), key);
	}

	if (iv != NULL)
		memcpy(c->cast5_counter, iv, CAST_BLOCK);
	return (1);
}

static int
ssh_cast5_ctr_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_cast5_ctr_ctx *c;

	if((c = EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		SecureZeroMemory(c, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

#if !defined(LIBRESSL_VERSION_NUMBER) || LIBRESSL_VERSION_NUMBER <= 0x3040300fL || LIBRESSL_VERSION_NUMBER >= 0x3070100fL
const EVP_CIPHER *
evp_cast5_ctr(void)
{
#if !defined(LIBRESSL_VERSION_NUMBER) || LIBRESSL_VERSION_NUMBER >= 0x3070100fL
	static EVP_CIPHER *p = NULL;

	if (p == NULL) {
		p = EVP_CIPHER_meth_new(NID_undef, /*block_size*/CAST_BLOCK, /*key_len*/16);
		/*** TODO: OPENSSL1.1.1 ERROR CHECK(ticket#39335で処置予定) ***/
	}
	if (p) {
		EVP_CIPHER_meth_set_iv_length(p, CAST_BLOCK);
		EVP_CIPHER_meth_set_init(p, ssh_cast5_ctr_init);
		EVP_CIPHER_meth_set_cleanup(p, ssh_cast5_ctr_cleanup);
		EVP_CIPHER_meth_set_do_cipher(p, ssh_cast5_ctr);
		EVP_CIPHER_meth_set_flags(p, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
	}
	return (p);
#else
	static EVP_CIPHER cast5_ctr;

	memset(&cast5_ctr, 0, sizeof(EVP_CIPHER));
	cast5_ctr.nid = NID_undef;
	cast5_ctr.block_size = CAST_BLOCK;
	cast5_ctr.iv_len = CAST_BLOCK;
	cast5_ctr.key_len = 16;
	cast5_ctr.init = ssh_cast5_ctr_init;
	cast5_ctr.cleanup = ssh_cast5_ctr_cleanup;
	cast5_ctr.do_cipher = ssh_cast5_ctr;
#ifndef SSH_OLD_EVP
	cast5_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;
#endif
	return (&cast5_ctr);
#endif
}
#endif

//============================================================================
// Camellia
//============================================================================
static int
ssh_camellia_ctr(EVP_CIPHER_CTX *ctx, unsigned char *dest, const unsigned char *src, size_t len)
{
	struct ssh_camellia_ctr_ctx *c;
	unsigned int n = 0;
	unsigned char buf[CAMELLIA_BLOCK_SIZE];

	if (len == 0)
		return (1);
	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	while ((len--) > 0) {
		if (n == 0) {
			Camellia_encrypt(c->camellia_counter, buf, &c->camellia_ctx);
			ssh_ctr_inc(c->camellia_counter, CAMELLIA_BLOCK_SIZE);
		}
		*(dest++) = *(src++) ^ buf[n];
		n = (n + 1) % CAMELLIA_BLOCK_SIZE;
	}
	return (1);
}

static int
ssh_camellia_ctr_init(EVP_CIPHER_CTX *ctx, const unsigned char *key, const unsigned char *iv, int enc)
{
	struct ssh_camellia_ctr_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = malloc(sizeof(*c));
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		Camellia_set_key(key, EVP_CIPHER_CTX_key_length(ctx) * 8, &c->camellia_ctx);
	if (iv != NULL)
		memcpy(c->camellia_counter, iv, CAMELLIA_BLOCK_SIZE);
	return (1);
}

static int
ssh_camellia_ctr_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_camellia_ctr_ctx *c;

	if((c = EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		SecureZeroMemory(c, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

#if !defined(LIBRESSL_VERSION_NUMBER) || LIBRESSL_VERSION_NUMBER <= 0x3040300fL || LIBRESSL_VERSION_NUMBER >= 0x3070100fL
const EVP_CIPHER *
evp_camellia_128_ctr(void)
{
#if !defined(LIBRESSL_VERSION_NUMBER) || LIBRESSL_VERSION_NUMBER >= 0x3070100fL
	static EVP_CIPHER *p = NULL;

	if (p == NULL) {
		p = EVP_CIPHER_meth_new(NID_undef, /*block_size*/CAMELLIA_BLOCK_SIZE, /*key_len*/16);
		/*** TODO: OPENSSL1.1.1 ERROR CHECK(ticket#39335で処置予定) ***/
	}
	if (p) {
		EVP_CIPHER_meth_set_iv_length(p, CAMELLIA_BLOCK_SIZE);
		EVP_CIPHER_meth_set_init(p, ssh_camellia_ctr_init);
		EVP_CIPHER_meth_set_cleanup(p, ssh_camellia_ctr_cleanup);
		EVP_CIPHER_meth_set_do_cipher(p, ssh_camellia_ctr);
		EVP_CIPHER_meth_set_flags(p, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
	}
	return (p);
#else
	static EVP_CIPHER camellia_ctr;

	memset(&camellia_ctr, 0, sizeof(EVP_CIPHER));
	camellia_ctr.nid = NID_undef;
	camellia_ctr.block_size = CAMELLIA_BLOCK_SIZE;
	camellia_ctr.iv_len = CAMELLIA_BLOCK_SIZE;
	camellia_ctr.key_len = 16;
	camellia_ctr.init = ssh_camellia_ctr_init;
	camellia_ctr.cleanup = ssh_camellia_ctr_cleanup;
	camellia_ctr.do_cipher = ssh_camellia_ctr;
#ifndef SSH_OLD_EVP
	camellia_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;
#endif
	return (&camellia_ctr);
#endif
}
#endif
