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
 * $OpenBSD: hmac.h,v 1.9 2014/06/24 01:13:21 djm Exp $
 */

#ifndef HMAC_H
#define HMAC_H

#include "ttxssh.h"

/* Returns the algorithm's digest length in bytes or 0 for invalid algorithm */
size_t ssh_hmac_bytes(int alg);

struct ssh_hmac_ctx *ssh_hmac_start(int alg);

/* Sets the state of the HMAC or resets the state if key == NULL */
int ssh_hmac_init(struct ssh_hmac_ctx *ctx, const void *key, size_t klen);
int ssh_hmac_update(struct ssh_hmac_ctx *ctx, const void *m, size_t mlen);
int ssh_hmac_update_buffer(struct ssh_hmac_ctx *ctx, buffer_t *b);
int ssh_hmac_final(struct ssh_hmac_ctx *ctx, u_char *d, size_t dlen);
void ssh_hmac_free(struct ssh_hmac_ctx *ctx);

#endif /* HMAC_H */
