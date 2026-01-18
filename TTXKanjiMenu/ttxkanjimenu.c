/*
 * TTX KanjiMenu Plugin
 *    Copyright (C) 2007 Sunao HARA (naoh@nagoya-u.jp)
 *    (C) 2007- TeraTerm Project
 */

//// ORIGINAL SOURCE CODE: ttxtest.c

/* Tera Term extension mechanism
   Robert O'Callahan (roc+tt@cs.cmu.edu)

   Tera Term by Takashi Teranishi (teranishi@rikaxp.riken.go.jp)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <winsock2.h>
#include <windows.h>
#include <htmlhelp.h>

#include "teraterm.h"
#include "tttypes.h"
#include "tttypes_charset.h"
#include "ttplugin.h"
#include "tt_res.h"
#include "i18n.h"
#include "inifile_com.h"
#include "dlglib.h"
#include "asprintf.h"
#include "helpid.h"

#include "ttxkanjimenu.h"
#include "resource.h"

#define IniSection "TTXKanjiMenu"
#define IniSectionW L"TTXKanjiMenu"

#define ORDER 5000

#define ID_MI_KANJIRECV		54010	// 受信/送受
#define ID_MI_KANJISEND		54110	// 送信
#define ID_MI_USEONESETTING	54200	// Use one setting
#define ID_MI_SETTING		54201	// 設定
#define ID_MI_AMB_WIDTH_1	54202
#define ID_MI_AMB_WIDTH_2	54203

// メニュー項目名の情報
typedef struct {
	UINT id;						// メニューID
	ULONG_PTR data;   				// データ
	const wchar_t *default_text;	// テキスト
	const char *key;				// キー
} MenuInfo;

// 送受別漢字コード (日本語)
static const MenuInfo MenuNameRecvJ[] = {
	{ ID_MI_KANJIRECV + 0,		IdSJIS,	L"Recv: &Shift_JIS",	"MENU_RECV_SJIS" },
	{ ID_MI_KANJIRECV + 1,		IdEUC,	L"Recv: &EUC-JP",		"MENU_RECV_EUCJP" },
	{ ID_MI_KANJIRECV + 2,		IdJIS,	L"Recv: &JIS",			"MENU_RECV_JIS" },
	{ ID_MI_KANJIRECV + 3,		IdUTF8, L"Recv: &UTF-8",		"MENU_RECV_UTF8" },
	{ 0,  						0,		NULL,					NULL },
	{ ID_MI_KANJISEND + 0,		IdSJIS, L"Send: S&hift_JIS",	"MENU_SEND_SJIS" },
	{ ID_MI_KANJISEND + 1,		IdEUC,	L"Send: EU&C-JP",		"MENU_SEND_EUCJP" },
	{ ID_MI_KANJISEND + 2,		IdJIS,	L"Send: J&IS",			"MENU_SEND_JIS" },
	{ ID_MI_KANJISEND + 3,		IdUTF8, L"Send: U&TF-8",		"MENU_SEND_UTF8" },
};

// 送受同漢字コード (日本語)
static const MenuInfo MenuNameOneJ[] = {
	{ ID_MI_KANJIRECV + 0,		IdSJIS, L"Recv/Send: &Shift_JIS",	"MENU_SJIS" },
	{ ID_MI_KANJIRECV + 1,		IdEUC,	L"Recv/Send: &EUC-JP",		"MENU_EUCJP" },
	{ ID_MI_KANJIRECV + 2,		IdJIS,	L"Recv/Send: &JIS",			"MENU_JIS" },
	{ ID_MI_KANJIRECV + 3,		IdUTF8, L"Recv/Send: &UTF-8",		"MENU_UTF8" },
};

// 送受別漢字コード (韓国語)
static const MenuInfo MenuNameRecvK[] = {
	{ ID_MI_KANJIRECV + 0,		IdKoreanCP949,	L"Recv: &KS5601",		"MENU_RECV_KS5601" },
	{ ID_MI_KANJIRECV + 1,		IdUTF8,			L"Recv: &UTF-8",		"MENU_RECV_UTF8" },
	{ 0,  						0,				NULL,					NULL },
	{ ID_MI_KANJISEND + 0,		IdKoreanCP949,	L"Send: K&S5601",		"MENU_SEND_KS5601" },
	{ ID_MI_KANJISEND + 1,		IdUTF8,			L"Send: U&TF-8",		"MENU_SEND_UTF8" },
};

// 送受同漢字コード (韓国語)
static const MenuInfo MenuNameOneK[] = {
	{ ID_MI_KANJIRECV + 0,		IdKoreanCP949,	L"Recv/Send: &KS5601",	"MENU_KS5601" },
	{ ID_MI_KANJIRECV + 1,		IdUTF8, 		L"Recv/Send: &UTF-8",	"MENU_UTF8" },
};

// OneSetting
static const MenuInfo MenuOneSetting[] = {
	{ 0,  						0,				NULL,					NULL },
	{ ID_MI_USEONESETTING,		0,				L"Use &one setting",  	"MENU_USE_ONE_SETTING" },
};

// Ambiguous Character Width
static const MenuInfo MenuAmbiguousWidth[] = {
	{ ID_MI_AMB_WIDTH_1,		0,	L"Ambiguous Character Width &1",	NULL },
	{ ID_MI_AMB_WIDTH_2,		0,	L"Ambiguous Character Width &2",	NULL },
};

static const MenuInfo MenuSetting[] = {
	{ ID_MI_SETTING,			0,	L"Sett&ing",	"MENU_SETTING" },
};

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef enum {
	MENU_AUTO,
	MENU_JAPANESE,
	MENU_KOREAN,
} LanguageEnum;

typedef struct {
	PTTSet ts;
	PComVar cv;
	HMENU hmEncode;
	PSetupTerminal origSetupTermDlg;
	PReadIniFile origReadIniFile;
	PWriteIniFile origWriteIniFile;
	BOOL UseOneSetting;		// TRUE時送受同一、INIファイルに保存
	BOOL NeedResetCharSet;
	LanguageEnum language;
	const MenuInfo *menu_info_ptr;
	size_t menu_info_count;
	// INIファイルに保存する項目
	struct {
		BOOL charcode_menu;
		LanguageEnum language_ini;
		BOOL ambiguous_menu;
		BOOL setting_menu;
		BOOL one_setting_menu;
	} ini;
} TInstVar;

static TInstVar *pvar;

/* WIN32 allows multiple instances of a DLL */
static TInstVar InstVar;

/**
 *	メニューを作成する
 */
static void CreateMenuInfo(HMENU hMenu, const MenuInfo *infos, size_t count,
						   const wchar_t *UILanguageFile, const char *section)
{
	size_t i;
	for (i = 0; i < count; i++) {
		const MenuInfo *p = &infos[i];

		if (p->id != 0) {
			const char *key = p->key;
			const wchar_t *def_text = p->default_text;

			wchar_t *uimsg;
			GetI18nStrWW(section, key, def_text, UILanguageFile, &uimsg);

			MENUITEMINFOW mi = {0};
			mi.cbSize = sizeof(mi);
			mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
			mi.fType  = MFT_STRING;
			mi.wID = p->id;
			mi.dwTypeData = uimsg;
			mi.dwItemData = p->data;
			InsertMenuItemW(hMenu, ID_HELPMENU, FALSE, &mi);
			free(uimsg);
		}
		else {
			AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
		}
	}
}

/**
 *	hMenuのメニュー項目をすべて削除する
 */
static void DeleteMenus(HMENU hMenu)
{
	// countの個数メニューを削除するとすべて消える
	size_t i;
	for (i = 0; i < 100; i++) {
		// 一番上の項目を削除
		BOOL r = DeleteMenu(hMenu, 0, MF_BYPOSITION);
		if (r == FALSE) {
			// 失敗 = すべての項目を削除完了
			return;
		}
	}
}

static void CheckMenu(HMENU hMenu, const MenuInfo *info_ptr, size_t info_count, BOOL send, int code)
{
	int start = 0;
	int end = 0;
	size_t i;

	for (i = 0; i < info_count; i++) {
		const MenuInfo *p = &info_ptr[i];
		const int id = p->id;
		if ((send == FALSE && (ID_MI_KANJIRECV <= id && id < ID_MI_KANJISEND)) ||
			(send == TRUE && (ID_MI_KANJISEND <= id && id < ID_MI_USEONESETTING))) {
			start = p->id;
			break;
		}
	}

	i = info_count;
	do {
		i--;
		const MenuInfo *p = &info_ptr[i];
		const int id = p->id;
		if ((send == FALSE && (ID_MI_KANJIRECV <= id && id < ID_MI_KANJISEND)) ||
			(send == TRUE && (ID_MI_KANJISEND <= id && id < ID_MI_USEONESETTING))) {
			end = p->id;
			break;
		}
	} while (i > 0);

	int target_id = 0;
	for (i = 0; i < info_count; i++) {
		const MenuInfo *p = &info_ptr[i];
		const int id = p->id;
		if ((send == FALSE && (ID_MI_KANJIRECV <= id && id < ID_MI_KANJISEND)) ||
			(send == TRUE && (ID_MI_KANJISEND <= id && id < ID_MI_USEONESETTING))) {
			if (p->data == code) {
				target_id = id;
				break;
			}
		}
	}

	CheckMenuRadioItem(hMenu, start, end, target_id, MF_BYCOMMAND);
}

/**
 *	受信/送受のチェックを入れる
 */
static int UpdateRecvMenu(int val)
{
	CheckMenu(pvar->hmEncode, pvar->menu_info_ptr, pvar->menu_info_count, FALSE, val);
	return 1;
}

/**
 *	送信のチェックを入れる
 */
static int UpdateSendMenu(int val)
{
	CheckMenu(pvar->hmEncode, pvar->menu_info_ptr, pvar->menu_info_count, TRUE, val);
	return 1;
}

/*
 * 初期化
 */
static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
	pvar->ts = ts;
	pvar->cv = cv;
	pvar->origReadIniFile = NULL;
	pvar->origWriteIniFile = NULL;
	pvar->UseOneSetting = TRUE;
	pvar->NeedResetCharSet = FALSE;
	// iniファイル
	//		iniファイル読み込み部に実際の初期値がある
	pvar->ini.charcode_menu = TRUE;
	pvar->ini.one_setting_menu = TRUE;
	pvar->ini.language_ini = MENU_AUTO;
	pvar->ini.ambiguous_menu = FALSE;
	pvar->ini.setting_menu = TRUE;
}

/*
 * 端末設定ダイアログのフック関数1: UseOneSetting 用
 *
 * 送信と受信の漢字コード設定が同じになるように調整する。
 */
static BOOL PASCAL TTXKanjiMenuSetupTerminal(HWND parent, PTTSet ts) {
	WORD orgRecvCode, orgSendCode;
	BOOL ret;

	orgRecvCode = pvar->ts->KanjiCode;
	orgSendCode = pvar->ts->KanjiCodeSend;

	ret = pvar->origSetupTermDlg(parent, ts);

	if (ret) {
		if (orgRecvCode == pvar->ts->KanjiCode && orgSendCode != pvar->ts->KanjiCodeSend) {
			// 送信コードのみ変更した場合は送信コードに合わせる
			pvar->ts->KanjiCode = pvar->ts->KanjiCodeSend;
		}
		else {
			// それ以外は受信コードに合わせる
			pvar->ts->KanjiCodeSend = pvar->ts->KanjiCode;
		}
	}

	return ret;
}

/*
 * 端末設定ダイアログのフック関数2: 内部状態リセット用
 *
 * 端末設定ダイアログをフックし、設定ダイアログを開かずに TRUE を返す事によって
 * 設定ダイアログ呼出の後処理のみを利用する。
 */
static BOOL PASCAL ResetCharSet(HWND parent, PTTSet ts) {
	(void)parent;
	(void)ts;
	pvar->NeedResetCharSet = FALSE;
	return TRUE;
}

static void PASCAL TTXGetUIHooks(TTXUIHooks *hooks) {
	if (pvar->NeedResetCharSet) {
		// 内部状態リセットの為に呼び出された場合
		*hooks->SetupTerminal = ResetCharSet;
	}
	else if (pvar->UseOneSetting && (pvar->language == MENU_JAPANESE || pvar->language == MENU_KOREAN)) {
		// UseOneSetting が TRUE の時は端末設定ダイアログの後処理の為にフックする
		pvar->origSetupTermDlg = *hooks->SetupTerminal;
		*hooks->SetupTerminal = TTXKanjiMenuSetupTerminal;
	}
}

/*
 * 漢字コード関連の内部状態のリセット
 * TTXからはTera Termの内部状態を直接いじれない為、端末設定ダイアログの後処理を利用する。
 */
static void CallResetCharSet(HWND hWin){
	pvar->NeedResetCharSet = TRUE;
	SendMessage(hWin, WM_COMMAND, MAKELONG(ID_SETUP_TERMINAL, 0), 0);
}

/*
 * 設定の読み込み
 */
static void PASCAL TTXKanjiMenuReadIniFile(const wchar_t *fn, PTTSet ts) {
	wchar_t buff[20];

	/* Call original ReadIniFile */
	pvar->origReadIniFile(fn, ts);

	GetPrivateProfileStringW(IniSectionW, L"CharCodeMenu", L"on", buff, _countof(buff), fn);
	pvar->ini.charcode_menu = _wcsicmp(buff, L"off") == 0 ? FALSE : TRUE;
	GetPrivateProfileStringW(IniSectionW, L"UseOneSetting", L"on", buff, _countof(buff), fn);
	pvar->UseOneSetting = _wcsicmp(buff, L"off") == 0 ? FALSE : TRUE;
	GetPrivateProfileStringW(IniSectionW, L"Language", L"Auto", buff, _countof(buff), fn);
	if (_wcsicmp(buff, L"Japanese") == 0) {
		pvar->ini.language_ini = MENU_JAPANESE;
		pvar->language = pvar->ini.language_ini;
	} else if (_wcsicmp(buff, L"Korean") == 0) {
		pvar->ini.language_ini = MENU_KOREAN;
		pvar->language = pvar->ini.language_ini;
	} else {
		// 自動設定(未設定)
		pvar->ini.language_ini = MENU_AUTO;
	}
	GetPrivateProfileStringW(IniSectionW, L"UseOneSettingMenu", L"on", buff, _countof(buff), fn);
	pvar->ini.one_setting_menu = _wcsicmp(buff, L"off") == 0 ? FALSE : TRUE;
	GetPrivateProfileStringW(IniSectionW, L"AmbiguousCharacterWidthMenu", L"off", buff, _countof(buff), fn);
	pvar->ini.ambiguous_menu = _wcsicmp(buff, L"off") == 0 ? FALSE : TRUE;

	// UseOneSetting が on の場合は、送受信設定が同じになるように調整する
	if (pvar->UseOneSetting) {
		pvar->ts->KanjiCodeSend = pvar->ts->KanjiCode;
	}
}

/*
 * 設定の保存
 */
static void PASCAL TTXKanjiMenuWriteIniFile(const wchar_t *fn, PTTSet ts) {
	/* Call original WriteIniFile */
	pvar->origWriteIniFile(fn, ts);

	WritePrivateProfileStringW(IniSectionW, L"CharCodeMenu",
							   pvar->ini.charcode_menu ? L"on" : L"off", fn);
	WritePrivateProfileStringW(IniSectionW, L"UseOneSetting",
							   pvar->UseOneSetting ? L"on" : L"off", fn);
	WritePrivateProfileStringW(IniSectionW, L"Language",
							   pvar->ini.language_ini == MENU_JAPANESE ? L"Japanese" :
							   pvar->ini.language_ini == MENU_KOREAN ? L"Korean" :
							   L"Auto", fn);
	WritePrivateProfileStringW(IniSectionW, L"UseOneSettingMenu",
							   pvar->ini.one_setting_menu ? L"on" : L"off",  fn);
	WritePrivateProfileStringW(IniSectionW, L"AmbiguousCharacterWidthMenu",
							   pvar->ini.ambiguous_menu ? L"on" : L"off", fn);
}

/*
 * 設定の読み書きをフックする
 */
static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks) {
	pvar->origReadIniFile = *hooks->ReadIniFile;
	*hooks->ReadIniFile = TTXKanjiMenuReadIniFile;
	pvar->origWriteIniFile = *hooks->WriteIniFile;
	*hooks->WriteIniFile = TTXKanjiMenuWriteIniFile;
}

/*
 * メニュー項目の更新
 *
 * 以下の二つについてメニュー項目を更新する。
 * 1. UseOneSetting の設定に基づいて、受信用メニュー項目を受信専用/送受信兼用の切り替えを行う
 * 2. メニュー項目の国際化を行う
 *
 * 通常は 1 で設定した項目名は 2 で上書き更新されるが、lng ファイルが設定されていない、
 * または lng ファイルにメニュー項目名が含まれていない場合への対応として 1 を行っている。
 */
static void UpdateMenu(HMENU menu, BOOL UseOneSetting)
{
	if (pvar->ini.charcode_menu) {
		const MenuInfo *menu_info_ptr;
		size_t menu_info_count;

		switch (pvar->language) {
		case MENU_JAPANESE:
			if (UseOneSetting) {
				menu_info_ptr = MenuNameOneJ;
				menu_info_count = _countof(MenuNameOneJ);
			}
			else {
				menu_info_ptr = MenuNameRecvJ;
				menu_info_count = _countof(MenuNameRecvJ);
			}
			break;
		case MENU_KOREAN:
			if (UseOneSetting) {
				menu_info_ptr = MenuNameOneK;
				menu_info_count = _countof(MenuNameOneK);
			}
			else {
				menu_info_ptr = MenuNameRecvK;
				menu_info_count = _countof(MenuNameRecvK);
			}
			break;
		default:
			assert(FALSE);
			break;
		}

		CreateMenuInfo(menu, menu_info_ptr, menu_info_count,
					   pvar->ts->UILanguageFileW, IniSection);
		if (pvar->ini.one_setting_menu) {
			CreateMenuInfo(menu, MenuOneSetting, _countof(MenuOneSetting),
						   pvar->ts->UILanguageFileW, IniSection);
		}

		pvar->menu_info_ptr = menu_info_ptr;
		pvar->menu_info_count = menu_info_count;
	}

	if (pvar->ini.ambiguous_menu) {
		if (pvar->ini.charcode_menu) {
			AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
		}
		CreateMenuInfo(menu, MenuAmbiguousWidth, _countof(MenuAmbiguousWidth),
					   pvar->ts->UILanguageFileW, IniSection);
		UINT check = pvar->ts->UnicodeAmbiguousWidth == 1 ? ID_MI_AMB_WIDTH_1 : ID_MI_AMB_WIDTH_2;
		CheckMenuRadioItem(menu, ID_MI_AMB_WIDTH_1, ID_MI_AMB_WIDTH_2, check, MF_BYCOMMAND);
		if (pvar->ts->KanjiCode != IdUTF8 && pvar->ts->KanjiCodeSend != IdUTF8) {
			EnableMenuItem(menu, ID_MI_AMB_WIDTH_1, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(menu, ID_MI_AMB_WIDTH_2, MF_BYCOMMAND | MF_GRAYED);
		}
	}
	if (pvar->ini.setting_menu) {
		if (pvar->ini.ambiguous_menu || pvar->ini.charcode_menu) {
			AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
		}
		CreateMenuInfo(menu, MenuSetting, _countof(MenuSetting),
					   pvar->ts->UILanguageFileW, IniSection);
	}
}

/*
 * This function is called when Tera Term creates a new menu.
 */
static void PASCAL TTXModifyMenu(HMENU menu)
{
	MENUITEMINFOW mi = {0};
	wchar_t *uimsg;

	if (pvar->ini.language_ini == MENU_AUTO) {
		switch (GetACP()) {
		case 932:	// CP932 = Shift_JIS
			pvar->language = MENU_JAPANESE;
			break;
		case 949:	// CP949 = Korean
			pvar->language = MENU_KOREAN;
			break;
		default:
			pvar->language = MENU_JAPANESE;
			break;
		}
	}

	pvar->hmEncode = CreateMenu();

	if (pvar->language == MENU_JAPANESE) {
		GetI18nStrWW(IniSection, "MENU_KANJI", L"&KanjiCode", pvar->ts->UILanguageFileW, &uimsg);
	}
	else { // MENU_KOREAN
		GetI18nStrWW(IniSection, "MENU_KANJI_K", L"Coding(&K)", pvar->ts->UILanguageFileW, &uimsg);
	}

	mi.cbSize = sizeof(mi);
	mi.fMask  = MIIM_TYPE | MIIM_SUBMENU;
	mi.fType  = MFT_STRING;
	mi.hSubMenu = pvar->hmEncode;
	mi.dwTypeData = uimsg;
	InsertMenuItemW(menu, ID_HELPMENU, FALSE, &mi);
	free(uimsg);

	UpdateMenu(pvar->hmEncode, pvar->UseOneSetting);
}


/*
 * ラジオメニュー/チェックメニューの状態を設定に合わせて更新する。
 */
static void PASCAL TTXModifyPopupMenu(HMENU menu) {
	// メニューが呼び出されたら、最新の設定に更新する。(2007.5.25 yutaka)
	if (menu == pvar->hmEncode) {
		if (pvar->ini.charcode_menu) {
			UpdateRecvMenu(pvar->ts->KanjiCode);
			if (!pvar->UseOneSetting) {
				UpdateSendMenu(pvar->ts->KanjiCodeSend);
			}
		}
		CheckMenuItem(pvar->hmEncode, ID_MI_USEONESETTING, MF_BYCOMMAND | (pvar->UseOneSetting)?MF_CHECKED:0);
	}
}

static const MenuInfo *SearchMenuItem(const MenuInfo *menu_info_ptr, size_t menu_info_count, UINT cmd_id)
{
	size_t i;
	for (i = 0; i < menu_info_count; i++) {
		if (menu_info_ptr->id == cmd_id) {
			return menu_info_ptr;
		}
		menu_info_ptr++;
	}
	return NULL;
}

static void ArrangeControls(HWND dlg)
{
	const BOOL enabled = (IsDlgButtonChecked(dlg, IDC_CHECK_CHARCODE) == BST_CHECKED) ? TRUE : FALSE;
	EnableWindow(GetDlgItem(dlg, IDC_MENU_CONTENTS), enabled);
	EnableWindow(GetDlgItem(dlg, IDC_LANGUAGE_DROPBOX), enabled);
	EnableWindow(GetDlgItem(dlg, IDC_CHECK_USE_ONE_SETTING), enabled);
	EnableWindow(GetDlgItem(dlg, IDC_CHECK_ONE_SETTING), enabled);
}

static INT_PTR CALLBACK SettingDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_INITDIALOG: {
		static const DlgTextInfo TextInfos[] = {
			{ 0, "MENU_SETTING_TITLE" },
			{ IDC_CHECK_CHARCODE, "MENU_CHARCODE" },
			{ IDC_CHECK_ONE_SETTING, "MENU_USE_ONE_SETTING" },
			{ IDC_MENU_CONTENTS, "MENU_CONTENTS" },
			{ IDC_CHECK_USE_ONE_SETTING, "MENU_USE_ONE_SETTING_MENU" },
			{ IDC_CHECK_AMB_WIDTH, "MENU_AMB_MENU" },
			{ IDOK, "BTN_OK" },
			{ IDCANCEL, "BTN_CANCEL" },
			{ IDHELP, "BTN_HELP" },
		};
		static const I18nTextInfo infos[] = {
			{ "MENU_CONTENTS_AUTO", L"Auto" },
			{ "MENU_CONTENTS_JAPANESE", L"Japanese" },
			{ "MENU_CONTENTS_KOREAN", L"Korean" }
		};

		SetI18nDlgStrsW(dlg, "TTXKanjiMenu", TextInfos, _countof(TextInfos), pvar->ts->UILanguageFileW);
		CheckDlgButton(dlg, IDC_CHECK_CHARCODE,
					   pvar->ini.charcode_menu ? BST_CHECKED : BST_UNCHECKED);
		SetI18nListW("TTXKanjiMenu", dlg, IDC_LANGUAGE_DROPBOX, infos, _countof(infos), pvar->ts->UILanguageFileW, 0);
		SendDlgItemMessageA(dlg, IDC_LANGUAGE_DROPBOX, CB_SETCURSEL,
							pvar->ini.language_ini == MENU_JAPANESE ? 1 :
							pvar->ini.language_ini == MENU_KOREAN ? 2 :
							0 , 0);
		CheckDlgButton(dlg, IDC_CHECK_ONE_SETTING,
					   pvar->UseOneSetting ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(dlg, IDC_CHECK_USE_ONE_SETTING,
					   pvar->ini.one_setting_menu ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(dlg, IDC_CHECK_AMB_WIDTH,
					   pvar->ini.ambiguous_menu ? BST_CHECKED : BST_UNCHECKED);
		ArrangeControls(dlg);

		wchar_t *s;
		char *ver_substr = GetVersionSubstr();
		aswprintf(&s,
				  L"KanjiMenu version %d.%d.%d\r\n%hs",
				  TTXKANJIMENU_VERSION_MAJOR, TTXKANJIMENU_VERSION_MINOR, TTXKANJIMENU_VERSION_PATCH,
				  ver_substr);
		free(ver_substr);
		SetDlgItemTextW(dlg, IDC_VERSION, s);
		free(s);

		CenterWindow(dlg, pvar->cv->HWin);

		return TRUE;
	}

	case WM_COMMAND:
		switch (wp) {
		case IDC_CHECK_CHARCODE | (BN_CLICKED << 16): {
			ArrangeControls(dlg);
			break;
		}
		case IDOK | (BN_CLICKED << 16): {
			int cur = (int)SendDlgItemMessageA(dlg, IDC_LANGUAGE_DROPBOX, CB_GETCURSEL, 0, 0);
			pvar->ini.charcode_menu = (IsDlgButtonChecked(dlg, IDC_CHECK_CHARCODE) == BST_CHECKED) ? TRUE : FALSE;
			pvar->ini.language_ini =
				cur == 1 ? MENU_JAPANESE :
				cur == 2 ? MENU_KOREAN :
				MENU_AUTO;
			pvar->language = pvar->ini.language_ini;
			pvar->UseOneSetting =
				IsDlgButtonChecked(dlg, IDC_CHECK_ONE_SETTING) == BST_CHECKED ? TRUE : FALSE;
			pvar->ini.one_setting_menu =
				IsDlgButtonChecked(dlg, IDC_CHECK_USE_ONE_SETTING) == BST_CHECKED ? TRUE : FALSE;
			pvar->ini.ambiguous_menu =
				IsDlgButtonChecked(dlg, IDC_CHECK_AMB_WIDTH) == BST_CHECKED ? TRUE : FALSE;

			CallResetCharSet(pvar->cv->HWin);

			TTEndDialog(dlg, IDOK);
			break;
		}
		case IDCANCEL | (BN_CLICKED << 16):
			TTEndDialog(dlg, IDCANCEL);
			break;
		case IDHELP | (BN_CLICKED << 16):
			PostMessage(GetParent(dlg), WM_USER_DLGHELP2, HlpTTXPluginKanjiMenu, 0);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	case WM_DESTROY:
		return 0;

	default:
		return FALSE;
	}
}

/*
 * This function is called when Tera Term receives a command message.
 */
static int PASCAL TTXProcessCommand(HWND hWin, WORD cmd) {
	WORD val;

	if ((cmd >= ID_MI_KANJIRECV) && (cmd < ID_MI_USEONESETTING)) {
		/*
		 *	タイトルバーorメニューバーを隠してポップアップメニューで表示しているとき
		 *	この関数がコールされたとき、メニュは DestroyMenu() されている。
		 *	この関数内では GetMenuItemInfoW() が使えない
		 */
		const MenuInfo *menu_info_ptr = SearchMenuItem(pvar->menu_info_ptr, pvar->menu_info_count, cmd);
		if (menu_info_ptr == NULL) {
			// 知らないコマンド?
			assert(FALSE);
			return 0;
		}
		val = (WORD)menu_info_ptr->data;
		if (cmd < ID_MI_KANJISEND) {
			if (pvar->UseOneSetting) {
				// 送受コード
				pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = val;
				pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = val;
			}
			else {
				// 受信コード
				pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = val;
			}
		}
		else {
			// 送信コード
			pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = val;
		}
		CallResetCharSet(hWin);
		return UpdateRecvMenu(pvar->ts->KanjiCode)?1:0;
	}
	else if (cmd == ID_MI_USEONESETTING) {
		if (pvar->UseOneSetting) {
			pvar->UseOneSetting = FALSE;
		}
		else {
			pvar->UseOneSetting = TRUE;

			val = pvar->ts->KanjiCode;
			pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = val;
		}
		DeleteMenus(pvar->hmEncode);
		UpdateMenu(pvar->hmEncode, pvar->UseOneSetting);
		return 1;
	}
	else if (cmd == ID_MI_SETTING) {
		SetDialogFont(pvar->ts->DialogFontNameW, pvar->ts->DialogFontPoint,
					  pvar->ts->DialogFontCharSet, pvar->ts->UILanguageFileW,
					  "Tera Term", "DLG_TAHOMA_FONT");
		TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_DIALOG_SETTING), hWin, SettingDlgProc, 0);
	}
	else if (cmd == ID_MI_AMB_WIDTH_1) {
		pvar->ts->UnicodeAmbiguousWidth = 1;
		CallResetCharSet(hWin);
	}
	else if (cmd == ID_MI_AMB_WIDTH_2) {
		pvar->ts->UnicodeAmbiguousWidth = 2;
		CallResetCharSet(hWin);
	}

	return 0;
}


/*
 * This record contains all the information that the extension forwards to the
 * main Tera Term code. It mostly consists of pointers to the above functions.
 * Any of the function pointers can be replaced with NULL, in which case
 * Tera Term will just ignore that function and assume default behaviour, which
 * means "do nothing".
 */
static TTXExports Exports = {
/* This must contain the size of the structure. See below for its usage. */
	sizeof(TTXExports),

/* This is the load order number of this DLL. */
	ORDER,

/* Now we just list the functions that we've implemented. */
	TTXInit,
	TTXGetUIHooks,
	TTXGetSetupHooks,
	NULL, // TTXOpenTCP,
	NULL, // TTXCloseTCP,
	NULL, // TTXSetWinSize,
	TTXModifyMenu,
	TTXModifyPopupMenu,
	TTXProcessCommand,
	NULL, // TTXEnd,
	NULL, // TTXSetCommandLine
	NULL, // TTXOpenFile
	NULL, // TTXCloseFile
};

BOOL __declspec(dllexport) PASCAL TTXBind(WORD Version, TTXExports *exports) {
	int size = sizeof(Exports) - sizeof(exports->size);
	/* do version checking if necessary */
	/* if (Version!=TTVERSION) return FALSE; */
	(void)Version;

	if (size > exports->size) {
		size = exports->size;
	}
	memcpy((char *)exports + sizeof(exports->size),
	       (char *)&Exports + sizeof(exports->size),
	       size);
	return TRUE;
}

BOOL WINAPI DllMain(HANDLE hInstance,
                    ULONG ul_reason_for_call,
                    LPVOID lpReserved)
{
	(void)lpReserved;
	switch( ul_reason_for_call ) {
		case DLL_THREAD_ATTACH:
			/* do thread initialization */
			break;
		case DLL_THREAD_DETACH:
			/* do thread cleanup */
			break;
		case DLL_PROCESS_ATTACH:
			/* do process initialization */
			hInst = hInstance;
			pvar = &InstVar;
			break;
		case DLL_PROCESS_DETACH:
			/* do process cleanup */
			break;
	}
	return TRUE;
}

/* vim: set ts=4 sw=4 ff=dos : */
