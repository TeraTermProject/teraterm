/*
 * Copyright (C) 2008-2017 TeraTerm Project
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
// PuTTY is copyright 1997-2007 Simon Tatham.

// pageant.h
// 本当は pageant.h を include 出来るようにする方がいいのかもしれないけれど
// 関数のプロトタイプ宣言もここにあるので取りあえずここで。
#define AGENT_MAX_MSGLEN 8192

// エラー応答用
#define SSH_AGENT_FAILURE_MSG "\x00\x00\x00\x01\x05"

// MISC.C
extern void safefree(void *);

// WINDOWS\WINPGNTC.C
extern int agent_exists(void);
extern void *agent_query(void *in, int inlen, void **out, int *outlen,
                       void (*callback)(void *, void *, int), void *callback_ctx);

int putty_get_ssh2_keylist(unsigned char **keylist);
void *putty_sign_ssh2_key(unsigned char *pubkey,
                          unsigned char *data,
                          int *outlen);
int putty_get_ssh1_keylist(unsigned char **keylist);
void *putty_hash_ssh1_challenge(unsigned char *pubkey,
                                int pubkeylen,
                                unsigned char *data,
                                int datalen,
                                unsigned char *session_id,
                                int *outlen);
int putty_get_ssh1_keylen(unsigned char *key,
                          int maxlen);
const char *putty_get_version();

static void *get_keylist1(int *length);
static void *get_keylist2(int *length);
