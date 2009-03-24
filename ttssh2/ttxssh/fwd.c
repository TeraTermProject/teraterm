/*
Copyright (c) 1998-2001, Robert O'Callahan
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.

The name of Robert O'Callahan may not be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#include "ttxssh.h"

#include "x11util.h"

#include "fwd.h"

#include <assert.h>
#ifndef NO_INET6
#include "WSAAsyncGetAddrInfo.h"
#endif							/* NO_INET6 */

#define WM_SOCK_ACCEPT (WM_APP+9999)
#define WM_SOCK_IO     (WM_APP+9998)
#define WM_SOCK_GOTNAME (WM_APP+9997)

#define CHANNEL_READ_BUF_SIZE 8192

static LRESULT CALLBACK accept_wnd_proc(HWND wnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam);

static int find_request_num(PTInstVar pvar, SOCKET s)
{
	int i;
#ifndef NO_INET6
	int j;
#endif							/* NO_INET6 */

	if (s == INVALID_SOCKET)
		return -1;

#ifndef NO_INET6
	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		for (j = 0; j < pvar->fwd_state.requests[i].num_listening_sockets;
			 ++j) {
			if ((pvar->fwd_state.requests[i].status & FWD_DELETED) == 0
				&& pvar->fwd_state.requests[i].listening_sockets[j] == s) {
				return i;
			}
		}
	}
#else
	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		if ((pvar->fwd_state.requests[i].status & FWD_DELETED) == 0
			&& pvar->fwd_state.requests[i].listening_socket == s) {
			return i;
		}
	}
#endif							/* NO_INET6 */

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

#ifndef NO_INET6
static int find_listening_socket_num(PTInstVar pvar, int request_num,
                                     SOCKET s)
{
	FWDRequest FAR *request = pvar->fwd_state.requests + request_num;
	int i;

	for (i = 0; i < request->num_listening_sockets; ++i)
		if (request->listening_sockets[i] == s)
			return i;

	/* not found */
	return -1;
}
#endif							/* NO_INET6 */

static void drain_matching_messages(HWND wnd, UINT msg, WPARAM wParam)
{
	MSG m;
	MSG FAR *buf;
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
	buf = (MSG FAR *) malloc(sizeof(MSG) * buf_size);

	while (PeekMessage(&m, wnd, msg, msg, PM_REMOVE)) {
		if (buf_len == buf_size) {
			buf_size *= 2;
			buf = (MSG FAR *) realloc(buf, sizeof(MSG) * buf_size);
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
#ifndef NO_INET6
	SOCKET FAR *s =
		pvar->fwd_state.requests[request_num].listening_sockets;
	int i;
#else
	SOCKET s = pvar->fwd_state.requests[request_num].listening_socket;
#endif							/* NO_INET6 */

#ifndef NO_INET6
	for (i = 0;
	     i < pvar->fwd_state.requests[request_num].num_listening_sockets;
	     ++i) {
		if (s[i] != INVALID_SOCKET) {
			safe_closesocket(pvar, s[i]);
			pvar->fwd_state.requests[request_num].listening_sockets[i] =
				INVALID_SOCKET;
		}
	}
#else
	if (s != INVALID_SOCKET) {
		safe_closesocket(pvar, s);
		pvar->fwd_state.requests[request_num].listening_socket =
			INVALID_SOCKET;
	}
#endif							/* NO_INET6 */

	UTIL_get_lang_msg("MSG_FWD_REQUEST_ERROR", pvar,
	                  "Communications error while listening for a connection to forward.\n"
	                  "The listening port will be terminated.");
	notify_nonfatal_error(pvar, pvar->ts->UIMsg);
}

static void send_local_connection_closure(PTInstVar pvar, int channel_num)
{
	FWDChannel FAR *channel = pvar->fwd_state.channels + channel_num;

	if ((channel->status & (FWD_REMOTE_CONNECTED | FWD_LOCAL_CONNECTED))
	 == (FWD_REMOTE_CONNECTED | FWD_LOCAL_CONNECTED)) {
		SSH_channel_input_eof(pvar, channel->remote_num, channel_num);
		SSH_channel_output_eof(pvar, channel->remote_num);
		channel->status |= FWD_CLOSED_LOCAL_IN | FWD_CLOSED_LOCAL_OUT;
	}
}

static void closed_local_connection(PTInstVar pvar, int channel_num)
{
	FWDChannel FAR *channel = pvar->fwd_state.channels + channel_num;

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
	FWDRequest FAR *request = pvar->fwd_state.requests + request_num;

	if (request->to_host_lookup_handle != 0) {
		safe_WSACancelAsyncRequest(pvar, request->to_host_lookup_handle);
		request->to_host_lookup_handle = 0;
	}
#ifndef NO_INET6
  /*****/
	/* freeaddrinfo(); */
#else
	free(request->to_host_hostent_buf);
	request->to_host_hostent_buf = NULL;
#endif							/* NO_INET6 */
}

void FWD_free_channel(PTInstVar pvar, uint32 local_channel_num)
{
	FWDChannel FAR *channel = &pvar->fwd_state.channels[local_channel_num];

	if (channel->type == TYPE_AGENT) {
		buffer_free(channel->agent_msg);
		// channel_close 時と TTSSH 終了時に2回呼ばれるので、二重 free 防止のため
		channel->agent_msg = NULL;
		channel->status = 0;
	}
	else {
		UTIL_destroy_sock_write_buf(&channel->writebuf);
		if (channel->filter != NULL) {
			channel->filter(channel->filter_closure, 0, NULL, NULL);
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
		FWDRequest FAR *request =
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
	FWDChannel FAR *channel;

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
	FWDChannel FAR *channel;

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

static char FAR *describe_socket_error(PTInstVar pvar, int code)
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
		UTIL_get_lang_msg("MSG_FWD_ADDRNOTFOUTD_ERROR", pvar,
		                  "No address was found for the machine");
		return pvar->ts->UIMsg;
	default:
		UTIL_get_lang_msg("MSG_FWD_CONNECT_ERROR", pvar,
		                  "The forwarding connection could not be established");
		return pvar->ts->UIMsg;
	}
}

static void channel_error(PTInstVar pvar, char FAR * action,
                          int channel_num, int err)
{
	char FAR *err_msg;
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
	FWDChannel FAR *channel = &pvar->fwd_state.channels[channel_num];
	FWDRequest FAR *request =
		&pvar->fwd_state.requests[channel->request_num];
	char uimsg[MAX_UIMSG];

	SSH_fail_channel_open(pvar, channel->remote_num);
	strncpy_s(uimsg, sizeof(uimsg), describe_socket_error(pvar, err), _TRUNCATE);
	if (request->spec.type == FWD_REMOTE_X11_TO_LOCAL) {
		UTIL_get_lang_msg("MSG_FWD_CHANNEL_OPEN_X_ERROR", pvar,
		                  "The server attempted to forward a connection through this machine.\n"
		                  "It requested a connection to the X server on %s (screen %d).\n"
		                  "%s.\n" "The forwarded connection will be closed.");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->ts->UIMsg,
		            request->spec.to_host, request->spec.to_port - 6000,
		            uimsg);
	} else {
		UTIL_get_lang_msg("MSG_FWD_CHANNEL_OPEN_ERROR", pvar,
		                  "The server attempted to forward a connection through this machine.\n"
		                  "It requested a connection to %s (port %s).\n" "%s.\n"
		                  "The forwarded connection will be closed.");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->ts->UIMsg,
		            request->spec.to_host, request->spec.to_port_name,
		            uimsg);
	}
	notify_nonfatal_error(pvar, buf);
	FWD_free_channel(pvar, channel_num);
}

static void init_local_IP_numbers(PTInstVar pvar)
{
#ifndef NO_INET6
	struct addrinfo hints;
	struct addrinfo FAR *res0;
	struct addrinfo FAR *res;
	char buf[1024];
	int num_addrs = 0;
	int i;
	struct sockaddr_storage FAR *addrs;

	if (!gethostname(buf, sizeof(buf))) {
		memset(&hints, 0, sizeof(hints));
		if (getaddrinfo(buf, NULL, &hints, &res0))
			res0 = NULL;

		/* count number of addresses */
		for (res = res0; res; res = res->ai_next)
			num_addrs++;
	}

	addrs =
		(struct sockaddr_storage FAR *)
		malloc(sizeof(struct sockaddr_storage) * (num_addrs + 1));
	for (res = res0, i = 0; i < num_addrs; ++i, res = res->ai_next)
		memcpy(&addrs[i], res->ai_addr, res->ai_addrlen);

	pvar->fwd_state.local_host_IP_numbers = addrs;

	/* terminated by all zero filled sockaddr_storage */
	memset(&addrs[num_addrs], 0, sizeof(struct sockaddr_storage));
#else
	HOSTENT FAR *hostent;
	char buf[1024];
	int num_addrs = 0;
	uint32 FAR *addrs;

	if (!gethostname(buf, sizeof(buf))) {
		hostent = gethostbyname(buf);

		if (hostent != NULL) {
			for (; hostent->h_addr_list[num_addrs] != NULL; num_addrs++) {
			}
		}
	} else {
		hostent = NULL;
	}

	addrs = (uint32 FAR *) malloc(sizeof(uint32) * (num_addrs + 1));
	pvar->fwd_state.local_host_IP_numbers = addrs;
	if (hostent != NULL) {
		int i;

		for (i = 0; i < num_addrs; i++) {
			addrs[i] = ntohl(*(uint32 FAR *) hostent->h_addr_list[i]);
		}
	}
	addrs[num_addrs] = 0;
#endif							/* NO_INET6 */
}

#ifndef NO_INET6
static BOOL validate_IP_number(PTInstVar pvar, struct sockaddr FAR * addr)
{
#else
static BOOL validate_IP_number(PTInstVar pvar, uint32 addr)
{
#endif							/* NO_INET6 */
#ifndef NO_INET6
	int i;
	struct sockaddr_storage zss;	/* all bytes are filled by zero */

	/* Should we allow a wider range of loopback addresses here?
	   i.e. 127.xx.xx.xx or ::1
	   Wouldn't want to introduce a security hole if there's
	   some OS bug that lets an intruder get packets from us
	   to such an address */
	switch (addr->sa_family) {
	case AF_INET:
		if (((struct sockaddr_in FAR *) addr)->sin_addr.s_addr ==
			htonl(INADDR_LOOPBACK))
			return TRUE;
		break;
	case AF_INET6:
		if (IN6_IS_ADDR_LOOPBACK
			(&(((struct sockaddr_in6 FAR *) addr)->sin6_addr)))
			return TRUE;
		break;
	default:
		/* NOT REACHED */
		break;
	}

	if (pvar->fwd_state.local_host_IP_numbers == NULL) {
		init_local_IP_numbers(pvar);
	}

	memset(&zss, 0, sizeof(zss));
	for (i = 0;; i++) {
		if (memcmp
			(&pvar->fwd_state.local_host_IP_numbers[i], &zss,
			 sizeof(struct sockaddr_storage)) == 0)
			break;

		switch (addr->sa_family) {
		case AF_INET:
			if (memcmp
				(&pvar->fwd_state.local_host_IP_numbers[i], addr,
				 sizeof(struct sockaddr_in)) == 0)
				return TRUE;
			break;
		case AF_INET6:
			if (memcmp
				(&pvar->fwd_state.local_host_IP_numbers[i], addr,
				 sizeof(struct sockaddr_in6)) == 0)
				return TRUE;
			break;
		default:
			/* NOT REACHED */
			break;
		}
	}
	return FALSE;
#else
	int i;

	if (pvar->settings.LocalForwardingIdentityCheck) {
		/* Should we allow a wider range of loopback addresses here?
		   i.e. 127.xx.xx.xx
		   Wouldn't want to introduce a security hole if there's
		   some OS bug that lets an intruder get packets from us
		   to such an address */
		if (addr == INADDR_LOOPBACK) {
			return TRUE;
		}

		if (pvar->fwd_state.local_host_IP_numbers == NULL) {
			init_local_IP_numbers(pvar);
		}

		for (i = 0; pvar->fwd_state.local_host_IP_numbers[i] != 0; i++) {
			if (pvar->fwd_state.local_host_IP_numbers[i] == addr) {
				return TRUE;
			}
		}

		return FALSE;
	} else {
		return TRUE;
	}
#endif							/* NO_INET6 */
}

static int alloc_channel(PTInstVar pvar, int new_status,
                         int new_request_num)
{
	int i;
	int new_num_channels;
	int new_channel = -1;
	FWDChannel FAR *channel;

	for (i = 0; i < pvar->fwd_state.num_channels && new_channel < 0; i++) {
		if (pvar->fwd_state.channels[i].status == 0) {
			new_channel = i;
		}
	}

	if (new_channel < 0) {
		new_num_channels = pvar->fwd_state.num_channels + 1;
		pvar->fwd_state.channels =
			(FWDChannel FAR *) realloc(pvar->fwd_state.channels,
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
	FWDChannel FAR *channel;

	for (i = 0; i < pvar->fwd_state.num_channels && new_channel < 0; i++) {
		if (pvar->fwd_state.channels[i].status == 0) {
			new_channel = i;
		}
	}

	if (new_channel < 0) {
		new_num_channels = pvar->fwd_state.num_channels + 1;
		pvar->fwd_state.channels =
			(FWDChannel FAR *) realloc(pvar->fwd_state.channels,
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
	FWDChannel FAR *channel = pvar->fwd_state.channels + channel_num;
	FWDRequest FAR *request =
		pvar->fwd_state.requests + channel->request_num;
#ifndef NO_INET6
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
#else
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons((unsigned short) request->spec.to_port);
	addr.sin_addr.s_addr = htonl(request->to_host_addr);

	if ((channel->local_socket =
	     socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET
	  && WSAAsyncSelect(channel->local_socket, make_accept_wnd(pvar),
	                    WM_SOCK_IO,
	                    FD_CONNECT | FD_READ | FD_CLOSE | FD_WRITE) !=
	     SOCKET_ERROR) {
		if (connect
		    (channel->local_socket, (struct sockaddr FAR *) &addr,
		     sizeof(addr)) != SOCKET_ERROR) {
			connected_local_connection(pvar, channel_num);
			return;
		} else if (WSAGetLastError() == WSAEWOULDBLOCK) {
			/* do nothing, we'll just wait */
			return;
		}
	}

	channel_opening_error(pvar, channel_num, WSAGetLastError());
#endif							/* NO_INET6 */
}

#ifndef NO_INET6
static void accept_local_connection(PTInstVar pvar, int request_num,
                                    int listening_socket_num)
{
#else
static void accept_local_connection(PTInstVar pvar, int request_num)
{
#endif							/* NO_INET6 */
	int channel_num;
	SOCKET s;
#ifndef NO_INET6
	struct sockaddr_storage addr;
	char hname[NI_MAXHOST];
	char strport[NI_MAXSERV]; // ws2tcpip.h
#else
	struct sockaddr addr;
#endif							/* NO_INET6 */
	int addrlen = sizeof(addr);
	char buf[1024];
#ifndef INET6
	BYTE FAR *IP;
#endif							/* NO_INET6 */
	FWDChannel FAR *channel;
	FWDRequest FAR *request = &pvar->fwd_state.requests[request_num];
	BOOL is_localhost = FALSE;

#ifndef NO_INET6
	s = accept(request->listening_sockets[listening_socket_num],
	           (struct sockaddr FAR *) &addr, &addrlen);
#else
	s = accept(request->listening_socket, &addr, &addrlen);
#endif							/* NO_INET6 */
	if (s == INVALID_SOCKET)
		return;

#ifndef INET6
	IP = (BYTE FAR *) & ((struct sockaddr_in *) (&addr))->sin_addr.s_addr;
#endif

#ifndef NO_INET6
	is_localhost = validate_IP_number(pvar, (struct sockaddr FAR *) &addr);
	if ((pvar->settings.LocalForwardingIdentityCheck && !is_localhost) ||
	    (request->spec.check_identity && !is_localhost)) {
		char hname[NI_MAXHOST];
		if (getnameinfo((struct sockaddr FAR *) &addr, addrlen,
		                hname, sizeof(hname), NULL, 0, NI_NUMERICHOST)) {
			/* NOT REACHED */
		}
		UTIL_get_lang_msg("MSG_FWD_HOSTILE_ATTACK_ERROR", pvar,
		                  "Host with IP number %s tried to connect to "
		                  "forwarded local port %d.\n"
		                  "This could be some kind of hostile attack.");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->ts->UIMsg, hname,
		            request->spec.from_port);
		notify_nonfatal_error(pvar, buf);
		safe_closesocket(pvar, s);
		return;
	}
#else
	if (!validate_IP_number
		(pvar,
		 ntohl(((struct sockaddr_in *) (&addr))->sin_addr.S_un.S_addr))) {
		_snprintf(buf, sizeof(buf),
		          "Host with IP number %d.%d.%d.%d tried to connect to "
		          "forwarded local port %d.\n"
		          "This could be some kind of hostile attack.", IP[0],
		          IP[1], IP[2], IP[3], request->spec.from_port);
		buf[NUM_ELEM(buf) - 1] = 0;
		notify_nonfatal_error(pvar, buf);
		safe_closesocket(pvar, s);
		return;
	}
#endif							/* NO_INET6 */

#ifndef NO_INET6
	// SSH2 port-forwardingに接続元のリモートポートが必要。(2005.2.27 yutaka)
	if (getnameinfo
	    ((struct sockaddr FAR *) &addr, addrlen, hname, sizeof(hname),
	     strport, sizeof(strport), NI_NUMERICHOST | NI_NUMERICSERV)) {
		/* NOT REACHED */
	}
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
	          "Host %s connecting to port %d; forwarding to %s:%d",
	          hname, request->spec.from_port, request->spec.to_host,
	          request->spec.to_port);
#else
	_snprintf(buf, sizeof(buf),
	          "Host %d.%d.%d.%d connecting to port %d; forwarding to %s:%d",
	          IP[0], IP[1], IP[2], IP[3], request->spec.from_port,
	          request->spec.to_host, request->spec.to_port);
	buf[NUM_ELEM(buf) - 1] = 0;
#endif							/* NO_INET6 */
	notify_verbose_message(pvar, buf, LOG_LEVEL_VERBOSE);

#ifndef NO_INET6
	strncpy_s(buf, sizeof(buf), hname, _TRUNCATE);
#else
	_snprintf(buf, sizeof(buf), "%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
	buf[NUM_ELEM(buf) - 1] = 0;
#endif

	channel_num = alloc_channel(pvar, FWD_LOCAL_CONNECTED, request_num);
	channel = pvar->fwd_state.channels + channel_num;

	channel->local_socket = s;
	channel->filter_closure = NULL;
	channel->filter = NULL;

	// add originator-port (2005.2.27 yutaka)
	SSH_open_channel(pvar, channel_num, request->spec.to_host,
	                 request->spec.to_port, buf, atoi(strport));
}

static void write_local_connection_buffer(PTInstVar pvar, int channel_num)
{
	FWDChannel FAR *channel = pvar->fwd_state.channels + channel_num;

	if ((channel->status & (FWD_REMOTE_CONNECTED | FWD_LOCAL_CONNECTED))
	 == (FWD_REMOTE_CONNECTED | FWD_LOCAL_CONNECTED)) {
		if (!UTIL_sock_write_more
			(pvar, &channel->writebuf, channel->local_socket)) {
			channel_error(pvar, "writing", channel_num, WSAGetLastError());
		}
	}
}

static void read_local_connection(PTInstVar pvar, int channel_num)
{
	FWDChannel FAR *channel = pvar->fwd_state.channels + channel_num;

	if ((channel->status & (FWD_REMOTE_CONNECTED | FWD_LOCAL_CONNECTED))
	 != (FWD_REMOTE_CONNECTED | FWD_LOCAL_CONNECTED)) {
		return;
	}

	while (channel->local_socket != INVALID_SOCKET) {
		char buf[CHANNEL_READ_BUF_SIZE];
		int amount = recv(channel->local_socket, buf, sizeof(buf), 0);
		int err;

		if (amount > 0) {
			char FAR *new_buf = buf;
			int action = FWD_FILTER_RETAIN;

			if (channel->filter != NULL) {
				action =
					channel->filter(channel->filter_closure,
					                FWD_FILTER_FROM_CLIENT, &amount,
					                &new_buf);
			}

			if (amount > 0
			 && (channel->status & FWD_CLOSED_REMOTE_OUT) == 0) {
				// ポートフォワーディングにおいてクライアントからの送信要求を、SSH通信に乗せてサーバまで送り届ける。
				SSH_channel_send(pvar, channel_num, channel->remote_num, new_buf,
				                 amount);
			}

			switch (action) {
			case FWD_FILTER_REMOVE:
				channel->filter(channel->filter_closure, 0, NULL, NULL);
				channel->filter = NULL;
				channel->filter_closure = NULL;
				break;
			case FWD_FILTER_CLOSECHANNEL:
				closed_local_connection(pvar, channel_num);
				break;
			}
		} else if (amount == 0
		        || (err = WSAGetLastError()) == WSAEWOULDBLOCK) {
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

#ifdef NO_INET6
	pvar->fwd_state.requests[request_num].to_host_addr =
		ntohl(*(uint32 FAR *)
			  ((HOSTENT FAR *) pvar->fwd_state.requests[request_num].
			   to_host_hostent_buf)->h_addr_list[0]);
#endif							/* NO_INET6 */

	for (i = 0; i < pvar->fwd_state.num_channels; i++) {
		if (pvar->fwd_state.channels[i].request_num == request_num) {
			make_local_connection(pvar, i);
		}
	}
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
#ifndef NO_INET6
				int listening_socket_num;
#endif							/* NO_INET6 */
				switch (LOWORD(lParam)) {
				case FD_ACCEPT:
#ifndef NO_INET6
					listening_socket_num =
						find_listening_socket_num(pvar, request_num,
						                          (SOCKET) wParam);
					if (listening_socket_num == -1)
						return FALSE;
					accept_local_connection(pvar, request_num,
					                        listening_socket_num);
#else
					accept_local_connection(pvar, request_num);
#endif							/* NO_INET6 */
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
#ifdef NO_INET6
			free(pvar->fwd_state.requests[request_num].
			     to_host_hostent_buf);
			pvar->fwd_state.requests[request_num].to_host_hostent_buf =
				NULL;
#endif							/* NO_INET6 */
			return TRUE;
		}

	case WM_SOCK_IO:{
			int channel_num = find_channel_num(pvar, (SOCKET) wParam);
#ifndef NO_INET6
			FWDChannel FAR *channel =
				pvar->fwd_state.channels + channel_num;
#endif							/* NO_INET6 */

			if (channel_num < 0)
				return TRUE;

			if (HIWORD(lParam) != 0) {
#ifndef NO_INET6
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
#else
				if (LOWORD(lParam) == FD_CONNECT) {
					channel_opening_error(pvar, channel_num,
					                      HIWORD(lParam));
				} else {
					channel_error(pvar, "accessing", channel_num,
					              HIWORD(lParam));
				}
#endif							/* NO_INET6 */
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

int FWD_compare_specs(void const FAR * void_spec1,
                      void const FAR * void_spec2)
{
	FWDRequestSpec FAR *spec1 = (FWDRequestSpec FAR *) void_spec1;
	FWDRequestSpec FAR *spec2 = (FWDRequestSpec FAR *) void_spec2;
	int delta = spec1->from_port - spec2->from_port;

	if (delta == 0) {
		delta = spec1->type - spec2->type;
	}

	return delta;
}

/* Check if the server can listen for 'spec' using the old listening spec 'listener'.
   We check that the to_port and (for non-X connections) the to_host are equal
   so that we never lie to the server about where its forwarded connection is
   ending up. Maybe some SSH implementation depends on this information being
   reliable, for security? */
static BOOL can_server_listen_using(FWDRequestSpec FAR * listener,
                                    FWDRequestSpec FAR * spec)
{
	return listener->type == spec->type
	    && listener->from_port == spec->from_port
	    && listener->to_port == spec->to_port
	    && (spec->type == FWD_REMOTE_X11_TO_LOCAL
	     || strcmp(listener->to_host, spec->to_host) == 0);
}

BOOL FWD_can_server_listen_for(PTInstVar pvar, FWDRequestSpec FAR * spec)
{
	int num_server_listening_requests =
		pvar->fwd_state.num_server_listening_specs;

	if (num_server_listening_requests < 0) {
		return TRUE;
	} else {
		FWDRequestSpec FAR *listener =
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

void FWD_get_request_specs(PTInstVar pvar, FWDRequestSpec FAR * specs,
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
#ifndef NO_INET6
static SOCKET FAR *delete_request(PTInstVar pvar, int request_num,
                                  int *p_num_listening_sockets)
{
#else
static SOCKET delete_request(PTInstVar pvar, int request_num)
{
#endif							/* NO_INET6 */
	FWDRequest FAR *request = pvar->fwd_state.requests + request_num;
#ifndef NO_INET6
	SOCKET FAR *lp_listening_sockets;
#else
	SOCKET result = INVALID_SOCKET;
#endif							/* NO_INET6 */

/* safe to shut down the listening socket here. Any pending connections
   that haven't yet been turned into channels will be broken, but that's
   just tough luck. */
#ifndef NO_INET6
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
#else
	if (request->listening_socket != INVALID_SOCKET) {
		result = request->listening_socket;
		request->listening_socket = INVALID_SOCKET;
	}

	request->status |= FWD_DELETED;

	if (request->num_channels == 0) {
		really_delete_request(pvar, request_num);
	}

	return result;
#endif							/* NO_INET6 */
}

static BOOL are_specs_identical(FWDRequestSpec FAR * spec1,
                                FWDRequestSpec FAR * spec2)
{
	return spec1->type == spec2->type
	    && spec1->from_port == spec2->from_port
	    && spec1->to_port == spec2->to_port
	    && strcmp(spec1->to_host, spec2->to_host) == 0
	    && spec1->check_identity == spec2->check_identity;
}

static BOOL interactive_init_request(PTInstVar pvar, int request_num,
                                     BOOL report_error)
{
	FWDRequest FAR *request = pvar->fwd_state.requests + request_num;

	if (request->spec.type == FWD_LOCAL_TO_REMOTE) {
#ifndef NO_INET6
		struct addrinfo hints;
		struct addrinfo FAR *res;
		struct addrinfo FAR *res0;
		SOCKET s;
		char pname[NI_MAXSERV];

		_snprintf_s(pname, sizeof(pname), _TRUNCATE,
		            "%d", request->spec.from_port);
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;	/* a user will be able to specify protocol in future version */
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(NULL, pname, &hints, &res0))
			return FALSE;

		/* count number of listening sockets and allocate area for them */
		for (request->num_listening_sockets = 0, res = res0; res;
		     res = res->ai_next)
			request->num_listening_sockets++;
		request->listening_sockets =
			(SOCKET FAR *) malloc(sizeof(SOCKET) *
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
			if (WSAAsyncSelect
				(s, make_accept_wnd(pvar), WM_SOCK_ACCEPT,
				 FD_ACCEPT | FD_READ | FD_CLOSE | FD_WRITE
				) == SOCKET_ERROR) {
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
				char buf[256];
				UTIL_get_lang_msg("MSG_FWD_SOCKET_ERROR", pvar,
				                  "Some socket(s) required for port forwarding could not be initialized.\n"
				                  "Some port forwarding services may not be available.\n"
				                  "(errno %d)");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				            pvar->ts->UIMsg, WSAGetLastError());
				notify_verbose_message(pvar,buf,LOG_LEVEL_WARNING);
			}
			freeaddrinfo(res0);
			/* free(request->listening_sockets); /* DO NOT FREE HERE, listening_sockets'll be freed in FWD_end */
			return FALSE;
		}
		freeaddrinfo(res0);
	}

	return TRUE;
#else
		SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr;

		addr.sin_family = AF_INET;
		addr.sin_port = htons((unsigned short) request->spec.from_port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		request->listening_socket = s;
		if (s == INVALID_SOCKET
			|| WSAAsyncSelect(s, make_accept_wnd(pvar), WM_SOCK_ACCEPT,
							  FD_ACCEPT | FD_READ | FD_CLOSE | FD_WRITE) ==
			SOCKET_ERROR
			|| bind(s, (struct sockaddr FAR *) &addr,
					sizeof(addr)) == SOCKET_ERROR
			|| listen(s, SOMAXCONN) == SOCKET_ERROR) {
			if (report_error) {
				notify_nonfatal_error(pvar,
									  "Some socket(s) required for port forwarding could not be initialized.\n"
									  "Some port forwarding services may not be available.");
			}
			return FALSE;
		}
	}

	return TRUE;
#endif							/* NO_INET6 */
}

/* This function will only be called on a request when all its channels are
   closed. */
#ifndef NO_INET6
static BOOL init_request(PTInstVar pvar, int request_num,
                         BOOL report_error, SOCKET FAR * listening_sockets,
                         int num_listening_sockets)
{
#else
static BOOL init_request(PTInstVar pvar, int request_num,
						 BOOL report_error, SOCKET listening_socket)
{
#endif							/* NO_INET6 */
	FWDRequest FAR *request = pvar->fwd_state.requests + request_num;

#ifndef NO_INET6
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
			return interactive_init_request(pvar, request_num,
											report_error);
		}
	} else {
		assert(listening_sockets == NULL);
		return TRUE;
	}
#else
	request->listening_socket = INVALID_SOCKET;
	request->to_host_addr = 0;
	request->to_host_hostent_buf = NULL;
	request->to_host_lookup_handle = 0;
	request->status = 0;

	if (pvar->fwd_state.in_interactive_mode) {
		if (listening_socket != INVALID_SOCKET) {
			request->listening_socket = listening_socket;
			return TRUE;
		} else {
			return interactive_init_request(pvar, request_num,
											report_error);
		}
	} else {
		assert(listening_socket == INVALID_SOCKET);
		return TRUE;
	}
#endif							/* NO_INET6 */
}

void FWD_set_request_specs(PTInstVar pvar, FWDRequestSpec FAR * specs,
                           int num_specs)
{
	FWDRequestSpec FAR *new_specs =
		(FWDRequestSpec FAR *) malloc(sizeof(FWDRequestSpec) * num_specs);
	char FAR *specs_accounted_for;
#ifndef NO_INET6
	typedef struct _saved_sockets {
		SOCKET FAR *listening_sockets;
		int num_listening_sockets;
	} saved_sockets_t;
	saved_sockets_t FAR *ptr_to_saved_sockets;
#else
	SOCKET FAR *saved_sockets;
#endif							/* NO_INET6 */
	int i;
	int num_new_requests = num_specs;
	int num_free_requests = 0;
	int free_request = 0;
	BOOL report_err = TRUE;

	memcpy(new_specs, specs, sizeof(FWDRequestSpec) * num_specs);
	qsort(new_specs, num_specs, sizeof(FWDRequestSpec), FWD_compare_specs);

	for (i = 0; i < num_specs - 1; i++) {
		if (FWD_compare_specs(new_specs + i, new_specs + i + 1) == 0) {
			UTIL_get_lang_msg("MSG_FWD_DUPLICATE_ERROR", pvar,
			                  "TTSSH INTERNAL ERROR: Could not set port forwards because duplicate type/port requests were found");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			free(new_specs);
			return;
		}
	}

	specs_accounted_for = (char FAR *) malloc(sizeof(char) * num_specs);
#ifndef NO_INET6
	ptr_to_saved_sockets =
		(saved_sockets_t FAR *) malloc(sizeof(saved_sockets_t) *
		                               num_specs);
#else
	saved_sockets = (SOCKET FAR *) malloc(sizeof(SOCKET) * num_specs);
#endif							/* NO_INET6 */

	memset(specs_accounted_for, 0, num_specs);
	for (i = 0; i < num_specs; i++) {
#ifndef NO_INET6
		ptr_to_saved_sockets[i].listening_sockets = NULL;
		ptr_to_saved_sockets[i].num_listening_sockets = 0;
#else
		saved_sockets[i] = INVALID_SOCKET;
#endif							/* NO_INET6 */
	}

	for (i = pvar->fwd_state.num_requests - 1; i >= 0; i--) {
		if ((pvar->fwd_state.requests[i].status & FWD_DELETED) == 0) {
			FWDRequestSpec FAR *cur_spec =
				&pvar->fwd_state.requests[i].spec;
			FWDRequestSpec FAR *new_spec =
				bsearch(cur_spec, new_specs, num_specs,
				        sizeof(FWDRequestSpec), FWD_compare_specs);

			if (new_spec != NULL
			 && are_specs_identical(cur_spec, new_spec)) {
				specs_accounted_for[new_spec - new_specs] = 1;
				num_new_requests--;
			} else {
#ifndef NO_INET6
				int num_listening_sockets;
				SOCKET FAR *listening_sockets;
				listening_sockets =
					delete_request(pvar, i, &num_listening_sockets);
#else
				SOCKET listening_socket = delete_request(pvar, i);
#endif							/* NO_INET6 */

#ifndef NO_INET6
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
#else
				if (new_spec != NULL) {
					saved_sockets[new_spec - new_specs] = listening_socket;
				} else if (listening_socket != INVALID_SOCKET) {
					safe_closesocket(pvar, listening_socket);
				}
#endif							/* NO_INET6 */

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
			(FWDRequest FAR *) realloc(pvar->fwd_state.requests,
			                           sizeof(FWDRequest) *
			                           total_requests);
		for (i = pvar->fwd_state.num_requests; i < total_requests; i++) {
			pvar->fwd_state.requests[i].status = FWD_DELETED;
			pvar->fwd_state.requests[i].num_channels = 0;
		}
		pvar->fwd_state.num_requests = total_requests;
	}

	for (i = 0; i < num_specs; i++) {
		if (!specs_accounted_for[i]) {
			while ((pvar->fwd_state.requests[free_request].status & FWD_DELETED) == 0
			    || pvar->fwd_state.requests[free_request].num_channels != 0) {
				free_request++;
			}

			assert(free_request < pvar->fwd_state.num_requests);

			pvar->fwd_state.requests[free_request].spec = new_specs[i];
#ifndef NO_INET6
			if (!init_request(pvar, free_request, report_err,
			                  ptr_to_saved_sockets[i].listening_sockets,
			                  ptr_to_saved_sockets[i].num_listening_sockets)) {
#else
			if (!init_request
				(pvar, free_request, report_err, saved_sockets[i])) {
#endif							/* NO_INET6 */
				report_err = FALSE;
			}

			free_request++;
		}
	}

#ifndef NO_INET6
	free(ptr_to_saved_sockets);
#else
	free(saved_sockets);
#endif							/* NO_INET6 */
	free(specs_accounted_for);
	free(new_specs);
}

void FWD_prep_forwarding(PTInstVar pvar)
{
	int i;
	int num_server_listening_requests = 0;

	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		FWDRequest FAR *request = pvar->fwd_state.requests + i;

		if ((request->status & FWD_DELETED) == 0) {
			switch (request->spec.type) {
			case FWD_REMOTE_TO_LOCAL:
				SSH_request_forwarding(pvar, request->spec.from_port,
				                       request->spec.to_host,
				                       request->spec.to_port);
				num_server_listening_requests++;
				break;
			case FWD_REMOTE_X11_TO_LOCAL:{
					int screen_num = request->spec.to_port - 6000;

					pvar->fwd_state.X11_auth_data =
						X11_load_local_auth_data(screen_num);
					SSH_request_X11_forwarding(pvar,
					                           X11_get_spoofed_protocol_name
					                           (pvar->fwd_state.X11_auth_data),
					                           X11_get_spoofed_protocol_data
					                           (pvar->fwd_state.X11_auth_data),
					                           X11_get_spoofed_protocol_data_len
					                           (pvar->fwd_state.X11_auth_data),
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
		FWDRequestSpec FAR *server_listening_requests =
			(FWDRequestSpec FAR *) malloc(sizeof(FWDRequestSpec) *
			                              num_server_listening_requests);

		pvar->fwd_state.server_listening_specs = server_listening_requests;

		for (i = 0; i < pvar->fwd_state.num_requests; i++) {
			if ((pvar->fwd_state.requests[i].status & FWD_DELETED) == 0) {
				switch (pvar->fwd_state.requests[i].spec.type) {
				case FWD_REMOTE_X11_TO_LOCAL:
				case FWD_REMOTE_TO_LOCAL:
					*server_listening_requests =
						pvar->fwd_state.requests[i].spec;
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
	FWDChannel FAR *channel;
	FWDRequest FAR *request = pvar->fwd_state.requests + request_num;
#ifndef NO_INET6
	struct addrinfo hints;
	char pname[NI_MAXSERV];
#endif							/* NO_INET6 */

#ifndef NO_INET6
	if (request->to_host_addrs == NULL
	 && request->to_host_lookup_handle == 0) {
		HANDLE task_handle;
#else
	if (request->to_host_addr == 0 && request->to_host_lookup_handle == 0) {
		HANDLE task_handle;
#endif							/* NO_INET6 */

#ifdef NO_INET6
		if (request->to_host_hostent_buf == NULL) {
			request->to_host_hostent_buf =
				(char FAR *) malloc(MAXGETHOSTSTRUCT);
		}
#endif							/* NO_INET6 */

#ifndef NO_INET6
		_snprintf_s(pname, sizeof(pname), _TRUNCATE, "%d", request->spec.to_port);
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		task_handle =
			WSAAsyncGetAddrInfo(make_accept_wnd(pvar), WM_SOCK_GOTNAME,
			                    request->spec.to_host, pname, &hints,
			                    &request->to_host_addrs);
#else
		task_handle =
			WSAAsyncGetHostByName(make_accept_wnd(pvar), WM_SOCK_GOTNAME,
								  request->spec.to_host,
								  request->to_host_hostent_buf,
								  MAXGETHOSTSTRUCT);
#endif							/* NO_INET6 */

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
#ifndef NO_INET6
	  /*****/
			freeaddrinfo(request->to_host_addrs);
#else
			free(request->to_host_hostent_buf);
			request->to_host_hostent_buf = NULL;
#endif							/* NO_INET6 */
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

#ifndef NO_INET6
	if (request->to_host_addrs != NULL) {
		make_local_connection(pvar, channel_num);
	}
#else
	if (request->to_host_addr != 0) {
		make_local_connection(pvar, channel_num);
	}
#endif							/* NO_INET6 */
}

void FWD_open(PTInstVar pvar, uint32 remote_channel_num,
              char FAR * local_hostname, int local_port,
              char FAR * originator, int originator_len,
              int *chan_num)
{
	int i;
	char buf[1024];

	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		FWDRequest FAR *request = pvar->fwd_state.requests + i;

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
		FWDRequestSpec FAR *spec =
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
                  char FAR * originator, int originator_len,
                  int *chan_num)
{
	int i;

	for (i = 0; i < pvar->fwd_state.num_requests; i++) {
		FWDRequest FAR *request = pvar->fwd_state.requests + i;

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
		FWDRequestSpec FAR *spec =
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
	FWDChannel FAR *channel;

	if (!FWD_check_local_channel_num(pvar, local_channel_num))
		return;

	channel = pvar->fwd_state.channels + local_channel_num;
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

void FWD_failed_open(PTInstVar pvar, uint32 local_channel_num)
{
	if (!FWD_check_local_channel_num(pvar, local_channel_num))
		return;

	UTIL_get_lang_msg("MSG_FWD_DENIED_BY_SERVER_ERROR", pvar,
	                  "A program on the local machine attempted to connect to a forwarded port.\n"
	                  "The forwarding request was denied by the server. The connection has been closed.");
	notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	FWD_free_channel(pvar, local_channel_num);
}

static BOOL blocking_write(PTInstVar pvar, SOCKET s, const char FAR * data,
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
                       unsigned char FAR * data, int length)
{
	SOCKET s;
	FWDChannel FAR *channel;
	int action = FWD_FILTER_RETAIN;

	if (!FWD_check_local_channel_num(pvar, local_channel_num))
		return;

	channel = pvar->fwd_state.channels + local_channel_num;

	if (channel->filter != NULL) {
		action =
			channel->filter(channel->filter_closure,
			                FWD_FILTER_FROM_SERVER, &length, &data);
	}

	s = channel->local_socket;
	if (length > 0 && s != INVALID_SOCKET) {
		if (!UTIL_sock_buffered_write
			(pvar, &channel->writebuf, blocking_write, s, data, length)) {
			closed_local_connection(pvar, local_channel_num);
			UTIL_get_lang_msg("MSG_FWD_COMM_ERROR", pvar,
			                  "A communications error occurred while sending forwarded data to a local port.\n"
			                  "The forwarded connection will be closed.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		}
	}

	switch (action) {
	case FWD_FILTER_REMOVE:
		channel->filter(channel->filter_closure, 0, NULL, NULL);
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
	pvar->fwd_state.local_host_IP_numbers = NULL;
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
#ifndef NO_INET6
	if (pvar->fwd_state.requests != NULL) {
		for (i = 0; i < pvar->fwd_state.num_requests; i++) {
			int j;
			int num_listening_sockets;
			SOCKET FAR *s =
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
#else
	if (pvar->fwd_state.requests != NULL) {
		for (i = 0; i < pvar->fwd_state.num_requests; i++) {
			SOCKET s = delete_request(pvar, i);

			if (s != INVALID_SOCKET) {
				closesocket(s);
			}
		}
		free(pvar->fwd_state.requests);
	}
#endif							/* NO_INET6 */

	free(pvar->fwd_state.local_host_IP_numbers);
	free(pvar->fwd_state.server_listening_specs);

	if (pvar->fwd_state.X11_auth_data != NULL) {
		X11_dispose_auth_data(pvar->fwd_state.X11_auth_data);
	}

	if (pvar->fwd_state.accept_wnd != NULL) {
		DestroyWindow(pvar->fwd_state.accept_wnd);
	}
}
