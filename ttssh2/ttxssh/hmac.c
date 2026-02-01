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

/*
 * Import from OpenSSH 7.9p1
 * $OpenBSD: hmac.c,v 1.12 2015/03/24 20:03:44 markus Exp $
 */

#include "ttxssh.h"
#include "digest.h"
#include "hmac.h"
#include "ssherr.h"

struct ssh_hmac_ctx {
	int			 alg;
	struct ssh_digest_ctx	*ictx;
	struct ssh_digest_ctx	*octx;
	struct ssh_digest_ctx	*digest;
	u_char			*buf;
	size_t			 buf_len;
};

size_t
ssh_hmac_bytes(int alg)
{
	return ssh_digest_bytes(alg);
}

struct ssh_hmac_ctx *
ssh_hmac_start(int alg)
{
	struct ssh_hmac_ctx	*ret;

	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->alg = alg;
	if ((ret->ictx = ssh_digest_start(alg)) == NULL ||
	    (ret->octx = ssh_digest_start(alg)) == NULL ||
	    (ret->digest = ssh_digest_start(alg)) == NULL)
		goto fail;
	ret->buf_len = ssh_digest_blocksize(ret->ictx);
	if ((ret->buf = calloc(1, ret->buf_len)) == NULL)
		goto fail;
	return ret;
fail:
	ssh_hmac_free(ret);
	return NULL;
}

int
ssh_hmac_init(struct ssh_hmac_ctx *ctx, const void *key, size_t klen)
{
	size_t i;

	/* reset ictx and octx if no is key given */
	if (key != NULL) {
		/* truncate long keys */
		if (klen <= ctx->buf_len)
			memcpy(ctx->buf, key, klen);
		else if (ssh_digest_memory(ctx->alg, key, klen, ctx->buf,
		    ctx->buf_len) < 0)
			return -1;
		for (i = 0; i < ctx->buf_len; i++)
			ctx->buf[i] ^= 0x36;
		if (ssh_digest_update(ctx->ictx, ctx->buf, ctx->buf_len) < 0)
			return -1;
		for (i = 0; i < ctx->buf_len; i++)
			ctx->buf[i] ^= 0x36 ^ 0x5c;
		if (ssh_digest_update(ctx->octx, ctx->buf, ctx->buf_len) < 0)
			return -1;
		SecureZeroMemory(ctx->buf, ctx->buf_len);
	}
	/* start with ictx */
	if (ssh_digest_copy_state(ctx->ictx, ctx->digest) < 0)
		return -1;
	return 0;
}

int
ssh_hmac_update(struct ssh_hmac_ctx *ctx, const void *m, size_t mlen)
{
	return ssh_digest_update(ctx->digest, m, mlen);
}

int
ssh_hmac_update_buffer(struct ssh_hmac_ctx *ctx, buffer_t *b)
{
	return ssh_digest_update_buffer(ctx->digest, b);
}

int
ssh_hmac_final(struct ssh_hmac_ctx *ctx, u_char *d, size_t dlen)
{
	size_t len;

	len = ssh_digest_bytes(ctx->alg);
	if (dlen < len ||
	    ssh_digest_final(ctx->digest, ctx->buf, len))
		return -1;
	/* switch to octx */
	if (ssh_digest_copy_state(ctx->octx, ctx->digest) < 0 ||
	    ssh_digest_update(ctx->digest, ctx->buf, len) < 0 ||
	    ssh_digest_final(ctx->digest, d, dlen) < 0)
		return -1;
	return 0;
}

void
ssh_hmac_free(struct ssh_hmac_ctx *ctx)
{
	if (ctx != NULL) {
		ssh_digest_free(ctx->ictx);
		ssh_digest_free(ctx->octx);
		ssh_digest_free(ctx->digest);
		if (ctx->buf) {
			SecureZeroMemory(ctx->buf, ctx->buf_len);
			free(ctx->buf);
		}
		SecureZeroMemory(ctx, sizeof(*ctx));
		free(ctx);
	}
}
