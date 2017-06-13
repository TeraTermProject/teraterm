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

#include <memory.h>

#include "ttxssh.h"
#include "ssh.h"
#include "key.h"
#include "dns.h"

int is_numeric_hostname(const char *hostname)
{
	struct addrinfo hints, *res;

	if (!hostname) {
		return -1;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_flags = AI_NUMERICHOST;

	if (getaddrinfo(hostname, NULL, &hints, &res) == 0) {
		freeaddrinfo(res);
		return 1;
	}

	return 0;
}

int verify_hostkey_dns(PTInstVar pvar, char *hostname, Key *key)
{
	DNS_STATUS status;
	PDNS_RECORD rec, p;
	PDNS_SSHFP_DATA t;
	int hostkey_alg, hostkey_dtype, hostkey_dlen;
	digest_algorithm dgst_alg;
	BYTE *hostkey_digest = NULL;
	int found = DNS_VERIFY_NOTFOUND;

	if (!HasDnsQuery()) {
		// DnsQuery ‚Í Windows 2000 ˆÈã‚Å‚µ‚©“®ì‚µ‚È‚¢‚½‚ß
		return DNS_VERIFY_NONE;
	}

	switch (key->type) {
	case KEY_RSA:
		hostkey_alg = SSHFP_KEY_RSA;
		break;
	case KEY_DSA:
		hostkey_alg = SSHFP_KEY_DSA;
		break;
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
		hostkey_alg = SSHFP_KEY_ECDSA;
		break;
	case KEY_ED25519:
		hostkey_alg = SSHFP_KEY_ED25519;
		break;
	default: // Un-supported algorithm
		hostkey_alg = SSHFP_KEY_RESERVED;
	}
	logprintf(pvar, LOG_LEVEL_VERBOSE, "verify_hostkey_dns: key type = %d, SSHFP type = %d", key->type, hostkey_alg);

	status = DnsQuery(hostname, DNS_TYPE_SSHFP, DNS_QUERY_STANDARD, NULL, &rec, NULL);

	if (status == 0) {
		for (p=rec; p!=NULL; p=p->pNext) {
			if (p->wType == DNS_TYPE_SSHFP) {
				t = (PDNS_SSHFP_DATA)&(p->Data.Null);
				logprintf(pvar, LOG_LEVEL_VERBOSE,
				          "verify_hostkey_dns: SSHFP RR: Algorithm = %d, Digest type = %d",
				          t->Algorithm, t->DigestType);
				if (t->Algorithm == SSHFP_KEY_RESERVED) {
					notify_verbose_message(pvar, "verify_hostkey_dns: Invalid key algorithm (SSHFP_KEY_RESERVED)", LOG_LEVEL_WARNING);
					continue; // skip invalid record
				}
				if (t->Algorithm == hostkey_alg) {
					if (hostkey_digest == NULL || t->DigestType != hostkey_dtype) {
						switch (t->DigestType) {
						case SSHFP_HASH_SHA1:
							if (hostkey_alg != SSHFP_KEY_RSA && hostkey_alg != SSHFP_KEY_DSA) {
								// SHA1 does not allowed to use with ECDSA and ED25519 key
								logprintf(pvar, LOG_LEVEL_VERBOSE,
								          "verify_hostkey_dns: not allowed digest type. "
								          "Algorithm = %d, Digest type = %d",
								          t->Algorithm,
									  t->DigestType);
								dgst_alg = -1;
							}
							else {
								dgst_alg = SSH_DIGEST_SHA1;
							}
							break;
						case SSHFP_HASH_SHA256:
							dgst_alg = SSH_DIGEST_SHA256;
							break;
						default:
							dgst_alg = -1;
						}

						if (dgst_alg == -1)
							continue; // skip invalid/un-supported hash type.

						hostkey_dtype = t->DigestType;
						free(hostkey_digest);
						hostkey_digest = key_fingerprint_raw(key, dgst_alg, &hostkey_dlen);
						if (!hostkey_digest)
							continue;
					}
					if (hostkey_dlen == p->wDataLength-2 && memcmp(hostkey_digest, t->Digest, hostkey_dlen) == 0) {
						found = DNS_VERIFY_MATCH;
						notify_verbose_message(pvar, "verify_hostkey_dns: key matched", LOG_LEVEL_INFO);
					}
					else {
						notify_verbose_message(pvar, "verify_hostkey_dns: key mismatched", LOG_LEVEL_WARNING);
						found = DNS_VERIFY_MISMATCH;
						break;
					}
				}
				else {
					if (found == DNS_VERIFY_NOTFOUND)
						found = DNS_VERIFY_DIFFERENTTYPE;
				}
			}
			else {
				logprintf(pvar, LOG_LEVEL_VERBOSE, "verify_hostkey_dns: not SSHFP RR (%d)", p->wType);
			}
		}
		DnsRecordListFree(rec, DnsFreeRecordList);
	}
	else {
		notify_verbose_message(pvar, "verify_hostkey_dns: DnsQuery failed.", LOG_LEVEL_VERBOSE);
	}

	free(hostkey_digest);
	return found;
}
