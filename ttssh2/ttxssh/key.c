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

#include "key.h"
#include "kex.h"

#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/ecdsa.h>

#define INTBLOB_LEN 20
#define SIGBLOB_LEN (2*INTBLOB_LEN)

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
	EVP_MD_CTX md;
	unsigned char digest[EVP_MAX_MD_SIZE], *sigblob;
	unsigned int len, dlen;
	int ret;
	char *ptr;

	OpenSSL_add_all_digests();

	if (key == NULL) {
		return -2;
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
			return -3;
		}
		ptr += len;
	}

	// step2
	len = get_uint32_MSBfirst(ptr);
	ptr += 4;
	sigblob = ptr;
	ptr += len;

	if (len != SIGBLOB_LEN) {
		return -4;
	}

	/* parse signature */
	if ((sig = DSA_SIG_new()) == NULL)
		return -5;
	if ((sig->r = BN_new()) == NULL)
		return -6;
	if ((sig->s = BN_new()) == NULL)
		return -7;
	BN_bin2bn(sigblob, INTBLOB_LEN, sig->r);
	BN_bin2bn(sigblob+ INTBLOB_LEN, INTBLOB_LEN, sig->s);

	/* sha1 the data */
	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, data, datalen);
	EVP_DigestFinal(&md, digest, &dlen);

	ret = DSA_do_verify(digest, dlen, sig, key);
	memset(digest, 'd', sizeof(digest));

	DSA_SIG_free(sig);

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
		return 1; // error

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
                   u_char *data, u_int datalen)
{
	const EVP_MD *evp_md;
	EVP_MD_CTX md;
	//	char *ktype;
	u_char digest[EVP_MAX_MD_SIZE], *sigblob;
	u_int len, dlen, modlen;
//	int rlen, ret, nid;
	int ret, nid;
	char *ptr;

	OpenSSL_add_all_digests();

	if (key == NULL) {
		return -2;
	}
	if (BN_num_bits(key->n) < SSH_RSA_MINIMUM_MODULUS_SIZE) {
		return -3;
	}
	//debug_print(41, signature, signaturelen);
	ptr = signature;

	// step1
	len = get_uint32_MSBfirst(ptr);
	ptr += 4;
	if (strncmp("ssh-rsa", ptr, len) != 0) {
		return -4;
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
		return -5;

	} else if (len < modlen) {
		u_int diff = modlen - len;
		sigblob = realloc(sigblob, modlen);
		memmove(sigblob + diff, sigblob, len);
		memset(sigblob, 0, diff);
		len = modlen;
	}
	
	/* sha1 the data */
	//	nid = (datafellows & SSH_BUG_RSASIGMD5) ? NID_md5 : NID_sha1;
	nid = NID_sha1;
	if ((evp_md = EVP_get_digestbynid(nid)) == NULL) {
		//error("ssh_rsa_verify: EVP_get_digestbynid %d failed", nid);
		return -6;
	}
	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, data, datalen);
	EVP_DigestFinal(&md, digest, &dlen);

	ret = openssh_RSA_verify(nid, digest, dlen, sigblob, len, key);

	memset(digest, 'd', sizeof(digest));
	memset(sigblob, 's', len);
	//free(sigblob);
	//debug("ssh_rsa_verify: signature %scorrect", (ret==0) ? "in" : "");

	return ret;
}

int ssh_ecdsa_verify(EC_KEY *key, ssh_keytype keytype,
                     u_char *signature, u_int signaturelen,
                     u_char *data, u_int datalen)
{
	ECDSA_SIG *sig;
	const EVP_MD *evp_md;
	EVP_MD_CTX md;
	unsigned char digest[EVP_MAX_MD_SIZE], *sigblob;
	unsigned int len, dlen;
	int ret, nid = NID_undef;
	char *ptr;

	OpenSSL_add_all_digests();

	if (key == NULL) {
		return -2;
	}

	ptr = signature;

	len = get_uint32_MSBfirst(ptr);
	ptr += 4;
	if (strncmp(get_ssh_keytype_name(keytype), ptr, len) != 0) {
		return -3;
	}
	ptr += len;

	len = get_uint32_MSBfirst(ptr);
	ptr += 4;
	sigblob = ptr;
	ptr += len;

	/* parse signature */
	if ((sig = ECDSA_SIG_new()) == NULL)
		return -4;
	if ((sig->r = BN_new()) == NULL)
		return -5;
	if ((sig->s = BN_new()) == NULL)
		return -6;

	buffer_get_bignum2(&sigblob, sig->r);
	buffer_get_bignum2(&sigblob, sig->s);
	if (sigblob != ptr) {
		return -7;
	}

	/* hash the data */
	nid = keytype_to_hash_nid(keytype);
	if ((evp_md = EVP_get_digestbynid(nid)) == NULL) {
		return -8;
	}
	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, data, datalen);
	EVP_DigestFinal(&md, digest, &dlen);

	ret = ECDSA_do_verify(digest, dlen, sig, key);
	memset(digest, 'd', sizeof(digest));

	ECDSA_SIG_free(sig);

	return ret;
}

int key_verify(Key *key,
               unsigned char *signature, unsigned int signaturelen,
               unsigned char *data, unsigned int datalen)
{
	int ret = 0;

	switch (key->type) {
	case KEY_RSA:
		ret = ssh_rsa_verify(key->rsa, signature, signaturelen, data, datalen);
		break;
	case KEY_DSA:
		ret = ssh_dss_verify(key->dsa, signature, signaturelen, data, datalen);
		break;
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		ret = ssh_ecdsa_verify(key->ecdsa, key->type, signature, signaturelen, data, datalen);
		break;
	default:
		return -1;
	}

	return (ret);   // success
}

//
// RSA構造体の複製
//
RSA *duplicate_RSA(RSA *src)
{
	RSA *rsa = NULL;

	rsa = RSA_new();
	if (rsa == NULL)
		goto error;
	rsa->n = BN_new();
	rsa->e = BN_new();
	if (rsa->n == NULL || rsa->e == NULL) {
		RSA_free(rsa);
		goto error;
	}

	// 深いコピー(deep copy)を行う。浅いコピー(shallow copy)はNG。
	BN_copy(rsa->n, src->n);
	BN_copy(rsa->e, src->e);

error:
	return (rsa);
}


//
// DSA構造体の複製
//
DSA *duplicate_DSA(DSA *src)
{
	DSA *dsa = NULL;

	dsa = DSA_new();
	if (dsa == NULL)
		goto error;
	dsa->p = BN_new();
	dsa->q = BN_new();
	dsa->g = BN_new();
	dsa->pub_key = BN_new();
	if (dsa->p == NULL ||
	    dsa->q == NULL ||
	    dsa->g == NULL ||
	    dsa->pub_key == NULL) {
		DSA_free(dsa);
		goto error;
	}

	// 深いコピー(deep copy)を行う。浅いコピー(shallow copy)はNG。
	BN_copy(dsa->p, src->p);
	BN_copy(dsa->q, src->q);
	BN_copy(dsa->g, src->g);
	BN_copy(dsa->pub_key, src->pub_key);

error:
	return (dsa);
}


char* key_fingerprint_raw(Key *k, enum fp_type dgst_type, int *dgst_raw_length)
{
	const EVP_MD *md = NULL;
	EVP_MD_CTX ctx;
	char *blob = NULL;
	char *retval = NULL;
	int len = 0;
	int nlen, elen;
	RSA *rsa;

	*dgst_raw_length = 0;

	switch (dgst_type) {
	case SSH_FP_MD5:
		md = EVP_md5();
		break;
	case SSH_FP_SHA1:
		md = EVP_sha1();
		break;
	case SSH_FP_SHA256:
		md = EVP_sha256();
		break;
	default:
		md = EVP_md5();
	}

	switch (k->type) {
	case KEY_RSA1:
		rsa = make_key(NULL, k->bits, k->exp, k->mod);
		nlen = BN_num_bytes(rsa->n);
		elen = BN_num_bytes(rsa->e);
		len = nlen + elen;
		blob = malloc(len);
		if (blob == NULL) {
			// TODO:
		}
		BN_bn2bin(rsa->n, blob);
		BN_bn2bin(rsa->e, blob + nlen);
		RSA_free(rsa);
		break;

	case KEY_DSA:
	case KEY_RSA:
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		key_to_blob(k, &blob, &len);
		break;

	case KEY_UNSPEC:
		return retval;
		break;

	default:
		//fatal("key_fingerprint_raw: bad key type %d", k->type);
		break;
	}

	if (blob != NULL) {
		retval = malloc(EVP_MAX_MD_SIZE);
		if (retval == NULL) {
			// TODO:
		}
		EVP_DigestInit(&ctx, md);
		EVP_DigestUpdate(&ctx, blob, len);
		EVP_DigestFinal(&ctx, retval, dgst_raw_length);
		memset(blob, 0, len);
		free(blob);
	} else {
		//fatal("key_fingerprint_raw: blob is null");
	}
	return retval;
}


const char *
ssh_key_type(const Key *k)
{
	switch (k->type) {
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
	}
	return "unknown";
}

unsigned int
key_size(const Key *k)
{
	switch (k->type) {
	case KEY_RSA1:
		// SSH1の場合は key->rsa と key->dsa は NULL であるので、使わない。
		return k->bits;
	case KEY_RSA:
		return BN_num_bits(k->rsa->n);
	case KEY_DSA:
		return BN_num_bits(k->dsa->p);
	case KEY_ECDSA256:
		return 256;
	case KEY_ECDSA384:
		return 384;
	case KEY_ECDSA521:
		return 521;
	}
	return 0;
}

// based on OpenSSH 5.1
#define	FLDBASE		8
#define	FLDSIZE_Y	(FLDBASE + 1)
#define	FLDSIZE_X	(FLDBASE * 2 + 1)
static char *
key_fingerprint_randomart(u_char *dgst_raw, u_int dgst_raw_len, const Key *k)
{
	/*
	 * Chars to be used after each other every time the worm
	 * intersects with itself.  Matter of taste.
	 */
	char	*augmentation_string = " .o+=*BOX@%&#/^SE";
	char	*retval, *p;
	unsigned char	 field[FLDSIZE_X][FLDSIZE_Y];
	unsigned int	 i, b;
	int	 x, y;
	size_t	 len = strlen(augmentation_string) - 1;

	retval = calloc(1, (FLDSIZE_X + 3 + 1) * (FLDSIZE_Y + 2));

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
			field[x][y]++;
			input = input >> 2;
		}
	}

	/* mark starting point and end point*/
	field[FLDSIZE_X / 2][FLDSIZE_Y / 2] = len - 1;
	field[x][y] = len;

	/* fill in retval */
	_snprintf_s(retval, FLDSIZE_X, _TRUNCATE, "+--[%4s %4u]", ssh_key_type(k), key_size(k));
	p = strchr(retval, '\0');

	/* output upper border */
	for (i = p - retval - 1; i < FLDSIZE_X; i++)
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
	for (i = 0; i < FLDSIZE_X; i++)
		*p++ = '-';
	*p++ = '+';

	return retval;
}
#undef	FLDBASE	
#undef	FLDSIZE_Y
#undef	FLDSIZE_X

//
// fingerprint（指紋：ホスト公開鍵のハッシュ）を生成する
//
char *key_fingerprint(Key *key, enum fp_rep dgst_rep)
{
	char *retval = NULL;
	unsigned char *dgst_raw;
	int dgst_raw_len;
	int i, retval_len;

	// fingerprintのハッシュ値（バイナリ）を求める
	dgst_raw = key_fingerprint_raw(key, SSH_FP_MD5, &dgst_raw_len);

	if (dgst_rep == SSH_FP_HEX) {
		// 16進表記へ変換する
		retval_len = dgst_raw_len * 3 + 1;
		retval = malloc(retval_len);
		retval[0] = '\0';
		for (i = 0; i < dgst_raw_len; i++) {
			char hex[4];
			_snprintf_s(hex, sizeof(hex), _TRUNCATE, "%02x:", dgst_raw[i]);
			strncat_s(retval, retval_len, hex, _TRUNCATE);
		}

		/* Remove the trailing ':' character */
		retval[(dgst_raw_len * 3) - 1] = '\0';

	} else if (dgst_rep == SSH_FP_RANDOMART) {
		retval = key_fingerprint_randomart(dgst_raw, dgst_raw_len, key);

	} else {

	}

	memset(dgst_raw, 0, dgst_raw_len);
	free(dgst_raw);

	return (retval);
}


//
// キーのメモリ領域解放
//
void key_free(Key *key)
{
	if (key == NULL) {
		return;
	}

	switch (key->type) {
		case KEY_RSA1:
		case KEY_RSA:
			if (key->rsa != NULL)
				RSA_free(key->rsa);
			key->rsa = NULL;
			break;

		case KEY_DSA:
			if (key->dsa != NULL)
				DSA_free(key->dsa);
			key->dsa = NULL;
			break;

		case KEY_ECDSA256:
		case KEY_ECDSA384:
		case KEY_ECDSA521:
			if (key->ecdsa != NULL)
				EC_KEY_free(key->ecdsa);
			key->ecdsa = NULL;
			break;
	}
	free(key);
}

//
// キーから文字列を返却する
//
char *get_sshname_from_key(Key *key)
{
	return get_ssh_keytype_name(key->type);
}

//
// キー文字列から種別を判定する
//
ssh_keytype get_keytype_from_name(char *name)
{
	if (strcmp(name, "rsa1") == 0) {
		return KEY_RSA1;
	} else if (strcmp(name, "rsa") == 0) {
		return KEY_RSA;
	} else if (strcmp(name, "dsa") == 0) {
		return KEY_DSA;
	} else if (strcmp(name, "ssh-rsa") == 0) {
		return KEY_RSA;
	} else if (strcmp(name, "ssh-dss") == 0) {
		return KEY_DSA;
	} else if (strcmp(name, "ecdsa-sha2-nistp256") == 0) {
		return KEY_ECDSA256;
	} else if (strcmp(name, "ecdsa-sha2-nistp384") == 0) {
		return KEY_ECDSA384;
	} else if (strcmp(name, "ecdsa-sha2-nistp521") == 0) {
		return KEY_ECDSA521;
	}
	return KEY_UNSPEC;
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
// キー情報からバッファへ変換する (for SSH2)
// NOTE:
//
int key_to_blob(Key *key, char **blobp, int *lenp)
{
	buffer_t *b;
	char *sshname, *tmp;
	int len;
	int ret = 1;  // success

	b = buffer_init();
	sshname = get_sshname_from_key(key);

	switch (key->type) {
	case KEY_RSA:
		buffer_put_string(b, sshname, strlen(sshname));
		buffer_put_bignum2(b, key->rsa->e);
		buffer_put_bignum2(b, key->rsa->n);
		break;
	case KEY_DSA:
		buffer_put_string(b, sshname, strlen(sshname));
		buffer_put_bignum2(b, key->dsa->p);
		buffer_put_bignum2(b, key->dsa->q);
		buffer_put_bignum2(b, key->dsa->g);
		buffer_put_bignum2(b, key->dsa->pub_key);
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
// バッファからキー情報を取り出す(for SSH2)
// NOTE: 返値はアロケート領域になるので、呼び出し側で解放すること。
//
Key *key_from_blob(char *data, int blen)
{
	int keynamelen;
	char key[128];
	RSA *rsa = NULL;
	DSA *dsa = NULL;
	EC_KEY *ecdsa = NULL;
	EC_POINT *q = NULL;
	char *curve = NULL;
	Key *hostkey;  // hostkey
	ssh_keytype type;

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

	type = get_keytype_from_name(key);

	switch (type) {
	case KEY_RSA: // RSA key
		rsa = RSA_new();
		if (rsa == NULL) {
			goto error;
		}
		rsa->n = BN_new();
		rsa->e = BN_new();
		if (rsa->n == NULL || rsa->e == NULL) {
			goto error;
		}

		buffer_get_bignum2(&data, rsa->e);
		buffer_get_bignum2(&data, rsa->n);

		hostkey->type = type;
		hostkey->rsa = rsa;
		break;

	case KEY_DSA: // DSA key
		dsa = DSA_new();
		if (dsa == NULL) {
			goto error;
		}
		dsa->p = BN_new();
		dsa->q = BN_new();
		dsa->g = BN_new();
		dsa->pub_key = BN_new();
		if (dsa->p == NULL ||
		    dsa->q == NULL ||
		    dsa->g == NULL ||
		    dsa->pub_key == NULL) {
			goto error;
		}

		buffer_get_bignum2(&data, dsa->p);
		buffer_get_bignum2(&data, dsa->q);
		buffer_get_bignum2(&data, dsa->g);
		buffer_get_bignum2(&data, dsa->pub_key);

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

	return NULL;
}


BOOL generate_SSH2_keysign(Key *keypair, char **sigptr, int *siglen, char *data, int datalen)
{
	buffer_t *msg = NULL;
	char *s;

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		return FALSE;
	}

	switch (keypair->type) {
	case KEY_RSA: // RSA
	{
		const EVP_MD *evp_md = EVP_sha1();
		EVP_MD_CTX md;
		u_char digest[EVP_MAX_MD_SIZE], *sig;
		u_int slen, dlen, len;
		int ok, nid = NID_sha1;

		// ダイジェスト値の計算
		EVP_DigestInit(&md, evp_md);
		EVP_DigestUpdate(&md, data, datalen);
		EVP_DigestFinal(&md, digest, &dlen);

		slen = RSA_size(keypair->rsa);
		sig = malloc(slen);
		if (sig == NULL)
			goto error;

		// 電子署名を計算
		ok = RSA_sign(nid, digest, dlen, sig, &len, keypair->rsa);
		memset(digest, 'd', sizeof(digest));
		if (ok != 1) { // error
			free(sig);
			goto error;
		}
		// 署名のサイズがバッファより小さい場合、後ろへずらす。先頭はゼロで埋める。
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

		s = get_sshname_from_key(keypair);
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
		EVP_MD_CTX md;
		u_char digest[EVP_MAX_MD_SIZE], sigblob[SIGBLOB_LEN];
		u_int rlen, slen, len, dlen;

		// ダイジェストの計算
		EVP_DigestInit(&md, evp_md);
		EVP_DigestUpdate(&md, data, datalen);
		EVP_DigestFinal(&md, digest, &dlen);

		// DSA電子署名を計算
		sig = DSA_do_sign(digest, dlen, keypair->dsa);
		memset(digest, 'd', sizeof(digest));
		if (sig == NULL) {
			goto error;
		}

		// BIGNUMからバイナリ値への変換
		rlen = BN_num_bytes(sig->r);
		slen = BN_num_bytes(sig->s);
		if (rlen > INTBLOB_LEN || slen > INTBLOB_LEN) {
			DSA_SIG_free(sig);
			goto error;
		}
		memset(sigblob, 0, SIGBLOB_LEN);
		BN_bn2bin(sig->r, sigblob+ SIGBLOB_LEN - INTBLOB_LEN - rlen);
		BN_bn2bin(sig->s, sigblob+ SIGBLOB_LEN - slen);
		DSA_SIG_free(sig);

		// setting
		s = get_sshname_from_key(keypair);
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
		EVP_MD_CTX md;
		u_char digest[EVP_MAX_MD_SIZE];
		u_int len, dlen, nid;
		buffer_t *buf2 = NULL;

		nid = keytype_to_hash_nid(keypair->type);
		if ((evp_md = EVP_get_digestbynid(nid)) == NULL) {
			goto error;
		}
		EVP_DigestInit(&md, evp_md);
		EVP_DigestUpdate(&md, data, datalen);
		EVP_DigestFinal(&md, digest, &dlen);

		sig = ECDSA_do_sign(digest, dlen, keypair->ecdsa);
		memset(digest, 'd', sizeof(digest));

		if (sig == NULL) {
			goto error;
		}

		buf2 = buffer_init();
		if (buf2 == NULL) {
			// TODO: error check
			goto error;
		}
		buffer_put_bignum2(buf2, sig->r);
		buffer_put_bignum2(buf2, sig->s);
		ECDSA_SIG_free(sig);

		s = get_sshname_from_key(keypair);
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

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		return FALSE;
	}

	keypair = pvar->auth_state.cur_cred.key_pair;

	switch (keypair->type) {
	case KEY_RSA: // RSA
		s = get_sshname_from_key(keypair);
		buffer_put_string(msg, s, strlen(s));
		buffer_put_bignum2(msg, keypair->rsa->e); // 公開指数
		buffer_put_bignum2(msg, keypair->rsa->n); // p×q
		break;
	case KEY_DSA: // DSA
		s = get_sshname_from_key(keypair);
		buffer_put_string(msg, s, strlen(s));
		buffer_put_bignum2(msg, keypair->dsa->p); // 素数
		buffer_put_bignum2(msg, keypair->dsa->q); // (p-1)の素因数
		buffer_put_bignum2(msg, keypair->dsa->g); // 整数
		buffer_put_bignum2(msg, keypair->dsa->pub_key); // 公開鍵
		break;
	case KEY_ECDSA256: // ECDSA
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		s = get_sshname_from_key(keypair);
		buffer_put_string(msg, s, strlen(s));
		tmp = curve_keytype_to_name(keypair->type);
		buffer_put_string(msg, tmp, strlen(tmp));
		buffer_put_ecpoint(msg, EC_KEY_get0_group(keypair->ecdsa),
		                        EC_KEY_get0_public_key(keypair->ecdsa));
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
