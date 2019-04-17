/*
 * (C) 2008-2017 TeraTerm Project
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

#include "ttxssh.h"
#include "util.h"
#include "resource.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/engine.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#include "buffer.h"
#include "ssh.h"
#include "crypt.h"
#include "fwd.h"
#include "ssh.h"
#include "sftp.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>


#define WM_USER_CONSOLE (WM_USER + 1)

/* Separators for interactive commands */
#define WHITESPACE " \t\r\n"

/* Commands for interactive mode */
#define I_CHDIR     1
#define I_CHGRP     2
#define I_CHMOD     3
#define I_CHOWN     4
#define I_DF        24
#define I_GET       5
#define I_HELP      6
#define I_LCHDIR    7
#define I_LINK      25
#define I_LLS       8
#define I_LMKDIR    9
#define I_LPWD      10
#define I_LS        11
#define I_LUMASK    12
#define I_MKDIR     13
#define I_PUT       14
#define I_PWD       15
#define I_QUIT      16
#define I_RENAME    17
#define I_RM        18
#define I_RMDIR     19
#define I_SHELL     20
#define I_SYMLINK   21
#define I_VERSION   22
#define I_PROGRESS  23

/* Type of completion */
#define NOARGS  0
#define REMOTE  1
#define LOCAL   2

struct CMD {
	char *c;
	int n;
	int t;
};

static const struct CMD cmds[] = {
    { "bye",    I_QUIT,     NOARGS  },
    { "cd",     I_CHDIR,    REMOTE  },
    { "chdir",  I_CHDIR,    REMOTE  },
    { "chgrp",  I_CHGRP,    REMOTE  },
    { "chmod",  I_CHMOD,    REMOTE  },
    { "chown",  I_CHOWN,    REMOTE  },
    { "df",     I_DF,       REMOTE  },
    { "dir",    I_LS,       REMOTE  },
    { "exit",   I_QUIT,     NOARGS  },
    { "get",    I_GET,      REMOTE  },
    { "help",   I_HELP,     NOARGS  },
    { "lcd",    I_LCHDIR,   LOCAL   },
    { "lchdir", I_LCHDIR,   LOCAL   },
    { "lls",    I_LLS,      LOCAL   },
    { "lmkdir", I_LMKDIR,   LOCAL   },
    { "ln",     I_LINK,     REMOTE  },
    { "lpwd",   I_LPWD,     LOCAL   },
    { "ls",     I_LS,       REMOTE  },
    { "lumask", I_LUMASK,   NOARGS  },
    { "mkdir",  I_MKDIR,    REMOTE  },
    { "mget",   I_GET,      REMOTE  },
    { "mput",   I_PUT,      LOCAL   },
    { "progress",   I_PROGRESS, NOARGS  },
    { "put",    I_PUT,      LOCAL   },
    { "pwd",    I_PWD,      REMOTE  },
    { "quit",   I_QUIT,     NOARGS  },
    { "rename", I_RENAME,   REMOTE  },
    { "rm",     I_RM,       REMOTE  },
    { "rmdir",  I_RMDIR,    REMOTE  },
    { "symlink",    I_SYMLINK,  REMOTE  },
    { "version",    I_VERSION,  NOARGS  },
    { "!",      I_SHELL,    NOARGS  },
    { "?",      I_HELP,     NOARGS  },
    { NULL,     -1,     -1  }
};



static PTInstVar g_pvar;
static Channel_t *g_channel;

static void sftp_console_message(PTInstVar pvar, Channel_t *c, char *fmt, ...)
{
	char tmp[1024];
	va_list arg;

	va_start(arg, fmt);
	_vsnprintf_s(tmp, sizeof(tmp), _TRUNCATE, fmt, arg);
	va_end(arg);

	SendMessage(c->sftp.console_window, WM_USER_CONSOLE, 0, (LPARAM)tmp);
	logputs(LOG_LEVEL_VERBOSE, tmp);
}

#define sftp_do_syslog(pvar, level, ...) logprintf(level, __VA_ARGS__)
#define sftp_syslog(pvar, ...) logprintf(LOG_LEVEL_VERBOSE, __VA_ARGS__)

// SFTP専用バッファを確保する。SCPとは異なり、先頭に後続のデータサイズを埋め込む。
//
// buffer_t
//    +---------+------------------------------------+
//    | msg_len | data                               |  
//    +---------+------------------------------------+
//       4byte   <------------- msg_len ------------->
//
static void sftp_buffer_alloc(buffer_t **message)
{
	buffer_t *msg;

	msg = buffer_init();
	if (msg == NULL) {
		goto error;
	}
	// Message length(4byte)
	buffer_put_int(msg, 0); 

	*message = msg;

error:
	assert(msg != NULL);
	return;
}

static void sftp_buffer_free(buffer_t *message)
{
	buffer_free(message);
}

// サーバにSFTPパケットを送信する。
static void sftp_send_msg(PTInstVar pvar, Channel_t *c, buffer_t *msg)
{
	char *p;
	int len;

	len = buffer_len(msg);
	p = buffer_ptr(msg);
	// 最初にメッセージサイズを格納する。
	set_uint32(p, len - 4);
	// ペイロードの送信。
	SSH2_send_channel_data(pvar, c, p, len, 0);
}

// サーバから受信したSFTPパケットをバッファに格納する。
static void sftp_get_msg(PTInstVar pvar, Channel_t *c, unsigned char *data, unsigned int buflen, buffer_t **message)
{
	buffer_t *msg = *message;
	int msg_len;

	// バッファを確保し、データをすべて放り込む。以降は buffer_t 型を通して操作する。
	// そうしたほうが OpenSSH のコードとの親和性が良くなるため。
	buffer_clear(msg);
	buffer_append(msg, data, buflen);
	buffer_rewind(msg);

	msg_len = buffer_get_int(msg);
	if (msg_len > SFTP_MAX_MSG_LENGTH) {
		// TODO:
		sftp_syslog(pvar, "Received message too long %u", msg_len);
		goto error;
	}
	if (msg_len + 4 != buflen) {
		// TODO:
		sftp_syslog(pvar, "Buffer length %u is invalid", buflen);
		goto error;
	}

error:
	return;
}

static void sftp_send_string_request(PTInstVar pvar, Channel_t *c, unsigned int id, unsigned int code,
									 char *s, unsigned int len)
{
	buffer_t *msg;

	sftp_buffer_alloc(&msg);
	buffer_put_char(msg, code);
	buffer_put_int(msg, id);
	buffer_put_string(msg, s, len);
	sftp_send_msg(pvar, c, msg);
	sftp_syslog(pvar, "Sent message fd %d T:%u I:%u", c->remote_id, code, id);
	sftp_buffer_free(msg);
}


// SFTP通信開始前のネゴシエーション
// based on do_init()#sftp-client.c(OpenSSH 6.0)
void sftp_do_init(PTInstVar pvar, Channel_t *c)
{
	buffer_t *msg;

	// SFTP管理構造体の初期化
	memset(&c->sftp, 0, sizeof(c->sftp));
	c->sftp.state = SFTP_INIT;
	c->sftp.transfer_buflen = DEFAULT_COPY_BUFLEN;
	c->sftp.num_requests = DEFAULT_NUM_REQUESTS;
	c->sftp.exts = 0;
	c->sftp.limit_kbps = 0;

	// ネゴシエーションの開始
	sftp_buffer_alloc(&msg);
	buffer_put_char(msg, SSH2_FXP_INIT); 
	buffer_put_int(msg, SSH2_FILEXFER_VERSION);
	sftp_send_msg(pvar, c, msg);
	sftp_buffer_free(msg);

	sftp_syslog(pvar, "SFTP client version %u", SSH2_FILEXFER_VERSION);
}

static void sftp_do_init_recv(PTInstVar pvar, Channel_t *c, buffer_t *msg)
{
	unsigned int type;

	type = buffer_get_char(msg);
	if (type != SSH2_FXP_VERSION) {
		goto error;
	}
	c->sftp.version = buffer_get_int(msg);
	sftp_syslog(pvar, "SFTP server version %u, remote version %u", type, c->sftp.version);

	while (buffer_remain_len(msg) > 0) {
		char *name = buffer_get_string_msg(msg, NULL);
		char *value = buffer_get_string_msg(msg, NULL);
		int known = 0;

        if (strcmp(name, "posix-rename@openssh.com") == 0 &&
            strcmp(value, "1") == 0) {
            c->sftp.exts |= SFTP_EXT_POSIX_RENAME;
            known = 1;
        } else if (strcmp(name, "statvfs@openssh.com") == 0 &&
            strcmp(value, "2") == 0) {
            c->sftp.exts |= SFTP_EXT_STATVFS;
            known = 1;
        } else if (strcmp(name, "fstatvfs@openssh.com") == 0 &&
            strcmp(value, "2") == 0) {
            c->sftp.exts |= SFTP_EXT_FSTATVFS;
            known = 1;
        } else if (strcmp(name, "hardlink@openssh.com") == 0 &&
            strcmp(value, "1") == 0) {
            c->sftp.exts |= SFTP_EXT_HARDLINK;
            known = 1;
        }
        if (known) {
            sftp_syslog(pvar, "Server supports extension \"%s\" revision %s",
                name, value);
        } else {
            sftp_syslog(pvar, "Unrecognised server extension \"%s\"", name);
        }

		free(name);
		free(value);
	}

	if (c->sftp.version == 0) {
		c->sftp.transfer_buflen = min(c->sftp.transfer_buflen, 20480);
	}
	c->sftp.limit_kbps = 0;
	if (c->sftp.limit_kbps > 0) {
		// TODO:
	}

	sftp_syslog(pvar, "Connected to SFTP server.");

error:
	return;
}

// パスの送信
// based on do_realpath()#sftp-client.c(OpenSSH 6.0)
static void sftp_do_realpath(PTInstVar pvar, Channel_t *c, char *path)
{
	unsigned int id;

	strncpy_s(c->sftp.path, sizeof(c->sftp.path), path, _TRUNCATE);
	id = c->sftp.msg_id++;
	sftp_send_string_request(pvar, c, id, SSH2_FXP_REALPATH, path, strlen(path));
}

/* Convert from SSH2_FX_ status to text error message */
static const char *fx2txt(int status)
{       
    switch (status) {
    case SSH2_FX_OK:
        return("No error");
    case SSH2_FX_EOF:
        return("End of file");
    case SSH2_FX_NO_SUCH_FILE:
        return("No such file or directory");
    case SSH2_FX_PERMISSION_DENIED:
        return("Permission denied");
    case SSH2_FX_FAILURE:
        return("Failure");
    case SSH2_FX_BAD_MESSAGE:
        return("Bad message");
    case SSH2_FX_NO_CONNECTION:
        return("No connection");
    case SSH2_FX_CONNECTION_LOST:
        return("Connection lost");
    case SSH2_FX_OP_UNSUPPORTED:
        return("Operation unsupported");
    default:
        return("Unknown status");
    }
    /* NOTREACHED */
}

static char *sftp_do_realpath_recv(PTInstVar pvar, Channel_t *c, buffer_t *msg)
{
	unsigned int type, expected_id, count, id;
	char *filename = NULL, *longname;

	type = buffer_get_char(msg);
	id = buffer_get_int(msg);

	expected_id = c->sftp.msg_id - 1; 
	if (id != expected_id) {
		sftp_syslog(pvar, "ID mismatch (%u != %u)", id, expected_id);
		goto error;
	}

	if (type == SSH2_FXP_STATUS) {
		unsigned int status = buffer_get_int(msg);

		sftp_syslog(pvar, "Couldn't canonicalise: %s", fx2txt(status));
		goto error;
	} else if (type != SSH2_FXP_NAME) {
        sftp_syslog(pvar, "Expected SSH2_FXP_NAME(%u) packet, got %u",
            SSH2_FXP_NAME, type);
		goto error;
	}

	count = buffer_get_int(msg);
	if (count != 1) {
		sftp_syslog(pvar, "Got multiple names (%d) from SSH_FXP_REALPATH", count);
		goto error;
	}

	filename = buffer_get_string_msg(msg, NULL);
	longname = buffer_get_string_msg(msg, NULL);
	//a = decode_attrib(&msg);

	sftp_console_message(pvar, c, "SSH_FXP_REALPATH %s -> %s", c->sftp.path, filename);

	free(longname);

error:
	return (filename);
}


u_int
sftp_proto_version(struct sftp *conn)
{       
    return conn->version;
} 

static void
help(void)
{
	sftp_console_message(g_pvar, g_channel, 
		"Available commands:\r\n"
	    "bye                                Quit sftp\r\n"
	    "cd path                            Change remote directory to 'path'\r\n"
	    "chgrp grp path                     Change group of file 'path' to 'grp'\r\n"
	    "chmod mode path                    Change permissions of file 'path' to 'mode'\r\n"
	    "chown own path                     Change owner of file 'path' to 'own'\r\n"
	    "df [-hi] [path]                    Display statistics for current directory or\r\n"
	    "                                   filesystem containing 'path'\r\n"
	    "exit                               Quit sftp\r\n"
	    "get [-Ppr] remote [local]          Download file\r\n"
	    "help                               Display this help text\r\n"
	    "lcd path                           Change local directory to 'path'\r\n"
	    "lls [ls-options [path]]            Display local directory listing\r\n"
	    "lmkdir path                        Create local directory\r\n"
	    "ln [-s] oldpath newpath            Link remote file (-s for symlink)\r\n"
	    "lpwd                               Print local working directory\r\n"
	    "ls [-1afhlnrSt] [path]             Display remote directory listing\r\n"
	    "lumask umask                       Set local umask to 'umask'\r\n"
	    "mkdir path                         Create remote directory\r\n"
	    "progress                           Toggle display of progress meter\r\n"
	    "put [-Ppr] local [remote]          Upload file\r\n"
	    "pwd                                Display remote working directory\r\n"
	    "quit                               Quit sftp\r\n"
	    "rename oldpath newpath             Rename remote file\r\n"
	    "rm path                            Delete remote file\r\n"
	    "rmdir path                         Remove remote directory\r\n"
	    "symlink oldpath newpath            Symlink remote file\r\n"
	    "version                            Show SFTP version\r\n"
	    "!command                           Execute 'command' in local shell\r\n"
	    "!                                  Escape to local shell\r\n"
	    "?                                  Synonym for help\r\n");
}

/*
 * Split a string into an argument vector using sh(1)-style quoting,
 * comment and escaping rules, but with some tweaks to handle glob(3)
 * wildcards.
 * The "sloppy" flag allows for recovery from missing terminating quote, for
 * use in parsing incomplete commandlines during tab autocompletion.
 *
 * Returns NULL on error or a NULL-terminated array of arguments.
 *
 * If "lastquote" is not NULL, the quoting character used for the last
 * argument is placed in *lastquote ("\0", "'" or "\"").
 * 
 * If "terminated" is not NULL, *terminated will be set to 1 when the
 * last argument's quote has been properly terminated or 0 otherwise.
 * This parameter is only of use if "sloppy" is set.
 */
#define MAXARGS 	128
#define MAXARGLEN	8192
static char **
makeargv(const char *arg, int *argcp, int sloppy, char *lastquote,
    u_int *terminated)
{
	int argc, quot;
	size_t i, j;
	static char argvs[MAXARGLEN];
	static char *argv[MAXARGS + 1];
	enum { MA_START, MA_SQUOTE, MA_DQUOTE, MA_UNQUOTED } state, q;

	*argcp = argc = 0;
	if (strlen(arg) > sizeof(argvs) - 1) {
 args_too_longs:
		sftp_syslog(g_pvar, "string too long");
		return NULL;
	}
	if (terminated != NULL)
		*terminated = 1;
	if (lastquote != NULL)
		*lastquote = '\0';
	state = MA_START;
	i = j = 0;
	for (;;) {
		if (isspace(arg[i])) {
			if (state == MA_UNQUOTED) {
				/* Terminate current argument */
				argvs[j++] = '\0';
				argc++;
				state = MA_START;
			} else if (state != MA_START)
				argvs[j++] = arg[i];
		} else if (arg[i] == '"' || arg[i] == '\'') {
			q = arg[i] == '"' ? MA_DQUOTE : MA_SQUOTE;
			if (state == MA_START) {
				argv[argc] = argvs + j;
				state = q;
				if (lastquote != NULL)
					*lastquote = arg[i];
			} else if (state == MA_UNQUOTED) 
				state = q;
			else if (state == q)
				state = MA_UNQUOTED;
			else
				argvs[j++] = arg[i];
		} else if (arg[i] == '\\') {
			if (state == MA_SQUOTE || state == MA_DQUOTE) {
				quot = state == MA_SQUOTE ? '\'' : '"';
				/* Unescape quote we are in */
				/* XXX support \n and friends? */
				if (arg[i + 1] == quot) {
					i++;
					argvs[j++] = arg[i];
				} else if (arg[i + 1] == '?' ||
				    arg[i + 1] == '[' || arg[i + 1] == '*') {
					/*
					 * Special case for sftp: append
					 * double-escaped glob sequence -
					 * glob will undo one level of
					 * escaping. NB. string can grow here.
					 */
					if (j >= sizeof(argvs) - 5)
						goto args_too_longs;
					argvs[j++] = '\\';
					argvs[j++] = arg[i++];
					argvs[j++] = '\\';
					argvs[j++] = arg[i];
				} else {
					argvs[j++] = arg[i++];
					argvs[j++] = arg[i];
				}
			} else {
				if (state == MA_START) {
					argv[argc] = argvs + j;
					state = MA_UNQUOTED;
					if (lastquote != NULL)
						*lastquote = '\0';
				}
				if (arg[i + 1] == '?' || arg[i + 1] == '[' ||
				    arg[i + 1] == '*' || arg[i + 1] == '\\') {
					/*
					 * Special case for sftp: append
					 * escaped glob sequence -
					 * glob will undo one level of
					 * escaping.
					 */
					argvs[j++] = arg[i++];
					argvs[j++] = arg[i];
				} else {
					/* Unescape everything */
					/* XXX support \n and friends? */
					i++;
					argvs[j++] = arg[i];
				}
			}
		} else if (arg[i] == '#') {
			if (state == MA_SQUOTE || state == MA_DQUOTE)
				argvs[j++] = arg[i];
			else
				goto string_done;
		} else if (arg[i] == '\0') {
			if (state == MA_SQUOTE || state == MA_DQUOTE) {
				if (sloppy) {
					state = MA_UNQUOTED;
					if (terminated != NULL)
						*terminated = 0;
					goto string_done;
				}
				sftp_syslog(g_pvar, "Unterminated quoted argument");
				return NULL;
			}
 string_done:
			if (state == MA_UNQUOTED) {
				argvs[j++] = '\0';
				argc++;
			}
			break;
		} else {
			if (state == MA_START) {
				argv[argc] = argvs + j;
				state = MA_UNQUOTED;
				if (lastquote != NULL)
					*lastquote = '\0';
			}
			if ((state == MA_SQUOTE || state == MA_DQUOTE) &&
			    (arg[i] == '?' || arg[i] == '[' || arg[i] == '*')) {
				/*
				 * Special case for sftp: escape quoted
				 * glob(3) wildcards. NB. string can grow
				 * here.
				 */
				if (j >= sizeof(argvs) - 3)
					goto args_too_longs;
				argvs[j++] = '\\';
				argvs[j++] = arg[i];
			} else
				argvs[j++] = arg[i];
		}
		i++;
	}
	*argcp = argc;
	return argv;
}

static int parse_args(const char **cpp, int *pflag, int *rflag, int *lflag, int *iflag,
    int *hflag, int *sflag, unsigned long *n_arg, char **path1, char **path2)
{
    const char *cmd, *cp = *cpp;
    char *cp2 = NULL, **argv;
    int base = 0;
    long l = 0;
    int i, cmdnum, optidx, argc;

    /* Skip leading whitespace */
    cp = cp + strspn(cp, WHITESPACE);

    /* Check for leading '-' (disable error processing) */
    *iflag = 0;
    if (*cp == '-') {
        *iflag = 1;
        cp++;
        cp = cp + strspn(cp, WHITESPACE);
    }

    /* Ignore blank lines and lines which begin with comment '#' char */
    if (*cp == '\0' || *cp == '#')
        return (0);

    if ((argv = makeargv(cp, &argc, 0, NULL, NULL)) == NULL)
        return -1;

	/* Figure out which command we have */
	for (i = 0; cmds[i].c != NULL; i++) {
		if (_strcmpi(cmds[i].c, argv[0]) == 0)
			break;
	}
	cmdnum = cmds[i].n;
	cmd = cmds[i].c;

	/* Special case */
	if (*cp == '!') {
		cp++;
		cmdnum = I_SHELL;
	} else if (cmdnum == -1) {
		sftp_syslog(g_pvar, "Invalid command.");
		return -1;
	}

	/* Get arguments and parse flags */
	*lflag = *pflag = *rflag = *hflag = *n_arg = 0;
	*path1 = *path2 = NULL;
	optidx = 1;
	switch (cmdnum) {
#if 0
	case I_GET:
	case I_PUT:
		if ((optidx = parse_getput_flags(cmd, argv, argc,
		    pflag, rflag)) == -1)
			return -1;
		/* Get first pathname (mandatory) */
		if (argc - optidx < 1) {
			error("You must specify at least one path after a "
			    "%s command.", cmd);
			return -1;
		}
		*path1 = xstrdup(argv[optidx]);
		/* Get second pathname (optional) */
		if (argc - optidx > 1) {
			*path2 = xstrdup(argv[optidx + 1]);
			/* Destination is not globbed */
			undo_glob_escape(*path2);
		}
		break;
	case I_LINK:
		if ((optidx = parse_link_flags(cmd, argv, argc, sflag)) == -1)
			return -1;
	case I_SYMLINK:
	case I_RENAME:
		if (argc - optidx < 2) {
			error("You must specify two paths after a %s "
			    "command.", cmd);
			return -1;
		}
		*path1 = xstrdup(argv[optidx]);
		*path2 = xstrdup(argv[optidx + 1]);
		/* Paths are not globbed */
		undo_glob_escape(*path1);
		undo_glob_escape(*path2);
		break;
	case I_RM:
	case I_MKDIR:
	case I_RMDIR:
	case I_CHDIR:
	case I_LCHDIR:
	case I_LMKDIR:
		/* Get pathname (mandatory) */
		if (argc - optidx < 1) {
			error("You must specify a path after a %s command.",
			    cmd);
			return -1;
		}
		*path1 = xstrdup(argv[optidx]);
		/* Only "rm" globs */
		if (cmdnum != I_RM)
			undo_glob_escape(*path1);
		break;
	case I_DF:
		if ((optidx = parse_df_flags(cmd, argv, argc, hflag,
		    iflag)) == -1)
			return -1;
		/* Default to current directory if no path specified */
		if (argc - optidx < 1)
			*path1 = NULL;
		else {
			*path1 = xstrdup(argv[optidx]);
			undo_glob_escape(*path1);
		}
		break;
	case I_LS:
		if ((optidx = parse_ls_flags(argv, argc, lflag)) == -1)
			return(-1);
		/* Path is optional */
		if (argc - optidx > 0)
			*path1 = xstrdup(argv[optidx]);
		break;
	case I_LLS:
		/* Skip ls command and following whitespace */
		cp = cp + strlen(cmd) + strspn(cp, WHITESPACE);
	case I_SHELL:
		/* Uses the rest of the line */
		break;
	case I_LUMASK:
	case I_CHMOD:
		base = 8;
	case I_CHOWN:
	case I_CHGRP:
		/* Get numeric arg (mandatory) */
		if (argc - optidx < 1)
			goto need_num_arg;
		errno = 0;
		l = strtol(argv[optidx], &cp2, base);
		if (cp2 == argv[optidx] || *cp2 != '\0' ||
		    ((l == LONG_MIN || l == LONG_MAX) && errno == ERANGE) ||
		    l < 0) {
 need_num_arg:
			error("You must supply a numeric argument "
			    "to the %s command.", cmd);
			return -1;
		}
		*n_arg = l;
		if (cmdnum == I_LUMASK)
			break;
		/* Get pathname (mandatory) */
		if (argc - optidx < 2) {
			error("You must specify a path after a %s command.",
			    cmd);
			return -1;
		}
		*path1 = xstrdup(argv[optidx + 1]);
		break;
#endif
	case I_QUIT:
	case I_PWD:
	case I_LPWD:
	case I_HELP:
	case I_VERSION:
	case I_PROGRESS:
		break;
	default:
		//fatal("Command not implemented");
		return -1;
	}

	*cpp = cp;
	return(cmdnum);
}


/*
 * SFTP コマンドラインコンソール
 */
static WNDPROC hEditProc;

static LRESULT CALLBACK EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char buf[512];
	char *cmd;
    char *path1, *path2, *tmp = NULL;
    int pflag = 0, rflag = 0, lflag = 0, iflag = 0, hflag = 0, sflag = 0;
    int cmdnum, i = 0;
    unsigned long n_arg = 0;
	//Attrib a, *aa;
	char path_buf[1024] = {0};
	int err = 0;
	//glob_t g;
	int err_abort;

	switch (uMsg) {
		case WM_KEYDOWN:
		if ((int)wParam == VK_RETURN) {
			GetWindowText(hwnd, buf, sizeof(buf));
			SetWindowText(hwnd, "");
			if (buf[0] != '\0') {
				SendDlgItemMessage(GetParent(hwnd), IDC_SFTP_CONSOLE, EM_REPLACESEL, 0, (LPARAM) buf);
				SendDlgItemMessage(GetParent(hwnd), IDC_SFTP_CONSOLE, EM_REPLACESEL, 0,
								   (LPARAM) (char *) "\r\n");
				goto cmd_parsed;
			}
		}
		break;
	default:
		return (CallWindowProc(hEditProc, hwnd, uMsg, wParam, lParam));
	}
	return 0L;

cmd_parsed:
	// コマンドライン解析
	path1 = path2 = NULL;
	cmd = buf;
	cmdnum = parse_args(&cmd, &pflag, &rflag, &lflag, &iflag, &hflag,
		&sflag, &n_arg, &path1, &path2);

	if (iflag != 0)
		err_abort = 0;

	//memset(&g, 0, sizeof(g));

	/* Perform command */
	switch (cmdnum) {
	case 0:
		/* Blank line */
		break;
	case -1:
		/* Unrecognized command */
		err = -1;
		break;
#if 0
	case I_GET:
		err = process_get(conn, path1, path2, *pwd, pflag, rflag);
		break;
	case I_PUT:
		err = process_put(conn, path1, path2, *pwd, pflag, rflag);
		break;
	case I_RENAME:
		path1 = make_absolute(path1, *pwd);
		path2 = make_absolute(path2, *pwd);
		err = do_rename(conn, path1, path2);
		break;
	case I_SYMLINK:
		sflag = 1;
	case I_LINK:
		path1 = make_absolute(path1, *pwd);
		path2 = make_absolute(path2, *pwd);
		err = (sflag ? do_symlink : do_hardlink)(conn, path1, path2);
		break;
	case I_RM:
		path1 = make_absolute(path1, *pwd);
		remote_glob(conn, path1, GLOB_NOCHECK, NULL, &g);
		for (i = 0; g.gl_pathv[i] && !interrupted; i++) {
			printf("Removing %s\n", g.gl_pathv[i]);
			err = do_rm(conn, g.gl_pathv[i]);
			if (err != 0 && err_abort)
				break;
		}
		break;
	case I_MKDIR:
		path1 = make_absolute(path1, *pwd);
		attrib_clear(&a);
		a.flags |= SSH2_FILEXFER_ATTR_PERMISSIONS;
		a.perm = 0777;
		err = do_mkdir(conn, path1, &a, 1);
		break;
	case I_RMDIR:
		path1 = make_absolute(path1, *pwd);
		err = do_rmdir(conn, path1);
		break;
	case I_CHDIR:
		path1 = make_absolute(path1, *pwd);
		if ((tmp = do_realpath(conn, path1)) == NULL) {
			err = 1;
			break;
		}
		if ((aa = do_stat(conn, tmp, 0)) == NULL) {
			xfree(tmp);
			err = 1;
			break;
		}
		if (!(aa->flags & SSH2_FILEXFER_ATTR_PERMISSIONS)) {
			error("Can't change directory: Can't check target");
			xfree(tmp);
			err = 1;
			break;
		}
		if (!S_ISDIR(aa->perm)) {
			error("Can't change directory: \"%s\" is not "
			    "a directory", tmp);
			xfree(tmp);
			err = 1;
			break;
		}
		xfree(*pwd);
		*pwd = tmp;
		break;
	case I_LS:
		if (!path1) {
			do_ls_dir(conn, *pwd, *pwd, lflag);
			break;
		}

		/* Strip pwd off beginning of non-absolute paths */
		tmp = NULL;
		if (*path1 != '/')
			tmp = *pwd;

		path1 = make_absolute(path1, *pwd);
		err = do_globbed_ls(conn, path1, tmp, lflag);
		break;
	case I_DF:
		/* Default to current directory if no path specified */
		if (path1 == NULL)
			path1 = xstrdup(*pwd);
		path1 = make_absolute(path1, *pwd);
		err = do_df(conn, path1, hflag, iflag);
		break;
	case I_LCHDIR:
		if (chdir(path1) == -1) {
			error("Couldn't change local directory to "
			    "\"%s\": %s", path1, strerror(errno));
			err = 1;
		}
		break;
	case I_LMKDIR:
		if (mkdir(path1, 0777) == -1) {
			error("Couldn't create local directory "
			    "\"%s\": %s", path1, strerror(errno));
			err = 1;
		}
		break;
	case I_LLS:
		local_do_ls(cmd);
		break;
	case I_SHELL:
		local_do_shell(cmd);
		break;
	case I_LUMASK:
		umask(n_arg);
		printf("Local umask: %03lo\n", n_arg);
		break;
	case I_CHMOD:
		path1 = make_absolute(path1, *pwd);
		attrib_clear(&a);
		a.flags |= SSH2_FILEXFER_ATTR_PERMISSIONS;
		a.perm = n_arg;
		remote_glob(conn, path1, GLOB_NOCHECK, NULL, &g);
		for (i = 0; g.gl_pathv[i] && !interrupted; i++) {
			printf("Changing mode on %s\n", g.gl_pathv[i]);
			err = do_setstat(conn, g.gl_pathv[i], &a);
			if (err != 0 && err_abort)
				break;
		}
		break;
	case I_CHOWN:
	case I_CHGRP:
		path1 = make_absolute(path1, *pwd);
		remote_glob(conn, path1, GLOB_NOCHECK, NULL, &g);
		for (i = 0; g.gl_pathv[i] && !interrupted; i++) {
			if (!(aa = do_stat(conn, g.gl_pathv[i], 0))) {
				if (err_abort) {
					err = -1;
					break;
				} else
					continue;
			}
			if (!(aa->flags & SSH2_FILEXFER_ATTR_UIDGID)) {
				error("Can't get current ownership of "
				    "remote file \"%s\"", g.gl_pathv[i]);
				if (err_abort) {
					err = -1;
					break;
				} else
					continue;
			}
			aa->flags &= SSH2_FILEXFER_ATTR_UIDGID;
			if (cmdnum == I_CHOWN) {
				printf("Changing owner on %s\n", g.gl_pathv[i]);
				aa->uid = n_arg;
			} else {
				printf("Changing group on %s\n", g.gl_pathv[i]);
				aa->gid = n_arg;
			}
			err = do_setstat(conn, g.gl_pathv[i], aa);
			if (err != 0 && err_abort)
				break;
		}
		break;
	case I_PWD:
		printf("Remote working directory: %s\n", *pwd);
		break;
	case I_LPWD:
		if (!getcwd(path_buf, sizeof(path_buf))) {
			error("Couldn't get local cwd: %s", strerror(errno));
			err = -1;
			break;
		}
		printf("Local working directory: %s\n", path_buf);
		break;
#endif
	case I_QUIT:
		/* Processed below */
		break;
	case I_HELP:
		help();
		break;
	case I_VERSION:
		printf("SFTP protocol version %u\n", sftp_proto_version(&g_channel->sftp));
		break;
#if 0
	case I_PROGRESS:
		showprogress = !showprogress;
		if (showprogress)
			printf("Progress meter enabled\n");
		else
			printf("Progress meter disabled\n");
		break;
#endif
	default:
		//fatal("%d is not implemented", cmdnum);
		err = -1;
	}

#if 0
	if (g.gl_pathc)
		globfree(&g);
	if (path1)
		xfree(path1);
	if (path2)
		xfree(path2);

	/* If an unignored error occurs in batch mode we should abort. */
	if (err_abort && err != 0)
		return (-1);
	else if (cmdnum == I_QUIT)
		return (1);
#endif

	return 0L;
}

static LRESULT CALLBACK OnSftpConsoleDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static HFONT DlgDragDropFont = NULL;
	LOGFONT logfont;
	HFONT font;
	HWND hEdit;

	switch (msg) {
		case WM_INITDIALOG:
			font = (HFONT)SendMessage(hDlgWnd, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			DlgDragDropFont = NULL;

			hEdit = GetDlgItem(hDlgWnd, IDC_SFTP_EDIT);
			SetFocus(hEdit);

			// エディットコントロールのサブクラス化
			hEditProc = (WNDPROC)GetWindowLong(hEdit, GWL_WNDPROC);
			SetWindowLong(hEdit, GWL_WNDPROC, (LONG)EditProc);

			CenterWindow(hDlgWnd, GetParent(hDlgWnd));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					break;

				case IDCANCEL:
					if (DlgDragDropFont != NULL) {
						DeleteObject(DlgDragDropFont);
					}
					EndDialog(hDlgWnd, IDCANCEL);
					DestroyWindow(hDlgWnd);
					break;

				default:
					return FALSE;
			}

		case WM_USER_CONSOLE:
			SendDlgItemMessage(hDlgWnd, IDC_SFTP_CONSOLE, EM_REPLACESEL, 0, (LPARAM) lp);
			SendDlgItemMessage(hDlgWnd, IDC_SFTP_CONSOLE, EM_REPLACESEL, 0,
							   (LPARAM) (char *) "\r\n");
			return TRUE;

#if 0
		case WM_SIZE:
			{
				// 再配置
				int dlg_w, dlg_h;
				RECT rc_dlg;
				RECT rc;
				POINT p;

				// 新しいダイアログのサイズを得る
				GetClientRect(hDlgWnd, &rc_dlg);
				dlg_w = rc_dlg.right;
				dlg_h = rc_dlg.bottom;

				// コマンドプロンプト
				GetWindowRect(GetDlgItem(hDlgWnd, IDC_SFTP_EDIT), &rc);
				p.x = rc.left;
				p.y = rc.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDC_SFTP_EDIT), 0,
 							 0, 0, dlg_w, p.y,
							 SWP_NOSIZE | SWP_NOZORDER);
			}
			return TRUE;
#endif

		default:
			return FALSE;
	}
	return TRUE;
}

// SFTP受信処理 -ステートマシーン-
void sftp_response(PTInstVar pvar, Channel_t *c, unsigned char *data, unsigned int buflen)
{
	buffer_t *msg;
	HWND hDlgWnd;

	/*
	 * Allocate buffer
	 */
	sftp_buffer_alloc(&msg);
	sftp_get_msg(pvar, c, data, buflen, &msg);

	if (c->sftp.state == SFTP_INIT) {
		// グローバル変数に保存する。
		g_pvar = pvar;
		g_channel = c;

		sftp_do_init_recv(pvar, c, msg);

		// コンソールを起動する。
		hDlgWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SFTP_DIALOG), 
				pvar->cv->HWin, (DLGPROC)OnSftpConsoleDlgProc);	
		if (hDlgWnd != NULL) {
			c->sftp.console_window = hDlgWnd;
			ShowWindow(hDlgWnd, SW_SHOW);
		}

		sftp_do_realpath(pvar, c, ".");
		c->sftp.state = SFTP_CONNECTED;

	} else if (c->sftp.state == SFTP_CONNECTED) {
		char *remote_path;
		remote_path = sftp_do_realpath_recv(pvar, c, msg);

		c->sftp.state = SFTP_REALPATH;

	}

	/*
	 * Free buffer
	 */
	sftp_buffer_free(msg);
}
