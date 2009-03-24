/*
Copyright (c) 1998-2001, Robert O'Callahan
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.

The name of Robert O'Callahan may not be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#ifndef __CRYPT_H
#define __CRYPT_H

#include <openssl/rsa.h>
#include <openssl/des.h>
#include <openssl/idea.h>
#include <openssl/rc4.h>
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
  IDEA_KEY_SCHEDULE k;
  unsigned char ivec[8];
} CipherIDEAState;

typedef struct {
  DES_key_schedule k;
  DES_cblock ivec;
} CipherDESState;

typedef struct {
  RC4_KEY k;
} CipherRC4State;

typedef struct {
  BF_KEY k;
  unsigned char ivec[8];
} CipherBlowfishState;

typedef struct {
  uint32 FAR * h;
  uint32 n;
} CRYPTDetectAttack;

typedef struct {
  RSA FAR * RSA_key;
} CRYPTPublicKey;

typedef struct _CRYPTKeyPair {
  RSA FAR * RSA_key;
  DSA *DSA_key;
} CRYPTKeyPair;

typedef union {
  Cipher3DESState c3DES;
  CipherIDEAState cIDEA;
  CipherDESState cDES;
  CipherRC4State cRC4;
  CipherBlowfishState cBlowfish;
} CRYPTCipherState;

typedef void (* CRYPTCryptFun)(PTInstVar pvar, unsigned char FAR * buf, int bytes);

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
void CRYPT_set_random_data(PTInstVar pvar, unsigned char FAR * buf, int bytes);
void CRYPT_end(PTInstVar pvar);

void CRYPT_get_cipher_info(PTInstVar pvar, char FAR * dest, int len);
void CRYPT_get_server_key_info(PTInstVar pvar, char FAR * dest, int len);

void CRYPT_set_server_cookie(PTInstVar pvar, unsigned char FAR * cookie);
void CRYPT_set_client_cookie(PTInstVar pvar, unsigned char FAR * cookie);
#define CRYPT_get_server_cookie(pvar) ((pvar)->crypt_state.server_cookie)

void CRYPT_free_key_pair(CRYPTKeyPair FAR * key_pair);
void CRYPT_free_public_key(CRYPTPublicKey FAR * key);

BOOL CRYPT_set_server_RSA_key(PTInstVar pvar,
  int bits, unsigned char FAR * exp, unsigned char FAR * mod);
BOOL CRYPT_set_host_RSA_key(PTInstVar pvar,
  int bits, unsigned char FAR * exp, unsigned char FAR * mod);
int CRYPT_get_encrypted_session_key_len(PTInstVar pvar);
int CRYPT_choose_session_key(PTInstVar pvar, unsigned char FAR * encrypted_key_buf);
BOOL CRYPT_start_encryption(PTInstVar pvar, int sender_flag, int receiver_flag);
int CRYPT_generate_RSA_challenge_response(PTInstVar pvar, unsigned char FAR * challenge,
                                           int challenge_len, unsigned char FAR * response);

int CRYPT_get_receiver_MAC_size(PTInstVar pvar);
BOOL CRYPT_verify_receiver_MAC(PTInstVar pvar, uint32 sequence_number,
  char FAR * data, int len, char FAR * MAC);
int CRYPT_get_sender_MAC_size(PTInstVar pvar);

BOOL CRYPT_build_sender_MAC(PTInstVar pvar, uint32 sequence_number,
  char FAR * data, int len, char FAR * MAC);

BOOL CRYPT_set_supported_ciphers(PTInstVar pvar, int sender_ciphers, int receiver_ciphers);
BOOL CRYPT_choose_ciphers(PTInstVar pvar);
#define CRYPT_get_sender_cipher(pvar) ((pvar)->crypt_state.sender_cipher)
#define CRYPT_get_receiver_cipher(pvar) ((pvar)->crypt_state.receiver_cipher)
int CRYPT_get_decryption_block_size(PTInstVar pvar);
int CRYPT_get_encryption_block_size(PTInstVar pvar);
#define CRYPT_encrypt(pvar, buf, bytes) \
    ((pvar)->crypt_state.encrypt((pvar), (buf), (bytes)))
#define CRYPT_decrypt(pvar, buf, bytes) \
    ((pvar)->crypt_state.decrypt((pvar), (buf), (bytes)))

BOOL CRYPT_detect_attack(PTInstVar pvar, unsigned char FAR * buf, int bytes);
int CRYPT_passphrase_decrypt(int cipher, char FAR * passphrase, char FAR * buf, int len);
RSA FAR *make_key(PTInstVar pvar,
						 int bits, unsigned char FAR * exp,
						 unsigned char FAR * mod);

#endif
