
#include <stdlib.h>
#include "libputty.h"

int putty_get_ssh2_keylist(unsigned char **keylist)
{
	*keylist = NULL;
	return 0;
}

void *putty_sign_ssh2_key(unsigned char *pubkey,
                          unsigned char *data,
                          int datalen,
                          int *outlen,
                          int signflags)
{
	return NULL;
}

int putty_get_ssh1_keylist(unsigned char **keylist)
{
	return 0;
}

void *putty_hash_ssh1_challenge(unsigned char *pubkey,
                                int pubkeylen,
                                unsigned char *data,
                                int datalen,
                                unsigned char *session_id,
                                int *outlen)
{
	return NULL;
}

int putty_get_ssh1_keylen(unsigned char *key, int maxlen)
{
	return 0;
}

const char *putty_get_version()
{
	return "libsshagent 0.0";
}

void putty_agent_query_synchronous(void *in, int inlen, void **out, int *outlen)
{}

BOOL putty_agent_exists()
{
	return FALSE;
}

void safefree(void *p)
{
	free(p);
}
