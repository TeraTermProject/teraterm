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

#include "compat_win.h"
#include "compat_windns.h"

#include "dllutil.h"
#include "codeconv.h"

// for debug
//#define UNICODE_API_DISABLE	1

ATOM (WINAPI *pRegisterClassW)(const WNDCLASSW *lpWndClass);
HWND (WINAPI *pCreateWindowExW)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
 HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
LRESULT (WINAPI *pDefWindowProcW)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT (WINAPI *pSendMessageW)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT (WINAPI *pSendDlgItemMessageW)(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL (WINAPI *pModifyMenuW)(HMENU hMnu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
int(WINAPI *pGetMenuStringW)(HMENU hMenu, UINT uIDItem, LPWSTR lpString, int cchMax, UINT flags);
BOOL(WINAPI *pSetWindowTextW)(HWND hWnd, LPCWSTR lpString);
BOOL (WINAPI *pSetDlgItemTextW)(HWND hDlg, int nIDDlgItem, LPCWSTR lpString);
BOOL (WINAPI *pGetDlgItemTextW)(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax);
BOOL (WINAPI *pAlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
BOOL (WINAPI *pIsValidDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
UINT (WINAPI *pGetDpiForWindow)(HWND hwnd);
BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
HRESULT (WINAPI *pGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
BOOL (WINAPI *pAdjustWindowRectEx)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
BOOL (WINAPI *pAdjustWindowRectExForDpi)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
int (WINAPI *pMessageBoxW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
INT_PTR (WINAPI *pDialogBoxIndirectParamW)(HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
HWND (WINAPI *pCreateDialogIndirectParamW)(HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate,
										   HWND hWndParent, DLGPROC lpDialogFunc,
										   LPARAM dwInitParam);
LONG (WINAPI *pSetWindowLongW)(HWND hWnd, int nIndex, LONG dwNewLong);
LONG (WINAPI *pGetWindowLongW)(HWND hWnd, int nIndex);
#ifdef _WIN64
LONG_PTR (WINAPI *pSetWindowLongPtrW)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LONG_PTR (WINAPI *pGetWindowLongPtrW)(HWND hWnd, int nIndex);
#endif
LRESULT (WINAPI *pCallWindowProcW)(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
DWORD (WINAPI *pGetCurrentDirectoryW)(DWORD nBufferLength, LPWSTR lpBuffer);
BOOL (WINAPI *pSetCurrentDirectoryW)(LPCWSTR lpPathName);

// user32
int (WINAPI *pGetSystemMetricsForDpi)(int nIndex, UINT dpi);

// kernel32
DWORD (WINAPI *pGetFileAttributesW)(LPCWSTR lpFileName);
void (WINAPI *pOutputDebugStringW)(LPCWSTR lpOutputString);
HWND (WINAPI *pGetConsoleWindow)(void);
DWORD (WINAPI *pGetPrivateProfileStringW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault,
										 LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
BOOL (WINAPI *pWritePrivateProfileStringW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName);
UINT (WINAPI *pGetPrivateProfileIntW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName);
BOOL (WINAPI *pCreateProcessW)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
							   LPSECURITY_ATTRIBUTES lpProcessAttributes,
							   LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
							   DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
							   LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
BOOL (WINAPI *pCopyFileW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);
BOOL (WINAPI *pDeleteFileW)(LPCWSTR lpFileName);
BOOL (WINAPI *pMoveFileW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
HANDLE (WINAPI *pCreateFileW)(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
							  LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
							  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
HANDLE (WINAPI *pFindFirstFileW)(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
BOOL (WINAPI *pFindNextFileW)(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
BOOL (WINAPI *pRemoveDirectoryW)(LPCWSTR lpPathName);
DWORD (WINAPI *pGetFullPathNameW)(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR *lpFilePart);
HMODULE (WINAPI *pLoadLibraryW)(LPCWSTR lpLibFileName);
DWORD (WINAPI *pGetModuleFileNameW)(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);
DWORD (WINAPI *pExpandEnvironmentStringsW)(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize);
DWORD (WINAPI *pGetTempPathW)(DWORD nBufferLength, LPWSTR lpBuffer);
UINT (WINAPI *pGetTempFileNameW)(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName);
static ULONGLONG (WINAPI *pVerSetConditionMask)(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask);
static BOOL (WINAPI *pVerifyVersionInfoA)(LPOSVERSIONINFOEX lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask);

// gdi32
int (WINAPI *pAddFontResourceExW)(LPCWSTR name, DWORD fl, PVOID res);
BOOL (WINAPI *pRemoveFontResourceExW)(LPCWSTR name, DWORD fl, PVOID pdv);

// htmlhelp.dll (hhctrl.ocx)
static HWND (WINAPI *pHtmlHelpW)(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);
static HWND (WINAPI *pHtmlHelpA)(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);

BOOL (WINAPI *pInsertMenuW)(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
BOOL (WINAPI *pAppendMenuW)(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);

// multi monitor Windows98+/Windows2000+
BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPCRECT,MONITORENUMPROC,LPARAM);
HMONITOR (WINAPI *pMonitorFromWindow)(HWND hwnd, DWORD dwFlags);
HMONITOR (WINAPI *pMonitorFromPoint)(POINT pt, DWORD dwFlags);
HMONITOR (WINAPI *pMonitorFromRect)(LPCRECT lprc, DWORD dwFlags);
BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);

int (WINAPI *pGetWindowTextW)(HWND hWnd, LPWSTR lpString, int nMaxCount);
int (WINAPI *pGetWindowTextLengthW)(HWND hWnd);

// shell32
UINT (WINAPI *pDragQueryFileW)(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);
BOOL (WINAPI *pShell_NotifyIconW)(DWORD dwMessage, NOTIFYICONDATAW *lpData);
LPITEMIDLIST (WINAPI *pSHBrowseForFolderW)(LPBROWSEINFOW lpbi);
BOOL (WINAPI *pSHGetPathFromIDListW)(LPITEMIDLIST pidl, LPWSTR pszPath);

// comctl32
HPROPSHEETPAGE (WINAPI *pCreatePropertySheetPageW)(LPCPROPSHEETPAGEW constPropSheetPagePointer);
INT_PTR (WINAPI *pPropertySheetW)(LPCPROPSHEETHEADERW constPropSheetHeaderPointer);

// comdlg32
BOOL (WINAPI *pGetOpenFileNameW)(LPOPENFILENAMEW ofnW);
BOOL (WINAPI *pGetSaveFileNameW)(LPOPENFILENAMEW ofnW);

// dnsapi
DNS_STATUS (WINAPI *pDnsQuery_A)(PCSTR pszName, WORD wType, DWORD Options, PVOID pExtra, PDNS_RECORD *ppQueryResults, PVOID *pReserved);
VOID (WINAPI *pDnsFree)(PVOID pData, DNS_FREE_TYPE FreeType);

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
#ifndef UNICODE_API_DISABLE
	{ "RegisterClassW", (void **)&pRegisterClassW },
	{ "CreateWindowExW", (void **)&pCreateWindowExW },
	{ "DefWindowProcW", (void **)&pDefWindowProcW },
#endif
	{ "SetLayeredWindowAttributes", (void **)&pSetLayeredWindowAttributes },
	{ "SetThreadDpiAwarenessContext", (void **)&pSetThreadDpiAwarenessContext },
	{ "IsValidDpiAwarenessContext", (void **)&pIsValidDpiAwarenessContext },
	{ "GetDpiForWindow", (void **)&pGetDpiForWindow },
	{ "AdjustWindowRectEx", (void **)&pAdjustWindowRectEx },
	{ "AdjustWindowRectExForDpi", (void **)&pAdjustWindowRectExForDpi },
#ifndef UNICODE_API_DISABLE
	{ "SetDlgItemTextW", (void **)&pSetDlgItemTextW },
	{ "GetDlgItemTextW", (void **)&pGetDlgItemTextW },
	{ "SetWindowTextW", (void **)&pSetWindowTextW },
	{ "ModifyMenuW", (void **)&pModifyMenuW },
	{ "GetMenuStringW", (void **)&pGetMenuStringW },
	{ "SendDlgItemMessageW", (void **)&pSendDlgItemMessageW },
	{ "MessageBoxW", (void **)&pMessageBoxW },
	{ "DialogBoxIndirectParamW", (void **)&pDialogBoxIndirectParamW },
	{ "CreateDialogIndirectParamW", (void **)&pCreateDialogIndirectParamW },
	{ "InsertMenuW", (void **)&pInsertMenuW },
	{ "AppendMenuW", (void **)&pAppendMenuW },
	{ "SendMessageW", (void **)&pSendMessageW },
	{ "GetWindowTextW", (void **)&pGetWindowTextW },
	{ "GetWindowTextLengthW", (void **)&pGetWindowTextLengthW },
	{ "SetWindowLongW", (void **)&pSetWindowLongW },
	{ "GetWindowLongW", (void **)&pGetWindowLongW },
#ifdef _WIN64
	{ "SetWindowLongPtrW", (void **)&pSetWindowLongPtrW },
	{ "GetWindowLongPtrW", (void **)&pGetWindowLongPtrW },
#endif
	{ "CallWindowProcW", (void **)&pCallWindowProcW },
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
	{ "GetFileAttributesW", (void **)&pGetFileAttributesW },
	{ "OutputDebugStringW", (void **)&pOutputDebugStringW },
	{ "GetCurrentDirectoryW", (void **)&pGetCurrentDirectoryW },
	{ "SetCurrentDirectoryW", (void **)&pSetCurrentDirectoryW },
	{ "GetPrivateProfileStringW", (void **)&pGetPrivateProfileStringW },
	{ "WritePrivateProfileStringW", (void **)&pWritePrivateProfileStringW },
	{ "GetPrivateProfileIntW", (void **)&pGetPrivateProfileIntW },
	{ "CreateProcessW", (void **)&pCreateProcessW },
	{ "CopyFileW", (void **)&pCopyFileW },
	{ "DeleteFileW", (void **)&pDeleteFileW },
	{ "MoveFileW", (void **)&pMoveFileW },
	{ "CreateFileW", (void **)&pCreateFileW },
	{ "FindFirstFileW", (void **)&pFindFirstFileW },
	{ "FindNextFileW", (void **)&pFindNextFileW },
	{ "RemoveDirectoryW", (void **)&pRemoveDirectoryW },
	{ "GetFullPathNameW", (void **)&pGetFullPathNameW },
	{ "LoadLibraryW", (void **)&pLoadLibraryW },
	{ "GetModuleFileNameW", (void **)&pGetModuleFileNameW },
	{ "ExpandEnvironmentStringsW", (void **)&pExpandEnvironmentStringsW },
	{ "GetTempPathW", (void **)&pGetTempPathW },
	{ "GetTempFileNameW", (void **)&pGetTempFileNameW },
#endif
	{ "GetConsoleWindow", (void **)&pGetConsoleWindow },
	{ "VerSetConditionMask", (void **)&pVerSetConditionMask },
	{ "VerifyVersionInfoA", (void **)&pVerifyVersionInfoA },
	{},
};

static const APIInfo Lists_shell32[] = {
#ifndef UNICODE_API_DISABLE
	{ "DragQueryFileW", (void **)&pDragQueryFileW },
	{ "Shell_NotifyIconW", (void **)&pShell_NotifyIconW },
	{ "SHBrowseForFolderW", (void **)&pSHBrowseForFolderW },
	{ "SHGetPathFromIDListW", (void **)&pSHGetPathFromIDListW },
#endif
	{},
};

static const APIInfo Lists_comctl32[] = {
#ifndef UNICODE_API_DISABLE
	{ "CreatePropertySheetPageW", (void **)&pCreatePropertySheetPageW },
	{ "PropertySheetW", (void **)&pPropertySheetW },
#endif
	{},
};

static const APIInfo Lists_hhctrl[] = {
#ifndef UNICODE_API_DISABLE
	{ "HtmlHelpW", (void **)&pHtmlHelpW },
#endif
	{ "HtmlHelpA", (void **)&pHtmlHelpA },
	{},
};

static const APIInfo Lists_comdlg32[] = {
#ifndef UNICODE_API_DISABLE
	{ "GetOpenFileNameW", (void **)&pGetOpenFileNameW },
	{ "GetSaveFileNameW", (void **)&pGetSaveFileNameW },
#endif
	{},
};

static const APIInfo Lists_dnsapi[] = {
	{ "DnsQuery_A", (void **)&pDnsQuery_A },
	{ "DnsFree", (void **)&pDnsFree },
	{},
};

static const DllInfo DllInfos[] = {
	{ L"user32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_user32 },
	{ L"msimg32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_msimg32 },
	{ L"gdi32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_gdi32 },
	{ L"Shcore.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_Shcore },
	{ L"kernel32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_kernel32 },
	{ L"shell32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_shell32 },
	{ L"Comctl32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_comctl32 },
	{ L"hhctrl.ocx", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_hhctrl },
	{ L"comdlg32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_comdlg32 },
	{ L"dnsapi.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_dnsapi },
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

void WinCompatInit()
{
	static BOOL done = FALSE;
	if (done) return;
	done = TRUE;

	DLLGetApiAddressFromLists(DllInfos);

	// 9x特別処理
	if (!IsWindowsNTKernel()) {
		// Windows 9x に存在しているが正しく動作しないため無効化する
		pGetPrivateProfileStringW = NULL;
		pSetWindowTextW = NULL;
		pSetDlgItemTextW = NULL;
		pGetDlgItemTextW = NULL;
		pDialogBoxIndirectParamW = NULL;
		pCreateDialogIndirectParamW = NULL;
		pCreateWindowExW = NULL;
		pRegisterClassW = NULL;
		pDefWindowProcW = NULL;
		pSendMessageW = NULL;
		pSendDlgItemMessageW = NULL;
		pGetWindowTextW = NULL;
		pGetWindowTextLengthW = NULL;
		pShell_NotifyIconW = NULL;
		pGetFileAttributesW = NULL;
		pOutputDebugStringW = NULL;
		pDragQueryFileW = NULL;
		pGetCurrentDirectoryW = NULL;
		pSetCurrentDirectoryW = NULL;
		pGetOpenFileNameW = NULL;
		pSHBrowseForFolderW = NULL;
		pSHGetPathFromIDListW = NULL;
		pCreateProcessW = NULL;
		pCopyFileW = NULL;
		pDeleteFileW = NULL;
		pMoveFileW = NULL;
		pCreateFileW = NULL;
		pFindFirstFileW = NULL;
		pFindNextFileW = NULL;
		pRemoveDirectoryW = NULL;
		pGetFullPathNameW = NULL;
		pGetPrivateProfileIntW = NULL;
		pGetModuleFileNameW = NULL;
		pLoadLibraryW = NULL;
		pExpandEnvironmentStringsW = NULL;
		pGetTempPathW = NULL;
		pGetTempFileNameW = NULL;
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
	OSVERSIONINFO osvi;
	WORD cond;
	BOOL ret, check_next;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);

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
