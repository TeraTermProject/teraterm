/*
 * (C) 2019 TeraTerm Project
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

/* debug property page */

#include <stdio.h>

#include "tmfc.h"
#include "tt_res.h"
#include "debug_pp.h"
#include "../common/tt_res.h"
#include "unicode_test.h"
#include "dlglib.h"
#include "compat_win.h"

CDebugPropPage::CDebugPropPage(HINSTANCE inst, TTCPropertySheet *sheet)
	: TTCPropertyPage(inst, IDD_TABSHEET_DEBUG, sheet)
{
}

CDebugPropPage::~CDebugPropPage()
{
}

static const struct {
	WORD key_code;
	const char *key_str;
} key_list[] = {
	{VK_CONTROL, "VT_CONTROL"},
	{VK_SHIFT, "VK_SHIFT"},
};

void CDebugPropPage::OnInitDialog()
{
	// popup
	SetCheck(IDC_DEBUG_POPUP_ENABLE, UnicodeDebugParam.CodePopupEnable);
	for (int i = 0; i < (int)_countof(key_list); i++) {
		SendDlgItemMessage(IDC_DEBUG_POPUP_KEY1, CB_ADDSTRING, 0, (LPARAM)key_list[i].key_str);
		SendDlgItemMessage(IDC_DEBUG_POPUP_KEY2, CB_ADDSTRING, 0, (LPARAM)key_list[i].key_str);
		if (UnicodeDebugParam.CodePopupKey1 == key_list[i].key_code) {
			SendDlgItemMessageA(IDC_DEBUG_POPUP_KEY1, CB_SETCURSEL, i, 0);
		}
		if (UnicodeDebugParam.CodePopupKey2 == key_list[i].key_code) {
			SendDlgItemMessageA(IDC_DEBUG_POPUP_KEY2, CB_SETCURSEL, i, 0);
		}
	}

	// console button
	const char *caption;
	HWND hWnd = pGetConsoleWindow();
	if (hWnd == NULL) {
		caption = "Open console window";
	} else {
		if (::IsWindowVisible(hWnd)) {
			caption = "Hide console window";
		} else {
			caption = "Show console window";
		}
	}
	SetDlgItemTextA(IDC_DEBUG_CONSOLE_BUTTON, caption);
}

BOOL CDebugPropPage::OnCommand(WPARAM wParam, LPARAM)
{
	switch (wParam) {
		case IDC_DEBUG_CONSOLE_BUTTON | (BN_CLICKED << 16): {
			const char *caption;
			HWND hWnd = pGetConsoleWindow();
			if (hWnd == NULL) {
				FILE *fp;
				AllocConsole();
				freopen_s(&fp, "CONOUT$", "w", stdout);
				freopen_s(&fp, "CONOUT$", "w", stderr);
				caption = "Hide console window";
				hWnd = pGetConsoleWindow();
				HMENU hmenu = GetSystemMenu(hWnd, FALSE);
				RemoveMenu(hmenu, SC_CLOSE, MF_BYCOMMAND);
			}
			else {
				if (::IsWindowVisible(hWnd)) {
					::ShowWindow(hWnd, SW_HIDE);
					caption = "Show console window";
				}
				else {
					::ShowWindow(hWnd, SW_SHOW);
					caption = "Hide console window";
				}
			}
			SetDlgItemTextA(IDC_DEBUG_CONSOLE_BUTTON, caption);
			break;
		}
		default:
			break;
	}
	return FALSE;
}

void CDebugPropPage::OnOK()
{
	UnicodeDebugParam.CodePopupEnable = GetCheck(IDC_DEBUG_POPUP_ENABLE);
	int i = GetCurSel(IDC_DEBUG_POPUP_KEY1);
	UnicodeDebugParam.CodePopupKey1 = key_list[i].key_code;
	i = GetCurSel(IDC_DEBUG_POPUP_KEY2);
	UnicodeDebugParam.CodePopupKey2 = key_list[i].key_code;
}
