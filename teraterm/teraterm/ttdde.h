/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, DDE routines */
#include <ddeml.h>

#ifdef __cplusplus
extern "C" {
#endif

void SetTopic();
BOOL InitDDE();
void SendDDEReady();
void EndDDE();
void DDEAdv();
void EndDdeCmnd(int Result);
void SetDdeComReady(WORD Ready);
void RunMacro(PCHAR FName, BOOL Startup);

extern char TopicName[21];
extern HCONV ConvH;
extern BOOL AdvFlag;
extern BOOL CloseTT;
extern HWND HWndLog; //steven add 

#ifdef __cplusplus
}
#endif

