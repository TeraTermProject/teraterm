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

#ifndef __X11UTIL_H
#define __X11UTIL_H

#include "ttxssh.h"

typedef struct _X11AuthData {
  char FAR * local_protocol;
  unsigned char FAR * local_data;
  int local_data_len;
  char FAR * spoofed_protocol;
  unsigned char FAR * spoofed_data;
  int spoofed_data_len;
} X11AuthData;

#define X11_get_spoofed_protocol_name(d) ((d)->spoofed_protocol)
#define X11_get_spoofed_protocol_data(d) ((d)->spoofed_data)
#define X11_get_spoofed_protocol_data_len(d) ((d)->spoofed_data_len)

void X11_get_DISPLAY_info(char FAR * name_buf, int name_buf_len, int FAR * port);
X11AuthData FAR * X11_load_local_auth_data(int screen_num);
void FAR * X11_init_unspoofing_filter(struct _TInstVar FAR * pvar,
  X11AuthData FAR * auth_data);
int X11_unspoofing_filter(void FAR * closure, int direction,
  int FAR * length, unsigned char FAR * FAR * buf);
void X11_dispose_auth_data(X11AuthData FAR * auth_data);

#endif
