/*
(C) 2011 TeraTerm Project
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

#ifndef __TTSSH_DNS_H
#define __TTSSH_DNS_H

#include <windns.h>

#define DNS_TYPE_SSHFP	44

enum sshfp_types {
        SSHFP_KEY_RESERVED = 0,  // RFC4255
        SSHFP_KEY_RSA = 1,       // RFC4255
        SSHFP_KEY_DSA = 2,       // RFC4255
        SSHFP_KEY_ECDSA = 3,     // RFC6594
        SSHFP_KEY_ED25519 = 4    // RFC7479
};

enum sshfp_hashes {
        SSHFP_HASH_RESERVED = 0, // RFC4255
        SSHFP_HASH_SHA1 = 1,     // RFC4255
        SSHFP_HASH_SHA256 = 2    // RFC6594
};

enum verifydns_result {
	DNS_VERIFY_NONE,
	DNS_VERIFY_NOTFOUND,
	DNS_VERIFY_MATCH,
	DNS_VERIFY_MISMATCH,
	DNS_VERIFY_DIFFERENTTYPE,
	DNS_VERIFY_AUTH_MATCH,
	DNS_VERIFY_AUTH_MISMATCH,
	DNS_VERIFY_AUTH_DIFFERENTTYPE
};

typedef struct {
	BYTE Algorithm;
	BYTE DigestType;
	BYTE Digest[1];
} DNS_SSHFP_DATA, *PDNS_SSHFP_DATA;

int is_numeric_hostname(const char *hostname);
int verify_hostkey_dns(PTInstVar pvar, char *hostname, Key *key);

#endif //  __TTSSH_DNS_H
