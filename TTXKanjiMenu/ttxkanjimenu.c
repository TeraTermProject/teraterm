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

#define ID_MI_KANJIRECV		54010	// ��M/����
#define ID_MI_KANJISEND		54110	// ���M
#define ID_MI_USEONESETTING	54200	// Use one setting
#define ID_MI_SETTING		54201	// �ݒ�
#define ID_MI_AMB_WIDTH_1	54202
#define ID_MI_AMB_WIDTH_2	54203

// ���j���[���ږ��̏��
typedef struct {
	UINT id;						// ���j���[ID
	ULONG_PTR data;   				// �f�[�^
	const wchar_t *default_text;	// �e�L�X�g
	const char *key;				// �L�[
} MenuInfo;

// ����ʊ����R�[�h (���{��)
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

// ���󓯊����R�[�h (���{��)
static const MenuInfo MenuNameOneJ[] = {
	{ ID_MI_KANJIRECV + 0,		IdSJIS, L"Recv/Send: &Shift_JIS",	"MENU_SJIS" },
	{ ID_MI_KANJIRECV + 1,		IdEUC,	L"Recv/Send: &EUC-JP",		"MENU_EUCJP" },
	{ ID_MI_KANJIRECV + 2,		IdJIS,	L"Recv/Send: &JIS",			"MENU_JIS" },
	{ ID_MI_KANJIRECV + 3,		IdUTF8, L"Recv/Send: &UTF-8",		"MENU_UTF8" },
};

// ����ʊ����R�[�h (�؍���)
static const MenuInfo MenuNameRecvK[] = {
	{ ID_MI_KANJIRECV + 0,		IdKoreanCP949,	L"Recv: &KS5601",		"MENU_RECV_KS5601" },
	{ ID_MI_KANJIRECV + 1,		IdUTF8,			L"Recv: &UTF-8",		"MENU_RECV_UTF8" },
	{ 0,  						0,				NULL,					NULL },
	{ ID_MI_KANJISEND + 0,		IdKoreanCP949,	L"Send: K&S5601",		"MENU_SEND_KS5601" },
	{ ID_MI_KANJISEND + 1,		IdUTF8,			L"Send: U&TF-8",		"MENU_SEND_UTF8" },
};

// ���󓯊����R�[�h (�؍���)
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
	BOOL UseOneSetting;		// TRUE�����󓯈�AINI�t�@�C���ɕۑ�
	BOOL NeedResetCharSet;
	LanguageEnum language;
	const MenuInfo *menu_info_ptr;
	size_t menu_info_count;
	// INI�t�@�C���ɕۑ����鍀��
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
 *	���j���[���쐬����
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
 *	hMenu�̃��j���[���ڂ����ׂč폜����
 */
static void DeleteMenus(HMENU hMenu)
{
	// count�̌����j���[���폜����Ƃ��ׂď�����
	size_t i;
	for (i = 0; i < 100; i++) {
		// ��ԏ�̍��ڂ��폜
		BOOL r = DeleteMenu(hMenu, 0, MF_BYPOSITION);
		if (r == FALSE) {
			// ���s = ���ׂĂ̍��ڂ��폜����
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
 *	��M/����̃`�F�b�N������
 */
static int UpdateRecvMenu(int val)
{
	CheckMenu(pvar->hmEncode, pvar->menu_info_ptr, pvar->menu_info_count, FALSE, val);
	return 1;
}

/**
 *	���M�̃`�F�b�N������
 */
static int UpdateSendMenu(int val)
{
	CheckMenu(pvar->hmEncode, pvar->menu_info_ptr, pvar->menu_info_count, TRUE, val);
	return 1;
}

/*
 * ������
 */
static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
	pvar->ts = ts;
	pvar->cv = cv;
	pvar->origReadIniFile = NULL;
	pvar->origWriteIniFile = NULL;
	pvar->UseOneSetting = TRUE;
	pvar->NeedResetCharSet = FALSE;
	// ini�t�@�C��
	//		ini�t�@�C���ǂݍ��ݕ��Ɏ��ۂ̏����l������
	pvar->ini.charcode_menu = TRUE;
	pvar->ini.one_setting_menu = TRUE;
	pvar->ini.language_ini = MENU_AUTO;
	pvar->ini.ambiguous_menu = FALSE;
	pvar->ini.setting_menu = TRUE;
}

/*
 * �[���ݒ�_�C�A���O�̃t�b�N�֐�1: UseOneSetting �p
 *
 * ���M�Ǝ�M�̊����R�[�h�ݒ肪�����ɂȂ�悤�ɒ�������B
 */
static BOOL PASCAL TTXKanjiMenuSetupTerminal(HWND parent, PTTSet ts) {
	WORD orgRecvCode, orgSendCode;
	BOOL ret;

	orgRecvCode = pvar->ts->KanjiCode;
	orgSendCode = pvar->ts->KanjiCodeSend;

	ret = pvar->origSetupTermDlg(parent, ts);

	if (ret) {
		if (orgRecvCode == pvar->ts->KanjiCode && orgSendCode != pvar->ts->KanjiCodeSend) {
			// ���M�R�[�h�̂ݕύX�����ꍇ�͑��M�R�[�h�ɍ��킹��
			pvar->ts->KanjiCode = pvar->ts->KanjiCodeSend;
		}
		else {
			// ����ȊO�͎�M�R�[�h�ɍ��킹��
			pvar->ts->KanjiCodeSend = pvar->ts->KanjiCode;
		}
	}

	return ret;
}

/*
 * �[���ݒ�_�C�A���O�̃t�b�N�֐�2: ������ԃ��Z�b�g�p
 *
 * �[���ݒ�_�C�A���O���t�b�N���A�ݒ�_�C�A���O���J������ TRUE ��Ԃ����ɂ����
 * �ݒ�_�C�A���O�ďo�̌㏈���݂̂𗘗p����B
 */
static BOOL PASCAL ResetCharSet(HWND parent, PTTSet ts) {
	(void)parent;
	(void)ts;
	pvar->NeedResetCharSet = FALSE;
	return TRUE;
}

static void PASCAL TTXGetUIHooks(TTXUIHooks *hooks) {
	if (pvar->NeedResetCharSet) {
		// ������ԃ��Z�b�g�ׂ̈ɌĂяo���ꂽ�ꍇ
		*hooks->SetupTerminal = ResetCharSet;
	}
	else if (pvar->UseOneSetting && (pvar->language == MENU_JAPANESE || pvar->language == MENU_KOREAN)) {
		// UseOneSetting �� TRUE �̎��͒[���ݒ�_�C�A���O�̌㏈���ׂ̈Ƀt�b�N����
		pvar->origSetupTermDlg = *hooks->SetupTerminal;
		*hooks->SetupTerminal = TTXKanjiMenuSetupTerminal;
	}
}

/*
 * �����R�[�h�֘A�̓�����Ԃ̃��Z�b�g
 * TTX�����Tera Term�̓�����Ԃ𒼐ڂ�����Ȃ��ׁA�[���ݒ�_�C�A���O�̌㏈���𗘗p����B
 */
static void CallResetCharSet(HWND hWin){
	pvar->NeedResetCharSet = TRUE;
	SendMessage(hWin, WM_COMMAND, MAKELONG(ID_SETUP_TERMINAL, 0), 0);
}

/*
 * �ݒ�̓ǂݍ���
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
		// �����ݒ�(���ݒ�)
		pvar->ini.language_ini = MENU_AUTO;
	}
	GetPrivateProfileStringW(IniSectionW, L"UseOneSettingMenu", L"on", buff, _countof(buff), fn);
	pvar->ini.one_setting_menu = _wcsicmp(buff, L"off") == 0 ? FALSE : TRUE;
	GetPrivateProfileStringW(IniSectionW, L"AmbiguousCharacterWidthMenu", L"off", buff, _countof(buff), fn);
	pvar->ini.ambiguous_menu = _wcsicmp(buff, L"off") == 0 ? FALSE : TRUE;

	// UseOneSetting �� on �̏ꍇ�́A����M�ݒ肪�����ɂȂ�悤�ɒ�������
	if (pvar->UseOneSetting) {
		pvar->ts->KanjiCodeSend = pvar->ts->KanjiCode;
	}
}

/*
 * �ݒ�̕ۑ�
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
 * �ݒ�̓ǂݏ������t�b�N����
 */
static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks) {
	pvar->origReadIniFile = *hooks->ReadIniFile;
	*hooks->ReadIniFile = TTXKanjiMenuReadIniFile;
	pvar->origWriteIniFile = *hooks->WriteIniFile;
	*hooks->WriteIniFile = TTXKanjiMenuWriteIniFile;
}

/*
 * ���j���[���ڂ̍X�V
 *
 * �ȉ��̓�ɂ��ă��j���[���ڂ��X�V����B
 * 1. UseOneSetting �̐ݒ�Ɋ�Â��āA��M�p���j���[���ڂ���M��p/����M���p�̐؂�ւ����s��
 * 2. ���j���[���ڂ̍��ۉ����s��
 *
 * �ʏ�� 1 �Őݒ肵�����ږ��� 2 �ŏ㏑���X�V����邪�Alng �t�@�C�����ݒ肳��Ă��Ȃ��A
 * �܂��� lng �t�@�C���Ƀ��j���[���ږ����܂܂�Ă��Ȃ��ꍇ�ւ̑Ή��Ƃ��� 1 ���s���Ă���B
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

	// TODO
	//		Windows 9x �ł� ANSI ��API���g���悤�ɂ���
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
 * ���W�I���j���[/�`�F�b�N���j���[�̏�Ԃ�ݒ�ɍ��킹�čX�V����B
 */
static void PASCAL TTXModifyPopupMenu(HMENU menu) {
	// ���j���[���Ăяo���ꂽ��A�ŐV�̐ݒ�ɍX�V����B(2007.5.25 yutaka)
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

		SetI18nDlgStrsW(dlg, "TTXKanjiMenu", TextInfos, _countof(TextInfos), pvar->ts->UILanguageFileW);
		CheckDlgButton(dlg, IDC_CHECK_CHARCODE,
					   pvar->ini.charcode_menu ? BST_CHECKED : BST_UNCHECKED);
		SendDlgItemMessageW(dlg, IDC_LANGUAGE_DROPBOX, CB_ADDSTRING, 0, (LPARAM)L"Auto");
		SendDlgItemMessageW(dlg, IDC_LANGUAGE_DROPBOX, CB_ADDSTRING, 0, (LPARAM)L"Japanese");
		SendDlgItemMessageW(dlg, IDC_LANGUAGE_DROPBOX, CB_ADDSTRING, 0, (LPARAM)L"Korean");
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
				  L"KanjiMenu version %d.%d\r\n%hs",
				  TTXKANJIMENU_VERSION_MAJOR, TTXKANJIMENU_VERSION_MINOR,
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
		 *	�^�C�g���o�[or���j���[�o�[���B���ă|�b�v�A�b�v���j���[�ŕ\�����Ă���Ƃ�
		 *	���̊֐����R�[�����ꂽ�Ƃ��A���j���� DestroyMenu() ����Ă���B
		 *	���̊֐����ł� GetMenuItemInfoW() ���g���Ȃ�
		 */
		const MenuInfo *menu_info_ptr = SearchMenuItem(pvar->menu_info_ptr, pvar->menu_info_count, cmd);
		if (menu_info_ptr == NULL) {
			// �m��Ȃ��R�}���h?
			assert(FALSE);
			return 0;
		}
		val = (WORD)menu_info_ptr->data;
		if (cmd < ID_MI_KANJISEND) {
			if (pvar->UseOneSetting) {
				// ����R�[�h
				pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = val;
				pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = val;
			}
			else {
				// ��M�R�[�h
				pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = val;
			}
		}
		else {
			// ���M�R�[�h
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
