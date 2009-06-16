/////////////////////////////////////////////////////////////////////////////
// CygTerm+ - yet another Cygwin console
// Copyright (c) 2006-2008 TeraTerm Project
// Copyright (C) 2000-2006 NSym.
//---------------------------------------------------------------------------
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License (GPL) as published by
// the Free Software Foundation; either version 2 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// CygTerm+ - yet another Cygwin console
//
//   Using Cygwin with a terminal emulator.
//   
//   Writtern by TeraTerm Project.
//            http://ttssh2.sourceforge.jp/
//   
//   Original written by NSym.
//                         *** Web Pages ***
//  (English) http://www.dd.iij4u.or.jp/~nsym/cygwin/cygterm/index-e.html
// (Japanese) http://www.dd.iij4u.or.jp/~nsym/cygwin/cygterm/index.html
//
/////////////////////////////////////////////////////////////////////////////
// patch level 01 - support for "~/.cygtermrc" and "/etc/cygterm.conf"
//   Written by BabyDaemon. (babydamons@yahoo.co.jp)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 03 - change directory to home only if HOME_CHDIR is set
//   Written by IWAMOTO Kouichi. (doda)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 04 - add login shell option
//   Written by IWAMOTO Kouichi. (doda)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 05 - add mutex
//   Written by NAGATA Shinya. (maya.negeta@gmail.com)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 06 - limit a parameter length of -s and -t option
//   Written by IWAMOTO Kouichi. (doda)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 07 - use %HOME% for home directory
//   Written by NAGATA Shinya. (maya.negeta@gmail.com)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 08 - change the priority of config file
//   Written by NAGATA Shinya. (maya.negeta@gmail.com)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 09 - get shell from /etc/passwd if SHELL is not specified
//   Written by IWAMOTO Kouichi. (doda)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 10 - to get user name, use getlogin() instead of $USERNAME
//   Written by IWAMOTO Kouichi. (doda)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 11 - stopped using %HOME% and /etc/passwd for home directory
//                  changed the priority of config files
//   Written by NAGATA Shinya. (maya.negeta@gmail.com)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 12 - add SOCKET_TIMEOUT setting.
//   Written by IWAMOTO Kouichi. (doda)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 13 - added '-d' option that is specifies the start directory
//   Written by NAGATA Shinya. (maya.negeta@gmail.com)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 14 - added '-o' option that is specifies additional option for terminal emulator
//   Written by IWAMOTO Kouichi. (doda)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 15 - add ssh-agent proxy support
//   Written by IWAMOTO Kouichi. (doda)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 16 - added '-A' option and change '-a' option
//   Written by IWAMOTO Kouichi. (doda)
//
/////////////////////////////////////////////////////////////////////////////
// patch level 17 - added TELNET SGA/ECHO negotiation
//   Written by IWAMOTO Kouichi. (doda)
//

static char Program[] = "CygTerm+";
static char Version[] = "version 1.07_17 (2009/06/16)";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <windows.h>
#include <pwd.h>

// pageant support (ssh-agent proxy)
//----------------------------------
#define AGENT_COPYDATA_ID 0x804e50ba
#define AGENT_MAX_MSGLEN 8192
char sockdir[] = "/tmp/ssh-XXXXXXXXXX";
char sockname[256];

// PTY device name
//----------------
#define  DEVPTY  "/dev/ptmx"

// TCP port for TELNET
//--------------------
int port_start = 20000;  // default lowest port number
int port_range = 40;     // default number of ports

// command lines of a terminal-emulator and a shell
//-------------------------------------------------
char cmd_term[256] = "";
char cmd_termopt[256] = "";
char cmd_shell[128] = "";

// TCP port for connection to another terminal application
//--------------------------------------------------------
int cl_port = 0;

// telnet socket timeout
//----------------------
int telsock_timeout = 5;    // timeout 5 sec

// dumb terminal flag
//-------------------
bool dumb = false;

// chdir to HOME
//--------------
bool home_chdir = false;

// login shell flag
//-----------------
bool enable_loginshell = false;

// ssh agent proxy
//----------------
bool enable_agent_proxy = false;

// terminal type & size
//---------------------
char term_type[41] = "";
struct winsize win_size = {0,0,0,0};

// additional env vars given to a shell
//-------------------------------------
struct sh_env_t {
    struct sh_env_t* next;
    char env[1];
} sh_env = {NULL, ""};

sh_env_t* sh_envp = &sh_env;

//================//
// message output //
//----------------//
void msg_print(const char* msg)
{
    MessageBox(NULL, msg, Program, MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
}

//=========================//
// Win32-API error message //
//-------------------------//
void api_error(char* string = "")
{
    char msg[1024];
    char *ptr = msg;
    if (string != NULL)
        ptr += snprintf(ptr, sizeof(msg), "%s\n\n", string);
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        ptr, sizeof(msg)-(ptr-msg), NULL
    );
    msg_print(msg);
}

//=========================//
// C-runtime error message //
//-------------------------//
void c_error(char* string = "")
{
    char msg[1024];
    char *ptr = msg;
    if (string != NULL)
        ptr += snprintf(ptr, sizeof(msg), "%s\n\n", string);
    snprintf(ptr, sizeof(msg)-(ptr-msg), "%s\n", strerror(errno));
    msg_print(msg);
}

//==================================//
// parse line in configuration file //
//----------------------------------//
void parse_cfg_line(char *buf)
{
    // "KEY = VALUE" format in each line.
    // skip leading/trailing blanks. KEY is not case-sensitive.
    char* p1;
    for (p1 = buf; isspace(*p1); ++p1);
    if (!isalpha(*p1)) {
        return; // comment line with non-alphabet 1st char
    }
    char* name = p1;
    for (++p1; isalnum(*p1) || *p1 == '_'; ++p1);
    char* p2;
    for (p2 = p1; isspace(*p2); ++p2);
    if (*p2 != '=') {
        return; // igonore line without '='
    }
    for (++p2; isspace(*p2); ++p2);
    char* val = p2;
    for (p2 += strlen(p2); isspace(*(p2-1)); --p2);
    *p1 = *p2 = 0;

    if (!strcasecmp(name, "TERM")) {
        // terminal emulator command line (host:%s, port#:%d)
        strncpy(cmd_term, val, sizeof(cmd_term)-1);
        cmd_term[sizeof(cmd_term)-1] = 0;
    }
    else if (!strcasecmp(name, "SHELL")) {
        // shell command line
        if (strcasecmp(val, "AUTO") != 0) {
            strncpy(cmd_shell, val, sizeof(cmd_shell)-1);
            cmd_shell[sizeof(cmd_shell)-1] = 0;
        }
    }
    else if (!strcasecmp(name, "PORT_START")) {
        // minimum port# for TELNET
        port_start = atoi(val);
    }
    else if (!strcasecmp(name, "PORT_RANGE")) {
        // number of ports for TELNET
        port_range = atoi(val);
    }
    else if (!strcasecmp(name, "TERM_TYPE")) {
        // terminal type name (maybe overridden by TELNET negotiation.)
        strncpy(term_type, val, sizeof(term_type)-1);
        term_type[sizeof(term_type)-1] = 0;
    }
    else if (!strncasecmp(name, "ENV_", 4)) {
        // additional env vars given to a shell
        sh_env_t* e = (sh_env_t*)malloc(sizeof(sh_env_t)+strlen(val));
        if (e != NULL) {
            strcpy(e->env, val);
            e->next = NULL;
            sh_envp = (sh_envp->next = e);
        }
    }
    else if (!strcasecmp(name, "HOME_CHDIR")) {
        // change directory to home
        if (strchr("YyTt", *val) != NULL || atoi(val) > 0) {
            home_chdir = true;
        }
    }
    else if (!strcasecmp(name, "LOGIN_SHELL")) {
        // execute a shell as a login shell
        if (strchr("YyTt", *val) != NULL || atoi(val) > 0) {
            enable_loginshell = true;
        }
    }
    else if (!strcasecmp(name, "SOCKET_TIMEOUT")) {
        // telnet socket timeout
        telsock_timeout = atoi(val);
    }
    else if (!strcasecmp(name, "SSH_AGENT_PROXY")) {
        // ssh-agent proxy
        if (strchr("YyTt", *val) != NULL || atoi(val) > 0) {
            enable_agent_proxy = true;
        }
    }

    return;
}

//====================//
// load configuration //
//--------------------//
void load_cfg()
{
    // Windows system configuration file (.cfg) path
    char win_conf[MAX_PATH];

    // get cfg path from exe path
    if (GetModuleFileName(NULL, win_conf, MAX_PATH) <= 0) {
        return;
    }
    char* bs = strrchr(win_conf, '\\');
    if (bs == NULL) {
        return;
    }
    char* dot = strrchr(bs, '.');
    if (dot == NULL) {
        strcat(bs, ".cfg");
    } else {
        strcpy(dot, ".cfg");
    }

    char sys_conf[] = "/etc/cygterm.conf";

    // user configuration file (~/.*rc) path
    char usr_conf[MAX_PATH] = "";

    // auto generated configuration file path
    char tmp_conf[MAX_PATH] = "/tmp/cygtermrc.XXXXXX";

    // get user name from getlogin().  if it fails, use $USERNAME instead.
    // and get /etc/passwd information by getpwnam(3) with user name,
    // and generate temporary configuration file by mktemp(3).
    const char* username = getlogin();
    if (username == NULL)
        username = getenv("USERNAME");
    if (username != NULL) {
        struct passwd* pw_ent = getpwnam(username);
        if (pw_ent != NULL) {
            strcpy(usr_conf, pw_ent->pw_dir);
            strcat(usr_conf, "/.");
            strcat(usr_conf, bs + 1);
            char* dot = strrchr(usr_conf, '.');
            if (dot == NULL) {
                strcat(bs, "rc");
            } else {
                strcpy(dot, "rc");
            }
        }
        mktemp(tmp_conf);
        FILE* fp = fopen(tmp_conf, "w");
        if (fp != NULL) {
            if (pw_ent != NULL) {
                fprintf(fp, "ENV_1=USER=%s\n",  pw_ent->pw_name);
                fprintf(fp, "ENV_2=SHELL=%s\n", pw_ent->pw_shell);
                fprintf(fp, "SHELL=%s\n", pw_ent->pw_shell);
            } else {
                fprintf(fp, "ENV_1=USER=%s\n",       username);
            }
            fclose(fp);
        }
    }

    if (strcmp(usr_conf, "") == 0) {
        strcpy(usr_conf, "");
        strcpy(tmp_conf, "");
    }

    char *conf_path[] = { tmp_conf, win_conf, sys_conf, usr_conf };
    for (int i = 0; i < 4; i++) {
        // ignore empty configuration file path
        if (strcmp(conf_path[i], "") == 0) {
            continue;
        }
        // read each setting parameter
        FILE* fp;
        if ((fp = fopen(conf_path[i], "r")) == NULL) {
            continue;
        }
        char buf[BUFSIZ];
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            parse_cfg_line(buf);
        }
        fclose(fp);
    }

    // remove temporary configuration file, if it was generated.
    if (strcmp(tmp_conf, "") != 0) {
        unlink(tmp_conf);
    }
}

//=======================//
// commandline arguments //
//-----------------------//
void get_args(int argc, char** argv)
{
    char tmp[sizeof(cmd_termopt)];

    for (++argv; *argv != NULL; ++argv) {
        if (!strcmp(*argv, "-t")) {             // -t <terminal emulator>
            if (*++argv == NULL)
                break;
            strncpy(cmd_term, *argv, sizeof(cmd_term)-1);
            cmd_term[sizeof(cmd_term)-1] = '\0';
        }
        else if (!strcmp(*argv, "-p")) {        // -p <port#>
            if (*(argv+1) != NULL) {
                ++argv, cl_port = atoi(*argv);
            }
        }
        else if (!strcmp(*argv, "-dumb")) {     // -dumb
            dumb = true;
            strcpy(term_type, "dumb");
        }
        else if (!strcmp(*argv, "-s")) {        // -s <shell>
            if (*++argv == NULL)
                break;
            strncpy(cmd_shell, *argv, sizeof(cmd_shell)-1);
            cmd_shell[sizeof(cmd_shell)-1] = '\0';
        }
        else if (!strcmp(*argv, "-cd")) {       // -cd
            home_chdir = true;
        }
        else if (!strcmp(*argv, "-nocd")) {     // -nocd
            home_chdir = false;
        }
        else if (!strcmp(*argv, "+cd")) {       // +cd
            home_chdir = false;
        }
        else if (!strcmp(*argv, "-ls")) {       // -ls
            enable_loginshell = true;
        }
        else if (!strcmp(*argv, "-nols")) {     // -nols
            enable_loginshell = false;
        }
        else if (!strcmp(*argv, "+ls")) {       // +ls
            enable_loginshell = false;
        }
        else if (!strcmp(*argv, "-A")) {       // -A
            enable_agent_proxy = true;
        }
        else if (!strcmp(*argv, "-a")) {       // -a
            enable_agent_proxy = false;
        }
        else if (!strcmp(*argv, "-v")) {        // -v <additional env var>
            if (*(argv+1) != NULL) {
                ++argv;
                sh_env_t* e = (sh_env_t*)malloc(sizeof(sh_env_t)+strlen(*argv));
                if (e != NULL) {
                    strcpy(e->env, *argv);
                    e->next = NULL;
                    sh_envp = (sh_envp->next = e);
                }
            }
        }
        else if (!strcmp(*argv, "-d")) {        // -d <exec directory>
            if (*++argv == NULL)
                break;
            chdir(*argv);
        }
        else if (!strcmp(*argv, "-o")) {        // -o <additional option for terminal>
            if (*++argv == NULL)
                break;
            if (cmd_termopt[0] == '\0') {
                strncpy(cmd_termopt, *argv, sizeof(cmd_termopt)-1);
                cmd_termopt[sizeof(cmd_termopt)-1] = '\0';
            }
            else {
                snprintf(tmp, sizeof(tmp), "%s %s", cmd_termopt, *argv);
                strncpy(cmd_termopt, tmp, sizeof(cmd_termopt)-1);
                cmd_termopt[sizeof(cmd_termopt)-1] = '\0';
            }
        }
    }
}

//===================================//
// pageant support (ssh-agent proxy) //
//-----------------------------------//
unsigned long get_uint32(unsigned char *buff)
{
	return ((unsigned long)buff[0] << 24) +
	       ((unsigned long)buff[1] << 16) +
	       ((unsigned long)buff[2] <<  8) +
	       ((unsigned long)buff[3]);
}

void set_uint32(unsigned char *buff, unsigned long v)
{
	buff[0] = (unsigned char)(v >> 24);
	buff[1] = (unsigned char)(v >> 16);
	buff[2] = (unsigned char)(v >>  8);
	buff[3] = (unsigned char)v;
	return;
}

unsigned long agent_request(unsigned char *out, unsigned long out_size, unsigned char *in)
{
	HWND hwnd;
	char mapname[25];
	HANDLE fmap = NULL;
	unsigned char *p = NULL;
	COPYDATASTRUCT cds;
	unsigned long len;
	unsigned long ret = 0;

	if (out_size < 5) {
		return 0;
	}
	if ((len = get_uint32(in)) > AGENT_MAX_MSGLEN) {
		goto agent_error;
	}

	hwnd = FindWindow("Pageant", "Pageant");
	if (!hwnd) {
		goto agent_error;
	}

	sprintf(mapname, "PageantRequest%08x", (unsigned)GetCurrentThreadId());
	fmap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
	                         0, AGENT_MAX_MSGLEN, mapname);
	if (!fmap) {
		goto agent_error;
	}

	if ((p = (unsigned char *)MapViewOfFile(fmap, FILE_MAP_WRITE, 0, 0, 0)) == NULL) {
		goto agent_error;
	}

	cds.dwData = AGENT_COPYDATA_ID;
	cds.cbData = strlen(mapname) + 1;
	cds.lpData = mapname;

	memcpy(p, in, len + 4);
	if (SendMessage(hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds) > 0) {
		len = get_uint32(p);
		if (out_size >= len + 4) {
			memcpy(out, p, len + 4);
			ret = len + 4;
		}
	}

agent_error:
	if (p) {
		UnmapViewOfFile(p);
	}
	if (fmap) {
		CloseHandle(fmap);
	}
	if (ret == 0) {
		set_uint32(out, 1);
		out[4] = 5; // SSH_AGENT_FAILURE
	}

	return ret;
}

void sighandler(int sig) {
	unlink(sockname);
	rmdir(sockdir);
	exit(0);
};

//=================//
// ssh-agent proxy //
//-----------------//
void agent_proxy()
{
	int sock, asock;
	unsigned long readlen, reqlen, recvlen;
	struct sockaddr_un addr;
	unsigned char buff[AGENT_MAX_MSGLEN];
	struct sigaction act;
	sigset_t blk;

	if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		msg_print("socket failed.");
		exit(0);
	}
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strlcpy(addr.sun_path, sockname, sizeof(addr.sun_path));

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		goto agent_thread_cleanup;
	}
	if (listen(sock, -1) < 0) {
		goto agent_thread_cleanup;
	}

	sigfillset(&blk);
	sigdelset(&blk, SIGKILL);
	sigdelset(&blk, SIGSTOP);

	memset(&act, 0, sizeof(act));
	act.sa_handler = sighandler;
	act.sa_mask = blk;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);

	while (1) {
		if ((asock = accept(sock, NULL, NULL)) < 0) {
			switch (errno) {
			case EINTR:
			case ECONNABORTED:
				continue;
			}
			break;
		}
		recvlen = 0;
		while ((readlen = read(asock, buff+recvlen, 4-recvlen)) > 0) {
			recvlen += readlen;
			if (recvlen < 4) {
				continue;
			}
			recvlen = 0;
			reqlen = get_uint32(buff);
			while ((readlen = read(asock, buff+4+recvlen, reqlen-recvlen)) > 0) {
				recvlen += readlen;
				if (recvlen < reqlen) {
					continue;
				}

				sigprocmask(SIG_BLOCK, &blk, NULL);
				readlen = agent_request(buff, sizeof(buff), buff);
				sigprocmask(SIG_UNBLOCK, &blk, NULL);

				if (readlen > 0) {
					write(asock, buff, readlen);
				}
				else {
					set_uint32(buff, 1);
					buff[4] = 5; // SSH_AGENT_FAILURE
					write(asock, buff, 5);
				}
			}
			recvlen = 0;
		}
		shutdown(asock, SHUT_RDWR);
		close(asock);
	}

agent_thread_cleanup:
	shutdown(sock, SHUT_RDWR);
	close(sock);

	unlink(sockname);
	rmdir(sockdir);

	exit(0);
}

int exec_agent_proxy()
{
	int pid;
	sh_env_t *e;
	int malloc_size;

	if (mkdtemp(sockdir) == NULL) {
		return -1;
	}
	snprintf(sockname, sizeof(sockname), "%s/agent.%ld", sockdir, getpid());

	malloc_size = sizeof(sh_env_t) + strlen(sockname) + 15;
	e = (sh_env_t*)malloc(malloc_size);
	if (!e) {
		return -1;
	}
	snprintf(e->env, malloc_size - sizeof(sh_env_t), "SSH_AUTH_SOCK=%s", sockname);
	e->next = NULL;
	sh_envp = (sh_envp->next = e);
	
	if ((pid = fork()) < 0) {
		return -1;
	}
	if (pid == 0) {
		setsid();
		agent_proxy();
	}
	return pid;
}

//=============================//
// terminal emulator execution //
//-----------------------------//
DWORD WINAPI term_thread(LPVOID)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    FillMemory(&si, sizeof(si), 0);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;
    DWORD flag = 0;
    if (!CreateProcess(
         NULL, cmd_term, NULL, NULL, FALSE, flag, NULL, NULL, &si, &pi))
    {
        api_error(cmd_term);
        return 0;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}

//============================-==========//
// thread creation for terminal emulator //
//---------------------------------------//
HANDLE exec_term()
{
    DWORD id;
    return CreateThread(NULL, 0, term_thread, NULL, 0, &id);
}

//=======================================//
// listener socket for TELNET connection //
//---------------------------------------//
int listen_telnet(u_short* port)
{
    int lsock;
    if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int i;
    for (i = 0; i < port_range; ++i) { // find an unused port#
        addr.sin_port = htons(port_start + i);
        if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            break;
        }
    }
    if (i == port_range) {
        shutdown(lsock, 2);
        close(lsock);
        return -1;
    }
    if (listen(lsock, 1) != 0) {
        shutdown(lsock, 2);
        close(lsock);
        return -1;
    }
    *port = addr.sin_port;
    return lsock;
}

//=============================//
// accept of TELNET connection //
//-----------------------------//
int accept_telnet(int lsock)
{
    fd_set rbits;
    FD_ZERO(&rbits);
    FD_SET(lsock, &rbits);
    struct timeval tm;
    tm.tv_sec = telsock_timeout;
    tm.tv_usec = 0;
    if (select(FD_SETSIZE, &rbits, 0, 0, &tm) <= 0) {
        return -1;
    }
    if (!FD_ISSET(lsock, &rbits)) {
        return -1;
    }
    int asock;
    struct sockaddr_in addr;
    int len = sizeof(addr);
    if ((asock = accept(lsock, (struct sockaddr *)&addr, &len)) < 0) {
        return -1;
    }
    if (getpeername(asock, (struct sockaddr *)&addr, &len) != 0) {
        shutdown(asock, 2);
        close(asock);
        return -1;
    }
    if (addr.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
        // reject it except local connection
        shutdown(asock, 2);
        close(asock);
        return -1;
    }
    return asock;
}

//============================//
// connect to specified port# //
//----------------------------//
int connect_client()
{
    int csock;
    if ((csock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(cl_port);
    if (connect(csock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(csock);
        return -1;
    }
    return csock;
}

//========================================//
// setup *argv[] from a string for exec() //
//----------------------------------------//
void get_argv(char **argv, int maxc, char *s)
{
    int esc, sq, dq;  // recognize (\) (') (") and tokenize
    int c, argc;
    char *p;
    esc = sq = dq = 0;
    for (argc = 0; argc < maxc-1; ++argc) {
        for ( ; isascii(*s) && isspace(*s); ++s);
        if (*s == 0) {
            break;
        }
        argv[argc] = p = s;
        while ((c = *s) != 0) {
            ++s;
            if (isspace(c) && !esc && !sq && !dq) {
                break;
            }
            if (c == '\'' && !esc && !dq) {
                sq ^= 1;
            } else if (c == '"' && !esc && !sq) {
                dq ^= 1;
            } else if (c == '\\' && !esc) {
                esc = 1;
            } else {
                esc = 0;
                *p++ = c;
            }
        }
        *p = 0;
    }
    // not to judge syntax errors
    // if (dq || sq || esc) { syntax error }
    // if (argc == maxc) { overflow }
    argv[argc] = NULL;
}

//=================//
// shell execution //
//-----------------//
int exec_shell(int* sh_pid)
{
    // open pty master
    int master;
    if ((master = open(DEVPTY, O_RDWR)) < 0) {
        return -1;
    }
    int pid;
    if ((pid = fork()) < 0) {
        return -1;
    }
    if (pid == 0) {
        // detach from control tty
        setsid();
        // open pty slave
        int slave;
        if ((slave = open(ptsname(master), O_RDWR)) < 0) {
            exit(0);
        }
        // stdio redirection
        while (slave <= 2) {
            if ((slave = dup(slave)) < 0) {
                exit(0);
            }
        }
        int fd;
        for (fd = 0; fd < 3; ++fd) {
            close(fd);
            dup(slave);
            fcntl(fd, F_SETFD, 0);
        }
        for (fd = 3; fd < getdtablesize(); ++fd) {
            if (fcntl(fd, F_GETFD) == 0) {
                close(fd);
            }
        }
        // set env vars
        if (*term_type != 0) {
            // set terminal type to $TERM
            char env_term[64];
            sprintf(env_term, "TERM=%s", term_type);
            putenv(env_term);
        }
        // set other additional env vars
        sh_env_t* e;
        for (e = sh_env.next; e != NULL; e = e->next) {
            putenv(e->env);
        }
        if (home_chdir) {
            // chdir to home directory
            const char *home_dir = getenv("HOME");
            // ignore chdir(2) system-call error.
            chdir(home_dir);
        }
        // execute a shell
        char *argv[32];
        get_argv(argv, 32, cmd_shell);
        if (enable_loginshell) {
                char shell_path[128];
                char *pos;
                strcpy(shell_path, argv[0]);
                if ((pos = strrchr(argv[0], '/')) != NULL) {
                        *pos = '-';
                        argv[0] = pos;
                }
                execv(shell_path, argv);
        }
        else {
                execv(argv[0], argv);
        }
        // no error, exec() doesn't return
        c_error(argv[0]);
        exit(0);
    }
    *sh_pid = pid;
    return master;
}

//==================//
// i/o buffer class //
//------------------//
class IOBuf
{
private:
    int fd;
    u_char i_buf[4096];
    u_char o_buf[4096];
    int i_pos, i_len, o_pos;
public:
    IOBuf(int channel) : fd(channel), i_pos(0), i_len(0), o_pos(0) {}
    operator int() { return fd; }
    void ungetc() { --i_pos; }
    bool flush_in();
    bool getc(u_char*);
    bool nextc(u_char*);
    bool putc(u_char);
    bool flush_out();
};

// read bytes into input buffer
//-----------------------------
bool IOBuf::flush_in()
{
    if ((i_len = read(fd, i_buf, sizeof(i_buf))) <= 0)
        return false;
    i_pos = 0;
    return true;
}

// get 1 char from input buffer
//-----------------------------
inline bool IOBuf::getc(u_char* c)
{
    if (i_pos == i_len) return false;
    *c = i_buf[i_pos++];
    return true;
}

// get next 1 char from input buffer
//----------------------------------
inline bool IOBuf::nextc(u_char* c)
{
    if (i_pos == i_len)
        if (!flush_in()) return false;
    *c = i_buf[i_pos++];
    return true;
}

// put 1 char to output buffer
//----------------------------
inline bool IOBuf::putc(u_char c)
{
    if (o_pos == sizeof(o_buf))
        if (!flush_out()) return false;
    o_buf[o_pos++] = c;
    return true;
}

// write bytes from output buffer
//-------------------------------
bool IOBuf::flush_out()
{
    int n;
    for (int i = 0; i < o_pos; i += n) {
        if ((n = write(fd, o_buf+i, o_pos-i)) <= 0) return false;
    }
    o_pos = 0;
    return true;
}

//=========================//
// TELNET command handling //  (see RFC854 TELNET PROTOCOL SPECIFICATION)
//-------------------------//
enum { nIAC=255, nWILL=251, nWONT=252, nDO=253, nDONT=254 };
enum { sSEND=1, sIS=0, sSB=250, sSE=240 };
enum { oECHO=1, oSGA=3, oTERM=24, oNAWS=31 };

bool c_will_term = false;
bool c_will_naws = false;

u_char telnet_cmd(IOBuf* te)
{
    u_char cmd, c;
    te->nextc(&cmd);
    if (cmd == sSB) {
        te->nextc(&c);
        // accept terminal type request
        if (c == oTERM) {                      // "SB TERM
            te->nextc(&c);                     //     IS
            u_char* p = (u_char*)term_type;
            te->nextc(p);                      //     TERMINAL-TYPE
            while (*p != nIAC) {
                if (isupper(*p)) *p = _tolower(*p);
                ++p; te->nextc(p);
            }
            *p = 0;
            te->nextc(&c);                     //     IAC SE"
            return (u_char)oTERM;
        }
        // accept terminal size request
        if (c == oNAWS) {                      // "SB NAWS
            u_short col, row;
            te->nextc((u_char*)&col);
            te->nextc((u_char*)&col+1);        //     00 00 (cols)
            te->nextc((u_char*)&row);
            te->nextc((u_char*)&row+1);        //     00 00 (rows)
            te->nextc(&c);
            te->nextc(&c);                     //     TAC SE"
            win_size.ws_col = ntohs(col);
            win_size.ws_row = ntohs(row);
            return (u_char)oNAWS;
        }
        while (c != nIAC) te->nextc(&c);       // "... IAC SE"
        te->nextc(&c);
    }
    else if (cmd == nWILL || cmd == nWONT || cmd == nDO || cmd == nDONT) {
        u_char c;
        te->nextc(&c);
        if (cmd == nWILL && c == oTERM)        // "WILL TERM"
            c_will_term = true;
        else if (cmd == nWILL && c == oNAWS)   // "WILL NAWS"
            c_will_naws = true;
    }
    return cmd;
}

//============================//
// TELNET initial negotiation //
//----------------------------//
void telnet_nego(int te_sock)
{
    IOBuf te = te_sock;
    u_char c;

    // start terminal type negotiation
    // IAC DO TERMINAL-TYPE
    te.putc(nIAC); te.putc(nDO); te.putc(oTERM);
    te.flush_out();
    te.nextc(&c);
    if (c != nIAC) {
        te.ungetc();
        return;
    }
    (void)telnet_cmd(&te);
    if (c_will_term) {
        // terminal type sub-negotiation
        // IAC SB TERMINAL-TYPE SEND IAC SE
        te.putc(nIAC); te.putc(sSB); te.putc(oTERM);
        te.putc(sSEND); te.putc(nIAC); te.putc(sSE);
        te.flush_out();
        // accept terminal type response
        te.nextc(&c);
        if (c != nIAC) {
            te.ungetc();
            return;
        }
        (void)telnet_cmd(&te);
    }

    // start terminal size negotiation
    // IAC DO WINDOW-SIZE
    te.putc(nIAC); te.putc(nDO); te.putc(oNAWS);
    te.flush_out();
    te.nextc(&c);
    if (c != nIAC) {
        te.ungetc();
        return;
    }
    (void)telnet_cmd(&te);
    if (c_will_naws) {
        // accept terminal size response
        te.nextc(&c);
        if (c != nIAC) {
            te.ungetc();
            return;
        }
        (void)telnet_cmd(&te);
    }

    // SGA/ECHO
    te.putc(nIAC); te.putc(nWILL); te.putc(oSGA);
    te.putc(nIAC); te.putc(nDO); te.putc(oSGA);
    te.putc(nIAC); te.putc(nWILL); te.putc(oECHO);
    te.flush_out();
}

//=============================================//
// relaying of a terminal emulator and a shell //
//---------------------------------------------//
void telnet_session(int te_sock, int sh_pty)
{
    IOBuf te = te_sock;
    IOBuf sh = sh_pty;
    fd_set rtmp, rbits;
    FD_ZERO(&rtmp);
    FD_SET(te, &rtmp);
    FD_SET(sh, &rtmp);
    u_char c;
    int cr = 0;
    int cnt = 0;
    for (;;) {
        rbits = rtmp;
        if (select(FD_SETSIZE, &rbits, 0, 0, 0) <= 0) {
            break;
        }
        if (FD_ISSET(sh, &rbits)) {
            // send data from a shell to a terminal
            if (sh.flush_in() == false) {
                break;
            }
            while (sh.getc(&c) == true) {
                if (c == nIAC) {
                    // escape a TELNET IAC char
                    te.putc(c);
                }
                te.putc(c);
            }
            if (te.flush_out() == false) {
                break;
            }
            if (cnt++ < 20) {
                continue;  // give priority to data from a shell
            }
            cnt = 0;
        }
        if (FD_ISSET(te, &rbits)) {
            // send data from a terminal to a shell
            if (te.flush_in() == false) {
                break;
            }
            while (te.getc(&c) == true) {
                if (c == nIAC && !dumb) {
                    u_char cmd = telnet_cmd(&te) ;
                    if (cmd == oNAWS) {
                        // resize pty by terminal size change notice
                        ioctl(sh_pty, TIOCSWINSZ, &win_size);
                        continue;
                    }
                    if (cmd != nIAC) {
                        continue;
                    }
                } else if (c == '\r') {
                    cr = 1;
                } else if (c == '\n' || c == '\0') {
                    if (cr) {  // do not send LF or NUL just after CR
                        cr = 0;
                        continue;
                    }
                } else {
                    cr = 0;
                }
                sh.putc(c);
            }
            if (sh.flush_out() == false) {
                break;
            }
        }
    }
}

//=========================================================//
// connection of TELNET terminal emulator and Cygwin shell //
//---------------------------------------------------------//
int main(int argc, char** argv)
{
    int listen_sock = -1;
    u_short listen_port;
    int te_sock = -1;
    int sh_pty = -1;
    HANDLE hTerm = NULL;
    int sh_pid, agent_pid = 0;

    // Create mutex for running check by installer (2006.8.18 maya)
    HANDLE hMutex;
    hMutex = CreateMutex(NULL, TRUE, "CygTermAppMutex");

    // load configuration
    load_cfg();

    // read commandline arguments
    get_args(argc, argv);

    if (cmd_shell[0] == 0) {
        msg_print("missing shell");
        return 0;
    }
    if (cmd_term[0] == 0 && cl_port <= 0) {
        msg_print("missing terminal");
        return 0;
    }

    // terminal side connection
    if (cl_port > 0) {
        // connect to the specified TCP port
        if ((te_sock = connect_client()) < 0) {
            goto cleanup;
        }
    } else {
        // prepare a TELNET listener socket
        if ((listen_sock = listen_telnet(&listen_port)) < 0) {
            goto cleanup;
        }
        in_addr addr;
        addr.s_addr = htonl(INADDR_LOOPBACK);
        char tmp[256];
        snprintf(tmp, sizeof(tmp), cmd_term, inet_ntoa(addr), (int)ntohs(listen_port));
        snprintf(cmd_term, sizeof(cmd_term), "%s %s", tmp, cmd_termopt);

        // execute a terminal emulator
        if ((hTerm = exec_term()) == NULL) {
            goto cleanup;
        }
        // accept connection from the terminal emulator
        if ((te_sock = accept_telnet(listen_sock)) < 0) {
            goto cleanup;
        }
        shutdown(listen_sock, 2);
        close(listen_sock);
        listen_sock = -1;
    }
    // TELNET negotiation
    if (!dumb) {
        telnet_nego(te_sock);
    }

    // execute ssh-agent proxy
    if (enable_agent_proxy) {
        agent_pid = exec_agent_proxy();
    }

    // execute a shell
    if ((sh_pty = exec_shell(&sh_pid)) < 0) {
        goto cleanup;
    }
    // set initial pty window size
    if (!dumb && c_will_naws && win_size.ws_col != 0) {
        ioctl(sh_pty, TIOCSWINSZ, &win_size);
    }

    // relay the terminal emulator and the shell
    telnet_session(te_sock, sh_pty);

  cleanup:
    if (agent_pid > 0) {
        kill(agent_pid, SIGTERM);
    }
    if (sh_pty >= 0) {
        close(sh_pty);
        kill(sh_pid, SIGKILL);
    }
    if (agent_pid > 0 || sh_pty >= 0) {
        wait((int*)NULL);
    }
    if (listen_sock >= 0) {
        shutdown(listen_sock, 2);
        close(listen_sock);
    }
    if (te_sock >= 0) {
        shutdown(te_sock, 2);
        close(te_sock);
    }
    if (hTerm != NULL) {
        WaitForSingleObject(hTerm, INFINITE);
        CloseHandle(hTerm);
    }
    return 0;
}

#ifdef NO_WIN_MAIN
// This program is an Win32 application but, start as Cygwin main().
//------------------------------------------------------------------
extern "C" {
    void mainCRTStartup(void);
    void WinMainCRTStartup(void) { mainCRTStartup(); }
};
#endif

//EOF
