/////////////////////////////////////////////////////////////////////////////
// CygTerm+ - yet another Cygwin console
// Copyright (C) 2000-2006 NSym.
// (C) 2006- TeraTerm Project
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
//            https://ttssh2.osdn.jp/
//
//   Original written by NSym.
//

#if !defined(__CYGWIN__)
#error check compiler
#endif

// MessageBoxのタイトルで使用 TODO exeファイル名に変更
static char Program[] = "CygTerm+";
//static char Version[] = "version 1.07_30_beta (2021/11/14)";

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
#include <shlobj.h>
#include <pwd.h>
#include <sys/select.h>
#include <wchar.h>

#include "sub.h"

#include "cygterm_cfg.h"

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
#define PORT_START_DEFAULT 20000  // default lowest port number
#define PORT_RANGE_DEFAULT 40     // default number of ports

// TCP port for connection to another terminal application
//--------------------------------------------------------
int cl_port = 0;
u_short listen_port;

// telnet socket timeout
//----------------------
#define TELSOCK_TIMEOUT_DEFAULT 5   // timeout 5 sec

// chdir to HOME
//--------------
#define HOME_CHDIR_DEFAULT false

// login shell flag
//-----------------
#define ENABLE_LOGINSHELL_DEFAULT false

// ssh agent proxy
//----------------
#define ENABLE_AGENT_PROXY_DEFAULT false

// debug mode
//-----------
#define DEBUG_FLAG_DEFAULT false;
bool debug_flag = DEBUG_FLAG_DEFAULT;

// "cygterm.cfg"
static char *cfg_base;          // "cygterm.cfg"
static char *cfg_exe;           // [exe directory]/cygterm.cfg
static char *conf_appdata_full; // $APPDATA/teraterm5/cygterm.cfg
static char *sys_conf;          // /etc/cygterm.conf
static char *usr_conf;          // ~/cygtermrc  $HOME/cygtermrc

//================//
// message output //
//----------------//
// msg は ANSI文字コード (UTF8は化ける)
void msg_print(const char* msg)
{
    OutputDebugStringA(msg);
    MessageBoxA(NULL, msg, Program, MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
}

//=========================//
// Win32-API error message //
//-------------------------//
void api_error(const char* string = NULL)
{
    char msg[1024];
    char *ptr = msg;
    if (string != NULL)
        ptr += snprintf(ptr, sizeof(msg), "%s\n\n", string);
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        ptr, sizeof(msg)-(ptr-msg), NULL
    );
    msg_print(msg);
}

//=========================//
// C-runtime error message //
//-------------------------//
void c_error(const char* string = NULL)
{
    char msg[1024];
    char *ptr = msg;
    if (string != NULL)
        ptr += snprintf(ptr, sizeof(msg), "%s\n\n", string);
    snprintf(ptr, sizeof(msg)-(ptr-msg), "%s\n", strerror(errno));
    msg_print(msg);
}

//======================//
// debug message output //
//======================//
void debug_msg_print(const char* msg, ...)
{
	if (debug_flag) {
		char *tmp1;
		va_list arg;
		va_start(arg, msg);
		vasprintf(&tmp1, msg, arg);
		va_end(arg);

		char *tmp2;
		unsigned long pid = GetCurrentProcessId();
		asprintf(&tmp2, "dbg %lu: %s\n", pid, tmp1);
		OutputDebugStringA(tmp2);
		// printf("%s", tmp2);
		free(tmp2);
		free(tmp1);
	}
}

static void get_cfg_filenames()
{
	char *argv0 = GetModuleFileNameU8();
	// cfg base filename "cygterm.cfg"
	char *p = strrchr(argv0, '.');
	*p = 0;	// cut ".exe"
	p = strrchr(argv0, '/') + 1;
	cfg_base = (char *)malloc(strlen(p) + 5);
	strcpy(cfg_base, p);
	strcat(cfg_base, ".cfg");

	// exe path
	cfg_exe = (char *)malloc(strlen(argv0) + strlen(cfg_base));
	strcpy(cfg_exe, argv0);
	p = strrchr(cfg_exe, '/') + 1;
	strcpy(p, cfg_base);
	free(argv0);
	argv0 = NULL;

	// home	 $HOME/cygtermrc
	const char *home = getenv("HOME");
	usr_conf = (char *)malloc(strlen(home) + strlen(cfg_base) + 2);
	strcpy(usr_conf, home);
	strcat(usr_conf, "/.");
	strcat(usr_conf, cfg_base);
	p = strrchr(usr_conf, '.');		// ".cfg" -> "rc"
	strcpy(p, "rc");

	// system
	sys_conf = (char *)malloc(sizeof("/etc/") + strlen(cfg_base) + 2);
	strcpy(sys_conf, "/etc/");
	strcat(sys_conf, cfg_base);
	p = strrchr(sys_conf, '.');
	strcpy(p, ".conf");		// ".cfg" -> ".conf"

	// $APPDATA/teraterm5/cygterm.cfg
	char *appdata = GetAppDataDirU8();
	const char *teraterm = "/teraterm5/";
	size_t len = strlen(appdata) + strlen(teraterm) + strlen(cfg_base) + 1;
	conf_appdata_full = (char *)malloc(sizeof(char) * len);
	strcpy(conf_appdata_full, appdata);
	strcat(conf_appdata_full, teraterm);
	strcat(conf_appdata_full, cfg_base);
	free(appdata);
}

/**
 *  read /etc/passwd
 *  get user name from getlogin().  if it fails, use $USERNAME instead.
 *  and get /etc/passwd information by getpwnam(3) with user name,
 */
static void get_username_and_shell(cfg_data_t *cfg)
{
	const char* username = getlogin();
	if (username == NULL)
		username = getenv("USERNAME");
	if (username != NULL) {
		struct passwd* pw_ent = getpwnam(username);
		if (pw_ent != NULL) {
			free(cfg->shell);
			cfg->shell = strdup(pw_ent->pw_shell);
			free(cfg->username);
			cfg->username = strdup(pw_ent->pw_name);
		}
		else {
			free(cfg->username);
			cfg->username = strdup(username);
		}
	}
}

#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))

//====================//
// load configuration //
//--------------------//
static void load_cfg(cfg_data_t *cfg)
{
	// 設定ファイル読み込み順
	//		リストの上のほうから先に読み込まれる
	//		リストの下のほうから後に読み込まれる(あと勝ち)
	//		下の方が優先順位が高い
	// 通常時
	char *conf_order_normal_list[] = {
		cfg_exe,			// [exe directory]/cygterm.cfg
		sys_conf,			// /etc/cygterm.conf
		conf_appdata_full,	// $APPDATA/teraterm5/cygterm.cfg
		usr_conf			// ~/cygtermrc
	};
	const int conf_order_normal_count = (int)_countof(conf_order_normal_list);

	// ポータブル時
	char *conf_order_portable_list[] = {
		cfg_exe,			// [exe directory]/cygterm.cfg
	};
	const int conf_order_portable_count = (int)_countof(conf_order_portable_list);

	char **conf_order_list;
	int conf_order_count;
	if (IsPortableMode()) {
		// ポータブル時
		conf_order_list = conf_order_portable_list;
		conf_order_count = conf_order_portable_count;
	}
	else {
		// 通常時
		conf_order_list = conf_order_normal_list;
		conf_order_count = conf_order_normal_count;
	}

	// 実際に読み込む
	for (int i = 0; i < conf_order_count; i++) {
		const char *fname = conf_order_list[i];
		debug_msg_print("load %s", fname);
		// ignore empty configuration file path
		if (fname == NULL || strcmp(fname, "") == 0) {
			debug_msg_print("  pass");
			continue;
		}

		bool r = cfg->load(cfg, fname);
		debug_msg_print("  %s", r ? "ok" : "ng");
		cfg->dump(cfg, debug_msg_print);
	}
}

void quote_cut(char *dst, size_t len, char *src) {
	while (*src && len > 1) {
		if (*src != '"') {
			*dst++ = *src;
		}
		src++;
	}
	*dst = 0;
}

//=======================//
// commandline arguments //
//-----------------------//
void get_args(char** argv, cfg_data_t *cfg)
{
    for (++argv; *argv != NULL; ++argv) {
        if (!strcmp(*argv, "-t")) {             // -t <terminal emulator>
            if (*++argv == NULL)
                break;
            free(cfg->term);
            cfg->term = strdup(*argv);
        }
        else if (!strcmp(*argv, "-p")) {        // -p <port#>
            if (*(argv+1) != NULL) {
                ++argv;
                cfg->cl_port = atoi(*argv);
            }
        }
        else if (!strcmp(*argv, "-dumb")) {     // -dumb
            cfg->dumb = 1;
            free(cfg->term_type);
            cfg->term_type = strdup("dumb");
        }
        else if (!strcmp(*argv, "-s")) {        // -s <shell>
            if (*++argv == NULL)
                break;
            if (strcasecmp(*argv, "AUTO") != 0) {
                free(cfg->shell);
                cfg->shell = strdup(*argv);
            }
        }
        else if (!strcmp(*argv, "-cd")) {       // -cd
            cfg->home_chdir = true;
        }
        else if (!strcmp(*argv, "-nocd")) {     // -nocd
            cfg->home_chdir = false;
        }
        else if (!strcmp(*argv, "+cd")) {       // +cd
            cfg->home_chdir = false;
        }
        else if (!strcmp(*argv, "-ls")) {       // -ls
            cfg->enable_loginshell = true;
        }
        else if (!strcmp(*argv, "-nols")) {     // -nols
            cfg->enable_loginshell = false;
        }
        else if (!strcmp(*argv, "+ls")) {       // +ls
            cfg->enable_loginshell = false;
        }
        else if (!strcmp(*argv, "-A")) {       // -A
            cfg->enable_agent_proxy = true;
        }
        else if (!strcmp(*argv, "-a")) {       // -a
            cfg->enable_agent_proxy = false;
        }
        else if (!strcmp(*argv, "-v")) {        // -v <additional env var>
            if (*(argv+1) != NULL) {
                sh_env_t *sh_env = cfg->sh_env;
                ++argv;
                sh_env->add1(sh_env, *argv);
            }
        }
        else if (!strcmp(*argv, "-d")) {        // -d <exec directory>
            if (*++argv == NULL)
                break;
            char change_dir[256] = "";
            quote_cut(change_dir, sizeof(change_dir), *argv);
            cfg->change_dir = strdup(change_dir);
        }
        else if (!strcmp(*argv, "-o")) {        // -o <additional option for terminal>
            if (*++argv == NULL)
                break;
            free(cfg->termopt);
            cfg->termopt = strdup(*argv);
        }
        else if (!strcmp(*argv, "-debug")) {    // -debug
            cfg->debug_flag = true;
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

	hwnd = FindWindowA("Pageant", "Pageant");
	if (!hwnd) {
		goto agent_error;
	}

	sprintf(mapname, "PageantRequest%08x", (unsigned)GetCurrentThreadId());
	fmap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
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
	if (SendMessageA(hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds) > 0) {
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
	(void)sig;
	unlink(sockname);
	rmdir(sockdir);
	exit(0);
};

struct connList {
	int sock;
	int recvlen;
	int sendlen;
	struct connList *next;
	unsigned char ibuff[AGENT_MAX_MSGLEN];
	unsigned char obuff[AGENT_MAX_MSGLEN];
};

int proc_recvd(struct connList *conn)
{
	int reqlen, len;

	if (conn->sendlen > 0) {
		return 0;
	}

	if (conn->recvlen < 4) {
		return 0;
	}

	reqlen = get_uint32(conn->ibuff) + 4;
	if (conn->recvlen < reqlen) {
		return 0;
	}

	len = agent_request(conn->obuff, sizeof(conn->obuff), conn->ibuff);

	if (len > 0) {
		conn->sendlen = len;
	}
	else {
		set_uint32(conn->obuff, 1);
		conn->obuff[4] = 5; // SSH_AGENT_FAILURE
		conn->sendlen = 1;
	}

	if (conn->recvlen == reqlen) {
		conn->recvlen = 0;
	}
	else {
		conn->recvlen -= reqlen;
		memmove(conn->ibuff, conn->ibuff + reqlen, conn->recvlen);
	}

	return 1;
}

void agent_proxy()
{
	int sock, asock, ret;
	long len;
	struct sockaddr_un addr;
	struct connList connections, *new_conn, *prev, *cur;
	fd_set readfds, writefds, rfds, wfds;
	struct sigaction act;
	sigset_t blk;

	connections.next = NULL;

	if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		c_error("agent_proxy: socket failed.");
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

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_SET(sock, &readfds);

	while (1) {
		memcpy(&rfds, &readfds, sizeof(fd_set));
		memcpy(&wfds, &writefds, sizeof(fd_set));

		select(FD_SETSIZE, &rfds, &wfds, NULL, NULL);

		if (FD_ISSET(sock, &rfds)) {
			asock = accept(sock, NULL, NULL);
			if (asock < 0) {
				if (!(errno == EINTR || errno == ECONNABORTED)) {
					break;
				}
			}
			else {
				new_conn = (struct connList *)malloc(sizeof(struct connList));
				if (new_conn == NULL) {
					// no memory
					close(sock);
				}
				else {
					new_conn->sock = asock;
					new_conn->recvlen = 0;
					new_conn->sendlen = 0;
					new_conn->next = connections.next;
					connections.next = new_conn;
					FD_SET(asock, &readfds);
				}
			}
		}

		prev = &connections;
		for (cur=connections.next; cur != NULL; cur = cur->next) {
			if (FD_ISSET(cur->sock, &wfds)) {
				if (cur->sendlen > 0) {
					len = send(cur->sock, cur->obuff, cur->sendlen, 0);
					if (len < 0) {
						// write error
						prev->next = cur->next;
						shutdown(cur->sock, SHUT_RDWR);
						close(cur->sock);
						FD_CLR(cur->sock, &writefds);
						FD_CLR(cur->sock, &readfds);
						free(cur);
						cur = prev;
						continue;
					}
					else if (len >= cur->sendlen) {
						cur->sendlen = 0;

						sigprocmask(SIG_BLOCK, &blk, NULL);
						ret = proc_recvd(cur);
						sigprocmask(SIG_UNBLOCK, &blk, NULL);

						if (ret) {
							FD_SET(cur->sock, &writefds);
							FD_CLR(cur->sock, &readfds);
						}
						else {
							FD_CLR(cur->sock, &writefds);
							FD_SET(cur->sock, &readfds);
						}
					}
					else if (len > 0) {
						cur->sendlen -= len;
						memmove(cur->obuff, cur->obuff+len, cur->sendlen);
					}
				}
				else {
					FD_CLR(cur->sock, &writefds);
				}
			}

			if (FD_ISSET(cur->sock, &rfds)) {
				len = recv(cur->sock, cur->ibuff + cur->recvlen, sizeof(cur->ibuff) - cur->recvlen, 0);
				if (len > 0) {
					cur->recvlen += len;

					sigprocmask(SIG_BLOCK, &blk, NULL);
					ret = proc_recvd(cur);
					sigprocmask(SIG_UNBLOCK, &blk, NULL);

					if (ret) {
						FD_SET(cur->sock, &writefds);
						FD_CLR(cur->sock, &readfds);
					}
					else {
						FD_CLR(cur->sock, &writefds);
						FD_SET(cur->sock, &readfds);
					}
				}
				else if (len <= 0) {
					// read error
					prev->next = cur->next;
					shutdown(cur->sock, SHUT_RDWR);
					close(cur->sock);
					FD_CLR(cur->sock, &readfds);
					FD_CLR(cur->sock, &writefds);
					free(cur);
					cur = prev;
					continue;
				}
			}
		}
	}

agent_thread_cleanup:
	shutdown(sock, SHUT_RDWR);
	close(sock);

	unlink(sockname);
	rmdir(sockdir);

	exit(0);
}

static int exec_agent_proxy(sh_env_t *sh_env)
{
	int pid;

	if (mkdtemp(sockdir) == NULL) {
		return -1;
	}
	snprintf(sockname, sizeof(sockname), "%s/agent.%ld", sockdir, (long)getpid());

	if (!sh_env->add(sh_env, "SSH_AUTH_SOCK", sockname)) {
		return -1;
	}

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
DWORD WINAPI term_thread(LPVOID param)
{
	cfg_data_t *cfg = (cfg_data_t *)param;

	in_addr addr;
	addr.s_addr = htonl(INADDR_LOOPBACK);
	char *term;
	asprintf(&term, cfg->term, inet_ntoa(addr), (int)ntohs(listen_port));
	if (cfg->termopt != NULL) {
		char *tmp;
		asprintf(&tmp, "%s %s", tmp, cfg->termopt);
		free(term);
		term = tmp;
	}

	debug_msg_print("CreateProcess '%s'", term);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	FillMemory(&si, sizeof(si), 0);
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	DWORD flag = 0;

#if defined(UNICODE)
	wchar_t *termT = ToWcharU8(term);
#else
	char *termT = strdup(term);
#endif

	BOOL r =
		CreateProcess(
			NULL, termT, NULL, NULL, FALSE, flag, NULL, NULL, &si, &pi);
	free(termT);
	if (!r) {
		api_error(term);
		return 0;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}

//=======================================//
// thread creation for terminal emulator //
//---------------------------------------//
HANDLE exec_term(cfg_data_t *cfg)
{
    DWORD id;
    return CreateThread(NULL, 0, term_thread, cfg, 0, &id);
}

//=======================================//
// listener socket for TELNET connection //
//---------------------------------------//
int listen_telnet(u_short* port, cfg_data_t *cfg)
{
    int lsock;
    if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int i;
    for (i = 0; i < cfg->port_range; ++i) { // find an unused port#
        addr.sin_port = htons(cfg->port_start + i);
        if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            break;
        }
    }
    if (i == cfg->port_range) {
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
int accept_telnet(int lsock, cfg_data_t *cfg)
{
    fd_set rbits;
    FD_ZERO(&rbits);
    FD_SET(lsock, &rbits);
    struct timeval tm;
    tm.tv_sec = cfg->telsock_timeout;
    tm.tv_usec = 0;
    if (select(FD_SETSIZE, &rbits, 0, 0, &tm) <= 0) {
        c_error("accept_telnet: select failed");
        return -1;
    }
    if (!FD_ISSET(lsock, &rbits)) {
        c_error("accept_telnet: FD_ISSET failed");
        return -1;
    }
    int asock;
    struct sockaddr_in addr;
    int len = sizeof(addr);
    if ((asock = accept(lsock, (struct sockaddr *)&addr, &len)) < 0) {
        c_error("accept_telnet: accept failed");
        return -1;
    }
    if (getpeername(asock, (struct sockaddr *)&addr, &len) != 0) {
        c_error("accept_telnet: getpeername failed");
        shutdown(asock, 2);
        close(asock);
        return -1;
    }
    if (addr.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
        // reject it except local connection
        msg_print("not local connection");
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
static int exec_shell(int* sh_pid, cfg_data_t *cfg)
{
    // open pty master
    int master;
    if ((master = open(DEVPTY, O_RDWR)) < 0) {
        c_error("exec_shell: master pty open error");
        return -1;
    }
    int pid;
    if ((pid = fork()) < 0) {
        c_error("exec_shell: fork failed");
        return -1;
    }
    if (pid == 0) {
        // detach from control tty
        setsid();
        // open pty slave
        int slave;
        if ((slave = open(ptsname(master), O_RDWR)) < 0) {
            c_error("exec_shell: slave pty open error");
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
        if (cfg->term_type != NULL) {
            // set terminal type to $TERM
            setenv("TERM", cfg->term_type, 1);
        }
        // set other additional env vars
        int i = 0;
        sh_env_t *sh_env = cfg->sh_env;
        while(1) {
            char *value;
            const char *env = sh_env->get(sh_env, i++, &value);
            if (env == NULL) {
                break;
            }
            setenv(env, value, 1);
        }
        // change directory
        if (cfg->change_dir != NULL) {
            if (chdir(cfg->change_dir) < 0) {
                char tmp[256];
                snprintf(tmp, 256, "exec_shell: Can't chdir to \"%s\".", cfg->change_dir);
                tmp[255] = 0;
                c_error(tmp);
            }
        }
        else if (cfg->home_chdir) {
            // chdir to home directory
            const char *home_dir = getenv("HOME");
            // ignore chdir(2) system-call error.
            chdir(home_dir);
        }
        // execute a shell
        char *argv[32];
        char *cmd_shell = cfg->shell;
        get_argv(argv, 32, cmd_shell);
        cfg->shell = strdup(cmd_shell);
        if (cfg->enable_loginshell) {
            char shell_path[128];
            char *pos;
            strcpy(shell_path, argv[0]);
            if ((pos = strrchr(argv[0], '/')) != NULL) {
                *pos = '-';
                argv[0] = pos;
            }
            debug_msg_print("execv '%s' (login shell)", shell_path);
            execv(shell_path, argv);
        }
        else {
            debug_msg_print("execv '%s'", argv[0]);
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

// terminal type & size
//---------------------
char *term_type;
struct winsize win_size = {0,0,0,0};

// dumb terminal flag
//-------------------
bool dumb = false;

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
    (void)argc;
    int listen_sock = -1;
    int te_sock = -1;
    int sh_pty = -1;
    HANDLE hTerm = NULL;
    int sh_pid, agent_pid = 0;


    // configuration file (.cfg) path
    get_cfg_filenames();
    debug_msg_print("cfg_base %s", cfg_base);
    debug_msg_print("cfg_exe %s", cfg_exe);
    debug_msg_print("conf_appdata_full %s", conf_appdata_full);
    debug_msg_print("sys_conf %s", sys_conf);
    debug_msg_print("usr_conf %s", usr_conf);


    // set default values
    cfg_data_t *cfg = create_cfg();
    cfg->port_start = PORT_START_DEFAULT;
    cfg->port_range = PORT_RANGE_DEFAULT;
    cfg->telsock_timeout = TELSOCK_TIMEOUT_DEFAULT;
    cfg->home_chdir = HOME_CHDIR_DEFAULT;
    cfg->enable_loginshell = ENABLE_LOGINSHELL_DEFAULT;
    cfg->enable_agent_proxy = ENABLE_AGENT_PROXY_DEFAULT;
    cfg->debug_flag = DEBUG_FLAG_DEFAULT;

    // load configuration
    get_username_and_shell(cfg);	// from /etc/passwd
    cfg->dump(cfg, debug_msg_print);
    load_cfg(cfg);
    sh_env_t *sh_env = cfg->sh_env;
    sh_env->add(sh_env, "SHELL", cfg->shell);
    sh_env->add(sh_env, "USER", cfg->username);
    debug_msg_print("loginshell %d", cfg->enable_loginshell);
    cfg->dump(cfg, debug_msg_print);

    // read commandline arguments
    get_args(argv, cfg);
    cfg->dump(cfg, debug_msg_print);

    // restore values
    debug_flag = cfg->debug_flag;

    if (cfg->shell == NULL) {
        msg_print("missing shell");
        return 0;
    }
    if (cfg->term == NULL && cl_port <= 0) {
        msg_print("missing terminal");
        return 0;
    }

    if (cfg->change_dir != NULL) {
        cfg->home_chdir = false;
        if (cfg->enable_loginshell) {
            sh_env->add(sh_env, "CHERE_INVOKING", "y");
        }
    }

    // terminal side connection
    if (cfg->cl_port > 0) {
        // connect to the specified TCP port
        cl_port = cfg->cl_port;
        if ((te_sock = connect_client()) < 0) {
            goto cleanup;
        }
    } else {
        // prepare a TELNET listener socket
        if ((listen_sock = listen_telnet(&listen_port, cfg)) < 0) {
            goto cleanup;
        }
        debug_msg_print("execute terminal");

        // execute a terminal emulator
        if ((hTerm = exec_term(cfg)) == NULL) {
            api_error("exec_term failed");
            goto cleanup;
        }
        // accept connection from the terminal emulator
        if ((te_sock = accept_telnet(listen_sock, cfg)) < 0) {
            goto cleanup;
        }
        shutdown(listen_sock, 2);
        close(listen_sock);
        listen_sock = -1;
    }
    // TELNET negotiation
    term_type = cfg->term_type;
    dumb = cfg->dumb;
    if (!dumb) {
        telnet_nego(te_sock);
    }

    // execute ssh-agent proxy
    if (cfg->enable_agent_proxy) {
        agent_pid = exec_agent_proxy(sh_env);
    }

    // execute a shell
    debug_msg_print("execute shell");
    if ((sh_pty = exec_shell(&sh_pid, cfg)) < 0) {
        debug_msg_print("exec_shell failed");
        goto cleanup;
    }
    // set initial pty window size
    if (!dumb && c_will_naws && win_size.ws_col != 0) {
        ioctl(sh_pty, TIOCSWINSZ, &win_size);
    }

    debug_msg_print("entering telnet session");
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
// リンク時に -mwindows を指定しているので
// 実行ファイルは subsystem=windows で生成されている
// プログラムのエントリは WinMainCRTStartup() となる
// cygwinでインストールされるgcc 11.2 ではとくに指定しなくても main() がコールされる
//
// 以前のcygwinのgccでは次のコードが必要だったのかもしれない
// This program is an Win32 application but, start as Cygwin main().
//------------------------------------------------------------------
extern "C" {
    void mainCRTStartup(void);
    void WinMainCRTStartup(void) { mainCRTStartup(); }
};
#endif

//EOF
