/*
 * Copyright (C) 2023- TeraTerm Project
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

/* @(#)Copyright (C) NTT-IT 1998   -- registry.h -- Ver1.00b1 */
/* ========================================================================
	Project  Name			: Universal Library
	Outline					: Registry function Header
	Create					: 1998-02-17(Tue)
	Update					: 
	Copyright				: S.Hayakawa				NTT-IT
    Reference				: 
	======================================================================== */

#pragma once

#ifndef REGISTRY_H
#define	REGISTRY_H
#include	<windows.h>

void	RegInit(void);
void	RegExit(void);
HKEY	RegCreate(HKEY hCurrentKey, const wchar_t *lpszKeyName);
HKEY	RegOpen(HKEY hCurrentKey, const wchar_t *lpszKeyName);
BOOL	RegClose(HKEY hKey);
BOOL	RegSetStr(HKEY hKey, const wchar_t *lpszValueName, const wchar_t *buf);
BOOL	RegGetStr(HKEY hKey, const wchar_t *lpszValueName, wchar_t *buf, DWORD dwSize);
BOOL	RegSetDword(HKEY hKey, const wchar_t *lpszValueName, DWORD dwvalue);
BOOL	RegGetDword(HKEY hKey, const wchar_t *lpszValueName, DWORD &dwValue);
BOOL	RegGetLONG(HKEY hKey, const wchar_t *lpszValueName, LONG &dwValue);
BOOL	RegGetBOOL(HKEY hKey, const wchar_t *lpszValueName, BOOL &value);
BOOL	RegGetBYTE(HKEY hKey, const wchar_t *lpszValueName, BYTE &value);
BOOL	RegSetBinary(HKEY hKey, const wchar_t *lpszValueName, void *buf, DWORD dwSize);
BOOL	RegGetBinary(HKEY hKey, const wchar_t *lpszValueName, void *buf, LPDWORD lpdwSize);
LONG	RegEnumEx(HKEY hKey, DWORD dwIndex, wchar_t *lpName, LPDWORD lpcName, LPDWORD lpReserved, wchar_t *lpClass, LPDWORD lpcClass, PFILETIME lpftLastWriteTime);
LONG	RegDelete(HKEY hKey, const wchar_t *lpSubKey);
void	RegGetStatus(BOOL *use_ini, wchar_t **inifile);

#endif
