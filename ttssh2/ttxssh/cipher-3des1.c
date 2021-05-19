/* Imported from OpenSSH-7.5p1, TeraTerm Project */

/* $OpenBSD: cipher-3des1.c,v 1.12 2015/01/14 10:24:42 markus Exp $ */
/*
 * Copyright (c) 2003 Markus Friedl.  All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// #include "includes.h"

#include <sys/types.h>
#include <string.h>
#include <openssl/evp.h>
#include <windows.h>

typedef unsigned int u_int;
typedef unsigned char u_char;

#include "ssherr.h"

/*
 * This is used by SSH1:
 *
 * What kind of triple DES are these 2 routines?
 *
 * Why is there a redundant initialization vector?
 *
 * If only iv3 was used, then, this would till effect have been
 * outer-cbc. However, there is also a private iv1 == iv2 which
 * perhaps makes differential analysis easier. On the other hand, the
 * private iv1 probably makes the CRC-32 attack ineffective. This is a
 * result of that there is no longer any known iv1 to use when
 * choosing the X block.
 */
struct ssh1_3des_ctx
{
	EVP_CIPHER_CTX  *k1, *k2, *k3;
};

const EVP_CIPHER * evp_ssh1_3des(void);
int ssh1_3des_iv(EVP_CIPHER_CTX *, int, u_char *, int);

static int ssh1_3des_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh1_3des_ctx *c;
	u_char *k1, *k2, *k3;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		if ((c = calloc(1, sizeof(*c))) == NULL)
			return 0;
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key == NULL)
		return 1;
	if (enc == -1)
		enc = EVP_CIPHER_CTX_encrypting(ctx); // ctx->encrypt
	k1 = k2 = k3 = (u_char *) key;
	k2 += 8;
	if (EVP_CIPHER_CTX_key_length(ctx) >= 16+8) {
		if (enc)
			k3 += 16;
		else
			k1 += 16;
	}
	c->k1 = EVP_CIPHER_CTX_new();
	c->k2 = EVP_CIPHER_CTX_new();
	c->k3 = EVP_CIPHER_CTX_new();
	/*** TODO: OPENSSL1.1.1 ERROR CHECK(ticket#39335で処置予定) ***/
	if (EVP_CipherInit(c->k1, EVP_des_cbc(), k1, NULL, enc) == 0 ||
	    EVP_CipherInit(c->k2, EVP_des_cbc(), k2, NULL, !enc) == 0 ||
	    EVP_CipherInit(c->k3, EVP_des_cbc(), k3, NULL, enc) == 0) {
		EVP_CIPHER_CTX_free(c->k1);
		EVP_CIPHER_CTX_free(c->k2);
		EVP_CIPHER_CTX_free(c->k3);
		SecureZeroMemory(c, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
		return 0;
	}
	return 1;
}

static int ssh1_3des_cbc(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, u_int len)
{
	struct ssh1_3des_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		//error("ssh1_3des_cbc: no context");
		return 0;
	}
	if (EVP_Cipher(c->k1, dest, (u_char *)src, len) == 0 ||
	    EVP_Cipher(c->k2, dest, dest, len) == 0 ||
	    EVP_Cipher(c->k3, dest, dest, len) == 0)
		return 0;
	return 1;
}

static int ssh1_3des_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh1_3des_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		EVP_CIPHER_CTX_free(c->k1);
		EVP_CIPHER_CTX_free(c->k2);
		EVP_CIPHER_CTX_free(c->k3);
		SecureZeroMemory(c, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return 1;
}

// ssh1_3des_iv は未使用。
int ssh1_3des_iv(EVP_CIPHER_CTX *evp, int doset, u_char *iv, int len)
{
	struct ssh1_3des_ctx *c;

	if (len != 24) {
		//fatal("%s: bad 3des iv length: %d", __func__, len);
		return SSH_ERR_INVALID_ARGUMENT;
	}

	if ((c = EVP_CIPHER_CTX_get_app_data(evp)) == NULL) {
		//fatal("%s: no 3des context", __func__);
		return SSH_ERR_INTERNAL_ERROR;
	}

	if (doset) {
		//debug3("%s: Installed 3DES IV", __func__);
		memcpy(EVP_CIPHER_CTX_iv_noconst(c->k1), iv, 8);
		memcpy(EVP_CIPHER_CTX_iv_noconst(c->k2), iv + 8, 8);
		memcpy(EVP_CIPHER_CTX_iv_noconst(c->k3), iv + 16, 8);
	} else {
		//debug3("%s: Copying 3DES IV", __func__);
		memcpy(iv, EVP_CIPHER_CTX_iv(c->k1), 8);
		memcpy(iv + 8, EVP_CIPHER_CTX_iv(c->k2), 8);
		memcpy(iv + 16, EVP_CIPHER_CTX_iv(c->k3), 8);
	}
	return 0;
}

const EVP_CIPHER *evp_ssh1_3des(void)
{
	static EVP_CIPHER *p = NULL;

	if (p == NULL) {
		p = EVP_CIPHER_meth_new(NID_undef, /*block_size*/8, /*key_len*/16);
		/*** TODO: OPENSSL1.1.1 ERROR CHECK(ticket#39335で処置予定) ***/
	}
	if (p) {
		EVP_CIPHER_meth_set_iv_length(p, 0);
		EVP_CIPHER_meth_set_init(p, ssh1_3des_init);
		EVP_CIPHER_meth_set_cleanup(p, ssh1_3des_cleanup);
		EVP_CIPHER_meth_set_do_cipher(p, ssh1_3des_cbc);
		EVP_CIPHER_meth_set_flags(p, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH);
	}
	return (p);
}
