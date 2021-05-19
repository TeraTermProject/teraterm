/*
 * (C) 2011- TeraTerm Project
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

#ifndef KEX_H
#define KEX_H

#include "ttxssh.h"

// クライアントからサーバへの提案事項
enum kex_init_proposals {
	PROPOSAL_KEX_ALGS,
	PROPOSAL_SERVER_HOST_KEY_ALGS,
	PROPOSAL_ENC_ALGS_CTOS,
	PROPOSAL_ENC_ALGS_STOC,
	PROPOSAL_MAC_ALGS_CTOS,
	PROPOSAL_MAC_ALGS_STOC,
	PROPOSAL_COMP_ALGS_CTOS,
	PROPOSAL_COMP_ALGS_STOC,
	PROPOSAL_LANG_CTOS,
	PROPOSAL_LANG_STOC,
	PROPOSAL_MAX
};

#define KEX_DEFAULT_KEX     ""
#define KEX_DEFAULT_PK_ALG  ""
#define KEX_DEFAULT_ENCRYPT ""
#define KEX_DEFAULT_MAC     ""
#define KEX_DEFAULT_COMP    ""
#define KEX_DEFAULT_LANG    ""

extern char *myproposal[PROPOSAL_MAX];

typedef enum {
	KEX_DH_NONE,       /* disabled line */
	KEX_DH_GRP1_SHA1,
	KEX_DH_GRP14_SHA1,
	KEX_DH_GEX_SHA1,
	KEX_DH_GEX_SHA256,
	KEX_ECDH_SHA2_256,
	KEX_ECDH_SHA2_384,
	KEX_ECDH_SHA2_521,
	KEX_DH_GRP14_SHA256,
	KEX_DH_GRP16_SHA512,
	KEX_DH_GRP18_SHA512,
	KEX_DH_UNKNOWN,
	KEX_DH_MAX = KEX_DH_UNKNOWN,
} kex_algorithm;

char* get_kex_algorithm_name(kex_algorithm kextype);
const EVP_MD* get_kex_algorithm_EVP_MD(kex_algorithm kextype);

void normalize_kex_order(char *buf);
kex_algorithm choose_SSH2_kex_algorithm(char *server_proposal, char *my_proposal);
void SSH2_update_kex_myproposal(PTInstVar pvar);


// SSH_MSG_KEY_DH_GEX_REQUEST での min, n, max がとり得る範囲の上限/下限 (RFC 4419)
#define GEX_GRP_LIMIT_MIN   1024
#define GEX_GRP_LIMIT_MAX   8192
// GexMinimalGroupSize が 0 (デフォルト(未設定)) だった時に min に使う値
// RFC 8270 で min の最低値が 2048 に引き上げられたが、互換性の為に GEX_GRP_LIMIT_MIN
// を引き上げるのではなくて、デフォルトの値を変更する
#define GEX_GRP_DEFAULT_MIN 2048

DH *dh_new_group1(void);
DH *dh_new_group14(void);
DH *dh_new_group15(void);
DH *dh_new_group16(void);
DH *dh_new_group17(void);
DH *dh_new_group18(void);
void dh_gen_key(PTInstVar pvar, DH *dh, int we_need /* bytes */ );
int dh_estimate(int bits);

unsigned char *kex_dh_hash(const EVP_MD *evp_md,
                           char *client_version_string,
                           char *server_version_string,
                           char *ckexinit, int ckexinitlen,
                           char *skexinit, int skexinitlen,
                           u_char *serverhostkeyblob, int sbloblen,
                           BIGNUM *client_dh_pub,
                           BIGNUM *server_dh_pub,
                           BIGNUM *shared_secret,
                           unsigned int *hashlen);
unsigned char *kex_dh_gex_hash(const EVP_MD *evp_md,
                               char *client_version_string,
                               char *server_version_string,
                               char *ckexinit, int ckexinitlen,
                               char *skexinit, int skexinitlen,
                               u_char *serverhostkeyblob, int sbloblen,
                               int kexgex_min,
                               int kexgex_bits,
                               int kexgex_max,
                               BIGNUM *kexgex_p,
                               BIGNUM *kexgex_g,
                               BIGNUM *client_dh_pub,
                               BIGNUM *server_dh_pub,
                               BIGNUM *shared_secret,
                               unsigned int *hashlen);
unsigned char *kex_ecdh_hash(const EVP_MD *evp_md,
                             const EC_GROUP *ec_group,
                             char *client_version_string,
                             char *server_version_string,
                             char *ckexinit, int ckexinitlen,
                             char *skexinit, int skexinitlen,
                             u_char *serverhostkeyblob, int sbloblen,
                             const EC_POINT *client_dh_pub,
                             const EC_POINT *server_dh_pub,
                             BIGNUM *shared_secret,
                               unsigned int *hashlen);

int dh_pub_is_valid(DH *dh, BIGNUM *dh_pub);
void kex_derive_keys(PTInstVar pvar, int need, u_char *hash, BIGNUM *shared_secret,
                     char *session_id, int session_id_len);

#endif				/* KEX_H */
