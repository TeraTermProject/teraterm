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
#include "key.h"
#include "resource.h"
#include "dlglib.h"

#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/ecdsa.h>
#include <openssl/buffer.h>

#define INTBLOB_LEN 20
#define SIGBLOB_LEN (2*INTBLOB_LEN)


struct hostkeys_update_ctx {
	/* The hostname and (optionally) IP address string for the server */
	char *host_str, *ip_str;

	/*
	* Keys received from the server and a flag for each indicating
	* whether they already exist in known_hosts.
	* keys_seen is filled in by hostkeys_find() and later (for new
	* keys) by client_global_hostkeys_private_confirm().
	*/
	Key **keys;
	int *keys_seen;
	size_t nkeys;

	size_t nnew;

	/*
	* Keys that are in known_hosts, but were not present in the update
	* from the server (i.e. scheduled to be deleted).
	* Filled in by hostkeys_find().
	*/
	Key **old_keys;
	size_t nold;
};


//////////////////////////////////////////////////////////////////////////////
//
// Key verify function
//
//////////////////////////////////////////////////////////////////////////////

//
// DSS
//

int ssh_dss_verify(DSA *key,
                   u_char *signature, u_int signaturelen,
                   u_char *data, u_int datalen)
{
	DSA_SIG *sig;
	const EVP_MD *evp_md = EVP_sha1();
	EVP_MD_CTX *md = NULL;
	unsigned char digest[EVP_MAX_MD_SIZE], *sigblob;
	unsigned int len, dlen;
	int ret = -1;
	char *ptr;
	BIGNUM *r, *s;

	md = EVP_MD_CTX_new();
	if (md == NULL) {
		ret = -1;
		goto error;
	}

	OpenSSL_add_all_digests();

	if (key == NULL) {
		ret = -2;
		goto error;
	}

	ptr = signature;

	// step1
	if (signaturelen == 0x28) {
		// workaround for SSH-2.0-2.0* and SSH-2.0-2.1* (2006.11.18 maya)
		ptr -= 4;
	}
	else {
		len = get_uint32_MSBfirst(ptr);
		ptr += 4;
		if (strncmp("ssh-dss", ptr, len) != 0) {
			ret = -3;
			goto error;
		}
		ptr += len;
	}

	// step2
	len = get_uint32_MSBfirst(ptr);
	ptr += 4;
	sigblob = ptr;
	ptr += len;

	if (len != SIGBLOB_LEN) {
		ret = -4;
		goto error;
	}

	/* parse signature */
	if ((sig = DSA_SIG_new()) == NULL) {
		ret = -5;
		goto error;
	}
	if ((r = BN_new()) == NULL) {
		ret = -6;
		goto error;
	}
	if ((s = BN_new()) == NULL) {
		ret = -7;
		goto error;
	}
	DSA_SIG_set0(sig, r, s);
	BN_bin2bn(sigblob, INTBLOB_LEN, r);
	BN_bin2bn(sigblob+ INTBLOB_LEN, INTBLOB_LEN, s);

	/* sha1 the data */
	EVP_DigestInit(md, evp_md);
	EVP_DigestUpdate(md, data, datalen);
	EVP_DigestFinal(md, digest, &dlen);

	ret = DSA_do_verify(digest, dlen, sig, key);
	SecureZeroMemory(digest, sizeof(digest));

	DSA_SIG_free(sig);

error:
	if (md)
		EVP_MD_CTX_free(md);

	return ret;
}


//
// RSA
//

/*
 * See:
 * http://www.rsasecurity.com/rsalabs/pkcs/pkcs-1/
 * ftp://ftp.rsasecurity.com/pub/pkcs/pkcs-1/pkcs-1v2-1.asn
 */

/*
 * id-sha1 OBJECT IDENTIFIER ::= { iso(1) identified-organization(3)
 *	oiw(14) secsig(3) algorithms(2) 26 }
 */
static const u_char id_sha1[] = {
	0x30, 0x21, /* type Sequence, length 0x21 (33) */
	0x30, 0x09, /* type Sequence, length 0x09 */
	0x06, 0x05, /* type OID, length 0x05 */
	0x2b, 0x0e, 0x03, 0x02, 0x1a, /* id-sha1 OID */
	0x05, 0x00, /* NULL */
	0x04, 0x14  /* Octet string, length 0x14 (20), followed by sha1 hash */
};

/*
 * See http://csrc.nist.gov/groups/ST/crypto_apps_infra/csor/algorithms.html
 * id-sha256 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840)
 *	organization(1) gov(101) csor(3) nistAlgorithm(4) hashAlgs(2)
 *	id-sha256(1) }
 */
static const u_char id_sha256[] = {
	0x30, 0x31, /* type Sequence, length 0x31 (49) */
	0x30, 0x0d, /* type Sequence, length 0x0d (13) */
	0x06, 0x09, /* type OID, length 0x09 */
	0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, /* id-sha256 */
	0x05, 0x00, /* NULL */
	0x04, 0x20  /* Octet string, length 0x20 (32), followed by sha256 hash */
};

/*
 * See http://csrc.nist.gov/groups/ST/crypto_apps_infra/csor/algorithms.html
 * id-sha512 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840)
 *	organization(1) gov(101) csor(3) nistAlgorithm(4) hashAlgs(2)
 *	id-sha256(3) }
 */
static const u_char id_sha512[] = {
	0x30, 0x51, /* type Sequence, length 0x51 (81) */
	0x30, 0x0d, /* type Sequence, length 0x0d (13) */
	0x06, 0x09, /* type OID, length 0x09 */
	0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, /* id-sha512 */
	0x05, 0x00, /* NULL */
	0x04, 0x40  /* Octet string, length 0x40 (64), followed by sha512 hash */
};

/*
 * id-md5 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840)
 *	rsadsi(113549) digestAlgorithm(2) 5 }
 */
static const u_char id_md5[] = {
	0x30, 0x20, /* type Sequence, length 0x20 (32) */
	0x30, 0x0c, /* type Sequence, length 0x09 */
	0x06, 0x08, /* type OID, length 0x05 */
	0x2a, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05, /* id-md5 */
	0x05, 0x00, /* NULL */
	0x04, 0x10  /* Octet string, length 0x10 (16), followed by md5 hash */
};

static int openssh_RSA_verify(int type, u_char *hash, u_int hashlen,
                              u_char *sigbuf, u_int siglen, RSA *rsa)
{
	u_int ret, rsasize, oidlen = 0, hlen = 0;
	int len;
	const u_char *oid = NULL;
	u_char *decrypted = NULL;

	ret = 0;
	switch (type) {
	case NID_sha1:
		oid = id_sha1;
		oidlen = sizeof(id_sha1);
		hlen = 20;
		break;
	case NID_md5:
		oid = id_md5;
		oidlen = sizeof(id_md5);
		hlen = 16;
		break;
	case NID_sha256:
		oid = id_sha256;
		oidlen = sizeof(id_sha256);
		hlen = 32;
		break;
	case NID_sha512:
		oid = id_sha512;
		oidlen = sizeof(id_sha512);
		hlen = 64;
		break;
	default:
		goto done;
		break;
	}
	if (hashlen != hlen) {
		//error("bad hashlen");
		goto done;
	}
	rsasize = RSA_size(rsa);
	if (siglen == 0 || siglen > rsasize) {
		//error("bad siglen");
		goto done;
	}
	decrypted = malloc(rsasize);
	if (decrypted == NULL)
		return 0; // error

	if ((len = RSA_public_decrypt(siglen, sigbuf, decrypted, rsa,
	                              RSA_PKCS1_PADDING)) < 0) {
			//error("RSA_public_decrypt failed: %s",
			//    ERR_error_string(ERR_get_error(), NULL));
			goto done;
		}
		if (len != hlen + oidlen) {
			//error("bad decrypted len: %d != %d + %d", len, hlen, oidlen);
			goto done;
		}
		if (memcmp(decrypted, oid, oidlen) != 0) {
			//error("oid mismatch");
			goto done;
		}
		if (memcmp(decrypted + oidlen, hash, hlen) != 0) {
			//error("hash mismatch");
			goto done;
		}
		ret = 1;
done:
		if (decrypted)
			free(decrypted);
		return ret;
}

int ssh_rsa_verify(RSA *key,
                   u_char *signature, u_int signaturelen,
                   u_char *data, u_int datalen, ssh_keyalgo keyalgo)
{
	const EVP_MD *evp_md;
	EVP_MD_CTX *md = NULL;
	u_char digest[EVP_MAX_MD_SIZE], *sigblob;
	u_int len, dlen, modlen;
	int ret = -1, nid;
	char *ptr, *algo_name;
	BIGNUM *n;

	md = EVP_MD_CTX_new();
	if (md == NULL) {
		ret = -1;
		goto error;
	}

	OpenSSL_add_all_digests();

	if (key == NULL) {
		ret = -2;
		goto error;
	}

	RSA_get0_key(key, &n, NULL, NULL);
	if (BN_num_bits(n) < SSH_RSA_MINIMUM_MODULUS_SIZE) {
		ret = -3;
		goto error;
	}
	//debug_print(41, signature, signaturelen);
	ptr = signature;
	algo_name = get_ssh2_hostkey_algorithm_name(keyalgo);

	// step1
	len = get_uint32_MSBfirst(ptr);
	ptr += 4;
	if (strncmp(algo_name, ptr, len) != 0) {
		logprintf(10, "%s: signature type mismatch: sig: %s, hostkey: %s", __FUNCTION__, ptr, algo_name);
		ret = -4;
		goto error;
	}
	ptr += len;

	// step2
	len = get_uint32_MSBfirst(ptr);
	ptr += 4;
	sigblob = ptr;
	ptr += len;
#if 0
	rlen = get_uint32_MSBfirst(ptr);
	if (rlen != 0) {
		return -1;
	}
#endif

	/* RSA_verify expects a signature of RSA_size */
	modlen = RSA_size(key);
	if (len > modlen) {
		ret = -5;
		goto error;

	} else if (len < modlen) {
		u_int diff = modlen - len;
		sigblob = realloc(sigblob, modlen);
		memmove(sigblob + diff, sigblob, len);
		memset(sigblob, 0, diff);
		len = modlen;
	}

	nid = get_ssh2_key_hashtype(keyalgo);
	if ((evp_md = EVP_get_digestbynid(nid)) == NULL) {
		logprintf(10, "%s: EVP_get_digestbynid %d failed", __FUNCTION__, nid);
		ret = -6;
		goto error;
	}
	EVP_DigestInit(md, evp_md);
	EVP_DigestUpdate(md, data, datalen);
	EVP_DigestFinal(md, digest, &dlen);

	ret = openssh_RSA_verify(nid, digest, dlen, sigblob, len, key);

	SecureZeroMemory(digest, sizeof(digest));
	SecureZeroMemory(sigblob, len);
	//free(sigblob);
	//debug("ssh_rsa_verify: signature %scorrect", (ret==0) ? "in" : "");

error:
	if (md)
		EVP_MD_CTX_free(md);

	return ret;
}

int ssh_ecdsa_verify(EC_KEY *key, ssh_keytype keytype,
                     u_char *signature, u_int signaturelen,
                     u_char *data, u_int datalen)
{
	ECDSA_SIG *sig;
	const EVP_MD *evp_md;
	EVP_MD_CTX *md = NULL;
	unsigned char digest[EVP_MAX_MD_SIZE], *sigblob;
	unsigned int len, dlen;
	int ret = -1, nid = NID_undef;
	char *ptr;
	BIGNUM *r, *s;

	md = EVP_MD_CTX_new();
	if (md == NULL) {
		ret = -1;
		goto error;
	}

	OpenSSL_add_all_digests();

	if (key == NULL) {
		ret = -2;
		goto error;
	}

	ptr = signature;

	len = get_uint32_MSBfirst(ptr);
	ptr += 4;
	if (strncmp(get_ssh2_hostkey_type_name(keytype), ptr, len) != 0) {
		ret = -3;
		goto error;
	}
	ptr += len;

	len = get_uint32_MSBfirst(ptr);
	ptr += 4;
	sigblob = ptr;
	ptr += len;

	/* parse signature */
	if ((sig = ECDSA_SIG_new()) == NULL) {
		ret = -4;
		goto error;
	}
	if ((r = BN_new()) == NULL) {
		ret = -5;
		goto error;
	}
	if ((s = BN_new()) == NULL) {
		ret = -6;
		goto error;
	}

	ECDSA_SIG_set0(sig, r, s);
	buffer_get_bignum2((char **)&sigblob, r);
	buffer_get_bignum2((char **)&sigblob, s);
	if (sigblob != ptr) {
		ret = -7;
		goto error;
	}

	/* hash the data */
	nid = keytype_to_hash_nid(keytype);
	if ((evp_md = EVP_get_digestbynid(nid)) == NULL) {
		ret = -8;
		goto error;
	}
	EVP_DigestInit(md, evp_md);
	EVP_DigestUpdate(md, data, datalen);
	EVP_DigestFinal(md, digest, &dlen);

	ret = ECDSA_do_verify(digest, dlen, sig, key);
	SecureZeroMemory(digest, sizeof(digest));

	ECDSA_SIG_free(sig);

error:
	if (md)
		EVP_MD_CTX_free(md);

	return ret;
}

static int ssh_ed25519_verify(Key *key, unsigned char *signature, unsigned int signaturelen,
                              unsigned char *data, unsigned int datalen)
{
	buffer_t *b;
	char *ktype = NULL;
	unsigned char *sigblob = NULL, *sm = NULL, *m = NULL;
	unsigned int len;
	unsigned long long smlen, mlen;
	int rlen, ret;
	char *bptr;

	ret = -1;
	b = buffer_init();
	if (b == NULL)
		goto error;

	buffer_append(b, signature, signaturelen);
	bptr = buffer_ptr(b);
	ktype = buffer_get_string(&bptr, NULL);
	if (strcmp("ssh-ed25519", ktype) != 0) {
		goto error;
	}
	sigblob = buffer_get_string(&bptr, &len);
	rlen = buffer_remain_len(b);
	if (rlen != 0) {
		goto error;
	}
	if (len > crypto_sign_ed25519_BYTES) {
		goto error;
	}

	smlen = len + datalen;
	sm = malloc((size_t)smlen);
	memcpy(sm, sigblob, len);
	memcpy(sm+len, data, datalen);
	mlen = smlen;
	m = malloc((size_t)mlen);

	if ((ret = crypto_sign_ed25519_open(m, &mlen, sm, smlen,
	    key->ed25519_pk)) != 0) {
		//debug2("%s: crypto_sign_ed25519_open failed: %d",
		//    __func__, ret);
	}
	if (ret == 0 && mlen != datalen) {
		//debug2("%s: crypto_sign_ed25519_open "
		//    "mlen != datalen (%llu != %u)", __func__, mlen, datalen);
		ret = -1;
	}
	/* XXX compare 'm' and 'data' ? */

error:
	buffer_free(b);
	free(ktype);

	if (sigblob) {
		SecureZeroMemory(sigblob, len);
		free(sigblob);
	}
	if (sm) {
		SecureZeroMemory(sm, (size_t)smlen);
		free(sm);
	}
	if (m) {
		SecureZeroMemory(m, (size_t)smlen); /* NB. mlen may be invalid if ret != 0 */
		free(m);
	}

	/* translate return code carefully */
	return (ret == 0) ? 1 : -1;
}

int key_verify(Key *key,
               unsigned char *signature, unsigned int signaturelen,
               unsigned char *data, unsigned int datalen, ssh_keyalgo keyalgo)
{
	int ret = 0;

	switch (key->type) {
	case KEY_RSA:
		ret = ssh_rsa_verify(key->rsa, signature, signaturelen, data, datalen, keyalgo);
		break;
	case KEY_DSA:
		ret = ssh_dss_verify(key->dsa, signature, signaturelen, data, datalen);
		break;
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		ret = ssh_ecdsa_verify(key->ecdsa, key->type, signature, signaturelen, data, datalen);
		break;
	case KEY_ED25519:
		ret = ssh_ed25519_verify(key, signature, signaturelen, data, datalen);
		break;
	default:
		return -1;
	}

	return (ret);   // success
}

static char *copy_mp_int(char *num)
{
	int len = (get_ushort16_MSBfirst(num) + 7) / 8 + 2;
	char *result = (char *) malloc(len);

	if (result != NULL) {
		memcpy(result, num, len);
	}

	return result;
}

//
// RSA�\���̂̕���
//
RSA *duplicate_RSA(RSA *src)
{
	RSA *rsa = NULL;
	BIGNUM *e = NULL, *n = NULL;
	BIGNUM *se = NULL, *sn = NULL;

	rsa = RSA_new();
	if (rsa == NULL)
		goto error;
	n = BN_new();
	e = BN_new();
	RSA_set0_key(rsa, n, e, NULL);
	if (n == NULL || e == NULL) {
		RSA_free(rsa);
		goto error;
	}

	RSA_get0_key(src, &sn, &se, NULL);

	// �[���R�s�[(deep copy)���s���B�󂢃R�s�[(shallow copy)��NG�B
	BN_copy(n, sn);
	BN_copy(e, se);

error:
	return (rsa);
}


//
// DSA�\���̂̕���
//
DSA *duplicate_DSA(DSA *src)
{
	DSA *dsa = NULL;
	BIGNUM *p, *q, *g, *pub_key;
	BIGNUM *sp, *sq, *sg, *spub_key;

	dsa = DSA_new();
	if (dsa == NULL)
		goto error;
	p = BN_new();
	q = BN_new();
	g = BN_new();
	pub_key = BN_new();
	DSA_set0_pqg(dsa, p, q, g);
	DSA_set0_key(dsa, pub_key, NULL);
	if (p == NULL ||
	    q == NULL ||
	    g == NULL ||
	    pub_key == NULL) {
		DSA_free(dsa);
		// ��������������Ă���̂�NULL��Ԃ��悤�ɂ���B
		// �Ăь��ł̃`�F�b�N��ticket#39335�ŏ��u�\��B
		dsa = NULL;
		goto error;
	}

	// �[���R�s�[(deep copy)���s���B�󂢃R�s�[(shallow copy)��NG�B
	DSA_get0_pqg(src, &sp, &sq, &sg);
	DSA_get0_key(src, &spub_key, NULL);
	BN_copy(p, sp);
	BN_copy(q, sq);
	BN_copy(g, sg);
	BN_copy(pub_key, spub_key);

error:
	return (dsa);
}

unsigned char *duplicate_ED25519_PK(unsigned char *src)
{
	unsigned char *ptr = NULL;

	ptr = malloc(ED25519_PK_SZ);
	if (ptr) {
		memcpy(ptr, src, ED25519_PK_SZ);
	}
	return (ptr);
}

BOOL key_copy(Key *dest, Key *src)
{
	key_init(dest);
	switch (src->type) {
	case KEY_RSA1: // SSH1
		dest->type = KEY_RSA1;
		dest->bits = src->bits;
		dest->exp = copy_mp_int(src->exp);
		dest->mod = copy_mp_int(src->mod);
		break;
	case KEY_RSA: // SSH2 RSA
		dest->type = KEY_RSA;
		dest->rsa = duplicate_RSA(src->rsa);
		break;
	case KEY_DSA: // SSH2 DSA
		dest->type = KEY_DSA;
		dest->dsa = duplicate_DSA(src->dsa);
		break;
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		dest->type = src->type;
		dest->ecdsa = EC_KEY_dup(src->ecdsa);
		break;
	case KEY_ED25519:
		dest->type = src->type;
		dest->ed25519_pk = duplicate_ED25519_PK(src->ed25519_pk);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

char* key_fingerprint_raw(Key *k, digest_algorithm dgst_alg, int *dgst_raw_length)
{
	const EVP_MD *md = NULL;
	EVP_MD_CTX *ctx = NULL;
	char *blob = NULL;
	char *retval = NULL;
	int len = 0;
	int nlen, elen;
	RSA *rsa;
	BIGNUM *e = NULL, *n = NULL;

	ctx = EVP_MD_CTX_new();
	if (ctx == NULL) {
		goto error;
	}

	*dgst_raw_length = 0;

	switch (dgst_alg) {
	case SSH_DIGEST_MD5:
		md = EVP_md5();
		break;
	case SSH_DIGEST_SHA1:
		md = EVP_sha1();
		break;
	case SSH_DIGEST_SHA256:
		md = EVP_sha256();
		break;
	default:
		md = EVP_md5();
	}

	switch (k->type) {
	case KEY_RSA1:
		rsa = make_key(NULL, k->bits, k->exp, k->mod);
		RSA_get0_key(rsa, &n, &e, NULL);
		nlen = BN_num_bytes(n);
		elen = BN_num_bytes(e);
		len = nlen + elen;
		blob = malloc(len);
		if (blob == NULL) {
			// TODO:
		}
		BN_bn2bin(n, blob);
		BN_bn2bin(e, blob + nlen);
		RSA_free(rsa);
		break;

	case KEY_DSA:
	case KEY_RSA:
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
	case KEY_ED25519:
		key_to_blob(k, &blob, &len);
		break;

	case KEY_UNSPEC:
		return retval;
		break;

	default:
		//fatal("key_fingerprint_raw: bad key type %d", dgst_alg);
		break;
	}

	if (blob != NULL) {
		retval = malloc(EVP_MAX_MD_SIZE);
		if (retval == NULL) {
			// TODO:
		}
		EVP_DigestInit(ctx, md);
		EVP_DigestUpdate(ctx, blob, len);
		EVP_DigestFinal(ctx, retval, dgst_raw_length);
		SecureZeroMemory(blob, len);
		free(blob);
	} else {
		//fatal("key_fingerprint_raw: blob is null");
	}

error:
	if (ctx)
		EVP_MD_CTX_free(ctx);

	return retval;
}


const char *
ssh_key_type(ssh_keytype type)
{
	switch (type) {
	case KEY_RSA1:
		return "RSA1";
	case KEY_RSA:
		return "RSA";
	case KEY_DSA:
		return "DSA";
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		return "ECDSA";
	case KEY_ED25519:
		return "ED25519";
	}
	return "unknown";
}

unsigned int
key_size(const Key *k)
{
	BIGNUM *n = NULL;
	BIGNUM *p = NULL;

	switch (k->type) {
	case KEY_RSA1:
		// SSH1�̏ꍇ�� key->rsa �� key->dsa �� NULL �ł���̂ŁA�g��Ȃ��B
		return k->bits;
	case KEY_RSA:
		RSA_get0_key(k->rsa, &n, NULL, NULL);
		return BN_num_bits(n);
	case KEY_DSA:
		DSA_get0_pqg(k->dsa, &p, NULL, NULL);
		return BN_num_bits(p);
	case KEY_ECDSA256:
		return 256;
	case KEY_ECDSA384:
		return 384;
	case KEY_ECDSA521:
		return 521;
	case KEY_ED25519:
		return 256;	/* XXX */
	}
	return 0;
}

// based on OpenSSH 7.1
static char *
key_fingerprint_b64(const char *alg, u_char *dgst_raw, u_int dgst_raw_len)
{
	char *retval;
	unsigned int i, retval_len;
	BIO *bio, *b64;
	BUF_MEM *bufferPtr;

	retval_len = strlen(alg) + 1 + ((dgst_raw_len + 2) / 3) * 4 + 1;
	retval = malloc(retval_len);
	retval[0] = '\0';

	strncat_s(retval, retval_len, alg, _TRUNCATE);
	strncat_s(retval, retval_len, ":", _TRUNCATE);

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	BIO_write(bio, dgst_raw, dgst_raw_len);
	BIO_flush(bio);
	BIO_get_mem_ptr(bio, &bufferPtr);
	strncat_s(retval, retval_len, bufferPtr->data, _TRUNCATE);
	BIO_set_close(bio, BIO_NOCLOSE);
	BIO_free_all(bio);

	/* Remove the trailing '=' character */
	for (i = strlen(retval) - 1; retval[i] == '='; i--) {
		retval[i] = '\0';
	}

	return (retval);
}

static char *
key_fingerprint_hex(const char *alg, u_char *dgst_raw, u_int dgst_raw_len)
{
	char *retval;
	unsigned int i, retval_len;

	retval_len = strlen(alg) + 1 + dgst_raw_len * 3 + 1;
	retval = malloc(retval_len);
	retval[0] = '\0';

	strncat_s(retval, retval_len, alg, _TRUNCATE);
	strncat_s(retval, retval_len, ":", _TRUNCATE);

	for (i = 0; i < dgst_raw_len; i++) {
		char hex[4];
		_snprintf_s(hex, sizeof(hex), _TRUNCATE, "%02x:", dgst_raw[i]);
		strncat_s(retval, retval_len, hex, _TRUNCATE);
	}

	/* Remove the trailing ':' character */
	retval[retval_len - 2] = '\0';

	return (retval);
}

#define	FLDBASE		8
#define	FLDSIZE_Y	(FLDBASE + 1)
#define	FLDSIZE_X	(FLDBASE * 2 + 1)
static char *
key_fingerprint_randomart(const char *alg, u_char *dgst_raw, u_int dgst_raw_len, const Key *k)
{
	/*
	* Chars to be used after each other every time the worm
	* intersects with itself.  Matter of taste.
	*/
	char	*augmentation_string = " .o+=*BOX@%&#/^SE";
	char	*retval, *p, title[FLDSIZE_X], hash[FLDSIZE_X];
	unsigned char	 field[FLDSIZE_X][FLDSIZE_Y];
	size_t	 i, tlen, hlen;
	unsigned int	 b;
	int	 x, y, r;
	size_t	 len = strlen(augmentation_string) - 1;

	retval = calloc((FLDSIZE_X + 3 + 1), (FLDSIZE_Y + 2));

	/* initialize field */
	memset(field, 0, FLDSIZE_X * FLDSIZE_Y * sizeof(char));
	x = FLDSIZE_X / 2;
	y = FLDSIZE_Y / 2;

	/* process raw key */
	for (i = 0; i < dgst_raw_len; i++) {
		int input;
		/* each byte conveys four 2-bit move commands */
		input = dgst_raw[i];
		for (b = 0; b < 4; b++) {
			/* evaluate 2 bit, rest is shifted later */
			x += (input & 0x1) ? 1 : -1;
			y += (input & 0x2) ? 1 : -1;

			/* assure we are still in bounds */
			x = max(x, 0);
			y = max(y, 0);
			x = min(x, FLDSIZE_X - 1);
			y = min(y, FLDSIZE_Y - 1);

			/* augment the field */
			if (field[x][y] < len - 2)
				field[x][y]++;
			input = input >> 2;
		}
	}

	/* mark starting point and end point*/
	field[FLDSIZE_X / 2][FLDSIZE_Y / 2] = (unsigned char)(len - 1);
	field[x][y] = (unsigned char)len;

	/* assemble title */
	r = _snprintf_s(title, sizeof(title), _TRUNCATE, "[%s %u]",
	                ssh_key_type(k->type), key_size(k));
	/* If [type size] won't fit, then try [type]; fits "[ED25519-CERT]" */
	if (r < 0 || r >(int)sizeof(title))
		r = _snprintf_s(title, sizeof(title), _TRUNCATE, "[%s]",
		                ssh_key_type(k->type));
	tlen = (r <= 0) ? 0 : strlen(title);

	/* assemble hash ID. */
	r = _snprintf_s(hash, sizeof(hash), _TRUNCATE, "[%s]", alg);
	hlen = (r <= 0) ? 0 : strlen(hash);

	/* output upper border */
	p = retval;
	*p++ = '+';
	for (i = 0; i < (FLDSIZE_X - tlen) / 2; i++)
		*p++ = '-';
	memcpy(p, title, tlen);
	p += tlen;
	for (i += tlen; i < FLDSIZE_X; i++)
		*p++ = '-';
	*p++ = '+';
	*p++ = '\r';
	*p++ = '\n';

	/* output content */
	for (y = 0; y < FLDSIZE_Y; y++) {
		*p++ = '|';
		for (x = 0; x < FLDSIZE_X; x++)
			*p++ = augmentation_string[min(field[x][y], len)];
		*p++ = '|';
		*p++ = '\r';
		*p++ = '\n';
	}

	/* output lower border */
	*p++ = '+';
	for (i = 0; i < (FLDSIZE_X - hlen) / 2; i++)
		*p++ = '-';
	memcpy(p, hash, hlen);
	p += hlen;
	for (i += hlen; i < FLDSIZE_X; i++)
		*p++ = '-';
	*p++ = '+';

	return retval;
}
#undef	FLDBASE
#undef	FLDSIZE_Y
#undef	FLDSIZE_X

//
// fingerprint�i�w��F�z�X�g���J���̃n�b�V���j�𐶐�����
//
char *key_fingerprint(Key *key, fp_rep dgst_rep, digest_algorithm dgst_alg)
{
	char *retval = NULL, *alg;
	unsigned char *dgst_raw;
	int dgst_raw_len;

	// fingerprint�̃n�b�V���l�i�o�C�i���j�����߂�
	dgst_raw = key_fingerprint_raw(key, dgst_alg, &dgst_raw_len);
	if (dgst_raw == NULL)
		return NULL;

	alg = get_digest_algorithm_name(dgst_alg);

	switch (dgst_rep) {
	case SSH_FP_HEX:
		retval = key_fingerprint_hex(alg, dgst_raw, dgst_raw_len);
		break;
	case SSH_FP_BASE64:
		retval = key_fingerprint_b64(alg, dgst_raw, dgst_raw_len);
		break;
	case SSH_FP_RANDOMART:
		retval = key_fingerprint_randomart(alg, dgst_raw, dgst_raw_len, key);
		break;
	}

	SecureZeroMemory(dgst_raw, dgst_raw_len);
	free(dgst_raw);

	return (retval);
}

//
// �L�[�̃������̈�m��
//
static void key_add_private(Key *k)
{
	BIGNUM *d, *iqmp, *q, *p, *dmq1, *dmp1;
	BIGNUM *priv_key = NULL;

	d = iqmp = q = p = dmq1 = dmp1 = NULL;

	switch (k->type) {
		case KEY_RSA1:
		case KEY_RSA:
			d = BN_new();
			RSA_set0_key(k->rsa, NULL, NULL, d);

			iqmp = BN_new();
			q = BN_new();
			p = BN_new();
			RSA_set0_factors(k->rsa, p, q);

			dmq1 = BN_new();
			dmp1 = BN_new();
			RSA_set0_crt_params(k->rsa, dmp1, dmq1, iqmp);

			if (d == NULL || iqmp == NULL || q == NULL ||
				p == NULL || dmq1 == NULL || dmp1 == NULL)
				goto error;
			break;

		case KEY_DSA:
			priv_key = BN_new();
			DSA_set0_key(k->dsa, NULL, priv_key);
			if (priv_key == NULL)
				goto error;
			break;

		case KEY_ECDSA256:
		case KEY_ECDSA384:
		case KEY_ECDSA521:
			/* Cannot do anything until we know the group */
			break;

		case KEY_ED25519:
			/* no need to prealloc */
			break;

		case KEY_UNSPEC:
			break;

		default:
			goto error;
			break;
	}
	return;

error:
	if (d) {
		BN_free(d);
		// k->rsa->d�� NULL ���Z�b�g���邱�Ƃ͂ł��Ȃ��B
		// RSA_set0_key()�ł� NULL ��n���Ă��������Ȃ��B
	}
	if (iqmp) {
		BN_free(iqmp);
	}
	if (q) {
		BN_free(q);
	}
	if (p) {
		BN_free(p);
	}
	if (dmq1) {
		BN_free(dmq1);
	}
	if (dmp1) {
		BN_free(dmp1);
	}


	if (priv_key == NULL) {
		BN_free(priv_key);
	}

}

Key *key_new_private(int type)
{
	Key *k = key_new(type);

	key_add_private(k);
	return (k);
}


Key *key_new(int type)
{
	int success = 0;
	Key *k = NULL;
	RSA *rsa;
	DSA *dsa;
	BIGNUM *e = NULL, *n = NULL;
	BIGNUM *p, *q, *g, *pub_key;

	k = calloc(1, sizeof(Key));
	if (k == NULL)
		goto error;
	k->type = type;
	k->ecdsa = NULL;
	k->dsa = NULL;
	k->rsa = NULL;
	k->ed25519_pk = NULL;
	k->ed25519_sk = NULL;

	switch (k->type) {
		case KEY_RSA1:
		case KEY_RSA:
			rsa = RSA_new();
			if (rsa == NULL)
				goto error;
			n = BN_new();
			e = BN_new();
			RSA_set0_key(rsa, n, e, NULL);
			if (n == NULL || e == NULL)
				goto error;
			k->rsa = rsa;
			break;

		case KEY_DSA:
			dsa = DSA_new();
			if (dsa == NULL)
				goto error;
			p = BN_new();
			q = BN_new();
			g = BN_new();
			DSA_set0_pqg(dsa, p, q, g);
			pub_key = BN_new();
			DSA_set0_key(dsa, pub_key, NULL);
			if (p == NULL || q == NULL || g == NULL || pub_key == NULL)
				goto error;
			k->dsa = dsa;
			break;

		case KEY_ECDSA256:
		case KEY_ECDSA384:
		case KEY_ECDSA521:
			/* Cannot do anything until we know the group */
			break;

		case KEY_ED25519:
			/* no need to prealloc */
			break;

		case KEY_UNSPEC:
			break;

		default:
			goto error;
			break;
	}
	success = 1;

error:
	if (success == 0) {
		key_free(k);
		k = NULL;
	}
	return (k);
}


//
// Key �\���̂̃����o�̃������̈�� key ���̂��������
//   key_new() �Ȃǂ� malloc ���ꂽ���ʂ̃|�C���^��n��
//   Key �\���̂�n���Ă͂����Ȃ�
//
void key_free(Key *key)
{
	if (key == NULL) {
		return;
	}

	key_init(key);

	free(key);
}

//
// Key �\���̂̃����o�̃������̈���
//   �����o�݂̂�������Akey ���͉̂�����Ȃ�
//
void key_init(Key *key)
{
	key->type = KEY_UNSPEC;

	// SSH1
	key->bits = 0;
	if (key->exp != NULL) {
		free(key->exp);
		key->exp = NULL;
	}
	if (key->mod != NULL) {
		free(key->mod);
		key->mod = NULL;
	}

	// SSH2
	if (key->dsa != NULL) {
		DSA_free(key->dsa);
		key->dsa = NULL;
	}
	if (key->rsa != NULL) {
		RSA_free(key->rsa);
		key->rsa = NULL;
	}
	if (key->ecdsa != NULL) {
		EC_KEY_free(key->ecdsa);
		key->ecdsa = NULL;
	}
	if (key->ed25519_pk != NULL) {
		SecureZeroMemory(key->ed25519_pk, ED25519_PK_SZ);
		free(key->ed25519_pk);
		key->ed25519_pk = NULL;
	}
	if (key->ed25519_sk) {
		SecureZeroMemory(key->ed25519_sk, ED25519_SK_SZ);
		free(key->ed25519_sk);
		key->ed25519_sk = NULL;
	}
}

ssh_keytype key_curve_name_to_keytype(char *name)
{
	if (strcmp(name, "nistp256") == 0) {
		return KEY_ECDSA256;
	} else if (strcmp(name, "nistp384") == 0) {
		return KEY_ECDSA384;
	} else if (strcmp(name, "nistp521") == 0) {
		return KEY_ECDSA521;
	}
	return KEY_UNSPEC;
}

char *curve_keytype_to_name(ssh_keytype type)
{
	switch (type) {
		case KEY_ECDSA256:
			return "nistp256";
			break;
		case KEY_ECDSA384:
			return "nistp384";
			break;
		case KEY_ECDSA521:
			return "nistp521";
			break;
	}
	return NULL;
}

//
// �L�[��񂩂�o�b�t�@�֕ϊ����� (for SSH2)
// NOTE:
//
int key_to_blob(Key *key, char **blobp, int *lenp)
{
	buffer_t *b;
	char *sshname, *tmp;
	int len;
	int ret = 1;  // success
	BIGNUM *e = NULL, *n = NULL;
	BIGNUM *p, *q, *g, *pub_key;

	b = buffer_init();
	sshname = get_ssh2_hostkey_type_name_from_key(key);

	switch (key->type) {
	case KEY_RSA:
		RSA_get0_key(key->rsa, &n, &e, NULL);
		buffer_put_string(b, sshname, strlen(sshname));
		buffer_put_bignum2(b, e);
		buffer_put_bignum2(b, n);
		break;
	case KEY_DSA:
		DSA_get0_pqg(key->dsa, &p, &q, &g);
		DSA_get0_key(key->dsa, &pub_key, NULL);
		buffer_put_string(b, sshname, strlen(sshname));
		buffer_put_bignum2(b, p);
		buffer_put_bignum2(b, q);
		buffer_put_bignum2(b, g);
		buffer_put_bignum2(b, pub_key);
		break;
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		buffer_put_string(b, sshname, strlen(sshname));
		tmp = curve_keytype_to_name(key->type);
		buffer_put_string(b, tmp, strlen(tmp));
		buffer_put_ecpoint(b, EC_KEY_get0_group(key->ecdsa),
		                      EC_KEY_get0_public_key(key->ecdsa));
		break;
	case KEY_ED25519:
		buffer_put_cstring(b, sshname);
		buffer_put_string(b, key->ed25519_pk, ED25519_PK_SZ);
		break;

	default:
		ret = 0;
		goto error;
	}

	len = buffer_len(b);
	if (lenp != NULL)
		*lenp = len;
	if (blobp != NULL) {
		*blobp = malloc(len);
		if (*blobp == NULL) {
			ret = 0;
			goto error;
		}
		memcpy(*blobp, buffer_ptr(b), len);
	}

error:
	buffer_free(b);

	return (ret);
}


//
// �o�b�t�@����L�[�������o��(for SSH2)
// NOTE: �Ԓl�̓A���P�[�g�̈�ɂȂ�̂ŁA�Ăяo�����ŉ�����邱�ƁB
//
Key *key_from_blob(char *data, int blen)
{
	int keynamelen, len;
	char key[128];
	RSA *rsa = NULL;
	DSA *dsa = NULL;
	EC_KEY *ecdsa = NULL;
	EC_POINT *q = NULL;
	char *curve = NULL;
	Key *hostkey = NULL;  // hostkey
	ssh_keytype type;
	unsigned char *pk = NULL;
	BIGNUM *e = NULL, *n = NULL;
	BIGNUM *p, *dsa_q, *g, *pub_key;

	if (data == NULL)
		goto error;

	hostkey = malloc(sizeof(Key));
	if (hostkey == NULL)
		goto error;

	memset(hostkey, 0, sizeof(Key));

	keynamelen = get_uint32_MSBfirst(data);
	if (keynamelen >= sizeof(key)) {
		goto error;
	}
	data += 4;
	memcpy(key, data, keynamelen);
	key[keynamelen] = 0;
	data += keynamelen;

	type = get_hostkey_type_from_name(key);

	switch (type) {
	case KEY_RSA: // RSA key
		rsa = RSA_new();
		if (rsa == NULL) {
			goto error;
		}
		n = BN_new();
		e = BN_new();
		RSA_set0_key(rsa, n, e, NULL);
		if (n == NULL || e == NULL) {
			goto error;
		}

		buffer_get_bignum2(&data, e);
		buffer_get_bignum2(&data, n);

		hostkey->type = type;
		hostkey->rsa = rsa;
		break;

	case KEY_DSA: // DSA key
		dsa = DSA_new();
		if (dsa == NULL) {
			goto error;
		}
		p = BN_new();
		dsa_q = BN_new();
		g = BN_new();
		pub_key = BN_new();
		DSA_set0_pqg(dsa, p, dsa_q, g);
		DSA_set0_key(dsa, pub_key, NULL);
		if (p == NULL ||
		    dsa_q == NULL ||
		    g == NULL ||
		    pub_key == NULL) {
			goto error;
		}

		buffer_get_bignum2(&data, p);
		buffer_get_bignum2(&data, dsa_q);
		buffer_get_bignum2(&data, g);
		buffer_get_bignum2(&data, pub_key);

		hostkey->type = type;
		hostkey->dsa = dsa;
		break;

	case KEY_ECDSA256: // ECDSA
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		curve = buffer_get_string(&data, NULL);
		if (type != key_curve_name_to_keytype(curve)) {
			goto error;
		}

		ecdsa = EC_KEY_new_by_curve_name(keytype_to_cipher_nid(type));
		if (ecdsa == NULL) {
			goto error;
		}

		q = EC_POINT_new(EC_KEY_get0_group(ecdsa));
		if (q == NULL) {
			goto error;
		}

		buffer_get_ecpoint(&data, EC_KEY_get0_group(ecdsa), q);
		if (key_ec_validate_public(EC_KEY_get0_group(ecdsa), q) == -1) {
			goto error;
		}

		if (EC_KEY_set_public_key(ecdsa, q) != 1) {
			goto error;
		}

		hostkey->type = type;
		hostkey->ecdsa = ecdsa;
		break;

	case KEY_ED25519:
		pk = buffer_get_string(&data, &len);
		if (pk == NULL)
			goto error;
		if (len != ED25519_PK_SZ)
			goto error;

		hostkey->type = type;
		hostkey->ed25519_pk = pk;
		pk = NULL;
		break;

	default: // unknown key
		goto error;
	}

	return (hostkey);

error:
	if (rsa != NULL)
		RSA_free(rsa);
	if (dsa != NULL)
		DSA_free(dsa);
	if (ecdsa != NULL)
		EC_KEY_free(ecdsa);

	free(hostkey);

	return NULL;
}


static int ssh_ed25519_sign(Key *key, char **sigp, int *lenp, char *data, int datalen)
{
	char *sig;
	int slen, len;
	unsigned long long smlen;
	int ret;
	buffer_t *b;

	smlen = slen = datalen + crypto_sign_ed25519_BYTES;
	sig = malloc(slen);

	if ((ret = crypto_sign_ed25519(sig, &smlen, data, datalen,
	    key->ed25519_sk)) != 0 || smlen <= datalen) {
		//error("%s: crypto_sign_ed25519 failed: %d", __func__, ret);
		free(sig);
		return -1;
	}
	/* encode signature */
	b = buffer_init();
	buffer_put_cstring(b, "ssh-ed25519");
	buffer_put_string(b, sig, (int)(smlen - datalen));
	len = buffer_len(b);
	if (lenp != NULL)
		*lenp = len;
	if (sigp != NULL) {
		*sigp = malloc(len);
		memcpy(*sigp, buffer_ptr(b), len);
	}
	buffer_free(b);
	SecureZeroMemory(sig, slen);
	free(sig);

	return 0;
}


BOOL generate_SSH2_keysign(Key *keypair, char **sigptr, int *siglen, char *data, int datalen, ssh_keyalgo keyalgo)
{
	buffer_t *msg = NULL;
	char *s;
	int ret;

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		return FALSE;
	}

	switch (keypair->type) {
	case KEY_RSA: // RSA
	{
		const EVP_MD *evp_md;
		EVP_MD_CTX *md = NULL;
		u_char digest[EVP_MAX_MD_SIZE], *sig;
		u_int slen, dlen, len;
		int ok, nid;

		nid = get_ssh2_key_hashtype(keyalgo);

		switch(nid) {
		case NID_sha1:
			evp_md = EVP_sha1();
			break;
		case NID_sha256:
			evp_md = EVP_sha256();
			break;
		case NID_sha512:
			evp_md = EVP_sha512();
			break;
		default:
			goto error;
		}

		md = EVP_MD_CTX_new();
		if (md == NULL)
			goto error;

		// �_�C�W�F�X�g�l�̌v�Z
		EVP_DigestInit(md, evp_md);
		EVP_DigestUpdate(md, data, datalen);
		EVP_DigestFinal(md, digest, &dlen);

		EVP_MD_CTX_free(md);

		slen = RSA_size(keypair->rsa);
		sig = malloc(slen);
		if (sig == NULL)
			goto error;

		// �d�q�������v�Z
		ok = RSA_sign(nid, digest, dlen, sig, &len, keypair->rsa);
		SecureZeroMemory(digest, sizeof(digest));
		if (ok != 1) { // error
			free(sig);
			goto error;
		}
		// �����̃T�C�Y���o�b�t�@��菬�����ꍇ�A���ւ��炷�B�擪�̓[���Ŗ��߂�B
		if (len < slen) {
			u_int diff = slen - len;
			memmove(sig + diff, sig, len);
			memset(sig, 0, diff);

		} else if (len > slen) {
			free(sig);
			goto error;

		} else {
			// do nothing

		}

		s = get_ssh2_hostkey_algorithm_name(keyalgo);
		buffer_put_string(msg, s, strlen(s));
		buffer_append_length(msg, sig, slen);
		len = buffer_len(msg);

		// setting
		*siglen = len;
		*sigptr = malloc(len);
		if (*sigptr == NULL) {
			free(sig);
			goto error;
		}
		memcpy(*sigptr, buffer_ptr(msg), len);
		free(sig);

		break;
	}
	case KEY_DSA: // DSA
	{
		DSA_SIG *sig;
		const EVP_MD *evp_md = EVP_sha1();
		EVP_MD_CTX *md = NULL;
		u_char digest[EVP_MAX_MD_SIZE], sigblob[SIGBLOB_LEN];
		u_int rlen, slen, len, dlen;
		BIGNUM *bignum_r, *bignum_s;

		md = EVP_MD_CTX_new();
		if (md == NULL)
			goto error;

		// �_�C�W�F�X�g�̌v�Z
		EVP_DigestInit(md, evp_md);
		EVP_DigestUpdate(md, data, datalen);
		EVP_DigestFinal(md, digest, &dlen);

		EVP_MD_CTX_free(md);

		// DSA�d�q�������v�Z
		sig = DSA_do_sign(digest, dlen, keypair->dsa);
		SecureZeroMemory(digest, sizeof(digest));
		if (sig == NULL) {
			goto error;
		}

		// BIGNUM����o�C�i���l�ւ̕ϊ�
		DSA_SIG_get0(sig, &bignum_r, &bignum_s);
		rlen = BN_num_bytes(bignum_r);
		slen = BN_num_bytes(bignum_s);
		if (rlen > INTBLOB_LEN || slen > INTBLOB_LEN) {
			DSA_SIG_free(sig);
			goto error;
		}
		memset(sigblob, 0, SIGBLOB_LEN);
		BN_bn2bin(bignum_r, sigblob+ SIGBLOB_LEN - INTBLOB_LEN - rlen);
		BN_bn2bin(bignum_s, sigblob+ SIGBLOB_LEN - slen);
		DSA_SIG_free(sig);

		// setting
		s = get_ssh2_hostkey_type_name_from_key(keypair);
		buffer_put_string(msg, s, strlen(s));
		buffer_append_length(msg, sigblob, sizeof(sigblob));
		len = buffer_len(msg);

		// setting
		*siglen = len;
		*sigptr = malloc(len);
		if (*sigptr == NULL) {
			goto error;
		}
		memcpy(*sigptr, buffer_ptr(msg), len);

		break;
	}
	case KEY_ECDSA256: // ECDSA
	case KEY_ECDSA384:
	case KEY_ECDSA521:
	{
		ECDSA_SIG *sig;
		const EVP_MD *evp_md;
		EVP_MD_CTX *md = NULL;
		u_char digest[EVP_MAX_MD_SIZE];
		u_int len, dlen, nid;
		buffer_t *buf2 = NULL;
		BIGNUM *br, *bs;

		nid = keytype_to_hash_nid(keypair->type);
		if ((evp_md = EVP_get_digestbynid(nid)) == NULL) {
			goto error;
		}

		md = EVP_MD_CTX_new();
		if (md == NULL)
			goto error;

		EVP_DigestInit(md, evp_md);
		EVP_DigestUpdate(md, data, datalen);
		EVP_DigestFinal(md, digest, &dlen);

		EVP_MD_CTX_free(md);

		sig = ECDSA_do_sign(digest, dlen, keypair->ecdsa);
		SecureZeroMemory(digest, sizeof(digest));

		if (sig == NULL) {
			goto error;
		}

		buf2 = buffer_init();
		if (buf2 == NULL) {
			// TODO: error check
			goto error;
		}
		ECDSA_SIG_get0(sig, &br, &bs);
		buffer_put_bignum2(buf2, br);
		buffer_put_bignum2(buf2, bs);
		ECDSA_SIG_free(sig);

		s = get_ssh2_hostkey_type_name_from_key(keypair);
		buffer_put_string(msg, s, strlen(s));
		buffer_put_string(msg, buffer_ptr(buf2), buffer_len(buf2));
		buffer_free(buf2);
		len = buffer_len(msg);

		*siglen = len;
		*sigptr = malloc(len);
		if (*sigptr == NULL) {
			goto error;
		}
		memcpy(*sigptr, buffer_ptr(msg), len);

		break;
	}

	case KEY_ED25519:
		ret = ssh_ed25519_sign(keypair, sigptr, siglen, data, datalen);
		if (ret != 0)
			goto error;
		break;

	default:
		buffer_free(msg);
		return FALSE;
		break;
	}

	buffer_free(msg);
	return TRUE;

error:
	buffer_free(msg);

	return FALSE;
}


BOOL get_SSH2_publickey_blob(PTInstVar pvar, buffer_t **blobptr, int *bloblen)
{
	buffer_t *msg = NULL;
	Key *keypair;
	char *s, *tmp;
	BIGNUM *e = NULL, *n = NULL;
	BIGNUM *p, *q, *g, *pub_key;

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		return FALSE;
	}

	keypair = pvar->auth_state.cur_cred.key_pair;

	switch (keypair->type) {
	case KEY_RSA: // RSA
		s = get_ssh2_hostkey_type_name_from_key(keypair);
		RSA_get0_key(keypair->rsa, &n, &e, NULL);
		buffer_put_string(msg, s, strlen(s));
		buffer_put_bignum2(msg, e); // ���J�w��
		buffer_put_bignum2(msg, n); // p�~q
		break;
	case KEY_DSA: // DSA
		DSA_get0_pqg(keypair->dsa, &p, &q, &g);
		DSA_get0_key(keypair->dsa, &pub_key, NULL);
		s = get_ssh2_hostkey_type_name_from_key(keypair);
		buffer_put_string(msg, s, strlen(s));
		buffer_put_bignum2(msg, p); // �f��
		buffer_put_bignum2(msg, q); // (p-1)�̑f����
		buffer_put_bignum2(msg, g); // ����
		buffer_put_bignum2(msg, pub_key); // ���J��
		break;
	case KEY_ECDSA256: // ECDSA
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		s = get_ssh2_hostkey_type_name_from_key(keypair);
		buffer_put_string(msg, s, strlen(s));
		tmp = curve_keytype_to_name(keypair->type);
		buffer_put_string(msg, tmp, strlen(tmp));
		buffer_put_ecpoint(msg, EC_KEY_get0_group(keypair->ecdsa),
		                        EC_KEY_get0_public_key(keypair->ecdsa));
		break;
	case KEY_ED25519:
		s = get_ssh2_hostkey_type_name_from_key(keypair);
		buffer_put_cstring(msg, s);
		buffer_put_string(msg, keypair->ed25519_pk, ED25519_PK_SZ);
		break;
	default:
		return FALSE;
	}

	*blobptr = msg;
	*bloblen = buffer_len(msg);

	return TRUE;
}

int kextype_to_cipher_nid(kex_algorithm type)
{
	switch (type) {
		case KEX_ECDH_SHA2_256:
			return NID_X9_62_prime256v1;
		case KEX_ECDH_SHA2_384:
			return NID_secp384r1;
		case KEX_ECDH_SHA2_521:
			return NID_secp521r1;
	}
	return NID_undef;
}

int keytype_to_hash_nid(ssh_keytype type)
{
	switch (type) {
		case KEY_ECDSA256:
			return NID_sha256;
		case KEY_ECDSA384:
			return NID_sha384;
		case KEY_ECDSA521:
			return NID_sha512;
	}
	return NID_undef;
}

int keytype_to_cipher_nid(ssh_keytype type)
{
	switch (type) {
		case KEY_ECDSA256:
			return NID_X9_62_prime256v1;
		case KEY_ECDSA384:
			return NID_secp384r1;
		case KEY_ECDSA521:
			return NID_secp521r1;
	}
	return NID_undef;
}

ssh_keytype nid_to_keytype(int nid)
{
	switch (nid) {
		case NID_X9_62_prime256v1:
			return KEY_ECDSA256;
		case NID_secp384r1:
			return KEY_ECDSA384;
		case NID_secp521r1:
			return KEY_ECDSA521;
	}
	return KEY_UNSPEC;
}

void key_private_serialize(Key *key, buffer_t *b)
{
	char *s;
	BIGNUM *e, *n, *d, *iqmp, *p, *q;
	BIGNUM *g, *pub_key, *priv_key;

	s = get_ssh2_hostkey_type_name_from_key(key);
	buffer_put_cstring(b, s);

	switch (key->type) {
		case KEY_RSA:
			RSA_get0_key(key->rsa, &n, &e, &d);
			RSA_get0_factors(key->rsa, &p, &q);
			RSA_get0_crt_params(key->rsa, NULL, NULL, &iqmp);

			buffer_put_bignum2(b, n);
			buffer_put_bignum2(b, e);
			buffer_put_bignum2(b, d);
			buffer_put_bignum2(b, iqmp);
			buffer_put_bignum2(b, p);
			buffer_put_bignum2(b, q);
			break;

		case KEY_DSA:
			DSA_get0_pqg(key->dsa, &p, &q, &g);
			DSA_get0_key(key->dsa, &pub_key, &priv_key);
			buffer_put_bignum2(b, p);
			buffer_put_bignum2(b, q);
			buffer_put_bignum2(b, g);
			buffer_put_bignum2(b, pub_key);
			buffer_put_bignum2(b, priv_key);
			break;

		case KEY_ECDSA256:
		case KEY_ECDSA384:
		case KEY_ECDSA521:
			buffer_put_cstring(b, curve_keytype_to_name(key->type));
			buffer_put_ecpoint(b, EC_KEY_get0_group(key->ecdsa),
				EC_KEY_get0_public_key(key->ecdsa));
			buffer_put_bignum2(b, (BIGNUM *)EC_KEY_get0_private_key(key->ecdsa));
			break;

		case KEY_ED25519:
			buffer_put_string(b, key->ed25519_pk, ED25519_PK_SZ);
			buffer_put_string(b, key->ed25519_sk, ED25519_SK_SZ);
			break;

		default:
			break;
	}
}

/* calculate p-1 and q-1 */
static void rsa_generate_additional_parameters(RSA *rsa)
{
	BIGNUM *aux = NULL;
	BN_CTX *ctx = NULL;
	BIGNUM *e, *n, *d, *dmp1, *dmq1, *iqmp, *p, *q;

	if ((aux = BN_new()) == NULL)
		goto error;
	if ((ctx = BN_CTX_new()) == NULL)
		goto error;

	RSA_get0_key(rsa, &n, &e, &d);
	RSA_get0_factors(rsa, &p, &q);
	RSA_get0_crt_params(rsa, &dmp1, &dmq1, &iqmp);

	if ((BN_sub(aux, q, BN_value_one()) == 0) ||
	    (BN_mod(dmq1, d, aux, ctx) == 0) ||
	    (BN_sub(aux, p, BN_value_one()) == 0) ||
	    (BN_mod(dmp1, d, aux, ctx) == 0))
		goto error;

error:
	if (aux)
		BN_clear_free(aux);
	if (ctx)
		BN_CTX_free(ctx);
}

Key *key_private_deserialize(buffer_t *blob)
{
	int success = 0;
	char *type_name = NULL;
	Key *k = NULL;
	unsigned int pklen, sklen;
	int type;
	BIGNUM *e, *n, *d, *dmp1, *dmq1, *iqmp, *p, *q;
	BIGNUM *g, *pub_key, *priv_key;

	type_name = buffer_get_string_msg(blob, NULL);
	if (type_name == NULL)
		goto error;
	type = get_hostkey_type_from_name(type_name);

	k = key_new_private(type);

	switch (type) {
		case KEY_RSA:
			RSA_get0_key(k->rsa, &n, &e, &d);
			RSA_get0_factors(k->rsa, &p, &q);
			RSA_get0_crt_params(k->rsa, &dmp1, &dmq1, &iqmp);

			buffer_get_bignum2_msg(blob, n);
			buffer_get_bignum2_msg(blob, e);
			buffer_get_bignum2_msg(blob, d);
			buffer_get_bignum2_msg(blob, iqmp);
			buffer_get_bignum2_msg(blob, p);
			buffer_get_bignum2_msg(blob, q);

			/* Generate additional parameters */
			rsa_generate_additional_parameters(k->rsa);
			break;

		case KEY_DSA:
			DSA_get0_pqg(k->dsa, &p, &q, &g);
			DSA_get0_key(k->dsa, &pub_key, &priv_key);
			buffer_get_bignum2_msg(blob, p);
			buffer_get_bignum2_msg(blob, q);
			buffer_get_bignum2_msg(blob, g);
			buffer_get_bignum2_msg(blob, pub_key);
			buffer_get_bignum2_msg(blob, priv_key);
			break;

		case KEY_ECDSA256:
		case KEY_ECDSA384:
		case KEY_ECDSA521:
			{
			int success = 0;
			unsigned int nid;
			char *curve = NULL;
			ssh_keytype skt;
			BIGNUM *exponent = NULL;
			EC_POINT *q = NULL;

			nid = keytype_to_cipher_nid(type);
			curve = buffer_get_string_msg(blob, NULL);
			skt = key_curve_name_to_keytype(curve);
			if (nid != keytype_to_cipher_nid(skt))
				goto ecdsa_error;

			if ((k->ecdsa = EC_KEY_new_by_curve_name(nid)) == NULL)
				goto ecdsa_error;
			if ((q = EC_POINT_new(EC_KEY_get0_group(k->ecdsa))) == NULL)
				goto ecdsa_error;
			if ((exponent = BN_new()) == NULL)
				goto ecdsa_error;

			buffer_get_ecpoint_msg(blob, EC_KEY_get0_group(k->ecdsa), q);
			buffer_get_bignum2_msg(blob, exponent);
			if (EC_KEY_set_public_key(k->ecdsa, q) != 1)
				goto ecdsa_error;
			if (EC_KEY_set_private_key(k->ecdsa, exponent) != 1)
				goto ecdsa_error;
			if (key_ec_validate_public(EC_KEY_get0_group(k->ecdsa),
			                           EC_KEY_get0_public_key(k->ecdsa)) != 0)
				goto ecdsa_error;
			if (key_ec_validate_private(k->ecdsa) != 0)
				goto ecdsa_error;

			success = 1;

ecdsa_error:
			free(curve);
			if (exponent)
				BN_clear_free(exponent);
			if (q)
				EC_POINT_free(q);
			if (success == 0)
				goto error;
			}
			break;

		case KEY_ED25519:
			k->ed25519_pk = buffer_get_string_msg(blob, &pklen);
			k->ed25519_sk = buffer_get_string_msg(blob, &sklen);
			if (pklen != ED25519_PK_SZ)
				goto error;
			if (sklen != ED25519_SK_SZ)
				goto error;
			break;

		default:
			goto error;
			break;
	}

	/* enable blinding */
	switch (k->type) {
		case KEY_RSA1:
		case KEY_RSA:
			if (RSA_blinding_on(k->rsa, NULL) != 1)
				goto error;
			break;
	}

	success = 1;

error:
	free(type_name);

	if (success == 0) {
		key_free(k);
		k = NULL;
	}

	return (k);
}


int key_ec_validate_private(EC_KEY *key)
{
	BN_CTX *bnctx = NULL;
	BIGNUM *order, *tmp;
	int ret = -1;

	if ((bnctx = BN_CTX_new()) == NULL)
		goto out;
	BN_CTX_start(bnctx);

	if ((order = BN_CTX_get(bnctx)) == NULL ||
	    (tmp = BN_CTX_get(bnctx)) == NULL)
		goto out;

	/* log2(private) > log2(order)/2 */
	if (EC_GROUP_get_order(EC_KEY_get0_group(key), order, bnctx) != 1)
		goto out;
	if (BN_num_bits(EC_KEY_get0_private_key(key)) <=
	    BN_num_bits(order) / 2) {
		goto out;
	}

	/* private < order - 1 */
	if (!BN_sub(tmp, order, BN_value_one()))
		goto out;
	if (BN_cmp(EC_KEY_get0_private_key(key), tmp) >= 0) {
		goto out;
	}
	ret = 0;

out:
	if (bnctx)
		BN_CTX_free(bnctx);
	return ret;
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

static void hostkeys_update_ctx_free(struct hostkeys_update_ctx *ctx)
{
	size_t i;

	if (ctx == NULL)
		return;
	for (i = 0; i < ctx->nkeys; i++)
		key_free(ctx->keys[i]);
	free(ctx->keys);
	free(ctx->keys_seen);
	for (i = 0; i < ctx->nold; i++)
		key_free(ctx->old_keys[i]);
	free(ctx->old_keys);
	free(ctx->host_str);
	free(ctx->ip_str);
	free(ctx);
}


// �����ꂽ�z�X�g���A���S���Y�������`�F�b�N����B
//
// return 1: matched
//        0: not matched
//
static int check_hostkey_algorithm(PTInstVar pvar, Key *key)
{
	int i, index;

	for (i = 0; pvar->settings.HostKeyOrder[i] != 0; i++) {
		index = pvar->settings.HostKeyOrder[i] - '0';
		if (index == KEY_ALGO_NONE) // disabled line
			break;

		if (key->type == get_ssh2_hostkey_type_from_algorithm(index))
			return 1;
	}

	return 0;
}

// Callback function
//
// argument:
//   key: known_hosts�ɓo�^����Ă��錮
//   _ctx: �T�[�o���瑗���Ă��������Q
//
// return:
//   1: deprecated key�̂��߁A�Ăь���key�̈�̉���֎~�B
//   0: �Ăь��ł�key�̈�̉�����K�v�B
static int hostkeys_find(Key *key, void *_ctx)
{
	struct hostkeys_update_ctx *ctx = (struct hostkeys_update_ctx *)_ctx;
	int ret = 0;
	size_t i;
	Key **tmp;

	// SSH1�͑ΏۊO�B
	if (key->type == KEY_RSA1)
		goto error;

	// ���łɓo�^�ς݂̌����Ȃ�����T���B
	for (i = 0; i < ctx->nkeys; i++) {
		if (HOSTS_compare_public_key(key, ctx->keys[i]) == 1) {
			ctx->keys_seen[i] = 1;
			goto error;
		}
	}

	// deprecated�Ȍ��́A�Â����̃��X�g�ɓ���Ă����B
	tmp = realloc(ctx->old_keys, (ctx->nold + 1)*sizeof(*ctx->old_keys));
	if (tmp != NULL) {
		ctx->old_keys = tmp;
		ctx->old_keys[ctx->nold++] = key;
	}

	ret = 1;

error:
	return (ret);
}

static void hosts_updatekey_dlg_set_fingerprint(PTInstVar pvar, HWND dlg, digest_algorithm dgst_alg)
{
	char *fp, *buf;
	size_t i;
	int buf_len;
	struct hostkeys_update_ctx *ctx;

	ctx = pvar->hostkey_ctx;

	if (ctx->nkeys > 0) {
		buf_len = 100 * ctx->nkeys;
		buf = calloc(100, ctx->nkeys);
		for (i = 0; i < ctx->nkeys; i++) {
			if (ctx->keys_seen[i])
				continue;
			switch (dgst_alg) {
			case SSH_DIGEST_MD5:
				fp = key_fingerprint(ctx->keys[i], SSH_FP_HEX, dgst_alg);
				break;
			case SSH_DIGEST_SHA256:
			default:
				fp = key_fingerprint(ctx->keys[i], SSH_FP_BASE64, SSH_DIGEST_SHA256);
				break;
			}
			strncat_s(buf, buf_len, get_ssh2_hostkey_type_name_from_key(ctx->keys[i]), _TRUNCATE);
			strncat_s(buf, buf_len, " ", _TRUNCATE);
			if (fp != NULL) {
				strncat_s(buf, buf_len, fp, _TRUNCATE);
				free(fp);
			}
			if (i < ctx->nkeys - 1) {
				strncat_s(buf, buf_len, "\r\n", _TRUNCATE);
			}
		}
		SetDlgItemTextA(dlg, IDC_ADDKEY_EDIT, buf);
		free(buf);
	}

	if (ctx->nold > 0) {
		buf_len = 100 * ctx->nold;
		buf = calloc(100, ctx->nold);
		SetDlgItemTextA(dlg, IDC_REMOVEKEY_EDIT, "");
		for (i = 0; i < ctx->nold; i++) {
			switch (dgst_alg) {
			case SSH_DIGEST_MD5:
				fp = key_fingerprint(ctx->old_keys[i], SSH_FP_HEX, dgst_alg);
				break;
			case SSH_DIGEST_SHA256:
			default:
				fp = key_fingerprint(ctx->old_keys[i], SSH_FP_BASE64, SSH_DIGEST_SHA256);
				break;
			}
			strncat_s(buf, buf_len, get_ssh2_hostkey_type_name_from_key(ctx->old_keys[i]), _TRUNCATE);
			strncat_s(buf, buf_len, " ", _TRUNCATE);
			if (fp != NULL) {
				strncat_s(buf, buf_len, fp, _TRUNCATE);
				free(fp);
			}
			if (i < ctx->nold - 1) {
				strncat_s(buf, buf_len, "\r\n", _TRUNCATE);
			}
		}
		SetDlgItemTextA(dlg, IDC_REMOVEKEY_EDIT, buf);
		free(buf);
	}
}

//typedef INT_PTR (CALLBACK* DLGPROC)(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK hosts_updatekey_dlg_proc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PTInstVar pvar;
	char buf[1024];
	char *host;
	struct hostkeys_update_ctx *ctx;
	char uimsg[MAX_UIMSG];

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar)lParam;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		GetWindowText(dlg, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOSTKEY_ROTATION_TITLE", pvar, uimsg);
		SetWindowText(dlg, pvar->UIMsg);

		host = pvar->ssh_state.hostname;
		ctx = pvar->hostkey_ctx;

		GetDlgItemText(dlg, IDC_HOSTKEY_MESSAGE, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOSTKEY_ROTATION_WARNING", pvar, uimsg);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			pvar->UIMsg, host, ctx->nnew, ctx->nold
			);
		SetDlgItemText(dlg, IDC_HOSTKEY_MESSAGE, buf);

		GetDlgItemText(dlg, IDC_FP_HASH_ALG, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOSTKEY_ROTATION_FP_HASH_ALGORITHM", pvar, uimsg);
		SetDlgItemText(dlg, IDC_FP_HASH_ALG, pvar->UIMsg);

		GetDlgItemText(dlg, IDC_ADDKEY_TEXT, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOSTKEY_ROTATION_ADD", pvar, uimsg);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg, ctx->nnew);
		SetDlgItemText(dlg, IDC_ADDKEY_TEXT, buf);

		GetDlgItemText(dlg, IDC_REMOVEKEY_TEXT, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOSTKEY_ROTATION_REMOVE", pvar, uimsg);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg, ctx->nold);
		SetDlgItemText(dlg, IDC_REMOVEKEY_TEXT, buf);

		CheckDlgButton(dlg, IDC_FP_HASH_ALG_SHA256, TRUE);
		hosts_updatekey_dlg_set_fingerprint(pvar, dlg, SSH_DIGEST_SHA256);

		GetDlgItemText(dlg, IDOK, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("BTN_YES", pvar, uimsg);
		SetDlgItemText(dlg, IDOK, pvar->UIMsg);
		GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("BTN_NO", pvar, uimsg);
		SetDlgItemText(dlg, IDCANCEL, pvar->UIMsg);

		CenterWindow(dlg, GetParent(dlg));
		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar)GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDOK:
			TTEndDialog(dlg, 1);
			return TRUE;

		case IDCANCEL:			/* kill the connection */
			TTEndDialog(dlg, 0);
			return TRUE;

		case IDC_FP_HASH_ALG_MD5:
			hosts_updatekey_dlg_set_fingerprint(pvar, dlg, SSH_DIGEST_MD5);
			return TRUE;

		case IDC_FP_HASH_ALG_SHA256:
			hosts_updatekey_dlg_set_fingerprint(pvar, dlg, SSH_DIGEST_SHA256);
			return TRUE;

		default:
			return FALSE;
		}

	default:
		return FALSE;
	}
}

static void update_known_hosts(PTInstVar pvar, struct hostkeys_update_ctx *ctx)
{
	size_t i;

	// "/nosecuritywarning"���w�肳��Ă���ꍇ�A�X�V�͈�؍s��Ȃ��B
	if (pvar->nocheck_known_hosts) {
		logputs(LOG_LEVEL_VERBOSE, "Hostkey was not updated because `/nosecuritywarning' option was specified.");
		goto error;
	}

	// known_hosts�t�@�C���̍X�V���s�����߁A���[�U�ɖ₢���킹���s���B
	if (pvar->settings.UpdateHostkeys == SSH_UPDATE_HOSTKEYS_ASK) {
		INT_PTR dlgresult;
		HWND cur_active = GetActiveWindow();

		pvar->hostkey_ctx = ctx;
		dlgresult = TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SSHUPDATE_HOSTKEY),
			cur_active != NULL ? cur_active : pvar->NotificationWindow,
			hosts_updatekey_dlg_proc, (LPARAM)pvar);
		if (dlgresult != 1) {
			logputs(LOG_LEVEL_VERBOSE, "Hostkey was not updated because a user cancelled.");
			goto error;
		}
	}

	// �Â��L�[�����ׂč폜����B
	HOSTS_delete_all_hostkeys(pvar);

	// �V�����L�[�����ׂēo�^����B
	for (i = 0; i < ctx->nkeys; i++) {
		HOSTS_add_host_key(pvar, ctx->keys[i]);
	}
	logputs(LOG_LEVEL_VERBOSE, "Hostkey was successfully updated to known_hosts file.");

error:
	return;
}

static void client_global_hostkeys_private_confirm(PTInstVar pvar, int type, u_int32_t seq, void *_ctx)
{
	struct hostkeys_update_ctx *ctx = (struct hostkeys_update_ctx *)_ctx;
	char *data;
	int len;
	unsigned char *blob = NULL;
	int bloblen;
	buffer_t *b = NULL;
	buffer_t *bsig = NULL;
	char *cp, *sig;
	size_t i, ndone, siglen;
	int siglen_i;
	int ret;

	// SSH2 packet format:
	// [size(4) + padding size(1) + type(1)] + [payload(N) + padding(X)]
	//  header                                     body
	//                                         ^data
	//            <-----------------size------------------------------->
	//                              <---------len-------->
	//
	// data = payload(N) + padding(X): �p�f�B���O���܂߂��{�f�B���ׂĂ��w���B
	data = pvar->ssh_state.payload;
	// len = size - (padding size + 1): �p�f�B���O�������{�f�B�Btype���擪�Ɋ܂܂��B
	len = pvar->ssh_state.payloadlen;
	len--;   // type ��������

	bsig = buffer_init();
	if (bsig == NULL)
		goto error;
	cp = buffer_append_space(bsig, len);
	memcpy(cp, data, len);

	if (ctx->nnew == 0) {
		logprintf(LOG_LEVEL_FATAL,
			"Hostkey can not be updated because ctx->nnew %d(program bug).", ctx->nnew);
		goto error;
	}
	if (type != SSH2_MSG_REQUEST_SUCCESS) {
		logprintf(LOG_LEVEL_ERROR,
			"Server failed to confirm ownership of private host keys(type %d)", type);
		goto error;
	}
	if (pvar->session_id_len == 0) {
		logprintf(LOG_LEVEL_FATAL,
			"Hostkey can not be updated because pvar->session_id_len %d(program bug).",
			pvar->session_id_len);
		goto error;
	}

	b = buffer_init();
	if (b == NULL)
		goto error;

	ndone = 0;
	for (i = 0; i < ctx->nkeys; i++) {
		if (ctx->keys_seen[i])
			continue;

		buffer_clear(b);
		buffer_put_cstring(b, "hostkeys-prove-00@openssh.com");
		buffer_put_string(b, pvar->session_id, pvar->session_id_len);
		key_to_blob(ctx->keys[i], (char **)&blob, &bloblen);
		buffer_put_string(b, blob, bloblen);
		free(blob);
		blob = NULL;

		sig = buffer_get_string_msg(bsig, &siglen_i);
		siglen = siglen_i;
		// �蔲���Bhostkey algorithm ���g���̂� RSA �̎��݂̂Ȃ̂ŁA
		// �Ƃ肠���� KEY_ALGO_RSA ���w�肵�Ă����B
		ret = key_verify(ctx->keys[i], sig, siglen, buffer_ptr(b), buffer_len(b), KEY_ALGO_RSA);
		free(sig);
		sig = NULL;
		if (ret != 1) {
			logprintf(LOG_LEVEL_ERROR,
				"server gave bad signature for %s key %u",
				get_ssh2_hostkey_type_name_from_key(ctx->keys[i]), i);
			goto error;
		}
		ndone++;
	}

	if (ndone != ctx->nnew) {
		logprintf(LOG_LEVEL_FATAL,
			"Hostkey can not be updated because ndone != ctx->nnew (%u / %u)(program bug).",
			ndone, ctx->nnew);
		goto error;
	}

	update_known_hosts(pvar, ctx);

error:
	buffer_free(b);
	buffer_free(bsig);
	hostkeys_update_ctx_free(ctx);
}

//
// SSH�T�[�o�z�X�g��(known_hosts)�̎����X�V(OpenSSH 6.8 or later: host key rotation support)
//
// return 1: success
//        0: fail
//
int update_client_input_hostkeys(PTInstVar pvar, char *dataptr, int datalen)
{
	int success = 1;  // OpenSSH 6.8�̎����ł́A��ɐ����ŕԂ��悤�ɂȂ��Ă��邽�߁A
	                  // ����ɍ��킹�� Tera Term �ł������ƕԂ����Ƃɂ���B
	int len;
	size_t i;
	char *cp, *fp;
	unsigned char *blob = NULL;
	buffer_t *b = NULL;
	struct hostkeys_update_ctx *ctx = NULL;
	Key *key = NULL, **tmp;
	unsigned char *outmsg;

	// Tera Term�̐ݒ�ŁA���Y�@�\�̃I���I�t�𐧌�ł���悤�ɂ���B
	if (pvar->settings.UpdateHostkeys == SSH_UPDATE_HOSTKEYS_NO) {
		logputs(LOG_LEVEL_VERBOSE, "Hostkey was not updated because ts.UpdateHostkeys is disabled.");
		return 1;
	}

	ctx = calloc(1, sizeof(struct hostkeys_update_ctx));
	if (ctx == NULL)
		goto error;

	b = buffer_init();
	if (b == NULL)
		goto error;

	cp = buffer_append_space(b, datalen);
	memcpy(cp, dataptr, datalen);

	while (buffer_remain_len(b) > 0) {
		key_free(key);
		key = NULL;

		blob = buffer_get_string_msg(b, &len);
		key = key_from_blob(blob, len);
		if (key == NULL) {
			logprintf(LOG_LEVEL_ERROR, "Not found host key into blob %p (%d)", blob, len);
			goto error;
		}
		free(blob);
		blob = NULL;

		fp = key_fingerprint(key, SSH_FP_HEX, SSH_DIGEST_MD5);
		logprintf(LOG_LEVEL_VERBOSE, "Received %s host key %s",
		          get_ssh2_hostkey_type_name_from_key(key), fp);
		free(fp);

		// �����ꂽ�z�X�g�L�[�A���S���Y�������`�F�b�N����B
		if (check_hostkey_algorithm(pvar, key) == 0) {
			logprintf(LOG_LEVEL_VERBOSE, "%s host key is not permitted by ts.HostKeyOrder",
			          get_ssh2_hostkey_type_name_from_key(key));
			continue;
		}

		// Skip certs: Tera Term�ł͏ؖ����F�؂͖��T�|�[�g�B

		// �d�������L�[����M������G���[�Ƃ���B
		for (i = 0; i < ctx->nkeys; i++) {
			if (HOSTS_compare_public_key(key, ctx->keys[i]) == 1) {
				logprintf(LOG_LEVEL_ERROR, "Received duplicated %s host key",
				          get_ssh2_hostkey_type_name_from_key(key));
				goto error;
			}
		}

		// �L�[��o�^����B
		tmp = realloc(ctx->keys, (ctx->nkeys + 1)*sizeof(*ctx->keys));
		if (tmp == NULL) {
			logprintf(LOG_LEVEL_FATAL, "Not memory: realloc ctx->keys %d",
				ctx->nkeys);
			goto error;
		}
		ctx->keys = tmp;
		ctx->keys[ctx->nkeys++] = key;
		key = NULL;
	}

	if (ctx->nkeys == 0) {
		logputs(LOG_LEVEL_VERBOSE, "No host rotation key");
		goto error;
	}

	if ((ctx->keys_seen = calloc(ctx->nkeys, sizeof(*ctx->keys_seen))) == NULL) {
		logprintf(LOG_LEVEL_FATAL, "Not memory: calloc ctx->keys %d",
			ctx->nkeys);
		goto error;
	}

	HOSTS_hostkey_foreach(pvar, hostkeys_find, ctx);

	// �T�[�o�������Ă��������Q����A�����̌���V�K�ǉ�����̂��𐔂���B
	ctx->nnew = 0;
	for (i = 0; i < ctx->nkeys; i++) {
		if (!ctx->keys_seen[i])
			ctx->nnew++;
	}
	logprintf(LOG_LEVEL_VERBOSE, "%u keys from server: %u new, %u retained. %u to remove",
		ctx->nkeys, ctx->nnew, ctx->nkeys - ctx->nnew, ctx->nold);

	// �V�K�ǉ����錮�̓[�������Adeprecated�Ȍ������݂���B
	if (ctx->nnew == 0 && ctx->nold != 0) {
		update_known_hosts(pvar, ctx);

	}
	else if (ctx->nnew != 0) { // �V�K�ǉ�����ׂ��������݂���B
		buffer_clear(b);

		buffer_put_cstring(b, "hostkeys-prove-00@openssh.com");
		buffer_put_char(b, 1);  /* bool: want reply */

		for (i = 0; i < ctx->nkeys; i++) {
			if (ctx->keys_seen[i])
				continue;
			key_to_blob(ctx->keys[i], (char **) & blob, &len);
			buffer_put_string(b, blob, len);
			free(blob);
			blob = NULL;
		}

		len = buffer_len(b);
		outmsg = begin_send_packet(pvar, SSH2_MSG_GLOBAL_REQUEST, len);
		memcpy(outmsg, buffer_ptr(b), len);
		finish_send_packet(pvar);

		// SSH2_MSG_GLOBAL_REQUEST�̃��X�|���X�ɑΉ�����n���h����o�^����B
		client_register_global_confirm(client_global_hostkeys_private_confirm, ctx);
		ctx = NULL;   // callback�ŉ������̂ŁA�����ł�NULL�łԂ��Ă����B
	}

	success = 1;

error:
	buffer_free(b);
	hostkeys_update_ctx_free(ctx);
	free(blob);

	return (success);
}
