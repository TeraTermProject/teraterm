/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, Tera Term Language interpreter */

#define IdTimeOutTimer 1
#define TIMEOUT_TIMER_MS 50

// wakeup condition for sleep state
#define IdWakeupTimeout 1
#define IdWakeupUnlink  2
#define IdWakeupDisconn 4
#define IdWakeupConnect 8
/* connection trial state */
#define IdWakeupInit    16

#ifdef __cplusplus
extern "C" {
#endif

BOOL InitTTL(HWND HWin);
void EndTTL();
void Exec();
void SetMatchStr(PCHAR Str);
void SetGroupMatchStr(int no, PCHAR Str);
void SetInputStr(PCHAR Str);
void SetResult(int ResultCode);
BOOL CheckTimeout();
BOOL TestWakeup(int Wakeup);
void SetWakeup(int Wakeup);

// exit code of TTMACRO
extern int ExitCode;

#ifdef __cplusplus
}
#endif
