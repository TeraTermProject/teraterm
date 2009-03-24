// Tera Term
// Copyright(C) 1994-1998 T. Teranishi
// All rights reserved.

// TTMACRO.EXE, TTL parser

#include "teraterm.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "ttmdlg.h"
#include "ttmparse.h"


WORD TTLStatus = 0;
char LineBuff[MaxLineLen]; // 行バッファのサイズを拡張した。(2007.6.9 maya)
WORD LinePtr;
WORD LineLen;

// 変数の個数を128->256、ラベルの個数を256->512へ拡張した。(2006.2.1 yutaka)
// 変数の個数を、InitTTL で作っているシステム変数の分だけ追加した。(2006.7.26 maya)
#define MaxNumOfIntVar (LONG)(128*2+3)
#define MaxNumOfStrVar (LONG)(128*2+19)
#define MaxNumOfLabVar (LONG)256*2

#define IntVarIdOff (LONG)0
#define StrVarIdOff (IntVarIdOff+MaxNumOfIntVar)
#define LabVarIdOff (StrVarIdOff+MaxNumOfStrVar)
#define MaxNumOfName (MaxNumOfIntVar+MaxNumOfStrVar+MaxNumOfLabVar)
#define NameBuffLen MaxNumOfName*MaxNameLen
#define StrBuffLen MaxNumOfStrVar*MaxStrLen

static int IntVal[MaxNumOfIntVar];
static BINT LabVal[MaxNumOfLabVar];
static BYTE LabLevel[MaxNumOfLabVar];

static HANDLE HNameBuff;
static PCHAR NameBuff;
static HANDLE HStrBuff;
static PCHAR StrBuff;
static WORD IntVarCount, StrVarCount, LabVarCount;


BOOL InitVar()
{
	HNameBuff = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,NameBuffLen);
	if (HNameBuff==NULL) return FALSE;
	NameBuff = NULL;

	HStrBuff = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,StrBuffLen);
	if (HStrBuff==NULL) return FALSE;
	StrBuff = NULL;

	IntVarCount = 0;
	LabVarCount = 0;
	StrVarCount = 0;
	return TRUE;
}

void EndVar()
{
	UnlockVar();
	GlobalFree(HNameBuff);
	GlobalFree(HStrBuff);
}

void DispErr(WORD Err)
{
	char Msg[41];
	int i;

	switch (Err) {
		case ErrCloseParent: strncpy_s(Msg, sizeof(Msg),"\")\" expected.", _TRUNCATE); break;
		case ErrCantCall: strncpy_s(Msg, sizeof(Msg),"Can't call sub.", _TRUNCATE); break;
		case ErrCantConnect: strncpy_s(Msg, sizeof(Msg),"Can't link macro.", _TRUNCATE); break;
		case ErrCantOpen: strncpy_s(Msg, sizeof(Msg),"Can't open file.", _TRUNCATE); break;
		case ErrDivByZero: strncpy_s(Msg, sizeof(Msg),"Divide by zero.", _TRUNCATE); break;
		case ErrInvalidCtl: strncpy_s(Msg, sizeof(Msg),"Invalid control.", _TRUNCATE); break;
		case ErrLabelAlreadyDef: strncpy_s(Msg, sizeof(Msg),"Label already defined.", _TRUNCATE); break;
		case ErrLabelReq: strncpy_s(Msg, sizeof(Msg),"Label requiered.", _TRUNCATE); break;
		case ErrLinkFirst: strncpy_s(Msg, sizeof(Msg),"Link macro first. Use 'connect' macro.", _TRUNCATE); break;
		case ErrStackOver: strncpy_s(Msg, sizeof(Msg),"Stack overflow.", _TRUNCATE); break;
		case ErrSyntax: strncpy_s(Msg, sizeof(Msg),"Syntax error.", _TRUNCATE); break;
		case ErrTooManyLabels: strncpy_s(Msg, sizeof(Msg),"Too many labels.", _TRUNCATE); break;
		case ErrTooManyVar: strncpy_s(Msg, sizeof(Msg),"Too many variables.", _TRUNCATE); break;
		case ErrTypeMismatch: strncpy_s(Msg, sizeof(Msg),"Type mismatch.", _TRUNCATE); break;
		case ErrVarNotInit: strncpy_s(Msg, sizeof(Msg),"Variable not initialized.", _TRUNCATE); break;
	}

	i = OpenErrDlg(Msg,LineBuff);
	if (i==IDOK) TTLStatus = IdTTLEnd;
}

void LockVar()
{
	if (NameBuff==NULL)
		NameBuff = GlobalLock(HNameBuff);
	if (NameBuff==NULL)
		PostQuitMessage(0);

	if (StrBuff==NULL)
		StrBuff = GlobalLock(HStrBuff);
	if (StrBuff==NULL)
		PostQuitMessage(0);
}

void UnlockVar()
{
	if (NameBuff!=NULL)
		GlobalUnlock(HNameBuff);
	NameBuff = NULL;

	if (StrBuff!=NULL)
		GlobalUnlock(HStrBuff);
	StrBuff = NULL;
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
		break;
	case 'c':
		if (_stricmp(Str,"call")==0) *WordId = RsvCall;
		else if (_stricmp(Str,"callmenu")==0) *WordId = RsvCallMenu;
		else if (_stricmp(Str,"changedir")==0) *WordId = RsvChangeDir;
		else if (_stricmp(Str,"clearscreen")==0) *WordId = RsvClearScreen;
		else if (_stricmp(Str,"clipb2var")==0) *WordId = RsvClipb2Var;  // add 'clipb2var' (2006.9.17 maya)
		else if (_stricmp(Str,"closesbox")==0) *WordId = RsvCloseSBox;
		else if (_stricmp(Str,"closett")==0) *WordId = RsvCloseTT;
		else if (_stricmp(Str,"code2str")==0) *WordId = RsvCode2Str;
		else if (_stricmp(Str,"connect")==0) *WordId = RsvConnect;
		else if (_stricmp(Str,"crc32")==0) *WordId = RsvCrc32;
		else if (_stricmp(Str,"crc32file")==0) *WordId = RsvCrc32File;
		else if (_stricmp(Str,"cygconnect")==0) *WordId = RsvCygConnect;
		break;
	case 'd':
		if (_stricmp(Str,"delpassword")==0) *WordId = RsvDelPassword;
		else if (_stricmp(Str,"disconnect")==0) *WordId = RsvDisconnect;
		else if (_stricmp(Str,"do")==0) *WordId = RsvDo;
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
		break;
	case 'f':
		if (_stricmp(Str,"fileclose")==0) *WordId = RsvFileClose;
		else if (_stricmp(Str,"fileconcat")==0) *WordId = RsvFileConcat;
		else if (_stricmp(Str,"filecopy")==0) *WordId = RsvFileCopy;
		else if (_stricmp(Str,"filecreate")==0) *WordId = RsvFileCreate;
		else if (_stricmp(Str,"filedelete")==0) *WordId = RsvFileDelete;
		else if (_stricmp(Str,"filemarkptr")==0) *WordId = RsvFileMarkPtr;
		else if (_stricmp(Str,"filenamebox")==0) *WordId  = RsvFilenameBox; // add 'filenamebox' (2007.9.13 maya)
		else if (_stricmp(Str,"fileopen")==0) *WordId = RsvFileOpen;
		else if (_stricmp(Str,"filereadln")==0) *WordId = RsvFileReadln;
		else if (_stricmp(Str,"fileread")==0) *WordId = RsvFileRead;    // add
		else if (_stricmp(Str,"filerename")==0) *WordId = RsvFileRename;
		else if (_stricmp(Str,"filesearch")==0) *WordId = RsvFileSearch;
		else if (_stricmp(Str,"fileseek")==0) *WordId = RsvFileSeek;
		else if (_stricmp(Str,"fileseekback")==0) *WordId = RsvFileSeekBack;
		else if (_stricmp(Str,"filestrseek")==0) *WordId = RsvFileStrSeek;
		else if (_stricmp(Str,"filestrseek2")==0) *WordId = RsvFileStrSeek2;
		else if (_stricmp(Str,"filewrite")==0) *WordId = RsvFileWrite;
		else if (_stricmp(Str,"filewriteln")==0) *WordId = RsvFileWriteLn;
		else if (_stricmp(Str,"findclose")==0) *WordId = RsvFindClose;
		else if (_stricmp(Str,"findfirst")==0) *WordId = RsvFindFirst;
		else if (_stricmp(Str,"findnext")==0) *WordId = RsvFindNext;
		else if (_stricmp(Str,"flushrecv")==0) *WordId = RsvFlushRecv;
		else if (_stricmp(Str,"for")==0) *WordId = RsvFor;
		break;
	case 'g':
		if (_stricmp(Str,"getdate")==0) *WordId = RsvGetDate;
		else if (_stricmp(Str,"getdir")==0) *WordId = RsvGetDir;
		else if (_stricmp(Str,"getenv")==0) *WordId = RsvGetEnv;
		else if (_stricmp(Str,"gethostname")==0) *WordId = RsvGetHostname;
		else if (_stricmp(Str,"getpassword")==0) *WordId = RsvGetPassword;
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
		break;
	case 'k':
		if (_stricmp(Str,"kmtfinish")==0) *WordId = RsvKmtFinish;
		else if (_stricmp(Str,"kmtget")==0) *WordId = RsvKmtGet;
		else if (_stricmp(Str,"kmtrecv")==0) *WordId = RsvKmtRecv;
		else if (_stricmp(Str,"kmtsend")==0) *WordId = RsvKmtSend;
		break;
	case 'l':
		if (_stricmp(Str,"loadkeymap")==0) *WordId = RsvLoadKeyMap;
		else if (_stricmp(Str,"logclose")==0) *WordId = RsvLogClose;
		else if (_stricmp(Str,"logopen")==0) *WordId = RsvLogOpen;
		else if (_stricmp(Str,"logpause")==0) *WordId = RsvLogPause;
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
		else if (_stricmp(Str,"sendmulticast")==0) *WordId = RsvSendMulticast;
		else if (_stricmp(Str,"setmulticastname")==0) *WordId = RsvSetMulticastName;
		else if (_stricmp(Str,"sendfile")==0) *WordId = RsvSendFile;
		else if (_stricmp(Str,"sendkcode")==0) *WordId = RsvSendKCode;
		else if (_stricmp(Str,"sendln")==0) *WordId = RsvSendLn;
		else if (_stricmp(Str,"setbaud")==0) *WordId = RsvSetBaud;
		else if (_stricmp(Str,"setdate")==0) *WordId = RsvSetDate;
		else if (_stricmp(Str,"setdir")==0) *WordId = RsvSetDir;
		else if (_stricmp(Str,"setdlgpos")==0) *WordId = RsvSetDlgPos;
		else if (_stricmp(Str,"setdtr")==0) *WordId = RsvSetDtr;    // add 'setdtr'  (2008.3.12 maya)
		else if (_stricmp(Str,"setecho")==0) *WordId = RsvSetEcho;
		else if (_stricmp(Str,"setenv")==0) *WordId = RsvSetEnv;    // reactivate 'setenv' (2007.8.31 maya)
		else if (_stricmp(Str,"setexitcode")==0) *WordId = RsvSetExitCode;
		else if (_stricmp(Str,"setrts")==0) *WordId = RsvSetRts;    // add 'setrts'  (2008.3.12 maya)
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
		else if (_stricmp(Str,"strlen")==0) *WordId = RsvStrLen;
		else if (_stricmp(Str,"strmatch")==0) *WordId = RsvStrMatch;
		else if (_stricmp(Str,"strscan")==0) *WordId = RsvStrScan;
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

BYTE GetFirstChar()
{
	BYTE b;

	if (LinePtr<LineLen)
		b = LineBuff[LinePtr];
	else return 0;

	while ((LinePtr<LineLen) && ((b==' ') || (b=='\t')))
	{
		LinePtr++;
		if (LinePtr<LineLen) b = LineBuff[LinePtr];
	}
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

	int GetQuotedStr(PCHAR Str, BYTE q, LPWORD i)
	{
		BYTE b;

		b=0;
		if (LinePtr<LineLen) b = LineBuff[LinePtr];
		while ((LinePtr<LineLen) && (b>=' ') && (b!=q))
		{
			if (*i<MaxStrLen-1)
			{
				Str[*i] = b;
				(*i)++;
			}

			LinePtr++;
			if (LinePtr<LineLen) b = LineBuff[LinePtr];
		}
		if (b==q)
			if (LinePtr<LineLen) LinePtr++;
		else
			return (ErrSyntax);

		return 0;
	}

	WORD GetCharByCode(PCHAR Str, LPWORD i)
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

BOOL CheckVar(PCHAR Name, LPWORD VarType, LPWORD VarId)
{
	int i;
	long P;

	*VarType = TypUnknown;

	i = 0;
	while (i<IntVarCount)
	{
		P = (i+IntVarIdOff)*MaxNameLen;
		if (_stricmp(&NameBuff[P],Name)==0)
		{
			*VarType = TypInteger;
			*VarId = (WORD)i;
			return TRUE;
		}
		i++;
	}

	i = 0;
	while (i<StrVarCount)
	{
		P = (i+StrVarIdOff)*MaxNameLen;
		if (_stricmp(&NameBuff[P],Name)==0)
		{
			*VarType = TypString;
			*VarId = i;
			return TRUE;
		}
		i++;
	}

	i = 0;
	while (i<LabVarCount)
	{
		P = (i+LabVarIdOff)*MaxNameLen;
		if (_stricmp(&NameBuff[P],Name)==0)
		{
			*VarType = TypLabel;
			*VarId = i;
			return TRUE;
		}
		i++;
	}

	return FALSE;
}

BOOL NewIntVar(PCHAR Name, int InitVal)
{
	long P;

	if (IntVarCount>=MaxNumOfIntVar) return FALSE;
	P = (IntVarIdOff+IntVarCount)*MaxNameLen;
	strncpy_s(&NameBuff[P],MaxNameLen,Name,_TRUNCATE);
	IntVal[IntVarCount] = InitVal;
	IntVarCount++;
	return TRUE;
}

BOOL NewStrVar(PCHAR Name, PCHAR InitVal)
{
	long P;

	if (StrVarCount>=MaxNumOfStrVar) return FALSE;
	P = (StrVarIdOff+StrVarCount)*MaxNameLen;
	strncpy_s(&NameBuff[P],MaxNameLen,Name,_TRUNCATE);
	P = StrVarCount*MaxStrLen;
	strncpy_s(&StrBuff[P],MaxStrLen,InitVal,_TRUNCATE);
	StrVarCount++;
	return TRUE;
}

BOOL NewLabVar(PCHAR Name, BINT InitVal, WORD ILevel)
{
	long P;

	if (LabVarCount>=MaxNumOfLabVar) return FALSE;

	P = (LabVarIdOff+LabVarCount)*MaxNameLen;
	strncpy_s(&(NameBuff[P]),MaxNameLen,Name,_TRUNCATE);
	LabVal[LabVarCount] = InitVal;
	LabLevel[LabVarCount] = LOBYTE(ILevel);
	LabVarCount++;
	return TRUE;
}

void DelLabVar(WORD ILevel)
{
	while ((LabVarCount>0) && (LabLevel[LabVarCount-1]>=ILevel))
		LabVarCount--;
}

void CopyLabel(WORD ILabel, BINT far *Ptr, LPWORD Level)
{
	*Ptr = LabVal[ILabel];
	*Level = (WORD)LabLevel[ILabel];
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
	WORD P, Id;

	P = LinePtr;
	*Err = 0;
	if (GetIdentifier(Name)) {
		if (CheckReservedWord(Name,&Id)) {
			if (GetFactor(ValType, Val, Err)) {
				if ((*Err==0) && (*ValType!=TypInteger))
					*Err = ErrTypeMismatch;
				switch (Id) {
					case RsvBNot: *Val = ~(*Val); break;
					case RsvLNot: *Val = !(*Val); break;
					default: *Err = ErrSyntax;
				}
			}
			else {
				*Err = ErrSyntax;
			}
		}
		else if (CheckVar(Name, ValType, &Id)) {
			switch (*ValType) {
				case TypInteger: *Val = IntVal[Id]; break;
				case TypString: *Val = Id; break;
			}
		}
		else
			*Err = ErrVarNotInit;
	}
	else if (GetNumber(Val))
		*ValType = TypInteger;
	else if (GetOperator(&Id)) {
		if (GetFactor(ValType, Val, Err)) {
			if ((*Err==0) && (*ValType != TypInteger))
				*Err = ErrTypeMismatch;
			switch (Id) {
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

void SetIntVal(WORD VarId, int Val)
{
	IntVal[VarId] = Val;
}

int CopyIntVal(WORD VarId)
{
	return IntVal[VarId];
}

void GetIntVar(LPWORD VarId, LPWORD Err)
{
	TName Name;
	WORD VarType;

	if (*Err!=0) return;

	if (GetIdentifier(Name))
	{
		if (CheckVar(Name,&VarType,VarId))
		{
			if (VarType!=TypInteger)
				*Err = ErrTypeMismatch;
		}
		else {
			if (NewIntVar(Name,0))
				CheckVar(Name,&VarType,VarId);
			else
				*Err = ErrTooManyVar;
		}
	}
	else
		*Err = ErrSyntax;
}

void GetStrVal(PCHAR Str, LPWORD Err)
{
	GetStrVal2(Str, Err, FALSE);
}

void GetStrVal2(PCHAR Str, LPWORD Err, BOOL AutoConversion)
{
	WORD VarType;
	int VarId;

	Str[0] = 0;
	if (*Err!=0) return;

	if (GetString(Str,Err))
		return;
	else if (GetExpression(&VarType,&VarId,Err))
	{
		if (*Err!=0) return;
		switch (VarType) {
			case TypString:
				strncpy_s(Str,MaxStrLen,&StrBuff[VarId*MaxStrLen],_TRUNCATE);
				break;
			case TypInteger:
				if (AutoConversion)
					_snprintf_s(Str,MaxStrLen,_TRUNCATE,"%d",VarId);
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

void GetStrVar(LPWORD VarId, LPWORD Err)
{
	TName Name;
	WORD VarType;

	if (*Err!=0) return;

	if (GetIdentifier(Name))
	{
		if (CheckVar(Name,&VarType,VarId))
		{
			if (VarType!=TypString)
				*Err = ErrTypeMismatch;
		}
		else {
			if (NewStrVar(Name,""))
				CheckVar(Name,&VarType,VarId);
			else
				*Err = ErrTooManyVar;
		}
	}
	else
		*Err = ErrSyntax;
}

void SetStrVal(WORD VarId, PCHAR Str)
{
	// StrBuf の運用上 MaxStrLen が正しいサイズなのでサイズを固定
	// (2007.6.23 maya)
	strncpy_s(&StrBuff[VarId*MaxStrLen],MaxStrLen,Str,_TRUNCATE);
}

PCHAR StrVarPtr(WORD VarId)
{
	return (&StrBuff[VarId*MaxStrLen]);
}

// for ifdefined (2006.9.23 maya)
void GetVarType(LPWORD ValType, int far *Val, LPWORD Err)
{
	GetFactor(ValType,Val,Err);
	
	if (*Err == ErrVarNotInit) {
		ValType = TypUnknown;
	}

	*Err = 0;
}
