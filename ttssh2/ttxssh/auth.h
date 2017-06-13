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

#ifndef __AUTH_H
#define __AUTH_H

#include <stdio.h>

typedef struct {
  SSHAuthMethod method;
  char *password;
  char *rhosts_client_user;
  struct Key *key_pair;
} AUTHCred;

typedef enum { GENERIC_AUTH_MODE, TIS_AUTH_MODE } AuthMode;

typedef struct {
  char *user;
  AUTHCred cur_cred;
  SSHAuthMethod failed_method;
  int flags;
  int supported_types;
  HWND auth_dialog;

  AuthMode mode;
  char *TIS_prompt;
} AUTHState;

void AUTH_init(PTInstVar pvar);
char *AUTH_get_user_name(PTInstVar pvar);
int AUTH_set_supported_auth_types(PTInstVar pvar, int types);
void AUTH_set_generic_mode(PTInstVar pvar);
void AUTH_set_TIS_mode(PTInstVar pvar, char *prompt, int len);
void AUTH_advance_to_next_cred(PTInstVar pvar);
void AUTH_do_cred_dialog(PTInstVar pvar);
void AUTH_do_default_cred_dialog(PTInstVar pvar);
void AUTH_destroy_cur_cred(PTInstVar pvar);
void AUTH_get_auth_info(PTInstVar pvar, char *dest, int len);
void AUTH_notify_disconnecting(PTInstVar pvar);
void AUTH_notify_end_error(PTInstVar pvar);
void AUTH_end(PTInstVar pvar);
void destroy_malloced_string(char **str);
LRESULT CALLBACK password_wnd_proc(HWND control, UINT msg,
                                   WPARAM wParam, LPARAM lParam);

#define AUTH_get_cur_cred(pvar) (&(pvar)->auth_state.cur_cred)

#endif
