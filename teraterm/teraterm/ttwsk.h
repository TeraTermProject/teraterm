/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
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
/* IPv6 modification is Copyright(C) 2000 Jun-ya Kato <kato@win6.jp> */

/* TERATERM.EXE, Winsock interface */
#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (WINAPI *Tclosesocket) (SOCKET s);
typedef int (WINAPI *Tconnect)
  (SOCKET s, const struct sockaddr *name, int namelen);
typedef u_long (WINAPI *Thtonl)
  (u_long hostlong);
typedef u_short (WINAPI *Thtons)
  (u_short hostshort);
typedef unsigned long (WINAPI *Tinet_addr)
  (const char * cp);
typedef int (WINAPI *Tioctlsocket)
  (SOCKET s, long cmd, u_long *argp);
typedef int (WINAPI *Trecv)
  (SOCKET s, char * buf, int len, int flags);
typedef int (WINAPI *Tselect)
  (int nfds, fd_set *readfds, fd_set *writefds,
   fd_set *exceptfds, const struct timeval *timeout);
typedef int (WINAPI *Tsend)
  (SOCKET s, const char * buf, int len, int flags);
typedef int (WINAPI *Tsetsockopt)
  (SOCKET s, int level, int optname,
   const char * optval, int optlen);
typedef SOCKET (WINAPI *Tsocket)
  (int af, int type, int protocol);
//typedef struct hostent * (WINAPI *Tgethostbyname)
//  (const char * name);
typedef int (WINAPI *TWSAStartup)
  (WORD wVersionRequired, LPWSADATA lpWSAData);
typedef int (WINAPI *TWSACleanup)(void);
typedef int (WINAPI *TWSAAsyncSelect)
  (SOCKET s, HWND hWnd, u_int wMsg,long lEvent);
typedef HANDLE (WINAPI *TWSAAsyncGetHostByName)
  (HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen);
typedef int (WINAPI *TWSACancelAsyncRequest)(HANDLE hAsyncTaskHandle);
typedef int (WINAPI *TWSAGetLastError)(void);
typedef HANDLE (WINAPI *TWSAAsyncGetAddrInfo)
  (HWND hWnd, unsigned int wMsg, const char * hostname,
   const char * portname, struct addrinfo * hints,
   struct addrinfo * * res);
// typedef int (WINAPI *Tgetaddrinfo)(const char *name, const char *port, const struct addrinfo *hints, struct addrinfo **res);
typedef void (WINAPI *Tfreeaddrinfo)(struct addrinfo *ai);

BOOL LoadWinsock();
void FreeWinsock();

extern Tclosesocket Pclosesocket;
extern Tconnect Pconnect;
extern Thtonl Phtonl;
extern Thtons Phtons;
extern Tinet_addr Pinet_addr;
extern Tioctlsocket Pioctlsocket;
extern Trecv Precv;
extern Tselect Pselect;
extern Tsend Psend;
extern Tsetsockopt Psetsockopt;
extern Tsocket Psocket;
// extern Tgethostbyname Pgethostbyname;
extern TWSAAsyncSelect PWSAAsyncSelect;
extern TWSAAsyncGetHostByName PWSAAsyncGetHostByName;
extern TWSACancelAsyncRequest PWSACancelAsyncRequest;
extern TWSAGetLastError PWSAGetLastError;
extern TWSAStartup PWSAStartup;
extern TWSACleanup PWSACleanup;
extern TWSAAsyncGetAddrInfo PWSAAsyncGetAddrInfo;
// extern Tgetaddrinfo Pgetaddrinfo;
extern Tfreeaddrinfo Pfreeaddrinfo;

#ifdef __cplusplus
}
#endif

