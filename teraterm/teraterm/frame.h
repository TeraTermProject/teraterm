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
 *	Main window with tabs
 *
 *	The frame owns the menu bar and a tab strip. Every tab is a VT window
 *	(CVTWindow, a child window of the frame) with its own session.
 */

#pragma once

#include <windows.h>
#include "session.h"

class CVTWindow;

// the frame handles ID_FILE_NEWTAB, ID_FILE_CLOSETAB, ID_TAB_NEXT and
// ID_TAB_PREV (tt_res.h), a tab posts them to the frame

HWND FrameCreate(HINSTANCE hInstance);
HWND FrameGetHWND(void);
void FrameGetContentRect(RECT *rc);
void FrameAddTab(CVTWindow *vt, TTSession *session);
CVTWindow *FrameGetActiveVT(void);
HWND FrameGetActiveVTHWND(void);
void FrameNewTab(const wchar_t *command_line);
void FramePostNewTab(const wchar_t *command_line);
void FrameChildMenuChanged(HWND vt, HMENU menu);
void FrameChildTitleChanged(HWND vt);
void FrameChildSizeChanged(HWND vt);
void FrameChildDestroyed(HWND vt);
void FrameDarkModeChanged(void);

// teraterm.cpp
void InitSessionPaths(TTTSet *ts);
