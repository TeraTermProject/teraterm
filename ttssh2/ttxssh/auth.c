/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (c) 2004- TeraTerm Project
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

#include "ttxssh.h"
#include "util.h"
#include "ssh.h"
#include "key.h"
#include "ttlib.h"
#include "dlglib.h"

#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <lmcons.h>		// for UNLEN
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>

#include "resource.h"
#include "keyfiles.h"
#include "libputty.h"
#include "tipwin.h"
#include "auth.h"
#include "helpid.h"
#include "codeconv.h"
#include "asprintf.h"
#include "win32helper.h"
#include "compat_win.h"
#include "win32helper_u8.h"

#define AUTH_START_USER_AUTH_ON_ERROR_END 1

#define MAX_AUTH_CONTROL IDC_SSHUSEPAGEANT

#define USER_PASSWORD_IS_UTF8	1

void destroy_malloced_string(char **str)
{
	if (*str != NULL) {
		SecureZeroMemory(*str, strlen(*str));
		free(*str);
		*str = NULL;
	}
}

static int auth_types_to_control_IDs[] = {
	-1, IDC_SSHUSERHOSTS, IDC_SSHUSERSA, IDC_SSHUSEPASSWORD,
	IDC_SSHUSERHOSTS, IDC_SSHUSETIS, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, IDC_SSHUSEPAGEANT, -1
};

typedef struct {
	WNDPROC ProcOrg;
	PTInstVar pvar;
	TipWin *tipwin;
	BOOL *UseControlChar;
} TPasswordControlData;

static void password_wnd_proc_close_tooltip(TPasswordControlData *data)
{
	if (data->tipwin != NULL) {
		TipWinDestroy(data->tipwin);
		data->tipwin = NULL;
	}
}

static LRESULT CALLBACK password_wnd_proc(HWND control, UINT msg,
                                          WPARAM wParam, LPARAM lParam)
{
	LRESULT result;
	TPasswordControlData *data = (TPasswordControlData *)GetWindowLongPtr(control, GWLP_USERDATA);
	switch (msg) {
	case WM_CHAR:
		if ((data->UseControlChar == NULL || *data->UseControlChar == TRUE) &&
			(GetKeyState(VK_CONTROL) & 0x8000) != 0)
		{	// 制御文字を使用する && CTRLキーが押されている
			char chars[] = { (char) wParam, 0 };

			SendMessageA(control, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM)chars);

			if (data->tipwin == NULL) {
				PTInstVar pvar = data->pvar;
				const wchar_t *UILanguageFileW = pvar->ts->UILanguageFileW;
				wchar_t *uimsg;
				RECT rect;
				GetI18nStrWW("TTSSH", "DLG_AUTH_TIP_CONTROL_CODE", L"control character is entered", UILanguageFileW, &uimsg);
				if (wParam == 'V' - 'A' + 1) {
					// CTRL + V
					wchar_t *uimsg_add;
					GetI18nStrWW("TTSSH", "DLG_AUTH_TIP_PASTE_KEY", L"Use Shift + Insert to paste from clipboard", UILanguageFileW, &uimsg_add);
					awcscats(&uimsg, L"\n", uimsg_add, NULL);
					free(uimsg_add);
				}
				GetWindowRect(control, &rect);
				data->tipwin = TipWinCreateW(hInst, control, rect.left, rect.bottom, uimsg);
				free(uimsg);
			}

			return 0;
		} else {
			password_wnd_proc_close_tooltip(data);
		}
		break;
	case WM_KILLFOCUS:
		password_wnd_proc_close_tooltip(data);
		break;
	}

	result = CallWindowProcW(data->ProcOrg, control, msg, wParam, lParam);

	if (msg == WM_NCDESTROY) {
		SetWindowLongPtr(control, GWLP_WNDPROC, (LONG_PTR)data->ProcOrg);
		password_wnd_proc_close_tooltip(data);
		free(data);
	}

	return result;
}

void init_password_control(PTInstVar pvar, HWND dlg, int item, BOOL *UseControlChar)
{
	HWND passwordControl = GetDlgItem(dlg, item);
	TPasswordControlData *data = (TPasswordControlData *)malloc(sizeof(TPasswordControlData));
	data->ProcOrg = (WNDPROC)GetWindowLongPtr(passwordControl, GWLP_WNDPROC);
	data->pvar = pvar;
	data->tipwin = NULL;
	data->UseControlChar = UseControlChar;
	SetWindowLongPtrW(passwordControl, GWLP_WNDPROC, (LONG_PTR)password_wnd_proc);
	SetWindowLongPtrW(passwordControl, GWLP_USERDATA, (LONG_PTR)data);
	SetFocus(passwordControl);
}

// controlID の認証方式を on にし、その関連コントロールを enable にする
// その他の認証方式の関連コントロールを disable にする
static void set_auth_options_status(HWND dlg, int controlID)
{
	BOOL RSA_enabled = controlID == IDC_SSHUSERSA;
	BOOL rhosts_enabled = controlID == IDC_SSHUSERHOSTS;
	BOOL TIS_enabled = controlID == IDC_SSHUSETIS;
	BOOL PAGEANT_enabled = controlID == IDC_SSHUSEPAGEANT;
	int i;
	static const int password_item_ids[] = {
		IDC_SSHPASSWORDCAPTION,
		IDC_SSHPASSWORD,
		IDC_SSHPASSWORD_OPTION,
	};
	static const int rsa_item_ids[] = {
		IDC_RSAFILENAMELABEL,
		IDC_RSAFILENAME,
		IDC_CHOOSERSAFILE,
	};
	static const int rhosts_item_ids[] = {
		IDC_LOCALUSERNAMELABEL,
		IDC_LOCALUSERNAME,
		IDC_HOSTRSAFILENAMELABEL,
		IDC_HOSTRSAFILENAME,
		IDC_CHOOSEHOSTRSAFILE,
	};

	CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, controlID);

	for (i = 0; i < _countof(password_item_ids); i++) {
		EnableWindow(GetDlgItem(dlg, password_item_ids[i]), (!TIS_enabled && !PAGEANT_enabled));
	}

	for (i = 0; i < _countof(rsa_item_ids); i++) {
		EnableWindow(GetDlgItem(dlg, rsa_item_ids[i]), RSA_enabled);
	}

	for (i = 0; i < _countof(rhosts_item_ids); i++) {
		EnableWindow(GetDlgItem(dlg, rhosts_item_ids[i]), rhosts_enabled);
	}
}

static void init_auth_machine_banner(PTInstVar pvar, HWND dlg)
{
	wchar_t* buf;
	wchar_t buf2[1024];
	const char *host_name = SSH_get_host_name(pvar);
	wchar_t *host_nameW = ToWcharA(host_name);

	GetDlgItemTextW(dlg, IDC_SSHAUTHBANNER, buf2, _countof(buf2));
	aswprintf(&buf, buf2, host_nameW);
	SetDlgItemTextW(dlg, IDC_SSHAUTHBANNER, buf);
	free(host_nameW);
	free(buf);
}

// pvar->auth_state.supported_types で有効な認証方式に関して、認証方式のラジオボタンを enable にする
// 有効な認証方式のうち、認証ダイアログで一番上のものを on にする
// on になっている認証方式に関連するコントロールを有効にする
static void update_server_supported_types(PTInstVar pvar, HWND dlg)
{
	int supported_methods = pvar->auth_state.supported_types;
	int cur_control = -1;
	int control;
	HWND focus = GetFocus();

	if (supported_methods == 0) {
		return;
	}

	for (control = IDC_SSHUSEPASSWORD; control <= MAX_AUTH_CONTROL;
	     control++) {
		BOOL enabled = FALSE;
		int method;
		HWND item = GetDlgItem(dlg, control);

		if (item != NULL) {
			for (method = 0; method <= SSH_AUTH_MAX; method++) {
				if (auth_types_to_control_IDs[method] == control
				    && (supported_methods & (1 << method)) != 0) {
				    enabled = TRUE;
				}
			}

			EnableWindow(item, enabled);

			if (IsDlgButtonChecked(dlg, control)) {
				cur_control = control;
			}
		}
	}

	if (cur_control >= 0) {
		if (!IsWindowEnabled(GetDlgItem(dlg, cur_control))) {
			do {
				cur_control++;
				if (cur_control > MAX_AUTH_CONTROL) {
					cur_control = IDC_SSHUSEPASSWORD;
				}
			} while (!IsWindowEnabled(GetDlgItem(dlg, cur_control)));

			set_auth_options_status(dlg, cur_control);

			if (focus != NULL && !IsWindowEnabled(focus)) {
				SetFocus(GetDlgItem(dlg, cur_control));
			}
		}
	}
}

/**
 * GetUserNameW()
 *
 *	TODO win32helper に移動
 */
static DWORD hGetUserNameW(wchar_t **username)
{
	wchar_t name[UNLEN+1];
	DWORD len = _countof(name);
	BOOL r = GetUserNameW(name, &len);
	if (r == 0) {
		*username = NULL;
		return GetLastError();
	}
	*username = _wcsdup(name);
	return NO_ERROR;
}

static void init_auth_dlg(PTInstVar pvar, HWND dlg, BOOL *UseControlChar)
{
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_AUTH_TITLE" },
		{ IDC_SSHAUTHBANNER, "DLG_AUTH_BANNER" },
		{ IDC_SSHAUTHBANNER2, "DLG_AUTH_BANNER2" },
		{ IDC_SSHUSERNAMELABEL, "DLG_AUTH_USERNAME" },
		{ IDC_SSHPASSWORDCAPTION, "DLG_AUTH_PASSWORD" },
		{ IDC_REMEMBER_PASSWORD, "DLG_AUTH_REMEMBER_PASSWORD" },
		{ IDC_FORWARD_AGENT, "DLG_AUTH_FWDAGENT" },
		{ IDC_SSHAUTHMETHOD, "DLG_AUTH_METHOD" },
		{ IDC_SSHUSEPASSWORD, "DLG_AUTH_METHOD_PASSWORD" },
		{ IDC_SSHUSERSA, "DLG_AUTH_METHOD_RSA" },
		{ IDC_SSHUSERHOSTS, "DLG_AUTH_METHOD_RHOST" },
		{ IDC_SSHUSEPAGEANT, "DLG_AUTH_METHOD_PAGEANT" },
		{ IDC_RSAFILENAMELABEL, "DLG_AUTH_PRIVATEKEY" },
		{ IDC_LOCALUSERNAMELABEL, "DLG_AUTH_LOCALUSER" },
		{ IDC_HOSTRSAFILENAMELABEL, "DLG_AUTH_HOST_PRIVATEKEY" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_DISCONNECT" },
	};
	// デフォルト認証方式
	// 認証失敗時は直前の(失敗した)認証方式
	SSHAuthMethod method = pvar->session_settings.DefaultAuthMethod;
	const wchar_t *UILanguageFileW = pvar->ts->UILanguageFileW;
	int focus_id;

	SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), UILanguageFileW);

	init_auth_machine_banner(pvar, dlg);
	init_password_control(pvar, dlg, IDC_SSHPASSWORD, UseControlChar);
	focus_id = IDC_SSHPASSWORD;

	// 認証失敗後はラベルを書き換え
	if (pvar->auth_state.failed_method != SSH_AUTH_NONE) {
		/* must be retrying a failed attempt */
		wchar_t *uimsg;
		if (pvar->auth_state.partial_success) {
			GetI18nStrWW("TTSSH", "DLG_AUTH_BANNER2_FURTHER", L"Further authentication required.", UILanguageFileW, &uimsg);
		} else {
			GetI18nStrWW("TTSSH", "DLG_AUTH_BANNER2_FAILED", L"Authentication failed. Please retry.", UILanguageFileW, &uimsg);
		}
		SetDlgItemTextW(dlg, IDC_SSHAUTHBANNER2, uimsg);
		free(uimsg);
		GetI18nStrWW("TTSSH", "DLG_AUTH_TITLE_FAILED", L"Retrying SSH Authentication", UILanguageFileW, &uimsg);
		SetWindowTextW(dlg, uimsg);
		free(uimsg);
		method = pvar->auth_state.failed_method;
	}

	// パスワードを覚えておくチェックボックスにはデフォルトで有効とする
	if (pvar->ts_SSH->remember_password) {
		SendMessage(GetDlgItem(dlg, IDC_REMEMBER_PASSWORD), BM_SETCHECK, BST_CHECKED, 0);
	} else {
		SendMessage(GetDlgItem(dlg, IDC_REMEMBER_PASSWORD), BM_SETCHECK, BST_UNCHECKED, 0);
	}

	// ForwardAgent の設定を反映する
	CheckDlgButton(dlg, IDC_FORWARD_AGENT, pvar->settings.ForwardAgent);

	// SSH バージョンによって TIS のラベルを書き換え
	{
		const char *key;
		const wchar_t *def;
		wchar_t *uimsg;
		if (pvar->settings.ssh_protocol_version == 1) {
			key = "DLG_AUTH_METHOD_CHALLENGE1";
			def = L"Use challenge/response(&TIS) to log in";
		} else {
			key = "DLG_AUTH_METHOD_CHALLENGE2";
			def = L"Use keyboard-&interactive to log in";
		}
		GetI18nStrWW("TTSSH", key, def, UILanguageFileW, &uimsg);
		SetDlgItemTextW(dlg, IDC_SSHUSETIS, uimsg);
		free(uimsg);
	}

	if (pvar->auth_state.user != NULL) {
#if defined(USER_PASSWORD_IS_UTF8)
		SetDlgItemTextU8(dlg, IDC_SSHUSERNAME, pvar->auth_state.user);
#else
		SetDlgItemTextA(dlg, IDC_SSHUSERNAME, pvar->auth_state.user);
#endif
		EnableWindow(GetDlgItem(dlg, IDC_SSHUSERNAME), FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_USERNAME_OPTION), FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_SSHUSERNAMELABEL), FALSE);
	}
	else if (strlen(pvar->ssh2_username) > 0) {
		SetDlgItemText(dlg, IDC_SSHUSERNAME, pvar->ssh2_username);
		if (pvar->ssh2_autologin == 1) {
			EnableWindow(GetDlgItem(dlg, IDC_SSHUSERNAME), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_USERNAME_OPTION), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_SSHUSERNAMELABEL), FALSE);
		}
	}
	else {
		switch(pvar->session_settings.DefaultUserType) {
		case 0:
			// 入力しない
			break;
		case 1:
			// use DefaultUserName
			if (pvar->session_settings.DefaultUserName[0] == 0) {
				// 「入力しない」にしておく
				pvar->session_settings.DefaultUserType = 0;
			} else {
				SetDlgItemTextW(dlg, IDC_SSHUSERNAME, pvar->session_settings.DefaultUserName);
			}
			break;
		case 2: {
			wchar_t *user_name;
			hGetUserNameW(&user_name);
			SetDlgItemTextW(dlg, IDC_SSHUSERNAME, user_name);
			free(user_name);
			break;
		}
		default:
			// 入力しないにしておく
			pvar->session_settings.DefaultUserType = 0;
		}
	}

	if (strlen(pvar->ssh2_password) > 0) {
#if defined(USER_PASSWORD_IS_UTF8)
		SetDlgItemTextU8(dlg, IDC_SSHPASSWORD, pvar->ssh2_password);
#else
		SetDlgItemTextA(dlg, IDC_SSHPASSWORD, pvar->ssh2_password);
#endif
		if (pvar->ssh2_autologin == 1) {
			EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORDCAPTION), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD_OPTION), FALSE);
			focus_id = IDCANCEL;
		}
	}

	SetDlgItemTextW(dlg, IDC_RSAFILENAME,
	                pvar->session_settings.DefaultRSAPrivateKeyFile);
	SetDlgItemTextW(dlg, IDC_HOSTRSAFILENAME,
	               pvar->session_settings.DefaultRhostsHostPrivateKeyFile);
	SetDlgItemText(dlg, IDC_LOCALUSERNAME,
	               pvar->session_settings.DefaultRhostsLocalUserName);

	// /auth=passsword
	if (pvar->ssh2_authmethod == SSH_AUTH_PASSWORD) {
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, IDC_SSHUSEPASSWORD);

	}
	// /auth=publickey
	else if (pvar->ssh2_authmethod == SSH_AUTH_RSA) {
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, IDC_SSHUSERSA);

		SetDlgItemTextW(dlg, IDC_RSAFILENAME, pvar->ssh2_keyfile);
		if (pvar->ssh2_autologin == 1) {
			EnableWindow(GetDlgItem(dlg, IDC_CHOOSERSAFILE), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_RSAFILENAME), FALSE);
		}

	// /auth=challenge
	} else if (pvar->ssh2_authmethod == SSH_AUTH_TIS) {
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, IDC_SSHUSETIS);
		EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD), FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD_OPTION), FALSE);
		SetDlgItemText(dlg, IDC_SSHPASSWORD, "");
		focus_id = IDOK;

	// /auth=pageant
	} else if (pvar->ssh2_authmethod == SSH_AUTH_PAGEANT) {
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, IDC_SSHUSEPAGEANT);
		EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD), FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD_OPTION), FALSE);
		SetDlgItemTextA(dlg, IDC_SSHPASSWORD, "");
		focus_id = IDOK;

	}
	// 認証方式の指定がない
	else {
		set_auth_options_status(dlg, auth_types_to_control_IDs[method]);

		update_server_supported_types(pvar, dlg);

		if (method == SSH_AUTH_TIS) {
			focus_id = IDOK;
		}
		else if (method == SSH_AUTH_PAGEANT) {
			focus_id = IDOK;
		}
	}

	if (GetWindowTextLength(GetDlgItem(dlg, IDC_SSHUSERNAME)) == 0) {
		focus_id = IDC_SSHUSERNAME;
	}
	else if (pvar->ask4passwd == 1) {
		focus_id = IDC_SSHPASSWORD;
	}

	// '/I' 指定があるときのみ最小化する
	if (pvar->ts->Minimize) {
		ShowWindow(dlg,SW_MINIMIZE);
	}

	// フォーカスをセットする
	//SetFocus(GetDlgItem(dlg, focus_id));
	PostMessage(dlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(dlg, focus_id), TRUE);
}

static DWORD hGetDlgItemTextAorU8(HWND hDlg, int id, char **text)
{
#if defined(USER_PASSWORD_IS_UTF8)
	return hGetDlgItemTextU8(hDlg, id, text);
#else
	return hGetDlgItemTextA(hDlg, id, text);
#endif
}

/**
 *	ファイル名を返す
 *	@retval		ファイル名(不要になったらfree()すること)
 *	@retval		NULL キャンセルが押された
 */
static wchar_t *get_key_file_name(HWND parent, const wchar_t *UILanguageFileW)
{
	static const wchar_t *fullname_def = L"identity";
	static const wchar_t *filter_def =
		L"identity files\\0identity;id_rsa;id_dsa;id_ecdsa;id_ed25519;*.ppk;*.pem\\0"
		L"identity(RSA1)\\0identity\\0"
		L"id_rsa(SSH2)\\0id_rsa\\0"
		L"id_dsa(SSH2)\\0id_dsa\\0"
		L"id_ecdsa(SSH2)\\0id_ecdsa\\0"
		L"id_ed25519(SSH2)\\0id_ed25519\\0"
		L"PuTTY(*.ppk)\\0*.ppk\\0"
		L"PEM files(*.pem)\\0*.pem\\0"
		L"all(*.*)\\0*.*\\0"
		L"\\0";
	TTOPENFILENAMEW params;
	wchar_t *filter;
	wchar_t *title;
	wchar_t *fullnameW;
	BOOL r;

	ZeroMemory(&params, sizeof(params));
	params.hwndOwner = parent;
	GetI18nStrWW("TTSSH", "FILEDLG_OPEN_PRIVATEKEY_FILTER", filter_def,
				 UILanguageFileW, &filter);
	params.lpstrFilter = filter;
	params.nFilterIndex = 0;
	params.lpstrFile = fullname_def;
	params.lpstrInitialDir = NULL;
	GetI18nStrWW("TTSSH", "FILEDLG_OPEN_PRIVATEKEY_TITLE",
				 L"Choose a file with the RSA/DSA/ECDSA/ED25519 private key",
				 UILanguageFileW, &title);
	params.lpstrTitle = title;
	params.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	params.lpstrDefExt = NULL;

	r = TTGetOpenFileNameW(&params, &fullnameW);
	free(filter);
	free(title);
	if (r == FALSE) {
		return NULL;
	}
	else {
		return fullnameW;
	}
}

static void choose_RSA_key_file(HWND dlg, PTInstVar pvar)
{
	wchar_t *filename;

	filename = get_key_file_name(dlg, pvar->ts->UILanguageFileW);
	if (filename != NULL) {
		SetDlgItemTextW(dlg, IDC_RSAFILENAME, filename);
		free(filename);
	}
}

static void choose_host_RSA_key_file(HWND dlg, PTInstVar pvar)
{
	wchar_t *filename;

	filename = get_key_file_name(dlg, pvar->ts->UILanguageFileW);
	if (filename != NULL) {
		SetDlgItemTextW(dlg, IDC_HOSTRSAFILENAME, filename);
		free(filename);
	}
}

static BOOL end_auth_dlg(PTInstVar pvar, HWND dlg)
{
	SSHAuthMethod method = SSH_AUTH_PASSWORD;
	char *password;
	Key *key_pair = NULL;

	hGetDlgItemTextAorU8(dlg, IDC_SSHPASSWORD, &password);
	if (IsDlgButtonChecked(dlg, IDC_SSHUSERSA)) {
		method = SSH_AUTH_RSA;
	} else if (IsDlgButtonChecked(dlg, IDC_SSHUSERHOSTS)) {
		if (GetWindowTextLength(GetDlgItem(dlg, IDC_HOSTRSAFILENAME)) > 0) {
			method = SSH_AUTH_RHOSTS_RSA;
		} else {
			method = SSH_AUTH_RHOSTS;
		}
	} else if (IsDlgButtonChecked(dlg, IDC_SSHUSETIS)) {
		method = SSH_AUTH_TIS;
	} else if (IsDlgButtonChecked(dlg, IDC_SSHUSEPAGEANT)) {
		method = SSH_AUTH_PAGEANT;
	}

	if (method == SSH_AUTH_RSA || method == SSH_AUTH_RHOSTS_RSA) {
		wchar_t keyfile[2048];
		int file_ctl_ID =
			method == SSH_AUTH_RSA ? IDC_RSAFILENAME : IDC_HOSTRSAFILENAME;

		keyfile[0] = 0;
		GetDlgItemTextW(dlg, file_ctl_ID, keyfile, _countof(keyfile));
		if (keyfile[0] == 0) {
			UTIL_get_lang_msg("MSG_KEYSPECIFY_ERROR", pvar,
			                  "You must specify a file containing the RSA/DSA/ECDSA/ED25519 private key.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
			SetFocus(GetDlgItem(dlg, file_ctl_ID));
			destroy_malloced_string(&password);
			return FALSE;
		}

		if (SSHv1(pvar)) {
			BOOL invalid_passphrase = FALSE;

			key_pair = KEYFILES_read_private_key(pvar, keyfile, password,
			                                     &invalid_passphrase,
			                                     FALSE);

			if (key_pair == NULL) {
				if (invalid_passphrase) {
					HWND passwordCtl = GetDlgItem(dlg, IDC_SSHPASSWORD);

					SetFocus(passwordCtl);
					SendMessage(passwordCtl, EM_SETSEL, 0, -1);
				} else {
					SetFocus(GetDlgItem(dlg, file_ctl_ID));
				}
				destroy_malloced_string(&password);
				return FALSE;
			}

		} else { // SSH2(yutaka)
			BOOL invalid_passphrase = FALSE;
			char errmsg[256];
			FILE *fp = NULL;
			ssh2_keyfile_type keyfile_type;

			memset(errmsg, 0, sizeof(errmsg));

			keyfile_type = get_ssh2_keytype(keyfile, &fp, errmsg, sizeof(errmsg));
			switch (keyfile_type) {
				case SSH2_KEYFILE_TYPE_OPENSSH:
				{
					key_pair = read_SSH2_private_key(pvar, fp, password,
					                                 &invalid_passphrase,
					                                 FALSE,
					                                 errmsg,
					                                 sizeof(errmsg)
					                                );
					break;
				}
				case SSH2_KEYFILE_TYPE_PUTTY:
				{
					key_pair = read_SSH2_PuTTY_private_key(pvar, fp, password,
					                                       &invalid_passphrase,
					                                       FALSE,
					                                       errmsg,
					                                       sizeof(errmsg)
					                                      );
					break;
				}
				case SSH2_KEYFILE_TYPE_SECSH:
				{
					key_pair = read_SSH2_SECSH_private_key(pvar, fp, password,
					                                       &invalid_passphrase,
					                                       FALSE,
					                                       errmsg,
					                                       sizeof(errmsg)
					                                      );
					break;
				}
				default:
				{
					char buf[1024];

					// ファイルが開けた場合はファイル形式が不明でも読み込んでみる
					if (fp != NULL) {
						key_pair = read_SSH2_private_key(pvar, fp, password,
						                                 &invalid_passphrase,
						                                 FALSE,
						                                 errmsg,
						                                 sizeof(errmsg)
						                                );
						break;
					}

					UTIL_get_lang_msg("MSG_READKEY_ERROR", pvar,
					                  "read error SSH2 private key file\r\n%s");
					_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg, errmsg);
					notify_nonfatal_error(pvar, buf);
					// ここに来たということは SSH2 秘密鍵ファイルが開けないので
					// 鍵ファイルの選択ボタンにフォーカスを移す
					SetFocus(GetDlgItem(dlg, IDC_CHOOSERSAFILE));
					destroy_malloced_string(&password);
					return FALSE;
				}
			}

			if (key_pair == NULL) { // read error
				char buf[1024];
				UTIL_get_lang_msg("MSG_READKEY_ERROR", pvar,
				                  "read error SSH2 private key file\r\n%s");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg, errmsg);
				notify_nonfatal_error(pvar, buf);
				// パスフレーズが鍵と一致しなかった場合はIDC_SSHPASSWORDにフォーカスを移す (2006.10.29 yasuhide)
				if (invalid_passphrase) {
					HWND passwordCtl = GetDlgItem(dlg, IDC_SSHPASSWORD);

					SetFocus(passwordCtl);
					SendMessage(passwordCtl, EM_SETSEL, 0, -1);
				} else {
					SetFocus(GetDlgItem(dlg, file_ctl_ID));
				}
				destroy_malloced_string(&password);
				return FALSE;
			}

		}

	}
	else if (method == SSH_AUTH_PAGEANT) {
		pvar->pageant_key = NULL;
		pvar->pageant_curkey = NULL;
		pvar->pageant_keylistlen = 0;
		pvar->pageant_keycount = 0;
		pvar->pageant_keycurrent = 0;
		pvar->pageant_keyfinal=FALSE;

		// Pageant と通信
		if (!putty_agent_exists()) {
			UTIL_get_lang_msg("MSG_PAGEANT_NOTFOUND", pvar,
			                  "Can't find Pageant.");
			notify_nonfatal_error(pvar, pvar->UIMsg);

			return FALSE;
		}

		if (SSHv1(pvar)) {
			pvar->pageant_keylistlen = putty_get_ssh1_keylist(&pvar->pageant_key);
		}
		else {
			pvar->pageant_keylistlen = putty_get_ssh2_keylist(&pvar->pageant_key);
		}
		if (pvar->pageant_keylistlen == 0) {
			UTIL_get_lang_msg("MSG_PAGEANT_NOKEY", pvar,
			                  "Pageant has no valid key.");
			notify_nonfatal_error(pvar, pvar->UIMsg);

			return FALSE;
		}
		pvar->pageant_curkey = pvar->pageant_key;

		// 鍵の数
		pvar->pageant_keycount = get_uint32_MSBfirst(pvar->pageant_curkey);
		if (pvar->pageant_keycount == 0) {
			UTIL_get_lang_msg("MSG_PAGEANT_NOKEY", pvar,
			                  "Pageant has no valid key.");
			notify_nonfatal_error(pvar, pvar->UIMsg);

			return FALSE;
		}
		pvar->pageant_curkey += 4;
	}

	/* from here on, we cannot fail, so just munge cur_cred in place */
	pvar->auth_state.cur_cred.method = method;
	pvar->auth_state.cur_cred.key_pair = key_pair;
	/* we can't change the user name once it's set. It may already have
	   been sent to the server, and it can only be sent once. */
	if (pvar->auth_state.user == NULL) {
		hGetDlgItemTextAorU8(dlg, IDC_SSHUSERNAME, &pvar->auth_state.user);
	}

	// パスワードの保存をするかどうかを決める (2006.8.3 yutaka)
	if (SendMessage(GetDlgItem(dlg, IDC_REMEMBER_PASSWORD), BM_GETCHECK, 0,0) == BST_CHECKED) {
		pvar->settings.remember_password = 1;  // 覚えておく
		pvar->ts_SSH->remember_password = 1;
	} else {
		pvar->settings.remember_password = 0;  // ここですっかり忘れる
		pvar->ts_SSH->remember_password = 0;
	}

	// - パスワード認証の場合 pvar->auth_state.cur_cred.password が認証に使われる
	// - 公開鍵認証の場合 pvar->auth_state.cur_cred.password は認証に使われないが、
	//   セッション複製時にパスフレーズを使い回したいので解放しないようにする。
	if (method == SSH_AUTH_PASSWORD || method == SSH_AUTH_RSA) {
		pvar->auth_state.cur_cred.password = password;
	} else {
		destroy_malloced_string(&password);
	}

	if (method == SSH_AUTH_RHOSTS || method == SSH_AUTH_RHOSTS_RSA) {
		if (pvar->session_settings.DefaultAuthMethod != SSH_AUTH_RHOSTS) {
			UTIL_get_lang_msg("MSG_RHOSTS_NOTDEFAULT_ERROR", pvar,
			                  "Rhosts authentication will probably fail because it was not "
			                  "the default authentication method.\n"
			                  "To use Rhosts authentication "
			                  "in TTSSH, you need to set it to be the default by restarting\n"
			                  "TTSSH and selecting \"SSH Authentication...\" from the Setup menu "
			                  "before connecting.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
		}

		hGetDlgItemTextAorU8(dlg, IDC_LOCALUSERNAME,
							 &pvar->auth_state.cur_cred.rhosts_client_user);
	}
	pvar->auth_state.auth_dialog = NULL;

	GetDlgItemTextW(dlg, IDC_RSAFILENAME,
					pvar->session_settings.DefaultRSAPrivateKeyFile,
					_countof(pvar->session_settings.DefaultRSAPrivateKeyFile));
	GetDlgItemTextW(dlg, IDC_HOSTRSAFILENAME,
	                pvar->session_settings.DefaultRhostsHostPrivateKeyFile,
					_countof(pvar->session_settings.DefaultRhostsHostPrivateKeyFile));
	GetDlgItemText(dlg, IDC_LOCALUSERNAME,
	               pvar->session_settings.DefaultRhostsLocalUserName,
	               sizeof(pvar->session_settings.
	              DefaultRhostsLocalUserName));

	if (SSHv1(pvar)) {
		SSH_notify_user_name(pvar);
		SSH_notify_cred(pvar);
	} else {
		// for SSH2(yutaka)
		do_SSH2_userauth(pvar);
	}

	TTEndDialog(dlg, 1);

	return TRUE;
}

static INT_PTR CALLBACK auth_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
                                      LPARAM lParam)
{
	const int IDC_TIMER1 = 300; // 自動ログインが有効なとき
	const int IDC_TIMER2 = 301; // サポートされているメソッドを自動チェック(CheckAuthListFirst)
	const int IDC_TIMER3 = 302; // challenge で ask4passwd でCheckAuthListFirst が FALSE のとき
	const int autologin_timeout = 10; // ミリ秒
	PTInstVar pvar;
	static BOOL autologin_sent_none;
	static BOOL UseControlChar;
	static BOOL ShowPassPhrase;
	static size_t username_str_len;
	static wchar_t password_char;	// 伏せ字キャラクタ

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		pvar->auth_state.auth_dialog = dlg;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		UseControlChar = TRUE;
		ShowPassPhrase = FALSE;
		username_str_len = 0;
		password_char = 0;
		init_auth_dlg(pvar, dlg, &UseControlChar);

		// "▼"画像をセットする
		SetDlgItemIcon(dlg, IDC_USERNAME_OPTION, MAKEINTRESOURCEW(IDI_DROPDOWN), 16, 16);
		SetDlgItemIcon(dlg, IDC_SSHPASSWORD_OPTION, MAKEINTRESOURCEW(IDI_DROPDOWN), 16, 16);

		// SSH2 autologinが有効の場合は、タイマを仕掛ける。 (2004.12.1 yutaka)
		if (pvar->ssh2_autologin == 1) {
			autologin_sent_none = FALSE;
			SetTimer(dlg, IDC_TIMER1, autologin_timeout, 0);
		}
		else {
			// サポートされているメソッドをチェックする。(2007.9.24 maya)
			// 設定が有効で、まだ取りに行っておらず、ユーザ名が確定している
			if (pvar->session_settings.CheckAuthListFirst &&
			    !pvar->tryed_ssh2_authlist &&
			    GetWindowTextLength(GetDlgItem(dlg, IDC_SSHUSERNAME)) > 0) {
				SetTimer(dlg, IDC_TIMER2, autologin_timeout, 0);
			}
			// /auth=challenge と /ask4passwd が指定されていてユーザ名が確定している
			// 場合は、OK ボタンを押して TIS auth ダイアログを出す
			else if (pvar->ssh2_authmethod == SSH_AUTH_TIS &&
			         pvar->ask4passwd &&
			         GetWindowTextLength(GetDlgItem(dlg, IDC_SSHUSERNAME)) > 0) {
				SetTimer(dlg, IDC_TIMER3, autologin_timeout, 0);
			}
		}
		CenterWindow(dlg, GetParent(dlg));
		return FALSE;			/* because we set the focus */

	case WM_TIMER:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);
		// 認証準備ができてから、認証データを送信する。早すぎると、落ちる。(2004.12.16 yutaka)
		if (wParam == IDC_TIMER1) {
			// 自動ログインのため
			if (!(pvar->ssh_state.status_flags & STATUS_DONT_SEND_USER_NAME) &&
			    (pvar->ssh_state.status_flags & STATUS_HOST_OK)) {
				if (SSHv2(pvar) &&
				    pvar->session_settings.CheckAuthListFirst &&
				    !pvar->tryed_ssh2_authlist) {
					if (!autologin_sent_none) {
						autologin_sent_none = TRUE;

						// ダイアログのユーザ名を取得する
						if (pvar->auth_state.user == NULL) {
							hGetDlgItemTextAorU8(dlg, IDC_SSHUSERNAME, &pvar->auth_state.user);
						}

						// CheckAuthListFirst が TRUE のときは AuthList が帰ってきていないと
						// IDOK を押しても進まないので、認証メソッド none を送る (2008.10.12 maya)
						do_SSH2_userauth(pvar);
					}
					//else {
					//	none を送ってから帰ってくるまで待つ
					//}
				}
				else {
					// SSH1 のとき
					// または CheckAuthListFirst が FALSE のとき
					// または CheckAuthListFirst TRUE で、authlist が帰ってきたあと
					KillTimer(dlg, IDC_TIMER1);

					// ダイアログのユーザ名を取得する
					if (pvar->auth_state.user == NULL) {
						hGetDlgItemTextAorU8(dlg, IDC_SSHUSERNAME, &pvar->auth_state.user);
					}

					SendMessage(dlg, WM_COMMAND, IDOK, 0);
				}
			}
		}
		else if (wParam == IDC_TIMER2) {
			// authlist を得るため
			if (!(pvar->ssh_state.status_flags & STATUS_DONT_SEND_USER_NAME) &&
			    (pvar->ssh_state.status_flags & STATUS_HOST_OK)) {
				if (SSHv2(pvar)) {
					KillTimer(dlg, IDC_TIMER2);

					// ダイアログのユーザ名を取得する
					if (pvar->auth_state.user == NULL) {
						hGetDlgItemTextAorU8(dlg, IDC_SSHUSERNAME, &pvar->auth_state.user);
					}

					// ユーザ名を変更させない
					EnableWindow(GetDlgItem(dlg, IDC_SSHUSERNAME), FALSE);
					EnableWindow(GetDlgItem(dlg, IDC_USERNAME_OPTION), FALSE);

					// 認証メソッド none を送る
					do_SSH2_userauth(pvar);

					// TIS 用に OK を押すのは認証に失敗したあとにしないと
					// Unexpected SSH2 message になる。
				}
				else if (SSHv1(pvar)) {
					KillTimer(dlg, IDC_TIMER2);

					// TIS 用に OK を押す
					if (pvar->ssh2_authmethod == SSH_AUTH_TIS) {
						SendMessage(dlg, WM_COMMAND, IDOK, 0);
					}
					// SSH1 では認証メソッド none を送らない
				}
				// プロトコルバージョン確定前は何もしない
			}
		}
		else if (wParam == IDC_TIMER3) {
			if (!(pvar->ssh_state.status_flags & STATUS_DONT_SEND_USER_NAME) &&
			    (pvar->ssh_state.status_flags & STATUS_HOST_OK)) {
				if (SSHv2(pvar) || SSHv1(pvar)) {
					KillTimer(dlg, IDC_TIMER3);

					// TIS 用に OK を押す
					SendMessage(dlg, WM_COMMAND, IDOK, 0);
				}
				// プロトコルバージョン確定前は何もしない
			}
		}
		return FALSE;

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDOK:
			// 認証中にサーバから切断された場合は、キャンセル扱いとする。(2014.3.31 yutaka)
			if (!pvar->cv->Ready) {
				goto canceled;
			}

			// 認証準備ができてから、認証データを送信する。早すぎると、落ちる。(2001.1.25 yutaka)
			if (pvar->userauth_retry_count == 0 &&
				((pvar->ssh_state.status_flags & STATUS_DONT_SEND_USER_NAME) ||
				 !(pvar->ssh_state.status_flags & STATUS_HOST_OK))) {
				return FALSE;
			}
			else if (SSHv2(pvar) &&
			         pvar->session_settings.CheckAuthListFirst &&
			         !pvar->tryed_ssh2_authlist) {
				// CheckAuthListFirst が有効で認証方式が来ていないときは
				// OK を押せないようにする (2008.10.4 maya)
				return FALSE;
			}

			return end_auth_dlg(pvar, dlg);

		case IDCANCEL:			/* kill the connection */
canceled:
			pvar->auth_state.auth_dialog = NULL;
			notify_closed_connection(pvar, "authentication cancelled");
			TTEndDialog(dlg, 0);
			return TRUE;

		case IDCLOSE:
			// 認証中にネットワーク切断された場合、当該メッセージでダイアログを閉じる。
			pvar->auth_state.auth_dialog = NULL;
			TTEndDialog(dlg, 0);
			return TRUE;

		case IDC_SSHUSERNAME:
			switch (HIWORD(wParam)) {
			case EN_KILLFOCUS: {
				// ユーザ名がフォーカスを失ったとき (2007.9.29 maya)
				if (!(pvar->ssh_state.status_flags & STATUS_DONT_SEND_USER_NAME) &&
					(pvar->ssh_state.status_flags & STATUS_HOST_OK)) {
					// 設定が有効でまだ取りに行っていないなら
					if (SSHv2(pvar) &&
						pvar->session_settings.CheckAuthListFirst &&
						!pvar->tryed_ssh2_authlist) {
						// ダイアログのユーザ名を反映
						if (pvar->auth_state.user == NULL) {
							hGetDlgItemTextAorU8(dlg, IDC_SSHUSERNAME, &pvar->auth_state.user);
						}

						// ユーザ名が入力されているかチェックする
						if (strlen(pvar->auth_state.user) == 0) {
							return FALSE;
						}

						// ユーザ名を変更させない
						EnableWindow(GetDlgItem(dlg, IDC_SSHUSERNAME), FALSE);
						EnableWindow(GetDlgItem(dlg, IDC_USERNAME_OPTION), FALSE);

						// 認証メソッド none を送る
						do_SSH2_userauth(pvar);
						return TRUE;
					}
				}
				return FALSE;
			}
			case EN_CHANGE: {
				// ユーザー名が入力されていた場合、オプションを使うことはないので、
				// tabでのフォーカス移動時、オプションボタンをパスするようにする
				// 従来と同じキー操作でユーザー名とパスフレーズを入力可能とする
				HWND hWnd = (HWND)lParam;
				const int len = GetWindowTextLength(hWnd);
				if ((username_str_len == 0 && len != 0) ||
					(username_str_len != 0 && len == 0)) {
					// ユーザー名の文字長が 0になる or 0ではなくなる 時のみ処理
					const HWND hWndOption = GetDlgItem(dlg, IDC_USERNAME_OPTION);
					LONG_PTR style = GetWindowLongPtr(hWndOption, GWL_STYLE);

					if (len > 0) {
						// 不要tabstop
						style = style & (~(LONG_PTR)WS_TABSTOP);
					}
					else {
						// 要tabstop
						style = style | WS_TABSTOP;
					}
					SetWindowLongPtr(hWndOption, GWL_STYLE, style);
				}
				username_str_len = len;
				return FALSE;
			}
			}
			return FALSE;

		case IDC_SSHUSEPASSWORD:
		case IDC_SSHUSERSA:
		case IDC_SSHUSERHOSTS:
		case IDC_SSHUSETIS:
		case IDC_SSHUSEPAGEANT:
			set_auth_options_status(dlg, LOWORD(wParam));
			return TRUE;

		case IDC_CHOOSERSAFILE:
			choose_RSA_key_file(dlg, pvar);
			return TRUE;

		case IDC_CHOOSEHOSTRSAFILE:
			choose_host_RSA_key_file(dlg, pvar);
			return TRUE;

		case IDC_FORWARD_AGENT:
			// このセッションにのみ反映される (2008.12.4 maya)
			pvar->session_settings.ForwardAgent = IsDlgButtonChecked(dlg, IDC_FORWARD_AGENT);
			return TRUE;

		case IDC_SSHPASSWORD_OPTION: {
			RECT rect;
			HWND hWndButton;
			int result;
			HMENU hMenu= CreatePopupMenu();
			wchar_t *clipboard = GetClipboardTextW(dlg, FALSE);
			wchar_t *uimsg;
			GetI18nStrWW("TTSSH", "DLG_AUTH_PASTE_CLIPBOARD",
						 L"Paste from &clipboard",
						 pvar->ts->UILanguageFileW, &uimsg);
			AppendMenuW(hMenu, MF_ENABLED | MF_STRING | (clipboard == NULL ? MFS_DISABLED : 0), 1, uimsg);
			free(uimsg);
			GetI18nStrWW("ttssh", "DLG_AUTH_CLEAR_CLIPBOARD",
						 L"Paste from &clipboard and cl&ear clipboard",
						 pvar->ts->UILanguageFileW, &uimsg);
			AppendMenuW(hMenu, MF_ENABLED | MF_STRING | (clipboard == NULL ? MFS_DISABLED : 0), 2, uimsg);
			free(uimsg);
			GetI18nStrWW("ttssh", "DLG_AUTH_USE_CONTORL_CHARACTERS",
						 L"Use control charac&ters",
						 pvar->ts->UILanguageFileW, &uimsg);
			AppendMenuW(hMenu, MF_ENABLED | MF_STRING  | (UseControlChar ? MFS_CHECKED : 0), 3, uimsg);
			free(uimsg);
			GetI18nStrWW("ttssh", "DLG_AUTH_SHOW_PASSPHRASE",
						 L"&Show passphrase",
						 pvar->ts->UILanguageFileW, &uimsg);
			AppendMenuW(hMenu, MF_ENABLED | MF_STRING | (ShowPassPhrase ? MFS_CHECKED : 0), 4, uimsg);
			free(uimsg);
			if (clipboard != NULL) {
				free(clipboard);
			}
			hWndButton = GetDlgItem(dlg, IDC_SSHPASSWORD_OPTION);
			GetWindowRect(hWndButton, &rect);
			result = TrackPopupMenu(hMenu, TPM_RETURNCMD, rect.left, rect.bottom, 0 , hWndButton, NULL);
			DestroyMenu(hMenu);
			switch(result) {
			case 1:
			case 2: {
				// クリップボードからペースト
				BOOL clear_clipboard = result == 2;
				clipboard = GetClipboardTextW(dlg, clear_clipboard);
				if (clipboard != NULL) {
					SetDlgItemTextW(dlg, IDC_SSHPASSWORD, clipboard);
					free(clipboard);
					SendDlgItemMessage(dlg, IDC_SSHPASSWORD, EM_SETSEL, 0, -1);
					SendMessage(dlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(dlg, IDC_SSHPASSWORD), TRUE);
					return FALSE;
				}
				return TRUE;
			}
			case 3:
				// 制御コード使用/未使用
				UseControlChar = !UseControlChar;
				break;
			case 4:
				// パスフレーズ表示/非表示
				ShowPassPhrase = !ShowPassPhrase;
				{
					// 伏せ字 on/off を切り替える
					HWND hWnd = GetDlgItem(dlg, IDC_SSHPASSWORD);
					if (password_char == 0) {
						password_char = (wchar_t)SendMessageW(hWnd, EM_GETPASSWORDCHAR, 0, 0);
					}
					if (ShowPassPhrase) {
						SendMessageW(hWnd, EM_SETPASSWORDCHAR, 0, 0);
					} else {
						if (IsWindowUnicode(hWnd)) {
							SendMessageW(hWnd, EM_SETPASSWORDCHAR, (WPARAM)password_char, 0);
						}
						else {
							// EM_GETPASSWORDCHAR で Unicode キャラクタが取得できても
							// IsWindowUnicode(hWnd) == FALSE のとき Unicode は設定できない
							SendMessageA(hWnd, EM_SETPASSWORDCHAR, (WPARAM)'*', 0);
						}
					}
					SendDlgItemMessage(dlg, IDC_SSHPASSWORD, EM_SETSEL, 0, -1);
					SendMessage(dlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(dlg, IDC_SSHPASSWORD), TRUE);
					return TRUE;
				}
				break;
			}
			break;
		}

		case IDC_USERNAME_OPTION: {
			RECT rect;
			HWND hWndButton;
			HMENU hMenu= CreatePopupMenu();
			int result;
			const BOOL DisableDefaultUserName = pvar->session_settings.DefaultUserName[0] == 0;
			wchar_t *uimsg;
			GetI18nStrWW("TTSSH", "DLG_AUTH_USE_DEFAULT_USERNAME",
						 L"Use &default username",
						 pvar->ts->UILanguageFileW, &uimsg);
			AppendMenuW(hMenu, MF_ENABLED | MF_STRING | (DisableDefaultUserName ? MFS_DISABLED : 0), 1,
						uimsg);
			free(uimsg);
			GetI18nStrWW("TTSSH", "DLG_AUTH_USE_LOGON_USERNAME",
						 L"Use &logon username",
						 pvar->ts->UILanguageFileW, &uimsg);
			AppendMenuW(hMenu, MF_ENABLED | MF_STRING, 2, uimsg);
			free(uimsg);
			hWndButton = GetDlgItem(dlg, IDC_USERNAME_OPTION);
			GetWindowRect(hWndButton, &rect);
			result = TrackPopupMenu(hMenu, TPM_RETURNCMD, rect.left, rect.bottom, 0 , hWndButton, NULL);
			DestroyMenu(hMenu);
			switch (result) {
			case 1:
				SetDlgItemTextW(dlg, IDC_SSHUSERNAME, pvar->session_settings.DefaultUserName);
				goto after_user_name_set;
			case 2: {
				wchar_t *user_name;
				hGetUserNameW(&user_name);
				SetDlgItemTextW(dlg, IDC_SSHUSERNAME, user_name);
				free(user_name);
			after_user_name_set:
				SendDlgItemMessage(dlg, IDC_SSHUSERNAME, EM_SETSEL, 0, -1);
				SendMessage(dlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(dlg, IDC_SSHUSERNAME), TRUE);
				break;
			}
			}
			return TRUE;
		}

		default:
			return FALSE;
		}
		return FALSE;

	case WM_DPICHANGED:
		SendDlgItemMessage(dlg, IDC_USERNAME_OPTION, WM_DPICHANGED, wParam, lParam);
		SendDlgItemMessage(dlg, IDC_SSHPASSWORD_OPTION, WM_DPICHANGED, wParam, lParam);
		return FALSE;

	default:
		return FALSE;
	}
}

char *AUTH_get_user_name(PTInstVar pvar)
{
	return pvar->auth_state.user;
}

// pvar->auth_state.supported_types を更新する
int AUTH_set_supported_auth_types(PTInstVar pvar, int types)
{
	logprintf(LOG_LEVEL_VERBOSE, "Server reports supported authentication method mask = %d", types);

	if (SSHv1(pvar)) {
		types &= (1 << SSH_AUTH_PASSWORD) | (1 << SSH_AUTH_RSA)
		       | (1 << SSH_AUTH_RHOSTS_RSA) | (1 << SSH_AUTH_RHOSTS)
		       | (1 << SSH_AUTH_TIS) | (1 << SSH_AUTH_PAGEANT);
	} else {
		// for SSH2(yutaka)
//		types &= (1 << SSH_AUTH_PASSWORD);
		// 公開鍵認証を有効にする (2004.12.18 yutaka)
		// TISを追加。SSH2ではkeyboard-interactiveとして扱う。(2005.3.12 yutaka)
		types &= (1 << SSH_AUTH_PASSWORD) | (1 << SSH_AUTH_RSA)
		       | (1 << SSH_AUTH_TIS) | (1 << SSH_AUTH_PAGEANT);
	}
	pvar->auth_state.supported_types = types;

	if (types == 0) {
		UTIL_get_lang_msg("MSG_NOAUTHMETHOD_ERROR", pvar,
		                  "Server does not support any of the authentication options\n"
		                  "provided by TTSSH. This connection will now close.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		return 0;
	} else {
		if (pvar->auth_state.auth_dialog != NULL) {
			update_server_supported_types(pvar, pvar->auth_state.auth_dialog);
		}

		return 1;
	}
}

static void start_user_auth(PTInstVar pvar)
{
	// 認証ダイアログを表示させる (2004.12.1 yutaka)
	PostMessage(pvar->NotificationWindow, WM_COMMAND, (WPARAM) ID_SSHAUTH,
				(LPARAM) NULL);
	pvar->auth_state.cur_cred.method = SSH_AUTH_NONE;
}

static void try_default_auth(PTInstVar pvar)
{
	if (pvar->session_settings.TryDefaultAuth) {
		switch (pvar->session_settings.DefaultAuthMethod) {
		case SSH_AUTH_RSA:{
				BOOL invalid_passphrase;
				char password[] = "";

				pvar->auth_state.cur_cred.key_pair
					=
					KEYFILES_read_private_key(pvar,
					                          pvar->session_settings.
					                          DefaultRSAPrivateKeyFile,
					                          password,
					                          &invalid_passphrase, TRUE);
				if (pvar->auth_state.cur_cred.key_pair == NULL) {
					return;
				} else {
					pvar->auth_state.cur_cred.method = SSH_AUTH_RSA;
				}
				break;
			}

		case SSH_AUTH_RHOSTS:
			if (pvar->session_settings.
				DefaultRhostsHostPrivateKeyFile[0] != 0) {
				BOOL invalid_passphrase;
				char password[] = "";

				pvar->auth_state.cur_cred.key_pair
					=
					KEYFILES_read_private_key(pvar,
					                          pvar->session_settings.
					                          DefaultRhostsHostPrivateKeyFile,
					                          password,
					                          &invalid_passphrase, TRUE);
				if (pvar->auth_state.cur_cred.key_pair == NULL) {
					return;
				} else {
					pvar->auth_state.cur_cred.method = SSH_AUTH_RHOSTS_RSA;
				}
			} else {
				pvar->auth_state.cur_cred.method = SSH_AUTH_RHOSTS;
			}

			pvar->auth_state.cur_cred.rhosts_client_user =
				_strdup(pvar->session_settings.DefaultRhostsLocalUserName);
			break;

		case SSH_AUTH_PAGEANT:
			pvar->auth_state.cur_cred.method = SSH_AUTH_PAGEANT;
			break;

		case SSH_AUTH_PASSWORD:
			pvar->auth_state.cur_cred.password = _strdup("");
			pvar->auth_state.cur_cred.method = SSH_AUTH_PASSWORD;
			break;

		case SSH_AUTH_TIS:
		default:
			return;
		}

		pvar->auth_state.user =
			ToU8W(pvar->session_settings.DefaultUserName);
	}
}

void AUTH_notify_end_error(PTInstVar pvar)
{
	if ((pvar->auth_state.flags & AUTH_START_USER_AUTH_ON_ERROR_END) != 0) {
		start_user_auth(pvar);
		pvar->auth_state.flags &= ~AUTH_START_USER_AUTH_ON_ERROR_END;
	}
}

void AUTH_advance_to_next_cred(PTInstVar pvar)
{
	logprintf(LOG_LEVEL_VERBOSE, "User authentication will be shown by %d method.", pvar->auth_state.cur_cred.method);

	pvar->auth_state.failed_method = pvar->auth_state.cur_cred.method;

	if (pvar->auth_state.cur_cred.method == SSH_AUTH_NONE) {
		try_default_auth(pvar);

		if (pvar->auth_state.cur_cred.method == SSH_AUTH_NONE) {
			if (pvar->err_msg != NULL) {
				pvar->auth_state.flags |=
					AUTH_START_USER_AUTH_ON_ERROR_END;
			} else {
				// ここで認証ダイアログを出現させる (2004.12.1 yutaka)
				// コマンドライン指定なしの場合
				start_user_auth(pvar);
			}
		}
	} else {
		// ここで認証ダイアログを出現させる (2004.12.1 yutaka)
		// コマンドライン指定あり(/auth=xxxx)の場合
		start_user_auth(pvar);
	}
}

static void init_TIS_dlg(PTInstVar pvar, HWND dlg)
{
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_TIS_TITLE" },
		{ IDC_SSHAUTHBANNER, "DLG_TIS_BANNER" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_DISCONNECT" },
	};
	SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

	init_auth_machine_banner(pvar, dlg);
	init_password_control(pvar, dlg, IDC_SSHPASSWORD, NULL);

	if (pvar->auth_state.TIS_prompt != NULL) {
		if (strlen(pvar->auth_state.TIS_prompt) > 10000) {
			pvar->auth_state.TIS_prompt[10000] = 0;
		}
		SetDlgItemText(dlg, IDC_SSHAUTHBANNER2,
					   pvar->auth_state.TIS_prompt);
		destroy_malloced_string(&pvar->auth_state.TIS_prompt);
	}

	if (pvar->auth_state.echo) {
		SendMessage(GetDlgItem(dlg, IDC_SSHPASSWORD), EM_SETPASSWORDCHAR, 0, 0);
	}
}

static BOOL end_TIS_dlg(PTInstVar pvar, HWND dlg)
{
	char *password;

	hGetDlgItemTextAorU8(dlg, IDC_SSHPASSWORD, &password);

	pvar->auth_state.cur_cred.method = SSH_AUTH_TIS;
	pvar->auth_state.cur_cred.password = password;
	pvar->auth_state.auth_dialog = NULL;

	// add
	if (SSHv2(pvar)) {
		pvar->keyboard_interactive_password_input = 1;
		handle_SSH2_userauth_inforeq(pvar);
	}

	SSH_notify_cred(pvar);

	TTEndDialog(dlg, 1);
	return TRUE;
}

static INT_PTR CALLBACK TIS_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
                                     LPARAM lParam)
{
	PTInstVar pvar;

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		pvar->auth_state.auth_dialog = dlg;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		init_TIS_dlg(pvar, dlg);

		// /auth=challenge を追加 (2007.10.5 maya)
		if (pvar->ssh2_autologin == 1) {
			SetDlgItemText(dlg, IDC_SSHPASSWORD, pvar->ssh2_password);
			SendMessage(dlg, WM_COMMAND, IDOK, 0);
		}

		CenterWindow(dlg, GetParent(dlg));
		return FALSE;			/* because we set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDOK:
			return end_TIS_dlg(pvar, dlg);

		case IDCANCEL:			/* kill the connection */
			pvar->auth_state.auth_dialog = NULL;
			notify_closed_connection(pvar, "authentication cancelled");
			TTEndDialog(dlg, 0);
			return TRUE;

		case IDCLOSE:
			// 認証中にネットワーク切断された場合、当該メッセージでダイアログを閉じる。
			pvar->auth_state.auth_dialog = NULL;
			TTEndDialog(dlg, 0);
			return TRUE;

		default:
			return FALSE;
		}

	default:
		return FALSE;
	}
}

void AUTH_do_cred_dialog(PTInstVar pvar)
{
	if (pvar->auth_state.auth_dialog == NULL) {
		HWND cur_active = GetActiveWindow();
		DLGPROC dlg_proc;
		LPCWSTR dialog_template;
		INT_PTR r;

		switch (pvar->auth_state.mode) {
		case TIS_AUTH_MODE:
			dialog_template = MAKEINTRESOURCEW(IDD_SSHTISAUTH);
			dlg_proc = TIS_dlg_proc;
			break;
		case GENERIC_AUTH_MODE:
		default:
			dialog_template = MAKEINTRESOURCEW(IDD_SSHAUTH);
			dlg_proc = auth_dlg_proc;
		}

		r = TTDialogBoxParam(hInst, dialog_template,
							 cur_active !=
							 NULL ? cur_active : pvar->NotificationWindow,
							 dlg_proc, (LPARAM) pvar);
		if (r == -1) {
			UTIL_get_lang_msg("MSG_CREATEWINDOW_AUTH_ERROR", pvar,
			                  "Unable to display authentication dialog box.\n"
			                  "Connection terminated.");
			notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		}
	}
}

static void init_default_auth_dlg(PTInstVar pvar, HWND dlg)
{
	int id;
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_AUTHSETUP_TITLE" },
		{ IDC_SSHAUTHBANNER, "DLG_AUTHSETUP_BANNER" },
		{ IDC_SSH_USERNAME, "DLG_AUTHSETUP_USERNAME" },
		{ IDC_SSH_NO_USERNAME, "DLG_AUTHSETUP_NO_USERNAME" },
		{ IDC_SSH_DEFAULTUSERNAME, "DLG_AUTHSETUP_DEFAULT_USERNAME" },
		{ IDC_SSH_WINDOWS_USERNAME, "DLG_AUTHSETUP_LOGON_USERNAME" },
		{ IDC_SSH_WINDOWS_USERNAME_TEXT, "DLG_AUTHSETUP_LOGON_USERNAME_TEXT" },
		{ IDC_SSH_AUTHMETHOD, "DLG_AUTHSETUP_METHOD" },
		{ IDC_SSHUSEPASSWORD, "DLG_AUTHSETUP_METHOD_PASSWORD" },
		{ IDC_SSHUSERSA, "DLG_AUTHSETUP_METHOD_RSA" },
		{ IDC_SSHUSERHOSTS, "DLG_AUTHSETUP_METHOD_RHOST" },
		{ IDC_SSHUSETIS, "DLG_AUTHSETUP_METHOD_CHALLENGE" },
		{ IDC_SSHUSEPAGEANT, "DLG_AUTHSETUP_METHOD_PAGEANT" },
		{ IDC_RSAFILENAMELABEL, "DLG_AUTH_PRIVATEKEY" },
		{ IDC_LOCALUSERNAMELABEL, "DLG_AUTH_LOCALUSER" },
		{ IDC_HOSTRSAFILENAMELABEL, "DLG_AUTH_HOST_PRIVATEKEY" },
		{ IDC_CHECKAUTH, "DLG_AUTHSETUP_CHECKAUTH" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_SSHAUTHSETUP_HELP, "BTN_HELP" },
	};

	SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

	switch (pvar->settings.DefaultAuthMethod) {
	case SSH_AUTH_RSA:
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL,
		                 IDC_SSHUSERSA);
		break;
	case SSH_AUTH_RHOSTS:
	case SSH_AUTH_RHOSTS_RSA:
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL,
		                 IDC_SSHUSERHOSTS);
		break;
	case SSH_AUTH_TIS:
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL,
		                 IDC_SSHUSETIS);
		break;
	case SSH_AUTH_PAGEANT:
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL,
		                 IDC_SSHUSEPAGEANT);
		break;
	case SSH_AUTH_PASSWORD:
	default:
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL,
		                 IDC_SSHUSEPASSWORD);
	}

	SetDlgItemTextW(dlg, IDC_SSHUSERNAME, pvar->settings.DefaultUserName);
	SetDlgItemTextW(dlg, IDC_RSAFILENAME, pvar->settings.DefaultRSAPrivateKeyFile);
	SetDlgItemTextW(dlg, IDC_HOSTRSAFILENAME, pvar->settings.DefaultRhostsHostPrivateKeyFile);
	SetDlgItemText(dlg, IDC_LOCALUSERNAME,
	               pvar->settings.DefaultRhostsLocalUserName);

	if (pvar->settings.CheckAuthListFirst) {
		CheckDlgButton(dlg, IDC_CHECKAUTH, TRUE);
	}

	if (pvar->settings.DefaultUserType == 1 &&
		pvar->settings.DefaultUserName[0] == 0) {
		// 「デフォルトユーザ名を使用する」設定だがユーザ名がないので、「入力しない」にする
		pvar->settings.DefaultUserType = 0;
	}
	id = pvar->settings.DefaultUserType == 1 ? IDC_SSH_DEFAULTUSERNAME :
		pvar->settings.DefaultUserType == 2 ? IDC_SSH_WINDOWS_USERNAME :
		IDC_SSH_NO_USERNAME;
	CheckRadioButton(dlg, IDC_SSH_NO_USERNAME, IDC_SSH_WINDOWS_USERNAME, id);

	{
		wchar_t *user_name;
		wchar_t *format;
		wchar_t *text;

		hGetUserNameW(&user_name);
		hGetDlgItemTextW(dlg, IDC_SSH_WINDOWS_USERNAME_TEXT, &format);
		aswprintf(&text, format, user_name);
		free(format);
		free(user_name);
		SetDlgItemTextW(dlg, IDC_SSH_WINDOWS_USERNAME_TEXT, text);
		free(text);
	}
}

static BOOL end_default_auth_dlg(PTInstVar pvar, HWND dlg)
{
	if (IsDlgButtonChecked(dlg, IDC_SSHUSERSA)) {
		pvar->settings.DefaultAuthMethod = SSH_AUTH_RSA;
	} else if (IsDlgButtonChecked(dlg, IDC_SSHUSERHOSTS)) {
		if (GetWindowTextLength(GetDlgItem(dlg, IDC_HOSTRSAFILENAME)) > 0) {
			pvar->settings.DefaultAuthMethod = SSH_AUTH_RHOSTS_RSA;
		} else {
			pvar->settings.DefaultAuthMethod = SSH_AUTH_RHOSTS;
		}
	} else if (IsDlgButtonChecked(dlg, IDC_SSHUSETIS)) {
		pvar->settings.DefaultAuthMethod = SSH_AUTH_TIS;
	} else if (IsDlgButtonChecked(dlg, IDC_SSHUSEPAGEANT)) {
		pvar->settings.DefaultAuthMethod = SSH_AUTH_PAGEANT;
	} else {
		pvar->settings.DefaultAuthMethod = SSH_AUTH_PASSWORD;
	}

	GetDlgItemTextW(dlg, IDC_SSHUSERNAME, pvar->settings.DefaultUserName,
					_countof(pvar->settings.DefaultUserName));
	GetDlgItemTextW(dlg, IDC_RSAFILENAME,
	                pvar->settings.DefaultRSAPrivateKeyFile,
	                _countof(pvar->settings.DefaultRSAPrivateKeyFile));
	GetDlgItemTextW(dlg, IDC_HOSTRSAFILENAME,
	                pvar->settings.DefaultRhostsHostPrivateKeyFile,
				    _countof(pvar->settings.DefaultRhostsHostPrivateKeyFile));
	GetDlgItemText(dlg, IDC_LOCALUSERNAME,
	               pvar->settings.DefaultRhostsLocalUserName,
	               sizeof(pvar->settings.DefaultRhostsLocalUserName));
	pvar->settings.DefaultUserType =
		IsDlgButtonChecked(dlg, IDC_SSH_DEFAULTUSERNAME) ? 1 :
		IsDlgButtonChecked(dlg, IDC_SSH_WINDOWS_USERNAME) ? 2 : 0;

	if (IsDlgButtonChecked(dlg, IDC_CHECKAUTH)) {
		pvar->settings.CheckAuthListFirst = TRUE;
	}
	else {
		pvar->settings.CheckAuthListFirst = FALSE;
	}

	TTEndDialog(dlg, 1);
	return TRUE;
}

static INT_PTR CALLBACK default_auth_dlg_proc(HWND dlg, UINT msg,
                                              WPARAM wParam, LPARAM lParam)
{
	PTInstVar pvar;

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		init_default_auth_dlg(pvar, dlg);
		CenterWindow(dlg, GetParent(dlg));
		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDOK:
			return end_default_auth_dlg(pvar, dlg);

		case IDCANCEL:
			TTEndDialog(dlg, 0);
			return TRUE;

		case IDC_SSHAUTHSETUP_HELP:
			PostMessage(GetParent(dlg), WM_USER_DLGHELP2, HlpMenuSetupSshauth, 0);
			return TRUE;

		case IDC_CHOOSERSAFILE:
			choose_RSA_key_file(dlg, pvar);
			return TRUE;

		case IDC_CHOOSEHOSTRSAFILE:
			choose_host_RSA_key_file(dlg, pvar);
			return TRUE;

		default:
			return FALSE;
		}

	default:
		return FALSE;
	}
}

void AUTH_init(PTInstVar pvar)
{
	pvar->auth_state.failed_method = SSH_AUTH_NONE;
	pvar->auth_state.partial_success = 0;
	pvar->auth_state.multiple_required_auth = 0;
	pvar->auth_state.auth_dialog = NULL;
	pvar->auth_state.user = NULL;
	pvar->auth_state.flags = 0;
	pvar->auth_state.TIS_prompt = NULL;
	pvar->auth_state.echo = 0;
	pvar->auth_state.supported_types = 0;
	pvar->auth_state.cur_cred.method = SSH_AUTH_NONE;
	pvar->auth_state.cur_cred.password = NULL;
	pvar->auth_state.cur_cred.rhosts_client_user = NULL;
	pvar->auth_state.cur_cred.key_pair = NULL;
	AUTH_set_generic_mode(pvar);
}

void AUTH_set_generic_mode(PTInstVar pvar)
{
	pvar->auth_state.mode = GENERIC_AUTH_MODE;
	destroy_malloced_string(&pvar->auth_state.TIS_prompt);
}

void AUTH_set_TIS_mode(PTInstVar pvar, char *prompt, int len, int echo)
{
	if (pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) {
		pvar->auth_state.mode = TIS_AUTH_MODE;

		destroy_malloced_string(&pvar->auth_state.TIS_prompt);
		pvar->auth_state.TIS_prompt = malloc(len + 1);
		memcpy(pvar->auth_state.TIS_prompt, prompt, len);
		pvar->auth_state.TIS_prompt[len] = 0;
		pvar->auth_state.echo = echo;
	} else {
		AUTH_set_generic_mode(pvar);
	}
}

void AUTH_do_default_cred_dialog(PTInstVar pvar)
{
	HWND cur_active = GetActiveWindow();

	if (TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SSHAUTHSETUP),
						 cur_active != NULL ? cur_active
		                                  : pvar->NotificationWindow,
						 default_auth_dlg_proc, (LPARAM) pvar) == -1) {
		UTIL_get_lang_msg("MSG_CREATEWINDOW_AUTHSETUP_ERROR", pvar,
		                  "Unable to display authentication setup dialog box.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
	}
}

void AUTH_destroy_cur_cred(PTInstVar pvar)
{
	destroy_malloced_string(&pvar->auth_state.cur_cred.password);
	destroy_malloced_string(&pvar->auth_state.cur_cred.rhosts_client_user);
	if (pvar->auth_state.cur_cred.key_pair != NULL) {
		key_free(pvar->auth_state.cur_cred.key_pair);
		pvar->auth_state.cur_cred.key_pair = NULL;
	}
}

static const char *get_auth_method_name(SSHAuthMethod auth)
{
	switch (auth) {
	case SSH_AUTH_PASSWORD:
		return "password";
	case SSH_AUTH_RSA:
		return "publickey";
	case SSH_AUTH_PAGEANT:
		return "publickey";
	case SSH_AUTH_RHOSTS:
		return "rhosts";
	case SSH_AUTH_RHOSTS_RSA:
		return "rhosts with RSA";
	case SSH_AUTH_TIS:
		return "challenge/response (TIS)";
	default:
		return "unknown method";
	}
}

void AUTH_get_auth_info(PTInstVar pvar, char *dest, int len)
{
	const char *method = "unknown";
	char buf[256];

	if (pvar->auth_state.user == NULL) {
		strncpy_s(dest, len, "None", _TRUNCATE);
	} else if (pvar->auth_state.cur_cred.method != SSH_AUTH_NONE) {
		if (SSHv1(pvar)) {
			UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', %s authentication");
			_snprintf_s(dest, len, _TRUNCATE, pvar->UIMsg,
			            pvar->auth_state.user,
			            get_auth_method_name(pvar->auth_state.cur_cred.method));

			if (pvar->auth_state.cur_cred.method == SSH_AUTH_RSA) {
				UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO2", pvar, ", %s key");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
				            "RSA");
				strncat_s(dest, len, buf, _TRUNCATE);
			}
			else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
				UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO3", pvar, ", %s key");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
				            "RSA");
				strncat_s(dest, len, buf, _TRUNCATE);

				_snprintf_s(buf, sizeof(buf), _TRUNCATE, " (from Pageant)");
				strncat_s(dest, len, buf, _TRUNCATE);
			}
		} else {
			// SSH2:認証メソッドの判別 (2004.12.23 yutaka)
			// keyboard-interactiveメソッドを追加 (2005.3.12 yutaka)
			if (pvar->auth_state.cur_cred.method == SSH_AUTH_PASSWORD ||
				pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) {
				// keyboard-interactiveメソッドを追加 (2005.1.24 yutaka)
				if (pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) {
					method = "keyboard-interactive";
				} else {
					method = get_auth_method_name(pvar->auth_state.cur_cred.method);
				}
				UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', %s authentication");
				_snprintf_s(dest, len, _TRUNCATE, pvar->UIMsg,
				            pvar->auth_state.user, method);
			}
			else if (pvar->auth_state.cur_cred.method == SSH_AUTH_RSA) {
				ssh_keyalgo pubkey_algo;
				char *digest_name;

				method = get_auth_method_name(pvar->auth_state.cur_cred.method);
				UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', %s authentication");
				_snprintf_s(dest, len, _TRUNCATE, pvar->UIMsg,
				            pvar->auth_state.user,
				            get_auth_method_name(pvar->auth_state.cur_cred.method));

				pubkey_algo = choose_SSH2_keysign_algorithm(pvar, pvar->auth_state.cur_cred.key_pair->type);
				digest_name = get_ssh2_hostkey_algorithm_digest_name(pubkey_algo);
				if (strlen(digest_name) == 0) {
					UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO2", pvar, ", %s key");
					_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
					            ssh_key_type(pvar->auth_state.cur_cred.key_pair->type));
					strncat_s(dest, len, buf, _TRUNCATE);
				}
				else {
					UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO3", pvar, ", %s key with %s");
					_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
					            ssh_key_type(pvar->auth_state.cur_cred.key_pair->type),
					            digest_name);
					strncat_s(dest, len, buf, _TRUNCATE);
				}
			}
			else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
				int key_len = get_uint32_MSBfirst(pvar->pageant_curkey + 4);
				char *s = (char *)malloc(key_len+1);
				ssh_keytype keytype;
				ssh_keyalgo pubkey_algo;
				char *digest_name;

				method = get_auth_method_name(pvar->auth_state.cur_cred.method);
				UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', %s authentication");
				_snprintf_s(dest, len, _TRUNCATE, pvar->UIMsg,
				            pvar->auth_state.user,
				            get_auth_method_name(pvar->auth_state.cur_cred.method));

				memcpy(s, pvar->pageant_curkey+4+4, key_len);
				s[key_len] = '\0';
				keytype = get_hostkey_type_from_name(s);
				pubkey_algo = choose_SSH2_keysign_algorithm(pvar, keytype);
				digest_name = get_ssh2_hostkey_algorithm_digest_name(pubkey_algo);
				if (strlen(digest_name) == 0) {
					UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO2", pvar, ", %s key");
					_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
					            ssh_key_type(get_hostkey_type_from_name(s)));
					strncat_s(dest, len, buf, _TRUNCATE);
				}
				else {
					UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO3", pvar, ", %s key with %s");
					_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->UIMsg,
					            ssh_key_type(get_hostkey_type_from_name(s)),
					            digest_name);
					strncat_s(dest, len, buf, _TRUNCATE);
				}

				_snprintf_s(buf, sizeof(buf), _TRUNCATE, " (from Pageant)");
				strncat_s(dest, len, buf, _TRUNCATE);

				free(s);
			}
		}

	} else {
		UTIL_get_lang_msgU8("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', using %s");
		_snprintf_s(dest, len, _TRUNCATE, pvar->UIMsg, pvar->auth_state.user,
		            get_auth_method_name(pvar->auth_state.failed_method));
	}

	dest[len - 1] = 0;
}

void AUTH_notify_disconnecting(PTInstVar pvar)
{
	if (pvar->auth_state.auth_dialog != NULL) {
		PostMessage(pvar->auth_state.auth_dialog, WM_COMMAND, IDCANCEL, 0);
		/* the main window might not go away if it's not enabled. (see vtwin.cpp) */
		EnableWindow(pvar->NotificationWindow, TRUE);
	}
}

// TCPセッションがクローズされた場合、認証ダイアログを閉じるように指示を出す。
// AUTH_notify_disconnecting()とは異なり、ダイアログを閉じるのみで、
// SSHサーバに通知は出さない。
void AUTH_notify_closing_on_exit(PTInstVar pvar)
{
	if (pvar->auth_state.auth_dialog != NULL) {
		logprintf(LOG_LEVEL_INFO, "%s: Notify closing message to the authentication dialog.", __FUNCTION__);
		PostMessage(pvar->auth_state.auth_dialog, WM_COMMAND, IDCLOSE, 0);
	}
}

void AUTH_end(PTInstVar pvar)
{
	destroy_malloced_string(&pvar->auth_state.user);
	destroy_malloced_string(&pvar->auth_state.TIS_prompt);

	AUTH_destroy_cur_cred(pvar);
}

/* vim: set ts=4 sw=4 ff=dos : */
