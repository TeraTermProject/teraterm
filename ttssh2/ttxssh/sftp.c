/*
Copyright (c) 2008-2012 TeraTerm Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "ttxssh.h"
#include "util.h"
#include "resource.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/engine.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#include "buffer.h"
#include "ssh.h"
#include "crypt.h"
#include "fwd.h"
#include "ssh.h"
#include "sftp.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

// SFTP専用バッファを確保する。SCPとは異なり、先頭に後続のデータサイズを埋め込む。
static void sftp_buffer_alloc(buffer_t **message)
{
	buffer_t *msg;

	msg = buffer_init();
	if (msg == NULL) {
		goto error;
	}
	// Message length(4byte)
	buffer_put_int(msg, 0); 

	*message = msg;

error:
	assert(msg != NULL);
	return;
}

static void sftp_buffer_free(buffer_t *message)
{
	buffer_free(message);
}

// サーバにSFTPパケットを送信する。
static void sftp_send_msg(PTInstVar pvar, Channel_t *c, buffer_t *msg)
{
	char *p;
	int len;

	len = buffer_len(msg);
	p = buffer_ptr(msg);
	// 最初にメッセージサイズを格納する。
	set_uint32(p, len - 4);
	// ペイロードの送信。
	SSH2_send_channel_data(pvar, c, p, len);
}

// SFTP通信開始前のネゴシエーション
void sftp_do_init(PTInstVar pvar, Channel_t *c)
{
	buffer_t *msg;

	c->sftp.state = SFTP_INIT;

	sftp_buffer_alloc(&msg);
	buffer_put_char(msg, SSH2_FXP_INIT); 
	buffer_put_int(msg, SSH2_FILEXFER_VERSION);
	sftp_send_msg(pvar, c, msg);
	sftp_buffer_free(msg);
}

void sftp_response(PTInstVar pvar, Channel_t *c, unsigned char *data, unsigned int buflen)
{
	OutputDebugPrintf("len %d\n", buflen);

}
