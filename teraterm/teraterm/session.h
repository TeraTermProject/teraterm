/*
 * (C) 2026- TeraTerm Project
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
 *	Sessions (one per tab)
 *
 *	Tera Term was written for one connection per process. Its terminal,
 *	screen buffer and connection state live in global and file-static
 *	variables. A session keeps its own copy of that state:
 *
 *	- settings (TTTSet) and connection (TComVar) are separate objects per
 *	  session, reached through g_ts / g_cv (see ttwinman.h)
 *	- each module registers its file-static variables with
 *	  SessionRegisterVars(); SessionSwitch() saves them into the old session
 *	  and loads them from the new one
 *
 *	Code that runs for a session (window procedure, socket / serial
 *	notification, timer) must make that session active first, for example:
 *
 *		TTSession *prev = SessionEnter(session);
 *		...
 *		SessionLeave(prev);
 */

#pragma once

#include <windows.h>
#include "tttypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TTSession TTSession;

typedef struct {
	void *addr;
	size_t size;
} SessionVar;

#define SESSION_VAR(v) { (void *)&(v), sizeof(v) }

void SessionRegisterVars(const SessionVar *vars, size_t count);

void SessionInit(void);
TTSession *SessionCreate(void);
void SessionDestroy(TTSession *session);
void SessionSwitch(TTSession *session);
TTSession *SessionGetActive(void);
int SessionGetCount(void);

TTSession *SessionEnter(TTSession *session);
void SessionLeave(TTSession *prev);

#ifdef __cplusplus
}
#endif
