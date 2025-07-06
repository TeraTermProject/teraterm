/*
 * Copyright (c) 1998-2001, Robert O'Callahan
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

#include "ttxssh.h"
#include "util.h"
#include "resource.h"
#include "libputty.h"
#include "key.h"
#include "ttcommon.h"
#include "codeconv.h"
#include "ttxssh-version.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/engine.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/md5.h>

#include <string.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <process.h>
#include <commctrl.h>
#include <sys/utime.h>
#include <assert.h>

#include <io.h>
#ifdef _DEBUG	// KEX logging
#include <fcntl.h>
#endif


#include "ssh.h"
#include "crypt.h"
#include "fwd.h"
#include "sftp.h"
#include "kex.h"
#include "dlglib.h"
#include "win32helper.h"
#include "ttlib_types.h"
#include "makeoutputstring.h"
#include "win32helper_u8.h"

#ifndef MAX
# define MAX(a,b) (((a)>(b))?(a):(b))
# define MIN(a,b) (((a)<(b))?(a):(b))
#endif

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
// msg �� NULL �ł͖������̕ۏ؁BNULL �̏ꍇ�� "(null)" ��Ԃ��B
//
#define NonNull(msg) ((msg)?(msg):"(null)")

typedef enum {
	GetPayloadError = 0,
	GetPayloadOK = 1,
	GetPayloadTruncate = 2
} PayloadStat;

static struct global_confirm global_confirms;

static Channel_t channels[CHANNEL_MAX];

static char ssh_ttymodes[] = "\x01\x03\x02\x1c\x03\x08\x04\x15\x05\x04";

static CRITICAL_SECTION g_ssh_scp_lock;   /* SCP��M�p���b�N */

static int g_scp_sending;  /* SCP���M����? */

static void try_send_credentials(PTInstVar pvar);
static void prep_compression(PTInstVar pvar);

// �֐��v���g�^�C�v�錾
void SSH2_send_kexinit(PTInstVar pvar);
static BOOL handle_SSH2_kexinit(PTInstVar pvar);
static void SSH2_dh_kex_init(PTInstVar pvar);
static void SSH2_dh_gex_kex_init(PTInstVar pvar);
static void SSH2_ecdh_kex_init(PTInstVar pvar);
static BOOL handle_SSH2_dh_common_reply(PTInstVar pvar);
static BOOL handle_SSH2_dh_gex_reply(PTInstVar pvar);
static BOOL handle_SSH2_newkeys(PTInstVar pvar);
static BOOL handle_SSH2_service_accept(PTInstVar pvar);
static BOOL handle_SSH2_ext_info(PTInstVar pvar);
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
void SSH2_dispatch_init(PTInstVar pvar, int stage);
int SSH2_dispatch_enabled_check(unsigned char message);
void SSH2_dispatch_add_message(unsigned char message);
void SSH2_dispatch_add_range_message(unsigned char begin, unsigned char end);
int dh_pub_is_valid(DH *dh, BIGNUM *dh_pub);
static void start_ssh_heartbeat_thread(PTInstVar pvar);
void ssh2_channel_send_close(PTInstVar pvar, Channel_t *c);
static BOOL SSH_agent_response(PTInstVar pvar, Channel_t *c, int local_channel_num, unsigned char *data, unsigned int buflen);
static void ssh2_scp_get_packetlist(PTInstVar pvar, Channel_t *c, unsigned char **buf, unsigned int *buflen);
static void ssh2_scp_free_packetlist(PTInstVar pvar, Channel_t *c);
static void get_window_pixel_size(PTInstVar pvar, int *x, int *y);
static void do_SSH2_dispatch_setup_for_transfer(PTInstVar pvar);
static void ssh2_prep_userauth(PTInstVar pvar);
static void ssh2_send_newkeys(PTInstVar pvar);

// �}�N��
#define remained_payload(pvar) ((pvar)->ssh_state.payload + payload_current_offset(pvar))
#define remained_payloadlen(pvar) ((pvar)->ssh_state.payloadlen - (pvar)->ssh_state.payload_grabbed)
#define payload_current_offset(pvar) ((pvar)->ssh_state.payload_grabbed - 1)

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
                                   enum channel_type type, int local_num)
{
	int i, found;
	Channel_t *c;
	logprintf(LOG_LEVEL_VERBOSE, "%s: local_num %d", __FUNCTION__, local_num);

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
	c->remote_id = SSH_CHANNEL_INVALID;
	c->local_window = window;
	c->local_window_max = window;
	c->local_consumed = 0;
	c->local_maxpacket = maxpack;
	c->remote_window = 0;
	c->remote_maxpacket = 0;
	c->type = type;
	c->local_num = local_num;  // alloc_channel()�̕Ԓl��ۑ����Ă���
	c->bufchain = NULL;
	c->bufchain_amount = 0;
	c->bufchain_recv_suspended = FALSE;
	if (type == TYPE_SCP) {
		c->scp.state = SCP_INIT;
		c->scp.progress_window = NULL;
		c->scp.thread = INVALID_HANDLE_VALUE;
		c->scp.localfp = NULL;
		c->scp.filemtime = 0;
		c->scp.fileatime = 0;
		c->scp.pktlist_cursize = 0;
	}
	if (type == TYPE_AGENT) {
		c->agent_msg = buffer_init();
		c->agent_request_len = 0;
	}
	c->state = 0;

	return (c);
}

// remote_window�̋󂫂��Ȃ��ꍇ�ɁA����Ȃ������o�b�t�@�����X�g�i���͏��j�ւȂ��ł����B
// �����Ŋm�ۂ����������� ssh2_channel_retry_send_bufchain() �ŉ������B
static void ssh2_channel_add_bufchain(PTInstVar pvar, Channel_t *c, unsigned char *buf, unsigned int buflen)
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

	// �o�b�t�@�T�C�Y�̍��v���X�V����(�L�^�p)
	c->bufchain_amount += buflen;

	// remote_window�̋󂫂��Ȃ��̂ŁAlocal connection����̃p�P�b�g��M��
	// ��~�w�����o���B�����ɒʒm���~�܂�킯�ł͂Ȃ��B
	FWD_suspend_resume_local_connection(pvar, c, FALSE);
}

// remote_window�̋󂫂��ł�����A���X�g�Ɏc���Ă���f�[�^�����Ԃɑ���B
// ���M���ł����烁�������������B
static void ssh2_channel_retry_send_bufchain(PTInstVar pvar, Channel_t *c)
{
	bufchain_t *ch;
	unsigned int size;
	bufchain_t* ch_origin = c->bufchain;

	while (c->bufchain) {
		// �擪�����ɑ���
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

		// �o�b�t�@�T�C�Y�̍��v���X�V����(�L�^�p)
		c->bufchain_amount -= size;
	}

	// ���X���������X�g����ɂȂ�����A
	// local connection����̃p�P�b�g�ʒm���ĊJ����B
	if (ch_origin && c->bufchain == NULL) {
		FWD_suspend_resume_local_connection(pvar, c, TRUE);
	}
}

// channel close���Ƀ`���l���\���̂����X�g�֕ԋp����
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
		// SCP�����̍Ō�̏�Ԃ�ۑ�����B
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

				// SCP��M���������Ă��Ȃ���΁A���[�J���ɍ�����t�@�C���̎c�[���폜����B
				// (2017.2.12 yutaka)
				if (prev_state != SCP_CLOSING)
					remove(c->scp.localfilefull);
			}
		}
		if (c->scp.progress_window != NULL) {
			DestroyWindow(c->scp.progress_window);
			c->scp.progress_window = NULL;
		}
		if (c->scp.thread != INVALID_HANDLE_VALUE) {
			WaitForSingleObject(c->scp.thread, INFINITE);
			CloseHandle(c->scp.thread);
			c->scp.thread = INVALID_HANDLE_VALUE;
		}

		// SCP��M�̏ꍇ�̂݁ASCP�p���X�g�̊J�����s���B
		// Windows9x�ŗ���������C�������B
		if (c->scp.dir == FROMREMOTE) {
			PTInstVar pvar = c->scp.pvar;
			ssh2_scp_free_packetlist(pvar, c);
		}

		g_scp_sending = FALSE;
	}
	if (c->type == TYPE_AGENT) {
		buffer_free(c->agent_msg);
	}

	memset(c, 0, sizeof(Channel_t));
	c->used = 0;
}

// connection close���ɌĂ΂��
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
		logprintf(LOG_LEVEL_VERBOSE, "%s: invalid channel id. (%d)", __FUNCTION__, id);
		return (NULL);
	}
	c = &channels[id];
	if (c->used == 0) { // already freed
		logprintf(LOG_LEVEL_VERBOSE, "%s: channel was already freed. id:%d", __FUNCTION__, id);
		return (NULL);
	}
	return (c);
}

// SSH1�ŊǗ����Ă���channel�\���̂���ASSH2������Channel_t�֕ϊ�����B
// TODO: �����I�ɂ̓`���l���\���̂�1�ɓ�������B
// (2005.6.12 yutaka)
Channel_t *ssh2_local_channel_lookup(int local_num)
{
	int i;
	Channel_t *c;

	for (i = 0 ; i < CHANNEL_MAX ; i++) {
		c = &channels[i];
		if (c->type != TYPE_PORTFWD)
			continue;
		if (c->local_num == local_num)
			return (c);
	}
	return (NULL);
}

//
// SSH heartbeat mutex
//
// TTSSH�� thread-safe �ł͂Ȃ����߁A�}���`�X���b�h����̃p�P�b�g���M�͂ł��Ȃ��B
// �V���O���X���b�h�ł̓R���e�L�X�g�X�C�b�`���������邱�Ƃ͂Ȃ����߁A
// ���b�N�����K�v���Ȃ����߁A�폜����B(2007.12.26 yutaka)
//
static CRITICAL_SECTION g_ssh_heartbeat_lock;   /* ����M�p���b�N */

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
#define LOGDUMP L"ssh2connect.log"
#define LOG_PACKET_DUMP L"ssh2packet.log"
#define SENDTOME L"Please send '" LOGDUMP L"' file to Tera Term developer team."

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

/* �_���v���C�����t�H�[�}�b�g�\������ */
static void displine_memdump(FILE *fp, int addr, int *bytes, int byte_cnt)
{
	int i, c;

	/* �擪�̃A�h���X�\�� */
	fprintf(fp, "%08X : ", addr);

	/* �o�C�i���\���i4�o�C�g���Ƃɋ󔒂�}���j*/
	for (i = 0 ; i < byte_cnt ; i++) {
		if (i > 0 && i % 4 == 0)
			fprintf(fp, " ");

		fprintf(fp, "%02X", bytes[i]);
	}

	/* ASCII�\�������܂ł̋󔒂�₤ */
	fprintf(fp, "   %*s%*s", (16-byte_cnt)*2+1, " ", (16-byte_cnt+3)/4, " ");

	/* ASCII�\�� */
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


/* �_���v���[�`�� */
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

	// initialize����ĂȂ��Ƃ��͉��������ɖ߂�B(2005.4.3 yutaka)
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

void save_memdump(wchar_t *filename)
{
	FILE *fp;
	int i;
	time_t t;
	struct tm *tm;
	wchar_t *dumpfile = NULL;

	dumpfile = get_log_dir_relative_nameW(filename);
	_wfopen_s(&fp, dumpfile, L"w");
	free(dumpfile);
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

void log_kex_key(PTInstVar pvar, const BIGNUM *secret)
{
#ifdef _DEBUG	// KEX logging
	int fd, i;
	unsigned char buff[4], *cookie;
	char *hexstr;
	wchar_t *fname;

	if (pvar->settings.KexKeyLogging && pvar->settings.KexKeyLogFile[0] != 0) {
		hexstr = BN_bn2hex(secret);
		if (hexstr == NULL) {
			return;
		}

		fname = get_log_dir_relative_nameW(pvar->settings.KexKeyLogFile);
		if (fname == NULL) {
			OPENSSL_free(hexstr);
			return;
		}

		fd = _wopen(fname,
			_O_RDWR | _O_APPEND | _O_CREAT | _O_TEXT,
			_S_IREAD | _S_IWRITE);
		if (fd >= 0) {
			cookie = pvar->crypt_state.client_cookie;
			for (i=0; i<16; i++) {
				_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%02x", cookie[i]);
				_write(fd, buff, 2);
			}
			_write(fd, " ", 1);
			_write(fd, hexstr, strlen(hexstr));
			_write(fd, "\n", 1);
			_close(fd);
		}
		free(fname);
		OPENSSL_free(hexstr);
	}
#endif
}

static unsigned int get_predecryption_amount(PTInstVar pvar)
{
	static int small_block_decryption_sizes[] = { 5, 5, 6, 6, 8 };

	if (SSHv1(pvar)) {
		return 0;
	} else {
		unsigned int block_size = CRYPT_get_decryption_block_size(pvar);

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
			pvar->ssh_state.decompress_stream.next_out - pvar->ssh_state.postdecompress_inbuf;

		while (limit > cur_decompressed_bytes) {
			int result;

			pvar->ssh_state.payload = pvar->ssh_state.postdecompress_inbuf + 1;
			if (pvar->ssh_state.postdecompress_inbuflen == cur_decompressed_bytes) {
				buf_ensure_size(&pvar->ssh_state.postdecompress_inbuf,
				                &pvar->ssh_state.postdecompress_inbuflen,
				                min(limit, cur_decompressed_bytes * 2));
			}

			pvar->ssh_state.decompress_stream.next_out =
				pvar->ssh_state.postdecompress_inbuf + cur_decompressed_bytes;
			pvar->ssh_state.decompress_stream.avail_out =
				min(limit, pvar->ssh_state.postdecompress_inbuflen) - cur_decompressed_bytes;

			result = inflate(&pvar->ssh_state.decompress_stream, Z_SYNC_FLUSH);
			cur_decompressed_bytes =
				pvar->ssh_state.decompress_stream.next_out - pvar->ssh_state.postdecompress_inbuf;

			switch (result) {
			case Z_OK:
				break;
			case Z_BUF_ERROR:
				pvar->ssh_state.payloadlen = cur_decompressed_bytes;
				return cur_decompressed_bytes;
			default:
				UTIL_get_lang_msg("MSG_SSH_INVALID_COMPDATA_ERROR", pvar,
				                  "Invalid compressed data in received packet");
				notify_fatal_error(pvar, pvar->UIMsg, TRUE);
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
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
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
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
			            pvar->ssh_state.payload_grabbed, in_buffer);
			notify_fatal_error(pvar, buf, TRUE);
			return FALSE;
		} else {
			return TRUE;
		}
	}
}

static PayloadStat get_byte_from_payload(PTInstVar pvar, unsigned char *val)
{
	unsigned char *data;

	data = remained_payload(pvar);
	if (!grab_payload(pvar, 1)) {
		return GetPayloadError;
	}

	*val = *data;
	return GetPayloadOK;
}
#define get_boolean_from_payload(pvar, val) get_byte_from_payload(pvar, val)

static PayloadStat get_bytearray_from_payload(PTInstVar pvar, unsigned char *buff, unsigned int len)
{
	unsigned char *data;

	data = remained_payload(pvar);
	if (!grab_payload(pvar, len)) {
		return GetPayloadError;
	}

	memcpy_s(buff, len, data, len);
	return GetPayloadOK;
}

static PayloadStat get_uint32_from_payload(PTInstVar pvar, unsigned int *val)
{
	unsigned char *data;

	data = remained_payload(pvar);
	if (!grab_payload(pvar, 4)) {
		return  GetPayloadError;
	}

	*val = get_uint32(data);
	return GetPayloadOK;
}

static PayloadStat get_string_from_payload(
	PTInstVar pvar, unsigned char *buff, unsigned int bufflen, unsigned int *len, BOOL null_terminate)
{
	unsigned int size;
	unsigned char *data;

	if (!get_uint32_from_payload(pvar, &size)) {
		return GetPayloadError;
	}

	data = remained_payload(pvar);
	if (!grab_payload(pvar, size)) {
		return GetPayloadError;
	}

	*len = size;

	if (size < bufflen) {
		memcpy_s(buff, bufflen, data, size);
		if (null_terminate) {
			buff[size] = 0;
		}
		return GetPayloadOK;
	}
	else if (size == bufflen) {
		memcpy_s(buff, bufflen, data, bufflen);
		if (null_terminate) {
			buff[bufflen-1] = 0;
			return GetPayloadTruncate;
		}
		else {
			return GetPayloadOK;
		}
	}
	else {
		memcpy_s(buff, bufflen, data, bufflen);
		if (null_terminate) {
			buff[bufflen-1] = 0;
		}
		return GetPayloadTruncate;
	}
}
#define get_namelist_from_payload(pvar, buff, bufflen, size) get_string_from_payload(pvar, buff, bufflen, size, TRUE)

static PayloadStat get_mpint_from_payload(PTInstVar pvar, BIGNUM *bn)
{
	unsigned int len;
	unsigned char *data;

	if (!get_uint32_from_payload(pvar, &len)) {
		return GetPayloadError;
	}

	data = remained_payload(pvar);
	if (!grab_payload(pvar, len)) {
		return GetPayloadError;
	}

	BN_bin2bn(data, len, bn);

	return GetPayloadOK;
}

#define do_crc(buf, len) (~(uint32)crc32(0xFFFFFFFF, (buf), (len)))

/*
 * Decrypt the payload, checksum it, eat the padding, get the packet type and return it.
 * 'data' points to the start of the packet --- its length field.
 * 'len' is the length of the * payload + padding (+ length of CRC for SSHv1).
 * 'padding' is the length of the padding alone.
 */
static int prep_packet_ssh1(PTInstVar pvar, char *data, unsigned int len, unsigned int padding)
{
	pvar->ssh_state.payload = data + 4;
	pvar->ssh_state.payloadlen = len;

	if (CRYPT_detect_attack(pvar, pvar->ssh_state.payload, len)) {
		UTIL_get_lang_msg("MSG_SSH_COREINS_ERROR", pvar, "'CORE insertion attack' detected.  Aborting connection.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
	}

	CRYPT_decrypt(pvar, pvar->ssh_state.payload, len);
	/* PKT guarantees that the data is always 4-byte aligned */
	if (do_crc(pvar->ssh_state.payload, len - 4) != get_uint32_MSBfirst(pvar->ssh_state.payload + len - 4)) {
		UTIL_get_lang_msg("MSG_SSH_CORRUPTDATA_ERROR", pvar, "Detected corrupted data; connection terminating.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		return SSH_MSG_NONE;
	}

	pvar->ssh_state.payload += padding;
	pvar->ssh_state.payloadlen -= padding + 4;

	pvar->ssh_state.payload_grabbed = 0;

	if (pvar->ssh_state.decompressing) {
		if (pvar->ssh_state.decompress_stream.avail_in != 0) {
			UTIL_get_lang_msg("MSG_SSH_DECOMPRESS_ERROR", pvar,
			                  "Internal error: a packet was not fully decompressed.\n"
			                  "This is a bug, please report it.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
		}

		pvar->ssh_state.decompress_stream.next_in = pvar->ssh_state.payload;
		pvar->ssh_state.decompress_stream.avail_in = pvar->ssh_state.payloadlen;
		pvar->ssh_state.decompress_stream.next_out = pvar->ssh_state.postdecompress_inbuf;
		pvar->ssh_state.payloadlen = -1;
	} else {
		pvar->ssh_state.payload++;
	}

	if (!grab_payload_limited(pvar, 1)) {
		return SSH_MSG_NONE;
	}

	pvar->ssh_state.receiver_sequence_number++;

	return pvar->ssh_state.payload[-1];
}

/*
 * �p�P�b�g�����ׂ̈̈ȉ��̏������s���B(SSHv2�p)
 * �E�f�[�^����
 * �EMAC �̌���
 * �Epadding ����菜��
 * �E���b�Z�[�W�^�C�v�𔻕ʂ��ĕԂ�
 *
 * ����:
 *   data - ssh �p�P�b�g�̐擪���w���|�C���^
 *   len - �p�P�b�g�� (�擪�̃p�P�b�g���̈�(4�o�C�g)���������l)
 *   aadlen - �Í�������Ă��Ȃ����F�؂̑ΏۂƂȂ��Ă���f�[�^�̒���
 *            chacha20-poly1305 �ł͈Í��������p�P�b�g�������̒���
 *   authlen - �F�؃f�[�^(AEAD tag)��
 */

static int prep_packet_ssh2(PTInstVar pvar, char *data, unsigned int len, unsigned int aadlen, unsigned int authlen)
{
	unsigned int padding;

	if (authlen > 0) {
		if (!CRYPT_decrypt_aead(pvar, data, len, aadlen, authlen)) {
			UTIL_get_lang_msg("MSG_SSH_CORRUPTDATA_ERROR", pvar, "Detected corrupted data; connection terminating.");
			notify_fatal_error(pvar, pvar->UIMsg, TRUE);
			return SSH_MSG_NONE;
		}
	}
	else if (aadlen > 0) {
		// EtM �̏ꍇ�͐�� MAC �̌��؂��s��
		if (!CRYPT_verify_receiver_MAC(pvar, pvar->ssh_state.receiver_sequence_number, data, len + 4, data + len + 4)) {
			UTIL_get_lang_msg("MSG_SSH_CORRUPTDATA_ERROR", pvar, "Detected corrupted data; connection terminating.");
			notify_fatal_error(pvar, pvar->UIMsg, TRUE);
			return SSH_MSG_NONE;
		}

		// �p�P�b�g������(�擪4�o�C�g)�͈Í�������Ă��Ȃ��̂ŁA�������X�L�b�v���ĕ�������B
		CRYPT_decrypt(pvar, data + 4, len);
	}
	else {
		// E&M �ł͐擪���������O��������Ă���B
		// ���O�������ꂽ�������擾����B
		unsigned int already_decrypted = get_predecryption_amount(pvar);

		// ���O�������ꂽ�������X�L�b�v���āA�c��̕����𕜍�����B
		CRYPT_decrypt(pvar, data + already_decrypted, (4 + len) - already_decrypted);

		// E&M �ł͕������ MAC �̌��؂��s���B
		if (!CRYPT_verify_receiver_MAC(pvar, pvar->ssh_state.receiver_sequence_number, data, len + 4, data + len + 4)) {
			UTIL_get_lang_msg("MSG_SSH_CORRUPTDATA_ERROR", pvar, "Detected corrupted data; connection terminating.");
			notify_fatal_error(pvar, pvar->UIMsg, TRUE);
			return SSH_MSG_NONE;
		}
	}

	// �p�f�B���O���̎擾
	padding = (unsigned int) data[4];

	// �p�P�b�g��(4�o�C�g) �����ƃp�f�B���O��(1�o�C�g)�������X�L�b�v���� SSH �y�C���[�h�̐擪
	pvar->ssh_state.payload = data + 4 + 1;

	// �p�f�B���O������(1�o�C�g)�ƃp�f�B���O�����������ۂ̃y�C���[�h��
	pvar->ssh_state.payloadlen = len - 1 - padding;

	pvar->ssh_state.payload_grabbed = 0;

	// data compression
	if (pvar->ssh2_keys[MODE_IN].comp.enabled &&
	   (pvar->stoc_compression == COMP_ZLIB ||
	    pvar->stoc_compression == COMP_DELAYED && pvar->userauth_success)) {

		if (pvar->decomp_buffer == NULL) {
			pvar->decomp_buffer = buffer_init();
			if (pvar->decomp_buffer == NULL)
				return SSH_MSG_NONE;
		}
		// ��x�m�ۂ����o�b�t�@�͎g���񂷂̂ŏ�������Y�ꂸ�ɁB
		buffer_clear(pvar->decomp_buffer);

		// packet size��padding����菜�����y�C���[�h�����݂̂�W�J����B
		buffer_decompress(&pvar->ssh_state.decompress_stream,
		                  pvar->ssh_state.payload,
		                  pvar->ssh_state.payloadlen,
		                  pvar->decomp_buffer);

		// �|�C���^�̍X�V�B
		pvar->ssh_state.payload = buffer_ptr(pvar->decomp_buffer);
		pvar->ssh_state.payload++;
		pvar->ssh_state.payloadlen = buffer_len(pvar->decomp_buffer);
	} else {
		pvar->ssh_state.payload++;
	}

	if (!grab_payload_limited(pvar, 1)) {
		return SSH_MSG_NONE;
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
		                (int)(len + 30 + CRYPT_get_sender_MAC_size(pvar) +
		                CRYPT_get_encryption_block_size(pvar)));
		buf = pvar->ssh_state.outbuf + 12;
	}

	buf[0] = (unsigned char) type;
	return buf + 1;
}


// ���M���g���C�֐��̒ǉ�
//
// WinSock�� send() �̓o�b�t�@�T�C�Y(len)�������Ȃ��l�𐳏펞�ɕԂ��Ă���
// ���Ƃ�����̂ŁA���̏ꍇ�̓G���[�Ƃ��Ȃ��B
// ����ɂ��ATCP�R�l�N�V�����ؒf�̌댟�o��h���B
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
				// send()�̕Ԓl��0�����ŁA���G���[�ԍ��� 10000 �����̏ꍇ�́A
				// �����������̂ƌ��Ȃ��B
				// PuTTY 0.58�̎������Q�l�B
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
	// �p�P�b�g���M��Ƀo�b�t�@���g���܂킷���߁A�u���b�L���O�ő��M���Ă��܂��K�v������B
	// �m���u���b�L���O�ő��M����WSAEWOULDBLOCK���Ԃ��Ă����ꍇ�A���̃o�b�t�@�͑��M��������
	// �܂ŕێ����Ă����Ȃ��Ă͂Ȃ�Ȃ��B(2007.10.30 yutaka)
	u_long do_block = 0;
	int code = 0;
	char *kind = NULL, buf[256];

	if ((pvar->PWSAAsyncSelect) (pvar->socket, pvar->NotificationWindow, 0, 0) == SOCKET_ERROR) {
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
	                             pvar->notification_events) == SOCKET_ERROR) {
		code = WSAGetLastError();
		kind = "WSAAsyncSelect2";
		goto error;
	}
	return TRUE;

error:
	UTIL_get_lang_msg("MSG_SSH_SEND_PKT_ERROR", pvar,
	                  "A communications error occurred while sending an SSH packet.\n"
	                  "The connection will close. (%s:%d)");
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
	            kind, code);
	notify_fatal_error(pvar, buf, TRUE);
	return FALSE;
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
			                (int)(len + (len >> 6) + 50 + CRYPT_get_sender_MAC_size(pvar)));
			pvar->ssh_state.compress_stream.next_in = pvar->ssh_state.precompress_outbuf;
			pvar->ssh_state.compress_stream.avail_in = len;
			pvar->ssh_state.compress_stream.next_out = pvar->ssh_state.outbuf + 12;
			pvar->ssh_state.compress_stream.avail_out = pvar->ssh_state.outbuflen - 12;

			if (deflate(&pvar->ssh_state.compress_stream, Z_SYNC_FLUSH) != Z_OK) {
				UTIL_get_lang_msg("MSG_SSH_COMP_ERROR", pvar,
				                  "An error occurred while compressing packet data.\n"
				                  "The connection will close.");
				notify_fatal_error(pvar, pvar->UIMsg, TRUE);
				return;
			}
		}

		len = pvar->ssh_state.outbuflen - 12 - pvar->ssh_state.compress_stream.avail_out;
	}

	if (SSHv1(pvar)) {
		unsigned int padding = 8 - ((len + 4) % 8);

		data = pvar->ssh_state.outbuf + 8 - padding;
		data_length = padding + len + 8;

		set_uint32(data, len + 4);
		if (CRYPT_get_receiver_cipher(pvar) != SSH_CIPHER_NONE) {
			CRYPT_set_random_data(pvar, data + 4, padding);
		} else {
			memset(data + 4, 0, padding);
		}
		set_uint32(data + data_length - 4, do_crc(data + 4, data_length - 8));
		CRYPT_encrypt(pvar, data + 4, data_length - 4);
	} else { //for SSH2(yutaka)
		unsigned int block_size = CRYPT_get_encryption_block_size(pvar);
		unsigned int packet_length;
		unsigned int encryption_size;
		unsigned int padding_size;
		BOOL ret;
		struct Mac *mac = &pvar->ssh2_keys[MODE_OUT].mac;
		struct Enc *enc = &pvar->ssh2_keys[MODE_OUT].enc;
		unsigned int aadlen = 0, maclen = 0, authlen = 0;

		/*
		 �f�[�^�\��
		 pvar->ssh_state.outbuf:
		 offset: 0 1 2 3 4 5 6 7 8 9 10 11 12 ...         EOD
		         <--ignore---> ^^^^^^^^    <---- payload --->
		                       packet length

		                                ^^padding_size

		                       <---------------------------->
		                          SSH2 sending data on TCP

		 NOTE:
		   payload = type(1) + raw-data
		   len = ssh_state.outgoing_packet_len = payload size
		 */
		// �p�P�b�g���k���L���̏ꍇ�A�p�P�b�g�����k���Ă��瑗�M�p�P�b�g���\�z����B(2005.7.9 yutaka)
		// support of "Compression delayed" (2006.6.23 maya)
		if ((pvar->ctos_compression == COMP_ZLIB ||
		     pvar->ctos_compression == COMP_DELAYED && pvar->userauth_success) &&
		    pvar->ssh2_keys[MODE_OUT].comp.enabled) {
			// ���̃o�b�t�@�� packet-length(4) + padding(1) + payload(any) �������B
			msg = buffer_init();
			if (msg == NULL) {
				// TODO: error check
				logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
				return;
			}

			// ���k�Ώۂ̓w�b�_�������y�C���[�h�̂݁B
			buffer_append(msg, "\0\0\0\0\0", 5);  // 5 = packet-length(4) + padding(1)
			if (buffer_compress(&pvar->ssh_state.compress_stream, pvar->ssh_state.outbuf + 12, len, msg) == -1) {
				UTIL_get_lang_msg("MSG_SSH_COMP_ERROR", pvar,
				                  "An error occurred while compressing packet data.\n"
				                  "The connection will close.");
				notify_fatal_error(pvar, pvar->UIMsg, TRUE);
				return;
			}
			data = buffer_ptr(msg);
			len = buffer_len(msg) - 5;  // 'len' is overwritten.

		} else {
			// �����k
			data = pvar->ssh_state.outbuf + 7;
		}

		// ���M�p�P�b�g�\�z(input parameter: data, len)
		if (block_size < 8) {
			block_size = 8;
		}

		if (enc) {
			authlen = enc->auth_len;
		}

		if (mac && mac->etm || authlen > 0) {
			// �Í����Ώۂł͂Ȃ����AMAC �̑ΏۂƂȂ�p�P�b�g�������̒���
			// �܂��� chacha20-poly1305 �ňÍ��������p�P�b�g�������̒���
			// cf. PKT_recv ���̃R�����g
			aadlen = 4;
		}

		packet_length = 1 + len; // �p�f�B���O���̃T�C�Y + �y�C���[�h��
		encryption_size = 4 + packet_length - aadlen; // �p�P�b�g���̃T�C�Y + packet_length - addlen
		padding_size = block_size - (encryption_size % block_size);
		if (padding_size < 4)
			padding_size += block_size;
		packet_length += padding_size;
		encryption_size += padding_size;
		set_uint32(data, packet_length);
		data[4] = (unsigned char) padding_size;
		if (msg) {
			// �p�P�b�g���k�̏ꍇ�A�o�b�t�@���g������B(2011.6.10 yutaka)
			buffer_append_space(msg, padding_size + EVP_MAX_MD_SIZE);
			// realloc()�����ƁA�|�C���^���ς��\��������̂ŁA�ēx��蒼���B
			data = buffer_ptr(msg);
		}

		CRYPT_set_random_data(pvar, data + 5 + len, padding_size);

		if (authlen > 0) {
			// �p�P�b�g�Í����� MAC �̌v�Z
			CRYPT_encrypt_aead(pvar, data, encryption_size, aadlen, authlen);
			maclen = authlen;
		}
		else if (aadlen) {
			// �p�P�b�g�Í����iaadlen����낾���j
			CRYPT_encrypt(pvar, data + aadlen, encryption_size);

			// EtM �ł͈Í������ MAC ���v�Z����
			ret = CRYPT_build_sender_MAC(pvar, pvar->ssh_state.sender_sequence_number,
			                             data, aadlen + encryption_size, data + aadlen + encryption_size);
			if (ret) {
				maclen = CRYPT_get_sender_MAC_size(pvar);
			}
		}
		else {
			// E&M �ł͈Í����O�� MAC ���v�Z����
			ret = CRYPT_build_sender_MAC(pvar, pvar->ssh_state.sender_sequence_number,
			                             data, encryption_size, data + encryption_size);
			if (ret) {
				maclen = CRYPT_get_sender_MAC_size(pvar);
			}

			// �p�P�b�g�Í���
			CRYPT_encrypt(pvar, data, encryption_size);
		}

		data_length = encryption_size + aadlen + maclen;

		logprintf(150,
		          "%s: built packet info: "
		          "aadlen:%d, enclen:%d, padlen:%d, datalen:%d, maclen:%d, "
		          "Encrypt Mode:%s, MAC mode:%s",
		          __FUNCTION__,
		          aadlen, encryption_size, padding_size, data_length, maclen,
		          authlen ? "AEAD" : "not AEAD", aadlen ? "EtM" : "E&M");
	}

	send_packet_blocking(pvar, data, data_length);

	buffer_free(msg);

	pvar->ssh_state.sender_sequence_number++;

	// ���M�������L�^
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

	// retry count�̒ǉ� (2005.7.15 yutaka)
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
			// �S�Ă̌��������I�����
			safefree(pvar->pageant_key);
		}
		else {
			// �܂���������
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

			AUTH_set_TIS_mode(pvar, pvar->ssh_state.payload + 4, len, 0);
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

		// ���b�Z�[�W�� SSH2_MSG_IGNORE �̎��͉������Ȃ�
		// Cisco ���[�^�΍� (2006.11.28 maya)
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
		strncpy_s(uimsg, sizeof(uimsg), pvar->UIMsg, _TRUNCATE);
		explanation = uimsg;
	}

	if (description != NULL) {
		UTIL_get_lang_msg("MSG_SSH_SERVER_DISCON_ERROR", pvar,
		                  "Server disconnected with message '%s'%s");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->UIMsg, description,
		            explanation);
	} else {
		UTIL_get_lang_msg("MSG_SSH_SERVER_DISCON_NORES_ERROR", pvar,
		                  "Server disconnected (no reason given).%s");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->UIMsg, explanation);
	}

	if (SSHv2(pvar)) {
		// SSH2_MSG_DISCONNECT ���󂯎�������Ƃ͉������M���Ă͂����Ȃ�
		notify_fatal_error(pvar, buf, FALSE);
	}
	else {
		// SSH1 �̏ꍇ�̎d�l��������Ȃ��̂ŁA�ȑO�̂܂܂ɂ��Ă���
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

	// �n�[�g�r�[�g�E�X���b�h�̊J�n (2004.12.11 yutaka)
	start_ssh_heartbeat_thread(pvar);

	return FALSE;
}

/*
 * SSH1�T�[�o���瑗���Ă��������`�F�b�N���āA�Ō��known_hosts�_�C�A���O��\������B
 */
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
	int ret;

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

	// SSH1 �T�[�o�́A�T�|�[�g����Ă���F�ؕ����𑗂��Ă���
	// RSA ���L���Ȃ� PAGEANT ��L���ɂ���
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

	ret = HOSTS_check_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, &hostkey);
	if (ret == TRUE) {
		// known_hosts�_�C�A���O�̌Ăяo���͕s�v�Ȃ̂ŁA�����̏��������s����B
		SSH_notify_host_OK(pvar);

	} else {
		// known_hosts�_�C�A���O�̌Ăяo�����̂ŁA�ȍ~�A�������Ȃ��B

	}

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
			// �T�[�o�� 1.99 �Ń��[�U�� SSH2 ��I�����Ă���̂Ȃ��
			// 2.0 �ڑ��Ƃ���
			pvar->protocol_major = 2;
			pvar->protocol_minor = 0;
			return 0;
		}

		if (pvar->settings.ssh_protocol_version == 2) {
			// �o�[�W�����Ⴂ
			return -1;
		}

		if (pvar->protocol_minor > 5) {
			pvar->protocol_minor = 5;
		}

		return 0;

	// for SSH2(yutaka)
	case 2:
		if (pvar->settings.ssh_protocol_version == 1) {
			// �o�[�W�����Ⴂ
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

	// known_hosts�t�@�C������z�X�g���J�����ǂ݂��Ă���
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
		enque_handler(pvar, SSH2_MSG_EXT_INFO, handle_SSH2_ext_info);
		enque_handler(pvar, SSH2_MSG_USERAUTH_SUCCESS, handle_SSH2_userauth_success);
		enque_handler(pvar, SSH2_MSG_USERAUTH_FAILURE, handle_SSH2_userauth_failure);
		enque_handler(pvar, SSH2_MSG_USERAUTH_BANNER, handle_SSH2_userauth_banner);
		enque_handler(pvar, SSH2_MSG_USERAUTH_INFO_REQUEST, handle_SSH2_userauth_msg60);

		enque_handler(pvar, SSH2_MSG_UNIMPLEMENTED, handle_unimplemented);

		// ���[�U�F�،�̃f�B�X�p�b�`���[�`��
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
				notify_fatal_error(pvar, pvar->UIMsg, TRUE);
			}
			else if ((negotiate = negotiate_protocol(pvar)) == 1) {
				UTIL_get_lang_msg("MSG_SSH_VERSION_ERROR", pvar,
				                  "This program does not understand the server's version of the protocol.");
				notify_fatal_error(pvar, pvar->UIMsg, TRUE);
			}
			else if (negotiate == -1) {
				UTIL_get_lang_msg("MSG_SSH_VERSION_MISMATCH", pvar,
				                  "Protocol version mismatch. server:%d.%d client:%d");
				_snprintf_s(uimsg, sizeof(uimsg), _TRUNCATE, pvar->UIMsg,
				            pvar->protocol_major, pvar->protocol_minor, pvar->settings.ssh_protocol_version);
				notify_fatal_error(pvar, uimsg, TRUE);
			}
			else {
				char TTSSH_ID[1024];
				int TTSSH_ID_len;

				// SSH �o�[�W������ teraterm ���ɃZ�b�g����
				// SCP �R�}���h�̂��� (2008.2.3)
				pvar->cv->isSSH = pvar->protocol_major;

				// �������g�̃o�[�W�������擾���� (2005.3.3)
				_snprintf_s(TTSSH_ID, sizeof(TTSSH_ID), _TRUNCATE,
				            "SSH-%d.%d-TTSSH/%d.%d.%d Win32\r\n",
				            pvar->protocol_major, pvar->protocol_minor,
				            TTSSH_VERSION_MAJOR, TTSSH_VERSION_MINOR, TTSSH_VERSION_PATCH);
				TTSSH_ID_len = strlen(TTSSH_ID);

				// for SSH2
				// �N���C�A���g�o�[�W�����̕ۑ��i���s�͎�菜�����Ɓj
				strncpy_s(pvar->client_version_string, sizeof(pvar->client_version_string),
				          TTSSH_ID, _TRUNCATE);

				// �T�[�o�o�[�W�����̕ۑ��i���s�͎�菜�����Ɓj(2005.3.9)
				_snprintf_s(pvar->server_version_string,
				            sizeof(pvar->server_version_string), _TRUNCATE,
				            "%s", pvar->ssh_state.server_ID);

				// �T�[�o�o�[�W�����̃`�F�b�N
				server_version_check(pvar);

				if ((pvar->Psend) (pvar->socket, TTSSH_ID, TTSSH_ID_len, 0) != TTSSH_ID_len) {
					UTIL_get_lang_msg("MSG_SSH_SEND_ID_ERROR", pvar,
					                  "An error occurred while sending the SSH ID string.\n"
					                  "The connection will close.");
					notify_fatal_error(pvar, pvar->UIMsg, TRUE);
				} else {
					// ���s�̏���
					chop_newlines(pvar->client_version_string);
					logprintf(LOG_LEVEL_VERBOSE, "Sent client identification string: %s", pvar->client_version_string);

					push_memdump("server ID", NULL, pvar->server_version_string, strlen(pvar->server_version_string));
					push_memdump("client ID", NULL, pvar->client_version_string, strlen(pvar->client_version_string));

					// SSH�n���h���̓o�^���s��
					init_protocol(pvar);

					SSH2_dispatch_init(pvar, 1);
					SSH2_dispatch_add_message(SSH2_MSG_KEXINIT);
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
		// �ʒm���鑊��channel��������Ȃ��̂ŉ����ł��Ȃ�
	}
	*/

	return TRUE;
}



// �n���h�����O���郁�b�Z�[�W�����肷��

#define HANDLE_MESSAGE_MAX 30
static unsigned char handle_messages[HANDLE_MESSAGE_MAX];
static int handle_message_count = 0;
static int handle_message_stage = 0;

void SSH2_dispatch_init(PTInstVar pvar, int stage)
{
	handle_message_count = 0;
	handle_message_stage = stage;

	// DISCONNECT�͏�Ɏ󂯓����
	SSH2_dispatch_add_message(SSH2_MSG_DISCONNECT);

	// Strict KEX ���L���A������� KEX ���͎󂯓���郁�b�Z�[�W�𐧌�����
	if (pvar->server_strict_kex && pvar->kex_status == 0) {
		return;
	}

	SSH2_dispatch_add_message(SSH2_MSG_IGNORE);
	SSH2_dispatch_add_message(SSH2_MSG_DEBUG);
	SSH2_dispatch_add_message(SSH2_MSG_UNIMPLEMENTED);
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
		logprintf(LOG_LEVEL_ERROR, "%s: too many handlers. handlers:%d, max:%d", __FUNCTION__,
			handle_message_count, HANDLE_MESSAGE_MAX);
		return;
	}

	// ���łɓo�^����Ă��郁�b�Z�[�W�͒ǉ����Ȃ�
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

void SSH1_handle_packet(PTInstVar pvar, char *data, unsigned int len, unsigned int padding)
{
	unsigned char message = prep_packet_ssh1(pvar, data, len, padding);

	// SSH�̃��b�Z�[�W�^�C�v���`�F�b�N
	if (message != SSH_MSG_NONE) {
		// ���b�Z�[�W�^�C�v�ɉ������n���h�����N��
		SSHPacketHandler handler = get_handler(pvar, message);

		if (handler == NULL) {
			char buf[1024];

			UTIL_get_lang_msg("MSG_SSH_UNEXP_MSG_ERROR", pvar, "Unexpected packet type received: %d");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg, message, handle_message_stage);
			notify_fatal_error(pvar, buf, TRUE);
		} else {
			if (!handler(pvar)) {
				deque_handlers(pvar, message);
			}
		}
	}
}

void SSH2_handle_packet(PTInstVar pvar, char *data, unsigned int len, unsigned int aadlen, unsigned int authlen)
{
	unsigned char message = prep_packet_ssh2(pvar, data, len, aadlen, authlen);

	// SSH�̃��b�Z�[�W�^�C�v���`�F�b�N
	if (message != SSH_MSG_NONE) {
		// ���b�Z�[�W�^�C�v�ɉ������n���h�����N��
		SSHPacketHandler handler = get_handler(pvar, message);

		// �z��O�̃��b�Z�[�W�^�C�v������������A�{�[�g������B
		if (!SSH2_dispatch_enabled_check(message) || handler == NULL) {
			char buf[1024];

			UTIL_get_lang_msg("MSG_SSH_UNEXP_MSG2_ERROR", pvar, "Unexpected SSH2 message(%d) on current stage(%d)");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg, message, handle_message_stage);
			notify_fatal_error(pvar, buf, TRUE);
			return;
		}

		if (handler == NULL) {
			unsigned char *outmsg = begin_send_packet(pvar, SSH2_MSG_UNIMPLEMENTED, 4);

			set_uint32(outmsg, pvar->ssh_state.receiver_sequence_number - 1);
			finish_send_packet(pvar);

			logprintf(LOG_LEVEL_VERBOSE, "%s: SSH2_MSG_UNIMPLEMENTED was sent.", __FUNCTION__);
			/* XXX need to decompress incoming packet, but how? */
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
	enque_handler(pvar, SSH_MSG_CHANNEL_INPUT_EOF, handle_channel_input_eof);
	enque_handler(pvar, SSH_MSG_CHANNEL_OUTPUT_CLOSED, handle_channel_output_eof);
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
	notify_nonfatal_error(pvar, pvar->UIMsg);
	return handle_pty_success(pvar);
}

static void prep_pty(PTInstVar pvar)
{
	int len = strlen(pvar->ts->TermType);
	unsigned char *outmsg = begin_send_packet(pvar, SSH_CMSG_REQUEST_PTY, 4 + len + 16 + sizeof(ssh_ttymodes));
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
	static const SSHPacketHandler handlers[] = { handle_agent_request_success, handle_agent_request_failure };

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
	if (deflateInit(&pvar->ssh_state.compress_stream, pvar->ssh_state.compression_level) != Z_OK) {
		UTIL_get_lang_msg("MSG_SSH_SETUP_COMP_ERROR", pvar,
		                  "An error occurred while setting up compression.\n"
		                  "The connection will close.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		return;
	} else {
		// SSH2�ł͈��k�E�W�J������SSH1�Ƃ͕ʂɍs���̂ŁA���L�t���O�͗��Ƃ��Ă����B(2005.7.9 yutaka)
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
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		return;
	} else {
		// SSH2�ł͈��k�E�W�J������SSH1�Ƃ͕ʂɍs���̂ŁA���L�t���O�͗��Ƃ��Ă����B(2005.7.9 yutaka)
		if (SSHv2(pvar)) {
			pvar->ssh_state.decompressing = FALSE;
		} else {
			pvar->ssh_state.decompressing = TRUE;
		}

		buf_ensure_size(&pvar->ssh_state.postdecompress_inbuf, &pvar->ssh_state.postdecompress_inbuflen, 1000);
	}
}

static void enable_compression(PTInstVar pvar)
{
	enable_send_compression(pvar);
	enable_recv_compression(pvar);

	// SSH2�ł͈��k�E�W�J������SSH1�Ƃ͕ʂɍs���̂ŁA���L�t���O�͗��Ƃ��Ă����B(2005.7.9 yutaka)
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
			static const SSHPacketHandler handlers[] = { handle_enable_compression, handle_disable_compression };

			unsigned char *outmsg = begin_send_packet(pvar, SSH_CMSG_REQUEST_COMPRESSION, 4);

			set_uint32(outmsg, pvar->session_settings.CompressionLevel);
			finish_send_packet(pvar);

			enque_handlers(pvar, 2, msgs, handlers);
		}

		pvar->ssh_state.compression_level = pvar->session_settings.CompressionLevel;

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
	static const SSHPacketHandler handlers[] = { handle_auth_success, handle_auth_failure };

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
		unsigned char *outmsg = begin_send_packet(pvar, SSH_CMSG_AUTH_RSA_RESPONSE, 16);

		// rhosts�F��(SSH1)���ł���悤�� SSH_AUTH_RHOSTS_RSA �������ɒǉ�����B
		if (pvar->auth_state.cur_cred.method == SSH_AUTH_RSA ||
			pvar->auth_state.cur_cred.method == SSH_AUTH_RHOSTS_RSA
			) {
			if (CRYPT_generate_RSA_challenge_response
				(pvar, pvar->ssh_state.payload + 2, challenge_bytes, outmsg)) {

				// �Z�b�V�����������Ƀp�X���[�h���g���񂵂����̂ŁA�����ł̃��\�[�X����͂�߂�B
				// socket close���ɂ����̊֐��͌Ă΂�Ă���̂ŁA���Ԃ���Ȃ��B(2005.4.8 yutaka)
				//AUTH_destroy_cur_cred(pvar);

				finish_send_packet(pvar);

				enque_simple_auth_handlers(pvar);
			} else {
				UTIL_get_lang_msg("MSG_SSH_DECRYPT_RSA_ERROR", pvar,
								  "An error occurred while decrypting the RSA challenge.\n"
								  "Perhaps the key file is corrupted.");
				notify_fatal_error(pvar, pvar->UIMsg, TRUE);
			}
		}
		else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
			int server_key_bits;
			int host_key_bits;
			int server_key_bytes;
			int host_key_bytes;
			int session_buf_len;
			char *session_buf;
			unsigned char session_id[16];

			unsigned char *hash;
			int pubkeylen, hashlen;
			BIGNUM *server_n, *host_n;

			RSA_get0_key(pvar->crypt_state.server_key.RSA_key, &server_n, NULL, NULL);
			RSA_get0_key(pvar->crypt_state.host_key.RSA_key, &host_n, NULL, NULL);
			server_key_bits = BN_num_bits(server_n);
			host_key_bits = BN_num_bits(host_n);
			server_key_bytes = (server_key_bits + 7) / 8;
			host_key_bytes = (host_key_bits + 7) / 8;
			session_buf_len = server_key_bytes + host_key_bytes + 8;
			session_buf = (char *) malloc(session_buf_len);

			/* Pageant �Ƀn�b�V�����v�Z���Ă��炤 */
			// ���J���̒���
			pubkeylen = putty_get_ssh1_keylen(pvar->pageant_curkey, pvar->pageant_keylistlen);
			// �Z�b�V����ID���쐬
			BN_bn2bin(host_n, session_buf);
			BN_bn2bin(server_n, session_buf + host_key_bytes);
			memcpy(session_buf + server_key_bytes + host_key_bytes, pvar->crypt_state.server_cookie, 8);
			MD5(session_buf, session_buf_len, session_id);
			// �n�b�V�����󂯎��
			hash = putty_hash_ssh1_challenge(pvar->pageant_curkey,
			                                 pubkeylen,
			                                 pvar->ssh_state.payload,
			                                 challenge_bytes + 2,
			                                 session_id,
			                                 &hashlen);

			// �n�b�V���𑗐M
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
	BIGNUM *e, *n;

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

		// SSH2�̏ꍇ�͈ȉ��̏������X�L�b�v
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

				// �Z�b�V�����������Ƀp�X���[�h���g���񂵂����̂ŁA�����ł̃��\�[�X����͂�߂�B
				// socket close���ɂ����̊֐��͌Ă΂�Ă���̂ŁA���Ԃ���Ȃ��B(2005.4.8 yutaka)
				//AUTH_destroy_cur_cred(pvar);

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
				int len;
				unsigned char *outmsg;

				RSA_get0_key(cred->key_pair->rsa, &n, NULL, NULL);
				len = BN_num_bytes(n);
				outmsg = begin_send_packet(pvar, SSH_CMSG_AUTH_RSA, 2 + len);

				logputs(LOG_LEVEL_VERBOSE, "Trying RSA authentication...");

				set_ushort16_MSBfirst(outmsg, len * 8);
				BN_bn2bin(n, outmsg + 2);
				/* don't destroy the current credentials yet */
				enque_handlers(pvar, 2, RSA_msgs, RSA_handlers);
				break;
			}
		case SSH_AUTH_RHOSTS_RSA:{
				int mod_len;
				int name_len;
				int exp_len;
				int index;
				unsigned char *outmsg;

				RSA_get0_key(cred->key_pair->rsa, &n, &e, NULL);
				mod_len = BN_num_bytes(n);
				name_len = strlen(cred->rhosts_client_user);
				exp_len = BN_num_bytes(e);
				outmsg = begin_send_packet(pvar, SSH_CMSG_AUTH_RHOSTS_RSA,
					                       12 + mod_len + name_len + exp_len);

				logputs(LOG_LEVEL_VERBOSE, "Trying RHOSTS+RSA authentication...");

				set_uint32(outmsg, name_len);
				memcpy(outmsg + 4, cred->rhosts_client_user, name_len);
				index = 4 + name_len;

				set_uint32(outmsg + index, 8 * mod_len);
				set_ushort16_MSBfirst(outmsg + index + 4, 8 * exp_len);
				BN_bn2bin(e, outmsg + index + 6);
				index += 6 + exp_len;

				set_ushort16_MSBfirst(outmsg + index, 8 * mod_len);
				BN_bn2bin(n, outmsg + index + 2);
				/* don't destroy the current credentials yet */
				enque_handlers(pvar, 2, RSA_msgs, RSA_handlers);
				break;
			}
		case SSH_AUTH_PAGEANT:{
				unsigned char *outmsg;
				unsigned char *pubkey;
				int len, bn_bytes;

				if (pvar->pageant_keycurrent != 0) {
					// ���O�̌����X�L�b�v
					pvar->pageant_curkey += 4;
					len = get_ushort16_MSBfirst(pvar->pageant_curkey);
					bn_bytes = (len + 7) / 8;
					pvar->pageant_curkey += 2 + bn_bytes;
					len = get_ushort16_MSBfirst(pvar->pageant_curkey);
					bn_bytes = (len + 7) / 8;
					pvar->pageant_curkey += 2 + bn_bytes;
					// ���O�̌��̃R�����g���X�L�b�v
					len = get_uint32_MSBfirst(pvar->pageant_curkey);
					pvar->pageant_curkey += 4 + len;
					// ���̌��̈ʒu�֗���
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
			notify_fatal_error(pvar, pvar->UIMsg, TRUE);
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
	pvar->shell_id = SSH_CHANNEL_INVALID;
	pvar->session_nego_status = 0;
	pvar->settings.ssh_protocol_version = 2;  // SSH2(default)
	pvar->kex_status = 0;
	pvar->ssh2_autologin = 0;  // autologin disabled(default)
	pvar->ask4passwd = 0; // disabled(default) (2006.9.18 maya)
	pvar->userauth_retry_count = 0;
	pvar->decomp_buffer = NULL;
	pvar->authbanner_buffer = NULL;
	pvar->ssh2_authlist = NULL; // (2007.4.27 yutaka)
	pvar->tryed_ssh2_authlist = FALSE;
	pvar->agentfwd_enable = FALSE;
	pvar->use_subsystem = FALSE;
	pvar->nosession = FALSE;
	pvar->server_sig_algs = NULL;
	pvar->server_strict_kex = FALSE;

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

		// SSH2 server��disconnect��`����
		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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

void ssh2_finish_encryption_setup(PTInstVar pvar)
{
	notify_established_secure_connection(pvar);
	pvar->ssh_state.status_flags &= ~STATUS_DONT_SEND_USER_NAME;
}

void SSH_notify_host_OK(PTInstVar pvar)
{
	if ((pvar->ssh_state.status_flags & STATUS_HOST_OK) == 0) {
		pvar->ssh_state.status_flags |= STATUS_HOST_OK;

		if (SSHv1(pvar)) {
			send_session_key(pvar);
			// ���[�U�F�؂��s���Ă悢�^�C�~���O�ɂȂ��Ă���A�F�؃_�C�A���O���o��������B
			// STATUS_HOST_OK�������ASTATUS_DONT_SEND_USER_NAME�������Ă��Ȃ��ƁA
			// �F�؃_�C�A���O�͎����g���Ȃ��̂ŁA���̃^�C�~���O�Ŗ��Ȃ��B
			AUTH_advance_to_next_cred(pvar);
		}
	}

	if (SSHv2(pvar)) {
		// SSH2_MSG_NEWKEYS �������M�Ȃ�Α���
		if ((pvar->kex_status & KEX_FLAG_NEWKEYS_SENT) == 0) {
			ssh2_send_newkeys(pvar);
		}
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
			logprintf(LOG_LEVEL_VERBOSE, "%s: sending SSH_CMSG_WINDOW_SIZE. "
			          "cols: %d, rows: %d, x: %d, y: %d", __FUNCTION__, cols, rows, x, y);
		}

	} else if (SSHv2(pvar)) {
		// �^�[�~�i���T�C�Y�ύX�ʒm�̒ǉ� (2005.1.4 yutaka)
		// SSH2���ǂ����̃`�F�b�N���s���B(2005.1.5 yutaka)
		buffer_t *msg;
		char *req_type = "window-change";
		unsigned char *outmsg;
		int len;
		Channel_t *c;

		c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: shell channel not found.", __FUNCTION__);
			return;
		}
		if (c->remote_id == SSH_CHANNEL_INVALID) {
			// ���̏󋵂͔F�؊�������ɃE�B���h�E�T�C�Y��ύX����Ɣ�������B
			// �܂��V�F���̃`���l���ɑ΂��� SSH_MSG_OPEN_CONFIRMATION ���󂯂Ă��Ȃ��̂ŁA
			// ���葤�̃`���l���ԍ�������Ȃ��̂� window-change ���b�Z�[�W�͑���Ȃ��B
			logprintf(LOG_LEVEL_WARNING, "%s: remote shell channel number is unknown.", __FUNCTION__);
			return;
		}

		msg = buffer_init();
		if (msg == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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

		logprintf(LOG_LEVEL_VERBOSE, "%s: sending SSH2_MSG_CHANNEL_REQUEST. "
		          "local: %d, remote: %d, request-type: %s, cols: %d, rows: %d, x: %d, y: %d", __FUNCTION__,
		          c->self_id, c->remote_id, req_type, cols, rows, x, y);

	} else {
		// SSH�łȂ��ꍇ�͉������Ȃ��B
	}
}

// �u���[�N�M���𑗂� -- RFC 4335
// OpenSSH ��"~B"�ɑ�������B
// (2010.9.27 yutaka)
int SSH_notify_break_signal(PTInstVar pvar)
{
	int ret = 0;

	if (SSHv2(pvar)) { // SSH2 �̂ݑΉ�
		buffer_t *msg;
		char *req_type = "break";
		unsigned char *outmsg;
		int len;
		Channel_t *c;

		c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: shell channel not found.", __FUNCTION__);
			goto error;
		}
		if (c->remote_id == SSH_CHANNEL_INVALID) {
			// �F�ؒ���� send break ���s���Ɣ�������
			// �܂��V�F���̃`���l���ɑ΂��� SSH_MSG_OPEN_CONFIRMATION ���󂯂Ă��Ȃ��̂ŁA
			// ���葤�̃`���l���ԍ�������Ȃ��̂� break ���b�Z�[�W�͑���Ȃ��B
			logprintf(LOG_LEVEL_WARNING, "%s: remote shell channel number is unknown.", __FUNCTION__);
			goto error;
		}

		msg = buffer_init();
		if (msg == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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

		logprintf(LOG_LEVEL_VERBOSE, "%s: sending SSH2_MSG_CHANNEL_REQUEST. "
		          "local: %d, remote: %d, request-type: %s, break-length: %d", __FUNCTION__,
		          c->self_id, c->remote_id, req_type, 1000);

		ret = 1;
	}

error:
	return (ret);
}

unsigned int SSH_get_min_packet_size(PTInstVar pvar)
{
	if (SSHv1(pvar)) {
		return 12;
	} else {
		return max(16, CRYPT_get_decryption_block_size(pvar));
	}
}

/* data is guaranteed to be at least SSH_get_min_packet_size bytes long
   at least 5 bytes must be decrypted */
void SSH_predecrypt_packet(PTInstVar pvar, char *data)
{
	if (SSHv2(pvar)) {
		CRYPT_decrypt(pvar, data, get_predecryption_amount(pvar));
	}
}

unsigned int SSH_get_clear_MAC_size(PTInstVar pvar)
{
	if (SSHv1(pvar)) {
		return 0;
	} else {
		return CRYPT_get_receiver_MAC_size(pvar);
	}
}

unsigned int SSH_get_authdata_size(PTInstVar pvar, int direction)
{
	if (SSHv1(pvar)) {
		return 0;
	}
	else {
		struct Mac *mac = &pvar->ssh2_keys[direction].mac;
		struct Enc *enc = &pvar->ssh2_keys[direction].enc;

		if (enc && enc->auth_len > 0) {
			// AEAD
			return enc->auth_len;
		}
		else if (mac && mac->enabled) {
			return mac->mac_len;
		}
		else {
			return 0;
		}
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
	// RAW�p�P�b�g�_���v��ǉ� (2008.8.15 yutaka)
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
					notify_fatal_error(pvar, pvar->UIMsg, TRUE);
					return;
				}

				pvar->ssh_state.compress_stream.next_in =
					(unsigned char *) buf;
				pvar->ssh_state.compress_stream.avail_in = len;

				if (deflate(&pvar->ssh_state.compress_stream, Z_SYNC_FLUSH) != Z_OK) {
					UTIL_get_lang_msg("MSG_SSH_COMP_ERROR", pvar,
					                  "Error compressing packet data");
					notify_fatal_error(pvar, pvar->UIMsg, TRUE);
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
		if (c == NULL || c->remote_id == SSH_CHANNEL_INVALID) {
			logprintf(LOG_LEVEL_ERROR, "%s: shell channel not found.", __FUNCTION__);
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
			notify_fatal_error(pvar, pvar->UIMsg, TRUE);
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
			UTIL_get_lang_msgU8("DLG_ABOUT_COMP_INFO", pvar,
								"level %d; ratio %.1f (%ld:%ld)");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
			            pvar->ssh_state.compression_level,
			            ((double) total_in) / total_out, total_in,
			            total_out);
		} else {
			UTIL_get_lang_msgU8("DLG_ABOUT_COMP_INFO2", pvar, "level %d");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
			            pvar->ssh_state.compression_level);
		}
	} else {
		UTIL_get_lang_msgU8("DLG_ABOUT_COMP_NONE", pvar, "none");
		strncpy_s(buf, sizeof(buf), pvar->UIMsg, _TRUNCATE);
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
			UTIL_get_lang_msgU8("DLG_ABOUT_COMP_INFO", pvar,
								"level %d; ratio %.1f (%ld:%ld)");
			_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, pvar->UIMsg,
			            pvar->ssh_state.compression_level,
			            ((double) total_out) / total_in, total_out,
			            total_in);
		} else {
			UTIL_get_lang_msgU8("DLG_ABOUT_COMP_INFO2", pvar, "level %d");
			_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, pvar->UIMsg,
			            pvar->ssh_state.compression_level);
		}
	} else {
		UTIL_get_lang_msgU8("DLG_ABOUT_COMP_NONE", pvar, "none");
		strncpy_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
	}

	UTIL_get_lang_msgU8("DLG_ABOUT_COMP_UPDOWN", pvar,
						"Upstream %s; Downstream %s");
	_snprintf_s(dest, len, _TRUNCATE, pvar->UIMsg, buf, buf2);
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
	UTIL_get_lang_msgU8("DLG_ABOUT_MAC_INFO", pvar,
						"%s to server, %s from server");
	_snprintf_s(dest, len, _TRUNCATE, pvar->UIMsg,
	            get_ssh2_mac_name(pvar->macs[MODE_OUT]),
	            get_ssh2_mac_name(pvar->macs[MODE_IN]));
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
	pvar->use_subsystem = FALSE;
	pvar->nosession = FALSE;

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

	// SSH2�̃f�[�^��������� (2004.12.27 yutaka)
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
		pvar->kex_status = 0;

		if (pvar->session_id != NULL) {
			free(pvar->session_id);
			pvar->session_id = NULL;
		}
		pvar->session_id_len = 0;

		pvar->userauth_success = 0;
		pvar->shell_id = SSH_CHANNEL_INVALID;
		pvar->session_nego_status = 0;

		pvar->ssh_heartbeat_tick = 0;

		if (pvar->decomp_buffer != NULL) {
			buffer_free(pvar->decomp_buffer);
			pvar->decomp_buffer = NULL;
		}

		if (pvar->authbanner_buffer != NULL) {
			buffer_free(pvar->authbanner_buffer);
			pvar->authbanner_buffer = NULL;
		}

		if (pvar->ssh2_authlist != NULL) { // (2007.4.27 yutaka)
			free(pvar->ssh2_authlist);
			pvar->ssh2_authlist = NULL;
		}

		pvar->tryed_ssh2_authlist = FALSE;

		free(pvar->server_sig_algs);
		pvar->server_sig_algs = NULL;

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
}

void SSH2_send_channel_data(PTInstVar pvar, Channel_t *c, unsigned char *buf, unsigned int buflen, int retry)
{
	buffer_t *msg;
	unsigned char *outmsg;
	unsigned int len;

	// SSH2���������̏ꍇ�́A�p�P�b�g�𑗂�Ȃ��̂ł�������ۑ����Ă���
	if (pvar->kex_status & KEX_FLAG_REKEYING) {
		ssh2_channel_add_bufchain(pvar, c, buf, buflen);
		return;
	}

	if (c == NULL)
		return;

	// ���g���C�ł͂Ȃ��A�ʏ�̃p�P�b�g���M�̍ہA�ȑO����Ȃ������f�[�^��
	// �����N�h���X�g�Ɏc���Ă���悤�ł���΁A���X�g�̖����Ɍq���B
	// ����ɂ��p�P�b�g����ꂽ�悤�Ɍ����錻�ۂ����P�����B
	// (2012.10.14 yutaka)
	if (retry == 0 && c->bufchain) {
		ssh2_channel_add_bufchain(pvar, c, buf, buflen);
		return;
	}

	if ((unsigned int)buflen > c->remote_window) {
		unsigned int offset = 0;
		// ����Ȃ��f�[�^�͂�������ۑ����Ă���
		ssh2_channel_add_bufchain(pvar, c, buf + offset, buflen - offset);
		buflen = offset;
		return;
	}
	if (buflen > 0) {
		msg = buffer_init();
		if (msg == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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

		logprintf(LOG_LEVEL_SSHDUMP, "%s: sending SSH2_MSG_CHANNEL_DATA. "
				  "local:%d remote:%d len:%d", __FUNCTION__, c->self_id, c->remote_id, buflen);

		// remote window size�̒���
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
				notify_fatal_error(pvar, pvar->UIMsg, TRUE);
				return;
			}

			pvar->ssh_state.compress_stream.next_in =
				(unsigned char *) buf;
			pvar->ssh_state.compress_stream.avail_in = len;

			if (deflate(&pvar->ssh_state.compress_stream, Z_SYNC_FLUSH) !=
				Z_OK) {
				UTIL_get_lang_msg("MSG_SSH_COMP_ERROR", pvar,
				                  "Error compressing packet data");
				notify_fatal_error(pvar, pvar->UIMsg, TRUE);
				return;
			}
		} else {
			memcpy(outmsg + 8, buf, len);
		}

		finish_send_packet_special(pvar, 1);

	} else {
		// �|�[�g�t�H���[�f�B���O�ɂ����ăN���C�A���g����̑��M�v�����ASSH�ʐM�ɏ悹�ăT�[�o�܂ő���͂���B
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
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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

		// port-forwarding(remote to local)�̃��[�J���ڑ��ւ̐������T�[�o�֕Ԃ��B(2005.7.2 yutaka)
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
		// SSH2: ���ɂȂ��B

	}
}

void SSH2_channel_input_eof(PTInstVar pvar, Channel_t *c)
{
	buffer_t *msg;
	unsigned char *outmsg;
	int len;

	if (c == NULL)
		return;

	// SSH2���������̏ꍇ�A�p�P�b�g���̂Ă�B(2005.6.21 yutaka)
	if (pvar->kex_status & KEX_FLAG_REKEYING) {
		// TODO: ���z�Ƃ��Ă̓p�P�b�g�j���ł͂Ȃ��A�p�P�b�g�ǂݎ��x���ɂ������Ƃ��낾���A
		// �����������Ƃɂ���B
		logprintf(LOG_LEVEL_INFO, "%s: now rekeying. data is not sent.", __FUNCTION__);

		c = NULL;

		return;
	}

	msg = buffer_init();
	if (msg == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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
		// SSH2: �`���l���N���[�Y���T�[�o�֒ʒm
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

		logprintf(LOG_LEVEL_VERBOSE, "%s: Forwarding request (SSH1 RtoL): "
				  "remote_port=%d, to_host=%s, to_port=%d", __FUNCTION__,
			from_server_port, to_local_host, to_local_port);

		set_uint32(outmsg, from_server_port);
		set_uint32(outmsg + 4, host_len);
		memcpy(outmsg + 8, to_local_host, host_len);
		set_uint32(outmsg + 8 + host_len, to_local_port);
		finish_send_packet(pvar);

		enque_forwarding_request_handlers(pvar);

		logprintf(LOG_LEVEL_VERBOSE, "%s: sending SSH_CMSG_PORT_FORWARD_REQUEST."
				  "remote_port=%d, to_host=%s, to_port=%d", __FUNCTION__,
			from_server_port, to_local_host, to_local_port);

	} else {
		// SSH2 port-forwading remote to local (2005.6.21 yutaka)
		buffer_t *msg;
		char *req;
		unsigned char *outmsg;
		int len;

		logprintf(LOG_LEVEL_VERBOSE, "%s: Forwarding request (SSH2 RtoL): "
				  "bind_addr=%s, remote_port=%d, to_host=%s, to_port=%d", __FUNCTION__,
			bind_address, from_server_port, to_local_host, to_local_port);

		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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

		logprintf(LOG_LEVEL_VERBOSE, "%s: sending SSH2_MSG_GLOBAL_REQUEST. "
				  "request=%s, want_reply=%d, bind_address=%s, remote_port=%d", __FUNCTION__,
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
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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
			            outmsg_len - ((UINT_PTR)auth_data_ptr - (UINT_PTR)outmsg) - i * 2,
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
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
			return;
		}

		c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: shell channel not found.", __FUNCTION__);
			return;
		}
		if (c->remote_id == SSH_CHANNEL_INVALID) {
			logprintf(LOG_LEVEL_ERROR, "%s: remote shell channel number is unknown.", __FUNCTION__);
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

		logprintf(LOG_LEVEL_VERBOSE, "%s: sending SSH2_MSG_CHANNEL_REQUEST. "
		          "local: %d, remote: %d, request-type: %s, proto: %s, cookie: %s, screen: %d", __FUNCTION__,
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

			// SSH2���������̏ꍇ�A�p�P�b�g���̂Ă�B(2005.6.21 yutaka)
			if (pvar->kex_status & KEX_FLAG_REKEYING) {
				// TODO: ���z�Ƃ��Ă̓p�P�b�g�j���ł͂Ȃ��A�p�P�b�g�ǂݎ��x���ɂ������Ƃ��낾���A
				// �����������Ƃɂ���B
				logprintf(LOG_LEVEL_INFO, "%s: now rekeying. channel open request is not sent.", __FUNCTION__);

				c = NULL;

				return;
			}

			// changed window size from 128KB to 32KB. (2006.3.6 yutaka)
			// changed window size from 32KB to 128KB. (2007.10.29 maya)
			c = ssh2_channel_new(CHAN_TCP_WINDOW_DEFAULT, CHAN_TCP_PACKET_DEFAULT, TYPE_PORTFWD, local_channel_num);
			if (c == NULL) {
				// �]���`���l�����ɂ���\�P�b�g�̉���R����C�� (2007.7.26 maya)
				FWD_free_channel(pvar, local_channel_num);
				UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
				                  "Could not open new channel. TTSSH is already opening too many channels.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
				return;
			}

			msg = buffer_init();
			if (msg == NULL) {
				// TODO: error check
				logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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

	if (SSHv1(pvar)) { // SSH1�̂�
		finish_send_packet(pvar);
		enque_handlers(pvar, 2, msgs, handlers);
	}

}

/**
 *	fopen utf-8 wrapper
 */
static FILE *fopenU8(const char *filenameU8, const char *mode)
{
	wchar_t *filenameW = ToWcharU8(filenameU8);
	wchar_t *modeW = ToWcharU8(mode);
	FILE *fp;
	_wfopen_s(&fp, filenameW, modeW);
	free(modeW);
	free(filenameW);
	return fp;
}

/**
 * stat UTF-8 wrapper
 */
static int statU8(const char *filenameU8, struct __stat64 *st)
{
	wchar_t *filenameW = ToWcharU8(filenameU8);
	int r = _wstat64(filenameW, st);
	free(filenameW);
	return r;
}

/**
 *	ExtractFileName() UTF-8��
 */
static void ExtractFileNameU8(const char *PathName, char *FileName, size_t destlen)
{
	const char *sep = strrchr(PathName, '/');
	if (sep == NULL) {
		sep = strrchr(PathName, '\\');
	}
	if (sep != NULL) {
		strncpy_s(FileName, destlen, sep + 1, _TRUNCATE);
	}
	else {
		strncpy_s(FileName, destlen, PathName, _TRUNCATE);
	}
}

static int accessU8(const char *pathU8, int mode)
{
	wchar_t *pathW = ToWcharU8(pathU8);
	int r = _waccess(pathW, mode);
	free(pathW);
	return r;
}

/**
 *	SCP support
 *
 *	@param sendfile		�t�@�C����,UTF-8
 *	@param dstfile		�t�@�C����,UTF-8
 *						TOREMOTE �̂Ƃ��A
 *							NULL �̂Ƃ��A�z�[���t�H���_
 *							���΃p�X�A�z�[���t�H���_����̑���?
 *							��΃p�X
 *						TOLOCAL �̂Ƃ�
 *							NULL �̂Ƃ��A�_�E�����[�h�t�H���_
 *							���΃p�X�A�J�����g�t�H���_����̑���?
 *							��΃p�X
 *	@param direction	TOREMOTE	copy local to remote
 *						FROMREMOTE	copy remote to local
 *
 */
int SSH_scp_transaction(PTInstVar pvar, const char *sendfile, const char *dstfile, enum scp_dir direction)
{
	Channel_t *c = NULL;
	FILE *fp = NULL;

	// �\�P�b�g���N���[�Y����Ă���ꍇ�͉������Ȃ��B
	if (pvar->socket == INVALID_SOCKET)
		goto error;

	if (SSHv1(pvar))      // SSH1�T�|�[�g��TBD
		goto error;

	// �`���l���ݒ�
	c = ssh2_channel_new(CHAN_SES_WINDOW_DEFAULT, CHAN_SES_PACKET_DEFAULT, TYPE_SCP, -1);
	if (c == NULL) {
		UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
		                  "Could not open new channel. TTSSH is already opening too many channels.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		goto error;
	}
	if (c->type == TYPE_SCP) {
		c->scp.pvar = pvar;
		pvar->recv.suspended = FALSE;
		pvar->recv.data_finished = FALSE;
	}

	if (direction == TOREMOTE) {  // copy local to remote
		struct __stat64 st;
		fp = fopenU8(sendfile, "rb");
		if (fp == NULL) {
			static const TTMessageBoxInfoW info = {
				"TTSSH",
				"MSG_SSH_SCP_FILEOPEN_ERROR_TITLE", L"TTSSH: file open error",
				"MSG_SSH_SCP_FILEOPEN_READ_ERROR", L"Can't open file for reading: %s %s",
				MB_OK | MB_ICONERROR
			};
			DWORD error = GetLastError();
			wchar_t *err_str;
			hFormatMessageW(error, &err_str);
			wchar_t *fname = ToWcharU8(sendfile);
			TTMessageBoxW(pvar->cv->HWin, &info, pvar->ts->UILanguageFileW, err_str, fname);
			free(fname);
			free(err_str);
			goto error;
		}

		strncpy_s(c->scp.localfilefull, sizeof(c->scp.localfilefull), sendfile, _TRUNCATE);  // full path
		ExtractFileNameU8(sendfile, c->scp.localfile, sizeof(c->scp.localfile));   // file name only
		if (dstfile == NULL || dstfile[0] == '\0') { // remote file path
			strncpy_s(c->scp.remotefile, sizeof(c->scp.remotefile), ".", _TRUNCATE);  // full path
		} else {
			strncpy_s(c->scp.remotefile, sizeof(c->scp.remotefile), dstfile, _TRUNCATE);  // full path
		}
		c->scp.localfp = fp;     // file pointer

		if (statU8(c->scp.localfilefull, &st) == 0) {
			c->scp.filestat = st;
		} else {
			goto error;
		}
	} else { // copy remote to local
		strncpy_s(c->scp.remotefile, sizeof(c->scp.remotefile), sendfile, _TRUNCATE);

		if (dstfile == NULL || dstfile[0] == '\0') { // local file path is empty.
			char *fn;
			wchar_t *FileDirExpanded;
			char *FileDirExpandedU8;

			fn = strrchr(sendfile, '/');
			if (fn && fn[1] == '\0')
				goto error;

			FileDirExpanded = GetFileDir(pvar->ts);
			FileDirExpandedU8 = ToU8W(FileDirExpanded);
			_snprintf_s(c->scp.localfilefull, sizeof(c->scp.localfilefull), _TRUNCATE, "%s\\%s", FileDirExpandedU8, fn ? fn : sendfile);
			free(FileDirExpanded);
			free(FileDirExpandedU8);
			ExtractFileName(c->scp.localfilefull, c->scp.localfile, sizeof(c->scp.localfile));   // file name only
		} else {
			_snprintf_s(c->scp.localfilefull, sizeof(c->scp.localfilefull), _TRUNCATE, "%s", dstfile);
			ExtractFileName(dstfile, c->scp.localfile, sizeof(c->scp.localfile));   // file name only
		}

		if (accessU8(c->scp.localfilefull, 0x00) == 0) {
			int dlgresult;
			if (accessU8(c->scp.localfilefull, 0x02) == -1) { // 0x02 == writable
				static const TTMessageBoxInfoW info = {
					"TTSSH",
					"MSG_SSH_SCP_FILEOPEN_ERROR_TITLE", L"TTSSH: file open error",
					"MSG_SSH_SCP_FILEOPEN_READONLY_ERROR", L"`%s' file is read only.",
					MB_OK | MB_ICONERROR
				};
				wchar_t *fname = ToWcharU8(c->scp.localfilefull);
				TTMessageBoxW(pvar->cv->HWin, &info, pvar->ts->UILanguageFileW, fname);
				free(fname);
				goto error;
			}
			static const TTMessageBoxInfoW info = {
				"TTSSH",
				"MSG_SSH_SCP_CONFIRM_TITLE", L"TTSSH: confirm",
				"MSG_SSH_SCP_FILEOPEN_OVERWRITE_CONFIRM", L"`%s' file exists.\noverwrite it?",
				MB_YESNO | MB_ICONQUESTION
			};
			wchar_t *fname = ToWcharU8(c->scp.localfilefull);
			dlgresult = TTMessageBoxW(pvar->cv->HWin, &info, pvar->ts->UILanguageFileW, fname);
			free(fname);
			if (dlgresult == IDNO) {
				goto error;
			}
		}

		fp = fopenU8(c->scp.localfilefull, "wb");
		if (fp == NULL) {
			static const TTMessageBoxInfoW info = {
				"TTSSH",
				"MSG_SSH_SCP_FILEOPEN_ERROR_TITLE", L"TTSSH: file open error",
				"MSG_SSH_SCP_FILEOPEN_WRITE_ERROR", L"Can't open file for writing: %s %s",
				MB_OK | MB_ICONERROR
			};
			DWORD error = GetLastError();
			wchar_t *err_str;
			hFormatMessageW(error, &err_str);
			wchar_t *fname = ToWcharU8(c->scp.localfilefull);
			TTMessageBoxW(pvar->cv->HWin, &info, pvar->ts->UILanguageFileW, err_str, fname);
			free(fname);
			free(err_str);
			goto error;
		}

		c->scp.localfp = fp;     // file pointer
	}

	// setup SCP data
	c->scp.dir = direction;
	c->scp.state = SCP_INIT;

	{
		buffer_t *msg;
		char *s;
		unsigned char *outmsg;
		int len;

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
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);
	}

	g_scp_sending = TRUE;

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

int SSH_scp_sending_status(void)
{
	return g_scp_sending;
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

	// �\�P�b�g���N���[�Y����Ă���ꍇ�͉������Ȃ��B
	if (pvar->socket == INVALID_SOCKET)
		goto error;

	if (SSHv1(pvar))      // SSH1�T�|�[�g��TBD
		goto error;

	// �`���l���ݒ�
	c = ssh2_channel_new(CHAN_SES_WINDOW_DEFAULT, CHAN_SES_PACKET_DEFAULT, TYPE_SFTP, -1);
	if (c == NULL) {
		UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
		                  "Could not open new channel. TTSSH is already opening too many channels.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
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
	memcpy(outmsg, buffer_ptr(msg), len);
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


/*
 * �������Ő����������̒u����B���ۂ̒ʐM�Ɏg����̂�pvar->ssh2_keys[]�ł���A�����ɒu���������ł͎g���Ȃ��B
 * �L���ɂ���^�C�~���O�ŁApvar->ssh2_keys �ɃR�s�[����B
 */
static SSHKeys current_keys[MODE_MAX];


#define write_buffer_file(buf,len) do_write_buffer_file(buf,len,__FILE__,__LINE__)


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

// �N���C�A���g����T�[�o�ւ̃L�[�����J�n�v��
void SSH2_send_kexinit(PTInstVar pvar)
{
	char cookie[SSH2_COOKIE_LENGTH];
	buffer_t *msg;
	unsigned char *outmsg;
	int len, i;

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
		return;
	}
	if (pvar->my_kex != NULL)
		buffer_free(pvar->my_kex);
	pvar->my_kex = msg;

	// ���b�Z�[�W�^�C�v
	//SSH2_packet_start(msg, SSH2_MSG_KEXINIT);

	// cookie�̃Z�b�g
	CRYPT_set_random_data(pvar, cookie, sizeof(cookie));
	CRYPT_set_client_cookie(pvar, cookie);
	buffer_append(msg, cookie, sizeof(cookie));

	// �N���C�A���g�̃L�[���
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

	// my_kex�Ɏ���Ă������߁A�t���[���Ă͂����Ȃ��B
	//buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEXINIT was sent at SSH2_send_kexinit().");
}


void normalize_generic_order(char *buf, char default_strings[], int default_strings_len)
{
	char listed[max(KEX_DH_MAX,max(SSH_CIPHER_MAX,max(KEY_ALGO_MAX,max(HMAC_MAX,COMP_MAX)))) + 1];
	char allowed[max(KEX_DH_MAX,max(SSH_CIPHER_MAX,max(KEY_ALGO_MAX,max(HMAC_MAX,COMP_MAX)))) + 1];
	int i, j, k=-1;

	memset(listed, 0, sizeof(listed));
	memset(allowed, 0, sizeof(allowed));

	// ������Ă��镶���̃��X�g�����B
	for (i = 0; i < default_strings_len ; i++) {
		allowed[default_strings[i]] = 1;
	}

	// �w�肳�ꂽ������𑖍����A������Ă��Ȃ������A�d�����镶���͍폜����B
	//
	// ex. (i=5 �̕������폜����)
	// i=012345
	//   >:=9<87;A@?B3026(\0)
	//         i+1
	//         <------------>
	//       ��
	//   >:=9<7;A@?B3026(\0)
	//
	for (i = 0; buf[i] != 0; i++) {
		int num = buf[i] - '0';

		if (num < 0 || num > default_strings_len
			|| !allowed[num]
			|| listed[num]) {
			memmove(buf + i, buf + i + 1, strlen(buf + i + 1) + 1);
			i--;
		} else {
			listed[num] = 1;
		}

		// disabled line������΁A�ʒu���o���Ă����B
		if (num == 0) {
			k = i;
		}
	}

	// �w�肳��Ă��Ȃ�����������΁Adisabled line�̒��O�ɑ}������B
	//
	// ex. (Z��}������)
	//                k
	//   >:=9<87;A@?B3026(\0)
	//                 k+1
	//                 <---->
	//       ��       k
	//   >:=9<87;A@?B30026(\0)
	//       ��        k
	//   >:=9<87;A@?B3Z026(\0)
	//
	for (j = 0; j < default_strings_len && default_strings[j] != 0; j++) {
		int num = default_strings[j];

		if (!listed[num] && k >= 0) {
			int copylen = strlen(buf + k + 1) + 1;

			memmove(buf + k + 1, buf + k, copylen);
			buf[k + 1 + copylen] = '\0';   // �I�[��Y�ꂸ�ɕt����B
			buf[k] = num + '0';
			k++;
			i++;
		}
	}
	if (k < 0) {
		j = 0;
	}
	else {
		j++;
	}

	// disabled line�����݂��Ȃ��ꍇ�́A���̂܂ܖ����ɒǉ�����B
	for (; j < default_strings_len ; j++) {
		int num = default_strings[j];

		if (!listed[num]) {
			buf[i] = num + '0';
			listed[num] = 1;
			i++;
		}
	}

	buf[i] = 0;
}

void choose_SSH2_proposal(char *server_proposal,
                          char *my_proposal,
                          char *dest,
                          int dest_len)
{
	char tmp_cli[1024], *ptr_cli, *ctc_cli;
	char tmp_svr[1024], *ptr_svr, *ctc_svr;

	strncpy_s(tmp_cli, sizeof(tmp_cli), my_proposal, _TRUNCATE);
	ptr_cli = strtok_s(tmp_cli, ",", &ctc_cli);
	while (ptr_cli != NULL) {
		// server_proposal�ɂ̓T�[�o��proposal���J���}������Ŋi�[����Ă���
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

// �Í��A���S���Y���̃L�[�T�C�Y�A�u���b�N�T�C�Y�AMAC�T�C�Y�̂����ő�l(we_need)�����肷��B
static void choose_SSH2_key_maxlength(PTInstVar pvar)
{
	int mode, val;
	unsigned int need = 0;
	const EVP_MD *md;
	const struct ssh2cipher *cipher;
	const struct SSH2Mac *mac;

	for (mode = 0; mode < MODE_MAX; mode++) {
		cipher = pvar->ciphers[mode];
		mac = pvar->macs[mode];

		// current_keys[]�ɐݒ肵�Ă����āA���Ƃ� pvar->ssh2_keys[] �փR�s�[����B
		md = get_ssh2_mac_EVP_MD(mac);
		current_keys[mode].mac.md = md;
		current_keys[mode].mac.key_len = current_keys[mode].mac.mac_len = EVP_MD_size(md);
		val = get_ssh2_mac_truncatebits(mac);
		if (val != 0) {
			current_keys[mode].mac.mac_len = val / 8;
		}
		current_keys[mode].mac.etm = get_ssh2_mac_etm(mac);

		// �L�[�T�C�Y�ƃu���b�N�T�C�Y�������Őݒ肵�Ă��� (2004.11.7 yutaka)
		current_keys[mode].enc.key_len = get_cipher_key_len(cipher);
		current_keys[mode].enc.block_size = get_cipher_block_size(cipher);
		current_keys[mode].enc.iv_len = get_cipher_iv_len(cipher);
		current_keys[mode].enc.auth_len = get_cipher_auth_len(cipher);

		current_keys[mode].mac.enabled = 0;
		current_keys[mode].comp.enabled = 0; // (2005.7.9 yutaka)

		// �����_�ł�MAC��disable
		pvar->ssh2_keys[mode].mac.enabled = 0;
		pvar->ssh2_keys[mode].comp.enabled = 0; // (2005.7.9 yutaka)

		need = max(need, current_keys[mode].enc.key_len);
		need = max(need, current_keys[mode].enc.block_size);
		need = max(need, current_keys[mode].enc.iv_len);
		need = max(need, current_keys[mode].mac.key_len);
	}
	pvar->we_need = need;
}


/*
 * �L�[�����J�n�O�̃`�F�b�N (SSH2_MSG_KEXINIT)
 * �����Y�֐��̓f�[�^�ʐM���ɂ��Ă΂�Ă���\������
 *
 * SSH2_MSG_KEXINIT:
 *   byte         SSH_MSG_KEXINIT
 *   byte[16]     cookie (random bytes)
 *   name-list    kex_algorithms
 *   name-list    server_host_key_algorithms
 *   name-list    encryption_algorithms (c2s)
 *   name-list    encryption_algorithms (s2c)
 *   name-list    mac_algorithms (c2s)
 *   name-list    mac_algorithms (s2c)
 *   name-list    compression_algorithms (c2s)
 *   name-list    compression_algorithms (s2c)
 *   name-list    languages (c2s)
 *   name-list    languages (s2c)
 *   boolean      first_kex_packet_follows
 *   uint32       0 (reserved for future extension)
 */
static BOOL handle_SSH2_kexinit(PTInstVar pvar)
{
	char buf[1024];
	char *data;
	int len, size;
	char *msg = NULL;
	char tmp[1024+512];

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEXINIT was received.");

	// ���łɃL�[�������I����Ă���ɂ��ւ�炸�A�T�[�o���� SSH2_MSG_KEXINIT ��
	// �����Ă���ꍇ�́A�L�[�č쐬���s���B(2004.10.24 yutaka)
	if (pvar->kex_status == KEX_FLAG_KEXDONE) {
		pvar->kex_status = KEX_FLAG_REKEYING;

		// �L�[�č쐬���� myproposal ���� ",ext-info-c,kex-strict-c-v00@openssh.com" ���폜����
		// �X�V����̂� KEX �݂̂ł悢
		SSH2_update_kex_myproposal(pvar);

		// �T�[�o��SSH2_MSG_KEXINIT �𑗂�
		SSH2_send_kexinit(pvar);
	}

	data = remained_payload(pvar);
	len = remained_payloadlen(pvar);

	// KEX �̍Ō�� exchange-hash (session-id) ���v�Z����̂Ɏg���̂ŕۑ����Ă���
	if (pvar->peer_kex != NULL) {
		// already allocated
		buffer_clear(pvar->peer_kex);
	}
	else {
		pvar->peer_kex = buffer_init();
		if (pvar->peer_kex == NULL) {
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
						"%s: Out of memory", __FUNCTION__);
			msg = tmp;
			goto error;
		}
	}
	buffer_append(pvar->peer_kex, data, len);

	push_memdump("KEXINIT", "exchange algorithm list: receiving", data, len);

	// cookie
	if (! get_bytearray_from_payload(pvar, buf, SSH2_COOKIE_LENGTH)) {
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
					"%s: truncated packet (cookie)", __FUNCTION__);
		msg = tmp;
		goto error;
	}
	CRYPT_set_server_cookie(pvar, buf);

	// �e�v�f(������,�Í�����)�Ŏg�p����A���S���Y���̌���B
	// �T�[�o����̓J���}��؂�ł̃��X�g�������ė���B
	// �N���C�A���g�ƃT�[�o�������T�|�[�g���Ă��镨�̂����A
	// �N���C�A���g���ōł��O�Ɏw�肵�������g����B

	// �������A���S���Y��
	switch (get_namelist_from_payload(pvar, buf, sizeof(buf), &size)) {
	case GetPayloadError:
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
					"%s: truncated packet (kex algorithms)", __FUNCTION__);
		msg = tmp;
		goto error;
	case GetPayloadTruncate:
		logprintf(LOG_LEVEL_WARNING, "%s: server proposed kex algorithms is too long.", __FUNCTION__);
		break;
	}

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: KEX algorithm: %s", buf);

	pvar->kex_type = choose_SSH2_kex_algorithm(buf, myproposal[PROPOSAL_KEX_ALGS]);
	if (pvar->kex_type == KEX_DH_UNKNOWN) { // not match
		strncpy_s(tmp, sizeof(tmp), "unknown KEX algorithm: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}

	if (pvar->kex_status == 0) {
		// �T�[�o�[����Strict KEX�ɑΉ����Ă��邩�̊m�F
		choose_SSH2_proposal(buf, "kex-strict-s-v00@openssh.com", tmp, sizeof(tmp));
		if (tmp[0] != '\0') {
			pvar->server_strict_kex = TRUE;
			logprintf(LOG_LEVEL_INFO, "Server supports strict kex. Strict kex will be enabled.");
		}
	}

	// �z�X�g���A���S���Y��
	switch (get_namelist_from_payload(pvar, buf, sizeof(buf), &size)) {
	case GetPayloadError:
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
					"%s: truncated packet (hostkey algorithms)", __FUNCTION__);
		msg = tmp;
		goto error;
	case GetPayloadTruncate:
		logprintf(LOG_LEVEL_WARNING, "%s: server proposed hostkey algorithms is too long.", __FUNCTION__);
		break;
	}

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: server host key algorithm: %s", buf);

	pvar->hostkey_type = choose_SSH2_host_key_algorithm(buf, myproposal[PROPOSAL_SERVER_HOST_KEY_ALGS]);
	if (pvar->hostkey_type == KEY_ALGO_UNSPEC) {
		strncpy_s(tmp, sizeof(tmp), "unknown host KEY algorithm: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}

	// �Í��A���S���Y��(�N���C�A���g -> �T�[�o)
	switch (get_namelist_from_payload(pvar, buf, sizeof(buf), &size)) {
	case GetPayloadError:
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
					"%s: truncated packet (encryption algorithms client to server)", __FUNCTION__);
		msg = tmp;
		goto error;
	case GetPayloadTruncate:
		logprintf(LOG_LEVEL_WARNING, "%s: server proposed encryption algorithms (client to server) is too long.", __FUNCTION__);
		break;
	}

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: encryption algorithm client to server: %s", buf);

	pvar->ciphers[MODE_OUT] = choose_SSH2_cipher_algorithm(buf, myproposal[PROPOSAL_ENC_ALGS_CTOS]);
	if (pvar->ciphers[MODE_OUT] == NULL) {
		strncpy_s(tmp, sizeof(tmp), "unknown Encrypt algorithm(client to server): ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}

	// �Í��A���S���Y��(�T�[�o -> �N���C�A���g)
	switch (get_namelist_from_payload(pvar, buf, sizeof(buf), &size)) {
	case GetPayloadError:
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
					"%s: truncated packet (encryption algorithms server to client)", __FUNCTION__);
		msg = tmp;
		goto error;
	case GetPayloadTruncate:
		logprintf(LOG_LEVEL_WARNING, "%s: server proposed encryption algorithms (server to client) is too long.", __FUNCTION__);
		break;
	}

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: encryption algorithm server to client: %s", buf);

	pvar->ciphers[MODE_IN] = choose_SSH2_cipher_algorithm(buf, myproposal[PROPOSAL_ENC_ALGS_STOC]);
	if (pvar->ciphers[MODE_IN] == NULL) {
		strncpy_s(tmp, sizeof(tmp), "unknown Encrypt algorithm(server to client): ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}

	// MAC�A���S���Y��(�N���C�A���g -> �T�[�o)
	switch (get_namelist_from_payload(pvar, buf, sizeof(buf), &size)) {
	case GetPayloadError:
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
					"%s: truncated packet (MAC algorithms client to server)", __FUNCTION__);
		msg = tmp;
		goto error;
	case GetPayloadTruncate:
		logprintf(LOG_LEVEL_WARNING, "%s: server proposed MAC algorithms (client to server) is too long.", __FUNCTION__);
		break;
	}

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: MAC algorithm client to server: %s", buf);

	if (get_cipher_auth_len(pvar->ciphers[MODE_OUT]) > 0) {
		logputs(LOG_LEVEL_VERBOSE, "AEAD cipher is selected, ignoring MAC algorithms. (client to server)");
		pvar->macs[MODE_OUT] = get_ssh2_mac(HMAC_IMPLICIT);
	}
	else {
		pvar->macs[MODE_OUT] = choose_SSH2_mac_algorithm(buf, myproposal[PROPOSAL_MAC_ALGS_CTOS]);
		if (pvar->macs[MODE_OUT] == NULL) { // not match
			strncpy_s(tmp, sizeof(tmp), "unknown MAC algorithm: ", _TRUNCATE);
			strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
			msg = tmp;
			goto error;
		}
	}

	// MAC�A���S���Y��(�T�[�o -> �N���C�A���g)
	switch (get_namelist_from_payload(pvar, buf, sizeof(buf), &size)) {
	case GetPayloadError:
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
					"%s: truncated packet (MAC algorithms server to client)", __FUNCTION__);
		msg = tmp;
		goto error;
	case GetPayloadTruncate:
		logprintf(LOG_LEVEL_WARNING, "%s: server proposed MAC algorithms (server to client) is too long.", __FUNCTION__);
		break;
	}

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: MAC algorithm server to client: %s", buf);

	if (get_cipher_auth_len(pvar->ciphers[MODE_IN]) > 0) {
		logputs(LOG_LEVEL_VERBOSE, "AEAD cipher is selected, ignoring MAC algorithms. (server to client)");
		pvar->macs[MODE_IN] = get_ssh2_mac(HMAC_IMPLICIT);
	}
	else {
		pvar->macs[MODE_IN] = choose_SSH2_mac_algorithm(buf, myproposal[PROPOSAL_MAC_ALGS_STOC]);
		if (pvar->macs[MODE_IN] == NULL) { // not match
			strncpy_s(tmp, sizeof(tmp), "unknown MAC algorithm: ", _TRUNCATE);
			strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
			msg = tmp;
			goto error;
		}
	}

	// ���k�A���S���Y��(�N���C�A���g -> �T�[�o)
	switch (get_namelist_from_payload(pvar, buf, sizeof(buf), &size)) {
	case GetPayloadError:
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
					"%s: truncated packet (compression algorithms client to server)", __FUNCTION__);
		msg = tmp;
		goto error;
	case GetPayloadTruncate:
		logprintf(LOG_LEVEL_WARNING, "%s: server proposed compression algorithms (client to server) is too long.", __FUNCTION__);
		break;
	}

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: compression algorithm client to server: %s", buf);

	pvar->ctos_compression = choose_SSH2_compression_algorithm(buf, myproposal[PROPOSAL_COMP_ALGS_CTOS]);
	if (pvar->ctos_compression == COMP_UNKNOWN) { // not match
		strncpy_s(tmp, sizeof(tmp), "unknown Packet Compression algorithm: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}

	// ���k�A���S���Y��(�T�[�o -> �N���C�A���g)
	switch (get_namelist_from_payload(pvar, buf, sizeof(buf), &size)) {
	case GetPayloadError:
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
					"%s: truncated packet (compression algorithms server to client)", __FUNCTION__);
		msg = tmp;
		goto error;
	case GetPayloadTruncate:
		logprintf(LOG_LEVEL_WARNING, "%s: server proposed compression algorithms (server to client) is too long.", __FUNCTION__);
		break;
	}

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: compression algorithm server to client: %s", buf);

	pvar->stoc_compression = choose_SSH2_compression_algorithm(buf, myproposal[PROPOSAL_COMP_ALGS_STOC]);
	if (pvar->stoc_compression == COMP_UNKNOWN) { // not match
		strncpy_s(tmp, sizeof(tmp), "unknown Packet Compression algorithm: ", _TRUNCATE);
		strncat_s(tmp, sizeof(tmp), buf, _TRUNCATE);
		msg = tmp;
		goto error;
	}

	// ����(�N���C�A���g -> �T�[�o)
	// ����ł͖��g�p�B���O�ɋL�^���邾���B
	switch (get_namelist_from_payload(pvar, buf, sizeof(buf), &size)) {
	case GetPayloadError:
		// ����� name-list �����Ȃ��Ƃ������� KEXINIT �p�P�b�g�̃t�H�[�}�b�g���̂��z��O�ł���
		// �ُ�ȏ�Ԃł��邪�A�ʐM�ɕK�v�ȃA���S���Y���͂��łɃl�S�ς݂ŒʐM���͍̂s����B
		// ���܂ł͂��̕����̃`�F�b�N���s���Ă��Ȃ������̂ŁA�x�����L�^����݂̂ŏ����𑱍s����B
		logprintf(LOG_LEVEL_WARNING, "%s: truncated packet (language client to server)", __FUNCTION__);
		goto skip;
	case GetPayloadTruncate:
		logprintf(LOG_LEVEL_WARNING, "%s: server proposed language (client to server) is too long.", __FUNCTION__);
		break;
	}

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: language client to server: %s", buf);

	// ����(�T�[�o -> �N���C�A���g)
	// ����ł͖��g�p�B���O�ɋL�^���邾���B
	switch (get_namelist_from_payload(pvar, buf, sizeof(buf), &size)) {
	case GetPayloadError:
		// ����(�N���C�A���g -> �T�[�o) �Ɠ��l�ɁA��肪�����Ă��x���݂̂Ƃ���B
		logprintf(LOG_LEVEL_WARNING, "%s: truncated packet (language server to client)", __FUNCTION__);
		goto error;
	case GetPayloadTruncate:
		logprintf(LOG_LEVEL_WARNING, "%s: server proposed language (server to client) is too long.", __FUNCTION__);
		break;
	}

	logprintf(LOG_LEVEL_VERBOSE, "server proposal: language server to client: %s", buf);

	// first_kex_packet_follows:
	// KEXINIT �p�P�b�g�̌�ɁA�A���S���Y���̃l�S���ʂ𐄑����Č������p�P�b�g�𑗂��Ă��邩�B
	// SSH_MSG_KEXINIT �̌�̌������̓N���C�A���g�����瑗��̂ŃT�[�o���� 1 �ɂ��鎖�͂Ȃ��͂��B
	if (!get_boolean_from_payload(pvar, buf)) {
		// ����(�N���C�A���g -> �T�[�o) �Ɠ��l�ɁA��肪�����Ă��x���݂̂Ƃ���B
		logprintf(LOG_LEVEL_WARNING, "%s: truncated packet (first_kex_packet_follows)", __FUNCTION__);
		goto skip;
	}
	if (buf[0] != 0) {
		// �O�q�̂悤�ɃT�[�o���� 0 �ȊO�ɂ��鎖�͂Ȃ��͂��Ȃ̂ŁA�x�����L�^����B
		logprintf(LOG_LEVEL_WARNING, "%s: first_kex_packet_follows is not 0. (%d)", __FUNCTION__, buf[0]);
	}

	// reserved: ����͏�� 0 �ƂȂ�B
	if (!get_uint32_from_payload(pvar, &size)) {
		// ����(�N���C�A���g -> �T�[�o) �Ɠ��l�ɁA��肪�����Ă��x���݂̂Ƃ���B
		logprintf(LOG_LEVEL_WARNING, "%s: truncated packet (reserved)", __FUNCTION__ );
		goto skip;
	}
	if (size != 0) {
		logprintf(LOG_LEVEL_INFO, "%s: reserved data is not 0. (%d)", __FUNCTION__, size);
	}

skip:
	// ���肵�����������O�ɏo��
	logprintf(LOG_LEVEL_VERBOSE, "KEX algorithm: %s",
		get_kex_algorithm_name(pvar->kex_type));

	logprintf(LOG_LEVEL_VERBOSE, "server host key algorithm: %s",
		get_ssh2_hostkey_algorithm_name(pvar->hostkey_type));

	logprintf(LOG_LEVEL_VERBOSE, "encryption algorithm client to server: %s",
		get_cipher_string(pvar->ciphers[MODE_OUT]));

	logprintf(LOG_LEVEL_VERBOSE, "encryption algorithm server to client: %s",
		get_cipher_string(pvar->ciphers[MODE_IN]));

	logprintf(LOG_LEVEL_VERBOSE, "MAC algorithm client to server: %s",
		get_ssh2_mac_name(pvar->macs[MODE_OUT]));

	logprintf(LOG_LEVEL_VERBOSE, "MAC algorithm server to client: %s",
		get_ssh2_mac_name(pvar->macs[MODE_IN]));

	logprintf(LOG_LEVEL_VERBOSE, "compression algorithm client to server: %s",
		get_ssh2_comp_name(pvar->ctos_compression));

	logprintf(LOG_LEVEL_VERBOSE, "compression algorithm server to client: %s",
		get_ssh2_comp_name(pvar->stoc_compression));

	// we_need�̌��� (2004.11.6 yutaka)
	// �L�[�č쐬�̏ꍇ�̓X�L�b�v����B
	if ((pvar->kex_status & KEX_FLAG_REKEYING) == 0) {
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
// �Œ� DH Groups (RFC 4253, draft-baushke-ssh-dh-group-sha2-04)
//
static void SSH2_dh_kex_init(PTInstVar pvar)
{
	DH *dh = NULL;
	buffer_t *msg = NULL;
	unsigned char *outmsg;
	int len;
	BIGNUM *pub_key, *priv_key;

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

	// �閧�ɂ��ׂ�����(X)�𐶐�
	dh_gen_key(pvar, dh, pvar->we_need);
	DH_get0_key(dh, NULL, &priv_key);
	log_kex_key(pvar, priv_key);

	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
		return;
	}

	DH_get0_key(dh, &pub_key, NULL);
	buffer_put_bignum2(msg, pub_key);

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_KEXDH_INIT, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);

	if (pvar->kexdh != NULL) {
		DH_free(pvar->kexdh);
	}
	pvar->kexdh = dh;

	SSH2_dispatch_init(pvar, 2);
	SSH2_dispatch_add_message(SSH2_MSG_KEXDH_REPLY);

	buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEXDH_INIT was sent at SSH2_dh_kex_init().");

	return;

error:;
	DH_free(dh);
	buffer_free(msg);

	notify_fatal_error(pvar, "error occurred @ SSH2_dh_kex_init()", TRUE);
}



/*
 * DH-GEX (RFC 4419)
 *
 * SSH_MSG_KEY_DH_GEX_REQUEST:
 *   byte    SSH_MSG_KEY_DH_GEX_REQUEST (34)
 *   uint32  min, minimal size in bits of an acceptable group
 *   uint32  n, preferred size in bits of the group the server will send
 *   uint32  max, maximal size in bits of an acceptable group
 */

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

	// �T�[�o�ɗv������ group size �� min, n(preferred), max �����肷��B
	if (pvar->settings.GexMinimalGroupSize == 0) {
		// 0 (���ݒ�) ���������͍ŐV�̐����l���g��
		min = GEX_GRP_DEFAULT_MIN;
	}
	else if (pvar->settings.GexMinimalGroupSize < GEX_GRP_LIMIT_MIN) {
		min = GEX_GRP_LIMIT_MIN;
		logprintf(LOG_LEVEL_NOTICE,
			"%s: small GexMinimalGroupSize is too small (%d), use minimum limit (%d)", __FUNCTION__,
			pvar->settings.GexMinimalGroupSize, GEX_GRP_LIMIT_MIN);
	}
	else if (pvar->settings.GexMinimalGroupSize > GEX_GRP_LIMIT_MAX) {
		min = GEX_GRP_LIMIT_MAX;
		logprintf(LOG_LEVEL_NOTICE,
			"%s: small GexMinimalGroupSize is too large (%d), use maximum limit (%d)", __FUNCTION__,
			pvar->settings.GexMinimalGroupSize, GEX_GRP_LIMIT_MAX);
	}
	else {
		min = pvar->settings.GexMinimalGroupSize;
	}

	// max �͏�ɏ�������ς�
	max = GEX_GRP_LIMIT_MAX;

	// preferred �͎g�p����e�Í��v�f�̌���/�u���b�N���̂����A�ő�̂��̂��g��
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

	// �T�[�o��group size�𑗂��āAp �� g ������Ă��炤�B
	buffer_put_int(msg, min);
	buffer_put_int(msg, bits);
	buffer_put_int(msg, max);
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_KEX_DH_GEX_REQUEST, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);

	// ���ƂŃn�b�V���v�Z�Ɏg���̂Ŏ���Ă����B
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

	SSH2_dispatch_init(pvar, 2);
	SSH2_dispatch_add_message(SSH2_MSG_KEX_DH_GEX_GROUP);

	buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_DH_GEX_REQUEST was sent at SSH2_dh_gex_kex_init().");

	return;

error:;
	buffer_free(msg);

	notify_fatal_error(pvar, "error occurred @ SSH2_dh_gex_kex_init()", TRUE);
}

/*
 * SSH2_MSG_KEX_DH_GEX_GROUP:
 *   byte    SSH_MSG_KEX_DH_GEX_GROUP
 *   mpint   p, safe prime
 *   mpint   g, generator for subgroup in GF(p)
 */
static BOOL handle_SSH2_dh_gex_group(PTInstVar pvar)
{
	int len, grp_bits;
	BIGNUM *p = NULL, *g = NULL;
	DH *dh = NULL;
	buffer_t *msg = NULL;
	unsigned char *outmsg;
	char tmpbuf[256];
	BIGNUM *pub_key, *priv_key;

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_DH_GEX_GROUP was received.");

	p = BN_new();
	g = BN_new();
	if (p == NULL || g == NULL)
		goto error;

	if (!get_mpint_from_payload(pvar, p) || !get_mpint_from_payload(pvar, g)) {
		_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE,
					"%s:truncated packet (mpint)", __FUNCTION__);
		notify_fatal_error(pvar, tmpbuf, FALSE);
		return FALSE;
	}

	grp_bits = BN_num_bits(p);
	logprintf(LOG_LEVEL_VERBOSE, "DH-GEX: Request: %d / %d / %d, Received: %d",
	            pvar->kexgex_min, pvar->kexgex_bits, pvar->kexgex_max, BN_num_bits(p));

	//
	// (1) < GEX_GRP_LIMIT_MIN <= (2) < kexgex_min <= (3) < kexgex_bits <= (4) <= kexgex_max < (5) <= GEX_GRP_LIMIT_MAX < (6)
	//
	if (grp_bits < GEX_GRP_LIMIT_MIN || grp_bits > GEX_GRP_LIMIT_MAX) {
	// (1), (6) �v���g�R���ŔF�߂��Ă���͈�(1024 <= grp_bits <= 8192)�̊O�B�����ؒf�B
		UTIL_get_lang_msg("MSG_SSH_GEX_SIZE_OUTOFRANGE", pvar,
		                  "Received group size is out of range: %d");
		_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, pvar->UIMsg, grp_bits);
		notify_fatal_error(pvar, tmpbuf, FALSE);
		goto error;
	}
	else if (grp_bits < pvar->kexgex_min) {
	// (2) �v���g�R���ŔF�߂��Ă���͈͓������A������̐ݒ肵���ŏ��l��菬�����B�m�F�_�C�A���O���o���B
		logprintf(LOG_LEVEL_WARNING,
		    "DH-GEX: grp_bits(%d) < kexgex_min(%d)", grp_bits, pvar->kexgex_min);
		UTIL_get_lang_msg("MSG_SSH_GEX_SIZE_SMALLER", pvar,
		                  "Received group size is smaller than the requested minimal size.\nrequested: %d, received: %d\nAre you sure that you want to accecpt received group?");
		_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE,
			pvar->UIMsg, pvar->kexgex_min, grp_bits);
	}
	else if (grp_bits < pvar->kexgex_bits) {
	// (3) �v���̍ŏ��l�͖��������A�v���l���͏������B�m�F�_�C�A���O�͏o���Ȃ��B
		logprintf(LOG_LEVEL_NOTICE,
			"DH-GEX: grp_bits(%d) < kexgex_bits(%d)", grp_bits, pvar->kexgex_bits);
		tmpbuf[0] = 0; // no message
	}
	else if (grp_bits <= pvar->kexgex_max) {
	// (4) �v���l�ȏ�A���v���̍ő�l�ȉ��B���Ȃ��B
		tmpbuf[0] = 0; // no message
	}
	else {
	// (5) ������̐ݒ肵���ő�l���傫���B�m�F�_�C�A���O���o���B
	//     ����������ł� kexgex_max == GEX_GRP_LIMIT_MAX(8192) �ł���ׂ��̏󋵂ɂȂ鎖�͖����B
		logprintf(LOG_LEVEL_WARNING,
			"DH-GEX: grp_bits(%d) > kexgex_max(%d)", grp_bits, pvar->kexgex_max);
		UTIL_get_lang_msg("MSG_SSH_GEX_SIZE_LARGER", pvar,
		                  "Received group size is larger than the requested maximal size.\nrequested: %d, received: %d\nAre you sure that you want to accecpt received group?");
		_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE,
			pvar->UIMsg, pvar->kexgex_max, grp_bits);
	}

	if (tmpbuf[0] != 0) {
		UTIL_get_lang_msg("MSG_SSH_GEX_SIZE_TITLE", pvar,
		                  "TTSSH: Confirm GEX group size");
		if (MessageBox(NULL, tmpbuf, pvar->UIMsg, MB_YESNO | MB_ICONERROR) == IDNO) {
			UTIL_get_lang_msg("MSG_SSH_GEX_SIZE_CANCEL", pvar,
			                  "New connection is cancelled.");
			notify_fatal_error(pvar, pvar->UIMsg, FALSE);
			goto error;
		}
	}

	dh = DH_new();
	if (dh == NULL)
		goto error;
	DH_set0_pqg(dh, p, NULL, g);

	// �閧�ɂ��ׂ�����(X)�𐶐�
	dh_gen_key(pvar, dh, pvar->we_need);
	DH_get0_key(dh, NULL, &priv_key);
	log_kex_key(pvar, priv_key);

	// ���J�����T�[�o�֑��M
	msg = buffer_init();
	if (msg == NULL) {
		goto error;
	}
	DH_get0_key(dh, &pub_key, NULL);
	buffer_put_bignum2(msg, pub_key);
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_KEX_DH_GEX_INIT, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_DH_GEX_INIT was sent at handle_SSH2_dh_gex_group().");

	// �����ō쐬����DH���́A���ƂŃn�b�V���v�Z�Ɏg�����ߎ���Ă����B(2004.10.31 yutaka)
	if (pvar->kexdh != NULL) {
		DH_free(pvar->kexdh);
	}
	pvar->kexdh = dh;

	{
		BIGNUM *p, *q, *pub_key;

		DH_get0_pqg(dh, &p, &q, NULL);
		DH_get0_key(dh, &pub_key, NULL);

		push_bignum_memdump("DH_GEX_GROUP", "p", p);
		push_bignum_memdump("DH_GEX_GROUP", "g", g);
		push_bignum_memdump("DH_GEX_GROUP", "pub_key", pub_key);
	}

	SSH2_dispatch_init(pvar, 2);
	SSH2_dispatch_add_message(SSH2_MSG_KEX_DH_GEX_REPLY);

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
	int len, ret;
	char buf[128];

	client_key = EC_KEY_new();
	if (client_key == NULL) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s: EC_KEY_new was failed", __FUNCTION__);
		goto error;
	}
	client_key = EC_KEY_new_by_curve_name(kextype_to_cipher_nid(pvar->kex_type));
	if (client_key == NULL) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s: EC_KEY_new_by_curve_name was failed", __FUNCTION__);
		goto error;
	}
	ret = EC_KEY_generate_key(client_key);
	if (ret != 1) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s: EC_KEY_generate_key was failed(ret %d)", __FUNCTION__, ret);
		goto error;
	}
	group = EC_KEY_get0_group(client_key);
	log_kex_key(pvar, EC_KEY_get0_private_key(client_key));

	msg = buffer_init();
	if (msg == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s: buffer_init was failed", __FUNCTION__);
		goto error;
	}

	buffer_put_ecpoint(msg, group, EC_KEY_get0_public_key(client_key));

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_KEX_ECDH_INIT, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);

	// �����ō쐬�������́A���ƂŃn�b�V���v�Z�Ɏg�����ߎ���Ă����B
	if (pvar->ecdh_client_key) {
		EC_KEY_free(pvar->ecdh_client_key);
	}
	pvar->ecdh_client_key = client_key;

	SSH2_dispatch_init(pvar, 2);
	SSH2_dispatch_add_message(SSH2_MSG_KEX_ECDH_REPLY);

	buffer_free(msg);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_ECDH_INIT was sent at SSH2_ecdh_kex_init().");

	return;

error:;
	EC_KEY_free(client_key);
	buffer_free(msg);

	notify_fatal_error(pvar, buf, TRUE);
}


static void ssh2_set_newkeys(PTInstVar pvar, int mode)
{
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

	pvar->ssh2_keys[mode] = current_keys[mode];
}

static BOOL ssh2_kex_finish(PTInstVar pvar, char *hash, int hashlen, BIGNUM *share_key, Key *hostkey, char *signature, int siglen)
{
	int ret;
	char emsg[1024];  // error message

	//debug_print(30, hash, hashlen);
	//debug_print(31, pvar->client_version_string, strlen(pvar->client_version_string));
	//debug_print(32, pvar->server_version_string, strlen(pvar->server_version_string));
	//debug_print(33, buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex));
	//debug_print(34, buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex));
	//debug_print(35, server_host_key_blob, bloblen);

	// session id�̕ۑ��i����ڑ����̂݁j
	if (pvar->session_id == NULL) {
		pvar->session_id_len = hashlen;
		pvar->session_id = malloc(pvar->session_id_len);
		if (pvar->session_id != NULL) {
			memcpy(pvar->session_id, hash, pvar->session_id_len);
		} else {
			// TODO:
		}
	}

	if ((ret = key_verify(hostkey, signature, siglen, hash, hashlen, pvar->hostkey_type)) != 1) {
		if (ret == -3 && hostkey->type == KEY_RSA) {
			if (!pvar->settings.EnableRsaShortKeyServer) {
				BIGNUM *n;
				RSA_get0_key(hostkey->rsa, &n, NULL, NULL);
				_snprintf_s(emsg, sizeof(emsg), _TRUNCATE,
					"%s: key verify error. remote rsa key length is too short (%d-bit)", __FUNCTION__,
					BN_num_bits(n));
			}
			else {
				goto cont;
			}
		}
		else {
			char *buf = NULL;
			buf = ToCharW(SENDTOME);
			_snprintf_s(emsg, sizeof(emsg), _TRUNCATE, "%s: key verify error (%d)\r\n%s", __FUNCTION__, ret, buf);
			free(buf);
		}

		save_memdump(LOGDUMP);
		notify_fatal_error(pvar, emsg, TRUE);
		return FALSE;
	}

cont:
	kex_derive_keys(pvar, current_keys, pvar->we_need, hash, share_key, pvar->session_id, pvar->session_id_len);

	prep_compression(pvar);

	SSH2_dispatch_init(pvar, 3);
	SSH2_dispatch_add_message(SSH2_MSG_NEWKEYS);

	return TRUE;
}

static void ssh2_send_newkeys(PTInstVar pvar)
{
	// send SSH2_MSG_NEWKEYS
	begin_send_packet(pvar, SSH2_MSG_NEWKEYS, 0);
	finish_send_packet(pvar);

	logprintf(LOG_LEVEL_VERBOSE, "%s: SSH2_MSG_NEWKEYS was sent.", __FUNCTION__);

	// SSH2_MSG_NEWKEYS �𑗂�I������ȍ~�̃p�P�b�g�͈Í��������K�v���L��ׁA
	// ���̎��_�ő��M�����̈Í������J�n����B
	ssh2_set_newkeys(pvar, MODE_OUT);
	if (!CRYPT_start_encryption(pvar, 1, 0)) {
		// TODO: error
	}


	// ���l�ɁAMAC�ƃp�P�b�g���k�����̎��_�ŗL���ɂ���B
	pvar->ssh2_keys[MODE_OUT].mac.enabled = 1;
	pvar->ssh2_keys[MODE_OUT].comp.enabled = 1;
	enable_send_compression(pvar);

	pvar->kex_status |= KEX_FLAG_NEWKEYS_SENT;

	if (pvar->server_strict_kex) {
		logprintf(LOG_LEVEL_INFO, "%s: Strict kex is enabled, resetting sender sequence number %d", __FUNCTION__, pvar->ssh_state.sender_sequence_number);
		pvar->ssh_state.sender_sequence_number = 0;
	}

	// SSH2_MSG_NEWKEYS �����Ɏ󂯎���Ă�����KEX�͊����B���̏����Ɉڂ�B
	if (pvar->kex_status & KEX_FLAG_NEWKEYS_RECEIVED) {
		if ((pvar->kex_status & KEX_FLAG_REKEYING)) {
			do_SSH2_dispatch_setup_for_transfer(pvar);
		}
		else {
			// ����� SSH2_MSG_NEWKEYS �̑���M���������A�ȍ~�̒ʐM�͈Í������ꂽ��ԂɂȂ�
			ssh2_finish_encryption_setup(pvar);

			// ����̌�������̓��[�U�F�؂��J�n����
			ssh2_prep_userauth(pvar);
		}
		pvar->kex_status = KEX_FLAG_KEXDONE;
	}

	return;
}

/*
 * Diffie-Hellman Key Exchange Reply (SSH2_MSG_KEXDH_REPLY:31)
 *
 * return TRUE: ����
 *        FALSE: ���s
 */
static BOOL handle_SSH2_dh_kex_reply(PTInstVar pvar)
{
	char *data;
	int len;
	char *server_host_key_blob;
	int bloblen, siglen;
	BIGNUM *server_public = NULL;
	char *signature;
	int dh_len, share_len;
	char *dh_buf = NULL;
	BIGNUM *share_key = NULL;
	char *hash;
	char *emsg = NULL, emsg_tmp[1024];  // error message
	int hashlen;
	Key *hostkey = NULL;  // hostkey
	BOOL result = FALSE;
	BIGNUM *pub_key;
	int ret;

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEXDH_REPLY was received.");

	memset(&hostkey, 0, sizeof(hostkey));

	// ���b�Z�[�W�^�C�v�̌�ɑ����y�C���[�h�̐擪
	data = pvar->ssh_state.payload;
	// �y�C���[�h�̒���; ���b�Z�[�W�^�C�v���� 1 �o�C�g�����炷
	len = pvar->ssh_state.payloadlen - 1;

	// for debug
	push_memdump("KEXDH_REPLY", "key exchange: receiving", data, len);

	bloblen = get_uint32_MSBfirst(data);
	data += 4;
	server_host_key_blob = data; // for hash

	push_memdump("KEXDH_REPLY", "server_host_key_blob", server_host_key_blob, bloblen);

	hostkey = key_from_blob(data, bloblen);
	if (hostkey == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: key_from_blob error", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	data += bloblen;

	// known_hosts�Ή� (2006.3.20 yutaka)
	if (hostkey->type != get_ssh2_hostkey_type_from_algorithm(pvar->hostkey_type)) {  // �z�X�g�L�[�̎�ʔ�r
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
		            "%s: type mismatch for decoded server_host_key_blob (kex:%s(%s) blob:%s)",
		            /*__FUNCTION__*/"handle_SSH2_dh_kex_reply",
		            get_ssh2_hostkey_type_name_from_algorithm(pvar->hostkey_type),
		            get_ssh2_hostkey_algorithm_name(pvar->hostkey_type),
		            get_ssh2_hostkey_type_name(hostkey->type));
		emsg = emsg_tmp;
		goto error;
	}

	server_public = BN_new();
	if (server_public == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: Out of memory (1)", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}

	buffer_get_bignum2(&data, server_public);

	siglen = get_uint32_MSBfirst(data);
	data += 4;
	signature = data;
	data += siglen;

	push_memdump("KEXDH_REPLY", "signature", signature, siglen);

	// check public key
	if (!dh_pub_is_valid(pvar->kexdh, server_public)) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: invalid server public key", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	// ���ʌ��̐���
	dh_len = DH_size(pvar->kexdh);
	dh_buf = malloc(dh_len);
	if (dh_buf == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: Out of memory (2)", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	share_len = DH_compute_key(dh_buf, server_public, pvar->kexdh);
	share_key = BN_new();
	if (share_key == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: Out of memory (3)", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	// 'share_key'���T�[�o�ƃN���C�A���g�ŋ��L���錮�iG^A�~B mod P�j�ƂȂ�B
	BN_bin2bn(dh_buf, share_len, share_key);
	//debug_print(40, dh_buf, share_len);

	// �n�b�V���̌v�Z
	/* calc and verify H */
	DH_get0_key(pvar->kexdh, &pub_key, NULL);
	hash = kex_dh_hash(
		get_kex_algorithm_EVP_MD(pvar->kex_type),
		pvar->client_version_string,
		pvar->server_version_string,
		buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex),
		buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex),
		server_host_key_blob, bloblen,
		pub_key,
		server_public,
		share_key,
		&hashlen);

	{
		push_memdump("KEXDH_REPLY kex_dh_kex_hash", "my_kex", buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex));
		push_memdump("KEXDH_REPLY kex_dh_kex_hash", "peer_kex", buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex));

		push_bignum_memdump("KEXDH_REPLY kex_dh_kex_hash", "server_public", server_public);
		push_bignum_memdump("KEXDH_REPLY kex_dh_kex_hash", "share_key", share_key);

		push_memdump("KEXDH_REPLY kex_dh_kex_hash", "hash", hash, hashlen);
	}

	// TTSSH�o�[�W�������ɕ\������L�[�r�b�g�������߂Ă���
	DH_get0_key(pvar->kexdh, &pub_key, NULL);
	pvar->client_key_bits = BN_num_bits(pub_key);
	pvar->server_key_bits = BN_num_bits(server_public);

	result = ssh2_kex_finish(pvar, hash, hashlen, share_key, hostkey, signature, siglen);

	ret = HOSTS_check_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, hostkey);
	if (ret == TRUE) {
		// �z�X�g���̊m�F�����������̂ŁA�㑱�̏������s��
		SSH_notify_host_OK(pvar);
		// known_hosts�_�C�A���O�̌Ăяo�����̂ŁA�ȍ~�A�������Ȃ��B
	}

error:
	BN_free(server_public);
	DH_free(pvar->kexdh); pvar->kexdh = NULL;
	key_free(hostkey);
	free(dh_buf);
	BN_free(share_key);

	if (emsg)
		notify_fatal_error(pvar, emsg, TRUE);

	return result;
}

/*
 * Diffie-Hellman Group and Key Exchange Reply (SSH2_MSG_KEX_DH_GEX_REPLY:33)
 *
 * return TRUE: ����
 *        FALSE: ���s
 */
static BOOL handle_SSH2_dh_gex_reply(PTInstVar pvar)
{
	char *data;
	int len;
	char *server_host_key_blob;
	int bloblen, siglen;
	BIGNUM *server_public = NULL;
	char *signature;
	int dh_len, share_len;
	char *dh_buf = NULL;
	BIGNUM *share_key = NULL;
	char *hash;
	char *emsg = NULL, emsg_tmp[1024];  // error message
	int hashlen;
	Key *hostkey = NULL;  // hostkey
	BOOL result = FALSE;
	int ret;
	BIGNUM *p, *g;
	BIGNUM *pub_key;

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_DH_GEX_REPLY was received.");

	memset(&hostkey, 0, sizeof(hostkey));

	// ���b�Z�[�W�^�C�v�̌�ɑ����y�C���[�h�̐擪
	data = pvar->ssh_state.payload;
	// �y�C���[�h�̒���; ���b�Z�[�W�^�C�v���� 1 �o�C�g�����炷
	len = pvar->ssh_state.payloadlen - 1;

	// for debug
	push_memdump("DH_GEX_REPLY", "key exchange: receiving", data, len);

	bloblen = get_uint32_MSBfirst(data);
	data += 4;
	server_host_key_blob = data; // for hash

	push_memdump("DH_GEX_REPLY", "server_host_key_blob", server_host_key_blob, bloblen);

	hostkey = key_from_blob(data, bloblen);
	if (hostkey == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: key_from_blob error", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	data += bloblen;

	// known_hosts�Ή� (2006.3.20 yutaka)
	if (hostkey->type != get_ssh2_hostkey_type_from_algorithm(pvar->hostkey_type)) {  // �z�X�g�L�[�̎�ʔ�r
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
		            "%s: type mismatch for decoded server_host_key_blob (kex:%s(%s) blob:%s)",
		            /*__FUNCTION__*/"handle_SSH2_dh_gex_reply",
		            get_ssh2_hostkey_type_name_from_algorithm(pvar->hostkey_type),
		            get_ssh2_hostkey_algorithm_name(pvar->hostkey_type),
		            get_ssh2_hostkey_type_name(hostkey->type));
		emsg = emsg_tmp;
		goto error;
	}

	server_public = BN_new();
	if (server_public == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: Out of memory (1)", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}

	buffer_get_bignum2(&data, server_public);

	siglen = get_uint32_MSBfirst(data);
	data += 4;
	signature = data;
	data += siglen;

	push_memdump("DH_GEX_REPLY", "signature", signature, siglen);

	// check public key
	if (!dh_pub_is_valid(pvar->kexdh, server_public)) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: invalid server public key", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	// ���ʌ��̐���
	dh_len = DH_size(pvar->kexdh);
	dh_buf = malloc(dh_len);
	if (dh_buf == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: Out of memory (2)", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	share_len = DH_compute_key(dh_buf, server_public, pvar->kexdh);
	share_key = BN_new();
	if (share_key == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: Out of memory (3)", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	// 'share_key'���T�[�o�ƃN���C�A���g�ŋ��L���錮�iG^A�~B mod P�j�ƂȂ�B
	BN_bin2bn(dh_buf, share_len, share_key);
	//debug_print(40, dh_buf, share_len);

	// �n�b�V���̌v�Z
	/* calc and verify H */
	DH_get0_pqg(pvar->kexdh, &p, NULL, &g);
	DH_get0_key(pvar->kexdh, &pub_key, NULL);
	hash = kex_dh_gex_hash(
		get_kex_algorithm_EVP_MD(pvar->kex_type),
		pvar->client_version_string,
		pvar->server_version_string,
		buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex),
		buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex),
		server_host_key_blob, bloblen,
		pvar->kexgex_min,
		pvar->kexgex_bits,
		pvar->kexgex_max,
		p,
		g,
		pub_key,
		server_public,
		share_key,
		&hashlen);

	{
		push_memdump("DH_GEX_REPLY kex_dh_gex_hash", "my_kex", buffer_ptr(pvar->my_kex), buffer_len(pvar->my_kex));
		push_memdump("DH_GEX_REPLY kex_dh_gex_hash", "peer_kex", buffer_ptr(pvar->peer_kex), buffer_len(pvar->peer_kex));

		push_bignum_memdump("DH_GEX_REPLY kex_dh_gex_hash", "server_public", server_public);
		push_bignum_memdump("DH_GEX_REPLY kex_dh_gex_hash", "share_key", share_key);

		push_memdump("DH_GEX_REPLY kex_dh_gex_hash", "hash", hash, hashlen);
	}

	// TTSSH�o�[�W�������ɕ\������L�[�r�b�g�������߂Ă���
	DH_get0_key(pvar->kexdh, &pub_key, NULL);
	pvar->client_key_bits = BN_num_bits(pub_key);
	pvar->server_key_bits = BN_num_bits(server_public);

	result = ssh2_kex_finish(pvar, hash, hashlen, share_key, hostkey, signature, siglen);

	ret = HOSTS_check_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, hostkey);
	if (ret == TRUE) {
		// �z�X�g���̊m�F�����������̂ŁA�㑱�̏������s��
		SSH_notify_host_OK(pvar);
		// known_hosts�_�C�A���O�̌Ăяo�����̂ŁA�ȍ~�A�������Ȃ��B
	}

error:
	BN_free(server_public);
	DH_free(pvar->kexdh); pvar->kexdh = NULL;
	key_free(hostkey);
	free(dh_buf);
	BN_free(share_key);

	if (emsg)
		notify_fatal_error(pvar, emsg, TRUE);

	return result;
}


/*
 * Elliptic Curve Diffie-Hellman Key Exchange Reply (SSH2_MSG_KEX_ECDH_REPLY:31)
 *
 * return TRUE: ����
 *        FALSE: ���s
 */
static BOOL handle_SSH2_ecdh_kex_reply(PTInstVar pvar)
{
	char *data;
	int len;
	char *server_host_key_blob;
	int bloblen, siglen;
	EC_POINT *server_public = NULL;
	const EC_GROUP *group;
	char *signature;
	int ecdh_len;
	char *ecdh_buf = NULL;
	BIGNUM *share_key = NULL;
	char *hash;
	char *emsg = NULL, emsg_tmp[1024];  // error message
	int hashlen;
	Key *hostkey = NULL;  // hostkey
	BOOL result = FALSE;
	int ret;

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_KEX_ECDH_REPLY was received.");

	memset(&hostkey, 0, sizeof(hostkey));

	// ���b�Z�[�W�^�C�v�̌�ɑ����y�C���[�h�̐擪
	data = pvar->ssh_state.payload;
	// �y�C���[�h�̒���; ���b�Z�[�W�^�C�v���� 1 �o�C�g�����炷
	len = pvar->ssh_state.payloadlen - 1;

	// for debug
	push_memdump("KEX_ECDH_REPLY", "key exchange: receiving", data, len);

	bloblen = get_uint32_MSBfirst(data);
	data += 4;
	server_host_key_blob = data; // for hash

	push_memdump("KEX_ECDH_REPLY", "server_host_key_blob", server_host_key_blob, bloblen);

	hostkey = key_from_blob(data, bloblen);
	if (hostkey == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: key_from_blob error", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	data += bloblen;

	// known_hosts�Ή� (2006.3.20 yutaka)
	if (hostkey->type != get_ssh2_hostkey_type_from_algorithm(pvar->hostkey_type)) {  // �z�X�g�L�[�̎�ʔ�r
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
		            "%s: type mismatch for decoded server_host_key_blob (kex:%s(%s) blob:%s)",
		            /*__FUNCTION__*/"handle_SSH2_ecdh_kex_reply",
		            get_ssh2_hostkey_type_name_from_algorithm(pvar->hostkey_type),
		            get_ssh2_hostkey_algorithm_name(pvar->hostkey_type),
		            get_ssh2_hostkey_type_name(hostkey->type));
		emsg = emsg_tmp;
		goto error;
	}

	/* Q_S, server public key */
	group = EC_KEY_get0_group(pvar->ecdh_client_key);
	server_public = EC_POINT_new(group);
	if (server_public == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: Out of memory (1)", __FUNCTION__);
		emsg = emsg_tmp;
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
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: invalid server public key", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	// ���ʌ��̐���
	ecdh_len = (EC_GROUP_get_degree(group) + 7) / 8;
	ecdh_buf = malloc(ecdh_len);
	if (ecdh_buf == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: Out of memory (2)", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	if (ECDH_compute_key(ecdh_buf, ecdh_len, server_public,
	                     pvar->ecdh_client_key, NULL) != (int)ecdh_len) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: Out of memory (3)", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	share_key = BN_new();
	if (share_key == NULL) {
		_snprintf_s(emsg_tmp, sizeof(emsg_tmp), _TRUNCATE,
					"%s: Out of memory (4)", __FUNCTION__);
		emsg = emsg_tmp;
		goto error;
	}
	// 'share_key'���T�[�o�ƃN���C�A���g�ŋ��L���錮�iG^A�~B mod P�j�ƂȂ�B
	BN_bin2bn(ecdh_buf, ecdh_len, share_key);
	//debug_print(40, ecdh_buf, ecdh_len);

	// �n�b�V���̌v�Z
	/* calc and verify H */
	hash = kex_ecdh_hash(
		get_kex_algorithm_EVP_MD(pvar->kex_type),
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

	// TTSSH�o�[�W�������ɕ\������L�[�r�b�g�������߂Ă���
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

	result = ssh2_kex_finish(pvar, hash, hashlen, share_key, hostkey, signature, siglen);

	ret = HOSTS_check_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, hostkey);
	if (ret == TRUE) {
		// �z�X�g���̊m�F�����������̂ŁA�㑱�̏������s��
		SSH_notify_host_OK(pvar);
		// known_hosts�_�C�A���O�̌Ăяo�����̂ŁA�ȍ~�A�������Ȃ��B
	}

error:
	EC_POINT_clear_free(server_public);
	EC_KEY_free(pvar->ecdh_client_key); pvar->ecdh_client_key = NULL;
	key_free(hostkey);
	free(ecdh_buf);
	BN_free(share_key);

	if (emsg)
		notify_fatal_error(pvar, emsg, TRUE);

	return result;
}


// KEX�ɂ����ăT�[�o����Ԃ��Ă��� 31 �ԃ��b�Z�[�W�ɑ΂���n���h��
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
	pvar->kex_status = KEX_FLAG_KEXDONE;

	SSH2_dispatch_init(pvar, 6);
	SSH2_dispatch_add_range_message(SSH2_MSG_GLOBAL_REQUEST, SSH2_MSG_CHANNEL_FAILURE);
	SSH2_dispatch_add_message(SSH2_MSG_KEXINIT);
}


static BOOL handle_SSH2_newkeys(PTInstVar pvar)
{
	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_NEWKEYS was received(DH key generation is completed).");

	// ���O�̎�̏I�� (2005.3.7 yutaka)
	if (LogLevel(pvar, LOG_LEVEL_SSHDUMP)) {
		save_memdump(LOGDUMP);
	}
	finish_memdump();

	pvar->kex_status |= KEX_FLAG_NEWKEYS_RECEIVED;

	// SSH2_MSG_NEWKEYS ��M��͑��肩��̃p�P�b�g�͈Í�������Ă���̂ŁA
	// ��M�����̈Í���(����)��L���ɂ���B
	ssh2_set_newkeys(pvar, MODE_IN);
	if (!CRYPT_start_encryption(pvar, 0, 1)) {
		// TODO: error
	}

	// ���l�ɁAMAC����ш��k��L���ɂ���B
	pvar->ssh2_keys[MODE_IN].mac.enabled = 1;
	pvar->ssh2_keys[MODE_IN].comp.enabled = 1;
	enable_recv_compression(pvar);

	if (pvar->server_strict_kex) {
		logprintf(LOG_LEVEL_INFO, "%s: Strict kex is enabled, resetting receiver sequence number %d", __FUNCTION__, pvar->ssh_state.receiver_sequence_number);
		pvar->ssh_state.receiver_sequence_number = 0;
	}

	SSH2_dispatch_add_message(SSH2_MSG_EXT_INFO);

	// SSH2_MSG_NEWKEYS �����ɑ����Ă�����KEX�͊����B���̏����Ɉڂ�B
	if (pvar->kex_status & KEX_FLAG_NEWKEYS_SENT) {
		if (pvar->kex_status & KEX_FLAG_REKEYING) {
			int i;
			Channel_t *c;

			do_SSH2_dispatch_setup_for_transfer(pvar);

			// ���炸�o�b�t�@�ɕۑ����Ă������f�[�^�𑗂�
			for (i = 0 ; i < CHANNEL_MAX ; i++) {
				c = &channels[i];
				if (c->used) {
					ssh2_channel_retry_send_bufchain(pvar, c);
				}
			}
		}
		else {
			// ����� SSH2_MSG_NEWKEYS �̑���M���������A�ȍ~�̒ʐM�͈Í������ꂽ��ԂɂȂ�
			ssh2_finish_encryption_setup(pvar);

			// ����̌�������̓��[�U�F�؂��J�n����
			ssh2_prep_userauth(pvar);
		}
		pvar->kex_status = KEX_FLAG_KEXDONE;
	}

	return TRUE;
}

static void ssh2_prep_userauth(PTInstVar pvar)
{
	int type = (1 << SSH_AUTH_PASSWORD) | (1 << SSH_AUTH_RSA) |
	           (1 << SSH_AUTH_TIS) | (1 << SSH_AUTH_PAGEANT);

	// �F�ؕ����̐ݒ�
	AUTH_set_supported_auth_types(pvar, type);

	// �F�؃_�C�A���O��\������
	AUTH_advance_to_next_cred(pvar);
}

// ���[�U�F�؂̊J�n
BOOL do_SSH2_userauth(PTInstVar pvar)
{
	buffer_t *msg;
	char *s;
	unsigned char *outmsg;
	int len;

	// �p�X���[�h�����͂��ꂽ�� 1 �𗧂Ă� (2005.3.12 yutaka)
	pvar->keyboard_interactive_password_input = 0;

	// ���łɃ��O�C���������s���Ă���ꍇ�́ASSH2_MSG_SERVICE_REQUEST�̑��M��
	// ���Ȃ����Ƃɂ���BOpenSSH�ł͎x��Ȃ����ATru64 UNIX�ł̓T�[�o�G���[�ƂȂ��Ă��܂����߁B
	// (2005.3.10 yutaka)
	// Cisco 12.4.11T �ł����̌��ۂ���������͗l�B
	// �F�؃��\�b�h none �̎��_�� SSH2_MSG_SERVICE_REQUEST �𑗐M���Ă���B
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
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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

	SSH2_dispatch_init(pvar, 4);
	SSH2_dispatch_add_message(SSH2_MSG_SERVICE_ACCEPT);

	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_SERVICE_REQUEST was sent at do_SSH2_userauth().");

	return TRUE;
}


static BOOL handle_SSH2_service_accept(PTInstVar pvar)
{
	char *data, *svc;

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;

	if ((svc = buffer_get_string(&data, NULL)) == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_get_string returns NULL.", __FUNCTION__);
	}
	logprintf(LOG_LEVEL_VERBOSE, "SSH2_MSG_SERVICE_ACCEPT was received. service-name=%s", NonNull(svc));
	free(svc);

	SSH2_dispatch_init(pvar, 5);
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
	SSH2_dispatch_add_message(SSH2_MSG_EXT_INFO);

	return do_SSH2_authrequest(pvar);
}


/*
 * SSH_MSG_EXT_INFO:
 *     byte       SSH_MSG_EXT_INFO (value 7)
 *     uint32     nr-extensions
 *     repeat the following 2 fields "nr-extensions" times:
 *       string   extension-name
 *       string   extension-value (binary)
 */

static BOOL handle_SSH2_ext_info(PTInstVar pvar)
{
	unsigned int num_of_exts, i, len;
	unsigned char ext_name[256], ext_val[2048];
	char *new_payload_buffer = NULL;

	logputs(LOG_LEVEL_INFO, "SSH2_EXT_INFO was received.");

	if (!get_uint32_from_payload(pvar, &num_of_exts)) {
		logprintf(LOG_LEVEL_WARNING, "%s: ext info payload was corrupted", __FUNCTION__);
		return FALSE;
	}
	logprintf(LOG_LEVEL_VERBOSE, "%s: %d extensions", __FUNCTION__, num_of_exts);

	for (i=0; i<num_of_exts; i++) {
		if (!get_string_from_payload(pvar, ext_name, sizeof(ext_name), &len, TRUE)) {
			logprintf(LOG_LEVEL_WARNING, "%s: can't get extension name", __FUNCTION__);
			return FALSE;
		}
		if (strcmp(ext_name, "server-sig-algs") == 0) {
			if (!get_namelist_from_payload(pvar, ext_val, sizeof(ext_val), &len)) {
				logprintf(LOG_LEVEL_WARNING, "%s: can't get extension value", __FUNCTION__);
				return FALSE;
			}
			if (pvar->server_sig_algs) {
				logprintf(LOG_LEVEL_WARNING, "%s: update server-sig-algs, old=%s, new=%s",
				          __FUNCTION__, pvar->server_sig_algs, ext_val);
				free(pvar->server_sig_algs);
			}
			pvar->server_sig_algs = _strdup(ext_val);
			logprintf(LOG_LEVEL_VERBOSE, "%s: extension: server-sig-algs, value: %s", __FUNCTION__, ext_val);
		}
		else {
			if (!get_string_from_payload(pvar, ext_val, sizeof(ext_val), &len, TRUE)) {
				logprintf(LOG_LEVEL_WARNING, "%s: can't get extension value", __FUNCTION__);
				return FALSE;
			}
			logprintf(LOG_LEVEL_VERBOSE, "%s: extension: ext_name", __FUNCTION__, ext_name);
		}
	}

	return TRUE;
}

// ���[�U�F�؃p�P�b�g�̍\�z
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
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
		return FALSE;
	}

	// �y�C���[�h�̍\�z
	username = pvar->auth_state.user;  // ���[�U��
	buffer_put_string(msg, username, strlen(username));
	buffer_put_string(msg, connect_id, strlen(connect_id));

	if (!pvar->tryed_ssh2_authlist) { // "none"���\�b�h�̑��M
		// �F�؃��X�g���T�[�o����擾����B
		// SSH2_MSG_USERAUTH_FAILURE���Ԃ邪�A�T�[�o�ɂ̓��O�͎c��Ȃ��B
		// (2007.4.27 yutaka)
		s = "none";  // method name
		buffer_put_string(msg, s, strlen(s));

	} else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PASSWORD) { // �p�X���[�h�F��
		// password authentication method
		s = "password";
		buffer_put_string(msg, s, strlen(s));
		buffer_put_char(msg, 0); // 0

		if (pvar->ssh2_autologin == 1) { // SSH2�������O�C��
			s = pvar->ssh2_password;
		} else {
			s = pvar->auth_state.cur_cred.password;  // �p�X���[�h
		}
		buffer_put_string(msg, s, strlen(s));

	} else if (pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) { // keyboard-interactive (2005.3.12 yutaka)
		s = "keyboard-interactive";  // method name
		buffer_put_string(msg, s, strlen(s));
		s = "";  // language tag
		buffer_put_string(msg, s, strlen(s));
		s = "";  // submethods
		buffer_put_string(msg, s, strlen(s));

		SSH2_dispatch_add_message(SSH2_MSG_USERAUTH_INFO_REQUEST);

	} else if (pvar->auth_state.cur_cred.method == SSH_AUTH_RSA) { // ���J���F��
		buffer_t *signbuf = NULL;
		buffer_t *blob = NULL;
		int bloblen;
		char *signature = NULL;
		int siglen;
		Key *keypair = pvar->auth_state.cur_cred.key_pair;
		ssh_keyalgo keyalgo;
		char *keyalgo_name;

		if (get_SSH2_publickey_blob(pvar, &blob, &bloblen) == FALSE) {
			goto error;
		}

		keyalgo = choose_SSH2_keysign_algorithm(pvar, keypair->type);
		keyalgo_name = get_ssh2_hostkey_algorithm_name(keyalgo);

		// step1
		signbuf = buffer_init();
		if (signbuf == NULL) {
			buffer_free(blob);
			goto error;
		}
		// �Z�b�V����ID
		buffer_append_length(signbuf, pvar->session_id, pvar->session_id_len);
		buffer_put_char(signbuf, SSH2_MSG_USERAUTH_REQUEST);
		s = username;  // ���[�U��
		buffer_put_string(signbuf, s, strlen(s));
		s = connect_id;
		buffer_put_string(signbuf, s, strlen(s));
		s = "publickey";
		buffer_put_string(signbuf, s, strlen(s));
		buffer_put_char(signbuf, 1); // true

		s = keyalgo_name;
		buffer_put_string(signbuf, s, strlen(s));

		s = buffer_ptr(blob);
		buffer_append_length(signbuf, s, bloblen);

		// �����̍쐬
		if (generate_SSH2_keysign(keypair, &signature, &siglen, buffer_ptr(signbuf), buffer_len(signbuf), keyalgo) == FALSE) {
			buffer_free(blob);
			buffer_free(signbuf);
			goto error;
		}

		// step3
		s = "publickey";
		buffer_put_string(msg, s, strlen(s));
		buffer_put_char(msg, 1); // true

		s = keyalgo_name;
		buffer_put_string(msg, s, strlen(s));

		s = buffer_ptr(blob);
		buffer_append_length(msg, s, bloblen);
		buffer_append_length(msg, signature, siglen);

		buffer_free(blob);
		buffer_free(signbuf);
		free(signature);

	} else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) { // Pageant
		unsigned char *puttykey;
		unsigned char *keytype_name, *keyalgo_name;
		ssh_keytype keytype;
		ssh_keyalgo keyalgo;

		s = "publickey";
		buffer_put_string(msg, s, strlen(s));
		buffer_put_char(msg, 0); // false

		if (pvar->pageant_keycurrent != 0) {
			// ���O�̌����X�L�b�v
			len = get_uint32_MSBfirst(pvar->pageant_curkey);
			pvar->pageant_curkey += 4 + len;
			// ���O�̌��̃R�����g���X�L�b�v
			len = get_uint32_MSBfirst(pvar->pageant_curkey);
			pvar->pageant_curkey += 4 + len;
			// ���̌��̈ʒu�֗���
		}
		puttykey = pvar->pageant_curkey;

		// ����ʂ��痘�p���鏐���A���S���Y�������肷��
		len = get_uint32_MSBfirst(puttykey+4);
		keytype_name = puttykey + 8;
		keytype = get_hostkey_type_from_name(keytype_name);
		keyalgo = choose_SSH2_keysign_algorithm(pvar, keytype);
		keyalgo_name = get_ssh2_hostkey_algorithm_name(keyalgo);

		// �A���S���Y�����R�s�[����
		len = strlen(keyalgo_name);
		buffer_put_string(msg, keyalgo_name, len);

		// �����R�s�[����
		len = get_uint32_MSBfirst(puttykey);
		puttykey += 4;
		buffer_put_string(msg, puttykey, len);
		puttykey += len;

		pvar->pageant_keycurrent++;

		SSH2_dispatch_add_message(SSH2_MSG_USERAUTH_PK_OK);

	} else {
		goto error;

	}

	// �p�P�b�g���M
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_USERAUTH_REQUEST, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	{
		logprintf(LOG_LEVEL_VERBOSE,
				  "SSH2_MSG_USERAUTH_REQUEST was sent %s(). (method %d)",
				  __FUNCTION__,
				  pvar->auth_state.cur_cred.method);
#if defined(SSH2_DEBUG)
		logprintf_hexdump(LOG_LEVEL_SSHDUMP,
						  buffer_ptr(msg), len,
						  "send %s:%d %s() len=%d",
						  __FILE__, __LINE__,
						  __FUNCTION__, len);
#endif
	}
	buffer_free(msg);

	return TRUE;

error:
	buffer_free(msg);

	return FALSE;
}


//
// SSH2 heartbeat procedure
//
// NAT���ɂ����āASSH�N���C�A���g�ƃT�[�o�ԂŒʐM���������Ȃ������ꍇ�A
// ���[�^�������NAT�e�[�u�����N���A���邱�Ƃ�����ASSH�R�l�N�V������
// �؂�Ă��܂����Ƃ�����B����I�ɁA�N���C�A���g����_�~�[�p�P�b�g��
// ���M���邱�ƂőΏ�����B(2004.12.10 yutaka)
//
// ���[�h���X�_�C�A���O����p�P�b�g���M����悤�ɕύX�B(2007.12.26 yutaka)
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
				logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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
					TTEndDialog(hWnd, 0);
					return TRUE;
				default:
					return FALSE;
			}
			break;

		case WM_CLOSE:
			// close�{�^������������Ă� window �����Ȃ��悤�ɂ���B
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

	// ���łɎ��s���Ȃ牽�������ɕԂ�B
	if (instance > 0)
		return 0;
	instance++;

	for (;;) {
		// �\�P�b�g���N���[�Y���ꂽ��X���b�h���I���
		if (pvar->socket == INVALID_SOCKET)
			break;

		// ��莞�Ԗ��ʐM�ł���΁A�T�[�o�փ_�~�[�p�P�b�g�𑗂�
		// 臒l��0�ł���Ή������Ȃ��B
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
	HANDLE thread = INVALID_HANDLE_VALUE;
	unsigned tid;
	HWND hDlgWnd;

	// ���[�h���X�_�C�A���O���쐬�B�n�[�g�r�[�g�p�Ȃ̂Ń_�C�A���O�͔�\���̂܂܂�
	// ����̂ŁA���\�[�XID�͂Ȃ�ł��悢�B
	hDlgWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SSHSCP_PROGRESS),
               pvar->cv->HWin, (DLGPROC)ssh_heartbeat_dlg_proc);
	pvar->ssh_hearbeat_dialog = hDlgWnd;

	// TTSSH�� thread-safe �ł͂Ȃ��̂ŃX���b�h������̃p�P�b�g���M�͕s�B(2007.12.26 yutaka)
	thread = (HANDLE)_beginthreadex(NULL, 0, ssh_heartbeat_thread, pvar, 0, &tid);
	if (thread == 0) {
		// TODO:
		thread = INVALID_HANDLE_VALUE;
	}
	pvar->ssh_heartbeat_thread = thread;
}

// �X���b�h�̒�~ (2004.12.27 yutaka)
void halt_ssh_heartbeat_thread(PTInstVar pvar)
{
	if (pvar->ssh_heartbeat_thread != INVALID_HANDLE_VALUE) {
		WaitForSingleObject(pvar->ssh_heartbeat_thread, INFINITE);
		CloseHandle(pvar->ssh_heartbeat_thread);
		pvar->ssh_heartbeat_thread = INVALID_HANDLE_VALUE;

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

	{
		int len = pvar->ssh_state.payloadlen;
		char *data = pvar->ssh_state.payload;
		logprintf_hexdump(LOG_LEVEL_SSHDUMP,
						  data, len,
						  "receive %s:%d %s() len=%d",
						  __FILE__, __LINE__,
						  __FUNCTION__, len);
	}

	// �p�X���[�h�̔j�� (2006.8.22 yutaka)
	if (pvar->settings.remember_password == 0) {
		destroy_malloced_string(&pvar->auth_state.cur_cred.password);
	}

	// �F��OK
	pvar->userauth_success = 1;
	pvar->auth_state.partial_success = 0;

	// �f�B�X�p�b�`���[�`���̍Đݒ�
	do_SSH2_dispatch_setup_for_transfer(pvar);

	if (pvar->nosession) {
		// start forwarding
		FWD_prep_forwarding(pvar);
		FWD_enter_interactive_mode(pvar);
	}
	else {
		// �`���l���ݒ�
		// FWD_prep_forwarding()��shell ID���g���̂ŁA��ɐݒ�������Ă���B(2005.7.3 yutaka)
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
			notify_fatal_error(pvar, pvar->UIMsg, TRUE);
			return FALSE;
		}
		// �V�F����ID������Ă���
		pvar->shell_id = c->self_id;

		// �V�F���I�[�v��
		msg = buffer_init();
		if (msg == NULL) {
			// TODO: error check
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
			return FALSE;
		}
		s = "session";
		buffer_put_string(msg, s, strlen(s));  // ctype
		buffer_put_int(msg, c->self_id);  // self(channel number)
		buffer_put_int(msg, c->local_window);  // local_window
		buffer_put_int(msg, c->local_maxpacket);  // local_maxpacket
		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_OPEN, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		{
			logprintf(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_OPEN was sent at %s().", __FUNCTION__);
			logprintf_hexdump(LOG_LEVEL_SSHDUMP,
							  buffer_ptr(msg), len,
							  "send %s:%d %s() len=%d",
							  __FILE__, __LINE__,
							  __FUNCTION__, len);
		}
		buffer_free(msg);
	}

	// �n�[�g�r�[�g�E�X���b�h�̊J�n (2004.12.11 yutaka)
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

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	logprintf_hexdump(LOG_LEVEL_SSHDUMP,
					  data, len,
					  "receive %s:%d %s() len=%d",
					  __FILE__, __LINE__,
					  __FUNCTION__, len);

	cstring = buffer_get_string(&data, NULL); // �F�ؕ������X�g�̎擾
	partial = data[0];
	data += 1;

	// �L���ȔF�ؕ������Ȃ��ꍇ
	if (cstring == NULL) {
		UTIL_get_lang_msg("MSG_SSH_SERVER_NO_AUTH_METHOD_ERROR", pvar,
		                  "The server doesn't have valid authentication method.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		return FALSE;
	}

	// partial �� TRUE �̂Ƃ��͎��̔F�؂̏���������
	if (partial) {
		logprintf(LOG_LEVEL_VERBOSE, "Authenticated using \"%s\" with partial success.", cstring);

		// �͂��߂Ď��̔F�؂�v����Ƃ�
		if (!pvar->auth_state.partial_success) {
			// autologin �𖳌��ɂ���
			pvar->ssh2_autologin = 0;

			// ���̐ڑ��͕����F�ؗv�������Ă���
			pvar->auth_state.multiple_required_auth = 1;
		}

		// �o���Ă��� password ���N���A���Ď��̔F�؂ɔ�����
		SecureZeroMemory(pvar->ssh2_password, sizeof(pvar->ssh2_password));

		// ���̔F�؂̔F�ؕ������A���O�ɐ��������F�ؕ����ɂȂ�Ȃ��悤����������
		pvar->ssh2_authmethod = SSH_AUTH_NONE;
	}

	// �F�؃_�C�A���O�̏���
	//   tryed_ssh2_authlist �� FALSE �̂Ƃ� = �܂����ۂ̔F�؂����s�����Ă��Ȃ��inone �̕Ԏ����A���Ă����Ƃ���j
	//     CheckAuthListFirst �� TRUE �̂Ƃ��́A�\�����̃_�C�A���O���ύX�����
	//   partial �� TRUE �̂Ƃ�
	//     "���̔F��" �̂��߂ɁA���̂��ƕ\�������_�C�A���O�ŗ��p�����
	if (!pvar->tryed_ssh2_authlist || partial) {
		int type = 0;

		pvar->tryed_ssh2_authlist = TRUE;
		pvar->auth_state.partial_success = partial;

		// �F�؃_�C�A���O�̃��W�I�{�^�����X�V
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

		pvar->ssh2_authlist = cstring; // �s�v�ɂȂ�����t���[���邱��
		logprintf(LOG_LEVEL_VERBOSE, "method list from server: %s", cstring);

		if (pvar->auth_state.partial_success) {
			// �����F�؎��A�_�C�A���O���o��������
			goto auth;
		}

		if (pvar->ssh2_authmethod == SSH_AUTH_TIS &&
		    pvar->ask4passwd &&
		    pvar->session_settings.CheckAuthListFirst &&
		    pvar->auth_state.auth_dialog != NULL) {
			// challenge �� ask4passwd �̂Ƃ��A�F�؃��\�b�h�ꗗ�������擾������
			// �����I�� TIS �_�C�A���O���o�����߂� OK ������
			SendMessage(pvar->auth_state.auth_dialog, WM_COMMAND, IDOK, 0);
		}
		else {
			// �ЂƂ܂� none �Ŏ��s���ĕԂ��Ă����Ƃ���Ȃ̂ŁA���ۂ̃��O�C��������
			//   CheckAuthListFirst �� TRUE �̏ꍇ�͔F�ؕ������m�肵�Ă��Ȃ��̂ŁA���O�C�������͍s���Ȃ�
			do_SSH2_authrequest(pvar);
		}

		return TRUE;
	}

	// none �ł͂Ȃ����ۂ̔F�؂̎��s�Ɏ��s����

	// TCP connection closed
	//notify_closed_connection(pvar);

	// retry count�̒ǉ�
	if (pvar->auth_state.cur_cred.method != SSH_AUTH_PAGEANT) {
		pvar->userauth_retry_count++;
	}
	else {
		if (pvar->pageant_keycount <= pvar->pageant_keycurrent ||
		    pvar->pageant_keyfinal) {
			// �S�Ă̌��������I�����
			// �܂��́ATRUE �ł̃��O�C���Ɏ��s���Ă����ɗ���
			safefree(pvar->pageant_key);
			pvar->userauth_retry_count++;
		}
		else {
			// �܂���������
			do_SSH2_authrequest(pvar);
			return TRUE;
		}
	}

	if (pvar->ssh2_autologin == 1 && !pvar->auth_state.partial_success) {
		char uimsg[MAX_UIMSG];
		// SSH2�������O�C�����L���̏ꍇ�́A���g���C�͍s��Ȃ��B
		UTIL_get_lang_msg("MSG_SSH_AUTH_FAILURE_ERROR", pvar,
		                  "SSH2 auto-login error: user authentication failed.");
		strncpy_s(uimsg, sizeof(uimsg), pvar->UIMsg, _TRUNCATE);

		if (pvar->ssh2_authlist != NULL || strlen(pvar->ssh2_authlist) != 0) {
			if ((pvar->auth_state.supported_types & (1 << pvar->ssh2_authmethod)) == 0) {
				// �g�p�����F�؃��\�b�h�̓T�|�[�g����Ă��Ȃ�����
				UTIL_get_lang_msg("MSG_SSH_SERVER_UNSUPPORT_AUTH_METHOD_ERROR", pvar,
				                  "\nAuthentication method is not supported by server.");
				strncat_s(uimsg, sizeof(uimsg), pvar->UIMsg, _TRUNCATE);
			}
		}
		notify_fatal_error(pvar, uimsg, TRUE);
		return TRUE;
	}

	// �ǉ��F�؂̂Ƃ�
	// �܂��̓��[�U�F�؂Ɏ��s�����Ƃ�
	//   ���[�U���͌Œ肵�āA�p�X���[�h�̍ē���(SSH1 �Ɠ���)
auth:
	AUTH_set_generic_mode(pvar);
	AUTH_advance_to_next_cred(pvar);
	pvar->ssh_state.status_flags &= ~STATUS_DONT_SEND_CREDENTIALS;
	try_send_credentials(pvar);

	return TRUE;
}

void sanitize_str(buffer_t *buff, unsigned char *src, size_t srclen)
{
	unsigned char *start = src;
	int cplen = 0;
	unsigned int i;

	if (srclen == 0)
		return;

	for (i=0; i<srclen; i++) {
		if (src[i] < 0x20 && src[i] != '\t') {
			if (cplen > 0) {
				buffer_append(buff, start, cplen);
			}

			if (src[i] == '\r') {
				buffer_append(buff, "\r\n", 2);

				if (i < srclen - 1 && src[i+1] == '\n') {
					i++;
				}
			}
			else if (src[i] == '\n') {
				buffer_append(buff, "\r\n", 2);
			}

			start = src + i + 1;
			cplen = 0;
		}
		else {
			cplen++;
		}
	}

	if (cplen > 0) {
		buffer_append(buff, start, cplen);
	}

	buffer_append(buff, "\0", 1);
}

/**
 *	UTF-8 ���������M������ɕϊ�����
 */
static char *ConvertReceiveStr(TComVar *cv, char *strU8, size_t *len)
{
	TTTSet *pts = cv->ts;
	return MakeOutputStringConvU8(strU8, pts->KanjiCode, 0, 0, 0, len);
}

/*
 * SSH_MSG_USERAUTH_BANNER:
 *    byte      SSH_MSG_USERAUTH_BANNER
 *    string    message in ISO-10646 UTF-8 encoding
 *    string    language tag
 */
static BOOL handle_SSH2_userauth_banner(PTInstVar pvar)
{
	int msglen, ltaglen;
	char buff[2048];
	char *new_payload_buffer = NULL;

	logputs(LOG_LEVEL_INFO, "SSH2_MSG_USERAUTH_BANNER was received.");

	if (!get_string_from_payload(pvar, buff, sizeof(buff), &msglen, TRUE)) {
		logprintf(LOG_LEVEL_WARNING, "%s: banner payload corrupted.", __FUNCTION__);
		return TRUE;
	}

	if (msglen > 0) {
		char *msg;
		wchar_t *msgW;

		if (pvar->authbanner_buffer == NULL) {
			pvar->authbanner_buffer = buffer_init();
		}
		else {
			buffer_clear(pvar->authbanner_buffer);
		}

		if (pvar->authbanner_buffer != NULL) {
			sanitize_str(pvar->authbanner_buffer, buff, MIN(msglen, sizeof(buff)));
			msg = buffer_ptr(pvar->authbanner_buffer);
			msglen = buffer_len(pvar->authbanner_buffer) - 1; // NUL Terminate ���͐����Ȃ�
		}
		else {
			// �������m�ێ��s���͕ϊ��O�̕������\������B
			// �����AC0 ���䕶�������̂܂ܕ\�����悤�Ƃ���̂Ŗ]�܂����Ȃ������B
			msg = buff;
		}

		switch (pvar->settings.AuthBanner) {
		case 0:
			break;
		case 1:
			if (pvar->authbanner_buffer != NULL) {
				// ��M������ɕϊ�����
				size_t msglen_s;
				msg = ConvertReceiveStr(pvar->cv, msg, &msglen_s);
				msglen = (int)msglen_s;
				new_payload_buffer = msg;
				pvar->ssh_state.payload_datastart = 0;
				pvar->ssh_state.payload_datalen = msglen;
			}
			else {
				pvar->ssh_state.payload_datastart = 4;
				pvar->ssh_state.payload_datalen = msglen;
			}
			break;
		case 2:
			msgW = ToWcharU8(msg);
			if (msgW) {
				MessageBoxW(pvar->cv->HWin, msgW, L"Authentication Banner", MB_OK | MB_ICONINFORMATION);
				free(msgW);
			}
			break;
		case 3:
			msgW = ToWcharU8(msg);
			if (msgW) {
				NotifySetIconID(pvar->cv, hInst, pvar->settings.IconID);
				NotifyInfoMessageW(pvar->cv, msgW, L"Authentication Banner");
				NotifySetIconID(pvar->cv, NULL, 0);
				free(msgW);
			}
			break;
		}
		logprintf(LOG_LEVEL_NOTICE, "Banner len: %d, Banner message: %s.", msglen, msg);
	}
	else {
		logprintf(LOG_LEVEL_VERBOSE, "Empty banner");
	}

	if (!get_string_from_payload(pvar, buff, sizeof(buff), &ltaglen, TRUE)) {
		logprintf(LOG_LEVEL_WARNING, "%s: langtag payload corrupted.", __FUNCTION__);
		return TRUE;
	}

	if (ltaglen > 0) {
		logprintf(LOG_LEVEL_NOTICE, "Banner ltag len: %d, Banner Language Tag: %s", ltaglen, buff);
	}
	else {
		logprintf(LOG_LEVEL_VERBOSE, "Empty Language Tag");
	}

	if (new_payload_buffer) {
		pvar->ssh_state.payload = new_payload_buffer;
	}

	return TRUE;
}


// SSH2 ���b�Z�[�W 60 �Ԃ̏����֐�
//
// SSH2 �ł͈ȉ��̃��b�Z�[�W�� 60 �Ԃ֏d�����Ċ��蓖�Ă��Ă���B
//
// * SSH2_MSG_USERAUTH_INFO_REQUEST (keyboard-interactive)
// * SSH2_MSG_USERAUTH_PK_OK (publickey / Tera Term �ł� Pageant �F�؂̂�)
// * SSH2_MSG_USERAUTH_PASSWD_CHANGEREQ (password)
//
// ����̎����ł͓������b�Z�[�W�ԍ������݂ł��Ȃ��̂ŁA
// 60 �Ԃ͂��̊֐��Ŏ󂯁Amethod �ɂ���đΉ�����n���h���֐��ɐU�蕪����B
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

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	logprintf_hexdump(LOG_LEVEL_SSHDUMP,
					  data, len,
					  "receive %s:%d %s() len=%d",
					  __FILE__, __LINE__,
					  __FUNCTION__, len);

	///////// step1
	// get string
	name = buffer_get_string(&data, NULL);
	inst = buffer_get_string(&data, NULL);
	lang = buffer_get_string(&data, NULL);
	lprompt[0] = 0;
	if (inst == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_get_string returns NULL. (inst)", __FUNCTION__);
	}
	else if (strlen(inst) > 0) {
		strncat_s(lprompt, sizeof(lprompt), inst, _TRUNCATE);
		strncat_s(lprompt, sizeof(lprompt), "\r\n", _TRUNCATE);
	}
	if (lang == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_get_string returns NULL. (lang)", __FUNCTION__);
	}
	else if (strlen(lang) > 0) {
		strncat_s(lprompt, sizeof(lprompt), lang, _TRUNCATE);
		strncat_s(lprompt, sizeof(lprompt), "\r\n", _TRUNCATE);
	}

	logprintf(LOG_LEVEL_VERBOSE, "%s: user=%s, inst=%s, lang=%s", __FUNCTION__,
		NonNull(name), NonNull(inst), NonNull(lang));

	free(name);
	free(inst);
	free(lang);

	// num-prompts
	num = get_uint32_MSBfirst(data);
	data += 4;

	logprintf(LOG_LEVEL_VERBOSE, "%s: prompts=%d", __FUNCTION__, num);

	///////// step2
	// �T�[�o�փp�X�t���[�Y�𑗂�
	msg = buffer_init();
	if (msg == NULL) {
		// TODO: error check
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
		return FALSE;
	}
	buffer_put_int(msg, num);

	// �p�X���[�h�ύX�̏ꍇ�A���b�Z�[�W������΁A�\������B(2010.11.11 yutaka)
	if (num == 0) {
		if (strlen(lprompt) > 0)
			MessageBox(pvar->cv->HWin, lprompt, "USERAUTH INFO_REQUEST", MB_OK | MB_ICONINFORMATION);
	}

	// �v�����v�g�̐����� prompt & echo ���J��Ԃ����B
	for (i = 0 ; i < num ; i++) {
		// get string
		slen = get_uint32_MSBfirst(data);
		data += 4;
		prompt = data;  // prompt
		data += slen;

		// get boolean
		echo = data[0];
		data[0] = '\0'; // ���O�o�ׁ͂̈A�ꎞ�I�� NUL Terminate ����

		logprintf(LOG_LEVEL_VERBOSE, "%s:   prompt[%d]=\"%s\", echo=%d, pass-state=%d", __FUNCTION__,
			i, prompt, slen, pvar->keyboard_interactive_password_input);

		data[0] = echo; // ���O�o�͂��s�����̂ŁA���̒l�ɏ����߂�
		data += 1;

		// keyboard-interactive method (2005.3.12 yutaka)
		if (pvar->keyboard_interactive_password_input == 0 &&
			pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) {
			AUTH_set_TIS_mode(pvar, prompt, slen, echo);
			AUTH_advance_to_next_cred(pvar);
			pvar->ssh_state.status_flags &= ~STATUS_DONT_SEND_CREDENTIALS;
			//try_send_credentials(pvar);
			buffer_free(msg);
			return TRUE;
		}

		// TODO: �����Ńv�����v�g��\�����ă��[�U������͂�����̂������B
		s = pvar->auth_state.cur_cred.password;
		buffer_put_string(msg, s, strlen(s));

		// ���g���C�ɑΉ��ł���悤�A�t���O���N���A����B(2010.11.11 yutaka)
		pvar->keyboard_interactive_password_input = 0;
	}

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_USERAUTH_INFO_RESPONSE, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	{
		logprintf(LOG_LEVEL_VERBOSE,
				  "SSH2_MSG_USERAUTH_INFO_RESPONSE was sent %s().",
				  __FUNCTION__);
		logprintf_hexdump(LOG_LEVEL_SSHDUMP,
						  buffer_ptr(msg), len,
						  "send %s:%d %s() len=%d",
						  __FILE__, __LINE__,
						  __FUNCTION__, len);
	}
	buffer_free(msg);

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
		unsigned char *signedmsg;
		int signedlen;

		unsigned char *keytype_name, *keyalgo_name;
		ssh_keytype keytype;
		ssh_keyalgo keyalgo;
		ssh_agentflag signflag;

		logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_USERAUTH_PK_OK was received.");
		{
			const char *data = pvar->ssh_state.payload;
			int len = pvar->ssh_state.payloadlen;
			logprintf_hexdump(LOG_LEVEL_SSHDUMP,
							  data, len,
							  "receive %s:%d %s() len=%d",
							  __FILE__, __LINE__,
							  __FUNCTION__, len);
		}
		username = pvar->auth_state.user;  // ���[�U��

		// ��������f�[�^���쐬
		signbuf = buffer_init();
		if (signbuf == NULL) {
			safefree(pvar->pageant_key);
			return FALSE;
		}
		buffer_append_length(signbuf, pvar->session_id, pvar->session_id_len);
		buffer_put_char(signbuf, SSH2_MSG_USERAUTH_REQUEST);
		s = username;  // ���[�U��
		buffer_put_string(signbuf, s, strlen(s));
		s = connect_id;
		buffer_put_string(signbuf, s, strlen(s));
		s = "publickey";
		buffer_put_string(signbuf, s, strlen(s));
		buffer_put_char(signbuf, 1); // true

		puttykey = pvar->pageant_curkey;

		// ����ʂ��痘�p���鏐���A���S���Y�������肷��
		len = get_uint32_MSBfirst(puttykey+4);
		keytype_name = puttykey + 8;
		keytype = get_hostkey_type_from_name(keytype_name);
		keyalgo = choose_SSH2_keysign_algorithm(pvar, keytype);
		keyalgo_name = get_ssh2_hostkey_algorithm_name(keyalgo);
		signflag = get_ssh2_agent_flag(keyalgo);

		// �A���S���Y�����R�s�[����
		len = strlen(keyalgo_name);
		buffer_put_string(signbuf, keyalgo_name, len);

		// �����R�s�[����
		len = get_uint32_MSBfirst(puttykey);
		puttykey += 4;
		buffer_put_string(signbuf, puttykey, len);
		puttykey += len;

		// Pageant �ɏ������Ă��炤
		signedmsg = putty_sign_ssh2_key(pvar->pageant_curkey,
		                                buffer_ptr(signbuf), buffer_len(signbuf),
		                                &signedlen, signflag);
		buffer_free(signbuf);
		if (signedmsg == NULL) {
			safefree(pvar->pageant_key);
			return FALSE;
		}


		// �y�C���[�h�̍\�z
		msg = buffer_init();
		if (msg == NULL) {
			safefree(pvar->pageant_key);
			safefree(signedmsg);
			return FALSE;
		}
		s = username;  // ���[�U��
		buffer_put_string(msg, s, strlen(s));
		s = connect_id;
		buffer_put_string(msg, s, strlen(s));
		s = "publickey";
		buffer_put_string(msg, s, strlen(s));
		buffer_put_char(msg, 1); // true

		puttykey = pvar->pageant_curkey;

		// �A���S���Y�����R�s�[����
		len = strlen(keyalgo_name);
		buffer_put_string(msg, keyalgo_name, len);

		// �����R�s�[����
		len = get_uint32_MSBfirst(puttykey);
		puttykey += 4;
		buffer_put_string(msg, puttykey, len);
		puttykey += len;

		// �������ꂽ�f�[�^
		len  = get_uint32_MSBfirst(signedmsg);
		buffer_put_string(msg, signedmsg + 4, len);
		free(signedmsg);

		// �p�P�b�g���M
		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, SSH2_MSG_USERAUTH_REQUEST, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		{
			logprintf(LOG_LEVEL_VERBOSE,
					  "%s: sending SSH2_MSG_USERAUTH_REQUEST method=publickey",
					  __FUNCTION__);
			logprintf_hexdump(LOG_LEVEL_SSHDUMP,
							  buffer_ptr(msg), len,
							  "send %s:%d %s() len=%d",
							  __FILE__, __LINE__,
							  __FUNCTION__, len);
		}
		buffer_free(msg);

		pvar->pageant_keyfinal = TRUE;

		return TRUE;
	}

#define PASSWD_MAXLEN 150

struct change_password {
	PTInstVar pvar;
	char passwd[PASSWD_MAXLEN];
	char new_passwd[PASSWD_MAXLEN];
};

static INT_PTR CALLBACK passwd_change_dialog(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char old_passwd[PASSWD_MAXLEN];
	char new_passwd[PASSWD_MAXLEN];
	char retype_passwd[PASSWD_MAXLEN];
	static struct change_password *cp;
	char uimsg[MAX_UIMSG];
	static PTInstVar pvar;

	switch (msg) {
	case WM_INITDIALOG:
		cp = (struct change_password *)lParam;
		pvar = cp->pvar;

		GetWindowText(dlg, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_PASSCHG_TITLE", pvar, uimsg);
		SetWindowText(dlg, pvar->UIMsg);

		GetDlgItemText(dlg, IDC_PASSWD_CHANGEREQ_MSG, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_PASSCHG_MESSAGE", pvar, uimsg);
		SetDlgItemText(dlg, IDC_PASSWD_CHANGEREQ_MSG, pvar->UIMsg);

		GetDlgItemText(dlg, IDC_OLD_PASSWD_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_PASSCHG_OLDPASSWD", pvar, uimsg);
		SetDlgItemText(dlg, IDC_OLD_PASSWD_LABEL, pvar->UIMsg);

		GetDlgItemText(dlg, IDC_NEW_PASSWD_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_PASSCHG_NEWPASSWD", pvar, uimsg);
		SetDlgItemText(dlg, IDC_NEW_PASSWD_LABEL, pvar->UIMsg);

		GetDlgItemText(dlg, IDC_CONFIRM_PASSWD_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_PASSCHG_CONFIRMPASSWD", pvar, uimsg);
		SetDlgItemText(dlg, IDC_CONFIRM_PASSWD_LABEL, pvar->UIMsg);

		SetFocus(GetDlgItem(dlg, IDC_OLD_PASSWD));

		CenterWindow(dlg, GetParent(dlg));

		return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			SendMessage(GetDlgItem(dlg, IDC_OLD_PASSWD), WM_GETTEXT , sizeof(old_passwd), (LPARAM)old_passwd);
			SendMessage(GetDlgItem(dlg, IDC_NEW_PASSWD), WM_GETTEXT , sizeof(new_passwd), (LPARAM)new_passwd);
			SendMessage(GetDlgItem(dlg, IDC_CONFIRM_PASSWD), WM_GETTEXT , sizeof(retype_passwd), (LPARAM)retype_passwd);

			if (strcmp(new_passwd, retype_passwd) != 0) {
				UTIL_get_lang_msg("MSG_PASSCHG_MISMATCH", pvar, "Mismatch; try again.");
				MessageBox(NULL, pvar->UIMsg, "ERROR", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}

			if (new_passwd[0] == 0) {
				// �_�C�A���O���J���Ă����� Return �������Ă��܂������̑΍�ׁ̈A
				// �Ƃ肠�����V�p�X���[�h����̏ꍇ���͂����Ă����B
				// c.f. Ticket: #38970
				return FALSE;
			}

			strncpy_s(cp->passwd, sizeof(cp->passwd), old_passwd, _TRUNCATE);
			strncpy_s(cp->new_passwd, sizeof(cp->new_passwd), new_passwd, _TRUNCATE);

			TTEndDialog(dlg, 1); // dialog close
			return TRUE;

		case IDCANCEL:
			// �ڑ���؂�
			notify_closed_connection(pvar, "authentication cancelled");
			TTEndDialog(dlg, 0); // dialog close
			return TRUE;
		}
	}

	return FALSE;
}

BOOL handle_SSH2_userauth_passwd_changereq(PTInstVar pvar)
{
	int len;
	INT_PTR ret;
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
	ret = TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SSHPASSWD_INPUT), pvar->cv->HWin, passwd_change_dialog, (LPARAM)&cp);

	if (ret == -1) {
		logprintf(LOG_LEVEL_WARNING, "%s: DialogBoxParam failed.", __FUNCTION__);
		return FALSE;
	}
	else if (ret == 0) {
		logprintf(LOG_LEVEL_NOTICE, "%s: dialog cancelled.", __FUNCTION__);
		return FALSE;
	}

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	info = buffer_get_string(&data, NULL);
	lang = buffer_get_string(&data, NULL);
	if (info == NULL || lang == NULL) {
		logprintf(LOG_LEVEL_ERROR,
			"%s: buffer_get_string returns NULL. info=%s, lang=%s", __FUNCTION__,
			NonNull(info), NonNull(lang));
	}
	else {
		logprintf(LOG_LEVEL_VERBOSE, "%s: info=%s, lang=%s\n", __FUNCTION__, info, lang);
	}
	free(info);
	free(lang);

	msg = buffer_init();
	if (msg == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
		return FALSE;
	}

	// �y�C���[�h�̍\�z
	username = pvar->auth_state.user;  // ���[�U��
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

	// �p�P�b�g���M
	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_USERAUTH_REQUEST, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	{
		logprintf(LOG_LEVEL_VERBOSE,
				  "%s: sending SSH2_MSG_USERAUTH_REQUEST",
				  __FUNCTION__);
		logprintf_hexdump(LOG_LEVEL_SSHDUMP,
						  buffer_ptr(msg), len,
						  "send %s:%d %s() len=%d",
						  __FILE__, __LINE__,
						  __FUNCTION__, len);
	}
	buffer_free(msg);

	return TRUE;
}

/*
 * SSH_MSG_CHANNEL_REQUEST ���M�֐�
 * type-specific data �� string �� 0 �` 2 �̕��ɑΉ��B
 * �g�p���Ȃ����b�Z�[�W�� NULL �ɂ���B
 * type-specific data �����̌`���ɂ͑Ή����Ă��Ȃ��̂ŁA���O�ő��鎖�B
 */
static BOOL send_channel_request_gen(PTInstVar pvar, Channel_t *c, unsigned char *req, int want_reply, unsigned char *msg1, unsigned char *msg2)
{
	buffer_t *msg;
	unsigned char *outmsg;
	int len;

	msg = buffer_init();
	if (msg == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
		return FALSE;
	}
	if (c->remote_id == SSH_CHANNEL_INVALID) {
		logprintf(LOG_LEVEL_ERROR, "%s: invalid remote channel number (%d).", __FUNCTION__, c->remote_id);
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

	logprintf(LOG_LEVEL_VERBOSE, "%s: sending SSH2_MSG_CHANNEL_REQUEST. "
	          "local: %d, remote: %d, request-type: %s, msg1=%s, msg2=%s", __FUNCTION__,
	          c->self_id, c->remote_id, req, msg1 ? msg1 : "none", msg2 ? msg2 : "none");
	return TRUE;
}

BOOL send_pty_request(PTInstVar pvar, Channel_t *c)
{
	buffer_t *msg, *ttymsg;
	char *req_type = "pty-req";  // pseudo terminal�̃��N�G�X�g
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
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL. (msg)", __FUNCTION__);
		return FALSE;
	}
	ttymsg = buffer_init();
	if (ttymsg == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL. (ttymsg)", __FUNCTION__);
		buffer_free(msg);
		return FALSE;
	}

	buffer_put_int(msg, c->remote_id);
	buffer_put_string(msg, req_type, strlen(req_type));
	buffer_put_char(msg, want_reply);  // want_reply (disable�ɕύX 2005/3/28 yutaka)

	buffer_put_string(msg, pvar->ts->TermType, strlen(pvar->ts->TermType));
	buffer_put_int(msg, pvar->ssh_state.win_cols);  // columns
	buffer_put_int(msg, pvar->ssh_state.win_rows);  // lines
	get_window_pixel_size(pvar, &x, &y);
	buffer_put_int(msg, x);  // window width (pixel):
	buffer_put_int(msg, y);  // window height (pixel):

	// TTY mode�͂����œn�� (2005.7.17 yutaka)
	buffer_put_char(ttymsg, SSH2_TTY_OP_OSPEED);
	buffer_put_int(ttymsg, pvar->ts->TerminalOutputSpeed);  // baud rate
	buffer_put_char(ttymsg, SSH2_TTY_OP_ISPEED);
	buffer_put_int(ttymsg, pvar->ts->TerminalInputSpeed);  // baud rate

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

	// SSH2�ł͕�����Ƃ��ď������ށB
	buffer_put_string(msg, buffer_ptr(ttymsg), buffer_len(ttymsg));

	len = buffer_len(msg);
	outmsg = begin_send_packet(pvar, SSH2_MSG_CHANNEL_REQUEST, len);
	memcpy(outmsg, buffer_ptr(msg), len);
	finish_send_packet(pvar);
	buffer_free(msg);
	buffer_free(ttymsg);

	logprintf(LOG_LEVEL_VERBOSE, "%s: sending SSH2_MSG_CHANNEL_REQUEST. "
	          "local: %d, remote: %d, request-type: %s, "
	          "term: %s, cols: %d, rows: %d, x: %d, y: %d, "
	          "out-speed: %d, in-speed: %d, verase: %s, onlcr: %s", __FUNCTION__,
	          c->self_id, c->remote_id, req_type, pvar->ts->TermType,
	          pvar->ssh_state.win_cols, pvar->ssh_state.win_rows, x, y,
	          pvar->ts->TerminalOutputSpeed, pvar->ts->TerminalInputSpeed,
	          (pvar->ts->BSKey==IdBS)?"^h":"^?", (pvar->ts->CRReceive==IdBS)?"on":"off");

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

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	logprintf_hexdump(LOG_LEVEL_SSHDUMP, data, len,
					  "receive %s:%d %s() len=%d",
					  __FILE__, __LINE__,
					  __FUNCTION__, len);
	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: channel not found. (%d)", __FUNCTION__, id);
		return FALSE;
	}

	remote_id = get_uint32_MSBfirst(data);
	data += 4;

	c->remote_id = remote_id;
	if (c->self_id == pvar->shell_id) {
		// �ŏ��̃`���l���ȊO�Ń��Z�b�g���Ă͂����Ȃ� (2008.12.19 maya)
		pvar->session_nego_status = 1;
	}

	// remote window size
	c->remote_window = get_uint32_MSBfirst(data);
	data += 4;
	c->remote_maxpacket = get_uint32_MSBfirst(data);
	data += 4;

	switch (c->type) {
	case TYPE_PORTFWD:
		// port-forwading��"direct-tcpip"�������B
		FWD_confirmed_open(pvar, c->local_num, -1);
		break;

	case TYPE_SHELL:
		// �|�[�g�t�H���[�f�B���O�̏��� (2005.2.26, 2005.6.21 yutaka)
		// �V�F���I�[�v���������Ƃ� X11 �̗v�����o���Ȃ��Ă͂Ȃ�Ȃ��B(2005.7.3 yutaka)
		FWD_prep_forwarding(pvar);
		FWD_enter_interactive_mode(pvar);

		// �G�[�W�F���g�]�� (2008.11.25 maya)
		if (pvar->session_settings.ForwardAgent) {
			// pty-req ���O�Ƀ��N�G�X�g���Ȃ��ƃG���[�ɂȂ�͗l
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
			// �t�@�C�����ɋ󔒂��܂܂�Ă��Ă��悢�悤�ɁA�t�@�C�������d���p���ň͂ށB
			// (2014.7.13 yutaka)
			_snprintf_s(buff, sizeof(buff), _TRUNCATE, "scp -p -f \"%s\"", c->scp.remotefile);
		}

		if (!send_channel_request_gen(pvar, c, "exec", want_reply, buff, NULL)) {
			return FALSE;;
		}

		// SCP�� remote-to-local �̏ꍇ�́A�T�[�o����̃t�@�C�����M�v�����o���B
		// ���̎��_�ł� remote window size ��"0"�Ȃ̂ŁA�����ɂ͑����Ȃ����A�x�����M�����ő�����B
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

		// SFTP�Z�b�V�������J�n���邽�߂̃l�S�V�G�[�V�������s���B
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
		logprintf(LOG_LEVEL_ERROR, "%s: Invalid channel-type. (%d)", __FUNCTION__, c->type);
		return FALSE;
	}
	return TRUE;
}

// SSH2 port-forwarding �ɂ����ăZ�b�V�������I�[�v���ł��Ȃ��ꍇ�̃T�[�o����̃��v���C�i���s�j
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

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: channel not found. (%d)", __FUNCTION__, id);
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
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_get_string returns NULL", __FUNCTION__);
	}
	UTIL_get_lang_msg("MSG_SSH_CHANNEL_OPEN_ERROR", pvar,
	                  "SSH2_MSG_CHANNEL_OPEN_FAILURE was received.\r\nchannel [%d]: reason: %s(%d) message: %s");
	_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, pvar->UIMsg,
	            id, rmsg, reason, NonNull(cstring));
	if ((pvar->settings.DisablePopupMessage & POPUP_MSG_FWD_channel_open) == 0) {
		notify_nonfatal_error(pvar, tmpbuf);
	}
	else {
		logputs(LOG_LEVEL_ERROR, tmpbuf);
	}

	free(cstring);

	if (c->type == TYPE_PORTFWD) {
		FWD_failed_open(pvar, c->local_num, reason);
	}

	// �`���l���̉���R����C�� (2007.5.1 maya)
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
	// size(4) + padding size(1) + type(1) + payload(N) + padding(X)
	//                                       ^data
	//           <-----------------size---------------------------->
	//                             <--------len------->
	//
	// data: ���b�Z�[�W�^�C�v�ɑ����y�C���[�h�̐擪���w���|�C���^
	data = pvar->ssh_state.payload;
	// len = size - (padding size + sizeof(padding size)) = sizeof(type) + sizeof(payload):
	// �y�C���[�h�����̒����Btype �����܂�
	len = pvar->ssh_state.payloadlen;

	len--;   // type ��������

	rtype = buffer_get_string(&data, &n);
	len -= (n + 4);

	want_reply = data[0];
	data++;
	len--;

	if (rtype == NULL) {
		// rtype �� NULL �Ŗ������̕ۏ�
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_get_string returns NULL.", __FUNCTION__);
	}
	else if (strcmp(rtype, "hostkeys-00@openssh.com") == 0) {
		// OpenSSH 6.8�ł́A�T�[�o�̃z�X�g�����X�V�����ƁA���̒ʒm������B
		// OpenSSH 6.8�̎����ł́A��ɐ����ŕԂ��悤�ɂȂ��Ă��邽�߁A
		// ����ɍ��킹�� Tera Term �ł������ƕԂ����Ƃɂ���B
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


// SSH2 port-forwarding (remote -> local)�ɑ΂��郊�v���C�i�����j
static BOOL handle_SSH2_request_success(PTInstVar pvar)
{
	// �K�v�ł���΃��O�����B���ɉ������Ȃ��Ă��悢�B
	logputs(LOG_LEVEL_VERBOSE, "SSH2_MSG_REQUEST_SUCCESS was received.");

	client_global_request_reply(pvar, SSH2_MSG_REQUEST_SUCCESS, 0, NULL);

	return TRUE;
}

// SSH2 port-forwarding (remote -> local)�ɑ΂��郊�v���C�i���s�j
static BOOL handle_SSH2_request_failure(PTInstVar pvar)
{
	// �K�v�ł���΃��O�����B���ɉ������Ȃ��Ă��悢�B
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
		c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: shell channel not found.", __FUNCTION__);
			return FALSE;
		}
		if (c->remote_id == SSH_CHANNEL_INVALID) {
			logprintf(LOG_LEVEL_ERROR, "%s: remote shell channel number is unknown.", __FUNCTION__);
			return FALSE;
		}
		pvar->agentfwd_enable = TRUE;
		return send_pty_request(pvar, c);

	} else if (pvar->session_nego_status == 2) {
		pvar->session_nego_status = 3;

		c = ssh2_channel_lookup(pvar->shell_id);
		if (c == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: shell channel not found.", __FUNCTION__);
			return FALSE;
		}
		if (c->remote_id == SSH_CHANNEL_INVALID) {
			logprintf(LOG_LEVEL_ERROR, "%s: remote shell channel number is unknown.", __FUNCTION__);
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
		logprintf(LOG_LEVEL_ERROR, "%s: channel not found. (%d)", __FUNCTION__, channel_id);
		return FALSE;
	}

	if (pvar->shell_id == channel_id) {
		if (c->type == TYPE_SUBSYSTEM_GEN) {
			// �T�u�V�X�e���̋N���Ɏ��s�����̂Ő؂�B
			char errmsg[MAX_UIMSG];
			UTIL_get_lang_msg("MSG_SSH_SUBSYSTEM_REQUEST_ERROR", pvar, "subsystem request failed. (%s)");
			_snprintf_s(errmsg, sizeof(errmsg), _TRUNCATE, pvar->UIMsg, pvar->subsystem_name);
			notify_fatal_error(pvar, errmsg, TRUE);
			return TRUE;
		}
		else { // TYPE_SHELL
			if (pvar->session_nego_status == 1) {
				// �����[�g�� auth-agent-req@openssh.com ���T�|�[�g����ĂȂ��̂�
				// �G���[�͋C�ɂ������֐i��
				logputs(LOG_LEVEL_VERBOSE, "auth-agent-req@openssh.com is not supported by remote host.");

				return send_pty_request(pvar, c);
			}
		}
	}

	ssh2_channel_delete(c);
	return TRUE;
}



// �N���C�A���g��window size���T�[�o�֒m�点��
static void do_SSH2_adjust_window_size(PTInstVar pvar, Channel_t *c)
{
	// window size��32KB�֕ύX���Alocal window�̔��ʂ��C���B
	// ����ɂ��SSH2�̃X���[�v�b�g�����シ��B(2006.3.6 yutaka)
	buffer_t *msg;
	unsigned char *outmsg;
	int len;

	// ���[�J����window size�ɂ܂��]�T������Ȃ�A�������Ȃ��B
	// added /2 (2006.3.6 yutaka)
	if (c->local_window > c->local_window_max/2)
		return;

	{
		// pty open
		msg = buffer_init();
		if (msg == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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
		// �N���C�A���g��window size�𑝂₷
		c->local_window = c->local_window_max;
	}

}


void ssh2_channel_send_close(PTInstVar pvar, Channel_t *c)
{
	if (SSHv2(pvar)) {
		buffer_t *msg;
		unsigned char *outmsg;
		int len;

		// ����channel�ɂ���close�𑗐M�ς݂Ȃ瑗��Ȃ�
		if (c->state & SSH_CHANNEL_STATE_CLOSE_SENT) {
			return;
		}

		// SSH2 server��channel close��`����
		msg = buffer_init();
		if (msg == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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

static INT_PTR CALLBACK ssh_scp_dlg_proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static int closed = 0;

	switch (msg) {
		case WM_INITDIALOG:
			closed = 0;
			CenterWindow(hWnd, GetParent(hWnd));
			return FALSE;

		// SCP�t�@�C����M(remote-to-local)���A�g�p����B
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
					// �E�B���h�E�������Ȃ�j������̂ł͂Ȃ��A��\���ɂ���݂̂Ƃ��āA
					// �X���b�h����̃��b�Z�[�W�������ł���悤�ɂ���B
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
			// close�{�^������������Ă� window �����Ȃ��悤�ɂ���B
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

	SetDlgItemTextU8(hWnd, IDC_FILENAME, c->scp.localfilefull);

	InitDlgProgress(hWnd, IDC_PROGBAR, &ProgStat);

	stime = GetTickCount();
	prev_elapsed = 0;

	do {
		int readlen, count=0;

		// Cancel�{�^�����������ꂽ��E�B���h�E��������B
		if (is_canceled_window(hWnd))
			goto cancel_abort;

		// �t�@�C������ǂݍ��񂾃f�[�^�͂��Ȃ炸�T�[�o�֑��M����B
		readlen = max(4096, min(buflen, c->remote_window)); // min 4KB
		ret = fread(buf, 1, readlen, c->scp.localfp);
		if (ret == 0)
			break;

		// remote_window ���񕜂���܂ő҂�
		do {
			// socket or channel���N���[�Y���ꂽ��X���b�h���I���
			if (pvar->socket == INVALID_SOCKET || c->scp.state == SCP_CLOSING || c->used == 0)
				goto abort;

			if (ret > c->remote_window) {
				Sleep(100);
			}

			// 100�񔲂����Ȃ������甲���Ă��܂�
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
	// �`���l���̃N���[�Y���s���������A���� ssh2_channel_send_close() ���Ăяo���ƁA
	// ���Y�֐����X���b�h�Z�[�t�ł͂Ȃ����߁ASCP����������ɏI�����Ȃ��ꍇ������B
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

		hDlgWnd = TTCreateDialog(hInst, MAKEINTRESOURCEW(IDD_SSHSCP_PROGRESS),
								 pvar->cv->HWin, ssh_scp_dlg_proc);
		if (hDlgWnd != NULL) {
			static const DlgTextInfo text_info[] = {
				{ 0, "DLG_SCP_PROGRESS_TITLE_SENDFILE" },
				{ IDC_SCP_PROGRESS_FILENAME_LABEL, "DLG_SCP_PROGRESS_FILENAME_LABEL" },
				{ IDC_SCP_PROGRESS_BYTE_LABEL, "DLG_SCP_PROGRESS_BYTES_LABEL" },
				{ IDC_SCP_PROGRESS_TIME_LABEL, "DLG_SCP_PROGRESS_TIME_LABEL" },
			};
			SetI18nDlgStrsW(hDlgWnd, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

			c->scp.progress_window = hDlgWnd;
			ShowWindow(hDlgWnd, SW_SHOW);
		}

		thread = (HANDLE)_beginthreadex(NULL, 0, ssh_scp_thread, c, 0, &tid);
		if (thread == 0) {
			// TODO:
			thread = INVALID_HANDLE_VALUE;
		}
		c->scp.thread = thread;


	} else if (c->scp.state == SCP_DATA) {
		// ���M����
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
		// Cancel�{�^�����������ꂽ��E�B���h�E��������B
		if (is_canceled_window(hWnd))
			goto cancel_abort;

		ssh2_scp_get_packetlist(pvar, c, &data, &buflen);
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

	// �`���l���̃N���[�Y���s���������A���� ssh2_channel_send_close() ���Ăяo���ƁA
	// ���Y�֐����X���b�h�Z�[�t�ł͂Ȃ����߁ASCP����������ɏI�����Ȃ��ꍇ������B
	// (2011.6.1 yutaka)
	parm.c = c;
	parm.pvar = pvar;
	SendMessage(hWnd, WM_CHANNEL_CLOSE, (WPARAM)&parm, 0);
	return 0;

cancel_abort:
	pvar->recv.close_request = TRUE;
	return 0;
}

// do_SSH2_adjust_window_size() ��������x���Ԃ��o�߂��Ă���R�[������
// �t���[����A��M�������ĊJ
static void CALLBACK do_SSH2_adjust_window_size_timer(
	HWND hWnd, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
	Channel_t *c = (Channel_t *)nIDEvent;
	PTInstVar pvar = c->scp.pvar;

	(void)hWnd;
	(void)uMsg;
	(void)dwTime;

	if (pvar->recv.data_finished) {
		// ���M�I�������̂Ƀ��b�Z�[�W���c���Ă������΍�
		return;
	}
	if (pvar->recv.timer_id != 0) {
		// SetTimer() �̓C���^�[�o���ɔ�������̂ō폜����
		KillTimer(pvar->cv->HWin, pvar->recv.timer_id);
		pvar->recv.timer_id = 0;
	}

	logprintf(LOG_LEVEL_NOTICE, "%s: SCP receive, send SSH_MSG_CHANNEL_WINDOW_ADJUST", __FUNCTION__);
	pvar->recv.suspended = FALSE;
	do_SSH2_adjust_window_size(pvar, c);
}

// SSH�T�[�o���瑗���Ă����t�@�C���̃f�[�^�����X�g�ɂȂ��B
// ���X�g�̎��o���� ssh_scp_receive_thread �X���b�h�ōs���B
static void ssh2_scp_add_packetlist(PTInstVar pvar, Channel_t *c, unsigned char *buf, unsigned int buflen)
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

	// �L���[�ɋl�񂾃f�[�^�̑��T�C�Y�����Z����B
	c->scp.pktlist_cursize += buflen;

	// �L���[�ɋl�񂾃f�[�^�̑��T�C�Y�����臒l�𒴂����ꍇ�A
	// SSH�T�[�o��windows size�̍X�V���~����
	// ����ɂ�胊�X�g�G���g�������������A��������̔�剻��
	// ����ł���B
	if (c->scp.pktlist_cursize >= SCPRCV_HIGH_WATER_MARK) {
		logprintf(LOG_LEVEL_NOTICE,
			"%s: enter suspend", __FUNCTION__);
		pvar->recv.suspended = TRUE;
	}

	logprintf(LOG_LEVEL_NOTICE,
		"%s: channel=#%d SCP recv %u(bytes) and enqueued.%s",
		__FUNCTION__, c->self_id, c->scp.pktlist_cursize,
		pvar->recv.suspended ? "(suspended)" : ""
	);

error:;
	LeaveCriticalSection(&g_ssh_scp_lock);
}

static void ssh2_scp_get_packetlist(PTInstVar pvar, Channel_t *c, unsigned char **buf, unsigned int *buflen)
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

	// �L���[�ɋl�񂾃f�[�^�̑��T�C�Y�����Z����B
	c->scp.pktlist_cursize -= *buflen;

	// �L���[�ɋl�񂾃f�[�^�̑��T�C�Y������臒l����������ꍇ�A
	// SSH�T�[�o��window size�̍X�V���ĊJ����
	if (c->scp.pktlist_cursize <= SCPRCV_LOW_WATER_MARK) {
		logprintf(LOG_LEVEL_NOTICE, "%s: SCP receive resumed", __FUNCTION__);
		// �u���b�N���Ă���ꍇ
		if (pvar->recv.suspended) {
			// SCP��M�̃u���b�N����������B
			pvar->recv.suspended = FALSE;
			if (c->scp.filercvsize < c->scp.filetotalsize) {
				// ��������M
				pvar->recv.timer_id =
					SetTimer(pvar->cv->HWin, (UINT_PTR)c, USER_TIMER_MINIMUM, do_SSH2_adjust_window_size_timer);
			}
		}
	}

	logprintf(LOG_LEVEL_NOTICE,
		"%s: channel=#%d SCP recv %u(bytes) and dequeued.%s",
		__FUNCTION__, c->self_id, c->scp.pktlist_cursize,
		pvar->recv.suspended ? "(suspended)" : ""
	);

end:;
	LeaveCriticalSection(&g_ssh_scp_lock);
}

static void ssh2_scp_alloc_packetlist(PTInstVar pvar, Channel_t *c)
{
	c->scp.pktlist_head = NULL;
	c->scp.pktlist_tail = NULL;
	InitializeCriticalSection(&g_ssh_scp_lock);
	c->scp.pktlist_cursize = 0;
	pvar->recv.suspended = FALSE;
	pvar->recv.timer_id = 0;
	pvar->recv.close_request = FALSE;
}

static void ssh2_scp_free_packetlist(PTInstVar pvar, Channel_t *c)
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
	if (buflen == 0)
		return FALSE;

	if (c->scp.state == SCP_INIT) {
		if (data[0] == '\01' || data[1] == '\02') {  // error
			return FALSE;
		}

		if (data[0] == 'T') {  // Tmtime.sec mtime.usec atime.sec atime.usec
			DWORD mtime, atime;

			sscanf_s(data, "T%ld 0 %ld 0", &mtime, &atime);

			// �^�C���X�^���v���L�^
			c->scp.filemtime = mtime;
			c->scp.fileatime = atime;

			// ���v���C��Ԃ�
			goto reply;

		} else if (data[0] == 'C') {  // C0666 size file
			HWND hDlgWnd;
			HANDLE thread;
			unsigned int tid;
			int permission;
			long long size;
			char filename[MAX_PATH];

			sscanf_s(data, "C%o %lld %s", &permission, &size, filename, (unsigned int)sizeof(filename));
			logprintf(LOG_LEVEL_NOTICE, "%s: SCP '%s', size=%lld, perm=0x%08x",
					  __FUNCTION__,
					  filename, size, permission);

			// Windows�Ȃ̂Ńp�[�~�b�V�����͖����B�T�C�Y�̂݋L�^�B
			c->scp.filetotalsize = size;
			c->scp.filercvsize = 0;
			c->scp.recv.received_size = 0;

			c->scp.state = SCP_DATA;

			// �i���E�B���h�E
			c->scp.pvar = pvar;
			hDlgWnd = TTCreateDialog(hInst, MAKEINTRESOURCEW(IDD_SSHSCP_PROGRESS),
									 pvar->cv->HWin, ssh_scp_dlg_proc);
			if (hDlgWnd != NULL) {
				static const DlgTextInfo text_info[] = {
					{ 0, "DLG_SCP_PROGRESS_TITLE_RECEIVEFILE" },
					{ IDC_SCP_PROGRESS_FILENAME_LABEL, "DLG_SCP_SENDFILE_FROM" },
					{ IDC_SCP_PROGRESS_BYTE_LABEL, "DLG_SCP_PROGRESS_BYTES_LABEL" },
					{ IDC_SCP_PROGRESS_TIME_LABEL, "DLG_SCP_PROGRESS_TIME_LABEL" },
				};
				SetI18nDlgStrsW(hDlgWnd, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

				c->scp.progress_window = hDlgWnd;
				SetDlgItemTextU8(hDlgWnd, IDC_FILENAME, c->scp.localfilefull);
				ShowWindow(hDlgWnd, SW_SHOW);
			}

			ssh2_scp_alloc_packetlist(pvar, c);
			thread = (HANDLE)_beginthreadex(NULL, 0, ssh_scp_receive_thread, c, 0, &tid);
			if (thread == 0) {
				// TODO:
				thread = INVALID_HANDLE_VALUE;
			}
			c->scp.thread = thread;
			c->scp.thread_id = tid;

			goto reply;

		} else {
			// �T�[�o����̃f�[�^���s��̏ꍇ�́A�G���[�\�����s���B
			// (2014.7.13 yutaka)
			char msg[256];
			int copylen;

			copylen = min(buflen, sizeof(msg));
			memcpy(msg, data, copylen);
			msg[copylen - 1] = 0;

			UTIL_get_lang_msg("MSG_SSH_SCP_ERROR_INIT_TITLE", pvar,
			                  "TTSSH: SCP error (SCP_INIT)");
			MessageBox(NULL, msg, pvar->UIMsg, MB_OK | MB_ICONEXCLAMATION);
		}

	} else if (c->scp.state == SCP_DATA) {  // payload�̎�M
		logprintf(LOG_LEVEL_VERBOSE, "%s: SCP_DATA size=%u",
				  __FUNCTION__,
				  buflen);
		if (pvar->recv.close_request) {
			// �L�����Z���{�^���������ꂽ
			ssh2_channel_send_close(pvar, c);
		}
		else {
			unsigned char *newdata = malloc(buflen);
			if (newdata != NULL) {
				memcpy(newdata, data, buflen);

				// ���̒��� suspended �� TRUE �ɂȂ邱�Ƃ�����
				ssh2_scp_add_packetlist(pvar, c, newdata, buflen);
			}

			c->scp.recv.received_size += buflen;

			if (c->scp.recv.received_size >= c->scp.filetotalsize) {
				// ��M�I��
				PTInstVar pvar = c->scp.pvar;
				pvar->recv.data_finished = TRUE;
				if (pvar->recv.timer_id != 0) {
					pvar->recv.timer_id = 0;
					KillTimer(pvar->cv->HWin, pvar->recv.timer_id);
				}
			}
			else if (pvar->recv.suspended) {
				// �t���[���䒆
				logprintf(LOG_LEVEL_NOTICE, "%s: scp receive suspended", __FUNCTION__);
			}
			else {
				// ���[�J����window size���`�F�b�N
				//	���܂߂ɂ�炸�ɁA������x�܂Ƃ߂Ē������s��
				if (c->local_window < c->local_window_max/2) {
					// window�T�C�Y�𒲐�����
#if 0
					// �����ɒ�������
					//		�����ɃT�[�o�[����f�[�^����M�ł�����̏ꍇ�A
					// 		FD_READ���D�悳��ă��b�Z�[�W�L���[�ɐς܂��
					// 		����windows�̃��b�Z�[�W����(�L�����Z���{�^�������Ȃ�)��
					//		�ł��Ȃ��Ȃ邽�ߎg�p���Ȃ�
					do_ssh2_adjust_window_size(pvar, c);
#else
					// �������Ԃ�u���Ă��璲��
					//		�^�C�}�[���g����GUI�X���b�h�Ŋ֐����R�[������
					if (pvar->recv.timer_id == 0) {
						pvar->recv.timer_id =
							SetTimer(pvar->cv->HWin, (UINT_PTR)c, USER_TIMER_MINIMUM, do_SSH2_adjust_window_size_timer);
					}
#endif
				}
			}
		}

	} else if (c->scp.state == SCP_CLOSING) {  // EOF�̎�M
		ssh2_channel_send_close(pvar, c);

	}

	return TRUE;

reply:
	{
		char ch;
		ch = '\0';
		SSH2_send_channel_data(pvar, c, &ch, 1, 0);
	}
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
		unsigned int max;
		int offset, resp;

		resp = data[0];

		// �G���[�R�[�h�ɂ�蕶����̊i�[�ꏊ���኱�قȂ�B
		if (resp == 1 || /* error, followed by error msg */
			resp == 2) {  /* fatal error, "" */
			offset = 1;
		} else {
			offset = 0;
		}

		if (buflen > sizeof(msg) - 1)
			max = sizeof(msg) - 1;
		else
			max = buflen - offset;
		memcpy(msg, &data[offset], max);
		msg[max] = '\0';

		// �悭������Ȃ��G���[�̏ꍇ�́A���g�Ń`���l�����N���[�Y����B
		// .bashrc ��"stty stop undef"����`����Ă���ƁATTSSH����������ւ̎b�菈�u�B
		// �����錴���͕������Ă��Ȃ��B
		// (2013.4.5 yutaka)
		if (resp == 1) {
			ssh2_channel_send_close(pvar, c);
		} else {
			//ssh2_channel_delete(c);  // free channel
			//ssh2_channel_send_close(pvar, c);
		}
		static const TTMessageBoxInfoW info = {
				"TTSSH",
				"MSG_SSH_SCP_ERROR_TITLE", L"TTSSH: SCP error",
				"MSG_SSH_SCP_ERROR_MISC", L"%s",
				MB_OK | MB_ICONEXCLAMATION
		};
		wchar_t *msgW = ToWcharU8(msg);
		TTMessageBoxW(pvar->cv->HWin, &info, pvar->ts->UILanguageFileW, msgW);
		free(msgW);
	}
}


static BOOL handle_SSH2_channel_data(PTInstVar pvar)
{
	int len;
	char *data;
	int id;
	unsigned int str_len;
	Channel_t *c;

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	// channel number
	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: channel not found. (%d)", __FUNCTION__, id);
		return FALSE;
	}
	if (c->remote_id == SSH_CHANNEL_INVALID) {
		logprintf(LOG_LEVEL_ERROR, "%s: remote shell channel number is unknown.", __FUNCTION__);
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

	// �o�b�t�@�T�C�Y�̃`�F�b�N
	if (str_len > c->local_maxpacket) {
		logprintf(LOG_LEVEL_WARNING, "%s: Data length is larger than local_maxpacket. "
			"len:%d local_maxpacket:%d", __FUNCTION__, str_len, c->local_maxpacket);
	}
	if (str_len > c->local_window) {
		// local window size���傫�ȃp�P�b�g�͎̂Ă�
		logprintf(LOG_LEVEL_WARNING, "%s: Data length is larger than local_window. "
			"len:%d local_window:%d", __FUNCTION__, str_len, c->local_window);
		return FALSE;
	}

	// �y�C���[�h�Ƃ��ăN���C�A���g(Tera Term)�֓n��
	if (c->type == TYPE_SHELL || c->type == TYPE_SUBSYSTEM_GEN) {
		pvar->ssh_state.payload_datalen = str_len;
		pvar->ssh_state.payload_datastart = 8; // id + strlen

	} else if (c->type == TYPE_PORTFWD) {
		//debug_print(0, data, strlen);
		FWD_received_data(pvar, c->local_num, data, str_len);

	} else if (c->type == TYPE_SCP) {  // SCP
		SSH2_scp_response(pvar, c, data, str_len);
		// �E�B���h�E�T�C�Y�̒���
		c->local_window -= str_len;
		return TRUE;

	} else if (c->type == TYPE_SFTP) {  // SFTP
		sftp_response(pvar, c, data, str_len);

	} else if (c->type == TYPE_AGENT) {  // agent forward
		if (!SSH_agent_response(pvar, c, 0, data, str_len)) {
			return FALSE;
		}
	}

	// �E�B���h�E�T�C�Y�̒���
	c->local_window -= str_len;

	do_SSH2_adjust_window_size(pvar, c);

	return TRUE;
}


// Tectia Server �� Windows �ł́ADOS�R�}���h�����s�����Ƃ���stderr�ɏo�͂����
// �G���[���b�Z�[�W�� SSH2_MSG_CHANNEL_EXTENDED_DATA �ő��M���Ă���B
// SSH2_MSG_CHANNEL_EXTENDED_DATA ����������悤�ɂ����B(2006.10.30 maya)
static BOOL handle_SSH2_channel_extended_data(PTInstVar pvar)
{
	int len;
	char *data;
	int id;
	unsigned int strlen;
	Channel_t *c;
	int data_type;

	logputs(LOG_LEVEL_SSHDUMP, "SSH2_MSG_CHANNEL_EXTENDED_DATA was received.");

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	//debug_print(80, data, len);

	// channel number
	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: channel not found. (%d)", __FUNCTION__, id);
		return FALSE;
	}
	if (c->remote_id == SSH_CHANNEL_INVALID) {
		logprintf(LOG_LEVEL_ERROR, "%s: remote shell channel number is unknown.", __FUNCTION__);
		return FALSE;
	}

	// data_type_code
	data_type = get_uint32_MSBfirst(data);
	data += 4;

	// string length
	strlen = get_uint32_MSBfirst(data);
	data += 4;

	// �o�b�t�@�T�C�Y�̃`�F�b�N
	if (strlen > c->local_maxpacket) {
		logprintf(LOG_LEVEL_WARNING, "%s: Data length is larger than local_maxpacket. "
			"len:%d local_maxpacket:%d", __FUNCTION__, strlen, c->local_maxpacket);
	}
	if (strlen > c->local_window) {
		// local window size���傫�ȃp�P�b�g�͎̂Ă�
		logprintf(LOG_LEVEL_WARNING, "%s: Data length is larger than local_window. "
			"len:%d local_window:%d", __FUNCTION__, strlen, c->local_window);
		return FALSE;
	}

	// �y�C���[�h�Ƃ��ăN���C�A���g(Tera Term)�֓n��
	if (c->type == TYPE_SHELL || c->type == TYPE_SUBSYSTEM_GEN) {
		pvar->ssh_state.payload_datalen = strlen;
		pvar->ssh_state.payload_datastart = 12; // id + data_type + strlen

	} else if (c->type == TYPE_PORTFWD) {
		//debug_print(0, data, strlen);
		FWD_received_data(pvar, c->local_num, data, strlen);

	} else if (c->type == TYPE_SCP) {  // SCP
		char *msg = (char *)malloc(strlen+1);
		wchar_t *msgW;
		memcpy(msg, data, strlen);
		msg[strlen] = '\0';
		msgW = ToWcharU8(msg);
		if (msgW) {
			NotifySetIconID(pvar->cv, hInst, pvar->settings.IconID);
			NotifyWarnMessageW(pvar->cv, msgW, L"SSH_MSG_CHANNEL_EXTENDED_DATA");
			NotifySetIconID(pvar->cv, NULL, 0);
			free(msgW);
		}
		free(msg);

	} else if (c->type == TYPE_SFTP) {  // SFTP

	} else if (c->type == TYPE_AGENT) {  // agent forward
		if (!SSH_agent_response(pvar, c, 0, data, strlen)) {
			return FALSE;
		}
	}

	//debug_print(200, data, strlen);

	// �E�B���h�E�T�C�Y�̒���
	c->local_window -= strlen;

	if (c->type == TYPE_SCP && pvar->recv.suspended) {
		logprintf(LOG_LEVEL_NOTICE, "%s: SCP suspended", __FUNCTION__);
	}
	else {
		do_SSH2_adjust_window_size(pvar, c);
	}

	return TRUE;
}


static BOOL handle_SSH2_channel_eof(PTInstVar pvar)
{
	int len;
	char *data;
	int id;
	Channel_t *c;

	// �ؒf���ɃT�[�o�� SSH2_MSG_CHANNEL_EOF �𑗂��Ă���̂ŁA�`���l�����������B(2005.6.19 yutaka)

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	// channel number
	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: channel not found. (%d)", __FUNCTION__, id);
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
		// �ǂ�����̂��������H
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

	logprintf(LOG_LEVEL_VERBOSE, "%s: SSH2_MSG_CHANNEL_OPEN was received.", __FUNCTION__);

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
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

	logprintf(LOG_LEVEL_VERBOSE,
		"%s: type=%s, channel=%d, init_winsize=%d, max_packetsize:%d", __FUNCTION__,
		NonNull(ctype), remote_id, remote_window, remote_maxpacket);

	// check Channel Type(string)
	if (ctype == NULL) {
		// ctype �� NULL �Ŗ������̕ۏ؂ׁ̈A��Ƀ`�F�b�N����
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_get_string returns NULL. (ctype)", __FUNCTION__);
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
			logprintf(LOG_LEVEL_VERBOSE,
				"%s: %s: listen_addr=%s, listen_port=%d, orig_addr=%s, orig_port=%d", __FUNCTION__,
				ctype, listen_addr, listen_port, orig_addr, orig_port);
			// searching request entry by listen_port & create_local_channel
			FWD_open(pvar, remote_id, listen_addr, listen_port, orig_addr, orig_port, &chan_num);

			// channel���A���P�[�g���A�K�v�ȏ��iremote window size�j�������Ŏ���Ă����B
			// changed window size from 128KB to 32KB. (2006.3.6 yutaka)
			// changed window size from 32KB to 128KB. (2007.10.29 maya)
			c = ssh2_channel_new(CHAN_TCP_WINDOW_DEFAULT, CHAN_TCP_PACKET_DEFAULT, TYPE_PORTFWD, chan_num);
			if (c == NULL) {
				// �]���`���l�����ɂ���\�P�b�g�̉���R����C�� (2007.7.26 maya)
				FWD_free_channel(pvar, chan_num);
				UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
				                  "Could not open new channel. TTSSH is already opening too many channels.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
				return FALSE;
			}
			c->remote_id = remote_id;
			c->remote_window = remote_window;
			c->remote_maxpacket = remote_maxpacket;
		}
		else {
			logprintf(LOG_LEVEL_ERROR, "%s: %s: buffer_get_string returns NULL. "
				"linsten_addr=%s, orig_addr=%s", __FUNCTION__,
				ctype, NonNull(listen_addr), NonNull(orig_addr));
		}
		free(listen_addr);
		free(orig_addr);

	} else if (strcmp(ctype, "x11") == 0) { // port-forwarding(X11)
		// X application���^�[�~�i����Ŏ��s����ƁASSH2_MSG_CHANNEL_OPEN �������Ă���B
		char *orig_str;
		int orig_port;

		orig_str = buffer_get_string(&data, NULL);  // "127.0.0.1"
		orig_port = get_uint32_MSBfirst(data);
		data += 4;

		logprintf(LOG_LEVEL_VERBOSE, "%s: %s: orig_addr=%s, orig_port=%d", __FUNCTION__,
			ctype, orig_str, orig_port);

		free(orig_str);

		// X server �֐ڑ�����B
		FWD_X11_open(pvar, remote_id, NULL, 0, &chan_num);

		// channel���A���P�[�g���A�K�v�ȏ��iremote window size�j�������Ŏ���Ă����B
		// changed window size from 128KB to 32KB. (2006.3.6 yutaka)
		// changed window size from 32KB to 128KB. (2007.10.29 maya)
		c = ssh2_channel_new(CHAN_TCP_WINDOW_DEFAULT, CHAN_TCP_PACKET_DEFAULT, TYPE_PORTFWD, chan_num);
		if (c == NULL) {
			// �]���`���l�����ɂ���\�P�b�g�̉���R����C�� (2007.7.26 maya)
			FWD_free_channel(pvar, chan_num);
			UTIL_get_lang_msg("MSG_SSH_NO_FREE_CHANNEL", pvar,
			                  "Could not open new channel. TTSSH is already opening too many channels.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
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
				notify_nonfatal_error(pvar, pvar->UIMsg);
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
				logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
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

			logprintf(LOG_LEVEL_VERBOSE, "%s: SSH2_MSG_CHANNEL_OPEN_FAILURE was sent.", __FUNCTION__ );
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

	// �R�l�N�V�����ؒf���ɁA�p�P�b�g�_���v���t�@�C���֑|���o���B
	if (LOG_LEVEL_SSHDUMP <= pvar->session_settings.LogLevel) {
		save_memdump(LOG_PACKET_DUMP);
		finish_memdump();
	}

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	id = get_uint32_MSBfirst(data);
	data += 4;
	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: channel not found. (%d)", __FUNCTION__, id);
		return FALSE;
	}

	logprintf(LOG_LEVEL_VERBOSE, "SSH2_MSG_CHANNEL_CLOSE was received. local:%d remote:%d", c->self_id, c->remote_id);

	if (c->type == TYPE_SHELL || c->type == TYPE_SUBSYSTEM_GEN) {
		ssh2_channel_send_close(pvar, c);

		// TCP connection closed
		notify_closed_connection(pvar, "disconnected by server request");

	} else if (c->type == TYPE_PORTFWD) {
		// CHANNEL_CLOSE �𑗂�Ԃ��Ȃ��ƃ����[�g��channel���J������Ȃ�
		// c.f. RFC 4254 5.3. Closing a Channel
		ssh2_channel_send_close(pvar, c);

		// �]���`���l�����ɂ���\�P�b�g�̉���R����C�� (2007.7.26 maya)
		FWD_free_channel(pvar, c->local_num);

		// �`���l���̉���R����C�� (2007.4.26 yutaka)
		ssh2_channel_delete(c);

	} else if (c->type == TYPE_SCP) {
		// ��M�I��
		PTInstVar pvar = c->scp.pvar;
		pvar->recv.data_finished = TRUE;

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

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	// ID(4) + string(any) + want_reply(1) + exit status(4)
	id = get_uint32_MSBfirst(data);
	data += 4;
	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: channel not found. (%d)", __FUNCTION__, id);
		return FALSE;
	}
	if (c->remote_id == SSH_CHANNEL_INVALID) {
		logprintf(LOG_LEVEL_ERROR, "%s: remote shell channel number is unknown.", __FUNCTION__);
		return FALSE;
	}

	request = buffer_get_string(&data, NULL);

	want_reply = data[0];
	data += 1;

	logprintf(LOG_LEVEL_VERBOSE,
		"%s: local=%d, remote=%d, request=%s, want_reply=%d",  __FUNCTION__,
		c->self_id, c->remote_id, NonNull(request), want_reply);

	if (request == NULL) {
		// request �� NULL �Ŗ������̕ۏ�
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_get_string returns NULL. (request)", __FUNCTION__);
	}
	else if (strcmp(request, "exit-status") == 0) {
		// �I���R�[�h���܂܂�Ă���Ȃ��
		int estat = get_uint32_MSBfirst(data);
		success = 1;
		logprintf(LOG_LEVEL_VERBOSE, "%s: exit-status=%d", __FUNCTION__, estat);
	}
	else if (strcmp(request, "keepalive@openssh.com") == 0) {
		// �Â� OpenSSH �ł� SUCCESS ��Ԃ��Ă� keepalive ��
		// ���������ƊŘ􂳂�Ȃ��̂� FAILURE ��Ԃ��B[teraterm:1278]
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
			logprintf(LOG_LEVEL_ERROR, "%s: buffer_init returns NULL.", __FUNCTION__);
			return FALSE;
		}
		buffer_put_int(msg, c->remote_id);

		len = buffer_len(msg);
		outmsg = begin_send_packet(pvar, type, len);
		memcpy(outmsg, buffer_ptr(msg), len);
		finish_send_packet(pvar);
		buffer_free(msg);

		if (success) {
			logprintf(LOG_LEVEL_VERBOSE, "%s: SSH2_MSG_CHANNEL_SUCCESS was sent.", __FUNCTION__);
		} else {
			logprintf(LOG_LEVEL_VERBOSE, "%s: SSH2_MSG_CHANNEL_FAILURE was sent.", __FUNCTION__);
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

	// 6byte�i�T�C�Y�{�p�f�B���O�{�^�C�v�j����菜�����ȍ~�̃y�C���[�h
	data = pvar->ssh_state.payload;
	// �p�P�b�g�T�C�Y - (�p�f�B���O�T�C�Y+1)�G�^�̃p�P�b�g�T�C�Y
	len = pvar->ssh_state.payloadlen;

	//debug_print(80, data, len);

	// channel number
	id = get_uint32_MSBfirst(data);
	data += 4;

	c = ssh2_channel_lookup(id);
	if (c == NULL) {
		// channel close���adjust message���x��Ă���Ă���P�[�X�����邽�߁A
		// FALSE�ł͕Ԃ��Ȃ��悤�ɂ���B(2007.12.26 yutaka)
		logprintf(LOG_LEVEL_WARNING, "%s: channel not found. (%d)", __FUNCTION__, id);
		return TRUE;
	}
	if (c->remote_id == SSH_CHANNEL_INVALID) {
		logprintf(LOG_LEVEL_ERROR, "%s: remote shell channel number is unknown.", __FUNCTION__);
		return FALSE;
	}

	adjust = get_uint32_MSBfirst(data);
	data += 4;

	// window size�̒���
	c->remote_window += adjust;

	// ���炸�o�b�t�@�ɕۑ����Ă������f�[�^�𑗂�
	ssh2_channel_retry_send_bufchain(pvar, c);

	return TRUE;
}

// Channel_t ........... SSH2�̃`���l���\����
// local_channel_num ... SSH1�̃��[�J���`���l���ԍ�
static BOOL SSH_agent_response(PTInstVar pvar, Channel_t *c, int local_channel_num,
                               unsigned char *data, unsigned int buflen)
{
	unsigned int req_len;
	FWDChannel *fc;
	buffer_t *agent_msg;
	unsigned int *agent_request_len;
	unsigned char *response = NULL;
	unsigned int resplen = 0;


	// �������ꂽ CHANNEL_DATA �̎�M�ɑΉ� (2008.11.30 maya)
	if (SSHv2(pvar)) {
		agent_msg = c->agent_msg;
		agent_request_len = &c->agent_request_len;
	}
	else {
		fc = pvar->fwd_state.channels + local_channel_num;
		agent_msg = fc->agent_msg;
		agent_request_len = &fc->agent_request_len;
	}

	if (buffer_len(agent_msg) == 0) {
		req_len = get_uint32_MSBfirst(data);
		if (req_len > AGENT_MAX_MSGLEN - 4) {
			logprintf(LOG_LEVEL_NOTICE,
				"%s: Agent Forwarding Error: server request is too large. size=%u, allowd max=%u.",
				__FUNCTION__ , req_len, AGENT_MAX_MSGLEN-4);
			if (pvar->session_settings.ForwardAgentNotify) {
				char title[MAX_UIMSG];
				UTIL_get_lang_msg("MSG_SSH_AGENTERROR_TITLE", pvar, "Bad agent request");
				strncpy_s(title, sizeof(title), pvar->UIMsg, _TRUNCATE);
				UTIL_get_lang_msg("MSG_SSH_AGENTERROR_TOOLARGE", pvar,
					"Agent request size is too large, ignore it.");

				NotifySetIconID(pvar->cv, hInst, pvar->settings.IconID);
				NotifyInfoMessage(pvar->cv, pvar->UIMsg, title);
				NotifySetIconID(pvar->cv, NULL, 0);
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
		if (*agent_request_len > buffer_len(agent_msg)) {
			return TRUE;
		}
		data = buffer_ptr(agent_msg);
	}

	putty_agent_query_synchronous(data, *agent_request_len, (void **)&response, &resplen);
	if (response == NULL || resplen < 5) {
		logprintf(LOG_LEVEL_NOTICE, "%s Agent Forwarding Error: putty_agent_query_synchronous is failed.", __FUNCTION__);
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

	// �g���I������o�b�t�@���N���A
	buffer_clear(agent_msg);
	return TRUE;

error:
	// �G���[���� SSH_AGENT_FAILURE ��Ԃ�
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

	// �g���I������o�b�t�@���N���A
	buffer_clear(agent_msg);
	return TRUE;
}
