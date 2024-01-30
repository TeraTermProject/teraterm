/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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
/* IPv6 modification is Copyright(C) 2000 Jun-ya Kato <kato@win6.jp> */

/* TTDLG.DLL, dialog boxes */
#include "teraterm.h"
#include <stdio.h>
#include <string.h>
#include <commdlg.h>
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
#include "ttlib_charset.h"
#include "ttwinman.h"
#include "resize_helper.h"

#include "ttdlg.h"

static INT_PTR CALLBACK DirDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_DIR_TITLE" },
		{ IDC_DIRCURRENT_LABEL, "DLG_DIR_CURRENT" },
		{ IDC_DIRNEW_LABEL, "DLG_DIR_NEW" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_DIRHELP, "BTN_HELP" },
	};

	switch (Message) {
		case WM_INITDIALOG: {
			PTTSet ts;
			wchar_t *CurDir;
			RECT R;
			HDC TmpDC;
			SIZE s;
			HWND HDir, HSel, HOk, HCancel, HHelp;
			POINT D, B, S;
			int WX, WY, WW, WH, CW, DW, DH, BW, BH, SW, SH;

			ts = (PTTSet)lParam;
			CurDir = ts->FileDirW;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);
			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts->UILanguageFileW);
			SetDlgItemTextW(Dialog, IDC_DIRCURRENT, CurDir);

// adjust dialog size
			// get size of current dir text
			HDir = GetDlgItem(Dialog, IDC_DIRCURRENT);
			GetWindowRect(HDir,&R);
			D.x = R.left;
			D.y = R.top;
			ScreenToClient(Dialog,&D);
			DH = R.bottom-R.top;
			TmpDC = GetDC(Dialog);
			GetTextExtentPoint32W(TmpDC,CurDir,(int)wcslen(CurDir),&s);
			ReleaseDC(Dialog,TmpDC);
			DW = s.cx + s.cx/10;

			// get button size
			HOk = GetDlgItem(Dialog, IDOK);
			HCancel = GetDlgItem(Dialog, IDCANCEL);
			HHelp = GetDlgItem(Dialog, IDC_DIRHELP);
			GetWindowRect(HHelp,&R);
			B.x = R.left;
			B.y = R.top;
			ScreenToClient(Dialog,&B);
			BW = R.right-R.left;
			BH = R.bottom-R.top;

			// calc new dialog size
			GetWindowRect(Dialog,&R);
			WX = R.left;
			WY = R.top;
			WW = R.right-R.left;
			WH = R.bottom-R.top;
			GetClientRect(Dialog,&R);
			CW = R.right-R.left;
			if (D.x+DW < CW) {
				DW = CW-D.x;
			}
			WW = WW + D.x+DW - CW;

			// resize current dir text
			MoveWindow(HDir,D.x,D.y,DW,DH,TRUE);
			// move buttons
			MoveWindow(HOk,(D.x+DW-4*BW)/2,B.y,BW,BH,TRUE);
			MoveWindow(HCancel,(D.x+DW-BW)/2,B.y,BW,BH,TRUE);
			MoveWindow(HHelp,(D.x+DW+2*BW)/2,B.y,BW,BH,TRUE);
			// resize edit box
			HDir = GetDlgItem(Dialog, IDC_DIRNEW);
			GetWindowRect(HDir,&R);
			D.x = R.left;
			D.y = R.top;
			ScreenToClient(Dialog,&D);
			DW = R.right-R.left;
			if (DW<s.cx) {
				DW = s.cx;
			}
			MoveWindow(HDir,D.x,D.y,DW,R.bottom-R.top,TRUE);
			// select dir button
			HSel = GetDlgItem(Dialog, IDC_SELECT_DIR);
			GetWindowRect(HSel, &R);
			SW = R.right-R.left;
			SH = R.bottom-R.top;
			S.x = R.bottom;
			S.y = R.top;
			ScreenToClient(Dialog, &S);
			MoveWindow(HSel, D.x + DW + 8, S.y, SW, SH, TRUE);
			WW = WW + SW;

			// resize dialog
			MoveWindow(Dialog,WX,WY,WW,WH,TRUE);

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					PTTSet ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
					BOOL OK = FALSE;
					wchar_t *current_dir;
					wchar_t *new_dir;
					hGetCurrentDirectoryW(&current_dir);
					hGetDlgItemTextW(Dialog, IDC_DIRNEW, &new_dir);
					if (wcslen(new_dir) > 0) {
						wchar_t *FileDirExpanded;
						hExpandEnvironmentStringsW(new_dir, &FileDirExpanded);
						if (DoesFolderExistW(FileDirExpanded)) {
							free(ts->FileDirW);
							ts->FileDirW = new_dir;
							OK = TRUE;
						}
						else {
							free(new_dir);
						}
						free(FileDirExpanded);
					}
					SetCurrentDirectoryW(current_dir);
					free(current_dir);
					if (OK) {
						TTEndDialog(Dialog, 1);
					}
					else {
						static const TTMessageBoxInfoW info = {
							"Tera Term",
							"MSG_TT_ERROR", L"Tera Term: Error",
							"MSG_FIND_DIR_ERROR", L"Cannot find directory",
							MB_ICONEXCLAMATION
						};
						TTMessageBoxW(Dialog, &info, ts->UILanguageFileW);
					}
					return TRUE;
				}

				case IDCANCEL:
					TTEndDialog(Dialog, 0);
					return TRUE;

				case IDC_SELECT_DIR: {
					PTTSet ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
					wchar_t *uimsgW;
					wchar_t *buf;
					wchar_t *FileDirExpanded;
					GetI18nStrWW("Tera Term", "DLG_SELECT_DIR_TITLE", L"Select new directory", ts->UILanguageFileW, &uimsgW);
					hGetDlgItemTextW(Dialog, IDC_DIRNEW, &buf);
					if (buf[0] == 0) {
						free(buf);
						hGetDlgItemTextW(Dialog, IDC_DIRCURRENT, &buf);
					}
					hExpandEnvironmentStringsW(buf, &FileDirExpanded);
					free(buf);
					if (doSelectFolderW(Dialog, FileDirExpanded, uimsgW, &buf)) {
						SetDlgItemTextW(Dialog, IDC_DIRNEW, buf);
						free(buf);
					}
					free(FileDirExpanded);
					free(uimsgW);
					return TRUE;
				}

				case IDC_DIRHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,HlpFileChangeDir,0);
					break;
			}
	}
	return FALSE;
}

BOOL WINAPI _ChangeDirectory(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)TTDialogBoxParam(hInst,
							   MAKEINTRESOURCE(IDD_DIRDLG),
							   WndParent, DirDlg, (LPARAM)ts);
}
