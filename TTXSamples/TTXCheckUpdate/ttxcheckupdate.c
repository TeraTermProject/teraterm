
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"

#include "codeconv.h"
#include "compat_w95.h"
#include "dlglib.h"
#include "i18n.h"

#include "resource.h"
#include "parse.h"
#include "getcontent.h"

#define ORDER 4000
#define ID_MENUITEM 55900	// see reference/develop.txt

typedef struct {
	HANDLE hInst;
	PTTSet ts;
	PComVar cv;
	version_one_t *versions;
	size_t versions_count;
} TInstVar;

static TInstVar InstVar;
static TInstVar *pvar;

static int SetDropdown(HWND hDlg, int running_version)
{
	const int version_major = running_version / 10000;
	int cursor = -1;
	size_t i;

	SendDlgItemMessageW(hDlg, IDC_COMBO1, CB_RESETCONTENT, 0, 0);
	for (i = 0; i < pvar->versions_count; i++) {
		const version_one_t *v = &pvar->versions[i];
		wchar_t *strW = ToWcharU8(v->version_text);
		SendDlgItemMessageW(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)strW);
		free(strW);
		if (cursor == -1 && v->version_major == version_major) {
			cursor = (int)i;
		}
	}
	if (cursor == -1) {
		cursor = 0;
	}
	return cursor;
}

static void SetTexts(HWND hDlg, const version_one_t *version)
{
	const version_one_t *v = version;

	wchar_t *strW = ToWcharU8(v->text);
	SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT1), strW);
	free(strW);

	if (v->url == NULL) {
		EnableWindow(GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), FALSE);
		SetWindowTextA(GetDlgItem(hDlg, IDC_EDIT2), "");
	}
	else {
		EnableWindow(GetDlgItem(hDlg, IDC_BUTTON2), TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), TRUE);
		SetWindowTextA(GetDlgItem(hDlg, IDC_EDIT2), v->url);
	}
}

static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	(void)lParam;
	switch (msg) {
		case WM_INITDIALOG: {
			int cursor = SetDropdown(hDlg, pvar->ts->RunningVersion);
			SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, cursor, 0);
			SetTexts(hDlg, &pvar->versions[cursor]);
			CenterWindow(hDlg, GetParent(hDlg));
			break;
		}

		case WM_COMMAND: {
			switch (wParam) {
				case IDOK | (BN_CLICKED << 16):
					EndDialog(hDlg, 1);
					break;
				case IDCANCEL | (BN_CLICKED << 16):
					EndDialog(hDlg, 0);
					break;
				case IDC_BUTTON2 | (BN_CLICKED << 16): {
					int cursor = (int)SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
					const char *url = pvar->versions[cursor].url;
					ShellExecuteA(NULL, NULL, url, NULL, NULL, SW_SHOWNORMAL);
					break;
				}
				case IDC_COMBO1 | (CBN_SELCHANGE << 16): {
					int cursor = (int)SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
					SetTexts(hDlg, &pvar->versions[cursor]);
					break;
				}
				default:
					break;
			}
			break;
		}

		default:
			break;
	}
	return FALSE;
}

static void ShowDialog(HWND hWnd)
{
	const wchar_t *update_info_url = L"https://osdn.dl.osdn.net/storage/g/t/tt/ttssh2/snapshot/teraterm_version.json";
	const wchar_t *agent_base = L"teraterm_updatechecker";
	wchar_t buf[256];
	wchar_t agent[128];
	int result_mb;
	char *json_raw_ptr;
	size_t json_raw_size;
	BOOL result_bool;
	size_t json_size;
	char *json_ptr;
	const char *UILanguageFile = pvar->ts->UILanguageFile;
	wchar_t UIMsg[MAX_UIMSG];

	/* ファイルを取得してもok? */
	GetI18nStrW("TTXCheckUpdate", "MSG_CHECKUPDATE", UIMsg, _countof(UIMsg),
				L"Do you want to check update?\n"
				L"  %s\n",
				UILanguageFile);
	swprintf(buf, _countof(buf), UIMsg, update_info_url);
	result_mb = MessageBoxW(hWnd, buf, L"Tera Term", MB_YESNO | MB_ICONEXCLAMATION);
	if (result_mb == IDNO) {
		return;
	}

	/* ファイル取得、'\0'を追加する→ json文字列を作成 */
	swprintf(agent, _countof(agent), L"%s_%d", agent_base, pvar->ts->RunningVersion);
	result_bool = GetContent(update_info_url, agent, (void**)&json_raw_ptr, &json_raw_size);
	if (!result_bool) {
		MessageBoxW(hWnd, L"access error?", L"Tera Term", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	json_size = json_raw_size + 1;
	json_ptr = realloc(json_raw_ptr, json_size);
	if (json_ptr == NULL) {
		free(json_raw_ptr);
		return;
	}
	json_raw_ptr = NULL;
	json_ptr[json_size - 1] = '\0';

	/* jsonをパースする */
	pvar->versions = ParseJson(json_ptr, &pvar->versions_count);
	if (pvar->versions == NULL) {
		MessageBoxW(hWnd, L"parse error?", L"Tera Term", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	/* ダイアログを出す */
	SetDialogFont(pvar->ts->DialogFontName, pvar->ts->DialogFontPoint, pvar->ts->DialogFontCharSet,
				  pvar->ts->UILanguageFile, "Tera Term", "DLG_TAHOMA_FONT");
	TTDialogBoxParam(pvar->hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DlgProc, (LPARAM)pvar);

	/* 終了 */
	free(json_ptr);
	ParseFree(pvar->versions, pvar->versions_count);
	pvar->versions = NULL;
}

static void WINAPI TTXInit(PTTSet ts, PComVar cv)
{
	pvar->ts = ts;
	pvar->cv = cv;
}

/**
 *	メニューを追加する
 *
 *	@param[in]	menu			メニューハンドル
 *	@param[in]	beforeItemID	このIDのメニューの前にメニューを追加
 *	@param[in]	flags			メニューflag (InsertMenuの第3引数)
 *	@param[in]	newItemID		メニューID (InsertMenuの第4引数)
 *	@param[in]	text			メニュー文字列 (InsertMenuの第5引数)
 *
 *	TODO: ttlibに移動
 */
static void insertMenuBeforeItem(HMENU menu, WORD beforeItemID, WORD flags,
                                 WORD newItemID, char *text)
{
	int i, j;

	for (i = GetMenuItemCount(menu) - 1; i >= 0; i--) {
		HMENU submenu = GetSubMenu(menu, i);

		for (j = GetMenuItemCount(submenu) - 1; j >= 0; j--) {
			if (GetMenuItemID(submenu, j) == beforeItemID) {
				InsertMenu(submenu, j, MF_BYPOSITION | flags, newItemID, text);
				return;
			}
		}
	}
}

static void WINAPI TTXModifyMenu(HMENU menu)
{
	static const DlgTextInfo MenuTextInfo[] = {
		{ ID_MENUITEM, "MENU_CHECKUPDATE" },
	};
	const UINT ID_HELP_ABOUT = 50990;
	const char *UILanguageFile = pvar->ts->UILanguageFile;

	insertMenuBeforeItem(menu, ID_HELP_ABOUT, MF_ENABLED, ID_MENUITEM, "Check &Update...");

	SetI18MenuStrs("TTXCheckUpdate", menu, MenuTextInfo, _countof(MenuTextInfo), UILanguageFile);
}

static int WINAPI TTXProcessCommand(HWND hWin, WORD cmd)
{
	if (cmd == ID_MENUITEM) {
		ShowDialog(hWin);
		return 1;
	}
	return 0;
}

static TTXExports Exports = {
	sizeof(TTXExports),
	ORDER,

	TTXInit,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	TTXModifyMenu,
	NULL,
	TTXProcessCommand,
	NULL,
	NULL,
	NULL,
	NULL,
};

BOOL __declspec(dllexport) WINAPI TTXBind(WORD Version, TTXExports *exports)
{
	size_t size;

	if (Version != TTVERSION) {
		return FALSE;
	}

	if (!IsWindowsNTKernel()) {
		// TODO Windows10以外、未検証
		return FALSE;
	}

	size = sizeof(Exports) - sizeof(exports->size);
	if ((int)size > exports->size) {
		size = exports->size;
	}
	memcpy((char *)exports + sizeof(exports->size), (char *)&Exports + sizeof(exports->size), size);
	return TRUE;
}

BOOL WINAPI DllMain(HANDLE hInstance, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	(void)lpReserved;
	switch (ul_reason_for_call) {
		case DLL_THREAD_ATTACH:
			/* do thread initialization */
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF) | _CRTDBG_LEAK_CHECK_DF);
			break;
		case DLL_THREAD_DETACH:
			/* do thread cleanup */
			break;
		case DLL_PROCESS_ATTACH:
			/* do process initialization */
			DoCover_IsDebuggerPresent();
			pvar = &InstVar;
			pvar->hInst = hInstance;
			break;
		case DLL_PROCESS_DETACH:
			/* do process cleanup */
			break;
	}
	return TRUE;
}
