/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2017 TeraTerm Project
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

/* TTMACRO.EXE, DDE routines */

#define CmdSetHWnd      ' '
#define CmdSetFile      '!'
#define CmdSetBinary    '"'
#define CmdSetAppend    '#'
#define CmdSetXmodemOpt '$'
#define CmdSetSync      '%'

#define CmdBPlusRecv    '&'
#define CmdBPlusSend    '\''
#define CmdChangeDir    '('
#define CmdClearScreen  ')'
#define CmdCloseWin     '*'
#define CmdConnect      '+'
#define CmdDisconnect   ','
#define CmdEnableKeyb   '-'
#define CmdGetTitle     '.'
#define CmdInit         '/'
#define CmdKmtFinish    '0'
#define CmdKmtGet       '1'
#define CmdKmtRecv      '2'
#define CmdKmtSend      '3'
#define CmdLoadKeyMap   '4'
#define CmdLogClose     '5'
#define CmdLogOpen      '6'
#define CmdLogPause     '7'
#define CmdLogStart     '8'
#define CmdLogWrite     '9'
#define CmdQVRecv       ':'
#define CmdQVSend       ';'
#define CmdRestoreSetup '<'
#define CmdSendBreak    '='
#define CmdSendFile     '>'
#define CmdSendKCode    '?'
#define CmdSetEcho      '@'
#define CmdSetTitle     'A'
#define CmdShowTT       'B'
#define CmdXmodemSend   'C'
#define CmdXmodemRecv   'D'
#define CmdZmodemSend   'E'
#define CmdZmodemRecv   'F'
#define CmdCallMenu     'G'
#define CmdScpSend      'H'
#define CmdScpRcv       'I'
#define CmdSetSecondFile 'J'
#define CmdSetBaud      'K'
#define CmdSetRts       'L'
#define CmdSetDtr       'M'
#define CmdGetHostname  'N'
#define CmdSendBroadcast 'O'
#define CmdSendMulticast 'P'
#define CmdSetMulticastName 'Q'
#define CmdSetDebug     'R'
#define CmdYmodemSend   'S'
#define CmdYmodemRecv   'T'
#define CmdDispStr      'U'
#define CmdLogInfo      'V'
#define CmdLogRotate    'W'
#define CmdLogAutoClose 'X'
#define CmdGetModemStatus 'Y'
#define CmdSetFlowCtrl  'Z'

#ifdef __cplusplus
extern "C" {
#endif

void Word2HexStr(WORD w, PCHAR HexStr);
BOOL InitDDE(HWND HWin);
void EndDDE();
void DDEOut1Byte(BYTE B);
void DDEOut(PCHAR B);
void DDESend();
PCHAR GetRecvLnBuff();
void FlushRecv();
void ClearWait();
void SetWait(int Index, PCHAR Str);
void ClearWaitN();
void SetWaitN(int Len);
int CmpWait(int Index, PCHAR Str);
void SetWait2(PCHAR Str, int Len, int Pos);
int Wait();
BOOL Wait2();
BOOL WaitN();
int Wait4all();
void SetFile(PCHAR FN);
void SetSecondFile(PCHAR FN);
void SetBinary(int BinFlag);
void SetDebug(int DebugFlag);
void SetAppend(int AppendFlag);
void SetXOption(int XOption);
void SendSync();
void SetSync(BOOL OnFlag);
WORD SendCmnd(char OpId, int WaitFlag);
WORD GetTTParam(char OpId, PCHAR Param, int destlen);
int FindRegexStringOne(char *regex, int regex_len, char *target, int target_len);

extern BOOL Linked;
extern WORD ComReady;
extern int OutLen;

  // for "WaitRecv" command
extern TStrVal Wait2Str;
extern BOOL    Wait2Found;

enum regex_type {
	REGEX_NONE,
	REGEX_WAIT,
	REGEX_WAITLN,
	REGEX_WAITRECV,
	REGEX_WAITEVENT,
};
extern enum regex_type RegexActionType;

extern BOOL Wait4allGotIndex;
extern int Wait4allFoundNum;


#ifdef __cplusplus
}
#endif
