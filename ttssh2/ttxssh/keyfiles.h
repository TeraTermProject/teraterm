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

#ifndef __KEYFILES_H
#define __KEYFILES_H

typedef enum {
	SSH2_KEYFILE_TYPE_NONE,
	SSH2_KEYFILE_TYPE_OPENSSH,
	SSH2_KEYFILE_TYPE_PUTTY,
	SSH2_KEYFILE_TYPE_SECSH
} ssh2_keyfile_type;

Key * KEYFILES_read_private_key(PTInstVar pvar,
                                char * relative_name,
                                char * passphrase,
                                BOOL * invalid_passphrase,
                                BOOL is_auto_login);

Key *read_SSH2_private_key(PTInstVar pvar,
                           FILE * fp,
                           char * passphrase,
                           BOOL * invalid_passphrase,
                           BOOL is_auto_login,
                           char *errmsg,
                           int errmsg_len);

Key *read_SSH2_PuTTY_private_key(PTInstVar pvar,
                                 FILE * fp,
                                 char * passphrase,
                                 BOOL * invalid_passphrase,
                                 BOOL is_auto_login,
                                 char *errmsg,
                                 int errmsg_len);

Key *read_SSH2_SECSH_private_key(PTInstVar pvar,
                                 FILE * fp,
                                 char * passphrase,
                                 BOOL * invalid_passphrase,
                                 BOOL is_auto_login,
                                 char *errmsg,
                                 int errmsg_len);

ssh2_keyfile_type get_ssh2_keytype(char *relative_name,
                                   FILE **fp,
                                   char *errmsg,
                                   int errmsg_len);

/* openssh private key file format */
#define MARK_BEGIN		"-----BEGIN OPENSSH PRIVATE KEY-----\n"
#define MARK_END		"-----END OPENSSH PRIVATE KEY-----\n"
#define KDFNAME			"bcrypt"
#define AUTH_MAGIC		"openssh-key-v1"
#define SALT_LEN		16
#define DEFAULT_CIPHERNAME	"aes256-cbc"
#define	DEFAULT_ROUNDS		16

#define MAX_KEY_FILE_SIZE	(1024 * 1024)

#endif
