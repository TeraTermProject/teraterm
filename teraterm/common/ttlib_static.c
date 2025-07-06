/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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
#include <assert.h>

#include "compat_win.h"

#include "ttlib.h"

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

int SetDlgTextsW(HWND hDlgWnd, const DlgTextInfo *infos, int infoCount, const wchar_t *UILanguageFile)
{
	return SetI18nDlgStrsW(hDlgWnd, "Tera Term", infos, infoCount, UILanguageFile);
}

int SetDlgTexts(HWND hDlgWnd, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile)
{
	return SetI18nDlgStrsA(hDlgWnd, "Tera Term", infos, infoCount, UILanguageFile);
}

void SetDlgMenuTextsW(HMENU hMenu, const DlgTextInfo *infos, int infoCount, const wchar_t *UILanguageFile)
{
	SetI18nMenuStrsW(hMenu, "Tera Term", infos, infoCount, UILanguageFile);
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
 *	point�����݂���f�B�X�v���C�̃f�X�N�g�b�v�͈̔͂��擾����
 */
void GetDesktopRectFromPoint(const POINT *p, RECT *rect)
{
	if (pMonitorFromPoint == NULL) {
		// NT4.0, 95 �̓}���`���j�^API�ɔ�Ή�
		SystemParametersInfo(SPI_GETWORKAREA, 0, rect, 0);
	}
	else {
		// �}���`���j�^���T�|�[�g����Ă���ꍇ
		HMONITOR hm;
		POINT pt;
		MONITORINFO mi;

		pt.x = p->x;
		pt.y = p->y;
		hm = pMonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(MONITORINFO);
		pGetMonitorInfoA(hm, &mi);
		*rect = mi.rcWork;
	}
}

/**
 *	�E�B���h�E��̈悩��͂ݏo���Ȃ��悤�Ɉړ�����
 *	�͂ݏo�Ă��Ȃ��ꍇ�͈ړ����Ȃ�
 *
 *	@param[in]	hWnd		�ʒu�𒲐�����E�B���h�E
 */
void MoveWindowToDisplayRect(HWND hWnd, const RECT *rect)
{
	RECT desktop = *rect;
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
 *	�E�B���h�E���f�B�X�v���C����͂ݏo���Ȃ��悤�Ɉړ�����
 *	�f�B�X�v���C�̓E�B���h�E�������Ă���f�B�X�v���C
 *	�܂����ł���Ƃ��͖ʐς̍L����
 *	�͂ݏo�Ă��Ȃ��ꍇ�͈ړ����Ȃ�
 *
 *	@param[in]	hWnd		�ʒu�𒲐�����E�B���h�E
 */
void MoveWindowToDisplay(HWND hWnd)
{
	RECT desktop;
	GetDesktopRect(hWnd, &desktop);
	MoveWindowToDisplayRect(hWnd, &desktop);
}

/**
 *	�E�B���h�E���f�B�X�v���C����͂ݏo���Ȃ��悤�Ɉړ�����
 *	�f�B�X�v���C��point�������Ă���f�B�X�v���C
 *	�͂ݏo�Ă��Ȃ��ꍇ�͈ړ����Ȃ�
 *
 *	@param[in]	hWnd		�ʒu�𒲐�����E�B���h�E
 *	@param[in]	point		�ʒu
 */
void MoveWindowToDisplayPoint(HWND hWnd, const POINT *point)
{
	RECT desktop;
	GetDesktopRectFromPoint(point, &desktop);
	MoveWindowToDisplayRect(hWnd, &desktop);
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
 *	@param	hWnd	(NULL�̂Ƃ�Primary monitor)
 *	@retval	DPI�l(�ʏ��DPI��96)
 */
int GetMonitorDpiFromWindow(HWND hWnd)
{
	if (pGetDpiForWindow == NULL || hWnd == NULL) {
		int dpiY;
		HDC hDC = GetDC(hWnd);
		dpiY = GetDeviceCaps(hDC,LOGPIXELSY);
		ReleaseDC(hWnd, hDC);
		return dpiY;
	}
	else {
		int dpi = 96;
		RECT r;
		GetWindowRect(hWnd, &r);
		HINSTANCE hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
		HWND tmphWnd = CreateWindowExW(0, WC_STATICW, (LPCWSTR)NULL, 0,
							r.left, r.top, r.right - r.left, r.bottom - r.top,
							NULL, (HMENU)0x00, hInst, (LPVOID)NULL);
		if (tmphWnd) {
			dpi = pGetDpiForWindow(tmphWnd);
			DestroyWindow(tmphWnd);
		}
		return dpi;
	}
}

void OutputDebugPrintfW(const wchar_t *fmt, ...)
{
	wchar_t tmp[1024];
	va_list arg;
	va_start(arg, fmt);
	_vsnwprintf_s(tmp, _countof(tmp), _TRUNCATE, fmt, arg);
	va_end(arg);
	OutputDebugStringW(tmp);
}

/* vim: set ts=4 sw=4 ff=dos : */
