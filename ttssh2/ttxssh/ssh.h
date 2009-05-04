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

#ifndef __SSH_H
#define __SSH_H

#include "zlib.h"
#include <openssl/evp.h>

#include "buffer.h"

#define DEBUG_PRINT_TO_FILE(base, msg, len) { \
	static int count = 0; \
	debug_print(count + base, msg, len); \
	count++; \
}

// from OpenSSH
extern const EVP_CIPHER *evp_aes_128_ctr(void);

// yutaka
#define SSH2_USE


/* Some of this code has been adapted from Ian Goldberg's Pilot SSH */

typedef enum {
	SSH_MSG_NONE, SSH_MSG_DISCONNECT, SSH_SMSG_PUBLIC_KEY, //2
	SSH_CMSG_SESSION_KEY, SSH_CMSG_USER, SSH_CMSG_AUTH_RHOSTS, // 5
	SSH_CMSG_AUTH_RSA, SSH_SMSG_AUTH_RSA_CHALLENGE,
	SSH_CMSG_AUTH_RSA_RESPONSE, SSH_CMSG_AUTH_PASSWORD,
	SSH_CMSG_REQUEST_PTY, // 10
	SSH_CMSG_WINDOW_SIZE, SSH_CMSG_EXEC_SHELL,
	SSH_CMSG_EXEC_CMD, SSH_SMSG_SUCCESS, SSH_SMSG_FAILURE,
	SSH_CMSG_STDIN_DATA, SSH_SMSG_STDOUT_DATA, SSH_SMSG_STDERR_DATA,
	SSH_CMSG_EOF, SSH_SMSG_EXITSTATUS,
	SSH_MSG_CHANNEL_OPEN_CONFIRMATION, SSH_MSG_CHANNEL_OPEN_FAILURE,
	SSH_MSG_CHANNEL_DATA, SSH_MSG_CHANNEL_INPUT_EOF,
	SSH_MSG_CHANNEL_OUTPUT_CLOSED, SSH_MSG_OBSOLETED0,
	SSH_SMSG_X11_OPEN, SSH_CMSG_PORT_FORWARD_REQUEST, SSH_MSG_PORT_OPEN,
	SSH_CMSG_AGENT_REQUEST_FORWARDING, SSH_SMSG_AGENT_OPEN,
	SSH_MSG_IGNORE, SSH_CMSG_EXIT_CONFIRMATION,
	SSH_CMSG_X11_REQUEST_FORWARDING, SSH_CMSG_AUTH_RHOSTS_RSA,
	SSH_MSG_DEBUG, SSH_CMSG_REQUEST_COMPRESSION,
	SSH_CMSG_MAX_PACKET_SIZE, SSH_CMSG_AUTH_TIS,
	SSH_SMSG_AUTH_TIS_CHALLENGE, SSH_CMSG_AUTH_TIS_RESPONSE,
	SSH_CMSG_AUTH_KERBEROS, SSH_SMSG_AUTH_KERBEROS_RESPONSE
} SSHMessage;

typedef enum {
	SSH_CIPHER_NONE, SSH_CIPHER_IDEA, SSH_CIPHER_DES, SSH_CIPHER_3DES,
	SSH_CIPHER_TSS, SSH_CIPHER_RC4, SSH_CIPHER_BLOWFISH,
	// for SSH2
	SSH2_CIPHER_3DES_CBC, SSH2_CIPHER_AES128_CBC,
	SSH2_CIPHER_AES192_CBC, SSH2_CIPHER_AES256_CBC,
	SSH2_CIPHER_BLOWFISH_CBC, SSH2_CIPHER_AES128_CTR,
	SSH2_CIPHER_AES192_CTR, SSH2_CIPHER_AES256_CTR,
	SSH2_CIPHER_ARCFOUR, SSH2_CIPHER_ARCFOUR128, SSH2_CIPHER_ARCFOUR256,
	SSH2_CIPHER_CAST128_CBC,
} SSHCipher;

#define SSH_CIPHER_MAX SSH2_CIPHER_CAST128_CBC

typedef enum {
	SSH_AUTH_NONE, SSH_AUTH_RHOSTS, SSH_AUTH_RSA, SSH_AUTH_PASSWORD,
	SSH_AUTH_RHOSTS_RSA, SSH_AUTH_TIS, SSH_AUTH_KERBEROS,
	SSH_AUTH_PAGEANT = 16,
} SSHAuthMethod;

#define SSH_AUTH_MAX SSH_AUTH_PAGEANT

typedef enum {
	SSH_GENERIC_AUTHENTICATION, SSH_TIS_AUTHENTICATION
} SSHAuthMode;

#define SSH_PROTOFLAG_SCREEN_NUMBER 1
#define SSH_PROTOFLAG_HOST_IN_FWD_OPEN 2

enum channel_type {
	TYPE_SHELL, TYPE_PORTFWD, TYPE_SCP, TYPE_SFTP, TYPE_AGENT,
};

// for SSH1
#define SSH_MAX_SEND_PACKET_SIZE   250000

// for SSH2
/* default window/packet sizes for tcp/x11-fwd-channel */
// changed CHAN_SES_WINDOW_DEFAULT from 32KB to 128KB. (2007.10.29 maya)
#define CHAN_SES_PACKET_DEFAULT (32*1024)
#define CHAN_SES_WINDOW_DEFAULT (4*CHAN_SES_PACKET_DEFAULT)
#define CHAN_TCP_PACKET_DEFAULT (32*1024)
#define CHAN_TCP_WINDOW_DEFAULT (4*CHAN_TCP_PACKET_DEFAULT)
#if 0 // unused
#define CHAN_X11_PACKET_DEFAULT (16*1024)
#define CHAN_X11_WINDOW_DEFAULT (4*CHAN_X11_PACKET_DEFAULT)
#endif


/* SSH2 constants */

/* SSH2 messages */
#define SSH2_MSG_DISCONNECT             1
#define SSH2_MSG_IGNORE                 2
#define SSH2_MSG_UNIMPLEMENTED          3
#define SSH2_MSG_DEBUG                  4
#define SSH2_MSG_SERVICE_REQUEST        5
#define SSH2_MSG_SERVICE_ACCEPT         6

#define SSH2_MSG_KEXINIT                20
#define SSH2_MSG_NEWKEYS                21

#define SSH2_MSG_KEXDH_INIT             30
#define SSH2_MSG_KEXDH_REPLY            31

#define SSH2_MSG_KEX_DH_GEX_GROUP           31
#define SSH2_MSG_KEX_DH_GEX_INIT            32
#define SSH2_MSG_KEX_DH_GEX_REPLY           33
#define SSH2_MSG_KEX_DH_GEX_REQUEST         34

#define SSH2_MSG_USERAUTH_REQUEST            50
#define SSH2_MSG_USERAUTH_FAILURE            51
#define SSH2_MSG_USERAUTH_SUCCESS            52
#define SSH2_MSG_USERAUTH_BANNER             53

#define SSH2_MSG_USERAUTH_PK_OK              60
#define SSH2_MSG_USERAUTH_PASSWD_CHANGEREQ   60
#define SSH2_MSG_USERAUTH_INFO_REQUEST          60
#define SSH2_MSG_USERAUTH_INFO_RESPONSE         61

#define SSH2_MSG_GLOBAL_REQUEST                  80
#define SSH2_MSG_REQUEST_SUCCESS                 81
#define SSH2_MSG_REQUEST_FAILURE                 82
#define SSH2_MSG_CHANNEL_OPEN                    90
#define SSH2_MSG_CHANNEL_OPEN_CONFIRMATION       91
#define SSH2_MSG_CHANNEL_OPEN_FAILURE            92
#define SSH2_MSG_CHANNEL_WINDOW_ADJUST           93
#define SSH2_MSG_CHANNEL_DATA                    94
#define SSH2_MSG_CHANNEL_EXTENDED_DATA           95
#define SSH2_MSG_CHANNEL_EOF                     96
#define SSH2_MSG_CHANNEL_CLOSE                   97
#define SSH2_MSG_CHANNEL_REQUEST                 98
#define SSH2_MSG_CHANNEL_SUCCESS                 99
#define SSH2_MSG_CHANNEL_FAILURE                 100

/* SSH2 miscellaneous constants */
#define SSH2_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT      1
#define SSH2_DISCONNECT_PROTOCOL_ERROR                   2
#define SSH2_DISCONNECT_KEY_EXCHANGE_FAILED              3
#define SSH2_DISCONNECT_HOST_AUTHENTICATION_FAILED       4
#define SSH2_DISCONNECT_MAC_ERROR                        5
#define SSH2_DISCONNECT_COMPRESSION_ERROR                6
#define SSH2_DISCONNECT_SERVICE_NOT_AVAILABLE            7
#define SSH2_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED   8
#define SSH2_DISCONNECT_HOST_KEY_NOT_VERIFIABLE          9
#define SSH2_DISCONNECT_CONNECTION_LOST                  10
#define SSH2_DISCONNECT_BY_APPLICATION                   11

#define SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED    1
#define SSH2_OPEN_CONNECT_FAILED                 2
#define SSH2_OPEN_UNKNOWN_CHANNEL_TYPE           3
#define SSH2_OPEN_RESOURCE_SHORTAGE              4

// キー交換アルゴリズム
#define KEX_DH1     "diffie-hellman-group1-sha1"
#define KEX_DH14    "diffie-hellman-group14-sha1"
#define KEX_DHGEX   "diffie-hellman-group-exchange-sha1"

// support of "Compression delayed" (2006.6.23 maya)
enum compression_type {
	COMP_NONE,
	COMP_ZLIB,
	COMP_DELAYED,
	COMP_UNKNOWN
};

enum kex_exchange {
	KEX_DH_GRP1_SHA1,
	KEX_DH_GRP14_SHA1,
	KEX_DH_GEX_SHA1,
	KEX_MAX
};

enum hostkey_type {
	KEY_RSA1,
	KEY_RSA,
	KEY_DSA,
	KEY_UNSPEC,
};

// 下記のインデックスは ssh2_macs[] と合わせること。
enum hmac_type {
	HMAC_SHA1,
	HMAC_MD5,
	HMAC_UNKNOWN
};

#define KEX_DEFAULT_KEX     "diffie-hellman-group-exchange-sha1," \
                            "diffie-hellman-group14-sha1," \
                            "diffie-hellman-group1-sha1"
#define KEX_DEFAULT_PK_ALG  "ssh-rsa,ssh-dss"
// use the setting of pvar.CipherOrder.
#define KEX_DEFAULT_ENCRYPT ""
#define KEX_DEFAULT_MAC     "hmac-sha1,hmac-md5"
// support of "Compression delayed" (2006.6.23 maya)
#define KEX_DEFAULT_COMP	"none,zlib@openssh.com,zlib"
#define KEX_DEFAULT_LANG	""

/* Minimum modulus size (n) for RSA keys. */
#define SSH_RSA_MINIMUM_MODULUS_SIZE    768

#define SSH_KEYGEN_DEFAULT_BITS   2048
#define SSH_RSA_MINIMUM_KEY_SIZE   768
#define SSH_DSA_MINIMUM_KEY_SIZE  1024

enum kex_init_proposals {
	PROPOSAL_KEX_ALGS,
	PROPOSAL_SERVER_HOST_KEY_ALGS,
	PROPOSAL_ENC_ALGS_CTOS,
	PROPOSAL_ENC_ALGS_STOC,
	PROPOSAL_MAC_ALGS_CTOS,
	PROPOSAL_MAC_ALGS_STOC,
	PROPOSAL_COMP_ALGS_CTOS,
	PROPOSAL_COMP_ALGS_STOC,
	PROPOSAL_LANG_CTOS,
	PROPOSAL_LANG_STOC,
	PROPOSAL_MAX
};


// クライアントからサーバへの提案事項
#ifdef SSH2_DEBUG
static char *myproposal[PROPOSAL_MAX] = {
//	KEX_DEFAULT_KEX,
	"diffie-hellman-group14-sha1,diffie-hellman-group1-sha1,diffie-hellman-group-exchange-sha1",
	KEX_DEFAULT_PK_ALG,
//	"ssh-dss,ssh-rsa",
	KEX_DEFAULT_ENCRYPT,
	KEX_DEFAULT_ENCRYPT,
	"hmac-md5,hmac-sha1",
	"hmac-md5,hmac-sha1",
//	"hmac-sha1",
//	"hmac-sha1",
//	KEX_DEFAULT_MAC,
//	KEX_DEFAULT_MAC,
	KEX_DEFAULT_COMP,
	KEX_DEFAULT_COMP,
	KEX_DEFAULT_LANG,
	KEX_DEFAULT_LANG,
};
#else
static char *myproposal[PROPOSAL_MAX] = {
	KEX_DEFAULT_KEX,
	KEX_DEFAULT_PK_ALG,
	KEX_DEFAULT_ENCRYPT,
	KEX_DEFAULT_ENCRYPT,
	KEX_DEFAULT_MAC,
	KEX_DEFAULT_MAC,
	KEX_DEFAULT_COMP,
	KEX_DEFAULT_COMP,
	KEX_DEFAULT_LANG,
	KEX_DEFAULT_LANG,
};
#endif


typedef struct ssh2_cipher {
	SSHCipher cipher;
	char *name;
	int block_size;
	int key_len;
	int discard_len;
	const EVP_CIPHER *(*func)(void);
} ssh2_cipher_t;

static ssh2_cipher_t ssh2_ciphers[] = {
	{SSH2_CIPHER_3DES_CBC,     "3des-cbc",      8, 24, 0, EVP_des_ede3_cbc},
	{SSH2_CIPHER_AES128_CBC,   "aes128-cbc",   16, 16, 0, EVP_aes_128_cbc},
	{SSH2_CIPHER_AES192_CBC,   "aes192-cbc",   16, 24, 0, EVP_aes_192_cbc},
	{SSH2_CIPHER_AES256_CBC,   "aes256-cbc",   16, 32, 0, EVP_aes_256_cbc},
	{SSH2_CIPHER_BLOWFISH_CBC, "blowfish-cbc",  8, 16, 0, EVP_bf_cbc},
	{SSH2_CIPHER_AES128_CTR,   "aes128-ctr",   16, 16, 0, evp_aes_128_ctr},
	{SSH2_CIPHER_AES192_CTR,   "aes192-ctr",   16, 24, 0, evp_aes_128_ctr},
	{SSH2_CIPHER_AES256_CTR,   "aes256-ctr",   16, 32, 0, evp_aes_128_ctr},
	{SSH2_CIPHER_ARCFOUR,      "arcfour",       8, 16, 0, EVP_rc4},
	{SSH2_CIPHER_ARCFOUR128,   "arcfour128",    8, 16, 1536, EVP_rc4},
	{SSH2_CIPHER_ARCFOUR256,   "arcfour256",    8, 32, 1536, EVP_rc4},
	{SSH2_CIPHER_CAST128_CBC,  "cast128-cbc",   8, 16, 0, EVP_cast5_cbc},
	{SSH_CIPHER_NONE, NULL, 0, 0, 0, NULL},
};


typedef struct ssh2_mac {
	char *name;
	const EVP_MD *(*func)(void);
	int truncatebits;
} ssh2_mac_t;

static ssh2_mac_t ssh2_macs[] = {
	{"hmac-sha1", EVP_sha1, 0},
	{"hmac-md5", EVP_md5, 0},
	{NULL, NULL, 0},
};

static char *ssh_comp[] = {
	"none",
	"zlib",
	"zlib@openssh.com",
};


struct Enc {
	u_char          *key;
	u_char          *iv;
	unsigned int    key_len;
	unsigned int    block_size;
};

struct Mac {
	char            *name; 
	int             enabled; 
	const EVP_MD    *md;
	int             mac_len; 
	u_char          *key;
	int             key_len;
};

struct Comp {
	int     type;
	int     enabled;
	char    *name;
};

typedef struct {
	struct Enc  enc;
	struct Mac  mac;
	struct Comp comp;
} Newkeys;

#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))

enum kex_modes {
	MODE_IN,
	MODE_OUT,
	MODE_MAX
};


// ホストキー(SSH1, SSH2含む)のデータ構造 (2006.3.21 yutaka)
typedef struct Key {
	// host key type
	enum hostkey_type type;
	// SSH2 RSA
	RSA *rsa;
	// SSH2 DSA
	DSA *dsa;
	// SSH1 RSA
	int bits;
	unsigned char *exp;
	unsigned char *mod;
} Key;

// fingerprintの種別
enum fp_rep {
	SSH_FP_HEX,
	SSH_FP_BUBBLEBABBLE,
	SSH_FP_RANDOMART
};

enum scp_dir {
	TOREMOTE, FROMREMOTE,
};

/* The packet handler returns TRUE to keep the handler in place,
   FALSE to remove the handler. */
typedef BOOL (* SSHPacketHandler)(PTInstVar pvar);

typedef struct _SSHPacketHandlerItem SSHPacketHandlerItem;
struct _SSHPacketHandlerItem {
	SSHPacketHandler handler;
	/* Circular list of handlers for given message */
	SSHPacketHandlerItem FAR * next_for_message;
	SSHPacketHandlerItem FAR * last_for_message;
	/* Circular list of handlers in set */
	SSHPacketHandlerItem FAR * next_in_set;
	int active_for_message;
};

typedef struct {
	char FAR * hostname;

	int server_protocol_flags;
	char FAR * server_ID;

	/* This buffer is used to hold the outgoing data, and encrypted in-place
	   here if necessary. */
	unsigned char FAR * outbuf;
	long outbuflen;
	/* This buffer is used by the SSH protocol processing to store uncompressed
	   packet data for compression. User data is never streamed through here;
	   it is compressed directly from the user's buffer. */
	unsigned char FAR * precompress_outbuf;
	long precompress_outbuflen;
	/* this is the length of the packet data, including the type header */
	long outgoing_packet_len;

	/* This buffer is used by the SSH protocol processing to store decompressed
	   packet data. User data is never streamed through here; it is decompressed
	   directly to the user's buffer. */
	unsigned char FAR * postdecompress_inbuf;
	long postdecompress_inbuflen;

	unsigned char FAR * payload;
	long payload_grabbed;
	long payloadlen;
	long payload_datastart;
	long payload_datalen;

	uint32 receiver_sequence_number;
	uint32 sender_sequence_number;

	z_stream compress_stream;
	z_stream decompress_stream;
	BOOL compressing;
	BOOL decompressing;
	int compression_level;

	SSHPacketHandlerItem FAR * packet_handlers[256];
	int status_flags;

	int win_cols;
	int win_rows;

	unsigned short tcpport;
} SSHState;

#define STATUS_DONT_SEND_USER_NAME            0x01
#define STATUS_EXPECTING_COMPRESSION_RESPONSE 0x02
#define STATUS_DONT_SEND_CREDENTIALS          0x04
#define STATUS_HOST_OK                        0x08
#define STATUS_INTERACTIVE                    0x10
#define STATUS_IN_PARTIAL_ID_STRING           0x20

void SSH_init(PTInstVar pvar);
void SSH_open(PTInstVar pvar);
void SSH_notify_disconnecting(PTInstVar pvar, char FAR * reason);
/* SSH_handle_server_ID returns TRUE iff a valid ID string has been
   received. If it returns FALSE, we need to keep looking for another
   ID string. */
BOOL SSH_handle_server_ID(PTInstVar pvar, char FAR * ID, int ID_len);
/* SSH_handle_packet requires NO PAYLOAD on entry.
   'len' is the size of the packet: payload + padding (+ CRC for SSHv1)
   'padding' is the size of the padding.
   'data' points to the start of the packet data (the length field)
*/
void SSH_handle_packet(PTInstVar pvar, char FAR * data, int len, int padding);
void SSH_notify_win_size(PTInstVar pvar, int cols, int rows);
void SSH_notify_user_name(PTInstVar pvar);
void SSH_notify_cred(PTInstVar pvar);
void SSH_notify_host_OK(PTInstVar pvar);
void SSH_send(PTInstVar pvar, unsigned char const FAR * buf, unsigned int buflen);
/* SSH_extract_payload returns number of bytes extracted */
int SSH_extract_payload(PTInstVar pvar, unsigned char FAR * dest, int len);
void SSH_end(PTInstVar pvar);

void SSH_get_server_ID_info(PTInstVar pvar, char FAR * dest, int len);
void SSH_get_protocol_version_info(PTInstVar pvar, char FAR * dest, int len);
void SSH_get_compression_info(PTInstVar pvar, char FAR * dest, int len);

/* len must be <= SSH_MAX_SEND_PACKET_SIZE */
void SSH_channel_send(PTInstVar pvar, int channel_num,
                      uint32 remote_channel_num,
                      unsigned char FAR * buf, int len);
void SSH_fail_channel_open(PTInstVar pvar, uint32 remote_channel_num);
void SSH_confirm_channel_open(PTInstVar pvar, uint32 remote_channel_num, uint32 local_channel_num);
void SSH_channel_output_eof(PTInstVar pvar, uint32 remote_channel_num);
void SSH_channel_input_eof(PTInstVar pvar, uint32 remote_channel_num, uint32 local_channel_num);
void SSH_request_forwarding(PTInstVar pvar, int from_server_port,
  char FAR * to_local_host, int to_local_port);
void SSH_request_X11_forwarding(PTInstVar pvar,
  char FAR * auth_protocol, unsigned char FAR * auth_data, int auth_data_len, int screen_num);
void SSH_open_channel(PTInstVar pvar, uint32 local_channel_num,
                      char FAR * to_remote_host, int to_remote_port,
                      char FAR * originator, unsigned short originator_port);

int SSH_start_scp(PTInstVar pvar, char *sendfile, char *dstfile);
int SSH_start_scp_receive(PTInstVar pvar, char *filename);
int SSH_scp_transaction(PTInstVar pvar, char *sendfile, char *dstfile, enum scp_dir direction);
int SSH_sftp_transaction(PTInstVar pvar);

/* auxiliary SSH2 interfaces for pkt.c */
int SSH_get_min_packet_size(PTInstVar pvar);
/* data is guaranteed to be at least SSH_get_min_packet_size bytes long
   at least 5 bytes must be decrypted */
void SSH_predecrpyt_packet(PTInstVar pvar, char FAR * data);
int SSH_get_clear_MAC_size(PTInstVar pvar);

#define SSH_is_any_payload(pvar) ((pvar)->ssh_state.payload_datalen > 0)
#define SSH_get_host_name(pvar) ((pvar)->ssh_state.hostname)
#define SSH_get_compression_level(pvar) ((pvar)->ssh_state.compressing ? (pvar)->ts_SSH_CompressionLevel : 0)

void SSH2_send_kexinit(PTInstVar pvar);
BOOL do_SSH2_userauth(PTInstVar pvar);
BOOL do_SSH2_authrequest(PTInstVar pvar);
void debug_print(int no, char *msg, int len);
int get_cipher_block_size(SSHCipher cipher);
int get_cipher_key_len(SSHCipher cipher);
const EVP_CIPHER* get_cipher_EVP_CIPHER(SSHCipher cipher);
int get_cipher_discard_len(SSHCipher cipher);
void ssh_heartbeat_lock_initialize(void);
void ssh_heartbeat_lock_finalize(void);
void ssh_heartbeat_lock(void);
void ssh_heartbeat_unlock(void);
void halt_ssh_heartbeat_thread(PTInstVar pvar);
void ssh2_channel_free(void);
BOOL handle_SSH2_userauth_inforeq(PTInstVar pvar);
void SSH2_update_compression_myproposal(PTInstVar pvar);
void SSH2_update_cipher_myproposal(PTInstVar pvar);

enum hostkey_type get_keytype_from_name(char *name);
char *get_sshname_from_key(Key *key);
int key_to_blob(Key *key, char **blobp, int *lenp);
Key *key_from_blob(char *data, int blen);
void key_free(Key *key);
RSA *duplicate_RSA(RSA *src);
DSA *duplicate_DSA(DSA *src);
char *key_fingerprint(Key *key, enum fp_rep dgst_rep);

#endif
