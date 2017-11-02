/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (C) 2004-2017 TeraTerm Project
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

/* Tera Term extension mechanism
   Robert O'Callahan (roc+tt@cs.cmu.edu)

   Tera Term by Takashi Teranishi (teranishi@rikaxp.riken.go.jp)
*/

#include "ttxssh.h"
#include "fwdui.h"
#include "util.h"
#include "ssh.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "keyfiles.h"
#include "arc4random.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <mbstring.h>

#include "resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <winsock2.h>
static char *ProtocolFamilyList[] = { "UNSPEC", "IPv6", "IPv4", NULL };

#include <Lmcons.h>

// include OpenSSL header file
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rc4.h>
#include <openssl/md5.h>

// include ZLib header file
#include <zlib.h>

#include "buffer.h"
#include "cipher.h"
#include "key.h"

#include "sftp.h"

#include "compat_w95.h"

#include "libputty.h"

#define MATCH_STR(s, o) strncmp((s), (o), NUM_ELEM(o) - 1)
#define MATCH_STR_I(s, o) _strnicmp((s), (o), NUM_ELEM(o) - 1)

/* This extension implements SSH, so we choose a load order in the
   "protocols" range. */
#define ORDER 2500

static HICON SecureLargeIcon = NULL;
static HICON SecureSmallIcon = NULL;
static HICON SecureNotifyIcon = NULL;
static HICON OldNotifyIcon = NULL;

static HFONT DlgHostFont;
static HFONT DlgAboutFont;
static HFONT DlgAboutTextFont;
static HFONT DlgSetupFont;
static HFONT DlgKeygenFont;

static TInstVar *pvar;

typedef struct {
	int cnt;
	HWND dlg;
	ssh_keytype type;
} cbarg_t;

  /* WIN32 allows multiple instances of a DLL */
static TInstVar InstVar;

/*
This code makes lots of assumptions about the order in which Tera Term
does things, and how. A key assumption is that the Notification window
passed into WSAAsyncSelect is the main terminal window. We also assume
that the socket used in the first WSAconnect is the main session socket.
*/

static void init_TTSSH(PTInstVar pvar)
{
	pvar->socket = INVALID_SOCKET;
	pvar->OldLargeIcon = NULL;
	pvar->OldSmallIcon = NULL;
	pvar->NotificationWindow = NULL;
	pvar->hostdlg_activated = FALSE;
	pvar->socket = INVALID_SOCKET;
	pvar->NotificationWindow = NULL;
	pvar->protocol_major = 0;
	pvar->protocol_minor = 0;

	PKT_init(pvar);
	SSH_init(pvar);
	CRYPT_init(pvar);
	AUTH_init(pvar);
	HOSTS_init(pvar);
	FWD_init(pvar);
	FWDUI_init(pvar);

	ssh_heartbeat_lock_initialize();
}

static void uninit_TTSSH(PTInstVar pvar)
{
	halt_ssh_heartbeat_thread(pvar);

	ssh2_channel_free();

	SSH_end(pvar);
	PKT_end(pvar);
	AUTH_end(pvar);
	CRYPT_end(pvar);
	HOSTS_end(pvar);
	FWD_end(pvar);
	FWDUI_end(pvar);

	if (pvar->OldLargeIcon != NULL) {
		PostMessage(pvar->NotificationWindow, WM_SETICON, ICON_BIG,
		            (LPARAM) pvar->OldLargeIcon);
		pvar->OldLargeIcon = NULL;
	}
	if (pvar->OldSmallIcon != NULL) {
		PostMessage(pvar->NotificationWindow, WM_SETICON, ICON_SMALL,
		            (LPARAM) pvar->OldSmallIcon);
		pvar->OldSmallIcon = NULL;
	}
	if (OldNotifyIcon) {
		SetCustomNotifyIcon(OldNotifyIcon);
	}

	ssh_heartbeat_lock_finalize();
}

static void PASCAL TTXInit(PTTSet ts, PComVar cv)
{
	pvar->settings = *pvar->ts_SSH;
	pvar->ts = ts;
	pvar->cv = cv;
	pvar->fatal_error = FALSE;
	pvar->showing_err = FALSE;
	pvar->err_msg = NULL;

	init_TTSSH(pvar);
}

static void normalize_generic_order(char *buf, char default_strings[], int default_strings_len)
{
	char listed[max(KEX_DH_MAX,max(SSH_CIPHER_MAX,max(KEY_MAX,max(HMAC_MAX,COMP_MAX)))) + 1];
	char allowed[max(KEX_DH_MAX,max(SSH_CIPHER_MAX,max(KEY_MAX,max(HMAC_MAX,COMP_MAX)))) + 1];
	int i, j, k=-1;

	memset(listed, 0, sizeof(listed));
	memset(allowed, 0, sizeof(allowed));

	// 許可されている文字のリストを作る。
	for (i = 0; i < default_strings_len ; i++) {
		allowed[default_strings[i]] = 1;
	}

	// 指定された文字列を走査し、許可されていない文字、重複する文字は削除する。
	// 
	// ex. (i=5 の文字を削除する)
	// i=012345
	//   >:=9<87;A@?B3026(\0)
	//         i+1
	//         <------------>
	//       ↓
	//   >:=9<7;A@?B3026(\0)
	//         
	for (i = 0; buf[i] != 0; i++) {
		int num = buf[i] - '0';

		if (num < 0 || num > default_strings_len
			|| !allowed[num]
			|| listed[num]) {
			memmove(buf + i, buf + i + 1, strlen(buf + i + 1) + 1);
			i--;
		} else {
			listed[num] = 1;
		}

		// disabled lineがあれば、位置を覚えておく。
		if (num == 0) {
			k = i;
		}
	}

	// 指定されていない文字があれば、disabled lineの直前に挿入する。
	// 
	// ex. (Zを挿入する)
	//                k
	//   >:=9<87;A@?B3026(\0)
	//                 k+1
	//                 <---->
	//       ↓       k
	//   >:=9<87;A@?B30026(\0)
	//       ↓        k
	//   >:=9<87;A@?B3Z026(\0)
	//       
	for (j = 0; j < default_strings_len && default_strings[j] != 0; j++) {
		int num = default_strings[j];

		if (!listed[num] && k >= 0) {
			int copylen = strlen(buf + k + 1) + 1;

			memmove(buf + k + 1, buf + k, copylen);
			buf[k + 1 + copylen] = '\0';   // 終端を忘れずに付ける。
			buf[k] = num + '0';
			k++;
			i++;
		}
	}
	if (k < 0) {
		j = 0;
	}
	else {
		j++;
	}

	// disabled lineが存在しない場合は、そのまま末尾に追加する。
	for (; j < default_strings_len ; j++) {
		int num = default_strings[j];

		if (!listed[num]) {
			buf[i] = num + '0';
			i++;
		}
	}

	buf[i] = 0;
}

/*
 * Remove unsupported cipher or duplicated cipher.
 * Add unspecified ciphers at the end of list.
 */
static void normalize_cipher_order(char *buf)
{
	/* SSH_CIPHER_NONE means that all ciphers below that one are disabled.
	   We *never* allow no encryption. */
	static char default_strings[] = {
		SSH2_CIPHER_CAMELLIA256_CTR,
		SSH2_CIPHER_AES256_CTR,
		SSH2_CIPHER_CAMELLIA256_CBC,
		SSH2_CIPHER_AES256_CBC,
		SSH2_CIPHER_CAMELLIA192_CTR,
		SSH2_CIPHER_AES192_CTR,
		SSH2_CIPHER_CAMELLIA192_CBC,
		SSH2_CIPHER_AES192_CBC,
		SSH2_CIPHER_CAMELLIA128_CTR,
		SSH2_CIPHER_AES128_CTR,
		SSH2_CIPHER_CAMELLIA128_CBC,
		SSH2_CIPHER_AES128_CBC,
		SSH2_CIPHER_3DES_CTR,
		SSH2_CIPHER_3DES_CBC,
		SSH2_CIPHER_BLOWFISH_CTR,
		SSH2_CIPHER_BLOWFISH_CBC,
		SSH2_CIPHER_CAST128_CTR,
		SSH2_CIPHER_CAST128_CBC,
		SSH_CIPHER_3DES,
		SSH_CIPHER_NONE,
		SSH2_CIPHER_ARCFOUR256,
		SSH2_CIPHER_ARCFOUR128,
		SSH2_CIPHER_ARCFOUR,
		SSH_CIPHER_BLOWFISH,
		SSH_CIPHER_DES,
		0, 0, 0 // Dummy for SSH_CIPHER_IDEA, SSH_CIPHER_TSS, SSH_CIPHER_RC4
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}

static void normalize_kex_order(char *buf)
{
	static char default_strings[] = {
		KEX_ECDH_SHA2_256,
		KEX_ECDH_SHA2_384,
		KEX_ECDH_SHA2_521,
		KEX_DH_GRP18_SHA512,
		KEX_DH_GRP16_SHA512,
		KEX_DH_GRP14_SHA256,
		KEX_DH_GEX_SHA256,
		KEX_DH_GEX_SHA1,
		KEX_DH_GRP14_SHA1,
		KEX_DH_GRP1_SHA1,
		KEX_DH_NONE,
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}

static void normalize_host_key_order(char *buf)
{
	static char default_strings[] = {
		KEY_ECDSA256,
		KEY_ECDSA384,
		KEY_ECDSA521,
		KEY_ED25519,
		KEY_RSA,
		KEY_DSA,
		KEY_NONE,
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}

static void normalize_mac_order(char *buf)
{
	static char default_strings[] = {
		HMAC_SHA2_512_EtM,
		HMAC_SHA2_256_EtM,
		HMAC_SHA1_EtM,
		HMAC_SHA2_512,
		HMAC_SHA2_256,
		HMAC_SHA1,
		HMAC_RIPEMD160_EtM,
		HMAC_RIPEMD160,
		HMAC_MD5_EtM,
		HMAC_MD5,
		HMAC_NONE,
		HMAC_SHA1_96_EtM,
		HMAC_MD5_96_EtM,
		HMAC_SHA1_96,
		HMAC_MD5_96,
		0, // Dummy for HMAC_SHA2_512_96,
		0, // Dummy for HMAC_SHA2_256_96,
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}

static void normalize_comp_order(char *buf)
{
	static char default_strings[] = {
		COMP_DELAYED,
		COMP_ZLIB,
		COMP_NOCOMP,
		COMP_NONE,
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}


/* Remove local settings from the shared memory block. */
static void clear_local_settings(PTInstVar pvar)
{
	pvar->ts_SSH->TryDefaultAuth = FALSE;
}

static BOOL read_BOOL_option(PCHAR fileName, char *keyName, BOOL def)
{
	char buf[1024];

	buf[0] = 0;
	GetPrivateProfileString("TTSSH", keyName, "", buf, sizeof(buf),
	                        fileName);
	if (buf[0] == 0) {
		return def;
	} else {
		return atoi(buf) != 0 ||
		       _stricmp(buf, "yes") == 0 ||
		       _stricmp(buf, "y") == 0;
	}
}

static void read_string_option(PCHAR fileName, char *keyName,
                               char *def, char *buf, int bufSize)
{

	buf[0] = 0;
	GetPrivateProfileString("TTSSH", keyName, def, buf, bufSize, fileName);
}

static void read_ssh_options(PTInstVar pvar, PCHAR fileName)
{
	char buf[1024];
	TS_SSH *settings = pvar->ts_SSH;

#define READ_STD_STRING_OPTION(name) \
	read_string_option(fileName, #name, "", settings->name, sizeof(settings->name))

	settings->Enabled = read_BOOL_option(fileName, "Enabled", FALSE);

	buf[0] = 0;
	GetPrivateProfileString("TTSSH", "Compression", "", buf, sizeof(buf),
	                        fileName);
	settings->CompressionLevel = atoi(buf);
	if (settings->CompressionLevel < 0 || settings->CompressionLevel > 9) {
		settings->CompressionLevel = 0;
	}

	READ_STD_STRING_OPTION(DefaultUserName);
	READ_STD_STRING_OPTION(DefaultForwarding);
	READ_STD_STRING_OPTION(DefaultRhostsLocalUserName);
	READ_STD_STRING_OPTION(DefaultRhostsHostPrivateKeyFile);
	READ_STD_STRING_OPTION(DefaultRSAPrivateKeyFile);

	READ_STD_STRING_OPTION(CipherOrder);
	normalize_cipher_order(settings->CipherOrder);

	// KEX order
	READ_STD_STRING_OPTION(KexOrder);
	normalize_kex_order(settings->KexOrder);
	// Host Key algorithm order
	READ_STD_STRING_OPTION(HostKeyOrder);
	normalize_host_key_order(settings->HostKeyOrder);
	// HMAC order
	READ_STD_STRING_OPTION(MacOrder);
	normalize_mac_order(settings->MacOrder);
	// Compression algorithm order
	READ_STD_STRING_OPTION(CompOrder);
	normalize_comp_order(settings->CompOrder);

	read_string_option(fileName, "KnownHostsFiles", "ssh_known_hosts",
	                   settings->KnownHostsFiles,
	                   sizeof(settings->KnownHostsFiles));

	buf[0] = 0;
	GetPrivateProfileString("TTSSH", "DefaultAuthMethod", "", buf,
	                        sizeof(buf), fileName);
	settings->DefaultAuthMethod = atoi(buf);
	if (settings->DefaultAuthMethod != SSH_AUTH_PASSWORD
	 && settings->DefaultAuthMethod != SSH_AUTH_RSA
	 && settings->DefaultAuthMethod != SSH_AUTH_TIS  // add (2005.3.12 yutaka)
	 && settings->DefaultAuthMethod != SSH_AUTH_RHOSTS
	 && settings->DefaultAuthMethod != SSH_AUTH_PAGEANT) {
		/* this default can never be SSH_AUTH_RHOSTS_RSA because that is not a
		   selection in the dialog box; SSH_AUTH_RHOSTS_RSA is automatically chosen
		   when the dialog box has rhosts selected and an host private key file
		   is supplied. */
		settings->DefaultAuthMethod = SSH_AUTH_PASSWORD;
	}

	buf[0] = 0;
	GetPrivateProfileString("TTSSH", "LogLevel", "", buf, sizeof(buf),
	                        fileName);
	settings->LogLevel = atoi(buf);

	buf[0] = 0;
	GetPrivateProfileString("TTSSH", "WriteBufferSize", "", buf,
	                        sizeof(buf), fileName);
	settings->WriteBufferSize = atoi(buf);
	if (settings->WriteBufferSize <= 0) {
		settings->WriteBufferSize = (PACKET_MAX_SIZE / 2);   // 2MB
	}

	// SSH protocol version (2004.10.11 yutaka)
	// default is SSH2 (2004.11.30 yutaka)
	settings->ssh_protocol_version = GetPrivateProfileInt("TTSSH", "ProtocolVersion", 2, fileName);

	// SSH heartbeat time(second) (2004.12.11 yutaka)
	settings->ssh_heartbeat_overtime = GetPrivateProfileInt("TTSSH", "HeartBeat", 60, fileName);

	// パスワード認証および公開鍵認証に使うパスワードをメモリ上に保持しておくかどうかを
	// 表す。(2006.8.5 yutaka)
	settings->remember_password = GetPrivateProfileInt("TTSSH", "RememberPassword", 1, fileName);

	// 初回の認証ダイアログでサポートされているメソッドをチェックし、
	// 無効なメソッドをグレイアウトする (2007.9.24 maya)
	settings->CheckAuthListFirst = read_BOOL_option(fileName, "CheckAuthListFirst", FALSE);

	// 768bit 未満の RSA 鍵を持つサーバへの接続を有効にする (2008.9.11 maya)
	settings->EnableRsaShortKeyServer = read_BOOL_option(fileName, "EnableRsaShortKeyServer", FALSE);

	// agent forward を有効にする (2008.11.25 maya)
	settings->ForwardAgent = read_BOOL_option(fileName, "ForwardAgent", FALSE);

	// agent forward 確認を有効にする
	settings->ForwardAgentConfirm = read_BOOL_option(fileName, "ForwardAgentConfirm", TRUE);

	// agent forward 確認を有効にする
	settings->ForwardAgentNotify = read_BOOL_option(fileName, "ForwardAgentNotify", TRUE);

	// ホスト鍵の DNS でのチェック (RFC 4255)
	settings->VerifyHostKeyDNS = read_BOOL_option(fileName, "VerifyHostKeyDNS", TRUE);

	// icon
	GetPrivateProfileString("TTSSH", "SSHIcon", "", buf, sizeof(buf), fileName);
	if ((_stricmp(buf, "old") == 0) ||
	    (_stricmp(buf, "yellow") == 0) ||
	    (_stricmp(buf, "securett_yellow") == 0)) {
		settings->IconID = IDI_SECURETT_YELLOW;
	}
	else if ((_stricmp(buf, "green") == 0) ||
	         (_stricmp(buf, "securett_green") == 0)) {
		settings->IconID = IDI_SECURETT_GREEN;
	}
	else {
		settings->IconID = IDI_SECURETT;
	}

	// エラーおよび警告時のポップアップメッセージを抑止する (2014.6.26 yutaka)
	settings->DisablePopupMessage = GetPrivateProfileInt("TTSSH", "DisablePopupMessage", 0, fileName);

	READ_STD_STRING_OPTION(X11Display);

	settings->UpdateHostkeys = GetPrivateProfileInt("TTSSH", "UpdateHostkeys", 0, fileName);

	settings->GexMinimalGroupSize = GetPrivateProfileInt("TTSSH", "GexMinimalGroupSize", 0, fileName);

	clear_local_settings(pvar);
}

static void write_ssh_options(PTInstVar pvar, PCHAR fileName,
                              TS_SSH *settings, BOOL copy_forward)
{
	char buf[1024];

	WritePrivateProfileString("TTSSH", "Enabled",
	                          settings->Enabled ? "1" : "0", fileName);

	_itoa(settings->CompressionLevel, buf, 10);
	WritePrivateProfileString("TTSSH", "Compression", buf, fileName);

	WritePrivateProfileString("TTSSH", "DefaultUserName",
	                          settings->DefaultUserName, fileName);

	if (copy_forward) {
		WritePrivateProfileString("TTSSH", "DefaultForwarding",
		                          settings->DefaultForwarding, fileName);
	}

	WritePrivateProfileString("TTSSH", "CipherOrder",
	                          settings->CipherOrder, fileName);

	WritePrivateProfileString("TTSSH", "KexOrder",
	                          settings->KexOrder, fileName);

	WritePrivateProfileString("TTSSH", "HostKeyOrder",
	                          settings->HostKeyOrder, fileName);

	WritePrivateProfileString("TTSSH", "MacOrder",
	                          settings->MacOrder, fileName);

	WritePrivateProfileString("TTSSH", "CompOrder",
	                          settings->CompOrder, fileName);

	WritePrivateProfileString("TTSSH", "KnownHostsFiles",
	                          settings->KnownHostsFiles, fileName);

	WritePrivateProfileString("TTSSH", "DefaultRhostsLocalUserName",
	                          settings->DefaultRhostsLocalUserName,
	                          fileName);

	WritePrivateProfileString("TTSSH", "DefaultRhostsHostPrivateKeyFile",
	                          settings->DefaultRhostsHostPrivateKeyFile,
	                          fileName);

	WritePrivateProfileString("TTSSH", "DefaultRSAPrivateKeyFile",
	                          settings->DefaultRSAPrivateKeyFile,
	                          fileName);

	_itoa(settings->DefaultAuthMethod, buf, 10);
	WritePrivateProfileString("TTSSH", "DefaultAuthMethod", buf, fileName);

	_itoa(settings->LogLevel, buf, 10);
	WritePrivateProfileString("TTSSH", "LogLevel", buf, fileName);

	_itoa(settings->WriteBufferSize, buf, 10);
	WritePrivateProfileString("TTSSH", "WriteBufferSize", buf, fileName);

	// SSH protocol version (2004.10.11 yutaka)
	WritePrivateProfileString("TTSSH", "ProtocolVersion",
	    settings->ssh_protocol_version==2 ? "2" : "1",
	    fileName);

	// SSH heartbeat time(second) (2004.12.11 yutaka)
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
	            "%d", settings->ssh_heartbeat_overtime);
	WritePrivateProfileString("TTSSH", "HeartBeat", buf, fileName);

	// Remember password (2006.8.5 yutaka)
	WritePrivateProfileString("TTSSH", "RememberPassword",
	    settings->remember_password ? "1" : "0",
	    fileName);

	// 初回の認証ダイアログでサポートされているメソッドをチェックし、
	// 無効なメソッドをグレイアウトする (2007.9.24 maya)
	WritePrivateProfileString("TTSSH", "CheckAuthListFirst",
	                          settings->CheckAuthListFirst ? "1" : "0", fileName);

	// 768bit 未満の RSA 鍵を持つサーバへの接続を有効にする (2008.9.11 maya)
	WritePrivateProfileString("TTSSH", "EnableRsaShortKeyServer",
	                          settings->EnableRsaShortKeyServer ? "1" : "0", fileName);

	// agent forward を有効にする (2008.11.25 maya)
	WritePrivateProfileString("TTSSH", "ForwardAgent",
	                          settings->ForwardAgent ? "1" : "0", fileName);

	// agent forward 確認を有効にする
	WritePrivateProfileString("TTSSH", "ForwardAgentConfirm",
	                          settings->ForwardAgentConfirm ? "1" : "0", fileName);

	// agent forward 通知を有効にする
	WritePrivateProfileString("TTSSH", "ForwardAgentNotify",
	                          settings->ForwardAgentNotify ? "1" : "0", fileName);

	// ホスト鍵の DNS でのチェック (RFC 4255)
	WritePrivateProfileString("TTSSH", "VerifyHostKeyDNS",
	                          settings->VerifyHostKeyDNS ? "1" : "0", fileName);

	// SSH アイコン
	if (settings->IconID==IDI_SECURETT_YELLOW) {
		WritePrivateProfileString("TTSSH", "SSHIcon", "yellow", fileName);
	}
	else if (settings->IconID==IDI_SECURETT_GREEN) {
		WritePrivateProfileString("TTSSH", "SSHIcon", "green", fileName);
	}
	else {
		WritePrivateProfileString("TTSSH", "SSHIcon", "Default", fileName);
	}

	_itoa(settings->DisablePopupMessage, buf, 10);
	WritePrivateProfileString("TTSSH", "DisablePopupMessage", buf, fileName);

	WritePrivateProfileString("TTSSH", "X11Display", settings->X11Display, fileName);

	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"%d", settings->UpdateHostkeys);
	WritePrivateProfileString("TTSSH", "UpdateHostkeys", buf, fileName);

	_itoa_s(settings->GexMinimalGroupSize, buf, sizeof(buf), 10);
	WritePrivateProfileString("TTSSH", "GexMinimalGroupSize", buf, fileName);
}


/* find free port in all protocol family */
static unsigned short find_local_port(PTInstVar pvar)
{
	int tries;
	SOCKET connecter;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *res0;
	unsigned short port;
	char pname[NI_MAXHOST];

	if (pvar->session_settings.DefaultAuthMethod != SSH_AUTH_RHOSTS) {
		return 0;
	}

	/* The random numbers here are only used to try to get fresh
	   ports across runs (dangling ports can cause bind errors
	   if we're unlucky). They do not need to be (and are not)
	   cryptographically strong.
	 */
	srand((unsigned) GetTickCount());

	for (tries = 20; tries > 0; tries--) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = pvar->ts->ProtocolFamily;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;
		port = (unsigned) rand() % 512 + 512;
		_snprintf_s(pname, sizeof(pname), _TRUNCATE, "%d", (int) port);
		if (getaddrinfo(NULL, pname, &hints, &res0)) {
			return 0;
			/* NOT REACHED */
		}

		for (res = res0; res; res = res->ai_next) {
			if (res->ai_family == AF_INET || res->ai_family == AF_INET6)
				continue;

			connecter =
				socket(res->ai_family, res->ai_socktype, res->ai_protocol);
			if (connecter == INVALID_SOCKET) {
				freeaddrinfo(res0);
				return 0;
			}

			if (bind(connecter, res->ai_addr, res->ai_addrlen) !=
			    SOCKET_ERROR) {
				return port;
				freeaddrinfo(res0);
				closesocket(connecter);
			} else if (WSAGetLastError() != WSAEADDRINUSE) {
				closesocket(connecter);
				freeaddrinfo(res0);
				return 0;
			}

			closesocket(connecter);
		}
		freeaddrinfo(res0);
	}

	return 0;
}

static int PASCAL TTXconnect(SOCKET s,
                                 const struct sockaddr *name,
                                 int namelen)
{
	if (pvar->socket == INVALID_SOCKET || pvar->socket != s) {
		struct sockaddr_storage ss;
		int len;

		pvar->socket = s;

		memset(&ss, 0, sizeof(ss));
		switch (pvar->ts->ProtocolFamily) {
		case AF_INET:
			len = sizeof(struct sockaddr_in);
			((struct sockaddr_in *) &ss)->sin_family = AF_INET;
			((struct sockaddr_in *) &ss)->sin_addr.s_addr = INADDR_ANY;
			((struct sockaddr_in *) &ss)->sin_port =
				htons(find_local_port(pvar));
			break;
		case AF_INET6:
			len = sizeof(struct sockaddr_in6);
			((struct sockaddr_in6 *) &ss)->sin6_family = AF_INET6;
			memset(&((struct sockaddr_in6 *) &ss)->sin6_addr, 0,
			       sizeof(struct in_addr6));
			((struct sockaddr_in6 *) &ss)->sin6_port =
				htons(find_local_port(pvar));
			break;
		default:
			/* UNSPEC */
			break;
		}

		bind(s, (struct sockaddr *) &ss, len);
	}

	return (pvar->Pconnect) (s, name, namelen);
}

static int PASCAL TTXWSAAsyncSelect(SOCKET s, HWND hWnd, u_int wMsg,
                                        long lEvent)
{
	if (s == pvar->socket) {
		pvar->notification_events = lEvent;
		pvar->notification_msg = wMsg;

		if (pvar->NotificationWindow == NULL) {
			pvar->NotificationWindow = hWnd;
			AUTH_advance_to_next_cred(pvar);
		}
	}

	return (pvar->PWSAAsyncSelect) (s, hWnd, wMsg, lEvent);
}

static int PASCAL TTXrecv(SOCKET s, char *buf, int len, int flags)
{
	if (s == pvar->socket) {
		int ret;

		ssh_heartbeat_lock();
		ret = PKT_recv(pvar, buf, len);
		ssh_heartbeat_unlock();
		return (ret);

	} else {
		return (pvar->Precv) (s, buf, len, flags);
	}
}

static int PASCAL TTXsend(SOCKET s, char const *buf, int len,
                              int flags)
{
	if (s == pvar->socket) {
		ssh_heartbeat_lock();
		SSH_send(pvar, buf, len);
		ssh_heartbeat_unlock();
		return len;
	} else {
		return (pvar->Psend) (s, buf, len, flags);
	}
}

void notify_established_secure_connection(PTInstVar pvar)
{
	int fuLoad = LR_DEFAULTCOLOR;

	if (IsWindowsNT4()) {
		fuLoad = LR_VGACOLOR;
	}

	// LoadIcon ではなく LoadImage を使うようにし、
	// 16x16 のアイコンを明示的に取得するようにした (2006.8.9 maya)
	if (SecureLargeIcon == NULL) {
		SecureLargeIcon = LoadImage(hInst, MAKEINTRESOURCE(pvar->settings.IconID),
		                            IMAGE_ICON, 0, 0, fuLoad);
	}
	if (SecureSmallIcon == NULL) {
		SecureSmallIcon = LoadImage(hInst, MAKEINTRESOURCE(pvar->settings.IconID),
		                            IMAGE_ICON, 16, 16, fuLoad);
	}

	if (SecureLargeIcon != NULL && SecureSmallIcon != NULL) {
		pvar->OldLargeIcon =
			(HICON) SendMessage(pvar->NotificationWindow, WM_GETICON,
			                    ICON_BIG, 0);
		pvar->OldSmallIcon =
			(HICON) SendMessage(pvar->NotificationWindow, WM_GETICON,
			                    ICON_SMALL, 0);

		PostMessage(pvar->NotificationWindow, WM_SETICON, ICON_BIG,
		            (LPARAM) SecureLargeIcon);
		PostMessage(pvar->NotificationWindow, WM_SETICON, ICON_SMALL,
		            (LPARAM) SecureSmallIcon);
	}

	if (IsWindows2000()) {
		if (SecureNotifyIcon == NULL) {
			SecureNotifyIcon = LoadImage(hInst, MAKEINTRESOURCE(pvar->settings.IconID),
			                             IMAGE_ICON, 0, 0, LR_VGACOLOR | LR_SHARED);
		}
		OldNotifyIcon = GetCustomNotifyIcon();
		SetCustomNotifyIcon(SecureNotifyIcon);
	}

	logputs(LOG_LEVEL_VERBOSE, "Entering secure mode");
}

void notify_closed_connection(PTInstVar pvar, char *send_msg)
{
	SSH_notify_disconnecting(pvar, send_msg);
	AUTH_notify_disconnecting(pvar);
	HOSTS_notify_disconnecting(pvar);

	PostMessage(pvar->NotificationWindow, WM_USER_COMMNOTIFY,
	            pvar->socket, MAKELPARAM(FD_CLOSE, 0));
}

static void add_err_msg(PTInstVar pvar, char *msg)
{
	if (pvar->err_msg != NULL) {
		int buf_len = strlen(pvar->err_msg) + 3 + strlen(msg);
		char *buf = (char *) malloc(buf_len);

		strncpy_s(buf, buf_len, pvar->err_msg, _TRUNCATE);
		strncat_s(buf, buf_len, "\n\n", _TRUNCATE);
		strncat_s(buf, buf_len, msg, _TRUNCATE);
		free(pvar->err_msg);
		pvar->err_msg = buf;
	} else {
		pvar->err_msg = _strdup(msg);
	}
}

void notify_nonfatal_error(PTInstVar pvar, char *msg)
{
	if (!pvar->showing_err) {
		// 未接続の状態では通知先ウィンドウがないので、デスクトップをオーナーとして
		// メッセージボックスを出現させる。(2006.6.11 yutaka)
		if (pvar->NotificationWindow == NULL) {
			UTIL_get_lang_msg("MSG_NONFATAL_ERROR", pvar,
			                  "Tera Term: not fatal error");
			MessageBox(NULL, msg, pvar->ts->UIMsg, MB_OK|MB_ICONINFORMATION);
			msg[0] = '\0';

		} else {
			PostMessage(pvar->NotificationWindow, WM_COMMAND,
			            ID_SSHASYNCMESSAGEBOX, 0);
		}
	}
	if (msg[0] != 0) {
		logputs(LOG_LEVEL_ERROR, msg);
		add_err_msg(pvar, msg);
	}
}

void notify_fatal_error(PTInstVar pvar, char *msg, BOOL send_disconnect)
{
	if (msg[0] != 0) {
		logputs(LOG_LEVEL_FATAL, msg);
		add_err_msg(pvar, msg);
	}

	if (!pvar->fatal_error) {
		pvar->fatal_error = TRUE;

		if (send_disconnect) {
			SSH_notify_disconnecting(pvar, msg);
		}
		AUTH_notify_disconnecting(pvar);
		HOSTS_notify_disconnecting(pvar);

		PostMessage(pvar->NotificationWindow, WM_USER_COMMNOTIFY,
		            pvar->socket, MAKELPARAM(FD_CLOSE,
		                                     (pvar->PWSAGetLastError) ()));
	}
}

void logputs(int level, char *msg)
{
	if (level <= pvar->settings.LogLevel) {
		char buf[1024];
		int file;

		get_teraterm_dir_relative_name(buf, NUM_ELEM(buf), "TTSSH.LOG");
		file = _open(buf, _O_RDWR | _O_APPEND | _O_CREAT | _O_TEXT,
		             _S_IREAD | _S_IWRITE);

		if (file >= 0) {
			char *strtime = mctimelocal("%Y-%m-%d %H:%M:%S.%NZ", TRUE);
			DWORD processid;
			char tmp[26];

			_write(file, strtime, strlen(strtime));
			GetWindowThreadProcessId(pvar->cv->HWin, &processid);
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, " [%lu] ",processid);
			_write(file, tmp, strlen(tmp));
			_write(file, msg, strlen(msg));
			_write(file, "\n", 1);
			_close(file);
		}
	}
}

void logprintf(int level, char *fmt, ...)
{
	char buff[4096];
	va_list params;

	if (level <= pvar->settings.LogLevel) {
		va_start(params, fmt);
		vsnprintf_s(buff, sizeof(buff), _TRUNCATE, fmt, params);
		va_end(params);

		logputs(level, buff);
	}
}

static void format_line_hexdump(char *buf, int buflen, int addr, int *bytes, int byte_cnt)
{
	int i, c;
	char tmp[128];

	buf[0] = 0;

	/* 先頭のアドレス表示 */
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%08X : ", addr);
	strncat_s(buf, buflen, tmp, _TRUNCATE);

	/* バイナリ表示（4バイトごとに空白を挿入）*/
	for (i = 0; i < byte_cnt; i++) {
		if (i > 0 && i % 4 == 0) {
			strncat_s(buf, buflen, " ", _TRUNCATE);
		}

		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02X", bytes[i]);
		strncat_s(buf, buflen, tmp, _TRUNCATE);
	}

	/* ASCII表示部分までの空白を補う */
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "   %*s%*s", (16 - byte_cnt) * 2 + 1, " ", (16 - byte_cnt + 3) / 4, " ");
	strncat_s(buf, buflen, tmp, _TRUNCATE);

	/* ASCII表示 */
	for (i = 0; i < byte_cnt; i++) {
		c = bytes[i];
		if (isprint(c)) {
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%c", c);
			strncat_s(buf, buflen, tmp, _TRUNCATE);
		}
		else {
			strncat_s(buf, buflen, ".", _TRUNCATE);
		}
	}

	//strncat_s(buf, buflen, "\n", _TRUNCATE);
}

void logprintf_hexdump(int level, char *data, int len, char *fmt, ...)
{
	char buff[4096];
	va_list params;
	int c, addr;
	int bytes[16], *ptr;
	int byte_cnt;
	int i;

	if (level <= pvar->settings.LogLevel) {
		va_start(params, fmt);
		vsnprintf_s(buff, sizeof(buff), _TRUNCATE, fmt, params);
		va_end(params);

		logputs(level, buff);

		addr = 0;
		byte_cnt = 0;
		ptr = bytes;
		for (i = 0; i < len; i++) {
			c = data[i];
			*ptr++ = c & 0xff;
			byte_cnt++;

			if (byte_cnt == 16) {
				format_line_hexdump(buff, sizeof(buff), addr, bytes, byte_cnt);
				logputs(level, buff);

				addr += 16;
				byte_cnt = 0;
				ptr = bytes;
			}
		}

		if (byte_cnt > 0) {
			format_line_hexdump(buff, sizeof(buff), addr, bytes, byte_cnt);
			logputs(level, buff);
		}
	}
}

static void PASCAL TTXOpenTCP(TTXSockHooks *hooks)
{
	if (pvar->settings.Enabled) {
		// TCPLocalEcho/TCPCRSend を無効にする (maya 2007.4.25)
		pvar->origDisableTCPEchoCR = pvar->ts->DisableTCPEchoCR;
		pvar->ts->DisableTCPEchoCR = TRUE;

		pvar->session_settings = pvar->settings;

		logputs(LOG_LEVEL_VERBOSE, "---------------------------------------------------------------------");
		logputs(LOG_LEVEL_VERBOSE, "Initiating SSH session");

		FWDUI_load_settings(pvar);

		pvar->cv->TelAutoDetect = FALSE;
		/* This next line should not be needed because Tera Term's
		   CommLib should find ts->Telnet == 0 ... but we'll do this
		   just to be on the safe side. */
		pvar->cv->TelFlag = FALSE;
		pvar->cv->TelLineMode = FALSE;

		pvar->Precv = *hooks->Precv;
		pvar->Psend = *hooks->Psend;
		pvar->PWSAAsyncSelect = *hooks->PWSAAsyncSelect;
		pvar->Pconnect = *hooks->Pconnect;
		pvar->PWSAGetLastError = *hooks->PWSAGetLastError;

		*hooks->Precv = TTXrecv;
		*hooks->Psend = TTXsend;
		*hooks->PWSAAsyncSelect = TTXWSAAsyncSelect;
		*hooks->Pconnect = TTXconnect;

		SSH_open(pvar);
		HOSTS_open(pvar);
		FWDUI_open(pvar);

		// 設定を myproposal に反映するのは、接続直前のここだけ。 (2006.6.26 maya)
		SSH2_update_cipher_myproposal(pvar);
		SSH2_update_kex_myproposal(pvar);
		SSH2_update_host_key_myproposal(pvar);
		SSH2_update_hmac_myproposal(pvar);
		SSH2_update_compression_myproposal(pvar);
	}
}

static void PASCAL TTXCloseTCP(TTXSockHooks *hooks)
{
	if (pvar->session_settings.Enabled) {
		pvar->socket = INVALID_SOCKET;

		logputs(LOG_LEVEL_VERBOSE, "Terminating SSH session...");

		*hooks->Precv = pvar->Precv;
		*hooks->Psend = pvar->Psend;
		*hooks->PWSAAsyncSelect = pvar->PWSAAsyncSelect;
		*hooks->Pconnect = pvar->Pconnect;

		pvar->ts->DisableTCPEchoCR = pvar->origDisableTCPEchoCR;
	}

	uninit_TTSSH(pvar);
	init_TTSSH(pvar);
}

static void enable_dlg_items(HWND dlg, int from, int to, BOOL enabled)
{
	for (; from <= to; from++) {
		EnableWindow(GetDlgItem(dlg, from), enabled);
	}
}

// C-p/C-n/C-b/C-f/C-a/C-e をサポート (2007.9.5 maya)
// C-d/C-k をサポート (2007.10.3 yutaka)
// ドロップダウンの中のエディットコントロールを
// サブクラス化するためのウインドウプロシージャ
WNDPROC OrigHostnameEditProc; // Original window procedure
LRESULT CALLBACK HostnameEditProc(HWND dlg, UINT msg,
                                  WPARAM wParam, LPARAM lParam)
{
	HWND parent;
	int  max, select, len;
	char *str, *orgstr;

	switch (msg) {
		// キーが押されたのを検知する
		case WM_KEYDOWN:
			if (GetKeyState(VK_CONTROL) < 0) {
				switch (wParam) {
					case 0x50: // Ctrl+p ... up
						parent = GetParent(dlg);
						select = SendMessage(parent, CB_GETCURSEL, 0, 0);
						if (select > 0) {
							PostMessage(parent, CB_SETCURSEL, select - 1, 0);
						}
						return 0;
					case 0x4e: // Ctrl+n ... down
						parent = GetParent(dlg);
						max = SendMessage(parent, CB_GETCOUNT, 0, 0);
						select = SendMessage(parent, CB_GETCURSEL, 0, 0);
						if (select < max - 1) {
							PostMessage(parent, CB_SETCURSEL, select + 1, 0);
						}
						return 0;
					case 0x42: // Ctrl+b ... left
						SendMessage(dlg, EM_GETSEL, 0, (LPARAM)&select);
						PostMessage(dlg, EM_SETSEL, select-1, select-1);
						return 0;
					case 0x46: // Ctrl+f ... right
						SendMessage(dlg, EM_GETSEL, 0, (LPARAM)&select);
						max = GetWindowTextLength(dlg) ;
						PostMessage(dlg, EM_SETSEL, select+1, select+1);
						return 0;
					case 0x41: // Ctrl+a ... home
						PostMessage(dlg, EM_SETSEL, 0, 0);
						return 0;
					case 0x45: // Ctrl+e ... end
						max = GetWindowTextLength(dlg) ;
						PostMessage(dlg, EM_SETSEL, max, max);
						return 0;

					case 0x44: // Ctrl+d
					case 0x4b: // Ctrl+k
					case 0x55: // Ctrl+u
						SendMessage(dlg, EM_GETSEL, 0, (LPARAM)&select);
						max = GetWindowTextLength(dlg);
						max++; // '\0'
						orgstr = str = malloc(max);
						if (str != NULL) {
							len = GetWindowText(dlg, str, max);
							if (select >= 0 && select < len) {
								if (wParam == 0x44) { // カーソル配下の文字のみを削除する
									memmove(&str[select], &str[select + 1], len - select - 1);
									str[len - 1] = '\0';

								} else if (wParam == 0x4b) { // カーソルから行末まで削除する
									str[select] = '\0';

								}
							}

							if (wParam == 0x55) { // カーソルより左側をすべて消す
								if (select >= len) {
									str[0] = '\0';
								} else {
									str = &str[select];
								}
								select = 0;
							}

							SetWindowText(dlg, str);
							SendMessage(dlg, EM_SETSEL, select, select);
							free(orgstr);
							return 0;
						}
						break;
				}
			}
			break;

		// 上のキーを押した結果送られる文字で音が鳴るので捨てる
		case WM_CHAR:
			switch (wParam) {
				case 0x01:
				case 0x02:
				case 0x04:
				case 0x05:
				case 0x06:
				case 0x0b:
				case 0x0e:
				case 0x10:
				case 0x15:
					return 0;
			}
	}

	return CallWindowProc(OrigHostnameEditProc, dlg, msg, wParam, lParam);
}

static BOOL CALLBACK TTXHostDlg(HWND dlg, UINT msg, WPARAM wParam,
                                LPARAM lParam)
{
	static char *ssh_version[] = {"SSH1", "SSH2", NULL};
	PGetHNRec GetHNRec;
	char EntName[128];
	char TempHost[HostNameMaxLength + 1];
	WORD i, j, w;
	WORD ComPortTable[MAXCOMPORT];
	static char *ComPortDesc[MAXCOMPORT];
	int comports;
	BOOL Ok;
	LOGFONT logfont;
	HFONT font;
	char uimsg[MAX_UIMSG];
	static HWND hwndHostname     = NULL; // HOSTNAME dropdown
	static HWND hwndHostnameEdit = NULL; // Edit control on HOSTNAME dropdown

	switch (msg) {
	case WM_INITDIALOG:
		GetHNRec = (PGetHNRec) lParam;
		SetWindowLong(dlg, DWL_USER, lParam);

		GetWindowText(dlg, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_TITLE", pvar, uimsg);
		SetWindowText(dlg, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTNAMELABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_TCPIPHOST", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTNAMELABEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HISTORY, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_TCPIPHISTORY", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HISTORY, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_SERVICELABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_TCPIPSERVICE", pvar, uimsg);
		SetDlgItemText(dlg, IDC_SERVICELABEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTOTHER, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_TCPIPOTHER", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTOTHER, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTTCPPORTLABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_TCPIPPORT", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTTCPPORTLABEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_SSH_VERSION_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_TCPIPSSHVERSION", pvar, uimsg);
		SetDlgItemText(dlg, IDC_SSH_VERSION_LABEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTTCPPROTOCOLLABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_TCPIPPROTOCOL", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTTCPPROTOCOLLABEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTSERIAL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_SERIAL", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTSERIAL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTCOMLABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_SERIALPORT", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTCOMLABEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_HOSTHELP, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_HOST_HELP", pvar, uimsg);
		SetDlgItemText(dlg, IDC_HOSTHELP, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDOK, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("BTN_OK", pvar, uimsg);
		SetDlgItemText(dlg, IDOK, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("BTN_CANCEL", pvar, uimsg);
		SetDlgItemText(dlg, IDCANCEL, pvar->ts->UIMsg);

		// ホストヒストリのチェックボックスを追加 (2005.10.21 yutaka)
		if (pvar->ts->HistoryList > 0) {
			SendMessage(GetDlgItem(dlg, IDC_HISTORY), BM_SETCHECK, BST_CHECKED, 0);
		} else {
			SendMessage(GetDlgItem(dlg, IDC_HISTORY), BM_SETCHECK, BST_UNCHECKED, 0);
		}

		// ファイルおよび名前付きパイプの場合、TCP/IP扱いとする。
		if (GetHNRec->PortType == IdFile ||
			GetHNRec->PortType == IdNamedPipe
			)
			GetHNRec->PortType = IdTCPIP;

		strncpy_s(EntName, sizeof(EntName), "Host", _TRUNCATE);

		i = 1;
		do {
			_snprintf_s(&EntName[4], sizeof(EntName)-4, _TRUNCATE, "%d", i);
			GetPrivateProfileString("Hosts", EntName, "",
			                        TempHost, sizeof(TempHost),
			                        GetHNRec->SetupFN);
			if (strlen(TempHost) > 0)
				SendDlgItemMessage(dlg, IDC_HOSTNAME, CB_ADDSTRING,
				                   0, (LPARAM) TempHost);
			i++;
		} while (i <= MAXHOSTLIST);

		SendDlgItemMessage(dlg, IDC_HOSTNAME, EM_LIMITTEXT,
		                   HostNameMaxLength - 1, 0);

		SendDlgItemMessage(dlg, IDC_HOSTNAME, CB_SETCURSEL, 0, 0);

		// C-n/C-p のためにサブクラス化 (2007.9.4 maya)
		hwndHostname = GetDlgItem(dlg, IDC_HOSTNAME);
		hwndHostnameEdit = GetWindow(hwndHostname, GW_CHILD);
		OrigHostnameEditProc = (WNDPROC)GetWindowLong(hwndHostnameEdit, GWL_WNDPROC);
		SetWindowLong(hwndHostnameEdit, GWL_WNDPROC, (LONG)HostnameEditProc);

		CheckRadioButton(dlg, IDC_HOSTTELNET, IDC_HOSTOTHER,
		                 pvar->settings.Enabled ? IDC_HOSTSSH : GetHNRec->
		                 Telnet ? IDC_HOSTTELNET : IDC_HOSTOTHER);
		SendDlgItemMessage(dlg, IDC_HOSTTCPPORT, EM_LIMITTEXT, 5, 0);
		SetDlgItemInt(dlg, IDC_HOSTTCPPORT, GetHNRec->TCPPort, FALSE);
		for (i = 0; ProtocolFamilyList[i]; ++i) {
			SendDlgItemMessage(dlg, IDC_HOSTTCPPROTOCOL, CB_ADDSTRING,
			                   0, (LPARAM) ProtocolFamilyList[i]);
		}
		SendDlgItemMessage(dlg, IDC_HOSTTCPPROTOCOL, EM_LIMITTEXT,
		                   ProtocolFamilyMaxLength - 1, 0);
		SendDlgItemMessage(dlg, IDC_HOSTTCPPROTOCOL, CB_SETCURSEL, 0, 0);

		/////// SSH version
		for (i = 0; ssh_version[i]; ++i) {
			SendDlgItemMessage(dlg, IDC_SSH_VERSION, CB_ADDSTRING,
			                   0, (LPARAM) ssh_version[i]);
		}
		SendDlgItemMessage(dlg, IDC_SSH_VERSION, EM_LIMITTEXT,
		                   NUM_ELEM(ssh_version) - 1, 0);

		if (pvar->settings.ssh_protocol_version == 1) {
			SendDlgItemMessage(dlg, IDC_SSH_VERSION, CB_SETCURSEL, 0, 0); // SSH1
		} else {
			SendDlgItemMessage(dlg, IDC_SSH_VERSION, CB_SETCURSEL, 1, 0); // SSH2
		}

		if (IsDlgButtonChecked(dlg, IDC_HOSTSSH)) {
			enable_dlg_items(dlg, IDC_SSH_VERSION, IDC_SSH_VERSION, TRUE); // enabled
		} else {
			enable_dlg_items(dlg, IDC_SSH_VERSION, IDC_SSH_VERSION, FALSE); // disabled
		}
		/////// SSH version


		j = 0;
		w = 1;
		if ((comports=DetectComPorts(ComPortTable, GetHNRec->MaxComPort, ComPortDesc)) >= 0) {
			for (i=0; i<comports; i++) {
				// MaxComPort を越えるポートは表示しない
				if (ComPortTable[i] > GetHNRec->MaxComPort) {
					continue;
				}

				// 使用中のポートは表示しない
				if (CheckCOMFlag(ComPortTable[i]) == 1) {
					continue;
				}

				_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "COM%d", ComPortTable[i]);
				if (ComPortDesc[i] != NULL) {
					strncat_s(EntName, sizeof(EntName), ": ", _TRUNCATE);
					strncat_s(EntName, sizeof(EntName), ComPortDesc[i], _TRUNCATE);
				}
				SendDlgItemMessage(dlg, IDC_HOSTCOM, CB_ADDSTRING,
				                   0, (LPARAM)EntName);
				j++;
				if (GetHNRec->ComPort == ComPortTable[i])
					w = j;
			}

		} else {
			for (i = 1; i <= GetHNRec->MaxComPort; i++) {
				// 使用中のポートは表示しない
				if (CheckCOMFlag(i) == 1) {
					continue;
				}

				_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "COM%d", i);
				SendDlgItemMessage(dlg, IDC_HOSTCOM, CB_ADDSTRING,
				                   0, (LPARAM) EntName);
				j++;
				if (GetHNRec->ComPort == i)
					w = j;
			}
		}

		if (j > 0)
			SendDlgItemMessage(dlg, IDC_HOSTCOM, CB_SETCURSEL, w - 1, 0);
		else {					/* All com ports are already used */
			GetHNRec->PortType = IdTCPIP;
			enable_dlg_items(dlg, IDC_HOSTSERIAL, IDC_HOSTSERIAL, FALSE);
		}

		CheckRadioButton(dlg, IDC_HOSTTCPIP, IDC_HOSTSERIAL,
		                 IDC_HOSTTCPIP + GetHNRec->PortType - 1);

		if (GetHNRec->PortType == IdTCPIP) {
			enable_dlg_items(dlg, IDC_HOSTCOMLABEL, IDC_HOSTCOM, FALSE);

			enable_dlg_items(dlg, IDC_SSH_VERSION, IDC_SSH_VERSION, TRUE);
			enable_dlg_items(dlg, IDC_SSH_VERSION_LABEL, IDC_SSH_VERSION_LABEL, TRUE);

			enable_dlg_items(dlg, IDC_HISTORY, IDC_HISTORY, TRUE); // enabled
		}
		else {
			enable_dlg_items(dlg, IDC_HOSTNAMELABEL, IDC_HOSTTCPPORT,
			                 FALSE);
			enable_dlg_items(dlg, IDC_HOSTTCPPROTOCOLLABEL,
			                 IDC_HOSTTCPPROTOCOL, FALSE);

			enable_dlg_items(dlg, IDC_SSH_VERSION, IDC_SSH_VERSION, FALSE); // disabled
			enable_dlg_items(dlg, IDC_SSH_VERSION_LABEL, IDC_SSH_VERSION_LABEL, FALSE); // disabled (2004.11.23 yutaka)

			enable_dlg_items(dlg, IDC_HISTORY, IDC_HISTORY, FALSE); // disabled
		}

		// Host dialogにフォーカスをあてる (2004.10.2 yutaka)
		if (GetHNRec->PortType == IdTCPIP) {
			HWND hwnd = GetDlgItem(dlg, IDC_HOSTNAME);
			SetFocus(hwnd);
		} else {
			HWND hwnd = GetDlgItem(dlg, IDC_HOSTCOM);
			SetFocus(hwnd);
		}

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_SYSTEM_FONT", dlg, &logfont, &DlgHostFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_HOSTTCPIP, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTNAMELABEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTNAME, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HISTORY, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SERVICELABEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTTELNET, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTSSH, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTOTHER, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTTCPPORTLABEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTTCPPORT, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSH_VERSION_LABEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSH_VERSION, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTTCPPROTOCOLLABEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTTCPPROTOCOL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTSERIAL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTCOMLABEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTCOM, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDOK, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDCANCEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HOSTHELP, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgHostFont = NULL;
		}

		// SetFocus()でフォーカスをあわせた場合、FALSEを返す必要がある。
		// TRUEを返すと、TABSTOP対象の一番はじめのコントロールが選ばれる。
		// (2004.11.23 yutaka)
		return FALSE;
		//return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetHNRec = (PGetHNRec) GetWindowLong(dlg, DWL_USER);
			if (GetHNRec != NULL) {
				if (IsDlgButtonChecked(dlg, IDC_HOSTTCPIP)) {
					char afstr[BUFSIZ];
					i = GetDlgItemInt(dlg, IDC_HOSTTCPPORT, &Ok, FALSE);
					if (Ok) {
						GetHNRec->TCPPort = i;
					} else {
						UTIL_get_lang_msg("MSG_TCPPORT_NAN_ERROR", pvar,
						                  "The TCP port must be a number.");
						MessageBox(dlg, pvar->ts->UIMsg,
						           "Tera Term", MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
#define getaf(str) \
((strcmp((str), "IPv6") == 0) ? AF_INET6 : \
 ((strcmp((str), "IPv4") == 0) ? AF_INET : AF_UNSPEC))
					memset(afstr, 0, sizeof(afstr));
					GetDlgItemText(dlg, IDC_HOSTTCPPROTOCOL, afstr,
					               sizeof(afstr));
					GetHNRec->ProtocolFamily = getaf(afstr);
					GetHNRec->PortType = IdTCPIP;
					GetDlgItemText(dlg, IDC_HOSTNAME, GetHNRec->HostName,
					               HostNameMaxLength);
					pvar->hostdlg_activated = TRUE;
					pvar->hostdlg_Enabled = FALSE;
					if (IsDlgButtonChecked(dlg, IDC_HOSTTELNET)) {
						GetHNRec->Telnet = TRUE;
					} else if (IsDlgButtonChecked(dlg, IDC_HOSTSSH)) {
						pvar->hostdlg_Enabled = TRUE;

						// check SSH protocol version
						memset(afstr, 0, sizeof(afstr));
						GetDlgItemText(dlg, IDC_SSH_VERSION, afstr, sizeof(afstr));
						if (_stricmp(afstr, "SSH1") == 0) {
							pvar->settings.ssh_protocol_version = 1;
						} else {
							pvar->settings.ssh_protocol_version = 2;
						}
					}
					else {	// IDC_HOSTOTHER
						GetHNRec->Telnet = FALSE;
					}

					// host history check button
					if (SendMessage(GetDlgItem(dlg, IDC_HISTORY), BM_GETCHECK, 0, 0) == BST_CHECKED) {
						pvar->ts->HistoryList = 1;
					} else {
						pvar->ts->HistoryList = 0;
					}

				} else {
					GetHNRec->PortType = IdSerial;
					GetHNRec->HostName[0] = 0;
					memset(EntName, 0, sizeof(EntName));
					GetDlgItemText(dlg, IDC_HOSTCOM, EntName,
					               sizeof(EntName) - 1);
					if (strncmp(EntName, "COM", 3) == 0 && EntName[3] != '\0') {
						GetHNRec->ComPort = atoi(&EntName[3]);
						if (GetHNRec->ComPort > GetHNRec->MaxComPort)
							GetHNRec->ComPort = 1;
					} else {
						GetHNRec->ComPort = 1;
					}
				}
			}
			SetWindowLong(hwndHostnameEdit, GWL_WNDPROC, (LONG)OrigHostnameEditProc);
			EndDialog(dlg, 1);

			if (DlgHostFont != NULL) {
				DeleteObject(DlgHostFont);
			}

			return TRUE;

		case IDCANCEL:
			SetWindowLong(hwndHostnameEdit, GWL_WNDPROC, (LONG)OrigHostnameEditProc);
			EndDialog(dlg, 0);

			if (DlgHostFont != NULL) {
				DeleteObject(DlgHostFont);
			}

			return TRUE;

		case IDC_HOSTTCPIP:
			enable_dlg_items(dlg, IDC_HOSTNAMELABEL, IDC_HOSTTCPPORT,
			                 TRUE);
			enable_dlg_items(dlg, IDC_HOSTTCPPROTOCOLLABEL,
			                 IDC_HOSTTCPPROTOCOL, TRUE);
			enable_dlg_items(dlg, IDC_HOSTCOMLABEL, IDC_HOSTCOM, FALSE);

			enable_dlg_items(dlg, IDC_SSH_VERSION_LABEL, IDC_SSH_VERSION_LABEL, TRUE); // disabled (2004.11.23 yutaka)
			if (IsDlgButtonChecked(dlg, IDC_HOSTSSH)) {
				enable_dlg_items(dlg, IDC_SSH_VERSION, IDC_SSH_VERSION, TRUE);
			} else {
				enable_dlg_items(dlg, IDC_SSH_VERSION, IDC_SSH_VERSION, FALSE); // disabled
			}

			enable_dlg_items(dlg, IDC_HISTORY, IDC_HISTORY, TRUE); // disabled

			return TRUE;

		case IDC_HOSTSERIAL:
			enable_dlg_items(dlg, IDC_HOSTCOMLABEL, IDC_HOSTCOM, TRUE);
			enable_dlg_items(dlg, IDC_HOSTNAMELABEL, IDC_HOSTTCPPORT,
			                 FALSE);
			enable_dlg_items(dlg, IDC_HOSTTCPPROTOCOLLABEL,
			                 IDC_HOSTTCPPROTOCOL, FALSE);
			enable_dlg_items(dlg, IDC_SSH_VERSION, IDC_SSH_VERSION, FALSE); // disabled
			enable_dlg_items(dlg, IDC_SSH_VERSION_LABEL, IDC_SSH_VERSION_LABEL, FALSE); // disabled (2004.11.23 yutaka)

			enable_dlg_items(dlg, IDC_HISTORY, IDC_HISTORY, FALSE); // disabled

			return TRUE;

		case IDC_HOSTSSH:
			enable_dlg_items(dlg, IDC_SSH_VERSION,
			                 IDC_SSH_VERSION, TRUE);
			goto hostssh_enabled;

		case IDC_HOSTTELNET:
		case IDC_HOSTOTHER:
			enable_dlg_items(dlg, IDC_SSH_VERSION, IDC_SSH_VERSION, FALSE); // disabled
hostssh_enabled:

			GetHNRec = (PGetHNRec) GetWindowLong(dlg, DWL_USER);

			if (IsDlgButtonChecked(dlg, IDC_HOSTTELNET)) {
				if (GetHNRec != NULL)
					SetDlgItemInt(dlg, IDC_HOSTTCPPORT, GetHNRec->TelPort,
					              FALSE);
			} else if (IsDlgButtonChecked(dlg, IDC_HOSTSSH)) {
				SetDlgItemInt(dlg, IDC_HOSTTCPPORT, 22, FALSE);
			}
			return TRUE;

		case IDC_HOSTCOM:
			if(HIWORD(wParam) == CBN_DROPDOWN) {
				HWND hostcom = GetDlgItem(dlg, IDC_HOSTCOM);
				int count = SendMessage(hostcom, CB_GETCOUNT, 0, 0);
				int i, len, max_len = 0;
				char *lbl;
				HDC TmpDC = GetDC(hostcom);
				SIZE s;
				for (i=0; i<count; i++) {
					len = SendMessage(hostcom, CB_GETLBTEXTLEN, i, 0);
					lbl = (char *)calloc(len+1, sizeof(char));
					SendMessage(hostcom, CB_GETLBTEXT, i, (LPARAM)lbl);
					GetTextExtentPoint32(TmpDC, lbl, len, &s);
					if (s.cx > max_len)
						max_len = s.cx;
					free(lbl);
				}
				SendMessage(hostcom, CB_SETDROPPEDWIDTH,
							max_len + GetSystemMetrics(SM_CXVSCROLL), 0);
			}
			break;

		case IDC_HOSTHELP:
			PostMessage(GetParent(dlg), WM_USER_DLGHELP2, 0, 0);
		}
	}
	return FALSE;
}

static BOOL PASCAL TTXGetHostName(HWND parent, PGetHNRec rec)
{
	return (BOOL) DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_HOSTDLG),
	                             parent, TTXHostDlg, (LONG) rec);
}

static void PASCAL TTXGetUIHooks(TTXUIHooks *hooks)
{
	*hooks->GetHostName = TTXGetHostName;
}

static void PASCAL TTXReadINIFile(PCHAR fileName, PTTSet ts)
{
	(pvar->ReadIniFile) (fileName, ts);
	read_ssh_options(pvar, fileName);
	pvar->settings = *pvar->ts_SSH;
	logputs(LOG_LEVEL_VERBOSE, "Reading INI file");
	FWDUI_load_settings(pvar);
}

static void PASCAL TTXWriteINIFile(PCHAR fileName, PTTSet ts)
{
	(pvar->WriteIniFile) (fileName, ts);
	*pvar->ts_SSH = pvar->settings;
	clear_local_settings(pvar);
	logputs(LOG_LEVEL_VERBOSE, "Writing INI file");
	write_ssh_options(pvar, fileName, pvar->ts_SSH, TRUE);
}

static void read_ssh_options_from_user_file(PTInstVar pvar,
                                            char *user_file_name)
{
	if (user_file_name[0] == '.') {
		read_ssh_options(pvar, user_file_name);
	} else {
		char buf[1024];

		get_teraterm_dir_relative_name(buf, sizeof(buf), user_file_name);
		read_ssh_options(pvar, buf);
	}

	pvar->settings = *pvar->ts_SSH;
	FWDUI_load_settings(pvar);
}

// Percent-encodeされた文字列srcをデコードしてdstにコピーする。
// dstlenはdstのサイズ。これより結果が長い場合、その分は切り捨てられる。
static void percent_decode(char *dst, int dstlen, char *src) {
	if (src == NULL || dst == NULL || dstlen < 1) {
		return;
	}

	while (*src != 0 && dstlen > 1) {
		if (*src == '%' && isxdigit(*(src+1)) && isxdigit(*(src+2))) {
			src++; *dst  = (isalpha(*src) ? (*src|0x20) - 'a' + 10 : *src - '0') << 4;
			src++; *dst |= (isalpha(*src) ? (*src|0x20) - 'a' + 10 : *src - '0');
			src++; dst++;
		}
		else {
			*dst++ = *src++;
		}
		dstlen--;
	}
	*dst = 0;
	return;
}

void add_forward_param(PTInstVar pvar, char *param)
{
	if (pvar->settings.DefaultForwarding[0] == 0) {
		strncpy_s(pvar->settings.DefaultForwarding,
		          sizeof(pvar->settings.DefaultForwarding),
		          param, _TRUNCATE);
	} else {
		strncat_s(pvar->settings.DefaultForwarding,
		          sizeof(pvar->settings.DefaultForwarding),
		          ";", _TRUNCATE);
		strncat_s(pvar->settings.DefaultForwarding,
		          sizeof(pvar->settings.DefaultForwarding),
		          param, _TRUNCATE);
	}
}

static void PASCAL TTXParseParam(PCHAR param, PTTSet ts, PCHAR DDETopic) {
	int param_len=strlen(param);
	int opt_len = param_len+1;
	char *option = (char *)calloc(opt_len, sizeof(char));
	char *option2 = (char *)calloc(opt_len, sizeof(char));
	int action;
	PCHAR start, cur, next;
	size_t i;

	if (pvar->hostdlg_activated) {
		pvar->settings.Enabled = pvar->hostdlg_Enabled;
	}

	/* the first term shuld be executable filename of Tera Term */
	start = GetParam(option, opt_len, param);

	cur = start;
	while (next = GetParam(option, opt_len, cur)) {
		DequoteParam(option, opt_len, option);
		action = OPTION_NONE;

		if ((option[0] == '-' || option[0] == '/')) {
			if (MATCH_STR(option + 1, "ssh") == 0) {
				if (MATCH_STR(option + 4, "-f=") == 0) {
					strncpy_s(option2, opt_len, option + 7, _TRUNCATE);
					read_ssh_options_from_user_file(pvar, option2);
					action = OPTION_CLEAR;
				} else if (MATCH_STR(option + 4, "-consume=") == 0) {
					strncpy_s(option2, opt_len, option + 13, _TRUNCATE);
					read_ssh_options_from_user_file(pvar, option2);
					DeleteFile(option2);
					action = OPTION_CLEAR;
				}

			// ttermpro.exe の /F= 指定でも TTSSH の設定を読む (2006.10.11 maya)
			} else if (MATCH_STR_I(option + 1, "f=") == 0) {
				strncpy_s(option2, opt_len, option + 3, _TRUNCATE);
				read_ssh_options_from_user_file(pvar, option2);
				// Tera Term側でも解釈する必要があるので消さない
			}
		}

		switch (action) {
			case OPTION_CLEAR:
				memset(cur, ' ', next-cur);
				break;
			case OPTION_REPLACE:
				memset(cur, ' ', next-cur);
				memcpy(cur+1, option, strlen(option));
				break;
		}

		cur = next;
	}

	cur = start;
	while (next = GetParam(option, opt_len, cur)) {	
		DequoteParam(option, opt_len, option);
		action = OPTION_NONE;

		if ((option[0] == '-' || option[0] == '/')) {
			action = OPTION_CLEAR;
			if (MATCH_STR(option + 1, "ssh") == 0) {
				if (option[4] == 0) {
					pvar->settings.Enabled = 1;
				} else if (MATCH_STR(option + 4, "-L") == 0 ||
				           MATCH_STR(option + 4, "-R") == 0 ||
				           MATCH_STR(option + 4, "-D") == 0) {
					char *p = option + 5;
					option2[0] = *p;
					i = 1;
					while (*++p) {
						if (*p == ';' || *p == ',') {
							option2[i] = 0;
							add_forward_param(pvar, option2);
							i = 1;
						}
						else {
							option2[i++] = *p;
						}
					}
					if (i > 1) {
						option2[i] = 0;
						add_forward_param(pvar, option2);
					}
				} else if (MATCH_STR(option + 4, "-X") == 0) {
					add_forward_param(pvar, "X");
					if (option+6 != 0) {
						strncpy_s(pvar->settings.X11Display,
						          sizeof(pvar->settings.X11Display),
						          option + 6, _TRUNCATE);
					}
				} else if (strcmp(option + 4, "-v") == 0) {
					pvar->settings.LogLevel = LOG_LEVEL_VERBOSE;
				} else if (_stricmp(option + 4, "-autologin") == 0 ||
				           _stricmp(option + 4, "-autologon") == 0) {
					pvar->settings.TryDefaultAuth = TRUE;
				} else if (MATCH_STR_I(option + 4, "-agentconfirm=") == 0) {
					if ((_stricmp(option+18, "off") == 0) ||
					    (_stricmp(option+18, "no") == 0) ||
					    (_stricmp(option+18, "false") == 0) ||
					    (_stricmp(option+18, "0") == 0) ||
					    (_stricmp(option+18, "n") == 0)) {
						pvar->settings.ForwardAgentConfirm = 0;
					}
					else {
						pvar->settings.ForwardAgentConfirm = 1;
					}
				} else if (strcmp(option + 4, "-a") == 0) {
					pvar->settings.ForwardAgent = FALSE;
				} else if (strcmp(option + 4, "-A") == 0) {
					pvar->settings.ForwardAgent = TRUE;

				} else if (MATCH_STR(option + 4, "-C=") == 0) {
					pvar->settings.CompressionLevel = atoi(option+7);
					if (pvar->settings.CompressionLevel < 0) {
						pvar->settings.CompressionLevel = 0;
					}
					else if (pvar->settings.CompressionLevel > 9) {
						pvar->settings.CompressionLevel = 9;
					}
				} else if (strcmp(option + 4, "-C") == 0) {
					pvar->settings.CompressionLevel = 6;
				} else if (strcmp(option + 4, "-c") == 0) {
					pvar->settings.CompressionLevel = 0;
				} else if (MATCH_STR_I(option + 4, "-icon=") == 0) {
					if ((_stricmp(option+10, "old") == 0) ||
					    (_stricmp(option+10, "yellow") == 0) ||
					    (_stricmp(option+10, "securett_yellow") == 0)) {
						pvar->settings.IconID = IDI_SECURETT_YELLOW;
					}
					else if ((_stricmp(option+10, "green") == 0) ||
					         (_stricmp(option+10, "securett_green") == 0)) {
						pvar->settings.IconID = IDI_SECURETT_GREEN;
					}
					else {
						pvar->settings.IconID = IDI_SECURETT;
					}
				} else if (MATCH_STR(option + 4, "-subsystem=") == 0) {
					pvar->use_subsystem = TRUE;
					strncpy_s(pvar->subsystem_name,
					          sizeof(pvar->subsystem_name),
					          option + 15, _TRUNCATE);

				// /ssh1 と /ssh2 オプションの新規追加 (2006.9.16 maya)
				} else if (strcmp(option + 4, "1") == 0) {
					pvar->settings.Enabled = 1;
					pvar->settings.ssh_protocol_version = 1;
				} else if (strcmp(option + 4, "2") == 0) {
					pvar->settings.Enabled = 1;
					pvar->settings.ssh_protocol_version = 2;

				} else {
					char buf[1024];

					UTIL_get_lang_msg("MSG_UNKNOWN_OPTION_ERROR", pvar,
					                  "Unrecognized command-line option: %s");
					_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg, option);

					MessageBox(NULL, buf, "TTSSH", MB_OK | MB_ICONEXCLAMATION);
				}

			// ttermpro.exe の /T= 指定の流用なので、大文字も許す (2006.10.19 maya)
			} else if (MATCH_STR_I(option + 1, "t=") == 0) {
				if (strcmp(option + 3, "2") == 0) {
					pvar->settings.Enabled = 1;
					// /t=2はttssh側での拡張なので消す
				} else {
					pvar->settings.Enabled = 0;
					action = OPTION_NONE;	// Tera Term側で解釈するので消さない
				}

			// /1 および /2 オプションの新規追加 (2004.10.3 yutaka)
			} else if (strcmp(option + 1, "1") == 0) {
				// command line: /ssh /1 is SSH1 only
				pvar->settings.ssh_protocol_version = 1;

			} else if (strcmp(option + 1, "2") == 0) {
				// command line: /ssh /2 is SSH2 & SSH1
				pvar->settings.ssh_protocol_version = 2;

			} else if (strcmp(option + 1, "nossh") == 0) {
				// '/nossh' オプションの追加。
				// TERATERM.INI でSSHが有効になっている場合、うまくCygtermが起動しないことが
				// あることへの対処。(2004.10.11 yutaka)
				pvar->settings.Enabled = 0;

			} else if (strcmp(option + 1, "telnet") == 0) {
				// '/telnet' が指定されているときには '/nossh' と同じく
				// SSHを無効にする (2006.9.16 maya)
				pvar->settings.Enabled = 0;
				// Tera Term の Telnet フラグも付ける
				pvar->ts->Telnet = 1;

			} else if (MATCH_STR(option + 1, "auth=") == 0) {
				// SSH2自動ログインオプションの追加
				//
				// SYNOPSIS: /ssh /auth=passowrd /user=ユーザ名 /passwd=パスワード
				//           /ssh /auth=publickey /user=ユーザ名 /passwd=パスワード /keyfile=パス
				// EXAMPLE: /ssh /auth=password /user=nike /passwd="a b""c"  ; パスワード: 「a b"c」
				//          /ssh /auth=publickey /user=foo /passwd=bar /keyfile=d:\tmp\id_rsa
				// NOTICE: パスワードやパスに空白やセミコロンが含まれる場合はダブルクォート " で囲む
				//         パスワードにダブルクォートが含まれる場合は連続したダブルクォート "" に置き換える
				//
				pvar->ssh2_autologin = 1; // for SSH2 (2004.11.30 yutaka)

				if (_stricmp(option + 6, "password") == 0) { // パスワード
					//pvar->auth_state.cur_cred.method = SSH_AUTH_PASSWORD;
					pvar->ssh2_authmethod = SSH_AUTH_PASSWORD;

				// /auth=challenge を追加 (2007.10.5 maya)
				} else if (_stricmp(option + 6, "challenge") == 0) { // keyboard-interactive認証
					//pvar->auth_state.cur_cred.method = SSH_AUTH_TIS;
					pvar->ssh2_authmethod = SSH_AUTH_TIS;

				} else if (_stricmp(option + 6, "publickey") == 0) { // 公開鍵認証
					//pvar->auth_state.cur_cred.method = SSH_AUTH_RSA;
					pvar->ssh2_authmethod = SSH_AUTH_RSA;

				} else if (_stricmp(option + 6, "pageant") == 0) { // 公開鍵認証 by Pageant
					//pvar->auth_state.cur_cred.method = SSH_AUTH_RSA;
					pvar->ssh2_authmethod = SSH_AUTH_PAGEANT;

				} else {
					// TODO:
				}

			} else if (MATCH_STR(option + 1, "user=") == 0) {
				_snprintf_s(pvar->ssh2_username, sizeof(pvar->ssh2_username), _TRUNCATE, "%s", option+6);

			} else if (MATCH_STR(option + 1, "passwd=") == 0) {
				_snprintf_s(pvar->ssh2_password, sizeof(pvar->ssh2_password), _TRUNCATE, "%s", option+8);

			} else if (MATCH_STR(option + 1, "keyfile=") == 0) {
				_snprintf_s(pvar->ssh2_keyfile, sizeof(pvar->ssh2_keyfile), _TRUNCATE, "%s", option+9);

			} else if (strcmp(option + 1, "ask4passwd") == 0) {
				// パスワードを聞く (2006.9.18 maya)
				pvar->ask4passwd = 1;

			} else if (strcmp(option + 1, "nosecuritywarning") == 0) {
				// known_hostsチェックをしない。当該オプションを使うと、セキュリティ性が低下する
				// ため、隠しオプション扱いとする。
				// (2009.10.4 yutaka)
				pvar->nocheck_known_hosts = TRUE;

			}
			else {	// Other (not ttssh) option
				action = OPTION_NONE;	// ttsshのオプションではないので消さない
			}

			// パスワードを聞く場合は自動ログインが無効になる
			// /auth は認証メソッドの指定としては利用される (2006.9.18 maya)
			if (pvar->ask4passwd == 1) {
				pvar->ssh2_autologin = 0;
			}

		}
		else if ((MATCH_STR_I(option, "ssh://") == 0) ||
		         (MATCH_STR_I(option, "ssh1://") == 0) ||
		         (MATCH_STR_I(option, "ssh2://") == 0) ||
		         (MATCH_STR_I(option, "slogin://") == 0) ||
		         (MATCH_STR_I(option, "slogin1://") == 0) ||
		         (MATCH_STR_I(option, "slogin2://") == 0)) {
			//
			// ssh://user@host/ 等のURL形式のサポート
			// 基本的な書式は telnet:// URLに順ずる
			//
			// 参考:
			//   RFC3986: Uniform Resource Identifier (URI): Generic Syntax
			//   RFC4248: The telnet URI Scheme
			//
			char *p, *p2, *p3;
			int optlen, hostlen;

			optlen = strlen(option);

			// 最初の':'の前の文字が数字だった場合、それをsshプロトコルバージョンとみなす
			p = _mbschr(option, ':');
			switch (*(p-1)) {
			case '1':
				pvar->settings.ssh_protocol_version = 1;
				break;
			case '2':
				pvar->settings.ssh_protocol_version = 2;
				break;
			}

			// authority part までポインタを移動
			p += 3;

			// path part を切り捨てる
			if ((p2 = _mbschr(p, '/')) != NULL) {
				*p2 = 0;
			}

			// '@'があった場合、それより前はユーザ情報
			if ((p2 = _mbschr(p, '@')) != NULL) {
				*p2 = 0;
				// ':'以降はパスワード
				if ((p3 = _mbschr(p, ':')) != NULL) {
					*p3 = 0;
					percent_decode(pvar->ssh2_password, sizeof(pvar->ssh2_password), p3 + 1);
				}
				percent_decode(pvar->ssh2_username, sizeof(pvar->ssh2_username), p);
				// p が host part の先頭('@'の次の文字)を差すようにする
				p = p2 + 1;
			}

			// host part を option の先頭に移動して、scheme part を潰す
			// port指定が無かった時にport番号を足すための領域確保の意味もある
			hostlen = strlen(p);
			memmove_s(option, optlen, p, hostlen);
			option[hostlen] = 0;

			// ポート指定が無い時は":22"を足す
			if (option[0] == '[' && option[hostlen-1] == ']' ||     // IPv6 raw address without port
			    option[0] != '[' && _mbschr(option, ':') == NULL) { // hostname or IPv4 raw address without port
				memcpy_s(option+hostlen, optlen-hostlen, ":22", 3);
				hostlen += 3;
			}

			// ポート指定より後をすべてスペースで潰す
			memset(option+hostlen, ' ', optlen-hostlen);

			pvar->settings.Enabled = 1;

			action = OPTION_REPLACE;
		}
		else if (_mbschr(option, '@') != NULL) {
			//
			// user@host 形式のサポート
			//   取り合えずsshでのみサポートの為、ユーザ名はttssh内で潰す。
			//   (ssh接続以外 --  ttsshには関係ない場合でも)
			//   将来的にtelnet authentication optionをサポートした時は
			//   Tera Term本体で処理するようにする予定。
			//
			char *p;
			p = _mbschr(option, '@');
			*p = 0;

			strncpy_s(pvar->ssh2_username, sizeof(pvar->ssh2_username), option, _TRUNCATE);

			// ユーザ名部分をスペースで潰す。
			// 後続のTTXやTera Term本体で解釈する時にはスペースを読み飛ばすので、
			// ホスト名を先頭に詰める必要は無い。
			memset(option, ' ', p-option+1);

			action = OPTION_REPLACE;
		}


		switch (action) {
			case OPTION_CLEAR:
				memset(cur, ' ', next-cur);
				break;
			case OPTION_REPLACE:
				memset(cur, ' ', next-cur);
				memcpy(cur+1, option, strlen(option));
				break;
		}

		cur = next;
	}

	free(option);

	FWDUI_load_settings(pvar);

	(pvar->ParseParam) (param, ts, DDETopic);
}

static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks)
{
	pvar->ReadIniFile = *hooks->ReadIniFile;
	pvar->WriteIniFile = *hooks->WriteIniFile;
	pvar->ParseParam = *hooks->ParseParam;

	*hooks->ReadIniFile = TTXReadINIFile;
	*hooks->WriteIniFile = TTXWriteINIFile;
	*hooks->ParseParam = TTXParseParam;
}

static void PASCAL TTXSetWinSize(int rows, int cols)
{
	SSH_notify_win_size(pvar, cols, rows);
}

static void insertMenuBeforeItem(HMENU menu, WORD beforeItemID, WORD flags,
                                 WORD newItemID, char *text)
{
	int i, j;

	for (i = GetMenuItemCount(menu) - 1; i >= 0; i--) {
		HMENU submenu = GetSubMenu(menu, i);

		for (j = GetMenuItemCount(submenu) - 1; j >= 0; j--) {
			if (GetMenuItemID(submenu, j) == beforeItemID) {
				InsertMenu(submenu, j, MF_BYPOSITION | flags, newItemID, text);
				return;
			}
		}
	}
}

#define GetFileMenu(menu)       GetSubMenuByChildID(menu, 50110) // ID_FILE_NEWCONNECTION
#define GetEditMenu(menu)       GetSubMenuByChildID(menu, 50210) // ID_EDIT_COPY2
#define GetSetupMenu(menu)      GetSubMenuByChildID(menu, 50310) // ID_SETUP_TERMINAL
#define GetControlMenu(menu)    GetSubMenuByChildID(menu, 50410) // ID_CONTROL_RESETTERMINAL
#define GetHelpMenu(menu)       GetSubMenuByChildID(menu, 50990) // ID_HELP_ABOUT

HMENU GetSubMenuByChildID(HMENU menu, UINT id) {
  int i, j, items, subitems, cur_id;
  HMENU m;

  items = GetMenuItemCount(menu);

  for (i=0; i<items; i++) {
    if (m = GetSubMenu(menu, i)) {
      subitems = GetMenuItemCount(m);
      for (j=0; j<subitems; j++) {
        cur_id = GetMenuItemID(m, j);
        if (cur_id == id) {
          return m;
        }
      }
    }
  }
  return NULL;
}

static void PASCAL TTXModifyMenu(HMENU menu)
{
	pvar->FileMenu = GetFileMenu(menu);

	/* inserts before ID_HELP_ABOUT */
	UTIL_get_lang_msg("MENU_ABOUT", pvar, "About &TTSSH...");
	insertMenuBeforeItem(menu, 50990, MF_ENABLED, ID_ABOUTMENU, pvar->ts->UIMsg);

	/* inserts before ID_SETUP_TCPIP */
	UTIL_get_lang_msg("MENU_SSH", pvar, "SS&H...");
	insertMenuBeforeItem(menu, 50360, MF_ENABLED, ID_SSHSETUPMENU, pvar->ts->UIMsg);
	/* inserts before ID_SETUP_TCPIP */
	UTIL_get_lang_msg("MENU_SSH_AUTH", pvar, "SSH &Authentication...");
	insertMenuBeforeItem(menu, 50360, MF_ENABLED, ID_SSHAUTHSETUPMENU, pvar->ts->UIMsg);
	/* inserts before ID_SETUP_TCPIP */
	UTIL_get_lang_msg("MENU_SSH_FORWARD", pvar, "SSH F&orwarding...");
	insertMenuBeforeItem(menu, 50360, MF_ENABLED, ID_SSHFWDSETUPMENU, pvar->ts->UIMsg);
	UTIL_get_lang_msg("MENU_SSH_KEYGEN", pvar, "SSH KeyGe&nerator...");
	insertMenuBeforeItem(menu, 50360, MF_ENABLED, ID_SSHKEYGENMENU, pvar->ts->UIMsg);

	/* inserts before ID_FILE_CHANGEDIR */
	UTIL_get_lang_msg("MENU_SSH_SCP", pvar, "SS&H SCP...");
	insertMenuBeforeItem(menu, 50170, MF_GRAYED, ID_SSHSCPMENU, pvar->ts->UIMsg);
}

static void PASCAL TTXModifyPopupMenu(HMENU menu) {
	if (menu == pvar->FileMenu) {
		if (pvar->cv->Ready && pvar->settings.Enabled)
			EnableMenuItem(menu, ID_SSHSCPMENU, MF_BYCOMMAND | MF_ENABLED);
		else
			EnableMenuItem(menu, ID_SSHSCPMENU, MF_BYCOMMAND | MF_GRAYED);
	}
}

static void about_dlg_set_abouttext(PTInstVar pvar, HWND dlg, digest_algorithm dgst_alg)
{
	char buf[1024], buf2[2048];
	char *fp = NULL;

	// TTSSHダイアログに表示するSSHに関する情報 (2004.10.30 yutaka)
	if (pvar->socket != INVALID_SOCKET) {
		buf2[0] = '\0';

		if (SSHv1(pvar)) {
			UTIL_get_lang_msg("DLG_ABOUT_SERVERID", pvar, "Server ID:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_server_ID_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_PROTOCOL", pvar, "Using protocol:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_protocol_version_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_ENCRYPTION", pvar, "Encryption:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			CRYPT_get_cipher_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_SERVERKEY", pvar, "Server keys:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			CRYPT_get_server_key_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_AUTH", pvar, "Authentication:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			AUTH_get_auth_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_COMP", pvar, "Compression:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_compression_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

		} else { // SSH2
			UTIL_get_lang_msg("DLG_ABOUT_SERVERID", pvar, "Server ID:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_server_ID_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_CLIENTID", pvar, "Client ID:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), pvar->client_version_string, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_PROTOCOL", pvar, "Using protocol:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_protocol_version_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_KEX", pvar, "Key exchange algorithm:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), get_kex_algorithm_name(pvar->kex_type), _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_HOSTKEY", pvar, "Host Key:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), get_ssh_keytype_name(pvar->hostkey_type), _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_ENCRYPTION", pvar, "Encryption:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			CRYPT_get_cipher_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_MAC", pvar, "MAC algorithm:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_mac_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			if (pvar->ctos_compression == COMP_DELAYED) { // 遅延パケット圧縮の場合 (2006.6.23 yutaka)
				UTIL_get_lang_msg("DLG_ABOUT_COMPDELAY", pvar, "Delayed Compression:");
			}
			else {
				UTIL_get_lang_msg("DLG_ABOUT_COMP", pvar, "Compression:");
			}
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_compression_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_KEXKEY", pvar, "Key exchange keys:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			CRYPT_get_server_key_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msg("DLG_ABOUT_AUTH", pvar, "Authentication:");
			strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			AUTH_get_auth_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);
		}

		// ホスト公開鍵のfingerprintを表示する。
		// (2014.5.1 yutaka)
		UTIL_get_lang_msg("DLG_ABOUT_FINGERPRINT", pvar, "Host key's fingerprint:");
		strncat_s(buf2, sizeof(buf2), pvar->ts->UIMsg, _TRUNCATE);
		strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

		switch (dgst_alg) {
		case SSH_DIGEST_MD5:
			fp = key_fingerprint(&pvar->hosts_state.hostkey, SSH_FP_HEX, dgst_alg);
			strncat_s(buf2, sizeof(buf2), fp, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);
			free(fp);
			break;
		case SSH_DIGEST_SHA256:
		default:
			fp = key_fingerprint(&pvar->hosts_state.hostkey, SSH_FP_BASE64, SSH_DIGEST_SHA256);
			if (fp != NULL) {
				strncat_s(buf2, sizeof(buf2), fp, _TRUNCATE);
				free(fp);
			}
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);
			break;
		}

		fp = key_fingerprint(&pvar->hosts_state.hostkey, SSH_FP_RANDOMART, dgst_alg);
		strncat_s(buf2, sizeof(buf2), fp, _TRUNCATE);
		free(fp);

		SendDlgItemMessage(dlg, IDC_ABOUTTEXT, WM_SETTEXT, 0, (LPARAM)(char *)buf2);
	}
}

static void init_about_dlg(PTInstVar pvar, HWND dlg)
{
	char buf[1024];
	char uimsg[MAX_UIMSG];

	GetWindowText(dlg, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_ABOUT_TITLE", pvar, uimsg);
	SetWindowText(dlg, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_FP_HASH_ALG, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_ABOUT_FP_HASH_ALGORITHM", pvar, uimsg);
	SetDlgItemText(dlg, IDC_FP_HASH_ALG, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDOK, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_OK", pvar, uimsg);
	SetDlgItemText(dlg, IDOK, pvar->ts->UIMsg);

	// TTSSHのバージョンを設定する (2005.2.28 yutaka)
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
	            "TTSSH\r\nTera Term Secure Shell extension, %d.%d", TTSSH_VERSION_MAJOR, TTSSH_VERSION_MINOR);
	SendMessage(GetDlgItem(dlg, IDC_TTSSH_VERSION), WM_SETTEXT, 0, (LPARAM)buf);

	// OpenSSLのバージョンを設定する (2005.1.24 yutaka)
	// 条件文追加 (2005.5.11 yutaka)
	// OPENSSL_VERSION_TEXT マクロ定義ではなく、関数を使ってバージョンを取得する。(2013.11.24 yutaka)
	SendMessage(GetDlgItem(dlg, IDC_OPENSSL_VERSION), WM_SETTEXT, 0, (LPARAM)SSLeay_version(SSLEAY_VERSION));

	// zlibのバージョンを設定する (2005.5.11 yutaka)
#ifdef ZLIB_VERSION
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "ZLib %s", ZLIB_VERSION);
#else
	_snprintf(buf, sizeof(buf), "ZLib Unknown");
#endif
	SendMessage(GetDlgItem(dlg, IDC_ZLIB_VERSION), WM_SETTEXT, 0, (LPARAM)buf);

	// PuTTYのバージョンを設定する (2011.7.26 yutaka)
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "PuTTY %s", putty_get_version());
	SendMessage(GetDlgItem(dlg, IDC_PUTTY_VERSION), WM_SETTEXT, 0, (LPARAM)buf);
}

// WM_MOUSEWHEEL は winuser.h ヘッダで宣言されていますが、#define _WIN32_WINNT 0x0400 が宣言されていないと認識されません。
#define WM_MOUSEWHEEL                   0x020A
#define WHEEL_DELTA                     120
#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#define GET_KEYSTATE_WPARAM(wParam)     (LOWORD(wParam))

static WNDPROC g_defAboutDlgEditWndProc;  // Edit Controlのサブクラス化用
static int g_deltaSumAboutDlg = 0;        // マウスホイールのDelta累積用

static LRESULT CALLBACK AboutDlgEditWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) 
{
	WORD keys;
	short delta;
	BOOL page;

	switch (msg) {
		case WM_KEYDOWN:
			// Edit control上で CTRL+A を押下すると、テキストを全選択する。
			if (wp == 'A' && GetKeyState(VK_CONTROL) < 0) {
				PostMessage(hWnd, EM_SETSEL, 0, -1);
				return 0;
			}
			break;

		case WM_MOUSEWHEEL:
			// CTRLorSHIFT + マウスホイールの場合、横スクロールさせる。
			keys = GET_KEYSTATE_WPARAM(wp);
			delta = GET_WHEEL_DELTA_WPARAM(wp);
			page = keys & (MK_CONTROL | MK_SHIFT);

			if (page == 0)
				break;

			g_deltaSumAboutDlg += delta;

			if (g_deltaSumAboutDlg >= WHEEL_DELTA) {
				g_deltaSumAboutDlg -= WHEEL_DELTA;
				SendMessage(hWnd, WM_HSCROLL, SB_PAGELEFT , 0);
			} else if (g_deltaSumAboutDlg <= -WHEEL_DELTA) {
				g_deltaSumAboutDlg += WHEEL_DELTA;
				SendMessage(hWnd, WM_HSCROLL, SB_PAGERIGHT, 0);
			}

			break;
	}
    return CallWindowProc(g_defAboutDlgEditWndProc, hWnd, msg, wp, lp);
}

static BOOL CALLBACK TTXAboutDlg(HWND dlg, UINT msg, WPARAM wParam,
                                 LPARAM lParam)
{
	LOGFONT logfont;
	HFONT font;

	switch (msg) {
	case WM_INITDIALOG:
		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgAboutFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_TTSSH_VERSION, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHVERSIONS, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_INCLUDES, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_OPENSSL_VERSION, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_ZLIB_VERSION, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_PUTTY_VERSION, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE, 0));
			SendDlgItemMessage(dlg, IDC_WEBSITES, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE, 0));
			SendDlgItemMessage(dlg, IDC_CRYPTOGRAPHY, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CREDIT, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_FP_HASH_ALG, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE, 0));
			SendDlgItemMessage(dlg, IDC_FP_HASH_ALG_MD5, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE, 0));
			SendDlgItemMessage(dlg, IDC_FP_HASH_ALG_SHA256, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE, 0));
			SendDlgItemMessage(dlg, IDOK, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgAboutFont = NULL;
		}

		// Edit controlは等幅フォントで表示したいので、別設定情報からフォントをセットする。
		// (2014.5.5. yutaka)
		if (UTIL_get_lang_font("DLG_ABOUT_FONT", dlg, &logfont, &DlgAboutTextFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_ABOUTTEXT, WM_SETFONT, (WPARAM)DlgAboutTextFont, MAKELPARAM(TRUE,0));
		} else {
			// 読み込めなかった場合は等幅フォントを指定する。
			// エディットコントロールはダイアログと同じフォントを持っており
			// 等幅フォントではないため。
			strncpy_s(logfont.lfFaceName, sizeof(logfont.lfFaceName), "Courier New", _TRUNCATE);
			logfont.lfCharSet = 0;
			logfont.lfHeight = MulDiv(8, GetDeviceCaps(GetDC(dlg),LOGPIXELSY) * -1, 72);
			logfont.lfWidth = 0;
			if ((DlgAboutTextFont = CreateFontIndirect(&logfont)) != NULL) {
				SendDlgItemMessage(dlg, IDC_ABOUTTEXT, WM_SETFONT, (WPARAM)DlgAboutTextFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgAboutTextFont = NULL;
			}
		}

		// アイコンを動的にセット
		{
			int fuLoad = LR_DEFAULTCOLOR;
			HICON hicon;

			if (IsWindowsNT4()) {
				fuLoad = LR_VGACOLOR;
			}

			hicon = LoadImage(hInst, MAKEINTRESOURCE(pvar->settings.IconID),
			                  IMAGE_ICON, 32, 32, fuLoad);
			SendDlgItemMessage(dlg, IDC_TTSSH_ICON, STM_SETICON, (WPARAM)hicon, 0);
		}

		init_about_dlg(pvar, dlg);
		CheckDlgButton(dlg, IDC_FP_HASH_ALG_SHA256, TRUE);
		about_dlg_set_abouttext(pvar, dlg, SSH_DIGEST_SHA256);
		SetFocus(GetDlgItem(dlg, IDOK));

		// Edit controlをサブクラス化する。
		g_deltaSumAboutDlg = 0;
		g_defAboutDlgEditWndProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(dlg, IDC_ABOUTTEXT), GWLP_WNDPROC, (LONG_PTR)AboutDlgEditWindowProc);

		return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(dlg, 1);
			if (DlgAboutFont != NULL) {
				DeleteObject(DlgAboutFont);
			}
			if (DlgAboutTextFont != NULL) {
				DeleteObject(DlgAboutTextFont);
			}
			return TRUE;
		case IDCANCEL:			/* there isn't a cancel button, but other Windows
								   UI things can send this message */
			EndDialog(dlg, 0);
			if (DlgAboutFont != NULL) {
				DeleteObject(DlgAboutFont);
			}
			if (DlgAboutTextFont != NULL) {
				DeleteObject(DlgAboutTextFont);
			}
			return TRUE;
		case IDC_FP_HASH_ALG_MD5:
			about_dlg_set_abouttext(pvar, dlg, SSH_DIGEST_MD5);
			return TRUE;
		case IDC_FP_HASH_ALG_SHA256:
			about_dlg_set_abouttext(pvar, dlg, SSH_DIGEST_SHA256);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

static char *get_cipher_name(int cipher)
{
	switch (cipher) {
	case SSH_CIPHER_NONE:
		UTIL_get_lang_msg("DLG_SSHSETUP_CIPHER_BORDER", pvar,
		                  "<ciphers below this line are disabled>");
		return pvar->ts->UIMsg;
	case SSH_CIPHER_3DES:
		return "3DES(SSH1)";
	case SSH_CIPHER_DES:
		return "DES(SSH1)";
	case SSH_CIPHER_BLOWFISH:
		return "Blowfish(SSH1)";

	// for SSH2(yutaka)
	case SSH2_CIPHER_AES128_CBC:
		return "AES128-CBC(SSH2)";
	case SSH2_CIPHER_AES192_CBC:
		return "AES192-CBC(SSH2)";
	case SSH2_CIPHER_AES256_CBC:
		return "AES256-CBC(SSH2)";
	case SSH2_CIPHER_3DES_CBC:
		return "3DES-CBC(SSH2)";
	case SSH2_CIPHER_BLOWFISH_CBC:
		return "Blowfish-CBC(SSH2)";
	case SSH2_CIPHER_AES128_CTR:
		return "AES128-CTR(SSH2)";
	case SSH2_CIPHER_AES192_CTR:
		return "AES192-CTR(SSH2)";
	case SSH2_CIPHER_AES256_CTR:
		return "AES256-CTR(SSH2)";
	case SSH2_CIPHER_ARCFOUR:
		return "Arcfour(SSH2)";
	case SSH2_CIPHER_ARCFOUR128:
		return "Arcfour128(SSH2)";
	case SSH2_CIPHER_ARCFOUR256:
		return "Arcfour256(SSH2)";
	case SSH2_CIPHER_CAST128_CBC:
		return "CAST128-CBC(SSH2)";
	case SSH2_CIPHER_3DES_CTR:
		return "3DES-CTR(SSH2)";
	case SSH2_CIPHER_BLOWFISH_CTR:
		return "Blowfish-CTR(SSH2)";
	case SSH2_CIPHER_CAST128_CTR:
		return "CAST128-CTR(SSH2)";
	case SSH2_CIPHER_CAMELLIA128_CBC:
		return "Camellia128-CBC(SSH2)";
	case SSH2_CIPHER_CAMELLIA192_CBC:
		return "Camellia192-CBC(SSH2)";
	case SSH2_CIPHER_CAMELLIA256_CBC:
		return "Camellia256-CBC(SSH2)";
	case SSH2_CIPHER_CAMELLIA128_CTR:
		return "Camellia128-CTR(SSH2)";
	case SSH2_CIPHER_CAMELLIA192_CTR:
		return "Camellia192-CTR(SSH2)";
	case SSH2_CIPHER_CAMELLIA256_CTR:
		return "Camellia256-CTR(SSH2)";

	default:
		return NULL;
	}
}

static void set_move_button_status(HWND dlg, int type, int up, int down)
{
	HWND cipherControl = GetDlgItem(dlg, type);
	int curPos = (int) SendMessage(cipherControl, LB_GETCURSEL, 0, 0);
	int maxPos = (int) SendMessage(cipherControl, LB_GETCOUNT, 0, 0) - 1;

	EnableWindow(GetDlgItem(dlg, up),
	             curPos > 0 && curPos <= maxPos);
	EnableWindow(GetDlgItem(dlg, down),
	             curPos >= 0 && curPos < maxPos);
}

static void init_setup_dlg(PTInstVar pvar, HWND dlg)
{
	HWND compressionControl = GetDlgItem(dlg, IDC_SSHCOMPRESSIONLEVEL);
	HWND cipherControl = GetDlgItem(dlg, IDC_SSHCIPHERPREFS);
	HWND kexControl = GetDlgItem(dlg, IDC_SSHKEX_LIST);
	HWND hostkeyControl = GetDlgItem(dlg, IDC_SSHHOST_KEY_LIST);
	HWND macControl = GetDlgItem(dlg, IDC_SSHMAC_LIST);
	HWND compControl = GetDlgItem(dlg, IDC_SSHCOMP_LIST);
	HWND hostkeyRotationControl = GetDlgItem(dlg, IDC_HOSTKEY_ROTATION_STATIC);
	HWND hostkeyRotationControlList = GetDlgItem(dlg, IDC_HOSTKEY_ROTATION_COMBO);
	int i;
	int ch;
	char uimsg[MAX_UIMSG];
	char *rotationItem[SSH_UPDATE_HOSTKEYS_MAX] = {
		"No",
		"Yes",
		"Ask",
	};
	char *rotationItemKey[SSH_UPDATE_HOSTKEYS_MAX] = {
		"DLG_SSHSETUP_HOSTKEY_ROTATION_NO",
		"DLG_SSHSETUP_HOSTKEY_ROTATION_YES",
		"DLG_SSHSETUP_HOSTKEY_ROTATION_ASK",
	};

	GetWindowText(dlg, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_TITLE", pvar, uimsg);
	SetWindowText(dlg, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_COMPRESSLABEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_COMPRESS", pvar, uimsg);
	SetDlgItemText(dlg, IDC_COMPRESSLABEL, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_COMPRESSNONE, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_COMPRESS_NONE", pvar, uimsg);
	SetDlgItemText(dlg, IDC_COMPRESSNONE, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_COMPRESSHIGH, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_COMPRESS_HIGHEST", pvar, uimsg);
	SetDlgItemText(dlg, IDC_COMPRESSHIGH, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_COMPRESSNOTE, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_COMPRESS_NOTE", pvar, uimsg);
	SetDlgItemText(dlg, IDC_COMPRESSNOTE, pvar->ts->UIMsg);

	GetDlgItemText(dlg, IDC_CIPHERORDER, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_CIPHER", pvar, uimsg);
	SetDlgItemText(dlg, IDC_CIPHERORDER, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHMOVECIPHERUP, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_CIPHER_UP", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHMOVECIPHERUP, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHMOVECIPHERDOWN, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_CIPHER_DOWN", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHMOVECIPHERDOWN, pvar->ts->UIMsg);

	GetDlgItemText(dlg, IDC_KEX_ORDER, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_KEX", pvar, uimsg);
	SetDlgItemText(dlg, IDC_KEX_ORDER, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHKEX_MOVEUP, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_KEX_UP", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHKEX_MOVEUP, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHKEX_MOVEDOWN, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_KEX_DOWN", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHKEX_MOVEDOWN, pvar->ts->UIMsg);

	GetDlgItemText(dlg, IDC_HOST_KEY_ORDER, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_HOST_KEY", pvar, uimsg);
	SetDlgItemText(dlg, IDC_HOST_KEY_ORDER, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHHOST_KEY_MOVEUP, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_HOST_KEY_UP", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHHOST_KEY_MOVEUP, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHHOST_KEY_MOVEDOWN, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_HOST_KEY_DOWN", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHHOST_KEY_MOVEDOWN, pvar->ts->UIMsg);

	GetDlgItemText(dlg, IDC_MAC_ORDER, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_MAC", pvar, uimsg);
	SetDlgItemText(dlg, IDC_MAC_ORDER, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHMAC_MOVEUP, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_MAC_UP", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHMAC_MOVEUP, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHMAC_MOVEDOWN, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_MAC_DOWN", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHMAC_MOVEDOWN, pvar->ts->UIMsg);

	GetDlgItemText(dlg, IDC_COMP_ORDER, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_COMP", pvar, uimsg);
	SetDlgItemText(dlg, IDC_COMP_ORDER, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHCOMP_MOVEUP, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_COMP_UP", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHCOMP_MOVEUP, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHCOMP_MOVEDOWN, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_COMP_DOWN", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHCOMP_MOVEDOWN, pvar->ts->UIMsg);

	GetDlgItemText(dlg, IDC_KNOWNHOSTS, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_KNOWNHOST", pvar, uimsg);
	SetDlgItemText(dlg, IDC_KNOWNHOSTS, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_CHOOSEREADWRITEFILE, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_KNOWNHOST_RW", pvar, uimsg);
	SetDlgItemText(dlg, IDC_CHOOSEREADWRITEFILE, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_CHOOSEREADONLYFILE, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_KNOWNHOST_RO", pvar, uimsg);
	SetDlgItemText(dlg, IDC_CHOOSEREADONLYFILE, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_HEARTBEATLABEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_HEARTBEAT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_HEARTBEATLABEL, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_HEARTBEATLABEL2, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_HEARTBEAT_UNIT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_HEARTBEATLABEL2, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_REMEMBERPASSWORD, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_PASSWORD", pvar, uimsg);
	SetDlgItemText(dlg, IDC_REMEMBERPASSWORD, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_FORWARDAGENT, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_FORWARDAGENT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_FORWARDAGENT, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_FORWARDAGENTCONFIRM, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_FORWARDAGENTCONFIRM", pvar, uimsg);
	SetDlgItemText(dlg, IDC_FORWARDAGENTCONFIRM, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_FORWARDAGENTNOTIFY, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_FORWARDAGENTNOTIFY", pvar, uimsg);
	SetDlgItemText(dlg, IDC_FORWARDAGENTNOTIFY, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_VERIFYHOSTKEYDNS, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_VERIFYHOSTKEYDNS", pvar, uimsg);
	SetDlgItemText(dlg, IDC_VERIFYHOSTKEYDNS, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_NOTICEBANNER, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_NOTICE", pvar, uimsg);
	SetDlgItemText(dlg, IDC_NOTICEBANNER, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDOK, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_OK", pvar, uimsg);
	SetDlgItemText(dlg, IDOK, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_CANCEL", pvar, uimsg);
	SetDlgItemText(dlg, IDCANCEL, pvar->ts->UIMsg);

	GetDlgItemText(dlg, IDC_HOSTKEY_ROTATION_STATIC, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_SSHSETUP_HOSTKEY_ROTATION", pvar, uimsg);
	SetDlgItemText(dlg, IDC_HOSTKEY_ROTATION_STATIC, pvar->ts->UIMsg);

	SendMessage(compressionControl, TBM_SETRANGE, TRUE, MAKELONG(0, 9));
	SendMessage(compressionControl, TBM_SETPOS, TRUE,
	            pvar->settings.CompressionLevel);

	// Cipher order
	normalize_cipher_order(pvar->settings.CipherOrder);

	for (i = 0; pvar->settings.CipherOrder[i] != 0; i++) {
		int cipher = pvar->settings.CipherOrder[i] - '0';
		char *name = get_cipher_name(cipher);

		if (name != NULL) {
			SendMessage(cipherControl, LB_ADDSTRING, 0, (LPARAM) name);
		}
	}

	SendMessage(cipherControl, LB_SETCURSEL, 0, 0);
	set_move_button_status(dlg, IDC_SSHCIPHERPREFS, IDC_SSHMOVECIPHERUP, IDC_SSHMOVECIPHERDOWN);

	// KEX order
	normalize_kex_order(pvar->settings.KexOrder);
	for (i = 0; pvar->settings.KexOrder[i] != 0; i++) {
		int index = pvar->settings.KexOrder[i] - '0';
		char *name = NULL;

		if (index == 0)	{
			UTIL_get_lang_msg("DLG_SSHSETUP_KEX_BORDER", pvar,
							  "<KEXs below this line are disabled>");
			name = pvar->ts->UIMsg;
		} else {
			name = get_kex_algorithm_name(index);
		}

		if (name != NULL) {
			SendMessage(kexControl, LB_ADDSTRING, 0, (LPARAM) name);
		}
	}
	SendMessage(kexControl, LB_SETCURSEL, 0, 0);
	set_move_button_status(dlg, IDC_SSHKEX_LIST, IDC_SSHKEX_MOVEUP, IDC_SSHKEX_MOVEDOWN);

	// Host Key order
	normalize_host_key_order(pvar->settings.HostKeyOrder);
	for (i = 0; pvar->settings.HostKeyOrder[i] != 0; i++) {
		int index = pvar->settings.HostKeyOrder[i] - '0';
		char *name = NULL;

		if (index == 0)	{
			UTIL_get_lang_msg("DLG_SSHSETUP_HOST_KEY_BORDER", pvar,
							  "<Host Keys below this line are disabled>");
			name = pvar->ts->UIMsg;
		} else {
			name = get_ssh_keytype_name(index);
		}

		if (name != NULL) {
			SendMessage(hostkeyControl, LB_ADDSTRING, 0, (LPARAM) name);
		}
	}
	SendMessage(hostkeyControl, LB_SETCURSEL, 0, 0);
	set_move_button_status(dlg, IDC_SSHHOST_KEY_LIST, IDC_SSHHOST_KEY_MOVEUP, IDC_SSHHOST_KEY_MOVEDOWN);

	// MAC order
	normalize_mac_order(pvar->settings.MacOrder);
	for (i = 0; pvar->settings.MacOrder[i] != 0; i++) {
		int index = pvar->settings.MacOrder[i] - '0';
		char *name = NULL;

		if (index == 0)	{
			UTIL_get_lang_msg("DLG_SSHSETUP_MAC_BORDER", pvar,
							  "<MACs below this line are disabled>");
			name = pvar->ts->UIMsg;
		} else {
			name = get_ssh2_mac_name(index);
		}

		if (name != NULL) {
			SendMessage(macControl, LB_ADDSTRING, 0, (LPARAM) name);
		}
	}
	SendMessage(macControl, LB_SETCURSEL, 0, 0);
	set_move_button_status(dlg, IDC_SSHMAC_LIST, IDC_SSHMAC_MOVEUP, IDC_SSHMAC_MOVEDOWN);

	// Compression order
	normalize_comp_order(pvar->settings.CompOrder);
	for (i = 0; pvar->settings.CompOrder[i] != 0; i++) {
		int index = pvar->settings.CompOrder[i] - '0';
		char *name = NULL;

		if (index == 0)	{
			UTIL_get_lang_msg("DLG_SSHSETUP_COMP_BORDER", pvar,
							  "<Compression methods below this line are disabled>");
			name = pvar->ts->UIMsg;
		} else {
			name = get_ssh2_comp_name(index);
		}

		if (name != NULL) {
			SendMessage(compControl, LB_ADDSTRING, 0, (LPARAM) name);
		}
	}
	SendMessage(compControl, LB_SETCURSEL, 0, 0);
	set_move_button_status(dlg, IDC_SSHCOMP_LIST, IDC_SSHCOMP_MOVEUP, IDC_SSHCOMP_MOVEDOWN);

	for (i = 0; (ch = pvar->settings.KnownHostsFiles[i]) != 0 && ch != ';';
		 i++) {
	}
	if (ch != 0) {
		pvar->settings.KnownHostsFiles[i] = 0;
		SetDlgItemText(dlg, IDC_READWRITEFILENAME,
		               pvar->settings.KnownHostsFiles);
		pvar->settings.KnownHostsFiles[i] = ch;
		SetDlgItemText(dlg, IDC_READONLYFILENAME,
		               pvar->settings.KnownHostsFiles + i + 1);
	} else {
		SetDlgItemText(dlg, IDC_READWRITEFILENAME,
		               pvar->settings.KnownHostsFiles);
	}

	// SSH2 HeartBeat(keep-alive)を追加 (2005.2.22 yutaka)
	{
		char buf[10];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            "%d", pvar->settings.ssh_heartbeat_overtime);
		SetDlgItemText(dlg, IDC_HEARTBEAT_EDIT, buf);
	}

	if (pvar->settings.remember_password) {
		CheckDlgButton(dlg, IDC_REMEMBERPASSWORD, TRUE);
	}
	if (pvar->settings.ForwardAgent) {
		CheckDlgButton(dlg, IDC_FORWARDAGENT, TRUE);
	}
	else {
		EnableWindow(GetDlgItem(dlg, IDC_FORWARDAGENTCONFIRM), FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_FORWARDAGENTNOTIFY), FALSE);
	}
	if (pvar->settings.ForwardAgentConfirm) {
		CheckDlgButton(dlg, IDC_FORWARDAGENTCONFIRM, TRUE);
	}

	if (pvar->settings.ForwardAgentNotify) {
		CheckDlgButton(dlg, IDC_FORWARDAGENTNOTIFY, TRUE);
	}
	if (!HasBalloonTipSupport()) {
		EnableWindow(GetDlgItem(dlg, IDC_FORWARDAGENTNOTIFY), FALSE);
	}

	if (pvar->settings.VerifyHostKeyDNS) {
		CheckDlgButton(dlg, IDC_VERIFYHOSTKEYDNS, TRUE);
	}

	// hostkey rotation(OpenSSH 6.8)
	for (i = 0; i < SSH_UPDATE_HOSTKEYS_MAX; i++) {
		UTIL_get_lang_msg(rotationItemKey[i], pvar, rotationItem[i]);
		SendMessage(hostkeyRotationControlList, CB_INSERTSTRING, i, (LPARAM)pvar->ts->UIMsg);
	}
	ch = pvar->settings.UpdateHostkeys;
	if (!(ch >= 0 && ch < SSH_UPDATE_HOSTKEYS_MAX))
		ch = 0;
	SendMessage(hostkeyRotationControlList, CB_SETCURSEL, ch, 0);

}

void get_teraterm_dir_relative_name(char *buf, int bufsize,
                                    char *basename)
{
	int filename_start = 0;
	int i;
	int ch;

	if (basename[0] == '\\' || basename[0] == '/'
	 || (basename[0] != 0 && basename[1] == ':')) {
		strncpy_s(buf, bufsize, basename, _TRUNCATE);
		return;
	}

	GetModuleFileName(NULL, buf, bufsize);
	for (i = 0; (ch = buf[i]) != 0; i++) {
		if (ch == '\\' || ch == '/' || ch == ':') {
			filename_start = i + 1;
		}
	}

	if (bufsize > filename_start) {
		strncpy_s(buf + filename_start, bufsize - filename_start, basename, _TRUNCATE);
	}
}

int copy_teraterm_dir_relative_path(char *dest, int destsize,
                                    char *basename)
{
	char buf[1024];
	int filename_start = 0;
	int i;
	int ch, ch2;

	if (basename[0] != '\\' && basename[0] != '/'
	 && (basename[0] == 0 || basename[1] != ':')) {
		strncpy_s(dest, destsize, basename, _TRUNCATE);
		return strlen(dest);
	}

	GetModuleFileName(NULL, buf, sizeof(buf));
	for (i = 0; (ch = buf[i]) != 0; i++) {
		if (ch == '\\' || ch == '/' || ch == ':') {
			filename_start = i + 1;
		}
	}

	for (i = 0; i < filename_start; i++) {
		ch = toupper(buf[i]);
		ch2 = toupper(basename[i]);

		if (ch == ch2 ||
		    ((ch == '\\' || ch == '/') && (ch2 == '\\' || ch2 == '/'))) {
		} else {
			break;
		}
	}

	if (i == filename_start) {
		strncpy_s(dest, destsize, basename + i, _TRUNCATE);
	} else {
		strncpy_s(dest, destsize, basename, _TRUNCATE);
	}
	return strlen(dest);
}

static void complete_setup_dlg(PTInstVar pvar, HWND dlg)
{
	char buf[4096];
	char buf2[1024];
	HWND compressionControl = GetDlgItem(dlg, IDC_SSHCOMPRESSIONLEVEL);
	HWND cipherControl;
	int i, j, buf2index, bufindex;
	int count;

	pvar->settings.CompressionLevel =
		(int) SendMessage(compressionControl, TBM_GETPOS, 0, 0);

	// Cipher order
	cipherControl = GetDlgItem(dlg, IDC_SSHCIPHERPREFS);
	count = (int) SendMessage(cipherControl, LB_GETCOUNT, 0, 0);
	buf2index = 0;
	for (i = 0; i < count; i++) {
		int len = SendMessage(cipherControl, LB_GETTEXTLEN, i, 0);

		if (len > 0 && len < sizeof(buf)) {	/* should always be true */
			buf[0] = 0;
			SendMessage(cipherControl, LB_GETTEXT, i, (LPARAM) buf);
			for (j = 0; j <= SSH_CIPHER_MAX; j++) {
				char *cipher_name = get_cipher_name(j);
				if (cipher_name != NULL && strcmp(buf, cipher_name) == 0) {
					break;
				}
			}
			if (j <= SSH_CIPHER_MAX) {
				buf2[buf2index] = '0' + j;
				buf2index++;
			}
		}
	}
	buf2[buf2index] = 0;
	normalize_cipher_order(buf2);
	strncpy_s(pvar->settings.CipherOrder, sizeof(pvar->settings.CipherOrder), buf2, _TRUNCATE);

	// KEX order
	cipherControl = GetDlgItem(dlg, IDC_SSHKEX_LIST);
	count = (int) SendMessage(cipherControl, LB_GETCOUNT, 0, 0);
	buf2index = 0;
	for (i = 0; i < count; i++) {
		int len = SendMessage(cipherControl, LB_GETTEXTLEN, i, 0);

		if (len > 0 && len < sizeof(buf)) {	/* should always be true */
			buf[0] = 0;
			SendMessage(cipherControl, LB_GETTEXT, i, (LPARAM) buf);
			for (j = 0;
				 j <= KEX_DH_MAX
				 && strcmp(buf, get_kex_algorithm_name(j)) != 0; j++) {
			}
			if (j <= KEX_DH_MAX) {
				buf2[buf2index] = '0' + j;
				buf2index++;
			} else {
				buf2[buf2index] = '0';  // disabled line
				buf2index++;
			}
		}
	}
	buf2[buf2index] = 0;
	normalize_kex_order(buf2);
	strncpy_s(pvar->settings.KexOrder, sizeof(pvar->settings.KexOrder), buf2, _TRUNCATE);

	// Host Key order
	cipherControl = GetDlgItem(dlg, IDC_SSHHOST_KEY_LIST);
	count = (int) SendMessage(cipherControl, LB_GETCOUNT, 0, 0);
	buf2index = 0;
	for (i = 0; i < count; i++) {
		int len = SendMessage(cipherControl, LB_GETTEXTLEN, i, 0);

		if (len > 0 && len < sizeof(buf)) {	/* should always be true */
			buf[0] = 0;
			SendMessage(cipherControl, LB_GETTEXT, i, (LPARAM) buf);
			for (j = 0;
				j <= KEY_MAX
				 && strcmp(buf, get_ssh_keytype_name(j)) != 0; j++) {
			}
			if (j <= KEY_MAX) {
				buf2[buf2index] = '0' + j;
				buf2index++;
			} else {
				buf2[buf2index] = '0';  // disabled line
				buf2index++;
			}
		}
	}
	buf2[buf2index] = 0;
	normalize_host_key_order(buf2);
	strncpy_s(pvar->settings.HostKeyOrder, sizeof(pvar->settings.HostKeyOrder), buf2, _TRUNCATE);

	// MAC order
	cipherControl = GetDlgItem(dlg, IDC_SSHMAC_LIST);
	count = (int) SendMessage(cipherControl, LB_GETCOUNT, 0, 0);
	buf2index = 0;
	for (i = 0; i < count; i++) {
		int len = SendMessage(cipherControl, LB_GETTEXTLEN, i, 0);

		if (len > 0 && len < sizeof(buf)) {	/* should always be true */
			buf[0] = 0;
			SendMessage(cipherControl, LB_GETTEXT, i, (LPARAM) buf);
			for (j = 0;
				j <= HMAC_MAX
				&& strcmp(buf, get_ssh2_mac_name(j)) != 0; j++) {
			}
			if (j <= HMAC_MAX) {
				buf2[buf2index] = '0' + j;
				buf2index++;
			} else {
				buf2[buf2index] = '0';  // disabled line
				buf2index++;
			}
		}
	}
	buf2[buf2index] = 0;
	normalize_mac_order(buf2);
	strncpy_s(pvar->settings.MacOrder, sizeof(pvar->settings.MacOrder), buf2, _TRUNCATE);

	// Compression order
	cipherControl = GetDlgItem(dlg, IDC_SSHCOMP_LIST);
	count = (int) SendMessage(cipherControl, LB_GETCOUNT, 0, 0);
	buf2index = 0;
	for (i = 0; i < count; i++) {
		int len = SendMessage(cipherControl, LB_GETTEXTLEN, i, 0);

		if (len > 0 && len < sizeof(buf)) {	/* should always be true */
			buf[0] = 0;
			SendMessage(cipherControl, LB_GETTEXT, i, (LPARAM) buf);
			for (j = 0;
				j <= COMP_MAX
				&& strcmp(buf, get_ssh2_comp_name(j)) != 0; j++) {
			}
			if (j <= COMP_MAX) {
				buf2[buf2index] = '0' + j;
				buf2index++;
			} else {
				buf2[buf2index] = '0';  // disabled line
				buf2index++;
			}
		}
	}
	buf2[buf2index] = 0;
	normalize_comp_order(buf2);
	strncpy_s(pvar->settings.CompOrder, sizeof(pvar->settings.CompOrder), buf2, _TRUNCATE);

	buf[0] = 0;
	GetDlgItemText(dlg, IDC_READWRITEFILENAME, buf, sizeof(buf));
	j = copy_teraterm_dir_relative_path(pvar->settings.KnownHostsFiles,
	                                    sizeof(pvar->settings.KnownHostsFiles),
	                                    buf);
	buf[0] = 0;
	bufindex = 0;
	GetDlgItemText(dlg, IDC_READONLYFILENAME, buf, sizeof(buf));
	for (i = 0; buf[i] != 0; i++) {
		if (buf[i] == ';') {
			buf[i] = 0;
			if (j < sizeof(pvar->settings.KnownHostsFiles) - 1) {
				pvar->settings.KnownHostsFiles[j] = ';';
				j++;
				j += copy_teraterm_dir_relative_path(pvar->settings.
													 KnownHostsFiles + j,
													 sizeof(pvar->settings.
															KnownHostsFiles)
													 - j, buf + bufindex);
			}
			bufindex = i + 1;
		}
	}
	if (bufindex < i && j < sizeof(pvar->settings.KnownHostsFiles) - 1) {
		pvar->settings.KnownHostsFiles[j] = ';';
		j++;
		copy_teraterm_dir_relative_path(pvar->settings.KnownHostsFiles + j,
		                                sizeof(pvar->settings. KnownHostsFiles) - j,
		                                buf + bufindex);
	}

	// get SSH HeartBeat(keep-alive)
	SendMessage(GetDlgItem(dlg, IDC_HEARTBEAT_EDIT), WM_GETTEXT, sizeof(buf), (LPARAM)buf);
	i = atoi(buf);
	if (i < 0)
		i = 60;
	pvar->settings.ssh_heartbeat_overtime = i;

	pvar->settings.remember_password = IsDlgButtonChecked(dlg, IDC_REMEMBERPASSWORD);
	pvar->settings.ForwardAgent = IsDlgButtonChecked(dlg, IDC_FORWARDAGENT);
	pvar->settings.ForwardAgentConfirm = IsDlgButtonChecked(dlg, IDC_FORWARDAGENTCONFIRM);
	pvar->settings.ForwardAgentNotify = IsDlgButtonChecked(dlg, IDC_FORWARDAGENTNOTIFY);
	pvar->settings.VerifyHostKeyDNS = IsDlgButtonChecked(dlg, IDC_VERIFYHOSTKEYDNS);

	// hostkey rotation(OpenSSH 6.8)
	i = SendMessage(GetDlgItem(dlg, IDC_HOSTKEY_ROTATION_COMBO), CB_GETCURSEL, 0, 0);
	if (!(i >= 0 && i < SSH_UPDATE_HOSTKEYS_MAX))
		i = 0;
	pvar->settings.UpdateHostkeys = i;
}

static void move_cur_sel_delta(HWND listbox, int delta)
{
	int curPos = (int) SendMessage(listbox, LB_GETCURSEL, 0, 0);
	int maxPos = (int) SendMessage(listbox, LB_GETCOUNT, 0, 0) - 1;
	int newPos = curPos + delta;
	char buf[1024];

	if (curPos >= 0 && newPos >= 0 && newPos <= maxPos) {
		int len = SendMessage(listbox, LB_GETTEXTLEN, curPos, 0);

		if (len > 0 && len < sizeof(buf)) {	/* should always be true */
			buf[0] = 0;
			SendMessage(listbox, LB_GETTEXT, curPos, (LPARAM) buf);
			SendMessage(listbox, LB_DELETESTRING, curPos, 0);
			SendMessage(listbox, LB_INSERTSTRING, newPos,
			            (LPARAM) (char *) buf);
			SendMessage(listbox, LB_SETCURSEL, newPos, 0);
		}
	}
}

static int get_keys_file_name(HWND parent, char *buf, int bufsize,
                              int readonly)
{
	OPENFILENAME params;
	char fullname_buf[2048] = "ssh_known_hosts";

	params.lStructSize = sizeof(OPENFILENAME);
	params.hwndOwner = parent;
	params.lpstrFilter = NULL;
	params.lpstrCustomFilter = NULL;
	params.nFilterIndex = 0;
	buf[0] = 0;
	params.lpstrFile = fullname_buf;
	params.nMaxFile = sizeof(fullname_buf);
	params.lpstrFileTitle = NULL;
	params.lpstrInitialDir = NULL;
	if (readonly) {
		UTIL_get_lang_msg("MSG_OPEN_KNOWNHOSTS_RO_TITLE", pvar,
		                  "Choose a read-only known-hosts file to add");
	}
	else {
		UTIL_get_lang_msg("MSG_OPEN_KNOWNHOSTS_RW_TITLE", pvar,
		                  "Choose a read/write known-hosts file");
	}
	params.lpstrTitle = pvar->ts->UIMsg;
	params.Flags = (readonly ? OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST : 0)
	             | OFN_HIDEREADONLY | (!readonly ? OFN_NOREADONLYRETURN : 0);
	params.lpstrDefExt = NULL;

	if (GetOpenFileName(&params) != 0) {
		copy_teraterm_dir_relative_path(buf, bufsize, fullname_buf);
		return 1;
	} else {
		int err = CommDlgExtendedError();

		if (err != 0) {
			char buf[1024];
			UTIL_get_lang_msg("MSG_OPEN_FILEDLG_KNOWNHOSTS_ERROR", pvar,
			                  "Unable to display file dialog box: error %d");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, pvar->ts->UIMsg, err);
			MessageBox(parent, buf, "TTSSH Error",
			           MB_OK | MB_ICONEXCLAMATION);
		}

		return 0;
	}
}

static void choose_read_write_file(HWND dlg)
{
	char buf[1024];

	if (get_keys_file_name(dlg, buf, sizeof(buf), 0)) {
		SetDlgItemText(dlg, IDC_READWRITEFILENAME, buf);
	}
}

static void choose_read_only_file(HWND dlg)
{
	char buf[1024];
	char buf2[4096];

	if (get_keys_file_name(dlg, buf, sizeof(buf), 1)) {
		buf2[0] = 0;
		GetDlgItemText(dlg, IDC_READONLYFILENAME, buf2, sizeof(buf2));
		if (buf2[0] != 0 && buf2[strlen(buf2) - 1] != ';') {
			strncat_s(buf2, sizeof(buf2), ";", _TRUNCATE);
		}
		strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
		SetDlgItemText(dlg, IDC_READONLYFILENAME, buf2);
	}
}

static BOOL CALLBACK TTXSetupDlg(HWND dlg, UINT msg, WPARAM wParam,
                                 LPARAM lParam)
{
	LOGFONT logfont;
	HFONT font;

	switch (msg) {
	case WM_INITDIALOG:
		SetWindowLong(dlg, DWL_USER, lParam);
		init_setup_dlg((PTInstVar) lParam, dlg);

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgSetupFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_COMPRESSLABEL, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_COMPRESSNONE, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_COMPRESSHIGH, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_COMPRESSNOTE, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));

			SendDlgItemMessage(dlg, IDC_CIPHERORDER, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHCIPHERPREFS, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHMOVECIPHERUP, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHMOVECIPHERDOWN, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));

			SendDlgItemMessage(dlg, IDC_KEX_ORDER, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHKEX_LIST, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHKEX_MOVEUP, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHKEX_MOVEDOWN, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));

			SendDlgItemMessage(dlg, IDC_HOST_KEY_ORDER, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHHOST_KEY_LIST, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHHOST_KEY_MOVEUP, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHHOST_KEY_MOVEDOWN, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));

			SendDlgItemMessage(dlg, IDC_MAC_ORDER, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHMAC_LIST, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHMAC_MOVEUP, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHMAC_MOVEDOWN, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));

			SendDlgItemMessage(dlg, IDC_COMP_ORDER, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHCOMP_LIST, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHCOMP_MOVEUP, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHCOMP_MOVEDOWN, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));

			SendDlgItemMessage(dlg, IDC_CHOOSEREADWRITEFILE, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_READWRITEFILENAME, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CHOOSEREADONLYFILE, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_READONLYFILENAME, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_KNOWNHOSTS, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HEARTBEATLABEL, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HEARTBEAT_EDIT, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_HEARTBEATLABEL2, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_REMEMBERPASSWORD, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_FORWARDAGENT, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_FORWARDAGENTCONFIRM, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_FORWARDAGENTNOTIFY, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_VERIFYHOSTKEYDNS, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_NOTICEBANNER, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDOK, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDCANCEL, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE,0));

			SendDlgItemMessage(dlg, IDC_HOSTKEY_ROTATION_STATIC, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE, 0));
			SendDlgItemMessage(dlg, IDC_HOSTKEY_ROTATION_COMBO, WM_SETFONT, (WPARAM)DlgSetupFont, MAKELPARAM(TRUE, 0));
		}
		else {
			DlgSetupFont = NULL;
		}

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			complete_setup_dlg((PTInstVar) GetWindowLong(dlg, DWL_USER), dlg);
			EndDialog(dlg, 1);
			if (DlgSetupFont != NULL) {
				DeleteObject(DlgSetupFont);
			}
			return TRUE;
		case IDCANCEL:			/* there isn't a cancel button, but other Windows
								   UI things can send this message */
			EndDialog(dlg, 0);
			if (DlgSetupFont != NULL) {
				DeleteObject(DlgSetupFont);
			}
			return TRUE;
		// Cipher order
		case IDC_SSHMOVECIPHERUP:
			move_cur_sel_delta(GetDlgItem(dlg, IDC_SSHCIPHERPREFS), -1);
			set_move_button_status(dlg, IDC_SSHCIPHERPREFS, IDC_SSHMOVECIPHERUP, IDC_SSHMOVECIPHERDOWN);
			SetFocus(GetDlgItem(dlg, IDC_SSHCIPHERPREFS));
			return TRUE;
		case IDC_SSHMOVECIPHERDOWN:
			move_cur_sel_delta(GetDlgItem(dlg, IDC_SSHCIPHERPREFS), 1);
			set_move_button_status(dlg, IDC_SSHCIPHERPREFS, IDC_SSHMOVECIPHERUP, IDC_SSHMOVECIPHERDOWN);
			SetFocus(GetDlgItem(dlg, IDC_SSHCIPHERPREFS));
			return TRUE;
		case IDC_SSHCIPHERPREFS:
			set_move_button_status(dlg, IDC_SSHCIPHERPREFS, IDC_SSHMOVECIPHERUP, IDC_SSHMOVECIPHERDOWN);
			return TRUE;
		// KEX order
		case IDC_SSHKEX_MOVEUP:
			move_cur_sel_delta(GetDlgItem(dlg, IDC_SSHKEX_LIST), -1);
			set_move_button_status(dlg, IDC_SSHKEX_LIST, IDC_SSHKEX_MOVEUP, IDC_SSHKEX_MOVEDOWN);
			SetFocus(GetDlgItem(dlg, IDC_SSHKEX_LIST));
			return TRUE;
		case IDC_SSHKEX_MOVEDOWN:
			move_cur_sel_delta(GetDlgItem(dlg, IDC_SSHKEX_LIST), 1);
			set_move_button_status(dlg, IDC_SSHKEX_LIST, IDC_SSHKEX_MOVEUP, IDC_SSHKEX_MOVEDOWN);
			SetFocus(GetDlgItem(dlg, IDC_SSHKEX_LIST));
			return TRUE;
		case IDC_SSHKEX_LIST:
			set_move_button_status(dlg, IDC_SSHKEX_LIST, IDC_SSHKEX_MOVEUP, IDC_SSHKEX_MOVEDOWN);
			return TRUE;
		// Host Key order
		case IDC_SSHHOST_KEY_MOVEUP:
			move_cur_sel_delta(GetDlgItem(dlg, IDC_SSHHOST_KEY_LIST), -1);
			set_move_button_status(dlg, IDC_SSHHOST_KEY_LIST, IDC_SSHHOST_KEY_MOVEUP, IDC_SSHHOST_KEY_MOVEDOWN);
			SetFocus(GetDlgItem(dlg, IDC_SSHHOST_KEY_LIST));
			return TRUE;
		case IDC_SSHHOST_KEY_MOVEDOWN:
			move_cur_sel_delta(GetDlgItem(dlg, IDC_SSHHOST_KEY_LIST), 1);
			set_move_button_status(dlg, IDC_SSHHOST_KEY_LIST, IDC_SSHHOST_KEY_MOVEUP, IDC_SSHHOST_KEY_MOVEDOWN);
			SetFocus(GetDlgItem(dlg, IDC_SSHHOST_KEY_LIST));
			return TRUE;
		case IDC_SSHHOST_KEY_LIST:
			set_move_button_status(dlg, IDC_SSHHOST_KEY_LIST, IDC_SSHHOST_KEY_MOVEUP, IDC_SSHHOST_KEY_MOVEDOWN);
			return TRUE;
		// Mac order
		case IDC_SSHMAC_MOVEUP:
			move_cur_sel_delta(GetDlgItem(dlg, IDC_SSHMAC_LIST), -1);
			set_move_button_status(dlg, IDC_SSHMAC_LIST, IDC_SSHMAC_MOVEUP, IDC_SSHMAC_MOVEDOWN);
			SetFocus(GetDlgItem(dlg, IDC_SSHMAC_LIST));
			return TRUE;
		case IDC_SSHMAC_MOVEDOWN:
			move_cur_sel_delta(GetDlgItem(dlg, IDC_SSHMAC_LIST), 1);
			set_move_button_status(dlg, IDC_SSHMAC_LIST, IDC_SSHMAC_MOVEUP, IDC_SSHMAC_MOVEDOWN);
			SetFocus(GetDlgItem(dlg, IDC_SSHMAC_LIST));
			return TRUE;
		case IDC_SSHMAC_LIST:
			set_move_button_status(dlg, IDC_SSHMAC_LIST, IDC_SSHMAC_MOVEUP, IDC_SSHMAC_MOVEDOWN);
			return TRUE;
		// Compression order
		case IDC_SSHCOMP_MOVEUP:
			move_cur_sel_delta(GetDlgItem(dlg, IDC_SSHCOMP_LIST), -1);
			set_move_button_status(dlg, IDC_SSHCOMP_LIST, IDC_SSHCOMP_MOVEUP, IDC_SSHCOMP_MOVEDOWN);
			SetFocus(GetDlgItem(dlg, IDC_SSHCOMP_LIST));
			return TRUE;
		case IDC_SSHCOMP_MOVEDOWN:
			move_cur_sel_delta(GetDlgItem(dlg, IDC_SSHCOMP_LIST), 1);
			set_move_button_status(dlg, IDC_SSHCOMP_LIST, IDC_SSHCOMP_MOVEUP, IDC_SSHCOMP_MOVEDOWN);
			SetFocus(GetDlgItem(dlg, IDC_SSHCOMP_LIST));
			return TRUE;
		case IDC_SSHCOMP_LIST:
			set_move_button_status(dlg, IDC_SSHCOMP_LIST, IDC_SSHCOMP_MOVEUP, IDC_SSHCOMP_MOVEDOWN);
			return TRUE;
		case IDC_CHOOSEREADWRITEFILE:
			choose_read_write_file(dlg);
			return TRUE;
		case IDC_CHOOSEREADONLYFILE:
			choose_read_only_file(dlg);
			return TRUE;
		case IDC_FORWARDAGENT:
			if (!IsDlgButtonChecked(dlg, IDC_FORWARDAGENT)) {
				EnableWindow(GetDlgItem(dlg, IDC_FORWARDAGENTCONFIRM), FALSE);
				EnableWindow(GetDlgItem(dlg, IDC_FORWARDAGENTNOTIFY), FALSE);
			}
			else {
				EnableWindow(GetDlgItem(dlg, IDC_FORWARDAGENTCONFIRM), TRUE);
				if (HasBalloonTipSupport()) {
					EnableWindow(GetDlgItem(dlg, IDC_FORWARDAGENTNOTIFY), TRUE);
				}
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


//
// SSH key generator dialog (2005.4.10 yutaka)
//

typedef struct {
	RSA *rsa;
	DSA *dsa;
	EC_KEY *ecdsa;
	unsigned char *ed25519_sk; 
	unsigned char *ed25519_pk; 
	ssh_keytype type;
} ssh_private_key_t;

static ssh_private_key_t private_key = {NULL, NULL, NULL, NULL, NULL, KEY_UNSPEC};

typedef struct {
	RSA *rsa;
	DSA *dsa;
	EC_KEY *ecdsa;
	unsigned char *ed25519_sk; 
	unsigned char *ed25519_pk; 
	ssh_keytype type;
} ssh_public_key_t;

static ssh_public_key_t public_key = {NULL, NULL, NULL, NULL, NULL, KEY_UNSPEC};

static void free_ssh_key(void)
{
	// DSA_free(), RSA_free()にNULLを渡しても問題はなし。
	DSA_free(private_key.dsa);
	private_key.dsa = NULL;
	DSA_free(public_key.dsa);
	public_key.dsa = NULL;

	RSA_free(private_key.rsa);
	private_key.rsa = NULL;
	RSA_free(public_key.rsa);
	public_key.rsa = NULL;

	EC_KEY_free(private_key.ecdsa);
	private_key.ecdsa = NULL;
	EC_KEY_free(public_key.ecdsa);
	public_key.ecdsa = NULL;

	free(private_key.ed25519_sk);
	private_key.ed25519_sk = NULL;
	free(private_key.ed25519_pk);
	private_key.ed25519_pk = NULL;

	private_key.type = KEY_UNSPEC;
	public_key.type = KEY_UNSPEC;
}

static BOOL generate_ssh_key(ssh_keytype type, int bits, void (*cbfunc)(int, int, void *), void *cbarg)
{
	// if SSH key already is generated, should free the resource.
	free_ssh_key();

	switch (type) {
	case KEY_RSA1:
	case KEY_RSA:
	{
		RSA *priv = NULL;
		RSA *pub = NULL;

		// private key
		priv =  RSA_generate_key(bits, 35, cbfunc, cbarg);
		if (priv == NULL)
			goto error;
		private_key.rsa = priv;

		// public key
		pub = RSA_new();
		pub->n = BN_new();
		pub->e = BN_new();
		if (pub->n == NULL || pub->e == NULL) {
			RSA_free(pub);
			goto error;
		}

		BN_copy(pub->n, priv->n);
		BN_copy(pub->e, priv->e);
		public_key.rsa = pub;
		break;
	}
	
	case KEY_DSA:
	{
		DSA *priv = NULL;
		DSA *pub = NULL;

		// private key
		priv = DSA_generate_parameters(bits, NULL, 0, NULL, NULL, cbfunc, cbarg);
		if (priv == NULL)
			goto error;
		if (!DSA_generate_key(priv)) {
			// TODO: free 'priv'?
			goto error;
		}
		private_key.dsa = priv;

		// public key
		pub = DSA_new();
		if (pub == NULL)
			goto error;
		pub->p = BN_new();
		pub->q = BN_new();
		pub->g = BN_new();
		pub->pub_key = BN_new();
		if (pub->p == NULL || pub->q == NULL || pub->g == NULL || pub->pub_key == NULL) {
			DSA_free(pub);
			goto error;
		}

		BN_copy(pub->p, priv->p);
		BN_copy(pub->q, priv->q);
		BN_copy(pub->g, priv->g);
		BN_copy(pub->pub_key, priv->pub_key);
		public_key.dsa = pub;
		break;
	}

	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
	{
		EC_KEY *priv = NULL;
		EC_KEY *pub = NULL;

		UTIL_get_lang_msg("MSG_KEYGEN_GENERATING", pvar, "generating key");
		SetDlgItemText(((cbarg_t *)cbarg)->dlg, IDC_KEYGEN_PROGRESS_LABEL, pvar->ts->UIMsg);

		priv = EC_KEY_new_by_curve_name(keytype_to_cipher_nid(type));
		pub = EC_KEY_new_by_curve_name(keytype_to_cipher_nid(type));
		if (priv == NULL || pub == NULL) {
			goto error;
		}

		// private key
		if (EC_KEY_generate_key(priv) != 1) {
			goto error;
		}
		EC_KEY_set_asn1_flag(priv, OPENSSL_EC_NAMED_CURVE);
		private_key.ecdsa = priv;

		// public key
		if (EC_KEY_set_public_key(pub, EC_KEY_get0_public_key(priv)) != 1) {
			goto error;
		}
		public_key.ecdsa = pub;

		UTIL_get_lang_msg("MSG_KEYGEN_GENERATED", pvar, "key generated");
		SetDlgItemText(((cbarg_t *)cbarg)->dlg, IDC_KEYGEN_PROGRESS_LABEL, pvar->ts->UIMsg);

		break;
	}

	case KEY_ED25519:
	{
		// 秘密鍵を作る
		private_key.ed25519_pk = malloc(ED25519_PK_SZ);
		private_key.ed25519_sk = malloc(ED25519_SK_SZ);
		if (private_key.ed25519_pk == NULL || private_key.ed25519_sk == NULL)
			goto error;
		crypto_sign_ed25519_keypair(private_key.ed25519_pk, private_key.ed25519_sk);

		// 公開鍵を作る
		public_key.ed25519_pk = malloc(ED25519_PK_SZ);
		if (public_key.ed25519_pk == NULL)
			goto error;
		memcpy(public_key.ed25519_pk, private_key.ed25519_pk, ED25519_PK_SZ);

		break;
	}

	default:
		goto error;
	}

	private_key.type = type;
	public_key.type = type;

	return TRUE;

error:
	free_ssh_key();
	return FALSE;
}


//
// SSH1 3DES
//
/*
 * This is used by SSH1:
 *
 * What kind of triple DES are these 2 routines?
 *
 * Why is there a redundant initialization vector?
 *
 * If only iv3 was used, then, this would till effect have been
 * outer-cbc. However, there is also a private iv1 == iv2 which
 * perhaps makes differential analysis easier. On the other hand, the
 * private iv1 probably makes the CRC-32 attack ineffective. This is a
 * result of that there is no longer any known iv1 to use when
 * choosing the X block.
 */
struct ssh1_3des_ctx
{
	EVP_CIPHER_CTX  k1, k2, k3;
};

static int ssh1_3des_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh1_3des_ctx *c;
	u_char *k1, *k2, *k3;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = malloc(sizeof(*c));
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key == NULL)
		return (1);
	if (enc == -1)
		enc = ctx->encrypt;
	k1 = k2 = k3 = (u_char *) key;
	k2 += 8;
	if (EVP_CIPHER_CTX_key_length(ctx) >= 16+8) {
		if (enc)
			k3 += 16;
		else
			k1 += 16;
	}
	EVP_CIPHER_CTX_init(&c->k1);
	EVP_CIPHER_CTX_init(&c->k2);
	EVP_CIPHER_CTX_init(&c->k3);
	if (EVP_CipherInit(&c->k1, EVP_des_cbc(), k1, NULL, enc) == 0 ||
		EVP_CipherInit(&c->k2, EVP_des_cbc(), k2, NULL, !enc) == 0 ||
		EVP_CipherInit(&c->k3, EVP_des_cbc(), k3, NULL, enc) == 0) {
			SecureZeroMemory(c, sizeof(*c));
			free(c);
			EVP_CIPHER_CTX_set_app_data(ctx, NULL);
			return (0);
	}
	return (1);
}

static int ssh1_3des_cbc(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, u_int len)
{
	struct ssh1_3des_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		//error("ssh1_3des_cbc: no context");
		return (0);
	}
	if (EVP_Cipher(&c->k1, dest, (u_char *)src, len) == 0 ||
		EVP_Cipher(&c->k2, dest, dest, len) == 0 ||
		EVP_Cipher(&c->k3, dest, dest, len) == 0)
		return (0);
	return (1);
}

static int ssh1_3des_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh1_3des_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		EVP_CIPHER_CTX_cleanup(&c->k1);
		EVP_CIPHER_CTX_cleanup(&c->k2);
		EVP_CIPHER_CTX_cleanup(&c->k3);
		SecureZeroMemory(c, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

void ssh1_3des_iv(EVP_CIPHER_CTX *evp, int doset, u_char *iv, int len)
{
	struct ssh1_3des_ctx *c;

	if (len != 24)
		//fatal("%s: bad 3des iv length: %d", __func__, len);
		;

	if ((c = EVP_CIPHER_CTX_get_app_data(evp)) == NULL)
		//fatal("%s: no 3des context", __func__);
		;

	if (doset) {
		//debug3("%s: Installed 3DES IV", __func__);
		memcpy(c->k1.iv, iv, 8);
		memcpy(c->k2.iv, iv + 8, 8);
		memcpy(c->k3.iv, iv + 16, 8);
	} else {
		//debug3("%s: Copying 3DES IV", __func__);
		memcpy(iv, c->k1.iv, 8);
		memcpy(iv + 8, c->k2.iv, 8);
		memcpy(iv + 16, c->k3.iv, 8);
	}
}

const EVP_CIPHER *evp_ssh1_3des(void)
{
	static EVP_CIPHER ssh1_3des;

	memset(&ssh1_3des, 0, sizeof(EVP_CIPHER));
	ssh1_3des.nid = NID_undef;
	ssh1_3des.block_size = 8;
	ssh1_3des.iv_len = 0;
	ssh1_3des.key_len = 16;
	ssh1_3des.init = ssh1_3des_init;
	ssh1_3des.cleanup = ssh1_3des_cleanup;
	ssh1_3des.do_cipher = ssh1_3des_cbc;
	ssh1_3des.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH;
	return (&ssh1_3des);
}

static void ssh_make_comment(char *comment, int maxlen)
{
	char user[UNLEN + 1], host[128];
	DWORD dwSize;
	WSADATA wsaData;
	int ret;

	// get Windows logon user name
	dwSize = sizeof(user);
	if (GetUserName(user, &dwSize) == 0) {
		strncpy_s(user, sizeof(user), "yutaka", _TRUNCATE);
	}

	// get local hostname (by WinSock)
	ret = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (ret == 0) {
		if (gethostname(host, sizeof(host)) != 0) {
			ret = WSAGetLastError();
		}
		WSACleanup();
	}
	if (ret != 0) {
		strncpy_s(host, sizeof(host), "sai", _TRUNCATE);
	}

	_snprintf_s(comment, maxlen, _TRUNCATE, "%s@%s", user, host);
}

// uuencode (rfc1521)
int uuencode(unsigned char *src, int srclen, unsigned char *target, int targsize)
{
	char base64[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char pad = '=';
	int datalength = 0;
	unsigned char input[3];
	unsigned char output[4];
	int i;

	while (srclen > 2) {
		input[0] = *src++;
		input[1] = *src++;
		input[2] = *src++;
		srclen -= 3;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = input[2] & 0x3f;
		if (output[0] >= 64 ||
		    output[1] >= 64 ||
		    output[2] >= 64 ||
		    output[3] >= 64)
			return -1;

		if (datalength + 4 > targsize)
			return (-1);
		target[datalength++] = base64[output[0]];
		target[datalength++] = base64[output[1]];
		target[datalength++] = base64[output[2]];
		target[datalength++] = base64[output[3]];
	}

	if (srclen != 0) {
		/* Get what's left. */
		input[0] = input[1] = input[2] = '\0';
		for (i = 0; i < srclen; i++)
			input[i] = *src++;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		if (output[0] >= 64 ||
		    output[1] >= 64 ||
		    output[2] >= 64)
			return -1;

		if (datalength + 4 > targsize)
			return (-1);
		target[datalength++] = base64[output[0]];
		target[datalength++] = base64[output[1]];
		if (srclen == 1)
			target[datalength++] = pad;
		else
			target[datalength++] = base64[output[2]];
		target[datalength++] = pad;
	}
	if (datalength >= targsize)
		return (-1);
	target[datalength] = '\0';  /* Returned value doesn't count \0. */

	return (datalength); // success
}

//
// SCP dialog
//
static BOOL CALLBACK TTXScpDialog(HWND dlg, UINT msg, WPARAM wParam,
                                     LPARAM lParam)
{
	static char sendfile[MAX_PATH] = "";
	static char sendfiledir[MAX_PATH] = "";
	static char recvdir[MAX_PATH] = "";
	HWND hWnd;
	HDROP hDrop;
	UINT uFileNo;
	char szFileName[256];
	int i;

	switch (msg) {
	case WM_INITDIALOG:
		DragAcceptFiles(dlg, TRUE);

		// SCPファイル送信先を表示する
		if (sendfiledir[0] == '\0') {
			_snprintf_s(sendfiledir, sizeof(sendfiledir), _TRUNCATE, pvar->ts->ScpSendDir); // home directory
		}
		SendMessage(GetDlgItem(dlg, IDC_SENDFILE_TO), WM_SETTEXT, 0, (LPARAM)sendfiledir);

		// SCPファイル受信先を表示する
		if (recvdir[0] == '\0') {
			_snprintf_s(recvdir, sizeof(recvdir), _TRUNCATE, "%s", pvar->ts->FileDir);
		}
		SendMessage(GetDlgItem(dlg, IDC_RECVFILE_TO), WM_SETTEXT, 0, (LPARAM)recvdir);

#ifdef SFTP_DEBUG
		ShowWindow(GetDlgItem(dlg, IDC_SFTP_TEST), SW_SHOW);
#endif

		return TRUE;

	case WM_DROPFILES:
		{
		hDrop = (HDROP)wParam;
		uFileNo = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
		for(i = 0; i < (int)uFileNo; i++) {
			DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));

			// update edit box
			hWnd = GetDlgItem(dlg, IDC_SENDFILE_EDIT);
			SendMessage(hWnd, WM_SETTEXT , 0, (LPARAM)szFileName);
		}
		DragFinish(hDrop);
		}
		return TRUE;

	case WM_COMMAND:
		switch (wParam) {
		case IDC_SENDFILE_SELECT | (BN_CLICKED << 16):
			{
			OPENFILENAME ofn;

			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = dlg;
#if 0
			get_lang_msg("FILEDLG_SELECT_LOGVIEW_APP_FILTER", ts.UIMsg, sizeof(ts.UIMsg),
			             "exe(*.exe)\\0*.exe\\0all(*.*)\\0*.*\\0\\0", ts.UILanguageFile);
#endif
			ofn.lpstrFilter = "all(*.*)\0*.*\0\0";
			ofn.lpstrFile = sendfile;
			ofn.nMaxFile = sizeof(sendfile);
#if 0
			get_lang_msg("FILEDLG_SELECT_LOGVIEW_APP_TITLE", uimsg, sizeof(uimsg),
			             "Choose a executing file with launching logging file", ts.UILanguageFile);
#endif
			ofn.lpstrTitle = "Choose a sending file with SCP";

// WINVER がセットされないためにマクロが定義されないので、ここで定義する(2008.1.21 maya)
#ifndef OFN_FORCESHOWHIDDEN
/* from commdlg.h */
#define OFN_FORCESHOWHIDDEN          0x10000000
#endif
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_FORCESHOWHIDDEN | OFN_HIDEREADONLY;
			if (GetOpenFileName(&ofn) != 0) {
				hWnd = GetDlgItem(dlg, IDC_SENDFILE_EDIT);
				SendMessage(hWnd, WM_SETTEXT , 0, (LPARAM)sendfile);
			}
			}
			return TRUE;
		case IDC_RECVDIR_SELECT | (BN_CLICKED << 16):
			{
			char buf[MAX_PATH], buf2[MAX_PATH];
			hWnd = GetDlgItem(dlg, IDC_RECVFILE_TO);
			SendMessage(hWnd, WM_GETTEXT , sizeof(buf), (LPARAM)buf);
			if (doSelectFolder(dlg, buf2, sizeof(buf2), buf, "Choose destination directory")) {
				strncpy_s(recvdir, sizeof(recvdir), buf2, _TRUNCATE);
				SendMessage(GetDlgItem(dlg, IDC_RECVFILE_TO), WM_SETTEXT, 0, (LPARAM)recvdir);
			}
			}
			return TRUE;
		}

		switch (LOWORD(wParam)) {
		case IDOK:
			hWnd = GetDlgItem(dlg, IDC_SENDFILE_EDIT);
			SendMessage(hWnd, WM_GETTEXT , sizeof(sendfile), (LPARAM)sendfile);
			if (sendfile[0] != '\0') {
				// 送信パスを取り出し、teraterm.ini も合わせて更新する。
				hWnd = GetDlgItem(dlg, IDC_SENDFILE_TO);
				SendMessage(hWnd, WM_GETTEXT , sizeof(sendfiledir), (LPARAM)sendfiledir);
				strncpy_s(pvar->ts->ScpSendDir, sizeof(pvar->ts->ScpSendDir), sendfiledir, _TRUNCATE);

				SSH_start_scp(pvar, sendfile, sendfiledir);
				//SSH_scp_transaction(pvar, "bigfile30.bin", "", FROMREMOTE);
				EndDialog(dlg, 1); // dialog close
				return TRUE;
			}
			return FALSE;

		case IDCANCEL:
			// 送信パスを取り出し、teraterm.ini も合わせて更新する。
			hWnd = GetDlgItem(dlg, IDC_SENDFILE_TO);
			SendMessage(hWnd, WM_GETTEXT , sizeof(sendfiledir), (LPARAM)sendfiledir);
			strncpy_s(pvar->ts->ScpSendDir, sizeof(pvar->ts->ScpSendDir), sendfiledir, _TRUNCATE);

			// 受信パスに関しても更新する。(2013.8.18 yutaka)
			hWnd = GetDlgItem(dlg, IDC_RECVFILE_TO);
			SendMessage(hWnd, WM_GETTEXT , sizeof(recvdir), (LPARAM)recvdir);
			strncpy_s(pvar->ts->FileDir, sizeof(pvar->ts->FileDir), recvdir, _TRUNCATE);

			EndDialog(dlg, 0); // dialog close
			return TRUE;

		case IDC_RECV:  // ファイル受信
			hWnd = GetDlgItem(dlg, IDC_RECVFILE);
			SendMessage(hWnd, WM_GETTEXT , sizeof(szFileName), (LPARAM)szFileName);
			if (szFileName[0] != '\0') {
				char recvpath[MAX_PATH] = "";
				char* fn = strrchr(szFileName, '/');
				char recvfn[sizeof(szFileName)];
				if (fn) {
					fn++;
					if (*fn == '\0') {
						return FALSE;
					}
				}
				else {
					fn = szFileName;
				}
				strncpy_s(recvfn, sizeof(recvfn), fn, _TRUNCATE);
				replaceInvalidFileNameChar(recvfn, '_');
				SendMessage(GetDlgItem(dlg, IDC_RECVFILE_TO), WM_GETTEXT, sizeof(recvdir), (LPARAM)recvdir);
				_snprintf_s(recvpath, sizeof(recvpath), _TRUNCATE, "%s\\%s", recvdir, recvfn);
				SSH_scp_transaction(pvar, szFileName, recvpath, FROMREMOTE);
				EndDialog(dlg, 1); // dialog close
				return TRUE;
			}
			return FALSE;

		case IDC_SFTP_TEST:
			SSH_sftp_transaction(pvar);
			return TRUE;
		}
	}

	return FALSE;
}

// マクロコマンド"scpsend"から呼び出すために、DLL外へエクスポートする。"ttxssh.def"ファイルに記載。
// (2008.1.1 yutaka)
__declspec(dllexport) int CALLBACK TTXScpSendfile(char *filename, char *dstfile)
{
	return SSH_start_scp(pvar, filename, dstfile);
}

__declspec(dllexport) int CALLBACK TTXScpReceivefile(char *remotefile, char *localfile)
{
	return SSH_scp_transaction(pvar, remotefile, localfile, FROMREMOTE);
}


// TTSSHの設定内容(known hosts file)を返す。
//
// return TRUE: 返却成功
//        FALSE: 失敗
// (2015.3.9 yutaka)
__declspec(dllexport) int CALLBACK TTXReadKnownHostsFile(char *filename, int maxlen)
{
	int ret = FALSE;
	char *p;

	if (pvar->settings.Enabled) {
		strncpy_s(filename, maxlen, pvar->settings.KnownHostsFiles, _TRUNCATE);
		p = strchr(filename, ';');
		if (p)
			*p = 0;

		ret = TRUE;
	}

	return (ret);
}


static void keygen_progress(int phase, int count, cbarg_t *cbarg) {
	char buff[1024];
	static char msg[1024];

	switch (phase) {
	case 0:
		if (count == 0) {
			UTIL_get_lang_msg("MSG_KEYGEN_GENERATING", pvar, "generating key");
			strncpy_s(msg, sizeof(msg), pvar->ts->UIMsg, _TRUNCATE);
		}
		if (cbarg->type == KEY_DSA && count %10 != 0) {
			return;
		}
		_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%s %d/4 (%d)", msg, cbarg->cnt*2+1, count);
		break;
	case 1:
		if (count < 0) {
			return;
		}
		_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%s %d/4 (%d)", msg, cbarg->cnt*2+2, count);
		break;
	case 2:
		return;
		break;
	case 3:
		if (count == 0) {
			cbarg->cnt = 1;
			return;
		}
		else {
			UTIL_get_lang_msg("MSG_KEYGEN_GENERATED", pvar, "key generated");
			_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%s", pvar->ts->UIMsg);
		}
		break;
	default:
		return;
	}

	SetDlgItemText(cbarg->dlg, IDC_KEYGEN_PROGRESS_LABEL, buff);
	return;
}

static void init_password_control(HWND dlg, int item)
{
	HWND passwordControl = GetDlgItem(dlg, item);

	SetWindowLong(passwordControl, GWL_USERDATA,
	              SetWindowLong(passwordControl, GWL_WNDPROC,
	                            (LONG) password_wnd_proc));
}

// bcrypt KDF形式で秘密鍵を保存する
// based on OpenSSH 6.5:key_save_private(), key_private_to_blob2()
static void save_bcrypt_private_key(char *passphrase, char *filename, char *comment, HWND dlg, PTInstVar pvar, int rounds)
{
	SSHCipher ciphernameval = SSH_CIPHER_NONE;
	char *ciphername = DEFAULT_CIPHERNAME;
	buffer_t *b = NULL;
	buffer_t *kdf = NULL;
	buffer_t *encoded = NULL;
	buffer_t *blob = NULL;
	int blocksize, keylen, ivlen, authlen, i, n; 
	unsigned char *key = NULL, salt[SALT_LEN];
	char *kdfname = KDFNAME;
	EVP_CIPHER_CTX cipher_ctx;
	Key keyblob;
	unsigned char *cp = NULL;
	unsigned int len, check;
	FILE *fp;
	char uimsg[MAX_UIMSG];

	b = buffer_init();
	kdf = buffer_init();
	encoded = buffer_init();
	blob = buffer_init();
	if (b == NULL || kdf == NULL || encoded == NULL || blob == NULL)
		goto ed25519_error;

	if (passphrase == NULL || !strlen(passphrase)) {
		ciphername = "none";
		kdfname = "none";
	}

	ciphernameval = get_cipher_by_name(ciphername);
	blocksize = get_cipher_block_size(ciphernameval);
	keylen = get_cipher_key_len(ciphernameval);
	ivlen = blocksize;
	authlen = 0;  // TODO: とりあえず固定化
	key = calloc(1, keylen + ivlen);

	if (strcmp(kdfname, "none") != 0) {
		arc4random_buf(salt, SALT_LEN);
		if (bcrypt_pbkdf(passphrase, strlen(passphrase),
			salt, SALT_LEN, key, keylen + ivlen, rounds) < 0)
			//fatal("bcrypt_pbkdf failed");
			;
		buffer_put_string(kdf, salt, SALT_LEN);
		buffer_put_int(kdf, rounds);
	}
	// 暗号化の準備
	// TODO: OpenSSH 6.5では -Z オプションで、暗号化アルゴリズムを指定可能だが、
	// ここでは"AES256-CBC"に固定とする。
	cipher_init_SSH2(&cipher_ctx, key, keylen, key + keylen, ivlen, CIPHER_ENCRYPT, 
		get_cipher_EVP_CIPHER(ciphernameval), 0, pvar);
	SecureZeroMemory(key, keylen + ivlen);
	free(key);

	buffer_append(encoded, AUTH_MAGIC, sizeof(AUTH_MAGIC));
	buffer_put_cstring(encoded, ciphername);
	buffer_put_cstring(encoded, kdfname);
	buffer_put_string(encoded, buffer_ptr(kdf), buffer_len(kdf));
	buffer_put_int(encoded, 1);			/* number of keys */

	// key_to_blob()を一時利用するため、Key構造体を初期化する。
	keyblob.type = private_key.type;
	keyblob.rsa = private_key.rsa;
	keyblob.dsa = private_key.dsa;
	keyblob.ecdsa = private_key.ecdsa;
	keyblob.ed25519_pk = private_key.ed25519_pk;
	keyblob.ed25519_sk = private_key.ed25519_sk;
	key_to_blob(&keyblob, &cp, &len);			/* public key */

	buffer_put_string(encoded, cp, len);

	SecureZeroMemory(cp, len);
	free(cp);

	/* Random check bytes */
	check = arc4random();
	buffer_put_int(b, check);
	buffer_put_int(b, check);

	/* append private key and comment*/
	key_private_serialize(&keyblob, b);
	buffer_put_cstring(b, comment);

	/* padding */
	i = 0;
	while (buffer_len(b) % blocksize)
		buffer_put_char(b, ++i & 0xff);

	/* length */
	buffer_put_int(encoded, buffer_len(b));

	/* encrypt */
	cp = buffer_append_space(encoded, buffer_len(b) + authlen);
	if (EVP_Cipher(&cipher_ctx, cp, buffer_ptr(b), buffer_len(b)) == 0) {
		//strncpy_s(errmsg, errmsg_len, "Key decrypt error", _TRUNCATE);
		//free(decrypted);
		//goto error;
	}
	cipher_cleanup_SSH2(&cipher_ctx);

	len = 2 * buffer_len(encoded);
	cp = malloc(len);
	n = uuencode(buffer_ptr(encoded), buffer_len(encoded), (char *)cp, len);
	if (n < 0) {
		free(cp);
		goto ed25519_error;
	}

	buffer_clear(blob);
	buffer_append(blob, MARK_BEGIN, sizeof(MARK_BEGIN) - 1);
	for (i = 0; i < n; i++) {
		buffer_put_char(blob, cp[i]);
		if (i % 70 == 69)
			buffer_put_char(blob, '\n');
	}
	if (i % 70 != 69)
		buffer_put_char(blob, '\n');
	buffer_append(blob, MARK_END, sizeof(MARK_END) - 1);
	free(cp);

	len = buffer_len(blob);

	// 秘密鍵をファイルに保存する。
	fp = fopen(filename, "wb");
	if (fp == NULL) {
		UTIL_get_lang_msg("MSG_SAVE_KEY_OPENFILE_ERROR", pvar,
		                  "Can't open key file");
		strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
		UTIL_get_lang_msg("MSG_ERROR", pvar, "ERROR");
		MessageBox(dlg, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
		goto ed25519_error;
	}
	n = fwrite(buffer_ptr(blob), buffer_len(blob), 1, fp);
	if (n != 1) {
		UTIL_get_lang_msg("MSG_SAVE_KEY_WRITEFILE_ERROR", pvar,
		                  "Can't open key file");
		strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
		UTIL_get_lang_msg("MSG_ERROR", pvar, "ERROR");
		MessageBox(dlg, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
	}
	fclose(fp);


ed25519_error:
	buffer_free(b);
	buffer_free(kdf);
	buffer_free(encoded);
	buffer_free(blob);
}

static BOOL CALLBACK TTXKeyGenerator(HWND dlg, UINT msg, WPARAM wParam,
                                     LPARAM lParam)
{
	static ssh_keytype key_type;
	static int saved_key_bits;
	char uimsg[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;

	switch (msg) {
	case WM_INITDIALOG:
		{
		GetWindowText(dlg, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_TITLE", pvar, uimsg);
		SetWindowText(dlg, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_KEYTYPE, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_KEYTYPE", pvar, uimsg);
		SetDlgItemText(dlg, IDC_KEYTYPE, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_KEYBITS_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_BITS", pvar, uimsg);
		SetDlgItemText(dlg, IDC_KEYBITS_LABEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_KEY_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_PASSPHRASE", pvar, uimsg);
		SetDlgItemText(dlg, IDC_KEY_LABEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_CONFIRM_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_PASSPHRASE2", pvar, uimsg);
		SetDlgItemText(dlg, IDC_CONFIRM_LABEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_COMMENT_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_COMMENT", pvar, uimsg);
		SetDlgItemText(dlg, IDC_COMMENT_LABEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_SAVE_PUBLIC_KEY, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_SAVEPUBLIC", pvar, uimsg);
		SetDlgItemText(dlg, IDC_SAVE_PUBLIC_KEY, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_SAVE_PRIVATE_KEY, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_SAVEPRIVATE", pvar, uimsg);
		SetDlgItemText(dlg, IDC_SAVE_PRIVATE_KEY, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDOK, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_GENERATE", pvar, uimsg);
		SetDlgItemText(dlg, IDOK, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("BTN_CLOSE", pvar, uimsg);
		SetDlgItemText(dlg, IDCANCEL, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_BCRYPT_KDF_CHECK, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_BCRYPT_KDF", pvar, uimsg);
		SetDlgItemText(dlg, IDC_BCRYPT_KDF_CHECK, pvar->ts->UIMsg);
		GetDlgItemText(dlg, IDC_BCRYPT_KDF_ROUNDS_LABEL, uimsg, sizeof(uimsg));
		UTIL_get_lang_msg("DLG_KEYGEN_BCRYPT_ROUNDS", pvar, uimsg);
		SetDlgItemText(dlg, IDC_BCRYPT_KDF_ROUNDS_LABEL, pvar->ts->UIMsg);

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgKeygenFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_KEYTYPE, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_RSA1_TYPE, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_RSA_TYPE, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_DSA_TYPE, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_ECDSA256_TYPE, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_ECDSA384_TYPE, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_ECDSA521_TYPE, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_ED25519_TYPE, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_KEYBITS_LABEL, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_KEYBITS, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_KEY_LABEL, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CONFIRM_LABEL, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_COMMENT_LABEL, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_KEY_EDIT, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CONFIRM_EDIT, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_COMMENT_EDIT, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_KEYGEN_PROGRESS_LABEL, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SAVE_PUBLIC_KEY, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SAVE_PRIVATE_KEY, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDOK, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDCANCEL, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_BCRYPT_KDF_CHECK, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_BCRYPT_KDF_ROUNDS_LABEL, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_BCRYPT_KDF_ROUNDS, WM_SETFONT, (WPARAM)DlgKeygenFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgHostFont = NULL;
		}

		init_password_control(dlg, IDC_KEY_EDIT);
		init_password_control(dlg, IDC_CONFIRM_EDIT);

		// default key type
		SendMessage(GetDlgItem(dlg, IDC_RSA_TYPE), BM_SETCHECK, BST_CHECKED, 0);
		key_type = KEY_RSA;
		saved_key_bits = GetDlgItemInt(dlg, IDC_KEYBITS, NULL, FALSE);

		// default key bits
		SetDlgItemInt(dlg, IDC_KEYBITS, SSH_KEYGEN_DEFAULT_BITS, FALSE);
		SendDlgItemMessage(dlg, IDC_KEYBITS, EM_LIMITTEXT, 4, 0);

		// passphrase edit box disabled(default)
		EnableWindow(GetDlgItem(dlg, IDC_KEY_EDIT), FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_CONFIRM_EDIT), FALSE);

		// comment edit box disabled (default)
		EnableWindow(GetDlgItem(dlg, IDC_COMMENT_EDIT), FALSE);

		// file saving dialog disabled(default)
		EnableWindow(GetDlgItem(dlg, IDC_SAVE_PUBLIC_KEY), FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_SAVE_PRIBATE_KEY), FALSE);

		// default bcrypt KDF
		EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), TRUE);
		EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), FALSE);
		SetDlgItemInt(dlg, IDC_BCRYPT_KDF_ROUNDS, DEFAULT_ROUNDS, FALSE);
		SendDlgItemMessage(dlg, IDC_BCRYPT_KDF_ROUNDS, EM_LIMITTEXT, 4, 0);

		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: // key generate button pressed
			{
			int bits;
			cbarg_t cbarg;
			char comment[1024]; // comment string in private key
			int enable_bcrypt_kdf = 0, enable_bcrypt_rounds = 0;

			cbarg.cnt = 0;
			cbarg.type = key_type;
			cbarg.dlg = dlg;

			bits = GetDlgItemInt(dlg, IDC_KEYBITS, NULL, FALSE);

			switch (key_type) {
				case KEY_RSA1:
				case KEY_RSA:
				case KEY_DSA:
					if (bits < ((key_type==KEY_DSA)?SSH_DSA_MINIMUM_KEY_SIZE:SSH_RSA_MINIMUM_KEY_SIZE)) {
						UTIL_get_lang_msg("MSG_KEYBITS_MIN_ERROR", pvar,
						                  "The key bits is too small.");
						MessageBox(dlg, pvar->ts->UIMsg,
						           "Tera Term", MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					break;
				case KEY_ECDSA256:
					SetDlgItemInt(dlg, IDC_KEYBITS, 256, FALSE);
					break;
				case KEY_ECDSA384:
					SetDlgItemInt(dlg, IDC_KEYBITS, 384, FALSE);
					break;
				case KEY_ECDSA521:
					SetDlgItemInt(dlg, IDC_KEYBITS, 521, FALSE);
					break;
				case KEY_ED25519:
					SetDlgItemInt(dlg, IDC_KEYBITS, 256, FALSE);
					break;
			}

			// passphrase edit box disabled(default)
			EnableWindow(GetDlgItem(dlg, IDC_KEY_EDIT), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_CONFIRM_EDIT), FALSE);

			// comment edit box disabled (default)
			EnableWindow(GetDlgItem(dlg, IDC_COMMENT_EDIT), FALSE);

			// file saving dialog disabled(default)
			EnableWindow(GetDlgItem(dlg, IDC_SAVE_PUBLIC_KEY), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_SAVE_PRIBATE_KEY), FALSE);

			EnableWindow(GetDlgItem(dlg, IDC_RSA1_TYPE), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_RSA_TYPE), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_DSA_TYPE), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_ECDSA256_TYPE), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_ECDSA384_TYPE), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_ECDSA521_TYPE), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_ED25519_TYPE), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_KEYBITS), FALSE);
			EnableWindow(GetDlgItem(dlg, IDOK), FALSE);
			EnableWindow(GetDlgItem(dlg, IDCANCEL), FALSE);

			enable_bcrypt_kdf = IsWindowEnabled(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK));
			enable_bcrypt_rounds = IsWindowEnabled(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS));
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), FALSE);

			if (generate_ssh_key(key_type, bits, keygen_progress, &cbarg)) {
				MSG msg;
				int c = 0;
				// 鍵の計算中に発生したイベント（ボタン連打など）をフラッシュする。
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
					c++;
					if (c >= 100)
						break;
				}

				// passphrase edit box disabled(default)
				EnableWindow(GetDlgItem(dlg, IDC_KEY_EDIT), TRUE);
				EnableWindow(GetDlgItem(dlg, IDC_CONFIRM_EDIT), TRUE);

				// enable comment edit box
				EnableWindow(GetDlgItem(dlg, IDC_COMMENT_EDIT), TRUE);
				ssh_make_comment(comment, sizeof(comment));
				SetDlgItemText(dlg, IDC_COMMENT_EDIT, comment);

				// file saving dialog disabled(default)
				EnableWindow(GetDlgItem(dlg, IDC_SAVE_PUBLIC_KEY), TRUE);
				EnableWindow(GetDlgItem(dlg, IDC_SAVE_PRIBATE_KEY), TRUE);

				EnableWindow(GetDlgItem(dlg, IDC_RSA1_TYPE), TRUE);
				EnableWindow(GetDlgItem(dlg, IDC_RSA_TYPE), TRUE);
				EnableWindow(GetDlgItem(dlg, IDC_DSA_TYPE), TRUE);
				EnableWindow(GetDlgItem(dlg, IDC_ECDSA256_TYPE), TRUE);
				EnableWindow(GetDlgItem(dlg, IDC_ECDSA384_TYPE), TRUE);
				EnableWindow(GetDlgItem(dlg, IDC_ECDSA521_TYPE), TRUE);
				EnableWindow(GetDlgItem(dlg, IDC_ED25519_TYPE), TRUE);
				if (!isFixedLengthKey(key_type)) {
					EnableWindow(GetDlgItem(dlg, IDC_KEYBITS), TRUE);
				}
				EnableWindow(GetDlgItem(dlg, IDOK), TRUE);
				EnableWindow(GetDlgItem(dlg, IDCANCEL), TRUE);

				if (enable_bcrypt_kdf) {
					EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), TRUE);
				}
				if (enable_bcrypt_rounds) {
					EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), TRUE);
				}

				// set focus to passphrase edit control (2007.1.27 maya)
				SetFocus(GetDlgItem(dlg, IDC_KEY_EDIT));
			}
			return TRUE;
			}

		case IDCANCEL:
			// don't forget to free SSH resource!
			free_ssh_key();
			EndDialog(dlg, 0); // dialog close
			if (DlgKeygenFont != NULL) {
				DeleteObject(DlgKeygenFont);
			}
			return TRUE;

		// if radio button pressed...
		case IDC_RSA1_TYPE | (BN_CLICKED << 16):
			if (isFixedLengthKey(key_type)) {
				EnableWindow(GetDlgItem(dlg, IDC_KEYBITS), TRUE);
				SetDlgItemInt(dlg, IDC_KEYBITS, saved_key_bits, FALSE);
			}
			SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), FALSE);
			key_type = KEY_RSA1;
			break;

		case IDC_RSA_TYPE | (BN_CLICKED << 16):
			if (isFixedLengthKey(key_type)) {
				EnableWindow(GetDlgItem(dlg, IDC_KEYBITS), TRUE);
				SetDlgItemInt(dlg, IDC_KEYBITS, saved_key_bits, FALSE);
			}
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), TRUE);
			if (SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), TRUE);
			}
			else {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), FALSE);
			}
			key_type = KEY_RSA;
			break;

		case IDC_DSA_TYPE | (BN_CLICKED << 16):
			if (!isFixedLengthKey(key_type)) {
				EnableWindow(GetDlgItem(dlg, IDC_KEYBITS), FALSE);
				saved_key_bits = GetDlgItemInt(dlg, IDC_KEYBITS, NULL, FALSE);
			}
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), TRUE);
			if (SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), TRUE);
			}
			else {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), FALSE);
			}
			key_type = KEY_DSA;
			SetDlgItemInt(dlg, IDC_KEYBITS, 1024, FALSE);
			break;

		case IDC_ECDSA256_TYPE | (BN_CLICKED << 16):
			if (!isFixedLengthKey(key_type)) {
				EnableWindow(GetDlgItem(dlg, IDC_KEYBITS), FALSE);
				saved_key_bits = GetDlgItemInt(dlg, IDC_KEYBITS, NULL, FALSE);
			}
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), TRUE);
			if (SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), TRUE);
			}
			else {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), FALSE);
			}
			key_type = KEY_ECDSA256;
			SetDlgItemInt(dlg, IDC_KEYBITS, 256, FALSE);
			break;

		case IDC_ECDSA384_TYPE | (BN_CLICKED << 16):
			if (!isFixedLengthKey(key_type)) {
				EnableWindow(GetDlgItem(dlg, IDC_KEYBITS), FALSE);
				saved_key_bits = GetDlgItemInt(dlg, IDC_KEYBITS, NULL, FALSE);
			}
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), TRUE);
			if (SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), TRUE);
			}
			else {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), FALSE);
			}
			key_type = KEY_ECDSA384;
			SetDlgItemInt(dlg, IDC_KEYBITS, 384, FALSE);
			break;

		case IDC_ECDSA521_TYPE | (BN_CLICKED << 16):
			if (!isFixedLengthKey(key_type)) {
				EnableWindow(GetDlgItem(dlg, IDC_KEYBITS), FALSE);
				saved_key_bits = GetDlgItemInt(dlg, IDC_KEYBITS, NULL, FALSE);
			}
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), TRUE);
			if (SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), TRUE);
			}
			else {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), FALSE);
			}
			key_type = KEY_ECDSA521;
			SetDlgItemInt(dlg, IDC_KEYBITS, 521, FALSE);
			break;

		case IDC_ED25519_TYPE | (BN_CLICKED << 16):
			/* ED25519 ではビット数を指定できない。*/
			if (!isFixedLengthKey(key_type)) {
				EnableWindow(GetDlgItem(dlg, IDC_KEYBITS), FALSE);
				saved_key_bits = GetDlgItemInt(dlg, IDC_KEYBITS, NULL, FALSE);
			}
			SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_SETCHECK, BST_CHECKED, 0);
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), FALSE);
			EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), TRUE);
			key_type = KEY_ED25519;
			SetDlgItemInt(dlg, IDC_KEYBITS, 256, FALSE);
			break;

		case IDC_BCRYPT_KDF_CHECK | (BN_CLICKED << 16):
			if (SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), TRUE);
			}
			else {
				EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), FALSE);
			}
			break;

		// saving public key file
		case IDC_SAVE_PUBLIC_KEY:
			{
			int ret;
			OPENFILENAME ofn;
			char filename[MAX_PATH];
			FILE *fp;
			char comment[1024]; // comment string in private key

			arc4random_stir();

			// saving file dialog
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = dlg;
			switch (public_key.type) {
			case KEY_RSA1:
				UTIL_get_lang_msg("FILEDLG_SAVE_PUBLICKEY_RSA1_FILTER", pvar,
				                  "SSH1 RSA key(identity.pub)\\0identity.pub\\0All Files(*.*)\\0*.*\\0\\0");
				memcpy(uimsg, pvar->ts->UIMsg, sizeof(uimsg));
				ofn.lpstrFilter = uimsg;
				strncpy_s(filename, sizeof(filename), "identity.pub", _TRUNCATE);
				break;
			case KEY_RSA:
				UTIL_get_lang_msg("FILEDLG_SAVE_PUBLICKEY_RSA_FILTER", pvar,
				                  "SSH2 RSA key(id_rsa.pub)\\0id_rsa.pub\\0All Files(*.*)\\0*.*\\0\\0");
				memcpy(uimsg, pvar->ts->UIMsg, sizeof(uimsg));
				ofn.lpstrFilter = uimsg;
				strncpy_s(filename, sizeof(filename), "id_rsa.pub", _TRUNCATE);
				break;
			case KEY_DSA:
				UTIL_get_lang_msg("FILEDLG_SAVE_PUBLICKEY_DSA_FILTER", pvar,
				                  "SSH2 DSA key(id_dsa.pub)\\0id_dsa.pub\\0All Files(*.*)\\0*.*\\0\\0");
				memcpy(uimsg, pvar->ts->UIMsg, sizeof(uimsg));
				ofn.lpstrFilter = uimsg;
				strncpy_s(filename, sizeof(filename), "id_dsa.pub", _TRUNCATE);
				break;
			case KEY_ECDSA256:
			case KEY_ECDSA384:
			case KEY_ECDSA521:
				UTIL_get_lang_msg("FILEDLG_SAVE_PUBLICKEY_ECDSA_FILTER", pvar,
				                  "SSH2 ECDSA key(id_ecdsa.pub)\\0id_ecdsa.pub\\0All Files(*.*)\\0*.*\\0\\0");
				memcpy(uimsg, pvar->ts->UIMsg, sizeof(uimsg));
				ofn.lpstrFilter = uimsg;
				strncpy_s(filename, sizeof(filename), "id_ecdsa.pub", _TRUNCATE);
				break;
			case KEY_ED25519:
				UTIL_get_lang_msg("FILEDLG_SAVE_PUBLICKEY_ED25519_FILTER", pvar,
				                  "SSH2 ED25519 key(id_ed25519.pub)\\0id_ed25519.pub\\0All Files(*.*)\\0*.*\\0\\0");
				memcpy(uimsg, pvar->ts->UIMsg, sizeof(uimsg));
				ofn.lpstrFilter = uimsg;
				strncpy_s(filename, sizeof(filename), "id_ed25519.pub", _TRUNCATE);
				break;
			default:
				break;
			}
			ofn.lpstrFile = filename;
			ofn.nMaxFile = sizeof(filename);
			UTIL_get_lang_msg("FILEDLG_SAVE_PUBLICKEY_TITLE", pvar,
			                  "Save public key as:");
			ofn.lpstrTitle = pvar->ts->UIMsg;
			if (GetSaveFileName(&ofn) == 0) { // failure
				ret = CommDlgExtendedError();
				break;
			}

			GetDlgItemText(dlg, IDC_COMMENT_EDIT, comment, sizeof(comment));

			// saving public key file
			fp = fopen(filename, "wb");
			if (fp == NULL) {
				UTIL_get_lang_msg("MSG_SAVE_KEY_OPENFILE_ERROR", pvar,
				                  "Can't open key file");
				strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
				UTIL_get_lang_msg("MSG_ERROR", pvar, "ERROR");
				MessageBox(dlg, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
				break;
			}

			if (public_key.type == KEY_RSA1) { // SSH1 RSA
				RSA *rsa = public_key.rsa;
				int bits;
				char *buf;

				bits = BN_num_bits(rsa->n);
				fprintf(fp, "%u", bits);

				buf = BN_bn2dec(rsa->e);
				fprintf(fp, " %s", buf);
				OPENSSL_free(buf);

				buf = BN_bn2dec(rsa->n);
				fprintf(fp, " %s", buf);
				OPENSSL_free(buf);

			} else { // SSH2 RSA, DSA, ECDSA
				buffer_t *b;
				char *keyname, *s;
				DSA *dsa = public_key.dsa;
				RSA *rsa = public_key.rsa;
				EC_KEY *ecdsa = public_key.ecdsa;
				int len;
				char *blob;
				char *uuenc; // uuencode data
				int uulen;

				b = buffer_init();
				if (b == NULL)
					goto public_error;

				switch (public_key.type) {
				case KEY_DSA: // DSA
					keyname = "ssh-dss";
					buffer_put_string(b, keyname, strlen(keyname));
					buffer_put_bignum2(b, dsa->p);
					buffer_put_bignum2(b, dsa->q);
					buffer_put_bignum2(b, dsa->g);
					buffer_put_bignum2(b, dsa->pub_key);
					break;

				case KEY_RSA: // RSA
					keyname = "ssh-rsa";
					buffer_put_string(b, keyname, strlen(keyname));
					buffer_put_bignum2(b, rsa->e);
					buffer_put_bignum2(b, rsa->n);
					break;

				case KEY_ECDSA256: // ECDSA
				case KEY_ECDSA384:
				case KEY_ECDSA521:
					keyname = get_ssh_keytype_name(public_key.type);
					buffer_put_string(b, keyname, strlen(keyname));
					s = curve_keytype_to_name(public_key.type);
					buffer_put_string(b, s, strlen(s));
					buffer_put_ecpoint(b, EC_KEY_get0_group(ecdsa),
					                      EC_KEY_get0_public_key(ecdsa));
					break;

				case KEY_ED25519:
					keyname = get_ssh_keytype_name(public_key.type);
					buffer_put_cstring(b, keyname);
					buffer_put_string(b, public_key.ed25519_pk, ED25519_PK_SZ);
					break;
				}

				blob = buffer_ptr(b);
				len = buffer_len(b);
				uuenc = malloc(len * 2);
				if (uuenc == NULL) {
					buffer_free(b);
					goto public_error;
				}
				uulen = uuencode(blob, len, uuenc, len * 2);
				if (uulen > 0) {
					fprintf(fp, "%s %s", keyname, uuenc);
				}
				free(uuenc);
				buffer_free(b);
			}

			// writing a comment(+LF)
			if (comment[0] != 0) {
				fprintf(fp, " %s", comment);
			}
			fputc(0x0a, fp);

public_error:
			fclose(fp);

			}
			break;

		// saving private key file
		case IDC_SAVE_PRIVATE_KEY:
			{
			char buf[1024], buf_conf[1024];  // passphrase
			int ret, rounds;
			OPENFILENAME ofn;
			char filename[MAX_PATH];
			char comment[1024]; // comment string in private key

			// パスフレーズのチェックを行う。パスフレーズは秘密鍵ファイルに付ける。
			SendMessage(GetDlgItem(dlg, IDC_KEY_EDIT), WM_GETTEXT, sizeof(buf), (LPARAM)buf);
			SendMessage(GetDlgItem(dlg, IDC_CONFIRM_EDIT), WM_GETTEXT, sizeof(buf_conf), (LPARAM)buf_conf);

			// check matching
			if (strcmp(buf, buf_conf) != 0) {
				UTIL_get_lang_msg("MSG_SAVE_PRIVATE_KEY_MISMATCH_ERROR", pvar,
				                  "Two passphrases don't match.");
				strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
				UTIL_get_lang_msg("MSG_ERROR", pvar, "ERROR");
				MessageBox(dlg, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
				break;
			}

			// check empty-passphrase (this is warning level)
			if (buf[0] == '\0') {
				UTIL_get_lang_msg("MSG_SAVE_PRIVATEKEY_EMPTY_WARN", pvar,
				                  "Are you sure that you want to use a empty passphrase?");
				strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
				UTIL_get_lang_msg("MSG_WARNING", pvar, "WARNING");
				ret = MessageBox(dlg, uimsg, pvar->ts->UIMsg, MB_YESNO | MB_ICONWARNING);
				if (ret == IDNO)
					break;
			}

			// number of rounds
			if (SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				rounds = GetDlgItemInt(dlg, IDC_BCRYPT_KDF_ROUNDS, NULL, FALSE);
				if (rounds < SSH_KEYGEN_MINIMUM_ROUNDS) {
					UTIL_get_lang_msg("MSG_BCRYPT_ROUNDS_MIN_ERROR", pvar,
					                  "The number of rounds is too small.");
					MessageBox(dlg, pvar->ts->UIMsg,
					           "Tera Term", MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				if (rounds > SSH_KEYGEN_MAXIMUM_ROUNDS) {
					UTIL_get_lang_msg("MSG_BCRYPT_ROUNDS_MAX_ERROR", pvar,
					                  "The number of rounds is too large.");
					MessageBox(dlg, pvar->ts->UIMsg,
					           "Tera Term", MB_OK | MB_ICONEXCLAMATION);
					break;
				}
			}

			ssh_make_comment(comment, sizeof(comment));

			// saving file dialog
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = dlg;
			switch (private_key.type) {
			case KEY_RSA1:
				UTIL_get_lang_msg("FILEDLG_SAVE_PRIVATEKEY_RSA1_FILTER", pvar,
				                  "SSH1 RSA key(identity)\\0identity\\0All Files(*.*)\\0*.*\\0\\0");
				memcpy(uimsg, pvar->ts->UIMsg, sizeof(uimsg));
				ofn.lpstrFilter = uimsg;
				strncpy_s(filename, sizeof(filename), "identity", _TRUNCATE);
				break;
			case KEY_RSA:
				UTIL_get_lang_msg("FILEDLG_SAVE_PRIVATEKEY_RSA_FILTER", pvar,
				                  "SSH2 RSA key(id_rsa)\\0id_rsa\\0All Files(*.*)\\0*.*\\0\\0");
				memcpy(uimsg, pvar->ts->UIMsg, sizeof(uimsg));
				ofn.lpstrFilter = uimsg;
				strncpy_s(filename, sizeof(filename), "id_rsa", _TRUNCATE);
				break;
			case KEY_DSA:
				UTIL_get_lang_msg("FILEDLG_SAVE_PRIVATEKEY_DSA_FILTER", pvar,
				                  "SSH2 DSA key(id_dsa)\\0id_dsa\\0All Files(*.*)\\0*.*\\0\\0");
				memcpy(uimsg, pvar->ts->UIMsg, sizeof(uimsg));
				ofn.lpstrFilter = uimsg;
				strncpy_s(filename, sizeof(filename), "id_dsa", _TRUNCATE);
				break;
			case KEY_ECDSA256:
			case KEY_ECDSA384:
			case KEY_ECDSA521:
				UTIL_get_lang_msg("FILEDLG_SAVE_PRIVATEKEY_ECDSA_FILTER", pvar,
				                  "SSH2 ECDSA key(id_ecdsa)\\0id_ecdsa\\0All Files(*.*)\\0*.*\\0\\0");
				memcpy(uimsg, pvar->ts->UIMsg, sizeof(uimsg));
				ofn.lpstrFilter = uimsg;
				strncpy_s(filename, sizeof(filename), "id_ecdsa", _TRUNCATE);
				break;
			case KEY_ED25519:
				UTIL_get_lang_msg("FILEDLG_SAVE_PRIVATEKEY_ED25519_FILTER", pvar,
				                  "SSH2 ED25519 key(id_ed25519)\\0id_ed25519\\0All Files(*.*)\\0*.*\\0\\0");
				memcpy(uimsg, pvar->ts->UIMsg, sizeof(uimsg));
				ofn.lpstrFilter = uimsg;
				strncpy_s(filename, sizeof(filename), "id_ed25519", _TRUNCATE);
				break;
			default:
				break;
			}
			ofn.lpstrFile = filename;
			ofn.nMaxFile = sizeof(filename);
			UTIL_get_lang_msg("FILEDLG_SAVE_PRIVATEKEY_TITLE", pvar,
			                  "Save private key as:");
			ofn.lpstrTitle = pvar->ts->UIMsg;
			if (GetSaveFileName(&ofn) == 0) { // failure
				ret = CommDlgExtendedError();
				break;
			}

			// saving private key file
			if (private_key.type == KEY_RSA1) { // SSH1 RSA
				int cipher_num;
				buffer_t *b, *enc;
				unsigned int rnd;
				unsigned char tmp[128];
				RSA *rsa;
				int i, len;
				char authfile_id_string[] = "SSH PRIVATE KEY FILE FORMAT 1.1";
				MD5_CTX md;
				unsigned char digest[16];
				char *passphrase = buf;
				EVP_CIPHER_CTX cipher_ctx;
				FILE *fp;
				char wrapped[4096];

				if (passphrase[0] == '\0') { // passphrase is empty
					cipher_num = SSH_CIPHER_NONE;
				} else {
					cipher_num = SSH_CIPHER_3DES; // 3DES-CBC
				}

				b = buffer_init();
				if (b == NULL)
					break;
				enc = buffer_init();
				if (enc == NULL) {
					buffer_free(b);
					break;
				}

				// set random value
				rnd = arc4random();
				tmp[0] = rnd & 0xff;
				tmp[1] = (rnd >> 8) & 0xff;
				tmp[2] = tmp[0];
				tmp[3] = tmp[1];
				buffer_append(b, tmp, 4);

				// set private key
				rsa = private_key.rsa;
				buffer_put_bignum(b, rsa->d);
				buffer_put_bignum(b, rsa->iqmp);
				buffer_put_bignum(b, rsa->q);
				buffer_put_bignum(b, rsa->p);

				// padding with 8byte align
				while (buffer_len(b) % 8) {
					buffer_put_char(b, 0);
				}

				//
				// step(2)
				//
				// encrypted buffer
			    /* First store keyfile id string. */
				for (i = 0 ; authfile_id_string[i] ; i++) {
					buffer_put_char(enc, authfile_id_string[i]);
				}
				buffer_put_char(enc, 0x0a); // LF
				buffer_put_char(enc, 0);

				/* Store cipher type. */
				buffer_put_char(enc, cipher_num);
				buffer_put_int(enc, 0);  // type is 'int'!! (For future extension)

				/* Store public key.  This will be in plain text. */
				buffer_put_int(enc, BN_num_bits(rsa->n));
				buffer_put_bignum(enc, rsa->n);
				buffer_put_bignum(enc, rsa->e);
				buffer_put_string(enc, comment, strlen(comment));

				// setup the MD5ed passphrase to cipher encryption key
				MD5_Init(&md);
				MD5_Update(&md, (const unsigned char *)passphrase, strlen(passphrase));
				MD5_Final(digest, &md);
				if (cipher_num == SSH_CIPHER_NONE) {
					cipher_init_SSH2(&cipher_ctx, digest, 16, NULL, 0, CIPHER_ENCRYPT, EVP_enc_null(), 0, pvar);
				} else {
					cipher_init_SSH2(&cipher_ctx, digest, 16, NULL, 0, CIPHER_ENCRYPT, evp_ssh1_3des(), 0, pvar);
				}
				len = buffer_len(b);
				if (len % 8) { // fatal error
					goto error;
				}

				// check buffer overflow
				if (buffer_overflow_verify(enc, len) && (sizeof(wrapped) < len)) {
					goto error;
				}

				if (EVP_Cipher(&cipher_ctx, wrapped, buffer_ptr(b), len) == 0) {
					goto error;
				}
				if (EVP_CIPHER_CTX_cleanup(&cipher_ctx) == 0) {
					goto error;
				}

				buffer_append(enc, wrapped, len);

				// saving private key file (binary mode)
				fp = fopen(filename, "wb");
				if (fp == NULL) {
					UTIL_get_lang_msg("MSG_SAVE_KEY_OPENFILE_ERROR", pvar,
					                  "Can't open key file");
					strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
					UTIL_get_lang_msg("MSG_ERROR", pvar, "ERROR");
					MessageBox(dlg, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				fwrite(buffer_ptr(enc), buffer_len(enc), 1, fp);

				fclose(fp);

error:;
				buffer_free(b);
				buffer_free(enc);

			} else if (private_key.type == KEY_ED25519) { // SSH2 ED25519 
				save_bcrypt_private_key(buf, filename, comment, dlg, pvar, rounds);

			} else { // SSH2 RSA, DSA, ECDSA			
				int len;
				FILE *fp;
				const EVP_CIPHER *cipher;

				if (SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
					save_bcrypt_private_key(buf, filename, comment, dlg, pvar, rounds);
					break;
				}

				len = strlen(buf);
				// TODO: range check (len >= 4)

				cipher = NULL;
				if (len > 0) {
					cipher = EVP_aes_128_cbc();
				}

				fp = fopen(filename, "w");
				if (fp == NULL) {
					UTIL_get_lang_msg("MSG_SAVE_KEY_OPENFILE_ERROR", pvar,
					                  "Can't open key file");
					strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
					UTIL_get_lang_msg("MSG_ERROR", pvar, "ERROR");
					MessageBox(dlg, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
					break;
				}
 
				switch (key_type) {
				case KEY_RSA: // RSA
					ret = PEM_write_RSAPrivateKey(fp, private_key.rsa, cipher, buf, len, NULL, NULL);
					break;
				case KEY_DSA: // DSA
					ret = PEM_write_DSAPrivateKey(fp, private_key.dsa, cipher, buf, len, NULL, NULL);
					break;
				case KEY_ECDSA256: // ECDSA
				case KEY_ECDSA384:
				case KEY_ECDSA521:
					ret = PEM_write_ECPrivateKey(fp, private_key.ecdsa, cipher, buf, len, NULL, NULL);
					break;
				}
				if (ret == 0) {
					UTIL_get_lang_msg("MSG_SAVE_KEY_WRITEFILE_ERROR", pvar,
					                  "Can't open key file");
					strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
					UTIL_get_lang_msg("MSG_ERROR", pvar, "ERROR");
					MessageBox(dlg, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
				}
				fclose(fp);
			}


			}
			break;

		}
		break;
	}

	return FALSE;
}


static int PASCAL TTXProcessCommand(HWND hWin, WORD cmd)
{
	char uimsg[MAX_UIMSG];

	if (pvar->fatal_error) {
		return 0;
	}

	switch (cmd) {
	case 50430: // FIXME: ID_CONTROL_SENDBREAK(tt_res.h)を指定したいが、ヘッダをincludeすると、多重定義となる。
		if (SSH_notify_break_signal(pvar))
			return 1;
		else
			return 0;  // SSH2で処理されなかった場合は、本来の動作を行うべく、ゼロを返す。

	case ID_SSHSCPMENU:
		if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHSCP), hWin, TTXScpDialog,
			(LPARAM) pvar) == -1) {
			UTIL_get_lang_msg("MSG_CREATEWINDOW_SCP_ERROR", pvar,
			                  "Unable to display SCP dialog box.");
			strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
			UTIL_get_lang_msg("MSG_TTSSH_ERROR", pvar, "TTSSH Error");
			MessageBox(hWin, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
		}
		return 1;

	case ID_SSHKEYGENMENU:
		if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHKEYGEN), hWin, TTXKeyGenerator,
			(LPARAM) pvar) == -1) {
			UTIL_get_lang_msg("MSG_CREATEWINDOW_KEYGEN_ERROR", pvar,
			                  "Unable to display Key Generator dialog box.");
			strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
			UTIL_get_lang_msg("MSG_TTSSH_ERROR", pvar, "TTSSH Error");
			MessageBox(hWin, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
		}
		return 1;

	case ID_ABOUTMENU:
		if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ABOUTDIALOG),
		                   hWin, TTXAboutDlg, (LPARAM) pvar) == -1) {
			UTIL_get_lang_msg("MSG_CREATEWINDOW_ABOUT_ERROR", pvar,
			                  "Unable to display About dialog box.");
			strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
			UTIL_get_lang_msg("MSG_TTSSH_ERROR", pvar, "TTSSH Error");
			MessageBox(hWin, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
		}
		return 1;
	case ID_SSHAUTH:
		AUTH_do_cred_dialog(pvar);
		return 1;
	case ID_SSHSETUPMENU:
		if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHSETUP),
		                   hWin, TTXSetupDlg, (LPARAM) pvar) == -1) {
			UTIL_get_lang_msg("MSG_CREATEWINDOW_SETUP_ERROR", pvar,
			                  "Unable to display TTSSH Setup dialog box.");
			strncpy_s(uimsg, sizeof(uimsg), pvar->ts->UIMsg, _TRUNCATE);
			UTIL_get_lang_msg("MSG_TTSSH_ERROR", pvar, "TTSSH Error");
			MessageBox(hWin, uimsg, pvar->ts->UIMsg, MB_OK | MB_ICONEXCLAMATION);
		}
		return 1;
	case ID_SSHAUTHSETUPMENU:
		AUTH_do_default_cred_dialog(pvar);
		return 1;
	case ID_SSHFWDSETUPMENU:
		FWDUI_do_forwarding_dialog(pvar);
		return 1;
	case ID_SSHUNKNOWNHOST:
		HOSTS_do_unknown_host_dialog(hWin, pvar);
		return 1;
	case ID_SSHDIFFERENTKEY:
		HOSTS_do_different_key_dialog(hWin, pvar);
		return 1;
	case ID_SSHASYNCMESSAGEBOX:
		if (pvar->err_msg != NULL) {
			char *msg = pvar->err_msg;

			/* Could there be a buffer overrun bug anywhere in Win32
			   MessageBox? Who knows? I'm paranoid. */
			if (strlen(msg) > 2048) {
				msg[2048] = 0;
			}

			pvar->showing_err = TRUE;
			pvar->err_msg = NULL;
			MessageBox(NULL, msg, "TTSSH",
					   MB_TASKMODAL | MB_ICONEXCLAMATION);
			free(msg);
			pvar->showing_err = FALSE;

			if (pvar->err_msg != NULL) {
				PostMessage(hWin, WM_COMMAND, ID_SSHASYNCMESSAGEBOX, 0);
			} else {
				AUTH_notify_end_error(pvar);
			}
		}
		return 1;
	default:
		return 0;
	}
}


static void _dquote_string(char *str, char *dst, int dst_len)
{
	int i, len, n;
	
	len = strlen(str);
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

static void dquote_string(char *str, char *dst, int dst_len)
{
	// ",スペース,;,^A-^_ が含まれる場合にはクオートする
	if (strchr(str, '"') != NULL ||
	    strchr(str, ' ') != NULL ||
	    strchr(str, ';') != NULL ||
	    strchr(str, 0x01) != NULL ||
	    strchr(str, 0x02) != NULL ||
	    strchr(str, 0x03) != NULL ||
	    strchr(str, 0x04) != NULL ||
	    strchr(str, 0x05) != NULL ||
	    strchr(str, 0x06) != NULL ||
	    strchr(str, 0x07) != NULL ||
	    strchr(str, 0x08) != NULL ||
	    strchr(str, 0x09) != NULL ||
	    strchr(str, 0x0a) != NULL ||
	    strchr(str, 0x0b) != NULL ||
	    strchr(str, 0x0c) != NULL ||
	    strchr(str, 0x0d) != NULL ||
	    strchr(str, 0x0e) != NULL ||
	    strchr(str, 0x0f) != NULL ||
	    strchr(str, 0x10) != NULL ||
	    strchr(str, 0x11) != NULL ||
	    strchr(str, 0x12) != NULL ||
	    strchr(str, 0x13) != NULL ||
	    strchr(str, 0x14) != NULL ||
	    strchr(str, 0x15) != NULL ||
	    strchr(str, 0x16) != NULL ||
	    strchr(str, 0x17) != NULL ||
	    strchr(str, 0x18) != NULL ||
	    strchr(str, 0x19) != NULL ||
	    strchr(str, 0x1a) != NULL ||
	    strchr(str, 0x1b) != NULL ||
	    strchr(str, 0x1c) != NULL ||
	    strchr(str, 0x1d) != NULL ||
	    strchr(str, 0x1e) != NULL ||
	    strchr(str, 0x1f) != NULL) {
		_dquote_string(str, dst, dst_len);
		return;
	}
	// そのままコピーして戻る
	strncpy_s(dst, dst_len, str, _TRUNCATE);
}

static void PASCAL TTXSetCommandLine(PCHAR cmd, int cmdlen,
                                         PGetHNRec rec)
{
	char tmpFile[MAX_PATH];
	char tmpPath[1024];
	char *buf;
	char *p;
	int i;

	GetTempPath(sizeof(tmpPath), tmpPath);
	GetTempFileName(tmpPath, "TTX", 0, tmpFile);

	for (i = 0; cmd[i] != ' ' && cmd[i] != 0; i++) {
	}

	if (i < cmdlen) {
		int ssh_enable = -1;

		buf = malloc(cmdlen+1);
		strncpy_s(buf, cmdlen, cmd + i, _TRUNCATE);
		buf[cmdlen] = 0;
		cmd[i] = 0;

		write_ssh_options(pvar, tmpFile, &pvar->settings, FALSE);

		strncat_s(cmd, cmdlen, " /ssh-consume=", _TRUNCATE);
		strncat_s(cmd, cmdlen, tmpFile, _TRUNCATE);

		strncat_s(cmd, cmdlen, buf, _TRUNCATE);

		// コマンドラインでの指定をチェック
		if (p = strstr(buf, " /ssh")) {
			switch (*(p + 5)) {
				case '\0':
				case ' ':
				case '1':
				case '2':
					ssh_enable = 1;
			}
		}
		else if (strstr(buf, " /nossh") ||
		         strstr(buf, " /telnet")) {
			ssh_enable = 0;
		}

		// ホスト名で /ssh /1, /ssh  /2, /ssh1, /ssh2, /nossh, /telnet が
		// 指定されたときは、ラジオボタンの SSH および SSH プロトコルバージョンを
		// 適用するのをやめる (2007.11.1 maya)
		if (pvar->hostdlg_Enabled && ssh_enable == -1) {
			strncat_s(cmd, cmdlen, " /ssh", _TRUNCATE);

			// add option of SSH protcol version (2004.10.11 yutaka)
			if (pvar->settings.ssh_protocol_version == 2) {
				strncat_s(cmd, cmdlen, " /2", _TRUNCATE);
			} else {
				strncat_s(cmd, cmdlen, " /1", _TRUNCATE);
			}

		}

		// セッション複製の場合は、自動ログイン用パラメータを付ける。(2005.4.8 yutaka)
		if (strstr(buf, "DUPLICATE")) {
			char mark[MAX_PATH];
			char tmp[MAX_PATH*2];

			// 自動ログインの場合は下記フラグが0のため、必要なコマンドを付加する。
			if (!pvar->hostdlg_Enabled) {
				_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
					" /ssh /%d", pvar->settings.ssh_protocol_version);
				strncat_s(cmd, cmdlen, tmp, _TRUNCATE);
			}

			// パスワードを覚えている場合のみ、コマンドラインに渡す。(2006.8.3 yutaka)
			if (pvar->settings.remember_password &&
			    pvar->auth_state.cur_cred.method == SSH_AUTH_PASSWORD) {
				dquote_string(pvar->auth_state.cur_cred.password, mark, sizeof(mark));
				_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
				            " /auth=password /user=%s /passwd=%s", pvar->auth_state.user, mark);
				strncat_s(cmd, cmdlen, tmp, _TRUNCATE);

			} else if (pvar->settings.remember_password &&
			           pvar->auth_state.cur_cred.method == SSH_AUTH_RSA) {
				dquote_string(pvar->auth_state.cur_cred.password, mark, sizeof(mark));
				_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
				            " /auth=publickey /user=%s /passwd=%s", pvar->auth_state.user, mark);
				strncat_s(cmd, cmdlen, tmp, _TRUNCATE);

				dquote_string(pvar->session_settings.DefaultRSAPrivateKeyFile, mark, sizeof(mark));
				_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, " /keyfile=%s", mark);
				strncat_s(cmd, cmdlen, tmp, _TRUNCATE);

			} else if (pvar->settings.remember_password &&
			           pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) {
				dquote_string(pvar->auth_state.cur_cred.password, mark, sizeof(mark));
				_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
				            " /auth=challenge /user=%s /passwd=%s", pvar->auth_state.user, mark);
				strncat_s(cmd, cmdlen, tmp, _TRUNCATE);

			} else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT) {
				_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
				            " /auth=pageant /user=%s", pvar->auth_state.user);
				strncat_s(cmd, cmdlen, tmp, _TRUNCATE);

			} else {
				// don't come here

			}

		}
		free(buf);
	}
}

/* This function is called when Tera Term is quitting. You can use it to clean
   up.

   This function is called for each extension, in reverse load order (see
   below).
*/
static void PASCAL TTXEnd(void)
{
	uninit_TTSSH(pvar);

	if (pvar->err_msg != NULL) {
		/* Could there be a buffer overrun bug anywhere in Win32
		   MessageBox? Who knows? I'm paranoid. */
		if (strlen(pvar->err_msg) > 2048) {
			pvar->err_msg[2048] = 0;
		}

		MessageBox(NULL, pvar->err_msg, "TTSSH",
		           MB_TASKMODAL | MB_ICONEXCLAMATION);

		free(pvar->err_msg);
		pvar->err_msg = NULL;
	}
}

/* This record contains all the information that the extension forwards to the
   main Tera Term code. It mostly consists of pointers to the above functions.
   Any of the function pointers can be replaced with NULL, in which case
   Tera Term will just ignore that function and assume default behaviour, which
   means "do nothing".
*/
static TTXExports Exports = {
/* This must contain the size of the structure. See below for its usage. */
	sizeof(TTXExports),
	ORDER,

/* Now we just list the functions that we've implemented. */
	TTXInit,
	TTXGetUIHooks,
	TTXGetSetupHooks,
	TTXOpenTCP,
	TTXCloseTCP,
	TTXSetWinSize,
	TTXModifyMenu,
	TTXModifyPopupMenu,
	TTXProcessCommand,
	TTXEnd,
	TTXSetCommandLine
};

BOOL __declspec(dllexport)
PASCAL TTXBind(WORD Version, TTXExports *exports)
{
	int size = sizeof(Exports) - sizeof(exports->size);
	/* do version checking if necessary */
	/* if (Version!=TTVERSION) return FALSE; */

	if (size > exports->size) {
		size = exports->size;
	}
	memcpy((char *) exports + sizeof(exports->size),
		   (char *) &Exports + sizeof(exports->size), size);
	return TRUE;
}

static HANDLE __mem_mapping = NULL;

BOOL WINAPI DllMain(HANDLE hInstance,
                    ULONG ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_THREAD_ATTACH:
		/* do thread initialization */
		break;
	case DLL_THREAD_DETACH:
		/* do thread cleanup */
		break;
	case DLL_PROCESS_ATTACH:
		/* do process initialization */
#ifdef _DEBUG
  //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  // リーク時のブロック番号を元にブレークを仕掛けるには、以下のようにする。
  // cf. http://www.microsoft.com/japan/msdn/vs_previous/visualc/techmat/feature/MemLeaks/
  //_CrtSetBreakAlloc(3228);
#endif
		DoCover_IsDebuggerPresent();
		DisableThreadLibraryCalls(hInstance);
		hInst = hInstance;
		pvar = &InstVar;
		__mem_mapping =
			CreateFileMapping((HANDLE) 0xFFFFFFFF, NULL, PAGE_READWRITE, 0,
			                  sizeof(TS_SSH), TTSSH_FILEMAPNAME);
		if (__mem_mapping == NULL) {
			/* fake it. The settings won't be shared, but what the heck. */
			pvar->ts_SSH = NULL;
		} else {
			pvar->ts_SSH =
				(TS_SSH *) MapViewOfFile(__mem_mapping, FILE_MAP_WRITE, 0,
				                         0, 0);
		}
		if (pvar->ts_SSH == NULL) {
			/* fake it. The settings won't be shared, but what the heck. */
			pvar->ts_SSH = (TS_SSH *) malloc(sizeof(TS_SSH));
			if (__mem_mapping != NULL) {
				CloseHandle(__mem_mapping);
			}
		}
		break;
	case DLL_PROCESS_DETACH:
		/* do process cleanup */
		if (__mem_mapping == NULL) {
			free(pvar->ts_SSH);
		} else {
			CloseHandle(__mem_mapping);
			UnmapViewOfFile(pvar->ts_SSH);
		}
		break;
	}
	return TRUE;
}
