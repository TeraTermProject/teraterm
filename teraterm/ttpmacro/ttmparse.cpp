/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2020 TeraTerm Project
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

// TTMACRO.EXE, TTL parser

#include "teraterm.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include "ttmdlg.h"
#include "ttmparse.h"
#include "ttmbuff.h"

/* C言語スタイルのコメントをサポートするかどうか (2009.7.2 yutaka) */
#define SUPPORT_C_STYLE_COMMENT
static int commenting = 0;   /* C言語コメント */

WORD TTLStatus = 0;
char LineBuff[MaxLineLen]; // 行バッファのサイズを拡張した。(2007.6.9 maya)
WORD LinePtr;
WORD LineLen;
WORD LineParsePtr;  // トークンの解析を開始した位置 (2013.6.23 yutaka)

typedef struct {
    int size;
    int *val;
} TIntAry, *PIntAry;

typedef struct {
    int size;
//    PStrVal val;
    char **val;
} TStrAry, *PStrAry;

typedef struct {
	BINT val;
	WORD level;
} TLab;

typedef enum {
	TypeUnknown = TypUnknown,
	TypeInteger = TypInteger,
	//TypeLogical = TypLogical,
	TypeString = TypString,
	TypeLabel = TypLabel,
	TypeIntArray = TypIntArray,
	TypeStrArray = TypStrArray,
} VariableType_t;

typedef struct {
	char *Name;
	VariableType_t Type;
	union {
		char *Str;
		int Int;
		TLab Lab;
		TIntAry IntAry;
		TStrAry StrAry;
	} Value;
} Variable_t;

static Variable_t *Variables;
static int VariableCount;

// トークンの解析開始位置を更新する。
static void UpdateLineParsePtr(void)
{
	LineParsePtr = LinePtr;
}


BOOL InitVar()
{
	Variables = NULL;
	VariableCount = 0;
	return TRUE;
}

void EndVar()
{
	Variable_t *v = Variables;
	for (;v != &Variables[VariableCount]; v++) {
		free(v->Name);
		switch (v->Type) {
		case TypeString:
			free(v->Value.Str);
			break;
		case TypeIntArray:
			free(v->Value.IntAry.val);
			break;
		case TypeStrArray:
			free(v->Value.StrAry.val);
			break;
		default:
			break;
		}
	}
	free(Variables);
	Variables = NULL;
	VariableCount = 0;
}

void DispErr(WORD Err)
{
	const char *Msg;
	int i;
	int no, start, end;
	char *filename;

	switch (Err) {
		case ErrCloseParent: Msg = "\")\" expected."; break;
		case ErrCantCall: Msg = "Can't call sub."; break;
		case ErrCantConnect: Msg = "Can't link macro."; break;
		case ErrCantOpen: Msg = "Can't open file."; break;
		case ErrDivByZero: Msg = "Divide by zero."; break;
		case ErrInvalidCtl: Msg = "Invalid control."; break;
		case ErrLabelAlreadyDef: Msg = "Label already defined."; break;
		case ErrLabelReq: Msg = "Label requiered."; break;
		case ErrLinkFirst: Msg = "Link macro first. Use 'connect' macro."; break;
		case ErrStackOver: Msg = "Stack overflow."; break;
		case ErrSyntax: Msg = "Syntax error."; break;
		case ErrTooManyLabels: Msg = "Too many labels."; break;
		case ErrTooManyVar: Msg = "Too many variables."; break;
		case ErrTypeMismatch: Msg = "Type mismatch."; break;
		case ErrVarNotInit: Msg = "Variable not initialized."; break;
		case ErrCloseComment: Msg = "\"*/\" expected."; break;
		case ErrOutOfRange: Msg = "Index out of range."; break;
		case ErrCloseBracket: Msg = "\"]\" expected."; break;
		case ErrFewMemory: Msg = "Can't allocate memory."; break;
		case ErrNotSupported: Msg = "Unknown command."; break;
		case ErrCantExec: Msg = "Can't execute command."; break;
		default: Msg = "Unknown error message number."; break;
	};

	no = GetLineNo();
	start = LineParsePtr;
	end = LinePtr;
	if (start == end)
		end = LineLen;

	filename = GetMacroFileName();

	i = OpenErrDlg(Msg, LineBuff, no, start, end, filename);
	if (i==IDOK) TTLStatus = IdTTLEnd;
}

void LockVar()
{
}

void UnlockVar()
{
}

BOOL CheckReservedWord(PCHAR Str, LPWORD WordId)
{
	*WordId = 0;

	switch (Str[0] | 0x20) { // to lower-case
	case 'a':
		if (_stricmp(Str,"and")==0) *WordId = RsvBAnd;
		break;
	case 'b':
		if (_stricmp(Str,"beep")==0) *WordId = RsvBeep;
		else if (_stricmp(Str,"bplusrecv")==0) *WordId = RsvBPlusRecv;
		else if (_stricmp(Str,"bplussend")==0) *WordId = RsvBPlusSend;
		else if (_stricmp(Str,"break")==0) *WordId = RsvBreak;
		else if (_stricmp(Str,"bringupbox")==0) *WordId = RsvBringupBox;
		else if (_stricmp(Str,"basename")==0) *WordId = RsvBasename;
		break;
	case 'c':
		if (_stricmp(Str,"call")==0) *WordId = RsvCall;
		else if (_stricmp(Str,"callmenu")==0) *WordId = RsvCallMenu;
		else if (_stricmp(Str,"changedir")==0) *WordId = RsvChangeDir;
		else if (_stricmp(Str,"checksum8")==0) *WordId = RsvChecksum8;
		else if (_stricmp(Str,"checksum8file")==0) *WordId = RsvChecksum8File;
		else if (_stricmp(Str,"checksum16")==0) *WordId = RsvChecksum16;
		else if (_stricmp(Str,"checksum16file")==0) *WordId = RsvChecksum16File;
		else if (_stricmp(Str,"checksum32")==0) *WordId = RsvChecksum32;
		else if (_stricmp(Str,"checksum32file")==0) *WordId = RsvChecksum32File;
		else if (_stricmp(Str,"clearscreen")==0) *WordId = RsvClearScreen;
		else if (_stricmp(Str,"clipb2var")==0) *WordId = RsvClipb2Var;  // add 'clipb2var' (2006.9.17 maya)
		else if (_stricmp(Str,"closesbox")==0) *WordId = RsvCloseSBox;
		else if (_stricmp(Str,"closett")==0) *WordId = RsvCloseTT;
		else if (_stricmp(Str,"code2str")==0) *WordId = RsvCode2Str;
		else if (_stricmp(Str,"connect")==0) *WordId = RsvConnect;
		else if (_stricmp(Str,"continue")==0) *WordId = RsvContinue;
		else if (_stricmp(Str,"crc16")==0) *WordId = RsvCrc16;
		else if (_stricmp(Str,"crc16file")==0) *WordId = RsvCrc16File;
		else if (_stricmp(Str,"crc32")==0) *WordId = RsvCrc32;
		else if (_stricmp(Str,"crc32file")==0) *WordId = RsvCrc32File;
		else if (_stricmp(Str,"cygconnect")==0) *WordId = RsvCygConnect;
		break;
	case 'd':
		if (_stricmp(Str,"delpassword")==0) *WordId = RsvDelPassword;
		else if (_stricmp(Str,"disconnect")==0) *WordId = RsvDisconnect;
		else if (_stricmp(Str,"dispstr")==0) *WordId = RsvDispStr;
		else if (_stricmp(Str,"do")==0) *WordId = RsvDo;
		else if (_stricmp(Str,"dirname")==0) *WordId = RsvDirname;
		else if (_stricmp(Str, "dirnamebox") == 0) *WordId = RsvDirnameBox;
		break;
	case 'e':
		if (_stricmp(Str,"else")==0) *WordId = RsvElse;
		else if (_stricmp(Str,"elseif")==0) *WordId = RsvElseIf;
		else if (_stricmp(Str,"enablekeyb")==0) *WordId = RsvEnableKeyb;
		else if (_stricmp(Str,"end")==0) *WordId = RsvEnd;
		else if (_stricmp(Str,"endif")==0) *WordId = RsvEndIf;
		else if (_stricmp(Str,"enduntil")==0) *WordId = RsvEndUntil;
		else if (_stricmp(Str,"endwhile")==0) *WordId = RsvEndWhile;
		else if (_stricmp(Str,"exec")==0) *WordId = RsvExec;
		else if (_stricmp(Str,"execcmnd")==0) *WordId = RsvExecCmnd;
		else if (_stricmp(Str,"exit")==0) *WordId = RsvExit;
		else if (_stricmp(Str,"expandenv")==0) *WordId = RsvExpandEnv;
		break;
	case 'f':
		if (_stricmp(Str,"fileclose")==0) *WordId = RsvFileClose;
		else if (_stricmp(Str,"fileconcat")==0) *WordId = RsvFileConcat;
		else if (_stricmp(Str,"filecopy")==0) *WordId = RsvFileCopy;
		else if (_stricmp(Str,"filecreate")==0) *WordId = RsvFileCreate;
		else if (_stricmp(Str,"filedelete")==0) *WordId = RsvFileDelete;
		else if (_stricmp(Str,"filelock")==0) *WordId = RsvFileLock;
		else if (_stricmp(Str,"filemarkptr")==0) *WordId = RsvFileMarkPtr;
		else if (_stricmp(Str,"filenamebox")==0) *WordId  = RsvFilenameBox; // add 'filenamebox' (2007.9.13 maya)
		else if (_stricmp(Str,"fileopen")==0) *WordId = RsvFileOpen;
		else if (_stricmp(Str,"filereadln")==0) *WordId = RsvFileReadln;
		else if (_stricmp(Str,"fileread")==0) *WordId = RsvFileRead;    // add
		else if (_stricmp(Str,"filerename")==0) *WordId = RsvFileRename;
		else if (_stricmp(Str,"filesearch")==0) *WordId = RsvFileSearch;
		else if (_stricmp(Str,"fileseek")==0) *WordId = RsvFileSeek;
		else if (_stricmp(Str,"fileseekback")==0) *WordId = RsvFileSeekBack;
		else if (_stricmp(Str,"filestat")==0) *WordId = RsvFileStat;
		else if (_stricmp(Str,"filestrseek")==0) *WordId = RsvFileStrSeek;
		else if (_stricmp(Str,"filestrseek2")==0) *WordId = RsvFileStrSeek2;
		else if (_stricmp(Str,"filetruncate")==0) *WordId = RsvFileTruncate;
		else if (_stricmp(Str,"fileunlock")==0) *WordId = RsvFileUnLock;
		else if (_stricmp(Str,"filewrite")==0) *WordId = RsvFileWrite;
		else if (_stricmp(Str,"filewriteln")==0) *WordId = RsvFileWriteLn;
		else if (_stricmp(Str,"findclose")==0) *WordId = RsvFindClose;
		else if (_stricmp(Str,"findfirst")==0) *WordId = RsvFindFirst;
		else if (_stricmp(Str,"findnext")==0) *WordId = RsvFindNext;
		else if (_stricmp(Str,"flushrecv")==0) *WordId = RsvFlushRecv;
		else if (_stricmp(Str,"foldercreate")==0) *WordId = RsvFolderCreate;
		else if (_stricmp(Str,"folderdelete")==0) *WordId = RsvFolderDelete;
		else if (_stricmp(Str,"foldersearch")==0) *WordId = RsvFolderSearch;
		else if (_stricmp(Str,"for")==0) *WordId = RsvFor;
		break;
	case 'g':
		if (_stricmp(Str,"getdate")==0) *WordId = RsvGetDate;
		else if (_stricmp(Str,"getdir")==0) *WordId = RsvGetDir;
		else if (_stricmp(Str,"getenv")==0) *WordId = RsvGetEnv;
		else if (_stricmp(Str,"getfileattr")==0) *WordId = RsvGetFileAttr;
		else if (_stricmp(Str,"gethostname")==0) *WordId = RsvGetHostname;
		else if (_stricmp(Str,"getipv4addr")==0) *WordId = RsvGetIPv4Addr;
		else if (_stricmp(Str,"getipv6addr")==0) *WordId = RsvGetIPv6Addr;
		else if (_stricmp(Str,"getmodemstatus") == 0) *WordId = RsvGetModemStatus;
		else if (_stricmp(Str,"getpassword")==0) *WordId = RsvGetPassword;
		else if (_stricmp(Str,"getspecialfolder")==0) *WordId = RsvGetSpecialFolder;
		else if (_stricmp(Str,"gettime")==0) *WordId = RsvGetTime;
		else if (_stricmp(Str,"gettitle")==0) *WordId = RsvGetTitle;
		else if (_stricmp(Str,"getttdir")==0) *WordId = RsvGetTTDir;
		else if (_stricmp(Str,"getver")==0) *WordId = RsvGetVer;
		else if (_stricmp(Str,"goto")==0) *WordId = RsvGoto;
		break;
	case 'i':
		if (_stricmp(Str,"if")==0) *WordId = RsvIf;
		else if (_stricmp(Str,"ifdefined")==0) *WordId = RsvIfDefined;
		else if (_stricmp(Str,"include")==0) *WordId = RsvInclude ;
		else if (_stricmp(Str,"inputbox")==0) *WordId = RsvInputBox;
		else if (_stricmp(Str,"int2str")==0) *WordId = RsvInt2Str;
		else if (_stricmp(Str,"intdim")==0) *WordId = RsvIntDim;
		else if (_stricmp(Str,"ispassword")==0) *WordId = RsvIsPassword;    // add 'ispassword'  (2012.5.24 yutaka)
		break;
	case 'k':
		if (_stricmp(Str,"kmtfinish")==0) *WordId = RsvKmtFinish;
		else if (_stricmp(Str,"kmtget")==0) *WordId = RsvKmtGet;
		else if (_stricmp(Str,"kmtrecv")==0) *WordId = RsvKmtRecv;
		else if (_stricmp(Str,"kmtsend")==0) *WordId = RsvKmtSend;
		break;
	case 'l':
		if (_stricmp(Str,"listbox")==0) *WordId = RsvListBox;
		else if (_stricmp(Str,"loadkeymap")==0) *WordId = RsvLoadKeyMap;
		else if (_stricmp(Str,"logautoclosemode")==0) *WordId = RsvLogAutoClose;
		else if (_stricmp(Str,"logclose")==0) *WordId = RsvLogClose;
		else if (_stricmp(Str,"loginfo")==0) *WordId = RsvLogInfo;
		else if (_stricmp(Str,"logopen")==0) *WordId = RsvLogOpen;
		else if (_stricmp(Str,"logpause")==0) *WordId = RsvLogPause;
		else if (_stricmp(Str,"logrotate")==0) *WordId = RsvLogRotate;
		else if (_stricmp(Str,"logstart")==0) *WordId = RsvLogStart;
		else if (_stricmp(Str,"logwrite")==0) *WordId = RsvLogWrite;
		else if (_stricmp(Str,"loop")==0) *WordId = RsvLoop;
		break;
	case 'm':
		if (_stricmp(Str,"makepath")==0) *WordId = RsvMakePath;
		else if (_stricmp(Str,"messagebox")==0) *WordId = RsvMessageBox;
		else if (_stricmp(Str,"mpause")==0) *WordId = RsvMilliPause;
		break;
	case 'n':
		if (_stricmp(Str,"next")==0) *WordId = RsvNext;
		else if (_stricmp(Str,"not")==0) *WordId = RsvBNot;
		break;
	case 'o':
		if (_stricmp(Str,"or")==0) *WordId = RsvBOr;
		break;
	case 'p':
		if (_stricmp(Str,"passwordbox")==0) *WordId = RsvPasswordBox;
		else if (_stricmp(Str,"pause")==0) *WordId = RsvPause;
		break;
	case 'q':
		if (_stricmp(Str,"quickvanrecv")==0) *WordId = RsvQuickVANRecv;
		else if (_stricmp(Str,"quickvansend")==0) *WordId = RsvQuickVANSend;
		break;
	case 'r':
		if (_stricmp(Str,"random")==0) *WordId = RsvRandom;    // add 'random' (2006.2.11 yutaka)
		else if (_stricmp(Str,"recvln")==0) *WordId = RsvRecvLn;
		else if (_stricmp(Str,"regexoption")==0) *WordId = RsvRegexOption;
		else if (_stricmp(Str,"restoresetup")==0) *WordId = RsvRestoreSetup;
		else if (_stricmp(Str,"return")==0) *WordId = RsvReturn;
		else if (_stricmp(Str,"rotateleft")==0) *WordId = RsvRotateL;   // add 'rotateleft' (2007.8.19 maya)
		else if (_stricmp(Str,"rotateright")==0) *WordId = RsvRotateR;  // add 'rotateright' (2007.8.19 maya)
		break;
	case 's':
		if (_stricmp(Str,"scprecv")==0) *WordId = RsvScpRecv;      // add 'scprecv' (2008.1.1 yutaka)
		else if (_stricmp(Str,"scpsend")==0) *WordId = RsvScpSend;      // add 'scpsend' (2008.1.1 yutaka)
		else if (_stricmp(Str,"send")==0) *WordId = RsvSend;
		else if (_stricmp(Str,"sendbreak")==0) *WordId = RsvSendBreak;
		else if (_stricmp(Str,"sendbroadcast")==0) *WordId = RsvSendBroadcast;
		else if (_stricmp(Str,"sendlnbroadcast")==0) *WordId = RsvSendlnBroadcast;
		else if (_stricmp(Str,"sendlnmulticast")==0) *WordId = RsvSendlnMulticast;
		else if (_stricmp(Str,"sendmulticast")==0) *WordId = RsvSendMulticast;
		else if (_stricmp(Str,"setfileattr")==0) *WordId = RsvSetFileAttr;
		else if (_stricmp(Str,"setmulticastname")==0) *WordId = RsvSetMulticastName;
		else if (_stricmp(Str,"sendfile")==0) *WordId = RsvSendFile;
		else if (_stricmp(Str,"sendkcode")==0) *WordId = RsvSendKCode;
		else if (_stricmp(Str,"sendln")==0) *WordId = RsvSendLn;
		else if (_stricmp(Str,"setbaud")==0) *WordId = RsvSetBaud;
		else if (_stricmp(Str,"setdate")==0) *WordId = RsvSetDate;
		else if (_stricmp(Str,"setdebug")==0) *WordId = RsvSetDebug;
		else if (_stricmp(Str,"setdir")==0) *WordId = RsvSetDir;
		else if (_stricmp(Str,"setdlgpos")==0) *WordId = RsvSetDlgPos;
		else if (_stricmp(Str,"setdtr")==0) *WordId = RsvSetDtr;    // add 'setdtr'  (2008.3.12 maya)
		else if (_stricmp(Str,"setecho")==0) *WordId = RsvSetEcho;
		else if (_stricmp(Str,"setenv")==0) *WordId = RsvSetEnv;    // reactivate 'setenv' (2007.8.31 maya)
		else if (_stricmp(Str,"setexitcode")==0) *WordId = RsvSetExitCode;
		else if (_stricmp(Str,"setflowctrl")==0) *WordId = RsvSetFlowCtrl;
		else if (_stricmp(Str,"setpassword")==0) *WordId = RsvSetPassword;    // add 'setpassword'  (2012.5.23 yutaka)
		else if (_stricmp(Str,"setrts")==0) *WordId = RsvSetRts;    // add 'setrts'  (2008.3.12 maya)
		else if (_stricmp(Str,"setspeed")==0) *WordId = RsvSetBaud;
		else if (_stricmp(Str,"setsync")==0) *WordId = RsvSetSync;
		else if (_stricmp(Str,"settime")==0) *WordId = RsvSetTime;
		else if (_stricmp(Str,"settitle")==0) *WordId = RsvSetTitle;
		else if (_stricmp(Str,"show")==0) *WordId = RsvShow;
		else if (_stricmp(Str,"showtt")==0) *WordId = RsvShowTT;
		else if (_stricmp(Str,"sprintf")==0) *WordId = RsvSprintf;  // add 'sprintf' (2007.5.1 yutaka)
		else if (_stricmp(Str,"sprintf2")==0) *WordId = RsvSprintf2;  // add 'sprintf2' (2008.12.18 maya)
		else if (_stricmp(Str,"statusbox")==0) *WordId = RsvStatusBox;
		else if (_stricmp(Str,"str2code")==0) *WordId = RsvStr2Code;
		else if (_stricmp(Str,"str2int")==0) *WordId = RsvStr2Int;
		else if (_stricmp(Str,"strcompare")==0) *WordId = RsvStrCompare;
		else if (_stricmp(Str,"strconcat")==0) *WordId = RsvStrConcat;
		else if (_stricmp(Str,"strcopy")==0) *WordId = RsvStrCopy;
		else if (_stricmp(Str,"strdim")==0) *WordId = RsvStrDim;
		else if (_stricmp(Str,"strinsert")==0) *WordId = RsvStrInsert;
		else if (_stricmp(Str,"strjoin")==0) *WordId = RsvStrJoin;
		else if (_stricmp(Str,"strlen")==0) *WordId = RsvStrLen;
		else if (_stricmp(Str,"strmatch")==0) *WordId = RsvStrMatch;
		else if (_stricmp(Str,"strremove")==0) *WordId = RsvStrRemove;
		else if (_stricmp(Str,"strreplace")==0) *WordId = RsvStrReplace;
		else if (_stricmp(Str,"strscan")==0) *WordId = RsvStrScan;
		else if (_stricmp(Str,"strspecial")==0) *WordId = RsvStrSpecial;
		else if (_stricmp(Str,"strsplit")==0) *WordId = RsvStrSplit;
		else if (_stricmp(Str,"strtrim")==0) *WordId = RsvStrTrim;
		break;
	case 't':
		if (_stricmp(Str,"testlink")==0) *WordId = RsvTestLink;
		else if (_stricmp(Str,"then")==0) *WordId = RsvThen;
		else if (_stricmp(Str,"tolower")==0) *WordId = RsvToLower;  // add 'tolower' (2007.7.12 maya)
		else if (_stricmp(Str,"toupper")==0) *WordId = RsvToUpper;  // add 'toupper' (2007.7.12 maya)
		break;
	case 'u':
		if (_stricmp(Str,"unlink")==0) *WordId = RsvUnlink;
		else if (_stricmp(Str,"until")==0) *WordId = RsvUntil;
		else if (_stricmp(Str,"uptime")==0) *WordId = RsvUptime;
		break;
	case 'v':
		if (_stricmp(Str,"var2clipb")==0) *WordId = RsvVar2Clipb;  // add 'var2clipb' (2006.9.17 maya)
		break;
	case 'w':
		if (_stricmp(Str,"waitregex")==0) *WordId = RsvWaitRegex;  // add 'waitregex' (2005.10.5 yutaka)
		else if (_stricmp(Str,"wait")==0) *WordId = RsvWait;
		else if (_stricmp(Str,"wait4all")==0) *WordId = RsvWait4all;
		else if (_stricmp(Str,"waitevent")==0) *WordId = RsvWaitEvent;
		else if (_stricmp(Str,"waitln")==0) *WordId = RsvWaitLn;
		else if (_stricmp(Str,"waitn")==0) *WordId = RsvWaitN;  // add 'waitn'  (2009.1.26 maya)
		else if (_stricmp(Str,"waitrecv")==0) *WordId = RsvWaitRecv;
		else if (_stricmp(Str,"while")==0) *WordId = RsvWhile;
		break;
	case 'x':
		if (_stricmp(Str,"xmodemrecv")==0) *WordId = RsvXmodemRecv;
		else if (_stricmp(Str,"xmodemsend")==0) *WordId = RsvXmodemSend;
		else if (_stricmp(Str,"xor")==0) *WordId = RsvBXor;
		break;
	case 'y':
		if (_stricmp(Str,"yesnobox")==0) *WordId = RsvYesNoBox;
		else if (_stricmp(Str,"ymodemrecv")==0) *WordId = RsvYmodemRecv;
		else if (_stricmp(Str,"ymodemsend")==0) *WordId = RsvYmodemSend;
		break;
	case 'z':
		if (_stricmp(Str,"zmodemrecv")==0) *WordId = RsvZmodemRecv;
		else if (_stricmp(Str,"zmodemsend")==0) *WordId = RsvZmodemSend;
		break;
	default:
		; /* nothing to do */
	}

	return (*WordId!=0);
}

/* C言語コメントが閉じられているかどうか */
int IsCommentClosed(void)
{
#ifdef SUPPORT_C_STYLE_COMMENT
	int ret = (commenting == 0);

	/* コメントが始まる手前までのコマンドを実行できるようにするため、
	 * 内部フラグはクリアしておく。
	 */
	commenting = 0;
	return (ret);
#else
	// 当該機能が無効の場合は、常に「真」を返す。
	return 1;
#endif
}

BYTE GetFirstChar()
{
	BYTE b;
	int comment_starting = 0;

	if (LinePtr<LineLen)
		b = LineBuff[LinePtr];
	else return 0;

	while ((LinePtr<LineLen) && ((b==' ') || (b=='\t')))
	{
		LinePtr++;
		if (LinePtr<LineLen) b = LineBuff[LinePtr];
	}

#ifdef SUPPORT_C_STYLE_COMMENT
	if (commenting) {
		while (LinePtr < LineLen) {
			/* コメントの終わりが出てくるまでスキップ */
			if (LineBuff[LinePtr] == '*' && LineBuff[LinePtr + 1] == '/') {
				commenting = 0;
				LinePtr += 2;
				break;
			}
			LinePtr++;
		}
		/* 一行にコメントの終わりがなかったら、次の行を読む。*/
		if (commenting)
			return 0;   // next line

		if (LinePtr < LineLen) 
			b = LineBuff[LinePtr];
		else
			b = 0;

		while ((LinePtr<LineLen) && ((b==' ') || (b=='\t')))
		{
			LinePtr++;
			if (LinePtr<LineLen) b = LineBuff[LinePtr];
		}
	}

	/* C言語コメントの始まり */
	do {
		if (LineBuff[LinePtr] == '/' && LineBuff[LinePtr + 1] == '*') {
			comment_starting = 1;
			LinePtr += 2;
			while (LinePtr < LineLen) {
				/* コメントの終わりが出てくるまでスキップ */
				if (LineBuff[LinePtr] == '*' && LineBuff[LinePtr + 1] == '/') {
					LinePtr += 2;
					comment_starting = 0;
					break;
				}
				LinePtr++;
			}

			if (LinePtr < LineLen) 
				b = LineBuff[LinePtr];
			else
				b = 0;

			while ((LinePtr<LineLen) && ((b==' ') || (b=='\t')))
			{
				LinePtr++;
				if (LinePtr<LineLen) b = LineBuff[LinePtr];
			}

			/* コメントの終わりが一行に登場しない場合は、永続的に記録する。*/
			if (comment_starting)
				commenting = 1;
		}
		else {
			break;
		}

		/* 1つの行に、複数のコメントがある場合に対応するため、次の文字がスラッシュならば、
		 * ループ処理の始めに戻す。
		 */
	} while (b == '/');
#endif

	if ((b>' ') && (b!=';'))
	{
		LinePtr++;
		return b;
	}
	return 0;
}

BOOL CheckParameterGiven()
{
	WORD P;

	P = LinePtr;

	if (GetFirstChar()) {
		LinePtr = P;
		return TRUE;
	}
	else {
		LinePtr = P;
		return FALSE;
	}
}

BOOL GetIdentifier(PCHAR Name)
{
	int i;
	BYTE b;

	memset(Name,0,MaxNameLen);

	b = GetFirstChar();
	if (b==0) return FALSE;

	// Check first character of identifier
	if (! __iscsymf(b)) { // [^A-Za-z_]
		LinePtr--;
		return FALSE;
	}

	Name[0] = b;
	i = 1;

	if (LinePtr<LineLen) b = LineBuff[LinePtr];
	while ((LinePtr<LineLen) && __iscsym(b)) { // [0-9A-Za-z_]
		if (i<MaxNameLen-1)
		{
			Name[i] = b;
			i++;
		}
		LinePtr++;
		if (LinePtr<LineLen) b = LineBuff[LinePtr];
	}
	return TRUE;
}

BOOL GetReservedWord(LPWORD WordId)
{
	TName Name;
	WORD P;

	P = LinePtr;
	if (! GetIdentifier(Name)) return FALSE;
	if (! CheckReservedWord(Name,WordId))
	{
		LinePtr = P;
		return FALSE;
	}
	if (0 < *WordId)
		return TRUE;
	else
		LinePtr = P;
	return FALSE;
}

BOOL GetOperator(LPWORD WordId)
{
	WORD P;
	BYTE b;

	P = LinePtr;
	b = GetFirstChar();
	switch (b) {
		case 0:	return FALSE; break;
		case '*': *WordId = RsvMul;   break;
		case '+': *WordId = RsvPlus;  break;
		case '-': *WordId = RsvMinus; break;
		case '/': *WordId = RsvDiv;   break;
		case '%': *WordId = RsvMod;   break;
		case '=': *WordId = RsvEQ;
			if (LinePtr < LineLen && LineBuff[LinePtr] == '=') {
				LinePtr++;
			}
			break;
		case '<': *WordId = RsvLT;
			if (LinePtr < LineLen) {
				switch (LineBuff[LinePtr++]) {
					case '=': *WordId = RsvLE; break;
					case '>': *WordId = RsvNE; break;
					case '<': *WordId = RsvALShift; break;
					default: LinePtr--;
				}
			}
			break;
		case '>': *WordId = RsvGT;
			if (LinePtr < LineLen) {
				switch (LineBuff[LinePtr++]) {
					case '=': *WordId = RsvGE; break;
					case '>': *WordId = RsvARShift;
						if (LinePtr < LineLen && LineBuff[LinePtr] == '>') {
							*WordId = RsvLRShift; LinePtr++;
						}
						break;
					default: LinePtr--;
				}
			}
			break;
		case '&': *WordId = RsvBAnd;
			if (LinePtr < LineLen && LineBuff[LinePtr] == '&') {
				*WordId = RsvLAnd; LinePtr++;
			}
			break;
		case '|': *WordId = RsvBOr;
			if (LinePtr < LineLen && LineBuff[LinePtr] == '|') {
				*WordId = RsvLOr; LinePtr++;
			}
			break;
		case '^': *WordId = RsvBXor; break;
		case '~': *WordId = RsvBNot; break;
		case '!': *WordId = RsvLNot;
			if (LinePtr < LineLen && LineBuff[LinePtr] == '=') {
				*WordId = RsvNE; LinePtr++;
			}
			break;
		default:
			LinePtr--;
			if (! GetReservedWord(WordId) || (*WordId < RsvOperator)) {
				LinePtr = P;
				return FALSE;
			}
	}

	return TRUE;
}

BOOL GetLabelName(PCHAR Name)
{
	int i;
	BYTE b;

	memset(Name,0,MaxNameLen);

	b = GetFirstChar();
	if (b==0) return FALSE;
	Name[0] = b;

	i = 1;
	if (LinePtr<LineLen) b = LineBuff[LinePtr];
	while ((LinePtr<LineLen) && __iscsym(b)) { // [0-9A-Za-z_]
		if (i<MaxNameLen-1)
		{
			Name[i] = b;
			i++;
		}
		LinePtr++;
		if (LinePtr<LineLen) b = LineBuff[LinePtr];
	}

	return (strlen(Name)>0);
}

static int GetQuotedStr(PCHAR Str, BYTE q, LPWORD i)
{
	BYTE b;

	b=0;
	if (LinePtr<LineLen) b = LineBuff[LinePtr];
	while ((LinePtr<LineLen) && (b>=' '||b=='\t') && (b!=q))
	{
		if (*i<MaxStrLen-1)
		{
			Str[*i] = b;
			(*i)++;
		}

		LinePtr++;
		if (LinePtr<LineLen) b = LineBuff[LinePtr];
	}
	if (b==q) {
		if (LinePtr<LineLen)
			LinePtr++;
	}
	else
		return (ErrSyntax);

	return 0;
}

static WORD GetCharByCode(PCHAR Str, LPWORD i)
{
	BYTE b;
	WORD n;

	b=0;
	n = 0;
	if (LinePtr<LineLen) b = LineBuff[LinePtr];
	if (!isdigit(b) && (b!='$')) return ErrSyntax;

	if (b!='$') { /* decimal */
		while ((LinePtr<LineLen) && isdigit(b)) { // [0-9]
			n = n * 10 + b - '0';
			LinePtr++;
			if (LinePtr<LineLen) b = LineBuff[LinePtr];
		}
	}
	else { /* hexadecimal */
		LinePtr++;
		if (LinePtr<LineLen) b = LineBuff[LinePtr];
		while ((LinePtr<LineLen) && isxdigit(b)) { // [0-9A-Fa-f]
			if (isalpha(b))
				b = (b|0x20) - 'a' + 10;
			else
				b = b - '0';
			n = n * 16 + b;
			LinePtr++;
			if (LinePtr<LineLen) b = LineBuff[LinePtr];
		}
	}

	if ((n==0) || (n>255)) return ErrSyntax;

	if (*i<MaxStrLen-1)
	{
		Str[*i] = (char)n;
		(*i)++;
	}
	return 0;
}

BOOL GetString(PCHAR Str, LPWORD Err)
{
	BYTE q;
	WORD i;

	*Err = 0;
	memset(Str,0,MaxStrLen);

	q = GetFirstChar();
	if (q==0) return FALSE;
	LinePtr--;
	if ((q!='"') && (q!='\'') && (q!='#'))
		return FALSE;

	i = 0;
	while (((q=='"') || (q=='\'') || (q=='#')) && (*Err==0))
	{
		LinePtr++;
		switch (q) {
			case '"':
			case '\'': *Err = GetQuotedStr(Str,q,&i); break;
			case '#':  *Err = GetCharByCode(Str,&i);  break;
		}
		q = LineBuff[LinePtr];
	}
	return TRUE;
}

BOOL GetNumber(int far *Num)
{
	BYTE b;

	*Num = 0;

	b = GetFirstChar();
	if (b==0) return FALSE;
	if (isdigit(b)) { /* decimal constant */
		*Num = b - '0';
		if (LinePtr<LineLen) b = LineBuff[LinePtr];
		while ((LinePtr<LineLen) && isdigit(b)) {
			*Num = *Num * 10 + b - '0';
			LinePtr++;
			if (LinePtr<LineLen) b = LineBuff[LinePtr];
		}
	}
	else if (b=='$')
	{ /* hexadecimal constant */
		if (LinePtr<LineLen) b = LineBuff[LinePtr];
		while ((LinePtr<LineLen) && isxdigit(b)) { // [0-9A-Fa-f]
			if (isalpha(b))
				b = (b|0x20) - 'a' + 10;
			else
				b = b - '0';
			*Num = *Num * 16 + b;
			LinePtr++;
			if (LinePtr<LineLen) b = LineBuff[LinePtr];
		}
	}
	else {
		LinePtr--;
		return FALSE;
	}
	return TRUE;
}

BOOL CheckVar(const char *Name, LPWORD VarType, PVarId VarId)
{
	int i;
	const Variable_t *v = Variables;
	for (i = 0; i < VariableCount; v++,i++) {
		if (_stricmp(v->Name, Name) == 0) {
			*VarType = v->Type;
			*VarId = (TVarId)i;
			return TRUE;
		}
	}
	*VarType = TypUnknown;
	*VarId = 0;
	return FALSE;
}

static Variable_t *NewVar(const char *name, VariableType_t type)
{
	Variable_t *new_v = (Variable_t * )realloc(Variables, sizeof(Variable_t) * (VariableCount + 1));
	if (new_v == NULL) {
		// TODO メモリがない
		return NULL;
	};
	Variables = new_v;
	Variable_t *v = &Variables[VariableCount];
	VariableCount++;
	v->Name = _strdup(name);
	v->Type = type;
	return v;
}

BOOL NewIntVar(const char *Name, int InitVal)
{
	Variable_t *v = NewVar(Name, TypeInteger);
	v->Value.Int = InitVal;
	return TRUE;
}

BOOL NewStrVar(const char *Name, const char *InitVal)
{
	Variable_t *v = NewVar(Name, TypeString);
	v->Value.Str = _strdup(InitVal);
	return TRUE;
}

int NewIntAryVar(const char *Name, int size)
{
	Variable_t *v = NewVar(Name, TypeIntArray);
	TIntAry *intAry = &v->Value.IntAry;
	int *array = (int *)calloc(size, sizeof(int));
	if (array == NULL) {
		return ErrFewMemory;
	}
	intAry->val = array;
	intAry->size = size;
	return 0;
}

int NewStrAryVar(const char *Name, int size)
{
	Variable_t *v = NewVar(Name, TypeStrArray);
	TStrAry *strAry = &v->Value.StrAry;
	char **array = (char **)calloc(size, sizeof(char *));
	if (array == NULL) {
		return ErrFewMemory;
	}
	strAry->val = array;
	strAry->size = size;
	return 0;
}

BOOL NewLabVar(const char *Name, BINT InitVal, WORD ILevel)
{
	Variable_t *v = NewVar(Name, TypeLabel);
	TLab *lab = &v->Value.Lab;
	lab->val = InitVal;
	lab->level = ILevel;
	return TRUE;
}

void DelLabVar(WORD ILevel)
{
	Variable_t *v = Variables;
	for (;;) {
		if (v == &Variables[VariableCount]) {
			// 最後まで来た
			break;
		}
		if (v->Type == TypeLabel) {
			if (v->Value.Lab.level >= ILevel) {
				size_t left;
				// 削除する
				free(v->Name);
				// 後ろを前につめる
				left = &Variables[VariableCount - 1] - v;
				if (left > 0) {
					memmove(v, v+1, sizeof(Variable_t) * left);
				}
				// 1つ減る
				VariableCount--;

				continue;
			}
		}
		v++;
	}
	Variables = (Variable_t *)realloc(Variables, sizeof(Variable_t) * VariableCount);
}

void CopyLabel(WORD ILabel, BINT far *Ptr, LPWORD Level)
{
	Variable_t *v = &Variables[ILabel];
	*Ptr = v->Value.Lab.val;
	*Level = v->Value.Lab.level;
}

/*
 * Precedence: 1
 *   Evaluate variable.
 *   Evaluate number.
 *   Evaluate parenthesis.
 *   Evaluate following operator.
 *     not, ~, !, +(unary), -(unary)
 */
BOOL GetFactor(LPWORD ValType, int far *Val, LPWORD Err)
{
	TName Name;
	WORD P, WId;
	TVarId VarId;
	int Index;

	P = LinePtr;
	*Err = 0;
	if (GetIdentifier(Name)) {
		if (CheckReservedWord(Name,&WId)) {
			if (GetFactor(ValType, Val, Err)) {
				if ((*Err==0) && (*ValType!=TypInteger))
					*Err = ErrTypeMismatch;
				switch (WId) {
					case RsvBNot: *Val = ~(*Val); break;
					case RsvLNot: *Val = !(*Val); break;
					default: *Err = ErrSyntax;
				}
			}
			else {
				*Err = ErrSyntax;
			}
		}
		else if (CheckVar(Name, ValType, &VarId)) {
			switch (*ValType) {
				case TypInteger:
					*Val = Variables[VarId].Value.Int;
					break;
				case TypString: *Val = VarId; break;
				case TypIntArray:
					if (GetIndex(&Index, Err)) {
						TIntAry *intAry = &Variables[VarId].Value.IntAry;
						if (Index >= 0 && Index < intAry->size) {
							*Val = intAry->val[Index];
							*ValType = TypInteger;
						}
						else {
							*Err = ErrOutOfRange;
						}
					}
					else if (*Err == 0) {
						*Val = VarId;
					}
					break;
				case TypStrArray:
					if (GetIndex(&Index, Err)) {
						VarId = GetStrVarFromArray(VarId, Index, Err);
						if (*Err == 0) {
							*Val = VarId;
							*ValType = TypString;
						}
					}
					else if (*Err == 0) {
						*Val = VarId;
					}
					break;
			}
		}
		else
			*Err = ErrVarNotInit;
	}
	else if (GetNumber(Val))
		*ValType = TypInteger;
	else if (GetOperator(&WId)) {
		if (GetFactor(ValType, Val, Err)) {
			if ((*Err==0) && (*ValType != TypInteger))
				*Err = ErrTypeMismatch;
			switch (WId) {
				case RsvPlus:                    break;
				case RsvMinus:   *Val = -(*Val); break;
				case RsvBNot:    *Val = ~(*Val); break;
				case RsvLNot:    *Val = !(*Val); break;
				default: *Err = ErrSyntax;
			}
		}
		else {
			*Err = ErrSyntax;
		}
	}
	else if (GetFirstChar()=='(') {
		if (GetExpression(ValType, Val, Err)) {
			if ((*Err==0) && (GetFirstChar()!=')'))
				*Err = ErrCloseParent;
		}
		else
			*Err = ErrSyntax;
	}
	else {
		*Err = 0;
		return FALSE;
	}

	if (*Err!=0) LinePtr = P;
	return TRUE;
}

/*
 * Precedence: 2
 *   Evaluate following operator.
 *     *, /, %
 */
BOOL EvalMultiplication(LPWORD ValType, int far *Val, LPWORD Err)
{
	WORD P, Type, Er;
	int Val1, Val2;
	WORD WId;

	if (! GetFactor(&Type, &Val1, &Er)) return FALSE;
	*ValType = Type;
	*Val = Val1;
	*Err = Er;
	if (Er) return TRUE;
	if (Type!=TypInteger) return TRUE;

	while (TRUE) {
		P = LinePtr;
		if (! GetOperator(&WId)) return TRUE;

		switch (WId) {
			case RsvMul:
			case RsvDiv:
			case RsvMod:
				break;
			default:
				LinePtr = P;
				return TRUE;
		}

		if (! GetFactor(&Type, &Val2, &Er)) {
			*Err = ErrSyntax;
			return TRUE;
		}

		if (Er) {
			*Err = Er;
			return TRUE;
		}

		if (Type!=TypInteger) {
			*Err = ErrTypeMismatch;
			return TRUE;
		}

		if (Val2 == 0 && WId != RsvMul) {
			*Err = ErrDivByZero;
			return TRUE;
		}

		switch (WId) {
			case RsvMul: Val1 = Val1 * Val2; break;
			case RsvDiv: Val1 = Val1 / Val2; break;
			case RsvMod: Val1 = Val1 % Val2; break;
		}
		*Val = Val1;
	}
}

/*
 * Precedence: 3
 *   Evaluate following operator.
 *     +, -
 */
BOOL EvalAddition(LPWORD ValType, int far *Val, LPWORD Err)
{
	WORD P, Type, Er;
	int Val1, Val2;
	WORD WId;

	if (! EvalMultiplication(&Type, &Val1, &Er)) return FALSE;
	*ValType = Type;
	*Val = Val1;
	*Err = Er;
	if (Er) return TRUE;
	if (Type!=TypInteger) return TRUE;

	while (TRUE) {
		P = LinePtr;
		if (! GetOperator(&WId)) return TRUE;

		switch (WId) {
			case RsvPlus:
			case RsvMinus:
				break;
			default:
				LinePtr = P;
				return TRUE;
		}

		if (! EvalMultiplication(&Type, &Val2, &Er)) {
			*Err = ErrSyntax;
			return TRUE;
		}

		if (Er) {
			*Err = Er;
			return TRUE;
		}

		if (Type!=TypInteger) {
			*Err = ErrTypeMismatch;
			return TRUE;
		}

		switch (WId) {
			case RsvPlus:    Val1 = Val1 + Val2;  break;
			case RsvMinus:   Val1 = Val1 - Val2;  break;
		}
		*Val = Val1;
	}
}

/*
 * Precedence: 4
 *   Evaluate following operator.
 *     >>, <<, >>>
 */
BOOL EvalBitShift(LPWORD ValType, int far *Val, LPWORD Err)
{
	WORD P, Type, Er;
	int Val1, Val2;
	WORD WId;

	if (! EvalAddition(&Type, &Val1, &Er)) return FALSE;
	*ValType = Type;
	*Val = Val1;
	*Err = Er;
	if (Er) return TRUE;
	if (Type!=TypInteger) return TRUE;

	while (TRUE) {
		P = LinePtr;
		if (! GetOperator(&WId)) return TRUE;

		switch (WId) {
			case RsvARShift:
			case RsvALShift:
			case RsvLRShift:
				break;
			default:
				LinePtr = P;
				return TRUE;
		}

		if (! EvalAddition(&Type, &Val2, &Er)) {
			*Err = ErrSyntax;
			return TRUE;
		}

		if (Er) {
			*Err = Er;
			return TRUE;
		}

		if (Type!=TypInteger) {
			*Err = ErrTypeMismatch;
			return TRUE;
		}

		if (WId == RsvALShift)
			Val2 = -Val2;

		if (Val2 <= -(int)INT_BIT) {	/* Val2 <= -32 */
			Val1 = 0;
		} else if (Val2 < 0 ) {		/* -32 < Val2 < 0 */
			Val1 = Val1 << -Val2;
		} else if (Val2 == 0 ) {	/* Val2 == 0 */
			; /* do nothing */
		} else if (Val2 < INT_BIT) {	/* 0 < Val2 < 32 */
			if (WId == RsvLRShift) {
				// use unsigned int for logical right shift
				Val1 = (unsigned int)Val1 >> Val2;
			} else {
				Val1 = Val1 >> Val2;
			}
		} else {			/* Val2 >= 32 */
			if (Val1 > 0 || WId == RsvLRShift) {
				Val1 = 0;
			} else {
				Val1 = ~0; 
			}
		}
		*Val = Val1;
	}
}

/*
 * Precedence: 5
 *   Evaluate following operator.
 *     &
 */
BOOL EvalBitAnd(LPWORD ValType, int far *Val, LPWORD Err)
{
	WORD P, Type, Er;
	int Val1, Val2;
	WORD WId;

	if (! EvalBitShift(&Type, &Val1, &Er)) return FALSE;
	*ValType = Type;
	*Val = Val1;
	*Err = Er;
	if (Er) return TRUE;
	if (Type!=TypInteger) return TRUE;

	while (TRUE) {
		P = LinePtr;
		if (! GetOperator(&WId)) return TRUE;

		if (WId != RsvBAnd) {
			LinePtr = P;
			return TRUE;
		}

		if (! EvalBitShift(&Type, &Val2, &Er)) {
			*Err = ErrSyntax;
			return TRUE;
		}

		if (Er) {
			*Err = Er;
			return TRUE;
		}

		if (Type!=TypInteger) {
			*Err = ErrTypeMismatch;
			return TRUE;
		}

		Val1 = Val1 & Val2;
		*Val = Val1;
	}
}

/*
 * Precedence: 6
 *   Evaluate following operator.
 *     ^
 */
BOOL EvalBitXor(LPWORD ValType, int far *Val, LPWORD Err)
{
	WORD P, Type, Er;
	int Val1, Val2;
	WORD WId;

	if (! EvalBitAnd(&Type, &Val1, &Er)) return FALSE;
	*ValType = Type;
	*Val = Val1;
	*Err = Er;
	if (Er) return TRUE;
	if (Type!=TypInteger) return TRUE;

	while (TRUE) {
		P = LinePtr;
		if (! GetOperator(&WId)) return TRUE;

		if (WId != RsvBXor) {
			LinePtr = P;
			return TRUE;
		}

		if (! EvalBitAnd(&Type, &Val2, &Er)) {
			*Err = ErrSyntax;
			return TRUE;
		}

		if (Er) {
			*Err = Er;
			return TRUE;
		}

		if (Type!=TypInteger) {
			*Err = ErrTypeMismatch;
			return TRUE;
		}

		Val1 = Val1 ^ Val2;
		*Val = Val1;
	}
}

/*
 * Precedence: 7
 *   Evaluate following operator.
 *     |
 */
BOOL EvalBitOr(LPWORD ValType, int far *Val, LPWORD Err)
{
	WORD P, Type, Er;
	int Val1, Val2;
	WORD WId;

	if (! EvalBitXor(&Type, &Val1, &Er)) return FALSE;
	*ValType = Type;
	*Val = Val1;
	*Err = Er;
	if (Er) return TRUE;
	if (Type!=TypInteger) return TRUE;

	while (TRUE) {
		P = LinePtr;
		if (! GetOperator(&WId)) return TRUE;

		if (WId != RsvBOr) {
			LinePtr = P;
			return TRUE;
		}

		if (! EvalBitXor(&Type, &Val2, &Er)) {
			*Err = ErrSyntax;
			return TRUE;
		}

		if (Er) {
			*Err = Er;
			return TRUE;
		}

		if (Type!=TypInteger) {
			*Err = ErrTypeMismatch;
			return TRUE;
		}

		Val1 = Val1 | Val2;
		*Val = Val1;
	}
}

/*
 * Precedence: 8
 *   Evaluate following operator.
 *     <, >, <=, >=
 */
BOOL EvalGreater(LPWORD ValType, int far *Val, LPWORD Err)
{
	WORD P, Type, Er;
	int Val1, Val2;
	WORD WId;

	if (! EvalBitOr(&Type, &Val1, &Er)) return FALSE;
	*ValType = Type;
	*Val = Val1;
	*Err = Er;
	if (Er) return TRUE;
	if (Type!=TypInteger) return TRUE;

	while (TRUE) {
		P = LinePtr;
		if (! GetOperator(&WId)) return TRUE;

		switch (WId) {
			case RsvLT:
			case RsvGT:
			case RsvLE:
			case RsvGE:
				break;
			default:
				LinePtr = P;
				return TRUE;
		}

		if (! EvalBitOr(&Type, &Val2, &Er)) {
			*Err = ErrSyntax;
			return TRUE;
		}

		if (Er) {
			*Err = Er;
			return TRUE;
		}

		if (Type!=TypInteger) {
			*Err = ErrTypeMismatch;
			return TRUE;
		}

		switch (WId) {
			case RsvLT: Val1 = (Val1 <Val2); break;
			case RsvGT: Val1 = (Val1 >Val2); break;
			case RsvLE: Val1 = (Val1<=Val2); break;
			case RsvGE: Val1 = (Val1>=Val2); break;
		}
		*Val = Val1;
	}
}

/*
 * Precedence: 9
 *   Evaluate following operator.
 *     =, ==, <>, !=
 */
BOOL EvalEqual(LPWORD ValType, int far *Val, LPWORD Err)
{
	WORD P, Type, Er;
	int Val1, Val2;
	WORD WId;

	if (! EvalGreater(&Type, &Val1, &Er)) return FALSE;
	*ValType = Type;
	*Val = Val1;
	*Err = Er;
	if (Er) return TRUE;
	if (Type!=TypInteger) return TRUE;

	while (TRUE) {
		P = LinePtr;
		if (! GetOperator(&WId)) return TRUE;

		switch (WId) {
			case RsvEQ:
			case RsvNE:
				break;
			default:
				LinePtr = P;
				return TRUE;
		}

		if (! EvalGreater(&Type, &Val2, &Er)) {
			*Err = ErrSyntax;
			return TRUE;
		}

		if (Er) {
			*Err = Er;
			return TRUE;
		}

		if (Type!=TypInteger) {
			*Err = ErrTypeMismatch;
			return TRUE;
		}

		switch (WId) {
			case RsvEQ: Val1 = (Val1==Val2); break;
			case RsvNE: Val1 = (Val1!=Val2); break;
		}
		*Val = Val1;
	}
}

/*
 * Precedence: 10
 *   Evaluate following operator.
 *     &&
 */
BOOL EvalLogicalAnd(LPWORD ValType, int far *Val, LPWORD Err)
{
	WORD P, Type, Er;
	int Val1, Val2;
	WORD WId;

	if (! EvalEqual(&Type, &Val1, &Er)) return FALSE;
	*ValType = Type;
	*Val = Val1;
	*Err = Er;
	if (Er) return TRUE;
	if (Type!=TypInteger) return TRUE;

	while (TRUE) {
		P = LinePtr;
		if (! GetOperator(&WId)) return TRUE;

		if (WId != RsvLAnd) {
			LinePtr = P;
			return TRUE;
		}

		if (! EvalEqual(&Type, &Val2, &Er)) {
			*Err = ErrSyntax;
			return TRUE;
		}

		if (Er) {
			*Err = Er;
			return TRUE;
		}

		if (Type!=TypInteger) {
			*Err = ErrTypeMismatch;
			return TRUE;
		}

		Val1 = Val1 && Val2;
		*Val = Val1;
	}
}

/*
 * Precedence: 11
 *   Evaluate following operator.
 *     ||
 */
BOOL GetExpression(LPWORD ValType, int far *Val, LPWORD Err)
{
	WORD P1, P2, Type, Er;
	int Val1, Val2;
	WORD WId;

	P1 = LinePtr;
	if (! EvalLogicalAnd(&Type, &Val1, &Er)) {
		LinePtr = P1;
		return FALSE;
	}
	*ValType = Type;
	*Val = Val1;
	*Err = Er;
	if (Er) {
		LinePtr = P1;
		return TRUE;
	}
	if (Type!=TypInteger) return TRUE;

	while (TRUE) {
		P2 = LinePtr;
		if (! GetOperator(&WId)) return TRUE;

		if (WId != RsvLOr && WId != RsvLXor) {
			LinePtr = P2;
			return TRUE;
		}

		if (! EvalLogicalAnd(&Type, &Val2, &Er)) {
			*Err = ErrSyntax;
			LinePtr = P1;
			return TRUE;
		}

		if (Er) {
			*Err = Er;
			LinePtr = P1;
			return TRUE;
		}

		if (Type!=TypInteger) {
			*Err = ErrTypeMismatch;
			LinePtr = P1;
			return TRUE;
		}

		switch (WId) {
			case RsvLOr:  Val1 = Val1 || Val2; break;
			case RsvLXor: Val1 = (Val1 && !Val2) || (!Val1 && Val2); break;
		}
		*Val = Val1;
	}
}

void GetIntVal(int far *Val, LPWORD Err)
{
	WORD ValType;

	UpdateLineParsePtr();

	if (*Err != 0) return;
	if (! GetExpression(&ValType,Val,Err))
	{
		*Err = ErrSyntax;
		return;
	}
	if (*Err!=0) return;
	if (ValType!=TypInteger)
		*Err = ErrTypeMismatch;
}

void SetIntVal(TVarId VarId, int Val)
{
	if (VarId >> 16) {
		Variable_t *v = &Variables[(VarId>>16)-1];
		int *int_val = &v->Value.IntAry.val[VarId & 0xffff];
		*int_val = Val;
	}
	else {
		Variable_t *v = &Variables[VarId];
		v->Value.Int = Val;
	}
}

int CopyIntVal(TVarId VarId)
{
	Variable_t *v;
	if (VarId >> 16) {
		v = &Variables[(VarId>>16)-1];
		return v->Value.IntAry.val[VarId & 0xffff];
	}
	else {
		v = &Variables[VarId];
		return v->Value.Int;
	}
}

void GetIntVar(PVarId VarId, LPWORD Err)
{
	TName Name;
	WORD VarType;
	int Index;

	if (*Err!=0) return;

	if (GetIdentifier(Name)) {
		if (CheckVar(Name, &VarType, VarId)) {
			switch (VarType) {
			case TypInteger:
				break;
			case TypIntArray:
				if (GetIndex(&Index, Err)) {
					*VarId = GetIntVarFromArray(*VarId, Index, Err);
				}
				else if (*Err == 0) {
					*Err = ErrTypeMismatch;
				}
				break;
			default:
				*Err = ErrTypeMismatch;
			}
		}
		else {
			if (NewIntVar(Name, 0))
				CheckVar(Name, &VarType, VarId);
			else
				*Err = ErrTooManyVar;
		}
	}
	else
		*Err = ErrSyntax;
}

/**
 *	文字列を取得する
 *	@param	Str	文字列へのポインタ (MaxStrLen byte の領域が必要)
 */
void GetStrVal(PCHAR Str, LPWORD Err)
{
	UpdateLineParsePtr();
	GetStrVal2(Str, Err, FALSE);
}

/**
 *	文字列を取得する
 *	@param	Str	文字列へのポインタ (MaxStrLen byte の領域が必要)
 */
void GetStrVal2(PCHAR Str, LPWORD Err, BOOL AutoConversion)
{
	WORD VarType;
	int VarId;

	Str[0] = 0;
	if (*Err!=0) return;

	if (GetString(Str, Err))
		return;
	else if (GetExpression(&VarType, &VarId, Err)) {
		if (*Err!=0) return;
		switch (VarType) {
			case TypString:
				strncpy_s(Str, MaxStrLen, StrVarPtr((TVarId)VarId), _TRUNCATE);
				break;
			case TypInteger:
				if (AutoConversion)
					_snprintf_s(Str, MaxStrLen, _TRUNCATE, "%d", VarId);
				else
					*Err = ErrTypeMismatch;
				break;
			default:
				*Err = ErrTypeMismatch;
		}
	}
	else
		*Err = ErrSyntax;
}

void GetStrVar(PVarId VarId, LPWORD Err)
{
	TName Name;
	WORD VarType;
	int Index;

	if (*Err!=0) return;

	if (GetIdentifier(Name)) {
		if (CheckVar(Name, &VarType, VarId)) {
			switch (VarType) {
			case TypString:
				break;
			case TypStrArray:
				if (GetIndex(&Index, Err)) {
					*VarId = GetStrVarFromArray(*VarId, Index, Err);
				}
				else if (*Err == 0) {
					*Err = ErrTypeMismatch;
				}
				break;
			default:
				*Err = ErrTypeMismatch;
			}
		}
		else {
			if (NewStrVar(Name, ""))
				CheckVar(Name, &VarType, VarId);
			else
				*Err = ErrTooManyVar;
		}
	}
	else
		*Err = ErrSyntax;
}

void SetStrVal(TVarId VarId, const char *Str)
{
	if (VarId >> 16) {
		Variable_t *v = &Variables[(VarId>>16)-1];
		char **str = &v->Value.StrAry.val[VarId & 0xffff];
		free(*str);
		*str = _strdup(Str);
	}
	else {
		Variable_t *v = &Variables[VarId];
		char **str = &v->Value.Str;
		free(*str);
		*str = _strdup(Str);
	}
}

const char *StrVarPtr(TVarId VarId)
{
	Variable_t *v;
	if (VarId >> 16) {
		v = &Variables[(VarId>>16)-1];
		return v->Value.StrAry.val[VarId & 0xffff];
	}
	else {
		v = &Variables[VarId];
		return v->Value.Str;
	}
}

// for ifdefined (2006.9.23 maya)
void GetVarType(LPWORD ValType, int far *Val, LPWORD Err)
{
	TName Name;
	WORD WId;
	TVarId VarId;
	int Index;

	if (GetIdentifier(Name)) {
		if (CheckReservedWord(Name,&WId)) {
			*ValType = TypUnknown;
		}
		else {
			CheckVar(Name, ValType, &VarId);
			switch (*ValType) {
				case TypIntArray:
					if (GetIndex(&Index, Err)) {
						TIntAry *intAry = &Variables[VarId].Value.IntAry;
						if (Index >= 0 && Index < intAry->size) {
							*ValType = TypInteger;
						}
						else {
							*ValType = TypUnknown;
						}
					}
					break;
				case TypStrArray:
					if (GetIndex(&Index, Err)) {
						VarId = GetStrVarFromArray(VarId, Index, Err);
						if (*Err == 0) {
							*ValType = TypString;
						}
						else {
							*ValType = TypUnknown;
						}
					}
					break;
			}
		}
	}
	else {
		*ValType = TypUnknown;
	}
	
	*Err = 0;
}

BOOL GetIndex(int *Index, LPWORD Err)
{
	WORD P;
	int Idx;

	*Err = 0;
	P = LinePtr;
	if (GetFirstChar() == '[') {
		GetIntVal(&Idx, Err);
		if (*Err == 0) {
			if (GetFirstChar() == ']') {
				*Index = Idx;
				return TRUE;
			}
			*Err = ErrCloseBracket;
		}
	}
	LinePtr = P;
	return FALSE;
}

TVarId GetIntVarFromArray(TVarId VarId, int Index, LPWORD Err)
{
	TIntAry *intAry = &Variables[VarId].Value.IntAry;
	if (Index < 0 || Index >= intAry->size) {
		*Err = ErrOutOfRange;
		return -1;
	}
	*Err = 0;
	return ((VarId+1) << 16) | Index;
}

TVarId GetStrVarFromArray(TVarId VarId, int Index, LPWORD Err)
{
	TStrAry *strAry = &Variables[VarId].Value.StrAry;
	if (Index < 0 || Index >= strAry->size) {
		*Err = ErrOutOfRange;
		return -1;
	}
	*Err = 0;
	return ((VarId+1) << 16) | Index;
}

void GetAryVar(PVarId VarId, WORD VarType, LPWORD Err)
{
	TName Name;

	if (*Err!=0) return;

	if (GetIdentifier(Name)) {
		GetAryVarByName(VarId, Name, VarType, Err);
	}
	else {
		*Err = ErrSyntax;
	}
}

void GetAryVarByName(PVarId VarId, const char *Name, WORD VarType, LPWORD Err)
{
	WORD typ;

	if (CheckVar(Name, &typ, VarId)) {
		if (typ != VarType) {
			*Err = ErrTypeMismatch;
		}
	}
	else {
		*Err = ErrVarNotInit;
	}
}

void SetIntValInArray(TVarId VarId, int Index, int Val, LPWORD Err)
{
	TVarId id;

	id = GetIntVarFromArray(VarId, Index, Err);
	if (*Err == 0) {
		SetIntVal(id, Val);
	}
}

void SetStrValInArray(TVarId VarId, int Index, PCHAR Str, LPWORD Err)
{
	TVarId id;

	id = GetStrVarFromArray(VarId, Index, Err);
	if (*Err == 0) {
		SetStrVal(id, Str);
	}
}

int GetIntAryVarSize(TVarId VarId)
{
	TIntAry *intAry = &Variables[VarId].Value.IntAry;
	return intAry->size;
}

int GetStrAryVarSize(TVarId VarId)
{
	TIntAry *strAry = &Variables[VarId].Value.IntAry;
	return strAry->size;
}
