#ifndef REGISTRY_H
#define	REGISTRY_H
/* @(#)Copyright (C) NTT-IT 1998   -- registry.h -- Ver1.00b1 */
/* ========================================================================
	Project  Name			: Universal Library
	Outline					: Registry function Header
	Create					: 1998-02-17(Tue)
	Update					: 
	Copyright				: S.Hayakawa				NTT-IT
    Reference				: 
	======================================================================== */
#include	<windows.h>

HKEY	RegCreate(HKEY hCurrentKey, const wchar_t *lpszKeyName);
HKEY	RegOpen(HKEY hCurrentKey, const wchar_t *lpszKeyName);
BOOL	RegClose(HKEY hKey);
BOOL	RegSetStr(HKEY hKey, const wchar_t *lpszValueName, const wchar_t *buf);
BOOL	RegGetStr(HKEY hKey, const wchar_t *lpszValueName, wchar_t *buf, DWORD dwSize);
BOOL	RegSetDword(HKEY hKey, const wchar_t *lpszValueName, DWORD dwvalue);
BOOL	RegGetDword(HKEY hKey, const wchar_t *lpszValueName, DWORD *dwValue);
BOOL	RegSetBinary(HKEY hKey, const wchar_t *lpszValueName, void *buf, DWORD dwSize);
BOOL	RegGetBinary(HKEY hKey, const wchar_t *lpszValueName, void *buf, LPDWORD lpdwSize);

LONG RegEnumEx(HKEY hKey, DWORD dwIndex, wchar_t *lpName, LPDWORD lpcName, LPDWORD lpReserved, wchar_t *lpClass, LPDWORD lpcClass, PFILETIME lpftLastWriteTime);
LONG RegDelete(HKEY hKey, const wchar_t *lpSubKey);

void checkIniFile();

#endif
