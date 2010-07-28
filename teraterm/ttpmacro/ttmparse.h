/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, TTL parser */

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

#define TypUnknown  0
#define TypInteger  1
#define TypLogical  2
#define TypString   3
#define TypLabel    4

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
#define RsvWaitInt      149
#define RsvSendInt      150
#define RsvSetDebug     151
#define RsvYmodemRecv   152
#define RsvYmodemSend   153
#define RsvFileStat     154
#define RsvFileTruncate 155

#define RsvOperator     200
#define RsvBNot         201
#define RsvBAnd         202
#define RsvBOr          203
#define RsvBXor         204
#define RsvMul          205
#define RsvPlus         206
#define RsvMinus        207
#define RsvDiv          208
#define RsvMod          209
#define RsvLT           210
#define RsvEQ           211
#define RsvGT           212
#define RsvLE           213
#define RsvNE           214
#define RsvGE           215
#define RsvLNot         216
#define RsvLAnd         217
#define RsvLOr          218
#define RsvLXor         219
#define RsvARShift      220 // arithmetic right shift
#define RsvALShift      221 // arithmetic left shift
#define RsvLRShift      222 // logical right shift


// integer type for buffer pointer
typedef DWORD BINT;

#define MaxNameLen (LONG)32
#define MaxStrLen (LONG)256
#define MaxLineLen (LONG)501

#define INT_BIT (CHAR_BIT * sizeof(int))

typedef char TName[MaxNameLen];
typedef TName far *PName;
typedef char TStrVal [MaxStrLen];
typedef TStrVal far *PStrVal;

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
BOOL GetLabelName(PCHAR Name);
BOOL GetString(PCHAR Str, LPWORD Err);
BOOL CheckVar(PCHAR Name, LPWORD VarType, LPWORD VarId);
BOOL NewIntVar(PCHAR Name, int InitVal);
BOOL NewStrVar(PCHAR Name, PCHAR InitVal);
BOOL NewLabVar(PCHAR Name, BINT InitVal, WORD ILevel);
void DelLabVar(WORD ILevel);
void CopyLabel(WORD ILabel, BINT far *Ptr, LPWORD Level);
BOOL GetExpression(LPWORD ValType, int far *Val, LPWORD Err);
void GetIntVal(int far *Val, LPWORD Err);
void SetIntVal(WORD VarId, int Val);
int CopyIntVal(WORD VarId);
void GetIntVar(LPWORD VarId, LPWORD Err);
void GetStrVal(PCHAR Str, LPWORD Err);
void GetStrVal2(PCHAR Str, LPWORD Err, BOOL AutoConversion);
void GetStrVar(LPWORD VarId, LPWORD Err);
void SetStrVal(WORD VarId, PCHAR Str);
PCHAR StrVarPtr(WORD VarId);
void GetVarType(LPWORD ValType, int far *Val, LPWORD Err);

extern WORD TTLStatus;
extern char LineBuff[MaxLineLen];
extern WORD LinePtr;
extern WORD LineLen;

#ifdef __cplusplus
}
#endif
