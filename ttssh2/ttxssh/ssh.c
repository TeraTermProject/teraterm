/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (C) 2004-2017 TeraTerm Project
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

#include "ttxssh.h"
#include "util.h"
#include "resource.h"
#include "libputty.h"
#include "key.h"
#include "ttcommon.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/engine.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/md5.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#include <commctrl.h>
#include "buffer.h"
#include "ssh.h"
#include "crypt.h"
#include "fwd.h"
#include "sftp.h"
#include "kex.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include <assert.h>

#include <direct.h>
#include <io.h>

// SSH2 macro
#ifdef _DEBUG
#define SSH2_DEBUG
#endif

//#define DONT_WANTCONFIRM 1  // (2005.3.28 yutaka)
#undef DONT_WANTCONFIRM // (2008.11.25 maya)

//
// SSH2 data structure
//

// channel data structure
#define CHANNEL_MAX 100

//
// msg が NULL では無い事の保証。NULL の場合は "(null)" を返す。
//
#define NonNull(msg) ((msg)?(msg):"(null)")

static struct global_confirm global_confirms;

static Channel_t channels[CHANNEL_MAX];

static char ssh_ttymodes[] = "\x01\x03\x02\x1c\x03\x08\x04\x15\x05\x04";

static CRITICAL_SECTION g_ssh_scp_lock;   /* SCP受信用ロック */

static void try_send_credentials(PTInstVar pvar);
static void prep_compression(PTInstVar pvar);

// 関数プロトタイプ宣言
void SSH2_send_kexinit(PTInstVar pvar);
static BOOL handle_SSH2_kexinit(PTInstVar pvar);
static void SSH2_dh_kex_init(PTInstVar pvar);
static void SSH2_dh_gex_kex_init(PTInstVar pvar);
static void SSH2_ecdh_kex_init(PTInstVar pvar);
static BOOL handle_SSH2_dh_common_reply(PTInstVar pvar);
static BOOL handle_SSH2_dh_gex_reply(PTInstVar pvar);
static BOOL handle_SSH2_newkeys(PTInstVar pvar);
static BOOL handle_SSH2_service_accept(PTInstVar pvar);
static BOOL handle_SSH2_userauth_success(PTInstVar pvar);
static BOOL handle_SSH2_userauth_failure(PTInstVar pvar);
static BOOL handle_SSH2_userauth_banner(PTInstVar pvar);
static BOOL handle_SSH2_open_confirm(PTInstVar pvar);
static BOOL handle_SSH2_open_failure(PTInstVar pvar);
static BOOL handle_SSH2_client_global_request(PTInstVar pvar);
static BOOL handle_SSH2_request_success(PTInstVar pvar);
static BOOL handle_SSH2_request_failure(PTInstVar pvar);
static BOOL handle_SSH2_channel_success(PTInstVar pvar);
static BOOL handle_SSH2_channel_failure(PTInstVar pvar);
static BOOL handle_SSH2_channel_data(PTInstVar pvar);
static BOOL handle_SSH2_channel_extended_data(PTInstVar pvar);
static BOOL handle_SSH2_channel_eof(PTInstVar pvar);
static BOOL handle_SSH2_channel_close(PTInstVar pvar);
static BOOL handle_SSH2_channel_open(PTInstVar pvar);
static BOOL handle_SSH2_window_adjust(PTInstVar pvar);
static BOOL handle_SSH2_channel_request(PTInstVar pvar);
void SSH2_dispatch_init(int stage);
int SSH2_dispatch_enabled_check(unsigned char message);
void SSH2_dispatch_add_message(unsigned char message);
void SSH2_dispatch_add_range_message(unsigned char begin, unsigned char end);
int dh_pub_is_valid(DH *dh, BIGNUM *dh_pub);
static void start_ssh_heartbeat_thread(PTInstVar pvar);
void ssh2_channel_send_close(PTInstVar pvar, Channel_t *c);
static BOOL SSH_agent_response(PTInstVar pvar, Channel_t *c, int local_channel_num, unsigned char *data, unsigned int buflen);
static void ssh2_scp_get_packetlist(Channel_t *c, unsigned char **buf, unsigned int *buflen);
static void ssh2_scp_free_packetlist(Channel_t *c);
static void get_window_pixel_size(PTInstVar pvar, int *x, int *y);

//
// Global request confirm
//
static void client_init_global_confirm(void)
{
	memset(&global_confirms, 0, sizeof(global_confirms));
	global_confirms.ref_count = 0;
}

void client_register_global_confirm(global_confirm_cb *cb, void *ctx)
{
	struct global_confirm *gc = &global_confirms;

	if (gc->ref_count == 0) {
		gc->cb = cb;
		gc->ctx = ctx;
		gc->ref_count = 1;
	}
}

static int client_global_request_reply(PTInstVar pvar, int type, unsigned int seq, void *ctxt)
{
	struct global_confirm *gc = &global_confirms;

	if (gc->ref_count >= 1) {
		if (gc->cb)
			gc->cb(pvar, type, seq, gc->ctx);
		gc->ref_count = 0;
	}

	return 0;
}

//
// channel function
//
static Channel_t *ssh2_channel_new(unsigned int window, unsigned int maxpack,
                                   enum confirm_type type, int local_num)
{
	int i, found;
	Channel_t *c;

	found = -1;
	for (i = 0 ; i < CHANNEL_MAX ; i++) {
		if (channels[i].used == 0) { // free channel
			found = i;
			break;
		}
	}
	if (found == -1) { // not free channel
		return (NULL);
	}

	// setup
	c = &channels[found];
	memset(c, 0, sizeof(Channel_t));
	c->used = 1;
	c->self_id = i;
	c->remote_id = -1;
	c->local_window = window;
	c->local_window_max = window;
	c->local_consumed = 0;
	c->local_maxpacket = maxpack;
	c->remote_window = 0;
	c->remote_maxpacket = 0;
	c->type = type;
	c->local_num = local_num;  // alloc_channel()の返値を保存しておく
	c->bufchain = NULL;
	if (type == TYPE_SCP) {
		c->scp.state = SCP_INIT;
		c->scp.progress_window = NULL;
		c->scp.thread = (HANDLE)-1;
		c->scp.localfp = NULL;
		c->scp.filemtime = 0;
		c->scp.fileatime = 0;
	}
	if (type == TYPE_AGENT) {
		c->agent_msg = buffer_init();
		c->agent_request_len = 0;
	}
	c->state = 0;

	return (c);
}

// remote_windowの空きがない場合に、送れなかったバッファをリスト（入力順）へつないでおく。
static void ssh2_channel_add_bufchain(Channel_t *c, unsigned char *buf, unsigned int buflen)
{
	bufchain_t *p, *old;

	// allocate new buffer
	p = malloc(sizeof(bufchain_t));
	if (p == NULL)
		return;
	p->msg = buffer_init();
	if (p == NULL) {
		free(p);
		return;
	}
	buffer_put_raw(p->msg, buf, buflen);
	p->next = NULL;

	if (c->bufchain == NULL) {
		c->bufchain = p;
	} else {
		old = c->bufchain;
		while (old->next)
			old = old->next;
		old->next = p;
	}
}


static void ssh2_channel_retry_send_bufchain(PTInstVar pvar, Channel_t *c)
{
	bufchain_t *ch;
	unsigned int size;

	while (c->bufchain) {
		// 先頭から先に送る
		ch = c->bufchain;
		size = buffer_len(ch->msg);
		if (size >= c->remote_window)
			break;

		if (c->local_num == -1) { // shell or SCP
			SSH2_send_channel_data(pvar, c, buffer_ptr(ch->msg), size, TRUE);
		} else { // port-forwarding
			SSH_channel_send(pvar, c->local_num, -1, buffer_ptr(ch->msg), size, TRUE);
		}

		c->bufchain = ch->next;

		buffer_free(ch->msg);
		free(ch);
	}
}



// channel close時にチャネル構造体をリストへ返却する
// (2007.4.26 yutaka)
static void ssh2_channel_delete(Channel_t *c)
{
	bufchain_t *ch, *ptr;
	enum scp_state prev_state;

	ch = c->bufchain;
	while (ch) {
		if (ch->msg)
			buffer_free(ch->msg);
		ptr = ch;
		ch = ch->next;
		free(ptr);
	}

	if (c->type == TYPE_SCP) {
		// SCP処理の最後の状態を保存する。
		prev_state = c->scp.state;

		c->scp.state = SCP_CLOSING;
		if (c->scp.localfp != NULL) {
			fclose(c->scp.localfp);
			if (c->scp.dir == FROMREMOTE) {
				if (c->scp.fileatime > 0 && c->scp.filemtime > 0) {
					struct _utimbuf filetime;
					filetime.actime = c->scp.fileatime;
					filetime.modtime = c->scp.filemtime;
					_utime(c->scp.localfilefull, &filetime);
				}

				// SCP受信が成功していなければ、ローカルに作ったファイルの残骸を削除する。
				// (2017.2.12 yutaka)
				if (prev_state != SCP_CLOSING)
					remove(c->scp.localfilefull);
			}
		}
		if (c->scp.progress_window != NULL) {
			DestroyWindow(c->scp.progress_window);
			c->scp.progress_window = NULL;
		}
		if (c->scp.thread != (HANDLE)-1L) {
			WaitForSingleObject(c->scp.thread, INFINITE);
			CloseHandle(c->scp.thread);
			c->scp.thread = (HANDLE)-1L;
		}

		ssh2_scp_free_packetlist(c);
	}
	if (c->type == TYPE_AGENT) {
		buffer_free(c->agent_msg);
	}

	memset(c, 0, sizeof(Channel_t));
	c->used = 0;
}


// connection close時に呼ばれる
void ssh2_channel_free(void)
{
	int i;
	Channel_t *c;

	for (i = 0 ; i < CHANNEL_MAX ; i++) {
		c = &channels[i];
		ssh2_channel_delete(c);
	}

}

static Channel_t *ssh2_channel_lookup(int id)
{
	Channel_t *c;

	if (id < 0 || id >= CHANNEL_MAX) {
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": invalid channel id. (%d)", id);
		return (NULL);
	}
	c = &channels[id];
	if (c->used == 0) { // already freed
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": channel was already freed. id:%d", id);
		return (NULL);
	}
	return (c);
}

// SSH1で管理しているchannel構造体から、SSH2向けのChannel_tへ変換する。
// TODO: 将来的にはチャネル構造体は1つに統合する。
// (2005.6.12 yutaka)
static Channel_t *ssh2_local_channel_lookup(int local_num)
{
	int i;
	Channel_t *c;

	for (i = 0 ; i < CHANNEL_MAX ; i++) {
		c = &channels[i];
		if (c->local_num == local_num)
			return (c);
	}
	return (NULL);
}


//
// SSH heartbeat mutex
//
// TTSSHは thread-safe ではないため、マルチスレッドからのパケット送信はできない。
// シングルスレッドではコンテキストスイッチが発生することはないため、
// ロックを取る必要もないため、削除する。(2007.12.26 yutaka)
//
static CRITICAL_SECTION g_ssh_heartbeat_lock;   /* 送受信用ロック */

void ssh_heartbeat_lock_initialize(void)
{
	//InitializeCriticalSection(&g_ssh_heartbeat_lock);
}

void ssh_heartbeat_lock_finalize(void)
{
	//DeleteCriticalSection(&g_ssh_heartbeat_lock);
}

void ssh_heartbeat_lock(void)
{
	//EnterCriticalSection(&g_ssh_heartbeat_lock);
}

void ssh_heartbeat_unlock(void)
{
	//LeaveCriticalSection(&g_ssh_heartbeat_lock);
}


//
// SSH memory dump (for debug)
//
// (2005.3.7 yutaka)
//
#define MEMTAG_MAX 300
#define LOGDUMP "ssh2connect.log"
#define LOG_PACKET_DUMP "ssh2packet.log"
#define SENDTOME "Please send '"LOGDUMP"' file to Tera Term developer team."

typedef struct memtag {
	char *name;
	char *desc;
	time_t time;
	int len;
	char *data;
} memtag_t;

static memtag_t memtags[MEMTAG_MAX];
static int memtag_count = 0;
static int memtag_use = 0;

/* ダンプラインをフォーマット表示する */
static void displine_memdump(FILE *fp, int addr, int *bytes, int byte_cnt)
{
	int i, c;

	/* 先頭のアドレス表示 */
	fprintf(fp, "%08X : ", addr);

	/* バイナリ表示（4バイトごとに空白を挿入）*/
	for (i = 0 ; i < byte_cnt ; i++) {
		if (i > 0 && i % 4 == 0)
			fprintf(fp, " ");

		fprintf(fp, "%02X", bytes[i]);
	}

	/* ASCII表示部分までの空白を補う */
	fprintf(fp, "   %*s%*s", (16-byte_cnt)*2+1, " ", (16-byte_cnt+3)/4, " ");

	/* ASCII表示 */
	for (i = 0 ; i < byte_cnt ; i++) {
		c = bytes[i];
		if (isprint(c)) {
			fprintf(fp, "%c", c);
		} else {
			fprintf(fp, ".");
		}
	}

	fprintf(fp, "\n");
}


/* ダンプルーチン */
static void dump_memdump(FILE *fp, char *data, int len)
{
	int c, addr;
	int bytes[16], *ptr;
	int byte_cnt;
	int i;

	addr = 0;
	byte_cnt = 0;
	ptr = bytes;
	for (i = 0 ; i < len ; i++) {
		c = data[i];
		*ptr++ = c & 0xff;
		byte_cnt++;

		if (byte_cnt == 16) {
			displine_memdump(fp, addr, bytes, byte_cnt);

			addr += 16;
			byte_cnt = 0;
			ptr = bytes;
		}
	}

	if (byte_cnt > 0) {
		displine_memdump(fp, addr, bytes, byte_cnt);
	}
}

void init_memdump(void)
{
	int i;

	if (memtag_use > 0) 
		return;

	for (i = 0 ; i < MEMTAG_MAX ; i++) {
		memtags[i].name = NULL;
		memtags[i].desc = NULL;
		memtags[i].data = NULL;
		memtags[i].len = 0;
	}
	memtag_use++;
}

void finish_memdump(void)
{
	int i;

	// initializeされてないときは何もせずに戻る。(2005.4.3 yutaka)
	if (memtag_use <= 0)
		return;
	memtag_use--;

	for (i = 0 ; i < MEMTAG_MAX ; i++) {
		free(memtags[i].name);
		free(memtags[i].desc);
		free(memtags[i].data);
		memtags[i].len = 0;
	}
	memtag_count = 0;
}

void save_memdump(char *filename)
{
	FILE *fp;
	int i;
	time_t t;
	struct tm *tm;

	fp = fopen(filename, "w");
	if (fp == NULL)
		return;

	t = time(NULL);
	tm = localtime(&t);

	fprintf(fp, "<<< Tera Term SSH2 log dump >>>\n");
	fprintf(fp, "saved time: %04d/%02d/%02d %02d:%02d:%02d\n",
	        tm->tm_year + 1900,
	        tm->tm_mon + 1,
	        tm->tm_mday,
	        tm->tm_hour,
	        tm->tm_min,
	        tm->tm_sec);
	fprintf(fp, "\n");

	for (i = 0 ; i < memtag_count ; i++) {
		fprintf(fp, "============================================\n");
		fprintf(fp, "name: %s\n", memtags[i].name);
		fprintf(fp, "--------------------------------------------\n");
		fprintf(fp, "description: %s\n", memtags[i].desc);
		fprintf(fp, "--------------------------------------------\n");
		fprintf(fp, "time: %s", ctime(&memtags[i].time));
		fprintf(fp, "============================================\n");
		dump_memdump(fp, memtags[i].data, memtags[i].len);
		fprintf(fp, "\n\n\n");
	}

	fprintf(fp, "[EOF]\n");

	fclose(fp);
}

void push_memdump(char *name, char *desc, char *data, int len)
{
	memtag_t *ptr;
	char *dp;

	dp = malloc(len);
	if (dp == NULL)
		return;
	memcpy(dp, data, len);

	if (memtag_count >= MEMTAG_MAX)
		return;

	ptr = &memtags[memtag_count];
	memtag_count++;
	ptr->name = _strdup(name);
	ptr->desc = _strdup(desc);
	ptr->time = time(NULL);
	ptr->data = dp;
	ptr->len = len;
}

void push_bignum_memdump(char *name, char *desc, BIGNUM *bignum)
{
	int len;
	char *buf;

	len = BN_num_bytes(bignum);
	buf = malloc(len); // allocate
	if (buf == NULL)
		return;
	BN_bn2bin(bignum, buf);
	push_memdump(name, desc, buf, len); // at push_bignum_memdump()
	free(buf); // free
}


//
//
//


static int get_predecryption_amount(PTInstVar pvar)
{
	static int small_block_decryption_sizes[] = { 5, 5, 6, 6, 8 };

	if (SSHv1(pvar)) {
		return 0;
	} else {
		int block_size = CRYPT_get_decryption_block_size(pvar);

		if (block_size < 5) {
			return small_block_decryption_sizes[block_size];
		} else {
			return block_size;
		}
	}
}

/* Get up to 'limit' bytes into the payload buffer.
   'limit' is counted from the start of the payload data.
   Returns the amount of data in the payload buffer, or
   -1 if there is an error.
   We can return more than limit in some cases. */
static int buffer_packet_data(PTInstVar pvar, int limit)
{
	if (pvar->ssh_state.payloadlen >= 0) {
		return pvar->ssh_state.payloadlen;
	} else {
		int cur_decompressed_bytes =
			pvar->ssh_state.decompress_stream.next_out -
			pvar->ssh_state.postdecompress_inbuf;

		while (limit > cur_decompressed_bytes) {
			int result;

			pvar->ssh_state.payload =
				pvar->ssh_state.postdecompress_inbuf + 1;
			if (pvar->ssh_state.postdecompress_inbuflen == cur_decompressed_bytes) {
				buf_ensure_size(&pvar->ssh_state.postdecompress_inbuf,
				                &pvar->ssh_state.postdecompress_inbuflen,
				                min(limit, cur_decompressed_bytes * 2));
			}

			pvar->ssh_state.decompress_stream.next_out =
				pvar->ssh_state.postdecompress_inbuf +
				cur_decompressed_bytes;
			pvar->ssh_state.decompress_stream.avail_out =
				min(limit, pvar->ssh_state.postdecompress_inbuflen)
				- cur_decompressed_bytes;

			result =
				inflate(&pvar->ssh_state.decompress_stream, Z_SYNC_FLUSH);
			cur_decompressed_bytes =
				pvar->ssh_state.decompress_stream.next_out -
				pvar->ssh_state.postdecompress_inbuf;

			switch (result) {
			case Z_OK:
				break;
			case Z_BUF_ERROR:
				pvar->ssh_state.payloadlen = cur_decompressed_bytes;
				return cur_decompressed_bytes;
			default:
				UTIL_get_lang_msg("MSG_SSH_INVALID_COMPDATA_ERROR", pvar,
				                  "Invalid compressed data in received packet");
				notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
				return -1;
			}
		}

		return cur_decompressed_bytes;
	}
}

/* For use by the protocol processing code.
   Gets N bytes of uncompressed payload. Returns FALSE if data not available
   and a fatal error has been signaled.
   The data is available in the payload buffer. This buffer address
   can change during a call to grab_payload, so take care!
   The payload pointer is set to point to the first byte of the actual data
   (after the packet type byte).
*/
static BOOL grab_payload(PTInstVar pvar, int num_bytes)
{
	/* Accept maximum of 4MB of payload data */
	int in_buffer = buffer_packet_data(pvar, PACKET_MAX_SIZE);

	if (in_buffer < 0) {
		return FALSE;
	} else {
		pvar->ssh_state.payload_grabbed += num_bytes;
		if (pvar->ssh_state.payload_grabbed > in_buffer) {
			char buf[128];
			UTIL_get_lang_msg("MSG_SSH_TRUNCATED_PKT_ERROR", pvar,
			                  "Received truncated packet (%ld > %d) @ grab_payload()");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg,
			            pvar->ssh_state.payload_grabbed, in_buffer);
			notify_fatal_error(pvar, buf, TRUE);
			return FALSE;
		} else {
			return TRUE;
		}
	}
}

static BOOL grab_payload_limited(PTInstVar pvar, int num_bytes)
{
	int in_buffer;

	pvar->ssh_state.payload_grabbed += num_bytes;
	in_buffer = buffer_packet_data(pvar, pvar->ssh_state.payload_grabbed);

	if (in_buffer < 0) {
		return FALSE;
	} else {
		if (pvar->ssh_state.payload_grabbed > in_buffer) {
			char buf[128];
			UTIL_get_lang_msg("MSG_SSH_TRUNCATED_PKT_LIM_ERROR", pvar,
			                  "Received truncated packet (%ld > %d) @ grab_payload_limited()");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg,
			            pvar->ssh_state.payload_grabbed, in_buffer);
			notify_fatal_error(pvar, buf, TRUE);
			return FALSE;
		} else {
			return TRUE;
		}
	}
}

#define do_crc(buf, len) (~(uint32)crc32(0xFFFFFFFF, (buf), (len)))

/* Decrypt the payload, checksum it, eat the padding, get the packet type
   and return it.
   'data' points to the start of the packet --- its length field.
   'len' is the length of the
   payload + padding (+ length of CRC for SSHv1). 'padding' is the length
   of the padding alone. */
static int prep_packet(PTInstVar pvar, char *data, int len,
					   int padding)
{
	pvar->ssh_state.payload = data + 4;
	pvar->ssh_state.payloadlen = len;

	if (SSHv1(pvar)) {
		if (CRYPT_detect_attack(pvar, pvar->ssh_state.payload, len)) {
			UTIL_get_lang_msg("MSG_SSH_COREINS_ERROR", pvar,
			                  "'CORE insertion attack' detected.  Aborting connection.");
			notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
		}

		CRYPT_decrypt(pvar, pvar->ssh_state.payload, len);
		/* PKT guarantees that the data is always 4-byte aligned */
		if (do_crc(pvar->ssh_state.payload, len - 4) !=
			get_uint32_MSBfirst(pvar->ssh_state.payload + len - 4)) {
			UTIL_get_lang_msg("MSG_SSH_CORRUPTDATA_ERROR", pvar,
			                  "Detected corrupted data; connection terminating.");
			notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
			return SSH_MSG_NONE;
		}

		pvar->ssh_state.payload += padding;
		pvar->ssh_state.payloadlen -= padding + 4;
	} else {
		int already_decrypted = get_predecryption_amount(pvar);

		CRYPT_decrypt(pvar, data + already_decrypted,
		              (4 + len) - already_decrypted);

		if (!CRYPT_verify_receiver_MAC
			(pvar, pvar->ssh_state.receiver_sequence_number, data, len + 4,
			 data + len + 4)) {
			UTIL_get_lang_msg("MSG_SSH_CORRUPTDATA_ERROR", pvar,
			                  "Detected corrupted data; connection terminating.");
			notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
			return SSH_MSG_NONE;
		}

		pvar->ssh_state.payload++;
		pvar->ssh_state.payloadlen -= padding + 1;
	}

	pvar->ssh_state.payload_grabbed = 0;

	if (SSHv1(pvar)) {
		if (pvar->ssh_state.decompressing) {
			if (pvar->ssh_state.decompress_stream.avail_in != 0) {
				UTIL_get_lang_msg("MSG_SSH_DECOMPRESS_ERROR", pvar,
				                  "Internal error: a packet was not fully decompressed.\n"
				                  "This is a bug, please report it.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			}

			pvar->ssh_state.decompress_stream.next_in =
				pvar->ssh_state.payload;
			pvar->ssh_state.decompress_stream.avail_in =
				pvar->ssh_state.payloadlen;
			pvar->ssh_state.decompress_stream.next_out =
				pvar->ssh_state.postdecompress_inbuf;
			pvar->ssh_state.payloadlen = -1;
		} else {
			pvar->ssh_state.payload++;
		}

		if (!grab_payload_limited(pvar, 1)) {
			return SSH_MSG_NONE;
		}

	} else {
		// support of SSH2 packet compression (2005.7.9 yutaka)
		// support of "Compression delayed" (2006.6.23 maya)
		if ((pvar->stoc_compression == COMP_ZLIB ||
		     pvar->stoc_compression == COMP_DELAYED && pvar->userauth_success) &&
		    pvar->ssh2_keys[MODE_IN].comp.enabled) { // compression enabled
			int ret;

			if (pvar->decomp_buffer == NULL) {
				pvar->decomp_buffer = buffer_init();
				if (pvar->decomp_buffer == NULL)
					return SSH_MSG_NONE;
			}
			// 一度確保したバッファは使い回すので初期化を忘れずに。
			buffer_clear(pvar->decomp_buffer);

			// packet sizeとpaddingを取り除いたペイロード部分のみを展開する。
			ret = buffer_decompress(&pvar->ssh_state.decompress_stream,
			                        pvar->ssh_state.payload,
			                        pvar->ssh_state.payloadlen,
			                        pvar->decomp_buffer);

			// ポインタの更新。
			pvar->ssh_state.payload = buffer_ptr(pvar->decomp_buffer);
			pvar->ssh_state.payload++;
			pvar->ssh_state.payloadlen = buffer_len(pvar->decomp_buffer);

		} else {
			pvar->ssh_state.payload++;
		}

		if (!grab_payload_limited(pvar, 1)) {
			return SSH_MSG_NONE;
		}

	}

	pvar->ssh_state.receiver_sequence_number++;

	return pvar->ssh_state.payload[-1];
}

/* Create a packet to be sent. The SSH protocol packet type is in 'type';
   'len' contains the length of the packet payload, in bytes (this
   does not include the space for any of the packet headers or padding,
   or for the packet type byte).
   Returns a pointer to the payload data area, a region of length 'len',
   to be filled by the caller. */
unsigned char *begin_send_packet(PTInstVar pvar, int type, int len)
{
	unsigned char *buf;

	pvar->ssh_state.outgoing_packet_len = len + 1;

	if (pvar->ssh_state.compressing) {
		buf_ensure_size(&pvar->ssh_state.precompress_outbuf,
		                &pvar->ssh_state.precompress_outbuflen, 1 + len);
		buf = pvar->ssh_state.precompress_outbuf;
	} else {
		/* For SSHv2,
		   Encrypted_length is 4(packetlength) + 1(paddinglength) + 1(packettype)
		   + len(payload) + 4(minpadding), rounded up to nearest block_size
		   We only need a reasonable upper bound for the buffer size */
		buf_ensure_size(&pvar->ssh_state.outbuf,
		                &pvar->ssh_state.outbuflen,
		                len + 30 + CRYPT_get_sender_MAC_size(pvar) +
		                CRYPT_get_encryption_block_size(pvar));
		buf = pvar->ssh_state.outbuf + 12;
	}

	buf[0] = (unsigned char) type;
	return buf + 1;
}


// 送信リトライ関数の追加
//
// WinSockの send() はバッファサイズ(len)よりも少ない値を正常時に返してくる
// ことがあるので、その場合はエラーとしない。
// これにより、TCPコネクション切断の誤検出を防ぐ。
// (2006.12.9 yutaka)
static int retry_send_packet(PTInstVar pvar, char *data, int len)
{
	int n;
	int err;

	while (len > 0) {
		n = (pvar->Psend)(pvar->socket, data, len, 0);

		if (n < 0) {
			err = WSAGetLastError();
			if (err < WSABASEERR || err == WSAEWOULDBLOCK) {
				// send()の返値が0未満で、かつエラー番号が 10000 未満の場合は、
				// 成功したものと見なす。
				// PuTTY 0.58の実装を参考。
				// (2007.2.4 yutak)
				return 0; // success
			}
			return 1; // error
		}

		len -= n;
		data += n;
	}

	return 0; // success
}

static BOOL send_packet_blocking(PTInstVar pvar, char *data, int len)
{
	// パケット送信後にバッファを使いまわすため、ブロッキングで送信してしまう必要がある。
	// ノンブロッキングで送信してWSAEWOULDBLOCKが返ってきた場合、そのバッファは送信完了する
	// まで保持しておかなくてはならない。(2007.10.30 yutaka)
	u_long do_block = 0;
	int code = 0;
	char *kind = NULL, buf[256];

#if 0
	if ((pvar->PWSAAsyncSelect) (pvar->socket, pvar->NotificationWindow,
	                             0, 0) == SOCKET_ERROR
	 || ioctlsocket(pvar->socket, FIONBIO, &do_block) == SOCKET_ERROR
	 || retry_send_packet(pvar, data, len)
	 || (pvar->PWSAAsyncSelect) (pvar->socket, pvar->NotificationWindow,
	                             pvar->notification_msg,
	                             pvar->notification_events) ==
		SOCKET_ERROR) {
		UTIL_get_lang_msg("MSG_SSH_SEND_PKT_ERROR", pvar,
		                  "A communications error occurred while sending an SSH packet.\n"
		                  "The connection will close.");
		notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
		return FALSE;
	} else {
		return TRUE;
	}
#else
	if ((pvar->PWSAAsyncSelect) (pvar->socket, pvar->NotificationWindow,
	                             0, 0) == SOCKET_ERROR) {
			code = WSAGetLastError();
			kind = "WSAAsyncSelect1";
			goto error;
	}
	if (ioctlsocket(pvar->socket, FIONBIO, &do_block) == SOCKET_ERROR) {
			code = WSAGetLastError();
			kind = "ioctlsocket";
			goto error;
	}
	if (retry_send_packet(pvar, data, len) != 0) {
			code = WSAGetLastError();
			kind = "retry_send_packet";
			goto error;
	}
	if ((pvar->PWSAAsyncSelect) (pvar->socket, pvar->NotificationWindow,
	                             pvar->notification_msg,
	                             pvar->notification_events) ==
		SOCKET_ERROR) {
			code = WSAGetLastError();
			kind = "WSAAsyncSelect2";
			goto error;
	}
	return TRUE;

error:
	UTIL_get_lang_msg("MSG_SSH_SEND_PKT_ERROR", pvar,
	                  "A communications error occurred while sending an SSH packet.\n"
	                  "The connection will close. (%s:%d)");
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg,
	            kind, code);
	notify_fatal_error(pvar, buf, TRUE);
	return FALSE;
#endif
}

/* if skip_compress is true, then the data has already been compressed
   into outbuf + 12 */
void finish_send_packet_special(PTInstVar pvar, int skip_compress)
{
	unsigned int len = pvar->ssh_state.outgoing_packet_len;
	unsigned char *data;
	unsigned int data_length;
	buffer_t *msg = NULL; // for SSH2 packet compression

	if (pvar->ssh_state.compressing) {
		if (!skip_compress) {
			buf_ensure_size(&pvar->ssh_state.outbuf,
			                &pvar->ssh_state.outbuflen,
			                (int)(len + (len >> 6) + 50 +
			                      CRYPT_get_sender_MAC_size(pvar)));
			pvar->ssh_state.compress_stream.next_in =
				pvar->ssh_state.precompress_outbuf;
			pvar->ssh_state.compress_stream.avail_in = len;
			pvar->ssh_state.compress_stream.next_out =
				pvar->ssh_state.outbuf + 12;
			pvar->ssh_state.compress_stream.avail_out =
				pvar->ssh_state.outbuflen - 12;

			if (deflate(&pvar->ssh_state.compress_stream, Z_SYNC_FLUSH) != Z_OK) {
				UTIL_get_lang_msg("MSG_SSH_COMP_ERROR", pvar,
				                  "An error occurred while compressing packet data.\n"
				                  "The connection will close.");
				notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
				return;
			}
		}

		len =
			pvar->ssh_state.outbuflen - 12 -
			pvar->ssh_state.compress_stream.avail_out;
	}

	if (SSHv1(pvar)) {
		int padding = 8 - ((len + 4) % 8);

		data = pvar->ssh_state.outbuf + 8 - padding;
		data_length = padding + len + 8;

		set_uint32(data, len + 4);
		if (CRYPT_get_receiver_cipher(pvar) != SSH_CIPHER_NONE) {
			CRYPT_set_random_data(pvar, data + 4, padding);
		} else {
			memset(data + 4, 0, padding);
		}
		set_uint32(data + data_length - 4,
		           do_crc(data + 4, data_length - 8));
		CRYPT_encrypt(pvar, data + 4, data_length - 4);
	} else { //for SSH2(yutaka)
		int block_size = CRYPT_get_encryption_block_size(pvar);
		unsigned int encryption_size;
		unsigned int padding;
		BOOL ret;

		/*
		 データ構造
		 pvar->ssh_state.outbuf:
		 offset: 0 1 2 3 4 5 6 7 8 9 10 11 12 ...         EOD
		         <--ignore---> ^^^^^^^^    <---- payload --->
				               packet length

							            ^^padding

							   <---------------------------->
							      SSH2 sending data on TCP
		
		 NOTE:
		   payload = type(1) + raw-data
		   len = ssh_state.outgoing_packet_len = payload size
		 */
		// パケット圧縮が有効の場合、パケットを圧縮してから送信パケットを構築する。(2005.7.9 yutaka)
		// support of "Compression delayed" (2006.6.23 maya)
		if ((pvar->ctos_compression == COMP_ZLIB ||
		     pvar->ctos_compression == COMP_DELAYED && pvar->userauth_success) &&
		    pvar->ssh2_keys[MODE_OUT].comp.enabled) {
			// このバッファは packet-length(4) + padding(1) + payload(any) を示す。
			msg = buffer_init();
			if (msg == NULL) {
				// TODO: error check
				logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
				return;
			}

			// 圧縮対象はヘッダを除くペイロードのみ。
			buffer_append(msg, "\0\0\0\0\0", 5);  // 5 = packet-length(4) + padding(1)
			if (buffer_compress(&pvar->ssh_state.compress_stream, pvar->ssh_state.outbuf + 12, len, msg) == -1) {
				UTIL_get_lang_msg("MSG_SSH_COMP_ERROR", pvar,
				                  "An error occurred while compressing packet data.\n"
				                  "The connection will close.");
				notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
				return;
			}
			data = buffer_ptr(msg);
			len = buffer_len(msg) - 5;  // 'len' is overwritten.

		} else {
			// 無圧縮
			data = pvar->ssh_state.outbuf + 7;

		}

		// 送信パケット構築(input parameter: data, len)
		if (block_size < 8) {
			block_size = 8;
		}
#if 0
		encryption_size = ((len + 8) / block_size + 1) * block_size;
		data_length = encryption_size + CRYPT_get_sender_MAC_size(pvar);

		set_uint32(data, encryption_size - 4);
		padding = encryption_size - len - 5;
		data[4] = (unsigned char) padding;
#else
		// でかいパケットを送ろうとすると、サーバ側で"Bad packet length"になってしまう問題への対処。
		// (2007.10.29 yutaka)
		encryption_size = 4 + 1 + len;
		padding = block_size - (encryption_size % block_size);
		if (padding < 4)
			padding += block_size;
		encryption_size += padding;
		set_uint32(data, encryption_size - 4);
		data[4] = (unsigned char) padding;
		data_length = encryption_size + CRYPT_get_sender_MAC_size(pvar);
		if (msg) {
			// パケット圧縮の場合、バッファを拡張する。(2011.6.10 yutaka)
			buffer_append_space(msg, padding + EVP_MAX_MD_SIZE);
			// realloc()されると、ポインタが変わる可能性があるので、再度取り直す。
			data = buffer_ptr(msg);
		}
#endif
		//if (pvar->ssh_state.outbuflen <= 7 + data_length) *(int *)0 = 0;
		CRYPT_set_random_data(pvar, data + 5 + len, padding);
		ret = CRYPT_build_sender_MAC(pvar,
		                             pvar->ssh_state.sender_sequence_number,
		                             data, encryption_size,
		                             data + encryption_size);
		if (ret == FALSE) { // MACがまだ設定されていない場合
			data_length = encryption_size;
		}

		// パケットを暗号化する。MAC以降は暗号化対象外。
		CRYPT_encrypt(pvar, data, encryption_size);
	}

	send_packet_blocking(pvar, data, data_length);

	buffer_free(msg);

	pvar->ssh_state.sender_sequence_number++;

	// 送信時刻を記録
	pvar->ssh_heartbeat_tick = time(NULL);
}

static void destroy_packet_buf(PTInstVar pvar)
{
	memset(pvar->ssh_state.outbuf, 0, pvar->ssh_state.outbuflen);
	if (pvar->ssh_state.compressing) {
		memset(pvar->ssh_state.precompress_outbuf, 0,
		       pvar->ssh_state.precompress_outbuflen);
	}
}

/* The handlers are added to the queue for each message. When one of the
   handlers fires, if it returns FALSE, then all handlers in the set are
   removed from their queues. */
static void enque_handlers(PTInstVar pvar, int num_msgs,
                           const int *messages,
                           const SSHPacketHandler *handlers)
{
	SSHPacketHandlerItem *first_item;
	SSHPacketHandlerItem *last_item = NULL;
	int i;

	for (i = 0; i < num_msgs; i++) {
		SSHPacketHandlerItem *item =
			(SSHPacketHandlerItem *)
			malloc(sizeof(SSHPacketHandlerItem));
		SSHPacketHandlerItem *cur_item =
			pvar->ssh_state.packet_handlers[messages[i]];

		item->handler = handlers[i];

		if (cur_item == NULL) {
			pvar->ssh_state.packet_handlers[messages[i]] = item;
			item->next_for_message = item;
			item->last_for_message = item;
			item->active_for_message = messages[i];
		} else {
			item->next_for_message = cur_item;
			item->last_for_message = cur_item->last_for_message;
			cur_item->last_for_message->next_for_message = item;
			cur_item->last_for_message = item;
			item->active_for_message = -1;
		}

		if (last_item != NULL) {
			last_item->next_in_set = item;
		} else {
			first_item = item;
		}
		last_item = item;
	}

	if (last_item != NULL) {
		last_item->next_in_set = first_item;
	}
}

static SSHPacketHandler get_handler(PTInstVar pvar, int message)
{
	SSHPacketHandlerItem *cur_item =
		pvar->ssh_state.packet_handlers[message];

	if (cur_item == NULL) {
		return NULL;
	} else {
		return cur_item->handler;
	}
}

/* Called only by SSH_handle_packet */
static void deque_handlers(PTInstVar pvar, int message)
{
	SSHPacketHandlerItem *cur_item =
		pvar->ssh_state.packet_handlers[message];
	SSHPacketHandlerItem *first_item_in_set = cur_item;

	if (cur_item == NULL)
		return;

	do {
		SSHPacketHandlerItem *next_in_set = cur_item->next_in_set;

		if (cur_item->active_for_message >= 0) {
			SSHPacketHandlerItem *replacement =
				cur_item->next_for_message;

			if (replacement == cur_item) {
				replacement = NULL;
			} else {
				replacement->active_for_message =
					cur_item->active_for_message;
			}
			pvar->ssh_state.packet_handlers[cur_item->active_for_message] =
				replacement;
		}
		cur_item->next_for_message->last_for_message =
			cur_item->last_for_message;
		cur_item->last_for_message->next_for_message =
			cur_item->next_for_message;

		free(cur_item);
		cur_item = next_in_set;
	} while (cur_item != first_item_in_set);
}

static void enque_handler(PTInstVar pvar, int message,
                          SSHPacketHandler handler)
{
	enque_handlers(pvar, 1, &message, &handler);
}

static void chop_newlines(char *buf)
{
	int len = strlen(buf);

	while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
		buf[len - 1] = 0;
		len--;
	}
}

/********************/
/* Message handlers */
/********************/

static BOOL handle_forwarding_success(PTInstVar pvar)
{
	return FALSE;
}

static BOOL handle_forwarding_failure(PTInstVar pvar)
{
	return FALSE;
}

static void enque_forwarding_request_handlers(PTInstVar pvar)
{
	static const int msgs[] = { SSH_SMSG_SUCCESS, SSH_SMSG_FAILURE };
	static const SSHPacketHandler handlers[]
	= { handle_forwarding_success, handle_forwarding_failure };

	enque_handlers(pvar, 2, msgs, handlers);
}

static BOOL handle_auth_failure(PTInstVar pvar)
{
	logputs(LOG_LEVEL_VERBOSE, "Authentication failed");

	// retry countの追加 (2005.7.15 yutaka)
	pvar->userauth_retry_count++;

	AUTH_set_generic_mode(pvar);
	AUTH_advance_to_next_cred(pvar);
	pvar->ssh_state.status_flags &= ~STATUS_DONT_SEND_CREDENTIALS;
	try_send_credentials(pvar);
	return FALSE;
}

static BOOL handle_rsa_auth_refused(PTInstVar pvar)
{
	if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
		if (pvar->pageant_keycount <= pvar->pageant_keycurrent) {
			// 全ての鍵を試し終わった
			safefree(pvar->pageant_key);
		}
		else {
			// まだ鍵がある
			pvar->ssh_state.status_flags &= ~STATUS_DONT_SEND_CREDENTIALS;
			try_send_credentials(pvar);
			return TRUE;
		}
	}
	AUTH_destroy_cur_cred(pvar);
	return handle_auth_failure(pvar);
}

static BOOL handle_TIS_challenge(PTInstVar pvar)
{
	if (grab_payload(pvar, 4)) {
		int len = get_payload_uint32(pvar, 0);

		if (grab_payload(pvar, len)) {
			logputs(LOG_LEVEL_VERBOSE, "Received TIS challenge");

			AUTH_set_TIS_mode(pvar, pvar->ssh_state.payload + 4, len);
			AUTH_advance_to_next_cred(pvar);
			pvar->ssh_state.status_flags &= ~STATUS_DONT_SEND_CREDENTIALS;
			try_send_credentials(pvar);
		}
	}
	return FALSE;
}

static BOOL handle_auth_required(PTInstVar pvar)
{
	logputs(LOG_LEVEL_VERBOSE, "Server requires authentication");

	pvar->ssh_state.status_flags &= ~STATUS_DONT_SEND_CREDENTIALS;
	try_send_credentials(pvar);
	/* the first AUTH_advance_to_next_cred is issued early by ttxssh.c */

	return FALSE;
}

static BOOL handle_ignore(PTInstVar pvar)
{
	if (SSHv1(pvar)) {
		logputs(LOG_LEVEL_VERBOSE, "SSH_MSG_IGNORE was received.");

		if (grab_payload(pvar, 4)
		 && grab_payload(pvar, get_payload_uint32(pvar, 0))) {
			/* ignore it! but it must be decompressed */
		}
	}
	else {
		logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_IGNORE was received.");

		// メッセージが SSH2_MSG_IGNORE の時は何もしない
		// Cisco ルータ対策 (2006.11.28 maya)
	}
	return TRUE;
}

static BOOL handle_debug(PTInstVar pvar)
{
	BOOL always_display;
	char *description;
	int description_len;
	char buf[2048];

	if (SSHv1(pvar)) {
		logputs(LOG_LEVEL_VERBOSE, "SSH_MSG_DEBUG was received.");

		if (grab_payload(pvar, 4)
		 && grab_payload(pvar, description_len =
		                 get_payload_uint32(pvar, 0))) {
			always_display = FALSE;
			description = pvar->ssh_state.payload + 4;
			description[description_len] = 0;
		} else {
			return TRUE;
		}
	} else {
		logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_DEBUG was received.");

		if (grab_payload(pvar, 5)
		 && grab_payload(pvar,
		                 (description_len = get_payload_uint32(pvar, 1)) + 4)
		 && grab_payload(pvar,
		                 get_payload_uint32(pvar, 5 + description_len))) {
			always_display = pvar->ssh_state.payload[0] != 0;
			description = pvar->ssh_state.payload + 5;
			description[description_len] = 0;
		} else {
			return TRUE;
		}
	}

	chop_newlines(description);
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "DEBUG message from server: %s",
	            description);
	if (always_display) {
		notify_nonfatal_error(pvar, buf);
	} else {
		logputs(LOG_LEVEL_VERBOSE, buf);
	}
	return TRUE;
}

static BOOL handle_disconnect(PTInstVar pvar)
{
	int reason_code;
	char *description;
	int description_len;
	char buf[2048];
	char *explanation = "";
	char uimsg[MAX_UIMSG];

	if (SSHv1(pvar)) {
		logputs(LOG_LEVEL_VERBOSE, "SSH_MSG_DISCONNECT was received.");

		if (grab_payload(pvar, 4)
		 && grab_payload(pvar, description_len = get_payload_uint32(pvar, 0))) {
			reason_code = -1;
			description = pvar->ssh_state.payload + 4;
			description[description_len] = 0;
		} else {
			return TRUE;
		}
	} else {
		logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_DISCONNECT was received.");

		if (grab_payload(pvar, 8)
		 && grab_payload(pvar,
		                 (description_len = get_payload_uint32(pvar, 4)) + 4)
		 && grab_payload(pvar,
		                 get_payload_uint32(pvar, 8 + description_len))) {
			reason_code = get_payload_uint32(pvar, 0);
			description = pvar->ssh_state.payload + 8;
			description[description_len] = 0;
		} else {
			return TRUE;
		}
	}

	chop_newlines(description);
	if (description[0] == 0) {
		description = NULL;
	}

	if (get_handler(pvar, SSH_SMSG_FAILURE) == handle_forwarding_failure) {
		UTIL_get_lang_msg("MSG_SSH_UNABLE_FWD_ERROR", pvar,
		                  "\nIt may have disconnected because it was unable to forward a port you requested to be forwarded from the server.\n"
		                  "This often happens when someone is already forwarding that port from the server.");
		strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
		explanation = uimsg;
	}

	if (description != NULL) {
		UTIL_get_lang_msg("MSG_SSH_SERVER_DISCON_ERROR", pvar,
		                  "Server disconnected with message '%s'%s");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->ts->UIMsg, description,
		            explanation);
	} else {
		UTIL_get_lang_msg("MSG_SSH_SERVER_DISCON_NORES_ERROR", pvar,
		                  "Server disconnected (no reason given).%s");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->ts->UIMsg, explanation);
	}

	if (SSHv2(pvar)) {
		// SSH2_MSG_DISCONNECT を受け取ったあとは何も送信してはいけない
		notify_fatal_error(pvar, buf, FALSE);
	}
	else {
		// SSH1 の場合の仕様が分からないので、以前のままにしておく
		notify_fatal_error(pvar, buf, TRUE);
	}

	return TRUE;
}

static BOOL handle_unimplemented(PTInstVar pvar)
{
	/* Should never receive this since we only send base 2.0 protocol messages */
	grab_payload(pvar, 4);
	return TRUE;
}

static BOOL handle_crypt_success(PTInstVar pvar)
{
	logputs(LOG_LEVEL_VERBOSE, "Secure mode successfully achieved");
	return FALSE;
}

static BOOL handle_noauth_success(PTInstVar pvar)
{
	logputs(LOG_LEVEL_VERBOSE, "Server does not require authentication");
	prep_compression(pvar);
	return FALSE;
}

static BOOL handle_auth_success(PTInstVar pvar)
{
	logputs(LOG_LEVEL_VERBOSE, "Authentication accepted");
	prep_compression(pvar);

	// ハートビート・スレッドの開始 (2004.12.11 yutaka)
	start_ssh_heartbeat_thread(pvar);

	return FALSE;
}

static BOOL handle_server_public_key(PTInstVar pvar)
{
	int server_key_public_exponent_len;
	int server_key_public_modulus_pos;
	int server_key_public_modulus_len;
	int host_key_bits_pos;
	int host_key_public_exponent_len;
	int host_key_public_modulus_pos;
	int host_key_public_modulus_len;
	int protocol_flags_pos;
	int supported_ciphers;
	char *inmsg;
	Key hostkey;
	int supported_types;

	logputs(LOG_LEVEL_VERBOSE, "SSH_SMSG_PUBLIC_KEY was received.");

	if (!grab_payload(pvar, 14))
		return FALSE;
	server_key_public_exponent_len = get_mpint_len(pvar, 12);

	if (!grab_payload(pvar, server_key_public_exponent_len + 2))
		return FALSE;
	server_key_public_modulus_pos = 14 + server_key_public_exponent_len;
	server_key_public_modulus_len =
		get_mpint_len(pvar, server_key_public_modulus_pos);

	if (!grab_payload(pvar, server_key_public_modulus_len + 6))
		return FALSE;
	host_key_bits_pos =
		server_key_public_modulus_pos + 2 + server_key_public_modulus_len;
	host_key_public_exponent_len =
		get_mpint_len(pvar, host_key_bits_pos + 4);

	if (!grab_payload(pvar, host_key_public_exponent_len + 2))
		return FALSE;
	host_key_public_modulus_pos =
		host_key_bits_pos + 6 + host_key_public_exponent_len;
	host_key_public_modulus_len =
		get_mpint_len(pvar, host_key_public_modulus_pos);

	if (!grab_payload(pvar, host_key_public_modulus_len + 12))
		return FALSE;
	protocol_flags_pos =
		host_key_public_modulus_pos + 2 + host_key_public_modulus_len;

	inmsg = pvar->ssh_state.payload;

	CRYPT_set_server_cookie(pvar, inmsg);
	if (!CRYPT_set_server_RSA_key(pvar,
	                              get_uint32(inmsg + 8),
	                              pvar->ssh_state.payload + 12,
	                              inmsg + server_key_public_modulus_pos))
		return FALSE;
	if (!CRYPT_set_host_RSA_key(pvar,
	                            get_uint32(inmsg + host_key_bits_pos),
	                            inmsg + host_key_bits_pos + 4,
	                            inmsg + host_key_public_modulus_pos))
		return FALSE;
	pvar->ssh_state.server_protocol_flags =
		get_uint32(inmsg + protocol_flags_pos);

	supported_ciphers = get_uint32(inmsg + protocol_flags_pos + 4);
	if (!CRYPT_set_supported_ciphers(pvar,
	                                 supported_ciphers,
	                                 supported_ciphers))
		return FALSE;

	// SSH1 サーバは、サポートされている認証方式を送ってくる
	// RSA が有効なら PAGEANT を有効にする
	supported_types = get_uint32(inmsg + protocol_flags_pos + 8);
	if ((supported_types & (1 << SSH_AUTH_RSA)) > 0) {
		supported_types |= (1 << SSH_AUTH_PAGEANT);
	}
	if (!AUTH_set_supported_auth_types(pvar,
	                                   supported_types))
		return FALSE;

	/* this must be the LAST THING in this function, since it can cause
	   host_is_OK to be called. */
	hostkey.type = KEY_RSA1;
	hostkey.bits = get_uint32(inmsg + host_key_bits_pos);
	hostkey.exp = inmsg + host_key_bits_pos + 4;
	hostkey.mod = inmsg + host_key_public_modulus_pos;
	HOSTS_check_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, &hostkey);

	return FALSE;
}

/*
The ID must have already been found to start with "SSH-". It must
be null-terminated.
*/
static BOOL parse_protocol_ID(PTInstVar pvar, char *ID)
{
	char *str;

	for (str = ID + 4; *str >= '0' && *str <= '9'; str++) {
	}

	if (*str != '.') {
		return FALSE;
	}

	pvar->protocol_major = atoi(ID + 4);
	pvar->protocol_minor = atoi(str + 1);

	for (str = str + 1; *str >= '0' && *str <= '9'; str++) {
	}

	return *str == '-';
}

/*
On entry, the pvar->protocol_xxx fields hold the server's advertised
protocol number. We replace the fields with the protocol number we will
actually use, or return FALSE if there is no usable protocol version.
*/
static int negotiate_protocol(PTInstVar pvar)
{
	switch (pvar->protocol_major) {
	case 1:
		if (pvar->protocol_minor == 99 &&
		    pvar->settings.ssh_protocol_version == 2) {
			// サーバが 1.99 でユーザが SSH2 を選択しているのならば
			// 2.0 接続とする
			pvar->protocol_major = 2;
			pvar->protocol_minor = 0;
			return 0;
		}

		if (pvar->settings.ssh_protocol_version == 2) {
			// バージョン違い
			return -1;
		}

		if (pvar->protocol_minor > 5) {
			pvar->protocol_minor = 5;
		}

		return 0;

	// for SSH2(yutaka)
	case 2:
		if (pvar->settings.ssh_protocol_version == 1) {
			// バージョン違い
			return -1;
		}

		return 0;			// SSH2 support

	default:
		return 1;
	}
}

static void init_protocol(PTInstVar pvar)
{
	CRYPT_initialize_random_numbers(pvar);

	// known_hostsファイルからホスト公開鍵を先読みしておく
	HOSTS_prefetch_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport);

	/* while we wait for a response from the server... */

	if (SSHv1(pvar)) {
		enque_handler(pvar, SSH_MSG_DISCONNECT, handle_disconnect);
		enque_handler(pvar, SSH_MSG_IGNORE, handle_ignore);
		enque_handler(pvar, SSH_MSG_DEBUG, handle_debug);
		enque_handler(pvar, SSH_SMSG_PUBLIC_KEY, handle_server_public_key);

	} else {  // for SSH2(yutaka)
		enque_handler(pvar, SSH2_MSG_DISCONNECT, handle_disconnect);
		enque_handler(pvar, SSH2_MSG_IGNORE, handle_ignore);
		enque_handler(pvar, SSH2_MSG_DEBUG, handle_debug);
		enque_handler(pvar, SSH2_MSG_KEXINIT, handle_SSH2_kexinit);
		enque_handler(pvar, SSH2_MSG_KEXDH_INIT, handle_unimplemented);
		enque_handler(pvar, SSH2_MSG_KEXDH_REPLY, handle_SSH2_dh_common_reply);
		enque_handler(pvar, SSH2_MSG_KEX_DH_GEX_REPLY, handle_SSH2_dh_gex_reply);
		enque_handler(pvar, SSH2_MSG_NEWKEYS, handle_SSH2_newkeys);
		enque_handler(pvar, SSH2_MSG_SERVICE_ACCEPT, handle_SSH2_service_accept);
		enque_handler(pvar, SSH2_MSG_USERAUTH_SUCCESS, handle_SSH2_userauth_success);
		enque_handler(pvar, SSH2_MSG_USERAUTH_FAILURE, handle_SSH2_userauth_failure);
		enque_handler(pvar, SSH2_MSG_USERAUTH_BANNER, handle_SSH2_userauth_banner);
		enque_handler(pvar, SSH2_MSG_USERAUTH_INFO_REQUEST, handle_SSH2_userauth_msg60);

		enque_handler(pvar, SSH2_MSG_UNIMPLEMENTED, handle_unimplemented);

		// ユーザ認証後のディスパッチルーチン
		enque_handler(pvar, SSH2_MSG_CHANNEL_CLOSE, handle_SSH2_channel_close);
		enque_handler(pvar, SSH2_MSG_CHANNEL_DATA, handle_SSH2_channel_data);
		enque_handler(pvar, SSH2_MSG_CHANNEL_EOF, handle_SSH2_channel_eof);
		enque_handler(pvar, SSH2_MSG_CHANNEL_EXTENDED_DATA, handle_SSH2_channel_extended_data);
		enque_handler(pvar, SSH2_MSG_CHANNEL_OPEN, handle_SSH2_channel_open);
		enque_handler(pvar, SSH2_MSG_CHANNEL_OPEN_CONFIRMATION, handle_SSH2_open_confirm);
		enque_handler(pvar, SSH2_MSG_CHANNEL_OPEN_FAILURE, handle_SSH2_open_failure);
		enque_handler(pvar, SSH2_MSG_CHANNEL_REQUEST, handle_SSH2_channel_request);
		enque_handler(pvar, SSH2_MSG_CHANNEL_WINDOW_ADJUST, handle_SSH2_window_adjust);
		enque_handler(pvar, SSH2_MSG_CHANNEL_SUCCESS, handle_SSH2_channel_success);
		enque_handler(pvar, SSH2_MSG_CHANNEL_FAILURE, handle_SSH2_channel_failure);
		enque_handler(pvar, SSH2_MSG_GLOBAL_REQUEST, handle_SSH2_client_global_request);
		enque_handler(pvar, SSH2_MSG_REQUEST_FAILURE, handle_SSH2_request_failure);
		enque_handler(pvar, SSH2_MSG_REQUEST_SUCCESS, handle_SSH2_request_success);

		client_init_global_confirm();

	}
}

void server_version_check(PTInstVar pvar)
{
	char *server_swver;

	pvar->server_compat_flag = 0;

	if ((server_swver = strchr(pvar->server_version_string+4, '-')) == NULL) {
		logputs(LOG_LEVEL_WARNING, "Can't get server software version string.");
		return;
	}
	server_swver++;

	if (strncmp(server_swver, "Cisco-1", 7) == 0) {
		pvar->server_compat_flag |= SSH_BUG_DHGEX_LARGE;
		logputs(LOG_LEVEL_INFO, "Server version string is matched to \"Cisco-1\", compatibility flag SSH_BUG_DHGEX_LARGE is enabled.");
	}
}

BOOL SSH_handle_server_ID(PTInstVar pvar, char *ID, int ID_len)
{
	static char prefix[64];
	int negotiate;
	char uimsg[MAX_UIMSG];

	// initialize SSH2 memory dump (2005.3.7 yutaka)
	init_memdump();
	push_memdump("pure server ID", "start protocol version exchange", ID, ID_len);

	if (ID_len <= 0) {
		return FALSE;
	} else {
		int buf_len;
		char *buf;

		strncpy_s(prefix, sizeof(prefix), "Received server identification string: ", _TRUNCATE);
		buf_len = strlen(prefix) + ID_len + 1;
		buf = (char *) malloc(buf_len);
		strncpy_s(buf, buf_len, prefix, _TRUNCATE);
		strncat_s(buf, buf_len, ID, _TRUNCATE);
		chop_newlines(buf);
		logputs(LOG_LEVEL_VERBOSE, buf);
		free(buf);

		if (ID[ID_len - 1] != '\n') {
			pvar->ssh_state.status_flags |= STATUS_IN_PARTIAL_ID_STRING;
			return FALSE;
		} else if ((pvar->ssh_state.status_flags & STATUS_IN_PARTIAL_ID_STRING) != 0) {
			pvar->ssh_state.status_flags &= ~STATUS_IN_PARTIAL_ID_STRING;
			return FALSE;
		} else if (strncmp(ID, "SSH-", 4) != 0) {
			return FALSE;
		} else {
			ID[ID_len - 1] = 0;

			if (ID_len > 1 && ID[ID_len - 2] == '\r') {
				ID[ID_len - 2] = 0;
			}

			pvar->ssh_state.server_ID = _strdup(ID);

			if (!parse_protocol_ID(pvar, ID)) {
				UTIL_get_lang_msg("MSG_SSH_VERSION_ERROR", pvar,
				                  "This program does not understand the server's version of the protocol.");
				notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
			}
			else if ((negotiate = negotiate_protocol(pvar)) == 1) {
				UTIL_get_lang_msg("MSG_SSH_VERSION_ERROR", pvar,
				                  "This program does not understand the server's version of the protocol.");
				notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
			}
			else if (negotiate == -1) {
				UTIL_get_lang_msg("MSG_SSH_VERSION_MISMATCH", pvar,
				                  "Protocol version mismatch. server:%d.%d client:%d");
				_snprintf_s(uimsg, sizeof(uimsg), _TRUNCATE, pvar->ts->UIMsg,
				            pvar->protocol_major, pvar->protocol_minor, pvar->settings.ssh_protocol_version);
				notify_fatal_error(pvar, uimsg, TRUE);
			}
			else {
				char TTSSH_ID[1024];
				int TTSSH_ID_len;

				// SSH バージョンを teraterm 側にセットする
				// SCP コマンドのため (2008.2.3 maya)
				pvar->cv->isSSH = pvar->protocol_major;

				// 自分自身のバージョンを取得する (2005.3.3 yutaka)
				_snprintf_s(TTSSH_ID, sizeof(TTSSH_ID), _TRUNCATE,
				            "SSH-%d.%d-TTSSH/%d.%d Win32\r\n",
				            pvar->protocol_major, pvar->protocol_minor,
				            TTSSH_VERSION_MAJOR, TTSSH_VERSION_MINOR);
				TTSSH_ID_len = strlen(TTSSH_ID);

				// for SSH2(yutaka)
				// クライアントバージョンの保存（改行は取り除くこと）
				strncpy_s(pvar->client_version_string, sizeof(pvar->client_version_string),
				          TTSSH_ID, _TRUNCATE);

				// サーババージョンの保存（改行は取り除くこと）(2005.3.9 yutaka)
				_snprintf_s(pvar->server_version_string,
				            sizeof(pvar->server_version_string), _TRUNCATE,
				            "%s", pvar->ssh_state.server_ID);

				// サーババージョンのチェック
				server_version_check(pvar);

				if ((pvar->Psend) (pvar->socket, TTSSH_ID, TTSSH_ID_len, 0) != TTSSH_ID_len) {
					UTIL_get_lang_msg("MSG_SSH_SEND_ID_ERROR", pvar,
					                  "An error occurred while sending the SSH ID string.\n"
					                  "The connection will close.");
					notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
				} else {
					// 改行の除去
					chop_newlines(pvar->client_version_string);
					logprintf(LOG_LEVEL_VERBOSE, "Sent client identification string: %s", pvar->client_version_string);

					push_memdump("server ID", NULL, pvar->server_version_string, strlen(pvar->server_version_string));
					push_memdump("client ID", NULL, pvar->client_version_string, strlen(pvar->client_version_string));

					// SSHハンドラの登録を行う
					init_protocol(pvar);

					SSH2_dispatch_init(1);
					SSH2_dispatch_add_message(SSH2_MSG_KEXINIT);
					SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX: Tru64 UNIX workaround   (2005.3.3 yutaka)
					SSH2_dispatch_add_message(SSH2_MSG_DEBUG);
				}
			}

			return TRUE;
		}
	}
}

static BOOL handle_exit(PTInstVar pvar)
{
	if (grab_payload(pvar, 4)) {
		begin_send_packet(pvar, SSH_CMSG_EXIT_CONFIRMATION, 0);
		finish_send_packet(pvar);
		notify_closed_connection(pvar, "disconnected by server request");
	}
	return TRUE;
}

static BOOL handle_data(PTInstVar pvar)
{
	if (grab_payload_limited(pvar, 4)) {
		pvar->ssh_state.payload_datalen = get_payload_uint32(pvar, 0);
		pvar->ssh_state.payload_datastart = 4;
	}
	return TRUE;
}

static BOOL handle_channel_open(PTInstVar pvar)
{
	int host_len;
	int originator_len;

	if ((pvar->ssh_state.
		 server_protocol_flags & SSH_PROTOFLAG_HOST_IN_FWD_OPEN) != 0) {
		if (grab_payload(pvar, 8)
		 && grab_payload(pvar,
		                 8 + (host_len = get_payload_uint32(pvar, 4)))
		 && grab_payload(pvar, originator_len =
		                 get_payload_uint32(pvar, host_len + 12))) {
			int local_port = get_payload_uint32(pvar, 8 + host_len);

			pvar->ssh_state.payload[8 + host_len] = 0;
			FWD_open(pvar, get_payload_uint32(pvar, 0),
			         pvar->ssh_state.payload + 8, local_port,
			         pvar->ssh_state.payload + 16 + host_len,
			         originator_len,
			         NULL);
		}
	} else {
		if (grab_payload(pvar, 8)
		 && grab_payload(pvar,
		                 4 + (host_len = get_payload_uint32(pvar, 4)))) {
			int local_port = get_payload_uint32(pvar, 8 + host_len);

			pvar->ssh_state.payload[8 + host_len] = 0;
			FWD_open(pvar, get_payload_uint32(pvar, 0),
			         pvar->ssh_state.payload + 8, local_port, NULL, 0,
			         NULL);
		}
	}

	return TRUE;
}

static BOOL handle_X11_channel_open(PTInstVar pvar)
{
	int originator_len;

	if ((pvar->ssh_state.server_protocol_flags & SSH_PROTOFLAG_HOST_IN_FWD_OPEN) != 0) {
		if (grab_payload(pvar, 8)
		 && grab_payload(pvar, originator_len = get_payload_uint32(pvar, 4))) {
			FWD_X11_open(pvar, get_payload_uint32(pvar, 0),
			             pvar->ssh_state.payload + 8, originator_len, NULL);
		}
	} else {
		if (grab_payload(pvar, 4)) {
			FWD_X11_open(pvar, get_payload_uint32(pvar, 0), NULL, 0, NULL);
		}
	}

	return TRUE;
}

static BOOL handle_channel_open_confirmation(PTInstVar pvar)
{
	if (grab_payload(pvar, 8)) {
		FWD_confirmed_open(pvar, get_payload_uint32(pvar, 0),
		                   get_payload_uint32(pvar, 4));
	}
	return FALSE;
}

static BOOL handle_channel_open_failure(PTInstVar pvar)
{
	if (grab_payload(pvar, 4)) {
		FWD_failed_open(pvar, get_payload_uint32(pvar, 0), -1);
	}
	return FALSE;
}

static BOOL handle_channel_data(PTInstVar pvar)
{
	int len;

	if (grab_payload(pvar, 8)
	 && grab_payload(pvar, len = get_payload_uint32(pvar, 4))) {
		FWDChannel *channel;
		int local_channel_num = get_payload_uint32(pvar, 0);
		if (!FWD_check_local_channel_num(pvar, local_channel_num)) {
			return FALSE;
		}
		channel = pvar->fwd_state.channels + local_channel_num;
		if (channel->type == TYPE_AGENT) {
			SSH_agent_response(pvar, NULL, local_channel_num,
			                   pvar->ssh_state.payload + 8, len);
		}
		else {
			FWD_received_data(pvar, local_channel_num,
			                  pvar->ssh_state.payload + 8, len);
		}
	}
	return TRUE;
}

static BOOL handle_channel_input_eof(PTInstVar pvar)
{
	if (grab_payload(pvar, 4)) {
		int local_channel_num = get_payload_uint32(pvar, 0);
		FWDChannel *channel;
		if (!FWD_check_local_channel_num(pvar, local_channel_num)) {
			return FALSE;
		}
		channel = pvar->fwd_state.channels + local_channel_num;
		if (channel->type == TYPE_AGENT) {
			channel->status |= FWD_CLOSED_REMOTE_IN;
			SSH_channel_input_eof(pvar, channel->remote_num, local_channel_num);
		}
		else {
			FWD_channel_input_eof(pvar, local_channel_num);
		}
	}
	return TRUE;
}

static BOOL handle_channel_output_eof(PTInstVar pvar)
{
	if (grab_payload(pvar, 4)) {
		int local_channel_num = get_payload_uint32(pvar, 0);
		FWDChannel *channel;
		if (!FWD_check_local_channel_num(pvar, local_channel_num)) {
			return FALSE;
		}
		channel = pvar->fwd_state.channels + local_channel_num;
		if (channel->type == TYPE_AGENT) {
			channel->status |= FWD_CLOSED_REMOTE_OUT;
			SSH_channel_output_eof(pvar, channel->remote_num);
			FWD_free_channel(pvar, local_channel_num);
		}
		else {
			FWD_channel_output_eof(pvar, local_channel_num);
		}
	}
	return TRUE;
}

static BOOL handle_agent_open(PTInstVar pvar)
{
	if (grab_payload(pvar, 4)) {
		int remote_id = get_payload_uint32(pvar, 0);
		int local_id;

		if (pvar->agentfwd_enable && FWD_agent_forward_confirm(pvar)) {
			local_id = FWD_agent_open(pvar, remote_id);
			if (local_id == -1) {
				SSH_fail_channel_open(pvar, remote_id);
			}
			else {
				SSH_confirm_channel_open(pvar, remote_id, local_id);
			}
		}
		else {
			SSH_fail_channel_open(pvar, remote_id);
		}
	}
	/*
	else {
		// 通知する相手channelが分からないので何もできない
	}
	*/

	return TRUE;
}



// ハンドリングするメッセージを決定する

#define HANDLE_MESSAGE_MAX 30
static unsigned char handle_messages[HANDLE_MESSAGE_MAX];
static int handle_message_count = 0;
static int handle_message_stage = 0;

void SSH2_dispatch_init(int stage)
{
	handle_message_count = 0;
	handle_message_stage = stage;
}

int SSH2_dispatch_enabled_check(unsigned char message)
{
	int i;

	for (i = 0 ; i < handle_message_count ; i++) {
		if (handle_messages[i] == message)
			return 1;
	}
	return 0;
}

void SSH2_dispatch_add_message(unsigned char message)
{
	int i;

	if (handle_message_count >= HANDLE_MESSAGE_MAX) {
		// TODO: error check
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": too many handlers. handlers:%d, max:%d",
			handle_message_count, HANDLE_MESSAGE_MAX);
		return;
	}

	// すでに登録されているメッセージは追加しない
	for (i=0; i<handle_message_count; i++) {
		if (handle_messages[i] == message) {
			return;
		}
	}

	handle_messages[handle_message_count++] = message;
}

void SSH2_dispatch_add_range_message(unsigned char begin, unsigned char end)
{
	unsigned char c;

	for (c = begin ; c <= end ; c++) {
		SSH2_dispatch_add_message(c);
	}
}


void SSH_handle_packet(PTInstVar pvar, char *data, int len,
					   int padding)
{
	unsigned char message = prep_packet(pvar, data, len, padding);

	// SSHのメッセージタイプをチェック
	if (message != SSH_MSG_NONE) {
		// メッセージタイプに応じたハンドラを起動
		SSHPacketHandler handler = get_handler(pvar, message);

		// for SSH2(yutaka)
		if (SSHv2(pvar)) {
			// 想定外のメッセージタイプが到着したらアボートさせる。
			if (!SSH2_dispatch_enabled_check(message) || handler == NULL) {
				char buf[1024];

				UTIL_get_lang_msg("MSG_SSH_UNEXP_MSG2_ERROR", pvar,
								  "Unexpected SSH2 message(%d) on current stage(%d)");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					pvar->ts->UIMsg, message, handle_message_stage);
				notify_fatal_error(pvar, buf, TRUE);
				return;
			}
		}

		if (handler == NULL) {
			if (SSHv1(pvar)) {
				char buf[1024];

				UTIL_get_lang_msg("MSG_SSH_UNEXP_MSG_ERROR", pvar,
				                  "Unexpected packet type received: %d");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					pvar->ts->UIMsg, message, handle_message_stage);
				notify_fatal_error(pvar, buf, TRUE);
			} else {
				unsigned char *outmsg =
					begin_send_packet(pvar, SSH2_MSG_UNIMPLEMENTED, 4);

				set_uint32(outmsg,
				           pvar->ssh_state.receiver_sequence_number - 1);
				finish_send_packet(pvar);

				logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_UNIMPLEMENTED was sent at SSH_handle_packet().");
				/* XXX need to decompress incoming packet, but how? */
			}
		} else {
			if (!handler(pvar)) {
				deque_handlers(pvar, message);
			}
		}
	}
}

static BOOL handle_pty_success(PTInstVar pvar)
{
	FWD_enter_interactive_mode(pvar);
	enque_handler(pvar, SSH_SMSG_EXITSTATUS, handle_exit);
	enque_handler(pvar, SSH_SMSG_STDOUT_DATA, handle_data);
	enque_handler(pvar, SSH_SMSG_STDERR_DATA, handle_data);
	enque_handler(pvar, SSH_MSG_CHANNEL_DATA, handle_channel_data);
	enque_handler(pvar, SSH_MSG_CHANNEL_INPUT_EOF,
	              handle_channel_input_eof);
	enque_handler(pvar, SSH_MSG_CHANNEL_OUTPUT_CLOSED,
	              handle_channel_output_eof);
	enque_handler(pvar, SSH_MSG_PORT_OPEN, handle_channel_open);
	enque_handler(pvar, SSH_SMSG_X11_OPEN, handle_X11_channel_open);
	enque_handler(pvar, SSH_SMSG_AGENT_OPEN, handle_agent_open);
	return FALSE;
}

static BOOL handle_pty_failure(PTInstVar pvar)
{
	UTIL_get_lang_msg("MSG_SSH_ALLOC_TERMINAL_ERROR", pvar,
	                  "The server cannot allocate a pseudo-terminal. "
	                  "You may encounter some problems with the terminal.");
	notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	return handle_pty_success(pvar);
}

static void prep_pty(PTInstVar pvar)
{
	int len = strlen(pvar->ts->TermType);
	unsigned char *outmsg =
		begin_send_packet(pvar, SSH_CMSG_REQUEST_PTY,
		                  4 + len + 16 + sizeof(ssh_ttymodes));
	static const int msgs[] = { SSH_SMSG_SUCCESS, SSH_SMSG_FAILURE };
	static const SSHPacketHandler handlers[]
	= { handle_pty_success, handle_pty_failure };
	int x, y;

	get_window_pixel_size(pvar, &x, &y);

	set_uint32(outmsg, len);
	memcpy(outmsg + 4, pvar->ts->TermType, len);
	set_uint32(outmsg + 4 + len, pvar->ssh_state.win_rows);
	set_uint32(outmsg + 4 + len + 4, pvar->ssh_state.win_cols);
	set_uint32(outmsg + 4 + len + 8, x);
	set_uint32(outmsg + 4 + len + 12, y);
	memcpy(outmsg + 4 + len + 16, ssh_ttymodes, sizeof(ssh_ttymodes));
	finish_send_packet(pvar);

	enque_handlers(pvar, 2, msgs, handlers);

	begin_send_packet(pvar, SSH_CMSG_EXEC_SHELL, 0);
	finish_send_packet(pvar);
}

static BOOL handle_agent_request_success(PTInstVar pvar)
{
	pvar->agentfwd_enable = TRUE;
	prep_pty(pvar);
	return FALSE;
}

static BOOL handle_agent_request_failure(PTInstVar pvar)
{
	prep_pty(pvar);
	return FALSE;
}

static void prep_agent_request(PTInstVar pvar)
{
	static const int msgs[] = { SSH_SMSG_SUCCESS, SSH_SMSG_FAILURE };
	static const SSHPacketHandler handlers[]
	= { handle_agent_request_success, handle_agent_request_failure };

	enque_handlers(pvar, 2, msgs, handlers);

	begin_send_packet(pvar, SSH_CMSG_AGENT_REQUEST_FORWARDING, 0);
	finish_send_packet(pvar);
}

static void prep_forwarding(PTInstVar pvar)
{
	FWD_prep_forwarding(pvar);

	if (pvar->session_settings.ForwardAgent) {
		prep_agent_request(pvar);
	}
	else {
		prep_pty(pvar);
	}
}


//
//
// (2005.7.10 yutaka)
static void enable_send_compression(PTInstVar pvar)
{
	static int initialize = 0;

	if (initialize) {
		deflateEnd(&pvar->ssh_state.compress_stream);
	}
	initialize = 1;

	pvar->ssh_state.compress_stream.zalloc = NULL;
	pvar->ssh_state.compress_stream.zfree = NULL;
	pvar->ssh_state.compress_stream.opaque = NULL;
	if (deflateInit
	    (&pvar->ssh_state.compress_stream,
	     pvar->ssh_state.compression_level) != Z_OK) {
		UTIL_get_lang_msg("MSG_SSH_SETUP_COMP_ERROR", pvar,
		                  "An error occurred while setting up compression.\n"
		                  "The connection will close.");
		notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
		return;
	} else {
		// SSH2では圧縮・展開処理をSSH1とは別に行うので、下記フラグは落としておく。(2005.7.9 yutaka)
		if (SSHv2(pvar)) {
			pvar->ssh_state.compressing = FALSE;
		} else {
			pvar->ssh_state.compressing = TRUE;
		}
	}
}

static void enable_recv_compression(PTInstVar pvar)
{
	static int initialize = 0;

	if (initialize) {
		deflateEnd(&pvar->ssh_state.decompress_stream);
	}
	initialize = 1;

	pvar->ssh_state.decompress_stream.zalloc = NULL;
	pvar->ssh_state.decompress_stream.zfree = NULL;
	pvar->ssh_state.decompress_stream.opaque = NULL;
	if (inflateInit(&pvar->ssh_state.decompress_stream) != Z_OK) {
		deflateEnd(&pvar->ssh_state.compress_stream);
		UTIL_get_lang_msg("MSG_SSH_SETUP_COMP_ERROR", pvar,
		                  "An error occurred while setting up compression.\n"
		                  "The connection will close.");
		notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
		return;
	} else {
		// SSH2では圧縮・展開処理をSSH1とは別に行うので、下記フラグは落としておく。(2005.7.9 yutaka)
		if (SSHv2(pvar)) {
			pvar->ssh_state.decompressing = FALSE;
		} else {
			pvar->ssh_state.decompressing = TRUE;
		}

		buf_ensure_size(&pvar->ssh_state.postdecompress_inbuf,
		                &pvar->ssh_state.postdecompress_inbuflen, 1000);
	}
}

static void enable_compression(PTInstVar pvar)
{
	enable_send_compression(pvar);
	enable_recv_compression(pvar);

	// SSH2では圧縮・展開処理をSSH1とは別に行うので、下記フラグは落としておく。(2005.7.9 yutaka)
	if (SSHv2(pvar)) {
		pvar->ssh_state.compressing = FALSE;
		pvar->ssh_state.decompressing = FALSE;
	}

}

static BOOL handle_enable_compression(PTInstVar pvar)
{
	enable_compression(pvar);
	prep_forwarding(pvar);
	return FALSE;
}

static BOOL handle_disable_compression(PTInstVar pvar)
{
	prep_forwarding(pvar);
	return FALSE;
}

static void prep_compression(PTInstVar pvar)
{
	if (pvar->session_settings.CompressionLevel > 0) {
		// added if statement (2005.7.10 yutaka)
		if (SSHv1(pvar)) {
			static const int msgs[] = { SSH_SMSG_SUCCESS, SSH_SMSG_FAILURE };
			static const SSHPacketHandler handlers[]
			= { handle_enable_compression, handle_disable_compression };

			unsigned char *outmsg =
				begin_send_packet(pvar, SSH_CMSG_REQUEST_COMPRESSION, 4);

			set_uint32(outmsg, pvar->session_settings.CompressionLevel);
			finish_send_packet(pvar);

			enque_handlers(pvar, 2, msgs, handlers);
		}

		pvar->ssh_state.compression_level =
			pvar->session_settings.CompressionLevel;

	} else {
		// added if statement (2005.7.10 yutaka)
		if (SSHv1(pvar)) {
			prep_forwarding(pvar);
		}
	}
}

static void enque_simple_auth_handlers(PTInstVar pvar)
{
	static const int msgs[] = { SSH_SMSG_SUCCESS, SSH_SMSG_FAILURE };
	static const SSHPacketHandler handlers[]
	= { handle_auth_success, handle_auth_failure };

	enque_handlers(pvar, 2, msgs, handlers);
}

static BOOL handle_rsa_challenge(PTInstVar pvar)
{
	int challenge_bytes;

	if (!grab_payload(pvar, 2)) {
		return FALSE;
	}

	challenge_bytes = get_mpint_len(pvar, 0);

	if (grab_payload(pvar, challenge_bytes)) {
		unsigned char *outmsg =
			begin_send_packet(pvar, SSH_CMSG_AUTH_RSA_RESPONSE, 16);

		if (pvar->auth_state.cur_cred.method == SSH_AUTH_RSA) {
			if (CRYPT_generate_RSA_challenge_response
				(pvar, pvar->ssh_state.payload + 2, challenge_bytes, outmsg)) {

				// セッション複製時にパスワードを使い回したいので、ここでのリソース解放はやめる。
				// socket close時にもこの関数は呼ばれているので、たぶん問題ない。(2005.4.8 yutaka)
#if 0
				//AUTH_destroy_cur_cred(pvar);
#endif

				finish_send_packet(pvar);

				enque_simple_auth_handlers(pvar);
			} else {
				UTIL_get_lang_msg("MSG_SSH_DECRYPT_RSA_ERROR", pvar,
								  "An error occurred while decrypting the RSA challenge.\n"
								  "Perhaps the key file is corrupted.");
				notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
			}
		}
		else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
			int server_key_bits = BN_num_bits(pvar->crypt_state.server_key.RSA_key->n);
			int host_key_bits = BN_num_bits(pvar->crypt_state.host_key.RSA_key->n);
			int server_key_bytes = (server_key_bits + 7) / 8;
			int host_key_bytes = (host_key_bits + 7) / 8;
			int session_buf_len = server_key_bytes + host_key_bytes + 8;
			char *session_buf = (char *) malloc(session_buf_len);
			unsigned char session_id[16];

			unsigned char *hash;
			int pubkeylen, hashlen;

			/* Pageant にハッシュを計算してもらう */
			// 公開鍵の長さ
			pubkeylen = putty_get_ssh1_keylen(pvar->pageant_curkey,
			                                  pvar->pageant_keylistlen);
			// セッションIDを作成
			BN_bn2bin(pvar->crypt_state.host_key.RSA_key->n, session_buf);
			BN_bn2bin(pvar->crypt_state.server_key.RSA_key->n,
			          session_buf + host_key_bytes);
			memcpy(session_buf + server_key_bytes + host_key_bytes,
			       pvar->crypt_state.server_cookie, 8);
			MD5(session_buf, session_buf_len, session_id);
			// ハッシュを受け取る
			hash = putty_hash_ssh1_challenge(pvar->pageant_curkey,
			                                 pubkeylen,
			                                 pvar->ssh_state.payload,
			                                 challenge_bytes + 2,
			                                 session_id,
			                                 &hashlen);

			// ハッシュを送信
			memcpy(outmsg, hash, 16);
			free(hash);

			finish_send_packet(pvar);

			enque_simple_auth_handlers(pvar);
		}
	}

	return FALSE;
}

static void try_send_credentials(PTInstVar pvar)
{
	if ((pvar->ssh_state.status_flags & STATUS_DONT_SEND_CREDENTIALS) == 0) {
		AUTHCred *cred = AUTH_get_cur_cred(pvar);
		static const int RSA_msgs[] =
			{ SSH_SMSG_AUTH_RSA_CHALLENGE, SSH_SMSG_FAILURE };
		static const SSHPacketHandler RSA_handlers[]
		= { handle_rsa_challenge, handle_rsa_auth_refused };
		static const int TIS_msgs[] =
			{ SSH_SMSG_AUTH_TIS_CHALLENGE, SSH_SMSG_FAILURE };
		static const SSHPacketHandler TIS_handlers[]
		= { handle_TIS_challenge, handle_auth_failure };

		// SSH2の場合は以下の処理をスキップ
		if (SSHv2(pvar))
			goto skip_ssh2;

		switch (cred->method) {
		case SSH_AUTH_NONE:
			return;
		case SSH_AUTH_PASSWORD:{
				int len = strlen(cred->password);
				unsigned char *outmsg =
					begin_send_packet(pvar, SSH_CMSG_AUTH_PASSWORD,
					                  4 + len);

				logputs(LOG_LEVEL_VERBOSE, "Trying PASSWORD authentication...");

				set_uint32(outmsg, len);
				memcpy(outmsg + 4, cred->password, len);
				
				// セッション複製時にパスワードを使い回したいので、ここでのリソース解放はやめる。
				// socket close時にもこの関数は呼ばれているので、たぶん問題ない。(2005.4.8 yutaka)
#if 0
				//AUTH_destroy_cur_cred(pvar);
#endif
				
				enque_simple_auth_handlers(pvar);
				break;
			}
		case SSH_AUTH_RHOSTS:{
				int len = strlen(cred->rhosts_client_user);
				unsigned char *outmsg =
					begin_send_packet(pvar, SSH_CMSG_AUTH_RHOSTS, 4 + len);

				logputs(LOG_LEVEL_VERBOSE, "Trying RHOSTS authentication...");

				set_uint32(outmsg, len);
				memcpy(outmsg + 4, cred->rhosts_client_user, len);
				AUTH_destroy_cur_cred(pvar);
				enque_simple_auth_handlers(pvar);
				break;
			}
		case SSH_AUTH_RSA:{
				int len = BN_num_bytes(cred->key_pair->rsa->n);
				unsigned char *outmsg =
					begin_send_packet(pvar, SSH_CMSG_AUTH_RSA, 2 + len);

				logputs(LOG_LEVEL_VERBOSE, "Trying RSA authentication...");

				set_ushort16_MSBfirst(outmsg, len * 8);
				BN_bn2bin(cred->key_pair->rsa->n, outmsg + 2);
				/* don't destroy the current credentials yet */
				enque_handlers(pvar, 2, RSA_msgs, RSA_handlers);
				break;
			}
		case SSH_AUTH_RHOSTS_RSA:{
				int mod_len = BN_num_bytes(cred->key_pair->rsa->n);
				int name_len = strlen(cred->rhosts_client_user);
				int exp_len = BN_num_bytes(cred->key_pair->rsa->e);
				int index;
				unsigned char *outmsg =
					begin_send_packet(pvar, SSH_CMSG_AUTH_RHOSTS_RSA,
					                  12 + mod_len + name_len + exp_len);

				logputs(LOG_LEVEL_VERBOSE, "Trying RHOSTS+RSA authentication...");

				set_uint32(outmsg, name_len);
				memcpy(outmsg + 4, cred->rhosts_client_user, name_len);
				index = 4 + name_len;

				set_uint32(outmsg + index, 8 * mod_len);
				set_ushort16_MSBfirst(outmsg + index + 4, 8 * exp_len);
				BN_bn2bin(cred->key_pair->rsa->e, outmsg + index + 6);
				index += 6 + exp_len;

				set_ushort16_MSBfirst(outmsg + index, 8 * mod_len);
				BN_bn2bin(cred->key_pair->rsa->n, outmsg + index + 2);
				/* don't destroy the current credentials yet */
				enque_handlers(pvar, 2, RSA_msgs, RSA_handlers);
				break;
			}
		case SSH_AUTH_PAGEANT:{
				unsigned char *outmsg;
				unsigned char *pubkey;
				int len, bn_bytes;

				if (pvar->pageant_keycurrent != 0) {
					// 直前の鍵をスキップ
					pvar->pageant_curkey += 4;
					len = get_ushort16_MSBfirst(pvar->pageant_curkey);
					bn_bytes = (len + 7) / 8;
					pvar->pageant_curkey += 2 + bn_bytes;
					len = get_ushort16_MSBfirst(pvar->pageant_curkey);
					bn_bytes = (len + 7) / 8;
					pvar->pageant_curkey += 2 + bn_bytes;
					// 直前の鍵のコメントをスキップ
					len = get_uint32_MSBfirst(pvar->pageant_curkey);
					pvar->pageant_curkey += 4 + len;
					// 次の鍵の位置へ来る
				}
				pubkey = pvar->pageant_curkey + 4;
				len = get_ushort16_MSBfirst(pubkey);
				bn_bytes = (len + 7) / 8;
				pubkey += 2 + bn_bytes;
				len = get_ushort16_MSBfirst(pubkey);
				bn_bytes = (len + 7) / 8;
				pubkey += 2;
				outmsg = begin_send_packet(pvar, SSH_CMSG_AUTH_RSA, 2 + bn_bytes);

				logputs(LOG_LEVEL_VERBOSE, "Trying RSA authentication...");

				set_ushort16_MSBfirst(outmsg, bn_bytes * 8);
				memcpy(outmsg + 2, pubkey, bn_bytes);
				/* don't destroy the current credentials yet */

				pvar->pageant_keycurrent++;

				enque_handlers(pvar, 2, RSA_msgs, RSA_handlers);
				break;
			}
		case SSH_AUTH_TIS:{
				if (cred->password == NULL) {
					unsigned char *outmsg =
						begin_send_packet(pvar, SSH_CMSG_AUTH_TIS, 0);

					logputs(LOG_LEVEL_VERBOSE, "Trying TIS authentication...");
					enque_handlers(pvar, 2, TIS_msgs, TIS_handlers);
				} else {
					int len = strlen(cred->password);
					unsigned char *outmsg =
						begin_send_packet(pvar, SSH_CMSG_AUTH_TIS_RESPONSE,
						                  4 + len);

					logputs(LOG_LEVEL_VERBOSE, "Sending TIS response");

					set_uint32(outmsg, len);
					memcpy(outmsg + 4, cred->password, len);
					enque_simple_auth_handlers(pvar);
				}

				AUTH_destroy_cur_cred(pvar);
				break;
			}
		default:
			UTIL_get_lang_msg("MSG_SSH_UNSUPPORT_AUTH_METHOD_ERROR", pvar,
			                  "Internal error: unsupported authentication method");
			notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
			return;
		}

		finish_send_packet(pvar);

skip_ssh2:;
		destroy_packet_buf(pvar);

		pvar->ssh_state.status_flags |= STATUS_DONT_SEND_CREDENTIALS;
	}
}

static void try_send_user_name(PTInstVar pvar)
{
	if ((pvar->ssh_state.status_flags & STATUS_DONT_SEND_USER_NAME) == 0) {
		char *username = AUTH_get_user_name(pvar);

		if (username != NULL) {
			int len = strlen(username);
			unsigned char *outmsg =
				begin_send_packet(pvar, SSH_CMSG_USER, 4 + len);
			static const int msgs[] =
				{ SSH_SMSG_SUCCESS, SSH_SMSG_FAILURE };
			static const SSHPacketHandler handlers[]
			= { handle_noauth_success, handle_auth_required };

			set_uint32(outmsg, len);
			memcpy(outmsg + 4, username, len);
			finish_send_packet(pvar);

			pvar->ssh_state.status_flags |= STATUS_DONT_SEND_USER_NAME;

			logprintf(LOG_LEVEL_VERBOSE, "Sending user name: %s", username);

			enque_handlers(pvar, 2, msgs, handlers);
		}
	}
}

static void send_session_key(PTInstVar pvar)
{
	int encrypted_session_key_len;
	unsigned char *outmsg;

	if (SSHv1(pvar)) {
		encrypted_session_key_len =
			CRYPT_get_encrypted_session_key_len(pvar);
	}

	if (!CRYPT_choose_ciphers(pvar))
		return;

	if (SSHv1(pvar)) {
		outmsg =
			begin_send_packet(pvar, SSH_CMSG_SESSION_KEY,
			                  15 + encrypted_session_key_len);
		outmsg[0] = (unsigned char) CRYPT_get_sender_cipher(pvar);
		memcpy(outmsg + 1, CRYPT_get_server_cookie(pvar), 8);	/* antispoofing cookie */
		outmsg[9] = (unsigned char) (encrypted_session_key_len >> 5);
		outmsg[10] = (unsigned char) (encrypted_session_key_len << 3);
		if (!CRYPT_choose_session_key(pvar, outmsg + 11))
			return;
		set_uint32(outmsg + 11 + encrypted_session_key_len,
		           SSH_PROTOFLAG_SCREEN_NUMBER |
		           SSH_PROTOFLAG_HOST_IN_FWD_OPEN);
		finish_send_packet(pvar);
	}

	if (!CRYPT_start_encryption(pvar, 1, 1))
		return;
	notify_established_secure_connection(pvar);

	if (SSHv1(pvar)) {
		enque_handler(pvar, SSH_SMSG_SUCCESS, handle_crypt_success);
	}

	pvar->ssh_state.status_flags &= ~STATUS_DONT_SEND_USER_NAME;

	if (SSHv1(pvar)) {
		try_send_user_name(pvar);
	}
}

/*************************
   END of message handlers
   ************************/

void SSH_init(PTInstVar pvar)
{
	int i;

	buf_create(&pvar->ssh_state.outbuf, &pvar->ssh_state.outbuflen);
	buf_create(&pvar->ssh_state.precompress_outbuf,
	           &pvar->ssh_state.precompress_outbuflen);
	buf_create(&pvar->ssh_state.postdecompress_inbuf,
	           &pvar->ssh_state.postdecompress_inbuflen);
	pvar->ssh_state.payload = NULL;
	pvar->ssh_state.compressing = FALSE;
	pvar->ssh_state.decompressing = FALSE;
	pvar->ssh_state.status_flags =
		STATUS_DONT_SEND_USER_NAME | STATUS_DONT_SEND_CREDENTIALS;
	pvar->ssh_state.payload_datalen = 0;
	pvar->ssh_state.hostname = NULL;
	pvar->ssh_state.server_ID = NULL;
	pvar->ssh_state.receiver_sequence_number = 0;
	pvar->ssh_state.sender_sequence_number = 0;
	for (i = 0; i < NUM_ELEM(pvar->ssh_state.packet_handlers); i++) {
		pvar->ssh_state.packet_handlers[i] = NULL;
	}

	// for SSH2(yutaka)
	memset(pvar->ssh2_keys, 0, sizeof(pvar->ssh2_keys));
	pvar->userauth_success = 0;
	pvar->session_nego_status = 0;
	pvar->settings.ssh_protocol_version = 2;  // SSH2(default)
	pvar->rekeying = 0;
	pvar->key_done = 0;
	pvar->ssh2_autologin = 0;  // autologin disabled(default)
	pvar->ask4passwd = 0; // disabled(default) (2006.9.18 maya)
	pvar->userauth_retry_count = 0;
	pvar->decomp_buffer = NULL;
	pvar->ssh2_authlist = NULL; // (2007.4.27 yutaka)
	pvar->tryed_ssh2_authlist = FALSE;
	pvar->agentfwd_enable = FALSE;

}

void SSH_open(PTInstVar pvar)
{
	pvar->ssh_state.hostname = _strdup(pvar->ts->HostName);
	pvar->ssh_state.tcpport  = pvar->ts->TCPPort;
	pvar->ssh_state.win_cols = pvar->ts->TerminalWidth;
	pvar->ssh_state.win_rows = pvar->ts->TerminalHeight;
}

void SSH_notify_disconnecting(PTInstVar pvar, char *reason)
{
	if (SSHv1(pvar)) {
		int len = reason == NULL ? 0 : strlen(reason);
		unsigned char *outmsg =
			begin_send_packet(pvar, SSH_MSG_DISCONNECT, len + 4);

		set_uint32(outmsg, len);
		if (reason != NULL) {
			memcpy(outmsg + 4, reason, len);
		}
		finish_send_packet(pvar);

	} else { // for SSH2(yutaka)
		buffer_t *msg;
		unsigned char *outmsg;
		char *s;
		int len;

		// SSH2 serverにdisconnectを伝える
		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			return;
		}
		buffer_put_int(msg, SSH2_DISCONNECT_BY_APPLICATION);
		buffer_put_string(msg, reason, strlen(reason));
		s = "";
		buffer_put_string(msg, s, strlen(s));

		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_DISCONNECT, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_DISCONNECT was sent at SSH_notify_disconnecting().");
	}
}

void SSH_notify_host_OK(PTInstVar pvar)
{
	if ((pvar->ssh_state.status_flags & STATUS_HOST_OK) == 0) {
		pvar->ssh_state.status_flags |= STATUS_HOST_OK;
		send_session_key(pvar);
	}
}

static void get_window_pixel_size(PTInstVar pvar, int *x, int *y)
{
	RECT r;

	if (pvar->cv->HWin && GetWindowRect(pvar->cv->HWin, &r)) {
		*x = r.right - r.left;
		*y = r.bottom - r.top;
	}
	else {
		*x = 0;
		*y = 0;
	}

	return;
}

void SSH_notify_win_size(PTInstVar pvar, int cols, int rows)
{
	int x, y;

	pvar->ssh_state.win_cols = cols;
	pvar->ssh_state.win_rows = rows;

	get_window_pixel_size(pvar, &x, &y);

	if (SSHv1(pvar)) {
		if (get_handler(pvar, SSH_SMSG_STDOUT_DATA) == handle_data) {
			unsigned char *outmsg =
				begin_send_packet(pvar, SSH_CMSG_WINDOW_SIZE, 16);

			set_uint32(outmsg, rows);     // window height (characters)
			set_uint32(outmsg + 4, cols); // window width  (characters)
			set_uint32(outmsg + 8, x);    // window width  (pixels)
			set_uint32(outmsg + 12, y);   // window height (pixels)
			finish_send_packet(pvar);
			logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": sending SSH_CMSG_WINDOW_SIZE. "
			          "cols: %d, rows: %d, x: %d, y: %d", cols, rows, x, y);
		}

	} else if (SSHv2(pvar)) {
		// ターミナルサイズ変更通知の追加 (2005.1.4 yutaka)
		// SSH2かどうかのチェックも行う。(2005.1.5 yutaka)
		buffer_t *msg;
		char *req_type = "window-change";
		unsigned char *outmsg;
		int len;
		Channel_t *c;

		c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": shell channel not found.");
			return;
		}


		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			return;
		}
		buffer_put_int(msg, c->remote_id);
		buffer_put_string(msg, req_type, strlen(req_type));
		buffer_put_char(msg, 0);    // want_reply
		buffer_put_int(msg, cols);  // columns
		buffer_put_int(msg, rows);  // lines
		buffer_put_int(msg, x);     // window width (pixel):
		buffer_put_int(msg, y);     // window height (pixel):
		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_REQUEST, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": sending SSH2_MSG_CHANNEL_REQUEST. "
		          "local: %d, remote: %d, request-type: %s, cols: %d, rows: %d, x: %d, y: %d",
		          c->self_id, c->remote_id, req_type, cols, rows, x, y);

	} else {
		// SSHでない場合は何もしない。
	}
}

// ブレーク信号を送る -- RFC 4335
// OpenSSH の"~B"に相当する。
// (2010.9.27 yutaka)
int SSH_notify_break_signal(PTInstVar pvar)
{
	int ret = 0;

	if (SSHv2(pvar)) { // SSH2 のみ対応
		buffer_t *msg;
		char *req_type = "break";
		unsigned char *outmsg;
		int len;
		Channel_t *c;

		c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": shell channel not found.");
			goto error;
		}

		msg = buffer_init();
		if (msg == NULL) {
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			goto error;
		}
		buffer_put_int(msg, c->remote_id);
		buffer_put_string(msg, req_type, strlen(req_type));
		buffer_put_char(msg, 0);  // want_reply
		buffer_put_int(msg, 1000);  // break-length (msec)
		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_REQUEST, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": sending SSH2_MSG_CHANNEL_REQUEST. "
		          "local: %d, remote: %d, request-type: %s, break-length: %d",
		          c->self_id, c->remote_id, req_type, 1000);

		ret = 1;
	}

error:
	return (ret);
}

int SSH_get_min_packet_size(PTInstVar pvar)
{
	if (SSHv1(pvar)) {
		return 12;
	} else {
		int block_size = CRYPT_get_decryption_block_size(pvar);

		return max(16, block_size);
	}
}

/* data is guaranteed to be at least SSH_get_min_packet_size bytes long
   at least 5 bytes must be decrypted */
void SSH_predecrpyt_packet(PTInstVar pvar, char *data)
{
	if (SSHv2(pvar)) {
		CRYPT_decrypt(pvar, data, get_predecryption_amount(pvar));
	}
}

int SSH_get_clear_MAC_size(PTInstVar pvar)
{
	if (SSHv1(pvar)) {
		return 0;
	} else {
		return CRYPT_get_receiver_MAC_size(pvar);
	}
}

void SSH_notify_user_name(PTInstVar pvar)
{
	try_send_user_name(pvar);
}

void SSH_notify_cred(PTInstVar pvar)
{
	try_send_credentials(pvar);
}

void SSH_send(PTInstVar pvar, unsigned char const *buf, unsigned int buflen)
{
	// RAWパケットダンプを追加 (2008.8.15 yutaka)
	if (LogLevel(pvar, LOG_LEVEL_SSHDUMP)) {
		init_memdump();
		push_memdump("SSH sending packet", "SSH_send", (char *)buf, buflen);
	}

	if (SSHv1(pvar)) {
		if (get_handler(pvar, SSH_SMSG_STDOUT_DATA) != handle_data) {
			return;
		}

		while (buflen > 0) {
			int len =
				buflen >
				SSH_MAX_SEND_PACKET_SIZE ? SSH_MAX_SEND_PACKET_SIZE : buflen;
			unsigned char *outmsg =
				begin_send_packet(pvar, SSH_CMSG_STDIN_DATA, 4 + len);

			set_uint32(outmsg, len);

			if (pvar->ssh_state.compressing) {
				buf_ensure_size(&pvar->ssh_state.outbuf,
				                &pvar->ssh_state.outbuflen,
				                len + (len >> 6) + 50);
				pvar->ssh_state.compress_stream.next_in =
					pvar->ssh_state.precompress_outbuf;
				pvar->ssh_state.compress_stream.avail_in = 5;
				pvar->ssh_state.compress_stream.next_out =
					pvar->ssh_state.outbuf + 12;
				pvar->ssh_state.compress_stream.avail_out =
					pvar->ssh_state.outbuflen - 12;

				if (deflate(&pvar->ssh_state.compress_stream, Z_NO_FLUSH) != Z_OK) {
					UTIL_get_lang_msg("MSG_SSH_COMP_ERROR", pvar,
									  "Error compressing packet data");
					notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
					return;
				}

				pvar->ssh_state.compress_stream.next_in =
					(unsigned char *) buf;
				pvar->ssh_state.compress_stream.avail_in = len;

				if (deflate(&pvar->ssh_state.compress_stream, Z_SYNC_FLUSH) != Z_OK) {
					UTIL_get_lang_msg("MSG_SSH_COMP_ERROR", pvar,
					                  "Error compressing packet data");
					notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
					return;
				}
			} else {
				memcpy(outmsg + 4, buf, len);
			}

			finish_send_packet_special(pvar, 1);

			buflen -= len;
			buf += len;
		}

	} else { // for SSH2(yutaka)
		Channel_t *c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": shell channel not found.");
		}
		else {
			SSH2_send_channel_data(pvar, c, (unsigned char *)buf, buflen, 0);
		}
	}

}

int SSH_extract_payload(PTInstVar pvar, unsigned char *dest, int len)
{
	int num_bytes = pvar->ssh_state.payload_datalen;

	if (num_bytes > len) {
		num_bytes = len;
	}

	if (!pvar->ssh_state.decompressing) {
		memcpy(dest,
		       pvar->ssh_state.payload + pvar->ssh_state.payload_datastart,
		       num_bytes);
		pvar->ssh_state.payload_datastart += num_bytes;
	} else if (num_bytes > 0) {
		pvar->ssh_state.decompress_stream.next_out = dest;
		pvar->ssh_state.decompress_stream.avail_out = num_bytes;

		if (inflate(&pvar->ssh_state.decompress_stream, Z_SYNC_FLUSH) != Z_OK) {
			UTIL_get_lang_msg("MSG_SSH_INVALID_COMPDATA_ERROR", pvar,
			                  "Invalid compressed data in received packet");
			notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
			return 0;
		}
	}

	pvar->ssh_state.payload_datalen -= num_bytes;

	return num_bytes;
}

void SSH_get_compression_info(PTInstVar pvar, char *dest, int len)
{
	char buf[1024];
	char buf2[1024];

	// added support of SSH2 packet compression (2005.7.10 yutaka)
	// support of "Compression delayed" (2006.6.23 maya)
	if (pvar->ssh_state.compressing ||
		pvar->ctos_compression == COMP_ZLIB ||
		pvar->ctos_compression == COMP_DELAYED && pvar->userauth_success) {
		unsigned long total_in = pvar->ssh_state.compress_stream.total_in;
		unsigned long total_out =
			pvar->ssh_state.compress_stream.total_out;

		if (total_out > 0) {
			UTIL_get_lang_msg("DLG_ABOUT_COMP_INFO", pvar,
			                  "level %d; ratio %.1f (%ld:%ld)");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg,
			            pvar->ssh_state.compression_level,
			            ((double) total_in) / total_out, total_in,
			            total_out);
		} else {
			UTIL_get_lang_msg("DLG_ABOUT_COMP_INFO2", pvar, "level %d");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg,
			            pvar->ssh_state.compression_level);
		}
	} else {
		UTIL_get_lang_msg("DLG_ABOUT_COMP_NONE", pvar, "none");
		strncpy_s(buf, sizeof(buf), pvar->ts->UIMsg, _TRUNCATE);
	}

	// support of "Compression delayed" (2006.6.23 maya)
	if (pvar->ssh_state.decompressing ||
		pvar->stoc_compression == COMP_ZLIB ||
		pvar->stoc_compression == COMP_DELAYED && pvar->userauth_success) {
		unsigned long total_in =
			pvar->ssh_state.decompress_stream.total_in;
		unsigned long total_out =
			pvar->ssh_state.decompress_stream.total_out;

		if (total_in > 0) {
			UTIL_get_lang_msg("DLG_ABOUT_COMP_INFO", pvar,
			                  "level %d; ratio %.1f (%ld:%ld)");
			_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, pvar->ts->UIMsg,
			            pvar->ssh_state.compression_level,
			            ((double) total_out) / total_in, total_out,
			            total_in);
		} else {
			UTIL_get_lang_msg("DLG_ABOUT_COMP_INFO2", pvar, "level %d");
			_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, pvar->ts->UIMsg,
			            pvar->ssh_state.compression_level);
		}
	} else {
		UTIL_get_lang_msg("DLG_ABOUT_COMP_NONE", pvar, "none");
		strncpy_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
	}

	UTIL_get_lang_msg("DLG_ABOUT_COMP_UPDOWN", pvar,
	                  "Upstream %s; Downstream %s");
	_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg, buf, buf2);
}

void SSH_get_server_ID_info(PTInstVar pvar, char *dest, int len)
{
	strncpy_s(dest, len,
	          pvar->ssh_state.server_ID == NULL ? "Unknown"
	                                            : pvar->ssh_state.server_ID,
	          _TRUNCATE);
}

void SSH_get_protocol_version_info(PTInstVar pvar, char *dest,
                                   int len)
{
	if (pvar->protocol_major == 0) {
		strncpy_s(dest, len, "Unknown", _TRUNCATE);
	} else {
		_snprintf_s(dest, len, _TRUNCATE, "%d.%d", pvar->protocol_major,
		            pvar->protocol_minor);
	}
}

void SSH_get_mac_info(PTInstVar pvar, char *dest, int len)
{
	UTIL_get_lang_msg("DLG_ABOUT_MAC_INFO", pvar,
	                  "%s to server, %s from server");
	_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg,
	            get_ssh2_mac_name(pvar->ctos_hmac),
	            get_ssh2_mac_name(pvar->stoc_hmac));
}

void SSH_end(PTInstVar pvar)
{
	int i;
	int mode;

	for (i = 0; i < 256; i++) {
		SSHPacketHandlerItem *first_item =
			pvar->ssh_state.packet_handlers[i];

		if (first_item != NULL) {
			SSHPacketHandlerItem *item = first_item;

			do {
				SSHPacketHandlerItem *cur_item = item;

				item = item->next_for_message;
				free(cur_item);
			} while (item != first_item);
		}
		pvar->ssh_state.packet_handlers[i] = NULL;
	}

	free(pvar->ssh_state.hostname);
	pvar->ssh_state.hostname = NULL;
	free(pvar->ssh_state.server_ID);
	pvar->ssh_state.server_ID = NULL;
	buf_destroy(&pvar->ssh_state.outbuf, &pvar->ssh_state.outbuflen);
	buf_destroy(&pvar->ssh_state.precompress_outbuf,
	            &pvar->ssh_state.precompress_outbuflen);
	buf_destroy(&pvar->ssh_state.postdecompress_inbuf,
	            &pvar->ssh_state.postdecompress_inbuflen);
	pvar->agentfwd_enable = FALSE;

	// support of "Compression delayed" (2006.6.23 maya)
	if (pvar->ssh_state.compressing ||
		pvar->ctos_compression == COMP_ZLIB || // add SSH2 flag (2005.7.10 yutaka)
		pvar->ctos_compression == COMP_DELAYED && pvar->userauth_success) {
		deflateEnd(&pvar->ssh_state.compress_stream);
		pvar->ssh_state.compressing = FALSE;
	}
	// support of "Compression delayed" (2006.6.23 maya)
	if (pvar->ssh_state.decompressing ||
		pvar->stoc_compression == COMP_ZLIB || // add SSH2 flag (2005.7.10 yutaka)
		pvar->stoc_compression == COMP_DELAYED && pvar->userauth_success) {
		inflateEnd(&pvar->ssh_state.decompress_stream);
		pvar->ssh_state.decompressing = FALSE;
	}

#if 1
	// SSH2のデータを解放する (2004.12.27 yutaka)
	if (SSHv2(pvar)) {
		if (pvar->kexdh) {
			DH_free(pvar->kexdh);
			pvar->kexdh = NULL;
		}
		if (pvar->ecdh_client_key) {
			EC_KEY_free(pvar->ecdh_client_key);
			pvar->ecdh_client_key = NULL;
		}
		memset(pvar->server_version_string, 0, sizeof(pvar->server_version_string));
		memset(pvar->client_version_string, 0, sizeof(pvar->client_version_string));

		if (pvar->my_kex != NULL) {
			buffer_free(pvar->my_kex);
			pvar->my_kex = NULL;
		}
		if (pvar->peer_kex != NULL) {
			buffer_free(pvar->peer_kex);
			pvar->peer_kex = NULL;
		}

		pvar->we_need = 0;
		pvar->key_done = 0;
		pvar->rekeying = 0;

		if (pvar->session_id != NULL) {
			free(pvar->session_id);
			pvar->session_id = NULL;
		}
		pvar->session_id_len = 0;

		pvar->userauth_success = 0;
		//pvar->remote_id = 0;
		pvar->shell_id = 0;
		pvar->session_nego_status = 0;

		pvar->ssh_heartbeat_tick = 0;

		if (pvar->decomp_buffer != NULL) {
			buffer_free(pvar->decomp_buffer);
			pvar->decomp_buffer = NULL;
		}

		if (pvar->ssh2_authlist != NULL) { // (2007.4.27 yutaka)
			free(pvar->ssh2_authlist);
			pvar->ssh2_authlist = NULL;
		}

		pvar->tryed_ssh2_authlist = FALSE;

		// add (2008.3.2 yutaka)
		for (mode = 0 ; mode < MODE_MAX ; mode++) {
			if (pvar->ssh2_keys[mode].enc.iv != NULL) {
				free(pvar->ssh2_keys[mode].enc.iv);
				pvar->ssh2_keys[mode].enc.iv = NULL;
			}
			if (pvar->ssh2_keys[mode].enc.key != NULL) {
				free(pvar->ssh2_keys[mode].enc.key);
				pvar->ssh2_keys[mode].enc.key = NULL;
			}
			if (pvar->ssh2_keys[mode].mac.key != NULL) {
				free(pvar->ssh2_keys[mode].mac.key);
				pvar->ssh2_keys[mode].mac.key = NULL;
			}
		}
	}
#endif

}

void SSH2_send_channel_data(PTInstVar pvar, Channel_t *c, unsigned char *buf, unsigned int buflen, int retry)
{
	buffer_t *msg;
	unsigned char *outmsg;
	unsigned int len;

	// SSH2鍵交換中の場合、パケットを捨てる。(2005.6.19 yutaka)
	if (pvar->rekeying) {
		// TODO: 理想としてはパケット破棄ではなく、パケット読み取り遅延にしたいところだが、
		// 将来直すことにする。
		logputs(LOG_LEVEL_INFO, __FUNCTION__ ": now rekeying. data is not sent.");

		c = NULL;

		return;
	}

	if (c == NULL)
		return;

	// リトライではない、通常のパケット送信の際、以前送れなかったデータが
	// リンクドリストに残っているようであれば、リストの末尾に繋ぐ。
	// これによりパケットが壊れたように見える現象が改善される。
	// (2012.10.14 yutaka)
	if (retry == 0 && c->bufchain) {
		ssh2_channel_add_bufchain(c, buf, buflen);
		return;
	}

	if ((unsigned int)buflen > c->remote_window) {
		unsigned int offset = 0;
		// 送れないデータはいったん保存しておく
		ssh2_channel_add_bufchain(c, buf + offset, buflen - offset);
		buflen = offset;
		return;
	}
	if (buflen > 0) {
		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			return;
		}
		buffer_put_int(msg, c->remote_id);
		buffer_put_string(msg, (char *)buf, buflen);

		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_DATA, len);
		//if (len + 12 >= pvar->ssh_state.outbuflen) *(int *)0 = 0;
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		logprintf(LOG_LEVEL_SSHDUMP, __FUNCTION__ ": sending SSH2_MSG_CHANNEL_DATA. "
			"local:%d remote:%d len:%d", c->self_id, c->remote_id, buflen);

		// remote window sizeの調整
		if (buflen <= c->remote_window) {
			c->remote_window -= buflen;
		}
		else {
			c->remote_window = 0;
		}
	}
}

/* support for port forwarding */
void SSH_channel_send(PTInstVar pvar, int channel_num,
                      uint32 remote_channel_num,
                      unsigned char *buf, int len, int retry)
{
	if (SSHv1(pvar)) {
		unsigned char *outmsg =
			begin_send_packet(pvar, SSH_MSG_CHANNEL_DATA, 8 + len);

		set_uint32(outmsg, remote_channel_num);
		set_uint32(outmsg + 4, len);

		if (pvar->ssh_state.compressing) {
			buf_ensure_size(&pvar->ssh_state.outbuf,
			                &pvar->ssh_state.outbuflen, len + (len >> 6) + 50);
			pvar->ssh_state.compress_stream.next_in =
				pvar->ssh_state.precompress_outbuf;
			pvar->ssh_state.compress_stream.avail_in = 9;
			pvar->ssh_state.compress_stream.next_out =
				pvar->ssh_state.outbuf + 12;
			pvar->ssh_state.compress_stream.avail_out =
				pvar->ssh_state.outbuflen - 12;

			if (deflate(&pvar->ssh_state.compress_stream, Z_NO_FLUSH) != Z_OK) {
				UTIL_get_lang_msg("MSG_SSH_COMP_ERROR", pvar,
				                  "Error compressing packet data");
				notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
				return;
			}

			pvar->ssh_state.compress_stream.next_in =
				(unsigned char *) buf;
			pvar->ssh_state.compress_stream.avail_in = len;

			if (deflate(&pvar->ssh_state.compress_stream, Z_SYNC_FLUSH) !=
				Z_OK) {
				UTIL_get_lang_msg("MSG_SSH_COMP_ERROR", pvar,
				                  "Error compressing packet data");
				notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
				return;
			}
		} else {
			memcpy(outmsg + 8, buf, len);
		}

		finish_send_packet_special(pvar, 1);

	} else {
		// ポートフォワーディングにおいてクライアントからの送信要求を、SSH通信に乗せてサーバまで送り届ける。
		Channel_t *c = ssh2_local_channel_lookup(channel_num);
		SSH2_send_channel_data(pvar, c, buf, len, retry);
	}

}

void SSH_fail_channel_open(PTInstVar pvar, uint32 remote_channel_num)
{
	if (SSHv1(pvar)) {
		unsigned char *outmsg =
			begin_send_packet(pvar, SSH_MSG_CHANNEL_OPEN_FAILURE, 4);

		set_uint32(outmsg, remote_channel_num);
		finish_send_packet(pvar);

	} else { // SSH2 (2005.6.26 yutaka)
		int len;
		Channel_t *c = NULL;
		buffer_t *msg;
		unsigned char *outmsg;

		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			return;
		}
		buffer_put_int(msg, remote_channel_num);
		buffer_put_int(msg, SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED);
		buffer_put_string(msg, "", 0); // description
		buffer_put_string(msg, "", 0); // language tag

		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_OPEN_FAILURE, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_OPEN_FAILURE was sent at SSH_fail_channel_open().");
	}
}

void SSH2_confirm_channel_open(PTInstVar pvar, Channel_t *c)
{
	buffer_t *msg;
	unsigned char *outmsg;
	int len;

	if (c == NULL)
		return;

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
		return;
	}
	buffer_put_int(msg, c->remote_id);
	buffer_put_int(msg, c->self_id);
	buffer_put_int(msg, c->local_window);
	buffer_put_int(msg, c->local_maxpacket);

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_OPEN_CONFIRMATION, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);

	logprintf(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_OPEN_CONFIRMATION was sent at SSH_confirm_channel_open(). local:%d remote:%d", c->self_id, c->remote_id);
}

void SSH_confirm_channel_open(PTInstVar pvar, uint32 remote_channel_num,
							  uint32 local_channel_num)
{
	if (SSHv1(pvar)) {
		unsigned char *outmsg =
			begin_send_packet(pvar, SSH_MSG_CHANNEL_OPEN_CONFIRMATION, 8);

		set_uint32(outmsg, remote_channel_num);
		set_uint32(outmsg + 4, local_channel_num);
		finish_send_packet(pvar);

	} else {
		Channel_t *c;

		// port-forwarding(remote to local)のローカル接続への成功をサーバへ返す。(2005.7.2 yutaka)
		c = ssh2_local_channel_lookup(local_channel_num);
		if (c == NULL) {
			// It is sure to be successful as long as it's not a program bug either.
			return;
		}
		SSH2_confirm_channel_open(pvar, c);
	}
}

void SSH_channel_output_eof(PTInstVar pvar, uint32 remote_channel_num)
{
	if (SSHv1(pvar)){
		unsigned char *outmsg =
			begin_send_packet(pvar, SSH_MSG_CHANNEL_OUTPUT_CLOSED, 4);

		set_uint32(outmsg, remote_channel_num);
		finish_send_packet(pvar);

	} else {
		// SSH2: 特になし。

	}
}

void SSH2_channel_input_eof(PTInstVar pvar, Channel_t *c)
{
	buffer_t *msg;
	unsigned char *outmsg;
	int len;

	if (c == NULL)
		return;

	// SSH2鍵交換中の場合、パケットを捨てる。(2005.6.21 yutaka)
	if (pvar->rekeying) {
		// TODO: 理想としてはパケット破棄ではなく、パケット読み取り遅延にしたいところだが、
		// 将来直すことにする。
		logputs(LOG_LEVEL_INFO, __FUNCTION__ ": now rekeying. data is not sent.");

		c = NULL;

		return;
	}

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
		return;
	}
	buffer_put_int(msg, c->remote_id);  // remote ID

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_EOF, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);

	logprintf(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_EOF was sent at SSH2_channel_input_eof(). local:%d remote:%d", c->self_id, c->remote_id);
}

void SSH_channel_input_eof(PTInstVar pvar, uint32 remote_channel_num, uint32 local_channel_num)
{
	if (SSHv1(pvar)){
		unsigned char *outmsg =
			begin_send_packet(pvar, SSH_MSG_CHANNEL_INPUT_EOF, 4);

		set_uint32(outmsg, remote_channel_num);
		finish_send_packet(pvar);

	} else {
		// SSH2: チャネルクローズをサーバへ通知
		Channel_t *c;

		c = ssh2_local_channel_lookup(local_channel_num);
		if (c == NULL)
			return;
		
		SSH2_channel_input_eof(pvar, c);
	}
}

void SSH_request_forwarding(PTInstVar pvar, char *bind_address, int from_server_port,
                            char *to_local_host, int to_local_port)
{
	if (SSHv1(pvar)) {
		int host_len = strlen(to_local_host);
		unsigned char *outmsg =
			begin_send_packet(pvar, SSH_CMSG_PORT_FORWARD_REQUEST, 12 + host_len);

		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": Forwarding request (SSH1 RtoL): "
			"remote_port=%d, to_host=%s, to_port=%d",
			from_server_port, to_local_host, to_local_port);

		set_uint32(outmsg, from_server_port);
		set_uint32(outmsg + 4, host_len);
		memcpy(outmsg + 8, to_local_host, host_len);
		set_uint32(outmsg + 8 + host_len, to_local_port);
		finish_send_packet(pvar);

		enque_forwarding_request_handlers(pvar);

		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": sending SSH_CMSG_PORT_FORWARD_REQUEST."
			"remote_port=%d, to_host=%s, to_port=%d",
			from_server_port, to_local_host, to_local_port);

	} else {
		// SSH2 port-forwading remote to local (2005.6.21 yutaka)
		buffer_t *msg;
		char *req;
		unsigned char *outmsg;
		int len;

		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": Forwarding request (SSH2 RtoL): "
			"bind_addr=%s, remote_port=%d, to_host=%s, to_port=%d",
			bind_address, from_server_port, to_local_host, to_local_port);

		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			return;
		}
		req = "tcpip-forward";
		buffer_put_string(msg, req, strlen(req)); // ctype
		buffer_put_char(msg, 1);  // want reply
		buffer_put_string(msg, bind_address, strlen(bind_address));

		buffer_put_int(msg, from_server_port);  // listening port

		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_GLOBAL_REQUEST, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": sending SSH2_MSG_GLOBAL_REQUEST. "
			"request=%s, want_reply=%d, bind_address=%s, remote_port=%d",
			req, 1, bind_address, from_server_port);
	}
}

void SSH_cancel_request_forwarding(PTInstVar pvar, char *bind_address, int from_server_port, int reply)
{
	if (SSHv2(pvar)) {
		buffer_t *msg;
		char *s;
		unsigned char *outmsg;
		int len;

		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			return;
		}
		s = "cancel-tcpip-forward";
		buffer_put_string(msg, s, strlen(s)); // ctype
		buffer_put_char(msg, reply);  // want reply
		buffer_put_string(msg, bind_address, strlen(bind_address));

		buffer_put_int(msg, from_server_port);  // listening port

		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_GLOBAL_REQUEST, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_GLOBAL_REQUEST was sent at SSH_cancel_request_forwarding().");
	}
}

void SSH_request_X11_forwarding(PTInstVar pvar,
                                char *auth_protocol,
                                unsigned char *auth_data,
                                int auth_data_len, int screen_num)
{
	if (SSHv1(pvar)) {
		int protocol_len = strlen(auth_protocol);
		int data_len = auth_data_len * 2;
		int outmsg_len = 12 + protocol_len + data_len;
		unsigned char *outmsg =
			begin_send_packet(pvar, SSH_CMSG_X11_REQUEST_FORWARDING, outmsg_len);
		int i;
		char *auth_data_ptr;

		set_uint32(outmsg, protocol_len);
		memcpy(outmsg + 4, auth_protocol, protocol_len);
		set_uint32(outmsg + 4 + protocol_len, data_len);
		auth_data_ptr = outmsg + 8 + protocol_len;
		for (i = 0; i < auth_data_len; i++) {
			_snprintf_s(auth_data_ptr + i * 2,
			            outmsg_len - (auth_data_ptr - outmsg) - i * 2,
			            _TRUNCATE, "%.2x", auth_data[i]);
		}
		set_uint32(outmsg + 8 + protocol_len + data_len, screen_num);

		finish_send_packet(pvar);

		enque_forwarding_request_handlers(pvar);

	} else {
		// SSH2: X11 port-forwarding (2005.7.2 yutaka)
		buffer_t *msg;
		char *req_type = "x11-req";
		unsigned char *outmsg;
		int len;
		Channel_t *c;
		int newlen;
		char *newdata;
		int i;

		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			return;
		}

		c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": shell channel not found.");
			return;
		}

		// making the fake data	
		newlen = 2 * auth_data_len + 1;
		newdata = malloc(newlen);
		if (newdata == NULL)
			return;
		for (i = 0 ; i < auth_data_len ; i++) {
			_snprintf_s(newdata + i*2, newlen - i*2, _TRUNCATE, "%02x", auth_data[i]);
		}
		newdata[newlen - 1] = '\0';

		buffer_put_int(msg, c->remote_id);
		buffer_put_string(msg, req_type, strlen(req_type)); // service name
		buffer_put_char(msg, 0);  // want_reply (false)
		buffer_put_char(msg, 0);  // single connection

		buffer_put_string(msg, auth_protocol, strlen(auth_protocol)); // protocol ("MIT-MAGIC-COOKIE-1")
		buffer_put_string(msg, newdata, strlen(newdata)); // cookie

		buffer_put_int(msg, screen_num);

		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_REQUEST, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": sending SSH2_MSG_CHANNEL_REQUEST. "
		          "local: %d, remote: %d, request-type: %s, proto: %s, cookie: %s, screen: %d",
		          c->self_id, c->remote_id, req_type, auth_protocol, newdata, screen_num);

		free(newdata);
	}

}

void SSH_open_channel(PTInstVar pvar, uint32 local_channel_num,
                      char *to_remote_host, int to_remote_port,
                      char *originator, unsigned short originator_port)
{
	static const int msgs[]
	= { SSH_MSG_CHANNEL_OPEN_CONFIRMATION, SSH_MSG_CHANNEL_OPEN_FAILURE };
	static const SSHPacketHandler handlers[]
	= { handle_channel_open_confirmation, handle_channel_open_failure };

	int host_len = strlen(to_remote_host);

	if ((pvar->ssh_state.
		 server_protocol_flags & SSH_PROTOFLAG_HOST_IN_FWD_OPEN) != 0) {
		int originator_len = strlen(originator);
		unsigned char *outmsg =
			begin_send_packet(pvar, SSH_MSG_PORT_OPEN,
			                  16 + host_len + originator_len);

		set_uint32(outmsg, local_channel_num);
		set_uint32(outmsg + 4, host_len);
		memcpy(outmsg + 8, to_remote_host, host_len);
		set_uint32(outmsg + 8 + host_len, to_remote_port);
		set_uint32(outmsg + 12 + host_len, originator_len);
		memcpy(outmsg + 16 + host_len, originator, originator_len);
	} else {

		if (SSHv1(pvar)) {
			unsigned char *outmsg =
				begin_send_packet(pvar, SSH_MSG_PORT_OPEN,
				                  12 + host_len);

			set_uint32(outmsg, local_channel_num);
			set_uint32(outmsg + 4, host_len);
			memcpy(outmsg + 8, to_remote_host, host_len);
			set_uint32(outmsg + 8 + host_len, to_remote_port);

		} else {
			// SSH2 port-fowarding (2005.2.26 yutaka)
			buffer_t *msg;
			char *s;
			unsigned char *outmsg;
			int len;
			Channel_t *c;

			// SSH2鍵交換中の場合、パケットを捨てる。(2005.6.21 yutaka)
			if (pvar->rekeying) {
				// TODO: 理想としてはパケット破棄ではなく、パケット読み取り遅延にしたいところだが、
				// 将来直すことにする。
				logputs(LOG_LEVEL_INFO, __FUNCTION__ ": now rekeying. channel open request is not sent.");

				c = NULL;

				return;
			}

			// changed window size from 128KB to 32KB. (2006.3.6 yutaka)
			// changed window size from 32KB to 128KB. (2007.10.29 maya)
			c = ssh2_channel_new(CHAN_TCP_WINDOW_DEFAULT, CHAN_TCP_PACKET_DEFAULT, TYPE_PORTFWD, local_channel_num);
			if (c == NULL) {
				// 転送チャネル内にあるソケットの解放漏れを修正 (2007.7.26 maya)
				FWD_free_channel(pvar, local_channel_num);
				UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
				                  "Could not open new channel. TTSSH is already opening too many channels.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
				return;
			}

			msg = buffer_init();
			if (msg == NULL) {
				// TODO: error check
				logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
				return;
			}
			s = "direct-tcpip";
			buffer_put_string(msg, s, strlen(s)); // ctype
			buffer_put_int(msg, c->self_id);  // self
			buffer_put_int(msg, c->local_window);  // local_window
			buffer_put_int(msg, c->local_maxpacket);  // local_maxpacket

			s = to_remote_host;
			buffer_put_string(msg, s, strlen(s)); // target host
			buffer_put_int(msg, to_remote_port);  // target port

			s = originator;
			buffer_put_string(msg, s, strlen(s)); // originator host
			buffer_put_int(msg, originator_port);  // originator port

			len = buffer_len(msg);
			outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_OPEN, len);
			memcpy(outmsg, buffer_ptr(msg), len);
			finish_send_packet(pvar);
			buffer_free(msg);

			logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_OPEN was sent at SSH_open_channel().");

			return;

			/* NOT REACHED */
		}

	}

	if (SSHv1(pvar)) { // SSH1のみ
		finish_send_packet(pvar);
		enque_handlers(pvar, 2, msgs, handlers);
	}

}


//
// SCP support
//
// (2007.12.21 yutaka)
//
int SSH_scp_transaction(PTInstVar pvar, char *sendfile, char *dstfile, enum scp_dir direction)
{
	buffer_t *msg;
	char *s;
	unsigned char *outmsg;
	int len;
	Channel_t *c = NULL;
	FILE *fp = NULL;
	struct __stat64 st;

	// ソケットがクローズされている場合は何もしない。
	if (pvar->socket == INVALID_SOCKET)
		goto error;

	if (SSHv1(pvar))      // SSH1サポートはTBD
		goto error;

	// チャネル設定
	c = ssh2_channel_new(CHAN_SES_WINDOW_DEFAULT, CHAN_SES_PACKET_DEFAULT, TYPE_SCP, -1);
	if (c == NULL) {
		UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
		                  "Could not open new channel. TTSSH is already opening too many channels.");
		notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
		goto error;
	}

	if (direction == TOREMOTE) {  // copy local to remote
		fp = fopen(sendfile, "rb");
		if (fp == NULL) {
			char buf[80];
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "fopen: %d", GetLastError());
			MessageBox(NULL, buf, "TTSSH: file open error", MB_OK | MB_ICONERROR);
			goto error;
		}

		strncpy_s(c->scp.localfilefull, sizeof(c->scp.localfilefull), sendfile, _TRUNCATE);  // full path
		ExtractFileName(sendfile, c->scp.localfile, sizeof(c->scp.localfile));   // file name only
		if (dstfile == NULL || dstfile[0] == '\0') { // remote file path
			strncpy_s(c->scp.remotefile, sizeof(c->scp.remotefile), ".", _TRUNCATE);  // full path
		} else {
			strncpy_s(c->scp.remotefile, sizeof(c->scp.remotefile), dstfile, _TRUNCATE);  // full path
		}
		c->scp.localfp = fp;     // file pointer

		if (_stat64(c->scp.localfilefull, &st) == 0) {
			c->scp.filestat = st;
		} else {
			goto error;
		}
	} else { // copy remote to local
		strncpy_s(c->scp.remotefile, sizeof(c->scp.remotefile), sendfile, _TRUNCATE); 

		if (dstfile == NULL || dstfile[0] == '\0') { // local file path is empty.
			char *fn, *cwd;

			fn = strrchr(sendfile, '/');
			if (fn && fn[1] == '\0')
				goto error;
			cwd = pvar->ts->FileDir;
			//cwd = _getcwd(NULL, 0);

			_snprintf_s(c->scp.localfilefull, sizeof(c->scp.localfilefull), _TRUNCATE, "%s\\%s", cwd, fn ? fn : sendfile);
			ExtractFileName(c->scp.localfilefull, c->scp.localfile, sizeof(c->scp.localfile));   // file name only

			//free(cwd);  // free!!
		} else {
			_snprintf_s(c->scp.localfilefull, sizeof(c->scp.localfilefull), _TRUNCATE, "%s", dstfile);
			ExtractFileName(dstfile, c->scp.localfile, sizeof(c->scp.localfile));   // file name only
		}

		if (_access(c->scp.localfilefull, 0x00) == 0) {
			char buf[512];
			int dlgresult;
			if (_access(c->scp.localfilefull, 0x02) == -1) { // 0x02 == writable
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, "`%s' file is read only.", c->scp.localfilefull);
				MessageBox(NULL, buf, "TTSSH: file open error", MB_OK | MB_ICONERROR);
				goto error;
			}
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "`%s' file exists. (%d)\noverwrite it?", c->scp.localfilefull, GetLastError());
			dlgresult = MessageBox(NULL, buf, "TTSSH: confirm", MB_YESNO | MB_ICONERROR);
			if (dlgresult == IDNO) {
				goto error;
			}
		}

		fp = fopen(c->scp.localfilefull, "wb");
		if (fp == NULL) {
			char buf[512];
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "fopen: %d", GetLastError());
			MessageBox(NULL, buf, "TTSSH: file open write error", MB_OK | MB_ICONERROR);
			goto error;
		}

		c->scp.localfp = fp;     // file pointer
	}

	// setup SCP data
	c->scp.dir = direction;     
	c->scp.state = SCP_INIT;

	// session open
	msg = buffer_init();
	if (msg == NULL) {
		goto error;
	}
	s = "session";
	buffer_put_string(msg, s, strlen(s));  // ctype
	buffer_put_int(msg, c->self_id);  // self(channel number)
	buffer_put_int(msg, c->local_window);  // local_window
	buffer_put_int(msg, c->local_maxpacket);  // local_maxpacket
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_OPEN, len);
	memcpy(outmsg, buffer_ptr (msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_OPEN was sent at SSH_scp_transaction().");

	return TRUE;

error:
	if (c != NULL)
		ssh2_channel_delete(c);
	if (fp != NULL)
		fclose(fp);

	return FALSE;
}

int SSH_start_scp(PTInstVar pvar, char *sendfile, char *dstfile)
{
	return SSH_scp_transaction(pvar, sendfile, dstfile, TOREMOTE);
}

int SSH_start_scp_receive(PTInstVar pvar, char *filename)
{
	return SSH_scp_transaction(pvar, filename, NULL, FROMREMOTE);
}


int SSH_sftp_transaction(PTInstVar pvar)
{
	buffer_t *msg;
	char *s;
	unsigned char *outmsg;
	int len;
	Channel_t *c = NULL;
//	FILE *fp = NULL;
//	struct __stat64 st;

	// ソケットがクローズされている場合は何もしない。
	if (pvar->socket == INVALID_SOCKET)
		goto error;

	if (SSHv1(pvar))      // SSH1サポートはTBD
		goto error;

	// チャネル設定
	c = ssh2_channel_new(CHAN_SES_WINDOW_DEFAULT, CHAN_SES_PACKET_DEFAULT, TYPE_SFTP, -1);
	if (c == NULL) {
		UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
		                  "Could not open new channel. TTSSH is already opening too many channels.");
		notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
		goto error;
	}

	// session open
	msg = buffer_init();
	if (msg == NULL) {
		goto error;
	}
	s = "session";
	buffer_put_string(msg, s, strlen(s));  // ctype
	buffer_put_int(msg, c->self_id);  // self(channel number)
	buffer_put_int(msg, c->local_window);  // local_window
	buffer_put_int(msg, c->local_maxpacket);  // local_maxpacket
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_OPEN, len);
	memcpy(outmsg, buffer_ptr (msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_OPEN was sent at SSH_sftp_transaction().");

	return TRUE;

error:
	if (c != NULL)
		ssh2_channel_delete(c);

	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
// SSH2 protocol procedure in the following code:
//
/////////////////////////////////////////////////////////////////////////////

void debug_print(int no, char *msg, int len)
{
#ifdef _DEBUG
	FILE *fp;
	char file[128];

	_snprintf_s(file, sizeof(file), _TRUNCATE, "dump%d.bin", no);

	fp = fopen(file, "wb");
	if (fp == NULL)
		return;

	fwrite(msg, 1, len, fp);

	fclose(fp);
#endif
}

Newkeys current_keys[MODE_MAX];


#define write_buffer_file(buf,len) do_write_buffer_file(buf,len,__FILE__,__LINE__)


//
// general
//

int get_cipher_block_size(SSHCipher cipher)
{
	ssh2_cipher_t *ptr = ssh2_ciphers;
	int val = 8;

	while (ptr->name != NULL) {
		if (cipher == ptr->cipher) {
			val = ptr->block_size;
			break;
		}
		ptr++;
	}
	return (val);
}

int get_cipher_key_len(SSHCipher cipher)
{
	ssh2_cipher_t *ptr = ssh2_ciphers;
	int val = 0;

	while (ptr->name != NULL) {
		if (cipher == ptr->cipher) {
			val = ptr->key_len;
			break;
		}
		ptr++;
	}
	return (val);
}

int get_cipher_discard_len(SSHCipher cipher)
{
	ssh2_cipher_t *ptr = ssh2_ciphers;
	int val = 0;

	while (ptr->name != NULL) {
		if (cipher == ptr->cipher) {
			val = ptr->discard_len;
			break;
		}
		ptr++;
	}
	return (val);
}

// 暗号アルゴリズム名から検索する。
SSHCipher get_cipher_by_name(char *name)
{
	ssh2_cipher_t *ptr = ssh2_ciphers;
	SSHCipher ret = SSH_CIPHER_NONE;

	if (name == NULL)
		goto error;

	while (ptr->name != NULL) {
		if (strcmp(ptr->name, name) == 0) {
			ret = ptr->cipher;
			break;
		}
		ptr++;
	}
error:
	return (ret);
}

static char * get_cipher_string(SSHCipher cipher)
{
	ssh2_cipher_t *ptr = ssh2_ciphers;
	char *p = "unknown";

	while (ptr->name != NULL) {
		if (cipher == ptr->cipher) {
			p = ptr->name;
			break;
		}
		ptr++;
	}
	return p;
}

const EVP_CIPHER* get_cipher_EVP_CIPHER(SSHCipher cipher)
{
	ssh2_cipher_t *ptr = ssh2_ciphers;
	const EVP_CIPHER *type;

	type = EVP_enc_null();

	while (ptr->name != NULL) {
		if (cipher == ptr->cipher) {
			type = ptr->func();
			break;
		}
		ptr++;
	}
	return type;
}

char* get_kex_algorithm_name(kex_algorithm kextype)
{
	ssh2_kex_algorithm_t *ptr = ssh2_kex_algorithms;
	char *p = "unknown";

	while (ptr->name != NULL) {
		if (kextype == ptr->kextype) {
			p = ptr->name;
			break;
		}
		ptr++;
	}
	return p;
}

const EVP_MD* get_kex_algorithm_EVP_MD(kex_algorithm kextype)
{
	ssh2_kex_algorithm_t *ptr = ssh2_kex_algorithms;
	const EVP_MD *evp_md;

	while (ptr->name != NULL) {
		if (kextype == ptr->kextype) {
			evp_md = ptr->evp_md();
			break;
		}
		ptr++;
	}
	return evp_md;
}

char* get_ssh2_mac_name(hmac_type type)
{
	ssh2_mac_t *ptr = ssh2_macs;
	char *p = "unknown";

	while (ptr->name != NULL) {
		if (type == ptr->type) {
			p = ptr->name;
			break;
		}
		ptr++;
	}
	return p;
}

const EVP_MD* get_ssh2_mac_EVP_MD(hmac_type type)
{
	ssh2_mac_t *ptr = ssh2_macs;
	const EVP_MD *evp_md;

	while (ptr->name != NULL) {
		if (type == ptr->type) {
			evp_md = ptr->evp_md();
			break;
		}
		ptr++;
	}
	return evp_md;
}

int get_ssh2_mac_truncatebits(hmac_type type)
{
	ssh2_mac_t *ptr = ssh2_macs;
	int bits;

	while (ptr->name != NULL) {
		if (type == ptr->type) {
			bits = ptr->truncatebits;
			break;
		}
		ptr++;
	}
	return bits;
}

char* get_ssh2_comp_name(compression_type type)
{
	ssh2_comp_t *ptr = ssh2_comps;
	char *p = "unknown";

	while (ptr->name != NULL) {
		if (type == ptr->type) {
			p = ptr->name;
			break;
		}
		ptr++;
	}
	return p;
}

char* get_ssh_keytype_name(ssh_keytype type)
{
	ssh2_host_key_t *ptr = ssh2_host_key;
	char *p = "ssh-unknown";

	while (ptr->name != NULL) {
		if (type == ptr->type) {
			// ssh2_host_key[]はグローバル変数なので、そのまま返り値にできる。
			p = ptr->name;
			break;
		}
		ptr++;
	}
	return p;
}

char* get_digest_algorithm_name(digest_algorithm id)
{
	ssh_digest_t *ptr = ssh_digests;
	char *p = "unknown";

	while (ptr->name != NULL) {
		if (id == ptr->id) {
			p = ptr->name;
			break;
		}
		ptr++;
	}
	return p;
}

static void do_write_buffer_file(void *buf, int len, char *file, int lineno)
{
	FILE *fp;
	char filename[256];

	_snprintf_s(filename, sizeof(filename), _TRUNCATE, "data%d.bin", lineno);

	fp = fopen(filename, "wb");
	if (fp == NULL)
		return;

	fwrite(buf, 1, len, fp);

	fclose(fp);
}


void SSH2_packet_start(buffer_t *msg, unsigned char type)
{
	unsigned char buf[9];
	int len = 6;

	memset(buf, 0, sizeof(buf));
	buf[len - 1] = type;
	buffer_clear(msg);
	buffer_append(msg, buf, len);
}


// the caller is normalize_cipher_order()
void SSH2_update_cipher_myproposal(PTInstVar pvar)
{
	static char buf[512]; // TODO: malloc()にすべき
	int cipher;
	int len, i;
	char *c_str;

	// 通信中には呼ばれないはずだが、念のため。(2006.6.26 maya)
	if (pvar->socket != INVALID_SOCKET) {
		return;
	}

	// 暗号アルゴリズム優先順位に応じて、myproposal[]を書き換える。(2004.11.6 yutaka)
	buf[0] = '\0';
	for (i = 0 ; pvar->settings.CipherOrder[i] != 0 ; i++) {
		cipher = pvar->settings.CipherOrder[i] - '0';
		if (cipher == 0) // disabled line
			break;
		switch (cipher) {
			case SSH2_CIPHER_3DES_CBC:
				c_str = "3des-cbc,";
				break;
			case SSH2_CIPHER_3DES_CTR:
				c_str = "3des-ctr,";
				break;
			case SSH2_CIPHER_BLOWFISH_CBC:
				c_str = "blowfish-cbc,";
				break;
			case SSH2_CIPHER_BLOWFISH_CTR:
				c_str = "blowfish-ctr,";
				break;
			case SSH2_CIPHER_AES128_CBC:
				c_str = "aes128-cbc,";
				break;
			case SSH2_CIPHER_AES192_CBC:
				c_str = "aes192-cbc,";
				break;
			case SSH2_CIPHER_AES256_CBC:
				c_str = "aes256-cbc,";
				break;
			case SSH2_CIPHER_AES128_CTR:
				c_str = "aes128-ctr,";
				break;
			case SSH2_CIPHER_AES192_CTR:
				c_str = "aes192-ctr,";
				break;
			case SSH2_CIPHER_AES256_CTR:
				c_str = "aes256-ctr,";
				break;
			case SSH2_CIPHER_ARCFOUR:
				c_str = "arcfour,";
				break;
			case SSH2_CIPHER_ARCFOUR128:
				c_str = "arcfour128,";
				break;
			case SSH2_CIPHER_ARCFOUR256:
				c_str = "arcfour256,";
				break;
			case SSH2_CIPHER_CAST128_CBC:
				c_str = "cast128-cbc,";
				break;
			case SSH2_CIPHER_CAST128_CTR:
				c_str = "cast128-ctr,";
				break;
#ifdef WITH_CAMELLIA_PRIVATE
			case SSH2_CIPHER_CAMELLIA128_CBC:
				c_str = "camellia128-cbc,camellia128-cbc@openssh.org,";
				break;
			case SSH2_CIPHER_CAMELLIA192_CBC:
				c_str = "camellia192-cbc,camellia192-cbc@openssh.org,";
				break;
			case SSH2_CIPHER_CAMELLIA256_CBC:
				c_str = "camellia256-cbc,camellia256-cbc@openssh.org,";
				break;
			case SSH2_CIPHER_CAMELLIA128_CTR:
				c_str = "camellia128-ctr,camellia128-ctr@openssh.org,";
				break;
			case SSH2_CIPHER_CAMELLIA192_CTR:
				c_str = "camellia192-ctr,camellia192-ctr@openssh.org,";
				break;
			case SSH2_CIPHER_CAMELLIA256_CTR:
				c_str = "camellia256-ctr,camellia256-ctr@openssh.org,";
				break;
#endif // WITH_CAMELLIA_PRIVATE
			case SSH2_CIPHER_CAMELLIA128_CBC:
				c_str = "camellia128-cbc,";
				break;
			case SSH2_CIPHER_CAMELLIA192_CBC:
				c_str = "camellia192-cbc,";
				break;
			case SSH2_CIPHER_CAMELLIA256_CBC:
				c_str = "camellia256-cbc,";
				break;
			case SSH2_CIPHER_CAMELLIA128_CTR:
				c_str = "camellia128-ctr,";
				break;
			case SSH2_CIPHER_CAMELLIA192_CTR:
				c_str = "camellia192-ctr,";
				break;
			case SSH2_CIPHER_CAMELLIA256_CTR:
				c_str = "camellia256-ctr,";
				break;
			default:
				continue;
		}
		strncat_s(buf, sizeof(buf), c_str, _TRUNCATE);
	}
	len = strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';  // get rid of comma
	myproposal[PROPOSAL_ENC_ALGS_CTOS] = buf;  // Client To Server
	myproposal[PROPOSAL_ENC_ALGS_STOC] = buf;  // Server To Client
}


void SSH2_update_compression_myproposal(PTInstVar pvar)
{
	static char buf[128]; // TODO: malloc()にすべき
	int index;
	int len, i;

	// 通信中には呼ばれないはずだが、念のため。(2006.6.26 maya)
	if (pvar->socket != INVALID_SOCKET) {
		return;
	}

	// 圧縮レベルに応じて、myproposal[]を書き換える。(2005.7.9 yutaka)
	buf[0] = '\0';
	for (i = 0 ; pvar->settings.CompOrder[i] != 0 ; i++) {
		index = pvar->settings.CompOrder[i] - '0';
		if (index == COMP_NONE) // disabled line
			break;
		strncat_s(buf, sizeof(buf), get_ssh2_comp_name(index), _TRUNCATE);
		strncat_s(buf, sizeof(buf), ",", _TRUNCATE);
	}
	len = strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';  // get rid of comma

	// 圧縮指定がない場合は、圧縮レベルを無条件にゼロにする。
	if (buf[0] == '\0') {
		pvar->settings.CompressionLevel = 0;
	}

	if (pvar->settings.CompressionLevel == 0) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, get_ssh2_comp_name(COMP_NOCOMP));
	}
	if (buf[0] != '\0') {
		myproposal[PROPOSAL_COMP_ALGS_CTOS] = buf;  // Client To Server
		myproposal[PROPOSAL_COMP_ALGS_STOC] = buf;  // Server To Client
	}
}

// KEXアルゴリズム優先順位に応じて、myproposal[]を書き換える。
// (2011.2.28 yutaka)
void SSH2_update_kex_myproposal(PTInstVar pvar)
{
	static char buf[512]; // TODO: malloc()にすべき
	int index;
	int len, i;

	// 通信中には呼ばれないはずだが、念のため。(2006.6.26 maya)
	if (pvar->socket != INVALID_SOCKET) {
		return;
	}

	buf[0] = '\0';
	for (i = 0 ; pvar->settings.KexOrder[i] != 0 ; i++) {
		index = pvar->settings.KexOrder[i] - '0';
		if (index == KEX_DH_NONE) // disabled line
			break;
		strncat_s(buf, sizeof(buf), get_kex_algorithm_name(index), _TRUNCATE);
		strncat_s(buf, sizeof(buf), ",", _TRUNCATE);
	}
	len = strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';  // get rid of comma
	myproposal[PROPOSAL_KEX_ALGS] = buf; 
}

// Host Keyアルゴリズム優先順位に応じて、myproposal[]を書き換える。
// (2011.2.28 yutaka)
void SSH2_update_host_key_myproposal(PTInstVar pvar)
{
	static char buf[256]; // TODO: malloc()にすべき
	int index;
	int len, i;

	// 通信中には呼ばれないはずだが、念のため。(2006.6.26 maya)
	if (pvar->socket != INVALID_SOCKET) {
		return;
	}

	buf[0] = '\0';
	for (i = 0 ; pvar->settings.HostKeyOrder[i] != 0 ; i++) {
		index = pvar->settings.HostKeyOrder[i] - '0';
		if (index == KEY_NONE) // disabled line
			break;
		strncat_s(buf, sizeof(buf), get_ssh_keytype_name(index), _TRUNCATE);
		strncat_s(buf, sizeof(buf), ",", _TRUNCATE);
	}
	len = strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';  // get rid of comma
	myproposal[PROPOSAL_SERVER_HOST_KEY_ALGS] = buf; 
}

// HMACアルゴリズム優先順位に応じて、myproposal[]を書き換える。
// (2011.2.28 yutaka)
void SSH2_update_hmac_myproposal(PTInstVar pvar)
{
	static char buf[256]; // TODO: malloc()にすべき
	int index;
	int len, i;

	// 通信中には呼ばれないはずだが、念のため。(2006.6.26 maya)
	if (pvar->socket != INVALID_SOCKET) {
		return;
	}

	buf[0] = '\0';
	for (i = 0 ; pvar->settings.MacOrder[i] != 0 ; i++) {
		index = pvar->settings.MacOrder[i] - '0';
		if (index == HMAC_NONE) // disabled line
			break;
		strncat_s(buf, sizeof(buf), get_ssh2_mac_name(index), _TRUNCATE);
		strncat_s(buf, sizeof(buf), ",", _TRUNCATE);
	}
	len = strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';  // get rid of comma
	myproposal[PROPOSAL_MAC_ALGS_CTOS] = buf; 
	myproposal[PROPOSAL_MAC_ALGS_STOC] = buf; 
}

// クライアントからサーバへのキー交換開始要求
void SSH2_send_kexinit(PTInstVar pvar)
{
	char cookie[SSH2_COOKIE_LENGTH];
	buffer_t *msg;
	unsigned char *outmsg;
	int len, i;

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
		return;
	}
	if (pvar->my_kex != NULL)
		buffer_free(pvar->my_kex);
	pvar->my_kex = msg;

	// メッセージタイプ
	//SSH2_packet_start(msg, SSH2_MSG_KEXINIT);

	// cookieのセット
	CRYPT_set_random_data(pvar, cookie, sizeof(cookie));
	CRYPT_set_server_cookie(pvar, cookie);
	buffer_append(msg, cookie, sizeof(cookie));

	// クライアントのキー情報
	for (i = 0 ; i < PROPOSAL_MAX ; i++) {
		buffer_put_string(msg, myproposal[i], strlen(myproposal[i]));
	}
	buffer_put_char(msg, 0);
	buffer_put_int(msg, 0);


	logprintf(LOG_LEVEL_VERBOSE,
		"client proposal: KEX algorithm: %s",
		myproposal[0]);

	logprintf(LOG_LEVEL_VERBOSE,
		"client proposal: server host key algorithm: %s",
		myproposal[1]);

	logprintf(LOG_LEVEL_VERBOSE,
		"client proposal: encryption algorithm client to server: %s",
		myproposal[2]);

	logprintf(LOG_LEVEL_VERBOSE,
		"client proposal: encryption algorithm server to client: %s",
		myproposal[3]);

	logprintf(LOG_LEVEL_VERBOSE,
		"client proposal: MAC algorithm client to server: %s",
		myproposal[4]);

	logprintf(LOG_LEVEL_VERBOSE,
		"client proposal: MAC algorithm server to client: %s",
		myproposal[5]);

	logprintf(LOG_LEVEL_VERBOSE,
		"client proposal: compression algorithm client to server: %s",
		myproposal[6]);

	logprintf(LOG_LEVEL_VERBOSE,
		"client proposal: compression algorithm server to client: %s",
		myproposal[7]);


	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_KEXINIT, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);

	// my_kexに取っておくため、フリーしてはいけない。
	//buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEXINIT was sent at SSH2_send_kexinit().");
}


static void choose_SSH2_proposal(char *server_proposal,
                                 char *my_proposal,
                                 char *dest,
                                 int dest_len)
{
	char tmp_cli[1024], *ptr_cli, *ctc_cli;
	char tmp_svr[1024], *ptr_svr, *ctc_svr;
	SSHCipher cipher = SSH_CIPHER_NONE;

	strncpy_s(tmp_cli, sizeof(tmp_cli), my_proposal, _TRUNCATE);
	ptr_cli = strtok_s(tmp_cli, ",", &ctc_cli);
	while (ptr_cli != NULL) {
		// server_proposalにはサーバのproposalがカンマ文字列で格納されている
		strncpy_s(tmp_svr, sizeof(tmp_svr), server_proposal, _TRUNCATE);
		ptr_svr = strtok_s(tmp_svr, ",", &ctc_svr);
		while (ptr_svr != NULL) {
			if (strcmp(ptr_svr, ptr_cli) == 0) { // match
				goto found;
			}
			ptr_svr = strtok_s(NULL, ",", &ctc_svr);
		}
		ptr_cli = strtok_s(NULL, ",", &ctc_cli);
	}

found:
	if (ptr_cli != NULL) {
		strncpy_s(dest, dest_len, ptr_cli, _TRUNCATE);
	}
	else {
		strncpy_s(dest, dest_len, "", _TRUNCATE);
	}
}

static kex_algorithm choose_SSH2_kex_algorithm(char *server_proposal, char *my_proposal)
{
	kex_algorithm type = KEX_DH_UNKNOWN;
	char str_kextype[40];
	ssh2_kex_algorithm_t *ptr = ssh2_kex_algorithms;

	choose_SSH2_proposal(server_proposal, my_proposal, str_kextype, sizeof(str_kextype));

	while (ptr->name != NULL) {
		if (strcmp(ptr->name, str_kextype) == 0) {
			type = ptr->kextype;
			break;
		}
		ptr++;
	}

	return (type);
}

static SSHCipher choose_SSH2_cipher_algorithm(char *server_proposal, char *my_proposal)
{
	SSHCipher cipher = SSH_CIPHER_NONE;
	char str_cipher[32];
	ssh2_cipher_t *ptr = ssh2_ciphers;

	choose_SSH2_proposal(server_proposal, my_proposal, str_cipher, sizeof(str_cipher));

	while (ptr->name != NULL) {
		if (strcmp(ptr->name, str_cipher) == 0) {
			cipher = ptr->cipher;
			break;
		}
		ptr++;
	}

	return (cipher);
}


static hmac_type choose_SSH2_hmac_algorithm(char *server_proposal, char *my_proposal)
{
	hmac_type type = HMAC_UNKNOWN;
	char str_hmac[64];
	ssh2_mac_t *ptr = ssh2_macs;

	choose_SSH2_proposal(server_proposal, my_proposal, str_hmac, sizeof(str_hmac));

	while (ptr->name != NULL) {
		if (strcmp(ptr->name, str_hmac) == 0) {
			type = ptr->type;
			break;
		}
		ptr++;
	}

	return (type);
}


static compression_type choose_SSH2_compression_algorithm(char *server_proposal, char *my_proposal)
{
	compression_type type = COMP_UNKNOWN;
	char str_comp[20];
	ssh2_comp_t *ptr = ssh2_comps;

	// OpenSSH 4.3では遅延パケット圧縮("zlib@openssh.com")が新規追加されているため、
	// マッチしないように修正した。
	// 現Tera Termでは遅延パケット圧縮は将来的にサポートする予定。
	// (2006.6.14 yutaka)
	// 遅延パケット圧縮に対応。
	// (2006.6.23 maya)

	choose_SSH2_proposal(server_proposal, my_proposal, str_comp, sizeof(str_comp));

	while (ptr->name != NULL) {
		if (strcmp(ptr->name, str_comp) == 0) {
			type = ptr->type;
			break;
		}
		ptr++;
	}

	return (type);
}

// 暗号アルゴリズムのキーサイズ、ブロックサイズ、MACサイズのうち最大値(we_need)を決定する。
static void choose_SSH2_key_maxlength(PTInstVar pvar)
{
	int mode, need, val, ctos;
	const EVP_MD *md;

	for (mode = 0; mode < MODE_MAX; mode++) {
		if (mode == MODE_OUT)
			ctos = 1;
		else
			ctos = 0;

		if (ctos == 1) {
			val = pvar->ctos_hmac;
		} else {
			val = pvar->stoc_hmac;
		}

		// current_keys[]に設定しておいて、あとで pvar->ssh2_keys[] へコピーする。
		md = get_ssh2_mac_EVP_MD(val);
		current_keys[mode].mac.md = md;
		current_keys[mode].mac.key_len = current_keys[mode].mac.mac_len = EVP_MD_size(md);
		if (get_ssh2_mac_truncatebits(val) != 0) {
			current_keys[mode].mac.mac_len = get_ssh2_mac_truncatebits(val) / 8;
		}

		// キーサイズとブロックサイズもここで設定しておく (2004.11.7 yutaka)
		if (ctos == 1) {
			current_keys[mode].enc.key_len = get_cipher_key_len(pvar->ctos_cipher);
			current_keys[mode].enc.block_size = get_cipher_block_size(pvar->ctos_cipher);
		} else {
			current_keys[mode].enc.key_len = get_cipher_key_len(pvar->stoc_cipher);
			current_keys[mode].enc.block_size = get_cipher_block_size(pvar->stoc_cipher);
		}
		current_keys[mode].mac.enabled = 0;
		current_keys[mode].comp.enabled = 0; // (2005.7.9 yutaka)

		// 現時点ではMACはdisable
		pvar->ssh2_keys[mode].mac.enabled = 0;
		pvar->ssh2_keys[mode].comp.enabled = 0; // (2005.7.9 yutaka)
	}
	need = 0;
	for (mode = 0; mode < MODE_MAX; mode++) {
		if (mode == MODE_OUT)
			ctos = 1;
		else
			ctos = 0;

		val = current_keys[mode].enc.key_len;
		if (need < val)
			need = val;

		val = current_keys[mode].enc.block_size;
		if (need < val)
			need = val;

		val = current_keys[mode].mac.key_len;
		if (need < val)
			need = val;
	}
	pvar->we_need = need;

}


// キー交換開始前のチェック (SSH2_MSG_KEXINIT)
// ※当該関数はデータ通信中にも呼ばれてくる可能性あり
static BOOL handle_SSH2_kexinit(PTInstVar pvar)
{
	char buf[1024];
	char *data;
	int len, i, size;
	int offset = 0;
	char *msg = NULL;
	char tmp[1024+512];
	char str_keytype[20];

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEXINIT was received.");

	// すでにキー交換が終わっているにも関わらず、サーバから SSH2_MSG_KEXINIT が
	// 送られてくる場合は、キー再作成を行う。(2004.10.24 yutaka)
	if (pvar->key_done == 1) {
		pvar->rekeying = 1;
		pvar->key_done = 0;

		// サーバへSSH2_MSG_KEXINIT を送る
		SSH2_send_kexinit(pvar);
	}

	if (pvar->peer_kex != NULL) { // already allocated
		buffer_free(pvar->peer_kex);
	}
	pvar->peer_kex = buffer_init();
	if (pvar->peer_kex == NULL) {
		msg = "Out of memory @ handle_SSH2_kexinit()";
		goto error;
	}
	// [-2]:padding size
//	len = pvar->ssh_state.payloadlen + pvar->ssh_state.payload[-2] + 1;
	len = pvar->ssh_state.payloadlen - 1;
//	buffer_append(pvar->peer_kex, &pvar->ssh_state.payload[-6], len);
	buffer_append(pvar->peer_kex, pvar->ssh_state.payload, len);
	//write_buffer_file(&pvar->ssh_state.payload[-6], len);

	// TODO: buffer overrun check

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディング+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	//write_buffer_file(data, len);
	push_memdump("KEXINIT", "exchange algorithm list: receiving", data, len);

	if (offset + 20 >= len) {
		msg = "payload size too small @ handle_SSH2_kexinit()";
		goto error;
	}

#if 0
	for (i = 0; i < SSH2_COOKIE_LENGTH; i++) {
		cookie[i] = data[i];
	}
#endif
	// get rid of Cookie length
	offset += SSH2_COOKIE_LENGTH;


	// KEXの決定。判定順をmyproposal[PROPOSAL_KEX_ALGS]の並びと合わせること。
	// サーバは、クライアントから送られてきた myproposal[PROPOSAL_KEX_ALGS] のカンマ文字列のうち、
	// 先頭から自分の myproposal[] と比較を行い、最初にマッチしたものがKEXアルゴリズムとして
	// 選択される。(2004.10.30 yutaka)

	// キー交換アルゴリズムチェック
	size = get_payload_uint32(pvar, offset);
	offset += 4;
	for (i = 0; i < size; i++) {
		buf[i] = data[offset + i];
	}
	buf[i] = '\0'; // null-terminate
	offset += size;

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: KEX algorithm: %s", buf);

	pvar->kex_type = choose_SSH2_kex_algorithm(buf, myproposal[PROPOSAL_KEX_ALGS]);
	if (pvar->kex_type == KEX_DH_UNKNOWN) { // not match
		strncpy_s(tmp, sizeof(tmp), "unknown KEX algorithm: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}


	// ホストキーアルゴリズムチェック
	size = get_payload_uint32(pvar, offset);
	offset += 4;
	for (i = 0; i < size; i++) {
		buf[i] = data[offset + i];
	}
	buf[i] = 0;
	offset += size;

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: server host key algorithm: %s", buf);

	pvar->hostkey_type = KEY_UNSPEC;
	choose_SSH2_proposal(buf, myproposal[PROPOSAL_SERVER_HOST_KEY_ALGS], str_keytype, sizeof(str_keytype));
	if (strlen(str_keytype) == 0) { // not match
		strncpy_s(tmp, sizeof(tmp), "unknown host KEY type: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}
	pvar->hostkey_type = get_keytype_from_name(str_keytype);
	if (pvar->hostkey_type == KEY_UNSPEC) {
		strncpy_s(tmp, sizeof(tmp), "unknown host KEY type: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}


	// クライアント -> サーバ暗号アルゴリズムチェック
	size = get_payload_uint32(pvar, offset);
	offset += 4;
	for (i = 0; i < size; i++) {
		buf[i] = data[offset + i];
	}
	buf[i] = 0;
	offset += size;

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: encryption algorithm client to server: %s", buf);

	pvar->ctos_cipher = choose_SSH2_cipher_algorithm(buf, myproposal[PROPOSAL_ENC_ALGS_CTOS]);
	if (pvar->ctos_cipher == SSH_CIPHER_NONE) {
		strncpy_s(tmp, sizeof(tmp), "unknown Encrypt algorithm(ctos): ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}


	// サーバ -> クライアント暗号アルゴリズムチェック
	size = get_payload_uint32(pvar, offset);
	offset += 4;
	for (i = 0; i < size; i++) {
		buf[i] = data[offset + i];
	}
	buf[i] = 0;
	offset += size;

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: encryption algorithm server to client: %s", buf);

	pvar->stoc_cipher = choose_SSH2_cipher_algorithm(buf, myproposal[PROPOSAL_ENC_ALGS_STOC]);
	if (pvar->stoc_cipher == SSH_CIPHER_NONE) {
		strncpy_s(tmp, sizeof(tmp), "unknown Encrypt algorithm(stoc): ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}


	// MAC(Message Authentication Code)アルゴリズムの決定 (2004.12.17 yutaka)
	size = get_payload_uint32(pvar, offset);
	offset += 4;
	for (i = 0; i < size; i++) {
		buf[i] = data[offset + i];
	}
	buf[i] = 0;
	offset += size;

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: MAC algorithm client to server: %s", buf);

	pvar->ctos_hmac = choose_SSH2_hmac_algorithm(buf, myproposal[PROPOSAL_MAC_ALGS_CTOS]);
	if (pvar->ctos_hmac == HMAC_UNKNOWN) { // not match
		strncpy_s(tmp, sizeof(tmp), "unknown MAC algorithm: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}


	size = get_payload_uint32(pvar, offset);
	offset += 4;
	for (i = 0; i < size; i++) {
		buf[i] = data[offset + i];
	}
	buf[i] = 0;
	offset += size;

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: MAC algorithm server to client: %s", buf);

	pvar->stoc_hmac = choose_SSH2_hmac_algorithm(buf, myproposal[PROPOSAL_MAC_ALGS_STOC]);
	if (pvar->stoc_hmac == HMAC_UNKNOWN) { // not match
		strncpy_s(tmp, sizeof(tmp), "unknown MAC algorithm: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}


	// 圧縮アルゴリズムの決定
	// pvar->ssh_state.compressing = FALSE; として下記メンバを使用する。
	// (2005.7.9 yutaka)
	size = get_payload_uint32(pvar, offset);
	offset += 4;
	for (i = 0; i < size; i++) {
		buf[i] = data[offset + i];
	}
	buf[i] = 0;
	offset += size;

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: compression algorithm client to server: %s", buf);

	pvar->ctos_compression = choose_SSH2_compression_algorithm(buf, myproposal[PROPOSAL_COMP_ALGS_CTOS]);
	if (pvar->ctos_compression == COMP_UNKNOWN) { // not match
		strncpy_s(tmp, sizeof(tmp), "unknown Packet Compression algorithm: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}


	size = get_payload_uint32(pvar, offset);
	offset += 4;
	for (i = 0; i < size; i++) {
		buf[i] = data[offset + i];
	}
	buf[i] = 0;
	offset += size;

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: compression algorithm server to client: %s", buf);

	pvar->stoc_compression = choose_SSH2_compression_algorithm(buf, myproposal[PROPOSAL_COMP_ALGS_STOC]);
	if (pvar->stoc_compression == COMP_UNKNOWN) { // not match
		strncpy_s(tmp, sizeof(tmp), "unknown Packet Compression algorithm: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}


	// 決定
	logprintf(LOG_LEVEL_VERBOSE, "KEX algorithm: %s",
		get_kex_algorithm_name(pvar->kex_type));

	logprintf(LOG_LEVEL_VERBOSE,
		"server host key algorithm: %s",
		get_ssh_keytype_name(pvar->hostkey_type));

	logprintf(LOG_LEVEL_VERBOSE,
		"encryption algorithm client to server: %s",
		get_cipher_string(pvar->ctos_cipher));

	logprintf(LOG_LEVEL_VERBOSE,
		"encryption algorithm server to client: %s",
		get_cipher_string(pvar->stoc_cipher));

	logprintf(LOG_LEVEL_VERBOSE,
		"MAC algorithm client to server: %s",
		get_ssh2_mac_name(pvar->ctos_hmac));

	logprintf(LOG_LEVEL_VERBOSE,
		"MAC algorithm server to client: %s",
		get_ssh2_mac_name(pvar->stoc_hmac));

	logprintf(LOG_LEVEL_VERBOSE,
		"compression algorithm client to server: %s",
		get_ssh2_comp_name(pvar->ctos_compression));

	logprintf(LOG_LEVEL_VERBOSE,
		"compression algorithm server to client: %s",
		get_ssh2_comp_name(pvar->stoc_compression));


	// we_needの決定 (2004.11.6 yutaka)
	// キー再作成の場合はスキップする。
	if (pvar->rekeying == 0) {
		choose_SSH2_key_maxlength(pvar);
	}

	// send DH kex init
	switch (pvar->kex_type) {
		case KEX_DH_GRP1_SHA1:
		case KEX_DH_GRP14_SHA1:
		case KEX_DH_GRP14_SHA256:
		case KEX_DH_GRP16_SHA512:
		case KEX_DH_GRP18_SHA512:
			SSH2_dh_kex_init(pvar);
			break;
		case KEX_DH_GEX_SHA1:
		case KEX_DH_GEX_SHA256:
			SSH2_dh_gex_kex_init(pvar);
			break;
		case KEX_ECDH_SHA2_256:
		case KEX_ECDH_SHA2_384:
		case KEX_ECDH_SHA2_521:
			SSH2_ecdh_kex_init(pvar);
			break;
		default:
			// TODO
			break;
	}

	return TRUE;

error:;
	buffer_free(pvar->peer_kex);
	pvar->peer_kex = NULL;

	notify_fatal_error(pvar, msg, TRUE);

	return FALSE;
}


//
// 固定 DH Groups (RFC 4253, draft-baushke-ssh-dh-group-sha2-04)
//
static void SSH2_dh_kex_init(PTInstVar pvar)
{
	DH *dh = NULL;
	buffer_t *msg = NULL;
	unsigned char *outmsg;
	int len;

	// Diffie-Hellman key agreement
	switch (pvar->kex_type) {
	case KEX_DH_GRP1_SHA1:
		dh = dh_new_group1();
		break;
	case KEX_DH_GRP14_SHA1:
	case KEX_DH_GRP14_SHA256:
		dh = dh_new_group14();
		break;
	case KEX_DH_GRP16_SHA512:
		dh = dh_new_group16();
		break;
	case KEX_DH_GRP18_SHA512:
		dh = dh_new_group18();
		break;
	default:
		goto error;
	}

	// 秘密にすべき乱数(X)を生成
	dh_gen_key(pvar, dh, pvar->we_need);

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
		return;
	}

	buffer_put_bignum2(msg, dh->pub_key);

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_KEXDH_INIT, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);

	if (pvar->kexdh != NULL) {
		DH_free(pvar->kexdh);
	}
	pvar->kexdh = dh;

	SSH2_dispatch_init(2);
	SSH2_dispatch_add_message(SSH2_MSG_KEXDH_REPLY);
	SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX: Tru64 UNIX workaround   (2005.3.5 yutaka)
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);

	buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEXDH_INIT was sent at SSH2_dh_kex_init().");

	return;

error:;
	DH_free(dh);
	buffer_free(msg);

	notify_fatal_error(pvar, "error occurred @ SSH2_dh_kex_init()", TRUE);
}



//
// DH-GEX (RFC 4419)
//

static void SSH2_dh_gex_kex_init(PTInstVar pvar)
{
	buffer_t *msg = NULL;
	unsigned char *outmsg;
	int len;
	int bits, min, max;

	msg = buffer_init();
	if (msg == NULL) {
		goto error;
	}

	// サーバが保証すべき最低限のビット数を求める（we_needはバイト）。
	if (pvar->settings.GexMinimalGroupSize < GEX_GRP_MINSIZE) {
		min = GEX_GRP_MINSIZE;
	}
	else if (pvar->settings.GexMinimalGroupSize > GEX_GRP_MAXSIZE) {
		min = GEX_GRP_MAXSIZE;
	}
	else {
		min = pvar->settings.GexMinimalGroupSize;
	}
	max = GEX_GRP_MAXSIZE;
	bits = dh_estimate(pvar->we_need * 8);
	if (bits < min) {
		bits = min;
	}
	else if (bits > max) {
		bits = max;
	}
	if (pvar->server_compat_flag & SSH_BUG_DHGEX_LARGE && bits > 4096) {
		logprintf(LOG_LEVEL_NOTICE,
			"SSH_BUG_DHGEX_LARGE is enabled. DH-GEX group size is limited to 4096. "
			"(Original size is %d)", bits);
		bits = 4096;
	}

	// サーバへgroup sizeを送って、p と g を作ってもらう。
	buffer_put_int(msg, min);
	buffer_put_int(msg, bits);
	buffer_put_int(msg, max);
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_KEX_DH_GEX_REQUEST, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);

	// あとでハッシュ計算に使うので取っておく。
	pvar->kexgex_min = min;
	pvar->kexgex_bits = bits;
	pvar->kexgex_max = max;

	{
		char tmp[128];
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
		            "we_need %d min %d bits %d max %d",
		            pvar->we_need, min, bits, max);
		push_memdump("DH_GEX_REQUEST", "requested key bits", tmp, strlen(tmp));
	}

	SSH2_dispatch_init(2);
	SSH2_dispatch_add_message(SSH2_MSG_KEX_DH_GEX_GROUP);
	SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX: Tru64 UNIX workaround   (2005.3.5 yutaka)
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);

	buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_DH_GEX_REQUEST was sent at SSH2_dh_gex_kex_init().");

	return;

error:;
	buffer_free(msg);

	notify_fatal_error(pvar, "error occurred @ SSH2_dh_gex_kex_init()", TRUE);
}


// SSH2_MSG_KEX_DH_GEX_GROUP
static BOOL handle_SSH2_dh_gex_group(PTInstVar pvar)
{
	char *data;
	int len, grp_bits;
	BIGNUM *p = NULL, *g = NULL;
	DH *dh = NULL;
	buffer_t *msg = NULL;
	unsigned char *outmsg;
	char tmpbuf[256];

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_DH_GEX_GROUP was received.");

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	p = BN_new();
	g = BN_new();
	if (p == NULL || g == NULL)
		goto error;

	buffer_get_bignum2(&data, p); // 素数の取得
	buffer_get_bignum2(&data, g); // 生成元の取得

	grp_bits = BN_num_bits(p);
	logprintf(LOG_LEVEL_VERBOSE, "DH-GEX: Request: %d / %d / %d, Received: %d",
	            pvar->kexgex_min, pvar->kexgex_bits, pvar->kexgex_max, BN_num_bits(p));

	//
	// (1) < GEX_GRP_MINSIZE <= (2) < kexgex_min <= (3) < kexgex_bits <= (4) <= kexgex_max < (5) <= GEX_GRP_MAXSIZE < (6)
	//
	if (grp_bits < GEX_GRP_MINSIZE || grp_bits > GEX_GRP_MAXSIZE) {
	// (1), (6) プロトコルで認められている範囲(1024 <= grp_bits <= 8192)の外。強制切断。
		UTIL_get_lang_msg("MSG_SSH_GEX_SIZE_OUTOFRANGE", pvar,
		                  "Received group size is out of range: %d");
		_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, pvar->ts->UIMsg, grp_bits);
		notify_fatal_error(pvar, tmpbuf, FALSE);
		goto error;
	}
	else if (grp_bits < pvar->kexgex_min) {
	// (2) プロトコルで認められている範囲内だが、こちらの設定した最小値より小さい。確認ダイアログを出す。
		logprintf(LOG_LEVEL_WARNING,
		    "DH-GEX: grp_bits(%d) < kexgex_min(%d)", grp_bits, pvar->kexgex_min);
		UTIL_get_lang_msg("MSG_SSH_GEX_SIZE_SMALLER", pvar,
		                  "Received group size is smaller than the requested minimal size.\nrequested: %d, received: %d\nAre you sure that you want to accecpt received group?");
		_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE,
			pvar->ts->UIMsg, pvar->kexgex_min, grp_bits);
	}
	else if (grp_bits < pvar->kexgex_bits) {
	// (3) 要求の最小値は満たすが、要求値よりは小さい。確認ダイアログは出さない。
		logprintf(LOG_LEVEL_NOTICE,
			"DH-GEX: grp_bits(%d) < kexgex_bits(%d)", grp_bits, pvar->kexgex_bits);
#if 1
		tmpbuf[0] = 0; // no message
#else
		_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE,
			"Received group size is smaller than requested.\nrequested: %d, received: %d\nAccept this?",
			pvar->kexgex_bits, grp_bits);
#endif
	}
	else if (grp_bits <= pvar->kexgex_max) {
	// (4) 要求値以上、かつ要求の最大値以下。問題なし。
		tmpbuf[0] = 0; // no message
	}
	else {
	// (5) こちらの設定した最大値より大きい。確認ダイアログを出す。
	//     ただし現状では kexgex_max == GEX_GRP_MAXSIZE(8192) である為この状況になる事は無い。
		logprintf(LOG_LEVEL_WARNING,
			"DH-GEX: grp_bits(%d) > kexgex_max(%d)", grp_bits, pvar->kexgex_max);
		UTIL_get_lang_msg("MSG_SSH_GEX_SIZE_LARGER", pvar,
		                  "Received group size is larger than the requested maximal size.\nrequested: %d, received: %d\nAre you sure that you want to accecpt received group?");
		_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE,
			pvar->ts->UIMsg, pvar->kexgex_max, grp_bits);
	}
	
	if (tmpbuf[0] != 0) {
		UTIL_get_lang_msg("MSG_SSH_GEX_SIZE_TITLE", pvar,
		                  "TTSSH: Confirm GEX group size");
		if (MessageBox(NULL, tmpbuf, pvar->ts->UIMsg, MB_YESNO | MB_ICONERROR) == IDNO) {
			UTIL_get_lang_msg("MSG_SSH_GEX_SIZE_CANCEL", pvar,
			                  "New connection is cancelled.");
			notify_fatal_error(pvar, pvar->ts->UIMsg, FALSE);
			goto error;
		}
	}

	dh = DH_new();
	if (dh == NULL)
		goto error;
	dh->p = p;
	dh->g = g;

	// 秘密にすべき乱数(X)を生成
	dh_gen_key(pvar, dh, pvar->we_need);

	// 公開鍵をサーバへ送信
	msg = buffer_init();
	if (msg == NULL) {
		goto error;
	}
	buffer_put_bignum2(msg, dh->pub_key);
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_KEX_DH_GEX_INIT, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_DH_GEX_INIT was sent at handle_SSH2_dh_gex_group().");

	// ここで作成したDH鍵は、あとでハッシュ計算に使うため取っておく。(2004.10.31 yutaka)
	if (pvar->kexdh != NULL) {
		DH_free(pvar->kexdh);
	}
	pvar->kexdh = dh;

	{
		push_bignum_memdump("DH_GEX_GROUP", "p", dh->p);
		push_bignum_memdump("DH_GEX_GROUP", "g", dh->g);
		push_bignum_memdump("DH_GEX_GROUP", "pub_key", dh->pub_key);
	}

	SSH2_dispatch_init(2);
	SSH2_dispatch_add_message(SSH2_MSG_KEX_DH_GEX_REPLY);
	SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX: Tru64 UNIX workaround   (2005.3.5 yutaka)
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);

	buffer_free(msg);

	return TRUE;

error:;
	BN_free(p);
	BN_free(g);
	DH_free(dh);

	return FALSE;
}


//
// KEX_ECDH_SHA2_256 or KEX_ECDH_SHA2_384 or KEX_ECDH_SHA2_521
//
 
static void SSH2_ecdh_kex_init(PTInstVar pvar)
{
	EC_KEY *client_key = NULL;
	const EC_GROUP *group;
	buffer_t *msg = NULL;
	unsigned char *outmsg;
	int len;

	client_key = EC_KEY_new();
	if (client_key == NULL) {
		goto error;
	}
	client_key = EC_KEY_new_by_curve_name(kextype_to_cipher_nid(pvar->kex_type));
	if (client_key == NULL) {
		goto error;
	}
	if (EC_KEY_generate_key(client_key) != 1) {
		goto error;
	}
	group = EC_KEY_get0_group(client_key);


	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
		return;
	}

	buffer_put_ecpoint(msg, group, EC_KEY_get0_public_key(client_key));

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_KEX_ECDH_INIT, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);

	// ここで作成した鍵は、あとでハッシュ計算に使うため取っておく。
	if (pvar->ecdh_client_key) {
		EC_KEY_free(pvar->ecdh_client_key);
	}
	pvar->ecdh_client_key = client_key;

	SSH2_dispatch_init(2);
	SSH2_dispatch_add_message(SSH2_MSG_KEX_ECDH_REPLY);
	SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX: Tru64 UNIX workaround   (2005.3.5 yutaka)
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);

	buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_ECDH_INIT was sent at SSH2_ecdh_kex_init().");

	return;

error:;
	EC_KEY_free(client_key);
	buffer_free(msg);

	notify_fatal_error(pvar, "error occurred @ SSH2_ecdh_kex_init()", TRUE);
}


static void ssh2_set_newkeys(PTInstVar pvar, int mode)
{
#if 1
	// free already allocated buffer
	if (pvar->ssh2_keys[mode].enc.iv != NULL) {
		free(pvar->ssh2_keys[mode].enc.iv);
	}
	if (pvar->ssh2_keys[mode].enc.key != NULL) {
		free(pvar->ssh2_keys[mode].enc.key);
	}
	if (pvar->ssh2_keys[mode].mac.key != NULL) {
		free(pvar->ssh2_keys[mode].mac.key);
	}
#endif

	pvar->ssh2_keys[mode] = current_keys[mode];
}


//
// Diffie-Hellman Key Exchange Reply(SSH2_MSG_KEXDH_REPLY:31)
//
static BOOL handle_SSH2_dh_kex_reply(PTInstVar pvar)
{
	char *data;
	int len;
	int offset = 0;
	char *server_host_key_blob;
	int bloblen, siglen;
	BIGNUM *dh_server_pub = NULL;
	char *signature;
	int dh_len, share_len;
	char *dh_buf = NULL;
	BIGNUM *share_key = NULL;
	char *hash;
	char *emsg, emsg_tmp[1024];  // error message
	int ret, hashlen;
	Key *hostkey;  // hostkey

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEXDH_REPLY was received.");

	memset(&hostkey, 0, sizeof(hostkey));

	// TODO: buffer overrun check

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	// for debug
	push_memdump("KEXDH_REPLY", "key exchange: receiving", data, len);

	bloblen = get_uint32_MSBfirst(data);
	data += 4;
	server_host_key_blob = data; // for hash

	push_memdump("KEXDH_REPLY", "server_host_key_blob", server_host_key_blob, bloblen);

	hostkey = key_from_blob(data, bloblen);
	if (hostkey == NULL) {
		emsg = "key_from_blob error @ handle_SSH2_dh_kex_reply()";
		goto error;
	}
	data += bloblen;

	// known_hosts対応 (2006.3.20 yutaka)
	if (hostkey->type != pvar->hostkey_type) {  // ホストキーの種別比較
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
		            "type mismatch for decoded server_host_key_blob (kex:%s blob:%s) @ %s",
		            get_ssh_keytype_name(pvar->hostkey_type), get_ssh_keytype_name(hostkey->type), __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	HOSTS_check_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, hostkey);
	if (pvar->socket == INVALID_SOCKET) {
		emsg = "Server disconnected @ handle_SSH2_dh_kex_reply()";
		goto error;
	}

	dh_server_pub = BN_new();
	if (dh_server_pub == NULL) {
		emsg = "Out of memory1 @ handle_SSH2_dh_kex_reply()";
		goto error;
	}

	buffer_get_bignum2(&data, dh_server_pub);

	siglen = get_uint32_MSBfirst(data);
	data += 4;
	signature = data;
	data += siglen;


	// check DH public value
	if (!dh_pub_is_valid(pvar->kexdh, dh_server_pub)) {
		emsg = "DH public value invalid @ handle_SSH2_dh_kex_reply()";
		goto error;
	}
	// 共通鍵の生成
	dh_len = DH_size(pvar->kexdh);
	dh_buf = malloc(dh_len);
	if (dh_buf == NULL) {
		emsg = "Out of memory2 @ handle_SSH2_dh_kex_reply()";
		goto error;
	}
	share_len = DH_compute_key(dh_buf, dh_server_pub, pvar->kexdh);
	share_key = BN_new();
	if (share_key == NULL) {
		emsg = "Out of memory3 @ handle_SSH2_dh_kex_reply()";
		goto error;
	}
	// 'share_key'がサーバとクライアントで共有する鍵（G^A×B mod P）となる。
	BN_bin2bn(dh_buf, share_len, share_key);
	//debug_print(40, dh_buf, share_len);

	// ハッシュの計算
	/* calc and verify H */
	hash = kex_dh_hash(get_kex_algorithm_EVP_MD(pvar->kex_type),
	                   pvar->client_version_string,
	                   pvar->server_version_string,
	                   buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex),
	                   buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex),
	                   server_host_key_blob, bloblen,
	                   pvar->kexdh->pub_key,
	                   dh_server_pub,
	                   share_key,
	                   &hashlen);

	{
		push_memdump("KEXDH_REPLY kex_dh_kex_hash", "my_kex", buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex));
		push_memdump("KEXDH_REPLY kex_dh_kex_hash", "peer_kex", buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex));

		push_bignum_memdump("KEXDH_REPLY kex_dh_kex_hash", "dh_server_pub", dh_server_pub);
		push_bignum_memdump("KEXDH_REPLY kex_dh_kex_hash", "share_key", share_key);

		push_memdump("KEXDH_REPLY kex_dh_kex_hash", "hash", hash, hashlen);
	}

	//debug_print(30, hash, hashlen);
	//debug_print(31, pvar->client_version_string, strlen(pvar->client_version_string));
	//debug_print(32, pvar->server_version_string, strlen(pvar->server_version_string));
	//debug_print(33, buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex));
	//debug_print(34, buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex));
	//debug_print(35, server_host_key_blob, bloblen);

	// session idの保存（初回接続時のみ）
	if (pvar->session_id == NULL) {
		pvar->session_id_len = hashlen;
		pvar->session_id = malloc(pvar->session_id_len);
		if (pvar->session_id != NULL) {
			memcpy(pvar->session_id, hash, pvar->session_id_len);
		} else {
			// TODO:
		}
	}

	if ((ret = key_verify(hostkey, signature, siglen, hash, hashlen)) != 1) {
		if (ret == -3 && hostkey->type == KEY_RSA) {
			if (!pvar->settings.EnableRsaShortKeyServer) {
				_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
				            "key verify error(remote rsa key length is too short %d-bit) "
				            "@ handle_SSH2_dh_kex_reply()", BN_num_bits(hostkey->rsa->n));
			}
			else {
				goto cont;
			}
		}
		else {
			_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
			            "key verify error(%d) @ handle_SSH2_dh_kex_reply()\r\n%s", ret, SENDTOME);
		}
		emsg = emsg_tmp;
		save_memdump(LOGDUMP);
		goto error;
	}

cont:
	kex_derive_keys(pvar, pvar->we_need, hash, share_key, pvar->session_id, pvar->session_id_len);

	// KEX finish
	begin_send_packet(pvar, SSH2_MSG_NEWKEYS, 0);
	finish_send_packet(pvar);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_NEWKEYS was sent at handle_SSH2_dh_kex_reply().");

	// SSH2_MSG_NEWKEYSを送り終わったあとにキーの設定および再設定を行う
	// 送信用の暗号鍵は SSH2_MSG_NEWKEYS の送信後に、受信用のは SSH2_MSG_NEWKEYS の
	// 受信後に再設定を行う。
	if (pvar->rekeying == 1) { // キーの再設定
		// まず、送信用だけ設定する。
		ssh2_set_newkeys(pvar, MODE_OUT);
		pvar->ssh2_keys[MODE_OUT].mac.enabled = 1;
		pvar->ssh2_keys[MODE_OUT].comp.enabled = 1;
		enable_send_compression(pvar);
		if (!CRYPT_start_encryption(pvar, 1, 0)) {
			// TODO: error
		}

	} else {
		// 初回接続の場合は実際に暗号ルーチンが設定されるのは、あとになってから
		// なので（CRYPT_start_encryption関数）、ここで鍵の設定をしてしまってもよい。
		ssh2_set_newkeys(pvar, MODE_IN);
		ssh2_set_newkeys(pvar, MODE_OUT);

		// SSH2_MSG_NEWKEYSを送信した時点で、MACを有効にする。(2006.10.30 yutaka)
		pvar->ssh2_keys[MODE_OUT].mac.enabled = 1;
		pvar->ssh2_keys[MODE_OUT].comp.enabled = 1;

		// パケット圧縮が有効なら初期化する。(2005.7.9 yutaka)
		// SSH2_MSG_NEWKEYSの受信より前なのでここだけでよい。(2006.10.30 maya)
		prep_compression(pvar);
		enable_compression(pvar);
	}

	// TTSSHバージョン情報に表示するキービット数を求めておく (2004.10.30 yutaka)
	pvar->client_key_bits = BN_num_bits(pvar->kexdh->pub_key);
	pvar->server_key_bits = BN_num_bits(dh_server_pub);

	SSH2_dispatch_init(3);
	SSH2_dispatch_add_message(SSH2_MSG_NEWKEYS);
	SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX: Tru64 UNIX workaround   (2005.3.5 yutaka)
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);

	BN_free(dh_server_pub);
	DH_free(pvar->kexdh); pvar->kexdh = NULL;
	key_free(hostkey);
	if (dh_buf != NULL) free(dh_buf);
	return TRUE;

error:
	BN_free(dh_server_pub);
	DH_free(pvar->kexdh); pvar->kexdh = NULL;
	key_free(hostkey);
	if (dh_buf != NULL) free(dh_buf);
	BN_free(share_key);

	notify_fatal_error(pvar, emsg, TRUE);

	return FALSE;
}


// Diffie-Hellman Key Exchange Reply(SSH2_MSG_KEX_DH_GEX_REPLY:33)
//
// C then computes K = f^x mod p, H = hash(V_C ||
//          V_S || I_C || I_S || K_S || min || n || max || p || g || e ||
//          f || K), and verifies the signature s on H.
static BOOL handle_SSH2_dh_gex_reply(PTInstVar pvar)
{
	char *data;
	int len;
	int offset = 0;
	char *server_host_key_blob;
	int bloblen, siglen;
	BIGNUM *dh_server_pub = NULL;
	char *signature;
	int dh_len, share_len;
	char *dh_buf = NULL;
	BIGNUM *share_key = NULL;
	char *hash;
	char *emsg, emsg_tmp[1024];  // error message
	int ret, hashlen;
	Key *hostkey = NULL;  // hostkey

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_DH_GEX_REPLY was received.");

	memset(&hostkey, 0, sizeof(hostkey));

	// TODO: buffer overrun check

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	// for debug
	push_memdump("DH_GEX_REPLY", "key exchange: receiving", data, len);

	bloblen = get_uint32_MSBfirst(data);
	data += 4;
	server_host_key_blob = data; // for hash

	push_memdump("DH_GEX_REPLY", "server_host_key_blob", server_host_key_blob, bloblen);

	hostkey = key_from_blob(data, bloblen);
	if (hostkey == NULL) {
		emsg = "key_from_blob error @ handle_SSH2_dh_gex_reply()";
		goto error;
	}
	data += bloblen;

	// known_hosts対応 (2006.3.20 yutaka)
	if (hostkey->type != pvar->hostkey_type) {  // ホストキーの種別比較
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
		            "type mismatch for decoded server_host_key_blob (kex:%s blob:%s) @ %s",
		            get_ssh_keytype_name(pvar->hostkey_type), get_ssh_keytype_name(hostkey->type), __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	HOSTS_check_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, hostkey);
	if (pvar->socket == INVALID_SOCKET) {
		emsg = "Server disconnected @ handle_SSH2_dh_gex_reply()";
		goto error;
	}

	dh_server_pub = BN_new();
	if (dh_server_pub == NULL) {
		emsg = "Out of memory1 @ handle_SSH2_dh_gex_reply()";
		goto error;
	}

	buffer_get_bignum2(&data, dh_server_pub);

	siglen = get_uint32_MSBfirst(data);
	data += 4;
	signature = data;
	data += siglen;

	push_memdump("DH_GEX_REPLY", "signature", signature, siglen);

	// check DH public value
	if (!dh_pub_is_valid(pvar->kexdh, dh_server_pub)) {
		emsg = "DH public value invalid @ handle_SSH2_dh_gex_reply()";
		goto error;
	}
	// 共通鍵の生成
	dh_len = DH_size(pvar->kexdh);
	dh_buf = malloc(dh_len);
	if (dh_buf == NULL) {
		emsg = "Out of memory2 @ handle_SSH2_dh_gex_reply()";
		goto error;
	}
	share_len = DH_compute_key(dh_buf, dh_server_pub, pvar->kexdh);
	share_key = BN_new();
	if (share_key == NULL) {
		emsg = "Out of memory3 @ handle_SSH2_dh_gex_reply()";
		goto error;
	}
	// 'share_key'がサーバとクライアントで共有する鍵（G^A×B mod P）となる。
	BN_bin2bn(dh_buf, share_len, share_key);
	//debug_print(40, dh_buf, share_len);

	// ハッシュの計算
	/* calc and verify H */
	hash = kex_dh_gex_hash(
		get_kex_algorithm_EVP_MD(pvar->kex_type),
		pvar->client_version_string,
		pvar->server_version_string,
		buffer_ptr(pvar->my_kex),  buffer_len(pvar->my_kex),
		buffer_ptr(pvar->peer_kex),  buffer_len(pvar->peer_kex),
		server_host_key_blob, bloblen,
		/////// KEXGEX
		pvar->kexgex_min,
		pvar->kexgex_bits,
		pvar->kexgex_max,
		pvar->kexdh->p,
		pvar->kexdh->g,
		pvar->kexdh->pub_key,
		/////// KEXGEX
		dh_server_pub,
		share_key,
		&hashlen);

	{
		push_memdump("DH_GEX_REPLY kex_dh_gex_hash", "my_kex", buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex));
		push_memdump("DH_GEX_REPLY kex_dh_gex_hash", "peer_kex", buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex));

		push_bignum_memdump("DH_GEX_REPLY kex_dh_gex_hash", "dh_server_pub", dh_server_pub);
		push_bignum_memdump("DH_GEX_REPLY kex_dh_gex_hash", "share_key", share_key);

		push_memdump("DH_GEX_REPLY kex_dh_gex_hash", "hash", hash, hashlen);
	}

	//debug_print(30, hash, hashlen);
	//debug_print(31, pvar->client_version_string, strlen(pvar->client_version_string));
	//debug_print(32, pvar->server_version_string, strlen(pvar->server_version_string));
	//debug_print(33, buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex));
	//debug_print(34, buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex));
	//debug_print(35, server_host_key_blob, bloblen);

	// session idの保存（初回接続時のみ）
	if (pvar->session_id == NULL) {
		pvar->session_id_len = hashlen;
		pvar->session_id = malloc(pvar->session_id_len);
		if (pvar->session_id != NULL) {
			memcpy(pvar->session_id, hash, pvar->session_id_len);
		} else {
			// TODO:
		}
	}

	if ((ret = key_verify(hostkey, signature, siglen, hash, hashlen)) != 1) {
		if (ret == -3 && hostkey->type == KEY_RSA) {
			if (!pvar->settings.EnableRsaShortKeyServer) {
				_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
				            "key verify error(remote rsa key length is too short %d-bit) "
				            "@ handle_SSH2_dh_gex_reply()", BN_num_bits(hostkey->rsa->n));
			}
			else {
				goto cont;
			}
		}
		else {
			_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
			            "key verify error(%d) @ handle_SSH2_dh_gex_reply()\r\n%s", ret, SENDTOME);
		}
		emsg = emsg_tmp;
		save_memdump(LOGDUMP);
		goto error;
	}

cont:
	kex_derive_keys(pvar, pvar->we_need, hash, share_key, pvar->session_id, pvar->session_id_len);

	// KEX finish
	begin_send_packet(pvar, SSH2_MSG_NEWKEYS, 0);
	finish_send_packet(pvar);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_NEWKEYS was sent at handle_SSH2_dh_gex_reply().");

	// SSH2_MSG_NEWKEYSを送り終わったあとにキーの設定および再設定を行う
	// 送信用の暗号鍵は SSH2_MSG_NEWKEYS の送信後に、受信用のは SSH2_MSG_NEWKEYS の
	// 受信後に再設定を行う。
	if (pvar->rekeying == 1) { // キーの再設定
		// まず、送信用だけ設定する。
		ssh2_set_newkeys(pvar, MODE_OUT);
		pvar->ssh2_keys[MODE_OUT].mac.enabled = 1;
		pvar->ssh2_keys[MODE_OUT].comp.enabled = 1;
		enable_send_compression(pvar);
		if (!CRYPT_start_encryption(pvar, 1, 0)) {
			// TODO: error
		}

	} else {
		// 初回接続の場合は実際に暗号ルーチンが設定されるのは、あとになってから
		// なので（CRYPT_start_encryption関数）、ここで鍵の設定をしてしまってもよい。
		ssh2_set_newkeys(pvar, MODE_IN);
		ssh2_set_newkeys(pvar, MODE_OUT);

		// SSH2_MSG_NEWKEYSを送信した時点で、MACを有効にする。(2006.10.30 yutaka)
		pvar->ssh2_keys[MODE_OUT].mac.enabled = 1;
		pvar->ssh2_keys[MODE_OUT].comp.enabled = 1;

		// パケット圧縮が有効なら初期化する。(2005.7.9 yutaka)
		// SSH2_MSG_NEWKEYSの受信より前なのでここだけでよい。(2006.10.30 maya)
		prep_compression(pvar);
		enable_compression(pvar);
	}

	// TTSSHバージョン情報に表示するキービット数を求めておく (2004.10.30 yutaka)
	pvar->client_key_bits = BN_num_bits(pvar->kexdh->pub_key);
	pvar->server_key_bits = BN_num_bits(dh_server_pub);

	SSH2_dispatch_init(3);
	SSH2_dispatch_add_message(SSH2_MSG_NEWKEYS);
	SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX: Tru64 UNIX workaround   (2005.3.5 yutaka)
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);

	BN_free(dh_server_pub);
	DH_free(pvar->kexdh); pvar->kexdh = NULL;
	key_free(hostkey);
	if (dh_buf != NULL) free(dh_buf);
	return TRUE;

error:
	BN_free(dh_server_pub);
	DH_free(pvar->kexdh); pvar->kexdh = NULL;
	key_free(hostkey);
	if (dh_buf != NULL) free(dh_buf);
	BN_free(share_key);

	notify_fatal_error(pvar, emsg, TRUE);

	return FALSE;
}


//
// Elliptic Curv Diffie-Hellman Key Exchange Reply(SSH2_MSG_KEX_ECDH_REPLY:31)
//
static BOOL handle_SSH2_ecdh_kex_reply(PTInstVar pvar)
{
	char *data;
	int len;
	int offset = 0;
	char *server_host_key_blob;
	int bloblen, siglen;
	EC_POINT *server_public = NULL;
	const EC_GROUP *group;
	char *signature;
	int ecdh_len;
	char *ecdh_buf = NULL;
	BIGNUM *share_key = NULL;
	char *hash;
	char *emsg, emsg_tmp[1024];  // error message
	int ret, hashlen;
	Key *hostkey = NULL;  // hostkey

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_ECDH_REPLY was received.");

	memset(&hostkey, 0, sizeof(hostkey));

	// TODO: buffer overrun check

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	// for debug
	push_memdump("KEX_ECDH_REPLY", "key exchange: receiving", data, len);

	bloblen = get_uint32_MSBfirst(data);
	data += 4;
	server_host_key_blob = data; // for hash

	push_memdump("KEX_ECDH_REPLY", "server_host_key_blob", server_host_key_blob, bloblen);

	hostkey = key_from_blob(data, bloblen);
	if (hostkey == NULL) {
		emsg = "key_from_blob error @ handle_SSH2_ecdh_kex_reply()";
		goto error;
	}
	data += bloblen;

	// known_hosts対応 (2006.3.20 yutaka)
	if (hostkey->type != pvar->hostkey_type) {  // ホストキーの種別比較
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
		            "type mismatch for decoded server_host_key_blob (kex:%s blob:%s) @ %s",
		            get_ssh_keytype_name(pvar->hostkey_type), get_ssh_keytype_name(hostkey->type), __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	HOSTS_check_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, hostkey);
	if (pvar->socket == INVALID_SOCKET) {
		emsg = "Server disconnected @ handle_SSH2_ecdh_kex_reply()";
		goto error;
	}

	/* Q_S, server public key */
	group = EC_KEY_get0_group(pvar->ecdh_client_key);
	server_public = EC_POINT_new(group);
	if (server_public == NULL) {
		emsg = "Out of memory1 @ handle_SSH2_ecdh_kex_reply()";
		goto error;
	}

	buffer_get_ecpoint(&data, group, server_public);

	siglen = get_uint32_MSBfirst(data);
	data += 4;
	signature = data;
	data += siglen;

	push_memdump("KEX_ECDH_REPLY", "signature", signature, siglen);

	// check public key
	if (key_ec_validate_public(group, server_public) != 0) {
		emsg = "ECDH invalid server public key @ handle_SSH2_ecdh_kex_reply()";
		goto error;
	}
	// 共通鍵の生成
	ecdh_len = (EC_GROUP_get_degree(group) + 7) / 8;
	ecdh_buf = malloc(ecdh_len);
	if (ecdh_buf == NULL) {
		emsg = "Out of memory2 @ handle_SSH2_ecdh_kex_reply()";
		goto error;
	}
	if (ECDH_compute_key(ecdh_buf, ecdh_len, server_public,
	                     pvar->ecdh_client_key, NULL) != (int)ecdh_len) {
		emsg = "Out of memory3 @ handle_SSH2_ecdh_kex_reply()";
		goto error;
	}
	share_key = BN_new();
	if (share_key == NULL) {
		emsg = "Out of memory4 @ handle_SSH2_ecdh_kex_reply()";
		goto error;
	}
	// 'share_key'がサーバとクライアントで共有する鍵（G^A×B mod P）となる。
	BN_bin2bn(ecdh_buf, ecdh_len, share_key);
	//debug_print(40, ecdh_buf, ecdh_len);

	// ハッシュの計算
	/* calc and verify H */
	hash = kex_ecdh_hash(get_kex_algorithm_EVP_MD(pvar->kex_type),
	                     group,
	                     pvar->client_version_string,
	                     pvar->server_version_string,
	                     buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex),
	                     buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex),
	                     server_host_key_blob, bloblen,
	                     EC_KEY_get0_public_key(pvar->ecdh_client_key),
	                     server_public,
	                     share_key,
	                     &hashlen);

	{
		push_memdump("KEX_ECDH_REPLY ecdh_kex_reply", "my_kex", buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex));
		push_memdump("KEX_ECDH_REPLY ecdh_kex_reply", "peer_kex", buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex));

		push_bignum_memdump("KEX_ECDH_REPLY ecdh_kex_reply", "share_key", share_key);

		push_memdump("KEX_ECDH_REPLY ecdh_kex_reply", "hash", hash, hashlen);
	}

	//debug_print(30, hash, hashlen);
	//debug_print(31, pvar->client_version_string, strlen(pvar->client_version_string));
	//debug_print(32, pvar->server_version_string, strlen(pvar->server_version_string));
	//debug_print(33, buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex));
	//debug_print(34, buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex));
	//debug_print(35, server_host_key_blob, bloblen);

	// session idの保存（初回接続時のみ）
	if (pvar->session_id == NULL) {
		pvar->session_id_len = hashlen;
		pvar->session_id = malloc(pvar->session_id_len);
		if (pvar->session_id != NULL) {
			memcpy(pvar->session_id, hash, pvar->session_id_len);
		} else {
			// TODO:
		}
	}

	if ((ret = key_verify(hostkey, signature, siglen, hash, hashlen)) != 1) {
		if (ret == -3 && hostkey->type == KEY_RSA) {
			if (!pvar->settings.EnableRsaShortKeyServer) {
				_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
				            "key verify error(remote rsa key length is too short %d-bit) "
				            "@ handle_SSH2_ecdh_kex_reply()", BN_num_bits(hostkey->rsa->n));
			}
			else {
				goto cont;
			}
		}
		else {
			_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
			            "key verify error(%d) @ handle_SSH2_ecdh_kex_reply()\r\n%s", ret, SENDTOME);
		}
		emsg = emsg_tmp;
		save_memdump(LOGDUMP);
		goto error;
	}

cont:
	kex_derive_keys(pvar, pvar->we_need, hash, share_key, pvar->session_id, pvar->session_id_len);

	// KEX finish
	begin_send_packet(pvar, SSH2_MSG_NEWKEYS, 0);
	finish_send_packet(pvar);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_NEWKEYS was sent at handle_SSH2_ecdh_kex_reply().");

	// SSH2_MSG_NEWKEYSを送り終わったあとにキーの設定および再設定を行う
	// 送信用の暗号鍵は SSH2_MSG_NEWKEYS の送信後に、受信用のは SSH2_MSG_NEWKEYS の
	// 受信後に再設定を行う。
	if (pvar->rekeying == 1) { // キーの再設定
		// まず、送信用だけ設定する。
		ssh2_set_newkeys(pvar, MODE_OUT);
		pvar->ssh2_keys[MODE_OUT].mac.enabled = 1;
		pvar->ssh2_keys[MODE_OUT].comp.enabled = 1;
		enable_send_compression(pvar);
		if (!CRYPT_start_encryption(pvar, 1, 0)) {
			// TODO: error
		}

	} else {
		// 初回接続の場合は実際に暗号ルーチンが設定されるのは、あとになってから
		// なので（CRYPT_start_encryption関数）、ここで鍵の設定をしてしまってもよい。
		ssh2_set_newkeys(pvar, MODE_IN);
		ssh2_set_newkeys(pvar, MODE_OUT);

		// SSH2_MSG_NEWKEYSを送信した時点で、MACを有効にする。(2006.10.30 yutaka)
		pvar->ssh2_keys[MODE_OUT].mac.enabled = 1;
		pvar->ssh2_keys[MODE_OUT].comp.enabled = 1;

		// パケット圧縮が有効なら初期化する。(2005.7.9 yutaka)
		// SSH2_MSG_NEWKEYSの受信より前なのでここだけでよい。(2006.10.30 maya)
		prep_compression(pvar);
		enable_compression(pvar);
	}

	// TTSSHバージョン情報に表示するキービット数を求めておく
	switch (pvar->kex_type) {
		case KEX_ECDH_SHA2_256:
			pvar->client_key_bits = 256;
			pvar->server_key_bits = 256;
			break;
		case KEX_ECDH_SHA2_384:
			pvar->client_key_bits = 384;
			pvar->server_key_bits = 384;
			break;
		case KEX_ECDH_SHA2_521:
			pvar->client_key_bits = 521;
			pvar->server_key_bits = 521;
			break;
		default:
			// TODO
			break;
	}

	SSH2_dispatch_init(3);
	SSH2_dispatch_add_message(SSH2_MSG_NEWKEYS);
	SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX: Tru64 UNIX workaround   (2005.3.5 yutaka)
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);

	EC_KEY_free(pvar->ecdh_client_key); pvar->ecdh_client_key = NULL;
	EC_POINT_clear_free(server_public);
	key_free(hostkey);
	if (ecdh_buf != NULL) free(ecdh_buf);
	return TRUE;

error:
	EC_POINT_clear_free(server_public);
	EC_KEY_free(pvar->ecdh_client_key); pvar->ecdh_client_key = NULL;
	key_free(hostkey);
	if (ecdh_buf != NULL) free(ecdh_buf);
	BN_free(share_key);

	notify_fatal_error(pvar, emsg, TRUE);

	return FALSE;
}


// KEXにおいてサーバから返ってくる 31 番メッセージに対するハンドラ
static BOOL handle_SSH2_dh_common_reply(PTInstVar pvar)
{
	switch (pvar->kex_type) {
		case KEX_DH_GRP1_SHA1:
		case KEX_DH_GRP14_SHA1:
		case KEX_DH_GRP14_SHA256:
		case KEX_DH_GRP16_SHA512:
		case KEX_DH_GRP18_SHA512:
			handle_SSH2_dh_kex_reply(pvar);
			break;
		case KEX_DH_GEX_SHA1:
		case KEX_DH_GEX_SHA256:
			handle_SSH2_dh_gex_group(pvar);
			break;
		case KEX_ECDH_SHA2_256:
		case KEX_ECDH_SHA2_384:
		case KEX_ECDH_SHA2_521:
			handle_SSH2_ecdh_kex_reply(pvar);
			break;
		default:
			// TODO
			break;
	}

	return TRUE;
}


static void do_SSH2_dispatch_setup_for_transfer(PTInstVar pvar)
{
	// キー再作成情報のクリア (2005.6.19 yutaka)
	pvar->rekeying = 0;

	SSH2_dispatch_init(6);
	SSH2_dispatch_add_range_message(SSH2_MSG_GLOBAL_REQUEST, SSH2_MSG_CHANNEL_FAILURE);
	SSH2_dispatch_add_message(SSH2_MSG_UNIMPLEMENTED);
	SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX
	// OpenSSH 3.9ではデータ通信中のDH鍵交換要求が、サーバから送られてくることがある。
	SSH2_dispatch_add_message(SSH2_MSG_KEXINIT);
	// HP-UXでX11 forwardingが失敗した場合、下記のメッセージが送られてくる。(2006.4.7 yutaka)
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);
	// OpenSSH はデータ通信中に致命的なエラーがあると SSH2_MSG_DISCONNECT を送ってくる (2007.10.25 maya)
	SSH2_dispatch_add_message(SSH2_MSG_DISCONNECT);
}


static BOOL handle_SSH2_newkeys(PTInstVar pvar)
{
	int supported_ciphers = (1 << SSH2_CIPHER_3DES_CBC
	                       | 1 << SSH2_CIPHER_AES128_CBC
	                       | 1 << SSH2_CIPHER_AES192_CBC
	                       | 1 << SSH2_CIPHER_AES256_CBC
	                       | 1 << SSH2_CIPHER_BLOWFISH_CBC
	                       | 1 << SSH2_CIPHER_AES128_CTR
	                       | 1 << SSH2_CIPHER_AES192_CTR
	                       | 1 << SSH2_CIPHER_AES256_CTR
	                       | 1 << SSH2_CIPHER_ARCFOUR
	                       | 1 << SSH2_CIPHER_ARCFOUR128
	                       | 1 << SSH2_CIPHER_ARCFOUR256
	                       | 1 << SSH2_CIPHER_CAST128_CBC
	                       | 1 << SSH2_CIPHER_3DES_CTR
	                       | 1 << SSH2_CIPHER_BLOWFISH_CTR
	                       | 1 << SSH2_CIPHER_CAST128_CTR
	                       | 1 << SSH2_CIPHER_CAMELLIA128_CBC
	                       | 1 << SSH2_CIPHER_CAMELLIA192_CBC
	                       | 1 << SSH2_CIPHER_CAMELLIA256_CBC
	                       | 1 << SSH2_CIPHER_CAMELLIA128_CTR
	                       | 1 << SSH2_CIPHER_CAMELLIA192_CTR
	                       | 1 << SSH2_CIPHER_CAMELLIA256_CTR
	);
	int type = (1 << SSH_AUTH_PASSWORD) | (1 << SSH_AUTH_RSA) |
	           (1 << SSH_AUTH_TIS) | (1 << SSH_AUTH_PAGEANT);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_NEWKEYS was received(DH key generation is completed).");

	// ログ採取の終了 (2005.3.7 yutaka)
	if (LogLevel(pvar, LOG_LEVEL_SSHDUMP)) {
		save_memdump(LOGDUMP);
	}
	finish_memdump();

	// finish key exchange
	pvar->key_done = 1;

	// キー再作成なら認証はパスする。
	if (pvar->rekeying == 1) {
		// かつ、受信用の暗号鍵の再設定をここで行う。
		ssh2_set_newkeys(pvar, MODE_IN);
		pvar->ssh2_keys[MODE_IN].mac.enabled = 1;
		pvar->ssh2_keys[MODE_IN].comp.enabled = 1;
		enable_recv_compression(pvar);
		if (!CRYPT_start_encryption(pvar, 0, 1)) {
			// TODO: error
		}
		do_SSH2_dispatch_setup_for_transfer(pvar);
		return TRUE;

	} else {
		// SSH2_MSG_NEWKEYSを受け取った時点で、MACを有効にする。(2006.10.30 yutaka)
		pvar->ssh2_keys[MODE_IN].mac.enabled = 1;
		pvar->ssh2_keys[MODE_IN].comp.enabled = 1;

	}

	// 暗号アルゴリズムの設定
	if (!CRYPT_set_supported_ciphers(pvar, supported_ciphers, supported_ciphers))
		return FALSE;
	if (!AUTH_set_supported_auth_types(pvar, type))
		return FALSE;

	SSH_notify_host_OK(pvar);


	return TRUE;
}

// ユーザ認証の開始
BOOL do_SSH2_userauth(PTInstVar pvar)
{
	buffer_t *msg;
	char *s;
	unsigned char *outmsg;
	int len;

	// パスワードが入力されたら 1 を立てる (2005.3.12 yutaka)
	pvar->keyboard_interactive_password_input = 0;

	// すでにログイン処理を行っている場合は、SSH2_MSG_SERVICE_REQUESTの送信は
	// しないことにする。OpenSSHでは支障ないが、Tru64 UNIXではサーバエラーとなってしまうため。
	// (2005.3.10 yutaka)
	// Cisco 12.4.11T でもこの現象が発生する模様。
	// 認証メソッド none の時点で SSH2_MSG_SERVICE_REQUEST を送信している。
	// (2007.10.26 maya)
	if (pvar->userauth_retry_count > 0
	 || pvar->tryed_ssh2_authlist == TRUE) {
		return do_SSH2_authrequest(pvar);
		/* NOT REACHED */
	}

	// start user authentication
	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
		return FALSE;
	}
	s = "ssh-userauth";
	buffer_put_string(msg, s, strlen(s));
	//buffer_put_padding(msg, 32); // XXX:
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_SERVICE_REQUEST, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);

	SSH2_dispatch_init(4);
	SSH2_dispatch_add_message(SSH2_MSG_SERVICE_ACCEPT);
	SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX: Tru64 UNIX workaround   (2005.3.5 yutaka)
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_SERVICE_REQUEST was sent at do_SSH2_userauth().");

	return TRUE;
}


static BOOL handle_SSH2_service_accept(PTInstVar pvar)
{
	char *data, *svc;

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;

	if ((svc = buffer_get_string(&data, NULL)) == NULL) {
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_get_string returns NULL.");
	}
	logprintf(LOG_LEVEL_VERBOSE, "SSH2_MSG_SERVICE_ACCEPT was received. service-name=%s", NonNull(svc));
	free(svc);

	SSH2_dispatch_init(5);
	SSH2_dispatch_add_message(SSH2_MSG_IGNORE); // XXX: Tru64 UNIX workaround   (2005.3.5 yutaka)
	if (pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) {
		// keyboard-interactive method
		SSH2_dispatch_add_message(SSH2_MSG_USERAUTH_INFO_REQUEST);
	}
	else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
		// Pageant
		SSH2_dispatch_add_message(SSH2_MSG_USERAUTH_PK_OK);
	}
	else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PASSWORD) {
		SSH2_dispatch_add_message(SSH2_MSG_USERAUTH_PASSWD_CHANGEREQ);
	}
	SSH2_dispatch_add_message(SSH2_MSG_USERAUTH_SUCCESS);
	SSH2_dispatch_add_message(SSH2_MSG_USERAUTH_FAILURE);
	SSH2_dispatch_add_message(SSH2_MSG_USERAUTH_BANNER);
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);  // support for authorized_keys command (2006.2.23 yutaka)

	return do_SSH2_authrequest(pvar);
}

// ユーザ認証パケットの構築
BOOL do_SSH2_authrequest(PTInstVar pvar)
{
	buffer_t *msg = NULL;
	char *s, *username;
	unsigned char *outmsg;
	int len;
	char *connect_id = "ssh-connection";

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
		return FALSE;
	}

	// ペイロードの構築
	username = pvar->auth_state.user;  // ユーザ名
	buffer_put_string(msg, username, strlen(username));

	if (!pvar->tryed_ssh2_authlist) { // "none"メソッドの送信
		// 認証リストをサーバから取得する。
		// SSH2_MSG_USERAUTH_FAILUREが返るが、サーバにはログは残らない。
		// (2007.4.27 yutaka)
		s = connect_id;
		buffer_put_string(msg, s, strlen(s));
		s = "none";  // method name
		buffer_put_string(msg, s, strlen(s));

	} else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PASSWORD) { // パスワード認証
		// password authentication method
		s = connect_id;
		buffer_put_string(msg, s, strlen(s));
		s = "password";
		buffer_put_string(msg, s, strlen(s));
		buffer_put_char(msg, 0); // 0

		if (pvar->ssh2_autologin == 1) { // SSH2自動ログイン
			s = pvar->ssh2_password;
		} else {
			s = pvar->auth_state.cur_cred.password;  // パスワード
		}
		buffer_put_string(msg, s, strlen(s));

	} else if (pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) { // keyboard-interactive (2005.3.12 yutaka)
		//s = "ssh-userauth";  // service name
		s = connect_id;
		buffer_put_string(msg, s, strlen(s));
		s = "keyboard-interactive";  // method name
		buffer_put_string(msg, s, strlen(s));
		s = "";  // language tag
		buffer_put_string(msg, s, strlen(s));
		s = "";  // submethods
		buffer_put_string(msg, s, strlen(s));

		SSH2_dispatch_add_message(SSH2_MSG_USERAUTH_INFO_REQUEST);

	} else if (pvar->auth_state.cur_cred.method == SSH_AUTH_RSA) { // 公開鍵認証
		buffer_t *signbuf = NULL;
		buffer_t *blob = NULL;
		int bloblen;
		char *signature = NULL;
		int siglen;
		Key *keypair = pvar->auth_state.cur_cred.key_pair;

		if (get_SSH2_publickey_blob(pvar, &blob, &bloblen) == FALSE) {
			goto error;
		}

		// step1
		signbuf = buffer_init();
		if (signbuf == NULL) {
			buffer_free(blob);
			goto error;
		}
		// セッションID
		buffer_append_length(signbuf, pvar->session_id, pvar->session_id_len);
		buffer_put_char(signbuf, SSH2_MSG_USERAUTH_REQUEST);
		s = username;  // ユーザ名
		buffer_put_string(signbuf, s, strlen(s));
		s = connect_id;
		buffer_put_string(signbuf, s, strlen(s));
		s = "publickey";
		buffer_put_string(signbuf, s, strlen(s));
		buffer_put_char(signbuf, 1); // true
		s = get_sshname_from_key(keypair); // key typeに応じた文字列を得る
		buffer_put_string(signbuf, s, strlen(s));
		s = buffer_ptr(blob);
		buffer_append_length(signbuf, s, bloblen);

		// 署名の作成
		if ( generate_SSH2_keysign(keypair, &signature, &siglen, buffer_ptr(signbuf), buffer_len(signbuf)) == FALSE) {
			buffer_free(blob);
			buffer_free(signbuf);
			goto error;
		}

		// step3
		s = connect_id;
		buffer_put_string(msg, s, strlen(s));
		s = "publickey";
		buffer_put_string(msg, s, strlen(s));
		buffer_put_char(msg, 1); // true
		s = get_sshname_from_key(keypair); // key typeに応じた文字列を得る
		buffer_put_string(msg, s, strlen(s));
		s = buffer_ptr(blob);
		buffer_append_length(msg, s, bloblen);
		buffer_append_length(msg, signature, siglen);


		buffer_free(blob);
		buffer_free(signbuf);
		free(signature);

	} else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) { // Pageant
		unsigned char *puttykey;

		s = connect_id;
		buffer_put_string(msg, s, strlen(s));
		s = "publickey";
		buffer_put_string(msg, s, strlen(s));
		buffer_put_char(msg, 0); // false

		if (pvar->pageant_keycurrent != 0) {
			// 直前の鍵をスキップ
			len = get_uint32_MSBfirst(pvar->pageant_curkey);
			pvar->pageant_curkey += 4 + len;
			// 直前の鍵のコメントをスキップ
			len = get_uint32_MSBfirst(pvar->pageant_curkey);
			pvar->pageant_curkey += 4 + len;
			// 次の鍵の位置へ来る
		}
		puttykey = pvar->pageant_curkey;

		// アルゴリズムをコピーする
		len = get_uint32_MSBfirst(puttykey+4);
		buffer_put_string(msg, puttykey+8, len);

		// 鍵をコピーする
		len = get_uint32_MSBfirst(puttykey);
		puttykey += 4;
		buffer_put_string(msg, puttykey, len);
		puttykey += len;

		pvar->pageant_keycurrent++;

		SSH2_dispatch_add_message(SSH2_MSG_USERAUTH_PK_OK);

	} else {
		goto error;

	}

	// パケット送信
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_USERAUTH_REQUEST, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);

	{
		logprintf(LOG_LEVEL_VERBOSE,
			"SSH2_MSG_USERAUTH_REQUEST was sent do_SSH2_authrequest(). (method %d)",
			pvar->auth_state.cur_cred.method);
	}

	return TRUE;

error:
	buffer_free(msg);

	return FALSE;
}


//
// SSH2 heartbeat procedure
//
// NAT環境において、SSHクライアントとサーバ間で通信が発生しなかった場合、
// ルータが勝手にNATテーブルをクリアすることがあり、SSHコネクションが
// 切れてしまうことがある。定期的に、クライアントからダミーパケットを
// 送信することで対処する。(2004.12.10 yutaka)
//
// モードレスダイアログからパケット送信するように変更。(2007.12.26 yutaka)
//
#define WM_SEND_HEARTBEAT (WM_USER + 1)

static LRESULT CALLBACK ssh_heartbeat_dlg_proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{

	switch (msg) {
		case WM_INITDIALOG:
			return FALSE;

		case WM_SEND_HEARTBEAT:
			{
			PTInstVar pvar = (PTInstVar)wp;
			buffer_t *msg;
			char *s;
			unsigned char *outmsg;
			int len;

			msg = buffer_init();
			if (msg == NULL) {
				// TODO: error check
				logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
				return FALSE;
			}
			s = "ssh-heartbeat";
			buffer_put_string(msg, s, strlen(s));
			len = buffer_len(msg);
			if (SSHv1(pvar)) {
				outmsg = begin_send_packet(pvar, SSH_MSG_IGNORE, len);
			} else {
				outmsg = begin_send_packet(pvar, SSH2_MSG_IGNORE, len);
			}
			memcpy(outmsg, buffer_ptr(msg), len);
			finish_send_packet(pvar);
			buffer_free(msg);
			if (SSHv1(pvar)) {
				logputs(LOG_LEVEL_SSHDUMP, "SSH_MSG_IGNORE was sent at ssh_heartbeat_dlg_proc().");
			} else {
				logputs(LOG_LEVEL_SSHDUMP, "SSH2_MSG_IGNORE was sent at ssh_heartbeat_dlg_proc().");
			}
			}
			return TRUE;
			break;

		case WM_COMMAND:
			switch (wp) {
			}

			switch (LOWORD(wp)) {
				case IDOK:
					{
					return TRUE;
					}

				case IDCANCEL:
					EndDialog(hWnd, 0);
					return TRUE;
				default:
					return FALSE;
			}
			break;

		case WM_CLOSE:
			// closeボタンが押下されても window が閉じないようにする。
			return TRUE;

		case WM_DESTROY:
			return TRUE;

		default:
			return FALSE;
	}
	return TRUE;
}


static unsigned __stdcall ssh_heartbeat_thread(void *p)
{
	static int instance = 0;
	PTInstVar pvar = (PTInstVar)p;
	time_t tick;

	// すでに実行中なら何もせずに返る。
	if (instance > 0)
		return 0;
	instance++;

	for (;;) {
		// ソケットがクローズされたらスレッドを終わる
		if (pvar->socket == INVALID_SOCKET)
			break;

		// 一定時間無通信であれば、サーバへダミーパケットを送る
		// 閾値が0であれば何もしない。
		tick = time(NULL) - pvar->ssh_heartbeat_tick;
		if (pvar->session_settings.ssh_heartbeat_overtime > 0 &&
			tick > pvar->session_settings.ssh_heartbeat_overtime) {

			SendMessage(pvar->ssh_hearbeat_dialog, WM_SEND_HEARTBEAT, (WPARAM)pvar, 0);
		}

		Sleep(100); // yield
	}

	instance = 0;

	return 0;
}

static void start_ssh_heartbeat_thread(PTInstVar pvar)
{
	HANDLE thread = (HANDLE)-1;
	unsigned tid;
	HWND hDlgWnd;

	// モードレスダイアログを作成。ハートビート用なのでダイアログは非表示のままと
	// するので、リソースIDはなんでもよい。
	hDlgWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SSHSCP_PROGRESS),
               pvar->cv->HWin, (DLGPROC)ssh_heartbeat_dlg_proc);
	pvar->ssh_hearbeat_dialog = hDlgWnd;

	// TTSSHは thread-safe ではないのでスレッド内からのパケット送信は不可。(2007.12.26 yutaka)
	thread = (HANDLE)_beginthreadex(NULL, 0, ssh_heartbeat_thread, pvar, 0, &tid);
	if (thread == (HANDLE)-1) {
		// TODO:
	}
	pvar->ssh_heartbeat_thread = thread;
}

// スレッドの停止 (2004.12.27 yutaka)
void halt_ssh_heartbeat_thread(PTInstVar pvar)
{
	if (pvar->ssh_heartbeat_thread != (HANDLE)-1L) {
		WaitForSingleObject(pvar->ssh_heartbeat_thread, INFINITE);
		CloseHandle(pvar->ssh_heartbeat_thread);
		pvar->ssh_heartbeat_thread = (HANDLE)-1L;

		DestroyWindow(pvar->ssh_hearbeat_dialog);
	}
}


static BOOL handle_SSH2_userauth_success(PTInstVar pvar)
{
	buffer_t *msg;
	char *s;
	unsigned char *outmsg;
	int len;
	Channel_t *c;

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_USERAUTH_SUCCESS was received.");

	// パスワードの破棄 (2006.8.22 yutaka)
	if (pvar->settings.remember_password == 0) {
		destroy_malloced_string(&pvar->auth_state.cur_cred.password);
	}

	// 認証OK
	pvar->userauth_success = 1;

	// チャネル設定
	// FWD_prep_forwarding()でshell IDを使うので、先に設定を持ってくる。(2005.7.3 yutaka)
	// changed window size from 64KB to 32KB. (2006.3.6 yutaka)
	// changed window size from 32KB to 128KB. (2007.10.29 maya)
	if (pvar->use_subsystem) {
		c = ssh2_channel_new(CHAN_SES_WINDOW_DEFAULT, CHAN_SES_PACKET_DEFAULT, TYPE_SUBSYSTEM_GEN, -1);
	}
	else {
		c = ssh2_channel_new(CHAN_SES_WINDOW_DEFAULT, CHAN_SES_PACKET_DEFAULT, TYPE_SHELL, -1);
	}

	if (c == NULL) {
		UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
		                  "Could not open new channel. TTSSH is already opening too many channels.");
		notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
		return FALSE;
	}
	// シェルのIDを取っておく
	pvar->shell_id = c->self_id;

	// ディスパッチルーチンの再設定
	do_SSH2_dispatch_setup_for_transfer(pvar);

	// シェルオープン
	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
		return FALSE;
	}
	s = "session";
	buffer_put_string(msg, s, strlen(s));  // ctype
	buffer_put_int(msg, c->self_id);  // self(channel number)
	buffer_put_int(msg, c->local_window);  // local_window
	buffer_put_int(msg, c->local_maxpacket);  // local_maxpacket
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_OPEN, len);
	memcpy(outmsg, buffer_ptr (msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_OPEN was sent at handle_SSH2_userauth_success().");

	// ハートビート・スレッドの開始 (2004.12.11 yutaka)
	start_ssh_heartbeat_thread(pvar);

	logputs(LOG_LEVEL_VERBOSE, "User authentication is successful and SSH heartbeat thread is starting.");

	return TRUE;
}


static BOOL handle_SSH2_userauth_failure(PTInstVar pvar)
{
	int len;
	char *data;
	char *cstring;
	int partial;

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_USERAUTH_FAILURE was received.");

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	cstring = buffer_get_string(&data, NULL); // 認証リストの取得
	partial = data[0];
	data += 1;

	// 有効な認証方式がない場合
	if (cstring == NULL) {
		UTIL_get_lang_msg("MSG_SSH_SERVER_NO_AUTH_METHOD_ERROR", pvar,
		                  "The server doesn't have valid authentication method.");
		notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
		return FALSE;
	}

	// tryed_ssh2_authlist が FALSE の場合は、まだ認証を試行をしていない。
	if (!pvar->tryed_ssh2_authlist) {
		int type = 0;

		pvar->tryed_ssh2_authlist = TRUE;

		// 認証ダイアログのラジオボタンを更新
		if (strstr(cstring, "password")) {
			type |= (1 << SSH_AUTH_PASSWORD);
		}
		if (strstr(cstring, "publickey")) {
			type |= (1 << SSH_AUTH_RSA);
			type |= (1 << SSH_AUTH_PAGEANT);
		}
		if (strstr(cstring, "keyboard-interactive")) {
			type |= (1 << SSH_AUTH_TIS);
		}
		if (!AUTH_set_supported_auth_types(pvar, type))
			return FALSE;

		pvar->ssh2_authlist = cstring; // 不要になったらフリーすること
		logprintf(LOG_LEVEL_VERBOSE, "method list from server: %s", cstring);

		if (pvar->ssh2_authmethod == SSH_AUTH_TIS &&
		    pvar->ask4passwd &&
		    pvar->session_settings.CheckAuthListFirst &&
		    pvar->auth_state.auth_dialog != NULL) {
			// challenge で ask4passwd のとき、認証メソッド一覧を自動取得した後
			// 自動的に TIS ダイアログを出すために OK を押す
			SendMessage(pvar->auth_state.auth_dialog, WM_COMMAND, IDOK, 0);
		}
		else {
			// ひとまず none で試行して返ってきたところなので、実際のログイン処理へ
			do_SSH2_authrequest(pvar);
		}

		return TRUE;
	}

	// TCP connection closed
	//notify_closed_connection(pvar);

	// retry countの追加 (2005.3.10 yutaka)
	if (pvar->auth_state.cur_cred.method != SSH_AUTH_PAGEANT) {
		pvar->userauth_retry_count++;
	}
	else {
		if (pvar->pageant_keycount <= pvar->pageant_keycurrent ||
		    pvar->pageant_keyfinal) {
			// 全ての鍵を試し終わった
			// または、TRUE でのログインに失敗してここに来た
			safefree(pvar->pageant_key);
			pvar->userauth_retry_count++;
		}
		else {
			// まだ鍵がある
			do_SSH2_authrequest(pvar);
			return TRUE;
		}
	}

	if (pvar->ssh2_autologin == 1) {
		char uimsg[MAX_UIMSG];
		// SSH2自動ログインが有効の場合は、リトライは行わない。(2004.12.4 yutaka)
		UTIL_get_lang_msg("MSG_SSH_AUTH_FAILURE_ERROR", pvar,
		                  "SSH2 auto-login error: user authentication failed.");
		strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);

		if (pvar->ssh2_authlist != NULL || strlen(pvar->ssh2_authlist) != 0) {
			if ((pvar->auth_state.supported_types & (1 << pvar->ssh2_authmethod)) == 0) {
				// 使用した認証メソッドはサポートされていなかった
				UTIL_get_lang_msg("MSG_SSH_SERVER_UNSUPPORT_AUTH_METHOD_ERROR", pvar,
				                  "\nAuthentication method is not supported by server.");
				strncat_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
			}
		}
		notify_fatal_error(pvar, uimsg, TRUE);
		return TRUE;
	}

	// ユーザ認証に失敗したときは、ユーザ名は固定して、パスワードの再入力を
	// させる。ここの処理は SSH1 と同じ。(2004.10.3 yutaka)
	AUTH_set_generic_mode(pvar);
	AUTH_advance_to_next_cred(pvar);
	pvar->ssh_state.status_flags &= ~STATUS_DONT_SEND_CREDENTIALS;
	try_send_credentials(pvar);

	return TRUE;
}

static BOOL handle_SSH2_userauth_banner(PTInstVar pvar)
{
	//
	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_USERAUTH_BANNER was received.");

	return TRUE;
}


// SSH2 メッセージ 60 番の処理関数
//
// SSH2 では以下のメッセージが 60 番へ重複して割り当てられている。
// 
// * SSH2_MSG_USERAUTH_INFO_REQUEST (keyboard-interactive)
// * SSH2_MSG_USERAUTH_PK_OK (publickey / Tera Term では Pageant 認証のみ)
// * SSH2_MSG_USERAUTH_PASSWD_CHANGEREQ (password)
//
// 現状の実装では同じメッセージ番号が存在できないので、
// 60 番はこの関数で受け、method によって対応するハンドラ関数に振り分ける。
// 
BOOL handle_SSH2_userauth_msg60(PTInstVar pvar)
{
	if (pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) {
		return handle_SSH2_userauth_inforeq(pvar);
	}
	else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
		return handle_SSH2_userauth_pkok(pvar);
	}
	else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PASSWORD) {
		return handle_SSH2_userauth_passwd_changereq(pvar);
	}
	else {
		return FALSE;
	}

	return TRUE; // not reached
}

BOOL handle_SSH2_userauth_inforeq(PTInstVar pvar)
{
	// SSH2_MSG_USERAUTH_INFO_REQUEST
	int len;
	char *data;
	int slen = 0, num, echo;
	char *s, *prompt = NULL;
	buffer_t *msg;
	unsigned char *outmsg;
	int i;
	char *name, *inst, *lang;
	char lprompt[512];

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_USERAUTH_INFO_REQUEST was received.");

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	//debug_print(10, data, len);

	///////// step1
	// get string
	name = buffer_get_string(&data, NULL);
	inst = buffer_get_string(&data, NULL);
	lang = buffer_get_string(&data, NULL);
	lprompt[0] = 0;
	if (inst == NULL) {
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_get_string returns NULL. (inst)");
	}
	else if (strlen(inst) > 0) {
		strncat_s(lprompt, sizeof(lprompt), inst, _TRUNCATE);
		strncat_s(lprompt, sizeof(lprompt), "\r\n", _TRUNCATE);
	}
	if (lang == NULL) {
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_get_string returns NULL. (lang)");
	}
	else if (strlen(lang) > 0) {
		strncat_s(lprompt, sizeof(lprompt), lang, _TRUNCATE);
		strncat_s(lprompt, sizeof(lprompt), "\r\n", _TRUNCATE);
	}

	logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": user=%s, inst=%s, lang=%s",
		NonNull(name), NonNull(inst), NonNull(lang));

	free(name);
	free(inst);
	free(lang);

	// num-prompts
	num = get_uint32_MSBfirst(data);
	data += 4;

	logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": prompts=%d", num);

	///////// step2
	// サーバへパスフレーズを送る
	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
		return FALSE;
	}
	buffer_put_int(msg, num);

	// パスワード変更の場合、メッセージがあれば、表示する。(2010.11.11 yutaka)
	if (num == 0) {
		if (strlen(lprompt) > 0) 
			MessageBox(pvar->cv->HWin, lprompt, "USERAUTH INFO_REQUEST", MB_OK | MB_ICONINFORMATION);
	}

	// プロンプトの数だけ prompt & echo が繰り返される。
	for (i = 0 ; i < num ; i++) {
		// get string
		slen = get_uint32_MSBfirst(data);
		data += 4;
		prompt = data;  // prompt
		data += slen;

		// get boolean
		echo = data[0];
		data[0] = '\0'; // ログ出力の為、一時的に NUL Terminate する

		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ":   prompt[%d]=\"%s\", echo=%d, pass-state=%d",
			i, prompt, slen, pvar->keyboard_interactive_password_input);

		data[0] = echo; // ログ出力を行ったので、元の値に書き戻す
		data += 1;

		// keyboard-interactive method (2005.3.12 yutaka)
		if (pvar->keyboard_interactive_password_input == 0 &&
			pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) {
			AUTH_set_TIS_mode(pvar, prompt, slen);
			AUTH_advance_to_next_cred(pvar);
			pvar->ssh_state.status_flags &= ~STATUS_DONT_SEND_CREDENTIALS;
			//try_send_credentials(pvar);
			buffer_free(msg);
			return TRUE;
		}

		// TODO: ここでプロンプトを表示してユーザから入力させるのが正解。
		s = pvar->auth_state.cur_cred.password;
		buffer_put_string(msg, s, strlen(s));

		// リトライに対応できるよう、フラグをクリアする。(2010.11.11 yutaka)
		pvar->keyboard_interactive_password_input = 0;
	}

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_USERAUTH_INFO_RESPONSE, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, __FUNCTION__ ": sending SSH2_MSG_USERAUTH_INFO_RESPONSE.");
	return TRUE;
}

BOOL handle_SSH2_userauth_pkok(PTInstVar pvar)
{
		// SSH2_MSG_USERAUTH_PK_OK
		buffer_t *msg = NULL;
		char *s, *username;
		unsigned char *outmsg;
		int len;
		char *connect_id = "ssh-connection";

		unsigned char *puttykey;
		buffer_t *signbuf;
		unsigned char *signmsg;
		unsigned char *signedmsg;
		int signedlen;

		logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_USERAUTH_PK_OK was received.");

		username = pvar->auth_state.user;  // ユーザ名

		// 署名するデータを作成
		signbuf = buffer_init();
		if (signbuf == NULL) {
			safefree(pvar->pageant_key);
			return FALSE;
		}
		buffer_append_length(signbuf, pvar->session_id, pvar->session_id_len);
		buffer_put_char(signbuf, SSH2_MSG_USERAUTH_REQUEST);
		s = username;  // ユーザ名
		buffer_put_string(signbuf, s, strlen(s));
		s = connect_id;
		buffer_put_string(signbuf, s, strlen(s));
		s = "publickey";
		buffer_put_string(signbuf, s, strlen(s));
		buffer_put_char(signbuf, 1); // true

		puttykey = pvar->pageant_curkey;

		// アルゴリズムをコピーする
		len = get_uint32_MSBfirst(puttykey+4);
		buffer_put_string(signbuf, puttykey+8, len);

		// 鍵をコピーする
		len = get_uint32_MSBfirst(puttykey);
		puttykey += 4;
		buffer_put_string(signbuf, puttykey, len);
		puttykey += len;

		// Pageant に署名してもらう
		signmsg = (unsigned char *)malloc(signbuf->len + 4);
		set_uint32_MSBfirst(signmsg, signbuf->len);
		memcpy(signmsg + 4, signbuf->buf, signbuf->len);
		signedmsg = putty_sign_ssh2_key(pvar->pageant_curkey,
		                                signmsg, &signedlen);
		buffer_free(signbuf);
		if (signedmsg == NULL) {
			safefree(pvar->pageant_key);
			return FALSE;
		}


		// ペイロードの構築
		msg = buffer_init();
		if (msg == NULL) {
			safefree(pvar->pageant_key);
			safefree(signedmsg);
			return FALSE;
		}
		s = username;  // ユーザ名
		buffer_put_string(msg, s, strlen(s));
		s = connect_id;
		buffer_put_string(msg, s, strlen(s));
		s = "publickey";
		buffer_put_string(msg, s, strlen(s));
		buffer_put_char(msg, 1); // true

		puttykey = pvar->pageant_curkey;

		// アルゴリズムをコピーする
		len = get_uint32_MSBfirst(puttykey+4);
		buffer_put_string(msg, puttykey+8, len);

		// 鍵をコピーする
		len = get_uint32_MSBfirst(puttykey);
		puttykey += 4;
		buffer_put_string(msg, puttykey, len);
		puttykey += len;

		// 署名されたデータ
		len  = get_uint32_MSBfirst(signedmsg);
		buffer_put_string(msg, signedmsg + 4, len);
		free(signedmsg);

		// パケット送信
		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_USERAUTH_REQUEST, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_USERAUTH_REQUEST was sent at handle_SSH2_userauth_inforeq().");

		pvar->pageant_keyfinal = TRUE;

		return TRUE;
	}

#define PASSWD_MAXLEN 150

struct change_password {
	PTInstVar pvar;
	char passwd[PASSWD_MAXLEN];
	char new_passwd[PASSWD_MAXLEN];
};

static BOOL CALLBACK passwd_change_dialog(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char old_passwd[PASSWD_MAXLEN];
	char new_passwd[PASSWD_MAXLEN];
	char retype_passwd[PASSWD_MAXLEN];
	static struct change_password *cp;
	LOGFONT logfont;
	HFONT font;
	static HFONT DlgChgPassFont;
	char uimsg[MAX_UIMSG];
	static PTInstVar pvar;


	switch (msg) {
	case WM_INITDIALOG:
		cp = (struct change_password *)lParam;
		pvar = cp->pvar;

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);

		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgChgPassFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_OLD_PASSWD_LABEL, WM_SETFONT, (WPARAM)DlgChgPassFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgChgPassFont = NULL;
		}

		GetWindowText(dlg, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_PASSCHG_TITLE", pvar, uimsg);
		SetWindowText(dlg, pvar->ts->UIMsg);

		GetDlgItemText(dlg, IDC_PASSWD_CHANGEREQ_MSG, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_PASSCHG_MESSAGE", pvar, uimsg);
		SetDlgItemText(dlg, IDC_PASSWD_CHANGEREQ_MSG, pvar->ts->UIMsg);

		GetDlgItemText(dlg, IDC_OLD_PASSWD_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_PASSCHG_OLDPASSWD", pvar, uimsg);
		SetDlgItemText(dlg, IDC_OLD_PASSWD_LABEL, pvar->ts->UIMsg);

		GetDlgItemText(dlg, IDC_NEW_PASSWD_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_PASSCHG_NEWPASSWD", pvar, uimsg);
		SetDlgItemText(dlg, IDC_NEW_PASSWD_LABEL, pvar->ts->UIMsg);

		GetDlgItemText(dlg, IDC_CONFIRM_PASSWD_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_PASSCHG_CONFIRMPASSWD", pvar, uimsg);
		SetDlgItemText(dlg, IDC_CONFIRM_PASSWD_LABEL, pvar->ts->UIMsg);

		SetFocus(GetDlgItem(dlg, IDC_OLD_PASSWD));

		return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			SendMessage(GetDlgItem(dlg, IDC_OLD_PASSWD), WM_GETTEXT , sizeof(old_passwd), (LPARAM)old_passwd);
			SendMessage(GetDlgItem(dlg, IDC_NEW_PASSWD), WM_GETTEXT , sizeof(new_passwd), (LPARAM)new_passwd);
			SendMessage(GetDlgItem(dlg, IDC_CONFIRM_PASSWD), WM_GETTEXT , sizeof(retype_passwd), (LPARAM)retype_passwd);

			if (strcmp(new_passwd, retype_passwd) == 1) {
				UTIL_get_lang_msg("MSG_PASSCHG_MISMATCH", pvar, "Mismatch; try again.");
				MessageBox(NULL, pvar->ts->UIMsg, "ERROR", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}

			if (old_passwd[0] == 0 || new_passwd[0] == 0) {
				// ダイアログを開いてすぐに Return を押してしまった時の対策の為、
				// とりあえず空パスワードをはじいておく。
				return FALSE;
			}

			strncpy_s(cp->passwd, sizeof(cp->passwd), old_passwd, _TRUNCATE);
			strncpy_s(cp->new_passwd, sizeof(cp->new_passwd), new_passwd, _TRUNCATE);

			EndDialog(dlg, 1); // dialog close

			if (DlgChgPassFont != NULL) {
				DeleteObject(DlgChgPassFont);
				DlgChgPassFont = NULL;
			}

			return TRUE;

		case IDCANCEL:
			// 接続を切る
                        notify_closed_connection(pvar, "authentication cancelled");
			EndDialog(dlg, 0); // dialog close

			if (DlgChgPassFont != NULL) {
				DeleteObject(DlgChgPassFont);
				DlgChgPassFont = NULL;
			}

			return TRUE;
		}
	}

	return FALSE;
}

BOOL handle_SSH2_userauth_passwd_changereq(PTInstVar pvar)
{
	int len, ret;
	char *data;
	buffer_t *msg = NULL;
	char *s, *username;
	unsigned char *outmsg;
	char *connect_id = "ssh-connection";
	char *info, *lang;
	struct change_password cp;

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_USERAUTH_PASSWD_CHANGEREQ was received.");

	memset(&cp, 0, sizeof(cp));
	cp.pvar = pvar;
	ret = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHPASSWD_INPUT), pvar->cv->HWin, passwd_change_dialog, (LPARAM)&cp);

	if (ret == -1) {
		logprintf(LOG_LEVEL_WARNING, __FUNCTION__ ": DialogBoxParam failed.");
		return FALSE;
	}
	else if (ret == 0) {
		logprintf(LOG_LEVEL_NOTICE, __FUNCTION__ ": dialog cancelled.");
		return FALSE;
	}

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	info = buffer_get_string(&data, NULL);
	lang = buffer_get_string(&data, NULL);
	if (info == NULL || lang == NULL) {
		logprintf(LOG_LEVEL_ERROR,
			__FUNCTION__ ": buffer_get_string returns NULL. info=%s, lang=%s",
			NonNull(info), NonNull(lang));
	}
	else {
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": info=%s, lang=%s\n", info, lang);
	}
	free(info);
	free(lang);

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
		return FALSE;
	}

	// ペイロードの構築
	username = pvar->auth_state.user;  // ユーザ名
	buffer_put_string(msg, username, strlen(username));

	// password authentication method
	s = connect_id;
	buffer_put_string(msg, s, strlen(s));
	s = "password";
	buffer_put_string(msg, s, strlen(s));

	buffer_put_char(msg, 1); // additional info

	s = cp.passwd;
	buffer_put_string(msg, s, strlen(s));

	s = cp.new_passwd;
	buffer_put_string(msg, s, strlen(s));

	// パケット送信
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_USERAUTH_REQUEST, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);

	return TRUE;
}

/*
 * SSH_MSG_CHANNEL_REQUEST 送信関数
 * type-specific data が string で 0 ～ 2 の物に対応。
 * 使用しないメッセージは NULL にする。
 * type-specific data が他の形式には対応していないので、自前で送る事。
 */
static BOOL send_channel_request_gen(PTInstVar pvar, Channel_t *c, unsigned char *req, int want_reply, unsigned char *msg1, unsigned char *msg2)
{
	buffer_t *msg;
	unsigned char *outmsg;
	int len;

	msg = buffer_init();
	if (msg == NULL) {
		return FALSE;
	}

	buffer_put_int(msg, c->remote_id);
	buffer_put_string(msg, req, strlen(req));

	buffer_put_char(msg, want_reply);

	if (msg1) {
		buffer_put_string(msg, msg1, strlen(msg1));
	}
	if (msg2) {
		buffer_put_string(msg, msg1, strlen(msg1));
	}

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_REQUEST, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);

	logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": sending SSH2_MSG_CHANNEL_REQUEST. "
	          "local: %d, remote: %d, request-type: %s, msg1=%s, msg2=%s",
	          c->self_id, c->remote_id, req, msg1 ? msg1 : "none", msg2 ? msg2 : "none");
	return TRUE;
}

BOOL send_pty_request(PTInstVar pvar, Channel_t *c)
{
	buffer_t *msg, *ttymsg;
	char *req_type = "pty-req";  // pseudo terminalのリクエスト
	unsigned char *outmsg;
	int len, x, y;
#ifdef DONT_WANTCONFIRM
	int want_reply = 0; // false
#else
	int want_reply = 1; // true
#endif

	// pty open
	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL. (msg)");
		return FALSE;
	}
	ttymsg = buffer_init();
	if (ttymsg == NULL) {
		// TODO: error check
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL. (ttymsg)");
		buffer_free(msg);
		return FALSE;
	}

	buffer_put_int(msg, c->remote_id);
	buffer_put_string(msg, req_type, strlen(req_type));
	buffer_put_char(msg, want_reply);  // want_reply (disableに変更 2005/3/28 yutaka)

	buffer_put_string(msg, pvar->ts->TermType, strlen(pvar->ts->TermType));
	buffer_put_int(msg, pvar->ssh_state.win_cols);  // columns
	buffer_put_int(msg, pvar->ssh_state.win_rows);  // lines
	get_window_pixel_size(pvar, &x, &y);
	buffer_put_int(msg, x);  // window width (pixel):
	buffer_put_int(msg, y);  // window height (pixel):

	// TTY modeはここで渡す (2005.7.17 yutaka)
	buffer_put_char(ttymsg, SSH2_TTY_OP_OSPEED);
	buffer_put_int(ttymsg, 9600);  // baud rate
	buffer_put_char(ttymsg, SSH2_TTY_OP_ISPEED);
	buffer_put_int(ttymsg, 9600);  // baud rate

	// VERASE
	buffer_put_char(ttymsg, SSH2_TTY_KEY_VERASE);
	if (pvar->ts->BSKey == IdBS) {
		buffer_put_int(ttymsg, 0x08); // BS key
	} else {
		buffer_put_int(ttymsg, 0x7F); // DEL key
	}

	switch (pvar->ts->CRReceive) {
	  case IdLF:
		buffer_put_char(ttymsg, SSH2_TTY_OP_ONLCR);
		buffer_put_int(ttymsg, 0);
		break;
	  case IdCR:
		buffer_put_char(ttymsg, SSH2_TTY_OP_ONLCR);
		buffer_put_int(ttymsg, 1);
		break;
	  default:
		break;
	}

	buffer_put_char(ttymsg, SSH2_TTY_OP_END); // End of terminal modes

	// SSH2では文字列として書き込む。
	buffer_put_string(msg, buffer_ptr(ttymsg), buffer_len(ttymsg));

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_REQUEST, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);
	buffer_free(ttymsg);

	logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": sending SSH2_MSG_CHANNEL_REQUEST. "
	          "local: %d, remote: %d, request-type: %s, "
	          "term: %s, cols: %d, rows: %d, x: %d, y: %d, "
	          "out-speed: %d, in-speed: %d, verase: %s, onlcr: %s",
	          c->self_id, c->remote_id, req_type, pvar->ts->TermType,
	          pvar->ssh_state.win_cols, pvar->ssh_state.win_rows, x, y,
	          9600, 9600, (pvar->ts->BSKey==IdBS)?"^h":"^?", (pvar->ts->CRReceive==IdBS)?"on":"off");

	pvar->session_nego_status = 2;

	if (want_reply == 0) {
		handle_SSH2_channel_success(pvar);
	}

	return TRUE;
}

static BOOL handle_SSH2_open_confirm(PTInstVar pvar)
{
	int len;
	char *data;
	int id, remote_id;
	Channel_t *c;
	char buff[MAX_PATH + 30];

#ifdef DONT_WANTCONFIRM
	int want_reply = 0; // false
#else
	int want_reply = 1; // true
#endif

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_OPEN_CONFIRMATION was received.");

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		// TODO:
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": channel not found. (%d)", id);
		return FALSE;
	}

	// TODO: id check
	remote_id = get_uint32_MSBfirst(data);
	data += 4;

	c->remote_id = remote_id;
	if (c->self_id == pvar->shell_id) {
		// 最初のチャネル以外でリセットしてはいけない (2008.12.19 maya)
		pvar->session_nego_status = 1;
	}

	// remote window size
	c->remote_window = get_uint32_MSBfirst(data);
	data += 4;
	c->remote_maxpacket = get_uint32_MSBfirst(data);
	data += 4;

	switch (c->type) {
	case TYPE_PORTFWD:
		// port-forwadingの"direct-tcpip"が成功。
		FWD_confirmed_open(pvar, c->local_num, -1);
		break;

	case TYPE_SHELL:
		// ポートフォワーディングの準備 (2005.2.26, 2005.6.21 yutaka)
		// シェルオープンしたあとに X11 の要求を出さなくてはならない。(2005.7.3 yutaka)
		FWD_prep_forwarding(pvar);
		FWD_enter_interactive_mode(pvar);

		// エージェント転送 (2008.11.25 maya)
		if (pvar->session_settings.ForwardAgent) {
			// pty-req より前にリクエストしないとエラーになる模様
			return send_channel_request_gen(pvar, c, "auth-agent-req@openssh.com", 1, NULL, NULL);
		}
		else {
			return send_pty_request(pvar, c);
		}
		break;

	case TYPE_SCP:
		if (c->scp.dir == TOREMOTE) {
			_snprintf_s(buff, sizeof(buff), _TRUNCATE, "scp -t %s", c->scp.remotefile);

		} else {
			// ファイル名に空白を含まれていてもよいように、ファイル名を二重引用符で囲む。
			// (2014.7.13 yutaka)
			_snprintf_s(buff, sizeof(buff), _TRUNCATE, "scp -p -f \"%s\"", c->scp.remotefile);
		}

		if (!send_channel_request_gen(pvar, c, "exec", want_reply, buff, NULL)) {
			return FALSE;;
		}

		// SCPで remote-to-local の場合は、サーバからのファイル送信要求を出す。
		// この時点では remote window size が"0"なので、すぐには送られないが、遅延送信処理で送られる。
		// (2007.12.27 yutaka)
		if (c->scp.dir == FROMREMOTE) {
			char ch = '\0';
			SSH2_send_channel_data(pvar, c, &ch, 1, 0);
		}
		break;

	case TYPE_SFTP:
		if (!send_channel_request_gen(pvar, c, "subsystem", want_reply, "sftp", NULL)) {
			return FALSE;;
		}

		// SFTPセッションを開始するためのネゴシエーションを行う。
		// (2012.5.3 yutaka)
		sftp_do_init(pvar, c);
		break;

	case TYPE_SUBSYSTEM_GEN:
		if (!send_channel_request_gen(pvar, c, "subsystem", want_reply, pvar->subsystem_name, NULL)) {
			return FALSE;;
		}
		pvar->session_nego_status = 0;
		break;

	default: // NOT REACHED
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": Invalid channel-type. (%d)", c->type);
		return FALSE;
	}
	return TRUE;
}

// SSH2 port-forwarding においてセッションがオープンできない場合のサーバからのリプライ（失敗）
static BOOL handle_SSH2_open_failure(PTInstVar pvar)
{	
	int len;
	char *data;
	int id;
	Channel_t *c;
	int reason;
	char *cstring;
	char tmpbuf[256];
	char *rmsg;

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_OPEN_FAILURE was received.");

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		// TODO: SSH2_MSG_DISCONNECTを送る
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": channel not found. (%d)", id);
		return FALSE;
	}

	reason = get_uint32_MSBfirst(data);
	data += 4;

	if (reason == SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED) {
		rmsg = "administratively prohibited";
	} else if (reason == SSH2_OPEN_CONNECT_FAILED) {
		rmsg = "connect failed";
	} else if (reason == SSH2_OPEN_UNKNOWN_CHANNEL_TYPE) {
		rmsg = "unknown channel type";
	} else if (reason == SSH2_OPEN_RESOURCE_SHORTAGE) {
		rmsg = "resource shortage";
	} else {
		rmsg = "unknown reason";
	}

	cstring = buffer_get_string(&data, NULL);

	if (cstring == NULL) {
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_get_string returns NULL");
	}
	UTIL_get_lang_msg("MSG_SSH_CHANNEL_OPEN_ERROR", pvar,
	                  "SSH2_MSG_CHANNEL_OPEN_FAILURE was received.\r\nchannel [%d]: reason: %s(%d) message: %s");
	_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, pvar->ts->UIMsg,
	            id, rmsg, reason, NonNull(cstring));
	notify_nonfatal_error(pvar, tmpbuf);

	free(cstring);

	if (c->type == TYPE_PORTFWD) {
		FWD_failed_open(pvar, c->local_num, reason);
	}

	// チャネルの解放漏れを修正 (2007.5.1 maya)
	ssh2_channel_delete(c);

	return TRUE;
}

// SSH2_MSG_GLOBAL_REQUEST for OpenSSH 6.8
static BOOL handle_SSH2_client_global_request(PTInstVar pvar)
{
	int len, n;
	char *data;
	char *rtype;
	int want_reply;
	int success = 0;
	buffer_t *msg;
	unsigned char *outmsg;
	int type;

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_GLOBAL_REQUEST was received.");

	// SSH2 packet format:
	// [size(4) + padding size(1) + type(1)] + [payload(N) + padding(X)]
	//  header                                     body
	//                                         ^data
	//            <-----------------size------------------------------->
	//                              <---------len-------->
	//
	// data = payload(N) + padding(X): パディングも含めたボディすべてを指す。
	data = pvar->ssh_state.payload;
	// len = size - (padding size + 1): パディングを除くボディ。typeが先頭に含まれる。
	len = pvar->ssh_state.payloadlen;

	len--;   // type 分を除く

	rtype = buffer_get_string(&data, &n);
	len -= (n + 4);

	want_reply = data[0];
	data++;
	len--;

	if (rtype == NULL) {
		// rtype が NULL で無い事の保証
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_get_string returns NULL.");
	}
	else if (strcmp(rtype, "hostkeys-00@openssh.com") == 0) {
		// OpenSSH 6.8では、サーバのホスト鍵が更新されると、この通知が来る。
		// OpenSSH 6.8の実装では、常に成功で返すようになっているため、
		// それに合わせて Tera Term でも成功と返すことにする。
		success = update_client_input_hostkeys(pvar, data, len);

	}
	free(rtype);

	if (want_reply) {
		msg = buffer_init();
		if (msg) {
			len = buffer_len(msg);
			type = success ? SSH2_MSG_REQUEST_SUCCESS : SSH2_MSG_REQUEST_FAILURE;
			outmsg = begin_send_packet(pvar, type, len);
			memcpy(outmsg, buffer_ptr(msg), len);
			finish_send_packet(pvar);
			buffer_free(msg);
		}
	}

	return TRUE;
}


// SSH2 port-forwarding (remote -> local)に対するリプライ（成功）
static BOOL handle_SSH2_request_success(PTInstVar pvar)
{	
	// 必要であればログを取る。特に何もしなくてもよい。
	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_REQUEST_SUCCESS was received.");

	client_global_request_reply(pvar, SSH2_MSG_REQUEST_SUCCESS, 0, NULL);

	return TRUE;
}

// SSH2 port-forwarding (remote -> local)に対するリプライ（失敗）
static BOOL handle_SSH2_request_failure(PTInstVar pvar)
{	
	// 必要であればログを取る。特に何もしなくてもよい。
	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_REQUEST_FAILURE was received.");

	client_global_request_reply(pvar, SSH2_MSG_REQUEST_FAILURE, 0, NULL);

	return TRUE;
}

static BOOL handle_SSH2_channel_success(PTInstVar pvar)
{	
	Channel_t *c;
#ifdef DONT_WANTCONFIRM
	int want_reply = 0; // false
#else
	int want_reply = 1; // true
#endif

	logprintf(LOG_LEVEL_VERBOSE,
		"SSH2_MSG_CHANNEL_SUCCESS was received(nego_status %d).",
		pvar->session_nego_status);

	if (pvar->session_nego_status == 1) {
		// find channel by shell id(2005.2.27 yutaka)
		c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": shell channel not found.");
			return FALSE;
		}
		pvar->agentfwd_enable = TRUE;
		return send_pty_request(pvar, c);

	} else if (pvar->session_nego_status == 2) {
		pvar->session_nego_status = 3;

		// find channel by shell id(2005.2.27 yutaka)
		c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": shell channel not found.");
			return FALSE;
		}

		if (!send_channel_request_gen(pvar, c, "shell", want_reply, NULL, NULL)) {
			return FALSE;;
		}

		if (want_reply == 0) {
			handle_SSH2_channel_success(pvar);
		}

	} else if (pvar->session_nego_status == 3) {
		pvar->session_nego_status = 4;

	} else {

	}

	return TRUE;
}

static BOOL handle_SSH2_channel_failure(PTInstVar pvar)
{
	Channel_t *c;
	char *data;
	int channel_id;

	data = pvar->ssh_state.payload;
	channel_id = get_uint32_MSBfirst(data);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_FAILURE was received.");

	c = ssh2_channel_lookup(channel_id);
	if (c == NULL) {
		// TODO: error check
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": channel not found. (%d)", channel_id);
		return FALSE;
	}

	if (pvar->shell_id == channel_id) {
		if (c->type == TYPE_SUBSYSTEM_GEN) {
			// サブシステムの起動に失敗したので切る。
			char errmsg[MAX_UIMSG];
			UTIL_get_lang_msg("MSG_SSH_SUBSYSTEM_REQUEST_ERROR", pvar, "subsystem request failed. (%s)");
			_snprintf_s(errmsg, sizeof(errmsg), _TRUNCATE, pvar->ts->UIMsg, pvar->subsystem_name);
			notify_fatal_error(pvar, errmsg, TRUE);
			return TRUE;
		}
		else { // TYPE_SHELL
			if (pvar->session_nego_status == 1) {
				// リモートで auth-agent-req@openssh.com がサポートされてないので
				// エラーは気にせず次へ進む
				logputs(LOG_LEVEL_VERBOSE, "auth-agent-req@openssh.com is not supported by remote host.");

				return send_pty_request(pvar, c);
			}
		}
	}

	ssh2_channel_delete(c);
	return TRUE;
}



// クライアントのwindow sizeをサーバへ知らせる
static void do_SSH2_adjust_window_size(PTInstVar pvar, Channel_t *c)
{
	// window sizeを32KBへ変更し、local windowの判別を修正。
	// これによりSSH2のスループットが向上する。(2006.3.6 yutaka)
	buffer_t *msg;
	unsigned char *outmsg;
	int len;

	// ローカルのwindow sizeにまだ余裕があるなら、何もしない。
	// added /2 (2006.3.6 yutaka)
	if (c->local_window > c->local_window_max/2)
		return;

	{
		// pty open
		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			return;
		}
		buffer_put_int(msg, c->remote_id);
		buffer_put_int(msg, c->local_window_max - c->local_window);

		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_WINDOW_ADJUST, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		logputs(LOG_LEVEL_SSHDUMP, "SSH2_MSG_CHANNEL_WINDOW_ADJUST was sent at do_SSH2_adjust_window_size().");
		// クライアントのwindow sizeを増やす
		c->local_window = c->local_window_max;
	}

}


void ssh2_channel_send_close(PTInstVar pvar, Channel_t *c)
{
	if (SSHv2(pvar)) {
		buffer_t *msg;
		unsigned char *outmsg;
		int len;

		// このchannelについてcloseを送信済みなら送らない
		if (c->state & SSH_CHANNEL_STATE_CLOSE_SENT) {
			return;
		}

		// SSH2 serverにchannel closeを伝える
		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			return;
		}
		buffer_put_int(msg, c->remote_id);

		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_CLOSE, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		c->state |= SSH_CHANNEL_STATE_CLOSE_SENT;

		logprintf(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_CLOSE was sent at ssh2_channel_send_close(). local:%d remote:%d", c->self_id, c->remote_id);
	}
}


#define WM_SENDING_FILE (WM_USER + 1)
#define WM_CHANNEL_CLOSE (WM_USER + 2)
#define WM_GET_CLOSED_STATUS (WM_USER + 3)

typedef struct scp_dlg_parm {
	Channel_t *c;
	PTInstVar pvar;
	char *buf;
	size_t buflen;
} scp_dlg_parm_t;

static LRESULT CALLBACK ssh_scp_dlg_proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static int closed = 0;

	switch (msg) {
		case WM_INITDIALOG:
			closed = 0;
			return FALSE;

		// SCPファイル受信(remote-to-local)時、使用する。
		case WM_CHANNEL_CLOSE:
			{
			scp_dlg_parm_t *parm = (scp_dlg_parm_t *)wp;

			ssh2_channel_send_close(parm->pvar, parm->c);
			}
			return TRUE;
			break;

		case WM_SENDING_FILE:
			{
			scp_dlg_parm_t *parm = (scp_dlg_parm_t *)wp;

			SSH2_send_channel_data(parm->pvar, parm->c, parm->buf, parm->buflen, 0);
			}
			return TRUE;
			break;

		case WM_COMMAND:
			switch (wp) {
			}

			switch (LOWORD(wp)) {
				case IDOK:
					{
					return TRUE;
					}

				case IDCANCEL:
					// ウィンドウをいきなり破棄するのではなく、非表示にするのみとして、
					// スレッドからのメッセージを処理できるようにする。
					// (2011.6.8 yutaka)
					//EndDialog(hWnd, 0);
					//DestroyWindow(hWnd);
					ShowWindow(hWnd, SW_HIDE);
					closed = 1;
					return TRUE;
				default:
					return FALSE;
			}
			break;

		case WM_CLOSE:
			// closeボタンが押下されても window が閉じないようにする。
			return TRUE;

		case WM_DESTROY:
			return TRUE;

		case WM_GET_CLOSED_STATUS:
			{
			int *flag = (int *)wp;



			*flag = closed;
			}
			return TRUE;

		default:
			return FALSE;
	}
	return TRUE;
}

static int is_canceled_window(HWND hd)
{
	int closed = 0;

	SendMessage(hd, WM_GET_CLOSED_STATUS, (WPARAM)&closed, 0);
	if (closed)
		return 1;
	else
		return 0;
}

void InitDlgProgress(HWND HDlg, int id_Progress, int *CurProgStat) {
	HWND HProg;
	HProg = GetDlgItem(HDlg, id_Progress);

	*CurProgStat = 0;

	SendMessage(HProg, PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, 100));
	SendMessage(HProg, PBM_SETSTEP, (WPARAM)1, 0);
	SendMessage(HProg, PBM_SETPOS, (WPARAM)0, 0);

	return;
}

static unsigned __stdcall ssh_scp_thread(void *p)
{
	Channel_t *c = (Channel_t *)p;
	PTInstVar pvar = c->scp.pvar;
	long long total_size = 0;
	char *buf = NULL;
	size_t buflen;
	char s[80];
	size_t ret;
	HWND hWnd = c->scp.progress_window;
	scp_dlg_parm_t parm;
	int rate, ProgStat;
	DWORD stime;
	int elapsed, prev_elapsed;

	buflen = min(c->remote_window, 8192*4); // max 32KB
	buf = malloc(buflen);

	//SendMessage(GetDlgItem(hWnd, IDC_FILENAME), WM_SETTEXT, 0, (LPARAM)c->scp.localfile);
	SendMessage(GetDlgItem(hWnd, IDC_FILENAME), WM_SETTEXT, 0, (LPARAM)c->scp.localfilefull);

	InitDlgProgress(hWnd, IDC_PROGBAR, &ProgStat);

	stime = GetTickCount();
	prev_elapsed = 0;

	do {
		int readlen, count=0;

		// Cancelボタンが押下されたらウィンドウが消える。
		if (is_canceled_window(hWnd))
			goto cancel_abort;

		// ファイルから読み込んだデータはかならずサーバへ送信する。
		readlen = max(4096, min(buflen, c->remote_window)); // min 4KB
		ret = fread(buf, 1, readlen, c->scp.localfp);
		if (ret == 0)
			break;

		// remote_window が回復するまで待つ
		do {
			// socket or channelがクローズされたらスレッドを終わる
			if (pvar->socket == INVALID_SOCKET || c->scp.state == SCP_CLOSING || c->used == 0)
				goto abort;

			if (ret > c->remote_window) {
				Sleep(100);
			}

			// 100回抜けられなかったら抜けてしまう
			count++;
			if (count > 100) {
				break;
			}

		} while (ret > c->remote_window);

		// sending data
		parm.buf = buf;
		parm.buflen = ret;
		parm.c = c;
		parm.pvar = pvar;
		SendMessage(hWnd, WM_SENDING_FILE, (WPARAM)&parm, 0);

		total_size += ret;

		rate = (int)(100 * total_size / c->scp.filestat.st_size);
		_snprintf_s(s, sizeof(s), _TRUNCATE, "%lld / %lld (%d%%)", total_size, c->scp.filestat.st_size, rate);
		SendMessage(GetDlgItem(hWnd, IDC_PROGRESS), WM_SETTEXT, 0, (LPARAM)s);
		if (ProgStat != rate) {
			ProgStat = rate;
			SendDlgItemMessage(hWnd, IDC_PROGBAR, PBM_SETPOS, (WPARAM)ProgStat, 0);
		}

		elapsed = (GetTickCount() - stime) / 1000;
		if (elapsed > prev_elapsed) {
			if (elapsed > 2) {
				rate = (int)(total_size / elapsed);
				if (rate < 1200) {
					_snprintf_s(s, sizeof(s), _TRUNCATE, "%d:%02d (%d %s)", elapsed / 60, elapsed % 60, rate, "Bytes/s");
				}
				else if (rate < 1200000) {
					_snprintf_s(s, sizeof(s), _TRUNCATE, "%d:%02d (%d.%02d %s)", elapsed / 60, elapsed % 60, rate / 1000, rate / 10 % 100, "KBytes/s");
				}
				else {
					_snprintf_s(s, sizeof(s), _TRUNCATE, "%d:%02d (%d.%02d %s)", elapsed / 60, elapsed % 60, rate / (1000 * 1000), rate / 10000 % 100, "MBytes/s");
				}
			}
			else {
				_snprintf_s(s, sizeof(s), _TRUNCATE, "%d:%02d", elapsed / 60, elapsed % 60);
			}
			SendDlgItemMessage(hWnd, IDC_PROGTIME, WM_SETTEXT, 0, (LPARAM)s);
			prev_elapsed = elapsed;
		}

	} while (ret <= buflen);

	// eof
	c->scp.state = SCP_DATA;

	buf[0] = '\0';
	parm.buf = buf;
	parm.buflen = 1;
	parm.c = c;
	parm.pvar = pvar;
	SendMessage(hWnd, WM_SENDING_FILE, (WPARAM)&parm, 0);

	ShowWindow(hWnd, SW_HIDE);

	free(buf);

	return 0;

cancel_abort:
	// チャネルのクローズを行いたいが、直接 ssh2_channel_send_close() を呼び出すと、
	// 当該関数がスレッドセーフではないため、SCP処理が正常に終了しない場合がある。
	// (2011.6.8 yutaka)
	parm.c = c;
	parm.pvar = pvar;
	SendMessage(hWnd, WM_CHANNEL_CLOSE, (WPARAM)&parm, 0);

abort:

	free(buf);

	return 0;
}


static void SSH2_scp_toremote(PTInstVar pvar, Channel_t *c, unsigned char *data, unsigned int buflen)
{
	if (c->scp.state == SCP_INIT) {
		char buf[128];

		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "T%lu 0 %lu 0\n", 
			(unsigned long)c->scp.filestat.st_mtime,  (unsigned long)c->scp.filestat.st_atime);

		c->scp.state = SCP_TIMESTAMP;
		SSH2_send_channel_data(pvar, c, buf, strlen(buf), 0);

	} else if (c->scp.state == SCP_TIMESTAMP) {
		char buf[128];

		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "C0644 %lld %s\n", 
			c->scp.filestat.st_size, c->scp.localfile);

		c->scp.state = SCP_FILEINFO;
		SSH2_send_channel_data(pvar, c, buf, strlen(buf), 0);

	} else if (c->scp.state == SCP_FILEINFO) {
		HWND hDlgWnd;
		HANDLE thread;
		unsigned int tid;

		c->scp.pvar = pvar;

		hDlgWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SSHSCP_PROGRESS),
				   pvar->cv->HWin, (DLGPROC)ssh_scp_dlg_proc);
		if (hDlgWnd != NULL) {
			c->scp.progress_window = hDlgWnd;
			ShowWindow(hDlgWnd, SW_SHOW);
		}

		thread = (HANDLE)_beginthreadex(NULL, 0, ssh_scp_thread, c, 0, &tid);
		if (thread == (HANDLE)-1) {
			// TODO:
		}
		c->scp.thread = thread;


	} else if (c->scp.state == SCP_DATA) {
		// 送信完了
		ssh2_channel_send_close(pvar, c);
		//ssh2_channel_delete(c);  // free channel

		//MessageBox(NULL, "SCP sending done.", "TTSSH", MB_OK);
	}
}


#define WM_RECEIVING_FILE (WM_USER + 2)

static unsigned __stdcall ssh_scp_receive_thread(void *p)
{
	Channel_t *c = (Channel_t *)p;
	PTInstVar pvar = c->scp.pvar;
	long long total_size = 0;
	char s[80];
	HWND hWnd = c->scp.progress_window;
	MSG msg;
	unsigned char *data;
	unsigned int buflen;
	int eof;
	int rate, ProgStat;
	DWORD stime;
	int elapsed, prev_elapsed;
	scp_dlg_parm_t parm;

	InitDlgProgress(hWnd, IDC_PROGBAR, &ProgStat);

	stime = GetTickCount();
	prev_elapsed = 0;

	for (;;) {
		// Cancelボタンが押下されたらウィンドウが消える。
		if (is_canceled_window(hWnd))
			goto cancel_abort;

		ssh2_scp_get_packetlist(c, &data, &buflen);
		if (data && buflen) {
			msg.message = WM_RECEIVING_FILE;

			switch (msg.message) {
			case WM_RECEIVING_FILE:
				//data = (unsigned char *)msg.wParam;
				//buflen = (unsigned int)msg.lParam;
				eof = 0;

				if (c->scp.filercvsize >= c->scp.filetotalsize) { // EOF
					free(data);  // free!
					goto done;
				}

				if (c->scp.filercvsize + buflen > c->scp.filetotalsize) { // overflow (include EOF)
					buflen = (unsigned int)(c->scp.filetotalsize - c->scp.filercvsize);
					eof = 1;
				}

				c->scp.filercvsize += buflen;

				if (fwrite(data, 1, buflen, c->scp.localfp) < buflen) { // error
					// TODO:
				}

				free(data);  // free!

				rate =(int)(100 * c->scp.filercvsize / c->scp.filetotalsize);
				_snprintf_s(s, sizeof(s), _TRUNCATE, "%lld / %lld (%d%%)", c->scp.filercvsize, c->scp.filetotalsize, rate);
				SendMessage(GetDlgItem(c->scp.progress_window, IDC_PROGRESS), WM_SETTEXT, 0, (LPARAM)s);

				if (ProgStat != rate) {
					ProgStat = rate;
					SendDlgItemMessage(c->scp.progress_window, IDC_PROGBAR, PBM_SETPOS, (WPARAM)ProgStat, 0);
				}

				elapsed = (GetTickCount() - stime) / 1000;
				if (elapsed > prev_elapsed) {
					if (elapsed > 2) {
						rate = (int)(c->scp.filercvsize / elapsed);
						if (rate < 1200) {
							_snprintf_s(s, sizeof(s), _TRUNCATE, "%d:%02d (%d %s)", elapsed / 60, elapsed % 60, rate, "Bytes/s");
						}
						else if (rate < 1200000) {
							_snprintf_s(s, sizeof(s), _TRUNCATE, "%d:%02d (%d.%02d %s)", elapsed / 60, elapsed % 60, rate / 1000, rate / 10 % 100, "KBytes/s");
						}
						else {
							_snprintf_s(s, sizeof(s), _TRUNCATE, "%d:%02d (%d.%02d %s)", elapsed / 60, elapsed % 60, rate / (1000 * 1000), rate / 10000 % 100, "MBytes/s");
						}
					}
					else {
						_snprintf_s(s, sizeof(s), _TRUNCATE, "%d:%02d", elapsed / 60, elapsed % 60);
					}
					SendDlgItemMessage(hWnd, IDC_PROGTIME, WM_SETTEXT, 0, (LPARAM)s);
					prev_elapsed = elapsed;
				}

				if (eof)
					goto done;

				break;
			}
		}
		Sleep(0);
	}

done:
	c->scp.state = SCP_CLOSING;
	ShowWindow(c->scp.progress_window, SW_HIDE);

cancel_abort:
	// チャネルのクローズを行いたいが、直接 ssh2_channel_send_close() を呼び出すと、
	// 当該関数がスレッドセーフではないため、SCP処理が正常に終了しない場合がある。
	// (2011.6.1 yutaka)
	parm.c = c;
	parm.pvar = pvar;
	SendMessage(hWnd, WM_CHANNEL_CLOSE, (WPARAM)&parm, 0);

	return 0;
}

static void ssh2_scp_add_packetlist(Channel_t *c, unsigned char *buf, unsigned int buflen)
{
	PacketList_t *p, *old;

	EnterCriticalSection(&g_ssh_scp_lock);

	// allocate new buffer
	p = malloc(sizeof(PacketList_t));
	if (p == NULL)
		goto error;
	p->buf = buf;
	p->buflen = buflen;
	p->next = NULL;

	if (c->scp.pktlist_head == NULL) {
		c->scp.pktlist_head = p;
		c->scp.pktlist_tail = p;
	}
	else {
		old = c->scp.pktlist_tail;
		old->next = p;
		c->scp.pktlist_tail = p;
	}

error:;
	LeaveCriticalSection(&g_ssh_scp_lock);
}

static void ssh2_scp_get_packetlist(Channel_t *c, unsigned char **buf, unsigned int *buflen)
{
	PacketList_t *p;

	EnterCriticalSection(&g_ssh_scp_lock);

	if (c->scp.pktlist_head == NULL) {
		*buf = NULL;
		*buflen = 0;
		goto end;
	}

	p = c->scp.pktlist_head;
	*buf = p->buf;
	*buflen = p->buflen;

	c->scp.pktlist_head = p->next;

	if (c->scp.pktlist_head == NULL)
		c->scp.pktlist_tail = NULL;

	free(p);

end:;
	LeaveCriticalSection(&g_ssh_scp_lock);
}

static void ssh2_scp_alloc_packetlist(Channel_t *c)
{
	c->scp.pktlist_head = NULL;
	c->scp.pktlist_tail = NULL;
	InitializeCriticalSection(&g_ssh_scp_lock);
}

static void ssh2_scp_free_packetlist(Channel_t *c)
{
	PacketList_t *p, *old;

	p = c->scp.pktlist_head;
	while (p) {
		old = p;
		p = p->next;

		free(old->buf);
		free(old);
	}

	c->scp.pktlist_head = NULL;
	c->scp.pktlist_tail = NULL;
	DeleteCriticalSection(&g_ssh_scp_lock);
}

static BOOL SSH2_scp_fromremote(PTInstVar pvar, Channel_t *c, unsigned char *data, unsigned int buflen)
{
	int permission;
	long long size;
	char filename[MAX_PATH];
	char ch;
	HWND hDlgWnd;
	char msg[256];
	int copylen;

	if (buflen == 0)
		return FALSE;

	if (c->scp.state == SCP_INIT) {
		if (data[0] == '\01' || data[1] == '\02') {  // error
			return FALSE;
		}

		if (data[0] == 'T') {  // Tmtime.sec mtime.usec atime.sec atime.usec
			DWORD mtime, atime;

			sscanf_s(data, "T%ld 0 %ld 0", &mtime, &atime);

			// タイムスタンプを記録
			c->scp.filemtime = mtime;
			c->scp.fileatime = atime;

			// リプライを返す
			goto reply;

		} else if (data[0] == 'C') {  // C0666 size file
			HANDLE thread;
			unsigned int tid;

			sscanf_s(data, "C%o %lld %s", &permission, &size, filename, sizeof(filename));

			// Windowsなのでパーミッションは無視。サイズのみ記録。
			c->scp.filetotalsize = size;
			c->scp.filercvsize = 0;

			c->scp.state = SCP_DATA;

			// 進捗ウィンドウ
			c->scp.pvar = pvar;
			hDlgWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SSHSCP_PROGRESS),
					   pvar->cv->HWin, (DLGPROC)ssh_scp_dlg_proc);
			if (hDlgWnd != NULL) {
				c->scp.progress_window = hDlgWnd;
				SetWindowText(hDlgWnd, "TTSSH: SCP receiving file");
				SendMessage(GetDlgItem(hDlgWnd, IDC_FILENAME), WM_SETTEXT, 0, (LPARAM)c->scp.localfilefull);
				ShowWindow(hDlgWnd, SW_SHOW);
			}

			ssh2_scp_alloc_packetlist(c);
			thread = (HANDLE)_beginthreadex(NULL, 0, ssh_scp_receive_thread, c, 0, &tid);
			if (thread == (HANDLE)-1) {
				// TODO:
			}
			c->scp.thread = thread;
			c->scp.thread_id = tid;

			goto reply;

		} else {
			// サーバからのデータが不定の場合は、エラー表示を行う。
			// (2014.7.13 yutaka)
			copylen = min(buflen, sizeof(msg));
			memcpy(msg, data, copylen);
			msg[copylen - 1] = 0;
			MessageBox(NULL, msg, "TTSSH: SCP error(SCP_INIT)", MB_OK | MB_ICONEXCLAMATION);

		}

	} else if (c->scp.state == SCP_DATA) {  // payloadの受信
		unsigned char *newdata = malloc(buflen);
		if (newdata != NULL) {
			memcpy(newdata, data, buflen);

			// SCP受信処理のスピードが速い場合、スレッドのメッセージキューがフル(10000個)に
			// なり、かつスレッド上での SendMessage がブロックすることにより、Tera Term(TTSSH)
			// 自体がストールしてしまう。この問題を回避するため、スレッドのメッセージキューを
			// 使うのをやめて、リンクドリスト方式に切り替える。
			// (2016.11.3 yutaka)
			ssh2_scp_add_packetlist(c, newdata, buflen);
		}

	} else if (c->scp.state == SCP_CLOSING) {  // EOFの受信
		ssh2_channel_send_close(pvar, c);

	}

	return TRUE;

reply:
	ch = '\0';
	SSH2_send_channel_data(pvar, c, &ch, 1, 0);
	return TRUE;
}

// cf. response()#scp.c
static void SSH2_scp_response(PTInstVar pvar, Channel_t *c, unsigned char *data, unsigned int buflen)
{
	if (c->scp.dir == FROMREMOTE) {
		if (SSH2_scp_fromremote(pvar, c, data, buflen) == FALSE)
			goto error;

	} else	if (c->scp.dir == TOREMOTE) {
		if (buflen == 1 && data[0] == '\0') {  // OK
			SSH2_scp_toremote(pvar, c, data, buflen);
		} else {
			goto error;
		}
	}
	return;

error:
	{  // error
		char msg[2048];
		unsigned int i, max;
		int offset, resp;

		resp = data[0];

		// エラーコードにより文字列の格納場所が若干異なる。
		if (resp == 1 || /* error, followed by error msg */
			resp == 2) {  /* fatal error, "" */
			offset = 1;
		} else {
			offset = 0;
		}

		if (buflen > sizeof(msg))
			max = sizeof(msg);
		else
			max = buflen - offset;
		for (i = 0 ; i < max ; i++) {
			msg[i] = data[i + offset];
		}
		msg[i] = '\0';

		// よく分からないエラーの場合は、自身でチャネルをクローズする。
		// .bashrc に"stty stop undef"が定義されていると、TTSSHが落ちる問題への暫定処置。
		// 落ちる原因は分かっていない。
		// (2013.4.5 yutaka)
		if (resp == 1) {
			ssh2_channel_send_close(pvar, c);
		} else {
			//ssh2_channel_delete(c);  // free channel
			//ssh2_channel_send_close(pvar, c);
		}

		MessageBox(NULL, msg, "TTSSH: SCP error", MB_OK | MB_ICONEXCLAMATION);
	}
}


static BOOL handle_SSH2_channel_data(PTInstVar pvar)
{	
	int len;
	char *data;
	int id;
	unsigned int str_len;
	Channel_t *c;

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	// channel number
	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		// TODO:
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": channel not found. (%d)", id);
		return FALSE;
	}

	// string length
	str_len = get_uint32_MSBfirst(data);
	data += 4;

	if (LogLevel(pvar, LOG_LEVEL_SSHDUMP)) {
		logprintf(LOG_LEVEL_SSHDUMP, "SSH2_MSG_CHANNEL_DATA was received. local:%d remote:%d len:%d", c->self_id, c->remote_id, str_len);
		init_memdump();
		push_memdump("SSH receiving packet", "PKT_recv", (char *)data, str_len);
	}

	// バッファサイズのチェック
	if (str_len > c->local_maxpacket) {
		logprintf(LOG_LEVEL_WARNING, __FUNCTION__ ": Data length is larger than local_maxpacket. "
			"len:%d local_maxpacket:%d", str_len, c->local_maxpacket);
	}
	if (str_len > c->local_window) {
		// TODO: logging
		// local window sizeより大きなパケットは捨てる
		logprintf(LOG_LEVEL_WARNING, __FUNCTION__ ": Data length is larger than local_window. "
			"len:%d local_window:%d", str_len, c->local_window);
		return FALSE;
	}

	// ペイロードとしてクライアント(Tera Term)へ渡す
	if (c->type == TYPE_SHELL || c->type == TYPE_SUBSYSTEM_GEN) {
		pvar->ssh_state.payload_datalen = str_len;
		pvar->ssh_state.payload_datastart = 8; // id + strlen

	} else if (c->type == TYPE_PORTFWD) {
		//debug_print(0, data, strlen);
		FWD_received_data(pvar, c->local_num, data, str_len);

	} else if (c->type == TYPE_SCP) {  // SCP
		SSH2_scp_response(pvar, c, data, str_len);

	} else if (c->type == TYPE_SFTP) {  // SFTP
		sftp_response(pvar, c, data, str_len);

	} else if (c->type == TYPE_AGENT) {  // agent forward
		if (!SSH_agent_response(pvar, c, 0, data, str_len)) {
			return FALSE;
		}
	}

	// ウィンドウサイズの調整
	c->local_window -= str_len;

	do_SSH2_adjust_window_size(pvar, c);

	return TRUE;
}


// Tectia Server の Windows 版は、DOSコマンドが失敗したときにstderrに出力される
// エラーメッセージを SSH2_MSG_CHANNEL_EXTENDED_DATA で送信してくる。
// SSH2_MSG_CHANNEL_EXTENDED_DATA を処理するようにした。(2006.10.30 maya)
static BOOL handle_SSH2_channel_extended_data(PTInstVar pvar)
{	
	int len;
	char *data;
	int id;
	unsigned int strlen;
	Channel_t *c;
	int data_type;

	logputs(LOG_LEVEL_SSHDUMP, "SSH2_MSG_CHANNEL_EXTENDED_DATA was received.");

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	//debug_print(80, data, len);

	// channel number
	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		// TODO:
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": channel not found. (%d)", id);
		return FALSE;
	}

	// data_type_code
	data_type = get_uint32_MSBfirst(data);
	data += 4;

	// string length
	strlen = get_uint32_MSBfirst(data);
	data += 4;

	// バッファサイズのチェック
	if (strlen > c->local_maxpacket) {
		// TODO: logging
		logprintf(LOG_LEVEL_WARNING, __FUNCTION__ ": Data length is larger than local_maxpacket. "
			"len:%d local_maxpacket:%d", strlen, c->local_maxpacket);
	}
	if (strlen > c->local_window) {
		// TODO: logging
		// local window sizeより大きなパケットは捨てる
		logprintf(LOG_LEVEL_WARNING, __FUNCTION__ ": Data length is larger than local_window. "
			"len:%d local_window:%d", strlen, c->local_window);
		return FALSE;
	}

	// ペイロードとしてクライアント(Tera Term)へ渡す
	if (c->type == TYPE_SHELL || c->type == TYPE_SUBSYSTEM_GEN) {
		pvar->ssh_state.payload_datalen = strlen;
		pvar->ssh_state.payload_datastart = 12; // id + data_type + strlen

	} else if (c->type == TYPE_PORTFWD) {
		//debug_print(0, data, strlen);
		FWD_received_data(pvar, c->local_num, data, strlen);

	} else if (c->type == TYPE_SCP) {  // SCP
		SSH2_scp_response(pvar, c, data, strlen);

	} else if (c->type == TYPE_SFTP) {  // SFTP

	} else if (c->type == TYPE_AGENT) {  // agent forward
		if (!SSH_agent_response(pvar, c, 0, data, strlen)) {
			return FALSE;
		}
	}

	//debug_print(200, data, strlen);

	// ウィンドウサイズの調整
	c->local_window -= strlen;

	do_SSH2_adjust_window_size(pvar, c);

	return TRUE;
}


static BOOL handle_SSH2_channel_eof(PTInstVar pvar)
{	
	int len;
	char *data;
	int id;
	Channel_t *c;

	// 切断時にサーバが SSH2_MSG_CHANNEL_EOF を送ってくるので、チャネルを解放する。(2005.6.19 yutaka)

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	// channel number
	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		// TODO:
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": channel not found. (%d)", id);
		return FALSE;
	}

	logprintf(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_EOF was received. local:%d remote:%d", c->self_id, c->remote_id);

	if (c->type == TYPE_PORTFWD) {
		FWD_channel_input_eof(pvar, c->local_num);
	}
	else if (c->type == TYPE_AGENT) {
		ssh2_channel_send_close(pvar, c);
	}
	else {
		// どうするのが正しい？
	}

	return TRUE;
}

static BOOL handle_SSH2_channel_open(PTInstVar pvar)
{	
	int len;
	char *data;
	Channel_t *c = NULL;
	int buflen;
	char *ctype;
	int remote_id;
	int remote_window;
	int remote_maxpacket;
	int chan_num = -1;
	buffer_t *msg;
	unsigned char *outmsg;

	logputs(LOG_LEVEL_VERBOSE, __FUNCTION__ ": SSH2_MSG_CHANNEL_OPEN was received.");

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	// get string
	ctype = buffer_get_string(&data, &buflen);

	// get value
	remote_id = get_uint32_MSBfirst(data);
	data += 4;
	remote_window = get_uint32_MSBfirst(data);
	data += 4;
	remote_maxpacket = get_uint32_MSBfirst(data);
	data += 4;

	logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__
		": type=%s, channel=%d, init_winsize=%d, max_packetsize:%d",
		NonNull(ctype), remote_id, remote_window, remote_maxpacket);

	// check Channel Type(string)
	if (ctype == NULL) {
		// ctype が NULL で無い事の保証の為、先にチェックする
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_get_string returns NULL. (ctype)");
	}
	else if (strcmp(ctype, "forwarded-tcpip") == 0) { // port-forwarding(remote to local)
		char *listen_addr, *orig_addr;
		int listen_port, orig_port;

		listen_addr = buffer_get_string(&data, &buflen);  // 0.0.0.0
		listen_port = get_uint32_MSBfirst(data); // 5000
		data += 4;

		orig_addr = buffer_get_string(&data, &buflen);  // 127.0.0.1
		orig_port = get_uint32_MSBfirst(data);  // 32776
		data += 4;

		if (listen_addr && orig_addr) {
			logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__
				": %s: listen_addr=%s, listen_port=%d, orig_addr=%s, orig_port=%d",
				ctype, listen_addr, listen_port, orig_addr, orig_port);
			// searching request entry by listen_port & create_local_channel
			FWD_open(pvar, remote_id, listen_addr, listen_port, orig_addr, orig_port, &chan_num);

			// channelをアロケートし、必要な情報（remote window size）をここで取っておく。
			// changed window size from 128KB to 32KB. (2006.3.6 yutaka)
			// changed window size from 32KB to 128KB. (2007.10.29 maya)
			c = ssh2_channel_new(CHAN_TCP_WINDOW_DEFAULT, CHAN_TCP_PACKET_DEFAULT, TYPE_PORTFWD, chan_num);
			if (c == NULL) {
				// 転送チャネル内にあるソケットの解放漏れを修正 (2007.7.26 maya)
				FWD_free_channel(pvar, chan_num);
				UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
				                  "Could not open new channel. TTSSH is already opening too many channels.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
				return FALSE;
			}
			c->remote_id = remote_id;
			c->remote_window = remote_window;
			c->remote_maxpacket = remote_maxpacket;
		}
		else {
			logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": %s: buffer_get_string returns NULL. "
				"linsten_addr=%s, orig_addr=%s",
				ctype, NonNull(listen_addr), NonNull(orig_addr));
		}
		free(listen_addr);
		free(orig_addr);

	} else if (strcmp(ctype, "x11") == 0) { // port-forwarding(X11)
		// X applicationをターミナル上で実行すると、SSH2_MSG_CHANNEL_OPEN が送られてくる。
		char *orig_str;
		int orig_port;

		orig_str = buffer_get_string(&data, NULL);  // "127.0.0.1"
		orig_port = get_uint32_MSBfirst(data);
		data += 4;

		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": %s: orig_addr=%s, orig_port=%d",
			ctype, orig_str, orig_port);

		free(orig_str);

		// X server(port 6000)へ接続する。接続に失敗するとTera Term自身が切断される。
		// TODO: 将来、切断されないようにしたい。(2005.7.3 yutaka)
		FWD_X11_open(pvar, remote_id, NULL, 0, &chan_num);

		// channelをアロケートし、必要な情報（remote window size）をここで取っておく。
		// changed window size from 128KB to 32KB. (2006.3.6 yutaka)
		// changed window size from 32KB to 128KB. (2007.10.29 maya)
		c = ssh2_channel_new(CHAN_TCP_WINDOW_DEFAULT, CHAN_TCP_PACKET_DEFAULT, TYPE_PORTFWD, chan_num);
		if (c == NULL) {
			// 転送チャネル内にあるソケットの解放漏れを修正 (2007.7.26 maya)
			FWD_free_channel(pvar, chan_num);
			UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
			                  "Could not open new channel. TTSSH is already opening too many channels.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			return FALSE;
		}
		c->remote_id = remote_id;
		c->remote_window = remote_window;
		c->remote_maxpacket = remote_maxpacket;

	} else if (strcmp(ctype, "auth-agent@openssh.com") == 0) { // agent forwarding
		if (pvar->agentfwd_enable && FWD_agent_forward_confirm(pvar)) {
			c = ssh2_channel_new(CHAN_TCP_WINDOW_DEFAULT, CHAN_TCP_PACKET_DEFAULT, TYPE_AGENT, -1);
			if (c == NULL) {
				UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
				                  "Could not open new channel. TTSSH is already opening too many channels.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
				return FALSE;
			}
			c->remote_id = remote_id;
			c->remote_window = remote_window;
			c->remote_maxpacket = remote_maxpacket;
			
			SSH2_confirm_channel_open(pvar, c);
		}
		else {
			msg = buffer_init();
			if (msg == NULL) {
				// TODO: error check
				logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
				return FALSE;
			}
			buffer_put_int(msg, remote_id);
			buffer_put_int(msg, SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED);
			buffer_put_string(msg, "", 0); // description
			buffer_put_string(msg, "", 0); // language tag

			len = buffer_len(msg);
			outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_OPEN_FAILURE, len);
			memcpy(outmsg, buffer_ptr(msg), len);
			finish_send_packet(pvar);
			buffer_free(msg);

			logputs(LOG_LEVEL_VERBOSE, __FUNCTION__ ": SSH2_MSG_CHANNEL_OPEN_FAILURE was sent.");
		}

	} else {
		// unknown type(unsupported)
	}

	free(ctype);

	return(TRUE);
}


static BOOL handle_SSH2_channel_close(PTInstVar pvar)
{	
	int len;
	char *data;
	int id;
	Channel_t *c;

	// コネクション切断時に、パケットダンプをファイルへ掃き出す。
	if (LOG_LEVEL_SSHDUMP <= pvar->session_settings.LogLevel) {
		save_memdump(LOG_PACKET_DUMP);
		finish_memdump();
	}

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	id = get_uint32_MSBfirst(data);
	data += 4;
	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		// TODO:
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": channel not found. (%d)", id);
		return FALSE;
	}

	logprintf(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_CLOSE was received. local:%d remote:%d", c->self_id, c->remote_id);

	if (c->type == TYPE_SHELL || c->type == TYPE_SUBSYSTEM_GEN) {
		ssh2_channel_send_close(pvar, c);

		// TCP connection closed
		notify_closed_connection(pvar, "disconnected by server request");

	} else if (c->type == TYPE_PORTFWD) {
		// CHANNEL_CLOSE を送り返さないとリモートのchannelが開放されない
		// c.f. RFC 4253 5.3. Closing a Channel
		ssh2_channel_send_close(pvar, c);

		// 転送チャネル内にあるソケットの解放漏れを修正 (2007.7.26 maya)
		FWD_free_channel(pvar, c->local_num);

		// チャネルの解放漏れを修正 (2007.4.26 yutaka)
		ssh2_channel_delete(c);

	} else if (c->type == TYPE_SCP) {
		ssh2_channel_delete(c);

	} else if (c->type == TYPE_AGENT) {
		ssh2_channel_delete(c);

	} else {
		ssh2_channel_delete(c);

	}

	return TRUE;
}

static BOOL handle_SSH2_channel_request(PTInstVar pvar)
{
	int len;
	char *data;
	int id;
	char *request;
	int want_reply;
	int success = 0;
	Channel_t *c;

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_REQUEST was received.");

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	// ID(4) + string(any) + want_reply(1) + exit status(4)
	id = get_uint32_MSBfirst(data);
	data += 4;
	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		// TODO:
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": channel not found. (%d)", id);
		return FALSE;
	}

	request = buffer_get_string(&data, NULL);

	want_reply = data[0];
	data += 1;

	logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__
		": local=%d, remote=%d, request=%s, want_reply=%d",
		c->self_id, c->remote_id, NonNull(request), want_reply);

	if (request == NULL) {
		// request が NULL で無い事の保証
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_get_string returns NULL. (request)");
	}
	else if (strcmp(request, "exit-status") == 0) {
		// 終了コードが含まれているならば
		int estat = get_uint32_MSBfirst(data);
		success = 1;
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": exit-status=%d", estat);
	}
	else if (strcmp(request, "keepalive@openssh.com") == 0) {
		// 古い OpenSSH では SUCCESS を返しても keepalive に
		// 応答したと看做されないので FAILURE を返す。[teraterm:1278]
		success = 0;
	}

	free(request);

	if (want_reply) {
		buffer_t *msg;
		unsigned char *outmsg;
		int len;
		int type;

		if (success) {
			type = SSH2_MSG_CHANNEL_SUCCESS;
		} else {
			type = SSH2_MSG_CHANNEL_FAILURE;
		}

		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_init returns NULL.");
			return FALSE;
		}
		buffer_put_int(msg, c->remote_id);

		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, type, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		if (success) {
			logputs(LOG_LEVEL_VERBOSE, __FUNCTION__ ": SSH2_MSG_CHANNEL_SUCCESS was sent.");
		} else {
			logputs(LOG_LEVEL_VERBOSE, __FUNCTION__ ": SSH2_MSG_CHANNEL_FAILURE was sent.");
		}
	}

	return TRUE;
}


static BOOL handle_SSH2_window_adjust(PTInstVar pvar)
{	
	int len;
	char *data;
	int id;
	unsigned int adjust;
	Channel_t *c;

	logputs(LOG_LEVEL_SSHDUMP, "SSH2_MSG_CHANNEL_WINDOW_ADJUST was received.");

	// 6byte（サイズ＋パディング＋タイプ）を取り除いた以降のペイロード
	data = pvar->ssh_state.payload;
	// パケットサイズ - (パディングサイズ+1)；真のパケットサイズ
	len = pvar->ssh_state.payloadlen;

	//debug_print(80, data, len);

	// channel number
	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		// channel close後にadjust messageが遅れてやってくるケースもあるため、
		// FALSEでは返さないようにする。(2007.12.26 yutaka)
		logprintf(LOG_LEVEL_WARNING, __FUNCTION__ ": channel not found. (%d)", id);
		return TRUE;
	}

	adjust = get_uint32_MSBfirst(data);
	data += 4;

	// window sizeの調整
	c->remote_window += adjust;

	// 送り残し
	ssh2_channel_retry_send_bufchain(pvar, c);

	return TRUE;
}

// Channel_t ........... SSH2のチャネル構造体
// local_channel_num ... SSH1のローカルチャネル番号
static BOOL SSH_agent_response(PTInstVar pvar, Channel_t *c, int local_channel_num,
                               unsigned char *data, unsigned int buflen)
{
	unsigned int req_len;
	FWDChannel *fc;
	buffer_t *agent_msg;
	unsigned int *agent_request_len;
	unsigned char *response = NULL;
	unsigned int resplen = 0;


	// 分割された CHANNEL_DATA の受信に対応 (2008.11.30 maya)
	if (SSHv2(pvar)) {
		agent_msg = c->agent_msg;
		agent_request_len = &c->agent_request_len;
	}
	else {
		fc = pvar->fwd_state.channels + local_channel_num;
		agent_msg = fc->agent_msg;
		agent_request_len = &fc->agent_request_len;
	}

	if (agent_msg->len == 0) {
		req_len = get_uint32_MSBfirst(data);
		if (req_len > AGENT_MAX_MSGLEN - 4) {
			logprintf(LOG_LEVEL_NOTICE,
				__FUNCTION__ ": Agent Forwarding Error: server request is too large. "
				"size=%u, allowd max=%u.", req_len, AGENT_MAX_MSGLEN-4);
			if (pvar->session_settings.ForwardAgentNotify) {
				char title[MAX_UIMSG];
				UTIL_get_lang_msg("MSG_SSH_AGENTERROR_TITLE", pvar, "Bad agent request");
				strncpy_s(title, sizeof(title), pvar->ts->UIMsg, _TRUNCATE);
				UTIL_get_lang_msg("MSG_SSH_AGENTERROR_TOOLARGE", pvar,
					"Agent request size is too large, ignore it.");
				NotifyInfoMessage(pvar->cv, pvar->ts->UIMsg, title);
			}

			goto error;
		}

		*agent_request_len = req_len + 4;

		if (*agent_request_len > buflen) {
			buffer_put_raw(agent_msg, data, buflen);
			return TRUE;
		}
	}
	else {
		buffer_put_raw(agent_msg, data, buflen);
		if (*agent_request_len > agent_msg->len) {
			return TRUE;
		}
		data = agent_msg->buf;
	}

	agent_query(data, *agent_request_len, &response, &resplen, NULL, NULL);
	if (response == NULL || resplen < 5) {
		logprintf(LOG_LEVEL_NOTICE, __FUNCTION__ "Agent Forwarding Error: agent_query is failed.");
		goto error;
	}

	if (SSHv2(pvar)) {
		SSH2_send_channel_data(pvar, c, response, resplen, 0);
	}
	else {
		SSH_channel_send(pvar, local_channel_num, fc->remote_num,
		                 response, resplen, 0);
	}
	safefree(response);

	// 使い終わったバッファをクリア
	buffer_clear(agent_msg);
	return TRUE;

error:
	// エラー時は SSH_AGENT_FAILURE を返す
	if (SSHv2(pvar)) {
		SSH2_send_channel_data(pvar, c, SSH_AGENT_FAILURE_MSG, sizeof(SSH_AGENT_FAILURE_MSG), 0);
	}
	else {
		SSH_channel_send(pvar, local_channel_num, fc->remote_num,
		                 SSH_AGENT_FAILURE_MSG, sizeof(SSH_AGENT_FAILURE_MSG), 0);
	}
	if (response) {
		safefree(response);
	}

	// 使い終わったバッファをクリア
	buffer_clear(agent_msg);
	return TRUE;
}
