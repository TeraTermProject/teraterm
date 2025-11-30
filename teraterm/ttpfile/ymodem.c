/*
 * Copyright (C) 2008- TeraTerm Project
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

/* YMODEM protocol */
#include <stdio.h>
#include <assert.h>

#include "tttypes.h"
#include "ftlib.h"
#include "protolog.h"
#include "filesys_proto.h"
#include "filesys.h"

#include "ymodem.h"

/* YMODEM */
typedef struct {
	BYTE PktIn[1030], PktOut[1030];
	int PktBufCount, PktBufPtr;
	BYTE PktNum, PktNumSent;
	int PktNumOffset;
	int PktReadMode;
	YMODEM_MODE_T YMode;
	WORD YOpt, TextFlag;
	WORD NAKMode;
	int NAKCount;
	WORD __DataLen, CheckLen;
	BOOL CRRecv;
	int TOutShort;
	int TOutLong;
	int TOutInit;
	int TOutInitCRC;
	int TOutVLong;
	int SendFileInfo;
	int SendEot;
	int LastSendEot;
	WORD DataLen;
	BYTE LastMessage;
	BOOL RecvFilesize;
	TProtoLog *log;
	const char *FullName;		// Windows上のファイル名 UTF-8
	WORD LogState;

	BOOL FileOpen;
	LONG FileSize;
	LONG ByteCount;

	int ProgStat;

	DWORD StartTime;

	DWORD FileMtime;

	TComm *Comm;
	PComVar cv;
	PFileVarProto fv;
	TFileIO *file;
} TYVar;
typedef TYVar *PYVar;

  /* YMODEM states */
#define YpktSOH 0x01
#define YpktSTX 0x02
#define YpktEOT 0x04
#define YpktACK 0x06
#define YpktNAK 0x15
#define YpktCAN 0x18

#define YnakC 1
#define YnakG 2

// データ転送サイズ。YMODEMでは 128 or 1024 byte をサポートする。
#define SOH_DATALEN	128
#define STX_DATALEN	1024

  /* XMODEM states */
#define XpktSOH 1
#define XpktBLK 2
#define XpktBLK2 3
#define XpktDATA 4

#define XnakNAK 1
#define XnakC 2

static int YRead1Byte(PYVar yv, LPBYTE b)
{
	if (yv->Comm->op->Read1Byte(yv->Comm, b) == 0)
		return 0;

	if (yv->log != NULL)
	{
		TProtoLog *log = yv->log;
		if (yv->LogState==0)
		{
			// 残りのASCII表示を行う
			log->DumpFlush(log);

			yv->LogState = 1;
			log->WriteRaw(log, "\015\012<<<\015\012", 7);
		}
		log->DumpByte(log, *b);
	}
	return 1;
}

static int YWrite(PYVar yv, PCHAR B, int C)
{
	int i, j;

	i = yv->Comm->op->BinaryOut(yv->Comm, B, C);
	if (yv->log != NULL && (i>0))
	{
		TProtoLog* log = yv->log;
		if (yv->LogState != 0)
		{
			// 残りのASCII表示を行う
			log->DumpFlush(log);

			yv->LogState = 0;
			log->WriteRaw(log, "\015\012>>>\015\012", 7);
		}
		for (j=0 ; j <= i-1 ; j++)
			log->DumpByte(log, B[j]);
	}
	return i;
}

static void YCancel_(PYVar yv)
{
	// five cancels & five backspaces per spec
	static const BYTE cancel[] = {
		CAN, CAN, CAN, CAN, CAN, BS, BS, BS, BS, BS
	};

	YWrite(yv, (PCHAR)&cancel, sizeof(cancel));
	yv->YMode = IdYQuit;	// quit
	if (! yv->cv->Ready){
		ProtoEnd();	// セッション断の場合は直接 ProtoEnd() を呼んでウィンドウを閉じる
	}
}

static void YSetOpt(PYVar yv, WORD Opt)
{
	PFileVarProto fv = yv->fv;
	char Tmp[21];

	yv->YOpt = Opt;

	strncpy_s(Tmp, sizeof(Tmp),"YMODEM (", _TRUNCATE);
	switch (yv->YOpt) {
	case Yopt1K: /* YMODEM */
		strncat_s(Tmp,sizeof(Tmp),"1k)",_TRUNCATE);
		yv->__DataLen = STX_DATALEN;
		yv->DataLen = STX_DATALEN;
		yv->CheckLen = 2;
		break;
	case YoptG: /* YMODEM-g */
		strncat_s(Tmp,sizeof(Tmp),"-g)",_TRUNCATE);
		yv->__DataLen = STX_DATALEN;
		yv->DataLen = STX_DATALEN;
		yv->CheckLen = 2;
		break;
	case YoptSingle: /* YMODEM(-g) single mode */
		strncat_s(Tmp,sizeof(Tmp),"single mode)",_TRUNCATE);
		yv->__DataLen = STX_DATALEN;
		yv->DataLen = STX_DATALEN;
		yv->CheckLen = 2;
		break;
	}
	fv->InfoOp->SetDlgProtoText(fv, Tmp);
}

static void YSendNAK(PYVar yv)
{
	BYTE b;
	int t;
	PFileVarProto fv = yv->fv;

	/* flush comm buffer */
	yv->Comm->op->FlashReceiveBuf(yv->Comm);

	yv->NAKCount--;
	if (yv->NAKCount<0)
	{
		if (yv->NAKMode==YnakC)
		{
			YSetOpt(yv,Yopt1K);
			yv->NAKMode = YnakC;
			yv->NAKCount = 9;
		}
		else {
			YCancel_(yv);
			return;
		}
	}

	if (yv->NAKMode!=YnakC)
	{
		b = NAK;
		if ((yv->PktNum==0) && (yv->PktNumOffset==0))
			t = yv->TOutInit;
		else
			t = yv->TOutLong;
	}
	else {
		b = 'C';
		t = yv->TOutInitCRC;
	}
	YWrite(yv,&b,1);
	yv->PktReadMode = XpktSOH;
	fv->FTSetTimeOut(fv,t);
}

static void YSendNAKTimeout(PYVar yv)
{
	BYTE b;
	int t;
	PFileVarProto fv = yv->fv;

	/* flush comm buffer */
	yv->Comm->op->FlashReceiveBuf(yv->Comm);

	yv->NAKCount--;
	if (yv->NAKCount<0)
	{
		if (yv->NAKMode==YnakC)
		{
			YSetOpt(yv,Yopt1K);
			yv->NAKMode = YnakC;
			yv->NAKCount = 9;
		}
		else {
			YCancel_(yv);
			return;
		}
	}

	//if (yv->NAKMode!=YnakC)
	if (1)
	{
		b = NAK;
		if ((yv->PktNum==0) && (yv->PktNumOffset==0))
			t = yv->TOutInit;
		else
			t = yv->TOutLong;
	}
	else {
		b = 'C';
		t = yv->TOutInitCRC;
	}
	YWrite(yv,&b,1);
	yv->PktReadMode = XpktSOH;
	fv->FTSetTimeOut(fv,t);
}

static WORD YCalcCheck(PYVar yv, const PCHAR PktBuf, const WORD len)
{
	int i;
	WORD Check;

	// CheckSum.
	if (1 == yv->CheckLen)
	{
		// Calc sum.
		Check = 0;
		for (i = 0 ; i <= len - 1 ; i++)
			Check = Check + (BYTE)(PktBuf[3 + i]);
		return (Check & 0xff);
	}
	else
	{
		// CRC.
		Check = 0;
		for (i = 0 ; i <= len - 1 ; i++)
			Check = UpdateCRC(PktBuf[3 + i],Check);
		return (Check);
	}
}

static BOOL YCheckPacket(PYVar yv, const WORD len)
{
	WORD Check;

	Check = YCalcCheck(yv, yv->PktIn, len);
	// Checksum.
	if (1 == yv->CheckLen)
		return ((BYTE)Check == yv->PktIn[len + 3]);
	else
		return ((HIBYTE(Check) == yv->PktIn[len + 3]) &&
		        (LOBYTE(Check) == yv->PktIn[len + 4]));
}

static void initialize_file_info(PYVar yv)
{
	PFileVarProto fv = yv->fv;
	TFileIO *file = yv->file;
	if (yv->YMode == IdYSend) {
		if (yv->FileOpen) {
			file->Close(file);

			if (yv->FileMtime > 0) {
				file->SetFMtime(file, yv->FullName, yv->FileMtime);
			}
		}
		yv->FileOpen = file->OpenRead(file, yv->FullName);
		yv->FileSize = file->GetFSize(file, yv->FullName);
	} else {
		yv->FileOpen = FALSE;
		yv->FileSize = 0;
		yv->FileMtime = 0;
		yv->RecvFilesize = FALSE;
	}

	if (yv->YMode == IdYSend) {
		fv->InfoOp->InitDlgProgress(fv, &yv->ProgStat);
	} else {
		yv->ProgStat = -1;
	}
	yv->StartTime = GetTickCount();
	fv->InfoOp->SetDlgProtoFileName(fv, yv->FullName);

	yv->PktNumOffset = 0;
	yv->PktNum = 0;
	yv->PktNumSent = 0;
	yv->PktBufCount = 0;
	yv->CRRecv = FALSE;
	yv->ByteCount = 0;
	yv->SendFileInfo = 0;
	yv->SendEot = 0;
	yv->LastSendEot = 0;
	yv->LastMessage = 0;
}

static BOOL YInit(TProto *pv, PComVar cv, PTTSet ts)
{
	PYVar yv = pv->PrivateData;
	PFileVarProto fv = yv->fv;

	if (yv->YMode == IdYSend) {
		char *filename = fv->GetNextFname(fv);
		if (filename == NULL) {
			return FALSE;
		}
		yv->FullName = filename;
	}

	if ((ts->LogFlag & LOG_Y)!=0) {
		TProtoLog* log = ProtoLogCreate();
		yv->log = log;
		log->SetFolderW(log, ts->LogDirW);
		log->Open(log, "YMODEM.LOG");
		yv->LogState = 0;
	}

	initialize_file_info(yv);

	yv->TOutInit = ts->YmodemTimeOutInit;
	yv->TOutInitCRC = ts->YmodemTimeOutInitCRC;
	yv->TOutVLong = ts->YmodemTimeOutVLong;

	yv->cv = cv;
	if (cv->PortType==IdTCPIP)
	{
		yv->TOutShort = ts->YmodemTimeOutVLong;
		yv->TOutLong = ts->YmodemTimeOutVLong;
	}
	else {
		yv->TOutShort = ts->YmodemTimeOutShort;
		yv->TOutLong = ts->YmodemTimeOutLong;
	}

	YSetOpt(yv,yv->YOpt);

	if (yv->YOpt == Yopt1K)
	{
		yv->NAKMode = YnakC;
		yv->NAKCount = 10;
	}
	else {
		yv->NAKMode = YnakG;
		yv->NAKCount = 10;
	}

	if (yv->log != NULL) {
		char buf[128];
		char ctime_str[128];
		time_t tm = time(NULL);
		ctime_s(ctime_str, sizeof(ctime_str), &tm);

		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "YMODEM %s start: %s\n",
		            yv->YMode == IdYSend ? "Send" : "Recv",
		            ctime_str);
		yv->log->WriteRaw(yv->log, buf, strlen(buf));
	}

	switch (yv->YMode) {
	case IdYSend:
		yv->TextFlag = 0;

		// ファイル送信開始前に、"rb ファイル名"を自動的に呼び出す。(2007.12.20 yutaka)
		//strcpy(ts->YModemRcvCommand, "rb");
		if (ts->YModemRcvCommand[0] != '\0') {
			char inistr[MAX_PATH + 10];
			_snprintf_s(inistr, sizeof(inistr), _TRUNCATE, "%s\015",
			            ts->YModemRcvCommand);
			YWrite(yv, inistr , strlen(inistr));
		}

		fv->FTSetTimeOut(fv, yv->TOutVLong);
		break;

	case IdYReceive:
#if 0   // for debug
		strcpy(inistr, "sb -b svnrev.exe lrzsz-0.12.20.tar.gz\r\n");
//		strcpy(inistr, "sb url3.txt url4.txt url5.txt\r\n");
		YWrite(fv, yv, inistr , strlen(inistr));
#endif
		yv->TextFlag = 0;

		YSendNAK(yv);

		break;
	default:
		assert(0);
		break;
	}

	return TRUE;
}

static void YCancel(TProto *pv)
{
	PYVar yv = pv->PrivateData;
	YCancel_(yv);
}

static void YTimeOutProc(TProto *pv)
{
	PYVar yv = pv->PrivateData;
	switch (yv->YMode) {
	case IdYSend:
		yv->YMode = IdYQuit;	// quit
		break;
	case IdYReceive:
		if ((yv->PktNum == 0) && yv->PktNumOffset == 0)
			YSendNAK(yv);
		else
			YSendNAKTimeout(yv);
		break;
	case IdYQuit:
		// キャンセルが連続して発生?
		break;
	default:
		assert(0);
		break;
	}
}

static BOOL FTCreateFile(PYVar yv)
{
	PFileVarProto fv = yv->fv;
	TFileIO *file = yv->file;

	fv->InfoOp->SetDlgProtoFileName(fv, yv->FullName);
	yv->FileOpen = file->OpenWrite(file, yv->FullName);
	if (!yv->FileOpen) {
		if (!fv->NoMsg) {
			MessageBox(fv->HMainWin,"Cannot create file",
					   "Tera Term: Error",MB_ICONEXCLAMATION);
		}
		return FALSE;
	}

	yv->ByteCount = 0;
	yv->FileSize = 0;
	if (yv->ProgStat != -1) {
		yv->ProgStat = 0;
	}
	yv->StartTime = GetTickCount();

	return TRUE;
}

// YMODEMサーバからファイルを受信する際、ProtoParse()から呼び出される関数。
//
// +-------+-------+--------+---------+-----+
// |Header |Block# |1-Block#| Payload | CRC |
// +-------+-------+--------+---------+-----+
//    1       1        1      128/1024   2      byte
//
// return TRUE: ファイル受信中
//        FALSE: 受信完了
static BOOL YReadPacket(PYVar yv)
{
	BYTE b, d;
	int i, c, nak;
	BOOL GetPkt;
	TFileIO *file = yv->file;
	PFileVarProto fv = yv->fv;

	c = YRead1Byte(yv,&b);

	GetPkt = FALSE;

	while ((c>0) && (! GetPkt))
	{
		switch (yv->PktReadMode) {
		case XpktSOH:
			// SOH か STX かでブロック長が決まる。
			if (b==SOH)
			{
				yv->PktIn[0] = b;
				yv->PktReadMode = XpktBLK;
				yv->__DataLen = SOH_DATALEN;
				fv->FTSetTimeOut(fv,yv->TOutShort);
			}
			else if (b==STX)
			{
				yv->PktIn[0] = b;
				yv->PktReadMode = XpktBLK;
				yv->__DataLen = STX_DATALEN;
				fv->FTSetTimeOut(fv,yv->TOutShort);
			}
			else if (b==EOT)
			{
				// EOTが来たら、1つのファイル受信が完了したことを示す。
				if (yv->FileOpen) {
					file->Close(file);
					yv->FileOpen = FALSE;

					if (yv->FileMtime > 0) {
						file->SetFMtime(file, yv->FullName, yv->FileMtime);
					}

					// 1回目のEOTに対してNAKを返す
					b = NAK;
					YWrite(yv, &b, 1);
					return TRUE;
				}

				initialize_file_info(yv);

				// EOTに対してACKを返す
				b = ACK;
				YWrite(yv, &b, 1);

				// 次のファイル送信を促すため、'C'を送る。
				YSendNAK(yv);

				return TRUE;
			}
			else if (b == CAN) {
				// quit
				yv->YMode = IdYQuit;
				return FALSE;
			}
			else {
				/* flush comm buffer */
				yv->Comm->op->FlashReceiveBuf(yv->Comm);
				return TRUE;
			}
			break;
		case XpktBLK:
			yv->PktIn[1] = b;
			yv->PktReadMode = XpktBLK2;
			fv->FTSetTimeOut(fv,yv->TOutShort);
			break;
		case XpktBLK2:
			nak = 1;
			yv->PktIn[2] = b;
			if ((b ^ yv->PktIn[1]) == 0xff) {
				nak = 0;
				if (yv->SendFileInfo) {
					if (yv->PktIn[1] == (BYTE)(yv->PktNum + 1))  // 次のブロック番号か
						nak = 0;
				}
			}

			if (nak == 0)
			{
				yv->PktBufPtr = 3;
				yv->PktBufCount = yv->__DataLen + yv->CheckLen;
				yv->PktReadMode = XpktDATA;
				fv->FTSetTimeOut(fv,yv->TOutShort);
			}
			else
				YSendNAK(yv);
			break;
		case XpktDATA:
			yv->PktIn[yv->PktBufPtr] = b;
			yv->PktBufPtr++;
			yv->PktBufCount--;
			GetPkt = yv->PktBufCount==0;
			if (GetPkt)
			{
				fv->FTSetTimeOut(fv,yv->TOutLong);
				yv->PktReadMode = XpktSOH;
			}
			else
				fv->FTSetTimeOut(fv,yv->TOutShort);
			break;
		}

		if (! GetPkt) c = YRead1Byte(yv,&b);
	}

	if (! GetPkt) return TRUE;

	GetPkt = YCheckPacket(yv, yv->__DataLen);
	if (! GetPkt)
	{
		YSendNAK(yv);
		return TRUE;
	}

	// オールゼロならば、全ファイル受信の完了を示す。
	if (yv->PktIn[1] == 0x00 && yv->PktIn[2] == 0xFF &&
		yv->SendFileInfo == 0
		) {
		c = yv->__DataLen;
		while ((c>0) && (yv->PktIn[2+c]==0x00))
			c--;
		if (c == 0) {
		  b = ACK;
		  YWrite(yv, &b, 1);
		  fv->Success = TRUE;
		  return FALSE;
		}
	}

	d = yv->PktIn[1] - yv->PktNum;
	if (d>1)
	{
		YCancel_(yv);
		return FALSE;
	}

	/* send ACK */
	b = ACK;
	YWrite(yv, &b, 1);
	yv->NAKMode = YnakC;
	yv->NAKCount = 10;

	// 重複している場合は、何もしない。
	if (yv->SendFileInfo &&
		yv->PktIn[1] == (BYTE)(yv->PktNum)) {
		return TRUE;
	}

	yv->PktNum = yv->PktIn[1];

	// YMODEMの場合、block#0が「ファイル情報」となる。
	if (d == 0 &&
		yv->SendFileInfo == 0) {
		long modtime;
		long bytes_total;
		int mode;
		int ret;
		BYTE *p;
		char *name, *nameend;
		char *RecievePath;

		p = (BYTE *)malloc(yv->__DataLen + 1);
		memset(p, 0, yv->__DataLen + 1);
		memcpy(p, &(yv->PktIn[3]), yv->__DataLen);
		name = p;

		RecievePath = fv->GetRecievePath(fv);
		free((void *)yv->FullName);
		yv->FullName = file->GetReceiveFilename(file, name, FALSE, RecievePath, !fv->OverWrite);
		free(RecievePath);
		if (!FTCreateFile(yv)) {
			free(p);
			return FALSE;
		}
		nameend = name + 1 + strlen(name);
		if (*nameend) {
			ret = sscanf_s(nameend, "%ld%lo%o", &bytes_total, &modtime, &mode);
			if (ret >= 1) {
				yv->FileSize = bytes_total;
				yv->RecvFilesize = TRUE;
			}
			if (ret >= 2) {
				yv->FileMtime = modtime;
			}
		}

		fv->InfoOp->SetDlgProtoFileName(fv, name);

		yv->SendFileInfo = 1;

		// 次のファイル送信を促すため、'C'を送る。
		YSendNAK(yv);

		free(p);
		return TRUE;
	}

	if (yv->PktNum==0)
		yv->PktNumOffset = yv->PktNumOffset + 256;

	c = yv->__DataLen;
	if (yv->TextFlag>0)
		while ((c>0) && (yv->PktIn[2+c]==0x1A))
			c--;

	// 最終ブロックの余分なデータを除去する
	if (yv->RecvFilesize && yv->ByteCount + c > yv->FileSize) {
		c = yv->FileSize - yv->ByteCount;
	}

	if (yv->TextFlag>0)
		for (i = 0 ; i <= c-1 ; i++)
		{
			b = yv->PktIn[3+i];
			if ((b==LF) && (! yv->CRRecv))
				file->WriteFile(file,"\015",1);
			if (yv->CRRecv && (b!=LF))
				file->WriteFile(file,"\012",1);
			yv->CRRecv = b==CR;
			file->WriteFile(file,&b,1);
		}
	else
		file->WriteFile(file, &(yv->PktIn[3]), c);

	yv->ByteCount = yv->ByteCount + c;

	fv->InfoOp->SetDlgPacketNum(fv, yv->PktNumOffset + yv->PktNum);
	fv->InfoOp->SetDlgByteCount(fv, yv->ByteCount);
	fv->InfoOp->SetDlgTime(fv, yv->StartTime, yv->ByteCount);

	fv->FTSetTimeOut(fv,yv->TOutLong);

	return TRUE;
}

// ファイル送信(local-to-remote)時に、YMODEMサーバからデータが送られてきたときに呼び出される。
static BOOL YSendPacket(PYVar yv)
{
	PFileVarProto fv = yv->fv;
	// If current buffer is empty.
	if (0 == yv->PktBufCount)
	{
		// Main read loop.
		BOOL continue_read = TRUE;
		while (continue_read)
		{
			BYTE isym = 0;
			int is_success = YRead1Byte(yv, &isym);
			if (0 == is_success) return TRUE;

			// Analyze responce.
			switch (isym)
			{
			case ACK:
				// 1回目のEOT送信後のACK受信で、「1ファイル送信」の終わりとする。
				// If we already send EOT, ACK means that client confirms it.
				if (yv->SendEot)
				{
					char *filename;
					// Reset the flag.
					yv->SendEot = 0;

					// 送信ファイルが残っていない場合は、「全てのファイルを転送終了」を通知する。
					filename = fv->GetNextFname(fv);
					if (filename == NULL)
					{
						// If it is the last file.
						yv->LastSendEot = 1;
						break;
					}
					else
					{
						// Process with next file.
						free((void *)yv->FullName);
						yv->FullName = filename;
						initialize_file_info(yv);
					}
				}

				// If client confirms that last (empty) packed was received.
				// もう送信するファイルがない場合は、正常終了。
				if (!yv->FileOpen)
				{
					fv->Success = TRUE;
					yv->LastMessage = isym;
					return FALSE;
				}
				// 次のブロックを送る
				else if (yv->PktNumSent == (BYTE)(yv->PktNum + 1))
				{
					// ブロック0（ファイル情報）送信後は、ACK と 'C' を連続して受信することに
					// なっているため、次の'C'を待つ。(2010.6.20 yutaka)
					if ((yv->PktNum==0) && (yv->PktNumOffset==0))
					{
						// It is an ACK for file info, wait for 'C' by some reason (?).
						// 送信済みフラグon
						yv->SendFileInfo = 1;
						continue_read = TRUE;
						break;
					}
					yv->PktNum = yv->PktNumSent;
					if (0 == yv->PktNum)
						yv->PktNumOffset = yv->PktNumOffset + 256;
					continue_read = FALSE;
				}
				break;

			case NAK:
				// 1回目のEOT送信後のNAK受信で、最後"EOT"を送る。
				if (yv->SendEot)
				{
					yv->PktNum = yv->PktNumSent;
					if (0 == yv->PktNum)
						yv->PktNumOffset = yv->PktNumOffset + 256;
				}

				continue_read = FALSE;
				break;

			case CAN:
				// 直前も CAN の場合はキャンセル
				if (yv->LastMessage == CAN) {
					fv->Success = FALSE;       // failure
					return FALSE;
				}
				break;

			case 'C':
			case 'G':
				// 'C'を受け取ると、ブロックの送信を開始する。
				if ((0 == yv->PktNum) && (0 == yv->PktNumOffset) && !(yv->LastMessage == 'C'))
				{
					// ファイル情報送信後、ACK -> 'C' と受信したので、次のブロックを送信する。
					if (yv->SendFileInfo)
					{
						yv->PktNum = yv->PktNumSent;
						if (0 == yv->PktNum)
							yv->PktNumOffset = yv->PktNumOffset + 256;
					}

					continue_read = FALSE;
				}
				else if (yv->LastSendEot)
				{
					continue_read = FALSE;
				}
				else
				{
					// TODO: analyze else branch.
				}
				break;

			default:
				assert(0);
				break;
			}
			yv->LastMessage = isym;
		}

		// reset timeout timer
		fv->FTSetTimeOut(fv, yv->TOutVLong);
#if 0
		// 後続のサーバからのデータを読み捨てる。
		do
		{
			lastrx = firstch;
			i = YRead1Byte(yv,&b);
			if (i != 0) {
				firstch = b;
				if (firstch == CAN && lastrx == CAN) {
					// CAN(0x18)が連続してくると、ファイル送信の失敗と見なす。
					// たとえば、サーバに同名のファイルが存在する場合など。
					// (2010.3.23 yutaka)
					fv->Success = FALSE;       // failure
					return FALSE;
				}
			}
		} while (i != 0);
#endif

		//================================
		// Last packet case.
		//================================
		// オールゼロのブロックを送信して、もうファイルがないことを知らせる。
		if (yv->LastSendEot)
		{
			WORD Check;
			// Always 128 bytes for the last packet.
			WORD last_packet_size = SOH_DATALEN;
			int i;

			// Clear the flag.
			yv->LastSendEot = 0;

			yv->__DataLen = last_packet_size;
			yv->PktOut[0] = SOH;
			yv->PktOut[1] = 0;
			yv->PktOut[2] = ~0;

			i = 0;
			while (i < last_packet_size)
			{
				yv->PktOut[i+3] = 0x00;
				i++;
			}

			Check = YCalcCheck(yv, yv->PktOut, last_packet_size);
			// TODO: move checksum calculation to a function.
			if (1 == yv->CheckLen)
			{
				yv->PktOut[last_packet_size + 3] = (BYTE)Check;
			}
			else
			{
				yv->PktOut[last_packet_size + 3] = HIBYTE(Check);
				yv->PktOut[last_packet_size + 4] = LOBYTE(Check);
			}

			// TODO: remove magic number.
			yv->PktBufCount = 3 + last_packet_size + yv->CheckLen;
		}

		//================================
		// First or 256th packet case.
		//================================

		// Start a new sequence.
		else if (yv->PktNumSent==yv->PktNum)
		{
			 /* make a new packet */
			BYTE *dataptr = &yv->PktOut[3];
			int eot = 0;  // End Of Transfer
			WORD current_packet_size = yv->DataLen;

			if (SOH_DATALEN == current_packet_size)
				yv->PktOut[0] = SOH;
			else
				yv->PktOut[0] = STX;
			yv->PktOut[1] = yv->PktNumSent;
			yv->PktOut[2] = ~yv->PktNumSent;

			// ブロック番号のカウントアップ。YMODEMでは"0"から開始する。
			yv->PktNumSent++;

			//================================
			// First packet case.
			//================================
			// ブロック0
			// ファイル情報の送信
			if (yv->SendFileInfo == 0)
			{
				int ret, total;
				size_t idx;
				// TODO: remove magic number.
				BYTE buf[1024 + 10];
				TFileIO *file = yv->file;
				char *filename;

				// 128 bytes for the first packet.
				current_packet_size = SOH_DATALEN;
				yv->__DataLen = current_packet_size;
				yv->PktOut[0] = SOH;

				// Timestamp.
				yv->FileMtime = file->GetFMtime(file, yv->FullName);

				filename = file->GetSendFilename(file, yv->FullName, FALSE, FALSE, FALSE);
				ret = _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s", filename);
				free(filename);

				// NULL-terminated string.
				buf[ret] = 0x00;
				total = ret + 1;

				ret = _snprintf_s(&(buf[total]), sizeof(buf) - total, _TRUNCATE, "%lu %lo %o",
				                  yv->FileSize, yv->FileMtime, 0644|_S_IFREG);
				total += ret;

				// if bloack0 is long, expand to 1024 bytes.
				if (total > SOH_DATALEN) {
					current_packet_size = STX_DATALEN;
					yv->__DataLen = current_packet_size;
					yv->PktOut[0] = STX;
				}

				// Padding.
				idx = total;
				while (idx <= current_packet_size)
				{
					buf[idx] = 0x00;
					++idx;
				}

				// データコピー
				memcpy(dataptr, buf, current_packet_size);
			}

			//================================
			// 256th packet case.
			//================================

			else
			{
				TFileIO *file = yv->file;
				BYTE fsym = 0;
				size_t idx = 1;

				yv->__DataLen = current_packet_size;

				while ((idx <= current_packet_size) && yv->FileOpen &&
				       (1 == file->ReadFile(file, &fsym, 1)))
				{
					// TODO: remove magic number.
					yv->PktOut[2 + idx] = fsym;
					++idx;
					yv->ByteCount++;
				}

				// No bytes were read.
				if (1 == idx)
				{
					// Close file handle.
					if (yv->FileOpen)
					{
						file->Close(file);
						yv->FileOpen = FALSE;
					}

					// Send EOT.
					eot = 1;
				}
				else
				{
					// Padding.
					while (idx <= current_packet_size)
					{
						// TODO: remove magic number.
						yv->PktOut[2 + idx] = 0x1A;
						++idx;
					}
				}
			}

			// データブロック
			if (0 == eot)
			{
				// Add CRC if not End-of-Tranfer.
				WORD Check = YCalcCheck(yv, yv->PktOut, current_packet_size);
				// Checksum.
				if (1 == yv->CheckLen)
					yv->PktOut[current_packet_size + 3] = (BYTE)Check;
				else {
					yv->PktOut[current_packet_size + 3] = HIBYTE(Check);
					yv->PktOut[current_packet_size + 4] = LOBYTE(Check);
				}
				// TODO: remove magic number.
				yv->PktBufCount = 3 + current_packet_size + yv->CheckLen;

			}
			else
			{
				// EOT.
				yv->PktOut[0] = EOT;
				yv->PktBufCount = 1;

				// EOTフラグon。次はNAKを期待する。
				yv->SendEot = 1;
				yv->LastSendEot = 0;
			}
		}

		//================================
		// TODO: Analyze resend case.
		//================================

		else
		{
			// Resend packet.
			yv->PktBufCount = 3 + yv->__DataLen + yv->CheckLen;
		}

		// Reset counter.
		yv->PktBufPtr = 0;
	}
#if 0
	/* a NAK or C could have arrived while we were buffering.  Consume it. */
	// 後続のサーバからのデータを読み捨てる。
	do {
		lastrx = firstch;
		i = YRead1Byte(yv,&b);
		if (i != 0) {
			firstch = b;
			if (firstch == CAN && lastrx == CAN) {
				// CAN(0x18)が連続してくると、ファイル送信の失敗と見なす。
				// たとえば、サーバに同名のファイルが存在する場合など。
				// (2010.3.23 yutaka)
				fv->Success = FALSE;       // failure
				return FALSE;
			}
		}
	} while (i != 0);
#endif

	// Write bytes to COM.
	while (yv->PktBufCount > 0)
	{
		BYTE osym = yv->PktOut[yv->PktBufPtr];
		int is_success = YWrite(yv, &osym, 1);
		if (is_success > 0)
		{
			--yv->PktBufCount;
			++yv->PktBufPtr;
		}
		else
			break;
	}

	// Update dialog window.
	if (0 == yv->PktBufCount)
	{
		if (0 == yv->PktNumSent)
		{
			fv->InfoOp->SetDlgPacketNum(fv, yv->PktNumOffset + 256);
		}
		else
		{
			fv->InfoOp->SetDlgPacketNum(fv, yv->PktNumOffset + yv->PktNumSent);
		}

		fv->InfoOp->SetDlgByteCount(fv, yv->ByteCount);
		fv->InfoOp->SetDlgPercent(fv, yv->ByteCount, yv->FileSize, &yv->ProgStat);
		fv->InfoOp->SetDlgTime(fv, yv->StartTime, yv->ByteCount);
	}

	return TRUE;
}

static BOOL YParse(TProto *pv)
{
	PYVar yv = pv->PrivateData;
	switch (yv->YMode) {
	case IdYReceive:
		return YReadPacket(yv);
		break;
	case IdYSend:
		return YSendPacket(yv);
		break;
	case IdYQuit:
		return FALSE;
	default:
		assert(0);
		return FALSE;
	}
}

static int SetOptV(TProto *pv, int request, va_list ap)
{
	PYVar yv = pv->PrivateData;
	switch(request) {
	case YMODEM_MODE: {
		YMODEM_MODE_T Mode = va_arg(ap, YMODEM_MODE_T);
		yv->YMode = Mode;
		return 0;
	}
	case YMODEM_OPT: {
		WORD Opt1 = va_arg(ap, int);
		yv->YOpt = Opt1;
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
	PYVar yv = pv->PrivateData;
	if (yv->log != NULL) {
		TProtoLog* log = yv->log;
		log->Destory(log);
		yv->log = NULL;
	}
	free((void *)yv->FullName);
	yv->FullName = NULL;
	free(yv);
	pv->PrivateData = NULL;
	free(pv);
}

static const TProtoOp Op = {
	YInit,
	YParse,
	YTimeOutProc,
	YCancel,
	SetOpt,
	SetOptV,
	Destroy,
};

TProto *YCreate(PFileVarProto fv)
{
	TProto *pv = malloc(sizeof(*pv));
	if (pv == NULL) {
		return NULL;
	}
	pv->Op = &Op;
	PYVar yv = malloc(sizeof(TYVar));
	pv->PrivateData = yv;
	if (yv == NULL) {
		free(pv);
		return NULL;
	}
	memset(yv, 0, sizeof(*yv));
	yv->FileOpen = FALSE;
	yv->Comm = fv->Comm;
	yv->file = fv->file_fv;
	yv->fv = fv;

	return pv;
}
