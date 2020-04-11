/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2020 TeraTerm Project
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

/* misc. routines  */

#include <sys/stat.h>
#include <sys/utime.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <shlobj.h>
#include <ctype.h>
#include <mbctype.h>	// for _ismbblead
#include <assert.h>

#include "teraterm_conf.h"
#include "teraterm.h"
#include "tttypes.h"
#include "compat_win.h"
#include "layer_for_unicode.h"

#include "../teraterm/unicode_test.h"


/**
 *	�E�B���h�E��̈ʒu���擾����
 *	@Param[in]		hWnd
 *	@Param[in]		point		�ʒu(x,y)
 *	@Param[in,out]	InWindow	�E�B���h�E��
 *	@Param[in,out]	InClient	�N���C�A���g�̈��
 *	@Param[in,out]	InTitleBar	�^�C�g���o�[��
 *	@retval			FALSE		������hWnd
 */
BOOL GetPositionOnWindow(
	HWND hWnd, const POINT *point,
	BOOL *InWindow, BOOL *InClient, BOOL *InTitleBar)
{
	const int x = point->x;
	const int y = point->y;
	RECT winRect;
	RECT clientRect;

	if (InWindow != NULL) *InWindow = FALSE;
	if (InClient != NULL) *InClient = FALSE;
	if (InTitleBar != NULL) *InTitleBar = FALSE;

	if (!GetWindowRect(hWnd, &winRect)) {
		return FALSE;
	}

	if ((x < winRect.left) || (winRect.right < x) ||
		(y < winRect.top) || (winRect.bottom < y))
	{
		return TRUE;
	}
	if (InWindow != NULL) *InWindow = TRUE;

	{
		POINT pos;
		GetClientRect(hWnd, &clientRect);
		pos.x = clientRect.left;
		pos.y = clientRect.top;
		ClientToScreen(hWnd, &pos);
		clientRect.left = pos.x;
		clientRect.top = pos.y;

		pos.x = clientRect.right;
		pos.y = clientRect.bottom;
		ClientToScreen(hWnd, &pos);
		clientRect.right = pos.x;
		clientRect.bottom = pos.y;
	}

	if ((clientRect.left <= x) && (x < clientRect.right) &&
		(clientRect.top <= y) && (y < clientRect.bottom))
	{
		if (InClient != NULL) *InClient = TRUE;
		if (InTitleBar != NULL) *InTitleBar = FALSE;
		return TRUE;
	}
	if (InClient != NULL) *InClient = FALSE;

	if (InTitleBar != NULL) {
		*InTitleBar = (y < clientRect.top) ? TRUE : FALSE;
	}

	return TRUE;
}

int SetDlgTexts(HWND hDlgWnd, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile)
{
	return SetI18nDlgStrs("Tera Term", hDlgWnd, infos, infoCount, UILanguageFile);
}

void SetDlgMenuTexts(HMENU hMenu, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile)
{
	SetI18nMenuStrs("Tera Term", hMenu, infos, infoCount, UILanguageFile);
}

/**
 *	�E�B���h�E�\������Ă���f�B�X�v���C�̃f�X�N�g�b�v�͈̔͂��擾����
 *	@param[in]		hWnd	�E�B���h�E�̃n���h��
 *	@param[out]		rect	�f�X�N�g�b�v
 */
void GetDesktopRect(HWND hWnd, RECT *rect)
{
	if (pMonitorFromWindow != NULL) {
		// �}���`���j�^���T�|�[�g����Ă���ꍇ
		MONITORINFO monitorInfo;
		HMONITOR hMonitor = pMonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		monitorInfo.cbSize = sizeof(MONITORINFO);
		pGetMonitorInfoA(hMonitor, &monitorInfo);
		*rect = monitorInfo.rcWork;
	} else {
		// �}���`���j�^���T�|�[�g����Ă��Ȃ��ꍇ
		SystemParametersInfo(SPI_GETWORKAREA, 0, rect, 0);
	}
}

/**
 *	�E�B���h�E���f�B�X�v���C����͂ݏo���Ȃ��悤�Ɉړ�����
 *	�͂ݏo�Ă��Ȃ��ꍇ�͈ړ����Ȃ�
 *
 *	@param[in]	hWnd		�ʒu�𒲐�����E�B���h�E
 */
void MoveWindowToDisplay(HWND hWnd)
{
	RECT desktop;
	RECT win_rect;
	int win_width;
	int win_height;
	int win_x;
	int win_y;
	BOOL modify = FALSE;

	GetDesktopRect(hWnd, &desktop);

	GetWindowRect(hWnd, &win_rect);
	win_x = win_rect.left;
	win_y = win_rect.top;
	win_height = win_rect.bottom - win_rect.top;
	win_width = win_rect.right - win_rect.left;
	if (win_y < desktop.top) {
		win_y = desktop.top;
		modify = TRUE;
	}
	else if (win_y + win_height > desktop.bottom) {
		win_y = desktop.bottom - win_height;
		modify = TRUE;
	}
	if (win_x < desktop.left) {
		win_x = desktop.left;
		modify = TRUE;
	}
	else if (win_x + win_width > desktop.right) {
		win_x = desktop.right - win_width;
		modify = TRUE;
	}

	if (modify) {
		SetWindowPos(hWnd, NULL, win_x, win_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
}

/**
 *	�E�B���h�E���f�B�X�v���C�̒����ɔz�u����
 *
 *	@param[in]	hWnd		�ʒu�𒲐�����E�B���h�E
 *	@param[in]	hWndParent	���̃E�B���h�E�̒����Ɉړ�����
 *							(NULL�̏ꍇ�f�B�X�v���C�̒���)
 *
 *	hWndParent�̎w�肪����ꍇ
 *		hWndParent���\����Ԃ̏ꍇ
 *			- hWndParent�̒����ɔz�u
 *			- �������\������Ă���f�B�X�v���C����͂ݏo���ꍇ�͒��������
 *		hWndParent����\����Ԃ̏ꍇ
 *			- hWnd���\������Ă���f�B�X�v���C�̒����ɔz�u�����
 *	hWndParent��NULL�̏ꍇ
 *		- hWnd���\������Ă���f�B�X�v���C�̒����ɔz�u�����
 */
void CenterWindow(HWND hWnd, HWND hWndParent)
{
	RECT rcWnd;
	LONG WndWidth;
	LONG WndHeight;
	int NewX;
	int NewY;
	RECT rcDesktop;
	BOOL r;

	r = GetWindowRect(hWnd, &rcWnd);
	assert(r != FALSE); (void)r;
	WndWidth = rcWnd.right - rcWnd.left;
	WndHeight = rcWnd.bottom - rcWnd.top;

	if (hWndParent == NULL || !IsWindowVisible(hWndParent) || IsIconic(hWndParent)) {
		// �e���ݒ肳��Ă��Ȃ� or �\������Ă��Ȃ� or icon������Ă��� �ꍇ
		// �E�B���h�E�̕\������Ă���f�B�X�v���C�̒����ɕ\������
		GetDesktopRect(hWnd, &rcDesktop);

		// �f�X�N�g�b�v(�\������Ă���f�B�X�v���C)�̒���
		NewX = (rcDesktop.left + rcDesktop.right) / 2 - WndWidth / 2;
		NewY = (rcDesktop.top + rcDesktop.bottom) / 2 - WndHeight / 2;
	} else {
		RECT rcParent;
		r = GetWindowRect(hWndParent, &rcParent);
		assert(r != FALSE); (void)r;

		// hWndParent�̒���
		NewX = (rcParent.left + rcParent.right) / 2 - WndWidth / 2;
		NewY = (rcParent.top + rcParent.bottom) / 2 - WndHeight / 2;

		GetDesktopRect(hWndParent, &rcDesktop);
	}

	// �f�X�N�g�b�v����͂ݏo���ꍇ�A��������
	if (NewX + WndWidth > rcDesktop.right)
		NewX = rcDesktop.right - WndWidth;
	if (NewX < rcDesktop.left)
		NewX = rcDesktop.left;

	if (NewY + WndHeight > rcDesktop.bottom)
		NewY = rcDesktop.bottom - WndHeight;
	if (NewY < rcDesktop.top)
		NewY = rcDesktop.top;

	// �ړ�����
	SetWindowPos(hWnd, NULL, NewX, NewY, 0, 0,
				 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/**
 *	hWnd�̕\������Ă��郂�j�^��DPI���擾����
 *	Per-monitor DPI awareness�Ή�
 *
 *	@retval	DPI�l(�ʏ��DPI��96)
 */
int GetMonitorDpiFromWindow(HWND hWnd)
{
	if (pGetDpiForMonitor == NULL) {
		// �_�C�A���O���ł͎����X�P�[�����O�������Ă���̂�
		// ���96��Ԃ��悤��
		int dpiY;
		HDC hDC = GetDC(hWnd);
		dpiY = GetDeviceCaps(hDC,LOGPIXELSY);
		ReleaseDC(hWnd, hDC);
		return dpiY;
	} else {
		UINT dpiX;
		UINT dpiY;
		HMONITOR hMonitor = pMonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		pGetDpiForMonitor(hMonitor, 0 /*0=MDT_EFFECTIVE_DPI*/, &dpiX, &dpiY);
		return (int)dpiY;
	}
}

void OutputDebugPrintfW(const wchar_t *fmt, ...)
{
	wchar_t tmp[1024];
	va_list arg;
	va_start(arg, fmt);
	_vsnwprintf_s(tmp, _countof(tmp), _TRUNCATE, fmt, arg);
	va_end(arg);
	_OutputDebugStringW(tmp);
}

static int CALLBACK setDefaultFolder(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if(uMsg == BFFM_INITIALIZED) {
		SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
	}
	return 0;
}

BOOL doSelectFolderW(HWND hWnd, wchar_t *path, int pathlen, const wchar_t *def, const wchar_t *msg)
{
	BROWSEINFOW     bi;
	LPITEMIDLIST    pidlRoot;      // �u���E�Y�̃��[�gPIDL
	LPITEMIDLIST    pidlBrowse;    // ���[�U�[���I������PIDL
	wchar_t buf[MAX_PATH];
	BOOL ret = FALSE;

	// �_�C�A���O�\�����̃��[�g�t�H���_��PIDL���擾
	// ���ȉ��̓f�X�N�g�b�v�����[�g�Ƃ��Ă���B�f�X�N�g�b�v�����[�g�Ƃ���
	//   �ꍇ�́A�P�� bi.pidlRoot �ɂO��ݒ肷�邾���ł��悢�B���̑��̓�
	//   ��t�H���_�����[�g�Ƃ��鎖���ł���B�ڍׂ�SHGetSpecialFolderLoca
	//   tion�̃w���v���Q�Ƃ̎��B
	if (!SUCCEEDED(SHGetSpecialFolderLocation(hWnd, CSIDL_DESKTOP, &pidlRoot))) {
		return FALSE;
	}

	// BROWSEINFO�\���̂̏����l�ݒ�
	// ��BROWSEINFO�\���̂̊e�����o�̏ڍא������w���v���Q��
	bi.hwndOwner = hWnd;
	bi.pidlRoot = pidlRoot;
	bi.pszDisplayName = buf;
	bi.lpszTitle = msg;
	bi.ulFlags = 0;
	bi.lpfn = setDefaultFolder;
	bi.lParam = (LPARAM)def;
	// �t�H���_�I���_�C�A���O�̕\��
	pidlBrowse = _SHBrowseForFolderW(&bi);
	if (pidlBrowse != NULL) {
		// PIDL�`���̖߂�l�̃t�@�C���V�X�e���̃p�X�ɕϊ�
		if (_SHGetPathFromIDListW(pidlBrowse, buf)) {
			// �擾����
			wcsncpy_s(path, pathlen, buf, _TRUNCATE);
			ret = TRUE;
		}
		// SHBrowseForFolder�̖߂�lPIDL�����
		CoTaskMemFree(pidlBrowse);
	}
	// �N���[���A�b�v����
	CoTaskMemFree(pidlRoot);

	return ret;
}

/* vim: set ts=4 sw=4 ff=dos : */