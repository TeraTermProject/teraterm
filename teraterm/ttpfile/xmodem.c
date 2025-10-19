/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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

/* XMODEM protocol */
#include <stdio.h>

#include "tttypes.h"
#include "ftlib.h"
#include "protolog.h"
#include "filesys_proto.h"

#include "xmodem.h"

typedef enum {
	XpktSOH = 1,
	XpktBLK = 2,
	XpktBLK2 = 3,
	XpktDATA = 4,
} PKT_READ_MODE;

typedef enum {
	XnakNAK = 1,
	XnakC = 2
} NAK_MODE;

typedef struct {
	BYTE PktIn[1030], PktOut[1030];
	int PktBufCount, PktBufPtr;
	BYTE PktNum, PktNumSent;
	int PktNumOffset;
	PKT_READ_MODE PktReadMode;
	WORD XMode;		// IdXSend or IdXReceive
	WORD XOpt;		// XoptCheck or XoptCRC or Xopt1kCRC or Xopt1kCksum
	BOOL TextFlagConvertCRLF;
	BOOL TextFlagTrim1A;
	NAK_MODE NAKMode;
	int NAKCount;
	WORD DataLen, CheckLen;
	BOOL CRRecv;
	int TOutShort;
	int TOutLong;
	int TOutInit;
	int TOutInitCRC;
	int TOutVLong;
	int CANCount;
	TProtoLog *log;
	const char *FullName;		// Windows上のファイル名 UTF-8
	WORD LogState;

	BOOL FileOpen;
	LONG FileSize;
	LONG ByteCount;

	int ProgStat;

	DWORD StartTime;

	DWORD FileMtime;

	enum {
		STATE_FLUSH,		// 処理開始時に受信データを捨てる
		STATE_NORMAL,
		STATE_CANCELED,		// キャンセル通知を受けた
	} state;

	TComm *Comm;
	PFileVarProto fv;
	TFileIO *file;
} TXVar;
typedef TXVar *PXVar;

static int XRead1Byte(PXVar xv, LPBYTE b)
{
	if (xv->Comm->op->Read1Byte(xv->Comm, b) == 0)
		return 0;

	if (xv->log != NULL) {
		TProtoLog *log = xv->log;

		if (xv->LogState == 0) {
			// 残りのASCII表示を行う
			log->DumpFlush(log);

			xv->LogState = 1;
			log->WriteRaw(log, "\015\012<<<\015\012", 7);
		}
		log->DumpByte(log, *b);
	}
	return 1;
}

static int XWrite(PXVar xv, const void *_B, size_t C)
{
	int i, j;
	char *B = (char *)_B;

	i = xv->Comm->op->BinaryOut(xv->Comm, B, (int)C);
	if (xv->log != NULL && (i > 0)) {
		TProtoLog* log = xv->log;
		if (xv->LogState != 0) {
			// 残りのASCII表示を行う
			log->DumpFlush(log);

			xv->LogState = 0;
			log->WriteRaw(log, "\015\012>>>\015\012", 7);
		}
		for (j = 0; j <= i - 1; j++)
			log->DumpByte(log, B[j]);
	}
	return i;
}

static void XFlushReceiveBuf(PXVar xv)
{
	BYTE b;
	int r;

	do {
		r = XRead1Byte(xv, &b);
	} while (r != 0);
}

static void XCancel_(PXVar xv)
{
	// five cancels & five backspaces per spec
	static const BYTE cancel[] = { CAN, CAN, CAN, CAN, CAN, BS, BS, BS, BS, BS };

	XWrite(xv, (PCHAR)&cancel, sizeof(cancel));
	xv->state = STATE_CANCELED;		// quit
}

static void XSetOpt(PXVar xv, WORD Opt)
{
	char Tmp[21];
	PFileVarProto fv = xv->fv;

	xv->XOpt = Opt;

	strncpy_s(Tmp, sizeof(Tmp), "XMODEM (", _TRUNCATE);
	switch (xv->XOpt) {
	case XoptCheck:			/* Checksum */
		strncat_s(Tmp, sizeof(Tmp), "checksum)", _TRUNCATE);
		xv->DataLen = 128;
		xv->CheckLen = 1;
		break;
	case XoptCRC:				/* CRC */
		strncat_s(Tmp, sizeof(Tmp), "CRC)", _TRUNCATE);
		xv->DataLen = 128;
		xv->CheckLen = 2;
		break;
	case Xopt1kCRC:				/* 1k */
		strncat_s(Tmp, sizeof(Tmp), "1k)", _TRUNCATE);
		xv->DataLen = 1024;
		xv->CheckLen = 2;
		break;
	case Xopt1kCksum:			/* 1k with Checksum (unofficial) */
		strncat_s(Tmp, sizeof(Tmp), "1k*)", _TRUNCATE);
		xv->DataLen = 1024;
		xv->CheckLen = 1;
		break;
	}
	fv->InfoOp->SetDlgProtoText(fv, Tmp);
}

static void XSendNAK(PXVar xv)
{
	BYTE b;
	int t;
	PFileVarProto fv = xv->fv;

	/* flush comm buffer */
	xv->Comm->op->FlashReceiveBuf(xv->Comm);

	xv->NAKCount--;
	if (xv->NAKCount < 0) {
		if (xv->NAKMode == XnakC) {
			XSetOpt(xv, XoptCheck);
			xv->NAKMode = XnakNAK;
			xv->NAKCount = 9;
		} else {
			XCancel_(xv);
			return;
		}
	}

	if (xv->NAKMode == XnakNAK) {
		b = NAK;
		if ((xv->PktNum == 0) && (xv->PktNumOffset == 0))
			t = xv->TOutInit;
		else
			t = xv->TOutLong;
	} else {
		b = 'C';
		t = xv->TOutInitCRC;
	}
	XWrite(xv, &b, 1);
	xv->PktReadMode = XpktSOH;
	fv->FTSetTimeOut(fv, t);
}

static WORD XCalcCheck(PXVar xv, const BYTE *PktBuf)
{
	int i;
	WORD Check;

	if (xv->CheckLen == 1) {	/* CheckSum */
		/* Calc sum */
		Check = 0;
		for (i = 0; i <= xv->DataLen - 1; i++)
			Check = Check + (BYTE) (PktBuf[3 + i]);
		return (Check & 0xff);
	} else {					/* CRC */
		Check = 0;
		for (i = 0; i <= xv->DataLen - 1; i++)
			Check = UpdateCRC(PktBuf[3 + i], Check);
		return (Check);
	}
}

static BOOL XCheckPacket(PXVar xv)
{
	WORD Check;

	Check = XCalcCheck(xv, xv->PktIn);
	if (xv->CheckLen == 1)		/* Checksum */
		return ((BYTE) Check == xv->PktIn[xv->DataLen + 3]);
	else
		return ((HIBYTE(Check) == xv->PktIn[xv->DataLen + 3]) &&
				(LOBYTE(Check) == xv->PktIn[xv->DataLen + 4]));
}

static BOOL XInit(TProto *pv, PComVar cv, PTTSet ts)
{
	PXVar xv = pv->PrivateData;
	PFileVarProto fv = xv->fv;
	TFileIO *file = xv->file;
	BOOL LogFlag = ((ts->LogFlag & LOG_X) != 0);
	if (LogFlag) {
		TProtoLog* log = ProtoLogCreate();
		xv->log = log;
		log->SetFolderW(log, ts->LogDirW);
		log->Open(log, "XMODEM.LOG");
		xv->LogState = 0;
	}

	xv->FullName = fv->GetNextFname(fv);
	if (xv->XMode == IdXSend) {
		size_t size;
		xv->FileOpen = file->OpenRead(file, xv->FullName);
		if (xv->FileOpen == FALSE) {
			return FALSE;
		}
		size = file->GetFSize(file, xv->FullName);
		if (size > 0x7FFFFFFF) { // LONG_MAX
			// TODO
			//  FileSize は各プロトコルごとに持つほうがよさそう
			return FALSE;
		}
		xv->FileSize = (LONG)size;
		fv->InfoOp->InitDlgProgress(fv, &xv->ProgStat);
		XFlushReceiveBuf(xv);
	} else {
		xv->FileOpen = file->OpenWrite(file, xv->FullName);
		if (xv->FileOpen == FALSE) {
			return FALSE;
		}
		xv->FileSize = 0;
		xv->ProgStat = -1;
	}
	fv->InfoOp->SetDlgProtoFileName(fv, xv->FullName);
	xv->StartTime = GetTickCount();

	xv->PktNumOffset = 0;
	xv->PktNum = 0;
	xv->PktNumSent = 0;
	xv->PktBufCount = 0;
	xv->CRRecv = FALSE;
	xv->CANCount = 0;

	xv->ByteCount = 0;

	xv->TOutInit = ts->XmodemTimeOutInit;
	xv->TOutInitCRC = ts->XmodemTimeOutInitCRC;
	xv->TOutVLong = ts->XmodemTimeOutVLong;

	if (cv->PortType == IdTCPIP) {
		xv->TOutShort = ts->XmodemTimeOutVLong;
		xv->TOutLong = ts->XmodemTimeOutVLong;
	} else {
		xv->TOutShort = ts->XmodemTimeOutShort;
		xv->TOutLong = ts->XmodemTimeOutLong;
	}

	XSetOpt(xv, xv->XOpt);

	if (xv->XOpt == XoptCheck || xv->XOpt == Xopt1kCksum) {
		xv->NAKMode = XnakNAK;
		xv->NAKCount = 10;
	} else {
		xv->NAKMode = XnakC;
		xv->NAKCount = 3;
	}

	switch (xv->XMode) {
	case IdXSend:
		// ファイル送信開始前に、"rx ファイル名"を自動的に呼び出す。(2007.12.20 yutaka)
		if (ts->XModemRcvCommand[0] != '\0') {
			TFileIO *file = xv->file;
			char inistr[MAX_PATH + 10];
			char *filename = file->GetSendFilename(file, xv->FullName, FALSE, TRUE, FALSE);
			_snprintf_s(inistr, sizeof(inistr), _TRUNCATE, "%s %s\015",
						ts->XModemRcvCommand, filename);
			XWrite(xv, inistr, strlen(inistr));
			free(filename);
		}

		fv->FTSetTimeOut(fv, xv->TOutVLong);
		break;
	case IdXReceive:
		XSendNAK(xv);
		break;
	}
	xv->state = STATE_FLUSH;
	return TRUE;
}

static void XCancel(TProto *pv)
{
	PXVar xv = pv->PrivateData;
	XCancel_(xv);
}

static void XTimeOutProc(TProto *pv)
{
	PXVar xv = pv->PrivateData;
	switch (xv->XMode) {
	case IdXSend:
		xv->state = STATE_CANCELED;	// quit
		break;
	case IdXReceive:
		XSendNAK(xv);
		break;
	}
}

static BOOL XReadPacket(PXVar xv)
{
	PFileVarProto fv = xv->fv;
	BYTE b, d;
	int i, c;
	BOOL GetPkt = FALSE;
	TFileIO *file = xv->file;

	for (c=XRead1Byte(xv, &b); (c > 0) && !GetPkt; c=XRead1Byte(xv, &b)) {
		switch (xv->PktReadMode) {
		case XpktSOH:
			switch (b) {
			case SOH:
				xv->PktIn[0] = b;
				xv->PktReadMode = XpktBLK;
				if (xv->XOpt == Xopt1kCRC)
					XSetOpt(xv, XoptCRC);
				else if (xv->XOpt == Xopt1kCksum)
					XSetOpt(xv, XoptCheck);
				fv->FTSetTimeOut(fv, xv->TOutShort);
				break;
			case STX:
				xv->PktIn[0] = b;
				xv->PktReadMode = XpktBLK;
				if (xv->XOpt == XoptCRC)
					XSetOpt(xv, Xopt1kCRC);
				else if (xv->XOpt == XoptCheck)
					XSetOpt(xv, Xopt1kCksum);
				fv->FTSetTimeOut(fv, xv->TOutShort);
				break;
			case EOT:
				b = ACK;
				fv->Success = TRUE;
				XWrite(xv, &b, 1);
				return FALSE;
				break;
			case CAN:
				xv->CANCount++;
				if (xv->CANCount <= 2) {
					continue;
				}
				else {
					return FALSE;
				}
				break;
			default:
				/* flush comm buffer */
				xv->Comm->op->FlashReceiveBuf(xv->Comm);
				return TRUE;
				break;
			}
			xv->CANCount = 0;
			break;
		case XpktBLK:
			xv->PktIn[1] = b;
			xv->PktReadMode = XpktBLK2;
			fv->FTSetTimeOut(fv, xv->TOutShort);
			break;
		case XpktBLK2:
			xv->PktIn[2] = b;
			if ((b ^ xv->PktIn[1]) == 0xff) {
				xv->PktBufPtr = 3;
				xv->PktBufCount = xv->DataLen + xv->CheckLen;
				xv->PktReadMode = XpktDATA;
				fv->FTSetTimeOut(fv, xv->TOutShort);
			} else
				XSendNAK(xv);
			break;
		case XpktDATA:
			xv->PktIn[xv->PktBufPtr] = b;
			xv->PktBufPtr++;
			xv->PktBufCount--;
			GetPkt = xv->PktBufCount == 0;
			if (GetPkt) {
				fv->FTSetTimeOut(fv, xv->TOutLong);
				xv->PktReadMode = XpktSOH;
			} else
				fv->FTSetTimeOut(fv, xv->TOutShort);
			break;
		}
	}

	if (!GetPkt)
		return TRUE;

	if ((xv->PktIn[1] == 0) && (xv->PktNum == 0) &&
		(xv->PktNumOffset == 0)) {
		if (xv->NAKMode == XnakNAK)
			xv->NAKCount = 10;
		else
			xv->NAKCount = 3;
		XSendNAK(xv);
		return TRUE;
	}

	GetPkt = XCheckPacket(xv);
	if (!GetPkt) {
		XSendNAK(xv);
		return TRUE;
	}

	d = xv->PktIn[1] - xv->PktNum;
	if (d > 1) {
		XCancel_(xv);
		return FALSE;
	}

	/* send ACK */
	b = ACK;
	XWrite(xv, &b, 1);
	xv->NAKMode = XnakNAK;
	xv->NAKCount = 10;

	if (d == 0)
		return TRUE;
	xv->PktNum = xv->PktIn[1];
	if (xv->PktNum == 0)
		xv->PktNumOffset = xv->PktNumOffset + 256;

	c = xv->DataLen;
	if (xv->TextFlagTrim1A) {
		// パケット末の 0x1A を削除
		while ((c > 0) && (xv->PktIn[2 + c] == 0x1A))
			c--;
	}

	if (xv->TextFlagConvertCRLF) {
		// パケット内のCRのみLFのみをCR+LFに変換しながらファイルへ出力
		for (i = 0; i <= c - 1; i++) {
			b = xv->PktIn[3 + i];
			if ((b == LF) && (!xv->CRRecv)) {
				// LF(0x0A)受信 && 前のデータがCR(0x0D)ではない
				// CR(0x0D)出力
				//  → (CR以外)+(LF) は、(CR以外)+(CR)+(LF)で出力される
				file->WriteFile(file, "\015", 1);
			}
			if (xv->CRRecv && (b != LF)) {
				// LF(0x0A)以外受信 && 前のデータがCR(0xOD)
				// LF(0x0A)出力
				//  → (CR)+(LF以外)は、(CR)+(LF)+(LF以外)で出力される
				file->WriteFile(file, "\012", 1);
			}
			xv->CRRecv = b == CR;
			file->WriteFile(file, &b, 1);
		}
	} else {
		// そのままファイルへ出力
		file->WriteFile(file, &(xv->PktIn[3]), c);
	}

	xv->ByteCount = xv->ByteCount + c;

	fv->InfoOp->SetDlgPacketNum(fv, xv->PktNumOffset + xv->PktNum);
	fv->InfoOp->SetDlgByteCount(fv, xv->ByteCount);
	fv->InfoOp->SetDlgTime(fv, xv->StartTime, xv->ByteCount);

	fv->FTSetTimeOut(fv, xv->TOutLong);

	return TRUE;
}

static BOOL XSendPacket(PXVar xv)
{
	PFileVarProto fv = xv->fv;
	BYTE b;
	int i;
	BOOL SendFlag;
	WORD Check;
	BOOL is0x43Received = FALSE;//受信文字に'C'(0x43)が含まれているかのフラグ

	SendFlag = FALSE;
	if (xv->PktBufCount == 0) {
		for(;;){
			i = XRead1Byte(xv, &b);
			if (i == 0)
				break; //先行して受信した文字が全て無くなるまで繰り返す(CAN CANのみ即時キャンセル)

			switch (b) {
			case ACK:
				if (!xv->FileOpen) {
					fv->Success = TRUE;
					return FALSE;
				} else if (xv->PktNumSent == (BYTE) (xv->PktNum + 1)) {
					xv->PktNum = xv->PktNumSent;
					if (xv->PktNum == 0)
						xv->PktNumOffset = xv->PktNumOffset + 256;
					SendFlag = TRUE;
				}
				break;
			case NAK:
				if ((xv->PktNum == 0) && (xv->PktNumOffset == 0) && (xv->PktNumSent == 0)) {
					if (!is0x43Received) { //先にCRC要求'C'(0x43)を受け付けていた場合はCRCモードを維持。(CRCで送って受け付けなかった場合はNAKを送ってくるはずなのでCheckSumでの再送に切り替わる)
						if (xv->XOpt == XoptCRC) {
							// receiver wants to use checksum.
							XSetOpt(xv, XoptCheck);
						} else if (xv->XOpt == Xopt1kCRC) {
							/* we wanted 1k with CRC, but the other end specified checksum */
							/* keep the 1k block, but move back to checksum mode.          */
							XSetOpt(xv, Xopt1kCksum);
						}
					}
				}
				SendFlag = TRUE;
				break;
			case CAN:
				xv->CANCount++;
				if (xv->CANCount <= 2) {
					continue;
				}
				else {
					return FALSE;
				}
				break;
			case 0x43:
				// 0x43 = 'C' crcモードを要求している
				if ((xv->PktNum == 0) && (xv->PktNumOffset == 0) && (xv->PktNumSent == 0)) {
					if (xv->XOpt == XoptCheck) {
						XSetOpt(xv, XoptCRC);
					}
					else if (xv->XOpt == Xopt1kCksum) {
						XSetOpt(xv, Xopt1kCRC);
					}
					SendFlag = TRUE;
					is0x43Received = TRUE;//CRCで要求があった
				}
				break;
			}
			xv->CANCount = 0;
		}

		if (!SendFlag){
			return TRUE; //送信するものがないなら処理を抜ける
		}


		// reset timeout timer
		fv->FTSetTimeOut(fv, xv->TOutVLong);

		XFlushReceiveBuf(xv);

		if (xv->PktNumSent == xv->PktNum) {	/* make a new packet */
			TFileIO *file = xv->file;

			xv->PktNumSent++;
			if (xv->DataLen == 128)
				xv->PktOut[0] = SOH;
			else
				xv->PktOut[0] = STX;
			xv->PktOut[1] = xv->PktNumSent;
			xv->PktOut[2] = ~xv->PktNumSent;

			i = 1;
			while ((i <= xv->DataLen) && xv->FileOpen &&
				   (file->ReadFile(file, &b, 1) == 1)) {
				xv->PktOut[2 + i] = b;
				i++;
				xv->ByteCount++;
			}

			if (i > 1) {
				while (i <= xv->DataLen) {
					xv->PktOut[2 + i] = 0x1A;
					i++;
				}

				Check = XCalcCheck(xv, xv->PktOut);
				if (xv->CheckLen == 1)	/* Checksum */
					xv->PktOut[xv->DataLen + 3] = (BYTE) Check;
				else {
					xv->PktOut[xv->DataLen + 3] = HIBYTE(Check);
					xv->PktOut[xv->DataLen + 4] = LOBYTE(Check);
				}
				xv->PktBufCount = 3 + xv->DataLen + xv->CheckLen;
			} else {			/* send EOT */
				if (xv->FileOpen) {
					file->Close(file);
					xv->FileOpen = FALSE;
				}
				xv->PktOut[0] = EOT;
				xv->PktBufCount = 1;
			}
		} else {				/* resend packet */
			if (xv->PktOut[0] == EOT) {
				xv->PktBufCount = 1;
			} else {
				xv->PktBufCount = 3 + xv->DataLen + xv->CheckLen;
			}
		}
		xv->PktBufPtr = 0;
	}
	/* a NAK or C could have arrived while we were buffering.  Consume it. */
	XFlushReceiveBuf(xv);

	i = 1;
	while ((xv->PktBufCount > 0) && (i > 0)) {
		b = xv->PktOut[xv->PktBufPtr];
		i = XWrite(xv, &b, 1);
		if (i > 0) {
			xv->PktBufCount--;
			xv->PktBufPtr++;
		}
	}

	if (xv->PktBufCount == 0) {
		if (xv->PktNumSent == 0) {
			fv->InfoOp->SetDlgPacketNum(fv, xv->PktNumOffset + 256);
		}
		else {
			fv->InfoOp->SetDlgPacketNum(fv, xv->PktNumOffset + xv->PktNumSent);
		}
		fv->InfoOp->SetDlgByteCount(fv, xv->ByteCount);
		fv->InfoOp->SetDlgPercent(fv, xv->ByteCount, xv->FileSize, &xv->ProgStat);
		fv->InfoOp->SetDlgTime(fv, xv->StartTime, xv->ByteCount);
	}

	return TRUE;
}

static BOOL XParse(TProto *pv)
{
	PXVar xv = pv->PrivateData;
	switch (xv->state) {
	case STATE_FLUSH:
		// 受信データを捨てる
		XFlushReceiveBuf(xv);
		xv->state = STATE_NORMAL;	// 送受信処理を行う
		return TRUE;
	case STATE_NORMAL:
		switch (xv->XMode) {
		case IdXReceive:
			return XReadPacket(xv);
		case IdXSend:
			return XSendPacket(xv);
		default:
			return FALSE;
		}
	case STATE_CANCELED:
		XFlushReceiveBuf(xv);
		return FALSE;
	}
	return FALSE;
}

static int SetOptV(TProto *pv, int request, va_list ap)
{
	PXVar xv = pv->PrivateData;
	switch(request) {
	case XMODEM_MODE: {
		const int Mode = va_arg(ap, int);
		xv->XMode = Mode;
		return 0;
	}
	case XMODEM_OPT: {
		const WORD Opt1 = va_arg(ap, int);
		xv->XOpt = Opt1;
		return 0;
	}
	case XMODEM_TEXT_FLAG: {
		const WORD Opt2 = va_arg(ap, int);
		const BOOL TextFlag = Opt2 == 0 ? FALSE : TRUE;
		xv->TextFlagConvertCRLF = TextFlag;
		xv->TextFlagTrim1A = TextFlag;
		return 0;
	}
	}
	return -1;
}

static int SetOpt(TProto *pv, int request, ...)
{
	va_list ap;
	va_start(ap, request);
	int r = SetOptV(pv, request, ap);
	va_end(ap);
	return r;
}

static void Destroy(TProto *pv)
{
	PXVar xv = pv->PrivateData;
	if (xv->log != NULL) {
		TProtoLog* log = xv->log;
		log->Destory(log);
		xv->log = NULL;
	}
	free((void*)xv->FullName);
	xv->FullName = NULL;
	free(xv);
	pv->PrivateData = NULL;
	free(pv);
}

static const TProtoOp Op = {
	XInit,
	XParse,
	XTimeOutProc,
	XCancel,
	SetOpt,
	SetOptV,
	Destroy,
};

TProto *XCreate(PFileVarProto fv)
{
	TProto *pv = malloc(sizeof(*pv));
	if (pv == NULL) {
		return NULL;
	}
	pv->Op = &Op;
	PXVar xv = malloc(sizeof(TXVar));
	pv->PrivateData = xv;
	if (xv == NULL) {
		free(pv);
		return NULL;
	}
	memset(xv, 0, sizeof(*xv));
	xv->FileOpen = FALSE;
	xv->XOpt = XoptCRC;
	xv->state = STATE_FLUSH;
	xv->Comm = fv->Comm;
	xv->file = fv->file_fv;
	xv->fv = fv;

	return pv;
}
