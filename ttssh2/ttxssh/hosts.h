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

#ifndef __HOSTS_H
#define __HOSTS_H

typedef struct {
  char *prefetched_hostname;

#if 0
  int key_bits;
  /* The key exponent and modulus, in SSH mp_int format */
  unsigned char *key_exp;
  unsigned char *key_mod;
#else
  // ホストキー(SSH1,SSH2)は Key 構造体に集約する
  Key hostkey;
#endif

  int file_num;
  char **file_names;
  int file_data_index;
  char *file_data;  // known_hostsファイルの内容がすべて格納される

  HWND hosts_dialog;
} HOSTSState;

typedef int hostkeys_foreach_fn(Key *key, void *ctx);

void HOSTS_init(PTInstVar pvar);
void HOSTS_open(PTInstVar pvar);
void HOSTS_prefetch_host_key(PTInstVar pvar, char *hostname, unsigned short tcpport);
#if 0
BOOL HOSTS_check_host_key(PTInstVar pvar, char *hostname,
                         int bits, unsigned char *exp, unsigned char *mod);
#else
BOOL HOSTS_check_host_key(PTInstVar pvar, char *hostname, unsigned short tcpport, Key *key);
#endif
void HOSTS_do_unknown_host_dialog(HWND wnd, PTInstVar pvar);
void HOSTS_do_different_key_dialog(HWND wnd, PTInstVar pvar);
void HOSTS_do_different_type_key_dialog(HWND wnd, PTInstVar pvar);
void HOSTS_notify_disconnecting(PTInstVar pvar);
void HOSTS_notify_closing_on_exit(PTInstVar pvar);
void HOSTS_end(PTInstVar pvar);

int HOSTS_compare_public_key(Key *src, Key *key);
int HOSTS_hostkey_foreach(PTInstVar pvar, hostkeys_foreach_fn *callback, void *ctx);
void HOSTS_add_host_key(PTInstVar pvar, Key *key);
void HOSTS_delete_all_hostkeys(PTInstVar pvar);

#endif
