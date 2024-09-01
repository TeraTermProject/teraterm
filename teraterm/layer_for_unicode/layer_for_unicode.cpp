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
 * API����W�ł̓��� '_' ��t�������̂��g�p����
 */

#include <windows.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "codeconv.h"
#include "compat_win.h"

#include "layer_for_unicode.h"

BOOL WINAPI _SetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString)
{
	char *strA = ToCharW(lpString);
	BOOL retval = SetDlgItemTextA(hDlg, nIDDlgItem, strA);
	free(strA);
	return retval;
}

UINT WINAPI _DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch)
{
	UINT retval;
	if (iFile == 0xffffffff) {
		// �t�@�C�����₢���킹
		retval = DragQueryFileA(hDrop, iFile, NULL, 0);
	}
	else if (lpszFile == NULL) {
		// �t�@�C�����̕������₢���킹
		char FileNameA[MAX_PATH];
		retval = DragQueryFileA(hDrop, iFile, FileNameA, MAX_PATH);
		if (retval != 0) {
			wchar_t *FileNameW = ToWcharA(FileNameA);
			retval = (UINT)(wcslen(FileNameW) + 1);
			free(FileNameW);
		}
	}
	else {
		// �t�@�C�����擾
		char FileNameA[MAX_PATH];
		retval = DragQueryFileA(hDrop, iFile, FileNameA, MAX_PATH);
		if (retval != 0) {
			wchar_t *FileNameW = ToWcharA(FileNameA);
			wcscpy_s(lpszFile, cch, FileNameW);
			free(FileNameW);
		}
	}
	return retval;
}

DWORD WINAPI _GetFileAttributesW(LPCWSTR lpFileName)
{
	char *FileNameA;
	if (lpFileName == NULL) {
		FileNameA = NULL;
	} else {
		FileNameA = ToCharW(lpFileName);
	}
	const DWORD attr = GetFileAttributesA(FileNameA);
	free(FileNameA);
	return attr;
}

/**
 * hWnd �ɐݒ肳��Ă��镶������擾
 *
 * @param[in]		hWnd
 * @param[in,out]	lenW	������(L'\0'���܂܂Ȃ�)
 * @return			������
 */
static wchar_t *SendMessageAFromW_WM_GETTEXT(HWND hWnd, size_t *lenW)
{
	// lenA = excluding the terminating null character.
	size_t lenA = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
	char *strA = (char *)malloc(lenA + 1);
	if (strA == NULL) {
		*lenW = 0;
		return NULL;
	}
	lenA = GetWindowTextA(hWnd, strA, (int)(lenA + 1));
	strA[lenA] = '\0';
	wchar_t *strW = ToWcharA(strA);
	free(strA);
	if (strW == NULL) {
		*lenW = 0;
		return NULL;
	}
	*lenW = wcslen(strW);
	return strW;
}

/**
 * hWnd(ListBox) �ɐݒ肳��Ă��镶������擾
 *
 * @param[in]		hWnd
 * @param[in]		wParam	���ڔԍ�(0�`)
 * @param[in,out]	lenW	������(L'\0'���܂܂Ȃ�)
 * @return			������
 */
static wchar_t *SendMessageAFromW_LB_GETTEXT(HWND hWnd, WPARAM wParam, size_t *lenW)
{
	// lenA = excluding the terminating null character.
	size_t lenA = SendMessageA(hWnd, LB_GETTEXTLEN, wParam, 0);
	char *strA = (char *)malloc(lenA + 1);
	if (strA == NULL) {
		*lenW = 0;
		return NULL;
	}
	lenA = SendMessageA(hWnd, LB_GETTEXT, wParam, (LPARAM)strA);
	wchar_t *strW = ToWcharA(strA);
	free(strA);
	if (strW == NULL) {
		*lenW = 0;
		return NULL;
	}
	*lenW = wcslen(strW);
	return strW;
}

int WINAPI _GetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
	size_t lenW;
	wchar_t *strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
	wchar_t *dest_ptr = (wchar_t *)lpString;
	size_t dest_len = (size_t)nMaxCount;
	wcsncpy_s(dest_ptr, dest_len, strW, _TRUNCATE);
	free(strW);
	return (int)(dest_len - 1);
}

int WINAPI _GetWindowTextLengthW(HWND hWnd)
{
	size_t lenW;// ������(`\0`�͊܂܂Ȃ�)
	wchar_t *strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
	free(strW);
	return (int)lenW;
}

LRESULT WINAPI _SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT retval;
	switch(Msg) {
	case CB_ADDSTRING:
	case CB_INSERTSTRING:
	case LB_ADDSTRING:
	case LB_INSERTSTRING: {
		char *strA = ToCharW((wchar_t *)lParam);
		retval = SendMessageA(hWnd, Msg, wParam, (LPARAM)strA);
		free(strA);
		return retval;
	}
	case WM_GETTEXTLENGTH:
	case LB_GETTEXTLEN: {
		size_t lenW;
		wchar_t *strW;
		if (Msg == WM_GETTEXTLENGTH) {
			strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
		}
		else {
			strW = SendMessageAFromW_LB_GETTEXT(hWnd, wParam, &lenW);
		}
		free(strW);
		return lenW;
	}
	case WM_GETTEXT:
	case LB_GETTEXT: {
		size_t lenW;
		wchar_t *strW;
		size_t dest_len;
		if (Msg == WM_GETTEXT) {
			strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
			dest_len = (size_t)wParam;
		}
		else {
			strW = SendMessageAFromW_LB_GETTEXT(hWnd, wParam, &lenW);
			dest_len = lenW + 1;
		}
		wchar_t *dest_ptr = (wchar_t *)lParam;
		wcsncpy_s(dest_ptr, dest_len, strW, _TRUNCATE);
		free(strW);
		return dest_len - 1 < lenW ? dest_len - 1 : lenW;
	}
	case BFFM_SETSELECTIONW: {
		if (wParam == TRUE) {
			// �t�H���_��I����Ԃɂ���
			char *folderA = ToCharW((wchar_t *)lParam);
			retval = SendMessageA(hWnd, BFFM_SETSELECTIONA, wParam, (LPARAM)folderA);
			free(folderA);
		}
		else {
			retval = SendMessageA(hWnd, Msg, wParam, lParam);
		}
		break;
	}
	default:
		retval = SendMessageA(hWnd, Msg, wParam, lParam);
		break;
	}
	return retval;
}

LRESULT WINAPI _SendDlgItemMessageW(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
	return _SendMessageW(hWnd, Msg, wParam, lParam);
}

HWND WINAPI _CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X,
									int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
									LPVOID lpParam)
{
	char *lpClassNameA = ToCharW(lpClassName);
	char *lpWindowNameA = ToCharW(lpWindowName);
	HWND hWnd = CreateWindowExA(dwExStyle, lpClassNameA, lpWindowNameA, dwStyle, X, Y, nWidth, nHeight, hWndParent,
								hMenu, hInstance, lpParam);
	free(lpClassNameA);
	if (lpWindowNameA != NULL) {
		free(lpWindowNameA);
	}
	return hWnd;
}

ATOM WINAPI _RegisterClassW(const WNDCLASSW *lpWndClass)
{
	char *menu_nameA = ToCharW(lpWndClass->lpszMenuName);
	char *class_nameA = ToCharW(lpWndClass->lpszClassName);

	WNDCLASSA WndClassA;
	WndClassA.style = lpWndClass->style;
	WndClassA.lpfnWndProc = lpWndClass->lpfnWndProc;
	WndClassA.cbClsExtra = lpWndClass->cbClsExtra;
	WndClassA.cbWndExtra = lpWndClass->cbWndExtra;
	WndClassA.hInstance = lpWndClass->hInstance;
	WndClassA.hIcon = lpWndClass->hIcon;
	WndClassA.hCursor = lpWndClass->hCursor;
	WndClassA.hbrBackground = lpWndClass->hbrBackground;
	WndClassA.lpszMenuName = menu_nameA;
	WndClassA.lpszClassName = class_nameA;
	ATOM atom = RegisterClassA(&WndClassA);

	if (menu_nameA != NULL) {
		free(menu_nameA);
	}
	if (class_nameA != NULL) {
		free(class_nameA);
	}
	return atom;
}

BOOL WINAPI _SetWindowTextW(HWND hWnd, LPCWSTR lpString)
{
	char *strA = ToCharW(lpString);
	BOOL retval = SetWindowTextA(hWnd, strA);
	free(strA);
	return retval;
}

UINT WINAPI _GetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax)
{
	if (cchMax <= 1) {
		return 0;
	}

	HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
	size_t lenW;
	wchar_t *strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
	wchar_t *dest_ptr = lpString;
	size_t dest_len = (size_t)cchMax;
	wcsncpy_s(dest_ptr, dest_len, strW, _TRUNCATE);
	free(strW);
	return (UINT)(dest_len - 1 < lenW ? dest_len - 1 : lenW);
}

/**
 *	@param[in]		hdc
 *	@param[in]		lpchText	������
 *	@param[in]		cchText		������(-1�̂Ƃ�lpchText�̕�����)
 *	@param[in]		lprc		�\��rect
 *	@param[in]		format
 */
int WINAPI _DrawTextW(HDC hdc, LPCWSTR lpchText, int cchText, LPRECT lprc, UINT format)
{
	int strW_len = (cchText == -1) ? 0 : cchText;
	size_t strA_len;
	char *strA = _WideCharToMultiByte(lpchText, strW_len, CP_ACP, &strA_len);
	int result = DrawTextA(hdc, strA, (int)strA_len, lprc, format);
	free(strA);
	return result;
}

int WINAPI _MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
	char *textA = ToCharW(lpText);
	char *captionA = ToCharW(lpCaption);
	int result = MessageBoxA(hWnd, textA, captionA, uType);
	free(textA);
	free(captionA);
	return result;
}

BOOL WINAPI _InsertMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	char *itemA = ToCharW(lpNewItem);
	int result = InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, itemA);
	free(itemA);
	return result;
}

BOOL WINAPI _AppendMenuW(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	char *itemA = ToCharW(lpNewItem);
	BOOL result = AppendMenuA(hMenu, uFlags, uIDNewItem, itemA);
	free(itemA);
	return result;
}

int WINAPI _AddFontResourceW(LPCWSTR lpFileName)
{
	char *filenameA = ToCharW(lpFileName);
	int result = AddFontResourceA(filenameA);
	free(filenameA);
	return result;
}

BOOL WINAPI _RemoveFontResourceW(LPCWSTR lpFileName)
{
	char *filenameA = ToCharW(lpFileName);
	int result = RemoveFontResourceA(filenameA);
	free(filenameA);
	return result;
}

BOOL WINAPI _Shell_NotifyIconW(DWORD dwMessage, TT_NOTIFYICONDATAW_V2 *lpData)
{
	const TT_NOTIFYICONDATAW_V2 *w = lpData;
	if (w->cbSize != sizeof(TT_NOTIFYICONDATAW_V2)) {
		return FALSE;
	}

	// lpData.cbSize == 952�̂Ƃ����� ANSI�֐��ŏ�������
	TT_NOTIFYICONDATAA_V2 nid;
	TT_NOTIFYICONDATAA_V2 *p = &nid;
	p->cbSize = sizeof(nid);
	p->hWnd = w->hWnd;
	p->uID = w->uID;
	p->uFlags = w->uFlags;
	p->uCallbackMessage = w->uCallbackMessage;
	p->hIcon = w->hIcon;
	p->dwState = w->dwState;
	p->dwStateMask = w->dwStateMask;
	p->uTimeout = w->uTimeout;
	p->dwInfoFlags = w->dwInfoFlags;

	char *strA = ToCharW(w->szTip);
	strcpy_s(p->szTip, strA);
	free(strA);
	strA = ToCharW(w->szInfoTitle);
	strcpy_s(p->szInfoTitle, strA);
	free(strA);
	strA = ToCharW(w->szInfo);
	strcpy_s(p->szInfo, strA);
	free(strA);

	BOOL r = Shell_NotifyIconA(dwMessage, (PNOTIFYICONDATAA)p);
	return r;
}

HWND WINAPI _CreateDialogIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc,
								 LPARAM dwInitParam)
{
	return CreateDialogIndirectParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

INT_PTR WINAPI _DialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent,
								 DLGPROC lpDialogFunc, LPARAM lParamInit)
{
	return DialogBoxIndirectParamA(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParamInit);
}

LONG WINAPI _SetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong)
{
	return SetWindowLongA(hWnd, nIndex, dwNewLong);
}

LONG_PTR WINAPI _SetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
#ifdef _WIN64
	if (pSetWindowLongPtrW != NULL) {
		return pSetWindowLongPtrW(hWnd, nIndex, dwNewLong);
	}
	return SetWindowLongPtrA(hWnd, nIndex, dwNewLong);
#else
	return _SetWindowLongW(hWnd, nIndex, dwNewLong);
#endif
}

LONG WINAPI _GetWindowLongW(HWND hWnd, int nIndex)
{
	return GetWindowLongA(hWnd, nIndex);
}

LONG_PTR WINAPI _GetWindowLongPtrW(HWND hWnd, int nIndex)
{
#ifdef _WIN64
	if (pGetWindowLongPtrW != NULL) {
		return pGetWindowLongPtrW(hWnd, nIndex);
	}
	return GetWindowLongPtrA(hWnd, nIndex);
#else
	return _GetWindowLongW(hWnd, nIndex);
#endif
}

LRESULT WINAPI _CallWindowProcW(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}

void WINAPI _OutputDebugStringW(LPCWSTR lpOutputString)
{
	if (pOutputDebugStringW != NULL) {
		return pOutputDebugStringW(lpOutputString);
	}

	char *strA = ToCharW(lpOutputString);
	OutputDebugStringA(strA);
	free(strA);
}

DWORD WINAPI _GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	char dir[MAX_PATH];
	GetCurrentDirectoryA(_countof(dir), dir);
	wchar_t *strW = ToWcharA(dir);
	DWORD r;
	if (lpBuffer != NULL) {
		wcsncpy_s(lpBuffer, nBufferLength, strW, _TRUNCATE);
		r =  (DWORD)wcslen(lpBuffer);
	}
	else {
		r =  (DWORD)wcslen(strW) + 1;
	}
	free(strW);
	return r;
}

BOOL WINAPI _SetCurrentDirectoryW(LPCWSTR lpPathName)
{
	char *strA = ToCharW(lpPathName);
	BOOL r = SetCurrentDirectoryA(strA);
	free(strA);
	return r;
}

LPITEMIDLIST WINAPI _SHBrowseForFolderW(LPBROWSEINFOW lpbi)
{
	char display_name[MAX_PATH];
	BROWSEINFOA biA = {};
	biA.hwndOwner = lpbi->hwndOwner;
	biA.pidlRoot = lpbi->pidlRoot;
	biA.pszDisplayName = display_name;
	biA.lpszTitle = ToCharW(lpbi->lpszTitle);
	biA.ulFlags = lpbi->ulFlags;
	biA.lpfn = lpbi->lpfn;
	biA.lParam = lpbi->lParam;
	LPITEMIDLIST pidlBrowse = SHBrowseForFolderA(&biA);
	ACPToWideChar_t(display_name, lpbi->pszDisplayName, MAX_PATH);
	free((void *)biA.lpszTitle);

	return pidlBrowse;
}

BOOL WINAPI _SHGetPathFromIDListW(LPITEMIDLIST pidl, LPWSTR pszPath)
{
	char pathA[MAX_PATH];
	BOOL r = SHGetPathFromIDListA(pidl, pathA);
	::MultiByteToWideChar(CP_ACP, 0, pathA, -1, pszPath, MAX_PATH);
	return r;
}

DWORD WINAPI _GetPrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault,
								LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
	assert(lpFileName != NULL);
	if (lpDefault == NULL) {
		lpDefault = L"";
	}
	char *buf = (char* )malloc(nSize);
	char *appA = ToCharW(lpAppName);
	char *keyA = ToCharW(lpKeyName);
	char *defA = ToCharW(lpDefault);
	char *fileA = ToCharW(lpFileName);
	DWORD r = GetPrivateProfileStringA(appA, keyA, defA, buf, nSize, fileA);
	if (r == 0 && defA == NULL) {
		// GetPrivateProfileStringA()�̖߂�l��buf�ɃZ�b�g����������(�I�[�܂܂�)
		// OS�̃o�[�W�����ɂ���Ă�def��NULL�̎��Abuf�����ݒ�ƂȂ邱�Ƃ�����
		buf[0] = 0;
	}
	::MultiByteToWideChar(CP_ACP, 0, buf, -1, lpReturnedString, nSize);
	r = (DWORD)wcslen(lpReturnedString);
	free(appA);
	free(keyA);
	free(defA);
	free(fileA);
	free(buf);
	return r;
}

BOOL WINAPI _WritePrivateProfileStringW(LPCWSTR lpAppName,LPCWSTR lpKeyName,LPCWSTR lpString,LPCWSTR lpFileName)
{
	char *appA = ToCharW(lpAppName);
	char *keyA = ToCharW(lpKeyName);
	char *strA = ToCharW(lpString);
	char *fileA = ToCharW(lpFileName);
	BOOL r = WritePrivateProfileStringA(appA, keyA, strA, fileA);
	free(appA);
	free(keyA);
	free(strA);
	free(fileA);
	return r;
}

UINT WINAPI _GetPrivateProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName)
{
	char *appA = ToCharW(lpAppName);
	char *keyA = ToCharW(lpKeyName);
	char *fileA = ToCharW(lpFileName);
	UINT r = GetPrivateProfileIntA(appA, keyA, nDefault, fileA);
	free(appA);
	free(keyA);
	free(fileA);
	return r;
}

BOOL WINAPI _CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
					 LPSECURITY_ATTRIBUTES lpProcessAttributes,
					 LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
					 DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
					 LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	STARTUPINFOA suiA = {};
	suiA.cb = sizeof(suiA);
	suiA.lpReserved = NULL;
	suiA.lpDesktop = ToCharW(lpStartupInfo->lpDesktop);
	suiA.lpTitle = ToCharW(lpStartupInfo->lpTitle);
	suiA.dwX = lpStartupInfo->dwX;
	suiA.dwY = lpStartupInfo->dwY;
	suiA.dwXSize = lpStartupInfo->dwXSize;
	suiA.dwYSize = lpStartupInfo->dwYSize;
	suiA.dwXCountChars = lpStartupInfo->dwXCountChars;
	suiA.dwYCountChars = lpStartupInfo->dwYCountChars;
	suiA.dwFillAttribute = lpStartupInfo->dwFillAttribute;
	suiA.dwFlags = lpStartupInfo->dwFlags;
	suiA.wShowWindow = lpStartupInfo->wShowWindow;
	suiA.cbReserved2 = lpStartupInfo->cbReserved2;
	suiA.lpReserved2 = lpStartupInfo->lpReserved2;
	suiA.hStdInput = lpStartupInfo->hStdInput;
	suiA.hStdOutput = lpStartupInfo->hStdOutput;
	suiA.hStdError = lpStartupInfo->hStdError;

	char *appA = ToCharW(lpApplicationName);
	char *cmdA = ToCharW(lpCommandLine);
	char *curA = ToCharW(lpCurrentDirectory);
	BOOL r =
		CreateProcessA(appA, cmdA, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
					   dwCreationFlags, lpEnvironment, curA, &suiA, lpProcessInformation);
	free(appA);
	free(cmdA);
	free(curA);
	free(suiA.lpDesktop);
	free(suiA.lpTitle);

	return r;
}

BOOL WINAPI _CopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists)
{
	char *lpExistingFileNameA = ToCharW(lpExistingFileName);
	char *lpNewFileNameA = ToCharW(lpNewFileName);
	BOOL r = CopyFileA(lpExistingFileNameA, lpNewFileNameA, bFailIfExists);
	free(lpExistingFileNameA);
	free(lpNewFileNameA);
	return r;
}

BOOL WINAPI _DeleteFileW(LPCWSTR lpFileName)
{
	char *lpFileNameA = ToCharW(lpFileName);
	BOOL r = DeleteFileA(lpFileNameA);
	free(lpFileNameA);
	return r;
}

BOOL WINAPI _MoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
	char *lpExistingFileNameA = ToCharW(lpExistingFileName);
	char *lpNewFileNameA = ToCharW(lpNewFileName);
	BOOL r = MoveFileA(lpExistingFileNameA, lpNewFileNameA);
	free(lpExistingFileNameA);
	free(lpNewFileNameA);
	return r;
}

HANDLE WINAPI _CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
					LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
					HANDLE hTemplateFile)
{
	char *lpFileNameA = ToCharW(lpFileName);
	HANDLE handle = CreateFileA(lpFileNameA, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition,
								dwFlagsAndAttributes, hTemplateFile);
	free(lpFileNameA);
	return handle;
}

static void FindDataAW(const WIN32_FIND_DATAA *a, WIN32_FIND_DATAW *w)
{
	w->dwFileAttributes = a->dwFileAttributes;
	w->ftCreationTime = a->ftCreationTime;
	w->ftLastAccessTime = a->ftLastAccessTime;
	w->ftLastWriteTime = a->ftLastWriteTime;
	w->nFileSizeHigh = a->nFileSizeHigh;
	w->nFileSizeLow = a->nFileSizeLow;
	w->dwReserved0 = a->dwReserved0;
	w->dwReserved1 = a->dwReserved1;
	::MultiByteToWideChar(CP_ACP, 0, a->cFileName, _countof(a->cFileName), w->cFileName, _countof(w->cFileName));
	::MultiByteToWideChar(CP_ACP, 0, a->cAlternateFileName, _countof(a->cAlternateFileName), w->cAlternateFileName, _countof(w->cAlternateFileName));
}

HANDLE WINAPI _FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	WIN32_FIND_DATAA find_file_data;
	char *lpFileNameA = ToCharW(lpFileName);
	HANDLE handle = FindFirstFileA(lpFileNameA, &find_file_data);
	free(lpFileNameA);
	FindDataAW(&find_file_data, lpFindFileData);
	return handle;
}

BOOL WINAPI _FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	WIN32_FIND_DATAA find_file_data;
	BOOL r = FindNextFileA(hFindFile, &find_file_data);
	FindDataAW(&find_file_data, lpFindFileData);
	return r;
}

BOOL WINAPI _RemoveDirectoryW(LPCWSTR lpPathName)
{
	char *lpPathNameA = ToCharW(lpPathName);
	BOOL r = RemoveDirectoryA(lpPathNameA);
	free(lpPathNameA);
	return r;
}

DWORD WINAPI _GetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR *lpFilePart)
{
	if (nBufferLength == 0 || lpBuffer == NULL) {
		char *filenameA = ToCharW(lpFileName);
		DWORD r = GetFullPathNameA(filenameA, 0, NULL, NULL);
		free(filenameA);
		return r;
	}
	else {
		char *filenameA = ToCharW(lpFileName);
		char bufA[MAX_PATH];
		char *filepartA;
		DWORD r = GetFullPathNameA(filenameA, sizeof(bufA), bufA, &filepartA);
		if (r == 0) {
			// error
			free(filenameA);
			return 0;
		}
		wchar_t *bufW = ToWcharA(bufA);
		r = (DWORD)wcslen(bufW);
		if (nBufferLength == 0 || lpBuffer == NULL) {
			// �K�v�ȕ�������Ԃ�('\0'�܂�)
			r = r + 1;
		} else {
			// �p�X���R�s�[���āA�����񒷂�Ԃ�('\0'�܂܂Ȃ�)
			wcsncpy_s(lpBuffer, nBufferLength, bufW, _TRUNCATE);
			if (lpFilePart != NULL) {
				// �p�X���������o��(ANSI->Unicode)
				const size_t countA = filepartA - bufA;
				char *pathA = (char *)malloc(countA+1);
				memcpy(pathA, bufA, countA);
				pathA[countA] = 0;
				wchar_t *pathW = ToWcharA(pathA);

				// �p�X�����̕�����(Unicode)
				size_t countW = wcslen(pathW);
				free(pathW);
				free(pathA);

				// �t�@�C���������ւ̃|�C���^
				*lpFilePart = lpBuffer + countW;
			}
		}
		free(filenameA);
		free(bufW);
		return r;
	}
}

HMODULE WINAPI _LoadLibraryW(LPCWSTR lpLibFileName)
{
	char *LibFileNameA = ToCharW(lpLibFileName);
	HMODULE r = LoadLibraryA(LibFileNameA);
	free(LibFileNameA);
	return r;
}

DWORD WINAPI _GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize)
{
	char filenameA[MAX_PATH];
	DWORD r = GetModuleFileNameA(hModule, filenameA, sizeof(filenameA));
	if (r == 0) {
		return 0;
	}
	DWORD wlen = ACPToWideChar_t(filenameA, lpFilename, nSize);
	return wlen - 1;	// not including the terminating null character
}

DWORD WINAPI _ExpandEnvironmentStringsW(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize)
{
	char *srcA = ToCharW(lpSrc);
	char dstA[MAX_PATH];	// MAX_PATH?
	DWORD r = ExpandEnvironmentStringsA(srcA, dstA, sizeof(dstA));
	wchar_t *dstW = ToWcharA(dstA);
	r = (DWORD)wcslen(dstW);
	if (lpDst == NULL || nSize == 0) {
		r++;
	}
	else {
		wcsncpy_s(lpDst, nSize, dstW, _TRUNCATE);
	}
	free(srcA);
	free(dstW);
	return r;
}

HMODULE WINAPI _GetModuleHandleW(LPCWSTR lpModuleName)
{
	char *lpStringA = ToCharW(lpModuleName);
	HMODULE h = GetModuleHandleA(lpStringA);
	free(lpStringA);
	return h;
}

UINT WINAPI _GetSystemDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
	char buf[MAX_PATH];
	UINT r = GetSystemDirectoryA(buf, _countof(buf));
	if (r == 0) {
		return 0;
	}
	size_t wlen = ACPToWideChar_t(buf, lpBuffer, uSize);
	return wlen - 1;	// not including the terminating null character
}

DWORD WINAPI _GetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	char buf[MAX_PATH];
	DWORD r = GetTempPathA(_countof(buf), buf);
	if (r == 0) {
		return 0;
	}
	size_t wlen = ACPToWideChar_t(buf, lpBuffer, nBufferLength);
	return wlen - 1;	// not including the terminating null character
}

UINT WINAPI _GetTempFileNameW(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName)
{
	char buf[MAX_PATH];
	char *lpPathNameA = ToCharW(lpPathName);
	char *lpPrefixStringA = ToCharW(lpPrefixString);
	UINT r = GetTempFileNameA(lpPathNameA, lpPrefixStringA, uUnique, buf);
	ACPToWideChar_t(buf, lpTempFileName, MAX_PATH);
	free(lpPathNameA);
	free(lpPrefixStringA);
	return r;
}

LRESULT WINAPI _DefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProcA(hWnd, Msg, wParam, lParam);
}

BOOL WINAPI _ModifyMenuW(HMENU hMnu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	char *lpNewItemA = ToCharW(lpNewItem);
	BOOL r = ModifyMenuA(hMnu, uPosition, uFlags, uIDNewItem, lpNewItemA);
	free(lpNewItemA);
	return r;
}

/**
 *	@param cchMax	�o�b�t�@�̕�����(�I�[ L'\0' ���܂�(sizeof(buf)/sizeof(wchar_t)))
 *	@retrun			������(�I�[ L'\0' ���܂܂Ȃ�,wcslen()�̖߂�l�Ɠ���)
 */
int WINAPI _GetMenuStringW(HMENU hMenu, UINT uIDItem, LPWSTR lpString, int cchMax, UINT flags)
{
	int len = GetMenuStringA(hMenu, uIDItem, NULL, 0, flags);
	if (len == 0) {
		return 0;
	}
	len++;	// for '\0'
	char *strA = (char* )malloc(len);
	int r = GetMenuStringA(hMenu, uIDItem, strA, len, flags);
	if (r == 0) {
		free(strA);
		return 0;
	}
	wchar_t *strW = ToWcharA(strA);
	len = (int)wcslen(strW);
	if (lpString != NULL && cchMax != 0) {
		// �������Z�b�g����
		// �Z�b�g���Ȃ��Ƃ��͕����񒷂�Ԃ�
		wcsncpy_s(lpString, cchMax, strW, _TRUNCATE);
		if (len > cchMax - 1) {
			len = cchMax - 1;
		}
	}
	free(strW);
	free(strA);
	return len;	// ������(L'\0'�����܂܂Ȃ�)
}

HANDLE WINAPI _LoadImageW(HINSTANCE hInst, LPCWSTR name, UINT type,
						  int cx, int cy, UINT fuLoad)
{
	HANDLE handle;
	if (HIWORD(name) == 0) {
		handle = LoadImageA(hInst, (LPCSTR)name, type, cx, cy, fuLoad);
	} else {
		char *nameA = ToCharW(name);
		handle = LoadImageA(hInst, nameA, type, cx, cy, fuLoad);
		free(nameA);
	}
	return handle;
}

static void LOGFONTAtoW(const LOGFONTA *logfontA, LOGFONTW *logfontW)
{
	LOGFONTA *d = (LOGFONTA *)logfontW;
	*d = *logfontA;
	ACPToWideChar_t(logfontA->lfFaceName, logfontW->lfFaceName, _countof(logfontW->lfFaceName));
}

BOOL WINAPI _SystemParametersInfoW(UINT uiAction, UINT uiParam,
								   PVOID pvParam, UINT fWinIni)
{
	if (uiAction == SPI_GETNONCLIENTMETRICS) {
		NONCLIENTMETRICSA ncmA = {};
		// NONCLIENTMETRICSA �� VISTA �ȍ~�g������Ă���
		ncmA.cbSize = CCSIZEOF_STRUCT(NONCLIENTMETRICSA, lfMessageFont);
		BOOL r = SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, uiParam, &ncmA, 0);
		if (r != FALSE) {
			NONCLIENTMETRICSW *ncmW = (NONCLIENTMETRICSW *)pvParam;
			// Tera Term �Ŏg�p���郁���o�����ݒ�
			LOGFONTAtoW(&ncmA.lfStatusFont, &ncmW->lfStatusFont);
		}
		return r;
	}
	return SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
}

/**
 *	GetTabbedTextExtentW()
 *
 *	Tera Term �ł� TabStop �͎g�p���Ă��Ȃ��̂Ńe�X�g���Ă��Ȃ�
 */
DWORD WINAPI _GetTabbedTextExtentW(HDC hdc, LPCWSTR lpString, int chCount,
								   int nTabPositions, const int *lpnTabStopPositions)
{
	char *strA = ToCharW(lpString);
	int lenA = (int)strlen(strA);
	DWORD r = GetTabbedTextExtentA(hdc, strA, lenA, nTabPositions, lpnTabStopPositions);
	free(strA);
	return r;
}
