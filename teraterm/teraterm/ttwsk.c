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

/* TERATERM.EXE, Winsock interface */

#include "teraterm.h"
#include "ttwsk.h"
#include "WSAAsyncGetAddrInfo.h"

static HANDLE HWinsock = NULL;

Tclosesocket Pclosesocket;
Tconnect Pconnect;
Thtonl Phtonl;
Thtons Phtons;
Tinet_addr Pinet_addr;
Tioctlsocket Pioctlsocket;
Trecv Precv;
Tselect Pselect;
Tsend Psend;
Tsetsockopt Psetsockopt;
Tsocket Psocket;
// Tgethostbyname Pgethostbyname;
TWSAAsyncSelect PWSAAsyncSelect;
TWSAAsyncGetHostByName PWSAAsyncGetHostByName;
TWSACancelAsyncRequest PWSACancelAsyncRequest;
TWSAGetLastError PWSAGetLastError;
TWSAStartup PWSAStartup;
TWSACleanup PWSACleanup;
// Tgetaddrinfo Pgetaddrinfo;
Tfreeaddrinfo Pfreeaddrinfo;
TWSAAsyncGetAddrInfo PWSAAsyncGetAddrInfo;

void CheckWinsock()
{
  WORD wVersionRequired;
  WSADATA WSData;

  if (HWinsock == NULL) return;
#if 0
  wVersionRequired = 1*256+1;
  if ((PWSAStartup(wVersionRequired, &WSData) != 0) ||
      (LOBYTE(WSData.wVersion) != 1) ||
      (HIBYTE(WSData.wVersion) != 1))
  {
    PWSACleanup();
    FreeLibrary(HWinsock);
    HWinsock = NULL;
  }
#else
  wVersionRequired = MAKEWORD(2, 2);
  if ((PWSAStartup(wVersionRequired, &WSData) != 0) ||
      (LOBYTE(WSData.wVersion) != 2) ||
      (HIBYTE(WSData.wVersion) != 2))
  {
    PWSACleanup();
    FreeLibrary(HWinsock);
    HWinsock = NULL;
  }
#endif
}

#define IdCLOSESOCKET	  3
#define IdCONNECT	  4
#define IdHTONL		  8
#define IdHTONS		  9
#define IdINET_ADDR	  10
#define IdIOCTLSOCKET	  12
#define IdRECV		  16
#define IdSELECT	  18
#define IdSEND		  19
#define IdSETSOCKOPT	  21
#define IdSOCKET	  23
// #define IdGETHOSTBYNAME   52
#define IdWSAASYNCSELECT  101
#define IdWSAASYNCGETHOSTBYNAME 103
#define IdWSACANCELASYNCREQUEST 108
#define IdWSAGETLASTERROR 111
#define IdWSASTARTUP	  115
#define IdWSACLEANUP	  116

BOOL LoadWinsock()
{
  BOOL Err;

  if (HWinsock == NULL)
  {
    char wsock32_dll[MAX_PATH];

    GetSystemDirectory(wsock32_dll, sizeof(wsock32_dll));
    strncat_s(wsock32_dll, sizeof(wsock32_dll), "\\wsock32.dll", _TRUNCATE);
    HWinsock = LoadLibrary(wsock32_dll);
    if (HWinsock == NULL) return FALSE;

    Err = FALSE;

    Pclosesocket = (Tclosesocket)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdCLOSESOCKET));
    if (Pclosesocket==NULL) Err = TRUE;

    Pconnect = (Tconnect)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdCONNECT));
    if (Pconnect==NULL) Err = TRUE;

    Phtonl = (Thtonl)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdHTONL));
    if (Phtonl==NULL) Err = TRUE;

    Phtons = (Thtons)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdHTONS));
    if (Phtons==NULL) Err = TRUE;

    Pinet_addr = (Tinet_addr)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdINET_ADDR));
    if (Pinet_addr==NULL) Err = TRUE;

    Pioctlsocket = (Tioctlsocket)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdIOCTLSOCKET));
    if (Pioctlsocket==NULL) Err = TRUE;

    Precv = (Trecv)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdRECV));
    if (Precv==NULL) Err = TRUE;

    Pselect = (Tselect)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdSELECT));
    if (Pselect==NULL) Err = TRUE;

    Psend = (Tsend)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdSEND));
    if (Psend==NULL) Err = TRUE;

    Psetsockopt = (Tsetsockopt)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdSETSOCKOPT));
    if (Psetsockopt==NULL) Err = TRUE;

    Psocket = (Tsocket)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdSOCKET));
    if (Psocket==NULL) Err = TRUE;

//    Pgethostbyname = (Tgethostbyname)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdGETHOSTBYNAME));
//    if (Pgethostbyname==NULL) Err = TRUE;

    PWSAAsyncSelect = (TWSAAsyncSelect)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdWSAASYNCSELECT));
    if (PWSAAsyncSelect==NULL) Err = TRUE;

    PWSAAsyncGetHostByName =
      (TWSAAsyncGetHostByName)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdWSAASYNCGETHOSTBYNAME));
    if (PWSAAsyncSelect==NULL) Err = TRUE;

    PWSACancelAsyncRequest = (TWSACancelAsyncRequest)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdWSACANCELASYNCREQUEST));
    if (PWSACancelAsyncRequest==NULL) Err = TRUE;

    PWSAGetLastError = (TWSAGetLastError)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdWSAGETLASTERROR));
    if (PWSAGetLastError==NULL) Err = TRUE;

    PWSAStartup = (TWSAStartup)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdWSASTARTUP));
    if (PWSAStartup==NULL) Err = TRUE;

    PWSACleanup = (TWSACleanup)GetProcAddress(HWinsock, MAKEINTRESOURCE(IdWSACLEANUP));
    if (PWSACleanup==NULL) Err = TRUE;

//    Pgetaddrinfo = (Tgetaddrinfo)GetProcAddress(HWinsock, "getaddrinfo");
//    if (Pgetaddrinfo==NULL) Err = TRUE;

    Pfreeaddrinfo = freeaddrinfo;
    PWSAAsyncGetAddrInfo = WSAAsyncGetAddrInfo;

    if (Err)
    {
      FreeLibrary(HWinsock);
      HWinsock = NULL;
      return FALSE;
    }
  }

  CheckWinsock();

  return (HWinsock != NULL);
}

void FreeWinsock()
{
  HANDLE HTemp;

  if (HWinsock == NULL) return;
  HTemp = HWinsock;
  HWinsock = NULL;
  PWSACleanup();
  Sleep(50); // for safety
  FreeLibrary(HTemp);
}
