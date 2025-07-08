/*
 * (C) 2025- TeraTerm Project
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

#include "ttxssh.h"
#include "digest.h"
#include "ssherr.h"


struct ssh_digest_ctx {
	int alg;
	EVP_MD_CTX *mdctx;
};

struct ssh_digest_t {
	digest_algorithm id;
	char *name;
	size_t digest_len;
	const EVP_MD *(*mdfunc)(void);
};

/* NB. Indexed directly by algorithm number */
static const struct ssh_digest_t ssh_digests[] = {
	{ SSH_DIGEST_MD5,       "MD5",       16, EVP_md5 },
	{ SSH_DIGEST_RIPEMD160, "RIPEMD160", 20, EVP_ripemd160 },
	{ SSH_DIGEST_SHA1,      "SHA1",      20, EVP_sha1 },
	{ SSH_DIGEST_SHA256,    "SHA256",    32, EVP_sha256 },
	{ SSH_DIGEST_SHA384,    "SHA384",    48, EVP_sha384},
	{ SSH_DIGEST_SHA512,    "SHA512",    64, EVP_sha512 },
	{ SSH_DIGEST_MAX,       NULL,        0,  NULL},
};


char* get_digest_algorithm_name(digest_algorithm id)
{
	const struct ssh_digest_t *ptr = ssh_digests;

	while (ptr->name != NULL) {
		if (id == ptr->id) {
			return ptr->name;
		}
		ptr++;
	}

	// not found.
	return "unknown";
}

/*
 * Import from OpenSSH 7.9p1
 * $OpenBSD: digest-openssl.c,v 1.7 2017/05/08 22:57:38 djm Exp $
 */

static const struct ssh_digest_t *
ssh_digest_by_alg(digest_algorithm alg)
{
	if (alg < 0 || alg >= SSH_DIGEST_MAX)
		return NULL;
	if (ssh_digests[alg].id != alg) /* sanity */
		return NULL;
	if (ssh_digests[alg].mdfunc == NULL)
		return NULL;
	return &(ssh_digests[alg]);
}

size_t
ssh_digest_bytes(int alg)
{
	const struct ssh_digest_t *digest = ssh_digest_by_alg(alg);

	return digest == NULL ? 0 : digest->digest_len;
}

size_t
ssh_digest_blocksize(struct ssh_digest_ctx *ctx)
{
	return EVP_MD_CTX_block_size(ctx->mdctx);
}

struct ssh_digest_ctx *
ssh_digest_start(int alg)
{
	const struct ssh_digest_t *digest = ssh_digest_by_alg(alg);
	struct ssh_digest_ctx *ret;

	if (digest == NULL || ((ret = calloc(1, sizeof(*ret))) == NULL))
		return NULL;
	ret->alg = alg;
	if ((ret->mdctx = EVP_MD_CTX_new()) == NULL) {
		free(ret);
		return NULL;
	}
	if (EVP_DigestInit_ex(ret->mdctx, digest->mdfunc(), NULL) != 1) {
		ssh_digest_free(ret);
		return NULL;
	}
	return ret;
}

int
ssh_digest_copy_state(struct ssh_digest_ctx *from, struct ssh_digest_ctx *to)
{
	if (from->alg != to->alg)
		return SSH_ERR_INVALID_ARGUMENT;
	/* we have bcopy-style order while openssl has memcpy-style */
	if (!EVP_MD_CTX_copy_ex(to->mdctx, from->mdctx))
		return SSH_ERR_LIBCRYPTO_ERROR;
	return 0;
}

int
ssh_digest_update(struct ssh_digest_ctx *ctx, const void *m, size_t mlen)
{
	if (EVP_DigestUpdate(ctx->mdctx, m, mlen) != 1)
		return SSH_ERR_LIBCRYPTO_ERROR;
	return 0;
}

int
ssh_digest_update_buffer(struct ssh_digest_ctx *ctx, buffer_t *b)
{
	return ssh_digest_update(ctx, buffer_ptr(b), buffer_len(b));
}

int
ssh_digest_final(struct ssh_digest_ctx *ctx, u_char *d, size_t dlen)
{
	const struct ssh_digest_t *digest = ssh_digest_by_alg(ctx->alg);
	u_int l = dlen;

	if (digest == NULL || dlen > UINT_MAX)
		return SSH_ERR_INVALID_ARGUMENT;
	if (dlen < digest->digest_len) /* No truncation allowed */
		return SSH_ERR_INVALID_ARGUMENT;
	if (EVP_DigestFinal_ex(ctx->mdctx, d, &l) != 1)
		return SSH_ERR_LIBCRYPTO_ERROR;
	if (l != digest->digest_len) /* sanity */
		return SSH_ERR_INTERNAL_ERROR;
	return 0;
}

void
ssh_digest_free(struct ssh_digest_ctx *ctx)
{
	if (ctx == NULL)
		return;
	EVP_MD_CTX_free(ctx->mdctx);
	freezero(ctx, sizeof(*ctx));
}

int
ssh_digest_memory(digest_algorithm alg, const void *m, size_t mlen, u_char *d, size_t dlen)
{
	const struct ssh_digest_t *digest = ssh_digest_by_alg(alg);
	u_int mdlen;

	if (digest == NULL)
		return SSH_ERR_INVALID_ARGUMENT;
	if (dlen > UINT_MAX)
		return SSH_ERR_INVALID_ARGUMENT;
	if (dlen < digest->digest_len)
		return SSH_ERR_INVALID_ARGUMENT;
	mdlen = dlen;
	if (!EVP_Digest(m, mlen, d, &mdlen, digest->mdfunc(), NULL))
		return SSH_ERR_LIBCRYPTO_ERROR;
	return 0;
}

int
ssh_digest_buffer(digest_algorithm alg, buffer_t *b, u_char *d, size_t dlen)
{
	return ssh_digest_memory(alg, buffer_ptr(b), buffer_len(b), d, dlen);
}
