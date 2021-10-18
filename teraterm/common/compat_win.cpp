/*
 * (C) 2018- TeraTerm Project
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

/* compat_win */

#include <windows.h>
#include <windns.h>
#include <assert.h>

#include "compat_win.h"
#include "compat_windns.h"

#include "dllutil.h"
#include "codeconv.h"

// for debug
//#define UNICODE_API_DISABLE	1

BOOL (WINAPI *pAlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
BOOL (WINAPI *pIsValidDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
UINT (WINAPI *pGetDpiForWindow)(HWND hwnd);
BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
HRESULT (WINAPI *pGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
BOOL (WINAPI *pAdjustWindowRectEx)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
BOOL (WINAPI *pAdjustWindowRectExForDpi)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
#ifdef _WIN64
LONG_PTR (WINAPI *pSetWindowLongPtrW)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LONG_PTR (WINAPI *pGetWindowLongPtrW)(HWND hWnd, int nIndex);
#endif

// user32
int (WINAPI *pGetSystemMetricsForDpi)(int nIndex, UINT dpi);

// kernel32
void (WINAPI *pOutputDebugStringW)(LPCWSTR lpOutputString);
HWND (WINAPI *pGetConsoleWindow)(void);
DWORD (WINAPI *pExpandEnvironmentStringsW)(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize);
static ULONGLONG (WINAPI *pVerSetConditionMask)(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask);
static BOOL (WINAPI *pVerifyVersionInfoA)(LPOSVERSIONINFOEX lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask);
BOOL (WINAPI *pSetDefaultDllDirectories)(DWORD DirectoryFlags);
BOOL (WINAPI *pSetDllDirectoryA)(LPCSTR lpPathName);

// gdi32
int (WINAPI *pAddFontResourceExW)(LPCWSTR name, DWORD fl, PVOID res);
BOOL (WINAPI *pRemoveFontResourceExW)(LPCWSTR name, DWORD fl, PVOID pdv);

// htmlhelp.dll (hhctrl.ocx)
static HWND (WINAPI *pHtmlHelpW)(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);
static HWND (WINAPI *pHtmlHelpA)(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);

// multi monitor Windows98+/Windows2000+
BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPCRECT,MONITORENUMPROC,LPARAM);
HMONITOR (WINAPI *pMonitorFromWindow)(HWND hwnd, DWORD dwFlags);
HMONITOR (WINAPI *pMonitorFromPoint)(POINT pt, DWORD dwFlags);
HMONITOR (WINAPI *pMonitorFromRect)(LPCRECT lprc, DWORD dwFlags);
BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);

// dnsapi
DNS_STATUS (WINAPI *pDnsQuery_A)(PCSTR pszName, WORD wType, DWORD Options, PVOID pExtra, PDNS_RECORD *ppQueryResults, PVOID *pReserved);
VOID (WINAPI *pDnsFree)(PVOID pData, DNS_FREE_TYPE FreeType);

// imagehlp.dll
BOOL (WINAPI *pSymGetLineFromAddr)(HANDLE hProcess, DWORD dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE Line);

// dbghelp.dll
BOOL(WINAPI *pMiniDumpWriteDump)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
								 PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
								 PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
								 PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

// shell32.dll
static HRESULT (WINAPI *pSHGetKnownFolderPath)(REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR* ppszPath);

// comctl32.dll
static HRESULT (WINAPI *pLoadIconWithScaleDown)(HINSTANCE hinst, PCWSTR pszName, int cx, int cy, HICON *phico);


class Initializer {
public:
	Initializer() {
		DLLInit();
		WinCompatInit();
	}
	~Initializer() {
		DLLExit();
	}
};

static Initializer initializer;

/**
 *	GetConsoleWindow() と同じ動作をする
 *	 https://support.microsoft.com/ja-jp/help/124103/how-to-obtain-a-console-window-handle-hwnd
 */
static HWND WINAPI GetConsoleWindowLocal(void)
{
#define MY_BUFSIZE 1024					 // Buffer size for console window titles.
	HWND hwndFound;						 // This is what is returned to the caller.
	char pszNewWindowTitle[MY_BUFSIZE];  // Contains fabricated WindowTitle.
	char pszOldWindowTitle[MY_BUFSIZE];  // Contains original WindowTitle.

	// Fetch current window title.
	DWORD size = GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);
	if (size == 0) {
		DWORD err = GetLastError();
		if (err == ERROR_INVALID_HANDLE) {
			// コンソールが開いていない
			return NULL;
		}
	}

	// Format a "unique" NewWindowTitle.
	wsprintf(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

	// Change current window title.
	SetConsoleTitle(pszNewWindowTitle);

	// Ensure window title has been updated.
	Sleep(40);

	// Look for NewWindowTitle.
	hwndFound = FindWindow(NULL, pszNewWindowTitle);

	// Restore original window title.
	SetConsoleTitle(pszOldWindowTitle);

	return hwndFound;
}

static const APIInfo Lists_user32[] = {
	{ "SetLayeredWindowAttributes", (void **)&pSetLayeredWindowAttributes },
	{ "SetThreadDpiAwarenessContext", (void **)&pSetThreadDpiAwarenessContext },
	{ "IsValidDpiAwarenessContext", (void **)&pIsValidDpiAwarenessContext },
	{ "GetDpiForWindow", (void **)&pGetDpiForWindow },
	{ "AdjustWindowRectEx", (void **)&pAdjustWindowRectEx },
	{ "AdjustWindowRectExForDpi", (void **)&pAdjustWindowRectExForDpi },
#ifndef UNICODE_API_DISABLE
#ifdef _WIN64
	{ "SetWindowLongPtrW", (void **)&pSetWindowLongPtrW },
	{ "GetWindowLongPtrW", (void **)&pGetWindowLongPtrW },
#endif
#endif
	{ "EnumDisplayMonitors", (void **)&pEnumDisplayMonitors },
	{ "MonitorFromWindow", (void **)&pMonitorFromWindow },
	{ "MonitorFromPoint", (void **)&pMonitorFromPoint },
	{ "MonitorFromRect", (void **)&pMonitorFromRect },
	{ "GetMonitorInfoA", (void **)&pGetMonitorInfoA },
	{ "GetSystemMetricsForDpi", (void **)&pGetSystemMetricsForDpi },
	{},
};

static const APIInfo Lists_msimg32[] = {
	{ "AlphaBlend", (void **)&pAlphaBlend },
	{},
};

static const APIInfo Lists_gdi32[] = {
#ifndef UNICODE_API_DISABLE
	{ "AddFontResourceExW", (void **)&pAddFontResourceExW },
	{ "RemoveFontResourceExW", (void **)&pRemoveFontResourceExW },
#endif
	{},
};

static const APIInfo Lists_Shcore[] = {
	{ "GetDpiForMonitor", (void **)&pGetDpiForMonitor },
	{},
};

static const APIInfo Lists_kernel32[] = {
#ifndef UNICODE_API_DISABLE
	{ "OutputDebugStringW", (void **)&pOutputDebugStringW },
	{ "ExpandEnvironmentStringsW", (void **)&pExpandEnvironmentStringsW },
#endif
	{ "GetConsoleWindow", (void **)&pGetConsoleWindow },
	{ "VerSetConditionMask", (void **)&pVerSetConditionMask },
	{ "VerifyVersionInfoA", (void **)&pVerifyVersionInfoA },
	{ "SetDefaultDllDirectories", (void **)&pSetDefaultDllDirectories },
	{ "SetDllDirectoryA", (void **)&pSetDllDirectoryA },
	{},
};

static const APIInfo Lists_hhctrl[] = {
#ifndef UNICODE_API_DISABLE
	{ "HtmlHelpW", (void **)&pHtmlHelpW },
#endif
	{ "HtmlHelpA", (void **)&pHtmlHelpA },
	{},
};

static const APIInfo Lists_dnsapi[] = {
	{ "DnsQuery_A", (void **)&pDnsQuery_A },
	{ "DnsFree", (void **)&pDnsFree },
	{},
};

static const APIInfo Lists_imagehlp[] = {
	{ "SymGetLineFromAddr", (void **)&pSymGetLineFromAddr },
	{},
};

static const APIInfo Lists_dbghelp[] = {
	{ "MiniDumpWriteDump", (void **)&pMiniDumpWriteDump },
	{},
};

static const APIInfo Lists_shell32[] = {
	{ "SHGetKnownFolderPath", (void **)&pSHGetKnownFolderPath },
	{},
};

static const APIInfo Lists_comctl32[] = {
	{ "LoadIconWithScaleDown", (void **)&pLoadIconWithScaleDown },
	{},
};

static const DllInfo DllInfos[] = {
	{ L"user32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_user32 },
	{ L"msimg32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_msimg32 },
	{ L"gdi32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_gdi32 },
	{ L"Shcore.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_Shcore },
	{ L"kernel32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_kernel32 },
	{ L"hhctrl.ocx", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_hhctrl },
	{ L"dnsapi.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_dnsapi },
	{ L"imagehlp.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_imagehlp },
	{ L"dbghelp.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_dbghelp },
	{ L"shell32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_shell32 },
	{ L"comctl32.dll", DLL_LOAD_LIBRARY_SxS, DLL_ACCEPT_NOT_EXIST, Lists_comctl32 },
	{},
};

static bool IsWindowsNTKernel()
{
	OSVERSIONINFOA osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionExA(&osvi);
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		// Windows 9x
		return false;
	}
	else {
		return true;
	}
}

static bool IsWindowsNT4()
{
	OSVERSIONINFOA osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionExA(&osvi);
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		osvi.dwMajorVersion == 4 &&
		osvi.dwMinorVersion == 0) {
		// NT4
		return true;
	}
	return false;
}

void WinCompatInit()
{
	static BOOL done = FALSE;
	if (done) return;
	done = TRUE;

#if _WIN32_IE >= 0x600
	// _WIN32_IE < 0x600 のとき guidItem が使用できず
	// FIELD_OFFSET(NOTIFYICONDATAA, guidItem) などがエラーとなるので
	// >= 0x600 のときのみ
	assert(sizeof(TT_NOTIFYICONDATAA_V2) == NOTIFYICONDATAA_V2_SIZE);
	assert(sizeof(TT_NOTIFYICONDATAW_V2) == NOTIFYICONDATAW_V2_SIZE);
#endif
#if defined(_WIN64)
	assert(sizeof(TT_NOTIFYICONDATAA_V2) == 504);
	assert(sizeof(TT_NOTIFYICONDATAW_V2) == 952);
#else
	assert(sizeof(TT_NOTIFYICONDATAA_V2) == 488);
	assert(sizeof(TT_NOTIFYICONDATAW_V2) == 936);
#endif

	DLLGetApiAddressFromLists(DllInfos);

	// 9x特別処理
	if (!IsWindowsNTKernel()) {
		// Windows 9x に存在しているが正しく動作しないため無効化する
		pOutputDebugStringW = NULL;
		pExpandEnvironmentStringsW = NULL;
	}

	// GetConsoleWindow特別処理
	if (pGetConsoleWindow == NULL) {
		pGetConsoleWindow = GetConsoleWindowLocal;
	}
}

HWND _HtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
	if (pHtmlHelpW != NULL) {
		return pHtmlHelpW(hwndCaller, pszFile, uCommand, dwData);
	}

	if (pHtmlHelpA != NULL) {
		char *fileA = ToCharW(pszFile);
		HWND r = pHtmlHelpA(hwndCaller, fileA, uCommand, dwData);
		free(fileA);
		return r;
	}

	// error
	return NULL;
}

static BOOL vercmp(
	DWORD cond_val,
	DWORD act_val,
	DWORD dwTypeMask)
{
	switch (dwTypeMask) {
	case VER_EQUAL:
		if (act_val == cond_val) {
			return TRUE;
		}
		break;
	case VER_GREATER:
		if (act_val > cond_val) {
			return TRUE;
		}
		break;
	case VER_GREATER_EQUAL:
		if (act_val >= cond_val) {
			return TRUE;
		}
		break;
	case VER_LESS:
		if (act_val < cond_val) {
			return TRUE;
		}
		break;
	case VER_LESS_EQUAL:
		if (act_val <= cond_val) {
			return TRUE;
		}
		break;
	}
	return FALSE;
}

/*
DWORDLONG dwlConditionMask
| 000 | 000 | 000 | 000 | 000 | 000 | 000 | 000 |
   |     |     |     |     |     |     |     +- condition of dwMinorVersion
   |     |     |     |     |     |     +------- condition of dwMajorVersion
   |     |     |     |     |     +------------- condition of dwBuildNumber
   |     |     |     |     +------------------- condition of dwPlatformId
   |     |     |     +------------------------- condition of wServicePackMinor
   |     |     +------------------------------- condition of wServicePackMajor
   |     +------------------------------------- condition of wSuiteMask
   +------------------------------------------- condition of wProductType
*/
static BOOL _myVerifyVersionInfo(
	LPOSVERSIONINFOEX lpVersionInformation,
	DWORD dwTypeMask,
	DWORDLONG dwlConditionMask)
{
	OSVERSIONINFOA osvi;
	WORD cond;
	BOOL ret, check_next;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	GetVersionExA(&osvi);

	if (dwTypeMask & VER_BUILDNUMBER) {
		cond = (WORD)((dwlConditionMask >> (2*3)) & 0x07);
		if (!vercmp(lpVersionInformation->dwBuildNumber, osvi.dwBuildNumber, cond)) {
			return FALSE;
		}
	}
	if (dwTypeMask & VER_PLATFORMID) {
		cond = (WORD)((dwlConditionMask >> (3*3)) & 0x07);
		if (!vercmp(lpVersionInformation->dwPlatformId, osvi.dwPlatformId, cond)) {
			return FALSE;
		}
	}
	ret = TRUE;
	if (dwTypeMask & (VER_MAJORVERSION | VER_MINORVERSION)) {
		check_next = TRUE;
		if (dwTypeMask & VER_MAJORVERSION) {
			cond = (WORD)((dwlConditionMask >> (1*3)) & 0x07);
			if (cond == VER_EQUAL) {
				if (!vercmp(lpVersionInformation->dwMajorVersion, osvi.dwMajorVersion, cond)) {
					return FALSE;
				}
			}
			else {
				ret = vercmp(lpVersionInformation->dwMajorVersion, osvi.dwMajorVersion, cond);
				// ret: result of major version
				if (!vercmp(lpVersionInformation->dwMajorVersion, osvi.dwMajorVersion, VER_EQUAL)) {
					// !vercmp(...: result of GRATOR/LESS than (not "GRATOR/LESS than or equal to") of major version
					// e.g.
					//   lpvi:5.1 actual:5.0 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.1 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.2 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:6.0 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.1 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.2 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:5.0 cond:VER_GREATER        ret:FALSE !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.1 cond:VER_GREATER        ret:FALSE !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.2 cond:VER_GREATER        ret:FALSE !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:6.0 cond:VER_GREATER        ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.1 cond:VER_GREATER        ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.2 cond:VER_GREATER        ret:TRUE  !vercmp(...:TRUE   must not check minor
					check_next = FALSE;
				}
			}
		}
		if (check_next && (dwTypeMask & VER_MINORVERSION)) {
			cond = (WORD)((dwlConditionMask >> (0*3)) & 0x07);
			if (cond == VER_EQUAL) {
				if (!vercmp(lpVersionInformation->dwMinorVersion, osvi.dwMinorVersion, cond)) {
					return FALSE;
				}
			}
			else {
				ret = vercmp(lpVersionInformation->dwMinorVersion, osvi.dwMinorVersion, cond);
			}
		}
	}
	return ret;
}

// Windows 2000+
BOOL _VerifyVersionInfoA(
	LPOSVERSIONINFOEXA lpVersionInformation,
	DWORD dwTypeMask,
	DWORDLONG dwlConditionMask)
{
	if (pVerifyVersionInfoA != NULL) {
		return pVerifyVersionInfoA(lpVersionInformation, dwTypeMask, dwlConditionMask);
	}

	return _myVerifyVersionInfo(lpVersionInformation, dwTypeMask, dwlConditionMask);
}

static ULONGLONG _myVerSetConditionMask(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask)
{
	ULONGLONG result, mask;
	BYTE op = dwConditionMask & 0x07;

	switch (dwTypeBitMask) {
		case VER_MINORVERSION:
			mask = 0x07 << (0 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (0 * 3);
			break;
		case VER_MAJORVERSION:
			mask = 0x07 << (1 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (1 * 3);
			break;
		case VER_BUILDNUMBER:
			mask = 0x07 << (2 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (2 * 3);
			break;
		case VER_PLATFORMID:
			mask = 0x07 << (3 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (3 * 3);
			break;
		case VER_SERVICEPACKMINOR:
			mask = 0x07 << (4 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (4 * 3);
			break;
		case VER_SERVICEPACKMAJOR:
			mask = 0x07 << (5 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (5 * 3);
			break;
		case VER_SUITENAME:
			mask = 0x07 << (6 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (6 * 3);
			break;
		case VER_PRODUCT_TYPE:
			mask = 0x07 << (7 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (7 * 3);
			break;
		default:
			result = 0;
			break;
	}

	return result;
}

// Windows 2000+
ULONGLONG _VerSetConditionMask(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask)
{
	if (pVerSetConditionMask != NULL) {
		return pVerSetConditionMask(dwlConditionMask, dwTypeBitMask, dwConditionMask);
	}
	return _myVerSetConditionMask(dwlConditionMask, dwTypeBitMask, dwConditionMask);
}

static BOOL GetCSIDLFromFKNOWNFOLDERID(REFKNOWNFOLDERID rfid, int *csidl)
{
	static const struct {
		REFKNOWNFOLDERID rfid;
		int csidl;
	} list[] = {
		{ FOLDERID_PublicDesktop, CSIDL_COMMON_DESKTOPDIRECTORY },
		{ FOLDERID_CommonStartMenu, CSIDL_COMMON_STARTMENU },
		{ FOLDERID_CommonPrograms, CSIDL_COMMON_PROGRAMS },
		{ FOLDERID_CommonStartup, CSIDL_COMMON_STARTUP },
		{ FOLDERID_Desktop, CSIDL_DESKTOPDIRECTORY },
		{ FOLDERID_Favorites, CSIDL_FAVORITES },
		{ FOLDERID_Fonts, CSIDL_FONTS },
		{ FOLDERID_Documents, CSIDL_MYDOCUMENTS },	// %USERPROFILE%\My Documents
		{ FOLDERID_NetHood, CSIDL_NETHOOD },
		{ FOLDERID_PrintHood, CSIDL_PRINTHOOD },
		{ FOLDERID_Programs, CSIDL_PROGRAMS },
		{ FOLDERID_Recent, CSIDL_RECENT },
		{ FOLDERID_SendTo, CSIDL_SENDTO },
		{ FOLDERID_StartMenu, CSIDL_STARTMENU },
		{ FOLDERID_Startup, CSIDL_STARTUP },
		{ FOLDERID_Templates, CSIDL_TEMPLATES },
		{ FOLDERID_LocalAppData, CSIDL_LOCAL_APPDATA },
		{ FOLDERID_Downloads, CSIDL_MYDOCUMENTS },	// %USERPROFILE%\Downloads, My Documents
	};

	for (size_t i = 0; i < _countof(list); i++) {
		if (IsEqualGUID(rfid, list[i].rfid)) {
			*csidl = list[i].csidl;
			return TRUE;
		}
	}
	return FALSE;
}

/**
 *	SHGetKnownFolderPath() の互換関数
 *
 *	@param[out]	ppszPath	パス
 *							CoTaskMemFree() ではなく、free() を使って開放する
 *							エラーに関係なく free() すること
 */
HRESULT _SHGetKnownFolderPath(REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR* ppszPath)
{
	if (pSHGetKnownFolderPath != NULL) {
		// Vista+
		wchar_t *path;
		HRESULT r = pSHGetKnownFolderPath(rfid, dwFlags, hToken, &path);
		*ppszPath = _wcsdup(path);
		CoTaskMemFree(path);	// エラーに関係なく呼び出す必要あり
		return r;
	}

	int csidl;
	if (GetCSIDLFromFKNOWNFOLDERID(rfid, &csidl) == FALSE) {
		*ppszPath = _wcsdup(L"");
		return E_FAIL;
	}
	wchar_t path[MAX_PATH];
#if 0
	// SHGetSpecialFolderLocation() でカバーできる?
	if (pSHGetSpecialFolderPathW != NULL) {
		BOOL create = (dwFlags & KF_FLAG_CREATE) != 0 ? TRUE : FALSE;
		BOOL r = SHGetSpecialFolderPathW(NULL, path, csidl, create);
		if (!r) {
			path[0] = 0;
		}
		*ppszPath = _wcsdup(path);
		return r ? S_OK : E_FAIL;
	}
#endif
	LPITEMIDLIST pidl;
	HRESULT r = SHGetSpecialFolderLocation(NULL, csidl, &pidl);
	if (r != NOERROR) {
		*ppszPath = _wcsdup(L"");
		return E_FAIL;
	}
	SHGetPathFromIDListW(pidl, path);
	CoTaskMemFree(pidl);
	*ppszPath = _wcsdup(path);
	return S_OK;
}

HRESULT _LoadIconWithScaleDown(HINSTANCE hinst, PCWSTR pszName, int cx, int cy, HICON *phico)
{
	if (pLoadIconWithScaleDown != NULL) {
		HRESULT hr = pLoadIconWithScaleDown(hinst, pszName, cx, cy, phico);
		if (SUCCEEDED(hr)) {
			return hr;
		}
	}

	HICON hIcon;
	int fuLoad = LR_DEFAULTCOLOR;
	if (IsWindowsNT4()) {
		// Windows NT 4.0 は 4bit アイコンしかサポートしていない
		// 16(4bit) color = VGA color
		fuLoad = LR_VGACOLOR;
	}
	hIcon = (HICON)LoadImageW(hinst, pszName, IMAGE_ICON, cx, cy, fuLoad);
	if (hIcon == NULL) {
		*phico = NULL;
		return E_NOTIMPL;
	}
	*phico = hIcon;
	return S_OK;
}
