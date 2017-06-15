/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

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
#define SGA	   3
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
