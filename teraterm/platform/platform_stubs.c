/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * Stub/no-op implementations for Windows-only features on macOS.
 * These allow the codebase to link without implementing every
 * Windows API function that is referenced.
 */

#if defined(__APPLE__) || defined(__linux__)

#include "platform_macos_types.h"
#include <stdio.h>
#include <stdlib.h>

/* --- Registry stubs (Tera Term uses INI files, registry rarely used) --- */

LONG RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, DWORD samDesired, HKEY* phkResult) {
    (void)hKey; (void)lpSubKey; (void)ulOptions; (void)samDesired;
    if (phkResult) *phkResult = NULL;
    return 2; /* ERROR_FILE_NOT_FOUND */
}

LONG RegCloseKey(HKEY hKey) {
    (void)hKey;
    return 0;
}

LONG RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved,
                      LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    (void)hKey; (void)lpValueName; (void)lpReserved; (void)lpType; (void)lpData; (void)lpcbData;
    return 2; /* ERROR_FILE_NOT_FOUND */
}

/* --- DDE stubs (DDE not available on macOS) --- */

int DdeInitializeW(void* pidInst, void* pfnCallback, DWORD afCmd, DWORD ulRes) {
    (void)pidInst; (void)pfnCallback; (void)afCmd; (void)ulRes;
    return 1; /* DMLERR_DLL_USAGE - indicate DDE unavailable */
}

int DdeUninitialize(DWORD idInst) {
    (void)idInst;
    return 1;
}

/* --- GDI+ stubs --- */
HRESULT GdiplusStartup(void* token, void* input, void* output) {
    (void)token; (void)input; (void)output;
    return S_OK;
}

void GdiplusShutdown(void* token) {
    (void)token;
}

/* --- HTML Help stub --- */
HWND HtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData) {
    (void)hwndCaller; (void)pszFile; (void)uCommand; (void)dwData;
    return NULL;
}

/* --- Print stubs --- */
HDC CreateDCW(LPCWSTR pwszDriver, LPCWSTR pwszDevice, LPCWSTR pszPort, const void* pdm) {
    (void)pwszDriver; (void)pwszDevice; (void)pszPort; (void)pdm;
    return NULL;
}

BOOL DeleteDC(HDC hdc) {
    (void)hdc;
    return TRUE;
}

int StartDocW(HDC hdc, const void* lpdi) {
    (void)hdc; (void)lpdi;
    return -1; /* Error - printing not supported */
}

int EndDoc(HDC hdc) {
    (void)hdc;
    return 0;
}

int StartPage(HDC hdc) {
    (void)hdc;
    return -1;
}

int EndPage(HDC hdc) {
    (void)hdc;
    return 0;
}

/* --- Shared memory stubs (use mmap/shm on macOS) --- */
HANDLE CreateFileMappingW(HANDLE hFile, void* lpAttributes, DWORD flProtect,
                          DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName) {
    (void)hFile; (void)lpAttributes; (void)flProtect;
    (void)dwMaximumSizeHigh; (void)dwMaximumSizeLow; (void)lpName;
    /* TODO: implement with shm_open + mmap */
    return NULL;
}

LPVOID MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess,
                     DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, size_t dwNumberOfBytesToMap) {
    (void)hFileMappingObject; (void)dwDesiredAccess;
    (void)dwFileOffsetHigh; (void)dwFileOffsetLow; (void)dwNumberOfBytesToMap;
    return NULL;
}

BOOL UnmapViewOfFile(LPCVOID lpBaseAddress) {
    (void)lpBaseAddress;
    return TRUE;
}

/* --- WinSock async stubs --- */
HANDLE WSAAsyncGetHostByName(HWND hWnd, UINT wMsg, const char* name, char* buf, int buflen) {
    (void)hWnd; (void)wMsg; (void)name; (void)buf; (void)buflen;
    return NULL;
}

int WSAStartup(WORD wVersionRequested, void* lpWSAData) {
    (void)wVersionRequested; (void)lpWSAData;
    return 0;
}

int WSACleanup(void) {
    return 0;
}

/* --- Window management stubs --- */
HWND CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
                     DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                     HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    (void)dwExStyle; (void)lpClassName; (void)lpWindowName; (void)dwStyle;
    (void)X; (void)Y; (void)nWidth; (void)nHeight;
    (void)hWndParent; (void)hMenu; (void)hInstance; (void)lpParam;
    /* TODO: Bridge to Cocoa window creation */
    return NULL;
}

BOOL DestroyWindow(HWND hWnd) {
    (void)hWnd;
    return TRUE;
}

BOOL ShowWindow(HWND hWnd, int nCmdShow) {
    (void)hWnd; (void)nCmdShow;
    return TRUE;
}

BOOL UpdateWindow(HWND hWnd) {
    (void)hWnd;
    return TRUE;
}

LRESULT SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    (void)hWnd; (void)Msg; (void)wParam; (void)lParam;
    return 0;
}

LRESULT DefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    (void)hWnd; (void)Msg; (void)wParam; (void)lParam;
    return 0;
}

/* --- Timer --- */
UINT_PTR SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, void* lpTimerFunc) {
    (void)hWnd; (void)uElapse; (void)lpTimerFunc;
    /* TODO: Bridge to macOS timer */
    return nIDEvent;
}

BOOL KillTimer(HWND hWnd, UINT_PTR uIDEvent) {
    (void)hWnd; (void)uIDEvent;
    return TRUE;
}

/* --- Message box --- */
int MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
    (void)hWnd; (void)uType;
    /* Simple console fallback - will be replaced by Cocoa dialog */
    if (lpCaption) fwprintf(stderr, L"%ls: ", lpCaption);
    if (lpText) fwprintf(stderr, L"%ls\n", lpText);
    return 1; /* IDOK */
}

/* --- GetModuleFileName --- */
DWORD GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize) {
    (void)hModule;
    if (!lpFilename || nSize == 0) return 0;
    /* Get executable path using _NSGetExecutablePath on macOS */
    char path[1024];
    uint32_t size = sizeof(path);
    extern int _NSGetExecutablePath(char* buf, uint32_t* bufsize);
    if (_NSGetExecutablePath(path, &size) == 0) {
        mbstowcs(lpFilename, path, nSize);
        return (DWORD)wcslen(lpFilename);
    }
    lpFilename[0] = L'\0';
    return 0;
}

/* --- File operations --- */
DWORD GetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer) {
    if (!lpBuffer) return 0;
    const char* tmp = getenv("TMPDIR");
    if (!tmp) tmp = "/tmp";
    mbstowcs(lpBuffer, tmp, nBufferLength);
    return (DWORD)wcslen(lpBuffer);
}

BOOL CreateDirectoryW(LPCWSTR lpPathName, void* lpSecurityAttributes) {
    (void)lpSecurityAttributes;
    if (!lpPathName) return FALSE;
    char path[1024];
    wcstombs(path, lpPathName, sizeof(path));
    return mkdir(path, 0755) == 0 ? TRUE : FALSE;
}

#include <sys/stat.h>
BOOL PathFileExistsW(LPCWSTR pszPath) {
    if (!pszPath) return FALSE;
    char path[1024];
    wcstombs(path, pszPath, sizeof(path));
    struct stat st;
    return stat(path, &st) == 0 ? TRUE : FALSE;
}

/* --- String resources (stub) --- */
int LoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int cchBufferMax) {
    (void)hInstance; (void)uID;
    if (lpBuffer && cchBufferMax > 0) lpBuffer[0] = L'\0';
    return 0;
}

#endif /* __APPLE__ || __linux__ */
