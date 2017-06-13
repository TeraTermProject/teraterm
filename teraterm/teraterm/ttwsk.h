/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */
/* IPv6 modification is Copyright(C) 2000 Jun-ya Kato <kato@win6.jp> */

/* TERATERM.EXE, Winsock interface */
#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (PASCAL *Tclosesocket) (SOCKET s);
typedef int (PASCAL *Tconnect)
  (SOCKET s, const struct sockaddr *name, int namelen);
typedef u_long (PASCAL *Thtonl)
  (u_long hostlong);
typedef u_short (PASCAL *Thtons)
  (u_short hostshort);
typedef unsigned long (PASCAL *Tinet_addr)
  (const char * cp);
typedef int (PASCAL *Tioctlsocket)
  (SOCKET s, long cmd, u_long *argp);
typedef int (PASCAL *Trecv)
  (SOCKET s, char * buf, int len, int flags);
typedef int (PASCAL *Tselect)
  (int nfds, fd_set *readfds, fd_set *writefds,
   fd_set *exceptfds, const struct timeval *timeout);
typedef int (PASCAL *Tsend)
  (SOCKET s, const char * buf, int len, int flags);
typedef int (PASCAL *Tsetsockopt)
  (SOCKET s, int level, int optname,
   const char * optval, int optlen);
typedef SOCKET (PASCAL *Tsocket)
  (int af, int type, int protocol);
//typedef struct hostent * (PASCAL *Tgethostbyname)
//  (const char * name);
typedef int (PASCAL *TWSAStartup)
  (WORD wVersionRequired, LPWSADATA lpWSAData);
typedef int (PASCAL *TWSACleanup)(void);
typedef int (PASCAL *TWSAAsyncSelect)
  (SOCKET s, HWND hWnd, u_int wMsg,long lEvent);
typedef HANDLE (PASCAL *TWSAAsyncGetHostByName)
  (HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen);
typedef int (PASCAL *TWSACancelAsyncRequest)(HANDLE hAsyncTaskHandle);
typedef int (PASCAL *TWSAGetLastError)(void);
typedef HANDLE (PASCAL *TWSAAsyncGetAddrInfo)
  (HWND hWnd, unsigned int wMsg, const char * hostname,
   const char * portname, struct addrinfo * hints,
   struct addrinfo * * res);
// typedef int (PASCAL *Tgetaddrinfo)(const char *name, const char *port, const struct addrinfo *hints, struct addrinfo **res);
typedef void (PASCAL *Tfreeaddrinfo)(struct addrinfo *ai);

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

