/* Imported from PuTTY 0.74, 0.75, TeraTerm Project */

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

#include "ttxssh.h"
#include "keyfiles-putty.h"

// from sshpubk.c (ver 0.75)
BOOL str_to_uint32_t(const char *s, uint32_t *out)
{
	char *endptr;
	unsigned long converted = strtoul(s, &endptr, 10);
	if (*s && !*endptr && converted <= ~(uint32_t)0) {
		*out = converted;
		return TRUE;
	} else {
		return FALSE;
	}
}

// from sshpubk.c (ver 0.74)
BOOL ppk_read_header(FILE * fp, char *header)
{
	int len = 39;
	int c;

	while (1) {
		c = fgetc(fp);
		if (c == '\n' || c == '\r' || c == EOF)
			return FALSE;
		if (c == ':') {
			c = fgetc(fp);
			if (c != ' ')
				return FALSE;
			*header = '\0';
			return TRUE;
		}
		if (len == 0)
			return FALSE;
		*header++ = c;
		len--;
	}
	return FALSE;
}

// from sshpubk.c (ver 0.74)
char *ppk_read_body(FILE * fp)
{
	buffer_t *buf = buffer_init();

	while (1) {
		int c = fgetc(fp);
		if (c == '\r' || c == '\n' || c == EOF) {
			if (c != EOF) {
				c = fgetc(fp);
				if (c != '\r' && c != '\n')
					ungetc(c, fp);
			}
			return buffer_ptr(buf);
		}
		buffer_put_char(buf, c);
	}
}

// from sshpubk.c (ver 0.74), and modified
// - use buffer_t insted of strbuf
// - use OpenSSL function
BOOL ppk_read_blob(FILE* fp, int nlines, buffer_t *blob)
{
	BIO *bmem, *b64, *chain;
	int i, len;
	char line[200], buf[100];

	b64 = BIO_new(BIO_f_base64());
	bmem = BIO_new(BIO_s_mem());
	for (i=0; i<nlines && fgets(line, sizeof(line), fp)!=NULL; i++) {
		BIO_write(bmem, line, strlen(line));
	}
	BIO_flush(bmem);
	chain = BIO_push(b64, bmem);
	BIO_set_mem_eof_return(chain, 0);
	while ((len = BIO_read(chain, buf, sizeof(buf))) > 0) {
		buffer_append(blob, buf, len);
	}
	BIO_free_all(chain);

	return TRUE;
}

// from sshsha.c (ver 0.70), and modifled
// - use OpenSSL function
void hmac_sha1_simple(unsigned char *key, int keylen, void *data, int datalen,
                      unsigned char *output)
{
	EVP_MD_CTX *ctx[2] = {0, 0};
	unsigned char intermediate[20];
	unsigned char foo[64];
	const EVP_MD *md = EVP_sha1();
	int i;
	unsigned int len;

	ctx[0] = EVP_MD_CTX_new();
	if (ctx[0] == NULL) {
		return;
	}
	ctx[1] = EVP_MD_CTX_new();
	if (ctx[1] == NULL) {
		EVP_MD_CTX_free(ctx[0]);
		return;
	}

	memset(foo, 0x36, sizeof(foo));
	for (i = 0; i < keylen && i < sizeof(foo); i++) {
		foo[i] ^= key[i];
	}
	EVP_DigestInit(ctx[0], md);
	EVP_DigestUpdate(ctx[0], foo, sizeof(foo));

	memset(foo, 0x5C, sizeof(foo));
	for (i = 0; i < keylen && i < sizeof(foo); i++) {
		foo[i] ^= key[i];
	}
	EVP_DigestInit(ctx[1], md);
	EVP_DigestUpdate(ctx[1], foo, sizeof(foo));

	memset(foo, 0, sizeof(foo));

	EVP_DigestUpdate(ctx[0], data, datalen);
	EVP_DigestFinal(ctx[0], intermediate, &len);

	EVP_DigestUpdate(ctx[1], intermediate, sizeof(intermediate));
	EVP_DigestFinal(ctx[1], output, &len);

	EVP_MD_CTX_free(ctx[0]);
	EVP_MD_CTX_free(ctx[1]);
}

void hmac_sha256_simple(unsigned char *key, int keylen, void *data, int datalen,
                        unsigned char *output)
{
	EVP_MD_CTX *ctx[2] = {0, 0};
	unsigned char intermediate[32];
	unsigned char foo[64];
	const EVP_MD *md = EVP_sha256();
	int i;
	unsigned int len;

	ctx[0] = EVP_MD_CTX_new();
	if (ctx[0] == NULL) {
		return;
	}
	ctx[1] = EVP_MD_CTX_new();
	if (ctx[1] == NULL) {
		EVP_MD_CTX_free(ctx[0]);
		return;
	}

	memset(foo, 0x36, sizeof(foo));
	for (i = 0; i < keylen && i < sizeof(foo); i++) {
		foo[i] ^= key[i];
	}
	EVP_DigestInit(ctx[0], md);
	EVP_DigestUpdate(ctx[0], foo, sizeof(foo));

	memset(foo, 0x5C, sizeof(foo));
	for (i = 0; i < keylen && i < sizeof(foo); i++) {
		foo[i] ^= key[i];
	}
	EVP_DigestInit(ctx[1], md);
	EVP_DigestUpdate(ctx[1], foo, sizeof(foo));

	memset(foo, 0, sizeof(foo));

	EVP_DigestUpdate(ctx[0], data, datalen);
	EVP_DigestFinal(ctx[0], intermediate, &len);

	EVP_DigestUpdate(ctx[1], intermediate, sizeof(intermediate));
	EVP_DigestFinal(ctx[1], output, &len);

	EVP_MD_CTX_free(ctx[0]);
	EVP_MD_CTX_free(ctx[1]);
}

// from sshsha.c (ver 0.70) hmac_sha1_simple
//      sshauxcrypt.c (ver 0.75) mac_simple, and modifled
// - use OpenSSL function
// - use EVP_MD instead of ssh2_macalg
void mac_simple(const EVP_MD *md,
                unsigned char *key, int keylen, void *data, int datalen,
                unsigned char *output)
{
	EVP_MD_CTX *ctx[2] = {0, 0};
	unsigned char intermediate[32]; // sha1: 160bit / sha256: 256bit
	unsigned char foo[64]; // block size ... sha1: 512bit / sha256: 512bit
	int i;
	unsigned int len;

	ctx[0] = EVP_MD_CTX_new();
	if (ctx[0] == NULL) {
		return;
	}
	ctx[1] = EVP_MD_CTX_new();
	if (ctx[1] == NULL) {
		EVP_MD_CTX_free(ctx[0]);
		return;
	}

	memset(foo, 0x36, sizeof(foo));
	for (i = 0; i < keylen && i < sizeof(foo); i++) {
		foo[i] ^= key[i];
	}
	EVP_DigestInit(ctx[0], md);
	EVP_DigestUpdate(ctx[0], foo, sizeof(foo));

	memset(foo, 0x5C, sizeof(foo));
	for (i = 0; i < keylen && i < sizeof(foo); i++) {
		foo[i] ^= key[i];
	}
	EVP_DigestInit(ctx[1], md);
	EVP_DigestUpdate(ctx[1], foo, sizeof(foo));

	memset(foo, 0, sizeof(foo));

	EVP_DigestUpdate(ctx[0], data, datalen);
	EVP_DigestFinal(ctx[0], intermediate, &len);

	EVP_DigestUpdate(ctx[1], intermediate, EVP_MD_size(md));
	EVP_DigestFinal(ctx[1], output, &len);

	EVP_MD_CTX_free(ctx[0]);
	EVP_MD_CTX_free(ctx[1]);
}

// from sshpubk.c (ver 0.75), and modifled
// - delete unnecessary paramters
// - use char ** and int * instead of ptrlen
// - use buffer_t instead of strbuf
// - use OpenSSL function
// - use argon2 function
void ssh2_ppk_derive_keys(
	unsigned fmt_version, const struct ssh2cipher* ciphertype,
	unsigned char *passphrase, buffer_t *storage,
	unsigned char **cipherkey, unsigned int *cipherkey_len,
	unsigned char **cipheriv, unsigned int *cipheriv_len,
	unsigned char **mackey, unsigned int *mackey_len,
	ppk_argon2_parameters *params)
{
	size_t mac_keylen = 0;
	u_int ivlen;
	unsigned int cipherkey_offset = 0;

	ivlen = (ciphertype->iv_len == 0) ? ciphertype->block_size : ciphertype->iv_len;

	switch (fmt_version) {
		case 3: {
			uint32_t taglen;
			unsigned char *tag;

			if (ciphertype->key_len == 0) {
				mac_keylen = 0;
				break;
			}
			mac_keylen = 32;
			taglen = ciphertype->key_len + ivlen + mac_keylen;
			tag = (char *)malloc(taglen);

			argon2_hash(params->argon2_passes, params->argon2_mem,
			            params->argon2_parallelism,
			            passphrase, strlen(passphrase),
			            params->salt, params->saltlen,
			            tag, taglen,
			            NULL, 0,
			            params->type, 0x13);
			buffer_append(storage, tag, taglen);

			free(tag);

			break;
		}
		case 2: {
			unsigned ctr;
			const EVP_MD *md = EVP_sha1();
			EVP_MD_CTX *ctx = NULL;
			unsigned char u[4], buf[20]; // SHA1: 20byte
			unsigned int i, len, cipherkey_write_byte = 0;

			ctx = EVP_MD_CTX_new();

			/* Counter-mode iteration to generate cipher key data. */
			for (ctr = 0; ctr * 20 < ciphertype->key_len; ctr++) {
				EVP_DigestInit(ctx, md);
				set_uint32_MSBfirst(u, ctr);
				EVP_DigestUpdate(ctx, u, 4);
				EVP_DigestUpdate(ctx, passphrase, strlen(passphrase));
				EVP_DigestFinal(ctx, buf, &len);
				buffer_append(storage, buf, 20);
				cipherkey_write_byte += 20;
			}
			// TTSSH ‚Ì buffer_t ‚É‚Í shrink ‚·‚éŠÖ”‚ª‚È‚¢‚Ì‚ÅA
			// shrink ‚¹‚¸‚É 40byte ‚Ì‚¤‚¿ 32byte ‚¾‚¯‚ðŽg‚¤
			cipherkey_offset = cipherkey_write_byte - ciphertype->key_len;

			/* In this version of the format, the CBC IV was always all 0. */
			for (i = 0; i < ivlen; i++) {
				buffer_put_char(storage, 0);
			}

			/* Completely separate hash for the MAC key. */
			EVP_DigestInit(ctx, md);
			mac_keylen = EVP_MD_size(md); // SHA1: 20byte
			EVP_DigestUpdate(ctx, "putty-private-key-file-mac-key", 30);
			EVP_DigestUpdate(ctx, passphrase, strlen(passphrase));
			EVP_DigestFinal(ctx, buf, &len);
			buffer_append(storage, buf, mac_keylen);

			EVP_MD_CTX_free(ctx);

			break;
		}
	}

	*cipherkey = storage->buf;
	*cipherkey_len = ciphertype->key_len;
	*cipheriv = storage->buf + ciphertype->key_len + cipherkey_offset;
	*cipheriv_len = ivlen;
	*mackey = storage->buf + ciphertype->key_len + cipherkey_offset + ivlen;
	*mackey_len = mac_keylen;
}
