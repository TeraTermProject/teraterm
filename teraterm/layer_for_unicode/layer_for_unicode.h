/*
 * Copyright (C) 2019- TeraTerm Project
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

/*
 * W to A Wrapper
 *
 * API–¼‚ÍW”Å‚Ì“ª‚É '_' ‚ð•t‚¯‚½‚à‚Ì‚ðŽg—p‚·‚é
 */

#pragma once

#include <windows.h>
#include <commdlg.h>	// for _GetOpenFileNameW()
#include <shlobj.h>	// for _SHBrowseForFolderW()

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI _SetWindowTextW(HWND hWnd, LPCWSTR lpString);
BOOL WINAPI _SetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString);
UINT WINAPI _GetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax);
LRESULT WINAPI _SendDlgItemMessageW(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI _SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
HWND WINAPI WINAPI _CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y,
							 int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
							 LPVOID lpParam);
ATOM WINAPI _RegisterClassW(const WNDCLASSW *lpWndClass);
int WINAPI _MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
BOOL WINAPI _InsertMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
BOOL WINAPI _AppendMenuW(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
int WINAPI _GetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount);
int WINAPI _GetWindowTextLengthW(HWND hWnd);
LONG WINAPI _SetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong);
LONG WINAPI _GetWindowLongW(HWND hWnd, int nIndex);
LONG_PTR WINAPI _SetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LONG_PTR WINAPI _GetWindowLongPtrW(HWND hWnd, int nIndex);
LRESULT WINAPI _CallWindowProcW(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int WINAPI _DrawTextW(HDC hdc, LPCWSTR lpchText, int cchText, LPRECT lprc, UINT format);

// kernel32.dll
DWORD WINAPI _GetFileAttributesW(LPCWSTR lpFileName);
DWORD WINAPI _GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer);
BOOL WINAPI _SetCurrentDirectoryW(LPCWSTR lpPathName);
void WINAPI _OutputDebugStringW(LPCWSTR lpOutputString);
DWORD WINAPI _GetPrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault,
								LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
BOOL WINAPI _WritePrivateProfileStringW(LPCWSTR lpAppName,LPCWSTR lpKeyName,LPCWSTR lpString,LPCWSTR lpFileName);
UINT WINAPI _GetPrivateProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName);
BOOL WINAPI _CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
					 LPSECURITY_ATTRIBUTES lpProcessAttributes,
					 LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
					 DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
					 LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
BOOL WINAPI _CopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);
BOOL WINAPI _DeleteFileW(LPCWSTR lpFileName);
BOOL WINAPI _MoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
HANDLE WINAPI _CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
					LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
					HANDLE hTemplateFile);
HANDLE WINAPI _FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
BOOL WINAPI _FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
BOOL WINAPI _RemoveDirectoryW(LPCWSTR lpPathName);
DWORD WINAPI _GetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR *lpFilePart);
HMODULE WINAPI _LoadLibraryW(LPCWSTR lpLibFileName);
DWORD WINAPI WINAPI _GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);
DWORD WINAPI _ExpandEnvironmentStringsW(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize);
HMODULE WINAPI _GetModuleHandleW(LPCWSTR lpModuleName);
UINT WINAPI _GetSystemDirectoryW(LPWSTR lpBuffer, UINT uSize);
DWORD WINAPI _GetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer);
UINT WINAPI _GetTempFileNameW(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName);

// gdi32.lib
int WINAPI _AddFontResourceW(LPCWSTR lpFileName);
BOOL WINAPI _RemoveFontResourceW(LPCWSTR lpFileName);

// Comctl32.lib
HPROPSHEETPAGE WINAPI _CreatePropertySheetPageW(LPCPROPSHEETPAGEW_V1 constPropSheetPagePointer);
INT_PTR WINAPI _PropertySheetW(PROPSHEETHEADERW *constPropSheetHeaderPointer);
//INT_PTR WINAPI _PropertySheetW(PROPSHEETHEADERW_V1 *constPropSheetHeaderPointer);

// Comdlg32.lib
BOOL WINAPI _GetOpenFileNameW(LPOPENFILENAMEW ofnW);
BOOL WINAPI _GetSaveFileNameW(LPOPENFILENAMEW ofnW);

// shell32.lib
UINT WINAPI _DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);
BOOL WINAPI _Shell_NotifyIconW(DWORD dwMessage, TT_NOTIFYICONDATAW_V2 *lpData);
LPITEMIDLIST WINAPI _SHBrowseForFolderW(LPBROWSEINFOW lpbi);
BOOL WINAPI _SHGetPathFromIDListW(LPITEMIDLIST pidl, LPWSTR pszPath);

HWND WINAPI _CreateDialogIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate,
								 HWND hWndParent, DLGPROC lpDialogFunc,
								 LPARAM dwInitParam);
INT_PTR WINAPI _DialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent,
								 DLGPROC lpDialogFunc, LPARAM lParamInit);

#ifdef __cplusplus
}
#endif
