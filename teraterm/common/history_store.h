/*
 * Copyright (C) 2024- TeraTerm Project
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

typedef struct HistoryStoreTag HistoryStore;

HistoryStore *HistoryStoreCreate(size_t max);
void HistoryStoreDestroy(HistoryStore *h);
void HistoryStoreClear(HistoryStore *h);
BOOL HistoryStoreAddTop(HistoryStore *h, const wchar_t *add_str, BOOL enable_duplicate);
void HistoryStoreReadIni(HistoryStore *h, const wchar_t *FName, const wchar_t *section, const wchar_t *key);
void HistoryStoreSaveIni(HistoryStore *h, const wchar_t *FName, const wchar_t *section, const wchar_t *key);
void HistoryStoreSetControl(HistoryStore *h, HWND dlg, int dlg_item);
void HistoryStoreGetControl(HistoryStore *h, HWND dlg, int dlg_item);
const wchar_t *HistoryStoreGet(HistoryStore *h, size_t index);
void HistoryStoreSet(HistoryStore *h, size_t index, const wchar_t *str);

#ifdef __cplusplus
}
#endif