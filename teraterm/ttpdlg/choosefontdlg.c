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

/* choosefont dialog */
#include "teraterm.h"
#include <stdio.h>
#include <string.h>
#include <commdlg.h>
#include <dlgs.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "ttcommon.h"
#include "dlg_res.h"
#include "codeconv.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "compat_win.h"
#include "asprintf.h"
#include "ttwinman.h"

#include "ttdlg.h"

static UINT_PTR CALLBACK TFontHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) {
		case WM_INITDIALOG:
		{
			PTTSet ts;
			wchar_t *uimsg;

			//EnableWindow(GetDlgItem(Dialog, cmb2), FALSE);
			ts = (PTTSet)((CHOOSEFONTA *)lParam)->lCustData;

			GetI18nStrWW("Tera Term", "DLG_CHOOSEFONT_STC6", L"\"Font style\" selection here won't affect actual font appearance.",
						 ts->UILanguageFileW, &uimsg);
			SetDlgItemTextW(Dialog, stc6, uimsg);
			free(uimsg);

			SetFocus(GetDlgItem(Dialog,cmb1));

			CenterWindow(Dialog, GetParent(Dialog));

			break;
		}
#if 0
		case WM_COMMAND:
			if (LOWORD(wParam) == cmb2) {
				if (HIWORD(wParam) == CBN_SELCHANGE) {
					// フォントの変更による(メッセージによる)スタイルの変更では
					// cmb2 からの通知が来ない
					SendMessage(GetDlgItem(Dialog, cmb2), CB_GETCURSEL, 0, 0);
				}
			}
			else if (LOWORD(wParam) == cmb1) {
				if (HIWORD(wParam) == CBN_SELCHANGE) {
					// フォントの変更前に一時保存されたスタイルが
					// ここを抜けたあとに改めてセットされてしまうようだ
					SendMessage(GetDlgItem(Dialog, cmb2), CB_GETCURSEL, 0, 0);
				}
			}
			break;
#endif
	}
	return FALSE;
}

BOOL WINAPI _ChooseFontDlg(HWND WndParent, LPLOGFONTA LogFont, PTTSet ts)
{
	CHOOSEFONTA cf;
	BOOL Ok;

	memset(&cf, 0, sizeof(cf));
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = WndParent;
	cf.lpLogFont = LogFont;
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT |
	           CF_FIXEDPITCHONLY | CF_SHOWHELP | CF_NOVERTFONTS |
	           CF_ENABLEHOOK;
	if (ts->ListHiddenFonts) {
		cf.Flags |= CF_INACTIVEFONTS;
	}
	cf.lpfnHook = (LPCFHOOKPROC)(&TFontHook);
	cf.nFontType = REGULAR_FONTTYPE;
	cf.hInstance = hInst;
	cf.lCustData = (LPARAM)ts;
	Ok = ChooseFontA(&cf);
	return Ok;
}
