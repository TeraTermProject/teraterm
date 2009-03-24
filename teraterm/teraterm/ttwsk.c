/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, Winsock interface */

#include "teraterm.h"
#include "ttwsk.h"

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
#define IdHTONL 	  8
#define IdHTONS 	  9
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
    HWinsock = LoadLibrary("WSOCK32.DLL");
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
