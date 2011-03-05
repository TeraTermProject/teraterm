/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 Copyright(C) 2010 Tera Term Project
 All rights reserved. */

/*
 "ZMODEM.LOG"の見方：
 　"B"の直後の二桁が Header Type を示す。

2A 2A 18 42 30 31 30 30 30 30 30 30 32 33 62 65     **.B0100000023be
35 30 0D 8A 11 
   ↓
2A 2A 18 42 30 31 30 30 30 30 30 30 32 33 62 65     **.B0100000023be
^^^^^^^^ZPAD+ZPAD+ZDLE 
         ^^ZHEX
            ^^^^ZRINIT（2桁で表す）
35 30 0D 8A 11 
^^^^^CRC
 */

/* TTFILE.DLL, ZMODEM protocol */
#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include <stdio.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "tt_res.h"

#include "dlglib.h"
#include "ftlib.h"
#include "ttcommon.h"
#include "ttlib.h"

#define NormalTimeOut 10
#define TCPIPTimeOut 0
#define IniTimeOut 10
#define FinTimeOut 3

#define ZPAD   '*'
#define ZDLE   0x18
#define ZDLEE  0x58
#define ZBIN   'A'
#define ZHEX   'B'
#define ZBIN32 'C'

#define ZRQINIT 0
#define ZRINIT	1
#define ZSINIT	2
#define ZACK	3
#define ZFILE	4
#define ZSKIP	5
#define ZNAK	6
#define ZABORT	7
#define ZFIN	8
#define ZRPOS	9
#define ZDATA	10
#define ZEOF	11
#define ZFERR	12
#define ZCRC	13
#define ZCHALLENGE 14
#define ZCOMPL	15
#define ZCAN	16
#define ZFREECNT 17
#define ZCOMMAND 18
#define ZSTDERR 19

#define ZCRCE 'h'
#define ZCRCG 'i'
#define ZCRCQ 'j'
#define ZCRCW 'k'
#define ZRUB0 'l'
#define ZRUB1 'm'

#define ZF0 3
#define ZF1 2
#define ZF2 1
#define ZF3 0
#define ZP0 0
#define ZP1 1
#define ZP2 2
#define ZP3 3

#define CANFDX	0x01
#define CANOVIO 0x02
#define CANBRK	0x04
#define CANCRY	0x08
#define CANLZW	0x10
#define CANFC32 0x20
#define ESCCTL	0x40
#define ESC8	0x80

#define ZCBIN	1
#define ZCNL	2

/* ログファイル用バッファ */
#define LOGBUFSIZE 256

static char recvbuf[LOGBUFSIZE];
static char sendbuf[LOGBUFSIZE];

static void add_recvbuf(char *fmt, ...)
{
	va_list arg;
	char buf[128];

	va_start(arg, fmt);
	vsnprintf(buf, sizeof(buf), fmt, arg);
	strncat_s(recvbuf, sizeof(recvbuf), buf, _TRUNCATE);
	va_end(arg);
}


static void show_recvbuf(PFileVar fv)
{
	char *s;

	s = recvbuf;
	strncat_s(recvbuf, sizeof(recvbuf), "\015\012", _TRUNCATE);
	_lwrite(fv->LogFile, s, strlen(s));

	memset(recvbuf, 0, sizeof(recvbuf));
}

static void add_sendbuf(char *fmt, ...)
{
	va_list arg;
	char buf[128];

	va_start(arg, fmt);
	vsnprintf(buf, sizeof(buf), fmt, arg);
	strncat_s(sendbuf, sizeof(sendbuf), buf, _TRUNCATE);
	va_end(arg);
}

static void show_sendbuf(PFileVar fv)
{
	char *s;

	s = sendbuf;
	strncat_s(sendbuf, sizeof(sendbuf), "\015\012", _TRUNCATE);
	_lwrite(fv->LogFile, s, strlen(s));

	memset(sendbuf, 0, sizeof(sendbuf));
}

static char *hdrtype_name(int type)
{
	static char *s[] = {
		"ZRQINIT",
		"ZRINIT",
		"ZSINIT",
		"ZACK",
		"ZFILE",
		"ZSKIP",
		"ZNAK",
		"ZABORT",
		"ZFIN",
		"ZRPOS",
		"ZDATA",
		"ZEOF",
		"ZFERR",
		"ZCRC",
		"ZCHALLENGE",
		"ZCOMPL",
		"ZCAN",
		"ZFREECNT",
		"ZCOMMAND",
		"ZSTDERR",
	};

	if (type >= ZRQINIT && type <= ZSTDERR)
		return (s[type]);
	else
		return NULL;
}

int ZRead1Byte(PFileVar fv, PZVar zv, PComVar cv, LPBYTE b)
{
	char *s;

	if (CommRead1Byte(cv, b) == 0)
		return 0;

	if (fv->LogFlag) {
		if (fv->LogState == 0) {
			// 残りのASCII表示を行う
			fv->FlushLogLineBuf = 1;
			FTLog1Byte(fv, 0);
			fv->FlushLogLineBuf = 0;

			show_sendbuf(fv);

			fv->LogState = 1;
			fv->LogCount = 0;
			s = "\015\012<<< Received\015\012";
			_lwrite(fv->LogFile, s, strlen(s));
		}
		FTLog1Byte(fv, *b);
	}
	/* ignore 0x11, 0x13, 0x81 and 0x83 */
	if (((*b & 0x7F) == 0x11) || ((*b & 0x7F) == 0x13))
		return 0;
	return 1;
}

int ZWrite(PFileVar fv, PZVar zv, PComVar cv, PCHAR B, int C)
{
	int i, j;
	char *s;

	i = CommBinaryOut(cv, B, C);

	if (fv->LogFlag && (i > 0)) {
		if (fv->LogState != 0) {
			// 残りのASCII表示を行う
			fv->FlushLogLineBuf = 1;
			FTLog1Byte(fv, 0);
			fv->FlushLogLineBuf = 0;

			show_recvbuf(fv);

			fv->LogState = 0;
			fv->LogCount = 0;
			s = "\015\012Sending >>>\015\012";
			_lwrite(fv->LogFile, s, strlen(s));
		}
		for (j = 0; j <= i - 1; j++)
			FTLog1Byte(fv, B[j]);
	}
	return i;
}

void ZPutHex(PZVar zv, int *i, BYTE b)
{
	if (b <= 0x9f)
		zv->PktOut[*i] = (b >> 4) + 0x30;
	else
		zv->PktOut[*i] = (b >> 4) + 0x57;

	(*i)++;
	if ((b & 0x0F) <= 0x09)
		zv->PktOut[*i] = (b & 0x0F) + 0x30;
	else
		zv->PktOut[*i] = (b & 0x0F) + 0x57;

	(*i)++;
}

void ZShHdr(PZVar zv, BYTE HdrType)
{
	int i;

	zv->PktOut[0] = ZPAD;
	zv->PktOut[1] = ZPAD;
	zv->PktOut[2] = ZDLE;
	zv->PktOut[3] = ZHEX;
	zv->PktOutCount = 4;
	ZPutHex(zv, &(zv->PktOutCount), HdrType);
	zv->CRC = UpdateCRC(HdrType, 0);
	for (i = 0; i <= 3; i++) {
		ZPutHex(zv, &(zv->PktOutCount), zv->TxHdr[i]);
		zv->CRC = UpdateCRC(zv->TxHdr[i], zv->CRC);
	}
	ZPutHex(zv, &(zv->PktOutCount), HIBYTE(zv->CRC));
	ZPutHex(zv, &(zv->PktOutCount), LOBYTE(zv->CRC));
	zv->PktOut[zv->PktOutCount] = 0x8D;
	zv->PktOutCount++;
	zv->PktOut[zv->PktOutCount] = 0x8A;
	zv->PktOutCount++;

	if ((HdrType != ZFIN) && (HdrType != ZACK)) {
		zv->PktOut[zv->PktOutCount] = XON;
		zv->PktOutCount++;
	}

	zv->PktOutPtr = 0;
	zv->Sending = TRUE;

	add_sendbuf("%s: %s", __FUNCTION__, hdrtype_name(HdrType));
#if 0
	if (HdrType == ZRPOS) {
		LONG pos;
		memcpy(&pos, zv->TxHdr[ZP0], 4);
		add_sendbuf(" offset=%ld", pos);
	}
#endif
}

void ZPutBin(PZVar zv, int *i, BYTE b)
{
	switch (b) {
	case 0x0D:
	case 0x8D:
		/* if (zv->CtlEsc ||
		   ((zv->LastSent & 0x7f) == '@')) */
//  {
		zv->PktOut[*i] = ZDLE;
		(*i)++;
		b = b ^ 0x40;
//  }
		break;
	case 0x10:
	case 0x11:
	case 0x13:
	case ZDLE:
	case 0x90:
	case 0x91:
	case 0x93:
		zv->PktOut[*i] = ZDLE;
		(*i)++;
		b = b ^ 0x40;
		break;
	default:
		if (zv->CtlEsc && ((b & 0x60) == 0)) {
			zv->PktOut[*i] = ZDLE;
			(*i)++;
			b = b ^ 0x40;
		}
	}
	zv->LastSent = b;
	zv->PktOut[*i] = b;
	(*i)++;
}

void ZSbHdr(PZVar zv, BYTE HdrType)
{
	int i;

	zv->PktOut[0] = ZPAD;
	zv->PktOut[1] = ZDLE;
	zv->PktOut[2] = ZBIN;
	zv->PktOutCount = 3;
	ZPutBin(zv, &(zv->PktOutCount), HdrType);
	zv->CRC = UpdateCRC(HdrType, 0);
	for (i = 0; i <= 3; i++) {
		ZPutBin(zv, &(zv->PktOutCount), zv->TxHdr[i]);
		zv->CRC = UpdateCRC(zv->TxHdr[i], zv->CRC);
	}
	ZPutBin(zv, &(zv->PktOutCount), HIBYTE(zv->CRC));
	ZPutBin(zv, &(zv->PktOutCount), LOBYTE(zv->CRC));

	zv->PktOutPtr = 0;
	zv->Sending = TRUE;

	add_sendbuf("%s: %s ", __FUNCTION__, hdrtype_name(HdrType));
}

void ZStoHdr(PZVar zv, LONG Pos)
{
	zv->TxHdr[ZP0] = LOBYTE(LOWORD(Pos));
	zv->TxHdr[ZP1] = HIBYTE(LOWORD(Pos));
	zv->TxHdr[ZP2] = LOBYTE(HIWORD(Pos));
	zv->TxHdr[ZP3] = HIBYTE(HIWORD(Pos));
}


LONG ZRclHdr(PZVar zv)
{
	LONG L;

	L = (BYTE) (zv->RxHdr[ZP3]);
	L = (L << 8) + (BYTE) (zv->RxHdr[ZP2]);
	L = (L << 8) + (BYTE) (zv->RxHdr[ZP1]);
	return ((L << 8) + (BYTE) (zv->RxHdr[ZP0]));
}

void ZSendRInit(PFileVar fv, PZVar zv)
{
	zv->Pos = 0;
	ZStoHdr(zv, 0);
	zv->TxHdr[ZF0] = /* CANFC32 | */ CANFDX | CANOVIO;
	if (zv->CtlEsc)
		zv->TxHdr[ZF0] = zv->TxHdr[ZF0] | ESCCTL;
	ZShHdr(zv, ZRINIT);
	FTSetTimeOut(fv, IniTimeOut);
}

void ZSendRQInit(PFileVar fv, PZVar zv, PComVar cv)
{
	ZStoHdr(zv, 0);
	ZShHdr(zv, ZRQINIT);
}

void ZSendRPOS(PFileVar fv, PZVar zv)
{
	ZStoHdr(zv, zv->Pos);
	ZShHdr(zv, ZRPOS);
	FTSetTimeOut(fv, zv->TimeOut);
}

void ZSendACK(PFileVar fv, PZVar zv)
{
	ZStoHdr(zv, 0);
	ZShHdr(zv, ZACK);
	FTSetTimeOut(fv, zv->TimeOut);
}

void ZSendNAK(PZVar zv)
{
	ZStoHdr(zv, 0);
	ZShHdr(zv, ZNAK);
}

void ZSendEOF(PZVar zv)
{
	ZStoHdr(zv, zv->Pos);
	ZShHdr(zv, ZEOF);
	zv->ZState = Z_SendEOF;
}

void ZSendFIN(PZVar zv)
{
	ZStoHdr(zv, 0);
	ZShHdr(zv, ZFIN);
}

void ZSendCancel(PZVar zv)
{
	int i;

	for (i = 0; i <= 7; i++)
		zv->PktOut[i] = ZDLE;
	for (i = 8; i <= 17; i++)
		zv->PktOut[i] = 0x08;
	zv->PktOutCount = 18;
	zv->PktOutPtr = 0;
	zv->Sending = TRUE;
	zv->ZState = Z_Cancel;

	add_sendbuf("%s: ", __FUNCTION__);
}

void ZSendInitHdr(PZVar zv)
{
	ZStoHdr(zv, 0);
	if (zv->CtlEsc)
		zv->TxHdr[ZF0] = ESCCTL;
	ZShHdr(zv, ZSINIT);
	zv->ZState = Z_SendInitHdr;
}

void ZSendInitDat(PZVar zv)
{
	zv->CRC = 0;
	zv->PktOutCount = 0;
	ZPutBin(zv, &(zv->PktOutCount), 0);
	zv->CRC = UpdateCRC(0, zv->CRC);

	zv->PktOut[zv->PktOutCount] = ZDLE;
	zv->PktOutCount++;
	zv->PktOut[zv->PktOutCount] = ZCRCW;
	zv->PktOutCount++;
	zv->CRC = UpdateCRC(ZCRCW, zv->CRC);

	ZPutBin(zv, &(zv->PktOutCount), HIBYTE(zv->CRC));
	ZPutBin(zv, &(zv->PktOutCount), LOBYTE(zv->CRC));

	zv->PktOutPtr = 0;
	zv->Sending = TRUE;
	zv->ZState = Z_SendInitDat;

	add_sendbuf("%s: ", __FUNCTION__);
}

void ZSendFileHdr(PZVar zv)
{
	ZStoHdr(zv, 0);
	if (zv->BinFlag)
		zv->TxHdr[ZF0] = ZCBIN;	/* binary file */
	else
		zv->TxHdr[ZF0] = ZCNL;	/* text file, convert newline */
	ZSbHdr(zv, ZFILE);
	zv->ZState = Z_SendFileHdr;
}

void ZSendFileDat(PFileVar fv, PZVar zv)
{
	int i, j;
	struct _stat st;

	if (!fv->FileOpen) {
		ZSendCancel(zv);
		return;
	}
	SetDlgItemText(fv->HWin, IDC_PROTOFNAME, &(fv->FullName[fv->DirLen]));

	/* file name */
	strncpy_s(zv->PktOut, sizeof(zv->PktOut), &(fv->FullName[fv->DirLen]),
			  _TRUNCATE);
	FTConvFName(zv->PktOut);	// replace ' ' by '_' in FName
	zv->PktOutCount = strlen(zv->PktOut);
	zv->CRC = 0;
	for (i = 0; i <= zv->PktOutCount - 1; i++)
		zv->CRC = UpdateCRC(zv->PktOut[i], zv->CRC);
	ZPutBin(zv, &(zv->PktOutCount), 0);
	zv->CRC = UpdateCRC(0, zv->CRC);
	/* file size */
	fv->FileSize = GetFSize(fv->FullName);

	/* timestamp */
	_stat(fv->FullName, &st);

	// ファイルのタイムスタンプとパーミッションも送るようにした。(2007.12.20 maya, yutaka)
	_snprintf_s(&(zv->PktOut[zv->PktOutCount]),
				sizeof(zv->PktOut) - zv->PktOutCount, _TRUNCATE,
				"%lu %lo %o", fv->FileSize, (long) st.st_mtime,
				0644 | _S_IFREG);
	j = strlen(&(zv->PktOut[zv->PktOutCount])) - 1;
	for (i = 0; i <= j; i++) {
		zv->CRC = UpdateCRC(zv->PktOut[zv->PktOutCount], zv->CRC);
		zv->PktOutCount++;
	}

	ZPutBin(zv, &(zv->PktOutCount), 0);
	zv->CRC = UpdateCRC(0, zv->CRC);
	zv->PktOut[zv->PktOutCount] = ZDLE;
	zv->PktOutCount++;
	zv->PktOut[zv->PktOutCount] = ZCRCW;
	zv->PktOutCount++;
	zv->CRC = UpdateCRC(ZCRCW, zv->CRC);

	ZPutBin(zv, &(zv->PktOutCount), HIBYTE(zv->CRC));
	ZPutBin(zv, &(zv->PktOutCount), LOBYTE(zv->CRC));

	zv->PktOutPtr = 0;
	zv->Sending = TRUE;
	zv->ZState = Z_SendFileDat;

	fv->ByteCount = 0;
	fv->ProgStat = 0;
	SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
	SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
				  fv->ByteCount, fv->FileSize, &fv->ProgStat);

	add_sendbuf("%s: ZFILE: ZF0=%x ZF1=%x ZF2=%x file=%s size=%lu",
		__FUNCTION__,
		zv->TxHdr[ZF0], zv->TxHdr[ZF1],zv->TxHdr[ZF2],
		&(fv->FullName[fv->DirLen]), fv->FileSize);
}

void ZSendDataHdr(PZVar zv)
{
	ZStoHdr(zv, zv->Pos);
	ZSbHdr(zv, ZDATA);
	zv->ZState = Z_SendDataHdr;
}

void ZSendDataDat(PFileVar fv, PZVar zv)
{
	int c;
	BYTE b;

	if (zv->Pos >= fv->FileSize) {
		zv->Pos = fv->FileSize;
		ZSendEOF(zv);
		return;
	}

	fv->ByteCount = zv->Pos;

	if (fv->FileOpen && (zv->Pos < fv->FileSize))
		_llseek(fv->FileHandle, zv->Pos, 0);

	zv->CRC = 0;
	zv->PktOutCount = 0;
	do {
		c = _lread(fv->FileHandle, &b, 1);
		if (c > 0) {
			ZPutBin(zv, &(zv->PktOutCount), b);
			zv->CRC = UpdateCRC(b, zv->CRC);
			fv->ByteCount++;
		}
	} while ((c != 0) && (zv->PktOutCount <= zv->MaxDataLen - 2));

	SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
	SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
				  fv->ByteCount, fv->FileSize, &fv->ProgStat);
	zv->Pos = fv->ByteCount;

	zv->PktOut[zv->PktOutCount] = ZDLE;
	zv->PktOutCount++;
	if (zv->Pos >= fv->FileSize)
		b = ZCRCE;
	else if ((zv->WinSize >= 0) && (zv->Pos - zv->LastPos > zv->WinSize))
		b = ZCRCQ;
	else
		b = ZCRCG;
	zv->PktOut[zv->PktOutCount] = b;
	zv->PktOutCount++;
	zv->CRC = UpdateCRC(b, zv->CRC);

	ZPutBin(zv, &(zv->PktOutCount), HIBYTE(zv->CRC));
	ZPutBin(zv, &(zv->PktOutCount), LOBYTE(zv->CRC));

	zv->PktOutPtr = 0;
	zv->Sending = TRUE;
	if (b == ZCRCQ)
		zv->ZState = Z_SendDataDat2;	/* wait response from receiver */
	else
		zv->ZState = Z_SendDataDat;

	add_sendbuf("%s: ", __FUNCTION__);
}

void ZInit(PFileVar fv, PZVar zv, PComVar cv, PTTSet ts) {
	int Max;
	char uimsg[MAX_UIMSG];

	zv->CtlEsc = ((ts->FTFlag & FT_ZESCCTL) != 0);
	zv->MaxDataLen = ts->ZmodemDataLen;
	zv->WinSize = ts->ZmodemWinSize;
	fv->LogFlag = ((ts->LogFlag & LOG_Z) != 0);

	if (zv->ZMode == IdZAuto) {
		CommInsert1Byte(cv, 'B');
		CommInsert1Byte(cv, ZDLE);
		CommInsert1Byte(cv, ZPAD);
		zv->ZMode = IdZReceive;
	}

	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption), "Tera Term: ",
			  _TRUNCATE);
	switch (zv->ZMode) {
	case IdZSend:
		get_lang_msg("FILEDLG_TRANS_TITLE_ZSEND", uimsg, sizeof(uimsg),
					 TitZSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg,
				  _TRUNCATE);
		break;
	case IdZReceive:
		get_lang_msg("FILEDLG_TRANS_TITLE_ZRCV", uimsg, sizeof(uimsg),
					 TitZRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg,
				  _TRUNCATE);
		break;
	}

	SetWindowText(fv->HWin, fv->DlgCaption);
	SetDlgItemText(fv->HWin, IDC_PROTOPROT, "ZMODEM");

	InitDlgProgress(fv->HWin, IDC_PROTOPROGRESS, &fv->ProgStat);

	fv->FileSize = 0;

	zv->PktOutCount = 0;
	zv->Pos = 0;
	zv->LastPos = 0;
	zv->ZPktState = Z_PktGetPAD;
	zv->Sending = FALSE;
	zv->LastSent = 0;
	zv->CanCount = 5;

	if (zv->MaxDataLen <= 0)
		zv->MaxDataLen = 1024;
	if (zv->MaxDataLen < 64)
		zv->MaxDataLen = 64;

	/* Time out & Max block size */
	if (cv->PortType == IdTCPIP) {
		zv->TimeOut = TCPIPTimeOut;
		Max = 1024;
	} else {
		zv->TimeOut = NormalTimeOut;
		if (ts->Baud <= 110) {
			Max = 64;
		}
		else if (ts->Baud <= 300) {
			Max = 128;
		}
		else if (ts->Baud <= 1200) {
			Max = 256;
		}
		else if (ts->Baud <= 2400) {
			Max = 512;
		}
		else {
			Max = 1024;
		}
	}
	if (zv->MaxDataLen > Max)
		zv->MaxDataLen = Max;

	if (fv->LogFlag)
		fv->LogFile = _lcreat("ZMODEM.LOG", 0);
	fv->LogState = 0;
	fv->LogCount = 0;

	switch (zv->ZMode) {
	case IdZReceive:
		zv->ZState = Z_RecvInit;
		ZSendRInit(fv, zv);
		break;
	case IdZSend:
		zv->ZState = Z_SendInit;

		// ファイル送信開始前に、"rz"を自動的に呼び出す。(2007.12.21 yutaka)
		if (ts->ZModemRcvCommand[0] != '\0') {
			ZWrite(fv, zv, cv, ts->ZModemRcvCommand,
				   strlen(ts->ZModemRcvCommand));
			ZWrite(fv, zv, cv, "\015", 1);
		}

		ZSendRQInit(fv, zv, cv);
		break;
	}
}

void ZTimeOutProc(PFileVar fv, PZVar zv, PComVar cv)
{
	switch (zv->ZState) {
	case Z_RecvInit:
		ZSendRInit(fv, zv);
		break;
	case Z_RecvInit2:
		ZSendACK(fv, zv);		/* Ack for ZSINIT */
		break;
	case Z_RecvData:
		ZSendRPOS(fv, zv);
		break;
	case Z_RecvFIN:
		zv->ZState = Z_End;
		break;
	}
}

BOOL ZCheckHdr(PFileVar fv, PZVar zv)
{
	int i;
	BOOL Ok;

	if (zv->CRC32) {
		zv->CRC3 = 0xFFFFFFFF;
		for (i = 0; i <= 8; i++)
			zv->CRC3 = UpdateCRC32(zv->PktIn[i], zv->CRC3);
		Ok = zv->CRC3 == 0xDEBB20E3;
	} else {
		zv->CRC = 0;
		for (i = 0; i <= 6; i++)
			zv->CRC = UpdateCRC(zv->PktIn[i], zv->CRC);
		Ok = zv->CRC == 0;
	}

	if (!Ok) {
		switch (zv->ZState) {
		case Z_RecvInit:
			ZSendRInit(fv, zv);
			break;
		case Z_RecvData:
			ZSendRPOS(fv, zv);
			break;
		}
	}
	zv->RxType = zv->PktIn[0];
	for (i = 1; i <= 4; i++)
		zv->RxHdr[i - 1] = zv->PktIn[i];

	return Ok;
}

void ZParseRInit(PFileVar fv, PZVar zv)
{
	int Max;

	if ((zv->ZState != Z_SendInit) && (zv->ZState != Z_SendEOF))
		return;

	if (fv->FileOpen)			// close previous file
	{
		_lclose(fv->FileHandle);
		fv->FileOpen = FALSE;
	}

	if (!GetNextFname(fv)) {
		zv->ZState = Z_SendFIN;
		ZSendFIN(zv);
		return;
	}

	/* file open */
	fv->FileHandle = _lopen(fv->FullName, OF_READ);
	fv->FileOpen = fv->FileHandle > 0;

	if (zv->CtlEsc) {
		if ((zv->RxHdr[ZF0] & ESCCTL) == 0) {
			zv->ZState = Z_SendInitHdr;
			ZSendInitHdr(zv);
			return;
		}
	} else
		zv->CtlEsc = (zv->RxHdr[ZF0] & ESCCTL) != 0;

	Max = (zv->RxHdr[ZP1] << 8) + zv->RxHdr[ZP0];
	if (Max <= 0)
		Max = 1024;
	if (zv->MaxDataLen > Max)
		zv->MaxDataLen = Max;

	zv->ZState = Z_SendFileHdr;
	ZSendFileHdr(zv);
}

BOOL ZParseSInit(PZVar zv)
{
	if (zv->ZState != Z_RecvInit)
		return FALSE;
	zv->ZState = Z_RecvInit2;
	zv->CtlEsc = zv->CtlEsc || ((zv->RxHdr[ZF0] & ESCCTL) != 0);
	return TRUE;
}

void ZParseHdr(PFileVar fv, PZVar zv, PComVar cv)
{
	add_recvbuf("%s: RxType %s ", __FUNCTION__, hdrtype_name(zv->RxType));

	switch (zv->RxType) {
	case ZRQINIT:
		if (zv->ZState == Z_RecvInit)
			ZSendRInit(fv, zv);
		break;
	case ZRINIT:
		ZParseRInit(fv, zv);
		break;
	case ZSINIT:
		zv->ZPktState = Z_PktGetData;
		if (zv->ZState == Z_RecvInit)
			FTSetTimeOut(fv, IniTimeOut);
		break;
	case ZACK:
		switch (zv->ZState) {
		case Z_SendInitDat:
			ZSendFileHdr(zv);
			break;
		case Z_SendDataDat2:
			zv->LastPos = ZRclHdr(zv);
			if (zv->Pos == zv->LastPos)
				ZSendDataDat(fv, zv);
			else {
				zv->Pos = zv->LastPos;
				ZSendDataHdr(zv);
			}
			break;
		}
		break;
	case ZFILE:
		zv->ZPktState = Z_PktGetData;
		if ((zv->ZState == Z_RecvInit) || (zv->ZState == Z_RecvInit2)) {
			zv->BinFlag = zv->RxHdr[ZF0] != ZCNL;
			FTSetTimeOut(fv, IniTimeOut);
		}
		break;
	case ZSKIP:
		if (fv->FileOpen) {
			_lclose(fv->FileHandle);
			// サーバ側に存在するファイルを送信しようとすると、ZParseRInit()で二重closeになるため、
			// ここでフラグを落としておく。 (2007.12.20 yutaka)
			fv->FileOpen = FALSE;
		}
		ZStoHdr(zv, 0);
		if (zv->CtlEsc)
			zv->RxHdr[ZF0] = ESCCTL;
		zv->ZState = Z_SendInit;
		ZParseRInit(fv, zv);
		break;
	case ZNAK:
		switch (zv->ZState) {
		case Z_SendInitHdr:
		case Z_SendInitDat:
			ZSendInitHdr(zv);
			break;
		case Z_SendFileHdr:
		case Z_SendFileDat:
			ZSendFileHdr(zv);
			break;
		}
		break;
	case ZABORT:
	case ZFERR:
		if (zv->ZMode == IdZSend) {
			zv->ZState = Z_SendFIN;
			ZSendFIN(zv);
		}
		break;
	case ZFIN:
		fv->Success = TRUE;
		if (zv->ZMode == IdZReceive) {
			zv->ZState = Z_RecvFIN;
			ZSendFIN(zv);
			zv->CanCount = 2;
			FTSetTimeOut(fv, FinTimeOut);
		} else {
			zv->ZState = Z_End;
			ZWrite(fv, zv, cv, "OO", 2);
		}
		break;
	case ZRPOS:
		switch (zv->ZState) {
		case Z_SendFileDat:
		case Z_SendDataHdr:
		case Z_SendDataDat:
		case Z_SendDataDat2:
		case Z_SendEOF:
			zv->Pos = ZRclHdr(zv);
			zv->LastPos = zv->Pos;
			add_recvbuf(" pos=%ld", zv->Pos);
			ZSendDataHdr(zv);
			break;
		}
		break;
	case ZDATA:
		if (zv->Pos != ZRclHdr(zv)) {
			ZSendRPOS(fv, zv);
			return;
		} else {
			FTSetTimeOut(fv, zv->TimeOut);
			zv->ZPktState = Z_PktGetData;
		}
		break;
	case ZEOF:
		if (zv->Pos != ZRclHdr(zv)) {
			ZSendRPOS(fv, zv);
			return;
		} else {
			if (fv->FileOpen) {
				if (zv->CRRecv) {
					zv->CRRecv = FALSE;
					_lwrite(fv->FileHandle, "\012", 1);
				}
				_lclose(fv->FileHandle);
				fv->FileOpen = FALSE;
			}
			zv->ZState = Z_RecvInit;
			ZSendRInit(fv, zv);
		}
		break;
	}
	zv->Quoted = FALSE;
	zv->CRC = 0;
	zv->CRC3 = 0xFFFFFFFF;
	zv->PktInPtr = 0;
	zv->PktInCount = 0;
}

BOOL ZParseFile(PFileVar fv, PZVar zv)
{
	BYTE b;
	int i, j;

	if ((zv->ZState != Z_RecvInit) && (zv->ZState != Z_RecvInit2))
		return FALSE;
	/* kill timer */
	FTSetTimeOut(fv, 0);
	zv->CRRecv = FALSE;

	/* file name */
	zv->PktIn[zv->PktInPtr] = 0;	/* for safety */

	GetFileNamePos(zv->PktIn, &i, &j);
	strncpy_s(&(fv->FullName[fv->DirLen]),
			  sizeof(fv->FullName) - fv->DirLen, &(zv->PktIn[j]),
			  _TRUNCATE);
	/* file open */
	if (!FTCreateFile(fv))
		return FALSE;

	/* file size */
	i = strlen(zv->PktIn) + 1;
	do {
		b = zv->PktIn[i];
		if ((b >= 0x30) && (b <= 0x39))
			fv->FileSize = fv->FileSize * 10 + b - 0x30;
		i++;
	} while ((b >= 0x30) && (b <= 0x39));

	zv->Pos = 0;
	fv->ByteCount = 0;
	ZStoHdr(zv, 0);
	zv->ZState = Z_RecvData;

	SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, 0);
	if (fv->FileSize > 0)
		SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
					  0, fv->FileSize, &fv->ProgStat);
	/* set timeout for data */
	FTSetTimeOut(fv, zv->TimeOut);
	return TRUE;
}

BOOL ZWriteData(PFileVar fv, PZVar zv)
{
	int i;
	BYTE b;

	if (zv->ZState != Z_RecvData)
		return FALSE;
	/* kill timer */
	FTSetTimeOut(fv, 0);

	if (zv->BinFlag)
		_lwrite(fv->FileHandle, zv->PktIn, zv->PktInPtr);
	else
		for (i = 0; i <= zv->PktInPtr - 1; i++) {
			b = zv->PktIn[i];
			if ((b == 0x0A) && (!zv->CRRecv))
				_lwrite(fv->FileHandle, "\015", 1);
			if (zv->CRRecv && (b != 0x0A))
				_lwrite(fv->FileHandle, "\012", 1);
			zv->CRRecv = b == 0x0D;
			_lwrite(fv->FileHandle, &b, 1);
		}

	fv->ByteCount = fv->ByteCount + zv->PktInPtr;
	zv->Pos = zv->Pos + zv->PktInPtr;
	ZStoHdr(zv, zv->Pos);
	SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
	if (fv->FileSize > 0)
		SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
					  fv->ByteCount, fv->FileSize, &fv->ProgStat);

	/* set timeout for data */
	FTSetTimeOut(fv, zv->TimeOut);
	return TRUE;
}

void ZCheckData(PFileVar fv, PZVar zv)
{
	BOOL Ok;

	/* check CRC */
	if (zv->CRC32 && (zv->CRC3 != 0xDEBB20E3) || (!zv->CRC32 && (zv->CRC != 0))) {	/* CRC */
		switch (zv->ZState) {
		case Z_RecvInit:
		case Z_RecvInit2:
			ZSendNAK(zv);
			break;
		case Z_RecvData:
			ZSendRPOS(fv, zv);
			break;
		}
		zv->ZPktState = Z_PktGetPAD;
		return;
	}
	/* parse data */
	switch (zv->RxType) {
	case ZSINIT:
		Ok = ZParseSInit(zv);
		break;
	case ZFILE:
		Ok = ZParseFile(fv, zv);
		break;
	case ZDATA:
		Ok = ZWriteData(fv, zv);
		break;
	default:
		Ok = FALSE;
	}

	if (!Ok) {
		zv->ZPktState = Z_PktGetPAD;
		return;
	}

	if (zv->RxType == ZFILE)
		ZShHdr(zv, ZRPOS);

	/* next state */
	switch (zv->TERM) {
	case ZCRCE:
		zv->ZPktState = Z_PktGetPAD;
		break;
	case ZCRCG:
		zv->ZPktState = Z_PktGetData;
		break;
	case ZCRCQ:
		zv->ZPktState = Z_PktGetData;
		if (zv->RxType != ZFILE)
			ZShHdr(zv, ZACK);
		break;
	case ZCRCW:
		zv->ZPktState = Z_PktGetPAD;
		if (zv->RxType != ZFILE)
			ZShHdr(zv, ZACK);
		break;
	default:
		zv->ZPktState = Z_PktGetPAD;
	}

	if (zv->ZPktState == Z_PktGetData) {
		zv->Quoted = FALSE;
		zv->CRC = 0;
		zv->CRC3 = 0xFFFFFFFF;
		zv->PktInPtr = 0;
		zv->PktInCount = 0;
	}
}

BOOL ZParse(PFileVar fv, PZVar zv, PComVar cv)
{
	BYTE b;
	int c;

	do {
		/* Send packet */
		if (zv->Sending) {
			c = 1;
			while ((c > 0) && (zv->PktOutCount > 0)) {
				c = ZWrite(fv, zv, cv, &(zv->PktOut[zv->PktOutPtr]),
						   zv->PktOutCount);
				zv->PktOutPtr = zv->PktOutPtr + c;
				zv->PktOutCount = zv->PktOutCount - c;
			}
			if (zv->PktOutCount <= 0)
				zv->Sending = FALSE;
			if ((zv->ZMode == IdZReceive) && (zv->PktOutCount > 0))
				return TRUE;
		}

		c = ZRead1Byte(fv, zv, cv, &b);
		while (c > 0) {
			if (zv->ZState == Z_RecvFIN) {
				if (b == 'O')
					zv->CanCount--;
				if (zv->CanCount <= 0) {
					zv->ZState = Z_End;
					return FALSE;
				}
			} else
				switch (b) {
				case ZDLE:
					zv->CanCount--;
					if (zv->CanCount <= 0) {
						zv->ZState = Z_End;
						return FALSE;
					}
					break;
				default:
					zv->CanCount = 5;
				}

			switch (zv->ZPktState) {
			case Z_PktGetPAD:
				switch (b) {
				case ZPAD:
					zv->ZPktState = Z_PktGetDLE;
					break;
				}
				break;
			case Z_PktGetDLE:
				switch (b) {
				case ZPAD:
					break;
				case ZDLE:
					zv->ZPktState = Z_PktHdrFrm;
					break;
				default:
					zv->ZPktState = Z_PktGetPAD;
				}
				break;
			case Z_PktHdrFrm:	/* Get header format type */
				switch (b) {
				case ZBIN:
					zv->CRC32 = FALSE;
					zv->PktInCount = 7;
					zv->ZPktState = Z_PktGetBin;
					break;
				case ZHEX:
					zv->HexLo = FALSE;
					zv->CRC32 = FALSE;
					zv->PktInCount = 7;
					zv->ZPktState = Z_PktGetHex;
					break;
				case ZBIN32:
					zv->CRC32 = TRUE;
					zv->PktInCount = 9;
					zv->ZPktState = Z_PktGetBin;
					break;
				default:
					zv->ZPktState = Z_PktGetPAD;
				}
				zv->Quoted = FALSE;
				zv->PktInPtr = 0;
				break;
			case Z_PktGetBin:
				switch (b) {
				case ZDLE:
					zv->Quoted = TRUE;
					break;
				default:
					if (zv->Quoted) {
						b = b ^ 0x40;
						zv->Quoted = FALSE;
					}
					zv->PktIn[zv->PktInPtr] = b;
					zv->PktInPtr++;
					zv->PktInCount--;
					if (zv->PktInCount == 0) {
						zv->ZPktState = Z_PktGetPAD;
						if (ZCheckHdr(fv, zv))
							ZParseHdr(fv, zv, cv);
					}
				}
				break;
			case Z_PktGetHex:
				if (b <= '9')
					b = b - 0x30;
				else if ((b >= 'a') && (b <= 'f'))
					b = b - 0x57;
				else {
					zv->ZPktState = Z_PktGetPAD;
					return TRUE;
				}

				if (zv->HexLo) {
					zv->PktIn[zv->PktInPtr] = zv->PktIn[zv->PktInPtr] + b;
					zv->HexLo = FALSE;
					zv->PktInPtr++;
					zv->PktInCount--;
					if (zv->PktInCount <= 0) {
						zv->ZPktState = Z_PktGetHexEOL;
						zv->PktInCount = 2;
					}
				} else {
					zv->PktIn[zv->PktInPtr] = b << 4;
					zv->HexLo = TRUE;
				}
				break;
			case Z_PktGetHexEOL:
				zv->PktInCount--;
				if (zv->PktInCount <= 0) {
					zv->ZPktState = Z_PktGetPAD;
					if (ZCheckHdr(fv, zv))
						ZParseHdr(fv, zv, cv);
				}
				break;
			case Z_PktGetData:
				switch (b) {
				case ZDLE:
					zv->Quoted = TRUE;
					break;
				default:
					if (zv->Quoted) {
						switch (b) {
						case ZCRCE:
						case ZCRCG:
						case ZCRCQ:
						case ZCRCW:
							zv->TERM = b;
							if (zv->CRC32)
								zv->PktInCount = 4;
							else
								zv->PktInCount = 2;
							zv->ZPktState = Z_PktGetCRC;
							break;
						case ZRUB0:
							b = 0x7F;
							break;
						case ZRUB1:
							b = 0xFF;
							break;
						default:
							b = b ^ 0x40;
						}
						zv->Quoted = FALSE;
					}
					if (zv->CRC32)
						zv->CRC3 = UpdateCRC32(b, zv->CRC3);
					else
						zv->CRC = UpdateCRC(b, zv->CRC);
					if (zv->ZPktState == Z_PktGetData) {
						if (zv->PktInPtr < 1024) {
							zv->PktIn[zv->PktInPtr] = b;
							zv->PktInPtr++;
						} else
							zv->ZPktState = Z_PktGetPAD;
					}
				}
				break;
			case Z_PktGetCRC:
				switch (b) {
				case ZDLE:
					zv->Quoted = TRUE;
					break;
				default:
					if (zv->Quoted) {
						switch (b) {
						case ZRUB0:
							b = 0x7F;
							break;
						case ZRUB1:
							b = 0xFF;
							break;
						default:
							b = b ^ 0x40;
						}
						zv->Quoted = FALSE;
					}
					if (zv->CRC32)
						zv->CRC3 = UpdateCRC32(b, zv->CRC3);
					else
						zv->CRC = UpdateCRC(b, zv->CRC);
					zv->PktInCount--;
					if (zv->PktInCount <= 0)
						ZCheckData(fv, zv);
				}
				break;
			}
			c = ZRead1Byte(fv, zv, cv, &b);
		}

		if (!zv->Sending)
			switch (zv->ZState) {
			case Z_SendInitHdr:
				ZSendInitDat(zv);
				break;
			case Z_SendFileHdr:
				ZSendFileDat(fv, zv);
				break;
			case Z_SendDataHdr:
			case Z_SendDataDat:
				ZSendDataDat(fv, zv);
				break;
			case Z_Cancel:
				zv->ZState = Z_End;
				break;
			}

		if (zv->Sending && (zv->PktOutCount > 0))
			return TRUE;
	} while (zv->Sending);

	if (zv->ZState == Z_End)
		return FALSE;
	return TRUE;
}

void ZCancel(PZVar zv)
{
	ZSendCancel(zv);
}
