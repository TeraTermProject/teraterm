/*
 * TTX KanjiMenu Plugin
 *    Copyright (C) 2007 Sunao HARA (naoh@nagoya-u.jp)
 *    Copyright (C) 2007-2009 TeraTerm Project.
 */

//// ORIGINAL SOURCE CODE: ttxtest.c

/* Tera Term extension mechanism
   Robert O'Callahan (roc+tt@cs.cmu.edu)
   
   Tera Term by Takashi Teranishi (teranishi@rikaxp.riken.go.jp)
*/

#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "i18n.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compat_w95.h"

#define IniSection "TTXKanjiMenu"
#define ORDER 5000

#define UpdateRecvMenu(val)	\
	CheckMenuRadioItem(pvar->hmEncode, \
	                   ID_MI_KANJIRECV + IdSJIS, \
	                   ID_MI_KANJIRECV + IdUTF8m, \
	                   ID_MI_KANJIRECV + (val), \
	                   MF_BYCOMMAND)
#define UpdateSendMenu(val)	\
	CheckMenuRadioItem(pvar->hmEncode, \
	                   ID_MI_KANJISEND + IdSJIS, \
	                   ID_MI_KANJISEND + IdUTF8, \
	                   ID_MI_KANJISEND + (val), \
	                   MF_BYCOMMAND)

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
	PTTSet ts;
	PComVar cv;
	HMENU hmEncode;
	PSetupTerminal origSetupTermDlg;
	PReadIniFile origReadIniFile;
	PWriteIniFile origWriteIniFile;
	BOOL UseOneSetting;
} TInstVar;

static TInstVar FAR * pvar;

/* WIN32 allows multiple instances of a DLL */
static TInstVar InstVar;

/*
 * This function is called when Tera Term starts up.
 */
static void PASCAL FAR TTXInit(PTTSet ts, PComVar cv) {
	pvar->ts = ts;
	pvar->cv = cv;
	pvar->origReadIniFile = NULL;
	pvar->origWriteIniFile = NULL;
	pvar->UseOneSetting = TRUE;
}

static BOOL FAR PASCAL TTXKanjiMenuSetupTerminal(HWND parent, PTTSet ts) {
	WORD orgRecvCode, orgSendCode;
	BOOL ret;

	orgRecvCode = pvar->ts->KanjiCode;
	orgSendCode = pvar->ts->KanjiCodeSend;

	ret = pvar->origSetupTermDlg(parent, ts);

	if (ret) {
		if (orgRecvCode == pvar->ts->KanjiCode && orgSendCode != pvar->ts->KanjiCodeSend) {
			// 送信コードのみ変更した場合は送信コードに合わせる
			// ただし、送信:UTF-8 && 受信:UTF-8m の場合は対象外
			if (pvar->ts->KanjiCodeSend != IdUTF8 || pvar->ts->KanjiCode != IdUTF8m) {
				pvar->ts->KanjiCode = pvar->ts->KanjiCodeSend;
			}
		}
		else {
			// それ以外は受信コードに合わせる
			if (pvar->ts->KanjiCode == IdUTF8m) {
				pvar->ts->KanjiCodeSend = IdUTF8;
			}
			else {
				pvar->ts->KanjiCodeSend = pvar->ts->KanjiCode;
			}
		}
	}

	return ret;
}

static void PASCAL FAR TTXGetUIHooks(TTXUIHooks FAR * hooks) {
	if (pvar->UseOneSetting && pvar->ts->Language == IdJapanese) {
		pvar->origSetupTermDlg = *hooks->SetupTerminal;
		*hooks->SetupTerminal = TTXKanjiMenuSetupTerminal;
	}
}

static void PASCAL FAR TTXKanjiMenuReadIniFile(PCHAR fn, PTTSet ts) {
	char buff[20];

	/* Call original ReadIniFile */
	pvar->origReadIniFile(fn, ts);

	GetPrivateProfileString(IniSection, "UseOneSetting", "on", buff, sizeof(buff), fn);
	if (_stricmp(buff, "off") == 0) {
		pvar->UseOneSetting = FALSE;
	}
	else {
		pvar->UseOneSetting = TRUE;
		if (pvar->ts->Language == IdJapanese) {
			if (pvar->ts->KanjiCode == IdUTF8m) {
				pvar->ts->KanjiCodeSend = IdUTF8;
			}
			else {
				pvar->ts->KanjiCodeSend = pvar->ts->KanjiCode;
			}
		}
	}
	return;
}

static void PASCAL FAR TTXKanjiMenuWriteIniFile(PCHAR fn, PTTSet ts) {
	/* Call original WriteIniFile */
	pvar->origWriteIniFile(fn, ts);

	WritePrivateProfileString(IniSection, "UseOneSetting", pvar->UseOneSetting?"on":"off", fn);

	return;
}

static void PASCAL FAR TTXGetSetupHooks(TTXSetupHooks FAR *hooks) {
	pvar->origReadIniFile = *hooks->ReadIniFile;
	*hooks->ReadIniFile = TTXKanjiMenuReadIniFile;
	pvar->origWriteIniFile = *hooks->WriteIniFile;
	*hooks->WriteIniFile = TTXKanjiMenuWriteIniFile;
}

// #define ID_MI_KANJIMASK 0xFF00
#define ID_MI_KANJIRECV 54009
#define ID_MI_KANJISEND 54109
#define ID_MI_USEONESETTING 54200

static void PASCAL FAR InsertSendKcodeMenu(HMENU menu) {
	UINT flag = MF_BYPOSITION | MF_STRING | MF_CHECKED;

	InsertMenu(menu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

	GetI18nStr(IniSection, "MENU_SEND_SJIS", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
	           "Send: S&hift_JIS", pvar->ts->UILanguageFile);
	InsertMenu(menu, 6, flag, ID_MI_KANJISEND+IdSJIS,  pvar->ts->UIMsg);
	GetI18nStr(IniSection, "MENU_SEND_EUCJP", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
	           "Send: EU&C-JP", pvar->ts->UILanguageFile);
	InsertMenu(menu, 7, flag, ID_MI_KANJISEND+IdEUC,   pvar->ts->UIMsg);
	GetI18nStr(IniSection, "MENU_SEND_JIS", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
	           "Send: J&IS", pvar->ts->UILanguageFile);
	InsertMenu(menu, 8, flag, ID_MI_KANJISEND+IdJIS,   pvar->ts->UIMsg);
	GetI18nStr(IniSection, "MENU_SEND_UTF8", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
	           "Send: U&TF-8", pvar->ts->UILanguageFile);
	InsertMenu(menu, 9, flag, ID_MI_KANJISEND+IdUTF8,  pvar->ts->UIMsg);
}

static void PASCAL FAR DeleteSendKcodeMenu(HMENU menu) {
	DeleteMenu(menu, 5, MF_BYPOSITION);
	DeleteMenu(menu, 5, MF_BYPOSITION);
	DeleteMenu(menu, 5, MF_BYPOSITION);
	DeleteMenu(menu, 5, MF_BYPOSITION);
	DeleteMenu(menu, 5, MF_BYPOSITION);
}

static void PASCAL FAR UpdateRecvMenuCaption(HMENU menu, BOOL UseOneSetting) {
	if (UseOneSetting) {
		GetI18nStr(IniSection, "MENU_SJIS", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv/Send: &Shift_JIS", pvar->ts->UILanguageFile);
		ModifyMenu(menu, ID_MI_KANJIRECV+IdSJIS,  MF_BYCOMMAND, ID_MI_KANJIRECV+IdSJIS,  pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_EUCJP", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv/Send: &EUC-JP", pvar->ts->UILanguageFile);
		ModifyMenu(menu, ID_MI_KANJIRECV+IdEUC,   MF_BYCOMMAND, ID_MI_KANJIRECV+IdEUC,   pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_JIS", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv/Send: &JIS", pvar->ts->UILanguageFile);
		ModifyMenu(menu, ID_MI_KANJIRECV+IdJIS,   MF_BYCOMMAND, ID_MI_KANJIRECV+IdJIS,   pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_UTF8", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv/Send: &UTF-8", pvar->ts->UILanguageFile);
		ModifyMenu(menu, ID_MI_KANJIRECV+IdUTF8,  MF_BYCOMMAND, ID_MI_KANJIRECV+IdUTF8,  pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_UTF8m", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: UTF-8&m/Send: UTF-8", pvar->ts->UILanguageFile);
		ModifyMenu(menu, ID_MI_KANJIRECV+IdUTF8m, MF_BYCOMMAND, ID_MI_KANJIRECV+IdUTF8m, pvar->ts->UIMsg);
	}
	else {
		GetI18nStr(IniSection, "MENU_RECV_SJIS", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: &Shift_JIS", pvar->ts->UILanguageFile);
		ModifyMenu(menu, ID_MI_KANJIRECV+IdSJIS,  MF_BYCOMMAND, ID_MI_KANJIRECV+IdSJIS,  pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_RECV_EUCJP", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: &EUC-JP", pvar->ts->UILanguageFile);
		ModifyMenu(menu, ID_MI_KANJIRECV+IdEUC,   MF_BYCOMMAND, ID_MI_KANJIRECV+IdEUC,   pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_RECV_JIS", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: &JIS", pvar->ts->UILanguageFile);
		ModifyMenu(menu, ID_MI_KANJIRECV+IdJIS,   MF_BYCOMMAND, ID_MI_KANJIRECV+IdJIS,   pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_RECV_UTF8", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: &UTF-8", pvar->ts->UILanguageFile);
		ModifyMenu(menu, ID_MI_KANJIRECV+IdUTF8,  MF_BYCOMMAND, ID_MI_KANJIRECV+IdUTF8,  pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_RECV_UTF8m", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: UTF-8&m", pvar->ts->UILanguageFile);
		ModifyMenu(menu, ID_MI_KANJIRECV+IdUTF8m, MF_BYCOMMAND, ID_MI_KANJIRECV+IdUTF8m, pvar->ts->UIMsg);
	}
}

/*
 * This function is called when Tera Term creates a new menu.
 */
static void PASCAL FAR TTXModifyMenu(HMENU menu) {
	UINT flag = MF_ENABLED;

	// 言語が日本語のときのみメニューに追加されるようにした。 (2007.7.14 maya)
	if (pvar->ts->Language != IdJapanese) {
		return;
	}

	{
		OSVERSIONINFO osvi;
		MENUITEMINFO mi;

		pvar->hmEncode = CreateMenu();

		// Windows 95 でメニューが表示されないのでバージョンチェックを入れる (2009.2.18 maya)
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);
		if (osvi.dwMajorVersion >= 5) {
			memset(&mi, 0, sizeof(MENUITEMINFO));
			mi.cbSize = sizeof(MENUITEMINFO);
		}
		else {
			memset(&mi, 0, sizeof(MENUITEMINFO)-sizeof(HBITMAP));
			mi.cbSize = sizeof(MENUITEMINFO)-sizeof(HBITMAP);
		}
		mi.fMask  = MIIM_TYPE | MIIM_SUBMENU;
		mi.fType  = MFT_STRING;
		mi.hSubMenu = pvar->hmEncode;
		GetI18nStr(IniSection, "MENU_KANJI", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "&KanjiCode", pvar->ts->UILanguageFile);
		mi.dwTypeData = pvar->ts->UIMsg;
		InsertMenuItem(menu, ID_HELPMENU, FALSE, &mi);

		flag = MF_STRING|MF_CHECKED;
		GetI18nStr(IniSection, "MENU_RECV_SJIS", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: &Shift_JIS", pvar->ts->UILanguageFile);
		AppendMenu(pvar->hmEncode, flag, ID_MI_KANJIRECV+IdSJIS,  pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_RECV_EUCJP", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: &EUC-JP", pvar->ts->UILanguageFile);
		AppendMenu(pvar->hmEncode, flag, ID_MI_KANJIRECV+IdEUC,   pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_RECV_JIS", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: &JIS", pvar->ts->UILanguageFile);
		AppendMenu(pvar->hmEncode, flag, ID_MI_KANJIRECV+IdJIS,   pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_RECV_UTF8", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: &UTF-8", pvar->ts->UILanguageFile);
		AppendMenu(pvar->hmEncode, flag, ID_MI_KANJIRECV+IdUTF8,  pvar->ts->UIMsg);
		GetI18nStr(IniSection, "MENU_RECV_UTF8m", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Recv: UTF-8&m", pvar->ts->UILanguageFile);
		AppendMenu(pvar->hmEncode, flag, ID_MI_KANJIRECV+IdUTF8m, pvar->ts->UIMsg);

		if (!pvar->UseOneSetting) {
			InsertSendKcodeMenu(pvar->hmEncode);
		}
		else {
			UpdateRecvMenuCaption(pvar->hmEncode, pvar->UseOneSetting);
		}

		AppendMenu(pvar->hmEncode, MF_SEPARATOR, 0, NULL);
		GetI18nStr(IniSection, "MENU_USE_ONE_SETTING", pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		           "Use &one setting", pvar->ts->UILanguageFile);
		AppendMenu(pvar->hmEncode, flag, ID_MI_USEONESETTING ,  pvar->ts->UIMsg);

		UpdateRecvMenu(pvar->ts->KanjiCode);
		if (!pvar->UseOneSetting) {
			UpdateSendMenu(pvar->ts->KanjiCodeSend);
		}

		CheckMenuItem(pvar->hmEncode, ID_MI_USEONESETTING, MF_BYCOMMAND | (pvar->UseOneSetting)?MF_CHECKED:0);
	}
}


/*
 * This function is called when Tera Term pops up a submenu menu.
 */
static void PASCAL FAR TTXModifyPopupMenu(HMENU menu) {
	// メニューが呼び出されたら、最新の設定に更新する。(2007.5.25 yutaka)
	UpdateRecvMenu(pvar->ts->KanjiCode);
	if (!pvar->UseOneSetting) {
		UpdateSendMenu(pvar->ts->KanjiCodeSend);
	}
	CheckMenuItem(pvar->hmEncode, ID_MI_USEONESETTING, MF_BYCOMMAND | (pvar->UseOneSetting)?MF_CHECKED:0);
}


/*
 * This function is called when Tera Term receives a command message.
 */
static int PASCAL FAR TTXProcessCommand(HWND hWin, WORD cmd) {
	WORD val;

	if ((cmd > ID_MI_KANJIRECV) && (cmd <= ID_MI_KANJIRECV+IdUTF8m)) {
		// 範囲チェックを追加 
		// TTProxyのバージョンダイアログを開くと、当該ハンドラが呼ばれ、誤動作していたのを修正。
		// (2007.7.13 yutaka)
		val = cmd - ID_MI_KANJIRECV;
		pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = val;
		if (pvar->UseOneSetting) {
			if (val == IdUTF8m) {
				val = IdUTF8;
			}
			pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = val;
		}
		return UpdateRecvMenu(pvar->ts->KanjiCode)?1:0;
	}
	else if ((cmd > ID_MI_KANJISEND) && (cmd <= ID_MI_KANJISEND+IdUTF8)) {
		val = cmd - ID_MI_KANJISEND;
		pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = val;
		if (pvar->UseOneSetting) {
			pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = val;
			return UpdateRecvMenu(pvar->ts->KanjiCode)?1:0;
		}
		else {
			return UpdateSendMenu(pvar->ts->KanjiCodeSend)?1:0;
		}
	}
	else if (cmd == ID_MI_USEONESETTING) {
		if (pvar->UseOneSetting) {
			pvar->UseOneSetting = FALSE;
			InsertSendKcodeMenu(pvar->hmEncode);
			CheckMenuItem(pvar->hmEncode, ID_MI_USEONESETTING, MF_BYCOMMAND);
		}
		else {
			pvar->UseOneSetting = TRUE;

			if (pvar->ts->KanjiCode == IdUTF8m) {
				val = IdUTF8;
			}
			else {
				val = pvar->ts->KanjiCode;
			}
			pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = val;

			DeleteSendKcodeMenu(pvar->hmEncode);
			CheckMenuItem(pvar->hmEncode, ID_MI_USEONESETTING, MF_BYCOMMAND | MF_CHECKED);
		}
		UpdateRecvMenuCaption(pvar->hmEncode, pvar->UseOneSetting);
		return 1;
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
	NULL  // TTXSetCommandLine
};

BOOL __declspec(dllexport) PASCAL FAR TTXBind(WORD Version, TTXExports FAR * exports) {
	int size = sizeof(Exports) - sizeof(exports->size);
	/* do version checking if necessary */
	/* if (Version!=TTVERSION) return FALSE; */

	if (size > exports->size) {
		size = exports->size;
	}
	memcpy((char FAR *)exports + sizeof(exports->size),
	       (char FAR *)&Exports + sizeof(exports->size),
	       size);
	return TRUE;
}

BOOL WINAPI DllMain(HANDLE hInstance, 
                    ULONG ul_reason_for_call,
                    LPVOID lpReserved)
{
	switch( ul_reason_for_call ) {
		case DLL_THREAD_ATTACH:
			/* do thread initialization */
			break;
		case DLL_THREAD_DETACH:
			/* do thread cleanup */
			break;
		case DLL_PROCESS_ATTACH:
			/* do process initialization */
			DoCover_IsDebuggerPresent();
			hInst = hInstance;
			pvar = &InstVar;
			break;
		case DLL_PROCESS_DETACH:
			/* do process cleanup */
			break;
	}
	return TRUE;
}
