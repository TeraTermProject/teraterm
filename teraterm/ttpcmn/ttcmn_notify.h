/*
 * Copyright (C) 2022- TeraTerm Project
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

// プラグインから使用するAPI

#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DllExport)
#define DllExport __declspec(dllimport)
#endif

DllExport void WINAPI NotifyMessageW(PComVar cv, const wchar_t *message, const wchar_t *title, DWORD flag);
DllExport void WINAPI NotifyMessage(PComVar cv, const char *message, const char *title, DWORD flag);
DllExport void WINAPI NotifySetIconID(PComVar cv, HINSTANCE hInstance, WORD IconID);

#define NotifyInfoMessage(cv, msg, title) NotifyMessage(cv, msg, title, 1)
#define NotifyWarnMessage(cv, msg, title) NotifyMessage(cv, msg, title, 2)
#define NotifyErrorMessage(cv, msg, title) NotifyMessage(cv, msg, title, 3)
#define NotifyInfoMessageW(cv, msg, title) NotifyMessageW(cv, msg, title, 1)
#define NotifyWarnMessageW(cv, msg, title) NotifyMessageW(cv, msg, title, 2)
#define NotifyErrorMessageW(cv, msg, title) NotifyMessageW(cv, msg, title, 3)

#ifdef __cplusplus
}
#endif
