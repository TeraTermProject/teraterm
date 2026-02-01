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

/* Mouse property page */
#include <stdio.h>
#include <string.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "helpid.h"

#include "dlg_res.h"
#include "resource.h"
#include "mouse_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

typedef struct {
	HINSTANCE hInst;
	TTTSet *pts;
	DLGTEMPLATE *dlg_templ;
} MousePPData;

static INT_PTR CALLBACK proc(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_CLICKABLE_URL, "DLG_TAB_GENERAL_CLICKURL" },
		{ IDC_MOUSEWHEEL_SCROLL_LINE, "DLG_TAB_GENERAL_MOUSEWHEEL_SCROLL_LINE" },
	};

	switch (Message) {
		case WM_INITDIALOG: {
			MousePPData *data = (MousePPData *)(((PROPSHEETPAGEW_V1 *)lParam)->lParam);
			PTTSet pts = data->pts;
			SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)data);

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), pts->UILanguageFileW);

			CheckDlgButton(Dialog, IDC_CLICKABLE_URL, pts->EnableClickableUrl);
			SetDlgNum(Dialog, IDC_SCROLL_LINE, pts->MouseWheelScrollLine);

			return TRUE;
		}

		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lParam;
			MousePPData *data = (MousePPData *)GetWindowLongPtr(Dialog, DWLP_USER);
			PTTSet pts = data->pts;
			switch (nmhdr->code) {
			case PSN_APPLY: {
				pts->EnableClickableUrl = ::IsDlgButtonChecked(Dialog, IDC_CLICKABLE_URL);
				pts->MouseWheelScrollLine = GetDlgItemInt(Dialog, IDC_SCROLL_LINE, NULL, FALSE);

				break;
			}
			case PSN_HELP: {
				HWND vtwin = GetParent(GetParent(Dialog));
				PostMessage(vtwin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalMouse, 0);
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

HPROPSHEETPAGE MousePageCreate(HINSTANCE inst, TTTSet *pts)
{
	MousePPData *Param = (MousePPData *)calloc(1, sizeof(MousePPData));
	Param->hInst = inst;
	Param->pts = pts;

	PROPSHEETPAGEW_V1 psp = {0};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t* UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_MOUSE",
				 L"Mouse", pts->UILanguageFileW, &UIMsg);
	psp.pszTitle = UIMsg;
	psp.pszTemplate = MAKEINTRESOURCEW(IDD_TABSHEET_MOUSE);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	Param->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(IDD_TABSHEET_MOUSE));
	psp.pResource = Param->dlg_templ;
#endif

	psp.pfnDlgProc = proc;
	psp.lParam = (LPARAM)Param;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(UIMsg);
	return hpsp;
}
