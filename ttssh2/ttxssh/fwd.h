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

#ifndef __FWD_H
#define __FWD_H

#define FWD_REMOTE_CONNECTED  0x01
#define FWD_LOCAL_CONNECTED   0x02
#define FWD_CLOSED_REMOTE_IN  0x04
#define FWD_CLOSED_REMOTE_OUT 0x08
#define FWD_CLOSED_LOCAL_IN   0x10
#define FWD_CLOSED_LOCAL_OUT  0x20
#define FWD_AGENT_DUMMY       0x40

#define FWD_FILTER_REMOVE       0
#define FWD_FILTER_RETAIN       1
#define FWD_FILTER_CLOSECHANNEL 2

#define FWD_FILTER_FROM_CLIENT 0
#define FWD_FILTER_FROM_SERVER 1

/* a length == 0 means that we're killing the channel and the filter
   should deallocate */
typedef int (* FWDFilter)(void FAR * closure, int direction,
  int FAR * length, unsigned char FAR * FAR * buf);

typedef struct {
  int status;
  int remote_num;
  SOCKET local_socket;
  int request_num;
  UTILSockWriteBuf writebuf;

  void FAR * filter_closure;
  FWDFilter filter;
#ifndef NO_INET6
  struct addrinfo FAR * to_host_addrs;
#endif /* NO_INET6 */

  // for agent forwarding
  buffer_t *agent_msg;
  int agent_request_len;
  enum channel_type type;
} FWDChannel;

/* Request types */
#define FWD_LOCAL_TO_REMOTE              1
#define FWD_REMOTE_TO_LOCAL              2
#define FWD_REMOTE_X11_TO_LOCAL          3

/* If 'type' is FWD_REMOTE_X11_TO_LOCAL, then from_port must be
   -1, to_port must be 6000 + screen number, and to_host must
   be the desired X server host.

   There can be no more than one spec of a given type and from_port
   at one time.
*/
typedef struct {
  int type;
  int from_port;
  char from_port_name[32];
  int to_port;
  char to_port_name[32];
  char to_host[256];
  BOOL check_identity;
} FWDRequestSpec;

#define FWD_DELETED                      0x01

#ifndef NO_INET6
#define MAX_LISTENING_SOCKETS 4096
#endif
typedef struct {
#ifndef NO_INET6
  int num_listening_sockets;
  SOCKET FAR * listening_sockets;
  struct addrinfo FAR * to_host_addrs;
#else
  SOCKET listening_socket;

  uint32 to_host_addr;
  char FAR * to_host_hostent_buf;
#endif /* NO_INET6 */
  HANDLE to_host_lookup_handle;

  int num_channels;

  int status;
  FWDRequestSpec spec;
} FWDRequest;

typedef struct {
  HWND accept_wnd;
  WNDPROC old_accept_wnd_proc;
  int num_server_listening_specs;
  /* stored in sorted order */
  FWDRequestSpec FAR * server_listening_specs;
  int num_requests;
  FWDRequest FAR * requests;
  int num_channels;
  FWDChannel FAR * channels;
#ifndef NO_INET6
  struct sockaddr_storage FAR * local_host_IP_numbers;
#else
  uint32 * local_host_IP_numbers;
#endif /* NO_INET6 */
  struct _X11AuthData FAR * X11_auth_data;
  BOOL in_interactive_mode;
} FWDState;

void FWD_init(PTInstVar pvar);
/* This function checks to see whether the server is (or can) listen for a
   given request. Before the SSH session's prep phase, this returns true for
   all requests. After the SSH session's prep phase, this returns true only
   for requests that the server actually was told about during the prep phase. */
BOOL FWD_can_server_listen_for(PTInstVar pvar, FWDRequestSpec FAR * spec);
int FWD_get_num_request_specs(PTInstVar pvar);
void FWD_get_request_specs(PTInstVar pvar, FWDRequestSpec FAR * specs, int num_specs);
void FWD_set_request_specs(PTInstVar pvar, FWDRequestSpec FAR * specs, int num_specs);
int FWD_compare_specs(void const FAR * void_spec1, void const FAR * void_spec2);
void FWD_prep_forwarding(PTInstVar pvar);
void FWD_enter_interactive_mode(PTInstVar pvar);
void FWD_open(PTInstVar pvar, uint32 remote_channel_num,
			  char FAR * local_hostname, int local_port,
			  char FAR * originator, int originator_len,
			  int *chan_num);
void FWD_X11_open(PTInstVar pvar, uint32 remote_channel_num,
				  char FAR * originator, int originator_len,
				  int *chan_num);
void FWD_confirmed_open(PTInstVar pvar, uint32 local_channel_num,
  uint32 remote_channel_num);
void FWD_failed_open(PTInstVar pvar, uint32 local_channel_num);
void FWD_received_data(PTInstVar pvar, uint32 local_channel_num,
  unsigned char FAR * data, int length);
void FWD_channel_input_eof(PTInstVar pvar, uint32 local_channel_num);
void FWD_channel_output_eof(PTInstVar pvar, uint32 local_channel_num);
void FWD_end(PTInstVar pvar);
void FWD_free_channel(PTInstVar pvar, uint32 local_channel_num);
int FWD_check_local_channel_num(PTInstVar pvar, int local_num);
int FWD_agent_open(PTInstVar pvar, uint32 remote_channel_num);

#endif
