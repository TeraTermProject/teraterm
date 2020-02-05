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

#ifndef __SSH_H
#define __SSH_H

#include "zlib.h"
#include <openssl/evp.h>

#include "buffer.h"
#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>

#define DEBUG_PRINT_TO_FILE(base, msg, len) { \
	static int count = 0; \
	debug_print(count + base, msg, len); \
	count++; \
}

// from OpenSSH
extern const EVP_CIPHER *evp_aes_128_ctr(void);
extern const EVP_CIPHER *evp_des3_ctr(void);
extern const EVP_CIPHER *evp_bf_ctr(void);
extern const EVP_CIPHER *evp_cast5_ctr(void);
extern const EVP_CIPHER *evp_camellia_128_ctr(void);

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
	// SSH1
	SSH_CIPHER_NONE, SSH_CIPHER_IDEA, SSH_CIPHER_DES, SSH_CIPHER_3DES,
	SSH_CIPHER_TSS, SSH_CIPHER_RC4, SSH_CIPHER_BLOWFISH,
	// SSH2
	SSH2_CIPHER_3DES_CBC, SSH2_CIPHER_AES128_CBC,
	SSH2_CIPHER_AES192_CBC, SSH2_CIPHER_AES256_CBC,
	SSH2_CIPHER_BLOWFISH_CBC, SSH2_CIPHER_AES128_CTR,
	SSH2_CIPHER_AES192_CTR, SSH2_CIPHER_AES256_CTR,
	SSH2_CIPHER_ARCFOUR, SSH2_CIPHER_ARCFOUR128, SSH2_CIPHER_ARCFOUR256,
	SSH2_CIPHER_CAST128_CBC,
	SSH2_CIPHER_3DES_CTR, SSH2_CIPHER_BLOWFISH_CTR, SSH2_CIPHER_CAST128_CTR,
	SSH2_CIPHER_CAMELLIA128_CBC, SSH2_CIPHER_CAMELLIA192_CBC, SSH2_CIPHER_CAMELLIA256_CBC,
	SSH2_CIPHER_CAMELLIA128_CTR, SSH2_CIPHER_CAMELLIA192_CTR, SSH2_CIPHER_CAMELLIA256_CTR,
	SSH2_CIPHER_AES128_GCM, SSH2_CIPHER_AES256_GCM,
	SSH_CIPHER_MAX = SSH2_CIPHER_AES256_GCM,
} SSHCipherId;

typedef enum {
	SSH_AUTH_NONE, SSH_AUTH_RHOSTS, SSH_AUTH_RSA, SSH_AUTH_PASSWORD,
	SSH_AUTH_RHOSTS_RSA, SSH_AUTH_TIS, SSH_AUTH_KERBEROS,
	SSH_AUTH_PAGEANT = 16,
	SSH_AUTH_MAX = SSH_AUTH_PAGEANT,
} SSHAuthMethod;

typedef enum {
	SSH_GENERIC_AUTHENTICATION, SSH_TIS_AUTHENTICATION
} SSHAuthMode;

#define SSH_PROTOFLAG_SCREEN_NUMBER 1
#define SSH_PROTOFLAG_HOST_IN_FWD_OPEN 2

enum channel_type {
	TYPE_SHELL, TYPE_PORTFWD, TYPE_SCP, TYPE_SFTP, TYPE_AGENT, TYPE_SUBSYSTEM_GEN,
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
#define SSH_CHANNEL_INVALID -1

/* SSH2 messages */
typedef enum {
	// Transport layer protocol
	// 1..19 Transport layer generic (RFC 4253)
	SSH2_MSG_DISCONNECT      = 1,
	SSH2_MSG_IGNORE          = 2,
	SSH2_MSG_UNIMPLEMENTED   = 3,
	SSH2_MSG_DEBUG           = 4,
	SSH2_MSG_SERVICE_REQUEST = 5,
	SSH2_MSG_SERVICE_ACCEPT  = 6,

	// 20..29 Algorithm negotiation (RFC 4253)
	SSH2_MSG_KEXINIT = 20,
	SSH2_MSG_NEWKEYS = 21,

	// 30..49 Key excahnge method specific
	// Diffie-Hellman Key Exchange (RFC 4253)
	SSH2_MSG_KEXDH_INIT  = 30,
	SSH2_MSG_KEXDH_REPLY = 31,

	// RFC 4419 - Diffie-Hellman Group Exchange for the Secure Shell (SSH) Transport Layer Protocol
	SSH2_MSG_KEX_DH_GEX_GROUP   = 31,
	SSH2_MSG_KEX_DH_GEX_INIT    = 32,
	SSH2_MSG_KEX_DH_GEX_REPLY   = 33,
	SSH2_MSG_KEX_DH_GEX_REQUEST = 34,

	// RFC 5656 - Elliptic Curve Algorithm Integration in the Secure Shell Transport Layer
	SSH2_MSG_KEX_ECDH_INIT  = 30,
	SSH2_MSG_KEX_ECDH_REPLY = 31,

	// User authentication protocol
	// 50..59 User authentication generic (RFC 4252)
	SSH2_MSG_USERAUTH_REQUEST = 50,
	SSH2_MSG_USERAUTH_FAILURE = 51,
	SSH2_MSG_USERAUTH_SUCCESS = 52,
	SSH2_MSG_USERAUTH_BANNER  = 53,

	// Public key authentication (RFC 4252)
	SSH2_MSG_USERAUTH_PK_OK = 60,

	// Password authentication (RFC 4252)
	SSH2_MSG_USERAUTH_PASSWD_CHANGEREQ = 60,

	// RFC 4256 - Generic Message Exchange Authentication for the Secure Shell Protocol (SSH)
	// Keyboard-interactive authentication
	SSH2_MSG_USERAUTH_INFO_REQUEST  = 60,
	SSH2_MSG_USERAUTH_INFO_RESPONSE = 61,

	// Connection protocol
	// 80..89 Connection protocol generic (RFC 4254)
	SSH2_MSG_GLOBAL_REQUEST            = 80,
	SSH2_MSG_REQUEST_SUCCESS           = 81,
	SSH2_MSG_REQUEST_FAILURE           = 82,

	// 90..127 Channel related messages (RFC 4254)
	SSH2_MSG_CHANNEL_OPEN              = 90,
	SSH2_MSG_CHANNEL_OPEN_CONFIRMATION = 91,
	SSH2_MSG_CHANNEL_OPEN_FAILURE      = 92,
	SSH2_MSG_CHANNEL_WINDOW_ADJUST     = 93,
	SSH2_MSG_CHANNEL_DATA              = 94,
	SSH2_MSG_CHANNEL_EXTENDED_DATA     = 95,
	SSH2_MSG_CHANNEL_EOF               = 96,
	SSH2_MSG_CHANNEL_CLOSE             = 97,
	SSH2_MSG_CHANNEL_REQUEST           = 98,
	SSH2_MSG_CHANNEL_SUCCESS           = 99,
	SSH2_MSG_CHANNEL_FAILURE           = 100

	// Reserved for client protocols
	// 128..191 Reserved

	// Local extensions:
	// 192..255 Local extensions

} SSH2Message;

/* SSH2 miscellaneous constants */
typedef enum {
	SSH2_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT = 1,
	SSH2_DISCONNECT_PROTOCOL_ERROR,
	SSH2_DISCONNECT_KEY_EXCHANGE_FAILED,
	SSH2_DISCONNECT_HOST_AUTHENTICATION_FAILED,
	SSH2_DISCONNECT_MAC_ERROR,
	SSH2_DISCONNECT_COMPRESSION_ERROR,
	SSH2_DISCONNECT_SERVICE_NOT_AVAILABLE,
	SSH2_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED,
	SSH2_DISCONNECT_HOST_KEY_NOT_VERIFIABLE,
	SSH2_DISCONNECT_CONNECTION_LOST,
	SSH2_DISCONNECT_BY_APPLICATION,
	SSH2_DISCONNECT_TOO_MANY_CONNECTIONS,
	SSH2_DISCONNECT_AUTH_CANCELLED_BY_USER,
	SSH2_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE,
	SSH2_DISCONNECT_ILLEGAL_USER_NAME
} SSH2DisconnectMessage;

typedef enum {
	SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED = 1,
	SSH2_OPEN_CONNECT_FAILED,
	SSH2_OPEN_UNKNOWN_CHANNEL_TYPE,
	SSH2_OPEN_RESOURCE_SHORTAGE
} SSH2ChannelFailReason;

// Terminal Modes
typedef enum {
	SSH2_TTY_OP_END       =   0,
	SSH2_TTY_KEY_VINTR    =   1,
	SSH2_TTY_KEY_VQUIT    =   2,
	SSH2_TTY_KEY_VERASE   =   3,
	SSH2_TTY_KEY_VKILL    =   4,
	SSH2_TTY_KEY_VEOF     =   5,
	SSH2_TTY_KEY_VEOL     =   6,
	SSH2_TTY_KEY_VEOL2    =   7,
	SSH2_TTY_KEY_VSTART   =   8,
	SSH2_TTY_KEY_VSTOP    =   9,
	SSH2_TTY_KEY_VSUSP    =  10,
	SSH2_TTY_KEY_VDSUSP   =  11,
	SSH2_TTY_KEY_VREPRINT =  12,
	SSH2_TTY_KEY_VWERASE  =  13,
	SSH2_TTY_KEY_VLNEXT   =  14,
	SSH2_TTY_KEY_VFLUSH   =  15,
	SSH2_TTY_KEY_VSWTCH   =  16,
	SSH2_TTY_KEY_VSTATUS  =  17,
	SSH2_TTY_KEY_VDISCARD =  18,
	SSH2_TTY_OP_IGNPAR    =  30,
	SSH2_TTY_OP_PARMRK    =  31,
	SSH2_TTY_OP_INPCK     =  32,
	SSH2_TTY_OP_ISTRIP    =  33,
	SSH2_TTY_OP_INLCR     =  34,
	SSH2_TTY_OP_IGNCR     =  35,
	SSH2_TTY_OP_ICRNL     =  36,
	SSH2_TTY_OP_IUCLC     =  37,
	SSH2_TTY_OP_IXON      =  38,
	SSH2_TTY_OP_IXANY     =  39,
	SSH2_TTY_OP_IXOFF     =  40,
	SSH2_TTY_OP_IMAXBEL   =  41,
	SSH2_TTY_OP_ISIG      =  50,
	SSH2_TTY_OP_ICANON    =  51,
	SSH2_TTY_OP_XCASE     =  52,
	SSH2_TTY_OP_ECHO      =  53,
	SSH2_TTY_OP_ECHOE     =  54,
	SSH2_TTY_OP_ECHOK     =  55,
	SSH2_TTY_OP_ECHONL    =  56,
	SSH2_TTY_OP_NOFLSH    =  57,
	SSH2_TTY_OP_TOSTOP    =  58,
	SSH2_TTY_OP_IEXTEN    =  59,
	SSH2_TTY_OP_ECHOCTL   =  60,
	SSH2_TTY_OP_ECHOKE    =  61,
	SSH2_TTY_OP_PENDIN    =  62,
	SSH2_TTY_OP_OPOST     =  70,
	SSH2_TTY_OP_OLCUC     =  71,
	SSH2_TTY_OP_ONLCR     =  72,
	SSH2_TTY_OP_OCRNL     =  73,
	SSH2_TTY_OP_ONOCR     =  74,
	SSH2_TTY_OP_ONLRET    =  75,
	SSH2_TTY_OP_CS7       =  90,
	SSH2_TTY_OP_CS8       =  91,
	SSH2_TTY_OP_PARENB    =  92,
	SSH2_TTY_OP_PARODD    =  93,
	SSH2_TTY_OP_ISPEED    = 128,
	SSH2_TTY_OP_OSPEED    = 129
} SSH2TTYMode;


// クライアントからサーバへの提案事項
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

typedef enum {
	KEY_NONE,
	KEY_RSA1,
	KEY_RSA,
	KEY_DSA,
	KEY_ECDSA256,
	KEY_ECDSA384,
	KEY_ECDSA521,
	KEY_ED25519,
	KEY_UNSPEC,
	KEY_MAX = KEY_UNSPEC,
} ssh_keytype;
#define isFixedLengthKey(type)	((type) >= KEY_DSA && (type) <= KEY_ED25519)

/* Minimum modulus size (n) for RSA keys. */
#define SSH_RSA_MINIMUM_MODULUS_SIZE    768

#define SSH_KEYGEN_DEFAULT_BITS   2048
#define SSH_RSA_MINIMUM_KEY_SIZE   768
#define SSH_DSA_MINIMUM_KEY_SIZE  1024

#define SSH_KEYGEN_MINIMUM_ROUNDS       1
#define SSH_KEYGEN_MAXIMUM_ROUNDS INT_MAX


typedef struct ssh2_cipher {
	SSHCipherId id;
	char *name;
	int block_size;
	int key_len;
	int discard_len;
	int iv_len;
	int auth_len;
	const EVP_CIPHER *(*func)(void);
} SSH2Cipher;

typedef enum {
	KEX_DH_NONE,       /* disabled line */
	KEX_DH_GRP1_SHA1,
	KEX_DH_GRP14_SHA1,
	KEX_DH_GEX_SHA1,
	KEX_DH_GEX_SHA256,
	KEX_ECDH_SHA2_256,
	KEX_ECDH_SHA2_384,
	KEX_ECDH_SHA2_521,
	KEX_DH_GRP14_SHA256,
	KEX_DH_GRP16_SHA512,
	KEX_DH_GRP18_SHA512,
	KEX_DH_UNKNOWN,
	KEX_DH_MAX = KEX_DH_UNKNOWN,
} kex_algorithm;

typedef enum {
	HMAC_NONE,      /* disabled line */
	HMAC_SHA1,
	HMAC_MD5,
	HMAC_SHA1_96,
	HMAC_MD5_96,
	HMAC_RIPEMD160,
	HMAC_SHA2_256,
	HMAC_SHA2_256_96,
	HMAC_SHA2_512,
	HMAC_SHA2_512_96,
	HMAC_SHA1_EtM,
	HMAC_MD5_EtM,
	HMAC_SHA1_96_EtM,
	HMAC_MD5_96_EtM,
	HMAC_RIPEMD160_EtM,
	HMAC_SHA2_256_EtM,
	HMAC_SHA2_512_EtM,
	HMAC_IMPLICIT,
	HMAC_UNKNOWN,
	HMAC_MAX = HMAC_UNKNOWN,
} SSH2MacId;

typedef struct ssh2_mac {
	SSH2MacId id;
	char *name;
	const EVP_MD *(*evp_md)(void);
	int truncatebits;
	int etm;
} SSH2Mac;

typedef enum {
	COMP_NONE,      /* disabled line */
	COMP_NOCOMP,
	COMP_ZLIB,
	COMP_DELAYED,
	COMP_UNKNOWN,
	COMP_MAX = COMP_UNKNOWN,
} compression_type;

struct Enc {
	u_char          *key;
	u_char          *iv;
	unsigned int    key_len;
	unsigned int    block_size;
	unsigned int    iv_len;
	unsigned int    auth_len;
};

struct Mac {
	char            *name;
	int             enabled;
	const EVP_MD    *md;
	unsigned int    mac_len;
	u_char          *key;
	unsigned int    key_len;
	int             etm;
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
} SSHKeys;

#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))

enum kex_modes {
	MODE_IN,
	MODE_OUT,
	MODE_MAX
};


// ホストキー(SSH1, SSH2含む)のデータ構造 (2006.3.21 yutaka)
typedef struct Key {
	// host key type
	ssh_keytype type;
	// SSH2 RSA
	RSA *rsa;
	// SSH2 DSA
	DSA *dsa;
	// SSH2 ECDSA
	EC_KEY *ecdsa;
	// SSH1 RSA
	int bits;
	unsigned char *exp;
	unsigned char *mod;
	// SSH2 ED25519
	unsigned char *ed25519_sk;
	unsigned char *ed25519_pk;
	int bcrypt_kdf;
} Key;

// fingerprintの種別
enum fp_rep {
	SSH_FP_DEFAULT = 0,
	SSH_FP_HEX,
	SSH_FP_BASE64,
	SSH_FP_BUBBLEBABBLE,
	SSH_FP_RANDOMART
};
/*
enum fp_type {
	SSH_FP_MD5,
	SSH_FP_SHA1,
	SSH_FP_SHA256
};
*/
typedef enum {
	SSH_DIGEST_MD5,
	SSH_DIGEST_RIPEMD160,
	SSH_DIGEST_SHA1,
	SSH_DIGEST_SHA256,
	SSH_DIGEST_SHA384,
	SSH_DIGEST_SHA512,
	SSH_DIGEST_MAX,
} digest_algorithm;

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
	SSHPacketHandlerItem *next_for_message;
	SSHPacketHandlerItem *last_for_message;
	/* Circular list of handlers in set */
	SSHPacketHandlerItem *next_in_set;
	int active_for_message;
};

typedef struct {
	char *hostname;

	int server_protocol_flags;
	char *server_ID;

	/* This buffer is used to hold the outgoing data, and encrypted in-place
	   here if necessary. */
	unsigned char *outbuf;
	long outbuflen;
	/* This buffer is used by the SSH protocol processing to store uncompressed
	   packet data for compression. User data is never streamed through here;
	   it is compressed directly from the user's buffer. */
	unsigned char *precompress_outbuf;
	long precompress_outbuflen;
	/* this is the length of the packet data, including the type header */
	long outgoing_packet_len;

	/* This buffer is used by the SSH protocol processing to store decompressed
	   packet data. User data is never streamed through here; it is decompressed
	   directly to the user's buffer. */
	unsigned char *postdecompress_inbuf;
	long postdecompress_inbuflen;

	unsigned char *payload;
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

	SSHPacketHandlerItem *packet_handlers[256];
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
void SSH_notify_disconnecting(PTInstVar pvar, char *reason);
/* SSH_handle_server_ID returns TRUE iff a valid ID string has been
   received. If it returns FALSE, we need to keep looking for another
   ID string. */
BOOL SSH_handle_server_ID(PTInstVar pvar, char *ID, int ID_len);
/* SSH_handle_packet requires NO PAYLOAD on entry.
   'len' is the size of the packet: payload + padding (+ CRC for SSHv1)
   'padding' is the size of the padding.
   'data' points to the start of the packet data (the length field)
*/
void SSH1_handle_packet(PTInstVar pvar, char *data, unsigned int len, unsigned int padding);
void SSH2_handle_packet(PTInstVar pvar, char *data, unsigned int len, unsigned int aadlen, unsigned int authlen);
void SSH_notify_win_size(PTInstVar pvar, int cols, int rows);
void SSH_notify_user_name(PTInstVar pvar);
void SSH_notify_cred(PTInstVar pvar);
void SSH_notify_host_OK(PTInstVar pvar);
void SSH_send(PTInstVar pvar, unsigned char const *buf, unsigned int buflen);
/* SSH_extract_payload returns number of bytes extracted */
int SSH_extract_payload(PTInstVar pvar, unsigned char *dest, int len);
void SSH_end(PTInstVar pvar);

void SSH_get_server_ID_info(PTInstVar pvar, char *dest, int len);
void SSH_get_protocol_version_info(PTInstVar pvar, char *dest, int len);
void SSH_get_compression_info(PTInstVar pvar, char *dest, int len);
void SSH_get_mac_info(PTInstVar pvar, char *dest, int len);

/* len must be <= SSH_MAX_SEND_PACKET_SIZE */
void SSH_channel_send(PTInstVar pvar, int channel_num,
                      uint32 remote_channel_num,
                      unsigned char *buf, int len, int retry);
void SSH_fail_channel_open(PTInstVar pvar, uint32 remote_channel_num);
void SSH_confirm_channel_open(PTInstVar pvar, uint32 remote_channel_num, uint32 local_channel_num);
void SSH_channel_output_eof(PTInstVar pvar, uint32 remote_channel_num);
void SSH_channel_input_eof(PTInstVar pvar, uint32 remote_channel_num, uint32 local_channel_num);
void SSH_request_forwarding(PTInstVar pvar, char *bind_address, int from_server_port,
                            char *to_local_host, int to_local_port);
void SSH_cancel_request_forwarding(PTInstVar pvar, char *bind_address, int from_server_port, int reply);
void SSH_request_X11_forwarding(PTInstVar pvar,
  char *auth_protocol, unsigned char *auth_data, int auth_data_len, int screen_num);
void SSH_open_channel(PTInstVar pvar, uint32 local_channel_num,
                      char *to_remote_host, int to_remote_port,
                      char *originator, unsigned short originator_port);

int SSH_start_scp(PTInstVar pvar, char *sendfile, char *dstfile);
int SSH_start_scp_receive(PTInstVar pvar, char *filename);
int SSH_scp_transaction(PTInstVar pvar, char *sendfile, char *dstfile, enum scp_dir direction);
int SSH_sftp_transaction(PTInstVar pvar);

/* auxiliary SSH2 interfaces for pkt.c */
unsigned int SSH_get_min_packet_size(PTInstVar pvar);
/* data is guaranteed to be at least SSH_get_min_packet_size bytes long
   at least 5 bytes must be decrypted */
void SSH_predecrypt_packet(PTInstVar pvar, char *data);
unsigned int SSH_get_clear_MAC_size(PTInstVar pvar);
unsigned int SSH_get_authdata_size(PTInstVar pvar, int direction);

#define SSH_is_any_payload(pvar) ((pvar)->ssh_state.payload_datalen > 0)
#define SSH_get_host_name(pvar) ((pvar)->ssh_state.hostname)
#define SSH_get_compression_level(pvar) ((pvar)->ssh_state.compressing ? (pvar)->ts_SSH_CompressionLevel : 0)

void SSH2_send_kexinit(PTInstVar pvar);
BOOL do_SSH2_userauth(PTInstVar pvar);
BOOL do_SSH2_authrequest(PTInstVar pvar);
void debug_print(int no, char *msg, int len);
int get_cipher_block_size(const SSH2Cipher *cipher);
int get_cipher_key_len(const SSH2Cipher *cipher);
int get_cipher_iv_len(const SSH2Cipher *cipher);
int get_cipher_auth_len(const SSH2Cipher *cipher);
const SSH2Cipher *get_cipher_by_name(char *name);
char* get_kex_algorithm_name(kex_algorithm kextype);
const EVP_CIPHER* get_cipher_EVP_CIPHER(const SSH2Cipher *cipher);
const EVP_MD* get_kex_algorithm_EVP_MD(kex_algorithm kextype);
const SSH2Mac *get_ssh2_mac(SSH2MacId id);
char* get_ssh2_mac_name(const SSH2Mac *mac);
const char* get_ssh2_mac_name_by_id(SSH2MacId id);
const EVP_MD* get_ssh2_mac_EVP_MD(const SSH2Mac *mac);
int get_ssh2_mac_truncatebits(const SSH2Mac *mac);
char* get_ssh2_comp_name(compression_type type);
char* get_ssh_keytype_name(ssh_keytype type);
char* get_digest_algorithm_name(digest_algorithm id);
int get_cipher_discard_len(const SSH2Cipher *cipher);
void ssh_heartbeat_lock_initialize(void);
void ssh_heartbeat_lock_finalize(void);
void ssh_heartbeat_lock(void);
void ssh_heartbeat_unlock(void);
void halt_ssh_heartbeat_thread(PTInstVar pvar);
void ssh2_channel_free(void);
BOOL handle_SSH2_userauth_msg60(PTInstVar pvar);
BOOL handle_SSH2_userauth_inforeq(PTInstVar pvar);
BOOL handle_SSH2_userauth_pkok(PTInstVar pvar);
BOOL handle_SSH2_userauth_passwd_changereq(PTInstVar pvar);
void SSH2_update_compression_myproposal(PTInstVar pvar);
void SSH2_update_cipher_myproposal(PTInstVar pvar);
void SSH2_update_kex_myproposal(PTInstVar pvar);
void SSH2_update_host_key_myproposal(PTInstVar pvar);
void SSH2_update_hmac_myproposal(PTInstVar pvar);
int SSH_notify_break_signal(PTInstVar pvar);

///
enum scp_state {
	SCP_INIT, SCP_TIMESTAMP, SCP_FILEINFO, SCP_DATA, SCP_CLOSING,
};

typedef struct bufchain {
	buffer_t *msg;
	struct bufchain *next;
} bufchain_t;

typedef struct PacketList {
	char *buf;
	unsigned int buflen;
	struct PacketList *next;
} PacketList_t;

typedef struct scp {
	enum scp_dir dir;              // transfer direction
	enum scp_state state;          // SCP state 
	char localfile[MAX_PATH];      // local filename
	char localfilefull[MAX_PATH];  // local filename fullpath
	char remotefile[MAX_PATH];     // remote filename
	FILE *localfp;                 // file pointer for local file
	struct __stat64 filestat;      // file status information
	HWND progress_window;
	HANDLE thread;
	unsigned int thread_id;
	PTInstVar pvar;
	// for receiving file
	long long filetotalsize;
	long long filercvsize;
	DWORD filemtime;
	DWORD fileatime;
	PacketList_t *pktlist_head;
	PacketList_t *pktlist_tail;
} scp_t;

enum sftp_state {
	SFTP_INIT, SFTP_CONNECTED, SFTP_REALPATH, 
};

typedef struct sftp {
	enum sftp_state state;
	HWND console_window;
	unsigned int transfer_buflen;
	unsigned int num_requests;
	unsigned int version;
	unsigned int msg_id;
#define SFTP_EXT_POSIX_RENAME   0x00000001
#define SFTP_EXT_STATVFS    0x00000002
#define SFTP_EXT_FSTATVFS   0x00000004
#define SFTP_EXT_HARDLINK   0x00000008
	unsigned int exts;
	unsigned long long limit_kbps;
	//struct bwlimit bwlimit_in, bwlimit_out;
	char path[1024];
} sftp_t;

typedef struct channel {
	int used;
	int self_id;
	int remote_id;
	unsigned int local_window;
	unsigned int local_window_max;
	unsigned int local_consumed;
	unsigned int local_maxpacket;
	unsigned int remote_window;
	unsigned int remote_maxpacket;
	enum channel_type type;
	int local_num;
	bufchain_t *bufchain;
	unsigned long bufchain_amount;
	BOOL bufchain_recv_suspended;
	scp_t scp;
	buffer_t *agent_msg;
	int agent_request_len;
	sftp_t sftp;
#define SSH_CHANNEL_STATE_CLOSE_SENT 0x00000001
	unsigned int state;
} Channel_t;

unsigned char *begin_send_packet(PTInstVar pvar, int type, int len);
void finish_send_packet_special(PTInstVar pvar, int skip_compress);
void SSH2_send_channel_data(PTInstVar pvar, Channel_t *c, unsigned char *buf, unsigned int buflen, int retry);
Channel_t* ssh2_local_channel_lookup(int local_num);

#define finish_send_packet(pvar) finish_send_packet_special((pvar), 0)
#define get_payload_uint32(pvar, offset) get_uint32_MSBfirst((pvar)->ssh_state.payload + (offset))
#define get_uint32(buf) get_uint32_MSBfirst((buf))
#define set_uint32(buf, v) set_uint32_MSBfirst((buf), (v))
#define get_mpint_len(pvar, offset) ((get_ushort16_MSBfirst((pvar)->ssh_state.payload + (offset)) + 7) >> 3)
#define get_ushort16(buf) get_ushort16_MSBfirst((buf))
///

/* Global request confirmation callbacks */
typedef void global_confirm_cb(PTInstVar pvar, int type, unsigned int seq, void *ctx);
void client_register_global_confirm(global_confirm_cb *cb, void *ctx);

/* Global request success/failure callbacks */
struct global_confirm {
	global_confirm_cb *cb;
	void *ctx;
	int ref_count;
};

/*  
 * SSH bottom half after known_hosts
 */
enum ssh_kex_known_hosts {
	NONE_KNOWN_HOSTS = 0,
	SSH1_PUBLIC_KEY_KNOWN_HOSTS,
	SSH2_DH_KEX_REPLY_KNOWN_HOSTS,
	SSH2_DH_GEX_REPLY_KNOWN_HOSTS,
	SSH2_ECDH_KEX_REPLY_KNOWN_HOSTS,
};

typedef struct bottom_half_known_hosts {
	enum ssh_kex_known_hosts kex_type;

	unsigned char *payload;
	int payload_len;
	UINT_PTR payload_offset;

	BOOL SSH2_MSG_NEWKEYS_received;	
} bottom_half_known_hosts_t;

void handle_SSH2_canel_reply_after_known_hosts(PTInstVar pvar);

BOOL handle_server_public_key_after_known_hosts(PTInstVar pvar);
BOOL handle_SSH2_dh_kex_reply_after_known_hosts(PTInstVar pvar);
BOOL handle_SSH2_dh_gex_reply_after_known_hosts(PTInstVar pvar);
BOOL handle_SSH2_ecdh_kex_reply_after_known_hosts(PTInstVar pvar);

#endif
