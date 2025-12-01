/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007- TeraTerm Project
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

/* B-Plus protocol */
#include <string.h>

#include "tttypes.h"
#include "ftlib.h"
#include "protolog.h"
#include "filesys.h"

#include "bplus.h"

/* proto type */
//BOOL PASCAL GetTransFname(PFileVarProto fv, PCHAR CurDir, WORD FuncId, LPLONG Option);

/* B Plus */
typedef struct {
	BYTE WS;
	BYTE WR;
	BYTE B_S;
	BYTE CM;
	BYTE DQ;
	BYTE TL;
	BYTE Q[8];
	BYTE DR;
	BYTE UR;
	BYTE FI;
} TBPParam;


typedef struct {
	BYTE PktIn[2066], PktOut[2066];
	int PktInCount, CheckCount;
	int PktOutLen, PktOutCount, PktOutPtr;
	LONG Check, CheckCalc;
	BYTE PktNum, PktNumSent;
	int PktNumOffset;
	int PktSize;
	WORD BPMode, BPState, BPPktState;
	BOOL GetPacket, EnqSent;
	BYTE MaxBS, CM;
	BOOL Quoted;
	int TimeOut;
	BOOL CtlEsc;
	BYTE Q[8];
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
} TBPVar;
typedef TBPVar *PBPVar;

  /* B Plus states */
#define BP_Init      1
#define BP_RecvFile  2
#define BP_RecvData  3
#define BP_SendFile  4
#define BP_SendData  5
#define BP_SendClose 6
#define BP_Failure   7
#define BP_Close     8
#define BP_AutoFile  9

  /* B Plus packet states */
#define BP_PktGetDLE   1
#define BP_PktDLESeen  2
#define BP_PktGetData  3
#define BP_PktGetCheck 4
#define BP_PktSending  5

#define BPTimeOut 10
#define BPTimeOutTCPIP 0

static BOOL BPOpenFileToBeSent(PBPVar bv)
{
  BOOL r;
  PFileVarProto fv = bv->fv;
  TFileIO *fileio = bv->file;

  if (bv->FileOpen)
    return TRUE;
  if (bv->FullName == NULL) return FALSE;

  r = fileio->OpenRead(fileio, bv->FullName);
  bv->FileOpen = r;
  if (r == TRUE) {
    fv->InfoOp->SetDlgProtoFileName(fv, bv->FullName);
    bv->FileSize = fileio->GetFSize(fileio, bv->FullName);
  }
  return bv->FileOpen;
}

// 通信中に受信/送信が切り替わる?
static void BPDispMode(PBPVar bv)
{
  PFileVarProto fv = bv->fv;
	switch (bv->BPMode) {
    case IdBPSend:
		fv->SetDialogCation(fv, "FILEDLG_TRANS_TITLE_BPSEND", TitBPSend);
		break;
    case IdBPReceive:
    case IdBPAuto:
		fv->SetDialogCation(fv, "FILEDLG_TRANS_TITLE_BPRCV", TitBPRcv);
		break;
	}
}

static BOOL BPInit(TProto *pv, PComVar cv, PTTSet ts)
{
  int i;
  PBPVar bv = pv->PrivateData;
  PFileVarProto fv = bv->fv;

  if (bv->BPMode==IdBPAuto)
  {
	bv->Comm->op->Insert1Byte(bv->Comm, 'B');
	bv->Comm->op->Insert1Byte(bv->Comm, DLE);
  }

  BPDispMode(bv);
  fv->InfoOp->SetDlgProtoText(fv, "B-Plus");

  fv->InfoOp->InitDlgProgress(fv, &bv->ProgStat);
  bv->StartTime = GetTickCount();

  /* file name, file size */
  if (bv->BPMode==IdBPSend)
    BPOpenFileToBeSent(bv);

  /* default parameters */
  bv->cv = cv;
  for (i = 0 ; i<= 7 ; i++)
    bv->Q[i] = 0xFF;
  bv->CM = 0;

  bv->PktNumOffset = 0;
  bv->PktNum = 0;
  bv->PktNumSent = 0;
  bv->PktOutLen = 0;
  bv->PktOutCount = 0;
  bv->BPState = BP_Init;
  bv->BPPktState = BP_PktGetDLE;
  bv->GetPacket = FALSE;
  bv->EnqSent = FALSE;
  bv->CtlEsc = ((ts->FTFlag & FT_BPESCCTL)!=0);

  /* Time out & Max block size */
  if (cv->PortType==IdTCPIP)
  {
    bv->TimeOut = BPTimeOutTCPIP;
    bv->MaxBS = 16;
  }
  else {
    bv->TimeOut = BPTimeOut;
    if (ts->Baud <= 110) {
      bv->TimeOut = BPTimeOut*2;
      bv->MaxBS = 1;
    }
    else if (ts->Baud <= 300) {
      bv->MaxBS = 1;
    }
    else if (ts->Baud <= 600) {
      bv->MaxBS = 2;
    }
    else if (ts->Baud <= 1200) {
      bv->MaxBS = 4;
    }
    else if (ts->Baud <= 2400) {
      bv->MaxBS = 8;
    }
    else if (ts->Baud <= 480) {
      bv->MaxBS = 12;
    }
    else {
      bv->MaxBS = 16;
    }
  }

  if ((ts->LogFlag & LOG_BP)!=0) {
	  TProtoLog* log = ProtoLogCreate();
	  bv->log = log;
	  log->SetFolderW(log, ts->LogDirW);
	  log->Open(log, "BPLUS.LOG");
	  bv->LogState = 0;
  }

  return TRUE;
}

static int BPRead1Byte(PBPVar bv, LPBYTE b)
{
  if (bv->Comm->op->Read1Byte(bv->Comm, b) == 0)
    return 0;

  if (bv->log != NULL)
  {
	TProtoLog *log = bv->log;
    if (bv->LogState==0)
    {
		bv->LogState = 1;
		log->WriteRaw(log, "\015\012<<<\015\012", 7);
    }
    log->DumpByte(log, *b);
  }
  return 1;
}

static int BPWrite(PBPVar bv, PCHAR B, int C)
{
  int i, j;

  i = bv->Comm->op->BinaryOut(bv->Comm, B, C);

  if (bv->log != NULL && (i>0))
  {
	  TProtoLog* log = bv->log;
	  if (bv->LogState != 0)
    {
		  bv->LogState = 0;
		  log->WriteRaw(log,"\015\012>>>\015\012",7);
    }
    for (j=0 ; j <= i-1 ; j++)
      log->DumpByte(log,B[j]);
  }
  return i;
}

static void BPTimeOutProc(TProto *pv)
{
  PBPVar bv = pv->PrivateData;
  PFileVarProto fv = bv->fv;
  BPWrite(bv, "\005\005",2); /* two ENQ */
  fv->FTSetTimeOut(fv,bv->TimeOut);
  bv->EnqSent = TRUE;
}

static void BPUpdateCheck(PBPVar bv, BYTE b)
{
  WORD w;

  switch (bv->CM) {
    case 0: /* Standard checksum */
      w = (WORD)bv->CheckCalc;
      w = w << 1;
      if (w > 0xFF)
	w = (w & 0xFF) + 1;
      w = w + (WORD)b;
      if (w > 0xFF)
	w = (w & 0xFF) + 1;
      bv->CheckCalc = w;
      break;
    case 1: /* Modified XMODEM CRC-16 */
      bv->CheckCalc = (LONG)UpdateCRC(b,(WORD)bv->CheckCalc);
      break;
    default:
    /* CCITT CRC-16/32 are not implemented */
      bv->CheckCalc = 0;
  }
}

static void BPSendACK(PBPVar bv)
{
  char Temp[2];

  if ((bv->BPState != BP_Failure) &&
      (bv->BPState != BP_Close))
  {
    Temp[0] = 0x10; /* DLE */
    Temp[1] = (char)(bv->PktNum % 10 + 0x30);
    BPWrite(bv,Temp,2);
  }
  bv->BPPktState = BP_PktGetDLE;
}

static void BPSendNAK(PBPVar bv)
{
  if ((bv->BPState != BP_Failure) &&
      (bv->BPState != BP_Close))
    BPWrite(bv,"\025",1); /* NAK */
  bv->BPPktState = BP_PktGetDLE;
}

static void BPPut1Byte(PBPVar bv, BYTE b, int *OutPtr)
{
  int Iq;
  BYTE Mq;

  Mq = 0x80 >> (b % 8);
  if (b<=0x1f)
    Iq = b / 8;
  else if ((b>=0x80) && (b<=0x9f))
    Iq = b / 8 - 12;
  else {
    Iq = 0;
    Mq = 0;
  }

  if ((Mq & bv->Q[Iq]) > 0)
  {
    bv->PktOut[*OutPtr] = 0x10; /* DLE */
    (*OutPtr)++;
    if (b<=0x1f)
      b = b + 0x40;
    else if ((b>=0x80) && (b<=0x9f))
      b = b - 0x20;
  }
  bv->PktOut[*OutPtr] = b;
  (*OutPtr)++;
}

static void BPMakePacket(PBPVar bv, BYTE PktType, int DataLen)
{
  int i;
  BYTE b;
  BOOL Qflag;

  bv->PktNumSent = (bv->PktNum + 1) % 10;

  bv->PktOut[0] = 0x10; /* DLE */
  bv->PktOut[1] = 'B';
  bv->PktOut[2] = bv->PktNumSent + 0x30; /* Sequence number */
  bv->PktOut[3] = PktType;
  bv->PktOut[4+DataLen] = 0x03; /* ETX */

  /* Calc checksum */
  switch (bv->CM) {
    case 1: /* modified XMODEM-CRC */
      bv->CheckCalc = 0xFFFF;
      break;
    default: /* standard checksum */
    /* CCITT CRC-16/32 are not supported */
      bv->CheckCalc = 0;
  }
  Qflag = FALSE;
  for (i = 0 ; i <= DataLen+2 ; i++)
  {
    b = bv->PktOut[i+2];
    if (b==0x10)
      Qflag = TRUE;
    else {
      if (Qflag)
      {
	if ((b>=0x40) && (b<=0x5f))
	  b = b - 0x40;
	else if ((b>=0x60) && (b<=0x7f))
	  b = b + 0x20;
      }
      Qflag = FALSE;
      BPUpdateCheck(bv,b);
    }
  }

  /* Put check value */
  bv->PktOutCount = 5 + DataLen;
  switch (bv->CM) {
    case 1: /* Modified XMODEM-CRC */
      BPPut1Byte(bv,HIBYTE((WORD)(bv->CheckCalc)),&(bv->PktOutCount));
      BPPut1Byte(bv,LOBYTE((WORD)(bv->CheckCalc)),&(bv->PktOutCount));
      break;
    default: /* Standard checksum */
      BPPut1Byte(bv,(BYTE)(bv->CheckCalc),&(bv->PktOutCount));
  }
  bv->PktOutLen = bv->PktOutCount;
  bv->PktOutPtr = 0;
  bv->BPPktState = BP_PktSending;
}

static void BPSendFailure(PBPVar bv, BYTE b)
{
  int i;

  i = 4;
  BPPut1Byte(bv,b,&i);
  i = i - 4;
  BPMakePacket(bv,'F',i);
  bv->BPState = BP_Failure;
}

static void BPSendInit(PBPVar bv)
{
  BYTE b;
  int i, Count;
  TBPParam Param;

  memset(&Param,0,sizeof(Param));
  for (i = 1 ; i <= bv->PktInCount-2 ; i++)
  {
    b = bv->PktIn[i+1];
    switch (i) {
      case 1: Param.WS = b; break;
      case 2: Param.WR = b; break;
      case 3: Param.B_S = b; break;
      case 4: Param.CM = b; break;
      case 5: Param.DQ = b; break;
      case 6: Param.TL = b; break;
      case 7:
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
	Param.Q[i-7] = b;
	break;
      case 15: Param.DR = b; break;
      case 16: Param.UR = b; break;
      case 17: Param.FI = b; break;
    }
  }

  if (Param.B_S==0)
    Param.B_S = 4;
  if (Param.B_S > bv->MaxBS)
    Param.B_S = bv->MaxBS;

  if (Param.CM>1)
    Param.CM = 1;

  if (bv->PktInCount < 7)
  {
    Param.Q[0] = 0x14;
    Param.Q[2] = 0xD4;
    switch (Param.DQ) {
      case 1: break;
      case 2: Param.Q[6] = 0x50; break;
      case 3:
	for (i = 0 ; i <= 7 ; i++)
	  Param.Q[i] = 0xFF;
	break;
      default:
	Param.Q[0] = 0x94;
    }
  }

  if (bv->CtlEsc) // escape all ctrl chars
    for (i = 0 ; i <= 7 ; i++)
      Param.Q[i] = 0xFF;
  else {
    Param.Q[0] = Param.Q[0] | 0x14;
    Param.Q[1] = Param.Q[1] | 0x04;
    Param.Q[2] = Param.Q[2] | 0xD4;
  }

  for (i = 0 ; i <= 7 ; i++)
    bv->Q[i] = 0xFF;

  Count = 4;
  BPPut1Byte(bv,0,&Count); /* WS */
  BPPut1Byte(bv,Param.WS,&Count); /* WR */
  BPPut1Byte(bv,Param.B_S,&Count); /* BS */
  BPPut1Byte(bv,Param.CM,&Count); /* CM */
  BPPut1Byte(bv,Param.DQ,&Count); /* DQ */
  BPPut1Byte(bv,0,&Count); /* TL */
  for (i = 0 ; i <= 7 ; i++)
    BPPut1Byte(bv,Param.Q[i],&Count); /* Q1-8 */
  BPPut1Byte(bv,0,&Count); /* DR */
  BPPut1Byte(bv,0,&Count); /* UR */
  BPPut1Byte(bv,Param.FI,&Count); /* FI */

  Count = Count - 4;
  BPMakePacket(bv,'+',Count);

  bv->PktSize = Param.B_S*128;
  bv->CM = Param.CM;
  for (i = 0 ; i <= 7 ; i++)
    bv->Q[i] = Param.Q[i];
}

static void BPSendTCPacket(PBPVar bv)
{
  int i;

  i = 4;
  BPPut1Byte(bv,'C',&i);
  i = i - 4;
  BPMakePacket(bv,'T',i);
  bv->BPState = BP_SendClose;
}

static void BPSendNPacket(PBPVar bv)
{
  int i, c;
  BYTE b;
  PFileVarProto fv = bv->fv;
  TFileIO *fileio = bv->file;

  i = 4;
  c = 1;
  while ((i-4 < bv->PktSize-1) && (c>0))
  {
    c = fileio->ReadFile(fileio, &b, 1);
    if (c==1)
      BPPut1Byte(bv,b,&i);
    bv->ByteCount = bv->ByteCount + c;
  }
  if (c==0)
  {
    fileio->Close(fileio);
    bv->FileOpen = FALSE;
  }
  i = i - 4;
  BPMakePacket(bv,'N',i);

  fv->InfoOp->SetDlgByteCount(fv, bv->ByteCount);
  if (bv->FileSize > 0)
    fv->InfoOp->SetDlgPercent(fv, bv->ByteCount, bv->FileSize, &bv->ProgStat);
  fv->InfoOp->SetDlgTime(fv, bv->StartTime, bv->ByteCount);
}

static void BPCheckPacket(PBPVar bv)
{
  PFileVarProto fv = bv->fv;
  if (bv->Check != bv->CheckCalc)
  {
    BPSendNAK(bv);
    return;
  }

  /* Sequence number */
  if ((bv->PktNum+1) % 10 + 0x30 != bv->PktIn[0])
  {
    BPSendNAK(bv);
    return;
  }

  bv->PktNum++;
  if (bv->PktNum==10)
  {
    bv->PktNum = 0;
    bv->PktNumOffset = bv->PktNumOffset + 10;
  }
  fv->InfoOp->SetDlgPacketNum(fv, bv->PktNum + bv->PktNumOffset);

  if (bv->PktIn[1] != '+')
    BPSendACK(bv); /* Send ack */

  bv->GetPacket = TRUE;
}

static   int BPGet1(PBPVar bv, int *i, LPBYTE b)
{
	if (*i < bv->PktInCount)
	{
		*b = bv->PktIn[*i];
		(*i)++;
		return 1;
	}
	return 0;
}

static BOOL FTCreateFile(PBPVar bv)
{
	TFileIO *file = bv->file;
	PFileVarProto fv = bv->fv;

	fv->InfoOp->SetDlgProtoFileName(fv, bv->FullName);
	bv->FileOpen = file->OpenWrite(file, bv->FullName);
	if (!bv->FileOpen) {
		if (!fv->NoMsg) {
			MessageBox(fv->HMainWin,"Cannot create file",
					   "Tera Term: Error",MB_ICONEXCLAMATION);
		}
		return FALSE;
	}

	bv->ByteCount = 0;
	bv->FileSize = 0;
	if (bv->ProgStat != -1) {
		bv->ProgStat = 0;
	}
	bv->StartTime = GetTickCount();

	return TRUE;
}

static void BPParseTPacket(PBPVar bv)
{
  PFileVarProto fv = bv->fv;
  int i, j, c;
  BYTE b;
//  char Temp[HostNameMaxLength + 1]; // 81(yutaka)
  char Temp[81]; // 81(yutaka)
  TFileIO *fileio = bv->file;
  char *RecievePath;

  switch (bv->PktIn[2]) {
    case 'C': /* Close */
      if (bv->FileOpen)
      {
	fileio->Close(fileio);
	bv->FileOpen = FALSE;
      }
      fv->Success = TRUE;
      bv->BPState = BP_Close;
      break;
    case 'D': /* Download */
      if ((bv->BPState != BP_RecvFile) &&
	  (bv->BPState != BP_AutoFile))
      {
	BPSendFailure(bv,'E');
	return;
      }
      bv->BPMode = IdBPReceive;
      bv->BPState = BP_RecvFile;
      BPDispMode(bv);

      /* Get file name */
      j = 0;
      for (i = 4 ; i <=  bv->PktInCount-1 ; i++)
      {
	b = bv->PktIn[i];
	if (j < sizeof(Temp)-1)
	{
	  Temp[j] = b;
	  j++;
	}
      }
      Temp[j] = 0;

      RecievePath = fv->GetRecievePath(fv);
      free((void *)bv->FullName);
      bv->FullName = fileio->GetReceiveFilename(fileio, Temp, FALSE, RecievePath, !fv->OverWrite);
      free(RecievePath);


      /* file open */
      if (! FTCreateFile(bv))
      {
	BPSendFailure(bv,'E');
	return;
      }
      break;
    case 'I': /* File information */
      i = 5;
      /* file size */
      bv->FileSize = 0;
      do {
	c = BPGet1(bv,&i,&b);
	if ((c==1) &&
	    (b>=0x30) && (b<=0x39))
	  bv->FileSize =
	    bv->FileSize * 10 + b - 0x30;
      } while ((c!=0) && (b>=0x30) && (b<=0x39));
      break;
    case 'U': /* Upload */
      if ((bv->BPState != BP_SendFile) &&
	  (bv->BPState != BP_AutoFile))
      {
	BPSendFailure(bv,'E');
	return;
      }
      bv->BPMode = IdBPSend;
      BPDispMode(bv);

      if (!bv->FileOpen)
      {
	/* Get file name */
	j = 0;
	for (i = 4 ; i <=  bv->PktInCount-1 ; i++)
	{
	  b = bv->PktIn[i];
	  if (j < sizeof(Temp)-1)
	  {
	    Temp[j] = b;
	    j++;
	  }
	}
	Temp[j] = 0;

	RecievePath = fv->GetRecievePath(fv);
	free((void *)bv->FullName);
	bv->FullName = fileio->GetReceiveFilename(fileio, Temp, FALSE, RecievePath, !fv->OverWrite);
	free(RecievePath);

	/* file open */
	if (! BPOpenFileToBeSent(bv))
	{
	  /* if file not found, ask user new file name */
	  free((void *)bv->FullName);
	  bv->FullName = NULL;

	  // ダイアログを開いてファイル名をユーザーに指定してもらう
	  // この位置で行うのは適切でないと思われるため
	  // [cancel]を押したときと同様の動作とした
	  //if (! GetTransFname(fv, NULL, GTF_BP, (PLONG)&i))
	  if (FALSE)
	  {
	    BPSendFailure(bv,'E');
	    return;
	  }
	  /* open retry */
	  if (! BPOpenFileToBeSent(bv))
	  {
	    BPSendFailure(bv,'E');
	    return;
	  }
	}
      }
      bv->ByteCount = 0;

      bv->BPState = BP_SendData;
      BPSendNPacket(bv);
      break;
    default: break;
  }
}

static void BPParsePacket(PBPVar bv)
{
	PFileVarProto fv = bv->fv;
	TFileIO *fileio = bv->file;
	bv->GetPacket = FALSE;
	/* Packet type */

	switch (bv->PktIn[1]) {
    case '+': /* Transport parameters */
		if (bv->BPState==BP_Init)
			BPSendInit(bv);
		else
			BPSendFailure(bv,'E');
		return;
    case 'F': /* Failure */
		if (!fv->NoMsg)
			MessageBox(fv->HMainWin,"Transfer failure",
					   "Tera Term: Error",MB_ICONEXCLAMATION);
		bv->BPState = BP_Close;
		break;
    case 'N': /* Data */
		if ((bv->BPState==BP_RecvFile) &&
			bv->FileOpen)
			bv->BPState = BP_RecvData;
		else if (bv->BPState != BP_RecvData)
		{
			BPSendFailure(bv,'E');
			return;
		}
		fileio->WriteFile(fileio, &(bv->PktIn[2]), bv->PktInCount-2);
		bv->ByteCount = bv->ByteCount +
			bv->PktInCount - 2;
		fv->InfoOp->SetDlgByteCount(fv, bv->ByteCount);
		if (bv->FileSize > 0)
			fv->InfoOp->SetDlgPercent(fv, bv->ByteCount, bv->FileSize, &bv->ProgStat);
		fv->InfoOp->SetDlgTime(fv, bv->StartTime, bv->ByteCount);
		break;
    case 'T':
		BPParseTPacket(bv); /* File transfer */
		break;
	}
}

static void BPParseAck(PBPVar bv, BYTE b)
{
  PFileVarProto fv = bv->fv;
  b = (b - 0x30) % 10;
  if (bv->EnqSent)
  {
    fv->FTSetTimeOut(fv,0);
    bv->EnqSent = FALSE;
    if ((bv->PktOutLen>0) && (b==bv->PktNum)) /* Resend packet */
    {
      bv->PktOutCount = bv->PktOutLen;
      bv->PktOutPtr = 0;
      bv->BPPktState = BP_PktSending;
    }
    return;
  }
  if (bv->PktOutLen==0) return;

  if (b==bv->PktNumSent)
    bv->PktOutLen = 0; /* Release packet */
  else
    return;

  fv->FTSetTimeOut(fv,0);
  bv->PktNum = b;
  if (b==0)
    bv->PktNumOffset = bv->PktNumOffset + 10;

  switch (bv->BPState) {
    case BP_Init:
	switch (bv->BPMode) {
	  case IdBPSend:
	    bv->BPState = BP_SendFile;
	    break;
	  case IdBPReceive:
	    bv->BPState = BP_RecvFile;
	    break;
	  case IdBPAuto:
	    bv->BPState = BP_AutoFile;
	    break;
	}
      break;
    case BP_SendData:
      if (b==bv->PktNumSent)
      {
	if (bv->FileOpen)
	  BPSendNPacket(bv);
	else
	  BPSendTCPacket(bv);
      }
      break;
    case BP_SendClose:
      fv->Success = TRUE;
      bv->BPState = BP_Close;
      break;
    case BP_Failure: bv->BPState = BP_Close; break;
  }
  fv->InfoOp->SetDlgPacketNum(fv, bv->PktNum + bv->PktNumOffset);
}

static void BPDequote(LPBYTE b)
{
	if ((*b>=0x40) && (*b<=0x5f))
		*b = *b - 0x40;
	else if ((*b>=0x60) && (*b<=0x7f))
		*b = *b + 0x20;
}

static BOOL BPParse(TProto *pv)
{
  int c;
  BYTE b;
  PBPVar bv = pv->PrivateData;
  PFileVarProto fv = bv->fv;

  if (bv->BPState==BP_Close) {
    return FALSE;
  }

  do {

    /* Send packet */
    if (bv->BPPktState==BP_PktSending)
    {
      c = 1;
      while ((c>0) && (bv->PktOutCount>0))
      {
	c = BPWrite(bv,&(bv->PktOut[bv->PktOutPtr]),bv->PktOutCount);
	bv->PktOutPtr = bv->PktOutPtr + c;
	bv->PktOutCount = bv->PktOutCount - c;
      }
      if (bv->PktOutCount>0) return TRUE;
      if (bv->cv->OutBuffCount==0)
      {
	bv->BPPktState = BP_PktGetDLE;
	fv->FTSetTimeOut(fv,bv->TimeOut);
      }
      else
	return TRUE;
    }

    /* Get packet */
    c = BPRead1Byte(bv,&b);
    while ((c>0) && (bv->BPPktState!=BP_PktSending) &&
	   ! bv->GetPacket)
    {
      switch (bv->BPPktState) {
	case BP_PktGetDLE:
	  switch (b) {
	    case 0x03: /* ETX */
	      BPSendNAK(bv);
	      break;
	    case 0x05: /* ENQ */
	      if (bv->BPState==BP_Init)
		BPWrite(bv,"\020++\0200",5);
	      else
		BPSendACK(bv);
	      break;
	    case 0x10: /* DLE */
	      bv->BPPktState = BP_PktDLESeen;
	      break;
	    case 0x15: /* NAK */
	      BPWrite(bv,"\005\005",2); /* two ENQs */
	      fv->FTSetTimeOut(fv,bv->TimeOut);
	      bv->EnqSent = TRUE;
	      break;
	  }
	  break;
	case BP_PktDLESeen:
	  switch (b) {
	    case 0x05: /* ENQ */
	      BPSendACK(bv);
	      bv->BPPktState = BP_PktGetDLE;
	      break;
	    case 0x3B: /* Wait */
	      bv->BPPktState = BP_PktGetDLE;
	      break;
	    case 0x42: /* B */
	      bv->PktInCount = 0;
	      bv->Quoted = FALSE;
	      bv->BPPktState = BP_PktGetData;
	      switch (bv->CM) {
		case 1: /* modified XMODEM-CRC */
		  bv->CheckCalc = 0xFFFF;
		  bv->CheckCount = 2;
		  break;
		default: /* standard checksum */
		  /* CCITT CRC-16/32 are not supported */
		  bv->CheckCalc = 0;
		  bv->CheckCount = 1;
		  break;
	      }
	      break;
	    default:
	      if ((b>=0x30) && (b<=0x39))
	      {  /* ACK */
		bv->BPPktState = BP_PktGetDLE;
		BPParseAck(bv,b);
	      }
	      break;
	  }
	  break;
	case BP_PktGetData:
	  switch (b) {
	    case 0x03: /* ETX */
	      BPUpdateCheck(bv,b);
	      bv->Quoted = FALSE;
	      bv->Check = 0;
	      bv->BPPktState = BP_PktGetCheck;
	      break;
	    case 0x05: /* ENQ */
	      BPSendACK(bv);
	      bv->BPPktState = BP_PktGetDLE;
	      break;
	    case 0x10:
	      bv->Quoted = TRUE; /* DLE */
	      break;
	    default:
	      if (bv->Quoted) BPDequote(&b);
	      bv->Quoted = FALSE;
	      if (bv->PktInCount < sizeof(bv->PktIn))
	      {
		BPUpdateCheck(bv,b);
		bv->PktIn[bv->PktInCount] = b;
		bv->PktInCount++;
	      }
	  }
	  break;
	case BP_PktGetCheck:
	  switch (b) {
	    case 0x10: /* DLE */
	      bv->Quoted = TRUE;
	      break;
	    default:
	      if (bv->Quoted) BPDequote(&b);
	      bv->Quoted = FALSE;
	      bv->Check = (bv->Check << 8) + b;
	      bv->CheckCount--;
	      if (bv->CheckCount<=0)
	      {
		bv->BPPktState = BP_PktGetDLE;
		BPCheckPacket(bv);
	      }
	  }
	  break;
      }
      if ((bv->BPPktState != BP_PktSending) &&
	  ! bv->GetPacket)
	c = BPRead1Byte(bv,&b);
      else
	c = 0;
    }

    /* Parse packet */
    if (bv->GetPacket)
      BPParsePacket(bv);

  } while ((c!=0) || (bv->BPPktState==BP_PktSending));

  if (bv->BPState==BP_Close)
    return FALSE;

  return TRUE;
}

static void BPCancel(TProto *pv)
{
	PBPVar bv = pv->PrivateData;
	if ((bv->BPState != BP_Failure) &&
		(bv->BPState != BP_Close))
		BPSendFailure(bv,'A');
	if (! bv->cv->Ready){
		ProtoEnd();	// セッション断の場合は直接 ProtoEnd() を呼んでウィンドウを閉じる
	}
}

static int SetOptV(TProto *pv, int request, va_list ap)
{
	PBPVar bv = pv->PrivateData;
	switch(request) {
	case BPLUS_MODE: {
		int Mode = va_arg(ap, int);
		bv->BPMode = Mode;
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
	PBPVar bv = pv->PrivateData;
	if (bv->log != NULL) {
		TProtoLog* log = bv->log;
		log->Destory(log);
		bv->log = NULL;
	}
	free((void *)bv->FullName);
	bv->FullName = NULL;
	free(bv);
	pv->PrivateData = NULL;
	free(pv);
}

static const TProtoOp Op = {
	BPInit,
	BPParse,
	BPTimeOutProc,
	BPCancel,
	SetOpt,
	SetOptV,
	Destroy,
};

TProto *BPCreate(PFileVarProto fv)
{
	TProto *pv = malloc(sizeof(*pv));
	if (pv == NULL) {
		return NULL;
	}
	pv->Op = &Op;
	PBPVar bv = malloc(sizeof(*bv));
	pv->PrivateData = bv;
	if (bv == NULL) {
		free(pv);
		return NULL;
	}
	memset(bv, 0, sizeof(*bv));
	bv->FileOpen = FALSE;
	bv->Comm = fv->Comm;
	bv->file = fv->file_fv;
	bv->fv = fv;

	return pv;
}
