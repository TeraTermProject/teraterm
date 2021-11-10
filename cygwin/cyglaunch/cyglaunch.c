/*
 * Copyright (C) 2007- TeraTerm Project
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

//
// Cygterm launcher
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "ttlib.h"
#include "asprintf.h"
#include "cyglib.h"

#define Section L"Tera Term"

/**
 * TERATERM.INI から CygwinDirectory を読み込む
 */
static wchar_t *GetCygwinDir(void)
{
	wchar_t *HomeDir;
	wchar_t *teraterm_ini;
	wchar_t CygwinDir[256];

	HomeDir = GetHomeDirW(NULL);
	teraterm_ini = NULL;
	awcscats(&teraterm_ini, HomeDir, L"\\TERATERM.INI", NULL);
	free(HomeDir);

	// Cygwin install path
 	GetPrivateProfileStringW(Section, L"CygwinDirectory", L"",
							 CygwinDir, _countof(CygwinDir), teraterm_ini);
	free(teraterm_ini);

	if (CygwinDir[0] == 0) {
		return NULL;
	}
	return _wcsdup(CygwinDir);
}

int wmain(int argc, wchar_t *argv[])
{
	wchar_t *CygwinDir;
	wchar_t *Cmdline;
	int i;
	BOOL d_opt=FALSE;
	DWORD e;

	setlocale(LC_ALL, "");

	// 引数を結合してコマンドラインを作成
	Cmdline = NULL;
	for (i=1; i<argc; i++) {
		if (i != 1) {
			awcscat(&Cmdline, L" ");
		}
		if (d_opt && wcsncmp(L"\"\\\\", argv[i], 3) == 0) {
			argv[i][1] = '/';
			argv[i][2] = '/';
		}
		awcscat(&Cmdline, argv[i]);
		if (wcscmp(argv[i], L"-d") == 0) {
			d_opt = TRUE;
		}
		else {
			d_opt = FALSE;
		}
	}

	// cygwinがインストールされているフォルダ
	CygwinDir = GetCygwinDir();

	// cygtermを実行する
	e = CygwinConnect(CygwinDir, Cmdline);
	switch(e) {
	case NO_ERROR:
		break;
	case ERROR_FILE_NOT_FOUND:
		MessageBox(NULL, "Can't find Cygwin directory.", "ERROR", MB_OK | MB_ICONWARNING);
		break;
	case ERROR_NOT_ENOUGH_MEMORY:
		MessageBox(NULL, "Can't allocate memory.", "ERROR", MB_OK | MB_ICONWARNING);
		break;
	case ERROR_OPEN_FAILED:
	default:
		MessageBox(NULL, "Can't execute Cygterm.", "ERROR", MB_OK | MB_ICONWARNING);
		break;
	}

	return 0;
}
