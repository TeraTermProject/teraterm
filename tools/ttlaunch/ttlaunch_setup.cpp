/*
 * Copyright (C) 2026- TeraTerm Project
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

//
// Tera Term launcher
//	設定ダイアログ
//	エクスプローラーのコンテキストメニューへの登録/削除と、そのための reg ファイルの書き出し
//

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <string>
#include <vector>

#include "win32helper.h"
#include "compat_win.h"
#include "ttcommdlg.h"
#include "ttlib.h"
#include "dlglib.h"
#include "i18n.h"
#include "asprintf.h"
#include "resource.h"
#include "ttlaunch.h"
#include "ttlaunch_setup.h"
#include "ttlaunch_version.h"

//#define	HERE_LABEL	L"Tera Term &Here";
#define	HERE_LABEL	L"&Open in Tera Term";

/**
 *	エクスプローラーのコンテキストメニューのレジストリキー
 *		Cygterm Here (インストーラー, teraterm.iss) と同じ場所
 *		"Open in Tera Term" は <parent>\ttlaunch、
 *		項目を直接登録するときは <parent>\ttlaunch_NN_<id>
 */
static const struct {
	const wchar_t *parent;	// HKEY_CLASSES_ROOT(Software\Classes) 以下の shell キー
	const wchar_t *param;	// フォルダを示すパラメータ
} context_menu_keys[] = {
	{ L"Folder\\shell", L"%L" },
	{ L"Directory\\Background\\shell", L"%V" },
	{ L"LibraryFolder\\Background\\shell", L"%V" },
};
static const wchar_t *context_menu_name = L"ttlaunch";	// キー名, 直接登録する項目は "ttlaunch_" で始まる

/**
 *	コンテキストメニューの種類
 */
typedef enum {
	MENU_TYPE_POPUP,		// "Open in Tera Term" 1 項目, 選択すると ttlaunch のポップアップメニューを表示する
	MENU_TYPE_ITEMS,		// 各 shell をコンテキストメニューの項目として直接登録する (サブメニューなし, 古い Windows でも使える)
	MENU_TYPE_SUBMENU,		// "Open in Tera Term" のサブメニューに各 shell を並べる (Windows 7 以降)
} menu_type_t;

/**
 *	コンテキストメニューの登録状態
 */
struct ContextMenuStatus {
	bool registered;		// HKEY_CLASSES_ROOT に全キーが登録されている (HKCU, HKLM どちらでもよい)
	bool current_user;		// HKEY_CURRENT_USER に登録されている (一部のキーだけでも true)
	bool local_machine;		// HKEY_LOCAL_MACHINE に登録されている (この設定ダイアログでは変更できない)
	menu_type_t type;		// 登録されているメニューの種類 (registered のとき有効)
};

static bool RegKeyExists(HKEY root, const std::wstring &key)
{
	HKEY hKey;
	if (RegOpenKeyExW(root, key.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return false;
	}
	RegCloseKey(hKey);
	return true;
}

static bool RegValueExists(HKEY root, const std::wstring &key, const wchar_t *name)
{
	HKEY hKey;
	if (RegOpenKeyExW(root, key.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return false;
	}
	const LSTATUS r = RegQueryValueExW(hKey, name, NULL, NULL, NULL, NULL);
	RegCloseKey(hKey);
	return r == ERROR_SUCCESS;
}

/**
 *	shell キーの下にある ttlaunch の登録 ("ttlaunch", "ttlaunch_*") を列挙する
 *
 *	@param	root	HKEY_CLASSES_ROOT, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE
 *	@param	parent	root 以下の shell キー
 *	@return	キー名 (parent 以下の名前) の一覧
 */
static std::vector<std::wstring> EnumContextMenuKeys(HKEY root, const std::wstring &parent)
{
	std::vector<std::wstring> keys;
	HKEY hKey;
	if (RegOpenKeyExW(root, parent.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return keys;
	}
	const size_t name_len = wcslen(context_menu_name);
	for (DWORD index = 0;; index++) {
		wchar_t name[256];
		DWORD len = _countof(name);
		if (RegEnumKeyExW(hKey, index, name, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
			break;
		}
		if (_wcsnicmp(name, context_menu_name, name_len) == 0 && (name[name_len] == 0 || name[name_len] == L'_')) {
			keys.push_back(name);
		}
	}
	RegCloseKey(hKey);
	return keys;
}

static ContextMenuStatus GetContextMenuStatus(void)
{
	ContextMenuStatus st = { true, false, false, MENU_TYPE_POPUP };
	for (size_t i = 0; i < _countof(context_menu_keys); i++) {
		const std::wstring parent = context_menu_keys[i].parent;
		if (EnumContextMenuKeys(HKEY_CLASSES_ROOT, parent).empty()) {
			st.registered = false;
		}
		if (!EnumContextMenuKeys(HKEY_CURRENT_USER, L"Software\\Classes\\" + parent).empty()) {
			st.current_user = true;
		}
		if (!EnumContextMenuKeys(HKEY_LOCAL_MACHINE, L"Software\\Classes\\" + parent).empty()) {
			st.local_machine = true;
		}
	}
	if (st.registered) {
		const std::wstring base = std::wstring(context_menu_keys[0].parent) + L"\\" + context_menu_name;
		if (!RegKeyExists(HKEY_CLASSES_ROOT, base)) {
			st.type = MENU_TYPE_ITEMS;
		}
		else if (RegValueExists(HKEY_CLASSES_ROOT, base, L"SubCommands")) {
			st.type = MENU_TYPE_SUBMENU;
		}
		else {
			st.type = MENU_TYPE_POPUP;
		}
	}
	return st;
}

/**
 *	レジストリへの操作 1 件
 *		レジストリへ直接書き込む (ApplyRegEntries()) ことも、
 *		.reg ファイルに書き出す (RegEntriesToText()) こともできる
 */
struct RegEntry {
	std::wstring key;												// HKEY_CURRENT_USER\Software\Classes 以下のキー
	bool delete_key;												// true のときキーをサブキーごと削除する (他のメンバは使用しない)
	std::vector<std::pair<std::wstring, std::wstring> > strings;	// 文字列値 (名前, 値), 名前 "" は既定値
	std::vector<std::pair<std::wstring, DWORD> > dwords;			// DWORD 値 (名前, 値)
};

static RegEntry MakeDeleteEntry(const std::wstring &key)
{
	RegEntry e;
	e.key = key;
	e.delete_key = true;
	return e;
}

static RegEntry MakeKeyEntry(const std::wstring &key)
{
	RegEntry e;
	e.key = key;
	e.delete_key = false;
	return e;
}

static void AddString(RegEntry &e, const wchar_t *name, const std::wstring &value)
{
	e.strings.push_back(std::make_pair(std::wstring(name), value));
}

/**
 *	id をレジストリのキー名に使える文字列にする
 *		英数字以外は '_' に置き換える('\\' はキー名に使えない)
 */
static std::wstring ToKeyName(const std::wstring &id)
{
	std::wstring s;
	for (size_t i = 0; i < id.size(); i++) {
		const wchar_t c = id[i];
		s += iswalnum(c) ? c : L'_';
	}
	return s;
}

/**
 *	shell の項目 1 つ分 (キーと command サブキー) を追加する
 *
 *	@param	mui			true のとき MUIVerb に、false のとき既定値にラベルを書く
 *	@param	icon		アイコン, 無いとき ""
 *	@param	separator	true のとき項目の前に区切り線を入れる (Windows 7 以降のサブメニューのみ)
 */
static void AddItemEntries(std::vector<RegEntry> &entries, const std::wstring &key, bool mui, const std::wstring &label,
						   const std::wstring &icon, bool separator, const std::wstring &command)
{
	RegEntry e = MakeKeyEntry(key);
	AddString(e, mui ? L"MUIVerb" : L"", label);
	if (!icon.empty()) {
		AddString(e, L"Icon", icon);
	}
	if (separator) {
		e.dwords.push_back(std::make_pair(std::wstring(L"CommandFlags"), (DWORD)0x20));	// ECF_SEPARATORBEFORE
	}
	entries.push_back(e);

	RegEntry c = MakeKeyEntry(key + L"\\command");
	AddString(c, L"", command);
	entries.push_back(c);
}

/**
 *	コンテキストメニューを HKEY_CURRENT_USER から削除するレジストリ操作を作成する
 *		今登録されている "ttlaunch", "ttlaunch_*" をすべて削除する
 *		HKEY_LOCAL_MACHINE に登録されたものは削除できない
 */
static std::vector<RegEntry> CreateUnregisterEntries(void)
{
	std::vector<RegEntry> entries;
	for (size_t i = 0; i < _countof(context_menu_keys); i++) {
		const std::wstring parent = context_menu_keys[i].parent;
		std::vector<std::wstring> names = EnumContextMenuKeys(HKEY_CURRENT_USER, L"Software\\Classes\\" + parent);
		bool base_found = false;
		for (size_t j = 0; j < names.size(); j++) {
			if (_wcsicmp(names[j].c_str(), context_menu_name) == 0) {
				base_found = true;
			}
			entries.push_back(MakeDeleteEntry(parent + L"\\" + names[j]));
		}
		if (!base_found) {
			// 登録されていなくても "Open in Tera Term" の削除は含める (reg ファイルを他の環境で使うとき用)
			entries.push_back(MakeDeleteEntry(parent + L"\\" + context_menu_name));
		}
	}
	return entries;
}

/**
 *	コンテキストメニューを HKEY_CURRENT_USER に登録するレジストリ操作を作成する
 *
 *	既存の登録(別の種類の登録など)が残っていると
 *	正しく動作しないため、すべて削除してから書き直す
 *	特にサブメニューでは、親キーに既定値があると項目が表示されない
 *
 *	@param	items	起動方法の一覧 (MENU_TYPE_ITEMS, MENU_TYPE_SUBMENU のとき各項目になる)
 *	@param	type	メニューの種類
 */
static std::vector<RegEntry> CreateRegisterEntries(const std::vector<LaunchItem> &items, menu_type_t type)
{
	const LangInfo &lang = GetLangInfo();

	wchar_t *exe_path;
	hGetModuleFileNameW(NULL, &exe_path);
	const std::wstring exe_quoted = L"\"" + std::wstring(exe_path) + L"\"";
	free(exe_path);

	// 32bit プロセス用の Sysnative はエクスプローラー(64bit)では使えない
	wchar_t windir[MAX_PATH];
	GetWindowsDirectoryW(windir, _countof(windir));
	const std::wstring sysnative = std::wstring(windir) + L"\\Sysnative\\";
	const std::wstring system32 = std::wstring(windir) + L"\\System32\\";

	const std::wstring here_label = HERE_LABEL;
	const std::wstring setup_label = GetLangStr(lang, "MENU_SETUP", L"Open &settings dialog...");

	std::vector<RegEntry> entries = CreateUnregisterEntries();
	for (size_t i = 0; i < _countof(context_menu_keys); i++) {
		const std::wstring base = std::wstring(context_menu_keys[i].parent) + L"\\" + context_menu_name;
		const std::wstring param = std::wstring(L"\"") + context_menu_keys[i].param + L"\"";

		if (type == MENU_TYPE_POPUP) {
			// Cygterm Here (インストーラー, teraterm.iss) と同じ形式
			AddItemEntries(entries, base, false, here_label, exe_quoted, false, exe_quoted + L" -d " + param);
			continue;
		}

		if (type == MENU_TYPE_SUBMENU) {
			// 親 (サブメニュー)
			//	SubCommands が空のとき、shell サブキーの項目がサブメニューになる
			//	既定値があると項目が表示されないため MUIVerb を使う
			RegEntry parent = MakeKeyEntry(base);
			AddString(parent, L"MUIVerb", here_label);
			AddString(parent, L"Icon", exe_quoted);
			AddString(parent, L"SubCommands", L"");
			entries.push_back(parent);
		}

		// 各 shell
		//	MENU_TYPE_SUBMENU: <base>\shell\NN_<id> (キー名のアルファベット順に並ぶため、番号を付けて items の順序を保つ)
		//	MENU_TYPE_ITEMS:   <parent>\ttlaunch_NN_<id> (他のアプリの項目と混ざって並ぶため、ラベルに "Tera Term: " を付ける)
		const bool submenu = type == MENU_TYPE_SUBMENU;
		const std::wstring item_base = submenu ? base + L"\\shell\\" : base + L"_";
		for (size_t j = 0; j < items.size(); j++) {
			const LaunchItem &item = items[j];
			wchar_t prefix[16];
			swprintf(prefix, _countof(prefix), L"%02u_", (unsigned int)(j + 1));
			const std::wstring key = item_base + prefix + ToKeyName(item.id);

			std::wstring icon = item.icon;
			if (!icon.empty()) {
				if (_wcsnicmp(icon.c_str(), sysnative.c_str(), sysnative.size()) == 0) {
					icon = system32 + icon.substr(sysnative.size());
				}
				const size_t len = icon.size();
				if (!(len > 4 && _wcsicmp(icon.c_str() + len - 4, L".ico") == 0)) {
					icon += L",0";	// exe の最初のアイコン
				}
			}
			const std::wstring label = submenu ? item.label : FormatLangStr(lang, "MENU_ITEM_FORMAT", L"Tera Term: %s", item.label);
			const bool separator = submenu && j != 0 && items[j - 1].group != item.group;
			const std::wstring id = item.id.find(L' ') == std::wstring::npos ? item.id : QuoteArg(item.id);
			AddItemEntries(entries, key, submenu, label, icon, separator, exe_quoted + L" -e " + id + L" -d " + param);
		}

		// 設定ダイアログ (ttlaunch -setup)
		//	ポップアップメニューの "Open settings dialog..." に相当する
		{
			const std::wstring label = submenu ? setup_label : FormatLangStr(lang, "MENU_ITEM_FORMAT", L"Tera Term: %s", setup_label);
			AddItemEntries(entries, item_base + L"99_setup", submenu, label, exe_quoted, submenu, exe_quoted + L" -setup");
		}
	}
	return entries;
}

/**
 *	キーをサブキーごと削除する
 *		RegDeleteTree() は Vista 以降のため自前で再帰する
 *		キーが無いときは成功とする
 */
static LSTATUS RegDeleteKeyRecursive(HKEY root, const std::wstring &key)
{
	HKEY hKey;
	LSTATUS r = RegOpenKeyExW(root, key.c_str(), 0, KEY_READ, &hKey);
	if (r == ERROR_FILE_NOT_FOUND) {
		return ERROR_SUCCESS;
	}
	if (r != ERROR_SUCCESS) {
		return r;
	}
	for (;;) {
		// 削除すると index がずれるため、常に先頭のサブキーを取る
		wchar_t name[256];
		DWORD len = _countof(name);
		if (RegEnumKeyExW(hKey, 0, name, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
			break;
		}
		r = RegDeleteKeyRecursive(hKey, name);
		if (r != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return r;
		}
	}
	RegCloseKey(hKey);
	r = RegDeleteKeyW(root, key.c_str());
	return r == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : r;
}

/**
 *	レジストリ操作を HKEY_CURRENT_USER に適用する (ApplyRegEntries() から呼ばれる)
 */
static LSTATUS ApplyRegEntriesSub(const std::vector<RegEntry> &entries)
{
	for (size_t i = 0; i < entries.size(); i++) {
		const RegEntry &e = entries[i];
		const std::wstring key = L"Software\\Classes\\" + e.key;
		LSTATUS r;
		if (e.delete_key) {
			r = RegDeleteKeyRecursive(HKEY_CURRENT_USER, key);
			if (r != ERROR_SUCCESS) {
				return r;
			}
			continue;
		}
		HKEY hKey;
		r = RegCreateKeyExW(HKEY_CURRENT_USER, key.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
		if (r != ERROR_SUCCESS) {
			return r;
		}
		for (size_t j = 0; j < e.strings.size() && r == ERROR_SUCCESS; j++) {
			const std::wstring &name = e.strings[j].first;
			const std::wstring &value = e.strings[j].second;
			r = RegSetValueExW(hKey, name.empty() ? NULL : name.c_str(), 0, REG_SZ, (const BYTE *)value.c_str(),
							   (DWORD)((value.size() + 1) * sizeof(wchar_t)));
		}
		for (size_t j = 0; j < e.dwords.size() && r == ERROR_SUCCESS; j++) {
			const DWORD value = e.dwords[j].second;
			r = RegSetValueExW(hKey, e.dwords[j].first.c_str(), 0, REG_DWORD, (const BYTE *)&value, sizeof(value));
		}
		RegCloseKey(hKey);
		if (r != ERROR_SUCCESS) {
			return r;
		}
	}
	return ERROR_SUCCESS;
}

/**
 *	レジストリ操作を HKEY_CURRENT_USER に適用する
 *
 *	エクスプローラーは関連付けをキャッシュしているため、変更を通知する
 *	通知しないと、デスクトップのコンテキストメニューに反映されないことがある
 *	途中で失敗しても一部は変更されているため、常に通知する
 */
static LSTATUS ApplyRegEntries(const std::vector<RegEntry> &entries)
{
	const LSTATUS r = ApplyRegEntriesSub(entries);
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	return r;
}

/**
 *	.reg ファイルの文字列値用にエスケープする ('\\' と '"')
 */
static std::wstring EscapeReg(const std::wstring &text)
{
	std::wstring s;
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == L'\\' || text[i] == L'"') {
			s += L'\\';
		}
		s += text[i];
	}
	return s;
}

/**
 *	レジストリ操作を .reg ファイルの内容にする
 */
static std::wstring RegEntriesToText(const std::vector<RegEntry> &entries)
{
	std::wstring s = L"Windows Registry Editor Version 5.00\r\n\r\n";
	for (size_t i = 0; i < entries.size(); i++) {
		const RegEntry &e = entries[i];
		const std::wstring key = L"HKEY_CURRENT_USER\\Software\\Classes\\" + e.key;
		if (e.delete_key) {
			s += L"[-" + key + L"]\r\n\r\n";
			continue;
		}
		s += L"[" + key + L"]\r\n";
		for (size_t j = 0; j < e.strings.size(); j++) {
			const std::wstring &name = e.strings[j].first;
			const std::wstring &value = e.strings[j].second;
			s += name.empty() ? L"@" : L"\"" + EscapeReg(name) + L"\"";
			s += L"=\"" + EscapeReg(value) + L"\"\r\n";
		}
		for (size_t j = 0; j < e.dwords.size(); j++) {
			wchar_t buf[16];
			swprintf(buf, _countof(buf), L"%08x", (unsigned int)e.dwords[j].second);
			s += L"\"" + EscapeReg(e.dwords[j].first) + L"\"=dword:" + buf + L"\r\n";
		}
		s += L"\r\n";
	}
	return s;
}

/**
 *	UTF-16LE (BOM 付き) でファイルに書き込む
 */
static bool WriteTextFile(const std::wstring &path, const std::wstring &text)
{
	FILE *fp;
	errno_t e = _wfopen_s(&fp, path.c_str(), L"wb");
	if (e != 0 || fp == NULL) {
		return false;
	}
	static const unsigned char bom[] = { 0xff, 0xfe };
	const bool ok = fwrite(bom, 1, sizeof(bom), fp) == sizeof(bom) &&
					fwrite(text.c_str(), sizeof(wchar_t), text.size(), fp) == text.size();
	fclose(fp);
	return ok;
}

/**
 *	メニュー用の '&' を取り除く ("&&" -> "&", "&x" -> "x")
 */
static std::wstring StripMnemonic(const std::wstring &text)
{
	std::wstring s;
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == L'&' && i + 1 < text.size()) {
			i++;
		}
		s += text[i];
	}
	return s;
}

static void ShowErrorMessage(HWND hWnd, const std::wstring &text, DWORD error)
{
	wchar_t *error_msg;
	hFormatMessageW(error, &error_msg);
	const std::wstring msg = text + L"\n" + error_msg;
	free(error_msg);
	MessageBoxW(hWnd, msg.c_str(), L"ttlaunch", MB_OK | MB_ICONWARNING);
}

/**
 *	設定ダイアログ
 */
struct SetupDialogData {
	const std::vector<LaunchItem> *items;
	LangInfo lang;
};

static menu_type_t GetSelectedMenuType(HWND hDlg)
{
	if (IsDlgButtonChecked(hDlg, IDC_TYPE_SUBMENU) == BST_CHECKED) {
		return MENU_TYPE_SUBMENU;
	}
	if (IsDlgButtonChecked(hDlg, IDC_TYPE_ITEMS) == BST_CHECKED) {
		return MENU_TYPE_ITEMS;
	}
	return MENU_TYPE_POPUP;
}

/**
 *	出力先が reg ファイルか
 */
static bool IsRegFileSelected(HWND hDlg)
{
	return IsDlgButtonChecked(hDlg, IDC_OUT_REGFILE) == BST_CHECKED;
}

/**
 *	登録状態の表示、ボタンの文言と有効/無効を更新する
 */
static void UpdateSetupDialog(HWND hDlg, const LangInfo &lang)
{
	const ContextMenuStatus st = GetContextMenuStatus();
	std::wstring status;
	if (!st.registered) {
		status = GetLangStr(lang, "MSG_STATUS_NOT_REGISTERED", L"Not registered");
	}
	else {
		switch (st.type) {
		case MENU_TYPE_SUBMENU:
			status = GetLangStr(lang, "MSG_STATUS_REGISTERED_SUBMENU", L"Registered (submenu)");
			break;
		case MENU_TYPE_ITEMS:
			status = GetLangStr(lang, "MSG_STATUS_REGISTERED_ITEMS", L"Registered (items)");
			break;
		default:
			status = GetLangStr(lang, "MSG_STATUS_REGISTERED_POPUP", L"Registered (popup menu)");
			break;
		}
		if (st.current_user) {
			status += GetLangStr(lang, "MSG_STATUS_HKCU", L", HKEY_CURRENT_USER");
		}
		if (st.local_machine) {
			status += GetLangStr(lang, "MSG_STATUS_HKLM", L", HKEY_LOCAL_MACHINE");
			// HKEY_LOCAL_MACHINE に登録されたものはここでは削除できないことを 2 行目に表示する
			status += L"\n";
			status += GetLangStr(lang, "MSG_STATUS_HKLM_NOTE", L"The registration in HKEY_LOCAL_MACHINE cannot be changed here");
		}
	}
	SetDlgItemTextW(hDlg, IDC_STATUS, status.c_str());

	// 出力先によってボタンの文言を変える, reg ファイルは登録状態にかかわらず書き出せる
	const bool regfile = IsRegFileSelected(hDlg);
	const std::wstring register_label =
		regfile ? GetLangStr(lang, "DLG_SETUP_SAVE_REGISTER", L"Save (&register)...") : GetLangStr(lang, "DLG_SETUP_REGISTER", L"&Register");
	const std::wstring unregister_label =
		regfile ? GetLangStr(lang, "DLG_SETUP_SAVE_UNREGISTER", L"Save (&unregister)...") : GetLangStr(lang, "DLG_SETUP_UNREGISTER", L"&Unregister");
	SetDlgItemTextW(hDlg, IDC_REGISTER, register_label.c_str());
	SetDlgItemTextW(hDlg, IDC_UNREGISTER, unregister_label.c_str());
	EnableWindow(GetDlgItem(hDlg, IDC_UNREGISTER), regfile || st.current_user);
}

/**
 *	.reg ファイルを保存する
 */
static void SaveRegFile(HWND hDlg, const LangInfo &lang, const wchar_t *default_name, const std::wstring &text)
{
	const std::wstring exe_dir = GetExeDir();
	const std::wstring title = GetLangStr(lang, "MSG_SAVE_REG_TITLE", L"Save .reg file");
	TTOPENFILENAMEW ofn = {};
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = L"Registration Entries (*.reg)\0*.reg\0All Files (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = default_name;
	ofn.lpstrInitialDir = exe_dir.c_str();
	ofn.lpstrTitle = title.c_str();
	ofn.lpstrDefExt = L"reg";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	wchar_t *filename;
	if (!TTGetSaveFileNameW(&ofn, &filename)) {
		return;
	}
	const std::wstring reg_file = filename;
	free(filename);

	if (!WriteTextFile(reg_file, text)) {
		ShowErrorMessage(hDlg, FormatLangStr(lang, "MSG_SAVE_REG_ERROR", L"Failed to write the file.\n%s", reg_file),
						 GetLastError());
		return;
	}
	const std::wstring msg = FormatLangStr(lang, "MSG_SAVE_REG_DONE",
										   L"Saved.\n%s\n\nDouble-click the file to apply it to HKEY_CURRENT_USER.", reg_file);
	MessageBoxW(hDlg, msg.c_str(), L"ttlaunch", MB_OK | MB_ICONINFORMATION);
}

/**
 *	レジストリエディターで登録先のキーを開く
 *		regedit は前回開いていたキー (LastKey) を選択して起動するので、そこへ書き込んでから起動する
 *		regedit の起動には管理者権限の確認 (UAC) が出る
 */
static void OpenRegistryEditor(HWND hDlg, const LangInfo &lang)
{
	// 登録があればそのキー、無ければ登録先の shell キー
	const std::wstring parent = std::wstring(L"Software\\Classes\\") + context_menu_keys[1].parent;	// Directory\Background\shell
	std::wstring key = L"HKEY_CURRENT_USER\\" + parent;
	if (RegKeyExists(HKEY_CURRENT_USER, parent + L"\\" + context_menu_name)) {
		key += std::wstring(L"\\") + context_menu_name;
	}

	HKEY hKey;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit", 0, NULL, 0,
						KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
		RegSetValueExW(hKey, L"LastKey", 0, REG_SZ, (const BYTE *)key.c_str(), (DWORD)((key.size() + 1) * sizeof(wchar_t)));
		RegCloseKey(hKey);
	}

	// /m: 起動済みでも新しいウィンドウを開く (LastKey を反映させるため)
	const HINSTANCE r = ShellExecuteW(hDlg, L"open", L"regedit.exe", L"/m", NULL, SW_SHOWNORMAL);
	if ((INT_PTR)r <= 32 && (INT_PTR)r != SE_ERR_ACCESSDENIED) {
		// SE_ERR_ACCESSDENIED は UAC でキャンセルされたとき
		ShowErrorMessage(hDlg, GetLangStr(lang, "MSG_REGEDIT_ERROR", L"Failed to start the Registry Editor."), GetLastError());
	}
}

static INT_PTR CALLBACK SetupDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo setup_dlg_texts[] = {
		{ 0, "DLG_SETUP_TITLE" },
		{ IDC_STATUS_LABEL, "DLG_SETUP_STATUS" },
		{ IDC_TYPE_LABEL, "DLG_SETUP_TYPE_LABEL" },
		{ IDC_TYPE_POPUP, "DLG_SETUP_TYPE_POPUP" },
		{ IDC_TYPE_ITEMS, "DLG_SETUP_TYPE_ITEMS" },
		{ IDC_TYPE_SUBMENU, "DLG_SETUP_TYPE_SUBMENU" },
		{ IDC_ITEMS_LABEL, "DLG_SETUP_ITEMS" },
		{ IDC_OUT_LABEL, "DLG_SETUP_OUT_LABEL" },
		{ IDC_OUT_REGISTRY, "DLG_SETUP_OUT_REGISTRY" },
		{ IDC_OUT_REGFILE, "DLG_SETUP_OUT_REGFILE" },
		{ IDC_REGEDIT_LABEL, "DLG_SETUP_REGEDIT_LABEL" },
		{ IDC_REGEDIT, "DLG_SETUP_REGEDIT" },
		{ IDCANCEL, "DLG_SETUP_CLOSE" },
	};

	switch (msg) {
	case WM_INITDIALOG: {
		SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
		const SetupDialogData *data = (const SetupDialogData *)lParam;
		const std::vector<LaunchItem> &items = *data->items;

		SetI18nDlgStrsW(hDlg, data->lang.section.c_str(), setup_dlg_texts, _countof(setup_dlg_texts), data->lang.file.c_str());

		// メニューに並ぶ項目の一覧
		HWND hList = GetDlgItem(hDlg, IDC_ITEMS);
		for (size_t i = 0; i < items.size(); i++) {
			SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)StripMnemonic(items[i].label).c_str());
		}

		// メニューの種類: 登録済みならその種類、未登録ならサブメニュー
		//	サブメニューは Windows 7 以降
		//	起動できる項目が無いときはポップアップメニューだけ
		const ContextMenuStatus st = GetContextMenuStatus();
		menu_type_t type = st.registered ? st.type : MENU_TYPE_SUBMENU;
		if (!IsWindows7OrLater()) {
			EnableWindow(GetDlgItem(hDlg, IDC_TYPE_SUBMENU), FALSE);
			if (type == MENU_TYPE_SUBMENU) {
				type = MENU_TYPE_ITEMS;
			}
		}
		if (items.empty()) {
			EnableWindow(GetDlgItem(hDlg, IDC_TYPE_ITEMS), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_TYPE_SUBMENU), FALSE);
			type = MENU_TYPE_POPUP;
		}
		const int type_id = type == MENU_TYPE_SUBMENU ? IDC_TYPE_SUBMENU : type == MENU_TYPE_ITEMS ? IDC_TYPE_ITEMS : IDC_TYPE_POPUP;
		CheckRadioButton(hDlg, IDC_TYPE_POPUP, IDC_TYPE_SUBMENU, type_id);

		// 出力先: レジストリ
		CheckRadioButton(hDlg, IDC_OUT_REGISTRY, IDC_OUT_REGFILE, IDC_OUT_REGISTRY);

		// バージョン
		wchar_t *ver;
		char *ver_substr = GetVersionSubstr();
		aswprintf(&ver,
				  L"ttlaunch version %d.%d.%d\r\n%hs",
				  TTLAUNCH_VERSION_MAJOR, TTLAUNCH_VERSION_MINOR, TTLAUNCH_VERSION_PATCH,
				  ver_substr);
		free(ver_substr);
		SetDlgItemTextW(hDlg, IDC_VERSION, ver);
		free(ver);

		UpdateSetupDialog(hDlg, data->lang);
		return TRUE;
	}
	case WM_COMMAND: {
		const SetupDialogData *data = (const SetupDialogData *)GetWindowLongPtrW(hDlg, DWLP_USER);
		const LangInfo &lang = data->lang;
		switch (LOWORD(wParam)) {
		case IDC_OUT_REGISTRY:
		case IDC_OUT_REGFILE:
			UpdateSetupDialog(hDlg, lang);
			return TRUE;
		case IDC_REGISTER: {
			const std::vector<RegEntry> entries = CreateRegisterEntries(*data->items, GetSelectedMenuType(hDlg));
			if (IsRegFileSelected(hDlg)) {
				SaveRegFile(hDlg, lang, L"ttlaunch_context_menu.reg", RegEntriesToText(entries));
				return TRUE;
			}
			const LSTATUS r = ApplyRegEntries(entries);
			if (r != ERROR_SUCCESS) {
				ShowErrorMessage(hDlg, GetLangStr(lang, "MSG_REGISTER_ERROR", L"Failed to register to the Explorer context menu."), r);
			}
			UpdateSetupDialog(hDlg, lang);
			return TRUE;
		}
		case IDC_UNREGISTER: {
			const std::vector<RegEntry> entries = CreateUnregisterEntries();
			if (IsRegFileSelected(hDlg)) {
				SaveRegFile(hDlg, lang, L"ttlaunch_context_menu_remove.reg", RegEntriesToText(entries));
				return TRUE;
			}
			const LSTATUS r = ApplyRegEntries(entries);
			if (r != ERROR_SUCCESS) {
				ShowErrorMessage(hDlg, GetLangStr(lang, "MSG_UNREGISTER_ERROR", L"Failed to remove from the Explorer context menu."), r);
			}
			UpdateSetupDialog(hDlg, lang);
			return TRUE;
		}
		case IDC_REGEDIT:
			OpenRegistryEditor(hDlg, lang);
			return TRUE;
		case IDOK:
		case IDCANCEL:
			TTEndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	}
	}
	return FALSE;
}

/**
 *	ダイアログフォントを設定する
 *	Tera Term 本体と同じく TERATERM.INI の DlgFont、lng ファイルの DLG_SYSTEM_FONT、
 *	MessageBox() のフォントの順に使用する
 */
static void SetDialogFontFromIni(void)
{
	wchar_t *setup_fname = GetDefaultSetupFNameW(NULL);
	wchar_t *dlg_font;
	hGetPrivateProfileStringW(L"Tera Term", L"DlgFont", L"", setup_fname, &dlg_font);

	wchar_t face[LF_FACESIZE];
	int point = 0;
	int charset = 0;
	GetNthStringW(dlg_font, 1, _countof(face), face);
	GetNthNumW(dlg_font, 2, &point);
	GetNthNumW(dlg_font, 3, &charset);
	free(dlg_font);

	wchar_t *ui_language_file = GetUILanguageFileFullW(setup_fname);
	SetDialogFont(face, point, charset, ui_language_file, "Tera Term", "DLG_SYSTEM_FONT");
	free(ui_language_file);
	free(setup_fname);
}

/**
 *	設定ダイアログを表示する
 */
void ShowSetupDialog(HWND hParent, const std::vector<LaunchItem> &items)
{
	SetupDialogData data;
	data.items = &items;
	data.lang = GetLangInfo();
	SetDialogFontFromIni();
	TTDialogBoxParam(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_SETUP),
					 hParent, SetupDlgProc, (LPARAM)&data);
}
