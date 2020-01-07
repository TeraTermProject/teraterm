/*
 * (C) 2019-2020 TeraTerm Project
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

#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SendMemTag SendMem;

SendMem *SendMemTextW(wchar_t *ptr, size_t len);
SendMem *SendMemBinary(void *ptr, size_t len);
void SendMemInitEcho(SendMem *sm, BOOL echo);
void SendMemInitDelay(SendMem *sm, DWORD per_line, DWORD per_char);
void SendMemInitDialog(SendMem *sm, HINSTANCE hInstance, HWND hWndParent, const char *UILanguageFile);
void SendMemInitDialogCaption(SendMem *sm, const wchar_t *caption);
void SendMemInitDialogFilename(SendMem *sm, const wchar_t *filename);
BOOL SendMemStart(SendMem *sm);		// 送信開始
void SendMemFinish(SendMem *sm);

// convenient function
BOOL SendMemSendFile(const wchar_t *filename, BOOL binary);
BOOL SendMemPasteString(wchar_t *str);

// idleからの送信用API
void SendMemContinuously(void);

#ifdef __cplusplus
}
#endif

