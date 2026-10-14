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


#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ttwinman.h"
#include "session.h"
#include "ttplug.h"
#include "dbglog.h"

struct TTSession {
	TTTSet *ts;
	TComVar *cv;
	unsigned char *state;	// saved module variables
	BOOL static_tscv;		// ts / cv are the static ones in ttwinman.c
};

#define MAX_VARS 512

static SessionVar vars[MAX_VARS];
static size_t var_count;
static size_t state_size;
static unsigned char *default_state;	// initial values of module variables

static TTSession *active;
static TTSession **sessions;
static int session_count;

static void AddToList(TTSession *s)
{
	sessions = (TTSession **)realloc(sessions, sizeof(TTSession *) * (session_count + 1));
	sessions[session_count++] = s;
}

static void RemoveFromList(TTSession *s)
{
	for (int i = 0; i < session_count; i++) {
		if (sessions[i] == s) {
			memmove(&sessions[i], &sessions[i + 1], sizeof(TTSession *) * (session_count - i - 1));
			session_count--;
			return;
		}
	}
	assert(FALSE);
}

// modules with per session state
void VTTermRegisterSessionVars(void);
void DispRegisterSessionVars(void);
void BuffRegisterSessionVars(void);
void WinManRegisterSessionVars(void);
void KeybRegisterSessionVars(void);
void TelRegisterSessionVars(void);
void CommRegisterSessionVars(void);
void FileSysRegisterSessionVars(void);
void FileSysProtoRegisterSessionVars(void);
void SendMemRegisterSessionVars(void);
void VTWinRegisterSessionVars(void);
void WinsockRegisterSessionVars(void);
void FileIORegisterSessionVars(void);

void SessionRegisterVars(const SessionVar *v, size_t count)
{
	assert(default_state == NULL);	// register in SessionInit() only
	assert(var_count + count <= MAX_VARS);
	for (size_t i = 0; i < count; i++) {
		vars[var_count++] = v[i];
		state_size += v[i].size;
	}
}

static void SaveVars(unsigned char *dst)
{
	for (size_t i = 0; i < var_count; i++) {
		memcpy(dst, vars[i].addr, vars[i].size);
		dst += vars[i].size;
	}
}

static void LoadVars(const unsigned char *src)
{
	for (size_t i = 0; i < var_count; i++) {
		memcpy(vars[i].addr, src, vars[i].size);
		src += vars[i].size;
	}
}

/**
 *	Call first thing in WinMain(), before any module is initialized
 *	The first session uses the settings and connection that g_ts / g_cv
 *	already point to
 */
void SessionInit(void)
{
	assert(active == NULL);
	VTTermRegisterSessionVars();
	DispRegisterSessionVars();
	BuffRegisterSessionVars();
	WinManRegisterSessionVars();
	KeybRegisterSessionVars();
	TelRegisterSessionVars();
	CommRegisterSessionVars();
	FileSysRegisterSessionVars();
	FileSysProtoRegisterSessionVars();
	SendMemRegisterSessionVars();
	VTWinRegisterSessionVars();
	WinsockRegisterSessionVars();
	FileIORegisterSessionVars();

	default_state = (unsigned char *)malloc(state_size);
	SaveVars(default_state);

	TTSession *first = (TTSession *)calloc(1, sizeof(TTSession));
	first->ts = g_ts;
	first->cv = g_cv;
	first->state = (unsigned char *)malloc(state_size);
	memcpy(first->state, default_state, state_size);
	first->static_tscv = TRUE;
	active = first;
	AddToList(first);
	DbgLog("session %p created (first), %u bytes of state per session", first, (unsigned)state_size);
}

/**
 *	Create a session with initial (not yet set up) state
 *	It is not made active
 */
TTSession *SessionCreate(void)
{
	TTSession *s = (TTSession *)calloc(1, sizeof(TTSession));
	s->ts = (TTTSet *)calloc(1, sizeof(TTTSet));
	s->cv = (TComVar *)calloc(1, sizeof(TComVar));
	s->state = (unsigned char *)malloc(state_size);
	memcpy(s->state, default_state, state_size);
	AddToList(s);
	DbgLog("session %p created (%d sessions)", s, session_count);
	return s;
}

/**
 *	Free a session that is not active
 *	Resources owned by the session (buffers, handles) must be released by
 *	the modules while it is active, before calling this
 */
void SessionDestroy(TTSession *s)
{
	assert(s != active);
	DbgLog("session %p destroyed", s);
	TTXSessionEnd(s);
	RemoveFromList(s);
	free(s->state);
	if (!s->static_tscv) {
		free(s->ts);
		free(s->cv);
	}
	free(s);
}

void SessionSwitch(TTSession *s)
{
	assert(s != NULL);
	if (s == active) {
		return;
	}
	SaveVars(active->state);
	LoadVars(s->state);
	g_ts = s->ts;
	g_cv = s->cv;
	active = s;
	// plugins with per session state (ttssh2)
	TTXSessionSwitch(s, s->ts, s->cv);
}

TTSession *SessionGetActive(void)
{
	return active;
}

int SessionGetCount(void)
{
	return session_count;
}

TTSession *SessionGetAt(int index)
{
	if (index < 0 || index >= session_count) {
		return NULL;
	}
	return sessions[index];
}

/**
 *	TRUE for the first session
 *	TTX plugins (ttssh2 ...) were initialized with its settings and
 *	connection, they work only for it
 */
BOOL SessionIsPluginSession(TTSession *s)
{
	return s->static_tscv;
}

TTTSet *SessionGetTS(TTSession *s)
{
	return s->ts;
}

TComVar *SessionGetCV(TTSession *s)
{
	return s->cv;
}

TTSession *SessionEnter(TTSession *s)
{
	TTSession *prev = active;
	SessionSwitch(s);
	return prev;
}

void SessionLeave(TTSession *prev)
{
	SessionSwitch(prev);
}
