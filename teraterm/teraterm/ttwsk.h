/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */
/* IPv6 modification is Copyright(C) 2000 Jun-ya Kato <kato@win6.jp> */

/* TERATERM.EXE, Winsock interface */
#ifndef NO_INET6
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <winsock.h>
#endif /* NO_INET6 */

#ifdef __cplusplus
extern "C" {
#endif

typedef int (PASCAL FAR *Tclosesocket) (SOCKET s);
typedef int (PASCAL FAR *Tconnect)
  (SOCKET s, const struct sockaddr FAR *name, int namelen);
typedef u_long (PASCAL FAR *Thtonl)
  (u_long hostlong);
typedef u_short (PASCAL FAR *Thtons)
  (u_short hostshort);
typedef unsigned long (PASCAL FAR *Tinet_addr)
  (const char FAR * cp);
typedef int (PASCAL FAR *Tioctlsocket)
  (SOCKET s, long cmd, u_long FAR *argp);
typedef int (PASCAL FAR *Trecv)
  (SOCKET s, char FAR * buf, int len, int flags);
typedef int (PASCAL FAR *Tselect)
  (int nfds, fd_set FAR *readfds, fd_set FAR *writefds,
   fd_set FAR *exceptfds, const struct timeval FAR *timeout);
typedef int (PASCAL FAR *Tsend)
  (SOCKET s, const char FAR * buf, int len, int flags);
typedef int (PASCAL FAR *Tsetsockopt)
  (SOCKET s, int level, int optname,
   const char FAR * optval, int optlen);
typedef SOCKET (PASCAL FAR *Tsocket)
  (int af, int type, int protocol);
//typedef struct hostent FAR * (PASCAL FAR *Tgethostbyname)
//  (const char FAR * name);
typedef int (PASCAL FAR *TWSAStartup)
  (WORD wVersionRequired, LPWSADATA lpWSAData);
typedef int (PASCAL FAR *TWSACleanup)(void);
typedef int (PASCAL FAR *TWSAAsyncSelect)
  (SOCKET s, HWND hWnd, u_int wMsg,long lEvent);
typedef HANDLE (PASCAL FAR *TWSAAsyncGetHostByName)
  (HWND hWnd, u_int wMsg, const char FAR * name, char FAR * buf, int buflen);
typedef int (PASCAL FAR *TWSACancelAsyncRequest)(HANDLE hAsyncTaskHandle);
typedef int (PASCAL FAR *TWSAGetLastError)(void);

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

#ifdef __cplusplus
}
#endif

