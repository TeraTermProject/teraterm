/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (C) 2004-2019 TeraTerm Project
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
#include <Lmcons.h>		// for UNLEN
#include <crtdbg.h>

#include "resource.h"
#include "keyfiles.h"
#include "libputty.h"
#include "tipwin.h"
#include "auth.h"

#if defined(_DEBUG) && !defined(_CRTDBG_MAP_ALLOC)
#define malloc(l) _malloc_dbg((l), _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)   _free_dbg((p), _NORMAL_BLOCK)
#endif

#define AUTH_START_USER_AUTH_ON_ERROR_END 1

#define MAX_AUTH_CONTROL IDC_SSHUSEPAGEANT

#undef DialogBoxParam
#define DialogBoxParam(p1,p2,p3,p4,p5) \
	TTDialogBoxParam(p1,p2,p3,p4,p5)
#undef EndDialog
#define EndDialog(p1,p2) \
	TTEndDialog(p1, p2)

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
			TCHAR chars[] = { (TCHAR) wParam, 0 };

			SendMessage(control, EM_REPLACESEL, (WPARAM) TRUE,
			            (LPARAM) (TCHAR *) chars);

			if (data->tipwin == NULL) {
				TCHAR uimsg[MAX_UIMSG];
				RECT rect;
				PTInstVar pvar = data->pvar;
				UTIL_get_lang_msg("DLG_AUTH_TIP_CONTROL_CODE", pvar, "control character is entered");
				_tcscpy_s(uimsg, _countof(uimsg), pvar->ts->UIMsg);
				if (wParam == 'V' - 'A' + 1) {
					// CTRL + V
					_tcscat_s(uimsg, _countof(uimsg), _T("\n"));
					UTIL_get_lang_msg("DLG_AUTH_TIP_PASTE_KEY", pvar, "Use Shift + Insert to paste from clipboard");
					_tcscat_s(uimsg, _countof(uimsg), pvar->ts->UIMsg);
				}
				GetWindowRect(control, &rect);
				data->tipwin = TipWinCreate(control, rect.left, rect.bottom, uimsg, FALSE);
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

	result = CallWindowProc((WNDPROC)data->ProcOrg,
							control, msg, wParam, lParam);

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
	SetWindowLongPtr(passwordControl, GWLP_WNDPROC, (LONG_PTR)password_wnd_proc);
	SetWindowLongPtr(passwordControl, GWLP_USERDATA, (LONG_PTR)data);
	SetFocus(passwordControl);
}

static void set_auth_options_status(HWND dlg, int controlID)
{
	BOOL RSA_enabled = controlID == IDC_SSHUSERSA;
	BOOL rhosts_enabled = controlID == IDC_SSHUSERHOSTS;
	BOOL TIS_enabled = controlID == IDC_SSHUSETIS;
	BOOL PAGEANT_enabled = controlID == IDC_SSHUSEPAGEANT;
	int i;

	CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, controlID);

	EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORDCAPTION), (!TIS_enabled && !PAGEANT_enabled));
	EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD), (!TIS_enabled && !PAGEANT_enabled));
	EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD_OPTION), (!TIS_enabled && !PAGEANT_enabled));

	for (i = IDC_CHOOSERSAFILE; i <= IDC_RSAFILENAME; i++) {
		EnableWindow(GetDlgItem(dlg, i), RSA_enabled);
	}

	for (i = IDC_LOCALUSERNAMELABEL; i <= IDC_HOSTRSAFILENAME; i++) {
		EnableWindow(GetDlgItem(dlg, i), rhosts_enabled);
	}
}

static void init_auth_machine_banner(PTInstVar pvar, HWND dlg)
{
	char buf[1024], buf2[1024];

	GetDlgItemText(dlg, IDC_SSHAUTHBANNER, buf2, sizeof(buf2));
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, buf2, SSH_get_host_name(pvar));
	SetDlgItemText(dlg, IDC_SSHAUTHBANNER, buf);
}

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

static LRESULT CALLBACK username_proc(HWND hWnd, UINT msg,
									  WPARAM wParam, LPARAM lParam)
{
	const WNDPROC ProcOrg = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	const LRESULT result = CallWindowProc(ProcOrg, hWnd, msg, wParam, lParam);
	switch (msg) {
	case WM_CHAR:
	case WM_SETTEXT: {
		// ユーザー名が入力されていた場合、オプションを使うことはないので、
		// tabでのフォーカス移動時、オプションボタンをパスするようにする
		// 従来と同じキー操作でユーザー名とパスフレーズを入力可能とする
		const HWND dlg = GetParent(hWnd);
		const HWND hWndOption = GetDlgItem(dlg, IDC_USERNAME_OPTION);
		const int len = GetWindowTextLength(hWnd);
		LONG_PTR style = GetWindowLongPtr(hWndOption, GWL_STYLE);
		if (len > 0) {
			// 不要tabstop
			style = style & (~(LONG_PTR)WS_TABSTOP);
		} else {
			// 要tabstop
			style = style | WS_TABSTOP;
		}
		SetWindowLongPtr(hWndOption, GWL_STYLE, style);
	}
	}
	return result;
}

static void init_auth_dlg(PTInstVar pvar, HWND dlg, BOOL *UseControlChar)
{
	const static DlgTextInfo text_info[] = {
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
	int default_method = pvar->session_settings.DefaultAuthMethod;

	SetI18DlgStrs("TTSSH", dlg, text_info, _countof(text_info), pvar->ts->UILanguageFile);

	init_auth_machine_banner(pvar, dlg);
	init_password_control(pvar, dlg, IDC_SSHPASSWORD, UseControlChar);

	// 認証失敗後はラベルを書き換え
	if (pvar->auth_state.failed_method != SSH_AUTH_NONE) {
		/* must be retrying a failed attempt */
		UTIL_get_lang_msg("DLG_AUTH_BANNER2_FAILED", pvar, "Authentication failed. Please retry.");
		SetDlgItemText(dlg, IDC_SSHAUTHBANNER2, pvar->ts->UIMsg);
		UTIL_get_lang_msg("DLG_AUTH_TITLE_FAILED", pvar, "Retrying SSH Authentication");
		SetWindowText(dlg, pvar->ts->UIMsg);
		default_method = pvar->auth_state.failed_method;
	}

	// パスワードを覚えておくチェックボックスにはデフォルトで有効とする (2006.8.3 yutaka)
	if (pvar->ts_SSH->remember_password) {
		SendMessage(GetDlgItem(dlg, IDC_REMEMBER_PASSWORD), BM_SETCHECK, BST_CHECKED, 0);
	} else {
		SendMessage(GetDlgItem(dlg, IDC_REMEMBER_PASSWORD), BM_SETCHECK, BST_UNCHECKED, 0);
	}

	// ForwardAgent の設定を反映する (2008.12.4 maya)
	CheckDlgButton(dlg, IDC_FORWARD_AGENT, pvar->settings.ForwardAgent);

	// SSH バージョンによって TIS のラベルを書き換え
	if (pvar->settings.ssh_protocol_version == 1) {
		UTIL_get_lang_msg("DLG_AUTH_METHOD_CHALLENGE1", pvar,
		                  "Use challenge/response(&TIS) to log in");
		SetDlgItemText(dlg, IDC_SSHUSETIS, pvar->ts->UIMsg);
	} else {
		UTIL_get_lang_msg("DLG_AUTH_METHOD_CHALLENGE2", pvar,
		                  "Use keyboard-&interactive to log in");
		SetDlgItemText(dlg, IDC_SSHUSETIS, pvar->ts->UIMsg);
	}

	// usernameのサブクラス化
	{
		HWND hWndUserName = GetDlgItem(dlg, IDC_SSHUSERNAME);
		LONG_PTR ProcOrg =
			SetWindowLongPtr(hWndUserName, GWLP_WNDPROC, (LONG_PTR)username_proc);
		SetWindowLongPtr(hWndUserName, GWLP_USERDATA, ProcOrg);
	}

	if (pvar->auth_state.user != NULL) {
		SetDlgItemText(dlg, IDC_SSHUSERNAME, pvar->auth_state.user);
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
				SetDlgItemText(dlg, IDC_SSHUSERNAME,
							   pvar->session_settings.DefaultUserName);
			}
			break;
		case 2: {
			TCHAR user_name[UNLEN+1];
			DWORD len = _countof(user_name);
			BOOL r = GetUserName(user_name, &len);
			if (r != 0) {
				SetDlgItemText(dlg, IDC_SSHUSERNAME, user_name);
			}
			break;
		}
		default:
			// 入力しないにしておく
			pvar->session_settings.DefaultUserType = 0;
		}
	}

	if (strlen(pvar->ssh2_password) > 0) {
		SetDlgItemText(dlg, IDC_SSHPASSWORD, pvar->ssh2_password);
		if (pvar->ssh2_autologin == 1) {
			EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORDCAPTION), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD_OPTION), FALSE);
		}
	}

	SetDlgItemText(dlg, IDC_RSAFILENAME,
	               pvar->session_settings.DefaultRSAPrivateKeyFile);
	SetDlgItemText(dlg, IDC_HOSTRSAFILENAME,
	               pvar->session_settings.DefaultRhostsHostPrivateKeyFile);
	SetDlgItemText(dlg, IDC_LOCALUSERNAME,
	               pvar->session_settings.DefaultRhostsLocalUserName);

	if (pvar->ssh2_authmethod == SSH_AUTH_PASSWORD) {
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, IDC_SSHUSEPASSWORD);

	} else if (pvar->ssh2_authmethod == SSH_AUTH_RSA) {
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, IDC_SSHUSERSA);

		SetDlgItemText(dlg, IDC_RSAFILENAME, pvar->ssh2_keyfile);
		if (pvar->ssh2_autologin == 1) {
			EnableWindow(GetDlgItem(dlg, IDC_CHOOSERSAFILE), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_RSAFILENAME), FALSE);
		}

	// /auth=challenge を追加 (2007.10.5 maya)
	} else if (pvar->ssh2_authmethod == SSH_AUTH_TIS) {
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, IDC_SSHUSETIS);
		EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD), FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD_OPTION), FALSE);
		SetDlgItemText(dlg, IDC_SSHPASSWORD, "");

	// /auth=pageant を追加
	} else if (pvar->ssh2_authmethod == SSH_AUTH_PAGEANT) {
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, IDC_SSHUSEPAGEANT);
		EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD), FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD_OPTION), FALSE);
		SetDlgItemText(dlg, IDC_SSHPASSWORD, "");

	} else {
		// デフォルトの認証メソッドをダイアログに反映
		set_auth_options_status(dlg, auth_types_to_control_IDs[default_method]);

		update_server_supported_types(pvar, dlg);

		// ホスト確認ダイアログから抜けたとき=ウィンドウがアクティブになったとき
		// に SetFocus が実行され、コマンドラインで渡された認証方式が上書きされて
		// しまうので、自動ログイン有効時は SetFocus しない (2009.1.31 maya)
		if (default_method == SSH_AUTH_TIS) {
			/* we disabled the password control, so fix the focus */
			SetFocus(GetDlgItem(dlg, IDC_SSHUSETIS));
		}
		else if (default_method == SSH_AUTH_PAGEANT) {
			SetFocus(GetDlgItem(dlg, IDC_SSHUSEPAGEANT));
		}

	}

	if (GetWindowTextLength(GetDlgItem(dlg, IDC_SSHUSERNAME)) == 0) {
		SetFocus(GetDlgItem(dlg, IDC_SSHUSERNAME));
	}
	else if (pvar->ask4passwd == 1) {
		SetFocus(GetDlgItem(dlg, IDC_SSHPASSWORD));
	}

	// '/I' 指定があるときのみ最小化する (2005.9.5 yutaka)
	if (pvar->ts->Minimize) {
		//20050822追加 start T.Takahashi
		ShowWindow(dlg,SW_MINIMIZE);
		//20050822追加 end T.Takahashi
	}
}

static char *alloc_control_text(HWND ctl)
{
	int len = GetWindowTextLength(ctl);
	char *result = malloc(len + 1);

	if (result != NULL) {
		GetWindowText(ctl, result, len + 1);
		result[len] = 0;
	}

	return result;
}

static int get_key_file_name(HWND parent, char *buf, int bufsize, PTInstVar pvar)
{
	OPENFILENAME params;
	char fullname_buf[2048] = "identity";
	char filter[MAX_UIMSG];

	ZeroMemory(&params, sizeof(params));
	params.lStructSize = get_OPENFILENAME_SIZE();
	params.hwndOwner = parent;
	// フィルタの追加 (2004.12.19 yutaka)
	// 3ファイルフィルタの追加 (2005.4.26 yutaka)
	UTIL_get_lang_msg("FILEDLG_OPEN_PRIVATEKEY_FILTER", pvar,
	                  "identity files\\0identity;id_rsa;id_dsa;id_ecdsa;id_ed25519;*.ppk;*.pem\\0identity(RSA1)\\0identity\\0id_rsa(SSH2)\\0id_rsa\\0id_dsa(SSH2)\\0id_dsa\\0id_ecdsa(SSH2)\\0id_ecdsa\\0id_ed25519(SSH2)\\0id_ed25519\\0PuTTY(*.ppk)\\0*.ppk\\0PEM files(*.pem)\\0*.pem\\0all(*.*)\\0*.*\\0\\0");
	memcpy(filter, pvar->ts->UIMsg, sizeof(filter));
	params.lpstrFilter = filter;
	params.lpstrCustomFilter = NULL;
	params.nFilterIndex = 0;
	buf[0] = 0;
	params.lpstrFile = fullname_buf;
	params.nMaxFile = sizeof(fullname_buf);
	params.lpstrFileTitle = NULL;
	params.lpstrInitialDir = NULL;
	UTIL_get_lang_msg("FILEDLG_OPEN_PRIVATEKEY_TITLE", pvar,
	                  "Choose a file with the RSA/DSA/ECDSA/ED25519 private key");
	params.lpstrTitle = pvar->ts->UIMsg;
	params.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	params.lpstrDefExt = NULL;

	if (GetOpenFileName(&params) != 0) {
		copy_teraterm_dir_relative_path(buf, bufsize, fullname_buf);
		return 1;
	} else {
		return 0;
	}
}

static void choose_RSA_key_file(HWND dlg, PTInstVar pvar)
{
	char buf[1024];

	if (get_key_file_name(dlg, buf, sizeof(buf), pvar)) {
		SetDlgItemText(dlg, IDC_RSAFILENAME, buf);
	}
}

static void choose_host_RSA_key_file(HWND dlg, PTInstVar pvar)
{
	char buf[1024];

	if (get_key_file_name(dlg, buf, sizeof(buf), pvar)) {
		SetDlgItemText(dlg, IDC_HOSTRSAFILENAME, buf);
	}
}

static BOOL end_auth_dlg(PTInstVar pvar, HWND dlg)
{
	int method = SSH_AUTH_PASSWORD;
	char *password =
		alloc_control_text(GetDlgItem(dlg, IDC_SSHPASSWORD));
	Key *key_pair = NULL;

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
		char keyfile[2048];
		int file_ctl_ID =
			method == SSH_AUTH_RSA ? IDC_RSAFILENAME : IDC_HOSTRSAFILENAME;

		keyfile[0] = 0;
		GetDlgItemText(dlg, file_ctl_ID, keyfile, sizeof(keyfile));
		if (keyfile[0] == 0) {
			UTIL_get_lang_msg("MSG_KEYSPECIFY_ERROR", pvar,
			                  "You must specify a file containing the RSA/DSA/ECDSA/ED25519 private key.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
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
					_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg, errmsg);
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
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg, errmsg);
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
		if (SSHv1(pvar)) {
			pvar->pageant_keylistlen = putty_get_ssh1_keylist(&pvar->pageant_key);
		}
		else {
			pvar->pageant_keylistlen = putty_get_ssh2_keylist(&pvar->pageant_key);
		}
		if (pvar->pageant_keylistlen == 0) {
			UTIL_get_lang_msg("MSG_PAGEANT_NOTFOUND", pvar,
			                  "Can't find Pageant.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);

			return FALSE;
		}
		pvar->pageant_curkey = pvar->pageant_key;

		// 鍵の数
		pvar->pageant_keycount = get_uint32_MSBfirst(pvar->pageant_curkey);
		if (pvar->pageant_keycount == 0) {
			UTIL_get_lang_msg("MSG_PAGEANT_NOKEY", pvar,
			                  "Pageant has no valid key.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);

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
		pvar->auth_state.user =
			alloc_control_text(GetDlgItem(dlg, IDC_SSHUSERNAME));
	}

	// パスワードの保存をするかどうかを決める (2006.8.3 yutaka)
	if (SendMessage(GetDlgItem(dlg, IDC_REMEMBER_PASSWORD), BM_GETCHECK, 0,0) == BST_CHECKED) {
		pvar->settings.remember_password = 1;  // 覚えておく
		pvar->ts_SSH->remember_password = 1;
	} else {
		pvar->settings.remember_password = 0;  // ここですっかり忘れる
		pvar->ts_SSH->remember_password = 0;
	}

	// 公開鍵認証の場合、セッション複製時にパスワードを使い回したいので解放しないようにする。
	// (2005.4.8 yutaka)
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
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		}

		pvar->auth_state.cur_cred.rhosts_client_user =
			alloc_control_text(GetDlgItem(dlg, IDC_LOCALUSERNAME));
	}
	pvar->auth_state.auth_dialog = NULL;

	GetDlgItemText(dlg, IDC_RSAFILENAME,
	               pvar->session_settings.DefaultRSAPrivateKeyFile,
	               sizeof(pvar->session_settings.
	                      DefaultRSAPrivateKeyFile));
	GetDlgItemText(dlg, IDC_HOSTRSAFILENAME,
	               pvar->session_settings.DefaultRhostsHostPrivateKeyFile,
	               sizeof(pvar->session_settings.
	                      DefaultRhostsHostPrivateKeyFile));
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

	EndDialog(dlg, 1);

	return TRUE;
}

/**
 *	クリップボードからANSI文字列を取得する
 *	文字列長が必要なときはstrlen()すること
 *	@param	hWnd
 *	@param	emtpy	TRUEのときクリップボードを空にする
 *	@retval	文字列へのポインタ 使用後free()すること
 *			文字がない(またはエラー時)はNULL
 */
char *GetClipboardTextA(HWND hWnd, BOOL empty)
{
	HGLOBAL hGlobal;
	const char *lpStr;
	size_t length;
	char *pool;

    OpenClipboard(hWnd);
    hGlobal = (HGLOBAL)GetClipboardData(CF_TEXT);
    if (hGlobal == NULL) {
        CloseClipboard();
		return NULL;
    }
    lpStr = (const char *)GlobalLock(hGlobal);
	length = GlobalSize(hGlobal);
	if (length == 0) {
		pool = NULL;
	} else {
		pool = (char *)malloc(length + 1);	// +1 for terminator
		memcpy(pool, lpStr, length);
		pool[length] = '\0';
	}
	GlobalUnlock(hGlobal);
	if (empty) {
		EmptyClipboard();
	}
	CloseClipboard();

	return pool;
}


BOOL autologin_sent_none;
static INT_PTR CALLBACK auth_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
									  LPARAM lParam)
{
	const int IDC_TIMER1 = 300; // 自動ログインが有効なとき
	const int IDC_TIMER2 = 301; // サポートされているメソッドを自動チェック(CheckAuthListFirst)
	const int IDC_TIMER3 = 302; // challenge で ask4passwd でCheckAuthListFirst が FALSE のとき
	const int autologin_timeout = 10; // ミリ秒
	PTInstVar pvar;
	static BOOL UseControlChar;
	static BOOL ShowPassPhrase;
	static HICON hIconDropdown;
	TCHAR uimsg[MAX_UIMSG];

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		pvar->auth_state.auth_dialog = dlg;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		UseControlChar = TRUE;
		ShowPassPhrase = FALSE;
		init_auth_dlg(pvar, dlg, &UseControlChar);

		// "▼"画像をセットする
		hIconDropdown = LoadImage(hInst, MAKEINTRESOURCE(IDI_DROPDOWN),
								  IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
		SendMessage(GetDlgItem(dlg, IDC_USERNAME_OPTION), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconDropdown);
		SendMessage(GetDlgItem(dlg, IDC_SSHPASSWORD_OPTION), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconDropdown);

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
							pvar->auth_state.user =
								alloc_control_text(GetDlgItem(dlg, IDC_SSHUSERNAME));
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
						pvar->auth_state.user =
							alloc_control_text(GetDlgItem(dlg, IDC_SSHUSERNAME));
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
						pvar->auth_state.user =
							alloc_control_text(GetDlgItem(dlg, IDC_SSHUSERNAME));
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
			EndDialog(dlg, 0);
			return TRUE;

		case IDC_SSHUSERNAME:
			// ユーザ名がフォーカスを失ったとき (2007.9.29 maya)
			if (!(pvar->ssh_state.status_flags & STATUS_DONT_SEND_USER_NAME) &&
			    (pvar->ssh_state.status_flags & STATUS_HOST_OK) &&
			    HIWORD(wParam) == EN_KILLFOCUS) {
				// 設定が有効でまだ取りに行っていないなら
				if (SSHv2(pvar) &&
					pvar->session_settings.CheckAuthListFirst &&
					!pvar->tryed_ssh2_authlist) {
					// ダイアログのユーザ名を反映
					if (pvar->auth_state.user == NULL) {
						pvar->auth_state.user =
							alloc_control_text(GetDlgItem(dlg, IDC_SSHUSERNAME));
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
			char *clipboard = GetClipboardTextA(dlg, FALSE);
			GetI18nStrT("TTSSH", "DLG_AUTH_PASTE_CLIPBOARD",
						uimsg, _countof(uimsg),
						"Paste from &clipboard",
						pvar->ts->UILanguageFile);
			AppendMenu(hMenu, MF_ENABLED | MF_STRING | (clipboard == NULL ? MFS_DISABLED : 0), 1, uimsg);
			GetI18nStrT("ttssh", "DLG_AUTH_CLEAR_CLIPBOARD",
						uimsg, _countof(uimsg),
						"Paste from &clipboard and cl&ear clipboard",
						pvar->ts->UILanguageFile);
			AppendMenu(hMenu, MF_ENABLED | MF_STRING | (clipboard == NULL ? MFS_DISABLED : 0), 2, uimsg);
			GetI18nStrT("ttssh", "DLG_AUTH_USE_CONTORL_CHARACTERS",
						uimsg, _countof(uimsg),
						"Use control charac&ters",
						pvar->ts->UILanguageFile);
			AppendMenu(hMenu, MF_ENABLED | MF_STRING  | (UseControlChar ? MFS_CHECKED : 0), 3, uimsg);
			GetI18nStrT("ttssh", "DLG_AUTH_SHOW_PASSPHRASE",
						uimsg, _countof(uimsg),
						"&Show passphrase",
						pvar->ts->UILanguageFile);
			AppendMenu(hMenu, MF_ENABLED | MF_STRING | (ShowPassPhrase ? MFS_CHECKED : 0), 4, uimsg);
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
				clipboard = GetClipboardTextA(dlg, clear_clipboard);
				if (clipboard != NULL) {
					SetDlgItemTextA(dlg, IDC_SSHPASSWORD, clipboard);
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
					static wchar_t password_char;
					if (password_char == 0) {
						wchar_t c = (wchar_t)SendMessage(hWnd, EM_GETPASSWORDCHAR, 0, 0);
						password_char = c;
					}
					if (ShowPassPhrase) {
						SendMessage(hWnd, EM_SETPASSWORDCHAR, 0, 0);
					} else {
#if !defined(UNICODE)
						if (password_char < 0x100) {
							SendMessageA(hWnd, EM_SETPASSWORDCHAR, (WPARAM)password_char, 0);
						} else {
							// TODO W系直呼び ↓うまくいかない
							//SendMessageW(hWnd, EM_SETPASSWORDCHAR, (WPARAM)password_char, 0);
							SendMessageA(hWnd, EM_SETPASSWORDCHAR, (WPARAM)'*', 0);
						}
#else
						SendMessageW(hWnd, EM_SETPASSWORDCHAR, (WPARAM)password_char, 0);
#endif
					}
					//InvalidateRect(hWnd, NULL, TRUE);
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
			GetI18nStrT("TTSSH", "DLG_AUTH_USE_DEFAULT_USERNAME",
						uimsg, _countof(uimsg),
						"Use &default username",
						pvar->ts->UILanguageFile);
			AppendMenu(hMenu, MF_ENABLED | MF_STRING | (DisableDefaultUserName ? MFS_DISABLED : 0), 1,
					   uimsg);
			GetI18nStrT("TTSSH", "DLG_AUTH_USE_LOGON_USERNAME",
						uimsg, _countof(uimsg),
						"Use &logon username",
						pvar->ts->UILanguageFile);
			AppendMenu(hMenu, MF_ENABLED | MF_STRING, 2, uimsg);
			hWndButton = GetDlgItem(dlg, IDC_USERNAME_OPTION);
			GetWindowRect(hWndButton, &rect);
			result = TrackPopupMenu(hMenu, TPM_RETURNCMD, rect.left, rect.bottom, 0 , hWndButton, NULL);
			DestroyMenu(hMenu);
			switch (result) {
			case 1:
				SetDlgItemText(dlg, IDC_SSHUSERNAME, pvar->session_settings.DefaultUserName);
				goto after_user_name_set;
			case 2: {
				TCHAR user_name[UNLEN+1];
				DWORD len = _countof(user_name);
				BOOL r = GetUserName(user_name, &len);
				if (r == 0) {
					break;
				}
				SetDlgItemText(dlg, IDC_SSHUSERNAME, user_name);
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

	case WM_DESTROY:
		if (hIconDropdown != NULL) {
			DeleteObject(hIconDropdown);
		}
		return FALSE;

	default:
		return FALSE;
	}
}

char *AUTH_get_user_name(PTInstVar pvar)
{
	return pvar->auth_state.user;
}

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
		notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
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
			_strdup(pvar->session_settings.DefaultUserName);
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
	char uimsg[MAX_UIMSG];

	GetWindowText(dlg, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_TIS_TITLE", pvar, uimsg);
	SetWindowText(dlg, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHAUTHBANNER, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_TIS_BANNER", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHAUTHBANNER, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDOK, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_OK", pvar, uimsg);
	SetDlgItemText(dlg, IDOK, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_DISCONNECT", pvar, uimsg);
	SetDlgItemText(dlg, IDCANCEL, pvar->ts->UIMsg);

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
	char *password =
		alloc_control_text(GetDlgItem(dlg, IDC_SSHPASSWORD));

	pvar->auth_state.cur_cred.method = SSH_AUTH_TIS;
	pvar->auth_state.cur_cred.password = password;
	pvar->auth_state.auth_dialog = NULL;

	// add
	if (SSHv2(pvar)) {
		pvar->keyboard_interactive_password_input = 1;
		handle_SSH2_userauth_inforeq(pvar);
	} 

	SSH_notify_cred(pvar);

	EndDialog(dlg, 1);
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
			EndDialog(dlg, 0);
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
		LPCTSTR dialog_template;

		switch (pvar->auth_state.mode) {
		case TIS_AUTH_MODE:
			dialog_template = MAKEINTRESOURCE(IDD_SSHTISAUTH);
			dlg_proc = TIS_dlg_proc;
			break;
		case GENERIC_AUTH_MODE:
		default:
			dialog_template = MAKEINTRESOURCE(IDD_SSHAUTH);
			dlg_proc = auth_dlg_proc;
		}

		if (!DialogBoxParam(hInst, dialog_template,
		                    cur_active !=
		                    NULL ? cur_active : pvar->NotificationWindow,
		                    dlg_proc, (LPARAM) pvar) == -1) {
			UTIL_get_lang_msg("MSG_CREATEWINDOW_AUTH_ERROR", pvar,
			                  "Unable to display authentication dialog box.\n"
			                  "Connection terminated.");
			notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
		}
	}
}

static void init_default_auth_dlg(PTInstVar pvar, HWND dlg)
{
	int id;
	TCHAR user_name[UNLEN+1];
	DWORD len;
	TCHAR uimsg[MAX_UIMSG];
	TCHAR uimsg2[MAX_UIMSG];
	const static DlgTextInfo text_info[] = {
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
	};

	SetI18DlgStrs("TTSSH", dlg, text_info, _countof(text_info), pvar->ts->UILanguageFile);

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

	SetDlgItemText(dlg, IDC_SSHUSERNAME, pvar->settings.DefaultUserName);
	SetDlgItemText(dlg, IDC_RSAFILENAME,
	               pvar->settings.DefaultRSAPrivateKeyFile);
	SetDlgItemText(dlg, IDC_HOSTRSAFILENAME,
	               pvar->settings.DefaultRhostsHostPrivateKeyFile);
	SetDlgItemText(dlg, IDC_LOCALUSERNAME,
	               pvar->settings.DefaultRhostsLocalUserName);

	if (pvar->settings.CheckAuthListFirst) {
		CheckDlgButton(dlg, IDC_CHECKAUTH, TRUE);
	}

	if (pvar->settings.DefaultUserType == 1 &&
		pvar->session_settings.DefaultUserName[0] == 0) {
		// 空なので「入力しない」にしておく
		pvar->settings.DefaultUserType = 0;
	}
	id = pvar->settings.DefaultUserType == 1 ? IDC_SSH_DEFAULTUSERNAME :
		pvar->settings.DefaultUserType == 2 ? IDC_SSH_WINDOWS_USERNAME :
		IDC_SSH_NO_USERNAME;
	CheckRadioButton(dlg, IDC_SSH_NO_USERNAME, IDC_SSH_WINDOWS_USERNAME, id);

	len = _countof(user_name);
	GetUserName(user_name, &len);

	GetDlgItemText(dlg, IDC_SSH_WINDOWS_USERNAME_TEXT, uimsg, _countof(uimsg));
	_stprintf_s(uimsg2, _countof(uimsg2), uimsg, user_name);
	SetDlgItemText(dlg, IDC_SSH_WINDOWS_USERNAME_TEXT, uimsg2);
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

	GetDlgItemText(dlg, IDC_SSHUSERNAME, pvar->settings.DefaultUserName,
	               sizeof(pvar->settings.DefaultUserName));
	GetDlgItemText(dlg, IDC_RSAFILENAME,
	               pvar->settings.DefaultRSAPrivateKeyFile,
	               sizeof(pvar->settings.DefaultRSAPrivateKeyFile));
	GetDlgItemText(dlg, IDC_HOSTRSAFILENAME,
	               pvar->settings.DefaultRhostsHostPrivateKeyFile,
	               sizeof(pvar->settings.DefaultRhostsHostPrivateKeyFile));
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

	EndDialog(dlg, 1);
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
			EndDialog(dlg, 0);
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

	if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHAUTHSETUP),
		               cur_active != NULL ? cur_active
		                                  : pvar->NotificationWindow,
		               default_auth_dlg_proc, (LPARAM) pvar) == -1) {
		UTIL_get_lang_msg("MSG_CREATEWINDOW_AUTHSETUP_ERROR", pvar,
		                  "Unable to display authentication setup dialog box.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
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
			UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', using %s");
			_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg,
			            pvar->auth_state.user,
			            get_auth_method_name(pvar->auth_state.cur_cred.method));

			if (pvar->auth_state.cur_cred.method == SSH_AUTH_RSA) {
				UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO2", pvar, " with %s key");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg,
				            "RSA");
				strncat_s(dest, len, buf, _TRUNCATE);
			}
			else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
				UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO3", pvar, " with %s key from Pageant");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg,
				            "RSA");
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
				UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', using %s");
				_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg,
				            pvar->auth_state.user, method);
			}
			else if (pvar->auth_state.cur_cred.method == SSH_AUTH_RSA) {
				method = get_auth_method_name(pvar->auth_state.cur_cred.method);
				UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', using %s");
				_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg,
				            pvar->auth_state.user,
				            get_auth_method_name(pvar->auth_state.cur_cred.method));

				UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO2", pvar, " with %s key");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg,
				            ssh_key_type(pvar->auth_state.cur_cred.key_pair->type));
				strncat_s(dest, len, buf, _TRUNCATE);
			}
			else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
				int key_len = get_uint32_MSBfirst(pvar->pageant_curkey + 4);
				char *s = (char *)malloc(key_len+1);

				method = get_auth_method_name(pvar->auth_state.cur_cred.method);
				UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', using %s");
				_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg,
				            pvar->auth_state.user,
				            get_auth_method_name(pvar->auth_state.cur_cred.method));

				memcpy(s, pvar->pageant_curkey+4+4, key_len);
				s[key_len] = '\0';
				UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO3", pvar, " with %s key from Pageant");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg,
				            ssh_key_type(get_keytype_from_name(s)));
				strncat_s(dest, len, buf, _TRUNCATE);

				free(s);
			}
		}

	} else {
		UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', using %s");
		_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg, pvar->auth_state.user,
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

void AUTH_end(PTInstVar pvar)
{
	destroy_malloced_string(&pvar->auth_state.user);
	destroy_malloced_string(&pvar->auth_state.TIS_prompt);

	AUTH_destroy_cur_cred(pvar);
}
