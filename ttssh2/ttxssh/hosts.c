/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (C) 2004-2020 TeraTerm Project
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

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/
#include "ttxssh.h"
#include "util.h"
#include "resource.h"
#include "matcher.h"
#include "ssh.h"
#include "key.h"
#include "hosts.h"
#include "dns.h"
#include "dlglib.h"
#include "compat_win.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>

#include <fcntl.h>
#include <io.h>
#include <errno.h>
#include <sys/stat.h>
#include <direct.h>
#include <memory.h>


#undef DialogBoxParam
#define DialogBoxParam(p1,p2,p3,p4,p5) \
	TTDialogBoxParam(p1,p2,p3,p4,p5)
#undef EndDialog
#define EndDialog(p1,p2) \
	TTEndDialog(p1, p2)

// BASE64構成文字列（ここでは'='は含まれていない）
static char base64[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


BOOL HOSTS_resume_session_after_known_hosts(PTInstVar pvar);
void HOSTS_cancel_session_after_known_hosts(PTInstVar pvar);


static char **parse_multi_path(char *buf)
{
	int i;
	int ch;
	int num_paths = 1;
	char ** result;
	int last_path_index;

	for (i = 0; (ch = buf[i]) != 0; i++) {
		if (ch == ';') {
			num_paths++;
		}
	}

	result =
		(char **) malloc(sizeof(char *) * (num_paths + 1));

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
	key_init(&pvar->hosts_state.hostkey);
	pvar->hosts_state.hosts_dialog = NULL;
	pvar->hosts_state.file_names = NULL;

	/*
	 * 前回のオプション指定(/nosecuritywarning)が残らないように初期化しておく。
	 */
	pvar->nocheck_known_hosts = FALSE;
}

void HOSTS_open(PTInstVar pvar)
{
	pvar->hosts_state.file_names =
		parse_multi_path(pvar->session_settings.KnownHostsFiles);
}

//
// known_hostsファイルの内容をすべて pvar->hosts_state.file_data へ読み込む
//
static int begin_read_file(PTInstVar pvar, char *name,
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
static int eat_base64(char *data)
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

static int eat_spaces(char *data)
{
	int index = 0;
	int ch;

	while ((ch = data[index]) == ' ' || ch == '\t') {
		index++;
	}
	return index;
}

static int eat_digits(char *data)
{
	int index = 0;
	int ch;

	while ((ch = data[index]) >= '0' && ch <= '9') {
		index++;
	}
	return index;
}

static int eat_to_end_of_line(char *data)
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

static int eat_to_end_of_pattern(char *data)
{
	int index = 0;
	int ch;

	while (ch = data[index], is_pattern_char(ch)) {
		index++;
	}

	return index;
}

// SSH2鍵は BASE64 形式で格納されている
static Key *parse_base64data(char *data)
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
	n = b64decode(blob, len, data);
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


static char *parse_bignum(char *data)
{
	uint32 digits = 0;
	BIGNUM *num = BN_new();
	BIGNUM *billion = BN_new();
	BIGNUM *digits_num = BN_new();
	BN_CTX *ctx = BN_CTX_new();
	char *result;
	int ch;
	int leftover_digits = 1;

	// BN_CTX_init関数は OpenSSL 1.1.0 で削除された。
	// OpenSSL 1.0.2の時点ですでに deprecated 扱いだった。
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

	result = (char *) malloc(2 + BN_num_bytes(num));
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
static int check_host_key(PTInstVar pvar, char *hostname,
                          unsigned short tcpport, char *data,
                          Key *key)
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

			key->type = KEY_RSA1;

			key->bits = rsa1_key_bits;
			index += eat_digits(data + index);
			index += eat_spaces(data + index);

			key->exp = parse_bignum(data + index);
			index += eat_digits(data + index);
			index += eat_spaces(data + index);

			key->mod = parse_bignum(data + index);
		} else {
			char *cp, *p;
			Key *key2;
			ssh_keytype key_type;

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
			key_type = get_keytype_from_name(cp);
			*p = ' ';

			index += eat_spaces(data + index);  // update index

			// base64 decode
			key2 = parse_base64data(data + index);
			if (key2 == NULL) {
				return index + eat_to_end_of_line(data + index);
			}

			// setup
			key->type = key2->type;
			key->dsa = key2->dsa;
			key->rsa = key2->rsa;
			key->ecdsa = key2->ecdsa;
			key->ed25519_pk = key2->ed25519_pk;

			index += eat_base64(data + index);
			index += eat_spaces(data + index);

			// Key構造体自身を解放する (2008.3.2 yutaka)
			free(key2);
		}

		return index + eat_to_end_of_line(data + index);
	}
}

//
// known_hostsファイルからホスト名に合致する行を読む
//   return_always
//     0: 見つかるまで探す
//     1: 1行だけ探して戻る
//
static int read_host_key(PTInstVar pvar,
                         char *hostname, unsigned short tcpport,
                         int suppress_errors, int return_always,
                         Key *key)
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
				notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
			}
			return 0;
		}
	}

	if (i == 0) {
		if (!suppress_errors) {
			UTIL_get_lang_msg("MSG_HOSTS_HOSTNAME_EMPTY_ERROR", pvar,
			                  "The host name should not be empty.\n"
			                  "This session will be terminated.");
			notify_fatal_error(pvar, pvar->ts->UIMsg, TRUE);
		}
		return 0;
	}

	// hostkey type is KEY_UNSPEC.
	key_init(key);

	do {
		if (pvar->hosts_state.file_data == NULL
		 || pvar->hosts_state.file_data[pvar->hosts_state.file_data_index] == 0) {
			char *filename;
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
			               pvar->hosts_state.file_data_index,
			               key);

		if (!return_always) {
			// 有効なキーが見つかるまで
			while_flg = (key->type == KEY_UNSPEC);
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
void HOSTS_prefetch_host_key(PTInstVar pvar, char *hostname, unsigned short tcpport)
{
	Key key; // known_hostsに登録されている鍵

	if (!begin_read_host_files(pvar, 1)) {
		return;
	}

	memset(&key, 0, sizeof(key));
	if (!read_host_key(pvar, hostname, tcpport, 1, 0, &key)) {
		return;
	}

	key_copy(&pvar->hosts_state.hostkey, &key);
	key_init(&key);

	free(pvar->hosts_state.prefetched_hostname);
	pvar->hosts_state.prefetched_hostname = _strdup(hostname);

	finish_read_host_files(pvar, 1);
}


// known_hostsファイルから該当するキー情報を取得する。
//
// return:
//   *keyptr != NULL  取得成功
//
static int parse_hostkey_file(PTInstVar pvar, char *hostname,
	unsigned short tcpport, char *data, Key **keyptr)
{
	int index = eat_spaces(data);
	int matched = 0;
	int keybits = 0;
	ssh_keytype ktype;
	Key *key;

	*keyptr = NULL;

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
		}
		else {
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
	}
	else {
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

			key = key_new(KEY_RSA1);
			key->bits = rsa1_key_bits;

			index += eat_digits(data + index);
			index += eat_spaces(data + index);
			key->exp = parse_bignum(data + index);

			index += eat_digits(data + index);
			index += eat_spaces(data + index);
			key->mod = parse_bignum(data + index);

			// setup
			*keyptr = key;

		}
		else {
			char *cp, *p;

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
			ktype = get_keytype_from_name(cp);
			*p = ' ';

			index += eat_spaces(data + index);  // update index

			// base64 decode
			key = parse_base64data(data + index);
			if (key == NULL) {
				return index + eat_to_end_of_line(data + index);
			}

			// setup
			*keyptr = key;

			index += eat_base64(data + index);
			index += eat_spaces(data + index);
		}

		return index + eat_to_end_of_line(data + index);
	}
}

// known_hostsファイルからホスト公開鍵を取得する。
// 既存の処理を改変したくないので、Host key rotation用に新規に用意する。
//
// return 1: success
//        0: fail
int HOSTS_hostkey_foreach(PTInstVar pvar, hostkeys_foreach_fn *callback, void *ctx)
{
	int success = 0;
	int suppress_errors = 1;
	unsigned short tcpport;
	char *filename;
	char *hostname;
	Key *key;

	if (!begin_read_host_files(pvar, 1)) {
		goto error;
	}

	// Host key rotationでは、known_hosts ファイルを書き換えるので、
	// 検索するのは1つめのファイルのみでよい（2つめのファイルはReadOnlyのため）。
	filename = pvar->hosts_state.file_names[pvar->hosts_state.file_num];
	pvar->hosts_state.file_num++;

	pvar->hosts_state.file_data_index = -1;
	if (filename[0] != 0) {
		if (begin_read_file(pvar, filename, suppress_errors)) {
			pvar->hosts_state.file_data_index = 0;
		}
	}
	if (pvar->hosts_state.file_data_index == -1)
		goto error;

	// 検索対象となるホスト名とポート番号。
	hostname = pvar->ssh_state.hostname;
	tcpport = pvar->ssh_state.tcpport;

	// known_hostsファイルの内容がすべて pvar->hosts_state.file_data に読み込まれている。
	// 末尾は \0 。
	while (pvar->hosts_state.file_data[pvar->hosts_state.file_data_index] != 0) {
		key = NULL;

		pvar->hosts_state.file_data_index +=
			parse_hostkey_file(pvar, hostname, tcpport,
				pvar->hosts_state.file_data +
				pvar->hosts_state.file_data_index,
				&key);

		// 該当する鍵が見つかったら、コールバック関数を呼び出す。
		if (key != NULL) {
			if (callback(key, ctx) == 0) 
				key_free(key);
		}
	}

	success = 1;

error:
	finish_read_host_files(pvar, 1);

	return (success);
}


static BOOL equal_mp_ints(unsigned char *num1,
                          unsigned char *num2)
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


// 公開鍵の比較を行う。
//
// return
//   -1 ... 鍵の型が違う
//    0 ... 等しくない
//    1 ... 等しい
int HOSTS_compare_public_key(Key *src, Key *key)
{
	int bits;
	unsigned char *exp;
	unsigned char *mod;
	const EC_GROUP *group;
	const EC_POINT *pa, *pb;
	Key *a, *b;
	BIGNUM *e = NULL, *n = NULL;
	BIGNUM *se = NULL, *sn = NULL;
	BIGNUM *p, *q, *g, *pub_key;
	BIGNUM *sp, *sq, *sg, *spub_key;

	if (src->type != key->type) {
		return -1;
	}

	switch (key->type) {
	case KEY_RSA1: // SSH1 host public key
		bits = key->bits;
		exp = key->exp;
		mod = key->mod;

		/* just check for equal exponent and modulus */
		return equal_mp_ints(exp, src->exp)
			&& equal_mp_ints(mod, src->mod);
		/*
		return equal_mp_ints(exp, pvar->hosts_state.key_exp)
		&& equal_mp_ints(mod, pvar->hosts_state.key_mod);
		*/

	case KEY_RSA: // SSH2 RSA host public key
		RSA_get0_key(key->rsa, &n, &e, NULL);
		RSA_get0_key(src->rsa, &sn, &se, NULL);
		return key->rsa != NULL && src->rsa != NULL &&
			BN_cmp(e, se) == 0 &&
			BN_cmp(n, sn) == 0;

	case KEY_DSA: // SSH2 DSA host public key
		DSA_get0_pqg(key->dsa, &p, &q, &g);
		DSA_get0_pqg(src->dsa, &sp, &sq, &sg);
		DSA_get0_key(key->dsa, &pub_key, NULL);
		DSA_get0_key(src->dsa, &spub_key, NULL);
		return key->dsa != NULL && src->dsa &&
			BN_cmp(p, sp) == 0 &&
			BN_cmp(q, sq) == 0 &&
			BN_cmp(g, sg) == 0 &&
			BN_cmp(pub_key, spub_key) == 0;

	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		if (key->ecdsa == NULL || src->ecdsa == NULL) {
			return FALSE;
		}
		group = EC_KEY_get0_group(key->ecdsa);
		pa = EC_KEY_get0_public_key(key->ecdsa),
			pb = EC_KEY_get0_public_key(src->ecdsa);
		return EC_POINT_cmp(group, pa, pb, NULL) == 0;

	case KEY_ED25519:
		a = key;
		b = src;
		return a->ed25519_pk != NULL && b->ed25519_pk != NULL &&
			memcmp(a->ed25519_pk, b->ed25519_pk, ED25519_PK_SZ) == 0;

	default:
		return FALSE;
	}
}

#if 0
// pvar->hosts_state.hostkey と渡された公開鍵が等しいかを検証する
//   -1 ... 鍵の型が違う
//    0 ... 等しくない
//    1 ... 等しい
static int match_key(PTInstVar pvar, Key *key)
{
	return HOSTS_compare_public_key(&pvar->hosts_state.hostkey, key);
}
#endif

static void hosts_dlg_set_fingerprint(PTInstVar pvar, HWND dlg, digest_algorithm dgst_alg)
{
	char *fp = NULL;

	// fingerprintを設定する
	switch (dgst_alg) {
	case SSH_DIGEST_MD5:
		fp = key_fingerprint(&pvar->hosts_state.hostkey, SSH_FP_HEX, dgst_alg);
		if (fp != NULL) {
			SendMessage(GetDlgItem(dlg, IDC_FINGER_PRINT), WM_SETTEXT, 0, (LPARAM)fp);
			free(fp);
		}
		break;
	case SSH_DIGEST_SHA256:
	default:
		fp = key_fingerprint(&pvar->hosts_state.hostkey, SSH_FP_BASE64, SSH_DIGEST_SHA256);
		if (fp != NULL) {
			SendMessage(GetDlgItem(dlg, IDC_FINGER_PRINT), WM_SETTEXT, 0, (LPARAM)fp);
			free(fp);
		}
		break;
	}

	// ビジュアル化fingerprintを表示する
	fp = key_fingerprint(&pvar->hosts_state.hostkey, SSH_FP_RANDOMART, dgst_alg);
	if (fp != NULL) {
		SendMessage(GetDlgItem(dlg, IDC_FP_RANDOMART), WM_SETTEXT, 0, (LPARAM)fp);
		free(fp);
	}
}

static void init_hosts_dlg(PTInstVar pvar, HWND dlg)
{
	char buf[1024];
	char buf2[2048];
	int i, j;
	int ch;

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

	pvar->hFontFixed = UTIL_get_lang_fixedfont(dlg, pvar->ts->UILanguageFile);
	if (pvar->hFontFixed != NULL) {
		SendDlgItemMessage(dlg, IDC_FP_RANDOMART, WM_SETFONT,
						   (WPARAM)pvar->hFontFixed, MAKELPARAM(TRUE,0));
	}

	CheckDlgButton(dlg, IDC_FP_HASH_ALG_SHA256, TRUE);
	hosts_dlg_set_fingerprint(pvar, dlg, SSH_DIGEST_SHA256);
}

static int print_mp_int(char *buf, unsigned char *mp)
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
static char *format_host_key(PTInstVar pvar)
{
	int host_len = strlen(pvar->hosts_state.prefetched_hostname);
	char *result = NULL;
	int index;
	ssh_keytype type = pvar->hosts_state.hostkey.type;

	switch (type) {
	case KEY_RSA1:
	{
		int result_len = host_len + 50 + 8 +
		                 get_ushort16_MSBfirst(pvar->hosts_state.hostkey.exp) / 3 +
		                 get_ushort16_MSBfirst(pvar->hosts_state.hostkey.mod) / 3;
		result = (char *) malloc(result_len);

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

		// 第2引数(sizeOfBuffer)の指定誤りにより、実際のバッファサイズより
		// 大きくなっていた問題を修正した。
		// ポート番号が22以外の場合、VS2005のdebug buildでは、add_host_key()の
		// free(keydata)で、かならず「ブレークポイントが発生しました。ヒープが壊れていることが
		// 原因として考えられます。」という例外が発生する。
		// release buildでは再現性が低い。
		_snprintf_s(result + index, result_len - index, _TRUNCATE,
		            " %d ", pvar->hosts_state.hostkey.bits);
		index += strlen(result + index);
		index += print_mp_int(result + index, pvar->hosts_state.hostkey.exp);
		result[index] = ' ';
		index++;
		index += print_mp_int(result + index, pvar->hosts_state.hostkey.mod);
		strncpy_s(result + index, result_len - index, " \r\n", _TRUNCATE);

		break;
	}

	case KEY_RSA:
	case KEY_DSA:
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
	case KEY_ED25519:
	{
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

		break;
	}

	default:
		return NULL;

	}

	return result;
}

static char *format_specified_host_key(Key *key, char *hostname, unsigned short tcpport)
{
	int host_len = strlen(hostname);
	char *result = NULL;
	int index;
	ssh_keytype type = key->type;

	switch (type) {
	case KEY_RSA1:
	{
		int result_len = host_len + 50 + 8 +
			get_ushort16_MSBfirst(key->exp) / 3 +
			get_ushort16_MSBfirst(key->mod) / 3;
		result = (char *) malloc(result_len);

		if (tcpport == 22) {
			strncpy_s(result, result_len, hostname, _TRUNCATE);
			index = host_len;
		}
		else {
			_snprintf_s(result, result_len, _TRUNCATE, "[%s]:%d",
				hostname,
				tcpport);
			index = strlen(result);
		}

		_snprintf_s(result + index, result_len - host_len, _TRUNCATE,
			" %d ", key->bits);
		index += strlen(result + index);
		index += print_mp_int(result + index, key->exp);
		result[index] = ' ';
		index++;
		index += print_mp_int(result + index, key->mod);
		strncpy_s(result + index, result_len - index, " \r\n", _TRUNCATE);

		break;
	}

	case KEY_RSA:
	case KEY_DSA:
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
	case KEY_ED25519:
	{
		//Key *key = &pvar->hosts_state.hostkey;
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
			if (tcpport == 22) {
				_snprintf_s(result, msize, _TRUNCATE, "%s %s %s\r\n",
					hostname,
					get_sshname_from_key(key),
					uu);
			}
			else {
				_snprintf_s(result, msize, _TRUNCATE, "[%s]:%d %s %s\r\n",
					hostname,
					tcpport,
					get_sshname_from_key(key),
					uu);
			}
		}
	error:
		if (blob != NULL)
			free(blob);
		if (uu != NULL)
			free(uu);

		break;
	}

	default:
		return NULL;

	}

	return result;
}

static void add_host_key(PTInstVar pvar)
{
	char *name = NULL;

	if ( pvar->hosts_state.file_names != NULL)
		name = pvar->hosts_state.file_names[0];

	if (name == NULL || name[0] == 0) {
		UTIL_get_lang_msg("MSG_HOSTS_FILE_UNSPECIFY_ERROR", pvar,
		                  "The host and its key cannot be added, because no known-hosts file has been specified.\n"
		                  "Restart Tera Term and specify a read/write known-hosts file in the TTSSH Setup dialog box.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	} else {
		char *keydata = format_host_key(pvar);
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

// 指定したキーを known_hosts に追加する。
void HOSTS_add_host_key(PTInstVar pvar, Key *key)
{
	char *name = NULL;
	char *hostname;
	unsigned short tcpport;

	hostname = pvar->ssh_state.hostname;
	tcpport = pvar->ssh_state.tcpport;

	if (pvar->hosts_state.file_names != NULL)
		name = pvar->hosts_state.file_names[0];

	if (name == NULL || name[0] == 0) {
		UTIL_get_lang_msg("MSG_HOSTS_FILE_UNSPECIFY_ERROR", pvar,
			"The host and its key cannot be added, because no known-hosts file has been specified.\n"
			"Restart Tera Term and specify a read/write known-hosts file in the TTSSH Setup dialog box.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	}
	else {
		char *keydata = format_specified_host_key(key, hostname, tcpport);
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
			}
			else {
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

//
// 同じホストで内容の異なるキーを削除する
// add_host_key のあとに呼ぶこと
//
static void delete_different_key(PTInstVar pvar)
{
	char *name = pvar->hosts_state.file_names[0];

	if (name == NULL || name[0] == 0) {
		UTIL_get_lang_msg("MSG_HOSTS_FILE_UNSPECIFY_ERROR", pvar,
		                  "The host and its key cannot be added, because no known-hosts file has been specified.\n"
		                  "Restart Tera Term and specify a read/write known-hosts file in the TTSSH Setup dialog box.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	}
	else {
		Key key; // known_hostsに登録されている鍵
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

		// ファイルから読み込む
		memset(&key, 0, sizeof(key));
		begin_read_host_files(pvar, 0);
		do {
			int host_index = 0;
			int matched = 0;
			int keybits = 0;
			char *data;
			int do_write = 0;
			length = amount_written = 0;

			if (!read_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, 0, 1, &key)) {
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

				// ホストが等しくない
				if (!matched) {
					do_write = 1;
				}
				// ホストが等しい
				else {
					// 鍵の形式が違う or 合致するキー
					if (HOSTS_compare_public_key(&pvar->hosts_state.hostkey, &key) != 0) {
						do_write = 1;
					}
					// 鍵の形式が同じで合致しないキーはスキップされる
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

		// 最後にメモリを解放しておく。
		key_init(&key);
	}
}


void HOSTS_delete_all_hostkeys(PTInstVar pvar)
{
	char *name = pvar->hosts_state.file_names[0];
	char *hostname;
	unsigned short tcpport;

	hostname = pvar->ssh_state.hostname;
	tcpport = pvar->ssh_state.tcpport;

	if (name == NULL || name[0] == 0) {
		UTIL_get_lang_msg("MSG_HOSTS_FILE_UNSPECIFY_ERROR", pvar,
			"The host and its key cannot be added, because no known-hosts file has been specified.\n"
			"Restart Tera Term and specify a read/write known-hosts file in the TTSSH Setup dialog box.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	}
	else {
		Key key; // known_hostsに登録されている鍵
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
		tmpnam_s(tmp, sizeof(tmp));
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
			}
			else {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
					"An error occurred while trying to write the host key.\n"
					"The host key could not be written.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			}
			return;
		}

		// ファイルから読み込む
		memset(&key, 0, sizeof(key));
		begin_read_host_files(pvar, 0);
		do {
			int host_index = 0;
			int matched = 0;
			int keybits = 0;
			char *data;
			int do_write = 0;
			length = amount_written = 0;

			if (!read_host_key(pvar, pvar->ssh_state.hostname, pvar->ssh_state.tcpport, 0, 1, &key)) {
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

				// ホストが等しくない
				if (!matched) {
					do_write = 1;
				}
				// ホストが等しい
				else {
					// 一切書き込みをしない。

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

		// 最後にメモリを解放しておく。
		key_init(&key);
	}
}


//
// Unknown hostのホスト公開鍵を known_hosts ファイルへ保存するかどうかを
// ユーザに確認させる。
// TODO: finger printの表示も行う。
// (2006.3.25 yutaka)
//
static INT_PTR CALLBACK hosts_add_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
										   LPARAM lParam)
{
	const static DlgTextInfo text_info[] = {
		{ 0, "DLG_UNKNOWNHOST_TITLE" },
		{ IDC_HOSTWARNING, "DLG_UNKNOWNHOST_WARNING" },
		{ IDC_HOSTWARNING2, "DLG_UNKNOWNHOST_WARNING2" },
		{ IDC_HOSTFINGERPRINT, "DLG_UNKNOWNHOST_FINGERPRINT" },
		{ IDC_FP_HASH_ALG, "DLG_UNKNOWNHOST_FP_HASH_ALGORITHM" },
		{ IDC_ADDTOKNOWNHOSTS, "DLG_UNKNOWNHOST_ADD" },
		{ IDC_CONTINUE, "BTN_CONTINUE" },
		{ IDCANCEL, "BTN_DISCONNECT" },
	};
	PTInstVar pvar;

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		pvar->hosts_state.hosts_dialog = dlg;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		// 追加・置き換えとも init_hosts_dlg を呼んでいるので、その前にセットする必要がある
		SetI18nDlgStrs("TTSSH", dlg, text_info, _countof(text_info), pvar->ts->UILanguageFile);

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_NOTFOUND:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_NOTFOUND", pvar, "No host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_AUTH_MATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MATCH", pvar, "Matching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MISMATCH", pvar, "Mismatching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_DIFFERENTTYPE:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_DIFFTYPE", pvar, "Mismatching host key type found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		}

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_NG", pvar, "Found insecure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_AUTH_MATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_OK", pvar, "Found secure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->ts->UIMsg);
			break;
		}

		init_hosts_dlg(pvar, dlg);
		// add host check boxにチェックをデフォルトで入れておく 
		SendMessage(GetDlgItem(dlg, IDC_ADDTOKNOWNHOSTS), BM_SETCHECK, BST_CHECKED, 0);

		CenterWindow(dlg, GetParent(dlg));

		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDC_CONTINUE:
			// 認証中にサーバから切断された場合は、キャンセル扱いとする。(2014.3.31 yutaka)
			if (!pvar->cv->Ready) {
				goto canceled;
			}

			if (IsDlgButtonChecked(dlg, IDC_ADDTOKNOWNHOSTS)) {
				add_host_key(pvar);
			}

			/*
			 * known_hostsダイアログのために一時停止していた
			 * SSHサーバとのネゴシエーションを再開させる。
			 */
			HOSTS_resume_session_after_known_hosts(pvar);

			pvar->hosts_state.hosts_dialog = NULL;

			EndDialog(dlg, 1);
			return TRUE;

		case IDCANCEL:			/* kill the connection */
canceled:
			/*
			 * known_hostsをキャンセルするため、再開用のリソースを破棄しておく。
			 */
			HOSTS_cancel_session_after_known_hosts(pvar);

			pvar->hosts_state.hosts_dialog = NULL;
			notify_closed_connection(pvar, "authentication cancelled");
			EndDialog(dlg, 0);
			return TRUE;

		case IDCLOSE:
			/*
			 * known_hosts中にサーバ側からネットワーク切断された場合、
			 * ダイアログのみを閉じる。
			 */
			HOSTS_cancel_session_after_known_hosts(pvar);

			pvar->hosts_state.hosts_dialog = NULL;
			EndDialog(dlg, 0);
			return TRUE;

		case IDC_FP_HASH_ALG_MD5:
			hosts_dlg_set_fingerprint(pvar, dlg, SSH_DIGEST_MD5);
			return TRUE;

		case IDC_FP_HASH_ALG_SHA256:
			hosts_dlg_set_fingerprint(pvar, dlg, SSH_DIGEST_SHA256);
			return TRUE;

		default:
			return FALSE;
		}

	case WM_DPICHANGED:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);
		if (pvar->hFontFixed != NULL) {
			DeleteObject(pvar->hFontFixed);
		}
		pvar->hFontFixed = UTIL_get_lang_fixedfont(dlg, pvar->ts->UILanguageFile);
		if (pvar->hFontFixed != NULL) {
			SendDlgItemMessage(dlg, IDC_FP_RANDOMART, WM_SETFONT,
							   (WPARAM)pvar->hFontFixed, MAKELPARAM(TRUE,0));
		}
		return FALSE;

	case WM_DESTROY:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);
		if (pvar->hFontFixed != NULL) {
			DeleteObject(pvar->hFontFixed);
			pvar->hFontFixed = NULL;
		}
		return FALSE;

	default:
		return FALSE;
	}
}

//
// 置き換え時の確認ダイアログを分離
//
static INT_PTR CALLBACK hosts_replace_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
											   LPARAM lParam)
{
	const static DlgTextInfo text_info[] = {
		{ 0, "DLG_UNKNOWNHOST_TITLE" },
		{ IDC_HOSTWARNING, "DLG_DIFFERENTKEY_WARNING" },
		{ IDC_HOSTWARNING2, "DLG_DIFFERENTKEY_WARNING2" },
		{ IDC_HOSTFINGERPRINT, "DLG_DIFFERENTKEY_FINGERPRINT" },
		{ IDC_FP_HASH_ALG, "DLG_DIFFERENTKEY_FP_HASH_ALGORITHM" },
		{ IDC_ADDTOKNOWNHOSTS, "DLG_DIFFERENTKEY_REPLACE" },
		{ IDC_CONTINUE, "BTN_CONTINUE" },
		{ IDCANCEL, "BTN_DISCONNECT" },
	};
	PTInstVar pvar;

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		pvar->hosts_state.hosts_dialog = dlg;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		// 追加・置き換えとも init_hosts_dlg を呼んでいるので、その前にセットする必要がある
		SetI18nDlgStrs("TTSSH", dlg, text_info, _countof(text_info), pvar->ts->UILanguageFile);

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_NOTFOUND:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_NOTFOUND", pvar, "No host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_AUTH_MATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MATCH", pvar, "Matching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MISMATCH", pvar, "Mismatching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_DIFFERENTTYPE:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_DIFFTYPE", pvar, "Mismatching host key type found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		}

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_NG", pvar, "Found insecure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_AUTH_MATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_OK", pvar, "Found secure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->ts->UIMsg);
			break;
		}

		init_hosts_dlg(pvar, dlg);
		CenterWindow(dlg, GetParent(dlg));
		// デフォルトでチェックは入れない
		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDC_CONTINUE:
			// 認証中にサーバから切断された場合は、キャンセル扱いとする。(2014.3.31 yutaka)
			if (!pvar->cv->Ready) {
				goto canceled;
			}

			if (IsDlgButtonChecked(dlg, IDC_ADDTOKNOWNHOSTS)) {
				add_host_key(pvar);
				delete_different_key(pvar);
			}

			/*
			 * known_hostsダイアログのために一時停止していた
			 * SSHサーバとのネゴシエーションを再開させる。
			 */
			HOSTS_resume_session_after_known_hosts(pvar);

			pvar->hosts_state.hosts_dialog = NULL;

			EndDialog(dlg, 1);
			return TRUE;

		case IDCANCEL:			/* kill the connection */
canceled:
			/*
			 * known_hostsをキャンセルするため、再開用のリソースを破棄しておく。
			 */
			HOSTS_cancel_session_after_known_hosts(pvar);

			pvar->hosts_state.hosts_dialog = NULL;
			notify_closed_connection(pvar, "authentication cancelled");
			EndDialog(dlg, 0);
			return TRUE;

		case IDCLOSE:
			/*
			 * known_hosts中にサーバ側からネットワーク切断された場合、
			 * ダイアログのみを閉じる。
			 */
			HOSTS_cancel_session_after_known_hosts(pvar);

			pvar->hosts_state.hosts_dialog = NULL;
			EndDialog(dlg, 0);
			return TRUE;

		case IDC_FP_HASH_ALG_MD5:
			hosts_dlg_set_fingerprint(pvar, dlg, SSH_DIGEST_MD5);
			return TRUE;

		case IDC_FP_HASH_ALG_SHA256:
			hosts_dlg_set_fingerprint(pvar, dlg, SSH_DIGEST_SHA256);
			return TRUE;

		default:
			return FALSE;
		}

	case WM_DPICHANGED:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);
		if (pvar->hFontFixed != NULL) {
			DeleteObject(pvar->hFontFixed);
		}
		pvar->hFontFixed = UTIL_get_lang_fixedfont(dlg, pvar->ts->UILanguageFile);
		if (pvar->hFontFixed != NULL) {
			SendDlgItemMessage(dlg, IDC_FP_RANDOMART, WM_SETFONT,
							   (WPARAM)pvar->hFontFixed, MAKELPARAM(TRUE,0));
		}
		return FALSE;

	case WM_DESTROY:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);
		if (pvar->hFontFixed != NULL) {
			DeleteObject(pvar->hFontFixed);
			pvar->hFontFixed = NULL;
		}
		return FALSE;

	default:
		return FALSE;
	}
}

//
// 同じホストで鍵形式が違う時の追加確認ダイアログを分離
//
static INT_PTR CALLBACK hosts_add2_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
											LPARAM lParam)
{
	const static DlgTextInfo text_info[] = {
		{ 0, "DLG_DIFFERENTTYPEKEY_TITLE" },
		{ IDC_HOSTWARNING, "DLG_DIFFERENTTYPEKEY_WARNING" },
		{ IDC_HOSTWARNING2, "DLG_DIFFERENTTYPEKEY_WARNING2" },
		{ IDC_HOSTFINGERPRINT, "DLG_DIFFERENTTYPEKEY_FINGERPRINT" },
		{ IDC_FP_HASH_ALG, "DLG_DIFFERENTTYPEKEY_FP_HASH_ALGORITHM" },
		{ IDC_ADDTOKNOWNHOSTS, "DLG_DIFFERENTTYPEKEY_ADD" },
		{ IDC_CONTINUE, "BTN_CONTINUE" },
		{ IDCANCEL, "BTN_DISCONNECT" },
	};
	PTInstVar pvar;

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		pvar->hosts_state.hosts_dialog = dlg;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		// 追加・置き換えとも init_hosts_dlg を呼んでいるので、その前にセットする必要がある
		SetI18nDlgStrs("TTSSH", dlg, text_info, _countof(text_info), pvar->ts->UILanguageFile);

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_NOTFOUND:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_NOTFOUND", pvar, "No host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_AUTH_MATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MATCH", pvar, "Matching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MISMATCH", pvar, "Mismatching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_DIFFERENTTYPE:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_DIFFTYPE", pvar, "Mismatching host key type found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->ts->UIMsg);
			break;
		}

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_NG", pvar, "Found insecure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->ts->UIMsg);
			break;
		case DNS_VERIFY_AUTH_MATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_OK", pvar, "Found secure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->ts->UIMsg);
			break;
		}

		init_hosts_dlg(pvar, dlg);
		CenterWindow(dlg, GetParent(dlg));
		// add host check box のデフォルトは off にする
		// SendMessage(GetDlgItem(dlg, IDC_ADDTOKNOWNHOSTS), BM_SETCHECK, BST_CHECKED, 0);

		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDC_CONTINUE:
			// 認証中にサーバから切断された場合は、キャンセル扱いとする。(2014.3.31 yutaka)
			if (!pvar->cv->Ready) {
				goto canceled;
			}

			if (IsDlgButtonChecked(dlg, IDC_ADDTOKNOWNHOSTS)) {
				add_host_key(pvar);
			}

			/*
			 * known_hostsダイアログのために一時停止していた
			 * SSHサーバとのネゴシエーションを再開させる。
			 */
			HOSTS_resume_session_after_known_hosts(pvar);

			pvar->hosts_state.hosts_dialog = NULL;

			EndDialog(dlg, 1);
			return TRUE;

		case IDCANCEL:			/* kill the connection */
canceled:
			/*
			 * known_hostsをキャンセルするため、再開用のリソースを破棄しておく。
			 */
			HOSTS_cancel_session_after_known_hosts(pvar);

			pvar->hosts_state.hosts_dialog = NULL;
			notify_closed_connection(pvar, "authentication cancelled");
			EndDialog(dlg, 0);
			return TRUE;

		case IDCLOSE:
			/*
			 * known_hosts中にサーバ側からネットワーク切断された場合、
			 * ダイアログのみを閉じる。
			 */
			HOSTS_cancel_session_after_known_hosts(pvar);

			pvar->hosts_state.hosts_dialog = NULL;
			EndDialog(dlg, 0);
			return TRUE;

		case IDC_FP_HASH_ALG_MD5:
			hosts_dlg_set_fingerprint(pvar, dlg, SSH_DIGEST_MD5);
			return TRUE;

		case IDC_FP_HASH_ALG_SHA256:
			hosts_dlg_set_fingerprint(pvar, dlg, SSH_DIGEST_SHA256);
			return TRUE;

		default:
			return FALSE;
		}

	case WM_DPICHANGED:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);
		if (pvar->hFontFixed != NULL) {
			DeleteObject(pvar->hFontFixed);
		}
		pvar->hFontFixed = UTIL_get_lang_fixedfont(dlg, pvar->ts->UILanguageFile);
		if (pvar->hFontFixed != NULL) {
			SendDlgItemMessage(dlg, IDC_FP_RANDOMART, WM_SETFONT,
							   (WPARAM)pvar->hFontFixed, MAKELPARAM(TRUE,0));
		}
		return FALSE;

	case WM_DESTROY:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);
		if (pvar->hFontFixed != NULL) {
			DeleteObject(pvar->hFontFixed);
			pvar->hFontFixed = NULL;
		}
		return FALSE;

	default:
		return FALSE;
	}
}

void HOSTS_do_unknown_host_dialog(HWND wnd, PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog == NULL) {
		/* known_hostsの読み込み時、ID_SSHASYNCMESSAGEBOX を使った
		 * MessageBox が表示される場合、オーナーなし(no owner)になるため、
		 * MessageBox がTera Termの裏に隠れることがある。
		 * その状態で GetActiveWindow() を呼び出すと、known_hostsダイアログの
		 * オーナーが MessageBox となって、Tera Termの裏に隠れてしまう。
		 * そこで、known_hostsダイアログのオーナーは常に Tera Term を指し示す
		 * ようにする。
		 */
		HWND cur_active = NULL;

		DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHUNKNOWNHOST),
		               cur_active != NULL ? cur_active : wnd,
		               hosts_add_dlg_proc, (LPARAM) pvar);
	}
}

void HOSTS_do_different_key_dialog(HWND wnd, PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog == NULL) {
		/* known_hostsの読み込み時、ID_SSHASYNCMESSAGEBOX を使った
		 * MessageBox が表示される場合、オーナーなし(no owner)になるため、
		 * MessageBox がTera Termの裏に隠れることがある。
		 * その状態で GetActiveWindow() を呼び出すと、known_hostsダイアログの
		 * オーナーが MessageBox となって、Tera Termの裏に隠れてしまう。
		 * そこで、known_hostsダイアログのオーナーは常に Tera Term を指し示す
		 * ようにする。
		 */
		HWND cur_active = NULL;

		DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHDIFFERENTKEY),
		               cur_active != NULL ? cur_active : wnd,
		               hosts_replace_dlg_proc, (LPARAM) pvar);
	}
}

void HOSTS_do_different_type_key_dialog(HWND wnd, PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog == NULL) {
		/* known_hostsの読み込み時、ID_SSHASYNCMESSAGEBOX を使った
		 * MessageBox が表示される場合、オーナーなし(no owner)になるため、
		 * MessageBox がTera Termの裏に隠れることがある。
		 * その状態で GetActiveWindow() を呼び出すと、known_hostsダイアログの
		 * オーナーが MessageBox となって、Tera Termの裏に隠れてしまう。
		 * そこで、known_hostsダイアログのオーナーは常に Tera Term を指し示す
		 * ようにする。
		 */
		HWND cur_active = NULL;

		DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHDIFFERENTTYPEKEY),
		               cur_active != NULL ? cur_active : wnd,
		               hosts_add2_dlg_proc, (LPARAM) pvar);
	}
}

/*
 * サーバから送られてきたホスト公開鍵の妥当性をチェックし、
 * 必要に応じて known_hosts ダイアログを呼び出す。
 *
 *   hostname: 接続先のホスト名
 *   tcpport: 接続先ポート番号 
 *   key: サーバからの公開鍵
 *
 * return:
 *    TRUE:  known_hostsダイアログの呼び出しは不要
 *    FALSE: known_hostsダイアログを呼び出した
 *
 */
BOOL HOSTS_check_host_key(PTInstVar pvar, char *hostname, unsigned short tcpport, Key *key)
{
	int found_different_key = 0, found_different_type_key = 0;
	Key key2; // known_hostsに登録されている鍵
	DWORD id;

	pvar->dns_key_check = DNS_VERIFY_NONE;

	// すでに known_hosts ファイルからホスト公開鍵を読み込んでいるなら、それと比較する。
	if (pvar->hosts_state.prefetched_hostname != NULL
	 && _stricmp(pvar->hosts_state.prefetched_hostname, hostname) == 0
	 && HOSTS_compare_public_key(&pvar->hosts_state.hostkey, key) == 1) {

		 // 何もせずに戻る。		
		 return TRUE;
	}

	// 先読みされていない場合は、この時点でファイルから読み込む
	memset(&key2, 0, sizeof(key2));
	if (begin_read_host_files(pvar, 0)) {
		do {
			if (!read_host_key(pvar, hostname, tcpport, 0, 0, &key2)) {
				break;
			}

			if (key2.type != KEY_UNSPEC) {
				int match = HOSTS_compare_public_key(&key2, key);
				if (match == 1) {
					finish_read_host_files(pvar, 0);
					// すべてのエントリを参照して、合致するキーが見つかったら戻る。
					// About TTSSH ダイアログでの表示のために、ここで保存しておく。
					key_copy(&pvar->hosts_state.hostkey, key);

					return TRUE;
				}
				else if (match == 0) {
					// キーは known_hosts に見つかったが、キーの内容が異なる。
					found_different_key = 1;
				}
				else {
					// キーの形式が違う場合
					found_different_type_key = 1;
				}
			}
		} while (key2.type != KEY_UNSPEC);  // キーが見つかっている間はループする

		key_init(&key2);
		finish_read_host_files(pvar, 0);
	}

	// known_hosts に存在しないキーはあとでファイルへ書き込むために、ここで保存しておく。
	key_copy(&pvar->hosts_state.hostkey, key);

	free(pvar->hosts_state.prefetched_hostname);
	pvar->hosts_state.prefetched_hostname = _strdup(hostname);

	// "/nosecuritywarning"が指定されている場合、ダイアログを表示させずに return success する。
	if (pvar->nocheck_known_hosts == TRUE) {
		 // 何もせずに戻る。		
		return TRUE;
	}

	if (pvar->settings.VerifyHostKeyDNS && !is_numeric_hostname(hostname)) {
		pvar->dns_key_check = verify_hostkey_dns(pvar, hostname, key);
	}

	// known_hostsダイアログは同期的に表示させ、この時点においてユーザに確認
	// させる必要があるため、直接コールに変更する。
	// これによりknown_hostsの確認を待たずに、サーバへユーザ情報を送ってしまう問題を回避する。
	// (2007.10.1 yutaka)
	/*
	 * known_hostsダイアログは非同期で表示させるのが正しかった。
	 * known_hostsダイアログが表示されている状態で、サーバから切断を行うと、
	 * TTXCloseTCPが呼び出され、TTSSHのリソースが解放されてしまう。
	 * SSHハンドラの延長でknown_hostsダイアログを出して止まっているため、
	 * 処理再開後に不正アクセスで落ちる。
	 * (2019.9.3 yutaka)
	 */
	if (found_different_key) {
		// TTXProcessCommand から HOSTS_do_different_key_dialog() を呼び出す。
		id = ID_SSHDIFFERENTKEY;
	}
	else if (found_different_type_key) {
		// TTXProcessCommand から HOSTS_do_different_type_key_dialog() を呼び出す。
		id = ID_SSHDIFFERENT_TYPE_KEY;
	}
	else {
		// TTXProcessCommand から HOSTS_do_unknown_host_dialog() を呼び出す。
		id = ID_SSHUNKNOWNHOST;
	}

	PostMessage(pvar->NotificationWindow, WM_COMMAND, id, 0);

	logprintf(LOG_LEVEL_INFO, "Calling known_hosts dialog...(%s)", 
		id == ID_SSHDIFFERENTKEY ? "SSHDIFFERENTKEY" : 
		id == ID_SSHDIFFERENT_TYPE_KEY ? "SSHDIFFERENT_TYPE_KEY" : "SSHUNKNOWNHOST"
		);

	return FALSE;
}

/*
 * known_hostsダイアログでユーザ承認後、SSHサーバとのネゴシエーションを再開する。
 */
BOOL HOSTS_resume_session_after_known_hosts(PTInstVar pvar)
{
	enum ssh_kex_known_hosts type;
	int ret = FALSE;

	type = pvar->contents_after_known_hosts.kex_type;
	if (type == SSH1_PUBLIC_KEY_KNOWN_HOSTS) {
		ret = handle_server_public_key_after_known_hosts(pvar);

	} else if (type == SSH2_DH_KEX_REPLY_KNOWN_HOSTS) {
		ret = handle_SSH2_dh_kex_reply_after_known_hosts(pvar);

	} else if (type == SSH2_DH_GEX_REPLY_KNOWN_HOSTS) {
		ret = handle_SSH2_dh_gex_reply_after_known_hosts(pvar);

	} else if (type == SSH2_ECDH_KEX_REPLY_KNOWN_HOSTS) {
		ret = handle_SSH2_ecdh_kex_reply_after_known_hosts(pvar);

	}

	return (ret);
}

/*
 * known_hostsダイアログのSSHごとのキャンセル処理
 */
void HOSTS_cancel_session_after_known_hosts(PTInstVar pvar)
{
	enum ssh_kex_known_hosts type;

	type = pvar->contents_after_known_hosts.kex_type;
	if (type != NONE_KNOWN_HOSTS) {
		handle_SSH2_canel_reply_after_known_hosts(pvar);
	}

	return;
}


void HOSTS_notify_disconnecting(PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog != NULL) {
		PostMessage(pvar->hosts_state.hosts_dialog, WM_COMMAND, IDCANCEL, 0);
		/* the main window might not go away if it's not enabled. (see vtwin.cpp) */
		EnableWindow(pvar->NotificationWindow, TRUE);
	}
}

// TCPセッションがクローズされた場合、known_hostsダイアログを閉じるように指示を出す。
// HOSTS_notify_disconnecting()とは異なり、ダイアログを閉じるのみで、
// SSHサーバに通知は出さない。
void HOSTS_notify_closing_on_exit(PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog != NULL) {
		logprintf(LOG_LEVEL_INFO, "%s: Notify closing message to the known_hosts dialog.", __FUNCTION__);
		PostMessage(pvar->hosts_state.hosts_dialog, WM_COMMAND, IDCLOSE, 0);
	}
}

void HOSTS_end(PTInstVar pvar)
{
	int i;

	free(pvar->hosts_state.prefetched_hostname);
	key_init(&pvar->hosts_state.hostkey);

	if (pvar->hosts_state.file_names != NULL) {
		for (i = 0; pvar->hosts_state.file_names[i] != NULL; i++) {
			free(pvar->hosts_state.file_names[i]);
		}
		free(pvar->hosts_state.file_names);
	}
}

/* vim: set ts=4 sw=4 ff=dos : */
