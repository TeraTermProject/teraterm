/*
 * Copyright (C) 2023- TeraTerm Project
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
 * Referred to PuTTY and RLogin
 *  License of PuTTY
 *   PuTTY is copyright 1997-2022 Simon Tatham.
 *  License of RLogin
 *   Copyright (c) 2017 Culti
 */

#include <stdio.h>
#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdint.h>
#include <assert.h>
#include <vector>
#include <windows.h>
#include <lmcons.h>	// for UNLEN
#include "libputty.h"
#include "sha256.h"

#define PUTTY_SHM		1	// pageant shared memory
#define PUTTY_NAMEDPIPE	1	// pageant named pipe
#define MS_NAMEDPIPE	1	// Microsoft agent
#define SSH1_ENABLE		1

// SSH Agent
//	Message numbers
//	https://datatracker.ietf.org/doc/html/draft-miller-ssh-agent-04#section-5.1

// requests from the client to the agent
#define SSH_AGENTC_REQUEST_IDENTITIES			11
#define SSH_AGENTC_SIGN_REQUEST					13
#define SSH_AGENTC_ADD_IDENTITY					17
#define SSH_AGENTC_REMOVE_IDENTITY				18
#define SSH_AGENTC_REMOVE_ALL_IDENTITIES		19
#define SSH_AGENTC_EXTENSION					27

// replies from the agent to the client
#define SSH_AGENT_FAILURE						5
#define SSH_AGENT_SUCCESS						6
#define SSH_AGENT_EXTENSION_FAILURE				28
#define SSH_AGENT_IDENTITIES_ANSWER				12
#define SSH_AGENT_SIGN_RESPONSE					14

// ssh1
#if SSH1_ENABLE
#define SSH1_AGENTC_REQUEST_RSA_IDENTITIES		1
#define SSH1_AGENT_RSA_IDENTITIES_ANSWER		2
#define SSH1_AGENTC_RSA_CHALLENGE				3
#define SSH1_AGENT_RSA_RESPONSE					4
#endif

#if PUTTY_SHM
static PSID usersid;
#endif

static uint32_t get_uint32(const uint8_t *p)
{
	return (((uint32_t)p[3]		 ) | ((uint32_t)p[2] <<	 8) |
			((uint32_t)p[1] << 16) | ((uint32_t)p[0] << 24));
}

#if SSH1_ENABLE
static uint16_t get_uint16(const uint8_t *p)
{
	return ((uint32_t)p[1] | ((uint32_t)p[0] << 8));
}
#endif

/**
 *	pageant の named pipe名の一部
 *	from putty windows/utils/cryptapi.c
 *
 *	@param		realname	元になる文字列
 *	@return		named pipe名の一部
 *				サインイン中は同一文字列が返る
 *				不要になったら free()
 *
 *	TODO
 *		CryptProtectMemory() API は比較的新しい Windows のみと思われる
 */
static char *capi_obfuscate_string(const char *realname)
{
	char *cryptdata;
	int cryptlen;
	unsigned char digest[32];
	char retbuf[65];
	int i;

	cryptlen = (int)(strlen(realname) + 1);
	cryptlen += CRYPTPROTECTMEMORY_BLOCK_SIZE - 1;
	cryptlen /= CRYPTPROTECTMEMORY_BLOCK_SIZE;
	cryptlen *= CRYPTPROTECTMEMORY_BLOCK_SIZE;

	cryptdata = (char *)malloc(cryptlen);
	memset(cryptdata, 0, cryptlen);
	memcpy(cryptdata, realname, strlen(realname));

	/*
	 * CRYPTPROTECTMEMORY_CROSS_PROCESS causes CryptProtectMemory to
	 * use the same key in all processes with this user id, meaning
	 * that the next PuTTY process calling this function with the same
	 * input will get the same data.
	 *
	 * (Contrast with CryptProtectData, which invents a new session
	 * key every time since its API permits returning more data than
	 * was input, so calling _that_ and hashing the output would not
	 * be stable.)
	 *
	 * We don't worry too much if this doesn't work for some reason.
	 * Omitting this step still has _some_ privacy value (in that
	 * another user can test-hash things to confirm guesses as to
	 * where you might be connecting to, but cannot invert SHA-256 in
	 * the absence of any plausible guess). So we don't abort if we
	 * can't call CryptProtectMemory at all, or if it fails.
	 */
	CryptProtectMemory(cryptdata, cryptlen,
					   CRYPTPROTECTMEMORY_CROSS_PROCESS);

	/*
	 * We don't want to give away the length of the hostname either,
	 * so having got it back out of CryptProtectMemory we now hash it.
	 */
	assert(cryptlen == 16);
	uint8_t buf[4+16] = {0};
	buf[3] = 0x10;	// = 0x00000010
	memcpy(&buf[4], cryptdata, cryptlen);
	sha256(&buf[0], sizeof(buf), digest);
	free(cryptdata);

	/*
	 * Finally, make printable.
	 */
	retbuf[0] = 0;
	for (i = 0; i < 32; i++) {
		char s[4];
		sprintf_s(s, "%02x", digest[i]);
		strcat_s(retbuf, s);
	}

	return _strdup(retbuf);
}

/**
 *	pagent named pipe名
 *	from PuTTY windows/utils/agent_named_pipe_name.c
 */
static char *agent_named_pipe_name(void)
{
	char user_name[UNLEN+1];
	DWORD len = _countof(user_name);
	BOOL r = GetUserNameA(user_name, &len);
	if (r == 0) {
		return NULL;
	}
	char *suffix = capi_obfuscate_string("Pageant");
	// asprintf(&pipename, "\\\\.\\pipe\\pageant.%s.%s", user_name, suffix);
	const char *base = "\\\\.\\pipe\\pageant.";
	size_t pipe_len = strlen(base) + 2 + strlen(user_name) + strlen(suffix);
	char *pipename = (char *)malloc(pipe_len);
	strcpy_s(pipename, pipe_len, base);
	strcat_s(pipename, pipe_len, user_name);
	strcat_s(pipename, pipe_len, ".");
	strcat_s(pipename, pipe_len, suffix);
	free(suffix);
	return pipename;
}

/**
 *	バッファ操作
 */
class Buffer {
public:
	virtual ~Buffer() {
		clear();
	}
	size_t size() const
	{
		return buf_.size();
	}
	void clear()
	{
		const size_t size = buf_.size();
		if (size > 0) {
			SecureZeroMemory(&buf_[0], size);
			buf_.clear();
		}
	}
	void *get_ptr() const
	{
		if (buf_.size() == 0) {
			return NULL;
		}
		return (void *)&buf_[0];
	}
	void append_array(const void *ptr, size_t len) {
		const uint8_t *u8ptr = (uint8_t *)ptr;
		buf_.insert(buf_.end(), &u8ptr[0], &u8ptr[len]);
	}
	void append_byte(uint8_t u8)
	{
		buf_.push_back(u8);
	}
	void append_uint32(uint32_t u32)
	{
		buf_.push_back((u32 >> (8*3)) & 0xff);
		buf_.push_back((u32 >> (8*2)) & 0xff);
		buf_.push_back((u32 >> (8*1)) & 0xff);
		buf_.push_back((u32 >> (8*0)) & 0xff);
	}
	/**
	 *	mallocした領域に内容をコピーして返す
	 */
	void *get_mallocdbuf(size_t *size)
	{
		size_t len = buf_.size();
		*size = len;
		void *p = NULL;
		if (len > 0) {
			p = malloc(len);
			memcpy(p, &buf_[0], len);
		}
		return p;
	}
	/**
	 *	バッファの先頭に追加する
	 */
	void prepend_uint32(uint32_t u32)
	{
		Buffer new_buf;
		new_buf.append_uint32(u32);
		new_buf.buf_.insert(new_buf.buf_.end(), buf_.begin(), buf_.end());
		buf_.swap(new_buf.buf_);
	}
private:
	std::vector<uint8_t> buf_;
};

/**
 *	Microsoft named pipe 名を取得
 *
 *	環境変数 SSH_AUTH_SOCK が設定されていれば、その値を返す
 *	設定されていなければデフォルトを返す
 *
 *	@return	pipe name (不要になったら free() すること)
 */
#if MS_NAMEDPIPE
static char *get_ms_namedpipe(void)
{
	static const char *var_name = "SSH_AUTH_SOCK";
	static const char *pipename_default = "\\\\.\\pipe\\openssh-ssh-agent";
	char *var_ptr = NULL;
	size_t var_size = 0;
	getenv_s(&var_size, NULL, 0, var_name);
	if (var_size != 0) {
		var_ptr = (char*)malloc(var_size);
		getenv_s(&var_size, var_ptr, var_size, var_name);
	}
	if (var_ptr == NULL) {
		var_ptr = _strdup(pipename_default);
	}
	return var_ptr;
}
#endif

/**
 *	agentと通信,named pipe経由
 *	RLogin CMainFrame::WageantQuery() MainFrm.cpp を参考にした
 */
#if PUTTY_NAMEDPIPE || MS_NAMEDPIPE
static BOOL query_namedpipe(const char *pipename, const Buffer &request, Buffer &reply)
{
	BOOL r = FALSE;
	std::vector<BYTE> read_buffer(4096);
	DWORD read_len = 0;

	HANDLE hPipe = CreateFileA(pipename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	// リクエスト送信
	const BYTE *req_ptr = (BYTE *)request.get_ptr();
	DWORD req_len = (DWORD)request.size();
	while (req_len > 0) {
		DWORD written;
		if (!WriteFile(hPipe, req_ptr, req_len, &written, NULL)) {
			// 書き込みエラーが起きた、通信中に agent が落ちた?
			goto finish;
		}
		req_ptr += written;
		req_len -= written;
	}

	// リプライ受信
	reply.clear();
	if (ReadFile(hPipe, &read_buffer[0], 4, &read_len, NULL) && read_len == 4) {
		const uint32_t len = get_uint32(&read_buffer[0]);
		if (len < AGENT_MAX_MSGLEN) {
			uint32_t total_len;
			reply.append_array(&read_buffer[0], 4);
			for (total_len = 0; total_len < len;) {
				if (!ReadFile(hPipe, &read_buffer[0], (DWORD)read_buffer.size(), &read_len, NULL)) {
					reply.clear();
					break;
				}
				reply.append_array(&read_buffer[0], read_len);
				total_len += read_len;
			}
		}
	}

	if (reply.size() > 0)
		r = TRUE;

finish:
	CloseHandle(hPipe);
	SecureZeroMemory(&read_buffer[0], read_buffer.size());
	return r;
}
#endif

/**
 *	sidの取得
 *	from PuTTY windows/utils/security.c get_user_sid()
 */
#if PUTTY_SHM
static PSID get_user_sid(void)
{
    HANDLE proc = NULL, tok = NULL;
    TOKEN_USER *user = NULL;
    DWORD toklen, sidlen;
    PSID sid = NULL, ret = NULL;

    if (usersid)
        return usersid;

    if ((proc = OpenProcess(MAXIMUM_ALLOWED, false,
                            GetCurrentProcessId())) == NULL)
        goto cleanup;

    if (!OpenProcessToken(proc, TOKEN_QUERY, &tok))
        goto cleanup;

    if (!GetTokenInformation(tok, TokenUser, NULL, 0, &toklen) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        goto cleanup;

    if ((user = (TOKEN_USER *)LocalAlloc(LPTR, toklen)) == NULL)
        goto cleanup;

    if (!GetTokenInformation(tok, TokenUser, user, toklen, &toklen))
        goto cleanup;

    sidlen = GetLengthSid(user->User.Sid);

    sid = (PSID)malloc(sidlen);

    if (!CopySid(sidlen, sid, user->User.Sid))
        goto cleanup;

    /* Success. Move sid into the return value slot, and null it out
     * to stop the cleanup code freeing it. */
    ret = usersid = sid;
    sid = NULL;

  cleanup:
    if (proc != NULL)
        CloseHandle(proc);
    if (tok != NULL)
        CloseHandle(tok);
    if (user != NULL)
        LocalFree(user);
    if (sid != NULL)
        free(sid);

    return ret;
}
#endif

/**
 *	SECURITY_ATTRIBUTES の取得
 *	from PuTTY windows/agent-client.c wm_copydata_agent_query() の一部を切り出し
 *
 *	@param	psa		SECURITY_ATTRIBUTES へのポインタ
 *					ここに取得する
 *	@return	SECURITY_ATTRIBUTESへのポインタ
 *			不要になったら psa->lpSecurityDescriptor を LocalFree() すること
 *			取得できなかったときは NULL
 */
#if PUTTY_SHM
static SECURITY_ATTRIBUTES *get_sa(SECURITY_ATTRIBUTES *psa)
{
	memset(psa, 0, sizeof(*psa));
	usersid = get_user_sid();
	if (!usersid) {
		return NULL;
	}
	PSECURITY_DESCRIPTOR psd = (PSECURITY_DESCRIPTOR)
		LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (!psd) {
		return NULL;
	}

	if (InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION) &&
		SetSecurityDescriptorOwner(psd, usersid, false)) {
		psa->nLength = sizeof(*psa);
		psa->bInheritHandle = true;
		psa->lpSecurityDescriptor = psd;	// LocalFree() すること
	} else {
		LocalFree(psd);
		psa = NULL;
	}
	return psa;
}
#endif

#if PUTTY_SHM
/**
 *	agent(pageant)と通信,共有メモリ経由
 *	PuTTY windows/agent-client.c wm_copydata_agent_query() と同等
 *
 *	@retval	FALSE	エラー
 */
static BOOL query_wm_copydata(const Buffer &request, Buffer &reply)
{
	HWND hwnd;
	char mapname[25];
	HANDLE filemap = NULL;
	unsigned char *p = NULL;
	unsigned long len;
	BOOL ret = FALSE;
	const uint8_t *in = (uint8_t *)request.get_ptr();
	SECURITY_ATTRIBUTES *psa = NULL;

	reply.clear();

	if ((len = get_uint32(in)) > AGENT_MAX_MSGLEN) {
		goto agent_error;
	}

	hwnd = FindWindowA("Pageant", "Pageant");
	if (!hwnd) {
		goto agent_error;
	}

	SECURITY_ATTRIBUTES sa;
	psa = get_sa(&sa);
	sprintf_s(mapname, "PageantRequest%08x", (unsigned)GetCurrentThreadId());
	filemap = CreateFileMappingA(INVALID_HANDLE_VALUE, psa, PAGE_READWRITE,
	                             0, AGENT_MAX_MSGLEN, mapname);
	if (!filemap) {
		goto agent_error;
	}

	if ((p = (unsigned char *)MapViewOfFile(filemap, FILE_MAP_WRITE, 0, 0, 0)) == NULL) {
		goto agent_error;
	}

	COPYDATASTRUCT cds;
#define AGENT_COPYDATA_ID 0x804e50ba	// from putty windows\platform.h
	cds.dwData = AGENT_COPYDATA_ID;
	cds.cbData = (DWORD)(strlen(mapname) + 1);
	cds.lpData = mapname;

	memcpy(p, in, len + 4);
	if (SendMessageA(hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds) > 0) {
		// 応答があった
		len = get_uint32(p);
		reply.append_array(p, len+4);
		ret = TRUE;
	}

agent_error:
	if (p) {
		UnmapViewOfFile(p);
	}
	if (filemap) {
		CloseHandle(filemap);
	}
	if (ret == 0) {
		reply.append_uint32(5);
		reply.append_byte(SSH_AGENT_FAILURE);
		ret = FALSE;
	}
	if (psa != NULL) {
		LocalFree(psa->lpSecurityDescriptor);
	}
	return ret;
}
#endif

/**
 *	PuTTY windows/agent-client.c agent_query() と同等
 */
static BOOL query(const Buffer &request, Buffer &reply)
{
	BOOL r;

	reply.clear();
#if PUTTY_NAMEDPIPE
	char *pname = agent_named_pipe_name();
	if (pname != NULL) {
		r = query_namedpipe(pname, request, reply);
		free(pname);
		if (r) {
			goto finish;
		}
	}
#endif
#if PUTTY_SHM
	r = query_wm_copydata(request, reply);
	if (r) {
		goto finish;
	}
#endif
#if MS_NAMEDPIPE
	{
		char *ms_namedpipe = get_ms_namedpipe();
		r = query_namedpipe(ms_namedpipe, request, reply);
		free(ms_namedpipe);
		if (r) {
			goto finish;
		}
	}
#endif
finish:
	return r;
}

static int putty_get_keylist(unsigned char **keylist, uint8_t req_byte, uint8_t rep_byte)
{
	Buffer req;
	req.append_uint32(1);
	req.append_byte(req_byte);

	Buffer rep;
	query(req, rep);

	uint32_t key_blob_len;
	uint8_t *key_blob_ptr;
	if (rep.size() < 4) {
	error:
		key_blob_len = 0;
		key_blob_ptr = NULL;
	}
	else {
		// check
		const uint8_t *reply_ptr = (uint8_t *)rep.get_ptr();
		uint32_t reply_len = get_uint32(reply_ptr);
		if (rep.size() < reply_len + 4 || reply_ptr[4] != rep_byte) {
			key_blob_len = 0;
			key_blob_ptr = NULL;
			goto error;
		}
		key_blob_len = reply_len - (4+1);
		key_blob_ptr = (uint8_t *)malloc(key_blob_len);
		memcpy(key_blob_ptr, reply_ptr + (4+1), key_blob_len);
	}

	*keylist = key_blob_ptr;
	return key_blob_len;
}

// https://datatracker.ietf.org/doc/html/draft-miller-ssh-agent-04#section-4.4
int putty_get_ssh2_keylist(unsigned char **keylist)
{
	return putty_get_keylist(keylist, SSH_AGENTC_REQUEST_IDENTITIES, SSH_AGENT_IDENTITIES_ANSWER);
}

// https://datatracker.ietf.org/doc/html/draft-miller-ssh-agent-04#section-4.5
void *putty_sign_ssh2_key(unsigned char *pubkey,
						  unsigned char *data,
						  int datalen,
						  int *outlen,
						  int signflags)
{
	int pubkeylen = get_uint32(pubkey);

	Buffer req;
	req.append_byte(SSH_AGENTC_SIGN_REQUEST);
	req.append_array(pubkey, 4 + pubkeylen);
	req.append_uint32(datalen);
	req.append_array(data, datalen);
	req.append_uint32(signflags);
	req.prepend_uint32((uint32_t)req.size());

	Buffer rep;
	query(req, rep);
	const uint8_t *reply_ptr = (uint8_t *)rep.get_ptr();

	if (rep.size() < 5 || reply_ptr[4] != SSH_AGENT_SIGN_RESPONSE) {
		return NULL;
	}

	size_t signed_blob_len = rep.size() - (4+1);
	void *signed_blob_ptr = malloc(signed_blob_len);
	memcpy(signed_blob_ptr, reply_ptr + (4+1), signed_blob_len);
	if (outlen)
		*outlen = (int)signed_blob_len;
	return signed_blob_ptr;
}

int putty_get_ssh1_keylist(unsigned char **keylist)
{
#if SSH1_ENABLE
	return putty_get_keylist(keylist, SSH1_AGENTC_REQUEST_RSA_IDENTITIES, SSH1_AGENT_RSA_IDENTITIES_ANSWER);
#else
	*keylist = NULL;
	return 0;
#endif
}

void *putty_hash_ssh1_challenge(unsigned char *pubkey,
								int pubkeylen,
								unsigned char *data,
								int datalen,
								unsigned char *session_id,
								int *outlen)
{
#if SSH1_ENABLE
	Buffer req;
	req.append_byte(SSH1_AGENTC_RSA_CHALLENGE);
	req.append_array(pubkey, pubkeylen);
	req.append_array(data, datalen);
	req.append_array(session_id, 16);
	req.append_uint32(1); // response format
	req.prepend_uint32((uint32_t)req.size());

	Buffer rep;
	query(req, rep);

	uint32_t ret_len;
	uint8_t *ret_ptr;
	if (rep.size() < 4) {
	error:
		ret_ptr = NULL;
		ret_len = 0;
	}
	else {
		const uint8_t *reply_ptr = (uint8_t *)rep.get_ptr();
		uint32_t reply_len = get_uint32(reply_ptr);
		if (reply_len != (uint32_t)(rep.size() - 4)) {
			goto error;
		}
		if (reply_len < 1 + 16 || reply_ptr[4] != SSH1_AGENT_RSA_RESPONSE) {
			goto error;
		}
		ret_len = (uint32_t)(reply_len - 1);
		ret_ptr = (uint8_t *)malloc(ret_len);
		memcpy(ret_ptr, reply_ptr + (4+1), ret_len);
	}
#else
	(void)pubkey;
	(void)pubkeylen;
	(void)data;
	(void)datalen;
	(void)session_id;
	uint32_t ret_len = 0;
	uint8_t *ret_ptr = NULL;
#endif

	if (outlen)
		*outlen = ret_len;

	return ret_ptr;
}

#if SSH1_ENABLE
static int mp_ssh1(const uint8_t *data, int len)
{
	if (len < 2) {
		return 0;
	}
	int bit_len = get_uint16(data);
	len -= 2;
	data += 2;
	int byte_len = (int)((bit_len + 7) / 8);
	if (len < byte_len) {
		return 0;
	}
	return 2 + byte_len;
}
#endif

int putty_get_ssh1_keylen(unsigned char *key, int maxlen)
{
#if SSH1_ENABLE
	int left = maxlen;
	const uint8_t *key_ptr = key;

	// 4byte
	if (left < 4) {
	error:
		return 0;
	}
	key_ptr += 4;
	left -= 4;

	// mp
	int len = mp_ssh1(key_ptr, left);
	if (len == 0) {
		goto error;
	}
	left -= len;
	key_ptr += len;

	// mp
	len = mp_ssh1(key_ptr, left);
	if (left == 0) {
		goto error;
	}
	left -= len;
	key_ptr += len;

	return maxlen - left;
#else
	(void)key;
	(void)maxlen;
	return 0;
#endif
}

const char *putty_get_version()
{
	return "libsshagent 1.0";
}

/**
 *	PuTTY aqsync.c agent_query_synchronous() と同等
 */
void putty_agent_query_synchronous(const void *req_ptr, int req_len, void **rep_ptr, int *rep_len)
{
	Buffer request;
	request.append_array(req_ptr, req_len);
	Buffer reply;
	query(request, reply);
	size_t len;
	*rep_ptr = reply.get_mallocdbuf(&len);
	*rep_len = (int)len;
}

#if PUTTY_NAMEDPIPE
/**
 *	PuTTY windows/agent-client.c named_pipe_agent_exists() と同等
 */
static BOOL check_puttyagent_namedpipe()
{
	char *pname = agent_named_pipe_name();
	DWORD r = GetFileAttributesA(pname);
	free(pname);
	return r != INVALID_FILE_ATTRIBUTES ? TRUE : FALSE;
}
#endif

#if PUTTY_SHM
/**
 *	from PuTTY windows\agent-client.c wm_copydata_agent_exists()
 */
static BOOL check_puttyagent_wm_copydata()
{
	HWND hwnd;
	hwnd = FindWindowA("Pageant", "Pageant");
	if (!hwnd)
		return false;
	else
		return true;
}
#endif

#if MS_NAMEDPIPE
static BOOL check_MSagent_namedpipe()
{
	char *ms_namedpipe = get_ms_namedpipe();
	DWORD r = GetFileAttributesA(ms_namedpipe);
	free(ms_namedpipe);
	return r != INVALID_FILE_ATTRIBUTES ? TRUE : FALSE;
}
#endif

/**
 *	PuTTY windows/agent-client.c agent_exists() と同等
 */
BOOL putty_agent_exists()
{
#if PUTTY_NAMEDPIPE
	if (check_puttyagent_namedpipe()) {
		return TRUE;
	}
#endif
#if PUTTY_SHM
	if (check_puttyagent_wm_copydata()) {
		return TRUE;
	}
#endif
#if MS_NAMEDPIPE
	if (check_MSagent_namedpipe()) {
		return TRUE;
	}
#endif
	return FALSE;
}

void safefree(void *p)
{
	free(p);
}
