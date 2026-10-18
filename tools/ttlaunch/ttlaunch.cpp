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
//	Tera Term をシェルのターミナルとして起動する
//	起動方法をポップアップメニューから選択する
//

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

#include <string>
#include <vector>

#include "ttlib_static_dir.h"
#include "win32helper.h"
#include "compat_win.h"
#include "cyglib.h"

#define Section L"Tera Term"

typedef enum {
	SYSTEM_CYGWIN,
	SYSTEM_MSYS2,
	SYSTEM_GIT_BASH
} system_t;

/**
 *	ポップアップに表示する起動方法
 */
struct LaunchItem {
	std::wstring id;		// -e で指定する名前
	std::wstring label;		// メニューに表示する文字列
	int group;				// グループが変わるところにセパレータを入れる
	system_t system;		// cygterm / msys2term のどちらを使うか
	std::wstring root;		// cygwin / msys2 / git のインストールフォルダ
	std::wstring args;		// cygterm(msys2term) に渡す引数
	bool posix_shell;		// cygwin/msys2 のシェル(CHERE_INVOKINGを設定する)
	std::wstring icon;		// メニューに表示するアイコン(.ico または .exe), 無いときは ""
};

/**
 *	ファイル(フォルダ)が存在するか
 */
static bool CheckExist(const std::wstring &file)
{
	DWORD r = GetFileAttributesW(file.c_str());
	return r != INVALID_FILE_ATTRIBUTES;
}

/**
 *	"\\" を "/" に置き換える
 */
static std::wstring ToSlash(std::wstring path)
{
	for (size_t i = 0; i < path.size(); i++) {
		if (path[i] == L'\\') {
			path[i] = L'/';
		}
	}
	return path;
}

/**
 *	コマンドライン引数として渡せるようクォートする
 */
static std::wstring QuoteArg(const std::wstring &arg)
{
	std::wstring s = L"\"";
	size_t backslashes = 0;
	for (size_t i = 0; i < arg.size(); i++) {
		wchar_t c = arg[i];
		if (c == L'\\') {
			backslashes++;
		}
		else if (c == L'"') {
			s.append(backslashes + 1, L'\\');
			backslashes = 0;
		}
		else {
			backslashes = 0;
		}
		s += c;
	}
	s.append(backslashes, L'\\');
	s += L'"';
	return s;
}

/**
 *	メニューに表示できるよう '&' をエスケープする
 */
static std::wstring EscapeMenu(const std::wstring &text)
{
	std::wstring s;
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == L'&') {
			s += L'&';
		}
		s += text[i];
	}
	return s;
}

/**
 *	固定ドライブのフォルダを探す
 *
 *	@param	path	"\\cygwin64" などドライブ名を除いたパス
 *	@param	check	path の下に存在すべきファイル
 *	@return	見つかったフォルダ, 見つからなかったとき ""
 */
static std::wstring SearchFixedDrives(const wchar_t *path, const wchar_t *check)
{
	DWORD drives = GetLogicalDrives();
	for (wchar_t c = L'C'; c <= L'Z'; c++) {
		if ((drives & (1 << (c - L'A'))) == 0) {
			continue;
		}
		wchar_t drive_root[] = { c, L':', L'\\', 0 };
		if (GetDriveTypeW(drive_root) != DRIVE_FIXED) {
			continue;
		}
		std::wstring dir = std::wstring(1, c) + L":" + path;
		if (CheckExist(dir + L"\\" + check)) {
			return dir;
		}
	}
	return L"";
}

/**
 *	TERATERM.INI から CygwinDirectory を読み込む
 */
static std::wstring GetCygwinDirFromIni(void)
{
	wchar_t *HomeDir = GetHomeDirW(NULL);
	std::wstring teraterm_ini = std::wstring(HomeDir) + L"\\TERATERM.INI";
	free(HomeDir);

	wchar_t *CygwinDir;
	hGetPrivateProfileStringW(Section, L"CygwinDirectory", L"", teraterm_ini.c_str(), &CygwinDir);
	std::wstring dir = CygwinDir;
	free(CygwinDir);
	return dir;
}

/**
 *	レジストリからインストールフォルダを取得する
 *
 *	HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER の順に、
 *	64bit, 32bit のレジストリビューを調べる
 *
 *	@param	subkey	"SOFTWARE\\Cygwin\\setup" など
 *	@param	value	"rootdir" など
 *	@param	check	フォルダの下に存在すべきファイル
 *	@return	見つかったフォルダ, 見つからなかったとき ""
 */
static std::wstring GetInstallDirFromRegistry(const wchar_t *subkey, const wchar_t *value, const wchar_t *check)
{
	static const HKEY roots[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };
	static const REGSAM views[] = { KEY_WOW64_64KEY, KEY_WOW64_32KEY };
	for (size_t i = 0; i < _countof(roots); i++) {
		for (size_t j = 0; j < _countof(views); j++) {
			HKEY hKey;
			if (RegOpenKeyExW(roots[i], subkey, 0, KEY_READ | views[j], &hKey) != ERROR_SUCCESS) {
				continue;
			}
			wchar_t *path;
			LSTATUS r = hRegQueryValueExW(hKey, value, NULL, NULL, (void **)&path, NULL);
			RegCloseKey(hKey);
			if (r == ERROR_SUCCESS) {
				std::wstring dir = path;
				free(path);
				if (!dir.empty() && CheckExist(dir + L"\\" + check)) {
					return dir;
				}
			}
		}
	}
	return L"";
}

static std::wstring FindCygwin(void)
{
	static const wchar_t *check = L"bin\\cygwin1.dll";
	std::wstring dir = GetCygwinDirFromIni();
	if (!dir.empty() && CheckExist(dir + L"\\" + check)) {
		return dir;
	}

	// Cygwin のインストーラー(setup-x86_64.exe)が登録するレジストリ
	//	管理者権限でインストールすると HKEY_LOCAL_MACHINE、
	//	管理者権限なし(--no-admin)でインストールすると HKEY_CURRENT_USER に登録される
	dir = GetInstallDirFromRegistry(L"SOFTWARE\\Cygwin\\setup", L"rootdir", check);
	if (!dir.empty()) {
		return dir;
	}

	dir = SearchFixedDrives(L"\\cygwin64", check);
	if (!dir.empty()) {
		return dir;
	}
	return SearchFixedDrives(L"\\cygwin", check);
}

static std::wstring FindMsys2(void)
{
	static const wchar_t *check = L"usr\\bin\\msys-2.0.dll";
	std::wstring dir = SearchFixedDrives(L"\\msys64", check);
	if (!dir.empty()) {
		return dir;
	}
	return SearchFixedDrives(L"\\msys", check);
}

static std::wstring FindGitForWindows(void)
{
	static const wchar_t *check = L"usr\\bin\\msys-2.0.dll";

	// Git for Windows のインストーラーが登録するレジストリ
	std::wstring dir = GetInstallDirFromRegistry(L"SOFTWARE\\GitForWindows", L"InstallPath", check);
	if (!dir.empty()) {
		return dir;
	}

	return SearchFixedDrives(L"\\Program Files\\Git", check);
}

/**
 *	インストール済みの WSL ディストリビューションを列挙する
 *
 *	@param[out]	default_distro	既定のディストリビューション
 */
static std::vector<std::wstring> GetWslDistributions(std::wstring &default_distro)
{
	std::vector<std::wstring> distros;
	default_distro.clear();

	HKEY hLxss;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Lxss", 0,
					  KEY_READ | KEY_WOW64_64KEY, &hLxss) != ERROR_SUCCESS) {
		return distros;
	}

	wchar_t *default_guid = NULL;
	hRegQueryValueExW(hLxss, L"DefaultDistribution", NULL, NULL, (void **)&default_guid, NULL);

	for (DWORD index = 0;; index++) {
		wchar_t guid[256];
		DWORD guid_len = _countof(guid);
		if (RegEnumKeyExW(hLxss, index, guid, &guid_len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
			break;
		}
		HKEY hDistro;
		if (RegOpenKeyExW(hLxss, guid, 0, KEY_READ | KEY_WOW64_64KEY, &hDistro) != ERROR_SUCCESS) {
			continue;
		}
		wchar_t *name;
		if (hRegQueryValueExW(hDistro, L"DistributionName", NULL, NULL, (void **)&name, NULL) == ERROR_SUCCESS) {
			// Docker Desktop が内部で使用するディストリビューションは除外する
			if (wcsncmp(name, L"docker-desktop", 14) != 0) {
				distros.push_back(name);
			}
			if (default_guid != NULL && _wcsicmp(default_guid, guid) == 0) {
				default_distro = name;
			}
			free(name);
		}
		RegCloseKey(hDistro);
	}

	free(default_guid);
	RegCloseKey(hLxss);
	return distros;
}

/**
 *	64bit の system32 フォルダ
 *		32bit プロセスでは System32 が SysWOW64 にリダイレクトされ、
 *		wsl.exe などが見つからないため Sysnative を使用する
 */
static std::wstring GetNativeSystemDir(void)
{
	wchar_t buf[MAX_PATH];
	GetWindowsDirectoryW(buf, _countof(buf));
	const std::wstring sysnative = std::wstring(buf) + L"\\Sysnative";
	if (CheckExist(sysnative)) {
		return sysnative;
	}
	GetSystemDirectoryW(buf, _countof(buf));
	return buf;
}

#ifndef IO_REPARSE_TAG_APPEXECLINK
#define IO_REPARSE_TAG_APPEXECLINK (0x8000001BL)
#endif
#ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE (16 * 1024)
#endif

/**
 *	アプリ実行エイリアス (App Execution Alias) の再解析ポイントのデータ
 *		REPARSE_DATA_BUFFER (ntifs.h) の AppExecLink 版
 */
typedef struct {
	ULONG ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	ULONG Version;			// 3
	WCHAR StringList[1];	// NUL 区切りの文字列: パッケージ名, AppUserModelID, 実体のパス, ...
} APPEXECLINK_REPARSE_BUFFER;

/**
 *	アプリ実行エイリアス (App Execution Alias) を実体のパスに解決する
 *
 *	Microsoft Store (MSIX) のアプリの %LOCALAPPDATA%\Microsoft\WindowsApps\*.exe は
 *	0 バイトの再解析ポイント (IO_REPARSE_TAG_APPEXECLINK) で、
 *	CreateProcess() では実行できるが、LoadLibraryEx() や ExtractIconEx() では開けず
 *	アイコンを取得できない
 *	実体 (C:\Program Files\WindowsApps\<パッケージ>\*.exe) はパスが分かれば読める
 *
 *	@return	実体のパス,
 *			アプリ実行エイリアスではないとき L""
 */
static std::wstring ResolveAppExecLink(const std::wstring &path)
{
	const DWORD attr = GetFileAttributesW(path.c_str());
	if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
		return L"";
	}
	HANDLE hFile = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
							   OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return L"";
	}
	std::vector<BYTE> buf(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
	DWORD size = 0;
	const BOOL r = DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, NULL, 0, &buf[0], (DWORD)buf.size(), &size, NULL);
	CloseHandle(hFile);
	const APPEXECLINK_REPARSE_BUFFER *rb = (const APPEXECLINK_REPARSE_BUFFER *)&buf[0];
	const size_t header = offsetof(APPEXECLINK_REPARSE_BUFFER, StringList);
	if (!r || size < header || rb->ReparseTag != IO_REPARSE_TAG_APPEXECLINK) {
		return L"";
	}

	// 文字列を順に取り出し、3 番目 (実体のパス) を返す
	size_t data_end = 8 + rb->ReparseDataLength;	// ReparseTag, ReparseDataLength, Reserved の後からデータ
	if (data_end > size) {
		data_end = size;
	}
	const wchar_t *p = rb->StringList;
	const wchar_t *end = (const wchar_t *)(&buf[0] + data_end);
	for (int i = 0; p < end; i++) {
		const wchar_t *q = p;
		while (q < end && *q != 0) {
			q++;
		}
		if (i == 2) {
			return std::wstring(p, q);
		}
		p = q + 1;
	}
	return L"";
}

/**
 *	アイコンファイルが存在すればそのパスを返す
 */
static std::wstring IconIfExist(const std::wstring &path)
{
	return CheckExist(path) ? path : L"";
}

/**
 *	起動方法の一覧を作成する
 */
static std::wstring GetExeDir(void)
{
	wchar_t *ExeDir = GetExeDirW(NULL);
	std::wstring exe_dir = ExeDir;
	free(ExeDir);
	return exe_dir;
}

static std::vector<LaunchItem> CreateItems(void)
{
	std::vector<LaunchItem> items;

	const std::wstring exe_dir = GetExeDir();
	const bool cygterm_exist = CheckExist(exe_dir + L"\\cygterm.exe");
	const bool msys2term_exist = CheckExist(exe_dir + L"\\msys2term.exe");

	const std::wstring cygwin_dir = cygterm_exist ? FindCygwin() : L"";
	const std::wstring msys2_dir = msys2term_exist ? FindMsys2() : L"";
	const std::wstring git_dir = msys2term_exist ? FindGitForWindows() : L"";

	// cygwin / msys2 / git bash
	if (!cygwin_dir.empty()) {
		LaunchItem item = { L"cygwin", L"&Cygwin", 0, SYSTEM_CYGWIN, cygwin_dir, L"", true, L"" };
		item.icon = IconIfExist(cygwin_dir + L"\\Cygwin.ico");
		items.push_back(item);
	}
	if (!msys2_dir.empty()) {
		static const struct {
			const wchar_t *id;
			const wchar_t *label;
			const wchar_t *msystem;
			const wchar_t *dir;		// 環境が存在するか調べるフォルダ
			const wchar_t *icon;	// msys2 のインストールフォルダにあるアイコン
		} msys2_envs[] = {
			{ L"msys2", L"&MSYS2 MSYS", L"MSYS", L"usr", L"msys2.ico" },
			{ L"mingw32", L"MSYS2 MinGW x&32", L"MINGW32", L"mingw32", L"mingw32.ico" },
			{ L"mingw64", L"MSYS2 MinGW x&64", L"MINGW64", L"mingw64", L"mingw64.ico" },
			{ L"ucrt64", L"MSYS2 &UCRT64", L"UCRT64", L"ucrt64", L"ucrt64.ico" },
			{ L"clang64", L"MSYS2 C&LANG64", L"CLANG64", L"clang64", L"clang64.ico" },
			{ L"clangarm64", L"MSYS2 CLANG&ARM64", L"CLANGARM64", L"clangarm64", L"clangarm64.ico" },
		};
		for (size_t i = 0; i < _countof(msys2_envs); i++) {
			if (!CheckExist(msys2_dir + L"\\" + msys2_envs[i].dir)) {
				continue;
			}
			LaunchItem item = { msys2_envs[i].id, msys2_envs[i].label, 0, SYSTEM_MSYS2, msys2_dir,
								std::wstring(L"-v MSYSTEM=") + msys2_envs[i].msystem, true, L"" };
			item.icon = IconIfExist(msys2_dir + L"\\" + msys2_envs[i].icon);
			items.push_back(item);
		}
	}
	if (!git_dir.empty()) {
		LaunchItem item = { L"gitbash", L"&Git Bash", 0, SYSTEM_GIT_BASH, git_dir, L"", true, L"" };
		item.icon = IconIfExist(git_dir + L"\\git-bash.exe");
		items.push_back(item);
	}

	// Windows のプログラムは cygterm または msys2term を経由して実行する
	system_t relay_system;
	std::wstring relay_dir;
	if (!cygwin_dir.empty()) {
		relay_system = SYSTEM_CYGWIN;
		relay_dir = cygwin_dir;
	}
	else if (!msys2_dir.empty()) {
		relay_system = SYSTEM_MSYS2;
		relay_dir = msys2_dir;
	}
	else if (!git_dir.empty()) {
		relay_system = SYSTEM_GIT_BASH;
		relay_dir = git_dir;
	}
	else {
		return items;
	}

	wchar_t system_dir_buf[MAX_PATH];
	GetSystemDirectoryW(system_dir_buf, _countof(system_dir_buf));
	const std::wstring system_dir = ToSlash(system_dir_buf);
	const std::wstring native_system_dir = GetNativeSystemDir();

	// wsl
	std::wstring default_distro;
	std::vector<std::wstring> distros = GetWslDistributions(default_distro);
	if (!distros.empty()) {
		const std::wstring wsl = system_dir + L"/wsl.exe";
		const std::wstring wsl_icon = IconIfExist(native_system_dir + L"\\wsl.exe");
		LaunchItem item = { L"wsl", L"&WSL", 1, relay_system, relay_dir,
							L"-nols -s " + QuoteArg(wsl), false, L"" };
		if (!default_distro.empty()) {
			item.label += L" (" + EscapeMenu(default_distro) + L")";
		}
		item.icon = wsl_icon;
		items.push_back(item);
		for (size_t i = 0; i < distros.size(); i++) {
			LaunchItem distro = { L"wsl:" + distros[i], L"WSL: " + EscapeMenu(distros[i]), 1, relay_system, relay_dir,
								  L"-nols -s " + QuoteArg(wsl + L" -d '" + distros[i] + L"'"), false, L"" };
			distro.icon = wsl_icon;
			items.push_back(distro);
		}
	}

	// Windows の shell
	{
		LaunchItem item = { L"cmd", L"C&ommand Prompt", 2, relay_system, relay_dir,
							L"-nols -s " + QuoteArg(system_dir + L"/cmd.exe"), false, L"" };
		item.icon = IconIfExist(native_system_dir + L"\\cmd.exe");
		items.push_back(item);
	}
	{
		LaunchItem item = { L"powershell", L"Windows &PowerShell", 2, relay_system, relay_dir,
							L"-nols -s " + QuoteArg(system_dir + L"/WindowsPowerShell/v1.0/powershell.exe"), false, L"" };
		item.icon = IconIfExist(native_system_dir + L"\\WindowsPowerShell\\v1.0\\powershell.exe");
		items.push_back(item);
	}
	{
		wchar_t pwsh[MAX_PATH];
		if (SearchPathW(NULL, L"pwsh.exe", NULL, _countof(pwsh), pwsh, NULL) > 0) {
			LaunchItem item = { L"pwsh", L"Po&werShell 7", 2, relay_system, relay_dir,
								L"-nols -s " + QuoteArg(L"'" + ToSlash(pwsh) + L"'"), false, L"" };
			// Microsoft Store 版はアプリ実行エイリアスで、そのままではアイコンを読めない
			//	起動はエイリアスのまま行う (更新でバージョン付きのフォルダ名が変わっても追従する)
			const std::wstring target = ResolveAppExecLink(pwsh);
			item.icon = target.empty() ? pwsh : target;
			items.push_back(item);
		}
	}

	return items;
}

/**
 *	エクスプローラーのコンテキストメニューのレジストリキー
 *		インストーラー(teraterm.iss)と同じキー
 */
static const struct {
	const wchar_t *key;		// HKEY_CLASSES_ROOT(Software\Classes) 以下のキー
	const wchar_t *param;	// フォルダを示すパラメータ
} context_menu_keys[] = {
	{ L"Folder\\shell\\ttlaunch", L"%L" },
	{ L"Directory\\Background\\shell\\ttlaunch", L"%V" },
	{ L"LibraryFolder\\Background\\shell\\ttlaunch", L"%V" },
};

/**
 *	コンテキストメニューが登録されているか
 *		HKEY_CLASSES_ROOT を調べるので HKCU, HKLM のどちらに登録されていても良い
 */
static bool IsContextMenuRegistered(void)
{
	for (size_t i = 0; i < _countof(context_menu_keys); i++) {
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_CLASSES_ROOT, context_menu_keys[i].key, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
			return false;
		}
		RegCloseKey(hKey);
	}
	return true;
}

/**
 *	コンテキストメニューが HKEY_CURRENT_USER に登録されているか
 *		一部のキーだけ登録されている場合も登録されているとする
 */
static bool IsContextMenuRegisteredCurrentUser(void)
{
	for (size_t i = 0; i < _countof(context_menu_keys); i++) {
		const std::wstring key = std::wstring(L"Software\\Classes\\") + context_menu_keys[i].key;
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, key.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return true;
		}
	}
	return false;
}

static LSTATUS RegSetString(HKEY hKey, const wchar_t *name, const std::wstring &value)
{
	return RegSetValueExW(hKey, name, 0, REG_SZ, (const BYTE *)value.c_str(),
						  (DWORD)((value.size() + 1) * sizeof(wchar_t)));
}

/**
 *	コンテキストメニューをレジストリ(HKEY_CURRENT_USER)に登録する
 */
static LSTATUS RegisterContextMenu(void)
{
	wchar_t *exe_path;
	hGetModuleFileNameW(NULL, &exe_path);
	const std::wstring exe = L"\"" + std::wstring(exe_path) + L"\"";
	free(exe_path);

	for (size_t i = 0; i < _countof(context_menu_keys); i++) {
		const std::wstring key = std::wstring(L"Software\\Classes\\") + context_menu_keys[i].key;
		HKEY hKey;
		LSTATUS r = RegCreateKeyExW(HKEY_CURRENT_USER, key.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
		if (r != ERROR_SUCCESS) {
			return r;
		}
		RegSetString(hKey, NULL, L"Tera Term &Here");
		RegSetString(hKey, L"Icon", exe);
		RegCloseKey(hKey);

		const std::wstring command_key = key + L"\\command";
		r = RegCreateKeyExW(HKEY_CURRENT_USER, command_key.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
		if (r != ERROR_SUCCESS) {
			return r;
		}
		r = RegSetString(hKey, NULL, exe + L" -d \"" + context_menu_keys[i].param + L"\"");
		RegCloseKey(hKey);
		if (r != ERROR_SUCCESS) {
			return r;
		}
	}
	return ERROR_SUCCESS;
}

static void RegisterContextMenuWithMessage(void)
{
	LSTATUS r = RegisterContextMenu();
	if (r == ERROR_SUCCESS) {
		MessageBoxW(NULL, L"\"Tera Term Here\" has been added to the Explorer context menu.", L"ttlaunch",
					MB_OK | MB_ICONINFORMATION);
	}
	else {
		wchar_t *error_msg;
		hFormatMessageW(r, &error_msg);
		std::wstring msg = L"Failed to add to the Explorer context menu.\n" + std::wstring(error_msg);
		free(error_msg);
		MessageBoxW(NULL, msg.c_str(), L"ttlaunch", MB_OK | MB_ICONWARNING);
	}
}

/**
 *	コンテキストメニューを HKEY_CURRENT_USER から削除する
 *		HKEY_LOCAL_MACHINE に登録されたもの(インストーラーで登録)は削除しない
 */
static LSTATUS UnregisterContextMenu(void)
{
	for (size_t i = 0; i < _countof(context_menu_keys); i++) {
		const std::wstring key = std::wstring(L"Software\\Classes\\") + context_menu_keys[i].key;
		const std::wstring command_key = key + L"\\command";
		LSTATUS r = RegDeleteKeyW(HKEY_CURRENT_USER, command_key.c_str());
		if (r != ERROR_SUCCESS && r != ERROR_FILE_NOT_FOUND) {
			return r;
		}
		r = RegDeleteKeyW(HKEY_CURRENT_USER, key.c_str());
		if (r != ERROR_SUCCESS && r != ERROR_FILE_NOT_FOUND) {
			return r;
		}
	}
	return ERROR_SUCCESS;
}

static void UnregisterContextMenuWithMessage(void)
{
	LSTATUS r = UnregisterContextMenu();
	if (r == ERROR_SUCCESS) {
		MessageBoxW(NULL, L"\"Tera Term Here\" has been removed from the Explorer context menu.", L"ttlaunch",
					MB_OK | MB_ICONINFORMATION);
	}
	else {
		wchar_t *error_msg;
		hFormatMessageW(r, &error_msg);
		std::wstring msg = L"Failed to remove from the Explorer context menu.\n" + std::wstring(error_msg);
		free(error_msg);
		MessageBoxW(NULL, msg.c_str(), L"ttlaunch", MB_OK | MB_ICONWARNING);
	}
}

/**
 *	ShowPopupMenu() の戻り値
 */
enum {
	MENU_CANCEL = -1,		// キャンセルされた
	MENU_REGISTER = -2,		// コンテキストメニューへの登録が選択された
	MENU_UNREGISTER = -3,	// コンテキストメニューからの削除が選択された
};

static const wchar_t *owner_class_name = L"TTLaunchWindowClass";

/**
 *	表示しないオーナーウィンドウを作成し、フォアグラウンドにする
 *		- TrackPopupMenu() のオーナーウィンドウ
 *		- 起動した ttermpro.exe がフォアグラウンドになれるよう、
 *		  ttermpro.exe のウィンドウが表示されるまでフォアグラウンドを保持する
 *		カーソルのあるモニタのDPIでメニューが表示されるよう、カーソル位置に作成する
 */
static HWND CreateOwnerWindow(void)
{
	HINSTANCE hInst = GetModuleHandleW(NULL);

	WNDCLASSW wc = {};
	wc.lpfnWndProc = DefWindowProcW;
	wc.hInstance = hInst;
	wc.lpszClassName = owner_class_name;
	RegisterClassW(&wc);

	POINT pt;
	GetCursorPos(&pt);
	HWND hWnd = CreateWindowExW(WS_EX_TOOLWINDOW, owner_class_name, L"ttlaunch", WS_POPUP,
								pt.x, pt.y, 0, 0, NULL, NULL, hInst, NULL);
	SetForegroundWindow(hWnd);
	return hWnd;
}

static void DestroyOwnerWindow(HWND hWnd)
{
	DestroyWindow(hWnd);
	UnregisterClassW(owner_class_name, GetModuleHandleW(NULL));
}

/**
 *	起動したプロセスのウィンドウがフォアグラウンドになるまで待つ
 *
 *	フォアグラウンドウィンドウを持っていないプロセスは、
 *	自分のウィンドウをフォアグラウンドにすることができない
 *	ttermpro.exe は cygterm から起動されるため、そのままでは
 *	他のウィンドウ(エクスプローラーなど)の裏に表示されてしまう
 *	オーナーウィンドウをフォアグラウンドにしたまま AllowSetForegroundWindow(ASFW_ANY) を呼び、
 *	ttermpro.exe のウィンドウがフォアグラウンドになる(オーナーウィンドウがフォアグラウンドでなくなる)まで待つ
 */
static void WaitForegroundChanged(HWND hWnd)
{
	static const DWORD timeout_ms = 10 * 1000;
	const DWORD start = GetTickCount();
	while (GetForegroundWindow() == hWnd && GetTickCount() - start < timeout_ms) {
		MSG msg;
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		Sleep(100);
	}
}

static BOOL CALLBACK EnumFirstIconProc(HMODULE, LPCWSTR, LPWSTR name, LONG_PTR param)
{
	LPWSTR *first = (LPWSTR *)param;
	*first = IS_INTRESOURCE(name) ? name : _wcsdup(name);
	return FALSE;	// 最初のアイコンだけ
}

/**
 *	アイコンを読み込む
 *
 *	@param	path	.ico ファイル、または .exe ファイル(最初のアイコンを使用する)
 *	@param	size	アイコンのサイズ(pixel)
 *	@return	アイコン, 読み込めなかったとき NULL, DestroyIcon() すること
 */
static HICON LoadIconFile(const std::wstring &path, int size)
{
	const size_t len = path.size();
	if (len > 4 && _wcsicmp(path.c_str() + len - 4, L".ico") == 0) {
		return (HICON)LoadImageW(NULL, path.c_str(), IMAGE_ICON, size, size, LR_LOADFROMFILE);
	}

	HMODULE hModule = LoadLibraryExW(path.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (hModule == NULL) {
		return NULL;
	}
	LPWSTR name = NULL;
	EnumResourceNamesW(hModule, MAKEINTRESOURCEW(14) /* RT_GROUP_ICON */, EnumFirstIconProc, (LONG_PTR)&name);
	HICON hIcon = NULL;
	if (name != NULL) {
		hIcon = (HICON)LoadImageW(hModule, name, IMAGE_ICON, size, size, 0);
		if (!IS_INTRESOURCE(name)) {
			free(name);
		}
	}
	FreeLibrary(hModule);
	return hIcon;
}

/**
 *	アイコンをメニューに表示できる 32bit(premultiplied alpha) のビットマップに変換する
 *
 *	@return	ビットマップ, 変換できなかったとき NULL, DeleteObject() すること
 */
static HBITMAP IconToBitmap(HICON hIcon)
{
	ICONINFO ii;
	if (!GetIconInfo(hIcon, &ii)) {
		return NULL;
	}

	HBITMAP hBitmap = NULL;
	BITMAP bm;
	if (ii.hbmColor != NULL && GetObjectW(ii.hbmColor, sizeof(bm), &bm) != 0) {
		const int width = bm.bmWidth;
		const int height = bm.bmHeight;
		const size_t count = (size_t)width * height;
		std::vector<DWORD> color(count);
		std::vector<DWORD> mask(count);

		BITMAPINFO bmi = {};
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = -height;	// top-down
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		HDC hDC = GetDC(NULL);
		BITMAPINFO bmi_tmp = bmi;
		int r1 = GetDIBits(hDC, ii.hbmColor, 0, height, &color[0], &bmi_tmp, DIB_RGB_COLORS);
		bmi_tmp = bmi;
		int r2 = GetDIBits(hDC, ii.hbmMask, 0, height, &mask[0], &bmi_tmp, DIB_RGB_COLORS);

		if (r1 == height && r2 == height) {
			// アルファチャンネルを持っているか?
			bool has_alpha = false;
			for (size_t i = 0; i < count; i++) {
				if ((color[i] & 0xff000000) != 0) {
					has_alpha = true;
					break;
				}
			}

			void *bits;
			hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
			if (hBitmap != NULL) {
				DWORD *dest = (DWORD *)bits;
				for (size_t i = 0; i < count; i++) {
					DWORD c = color[i];
					DWORD a;
					if (has_alpha) {
						a = c >> 24;
					}
					else {
						// マスクが黒(0)の部分が不透明
						a = (mask[i] & 0x00ffffff) == 0 ? 255 : 0;
					}
					DWORD r = ((c >> 16) & 0xff) * a / 255;
					DWORD g = ((c >> 8) & 0xff) * a / 255;
					DWORD b = (c & 0xff) * a / 255;
					dest[i] = (a << 24) | (r << 16) | (g << 8) | b;
				}
			}
		}
		ReleaseDC(NULL, hDC);
	}

	if (ii.hbmColor != NULL) {
		DeleteObject(ii.hbmColor);
	}
	if (ii.hbmMask != NULL) {
		DeleteObject(ii.hbmMask);
	}
	return hBitmap;
}

/**
 *	メニューに表示する小さいアイコンのサイズ
 *		ウィンドウのある(カーソルのある)モニタのDPIに合わせる
 */
static int GetMenuIconSize(HWND hWnd)
{
	if (pGetDpiForWindow != NULL && pGetSystemMetricsForDpi != NULL) {
		return pGetSystemMetricsForDpi(SM_CXSMICON, pGetDpiForWindow(hWnd));
	}
	return GetSystemMetrics(SM_CXSMICON);
}

/**
 *	起動できる項目が無いときのメッセージ
 */
static std::wstring GetNoItemMessage(void)
{
	const std::wstring exe_dir = GetExeDir();
	if (!CheckExist(exe_dir + L"\\cygterm.exe") && !CheckExist(exe_dir + L"\\msys2term.exe")) {
		// cygterm, msys2term が無いと Cygwin, MSYS2 などを探さない
		return L"cygterm.exe or msys2term.exe not found in " + EscapeMenu(exe_dir);
	}
	return L"Cygwin, MSYS2 or Git for Windows not found";
}

/**
 *	マウスカーソル位置にポップアップメニューを表示する
 *
 *	@param	hWnd			オーナーウィンドウ
 *	@param	show_register	コンテキストメニューへの登録項目を表示する
 *	@param	show_unregister	コンテキストメニューからの削除項目を表示する
 *	@return	選択された項目の index, MENU_CANCEL, MENU_REGISTER, MENU_UNREGISTER
 */
static int ShowPopupMenu(HWND hWnd, const std::vector<LaunchItem> &items, const std::wstring &dir,
						 bool show_register, bool show_unregister)
{
	static const UINT ID_REGISTER = 0x7000;
	static const UINT ID_UNREGISTER = 0x7001;

	POINT pt;
	GetCursorPos(&pt);

	HMENU hMenu = CreatePopupMenu();
	const std::wstring title = dir.empty() ? L"Tera Term" : L"Tera Term: " + EscapeMenu(dir);
	AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0, title.c_str());
	AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
	if (items.empty()) {
		AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0, GetNoItemMessage().c_str());
	}
	const int icon_size = GetMenuIconSize(hWnd);
	std::vector<HBITMAP> bitmaps;
	for (size_t i = 0; i < items.size(); i++) {
		if (i != 0 && items[i - 1].group != items[i].group) {
			AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
		}
		AppendMenuW(hMenu, MF_STRING, i + 1, items[i].label.c_str());

		// アイコン
		if (!items[i].icon.empty()) {
			HICON hIcon = LoadIconFile(items[i].icon, icon_size);
			if (hIcon != NULL) {
				HBITMAP hBitmap = IconToBitmap(hIcon);
				DestroyIcon(hIcon);
				if (hBitmap != NULL) {
					MENUITEMINFOW mii = {};
					mii.cbSize = sizeof(mii);
					mii.fMask = MIIM_BITMAP;
					mii.hbmpItem = hBitmap;
					SetMenuItemInfoW(hMenu, (UINT)(i + 1), FALSE, &mii);
					bitmaps.push_back(hBitmap);
				}
			}
		}
	}
	if (show_register || show_unregister) {
		AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
	}
	if (show_register) {
		AppendMenuW(hMenu, MF_STRING, ID_REGISTER, L"Add \"Tera Term &Here\" to Explorer context menu");
	}
	if (show_unregister) {
		AppendMenuW(hMenu, MF_STRING, ID_UNREGISTER, L"&Remove \"Tera Term Here\" from Explorer context menu");
	}

	SetForegroundWindow(hWnd);
	int cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN,
							   pt.x, pt.y, hWnd, NULL);
	PostMessageW(hWnd, WM_NULL, 0, 0);

	DestroyMenu(hMenu);
	for (size_t i = 0; i < bitmaps.size(); i++) {
		DeleteObject(bitmaps[i]);
	}

	if (cmd == 0) {
		return MENU_CANCEL;
	}
	if ((UINT)cmd == ID_REGISTER) {
		return MENU_REGISTER;
	}
	if ((UINT)cmd == ID_UNREGISTER) {
		return MENU_UNREGISTER;
	}
	return cmd - 1;
}

/**
 *	cygterm(msys2term) を実行する
 *
 *	@retval	true	実行できた
 */
static bool Launch(const LaunchItem &item, const std::wstring &dir, const std::wstring &extra_args)
{
	std::wstring cmdline = item.args;
	if (!dir.empty()) {
		cmdline += L" -nocd";
		if (item.posix_shell) {
			cmdline += L" -v CHERE_INVOKING=y";
		}
		cmdline += L" -d " + QuoteArg(ToSlash(dir));
	}
	cmdline += extra_args;

	DWORD e;
	switch (item.system) {
	default:
	case SYSTEM_CYGWIN:
		e = CygwinConnect(item.root.c_str(), cmdline.c_str());
		break;
	case SYSTEM_MSYS2:
		e = Msys2Connect(item.root.c_str(), cmdline.c_str());
		break;
	case SYSTEM_GIT_BASH:
		e = GitBashConnect(item.root.c_str(), cmdline.c_str());
		break;
	}

	switch (e) {
	case NO_ERROR:
		break;
	case ERROR_FILE_NOT_FOUND:
		MessageBoxW(NULL, L"Can't find Cygwin(MSYS2) directory.", L"ttlaunch", MB_OK | MB_ICONWARNING);
		break;
	case ERROR_NOT_ENOUGH_MEMORY:
		MessageBoxW(NULL, L"Can't allocate memory.", L"ttlaunch", MB_OK | MB_ICONWARNING);
		break;
	case ERROR_OPEN_FAILED:
	default: {
		const wchar_t *msg =
			item.system == SYSTEM_CYGWIN ? L"Can't execute cygterm." : L"Can't execute msys2term.";
		MessageBoxW(NULL, msg, L"ttlaunch", MB_OK | MB_ICONWARNING);
		break;
	}
	}

	return e == NO_ERROR;
}

/**
 *	-d で指定されたフォルダを cygterm に渡せる形にする
 *		エクスプローラーから "%V" で渡されたとき、"C:\" は C:" となるため '"' を取り除く
 */
static std::wstring NormalizeDir(const wchar_t *arg)
{
	std::wstring dir;
	for (const wchar_t *p = arg; *p != 0; p++) {
		if (*p != L'"') {
			dir += *p;
		}
	}
	if (dir.size() == 2 && dir[1] == L':') {
		// "C:" -> "C:\"
		dir += L'\\';
	}
	return dir;
}

static void ShowUsage(const std::vector<LaunchItem> &items)
{
	std::wstring msg =
		L"ttlaunch [options] [cygterm options]\n"
		L"\n"
		L"  -d <directory>\tstart directory\n"
		L"  -v <NAME=VALUE>\tenvironment variable (can be specified multiple times)\n"
		L"  -e <id>\tlaunch without popup menu\n"
		L"  -h\tshow this help\n"
		L"\n"
		L"other options are passed to cygterm(msys2term)\n"
		L"\n"
		L"available id:\n";
	for (size_t i = 0; i < items.size(); i++) {
		msg += L"  " + items[i].id + L"\n";
	}
	MessageBoxW(NULL, msg.c_str(), L"ttlaunch", MB_OK | MB_ICONINFORMATION);
}

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "");
	WinCompatInit();

	// DPI Aware (高DPI対応)
	//		Windows 10 Version 1703から使用可能な
	//		Per-monitor DPI awareness V2 の機能を利用
	if (pSetProcessDpiAwarenessContext != NULL) {
		pSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	}

	std::wstring dir;
	std::wstring launch_id;
	std::wstring extra_args;
	bool show_usage = false;

	for (int i = 1; i < argc; i++) {
		if (wcscmp(argv[i], L"-d") == 0 && i + 1 < argc) {
			i++;
			dir = NormalizeDir(argv[i]);
		}
		else if (wcscmp(argv[i], L"-v") == 0 && i + 1 < argc) {
			i++;
			extra_args += L" -v " + QuoteArg(argv[i]);
		}
		else if (wcscmp(argv[i], L"-e") == 0 && i + 1 < argc) {
			i++;
			launch_id = argv[i];
		}
		else if (wcscmp(argv[i], L"-h") == 0 || wcscmp(argv[i], L"/?") == 0) {
			show_usage = true;
		}
		else {
			extra_args += L" " + QuoteArg(argv[i]);
		}
	}

	std::vector<LaunchItem> items = CreateItems();

	if (show_usage) {
		ShowUsage(items);
		return 0;
	}

	int index = -1;
	if (!launch_id.empty()) {
		for (size_t i = 0; i < items.size(); i++) {
			if (_wcsicmp(items[i].id.c_str(), launch_id.c_str()) == 0) {
				index = (int)i;
				break;
			}
		}
		if (index < 0) {
			std::wstring msg = L"'" + launch_id + L"' is not available.";
			MessageBoxW(NULL, msg.c_str(), L"ttlaunch", MB_OK | MB_ICONWARNING);
			return 1;
		}
	}

	HWND hWnd = CreateOwnerWindow();

	if (launch_id.empty()) {
		// HKEY_LOCAL_MACHINE に登録されたもの(インストーラーで登録)は削除できないので、
		// HKEY_CURRENT_USER に登録されているときだけ削除項目を表示する
		index = ShowPopupMenu(hWnd, items, dir, !IsContextMenuRegistered(), IsContextMenuRegisteredCurrentUser());
		if (index < 0) {
			DestroyOwnerWindow(hWnd);
			if (index == MENU_REGISTER) {
				RegisterContextMenuWithMessage();
			}
			else if (index == MENU_UNREGISTER) {
				UnregisterContextMenuWithMessage();
			}
			return 0;
		}
	}

	// 起動するプロセスがフォアグラウンドになれるようにする
	//		AllowSetForegroundWindow() は Windows 98/2000 以降
	typedef BOOL (WINAPI *AllowSetForegroundWindow_t)(DWORD dwProcessId);
	AllowSetForegroundWindow_t pAllowSetForegroundWindow =
		(AllowSetForegroundWindow_t)(void *)GetProcAddress(GetModuleHandleW(L"user32.dll"), "AllowSetForegroundWindow");
	if (pAllowSetForegroundWindow != NULL) {
		pAllowSetForegroundWindow(ASFW_ANY);
	}
	if (Launch(items[index], dir, extra_args)) {
		WaitForegroundChanged(hWnd);
	}
	DestroyOwnerWindow(hWnd);
	return 0;
}
