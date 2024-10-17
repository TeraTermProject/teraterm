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
#include <devguid.h>

#include "compat_win.h"
#include "compat_windns.h"

#include "dllutil.h"
#include "codeconv.h"

#if defined(_MSC_VER)
#pragma comment(lib, "setupapi.lib")
#endif

/*
 *	devpkey.h �������?
 *		HAS_DEVPKEY_H �� define �����
 */
#if	defined(_MSC_VER)
#if	(_MSC_VER > 1400)

// VS2019�̂Ƃ�(VS2005���傫���Ƃ��Ă���)
#define HAS_DEVPKEY_H	1

#else // _MSC_VER > 1400

// VS2008�̂Ƃ�
#if defined(_INC_SDKDDKVER)

// VS2008 + SDK 7.0�ł͂Ȃ��Ƃ�(SDK 7.1�̂Ƃ�)
//   SDK 7.0 �̏ꍇ�� sdkddkver.h �� include ����Ă��Ȃ�
#define HAS_DEVPKEY_H	1

#endif  //  defined(_INC_SDKDDKVER)
#endif
#elif defined(__MINGW32__)

#if	__MINGW64_VERSION_MAJOR >= 8
// mingw64 8+ �̂Ƃ�
#define HAS_DEVPKEY_H	1
#endif

#endif  // defined(_MSC_VER)

/*
 *	devpkey.h �� include
 */
#if defined(HAS_DEVPKEY_H)

#define INITGUID
#include <devpkey.h>

#else //  defined(HAS_DEVPKEY_H)

#include "devpkey_teraterm.h"

#endif //  defined(HAS_DEVPKEY_H)

#define INITGUID
#include <guiddef.h>

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
HDEVNOTIFY (WINAPI *pRegisterDeviceNotificationA)(HANDLE hRecipient, LPVOID NotificationFilter, DWORD Flags);
BOOL (WINAPI *pUnregisterDeviceNotification)(HDEVNOTIFY Handle);

// kernel32
void (WINAPI *pOutputDebugStringW)(LPCWSTR lpOutputString);
HWND (WINAPI *pGetConsoleWindow)(void);
DWORD (WINAPI *pExpandEnvironmentStringsW)(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize);
static ULONGLONG (WINAPI *pVerSetConditionMask)(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask);
static BOOL (WINAPI *pVerifyVersionInfoA)(LPOSVERSIONINFOEXA lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask);
BOOL (WINAPI *pSetDefaultDllDirectories)(DWORD DirectoryFlags);
BOOL (WINAPI *pSetDllDirectoryA)(LPCSTR lpPathName);
static BOOL (WINAPI *pGetVersionExA)(LPOSVERSIONINFOA lpVersionInformation);
LANGID (WINAPI *pGetUserDefaultUILanguage)(void);

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
HRESULT (WINAPI *pSHCreateItemFromParsingName)(PCWSTR pszPath, IBindCtx *pbc, REFIID riid, void **ppv);

// comctl32.dll
static HRESULT (WINAPI *pLoadIconWithScaleDown)(HINSTANCE hinst, PCWSTR pszName, int cx, int cy, HICON *phico);

// dwmapi.dll
HRESULT (WINAPI *pDwmSetWindowAttribute)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
HRESULT (WINAPI *pDwmGetWindowAttribute)(HWND hwnd, DWORD dwAttribute, PVOID pvAttribute, DWORD cbAttribute);

// msimg32.dll
BOOL (WINAPI *pTransparentBlt)(HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest, HDC hdcSrc,
							   int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, UINT crTransparent);

// advapi32.dll
LSTATUS (WINAPI *pRegQueryValueExW)(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved,
									LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);

// setupapi.dll
BOOL (WINAPI *pSetupDiGetDevicePropertyW)(
	HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, CONST DEVPROPKEY *PropertyKey,
	DEVPROPTYPE *PropertyType, PBYTE PropertyBuffer, DWORD PropertyBufferSize,
	PDWORD RequiredSize, DWORD Flags);

BOOL (WINAPI *pSetupDiGetDeviceRegistryPropertyW)(
	HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
	DWORD Property, PDWORD PropertyRegDataType,
	PBYTE PropertyBuffer, DWORD PropertyBufferSize, PDWORD RequiredSize);

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
 *	GetConsoleWindow() �Ɠ������������
 *	 https://support.microsoft.com/ja-jp/help/124103/how-to-obtain-a-console-window-handle-hwnd
 */
static HWND WINAPI GetConsoleWindowLocal(void)
{
#define MY_BUFSIZE 1024					 // Buffer size for console window titles.
	HWND hwndFound;						 // This is what is returned to the caller.
	char pszNewWindowTitle[MY_BUFSIZE];  // Contains fabricated WindowTitle.
	char pszOldWindowTitle[MY_BUFSIZE];  // Contains original WindowTitle.

	// Fetch current window title.
	DWORD size = GetConsoleTitleA(pszOldWindowTitle, MY_BUFSIZE);
	if (size == 0) {
		DWORD err = GetLastError();
		if (err == ERROR_INVALID_HANDLE) {
			// �R���\�[�����J���Ă��Ȃ�
			return NULL;
		}
	}

	// Format a "unique" NewWindowTitle.
	wsprintfA(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

	// Change current window title.
	SetConsoleTitleA(pszNewWindowTitle);

	// Ensure window title has been updated.
	Sleep(40);

	// Look for NewWindowTitle.
	hwndFound = FindWindowA(NULL, pszNewWindowTitle);

	// Restore original window title.
	SetConsoleTitleA(pszOldWindowTitle);

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
	{ "RegisterDeviceNotificationA", (void **)&pRegisterDeviceNotificationA },
	{ "UnregisterDeviceNotification", (void **)&pUnregisterDeviceNotification },
	{},
};

static const APIInfo Lists_msimg32[] = {
	{ "AlphaBlend", (void **)&pAlphaBlend },
	{ "TransparentBlt", (void **)&pTransparentBlt },
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
	{ "GetUserDefaultUILanguage", (void **)&pGetUserDefaultUILanguage },
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
	{ "SHCreateItemFromParsingName", (void **)&pSHCreateItemFromParsingName },
	{},
};

static const APIInfo Lists_comctl32[] = {
	{ "LoadIconWithScaleDown", (void **)&pLoadIconWithScaleDown },
	{},
};

static const APIInfo Lists_dwmapi[] = { // Windows Vista or later
	{ "DwmSetWindowAttribute", (void **)&pDwmSetWindowAttribute },
	{ "DwmGetWindowAttribute", (void **)&pDwmGetWindowAttribute },
	{},
};

static const APIInfo Lists_advapi32[] = {
	{ "RegQueryValueExW", (void **)&pRegQueryValueExW },
	{},
};

static const APIInfo Lists_setupapi[] = {
	{ "SetupDiGetDevicePropertyW", (void **)&pSetupDiGetDevicePropertyW },
	{ "SetupDiGetDeviceRegistryPropertyW", (void **)&pSetupDiGetDeviceRegistryPropertyW},
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
	{ L"dwmapi.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_dwmapi },
	{ L"advapi32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_advapi32 },
	{ L"setupapi.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_setupapi },
	{},
};

/* OS version with GetVersionEx(*1)

                dwMajorVersion   dwMinorVersion    dwPlatformId
Windows95       4                0                 VER_PLATFORM_WIN32_WINDOWS
Windows98       4                10                VER_PLATFORM_WIN32_WINDOWS
WindowsMe       4                90                VER_PLATFORM_WIN32_WINDOWS
WindowsNT4.0    4                0                 VER_PLATFORM_WIN32_NT
Windows2000     5                0                 VER_PLATFORM_WIN32_NT
WindowsXP       5                1                 VER_PLATFORM_WIN32_NT
WindowsXPx64    5                2                 VER_PLATFORM_WIN32_NT
WindowsVista    6                0                 VER_PLATFORM_WIN32_NT
Windows7        6                1                 VER_PLATFORM_WIN32_NT
Windows8        6                2                 VER_PLATFORM_WIN32_NT
Windows8.1(*2)  6                2                 VER_PLATFORM_WIN32_NT
Windows8.1(*3)  6                3                 VER_PLATFORM_WIN32_NT
Windows10(*2)   6                2                 VER_PLATFORM_WIN32_NT
Windows10(*3)   10               0                 VER_PLATFORM_WIN32_NT

(*1) GetVersionEx()�� c4996 warning �ƂȂ�̂́AVS2013(_MSC_VER=1800) ����ł��B
(*2) manifest�� supportedOS Id ��ǉ����Ă��Ȃ��B
(*3) manifest�� supportedOS Id ��ǉ����Ă���B
*/
static BOOL _GetVersionExA(LPOSVERSIONINFOA lpVersionInformation)
{
	static OSVERSIONINFOA VersionInformation;
	if (pGetVersionExA == NULL) {
		// �G���[���Ԃ邱�Ƃ͂Ȃ� (2022-08-04)
		VersionInformation = *lpVersionInformation;
		void **func = (void **) & pGetVersionExA;
		DLLGetApiAddress(L"kernel32.dll", DLL_LOAD_LIBRARY_SYSTEM, "GetVersionExA", func);
		pGetVersionExA(&VersionInformation);
	}
	*lpVersionInformation = VersionInformation;
	return TRUE;
}

static bool IsWindowsNTKernel()
{
	OSVERSIONINFOA osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	_GetVersionExA(&osvi);
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
	_GetVersionExA(&osvi);
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
	// _WIN32_IE < 0x600 �̂Ƃ� guidItem ���g�p�ł���
	// FIELD_OFFSET(NOTIFYICONDATAA, guidItem) �Ȃǂ��G���[�ƂȂ�̂�
	// >= 0x600 �̂Ƃ��̂�
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

	// 9x���ʏ���
	if (!IsWindowsNTKernel()) {
		// Windows 9x �ɑ��݂��Ă��邪���������삵�Ȃ����ߖ���������
		pOutputDebugStringW = NULL;
		pExpandEnvironmentStringsW = NULL;
		pRegQueryValueExW = NULL;
		pSetupDiGetDeviceRegistryPropertyW = NULL;
	}

	// GetConsoleWindow���ʏ���
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
	LPOSVERSIONINFOEXA lpVersionInformation,
	DWORD dwTypeMask,
	DWORDLONG dwlConditionMask)
{
	OSVERSIONINFOA osvi;
	WORD cond;
	BOOL ret, check_next;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	_GetVersionExA(&osvi);

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
		// %APPDATA%, %USERPROFILE%\AppData\Roaming, %USERPROFILE%\Application Data
		{ FOLDERID_RoamingAppData, CSIDL_APPDATA },
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
 *	SHGetKnownFolderPath() �̌݊��֐�
 *
 *	@param[out]	ppszPath	�p�X
 *							CoTaskMemFree() �ł͂Ȃ��Afree() ���g���ĊJ������
 *							�G���[�Ɋ֌W�Ȃ� free() ���邱��
 */
HRESULT _SHGetKnownFolderPath(REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR* ppszPath)
{
	if (pSHGetKnownFolderPath != NULL) {
		// Vista+
		wchar_t *path;
		HRESULT r = pSHGetKnownFolderPath(rfid, dwFlags, hToken, &path);
		*ppszPath = _wcsdup(path);
		CoTaskMemFree(path);	// �G���[�Ɋ֌W�Ȃ��Ăяo���K�v����
		return r;
	}

	// �g�pAPI�� [ttssh2-dev 28] �Q��
	int csidl;
	if (GetCSIDLFromFKNOWNFOLDERID(rfid, &csidl) == FALSE) {
		*ppszPath = _wcsdup(L"");
		return E_FAIL;
	}
	wchar_t path[MAX_PATH];
#if 0
	// SHGetSpecialFolderLocation() �ŃJ�o�[�ł���?
	if (pSHGetSpecialFolderPathW != NULL) {
		BOOL create = (dwFlags & KF_FLAG_CREATE) != 0 ? TRUE : FALSE;
		BOOL r = SHGetSpecialFolderPathW(NULL, path, csidl, create);
		if (!r) {
			path[0] = 0;
		} else {
			if ((dwFlags & KF_FLAG_CREATE) != 0) {
				CreateDirectoryW(path, NULL);
			}
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
	if ((dwFlags & KF_FLAG_CREATE) != 0) {
		CreateDirectoryW(path, NULL);
	}
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
		// Windows NT 4.0 �� 4bit �A�C�R�������T�|�[�g���Ă��Ȃ�
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

static DWORD SearchProperty(const DEVPROPKEY *PropertyKey)
{
	static const struct {
		const DEVPROPKEY *PropertyKey;	// for SetupDiGetDeviceProperty() Vista+
		DWORD Property;					// for SetupDiGetDeviceRegistryProperty() 2000+
	} list[] = {
		{ &DEVPKEY_Device_FriendlyName, SPDRP_FRIENDLYNAME },
		{ &DEVPKEY_Device_Class, SPDRP_CLASS },
		{ &DEVPKEY_Device_InstanceId, SPDRP_MAXIMUM_PROPERTY },
		{ &DEVPKEY_Device_Manufacturer, SPDRP_MFG },
	};

	for (int i = 0; i < _countof(list); i++) {
		if (list[i].PropertyKey == PropertyKey) {
			return list[i].Property;
		}
	}
	return SPDRP_MAXIMUM_PROPERTY;
}

BOOL _SetupDiGetDevicePropertyW(
	HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
	const DEVPROPKEY *PropertyKey, DEVPROPTYPE *PropertyType,
	PBYTE PropertyBuffer, DWORD PropertyBufferSize,
	PDWORD RequiredSize, DWORD Flags)
{
	BOOL r;
	if (pSetupDiGetDevicePropertyW != NULL) {
		r = pSetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData,
									 PropertyKey, PropertyType,
									  PropertyBuffer, PropertyBufferSize,
									  RequiredSize, Flags);
		return r;
	}

	//	Windows 7 �ȑO�p
	if (PropertyKey == &DEVPKEY_Device_InstanceId) {
		// InstanceId��A�n�Ō��ߑł�
		DWORD len_a;
		r = SetupDiGetDeviceInstanceIdA(DeviceInfoSet,
										DeviceInfoData,
										NULL, 0,
										&len_a);
		if (r == FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			return FALSE;
		}
		char *str = (char *)malloc(len_a);
		r = SetupDiGetDeviceInstanceIdA(DeviceInfoSet,
										DeviceInfoData,
										str, len_a,
										&len_a);
		wchar_t *strW = ToWcharA(str);
		free(str);
		DWORD len_w = (DWORD)((wcslen(strW) + 1) * sizeof(wchar_t));  // +1 �� L'\0'
		*RequiredSize = len_w;
		if (PropertyBuffer != NULL) {
			wcscpy((wchar_t *)PropertyBuffer, strW);
		}
		*PropertyType = DEVPROP_TYPE_STRING;
		free(strW);
		return TRUE;
	}

	DWORD property_key = SearchProperty(PropertyKey);
	if (property_key == SPDRP_MAXIMUM_PROPERTY) {
		RequiredSize = 0;
		if (PropertyBuffer != NULL && PropertyBufferSize > 1) {
			PropertyBuffer[0] = 0;
		}
		return FALSE;
	}

	if (pSetupDiGetDeviceRegistryPropertyW != NULL) {
		r = pSetupDiGetDeviceRegistryPropertyW(DeviceInfoSet,
											  DeviceInfoData, property_key,
											  NULL,
											  PropertyBuffer, PropertyBufferSize,
											  RequiredSize);
		*PropertyType = DEVPROP_TYPE_STRING;
		return r;
	}
	else
	{
		DWORD len_a;
		r = SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet,
											  DeviceInfoData, property_key, NULL,
											  NULL, 0,
											  &len_a);
		if (r == FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			return FALSE;
		}
		char *str = (char *)malloc(len_a);
		r = SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet,
											  DeviceInfoData, property_key, NULL,
											  (PBYTE)str, len_a,
											  &len_a);
		wchar_t *strW = ToWcharA(str);
		free(str);
		DWORD len_w = (DWORD)((wcslen(strW) + 1) * sizeof(wchar_t));  // +1 �� L'\0'
		*RequiredSize = len_w;
		if (PropertyBuffer != NULL) {
			wcscpy((wchar_t *)PropertyBuffer, strW);
		}
		*PropertyType = DEVPROP_TYPE_STRING;
		free(strW);
		return TRUE;
	}
}

/*
 *	TODO
 *		layer_for_unicode �֒u���̂����Ó���
 */
LSTATUS _RegQueryValueExW(
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData)
{
	if (pRegQueryValueExW != NULL) {
		return pRegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	}
	else {
		DWORD type = 0;
		DWORD len;
		BYTE *p = NULL;
		char *lpValueNameA = ToCharW(lpValueName);
		LSTATUS r = RegQueryValueExA(hKey, lpValueNameA, 0, &type, NULL, &len);
		if (r != ERROR_SUCCESS) {
			len = 0;
			goto finish;
		}
		p = (BYTE*)malloc(len);
		r = RegQueryValueExA(hKey, lpValueNameA, 0, &type, p, &len);
		if (r != ERROR_SUCCESS) {
			free(p);
			p = NULL;
			goto finish;
		}
		if (type == REG_SZ) {
			wchar_t *strW = ToWcharA((char *)p);
			free(p);
			p = (BYTE *)strW;
			len = (wcslen(strW) + 1) * sizeof(wchar_t);
		}
	finish:
		free(lpValueNameA);
		if (lpData != NULL) {
			if (p != NULL) {
				memcpy(lpData, p, len);
			}
		}
		free(p);
		if (lpType != NULL) {
			*lpType = type;
		}
		if (lpcbData != NULL) {
			*lpcbData = len;
		}
		return r;
	}
}
