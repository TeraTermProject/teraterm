/*
 * (C) 2022- TeraTerm Project
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

#include <stdio.h>
#include <locale.h>
#include <windows.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdint.h>
#include <vector>

#include "libputty.h"
//#include "sshagent/sha256.h"

void put_byte(std::vector<uint8_t> &buf, uint8_t b)
{
	buf.push_back(b);
}

void put_data(std::vector<uint8_t> &buf, const void *ptr, size_t len)
{
	const uint8_t *u8ptr = (uint8_t *)ptr;
	buf.insert(buf.end(), &u8ptr[0], &u8ptr[len]);
}

void put_uint32(std::vector<uint8_t> &buf, uint32_t u32)
{
	buf.push_back((u32 >> (8*3)) & 0xff);
	buf.push_back((u32 >> (8*2)) & 0xff);
	buf.push_back((u32 >> (8*1)) & 0xff);
	buf.push_back((u32 >> (8*0)) & 0xff);
}

/*
 * OpenSSH's SSH-2 agent messages.
 */
#define SSH2_AGENTC_REQUEST_IDENTITIES          11
#define SSH2_AGENT_IDENTITIES_ANSWER            12
#define SSH2_AGENTC_SIGN_REQUEST                13
#define SSH2_AGENT_SIGN_RESPONSE                14
#define SSH2_AGENTC_ADD_IDENTITY                17
#define SSH2_AGENTC_REMOVE_IDENTITY             18
#define SSH2_AGENTC_REMOVE_ALL_IDENTITIES       19
#define SSH2_AGENTC_EXTENSION                   27
#define SSH_AGENT_EXTENSION_FAILURE             28

static void query()
{
	std::vector<uint8_t> buf;
	put_uint32(buf, 1);
	put_byte(buf, SSH2_AGENTC_REQUEST_IDENTITIES);

	uint8_t *out_ptr;
	int out_len;
	putty_agent_query_synchronous(&buf[0], (int)buf.size(), (void **)&out_ptr, &out_len);

	// SSH_AGENT_IDENTITIES_ANSWER(12, 0x0c) が返ってくる
	safefree(out_ptr);
}

void ssh2_key()
{
	unsigned char *keylist;
	int r = putty_get_ssh2_keylist(&keylist);
	safefree(keylist);
}

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "");

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
#if 0
	if (0) {
		const char s[] = "hoge";
		size_t l = sizeof(s) - 1;
		uint8_t digest[32];
		sha256(s, l, digest);
	}
#endif
	if (0) {
		query();
	}
	if (1) {
		putty_agent_exists();
	}
	if (1) {
		ssh2_key();
	}

	_CrtCheckMemory();
	return 0;
}
