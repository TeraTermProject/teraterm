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

/*
 * 使用している Windows SDK, Visual Studio の差をなくすためのファイル
 * windows.h などのファイルを include した後に include する
 */

#pragma once

#include <windows.h>
#include <imagehlp.h>	// for SymGetLineFromAddr()
#include <shlobj.h>		// for SHGetKnownFolderPath()
#include <setupapi.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE			((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE		((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2	((DPI_AWARENESS_CONTEXT)-4)
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#endif

#if !defined(DPI_ENUMS_DECLARED)
typedef enum MONITOR_DPI_TYPE {
    MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI = 1,
    MDT_RAW_DPI = 2,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
#endif

#if !defined(WM_DPICHANGED)
#define WM_DPICHANGED					0x02E0
#endif
#if !defined(WM_GETDPISCALEDSIZE)
#define WM_GETDPISCALEDSIZE             0x02E4
#endif
#if !defined(CF_INACTIVEFONTS)
#define CF_INACTIVEFONTS				0x02000000L
#endif
#if !defined(OPENFILENAME_SIZE_VERSION_400A)
#define OPENFILENAME_SIZE_VERSION_400A	76
#endif

// 10.0.22000 or later
#if !defined(_DWMAPI_H_)
#define _DWMAPI_H_
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
typedef enum  {
  DWMWCP_DEFAULT = 0,
  DWMWCP_DONOTROUND = 1,
  DWMWCP_ROUND = 2,
  DWMWCP_ROUNDSMALL = 3
} DWM_WINDOW_CORNER_PREFERENCE;
#endif

#if !defined(_WIN64)
#include <pshpack1.h>
#endif

/**
 *	NOTIFYICONDATA は define でサイズが変化する
 *	どんな環境でも変化しないよう定義
 *
 * Shlwapi.dll 5.0
 * 	Win98(ME?)+,2000+
 */
typedef struct {
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	char   szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	char   szInfo[256];
	union {
		UINT  uTimeout;
		UINT  uVersion;	 // used with NIM_SETVERSION, values 0, 3 and 4
	} DUMMYUNIONNAME;
	char   szInfoTitle[64];
	DWORD dwInfoFlags;
	//GUID guidItem;		// XP+ (V3)
	//HICON hBalloonIcon;	// Vista+ (V4)
} TT_NOTIFYICONDATAA_V2;

typedef struct {
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	wchar_t	 szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	wchar_t	 szInfo[256];
	union {
		UINT  uTimeout;
		UINT  uVersion;	 // used with NIM_SETVERSION, values 0, 3 and 4
	} DUMMYUNIONNAME;
	wchar_t	 szInfoTitle[64];
	DWORD dwInfoFlags;
	//GUID guidItem;		// XP+ (V3)
	//HICON hBalloonIcon;	// Vista+ (V4)
} TT_NOTIFYICONDATAW_V2;

#if !defined(_WIN64)
#include <poppack.h>
#endif

extern BOOL (WINAPI *pAlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
extern BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPCRECT,MONITORENUMPROC,LPARAM);
extern HMONITOR (WINAPI *pMonitorFromRect)(LPCRECT lprc, DWORD dwFlags);
extern DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
extern BOOL (WINAPI *pIsValidDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
extern UINT (WINAPI *pGetDpiForWindow)(HWND hwnd);
extern HRESULT (WINAPI *pGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
extern BOOL (WINAPI *pAdjustWindowRectEx)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
extern BOOL (WINAPI *pAdjustWindowRectExForDpi)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
extern BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
extern int (WINAPI *pAddFontResourceExW)(LPCWSTR name, DWORD fl, PVOID res);
extern BOOL (WINAPI *pRemoveFontResourceExW)(LPCWSTR name, DWORD fl, PVOID pdv);
extern HMONITOR (WINAPI *pMonitorFromWindow)(HWND hwnd, DWORD dwFlags);
extern HMONITOR (WINAPI *pMonitorFromPoint)(POINT pt, DWORD dwFlags);
extern HMONITOR (WINAPI *pMonitorFromRect)(LPCRECT lprc, DWORD dwFlags);
extern BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);
#ifdef _WIN64
extern LONG_PTR (WINAPI *pSetWindowLongPtrW)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
extern LONG_PTR (WINAPI *pGetWindowLongPtrW)(HWND hWnd, int nIndex);
#endif

// user32
extern int (WINAPI *pGetSystemMetricsForDpi)(int  nIndex, UINT dpi);
extern HDEVNOTIFY (WINAPI *pRegisterDeviceNotificationA)(HANDLE hRecipient, LPVOID NotificationFilter, DWORD Flags);
extern BOOL (WINAPI *pUnregisterDeviceNotification)(HDEVNOTIFY Handle);
extern HWINEVENTHOOK(WINAPI *pSetWinEventHook)(
	DWORD eventMin, DWORD eventMax, HMODULE hmodWinEventProc,
	WINEVENTPROC pfnWinEventProc, DWORD idProcess, DWORD idThread,
	DWORD dwFlags);
extern BOOL (WINAPI *pUnhookWinEvent)(HWINEVENTHOOK hWinEventHook);
BOOL WINAPI _GetComboBoxInfo(HWND hWndCombo, PCOMBOBOXINFO info);

// kernel32
extern void (WINAPI *pOutputDebugStringW)(LPCWSTR lpOutputString);
extern HWND (WINAPI *pGetConsoleWindow)(void);
ULONGLONG _VerSetConditionMask(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask);
BOOL _VerifyVersionInfoA(LPOSVERSIONINFOEXA lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask);
extern BOOL (WINAPI *pSetDefaultDllDirectories)(DWORD DirectoryFlags);
extern BOOL (WINAPI *pSetDllDirectoryA)(LPCSTR lpPathName);
extern LANGID (WINAPI *pGetUserDefaultUILanguage)(void);

#if !defined(LOAD_LIBRARY_SEARCH_SYSTEM32)
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#endif

// htmlhelp.dll (hhctrl.ocx)
HWND _HtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);

// imagehlp.dll
extern BOOL (WINAPI *pSymGetLineFromAddr)(HANDLE hProcess, DWORD dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE Line);

// dbghelp.dll
extern BOOL(WINAPI *pMiniDumpWriteDump)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
										PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
										PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
										PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

// shell32.dll
#if !defined(NTDDI_VERSION) || !defined(NTDDI_VISTA) || (NTDDI_VERSION < NTDDI_VISTA)
typedef GUID KNOWNFOLDERID;
#if !defined(REFKNOWNFOLDERID)
#ifdef __cplusplus
#define REFKNOWNFOLDERID const KNOWNFOLDERID &
#else // !__cplusplus
#define REFKNOWNFOLDERID const KNOWNFOLDERID * const
#endif // __cplusplus
#endif

#include "ttknownfolders.h"

typedef enum
{
	KF_FLAG_CREATE          = 0x00008000,
} KNOWN_FOLDER_FLAG;


#endif
HRESULT _SHGetKnownFolderPath(REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR* ppszPath);
extern HRESULT (WINAPI *pSHCreateItemFromParsingName)(PCWSTR pszPath, IBindCtx *pbc, REFIID riid, void **ppv);

// comctl32.dll
HRESULT _LoadIconWithScaleDown(HINSTANCE hinst, PCWSTR pszName, int cx, int cy, HICON *phico);

// dwmapi.dll
extern HRESULT (WINAPI *pDwmSetWindowAttribute)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
extern HRESULT (WINAPI *pDwmGetWindowAttribute)(HWND hwnd, DWORD dwAttribute, PVOID pvAttribute, DWORD cbAttribute);

// msimg32.dll
extern BOOL(WINAPI *pTransparentBlt)(HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest, HDC hdcSrc,
									 int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, UINT crTransparent);

// advapi32.dll
extern LSTATUS (WINAPI *pRegQueryValueExW)(
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData);

// setupapi.dll
extern BOOL (WINAPI *pSetupDiGetDevicePropertyW)(
	HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, CONST DEVPROPKEY *PropertyKey,
	DEVPROPTYPE *PropertyType, PBYTE PropertyBuffer, DWORD PropertyBufferSize,
	PDWORD RequiredSize, DWORD Flags);
extern BOOL (WINAPI *pSetupDiGetDeviceRegistryPropertyW)(
	HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
	DWORD Property, PDWORD PropertyRegDataType,
	PBYTE PropertyBuffer, DWORD PropertyBufferSize, PDWORD RequiredSize);
extern DWORD (WINAPI *pCM_Get_DevNode_Status)(
	PULONG pulStatus, PULONG pulProblemNumber, DWORD dnDevInst,
	ULONG ulFlags);
BOOL _SetupDiGetDevicePropertyW(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, const DEVPROPKEY *PropertyKey,
								DEVPROPTYPE *PropertyType, PBYTE PropertyBuffer, DWORD PropertyBufferSize,
								PDWORD RequiredSize, DWORD Flags);

// wintrust.dll
extern LONG (WINAPI *pWinVerifyTrust)(HWND hwnd, GUID *pgActionID, LPVOID pWVTData);

void WinCompatInit();

#ifdef __cplusplus
}
#endif
