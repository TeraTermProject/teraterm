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

#include "teraterm.h"
#include "tttypes.h"
#include "tttypes_charset.h"
#include "ttplugin.h"
#include "tt_res.h"
#include "i18n.h"
#include "inifile_com.h"

#define IniSection "TTXKanjiMenu"
#define ORDER 5000

#define ID_MI_KANJIRECV		54010	// 受信/送受
#define ID_MI_KANJISEND		54110	// 送信
#define ID_MI_USEONESETTING	54200	// Use one setting
#define ID_MI_JAPANESE		54201	// 日本語メニュー
#define ID_MI_KOREAN		54202	// 韓国語メニュー

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
	{ 0,  						0,		NULL,					NULL },
	{ ID_MI_USEONESETTING,		0,		L"Use &one setting",	"MENU_USE_ONE_SETTING" },
	{ 0,  						0,		NULL,					NULL },
	{ ID_MI_KOREAN,				0,		L"Korean menu",			NULL },
};

// 送受同漢字コード (日本語)
static const MenuInfo MenuNameOneJ[] = {
	{ ID_MI_KANJIRECV + 0,		IdSJIS, L"Recv/Send: &Shift_JIS",	"MENU_SJIS" },
	{ ID_MI_KANJIRECV + 1,		IdEUC,	L"Recv/Send: &EUC-JP",		"MENU_EUCJP" },
	{ ID_MI_KANJIRECV + 2,		IdJIS,	L"Recv/Send: &JIS",			"MENU_JIS" },
	{ ID_MI_KANJIRECV + 3,		IdUTF8, L"Recv/Send: &UTF-8",		"MENU_UTF8" },
	{ 0,						0,		NULL,						NULL },
	{ ID_MI_USEONESETTING,		0,		L"Use &one setting",		"MENU_USE_ONE_SETTING" },
	{ 0,  						0,		NULL,						NULL },
	{ ID_MI_KOREAN,				0,		L"Korean menu",				NULL },
};

// 送受別漢字コード (韓国語)
static const MenuInfo MenuNameRecvK[] = {
	{ ID_MI_KANJIRECV + 0,		IdKoreanCP949,	L"Recv: &KS5601",		"MENU_RECV_KS5601" },
	{ ID_MI_KANJIRECV + 1,		IdUTF8,			L"Recv: &UTF-8",		"MENU_RECV_UTF8" },
	{ 0,  						0,				NULL,					NULL },
	{ ID_MI_KANJISEND + 0,		IdKoreanCP949,	L"Send: K&S5601",		"MENU_SEND_KS5601" },
	{ ID_MI_KANJISEND + 1,		IdUTF8,			L"Send: U&TF-8",		"MENU_SEND_UTF8" },
	{ 0,  						0,				NULL,					NULL },
	{ ID_MI_USEONESETTING,		0,				L"Use &one setting",	"MENU_USE_ONE_SETTING" },
	{ 0,  						0,				NULL,					NULL },
	{ ID_MI_JAPANESE,			0,				L"Japanese menu",		NULL },
};

// 送受同漢字コード (韓国語)
static const MenuInfo MenuNameOneK[] = {
	{ ID_MI_KANJIRECV + 0,		IdKoreanCP949,	L"Recv/Send: &KS5601",	"MENU_KS5601" },
	{ ID_MI_KANJIRECV + 1,		IdUTF8, 		L"Recv/Send: &UTF-8",	"MENU_UTF8" },
	{ 0,  						0,				NULL,					NULL },
	{ ID_MI_USEONESETTING,		0,				L"Use &one setting",  	"MENU_USE_ONE_SETTING" },
	{ 0,  						0,				NULL,					NULL },
	{ ID_MI_JAPANESE,			0,				L"Japanese menu",		NULL },
};

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
	PTTSet ts;
	PComVar cv;
	HMENU hmEncode;
	PSetupTerminal origSetupTermDlg;
	PReadIniFile origReadIniFile;
	PWriteIniFile origWriteIniFile;
	BOOL UseOneSetting;
	BOOL NeedResetCharSet;
	enum {
		MENU_JAPANESE,
		MENU_KOREAN,
	} language;
	const MenuInfo *menu_info_ptr;
	size_t menu_info_count;
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
	char buff[20];

	/* Call original ReadIniFile */
	pvar->origReadIniFile(fn, ts);

	GetPrivateProfileStringAFileW(IniSection, "UseOneSetting", "on", buff, sizeof(buff), fn);
	if (_stricmp(buff, "off") == 0) {
		pvar->UseOneSetting = FALSE;
	}
	else {
		pvar->UseOneSetting = TRUE;
		// UseOneSetting が on の場合は、送受信設定が同じになるように調整する
		switch (pvar->language){
		case MENU_JAPANESE:
		case MENU_KOREAN:
			pvar->ts->KanjiCodeSend = pvar->ts->KanjiCode;
			break;
		}
	}
	return;
}

/*
 * 設定の保存
 */
static void PASCAL TTXKanjiMenuWriteIniFile(const wchar_t *fn, PTTSet ts) {
	/* Call original WriteIniFile */
	pvar->origWriteIniFile(fn, ts);

	WritePrivateProfileStringAFileW(IniSection, "UseOneSetting", pvar->UseOneSetting?"on":"off", fn);

	return;
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
		return;
	}

	pvar->menu_info_ptr = menu_info_ptr;
	pvar->menu_info_count = menu_info_count;

	CreateMenuInfo(menu, menu_info_ptr, menu_info_count, pvar->ts->UILanguageFileW, IniSection);
}

/*
 * This function is called when Tera Term creates a new menu.
 */
static void PASCAL TTXModifyMenu(HMENU menu)
{
	MENUITEMINFOW mi = {0};
	wchar_t *uimsg;
	// 言語が日本語または韓国語の時のみメニューに追加する
	if (pvar->language != MENU_JAPANESE && pvar->language != MENU_KOREAN) {
		return;
	}

	pvar->hmEncode = CreateMenu();

	if (pvar->language == MENU_JAPANESE) {
		GetI18nStrWW(IniSection, "MENU_KANJI", L"&KanjiCode", pvar->ts->UILanguageFileW, &uimsg);
	}
	else { // MENU_KOREAN
		GetI18nStrWW(IniSection, "MENU_KANJI_K", L"Coding(&K)", pvar->ts->UILanguageFileW, &uimsg);
	}

	// TODO
	//		Windows 9x では ANSI 版APIを使うようにする
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
		(void)menu;
		UpdateRecvMenu(pvar->ts->KanjiCode);
		if (!pvar->UseOneSetting) {
			UpdateSendMenu(pvar->ts->KanjiCodeSend);
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
	else if (cmd == ID_MI_KOREAN) {
		DeleteMenus(pvar->hmEncode);
		pvar->language = MENU_KOREAN;
		UpdateMenu(pvar->hmEncode, pvar->UseOneSetting);
	}
	else if (cmd == ID_MI_JAPANESE) {
		DeleteMenus(pvar->hmEncode);
		pvar->language = MENU_JAPANESE;
		UpdateMenu(pvar->hmEncode, pvar->UseOneSetting);
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
			if (GetACP() == 949) {
				// CP949 = Korean
				pvar->language = MENU_KOREAN;
			}
			else {
				pvar->language = MENU_JAPANESE;
			}
			break;
		case DLL_PROCESS_DETACH:
			/* do process cleanup */
			break;
	}
	return TRUE;
}

/* vim: set ts=4 sw=4 ff=dos : */
