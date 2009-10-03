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

#ifndef __TTXSSH_H
#define __TTXSSH_H

#pragma warning(3 : 4035)

#ifndef NO_INET6
#include <winsock2.h>
#include <ws2tcpip.h>
/* actual body of in6addr_any and in6addr_loopback is disappeared?? */
#undef IN6_IS_ADDR_LOOPBACK
#define IN6_IS_ADDR_LOOPBACK(a)         \
        ((*(unsigned int *)(&(a)->s6_addr[0]) == 0) &&     \
         (*(unsigned int *)(&(a)->s6_addr[4]) == 0) &&     \
         (*(unsigned int *)(&(a)->s6_addr[8]) == 0) &&     \
         (*(unsigned int *)(&(a)->s6_addr[12]) == ntohl(1)))
/* work around for MS Platform SDK Oct 2000 */
#include <malloc.h> /* prevent of conflict stdlib.h */
#endif /* NO_INET6 */
#include <stdlib.h>
#include <crtdbg.h>

#include "i18n.h"
#include "ttlib.h"

typedef struct _TInstVar FAR * PTInstVar;

#include "util.h"
#include "pkt.h"
#include "ssh.h"
#include "auth.h"
#include "crypt.h"
#include "hosts.h"
#include "fwd.h"

#include <openssl/dh.h>
#include <openssl/evp.h>
#include "buffer.h"

/* tttypes.h で定義されている EM マクロが openssl/rsa.h (OpenSSL 0.9.8)の関数プロトタイプ宣言に
 * ある引数名と重複してしまうので、ビルドエラーとなる。下記3ヘッダのinclude位置を下記に移動した。
 * (2005.7.9 yutaka)
 */
#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"

HANDLE hInst; /* Instance handle of TTXSSH.DLL */

#define ID_SSHSCPMENU       52110
#define ID_SSHSETUPMENU     52310
#define ID_SSHAUTHSETUPMENU 52320
#define ID_SSHFWDSETUPMENU  52330
#define ID_SSHKEYGENMENU    52340
#define ID_ABOUTMENU        52910

#define ID_SSHAUTH            62501
#define ID_SSHUNKNOWNHOST     62502
#define ID_SSHDIFFERENTHOST   62503
#define ID_SSHASYNCMESSAGEBOX 62504

#define OPTION_NONE     0
#define OPTION_CLEAR    1
#define OPTION_REPLACE  2

/*
These are the fields that WOULD go in Tera Term's 'ts' structure, if
we could put them there.
*/
typedef struct _TS_SSH {
	BOOL Enabled;
	int CompressionLevel; /* 0 = NONE, else 1-9 */
	char DefaultUserName[256];

	/* this next option is a string of digits. Each digit represents a
	   cipher. The first digit is the most preferred cipher, and so on.
	   The digit SSH_CIPHER_NONE signifies that any ciphers after it are
	   disabled. */
	char CipherOrder[SSH_CIPHER_MAX+1];

	char KnownHostsFiles[2048];
	int DefaultAuthMethod;
	char DefaultRhostsLocalUserName[256];
	char DefaultRhostsHostPrivateKeyFile[1024];
	char DefaultRSAPrivateKeyFile[1024];

	char DefaultForwarding[2048];
	BOOL TryDefaultAuth;

	int LogLevel;      /* 0 = NONE, 100 = Verbose */
	int WriteBufferSize;
	int LocalForwardingIdentityCheck;

	int ssh_protocol_version; // SSH version (2004.10.11 yutaka)
	int ssh_heartbeat_overtime; // SSH heartbeat(keepalive) (2004.12.11 yutaka)
	// whether password will permanently store on heap memory (2006.8.5 yutaka)
	int remember_password;

	// try auth with "none" method for disable unsupported on dialog (2007.9.24 maya)
	BOOL CheckAuthListFirst;

	// Enable connection to the server that has RSA key length less than 768 bit (2008.9.11 maya)
	BOOL EnableRsaShortKeyServer;

	// Enable Agent forwarding
	BOOL ForwardAgent;
} TS_SSH;

typedef struct _TInstVar {
	PTTSet ts;
	PComVar cv;

	/* shared memory for settings across instances. Basically it's
	   a cache for the INI file.*/
	TS_SSH FAR * ts_SSH;

	int fatal_error;
	int showing_err;
	char FAR * err_msg;

	Tconnect Pconnect;
	Trecv Precv;
	Tsend Psend;
	TWSAAsyncSelect PWSAAsyncSelect;
	TWSAGetLastError PWSAGetLastError;

	PReadIniFile ReadIniFile;
	PWriteIniFile WriteIniFile;
	PParseParam ParseParam;

	SOCKET socket;
	HWND NotificationWindow;
	unsigned int notification_msg;
	long notification_events;
	HICON OldSmallIcon;
	HICON OldLargeIcon;

	BOOL hostdlg_activated;
	BOOL hostdlg_Enabled;

	int protocol_major;
	int protocol_minor;

	PKTState pkt_state;
	SSHState ssh_state;
	AUTHState auth_state;
	CRYPTState crypt_state;
	HOSTSState hosts_state;
	FWDState fwd_state;

/* The settings applied to the current session. The user may change
   the settings but usually we don't want that to affect the session
   in progress (race conditions). So user setup changes usually
   modify the 'settings' field below. */
	TS_SSH session_settings;

/* our copy of the global settings. This is synced up with the shared
   memory only when we do a ReadIniFile or WriteIniFile
   (i.e. the user loads or saves setup) */
	TS_SSH settings;

	// SSH2
	DH *kexdh;
	char server_version_string[128];
	char client_version_string[128];
	buffer_t *my_kex;
	buffer_t *peer_kex;
	enum kex_exchange kex_type; // KEX algorithm
	enum hostkey_type hostkey_type;
	SSHCipher ctos_cipher;
	SSHCipher stoc_cipher;
	enum hmac_type ctos_hmac;
	enum hmac_type stoc_hmac;
	enum compression_type ctos_compression;
	enum compression_type stoc_compression;
	int we_need;
	int key_done;
	int rekeying;
	char *session_id;
	int session_id_len;
	Newkeys ssh2_keys[MODE_MAX];
	EVP_CIPHER_CTX evpcip[MODE_MAX];
	int userauth_success;
	int shell_id;
	/*int remote_id;*/
	int session_nego_status;
	/*
	unsigned int local_window;
	unsigned int local_window_max;
	unsigned int local_consumed;
	unsigned int local_maxpacket;
	unsigned int remote_window;
	unsigned int remote_maxpacket;
	*/
	int client_key_bits;
	int server_key_bits;
	int kexgex_min;
	int kexgex_bits;
	int kexgex_max;
	int ssh2_autologin;
	int ask4passwd;
	SSHAuthMethod ssh2_authmethod;
	char ssh2_username[MAX_PATH];
	char ssh2_password[MAX_PATH];
	char ssh2_keyfile[MAX_PATH];
	time_t ssh_heartbeat_tick;
	HANDLE ssh_heartbeat_thread;
	int keyboard_interactive_password_input;
	int userauth_retry_count;
	buffer_t *decomp_buffer;
	char *ssh2_authlist;
	BOOL tryed_ssh2_authlist;
	HWND ssh_hearbeat_dialog;

	/* Pageant との通信用 */
	unsigned char *pageant_key;
	unsigned char *pageant_curkey;
	int pageant_keylistlen;
	int pageant_keycount;
	int pageant_keycurrent;
	BOOL pageant_keyfinal;// SSH2 PK_OK が来たときに TRUE にする

	// agent forward
	BOOL agentfwd_enable;

	BOOL origDisableTCPEchoCR;

	BOOL nocheck_known_hosts;
} TInstVar;

#define LOG_LEVEL_FATAL      5
#define LOG_LEVEL_ERROR      10
#define LOG_LEVEL_URGENT     20
#define LOG_LEVEL_WARNING    30
#define LOG_LEVEL_VERBOSE    100
#define LOG_LEVEL_SSHDUMP    200

#define SSHv1(pvar) ((pvar)->protocol_major == 1)
#define SSHv2(pvar) ((pvar)->protocol_major == 2)

void notify_established_secure_connection(PTInstVar pvar);
void notify_closed_connection(PTInstVar pvar);
void notify_nonfatal_error(PTInstVar pvar, char FAR * msg);
void notify_fatal_error(PTInstVar pvar, char FAR * msg);
void notify_verbose_message(PTInstVar pvar, char FAR * msg, int level);

void get_teraterm_dir_relative_name(char FAR * buf, int bufsize, char FAR * basename);
int copy_teraterm_dir_relative_path(char FAR * dest, int destsize, char FAR * basename);
void get_file_version(char *exefile, int *major, int *minor, int *release, int *build);
int uuencode(unsigned char *src, int srclen, unsigned char *target, int targsize);

#endif
