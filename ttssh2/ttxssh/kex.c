/*
Copyright (c) 2011, TeraTerm Project
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

#include "ttxssh.h"
#include "kex.h"

extern Newkeys current_keys[MODE_MAX];

static DH *dh_new_group_asc(const char *gen, const char *modulus)
{
	DH *dh = NULL;

	if ((dh = DH_new()) == NULL) {
		printf("dh_new_group_asc: DH_new");
		goto error;
	}

	// PとGは公開してもよい素数の組み合わせ
	if (BN_hex2bn(&dh->p, modulus) == 0) {
		printf("BN_hex2bn p");
		goto error;
	}

	if (BN_hex2bn(&dh->g, gen) == 0) {
		printf("BN_hex2bn g");
		goto error;
	}

	return (dh);

error:
	DH_free(dh);
	return (NULL);
}


DH *dh_new_group1(void)
{
	static char *gen = "2", *group1 =
	    "FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
	    "29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
	    "EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
	    "E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
	    "EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE65381"
	    "FFFFFFFF" "FFFFFFFF";

	return (dh_new_group_asc(gen, group1));
}


DH *dh_new_group14(void)
{
    static char *gen = "2", *group14 =
        "FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
        "29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
        "EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
        "E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
        "EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE45B3D"
        "C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8" "FD24CF5F"
        "83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
        "670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B"
        "E39E772C" "180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9"
        "DE2BCBF6" "95581718" "3995497C" "EA956AE5" "15D22618" "98FA0510"
        "15728E5A" "8AACAA68" "FFFFFFFF" "FFFFFFFF";

	return (dh_new_group_asc(gen, group14));
}


// DH鍵を生成する
void dh_gen_key(PTInstVar pvar, DH *dh, int we_need /* bytes */ )
{
	int i;

	dh->priv_key = NULL;

	// 秘密にすべき乱数(X)を生成
	for (i = 0 ; i < 10 ; i++) { // retry counter
		if (dh->priv_key != NULL) {
			BN_clear_free(dh->priv_key);
		}
		dh->priv_key = BN_new();
		if (dh->priv_key == NULL)
			goto error;
		if (BN_rand(dh->priv_key, 2*(we_need*8), 0, 0) == 0)
			goto error;
		if (DH_generate_key(dh) == 0)
			goto error;
		if (dh_pub_is_valid(dh, dh->pub_key))
			break;
	}
	if (i >= 10) {
		goto error;
	}
	return;

error:;
	notify_fatal_error(pvar, "error occurred @ dh_gen_key()");

}


int dh_estimate(int bits)
{
	if (bits <= 128)
		return (1024);  /* O(2**86) */
	if (bits <= 192)
		return (2048);  /* O(2**116) */
	return (4096);      /* O(2**156) */
}


// SHA-1(160bit)を求める
unsigned char *kex_dh_hash(char *client_version_string,
                           char *server_version_string,
                           char *ckexinit, int ckexinitlen,
                           char *skexinit, int skexinitlen,
                           u_char *serverhostkeyblob, int sbloblen,
                           BIGNUM *client_dh_pub,
                           BIGNUM *server_dh_pub,
                           BIGNUM *shared_secret,
                           unsigned int *hashlen)
{
	buffer_t *b;
	static unsigned char digest[EVP_MAX_MD_SIZE];
	const EVP_MD *evp_md = EVP_sha1();
	EVP_MD_CTX md;

	b = buffer_init();
	buffer_put_string(b, client_version_string, strlen(client_version_string));
	buffer_put_string(b, server_version_string, strlen(server_version_string));

	/* kexinit messages: fake header: len+SSH2_MSG_KEXINIT */
	buffer_put_int(b, ckexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, ckexinit, ckexinitlen);
	buffer_put_int(b, skexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, skexinit, skexinitlen);

	buffer_put_string(b, serverhostkeyblob, sbloblen);
	buffer_put_bignum2(b, client_dh_pub);
	buffer_put_bignum2(b, server_dh_pub);
	buffer_put_bignum2(b, shared_secret);

	// yutaka
	//debug_print(38, buffer_ptr(b), buffer_len(b));

	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, buffer_ptr(b), buffer_len(b));
	EVP_DigestFinal(&md, digest, NULL);

	buffer_free(b);

	//write_buffer_file(digest, EVP_MD_size(evp_md));

	*hashlen = EVP_MD_size(evp_md);

	return digest;
}


// SHA-1(160bit)/SHA-256(256bit)を求める
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
                               unsigned int *hashlen)
{
	buffer_t *b;
	static unsigned char digest[EVP_MAX_MD_SIZE];
	EVP_MD_CTX md;

	b = buffer_init();
	buffer_put_string(b, client_version_string, strlen(client_version_string));
	buffer_put_string(b, server_version_string, strlen(server_version_string));

	/* kexinit messages: fake header: len+SSH2_MSG_KEXINIT */
	buffer_put_int(b, ckexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, ckexinit, ckexinitlen);
	buffer_put_int(b, skexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, skexinit, skexinitlen);

	buffer_put_string(b, serverhostkeyblob, sbloblen);

	// DH group sizeのビット数を加算する
	buffer_put_int(b, kexgex_min);
	buffer_put_int(b, kexgex_bits);
	buffer_put_int(b, kexgex_max);

	// DH鍵の素数と生成元を加算する
	buffer_put_bignum2(b, kexgex_p);
	buffer_put_bignum2(b, kexgex_g);

	buffer_put_bignum2(b, client_dh_pub);
	buffer_put_bignum2(b, server_dh_pub);
	buffer_put_bignum2(b, shared_secret);

	// yutaka
	//debug_print(38, buffer_ptr(b), buffer_len(b));

	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, buffer_ptr(b), buffer_len(b));
	EVP_DigestFinal(&md, digest, NULL);

	buffer_free(b);

	//write_buffer_file(digest, EVP_MD_size(evp_md));

	*hashlen = EVP_MD_size(evp_md);

	return digest;
}


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
                             unsigned int *hashlen)
{
	buffer_t *b;
	static unsigned char digest[EVP_MAX_MD_SIZE];
	EVP_MD_CTX md;

	b = buffer_init();
	buffer_put_string(b, client_version_string, strlen(client_version_string));
	buffer_put_string(b, server_version_string, strlen(server_version_string));

	/* kexinit messages: fake header: len+SSH2_MSG_KEXINIT */
	buffer_put_int(b, ckexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, ckexinit, ckexinitlen);
	buffer_put_int(b, skexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, skexinit, skexinitlen);

	buffer_put_string(b, serverhostkeyblob, sbloblen);

	buffer_put_ecpoint(b, ec_group, client_dh_pub);
	buffer_put_ecpoint(b, ec_group, server_dh_pub);
	buffer_put_bignum2(b, shared_secret);

	// yutaka
	//debug_print(38, buffer_ptr(b), buffer_len(b));

	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, buffer_ptr(b), buffer_len(b));
	EVP_DigestFinal(&md, digest, NULL);

	buffer_free(b);

	//write_buffer_file(digest, EVP_MD_size(evp_md));

	*hashlen = EVP_MD_size(evp_md);

	return digest;
}


int dh_pub_is_valid(DH *dh, BIGNUM *dh_pub)
{
	int i;
	int n = BN_num_bits(dh_pub);
	int bits_set = 0;

	if (dh_pub->neg) {
		//logit("invalid public DH value: negativ");
		return 0;
	}
	for (i = 0; i <= n; i++)
		if (BN_is_bit_set(dh_pub, i))
			bits_set++;
	//debug2("bits set: %d/%d", bits_set, BN_num_bits(dh->p));

	/* if g==2 and bits_set==1 then computing log_g(dh_pub) is trivial */
	if (bits_set > 1 && (BN_cmp(dh_pub, dh->p) == -1))
		return 1;
	//logit("invalid public DH value (%d/%d)", bits_set, BN_num_bits(dh->p));
	return 0;
}


// from openssh 5.8p1 key.c
int key_ec_validate_public(const EC_GROUP *group, const EC_POINT *public)
{
	BN_CTX *bnctx;
	EC_POINT *nq = NULL;
	BIGNUM *order, *x, *y, *tmp;
	int ret = -1;

	if ((bnctx = BN_CTX_new()) == NULL) {
		return ret;
	}
	BN_CTX_start(bnctx);

	/*
	 * We shouldn't ever hit this case because bignum_get_ecpoint()
	 * refuses to load GF2m points.
	 */
	if (EC_METHOD_get_field_type(EC_GROUP_method_of(group)) !=
	    NID_X9_62_prime_field) {
		goto out;
	}

	/* Q != infinity */
	if (EC_POINT_is_at_infinity(group, public)) {
		goto out;
	}

	if ((x = BN_CTX_get(bnctx)) == NULL ||
	    (y = BN_CTX_get(bnctx)) == NULL ||
	    (order = BN_CTX_get(bnctx)) == NULL ||
		(tmp = BN_CTX_get(bnctx)) == NULL) {
		goto out;
	}

	/* log2(x) > log2(order)/2, log2(y) > log2(order)/2 */
	if (EC_GROUP_get_order(group, order, bnctx) != 1) {
		goto out;
	}
	if (EC_POINT_get_affine_coordinates_GFp(group, public,
		x, y, bnctx) != 1) {
		goto out;
	}
	if (BN_num_bits(x) <= BN_num_bits(order) / 2) {
		goto out;
	}
	if (BN_num_bits(y) <= BN_num_bits(order) / 2) {
		goto out;
	}

	/* nQ == infinity (n == order of subgroup) */
	if ((nq = EC_POINT_new(group)) == NULL) {
		goto out;
	}
	if (EC_POINT_mul(group, nq, NULL, public, order, bnctx) != 1) {
		goto out;
	}
	if (EC_POINT_is_at_infinity(group, nq) != 1) {
		goto out;
	}

	/* x < order - 1, y < order - 1 */
	if (!BN_sub(tmp, order, BN_value_one())) {
		goto out;
	}
	if (BN_cmp(x, tmp) >= 0) {
		goto out;
	}
	if (BN_cmp(y, tmp) >= 0) {
		goto out;
	}
	ret = 0;
 out:
	BN_CTX_free(bnctx);
	EC_POINT_free(nq);
	return ret;
}


static u_char *derive_key(int id, int need, u_char *hash, BIGNUM *shared_secret,
                          char *session_id, int session_id_len,
                          const EVP_MD *evp_md)
{
	buffer_t *b;
	EVP_MD_CTX md;
	char c = id;
	int have;
	int mdsz = EVP_MD_size(evp_md);
	u_char *digest = malloc(roundup(need, mdsz));

	if (digest == NULL)
		goto skip;

	b = buffer_init();
	if (b == NULL)
		goto skip;

	buffer_put_bignum2(b, shared_secret);

	/* K1 = HASH(K || H || "A" || session_id) */
	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, buffer_ptr(b), buffer_len(b));
	EVP_DigestUpdate(&md, hash, mdsz);
	EVP_DigestUpdate(&md, &c, 1);
	EVP_DigestUpdate(&md, session_id, session_id_len);
	EVP_DigestFinal(&md, digest, NULL);

	/*
	 * expand key:
	 * Kn = HASH(K || H || K1 || K2 || ... || Kn-1)
	 * Key = K1 || K2 || ... || Kn
	 */
	for (have = mdsz; need > have; have += mdsz) {
		EVP_DigestInit(&md, evp_md);
		EVP_DigestUpdate(&md, buffer_ptr(b), buffer_len(b));
		EVP_DigestUpdate(&md, hash, mdsz);
		EVP_DigestUpdate(&md, digest, have);
		EVP_DigestFinal(&md, digest + have, NULL);
	}
	buffer_free(b);

skip:;
	return digest;
}


void kex_derive_keys(PTInstVar pvar, int need, u_char *hash, BIGNUM *shared_secret,
                     char *session_id, int session_id_len)
{
#define NKEYS	6
	u_char *keys[NKEYS];
	int i, mode, ctos;

	for (i = 0; i < NKEYS; i++) {
		keys[i] = derive_key('A'+i, need, hash, shared_secret, session_id, session_id_len,
		                     get_kex_algorithm_EVP_MD(pvar->kex_type));
		//debug_print(i, keys[i], need);
	}

	for (mode = 0; mode < MODE_MAX; mode++) {
		if (mode == MODE_OUT)
			ctos = 1;
		else
			ctos = 0;

#if 0
		// free already allocated buffer (2004.12.27 yutaka)
		// キー再作成時にMAC corruptとなるので削除。(2005.1.5 yutaka)
		if (current_keys[mode].enc.iv != NULL)
			free(current_keys[mode].enc.iv);
		if (current_keys[mode].enc.key != NULL)
			free(current_keys[mode].enc.key);
		if (current_keys[mode].mac.key != NULL)
			free(current_keys[mode].mac.key);
#endif

		// setting
		current_keys[mode].enc.iv  = keys[ctos ? 0 : 1];
		current_keys[mode].enc.key = keys[ctos ? 2 : 3];
		current_keys[mode].mac.key = keys[ctos ? 4 : 5];

		//debug_print(20 + mode*3, current_keys[mode]->enc.iv, 8);
		//debug_print(21 + mode*3, current_keys[mode]->enc.key, 24);
		//debug_print(22 + mode*3, current_keys[mode]->mac.key, 24);
	}
}
