/*
 * Copyright(C) 2008 TeraTerm Project
 */
// PuTTY is copyright 1997-2007 Simon Tatham.

// WINDOWS\WINPGNTC.C
extern int agent_exists(void);
extern int agent_query(void *in, int inlen, void **out, int *outlen,
                       void (*callback)(void *, void *, int), void *callback_ctx);

int putty_get_ssh2_keylist(unsigned char **keylist);
void *putty_sign_ssh2_key(unsigned char *pubkey,
                          unsigned char *data,
                          int *outlen);
int putty_get_ssh1_keylist(unsigned char **keylist);
void *putty_hash_ssh1_challenge(unsigned char *pubkey,
                                int pubkeylen,
                                unsigned char *data,
                                int datalen,
                                unsigned char *session_id,
                                int *outlen);
int putty_get_ssh1_keylen(unsigned char *key,
                          int maxlen);

void safefree(void *);

static void *get_keylist1(int *length);
static void *get_keylist2(int *length);
