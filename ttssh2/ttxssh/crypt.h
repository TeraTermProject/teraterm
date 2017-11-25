/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (C) 2004-2017 TeraTerm Project
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
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#ifndef __CRYPT_H
#define __CRYPT_H

#include <openssl/rsa.h>
#include <openssl/des.h>
#include <openssl/blowfish.h>

#define SSH_SESSION_KEY_LENGTH    32
#define SSH_RSA_CHALLENGE_LENGTH  32
#define SSH_COOKIE_LENGTH         8
#define SSH2_COOKIE_LENGTH         16

#define CRYPT_KEY_LENGTH     32
#define COOKIE_LENGTH        16

typedef struct {
  DES_key_schedule k1;
  DES_key_schedule k2;
  DES_key_schedule k3;
  DES_cblock ivec1;
  DES_cblock ivec2;
  DES_cblock ivec3;
} Cipher3DESState;

typedef struct {
  DES_key_schedule k;
  DES_cblock ivec;
} CipherDESState;

typedef struct {
  BF_KEY k;
  unsigned char ivec[8];
} CipherBlowfishState;

typedef struct {
  uint32 *h;
  uint32 n;
} CRYPTDetectAttack;

typedef struct {
  RSA *RSA_key;
} CRYPTPublicKey;

typedef union {
  Cipher3DESState c3DES;
  CipherDESState cDES;
  CipherBlowfishState cBlowfish;
} CRYPTCipherState;

typedef void (* CRYPTCryptFun)(PTInstVar pvar, unsigned char *buf, int bytes);

typedef struct {
  CRYPTDetectAttack detect_attack_statics;

  CRYPTPublicKey server_key;
  CRYPTPublicKey host_key;

  char server_cookie[COOKIE_LENGTH];
  char client_cookie[COOKIE_LENGTH];

  int supported_sender_ciphers;
  int supported_receiver_ciphers;
  int sender_cipher;
  int receiver_cipher;
  char sender_cipher_key[CRYPT_KEY_LENGTH];
  char receiver_cipher_key[CRYPT_KEY_LENGTH];
  CRYPTCryptFun encrypt;
  CRYPTCryptFun decrypt;
  CRYPTCipherState enc;
  CRYPTCipherState dec;
} CRYPTState;

void CRYPT_init(PTInstVar pvar);
/* this function is called during 'slack time' while we wait for a response
   from the server. Therefore we have some time available to do some
   moderately expensive computations. */
void CRYPT_initialize_random_numbers(PTInstVar pvar);
void CRYPT_set_random_data(PTInstVar pvar, unsigned char *buf, int bytes);
void CRYPT_end(PTInstVar pvar);

void CRYPT_get_cipher_info(PTInstVar pvar, char *dest, int len);
void CRYPT_get_server_key_info(PTInstVar pvar, char *dest, int len);

void CRYPT_set_server_cookie(PTInstVar pvar, unsigned char *cookie);
void CRYPT_set_client_cookie(PTInstVar pvar, unsigned char *cookie);
#define CRYPT_get_server_cookie(pvar) ((pvar)->crypt_state.server_cookie)

void CRYPT_free_public_key(CRYPTPublicKey *key);

BOOL CRYPT_set_server_RSA_key(PTInstVar pvar,
  int bits, unsigned char *exp, unsigned char *mod);
BOOL CRYPT_set_host_RSA_key(PTInstVar pvar,
  int bits, unsigned char *exp, unsigned char *mod);
unsigned int CRYPT_get_encrypted_session_key_len(PTInstVar pvar);
int CRYPT_choose_session_key(PTInstVar pvar, unsigned char *encrypted_key_buf);
BOOL CRYPT_start_encryption(PTInstVar pvar, int sender_flag, int receiver_flag);
int CRYPT_generate_RSA_challenge_response(PTInstVar pvar, unsigned char *challenge,
                                           int challenge_len, unsigned char *response);

unsigned int CRYPT_get_receiver_MAC_size(PTInstVar pvar);
BOOL CRYPT_verify_receiver_MAC(PTInstVar pvar, uint32 sequence_number,
  char *data, int len, char *MAC);
unsigned int CRYPT_get_sender_MAC_size(PTInstVar pvar);

BOOL CRYPT_build_sender_MAC(PTInstVar pvar, uint32 sequence_number,
  char *data, int len, char *MAC);

BOOL CRYPT_set_supported_ciphers(PTInstVar pvar, int sender_ciphers, int receiver_ciphers);
BOOL CRYPT_choose_ciphers(PTInstVar pvar);
#define CRYPT_get_sender_cipher(pvar) ((pvar)->crypt_state.sender_cipher)
#define CRYPT_get_receiver_cipher(pvar) ((pvar)->crypt_state.receiver_cipher)
unsigned int CRYPT_get_decryption_block_size(PTInstVar pvar);
unsigned int CRYPT_get_encryption_block_size(PTInstVar pvar);
#define CRYPT_encrypt(pvar, buf, bytes) \
    ((pvar)->crypt_state.encrypt((pvar), (buf), (bytes)))
#define CRYPT_decrypt(pvar, buf, bytes) \
    ((pvar)->crypt_state.decrypt((pvar), (buf), (bytes)))

BOOL CRYPT_detect_attack(PTInstVar pvar, unsigned char *buf, int bytes);
int CRYPT_passphrase_decrypt(int cipher, char *passphrase, char *buf, int len);
RSA *make_key(PTInstVar pvar,
                  int bits, unsigned char *exp,
                  unsigned char *mod);

#endif
