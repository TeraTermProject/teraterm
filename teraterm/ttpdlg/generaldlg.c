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

/* general dialog */
#include <windows.h>
#include "vtwin.h"
#include "ttdlg.h"

BOOL WINAPI _SetupGeneral(HWND WndParent, PTTSet ts)
{
	LangInfo *infos = NULL;
	size_t infos_size = 0;
	wchar_t *folder;
	aswprintf(&folder, L"%s\\%s", ts->ExeDirW, get_lang_folder());
	infos = LangAppendFileList(folder, infos, &infos_size);
	free(folder);
	if (wcscmp(ts->ExeDirW, ts->HomeDirW) != 0) {
		aswprintf(&folder, L"%s\\%s", ts->HomeDirW, get_lang_folder());
		infos = LangAppendFileList(folder, infos, &infos_size);
		free(folder);
	}
	LangRead(infos, infos_size);

	DlgData data;
	data.ts = ts;
	data.lng_infos = infos;
	data.lng_size = infos_size;
	INT_PTR r = TTDialogBoxParam(hInst,
								 MAKEINTRESOURCEW(IDD_GENDLG),
								 WndParent, GenDlg, (LPARAM)&data);
	LangFree(infos, infos_size);
	return (BOOL)r;
}
