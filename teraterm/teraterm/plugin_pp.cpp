/*
 * (C) 2025- TeraTerm Project
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

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>

#include "tttypes.h"
#include "ttcommon.h"
#include "ttdialog.h"
#include "ttlib.h"
#include "dlglib.h"
#include "compat_win.h"
#include "asprintf.h"
#include "helpid.h"
#include "win32helper.h"
#include "ttlib_types.h"
#include "ttplug.h"

#include "plugin_pp.h"
#include "plugin_pp_res.h"

#define REWRITE_TEMPLATE

typedef struct {
	HINSTANCE hInst;
	TTTSet *pts;
	DLGTEMPLATE *dlg_templ;
} PluginDlgData;

/**
 * @brief リストの内容を設定する
 *
 * @param hWnd			親ウィンドウ
 * @param pointer_pos	ポインタの位置
 * @param index
 */
static void SetFromListView(HWND hWnd)
{
	HWND hWndList = GetDlgItem(hWnd, IDC_SETUP_DIR_LIST);
	const int list_count = (int)SendMessageW(hWndList, LVM_GETITEMCOUNT, 0, 0);

	for (int i = 0; i < list_count; i++) {
		wchar_t filename[MAX_PATH];
		LV_ITEMW item{};
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = filename;
		item.cchTextMax = _countof(filename);
		SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM)&item);

		wchar_t enable_str[32];
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 1;
		item.pszText = enable_str;
		item.cchTextMax = _countof(enable_str);
		SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM)&item);
		ExtensionEnable enable =
			enable_str[0] == L'e' ? EXTENSION_ENABLE :
			enable_str[0] == L'd' ? EXTENSION_DISABLE : EXTENSION_UNSPECIFIED;

		PluginInfo info = {};
		info.filename = filename;
		info.enable = enable;
		PluginAddInfo(&info);
	}
}

/**
 * 署名検証
 */
static wchar_t *VerifySignature(LPCWSTR pwszSourceFile)
{
	if (pWinVerifyTrust == NULL) {
		// WinVerifyTrustが使用できない (XP未満)
		return _wcsdup(L"-");
	}

	WINTRUST_FILE_INFO FileData = {};
	FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
	FileData.pcwszFilePath = pwszSourceFile;

	WINTRUST_DATA WinTrustData = {};
	WinTrustData.cbStruct = sizeof(WinTrustData);
	WinTrustData.dwUIChoice = WTD_UI_NONE;
	WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
	WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
	WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
	WinTrustData.pFile = &FileData;

	GUID guidAction = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	LONG r = pWinVerifyTrust(NULL, &guidAction, &WinTrustData);	// XP+

	wchar_t *str;
	switch (r) {
	case ERROR_SUCCESS:
		str = _wcsdup(L"verified");
		break;
	case TRUST_E_NOSIGNATURE:
		str = _wcsdup(L"no signature");
		break;
	default:
		aswprintf(&str, L"not verified(%08x)", r);
		break;
	}

	// 状態データを閉じる
	WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
	pWinVerifyTrust(NULL, &guidAction, &WinTrustData);

	return str;
}

/**
 * @brief メニューを出して選択された処理を実行する
 * @param hWnd			親ウィンドウ
 * @param pointer_pos	ポインタの位置
 * @param index
 */
static void Popup(HWND hWnd, const POINT *pointer_pos, int index)
{
	wchar_t enable_str[32];
	LV_ITEMW item{};
	item.mask = LVIF_TEXT;
	item.iItem = index;
	item.iSubItem = 1;
	item.pszText = enable_str;
	item.cchTextMax = _countof(enable_str);
	SendMessageW(hWnd, LVM_GETITEMW, 0, (LPARAM)&item);
	ExtensionEnable enable =
		enable_str[0] == L'e' ? EXTENSION_ENABLE :
		enable_str[0] == L'd' ? EXTENSION_DISABLE : EXTENSION_UNSPECIFIED;

	HMENU hMenu= CreatePopupMenu();
	AppendMenuW(hMenu, enable != EXTENSION_ENABLE ? MF_ENABLED : MF_DISABLED | MF_STRING, 1, L"&Enable");
	AppendMenuW(hMenu, enable != EXTENSION_DISABLE ? MF_ENABLED : MF_DISABLED | MF_STRING, 2, L"&Disable");
	int result = TrackPopupMenu(hMenu, TPM_RETURNCMD, pointer_pos->x, pointer_pos->y, 0 , hWnd, NULL);
	DestroyMenu(hMenu);
	switch (result) {
	case 1:
	case 2: {
		item.mask = LVIF_TEXT;
		item.iItem = index;
		item.iSubItem = 1;
		item.pszText = result == 1 ? (LPWSTR)L"enable" : (LPWSTR)L"disable";
		SendMessageW(hWnd, LVM_SETITEMW, 0, (LPARAM)&item);
		break;
	}
	default:
		break;
	}
}

static INT_PTR CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_SETUPDIR_TITLE" },
	};

	switch (msg) {
	case WM_INITDIALOG: {
		PluginDlgData *data = (PluginDlgData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
		SetWindowLongPtrW(hWnd, DWLP_USER, (LONG_PTR)data);
		TTTSet *pts = data->pts;

		SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), pts->UILanguageFileW);

		HWND hWndList = GetDlgItem(hWnd, IDC_SETUP_DIR_LIST);
		ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

		LV_COLUMNA lvcol{};
		lvcol.mask = LVCF_TEXT | LVCF_SUBITEM;
		lvcol.pszText = (LPSTR)"path";
		lvcol.iSubItem = 0;
		SendMessageA(hWndList, LVM_INSERTCOLUMNA, 0, (LPARAM)&lvcol);

		lvcol.pszText = (LPSTR)"enable";
		lvcol.iSubItem = 1;
		SendMessageA(hWndList, LVM_INSERTCOLUMNA, 1, (LPARAM)&lvcol);

		lvcol.pszText = (LPSTR)"loaded";
		lvcol.iSubItem = 2;
		SendMessageA(hWndList, LVM_INSERTCOLUMNA, 2, (LPARAM)&lvcol);

		lvcol.pszText = (LPSTR)"order";
		lvcol.iSubItem = 3;
		SendMessageA(hWndList, LVM_INSERTCOLUMNA, 3, (LPARAM)&lvcol);

		lvcol.pszText = (LPSTR)"verify";
		lvcol.iSubItem = 4;
		SendMessageA(hWndList, LVM_INSERTCOLUMNA, 4, (LPARAM)&lvcol);

		for (int i = 0;; i++) {
			PluginInfo info;
			if(PluginGetInfo(i, &info) == FALSE) {
				break;
			}

			LVITEMW item{};
			item.mask = LVIF_TEXT;
			item.iItem = i;
			item.iSubItem = 0;
			item.pszText = (LPWSTR)info.filename;
			SendMessageW(hWndList, LVM_INSERTITEMW, 0, (LPARAM)&item);

			const wchar_t *enable_str =
				info.enable == EXTENSION_DISABLE ? L"disable" :
				info.enable == EXTENSION_ENABLE	? L"enable" :
				L"unspecified";
			item.iSubItem = 1;
			item.pszText = (LPWSTR)enable_str;
			SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&item);

			const wchar_t *loaded_str =
				info.loaded == EXTENSION_UNLOADED ? L"unloaded" : L"loaded";
			item.mask = LVIF_TEXT;
			item.iSubItem = 2;
			item.pszText = (LPWSTR)loaded_str;
			SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&item);

			wchar_t *order_str;
			if (info.loaded == EXTENSION_UNLOADED) {
				order_str = _wcsdup(L"-");
			} else {
				aswprintf(&order_str, L"%d", info.load_order);
			}
			item.iSubItem = 3;
			item.pszText = (LPWSTR)order_str;
			SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&item);
			free(order_str);

			wchar_t *verify_str = VerifySignature(info.filename);
			item.iSubItem = 4;
			item.pszText = verify_str;
			SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&item);
			free(verify_str);
		}

		// 幅を調整
		for (int i = 0; i < 5; i++) {
			ListView_SetColumnWidth(hWndList, i, LVSCW_AUTOSIZE);
		}

		return TRUE;
	}
	case WM_COMMAND: {
		PluginDlgData *data = (PluginDlgData *)GetWindowLongPtrW(hWnd, DWLP_USER);
		TTTSet *pts = data->pts;
		switch (LOWORD(wp)) {
		case IDC_BUTTON_ADD: {
			wchar_t *filename;
			TTOPENFILENAMEW ofn = {};
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = L"plugin (*.dll)\0*.dll\0all (*.*)\0*.*\0";
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.lpstrDefExt = L"dll";
			ofn.lpstrTitle = L"title";
			ofn.lpstrInitialDir = pts->ExeDirW;
			BOOL r = TTGetOpenFileNameW(&ofn, &filename);
			if (r == TRUE) {
				// 選択されたファイルをリストに追加
				HWND hWndList = GetDlgItem(hWnd, IDC_SETUP_DIR_LIST);

				int list_count = (int)SendMessageW(hWndList, LVM_GETITEMCOUNT, 0, 0);

				LVITEMW item{};
				item.mask = LVIF_TEXT;
				item.iItem = list_count;
				item.iSubItem = 0;
				item.pszText = filename;
				SendMessageW(hWndList, LVM_INSERTITEMW, 0, (LPARAM)&item);

				item.iSubItem = 1;
				item.pszText = (LPWSTR)L"enable";
				SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&item);

				free(filename);
			}
			break;
		}
		default:
			return FALSE;
		}
		return FALSE;
	}
	case WM_NOTIFY: {
		NMHDR *nmhdr = (NMHDR *)lp;
		if (nmhdr->code == PSN_HELP) {
			HWND vtwin = GetParent(hWnd);
			vtwin = GetParent(vtwin);
			PostMessageA(vtwin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalPlugin, 0);
		} else if (nmhdr->code == PSN_APPLY) {
			SetFromListView(hWnd);
		} else if (nmhdr->idFrom == IDC_SETUP_DIR_LIST) {
			NMLISTVIEW *nmlist = (NMLISTVIEW *)lp;
			switch (nmlist->hdr.code) {
			case NM_RCLICK: {
				POINT pointer_pos;
				GetCursorPos(&pointer_pos);
				LVHITTESTINFO ht = {};
				ht.pt = pointer_pos;
				ScreenToClient(nmlist->hdr.hwndFrom, &ht.pt);
				ListView_HitTest(nmlist->hdr.hwndFrom, &ht);
				if (ht.flags & LVHT_ONITEM) {
					int hit_item = ht.iItem;
					Popup(nmlist->hdr.hwndFrom, &pointer_pos, hit_item);
				}
				break;
			}
			}
		}
		break;
	}
	default:
		return FALSE;
	}

	return FALSE;
}

static UINT CALLBACK CallBack(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEW *ppsp)
{
	UINT ret_val = 0;
	switch (uMsg) {
	case PSPCB_CREATE:
		ret_val = 1;
		break;
	case PSPCB_RELEASE: {
		PluginDlgData *dlg_data = (PluginDlgData *)ppsp->lParam;

		free((void *)ppsp->pResource);
		ppsp->pResource = NULL;
		free(dlg_data);
		ppsp->lParam = (LPARAM)NULL;
		break;
	}
	default:
		break;
	}
	return ret_val;
}

HPROPSHEETPAGE PluginPageCreate(HINSTANCE inst, TTTSet *pts)
{
	PluginDlgData *data = (PluginDlgData *)calloc(1, sizeof(PluginDlgData));
	data->hInst = inst;
	data->pts = pts;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", NULL,
				 L"Plugin", pts->UILanguageFileW, &UIMsg);
	psp.pszTitle = UIMsg;
	psp.pszTemplate = MAKEINTRESOURCEW(IDD_TABSHEET_PLUGIN);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	data->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(IDD_TABSHEET_PLUGIN));
	psp.pResource = data->dlg_templ;
#endif

	psp.pfnDlgProc = Proc;
	psp.lParam = (LPARAM)data;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(UIMsg);
	return hpsp;
}
