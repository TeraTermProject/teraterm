/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TELNET routines */

#define TEL_EOF	236
#define SUSP	237
#define ABORT	238

#define SE	240
#define NOP	241
#define DM	242
#define BREAK	243
#define IP	244
#define AO	245
#define AYT	246
#define EC	247
#define EL	248
#define GOAHEAD	249
#define SB	250
#define WILLTEL	251
#define WONTTEL	252
#define DOTEL	253
#define DONTTEL	254
#define IAC	255

#define BINARY     0
#define ECHO	   1
#define RECONNECT  2
#define SGA 	   3
#define AMSN	   4
#define STATUS     5
#define TIMING     6
#define RCTAN	   7
#define OLW	   8
#define OPS	   9
#define OCRD	   10
#define OHTS	   11
#define OHTD	   12
#define OFFD	   13
#define OVTS	   14
#define OVTD	   15
#define OLFD	   16
#define XASCII     17
#define LOGOUT     18
#define BYTEM	   19
#define DET	   20
#define SUPDUP     21
#define SUPDUPOUT  22
#define SENDLOC    23
#define TERMTYPE   24
#define EOR	   25
#define TACACSUID  26
#define OUTPUTMARK 27
#define TERMLOCNUM 28
#define REGIME3270 29
#define X3PAD	   30
#define NAWS	   31
#define TERMSPEED  32
#define TFLOWCNTRL 33
#define LINEMODE   34
#define MaxTelOpt  34

  /* Telnet status */
#define TelIdle    0
#define TelIAC     1
#define TelSB      2
#define TelWill    3
#define TelWont    4
#define TelDo      5
#define TelDont    6
#define TelNop     7

#ifdef __cplusplus
extern "C" {
#endif

void InitTelnet();
void EndTelnet();
void ParseTel(BOOL *Size, int *Nx, int *Ny);
void TelEnableHisOpt(BYTE b);
void TelEnableMyOpt(BYTE b);
void TelInformWinSize(int nx, int ny);
void TelSendAYT();
void TelSendBreak();
void TelChangeEcho();
void TelSendNOP();
void TelStartKeepAliveThread();
void TelStopKeepAliveThread();
void TelUpdateKeepAliveInterval();

extern int TelStatus;

#ifdef __cplusplus
}
#endif
