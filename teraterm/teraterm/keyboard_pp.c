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

/* keyboard property page */

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
#include "helpid.h"

#include "keyboard_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

static const char *RussList2[] = {"Windows","KOI8-R",NULL};

typedef struct {
	HINSTANCE hInst;
	TTTSet *pts;
	DLGTEMPLATE *dlg_templ;
} KBPPData;

static INT_PTR CALLBACK proc(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_KEYBTRANS, "DLG_KEYB_TRANSMIT" },
		{ IDC_KEYBBS, "DLG_KEYB_BS" },
		{ IDC_KEYBDEL, "DLG_KEYB_DEL" },
		{ IDC_KEYBKEYBTEXT, "DLG_KEYB_KEYB" },
		{ IDC_KEYBMETATEXT, "DLG_KEYB_META" },
		{ IDC_KEYBDISABLE, "DLG_KEYB_DISABLE" },
		{ IDC_KEYBAPPKEY, "DLG_KEYB_APPKEY" },
		{ IDC_KEYBAPPCUR, "DLG_KEYB_APPCUR" },
	};

	switch (Message) {
		case WM_INITDIALOG: {
			KBPPData *data = (KBPPData *)(((PROPSHEETPAGEW_V1 *)lParam)->lParam);
			PTTSet ts = data->pts;
			SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)data);

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts->UILanguageFileW);

			SetRB(Dialog,ts->BSKey-1,IDC_KEYBBS,IDC_KEYBBS);
			SetRB(Dialog,ts->DelKey,IDC_KEYBDEL,IDC_KEYBDEL);
			SetRB(Dialog,ts->MetaKey,IDC_KEYBMETA,IDC_KEYBMETA);
			SetRB(Dialog,ts->DisableAppKeypad,IDC_KEYBAPPKEY,IDC_KEYBAPPKEY);
			SetRB(Dialog,ts->DisableAppCursor,IDC_KEYBAPPCUR,IDC_KEYBAPPCUR);

			if (!IsWindowsNTKernel()) {
				static const I18nTextInfo infos[] = {
					{ "DLG_KEYB_META_OFF", L"off", IdMetaOff },
					{ "DLG_KEYB_META_ON", L"on", IdMetaOn }
				};
				SetI18nListW("Tera Term", Dialog, IDC_KEYBMETA, infos, _countof(infos), ts->UILanguageFileW, ts->MetaKey);
			}
			else {
				static const I18nTextInfo infos[] = {
					{ "DLG_KEYB_META_OFF", L"off", IdMetaOff },
					{ "DLG_KEYB_META_ON", L"on", IdMetaOn },
					{ "DLG_KEYB_META_LEFT", L"left", IdMetaLeft },
					{ "DLG_KEYB_META_RIGHT", L"right", IdMetaRight }
				};
				SetI18nListW("Tera Term", Dialog, IDC_KEYBMETA, infos, _countof(infos), ts->UILanguageFileW, ts->MetaKey);
			}

			SetDropDownList(Dialog, IDC_KEYBKEYB, RussList2, ts->RussKeyb);
			if (IsWindowUnicode(Dialog) != TRUE || GetACP() != 1251) {
				// 非Unicode(ANSI)動作 && CP1251(Russian)のとき、
				// このオプションは使用可能となる
				EnableWindow(GetDlgItem(Dialog, IDC_KEYBKEYBTEXT), FALSE);
				EnableWindow(GetDlgItem(Dialog, IDC_KEYBKEYB), FALSE);
			}

			ShowWindow(GetDlgItem(Dialog, IDOK), FALSE);
			ShowWindow(GetDlgItem(Dialog, IDCANCEL), FALSE);
			ShowWindow(GetDlgItem(Dialog, IDC_KEYBHELP), FALSE);

			return TRUE;
		}

		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lParam;
			KBPPData *data = (KBPPData *)GetWindowLongPtr(Dialog, DWLP_USER);
			PTTSet ts = data->pts;
			switch (nmhdr->code) {
			case PSN_APPLY: {
				WORD w;

				GetRB(Dialog,&ts->BSKey,IDC_KEYBBS,IDC_KEYBBS);
				ts->BSKey++;
				GetRB(Dialog,&ts->DelKey,IDC_KEYBDEL,IDC_KEYBDEL);
				GetRB(Dialog,&ts->DisableAppKeypad,IDC_KEYBAPPKEY,IDC_KEYBAPPKEY);
				GetRB(Dialog,&ts->DisableAppCursor,IDC_KEYBAPPCUR,IDC_KEYBAPPCUR);
				if ((w = (WORD)GetCurSel(Dialog, IDC_KEYBMETA)) > 0) {
					ts->MetaKey = w - 1;
				}
				if ((w = (WORD)GetCurSel(Dialog, IDC_KEYBKEYB)) > 0) {
					ts->RussKeyb = w;
				}
				break;
			}
			case PSN_HELP: {
				HWND vtwin = GetParent(GetParent(Dialog));
				PostMessage(vtwin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalKeyboard, 0);
				break;
			}
			}
			break;
		}
	}
	return FALSE;
}

static UINT CALLBACK CallBack(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEW *ppsp)
{
	(void)hwnd;
	UINT ret_val = 0;
	switch (uMsg) {
	case PSPCB_CREATE:
		ret_val = 1;
		break;
	case PSPCB_RELEASE:
		free((void *)ppsp->pResource);
		ppsp->pResource = NULL;
		free((void *)ppsp->lParam);
		ppsp->lParam = (LPARAM)NULL;
		break;
	default:
		break;
	}
	return ret_val;
}

HPROPSHEETPAGE KeyboardPageCreate(HINSTANCE inst, TTTSet *pts)
{
	const int id = IDD_KEYBDLG;

	KBPPData *Param = (KBPPData *)calloc(1, sizeof(KBPPData));
	Param->hInst = inst;
	Param->pts = pts;

	PROPSHEETPAGEW_V1 psp = {0};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_KEYB_TITLE",
				 L"Keyboard", pts->UILanguageFileW, &UIMsg);
	psp.pszTitle = UIMsg;
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	Param->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(id));
	psp.pResource = Param->dlg_templ;
#endif

	psp.pfnDlgProc = proc;
	psp.lParam = (LPARAM)Param;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(UIMsg);
	return hpsp;
}
