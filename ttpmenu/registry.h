#ifndef REGISTRY_H
#define	REGISTRY_H
/* @(#)Copyright (C) NTT-IT 1998   -- registry.h -- Ver1.00b1 */
/* ========================================================================
	Project  Name			: Universal Library
	Outline					: Registory function Header
	Create					: 1998-02-17(Tue)
	Update					: 
	Copyright				: S.Hayakawa				NTT-IT
    Reference				: 
	======================================================================== */
#include	<windows.h>

HKEY	RegCreate(HKEY hCurrentKey, LPCTSTR lpszKeyName);
HKEY	RegOpen(HKEY hCurrentKey, LPCTSTR lpszKeyName);
BOOL	RegClose(HKEY hKey);
BOOL	RegSetStr(HKEY hKey, LPCTSTR lpszValueName, TCHAR *buf);
BOOL	RegGetStr(HKEY hKey, LPCTSTR lpszValueName, TCHAR *buf, DWORD dwSize);
BOOL	RegSetDword(HKEY hKey, LPCTSTR lpszValueName, DWORD dwvalue);
BOOL	RegGetDword(HKEY hKey, LPCTSTR lpszValueName, DWORD *dwValue);
BOOL	RegSetBinary(HKEY hKey, LPCTSTR lpszValueName, void *buf, DWORD dwSize);
BOOL	RegGetBinary(HKEY hKey, LPCTSTR lpszValueName, void *buf, LPDWORD lpdwSize);

LONG RegEnumEx(HKEY hKey, DWORD dwIndex, LPTSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, LPTSTR lpClass, LPDWORD lpcClass, PFILETIME lpftLastWriteTime);
LONG RegDelete(HKEY hKey, LPCTSTR lpSubKey);

void checkIniFile();

#endif
