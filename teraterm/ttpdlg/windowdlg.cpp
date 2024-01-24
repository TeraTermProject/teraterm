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

/* window dlg */
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
#include "tipwin.h"
#include "codeconv.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "compat_win.h"
#include "ttlib_charset.h"
#include "asprintf.h"
#include "ttwinman.h"

#include "ttdlg.h"

static INT_PTR CALLBACK WinListDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_WINLIST_TITLE" },
		{ IDC_WINLISTLABEL, "DLG_WINLIST_LABEL" },
		{ IDOK, "DLG_WINLIST_OPEN" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_WINLISTCLOSE, "DLG_WINLIST_CLOSEWIN" },
		{ IDC_WINLISTHELP, "BTN_HELP" },
	};
	PBOOL Close;
	int n;
	HWND Hw;

	switch (Message) {
		case WM_INITDIALOG:
			Close = (PBOOL)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

			SetWinList(GetParent(Dialog),Dialog,IDC_WINLISTLIST);

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					n = SendDlgItemMessage(Dialog,IDC_WINLISTLIST,
					LB_GETCURSEL, 0, 0);
					if (n!=CB_ERR) {
						SelectWin(n);
					}
					EndDialog(Dialog, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;

				case IDC_WINLISTLIST:
					if (HIWORD(wParam)==LBN_DBLCLK) {
						PostMessage(Dialog,WM_COMMAND,IDOK,0);
					}
					break;

				case IDC_WINLISTCLOSE:
					n = SendDlgItemMessage(Dialog,IDC_WINLISTLIST,
					LB_GETCURSEL, 0, 0);
					if (n==CB_ERR) {
						break;
					}
					Hw = GetNthWin(n);
					if (Hw!=GetParent(Dialog)) {
						if (! IsWindowEnabled(Hw)) {
							MessageBeep(0);
							break;
						}
						SendDlgItemMessage(Dialog,IDC_WINLISTLIST,
						                   LB_DELETESTRING,n,0);
						PostMessage(Hw,WM_SYSCOMMAND,SC_CLOSE,0);
					}
					else {
						Close = (PBOOL)GetWindowLongPtr(Dialog,DWLP_USER);
						if (Close!=NULL) {
							*Close = TRUE;
						}
						EndDialog(Dialog, 1);
						return TRUE;
					}
					break;

				case IDC_WINLISTHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,HlpWindowWindow,0);
					break;
			}
	}
	return FALSE;
}

BOOL WINAPI _WindowWindow(HWND WndParent, PBOOL Close)
{
	*Close = FALSE;
	return
		(BOOL)TTDialogBoxParam(hInst,
							   MAKEINTRESOURCE(IDD_WINLISTDLG),
							   WndParent,
							   WinListDlg, (LPARAM)Close);
}
