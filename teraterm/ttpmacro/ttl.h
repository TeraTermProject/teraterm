/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2019 TeraTerm Project
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

/* TTMACRO.EXE, Tera Term Language interpreter */

#define IdTimeOutTimer 1
#define TIMEOUT_TIMER_MS 50

// wakeup condition for sleep state
#define IdWakeupTimeout 1
#define IdWakeupUnlink  2
#define IdWakeupDisconn 4
#define IdWakeupConnect 8
/* connection trial state */
#define IdWakeupInit    16

#ifdef __cplusplus
extern "C" {
#endif

BOOL InitTTL(HWND HWin);
void EndTTL();
void Exec();
void SetMatchStr(PCHAR Str);
void SetGroupMatchStr(int no, const char *Str);
void SetInputStr(const char *Str);
void SetResult(int ResultCode);
BOOL CheckTimeout();
BOOL TestWakeup(int Wakeup);
void SetWakeup(int Wakeup);

// exit code of TTMACRO
extern int ExitCode;

#ifdef __cplusplus
}
#endif
