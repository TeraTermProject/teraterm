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

#ifndef DIGEST_H
#define DIGEST_H

#include "ttxssh.h"


/* Maximum digest output length */
#define SSH_DIGEST_MAX_LENGTH	64

typedef enum {
	SSH_DIGEST_MD5,
	SSH_DIGEST_RIPEMD160,
	SSH_DIGEST_SHA1,
	SSH_DIGEST_SHA256,
	SSH_DIGEST_SHA384,
	SSH_DIGEST_SHA512,
	SSH_DIGEST_MAX,
} digest_algorithm;

struct ssh_digest_ctx;

char* get_digest_algorithm_name(digest_algorithm id);

/*
 * Import from OpenSSH 7.9p1
 * $OpenBSD: digest.h,v 1.8 2017/05/08 22:57:38 djm Exp $
 */

/* Returns the algorithm's digest length in bytes or 0 for invalid algorithm */
size_t ssh_digest_bytes(int alg);

/* Returns the block size of the digest, e.g. for implementing HMAC */
size_t ssh_digest_blocksize(struct ssh_digest_ctx *ctx);

/* Copies internal state of digest of 'from' to 'to' */
int ssh_digest_copy_state(struct ssh_digest_ctx *from,
    struct ssh_digest_ctx *to);

/* One-shot API */
int ssh_digest_memory(digest_algorithm alg, const void *m, size_t mlen,
    u_char *d, size_t dlen);
int ssh_digest_buffer(digest_algorithm alg, buffer_t *b, u_char *d, size_t dlen);

/* Update API */
struct ssh_digest_ctx *ssh_digest_start(int alg);
int ssh_digest_update(struct ssh_digest_ctx *ctx, const void *m, size_t mlen);
int ssh_digest_update_buffer(struct ssh_digest_ctx *ctx,
    buffer_t *b);
int ssh_digest_final(struct ssh_digest_ctx *ctx, u_char *d, size_t dlen);
void ssh_digest_free(struct ssh_digest_ctx *ctx);

#endif /* DIGEST_H */
