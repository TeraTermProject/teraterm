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

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#include "ttxssh.h"
#include "util.h"
#include "resource.h"
#include "matcher.h"
#include "ssh.h"
#include "hosts.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>

#include <fcntl.h>
#include <io.h>
#include <errno.h>
#include <sys/stat.h>
#include <direct.h>

static HFONT DlgHostsAddFont;
static HFONT DlgHostsReplaceFont;

// BASE64構成文字列（ここでは'='は含まれていない）
static char base64[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


// ホストキーの初期化 (2006.3.21 yutaka)
static void init_hostkey(Key *key)
{
	key->type = KEY_UNSPEC;

	// SSH1
	key->bits = 0;
	if (key->exp != NULL) {
		free(key->exp);
		key->exp = NULL;
	}
	if (key->mod != NULL) {
		free(key->mod);
		key->mod = NULL;
	}

	// SSH2
	if (key->dsa != NULL) {
		DSA_free(key->dsa);
		key->dsa = NULL;
	}
	if (key->rsa != NULL) {
		RSA_free(key->rsa);
		key->rsa = NULL;
	}
}


static char FAR *FAR * parse_multi_path(char FAR * buf)
{
	int i;
	int ch;
	int num_paths = 1;
	char FAR *FAR * result;
	int last_path_index;

	for (i = 0; (ch = buf[i]) != 0; i++) {
		if (ch == ';') {
			num_paths++;
		}
	}

	result =
		(char FAR * FAR *) malloc(sizeof(char FAR *) * (num_paths + 1));

	last_path_index = 0;
	num_paths = 0;
	for (i = 0; (ch = buf[i]) != 0; i++) {
		if (ch == ';') {
			buf[i] = 0;
			result[num_paths] = _strdup(buf + last_path_index);
			num_paths++;
			buf[i] = ch;
			last_path_index = i + 1;
		}
	}
	if (i > last_path_index) {
		result[num_paths] = _strdup(buf + last_path_index);
		num_paths++;
	}
	result[num_paths] = NULL;
	return result;
}

void HOSTS_init(PTInstVar pvar)
{
	pvar->hosts_state.prefetched_hostname = NULL;
#if 0
	pvar->hosts_state.key_exp = NULL;
	pvar->hosts_state.key_mod = NULL;
#else
	init_hostkey(&pvar->hosts_state.hostkey);
#endif
	pvar->hosts_state.hosts_dialog = NULL;
	pvar->hosts_state.file_names = NULL;
}

void HOSTS_open(PTInstVar pvar)
{
	pvar->hosts_state.file_names =
		parse_multi_path(pvar->session_settings.KnownHostsFiles);
}

//
// known_hostsファイルの内容をすべて pvar->hosts_state.file_data へ読み込む
//
static int begin_read_file(PTInstVar pvar, char FAR * name,
                           int suppress_errors)
{
	int fd;
	int length;
	int amount_read;
	char buf[2048];

	get_teraterm_dir_relative_name(buf, sizeof(buf), name);
	fd = _open(buf, _O_RDONLY | _O_SEQUENTIAL | _O_BINARY);
	if (fd == -1) {
		if (!suppress_errors) {
			if (errno == ENOENT) {
				UTIL_get_lang_msg("MSG_HOSTS_READ_ENOENT_ERROR", pvar,
				                  "An error occurred while trying to read a known_hosts file.\n"
				                  "The specified filename does not exist.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			} else {
				UTIL_get_lang_msg("MSG_HOSTS_READ_ERROR", pvar,
				                  "An error occurred while trying to read a known_hosts file.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			}
		}
		return 0;
	}

	length = (int) _lseek(fd, 0, SEEK_END);
	_lseek(fd, 0, SEEK_SET);

	if (length >= 0 && length < 0x7FFFFFFF) {
		pvar->hosts_state.file_data = malloc(length + 1);
		if (pvar->hosts_state.file_data == NULL) {
			if (!suppress_errors) {
				UTIL_get_lang_msg("MSG_HOSTS_ALLOC_ERROR", pvar,
				                  "Memory ran out while trying to allocate space to read a known_hosts file.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			}
			_close(fd);
			return 0;
		}
	} else {
		if (!suppress_errors) {
			UTIL_get_lang_msg("MSG_HOSTS_READ_ERROR", pvar,
			                  "An error occurred while trying to read a known_hosts file.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		}
		_close(fd);
		return 0;
	}

	amount_read = _read(fd, pvar->hosts_state.file_data, length);
	pvar->hosts_state.file_data[length] = 0;

	_close(fd);

	if (amount_read != length) {
		if (!suppress_errors) {
			UTIL_get_lang_msg("MSG_HOSTS_READ_ERROR", pvar,
			                  "An error occurred while trying to read a known_hosts file.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		}
		free(pvar->hosts_state.file_data);
		pvar->hosts_state.file_data = NULL;
		return 0;
	} else {
		return 1;
	}
}

static int end_read_file(PTInstVar pvar, int suppress_errors)
{
	free(pvar->hosts_state.file_data);
	pvar->hosts_state.file_data = NULL;
	return 1;
}

static int begin_read_host_files(PTInstVar pvar, int suppress_errors)
{
	pvar->hosts_state.file_num = 0;
	pvar->hosts_state.file_data = NULL;
	return 1;
}

// MIME64の文字列をスキップする
static int eat_base64(char FAR * data)
{
	int index = 0;
	int ch;

	for (;;) {
		ch = data[index];
		if (ch == '=' || strchr(base64, ch)) {
			// BASE64の構成文字が見つかったら index を進める
			index++;
		} else {
			break;
		}
	}

	return index;
}

static int eat_spaces(char FAR * data)
{
	int index = 0;
	int ch;

	while ((ch = data[index]) == ' ' || ch == '\t') {
		index++;
	}
	return index;
}

static int eat_digits(char FAR * data)
{
	int index = 0;
	int ch;

	while ((ch = data[index]) >= '0' && ch <= '9') {
		index++;
	}
	return index;
}

static int eat_to_end_of_line(char FAR * data)
{
	int index = 0;
	int ch;

	while ((ch = data[index]) != '\n' && ch != '\r' && ch != 0) {
		index++;
	}

	while ((ch = data[index]) == '\n' || ch == '\r') {
		index++;
	}

	return index;
}

static int eat_to_end_of_pattern(char FAR * data)
{
	int index = 0;
	int ch;

	while (ch = data[index], is_pattern_char(ch)) {
		index++;
	}

	return index;
}

// 
// BASE64デコード処理を行う。(rfc1521)
// srcバッファは null-terminate している必要あり。
//
static int uudecode(unsigned char *src, int srclen, unsigned char *target, int targsize)
{
	char pad = '=';
	int tarindex, state, ch;
	char *pos;

	state = 0;
	tarindex = 0;

	while ((ch = *src++) != '\0') {
		if (isspace(ch))	/* Skip whitespace anywhere. */
			continue;

		if (ch == pad)
			break;

		pos = strchr(base64, ch);
		if (pos == 0) 		/* A non-base64 character. */
			return (-1);

		switch (state) {
		case 0:
			if (target) {
				if (tarindex >= targsize)
					return (-1);
				target[tarindex] = (pos - base64) << 2;
			}
			state = 1;
			break;
		case 1:
			if (target) {
				if (tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - base64) >> 4;
				target[tarindex+1]  = ((pos - base64) & 0x0f) << 4 ;
			}
			tarindex++;
			state = 2;
			break;
		case 2:
			if (target) {
				if (tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - base64) >> 2;
				target[tarindex+1]  = ((pos - base64) & 0x03) << 6;
			}
			tarindex++;
			state = 3;
			break;
		case 3:
			if (target) {
				if (tarindex >= targsize)
					return (-1);
				target[tarindex] |= (pos - base64);
			}
			tarindex++;
			state = 0;
			break;
		}
	}

	/*
	 * We are done decoding Base-64 chars.  Let's see if we ended
	 * on a byte boundary, and/or with erroneous trailing characters.
	 */

	if (ch == pad) {		/* We got a pad char. */
		ch = *src++;		/* Skip it, get next. */
		switch (state) {
		case 0:		/* Invalid = in first position */
		case 1:		/* Invalid = in second position */
			return (-1);

		case 2:		/* Valid, means one byte of info */
			/* Skip any number of spaces. */
			for (; ch != '\0'; ch = *src++)
				if (!isspace(ch))
					break;
			/* Make sure there is another trailing = sign. */
			if (ch != pad)
				return (-1);
			ch = *src++;		/* Skip the = */
			/* Fall through to "single trailing =" case. */
			/* FALLTHROUGH */

		case 3:		/* Valid, means two bytes of info */
			/*
			 * We know this char is an =.  Is there anything but
			 * whitespace after it?
			 */
			for (; ch != '\0'; ch = *src++)
				if (!isspace(ch))
					return (-1);

			/*
			 * Now make sure for cases 2 and 3 that the "extra"
			 * bits that slopped past the last full byte were
			 * zeros.  If we don't check them, they become a
			 * subliminal channel.
			 */
			if (target && target[tarindex] != 0)
				return (-1);
		}
	} else {
		/*
		 * We ended by seeing the end of the string.  Make sure we
		 * have no partial bytes lying around.
		 */
		if (state != 0)
			return (-1);
	}

	return (tarindex);
}


// SSH2鍵は BASE64 形式で格納されている
static Key *parse_uudecode(char *data)
{
	int count;
	unsigned char *blob = NULL;
	int len, n;
	Key *key = NULL;
	char ch;

	// BASE64文字列のサイズを得る
	count = eat_base64(data);
	len = 2 * count;
	blob = malloc(len);
	if (blob == NULL)
		goto error;

	// BASE64デコード
	ch = data[count];
	data[count] = '\0';  // ここは改行コードのはずなので書き潰しても問題ないはず
	n = uudecode(data, count, blob, len);
	data[count] = ch;
	if (n < 0) {
		goto error;
	}

	key = key_from_blob(blob, n);
	if (key == NULL)
		goto error;

error:
	if (blob != NULL)
		free(blob);

	return (key);
}


static char FAR *parse_bignum(char FAR * data)
{
	uint32 digits = 0;
	BIGNUM *num = BN_new();
	BIGNUM *billion = BN_new();
	BIGNUM *digits_num = BN_new();
	BN_CTX *ctx = BN_CTX_new();
	char FAR *result;
	int ch;
	int leftover_digits = 1;

	BN_CTX_init(ctx);
	BN_set_word(num, 0);
	BN_set_word(billion, 1000000000L);

	while ((ch = *data) >= '0' && ch <= '9') {
		if (leftover_digits == 1000000000L) {
			BN_set_word(digits_num, digits);
			BN_mul(num, num, billion, ctx);
			BN_add(num, num, digits_num);
			leftover_digits = 1;
			digits = 0;
		}

		digits = digits * 10 + ch - '0';
		leftover_digits *= 10;
		data++;
	}

	BN_set_word(digits_num, digits);
	BN_set_word(billion, leftover_digits);
	BN_mul(num, num, billion, ctx);
	BN_add(num, num, digits_num);

	result = (char FAR *) malloc(2 + BN_num_bytes(num));
	set_ushort16_MSBfirst(result, BN_num_bits(num));
	BN_bn2bin(num, result + 2);

	BN_CTX_free(ctx);
	BN_free(digits_num);
	BN_free(num);
	BN_free(billion);

	return result;
}

//
// known_hostsファイルの内容を解析し、指定したホストの公開鍵を探す。
//
static int check_host_key(PTInstVar pvar, char FAR * hostname,
                          unsigned short tcpport, char FAR * data)
{
	int index = eat_spaces(data);
	int matched = 0;
	int keybits = 0;

	if (data[index] == '#') {
		return index + eat_to_end_of_line(data + index);
	}

	/* if we find an empty line, then it won't have any patterns matching the hostname
	   and so we skip it */
	index--;
	do {
		int negated;
		int bracketed;
		char *end_bracket;
		int host_matched = 0;
		unsigned short keyfile_port = 22;

		index++;
		negated = data[index] == '!';

		if (negated) {
			index++;
			bracketed = data[index] == '[';
			if (bracketed) {
				end_bracket = strstr(data + index + 1, "]:");
				if (end_bracket != NULL) {
					*end_bracket = ' ';
					index++;
				}
			}
			host_matched = match_pattern(data + index, hostname);
			if (bracketed && end_bracket != NULL) {
				*end_bracket = ']';
				keyfile_port = atoi(end_bracket + 2);
			}
			if (host_matched && keyfile_port == tcpport) {
				return index + eat_to_end_of_line(data + index);
			}
		} else {
			bracketed = data[index] == '[';
			if (bracketed) {
				end_bracket = strstr(data + index + 1, "]:");
				if (end_bracket != NULL) {
					*end_bracket = ' ';
					index++;
				}
			}
			host_matched = match_pattern(data + index, hostname);
			if (bracketed && end_bracket != NULL) {
				*end_bracket = ']';
				keyfile_port = atoi(end_bracket + 2);
			}
			if (host_matched && keyfile_port == tcpport) {
				matched = 1;
			}
		}

		index += eat_to_end_of_pattern(data + index);
	} while (data[index] == ',');

	if (!matched) {
		return index + eat_to_end_of_line(data + index);
	} else {
		// 鍵の種類によりフォーマットが異なる
		// また、最初に一致したエントリを取得することになる。
		/*
		[SSH1]
		192.168.1.2 1024 35 13032....

		[SSH2]
		192.168.1.2 ssh-rsa AAAAB3NzaC1....
		192.168.1.2 ssh-dss AAAAB3NzaC1....
		192.168.1.2 rsa AAAAB3NzaC1....
		192.168.1.2 dsa AAAAB3NzaC1....
		192.168.1.2 rsa1 AAAAB3NzaC1....
		 */
		int rsa1_key_bits;

		index += eat_spaces(data + index);

		rsa1_key_bits = atoi(data + index);
		if (rsa1_key_bits > 0) { // RSA1である
			if (!SSHv1(pvar)) { // SSH2接続であれば無視する
				return index + eat_to_end_of_line(data + index);
			}

			pvar->hosts_state.hostkey.type = KEY_RSA1;

			pvar->hosts_state.hostkey.bits = rsa1_key_bits;
			index += eat_digits(data + index);
			index += eat_spaces(data + index);

			pvar->hosts_state.hostkey.exp = parse_bignum(data + index);
			index += eat_digits(data + index);
			index += eat_spaces(data + index);

			pvar->hosts_state.hostkey.mod = parse_bignum(data + index);

			/*
			if (pvar->hosts_state.key_bits < 0
				|| pvar->hosts_state.key_exp == NULL
				|| pvar->hosts_state.key_mod == NULL) {
				pvar->hosts_state.key_bits = 0;
				free(pvar->hosts_state.key_exp);
				free(pvar->hosts_state.key_mod);
			}*/

		} else {
			char *cp, *p;
			Key *key;

			if (!SSHv2(pvar)) { // SSH1接続であれば無視する
				return index + eat_to_end_of_line(data + index);
			}

			cp = data + index;
			p = strchr(cp, ' ');
			if (p == NULL) {
				return index + eat_to_end_of_line(data + index);
			}
			index += (p - cp);  // setup index
			*p = '\0';
			pvar->hosts_state.hostkey.type = get_keytype_from_name(cp);
			*p = ' ';

			index += eat_spaces(data + index);  // update index

			// uudecode
			key = parse_uudecode(data + index);
			if (key == NULL) {
				return index + eat_to_end_of_line(data + index);
			}

			// setup
			pvar->hosts_state.hostkey.type = key->type;
			pvar->hosts_state.hostkey.dsa = key->dsa;
			pvar->hosts_state.hostkey.rsa = key->rsa;

			index += eat_base64(data + index);
			index += eat_spaces(data + index);

			// Key構造体自身を解放する (2008.3.2 yutaka)
			free(key);
		}

		return index + eat_to_end_of_line(data + index);
	}
}

//
// known_hostsファイルからホスト名に合致する行を読む
//
static int read_host_key(PTInstVar pvar,
                         char FAR * hostname, unsigned short tcpport,
                         int suppress_errors, int return_always)
{
	int i;
	int while_flg;

	for (i = 0; hostname[i] != 0; i++) {
		int ch = hostname[i];

		if (!is_pattern_char(ch) || ch == '*' || ch == '?') {
			if (!suppress_errors) {
				UTIL_get_lang_msg("MSG_HOSTS_HOSTNAME_INVALID_ERROR", pvar,
				                  "The host name contains an invalid character.\n"
				                  "This session will be terminated.");
				notify_fatal_error(pvar, pvar->ts->UIMsg);
			}
			return 0;
		}
	}

	if (i == 0) {
		if (!suppress_errors) {
			UTIL_get_lang_msg("MSG_HOSTS_HOSTNAME_EMPTY_ERROR", pvar,
			                  "The host name should not be empty.\n"
			                  "This session will be terminated.");
			notify_fatal_error(pvar, pvar->ts->UIMsg);
		}
		return 0;
	}

#if 0
	pvar->hosts_state.key_bits = 0;
	free(pvar->hosts_state.key_exp);
	pvar->hosts_state.key_exp = NULL;
	free(pvar->hosts_state.key_mod);
	pvar->hosts_state.key_mod = NULL;
#else
	// hostkey type is KEY_UNSPEC.
	init_hostkey(&pvar->hosts_state.hostkey);
#endif

	do {
		if (pvar->hosts_state.file_data == NULL
		 || pvar->hosts_state.file_data[pvar->hosts_state.file_data_index] == 0) {
			char FAR *filename;
			int keep_going = 1;

			if (pvar->hosts_state.file_data != NULL) {
				end_read_file(pvar, suppress_errors);
			}

			do {
				filename =
					pvar->hosts_state.file_names[pvar->hosts_state.file_num];

				if (filename == NULL) {
					return 1;
				} else {
					pvar->hosts_state.file_num++;

					if (filename[0] != 0) {
						if (begin_read_file(pvar, filename, suppress_errors)) {
							pvar->hosts_state.file_data_index = 0;
							keep_going = 0;
						}
					}
				}
			} while (keep_going);
		}

		pvar->hosts_state.file_data_index +=
			check_host_key(pvar, hostname, tcpport,
			               pvar->hosts_state.file_data +
			               pvar->hosts_state.file_data_index);

		if (!return_always) {
			// 有効なキーが見つかるまで
			while_flg = (pvar->hosts_state.hostkey.type == KEY_UNSPEC);
		}
		else {
			while_flg = 0;
		}
	} while (while_flg);

	return 1;
}

static void finish_read_host_files(PTInstVar pvar, int suppress_errors)
{
	if (pvar->hosts_state.file_data != NULL) {
		end_read_file(pvar, suppress_errors);
	}
}

// サーバへ接続する前に、known_hostsファイルからホスト公開鍵を先読みしておく。
void HOSTS_prefetch_host_key(PTInstVar pvar, char FAR * hostname, unsigned short tcpport)
{
	if (!begin_read_host_files(pvar, 1)) {
		return;
	}

	if (!read_host_key(pvar, hostname, tcpport, 1, 0)) {
		return;
	}

	free(pvar->hosts_state.prefetched_hostname);
	pvar->hosts_state.prefetched_hostname = _strdup(hostname);

	finish_read_host_files(pvar, 1);
}

static BOOL equal_mp_ints(unsigned char FAR * num1,
                          unsigned char FAR * num2)
{
	if (num1 == NULL || num2 == NULL) {
		return FALSE;
	} else {
		uint32 bytes = (get_ushort16_MSBfirst(num1) + 7) / 8;

		if (bytes != (get_ushort16_MSBfirst(num2) + 7) / 8) {
			return FALSE;		/* different byte lengths */
		} else {
			return memcmp(num1 + 2, num2 + 2, bytes) == 0;
		}
	}
}

// 公開鍵が等しいかを検証する
static BOOL match_key(PTInstVar pvar, Key *key)
{
	int bits;
	unsigned char FAR * exp;
	unsigned char FAR * mod;

	if (key->type == KEY_RSA1) {  // SSH1 host public key
		bits = key->bits;
		exp = key->exp;
		mod = key->mod;

		/* just check for equal exponent and modulus */
		return equal_mp_ints(exp, pvar->hosts_state.hostkey.exp)
		    && equal_mp_ints(mod, pvar->hosts_state.hostkey.mod);
		/*
		return equal_mp_ints(exp, pvar->hosts_state.key_exp)
			&& equal_mp_ints(mod, pvar->hosts_state.key_mod);
			*/

	} else if (key->type == KEY_RSA) {  // SSH2 RSA host public key

		return key->rsa != NULL && pvar->hosts_state.hostkey.rsa != NULL &&
		       BN_cmp(key->rsa->e, pvar->hosts_state.hostkey.rsa->e) == 0 && 
		       BN_cmp(key->rsa->n, pvar->hosts_state.hostkey.rsa->n) == 0;

	} else { // // SSH2 DSA host public key

		return key->dsa != NULL && pvar->hosts_state.hostkey.dsa && 
		       BN_cmp(key->dsa->p, pvar->hosts_state.hostkey.dsa->p) == 0 && 
		       BN_cmp(key->dsa->q, pvar->hosts_state.hostkey.dsa->q) == 0 &&
		       BN_cmp(key->dsa->g, pvar->hosts_state.hostkey.dsa->g) == 0 &&
		       BN_cmp(key->dsa->pub_key, pvar->hosts_state.hostkey.dsa->pub_key) == 0;

	}

}

static void init_hosts_dlg(PTInstVar pvar, HWND dlg)
{
	char buf[1024];
	char buf2[2048];
	int i, j;
	int ch;
	char *fp = NULL;

	// static textの # 部分をホスト名に置換する
	GetDlgItemText(dlg, IDC_HOSTWARNING, buf, sizeof(buf));
	for (i = 0; (ch = buf[i]) != 0 && ch != '#'; i++) {
		buf2[i] = ch;
	}
	strncpy_s(buf2 + i, sizeof(buf2) - i,
	          pvar->hosts_state.prefetched_hostname, _TRUNCATE);
	j = i + strlen(buf2 + i);
	for (; buf[i] == '#'; i++) {
	}
	strncpy_s(buf2 + j, sizeof(buf2) - j, buf + i, _TRUNCATE);

	SetDlgItemText(dlg, IDC_HOSTWARNING, buf2);

	// fingerprintを設定する
	fp = key_fingerprint(&pvar->hosts_state.hostkey, SSH_FP_HEX);
	SendMessage(GetDlgItem(dlg, IDC_FINGER_PRINT), WM_SETTEXT, 0, (LPARAM)fp);
	free(fp);

	// ビジュアル化fingerprintを表示する
	fp = key_fingerprint(&pvar->hosts_state.hostkey, SSH_FP_RANDOMART);
	SendMessage(GetDlgItem(dlg, IDC_FP_RANDOMART), WM_SETTEXT, 0, (LPARAM)fp);
	SendMessage(GetDlgItem(dlg, IDC_FP_RANDOMART), WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT), TRUE);
	free(fp);
}

static int print_mp_int(char FAR * buf, unsigned char FAR * mp)
{
	int i = 0, j, k;
	BIGNUM *num = BN_new();
	int ch;

	BN_bin2bn(mp + 2, (get_ushort16_MSBfirst(mp) + 7) / 8, num);

	do {
		buf[i] = (char) ((BN_div_word(num, 10)) + '0');
		i++;
	} while (!BN_is_zero(num));

	/* we need to reverse the digits */
	for (j = 0, k = i - 1; j < k; j++, k--) {
		ch = buf[j];
		buf[j] = buf[k];
		buf[k] = ch;
	}

	buf[i] = 0;
	return i;
}

//
// known_hosts ファイルへ保存するエントリを作成する。
//
static char FAR *format_host_key(PTInstVar pvar)
{
	int host_len = strlen(pvar->hosts_state.prefetched_hostname);
	char *result = NULL;
	int index;
	enum hostkey_type type = pvar->hosts_state.hostkey.type;

	if (type == KEY_RSA1) {
		int result_len = host_len + 50 + 8 +
		                 get_ushort16_MSBfirst(pvar->hosts_state.hostkey.exp) / 3 +
		                 get_ushort16_MSBfirst(pvar->hosts_state.hostkey.mod) / 3;
		result = (char FAR *) malloc(result_len);

		if (pvar->ssh_state.tcpport == 22) {
			strncpy_s(result, result_len, pvar->hosts_state.prefetched_hostname, _TRUNCATE);
			index = host_len;
		}
		else {
			_snprintf_s(result, result_len, _TRUNCATE, "[%s]:%d",
			            pvar->hosts_state.prefetched_hostname,
			            pvar->ssh_state.tcpport);
			index = strlen(result);
		}

		_snprintf_s(result + index, result_len - host_len, _TRUNCATE,
		            " %d ", pvar->hosts_state.hostkey.bits);
		index += strlen(result + index);
		index += print_mp_int(result + index, pvar->hosts_state.hostkey.exp);
		result[index] = ' ';
		index++;
		index += print_mp_int(result + index, pvar->hosts_state.hostkey.mod);
		strncpy_s(result + index, result_len - index, " \r\n", _TRUNCATE);

	} else if (type == KEY_RSA || type == KEY_DSA) {
		Key *key = &pvar->hosts_state.hostkey;
		char *blob = NULL;
		int blen, uulen, msize;
		char *uu = NULL;
		int n;

		key_to_blob(key, &blob, &blen);
		uulen = 2 * blen;
		uu = malloc(uulen);
		if (uu == NULL) {
			goto error;
		}
		n = uuencode(blob, blen, uu, uulen);
		if (n > 0) {
			msize = host_len + 50 + uulen;
			result = malloc(msize);
			if (result == NULL) {
				goto error;
			}

			// setup
			if (pvar->ssh_state.tcpport == 22) {
				_snprintf_s(result, msize, _TRUNCATE, "%s %s %s\r\n",
				            pvar->hosts_state.prefetched_hostname, 
				            get_sshname_from_key(key),
				            uu);
			} else {
				_snprintf_s(result, msize, _TRUNCATE, "[%s]:%d %s %s\r\n",
				            pvar->hosts_state.prefetched_hostname,
				            pvar->ssh_state.tcpport,
				            get_sshname_from_key(key),
				            uu);
			}
		}
error:
		if (blob != NULL)
			free(blob);
		if (uu != NULL)
			free(uu);

	} else {
		return NULL;

	}

	return result;
}

static void add_host_key(PTInstVar pvar)
{
	char FAR *name = NULL;

	if ( pvar->hosts_state.file_names != NULL)
		name = pvar->hosts_state.file_names[0];

	if (name == NULL || name[0] == 0) {
		UTIL_get_lang_msg("MSG_HOSTS_FILE_UNSPECIFY_ERROR", pvar,
		                  "The host and its key cannot be added, because no known-hosts file has been specified.\n"
		                  "Restart Tera Term and specify a read/write known-hosts file in the TTSSH Setup dialog box.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	} else {
		char FAR *keydata = format_host_key(pvar);
		int length = strlen(keydata);
		int fd;
		int amount_written;
		int close_result;
		char buf[FILENAME_MAX];

		get_teraterm_dir_relative_name(buf, sizeof(buf), name);
		fd = _open(buf,
		          _O_APPEND | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL | _O_BINARY,
		          _S_IREAD | _S_IWRITE);
		if (fd == -1) {
			if (errno == EACCES) {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_EACCES_ERROR", pvar,
				                  "An error occurred while trying to write the host key.\n"
				                  "You do not have permission to write to the known-hosts file.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			} else {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
				                  "An error occurred while trying to write the host key.\n"
				                  "The host key could not be written.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			}
			return;
		}

		amount_written = _write(fd, keydata, length);
		free(keydata);
		close_result = _close(fd);

		if (amount_written != length || close_result == -1) {
			UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
			                  "An error occurred while trying to write the host key.\n"
			                  "The host key could not be written.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		}
	}
}

static char FAR *copy_mp_int(char FAR * num)
{
	int len = (get_ushort16_MSBfirst(num) + 7) / 8 + 2;
	char FAR *result = (char FAR *) malloc(len);

	if (result != NULL) {
		memcpy(result, num, len);
	}

	return result;
}

//
// 同じホストで内容の異なるキーを削除する
// add_host_key のあとに呼ぶこと
//
static void delete_different_key(PTInstVar pvar)
{
	char FAR *name = pvar->hosts_state.file_names[0];

	if (name == NULL || name[0] == 0) {
		UTIL_get_lang_msg("MSG_HOSTS_FILE_UNSPECIFY_ERROR", pvar,
		                  "The host and its key cannot be added, because no known-hosts file has been specified.\n"
		                  "Restart Tera Term and specify a read/write known-hosts file in the TTSSH Setup dialog box.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	}
	else {
		Key key; // 接続中のホストのキー
		int length;
		char filename[MAX_PATH];
		char tmp[L_tmpnam];
		int fd;
		int amount_written = 0;
		int close_result;
		int data_index = 0;
		char buf[FILENAME_MAX];

		// 書き込み一時ファイルを開く
		_getcwd(filename, sizeof(filename));
		tmpnam_s(tmp,sizeof(tmp));
		strcat_s(filename, sizeof(filename), tmp);
		fd = _open(filename,
		          _O_CREAT | _O_WRONLY | _O_SEQUENTIAL | _O_BINARY | _O_TRUNC,
		          _S_IREAD | _S_IWRITE);

		if (fd == -1) {
			if (errno == EACCES) {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_EACCES_ERROR", pvar,
				                  "An error occurred while trying to write the host key.\n"
				                  "You do not have permission to write to the known-hosts file.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			} else {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
				                  "An error occurred while trying to write the host key.\n"
				                  "The host key could not be written.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			}
			return;
		}

		// 接続中のサーバのキーを読み込む
		if (pvar->hosts_state.hostkey.type == KEY_RSA1) { // SSH1
			key.type = KEY_RSA1;
			key.bits = pvar->hosts_state.hostkey.bits;
			key.exp = copy_mp_int(pvar->hosts_state.hostkey.exp);
			key.mod = copy_mp_int(pvar->hosts_state.hostkey.mod);
		} else if (pvar->hosts_state.hostkey.type == KEY_RSA) { // SSH2 RSA
			key.type = KEY_RSA;
			key.rsa = duplicate_RSA(pvar->hosts_state.hostkey.rsa);
		} else { // SSH2 DSA
			key.type = KEY_DSA;
			key.dsa = duplicate_DSA(pvar->hosts_state.hostkey.dsa);
		}

		// ファイルから読み込む
		begin_read_host_files(pvar, 0);
		do {
			int host_index = 0;
			int matched = 0;
			int keybits = 0;
			char FAR *data;
			int do_write = 0;
			length = amount_written = 0;

			if (!read_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, 0, 1)) {
				break;
			}

			if (data_index == pvar->hosts_state.file_data_index) {
				// index が進まない == 最後まで読んだ
				break;
			}

			data = pvar->hosts_state.file_data + data_index;
			host_index = eat_spaces(data);

			if (data[host_index] == '#') {
				do_write = 1;
			}
			else {
				// ホストの照合
				host_index--;
				do {
					int negated;
					int bracketed;
					char *end_bracket;
					int host_matched = 0;
					unsigned short keyfile_port = 22;

					host_index++;
					negated = data[host_index] == '!';

					if (negated) {
						host_index++;
						bracketed = data[host_index] == '[';
						if (bracketed) {
							end_bracket = strstr(data + host_index + 1, "]:");
							if (end_bracket != NULL) {
								*end_bracket = ' ';
								host_index++;
							}
						}
						host_matched = match_pattern(data + host_index, pvar->ssh_state.hostname);
						if (bracketed && end_bracket != NULL) {
							*end_bracket = ']';
							keyfile_port = atoi(end_bracket + 2);
						}
						if (host_matched && keyfile_port == pvar->ssh_state.tcpport) {
							matched = 0;
							// 接続バージョンチェックのために host_index を進めてから抜ける
							host_index--;
							do {
								host_index++;
								host_index += eat_to_end_of_pattern(data + host_index);
							} while (data[host_index] == ',');
							break;
						}
					}
					else {
						bracketed = data[host_index] == '[';
						if (bracketed) {
							end_bracket = strstr(data + host_index + 1, "]:");
							if (end_bracket != NULL) {
								*end_bracket = ' ';
								host_index++;
							}
						}
						host_matched = match_pattern(data + host_index, pvar->ssh_state.hostname);
						if (bracketed && end_bracket != NULL) {
							*end_bracket = ']';
							keyfile_port = atoi(end_bracket + 2);
						}
						if (host_matched && keyfile_port == pvar->ssh_state.tcpport) {
							matched = 1;
						}
					}
					host_index += eat_to_end_of_pattern(data + host_index);
				} while (data[host_index] == ',');

				// ホストが等しくて合致するキーが見つかる
				if (match_key(pvar, &key)) {
					do_write = 1;
				}
				// ホストが等しくない
				else if (!matched) {
					do_write = 1;
				}
				// ホストが等しい and 接続のバージョンが違う
				else {
					int rsa1_key_bits=0;
					rsa1_key_bits = atoi(data + host_index + eat_spaces(data + host_index));
					
					if (rsa1_key_bits > 0) { // ファイルのキーは ssh1
						if (!SSHv1(pvar)) {
							do_write = 1;
						}
					}
					else { // ファイルのキーは ssh2
						if (!SSHv2(pvar)) {
							do_write = 1;
						}
					}
				}
			}

			// 書き込み処理
			if (do_write) {
				length = pvar->hosts_state.file_data_index - data_index;
				amount_written =
					_write(fd, pvar->hosts_state.file_data + data_index,
					       length);

				if (amount_written != length) {
					goto error1;
				}
			}
			data_index = pvar->hosts_state.file_data_index;
		} while (1); // 最後まで読む

error1:
		close_result = _close(fd);
		if (amount_written != length || close_result == -1) {
			UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
			                  "An error occurred while trying to write the host key.\n"
			                  "The host key could not be written.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			goto error2;
		}

		// 書き込み一時ファイルからリネーム
		get_teraterm_dir_relative_name(buf, sizeof(buf), name);
		_unlink(buf);
		rename(filename, buf);

error2:
		_unlink(filename);

		finish_read_host_files(pvar, 0);
	}
}

//
// Unknown hostのホスト公開鍵を known_hosts ファイルへ保存するかどうかを
// ユーザに確認させる。
// TODO: finger printの表示も行う。
// (2006.3.25 yutaka)
//
static BOOL CALLBACK hosts_add_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
                                        LPARAM lParam)
{
	PTInstVar pvar;
	LOGFONT logfont;
	HFONT font;
	char uimsg[MAX_UIMSG];

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		pvar->hosts_state.hosts_dialog = dlg;
		SetWindowLong(dlg, DWL_USER, lParam);

		// 追加・置き換えとも init_hosts_dlg を呼んでいるので、その前にセットする必要がある
		GetWindowText(dlg, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_UNKNONWHOST_TITLE", pvar, uimsg);
		SetWindowText(dlg, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTWARNING, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_UNKNOWNHOST_WARNINIG", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTWARNING, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTWARNING2, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_UNKNOWNHOST_WARNINIG2", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTWARNING2, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTFINGERPRINT, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_UNKNOWNHOST_FINGERPRINT", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTFINGERPRINT, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_ADDTOKNOWNHOSTS, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_UNKNOWNHOST_ADD", pvar, uimsg);
		SetDlgItemText(dlg, IDC_ADDTOKNOWNHOSTS, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_CONTINUE, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("BTN_CONTINUE", pvar, uimsg);
		SetDlgItemText(dlg, IDC_CONTINUE, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("BTN_DISCONNECT", pvar, uimsg);
		SetDlgItemText(dlg, IDCANCEL, pvar->ts->UIMsg);

		init_hosts_dlg(pvar, dlg);

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgHostsAddFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_HOSTWARNING, WM_SETFONT, (WPARAM)DlgHostsAddFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTWARNING2, WM_SETFONT, (WPARAM)DlgHostsAddFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTFINGERPRINT, WM_SETFONT, (WPARAM)DlgHostsAddFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_FINGER_PRINT, WM_SETFONT, (WPARAM)DlgHostsAddFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_ADDTOKNOWNHOSTS, WM_SETFONT, (WPARAM)DlgHostsAddFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CONTINUE, WM_SETFONT, (WPARAM)DlgHostsAddFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDCANCEL, WM_SETFONT, (WPARAM)DlgHostsAddFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgHostsAddFont = NULL;
		}

		// add host check boxにチェックをデフォルトで入れておく 
		SendMessage(GetDlgItem(dlg, IDC_ADDTOKNOWNHOSTS), BM_SETCHECK, BST_CHECKED, 0);

		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLong(dlg, DWL_USER);

		switch (LOWORD(wParam)) {
		case IDC_CONTINUE:
			if (IsDlgButtonChecked(dlg, IDC_ADDTOKNOWNHOSTS)) {
				add_host_key(pvar);
			}

			if (SSHv1(pvar)) {
				SSH_notify_host_OK(pvar);
			} else { // SSH2
				// SSH2ではあとで SSH_notify_host_OK() を呼ぶ。
			}

			pvar->hosts_state.hosts_dialog = NULL;

			EndDialog(dlg, 1);

			if (DlgHostsAddFont != NULL) {
				DeleteObject(DlgHostsAddFont);
			}

			return TRUE;

		case IDCANCEL:			/* kill the connection */
			pvar->hosts_state.hosts_dialog = NULL;
			notify_closed_connection(pvar);
			EndDialog(dlg, 0);

			if (DlgHostsAddFont != NULL) {
				DeleteObject(DlgHostsAddFont);
			}

			return TRUE;

		default:
			return FALSE;
		}

	default:
		return FALSE;
	}
}

//
// 置き換え時の確認ダイアログを分離
//
static BOOL CALLBACK hosts_replace_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
                                            LPARAM lParam)
{
	PTInstVar pvar;
	LOGFONT logfont;
	HFONT font;
	char uimsg[MAX_UIMSG];

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		pvar->hosts_state.hosts_dialog = dlg;
		SetWindowLong(dlg, DWL_USER, lParam);

		// 追加・置き換えとも init_hosts_dlg を呼んでいるので、その前にセットする必要がある
		GetWindowText(dlg, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_DIFFERENTHOST_TITLE", pvar, uimsg);
		SetWindowText(dlg, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTWARNING, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_DIFFERENTHOST_WARNING", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTWARNING, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTWARNING2, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_DIFFERENTHOST_WARNING2", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTWARNING2, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTFINGERPRINT, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_DIFFERENTHOST_FINGERPRINT", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTFINGERPRINT, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_ADDTOKNOWNHOSTS, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_DIFFERENTHOST_REPLACE", pvar, uimsg);
		SetDlgItemText(dlg, IDC_ADDTOKNOWNHOSTS, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_CONTINUE, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("BTN_CONTINUE", pvar, uimsg);
		SetDlgItemText(dlg, IDC_CONTINUE, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("BTN_DISCONNECT", pvar, uimsg);
		SetDlgItemText(dlg, IDCANCEL, pvar->ts->UIMsg);

		init_hosts_dlg(pvar, dlg);

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgHostsReplaceFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_HOSTWARNING, WM_SETFONT, (WPARAM)DlgHostsReplaceFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTWARNING2, WM_SETFONT, (WPARAM)DlgHostsReplaceFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTFINGERPRINT, WM_SETFONT, (WPARAM)DlgHostsReplaceFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_ADDTOKNOWNHOSTS, WM_SETFONT, (WPARAM)DlgHostsReplaceFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CONTINUE, WM_SETFONT, (WPARAM)DlgHostsReplaceFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDCANCEL, WM_SETFONT, (WPARAM)DlgHostsReplaceFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgHostsReplaceFont = NULL;
		}

		// デフォルトでチェックは入れない
		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLong(dlg, DWL_USER);

		switch (LOWORD(wParam)) {
		case IDC_CONTINUE:
			if (IsDlgButtonChecked(dlg, IDC_ADDTOKNOWNHOSTS)) {
				add_host_key(pvar);
				delete_different_key(pvar);
			}

			if (SSHv1(pvar)) {
				SSH_notify_host_OK(pvar);
			} else { // SSH2
				// SSH2ではあとで SSH_notify_host_OK() を呼ぶ。
			}

			pvar->hosts_state.hosts_dialog = NULL;

			EndDialog(dlg, 1);

			if (DlgHostsReplaceFont != NULL) {
				DeleteObject(DlgHostsReplaceFont);
			}

			return TRUE;

		case IDCANCEL:			/* kill the connection */
			pvar->hosts_state.hosts_dialog = NULL;
			notify_closed_connection(pvar);
			EndDialog(dlg, 0);

			if (DlgHostsReplaceFont != NULL) {
				DeleteObject(DlgHostsReplaceFont);
			}

			return TRUE;

		default:
			return FALSE;
		}

	default:
		return FALSE;
	}
}

void HOSTS_do_unknown_host_dialog(HWND wnd, PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog == NULL) {
		HWND cur_active = GetActiveWindow();

		DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHUNKNOWNHOST),
		               cur_active != NULL ? cur_active : wnd,
		               hosts_add_dlg_proc, (LPARAM) pvar);
	}
}

void HOSTS_do_different_host_dialog(HWND wnd, PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog == NULL) {
		HWND cur_active = GetActiveWindow();

		DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHDIFFERENTHOST),
		               cur_active != NULL ? cur_active : wnd,
		               hosts_replace_dlg_proc, (LPARAM) pvar);
	}
}

//
// サーバから送られてきたホスト公開鍵の妥当性をチェックする
//
// SSH2対応を追加 (2006.3.24 yutaka)
//
BOOL HOSTS_check_host_key(PTInstVar pvar, char FAR * hostname, unsigned short tcpport, Key *key)
{
	int found_different_key = 0;

	// すでに known_hosts ファイルからホスト公開鍵を読み込んでいるなら、それと比較する。
	if (pvar->hosts_state.prefetched_hostname != NULL
	 && _stricmp(pvar->hosts_state.prefetched_hostname, hostname) == 0
	 && match_key(pvar, key)) {

		if (SSHv1(pvar)) {
			SSH_notify_host_OK(pvar);
		} else {
			// SSH2ではあとで SSH_notify_host_OK() を呼ぶ。
		}
		return TRUE;
	}

	// 先読みされていない場合は、この時点でファイルから読み込む
	if (begin_read_host_files(pvar, 0)) {
		do {
			if (!read_host_key(pvar, hostname, tcpport, 0, 0)) {
				break;
			}

			if (pvar->hosts_state.hostkey.type != KEY_UNSPEC) {
				if (match_key(pvar, key)) {
					finish_read_host_files(pvar, 0);
					// すべてのエントリを参照して、合致するキーが見つかったら戻る。
					// SSH2の場合はここでは何もしない。(2006.3.29 yutaka)
					if (SSHv1(pvar)) {
						SSH_notify_host_OK(pvar);
					} else {
						// SSH2ではあとで SSH_notify_host_OK() を呼ぶ。
					}
					return TRUE;
				} else {
					// キーは known_hosts に見つかったが、キーの内容が異なる。
					found_different_key = 1;
				}
			}
		} while (pvar->hosts_state.hostkey.type != KEY_UNSPEC);  // キーが見つかっている間はループする

		finish_read_host_files(pvar, 0);
	}


	// known_hosts に存在しないキーはあとでファイルへ書き込むために、ここで保存しておく。
	pvar->hosts_state.hostkey.type = key->type;
	if (key->type == KEY_RSA1) { // SSH1
		pvar->hosts_state.hostkey.bits = key->bits;
		pvar->hosts_state.hostkey.exp = copy_mp_int(key->exp);
		pvar->hosts_state.hostkey.mod = copy_mp_int(key->mod);

	} else if (key->type == KEY_RSA) { // SSH2 RSA
		pvar->hosts_state.hostkey.rsa = duplicate_RSA(key->rsa);

	} else { // SSH2 DSA
		pvar->hosts_state.hostkey.dsa = duplicate_DSA(key->dsa);

	}
	free(pvar->hosts_state.prefetched_hostname);
	pvar->hosts_state.prefetched_hostname = _strdup(hostname);

	// known_hostsダイアログは同期的に表示させ、この時点においてユーザに確認
	// させる必要があるため、直接コールに変更する。
	// これによりknown_hostsの確認を待たずに、サーバへユーザ情報を送ってしまう問題を回避する。
	// (2007.10.1 yutaka)
	if (found_different_key) {
#if 0
		PostMessage(pvar->NotificationWindow, WM_COMMAND,
		            ID_SSHDIFFERENTHOST, 0);
#else
		HOSTS_do_different_host_dialog(pvar->NotificationWindow, pvar);
#endif
	} else {
#if 0
		PostMessage(pvar->NotificationWindow, WM_COMMAND,
		            ID_SSHUNKNOWNHOST, 0);
#else
		HOSTS_do_unknown_host_dialog(pvar->NotificationWindow, pvar);
#endif

	}

	return TRUE;
}

void HOSTS_notify_disconnecting(PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog != NULL) {
		PostMessage(pvar->hosts_state.hosts_dialog, WM_COMMAND, IDCANCEL,
		            0);
		/* the main window might not go away if it's not enabled. (see vtwin.cpp) */
		EnableWindow(pvar->NotificationWindow, TRUE);
	}
}

void HOSTS_end(PTInstVar pvar)
{
	int i;

	free(pvar->hosts_state.prefetched_hostname);
#if 0
	free(pvar->hosts_state.key_exp);
	free(pvar->hosts_state.key_mod);
#else
	init_hostkey(&pvar->hosts_state.hostkey);
#endif

	if (pvar->hosts_state.file_names != NULL) {
		for (i = 0; pvar->hosts_state.file_names[i] != NULL; i++) {
			free(pvar->hosts_state.file_names[i]);
		}
		free(pvar->hosts_state.file_names);
	}
}
