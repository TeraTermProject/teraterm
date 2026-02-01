/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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

/* TERATERM.EXE, DDE routines */
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <ddeml.h>
#include <stdint.h>
#include <assert.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "teraterm.h"
#include "tttypes.h"
#include "tttypes_key.h"
#include "ttwinman.h"
#include "ttsetup.h"
#include "telnet.h"
#include "ttlib.h"
#include "keyboard.h"
#include "ttdde.h"
#include "ttddecmnd.h"
#include "commlib.h"
#include "sendmem.h"
#include "codeconv.h"
#include "broadcast.h"
#include "filesys.h"
#include "scp.h"
#include "asprintf.h"
#include "vtterm.h"
#include "ttcstd.h"
#include "ddelib.h"
#include "vtdisp.h"

#define ServiceName "TERATERM"
#define ItemName "DATA"
#define ItemName2 "PARAM"

#define MaxStrLen (LONG)512

BOOL DDELog = FALSE;		// macro (DDE) が有効かどうかを示す
char TopicName[21] = "";
static HCONV ConvH = 0;
BOOL AdvFlag = FALSE;
BOOL CloseTT = FALSE;

static BOOL DdeCmnd = FALSE;

static DWORD Inst = 0;
static HSZ Service = 0;
static HSZ Topic = 0;
static HSZ Item = 0;
static HSZ Item2 = 0;
static HWND HWndDdeCli = NULL;

static BOOL StartupFlag = FALSE;

// for sync mode
static BOOL SyncMode = FALSE;
static BOOL SyncRecv;
static LONG SyncFreeSpace;

static char ParamFileName[MaxStrLen];
static WORD ParamBinaryFlag;
static WORD ParamXmodemOpt;
static char ParamSecondFileName[MaxStrLen];

static BOOL AutoLogClose = FALSE;

static char *cv_LogBuf;
static int cv_LogPtr;
static int cv_DStart;
static int cv_DCount;

/**
 *	マクロへの送信バッファへ1byteつみこむ
 *		バッファフルの時は古いものから捨てられる
 */
void DDEPut1(BYTE b)
{
	if (!DDELog) {
		return;
	}

	cv_LogBuf[cv_LogPtr] = b;
	cv_LogPtr++;
	if (cv_LogPtr >= InBuffSize)
		cv_LogPtr = cv_LogPtr - InBuffSize;

	if (cv_DCount >= InBuffSize)
	{
		cv_DCount = InBuffSize;
		cv_DStart = cv_LogPtr;
	}
	else
		cv_DCount++;
}

static BOOL DDECreateBuf(void)
{
	cv_LogBuf = (char *)malloc(InBuffSize);
	if (cv_LogBuf == NULL) {
		return FALSE;
	}
	cv_LogPtr = 0;
	cv_DStart = 0;
	cv_DCount = 0;
	return TRUE;
}

static void DDEFreeBuf(void)
{
	free(cv_LogBuf);
	cv_LogBuf = NULL;
}

static void BringupMacroWindow(BOOL flash_flag)
{
	HWND hwnd;
	DWORD pid_macro, pid;
	DWORD targetid;
	DWORD currentActiveThreadId;

	currentActiveThreadId = GetWindowThreadProcessId(GetForegroundWindow(), &pid);

	GetWindowThreadProcessId(HWndDdeCli, &pid_macro);
	hwnd = GetTopWindow(NULL);
	while (hwnd) {
		targetid = GetWindowThreadProcessId(hwnd, &pid);
		if (pid == pid_macro) {
			if (flash_flag)
				FlashWindow(hwnd, TRUE);

			SendMessage(hwnd, WM_USER_MSTATBRINGUP, (WPARAM)hwnd, 0);
		}
		hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
	}

	// マクロウィンドウ本体
	ShowWindow(HWndDdeCli, SW_NORMAL);
	SetForegroundWindow(HWndDdeCli);
	BringWindowToTop(HWndDdeCli);
}


static void GetClientHWnd(PCHAR HWndStr)
{
	int i;
	BYTE b;
	UINT_PTR HCli;

	HCli = 0;
	i = 0;
	b = HWndStr[0];
	while (b > 0)
	{
		if (b <= 0x39)
			HCli = (HCli << 4) + (b-0x30);
		else
			HCli = (HCli << 4) + (b-0x37);
		i++;
		b = HWndStr[i];
	}
	HWndDdeCli = (HWND)HCli;
}

void Byte2HexStr(BYTE b, LPSTR HexStr)
{
	if (b<0xa0)
		HexStr[0] = 0x30 + (b >> 4);
	else
		HexStr[0] = 0x37 + (b >> 4);
	if ((b & 0x0f) < 0x0a)
		HexStr[1] = 0x30 + (b & 0x0f);
	else
		HexStr[1] = 0x37 + (b & 0x0f);
	HexStr[2] = 0;
}

void SetTopic()
{
	WORD w;

	w = HIWORD(HVTWin);
	Byte2HexStr(HIBYTE(w),&(TopicName[0]));
	Byte2HexStr(LOBYTE(w),&(TopicName[2]));
	w = LOWORD(HVTWin);
	Byte2HexStr(HIBYTE(w),&(TopicName[4]));
	Byte2HexStr(LOBYTE(w),&(TopicName[6]));
}

HDDEDATA WildConnect(HSZ ServiceHsz, HSZ TopicHsz, UINT ClipFmt)
{
	HSZPAIR Pairs[2];
	BOOL Ok;

	Pairs[0].hszSvc  = Service;
	Pairs[0].hszTopic = Topic;
	Pairs[1].hszSvc = NULL;
	Pairs[1].hszTopic = NULL;

	Ok = FALSE;

	if ((ServiceHsz == 0) && (TopicHsz == 0))
		Ok = TRUE;
	else
		if ((TopicHsz == 0) &&
		    (DdeCmpStringHandles(Service, ServiceHsz) == 0))
			Ok = TRUE;
		else
			if ((DdeCmpStringHandles(Topic, TopicHsz) == 0) &&
			    (ServiceHsz == 0))
				Ok = TRUE;

	if (Ok)
		return DdeCreateDataHandle(Inst, (LPBYTE)(&Pairs), sizeof(Pairs),
		                           0, NULL, ClipFmt, 0);
	else
		return 0;
}

static BOOL DDEGet1(LPBYTE b)
{
	if (cv_DCount <= 0) return FALSE;
	*b = ((LPSTR)cv_LogBuf)[cv_DStart];
	cv_DStart++;
	if (cv_DStart>=InBuffSize)
		cv_DStart = cv_DStart-InBuffSize;
	cv_DCount--;
	return TRUE;
}

static LONG DDEGetDataLen()
{
	BYTE b;
	LONG Len;
	int Start, Count;

	Len = cv_DCount;
	Start = cv_DStart;
	Count = cv_DCount;
	while (Count>0)
	{
		b = ((LPSTR)cv_LogBuf)[Start];
		if ((b==0x00) || (b==0x01)) Len++;
		Start++;
		if (Start>=InBuffSize) Start = Start-InBuffSize;
		Count--;
	}

	return Len;
}

/**
 *	送信バッファに残っているデータ数取得
 */
int DDEGetCount(void)
{
	return cv_DCount;
}

static HDDEDATA AcceptRequest(HSZ ItemHSz)
{
	BYTE b;
	HDDEDATA DH;
	LPSTR DP;
	int i;
	LONG Len;

	if ((! DDELog) || (ConvH==0)) return 0;

	if (DdeCmpStringHandles(ItemHSz, Item2) == 0) { // item "PARAM"
		DH = DdeCreateDataHandle(Inst,ParamFileName,sizeof(ParamFileName),0,
		                         Item2,CF_OEMTEXT,0);
	}
	else if (DdeCmpStringHandles(ItemHSz, Item) == 0) // item "DATA"
	{
		Len = DDEGetDataLen();
		if ((SyncMode) &&
		    (SyncFreeSpace<Len))
			Len = SyncFreeSpace;

		DH = DdeCreateDataHandle(Inst,NULL,Len+2,0,
		                         Item,CF_OEMTEXT,0);
		DP = DdeAccessData(DH,NULL);
		if (DP != NULL)
		{
			i = 0;
			while (i < Len)
			{
				if (DDEGet1(&b)) {
					if ((b==0x00) || (b==0x01))
					{
						DP[i] = 0x01;
						DP[i+1] = b + 1;
						i = i + 2;
					}
					else {
						DP[i] = b;
						i++;
					}
				}
				else
					Len = 0;
			}
			DP[i] = 0;
			DdeUnaccessData(DH);
		}
	}
	else {
		return 0;
	}

	return DH;
}

/**
 *	バイナリデータかどうか判定する
 *
 *	@retval		TRUE	バイナリデータと解釈
 *
 *	データ内に 0x20 未満の値があった場合、バイナリデータと解釈する
 */
static BOOL IsBinaryData(const uint8_t *data, size_t len)
{
	size_t i;
	if (len == 0) {
		return FALSE;
	}
	len--;
	for (i = 0; i < len; i++) {
		uint8_t c = *data++;
		if (c != 0x0d && c != 0x0a && c < 0x20) {
			return TRUE;
		}
	}
	return FALSE;
}

static void SendData(const char *DataPtr, DWORD DataSize)
{
	// マクロコマンド "send" で送信すると、0x00から0xffまで自由に送信できる
	// DataPtr のデータはテキストではないこともあるので考慮が必要
	wchar_t *strW = NULL;
	BOOL binary_data = IsBinaryData(DataPtr, DataSize);
	if (binary_data == FALSE) {
		strW = ToWcharU8(DataPtr);
		if (strW == NULL) {
			// UTF-16LEへ変換できないなら、(多分)バイナリではなくテキスト
			binary_data = TRUE;
		}
		else {
			// UTF-16 -> UTF-8 に変換、再度比較
			char *strU8 = ToU8W(strW);
			if (strU8 == NULL) {
				binary_data = TRUE;
			}
			else {
				if (strcmp(strU8, DataPtr) != 0) {
					// 異なっていたらバイナリと判定
					binary_data = TRUE;
				}
				free(strU8);
			}
		}
	}

	if (binary_data) {
		uint8_t *p = (uint8_t *)malloc(DataSize);
		memcpy(p, DataPtr, DataSize);
		SendMem *sm = SendMemBinary(p, DataSize - 1);
		assert(sm != NULL);
		if (sm != NULL) {
			SendMemInitEcho(sm, ts.LocalEcho);
			SendMemInitDelay(sm, SENDMEM_DELAYTYPE_NO_DELAY, 0, 0);
			SendMemStart(sm);
		}
	} else {
		SendMem *sm = SendMemTextW(strW, 0);
		assert(sm != NULL);
		if (sm != NULL) {
			SendMemInitEcho(sm, ts.LocalEcho);
			SendMemInitDelay(sm, SENDMEM_DELAYTYPE_PER_LINE, 10, 0);
			SendMemStart(sm);
		}
	}
}

static void SendStringU8(const char *strU8)
{
	wchar_t *strW = ToWcharU8(strU8);
	if (strW == NULL) {
		return;
	}
	SendMem *sm = SendMemTextW(strW, 0);
	assert(sm != NULL);
	if (sm != NULL) {
		SendMemInitEcho(sm, ts.LocalEcho);
		SendMemInitDelay(sm, SENDMEM_DELAYTYPE_PER_LINE, 10, 0);
		SendMemStart(sm);
	}
}

static void SendBinary(const void *data_ptr, DWORD data_size)
{
	uint8_t *p = (uint8_t *)malloc(data_size);
	if (p == NULL) {
		return;
	}
	memcpy(p, data_ptr, data_size);
	SendMem *sm = SendMemBinary(p, data_size - 1);
	assert(sm != NULL);
	if (sm != NULL) {
		SendMemInitEcho(sm, ts.LocalEcho);
		SendMemInitDelay(sm, SENDMEM_DELAYTYPE_NO_DELAY, 0, 0);
		SendMemStart(sm);
	}
	// free(p); 送信完了後に自動で free() される
}

static HDDEDATA AcceptPoke(HSZ ItemHSz, UINT ClipFmt,
						   HDDEDATA Data)
{
	LPSTR DataPtr;
	DWORD DataSize;
	HDDEDATA result;

	// 連続してXTYP_POKEがクライアント（マクロ）から送られてくると、サーバ（本体）側がまだ
	// コマンドの貼り付けを行っていない場合、TalkStatusは IdTalkCB になので、DDE_FNOTPROCESSEDを
	// 返すことがある。DDE_FBUSYに変更。
	// (2006.11.6 yutaka)
	if (TalkStatus != IdTalkKeyb  && TalkStatus != IdTalkSendMem)
		return (HDDEDATA)DDE_FBUSY;

	if (ConvH==0) return DDE_FNOTPROCESSED;

	if ((ClipFmt!=CF_TEXT) && (ClipFmt!=CF_OEMTEXT)) return DDE_FNOTPROCESSED;

	if (DdeCmpStringHandles(ItemHSz, Item) != 0) return DDE_FNOTPROCESSED;

	result = (HDDEDATA)DDE_FNOTPROCESSED;
	DataPtr = DdeAccessData(Data,&DataSize);
	if (DataPtr != NULL) {
		SendData(DataPtr, DataSize);
		result = (HDDEDATA)DDE_FACK;
	}
	DdeUnaccessData(Data);
	return result;
}

static WORD HexStr2Word(PCHAR Str)
{
	int i;
	BYTE b;
	WORD w = 0;

	for (i=0; i<=3; i++)
	{
		b = Str[i];
		if (b <= 0x39)
			w = (w << 4) + (b-0x30);
		else
		w = (w << 4) + (b-0x37);
	}
	return w;
}

/**
 *	送信完了コールバック
 */
static void SendCallback(void *callback_data)
{
	(void)callback_data;
	EndDdeCmnd(0);
}

// 有効時 DdeAccessData() を使って受信データにアクセスする
// 無効時 DdeGetData() を使って受信データにアクセスする(従来と同じ)
//		DdeGetData()の場合、データ長上限(MaxStrLen)が存在する
#define	USE_ACCESSDATA	1

static HDDEDATA AcceptExecute(HSZ TopicHSz, HDDEDATA Data)
{
#if USE_ACCESSDATA
	char *Command;
#else
	char Command[MaxStrLen + 1];
#endif
	int i;
	BOOL r;
	DWORD len;
	HDDEDATA result = (HDDEDATA)DDE_FACK;

	if ((ConvH==0) ||
		(DdeCmpStringHandles(TopicHSz, Topic) != 0))
		return DDE_FNOTPROCESSED;

#if USE_ACCESSDATA
	Command = DdeAccessData(Data, &len);
	if (Command == NULL || len == 0) {
		return DDE_FNOTPROCESSED;
	}
#else
	memset(Command, 0, sizeof(Command));
	len = DdeGetData(Data,Command,sizeof(Command),0);
	if (len == 0) {
		return DDE_FNOTPROCESSED;
	}
#endif

	switch (Command[0]) {
	case CmdSetHWnd:
		GetClientHWnd(&Command[1]);
		if (cv.Ready)
			SetDdeComReady(1);
		break;
	case CmdSetFile:
		strncpy_s(ParamFileName, sizeof(ParamFileName),&(Command[1]), _TRUNCATE);
		break;
	case CmdSetBinary:
		ParamBinaryFlag = Command[1] & 1;
		break;
	case CmdSetLogOpt:
		ts.LogBinary        = Command[LogOptBinary] - 0x30;
		ts.Append           = Command[LogOptAppend] - 0x30;
		ts.LogTypePlainText = Command[LogOptPlainText] - 0x30;
		ts.LogTimestamp     = Command[LogOptTimestamp] - 0x30;
		ts.LogHideDialog    = Command[LogOptHideDialog] - 0x30;
		ts.LogAllBuffIncludedInFirst = Command[LogOptIncScrBuff] - 0x30;
		ts.LogTimestampType = Command[LogOptTimestampType] - 0x30;
		break;
	case CmdSetXmodemOpt:
		ParamXmodemOpt = Command[1] & 3;
		if (ParamXmodemOpt==0) ParamXmodemOpt = 1;
		break;
	case CmdSetSync:
		if (sscanf(&(Command[1]),"%lu",&SyncFreeSpace)!=1)
			SyncFreeSpace = 0;
		SyncMode = (SyncFreeSpace>0);
		SyncRecv = TRUE;
		break;
	case CmdBPlusRecv:
		if (BPStartReceive(TRUE, FALSE)) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	case CmdBPlusSend: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		r = BPStartSend(ParamFileNameW);
		free(ParamFileNameW);
		if (r) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	}
	case CmdChangeDir:
		free(ts.FileDirW);
		ts.FileDirW = ToWcharU8(ParamFileName);
		break;
	case CmdClearScreen:
		switch (ParamFileName[0]) {
		case '0':
			PostMessage(HVTWin,WM_USER_ACCELCOMMAND,IdCmdEditCLS,0);
			break;
		case '1':
			PostMessage(HVTWin,WM_USER_ACCELCOMMAND,IdCmdEditCLB,0);
			break;
		case '2':
			PostMessage(HTEKWin,WM_USER_ACCELCOMMAND,IdCmdEditCLS,0);
			break;
		}
		break;
	case CmdCloseWin:
		CloseTT = TRUE;
		break;
	case CmdConnect:
		if (cv.Open) {
			if (cv.Ready)
				SetDdeComReady(1);
			break;
		}
		if (LoadTTSET()) {
			char *tmp_u8;
			wchar_t *commandline;
			asprintf(&tmp_u8, "a %s", ParamFileName); // "a" = dummy exe name
			commandline = ToWcharU8(tmp_u8);
			(*ParseParam)(commandline, &ts, NULL);
			free(commandline);
			free(tmp_u8);
		}
		FreeTTSET();
		cv.NoMsg = 1; /* suppress error messages */
		PostMessage(HVTWin,WM_USER_COMMSTART,0,0);
		break;
	case CmdDisconnect:
		if (ParamFileName[0] == '0') {
			PostMessage(HVTWin,WM_USER_ACCELCOMMAND,IdCmdDisconnect,0);
		}
		else {
			PostMessage(HVTWin,WM_USER_ACCELCOMMAND,IdCmdDisconnect,1);
		}
		break;
	case CmdEnableKeyb:
		KeybEnabled = (ParamBinaryFlag!=0);
		break;
	case CmdGetTitle: {
		// title is transferred later by XTYP_REQUEST
		char *titleU8 = ToU8A(ts.Title);
		if (titleU8 == NULL) {
			ParamFileName[0] = 0;
		}
		else {
			strncpy_s(ParamFileName, sizeof(ParamFileName), titleU8, _TRUNCATE);
			free(titleU8);
		}
		break;
	}
	case CmdInit: // initialization signal from TTMACRO
		if (StartupFlag) // in case of startup macro
		{ // TTMACRO is waiting for connecting to the host
			// シリアル接続で自動接続が無効の場合は、接続ダイアログを出さない (2006.9.15 maya)
			if (!((ts.PortType==IdSerial) && (ts.ComAutoConnect == FALSE)) &&
				((ts.PortType==IdSerial) || (ts.HostName[0]!=0)))
			{
				cv.NoMsg = 1;
				// start connecting
				PostMessage(HVTWin,WM_USER_COMMSTART,0,0);
			}
			else  // notify TTMACRO that I can not connect
				SetDdeComReady(0);
			StartupFlag = FALSE;
		}
		break;
	case CmdKmtFinish:
		if (KermitFinish(TRUE)) {
			DdeCmnd = TRUE;
		}
		else {
			result = DDE_FNOTPROCESSED;
		}
		break;
	case CmdKmtRecv:
		if (KermitStartRecive(TRUE)) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	case CmdKmtGet: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		r = KermitGet(ParamFileNameW);
		free(ParamFileNameW);
		if (r) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	}
	case CmdKmtSend: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		r = KermitStartSend(ParamFileNameW);
		free(ParamFileNameW);
		if (r) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	}
	case CmdLoadKeyMap: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		free(ts.KeyCnfFNW);
		ts.KeyCnfFNW = ParamFileNameW;
		PostMessage(HVTWin, WM_USER_ACCELCOMMAND, IdCmdLoadKeyMap, 0);
		break;
	}

	case CmdLogRotate: {
		char *p = ParamFileName;
		int s;

		if (strncmp(p, "size", 4) == 0) {
			s = atoi(&p[5]);
			FLogRotateSize(s);

		} else if (strncmp(p, "rotate", 6) == 0) {
			s = atoi(&p[7]);
			FLogRotateRotate(s);

		} else if (strncmp(p, "halt", 4) == 0) {
			FLogRotateHalt();
		}
		break;
	}

	case CmdLogAutoClose:
		AutoLogClose = (ParamBinaryFlag!=0);
		break;

	case CmdLogClose:
		FLogClose();
		break;
	case CmdLogOpen:
		if (FLogIsOpend()) {
			result = DDE_FNOTPROCESSED;
		}
		else {
			BOOL ret;

			wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
			wchar_t *log_filenameW = FLogGetLogFilename(ParamFileNameW);
			if (log_filenameW == NULL) {
				ret = 0;
			} else {
				ret = FLogOpen(log_filenameW, LOG_UTF8, FALSE);
				free(log_filenameW);
			}
			free(ParamFileNameW);
			strncpy_s(ParamFileName, sizeof(ParamFileName), ret ? "1" : "0", _TRUNCATE);
		}
		break;
	case CmdLogPause:
		FLogPause(TRUE);
		break;
	case CmdLogStart:
		FLogPause(FALSE);
		break;
	case CmdLogWrite: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		FLogWriteStr(ParamFileNameW);
		free(ParamFileNameW);
		break;
	}
	case CmdQVRecv:
		if (QVStartReceive(TRUE)) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	case CmdQVSend: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		r = QVStartSend(ParamFileNameW);
		free(ParamFileNameW);
		if (r) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	}
	case CmdRestoreSetup: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		wchar_t *fnameW = GetFullPathW(ts.HomeDirW, ParamFileNameW);
		free(ParamFileNameW);
		free(ts.SetupFNameW);
		ts.SetupFNameW = fnameW;
		WideCharToACP_t(ts.SetupFNameW, ts.SetupFName, _countof(ts.SetupFName));
		PostMessage(HVTWin,WM_USER_ACCELCOMMAND,IdCmdRestoreSetup,0);
		break;
	}
	case CmdSendBreak:
		PostMessage(HVTWin,WM_USER_ACCELCOMMAND,IdBreak,0);
		break;
	case CmdSendFile: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
#if 0
		// Tera Term 4 と同じ方法で送信
		BOOL r = FileSendStart(ParamFileNameW, ParamBinaryFlag);
#else
		// 5 で追加した方法で送信
		BOOL r = SendMemSendFile2(ParamFileNameW, ParamBinaryFlag, (SendMemDelayType)ts.SendfileDelayType,
								  ts.SendfileDelayTick, ts.SendfileSize, ts.LocalEcho, SendCallback, NULL);
#endif
		free(ParamFileNameW);
		if (r) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	}
	case CmdSendKCode: {
		WORD w = HexStr2Word(ParamFileName);
		WORD c = HexStr2Word(&ParamFileName[4]);
		KeyCodeSend(w, c);
		break;
	}
	case CmdSetEcho:
		ts.LocalEcho = ParamBinaryFlag;
		if (cv.Ready && cv.TelFlag && (ts.TelEcho>0))
			TelChangeEcho();
		break;
	case CmdSetDebug:
		TermSetDebugMode(Command[1] - '0');
		break;
	case CmdSetTitle: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		WideCharToACP_t(ParamFileNameW, ts.Title, _countof(ts.Title));
		if (ts.AcceptTitleChangeRequest == IdTitleChangeRequestOverwrite) {
			free(cv.TitleRemoteW);
			cv.TitleRemoteW = NULL;
		}
		ChangeTitle();
		break;
	}
	case CmdShowTT:
		switch (ParamFileName[0]) {
		case '-': ShowWindow(HVTWin,SW_HIDE); break;
		case '0': ShowWindow(HVTWin,SW_MINIMIZE); break;
		case '1': ShowWindow(HVTWin,SW_RESTORE); break;
		case '2': ShowWindow(HTEKWin,SW_HIDE); break;
		case '3': ShowWindow(HTEKWin,SW_MINIMIZE); break;
		case '4':
			PostMessage(HVTWin,WM_USER_ACCELCOMMAND,IdCmdCtrlOpenTEK,0);
			break;
		case '5':
			PostMessage(HVTWin,WM_USER_ACCELCOMMAND,IdCmdCtrlCloseTEK,0);
			break;
		case '6': //steven add
			FLogWindow(SW_HIDE);
			break;
		case '7': //steven add
			FLogWindow(SW_MINIMIZE);
			break;
		case '8': //steven add
			FLogWindow(SW_RESTORE);
			break;
		}
		break;
	case CmdXmodemRecv: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		r = XMODEMStartReceive(ParamFileNameW, ParamBinaryFlag, ParamXmodemOpt);
		free(ParamFileNameW);
		if (r) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	}
	case CmdXmodemSend: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		r = XMODEMStartSend(ParamFileNameW, ParamXmodemOpt);
		free(ParamFileNameW);
		if (r) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	}
	case CmdZmodemRecv:
		if (ZMODEMStartReceive(TRUE, FALSE)) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	case CmdZmodemSend: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		r = ZMODEMStartSend(ParamFileNameW, ParamBinaryFlag, FALSE);
		free(ParamFileNameW);
		if (r) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	}
	case CmdYmodemRecv:
		if (YMODEMStartReceive(TRUE))
		{
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	case CmdYmodemSend: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		r = YMODEMStartSend(ParamFileNameW);
		if (r) {
			DdeCmnd = TRUE;
		}
		else
			result = DDE_FNOTPROCESSED;
		break;
	}
		// add 'callmenu' (2007.11.18 maya)
	case CmdCallMenu:
		i = atoi(ParamFileName);
		if (i >= 51110 && i <= 51990) {
			PostMessage(HTEKWin,WM_COMMAND,MAKELONG(i,0),0);
		}
		else {
			PostMessage(HVTWin,WM_COMMAND,MAKELONG(i,0),0);
		}
		break;

	case CmdSetSecondFile:  // add 'scpsend' (2008.1.3 yutaka)
		memset(ParamSecondFileName, 0, sizeof(ParamSecondFileName));
		strncpy_s(ParamSecondFileName, sizeof(ParamSecondFileName),&(Command[1]), _TRUNCATE);
		break;

	case CmdScpSend:  // add 'scpsend' (2008.1.1 yutaka)
		{
			wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
			wchar_t *ParamSecondFileNameW = ToWcharU8(ParamSecondFileName);
			BOOL r = ScpSend(ParamFileNameW, ParamSecondFileNameW);
			free(ParamFileNameW);
			free(ParamSecondFileNameW);
			if (r == FALSE) {
				const char *msg = "ttxssh.dll not support scp";
				MessageBox(NULL, msg, "Tera Term: scprecv command error", MB_OK | MB_ICONERROR);
				result = DDE_FNOTPROCESSED;
			}
		}
		break;

	case CmdScpRcv:
		{
			wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
			wchar_t *ParamSecondFileNameW = ToWcharU8(ParamSecondFileName);
			BOOL r = ScpReceive(ParamFileNameW, ParamSecondFileNameW);
			free(ParamFileNameW);
			free(ParamSecondFileNameW);
			if (r == FALSE) {
				const char *msg = "ttxssh.dll not support scp";
				MessageBox(NULL, msg, "Tera Term: scpsend command error", MB_OK | MB_ICONERROR);
				result = DDE_FNOTPROCESSED;
			}
		}
		break;

	case CmdSetBaud:  // add 'setbaud' (2008.2.13 steven patch)
		{
		int val;

		if (!cv.Open || cv.PortType != IdSerial) {
			result = DDE_FNOTPROCESSED;
			break;
		}

		val = atoi(ParamFileName);
		if (val > 0) {
			ts.Baud = val;
			CommResetSerial(&ts,&cv,FALSE);   // reset serial port
			PostMessage(HVTWin,WM_USER_CHANGETITLE,0,0); // refresh title bar
		}
		}
		break;

	case CmdSetFlowCtrl:
		{
		int val;

		if (!cv.Open || cv.PortType != IdSerial) {
			result = DDE_FNOTPROCESSED;
			break;
		}

		val = atoi(ParamFileName);
		switch(val) {
			case 1: // Xon/Xoff
			case 2: // RTS/CTS
			case 3: // none
			case 4: // DSR/DTR
				ts.Flow = val;
				break;
		}
		break;
		}

	case CmdSetRts:  // add 'setrts' (2008.3.12 maya)
		{
		int val, ret;

		if (!cv.Open || cv.PortType != IdSerial || ts.Flow != IdFlowNone) {
			result = DDE_FNOTPROCESSED;
			break;
		}

		val = atoi(ParamFileName);
		if (val == 0) {
			ret = EscapeCommFunction(cv.ComID, CLRRTS);
		}
		else {
			ret = EscapeCommFunction(cv.ComID, SETRTS);
		}
		}
		break;

	case CmdSetDtr:  // add 'setdtr' (2008.3.12 maya)
		{
		int val, ret;

		if (!cv.Open || cv.PortType != IdSerial || ts.Flow != IdFlowNone) {
			result = DDE_FNOTPROCESSED;
			break;
		}

		val = atoi(ParamFileName);
		if (val == 0) {
			ret = EscapeCommFunction(cv.ComID, CLRDTR);
		}
		else {
			ret = EscapeCommFunction(cv.ComID, SETDTR);
		}
		}
		break;

	case CmdSetSerialDelayChar:
		{
			int val;

			if (!cv.Open || cv.PortType != IdSerial) {
				result = DDE_FNOTPROCESSED;
				break;
			}

			val = atoi(ParamFileName);
			if (val >= 0) {
				ts.DelayPerChar = val;
				SendMem *sm = SendMemSetDelay(HWndDdeCli, ts.DelayPerChar, ts.DelayPerLine);
				assert(sm != NULL);
				if (sm != NULL) {
					// SendMemInitEcho(sm, FALSE); 使用しない
					// SendMemInitDelay(sm, SENDMEM_DELAYTYPE_NO_DELAY, 0, 0); 使用しない
					SendMemStart(sm);
				}
			}
		}
		break;

	case CmdSetSerialDelayLine:
		{
			int val;

			if (!cv.Open || cv.PortType != IdSerial) {
				result = DDE_FNOTPROCESSED;
				break;
			}

			val = atoi(ParamFileName);
			if (val >= 0) {
				ts.DelayPerLine = val;
				SendMem *sm = SendMemSetDelay(HWndDdeCli, ts.DelayPerChar, ts.DelayPerLine);
				assert(sm != NULL);
				if (sm != NULL) {
					// SendMemInitEcho(sm, FALSE); 使用しない
					// SendMemInitDelay(sm, SENDMEM_DELAYTYPE_NO_DELAY, 0, 0); 使用しない
					SendMemStart(sm);
				}
			}
		}
		break;

	case CmdGetModemStatus:	 // add 'getmodemstatus' (2015.1.8 yutaka)
		{
		DWORD val, n;

		if (!cv.Open || cv.PortType != IdSerial) {
			result = DDE_FNOTPROCESSED;
			break;
		}

		if (GetCommModemStatus(cv.ComID, &val) == 0) { // error
			result = DDE_FNOTPROCESSED;
			break;
		}
		//val = MS_CTS_ON | MS_DSR_ON | MS_RING_ON | MS_RLSD_ON;
		n = 0;
		if (val & MS_CTS_ON)
			n |= 0x01;
		if (val & MS_DSR_ON)
			n |= 0x02;
		if (val & MS_RING_ON)
			n |= 0x04;
		if (val & MS_RLSD_ON)
			n |= 0x08;

		_snprintf_s(ParamFileName, sizeof(ParamFileName), _TRUNCATE, "%d", n);
		}
		break;

	case CmdGetHostname:  // add 'gethostname' (2008.12.15 maya)
		if (cv.Open) {
			if (cv.PortType == IdTCPIP) {
				strncpy_s(ParamFileName, sizeof(ParamFileName),ts.HostName, _TRUNCATE);
			}
			else if (cv.PortType == IdSerial) {
				_snprintf_s(ParamFileName, sizeof(ParamFileName), _TRUNCATE, "COM%d", ts.ComPort);
			}
		}
		break;

	case CmdGetTTPos:
		int showflag;
		int w_x, w_y, w_width, w_height;	// ウインドウ領域
		int c_x, c_y, c_width, c_height;	// クライアント領域
		RECT r;

		if (IsIconic(HVTWin) == TRUE) {
		    showflag = 1;
		} else if (IsZoomed(HVTWin) == TRUE) {
		    showflag = 2;
		} else if (IsWindowVisible(HVTWin) == FALSE) {
		    showflag = 3;
		} else {
		    showflag = 0;
		}

		GetWindowRect(HVTWin, &r);
		w_x      = r.left;
		w_y      = r.top;
		w_width	 = r.right  - r.left;
		w_height = r.bottom - r.top;

		DispGetWindowPos(vt_src, &c_x, &c_y, TRUE);
		DispGetWindowSize(vt_src, &c_width, &c_height, TRUE);

		_snprintf_s(ParamFileName, sizeof(ParamFileName), _TRUNCATE,
			    "%d %d %d %d %d %d %d %d %d", showflag,
			    w_x, w_y, w_width, w_height,
			    c_x, c_y, c_width, c_height);
		break;

	case CmdSendBroadcast: { // 'sendbroadcast'
		wchar_t *strW = ToWcharU8(ParamFileName);
		if (strW != NULL) {
			SendBroadcastMessage(HVTWin, HVTWin, strW);
			free(strW);
		}
		break;
	}

	case CmdSendMulticast: {
		// 'sendmulticast'
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		wchar_t *ParamSecondFileNameW = ToWcharU8(ParamSecondFileName);
		if (ParamFileNameW != NULL && ParamSecondFileNameW != NULL) {
			SendMulticastMessage(HVTWin, HVTWin, ParamFileNameW, ParamSecondFileNameW);
		}
		free(ParamFileNameW);
		free(ParamSecondFileNameW);
		break;
	}

	case CmdSetMulticastName: {
		// 'setmulticastname'
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		if (ParamFileNameW != NULL) {
			SetMulticastName(ParamFileNameW);
			free(ParamFileNameW);
		}
		break;
	}

	case CmdDispStr: {
		wchar_t *strW = ToWcharU8(ParamFileName);
		if (strW != NULL) {
			SendMem *sm = SendMemTextW(strW, 0);
			if (sm != NULL) {
				SendMemInitEcho(sm, TRUE);
				SendMemInitSend(sm, FALSE);
				SendMemStart(sm);
			}
		}
		break;
	}

	case CmdLogInfo:
		FLogInfo(ParamFileName, sizeof(ParamFileName) - 1);
		break;

	case CmdSendUTF8String:
	case CmdSendBinary:
	case CmdSendCompatString: {
		const char command_byte = Command[0];
		const uint8_t *lptr = &Command[1];
		size_t data_len;
		uint8_t *data_ptr = DecodeDDEBinary(lptr, len - 1, &data_len);
		uint32_t slen =
			(data_ptr[0] << (8*3)) +
			(data_ptr[1] << (8*2)) +
			(data_ptr[2] << 8) +
			data_ptr[3];
		if (slen <= len - 1 - 4) {
			char *sptr = &data_ptr[4];
			switch(command_byte) {
			case CmdSendUTF8String:
				SendStringU8(sptr);
				break;
			case CmdSendBinary:
				SendBinary(sptr, slen);
				break;
			case CmdSendCompatString:
				SendData(sptr, slen);
				break;
			default:
				assert(FALSE);
				SendStringU8(sptr);
				break;
			}
		}
		free(data_ptr);
		break;
	}

	case CmdSetRecvFileOpt: {
		ts.ReceivefileAutoStopWaitTime = (int)HexStr2Word(&Command[1]);
		break;
	}

	case CmdRecvFile: {
		wchar_t *ParamFileNameW = ToWcharU8(ParamFileName);
		BOOL r = RawStartReceive(ParamFileNameW, ts.ReceivefileAutoStopWaitTime, TRUE);
		free(ParamFileNameW);
		if (r) {
			DdeCmnd = TRUE;
		} else{
			result = DDE_FNOTPROCESSED;
		}
		break;
	}

	default:
		result = DDE_FNOTPROCESSED;
		break;
	}
#if USE_ACCESSDATA
	DdeUnaccessData(Data);
#endif
	return result;
}

static HDDEDATA CALLBACK DdeCallbackProc(UINT CallType, UINT Fmt, HCONV Conv,
										 HSZ HSz1, HSZ HSz2, HDDEDATA Data,
										 ULONG_PTR Data1, ULONG_PTR Data2)
{
	HDDEDATA Result;

	if (Inst==0) return 0;
	Result = 0;

	switch (CallType) {
		case XTYP_WILDCONNECT:
			Result = WildConnect(HSz2, HSz1, Fmt);
			break;
		case XTYP_CONNECT:
			if (Conv == 0)
			{
				if ((DdeCmpStringHandles(Topic, HSz1) == 0) &&
				    (DdeCmpStringHandles(Service, HSz2) == 0))
				{
					if (cv.Ready)
						SetDdeComReady(1);
					Result = (HDDEDATA)TRUE;
				}
			}
			break;
		case XTYP_CONNECT_CONFIRM:
			ConvH = Conv;
			break;
		case XTYP_ADVREQ:
		case XTYP_REQUEST:
			Result = AcceptRequest(HSz2);
			break;

	// クライアント(ttpmacro.exe)からサーバ(ttermpro.exe)へデータが送られてくると、
	// このメッセージハンドラへ飛んでくる。
	// 処理したら DDE_FACK、ビジーの場合は DDE_FBUSY 、無視する場合は DDE_FNOTPROCESSED を
	// クライアントへ返す必要があり、break文が抜けていたので追加した。
	// (2006.11.6 yutaka)
		case XTYP_POKE:
			Result = AcceptPoke(HSz2, Fmt, Data);
			break;

		case XTYP_ADVSTART:
			if ((ConvH!=0) &&
			    (DdeCmpStringHandles(Topic, HSz1) == 0) &&
			    (DdeCmpStringHandles(Item, HSz2) == 0) &&
			    ! AdvFlag)
			{
				AdvFlag = TRUE;
				Result = (HDDEDATA)TRUE;
			}
			break;
		case XTYP_ADVSTOP:
			if ((ConvH!=0) &&
			    (DdeCmpStringHandles(Topic, HSz1) == 0) &&
			    (DdeCmpStringHandles(Item, HSz2) == 0) &&
			    AdvFlag)
			{
				AdvFlag = FALSE;
				Result = (HDDEDATA)TRUE;
			}
			break;
		case XTYP_DISCONNECT:
			// マクロ終了時、ログ採取を自動的に停止する。(2013.6.24 yutaka)
			if (AutoLogClose) {
				FLogClose();
				AutoLogClose = FALSE;
			}
			ConvH = 0;
			PostMessage(HVTWin,WM_USER_DDEEND,0,0);
			break;
		case XTYP_EXECUTE:
			Result = AcceptExecute(HSz1, Data);
	}  /* switch (CallType) */

	return Result;
}

BOOL InitDDE()
{
	BOOL Ok;

	if (ConvH != 0) return FALSE;

	Ok = TRUE;

	if (DdeInitialize(&Inst,DdeCallbackProc,0,0) == DMLERR_NO_ERROR)
	{
		Service= DdeCreateStringHandle(Inst, ServiceName, CP_WINANSI);
		Topic  = DdeCreateStringHandle(Inst, TopicName, CP_WINANSI);
		Item   = DdeCreateStringHandle(Inst, ItemName, CP_WINANSI);
		Item2  = DdeCreateStringHandle(Inst, ItemName2, CP_WINANSI);

		Ok = DdeNameService(Inst, Service, 0, DNS_REGISTER) != 0;
	}
	else
		Ok = FALSE;

	SyncMode = FALSE;
	CloseTT = FALSE;
	StartupFlag = FALSE;
	DDELog = FALSE;

	if (Ok)
	{
		Ok = DDECreateBuf();
		if (Ok) DDELog = TRUE;
	}

	if (! Ok) EndDDE();
	return Ok;
}

void SendDDEReady()
{
	GetClientHWnd(TopicName);
	PostMessage(HWndDdeCli,WM_USER_DDEREADY,0,0);
}

void EndDDE()
{
	DWORD Temp;

	if (ConvH != 0)
		DdeDisconnect(ConvH);
	ConvH = 0;
	SyncMode = FALSE;
	StartupFlag = FALSE;

	Temp = Inst;
	if (Inst != 0)
	{
		Inst = 0;
		DdeNameService(Temp, Service, 0, DNS_UNREGISTER);
		if (Service != 0)
			DdeFreeStringHandle(Temp, Service);
		Service = 0;
		if (Topic != 0)
			DdeFreeStringHandle(Temp, Topic);
		Topic = 0;
		if (Item != 0)
			DdeFreeStringHandle(Temp, Item);
		Item = 0;
		if (Item2 != 0)
			DdeFreeStringHandle(Temp, Item2);
		Item2 = 0;

		DdeUninitialize(Temp);
	}
	TopicName[0] = 0;

	if (HWndDdeCli!=NULL)
		PostMessage(HWndDdeCli,WM_USER_DDECMNDEND,0,0);
	HWndDdeCli = NULL;
	AdvFlag = FALSE;

	DDELog = FALSE;
	DDEFreeBuf();
	cv.NoMsg = 0;
}

void DDEAdv()
{
	if ((ConvH==0) ||
	    (! AdvFlag) ||
	    (DDEGetCount() == 0))
		return;

	if ((! SyncMode) ||
	    SyncMode && SyncRecv)
	{
		if (SyncFreeSpace<10)
			SyncFreeSpace=0;
		else
			SyncFreeSpace=SyncFreeSpace-10;
		DdePostAdvise(Inst,Topic,Item);
		SyncRecv = FALSE;
	}
}

void EndDdeCmnd(int Result)
{
	if ((ConvH==0) || (HWndDdeCli==NULL) || (! DdeCmnd)) return;
	PostMessage(HWndDdeCli,WM_USER_DDECMNDEND,(WPARAM)Result,0);
	DdeCmnd = FALSE;
}

void SetDdeComReady(WORD Ready)
{
	if (HWndDdeCli==NULL) return;
	PostMessage(HWndDdeCli,WM_USER_DDECOMREADY,Ready,0);
}

/**
 *	マクロを起動する
 *
 *	@param  FName: macro filename
 *	@param  startup: TRUE in case of startup macro execution.
 *		  In this case, the connection to the host will
 *		  made after the link to TT(P)MACRO is established.
 */
void RunMacroW(const wchar_t *FName, BOOL startup)
{
	PROCESS_INFORMATION pi;
	wchar_t *Cmnd;
	STARTUPINFOW si;
	DWORD pri = NORMAL_PRIORITY_CLASS;
	wchar_t *exe_dir;

	// Control menuからのマクロ呼び出しで、すでにマクロ起動中の場合、
	// 該当する"ttpmacro"をフラッシュする。
	// (2010.4.2 yutaka, maya)
	if ((FName == NULL && startup == FALSE) && ConvH != 0) {
		BringupMacroWindow(TRUE);
		return;
	}

	SetTopic();
	if (! InitDDE()) return;

	exe_dir = GetExeDirW(NULL);
	aswprintf(&Cmnd, L"%s\\TTPMACRO /D=%hs", exe_dir, TopicName);
	free(exe_dir);
	if (FName != NULL) {
		if (wcschr(FName, ' ') != NULL) {
			// ファイル名にスペースが含まれている -> quote('"'で囲む)する
			awcscats(&Cmnd, L" \"", FName, L"\"", NULL);
		}
		else {
			awcscats(&Cmnd, L" ", FName, NULL);
		}
	}
	if (startup) {
		awcscat(&Cmnd, L" /S"); // "startup" flag
	}

	StartupFlag = startup;

	// ログ採取中も下げないことにする。(2005.8.14 yutaka)
#if 0
	// Tera Term本体でログ採取中にマクロを実行すると、マクロの動作が停止することが
	// あるため、プロセスの優先度を1つ下げて実行させる。(2004/9/5 yutaka)
	// ログ採取中のみに下げる。(2004/11/28 yutaka)
	if (FileLog || BinLog) {
		pri = BELOW_NORMAL_PRIORITY_CLASS;
	}
	// 暫定処置として、常に下げることにする。(2005/5/15 yutaka)
	// マクロによるtelnet自動ログインが失敗することがあるので、下げないことにする。(2005/5/23 yutaka)
	pri = BELOW_NORMAL_PRIORITY_CLASS;
#endif

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	GetStartupInfoW(&si);
	si.wShowWindow = SW_MINIMIZE;

	if (CreateProcessW(
		NULL,
		Cmnd,
		NULL,
		NULL,
		FALSE,
		pri,
		NULL, NULL,
		&si, &pi) == 0) {
			EndDDE();
	}
	else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	free(Cmnd);
}

void RunMacro(PCHAR FName, BOOL Startup)
{
	wchar_t *fnameW = ToWcharA(FName);
	RunMacroW(fnameW, Startup);
	free(fnameW);
}
