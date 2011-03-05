//保存先にiniファイルを使用したい場合は、0バイトのファイルでよいのでttpmenu.exeと同じフォルダにttpmenu.iniを用意する
#define		STRICT
static char *Registry_id = 
	"@(#)Copyright (C) NTT-IT 1998  -- registry.cpp --  Ver1.00b1";
/* ==========================================================================
	Project Name		: Universal Library
	Outline				: Registory Function
	Create				: 1998-02-17(Tue)
	Update				: 
	Copyright			: S.Hayakawa				NTT-IT
	Reference			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
#include	"registry.h"
#include	"tchar.h"
#include	"stdio.h"

BOOL bUseINI = FALSE;					// 保存先(TRUE=INI, FALSE=レジストリ)
TCHAR szSectionName[MAX_PATH];			// INIのセクション名
TCHAR szSectionNames[MAX_PATH*10]={0};	// INIのセクション名一覧
TCHAR szApplicationName[MAX_PATH]={0};	// INIファイルのフルパス

BOOL getSection(LPCTSTR str)
{
	szSectionNames[0] = 0;
	LPCTSTR t = _tcsrchr(str, _T('\\'));
	if(t){
		t++;
	}else{
		t = str;
	}
	_tcscpy(szSectionName, t);
	return TRUE;
}

LPCTSTR getModuleName()
{
	if(*szApplicationName == 0){
		GetModuleFileName(NULL, szApplicationName, sizeof(TCHAR)*sizeof(szApplicationName));
		LPTSTR t = szApplicationName + _tcslen(szApplicationName) - 3;
		_tcscpy(t, _T("ini"));
	}
	return szApplicationName;
}

//exeと同じフォルダにiniファイルが存在すればiniを使用、その他の場合はレジストリを使用
void checkIniFile()
{
	DWORD dwAttr = ::GetFileAttributes(getModuleName());
	bUseINI = dwAttr != 0xFFFFFFFF;
}

/* ==========================================================================
	Function Name	: (HKEY) RegCreate()
	Outline			: 指定したレジストリキーを作成（またはオープン）する
	Arguments		: HKEY		hCurrentKey		(in)	現在のオープンキー
					: LPCTSTR	lpszKeyName		(in)	オープンするサブキーの
					: 									名前
	Return Value	: 成功	オープンまたは作成されたキーのハンドル
					: 失敗	NULL
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
HKEY RegCreate(HKEY hCurrentKey, LPCTSTR lpszKeyName)
{
	if(bUseINI){
		getSection(lpszKeyName);
		return ERROR_SUCCESS;
	}else{
		long	lError;
		HKEY	hkResult;
		DWORD	dwDisposition;

		lError = ::RegCreateKeyEx(hCurrentKey,
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
					: LPCTSTR	lpszKeyName		(in)	オープンするサブキーの
					: 									名前
	Return Value	: 成功	オープンまたは作成されたキーのハンドル
					: 失敗	NULL
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
HKEY RegOpen(HKEY hCurrentKey, LPCTSTR lpszKeyName)
{
	if(bUseINI){
		getSection(lpszKeyName);
		return ERROR_SUCCESS;
	}else{
		long	lError;
		HKEY	hkResult;

		lError = ::RegOpenKeyEx(hCurrentKey,
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
					: LPCTSTR	lpszValueName	(in)	設定する値
					: char		*buf			(in)	値データ
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegSetStr(HKEY hKey, LPCTSTR lpszValueName, TCHAR *buf)
{
	if(bUseINI){
		return WritePrivateProfileString(szSectionName, lpszValueName, buf, getModuleName());
	}else{
		long	lError;

		lError = ::RegSetValueEx(hKey,
								lpszValueName,
								0,
								REG_SZ,
								(CONST BYTE *) buf,
								(::lstrlen(buf) + 1) * sizeof(TCHAR));
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
					: LPCTSTR	lpszValueName	(in)		設定する値
					: char		*buf			(out)		値データを格納する
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
BOOL RegGetStr(HKEY hKey, LPCTSTR lpszValueName, TCHAR *buf, DWORD dwSize)
{
	if(bUseINI){
		return GetPrivateProfileString(szSectionName, lpszValueName, _T(""), buf, dwSize, getModuleName());
	}else{
		LONG	lError;
		DWORD	dwWriteSize;
		DWORD	dwType = REG_SZ;

		dwWriteSize = dwSize * sizeof(TCHAR);

		lError = ::RegQueryValueEx(hKey, lpszValueName, 0, &dwType, (LPBYTE) buf, &dwWriteSize);
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
					: LPCTSTR	lpszValueName	(in)	設定する値
					: DWORD		dwValue			(in)	値データ
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegSetDword(HKEY hKey, LPCTSTR lpszValueName, DWORD dwValue)
{
	if(bUseINI){
		TCHAR t[64];
		_stprintf(t, _T("%d"), dwValue);
		return WritePrivateProfileString(szSectionName, lpszValueName, t, getModuleName());
	}else{
		long	lError;

		lError = ::RegSetValueEx(hKey,
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
					: LPCTSTR	lpszValueName	(in)	設定する値
					: DWORD		*dwValue		(out)	値データ
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegGetDword(HKEY hKey, LPCTSTR lpszValueName, DWORD *dwValue)
{
	int defmark = 0xdeadbeef;

	if(bUseINI){
		// 読み込みに失敗した場合は false を返す (2007.11.14 yutaka)
		*dwValue = GetPrivateProfileInt(szSectionName, lpszValueName, defmark, getModuleName());
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

		lError = ::RegQueryValueEx(hKey,
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
	Outline			: レジストリキーの値から BINARYを読み込む
	Arguments		: HKEY		hKey			(in)	値を設定するキーのハンドル
					: LPCTSTR	lpszValueName	(in)	設定する値
					: void		*buf			(out)	値データ
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegSetBinary(HKEY hKey, LPCTSTR lpszValueName, void *buf, DWORD dwSize)
{
	if(bUseINI){
		TCHAR t[1024] = {0};
		LPBYTE s = (LPBYTE)buf;
		for(DWORD i=0; i<dwSize; i++){
			TCHAR c[4];
			_stprintf(c, _T("%02X "), s[i]);
			_tcscat(t, c);
		}
		BOOL ret =  WritePrivateProfileString(szSectionName, lpszValueName, t, getModuleName());
		return ret;
	}else{
		long	lError;
		DWORD	dwWriteSize;

		dwWriteSize = dwSize * sizeof(TCHAR);

		if ((lError = ::RegSetValueEx(hKey,
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
					: LPCTSTR	lpszValueName	(in)	設定する値
					: int		*buf			(out)	値データ
	Return Value	: 成功	TRUE
					: 失敗	FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
// 関数の返値の型を追加 (2006.2.18 yutaka)
int RegGetBinary(HKEY hKey, LPCTSTR lpszValueName, void *buf, LPDWORD lpdwSize)
{
	if(bUseINI){
		TCHAR t[1024] = {0};
		BOOL ret = GetPrivateProfileString(szSectionName, lpszValueName, _T(""), t, sizeof(t), getModuleName());
		if(ret){
			int size = _tcslen(t);
			while(t[size-1] == ' '){
				size--;
				t[size] = 0;
			}
			LPCTSTR s = t;
			LPBYTE p = (LPBYTE)buf;
			DWORD cnt = 0;
			*p = 0;
			for(int i=0; i<(size+1)/3; i++){
				*p++ = (BYTE)_tcstol(s, NULL, 16);
				s += 3;
				cnt ++;
			}
			*lpdwSize = cnt;
		}
		return ret;
	}else{
		long	lError;
		DWORD	dwType = REG_BINARY;
		DWORD	dwWriteSize;

		dwWriteSize = *lpdwSize * sizeof(TCHAR);

		if ((lError = ::RegQueryValueEx(hKey,
										lpszValueName,
										NULL,
										&dwType,
										(LPBYTE) buf,
										&dwWriteSize)) != ERROR_SUCCESS) {
			::SetLastError(lError);
			return FALSE;
		}
	}

	return TRUE;
}


LONG RegEnumEx(HKEY hKey, DWORD dwIndex, LPTSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, LPTSTR lpClass, LPDWORD lpcClass, PFILETIME lpftLastWriteTime)
{
	static LPCTSTR ptr = szSectionNames;
	if(bUseINI){
		if(*szSectionNames == 0){
			GetPrivateProfileSectionNames(szSectionNames, sizeof(szSectionNames), getModuleName());
			ptr = szSectionNames;
		}
		if(_tcscmp(ptr, _T("TTermMenu")) == 0){
			//skip
			while(*ptr++);
//			ptr++;
		}
		if(*ptr == 0){
			return ERROR_NO_MORE_ITEMS;
		}
		_tcscpy(lpName, ptr);
		while(*ptr++);
//		ptr++;
		return ERROR_SUCCESS;
	}else{
		return ::RegEnumKeyEx(hKey, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, lpftLastWriteTime);
	}
}

LONG RegDelete(HKEY hKey, LPCTSTR lpSubKey)
{
	if(bUseINI){
		return WritePrivateProfileString(szSectionName, NULL, NULL, getModuleName()) ? ERROR_SUCCESS : ERROR_ACCESS_DENIED;
	}else{
		return ::RegDeleteKey(hKey, lpSubKey);
	}
}
