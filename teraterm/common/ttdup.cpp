/*
 * Copyright (C) S.Hayakawa NTT-IT 1998-2002
 * (C) 2002- TeraTerm Project
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
#define		STRICT

#include	<windows.h>
#include	<commctrl.h>
#include	<stdio.h>
#include	<string.h>
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include	<stdlib.h>
#include	<crtdbg.h>
#include	<assert.h>

#include	"ttlib.h"
#include	"codeconv.h"
#include	"win32helper.h"
#include	"asprintf.h"
#include	"codeconv.h"

#include	"ttdup.h"

#define DEFAULT_PORT_SSH	22
#define DEFAULT_PORT_TELNET	23

// TTLファイルを保存する場合定義する(デバグ用)
//#define KEEP_TTL_FILE L"C:\\tmp\\start.ttl"

static void SecureZeroStringW(wchar_t *s)
{
	size_t len = wcslen(s);
	SecureZeroMemory(s, sizeof(wchar_t) * len);
}

static void SecureZeroString(char *s)
{
	size_t len = strlen(s);
	SecureZeroMemory(s, len);
}

static void _dquote_string(const wchar_t *str, wchar_t *dst, size_t dst_len)
{
	size_t i, len, n;

	len = wcslen(str);
	n = 0;
	for (i = 0 ; i < len ; i++) {
		if (str[i] == '"')
			n++;
	}
	if (dst_len < (len + 2*n + 2 + 1))
		return;

	*dst++ = '"';
	for (i = 0 ; i < len ; i++) {
		if (str[i] == '"') {
			*dst++ = '"';
			*dst++ = '"';

		} else {
			*dst++ = str[i];

		}
	}
	*dst++ = '"';
	*dst = '\0';
}

static void dquote_stringW(const wchar_t *str, wchar_t *dst, size_t dst_len)
{
	// ",スペース,;,^A-^_ が含まれる場合にはクオートする
	if (wcschr(str, '"') != NULL ||
	    wcschr(str, ' ') != NULL ||
	    wcschr(str, ';') != NULL ||
	    wcschr(str, 0x01) != NULL ||
	    wcschr(str, 0x02) != NULL ||
	    wcschr(str, 0x03) != NULL ||
	    wcschr(str, 0x04) != NULL ||
	    wcschr(str, 0x05) != NULL ||
	    wcschr(str, 0x06) != NULL ||
	    wcschr(str, 0x07) != NULL ||
	    wcschr(str, 0x08) != NULL ||
	    wcschr(str, 0x09) != NULL ||
	    wcschr(str, 0x0a) != NULL ||
	    wcschr(str, 0x0b) != NULL ||
	    wcschr(str, 0x0c) != NULL ||
	    wcschr(str, 0x0d) != NULL ||
	    wcschr(str, 0x0e) != NULL ||
	    wcschr(str, 0x0f) != NULL ||
	    wcschr(str, 0x10) != NULL ||
	    wcschr(str, 0x11) != NULL ||
	    wcschr(str, 0x12) != NULL ||
	    wcschr(str, 0x13) != NULL ||
	    wcschr(str, 0x14) != NULL ||
	    wcschr(str, 0x15) != NULL ||
	    wcschr(str, 0x16) != NULL ||
	    wcschr(str, 0x17) != NULL ||
	    wcschr(str, 0x18) != NULL ||
	    wcschr(str, 0x19) != NULL ||
	    wcschr(str, 0x1a) != NULL ||
	    wcschr(str, 0x1b) != NULL ||
	    wcschr(str, 0x1c) != NULL ||
	    wcschr(str, 0x1d) != NULL ||
	    wcschr(str, 0x1e) != NULL ||
	    wcschr(str, 0x1f) != NULL) {
		_dquote_string(str, dst, dst_len);
		return;
	}
	// そのままコピーして戻る
	wcsncpy_s(dst, dst_len, str, _TRUNCATE);
}

#if 0
static void dquote_string(const char *str, char *dst, size_t dst_len)
{
	wchar_t *dstW = (wchar_t *)malloc(sizeof(wchar_t) * dst_len);
	wchar_t *strW = ToWcharA(str);
	dquote_stringW(strW, dstW, dst_len);
	WideCharToACP_t(dstW, dst, dst_len);
	free(strW);
	free(dstW);
}
#endif

/**
 *	フォーマットして文字列を書き込む
 *
 *	@param	hFile	ファイルハンドル
 *	@param	fmt		フォーマット文字列
 *	@retval	TRUE	書き込み成功
 *	@retval	FALSE	書き込み失敗
 */
static BOOL fwritefU8(HANDLE hFile, const wchar_t *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	wchar_t *strW;
	int r = vaswprintf(&strW, fmt, ap);
	va_end(ap);
	if (r == -1) {
		return FALSE;
	}

	char *strU8 = ToU8W(strW);
	SecureZeroStringW(strW);
	free(strW);

	DWORD len = (DWORD)strlen(strU8);
	DWORD dwWrite;
	::WriteFile(hFile, strU8, len, &dwWrite, NULL);

	SecureZeroString(strU8);
	free(strU8);

	return dwWrite == len ? TRUE : FALSE;
}

/**
 *	一時TTLファイル作成
 *
 *	@param[out]	pFile	ファイルハンドル
 *	@return				ファイル名
 */
static wchar_t *CreateTempTTL(HANDLE *pFile)
{
	wchar_t *ttl;
#if defined(KEEP_TTL_FILE)
	ttl = _wcsdup(KEEP_TTL_FILE);
#else
	wchar_t	szTempPath[MAX_PATH];
	wchar_t	szMacroFile[MAX_PATH];
	::GetTempPathW(MAX_PATH, szTempPath);
	::GetTempFileNameW(szTempPath, L"ttm", 0, szMacroFile);
	ttl = _wcsdup(szMacroFile);
#endif

	HANDLE hFile = ::CreateFileW(ttl,
								 GENERIC_WRITE,
								 FILE_SHARE_WRITE | FILE_SHARE_READ,
								 NULL,
								 CREATE_ALWAYS,
								 FILE_ATTRIBUTE_NORMAL,
								 NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		*pFile = NULL;
		return NULL;
	}

	// BOM (UTF-8)
	DWORD dwWrite;
	::WriteFile(hFile, "\xef\xbb\xbf", 3, &dwWrite, NULL);

#if !defined(KEEP_TTL_FILE)
	fwritefU8(hFile, L"filedelete '%s'\r\n", ttl);
#endif

	*pFile = hFile;
	return ttl;
}

static void MakeTTL(HANDLE hFile, const TTDupInfo *info)
{
	int port = info->port != 0 ? info->port : DEFAULT_PORT_TELNET;
	fwritefU8(hFile, L"connect '%s:%d'\r\n", info->szHostName, port);

	if (info->szUsername == NULL) {
		return;
	}

	const wchar_t *LoginPrompt = info->szLoginPrompt;
	if (LoginPrompt == NULL) {
		LoginPrompt = L"login:";
	}

	fwritefU8(hFile,
			  L"UsernamePrompt = '%s'\r\nUsername = '%s'\r\n",
			  LoginPrompt,
			  info->szUsername);
	fwritefU8(hFile,
			  L"wait   UsernamePrompt\r\nsendln Username\r\n");

	const wchar_t *password = info->szPasswordW;
	if (password != NULL) {

		const wchar_t *PasswdPrompt = info->szPasswdPrompt;
		if (PasswdPrompt == NULL) {
			PasswdPrompt = L"Password:";
		}
		fwritefU8(hFile,
				  L"PasswordPrompt = '%s'\r\nPassword = '%s'\r\n",
				  PasswdPrompt, password);
		fwritefU8(hFile,
				  L"wait   PasswordPrompt\r\nsendln Password\r\n");
	}
}

/**
 *	パスワードオプション
 *
 *	次のどちらか
 *		"/ask4passwd"
 *		"/passwd=YOUR_PASSWORD"
 *
 *	@retval	パスワードオプション文字列、不要になったfree()すること
 */
static wchar_t *GetPasswordOption(const wchar_t *szPasswordW)
{
	if (szPasswordW == NULL) {
		return _wcsdup(L"/ask4passwd");
	}
	wchar_t *szPassStrW;
	aswprintf(&szPassStrW, L"/passwd=%s", szPasswordW);

	wchar_t passwd[MAX_PATH];
	dquote_stringW(szPassStrW, passwd, _countof(passwd));

	SecureZeroStringW(szPassStrW);
	free(szPassStrW);
	szPassStrW = _wcsdup(passwd);
	return szPassStrW;
}

/**
 *	コマンドラインを作成する
 *
 *	@param[out]		arg		コマンドライン引数 (free() すること)
 *	@param[out]		ttl		TTLファイル (free() すること)
 *	@retval			エラーコード(0でエラーなし)
 */
static DWORD ConnectArgs(const TTDupInfo *info, wchar_t **arg, wchar_t **ttl)
{
	wchar_t *szMacroFile = NULL;
	wchar_t *szArgment = NULL;
	const wchar_t *add_args = info->szOption;
	DWORD retval = NO_ERROR;

	HANDLE hFile;
	szMacroFile = CreateTempTTL(&hFile);
	if (szMacroFile == NULL) {
		return ERROR_OPEN_FAILED;
	}

	// コマンドラインに "/M=[TTL FILE]" を追加
	aswprintf(&szArgment, L" /M=\"%s\"", szMacroFile);

	// ログ
	if (info->szLog != NULL) {
		fwritefU8(hFile, L"logopen '%s' 0 1\r\n", info->szLog);
	}


	switch (info->mode) {
	case TTDUP_TELNET:
		// TELNET
		MakeTTL(hFile, info);

		// /nossh SSHを使わない
		awcscat(&szArgment, L" /nossh");
		break;
	case TTDUP_SSH_CHALLENGE: {
		// keyboard-interactive
		int port = info->port != 0 ? info->port : DEFAULT_PORT_SSH;
		wchar_t *passwd_opt = GetPasswordOption(info->szPasswordW);
		fwritefU8(hFile,
				  L"connect '%s:%d /ssh /auth=challenge /user=%s %s'\r\n",
				  info->szHostName,
				  port,
				  info->szUsername,
				  passwd_opt);
		SecureZeroStringW(passwd_opt);
		free(passwd_opt);
		break;
	}
	case TTDUP_SSH_PAGEANT: {
		// Pageant
		int port = info->port != 0 ? info->port : DEFAULT_PORT_SSH;
		fwritefU8(hFile,
				  L"connect '%s:%d /ssh /auth=pageant /user=%s'\r\n",
				  info->szHostName,
				  port,
				  info->szUsername);
		break;
	}
	case TTDUP_SSH_PASSWORD: {
		// password authentication
		int port = info->port != 0 ? info->port : DEFAULT_PORT_SSH;
		wchar_t *passwd_opt = GetPasswordOption(info->szPasswordW);
		fwritefU8(hFile,
				  L"connect '%s:%d /ssh /auth=password /user=%s %s'\r\n",
				  info->szHostName,
				  port,
				  info->szUsername,
				  passwd_opt);
		SecureZeroStringW(passwd_opt);
		free(passwd_opt);
		break;
	}
	case TTDUP_SSH_PUBLICKEY: {
		// publickey
		int port = info->port != 0 ? info->port : DEFAULT_PORT_SSH;
		wchar_t *passwd_opt = GetPasswordOption(info->szPasswordW);
		wchar_t keyfile[MAX_PATH];
		dquote_stringW(info->PrivateKeyFile, keyfile, _countof(keyfile));
		fwritefU8(hFile,
				  L"connect '%s:%d /ssh /auth=publickey /user=%s %s /keyfile=%s'\r\n",
				  info->szHostName,
				  port,
				  info->szUsername,
				  passwd_opt,
				  keyfile);
		SecureZeroStringW(passwd_opt);
		free(passwd_opt);
		break;
	}
	case TTDUP_COMMANDLINE: {
		// コマンドラインですべて指定
		fwritefU8(hFile, L"connect '%s:%d %s'\r\n",
				  info->szHostName, info->port, add_args);
		add_args = NULL;
		break;
	}
	default:
		assert(FALSE);
		break;
	}

	::CloseHandle(hFile);

	if (add_args != NULL) {
		awcscat(&szArgment, add_args);
	}

	*arg = szArgment;
	*ttl = szMacroFile;
	return retval;
}

/**
 *	Tera Termを起動する
 *
 *	マクロを使って起動する
 *	コマンドラインにパスワードなどが残らない
 *
 *	@param	hInstance
 *	@param	hWnd		所有者ウィンドウへのハンドル
 *	@param	info		起動オプション
 */
DWORD ConnectHost(HINSTANCE hInstance, HWND hWnd, const TTDupInfo *info)
{
	wchar_t *szMacroFile = NULL;
	wchar_t *szArgment = NULL;
	wchar_t *szDirectory = NULL;
	wchar_t *exe_fullpath = NULL;
	DWORD retval = NO_ERROR;

	retval = ConnectArgs(info, &szArgment, &szMacroFile);
	if (retval != NO_ERROR) {
		return retval;
	}

	szDirectory = GetExeDirW(NULL);

	// フルパス化する
	aswprintf(&exe_fullpath, L"%s\\ttermpro.exe", szDirectory);

	SHELLEXECUTEINFOW ExecInfo = {};
	ExecInfo.cbSize			= sizeof(ExecInfo);
	ExecInfo.fMask			= SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	ExecInfo.hwnd			= hWnd;
	ExecInfo.lpVerb			= NULL;
	ExecInfo.lpFile			= exe_fullpath;
	ExecInfo.lpParameters	= szArgment;
	ExecInfo.lpDirectory	= szDirectory;
	ExecInfo.nShow			= SW_SHOWNORMAL;
	ExecInfo.hInstApp = hInstance;

	if (::ShellExecuteExW(&ExecInfo) == FALSE) {
		retval = ::GetLastError();
		::DeleteFileW(szMacroFile);
		goto finish;
	}

	// TODO ログダイアログを非表示オプションが必要
	if (info->szLog != NULL) {
		Sleep(500);
		HWND hLog = ::FindWindowW(NULL, L"Tera Term: Log");
		if (hLog != NULL)
			ShowWindow(hLog, SW_HIDE);
	}

finish:
	free(exe_fullpath);
	exe_fullpath = NULL;
	free(szArgment);
	szArgment = NULL;
	free(szMacroFile);
	szMacroFile = NULL;
	free(szDirectory);
	szDirectory = NULL;
	return retval;
}
