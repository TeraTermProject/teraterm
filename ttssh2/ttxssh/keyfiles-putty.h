/*
 * (C) 2021- TeraTerm Project
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

#ifndef __KEYFILES_PUTTY_H
#define __KEYFILES_PUTTY_H

#include "argon2.h"

typedef struct ppk_argon2_parameters {
	argon2_type type;
	uint32_t argon2_mem;
	uint32_t argon2_passes;
	uint32_t argon2_parallelism;
	const uint8_t *salt;
	size_t saltlen;
} ppk_argon2_parameters;

BOOL str_to_uint32_t(const char *s, uint32_t *out);
BOOL ppk_read_header(FILE * fp, char *header);
char *ppk_read_body(FILE * fp);
BOOL ppk_read_blob(FILE* fp, int nlines, buffer_t *blob);

void hmac_sha1_simple(unsigned char *key, int keylen, void *data, int datalen,
                      unsigned char *output);
void hmac_sha256_simple(unsigned char *key, int keylen, void *data, int datalen,
                        unsigned char *output);
void mac_simple(const EVP_MD *md,
                unsigned char *key, int keylen, void *data, int datalen,
                unsigned char *output);

void ssh2_ppk_derive_keys(
	unsigned fmt_version, const struct ssh2cipher* ciphertype,
	unsigned char *passphrase, buffer_t *storage,
	unsigned char **cipherkey, unsigned int *cipherkey_len,
	unsigned char **cipheriv, unsigned int *cipheriv_len,
	unsigned char **mackey, unsigned int *mackey_len,
	ppk_argon2_parameters *params);

#endif /* __KEYFILES_PUTTY_H */
