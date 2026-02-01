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

#ifndef HOSTKEY_H
#define HOSTKEY_H

#include "digest.h"

typedef struct Key Key;

typedef enum {
	KEY_NONE,
	KEY_RSA1,
	KEY_RSA,
	KEY_DSA,
	KEY_ECDSA256,
	KEY_ECDSA384,
	KEY_ECDSA521,
	KEY_ED25519,
	KEY_UNSPEC,
	KEY_MAX = KEY_UNSPEC,
} ssh_keytype;

typedef enum {
	KEY_ALGO_NONE,
	KEY_ALGO_RSA1,
	KEY_ALGO_RSA,
	KEY_ALGO_DSA,
	KEY_ALGO_ECDSA256,
	KEY_ALGO_ECDSA384,
	KEY_ALGO_ECDSA521,
	KEY_ALGO_ED25519,
	KEY_ALGO_RSASHA256,
	KEY_ALGO_RSASHA512,
	KEY_ALGO_UNSPEC,
	KEY_ALGO_MAX = KEY_ALGO_UNSPEC,
} ssh_keyalgo;

typedef enum {
	SSH_AGENT_SIGN_DEFAULT = 0,
	SSH_AGENT_RSA_SHA2_256 = 2,
	SSH_AGENT_RSA_SHA2_512 = 4,
} ssh_agentflag;
#define isFixedLengthKey(type)	((type) >= KEY_DSA && (type) <= KEY_ED25519)

// fingerprintの種別
typedef enum {
	SSH_FP_DEFAULT = 0,
	SSH_FP_HEX,
	SSH_FP_BASE64,
	SSH_FP_BUBBLEBABBLE,
	SSH_FP_RANDOMART
} fp_rep;

/*
enum fp_type {
	SSH_FP_MD5,
	SSH_FP_SHA1,
	SSH_FP_SHA256
};
*/

typedef enum {
	RSA_PUBKEY_SIGN_ALGO_NONE,
	RSA_PUBKEY_SIGN_ALGO_RSA,
	RSA_PUBKEY_SIGN_ALGO_RSASHA256,
	RSA_PUBKEY_SIGN_ALGO_RSASHA512,
	RSA_PUBKEY_SIGN_ALGO_MAX,
} ssh_rsapubkeysignalgo;


ssh_keytype get_hostkey_type_from_name(char *name);
char* get_ssh2_hostkey_type_name(ssh_keytype type);
char *get_ssh2_hostkey_type_name_from_key(Key *key);
ssh_keyalgo get_ssh2_hostkey_algorithm_from_name(const char *name);
char* get_ssh2_hostkey_algorithm_name(ssh_keyalgo algo);
int get_ssh2_key_hashtype(ssh_keyalgo algo);
digest_algorithm get_ssh2_key_hash_alg(ssh_keyalgo algo);
int get_ssh2_agent_flag(ssh_keyalgo algo);
ssh_keytype get_ssh2_hostkey_type_from_algorithm(ssh_keyalgo algo);
const char* get_ssh2_hostkey_type_name_from_algorithm(ssh_keyalgo algo);

void normalize_host_key_order(char *buf);
void normalize_rsa_pubkey_sign_algo_order(char *buf);
ssh_keyalgo choose_SSH2_host_key_algorithm(char *server_proposal, char *my_proposal);
ssh_keyalgo choose_SSH2_keysign_algorithm(PTInstVar pvar, ssh_keytype keytype);
void SSH2_update_host_key_myproposal(PTInstVar pvar);
char* get_ssh2_hostkey_algorithm_digest_name(ssh_keyalgo algo);

#endif /* SSHCMAC_H */
