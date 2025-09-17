/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005- TeraTerm Project
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

// TTMACRO.EXE, DDE routines

#include "teraterm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ddeml.h>
#include <stdint.h>
#include "ttmdlg.h"
#include "ttmparse.h"
#include "ttmmsg.h"
#include "codeconv.h"
#include "asprintf.h"
#include "ddelib.h"

#include "ttmdde.h"

#include "ttl.h"

#include "wait4all.h"

#include "ttmonig.h"

BOOL Linked = FALSE;
WORD ComReady = 0;
int OutLen;

  // for "WaitRecv" command
TStrVal Wait2Str;
BOOL Wait2Found;

OnigOptionType RegexOpt = ONIG_OPTION_NONE;
OnigEncoding RegexEnc = ONIG_ENCODING_UTF8;
OnigSyntaxType *RegexSyntax = ONIG_SYNTAX_RUBY;

#define ServiceName "TERATERM"
#define ItemName "DATA"
#define ItemName2 "PARAM"

#define OutBufSize 512
// リングバッファを 4KB から 16KB へ拡張した (2006.10.15 yutaka)
// Tera Term本体へ SendSync を送るときの上限チェックを 8KB にした。
// Tera Term本体側のDDEバッファは 1KB なので、それよりも大きな値にすることで、
// Tera Termとマクロがスラッシング状態になることを防ぐ。
#define RingBufSize (4096*4)
#define RCountLimit (RingBufSize/2)

static HWND HMainWin = NULL;
static DWORD Inst = 0;
static HCONV ConvH = 0;

static HSZ Service = 0;
static HSZ Topic = 0;
static HSZ Item = 0;
static HSZ Item2 = 0;

// sync mode
static BOOL SyncMode = FALSE;
static BOOL SyncSent;

static BOOL QuoteFlag;
static char OutBuf[OutBufSize];

static char RingBuf[RingBufSize];
static int RBufStart = 0;
static int RBufPtr = 0;
static int RBufCount = 0;

  // for 'Wait' command
static PCHAR PWaitStr[10];
static int WaitStrLen[10];
static int WaitCount[10];
  // for "WaitRecv" command
static TStrVal Wait2SubStr;
static int Wait2Count, Wait2Len;
static int Wait2SubLen, Wait2SubPos;
 //  waitln & recvln
static TStrVal RecvLnBuff;
static int RecvLnPtr = 0;
static BYTE RecvLnLast = 0;
// for "WaitN" command
static int WaitNLen = 0;
static BOOL RecvLnClear = TRUE;

// regex action flag
enum regex_type RegexActionType;

// for wait4all
BOOL Wait4allGotIndex = FALSE;
int Wait4allFoundNum = 0;

// ring buffer
static void Put1Byte(BYTE b)
{
	// 従来のリングバッファへ書き込むと同時に、wait4all用共有メモリへも書き出す。(2009.3.12 yutaka)
	if (is_wait4all_enabled()) {
		put_macro_1byte(b);
		return;
	}

	RingBuf[RBufPtr] = b;
	RBufPtr++;
	if (RBufPtr>=RingBufSize) {
		RBufPtr = RBufPtr-RingBufSize;
	}
	if (RBufCount>=RingBufSize) {
		RBufCount = RingBufSize;
		RBufStart = RBufPtr;
	}
	else {
		RBufCount++;
	}
}

static BOOL Read1Byte(LPBYTE b)
{
	if (is_wait4all_enabled()) {
		return read_macro_1byte(macro_shmem_index, b);
	}

	if (RBufCount<=0) {
		return FALSE;
	}

	*b = RingBuf[RBufStart];
	RBufStart++;
	if (RBufStart>=RingBufSize) {
		RBufStart = RBufStart-RingBufSize;
	}
	RBufCount--;
	if (QuoteFlag) {
		*b= *b-1;
		QuoteFlag = FALSE;
	}
	else {
		QuoteFlag = (*b==0x01);
	}

	return (! QuoteFlag);
}

HDDEDATA AcceptData(HDDEDATA ItemHSz, HDDEDATA Data)
{
	HDDEDATA DH;
	PCHAR DPtr;
	DWORD DSize;
	int i;

	if (DdeCmpStringHandles((HSZ)ItemHSz, Item) != 0) {
		return 0;
	}

	DH = Data;
	if (DH==0) {
		DH = DdeClientTransaction(NULL,0,ConvH,Item,CF_OEMTEXT,XTYP_REQUEST,1000,NULL);
	}

	if (DH==0) {
		return (HDDEDATA)DDE_FACK;
	}
	DPtr = DdeAccessData(DH,&DSize);
	if (DPtr==NULL) {
		return (HDDEDATA)DDE_FACK;
	}
	DSize = strlen(DPtr);
	for (i=0; i<(signed)DSize; i++) {
		Put1Byte((BYTE)DPtr[i]);
	}
	DdeUnaccessData(DH);
	DdeFreeDataHandle(DH);
	SyncSent = FALSE;

	return (HDDEDATA)DDE_FACK;
}

// CallBack Procedure for DDEML
static HDDEDATA CALLBACK DdeCallbackProc(UINT CallType, UINT Fmt, HCONV Conv,
  HSZ hsz1, HSZ hsz2, HDDEDATA Data, ULONG_PTR Data1, ULONG_PTR Data2)
{
	if (Inst==0) {
		return 0;
	}
	switch (CallType) {
		case XTYP_REGISTER:
			break;
		case XTYP_UNREGISTER:
			break;
		case XTYP_XACT_COMPLETE:
			break;
		case XTYP_ADVDATA:
			return (AcceptData((HDDEDATA)hsz2,Data));
		case XTYP_DISCONNECT:
			ConvH = 0;
			Linked = FALSE;
			PostMessage(HMainWin,WM_USER_DDEEND,0,0);
			break;
	}
	return 0;
}

void Byte2HexStr(BYTE b, LPSTR HexStr)
{
	if (b<0xa0) {
		HexStr[0] = 0x30 + (b >> 4);
	}
	else {
		HexStr[0] = 0x37 + (b >> 4);
	}
	if ((b & 0x0f) < 0x0a) {
		HexStr[1] = 0x30 + (b & 0x0f);
	}
	else {
		HexStr[1] = 0x37 + (b & 0x0f);
	}
	HexStr[2] = 0;
}

void Word2HexStr(WORD w, LPSTR HexStr)
{
	Byte2HexStr(HIBYTE(w),HexStr);
	Byte2HexStr(LOBYTE(w),&HexStr[2]);
}

BOOL InitDDE(HWND HWin)
{
	int i;
	WORD w;
	char Cmd[10];

	HMainWin = HWin;
	Linked = FALSE;
	SyncMode = FALSE;
	OutLen = 0;
	RBufStart = 0;
	RBufPtr = 0;
	RBufCount = 0;
	QuoteFlag = FALSE;
	for (i = 0 ; i<=9 ; i++) {
		PWaitStr[i] = NULL;
		WaitStrLen[i] = 0;
		WaitCount[i] = 0;
	}

	if (DdeInitialize(&Inst, DdeCallbackProc,
	                  APPCMD_CLIENTONLY |
	                  CBF_SKIP_REGISTRATIONS |
	                  CBF_SKIP_UNREGISTRATIONS,0)
	    != DMLERR_NO_ERROR) {
		return FALSE;
	}

	Service= DdeCreateStringHandle(Inst, ServiceName, CP_WINANSI);
	{
		char *TopicNameA = ToCharW(TopicName);
		Topic  = DdeCreateStringHandle(Inst, TopicNameA, CP_WINANSI);
		free(TopicNameA);
	}
	Item   = DdeCreateStringHandle(Inst, ItemName, CP_WINANSI);
	Item2  = DdeCreateStringHandle(Inst, ItemName2, CP_WINANSI);
	if ((Service==0) || (Topic==0) ||
	    (Item==0) || (Item2==0)) {
		return FALSE;
	}

	ConvH = DdeConnect(Inst, Service, Topic, NULL);
	if (ConvH == 0) {
		return FALSE;
	}
	Linked = TRUE;

	Cmd[0] = CmdSetHWnd;
	w = HIWORD(HWin);
	Word2HexStr(w,&(Cmd[1]));
	w = LOWORD(HWin);
	Word2HexStr(w,&(Cmd[5]));

	DdeClientTransaction(Cmd,strlen(Cmd)+1,ConvH,0,
	                     CF_OEMTEXT,XTYP_EXECUTE,1000,NULL);

	DdeClientTransaction(NULL,0,ConvH,Item,
	                     CF_OEMTEXT,XTYP_ADVSTART,1000,NULL);

	return TRUE;
}

void EndDDE()
{
	DWORD Temp;

	Linked = FALSE;
	SyncMode = FALSE;

	ConvH = 0;
	TopicName[0] = 0;

	Temp = Inst;
	if (Inst != 0) {
		Inst = 0;
		if (Service != 0) {
			DdeFreeStringHandle(Temp, Service);
		}
		Service = 0;
		if (Topic != 0) {
			DdeFreeStringHandle(Temp, Topic);
		}
		Topic = 0;
		if (Item != 0) {
			DdeFreeStringHandle(Temp, Item);
		}
		Item = 0;
		if (Item2 != 0) {
			DdeFreeStringHandle(Temp, Item2);
		}
		Item2 = 0;

		DdeUninitialize(Temp);  // Ignore the return value
	}
}

void DDEOut1Byte(BYTE B)
{
	if (OutLen < OutBufSize-1) {
		OutBuf[OutLen] = B;
		OutLen++;
	}
}

void DDEOut(const char *B)
{
	while (*B) {
		if (OutLen >= OutBufSize - 1)
			break;
		OutBuf[OutLen++] = *B;
		B++;
	}
}

// 有効時 XTYP_EXECUTE で送信する
// 無効時 XTYP_POKE で送信する (変更前と同じ)
#define	USE_EXECUTE	1

/**
 *	TeraTermへバイナリを送信
 */
#if USE_EXECUTE
static void SendBinaryTTExecute(const uint8_t *ptr, size_t len)
{
	size_t encoded_len;
	uint8_t *encoded_bin = EncodeDDEBinary(ptr, len, &encoded_len);
	DdeClientTransaction(encoded_bin, encoded_len, ConvH, 0, 0, XTYP_EXECUTE, 1000, NULL);
	free(encoded_bin);
}
#else
static void SendBinaryTTPoke(const uint8_t *ptr, size_t len)
{
	size_t encoded_len;
	uint8_t *encoded_bin = EncodeDDEBinary(ptr, len, &encoded_len);
	HDDEDATA hd;
	int i;
	const int retry_count = 10;

	for (i = 0 ; i < retry_count ; i++) {
		// サーバ(Tera Term)へコマンド列を送る。成功すると非0が返る。
		hd = DdeClientTransaction(encoded_bin,len+1,ConvH,Item,CF_OEMTEXT,XTYP_POKE,1000,NULL);
		if (hd != 0) {
			break;
		} else {
			UINT ret = DdeGetLastError(Inst);
			if (ret == DMLERR_BUSY) { // ビジーの場合リトライを行う
				Sleep(100);        // スリープしてサーバへCPUを回す。100の値に根拠はない。
			} else {
				// よく分からないエラーはそのまま捨てる。
				// DDEサーバ(Tera Term)が AutoWinClose=off で切断した場合、DMLERR_NOTPROCESSED が
				// 返ってくる。ここで破棄しなければ、延々とリトライすることになり、CPUを食い潰す。
				// (2009.11.22 yutaka)
				break;
			}
		}
	}

	free(encoded_bin);
}
#endif

static void DDESendRaw(uint8_t command, const char *ptr, size_t len)
{
	uint8_t *buf = (uint8_t *)malloc(1 + 4 + len);
	if (buf == NULL) {
		return;
	}
	buf[0] = command;
	buf[1] = (uint8_t)((len & 0xff000000 ) >> (8*3));
	buf[2] = (uint8_t)((len & 0x00ff0000 ) >> (8*2));
	buf[3] = (uint8_t)((len & 0x0000ff00 ) >> (8*1));
	buf[4] = (uint8_t)((len & 0x000000ff ));
	memcpy(buf + 1 + 4, ptr, len);
#if USE_EXECUTE
	SendBinaryTTExecute(buf, 1 + 4 + len);
#else
	SendBinaryTTPoke(buf, 1 + 4 + len);
#endif
	free(buf);
}

void DDESend()
{
	OutBuf[OutLen] = 0;  // DDEデータの終端は null で終わることをサーバが期待している。
	DDESendRaw(CmdSendCompatString, OutBuf, OutLen + 1);
	OutLen = 0;
}

void DDESendStringU8(const char *strU8)
{
	OutBuf[OutLen] = 0;
	strU8 = OutBuf;
	size_t len = strlen(strU8) + 1;		// 終端 '\0' も送信する
	DDESendRaw(CmdSendUTF8String, strU8, len);
	OutLen = 0;
}

void DDESendBinary(const void *ptr, size_t len)
{
	OutBuf[OutLen] = 0;  // 1byte長く送る ?
	ptr = OutBuf;
	len = OutLen + 1;
	DDESendRaw(CmdSendBinary, ptr, len);
	OutLen = 0;
}

void ClearRecvLnBuff()
{
	RecvLnPtr = 0;
	RecvLnLast = 0;
}

void PutRecvLnBuff(BYTE b)
{
	if (RecvLnLast==0x0a && RecvLnClear) {
		ClearRecvLnBuff();
	}
	if (RecvLnPtr < sizeof(RecvLnBuff)-1) {
		RecvLnBuff[RecvLnPtr++] = b;
	}
	RecvLnLast = b;
}

PCHAR GetRecvLnBuff()
{
	if ((RecvLnPtr>0) &&
	    RecvLnBuff[RecvLnPtr-1]==0x0a) {
		RecvLnPtr--;
		if ((RecvLnPtr>0) &&
		    RecvLnBuff[RecvLnPtr-1]==0x0d) {
			RecvLnPtr--;
		}
	}
	RecvLnBuff[RecvLnPtr] = 0;
	ClearRecvLnBuff();
	return RecvLnBuff;
}

void FlushRecv()
{
	ClearRecvLnBuff();
	RBufStart = 0;
	RBufPtr = 0;
	RBufCount = 0;
}

void ClearWait()
{
	int i;

	for (i = 0 ; i <= 9 ; i++) {
		if (PWaitStr[i]!=NULL) {
			free(PWaitStr[i]);
		}
		PWaitStr[i] = NULL;
		WaitStrLen[i] = 0;
		WaitCount[i] = 0;
	}

	RegexActionType = REGEX_NONE; // regex disabled
}

void SetWait(int Index, const char *Str)
{
	if (PWaitStr[Index-1])
		free(PWaitStr[Index-1]);

	PWaitStr[Index-1] = _strdup(Str);

	if (PWaitStr[Index-1])
		WaitStrLen[Index-1] = strlen(Str);
	else
		WaitStrLen[Index-1] = 0;

	WaitCount[Index-1] = 0;
}

void SetRecvLnClear(BOOL v)
{
	RecvLnClear = v;
}

void ClearWaitN()
{
	WaitNLen = 0;
	SetRecvLnClear(TRUE);
}

void SetWaitN(int Len)
{
	WaitNLen = Len;
	SetRecvLnClear(FALSE);
}

int CmpWait(int Index, PCHAR Str)
{
	if (PWaitStr[Index-1]!=NULL) {
		return strcmp(PWaitStr[Index-1],Str);
	}
	return 1;
}

void SetWait2(PCHAR Str, int Len, int Pos)
{
	strncpy_s(Wait2SubStr, sizeof(Wait2SubStr),Str, _TRUNCATE);
	Wait2SubLen = strlen(Wait2SubStr);

	if (Len<1) {
		Wait2Len = 0;
	}
	else if (Len>MaxStrLen-1) {
		Wait2Len = MaxStrLen-1;
	}
	else {
		Wait2Len = Len;
	}

	if (Wait2Len<Wait2SubLen) {
		Wait2Len = Wait2SubLen;
	}

	if (Pos<1) {
		Wait2SubPos = 1;
	}
	else if (Pos>Wait2Len-Wait2SubLen+1) {
		Wait2SubPos = Wait2Len-Wait2SubLen+1;
	}
	else {
		Wait2SubPos = Pos;
	}

	Wait2Count = 0;
	Wait2Str[0] = 0;
	Wait2Found = (Wait2SubStr[0]==0);
}


// 正規表現によるパターンマッチを行う（Oniguruma使用）
//
// return 正: マッチした位置（1オリジン）
//         0: マッチしなかった
int FindRegexStringOne(char *regex, int regex_len, char *target, int target_len)
{
	int r;
	unsigned char *start, *range, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	UChar* pattern = (UChar* )regex;
	UChar* str     = (UChar* )target;
	int matched = 0;
	char ch;
	int mstart, mend;


	r = onig_new(&reg, pattern, pattern + regex_len,
		RegexOpt, RegexEnc, RegexSyntax, &einfo);
	if (r != ONIG_NORMAL) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str(s, r, &einfo);
		fprintf(stderr, "ERROR: %s\n", s);
		return -1;
	}

	region = onig_region_new();

	end   = str + target_len;
	start = str;
	range = end;
	r = onig_search(reg, str, end, start, range, region, ONIG_OPTION_NONE);
	if (r >= 0) {
		int i;

		// 前回の結果をクリアする (2008.3.28 maya)
		for (i = 1; i <= 9; i++) {
			LockVar();
			SetGroupMatchStr(i, "");
			UnlockVar();
		}

		for (i = 0; i < region->num_regs; i++) {
			mstart = region->beg[i];
			mend = region->end[i];

			// マッチパターン全体を matchstr へ格納する
			if (i == 0) {
				ch = target[mend];
				target[mend] = 0; // null terminate
				LockVar();
				SetMatchStr(target + mstart);
				UnlockVar();
				target[mend] = ch;  // restore

			} else { // 残りのマッチパターンは groupmatchstr[1-9] へ格納する
				ch = target[mend];
				target[mend] = 0; // null terminate
				LockVar();
				SetGroupMatchStr(i, target + mstart);
				UnlockVar();
				target[mend] = ch;  // restore

			}

		}

		matched = (r + 1);
	}
	else if (r == ONIG_MISMATCH) {
		fprintf(stderr, "search fail\n");
	}
	else { /* error */
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str(s, r);
		fprintf(stderr, "ERROR: %s\n", s);
		return -1;
	}

	onig_region_free(region, 1 /* 1:free self, 0:free contents only */);
	onig_free(reg);
	onig_end();

	return (matched);
}

// 正規表現によるパターンマッチを行う
int FindRegexString(void)
{
	int i;

	if (RegexActionType == REGEX_NONE)
		return 0;  // not match

	if (RecvLnPtr == 0)
		return 0;  // not match

	for (i = 0 ; i < 10 ; i++) {
		if (PWaitStr[i] && FindRegexStringOne(PWaitStr[i], WaitStrLen[i], RecvLnBuff, RecvLnPtr) > 0) { // matched
			// マッチした行を inputstr へ格納する
			LockVar();
			SetInputStr(GetRecvLnBuff());  // ここでバッファがクリアされる
			UnlockVar();

			return i+1;
		}
	}

	return 0;
}


// 'wait':
// ttmacro process sleeps to wait specified word(s).
//
// 'waitregex':
// ttmacro process sleeps to wait specified word(s) with regular expression.
//
// add 'waitregex' command (2005.10.5 yutaka)
int Wait()
{
	BYTE b;
	int i, j, Found, ret;
	PCHAR Str;

	Found = 0;
	while ((Found==0) && Read1Byte(&b))
	{
		if (b == 0x0a) { // 改行が来たら、バッファをクリアするのでその前にパターンマッチを行う。(waitregex command)
			ret = FindRegexString();
			if (ret > 0) {
				Found = ret;
				break;
			}
		}

		PutRecvLnBuff(b);

		if (RegexActionType == REGEX_NONE) { // 正規表現なしの場合は1バイトずつ検索する(wait command)
			for (i = 9 ; i >= 0 ; i--) {
				Str = PWaitStr[i];
				if (Str!=NULL) {
					if ((BYTE)Str[WaitCount[i]]==b) {
						WaitCount[i]++;
					}
					else if (WaitCount[i]>0) {
						for (j=WaitCount[i]-1; j>=0; j--) {
							if ((BYTE)Str[j] == b) {
								if (j== 0 || strncmp(Str, Str+(WaitCount[i]-j), j)==0) {
									break;
								}
							}
						}
						if (j >= 0) {
							WaitCount[i] = j+1;
						}
						else {
							WaitCount[i] = 0;
						}
					}
					if (WaitCount[i]==WaitStrLen[i]) {
						Found = i+1;
					}
				}
			}
		}
	}

	// 改行なしで文字列が流れてくる場合にもパターンマッチを試みる
	if (Found == 0) {
		ret = FindRegexString();
		if (ret > 0) {
			Found = ret;
		}
	}

	if (Found>0) ClearWait();
	SendSync();

	return Found;
}

BOOL Wait2()
{
	BYTE b;

	while (! (Wait2Found && (Wait2Count==Wait2Len)) && Read1Byte(&b)) {
		if (Wait2Count>=Wait2Len) {
			memmove(Wait2Str,&(Wait2Str[1]),Wait2Len-1);
			Wait2Str[Wait2Len-1] = b;
			Wait2Count = Wait2Len;
		}
		else {
			Wait2Str[Wait2Count] = b;
			Wait2Count++;
		}
		Wait2Str[Wait2Count] = 0;
		if ((! Wait2Found) && (Wait2Count>=Wait2SubPos+Wait2SubLen-1)) {
			Wait2Found = (strncmp(&(Wait2Str[Wait2SubPos-1]), Wait2SubStr,Wait2SubLen)==0);
		}
	}
	SendSync();

	return (Wait2Found && (Wait2Count==Wait2Len));
}

// add 'waitn'  (2009.1.26 maya)
//   patch from p3g4asus
BOOL WaitN()
{
	BYTE b;

	while (RecvLnPtr < WaitNLen && Read1Byte(&b)) {
		PutRecvLnBuff(b);
	}
	SendSync();

	if (RecvLnPtr>=WaitNLen) {
		return TRUE;
	}

	return FALSE;
}


static int Wait4allOneBuffer(int index)
{
	BYTE b;
	int i, Found;
	PCHAR Str;

	Found = 0;
	while ((Found==0) && read_macro_1byte(index, &b))
	{
		for (i = 9 ; i >= 0 ; i--) {
			Str = PWaitStr[i];
			if (Str!=NULL) {
				if ((BYTE)Str[WaitCount[i]]==b) {
					WaitCount[i]++;
				}
				else if (WaitCount[i]>0) {
					WaitCount[i] = 0;
					if ((BYTE)Str[0]==b) {
						WaitCount[i] = 1;
					}
				}
				if (WaitCount[i]==WaitStrLen[i]) {
					Found = i+1;
				}
			}
		}
	}

//	if (Found>0) ClearWait();
	SendSync();

	return Found;
}

//
// 全マクロのバッファをsnoopし、期待したキーワードがすべて揃うまでウェイトする。
//
//  1: 揃った
//  0: 揃っていない
//
int Wait4all()
{
	static int num, index[MAXNWIN], found[MAXNWIN];
	int i, ret;
	int curnum;

	if (Wait4allGotIndex == FALSE) {
		num = 0;
		memset(index, 0, sizeof(index));
		memset(found, 0, sizeof(found));
		get_macro_active_info(&num, index);
		Wait4allGotIndex = TRUE;
	}

	for (i = 0 ; i < num ; i++) {
		ret = Wait4allOneBuffer(index[i]);
		if (ret && found[i] == 0) {
			Wait4allFoundNum++;
			// 同じ端末で、重複してマッチした場合は、2重にカウントしないようにする。
			found[i] = 1;
		}
	}

	// 今現在、アクティブなttpmacroの数を取得する。
	// wait4all実行中に、プロセスが増減することがあるため。
	curnum = get_macro_active_num();
	if (curnum < num)
		num = curnum;

	if (Wait4allFoundNum >= num) {
		ClearWait();
		return 1; // 全部そろった

	} else {

		return 0;
	}
}

static void SetFileCommon(char cmd, const char *filenameU8)
{
	char *cmd_buf;
	int len = asprintf(&cmd_buf, "%c%s", cmd, filenameU8);
	if (len > 0) {
		DdeClientTransaction(cmd_buf, len, ConvH, 0, CF_OEMTEXT, XTYP_EXECUTE, 1000, NULL);
		free(cmd_buf);
	}
}

/**
 *	ファイル名(文字列)を送信する
 *
 *	@param	filenameU8	文字列(UTF-8)
 */
void SetFile(const char *filenameU8)
{
	SetFileCommon(CmdSetFile, filenameU8);
}

void SetSecondFile(const char *filenameU8)
{
	SetFileCommon(CmdSetSecondFile, filenameU8);
}

void SetBinary(int BinFlag)
{
	char Cmd[3];

	Cmd[0] = CmdSetBinary;
	Cmd[1] = 0x30 + (BinFlag & 1);
	Cmd[2] = 0;
	DdeClientTransaction(Cmd,strlen(Cmd)+1,ConvH,0,CF_OEMTEXT,XTYP_EXECUTE,1000,NULL);
}

void SetDebug(int DebugFlag)
{
  char Cmd[3];

  Cmd[0] = CmdSetDebug;
  Cmd[1] = DebugFlag+'0';
  Cmd[2] = 0;
  DdeClientTransaction(Cmd,strlen(Cmd)+1,ConvH,0,CF_OEMTEXT,XTYP_EXECUTE,1000,NULL);
}

void SetLogOption(int *LogFlags)
{
	char Cmd[LogOptMax+2];

	Cmd[0]                   = CmdSetLogOpt;
	Cmd[LogOptBinary]        = 0x30 + (LogFlags[LogOptBinary] != 0);
	Cmd[LogOptAppend]        = 0x30 + (LogFlags[LogOptAppend] != 0);
	Cmd[LogOptPlainText]     = 0x30 + (LogFlags[LogOptPlainText] != 0);
	Cmd[LogOptTimestamp]     = 0x30 + (LogFlags[LogOptTimestamp] != 0);
	Cmd[LogOptHideDialog]    = 0x30 + (LogFlags[LogOptHideDialog] != 0);
	Cmd[LogOptIncScrBuff]    = 0x30 + (LogFlags[LogOptIncScrBuff] != 0);
	Cmd[LogOptTimestampType] = 0x30 + LogFlags[LogOptTimestampType];
	Cmd[LogOptMax+1]         = 0;
	DdeClientTransaction(Cmd, sizeof(Cmd), ConvH, 0, CF_OEMTEXT, XTYP_EXECUTE, 1000, NULL);
}

void SetXOption(int XOption)
{
	char Cmd[3];

	if ((XOption<1) || (XOption>3)) {
		XOption = 1;
	}
	Cmd[0] = CmdSetXmodemOpt;
	Cmd[1] = 0x30 + XOption;
	Cmd[2] = 0;
	DdeClientTransaction(Cmd,strlen(Cmd)+1,ConvH,0,CF_OEMTEXT,XTYP_EXECUTE,1000,NULL);
}

void SendSync()
{
	char Cmd[10];
	int i;

	if (! Linked) {
		return;
	}
	if (! SyncMode) {
		return;
	}
	if (SyncSent) {
		return;
	}
	if (RBufCount>=RCountLimit) {
		return;
	}

	// notify free buffer space to Tera Term
	i = RingBufSize - RBufCount;
	if (i<1) {
		i = 1;
	}
	if (i>RingBufSize) {
		i = RingBufSize;
	}
	SyncSent = TRUE;

	_snprintf_s(Cmd,sizeof(Cmd),_TRUNCATE,"%c%u",CmdSetSync,i);
	DdeClientTransaction(Cmd,strlen(Cmd)+1,ConvH,0,CF_OEMTEXT,XTYP_EXECUTE,1000,NULL);
}

void SetSync(BOOL OnFlag)
{
	char Cmd[10];
	int i;

	if (! Linked) {
		return;
	}
	if (SyncMode==OnFlag) {
		return;
	}
	SyncMode = OnFlag;

	if (OnFlag) { // sync mode on
		// notify free buffer space to Tera Term
		i = RingBufSize - RBufCount;
		if (i<1) {
			i = 1;
		}
		if (i>RingBufSize) {
			i = RingBufSize;
		}
		SyncSent = TRUE;
	}
	else { // sync mode off
		i = 0;
	}

	_snprintf_s(Cmd,sizeof(Cmd),_TRUNCATE,"%c%u",CmdSetSync,i);
	DdeClientTransaction(Cmd,strlen(Cmd)+1,ConvH,0,CF_OEMTEXT,XTYP_EXECUTE,1000,NULL);
}

WORD SendCmnd(char OpId, int WaitFlag)
/* WaitFlag  should be 0 or IdTTLWaitCmndEnd or IdTTLWaitCmndResult. */
{
	char Cmd[10];

	if (! Linked) {
		return ErrLinkFirst;
	}

	if (WaitFlag!=0) {
		TTLStatus = WaitFlag;
	}
	// 1byte+NULだと、なぜかVisual Studioでデバッグできなくなる（ヒープエラー発生）ので、
	// 2byte+NULで送る。(2009.2.3 yutaka)
	Cmd[0] = OpId;
	Cmd[1] = OpId;
	Cmd[2] = '\0';
	if (DdeClientTransaction(Cmd,strlen(Cmd)+1,ConvH,0,CF_OEMTEXT,XTYP_EXECUTE,5000,NULL)==0) {
		TTLStatus = IdTTLRun;
	}

	return 0;
}

WORD GetTTParam(char OpId, PCHAR Param, int destlen)
{
	HDDEDATA Data;
	PCHAR DataPtr;

	if (! Linked) {
		return ErrLinkFirst;
	}

	SendCmnd(OpId,0);
	Data = DdeClientTransaction(NULL,0,ConvH,Item2,CF_OEMTEXT,XTYP_REQUEST,5000,NULL);
	if (Data == 0) {
		// トランザクションを開始できないときはエラーとする。(2016.10.22 yutaka)
		return 0;
	}
	DataPtr = (PCHAR)DdeAccessData(Data,NULL);
	if (DataPtr != NULL) {
		strncpy_s(Param,destlen,DataPtr,_TRUNCATE);
		DdeUnaccessData(Data);
	}
	DdeFreeDataHandle(Data);

	return 0;
}
