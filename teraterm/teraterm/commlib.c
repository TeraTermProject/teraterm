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
/* IPv6 modification is Copyright (C) 2000, 2001 Jun-ya KATO <kato@win6.jp> */

/* TERATERM.EXE, Communication routines */
#include "teraterm.h"
#include "tttypes.h"
#include "tt_res.h"
#include <process.h>

#include "ttcommon.h"
#include "ttwsk.h"
#include "ttlib.h"
#include "ttfileio.h"
#include "ttplug.h" /* TTPLUG */

#include "commlib.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h> /* for _snprintf() */
#include <time.h>
#include <locale.h>

static SOCKET OpenSocket(PComVar);
static void AsyncConnect(PComVar);
static int CloseSocket(SOCKET);

/* create socket */
static SOCKET OpenSocket(PComVar cv)
{
	cv->s = cv->res->ai_family;
	cv->s = Psocket(cv->res->ai_family, cv->res->ai_socktype, cv->res->ai_protocol);
	return cv->s;
}

/* connect with asynchronous mode */
static void AsyncConnect(PComVar cv)
{
	int Err;
	BOOL BBuf;
	BBuf = TRUE;
	/* set synchronous mode */
	PWSAAsyncSelect(cv->s,cv->HWin,0,0);
	Psetsockopt(cv->s,(int)SOL_SOCKET,SO_OOBINLINE,(char *)&BBuf,sizeof(BBuf));
	/* set asynchronous mode */
	PWSAAsyncSelect(cv->s,cv->HWin,WM_USER_COMMOPEN, FD_CONNECT);

	// ホストへの接続中に一定時間立つと、強制的にソケットをクローズして、
	// 接続処理をキャンセルさせる。値が0の場合は何もしない。
	// (2007.1.11 yutaka)
	if (*cv->ConnetingTimeout > 0) {
		SetTimer(cv->HWin, IdCancelConnectTimer, *cv->ConnetingTimeout * 1000, NULL);
	}

	/* WM_USER_COMMOPEN occurs, CommOpen is called, then CommStart is called */
	Err = Pconnect(cv->s, cv->res->ai_addr, cv->res->ai_addrlen);
	if (Err != 0) {
		Err = PWSAGetLastError();
		if (Err == WSAEWOULDBLOCK)  {
			/* Do nothing */
		} else if (Err!=0 ) {
			PostMessage(cv->HWin, WM_USER_COMMOPEN,0,
			            MAKELONG(FD_CONNECT,Err));
		}
	}
}

/* close socket */
static int CloseSocket(SOCKET s)
{
	return Pclosesocket(s);
}

#define CommInQueSize 8192
#define CommOutQueSize 2048
#define CommXonLim 2048
#define CommXoffLim 2048

#define READENDNAME "ReadEnd"
#define WRITENAME "Write"
#define READNAME "Read"
#define PRNWRITENAME "PrnWrite"

static HANDLE ReadEnd;
static OVERLAPPED wol, rol;

// Winsock async operation handle
static HANDLE HAsync=0;

BOOL TCPIPClosed = TRUE;

/* Printer port handle for
   direct pass-thru printing */
static HANDLE PrnID = INVALID_HANDLE_VALUE;
static BOOL LPTFlag;

// Initialize ComVar.
// This routine is called only once
// by the initialization procedure of Tera Term.
void CommInit(PComVar cv)
{
	cv->Open = FALSE;
	cv->Ready = FALSE;

// log-buffer variables
	cv->HLogBuf = 0;
	cv->HBinBuf = 0;
	cv->LogBuf = NULL;
	cv->BinBuf = NULL;
	cv->LogPtr = 0;
	cv->LStart = 0;
	cv->LCount = 0;
	cv->BinPtr = 0;
	cv->BStart = 0;
	cv->BCount = 0;
	cv->DStart = 0;
	cv->DCount = 0;
	cv->BinSkip = 0;
	cv->FilePause = 0;
	cv->ProtoFlag = FALSE;
/* message flag */
	cv->NoMsg = 0;

	cv->isSSH = 0;
	cv->TitleRemote[0] = '\0';

	cv->NotifyIcon = NULL;
}

/* reset a serial port which is already open */
void CommResetSerial(PTTSet ts, PComVar cv, BOOL ClearBuff)
{
	DCB dcb;
	DWORD DErr;
	COMMTIMEOUTS ctmo;

	if (! cv->Open ||
		(cv->PortType != IdSerial)) {
			return;
	}

	ClearCommError(cv->ComID,&DErr,NULL);
	SetupComm(cv->ComID,CommInQueSize,CommOutQueSize);
	/* flush input and output buffers */
	if (ClearBuff) {
		PurgeComm(cv->ComID, PURGE_TXABORT | PURGE_RXABORT |
		                     PURGE_TXCLEAR | PURGE_RXCLEAR);
	}

	memset(&ctmo,0,sizeof(ctmo));
	ctmo.ReadIntervalTimeout = MAXDWORD;
	ctmo.WriteTotalTimeoutConstant = 500;
	SetCommTimeouts(cv->ComID,&ctmo);
	cv->InBuffCount = 0;
	cv->InPtr = 0;
	cv->OutBuffCount = 0;
	cv->OutPtr = 0;

	cv->DelayPerChar = ts->DelayPerChar;
	cv->DelayPerLine = ts->DelayPerLine;

	memset(&dcb,0,sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = ts->Baud;
	dcb.fBinary = TRUE;
	switch (ts->Parity) {
		case IdParityNone:
			dcb.Parity = NOPARITY;
			break;
		case IdParityOdd:
			dcb.fParity = TRUE;
			dcb.Parity = ODDPARITY;
			break;
		case IdParityEven:
			dcb.fParity = TRUE;
			dcb.Parity = EVENPARITY;
			break;
		case IdParityMark:
			dcb.fParity = TRUE;
			dcb.Parity = MARKPARITY;
			break;
		case IdParitySpace:
			dcb.fParity = TRUE;
			dcb.Parity = SPACEPARITY;
			break;
	}

	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	switch (ts->Flow) {
		case IdFlowX:
			dcb.fOutX = TRUE;
			dcb.fInX = TRUE;
			dcb.XonLim = CommXonLim;
			dcb.XoffLim = CommXoffLim;
			dcb.XonChar = XON;
			dcb.XoffChar = XOFF;
			break;
		case IdFlowHard:
			dcb.fOutxCtsFlow = TRUE;
			dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
			break;
	}

	switch (ts->DataBit) {
		case IdDataBit7:
			dcb.ByteSize = 7;
			break;
		case IdDataBit8:
			dcb.ByteSize = 8;
			break;
	}
	switch (ts->StopBit) {
		case IdStopBit1:
			dcb.StopBits = ONESTOPBIT;
			break;
		case IdStopBit15:
			dcb.StopBits = ONE5STOPBITS;
			break;
		case IdStopBit2:
			dcb.StopBits = TWOSTOPBITS;
			break;
	}

	SetCommState(cv->ComID, &dcb);

	/* enable receive request */
	SetCommMask(cv->ComID,0);
	SetCommMask(cv->ComID,EV_RXCHAR);
}

// 名前付きパイプが正しい書式かをチェックする。
// \\ServerName\pipe\PipeName
//
// return  0: 正しい
//        -1: 不正
// (2012.3.10 yutaka)
int CheckNamedPipeFormat(char *p, int size)
{
	int ret = -1;
	char *s;

	if (size <= 8)
		goto error;

	if (p[0] == '\\' && p[1] == '\\') {
		s = strchr(&p[2], '\\');
		if (s && _strnicmp(s+1, "pipe\\", 5) == 0) {
			ret = 0;
		}
	}

error:
	return (ret);
}

void CommOpen(HWND HW, PTTSet ts, PComVar cv)
{
	char ErrMsg[21+256];
	char P[50+256];

	MSG Msg;
	ADDRINFO hints;
	char pname[NI_MAXSERV];

	BOOL InvalidHost;

	char uimsg[MAX_UIMSG];

	// ホスト名が名前付きパイプかどうかを調べる。
	if (ts->PortType == IdTCPIP) {
		if (CheckNamedPipeFormat(ts->HostName, strlen(ts->HostName)) == 0) {
			ts->PortType = IdNamedPipe;
		}
	}

	/* initialize ComVar */
	cv->InBuffCount = 0;
	cv->InPtr = 0;
	cv->OutBuffCount = 0;
	cv->OutPtr = 0;
	cv->HWin = HW;
	cv->Ready = FALSE;
	cv->Open = FALSE;
	cv->PortType = ts->PortType;
	cv->ComPort = 0;
	cv->RetryCount = 0;
	cv->RetryWithOtherProtocol = TRUE;
	cv->s = INVALID_SOCKET;
	cv->ComID = INVALID_HANDLE_VALUE;
	cv->CanSend = TRUE;
	cv->RRQ = FALSE;
	cv->SendKanjiFlag = FALSE;
	cv->SendCode = IdASCII;
	cv->EchoKanjiFlag = FALSE;
	cv->EchoCode = IdASCII;
	cv->Language = ts->Language;
	cv->CRSend = ts->CRSend;
	cv->KanjiCodeEcho = ts->KanjiCode;
	cv->JIS7KatakanaEcho = ts->JIS7Katakana;
	cv->KanjiCodeSend = ts->KanjiCodeSend;
	cv->JIS7KatakanaSend = ts->JIS7KatakanaSend;
	cv->KanjiIn = ts->KanjiIn;
	cv->KanjiOut = ts->KanjiOut;
	cv->RussHost = ts->RussHost;
	cv->RussClient = ts->RussClient;
	cv->DelayFlag = TRUE;
	cv->DelayPerChar = ts->DelayPerChar;
	cv->DelayPerLine = ts->DelayPerLine;
	cv->TelBinRecv = FALSE;
	cv->TelBinSend = FALSE;
	cv->TelFlag = FALSE;
	cv->TelMode = FALSE;
	cv->IACFlag = FALSE;
	cv->TelCRFlag = FALSE;
	cv->TelCRSend = FALSE;
	cv->TelCRSendEcho = FALSE;
	cv->TelAutoDetect = ts->TelAutoDetect; /* TTPLUG */
	cv->Locale = ts->Locale;
	cv->locale = _create_locale(LC_ALL, cv->Locale);
	cv->CodePage = &ts->CodePage;
	cv->ConnetingTimeout = &ts->ConnectingTimeout;
	cv->LastSendTime = time(NULL);
	cv->LineModeBuffCount = 0;
	cv->Flush = FALSE;
	cv->FlushLen = 0;
	cv->TelLineMode = FALSE;

	if ((ts->PortType!=IdSerial) && (strlen(ts->HostName)==0))
	{
		PostMessage(cv->HWin, WM_USER_COMMNOTIFY, 0, FD_CLOSE);
		return;
	}

	switch (ts->PortType) {
		case IdTCPIP:
			cv->TelFlag = (ts->Telnet > 0);
			if (ts->EnableLineMode) {
				cv->TelLineMode = TRUE;
			}
			if (! LoadWinsock()) {
				if (cv->NoMsg==0) {
					get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: Error", ts->UILanguageFile);
					get_lang_msg("MSG_WINSOCK_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Cannot use winsock", ts->UILanguageFile);
					MessageBox(cv->HWin,ts->UIMsg,uimsg,MB_TASKMODAL | MB_ICONEXCLAMATION);
				}
				InvalidHost = TRUE;
			}
			else {
				TTXOpenTCP(); /* TTPLUG */
				cv->Open = TRUE;
				/* resolving address */
				memset(&hints, 0, sizeof(hints));
				hints.ai_family = ts->ProtocolFamily;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_TCP;
				_snprintf_s(pname, sizeof(pname), _TRUNCATE, "%d", ts->TCPPort);

				HAsync = PWSAAsyncGetAddrInfo(HW, WM_USER_GETHOST,
				                             ts->HostName, pname, &hints, &cv->res0);
				if (HAsync == 0)
					InvalidHost = TRUE;
				else {
					cv->ComPort = 1; // set "getting host" flag
					                 //  (see CVTWindow::OnSysCommand())
					do {
						if (GetMessage(&Msg,0,0,0)) {
							if ((Msg.hwnd==HW) &&
							    ((Msg.message == WM_SYSCOMMAND) &&
							     ((Msg.wParam & 0xfff0) == SC_CLOSE) ||
							     (Msg.message == WM_COMMAND) &&
							     (LOWORD(Msg.wParam) == ID_FILE_EXIT) ||
							     (Msg.message == WM_CLOSE))) { /* Exit when the user closes Tera Term */
								PWSACancelAsyncRequest(HAsync);
								CloseHandle(HAsync);
								HAsync = 0;
								cv->ComPort = 0; // clear "getting host" flag
								PostMessage(HW,Msg.message,Msg.wParam,Msg.lParam);
								return;
							}
							if (Msg.message != WM_USER_GETHOST) { /* Prosess messages */
								TranslateMessage(&Msg);
								DispatchMessage(&Msg);
							}
						}
						else {
							return;
						}
					} while (Msg.message!=WM_USER_GETHOST);
					cv->ComPort = 0; // clear "getting host" flag
					CloseHandle(HAsync);
					HAsync = 0;
					InvalidHost = WSAGETASYNCERROR(Msg.lParam) != 0;
				}
			} /* if (!LoadWinsock()) */

			if (InvalidHost) {
				if (cv->NoMsg==0) {
					get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: Error", ts->UILanguageFile);
					get_lang_msg("MSG_INVALID_HOST_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Invalid host", ts->UILanguageFile);
					MessageBox(cv->HWin,ts->UIMsg,uimsg,MB_TASKMODAL | MB_ICONEXCLAMATION);
				}
				goto BreakSC;
			}
			for (cv->res = cv->res0; cv->res; cv->res = cv->res->ai_next) {
				cv->s =  OpenSocket(cv);
				if (cv->s == INVALID_SOCKET) {
					CloseSocket(cv->s);
					continue;
				}
				/* start asynchronous connect */
				AsyncConnect(cv);
				break; /* break for-loop immediately */
			}
			break;

		case IdSerial:
			InitFileIO(IdSerial);  /* TTPLUG */
			TTXOpenFile(); /* TTPLUG */
			_snprintf_s(P, sizeof(P), _TRUNCATE, "COM%d", ts->ComPort);
			strncpy_s(ErrMsg, sizeof(ErrMsg),P, _TRUNCATE);
			strncpy_s(P, sizeof(P),"\\\\.\\", _TRUNCATE);
			strncat_s(P, sizeof(P),ErrMsg, _TRUNCATE);
			cv->ComID =
			PCreateFile(P,GENERIC_READ | GENERIC_WRITE,
			            0,NULL,OPEN_EXISTING,
			            FILE_FLAG_OVERLAPPED,NULL);
			if (cv->ComID == INVALID_HANDLE_VALUE ) {
				get_lang_msg("MSG_CANTOPEN_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Cannot open %s", ts->UILanguageFile);
				_snprintf_s(ErrMsg, sizeof(ErrMsg), _TRUNCATE, ts->UIMsg, &P[4]);

				if (cv->NoMsg==0) {
					get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: Error", ts->UILanguageFile);
					MessageBox(cv->HWin,ErrMsg,uimsg,MB_TASKMODAL | MB_ICONEXCLAMATION);
				}
				InvalidHost = TRUE;
			}
			else {
				cv->Open = TRUE;
				cv->ComPort = ts->ComPort;
				CommResetSerial(ts, cv, ts->ClearComBuffOnOpen);
				if (!ts->ClearComBuffOnOpen) {
					cv->RRQ = TRUE;
				}

				/* notify to VT window that Comm Port is open */
				PostMessage(cv->HWin, WM_USER_COMMOPEN, 0, 0);
				InvalidHost = FALSE;

				SetCOMFlag(ts->ComPort);
			}
			break; /* end of "case IdSerial:" */

		case IdFile:
			InitFileIO(IdFile);  /* TTPLUG */
			TTXOpenFile(); /* TTPLUG */
			cv->ComID = PCreateFile(ts->HostName,GENERIC_READ,0,NULL,
			                        OPEN_EXISTING,0,NULL);
			InvalidHost = (cv->ComID == INVALID_HANDLE_VALUE);
			if (InvalidHost) {
				if (cv->NoMsg==0) {
					get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: Error", ts->UILanguageFile);
					get_lang_msg("MSG_CANTOPEN_FILE_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Cannot open file", ts->UILanguageFile);
					MessageBox(cv->HWin,ts->UIMsg,uimsg,MB_TASKMODAL | MB_ICONEXCLAMATION);
				}
			}
			else {
				cv->Open = TRUE;
				PostMessage(cv->HWin, WM_USER_COMMOPEN, 0, 0);
			}
			break;

		case IdNamedPipe:
			InitFileIO(IdNamedPipe);  /* TTPLUG */
			TTXOpenFile(); /* TTPLUG */

			memset(P, 0, sizeof(P));
			strncpy_s(P, sizeof(P), ts->HostName, _TRUNCATE);

			// 名前付きパイプが正しい書式かをチェックする。
			if (CheckNamedPipeFormat(P, strlen(P)) < 0) {
				InvalidHost = TRUE;

				_snprintf_s(ErrMsg, sizeof(ErrMsg), _TRUNCATE,
					"Invalid pipe name (%d)\n\n"
					"A valid pipe name has the form\n"
					"\"\\\\<ServerName>\\pipe\\<PipeName>\"",
					GetLastError());
				get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: Error", ts->UILanguageFile);
				MessageBox(cv->HWin,ErrMsg,uimsg,MB_TASKMODAL | MB_ICONEXCLAMATION);
				break;
			}

			cv->ComID =
			PCreateFile(P,GENERIC_READ | GENERIC_WRITE,
			            0,NULL,OPEN_EXISTING,
			            0,  // ブロッキングモードにする(FILE_FLAG_OVERLAPPED は指定しない)
						NULL);
			if (cv->ComID == INVALID_HANDLE_VALUE ) {
				get_lang_msg("MSG_CANTOPEN_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Cannot open %s", ts->UILanguageFile);
				_snprintf_s(ErrMsg, sizeof(ErrMsg), _TRUNCATE, ts->UIMsg, &P[4]);

				if (cv->NoMsg==0) {
					get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: Error", ts->UILanguageFile);
					MessageBox(cv->HWin,ErrMsg,uimsg,MB_TASKMODAL | MB_ICONEXCLAMATION);
				}
				InvalidHost = TRUE;
			}
			else {
				cv->Open = TRUE;
				PostMessage(cv->HWin, WM_USER_COMMOPEN, 0, 0);
				InvalidHost = FALSE;
			}
			break; /* end of "case IdNamedPipe:" */

	} /* end of "switch" */

BreakSC:
	if (InvalidHost) {
		PostMessage(cv->HWin, WM_USER_COMMNOTIFY, 0, FD_CLOSE);
		if ( (ts->PortType==IdTCPIP) && cv->Open ) {
			if ( cv->s!=INVALID_SOCKET ) {
				Pclosesocket(cv->s);
				cv->s = INVALID_SOCKET;  /* ソケット無効の印を付ける。(2010.8.6 yutaka) */
			}
			FreeWinsock();
		}
		return;
	}
}

// 名前付きパイプ用スレッド
void NamedPipeThread(void *arg)
{
	PComVar cv = (PComVar)arg;
	DWORD DErr;
	HANDLE REnd;
	char Temp[20];
	char Buffer[1];  // 1byte
	DWORD BytesRead, TotalBytesAvail, BytesLeftThisMessage;

	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%s%d", READENDNAME, cv->ComPort);
	REnd = OpenEvent(EVENT_ALL_ACCESS,FALSE, Temp);
	while (TRUE) {
		BytesRead = 0;
		// 名前付きパイプはイベントを待つことができない仕様なので、キューの中身を
		// 覗き見することで、ReadFile() するかどうか判断する。
		if (PeekNamedPipe(cv->ComID, Buffer, sizeof(Buffer), &BytesRead, &TotalBytesAvail, &BytesLeftThisMessage)) {
			if (! cv->Ready) {
				_endthread();
			}
			if (BytesRead == 0) {  // 空だったら、何もしない。
				Sleep(1);
				continue;
			}
			if (! cv->RRQ) {
				PostMessage(cv->HWin, WM_USER_COMMNOTIFY, 0, FD_READ);
			}
			// ReadFile() が終わるまで待つ。
			WaitForSingleObject(REnd,INFINITE);
		}
		else {
			DErr = GetLastError();
			// [VMware] this returns 109 (broken pipe) if a named pipe is removed.
			// [Virtual Box] this returns 233 (pipe not connected) if a named pipe is removed.
			if (! cv->Ready || ERROR_BROKEN_PIPE == DErr || ERROR_PIPE_NOT_CONNECTED == DErr) {
				PostMessage(cv->HWin, WM_USER_COMMNOTIFY, 0, FD_CLOSE);
				_endthread();
			}
		}
	}
}

void CommThread(void *arg)
{
	DWORD Evt;
	PComVar cv = (PComVar)arg;
	DWORD DErr;
	HANDLE REnd;
	char Temp[20];

	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%s%d", READENDNAME, cv->ComPort);
	REnd = OpenEvent(EVENT_ALL_ACCESS,FALSE, Temp);
	while (TRUE) {
		if (WaitCommEvent(cv->ComID,&Evt,NULL)) {
			if (! cv->Ready) {
				_endthread();
			}
			if (! cv->RRQ) {
				PostMessage(cv->HWin, WM_USER_COMMNOTIFY, 0, FD_READ);
			}
			WaitForSingleObject(REnd,INFINITE);
		}
		else {
			DErr = GetLastError();  // this returns 995 (operation aborted) if a USB com port is removed
			if (! cv->Ready || ERROR_OPERATION_ABORTED == DErr) {
				_endthread();
			}
			ClearCommError(cv->ComID,&DErr,NULL);
		}
	}
}

void CommStart(PComVar cv, LONG lParam, PTTSet ts)
{
	char ErrMsg[31];
	char Temp[20];
	char uimsg[MAX_UIMSG];

	if (! cv->Open ) {
		return;
	}
	if ( cv->Ready ) {
		return;
	}

	// キャンセルタイマがあれば取り消す。ただし、この時点で WM_TIMER が送られている可能性はある。
	if (*cv->ConnetingTimeout > 0) {
		KillTimer(cv->HWin, IdCancelConnectTimer);
	}

	switch (cv->PortType) {
		case IdTCPIP:
			ErrMsg[0] = 0;
			switch (HIWORD(lParam)) {
				case WSAECONNREFUSED:
					get_lang_msg("MSG_COMM_REFUSE_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Connection refused", ts->UILanguageFile);
					_snprintf_s(ErrMsg, sizeof(ErrMsg), _TRUNCATE, "%s", ts->UIMsg);
					break;
				case WSAENETUNREACH:
					get_lang_msg("MSG_COMM_REACH_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Network cannot be reached", ts->UILanguageFile);
					_snprintf_s(ErrMsg, sizeof(ErrMsg), _TRUNCATE, "%s", ts->UIMsg);
					break;
				case WSAETIMEDOUT:
					get_lang_msg("MSG_COMM_CONNECT_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Connection timed out", ts->UILanguageFile);
					_snprintf_s(ErrMsg, sizeof(ErrMsg), _TRUNCATE, "%s", ts->UIMsg);
					break;
				default:
					get_lang_msg("MSG_COMM_TIMEOUT_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Cannot connect the host", ts->UILanguageFile);
					_snprintf_s(ErrMsg, sizeof(ErrMsg), _TRUNCATE, "%s", ts->UIMsg);
			}
			if (HIWORD(lParam)>0) {
				/* connect() failed */
				if (cv->res->ai_next != NULL) {
					/* try to connect with other protocol */
					CloseSocket(cv->s);
					for (cv->res = cv->res->ai_next; cv->res; cv->res = cv->res->ai_next) {
						cv->s = OpenSocket(cv);
						if (cv->s == INVALID_SOCKET) {
							CloseSocket(cv->s);
							continue;
						}
						AsyncConnect(cv);
						cv->Ready = FALSE;
						cv->RetryWithOtherProtocol = TRUE; /* retry with other procotol */
						return;
					}
				} else {
					/* trying with all protocol family are failed */
					if (cv->NoMsg==0)
					{
						get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: Error", ts->UILanguageFile);
						MessageBox(cv->HWin,ErrMsg,uimsg,MB_TASKMODAL | MB_ICONEXCLAMATION);
					}
					PostMessage(cv->HWin, WM_USER_COMMNOTIFY, 0, FD_CLOSE);
					cv->RetryWithOtherProtocol = FALSE;
					return;
				}
			}

			/* here is connection established */
			cv->RetryWithOtherProtocol = FALSE;
			PWSAAsyncSelect(cv->s,cv->HWin,WM_USER_COMMNOTIFY, FD_READ | FD_OOB | FD_CLOSE);
			TCPIPClosed = FALSE;
			break;

		case IdSerial:
			_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%s%d", READENDNAME, cv->ComPort);
			ReadEnd = CreateEvent(NULL,FALSE,FALSE,Temp);
			_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%s%d", WRITENAME, cv->ComPort);
			memset(&wol,0,sizeof(OVERLAPPED));
			wol.hEvent = CreateEvent(NULL,TRUE,TRUE,Temp);
			_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%s%d", READNAME, cv->ComPort);
			memset(&rol,0,sizeof(OVERLAPPED));
			rol.hEvent = CreateEvent(NULL,TRUE,FALSE,Temp);

			/* create the receiver thread */
			if (_beginthread(CommThread,0,cv) == -1) {
				get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: Error", ts->UILanguageFile);
				get_lang_msg("MSG_TT_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Can't create thread", ts->UILanguageFile);
				MessageBox(cv->HWin,ts->UIMsg,uimsg,MB_TASKMODAL | MB_ICONEXCLAMATION);
			}
			break;

		case IdFile:
			cv->RRQ = TRUE;
			break;

		case IdNamedPipe:
			cv->ComPort = 0;
			_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%s%d", READENDNAME, cv->ComPort);
			ReadEnd = CreateEvent(NULL,FALSE,FALSE,Temp);
			_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%s%d", WRITENAME, cv->ComPort);
			memset(&wol,0,sizeof(OVERLAPPED));
			wol.hEvent = CreateEvent(NULL,TRUE,TRUE,Temp);
			_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%s%d", READNAME, cv->ComPort);
			memset(&rol,0,sizeof(OVERLAPPED));
			rol.hEvent = CreateEvent(NULL,TRUE,FALSE,Temp);

			/* create the receiver thread */
			if (_beginthread(NamedPipeThread,0,cv) == -1) {
				get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: Error", ts->UILanguageFile);
				get_lang_msg("MSG_TT_ERROR", ts->UIMsg, sizeof(ts->UIMsg), "Can't create thread", ts->UILanguageFile);
				MessageBox(cv->HWin,ts->UIMsg,uimsg,MB_TASKMODAL | MB_ICONEXCLAMATION);
			}
			break;
	}
	cv->Ready = TRUE;
}

BOOL CommCanClose(PComVar cv)
{ // check if data remains in buffer
	if (! cv->Open) {
		return TRUE;
	}
	if (cv->InBuffCount>0) {
		return FALSE;
	}
	if ((cv->HLogBuf!=NULL) &&
	    ((cv->LCount>0) ||
	     (cv->DCount>0))) {
		return FALSE;
	}
	if ((cv->HBinBuf!=NULL) &&
	    (cv->BCount>0)) {
		return FALSE;
	}
	return TRUE;
}

void CommClose(PComVar cv)
{
	if ( ! cv->Open ) {
		return;
	}
	cv->Open = FALSE;

	/* disable event message posting & flush buffer */
	cv->RRQ = FALSE;
	cv->Ready = FALSE;
	cv->InPtr = 0;
	cv->InBuffCount = 0;
	cv->OutPtr = 0;
	cv->OutBuffCount = 0;
	cv->LineModeBuffCount = 0;
	cv->FlushLen = 0;
	cv->Flush = FALSE;

	/* close port & release resources */
	switch (cv->PortType) {
		case IdTCPIP:
			if (HAsync!=0) {
				PWSACancelAsyncRequest(HAsync);
			}
			HAsync = 0;
			Pfreeaddrinfo(cv->res0);
			if ( cv->s!=INVALID_SOCKET ) {
				Pclosesocket(cv->s);
			}
			cv->s = INVALID_SOCKET;
			TTXCloseTCP(); /* TTPLUG */
			FreeWinsock();
			break;
		case IdSerial:
			if ( cv->ComID != INVALID_HANDLE_VALUE ) {
				CloseHandle(ReadEnd);
				CloseHandle(wol.hEvent);
				CloseHandle(rol.hEvent);
				PurgeComm(cv->ComID, PURGE_TXABORT | PURGE_RXABORT |
				                     PURGE_TXCLEAR | PURGE_RXCLEAR);
				EscapeCommFunction(cv->ComID,CLRDTR);
				SetCommMask(cv->ComID,0);
				PCloseFile(cv->ComID);
				ClearCOMFlag(cv->ComPort);
			}
			TTXCloseFile(); /* TTPLUG */
			break;
		case IdFile:
			if (cv->ComID != INVALID_HANDLE_VALUE) {
				PCloseFile(cv->ComID);
			}
			TTXCloseFile(); /* TTPLUG */
			break;

		case IdNamedPipe:
			if ( cv->ComID != INVALID_HANDLE_VALUE ) {
				CloseHandle(ReadEnd);
				CloseHandle(wol.hEvent);
				CloseHandle(rol.hEvent);
				PCloseFile(cv->ComID);
			}
			TTXCloseFile(); /* TTPLUG */
			break;
	}
	cv->ComID = INVALID_HANDLE_VALUE;
	cv->PortType = 0;

	_free_locale(cv->locale);
}

void CommProcRRQ(PComVar cv)
{
	if ( ! cv->Ready ) {
		return;
	}
	/* disable receive request */
	switch (cv->PortType) {
		case IdTCPIP:
			if (! TCPIPClosed) {
				PWSAAsyncSelect(cv->s,cv->HWin,WM_USER_COMMNOTIFY, FD_OOB | FD_CLOSE);
			}
			break;
		case IdSerial:
			break;
	}
	cv->RRQ = TRUE;
	CommReceive(cv);
}

void CommReceive(PComVar cv)
{
	DWORD C;
	DWORD DErr;

	if (! cv->Ready || ! cv->RRQ ||
	    (cv->InBuffCount>=InBuffSize)) {
		return;
	}

	/* Compact buffer */
	if ((cv->InBuffCount>0) && (cv->InPtr>0)) {
		memmove(cv->InBuff,&(cv->InBuff[cv->InPtr]),cv->InBuffCount);
		cv->InPtr = 0;
	}

	if (cv->InBuffCount<InBuffSize) {
		switch (cv->PortType) {
			case IdTCPIP:
				C = Precv(cv->s, &(cv->InBuff[cv->InBuffCount]),
				          InBuffSize-cv->InBuffCount, 0);
				if (C == SOCKET_ERROR) {
					C = 0;
					PWSAGetLastError();
				}
				cv->InBuffCount = cv->InBuffCount + C;
				break;
			case IdSerial:
				do {
					ClearCommError(cv->ComID,&DErr,NULL);
					if (! PReadFile(cv->ComID,&(cv->InBuff[cv->InBuffCount]),
					                InBuffSize-cv->InBuffCount,&C,&rol)) {
						if (GetLastError() == ERROR_IO_PENDING) {
							if (WaitForSingleObject(rol.hEvent, 1000) != WAIT_OBJECT_0) {
								C = 0;
							}
							else {
								GetOverlappedResult(cv->ComID,&rol,&C,FALSE);
							}
						}
						else {
							C = 0;
						}
					}
					cv->InBuffCount = cv->InBuffCount + C;
				} while ((C!=0) && (cv->InBuffCount<InBuffSize));
				ClearCommError(cv->ComID,&DErr,NULL);
				break;
			case IdFile:
				if (PReadFile(cv->ComID,&(cv->InBuff[cv->InBuffCount]),
				              InBuffSize-cv->InBuffCount,&C,NULL)) {
					if (C == 0) {
						DErr = ERROR_HANDLE_EOF;
					}
					else {
						cv->InBuffCount = cv->InBuffCount + C;
					}
				}
				else {
					DErr = GetLastError();
				}
				break;

			case IdNamedPipe:
				// キューの中に最低1バイト以上のデータが入っていることを確認できているため、
				// ReadFile() はブロックすることはないため、一括して読む。
				if (PReadFile(cv->ComID,&(cv->InBuff[cv->InBuffCount]),
				              InBuffSize-cv->InBuffCount,&C,NULL)) {
					if (C == 0) {
						DErr = ERROR_HANDLE_EOF;
					}
					else {
						cv->InBuffCount = cv->InBuffCount + C;
					}
				}
				else {
					DErr = GetLastError();
				}

				// 1バイト以上読めたら、イベントを起こし、スレッドを再開させる。
				if (cv->InBuffCount > 0) {
					cv->RRQ = FALSE;
					SetEvent(ReadEnd);
				}
				break;
		}
	}

	if (cv->InBuffCount==0) {
		switch (cv->PortType) {
			case IdTCPIP:
				if (! TCPIPClosed) {
					PWSAAsyncSelect(cv->s,cv->HWin, WM_USER_COMMNOTIFY,
					                FD_READ | FD_OOB | FD_CLOSE);
				}
				break;
			case IdSerial:
				cv->RRQ = FALSE;
				SetEvent(ReadEnd);
				return;
			case IdFile:
				if (DErr != ERROR_IO_PENDING) {
					PostMessage(cv->HWin, WM_USER_COMMNOTIFY, 0, FD_CLOSE);
					cv->RRQ = FALSE;
				}
				else {
					cv->RRQ = TRUE;
				}
				return;
			case IdNamedPipe:
				// TODO: たぶん、ここに来ることはない。
				if (DErr != ERROR_IO_PENDING) {
					PostMessage(cv->HWin, WM_USER_COMMNOTIFY, 0, FD_CLOSE);
					cv->RRQ = FALSE;
				}
				else {
					cv->RRQ = TRUE;
				}
				SetEvent(ReadEnd);
				return;
		}
		cv->RRQ = FALSE;
	}
}

void CommSend(PComVar cv)
{
	int delay;
	COMSTAT Stat;
	BYTE LineEnd;
	int C, D, Max;
	DWORD DErr;

	if ((! cv->Open) || (! cv->Ready)) {
		cv->OutBuffCount = 0;
		return;
	}

	if ((cv->OutBuffCount == 0) || (! cv->CanSend)) {
		return;
	}

	/* Max num of bytes to be written */
	switch (cv->PortType) {
		case IdTCPIP:
			if (TCPIPClosed) {
				cv->OutBuffCount = 0;
			}
			Max = cv->OutBuffCount;
			break;
		case IdSerial:
			ClearCommError(cv->ComID,&DErr,&Stat);
			Max = OutBuffSize - Stat.cbOutQue;
			break;
		case IdFile:
			Max = cv->OutBuffCount;
			break;
		case IdNamedPipe:
			Max = cv->OutBuffCount;
			break;
	}

	if ( Max<=0 ) {
		return;
	}
	if ( Max > cv->OutBuffCount ) {
		Max = cv->OutBuffCount;
	}

	if (cv->PortType == IdTCPIP && cv->TelFlag) {
		cv->LastSendTime = time(NULL);
	}

	C = Max;
	delay = 0;

	if ( cv->DelayFlag && (cv->PortType==IdSerial) ) {
		if ( cv->DelayPerLine > 0 ) {
			if ( cv->CRSend==IdCR ) {
				LineEnd = 0x0d;
			}
			else { // CRLF or LF
				LineEnd = 0x0a;
			}
			C = 1;
			if ( cv->DelayPerChar==0 ) {
				while ((C<Max) && (cv->OutBuff[cv->OutPtr+C-1]!=LineEnd)) {
					C++;
				}
			}
			if ( cv->OutBuff[cv->OutPtr+C-1]==LineEnd ) {
				delay = cv->DelayPerLine;
			}
			else {
				delay = cv->DelayPerChar;
			}
		}
		else if ( cv->DelayPerChar > 0 ) {
			C = 1;
			delay = cv->DelayPerChar;
		}
	}

	/* Write to comm driver/Winsock */
	switch (cv->PortType) {
		case IdTCPIP:
			D = Psend(cv->s, &(cv->OutBuff[cv->OutPtr]), C, 0);
			if ( D==SOCKET_ERROR ) { /* if error occurs */
				PWSAGetLastError(); /* Clear error */
				D = 0;
			}
			break;

		case IdSerial:
			if (! PWriteFile(cv->ComID,&(cv->OutBuff[cv->OutPtr]),C,(LPDWORD)&D,&wol)) {
				if (GetLastError() == ERROR_IO_PENDING) {
					if (WaitForSingleObject(wol.hEvent,1000) != WAIT_OBJECT_0) {
						D = C; /* Time out, ignore data */
					}
					else {
						GetOverlappedResult(cv->ComID,&wol,(LPDWORD)&D,FALSE);
					}
				}
				else { /* I/O error */
					D = C; /* ignore error */
				}
			}
			ClearCommError(cv->ComID,&DErr,&Stat);
			break;

		case IdFile:
			if (! PWriteFile(cv->ComID, &(cv->OutBuff[cv->OutPtr]), C, (LPDWORD)&D, NULL)) {
				if (! GetLastError() == ERROR_IO_PENDING) {
					D = C; /* ignore data */
				}
			}
			break;

		case IdNamedPipe:
			if (! PWriteFile(cv->ComID, &(cv->OutBuff[cv->OutPtr]), C, (LPDWORD)&D, NULL)) {
				// ERROR_IO_PENDING 以外のエラーだったら、パイプがクローズされているかもしれないが、
				// 送信できたことにする。
				if (! (GetLastError() == ERROR_IO_PENDING)) {
					D = C; /* ignore data */
				}
			}
			break;
	}

	cv->OutBuffCount = cv->OutBuffCount - D;
	if ( cv->OutBuffCount==0 ) {
		cv->OutPtr = 0;
	}
	else {
		cv->OutPtr = cv->OutPtr + D;
	}

	if ( (C==D) && (delay>0) ) {
		cv->CanSend = FALSE;
		SetTimer(cv->HWin, IdDelayTimer, delay, NULL);
	}
}

void CommSendBreak(PComVar cv, int msec)
/* for only serial ports */
{
	MSG DummyMsg;

	if ( ! cv->Ready ) {
		return;
	}

	switch (cv->PortType) {
		case IdSerial:
			/* Set com port into a break state */
			SetCommBreak(cv->ComID);

			/* pause for 1 sec */
			if (SetTimer(cv->HWin, IdBreakTimer, msec, NULL) != 0) {
				GetMessage(&DummyMsg,cv->HWin,WM_TIMER,WM_TIMER);
			}

			/* Set com port into the nonbreak state */
			ClearCommBreak(cv->ComID);
			break;
	}
}

void CommLock(PTTSet ts, PComVar cv, BOOL Lock)
{
	BYTE b;
	DWORD Func;

	if (! cv->Ready) {
		return;
	}
	if ((cv->PortType==IdTCPIP) ||
	    (cv->PortType==IdSerial) &&
	    (ts->Flow!=IdFlowHard)) {
		if (Lock) {
			b = XOFF;
		}
		else {
			b = XON;
		}
		CommBinaryOut(cv,&b,1);
	}
	else if ((cv->PortType==IdSerial) &&
	         (ts->Flow==IdFlowHard)) {
		if (Lock) {
			Func = CLRRTS;
		}
		else {
			Func = SETRTS;
		}
		EscapeCommFunction(cv->ComID,Func);
	}
}

BOOL PrnOpen(PCHAR DevName)
{
	char Temp[MAXPATHLEN], *c;
	DCB dcb;
	DWORD DErr;
	COMMTIMEOUTS ctmo;

	strncpy_s(Temp, sizeof(Temp),DevName, _TRUNCATE);
	c = Temp;
	while (*c != '\0' && *c != ':') {
		c++;
	}
	*c = '\0';
	LPTFlag = (Temp[0]=='L') ||
	          (Temp[0]=='l');

	if (IsWindowsNTKernel()) {
		// ネットワーク共有にマップされたデバイスが相手の場合、こうしないといけないらしい (2011.01.25 maya)
		// http://logmett.com/forum/viewtopic.php?f=2&t=1383
		// http://msdn.microsoft.com/en-us/library/aa363858(v=vs.85).aspx#5
		PrnID = CreateFile(Temp,GENERIC_WRITE | FILE_READ_ATTRIBUTES,
		                   FILE_SHARE_READ,NULL,CREATE_ALWAYS,
		                   0,NULL);
	}
	else {
		// 9x では上記のコードでうまくいかないので従来通りの処理
		PrnID = CreateFile(Temp,GENERIC_WRITE,
		                   0,NULL,OPEN_EXISTING,
		                   0,NULL);
	}

	if (PrnID == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	if (GetCommState(PrnID,&dcb)) {
		BuildCommDCB(DevName,&dcb);
		SetCommState(PrnID,&dcb);
	}
	ClearCommError(PrnID,&DErr,NULL);
	if (! LPTFlag) {
		SetupComm(PrnID,0,CommOutQueSize);
	}
	/* flush output buffer */
	PurgeComm(PrnID, PURGE_TXABORT | PURGE_TXCLEAR);
	memset(&ctmo,0,sizeof(ctmo));
	ctmo.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts(PrnID,&ctmo);
	if (! LPTFlag) {
		EscapeCommFunction(PrnID,SETDTR);
	}
	return TRUE;
}

int PrnWrite(PCHAR b, int c)
{
	int d;
	DWORD DErr;
	COMSTAT Stat;

	if (PrnID == INVALID_HANDLE_VALUE ) {
		return c;
	}

	ClearCommError(PrnID,&DErr,&Stat);
	if (! LPTFlag &&
		(OutBuffSize - (int)Stat.cbOutQue < c)) {
		c = OutBuffSize - Stat.cbOutQue;
	}
	if (c<=0) {
		return 0;
	}
	if (! WriteFile(PrnID,b,c,(LPDWORD)&d,NULL)) {
		d = 0;
	}
	ClearCommError(PrnID,&DErr,NULL);
	return d;
}

void PrnCancel()
{
	PurgeComm(PrnID, PURGE_TXABORT | PURGE_TXCLEAR);
	PrnClose();
}

void PrnClose()
{
	if (PrnID != INVALID_HANDLE_VALUE) {
		if (!LPTFlag) {
			EscapeCommFunction(PrnID,CLRDTR);
		}
		CloseHandle(PrnID);
	}
	PrnID = INVALID_HANDLE_VALUE;
}
