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

#include "ttlib_static_dir.h"
#include "asprintf.h"
#include "win32helper.h"
#include "cyglib.h"

#define Section L"Tera Term"

/**
 * TERATERM.INI から CygwinDirectory を読み込む
 */
static wchar_t *GetCygwinDir(void)
{
	wchar_t *HomeDir;
	wchar_t *teraterm_ini;
	wchar_t *CygwinDir;

	HomeDir = GetHomeDirW(NULL);
	teraterm_ini = NULL;
	awcscats(&teraterm_ini, HomeDir, L"\\TERATERM.INI", NULL);
	free(HomeDir);

	// Cygwin install path
 	hGetPrivateProfileStringW(Section, L"CygwinDirectory", L"",
							  teraterm_ini, &CygwinDir);
	free(teraterm_ini);

	return CygwinDir;
}

typedef enum {
	SYSTEM_CYGWIN,
	SYSTEM_MSYS2,
	SYSTEM_GIT_BASH
} system_t;

int wmain(int argc, wchar_t *argv[])
{
	wchar_t *Cmdline;
	int i;
	DWORD e;
	system_t system = SYSTEM_CYGWIN;

	setlocale(LC_ALL, "");

	// 引数を結合してコマンドラインを作成
	Cmdline = NULL;
	for (i=1; i<argc; i++) {
		if (i != 1) {
			awcscat(&Cmdline, L" ");
		}
		if (wcscmp(argv[i], L"-d") == 0 && *(argv+1) != NULL) {
			i++;
			if (wcsncmp(L"\"\\\\", argv[i], 3) == 0) {
				// -d "\\path\..." を書き換え
				argv[i][1] = '/';
				argv[i][2] = '/';
			}
			awcscat(&Cmdline, L"-d ");
			awcscat(&Cmdline, argv[i]);
		}
		else if (wcscmp(argv[i], L"-cygwin") == 0) {
			system = SYSTEM_CYGWIN;
		}
		else if (wcscmp(argv[i], L"-msys2") == 0) {
			system = SYSTEM_MSYS2;
		}
		else if (wcscmp(argv[i], L"-gitbash") == 0) {
			system = SYSTEM_GIT_BASH;
		}
		else {
			awcscat(&Cmdline, argv[i]);
		}
	}


	// cygtermを実行する
	switch(system) {
	default:
	case SYSTEM_CYGWIN: {
		// cygwinがインストールされているフォルダ
		wchar_t *CygwinDir = GetCygwinDir();
		e = CygwinConnect(CygwinDir, Cmdline);
		free(CygwinDir);
		break;
	}
	case SYSTEM_MSYS2: {
		e = Msys2Connect(L"c:\\msys64", Cmdline);
		break;
	}
	case SYSTEM_GIT_BASH: {
		e = GitBashConnect(L"c:\\Program Files\\Git\\usr\\bin", Cmdline);
		break;
	}
	}
	switch(e) {
	case NO_ERROR:
		break;
	case ERROR_FILE_NOT_FOUND:
		MessageBoxA(NULL, "Can't find Cygwin directory.", "ERROR", MB_OK | MB_ICONWARNING);
		break;
	case ERROR_NOT_ENOUGH_MEMORY:
		MessageBoxA(NULL, "Can't allocate memory.", "ERROR", MB_OK | MB_ICONWARNING);
		break;
	case ERROR_OPEN_FAILED:
	default: {
		const char *msg =
			system == SYSTEM_CYGWIN ? "Can't execute Cygterm.":
			system == SYSTEM_MSYS2 ? "Can't execute msys2term.":
			"Can't execute gitbash.";
		MessageBoxA(NULL, msg, "ERROR", MB_OK | MB_ICONWARNING);
		break;
	}
	}

	free(Cmdline);
	return 0;
}
