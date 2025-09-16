/*
 * Copyright (C) 2024- TeraTerm Project
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
 *	win32helper の UTF-8 版
 *	win32helper に統合しなかった理由
 *		- ttxssh でしか使用していない
 *		- codeconv に依存したくない
 */
#include <windows.h>
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>

#include "codeconv.h"
#include "win32helper.h"

#include "win32helper_u8.h"

BOOL SetDlgItemTextU8(HWND hDlg, int nIDDlgItem, const char *strU8)
{
	wchar_t *strW = ToWcharU8(strU8);
	BOOL retval = SetDlgItemTextW(hDlg, nIDDlgItem, strW);
	free(strW);
	return retval;
}

/**
 *	hGetWindowTextW の UTF-8 版
 */
DWORD hGetWindowTextU8(HWND ctl, char **textU8)
{
	DWORD e;
	wchar_t *textW;
	e = hGetWindowTextW(ctl, &textW);
	if (e != NO_ERROR) {
		*textU8 = NULL;
		return e;
	}
	*textU8 = ToU8W(textW);
	free(textW);
	return NO_ERROR;
}

DWORD hGetDlgItemTextU8(HWND hDlg, int id, char **textU8)
{
	HWND hWnd = GetDlgItem(hDlg, id);
	return hGetWindowTextU8(hWnd, textU8);
}

/**
 *	hGetWindowTextW の ANSI 版
 */
DWORD hGetWindowTextA(HWND ctl, char **textA)
{
	DWORD e;
	wchar_t *textW;
	e = hGetWindowTextW(ctl, &textW);
	if (e != NO_ERROR) {
		*textA = NULL;
		return e;
	}
	*textA = ToCharW(textW);
	free(textW);
	return NO_ERROR;
}

DWORD hGetDlgItemTextA(HWND hDlg, int id, char **textA)
{
	HWND hWnd = GetDlgItem(hDlg, id);
	return hGetWindowTextA(hWnd, textA);
}
