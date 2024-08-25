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
#include <stdio.h>
#include <string.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "dlg_res.h"
#include "codeconv.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "ttlib_charset.h"
#include "ttwinman.h"

#include "ttdlg.h"

typedef struct {
	wchar_t *fullname;
	wchar_t *filename;
	wchar_t *language;
	wchar_t *date;
	wchar_t *contributor;
} LangInfo;

static const wchar_t *get_lang_folder()
{
	return (IsWindowsNTKernel()) ? L"lang_utf16le" : L"lang";
}

static void LangFree(LangInfo *infos, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++) {
		LangInfo *p = infos + i;
		free(p->filename);
		free(p->fullname);
		free(p->language);
		free(p->date);
		free(p->contributor);
	}
	free(infos);
}

/**
 *	ファイル名をリストする(ファイル名のみ)
 *	infosに追加してreturnする
 */
static LangInfo *LangAppendFileList(const wchar_t *folder, LangInfo *infos, size_t *infos_size)
{
	wchar_t *fullpath;
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	size_t count = *infos_size;

	aswprintf(&fullpath, L"%s\\*.lng", folder);
	hFind = FindFirstFileW(fullpath, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				LangInfo *p = (LangInfo *)realloc(infos, sizeof(LangInfo) * (count +1));
				if (p != NULL) {
					infos = p;
					p = infos + count;
					count++;
					memset(p, 0, sizeof(*p));
					p->filename = _wcsdup(fd.cFileName);
					aswprintf(&p->fullname, L"%s\\%s", folder, fd.cFileName);
				}
			}
		} while(FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
	free(fullpath);

	*infos_size = count;
	return infos;
}

/**
 *	lngファイルの Info セクションを読み込む
 */
static void LangRead(LangInfo *infos, size_t infos_size)
{
	size_t i;

	for (i = 0; i < infos_size; i++) {
		LangInfo *p = infos + i;
		const wchar_t *lng = p->fullname;
		wchar_t *s;
		hGetPrivateProfileStringW(L"Info", L"language", NULL, lng, &s);
		if (s[0] == 0) {
			free(s);
			p->language = _wcsdup(p->filename);
		} else {
			p->language = s;
		}
		hGetPrivateProfileStringW(L"Info", L"date", NULL, lng, &s);
		if (s[0] == 0) {
			free(s);
			p->date = _wcsdup(L"-");
		} else {
			p->date = s;
		}
		hGetPrivateProfileStringW(L"Info", L"contributor", NULL, lng, &s);
		if (s[0] == 0) {
			free(s);
			p->contributor = _wcsdup(L"-");
		} else {
			p->contributor = s;
		}
	}
}

typedef struct {
	TTTSet *ts;
	LangInfo *lng_infos;
	size_t lng_size;
	size_t selected_lang;	// 選ばれていたlngファイル番号
} DlgData;

static wchar_t *LangInfoText(const LangInfo *p)
{
	wchar_t *info;
	aswprintf(&info,
			  L"language\r\n"
			  L"  %s\r\n"
			  L"filename\r\n"
			  L"  %s\r\n"
			  L"date\r\n"
			  L"  %s\r\n"
			  L"contributor\r\n"
			  L"  %s",
			  p->language, p->filename, p->date, p->contributor);
	return info;
}

static INT_PTR CALLBACK GenDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_GEN_TITLE" },
		{ IDC_GENLANGUI_LABEL, "DLG_GEN_LANG_UI" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_GENHELP, "BTN_HELP" },
	};
	DlgData *data;
	PTTSet ts;

	switch (Message) {
		case WM_INITDIALOG:
			data = (DlgData *)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

			ts = data->ts;
			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts->UILanguageFileW);

			data->selected_lang = 0;
			{
				LangInfo *infos = data->lng_infos;
				size_t info_size = data->lng_size;
				size_t i;
				for (i = 0; i < info_size; i++) {
					const LangInfo *p = infos + i;
					SendDlgItemMessageW(Dialog, IDC_GENLANG_UI, CB_ADDSTRING, 0, (LPARAM)p->language);
					if (wcscmp(p->fullname, ts->UILanguageFileW) == 0) {
						data->selected_lang = i;
					}
				}
				SendDlgItemMessage(Dialog, IDC_GENLANG_UI, CB_SETCURSEL, data->selected_lang, 0);
				wchar_t *info_text = LangInfoText(infos + data->selected_lang);
				SetDlgItemTextW(Dialog, IDC_GENLANUI_INFO, info_text);
				free(info_text);
			}

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
			switch (wParam) {
				case IDOK | (BN_CLICKED << 16): {
					LRESULT w;
					data = (DlgData *)GetWindowLongPtrW(Dialog,DWLP_USER);
					ts = data->ts;
					assert(ts != NULL);

					// 言語ファイルが変更されていた場合
					w = SendDlgItemMessageA(Dialog, IDC_GENLANG_UI, CB_GETCURSEL, 0, 0);
					if (w != data->selected_lang) {
						const LangInfo *p = data->lng_infos + w;
						ts->UILanguageFileW = _wcsdup(p->fullname);

						// タイトルの更新を行う。(2014.2.23 yutaka)
						PostMessage(GetParent(Dialog),WM_USER_CHANGETITLE,0,0);
					}

					// TTXKanjiMenu は Language を見てメニューを表示するので、変更の可能性がある
					// OK 押下時にメニュー再描画のメッセージを飛ばすようにした。 (2007.7.14 maya)
					// 言語ファイルの変更時にメニューの再描画が必要 (2012.5.5 maya)
					PostMessage(GetParent(Dialog),WM_USER_CHANGEMENU,0,0);

					TTEndDialog(Dialog, 1);
					return TRUE;
				}
				case IDCANCEL | (BN_CLICKED << 16):
					TTEndDialog(Dialog, 0);
					return TRUE;

				case IDC_GENHELP | (BN_CLICKED << 16):
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,HlpSetupGeneral,0);
					break;

				case IDC_GENLANG_UI | (CBN_SELCHANGE << 16): {
					data = (DlgData *)GetWindowLongPtr(Dialog,DWLP_USER);
					size_t ui_sel = (size_t)SendDlgItemMessageA(Dialog, IDC_GENLANG_UI, CB_GETCURSEL, 0, 0);
					wchar_t *info = LangInfoText(data->lng_infos + ui_sel);
					SetDlgItemTextW(Dialog, IDC_GENLANUI_INFO, info);
					free(info);
					break;
				}

			}
			break;

		case WM_DESTROY:
			//free_lang_ui_list();
			break;
	}
	return FALSE;
}

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
								 MAKEINTRESOURCE(IDD_GENDLG),
								 WndParent, GenDlg, (LPARAM)&data);
	LangFree(infos, infos_size);
	return (BOOL)r;
}
