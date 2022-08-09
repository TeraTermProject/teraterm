/*
 * (C) 2022- TeraTerm Project
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
// プラグインで使われる関数

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TKeyMap_st *PKeyMap;

void PASCAL _ReadIniFile(const wchar_t *FName, PTTSet ts);
void PASCAL _WriteIniFile(const wchar_t *FName, PTTSet ts);
void PASCAL _CopySerialList(const wchar_t *IniSrc, const wchar_t *IniDest, const wchar_t *section,
							const wchar_t *key, int MaxList);
void PASCAL _AddValueToList(const wchar_t *FName, const wchar_t *Host, const wchar_t *section,
							const wchar_t *key, int MaxList);
void PASCAL _CopyHostList(const wchar_t *IniSrc, const wchar_t *IniDest);
void PASCAL _AddHostToList(const wchar_t *FName, const wchar_t *Host);
void PASCAL _ParseParam(wchar_t *Param, PTTSet ts, PCHAR DDETopic);
void PASCAL _ReadKeyboardCnf(const wchar_t *FName, PKeyMap KeyMap, BOOL ShowWarning);

#ifdef __cplusplus
}
#endif
