/*
 * Copyright (c) 1998-2020, Robert O'Callahan
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

/* Tera Term extension mechanism
   Robert O'Callahan (roc+tt@cs.cmu.edu)

   Tera Term by Takashi Teranishi (teranishi@rikaxp.riken.go.jp)
*/

#include "ttxssh.h"
#include "ttxssh-version.h"
#include "fwdui.h"
#include "util.h"
#include "ssh.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "keyfiles.h"
#include "auth.h"
#include "helpid.h"
#include "ttcommdlg.h"
#include "ttlib_types.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <locale.h>		// for setlocale()
#include <assert.h>

#include "resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <winsock2.h>
#include <wchar.h>

#include <lmcons.h>

// include OpenSSL header file
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rc4.h>
#include <openssl/md5.h>

// for legacy cipher
#if !defined(LIBRESSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000UL
#include <openssl/provider.h>
#define OPENSSL_LEGACY_PROVIDER
#endif

#ifndef LIBRESSL_VERSION_NUMBER
  #include "arc4random.h"
#else
  // include LibreSSL header file
  #include <compat/stdlib.h>
#endif

// include ZLib header file
#include <zlib.h>

#include "buffer.h"
#include "cipher.h"
#include "key.h"
#include "kex.h"
#include "mac.h"
#include "comp.h"
#include "dlglib.h"
#include "win32helper.h"

#include "sftp.h"

#include "compat_win.h"
#include "codeconv.h"
#include "inifile_com.h"
#include "asprintf.h"
#include "win32helper.h"
#include "comportinfo.h"
#include "asprintf.h"
#include "ttcommdlg.h"
#include "resize_helper.h"
#include "libputty.h"
#include "edithistory.h"

#ifdef OPENSSL_LEGACY_PROVIDER
static OSSL_PROVIDER *legacy_provider = NULL;
static OSSL_PROVIDER *default_provider = NULL;
#endif

#undef GetPrivateProfileInt
#undef GetPrivateProfileString
#undef WritePrivateProfileString
#define GetPrivateProfileInt(p1, p2, p3, p4) GetPrivateProfileIntAFileW(p1, p2, p3, p4)
#define GetPrivateProfileString(p1, p2, p3, p4, p5, p6) GetPrivateProfileStringAFileW(p1, p2, p3, p4, p5, p6)
#define WritePrivateProfileString(p1, p2, p3, p4) WritePrivateProfileStringAFileW(p1, p2, p3, p4)

/* This extension implements SSH, so we choose a load order in the
   "protocols" range. */
#define ORDER 2500

HANDLE hInst; /* Instance handle of TTXSSH.DLL */

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

	/*
	 * pvar->contents_after_known_hosts は意図的に
	 * init_TTSSH()やuninit_TTSSH()では初期化や解放をしない。
	 * なぜならば、known_hostsダイアログで使用するためであり、
	 * ダイアログの表示中に TTXCloseTCP() が呼び出されることにより、
	 * pvar->contents_after_known_hosts が初期化や解放されては困るからである。
	 */

	PKT_init(pvar);
	SSH_init(pvar);
	CRYPT_init(pvar);
	AUTH_init(pvar);
	HOSTS_init(pvar);
	FWD_init(pvar);
	FWDUI_init(pvar);

	ssh_scp_thread_lock_initialize();
	ssh_heartbeat_lock_initialize();

	pvar->cc[MODE_IN] = NULL;
	pvar->cc[MODE_OUT] = NULL;
	// メモリ確保は CRYPT_start_encryption の先の cipher_init_SSH2 に移動
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

	// VT ウィンドウのアイコン
	SetVTIconID(pvar->cv, NULL, 0);

	ssh_scp_thread_lock_finalize();
	ssh_heartbeat_lock_finalize();

	cipher_free_SSH2(pvar->cc[MODE_IN]);
	cipher_free_SSH2(pvar->cc[MODE_OUT]);

	// CloseTCP と TTXEnd から 2 回呼ばれる場合があるため、2重 free しないように NULL をセットしておく
	pvar->cc[MODE_IN] = NULL;
	pvar->cc[MODE_OUT] = NULL;
}

static void PASCAL TTXInit(PTTSet ts, PComVar cv)
{
	pvar->settings = *pvar->ts_SSH;
	pvar->ts = ts;
	pvar->cv = cv;
	pvar->fatal_error = FALSE;
	pvar->showing_err = FALSE;
	pvar->err_msg = NULL;

#ifdef OPENSSL_LEGACY_PROVIDER
	// OpenSSL 3 以降で以下のアルゴリズムを使えるようにするため
	// - blowfish-cbc
	// - cast128-cbc
	// - arcfour
	// - arcfour128
	// - arcfour256
	// - hmac-ripemd160@openssh.com
	// - hmac-ripemd160-etm@openssh.com
	if (legacy_provider == NULL) {
		legacy_provider = OSSL_PROVIDER_load(NULL, "legacy");
	}
	if (default_provider == NULL) {
		default_provider = OSSL_PROVIDER_load(NULL, "default");
	}
#endif

	init_TTSSH(pvar);
}


/* Remove local settings from the shared memory block. */
static void clear_local_settings(PTInstVar pvar)
{
	pvar->ts_SSH->TryDefaultAuth = FALSE;
}

static BOOL read_BOOL_option(const wchar_t *fileName, char *keyName, BOOL def)
{
	char buf[1024];

	buf[0] = 0;
	GetPrivateProfileStringAFileW("TTSSH", keyName, "", buf, sizeof(buf),
								  fileName);
	if (buf[0] == 0) {
		return def;
	} else {
		return atoi(buf) != 0 ||
		       _stricmp(buf, "yes") == 0 ||
		       _stricmp(buf, "y") == 0;
	}
}

static void read_string_option(const wchar_t *fileName, char *keyName,
                               char *def, char *buf, int bufSize)
{
	buf[0] = 0;
	GetPrivateProfileStringAFileW("TTSSH", keyName, def, buf, bufSize, fileName);
}

static void read_string_optionW(const wchar_t *fileName, const char *keyName,
                                char *def, wchar_t *bufW, size_t bufSize)
{
	wchar_t *keyW = ToWcharA(keyName);
	wchar_t *defW = ToWcharA(def);
	bufW[0] = 0;
	GetPrivateProfileStringW(L"TTSSH", keyW, defW, bufW, (DWORD)bufSize, fileName);
	free(keyW);
	free(defW);
}

static void read_ssh_options(PTInstVar pvar, const wchar_t *fileName)
{
	char buf[1024];
	TS_SSH *settings = pvar->ts_SSH;

#define READ_STD_STRING_OPTION(name) \
	read_string_option(fileName, #name, "", settings->name, sizeof(settings->name))
#define READ_STD_STRING_OPTIONW(name) \
	read_string_optionW(fileName, #name, "", settings->name, sizeof(settings->name))

	settings->Enabled = read_BOOL_option(fileName, "Enabled", FALSE);

	buf[0] = 0;
	GetPrivateProfileString("TTSSH", "Compression", "", buf, sizeof(buf),
	                        fileName);
	settings->CompressionLevel = atoi(buf);
	if (settings->CompressionLevel < 0 || settings->CompressionLevel > 9) {
		settings->CompressionLevel = 0;
	}

	READ_STD_STRING_OPTIONW(DefaultUserName);
	settings->DefaultUserType = GetPrivateProfileInt("TTSSH", "DefaultUserType", 1, fileName);

	READ_STD_STRING_OPTION(DefaultForwarding);
	READ_STD_STRING_OPTION(DefaultRhostsLocalUserName);
	READ_STD_STRING_OPTIONW(DefaultRhostsHostPrivateKeyFile);
	READ_STD_STRING_OPTIONW(DefaultRSAPrivateKeyFile);

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
	// Sign algorithm order of RSA publickey authentication
	READ_STD_STRING_OPTION(RSAPubkeySignAlgorithmOrder);
	normalize_rsa_pubkey_sign_algo_order(settings->RSAPubkeySignAlgorithmOrder);

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
	settings->ssh_heartbeat_overtime = GetPrivateProfileInt("TTSSH", "HeartBeat", 300, fileName);

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
	else if ((_stricmp(buf, "flat") == 0) ||
	         (_stricmp(buf, "securett_flat") == 0)) {
		settings->IconID = IDI_SECURETT_FLAT;
	}
	else {
		settings->IconID = IDI_SECURETT;
	}

	// エラーおよび警告時のポップアップメッセージを抑止する (2014.6.26 yutaka)
	settings->DisablePopupMessage = GetPrivateProfileInt("TTSSH", "DisablePopupMessage", 0, fileName);

	READ_STD_STRING_OPTION(X11Display);

	settings->UpdateHostkeys = GetPrivateProfileInt("TTSSH", "UpdateHostkeys", 0, fileName);

	settings->GexMinimalGroupSize = GetPrivateProfileInt("TTSSH", "GexMinimalGroupSize", 0, fileName);

	settings->AuthBanner = GetPrivateProfileInt("TTSSH", "AuthBanner", 3, fileName);

	settings->MaxChannel = GetPrivateProfileInt("TTSSH", "MaxChannel", CHANNEL_MAX_DEFAULT, fileName);

#ifdef _DEBUG
	GetPrivateProfileStringW(L"TTSSH", L"KexKeyLogFile", L"", settings->KexKeyLogFile, _countof(settings->KexKeyLogFile), fileName);
	if (settings->KexKeyLogFile[0] == 0) {
		settings->KexKeyLogging = 0;
	}
	else {
		settings->KexKeyLogging = GetPrivateProfileInt("TTSSH", "KexKeyLogging", 0, fileName);
	}
#else
	settings->KexKeyLogFile[0] = 0;
	settings->KexKeyLogging = 0;
#endif

	clear_local_settings(pvar);
}

static void write_ssh_options(PTInstVar pvar, const wchar_t *fileName,
                              TS_SSH *settings, BOOL copy_forward)
{
	char buf[1024];

	WritePrivateProfileString("TTSSH", "Enabled",
	                          settings->Enabled ? "1" : "0", fileName);

	_itoa(settings->CompressionLevel, buf, 10);
	WritePrivateProfileString("TTSSH", "Compression", buf, fileName);

	_itoa(settings->DefaultUserType, buf, 10);
	WritePrivateProfileString("TTSSH", "DefaultUserType", buf, fileName);
	WritePrivateProfileStringW(L"TTSSH", L"DefaultUserName",
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

	WritePrivateProfileString("TTSSH", "RSAPubkeySignAlgorithmOrder",
	                          settings->RSAPubkeySignAlgorithmOrder, fileName);

	WritePrivateProfileString("TTSSH", "KnownHostsFiles",
	                          settings->KnownHostsFiles, fileName);

	WritePrivateProfileString("TTSSH", "DefaultRhostsLocalUserName",
	                          settings->DefaultRhostsLocalUserName,
	                          fileName);

	WritePrivateProfileStringW(L"TTSSH", L"DefaultRhostsHostPrivateKeyFile",
	                           settings->DefaultRhostsHostPrivateKeyFile,
	                           fileName);

	WritePrivateProfileStringW(L"TTSSH", L"DefaultRSAPrivateKeyFile",
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
	else if (settings->IconID==IDI_SECURETT_FLAT) {
		WritePrivateProfileString("TTSSH", "SSHIcon", "flat", fileName);
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

	_itoa_s(settings->AuthBanner, buf, sizeof(buf), 10);
	WritePrivateProfileString("TTSSH", "AuthBanner", buf, fileName);

	_itoa_s(settings->MaxChannel, buf, sizeof(buf), 10);
	WritePrivateProfileString("TTSSH", "MaxChannel", buf, fileName);

#ifdef _DEBUG
	WritePrivateProfileStringW(L"TTSSH", L"KexKeyLogFile", settings->KexKeyLogFile, fileName);
	WritePrivateProfileString("TTSSH", "KexKeyLogging",
	                          settings->KexKeyLogging ? "1" : "0", fileName);
#endif
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
			((struct sockaddr_in6 *) &ss)->sin6_addr = in6addr_any;
			((struct sockaddr_in6 *) &ss)->sin6_port =
				htons(find_local_port(pvar));
			break;
		default:
			/* UNSPEC */
			len = sizeof(ss);
			ss.ss_family = AF_UNSPEC;
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
			// AUTH_advance_to_next_cred()の呼び出しを削除する。
			// NotificationWindowにハンドルは設定しておくが、このタイミングでは
			// 認証ダイアログを出すのは早すぎた。ProxyやNAT経由でサーバに接続
			// できない場合、すでに切断状態にも関わらず、認証ダイアログが
			// 表示されたままとなっていた。
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
	// VT ウィンドウのアイコン
	SetVTIconID(pvar->cv, hInst, pvar->settings.IconID);

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

// non-fatalおよびfatal時のエラーメッセージを覚えておく。
// 一度、覚えたメッセージがあれば、改行を挟んで追加していく。
static void add_err_msg(PTInstVar pvar, char *msg)
{
	if (pvar->err_msg != NULL) {
		int buf_len;
		char *buf;

		// すでに同じメッセージが登録済みの場合は追加しない。
		if (strstr(pvar->err_msg, msg))
			return;

		buf_len = strlen(pvar->err_msg) + 3 + strlen(msg);
		buf = malloc(buf_len);
		// メモリが確保できない場合は何もしない。
		if (buf == NULL)
			return;

		strncpy_s(buf, buf_len, pvar->err_msg, _TRUNCATE);
		strncat_s(buf, buf_len, "\n\n", _TRUNCATE);
		strncat_s(buf, buf_len, msg, _TRUNCATE);
		free(pvar->err_msg);
		pvar->err_msg = buf;
	} else {
		// メモリが確保できない場合は、_strdup()はNULLを返す。
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
			MessageBox(NULL, msg, pvar->UIMsg, MB_OK|MB_ICONINFORMATION);
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
		char *buf;
		char *strtime;
		int len;
		int file;
		BOOL enable_log = TRUE;
		BOOL enable_outputdebugstring = FALSE;

		strtime = mctimelocal("%Y-%m-%d %H:%M:%S.%NZ", TRUE);
		len = asprintf(&buf, "%s [%lu] %s\n",
					   strtime, GetCurrentProcessId(), msg);
		free(strtime);

		if (enable_log) {
			wchar_t *fname = get_log_dir_relative_nameW(L"TTSSH.LOG");
			file = _wopen(fname, _O_RDWR | _O_APPEND | _O_CREAT | _O_TEXT,
						  _S_IREAD | _S_IWRITE);
			free(fname);

			if (file >= 0) {
				_write(file, buf, len - 1);	// len includes '\0'
				_close(file);
			}
		}

		if (enable_outputdebugstring) {
			OutputDebugStringA(buf);
		}

		free(buf);
	}
}

#if defined(_MSC_VER)
void logprintf(int level, _Printf_format_string_ const char *fmt, ...)
#else
void logprintf(int level, const char *fmt, ...)
#endif
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

#if defined(_MSC_VER)
void logprintf_hexdump(int level, const char *data, int len, _Printf_format_string_ const char *fmt, ...)
#else
void logprintf_hexdump(int level, const char *data, int len, const char *fmt, ...)
#endif
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

		// 接続直前のここで、設定を myproposal に反映している
		//   キー再作成のときは、同じ SSH2_update_kex_myproposal() を呼び出して
		//   myproposal から ",ext-info-c" を削除している
		SSH2_update_kex_myproposal(pvar);
		SSH2_update_host_key_myproposal(pvar);
		SSH2_update_cipher_myproposal(pvar);
		SSH2_update_hmac_myproposal(pvar);
		SSH2_update_compression_myproposal(pvar);
	}
}

static void enable_dlg_items(HWND dlg, int from, int to, BOOL enabled)
{
	for (; from <= to; from++) {
		EnableWindow(GetDlgItem(dlg, from), enabled);
	}
}

typedef struct {
	PGetHNRec GetHNRec;
	ComPortInfo_t *ComPortInfoPtr;
	int ComPortInfoCount;
	BOOL HostDropOpen;
} TTXHostDlgData;

static BOOL IsEditHistorySelected(HWND dlg)
{
	LRESULT index = SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_GETCURSEL, 0, 0);
	LRESULT data = SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_GETITEMDATA, (WPARAM)index, 0);
	return data == 999;
}

static void SetHostDropdown(HWND dlg, const TTTSet *ts)
{
	LRESULT index;
	SetComboBoxHostHistory(dlg, IDC_HOSTNAME, MAXHOSTLIST, ts->SetupFNameW);
	ExpandCBWidth(dlg, IDC_HOSTNAME);
	SendDlgItemMessage(dlg, IDC_HOSTNAME, EM_LIMITTEXT, HostNameMaxLength - 1, 0);
	if (SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_GETCOUNT, 0, 0) != 0) {
		// 一番最初を選択する
		SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_SETCURSEL, 0, 0);
	} else {
		// 選択しない、エディットボックスは空になる
		SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_SETCURSEL, -1, 0);
	}

	// Edit historyを追加(ITEMDATA=999)
	index = SendDlgItemMessageW(dlg, IDC_HOSTNAME, CB_ADDSTRING, 0, (LPARAM)L"<Edit host list...>");
	SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_SETITEMDATA, index, 999);
}

static void OpenEditHistory(HWND dlg, TTTSet *ts)
{
	EditHistoryDlgData data = {0};
	data.UILanguageFileW = ts->UILanguageFileW;
	data.SetupFNameW = ts->SetupFNameW;
	data.vtwin = pvar->cv->HWin;
	data.title = L"Edit Host list";
	if (EditHistoryDlg(NULL, dlg, &data)) {
		// 編集されたので、ドロップダウンを再設定する
		SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_RESETCONTENT, 0, 0);

		SetHostDropdown(dlg, ts);
	}

	if (SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_GETCOUNT, 0, 0) > 1) {
		// 一番最初を選択する
		SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_SETCURSEL, 0, 0);
	} else {
		// "<Edit History...>のみ、選択しない、エディットボックスは空になる
		SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_SETCURSEL, -1, 0);
	}
}

static INT_PTR CALLBACK TTXHostDlg(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_HOST_TITLE" },
		{ IDC_HOSTNAMELABEL, "DLG_HOST_TCPIPHOST" },
		{ IDC_HISTORY, "DLG_HOST_TCPIPHISTORY" },
		{ IDC_SERVICELABEL, "DLG_HOST_TCPIPSERVICE" },
		{ IDC_HOSTOTHER, "DLG_HOST_TCPIPOTHER" },
		{ IDC_HOSTTCPPORTLABEL, "DLG_HOST_TCPIPPORT" },
		{ IDC_SSH_VERSION_LABEL, "DLG_HOST_TCPIPSSHVERSION" },
		{ IDC_HOSTTCPPROTOCOLLABEL, "DLG_HOST_TCPIPPROTOCOL" },
		{ IDC_HOSTSERIAL, "DLG_HOST_SERIAL" },
		{ IDC_HOSTCOMLABEL, "DLG_HOST_SERIALPORT" },
		{ IDC_HOSTHELP, "DLG_HOST_HELP" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
	};
	static const char *ssh_version[] = {"SSH1", "SSH2", NULL};
	static const char *ProtocolFamilyList[] = { "AUTO", "IPv6", "IPv4", NULL };
	TTXHostDlgData *dlg_data = (TTXHostDlgData *)GetWindowLongPtr(dlg, DWLP_USER);
	PGetHNRec GetHNRec = dlg_data != NULL ? dlg_data->GetHNRec : NULL;
	WORD i;

	switch (msg) {
	case WM_INITDIALOG: {
		int j;
		int com_index;

		GetHNRec = (PGetHNRec)lParam;
		dlg_data = (TTXHostDlgData *)calloc(1, sizeof(*dlg_data));
		SetWindowLongPtr(dlg, DWLP_USER, (LPARAM)dlg_data);
		dlg_data->GetHNRec = GetHNRec;

		dlg_data->ComPortInfoPtr = ComPortInfoGet(&dlg_data->ComPortInfoCount);
		dlg_data->HostDropOpen = FALSE;

		SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

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

		// ホスト (コマンドライン)
		SetHostDropdown(dlg, pvar->ts);
		SetEditboxEmacsKeybind(dlg, IDC_HOSTNAME);

		CheckRadioButton(dlg, IDC_HOSTTELNET, IDC_HOSTOTHER,
		                 pvar->settings.Enabled ? IDC_HOSTSSH : GetHNRec->
		                 Telnet ? IDC_HOSTTELNET : IDC_HOSTOTHER);
		SendDlgItemMessage(dlg, IDC_HOSTTCPPORT, EM_LIMITTEXT, 5, 0);
		SetDlgItemInt(dlg, IDC_HOSTTCPPORT, GetHNRec->TCPPort, FALSE);
		for (i = 0; ProtocolFamilyList[i]; ++i) {
			SendDlgItemMessage(dlg, IDC_HOSTTCPPROTOCOL, CB_ADDSTRING,
			                   0, (LPARAM) ProtocolFamilyList[i]);
		}
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
		com_index = 1;
		for (i = 0; i < dlg_data->ComPortInfoCount; i++) {
			ComPortInfo_t *p = dlg_data->ComPortInfoPtr + i;
			wchar_t *EntNameW;
			int index;

			// MaxComPort を越えるポートは表示しない
			if (GetHNRec->MaxComPort >= 0 && i > GetHNRec->MaxComPort) {
				continue;
			}

			// 使用中のポートは表示しない
			if (CheckCOMFlag(p->port_no) == 1) {
				continue;
			}

			j++;
			if (GetHNRec->ComPort == p->port_no) {
				com_index = j;
			}

			if (p->friendly_name == NULL) {
				aswprintf(&EntNameW, L"%s", p->port_name);
			}
			else {
				aswprintf(&EntNameW, L"%s: %s", p->port_name, p->friendly_name);
			}
			index = (int)SendDlgItemMessageW(dlg, IDC_HOSTCOM, CB_ADDSTRING, 0, (LPARAM)EntNameW);
			SendDlgItemMessageA(dlg, IDC_HOSTCOM, CB_SETITEMDATA, index, i);
			free(EntNameW);
		}
		if (j > 0) {
			// GetHNRec->ComPort を選択する
			SendDlgItemMessageA(dlg, IDC_HOSTCOM, CB_SETCURSEL, com_index - 1, 0);
		} else {					/* All com ports are already used */
			GetHNRec->PortType = IdTCPIP;
			enable_dlg_items(dlg, IDC_HOSTSERIAL, IDC_HOSTSERIAL, FALSE);
		}
		ExpandCBWidth(dlg, IDC_HOSTCOM);

		CheckRadioButton(dlg, IDC_HOSTTCPIP, IDC_HOSTSERIAL,
		                 IDC_HOSTTCPIP + GetHNRec->PortType - 1);
		if (GetHNRec->PortType == IdTCPIP) {
			SendMessageA(dlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(dlg, IDC_HOSTNAME), TRUE);
			enable_dlg_items(dlg, IDC_HOSTCOMLABEL, IDC_HOSTCOM, FALSE);

			enable_dlg_items(dlg, IDC_SSH_VERSION, IDC_SSH_VERSION, TRUE);
			enable_dlg_items(dlg, IDC_SSH_VERSION_LABEL, IDC_SSH_VERSION_LABEL, TRUE);

			enable_dlg_items(dlg, IDC_HISTORY, IDC_HISTORY, TRUE); // enabled
		}
		else {
			SendMessageA(dlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(dlg, IDC_HOSTCOM), TRUE);
			enable_dlg_items(dlg, IDC_HOSTNAMELABEL, IDC_HOSTTCPPORT,
			                 FALSE);
			enable_dlg_items(dlg, IDC_HOSTTCPPROTOCOLLABEL,
			                 IDC_HOSTTCPPROTOCOL, FALSE);

			enable_dlg_items(dlg, IDC_SSH_VERSION, IDC_SSH_VERSION, FALSE); // disabled
			enable_dlg_items(dlg, IDC_SSH_VERSION_LABEL, IDC_SSH_VERSION_LABEL, FALSE); // disabled (2004.11.23 yutaka)

			enable_dlg_items(dlg, IDC_HISTORY, IDC_HISTORY, FALSE); // disabled
		}
		CenterWindow(dlg, GetParent(dlg));

		// TRUEを返すと、フォーカスが移動してしまう
		return FALSE;
		//return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if(IsEditHistorySelected(dlg)) {
				OpenEditHistory(dlg, pvar->ts);
				break;
			}
			if (IsDlgButtonChecked(dlg, IDC_HOSTTCPIP)) {
				BOOL Ok;
				i = GetDlgItemInt(dlg, IDC_HOSTTCPPORT, &Ok, FALSE);
				if (!Ok) {
					// TODO IDC_HOSTTCPPORTは数値しか入力できない、不要?
					static const TTMessageBoxInfoW info = {
						"TTSSH",
						NULL, L"Tera Term",
						"MSG_TCPPORT_NAN_ERROR", L"The TCP port must be a number.",
						MB_OK | MB_ICONEXCLAMATION
					};
					TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
					return TRUE;
				}
				GetHNRec->TCPPort = i;
				i = (int)SendDlgItemMessage(dlg, IDC_HOSTTCPPROTOCOL, CB_GETCURSEL, 0, 0);
				GetHNRec->ProtocolFamily =
					i == 0 ? AF_UNSPEC :
					i == 1 ? AF_INET6 : AF_INET;
				GetHNRec->PortType = IdTCPIP;
				GetDlgItemTextW(dlg, IDC_HOSTNAME, GetHNRec->HostName, HostNameMaxLength);
				pvar->hostdlg_activated = TRUE;
				pvar->hostdlg_Enabled = FALSE;
				if (IsDlgButtonChecked(dlg, IDC_HOSTTELNET)) {
					GetHNRec->Telnet = TRUE;
				} else if (IsDlgButtonChecked(dlg, IDC_HOSTSSH)) {
					pvar->hostdlg_Enabled = TRUE;

					// check SSH protocol version
					i = (int)SendDlgItemMessage(dlg, IDC_SSH_VERSION, CB_GETCURSEL, 0, 0);
					pvar->settings.ssh_protocol_version = (i == 0) ? 1 : 2;
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
				int pos;
				int index;

				GetHNRec->PortType = IdSerial;
				GetHNRec->HostName[0] = 0;
				pos = (int)SendDlgItemMessageA(dlg, IDC_HOSTCOM, CB_GETCURSEL, 0, 0);
				index = (int)SendDlgItemMessageA(dlg, IDC_HOSTCOM, CB_GETITEMDATA, pos, 0);
				GetHNRec->ComPort = dlg_data->ComPortInfoPtr[index].port_no;
			}
			TTEndDialog(dlg, 1);
			return TRUE;

		case IDCANCEL:
			TTEndDialog(dlg, 0);
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

			if (IsDlgButtonChecked(dlg, IDC_HOSTTELNET)) {
				SetDlgItemInt(dlg, IDC_HOSTTCPPORT, GetHNRec->TelPort, FALSE);
			} else if (IsDlgButtonChecked(dlg, IDC_HOSTSSH)) {
				SetDlgItemInt(dlg, IDC_HOSTTCPPORT, 22, FALSE);
			}
			return TRUE;

		case IDC_HOSTHELP:
			PostMessage(GetParent(dlg), WM_USER_DLGHELP2, HlpFileNewConnection, 0);
			break;

		case IDC_HOSTNAME: {
			switch (HIWORD(wParam)) {
			case CBN_DROPDOWN:
				dlg_data->HostDropOpen = TRUE;
				break;
			case CBN_CLOSEUP:
				dlg_data->HostDropOpen = FALSE;
				break;
			case CBN_SELENDOK:
				if (dlg_data->HostDropOpen) {
					//	ドロップダウンしていないときは、キーかホイールで選択している
					//	決定(Enter or OK押下)すると編集開始
					if(IsEditHistorySelected(dlg)) {
						OpenEditHistory(dlg, pvar->ts);
					}
				}
				break;
			}
			break;
		}
		}
		break;
	case WM_DESTROY:
		ComPortInfoFree(dlg_data->ComPortInfoPtr, dlg_data->ComPortInfoCount);
		free(dlg_data);
		SetWindowLongPtrW(dlg, DWLP_USER, 0);
		break;
	}
	return FALSE;
}

static void UTIL_SetDialogFont()
{
	SetDialogFont(pvar->ts->DialogFontNameW, pvar->ts->DialogFontPoint, pvar->ts->DialogFontCharSet,
	              pvar->ts->UILanguageFileW, "TTSSH", "DLG_TAHOMA_FONT");
}

static BOOL PASCAL TTXGetHostName(HWND parent, PGetHNRec rec)
{
	SetDialogFont(pvar->ts->DialogFontNameW, pvar->ts->DialogFontPoint, pvar->ts->DialogFontCharSet,
	              pvar->ts->UILanguageFileW, "TTSSH", "DLG_TAHOMA_FONT");
//	              pvar->ts->UILanguageFile, "TTSSH", "DLG_SYSTEM_FONT");
	return (BOOL)TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_HOSTDLG), parent, TTXHostDlg, (LPARAM)rec);
}

static void PASCAL TTXGetUIHooks(TTXUIHooks *hooks)
{
	*hooks->GetHostName = TTXGetHostName;
}

static void PASCAL TTXReadINIFile(const wchar_t *fileName, PTTSet ts)
{
	(pvar->ReadIniFile) (fileName, ts);
	read_ssh_options(pvar, fileName);
	pvar->settings = *pvar->ts_SSH;
	logputs(LOG_LEVEL_VERBOSE, "Reading INI file");
	FWDUI_load_settings(pvar);
}

static void PASCAL TTXWriteINIFile(const wchar_t *fileName, PTTSet ts)
{
	(pvar->WriteIniFile) (fileName, ts);
	*pvar->ts_SSH = pvar->settings;
	clear_local_settings(pvar);
	logputs(LOG_LEVEL_VERBOSE, "Writing INI file");
	write_ssh_options(pvar, fileName, pvar->ts_SSH, TRUE);
}

static void read_ssh_options_from_user_file(PTInstVar pvar,
                                            const wchar_t *user_file_name)
{
	wchar_t *fname;

	fname = get_home_dir_relative_nameW(user_file_name);
	read_ssh_options(pvar, fname);
	free(fname);

	pvar->settings = *pvar->ts_SSH;
	FWDUI_load_settings(pvar);
}

// Percent-encodeされた文字列srcをデコードしてdstにコピーする。
// dstlenはdstのサイズ。これより結果が長い場合、その分は切り捨てられる。
static void percent_decode(char *dst, int dstlen, const wchar_t *src) {
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
			*dst++ = (char)*src++;
		}
		dstlen--;
	}
	*dst = 0;
	return;
}

static void add_forward_param(PTInstVar pvar, const wchar_t *paramW)
{
	char *param = ToCharW(paramW);
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
	free(param);
}

static void PASCAL TTXParseParam(wchar_t *param, PTTSet ts, PCHAR DDETopic)
{
	int param_len = wcslen(param);
	int opt_len = param_len+1;
	wchar_t *option = (wchar_t *)calloc(opt_len, sizeof(wchar_t));
	wchar_t *option2 = (wchar_t *)calloc(opt_len, sizeof(wchar_t));
	int action;
	wchar_t *start, *cur, *next;
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
			if (wcsncmp(option + 1, L"ssh", 3) == 0) {
				if (wcsncmp(option + 4, L"-f=", 3) == 0) {
					const wchar_t *file = option + 7;
					read_ssh_options_from_user_file(pvar, file);
					action = OPTION_CLEAR;
				} else if (wcsncmp(option + 4, L"-consume=", 9) == 0) {
					const wchar_t* file = option + 13;
					read_ssh_options_from_user_file(pvar, file);
					DeleteFileW(file);
					action = OPTION_CLEAR;
				}

			// ttermpro.exe の /F= 指定でも TTSSH の設定を読む (2006.10.11 maya)
			} else if (_wcsnicmp(option + 1, L"f=", 2) == 0) {
				const wchar_t *file = option + 3;
				read_ssh_options_from_user_file(pvar, file);
				// Tera Term側でも解釈する必要があるので消さない
			}
		}

		switch (action) {
			case OPTION_CLEAR:
				wmemset(cur, ' ', next-cur);
				break;
			case OPTION_REPLACE:
				wmemset(cur, ' ', next-cur);
				wmemcpy(cur+1, option, wcslen(option));
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
			if (wcsncmp(option + 1, L"ssh", 3) == 0) {
				if (option[4] == 0) {
					pvar->settings.Enabled = 1;
				} else if (wcsncmp(option + 4, L"-L", 2) == 0 ||
				           wcsncmp(option + 4, L"-R", 2) == 0 ||
				           wcsncmp(option + 4, L"-D", 2) == 0) {
					wchar_t *p = option + 5;
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
				} else if (wcsncmp(option + 4, L"-X", 2) == 0) {
					add_forward_param(pvar, L"X");
					if (option+6 != 0) {
						char *option6 = ToCharW(option + 6);
						strncpy_s(pvar->settings.X11Display,
						          sizeof(pvar->settings.X11Display),
						          option6, _TRUNCATE);
						free(option6);
					}
				} else if (wcscmp(option + 4, L"-v") == 0) {
					pvar->settings.LogLevel = LOG_LEVEL_VERBOSE;
				} else if (_wcsicmp(option + 4, L"-autologin") == 0 ||
				           _wcsicmp(option + 4, L"-autologon") == 0) {
					pvar->settings.TryDefaultAuth = TRUE;
				} else if (_wcsnicmp(option + 4, L"-agentconfirm=", 14) == 0) {
					if ((_wcsicmp(option+18, L"off") == 0) ||
					    (_wcsicmp(option+18, L"no") == 0) ||
					    (_wcsicmp(option+18, L"false") == 0) ||
					    (_wcsicmp(option+18, L"0") == 0) ||
					    (_wcsicmp(option+18, L"n") == 0)) {
						pvar->settings.ForwardAgentConfirm = 0;
					}
					else {
						pvar->settings.ForwardAgentConfirm = 1;
					}
				} else if (wcscmp(option + 4, L"-a") == 0) {
					pvar->settings.ForwardAgent = FALSE;
				} else if (wcscmp(option + 4, L"-A") == 0) {
					pvar->settings.ForwardAgent = TRUE;

				} else if (wcsncmp(option + 4, L"-C=", 3) == 0) {
					pvar->settings.CompressionLevel = _wtoi(option+7);
					if (pvar->settings.CompressionLevel < 0) {
						pvar->settings.CompressionLevel = 0;
					}
					else if (pvar->settings.CompressionLevel > 9) {
						pvar->settings.CompressionLevel = 9;
					}
				} else if (wcscmp(option + 4, L"-C") == 0) {
					pvar->settings.CompressionLevel = 6;
				} else if (wcscmp(option + 4, L"-c") == 0) {
					pvar->settings.CompressionLevel = 0;
				} else if (_wcsnicmp(option + 4, L"-icon=", 6) == 0) {
					if ((_wcsicmp(option+10, L"old") == 0) ||
					    (_wcsicmp(option+10, L"yellow") == 0) ||
					    (_wcsicmp(option+10, L"securett_yellow") == 0)) {
						pvar->settings.IconID = IDI_SECURETT_YELLOW;
					}
					else if ((_wcsicmp(option+10, L"green") == 0) ||
					         (_wcsicmp(option+10, L"securett_green") == 0)) {
						pvar->settings.IconID = IDI_SECURETT_GREEN;
					}
					else if ((_wcsicmp(option+10, L"flat") == 0) ||
					         (_wcsicmp(option+10, L"securett_flat") == 0)) {
						pvar->settings.IconID = IDI_SECURETT_FLAT;
					}
					else {
						pvar->settings.IconID = IDI_SECURETT;
					}
				} else if (wcsncmp(option + 4, L"-subsystem=", 11) == 0) {
					char *option15 = ToCharW(option + 15);
					pvar->use_subsystem = TRUE;
					strncpy_s(pvar->subsystem_name,
					          sizeof(pvar->subsystem_name),
					          option15, _TRUNCATE);
					free(option15);
				} else if (wcscmp(option + 4, L"-N") == 0) {
					pvar->nosession = TRUE;

				// /ssh1 と /ssh2 オプションの新規追加 (2006.9.16 maya)
				} else if (wcscmp(option + 4, L"1") == 0) {
					pvar->settings.Enabled = 1;
					pvar->settings.ssh_protocol_version = 1;
				} else if (wcscmp(option + 4, L"2") == 0) {
					pvar->settings.Enabled = 1;
					pvar->settings.ssh_protocol_version = 2;

				} else {
					static const TTMessageBoxInfoW info = {
						"TTSSH",
						NULL, L"TTSSH",
						"MSG_UNKNOWN_OPTION_ERROR", L"Unrecognized command-line option: %s",
						MB_OK | MB_ICONEXCLAMATION
					};
					TTMessageBoxW(NULL, &info, pvar->ts->UILanguageFileW, option);
				}

			// ttermpro.exe の /T= 指定の流用なので、大文字も許す (2006.10.19 maya)
			} else if (_wcsnicmp(option + 1, L"t=", 2) == 0) {
				if (wcscmp(option + 3, L"2") == 0) {
					pvar->settings.Enabled = 1;
					// /t=2はttssh側での拡張なので消す
				} else {
					pvar->settings.Enabled = 0;
					action = OPTION_NONE;	// Tera Term側で解釈するので消さない
				}

			// /1 および /2 オプションの新規追加 (2004.10.3 yutaka)
			} else if (wcscmp(option + 1, L"1") == 0) {
				// command line: /ssh /1 is SSH1 only
				pvar->settings.ssh_protocol_version = 1;

			} else if (wcscmp(option + 1, L"2") == 0) {
				// command line: /ssh /2 is SSH2 & SSH1
				pvar->settings.ssh_protocol_version = 2;

			} else if (wcscmp(option + 1, L"nossh") == 0) {
				// '/nossh' オプションの追加。
				// TERATERM.INI でSSHが有効になっている場合、うまくCygtermが起動しないことが
				// あることへの対処。(2004.10.11 yutaka)
				pvar->settings.Enabled = 0;

			} else if (wcscmp(option + 1, L"telnet") == 0) {
				// '/telnet' が指定されているときには '/nossh' と同じく
				// SSHを無効にする (2006.9.16 maya)
				pvar->settings.Enabled = 0;
				// Tera Term の Telnet フラグも付ける
				pvar->ts->Telnet = 1;

			} else if (wcsncmp(option + 1, L"auth=", 5) == 0) {
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

				if (_wcsicmp(option + 6, L"password") == 0) { // パスワード
					//pvar->auth_state.cur_cred.method = SSH_AUTH_PASSWORD;
					pvar->ssh2_authmethod = SSH_AUTH_PASSWORD;

				} else if (_wcsicmp(option + 6, L"keyboard-interactive") == 0) { // keyboard-interactive認証
					//pvar->auth_state.cur_cred.method = SSH_AUTH_TIS;
					pvar->ssh2_authmethod = SSH_AUTH_TIS;

				} else if (_wcsicmp(option + 6, L"challenge") == 0) { // keyboard-interactive認証
					//pvar->auth_state.cur_cred.method = SSH_AUTH_TIS;
					pvar->ssh2_authmethod = SSH_AUTH_TIS;

				} else if (_wcsicmp(option + 6, L"publickey") == 0) { // 公開鍵認証
					//pvar->auth_state.cur_cred.method = SSH_AUTH_RSA;
					pvar->ssh2_authmethod = SSH_AUTH_RSA;

				} else if (_wcsicmp(option + 6, L"pageant") == 0) { // 公開鍵認証 by Pageant
					//pvar->auth_state.cur_cred.method = SSH_AUTH_RSA;
					pvar->ssh2_authmethod = SSH_AUTH_PAGEANT;

				} else {
					// TODO:
				}

			} else if (wcsncmp(option + 1, L"user=", 5) == 0) {
				WideCharToACP_t(option + 6, pvar->ssh2_username, sizeof(pvar->ssh2_username));

			} else if (wcsncmp(option + 1, L"passwd=", 7) == 0) {
				WideCharToACP_t(option + 8, pvar->ssh2_password, sizeof(pvar->ssh2_password));

			} else if (wcsncmp(option + 1, L"keyfile=", 8) == 0) {
				const wchar_t *keyfileW = option + 9;
				wcsncpy(pvar->ssh2_keyfile, keyfileW, _countof(pvar->ssh2_keyfile));

			} else if (wcscmp(option + 1, L"ask4passwd") == 0) {
				// パスワードを聞く (2006.9.18 maya)
				pvar->ask4passwd = 1;

			} else if (wcscmp(option + 1, L"nosecuritywarning") == 0) {
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
		else if ((_wcsnicmp(option, L"ssh://", 6) == 0) ||
		         (_wcsnicmp(option, L"ssh1://", 7) == 0) ||
		         (_wcsnicmp(option, L"ssh2://", 7) == 0) ||
		         (_wcsnicmp(option, L"slogin://", 9) == 0) ||
		         (_wcsnicmp(option, L"slogin1://", 10) == 0) ||
		         (_wcsnicmp(option, L"slogin2://", 10) == 0)) {
			//
			// ssh://user@host/ 等のURL形式のサポート
			// 基本的な書式は telnet:// URLに順ずる
			//
			// 参考:
			//   RFC3986: Uniform Resource Identifier (URI): Generic Syntax
			//   RFC4248: The telnet URI Scheme
			//
			wchar_t *p, *p2, *p3;
			int optlen, hostlen;

			optlen = wcslen(option);

			// 最初の':'の前の文字が数字だった場合、それをsshプロトコルバージョンとみなす
			p = wcschr(option, ':');
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
			if ((p2 = wcschr(p, '/')) != NULL) {
				*p2 = 0;
			}

			// '@'があった場合、それより前はユーザ情報
			if ((p2 = wcschr(p, '@')) != NULL) {
				*p2 = 0;
				// ':'以降はパスワード
				if ((p3 = wcschr(p, ':')) != NULL) {
					*p3 = 0;
					percent_decode(pvar->ssh2_password, sizeof(pvar->ssh2_password), p3 + 1);
				}
				percent_decode(pvar->ssh2_username, sizeof(pvar->ssh2_username), p);
				// p が host part の先頭('@'の次の文字)を差すようにする
				p = p2 + 1;
			}

			// host part を option の先頭に移動して、scheme part を潰す
			// port指定が無かった時にport番号を足すための領域確保の意味もある
			hostlen = wcslen(p);
			wmemmove_s(option, optlen, p, hostlen);
			option[hostlen] = 0;

			// ポート指定が無い時は":22"を足す
			if (option[0] == '[' && option[hostlen-1] == ']' ||     // IPv6 raw address without port
			    option[0] != '[' && wcschr(option, ':') == NULL) { // hostname or IPv4 raw address without port
				wmemcpy_s(option+hostlen, optlen-hostlen, L":22", 3);
				hostlen += 3;
			}

			// ポート指定より後をすべてスペースで潰す
			wmemset(option+hostlen, ' ', optlen-hostlen);

			pvar->settings.Enabled = 1;

			action = OPTION_REPLACE;
		}
		else if (wcschr(option, '@') != NULL) {
			//
			// user@host 形式のサポート
			//   取り合えずsshでのみサポートの為、ユーザ名はttssh内で潰す。
			//   (ssh接続以外 --  ttsshには関係ない場合でも)
			//   将来的にtelnet authentication optionをサポートした時は
			//   Tera Term本体で処理するようにする予定。
			//
			char *optionA;
			wchar_t *p;
			p = wcschr(option, '@');
			*p = 0;

			optionA = ToCharW(option);
			strncpy_s(pvar->ssh2_username, sizeof(pvar->ssh2_username), optionA, _TRUNCATE);
			free(optionA);

			// ユーザ名部分をスペースで潰す。
			// 後続のTTXやTera Term本体で解釈する時にはスペースを読み飛ばすので、
			// ホスト名を先頭に詰める必要は無い。
			wmemset(option, ' ', p-option+1);

			action = OPTION_REPLACE;
		}


		switch (action) {
			case OPTION_CLEAR:
				wmemset(cur, ' ', next-cur);
				break;
			case OPTION_REPLACE:
				wmemset(cur, ' ', next-cur);
				wmemcpy(cur+1, option, wcslen(option));
				break;
		}

		cur = next;
	}

	free(option);
	free(option2);

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

static HMENU GetSubMenuByChildID(HMENU menu, UINT id)
{
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
	static const DlgTextInfo MenuTextInfo[] = {
		{ ID_ABOUTMENU, "MENU_ABOUT" },
		{ ID_SSHSETUPMENU, "MENU_SSH" },
		{ ID_SSHAUTHSETUPMENU, "MENU_SSH_AUTH" },
		{ ID_SSHFWDSETUPMENU, "MENU_SSH_FORWARD" },
		{ ID_SSHKEYGENMENU, "MENU_SSH_KEYGEN" },
		{ ID_SSHSCPMENU, "MENU_SSH_SCP" },
	};
	// teratermのメニューID(tt_res.h)
	const UINT ID_FILE_NEWCONNECTION = 50110;
	const UINT ID_HELP_ABOUT = 50990;
	const UINT ID_SETUP_TCPIP = 50360;
	const UINT ID_FILE_CHANGEDIR = 50170;

	pvar->FileMenu = GetSubMenuByChildID(menu, ID_FILE_NEWCONNECTION);

	/* inserts before ID_HELP_ABOUT */
	insertMenuBeforeItem(menu, ID_HELP_ABOUT, MF_ENABLED, ID_ABOUTMENU, "About &TTSSH...");

	/* inserts before ID_SETUP_TCPIP */
	insertMenuBeforeItem(menu, ID_SETUP_TCPIP, MF_ENABLED, ID_SSHSETUPMENU, "SS&H...");
	insertMenuBeforeItem(menu, ID_SETUP_TCPIP, MF_ENABLED, ID_SSHAUTHSETUPMENU, "SSH &Authentication...");
	insertMenuBeforeItem(menu, ID_SETUP_TCPIP, MF_ENABLED, ID_SSHFWDSETUPMENU, "SSH F&orwarding...");
	insertMenuBeforeItem(menu, ID_SETUP_TCPIP, MF_ENABLED, ID_SSHKEYGENMENU, "SSH KeyGe&nerator...");

	/* inserts before ID_FILE_CHANGEDIR */
	insertMenuBeforeItem(menu, ID_FILE_CHANGEDIR, MF_GRAYED, ID_SSHSCPMENU, "SS&H SCP...");

	SetI18nMenuStrsW(menu, "TTSSH", MenuTextInfo, _countof(MenuTextInfo), pvar->ts->UILanguageFileW);
}

static void PASCAL TTXModifyPopupMenu(HMENU menu) {
	if (menu == pvar->FileMenu) {
		if (pvar->cv->Ready && pvar->socket != INVALID_SOCKET)
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
			UTIL_get_lang_msgU8("DLG_ABOUT_SERVERID", pvar, "Server ID:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_server_ID_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_PROTOCOL", pvar, "Using protocol:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_protocol_version_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_ENCRYPTION", pvar, "Encryption:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			CRYPT_get_cipher_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_SERVERKEY", pvar, "Server keys:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			CRYPT_get_server_key_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_AUTH", pvar, "Authentication:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			AUTH_get_auth_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_COMP", pvar, "Compression:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_compression_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

		} else { // SSH2
			UTIL_get_lang_msgU8("DLG_ABOUT_SERVERID", pvar, "Server ID:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_server_ID_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_CLIENTID", pvar, "Client ID:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_client_ID_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_PROTOCOL", pvar, "Using protocol:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_protocol_version_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_KEX", pvar, "Key exchange algorithm:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), get_kex_algorithm_name(pvar->kex->kex_type), _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_HOSTKEY", pvar, "Host Key:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), get_ssh2_hostkey_algorithm_name(pvar->kex->hostkey_type), _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_ENCRYPTION", pvar, "Encryption:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			CRYPT_get_cipher_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_MAC", pvar, "MAC algorithm:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_mac_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			if (pvar->kex->ctos_compression == COMP_DELAYED) { // 遅延パケット圧縮の場合 (2006.6.23 yutaka)
				UTIL_get_lang_msgU8("DLG_ABOUT_COMPDELAY", pvar, "Delayed Compression:");
			}
			else {
				UTIL_get_lang_msgU8("DLG_ABOUT_COMP", pvar, "Compression:");
			}
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			SSH_get_compression_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_KEXPARAM", pvar, "Key exchange param:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			CRYPT_get_kex_param_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);

			UTIL_get_lang_msgU8("DLG_ABOUT_AUTH", pvar, "Authentication:");
			strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), " ", _TRUNCATE);
			AUTH_get_auth_info(pvar, buf, sizeof(buf));
			strncat_s(buf2, sizeof(buf2), buf, _TRUNCATE);
			strncat_s(buf2, sizeof(buf2), "\r\n", _TRUNCATE);
		}

		// ホスト公開鍵のfingerprintを表示する。
		// (2014.5.1 yutaka)
		UTIL_get_lang_msgU8("DLG_ABOUT_FINGERPRINT", pvar, "Host key's fingerprint:");
		strncat_s(buf2, sizeof(buf2), pvar->UIMsg, _TRUNCATE);
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
		if (fp != NULL) {
			strncat_s(buf2, sizeof(buf2), fp, _TRUNCATE);
			free(fp);
		}

		{
			wchar_t *strW = ToWcharU8(buf2);
			SetDlgItemTextW(dlg, IDC_ABOUTTEXT, strW);
			free(strW);
		}
	}
}

static void init_about_dlg(PTInstVar pvar, HWND dlg)
{
	char buf[1024];
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_ABOUT_TITLE" },
		{ IDC_FP_HASH_ALG, "DLG_ABOUT_FP_HASH_ALGORITHM" },
		{ IDOK, "BTN_OK" },
	};
	SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

	// TTSSHのバージョンを設定する (2005.2.28 yutaka)
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
	            "TTSSH\r\nTera Term Secure Shell extension, %d.%d.%d ",
	            TTSSH_VERSION_MAJOR, TTSSH_VERSION_MINOR,  TTSSH_VERSION_PATCH);
	{
		char *substr = GetVersionSubstr();
		strncat_s(buf, sizeof(buf), substr, _TRUNCATE);
		free(substr);
	}
	strncat_s(buf, sizeof(buf),
			  "\r\nCompatible with SSH protocol version 1.5 and 2.0", _TRUNCATE);
	SetDlgItemTextA(dlg, IDC_TTSSH_VERSION, buf);

	// OpenSSLのバージョンを設定する (2005.1.24 yutaka)
	// 条件文追加 (2005.5.11 yutaka)
	// OPENSSL_VERSION_TEXT マクロ定義ではなく、関数を使ってバージョンを取得する。(2013.11.24 yutaka)
	SetDlgItemTextA(dlg, IDC_OPENSSL_VERSION, SSLeay_version(SSLEAY_VERSION));

	// zlibのバージョンを設定する (2005.5.11 yutaka)
#ifdef ZLIB_VERSION
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "zlib %s", ZLIB_VERSION);
#else
	_snprintf(buf, sizeof(buf), "zlib Unknown");
#endif
	SetDlgItemTextA(dlg, IDC_ZLIB_VERSION, buf);

	// ssh agent client の種類とバージョンを表示
	SetDlgItemTextA(dlg, IDC_PUTTY_VERSION, putty_get_version());
}

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

typedef struct {
	TInstVar *pvar;
	ReiseDlgHelper_t *resize_helper;
	HFONT DlgAboutTextFont;
} AboutDlgData;

static INT_PTR CALLBACK TTXAboutDlg(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static const ResizeHelperInfo resize_info[] = {
		{ IDC_TTSSH_ICON, RESIZE_HELPER_ANCHOR_RIGHT },
		{ IDC_WEBSITES, RESIZE_HELPER_ANCHOR_LR },
		{ IDC_CRYPTOGRAPHY, RESIZE_HELPER_ANCHOR_LR },
		{ IDC_CREDIT, RESIZE_HELPER_ANCHOR_LR },
		{ IDC_ABOUTTEXT, RESIZE_HELPER_ANCHOR_LRTB },
		{ IDOK, RESIZE_HELPER_ANCHOR_B + RESIZE_HELPER_ANCHOR_NONE_H },
	};

	switch (msg) {
	case WM_INITDIALOG: {
		AboutDlgData *data = (AboutDlgData *)lParam;
		TInstVar *pvar = data->pvar;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		// Edit controlは等幅フォントで表示したいので、別設定情報からフォントをセットする。
		// (2014.5.5. yutaka)
		data->DlgAboutTextFont = UTIL_get_lang_fixedfont(dlg, pvar->ts->UILanguageFileW);
		if (data->DlgAboutTextFont != NULL) {
			SendDlgItemMessage(dlg, IDC_ABOUTTEXT, WM_SETFONT, (WPARAM)data->DlgAboutTextFont, MAKELPARAM(TRUE, 0));
		}

		SetDlgItemIcon(dlg, IDC_TTSSH_ICON, MAKEINTRESOURCEW(pvar->settings.IconID), 0, 0);

		init_about_dlg(pvar, dlg);
		CheckDlgButton(dlg, IDC_FP_HASH_ALG_SHA256, TRUE);
		about_dlg_set_abouttext(pvar, dlg, SSH_DIGEST_SHA256);
		SetFocus(GetDlgItem(dlg, IDOK));

		// Edit controlをサブクラス化する。
		g_deltaSumAboutDlg = 0;
		g_defAboutDlgEditWndProc = (WNDPROC)SetWindowLongPtrW(GetDlgItem(dlg, IDC_ABOUTTEXT), GWLP_WNDPROC, (LONG_PTR)AboutDlgEditWindowProc);

		data->resize_helper = ReiseDlgHelperInit(dlg, TRUE, resize_info, _countof(resize_info));
		CenterWindow(dlg, GetParent(dlg));

		return FALSE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			TTEndDialog(dlg, 1);
			return TRUE;
		case IDCANCEL:			/* there isn't a cancel button, but other Windows
								   UI things can send this message */
			TTEndDialog(dlg, 0);
			return TRUE;
		case IDC_FP_HASH_ALG_MD5:
			about_dlg_set_abouttext(pvar, dlg, SSH_DIGEST_MD5);
			return TRUE;
		case IDC_FP_HASH_ALG_SHA256:
			about_dlg_set_abouttext(pvar, dlg, SSH_DIGEST_SHA256);
			return TRUE;
		}
		break;

	case WM_DESTROY: {
		AboutDlgData *data = (AboutDlgData *)GetWindowLongPtr(dlg, DWLP_USER);
		if (data->DlgAboutTextFont != NULL) {
			DeleteObject(data->DlgAboutTextFont);
			data->DlgAboutTextFont = NULL;
		}
		break;
	}

	case WM_DPICHANGED: {
		AboutDlgData *data = (AboutDlgData *)GetWindowLongPtr(dlg, DWLP_USER);
		if (data->DlgAboutTextFont != NULL) {
			DeleteObject(data->DlgAboutTextFont);
		}
		data->DlgAboutTextFont = UTIL_get_lang_fixedfont(dlg, pvar->ts->UILanguageFileW);
		if (data->DlgAboutTextFont != NULL) {
			SendDlgItemMessage(dlg, IDC_ABOUTTEXT, WM_SETFONT, (WPARAM)data->DlgAboutTextFont, MAKELPARAM(TRUE, 0));
		}
		SendDlgItemMessage(dlg, IDC_TTSSH_ICON, WM_DPICHANGED, wParam, lParam);
		ReiseDlgHelper_WM_DPICHANGED(data->resize_helper, wParam, lParam);
		return FALSE;
	}

	default: {
		AboutDlgData *data = (AboutDlgData *)GetWindowLongPtrW(dlg, DWLP_USER);
		if (data != NULL) {
			return ResizeDlgHelperProc(data->resize_helper, dlg, msg, wParam, lParam);
		}
		return FALSE;
	}
	}

	return FALSE;
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
	HWND hostkeyRotationControlList = GetDlgItem(dlg, IDC_HOSTKEY_ROTATION_COMBO);
	int i;
	int ch;
	static const wchar_t *rotationItem[SSH_UPDATE_HOSTKEYS_MAX] = {
		L"No",
		L"Yes",
		L"Ask",
	};
	static const char *rotationItemKey[SSH_UPDATE_HOSTKEYS_MAX] = {
		"DLG_SSHSETUP_HOSTKEY_ROTATION_NO",
		"DLG_SSHSETUP_HOSTKEY_ROTATION_YES",
		"DLG_SSHSETUP_HOSTKEY_ROTATION_ASK",
	};
	wchar_t uimsg[MAX_UIMSG];

	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_SSHSETUP_TITLE" },
		{ IDC_COMPRESSLABEL, "DLG_SSHSETUP_COMPRESS" },
		{ IDC_COMPRESSNONE, "DLG_SSHSETUP_COMPRESS_NONE" },
		{ IDC_COMPRESSHIGH, "DLG_SSHSETUP_COMPRESS_HIGHEST" },
		{ IDC_COMPRESSNOTE, "DLG_SSHSETUP_COMPRESS_NOTE" },

		{ IDC_CIPHERORDER, "DLG_SSHSETUP_CIPHER" },
		{ IDC_SSHMOVECIPHERUP, "DLG_SSHSETUP_CIPHER_UP" },
		{ IDC_SSHMOVECIPHERDOWN, "DLG_SSHSETUP_CIPHER_DOWN" },

		{ IDC_KEX_ORDER, "DLG_SSHSETUP_KEX" },
		{ IDC_SSHKEX_MOVEUP, "DLG_SSHSETUP_KEX_UP" },
		{ IDC_SSHKEX_MOVEDOWN, "DLG_SSHSETUP_KEX_DOWN" },

		{ IDC_HOST_KEY_ORDER, "DLG_SSHSETUP_HOST_KEY" },
		{ IDC_SSHHOST_KEY_MOVEUP, "DLG_SSHSETUP_HOST_KEY_UP" },
		{ IDC_SSHHOST_KEY_MOVEDOWN, "DLG_SSHSETUP_HOST_KEY_DOWN" },

		{ IDC_MAC_ORDER, "DLG_SSHSETUP_MAC" },
		{ IDC_SSHMAC_MOVEUP, "DLG_SSHSETUP_MAC_UP" },
		{ IDC_SSHMAC_MOVEDOWN, "DLG_SSHSETUP_MAC_DOWN" },

		{ IDC_COMP_ORDER, "DLG_SSHSETUP_COMP" },
		{ IDC_SSHCOMP_MOVEUP, "DLG_SSHSETUP_COMP_UP" },
		{ IDC_SSHCOMP_MOVEDOWN, "DLG_SSHSETUP_COMP_DOWN" },

		{ IDC_KNOWNHOSTS, "DLG_SSHSETUP_KNOWNHOST" },
		{ IDC_CHOOSEREADWRITEFILE, "DLG_SSHSETUP_KNOWNHOST_RW" },
		{ IDC_CHOOSEREADONLYFILE, "DLG_SSHSETUP_KNOWNHOST_RO" },
		{ IDC_HEARTBEATLABEL, "DLG_SSHSETUP_HEARTBEAT" },
		{ IDC_HEARTBEATLABEL2, "DLG_SSHSETUP_HEARTBEAT_UNIT" },
		{ IDC_REMEMBERPASSWORD, "DLG_SSHSETUP_PASSWORD" },
		{ IDC_FORWARDAGENT, "DLG_SSHSETUP_FORWARDAGENT" },
		{ IDC_FORWARDAGENTCONFIRM, "DLG_SSHSETUP_FORWARDAGENTCONFIRM" },
		{ IDC_FORWARDAGENTNOTIFY, "DLG_SSHSETUP_FORWARDAGENTNOTIFY" },
		{ IDC_VERIFYHOSTKEYDNS, "DLG_SSHSETUP_VERIFYHOSTKEYDNS" },
		{ IDC_NOTICEBANNER, "DLG_SSHSETUP_NOTICE" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_SSHSETUP_HELP, "BTN_HELP" },

		{ IDC_HOSTKEY_ROTATION_STATIC, "DLG_SSHSETUP_HOSTKEY_ROTATION" },
		{ IDC_LOGLEVEL, "DLG_SSHSETUP_LOGLEVEL" },
		{ IDC_LOGLEVEL_UNIT, "DLG_SSHSETUP_LOGLEVEL_UNIT" },
	};
	SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

	SendMessage(compressionControl, TBM_SETRANGE, TRUE, MAKELONG(0, 9));
	SendMessage(compressionControl, TBM_SETPOS, TRUE,
	            pvar->settings.CompressionLevel);

	// Cipher order
	normalize_cipher_order(pvar->settings.CipherOrder);

	for (i = 0; pvar->settings.CipherOrder[i] != 0; i++) {
		int cipher = pvar->settings.CipherOrder[i] - '0';
		wchar_t *name = get_listbox_cipher_nameW(cipher, pvar);

		if (name != NULL) {
			int index = SendMessageW(cipherControl, LB_ADDSTRING, 0, (LPARAM) name);
			SendMessageW(cipherControl, LB_SETITEMDATA, index, cipher);
			free(name);
		}
	}

	SendMessage(cipherControl, LB_SETCURSEL, 0, 0);
	set_move_button_status(dlg, IDC_SSHCIPHERPREFS, IDC_SSHMOVECIPHERUP, IDC_SSHMOVECIPHERDOWN);

	// KEX order
	normalize_kex_order(pvar->settings.KexOrder);
	for (i = 0; pvar->settings.KexOrder[i] != 0; i++) {
		int index = pvar->settings.KexOrder[i] - '0';

		if (index == 0) {
			UTIL_get_lang_msgW("DLG_SSHSETUP_KEX_BORDER", pvar,
			                   L"<KEXs below this line are disabled>", uimsg);
			SendMessageW(kexControl, LB_ADDSTRING, 0, (LPARAM)uimsg);
		} else {
			const char *name = get_kex_algorithm_name(index);
			if (name != NULL) {
				SendMessageA(kexControl, LB_ADDSTRING, 0, (LPARAM) name);
			}
		}
	}
	SendMessage(kexControl, LB_SETCURSEL, 0, 0);
	set_move_button_status(dlg, IDC_SSHKEX_LIST, IDC_SSHKEX_MOVEUP, IDC_SSHKEX_MOVEDOWN);

	// Host Key order
	normalize_host_key_order(pvar->settings.HostKeyOrder);
	for (i = 0; pvar->settings.HostKeyOrder[i] != 0; i++) {
		int index = pvar->settings.HostKeyOrder[i] - '0';

		if (index == 0) {
			UTIL_get_lang_msgW("DLG_SSHSETUP_HOST_KEY_BORDER", pvar,
			                   L"<Host Keys below this line are disabled>", uimsg);
			SendMessageW(hostkeyControl, LB_ADDSTRING, 0, (LPARAM)uimsg);
		} else {
			const char *name = get_ssh2_hostkey_algorithm_name(index);
			if (name != NULL) {
				SendMessageA(hostkeyControl, LB_ADDSTRING, 0, (LPARAM) name);
			}
		}
	}
	SendMessage(hostkeyControl, LB_SETCURSEL, 0, 0);
	set_move_button_status(dlg, IDC_SSHHOST_KEY_LIST, IDC_SSHHOST_KEY_MOVEUP, IDC_SSHHOST_KEY_MOVEDOWN);

	// MAC order
	normalize_mac_order(pvar->settings.MacOrder);
	for (i = 0; pvar->settings.MacOrder[i] != 0; i++) {
		int index = pvar->settings.MacOrder[i] - '0';

		if (index == 0) {
			UTIL_get_lang_msgW("DLG_SSHSETUP_MAC_BORDER", pvar,
			                   L"<MACs below this line are disabled>", uimsg);
			SendMessageW(macControl, LB_ADDSTRING, 0, (LPARAM)uimsg);
		} else {
			const char *name = get_ssh2_mac_name_by_id(index);
			if (name != NULL) {
				SendMessageA(macControl, LB_ADDSTRING, 0, (LPARAM) name);
			}
		}
	}
	SendMessage(macControl, LB_SETCURSEL, 0, 0);
	set_move_button_status(dlg, IDC_SSHMAC_LIST, IDC_SSHMAC_MOVEUP, IDC_SSHMAC_MOVEDOWN);

	// Compression order
	normalize_comp_order(pvar->settings.CompOrder);
	for (i = 0; pvar->settings.CompOrder[i] != 0; i++) {
		int index = pvar->settings.CompOrder[i] - '0';

		if (index == 0) {
			UTIL_get_lang_msgW("DLG_SSHSETUP_COMP_BORDER", pvar,
			                   L"<Compression methods below this line are disabled>", uimsg);
			SendMessageW(compControl, LB_ADDSTRING, 0, (LPARAM)uimsg);
		} else {
			const char *name = get_ssh2_comp_name(index);
			if (name != NULL) {
				SendMessageA(compControl, LB_ADDSTRING, 0, (LPARAM) name);
			}
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
		UTIL_get_lang_msgW(rotationItemKey[i], pvar, rotationItem[i], uimsg);
		SendMessageW(hostkeyRotationControlList, CB_INSERTSTRING, i, (LPARAM)uimsg);
	}
	ch = pvar->settings.UpdateHostkeys;
	if (!(ch >= 0 && ch < SSH_UPDATE_HOSTKEYS_MAX))
		ch = 0;
	SendMessage(hostkeyRotationControlList, CB_SETCURSEL, ch, 0);

	// LogLevel
	{
		char buf[10];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            "%d", pvar->settings.LogLevel);
		SetDlgItemTextA(dlg, IDC_LOGLEVEL_VALUE, buf);
	}


}

/**
 *	ファイル名を exe のあるフォルダからの相対パスとみなし、フルパスに変換する
 *	@return	フルパスファイル名
 *			free()すること
 */
wchar_t *get_teraterm_dir_relative_nameW(const wchar_t *basename)
{
	wchar_t *path;
	wchar_t *ret;

	if (!IsRelativePathW(basename)) {
		return _wcsdup(basename);
	}

	// ttermpro.exeのパス
	path = GetExeDirW(NULL);
	aswprintf(&ret, L"%s\\%s", path, basename);
	free(path);
	return ret;
}

void get_teraterm_dir_relative_name(char *buf, int bufsize, const char *basename)
{
	int filename_start = 0;
	int i;
	int ch;

	if (!IsRelativePathA(basename)) {
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

int copy_teraterm_dir_relative_path(char *dest, int destsize, const char *basename)
{
	char buf[1024];
	int filename_start = 0;
	int i;
	int ch, ch2;

	if (!IsRelativePathA(basename)) {
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

/**
 *	ファイル名を個人用設定ファイルフォルダからの相対パスとみなし、フルパスに変換する
 *		%APPDATA%\teraterm5 (%USERPROFILE%\AppData\Roaming\teraterm5)
 *	@return	フルパスファイル名
 *			free()すること
 */
wchar_t *get_home_dir_relative_nameW(const wchar_t *basename)
{
	wchar_t *path;

	if (!IsRelativePathW(basename)) {
		return _wcsdup(basename);
	}

	path = GetHomeDirW(NULL);
	awcscats(&path, L"\\", basename, NULL);
	return path;
}

/**
 *	ファイル名をログ保存フォルダからの相対パスとみなし、フルパスに変換する
 *		%LOCALAPPDATA%\teraterm5 (%USERPROFILE%\AppData\Local\teraterm5)
 *	@return	フルパスファイル名
 *			free()すること
 */
wchar_t *get_log_dir_relative_nameW(const wchar_t *basename)
{
	wchar_t *path;

	if (!IsRelativePathW(basename)) {
		return _wcsdup(basename);
	}

	path = GetLogDirW(NULL);
	awcscats(&path, L"\\", basename, NULL);
	return path;
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
		int chipher = SendMessageW(cipherControl, LB_GETITEMDATA, i, 0);
		buf2[buf2index] = '0' + chipher;
		buf2index++;
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
				j <= KEY_ALGO_MAX
				 && strcmp(buf, get_ssh2_hostkey_algorithm_name(j)) != 0; j++) {
			}
			if (j <= KEY_ALGO_MAX) {
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
				&& strcmp(buf, get_ssh2_mac_name_by_id(j)) != 0; j++) {
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

	// get LogLevel
	SendMessage(GetDlgItem(dlg, IDC_LOGLEVEL_VALUE), WM_GETTEXT, sizeof(buf), (LPARAM)buf);
	i = atoi(buf);
	if (i < 0)
		i = 0;
	pvar->settings.LogLevel = i;
}

static void move_cur_sel_delta(HWND listbox, int delta)
{
	int curPos = (int) SendMessageW(listbox, LB_GETCURSEL, 0, 0);
	int maxPos = (int) SendMessageW(listbox, LB_GETCOUNT, 0, 0) - 1;
	int newPos = curPos + delta;

	if (curPos >= 0 && newPos >= 0 && newPos <= maxPos) {
		int item_data;
		int index;
		size_t len = SendMessageW(listbox, LB_GETTEXTLEN, curPos, 0);
		wchar_t *buf = malloc(sizeof(wchar_t) * (len+1));
		buf[0] = 0;
		SendMessageW(listbox, LB_GETTEXT, curPos, (LPARAM) buf);
		item_data = (int)SendMessageW(listbox, LB_GETITEMDATA, curPos, 0);
		SendMessageW(listbox, LB_DELETESTRING, curPos, 0);
		index = (int)SendMessageW(listbox, LB_INSERTSTRING, newPos, (LPARAM)buf);
		SendMessageW(listbox, LB_SETITEMDATA, index, item_data);
		SendMessageW(listbox, LB_SETCURSEL, newPos, 0);
		free(buf);
	}
}

static int get_keys_file_name(HWND parent, char *buf, int bufsize,
                              int readonly)
{
	wchar_t *title;
	if (readonly) {
		GetI18nStrWW("TTSSH", "MSG_OPEN_KNOWNHOSTS_RO_TITLE",
					 L"Choose a read-only known-hosts file to add",
					 pvar->ts->UILanguageFileW, &title);
	}
	else {
		GetI18nStrWW("TTSSH", "MSG_OPEN_KNOWNHOSTS_RW_TITLE",
					 L"Choose a read/write known-hosts file",
					 pvar->ts->UILanguageFileW, &title);
	}

	TTOPENFILENAMEW params = {0};
	params.hwndOwner = parent;
	params.lpstrFilter = NULL;
	params.nFilterIndex = 0;
	params.lpstrFile = L"ssh_known_hosts";
	params.lpstrInitialDir = NULL;
	params.lpstrTitle = title;
	params.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	params.lpstrDefExt = NULL;
//	params.lpstrInitialDir = pvar->ts->HomeDirW;

	wchar_t *filenameW;
	BOOL r = TTGetOpenFileNameW(&params, &filenameW);
	free(title);
	if (r != FALSE) {
		// ANSIで保存
		char *filenameA = ToCharW(filenameW);
		copy_teraterm_dir_relative_path(buf, bufsize, filenameA);
		free(filenameA);
		return 1;
	} else {
		int err = CommDlgExtendedError();

		if (err != 0) {
			static const TTMessageBoxInfoW info = {
				"TTSSH",
				NULL, L"TTSSH Error",
				"MSG_OPEN_FILEDLG_KNOWNHOSTS_ERROR", L"Unable to display file dialog box: error %d",
				MB_OK | MB_ICONEXCLAMATION
			};
			TTMessageBoxW(parent, &info, pvar->ts->UILanguageFileW, err);
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

static INT_PTR CALLBACK TTXSetupDlg(HWND dlg, UINT msg, WPARAM wParam,
                                    LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		SetWindowLongPtr(dlg, DWLP_USER, lParam);
		init_setup_dlg((PTInstVar) lParam, dlg);

		CenterWindow(dlg, GetParent(dlg));

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			complete_setup_dlg((PTInstVar) GetWindowLongPtr(dlg, DWLP_USER), dlg);
			TTEndDialog(dlg, 1);
			return TRUE;
		case IDCANCEL:			/* there isn't a cancel button, but other Windows
								   UI things can send this message */
			TTEndDialog(dlg, 0);
			return TRUE;
		case IDC_SSHSETUP_HELP:
			PostMessage(GetParent(dlg), WM_USER_DLGHELP2, HlpMenuSetupSsh, 0);
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
		BIGNUM *e, *n;
		BIGNUM *p_e, *p_n;

		// private key
		priv =  RSA_generate_key(bits, 35, cbfunc, cbarg);
		if (priv == NULL)
			goto error;
		private_key.rsa = priv;

		// public key
		pub = RSA_new();
		n = BN_new();
		e = BN_new();
		RSA_set0_key(pub, n, e, NULL);
		if (n == NULL || e == NULL) {
			RSA_free(pub);
			goto error;
		}

		RSA_get0_key(priv, &p_n, &p_e, NULL);

		BN_copy(n, p_n);
		BN_copy(e, p_e);
		public_key.rsa = pub;
		break;
	}

	case KEY_DSA:
	{
		DSA *priv = NULL;
		DSA *pub = NULL;
		BIGNUM *p, *q, *g, *pub_key;
		BIGNUM *sp, *sq, *sg, *spub_key;

		// private key
		priv = DSA_new();
		BN_GENCB *cb = BN_GENCB_new();
		BN_GENCB_set_old(cb, cbfunc, cbarg);
		int r= DSA_generate_parameters_ex(priv, bits, NULL, 0, NULL, NULL, cb);
		BN_GENCB_free(cb);
		if (!r) {
			DSA_free(priv);
			goto error;
		}
		if (!DSA_generate_key(priv)) {
			// TODO: free 'priv'?
			goto error;
		}
		private_key.dsa = priv;

		// public key
		pub = DSA_new();
		if (pub == NULL)
			// TODO: free 'priv'?
			goto error;
		p = BN_new();
		q = BN_new();
		g = BN_new();
		DSA_set0_pqg(pub, p, q, g);
		pub_key = BN_new();
		DSA_set0_key(pub, pub_key, NULL);
		if (p == NULL || q == NULL || g == NULL || pub_key == NULL) {
			// TODO: free 'priv'?
			DSA_free(pub);
			goto error;
		}

		DSA_get0_pqg(priv, &sp, &sq, &sg);
		DSA_get0_key(priv, &spub_key, NULL);

		BN_copy(p, sp);
		BN_copy(q, sq);
		BN_copy(g, sg);
		BN_copy(pub_key, spub_key);
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
		SetDlgItemText(((cbarg_t *)cbarg)->dlg, IDC_KEYGEN_PROGRESS_LABEL, pvar->UIMsg);

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
		SetDlgItemText(((cbarg_t *)cbarg)->dlg, IDC_KEYGEN_PROGRESS_LABEL, pvar->UIMsg);

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

/**
 *	SCPのパスを保存する
 *
 *	TODO
 *	- tsに書き戻してしまってよい?
 *	- Unicode化する
 */
static void SavePaths(HWND dlg, TTTSet *pts)
{
	// 送信パスを ts->ScpSendDir に保存
	char sendfiledir[MAX_PATH];
	GetDlgItemTextA(dlg, IDC_SENDFILE_TO, sendfiledir, _countof(sendfiledir));
	strncpy_s(pts->ScpSendDir, sizeof(pts->ScpSendDir), sendfiledir, _TRUNCATE);

#if 0
	// 受信パスを ts->FileDir に保存
	char recvdir[MAX_PATH];
	GetDlgItemTextA(dlg, IDC_RECVFILE_TO, recvdir, _countof(recvdir));
	strncpy_s(pvar->ts->FileDir, sizeof(pvar->ts->FileDir), recvdir, _TRUNCATE);
#endif
}

//
// SCP dialog
//
static INT_PTR CALLBACK TTXScpDialog(HWND dlg, UINT msg, WPARAM wParam,
                                     LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG: {
		static const DlgTextInfo text_info[] = {
			{ 0, "DLG_SCP_TITLE" },
			{ IDC_SENDFILE_FROM_LABEL, "DLG_SCP_SENDFILE_FROM" },
			{ IDC_SENDFILE_TO_LABEL, "DLG_SCP_SENDFILE_TO" },
			{ IDC_SENDFILE_NOTE, "DLG_SCP_SENDFILE_DRAG" },
			{ IDOK, "DLG_SCP_SENDFILE_SEND" },
			{ IDCANCEL, "DLG_SCP_SENDFILE_CANCEL" },
			{ IDC_RECEIVEFILE_FROM_LABEL, "DLG_SCP_RECEIVEFILE_FROM" },
			{ IDC_RECVFILE_TO_LABEL, "DLG_SCP_RECEIVEFILE_TO" },
			{ IDC_RECV, "DLG_SCP_RECEIVEFILE_RECEIVE" },
			{ IDHELP, "BTN_HELP" },
		};
		SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

		DragAcceptFiles(dlg, TRUE);

		// SCPファイル送信先を表示する
		SetDlgItemTextA(dlg, IDC_SENDFILE_TO, pvar->ts->ScpSendDir);

		// SCPファイル受信先を表示する
		wchar_t *dir = GetFileDir(pvar->ts);
		SetDlgItemTextW(dlg, IDC_RECVFILE_TO, dir);
		free(dir);

#ifdef SFTP_DEBUG
		ShowWindow(GetDlgItem(dlg, IDC_SFTP_TEST), SW_SHOW);
#endif
		CenterWindow(dlg, GetParent(dlg));

		return TRUE;
	}

	case WM_DROPFILES: {
		HDROP hDrop = (HDROP)wParam;
		UINT uFileNo = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
		if (uFileNo > 0) {
			const UINT len = DragQueryFileW(hDrop, 0, NULL, 0);
			if (len == 0) {
				DragFinish(hDrop);
				return TRUE;
			}
			wchar_t *filename = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
			DragQueryFileW(hDrop, 0, filename, len + 1);
			filename[len] = '\0';
			// update edit box
			SetDlgItemTextW(dlg, IDC_SENDFILE_EDIT, filename);
			free(filename);
		}
		DragFinish(hDrop);
		return TRUE;
	}

	case WM_COMMAND:
		// フォーカス位置に応じてEnterキーで実行されるボタンを変更する
		if (HIWORD(wParam) == EN_SETFOCUS) {
			switch (LOWORD(wParam)) {
				case IDC_SENDFILE_EDIT:
				case IDC_SENDFILE_TO:
					SendMessage(dlg, DM_SETDEFID, (WPARAM)IDOK, 0);
					break;
				case IDC_RECVFILE:
				case IDC_RECVFILE_TO:
					SendMessage(dlg, DM_SETDEFID, (WPARAM)IDC_RECV, 0);
					break;
			}
		}

		switch (wParam) {
		case IDC_SENDFILE_SELECT | (BN_CLICKED << 16): {
			TTOPENFILENAMEW ofn = {0};

			ofn.hwndOwner = dlg;
#if 0
			get_lang_msg("FILEDLG_SELECT_LOGVIEW_APP_FILTER", ts.UIMsg, sizeof(ts.UIMsg),
			             "exe(*.exe)\\0*.exe\\0all(*.*)\\0*.*\\0\\0", ts.UILanguageFile);
#endif
			ofn.lpstrFilter = L"all(*.*)\0*.*\0\0";
			UTIL_get_lang_msg("DLG_SCP_SELECT_FILE_TITLE", pvar,
			                  "Choose a sending file with SCP");
			wchar_t *UIMsgW;
			GetI18nStrWW("TTSSH", "DLG_SCP_SELECT_FILE_TITLE", L"Choose a sending file with SCP", pvar->ts->UILanguageFileW, &UIMsgW);
			ofn.lpstrTitle = UIMsgW;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.Flags |= OFN_FORCESHOWHIDDEN;
			wchar_t *filename;
			if (TTGetOpenFileNameW(&ofn, &filename)) {
				SetDlgItemTextW(dlg, IDC_SENDFILE_EDIT, filename);
				free(filename);
			}
			free(UIMsgW);
			SendMessage(dlg, DM_SETDEFID, (WPARAM)IDOK, 0);	// Enterキーで実行されるボタンを Send にする

			return TRUE;
		}
		case IDC_RECVDIR_SELECT | (BN_CLICKED << 16): {
			wchar_t *cur_dir, *new_dir, uimsg[MAX_UIMSG];
			hGetDlgItemTextW(dlg, IDC_RECVFILE_TO, &cur_dir);
			UTIL_get_lang_msgW("DLG_SCP_SELECT_DEST_TITLE", pvar,
			                   L"Choose destination directory", uimsg);
			if (doSelectFolderW(dlg, cur_dir, uimsg, &new_dir)) {
				SetDlgItemTextW(dlg, IDC_RECVFILE_TO, new_dir);
				free(new_dir);
			}
			free(cur_dir);
			SendMessage(dlg, DM_SETDEFID, (WPARAM)IDC_RECV, 0);	// Enterキーで実行されるボタンを Receive にする
			}
			return TRUE;
		}

		switch (LOWORD(wParam)) {
		case IDOK: {  // ファイル送信
			wchar_t *filenameW;
			hGetDlgItemTextW(dlg, IDC_SENDFILE_EDIT, &filenameW);
			if (filenameW != NULL) {
				// 送信パス
				wchar_t *sendfiledirW;
				hGetDlgItemTextW(dlg, IDC_SENDFILE_TO, &sendfiledirW);

				char *sendfiledirU8 = ToU8W(sendfiledirW);
				char *filenameU8 = ToU8W(filenameW);
				SSH_start_scp_send(pvar, filenameU8, sendfiledirU8);
				free(filenameU8);
				free(sendfiledirU8);
				free(sendfiledirW);
				free(filenameW);

				SavePaths(dlg, pvar->ts);

				TTEndDialog(dlg, 1); // dialog close
				return TRUE;
			}
			return FALSE;
		}

		case IDCANCEL: {
			SavePaths(dlg, pvar->ts);

			TTEndDialog(dlg, 0); // dialog close
			return TRUE;
		}

		case IDC_RECV: {
			// ファイル受信
			wchar_t *FileNameW;
			hGetDlgItemTextW(dlg, IDC_RECVFILE, &FileNameW);
			if (FileNameW != NULL) {
				// 受信ファイルからパスを取り除く
				wchar_t *fn = wcsrchr(FileNameW, L'/');
				if (fn) {
					fn++;
					if (*fn == '\0') {
						return FALSE;
					}
				}
				else {
					fn = FileNameW;
				}
				wchar_t *recvfn = replaceInvalidFileNameCharW(fn, '_');

				// 受信パス
				wchar_t *recvdirW;
				hGetDlgItemTextW(dlg, IDC_RECVFILE_TO, &recvdirW);
				wchar_t *recvdir_expanded;
				hExpandEnvironmentStringsW(recvdirW, &recvdir_expanded);

				char *recvpathU8 = ToU8W(recvdir_expanded);
				char *FileNameU8 = ToU8W(FileNameW);
				SSH_start_scp_receive(pvar, FileNameU8, recvpathU8);
				free(FileNameW);
				free(recvfn);
				free(recvdirW);
				free(recvdir_expanded);
				free(recvpathU8);
				free(FileNameU8);

				SavePaths(dlg, pvar->ts);

				TTEndDialog(dlg, 1); // dialog close
				return TRUE;
			}
			return FALSE;
		}

		case IDC_SFTP_TEST:
			SSH_sftp_transaction(pvar);
			return TRUE;

		case IDHELP:
			PostMessage(GetParent(dlg), WM_USER_DLGHELP2, HlpMenuFileSshScp, 0);
			return TRUE;
		}
	}

	return FALSE;
}

// マクロコマンド"scpsend"から呼び出すために、DLL外へエクスポートする。"ttxssh.def"ファイルに記載。
// (2008.1.1 yutaka)
__declspec(dllexport) int CALLBACK TTXScpSendfile(char *sendfile, char *dest)
{
	return SSH_start_scp_send(pvar, sendfile, dest);
}

__declspec(dllexport) int CALLBACK TTXScpSendingStatus(void)
{
	return SSH_scp_sending_status();
}

__declspec(dllexport) int CALLBACK TTXScpReceivefile(char *remotefile, char *localfile)
{
	return SSH_start_scp_receive(pvar, remotefile, localfile);
}


/**
 * TTSSHの設定内容(known hosts file)を返す。
 * フルパスで返す
 *
 * @param[in]	filename	ファイル名へのポインタ
 *							NULLの時文字数を返す
 * @param[in]	maxlen		ファイル名の文字数
 *							(filename != NULLの時有効)
 * @retval		文字数(0以外),(filename == NULLの時は'\0'も含む)
 *				0	エラー
 */
__declspec(dllexport) size_t CALLBACK TTXReadKnownHostsFile(wchar_t *filename, size_t maxlen)
{
	if (!pvar->settings.Enabled) {
		return 0;
	}
	else {
		wchar_t *fullpath;
		wchar_t *filenameW = ToWcharA(pvar->settings.KnownHostsFiles);
		wchar_t *p = wcschr(filenameW, L';');
		size_t ret;
		if (p) {
			*p = 0;		// cut readonly ssh known hosts
		}

		fullpath = get_home_dir_relative_nameW(filenameW);
		ret = wcslen(fullpath);
		if (filename != NULL) {
			wcsncpy_s(filename, maxlen, fullpath, _TRUNCATE);
		}
		else {
			ret++;
		}
		free(fullpath);
		free(filenameW);

		return ret;
	}
}


static void keygen_progress(int phase, int count, void *cbarg_) {
	char buff[1024];
	static char msg[1024];
	cbarg_t *cbarg = cbarg_;

	switch (phase) {
	case 0:
		if (count == 0) {
			UTIL_get_lang_msg("MSG_KEYGEN_GENERATING", pvar, "generating key");
			strncpy_s(msg, sizeof(msg), pvar->UIMsg, _TRUNCATE);
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
			_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%s", pvar->UIMsg);
		}
		break;
	default:
		return;
	}

	SetDlgItemText(cbarg->dlg, IDC_KEYGEN_PROGRESS_LABEL, buff);
	return;
}

// bcrypt KDF形式で秘密鍵を保存する
// based on OpenSSH 6.5:key_save_private(), key_private_to_blob2()
static void save_bcrypt_private_key(char *passphrase, const wchar_t *filename, char *comment, HWND dlg, PTInstVar pvar, int rounds)
{
	const struct ssh2cipher *cipher = NULL;
	char *ciphername = DEFAULT_CIPHERNAME;
	buffer_t *b = NULL;
	buffer_t *kdf = NULL;
	buffer_t *encoded = NULL;
	buffer_t *blob = NULL;
	int blocksize, keylen, ivlen, authlen, i, n;
	unsigned char *key = NULL, salt[SALT_LEN];
	char *kdfname = KDFNAME;
	struct sshcipher_ctx *cc = NULL;
	Key keyblob;
	unsigned char *cp = NULL;
	unsigned int len, check;
	FILE *fp;

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

	cipher = get_cipher_by_name(ciphername);
	blocksize = get_cipher_block_size(cipher);
	keylen = get_cipher_key_len(cipher);
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
	cipher = get_cipher_by_name(ciphername);
	cipher_init_SSH2(&cc, cipher, key, keylen, key + keylen, ivlen, CIPHER_ENCRYPT, pvar);
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
	if (EVP_Cipher(cc->evp, cp, buffer_ptr(b), buffer_len(b)) == 0) {
		//strncpy_s(errmsg, errmsg_len, "Key decrypt error", _TRUNCATE);
		//free(decrypted);
		//goto error;
	}
	cipher_free_SSH2(cc);

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
	fp = _wfopen(filename, L"wb");
	if (fp == NULL) {
		static const TTMessageBoxInfoW info = {
			"TTSSH",
			"MSG_ERROR", L"ERROR",
			"MSG_SAVE_KEY_OPENFILE_ERROR", L"Can't open key file",
			MB_OK | MB_ICONEXCLAMATION
		};
		TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
		goto ed25519_error;
	}
	n = fwrite(buffer_ptr(blob), buffer_len(blob), 1, fp);
	if (n != 1) {
		static const TTMessageBoxInfoW info = {
			"TTSSH",
			"MSG_ERROR", L"ERROR",
			"MSG_SAVE_KEY_WRITEFILE_ERROR", L"Can't open key file",
			MB_OK | MB_ICONEXCLAMATION
		};
		TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
	}
	fclose(fp);


ed25519_error:
	buffer_free(b);
	buffer_free(kdf);
	buffer_free(encoded);
	buffer_free(blob);
}

static BOOL GetPublicKeyFilename(HWND dlg, ssh_keytype type, wchar_t **filename_)
{
	const wchar_t *UILanguageFileW = pvar->ts->UILanguageFileW;

	wchar_t *titleW = NULL;
	GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PUBLICKEY_TITLE", L"Save public key as:", UILanguageFileW, &titleW);

	wchar_t *filterW = NULL;
	const wchar_t *default_filenameW = NULL;
	switch (type) {
	case KEY_RSA1:
		GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PUBLICKEY_RSA1_FILTER",
					 L"SSH1 RSA key(identity.pub)\\0identity.pub\\0All Files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filterW);
		default_filenameW = L"identity.pub";
		break;
	case KEY_RSA:
		GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PUBLICKEY_RSA_FILTER",
					 L"SSH2 RSA key(id_rsa.pub)\\0id_rsa.pub\\0All Files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filterW);
		default_filenameW = L"id_rsa.pub";
		break;
	case KEY_DSA:
		GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PUBLICKEY_DSA_FILTER",
					 L"SSH2 DSA key(id_dsa.pub)\\0id_dsa.pub\\0All Files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filterW);
		default_filenameW = L"id_dsa.pub";
		break;
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PUBLICKEY_ECDSA_FILTER",
					 L"SSH2 ECDSA key(id_ecdsa.pub)\\0id_ecdsa.pub\\0All Files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filterW);
		default_filenameW = L"id_ecdsa.pub";
		break;
	case KEY_ED25519:
		GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PUBLICKEY_ED25519_FILTER",
					 L"SSH2 ED25519 key(id_ed25519.pub)\\0id_ed25519.pub\\0All Files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filterW);
		default_filenameW = L"id_ed25519.pub";
		break;
	default:
		assert(FALSE);
		break;
	}

	TTOPENFILENAMEW ofn = {0};
	ofn.hwndOwner = dlg;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrFile = default_filenameW;
	ofn.lpstrFilter = filterW;
	ofn.lpstrTitle = titleW;
	wchar_t *filename;
	BOOL r = TTGetSaveFileNameW(&ofn, &filename);
	free(titleW);
	free(filterW);
	if (r == FALSE) {
		// cancel/failure
		// int ret = CommDlgExtendedError();
		*filename_ = NULL;
		return FALSE;
	}
	*filename_ = filename;
	return TRUE;
}

static BOOL GetPrivateKeyFilename(HWND dlg, ssh_keytype type, wchar_t **filename_)
{
	const wchar_t *UILanguageFileW = pvar->ts->UILanguageFileW;

	wchar_t *titleW = NULL;
	GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PRIVATEKEY_TITLE", L"Save private key as:", pvar->ts->UILanguageFileW, &titleW);

	wchar_t *filterW = NULL;
	const wchar_t *default_filenameW = NULL;
	switch (private_key.type) {
	case KEY_RSA1:
		GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PRIVATEKEY_RSA1_FILTER",
					 L"SSH1 RSA key(identity)\\0identity\\0All Files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filterW);
		default_filenameW = L"identity";
		break;
	case KEY_RSA:
		GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PRIVATEKEY_RSA_FILTER",
					 L"SSH2 RSA key(id_rsa)\\0id_rsa\\0All Files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filterW);
		default_filenameW = L"id_rsa";
		break;
	case KEY_DSA:
		GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PRIVATEKEY_DSA_FILTER",
					 L"SSH2 DSA key(id_dsa)\\0id_dsa\\0All Files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filterW);
		default_filenameW = L"id_dsa";
		break;
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PRIVATEKEY_ECDSA_FILTER",
					 L"SSH2 ECDSA key(id_ecdsa)\\0id_ecdsa\\0All Files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filterW);
		default_filenameW = L"id_ecdsa";
		break;
	case KEY_ED25519:
		GetI18nStrWW("TTSSH", "FILEDLG_SAVE_PRIVATEKEY_ED25519_FILTER",
					 L"SSH2 ED25519 key(id_ed25519)\\0id_ed25519\\0All Files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filterW);
		default_filenameW = L"id_ed25519";
		break;
	default:
		assert(FALSE);
		break;
	}

	TTOPENFILENAMEW ofn = {0};
	ofn.hwndOwner = dlg;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrFile = default_filenameW;
	ofn.lpstrFilter = filterW;
	ofn.lpstrTitle = titleW;
	wchar_t *filename;
	BOOL r = TTGetSaveFileNameW(&ofn, &filename);
	free(titleW);
	free(filterW);
	if (r == FALSE) {
		// cancel/failure
		// int ret = CommDlgExtendedError();
		*filename_ = NULL;
		return FALSE;
	}
	*filename_ = filename;
	return TRUE;
}

static INT_PTR CALLBACK TTXKeyGenerator(HWND dlg, UINT msg, WPARAM wParam,
                                        LPARAM lParam)
{
	static ssh_keytype key_type;
	static int saved_key_bits;

	switch (msg) {
	case WM_INITDIALOG:
		{
		static const DlgTextInfo text_info[] = {
			{ 0, "DLG_KEYGEN_TITLE" },
			{ IDC_KEYTYPE, "DLG_KEYGEN_KEYTYPE" },
			{ IDC_KEYBITS_LABEL, "DLG_KEYGEN_BITS" },
			{ IDC_KEY_LABEL, "DLG_KEYGEN_PASSPHRASE" },
			{ IDC_CONFIRM_LABEL, "DLG_KEYGEN_PASSPHRASE2" },
			{ IDC_COMMENT_LABEL, "DLG_KEYGEN_COMMENT" },
			{ IDC_SAVE_PUBLIC_KEY, "DLG_KEYGEN_SAVEPUBLIC" },
			{ IDC_SAVE_PRIVATE_KEY, "DLG_KEYGEN_SAVEPRIVATE" },
			{ IDOK, "DLG_KEYGEN_GENERATE" },
			{ IDCANCEL, "BTN_CLOSE" },
			{ IDC_BCRYPT_KDF_CHECK, "DLG_KEYGEN_BCRYPT_KDF" },
			{ IDC_BCRYPT_KDF_ROUNDS_LABEL, "DLG_KEYGEN_BCRYPT_ROUNDS" },
			{ IDC_SSHKEYGENSETUP_HELP, "BTN_HELP" },
		};
		SetI18nDlgStrsW(dlg, "TTSSH", text_info, _countof(text_info), pvar->ts->UILanguageFileW);

		init_password_control(pvar, dlg, IDC_KEY_EDIT, NULL);
		init_password_control(pvar, dlg, IDC_CONFIRM_EDIT, NULL);

		// default key type
		SendMessage(GetDlgItem(dlg, IDC_ED25519_TYPE), BM_SETCHECK, BST_CHECKED, 0);
		key_type = KEY_ED25519;

		// bit 長を変更できる RSA/RSA1 のためにデフォルト値を保存
		//   ここでダイアログから取得すると 0 になってしまうので NG
		//   bit長固定の ED25519 をデフォルトにしたため、WM_INITDIALOG の最後でダイアログから取得するのも NG
		saved_key_bits = 3072;

		// default key bits
		EnableWindow(GetDlgItem(dlg, IDC_KEYBITS), FALSE);
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
		EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), FALSE);
		SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_SETCHECK, BST_CHECKED, 0);
		EnableWindow(GetDlgItem(dlg, IDC_BCRYPT_KDF_ROUNDS), TRUE);
		SetDlgItemInt(dlg, IDC_BCRYPT_KDF_ROUNDS, DEFAULT_ROUNDS, FALSE);
		SendDlgItemMessage(dlg, IDC_BCRYPT_KDF_ROUNDS, EM_LIMITTEXT, 4, 0);

		CenterWindow(dlg, GetParent(dlg));

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
						static const TTMessageBoxInfoW info = {
							"TTSSH",
							NULL, L"Tera Term",
							"MSG_KEYBITS_MIN_ERROR", L"The key bits is too small.",
							MB_OK | MB_ICONEXCLAMATION
						};
						TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
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

			} else {
				// generate_ssh_key()が失敗した場合においても、ダイアログを
				// クローズできるようにしておく。
				EnableWindow(GetDlgItem(dlg, IDOK), TRUE);
				EnableWindow(GetDlgItem(dlg, IDCANCEL), TRUE);

			}
			return TRUE;
			}

		case IDCANCEL:
			// don't forget to free SSH resource!
			free_ssh_key();
			TTEndDialog(dlg, 0); // dialog close
			return TRUE;

		case IDC_SSHKEYGENSETUP_HELP:
			PostMessage(GetParent(dlg), WM_USER_DLGHELP2, HlpMenuSetupSshkeygen, 0);
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
			// saving file dialog
			wchar_t *filename;
			if (GetPublicKeyFilename(dlg, public_key.type, &filename) == FALSE) {
				// cancel or failure
				break;
			}

			char comment[1024];	 // comment string in private key
			GetDlgItemText(dlg, IDC_COMMENT_EDIT, comment, sizeof(comment));

			// saving public key file
			FILE *fp = _wfopen(filename, L"wb");
			free(filename);
			if (fp == NULL) {
				static const TTMessageBoxInfoW info = {
					"TTSSH",
					"MSG_ERROR", L"ERROR",
					"MSG_SAVE_KEY_OPENFILE_ERROR", L"Can't open key file",
					MB_OK | MB_ICONEXCLAMATION
				};
				TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
				break;
			}

			if (public_key.type == KEY_RSA1) { // SSH1 RSA
				RSA *rsa = public_key.rsa;
				int bits;
				char *buf;
				BIGNUM *e, *n;

				RSA_get0_key(rsa, &n, &e, NULL);

				bits = BN_num_bits(n);
				fprintf(fp, "%u", bits);

				buf = BN_bn2dec(e);
				fprintf(fp, " %s", buf);
				OPENSSL_free(buf);

				buf = BN_bn2dec(n);
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
				BIGNUM *e, *n;
				BIGNUM *p, *q, *g, *pub_key;

				b = buffer_init();
				if (b == NULL)
					goto public_error;

				switch (public_key.type) {
				case KEY_DSA: // DSA
					DSA_get0_pqg(dsa, &p, &q, &g);
					DSA_get0_key(dsa, &pub_key, NULL);

					keyname = "ssh-dss";
					buffer_put_string(b, keyname, strlen(keyname));
					buffer_put_bignum2(b, p);
					buffer_put_bignum2(b, q);
					buffer_put_bignum2(b, g);
					buffer_put_bignum2(b, pub_key);
					break;

				case KEY_RSA: // RSA
					RSA_get0_key(rsa, &n, &e, NULL);
					keyname = "ssh-rsa";
					buffer_put_string(b, keyname, strlen(keyname));
					buffer_put_bignum2(b, e);
					buffer_put_bignum2(b, n);
					break;

				case KEY_ECDSA256: // ECDSA
				case KEY_ECDSA384:
				case KEY_ECDSA521:
					keyname = get_ssh2_hostkey_type_name(public_key.type);
					buffer_put_string(b, keyname, strlen(keyname));
					s = curve_keytype_to_name(public_key.type);
					buffer_put_string(b, s, strlen(s));
					buffer_put_ecpoint(b, EC_KEY_get0_group(ecdsa),
					                      EC_KEY_get0_public_key(ecdsa));
					break;

				case KEY_ED25519:
					keyname = get_ssh2_hostkey_type_name(public_key.type);
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

			// パスフレーズのチェックを行う。パスフレーズは秘密鍵ファイルに付ける。
			SendMessage(GetDlgItem(dlg, IDC_KEY_EDIT), WM_GETTEXT, sizeof(buf), (LPARAM)buf);
			SendMessage(GetDlgItem(dlg, IDC_CONFIRM_EDIT), WM_GETTEXT, sizeof(buf_conf), (LPARAM)buf_conf);

			// check matching
			if (strcmp(buf, buf_conf) != 0) {
				static const TTMessageBoxInfoW info = {
					"TTSSH",
					"MSG_ERROR", L"ERROR",
					"MSG_SAVE_PRIVATE_KEY_MISMATCH_ERROR", L"Two passphrases don't match.",
					MB_OK | MB_ICONEXCLAMATION
				};
				TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
				break;
			}

			// check empty-passphrase (this is warning level)
			if (buf[0] == '\0') {
				static const TTMessageBoxInfoW info = {
					"TTSSH",
					"MSG_WARNING", L"WARNING",
					"MSG_SAVE_PRIVATEKEY_EMPTY_WARN", L"Are you sure that you want to use a empty passphrase?",
					MB_OK | MB_ICONEXCLAMATION
				};
				ret = TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
				if (ret == IDNO)
					break;
			}

			// number of rounds
			if (SendMessage(GetDlgItem(dlg, IDC_BCRYPT_KDF_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED) {
				rounds = GetDlgItemInt(dlg, IDC_BCRYPT_KDF_ROUNDS, NULL, FALSE);
				if (rounds < SSH_KEYGEN_MINIMUM_ROUNDS) {
					static const TTMessageBoxInfoW info = {
						"TTSSH",
						NULL, L"Tera Term",
						"MSG_BCRYPT_ROUNDS_MIN_ERROR", L"The number of rounds is too small.",
						MB_OK | MB_ICONEXCLAMATION
					};
					TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
					break;
				}
				if (rounds > SSH_KEYGEN_MAXIMUM_ROUNDS) {
					static const TTMessageBoxInfoW info = {
						"TTSSH",
						NULL, L"Tera Term",
						"MSG_BCRYPT_ROUNDS_MAX_ERROR", L"The number of rounds is too large.",
						MB_OK | MB_ICONEXCLAMATION
					};
					TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
					break;
				}
			}

			char comment[1024];	 // comment string in private key
			ssh_make_comment(comment, sizeof(comment));

			// saving file dialog
			wchar_t *filename;
			if (GetPrivateKeyFilename(dlg, private_key.type, &filename) == FALSE) {
				// cancel
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
				const struct ssh2cipher *cipher = NULL;
				struct sshcipher_ctx *cc = NULL;
				FILE *fp;
				char wrapped[4096];
				BIGNUM *e, *n, *d, *dmp1, *dmq1, *iqmp, *p, *q;

				if (passphrase[0] == '\0') { // passphrase is empty
					cipher_num = SSH_CIPHER_NONE;
				} else {
					cipher_num = SSH_CIPHER_3DES; // 3DES
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
				RSA_get0_key(rsa, &n, &e, &d);
				RSA_get0_factors(rsa, &p, &q);
				RSA_get0_crt_params(rsa, &dmp1, &dmq1, &iqmp);
				buffer_put_bignum(b, d);
				buffer_put_bignum(b, iqmp);
				buffer_put_bignum(b, q);
				buffer_put_bignum(b, p);

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
				buffer_put_int(enc, BN_num_bits(n));
				buffer_put_bignum(enc, n);
				buffer_put_bignum(enc, e);
				buffer_put_string(enc, comment, strlen(comment));

				// setup the MD5ed passphrase to cipher encryption key
				MD5_Init(&md);
				MD5_Update(&md, (const unsigned char *)passphrase, strlen(passphrase));
				MD5_Final(digest, &md);
				if (cipher_num == SSH_CIPHER_NONE) {
					cipher = get_cipher_by_name("none");
					cipher_init_SSH2(&cc, cipher, digest, 16, NULL, 0, CIPHER_ENCRYPT, pvar);
				} else {
					cipher = get_cipher_by_name("3des");
					cipher_init_SSH2(&cc, cipher, digest, 16, NULL, 0, CIPHER_ENCRYPT, pvar);
				}
				len = buffer_len(b);
				if (len % 8) { // fatal error
					goto error;
				}

				// check buffer overflow
				if (buffer_overflow_verify(enc, len) && (sizeof(wrapped) < len)) {
					goto error;
				}

				if (EVP_Cipher(cc->evp, wrapped, buffer_ptr(b), len) == 0) {
					goto error;
				}

				buffer_append(enc, wrapped, len);

				// saving private key file (binary mode)
				fp = _wfopen(filename, L"wb");
				if (fp == NULL) {
					static const TTMessageBoxInfoW info = {
						"TTSSH",
						"MSG_ERROR", L"ERROR",
						"MSG_SAVE_KEY_OPENFILE_ERROR", L"Can't open key file",
						MB_OK | MB_ICONEXCLAMATION
					};
					TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
					break;
				}
				fwrite(buffer_ptr(enc), buffer_len(enc), 1, fp);

				fclose(fp);

error:;
				buffer_free(b);
				buffer_free(enc);
				cipher_free_SSH2(cc);

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

				fp = _wfopen(filename, L"w");
				if (fp == NULL) {
					static const TTMessageBoxInfoW info = {
						"TTSSH",
						"MSG_ERROR", L"ERROR",
						"MSG_SAVE_KEY_OPENFILE_ERROR", L"Can't open key file",
						MB_OK | MB_ICONEXCLAMATION
					};
					TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
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
					static const TTMessageBoxInfoW info = {
						"TTSSH",
						"MSG_ERROR", L"ERROR",
						"MSG_SAVE_KEY_WRITEFILE_ERROR", L"Can't open key file",
						MB_OK | MB_ICONEXCLAMATION
					};
					TTMessageBoxW(dlg, &info, pvar->ts->UILanguageFileW);
				}
				fclose(fp);
			}
			free(filename);

			}
			break;

		}
		break;
	}

	return FALSE;
}


static int PASCAL TTXProcessCommand(HWND hWin, WORD cmd)
{
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
		UTIL_SetDialogFont();
		if (TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SSHSCP), hWin, TTXScpDialog,
			(LPARAM) pvar) == -1) {
			static const TTMessageBoxInfoW info = {
				"TTSSH",
				"MSG_TTSSH_ERROR", L"TTSSH Error",
				"MSG_CREATEWINDOW_SCP_ERROR", L"Unable to display SCP dialog box.",
				MB_OK | MB_ICONEXCLAMATION
			};
			TTMessageBoxW(hWin, &info, pvar->ts->UILanguageFileW);
		}
		return 1;

	case ID_SSHKEYGENMENU:
		UTIL_SetDialogFont();
		if (TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SSHKEYGEN), hWin, TTXKeyGenerator,
			(LPARAM) pvar) == -1) {
			static const TTMessageBoxInfoW info = {
				"TTSSH",
				"MSG_TTSSH_ERROR", L"TTSSH Error",
				"MSG_CREATEWINDOW_KEYGEN_ERROR", L"Unable to display Key Generator dialog box.",
				MB_OK | MB_ICONEXCLAMATION
			};
			TTMessageBoxW(hWin, &info, pvar->ts->UILanguageFileW);
		}
		return 1;

	case ID_ABOUTMENU: {
		AboutDlgData *data = (AboutDlgData *)calloc(1, sizeof(*data));
		UTIL_SetDialogFont();
		data->pvar = pvar;
		if (TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_ABOUTDIALOG), hWin, TTXAboutDlg, (LPARAM)data) == -1) {
			static const TTMessageBoxInfoW info = {
				"TTSSH",
				"MSG_TTSSH_ERROR", L"TTSSH Error",
				"MSG_CREATEWINDOW_ABOUT_ERROR", L"Unable to display About dialog box.",
				MB_OK | MB_ICONEXCLAMATION
			};
			TTMessageBoxW(hWin, &info, pvar->ts->UILanguageFileW);
		}
		free(data);
		return 1;
	}
	case ID_SSHAUTH:
		UTIL_SetDialogFont();
		AUTH_do_cred_dialog(pvar);
		return 1;
	case ID_SSHSETUPMENU:
		UTIL_SetDialogFont();
		if (TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SSHSETUP),
							 hWin, TTXSetupDlg, (LPARAM) pvar) == -1) {
			static const TTMessageBoxInfoW info = {
				"TTSSH",
				"MSG_TTSSH_ERROR", L"TTSSH Error",
				"MSG_CREATEWINDOW_SETUP_ERROR", L"Unable to display TTSSH Setup dialog box.",
				MB_OK | MB_ICONEXCLAMATION
			};
			TTMessageBoxW(hWin, &info, pvar->ts->UILanguageFileW);
		}
		return 1;
	case ID_SSHAUTHSETUPMENU:
		UTIL_SetDialogFont();
		AUTH_do_default_cred_dialog(pvar);
		return 1;
	case ID_SSHFWDSETUPMENU:
		UTIL_SetDialogFont();
		FWDUI_do_forwarding_dialog(pvar);
		return 1;
	case ID_SSHUNKNOWNHOST:
		UTIL_SetDialogFont();
		HOSTS_do_unknown_host_dialog(hWin, pvar);
		return 1;
	case ID_SSHDIFFERENTKEY:
		UTIL_SetDialogFont();
		HOSTS_do_different_key_dialog(hWin, pvar);
		return 1;
	case ID_SSHDIFFERENT_TYPE_KEY:
		UTIL_SetDialogFont();
		HOSTS_do_different_type_key_dialog(hWin, pvar);
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

static void dquote_string(const char *str, char *dst, size_t dst_len)
{
	wchar_t *dstW = malloc(sizeof(wchar_t) * dst_len);
	wchar_t *strW = ToWcharA(str);
	dquote_stringW(strW, dstW, dst_len);
	WideCharToACP_t(dstW, dst, dst_len);
	free(strW);
	free(dstW);
}

static void PASCAL TTXSetCommandLine(wchar_t *cmd, int cmdlen, PGetHNRec rec)
{
	wchar_t tmpFile[MAX_PATH];
	wchar_t tmpPath[1024];
	wchar_t *buf;
	wchar_t *p;
	int i;

	GetTempPathW(_countof(tmpPath), tmpPath);
	GetTempFileNameW(tmpPath, L"TTX", 0, tmpFile);
	WriteIniBom(tmpFile, TRUE);

	for (i = 0; cmd[i] != ' ' && cmd[i] != 0; i++) {
	}

	if (i < cmdlen) {
		int ssh_enable = -1;

		buf = malloc(sizeof(wchar_t) * (cmdlen+1));
		wcsncpy_s(buf, cmdlen, cmd + i, _TRUNCATE);
		buf[cmdlen] = 0;
		cmd[i] = 0;

		write_ssh_options(pvar, tmpFile, &pvar->settings, FALSE);

		wcsncat_s(cmd, cmdlen, L" /ssh-consume=", _TRUNCATE);
		wcsncat_s(cmd, cmdlen, tmpFile, _TRUNCATE);

		wcsncat_s(cmd, cmdlen, buf, _TRUNCATE);

		// コマンドラインでの指定をチェック
		p = wcsstr(buf, L" /ssh");
		if (p != NULL) {
			switch (*(p + 5)) {
				case '\0':
				case ' ':
				case '1':
				case '2':
					ssh_enable = 1;
			}
		}
		else if (wcsstr(buf, L" /nossh") ||
		         wcsstr(buf, L" /telnet")) {
			ssh_enable = 0;
		}

		// ホスト名で /ssh /1, /ssh  /2, /ssh1, /ssh2, /nossh, /telnet が
		// 指定されたときは、ラジオボタンの SSH および SSH プロトコルバージョンを
		// 適用するのをやめる
		if (pvar->hostdlg_Enabled && ssh_enable == -1) {
			wcsncat_s(cmd, cmdlen, L" /ssh", _TRUNCATE);

			// add option of SSH protcol version (2004.10.11 yutaka)
			if (pvar->settings.ssh_protocol_version == 2) {
				wcsncat_s(cmd, cmdlen, L" /2", _TRUNCATE);
			} else {
				wcsncat_s(cmd, cmdlen, L" /1", _TRUNCATE);
			}

		}

		// セッション複製の場合は、自動ログイン用パラメータを付ける。
		if (wcsstr(buf, L"DUPLICATE")) {
			char mark[MAX_PATH];
			wchar_t tmp[MAX_PATH*2];

			// 自動ログインの場合は下記フラグが0のため、必要なコマンドを付加する。
			if (!pvar->hostdlg_Enabled) {
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE,
							 L" /ssh /%d", pvar->settings.ssh_protocol_version);
				wcsncat_s(cmd, cmdlen, tmp, _TRUNCATE);
			}

			// パスワードを覚えていて、かつ複数認証要求でない場合にのみコマンドラインに渡す。
			if (pvar->settings.remember_password &&
			    !pvar->auth_state.multiple_required_auth &&
			    pvar->auth_state.cur_cred.method == SSH_AUTH_PASSWORD) {
				dquote_string(pvar->auth_state.cur_cred.password, mark, sizeof(mark));
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE,
				             L" /auth=password /user=%hs /passwd=%hs", pvar->auth_state.user, mark);
				wcsncat_s(cmd, cmdlen, tmp, _TRUNCATE);

			} else if (pvar->settings.remember_password &&
			           !pvar->auth_state.multiple_required_auth &&
			           pvar->auth_state.cur_cred.method == SSH_AUTH_RSA) {
				wchar_t markW[MAX_PATH];
				dquote_string(pvar->auth_state.cur_cred.password, mark, sizeof(mark));
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE,
				             L" /auth=publickey /user=%hs /passwd=%hs", pvar->auth_state.user, mark);
				wcsncat_s(cmd, cmdlen, tmp, _TRUNCATE);

				dquote_stringW(pvar->session_settings.DefaultRSAPrivateKeyFile, markW, _countof(markW));
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE, L" /keyfile=%s", markW);
				wcsncat_s(cmd, cmdlen, tmp, _TRUNCATE);

			} else if (pvar->settings.remember_password &&
			           !pvar->auth_state.multiple_required_auth &&
			           pvar->auth_state.cur_cred.method == SSH_AUTH_TIS) {
				dquote_string(pvar->auth_state.cur_cred.password, mark, sizeof(mark));
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE,
				             L" /auth=challenge /user=%hs /passwd=%hs", pvar->auth_state.user, mark);
				wcsncat_s(cmd, cmdlen, tmp, _TRUNCATE);

			} else if (pvar->auth_state.cur_cred.method == SSH_AUTH_PAGEANT &&
			           !pvar->auth_state.multiple_required_auth) {
				_snwprintf_s(tmp, _countof(tmp), _TRUNCATE,
				             L" /auth=pageant /user=%hs", pvar->auth_state.user);
				wcsncat_s(cmd, cmdlen, tmp, _TRUNCATE);

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

static void PASCAL TTXCloseTCP(TTXSockHooks *hooks)
{
	if (pvar->session_settings.Enabled) {
		pvar->socket = INVALID_SOCKET;

		logputs(LOG_LEVEL_VERBOSE, "Terminating SSH session...");

		// 認証ダイアログが残っていれば閉じる。
		HOSTS_notify_closing_on_exit(pvar);
		AUTH_notify_closing_on_exit(pvar);

		*hooks->Precv = pvar->Precv;
		*hooks->Psend = pvar->Psend;
		*hooks->PWSAAsyncSelect = pvar->PWSAAsyncSelect;
		*hooks->Pconnect = pvar->Pconnect;

		pvar->ts->DisableTCPEchoCR = pvar->origDisableTCPEchoCR;
	}

	TTXEnd();
	pvar->fatal_error = FALSE;
	init_TTSSH(pvar);
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
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		// リーク時のブロック番号を元にブレークを仕掛けるには、以下のようにする。
		//_CrtSetBreakAlloc(3228);
#endif
		setlocale(LC_ALL, "");
		DisableThreadLibraryCalls(hInstance);
		hInst = hInstance;
		pvar = &InstVar;
		__mem_mapping =
			CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
			                  sizeof(TS_SSH), TTSSH_FILEMAPNAME);
		if (__mem_mapping == NULL) {
			/* fake it. The settings won't be shared, but what the heck. */
			pvar->ts_SSH = NULL;
		} else {
			pvar->ts_SSH =
				(TS_SSH *) MapViewOfFile(__mem_mapping, FILE_MAP_WRITE, 0,
				                         0, 0);
			if (pvar->ts_SSH->struct_size != sizeof(TS_SSH)) {
				// 構造体サイズが異なっていたら共有メモリは使用しない
				// バージョンが異なるときや開発時に発生する
				pvar->ts_SSH = NULL;
			}
		}
		if (pvar->ts_SSH == NULL) {
			/* fake it. The settings won't be shared, but what the heck. */
			pvar->ts_SSH = (TS_SSH *) malloc(sizeof(TS_SSH));
			pvar->ts_SSH->struct_size = sizeof(TS_SSH);
			if (__mem_mapping != NULL) {
				CloseHandle(__mem_mapping);
				__mem_mapping = NULL;
			}
		}
		break;
	case DLL_PROCESS_DETACH:
		/* do process cleanup */
		if (__mem_mapping == NULL) {
			free(pvar->ts_SSH);
		} else {
			UnmapViewOfFile(__mem_mapping);
			CloseHandle(__mem_mapping);
		}
		break;
	}
	return TRUE;
}

/* vim: set ts=4 sw=4 ff=dos : */
