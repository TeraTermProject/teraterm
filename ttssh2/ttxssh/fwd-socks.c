/*
 * Copyright (C) 2017 TeraTerm Project
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
#include "fwd.h"
#include "ttcommon.h"

// 4 + 1 + 255 + 2
#define SOCKS_REQUEST_MAXLEN 262

typedef enum {
	SOCKS_STATE_INIT,
	SOCKS_STATE_INITREPLY,
	SOCKS_STATE_AUTHREPLY,
	SOCKS_STATE_REQUESTSENT
} DynamicFwdState;

#define SOCKS4_COMMAND_CONNECT   0x01
#define SOCKS4_COMMAND_BIND      0x02

#define SOCKS4_RESULT_OK         0x5a
#define SOCKS4_RESULT_NG         0x5b
#define SOCKS4_RESULT_NOIDENTD   0x5c
#define SOCKS4_RESULT_IDENTDERR  0x5d

#define SOCKS5_COMMAND_CONNECT   0x01
#define SOCKS5_COMMAND_BIND      0x02
#define SOCKS5_COMMAND_UDP       0x03

#define SOCKS5_AUTH_NONE         0x00
#define SOCKS5_AUTH_GSSAPI       0x01
#define SOCKS5_AUTH_USERPASS     0x02
#define SOCKS5_AUTH_NOACCEPTABLE 0xff

#define SOCKS5_ADDRTYPE_IPV4     0x01
#define SOCKS5_ADDRTYPE_DOMAIN   0x03
#define SOCKS5_ADDRTYPE_IPV6     0x04

#define SOCKS5_OPEN_CONFIRM      128
#define SOCKS5_ERROR_COMMAND     129
#define SOCKS5_ERROR_ADDRTYPE    130

#if defined(__MINGW32__)
#define __FUNCTION__
#endif

typedef struct {
	PTInstVar pvar;

	int channel_num;

	char *peer_name;
	int peer_port;

	DynamicFwdState status;
	unsigned int socks_ver;
	unsigned char request_buf[SOCKS_REQUEST_MAXLEN];
	unsigned int buflen;
} FWDDynamicFilterClosure;

void *SOCKS_init_filter(PTInstVar pvar, int channel_num, char *peer_name, int port)
{
	FWDDynamicFilterClosure *closure = malloc(sizeof(FWDDynamicFilterClosure));

	if (closure == NULL) {
		logputs(LOG_LEVEL_ERROR, __FUNCTION__ ": Can't allocate memory for closure.");
		return NULL;
	}

	closure->pvar = pvar;
	closure->channel_num = channel_num;
	closure->peer_name = _strdup(peer_name);
	closure->peer_port = port;

	closure->status = SOCKS_STATE_INIT;
	closure->socks_ver = 0;
	closure->request_buf[0] = 0;
	closure->buflen = 0;

	return closure;
}

void ClearRemoteConnectFlag(FWDDynamicFilterClosure *closure)
{
	PTInstVar pvar = closure->pvar;
	FWDChannel *c = pvar->fwd_state.channels + closure->channel_num;

	c->status &= ~FWD_REMOTE_CONNECTED;
}

// ダミーのブロッキング書き込み用関数
// 通常は非同期書き込みで処理できるはずなので、ブロッキング書き込みに落ちた時点でエラーとする
static BOOL dummy_blocking_write(PTInstVar pvar, SOCKET s, const char *data, int length)
{
        return FALSE;
}

static int send_socks_reply(FWDDynamicFilterClosure *closure, const char *data, int len)
{
	PTInstVar pvar = closure->pvar;
	FWDChannel *c = pvar->fwd_state.channels + closure->channel_num;

	logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": sending %d bytes.", len);

	return UTIL_sock_buffered_write(pvar, &c->writebuf, dummy_blocking_write, c->local_socket, data, len);
}

send_socks4_reply(FWDDynamicFilterClosure *closure, int code) {
	unsigned char buff[] = {
		0, // NUL
		SOCKS4_RESULT_NG, // status
		0, 0, // field 3
		0, 0, 0, 0 // field 4
	};

	if (code == SOCKS4_RESULT_OK) {
		buff[1] = SOCKS4_RESULT_OK;
	}

	send_socks_reply(closure, buff, sizeof(buff));
}

static void send_socks5_open_success(FWDDynamicFilterClosure *closure)
{
	unsigned char buff[] = {
		5, // version
		0, // status -- success
		0, // reserved
		1, // addr-type (for dummy) -- IPv4
		0, 0, 0, 0, // dummy address -- サーバが BIND Address を通知してくれないので orz
		0, 0 // dummy port number
	};

	send_socks_reply(closure, buff, sizeof(buff));
}

static void send_socks5_open_failure(FWDDynamicFilterClosure *closure, int reason)
{
	unsigned char buff[] = {
		5, // version
		1, // status -- general failure
		0, // reserved
		1, // addr-type (for dummy) -- IPv4
		0, 0, 0, 0, // dummy address -- 接続出来ていないんだから BIND Address も何も無いだろ
		0, 0 // dummy port number
	};

	switch (reason) {
	case 1: // SSH_OPEN_ADMINISTRATIVELY_PROHIBITED
		buff[1] = 0x02; // connection not allowed
		break;
	case 2: // SSH_OPEN_CONNECT_FAILED
		buff[1] = 0x04; // Host unreachable -- 接続出来ない理由は色々あるけれど、とりあえずこれで
		break;
	case 3: // SSH_OPEN_UNKNOWN_CHANNEL_TYPE
		buff[1] = 0x01; // general failure
		break;
	case 4: // SSH_OPEN_RESOURCE_SHORTAGE
		buff[1] = 0x01; // general failure
		break;
	case SOCKS5_ERROR_COMMAND:
		buff[1] = 0x07; // command not supported
		break;
	case SOCKS5_ERROR_ADDRTYPE:
		buff[1] = 0x08; // address not supported
		break;
	}

	send_socks_reply(closure, buff, sizeof(buff));
}

struct socks4_header {
	unsigned char proto;
	unsigned char command;
	unsigned char port[2];
	unsigned char addr[4];
};

int parse_socks4_request(FWDDynamicFilterClosure *closure, unsigned char *buff, unsigned int bufflen)
{
	struct socks4_header s4hdr;
	unsigned char addrbuff[NI_MAXHOST];
	unsigned char pname[NI_MAXSERV];
	unsigned char *user, *addr;
	int port;

	if (bufflen < 8) {
		return 0;
	}

	memcpy_s(&s4hdr, sizeof(s4hdr), buff, 8);

	// CONNECT のみ対応
	if (s4hdr.proto != 4 || s4hdr.command != SOCKS4_COMMAND_CONNECT) {
		send_socks4_reply(closure, SOCKS4_RESULT_NG);
		return -1;
	}

	// skip socks header
	buff += 8;
	bufflen -= 8;

	user = buff;

	while (bufflen > 0 && *buff != 0) {
		bufflen--; buff++;
	}

	if (bufflen == 0) {
		// NUL terminate されてない -> リクエストがまだ途中
		return 0;
	}

	// skip NUL
	buff++;
	bufflen--;

	port = s4hdr.port[0] * 256 + s4hdr.port[1];

	if (s4hdr.addr[0] == 0 && s4hdr.addr[1] == 0 && s4hdr.addr[2] == 0 && s4hdr.addr[3] != 0) {
		// SOCKS4a
		addr = buff;
		while (bufflen > 0 && *buff != 0) {
			bufflen--; buff++;
		}
		if (bufflen == 0) {
			// NUL terminate されてない -> リクエストがまだ途中
			return 0;
		}
	}
	else {  // SOCKS4
		struct sockaddr_in saddr4;

		memset(&saddr4, 0, sizeof(saddr4));
		saddr4.sin_family = AF_INET;
		memcpy_s(&(saddr4.sin_addr), sizeof(saddr4.sin_addr), s4hdr.addr, 4);
		getnameinfo((struct sockaddr *)&saddr4, sizeof(saddr4), addrbuff, sizeof(addrbuff),
			pname, sizeof(pname), NI_NUMERICHOST | NI_NUMERICSERV);

		addr = addrbuff;
	}

	// サーバに要求を送る前に、フラグを本来の状態に戻しておく
	ClearRemoteConnectFlag(closure);

	closure->socks_ver = 4;

	SSH_open_channel(closure->pvar, closure->channel_num, addr, port, closure->peer_name, closure->peer_port);

	closure->status = SOCKS_STATE_REQUESTSENT;
	return 1;
}

int parse_socks5_init_request(FWDDynamicFilterClosure *closure, unsigned char *buff, unsigned int bufflen)
{
	unsigned int authmethod_count;
	unsigned int i;
	unsigned char reply_buff[2] = { 5, SOCKS5_AUTH_NOACCEPTABLE };
	PTInstVar pvar = closure->pvar;
	FWDChannel *channel = pvar->fwd_state.channels + closure->channel_num;

	if (bufflen < 2) {
		return 0;
	}

	if (buff[0] != 5) {
		// protocol version missmatch
		return -1;
	}

	authmethod_count = buff[1];

	if (bufflen < authmethod_count + 2) {
		return 0;
	}

	for (i=0; i<authmethod_count; i++) {
		if (buff[i+2] == SOCKS5_AUTH_NONE) {
			// 現状では認証なしのみサポート
			closure->socks_ver = 5;
			reply_buff[1] = SOCKS5_AUTH_NONE;
			send_socks_reply(closure, reply_buff, 2);

			closure->status = SOCKS_STATE_AUTHREPLY;
			closure->buflen = 0;
			return 1;
		}
	}
	send_socks_reply(closure, reply_buff, 2);
	return -1;
}

struct socks5_header {
	unsigned char proto;
	unsigned char command;
	unsigned char reserved;
	unsigned char addr_type;
	unsigned char addr_len;
};

int parse_socks5_connect_request(FWDDynamicFilterClosure *closure, unsigned char *buff, unsigned int bufflen)
{
	struct socks5_header s5hdr;
	unsigned char addr[NI_MAXHOST];
	unsigned char pname[NI_MAXSERV];
	int port;
	unsigned int reqlen, addrlen;

	if (bufflen < 5) {
		return 0;
	}
	memcpy_s(&s5hdr, sizeof(s5hdr), buff, 5);

	switch (s5hdr.addr_type) {
		case SOCKS5_ADDRTYPE_IPV4:
			addrlen = 4;
			break;
		case SOCKS5_ADDRTYPE_DOMAIN:
			addrlen = s5hdr.addr_len + 1;
			break;
		case SOCKS5_ADDRTYPE_IPV6:
			addrlen = 16;
			break;
		default: // Invalid address type
			send_socks5_open_failure(closure, SOCKS5_ERROR_ADDRTYPE);
			return -1;
	}

	reqlen = 4 + addrlen + 2;

	if (bufflen < reqlen) {
		return 0;
	}

	if (s5hdr.proto != 5 || s5hdr.reserved != 0) {
		return -1;
	}

	if (s5hdr.command != SOCKS5_COMMAND_CONNECT) {
		// CONNECT 以外は未対応
		send_socks5_open_failure(closure, SOCKS5_ERROR_COMMAND);
		return -1;
	}

	switch (s5hdr.addr_type) {
		case SOCKS5_ADDRTYPE_IPV4: {
			struct sockaddr_in saddr4;

			memset(&saddr4, 0, sizeof(saddr4));
			saddr4.sin_family = AF_INET;
			memcpy_s(&(saddr4.sin_addr), sizeof(saddr4.sin_addr), &buff[4], addrlen);
			getnameinfo((struct sockaddr *)&saddr4, sizeof(saddr4), addr, sizeof(addr),
				pname, sizeof(pname), NI_NUMERICHOST | NI_NUMERICSERV);
			break;
		}

		case SOCKS5_ADDRTYPE_DOMAIN:
			if (s5hdr.addr_len > sizeof(addr) - 1 ) {
				return -1;
			}
			memcpy_s(addr, sizeof(addr), &buff[5], s5hdr.addr_len);
			addr[s5hdr.addr_len] = 0;
			break;

		case SOCKS5_ADDRTYPE_IPV6: {
			struct sockaddr_in6 saddr6;

			memset(&saddr6, 0, sizeof(saddr6));
			saddr6.sin6_family = AF_INET6;
			memcpy_s(&(saddr6.sin6_addr), sizeof(saddr6.sin6_addr), &buff[4], addrlen);
			getnameinfo((struct sockaddr *)&saddr6, sizeof(saddr6), addr, sizeof(addr),
				pname, sizeof(pname), NI_NUMERICHOST | NI_NUMERICSERV);
			break;
		}
	}

	port = buff[4 + addrlen] * 256 + buff[4 + addrlen + 1];

	// サーバに要求を送る前に、フラグを本来の状態に戻しておく
	ClearRemoteConnectFlag(closure);

	SSH_open_channel(closure->pvar, closure->channel_num, addr, port, closure->peer_name, closure->peer_port);
	closure->status = SOCKS_STATE_REQUESTSENT;

	return 1;
}

FwdFilterResult parse_client_request(FWDDynamicFilterClosure *closure, int *len, unsigned char **buf)
{
	unsigned char *request = closure->request_buf;
	unsigned int reqlen, newlen;
	int result = 0;

	newlen = closure->buflen + *len;

	if (newlen > SOCKS_REQUEST_MAXLEN || *len < 0) {
		// リクエストが大きすぎる場合は切断する
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__
			": request too large: state=%d, buflen=%d, reqlen=%d",
			closure->status, closure->buflen, *len);
		return FWD_FILTER_CLOSECHANNEL;
	}

	memcpy_s(closure->request_buf + closure->buflen,
		sizeof(closure->request_buf) - closure->buflen,
		*buf, *len);
	closure->buflen = newlen;
	request = closure->request_buf;
	reqlen = closure->buflen;

	// テストなので、後続のデータがあっても無視して全て消す
	**buf = 0; *len = 0;

	switch (closure->status) {
	case SOCKS_STATE_INIT:
		if (request[0] == 4) {
			result = parse_socks4_request(closure, request, reqlen);
		}
		else if (request[0] == 5) {
			result = parse_socks5_init_request(closure, request, reqlen);
		}
		else {
			// Invalid request
			logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": Invalid request. protocol-version=%d", buf[0]);
			result = -1;
		}
		break;
	case SOCKS_STATE_AUTHREPLY:
		if (request[0] == 5) {
			result = parse_socks5_connect_request(closure, request, reqlen);
		}
		else {
			// Invalid request
			logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": Invalid request. protocol-version=%d", buf[0]);
			result = -1;
		}
		break;
	default:
		// NOT REACHED
		break;
	}


	if (result < 0) {
		// フラグを本来の状態(リモート未接続)に戻す
		// これをやっておかないと、未接続のチャネルを閉じようとしておかしくなる
		ClearRemoteConnectFlag(closure);

		return FWD_FILTER_CLOSECHANNEL;
	}

	return FWD_FILTER_RETAIN;
}

FwdFilterResult SOCKS_filter(void *void_closure, FwdFilterEvent event, int *len, unsigned char **buf)
{
	FWDDynamicFilterClosure *closure = (FWDDynamicFilterClosure *)void_closure;

	if (closure == NULL) {
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": closure does not available. event=%d", event);
		return FWD_FILTER_REMOVE;
	}

	switch (event) {
	case FWD_FILTER_CLEANUP:
		// FWD_FILTER_REMOVE を返すと、リソース開放の為にこれで再度呼ばれる
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": closure cleanup. channel=%d", closure->channel_num);
		free(closure->peer_name);
		free(closure);
		return FWD_FILTER_REMOVE;

	case FWD_FILTER_OPENCONFIRM:
		// SSH_open_channel() が成功
		logputs(LOG_LEVEL_VERBOSE, __FUNCTION__ ": OpenConfirmation received");
		if (closure->socks_ver == 4) {
			send_socks4_reply(closure, SOCKS4_RESULT_OK);
		}
		else if (closure->socks_ver == 5) {
			send_socks5_open_success(closure);
		}
		else {
			logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": protocol version missmatch. version=%d", closure->socks_ver);
		}
		return FWD_FILTER_REMOVE;

	case FWD_FILTER_OPENFAILURE:
		// SSH_open_channel() が失敗
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": Open Failure. reason=%d", *len);
		if (closure->socks_ver == 4) {
			send_socks4_reply(closure, SOCKS4_RESULT_NG);
		}
		else if (closure->socks_ver == 5) {
			send_socks5_open_failure(closure, *len);
		}
		else {
			logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": protocol version missmatch. version=%d", closure->socks_ver);
		}
		return FWD_FILTER_CLOSECHANNEL;

	case FWD_FILTER_FROM_SERVER:
		// このフィルタが有効な時点ではサーバへのチャネルは開いていないので
		// ここにはこないはず
		logputs(LOG_LEVEL_VERBOSE, __FUNCTION__ ": data received from server. (bug?)");
		return FWD_FILTER_RETAIN;

	case FWD_FILTER_FROM_CLIENT:
		// クライアントからの要求を処理する
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": data received from client. size=%d", *len);
		return parse_client_request(closure, len, buf);
	}

	// NOT REACHED
	return FWD_FILTER_RETAIN;
}
