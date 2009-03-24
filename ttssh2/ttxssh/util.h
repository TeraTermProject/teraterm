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

#ifndef __UTIL_H
#define __UTIL_H

typedef unsigned long uint32;
typedef unsigned short uint16;

#define NUM_ELEM(a) (sizeof(a) / sizeof((a)[0]))

#define buf_create(buf, len) (*(buf) = 0, *(len) = 0)

#define buf_ensure_size(buf, len, desired) \
  (*(len) < (desired) ? (*(buf) = realloc(*(buf), (desired)), *(len) = (desired)) : *(len))

#define buf_ensure_size_growing(buf, len, desired) \
  (*(len) < (desired) ? (*(buf) = realloc(*(buf), (desired)*2), *(len) = (desired)*2) : *(len))

#define buf_destroy(buf, len) (free(*(buf)), *(buf) = 0, *(len) = 0)

#define get_uint32_MSBfirst(buf) \
  (((uint32)(unsigned char)(buf)[0] << 24) + ((uint32)(unsigned char)(buf)[1] << 16) + \
  ((uint32)(unsigned char)(buf)[2] << 8) + (uint32)(unsigned char)(buf)[3])

#define get_ushort16_MSBfirst(buf) \
  (((uint32)(unsigned char)(buf)[0] << 8) + (uint32)(unsigned char)(buf)[1])

#define set_ushort16_MSBfirst(buf, v) \
  ((buf)[0] = (unsigned char)((uint16)(v) >> 8),     \
   (buf)[1] = (unsigned char)(uint16)(v))

#define set_uint32_MSBfirst(buf, v) \
  ((buf)[0] = (unsigned char)((uint32)(v) >> 24),     \
   (buf)[1] = (unsigned char)((uint32)(v) >> 16),     \
   (buf)[2] = (unsigned char)((uint32)(v) >> 8),      \
   (buf)[3] = (unsigned char)(uint32)(v))

typedef struct _UTILSockWriteBuf {
  char FAR * bufdata;
  int buflen;
  int datastart;
  int datalen;
} UTILSockWriteBuf;

typedef BOOL (* UTILBlockingWriteCallback)(PTInstVar pvar,
  SOCKET socket, const char FAR * data, int len);

void UTIL_init_sock_write_buf(UTILSockWriteBuf FAR * buf);
BOOL UTIL_sock_buffered_write(PTInstVar pvar, UTILSockWriteBuf FAR * buf,
  UTILBlockingWriteCallback blocking_write, SOCKET socket, const char FAR * data, int len);
BOOL UTIL_sock_write_more(PTInstVar pvar, UTILSockWriteBuf FAR * buf, SOCKET socket);
void UTIL_destroy_sock_write_buf(UTILSockWriteBuf FAR * buf);
BOOL UTIL_is_sock_deeply_buffered(UTILSockWriteBuf FAR * buf);

void UTIL_get_lang_msg(PCHAR key, PTInstVar pvar, PCHAR def);
int UTIL_get_lang_font(PCHAR key, HWND dlg, PLOGFONT logfont, HFONT *font, PTInstVar pvar);

BOOL is_NT4();

#endif
