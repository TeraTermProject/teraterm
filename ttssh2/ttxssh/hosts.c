/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (C) 2004- TeraTerm Project
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
#include "codeconv.h"

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
#include <wchar.h>

#include "codeconv.h"
#include "asprintf.h"

// BASE64�\��������i�����ł�'='�͊܂܂�Ă��Ȃ��j
static char base64[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


static wchar_t **parse_multi_path(wchar_t *buf)
{
	int i;
	wchar_t ch;
	int num_paths = 1;
	wchar_t ** result;
	int last_path_index;

	for (i = 0; (ch = buf[i]) != 0; i++) {
		if (ch == ';') {
			num_paths++;
		}
	}

	result =
		(wchar_t **) malloc(sizeof(wchar_t *) * (num_paths + 1));

	last_path_index = 0;
	num_paths = 0;
	for (i = 0; (ch = buf[i]) != 0; i++) {
		if (ch == ';') {
			buf[i] = 0;
			result[num_paths] = _wcsdup(buf + last_path_index);
			num_paths++;
			buf[i] = ch;
			last_path_index = i + 1;
		}
	}
	if (i > last_path_index) {
		result[num_paths] = _wcsdup(buf + last_path_index);
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
	 * �O��̃I�v�V�����w��(/nosecuritywarning)���c��Ȃ��悤�ɏ��������Ă����B
	 */
	pvar->nocheck_known_hosts = FALSE;
}

void HOSTS_open(PTInstVar pvar)
{
	wchar_t *known_hosts_filesW = ToWcharA(pvar->session_settings.KnownHostsFiles);
	pvar->hosts_state.file_names =
		parse_multi_path(known_hosts_filesW);
	free(known_hosts_filesW);
}

//
// known_hosts�t�@�C���̓��e�����ׂ� pvar->hosts_state.file_data �֓ǂݍ���
//
static int begin_read_file(PTInstVar pvar, wchar_t *name,
                           int suppress_errors)
{
	int fd;
	int length;
	int amount_read;
	wchar_t *bufW;

	bufW = get_home_dir_relative_nameW(name);
	fd = _wopen(bufW, _O_RDONLY | _O_SEQUENTIAL | _O_BINARY);
	free(bufW);
	if (fd == -1) {
		if (!suppress_errors) {
			if (errno == ENOENT) {
				UTIL_get_lang_msg("MSG_HOSTS_READ_ENOENT_ERROR", pvar,
				                  "An error occurred while trying to read a known_hosts file.\n"
				                  "The specified filename does not exist.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
			} else {
				UTIL_get_lang_msg("MSG_HOSTS_READ_ERROR", pvar,
				                  "An error occurred while trying to read a known_hosts file.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
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
				notify_nonfatal_error(pvar, pvar->UIMsg);
			}
			_close(fd);
			return 0;
		}
	} else {
		if (!suppress_errors) {
			UTIL_get_lang_msg("MSG_HOSTS_READ_ERROR", pvar,
			                  "An error occurred while trying to read a known_hosts file.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
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
			notify_nonfatal_error(pvar, pvar->UIMsg);
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

// MIME64�̕�������X�L�b�v����
static int eat_base64(char *data)
{
	int index = 0;
	int ch;

	for (;;) {
		ch = data[index];
		if (ch == '=' || strchr(base64, ch)) {
			// BASE64�̍\������������������ index ��i�߂�
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

// SSH2���� BASE64 �`���Ŋi�[����Ă���
static Key *parse_base64data(char *data)
{
	int count;
	unsigned char *blob = NULL;
	int len, n;
	Key *key = NULL;
	char ch;

	// BASE64������̃T�C�Y�𓾂�
	count = eat_base64(data);
	len = 2 * count;
	blob = malloc(len);
	if (blob == NULL)
		goto error;

	// BASE64�f�R�[�h
	ch = data[count];
	data[count] = '\0';  // �����͉��s�R�[�h�̂͂��Ȃ̂ŏ����ׂ��Ă����Ȃ��͂�
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

	// BN_CTX_init�֐��� OpenSSL 1.1.0 �ō폜���ꂽ�B
	// OpenSSL 1.0.2�̎��_�ł��ł� deprecated �����������B
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
// known_hosts�t�@�C���̓��e����͂��A�w�肵���z�X�g�̌��J����T���B
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
		// ���̎�ނɂ��t�H�[�}�b�g���قȂ�
		// �܂��A�ŏ��Ɉ�v�����G���g�����擾���邱�ƂɂȂ�B
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
		if (rsa1_key_bits > 0) { // RSA1�ł���
			if (!SSHv1(pvar)) { // SSH2�ڑ��ł���Ζ�������
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

			if (!SSHv2(pvar)) { // SSH1�ڑ��ł���Ζ�������
				return index + eat_to_end_of_line(data + index);
			}

			cp = data + index;
			p = strchr(cp, ' ');
			if (p == NULL) {
				return index + eat_to_end_of_line(data + index);
			}
			index += (p - cp);  // setup index
			*p = '\0';
			key_type = get_hostkey_type_from_name(cp);
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

			// Key�\���̎��g��������� (2008.3.2 yutaka)
			free(key2);
		}

		return index + eat_to_end_of_line(data + index);
	}
}

//
// known_hosts�t�@�C������z�X�g���ɍ��v����s��ǂ�
//   return_always
//     0: ������܂ŒT��
//     1: 1�s�����T���Ė߂�
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
				notify_fatal_error(pvar, pvar->UIMsg, TRUE);
			}
			return 0;
		}
	}

	if (i == 0) {
		if (!suppress_errors) {
			UTIL_get_lang_msg("MSG_HOSTS_HOSTNAME_EMPTY_ERROR", pvar,
			                  "The host name should not be empty.\n"
			                  "This session will be terminated.");
			notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		}
		return 0;
	}

	// hostkey type is KEY_UNSPEC.
	key_init(key);

	do {
		if (pvar->hosts_state.file_data == NULL
		 || pvar->hosts_state.file_data[pvar->hosts_state.file_data_index] == 0) {
			wchar_t *filename;
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
			// �L���ȃL�[��������܂�
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

// �T�[�o�֐ڑ�����O�ɁAknown_hosts�t�@�C������z�X�g���J�����ǂ݂��Ă����B
void HOSTS_prefetch_host_key(PTInstVar pvar, char *hostname, unsigned short tcpport)
{
	Key key; // known_hosts�ɓo�^����Ă��錮

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


// known_hosts�t�@�C������Y������L�[�����擾����B
//
// return:
//   *keyptr != NULL  �擾����
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
		// ���̎�ނɂ��t�H�[�}�b�g���قȂ�
		// �܂��A�ŏ��Ɉ�v�����G���g�����擾���邱�ƂɂȂ�B
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
		if (rsa1_key_bits > 0) { // RSA1�ł���
			if (!SSHv1(pvar)) { // SSH2�ڑ��ł���Ζ�������
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

			if (!SSHv2(pvar)) { // SSH1�ڑ��ł���Ζ�������
				return index + eat_to_end_of_line(data + index);
			}

			cp = data + index;
			p = strchr(cp, ' ');
			if (p == NULL) {
				return index + eat_to_end_of_line(data + index);
			}
			index += (p - cp);  // setup index
			*p = '\0';
			ktype = get_hostkey_type_from_name(cp);
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

// known_hosts�t�@�C������z�X�g���J�����擾����B
// �����̏��������ς������Ȃ��̂ŁAHost key rotation�p�ɐV�K�ɗp�ӂ���B
//
// return 1: success
//        0: fail
int HOSTS_hostkey_foreach(PTInstVar pvar, hostkeys_foreach_fn *callback, void *ctx)
{
	int success = 0;
	int suppress_errors = 1;
	unsigned short tcpport;
	wchar_t *filename;
	char *hostname;
	Key *key;

	if (!begin_read_host_files(pvar, 1)) {
		goto error;
	}

	// Host key rotation�ł́Aknown_hosts �t�@�C��������������̂ŁA
	// ��������̂�1�߂̃t�@�C���݂̂ł悢�i2�߂̃t�@�C����ReadOnly�̂��߁j�B
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

	// �����ΏۂƂȂ�z�X�g���ƃ|�[�g�ԍ��B
	hostname = pvar->ssh_state.hostname;
	tcpport = pvar->ssh_state.tcpport;

	// known_hosts�t�@�C���̓��e�����ׂ� pvar->hosts_state.file_data �ɓǂݍ��܂�Ă���B
	// ������ \0 �B
	while (pvar->hosts_state.file_data[pvar->hosts_state.file_data_index] != 0) {
		key = NULL;

		pvar->hosts_state.file_data_index +=
			parse_hostkey_file(pvar, hostname, tcpport,
				pvar->hosts_state.file_data +
				pvar->hosts_state.file_data_index,
				&key);

		// �Y�����錮������������A�R�[���o�b�N�֐����Ăяo���B
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


// ���J���̔�r���s���B
//
// return
//   -1 ... ���̌^���Ⴄ
//    0 ... �������Ȃ�
//    1 ... ������
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

	case KEY_DSA: { // SSH2 DSA host public key
		BIGNUM *p, *q, *g, *pub_key;
		BIGNUM *sp, *sq, *sg, *spub_key;

		DSA_get0_pqg(key->dsa, &p, &q, &g);
		DSA_get0_pqg(src->dsa, &sp, &sq, &sg);
		DSA_get0_key(key->dsa, &pub_key, NULL);
		DSA_get0_key(src->dsa, &spub_key, NULL);
		return key->dsa != NULL && src->dsa &&
			BN_cmp(p, sp) == 0 &&
			BN_cmp(q, sq) == 0 &&
			BN_cmp(g, sg) == 0 &&
			BN_cmp(pub_key, spub_key) == 0;
	}

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
// pvar->hosts_state.hostkey �Ɠn���ꂽ���J�����������������؂���
//   -1 ... ���̌^���Ⴄ
//    0 ... �������Ȃ�
//    1 ... ������
static int match_key(PTInstVar pvar, Key *key)
{
	return HOSTS_compare_public_key(&pvar->hosts_state.hostkey, key);
}
#endif

static void hosts_dlg_set_fingerprint(PTInstVar pvar, HWND dlg, digest_algorithm dgst_alg)
{
	char *fp = NULL;

	// fingerprint��ݒ肷��
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

	// �r�W���A����fingerprint��\������
	fp = key_fingerprint(&pvar->hosts_state.hostkey, SSH_FP_RANDOMART, dgst_alg);
	if (fp != NULL) {
		SendMessage(GetDlgItem(dlg, IDC_FP_RANDOMART), WM_SETTEXT, 0, (LPARAM)fp);
		free(fp);
	}
}

static void init_hosts_dlg(PTInstVar pvar, HWND dlg)
{
	wchar_t buf[MAX_UIMSG];
	wchar_t *buf2;
	wchar_t *hostW;

	// �z�X�g���ɒu������
	GetDlgItemTextW(dlg, IDC_HOSTWARNING, buf, _countof(buf));
	hostW = ToWcharA(pvar->hosts_state.prefetched_hostname);
	aswprintf(&buf2, buf, hostW);
	free(hostW);
	SetDlgItemTextW(dlg, IDC_HOSTWARNING, buf2);
	free(buf2);

	pvar->hFontFixed = UTIL_get_lang_fixedfont(dlg, pvar->ts->UILanguageFileW);
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
// known_hosts �t�@�C���֕ۑ�����G���g�����쐬����B
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

		// ��2����(sizeOfBuffer)�̎w����ɂ��A���ۂ̃o�b�t�@�T�C�Y���
		// �傫���Ȃ��Ă��������C�������B
		// �|�[�g�ԍ���22�ȊO�̏ꍇ�AVS2005��debug build�ł́Aadd_host_key()��
		// free(keydata)�ŁA���Ȃ炸�u�u���[�N�|�C���g���������܂����B�q�[�v�����Ă��邱�Ƃ�
		// �����Ƃ��čl�����܂��B�v�Ƃ�����O����������B
		// release build�ł͍Č������Ⴂ�B
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
				            get_ssh2_hostkey_type_name_from_key(key),
				            uu);
			} else {
				_snprintf_s(result, msize, _TRUNCATE, "[%s]:%d %s %s\r\n",
				            pvar->hosts_state.prefetched_hostname,
				            pvar->ssh_state.tcpport,
				            get_ssh2_hostkey_type_name_from_key(key),
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
					get_ssh2_hostkey_type_name_from_key(key),
					uu);
			}
			else {
				_snprintf_s(result, msize, _TRUNCATE, "[%s]:%d %s %s\r\n",
					hostname,
					tcpport,
					get_ssh2_hostkey_type_name_from_key(key),
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
	wchar_t *name = NULL;

	if ( pvar->hosts_state.file_names != NULL)
		name = pvar->hosts_state.file_names[0];

	if (name == NULL || name[0] == 0) {
		UTIL_get_lang_msg("MSG_HOSTS_FILE_UNSPECIFY_ERROR", pvar,
		                  "The host and its key cannot be added, because no known-hosts file has been specified.\n"
		                  "Restart Tera Term and specify a read/write known-hosts file in the TTSSH Setup dialog box.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
	} else {
		char *keydata = format_host_key(pvar);
		int length = strlen(keydata);
		int fd;
		int amount_written;
		int close_result;
		wchar_t *buf;

		buf = get_home_dir_relative_nameW(name);
		fd = _wopen(buf,
		          _O_APPEND | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL | _O_BINARY,
		          _S_IREAD | _S_IWRITE);
		free(buf);
		if (fd == -1) {
			if (errno == EACCES) {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_EACCES_ERROR", pvar,
				                  "An error occurred while trying to write the host key.\n"
				                  "You do not have permission to write to the known-hosts file.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
			} else {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
				                  "An error occurred while trying to write the host key.\n"
				                  "The host key could not be written.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
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
			notify_nonfatal_error(pvar, pvar->UIMsg);
		}
	}
}

// �w�肵���L�[�� known_hosts �ɒǉ�����B
void HOSTS_add_host_key(PTInstVar pvar, Key *key)
{
	wchar_t *name = NULL;
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
		notify_nonfatal_error(pvar, pvar->UIMsg);
	}
	else {
		char *keydata = format_specified_host_key(key, hostname, tcpport);
		int length = strlen(keydata);
		int fd;
		int amount_written;
		int close_result;
		wchar_t *buf;

		buf = get_home_dir_relative_nameW(name);
		fd = _wopen(buf,
			_O_APPEND | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL | _O_BINARY,
			_S_IREAD | _S_IWRITE);
		free(buf);
		if (fd == -1) {
			if (errno == EACCES) {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_EACCES_ERROR", pvar,
					"An error occurred while trying to write the host key.\n"
					"You do not have permission to write to the known-hosts file.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
			}
			else {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
					"An error occurred while trying to write the host key.\n"
					"The host key could not be written.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
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
			notify_nonfatal_error(pvar, pvar->UIMsg);
		}
	}
}

//
// �����z�X�g�œ��e�̈قȂ�L�[���폜����
// add_host_key �̂��ƂɌĂԂ���
//
static void delete_different_key(PTInstVar pvar)
{
	wchar_t *name = pvar->hosts_state.file_names[0];

	if (name == NULL || name[0] == 0) {
		UTIL_get_lang_msg("MSG_HOSTS_FILE_UNSPECIFY_ERROR", pvar,
		                  "The host and its key cannot be added, because no known-hosts file has been specified.\n"
		                  "Restart Tera Term and specify a read/write known-hosts file in the TTSSH Setup dialog box.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
	}
	else {
		Key key; // known_hosts�ɓo�^����Ă��錮
		int length;
		wchar_t *filename = NULL;
		int fd;
		int amount_written = 0;
		int close_result;
		int data_index = 0;
		char *newfiledata = NULL;
		int ret;
		struct _stat fileStat;
		long newFilePos = 0, totalSize;

		// known_hosts�t�@�C���T�C�Y���擾����B
		filename = get_home_dir_relative_nameW(name);
		ret = _wstat(filename, &fileStat);
		if (ret != 0) {
			// error
			goto error;
		}
		// �t�@�C���f�[�^�̃��������m�ۂ���B
		totalSize = fileStat.st_size;
		newfiledata = malloc(totalSize);
		if (newfiledata == NULL) {
			// error
			goto error;
		}


		// �t�@�C������ǂݍ���
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
				// index ���i�܂Ȃ� == �Ō�܂œǂ�
				break;
			}

			data = pvar->hosts_state.file_data + data_index;
			host_index = eat_spaces(data);

			if (data[host_index] == '#') {
				do_write = 1;
			}
			else {
				// �z�X�g�̏ƍ�
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
							// �ڑ��o�[�W�����`�F�b�N�̂��߂� host_index ��i�߂Ă��甲����
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

				// �z�X�g���������Ȃ�
				if (!matched) {
					do_write = 1;
				}
				// �z�X�g��������
				else {
					// ���̌`�����Ⴄ or ���v����L�[
					if (HOSTS_compare_public_key(&pvar->hosts_state.hostkey, &key) != 0) {
						do_write = 1;
					}
					// ���̌`���������ō��v���Ȃ��L�[�̓X�L�b�v�����
				}
			}

			// �������ݏ���
			if (do_write) {
				length = pvar->hosts_state.file_data_index - data_index;

				if ((newFilePos + length) >= totalSize) {
					UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
						"An error occurred while trying to write the host key.\n"
						"The host key could not be written.");
					notify_nonfatal_error(pvar, pvar->UIMsg);
					goto error;
				}

				memcpy(newfiledata + newFilePos,
					pvar->hosts_state.file_data + data_index,
					length);
				newFilePos += length;

			}
			data_index = pvar->hosts_state.file_data_index;
		} while (1); // �Ō�܂œǂ�

		finish_read_host_files(pvar, 0);

		// �Ō�Ƀ�������������Ă����B
		key_init(&key);

		// known_hosts�t�@�C���ɐV�����t�@�C���f�[�^�ŏ㏑������B
		fd = _wopen(filename,
			_O_CREAT | _O_WRONLY | _O_SEQUENTIAL | _O_BINARY | _O_TRUNC,
			_S_IREAD | _S_IWRITE);

		if (fd == -1) {
			if (errno == EACCES) {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_EACCES_ERROR", pvar,
					"An error occurred while trying to write the host key.\n"
					"You do not have permission to write to the known-hosts file.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
			}
			else {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
					"An error occurred while trying to write the host key.\n"
					"The host key could not be written.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
			}
			goto error;
		}

		amount_written = _write(fd, newfiledata, newFilePos);
		close_result = _close(fd);
		if (amount_written != newFilePos || close_result == -1) {
			UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
				"An error occurred while trying to write the host key.\n"
				"The host key could not be written.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
			goto error;
		}

error:
		free(filename);

		if (newfiledata) {
			free(newfiledata);
		}

	}
}


void HOSTS_delete_all_hostkeys(PTInstVar pvar)
{
	wchar_t *name = pvar->hosts_state.file_names[0];
	char *hostname;
	unsigned short tcpport;

	hostname = pvar->ssh_state.hostname;
	tcpport = pvar->ssh_state.tcpport;

	if (name == NULL || name[0] == 0) {
		UTIL_get_lang_msg("MSG_HOSTS_FILE_UNSPECIFY_ERROR", pvar,
			"The host and its key cannot be added, because no known-hosts file has been specified.\n"
			"Restart Tera Term and specify a read/write known-hosts file in the TTSSH Setup dialog box.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
	}
	else {
		Key key; // known_hosts�ɓo�^����Ă��錮
		int length;
		wchar_t *filename = NULL;
		int fd;
		int amount_written = 0;
		int close_result;
		int data_index = 0;
		char *newfiledata = NULL;
		int ret;
		struct _stat fileStat;
		long newFilePos = 0, totalSize;

		// known_hosts�t�@�C���T�C�Y���擾����B
		filename = get_home_dir_relative_nameW(name);
		ret = _wstat(filename, &fileStat);
		if (ret != 0) {
			// error
			goto error;
		}
		// �t�@�C���f�[�^�̃��������m�ۂ���B
		totalSize = fileStat.st_size;
		newfiledata = malloc(totalSize);
		if (newfiledata == NULL) {
			// error
			goto error;
		}

		// �t�@�C������ǂݍ���
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
				// index ���i�܂Ȃ� == �Ō�܂œǂ�
				break;
			}

			data = pvar->hosts_state.file_data + data_index;
			host_index = eat_spaces(data);

			if (data[host_index] == '#') {
				do_write = 1;
			}
			else {
				// �z�X�g�̏ƍ�
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
							// �ڑ��o�[�W�����`�F�b�N�̂��߂� host_index ��i�߂Ă��甲����
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

				// �z�X�g���������Ȃ�
				if (!matched) {
					do_write = 1;
				}
				// �z�X�g��������
				else {
					// ��؏������݂����Ȃ��B

				}
			}

			// �������ݏ���
			if (do_write) {
				length = pvar->hosts_state.file_data_index - data_index;

				if ((newFilePos + length) >= totalSize) {
					UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
						"An error occurred while trying to write the host key.\n"
						"The host key could not be written.");
					notify_nonfatal_error(pvar, pvar->UIMsg);
					goto error;
				}

				memcpy(newfiledata + newFilePos,
					pvar->hosts_state.file_data + data_index,
					length);
				newFilePos += length;

			}
			data_index = pvar->hosts_state.file_data_index;
		} while (1); // �Ō�܂œǂ�

		finish_read_host_files(pvar, 0);

		// �Ō�Ƀ�������������Ă����B
		key_init(&key);

		// known_hosts�t�@�C���ɐV�����t�@�C���f�[�^�ŏ㏑������B
		fd = _wopen(filename,
			_O_CREAT | _O_WRONLY | _O_SEQUENTIAL | _O_BINARY | _O_TRUNC,
			_S_IREAD | _S_IWRITE);

		if (fd == -1) {
			if (errno == EACCES) {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_EACCES_ERROR", pvar,
					"An error occurred while trying to write the host key.\n"
					"You do not have permission to write to the known-hosts file.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
			}
			else {
				UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
					"An error occurred while trying to write the host key.\n"
					"The host key could not be written.");
				notify_nonfatal_error(pvar, pvar->UIMsg);
			}
			goto error;
		}

		amount_written = _write(fd, newfiledata, newFilePos);
		close_result = _close(fd);
		if (amount_written != newFilePos || close_result == -1) {
			UTIL_get_lang_msg("MSG_HOSTS_WRITE_ERROR", pvar,
				"An error occurred while trying to write the host key.\n"
				"The host key could not be written.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
			goto error;
		}

error:
		free(filename);

		if (newfiledata) {
			free(newfiledata);
		}

	}
}


//
// Unknown host�̃z�X�g���J���� known_hosts �t�@�C���֕ۑ����邩�ǂ�����
// ���[�U�Ɋm�F������B
// TODO: finger print�̕\�����s���B
// (2006.3.25 yutaka)
//
static INT_PTR CALLBACK hosts_add_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
										   LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
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

		// �ǉ��E�u�������Ƃ� init_hosts_dlg ���Ă�ł���̂ŁA���̑O�ɃZ�b�g����K�v������
		SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_NOTFOUND:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_NOTFOUND", pvar, "No host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_AUTH_MATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MATCH", pvar, "Matching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MISMATCH", pvar, "Mismatching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		case DNS_VERIFY_DIFFERENTTYPE:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_DIFFTYPE", pvar, "Mismatching host key type found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		}

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_NG", pvar, "Found insecure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->UIMsg);
			break;
		case DNS_VERIFY_AUTH_MATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_OK", pvar, "Found secure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->UIMsg);
			break;
		}

		init_hosts_dlg(pvar, dlg);
		// add host check box�Ƀ`�F�b�N���f�t�H���g�œ���Ă���
		SendMessage(GetDlgItem(dlg, IDC_ADDTOKNOWNHOSTS), BM_SETCHECK, BST_CHECKED, 0);

		CenterWindow(dlg, GetParent(dlg));

		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDC_CONTINUE:
			// �F�ؒ��ɃT�[�o����ؒf���ꂽ�ꍇ�́A�L�����Z�������Ƃ���B(2014.3.31 yutaka)
			if (!pvar->cv->Ready) {
				goto canceled;
			}

			if (IsDlgButtonChecked(dlg, IDC_ADDTOKNOWNHOSTS)) {
				add_host_key(pvar);
			}

			/*
			 * known_hosts�_�C�A���O�̂��߂Ɉꎞ��~���Ă���
			 * SSH�T�[�o�Ƃ̃l�S�V�G�[�V�������ĊJ������B
			 */
			SSH_notify_host_OK(pvar);

			pvar->hosts_state.hosts_dialog = NULL;

			TTEndDialog(dlg, 1);
			return TRUE;

		case IDCANCEL:			/* kill the connection */
canceled:
			pvar->hosts_state.hosts_dialog = NULL;
			notify_closed_connection(pvar, "authentication cancelled");
			TTEndDialog(dlg, 0);
			return TRUE;

		case IDCLOSE:
			/*
			 * known_hosts���ɃT�[�o������l�b�g���[�N�ؒf���ꂽ�ꍇ�A
			 * �_�C�A���O�݂̂����B
			 */
			pvar->hosts_state.hosts_dialog = NULL;
			TTEndDialog(dlg, 0);
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
		pvar->hFontFixed = UTIL_get_lang_fixedfont(dlg, pvar->ts->UILanguageFileW);
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
// �u���������̊m�F�_�C�A���O�𕪗�
//
static INT_PTR CALLBACK hosts_replace_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
											   LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
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

		// �ǉ��E�u�������Ƃ� init_hosts_dlg ���Ă�ł���̂ŁA���̑O�ɃZ�b�g����K�v������
		SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_NOTFOUND:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_NOTFOUND", pvar, "No host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_AUTH_MATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MATCH", pvar, "Matching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MISMATCH", pvar, "Mismatching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		case DNS_VERIFY_DIFFERENTTYPE:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_DIFFTYPE", pvar, "Mismatching host key type found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		}

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_NG", pvar, "Found insecure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->UIMsg);
			break;
		case DNS_VERIFY_AUTH_MATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_OK", pvar, "Found secure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->UIMsg);
			break;
		}

		init_hosts_dlg(pvar, dlg);
		CenterWindow(dlg, GetParent(dlg));
		// �f�t�H���g�Ń`�F�b�N�͓���Ȃ�
		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDC_CONTINUE:
			// �F�ؒ��ɃT�[�o����ؒf���ꂽ�ꍇ�́A�L�����Z�������Ƃ���B(2014.3.31 yutaka)
			if (!pvar->cv->Ready) {
				goto canceled;
			}

			if (IsDlgButtonChecked(dlg, IDC_ADDTOKNOWNHOSTS)) {
				add_host_key(pvar);
				delete_different_key(pvar);
			}

			/*
			 * known_hosts�_�C�A���O�̂��߂Ɉꎞ��~���Ă���
			 * SSH�T�[�o�Ƃ̃l�S�V�G�[�V�������ĊJ������B
			 */
			SSH_notify_host_OK(pvar);

			pvar->hosts_state.hosts_dialog = NULL;

			TTEndDialog(dlg, 1);
			return TRUE;

		case IDCANCEL:			/* kill the connection */
canceled:
			pvar->hosts_state.hosts_dialog = NULL;
			notify_closed_connection(pvar, "authentication cancelled");
			TTEndDialog(dlg, 0);
			return TRUE;

		case IDCLOSE:
			/*
			 * known_hosts���ɃT�[�o������l�b�g���[�N�ؒf���ꂽ�ꍇ�A
			 * �_�C�A���O�݂̂����B
			 */
			pvar->hosts_state.hosts_dialog = NULL;
			TTEndDialog(dlg, 0);
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
		pvar->hFontFixed = UTIL_get_lang_fixedfont(dlg, pvar->ts->UILanguageFileW);
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
// �����z�X�g�Ō��`�����Ⴄ���̒ǉ��m�F�_�C�A���O�𕪗�
//
static INT_PTR CALLBACK hosts_add2_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
											LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
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

		// �ǉ��E�u�������Ƃ� init_hosts_dlg ���Ă�ł���̂ŁA���̑O�ɃZ�b�g����K�v������
		SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_NOTFOUND:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_NOTFOUND", pvar, "No host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_AUTH_MATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MATCH", pvar, "Matching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_MISMATCH", pvar, "Mismatching host key fingerprint found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		case DNS_VERIFY_DIFFERENTTYPE:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_SSHFP_DIFFTYPE", pvar, "Mismatching host key type found in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPCHECK, pvar->UIMsg);
			break;
		}

		switch (pvar->dns_key_check) {
		case DNS_VERIFY_MATCH:
		case DNS_VERIFY_MISMATCH:
		case DNS_VERIFY_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_NG", pvar, "Found insecure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->UIMsg);
			break;
		case DNS_VERIFY_AUTH_MATCH:
		case DNS_VERIFY_AUTH_MISMATCH:
		case DNS_VERIFY_AUTH_DIFFERENTTYPE:
			UTIL_get_lang_msg("DLG_HOSTKEY_DNSSEC_OK", pvar, "Found secure fingerprint in DNS.");
			SetDlgItemText(dlg, IDC_HOSTSSHFPDNSSEC, pvar->UIMsg);
			break;
		}

		init_hosts_dlg(pvar, dlg);
		CenterWindow(dlg, GetParent(dlg));
		// add host check box �̃f�t�H���g�� off �ɂ���
		// SendMessage(GetDlgItem(dlg, IDC_ADDTOKNOWNHOSTS), BM_SETCHECK, BST_CHECKED, 0);

		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDC_CONTINUE:
			// �F�ؒ��ɃT�[�o����ؒf���ꂽ�ꍇ�́A�L�����Z�������Ƃ���B(2014.3.31 yutaka)
			if (!pvar->cv->Ready) {
				goto canceled;
			}

			if (IsDlgButtonChecked(dlg, IDC_ADDTOKNOWNHOSTS)) {
				add_host_key(pvar);
			}

			/*
			 * known_hosts�_�C�A���O�̂��߂Ɉꎞ��~���Ă���
			 * SSH�T�[�o�Ƃ̃l�S�V�G�[�V�������ĊJ������B
			 */
			SSH_notify_host_OK(pvar);

			pvar->hosts_state.hosts_dialog = NULL;

			TTEndDialog(dlg, 1);
			return TRUE;

		case IDCANCEL:			/* kill the connection */
canceled:
			pvar->hosts_state.hosts_dialog = NULL;
			notify_closed_connection(pvar, "authentication cancelled");
			TTEndDialog(dlg, 0);
			return TRUE;

		case IDCLOSE:
			/*
			 * known_hosts���ɃT�[�o������l�b�g���[�N�ؒf���ꂽ�ꍇ�A
			 * �_�C�A���O�݂̂����B
			 */
			pvar->hosts_state.hosts_dialog = NULL;
			TTEndDialog(dlg, 0);
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
		pvar->hFontFixed = UTIL_get_lang_fixedfont(dlg, pvar->ts->UILanguageFileW);
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
		/* known_hosts�̓ǂݍ��ݎ��AID_SSHASYNCMESSAGEBOX ���g����
		 * MessageBox ���\�������ꍇ�A�I�[�i�[�Ȃ�(no owner)�ɂȂ邽�߁A
		 * MessageBox ��Tera Term�̗��ɉB��邱�Ƃ�����B
		 * ���̏�Ԃ� GetActiveWindow() ���Ăяo���ƁAknown_hosts�_�C�A���O��
		 * �I�[�i�[�� MessageBox �ƂȂ��āATera Term�̗��ɉB��Ă��܂��B
		 * �����ŁAknown_hosts�_�C�A���O�̃I�[�i�[�͏�� Tera Term ���w������
		 * �悤�ɂ���B
		 */
		HWND cur_active = NULL;

		TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SSHUNKNOWNHOST),
						 cur_active != NULL ? cur_active : wnd,
						 hosts_add_dlg_proc, (LPARAM) pvar);
	}
}

void HOSTS_do_different_key_dialog(HWND wnd, PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog == NULL) {
		/* known_hosts�̓ǂݍ��ݎ��AID_SSHASYNCMESSAGEBOX ���g����
		 * MessageBox ���\�������ꍇ�A�I�[�i�[�Ȃ�(no owner)�ɂȂ邽�߁A
		 * MessageBox ��Tera Term�̗��ɉB��邱�Ƃ�����B
		 * ���̏�Ԃ� GetActiveWindow() ���Ăяo���ƁAknown_hosts�_�C�A���O��
		 * �I�[�i�[�� MessageBox �ƂȂ��āATera Term�̗��ɉB��Ă��܂��B
		 * �����ŁAknown_hosts�_�C�A���O�̃I�[�i�[�͏�� Tera Term ���w������
		 * �悤�ɂ���B
		 */
		HWND cur_active = NULL;

		TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SSHDIFFERENTKEY),
						 cur_active != NULL ? cur_active : wnd,
						 hosts_replace_dlg_proc, (LPARAM) pvar);
	}
}

void HOSTS_do_different_type_key_dialog(HWND wnd, PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog == NULL) {
		/* known_hosts�̓ǂݍ��ݎ��AID_SSHASYNCMESSAGEBOX ���g����
		 * MessageBox ���\�������ꍇ�A�I�[�i�[�Ȃ�(no owner)�ɂȂ邽�߁A
		 * MessageBox ��Tera Term�̗��ɉB��邱�Ƃ�����B
		 * ���̏�Ԃ� GetActiveWindow() ���Ăяo���ƁAknown_hosts�_�C�A���O��
		 * �I�[�i�[�� MessageBox �ƂȂ��āATera Term�̗��ɉB��Ă��܂��B
		 * �����ŁAknown_hosts�_�C�A���O�̃I�[�i�[�͏�� Tera Term ���w������
		 * �悤�ɂ���B
		 */
		HWND cur_active = NULL;

		TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SSHDIFFERENTTYPEKEY),
						 cur_active != NULL ? cur_active : wnd,
						 hosts_add2_dlg_proc, (LPARAM) pvar);
	}
}

/*
 * �T�[�o���瑗���Ă����z�X�g���J���̑Ó������`�F�b�N���A
 * �K�v�ɉ����� known_hosts �_�C�A���O���Ăяo���B
 *
 *   hostname: �ڑ���̃z�X�g��
 *   tcpport: �ڑ���|�[�g�ԍ�
 *   key: �T�[�o����̌��J��
 *
 * return:
 *    TRUE:  known_hosts�_�C�A���O�̌Ăяo���͕s�v
 *    FALSE: known_hosts�_�C�A���O���Ăяo����
 *
 */
BOOL HOSTS_check_host_key(PTInstVar pvar, char *hostname, unsigned short tcpport, Key *key)
{
	int found_different_key = 0, found_different_type_key = 0;
	Key key2; // known_hosts�ɓo�^����Ă��錮
	DWORD id;

	pvar->dns_key_check = DNS_VERIFY_NONE;

	// ���ł� known_hosts �t�@�C������z�X�g���J����ǂݍ���ł���Ȃ�A����Ɣ�r����B
	if (pvar->hosts_state.prefetched_hostname != NULL
	 && _stricmp(pvar->hosts_state.prefetched_hostname, hostname) == 0
	 && HOSTS_compare_public_key(&pvar->hosts_state.hostkey, key) == 1) {

		 // ���������ɖ߂�B
		 return TRUE;
	}

	// ��ǂ݂���Ă��Ȃ��ꍇ�́A���̎��_�Ńt�@�C������ǂݍ���
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
					// ���ׂẴG���g�����Q�Ƃ��āA���v����L�[������������߂�B
					// About TTSSH �_�C�A���O�ł̕\���̂��߂ɁA�����ŕۑ����Ă����B
					key_copy(&pvar->hosts_state.hostkey, key);

					return TRUE;
				}
				else if (match == 0) {
					// �L�[�� known_hosts �Ɍ����������A�L�[�̓��e���قȂ�B
					found_different_key = 1;
				}
				else {
					// �L�[�̌`�����Ⴄ�ꍇ
					found_different_type_key = 1;
				}
			}
		} while (key2.type != KEY_UNSPEC);  // �L�[���������Ă���Ԃ̓��[�v����

		key_init(&key2);
		finish_read_host_files(pvar, 0);
	}

	// known_hosts �ɑ��݂��Ȃ��L�[�͂��ƂŃt�@�C���֏������ނ��߂ɁA�����ŕۑ����Ă����B
	key_copy(&pvar->hosts_state.hostkey, key);

	free(pvar->hosts_state.prefetched_hostname);
	pvar->hosts_state.prefetched_hostname = _strdup(hostname);

	// "/nosecuritywarning"���w�肳��Ă���ꍇ�A�_�C�A���O��\���������� return success ����B
	if (pvar->nocheck_known_hosts == TRUE) {
		 // ���������ɖ߂�B
		return TRUE;
	}

	if (pvar->settings.VerifyHostKeyDNS && !is_numeric_hostname(hostname)) {
		pvar->dns_key_check = verify_hostkey_dns(pvar, hostname, key);
	}

	// known_hosts�_�C�A���O�͓����I�ɕ\�������A���̎��_�ɂ����ă��[�U�Ɋm�F
	// ������K�v�����邽�߁A���ڃR�[���ɕύX����B
	// ����ɂ��known_hosts�̊m�F��҂����ɁA�T�[�o�փ��[�U���𑗂��Ă��܂������������B
	// (2007.10.1 yutaka)
	/*
	 * known_hosts�_�C�A���O�͔񓯊��ŕ\��������̂������������B
	 * known_hosts�_�C�A���O���\������Ă����ԂŁA�T�[�o����ؒf���s���ƁA
	 * TTXCloseTCP���Ăяo����ATTSSH�̃��\�[�X���������Ă��܂��B
	 * SSH�n���h���̉�����known_hosts�_�C�A���O���o���Ď~�܂��Ă��邽�߁A
	 * �����ĊJ��ɕs���A�N�Z�X�ŗ�����B
	 * (2019.9.3 yutaka)
	 */
	if (found_different_key) {
		// TTXProcessCommand ���� HOSTS_do_different_key_dialog() ���Ăяo���B
		id = ID_SSHDIFFERENTKEY;
	}
	else if (found_different_type_key) {
		// TTXProcessCommand ���� HOSTS_do_different_type_key_dialog() ���Ăяo���B
		id = ID_SSHDIFFERENT_TYPE_KEY;
	}
	else {
		// TTXProcessCommand ���� HOSTS_do_unknown_host_dialog() ���Ăяo���B
		id = ID_SSHUNKNOWNHOST;
	}

	PostMessage(pvar->NotificationWindow, WM_COMMAND, id, 0);

	logprintf(LOG_LEVEL_INFO, "Calling known_hosts dialog...(%s)",
		id == ID_SSHDIFFERENTKEY ? "SSHDIFFERENTKEY" :
		id == ID_SSHDIFFERENT_TYPE_KEY ? "SSHDIFFERENT_TYPE_KEY" : "SSHUNKNOWNHOST"
		);

	return FALSE;
}

void HOSTS_notify_disconnecting(PTInstVar pvar)
{
	if (pvar->hosts_state.hosts_dialog != NULL) {
		PostMessage(pvar->hosts_state.hosts_dialog, WM_COMMAND, IDCANCEL, 0);
		/* the main window might not go away if it's not enabled. (see vtwin.cpp) */
		EnableWindow(pvar->NotificationWindow, TRUE);
	}
}

// TCP�Z�b�V�������N���[�Y���ꂽ�ꍇ�Aknown_hosts�_�C�A���O�����悤�Ɏw�����o���B
// HOSTS_notify_disconnecting()�Ƃ͈قȂ�A�_�C�A���O�����݂̂ŁA
// SSH�T�[�o�ɒʒm�͏o���Ȃ��B
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
