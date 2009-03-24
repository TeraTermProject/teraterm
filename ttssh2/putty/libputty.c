/*
 * Copyright(C) 2008 TeraTerm Project
 */
// PuTTY is copyright 1997-2007 Simon Tatham.

#include <windows.h>
#include <assert.h>

#include "ssh.h"
#include "libputty.h"

/*
 * for SSH2
 *   鍵の一覧を得る
 */
int putty_get_ssh2_keylist(unsigned char **keylist)
{
	int keylistlen;

	*keylist = get_keylist2(&keylistlen);
	if (*keylist == NULL){
		// 取得に失敗
		return 0;
	}
	return keylistlen;
}

/*
 * for SSH2
 *   公開鍵とデータ(同じく公開鍵)を渡し、
 *   公開鍵によって署名されたデータを得る
 */
void *putty_sign_ssh2_key(unsigned char *pubkey,
                          unsigned char *data,
                          int *outlen)
{
	void *ret;

	unsigned char *request, *response;
	void *vresponse;
	int resplen, retval;
	int pubkeylen, datalen, reqlen;

	pubkeylen = GET_32BIT(pubkey);
	datalen = GET_32BIT(data);
	reqlen = 4 + 1 + (4 + pubkeylen) + (4 + datalen);
	request = (unsigned char *)malloc(reqlen);

	// request length
	PUT_32BIT(request, reqlen);
	// request type
	request[4] = SSH2_AGENTC_SIGN_REQUEST;
	// public key (length + data)
	memcpy(request + 5, pubkey, 4 + pubkeylen);
	// sign data (length + data)
	memcpy(request + 5 + 4 + pubkeylen, data, 4 + datalen);

	retval = agent_query(request, reqlen, &vresponse, &resplen, NULL, NULL);
	assert(retval == 1);
	response = vresponse;
	if (resplen < 5 || response[4] != SSH2_AGENT_SIGN_RESPONSE)
		return NULL;

	ret = snewn(resplen-5, unsigned char);
	memcpy(ret, response+5, resplen-5);
	sfree(response);

	if (outlen)
		*outlen = resplen-5;

	return ret;
}

/*
 * for SSH1
 *   鍵の一覧を得る
 */
int putty_get_ssh1_keylist(unsigned char **keylist)
{
	int keylistlen;

	*keylist = get_keylist1(&keylistlen);
	if (*keylist == NULL){
		// 取得に失敗
		return 0;
	}
	return keylistlen;
}

/*
 * for SSH1
 *   公開鍵と暗号化データを渡し
 *   復号データのハッシュを得る
 */
void *putty_hash_ssh1_challenge(unsigned char *pubkey,
                                int pubkeylen,
                                unsigned char *data,
                                int datalen,
                                unsigned char *session_id,
                                int *outlen)
{
	void *ret;

	unsigned char *request, *response, *p;
	void *vresponse;
	int resplen, retval;
	int reqlen;

	reqlen = 4 + 1 + pubkeylen + datalen + 16 + 4;
	request = (unsigned char *)malloc(reqlen);
	p = request;

	// request length
	PUT_32BIT(request, reqlen);
	// request type
	request[4] = SSH1_AGENTC_RSA_CHALLENGE;
	p += 5;

	// public key
	memcpy(p, pubkey, pubkeylen);
	p += pubkeylen;
	// challange from server
	memcpy(p, data, datalen);
	p += datalen;
	// session_id
	memcpy(p, session_id, 16);
	p += 16;
	// terminator?
	PUT_32BIT(p, 1);

	retval = agent_query(request, reqlen, &vresponse, &resplen, NULL, NULL);
	assert(retval == 1);
	response = vresponse;
	if (resplen < 5 || response[4] != SSH1_AGENT_RSA_RESPONSE)
		return NULL;

	ret = snewn(resplen-5, unsigned char);
	memcpy(ret, response+5, resplen-5);
	sfree(response);

	if (outlen)
		*outlen = resplen-5;

	return ret;
}

int putty_get_ssh1_keylen(unsigned char *key,
                          int maxlen)
{
	return rsa_public_blob_len(key, maxlen);
}


/*
 * Following functions are copied from putty source.
 */


// SSHRSA.C
/* Given a public blob, determine its length. */
int rsa_public_blob_len(void *data, int maxlen)
{
	unsigned char *p = (unsigned char *)data;
	int n;

	if (maxlen < 4)
		return -1;
	p += 4;			       /* length word */
	maxlen -= 4;

	n = ssh1_read_bignum(p, maxlen, NULL);    /* exponent */
	if (n < 0)
		return -1;
	p += n;

	n = ssh1_read_bignum(p, maxlen, NULL);    /* modulus */
	if (n < 0)
		return -1;
	p += n;

	return p - (unsigned char *)data;
}

// WINDOWS\WINPGNT.C
/*
 * Acquire a keylist1 from the primary Pageant; this means either
 * calling make_keylist1 (if that's us) or sending a message to the
 * primary Pageant (if it's not).
 */
static void *get_keylist1(int *length)
{
	void *ret;

	unsigned char request[5], *response;
	void *vresponse;
	int resplen, retval;
	request[4] = SSH1_AGENTC_REQUEST_RSA_IDENTITIES;
	PUT_32BIT(request, 4);

	retval = agent_query(request, 5, &vresponse, &resplen, NULL, NULL);
	assert(retval == 1);
	response = vresponse;
	if (resplen < 5 || response[4] != SSH1_AGENT_RSA_IDENTITIES_ANSWER)
		return NULL;

	ret = snewn(resplen-5, unsigned char);
	memcpy(ret, response+5, resplen-5);
	sfree(response);

	if (length)
		*length = resplen-5;

	return ret;
}

/*
 * Acquire a keylist2 from the primary Pageant; this means either
 * calling make_keylist2 (if that's us) or sending a message to the
 * primary Pageant (if it's not).
 */
static void *get_keylist2(int *length)
{
	void *ret;

	unsigned char request[5], *response;
	void *vresponse;
	int resplen, retval;

	request[4] = SSH2_AGENTC_REQUEST_IDENTITIES;
	PUT_32BIT(request, 4);

	retval = agent_query(request, 5, &vresponse, &resplen, NULL, NULL);
	assert(retval == 1);
	response = vresponse;
	if (resplen < 5 || response[4] != SSH2_AGENT_IDENTITIES_ANSWER)
		return NULL;

	ret = snewn(resplen-5, unsigned char);
	memcpy(ret, response+5, resplen-5);
	sfree(response);

	if (length)
		*length = resplen-5;

	return ret;
}

// WINDOWS\WINDOW.C
/*
 * Print a modal (Really Bad) message box and perform a fatal exit.
 */
void modalfatalbox(char *fmt, ...)
{
	va_list ap;
	char *stuff, morestuff[100];

	va_start(ap, fmt);
	stuff = dupvprintf(fmt, ap);
	va_end(ap);
	sprintf(morestuff, "%.70s Fatal Error", "TTSSH");
	MessageBox(NULL, stuff, morestuff,
	           MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
	sfree(stuff);
}
