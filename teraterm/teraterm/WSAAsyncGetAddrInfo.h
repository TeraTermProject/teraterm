/*
 * WSAAsyncGetAddrInfo.h -- declarations for WSAAsyncGetAddrInfo()
 * Copyright(C) 2000-2003 Jun-ya Kato <kato@win6.jp>
 */
#ifndef __WSAASYNCGETADDRINFO__
#define __WSAASYNCGETADDRINFO__

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

struct getaddrinfo_args {
  HWND hWnd;
  unsigned int wMsg;
  char *hostname;
  char *portname;
  struct addrinfo hints;
  struct addrinfo **res;
  HANDLE *lpHandle;
};

HANDLE PASCAL WSAAsyncGetAddrInfo(HWND hWnd,
			   unsigned int wMsg,
			   const char *hostname,
			   const char *portname,
			   struct addrinfo *hints,
			   struct addrinfo **res);

#endif /* __WSAASYNCGETADDRINFO__ */
