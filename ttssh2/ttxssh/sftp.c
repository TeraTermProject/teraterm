/*
Copyright (c) 2008-2012 TeraTerm Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "ttxssh.h"
#include "util.h"
#include "resource.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/engine.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#include "buffer.h"
#include "ssh.h"
#include "crypt.h"
#include "fwd.h"
#include "ssh.h"
#include "sftp.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>


#define WM_USER_CONSOLE (WM_USER + 1)

static void sftp_console_message(PTInstVar pvar, Channel_t *c, char *fmt, ...)
{
	char tmp[1024];
	va_list arg;

	va_start(arg, fmt);
	_vsnprintf(tmp, sizeof(tmp), fmt, arg);
	va_end(arg);

	SendMessage(c->sftp.console_window, WM_USER_CONSOLE, 0, (LPARAM)tmp);
	notify_verbose_message(pvar, tmp, LOG_LEVEL_VERBOSE);
}

static void sftp_do_syslog(PTInstVar pvar, int level, char *fmt, ...)
{
	char tmp[1024];
	va_list arg;

	va_start(arg, fmt);
	_vsnprintf(tmp, sizeof(tmp), fmt, arg);
	va_end(arg);

	notify_verbose_message(pvar, tmp, level);
}

static void sftp_syslog(PTInstVar pvar, char *fmt, ...)
{
	char tmp[1024];
	va_list arg;

	va_start(arg, fmt);
	_vsnprintf(tmp, sizeof(tmp), fmt, arg);
	va_end(arg);

	notify_verbose_message(pvar, tmp, LOG_LEVEL_VERBOSE);
}

// SFTP専用バッファを確保する。SCPとは異なり、先頭に後続のデータサイズを埋め込む。
//
// buffer_t
//    +---------+------------------------------------+
//    | msg_len | data                               |  
//    +---------+------------------------------------+
//       4byte   <------------- msg_len ------------->
//
static void sftp_buffer_alloc(buffer_t **message)
{
	buffer_t *msg;

	msg = buffer_init();
	if (msg == NULL) {
		goto error;
	}
	// Message length(4byte)
	buffer_put_int(msg, 0); 

	*message = msg;

error:
	assert(msg != NULL);
	return;
}

static void sftp_buffer_free(buffer_t *message)
{
	buffer_free(message);
}

// サーバにSFTPパケットを送信する。
static void sftp_send_msg(PTInstVar pvar, Channel_t *c, buffer_t *msg)
{
	char *p;
	int len;

	len = buffer_len(msg);
	p = buffer_ptr(msg);
	// 最初にメッセージサイズを格納する。
	set_uint32(p, len - 4);
	// ペイロードの送信。
	SSH2_send_channel_data(pvar, c, p, len);
}

// サーバから受信したSFTPパケットをバッファに格納する。
static void sftp_get_msg(PTInstVar pvar, Channel_t *c, unsigned char *data, unsigned int buflen, buffer_t **message)
{
	buffer_t *msg = *message;
	int msg_len;

	// バッファを確保し、データをすべて放り込む。以降は buffer_t 型を通して操作する。
	// そうしたほうが OpenSSH のコードとの親和性が良くなるため。
	buffer_clear(msg);
	buffer_append(msg, data, buflen);
	buffer_rewind(msg);

	msg_len = buffer_get_int(msg);
	if (msg_len > SFTP_MAX_MSG_LENGTH) {
		// TODO:
		sftp_syslog(pvar, "Received message too long %u", msg_len);
		goto error;
	}
	if (msg_len + 4 != buflen) {
		// TODO:
		sftp_syslog(pvar, "Buffer length %u is invalid", buflen);
		goto error;
	}

error:
	return;
}

static void sftp_send_string_request(PTInstVar pvar, Channel_t *c, unsigned int id, unsigned int code,
									 char *s, unsigned int len)
{
	buffer_t *msg;

	sftp_buffer_alloc(&msg);
	buffer_put_char(msg, code);
	buffer_put_int(msg, id);
	buffer_put_string(msg, s, len);
	sftp_send_msg(pvar, c, msg);
	sftp_syslog(pvar, "Sent message fd %d T:%u I:%u", c->remote_id, code, id);
	sftp_buffer_free(msg);
}


// SFTP通信開始前のネゴシエーション
// based on do_init()#sftp-client.c(OpenSSH 6.0)
void sftp_do_init(PTInstVar pvar, Channel_t *c)
{
	buffer_t *msg;

	// SFTP管理構造体の初期化
	memset(&c->sftp, 0, sizeof(c->sftp));
	c->sftp.state = SFTP_INIT;
	c->sftp.transfer_buflen = DEFAULT_COPY_BUFLEN;
	c->sftp.num_requests = DEFAULT_NUM_REQUESTS;
	c->sftp.exts = 0;
	c->sftp.limit_kbps = 0;

	// ネゴシエーションの開始
	sftp_buffer_alloc(&msg);
	buffer_put_char(msg, SSH2_FXP_INIT); 
	buffer_put_int(msg, SSH2_FILEXFER_VERSION);
	sftp_send_msg(pvar, c, msg);
	sftp_buffer_free(msg);

	sftp_syslog(pvar, "SFTP client version %u", SSH2_FILEXFER_VERSION);
}

static void sftp_do_init_recv(PTInstVar pvar, Channel_t *c, buffer_t *msg)
{
	unsigned int type;

	type = buffer_get_char(msg);
	if (type != SSH2_FXP_VERSION) {
		goto error;
	}
	c->sftp.version = buffer_get_int(msg);
	sftp_syslog(pvar, "SFTP server version %u, remote version %u", type, c->sftp.version);

	while (buffer_remain_len(msg) > 0) {
		char *name = buffer_get_string_msg(msg, NULL);
		char *value = buffer_get_string_msg(msg, NULL);
		int known = 0;

        if (strcmp(name, "posix-rename@openssh.com") == 0 &&
            strcmp(value, "1") == 0) {
            c->sftp.exts |= SFTP_EXT_POSIX_RENAME;
            known = 1;
        } else if (strcmp(name, "statvfs@openssh.com") == 0 &&
            strcmp(value, "2") == 0) {
            c->sftp.exts |= SFTP_EXT_STATVFS;
            known = 1;
        } else if (strcmp(name, "fstatvfs@openssh.com") == 0 &&
            strcmp(value, "2") == 0) {
            c->sftp.exts |= SFTP_EXT_FSTATVFS;
            known = 1;
        } else if (strcmp(name, "hardlink@openssh.com") == 0 &&
            strcmp(value, "1") == 0) {
            c->sftp.exts |= SFTP_EXT_HARDLINK;
            known = 1;
        }
        if (known) {
            sftp_syslog(pvar, "Server supports extension \"%s\" revision %s",
                name, value);
        } else {
            sftp_syslog(pvar, "Unrecognised server extension \"%s\"", name);
        }

		free(name);
		free(value);
	}

	if (c->sftp.version == 0) {
		c->sftp.transfer_buflen = min(c->sftp.transfer_buflen, 20480);
	}
	c->sftp.limit_kbps = 0;
	if (c->sftp.limit_kbps > 0) {
		// TODO:
	}

	sftp_syslog(pvar, "Connected to SFTP server.");

error:
	return;
}

// パスの送信
// based on do_realpath()#sftp-client.c(OpenSSH 6.0)
static void sftp_do_realpath(PTInstVar pvar, Channel_t *c, char *path)
{
	unsigned int id;

	strncpy_s(c->sftp.path, sizeof(c->sftp.path), path, _TRUNCATE);
	id = c->sftp.msg_id++;
	sftp_send_string_request(pvar, c, id, SSH2_FXP_REALPATH, path, strlen(path));
}

/* Convert from SSH2_FX_ status to text error message */
static const char *fx2txt(int status)
{       
    switch (status) {
    case SSH2_FX_OK:
        return("No error");
    case SSH2_FX_EOF:
        return("End of file");
    case SSH2_FX_NO_SUCH_FILE:
        return("No such file or directory");
    case SSH2_FX_PERMISSION_DENIED:
        return("Permission denied");
    case SSH2_FX_FAILURE:
        return("Failure");
    case SSH2_FX_BAD_MESSAGE:
        return("Bad message");
    case SSH2_FX_NO_CONNECTION:
        return("No connection");
    case SSH2_FX_CONNECTION_LOST:
        return("Connection lost");
    case SSH2_FX_OP_UNSUPPORTED:
        return("Operation unsupported");
    default:
        return("Unknown status");
    }
    /* NOTREACHED */
}

static char *sftp_do_realpath_recv(PTInstVar pvar, Channel_t *c, buffer_t *msg)
{
	unsigned int type, expected_id, count, id;
	char *filename = NULL, *longname;

	type = buffer_get_char(msg);
	id = buffer_get_int(msg);

	expected_id = c->sftp.msg_id - 1; 
	if (id != expected_id) {
		sftp_syslog(pvar, "ID mismatch (%u != %u)", id, expected_id);
		goto error;
	}

	if (type == SSH2_FXP_STATUS) {
		unsigned int status = buffer_get_int(msg);

		sftp_syslog(pvar, "Couldn't canonicalise: %s", fx2txt(status));
		goto error;
	} else if (type != SSH2_FXP_NAME) {
        sftp_syslog(pvar, "Expected SSH2_FXP_NAME(%u) packet, got %u",
            SSH2_FXP_NAME, type);
		goto error;
	}

	count = buffer_get_int(msg);
	if (count != 1) {
		sftp_syslog(pvar, "Got multiple names (%d) from SSH_FXP_REALPATH", count);
		goto error;
	}

	filename = buffer_get_string_msg(msg, NULL);
	longname = buffer_get_string_msg(msg, NULL);
	//a = decode_attrib(&msg);

	sftp_console_message(pvar, c, "SSH_FXP_REALPATH %s -> %s", c->sftp.path, filename);

	free(longname);

error:
	return (filename);
}


/*
 * SFTP コマンドラインコンソール
 */
static WNDPROC hEditProc;

static LRESULT CALLBACK EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_KEYDOWN:
		if ((int)wParam == VK_RETURN) {
			char buf[512];

			GetWindowText(hwnd, buf, sizeof(buf));
			SetWindowText(hwnd, "");
			if (buf[0] != '\0') {
				SendDlgItemMessage(GetParent(hwnd), IDC_SFTP_CONSOLE, EM_REPLACESEL, 0, (LPARAM) buf);
				SendDlgItemMessage(GetParent(hwnd), IDC_SFTP_CONSOLE, EM_REPLACESEL, 0,
								   (LPARAM) (char FAR *) "\r\n");
			}
		}
		break;
	default:
		return (CallWindowProc(hEditProc, hwnd, uMsg, wParam, lParam));
	}

	return 0L;
}

static LRESULT CALLBACK OnSftpConsoleDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static HFONT DlgDragDropFont = NULL;
	LOGFONT logfont;
	HFONT font;
	HWND hEdit;

	switch (msg) {
		case WM_INITDIALOG:
			font = (HFONT)SendMessage(hDlgWnd, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			DlgDragDropFont = NULL;

			hEdit = GetDlgItem(hDlgWnd, IDC_SFTP_EDIT);
			SetFocus(hEdit);

			// エディットコントロールのサブクラス化
			hEditProc = (WNDPROC)GetWindowLong(hEdit, GWL_WNDPROC);
			SetWindowLong(hEdit, GWL_WNDPROC, (LONG)EditProc);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					break;

				case IDCANCEL:
					if (DlgDragDropFont != NULL) {
						DeleteObject(DlgDragDropFont);
					}
					EndDialog(hDlgWnd, IDCANCEL);
					DestroyWindow(hDlgWnd);
					break;

				default:
					return FALSE;
			}

		case WM_USER_CONSOLE:
			SendDlgItemMessage(hDlgWnd, IDC_SFTP_CONSOLE, EM_REPLACESEL, 0, (LPARAM) lp);
			SendDlgItemMessage(hDlgWnd, IDC_SFTP_CONSOLE, EM_REPLACESEL, 0,
							   (LPARAM) (char FAR *) "\r\n");
			return TRUE;

#if 0
		case WM_SIZE:
			{
				// 再配置
				int dlg_w, dlg_h;
				RECT rc_dlg;
				RECT rc;
				POINT p;

				// 新しいダイアログのサイズを得る
				GetClientRect(hDlgWnd, &rc_dlg);
				dlg_w = rc_dlg.right;
				dlg_h = rc_dlg.bottom;

				// コマンドプロンプト
				GetWindowRect(GetDlgItem(hDlgWnd, IDC_SFTP_EDIT), &rc);
				p.x = rc.left;
				p.y = rc.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDC_SFTP_EDIT), 0,
 							 0, 0, dlg_w, p.y,
							 SWP_NOSIZE | SWP_NOZORDER);
			}
			return TRUE;
#endif

		default:
			return FALSE;
	}
	return TRUE;
}

// SFTP受信処理 -ステートマシーン-
void sftp_response(PTInstVar pvar, Channel_t *c, unsigned char *data, unsigned int buflen)
{
	buffer_t *msg;
	HWND hDlgWnd;

	/*
	 * Allocate buffer
	 */
	sftp_buffer_alloc(&msg);
	sftp_get_msg(pvar, c, data, buflen, &msg);

	if (c->sftp.state == SFTP_INIT) {
		sftp_do_init_recv(pvar, c, msg);

		// コンソールを起動する。
		hDlgWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SFTP_DIALOG), 
				pvar->cv->HWin, (DLGPROC)OnSftpConsoleDlgProc);	
		if (hDlgWnd != NULL) {
			c->sftp.console_window = hDlgWnd;
			ShowWindow(hDlgWnd, SW_SHOW);
		}

		sftp_do_realpath(pvar, c, ".");
		c->sftp.state = SFTP_CONNECTED;

	} else if (c->sftp.state == SFTP_CONNECTED) {
		char *remote_path;
		remote_path = sftp_do_realpath_recv(pvar, c, msg);

		c->sftp.state = SFTP_REALPATH;

	}

	/*
	 * Free buffer
	 */
	sftp_buffer_free(msg);
}
