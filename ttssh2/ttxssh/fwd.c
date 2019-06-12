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

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#include "ttxssh.h"
#include "x11util.h"
#include "fwd.h"
#include "fwd-socks.h"
#include "ttcommon.h"

#include <assert.h>
#include "WSAAsyncGetAddrInfo.h"

#define WM_SOCK_ACCEPT (WM_APP+9999)
#define WM_SOCK_IO     (WM_APP+9998)
#define WM_SOCK_GOTNAME (WM_APP+9997)

#define CHANNEL_READ_BUF_SIZE 8192

static LRESULT CALLBACK accept_wnd_proc(HWND wnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam);

static int find_request_num(PTInstVar pvar, SOCKET s)
{
	int i;
	int j;

	if (s == INVALID_SOCKET)
		return -1;

	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		for (j = 0; j < pvar->fwd_state.requests[i].num_listening_sockets;
			 ++j) {
			if ((pvar->fwd_state.requests[i].status & FWD_DELETED) == 0
				&& pvar->fwd_state.requests[i].listening_sockets[j] == s) {
				return i;
			}
		}
	}

	return -1;
}

static int find_channel_num(PTInstVar pvar, SOCKET s)
{
	int i;

	if (s == INVALID_SOCKET)
		return -1;

	for (i = 0; i < pvar->fwd_state.num_channels; i++) {
		if (pvar->fwd_state.channels[i].local_socket == s) {
			return i;
		}
	}

	return -1;
}

static int find_request_num_from_async_request(PTInstVar pvar,
                                               HANDLE request)
{
	int i;

	if (request == 0)
		return -1;

	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		if ((pvar->fwd_state.requests[i].status & FWD_DELETED) == 0
		 && pvar->fwd_state.requests[i].to_host_lookup_handle == request) {
			return i;
		}
	}

	return -1;
}

static int find_listening_socket_num(PTInstVar pvar, int request_num,
                                     SOCKET s)
{
	FWDRequest *request = pvar->fwd_state.requests + request_num;
	int i;

	for (i = 0; i < request->num_listening_sockets; ++i)
		if (request->listening_sockets[i] == s)
			return i;

	/* not found */
	return -1;
}

static void drain_matching_messages(HWND wnd, UINT msg, WPARAM wParam)
{
	MSG m;
	MSG *buf;
	int buf_len;
	int buf_size;
	int i;

	/* fast path for case where there are no suitable messages */
	if (!PeekMessage(&m, wnd, msg, msg, PM_NOREMOVE)) {
		return;
	}

	/* suck out all the messages */
	buf_size = 1;
	buf_len = 0;
	buf = (MSG *) malloc(sizeof(MSG) * buf_size);

	while (PeekMessage(&m, wnd, msg, msg, PM_REMOVE)) {
		if (buf_len == buf_size) {
			buf_size *= 2;
			buf = (MSG *) realloc(buf, sizeof(MSG) * buf_size);
		}

		buf[buf_len] = m;
		buf_len++;
	}

	for (i = 0; i < buf_len; i++) {
		if (buf[i].wParam != wParam) {
			PostMessage(wnd, msg, buf[i].wParam, buf[i].lParam);
		}
	}

	free(buf);
}

/* hideous hack to fix broken Winsock API.
   After we close a socket further WM_ messages may arrive for it, according to
   the docs for WSAAsyncSelect. This sucks because we may allocate a new socket
   before these messages are processed, which may get the same handle value as
   the old socket, which may mean that we get confused and try to process the old
   message according to the new request or channel, which could cause chaos and
   possibly even security problems.
   "Solution": scan the accept_wnd message queue whenever we close a socket and
   remove all pending messages for that socket. Possibly this might not even be
   enough if there is a system thread that is in the process of posting
   a new message while during execution of the closesocket. I'm hoping
   that those actions are serialized...
*/
static void safe_closesocket(PTInstVar pvar, SOCKET s)
{
	HWND wnd = pvar->fwd_state.accept_wnd;

	closesocket(s);
	if (wnd != NULL) {
		drain_matching_messages(wnd, WM_SOCK_ACCEPT, (WPARAM) s);
		drain_matching_messages(wnd, WM_SOCK_IO, (WPARAM) s);
	}
}

static void safe_WSACancelAsyncRequest(PTInstVar pvar, HANDLE request)
{
	HWND wnd = pvar->fwd_state.accept_wnd;

	WSACancelAsyncRequest(request);
	if (wnd != NULL) {
		drain_matching_messages(wnd, WM_SOCK_GOTNAME, (WPARAM) request);
	}
}

int FWD_check_local_channel_num(PTInstVar pvar, int local_num)
{
	if (local_num < 0 || local_num >= pvar->fwd_state.num_channels
	 || pvar->fwd_state.channels[local_num].status == 0) {
		UTIL_get_lang_msg("MSG_FWD_LOCAL_CHANNEL_ERROR", pvar,
		                  "The server attempted to manipulate a forwarding channel that does not exist.\n"
		                  "Either the server has a bug or is hostile. You should close this connection.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		return 0;
	} else {
		return 1;
	}
}

static void request_error(PTInstVar pvar, int request_num, int err)
{
	SOCKET *s =
		pvar->fwd_state.requests[request_num].listening_sockets;
	int i;

	for (i = 0;
	     i < pvar->fwd_state.requests[request_num].num_listening_sockets;
	     ++i) {
		if (s[i] != INVALID_SOCKET) {
			safe_closesocket(pvar, s[i]);
			pvar->fwd_state.requests[request_num].listening_sockets[i] =
				INVALID_SOCKET;
		}
	}

	UTIL_get_lang_msg("MSG_FWD_REQUEST_ERROR", pvar,
	                  "Communications error while listening for a connection to forward.\n"
	                  "The listening port will be terminated.");
	notify_nonfatal_error(pvar, pvar->ts->UIMsg);
}

static void send_local_connection_closure(PTInstVar pvar, int channel_num)
{
	FWDChannel *channel = pvar->fwd_state.channels + channel_num;

	if ((channel->status & FWD_BOTH_CONNECTED) == FWD_BOTH_CONNECTED) {
		SSH_channel_input_eof(pvar, channel->remote_num, channel_num);
		SSH_channel_output_eof(pvar, channel->remote_num);
		channel->status |= FWD_CLOSED_LOCAL_IN | FWD_CLOSED_LOCAL_OUT;
	}
}

static void closed_local_connection(PTInstVar pvar, int channel_num)
{
	FWDChannel *channel = pvar->fwd_state.channels + channel_num;

	if (channel->local_socket != INVALID_SOCKET) {
		safe_closesocket(pvar, channel->local_socket);
		channel->local_socket = INVALID_SOCKET;

		send_local_connection_closure(pvar, channel_num);
	}
}

/* This gets called when a request becomes both deleted and has no
   active channels. */
static void really_delete_request(PTInstVar pvar, int request_num)
{
	FWDRequest *request = pvar->fwd_state.requests + request_num;

	if (request->to_host_lookup_handle != 0) {
		safe_WSACancelAsyncRequest(pvar, request->to_host_lookup_handle);
		request->to_host_lookup_handle = 0;
	}

  /*****/
	/* freeaddrinfo(); */
}

void FWD_free_channel(PTInstVar pvar, uint32 local_channel_num)
{
	FWDChannel *channel = &pvar->fwd_state.channels[local_channel_num];

	if (channel->type == TYPE_AGENT) { // TYPE_AGENT でここに来るのは SSH1 のみ
		buffer_free(channel->agent_msg);
		// channel_close 時と TTSSH 終了時に2回呼ばれるので、二重 free 防止のため
		channel->agent_msg = NULL;
		channel->status = 0;
	}
	else { // TYPE_PORTFWD
		UTIL_destroy_sock_write_buf(&channel->writebuf);
		if (channel->filter != NULL) {
			channel->filter(channel->filter_closure, FWD_FILTER_CLEANUP, NULL, NULL);
			channel->filter = NULL;
			channel->filter_closure = NULL;
		}
		channel->status = 0;
		if (channel->local_socket != INVALID_SOCKET) {
			safe_closesocket(pvar, channel->local_socket);
			channel->local_socket = INVALID_SOCKET;
		}
	}

	if (channel->request_num >= 0) {
		FWDRequest *request =
			&pvar->fwd_state.requests[channel->request_num];

		request->num_channels--;
		if (request->num_channels == 0
		 && (request->status & FWD_DELETED) != 0) {
			really_delete_request(pvar, channel->request_num);
		}
		channel->request_num = -1;
	}
}

void FWD_channel_input_eof(PTInstVar pvar, uint32 local_channel_num)
{
	FWDChannel *channel;

	if (!FWD_check_local_channel_num(pvar, local_channel_num))
		return;

	channel = pvar->fwd_state.channels + local_channel_num;

	if (channel->local_socket != INVALID_SOCKET) {
		shutdown(channel->local_socket, 1);
	}
	channel->status |= FWD_CLOSED_REMOTE_IN;
	if ((channel->status & FWD_CLOSED_REMOTE_OUT) == FWD_CLOSED_REMOTE_OUT) {
		closed_local_connection(pvar, local_channel_num);
		FWD_free_channel(pvar, local_channel_num);
	}
}

void FWD_channel_output_eof(PTInstVar pvar, uint32 local_channel_num)
{
	FWDChannel *channel;

	if (!FWD_check_local_channel_num(pvar, local_channel_num))
		return;

	channel = pvar->fwd_state.channels + local_channel_num;

	if (channel->local_socket != INVALID_SOCKET) {
		shutdown(channel->local_socket, 0);
	}
	channel->status |= FWD_CLOSED_REMOTE_OUT;
	if ((channel->status & FWD_CLOSED_REMOTE_IN) == FWD_CLOSED_REMOTE_IN) {
		closed_local_connection(pvar, local_channel_num);
		FWD_free_channel(pvar, local_channel_num);
	}
}

static char *describe_socket_error(PTInstVar pvar, int code)
{
	switch (code) {
	case WSAECONNREFUSED:
		UTIL_get_lang_msg("MSG_FWD_REFUSED_ERROR", pvar,
		                  "Connection refused (perhaps the service is not currently running)");
		return pvar->ts->UIMsg;
	case WSAENETDOWN:
	case WSAENETUNREACH:
	case WSAEHOSTUNREACH:
		UTIL_get_lang_msg("MSG_FWD_NETDOWN_ERROR", pvar,
		                  "The machine could not be contacted (possibly a network problem)");
		return pvar->ts->UIMsg;
	case WSAETIMEDOUT:
	case WSAEHOSTDOWN:
		UTIL_get_lang_msg("MSG_FWD_MACHINEDOWN_ERROR", pvar,
		                  "The machine could not be contacted (possibly the machine is down)");
		return pvar->ts->UIMsg;
	case WSATRY_AGAIN:
	case WSANO_RECOVERY:
	case WSANO_ADDRESS:
	case WSAHOST_NOT_FOUND:
		UTIL_get_lang_msg("MSG_FWD_ADDRNOTFOUND_ERROR", pvar,
		                  "No address was found for the machine");
		return pvar->ts->UIMsg;
	default:
		UTIL_get_lang_msg("MSG_FWD_CONNECT_ERROR", pvar,
		                  "The forwarding connection could not be established");
		return pvar->ts->UIMsg;
	}
}

static void channel_error(PTInstVar pvar, char *action,
                          int channel_num, int err)
{
	char *err_msg;
	char uimsg[MAX_UIMSG];

	closed_local_connection(pvar, channel_num);

	switch (err) {
	case WSAECONNRESET:
	case WSAECONNABORTED:
		err_msg = NULL;
		break;
	default:
		strncpy_s(uimsg, sizeof(uimsg), describe_socket_error(pvar, err), _TRUNCATE);
		err_msg = uimsg;
	}

	if (err_msg != NULL) {
		char buf[1024];

		UTIL_get_lang_msg("MSG_FWD_CHANNEL_ERROR", pvar,
		                  "Communications error %s forwarded local %s.\n"
		                  "%s (code %d).\n"
		                  "The forwarded connection will be closed.");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->ts->UIMsg, action,
		            pvar->fwd_state.requests[pvar->fwd_state.channels[channel_num].
		                                     request_num].spec.from_port_name,
		            err_msg, err);
		notify_nonfatal_error(pvar, buf);
	}
}

static void channel_opening_error(PTInstVar pvar, int channel_num, int err)
{
	char buf[1024];
	FWDChannel *channel = &pvar->fwd_state.channels[channel_num];
	FWDRequest *request =
		&pvar->fwd_state.requests[channel->request_num];
	char uimsg[MAX_UIMSG];

	SSH_fail_channel_open(pvar, channel->remote_num);
	strncpy_s(uimsg, sizeof(uimsg), describe_socket_error(pvar, err), _TRUNCATE);
	if (request->spec.type == FWD_REMOTE_X11_TO_LOCAL) {
		UTIL_get_lang_msg("MSG_FWD_CHANNEL_OPEN_X_ERROR", pvar,
		                  "The server attempted to forward a connection through this machine.\n"
		                  "It requested a connection to the X server on %s (display %d:%d).\n"
		                  "%s(code %d).\n" "The forwarded connection will be closed.");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->ts->UIMsg,
		            request->spec.to_host, request->spec.to_port - 6000, request->spec.x11_screen,
		            uimsg, err);
	} else {
		UTIL_get_lang_msg("MSG_FWD_CHANNEL_OPEN_ERROR", pvar,
		                  "The server attempted to forward a connection through this machine.\n"
		                  "It requested a connection to %s (port %s).\n" "%s(code %d).\n"
		                  "The forwarded connection will be closed.");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->ts->UIMsg,
		            request->spec.to_host, request->spec.to_port_name,
		            uimsg, err);
	}
	notify_nonfatal_error(pvar, buf);
	FWD_free_channel(pvar, channel_num);
}

static int alloc_channel(PTInstVar pvar, int new_status,
                         int new_request_num)
{
	int i;
	int new_num_channels;
	int new_channel = -1;
	FWDChannel *channel;

	for (i = 0; i < pvar->fwd_state.num_channels && new_channel < 0; i++) {
		if (pvar->fwd_state.channels[i].status == 0) {
			new_channel = i;
		}
	}

	if (new_channel < 0) {
		new_num_channels = pvar->fwd_state.num_channels + 1;
		pvar->fwd_state.channels =
			(FWDChannel *) realloc(pvar->fwd_state.channels,
			                           sizeof(FWDChannel) *
			                           new_num_channels);

		for (; i < new_num_channels; i++) {
			channel = pvar->fwd_state.channels + i;

			channel->status = 0;
			channel->local_socket = INVALID_SOCKET;
			channel->request_num = -1;
			channel->filter = NULL;
			channel->filter_closure = NULL;
			UTIL_init_sock_write_buf(&channel->writebuf);
		}

		new_channel = pvar->fwd_state.num_channels;
		pvar->fwd_state.num_channels = new_num_channels;
	}

	channel = pvar->fwd_state.channels + new_channel;

	channel->status = new_status;
	channel->request_num = new_request_num;
	pvar->fwd_state.requests[new_request_num].num_channels++;
	UTIL_init_sock_write_buf(&channel->writebuf);

	return new_channel;
}

static int alloc_agent_channel(PTInstVar pvar, int remote_channel_num)
{
	int i;
	int new_num_channels;
	int new_channel = -1;
	FWDChannel *channel;

	for (i = 0; i < pvar->fwd_state.num_channels && new_channel < 0; i++) {
		if (pvar->fwd_state.channels[i].status == 0) {
			new_channel = i;
		}
	}

	if (new_channel < 0) {
		new_num_channels = pvar->fwd_state.num_channels + 1;
		pvar->fwd_state.channels =
			(FWDChannel *) realloc(pvar->fwd_state.channels,
			                           sizeof(FWDChannel) *
			                           new_num_channels);

		new_channel = pvar->fwd_state.num_channels;
		pvar->fwd_state.num_channels = new_num_channels;
	}

	channel = pvar->fwd_state.channels + new_channel;
	channel->status = FWD_AGENT_DUMMY;
	channel->remote_num = remote_channel_num;
	channel->request_num = -1;
	channel->type = TYPE_AGENT;
	channel->agent_msg = buffer_init();
	channel->agent_request_len = 0;

	return new_channel;
}

static HWND make_accept_wnd(PTInstVar pvar)
{
	if (pvar->fwd_state.accept_wnd == NULL) {
		UTIL_get_lang_msg("DLG_FWDMON_TITLE", pvar, "TTSSH Port Forwarding Monitor");
		pvar->fwd_state.accept_wnd =
			CreateWindow("STATIC", pvar->ts->UIMsg,
			             WS_DISABLED | WS_POPUP, 0, 0, 1, 1, NULL, NULL,
			             hInst, NULL);
		if (pvar->fwd_state.accept_wnd != NULL) {
			pvar->fwd_state.old_accept_wnd_proc =
				(WNDPROC) SetWindowLong(pvar->fwd_state.accept_wnd,
				                        GWL_WNDPROC,
				                        (LONG) accept_wnd_proc);
			SetWindowLong(pvar->fwd_state.accept_wnd, GWL_USERDATA,
			              (LONG) pvar);
		}
	}

	return pvar->fwd_state.accept_wnd;
}

static void connected_local_connection(PTInstVar pvar, int channel_num)
{
	SSH_confirm_channel_open(pvar,
	                         pvar->fwd_state.channels[channel_num].
	                         remote_num, channel_num);
	pvar->fwd_state.channels[channel_num].status |= FWD_LOCAL_CONNECTED;
}

static void make_local_connection(PTInstVar pvar, int channel_num)
{
	FWDChannel *channel = pvar->fwd_state.channels + channel_num;
	FWDRequest *request =
		pvar->fwd_state.requests + channel->request_num;

	for (channel->to_host_addrs = request->to_host_addrs;
	     channel->to_host_addrs;
	     channel->to_host_addrs = channel->to_host_addrs->ai_next) {
		channel->local_socket = socket(channel->to_host_addrs->ai_family,
		                               channel->to_host_addrs->ai_socktype,
		                               channel->to_host_addrs->ai_protocol);
		if (channel->local_socket == INVALID_SOCKET)
			continue;
		if (WSAAsyncSelect
		    (channel->local_socket, make_accept_wnd(pvar), WM_SOCK_IO,
		     FD_CONNECT | FD_READ | FD_CLOSE | FD_WRITE) == SOCKET_ERROR) {
			closesocket(channel->local_socket);
			channel->local_socket = INVALID_SOCKET;
			continue;
		}
		if (connect(channel->local_socket,
		            channel->to_host_addrs->ai_addr,
		            channel->to_host_addrs->ai_addrlen) != SOCKET_ERROR) {
			connected_local_connection(pvar, channel_num);
			return;
		} else if (WSAGetLastError() == WSAEWOULDBLOCK) {
			/* do nothing, we'll just wait */
			return;
		} else {
			/* connect() failed */
			closesocket(channel->local_socket);
			channel->local_socket = INVALID_SOCKET;
			continue;
		}
	}

	channel_opening_error(pvar, channel_num, WSAGetLastError());
}

static char *dump_fwdchannel(FWDChannel *c)
{
	static char buff[1024];

	_snprintf_s(buff, sizeof(buff), _TRUNCATE,
		"status=%d, channel: local=%d, remote=%d, socket: %s, filter: %s",
		c->status, c->request_num, c->remote_num, (c->local_socket != INVALID_SOCKET) ? "ok" : "invalid",
		(c->filter != NULL) ? "on" : "off");

	return buff;
}

static void accept_local_connection(PTInstVar pvar, int request_num, int listening_socket_num)
{
	int channel_num;
	SOCKET s;
	struct sockaddr_storage addr;
	char hname[NI_MAXHOST];
	char strport[NI_MAXSERV]; // ws2tcpip.h
	int addrlen = sizeof(addr);
	int port;
	FWDChannel *channel;
	FWDRequest *request = &pvar->fwd_state.requests[request_num];
	BOOL is_localhost = FALSE;

	s = accept(request->listening_sockets[listening_socket_num], (struct sockaddr *) &addr, &addrlen);
	if (s == INVALID_SOCKET)
		return;

	// SSH2 port-forwardingに接続元のリモートポートが必要。(2005.2.27 yutaka)
	if (getnameinfo((struct sockaddr *) &addr, addrlen, hname, sizeof(hname),
	                strport, sizeof(strport), NI_NUMERICHOST | NI_NUMERICSERV)) {
		/* NOT REACHED */
	}
	port = atoi(strport);

	channel_num = alloc_channel(pvar, FWD_LOCAL_CONNECTED, request_num);
	channel = pvar->fwd_state.channels + channel_num;

	channel->local_socket = s;

	if (request->spec.type == FWD_LOCAL_TO_REMOTE) {
		logprintf(LOG_LEVEL_VERBOSE,
		          "%s: Host %s(%d) connecting to port %d; forwarding to %s:%d; type=LtoR", __FUNCTION__,
		          hname, port, request->spec.from_port, request->spec.to_host, request->spec.to_port);

		channel->filter_closure = NULL;
		channel->filter = NULL;
		SSH_open_channel(pvar, channel_num, request->spec.to_host,
		                 request->spec.to_port, hname, port);
	}
	else { // FWD_LOCAL_DYNAMIC
		logprintf(LOG_LEVEL_VERBOSE,
		          "%s: Host %s(%d) connecting to port %d; type=dynamic",
				  __FUNCTION__, hname, port, request->spec.from_port);

		// SOCKS のリクエストを処理する為の filter を登録
		channel->filter_closure = SOCKS_init_filter(pvar, channel_num, hname, port);
		channel->filter = SOCKS_filter;

		// リモート側はまだ繋がっていないが、read_local_connection() 等で処理が行われるようにフラグを立てる。
		channel->status |= FWD_BOTH_CONNECTED;

	}
	logprintf(150, "%s: channel info: %s", __FUNCTION__, dump_fwdchannel(channel));
}

static void write_local_connection_buffer(PTInstVar pvar, int channel_num)
{
	FWDChannel *channel = pvar->fwd_state.channels + channel_num;

	if ((channel->status & FWD_BOTH_CONNECTED) == FWD_BOTH_CONNECTED) {
		if (!UTIL_sock_write_more
			(pvar, &channel->writebuf, channel->local_socket)) {
			channel_error(pvar, "writing", channel_num, WSAGetLastError());
		}
	}
}

static void read_local_connection(PTInstVar pvar, int channel_num)
{
	FWDChannel *channel = pvar->fwd_state.channels + channel_num;

	logprintf(LOG_LEVEL_VERBOSE, "%s: channel=%d", __FUNCTION__, channel_num);

	if ((channel->status & FWD_BOTH_CONNECTED) != FWD_BOTH_CONNECTED) {
		return;
	}

	while (channel->local_socket != INVALID_SOCKET) {
		char buf[CHANNEL_READ_BUF_SIZE];
		int amount;
		int err;

		// recvの一時停止中ならば、何もせずに戻る。
		if (SSHv2(pvar)) {
			Channel_t* c = ssh2_local_channel_lookup(channel_num);
			if (c->bufchain_recv_suspended) {
				logprintf(LOG_LEVEL_NOTICE, "%s: channel=%d recv was skipped for flow control",
					__FUNCTION__, channel_num);
				return;
			}
		}

		amount = recv(channel->local_socket, buf, sizeof(buf), 0);

		// Xサーバからのデータ受信があれば、ノンブロッキングモードでソケット受信を行い、
		// SSHサーバのXアプリケーションへ送信する。
		//OutputDebugPrintf("%s: recv %d\n", __FUNCTION__, amount);

		if (amount > 0) {
			char *new_buf = buf;
			FwdFilterResult action = FWD_FILTER_RETAIN;

			if (channel->filter != NULL) {
				action = channel->filter(channel->filter_closure, FWD_FILTER_FROM_CLIENT, &amount, &new_buf);
			}

			if (amount > 0 && (channel->status & FWD_CLOSED_REMOTE_OUT) == 0) {
				// ポートフォワーディングにおいてクライアントからの送信要求を、SSH通信に乗せてサーバまで送り届ける。
				SSH_channel_send(pvar, channel_num, channel->remote_num, new_buf, amount, 0);
			}

			switch (action) {
			case FWD_FILTER_REMOVE:
				channel->filter(channel->filter_closure, FWD_FILTER_CLEANUP, NULL, NULL);
				channel->filter = NULL;
				channel->filter_closure = NULL;
				break;
			case FWD_FILTER_CLOSECHANNEL:
				closed_local_connection(pvar, channel_num);
				break;
			}
		} else if (amount == 0 || (err = WSAGetLastError()) == WSAEWOULDBLOCK) {
			return;
		} else {
			channel_error(pvar, "reading", channel_num, err);
			return;
		}
	}
}

static void failed_to_host_addr(PTInstVar pvar, int request_num, int err)
{
	int i;

	for (i = 0; i < pvar->fwd_state.num_channels; i++) {
		if (pvar->fwd_state.channels[i].request_num == request_num) {
			channel_opening_error(pvar, i, err);
		}
	}
}

static void found_to_host_addr(PTInstVar pvar, int request_num)
{
	int i;

	for (i = 0; i < pvar->fwd_state.num_channels; i++) {
		if (pvar->fwd_state.channels[i].request_num == request_num) {
			make_local_connection(pvar, i);
		}
	}
}

// local connectionの受信の停止および再開の判断を行う
// 
// notify: TRUE    recvを再開する
//         FALSE   recvを停止する
//
// [目的]
// remote_windowに空きがない場合は通知オフとし、空きができた場合は
// 通知を再開する。
// remote_windowに余裕がない状態で、local connectionからのパケットを
// 受信し続けると、消費メモリが肥大化する(厳密にはメモリリークではない)
// という問題を回避する。
//
// (2019.6.5 yutaka)
void FWD_suspend_resume_local_connection(PTInstVar pvar, Channel_t* c, int notify)
{
	int channel_num;
	FWDChannel* channel;
	int changed = 0;

	channel_num = c->local_num;
	channel = pvar->fwd_state.channels + channel_num;

	if (notify) {
		// recvを再開するか判断する
		if (c->bufchain_amount <= FWD_LOW_WATER_MARK) {
			// 下限を下回ったので再開
			c->bufchain_recv_suspended = FALSE;

			// ここで再開のメッセージを飛ばす
			PostMessage(pvar->fwd_state.accept_wnd, WM_SOCK_IO, 
				(WPARAM)channel->local_socket,
				MAKEWPARAM(FD_READ, 0)
				);

			changed = 1;
		}

	} else {
		// recvを停止するか判断する
		if (c->bufchain_amount >= FWD_HIGH_WATER_MARK) {
			// 上限を超えたので停止
			c->bufchain_recv_suspended = TRUE;
			changed = 1;
		}
	}

	logprintf(LOG_LEVEL_NOTICE, 
		"%s: Local channel#%d recv has been `%s' for flow control(buffer size %lu, recv %s).",
		__FUNCTION__, channel_num, 
		c->bufchain_recv_suspended ? "disabled" : "enabled",
		c->bufchain_amount,
		changed ? "changed" : ""
		);

}


static LRESULT CALLBACK accept_wnd_proc(HWND wnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam)
{
	PTInstVar pvar = (PTInstVar) GetWindowLong(wnd, GWL_USERDATA);

	if (msg == WM_SOCK_ACCEPT &&
	    (LOWORD(lParam) == FD_READ || LOWORD(lParam) == FD_CLOSE ||
	     LOWORD(lParam) == FD_WRITE)) {
		msg = WM_SOCK_IO;
	}

	switch (msg) {
	case WM_SOCK_ACCEPT:{
			int request_num = find_request_num(pvar, (SOCKET) wParam);

			if (request_num < 0)
				return TRUE;

			if (HIWORD(lParam) != 0) {
				request_error(pvar, request_num, HIWORD(lParam));
			} else {
				int listening_socket_num;
				switch (LOWORD(lParam)) {
				case FD_ACCEPT:
					listening_socket_num =
						find_listening_socket_num(pvar, request_num,
						                          (SOCKET) wParam);
					if (listening_socket_num == -1)
						return FALSE;
					accept_local_connection(pvar, request_num,
					                        listening_socket_num);
					break;
				}
			}
			return TRUE;
		}

	case WM_SOCK_GOTNAME:{
			int request_num =
				find_request_num_from_async_request(pvar, (HANDLE) wParam);

			if (request_num < 0)
				return TRUE;

			if (HIWORD(lParam) != 0) {
				failed_to_host_addr(pvar, request_num, HIWORD(lParam));
			} else {
				found_to_host_addr(pvar, request_num);
			}
			pvar->fwd_state.requests[request_num].to_host_lookup_handle = 0;
			return TRUE;
		}

	case WM_SOCK_IO:{
			int channel_num = find_channel_num(pvar, (SOCKET) wParam);
			FWDChannel *channel =
				pvar->fwd_state.channels + channel_num;

			if (channel_num < 0)
				return TRUE;

			if (HIWORD(lParam) != 0) {
				if (LOWORD(lParam) == FD_CONNECT) {
					if (channel->to_host_addrs->ai_next == NULL) {
						/* all protocols were failed */
						channel_opening_error(pvar, channel_num,
											  HIWORD(lParam));
					} else {
						for (channel->to_host_addrs =
							 channel->to_host_addrs->ai_next;
							 channel->to_host_addrs;
							 channel->to_host_addrs =
							 channel->to_host_addrs->ai_next) {
							channel->local_socket =
								socket(channel->to_host_addrs->ai_family,
								       channel->to_host_addrs->ai_socktype,
								       channel->to_host_addrs->ai_protocol);
							if (channel->local_socket == INVALID_SOCKET)
								continue;
							if (WSAAsyncSelect
							    (channel->local_socket,
							     make_accept_wnd(pvar), WM_SOCK_IO,
							     FD_CONNECT | FD_READ | FD_CLOSE | FD_WRITE
							    ) == SOCKET_ERROR) {
								closesocket(channel->local_socket);
								channel->local_socket = INVALID_SOCKET;
								continue;
							}
							if (connect(channel->local_socket,
							            channel->to_host_addrs->ai_addr,
							            channel->to_host_addrs->ai_addrlen
							           ) != SOCKET_ERROR) {
								connected_local_connection(pvar,
								                           channel_num);
								return TRUE;
							} else if (WSAGetLastError() == WSAEWOULDBLOCK) {
								/* do nothing, we'll just wait */
								return TRUE;
							} else {
								closesocket(channel->local_socket);
								channel->local_socket = INVALID_SOCKET;
								continue;
							}
						}
						channel_opening_error(pvar, channel_num,
						                      HIWORD(lParam));
						return TRUE;
					}
				} else {
					channel_error(pvar, "accessing", channel_num,
					              HIWORD(lParam));
					return TRUE;
				}
			} else {
				switch (LOWORD(lParam)) {
				case FD_CONNECT:
					connected_local_connection(pvar, channel_num);
					break;
				case FD_READ:
					read_local_connection(pvar, channel_num);
					break;
				case FD_CLOSE:
					read_local_connection(pvar, channel_num);
					closed_local_connection(pvar, channel_num);
					break;
				case FD_WRITE:
					write_local_connection_buffer(pvar, channel_num);
					break;
				}
			}
			return TRUE;
		}
	}

	return CallWindowProc(pvar->fwd_state.old_accept_wnd_proc, wnd, msg,
	                      wParam, lParam);
}

int FWD_compare_specs(void const *void_spec1,
                      void const *void_spec2)
{
	FWDRequestSpec *spec1 = (FWDRequestSpec *) void_spec1;
	FWDRequestSpec *spec2 = (FWDRequestSpec *) void_spec2;
	int delta = spec1->from_port - spec2->from_port;

	if (delta == 0) {
		delta = FwdListeningType(spec1->type) - FwdListeningType(spec2->type);
	}

	if (delta == 0) {
		delta = strcmp(spec1->bind_address, spec2->bind_address);
	}

	return delta;
}

/* Check if the server can listen for 'spec' using the old listening spec 'listener'.
   We check that the to_port and (for non-X connections) the to_host are equal
   so that we never lie to the server about where its forwarded connection is
   ending up. Maybe some SSH implementation depends on this information being
   reliable, for security? */
static BOOL can_server_listen_using(FWDRequestSpec *listener,
                                    FWDRequestSpec *spec)
{
	return listener->type == spec->type
	    && listener->from_port == spec->from_port
	    && listener->to_port == spec->to_port
	    && (spec->type == FWD_REMOTE_X11_TO_LOCAL
	     || strcmp(listener->to_host, spec->to_host) == 0)
	    && (spec->type == FWD_REMOTE_X11_TO_LOCAL
	     || strcmp(listener->bind_address, spec->bind_address) == 0);
}

/*
 * ポート転送を有効に出来るかの判定関数。
 * shell / subsystem 開始前はすべて有効にできる (TRUEを返す)。
 * 開始後は以下の転送タイプは追加不可 (すでに開始されている転送設定のみTRUEを返す)
 * - SSH1接続時のRtoL転送 (SSH2は追加可能)
 * - X11転送
 */
BOOL FWD_can_server_listen_for(PTInstVar pvar, FWDRequestSpec *spec)
{
	FWDRequestSpec *listener;
	int num_server_listening_requests =
		pvar->fwd_state.num_server_listening_specs;

	if (num_server_listening_requests < 0) {
		return TRUE;
	}

	switch (spec->type) {
	case FWD_LOCAL_TO_REMOTE:
	case FWD_LOCAL_DYNAMIC:
		return TRUE;
	case FWD_REMOTE_TO_LOCAL:
		if (SSHv2(pvar)) {
			return TRUE;
		}
		// FALLTHROUGH
	default:
		listener =
			bsearch(spec, pvar->fwd_state.server_listening_specs,
			        num_server_listening_requests,
			        sizeof(FWDRequestSpec), FWD_compare_specs);

		if (listener == NULL) {
			return FALSE;
		} else {
			return can_server_listen_using(listener, spec);
		}
	}
}

int FWD_get_num_request_specs(PTInstVar pvar)
{
	int num_request_specs = 0;
	int i;

	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		if ((pvar->fwd_state.requests[i].status & FWD_DELETED) == 0) {
			num_request_specs++;
		}
	}

	return num_request_specs;
}

void FWD_get_request_specs(PTInstVar pvar, FWDRequestSpec *specs,
                           int num_specs)
{
	int i;

	for (i = 0; i < pvar->fwd_state.num_requests && num_specs > 0; i++) {
		if ((pvar->fwd_state.requests[i].status & FWD_DELETED) == 0) {
			*specs = pvar->fwd_state.requests[i].spec;
			num_specs--;
			specs++;
		}
	}
}

/* This function can be called while channels remain open on the request.
   Take care.
   It returns the listening socket for the request, if there is one.
   The caller must close this socket if it is not INVALID_SOCKET.
*/
static SOCKET *delete_request(PTInstVar pvar, int request_num,
                                  int *p_num_listening_sockets)
{
	FWDRequest *request = pvar->fwd_state.requests + request_num;
	SOCKET *lp_listening_sockets;

/* safe to shut down the listening socket here. Any pending connections
   that haven't yet been turned into channels will be broken, but that's
   just tough luck. */
	*p_num_listening_sockets = request->num_listening_sockets;
	lp_listening_sockets = request->listening_sockets;
	if (request->listening_sockets != NULL) {
		request->num_listening_sockets = 0;
		request->listening_sockets = NULL;
	}

	request->status |= FWD_DELETED;

	if (request->num_channels == 0) {
		really_delete_request(pvar, request_num);
	}

	return lp_listening_sockets;
}

static BOOL are_specs_identical(FWDRequestSpec *spec1,
                                FWDRequestSpec *spec2)
{
	return spec1->type == spec2->type
	    && spec1->from_port == spec2->from_port
	    && spec1->to_port == spec2->to_port
	    && strcmp(spec1->to_host, spec2->to_host) == 0
	    && strcmp(spec1->bind_address, spec2->bind_address) == 0;
}

static BOOL interactive_init_request(PTInstVar pvar, int request_num,
                                     BOOL report_error)
{
	FWDRequest *request = pvar->fwd_state.requests + request_num;

	if (request->spec.type == FWD_LOCAL_TO_REMOTE || request->spec.type == FWD_LOCAL_DYNAMIC) {
		struct addrinfo hints;
		struct addrinfo *res;
		struct addrinfo *res0;
		SOCKET s;
		char pname[NI_MAXSERV];
		char bname[NI_MAXHOST];

		_snprintf_s(pname, sizeof(pname), _TRUNCATE, "%d", request->spec.from_port);
		_snprintf_s(bname, sizeof(bname), _TRUNCATE, "%s", request->spec.bind_address);
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;	/* a user will be able to specify protocol in future version */
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(bname, pname, &hints, &res0))
			return FALSE;

		/* count number of listening sockets and allocate area for them */
		for (request->num_listening_sockets = 0, res = res0; res;
		     res = res->ai_next)
			request->num_listening_sockets++;
		request->listening_sockets =
			(SOCKET *) malloc(sizeof(SOCKET) *
			                      request->num_listening_sockets);
		if (request->listening_sockets == NULL) {
			freeaddrinfo(res0);
			return FALSE;
		}

		for (request->num_listening_sockets = 0, res = res0; res;
		     res = res->ai_next) {
			s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
			request->listening_sockets[request->num_listening_sockets++] = s;
			if (s == INVALID_SOCKET)
				continue;
			if (bind(s, res->ai_addr, res->ai_addrlen) == SOCKET_ERROR) {
				s = INVALID_SOCKET;
				continue;
			}
			if (WSAAsyncSelect(s, make_accept_wnd(pvar), WM_SOCK_ACCEPT,
			                   FD_ACCEPT | FD_READ | FD_CLOSE | FD_WRITE) == SOCKET_ERROR)
			{
				s = INVALID_SOCKET;
				continue;
			}
			if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
				s = INVALID_SOCKET;
				continue;
			}
		}
		if (s == INVALID_SOCKET) {
			if (report_error) {
				UTIL_get_lang_msg("MSG_FWD_SOCKET_ERROR", pvar,
				                  "Some socket(s) required for port forwarding could not be initialized.\n"
				                  "Some port forwarding services may not be available.\n"
				                  "(errno %d)");
				logprintf(LOG_LEVEL_WARNING, pvar->ts->UIMsg, WSAGetLastError());
			}
			freeaddrinfo(res0);
			/* free(request->listening_sockets); /* DO NOT FREE HERE, listening_sockets'll be freed in FWD_end */
			return FALSE;
		}
		freeaddrinfo(res0);
	}
	else if (request->spec.type == FWD_REMOTE_TO_LOCAL) {
		if (SSHv2(pvar)) {
			FWDRequestSpec *listener =
				bsearch(&request->spec, pvar->fwd_state.server_listening_specs,
					pvar->fwd_state.num_server_listening_specs,
				        sizeof(FWDRequestSpec), FWD_compare_specs);
			if (listener == NULL) {
				SSH_request_forwarding(pvar,
				                       request->spec.bind_address,
				                       request->spec.from_port,
				                       request->spec.to_host,
				                       request->spec.to_port);
			}
		}
	}

	return TRUE;
}

/* This function will only be called on a request when all its channels are
   closed. */
static BOOL init_request(PTInstVar pvar, int request_num,
                         BOOL report_error, SOCKET *listening_sockets,
                         int num_listening_sockets)
{
	FWDRequest *request = pvar->fwd_state.requests + request_num;

	request->num_listening_sockets = 0;
	request->listening_sockets = NULL;
	request->to_host_addrs = NULL;
	request->to_host_lookup_handle = 0;
	request->status = 0;

	if (pvar->fwd_state.in_interactive_mode) {
		if (listening_sockets != NULL) {
			request->num_listening_sockets = num_listening_sockets;
			request->listening_sockets = listening_sockets;
			return TRUE;
		} else {
			return interactive_init_request(pvar, request_num, report_error);
		}
	} else {
		assert(listening_sockets == NULL);
		return TRUE;
	}
}

static char *dump_fwdspec(FWDRequestSpec *spec, int stat)
{
	static char buff[1024];
	char *ftype;
	char *bind_addr = spec->bind_address;

	switch (spec->type) {
	  case FWD_LOCAL_TO_REMOTE: ftype = "LtoR"; break;
	  case FWD_REMOTE_TO_LOCAL: ftype = "RtoL"; break;
	  case FWD_REMOTE_X11_TO_LOCAL: ftype = "X11"; bind_addr = ftype; break;
	  case FWD_LOCAL_DYNAMIC: ftype="dynamic"; break;
	  default: ftype = "Unknown"; break;
	}

	_snprintf_s(buff, sizeof(buff), _TRUNCATE,
		"type=%s, bind_address=%s, from_port=%d, to_host=%s, to_port=%d%s",
		ftype, bind_addr, spec->from_port, spec->to_host, spec->to_port,
		(stat) ? ", deleted" : "");

	return buff;
}

void FWD_set_request_specs(PTInstVar pvar, FWDRequestSpec *specs, int num_specs)
{
	FWDRequestSpec *new_specs =
		(FWDRequestSpec *) malloc(sizeof(FWDRequestSpec) * num_specs);
	FWDRequestSpec *server_listening_specs = pvar->fwd_state.server_listening_specs;
	char *specs_accounted_for;
	char *listening_specs_remain_for = NULL;
	typedef struct _saved_sockets {
		SOCKET *listening_sockets;
		int num_listening_sockets;
	} saved_sockets_t;
	saved_sockets_t *ptr_to_saved_sockets;
	int i;
	int num_new_requests = num_specs;
	int num_free_requests = 0;
	int free_request = 0;
	int num_new_listening = 0;
	int num_cur_listening = pvar->fwd_state.num_server_listening_specs;
	int x11_listening = -1;
	BOOL report_err = TRUE;

	memcpy(new_specs, specs, sizeof(FWDRequestSpec) * num_specs);
	qsort(new_specs, num_specs, sizeof(FWDRequestSpec), FWD_compare_specs);

	for (i = 0; i < num_specs - 1; i++) {
		if (FWD_compare_specs(new_specs + i, new_specs + i + 1) == 0) {
			UTIL_get_lang_msg("MSG_FWD_DUPLICATE_ERROR", pvar,
			                  "TTSSH INTERNAL ERROR: Could not set port forwards "
			                  "because duplicate type/port requests were found");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			free(new_specs);
			return;
		}
	}

	specs_accounted_for = (char *) malloc(sizeof(char) * num_specs);
	ptr_to_saved_sockets =
		(saved_sockets_t *) malloc(sizeof(saved_sockets_t) * num_specs);

	memset(specs_accounted_for, 0, num_specs);
	for (i = 0; i < num_specs; i++) {
		ptr_to_saved_sockets[i].listening_sockets = NULL;
		ptr_to_saved_sockets[i].num_listening_sockets = 0;
	}

	//
	// 他の LOG_LEVEL_VERBOSE で出力しているログに比べて、
	// さらに高いログレベルで出力したいので暫定で 150 にする。
	// 他のも含めて LOG_LEVEL を整理したい……
	//
	if (LogLevel(pvar, 150)) {
		logprintf(150, "%s: old specs: %d", __FUNCTION__, pvar->fwd_state.num_requests);
		for (i=0; i < pvar->fwd_state.num_requests; i++) {
			logprintf(150, "%s:   #%d: %s", __FUNCTION__, i,
				dump_fwdspec(&pvar->fwd_state.requests[i].spec, pvar->fwd_state.requests[i].status));
		}

		logprintf(150, "%s: new specs: %d", __FUNCTION__, num_specs);
		for (i=0; i < num_specs; i++) {
			logprintf(150, "%s:   #%d: %s", __FUNCTION__, i, dump_fwdspec(new_specs+i, 0));
		}

		logprintf(150, "%s: listening specs: %d", __FUNCTION__, num_cur_listening);
		for (i=0; i < num_cur_listening; i++) {
			logprintf(150, "%s:   #%d: %s", __FUNCTION__, i,
				dump_fwdspec(&server_listening_specs[i], 0));
		}
	}

	for (i = pvar->fwd_state.num_requests - 1; i >= 0; i--) {
		if ((pvar->fwd_state.requests[i].status & FWD_DELETED) == 0) {
			FWDRequestSpec *cur_spec =
				&pvar->fwd_state.requests[i].spec;
			FWDRequestSpec *new_spec =
				bsearch(cur_spec, new_specs, num_specs,
				        sizeof(FWDRequestSpec), FWD_compare_specs);

			if (new_spec != NULL
			 && are_specs_identical(cur_spec, new_spec)) {
				specs_accounted_for[new_spec - new_specs] = 1;
				num_new_requests--;
			} else {
				int num_listening_sockets;
				SOCKET *listening_sockets;
				listening_sockets =
					delete_request(pvar, i, &num_listening_sockets);

				if (new_spec != NULL) {
					ptr_to_saved_sockets[new_spec -  new_specs].
						listening_sockets = listening_sockets;
					ptr_to_saved_sockets[new_spec - new_specs].
						num_listening_sockets = num_listening_sockets;
				} else if (listening_sockets != NULL) {
					/* if there is no new spec(new request), free listening_sockets */
					int i;
					for (i = 0; i < num_listening_sockets; ++i)
						safe_closesocket(pvar, listening_sockets[i]);
				}

				if (pvar->fwd_state.requests[i].num_channels == 0) {
					num_free_requests++;
				}
			}
		} else {
			if (pvar->fwd_state.requests[i].num_channels == 0) {
				num_free_requests++;
			}
		}
	}

	if (num_new_requests > num_free_requests) {
		int total_requests =
			pvar->fwd_state.num_requests + num_new_requests - num_free_requests;

		pvar->fwd_state.requests =
			(FWDRequest *) realloc(pvar->fwd_state.requests,
			                       sizeof(FWDRequest) * total_requests);
		for (i = pvar->fwd_state.num_requests; i < total_requests; i++) {
			pvar->fwd_state.requests[i].status = FWD_DELETED;
			pvar->fwd_state.requests[i].num_channels = 0;
		}
		pvar->fwd_state.num_requests = total_requests;
	}

	if (num_cur_listening > 0) {
		listening_specs_remain_for = (char *) malloc(sizeof(char) * num_cur_listening);
		memset(listening_specs_remain_for, 0, num_cur_listening);
	}

	for (i = 0; i < num_specs; i++) {
		if (!specs_accounted_for[i]) {
			while ((pvar->fwd_state.requests[free_request].status & FWD_DELETED) == 0
			    || pvar->fwd_state.requests[free_request].num_channels != 0) {
				free_request++;
			}

			assert(free_request < pvar->fwd_state.num_requests);

			pvar->fwd_state.requests[free_request].spec = new_specs[i];
			if (!init_request(pvar, free_request, report_err,
			                  ptr_to_saved_sockets[i].listening_sockets,
			                  ptr_to_saved_sockets[i].num_listening_sockets)) {
				report_err = FALSE;
			}

			free_request++;
		}

		// 更新後もサーバ側で listen し続けるかのマーク付け
		if (new_specs[i].type == FWD_REMOTE_TO_LOCAL) {
			if (num_cur_listening > 0) {
				FWDRequestSpec *listening_spec =
					bsearch(&new_specs[i], server_listening_specs, num_specs, sizeof(FWDRequestSpec), FWD_compare_specs);
				if (listening_spec != NULL) {
					listening_specs_remain_for[listening_spec - server_listening_specs] = 1;
				}
			}
			num_new_listening++;
		}
	}

	for (i = 0; i < num_cur_listening; i++) {
		FWDRequestSpec *lspec = &server_listening_specs[i];
		if (lspec->type == FWD_REMOTE_X11_TO_LOCAL) {
			// X11 転送はキャンセルできないので、そのまま引き継ぐ
			// 引き継ぐ為に場所を覚えておく
			x11_listening = i;
			num_new_listening++;
		}
		else if (!listening_specs_remain_for[i]) {
			SSH_cancel_request_forwarding(pvar, lspec->bind_address, lspec->from_port, 0);
		}
	}

	if (x11_listening > 0) {
		// X11 転送が有った場合は先頭に移動する (reallocで消されないようにする為)
		server_listening_specs[0] = server_listening_specs[x11_listening];
	}

	if (num_new_listening > 0) {
		if (num_cur_listening >= 0)  {
			server_listening_specs = realloc(server_listening_specs, sizeof(FWDRequestSpec) * num_new_listening);
			if (server_listening_specs) {
				FWDRequestSpec *dst = server_listening_specs;
				if (x11_listening >= 0) {
					dst++;
				}
				for (i=0; i < num_specs; i++) {
					if (new_specs[i].type == FWD_REMOTE_TO_LOCAL) {
						*dst = new_specs[i];
						dst++;
					}
				}
				qsort(server_listening_specs, num_new_listening, sizeof(FWDRequestSpec), FWD_compare_specs);
				pvar->fwd_state.server_listening_specs = server_listening_specs;
				pvar->fwd_state.num_server_listening_specs = num_new_listening;
			}
		}
	}
	else if (num_cur_listening > 0) {
		free(server_listening_specs);
		pvar->fwd_state.server_listening_specs = NULL;
		pvar->fwd_state.num_server_listening_specs = 0;
	}

	if (LogLevel(pvar, 150)) {
		logprintf(150, "%s: updated specs: %d", __FUNCTION__, pvar->fwd_state.num_requests);
		for (i=0; i < pvar->fwd_state.num_requests; i++) {
			logprintf(150, "%s:   #%d: %s", __FUNCTION__, i,
				dump_fwdspec(&pvar->fwd_state.requests[i].spec, pvar->fwd_state.requests[i].status));
		}
		logprintf(150, "%s: new listening specs: %d", __FUNCTION__, pvar->fwd_state.num_server_listening_specs);
		for (i=0; i < pvar->fwd_state.num_server_listening_specs; i++) {
			logprintf(150, "%s:   #%d: %s", __FUNCTION__, i,
				dump_fwdspec(&pvar->fwd_state.server_listening_specs[i], 0));
		}
	}

	free(ptr_to_saved_sockets);
	free(listening_specs_remain_for);
	free(specs_accounted_for);
	free(new_specs);
}

void FWD_prep_forwarding(PTInstVar pvar)
{
	int i;
	int num_server_listening_requests = 0;

	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		FWDRequest *request = pvar->fwd_state.requests + i;

		if ((request->status & FWD_DELETED) == 0) {
			switch (request->spec.type) {
			case FWD_REMOTE_TO_LOCAL:
				SSH_request_forwarding(pvar,
				                       request->spec.bind_address,
				                       request->spec.from_port,
				                       request->spec.to_host,
				                       request->spec.to_port);
				num_server_listening_requests++;
				break;
			case FWD_REMOTE_X11_TO_LOCAL:{
					int screen_num = request->spec.x11_screen;

					pvar->fwd_state.X11_auth_data =
						X11_load_local_auth_data(screen_num);
					SSH_request_X11_forwarding(
						pvar,
						X11_get_spoofed_protocol_name(pvar->fwd_state.X11_auth_data),
						X11_get_spoofed_protocol_data(pvar->fwd_state.X11_auth_data),
						X11_get_spoofed_protocol_data_len(pvar->fwd_state.X11_auth_data),
						screen_num);
					num_server_listening_requests++;
					break;
				}
			}
		}
	}

	pvar->fwd_state.num_server_listening_specs =
		num_server_listening_requests;

	if (num_server_listening_requests > 0) {
		FWDRequestSpec *server_listening_requests =
			(FWDRequestSpec *) malloc(sizeof(FWDRequestSpec) * num_server_listening_requests);

		pvar->fwd_state.server_listening_specs = server_listening_requests;

		for (i = 0; i < pvar->fwd_state.num_requests; i++) {
			if ((pvar->fwd_state.requests[i].status & FWD_DELETED) == 0) {
				switch (pvar->fwd_state.requests[i].spec.type) {
				case FWD_REMOTE_X11_TO_LOCAL:
				case FWD_REMOTE_TO_LOCAL:
					*server_listening_requests = pvar->fwd_state.requests[i].spec;
					server_listening_requests++;
					break;
				}
			}
		}

		/* No two server-side request specs can have the same port. */
		qsort(pvar->fwd_state.server_listening_specs,
		      num_server_listening_requests, sizeof(FWDRequestSpec),
		      FWD_compare_specs);
	}
}

void FWD_enter_interactive_mode(PTInstVar pvar)
{
	BOOL report_error = TRUE;
	int i;

	pvar->fwd_state.in_interactive_mode = TRUE;

	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		if (!interactive_init_request(pvar, i, report_error)) {
			report_error = FALSE;
		}
	}
}

static void create_local_channel(PTInstVar pvar, uint32 remote_channel_num,
                                 int request_num, void *filter_closure,
                                 FWDFilter filter, int *chan_num)
{
	char buf[1024];
	int channel_num;
	FWDChannel *channel;
	FWDRequest *request = pvar->fwd_state.requests + request_num;
	struct addrinfo hints;
	char pname[NI_MAXSERV];

	if (request->to_host_addrs == NULL
	 && request->to_host_lookup_handle == 0) {
		HANDLE task_handle;

		_snprintf_s(pname, sizeof(pname), _TRUNCATE, "%d", request->spec.to_port);
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		task_handle =
			WSAAsyncGetAddrInfo(make_accept_wnd(pvar), WM_SOCK_GOTNAME,
			                    request->spec.to_host, pname, &hints,
			                    &request->to_host_addrs);

		if (task_handle == 0) {
			SSH_fail_channel_open(pvar, remote_channel_num);
			UTIL_get_lang_msg("MSG_FWD_DENIED_HANDLE_ERROR", pvar,
			                  "The server attempted to forward a connection through this machine.\n"
			                  "It requested a connection to machine %s on port %s.\n"
			                  "An error occurred while processing the request, and it has been denied.");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			            pvar->ts->UIMsg,
			            request->spec.to_host, request->spec.to_port_name);
			notify_nonfatal_error(pvar, buf);
			freeaddrinfo(request->to_host_addrs);
			return;
		} else {
			request->to_host_lookup_handle = task_handle;
		}
	}

	channel_num = alloc_channel(pvar, FWD_REMOTE_CONNECTED, request_num);
	channel = pvar->fwd_state.channels + channel_num;

	channel->remote_num = remote_channel_num;
	channel->filter_closure = filter_closure;
	channel->filter = filter;

	// (2008.12.5 maya)
	channel->type = TYPE_PORTFWD;

	// save channel number (2005.7.2 yutaka)
	if (chan_num != NULL) {
		*chan_num = channel_num;
	}

	if (request->to_host_addrs != NULL) {
		make_local_connection(pvar, channel_num);
	}
}

void FWD_open(PTInstVar pvar, uint32 remote_channel_num,
              char *local_hostname, int local_port,
              char *originator, int originator_len,
              int *chan_num)
{
	int i;
	char buf[1024];

	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		FWDRequest *request = pvar->fwd_state.requests + i;

		if (SSHv1(pvar)) {
			if ((request->status & FWD_DELETED) == 0
			 && request->spec.type == FWD_REMOTE_TO_LOCAL
			 && request->spec.to_port == local_port
			 && strcmp(request->spec.to_host, local_hostname) == 0) {
				create_local_channel(pvar, remote_channel_num, i, NULL, NULL, chan_num);
				return;
			}

		} else { // SSH2
			if ((request->status & FWD_DELETED) == 0
			 && request->spec.type == FWD_REMOTE_TO_LOCAL
			 && request->spec.from_port == local_port
			// && strcmp(request->spec.to_host, local_hostname) == 0) {
			) {
				create_local_channel(pvar, remote_channel_num, i, NULL, NULL, chan_num);
				return;
			}
		}
	}

	SSH_fail_channel_open(pvar, remote_channel_num);

	/* now, before we panic, maybe we TOLD the server we could forward this port
	   and then the user changed the settings. */
	for (i = 0; i < pvar->fwd_state.num_server_listening_specs; i++) {
		FWDRequestSpec *spec =
			pvar->fwd_state.server_listening_specs + i;

		if (spec->type == FWD_REMOTE_TO_LOCAL
		 && spec->to_port == local_port
		 && strcmp(spec->to_host, local_hostname) == 0) {
			return;				/* no error message needed. The user just turned this off, that's all */
		}
	}

	/* this forwarding was not prespecified */
	UTIL_get_lang_msg("MSG_FWD_DENIED_ERROR", pvar,
	                  "The server attempted to forward a connection through this machine.\n"
	                  "It requested a connection to machine %s on port %d.\n"
	                  "You did not specify this forwarding to TTSSH in advance, and therefore the request was denied.");
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
	            pvar->ts->UIMsg,
	            local_hostname, local_port);
	notify_nonfatal_error(pvar, buf);
}

void FWD_X11_open(PTInstVar pvar, uint32 remote_channel_num,
                  char *originator, int originator_len,
                  int *chan_num)
{
	int i;

	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		FWDRequest *request = pvar->fwd_state.requests + i;

		if ((request->status & FWD_DELETED) == 0
		 && request->spec.type == FWD_REMOTE_X11_TO_LOCAL) {
			create_local_channel(pvar, remote_channel_num, i,
			                     X11_init_unspoofing_filter(pvar,
			                                                pvar->fwd_state.
			                                                X11_auth_data),
			                     X11_unspoofing_filter,
			                     chan_num);
			return;
		}
	}

	SSH_fail_channel_open(pvar, remote_channel_num);

	/* now, before we panic, maybe we TOLD the server we could forward this port
	   and then the user changed the settings. */
	for (i = 0; i < pvar->fwd_state.num_server_listening_specs; i++) {
		FWDRequestSpec *spec =
			pvar->fwd_state.server_listening_specs + i;

		if (spec->type == FWD_REMOTE_X11_TO_LOCAL) {
			return;				/* no error message needed. The user just turned this off, that's all */
		}
	}

	/* this forwarding was not prespecified */
	UTIL_get_lang_msg("MSG_FWD_DENIED_X_ERROR", pvar,
	                  "The server attempted to forward a connection through this machine.\n"
	                  "It requested a connection to the local X server.\n"
	                  "You did not specify this forwarding to TTSSH in advance, and therefore the request was denied.");
	notify_nonfatal_error(pvar, pvar->ts->UIMsg);
}

// agent forwarding の要求に対し、FWDChannel を作成する
// SSH1 のときのみ呼ばれる
int FWD_agent_open(PTInstVar pvar, uint32 remote_channel_num)
{
	if (SSHv1(pvar)) {
		return alloc_agent_channel(pvar, remote_channel_num);
	}

	return -1;
}

void FWD_confirmed_open(PTInstVar pvar, uint32 local_channel_num,
                        uint32 remote_channel_num)
{
	SOCKET s;
	FWDChannel *channel;
	FwdFilterResult action = FWD_FILTER_RETAIN;

	if (!FWD_check_local_channel_num(pvar, local_channel_num))
		return;

	channel = pvar->fwd_state.channels + local_channel_num;

	if (channel->filter != NULL) {
		action = channel->filter(channel->filter_closure, FWD_FILTER_OPENCONFIRM, NULL, NULL);
		switch (action) {
		case FWD_FILTER_REMOVE:
			channel->filter(channel->filter_closure, FWD_FILTER_CLEANUP, NULL, NULL);
			channel->filter = NULL;
			channel->filter_closure = NULL;
			break;
		case FWD_FILTER_CLOSECHANNEL:
			closed_local_connection(pvar, local_channel_num);
			break;
		}
	}

	s = channel->local_socket;
	if (s != INVALID_SOCKET) {
		channel->remote_num = remote_channel_num;
		channel->status |= FWD_REMOTE_CONNECTED;

		read_local_connection(pvar, local_channel_num);
	} else {
		SSH_channel_input_eof(pvar, remote_channel_num, local_channel_num);
		SSH_channel_output_eof(pvar, remote_channel_num);
		channel->status |= FWD_CLOSED_LOCAL_IN | FWD_CLOSED_LOCAL_OUT;
	}
}

void FWD_failed_open(PTInstVar pvar, uint32 local_channel_num, int reason)
{
	FWDChannel *channel;
	int r = reason;

	if (!FWD_check_local_channel_num(pvar, local_channel_num))
		return;

	channel = pvar->fwd_state.channels + local_channel_num;

	// SSH2 では呼び出し元で既にポップアップを出しているので、
	// ここでは SSH1 の時のみポップアップを出す
	if (SSHv1(pvar)) {
		UTIL_get_lang_msg("MSG_FWD_DENIED_BY_SERVER_ERROR", pvar,
		                  "A program on the local machine attempted to connect to a forwarded port.\n"
		                  "The forwarding request was denied by the server. The connection has been closed.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	}

	if (channel->filter != NULL) {
		channel->filter(channel->filter_closure, FWD_FILTER_OPENFAILURE, &r, NULL);
	}
	FWD_free_channel(pvar, local_channel_num);
}

static BOOL blocking_write(PTInstVar pvar, SOCKET s, const char *data,
                           int length)
{
	u_long do_block = 0;

#if 0
	return (pvar->PWSAAsyncSelect) (s, make_accept_wnd(pvar), 0,
	                                0) == SOCKET_ERROR
	    || ioctlsocket(s, FIONBIO, &do_block) == SOCKET_ERROR
	    || (pvar->Psend) (s, data, length, 0) != length
	    || (pvar->PWSAAsyncSelect) (s, pvar->fwd_state.accept_wnd,
	                                WM_SOCK_ACCEPT,
	                                FD_READ | FD_CLOSE | FD_WRITE) == SOCKET_ERROR;
#else
	if ( (pvar->PWSAAsyncSelect) (s, make_accept_wnd(pvar), 0, 0) == SOCKET_ERROR ) {
			goto error;
	}

	if ( ioctlsocket(s, FIONBIO, &do_block) == SOCKET_ERROR ) {
			goto error;
	}

	if ( (pvar->Psend) (s, data, length, 0) != length ) {
			goto error;
	}

	if ( (pvar->PWSAAsyncSelect) (s, pvar->fwd_state.accept_wnd,
	                                WM_SOCK_ACCEPT,
									FD_READ | FD_CLOSE | FD_WRITE) == SOCKET_ERROR ) {
			goto error;
	}

	return (TRUE);

error:
	return (FALSE);
#endif
}

void FWD_received_data(PTInstVar pvar, uint32 local_channel_num,
                       unsigned char *data, int length)
{
	SOCKET s;
	FWDChannel *channel;
	FwdFilterResult action = FWD_FILTER_RETAIN;

	if (!FWD_check_local_channel_num(pvar, local_channel_num))
		return;

	channel = pvar->fwd_state.channels + local_channel_num;

	if (channel->filter != NULL) {
		action = channel->filter(channel->filter_closure, FWD_FILTER_FROM_SERVER, &length, &data);
	}

	s = channel->local_socket;
	if (length > 0 && s != INVALID_SOCKET) {
		// SSHサーバのXアプリケーションから送られてきたX11データを、
		// Tera Termと同じPCに同居しているXサーバに、ブロッキングモードを使って、
		// 送信する。
		// Xサーバに送信時に発生する FD_WRITE は、処理する必要がないため無視する。
		//OutputDebugPrintf("%s: send %d\n", __FUNCTION__, length);
		if (!UTIL_sock_buffered_write(pvar, &channel->writebuf, blocking_write, s, data, length)) {
			closed_local_connection(pvar, local_channel_num);

			// ポップアップ抑止指定あれば、関数を呼び出さない。
			// (2014.6.26 yutaka)
			if ((pvar->settings.DisablePopupMessage & POPUP_MSG_FWD_received_data) == 0) {
				UTIL_get_lang_msg("MSG_FWD_COMM_ERROR", pvar,
				                  "A communications error occurred while sending forwarded data to a local port.\n"
				                  "The forwarded connection will be closed.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			}
		}
	}

	switch (action) {
	case FWD_FILTER_REMOVE:
		channel->filter(channel->filter_closure, FWD_FILTER_CLEANUP, NULL, NULL);
		channel->filter = NULL;
		channel->filter_closure = NULL;
		break;
	case FWD_FILTER_CLOSECHANNEL:
		closed_local_connection(pvar, local_channel_num);
		break;
	}
}

void FWD_init(PTInstVar pvar)
{
	pvar->fwd_state.requests = NULL;
	pvar->fwd_state.num_requests = 0;
	pvar->fwd_state.num_server_listening_specs = -1;	/* forwarding prep not yet done */
	pvar->fwd_state.server_listening_specs = NULL;
	pvar->fwd_state.num_channels = 0;
	pvar->fwd_state.channels = NULL;
	pvar->fwd_state.X11_auth_data = NULL;
	pvar->fwd_state.accept_wnd = NULL;
	pvar->fwd_state.in_interactive_mode = FALSE;
}

void FWD_end(PTInstVar pvar)
{
	int i;

	if (pvar->fwd_state.channels != NULL) {
		for (i = 0; i < pvar->fwd_state.num_channels; i++) {
			FWD_free_channel(pvar, i);
		}
		free(pvar->fwd_state.channels);
	}

	if (pvar->fwd_state.requests != NULL) {
		for (i = 0; i < pvar->fwd_state.num_requests; i++) {
			int j;
			int num_listening_sockets;
			SOCKET *s =
				delete_request(pvar, i, &num_listening_sockets);

			for (j = 0; j < num_listening_sockets; ++j) {
				if (s[j] != INVALID_SOCKET)
					closesocket(s[j]);
			}
			if (s != NULL)
				free(s);		/* free(request[i]->listening_sockets) */
		}
		free(pvar->fwd_state.requests);
	}

	free(pvar->fwd_state.server_listening_specs);

	if (pvar->fwd_state.X11_auth_data != NULL) {
		X11_dispose_auth_data(pvar->fwd_state.X11_auth_data);
	}

	if (pvar->fwd_state.accept_wnd != NULL) {
		DestroyWindow(pvar->fwd_state.accept_wnd);
	}
}

BOOL FWD_agent_forward_confirm(PTInstVar pvar)
{
	char title[MAX_UIMSG];
	HWND cur_active = GetActiveWindow();

	if (pvar->session_settings.ForwardAgentNotify) {
		UTIL_get_lang_msg("MSG_FWD_AGENT_NOTIFY_TITLE", pvar, "Agent Forwarding");
		strncpy_s(title, sizeof(title), pvar->ts->UIMsg, _TRUNCATE);
		UTIL_get_lang_msg("MSG_FWD_AGENT_NOTIFY", pvar, "Remote host access to agent");
		NotifyInfoMessage(pvar->cv, pvar->ts->UIMsg, title);
	}

	if (pvar->session_settings.ForwardAgentConfirm) {
		UTIL_get_lang_msg("MSG_FWD_AGENT_FORWARDING_CONFIRM", pvar,
		                  "Are you sure you want to accept agent-forwarding request?");
		if (MessageBox(cur_active != NULL ? cur_active : pvar->NotificationWindow,
		               pvar->ts->UIMsg, "TTSSH",
		               MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
			return TRUE;
		}
		else {
			return FALSE;
		}
	}
	return TRUE;
}
