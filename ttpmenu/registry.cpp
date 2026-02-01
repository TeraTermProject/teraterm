/*
 * Copyright (C) S.Hayakawa NTT-IT 1998-2002
 * (C) 2002- TeraTerm Project
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

//保存先にiniファイルを使用したい場合は、0バイトのファイルでよいのでttpmenu.exeと同じフォルダにttpmenu.iniを用意する
#define		STRICT
#include	"registry.h"
#include	<stdio.h>
#define		_CRTDBG_MAP_ALLOC
#include	<stdlib.h>
#include	<crtdbg.h>
#include	<wchar.h>
#include	<assert.h>

#include	"ttlib.h"
#include	"asprintf.h"
#include	"fileread.h"

#define ENABLE_CONVERT_EXE_INI 1

static BOOL bUseINI = FALSE;					// 保存先(TRUE=INI, FALSE=レジストリ)
static wchar_t szSectionName[MAX_PATH];			// INIのセクション名
static wchar_t szSectionNames[1024*10]={0};		// INIのセクション名一覧
static wchar_t *szApplicationName;				// INIファイルのフルパス

static BOOL getSection(const wchar_t *str)
{
	szSectionNames[0] = 0;
	const wchar_t *t = wcsrchr(str, L'\\');
	if(t){
		t++;
	}else{
		t = str;
	}
	wcscpy_s(szSectionName, t);
	return TRUE;
}

/**
 *	INIファイルのフルパス取得
 */
static const wchar_t *getModuleName()
{
	assert(szApplicationName != NULL);
	return szApplicationName;
}

/**
 *	exeと同じパスにあるiniファイルを
 *	%APPDATA%\teraterm5\ttpmenu.ini にコピーする
 */
#if ENABLE_CONVERT_EXE_INI
static void CopyIniFile(const wchar_t *ini)
{
	// exeフォルダのiniファイル
	wchar_t *exe_ini;
	wchar_t *exe_dir = GetExeDirW(NULL);
	aswprintf(&exe_ini, L"%s\\ttpmenu.ini", exe_dir);
	free(exe_dir);
	if (::GetFileAttributesW(exe_ini) == INVALID_FILE_ATTRIBUTES) {
		// 存在しないので何もしない
		free(exe_ini);
		return;
	}

	// UTF-16 で書き出す
	size_t content_len = 0;
	wchar_t *exe_ini_content = LoadFileWW(exe_ini, &content_len);
	if (exe_ini_content != NULL) {
		// write unicode ini
		FILE *fp;
		_wfopen_s(&fp, ini, L"wb");
		if (fp != NULL) {
			fwrite("\xff\xfe", 2, 1, fp);
			fwrite(exe_ini_content, content_len, sizeof(wchar_t), fp);
			fclose(fp);
		}
		free(exe_ini_content);
	}
	free(exe_ini);
}
#endif

void RegInit()
{
	// 通常			%APPDATA%\teraterm5\ttpmenu.ini
	// ポータブル	exeフォルダの\ttpmenu.ini
	wchar_t *ini;
	wchar_t *home_dir = GetHomeDirW(NULL);
	aswprintf(&ini, L"%s\\ttpmenu.ini", home_dir);
	free(home_dir);

#if ENABLE_CONVERT_EXE_INI
	// ポータブル版?
	if (!IsPortableMode()) {
		// %APPDATA%\teraterm5\ttpmenu.ini が存在する?
		if (::GetFileAttributesW(ini) == INVALID_FILE_ATTRIBUTES) {
			// iniファイルが存在しないなら
			// exeと同じフォルダのiniファイルをコピーする
			CopyIniFile(ini);
		}
	}
#endif

	// iniファイル存在する?
	if (::GetFileAttributesW(ini) != INVALID_FILE_ATTRIBUTES) {
		// ファイルサイズが0のとき UTF16 LE BOM を書き込む
		if (GetFSize64W(ini) == 0) {
			FILE *fp;
			_wfopen_s(&fp, ini, L"wb");
			if (fp != NULL) {
				fwrite("\xff\xfe", 2, 1, fp);
				fclose(fp);
			}
		}

		// ttpmenu.iniを使用する
		szApplicationName = ini;
		bUseINI = TRUE;
		return;
	}

	// レジストリを使用する
	free(ini);
	szApplicationName = NULL;
	bUseINI = FALSE;
}

void RegExit()
{
	free(szApplicationName);
	szApplicationName = NULL;
}

/**
 *	設定の状況
 *
 *	@param[out]	use_ini		TRUE/FALSE = iniファイル/ registry を使用している
 *	@param[out]	inifile		iniファイル名, registryの場合は NULL
 *							不要になったらfree()すること
 *
 */
void RegGetStatus(BOOL *use_ini, wchar_t **inifile)
{
	*use_ini = bUseINI;
	if (bUseINI) {
		*inifile = _wcsdup(szApplicationName);
	}
	else {
		*inifile = NULL;
	}
}

/* ==========================================================================
	Function Name	: (HKEY) RegCreate()
	Outline			: 指定したレジストリキーを作成（またはオープン）する
	Arguments		: HKEY		hCurrentKey		(in)	現在のオープンキー
					: const wchar_t *lpszKeyName		(in)	オープンするサブキーの
					: 									名前
	Return Value	: 成功	オープンまたは作成されたキーのハンドル
					: 失敗	NULL
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
HKEY RegCreate(HKEY hCurrentKey, const wchar_t *lpszKeyName)
{
	if(bUseINI){
		getSection(lpszKeyName);
		return ERROR_SUCCESS;
	}else{
		long	lError;
		HKEY	hkResult;
		DWORD	dwDisposition;

		lError = ::RegCreateKeyExW(hCurrentKey,
								lpszKeyName,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hkResult,
								&dwDisposition);
		if (lError != ERROR_SUCCESS) {
			::SetLastError(lError);
			return (HKEY) INVALID_HANDLE_VALUE;
		}

		return hkResult;
	}
}

/* ==========================================================================
	Function Name	: (HKEY) RegOpen()
	Outline			: 指定したレジストリキーをオープンする
	Arguments		: HKEY		hCurrentKey		(in)	現在のオープンキー
					: const wchar_t *lpszKeyName		(in)	オープンするサブキーの
					: 									名前
	Return Value	: 成功	オープンまたは作成されたキーのハンドル
					: 失敗	NULL
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
HKEY RegOpen(HKEY hCurrentKey, const wchar_t *lpszKeyName)
{
	if(bUseINI){
		getSection(lpszKeyName);
		return ERROR_SUCCESS;
	}else{
		long	lError;
		HKEY	hkResult;

		lError = ::RegOpenKeyExW(hCurrentKey,
								lpszKeyName,
								0,
								KEY_ALL_ACCESS,
								&hkResult);
		if (lError != ERROR_SUCCESS) {
			::SetLastError(lError);
			return (HKEY) INVALID_HANDLE_VALUE;
		}

		return hkResult;
	}
}

/* ==========================================================================
	Function Name	: (BOOL) RegClose()
	Outline			: 指定したレジストリキーをクローズする
	Arguments		: HKEY		hKey			(in)	クローズするキーのハンドル
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegClose(HKEY hKey)
{
	if(bUseINI){
		
	}else{
		long	lError;

		lError = ::RegCloseKey(hKey);
		if (lError != ERROR_SUCCESS) {
			::SetLastError(lError);
			return FALSE;
		}
	}

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) RegSetStr()
	Outline			: レジストリキーの値に文字列を書き込む
	Arguments		: HKEY		hKey			(in)	値を設定するキーのハンドル
					: const wchar_t *lpszValueName	(in)	設定する値
					: const wchar_t *buf			(in)	値データ
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegSetStr(HKEY hKey, const wchar_t *lpszValueName, const wchar_t *buf)
{
	if(bUseINI){
		return WritePrivateProfileStringW(szSectionName, lpszValueName, buf, getModuleName());
	}else{
		long	lError;

		lError = ::RegSetValueExW(hKey,
								lpszValueName,
								0,
								REG_SZ,
								(const BYTE *) buf,
								(DWORD)((wcslen(buf) + 1) * sizeof(wchar_t)));
		if (lError != ERROR_SUCCESS) {
			::SetLastError(lError);
			return FALSE;
		}
	}

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) RegGetStr()
	Outline			: レジストリキーの値から文字列を読み込む
	Arguments		: HKEY		hKey			(in)		値を設定するキーの
					: 										ハンドル
					: const wchar_t *	lpszValueName	(in)		設定する値
					: wchar_t	*buf			(out)		値データを格納する
					: 										バッファ
					: DWORD		dwSize			(in/out)	文字数
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegGetStr(HKEY hKey, const wchar_t *lpszValueName, wchar_t *buf, DWORD dwSize)
{
	if(bUseINI){
		return GetPrivateProfileStringW(szSectionName, lpszValueName, L"", buf, dwSize, getModuleName());
	}else{
		LONG	lError;
		DWORD	dwWriteSize;
		DWORD	dwType = REG_SZ;

		dwWriteSize = dwSize * sizeof(wchar_t);

		lError = ::RegQueryValueExW(hKey, lpszValueName, 0, &dwType, (LPBYTE) buf, &dwWriteSize);
		if (lError != ERROR_SUCCESS) {
			::SetLastError(lError);
			return FALSE;
		}

		buf[dwSize - 1] = '\0';
	}

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) RegSetDword()
	Outline			: レジストリキーの値に DWORDを書き込む
	Arguments		: HKEY		hKey			(in)	値を設定するキーのハンドル
					: const wchar_t *lpszValueName	(in)	設定する値
					: DWORD		dwValue			(in)	値データ
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegSetDword(HKEY hKey, const wchar_t *lpszValueName, DWORD dwValue)
{
	if(bUseINI){
		wchar_t t[64];
		swprintf_s(t, L"%d", dwValue);
		return WritePrivateProfileStringW(szSectionName, lpszValueName, t, getModuleName());
	}else{
		long	lError;

		lError = ::RegSetValueExW(hKey,
								lpszValueName,
								0,
								REG_DWORD,
								(CONST BYTE *) &dwValue,
								sizeof(DWORD));
		if (lError != ERROR_SUCCESS) {
			::SetLastError(lError);
			return FALSE;
		}
	}

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) RegGetDword()
	Outline			: レジストリキーの値から DWORDを読み込む
	Arguments		: HKEY		hKey			(in)	値を設定するキーのハンドル
					: const wchar_t *lpszValueName	(in)	設定する値
					: DWORD		*dwValue		(out)	値データ
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegGetDword(HKEY hKey, const wchar_t *lpszValueName, DWORD *dwValue)
{
	int defmark = 0xdeadbeef;

	if(bUseINI){
		// 読み込みに失敗した場合は false を返す (2007.11.14 yutaka)
		*dwValue = GetPrivateProfileIntW(szSectionName, lpszValueName, defmark, getModuleName());
		if (*dwValue == defmark) {
			*dwValue = 0;
			return FALSE;
		} else {
			return TRUE;
		}
	}else{
		long	lError;
		DWORD	dwType = REG_DWORD;
		DWORD	dwSize = sizeof(DWORD);

		lError = ::RegQueryValueExW(hKey,
									lpszValueName,
									0,
									&dwType,
									(LPBYTE) dwValue,
									&dwSize);
		if (lError != ERROR_SUCCESS) {
			::SetLastError(lError);
			return FALSE;
		}
	}

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) RegSetBinary()
	Outline			: レジストリキーの値から BINARYを書き込む
	Arguments		: HKEY		hKey			(in)	値を設定するキーのハンドル
					: const wchar_t *lpszValueName	(in)	設定する値
					: void		*buf			(out)	値データ
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegSetBinary(HKEY hKey, const wchar_t *lpszValueName, void *buf, DWORD dwSize)
{
	if(bUseINI){
		wchar_t t[1024] = {0};
		LPBYTE s = (LPBYTE)buf;
		DWORD i;
		for(i=0; i<dwSize; i++){
			wchar_t c[4];
			swprintf_s(c, L"%02X ", s[i]);
			wcscat_s(t, c);
		}
		if (i > 0) {
			t[i*3-1] = 0;
		}
		BOOL ret =  WritePrivateProfileStringW(szSectionName, lpszValueName, t, getModuleName());
		return ret;
	}else{
		long	lError;
		DWORD	dwWriteSize;

		dwWriteSize = dwSize;

		if ((lError = ::RegSetValueExW(hKey,
									lpszValueName,
									0,
									REG_BINARY,
									(CONST BYTE *) buf,
									dwWriteSize)) != ERROR_SUCCESS) {
			::SetLastError(lError);
			return FALSE;
		}
	}

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) RegGetBinary()
	Outline			: レジストリキーの値から BINARYを読み込む
	Arguments		: HKEY		hKey			(in)	値を設定するキーのハンドル
					: const wchar_t *lpszValueName	(in)	設定する値
					: int		*buf			(out)	値データ
					: LPDWORD lpdwSize			(in,out)	バッファサイズ,読み込みサイズ
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegGetBinary(HKEY hKey, const wchar_t *lpszValueName, void *buf, LPDWORD lpdwSize)
{
	if(bUseINI){
		wchar_t t[1024] = {0};
		BOOL ret = GetPrivateProfileStringW(szSectionName, lpszValueName, L"", t, _countof(t), getModuleName());
		if(ret){
			size_t size = wcslen(t);
			while(t[size-1] == ' '){
				size--;
				t[size] = 0;
			}
			wchar_t *s = t;
			LPBYTE p = (LPBYTE)buf;
			DWORD cnt = 0;
			*p = 0;
			for(size_t i=0; i<(size+1)/3; i++){
				*p++ = (BYTE)wcstol(s, NULL, 16);
				s += 3;
				cnt ++;
			}
			*lpdwSize = cnt;
		}
		return ret;
	}else{
		long	lError;
		DWORD	dwType = REG_BINARY;
		DWORD	dwReadSize;

		dwReadSize = *lpdwSize;

		if ((lError = ::RegQueryValueExW(hKey,
										lpszValueName,
										NULL,
										&dwType,
										(LPBYTE) buf,
										&dwReadSize)) != ERROR_SUCCESS) {
			::SetLastError(lError);
			return FALSE;
		}
		*lpdwSize = dwReadSize;
	}

	return TRUE;
}


LONG RegEnumEx(HKEY hKey, DWORD dwIndex, wchar_t *lpName, LPDWORD lpcName, LPDWORD lpReserved, wchar_t *lpClass, LPDWORD lpcClass, PFILETIME lpftLastWriteTime)
{
	static wchar_t *ptr = szSectionNames;
	if(bUseINI){
		if(*szSectionNames == 0){
			GetPrivateProfileSectionNamesW(szSectionNames, _countof(szSectionNames), getModuleName());
			ptr = szSectionNames;
		}
		if(wcscmp(ptr, L"TTermMenu") == 0){
			//skip
			while(*ptr++);
//			ptr++;
		}
		if(*ptr == 0){
			return ERROR_NO_MORE_ITEMS;
		}
		wcscpy(lpName, ptr);
		while(*ptr++);
//		ptr++;
		return ERROR_SUCCESS;
	}else{
		return ::RegEnumKeyExW(hKey, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, lpftLastWriteTime);
	}
}

LONG RegDelete(HKEY hKey, const wchar_t *lpSubKey)
{
	if(bUseINI){
		return WritePrivateProfileStringW(szSectionName, NULL, NULL, getModuleName()) ? ERROR_SUCCESS : ERROR_ACCESS_DENIED;
	}else{
		return ::RegDeleteKeyW(hKey, lpSubKey);
	}
}

BOOL RegGetDword(HKEY hKey, const wchar_t *lpszValueName, DWORD &dwValue)
{
	return RegGetDword(hKey, lpszValueName, &dwValue);
}

BOOL RegGetLONG(HKEY hKey, const wchar_t *lpszValueName, LONG &dwValue)
{
	DWORD d;
	BOOL r = RegGetDword(hKey, lpszValueName, &d);
	dwValue = d;
	return r;
}

BOOL RegGetBOOL(HKEY hKey, const wchar_t *lpszValueName, BOOL &value)
{
	DWORD d;
	BOOL r = RegGetDword(hKey, lpszValueName, &d);
	value = d;
	return r;
}

BOOL RegGetBYTE(HKEY hKey, const wchar_t *lpszValueName, BYTE &value)
{
	DWORD d;
	BOOL r = RegGetDword(hKey, lpszValueName, &d);
	value = (BYTE)d;
	return r;
}

