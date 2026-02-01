/*
 * Copyright (C) 2022- TeraTerm Project
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

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <windows.h>
#include <shobjidl.h>
#include <shlguid.h>

static wchar_t *EscapeCommand(const wchar_t *command)
{
	size_t len = wcslen(command);
	wchar_t *escaped_command = (wchar_t *)malloc(sizeof(wchar_t) * len * 2);
	wchar_t *p = escaped_command;

	wchar_t c;
	while((c = *command++) != L'\0') {
		if (c == L'"' || c == L'\\') {
			*p++ = L'\\';
		}
		*p++ = c;
	}
	*p = 0;
	return escaped_command;
}

/**
 * ショートカットを作る
 *		Win32 API
 */
HRESULT CreateShortcut(const wchar_t *exe, const wchar_t *args, const wchar_t *dest, const wchar_t *icon_path, int icon_no)
{
	HRESULT hres;

	IShellLinkW* psl;
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
	if (FAILED(hres)) {
		return hres;
	}

	hres = psl->SetPath(exe);
	if (args != NULL) {
		hres = psl->SetArguments(args);
	}

	if (icon_path != NULL) {
		psl->SetIconLocation(icon_path, icon_no);
	}

	// save
	IPersistFile* ppf;
	hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

	if (SUCCEEDED(hres))
	{
		hres = ppf->Save(dest, TRUE);
		ppf->Release();
	}

	psl->Release();
	return hres;
}

/**
 * ショートカットを作る
 */
void CreateShortcut(const wchar_t *dir, const wchar_t *shortcut, const wchar_t *teraterm, const wchar_t *args, const wchar_t *icon_path = NULL, int icon_no = 0)
{
	wchar_t shortcut_full[MAX_PATH];
	swprintf(shortcut_full, _countof(shortcut_full), L"%s\\%s", dir, shortcut);
	wchar_t *escaped_command = EscapeCommand(teraterm);
	CreateShortcut(escaped_command, args, shortcut_full, icon_path, icon_no);
	free(escaped_command);
}

/**
 *	レジストリを書き出す
 */
void WriteReg(FILE *fp, const wchar_t *menu, const wchar_t *key, const wchar_t *command, const wchar_t *arg, const wchar_t *icon = NULL)
{
	wchar_t *escaped_command = EscapeCommand(command);
	wchar_t *escaped_arg = EscapeCommand(arg);
	wchar_t *escaped_icon = icon == NULL ? NULL : EscapeCommand(icon);
	fwprintf(fp,
			 L"[HKEY_CURRENT_USER\\SOFTWARE\\Classes\\Folder\\shell\\%s]\n"
			 L"@=\"%s\"\n"
			 L"\"Icon\"=\"%s\"\n"
			 L"\n"
			 L"[HKEY_CURRENT_USER\\SOFTWARE\\Classes\\Folder\\shell\\%s\\command]\n"
			 L"@=\"%s %s\"\n\n",
			 key,
			 menu,
			 escaped_icon == NULL ? L"" : escaped_icon,
			 key,
			 escaped_command, escaped_arg);
	free(escaped_icon);
	free(escaped_arg);
	free(escaped_command);
}

BOOL CheckExist(const wchar_t *file)
{
	DWORD r = GetFileAttributesW(file);
	if (r == INVALID_FILE_ATTRIBUTES) {
		return FALSE;
	}
	return TRUE;
}

BOOL CheckCygwin(void)
{
	return CheckExist(L"C:\\cygwin64\\bin\\cygwin1.dll");
}

BOOL CheckMsys2(void)
{
	return CheckExist(L"C:\\msys64\\usr\\bin\\msys-2.0.dll");
}

BOOL CheckGitbash(void)
{
	return CheckExist(L"C:\\Program Files\\Git\\usr\\bin\\msys-2.0.dll");
}

BOOL CheckMsys2term(void)
{
	return CheckExist(L"msys2term.exe");
}

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "");
	CoInitialize(NULL);

	BOOL cygwin_installed = CheckCygwin();
	BOOL msys2tarm = CheckMsys2term();
	BOOL msys2_installed = CheckMsys2();
	BOOL gitbash_installed = CheckGitbash();

	if (cygwin_installed) {
		printf("cygwin found\n");
	}
	if (msys2_installed && msys2tarm) {
		printf("msys2 found\n");
	}
	if (gitbash_installed) {
		printf("git bash found\n");
	}

	const wchar_t *reg_fname = L"explorer_context.reg";

	wchar_t curdir[MAX_PATH];
	GetCurrentDirectoryW(_countof(curdir), curdir);

	wchar_t teraterm_full[MAX_PATH];
	swprintf(teraterm_full, _countof(teraterm_full), L"%s\\%s", curdir, L"ttermpro.exe");

	wchar_t cyglaunch_full[MAX_PATH];
	swprintf(cyglaunch_full, _countof(cyglaunch_full), L"%s\\%s", curdir, L"cyglaunch.exe");

	wchar_t shortcut_dir[MAX_PATH];
	swprintf(shortcut_dir, _countof(shortcut_dir), L"%s\\%s", curdir, L"shortcuts");
	CreateDirectoryW(shortcut_dir, NULL);
	wprintf(L"mkdir '%s'\n", shortcut_dir);

	FILE *fp = _wfopen(reg_fname, L"wt, ccs=UTF-16LE");
	//fwrite("\xff\xfe", 2, 1, fp);	// write UTF16LE bom

	fwprintf(fp, L"Windows Registry Editor Version 5.00\n\n");

	CreateShortcut(shortcut_dir, L"Tera Term.lnk", teraterm_full, NULL);

	if (cygwin_installed) {
		CreateShortcut(shortcut_dir, L"Tera Term + cmd.lnk", cyglaunch_full, L"-cygwin -s \"c:/windows/system32/cmd.exe\"",
					   L"c:\\windows\\system32\\cmd.exe", 0);
		WriteReg(fp,
				 L"Tera Term + cmd Here",
				 L"cygterm_cmd",
				 cyglaunch_full,
				 L"-nocd -v CHERE_INVOKING=y -d \"\\\"%L\\\"\" -s \"c:/windows/system32/cmd.exe\"",
				 L"c:\\windows\\system32\\cmd.exe");

		CreateShortcut(shortcut_dir, L"Tera Term + powershell.lnk", cyglaunch_full, L"-cygwin -s \"c:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe\"",
					   L"c:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", 0);
		CreateShortcut(shortcut_dir, L"Tera Term + wsl.lnk", cyglaunch_full, L"-cygwin -s \"c:/windows/system32/wsl.exe\"",
					   L"c:\\windows\\system32\\wsl.exe", 0);
	}

	if (cygwin_installed) {
		//static const wchar_t *cygwin_icon = L"C:\\cygwin64\\Cygwin.ico";
		static const wchar_t *cygwin_icon = cyglaunch_full;

		CreateShortcut(shortcut_dir, L"Tera Term + Cygwin.lnk", cyglaunch_full, L"-cygwin",
					   cygwin_icon, 0);

		WriteReg(fp,
				 L"Tera Term + Cygwin Here",
				 L"cygterm",
				 cyglaunch_full,
				 L"-nocd -v CHERE_INVOKING=y -d \"\\\"%L\\\"\"",
				 cygwin_icon);
	}

	if (msys2tarm && msys2_installed) {
		static const wchar_t *msys2_icon = L"c:\\msys64\\msys2.ico";
		static const wchar_t *mingw32_icon = L"c:\\msys64\\mingw32.ico";
		static const wchar_t *mingw64_icon = L"c:\\msys64\\mingw64.ico";

		CreateShortcut(shortcut_dir, L"Tera Term + MSYS2 MSYS.lnk", cyglaunch_full, L"-msys2 -v MSYSTEM=MSYS", msys2_icon, 0);
		CreateShortcut(shortcut_dir, L"Tera Term + MSYS2 MinGW x32.lnk", cyglaunch_full, L"-msys2 -v MSYSTEM=MINGW32", mingw32_icon, 0);
		CreateShortcut(shortcut_dir, L"Tera Term + MSYS2 MinGW x64.lnk", cyglaunch_full, L"-msys2 -v MSYSTEM=MINGW64", mingw64_icon, 0);

		WriteReg(fp,
				 L"Tera Term + MSYS2 MSYS Here",
				 L"msys2term_msys",
				 cyglaunch_full,
				 L"-msys2 -nocd -v CHERE_INVOKING=y -d \"\\\"%L\\\"\" -v MSYSTEM=MSYS",
				 msys2_icon);

		WriteReg(fp,
				 L"Tera Term + MSYS2 MinGW x32 Here",
				 L"msys2term_mingw32",
				 cyglaunch_full,
				 L"-msys2 -nocd -v CHERE_INVOKING=y -d \"\\\"%L\\\"\" -v MSYSTEM=MINGW32",
				 mingw32_icon);

		WriteReg(fp,
				 L"Tera Term + MSYS2 MinGW x64 Here",
				 L"msys2term_mingw64",
				 cyglaunch_full,
				 L"-msys2 -nocd -v CHERE_INVOKING=y -d \"\\\"%L\\\"\" -v MSYSTEM=MINGW64",
				 mingw64_icon);
	}

	if (gitbash_installed) {
		static const wchar_t *git_bash_icon = L"C:\\Program Files\\Git\\git-bash.exe";

		CreateShortcut(shortcut_dir, L"Tera Term + git_bash.lnk", cyglaunch_full, L"-gitbash", git_bash_icon, 0);

		WriteReg(fp,
				 L"Tera Term + Git Bash Here",
				 L"gitbash",
				 cyglaunch_full,
				 L"-gitbash -nocd -v CHERE_INVOKING=y -d \"\\\"%L\\\"\"",
				 git_bash_icon);
	}

	fclose(fp);

	wprintf(L"'%s' write\n", reg_fname);
	CoUninitialize();
	return 0;
}
