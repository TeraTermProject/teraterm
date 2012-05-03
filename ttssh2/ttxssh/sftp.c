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

static void sftp_do_syslog(PTInstVar pvar, int level, char *fmt, ...)
{
	char tmp[1024];
	va_list arg;

	va_start(arg, fmt);
	_vsnprintf(tmp, sizeof(tmp), fmt, arg);
	va_end(arg);

	notify_verbose_message(pvar, tmp, level);
}

static void sftp_syslog(PTInstVar pvar, char *fmt, ...)
{
	char tmp[1024];
	va_list arg;

	va_start(arg, fmt);
	_vsnprintf(tmp, sizeof(tmp), fmt, arg);
	va_end(arg);

	notify_verbose_message(pvar, tmp, LOG_LEVEL_VERBOSE);
}

// SFTP専用バッファを確保する。SCPとは異なり、先頭に後続のデータサイズを埋め込む。
//
// buffer_t
//    +---------+------------------------------------+
//    | msg_len | data                               |  
//    +---------+------------------------------------+
//       4byte   <------------- msg_len ------------->
//
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

// サーバから受信したSFTPパケットをバッファに格納する。
static void sftp_get_msg(PTInstVar pvar, Channel_t *c, unsigned char *data, unsigned int buflen, buffer_t **message)
{
	buffer_t *msg = *message;
	int msg_len;

	// バッファを確保し、データをすべて放り込む。以降は buffer_t 型を通して操作する。
	// そうしたほうが OpenSSH のコードとの親和性が良くなるため。
	buffer_clear(msg);
	buffer_append(msg, data, buflen);
	buffer_rewind(msg);

	msg_len = buffer_get_int(msg);
	if (msg_len > SFTP_MAX_MSG_LENGTH) {
		// TODO:
		sftp_syslog(pvar, "Received message too long %u", msg_len);
		goto error;
	}
	if (msg_len + 4 != buflen) {
		// TODO:
		sftp_syslog(pvar, "Buffer length %u is invalid", buflen);
		goto error;
	}

error:
	return;
}

// SFTP通信開始前のネゴシエーション
// based on do_init()#sftp-client.c
void sftp_do_init(PTInstVar pvar, Channel_t *c)
{
	buffer_t *msg;

	// SFTP管理構造体の初期化
	memset(&c->sftp, 0, sizeof(c->sftp));
	c->sftp.state = SFTP_INIT;
	c->sftp.transfer_buflen = DEFAULT_COPY_BUFLEN;
	c->sftp.num_requests = DEFAULT_NUM_REQUESTS;
	c->sftp.exts = 0;
	c->sftp.limit_kbps = 0;

	// ネゴシエーションの開始
	sftp_buffer_alloc(&msg);
	buffer_put_char(msg, SSH2_FXP_INIT); 
	buffer_put_int(msg, SSH2_FILEXFER_VERSION);
	sftp_send_msg(pvar, c, msg);
	sftp_buffer_free(msg);

	sftp_syslog(pvar, "SFTP client version %u", SSH2_FILEXFER_VERSION);
}

static void sftp_do_init_recv(PTInstVar pvar, Channel_t *c, buffer_t *msg)
{
	unsigned int type;

	type = buffer_get_char(msg);
	if (type != SSH2_FXP_VERSION) {
		goto error;
	}
	c->sftp.version = buffer_get_int(msg);
	sftp_syslog(pvar, "SFTP server version %u, remote version %u", type, c->sftp.version);

	while (buffer_remain_len(msg) > 0) {
		char *name = buffer_get_string_msg(msg, NULL);
		char *value = buffer_get_string_msg(msg, NULL);
		int known = 0;

        if (strcmp(name, "posix-rename@openssh.com") == 0 &&
            strcmp(value, "1") == 0) {
            c->sftp.exts |= SFTP_EXT_POSIX_RENAME;
            known = 1;
        } else if (strcmp(name, "statvfs@openssh.com") == 0 &&
            strcmp(value, "2") == 0) {
            c->sftp.exts |= SFTP_EXT_STATVFS;
            known = 1;
        } else if (strcmp(name, "fstatvfs@openssh.com") == 0 &&
            strcmp(value, "2") == 0) {
            c->sftp.exts |= SFTP_EXT_FSTATVFS;
            known = 1;
        } else if (strcmp(name, "hardlink@openssh.com") == 0 &&
            strcmp(value, "1") == 0) {
            c->sftp.exts |= SFTP_EXT_HARDLINK;
            known = 1;
        }
        if (known) {
            sftp_syslog(pvar, "Server supports extension \"%s\" revision %s",
                name, value);
        } else {
            sftp_syslog(pvar, "Unrecognised server extension \"%s\"", name);
        }

		free(name);
		free(value);
	}

	if (c->sftp.version == 0) {
		c->sftp.transfer_buflen = min(c->sftp.transfer_buflen, 20480);
	}
	c->sftp.limit_kbps = 0;
	if (c->sftp.limit_kbps > 0) {
		// TODO:
	}

error:
	return;
}

// SFTP受信処理 -メインルーチン-
void sftp_response(PTInstVar pvar, Channel_t *c, unsigned char *data, unsigned int buflen)
{
	buffer_t *msg;
	int state;

	/*
	 * Allocate buffer
	 */
	sftp_buffer_alloc(&msg);
	sftp_get_msg(pvar, c, data, buflen, &msg);

	state = c->sftp.state;
	if (state == SFTP_INIT) {
		sftp_do_init_recv(pvar, c, msg);
		sftp_syslog(pvar, "Connected to SFTP server.");
	}

	/*
	 * Free buffer
	 */
	sftp_buffer_free(msg);
}
