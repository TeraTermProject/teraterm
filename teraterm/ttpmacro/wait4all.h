/*
 * Copyright (C) 2009-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*
 * wait4all.h
 *
 */
#ifndef _WAIT4ALL_H_
#define _WAIT4ALL_H_

#ifdef __cplusplus
extern "C" {
#endif

// TODO: ˆÈ‰º‚Ì’è‹`‚Í‹¤’Êƒwƒbƒ_‚©‚çinclude‚·‚×‚«
#undef MAXNWIN
#define MAXNWIN 256
#undef RingBufSize
#define RingBufSize (4096*4)

int is_wait4all_enabled(void);
int register_macro_window(HWND hwnd);
int unregister_macro_window(HWND hwnd);
void get_macro_active_info(int *num, int *index);
int get_macro_active_num(void);
void put_macro_1byte(BYTE b);
int read_macro_1byte(int index, LPBYTE b);

extern int macro_shmem_index;

#ifdef __cplusplus
}
#endif

#endif

