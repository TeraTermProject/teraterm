/*
Copyright (c) 1998-2001, Robert O'Callahan
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.

The name of Robert O'Callahan may not be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ttxssh.h"
#include "util.h"
#include "ssh.h"

#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include "resource.h"
#include "keyfiles.h"
#include "libputty.h"

#define AUTH_START_USER_AUTH_ON_ERROR_END 1

#define MAX_AUTH_CONTROL IDC_SSHUSEPAGEANT

static HFONT DlgAuthFont;
static HFONT DlgTisFont;
static HFONT DlgAuthSetupFont;

void destroy_malloced_string(char FAR * FAR * str)
{
	if (*str != NULL) {
		memset(*str, 0, strlen(*str));
		free(*str);
		*str = NULL;
	}
}

static int auth_types_to_control_IDs[] = {
	-1, IDC_SSHUSERHOSTS, IDC_SSHUSERSA, IDC_SSHUSEPASSWORD,
	IDC_SSHUSERHOSTS, IDC_SSHUSETIS, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, IDC_SSHUSEPAGEANT, -1
};

static LRESULT CALLBACK password_wnd_proc(HWND control, UINT msg,
                                          WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CHAR:
		if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
			char chars[] = { (char) wParam, 0 };

			SendMessage(control, EM_REPLACESEL, (WPARAM) TRUE,
			            (LPARAM) (char FAR *) chars);
			return 0;
		}
	}

	return CallWindowProc((WNDPROC) GetWindowLong(control, GWL_USERDATA),
	                      control, msg, wParam, lParam);
}

static void init_password_control(HWND dlg)
{
	HWND passwordControl = GetDlgItem(dlg, IDC_SSHPASSWORD);

	SetWindowLong(passwordControl, GWL_USERDATA,
	              SetWindowLong(passwordControl, GWL_WNDPROC,
	                            (LONG) password_wnd_proc));

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

static void init_auth_dlg(PTInstVar pvar, HWND dlg)
{
	int default_method = pvar->session_settings.DefaultAuthMethod;
	char uimsg[MAX_UIMSG];

	GetWindowText(dlg, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_TITLE", pvar, uimsg);
	SetWindowText(dlg, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHAUTHBANNER, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_BANNER", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHAUTHBANNER, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHAUTHBANNER2, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_BANNER2", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHAUTHBANNER2, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSERNAMELABEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_USERNAME", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSERNAMELABEL, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHPASSWORDCAPTION, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_PASSWORD", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHPASSWORDCAPTION, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_REMEMBER_PASSWORD, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_REMEMBER_PASSWORD", pvar, uimsg);
	SetDlgItemText(dlg, IDC_REMEMBER_PASSWORD, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_FORWARD_AGENT, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_FWDAGENT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_FORWARD_AGENT, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSEPASSWORD, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_METHOD_PASSWORD", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSEPASSWORD, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSERSA, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_METHOD_RSA", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSERSA, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSERHOSTS, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_METHOD_RHOST", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSERHOSTS, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSEPAGEANT, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_METHOD_PAGEANT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSEPAGEANT, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_CHOOSERSAFILE, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_PRIVATEKEY", pvar, uimsg);
	SetDlgItemText(dlg, IDC_CHOOSERSAFILE, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_LOCALUSERNAMELABEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_LOCALUSER", pvar, uimsg);
	SetDlgItemText(dlg, IDC_LOCALUSERNAMELABEL, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_CHOOSEHOSTRSAFILE, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_HOST_PRIVATEKEY", pvar, uimsg);
	SetDlgItemText(dlg, IDC_CHOOSEHOSTRSAFILE, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDOK, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_OK", pvar, uimsg);
	SetDlgItemText(dlg, IDOK, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_DISCONNECT", pvar, uimsg);
	SetDlgItemText(dlg, IDCANCEL, pvar->ts->UIMsg);

	init_auth_machine_banner(pvar, dlg);
	init_password_control(dlg);

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
		                  "Use challenge/response to log in(&TIS)");
		SetDlgItemText(dlg, IDC_SSHUSETIS, pvar->ts->UIMsg);
	} else {
		UTIL_get_lang_msg("DLG_AUTH_METHOD_CHALLENGE2", pvar,
		                  "Use &challenge/response to log in(keyboard-interactive)");
		SetDlgItemText(dlg, IDC_SSHUSETIS, pvar->ts->UIMsg);
	}

	if (pvar->auth_state.user != NULL) {
		SetDlgItemText(dlg, IDC_SSHUSERNAME, pvar->auth_state.user);
		EnableWindow(GetDlgItem(dlg, IDC_SSHUSERNAME), FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_SSHUSERNAMELABEL), FALSE);
	}
	if (strlen(pvar->ssh2_username) > 0) {
		SetDlgItemText(dlg, IDC_SSHUSERNAME, pvar->ssh2_username);
		if (pvar->ssh2_autologin == 1) {
			EnableWindow(GetDlgItem(dlg, IDC_SSHUSERNAME), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_SSHUSERNAMELABEL), FALSE);
		}
	}
	else if (pvar->session_settings.DefaultUserName[0] != 0) {
		SetDlgItemText(dlg, IDC_SSHUSERNAME,
		               pvar->session_settings.DefaultUserName);
	}

	if (strlen(pvar->ssh2_password) > 0) {
		SetDlgItemText(dlg, IDC_SSHPASSWORD, pvar->ssh2_password);
		if (pvar->ssh2_autologin == 1) {
			EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORDCAPTION), FALSE);
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
		SetDlgItemText(dlg, IDC_SSHPASSWORD, "");

	// /auth=pageant を追加
	} else if (pvar->ssh2_authmethod == SSH_AUTH_PAGEANT) {
		CheckRadioButton(dlg, IDC_SSHUSEPASSWORD, MAX_AUTH_CONTROL, IDC_SSHUSEPAGEANT);
		EnableWindow(GetDlgItem(dlg, IDC_SSHPASSWORD), FALSE);
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

static char FAR *alloc_control_text(HWND ctl)
{
	int len = GetWindowTextLength(ctl);
	char FAR *result = malloc(len + 1);

	if (result != NULL) {
		GetWindowText(ctl, result, len + 1);
		result[len] = 0;
	}

	return result;
}

static int get_key_file_name(HWND parent, char FAR * buf, int bufsize, PTInstVar pvar)
{
	OPENFILENAME params;
	char fullname_buf[2048] = "identity";
	char filter[MAX_UIMSG];

	ZeroMemory(&params, sizeof(params));
	params.lStructSize = sizeof(OPENFILENAME);
	params.hwndOwner = parent;
	// フィルタの追加 (2004.12.19 yutaka)
	// 3ファイルフィルタの追加 (2005.4.26 yutaka)
	UTIL_get_lang_msg("FILEDLG_OPEN_PRIVATEKEY_FILTER", pvar,
	                  "identity files\\0identity;id_rsa;id_dsa\\0identity(RSA1)\\0identity\\0id_rsa(SSH2)\\0id_rsa\\0id_dsa(SSH2)\\0id_dsa\\0all(*.*)\\0*.*\\0\\0");
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
	                  "Choose a file with the RSA/DSA private key");
	params.lpstrTitle = pvar->ts->UIMsg;
	params.Flags =
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
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
	char FAR *password =
		alloc_control_text(GetDlgItem(dlg, IDC_SSHPASSWORD));
	CRYPTKeyPair FAR *key_pair = NULL;

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
		char buf[2048];
		int file_ctl_ID =
			method == SSH_AUTH_RSA ? IDC_RSAFILENAME : IDC_HOSTRSAFILENAME;

		buf[0] = 0;
		GetDlgItemText(dlg, file_ctl_ID, buf, sizeof(buf));
		if (buf[0] == 0) {
			UTIL_get_lang_msg("MSG_KEYSPECIFY_ERROR", pvar,
			                  "You must specify a file containing the RSA/DSA private key.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			SetFocus(GetDlgItem(dlg, file_ctl_ID));
			destroy_malloced_string(&password);
			return FALSE;
		} 
		
		if (SSHv1(pvar)) {
			BOOL invalid_passphrase = FALSE;

			key_pair = KEYFILES_read_private_key(pvar, buf, password,
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

			memset(errmsg, 0, sizeof(errmsg));
			//GetCurrentDirectory(sizeof(errmsg), errmsg);

			key_pair = read_SSH2_private_key(pvar, buf, password,
			                                 &invalid_passphrase,
			                                 FALSE,
			                                 errmsg,
			                                 sizeof(errmsg)
			                                );

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
			                  "TTSSH and selecting \"SSH Authentication...\" from the Setup menu"
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
	if (DlgAuthFont != NULL) {
		DeleteObject(DlgAuthFont);
	}

	return TRUE;
}

BOOL autologin_sent_none;
static BOOL CALLBACK auth_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
                                   LPARAM lParam)
{
	const int IDC_TIMER1 = 300; // 自動ログインが有効なとき
	const int IDC_TIMER2 = 301; // サポートされているメソッドを自動チェック(CheckAuthListFirst)
	const int IDC_TIMER3 = 302; // challenge で ask4passwd でCheckAuthListFirst が FALSE のとき
	const int autologin_timeout = 10; // ミリ秒
	PTInstVar pvar;
	LOGFONT logfont;
	HFONT font;

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		pvar->auth_state.auth_dialog = dlg;
		SetWindowLong(dlg, DWL_USER, lParam);

		init_auth_dlg(pvar, dlg);

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgAuthFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_SSHAUTHBANNER, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHAUTHBANNER2, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSERNAMELABEL, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSERNAME, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHPASSWORDCAPTION, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHPASSWORD, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_REMEMBER_PASSWORD, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_FORWARD_AGENT, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSEPASSWORD, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSERSA, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CHOOSERSAFILE, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_RSAFILENAME, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSERHOSTS, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_LOCALUSERNAMELABEL, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_LOCALUSERNAME, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CHOOSEHOSTRSAFILE, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTRSAFILENAME, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSETIS, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSEPAGEANT, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDOK, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDCANCEL, WM_SETFONT, (WPARAM)DlgAuthFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgAuthFont = NULL;
		}

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
		return FALSE;			/* because we set the focus */

	case WM_TIMER:
		pvar = (PTInstVar) GetWindowLong(dlg, DWL_USER);
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
		pvar = (PTInstVar) GetWindowLong(dlg, DWL_USER);

		switch (LOWORD(wParam)) {
		case IDOK:
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
			pvar->auth_state.auth_dialog = NULL;
			notify_closed_connection(pvar);
			EndDialog(dlg, 0);

			if (DlgAuthFont != NULL) {
				DeleteObject(DlgAuthFont);
			}

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

		default:
			return FALSE;
		}

	default:
		return FALSE;
	}
}

char FAR *AUTH_get_user_name(PTInstVar pvar)
{
	return pvar->auth_state.user;
}

int AUTH_set_supported_auth_types(PTInstVar pvar, int types)
{
	char buf[1024];

	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
	          "Server reports supported authentication method mask = %d",
	          types);
	buf[sizeof(buf) - 1] = 0;
	notify_verbose_message(pvar, buf, LOG_LEVEL_VERBOSE);

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
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		return 0;
	} else {
		if (pvar->auth_state.auth_dialog != NULL) {
			update_server_supported_types(pvar,
			                              pvar->auth_state.auth_dialog);
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
	init_password_control(dlg);

	if (pvar->auth_state.TIS_prompt != NULL) {
		if (strlen(pvar->auth_state.TIS_prompt) > 10000) {
			pvar->auth_state.TIS_prompt[10000] = 0;
		}
		SetDlgItemText(dlg, IDC_SSHAUTHBANNER2,
					   pvar->auth_state.TIS_prompt);
		destroy_malloced_string(&pvar->auth_state.TIS_prompt);
	}
}

static BOOL end_TIS_dlg(PTInstVar pvar, HWND dlg)
{
	char FAR *password =
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

static BOOL CALLBACK TIS_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
                                  LPARAM lParam)
{
	PTInstVar pvar;
	LOGFONT logfont;
	HFONT font;

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		pvar->auth_state.auth_dialog = dlg;
		SetWindowLong(dlg, DWL_USER, lParam);

		init_TIS_dlg(pvar, dlg);

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgTisFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_SSHAUTHBANNER, WM_SETFONT, (WPARAM)DlgTisFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHAUTHBANNER2, WM_SETFONT, (WPARAM)DlgTisFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHPASSWORD, WM_SETFONT, (WPARAM)DlgTisFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDOK, WM_SETFONT, (WPARAM)DlgTisFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDCANCEL, WM_SETFONT, (WPARAM)DlgTisFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgTisFont = NULL;
		}

		// /auth=challenge を追加 (2007.10.5 maya)
		if (pvar->ssh2_autologin == 1) {
			SetDlgItemText(dlg, IDC_SSHPASSWORD, pvar->ssh2_password);
			SendMessage(dlg, WM_COMMAND, IDOK, 0);
		}

		return FALSE;			/* because we set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLong(dlg, DWL_USER);

		switch (LOWORD(wParam)) {
		case IDOK:
			if (DlgTisFont != NULL) {
				DeleteObject(DlgTisFont);
			}

			return end_TIS_dlg(pvar, dlg);

		case IDCANCEL:			/* kill the connection */
			pvar->auth_state.auth_dialog = NULL;
			notify_closed_connection(pvar);
			EndDialog(dlg, 0);

			if (DlgTisFont != NULL) {
				DeleteObject(DlgTisFont);
			}

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
			notify_fatal_error(pvar, pvar->ts->UIMsg);
		}
	}
}

static void init_default_auth_dlg(PTInstVar pvar, HWND dlg)
{
	char uimsg[MAX_UIMSG];

	GetWindowText(dlg, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTHSETUP_TITLE", pvar, uimsg);
	SetWindowText(dlg, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHAUTHBANNER, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTHSETUP_BANNER", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHAUTHBANNER, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSERNAMELABEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTHSETUP_USERNAME", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSERNAMELABEL, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSEPASSWORD, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTHSETUP_METHOD_PASSWORD", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSEPASSWORD, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSERSA, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTHSETUP_METHOD_RSA", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSERSA, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSERHOSTS, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTHSETUP_METHOD_RHOST", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSERHOSTS, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSETIS, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTHSETUP_METHOD_CHALLENGE", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSETIS, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHUSEPAGEANT, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTHSETUP_METHOD_PAGEANT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHUSEPAGEANT, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_CHOOSERSAFILE, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_PRIVATEKEY", pvar, uimsg);
	SetDlgItemText(dlg, IDC_CHOOSERSAFILE, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_LOCALUSERNAMELABEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_LOCALUSER", pvar, uimsg);
	SetDlgItemText(dlg, IDC_LOCALUSERNAMELABEL, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_CHOOSEHOSTRSAFILE, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTH_HOST_PRIVATEKEY", pvar, uimsg);
	SetDlgItemText(dlg, IDC_CHOOSEHOSTRSAFILE, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_CHECKAUTH, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_AUTHSETUP_CHECKAUTH", pvar, uimsg);
	SetDlgItemText(dlg, IDC_CHECKAUTH, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDOK, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_OK", pvar, uimsg);
	SetDlgItemText(dlg, IDOK, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_CANCEL", pvar, uimsg);
	SetDlgItemText(dlg, IDCANCEL, pvar->ts->UIMsg);

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

	if (IsDlgButtonChecked(dlg, IDC_CHECKAUTH)) {
		pvar->settings.CheckAuthListFirst = TRUE;
	}
	else {
		pvar->settings.CheckAuthListFirst = FALSE;
	}

	EndDialog(dlg, 1);
	return TRUE;
}

static BOOL CALLBACK default_auth_dlg_proc(HWND dlg, UINT msg,
										   WPARAM wParam, LPARAM lParam)
{
	PTInstVar pvar;
	LOGFONT logfont;
	HFONT font;

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		SetWindowLong(dlg, DWL_USER, lParam);

		init_default_auth_dlg(pvar, dlg);

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgAuthSetupFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_SSHAUTHBANNER, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSERNAMELABEL, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSERNAME, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSEPASSWORD, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSERSA, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CHOOSERSAFILE, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_RSAFILENAME, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSERHOSTS, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_LOCALUSERNAMELABEL, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_LOCALUSERNAME, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CHOOSEHOSTRSAFILE, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTRSAFILENAME, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSETIS, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHUSEPAGEANT, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CHECKAUTH, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDOK, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDCANCEL, WM_SETFONT, (WPARAM)DlgAuthSetupFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgAuthSetupFont = NULL;
		}

		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLong(dlg, DWL_USER);

		switch (LOWORD(wParam)) {
		case IDOK:

			if (DlgAuthSetupFont != NULL) {
				DeleteObject(DlgAuthSetupFont);
			}

			return end_default_auth_dlg(pvar, dlg);

		case IDCANCEL:
			EndDialog(dlg, 0);

			if (DlgAuthSetupFont != NULL) {
				DeleteObject(DlgAuthSetupFont);
			}

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

void AUTH_set_TIS_mode(PTInstVar pvar, char FAR * prompt, int len)
{
	if (pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) {
		pvar->auth_state.mode = TIS_AUTH_MODE;

		destroy_malloced_string(&pvar->auth_state.TIS_prompt);
		pvar->auth_state.TIS_prompt = malloc(len + 1);
		memcpy(pvar->auth_state.TIS_prompt, prompt, len);
		pvar->auth_state.TIS_prompt[len] = 0;
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
		CRYPT_free_key_pair(pvar->auth_state.cur_cred.key_pair);
		pvar->auth_state.cur_cred.key_pair = NULL;
	}
}

static char FAR *get_auth_method_name(SSHAuthMethod auth)
{
	switch (auth) {
	case SSH_AUTH_PASSWORD:
		return "password";
	case SSH_AUTH_RSA:
		return "RSA";
	case SSH_AUTH_PAGEANT:
		return "RSA (with Pageant)";
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

void AUTH_get_auth_info(PTInstVar pvar, char FAR * dest, int len)
{
	char *method = "unknown";

	if (pvar->auth_state.user == NULL) {
		strncpy_s(dest, len, "None", _TRUNCATE);
	} else if (pvar->auth_state.cur_cred.method != SSH_AUTH_NONE) {
		if (SSHv1(pvar)) {
			UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', using %s");
			_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg, pvar->auth_state.user,
			            get_auth_method_name(pvar->auth_state.cur_cred.method));

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
				_snprintf_s(dest, len, _TRUNCATE,
				            pvar->ts->UIMsg, pvar->auth_state.user, method);

			} else {
				if (pvar->auth_state.cur_cred.method == SSH_AUTH_RSA) {
					if (pvar->auth_state.cur_cred.key_pair->RSA_key != NULL) {
						method = "RSA";
					} else if (pvar->auth_state.cur_cred.key_pair->DSA_key != NULL) {
						method = "DSA";
					}
				}
				else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
					int len = get_uint32_MSBfirst(pvar->pageant_curkey + 4);
					char *s = (char *)malloc(len+1);
					enum hostkey_type keytype;

					memcpy(s, pvar->pageant_curkey+4+4, len);
					s[len] = '\0';
					keytype = get_keytype_from_name(s);
					if (keytype == KEY_RSA) {
						method = "RSA with Pageant";
					} else if (keytype == KEY_DSA) {
						method = "DSA with Pageant";
					}
					free(s);
				}
				UTIL_get_lang_msg("DLG_ABOUT_AUTH_INFO", pvar, "User '%s', using %s");
				_snprintf_s(dest, len, _TRUNCATE,
				            pvar->ts->UIMsg, pvar->auth_state.user, method);
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
