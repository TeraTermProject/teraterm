/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2018 TeraTerm Project
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

/* TTMACRO.EXE, Tera Term Language interpreter */

#include "teraterm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mbstring.h>
#include <time.h>
#include <errno.h>
#include "tt-version.h"
#include "ttmdlg.h"
#include "ttmbuff.h"
#include "ttmparse.h"
#include "ttmdde.h"
#include "ttmlib.h"
#include "ttlib.h"
#include "ttmenc.h"
#include "tttypes.h"
#include "ttmonig.h"
#include <shellapi.h>
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>

// for _findXXXX() functions
#include <io.h>

// for _ismbblead
#include <mbctype.h>

#include "ttl.h"
#include "SFMT.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iptypes.h>
#include <iphlpapi.h>
#include "win16api.h"

#define TTERMCOMMAND "TTERMPRO /D="
#define CYGTERMCOMMAND "cyglaunch -o /D="

// for 'ExecCmnd' command
static BOOL ParseAgain;
static int IfNest;
static int ElseFlag;
static int EndIfFlag;
// Window handle of the main window
static HWND HMainWin;
// Timeout
static DWORD TimeLimit;
static DWORD TimeStart;
// for 'WaitEvent' command
static int WakeupCondition;

// exit code of TTMACRO
int ExitCode = 0;

// for "FindXXXX" commands
#define NumDirHandle 8
static intptr_t DirHandle[NumDirHandle] = {-1,-1, -1, -1, -1, -1, -1, -1};
/* for "FileMarkPtr" and "FileSeekBack" commands */
#define NumFHandle 16
static HANDLE FHandle[NumFHandle];
static long FPointer[NumFHandle];

// forward declaration
int ExecCmnd();

static void HandleInit()
{
	int i;
	for (i=0; i<_countof(FHandle); i++) {
		FHandle[i] = INVALID_HANDLE_VALUE;
	}
}

/**
 *	@retval	ファイルハンドルインデックス(0～)
 *			-1のときエラー
 */
static int HandlePut(HANDLE FH)
{
	int i;
	if (FH == INVALID_HANDLE_VALUE) {
		return -1;
	}
	for (i=0; i<_countof(FHandle); i++) {
		if (FHandle[i] == INVALID_HANDLE_VALUE) {
			FHandle[i] = FH;
			FPointer[i] = 0;
			return i;
		}
	}
	return -1;
}

static HANDLE HandleGet(int fhi)
{
	if (fhi < 0 || _countof(FHandle) < fhi) {
		return INVALID_HANDLE_VALUE;
	}
	return FHandle[fhi];
}

static void HandleFree(int fhi)
{
	FHandle[fhi] = INVALID_HANDLE_VALUE;
}

BOOL InitTTL(HWND HWin)
{
	int i;
	TStrVal Dir;
	TVarId ParamsVarId;
	char tmpname[10];
	WORD Err;

	HMainWin = HWin;

	if (! InitVar()) return FALSE;
	LockVar();

	// System variables
	NewIntVar("result",0);
	NewIntVar("timeout",0);
	NewIntVar("mtimeout",0);    // ミリ秒単位のタイムアウト用 (2009.1.23 maya)
	NewStrVar("inputstr","");
	NewStrVar("matchstr","");   // for 'waitregex' command (2005.10.7 yutaka)
	NewStrVar("groupmatchstr1","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr2","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr3","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr4","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr5","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr6","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr7","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr8","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr9","");   // for 'waitregex' command (2005.10.15 yutaka)

	if (ParamCnt == 0) {
		ParamCnt++;
	}
	NewIntVar("paramcnt",ParamCnt);  // ファイル名も含む引数の個数 (2012.4.10 yutaka)

	// 旧形式のパラメータ設定 (param1 ～ param9)
	NewStrVar("param1", ShortName);
	if (Params) {
		for (i=2; i<=9; i++) {
			_snprintf_s(tmpname, sizeof(tmpname), _TRUNCATE, "param%d", i);
			if (ParamCnt >= i && Params[i] != NULL) {
				NewStrVar(tmpname, Params[i]);
			}
			else {
				NewStrVar(tmpname, "");
			}
		}
	}

	// 新形式のパラメータ設定 (params[1～ParamCnt])
	if (NewStrAryVar("params", ParamCnt+1) == 0) {
		Err = 0;
		GetStrAryVarByName(&ParamsVarId, "params", &Err);
		if (Err == 0) {
			if (ShortName[0] != 0) {
				SetStrValInArray(ParamsVarId, 1, ShortName, &Err);
			}
			if (Params) {
				for (i=0; i<=ParamCnt; i++) {
					if (i == 1) {
						continue;
					}
					if (Params[i]) {
						SetStrValInArray(ParamsVarId, i, Params[i], &Err);
						free(Params[i]);
					}
				}
				free(Params);
				Params = NULL;
			}
		}
	}

	ParseAgain = FALSE;
	IfNest = 0;
	ElseFlag = 0;
	EndIfFlag = 0;

	for (i=0; i<NumDirHandle; i++)
		DirHandle[i] = -1L;
	HandleInit();

	if (! InitBuff(FileName))
	{
		TTLStatus = IdTTLEnd;
		return FALSE;
	}

	UnlockVar();

	ExtractDirName(FileName,Dir);
	TTMSetDir(Dir);

	if (SleepFlag)
	{ // synchronization for startup macro
		// sleep until Tera Term is ready
		WakeupCondition = IdWakeupInit;
		// WakeupCondition = IdWakeupConnect | IdWakeupDisconn | IdWakeupUnlink;
		TTLStatus = IdTTLSleep;
	}
	else
		TTLStatus = IdTTLRun;

	return TRUE;
}

void EndTTL()
{
	int i;

	CloseStatDlg();

	for (i=0; i<NumDirHandle; i++)
	{
		if (DirHandle[i]!=-1L)
			_findclose(DirHandle[i]);
		DirHandle[i] = -1L;
	}

	UnlockVar();
	if (TTLStatus==IdTTLWait)
		KillTimer(HMainWin,IdTimeOutTimer);
	CloseBuff(0);
	EndVar();
}

long int CalcTime()
{
	time_t time1;
	struct tm *ptm;

	time1 = time(NULL);
	ptm = localtime(&time1);
	return ((long int)ptm->tm_hour * 3600 +
	        (long int)ptm->tm_min * 60 +
	        (long int)ptm->tm_sec
	       );
}

//////////////// Beginning of TTL commands //////////////
WORD TTLCommCmd(char Cmd, int Wait)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	else if (! Linked)
		return ErrLinkFirst;
	else
		return SendCmnd(Cmd,Wait);
}

WORD TTLCommCmdFile(char Cmd, int Wait)
{
	TStrVal Str;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);

	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		SetFile(Str);
		Err = SendCmnd(Cmd,Wait);
	}
	return Err;
}

WORD TTLCommCmdBin(char Cmd, int Wait)
{
	int Val;
	WORD Err;

	Err = 0;
	GetIntVal(&Val,&Err);

	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		SetBinary(Val);
		Err = SendCmnd(Cmd,Wait);
	}
	return Err;
}


WORD TTLCommCmdDeb()
{
	int Val;
	WORD Err;

	Err = 0;
	GetIntVal(&Val,&Err);

	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		SetDebug(Val);
	}
	return Err;
}

WORD TTLCommCmdInt(char Cmd, int Wait)
{
	int Val;
	char Str[21];
	WORD Err;

	Err = 0;
	GetIntVal(&Val,&Err);

	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		_snprintf_s(Str,sizeof(Str),_TRUNCATE,"%d",Val);
		SetFile(Str);
		Err = SendCmnd(Cmd,Wait);
	}
	return Err;
}

WORD TTLBeep()
{
	int val = 0;
	WORD Err = 0;
	UINT type = MB_OK;

	if (CheckParameterGiven()) {
		GetIntVal(&val, &Err);
		if (Err!=0) return Err;

		switch (val) {
		case 0:
			type = -1;
			break;
		case 1:
			type = MB_ICONASTERISK;
			break;
		case 2:
			type = MB_ICONEXCLAMATION;
			break;
		case 3:
			type = MB_ICONHAND;
			break;
		case 4:
			type = MB_ICONQUESTION;
			break;
		case 5:
			type = MB_OK;
			break;
		default:
			return ErrSyntax;
			break;
		}
	}

	MessageBeep(type);

	return 0;
}

WORD TTLBreak(WORD WId) {
	if (GetFirstChar()!=0)
		return ErrSyntax;

	return BreakLoop(WId);
}

WORD TTLBringupBox()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	BringupStatDlg();
	return 0;
}

WORD TTLCall()
{
	TName LabName;
	WORD Err, VarType;
	TVarId VarId;

	if (GetLabelName(LabName) && (GetFirstChar()==0)) {
		if (CheckVar(LabName, &VarType, &VarId) && (VarType==TypLabel))
			Err = CallToLabel(VarId);
		else
			Err = ErrLabelReq;
	}
	else
	Err = ErrSyntax;

	return Err;
}

// add 'clipb2var' (2006.9.17 maya)
WORD TTLClipb2Var()
{
	WORD Err;
	TVarId VarId;
	HANDLE hText;
	PTSTR clipbText;
	char buf[MaxStrLen];
	int Num = 0;
	char *newbuff;
	static char *cbbuff;
	static int cbbuffsize, cblen;
	HANDLE wide_hText;
	LPWSTR wide_buf;
	int mb_len;
	UINT Cf;

	Err = 0;
	GetStrVar(&VarId, &Err);
	if (Err!=0) return Err;

	// get 2nd arg(optional) if given
	if (CheckParameterGiven()) {
		GetIntVal(&Num, &Err);
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (Num == 0) {
		if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
			Cf = CF_UNICODETEXT;
		}
		else if (IsClipboardFormatAvailable(CF_TEXT)) {
			Cf = CF_TEXT;
		}
		else {
			cblen = 0;
			SetResult(0);
			return Err;
		}
		if (OpenClipboard(NULL) == 0) {
			cblen = 0;
			SetResult(0);
			return Err;
		}

		if (Cf == CF_UNICODETEXT) {
			wide_hText = GetClipboardData(CF_UNICODETEXT);
			if (wide_hText != NULL) {
				wide_buf = GlobalLock(wide_hText);
				mb_len = WideCharToMultiByte(CP_ACP, 0, wide_buf, -1, NULL, 0, NULL, NULL);
				hText = GlobalAlloc(GMEM_MOVEABLE, sizeof(CHAR) * mb_len);
				clipbText = GlobalLock(hText);
				if (hText != NULL) {
					WideCharToMultiByte(CP_ACP, 0, wide_buf, -1, clipbText, mb_len, NULL, NULL);

					cblen = strlen(clipbText);
					if (cbbuffsize <= cblen) {
						if ((newbuff = realloc(cbbuff, cblen + 1)) == NULL) {
							// realloc failed. fall back to old mode.
							cblen = 0;
							strncpy_s(buf,sizeof(buf),clipbText,_TRUNCATE);
							GlobalUnlock(hText);
							CloseClipboard();
							SetStrVal(VarId, buf);
							SetResult(3);
							return Err;
						}
						cbbuff = newbuff;
						cbbuffsize = cblen + 1;
					}
					strncpy_s(cbbuff, cbbuffsize, clipbText, _TRUNCATE);

					GlobalUnlock(hText);
					GlobalFree(hText);
				}
				GlobalUnlock(wide_hText);
			}
			else {
				cblen = 0;
			}
		}
		else if (Cf == CF_TEXT) {
			hText = GetClipboardData(CF_TEXT);
			if (hText != NULL) {
				clipbText = GlobalLock(hText);
				cblen = strlen(clipbText);
				if (cbbuffsize <= cblen) {
					if ((newbuff = realloc(cbbuff, cblen + 1)) == NULL) {
						// realloc failed. fall back to old mode.
						cblen = 0;
						strncpy_s(buf,sizeof(buf),clipbText,_TRUNCATE);
						GlobalUnlock(hText);
						CloseClipboard();
						SetStrVal(VarId, buf);
						SetResult(3);
						return Err;
					}
					cbbuff = newbuff;
					cbbuffsize = cblen + 1;
				}
				strncpy_s(cbbuff, cbbuffsize, clipbText, _TRUNCATE);
				GlobalUnlock(hText);
			}
			else {
				cblen = 0;
			}
		}
		CloseClipboard();
	}

	if (cbbuff != NULL && Num >= 0 && Num * (MaxStrLen - 1) < cblen) {
		if (strncpy_s(buf ,sizeof(buf), cbbuff + Num * (MaxStrLen-1), _TRUNCATE) == STRUNCATE)
			SetResult(2); // Copied string is truncated.
		else {
			SetResult(1);
		}
		SetStrVal(VarId, buf);
	}
	else {
		SetResult(0);
	}

	return Err;
}

// add 'var2clipb' (2006.9.17 maya)
WORD TTLVar2Clipb()
{
	WORD Err;
	TStrVal Str;
	HGLOBAL hText;
	PTSTR clipbText;
	int wide_len;
	HGLOBAL wide_hText;
	LPWSTR wide_buf;

	Err = 0;
	GetStrVal(Str,&Err);
	if (Err!=0) return Err;

	hText = GlobalAlloc(GHND, sizeof(Str));
	clipbText = GlobalLock(hText);
	strncpy_s(clipbText, sizeof(Str), Str, _TRUNCATE);
	GlobalUnlock(hText);

	wide_len = MultiByteToWideChar(CP_ACP, 0, clipbText, -1, NULL, 0);
	wide_hText = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * wide_len);
	if (wide_hText) {
		wide_buf = (LPWSTR)GlobalLock(wide_hText);
		MultiByteToWideChar(CP_ACP, 0, clipbText, -1, wide_buf, wide_len);
		GlobalUnlock(wide_hText);
	}

	if (OpenClipboard(NULL) == 0) {
		SetResult(0);
	}
	else {
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hText);

		if (wide_buf) {
			SetClipboardData(CF_UNICODETEXT, wide_hText);
		}

		CloseClipboard();
		SetResult(1);
	}

	return Err;
}

WORD TTLCloseSBox()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	CloseStatDlg();
	return 0;
}

WORD TTLCloseTT()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (! Linked)
		return ErrLinkFirst;
	else {
		// Close Tera Term
		SendCmnd(CmdCloseWin,IdTTLWaitCmndEnd);
		EndDDE();
		return 0;
	}
}

WORD TTLCode2Str()
{
	TVarId VarId;
	WORD Err;
	int Num, Len, c, i;
	BYTE d;
	TStrVal Str;

	Err = 0;
	GetStrVar(&VarId, &Err);

	GetIntVal(&Num,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Len = sizeof(Num);
	i = 0;
	for (c=0; c<=Len-1; c++)
	{
		d = (Num >> ((Len-1-c)*8)) & 0xff;
		if ((i>0) || (d!=0))
		{
			Str[i] = d;
			i++;
		}
	}
	Str[i] = 0;
	SetStrVal(VarId, Str);
	return Err;
}

WORD TTLConnect(WORD mode)
{
	TStrVal Cmnd, Str;
	WORD Err;
	WORD w;

	Str[0] = 0;
	Err = 0;

	if (mode == RsvConnect || CheckParameterGiven()) {
		GetStrVal(Str,&Err);
		if (Err!=0) return Err;
	}

	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (Linked)
	{
		if (ComReady!=0)
		{
			SetResult(2);
			return Err;
		}

		if (mode == RsvConnect) {
			// new connection
			SetFile(Str);
			SendCmnd(CmdConnect,0);

			WakeupCondition = IdWakeupInit;
			TTLStatus = IdTTLSleep;
			return Err;
		}
		else {	// cygwin connection
			TTLCloseTT();
		}
	}

	SetResult(0);
	// link to Tera Term
	if (strlen(TopicName)==0)
	{
		switch (mode) {
		case RsvConnect:
			strncpy_s(Cmnd, sizeof(Cmnd),TTERMCOMMAND, _TRUNCATE);
			break;
		case RsvCygConnect:
			strncpy_s(Cmnd, sizeof(Cmnd),CYGTERMCOMMAND, _TRUNCATE);
			break;
		}
		w = HIWORD(HMainWin);
		Word2HexStr(w,TopicName);
		w = LOWORD(HMainWin);
		Word2HexStr(w,&(TopicName[4]));
		strncat_s(Cmnd,sizeof(Cmnd),TopicName,_TRUNCATE);
		strncat_s(Cmnd,sizeof(Cmnd)," ",_TRUNCATE);
		strncat_s(Cmnd,sizeof(Cmnd),Str,_TRUNCATE);
		if (WinExec(Cmnd,SW_SHOW)<32)
			return ErrCantConnect;
		TTLStatus = IdTTLInitDDE;
	}
	return Err;
}


/*
 * cf. http://oku.edu.mie-u.ac.jp/~okumura/algo/
 */

enum checksum_type {
	CHECKSUM8,
	CHECKSUM16,
	CHECKSUM32,
	CRC16,
	CRC32
};

static unsigned int checksum32(int n, unsigned char c[])
{
	unsigned long value = 0;
	int i;

	for (i = 0; i < n; i++) {
		value += (c[i] & 0xFF);
	}
	return (value & 0xFFFFFFFF);
}

static unsigned int checksum16(int n, unsigned char c[])
{
	unsigned long value = 0;
	int i;

	for (i = 0; i < n; i++) {
		value += (c[i] & 0xFF);
	}
	return (value & 0xFFFF);
}

static unsigned int checksum8(int n, unsigned char c[])
{
	unsigned long value = 0;
	int i;

	for (i = 0; i < n; i++) {
		value += (c[i] & 0xFF);
	}
	return (value & 0xFF);
}

// CRC-16-CCITT
static unsigned int crc16(int n, unsigned char c[])
{
#define CRC16POLY1  0x1021U  /* x^{16}+x^{12}+x^5+1 */
#define CRC16POLY2  0x8408U  /* 左右逆転 */

	int i, j;
	unsigned long r;

	r = 0xFFFFU;
	for (i = 0; i < n; i++) {
		r ^= c[i];
		for (j = 0; j < CHAR_BIT; j++)
			if (r & 1) r = (r >> 1) ^ CRC16POLY2;
			else       r >>= 1;
	}
	return r ^ 0xFFFFU;
}

static unsigned long crc32(int n, unsigned char c[])
{
#define CRC32POLY1 0x04C11DB7UL
	/* x^{32}+x^{26}+x^{23}+x^{22}+x^{16}+x^{12}+x^{11]+
	   x^{10}+x^8+x^7+x^5+x^4+x^2+x^1+1 */
#define CRC32POLY2 0xEDB88320UL  /* 左右逆転 */
	int i, j;
	unsigned long r;

	r = 0xFFFFFFFFUL;
	for (i = 0; i < n; i++) {
		r ^= c[i];
		for (j = 0; j < CHAR_BIT; j++)
			if (r & 1) r = (r >> 1) ^ CRC32POLY2;
			else       r >>= 1;
	}
	return r ^ 0xFFFFFFFFUL;
}

// チェックサムアルゴリズム・共通ルーチン
WORD TTLDoChecksum(enum checksum_type type)
{
	TStrVal Str;
	WORD Err;
	TVarId VarId;
	DWORD cksum;

	Err = 0;
	GetIntVar(&VarId, &Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	if (Str[0]==0) return Err;

	switch (type) {
		case CHECKSUM8:
			cksum = checksum8(strlen(Str), Str);
			break;
		case CHECKSUM16:
			cksum = checksum16(strlen(Str), Str);
			break;
		case CHECKSUM32:
			cksum = checksum32(strlen(Str), Str);
			break;
		case CRC16:
			cksum = crc16(strlen(Str), Str);
			break;
		case CRC32:
			cksum = crc32(strlen(Str), Str);
			break;
		default:
			cksum = 0;
			break;
	}

	SetIntVal(VarId, cksum);

	return Err;
}

WORD TTLDoChecksumFile(enum checksum_type type)
{
	TStrVal Str;
	int result = 0;
	WORD Err;
	TVarId VarId;
	HANDLE fh = INVALID_HANDLE_VALUE, hMap = NULL;
	LPBYTE lpBuf = NULL;
	DWORD fsize;
	DWORD cksum;

	Err = 0;
	GetIntVar(&VarId, &Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	if (Str[0]==0) return Err;

	fh = CreateFile(Str,GENERIC_READ,0,NULL,OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,NULL); /* ファイルオープン */
	if (fh == INVALID_HANDLE_VALUE) {
		result = -1;
		goto error;
	}
	/* ファイルマッピングオブジェクト作成 */
	hMap = CreateFileMapping(fh,NULL,PAGE_READONLY,0,0,NULL);
	if (hMap == NULL) {
		result = -1;
		goto error;
	}

	/* ファイルをマップし、先頭アドレスをlpBufに取得 */
	lpBuf = (LPBYTE)MapViewOfFile(hMap,FILE_MAP_READ,0,0,0);
	if (lpBuf == NULL) {
		result = -1;
		goto error;
	}

	fsize = GetFileSize(fh,NULL);

	switch (type) {
		case CHECKSUM8:
			cksum = checksum8(fsize, lpBuf);
			break;
		case CHECKSUM16:
			cksum = checksum16(fsize, lpBuf);
			break;
		case CHECKSUM32:
			cksum = checksum32(fsize, lpBuf);
			break;
		case CRC16:
			cksum = crc16(fsize, lpBuf);
			break;
		case CRC32:
			cksum = crc32(fsize, lpBuf);
			break;
		default:
			cksum = 0;
			break;
	}

	SetIntVal(VarId, cksum);

error:
	if (lpBuf != NULL) {
		UnmapViewOfFile(lpBuf);
	}

	if (hMap != NULL) {
		CloseHandle(hMap);
	}
	if (fh != INVALID_HANDLE_VALUE) {
		CloseHandle(fh);
	}

	SetResult(result);

	return Err;
}

WORD TTLDelPassword()
{
	TStrVal Str, Str2;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	if (Str[0]==0) return Err;

	GetAbsPath(Str,sizeof(Str));
	if (! DoesFileExist(Str)) return Err;
	if (Str2[0]==0) // delete all password
		WritePrivateProfileString("Password",NULL,NULL,Str);
	else            // delete password specified by Str2
		WritePrivateProfileString("Password",Str2,NULL,Str);
	return Err;
}

WORD TTLDim(WORD type)
{
	WORD Err, WordId, VarType;
	TName Name;
	TVarId VarId;
	int size;

	Err = 0;

	if (! GetIdentifier(Name)) return ErrSyntax;
	if (CheckReservedWord(Name, &WordId)) return ErrSyntax;
	if (CheckVar(Name, &VarType, &VarId)) return ErrSyntax;

	GetIntVal(&size, &Err);
	if (Err!=0) return Err;

	if (type == RsvIntDim) {
		Err = NewIntAryVar(Name, size);
	}
	else { // type == RsvStrDim
		Err = NewStrAryVar(Name, size);
	}
	return Err;
}

WORD TTLDisconnect()
{
	WORD Err;
	int Val = 1;
	char Str[21];

	Err = 0;
	// get 1rd arg(optional) if given
	if (CheckParameterGiven()) {
		GetIntVal(&Val, &Err);
	}

	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		_snprintf_s(Str,sizeof(Str),_TRUNCATE,"%d",Val);
		SetFile(Str);
		Err = SendCmnd(CmdDisconnect,0);
	}
	return Err;
}

WORD TTLDispStr()
{
	TStrVal Str, buff;
	WORD Err, ValType;
	int Val;

	if (! Linked)
		return ErrLinkFirst;

	buff[0] = 0;

	while (TRUE) {
		if (GetString(Str, &Err)) {
			if (Err) return Err;
			strncat_s(buff, MaxStrLen, Str, _TRUNCATE);
		}
		else if (GetExpression(&ValType, &Val, &Err)) {
			if (Err!=0) return Err;
			switch (ValType) {
				case TypInteger:
					Str[0] = LOBYTE(Val);
					Str[1] = 0;
					strncat_s(buff, MaxStrLen, Str, _TRUNCATE);
				case TypString:
					strncat_s(buff, MaxStrLen, StrVarPtr((TVarId)Val), _TRUNCATE);
					break;
				default:
					return ErrTypeMismatch;
			}
		}
		else {
			break;
		}
	}
	SetFile(buff);
	return SendCmnd(CmdDispStr, 0);
}

WORD TTLDo()
{
	WORD WId, Err;
	int Val = 1;

	Err = 0;
	if (CheckParameterGiven()) {
		if (GetReservedWord(&WId)) {
			switch (WId) {
				case RsvWhile:
					GetIntVal(&Val,&Err);
					break;
				case RsvUntil:
					GetIntVal(&Val,&Err);
					Val = Val == 0;
					break;
				default:
					Err = ErrSyntax;
			}
			if ((Err==0) && (GetFirstChar()!=0))
				Err = ErrSyntax;
		}
		else {
			Err = ErrSyntax;
		}
	}

	if (Err!=0) return Err;

	if (Val!=0)
		return SetWhileLoop();
	else
		EndWhileLoop();
	return Err;
}

WORD TTLElse()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (IfNest<1)
		return ErrInvalidCtl;

	// Skip until 'EndIf'
	IfNest--;
	EndIfFlag = 1;
	return 0;
}

int CheckElseIf(LPWORD Err)
{
	int Val;
	WORD WId;

	*Err = 0;
	GetIntVal(&Val,Err);
	if (*Err!=0) return 0;
	if (! GetReservedWord(&WId) ||
	    (WId!=RsvThen) ||
	    (GetFirstChar()!=0))
		*Err = ErrSyntax;
	return Val;
}

WORD TTLElseIf()
{
	WORD Err;
	int Val;

	Val = CheckElseIf(&Err);
	if (Err!=0) return Err;

	if (IfNest<1)
		return ErrInvalidCtl;

	// Skip until 'EndIf'
	IfNest--;
	EndIfFlag = 1;
	return Err;
}

WORD TTLEnd()
{
	if (GetFirstChar()==0)
	{
		TTLStatus = IdTTLEnd;
		return 0;
	}
	else
		return ErrSyntax;
}

WORD TTLEndIf()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (IfNest<1)
		return ErrInvalidCtl;

	IfNest--;
	return 0;
}

WORD TTLEndWhile(BOOL mode)
{
	WORD Err;
	int Val = mode;

	Err = 0;
	if (CheckParameterGiven()) {
		GetIntVal(&Val,&Err);
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	return BackToWhile((Val!=0) == mode);
}

WORD TTLExec()
{
	TStrVal Str,Str2, CurDir;
	int mode = SW_SHOW;
	int wait = 0, ret;
	WORD Err;
	STARTUPINFO sui;
	PROCESS_INFORMATION pi;
	BOOL bRet;

	memset(CurDir, 0, sizeof(CurDir));

	Err = 0;
	GetStrVal(Str,&Err);

	if (CheckParameterGiven()) {
		GetStrVal(Str2, &Err);
		if (Err!=0) return Err;

		if (_stricmp(Str2, "hide") == 0)
			mode = SW_HIDE;
		else if (_stricmp(Str2, "minimize") == 0)
			mode = SW_MINIMIZE;
		else if (_stricmp(Str2, "maximize") == 0)
			mode = SW_MAXIMIZE;
		else if (_stricmp(Str2, "show") == 0)
			mode = SW_SHOW;
		else
			Err = ErrSyntax;

		// get 3nd arg(optional) if given
		if (CheckParameterGiven()) {
			GetIntVal(&wait, &Err);
			if (Err!=0) return Err;

			// get 4th arg(optional) if given
			if (CheckParameterGiven()) {
				GetStrVal(CurDir, &Err);
				if (Err!=0) return Err;
			}
		}
	}

	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	memset(&sui, 0, sizeof(STARTUPINFO));
	sui.cb = sizeof(STARTUPINFO);
	sui.wShowWindow = mode;
	sui.dwFlags = STARTF_USESHOWWINDOW;
	if (CurDir[0] == 0)
		bRet = CreateProcess(NULL, Str, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &sui, &pi);
	else
		bRet = CreateProcess(NULL, Str, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, CurDir, &sui, &pi);
	// TODO: check bRet
	if (wait) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &ret);
		SetResult(ret);
	}
	return Err;
}

WORD TTLExecCmnd()
{
	WORD Err;
	TStrVal NextLine;
	BYTE b;

	Err = 0;
	GetStrVal(NextLine,&Err);
	if (Err!=0) return Err;
	if (GetFirstChar()!=0)
		return ErrSyntax;

	strncpy_s(LineBuff, sizeof(LineBuff),NextLine, _TRUNCATE);
	LineLen = (WORD)strlen(LineBuff);
	LinePtr = 0;
	b = GetFirstChar();
	LinePtr--;
	ParseAgain = (b!=0) && (b!=':') && (b!=';');
	return Err;
}

WORD TTLExit()
{
	if (GetFirstChar()==0)
	{
		if (! ExitBuffer())
			TTLStatus = IdTTLEnd;
		return 0;
	}
	else
	return ErrSyntax;
}

WORD TTLExpandEnv()
{
	WORD Err;
	TVarId VarId;
	TStrVal deststr, srcptr;

	Err = 0;
	GetStrVar(&VarId, &Err);
	if (Err!=0) return Err;

	if (CheckParameterGiven()) { // expandenv strvar strval
		GetStrVal(srcptr,&Err);
		if ((Err==0) && (GetFirstChar()!=0))
			Err = ErrSyntax;
		if (Err!=0) {
			return Err;
		}

		// ファイルパスに環境変数が含まれているならば、展開する。
		ExpandEnvironmentStrings(srcptr, deststr, MaxStrLen);
		SetStrVal(VarId, deststr);
	}
	else { // expandenv strvar
		// ファイルパスに環境変数が含まれているならば、展開する。
		ExpandEnvironmentStrings(StrVarPtr(VarId), deststr, MaxStrLen);
		SetStrVal(VarId, deststr);
	}

	return Err;
}

WORD TTLFileClose()
{
	WORD Err;
	int fhi;	// handle index
	HANDLE FH;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	_lclose(FH);
	HandleFree(fhi);
	return Err;
}

WORD TTLFileConcat()
{
	WORD Err;
	HANDLE FH1, FH2;
	int c;
	TStrVal FName1, FName2;
	BYTE buf[1024];

	Err = 0;
	GetStrVal(FName1,&Err);
	GetStrVal(FName2,&Err);
	if ((Err==0) &&
	    ((strlen(FName1)==0) ||
	     (strlen(FName2)==0) ||
	     (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName1,sizeof(FName1))) {
		SetResult(-1);
		return Err;
	}
	if (!GetAbsPath(FName2,sizeof(FName2))) {
		SetResult(-2);
		return Err;
	}
	if (_stricmp(FName1,FName2)==0) {
		SetResult(2);
		return Err;
	}

	FH1 = _lopen(FName1,OF_WRITE);
	if (FH1 == INVALID_HANDLE_VALUE)
		FH1 = _lcreat(FName1,0);
	if (FH1 == INVALID_HANDLE_VALUE) {
		SetResult(3);
		return Err;
	}
	_llseek(FH1,0,2);

	FH2 = _lopen(FName2,OF_READ);
	if (FH2 != INVALID_HANDLE_VALUE)
	{
		do {
			c = _lread(FH2,&(buf[0]),sizeof(buf));
			if (c>0)
				_lwrite(FH1,&(buf[0]),c);
		} while (c >= sizeof(buf));
		_lclose(FH2);
	}
	_lclose(FH1);

	SetResult(0);
	return Err;
}

WORD TTLFileCopy()
{
	WORD Err;
	TStrVal FName1, FName2;

	Err = 0;
	GetStrVal(FName1,&Err);
	GetStrVal(FName2,&Err);
	if ((Err==0) &&
	    ((strlen(FName1)==0) ||
	     (strlen(FName2)==0) ||
	     (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName1,sizeof(FName1))) {
		SetResult(-1);
		return Err;
	}
	if (!GetAbsPath(FName2,sizeof(FName2))) {
		SetResult(-2);
		return Err;
	}
	if (_stricmp(FName1,FName2)==0) return Err;

	CopyFile(FName1,FName2,FALSE);
	SetResult(0);
	return Err;
}

WORD TTLFileCreate()
{
	WORD Err;
	TVarId VarId;
	HANDLE FH;
	int fhi;
	TStrVal FName;

	Err = 0;
	GetIntVar(&VarId, &Err);
	GetStrVal(FName, &Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetIntVal(VarId, -1);
		SetResult(-1);
		return Err;
	}
	FH = _lcreat(FName,0);
	if (FH == INVALID_HANDLE_VALUE) {
		SetResult(2);
	}
	else {
		SetResult(0);
	}
	fhi = HandlePut(FH);
	SetIntVal(VarId, fhi);
	if (fhi == -1) {
		_lclose(FH);
	}
	return Err;
}

WORD TTLFileDelete()
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetResult(-1);
		return Err;
	}
	
	if (remove(FName) != 0) {
		SetResult(-1);
	}
	else {
		SetResult(0);
	}

	return Err;
}

WORD TTLFileMarkPtr()
{
	WORD Err;
	int fhi;
	HANDLE FH;
	LONG pos;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	pos = _llseek(FH,0,1);	 /* mark current pos */
	if (pos == INVALID_SET_FILE_POINTER) {
		pos = 0;	// ?
	}
	FPointer[fhi] = pos;
	return Err;
}

// add 'filenamebox' (2007.9.13 maya)
WORD TTLFilenameBox()
{
	TStrVal Str1;
	WORD Err, ValType;
	TVarId VarId;
	OPENFILENAME ofn;
	char uimsg[MAX_UIMSG];
	BOOL SaveFlag = FALSE;
	TStrVal InitDir = "";
	BOOL ret;

	Err = 0;
	GetStrVal(Str1,&Err);
	if (Err!=0) return Err;

	// get 2nd arg(optional) if given
	if (CheckParameterGiven()) { // dialogtype
		GetIntVal(&SaveFlag, &Err);
		if (Err!=0) return Err;

		// get 3rd arg(optional) if given
		if (CheckParameterGiven()) { // initdir
			GetStrVal(InitDir, &Err);
			if (Err!=0) return Err;
		}
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetInputStr("");
	if (CheckVar("inputstr", &ValType, &VarId) &&
	    (ValType==TypString)) {
		memset(&ofn, 0, sizeof(OPENFILENAME));
		ofn.lStructSize     = get_OPENFILENAME_SIZE();
		ofn.hwndOwner       = HMainWin;
		ofn.lpstrTitle      = Str1;
		ofn.lpstrFile       = StrVarPtr(VarId);
		ofn.nMaxFile        = MaxStrLen;
		get_lang_msg("FILEDLG_ALL_FILTER", uimsg, sizeof(uimsg), "All(*.*)\\0*.*\\0\\0", UILanguageFile);
		ofn.lpstrFilter     = uimsg;
		ofn.lpstrInitialDir = NULL;
		if (strlen(InitDir) > 0) {
			ofn.lpstrInitialDir = InitDir;
		}
		BringupWindow(HMainWin);
		if (SaveFlag) {
			ofn.Flags = OFN_OVERWRITEPROMPT;
			ret = GetSaveFileName(&ofn);
		}
		else {
			ret = GetOpenFileName(&ofn);
		}
		SetResult(ret);
	}
	return Err;
}

WORD TTLFileOpen()
{
	WORD Err;
	TVarId VarId;
	int fhi;
	HANDLE FH;
	int Append, ReadonlyFlag=0;
	TStrVal FName;

	Err = 0;
	GetIntVar(&VarId, &Err);
	GetStrVal(FName, &Err);
	GetIntVal(&Append, &Err);
	// get 4nd arg(optional) if given
	if (CheckParameterGiven()) { // readonly
		GetIntVal(&ReadonlyFlag, &Err);
	}
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetIntVal(VarId, -1);
		return Err;
	}

	if (ReadonlyFlag) {
		FH = _lopen(FName,OF_READ);
	}
	else {
		FH = _lopen(FName,OF_READWRITE);
		if (FH == INVALID_HANDLE_VALUE)
			FH = _lcreat(FName,0);
	}
	if (FH == INVALID_HANDLE_VALUE) {
		SetIntVal(VarId, -1);
		return ErrCantOpen;
	}
	fhi = HandlePut(FH);
	if (fhi == -1) {
		SetIntVal(VarId, -1);
		_lclose(FH);
		return ErrCantOpen;
	}
	SetIntVal(VarId, fhi);
	if (Append!=0) {
		_llseek(FH, 0, 2/*FILE_END*/);
	}
	return 0;	// no error
}

// Format: filelock <file handle> [<timeout>]
// (2012.4.19 yutaka)
WORD TTLFileLock()
{
	WORD Err;
	int FH;
	DWORD timeout;
	int result;
	BOOL ret;
	DWORD dwStart;

	Err = 0;
	GetIntVal(&FH,&Err);
	if (Err!=0) return Err;

	timeout = -1;  // 無限大
	if (CheckParameterGiven()) {
		GetIntVal(&timeout, &Err);
		if (Err!=0) return Err;
	}

	result = 1;  // error
	dwStart = GetTickCount();
	do {
		ret = LockFile((HANDLE)FH, 0, 0, (DWORD)-1, (DWORD)-1);
		if (ret != 0) { // ロック成功
			result = 0;  // success
			break;
		}
		Sleep(1);
	} while (GetTickCount() - dwStart < (timeout*1000));

	SetResult(result);
	return Err;
}

// Format: fileunlock <file handle> 
// (2012.4.19 yutaka)
WORD TTLFileUnLock()
{
	WORD Err;
	int FH;
	BOOL ret;

	Err = 0;
	GetIntVal(&FH,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	ret = UnlockFile((HANDLE)FH, 0, 0, (DWORD)-1, (DWORD)-1);
	if (ret != 0) { // アンロック成功
		SetResult(0);
	} else {
		SetResult(1);
	}

	return Err;
}

WORD TTLFileReadln()
{
	WORD Err;
	TVarId VarId;
	int fhi;
	HANDLE FH;
	int i, c;
	TStrVal Str;
	BOOL EndFile, EndLine;
	BYTE b;

	Err = 0;
	GetIntVal(&fhi, &Err);
	FH = HandleGet(fhi);
	GetStrVar(&VarId, &Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	i = 0;
	EndLine = FALSE;
	EndFile = TRUE;
	do {
		c = _lread(FH, &b, 1);
		if (c>0) EndFile = FALSE;
		if (c==1) {
			switch (b) {
				case 0x0d:
					c = _lread(FH, &b, 1);
					if ((c==1) && (b!=0x0a))
						_llseek(FH, -1, 1);
					EndLine = TRUE;
					break;
				case 0x0a: EndLine = TRUE; break;
				default:
					if (i<MaxStrLen-1)
					{
						Str[i] = b;
						i++;
					}
			}
		}
	} while ((c>=1) && (! EndLine));

	if (EndFile)
		SetResult(1);
	else
		SetResult(0);

	Str[i] = 0;
	SetStrVal(VarId, Str);
	return Err;
}


// Format: fileread <file handle> <read byte> <strvar>
// 指定したバイト数だけファイルから読み込む。
// ただし、<read byte>は 1～255 まで。
// (2006.11.1 yutaka)
WORD TTLFileRead()
{
	WORD Err;
	TVarId VarId;
	int fhi;
	HANDLE FH;
	int i, c;
	int ReadByte;   // 読み込むバイト数
	TStrVal Str;
	BOOL EndFile, EndLine;
	BYTE b;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	GetIntVal(&ReadByte,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (ReadByte < 1 || ReadByte > MaxStrLen-1))  // 範囲チェック
		Err = ErrSyntax;
	if (Err!=0) return Err;

	EndLine = FALSE;
	EndFile = FALSE;
	for (i = 0 ; i < ReadByte ; i++) {
		c = _lread(FH,&b,1);
		if (c <= 0) {  // EOF
			EndFile = TRUE;
			break;
		}
		if (i<MaxStrLen-1)
		{
			Str[i] = b;
		}
	} 

	if (EndFile)
		SetResult(1);
	else
		SetResult(0);

	Str[i] = 0;
	SetStrVal(VarId,Str);
	return Err;
}


WORD TTLFileRename()
{
	WORD Err;
	TStrVal FName1, FName2;

	Err = 0;
	GetStrVal(FName1,&Err);
	GetStrVal(FName2,&Err);
	if ((Err==0) &&
	    ((strlen(FName1)==0) ||
	     (strlen(FName2)==0) ||
	     (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}
	if (_stricmp(FName1,FName2)==0) {
		SetResult(2);
		return Err;
	}

	if (!GetAbsPath(FName1,sizeof(FName1))) {
		SetResult(-1);
		return Err;
	}
	if (!GetAbsPath(FName2,sizeof(FName2))) {
		SetResult(-2);
		return Err;
	}
	if (rename(FName1,FName2) != 0) {
		// リネームに失敗したら、エラーで返す。
		SetResult(-3);
		return Err;
	}

	SetResult(0);
	return Err;
}

WORD TTLFileSearch()
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(FName,sizeof(FName));
	if (DoesFileExist(FName))
		SetResult(1);
	else
		SetResult(0);
	return Err;
}

WORD TTLFileSeek()
{
	WORD Err;
	int fhi;
	HANDLE FH;
	int i, j;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	GetIntVal(&i,&Err);
	GetIntVal(&j,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	_llseek(FH,i,j);
	return Err;
}

WORD TTLFileSeekBack()
{
	WORD Err;
	int fhi;
	HANDLE FH;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	/* move back to the marked pos */
	_llseek(FH,FPointer[fhi],0);
	return Err;
}

WORD TTLFileStat()
{
	WORD Err;
	TVarId SizeVarId, TimeVarId, DrvVarId;
	TStrVal FName, TimeStr, DrvStr;
	struct _stat st;
	int ret;
	int result = -1;
	struct tm *tmp;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    (strlen(FName)==0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!GetAbsPath(FName,sizeof(FName))) {
		goto end;
	}

	ret = _stat(FName, &st);
	if (ret != 0) {
		goto end;
	}

	if (CheckParameterGiven()) { 
		GetIntVar(&SizeVarId,&Err);
		if (Err!=0) return Err;
		SetIntVal(SizeVarId, st.st_size);
	}

	if (CheckParameterGiven()) { 
		GetStrVar(&TimeVarId,&Err);
		if (Err!=0) return Err;
		tmp = localtime(&st.st_mtime);
		strftime(TimeStr, sizeof(TimeStr), "%Y-%m-%d %H:%M:%S", tmp);
		SetStrVal(TimeVarId, TimeStr);
	}

	if (CheckParameterGiven()) { 
		GetStrVar(&DrvVarId,&Err);
		if (Err!=0) return Err;
		_snprintf_s(DrvStr, sizeof(DrvStr), _TRUNCATE, "%c", st.st_dev + 'A');
		SetStrVal(DrvVarId, DrvStr);
	}

	result = 0;

end:
	SetResult(result);

	return Err;
}

WORD TTLFileStrSeek()
{
	WORD Err;
	int fhi;
	HANDLE FH;
	int Len, i, c;
	TStrVal Str;
	BYTE b;
	long int pos;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	GetStrVal(Str,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	pos = _llseek(FH,0,1);
	if (pos == INVALID_SET_FILE_POINTER) return Err;

	Len = strlen(Str);
	i = 0;
	do {
		c = _lread(FH,&b,1);
		if (c==1)
		{
			if (b==(BYTE)Str[i])
				i++;
			else if (i>0) {
				i = 0;
				if (b==(BYTE)Str[0])
					i = 1;
			}
		}
	} while ((c>=1) && (i!=Len));
	if (i==Len)
		SetResult(1);
	else {
		SetResult(0);
		_llseek(FH,pos,0);
	}
	return Err;
}

WORD TTLFileStrSeek2()
{
	WORD Err;
	int fhi;
	HANDLE FH;
	int Len, i, c;
	TStrVal Str;
	BYTE b;
	long int pos, pos2;
	BOOL Last;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	GetStrVal(Str,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	pos = _llseek(FH,0,1);
	if (pos == INVALID_SET_FILE_POINTER) return Err;

	Len = strlen(Str);
	i = 0;
	pos2 = pos;
	do {
		Last = (pos2<=0);
		c = _lread(FH,&b,1);
		pos2 = _llseek(FH,-2,1);
		if (c==1)
		{
			if (b==(BYTE)Str[Len-1-i])
				i++;
			else if (i>0) {
				i = 0;
				if (b==(BYTE)Str[Len-1])
					i = 1;
			}
		}
	} while (!Last && (i!=Len));
	if (i==Len) {
		// ファイルの1バイト目がヒットすると、ファイルポインタが突き破って
		// INVALID_SET_FILE_POINTER になるので、
		// ゼロオフセットになるように調整する。(2008.10.10 yutaka)
		if (pos == INVALID_SET_FILE_POINTER) 
			_llseek(FH, 0, 0);
		SetResult(1);
	} else {
		SetResult(0);
		_llseek(FH,pos,0);
	}
	return Err;
}

WORD TTLFileTruncate()
{
	WORD Err;
	TStrVal FName;
	int result = -1;
	int TruncByte;
	int fh = -1;
	int ret;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    (strlen(FName)==0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!GetAbsPath(FName,sizeof(FName))) {
		goto end;
	}

	if (CheckParameterGiven()) { 
		GetIntVal(&TruncByte,&Err);
		if (Err!=0) return Err;
	} else {
		Err = ErrSyntax;
		goto end;
	}

	// ファイルを指定したサイズで切り詰める。
   ret = _sopen_s( &fh, FName, _O_RDWR | _O_CREAT, _SH_DENYNO, _S_IREAD | _S_IWRITE );
   if (ret != 0) {
		Err = ErrCantOpen;
		goto end;
   }
   ret = _chsize_s(fh, TruncByte);
   if (ret != 0) {
		Err = ErrInvalidCtl;
		goto end;
   }

	result = 0;
	Err = 0;

end:
	SetResult(result);

	if (fh != -1)
		_close(fh);

	return Err;
}

WORD TTLFileWrite(BOOL addCRLF)
{
	WORD Err, P;
	int fhi;
	HANDLE FH;
	int Val;
	TStrVal Str;

	Err = 0;
	GetIntVal(&fhi, &Err);
	FH = HandleGet(fhi);
	if (Err) return Err;

	P = LinePtr;
	GetStrVal(Str, &Err);
	if (!Err) {
		if (GetFirstChar())
			return ErrSyntax;

		_lwrite(FH, Str, strlen(Str));
	}
	else if (Err == ErrTypeMismatch) {
		Err = 0;
		LinePtr = P;
		GetIntVal(&Val, &Err);
		if (Err) return Err;
		if (GetFirstChar())
			return ErrSyntax;

		Str[0] = Val & 0xff;
		_lwrite(FH, Str, 1);
	}
	else {
		return Err;
	}

	if (addCRLF) {
		_lwrite(FH,"\015\012",2);
	}
	return 0;
}

WORD TTLFindClose()
{
	WORD Err;
	int DH;

	Err = 0;
	GetIntVal(&DH,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if ((DH>=0) && (DH<NumDirHandle) &&
	    (DirHandle[DH]!=-1L))
	{
		_findclose(DirHandle[DH]);
		DirHandle[DH] = -1L;
	}
	return Err;
}

WORD TTLFindFirst()
{
	TVarId DH, Name;
	WORD Err;
	TStrVal Dir;
	int i;
	struct _finddata_t data;

	Err = 0;
	GetIntVar(&DH,&Err);
	GetStrVal(Dir,&Err);
	GetStrVar(&Name,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (Dir[0]==0) strncpy_s(Dir, sizeof(Dir),"*.*", _TRUNCATE);
	GetAbsPath(Dir,sizeof(Dir));
	i = 0;
	while ((i<NumDirHandle) &&
	       (DirHandle[i]!=-1L))
		i++;
	if (i<NumDirHandle)
	{
		DirHandle[i] = _findfirst(Dir,&data);
		if (DirHandle[i]!=-1L)
			SetStrVal(Name,data.name);
		else
			i = -1;
	}
	else
		i = -1;

	SetIntVal(DH,i);
	if (i<0)
	{
		SetResult(0);
		SetStrVal(Name,"");
	}
	else
		SetResult(1);
	return Err;
}

WORD TTLFindNext()
{
	TVarId Name;
	WORD Err;
	int DH;
	struct _finddata_t data;

	Err = 0;
	GetIntVal(&DH,&Err);
	GetStrVar(&Name,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if ((DH>=0) && (DH<NumDirHandle) &&
	    (DirHandle[DH]!=-1L) &&
	    (_findnext(DirHandle[DH],&data)==0))
	{
		SetStrVal(Name,data.name);
		SetResult(1);
	}
	else {
		SetStrVal(Name,"");
		SetResult(0);
	}
	return Err;
}

WORD TTLFlushRecv()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	FlushRecv();
	return 0;
}

WORD TTLFolderCreate()
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetResult(-1);
		return Err;
	}

	if (CreateDirectory(FName, NULL) == 0) {
		SetResult(2);
	}
	else {
		SetResult(0);
	}
	return Err;
}

WORD TTLFolderDelete()
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetResult(-1);
		return Err;
	}

	if (RemoveDirectory(FName) == 0) {
		SetResult(2);
	}
	else {
		SetResult(0);
	}
	return Err;
}

WORD TTLFolderSearch()
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(FName,sizeof(FName));
	if (DoesFolderExist(FName)) {
		SetResult(1);
	}
	else {
		SetResult(0);
	}
	return Err;
}

WORD TTLFor()
{
	WORD Err;
	TVarId VarId;
	int ValStart, ValEnd, i;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetIntVal(&ValStart,&Err);
	GetIntVal(&ValEnd,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (! CheckNext())
	{ // first time
		Err = SetForLoop();
		if (Err==0)
		{
			SetIntVal(VarId,ValStart);
			i = CopyIntVal(VarId);
			if (i==ValEnd)
				LastForLoop();
		}
	}
	else { // return from 'next'
		i = CopyIntVal(VarId);
		if (i<ValEnd)
			i++;
		else if (i>ValEnd)
			i--;
		SetIntVal(VarId,i);
		if (i==ValEnd)
			LastForLoop();
	}
	return Err;
}

WORD TTLGetDir()
{
	WORD Err;
	TVarId VarId;
	TStrVal Str;

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	TTMGetDir(Str, sizeof(Str));
	SetStrVal(VarId,Str);
	return Err;
}

WORD TTLGetEnv()
{
	WORD Err;
	TVarId VarId;
	TStrVal Str;
	PCHAR Str2;

	Err = 0;
	GetStrVal(Str,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Str2 = getenv(Str);

	if (Str2!=NULL)
		SetStrVal(VarId,Str2);
	else
		SetStrVal(VarId,"");
	return Err;
}

WORD TTLGetFileAttr()
{
	WORD Err;
	TStrVal Filename;

	Err = 0;
	GetStrVal(Filename,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(Filename, sizeof(Filename));
	SetResult(GetFileAttributes(Filename));

	return Err;
}

WORD TTLGetHostname()
{
	WORD Err;
	TVarId VarId;
	char Str[MaxStrLen];

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	Err = GetTTParam(CmdGetHostname,Str,sizeof(Str));
	if (Err==0)
		SetStrVal(VarId,Str);
	return Err;
}

#define MAX_IPADDR 30
/*
 strdim ipaddr 10
 getipv4addr ipaddr num
 */
WORD TTLGetIPv4Addr()
{
	WORD Err;
	TVarId VarId, VarId2, id;
	WSADATA ws;
	INTERFACE_INFO info[MAX_IPADDR];
	SOCKET sock;
	DWORD socknum;
	int num, result, arysize;
	int i, n;
	IN_ADDR addr;

	Err = 0;
	GetStrAryVar(&VarId,&Err);
	GetIntVar(&VarId2, &Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// 自分自身の全IPv4アドレスを取得する。
	if (WSAStartup(MAKEWORD(2,2), &ws) != 0) {
		SetResult(-1);
		SetIntVal(VarId2, 0);
		return Err;
	}
	arysize = GetStrAryVarSize(VarId);
	num = 0;
	result = 1;
	sock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
	if (WSAIoctl(sock, SIO_GET_INTERFACE_LIST, NULL, 0, info, sizeof(info), &socknum, NULL, NULL) != SOCKET_ERROR) {
		n = socknum / sizeof(info[0]);
		for (i = 0 ; i < n ; i++) {
			if ((info[i].iiFlags & IFF_UP) == 0) 
				continue;
			if ((info[i].iiFlags & IFF_LOOPBACK) != 0)
				continue;
			addr = info[i].iiAddress.AddressIn.sin_addr;

			if (num < arysize) {
				id = GetStrVarFromArray(VarId, num, &Err);
				SetStrVal(id, inet_ntoa(addr));
			}
			else {
				result = 0;
			}
			num++;
		}
	}
	closesocket(sock);
	WSACleanup();

	SetResult(result);
	SetIntVal(VarId2, num);

	return Err;
}


// IPv6アドレスを文字列に変換する。
static void myInetNtop(int Family, char *pAddr, char *pStringBuf, size_t StringBufSize)
{
	int i;
	char s[16];
	unsigned int val;

	pStringBuf[0] = '\0';
	for (i = 0 ; i < 16 ; i++) {
		val = (pAddr[i]) & 0xFF;
		_snprintf_s(s, sizeof(s), _TRUNCATE, "%02x", val);
		strncat_s(pStringBuf, StringBufSize, s, _TRUNCATE);
		if (i != 15 && (i & 1))
			strncat_s(pStringBuf, StringBufSize, ":", _TRUNCATE);
	}
}


WORD TTLGetIPv6Addr()
{
	WORD Err;
	TVarId VarId, VarId2, id;
	int num, result, arysize;
	DWORD ret;
	IP_ADAPTER_ADDRESSES addr[256];/* XXX */
	ULONG len = sizeof(addr);
	char ipv6str[64];

	Err = 0;
	GetStrAryVar(&VarId,&Err);
	GetIntVar(&VarId2, &Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// GetAdaptersAddresses がサポートされていない OS はここで return
	if (!HasGetAdaptersAddresses()) {
		SetResult(-1);
		SetIntVal(VarId2, 0);
		return Err;
	}

	// 自分自身の全IPv6アドレスを取得する。
	arysize = GetStrAryVarSize(VarId);
	num = 0;
	result = 1;
	ret = GetAdaptersAddresses(AF_INET6, 0, NULL, addr, &len);
	if (ret == ERROR_SUCCESS) {
		IP_ADAPTER_ADDRESSES *padap = &addr[0];

		do {
			IP_ADAPTER_UNICAST_ADDRESS *uni = padap->FirstUnicastAddress;

			if (!uni) {
				continue;
			}
			do {
				SOCKET_ADDRESS addr = uni->Address;
				struct sockaddr_in6 *sa;

				if (!(uni->Flags & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE)) {
					continue;
				}
				sa = (struct sockaddr_in6*)addr.lpSockaddr;
				myInetNtop(AF_INET6, (char*)&sa->sin6_addr, ipv6str, sizeof(ipv6str));

				if (num < arysize) {
					id = GetStrVarFromArray(VarId, num, &Err);
					SetStrVal(id, ipv6str);
				}
				else {
					result = 0;
				}
				num++;

			} while ((uni = uni->Next));
		} while ((padap = padap->Next));
	}

	SetResult(result);
	SetIntVal(VarId2, num);

	return Err;
}


WORD TTLGetPassword()
{
	TStrVal Str, Str2, Temp2;
	char Temp[512];
	WORD Err;
	TVarId VarId;
	int result = 0;  /* failure */

	Err = 0;
	GetStrVal(Str,&Err);
	GetStrVal(Str2,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	SetStrVal(VarId,"");
	if (Str[0]==0) return Err;
	if (Str2[0]==0) return Err;

	GetAbsPath(Str,sizeof(Str));

	GetPrivateProfileString("Password",Str2,"",
	                        Temp,sizeof(Temp), Str);
	if (Temp[0]==0) // password not exist
	{
		OpenInpDlg(Temp2, Str2, "Enter password", "", TRUE);
		if (Temp2[0]!=0) {
			Encrypt(Temp2,Temp);
			if (WritePrivateProfileString("Password",Str2,Temp, Str) != 0) {
				result = 1;  /* success */
			}
		}
	}
	else {// password exist
		Decrypt(Temp,Temp2);
		result = 1;  /* success */
	}

	SetStrVal(VarId,Temp2);

	SetResult(result);  // 成功可否を設定する。
	return Err;
}

// setpassword 'password.dat' 'mypassword' passowrd
WORD TTLSetPassword()
{
	TStrVal FileNameStr, KeyStr;
	char Temp[512];
	WORD Err;
	TVarId VarId;
	PCHAR VarStr;
	int result = 0;  /* failure */

	Err = 0;
	GetStrVal(FileNameStr, &Err);   // ファイル名
	GetStrVal(KeyStr, &Err);  // キー名
	GetStrVar(&VarId, &Err);
	VarStr = StrVarPtr(VarId);  // 変数へのポインタ
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// 文字列が空の場合はエラーとする。
	if (FileNameStr[0]==0 || 
	    KeyStr[0]==0 ||
	    VarStr[0]==0)   // "getpassword"同様、空パスワードも許可しない。
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(FileNameStr, sizeof(FileNameStr));

	// パスワードを暗号化する。
	Encrypt(VarStr, Temp);

	if (WritePrivateProfileString("Password", KeyStr, Temp, FileNameStr) != 0) 
		result = 1;  /* success */

	SetResult(result);  // 成功可否を設定する。
	return Err;
}

// ispassword 'password.dat' 'username' ; result: 0=false; 1=true
WORD TTLIsPassword()
{
	TStrVal FileNameStr, KeyStr;
	char Temp[512];
	WORD Err;
	int result = 0; 

	Err = 0;
	GetStrVal(FileNameStr, &Err);   // ファイル名
	GetStrVal(KeyStr, &Err);  // キー名
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// 文字列が空の場合はエラーとする。
	if (FileNameStr[0]==0 || 
	    KeyStr[0]==0)
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(FileNameStr, sizeof(FileNameStr));

	Temp[0] = 0;
	GetPrivateProfileString("Password", KeyStr,"",
	                        Temp, sizeof(Temp), FileNameStr);
	if (Temp[0] == 0) { // password not exist
		result = 0; 
	} else {
		result = 1; 
	}

	SetResult(result);  // 成功可否を設定する。
	return Err;
}

WORD TTLGetSpecialFolder()
{
	WORD Err;
	TVarId VarId;
	TStrVal type;
	int result;

	Err = 0;
	GetStrVar(&VarId,&Err);
	if (Err!=0) return Err;

	GetStrVal(type,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) {
		return Err;
	}

	result = GetSpecialFolder(StrVarPtr(VarId), sizeof(TStrVal), type);
	SetResult(result);

	return Err;
}

WORD TTLGetTime(WORD mode)
{
	WORD Err;
	TVarId VarId;
	TStrVal Str1, Str2, tzStr;
	time_t time1;
	struct tm *ptm;
	char *format;
	BOOL set_result;
	const char *tz = NULL;
	char tz_copy[128]; 

	// Save timezone
	tz = getenv("TZ");
	tz_copy[0] = 0;
	if (tz)
		strncpy_s(tz_copy, sizeof(tz_copy), tz, _TRUNCATE);

	Err = 0;
	GetStrVar(&VarId,&Err);

	if (CheckParameterGiven()) {
		GetStrVal(Str1, &Err);
		format = Str1;

		if (isInvalidStrftimeChar(Str1)) {
			SetResult(2);
			return 0;
//			return ErrSyntax;
		}
		set_result = TRUE;

		// タイムゾーンの指定があれば、localtime()に影響させる。(2012.5.2 yutaka)
		if (CheckParameterGiven()) {
			GetStrVal(tzStr, &Err);
			if (Err!=0) return Err;
			_putenv_s("TZ", tzStr);
			_tzset();
		}

	}
	else {
		switch (mode) {
		case RsvGetDate:
			format = "%Y-%m-%d";
			break;
		case RsvGetTime:
			format = "%H:%M:%S";
			break;
		}
		set_result = FALSE;
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// get current date & time
	time1 = time(NULL);
	ptm = localtime(&time1);

	if (strftime(Str2, sizeof(Str2), format, ptm)) {
		SetStrVal(VarId,Str2);
		if (set_result) SetResult(0);
	}
	else {
		if (set_result) SetResult(1);
	}

	// Restore timezone
	_putenv_s("TZ", tz_copy);
	_tzset();

	return Err;
}

WORD TTLGetTitle()
{
	TVarId VarId;
	WORD Err;
	char Str[TitleBuffSize*2];

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	Err = GetTTParam(CmdGetTitle,Str,sizeof(Str));
	if (Err==0)
		SetStrVal(VarId,Str);
	return Err;
}

WORD TTLGetTTDir()
{
	TVarId VarId;
	WORD Err;
	char Temp[MAX_PATH],HomeDir[MAX_PATH];

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (GetModuleFileName(NULL, Temp, sizeof(Temp)) == 0) {
		SetStrVal(VarId,"");
		SetResult(0);
		return Err;
	}
	ExtractDirName(Temp, HomeDir);
	SetStrVal(VarId,HomeDir);
	SetResult(1);

	return Err;
}

// COMポートからレジスタ値を読む。
// (2015.1.8 yutaka)
WORD TTLGetModemStatus()
{
	TVarId VarId;
	WORD Err;
	char Str[MaxStrLen];
	int Num;

	Err = 0;
	GetIntVar(&VarId, &Err);
	if ((Err == 0) && (GetFirstChar() != 0))
		Err = ErrSyntax;
	if ((Err == 0) && (!Linked))
		Err = ErrLinkFirst;
	if (Err != 0) return Err;

	memset(Str, 0, sizeof(Str));
	Err = GetTTParam(CmdGetModemStatus, Str, sizeof(Str));
	if (Err == 0) {
		Num = atoi(Str);
		SetIntVal(VarId, Num);
		SetResult(0);
	}
	else {
		SetResult(1);
	}

	return Err;
}

//
// Tera Term のバージョン取得 & 比較
// バージョン番号はコンパイル時に決定する。
// (現在は実行ファイルのバージョン情報は参照しない)
//
WORD TTLGetVer()
{
	TVarId VarId;
	WORD Err;
	TStrVal Str1, Str2;
	int cur_major = TT_VERSION_MAJOR;
	int cur_minor = TT_VERSION_MINOR;
	int compare = 0;
	int comp_major, comp_minor, ret;
	int cur_ver, comp_ver;

	Err = 0;
	GetStrVar(&VarId, &Err);

	if (CheckParameterGiven()) {
		GetStrVal(Str1, &Err);

		ret = sscanf_s(Str1, "%d.%d", &comp_major, &comp_minor);
		if (ret != 2) {
			SetResult(-2);
			return 0;
			//Err = ErrSyntax;
		}
		compare = 1;

	} else {
		compare = 0;

	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	_snprintf_s(Str2, sizeof(Str2), _TRUNCATE, "%d.%d", cur_major, cur_minor);
	SetStrVal(VarId, Str2);

	if (compare == 1) {
		cur_ver = cur_major * 10000 + cur_minor;
		comp_ver = comp_major * 10000 + comp_minor;

		if (cur_ver < comp_ver) {
			SetResult(-1);
		} else if (cur_ver == comp_ver) {
			SetResult(0);
		} else {
			SetResult(1);
		}
	}

	return Err;
}

WORD TTLGoto()
{
	TName LabName;
	WORD Err, VarType;
	TVarId VarId;

	if (GetLabelName(LabName) && (GetFirstChar()==0))
	{
		if (CheckVar(LabName,&VarType,&VarId) && (VarType==TypLabel))
		{
			JumpToLabel(VarId);
			Err = 0;
		}
		else
			Err = ErrLabelReq;
	}
	else
		Err = ErrSyntax;

	return Err;
}

// add 'ifdefined' (2006.9.23 maya)
WORD TTLIfDefined()
{
	WORD VarType, Err;
	int Val;

	GetVarType(&VarType,&Val,&Err);

	SetResult(VarType);

	return Err;
}

BOOL CheckThen(LPWORD Err)
{
	BYTE b;
	TName Temp;

	do {

		do {
			b = GetFirstChar();
			if (b==0) return FALSE;
		} while (((b<'A') || (b>'Z')) &&
		         (b!='_') &&
		         ((b<'a') || (b>'z')));
		LinePtr--;
		if (! GetIdentifier(Temp)) return FALSE;

	} while (_stricmp(Temp,"then")!=0);

	if (GetFirstChar()!=0)
		*Err = ErrSyntax;
	return TRUE;
}

WORD TTLIf()
{
	WORD Err, ValType, Tmp, WId;
	int Val;

	if (! GetExpression(&ValType,&Val,&Err))
		return ErrSyntax;

	if (Err!=0) return Err;

	if (ValType!=TypInteger)
		return ErrTypeMismatch;

	Tmp = LinePtr;
	if (GetReservedWord(&WId) &&
	    (WId==RsvThen))
	{  // If then  ... EndIf structure
		if (GetFirstChar()!=0)
			return ErrSyntax;
		IfNest++;
		if (Val==0)
			ElseFlag = 1; // Skip until 'Else' or 'EndIf'
	}
	else { // single line If command
		LinePtr = Tmp;
		if (!CheckParameterGiven())
			return ErrSyntax;
		if (Val==0)
			return 0;
		return ExecCmnd();
	}
	return Err;
}

WORD TTLInclude()
{
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	if (!GetAbsPath(Str,sizeof(Str))) {
		Err = ErrCantOpen;
		return Err;
	}
	if (! BuffInclude(Str)) {
		Err = ErrCantOpen;
		return Err;
	}

	return Err;
}

WORD TTLInputBox(BOOL Paswd)
{
	TStrVal Str1, Str2, Str3;
	WORD Err, ValType, P;
	TVarId VarId;
	int sp = 0;

	Err = 0;
	GetStrVal(Str1,&Err);
	GetStrVal(Str2,&Err);
	if (Err!=0) return Err;

	if (!Paswd && CheckParameterGiven()) {
		// get 3rd arg(optional)
		P = LinePtr;
		GetStrVal(Str3,&Err);
		if (Err == ErrTypeMismatch) {
			strncpy_s(Str3,sizeof(Str3),"",_TRUNCATE);
			LinePtr = P;
			Err = 0;
		}
	}
	else {
		strncpy_s(Str3,sizeof(Str3),"",_TRUNCATE);
	}

	// get 4th(3rd) arg(optional) if given
	if (CheckParameterGiven()) {
		GetIntVal(&sp, &Err);
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (sp) {
		RestoreNewLine(Str1);
	}

	SetInputStr("");
	if (CheckVar("inputstr",&ValType,&VarId) &&
	    (ValType==TypString))
		OpenInpDlg(StrVarPtr(VarId),Str1,Str2,Str3,Paswd);
	return Err;
}

WORD TTLInt2Str()
{
	TVarId VarId;
	WORD Err;
	int Num;
	TStrVal Str2;

	Err = 0;
	GetStrVar(&VarId,&Err);

	GetIntVal(&Num,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	_snprintf_s(Str2,sizeof(Str2),_TRUNCATE,"%d",Num);

	SetStrVal(VarId,Str2);
	return Err;
}

//
// logrotate size value
// logrotate rotate num
// logrotate halt
//
WORD TTLLogRotate() 
{
	WORD Err;
	char Str[MaxStrLen];
	char Str2[MaxStrLen];
	char buf[MaxStrLen*2];
	int size, num, len;

	Err = 0;
	GetStrVal(Str, &Err);
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	Err = ErrSyntax;
	if (strcmp(Str, "size") == 0) {   // ローテートサイズ
		if (CheckParameterGiven()) {
			Err = 0;
			size = 0;
			GetStrVal(Str2, &Err);
			if (Err == 0) {
				len = strlen(Str2);
				if (isdigit(Str2[len-1])) {
					num = 1;
				} else if (Str2[len-1] == 'K') {
					Str2[len-1] = 0;
					num = 1024;
				} else if (Str2[len-1] == 'M') {
					Str2[len-1] = 0;
					num = 1024*1024;
				} else {
					Err = ErrSyntax;
				}
				size = atoi(Str2) * num;
			}

			if (size < 128)
				Err = ErrSyntax;
			if (Err == 0)
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s %u", Str, size);
		}

	} else if (strcmp(Str, "rotate") == 0) {  // ローテートの世代数
		if (CheckParameterGiven()) {
			Err = 0;
			num = 0;
			GetIntVal(&num, &Err);
			if (num <= 0)
				Err = ErrSyntax;
			if (Err == 0)
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s %u", Str, num);
		}

	} else if (strcmp(Str, "halt") == 0) {
		Err = 0;
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s", Str);
	} 
	if (Err!=0) return Err;

	SetFile(buf);
	Err = SendCmnd(CmdLogRotate, 0);

	return Err;
}

WORD TTLLogInfo() 
{
	WORD Err;
	TVarId VarId;
	char Str[MaxStrLen];

	Err = 0;
	GetStrVar(&VarId, &Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	Err = GetTTParam(CmdLogInfo, Str, sizeof(Str));
	if (Err==0) {
		SetResult(Str[0] - '0');
		SetStrVal(VarId, Str+1);
	}
	return Err;
}

WORD TTLLogOpen()
{
	TStrVal Str;
	WORD Err;
	int LogFlags[LogOptMax+1] = { 0 };
	int TmpFlag;
	char ret[2];

	Err = 0;
	GetStrVal(Str, &Err);
	GetIntVal(&LogFlags[LogOptBinary], &Err);
	GetIntVal(&LogFlags[LogOptAppend], &Err);

	// plain text
	if (!CheckParameterGiven()) goto EndLogOptions;
	GetIntVal(&LogFlags[LogOptPlainText], &Err);
	if (Err!=0) return Err;

	// timestamp
	if (!CheckParameterGiven()) goto EndLogOptions;
	GetIntVal(&LogFlags[LogOptTimestamp], &Err);
	if (Err!=0) return Err;

	// hide file transfer dialog
	if (!CheckParameterGiven()) goto EndLogOptions;
	GetIntVal(&LogFlags[LogOptHideDialog], &Err);
	if (Err!=0) return Err;

	// Include screen buffer
	if (!CheckParameterGiven()) goto EndLogOptions;
	GetIntVal(&LogFlags[LogOptIncScrBuff], &Err);
	if (Err!=0) return Err;

	// Timestamp Type
	if (!CheckParameterGiven()) goto EndLogOptions;
	GetIntVal(&TmpFlag, &Err);
	if (Err!=0) return Err;
	if (TmpFlag < 0 || TmpFlag > 3)
		return ErrSyntax;
	LogFlags[LogOptTimestampType] = TmpFlag;

EndLogOptions:
	if (strlen(Str) == 0 || GetFirstChar() != 0)
		return ErrSyntax;

	SetFile(Str);
	SetLogOption(LogFlags);

	memset(ret, 0, sizeof(ret));
	Err = GetTTParam(CmdLogOpen,ret,sizeof(ret));
	if (Err==0) {
		if (ret[0] == 0x31) {
			SetResult(0);
		}
		else {
			SetResult(1);
		}
	}
	return Err;
}

WORD TTLLoop()
{
	WORD WId, Err;
	int Val = 1;

	Err = 0;
	if (CheckParameterGiven()) {
		if (GetReservedWord(&WId)) {
			switch (WId) {
				case RsvWhile:
					GetIntVal(&Val,&Err);
					break;
				case RsvUntil:
					GetIntVal(&Val,&Err);
					Val = Val == 0;
					break;
				default:
					Err = ErrSyntax;
			}
			if ((Err==0) && (GetFirstChar()!=0))
				Err = ErrSyntax;
		}
		else {
			Err = ErrSyntax;
		}
	}

	if (Err!=0) return Err;

	return BackToWhile(Val!=0);
}

WORD TTLMakePath()
{
	TVarId VarId;
	WORD Err;
	TStrVal Dir, Name;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Dir,&Err);
	GetStrVal(Name,&Err);
	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	AppendSlash(Dir,sizeof(Dir));
	strncat_s(Dir,sizeof(Dir),Name,_TRUNCATE);
	SetStrVal(VarId,Dir);

	return Err;
}

static void basedirname(char *fullpath, char *dest_base, int base_len, char *dest_dir, int dir_len) {
	char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	char dirname[MAX_PATH];
	char basename[MAX_PATH];

	_splitpath_s(fullpath, drive, sizeof(drive), dir, sizeof(dir), fname, sizeof(fname), ext, sizeof(ext));
	strncpy_s(dirname, sizeof(dirname), drive, _TRUNCATE);
	strncat_s(dirname, sizeof(dirname), dir, _TRUNCATE);
	DeleteSlash(dirname); // 末尾の \ を取り除く
	if (strlen(fname) == 0 && strlen(ext) == 0) {
		_splitpath_s(dirname, drive, sizeof(drive), dir, sizeof(dir), fname, sizeof(fname), ext, sizeof(ext));
		strncpy_s(dirname, sizeof(dirname), drive, _TRUNCATE);
		strncat_s(dirname, sizeof(dirname), dir, _TRUNCATE);
		DeleteSlash(dirname); // 末尾の \ を取り除く
		strncpy_s(basename, sizeof(basename), fname, _TRUNCATE);
		strncat_s(basename, sizeof(basename), ext, _TRUNCATE);
	}
	else {
		strncpy_s(basename, sizeof(basename), fname, _TRUNCATE);
		strncat_s(basename, sizeof(basename), ext, _TRUNCATE);
	}

	if (dest_dir != NULL) {
		strncpy_s(dest_dir, dir_len, dirname, _TRUNCATE);
	}
	if (dest_base != NULL) {
		strncpy_s(dest_base, base_len, basename, _TRUNCATE);
	}
}

static void basename(char *fullpath, char *dest, int len) {
	basedirname(fullpath, dest, len, NULL, 0);
}

static void dirname(char *fullpath, char *dest, int len) {
	basedirname(fullpath, NULL, 0, dest, len);
}

WORD TTLBasename()
{
	TVarId VarId;
	WORD Err;
	TStrVal Src, Name;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Src,&Err);
	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	basename(Src, Name, sizeof(Name));
	SetStrVal(VarId, Name);

	return Err;
}

WORD TTLDirname()
{
	TVarId VarId;
	WORD Err;
	TStrVal Src, Dir;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Src,&Err);
	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	dirname(Src, Dir, sizeof(Dir));
	SetStrVal(VarId, Dir);

	return Err;
}

WORD TTLDirnameBox()
{
	TStrVal Title;
	WORD Err, ValType;
	TVarId VarId;
	char buf[MAX_PATH];
	TStrVal InitDir = "";
	BOOL ret;

	Err = 0;
	GetStrVal(Title, &Err);
	if (Err != 0) return Err;

	// get 2nd arg(optional) if given
	if (CheckParameterGiven()) { // initdir
		GetStrVal(InitDir, &Err);
		if (Err != 0) return Err;
	}

	if ((Err == 0) && (GetFirstChar() != 0))
		Err = ErrSyntax;
	if (Err != 0) return Err;

	SetInputStr("");
	if (CheckVar("inputstr", &ValType, &VarId) &&
	    (ValType == TypString)) {
		BringupWindow(HMainWin);
		if (doSelectFolder(HMainWin, buf, sizeof(buf), InitDir, Title)) {
			SetInputStr(buf);
			ret = 1;
		}
		else {
			ret = 0;
		}
		SetResult(ret);
	}
	return Err;
}

#define IdMsgBox 1
#define IdYesNoBox 2
#define IdStatusBox 3
#define IdListBox 4
#define LISTBOX_ITEM_NUM 10

int MessageCommand(int BoxId, LPWORD Err)
{
	TStrVal Str1, Str2;
	int sp = 0;
	int ret;
	char **s;
	int i, ary_size;
	int sel = 0;
	TVarId VarId, VarId2;

	*Err = 0;
	GetStrVal2(Str1, Err, TRUE);
	GetStrVal2(Str2, Err, TRUE);
	if (*Err!=0) return 0;

	if (BoxId != IdListBox) {
		// get 3rd arg(optional) if given
		if (CheckParameterGiven()) {
			GetIntVal(&sp, Err);
		}
		if ((*Err==0) && (GetFirstChar()!=0))
			*Err = ErrSyntax;
		if (*Err!=0) return 0;
	}

	if (sp) {
		RestoreNewLine(Str1);
	}

	if (BoxId==IdMsgBox) {
		ret = OpenMsgDlg(Str1,Str2,FALSE);
		// メッセージボックスをキャンセルすると、マクロの終了とする。
		// (2008.8.5 yutaka)
		if (ret == IDCANCEL) {
			TTLStatus = IdTTLEnd;
		}
	} else if (BoxId==IdYesNoBox) {
		ret = OpenMsgDlg(Str1,Str2,TRUE);
		// メッセージボックスをキャンセルすると、マクロの終了とする。
		// (2008.8.6 yutaka)
		if (ret == IDCLOSE) {
			TTLStatus = IdTTLEnd;
		}
		return (ret);
	}
	else if (BoxId==IdStatusBox) {
		OpenStatDlg(Str1,Str2);

	} else if (BoxId==IdListBox) {
		//  リストボックスの選択肢を取得する。
		GetStrAryVar(&VarId, Err);

		if (CheckParameterGiven()) {
			GetIntVal(&sel, Err);
		}
		if (*Err==0 && GetFirstChar()!=0)
			*Err = ErrSyntax;
		if (*Err!=0) return 0;

		ary_size = GetStrAryVarSize(VarId);
		if (sel < 0 || sel >= ary_size) {
			sel = 0;
		}

		s = (char **)calloc(ary_size + 1, sizeof(char *));
		if (s == NULL) {
			*Err = ErrFewMemory;
			return -1;
		}
		for (i = 0 ; i < ary_size ; i++) {
			VarId2 = GetStrVarFromArray(VarId, i, Err);
			if (*Err!=0) return -1;
			s[i] = _strdup(StrVarPtr(VarId2));
		}
		if (s[0] == NULL) {
			*Err = ErrSyntax;
			return -1;
		}

		// return 
		//   0以上: 選択項目
		//   -1: キャンセル
		//	 -2: close
		ret = OpenListDlg(Str1, Str2, s, sel);

		for (i = 0 ; i < ary_size ; i++) {
			free(s[i]);
		}
		free(s);

		// リストボックスの閉じるボタン(&確認ダイアログ)で、マクロの終了とする。
		if (ret == -2) {
			TTLStatus = IdTTLEnd;
		}
		return (ret);

	}
	return 0;
}

// リストボックス
// (2013.3.13 yutaka)
WORD TTLListBox()
{
	WORD Err;
	int ret;

	ret = MessageCommand(IdListBox, &Err);
	SetResult(ret);
	return Err;
}

WORD TTLMessageBox()
{
	WORD Err;

	MessageCommand(IdMsgBox, &Err);
	return Err;
}

WORD TTLNext()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	return NextLoop();
}

WORD TTLPause()
{
	int TimeOut;
	WORD Err;

	Err = 0;
	GetIntVal(&TimeOut,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (TimeOut>0)
	{
		TTLStatus = IdTTLPause;
		TimeLimit = (DWORD)(TimeOut*1000);
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
	}
	return Err;
}

// add 'mpause' command
// SYNOPSIS: mpause millisecoand
// DESCRIPTION: This command would sleep Tera Term macro program by specified millisecond time.
// (2006.2.10 yutaka)
WORD TTLMilliPause()
{
	int TimeOut;
	WORD Err;

	Err = 0;
	GetIntVal(&TimeOut,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (TimeOut > 0) {
		Sleep(TimeOut);
	}

	return Err;
}


// add 'random' command
// SYNOPSIS: random <intvar> <value>
// DESCRIPTION: 
// This command generates the random value from 0 to <value> and
// stores the value to <intvar>.
// (2006.2.11 yutaka)
WORD TTLRandom()
{
	static int init_done = 0;
	static sfmt_t sfmt;
	TVarId VarId;
	WORD Err;
	int MaxNum;
	unsigned int Num, RandMin;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetIntVal(&MaxNum,&Err);

	if ( ((Err==0) && (GetFirstChar()!=0)) || MaxNum <= 0)
		Err = ErrSyntax;
	if (Err!=0) return Err;

	MaxNum++;

	/* 2**32 % x == (2**32 - x) % x */
	RandMin = (unsigned int) -MaxNum % MaxNum;

	if (!init_done) {
		init_done = 1;
		sfmt_init_gen_rand(&sfmt, (unsigned int)time(NULL));
	}

	do {
		Num = sfmt_genrand_uint32(&sfmt);
	} while (Num < RandMin);

	SetIntVal(VarId, (int)(Num % MaxNum));

	return Err;
}

WORD TTLRecvLn()
{
	TStrVal Str;
	WORD ValType;
	TVarId VarId;
	int TimeOut;

	if (GetFirstChar()!=0)
		return ErrSyntax;
	if (! Linked)
		return ErrLinkFirst;

	ClearWait();

	Str[0] = 0x0a;
	Str[1] = 0;
	SetWait(1,Str);
	SetInputStr("");
	SetResult(1);
	TTLStatus = IdTTLWaitNL;
	TimeOut = 0;
	if (CheckVar("timeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut = CopyIntVal(VarId) * 1000;
	}
	if (CheckVar("mtimeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut += CopyIntVal(VarId);
	}

	if (TimeOut>0)
	{
		TimeLimit = (DWORD)TimeOut;
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
	}

	return 0;
}

WORD TTLRegexOption()
{
	TStrVal Str;
	WORD Err;
	int opt_none_flag = 0;
	OnigOptionType new_opt = ONIG_OPTION_NONE;
	OnigEncoding new_enc = ONIG_ENCODING_UNDEF;
	OnigSyntaxType *new_syntax = NULL;

	Err = 0;

	while (CheckParameterGiven()) {
		GetStrVal(Str, &Err);
		if (Err)
			return Err;

		// Encoding
		if (_stricmp(Str, "ENCODING_ASCII")==0 || _stricmp(Str, "ASCII")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ASCII;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_1")==0 || _stricmp(Str, "ISO_8859_1")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_1;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_2")==0 || _stricmp(Str, "ISO_8859_2")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_2;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_3")==0 || _stricmp(Str, "ISO_8859_3")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_3;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_4")==0 || _stricmp(Str, "ISO_8859_4")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_4;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_5")==0 || _stricmp(Str, "ISO_8859_5")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_5;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_6")==0 || _stricmp(Str, "ISO_8859_6")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_6;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_7")==0 || _stricmp(Str, "ISO_8859_7")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_7;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_8")==0 || _stricmp(Str, "ISO_8859_8")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_8;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_9")==0 || _stricmp(Str, "ISO_8859_9")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_9;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_10")==0 || _stricmp(Str, "ISO_8859_10")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_10;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_11")==0 || _stricmp(Str, "ISO_8859_11")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_11;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_13")==0 || _stricmp(Str, "ISO_8859_13")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_13;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_14")==0 || _stricmp(Str, "ISO_8859_14")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_14;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_15")==0 || _stricmp(Str, "ISO_8859_15")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_15;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_16")==0 || _stricmp(Str, "ISO_8859_16")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_16;
		}
		else if (_stricmp(Str, "ENCODING_UTF8")==0 || _stricmp(Str, "UTF8")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_UTF8;
		}
		else if (_stricmp(Str, "ENCODING_UTF16_BE")==0 || _stricmp(Str, "UTF16_BE")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_UTF16_BE;
		}
		else if (_stricmp(Str, "ENCODING_UTF16_LE")==0 || _stricmp(Str, "UTF16_LE")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_UTF16_LE;
		}
		else if (_stricmp(Str, "ENCODING_UTF32_BE")==0 || _stricmp(Str, "UTF32_BE")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_UTF32_BE;
		}
		else if (_stricmp(Str, "ENCODING_UTF32_LE")==0 || _stricmp(Str, "UTF32_LE")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_UTF32_LE;
		}
		else if (_stricmp(Str, "ENCODING_EUC_JP")==0 || _stricmp(Str, "EUC_JP")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_EUC_JP;
		}
		else if (_stricmp(Str, "ENCODING_EUC_TW")==0 || _stricmp(Str, "EUC_TW")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_EUC_TW;
		}
		else if (_stricmp(Str, "ENCODING_EUC_KR")==0 || _stricmp(Str, "EUC_KR")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_EUC_KR;
		}
		else if (_stricmp(Str, "ENCODING_EUC_CN")==0 || _stricmp(Str, "EUC_CN")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_EUC_CN;
		}
		else if (_stricmp(Str, "ENCODING_SJIS")==0 || _stricmp(Str, "SJIS")==0
		      || _stricmp(Str, "ENCODING_CP932")==0 || _stricmp(Str, "CP932")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_SJIS;
		}
		else if (_stricmp(Str, "ENCODING_KOI8_R")==0 || _stricmp(Str, "KOI8_R")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_KOI8_R;
		}
		else if (_stricmp(Str, "ENCODING_CP1251")==0 || _stricmp(Str, "CP1251")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_CP1251;
		}
		else if (_stricmp(Str, "ENCODING_BIG5")==0 || _stricmp(Str, "BIG5")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_BIG5;
		}
		else if (_stricmp(Str, "ENCODING_GB18030")==0 || _stricmp(Str, "GB18030")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_GB18030;
		}

		// Syntax
		else if (_stricmp(Str, "SYNTAX_DEFAULT")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_DEFAULT;
		}
		else if (_stricmp(Str, "SYNTAX_ASIS")==0 || _stricmp(Str, "ASIS")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_ASIS;
		}
		else if (_stricmp(Str, "SYNTAX_POSIX_BASIC")==0 || _stricmp(Str, "POSIX_BASIC")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_POSIX_BASIC;
		}
		else if (_stricmp(Str, "SYNTAX_POSIX_EXTENDED")==0 || _stricmp(Str, "POSIX_EXTENDED")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_POSIX_EXTENDED;
		}
		else if (_stricmp(Str, "SYNTAX_EMACS")==0 || _stricmp(Str, "EMACS")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_EMACS;
		}
		else if (_stricmp(Str, "SYNTAX_GREP")==0 || _stricmp(Str, "GREP")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_GREP;
		}
		else if (_stricmp(Str, "SYNTAX_GNU_REGEX")==0 || _stricmp(Str, "GNU_REGEX")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_GNU_REGEX;
		}
		else if (_stricmp(Str, "SYNTAX_JAVA")==0 || _stricmp(Str, "JAVA")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_JAVA;
		}
		else if (_stricmp(Str, "SYNTAX_PERL")==0 || _stricmp(Str, "PERL")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_PERL;
		}
		else if (_stricmp(Str, "SYNTAX_PERL_NG")==0 || _stricmp(Str, "PERL_NG")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_PERL_NG;
		}
		else if (_stricmp(Str, "SYNTAX_RUBY")==0 || _stricmp(Str, "RUBY")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_RUBY;
		}

		// Option
		else if (_stricmp(Str, "OPTION_NONE")==0) {
			if (new_opt != ONIG_OPTION_NONE || opt_none_flag != 0)
				return ErrSyntax;
			new_opt = ONIG_OPTION_NONE;
			opt_none_flag = 1;
		}
		else if (_stricmp(Str, "OPTION_SINGLELINE")==0 || _stricmp(Str, "SINGLELINE")==0) {
			new_opt |= ONIG_OPTION_SINGLELINE;
		}
		else if (_stricmp(Str, "OPTION_MULTILINE")==0 || _stricmp(Str, "MULTILINE")==0) {
			new_opt |= ONIG_OPTION_MULTILINE;
		}
		else if (_stricmp(Str, "OPTION_IGNORECASE")==0 || _stricmp(Str, "IGNORECASE")==0) {
			new_opt |= ONIG_OPTION_IGNORECASE;
		}
		else if (_stricmp(Str, "OPTION_EXTEND")==0 || _stricmp(Str, "EXTEND")==0) {
			new_opt |= ONIG_OPTION_EXTEND;
		}
		else if (_stricmp(Str, "OPTION_FIND_LONGEST")==0 || _stricmp(Str, "FIND_LONGEST")==0) {
			new_opt |= ONIG_OPTION_FIND_LONGEST;
		}
		else if (_stricmp(Str, "OPTION_FIND_NOT_EMPTY")==0 || _stricmp(Str, "FIND_NOT_EMPTY")==0) {
			new_opt |= ONIG_OPTION_FIND_NOT_EMPTY;
		}
		else if (_stricmp(Str, "OPTION_NEGATE_SINGLELINE")==0 || _stricmp(Str, "NEGATE_SINGLELINE")==0) {
			new_opt |= ONIG_OPTION_NEGATE_SINGLELINE;
		}
		else if (_stricmp(Str, "OPTION_DONT_CAPTURE_GROUP")==0 || _stricmp(Str, "DONT_CAPTURE_GROUP")==0) {
			new_opt |= ONIG_OPTION_DONT_CAPTURE_GROUP;
		}
		else if (_stricmp(Str, "OPTION_CAPTURE_GROUP")==0 || _stricmp(Str, "CAPTURE_GROUP")==0) {
			new_opt |= ONIG_OPTION_CAPTURE_GROUP;
		}

		else {
			return ErrSyntax;
		}
	}


	if (new_enc != ONIG_ENCODING_UNDEF) {
		RegexEnc = new_enc;
	}
	if (new_syntax != NULL) {
		RegexSyntax = new_syntax;
	}
	if (new_opt != ONIG_OPTION_NONE || opt_none_flag != 0) {
		RegexOpt = new_opt;
	}

	return 0;
}

WORD TTLReturn()
{
	if (GetFirstChar()==0)
		return ReturnFromSub();
	else
		return ErrSyntax;
}

// add 'rotateleft' and 'rotateright' (2007.8.19 maya)
#define ROTATE_DIR_LEFT 0
#define ROTATE_DIR_RIGHT 1
WORD BitRotate(int direction)
{
	TVarId VarId;
	WORD Err;
	int x, n;

	Err = 0;
	GetIntVar(&VarId, &Err);
	GetIntVal(&x, &Err);
	GetIntVal(&n, &Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (direction == ROTATE_DIR_RIGHT)
		n = -n;

	n %= INT_BIT;

	if (n < 0)
		n += INT_BIT;

	if (n == 0) {
		SetIntVal(VarId, x);
	} else {
		SetIntVal(VarId, (x << n) | ((unsigned int)x >> (INT_BIT-n)));
	}
	return Err;
}

WORD TTLRotateLeft()
{
	return BitRotate(ROTATE_DIR_LEFT);
}

WORD TTLRotateRight()
{
	return BitRotate(ROTATE_DIR_RIGHT);
}

WORD TTLSend()
{
	TStrVal Str;
	WORD Err, ValType;
	int Val;
	BOOL EndOfLine;

	if (! Linked)
		return ErrLinkFirst;

	EndOfLine = FALSE;
	do {
		if (GetString(Str,&Err))
		{
			if (Err!=0) return Err;
			DDEOut(Str);
		}
		else if (GetExpression(&ValType,&Val,&Err))
		{
			if (Err!=0) return Err;
			switch (ValType) {
				case TypInteger: DDEOut1Byte(LOBYTE(Val)); break;
				case TypString: DDEOut(StrVarPtr((TVarId)Val)); break;
				default:
					return ErrTypeMismatch;
			}
		}
		else
			EndOfLine = TRUE;
	} while (! EndOfLine);
	return 0;
}

/*
 * src に含まれる 0x01 を 0x01 0x02 に置き換えて dst にコピーする。
 * TStrVal には 0x00 が含まれる事が無い(終端と区別できない)ので 0x00 は考慮する必要なし。
 */
static void AddBroadcastString(char *dst, int dstlen, char *src)
{
	int i;

	i = strlen(dst);
	dst += i;
	dstlen -= i;

	while (*src != 0 && dstlen > 1) {
		if (*src == 0x01) {
			// 0x01 を格納するには 0x01 0x02 の2バイト + NUL 終端用の1バイトが必要
			if (dstlen < 3) {
				break;
			}
			*dst++ = *src++;
			*dst++ = 0x02;
			dstlen -= 2;
		}
		else {
			*dst++ = *src++;
			dstlen--;
		}
	}

	*dst = 0;
}

/*
 * TTLSendBroadcast / TTLSendMulticast の下請け
 *
 * 各パラメータを連結した文字列を buff に格納して返す。
 * crlf が TRUE の時は各パラメータの間に "\n" を挟む。(要検討)
 *
 * パラメータが String の場合はそのまま、Integer の場合は ASCII コードとみなしてその文字を送る。
 * Tera Term 側では send 等と共通のルーチンが使われる為、DDE 通信の為のエンコードを行う必要有り。
 *   0x00 -> 0x01 0x01
 *   0x01 -> 0x01 0x02
 */
static WORD GetBroadcastString(char *buff, int bufflen, BOOL crlf)
{
	TStrVal Str;
	WORD Err, ValType;
	int Val;
	char tmp[3];

	buff[0] = '\0';

	while (1) {
		if (GetString(Str, &Err)) {
			if (Err!=0) return Err;
			AddBroadcastString(buff, bufflen, Str);
		}
		else if (GetExpression(&ValType, &Val, &Err)) {
			if (Err!=0) return Err;
			switch (ValType) {
				case TypInteger:
					Val = LOBYTE(Val);
					if (Val == 0 || Val == 1) {
						tmp[0] = 1;
						tmp[1] = Val + 1;
						tmp[2] = 0;
					}
					else {
						tmp[0] = Val;
						tmp[1] = 0;
					}
					strncat_s(buff, bufflen, tmp, _TRUNCATE);
					break;
				case TypString: 
					AddBroadcastString(buff, bufflen, StrVarPtr((TVarId)Val));
					break;
				default:
					return ErrTypeMismatch;
			}
		}
		else {
			break;
		}
	}
	if (crlf) {
		strncat_s(buff, bufflen, "\r\n", _TRUNCATE);
	}
	return 0;
}

// sendbroadcast / sendlnbroadcast の二つから利用 (crlfの値で動作を変える)
static WORD TTLSendBroadcast(BOOL crlf)
{
	TStrVal buf;
	WORD Err;

	if (! Linked)
		return ErrLinkFirst;

	if ((Err = GetBroadcastString(buf, MaxStrLen, crlf)) != 0)
		return Err;

	SetFile(buf);
	return SendCmnd(CmdSendBroadcast, 0);
}

WORD TTLSetMulticastName()
{
	TStrVal Str;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);
	if (Err!=0) return Err;

	SetFile(Str);
	return SendCmnd(CmdSetMulticastName, 0);
}

// sendmulticast / sendlnmulticast の二つから利用 (crlfの値で動作を変える)
WORD TTLSendMulticast(BOOL crlf)
{
	TStrVal buf, Str;
	WORD Err;

	if (! Linked)
		return ErrLinkFirst;

	// マルチキャスト識別用の名前を取得する。
	Err = 0;
	GetStrVal(Str,&Err);
	if (Err!=0) return Err;
	SetFile(Str);

	if ((Err = GetBroadcastString(buf, MaxStrLen, crlf)) != 0)
		return Err;

	SetSecondFile(buf);
	return SendCmnd(CmdSendMulticast, 0);
}

WORD TTLSendFile()
{
	TStrVal Str;
	WORD Err;
	int BinFlag;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&BinFlag,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	SetFile(Str);
	SetBinary(BinFlag);
	return SendCmnd(CmdSendFile,IdTTLWaitCmndEnd);
}

WORD TTLSendKCode()
{
	TStrVal Str;
	WORD Err;
	int KCode, Count;

	Err = 0;
	GetIntVal(&KCode,&Err);
	GetIntVal(&Count,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Word2HexStr((WORD)KCode,Str);
	Word2HexStr((WORD)Count,&Str[4]);
	SetFile(Str);
	return SendCmnd(CmdSendKCode,0);
}

WORD TTLSendLn()
{
	WORD Err;
	char Str[3];

	Err = TTLSend();
	if (Err==0)
	{
		Str[0] = 0x0D;
		Str[1] = 0x0A;
		Str[2] = 0;
		if (Linked)
			DDEOut(Str);
		else
			Err = ErrLinkFirst;
	}
	return Err;
}

WORD TTLSetDate()
{
	WORD Err;
	TStrVal Str;
	int y, m, d;
	SYSTEMTIME Time;

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Str[4] = 0;
	if (sscanf(Str,"%u",&y)!=1) return 0;
	Str[7] = 0;
	if (sscanf(&(Str[5]),"%u",&m)!=1) return 0;
	Str[10] = 0;
	if (sscanf(&(Str[8]),"%u",&d)!=1) return 0;
	GetLocalTime(&Time);
	Time.wYear = y;
	Time.wMonth = m;
	Time.wDay = d;
	SetLocalTime(&Time);
	return Err;
}

WORD TTLSetDir()
{
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	TTMSetDir(Str);

	return Err;
}

WORD TTLSetDlgPos()
{
	WORD Err;
	int x, y;

	Err = 0;
	GetIntVal(&x,&Err);
	GetIntVal(&y,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	SetDlgPos(x,y);
	return Err;
}

// reactivate 'setenv' (2007.8.31 maya)
WORD TTLSetEnv()
{
	WORD Err;
	TStrVal Str, Str2;

	Err = 0;
	GetStrVal(Str,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	_putenv_s(Str,Str2);
	return Err;
}

WORD TTLSetExitCode()
{
	WORD Err;
	int Val;

	Err = 0;
	GetIntVal(&Val,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	ExitCode = Val;
	return Err;
}

WORD TTLSetFileAttr()
{
	WORD Err;
	TStrVal Filename;
	int attributes;

	Err = 0;
	GetStrVal(Filename,&Err);
	GetIntVal(&attributes,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (SetFileAttributes(Filename, attributes) == 0) {
		SetResult(0);
	}
	else {
		SetResult(1);
	}

	return Err;
}

WORD TTLSetSync()
{
	WORD Err;
	int Val;

	Err = 0;
	GetIntVal(&Val,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	if (Val==0)
		SetSync(FALSE);
	else
		SetSync(TRUE);

	return 0;
}

WORD TTLSetTime()
{
	WORD Err;
	TStrVal Str;
	int h, m, s;
	SYSTEMTIME Time;

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Str[2] = 0;
	if (sscanf(Str,"%u",&h)!=1) return 0;
	Str[5] = 0;
	if (sscanf(&(Str[3]),"%u",&m)!=1) return 0;
	Str[8] = 0;
	if (sscanf(&(Str[6]),"%u",&s)!=1) return 0;

	GetLocalTime(&Time);
	Time.wHour = h;
	Time.wMinute = m;
	Time.wSecond = s;
	SetLocalTime(&Time);

	return Err;
}

WORD TTLShow()
{
	WORD Err;
	int Val;

	Err = 0;
	GetIntVal(&Val,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	if (Val==0)
		ShowWindow(HMainWin,SW_MINIMIZE);
	else if (Val>0) {
		ShowWindow(HMainWin,SW_RESTORE);
		PostMessage(HMainWin, WM_USER_MACROBRINGUP, 0, 0);
	}
	else
		ShowWindow(HMainWin,SW_HIDE);
	return Err;
}

// 'sprintf': Format a string in the style of sprintf
//
// (2007.5.1 yutaka)
// (2007.5.3 maya)
WORD TTLSprintf(int getvar)
{
	TStrVal Fmt;
	int Num, NumWidth, NumPrecision;
	TStrVal Str;
	WORD Err = 0, TmpErr;
	TVarId VarId;
	char buf[MaxStrLen];
	char *p, subFmt[MaxStrLen], buf2[MaxStrLen];
	int width_asterisk, precision_asterisk, reg_beg, reg_end, reg_len, i;
	char *match_str;

	enum arg_type {
		INTEGER,
		DOUBLE,
		STRING,
		NONE
	};
	enum arg_type type;

	int r;
	unsigned char *start, *range, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	UChar* pattern, * str;

	if (getvar) {
		GetStrVar(&VarId, &Err);
		if (Err!=0) {
			SetResult(4);
			return Err;
		}
	}

//	pattern = (UChar* )"^%[-+0 #]*(?:[1-9][0-9]*)?(?:\\.[0-9]*)?$";
	pattern = (UChar* )"^%[-+0 #]*([1-9][0-9]*|\\*)?(?:\\.([0-9]*|\\*))?$";
	//               flags--------
	//                       width------------------
	//                                     precision--------------------

	r = onig_new(&reg, pattern, pattern + strlen(pattern),
	             ONIG_OPTION_NONE, ONIG_ENCODING_ASCII, ONIG_SYNTAX_DEFAULT,
	             &einfo);
	if (r != ONIG_NORMAL) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str(s, r, &einfo);
		fprintf(stderr, "ERROR: %s\n", s);
		SetResult(-1);
		goto exit2;
	}

	region = onig_region_new();

	GetStrVal(Fmt, &Err);
	if (Err!=0) {
		SetResult(1);
		goto exit2;
	}

	p = Fmt;
	memset(buf, 0, sizeof(buf));
	memset(subFmt, 0, sizeof(subFmt));
	while(*p != '\0') {
		if (strlen(subFmt)>0) {
			type = NONE;
			switch (*p) {
				case '%':
					if (strlen(subFmt) == 1) { // "%%" -> "%"
						strncat_s(buf, sizeof(buf), "%", _TRUNCATE);
						memset(subFmt, 0, sizeof(subFmt));
					}
					else {
						// 一つ手前までをそのまま buf に格納
						strncat_s(buf, sizeof(buf), subFmt, _TRUNCATE);
						// 仕切り直し
						memset(subFmt, 0, sizeof(subFmt));
						strncat_s(subFmt, sizeof(subFmt), p, 1);
					}
					break;

				case 'c':
				case 'd':
				case 'i':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
					type = INTEGER;

				case 'e':
				case 'E':
				case 'f':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
					if (type == NONE) {
						type = DOUBLE;
					}

				case 's':
					if (type == NONE) {
						type = STRING;
					}

					// "%" と *p の間が正しいかチェック
					str = (UChar* )subFmt;
					end   = str + strlen(subFmt);
					start = str;
					range = end;
					r = onig_search(reg, str, end, start, range, region,
					                ONIG_OPTION_NONE);
					if (r != 0) {
						SetResult(2);
						Err = ErrSyntax;
						goto exit1;
					}

					strncat_s(subFmt, sizeof(subFmt), p, 1);

					// width, precision が * かどうかチェック
					width_asterisk = precision_asterisk = 0;
					if (region->num_regs != 3) {
						SetResult(-1);
						goto exit2;
					}
					reg_beg = region->beg[1];
					reg_end = region->end[1];
					reg_len = reg_end - reg_beg;
					match_str = (char*)calloc(reg_len + 1, sizeof(char));
					for (i = 0; i < reg_len; i++) {
						match_str[i] = str[reg_beg + i];
					}
					if (strcmp(match_str, "*") == 0) {
						width_asterisk = 1;
					}
					free(match_str);
					reg_beg = region->beg[2];
					reg_end = region->end[2];
					reg_len = reg_end - reg_beg;
					match_str = (char*)calloc(reg_len + 1, sizeof(char));
					for (i = 0; i < reg_len; i++) {
						match_str[i] = str[reg_beg + i];
					}
					if (strcmp(match_str, "*") == 0) {
						precision_asterisk = 1;
					}
					free(match_str);

					// * に対応する引数を取得
					if (width_asterisk) {
						TmpErr = 0;
						GetIntVal(&NumWidth, &TmpErr);
						if (TmpErr != 0) {
							SetResult(3);
							Err = TmpErr;
							goto exit1;
						}
					}
					if (precision_asterisk) {
						TmpErr = 0;
						GetIntVal(&NumPrecision, &TmpErr);
						if (TmpErr != 0) {
							SetResult(3);
							Err = TmpErr;
							goto exit1;
						}
					}

					if (type == STRING || type == DOUBLE) {
						// 文字列として読めるかトライ
						TmpErr = 0;
						GetStrVal(Str, &TmpErr);
						if (TmpErr == 0) {
							if (type == STRING) {
								if (!width_asterisk && !precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, Str);
								}
								else if (width_asterisk && !precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, Str);
								}
								else if (!width_asterisk && precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumPrecision, Str);
								}
								else { // width_asterisk && precision_asterisk
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, NumPrecision, Str);
								}
							}
							else { // DOUBLE
								if (!width_asterisk && !precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, atof(Str));
								}
								else if (width_asterisk && !precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, atof(Str));
								}
								else if (!width_asterisk && precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumPrecision, atof(Str));
								}
								else { // width_asterisk && precision_asterisk
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, NumPrecision, atof(Str));
								}
							}
						}
						else {
							SetResult(3);
							Err = TmpErr;
							goto exit1;
						}
					}
					else {
						// 数値として読めるかトライ
						TmpErr = 0;
						GetIntVal(&Num, &TmpErr);
						if (TmpErr == 0) {
							if (!width_asterisk && !precision_asterisk) {
								_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, Num);
							}
							else if (width_asterisk && !precision_asterisk) {
								_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, Num);
							}
							else if (!width_asterisk && precision_asterisk) {
								_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumPrecision, Num);
							}
							else { // width_asterisk && precision_asterisk
								_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, NumPrecision, Num);
							}
						}
						else {
							SetResult(3);
							Err = TmpErr;
							goto exit1;
						}
					}

					strncat_s(buf, sizeof(buf), buf2, _TRUNCATE);
					memset(subFmt, 0, sizeof(subFmt));
					onig_region_free(region, 0);
					break;

				default:
					strncat_s(subFmt, sizeof(subFmt), p, 1);
			}
		}
		else if (*p == '%') {
			strncat_s(subFmt, sizeof(subFmt), p, 1);
		}
		else if (strlen(buf) < sizeof(buf)-1) {
			strncat_s(buf, sizeof(buf), p, 1);
		}
		else {
			break;
		}
		p++;
	}
	if (strlen(subFmt) > 0) {
		strncat_s(buf, sizeof(buf), subFmt, _TRUNCATE);
	}

	if (getvar) {
		SetStrVal(VarId, buf);
	}
	else {
		// マッチした行を inputstr へ格納する
		SetInputStr(buf);  // ここでバッファがクリアされる
	}
	SetResult(0);

exit1:
	onig_region_free(region, 1);
exit2:
	onig_free(reg);
	onig_end();

	return Err;
}

WORD TTLStatusBox()
{
	WORD Err;

	MessageCommand(IdStatusBox, &Err);
	return Err;
}

WORD TTLStr2Code()
{
	TVarId VarId;
	WORD Err;
	TStrVal Str;
	int Len, c, i;
	unsigned int Num;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	Len = strlen(Str);
	if (Len > sizeof(Num))
		c = sizeof(Num);
	else
		c = Len;
	Num = 0;
	for (i=c; i>=1; i--)
		Num = Num*256 + (BYTE)Str[Len-i];
	SetIntVal(VarId,Num);

	return Err;
}

WORD TTLStr2Int()
{
	TVarId VarId;
	WORD Err;
	TStrVal Str;
	int Num;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	// C言語では16進は0xで始まるが、TTL仕様では $ で始まるため、後者もサポートする。
	if (Str[0] == '$') {
		memmove_s(Str + 2, sizeof(Str) - 2, Str + 1, strlen(Str));
		Str[0] = '0';
		Str[1] = 'x';
	}

	// '%d'から'%i'へ変更により、10進以外の数値を変換できるようにする。 (2007.5.1 yutaka)
	// 下位互換性のため10進と16進のみのサポートとする。(2007.5.2 yutaka)
	// 10 : decimal
	// 0x10, $10: hex
	if (Str[0] == '0' && tolower(Str[1]) == 'x') {
		if (sscanf(Str,"%i",&Num)!=1)
		{
			Num = 0;
			SetResult(0);
		}
		else {
			SetResult(1);
		}
	} else {
		if (sscanf(Str,"%d",&Num)!=1)
		{
			Num = 0;
			SetResult(0);
		}
		else {
			SetResult(1);
		}
	}
	SetIntVal(VarId,Num);

	return Err;
}

WORD TTLStrCompare()
{
	TStrVal Str1, Str2;
	WORD Err;
	int i;

	Err = 0;
	GetStrVal(Str1,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	i = strcmp(Str1,Str2);
	if (i<0)
		i = -1;
	else if (i>0)
		i = 1;
	SetResult(i);
	return Err;
}

WORD TTLStrConcat()
{
	TVarId VarId;
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	strncat_s(StrVarPtr(VarId),MaxStrLen,Str,_TRUNCATE);
	return Err;
}

WORD TTLStrCopy()
{
	WORD Err;
	TVarId VarId;
	int From, Len, SrcLen;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&From,&Err);
	GetIntVal(&Len,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (From<1) From = 1;
	SrcLen = strlen(Str)-From+1;
	if (Len > SrcLen) Len = SrcLen;
	if (Len < 0) Len = 0;
	memcpy(StrVarPtr(VarId),&(Str[From-1]),Len);
	StrVarPtr(VarId)[Len] = 0;
	return Err;
}

WORD TTLStrLen()
{
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	SetResult(strlen(Str));
	return Err;
}

/*
  書式: strmatch <文字列> <正規表現>
  <文字列>に<正規表現>がマッチするか調べるコマンド(awkのmatch関数相当)。
  resultには、マッチした位置をセット(マッチしない場合は0)。
  マッチした場合は、waitregexと同様にmatchstr,groupmatchstr1-9をセット。
 */
WORD TTLStrMatch()
{
	WORD Err;
	TStrVal Str1, Str2;
	int ret, result;

	Err = 0;
	GetStrVal(Str1,&Err);   // target string
	GetStrVal(Str2,&Err);   // regex pattern
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	ret = FindRegexStringOne(Str2, strlen(Str2), Str1, strlen(Str1));
	if (ret > 0) { // matched
		result = ret; 
	} else {
		result = 0;
	}

	// FindRegexStringOneの中でUnlockVar()されてしまうので、LockVar()しなおす。
	LockVar();

	SetResult(result);

	return Err;
}

WORD TTLStrScan()
{
	WORD Err;
	TStrVal Str1, Str2;
	char *p;

	Err = 0;
	GetStrVal(Str1,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if ((Str1[0] == 0) || (Str2[0] == 0)) {
		SetResult(0);
		return Err;
	}

	if ((p = _mbsstr(Str1, Str2)) != NULL) {
		SetResult(p - Str1 + 1);
	}
	else {
		SetResult(0);
	}
	return Err;
}

static void insert_string(char *str, int index, char *addstr)
{
	char *np;
	int srclen;
	int addlen;

	srclen = strlen(str);
	addlen = strlen(addstr);

	// まずは挿入される箇所以降のデータを、後ろに移動する。
	np = str + (index - 1);
	memmove_s(np + addlen, MaxStrLen, np, srclen - (index - 1));

	// 文字列を挿入する
	memcpy(np, addstr, addlen);

	// null-terminate
	str[srclen + addlen] = '\0';
}

WORD TTLStrInsert()
{
	WORD Err;
	TVarId VarId;
	int Index;
	TStrVal Str;
	int srclen, addlen;
	char *srcptr;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetIntVal(&Index,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	srcptr = StrVarPtr(VarId);
	srclen = strlen(srcptr);
	if (Index <= 0 || Index > srclen+1) {
		Err = ErrSyntax;
	}
	addlen = strlen(Str);
	if (srclen + addlen + 1 > MaxStrLen) {
		Err = ErrSyntax;
	}
	if (Err!=0) return Err;

	insert_string(srcptr, Index, Str);

	return Err;
}

// 文字列 str の index 文字目（1オリジン）から len 文字削除する
static void remove_string(char *str, int index, int len)
{
	char *np;
	int srclen, copylen;

	srclen = strlen(str);

	if (len <=0 || index <= 0 || (index-1 + len) > srclen) {
		return;
	}

	/*
	   remove_string(str, 6, 4);

	   <------------>srclen
			 <-->len
	   XXXXXX****YYY
	        ^index(np)
			     ^np+len 
				 <-->srclen - len - index
		    ↓
	   XXXXXXYYY
	 */

	np = str + (index - 1);
	copylen = srclen - len - (index - 1);
	if (copylen > 0)
		memmove_s(np, MaxStrLen, np + len, copylen);

	// null-terminate
	str[srclen - len] = '\0';
}

WORD TTLStrRemove()
{
	WORD Err;
	TVarId VarId;
	int Index, Len;
	int srclen;
	char *srcptr;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetIntVal(&Index,&Err);
	GetIntVal(&Len,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	srcptr = StrVarPtr(VarId);
	srclen = strlen(srcptr);
	if (Len <=0 || Index <= 0 || (Index-1 + Len) > srclen) {
		Err = ErrSyntax;
	}
	if (Err!=0) return Err;

	remove_string(srcptr, Index, Len);

	return Err;
}

WORD TTLStrReplace()
{
	WORD Err, VarType;
	TVarId VarId;
	TStrVal oldstr;
	TStrVal newstr;
	TStrVal tmpstr;
	char *srcptr, *matchptr;
	char *p;
	int srclen, oldlen, matchlen;
	int pos, ret;
	int result = 0;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetIntVal(&pos,&Err);
	GetStrVal(oldstr,&Err);
	GetStrVal(newstr,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	srcptr = StrVarPtr(VarId);
	srclen = strlen(srcptr);

	if (pos > srclen || pos <= 0) {
		result = 0;
		goto error;
	}
	pos--;

	strncpy_s(tmpstr, MaxStrLen, srcptr, _TRUNCATE);

	oldlen = strlen(oldstr);

	// strptr文字列の pos 文字目以降において、oldstr を探す。
	p = tmpstr + pos;
	ret = FindRegexStringOne(oldstr, oldlen, p, strlen(p));
	// FindRegexStringOneの中でUnlockVar()されてしまうので、LockVar()しなおす。
	LockVar();
	if (ret == 0) {
		// 見つからなかった場合は、"0"で戻る。
		result = 0;
		goto error;
	}
	else if (ret < 0) {
		// 正しくない正規表現等でエラーの場合は -1 を返す
		result = -1;
		goto error;
	}
	ret--;

	if (CheckVar("matchstr",&VarType,&VarId) &&
		(VarType==TypString)) {
		matchptr = StrVarPtr(VarId);
		matchlen = strlen(matchptr);
	} else {
		result = 0;
		goto error;
	}

	strncpy_s(srcptr, MaxStrLen, tmpstr, pos + ret);
	strncat_s(srcptr, MaxStrLen, newstr, _TRUNCATE);
	strncat_s(srcptr, MaxStrLen, tmpstr + pos + ret + matchlen, _TRUNCATE);

	result = 1;

error:
	SetResult(result);
	return Err;
}

WORD TTLStrSpecial()
{
	WORD Err;
	TVarId VarId;
	TStrVal srcstr;

	Err = 0;
	GetStrVar(&VarId,&Err);
	if (Err!=0) return Err;

	if (CheckParameterGiven()) { // strspecial strvar strval
		GetStrVal(srcstr,&Err);
		if ((Err==0) && (GetFirstChar()!=0))
			Err = ErrSyntax;
		if (Err!=0) {
			return Err;
		}

		RestoreNewLine(srcstr);
		SetStrVal(VarId, srcstr);
	}
	else { // strspecial strvar
		RestoreNewLine(StrVarPtr(VarId));
	}

	return Err;
}

WORD TTLStrTrim()
{
	TStrVal trimchars;
	WORD Err;
	TVarId VarId;
	int srclen;
	int i, start, end;
	char *srcptr, *p;
	char table[256];

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(trimchars,&Err);

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	srcptr = StrVarPtr(VarId);
	srclen = strlen(srcptr);

	// 削除する文字のテーブルを作る。
	memset(table, 0, sizeof(table));
	for (p = trimchars; *p ; p++) {
		table[*p] = 1;
	}

	// 文字列の先頭から検索する
	for (i = 0 ; i < srclen ; i++) {
		if (table[srcptr[i]] == 0) 
			break;
	}
	// 削除されない有効な文字列の始まり。
	// すべて削除対象となる場合は、start == srclen 。
	start = i;  

	// 文字列の末尾から検索する
	for (i = srclen - 1 ; i >= 0 ; i--) {
		if (table[srcptr[i]] == 0) 
			break;
	}
	// 削除されない有効な文字列の終わり。
	// すべて削除対象となる場合は、end == -1 。
	end = i;

	// 末尾を削る
	srcptr[end + 1] = '\0';

	// 次に、先頭から削る。
	remove_string(srcptr, 1, start);

	return Err;
}

WORD TTLStrSplit()
{
#define MAXVARNUM 9
	TStrVal src, delimchars, buf;
	WORD Err;
	int maxvar;
	int srclen, len;
	int i, count;
	BOOL ary = FALSE, omit = FALSE;
	char *p;
	char /* *last, */ *tok[MAXVARNUM];

	Err = 0;
	GetStrVal(src,&Err);
	GetStrVal(delimchars,&Err);
	// 3rd arg (optional)
	if (CheckParameterGiven()) {
		GetIntVal(&maxvar,&Err);
		if (Err!=0) {
			// TODO array
#if 0
			Err = 0;
			// Parameter から array を受け取る
			if (Err==0) {
				ary = TRUE;
			}
#endif
		}
	}
	else {
		maxvar = 9;
		omit = TRUE;
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!ary && (maxvar < 1 || maxvar > MAXVARNUM) )
		return ErrSyntax;

	// デリミタは1文字のみとする。
	len = strlen(delimchars);
	if (len != 1)
		return ErrSyntax;

	srclen = strlen(src);
	strcpy_s(buf, MaxStrLen, src);  /* 破壊されてもいいように、コピーバッファを使う。*/

#if 0
	// トークンの切り出しを行う。
	memset(tok, 0, sizeof(tok));
#if 0
	tok[0] = strtok_s(srcptr, delimchars, &last);
	for (i = 1 ; i < MAXVARNUM ; i++) {
		tok[i] = strtok_s(NULL, delimchars, &last);
		if (tok[i] == NULL)
			break;
	} 
#else
	/* strtokを使うと、連続した区切りが1つに丸められるため、自前でポインタを
	 * たどる。ただし、区切り文字は1つのみとする。
	 */
	i = 0;
	for (p = buf; *p == delimchars[0] ; p++) {
		tok[i++] = NULL;
		if (i >= maxvar)
			goto end;
	}
	
	for (p = strtok_s(p, delimchars, &last); p != NULL ; p = strtok_s(NULL, delimchars, &last) ) {
		tok[i++] = p;
		if (i >= maxvar)
			goto end;
		for (p += strlen(p) + 1 ; *p == delimchars[0] ; p++) {
			tok[i++] = NULL;
			if (i >= maxvar)
				goto end;
		}
	}
#endif
#else
	if (ary) {
		// TODO array
	}
	else {
		p = buf;
		count = 1;
		tok[count-1] = p;
		for (i=0; i < srclen && count < maxvar + omit; i++) { // count 省略時には、超過分を捨てるため 1 つ余分に進める
			if (*p == *delimchars) {
				*p = '\0';
				count++;
				if (count <= MAXVARNUM) { // tok の要素数を超えて代入しないようにする(count 省略時のため)
					tok[count-1] = p+1;
				}
			}
			p++;
		}
	}
#endif

//end:
	// 結果の格納
	for (i = 1 ; i <= count ; i++) {
		SetGroupMatchStr(i, tok[i-1]);
	}
	for (i = count+1 ; i <= MAXVARNUM ; i++) {
		SetGroupMatchStr(i, "");
	}
	SetResult(count);
	return Err;
#undef MAXVARNUM
}

WORD TTLStrJoin()
{
#define MAXVARNUM 9
	TStrVal delimchars, buf;
	WORD Err;
	TVarId VarId;
	WORD VarType;
	int maxvar;
	int srclen;
	int i;
	BOOL ary = FALSE;
	char *srcptr, *p;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(delimchars,&Err);
	// 3rd arg (optional)
	if (CheckParameterGiven()) {
		GetIntVal(&maxvar,&Err);
		if (Err!=0) {
			// TODO array
#if 0
			Err = 0;
			// Parameter から array を受け取る
			if (Err==0) {
				ary = TRUE;
			}
#endif
		}
	}
	else {
		maxvar = 9;
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!ary && (maxvar < 1 || maxvar > MAXVARNUM) )
		return ErrSyntax;

	srcptr = StrVarPtr(VarId);
	srclen = strlen(srcptr);

	srcptr[0] = '\0';
	if (ary) {
		// TODO array
	}
	else {
		for (i = 0 ; i < maxvar ; i++) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "groupmatchstr%d", i + 1);
			if (CheckVar(buf,&VarType,&VarId)) {
				if (VarType!=TypString)
					return ErrSyntax;
				p = StrVarPtr(VarId);
				strncat_s(srcptr, MaxStrLen, p, _TRUNCATE);
				if (i < maxvar-1) {
					strncat_s(srcptr, MaxStrLen, delimchars, _TRUNCATE);
				}
			}
		}
	}

	return Err;
#undef MAXVARNUM
}

WORD TTLTestLink()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (! Linked)
		SetResult(0);
	else if (ComReady==0)
		SetResult(1);
	else
		SetResult(2);

	return 0;
}

// added (2007.7.12 maya)
WORD TTLToLower()
{
	WORD Err;
	TVarId VarId;
	TStrVal Str;
	int i=0;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	while (Str[i] != 0) {
		if(_ismbblead(Str[i])) {
			i = i + 2;
			continue;
		}
		if (Str[i] >= 'A' && Str[i] <= 'Z') {
			Str[i] = Str[i] + 0x20;
		}
		i++;
	}

	strncpy_s(StrVarPtr(VarId), MaxStrLen, Str, _TRUNCATE);
	return Err;
}

// added (2007.7.12 maya)
WORD TTLToUpper()
{
	WORD Err;
	TVarId VarId;
	TStrVal Str;
	int i=0;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	while (Str[i] != 0) {
		if(_ismbblead(Str[i])) {
			i = i + 2;
			continue;
		}
		if (Str[i] >= 'a' && Str[i] <= 'z') {
			Str[i] = Str[i] - 0x20;
		}
		i++;
	}

	strncpy_s(StrVarPtr(VarId), MaxStrLen, Str, _TRUNCATE);
	return Err;
}

WORD TTLUnlink()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (Linked)
	{
		EndDDE();
	}
	return 0;
}


WORD TTLUptime()
{
	WORD Err;
	TVarId VarId;
	DWORD tick;

	Err = 0;
	GetIntVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// Windows OSが起動してからの経過時間（ミリ秒）を取得する。ただし、49日を経過すると、0に戻る。
	// GetTickCount64() API(Vista以降)を使うと、オーバーフローしなくなるが、そもそもTera Termでは
	// 64bit変数をサポートしていないので、意味がない。
	tick = GetTickCount();

	SetIntVal(VarId, tick);

	return Err;
}



WORD TTLWait(BOOL Ln)
{
	TStrVal Str;
	WORD Err, ValType;
	TVarId VarId;
	int i, Val;
	int TimeOut;

	ClearWait();

	for (i=0; i<10; i++) {
		Err = 0;
		if (GetString(Str, &Err)) {
			SetWait(i+1, Str);
		}
		else if (GetExpression(&ValType, &Val, &Err) && Err == 0) {
			if (ValType == TypString)
				SetWait(i+1, StrVarPtr((TVarId)Val));
			else
				Err = ErrTypeMismatch;
		}
		else
			break;

		if (Err)
			break;
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (! Linked)
		Err = ErrLinkFirst;

	if ((Err==0) && (i>0))
	{
		if (Ln)
			TTLStatus = IdTTLWaitLn;
		else
			TTLStatus = IdTTLWait;
		TimeOut = 0;
		if (CheckVar("timeout",&ValType,&VarId) && (ValType==TypInteger)) {
			TimeOut = CopyIntVal(VarId) * 1000;
		}
		if (CheckVar("mtimeout",&ValType,&VarId) && (ValType==TypInteger)) {
			TimeOut += CopyIntVal(VarId);
		}

		if (TimeOut>0)
		{
			TimeLimit = (DWORD)TimeOut;
			TimeStart = GetTickCount();
			SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
		}
	}
	else
		ClearWait();

	return Err;
}


WORD TTLWait4all(BOOL Ln)
{
	WORD Err = 0;

	Err = TTLWait(Ln);
	TTLStatus = IdTTLWait4all;

	Wait4allGotIndex = FALSE;
	Wait4allFoundNum = 0;

	return Err;
}


// 'waitregex'(wait regular expression): wait command with regular expression
//
// This command has almost same function of 'wait' command. Additionally 'waitregex' can search 
// the keyword with regular expression. Tera Term uses a regex library that is called 'Oniguruma'.
// cf. http://www.geocities.jp/kosako3/oniguruma/
//
// (2005.10.5 yutaka)
WORD TTLWaitRegex(BOOL Ln)
{
	WORD ret;

	ret = TTLWait(Ln);

	RegexActionType = REGEX_WAIT; // regex enabled

	return (ret);
}



WORD TTLWaitEvent()
{
	WORD Err, ValType;
	TVarId VarId;
	int TimeOut;

	Err = 0;
	GetIntVal(&WakeupCondition,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	WakeupCondition &= 15;
	TimeOut = 0;
	if (CheckVar("timeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut = CopyIntVal(VarId) * 1000;
	}
	if (CheckVar("mtimeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut += CopyIntVal(VarId);
	}

	if (TimeOut>0)
	{
		TimeLimit = (DWORD)TimeOut;
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
	}

	TTLStatus = IdTTLSleep;
	return Err;
}


WORD TTLWaitN()
{
	WORD Err, ValType;
	TVarId VarId;
	int TimeOut, WaitBytes;

	ClearWaitN();

	Err = 0;
	GetIntVal(&WaitBytes,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetWaitN(WaitBytes);

	if (! Linked)
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	TTLStatus = IdTTLWaitN;
	TimeOut = 0;
	if (CheckVar("timeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut = CopyIntVal(VarId) * 1000;
	}
	if (CheckVar("mtimeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut += CopyIntVal(VarId);
	}

	if (TimeOut>0)
	{
		TimeLimit = (DWORD)TimeOut;
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
	}

	return Err;
}


WORD TTLWaitRecv()
{
	TStrVal Str;
	WORD Err;
	int Pos, Len, TimeOut;
	WORD VarType;
	TVarId VarId;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&Len,&Err);
	GetIntVal(&Pos,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;
	SetInputStr("");
	SetWait2(Str,Len,Pos);

	TTLStatus = IdTTLWait2;
	TimeOut = 0;
	if (CheckVar("timeout",&VarType,&VarId) && (VarType==TypInteger)) {
		TimeOut = CopyIntVal(VarId) * 1000;
	}
	if (CheckVar("mtimeout",&VarType,&VarId) && (VarType==TypInteger)) {
		TimeOut += CopyIntVal(VarId);
	}

	if (TimeOut>0)
	{
		TimeLimit = (DWORD)TimeOut;
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
	}
	return Err;
}

WORD TTLWhile(BOOL mode)
{
	WORD Err;
	int Val = mode;

	Err = 0;
	if (CheckParameterGiven()) {
		GetIntVal(&Val,&Err);
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if ((Val!=0) == mode)
		return SetWhileLoop();
	else
		EndWhileLoop();
	return Err;
}

WORD TTLXmodemRecv()
{
	TStrVal Str;
	WORD Err;
	int BinFlag, XOption;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&BinFlag,&Err);
	GetIntVal(&XOption,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	switch (XOption) {
	case XoptCRC:
		// NOP
		break;
	case Xopt1kCRC:
		// for compatibility
		XOption = XoptCRC;
		break;
	default:
		XOption = XoptCheck;
	}

	SetFile(Str);
	SetBinary(BinFlag);
	SetXOption(XOption);
	return SendCmnd(CmdXmodemRecv,IdTTLWaitCmndResult);
}

WORD TTLXmodemSend()
{
	TStrVal Str;
	WORD Err;
	int XOption;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&XOption,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	switch (XOption) {
	case Xopt1kCRC:
		// NOP
		break;
	default:
		XOption = XoptCRC;
	}

	SetFile(Str);
	SetXOption(XOption);
	return SendCmnd(CmdXmodemSend,IdTTLWaitCmndResult);
}

WORD TTLYesNoBox()
{
	WORD Err;
	int YesNo;

	YesNo = MessageCommand(IdYesNoBox, &Err);
	if (Err!=0) return Err;
	if (YesNo==IDOK)
		YesNo = 1;	// Yes
	else
		YesNo = 0;	// No
	SetResult(YesNo);
	return Err;
}

WORD TTLZmodemSend()
{
	TStrVal Str;
	WORD Err;
	int BinFlag;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&BinFlag,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetFile(Str);
	SetBinary(BinFlag);
	return SendCmnd(CmdZmodemSend,IdTTLWaitCmndResult);
}

WORD TTLYmodemSend()
{
	TStrVal Str;
	WORD Err;
//	int BinFlag;

	Err = 0;
	GetStrVal(Str,&Err);
//	GetIntVal(&BinFlag,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetFile(Str);
//	SetBinary(BinFlag);
	return SendCmnd(CmdYmodemSend,IdTTLWaitCmndResult);
}

// SYNOPSIS: 
//   scpsend "c:\usr\sample.chm" "doc/sample.chm"
//   scpsend "c:\usr\sample.chm"
WORD TTLScpSend()
{
	TStrVal Str;
	TStrVal Str2;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);

	if ((Err==0) &&
	    ((strlen(Str)==0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetStrVal(Str2,&Err);
	if (Err) {
		Str2[0] = '\0';
		Err = 0;
	}

	if (GetFirstChar() != 0)
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetFile(Str);
	SetSecondFile(Str2);
	return SendCmnd(CmdScpSend, 0);
}

// SYNOPSIS: 
//   scprecv "foo.txt"
//   scprecv "src/foo.txt" "c:\foo.txt"
WORD TTLScpRecv()
{
	TStrVal Str;
	TStrVal Str2;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);

	if ((Err==0) &&
	    ((strlen(Str)==0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetStrVal(Str2,&Err);
	if (Err) {
		Str2[0] = '\0';
		Err = 0;
	}

	if (GetFirstChar() != 0)
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetFile(Str);
	SetSecondFile(Str2);
	return SendCmnd(CmdScpRcv, 0);
}

int ExecCmnd()
{
	WORD WId, Err;
	BOOL StrConst, E, WithIndex, Result;
	TStrVal Str;
	TName Cmnd;
	WORD ValType, VarType;
	TVarId VarId;
	int Val, Index;

	Err = 0;

	Result = GetReservedWord(&WId);

	if (EndWhileFlag>0) {
		if (Result) {
			switch (WId) {
			case RsvWhile:
			case RsvUntil:
			case RsvDo:
				EndWhileFlag++; break;

			case RsvEndWhile:
			case RsvEndUntil:
			case RsvLoop:
				EndWhileFlag--; break;
			}
		}
		return 0;
	}

	if (BreakFlag>0) {
		if (Result) {
			switch (WId) {
			case RsvIf:
				if (CheckThen(&Err))
					IfNest++;
				break;

			case RsvEndIf:
				if (IfNest<1)
					Err = ErrInvalidCtl;
				else
					IfNest--;
				break;

			case RsvFor:
			case RsvWhile:
			case RsvUntil:
			case RsvDo:
				BreakFlag++; break;

			case RsvNext:
			case RsvEndWhile:
			case RsvEndUntil:
			case RsvLoop:
				BreakFlag--; break;
			}
		}
		if (BreakFlag>0 || !ContinueFlag)
			return Err;
		ContinueFlag = FALSE;
	}

	if (EndIfFlag>0) {
		if (! Result)
			;
		else if ((WId==RsvIf) && CheckThen(&Err))
			EndIfFlag++;
		else if (WId==RsvEndIf)
			EndIfFlag--;
		return Err;
	}

	if (ElseFlag>0) {
		if (! Result)
			;
		else if ((WId==RsvIf) && CheckThen(&Err))
			EndIfFlag++;
		else if (WId==RsvElse)
			ElseFlag--;
		else if (WId==RsvElseIf)
		{
			if (CheckElseIf(&Err)!=0)
				ElseFlag--;
		}
		else if (WId==RsvEndIf)
		{
			ElseFlag--;
			if (ElseFlag==0)
				IfNest--;
		}
		return Err;
	}

	if (Result) 
		switch (WId) {
		case RsvBasename:
			Err = TTLBasename(); break;
		case RsvBeep:
			Err = TTLBeep(); break;
		case RsvBPlusRecv:
			Err = TTLCommCmd(CmdBPlusRecv,IdTTLWaitCmndResult); break;
		case RsvBPlusSend:
			Err = TTLCommCmdFile(CmdBPlusSend,IdTTLWaitCmndResult); break;
		case RsvBreak:
		case RsvContinue:
			Err = TTLBreak(WId); break;
		case RsvBringupBox:
			Err = TTLBringupBox(); break;
		case RsvCall:
			Err = TTLCall(); break;
		case RsvCallMenu:
			Err = TTLCommCmdInt(CmdCallMenu,0); break;
		case RsvChangeDir:
			Err = TTLCommCmdFile(CmdChangeDir,0); break;
		case RsvChecksum8:
			Err = TTLDoChecksum(CHECKSUM8); break;
		case RsvChecksum8File:
			Err = TTLDoChecksumFile(CHECKSUM8); break;
		case RsvChecksum16:
			Err = TTLDoChecksum(CHECKSUM16); break;
		case RsvChecksum16File:
			Err = TTLDoChecksumFile(CHECKSUM16); break;
		case RsvChecksum32:
			Err = TTLDoChecksum(CHECKSUM32); break;
		case RsvChecksum32File:
			Err = TTLDoChecksumFile(CHECKSUM32); break;
		case RsvClearScreen:
			Err = TTLCommCmdInt(CmdClearScreen,0); break;
		case RsvClipb2Var:
			Err = TTLClipb2Var(); break;    // add 'clipb2var' (2006.9.17 maya)
		case RsvCloseSBox:
			Err = TTLCloseSBox(); break;
		case RsvCloseTT:
			Err = TTLCloseTT(); break;
		case RsvCode2Str:
			Err = TTLCode2Str(); break;
		case RsvConnect:
		case RsvCygConnect:
			Err = TTLConnect(WId); break;
		case RsvCrc16:
			Err = TTLDoChecksum(CRC16); break;
		case RsvCrc16File:
			Err = TTLDoChecksumFile(CRC16); break;
		case RsvCrc32:
			Err = TTLDoChecksum(CRC32); break;
		case RsvCrc32File:
			Err = TTLDoChecksumFile(CRC32); break;
		case RsvDelPassword:
			Err = TTLDelPassword(); break;
		case RsvDirname:
			Err = TTLDirname(); break;
		case RsvDirnameBox:
			Err = TTLDirnameBox(); break;
		case RsvDisconnect:
			Err = TTLDisconnect(); break;
		case RsvDispStr:
			Err = TTLDispStr(); break;
		case RsvDo:
			Err = TTLDo(); break;
		case RsvElse:
			Err = TTLElse(); break;
		case RsvElseIf:
			Err = TTLElseIf(); break;
		case RsvEnableKeyb:
			Err = TTLCommCmdBin(CmdEnableKeyb,0); break;
		case RsvEnd:
			Err = TTLEnd(); break;
		case RsvEndIf:
			Err = TTLEndIf(); break;
		case RsvEndUntil:
			Err = TTLEndWhile(FALSE); break;
		case RsvEndWhile:
			Err = TTLEndWhile(TRUE); break;
		case RsvExec:
			Err = TTLExec(); break;
		case RsvExecCmnd:
			Err = TTLExecCmnd(); break;
		case RsvExit:
			Err = TTLExit(); break;
		case RsvExpandEnv:
			Err = TTLExpandEnv(); break;
		case RsvFileClose:
			Err = TTLFileClose(); break;
		case RsvFileConcat:
			Err = TTLFileConcat(); break;
		case RsvFileCopy:
			Err = TTLFileCopy(); break;
		case RsvFileCreate:
			Err = TTLFileCreate(); break;
		case RsvFileDelete:
			Err = TTLFileDelete(); break;
		case RsvFileMarkPtr:
			Err = TTLFileMarkPtr(); break;
		case RsvFilenameBox:
			Err = TTLFilenameBox(); break;  // add 'filenamebox' (2007.9.13 maya)
		case RsvFileOpen:
			Err = TTLFileOpen(); break;
		case RsvFileLock:
			Err = TTLFileLock(); break;
		case RsvFileUnLock:
			Err = TTLFileUnLock(); break;
		case RsvFileReadln:
			Err = TTLFileReadln(); break;
		case RsvFileRead:
			Err = TTLFileRead(); break;    // add 'fileread'
		case RsvFileRename:
			Err = TTLFileRename(); break;
		case RsvFileSearch:
			Err = TTLFileSearch(); break;
		case RsvFileSeek:
			Err = TTLFileSeek(); break;
		case RsvFileSeekBack:
			Err = TTLFileSeekBack(); break;
		case RsvFileStat:
			Err = TTLFileStat(); break;
		case RsvFileStrSeek:
			Err = TTLFileStrSeek(); break;
		case RsvFileStrSeek2:
			Err = TTLFileStrSeek2(); break;
		case RsvFileTruncate:
			Err = TTLFileTruncate(); break;
		case RsvFileWrite:
			Err = TTLFileWrite(FALSE); break;
		case RsvFileWriteLn:
			Err = TTLFileWrite(TRUE); break;
		case RsvFindClose:
			Err = TTLFindClose(); break;
		case RsvFindFirst:
			Err = TTLFindFirst(); break;
		case RsvFindNext:
			Err = TTLFindNext(); break;
		case RsvFlushRecv:
			Err = TTLFlushRecv(); break;
		case RsvFolderCreate:
			Err = TTLFolderCreate(); break;
		case RsvFolderDelete:
			Err = TTLFolderDelete(); break;
		case RsvFolderSearch:
			Err = TTLFolderSearch(); break;
		case RsvFor:
			Err = TTLFor(); break;
		case RsvGetDate:
		case RsvGetTime:
			Err = TTLGetTime(WId); break;
		case RsvGetDir:
			Err = TTLGetDir(); break;
		case RsvGetEnv:
			Err = TTLGetEnv(); break;
		case RsvGetFileAttr:
			Err = TTLGetFileAttr(); break;
		case RsvGetHostname:
			Err = TTLGetHostname(); break;
		case RsvGetIPv4Addr:
			Err = TTLGetIPv4Addr(); break;
		case RsvGetIPv6Addr:
			Err = TTLGetIPv6Addr(); break;
		case RsvGetModemStatus:
			Err = TTLGetModemStatus(); break;
		case RsvGetPassword:
			Err = TTLGetPassword(); break;
		case RsvSetPassword:
			Err = TTLSetPassword(); break;
		case RsvIsPassword:
			Err = TTLIsPassword(); break;
		case RsvGetSpecialFolder:
			Err = TTLGetSpecialFolder(); break;
		case RsvGetTitle:
			Err = TTLGetTitle(); break;
		case RsvGetTTDir:
			Err = TTLGetTTDir(); break;
		case RsvGetVer:
			Err = TTLGetVer(); break;
		case RsvGoto:
			Err = TTLGoto(); break;
		case RsvIfDefined:
			Err = TTLIfDefined(); break;
		case RsvIf:
			Err = TTLIf(); break;
		case RsvInclude:
			Err = TTLInclude(); break;
		case RsvInputBox:
			Err = TTLInputBox(FALSE); break;
		case RsvInt2Str:
			Err = TTLInt2Str(); break;
		case RsvIntDim:
		case RsvStrDim:
			Err = TTLDim(WId); break;
		case RsvKmtFinish:
			Err = TTLCommCmd(CmdKmtFinish,IdTTLWaitCmndResult); break;
		case RsvKmtGet:
			Err = TTLCommCmdFile(CmdKmtGet,IdTTLWaitCmndResult); break;
		case RsvKmtRecv:
			Err = TTLCommCmd(CmdKmtRecv,IdTTLWaitCmndResult); break;
		case RsvKmtSend:
			Err = TTLCommCmdFile(CmdKmtSend,IdTTLWaitCmndResult); break;
		case RsvListBox:
			Err = TTLListBox(); break;
		case RsvLoadKeyMap:
			Err = TTLCommCmdFile(CmdLoadKeyMap,0); break;
		case RsvLogAutoClose:
			Err = TTLCommCmdBin(CmdLogAutoClose, 0); break;
		case RsvLogClose:
			Err = TTLCommCmd(CmdLogClose,0); break;
		case RsvLogInfo:
			Err = TTLLogInfo(); break;
		case RsvLogOpen:
			Err = TTLLogOpen(); break;
		case RsvLogPause:
			Err = TTLCommCmd(CmdLogPause,0); break;
		case RsvLogRotate:
			Err = TTLLogRotate(); break;
		case RsvLogStart:
			Err = TTLCommCmd(CmdLogStart,0); break;
		case RsvLogWrite:
			Err = TTLCommCmdFile(CmdLogWrite,0); break;
		case RsvLoop:
			Err = TTLLoop(); break;
		case RsvMakePath:
			Err = TTLMakePath(); break;
		case RsvMessageBox:
			Err = TTLMessageBox(); break;
		case RsvNext:
			Err = TTLNext(); break;
		case RsvPasswordBox:
			Err = TTLInputBox(TRUE); break;
		case RsvPause:
			Err = TTLPause(); break;
		case RsvMilliPause:
			Err = TTLMilliPause(); break;    // add 'mpause'
		case RsvQuickVANRecv:
			Err = TTLCommCmd(CmdQVRecv,IdTTLWaitCmndResult); break;
		case RsvQuickVANSend:
			Err = TTLCommCmdFile(CmdQVSend,IdTTLWaitCmndResult); break;
		case RsvRandom:
			Err = TTLRandom(); break;
		case RsvRecvLn:
			Err = TTLRecvLn(); break;
		case RsvRegexOption:
			Err = TTLRegexOption(); break;
		case RsvRestoreSetup:
			Err = TTLCommCmdFile(CmdRestoreSetup,0); break;
		case RsvReturn:
			Err = TTLReturn(); break;
		case RsvRotateL:
			Err = TTLRotateLeft(); break;   // add 'rotateleft' (2007.8.19 maya)
		case RsvRotateR:
			Err = TTLRotateRight(); break;  // add 'rotateright' (2007.8.19 maya)
		case RsvScpSend:
			Err = TTLScpSend(); break;      // add 'scpsend' (2008.1.1 yutaka)
		case RsvScpRecv:
			Err = TTLScpRecv(); break;      // add 'scprecv' (2008.1.4 yutaka)
		case RsvSend:
			Err = TTLSend(); break;
		case RsvSendBreak:
			Err = TTLCommCmd(CmdSendBreak,0); break;
		case RsvSendBroadcast:
			Err = TTLSendBroadcast(FALSE); break;
		case RsvSendlnBroadcast:
			Err = TTLSendBroadcast(TRUE); break;
		case RsvSendlnMulticast:
			Err = TTLSendMulticast(TRUE); break;
		case RsvSendMulticast:
			Err = TTLSendMulticast(FALSE); break;
		case RsvSetMulticastName:
			Err = TTLSetMulticastName(); break;
		case RsvSendFile:
			Err = TTLSendFile(); break;
		case RsvSendKCode:
			Err = TTLSendKCode(); break;
		case RsvSendLn:
			Err = TTLSendLn(); break;
		case RsvSetBaud:
			Err = TTLCommCmdInt(CmdSetBaud,0); break;
		case RsvSetDate:
			Err = TTLSetDate(); break;
		case RsvSetDebug:
			Err = TTLCommCmdDeb(); break;
		case RsvSetDir:
			Err = TTLSetDir(); break;
		case RsvSetDlgPos:
			Err = TTLSetDlgPos(); break;
		case RsvSetDtr:
			Err = TTLCommCmdInt(CmdSetDtr,0); break;
		case RsvSetEcho:
			Err = TTLCommCmdBin(CmdSetEcho,0); break;
		case RsvSetEnv:
			Err = TTLSetEnv(); break;
		case RsvSetExitCode:
			Err = TTLSetExitCode(); break;
		case RsvSetFileAttr:
			Err = TTLSetFileAttr(); break;
		case RsvSetFlowCtrl:
			Err = TTLCommCmdInt(CmdSetFlowCtrl,0); break;
		case RsvSetRts:
			Err = TTLCommCmdInt(CmdSetRts,0); break;
		case RsvSetSync:
			Err = TTLSetSync(); break;
		case RsvSetTime:
			Err = TTLSetTime(); break;
		case RsvSetTitle:
			Err = TTLCommCmdFile(CmdSetTitle,0); break;
		case RsvShow:
			Err = TTLShow(); break;
		case RsvShowTT:
			Err = TTLCommCmdInt(CmdShowTT,0); break;
		case RsvSprintf:
			Err = TTLSprintf(0); break;
		case RsvSprintf2:
			Err = TTLSprintf(1); break;
		case RsvStatusBox:
			Err = TTLStatusBox(); break;
		case RsvStr2Code:
			Err = TTLStr2Code(); break;
		case RsvStr2Int:
			Err = TTLStr2Int(); break;
		case RsvStrCompare:
			Err = TTLStrCompare(); break;
		case RsvStrConcat:
			Err = TTLStrConcat(); break;
		case RsvStrCopy:
			Err = TTLStrCopy(); break;
		case RsvStrInsert:
			Err = TTLStrInsert(); break;
		case RsvStrJoin:
			Err = TTLStrJoin(); break;
		case RsvStrLen:
			Err = TTLStrLen(); break;
		case RsvStrMatch:
			Err = TTLStrMatch(); break;
		case RsvStrRemove:
			Err = TTLStrRemove(); break;
		case RsvStrReplace:
			Err = TTLStrReplace(); break;
		case RsvStrScan:
			Err = TTLStrScan(); break;
		case RsvStrSpecial:
			Err = TTLStrSpecial(); break;
		case RsvStrSplit:
			Err = TTLStrSplit(); break;
		case RsvStrTrim:
			Err = TTLStrTrim(); break;
		case RsvTestLink:
			Err = TTLTestLink(); break;
		case RsvToLower:
			Err = TTLToLower(); break;    // add 'tolower' (2007.7.12 maya)
		case RsvToUpper:
			Err = TTLToUpper(); break;    // add 'toupper' (2007.7.12 maya)
		case RsvUnlink:
			Err = TTLUnlink(); break;
		case RsvUntil:
			Err = TTLWhile(FALSE); break;
		case RsvUptime:
			Err = TTLUptime(); break;
		case RsvVar2Clipb:
			Err = TTLVar2Clipb(); break;    // add 'var2clipb' (2006.9.17 maya)
		case RsvWaitRegex:
			Err = TTLWaitRegex(FALSE); break;    // add 'waitregex' (2005.10.5 yutaka)
		case RsvWait:
			Err = TTLWait(FALSE); break;
		case RsvWait4all:
			Err = TTLWait4all(FALSE); break;
		case RsvWaitEvent:
			Err = TTLWaitEvent(); break;
		case RsvWaitLn:
			Err = TTLWait(TRUE); break;
		case RsvWaitN:
			Err = TTLWaitN(); break;
		case RsvWaitRecv:
			Err = TTLWaitRecv(); break;
		case RsvWhile:
			Err = TTLWhile(TRUE); break;
		case RsvXmodemRecv:
			Err = TTLXmodemRecv(); break;
		case RsvXmodemSend:
			Err = TTLXmodemSend(); break;
		case RsvYesNoBox:
			Err = TTLYesNoBox(); break;
		case RsvZmodemRecv:
			Err = TTLCommCmd(CmdZmodemRecv,IdTTLWaitCmndResult); break;
		case RsvZmodemSend:
			Err = TTLZmodemSend(); break;
		case RsvYmodemRecv:
			Err = TTLCommCmd(CmdYmodemRecv,IdTTLWaitCmndResult); break;
		case RsvYmodemSend:
			Err = TTLYmodemSend(); break;
		default:
			Err = ErrSyntax;
		}
	else if (GetIdentifier(Cmnd)) {
		if (GetIndex(&Index, &Err)) {
			WithIndex = TRUE;
		}
		else {
			WithIndex = FALSE;
		}

		if (!Err && GetFirstChar() == '=') {
			StrConst = GetString(Str,&Err);
			if (StrConst)
				ValType = TypString;
			else
				if (! GetExpression(&ValType,&Val,&Err))
					Err = ErrSyntax;

			if (!Err) {
				if (CheckVar(Cmnd,&VarType,&VarId)) {
					if (WithIndex) {
						switch (VarType) {
						case TypIntArray:
							VarId = GetIntVarFromArray(VarId, Index, &Err);
							if (!Err) VarType = TypInteger;
							break;
						case TypStrArray:
							VarId = GetStrVarFromArray(VarId, Index, &Err);
							if (!Err) VarType = TypString;
							break;
						default:
							Err = ErrSyntax;
						}
					}
					if (Err) return Err;
					if (VarType==ValType) {
						switch (ValType) {
						case TypInteger: SetIntVal(VarId,Val); break;
						case TypString:
							if (StrConst)
								SetStrVal(VarId,Str);
							else
							// StrVarPtr の返り値が TStrVal のポインタであることを期待してサイズを固定
							// (2007.6.23 maya)
								strncpy_s(StrVarPtr(VarId),MaxStrLen,StrVarPtr((TVarId)Val),_TRUNCATE);
						break;
						default:
							Err = ErrSyntax;
						}
					}
					else {
						Err = ErrTypeMismatch;
					}
				}
				else if (WithIndex) {
					Err = ErrSyntax;
				}
				else {
					switch (ValType) {
					case TypInteger: E = NewIntVar(Cmnd,Val); break;
					case TypString:
						if (StrConst)
							E = NewStrVar(Cmnd,Str);
						else
							E = NewStrVar(Cmnd,StrVarPtr((TVarId)Val));
						break;
					default: 
						E = FALSE;
					}
					if (! E) Err = ErrTooManyVar;
				}
				if (!Err && (GetFirstChar()!=0))
					Err = ErrSyntax;
			}
		}
		else Err = ErrNotSupported;
	}
	else
		Err = ErrSyntax;

	return Err;
}

void Exec()
{
	WORD Err;

	// ParseAgain is set by 'ExecCmnd'
	if (! ParseAgain &&
	    ! GetNewLine())
	{
		TTLStatus = IdTTLEnd;
		return;
	}
	ParseAgain = FALSE;

	LockVar();
	Err = ExecCmnd();
	if (Err>0) DispErr(Err);
	UnlockVar();
}

// 正規表現でマッチした文字列を記録する
// (2005.10.7 yutaka)
void SetMatchStr(PCHAR Str)
{
	WORD VarType;
	TVarId VarId;

	if (CheckVar("matchstr",&VarType,&VarId) &&
	    (VarType==TypString))
		SetStrVal(VarId,Str);
}

// 正規表現でグループマッチした文字列を記録する
// (2005.10.15 yutaka)
void SetGroupMatchStr(int no, PCHAR Str)
{
	WORD VarType;
	TVarId VarId;
	char buf[128];
	char *p;

	if (Str == NULL)
		p = "";
	else
		p = Str;

	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "groupmatchstr%d", no);

	if (CheckVar(buf,&VarType,&VarId) &&
	    (VarType==TypString))
		SetStrVal(VarId,p);
}

void SetInputStr(PCHAR Str)
{
	WORD VarType;
	TVarId VarId;

	if (CheckVar("inputstr",&VarType,&VarId) &&
	    (VarType==TypString))
		SetStrVal(VarId,Str);
}

void SetResult(int ResultCode)
{
  WORD VarType;
  TVarId VarId;

	if (CheckVar("result",&VarType,&VarId) &&
	    (VarType==TypInteger))
		SetIntVal(VarId,ResultCode);
}

BOOL CheckTimeout()
{
	BOOL ret;
	DWORD TimeUp = (TimeStart+TimeLimit);

	if (TimeUp > TimeStart) {
		ret = (GetTickCount() > TimeUp);
	}
	else { // for DWORD overflow (49.7 days)
		DWORD TimeTmp = GetTickCount();
		ret = (TimeUp < TimeTmp && TimeTmp >= TimeStart);
	}
	return ret;
}

BOOL TestWakeup(int Wakeup)
{
	return ((Wakeup & WakeupCondition) != 0);
}

void SetWakeup(int Wakeup)
{
	WakeupCondition = Wakeup;
}
