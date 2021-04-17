/* Imported from OpenSSH-8.5p1, TeraTerm Project */

/* $OpenBSD: cipher.h,v 1.44 2014/01/25 10:12:50 dtucker Exp $ */

/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef CIPHER_H
#define CIPHER_H

typedef unsigned int u_int;
typedef unsigned char u_char;

#include <openssl/evp.h>
/*
 * Cipher types for SSH-1.  New types can be added, but old types should not
 * be removed for compatibility.  The maximum allowed value is 31.
 */
#define SSH_CIPHER_SSH2		-3
#define SSH_CIPHER_ILLEGAL	-2	/* No valid cipher selected. */
#define SSH_CIPHER_NOT_SET	-1	/* None selected (invalid number). */
//#define SSH_CIPHER_NONE		0	/* no encryption */
//#define SSH_CIPHER_IDEA		1	/* IDEA CFB */
//#define SSH_CIPHER_DES		2	/* DES CBC */
//#define SSH_CIPHER_3DES		3	/* 3DES CBC */
//#define SSH_CIPHER_BROKEN_TSS	4	/* TRI's Simple Stream encryption CBC */
//#define SSH_CIPHER_BROKEN_RC4	5	/* Alleged RC4 */
//#define SSH_CIPHER_BLOWFISH	6
//#define SSH_CIPHER_RESERVED	7

#define CIPHER_ENCRYPT		1
#define CIPHER_DECRYPT		0


typedef enum {
	// SSH1
	SSH_CIPHER_NONE, SSH_CIPHER_IDEA, SSH_CIPHER_DES, SSH_CIPHER_3DES,
	SSH_CIPHER_TSS, SSH_CIPHER_RC4, SSH_CIPHER_BLOWFISH,
	// SSH2
	SSH2_CIPHER_3DES_CBC, SSH2_CIPHER_AES128_CBC,
	SSH2_CIPHER_AES192_CBC, SSH2_CIPHER_AES256_CBC,
	SSH2_CIPHER_BLOWFISH_CBC, SSH2_CIPHER_AES128_CTR,
	SSH2_CIPHER_AES192_CTR, SSH2_CIPHER_AES256_CTR,
	SSH2_CIPHER_ARCFOUR, SSH2_CIPHER_ARCFOUR128, SSH2_CIPHER_ARCFOUR256,
	SSH2_CIPHER_CAST128_CBC,
	SSH2_CIPHER_3DES_CTR, SSH2_CIPHER_BLOWFISH_CTR, SSH2_CIPHER_CAST128_CTR,
	SSH2_CIPHER_CAMELLIA128_CBC, SSH2_CIPHER_CAMELLIA192_CBC, SSH2_CIPHER_CAMELLIA256_CBC,
	SSH2_CIPHER_CAMELLIA128_CTR, SSH2_CIPHER_CAMELLIA192_CTR, SSH2_CIPHER_CAMELLIA256_CTR,
	SSH2_CIPHER_AES128_GCM, SSH2_CIPHER_AES256_GCM, SSH2_CIPHER_CHACHAPOLY,
	SSH_CIPHER_MAX = SSH2_CIPHER_CHACHAPOLY,
} SSHCipherId;

struct ssh2cipher {
	SSHCipherId id;
	char *name;
	u_int block_size;
	u_int key_len;
	u_int discard_len;
	u_int iv_len;
	u_int auth_len;
	const EVP_CIPHER *(*func)(void);
};

struct sshcipher_ctx {
	// TTSSH では SSH_CIPHER_NONE が無効なので、plaintext は使用されない
	// int	plaintext;
	// TTSSH では CRYPT_encrypt_aead(), CRYPT_decrypt_aead() が別れていて encrypt で切り替えないので使用されない
	// int	encrypt;
	EVP_CIPHER_CTX *evp;
	// struct chachapoly_ctx *cp_ctx;
	// OpenSSH で ifndef WITH_OPENSSL の時に使用されるものなので、ac_ctx は使用されない
	// aesctr_ctx ac_ctx; /* XXX union with evp? */
	// OpenSSH では const struct sshcipher *cipher;
	const struct ssh2cipher *cipher;
};


int get_cipher_id(const struct ssh2cipher *cipher);
u_int get_cipher_block_size(const struct ssh2cipher *cipher);
u_int get_cipher_key_len(const struct ssh2cipher *cipher);
u_int get_cipher_discard_len(const struct ssh2cipher *cipher);
u_int get_cipher_iv_len(const struct ssh2cipher *cipher);
u_int get_cipher_auth_len(const struct ssh2cipher *cipher);
const EVP_CIPHER *get_cipher_EVP_CIPHER(const struct ssh2cipher *cipher);
char *get_cipher_string(const struct ssh2cipher *cipher);
const struct ssh2cipher* get_cipher_by_name(char *name);
char *get_cipher_name(int cipher_id);
char *get_listbox_cipher_name(int cipher_id, PTInstVar pvar);

void normalize_cipher_order(char *buf);
const struct ssh2cipher *choose_SSH2_cipher_algorithm(char *server_proposal, char *my_proposal);
void SSH2_update_cipher_myproposal(PTInstVar pvar);

void cipher_init_SSH2(
	EVP_CIPHER_CTX *evp,
	const u_char *key, u_int keylen,
	const u_char *iv, u_int ivlen,
	int encrypt,
	const EVP_CIPHER *type,
	int discard_len,
	unsigned int authlen,
	PTInstVar pvar
);
void cipher_free_SSH2(EVP_CIPHER_CTX *evp);

#endif				/* CIPHER_H */
