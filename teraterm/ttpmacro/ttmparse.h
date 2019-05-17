/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2017 TeraTerm Project
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

/* TTMACRO.EXE, TTL parser */

#pragma once

#include "ttmdef.h"

#define IdTTLRun            1
#define IdTTLWait           2
#define IdTTLWaitLn         3
#define IdTTLWaitNL         4
#define IdTTLWait2          5
#define IdTTLInitDDE        6
#define IdTTLPause          7
#define IdTTLWaitCmndEnd    8
#define IdTTLWaitCmndResult 9
#define IdTTLSleep          10
#define IdTTLEnd            11
#define IdTTLWaitN          12
#define IdTTLWait4all       13

#define ErrCloseParent      1
#define ErrCantCall         2
#define ErrCantConnect      3
#define ErrCantOpen         4
#define ErrDivByZero        5
#define ErrInvalidCtl       6
#define ErrLabelAlreadyDef  7
#define ErrLabelReq         8
#define ErrLinkFirst        9
#define ErrStackOver        10
#define ErrSyntax           11
#define ErrTooManyLabels    12
#define ErrTooManyVar       13
#define ErrTypeMismatch     14
#define ErrVarNotInit       15
#define ErrCloseComment     16
#define ErrOutOfRange       17
#define ErrCloseBracket     18
#define ErrFewMemory        19
#define ErrNotSupported     20
#define ErrCantExec         21

#define TypUnknown  0
#define TypInteger  1
#define TypLogical  2
#define TypString   3
#define TypLabel    4
#define TypIntArray 5
#define TypStrArray 6

#define RsvBeep         1
#define RsvBPlusRecv    2
#define RsvBPlusSend    3
#define RsvCall         4
#define RsvChangeDir    5
#define RsvClearScreen  6
#define RsvCloseSBox    7
#define RsvCloseTT      8
#define RsvCode2Str     9
#define RsvConnect      10
#define RsvDelPassword  11
#define RsvDisconnect   12
#define RsvElse         13
#define RsvElseIf       14
#define RsvEnableKeyb   15
#define RsvEnd          16
#define RsvEndIf        17
#define RsvEndWhile     18
#define RsvExec         19
#define RsvExecCmnd     20
#define RsvExit         21
#define RsvFileClose    22
#define RsvFileConcat   23
#define RsvFileCopy     24
#define RsvFileCreate   25
#define RsvFileDelete   26
#define RsvFileMarkPtr  27
#define RsvFileOpen     28
#define RsvFileReadln   29
#define RsvFileRename   30
#define RsvFileSearch   31
#define RsvFileSeek     32
#define RsvFileSeekBack 33
#define RsvFileStrSeek  34
#define RsvFileStrSeek2 35
#define RsvFileWrite    36
#define RsvFileWriteLn  37
#define RsvFindClose    38
#define RsvFindFirst    39
#define RsvFindNext     40
#define RsvFlushRecv    41
#define RsvFor          42
#define RsvGetDate      43
#define RsvGetDir       44
#define RsvGetEnv       45
#define RsvGetPassword  46
#define RsvGetTime      47
#define RsvGetTitle     48
#define RsvGoto         49
#define RsvIf           50
#define RsvInclude      51
#define RsvInputBox     52
#define RsvInt2Str      53
#define RsvKmtFinish    54
#define RsvKmtGet       55
#define RsvKmtRecv      56
#define RsvKmtSend      57
#define RsvLoadKeyMap   58
#define RsvLogClose     59
#define RsvLogOpen      60
#define RsvLogPause     61
#define RsvLogStart     62
#define RsvLogWrite     63
#define RsvMakePath     64
#define RsvMessageBox   65
#define RsvNext         66
#define RsvPasswordBox  67
#define RsvPause        68
#define RsvQuickVANRecv 69
#define RsvQuickVANSend 70
#define RsvRecvLn       71
#define RsvRestoreSetup 72
#define RsvReturn       73
#define RsvSend         74
#define RsvSendBreak    75
#define RsvSendFile     76
#define RsvSendKCode    77
#define RsvSendLn       78
#define RsvSetDate      79
#define RsvSetDir       80
#define RsvSetDlgPos    81
#define RsvSetEcho      82
#define RsvSetExitCode  83
#define RsvSetSync      84
#define RsvSetTime      85
#define RsvSetTitle     86
#define RsvShow         87
#define RsvShowTT       88
#define RsvStatusBox    89
#define RsvStr2Code     90
#define RsvStr2Int      91
#define RsvStrCompare   92
#define RsvStrConcat    93
#define RsvStrCopy      94
#define RsvStrLen       95
#define RsvStrScan      96
#define RsvTestLink     97
#define RsvThen         98
#define RsvUnlink       99
#define RsvWait         100
#define RsvWaitEvent    101
#define RsvWaitLn       102
#define RsvWaitRecv     103
#define RsvWhile        104
#define RsvXmodemRecv   105
#define RsvXmodemSend   106
#define RsvYesNoBox     107
#define RsvZmodemRecv   108
#define RsvZmodemSend   109
#define RsvWaitRegex    110   // add 'waitregex' (2005.10.5 yutaka)
#define RsvMilliPause   111   // add 'mpause' (2006.2.10 yutaka)
#define RsvRandom       112   // add 'random' (2006.2.11 yutaka)
#define RsvClipb2Var    113   // add 'clipb2var' (2006.9.17 maya)
#define RsvVar2Clipb    114   // add 'var2clipb' (2006.9.17 maya)
#define RsvIfDefined    115   // add 'ifdefined' (2006.9.23 maya)
#define RsvFileRead     116   // add 'fileread' (2006.11.1 yutaka)
#define RsvSprintf      117   // add 'sprintf' (2007.5.1 yutaka)
#define RsvToLower      118   // add 'tolower' (2007.7.12 maya)
#define RsvToUpper      119   // add 'toupper' (2007.7.12 maya)
#define RsvBreak        120   // add 'break' (2007.7.20 doda)
#define RsvRotateR      121   // add 'rotateright' (2007.8.19 maya)
#define RsvRotateL      122   // add 'rotateleft' (2007.8.19 maya)
#define RsvSetEnv       123   // reactivate 'setenv' (2007.8.31 maya)
#define RsvFilenameBox  124   // add 'filenamebox' (2007.9.13 maya)
#define RsvCallMenu     125   // add 'callmenu' (2007.11.18 maya)
#define RsvDo           126   // add 'do' (2007.11.20 doda)
#define RsvLoop         127   // add 'loop' (2007.11.20 doda)
#define RsvUntil        128   // add 'until' (2007.11.20 doda)
#define RsvEndUntil     129   // add 'enduntil' (2007.11.20 doda)
#define RsvCygConnect   130   // add 'cygconnect' (2007.12.17 doda)
#define RsvScpRecv      131   // add 'scprecv' (2008.1.1 yutaka)
#define RsvScpSend      132   // add 'scpsend' (2008.1.1 yutaka)
#define RsvGetVer       133   // add 'getver'  (2008.2.4 yutaka)
#define RsvSetBaud      134   // add 'setbaud' (2008.2.13 yutaka)
#define RsvStrMatch     135   // add 'strmatch' (2008.3.26 yutaka)
#define RsvSetRts       136   // add 'setrts'  (2008.3.12 maya)
#define RsvSetDtr       137   // add 'setdtr'  (2008.3.12 maya)
#define RsvCrc32        138   // add 'crc32'  (2008.9.12 yutaka)
#define RsvCrc32File    139   // add 'crc32file'  (2008.9.13 yutaka)
#define RsvGetTTDir     140   // add 'getttdir'  (2008.9.20 maya)
#define RsvGetHostname  141   // add 'gethostname'  (2008.12.15 maya)
#define RsvSprintf2     142   // add 'sprintf2'  (2008.12.18 maya)
#define RsvWaitN        143   // add 'waitn'  (2009.1.26 maya)
#define RsvSendBroadcast    144
#define RsvSendMulticast    145
#define RsvSetMulticastName 146
#define RsvSendlnBroadcast  147
#define RsvWait4all     148
#define RsvDispStr      149
#define RsvIntDim       150
#define RsvStrDim       151
#define RsvLogInfo      152
#define RsvFileLock     153
#define RsvFileUnLock   154
#define RsvContinue     155
#define RsvRegexOption  156
#define RsvSendlnMulticast 157

#define RsvSetDebug     175
#define RsvYmodemRecv   176
#define RsvYmodemSend   177
#define RsvFileStat     178
#define RsvFileTruncate 179
#define RsvStrInsert    180
#define RsvStrRemove    181
#define RsvStrReplace   182
#define RsvStrTrim      183
#define RsvStrSplit     184
#define RsvStrJoin      185
#define RsvStrSpecial   186
#define RsvBasename     187
#define RsvDirname      188
#define RsvGetFileAttr  189
#define RsvSetFileAttr  190
#define RsvFolderCreate 191
#define RsvFolderDelete 192
#define RsvFolderSearch 193
#define RsvExpandEnv    194
#define RsvGetSpecialFolder  195
#define RsvSetPassword  196
#define RsvIsPassword   197
#define RsvListBox      198
#define RsvGetIPv4Addr  199
#define RsvGetIPv6Addr  200
#define RsvLogRotate    201
#define RsvCrc16        202
#define RsvCrc16File    203
#define RsvChecksum8    204
#define RsvChecksum8File    205
#define RsvChecksum16   206
#define RsvChecksum16File   207
#define RsvChecksum32   208
#define RsvChecksum32File   209
#define RsvBringupBox   210
#define RsvLogAutoClose 211
#define RsvUptime		212
#define RsvGetModemStatus	213
#define RsvDirnameBox   214
#define RsvSetFlowCtrl   215

#define RsvOperator     1000
#define RsvBNot         1001
#define RsvBAnd         1002
#define RsvBOr          1003
#define RsvBXor         1004
#define RsvMul          1005
#define RsvPlus         1006
#define RsvMinus        1007
#define RsvDiv          1008
#define RsvMod          1009
#define RsvLT           1010
#define RsvEQ           1011
#define RsvGT           1012
#define RsvLE           1013
#define RsvNE           1014
#define RsvGE           1015
#define RsvLNot         1016
#define RsvLAnd         1017
#define RsvLOr          1018
#define RsvLXor         1019
#define RsvARShift      1020 // arithmetic right shift
#define RsvALShift      1021 // arithmetic left shift
#define RsvLRShift      1022 // logical right shift


// integer type for buffer pointer
typedef DWORD BINT;

typedef char TName[MaxNameLen];
typedef TName far *PName;
typedef char TStrVal [MaxStrLen];
typedef TStrVal far *PStrVal;

typedef DWORD TVarId;
typedef TVarId far *PVarId;

#ifdef __cplusplus
extern "C" {
#endif

BOOL InitVar();
void EndVar();
void DispErr(WORD Err);
void LockVar();
void UnlockVar();
int IsCommentClosed(void);
BYTE GetFirstChar();
BOOL CheckParameterGiven();
BOOL GetIdentifier(PCHAR Name);
BOOL GetReservedWord(LPWORD WordId);
BOOL CheckReservedWord(PCHAR Str, LPWORD WordId);
BOOL GetLabelName(PCHAR Name);
BOOL GetString(PCHAR Str, LPWORD Err);
BOOL CheckVar(PCHAR Name, LPWORD VarType, PVarId VarId);
BOOL NewIntVar(PCHAR Name, int InitVal);
BOOL NewStrVar(PCHAR Name, PCHAR InitVal);
BOOL NewLabVar(PCHAR Name, BINT InitVal, WORD ILevel);
int NewIntAryVar(PCHAR Name, int size);
int NewStrAryVar(PCHAR Name, int size);
void DelLabVar(WORD ILevel);
void CopyLabel(WORD ILabel, BINT far *Ptr, LPWORD Level);
BOOL GetExpression(LPWORD ValType, int far *Val, LPWORD Err);
void GetIntVal(int far *Val, LPWORD Err);
void SetIntVal(TVarId VarId, int Val);
int CopyIntVal(TVarId VarId);
void GetIntVar(PVarId VarId, LPWORD Err);
void GetStrVal(PCHAR Str, LPWORD Err);
void GetStrVal2(PCHAR Str, LPWORD Err, BOOL AutoConversion);
void GetStrVar(PVarId VarId, LPWORD Err);
void SetStrVal(TVarId VarId, PCHAR Str);
PCHAR StrVarPtr(TVarId VarId);
void GetVarType(LPWORD ValType, int far *Val, LPWORD Err);
TVarId GetIntVarFromArray(TVarId VarId, int Index, LPWORD Err);
TVarId GetStrVarFromArray(TVarId VarId, int Index, LPWORD Err);
BOOL GetIndex(int *Index, LPWORD Err);
void GetAryVar(PVarId VarId, WORD VarType, LPWORD Err);
void GetAryVarByName(PVarId VarId, PCHAR Name, WORD VarType, LPWORD Err);
void SetIntValInArray(TVarId VarId, int Index, int Val, LPWORD Err);
void SetStrValInArray(TVarId VarId, int Index, PCHAR Str, LPWORD Err);
int GetIntAryVarSize(TVarId VarId);
int GetStrAryVarSize(TVarId VarId);

extern WORD TTLStatus;
extern char LineBuff[MaxLineLen];
extern WORD LinePtr;
extern WORD LineLen;
extern WORD LineParsePtr;

#ifdef __cplusplus
}
#endif

#define GetIntAryVar(VarId, Err)		GetAryVar(VarId, TypIntArray, Err)
#define GetStrAryVar(VarId, Err)		GetAryVar(VarId, TypStrArray, Err)
#define GetIntAryVarByName(VarId, Name, Err)	GetAryVarByName(VarId, Name, TypIntArray, Err)
#define GetStrAryVarByName(VarId, Name, Err)	GetAryVarByName(VarId, Name, TypStrArray, Err)
