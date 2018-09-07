/*
 * (C) 2011-2017 TeraTerm Project
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

#ifndef __KEY_H_
#define __KEY_H_

#include "ttxssh.h"
#include "ed25519_crypto_api.h"

#define	ED25519_SK_SZ	crypto_sign_ed25519_SECRETKEYBYTES
#define	ED25519_PK_SZ	crypto_sign_ed25519_PUBLICKEYBYTES

int key_verify(Key *key,
               unsigned char *signature, unsigned int signaturelen,
               unsigned char *data, unsigned int datalen);
RSA *duplicate_RSA(RSA *src);
DSA *duplicate_DSA(DSA *src);
unsigned char *duplicate_ED25519_PK(unsigned char *src);
BOOL key_copy(Key *dest, Key *src);

char *key_fingerprint_raw(Key *k, digest_algorithm dgst_alg, int *dgst_raw_length);
char *key_fingerprint(Key *key, enum fp_rep dgst_rep, digest_algorithm dgst_alg);

const char *ssh_key_type(ssh_keytype type);
char *get_sshname_from_key(Key *key);
ssh_keytype get_keytype_from_name(char *name);
char *curve_keytype_to_name(ssh_keytype type);
ssh_keytype key_curve_name_to_keytype(char *name);

Key *key_new_private(int type);
Key *key_new(int type);
void key_free(Key *key);
void key_init(Key *key);
int key_to_blob(Key *key, char **blobp, int *lenp);
Key *key_from_blob(char *data, int blen);
BOOL get_SSH2_publickey_blob(PTInstVar pvar, buffer_t **blobptr, int *bloblen);
BOOL generate_SSH2_keysign(Key *keypair, char **sigptr, int *siglen, char *data, int datalen);

int kextype_to_cipher_nid(kex_algorithm type);
int keytype_to_hash_nid(ssh_keytype type);
int keytype_to_cipher_nid(ssh_keytype type);
ssh_keytype nid_to_keytype(int nid);

void key_private_serialize(Key *key, buffer_t *b);
Key *key_private_deserialize(buffer_t *blob);

int key_ec_validate_private(EC_KEY *key);
int key_ec_validate_public(const EC_GROUP *group, const EC_POINT *public);

int update_client_input_hostkeys(PTInstVar pvar, char *dataptr, int datalen);

#endif
