/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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
// PuTTY is copyright 1997-2021 Simon Tatham.

/*
 * putty プロジェクトは
 * - libputty.c/h
 *   PuTTY の機能を利用するインターフェース
 *     PuTTY の内部型との変換
 * - PuTTY のソースコード (プロジェクトに読み込むファイルは最小限)
 * -- 展開した PuTTY のソースファイルをそのまま読み込んだもの (Putty Files グループ内)
 * -- putty プロジェクト側にコピーしたソースコード (putty-import.c)
 *    static なために呼び出せない関数の static を外す
 *    putty で使う部分だけを抜粋 (putty で使わない部分で増えてしまう依存を抑制)
 * から静的ライブラリ libputty.lib を生成し、TTXSSH からリンクされて利用される。
 * そのため、TTXSSH 側の定義は持ち込まない。
 */

#include <stdio.h>
#include <windows.h>
#include <assert.h>

#include "ssh.h"
#include "mpint.h"

#include "libputty.h"


// pageant.c
typedef struct KeyListEntry {
    ptrlen blob, comment;
    uint32_t flags;
} KeyListEntry;
typedef struct KeyList {
    strbuf *raw_data;
    KeyListEntry *keys;
    size_t nkeys;
    bool broken;
} KeyList;

// putty.h
//   aqsync.c
extern void agent_query_synchronous(strbuf *in, void **out, int *outlen);

// pageant.c
extern void keylist_free(KeyList *kl);

// pageant.c
extern KeyList *pageant_get_keylist(unsigned ssh_version);

// putty.h
//   windows/winpgntc.c
extern bool agent_exists(void);


/*
 * for SSH2
 *   鍵の一覧を得る
 */
int putty_get_ssh2_keylist(unsigned char **keylist)
{
	KeyList *kl;
	int keylistlen = 0;
	strbuf *sb_keylist = strbuf_new();
	size_t i;

	kl = pageant_get_keylist(2);
	if (kl) {
		if (kl->broken) {
			keylist_free(kl);
			strbuf_free(sb_keylist);
			return 0;
		}

		put_uint32(sb_keylist, kl->nkeys);
		keylistlen += 4;
		for (i = 0; i < kl->nkeys; i++) {
			put_uint32(sb_keylist, kl->keys[i].blob.len);
			keylistlen += 4;
			put_datapl(sb_keylist, kl->keys[i].blob);
			keylistlen += kl->keys[i].blob.len;
			put_uint32(sb_keylist, kl->keys[i].comment.len);
			keylistlen += 4;
			put_datapl(sb_keylist, kl->keys[i].comment);
			keylistlen += kl->keys[i].comment.len;
		}
		keylist_free(kl);

		*keylist = strbuf_to_str(sb_keylist);

		return keylistlen;
	}

	strbuf_free(sb_keylist);
	return 0;
}

/*
 * for SSH2
 *   公開鍵とデータを渡し、
 *   秘密鍵による署名を得る
 *   ssh2userauth.c (PuTTY 0.76) から再構成
 */
void *putty_sign_ssh2_key(unsigned char *pubkey /* length(4byte) + data */,
                          unsigned char *data,
                          int datalen,
                          int *outlen)
{
	void *ret;

	unsigned char *response;
	void *vresponse;
	int response_len;
	int pubkeylen;
	strbuf *agentreq = strbuf_new_for_agent_query();
	int signflags = 0;

	put_byte(agentreq, SSH2_AGENTC_SIGN_REQUEST);
	pubkeylen = GET_32BIT_MSB_FIRST(pubkey);
	put_data(agentreq, pubkey, 4 + pubkeylen);
	put_uint32(agentreq, datalen);
	put_data(agentreq, data, datalen);
	put_uint32(agentreq, signflags);
	agent_query_synchronous(agentreq, &vresponse, &response_len);
	strbuf_free(agentreq);

	response = vresponse;
	if (response_len < 5 || response[4] != SSH2_AGENT_SIGN_RESPONSE) {
		sfree(response);
		return NULL;
	}

	ret = snewn(response_len-5, unsigned char);
	memcpy(ret, response+5, response_len-5);
	sfree(response);

	if (outlen)
		*outlen = response_len-5;

	return ret;
}

/*
 * for SSH1
 *   鍵の一覧を得る
 */
int putty_get_ssh1_keylist(unsigned char **keylist)
{
	KeyList *kl;
	int keylistlen = 0;
	strbuf *sb_keylist = strbuf_new();
	size_t i;

	kl = pageant_get_keylist(1);
	if (kl) {
		if (kl->broken) {
			keylist_free(kl);
			strbuf_free(sb_keylist);
			return 0;
		}

		put_uint32(sb_keylist, kl->nkeys);
		keylistlen += 4;
		for (i = 0; i < kl->nkeys; i++) {
			put_data(sb_keylist, kl->keys[i].blob.ptr, kl->keys[i].blob.len);
			keylistlen += kl->keys[i].blob.len;
			put_uint32(sb_keylist, kl->keys[i].comment.len);
			keylistlen += 4;
			put_datapl(sb_keylist, kl->keys[i].comment);
			keylistlen += kl->keys[i].comment.len;
		}
		keylist_free(kl);

		*keylist = strbuf_to_str(sb_keylist);

		return keylistlen;
	}

	strbuf_free(sb_keylist);
	return 0;
}

/*
 * for SSH1
 *   公開鍵と暗号化データを渡し
 *   復号データのハッシュを得る
 *   ssh1login.c (PuTTY 0.76) から再構成
 */
void *putty_hash_ssh1_challenge(unsigned char *pubkey,
                                int pubkeylen,
                                unsigned char *data,
                                int datalen,
                                unsigned char *session_id,
                                int *outlen)
{
	void *ret;

	unsigned char *response;
	void *vresponse;
	int response_len;
	strbuf *agentreq = strbuf_new_for_agent_query();

	put_byte(agentreq, SSH1_AGENTC_RSA_CHALLENGE);
	put_data(agentreq, pubkey, pubkeylen);
	put_data(agentreq, data, datalen);
	put_data(agentreq, session_id, 16);
	put_uint32(agentreq, 1); // response format
	agent_query_synchronous(agentreq, &vresponse, &response_len);
	strbuf_free(agentreq);

	response = vresponse;
	if (response_len < 5+16 || response[4] != SSH1_AGENT_RSA_RESPONSE) {
		sfree(response);
		return NULL;
	}

	ret = snewn(response_len-5, unsigned char);
	memcpy(ret, response+5, response_len-5);
	sfree(response);

	if (outlen)
		*outlen = response_len-5;

	return ret;
}

int putty_get_ssh1_keylen(unsigned char *key, int maxlen)
{
	return rsa_ssh1_public_blob_len(make_ptrlen(key, maxlen));
}

const char *putty_get_version()
{
	extern const char ver[]; /* in version.c */
	return ver;
}

void putty_agent_query_synchronous(void *in, int inlen, void **out, int *outlen)
{
	strbuf *buf = strbuf_new();

	put_data(buf, in, inlen);
	agent_query_synchronous(buf, out, outlen);
	strbuf_free(buf);
}

BOOL putty_agent_exists()
{
	if (agent_exists()) {
		return TRUE;
	}
	return FALSE;
}

