

/* debug property page */

#include <stdio.h>

#include "tmfc.h"
#include "tt_res.h"
#include "debug_pp.h"
#include "../common/tt_res.h"
#include "unicode_test.h"
#include "dlglib.h"

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
	SetCheck(IDC_DEBUG_POPUP_ENABLE, UnicodeDebugParam.CodePopupEnable);
	for (int i = 0; i < _countof(key_list); i++) {
		const char *key_str = key_list[i].key_str;
		SendDlgItemMessage(IDC_DEBUG_POPUP_KEY1, CB_ADDSTRING, 0, (LPARAM)key_list[i].key_str);
		SendDlgItemMessage(IDC_DEBUG_POPUP_KEY2, CB_ADDSTRING, 0, (LPARAM)key_list[i].key_str);
		if (UnicodeDebugParam.CodePopupKey1 == key_list[i].key_code) {
			SendDlgItemMessageA(IDC_DEBUG_POPUP_KEY1, CB_SETCURSEL, i, 0);
		}
		if (UnicodeDebugParam.CodePopupKey2 == key_list[i].key_code) {
			SendDlgItemMessageA(IDC_DEBUG_POPUP_KEY2, CB_SETCURSEL, i, 0);
		}
	}
}

BOOL CDebugPropPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_DEBUG_CONSOLE_BUTTON | (BN_CLICKED << 16): {
			FILE *fp;
			AllocConsole();
			freopen_s(&fp, "CONOUT$", "w", stdout);
			freopen_s(&fp, "CONOUT$", "w", stderr);
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
