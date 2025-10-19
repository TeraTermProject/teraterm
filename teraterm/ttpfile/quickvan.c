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

/* Quick-VAN protocol */
#include <stdio.h>
#include <string.h>

#include "tttypes.h"
#include "ftlib.h"
#include "protolog.h"

#include "quickvan.h"

/* Quick-VAN */
typedef struct {
	BYTE PktIn[142], PktOut[142];
	int PktInCount, PktInPtr;
	int PktOutCount, PktOutPtr, PktOutLen;
	WORD Ver, WinSize;
	QV_MODE_T QVMode;
	WORD QVState, PktState;
	WORD AValue;
	WORD SeqNum;
	WORD FileNum;
	int RetryCount;
	BOOL CanFlag;
	WORD Year,Month,Day,Hour,Min,Sec;
	WORD SeqSent, WinEnd, FileEnd;
	BOOL EnqFlag;
	BYTE CheckSum;
	TProtoLog *log;
	const char *FullName;	// Windows上のファイル名 UTF-8
	WORD LogState;

	BOOL FileOpen;
	LONG FileSize;
	LONG ByteCount;

	int ProgStat;

	DWORD StartTime;

	DWORD FileMtime;

	TComm *Comm;
	TFileIO *file;
	PFileVarProto fv;
} TQVVar;
typedef TQVVar *PQVVar;

  /* Quick-VAN states */
#define QV_RecvInit1 1
#define QV_RecvInit2 2
#define QV_RecvData 3
#define QV_RecvDataRetry 4
#define QV_RecvNext 5
#define QV_RecvEOT 6
#define QV_Cancel 7
#define QV_Close 8

#define QV_SendInit1 11
#define QV_SendInit2 12
#define QV_SendInit3 13
#define QV_SendData 14
#define QV_SendDataRetry 15
#define QV_SendNext 16
#define QV_SendEnd 17

#define QVpktSOH 1
#define QVpktBLK 2
#define QVpktBLK2 3
#define QVpktDATA 4

#define QVpktSTX 5
#define QVpktCR 6

#define TimeOutCAN 1
#define TimeOutCANSend 2
#define TimeOutRecv 20
#define TimeOutSend 60
#define TimeOutEOT 5

#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

static int QVRead1Byte(PQVVar qv, LPBYTE b)
{
  if (qv->Comm->op->Read1Byte(qv->Comm, b) == 0)
    return 0;

  if (qv->log != NULL) {
	TProtoLog *log = qv->log;
    if (qv->LogState!=1)
    {
		qv->LogState = 1;
		log->WriteRaw(log, "\015\012<<<\015\012", 7);
    }
    log->DumpByte(log, *b);
  }
  return 1;
}

static int QVWrite(PQVVar qv, PCHAR B, int C)
{
  int i, j;

  i = qv->Comm->op->BinaryOut(qv->Comm, B, C);

  if (qv->log != NULL && (i>0)) {
	  TProtoLog* log = qv->log;
	  if (qv->LogState != 0) {
		  qv->LogState = 0;
		  log->WriteRaw(log, "\015\012>>>\015\012", 7);
    }
    for (j=0 ; j <= i-1 ; j++)
      log->DumpByte(log, B[j]);
  }
  return i;
}

static void QVSendNAK(PQVVar qv)
{
  PFileVarProto fv = qv->fv;
  BYTE b;

  b = NAK;
  QVWrite(qv,&b, 1);
  fv->FTSetTimeOut(fv,TimeOutRecv);
}

static void QVSendACK(PQVVar qv)
{
  PFileVarProto fv = qv->fv;
  BYTE b;

  fv->FTSetTimeOut(fv,0);
  b = ACK;
  QVWrite(qv, &b, 1);
  qv->QVState = QV_Close;
}

static BOOL QVInit(TProto *pv, PComVar cv, PTTSet ts)
{
  PQVVar qv = pv->PrivateData;
  PFileVarProto fv = qv->fv;
  (void)cv;

  qv->WinSize = ts->QVWinSize;

  if ((ts->LogFlag & LOG_QV)!=0) {
	  TProtoLog* log = ProtoLogCreate();
	  qv->log = log;
	  log->SetFolderW(log, ts->LogDirW);
	  log->Open(log, "QUICKVAN.LOG");
	  qv->LogState = 2;
  }

  qv->FileOpen = FALSE;
  qv->ByteCount = 0;

  fv->InfoOp->SetDlgProtoText(fv, "Quick-VAN");

  fv->InfoOp->InitDlgProgress(fv, &qv->ProgStat);
  qv->StartTime = GetTickCount();

  qv->SeqNum = 0;
  qv->FileNum = 0;
  qv->CanFlag = FALSE;
  qv->PktOutCount = 0;
  qv->PktOutPtr = 0;

  qv->Ver = 1; /* version */

  switch (qv->QVMode) {
    case IdQVSend:
      qv->QVState = QV_SendInit1;
      qv->PktState = QVpktSTX;
      fv->FTSetTimeOut(fv,TimeOutSend);
      break;
    case IdQVReceive:
      qv->QVState = QV_RecvInit1;
      qv->PktState = QVpktSOH;
      qv->RetryCount = 10;
      QVSendNAK(qv);
      break;
    default:
      return FALSE;
  }

  return TRUE;
}

static void QVCancel_(PQVVar qv)
{
  BYTE b;
  PFileVarProto fv = qv->fv;

  if ((qv->QVState==QV_Close) ||
      (qv->QVState==QV_RecvEOT) ||
      (qv->QVState==QV_Cancel))
    return;

  /* flush send buffer */
  qv->PktOutCount = 0;
  /* send CAN */
  b = CAN;
  QVWrite(qv, &b, 1);

  if ((qv->QVMode==IdQVReceive) &&
      ((qv->QVState==QV_RecvData) ||
       (qv->QVState==QV_RecvDataRetry)))
  {
    fv->FTSetTimeOut(fv,TimeOutEOT);
    qv->QVState = QV_RecvEOT;
    return;
  }
  fv->FTSetTimeOut(fv,TimeOutCANSend);
  qv->QVState = QV_Cancel;
}

static void QVCancel(TProto *pv)
{
  PQVVar qv = pv->PrivateData;
  QVCancel_(qv);
}

static BOOL QVCountRetry(PQVVar qv)
{
  qv->RetryCount--;
  if (qv->RetryCount<=0)
  {
    QVCancel_(qv);
    return TRUE;
  }
  else
    return FALSE;
}

static void QVResendPacket(PQVVar qv)
{
  PFileVarProto fv = qv->fv;

  qv->PktOutCount = qv->PktOutLen;
  qv->PktOutPtr = 0;
  if (qv->QVMode==IdQVReceive)
    fv->FTSetTimeOut(fv,TimeOutRecv);
  else
    fv->FTSetTimeOut(fv,TimeOutSend);
}

static void QVSetResPacket(PQVVar qv, BYTE Typ, BYTE Num, int DataLen)
{
  int i;
  BYTE Sum;

  qv->PktOut[0] = STX;
  qv->PktOut[1] = Typ;
  qv->PktOut[2] = Num | 0x80;
  Sum = 0;
  for (i = 0 ; i <= 2 + DataLen ; i++)
    Sum = Sum + qv->PktOut[i];
  qv->PktOut[3+DataLen] = Sum | 0x80;
  qv->PktOut[4+DataLen] = 0x0D;
  qv->PktOutCount = 5 + DataLen;
  qv->PktOutLen = qv->PktOutCount;
  qv->PktOutPtr = 0;
}

static void QVSendVACK(PQVVar qv, BYTE Seq)
{
  PFileVarProto fv = qv->fv;
  fv->FTSetTimeOut(fv,TimeOutRecv);
  qv->RetryCount = 10;
  qv->QVState = QV_RecvData;
  if (((int)qv->SeqNum % (int)qv->AValue) == 0)
    QVSetResPacket(qv,'A',Seq,0);
}

static void QVSendVNAK(PQVVar qv)
{
  PFileVarProto fv = qv->fv;
  fv->FTSetTimeOut(fv,TimeOutRecv);
  QVSetResPacket(qv,'N',LOBYTE(qv->SeqNum+1),0);
  if (qv->QVState==QV_RecvData)
  {
    qv->RetryCount = 10;
    qv->QVState = QV_RecvDataRetry;
  }
}

static void QVSendVSTAT(PQVVar qv)
{
  PFileVarProto fv = qv->fv;
  fv->FTSetTimeOut(fv,TimeOutRecv);
  qv->PktOut[3] = 0x30;
  QVSetResPacket(qv,'T',LOBYTE(qv->SeqNum),1);
  qv->RetryCount = 10;
  qv->QVState = QV_RecvNext;
}

static void QVTimeOutProc(TProto *pv)
{
  PQVVar qv = pv->PrivateData;
  if ((qv->QVState==QV_Cancel) ||
      (qv->QVState==QV_RecvEOT))
  {
    qv->QVState = QV_Close;
    return;
  }

  if (qv->QVMode==IdQVSend)
  {
    QVCancel_(qv);
    return;
  }

  if (qv->CanFlag)
  {
    qv->CanFlag = FALSE;
    qv->QVState = QV_Close;
    return;
  }

  if ((qv->QVState != QV_RecvData) &&
      QVCountRetry(qv)) return;

  qv->PktState = QVpktSOH;
  switch (qv->QVState) {
    case QV_RecvInit1:
      QVSendNAK(qv);
      break;
    case QV_RecvInit2:
      QVResendPacket(qv); /* resend RINIT */
      break;
    case QV_RecvData:
    case QV_RecvDataRetry:
      if (qv->SeqNum==0)
	QVResendPacket(qv); /* resend RPOS */
      else
	QVSendVNAK(qv);
      break;
    case QV_RecvNext:
      QVResendPacket(qv); /* resend VSTAT */
      break;
  }
}

static BOOL QVParseSINIT(PQVVar qv)
{
  PFileVarProto fv = qv->fv;
  int i;
  BYTE b, n;
  WORD WS;

  if (qv->QVState != QV_RecvInit1)
    return TRUE;

  for (i = 0 ; i<= 5 ; i++)
  {
    b = qv->PktIn[3+i];
    if ((i==5) && (b==0))
      b = 0x30;
    if ((b<0x30) || (b>0x39)) return FALSE;
    n = b - 0x30;
    switch (i) {
      case 0:
	if (n < qv->Ver) qv->Ver = n;
	break;
      case 2:
	WS = n;
	break;
      case 3:
	WS = WS*10 + (WORD)n;
	if (WS < qv->WinSize)
	  qv->WinSize = WS;
	break;
    }
  }
  qv->AValue = qv->WinSize / 2;
  if (qv->AValue==0) qv->AValue = 1;

  /* Send RINIT */
  qv->PktOut[3] = qv->Ver + 0x30;
  qv->PktOut[4] = 0x30;
  qv->PktOut[5] = qv->WinSize / 10 + 0x30;
  qv->PktOut[6] = qv->WinSize % 10 + 0x30;
  qv->PktOut[7] = 0x30;
  qv->PktOut[8] = 0;
  QVSetResPacket(qv,'R',0,6);
  qv->QVState = QV_RecvInit2;
  qv->RetryCount = 10;
  fv->FTSetTimeOut(fv,TimeOutRecv);

  return TRUE;
}

static BOOL QVGetNum2(PQVVar qv, int *i, LPWORD w)
{
    BOOL Ok;
    int j;
    BYTE b;

    *w = 0;
    Ok = FALSE;
    for (j = *i ; j <= *i + 1 ; j++)
    {
      b = qv->PktIn[j];
      if ((b>=0x30) && (b<=0x39))
      {
	*w = *w*10 + (WORD)(b - 0x30);
	Ok = TRUE;
      }
    }
    *i = *i + 2;
    return Ok;
}

static BOOL FTCreateFile(PQVVar qv)
{
	PFileVarProto fv = qv->fv;
	TFileIO *file = qv->file;

	fv->InfoOp->SetDlgProtoFileName(fv, qv->FullName);
	qv->FileOpen = file->OpenWrite(file, qv->FullName);
	if (!qv->FileOpen) {
		if (fv->NoMsg) {
			MessageBox(fv->HMainWin,"Cannot create file",
					   "Tera Term: Error",MB_ICONEXCLAMATION);
		}
		return FALSE;
	}

	qv->ByteCount = 0;
	qv->FileSize = 0;
	if (qv->ProgStat != -1) {
		qv->ProgStat = 0;
	}
	qv->StartTime = GetTickCount();

	return TRUE;
}

static BOOL QVParseVFILE(PQVVar qv)
{
  PFileVarProto fv = qv->fv;
  TFileIO *file = qv->file;
  int i;
  WORD w;
  BYTE b;
  char *RecievePath;

  if ((qv->QVState != QV_RecvInit2) &&
      (qv->QVState != QV_RecvNext))
    return TRUE;

  /* file name */
  RecievePath = fv->GetRecievePath(fv);
  free((void *)qv->FullName);
  qv->FullName = file->GetRecieveFilename(file, &(qv->PktIn[5]), FALSE, RecievePath, !fv->OverWrite);
  free(RecievePath);
  /* file open */
  if (! FTCreateFile(qv)) return FALSE;
  /* file size */
  i = strlen(&(qv->PktIn[5])) + 6;
  do {
    b = qv->PktIn[i];
    if ((b>=0x30) && (b<=0x39))
		qv->FileSize = qv->FileSize * 10 + b - 0x30;
    i++;
  } while ((b>=0x30) && (b<=0x39));
  /* year */
  if (QVGetNum2(qv,&i,&w))
  {
    qv->Year = w * 100;
    if (QVGetNum2(qv,&i,&w))
      qv->Year = qv->Year + w;
    else
      qv->Year = 0;
  }
  else qv->Year = 0;
  /* month */
  QVGetNum2(qv,&i,&(qv->Month));
  /* date */
  QVGetNum2(qv,&i,&(qv->Day));
  /* hour */
  if (! QVGetNum2(qv,&i,&(qv->Hour)))
    qv->Hour = 24;
  /* min */
  QVGetNum2(qv,&i,&(qv->Min));
  /* sec */
  QVGetNum2(qv,&i,&(qv->Sec));

  fv->InfoOp->SetDlgByteCount(fv, 0);
  if (qv->FileSize > 0)
	  fv->InfoOp->SetDlgPercent(fv, 0, qv->FileSize, &qv->ProgStat);
  fv->InfoOp->SetDlgTime(fv, qv->StartTime, qv->ByteCount);

  /* Send VRPOS */
  QVSetResPacket(qv,'P',0,0);
  qv->QVState = QV_RecvData;
  qv->SeqNum = 0;
  qv->RetryCount = 10;
  fv->FTSetTimeOut(fv,TimeOutRecv);

  return TRUE;
}

static BOOL QVParseVENQ(PQVVar qv)
{
  TFileIO *file = qv->file;
  struct tm time;
  struct utimbuf timebuf;

  if ((qv->QVState != QV_RecvData) &&
      (qv->QVState != QV_RecvDataRetry))
    return TRUE;

  if (qv->PktIn[3]==LOBYTE(qv->SeqNum))
  {
    if (qv->PktIn[4]==0x30)
    {
      if (qv->FileOpen)
      {
	file->Close(file);
	qv->FileOpen = FALSE;
	/* set file date & time */
	if ((qv->Year >= 1900) && (qv->Hour < 24))
	{
	  time.tm_year = qv->Year-1900;
	  time.tm_mon = qv->Month-1;
	  time.tm_mday = qv->Day;
	  time.tm_hour = qv->Hour;
	  time.tm_min = qv->Min;
	  time.tm_sec = qv->Sec;
	  time.tm_wday = 0;
	  time.tm_yday = 0;
	  time.tm_isdst = 0;
	  timebuf.actime = mktime(&time);
	  timebuf.modtime = timebuf.actime;
	  utime(qv->FullName,&timebuf);
	}
      }
      QVSendVSTAT(qv);
    }
    else
      return FALSE; /* exit and cancel */
  }
  else {
    if (qv->QVState==QV_RecvDataRetry)
    {
      qv->RetryCount--;
      if (qv->RetryCount<0)
	return FALSE; /* exit and cancel */
    }
    QVSendVNAK(qv);
  }

  return TRUE;
}

static void QVWriteToFile(PQVVar qv)
{
  PFileVarProto fv = qv->fv;
  TFileIO *file = qv->file;
  int C;

  if (qv->FileSize - qv->ByteCount < 128)
    C = qv->FileSize - qv->ByteCount;
  else
    C = 128;
  file->WriteFile(file,&(qv->PktIn[3]),C);
  qv->ByteCount = qv->ByteCount + C;

  fv->InfoOp->SetDlgPacketNum(fv, qv->SeqNum);
  fv->InfoOp->SetDlgByteCount(fv, qv->ByteCount);
  if (qv->FileSize > 0)
    fv->InfoOp->SetDlgPercent(fv, qv->ByteCount, qv->FileSize, &qv->ProgStat);
  fv->InfoOp->SetDlgTime(fv, qv->StartTime, qv->ByteCount);
}

static BOOL QVCheckWindow8(WORD w0, WORD w1, BYTE b, LPWORD  w)
{
  WORD i;

  for (i = w0 ; i <= w1 ; i++)
    if (LOBYTE(i)==b)
    {
      *w = i;
      return TRUE;
    }
  return FALSE;
}

static BOOL QVReadPacket(PQVVar qv)
{
  BYTE b;
  WORD w0, w1, w;
  int c;
  BOOL GetPkt, EOTFlag, Ok;
  PFileVarProto fv = qv->fv;

  if (qv->QVState == QV_Close)
    return FALSE;

  c = 1;
  while ((c>0) && (qv->PktOutCount>0))
  {
	c = QVWrite(qv, &(qv->PktOut[qv->PktOutPtr]), qv->PktOutCount);
    qv->PktOutPtr = qv->PktOutPtr + c;
    qv->PktOutCount = qv->PktOutCount - c;
  }

  c = QVRead1Byte(qv,&b);
  if ((c>0) && qv->CanFlag)
  {
    qv->CanFlag = FALSE;
    fv->FTSetTimeOut(fv,TimeOutRecv);
  }

  EOTFlag = FALSE;
  GetPkt = FALSE;
  while ((c>0) && (! GetPkt))
  {
    qv->CanFlag = (b==CAN) && (qv->PktState != QVpktDATA);
    EOTFlag = (b==EOT) && (qv->PktState != QVpktDATA);

    switch (qv->PktState) {
      case QVpktSOH:
	if (b==SOH)
	{
	  qv->PktIn[0] = b;
	  qv->PktState = QVpktBLK;
	}
	break;
      case QVpktBLK:
	qv->PktIn[1] = b;
	qv->PktState = QVpktBLK2;
	break;
      case QVpktBLK2:
	qv->PktIn[2] = b;
	if ((qv->PktIn[1]==0) &&
	    (b>=0x30) && (b<=0x32)) /* function block */
	{
	  qv->CheckSum = SOH + b;
	  qv->PktInPtr = 3;
	  qv->PktInCount = 129;
	  qv->PktState = QVpktDATA;
	}
	else if ((qv->QVState==QV_RecvData) &&
		 ((b ^ qv->PktIn[1]) == 0xff))
	{
	  if (qv->SeqNum+1 < qv->WinSize)
	    w0 = 0;
	  else
	    w0 = qv->SeqNum + 1 - qv->WinSize;
	  w1 = qv->SeqNum+1;
	  if (((qv->SeqNum==0) && (qv->PktIn[1]==1)) ||
	      ((qv->SeqNum>0) &&
		   QVCheckWindow8(w0,w1,qv->PktIn[1],&w)))
	  {
	    qv->CheckSum = 0;
	    qv->PktInPtr = 3;
	    qv->PktInCount = 129;
	    qv->PktState = QVpktDATA;
	  }
	  else {
	    qv->PktState =QVpktSOH;
	    QVSendVNAK(qv);
	  }
	}
	else if ((qv->QVState==QV_RecvDataRetry) &&
		 ((b ^ qv->PktIn[1]) == 0xff))
	{
	  if (qv->PktIn[1]==LOBYTE(qv->SeqNum+1))
	  {
	    qv->CheckSum = 0;
	    qv->PktInPtr = 3;
	    qv->PktInCount = 129;
	    qv->PktState = QVpktDATA;
	  }
	  else {
	    qv->PktState =QVpktSOH;
	    fv->FTSetTimeOut(fv,TimeOutRecv);
	  }
	}
	else
	  qv->PktState =QVpktSOH;
	break;
      case QVpktDATA:
	qv->PktIn[qv->PktInPtr] = b;
	qv->PktInPtr++;
	qv->PktInCount--;
	GetPkt = qv->PktInCount==0;
	if (GetPkt)
	 qv->PktState = QVpktSOH;
	else
	  qv->CheckSum = qv->CheckSum + b;
	break;
      default:
	qv->PktState = QVpktSOH;
    }

    if (! GetPkt) c = QVRead1Byte(qv,&b);
  }

  if (! GetPkt)
  {
    if (qv->CanFlag)
      fv->FTSetTimeOut(fv,TimeOutCAN);

    if (EOTFlag)
      switch (qv->QVState) {
	case QV_RecvInit2:
	case QV_RecvNext:
	  QVSendACK(qv);
	  fv->Success = TRUE;
	  return TRUE;
	case QV_RecvEOT:
	  qv->QVState = QV_Close;
	  break;
      }
    return TRUE;
  }

  if ((qv->PktIn[1]==0) &&
      (qv->PktIn[2]>=0x30) && (qv->PktIn[2]<=0x32))
  { /* function block */
    if (qv->CheckSum != qv->PktIn[qv->PktInPtr-1])
    { /* bad checksum */
      switch (qv->QVState) {
	case QV_RecvInit1:
	  if ((qv->PktIn[2]==0x30) && /* SINIT */
	      ! QVCountRetry(qv))
	    QVSendNAK(qv);
	  break;
	case QV_RecvInit2:
	case QV_RecvNext:
	  if ((qv->PktIn[2]==0x31) && /* VFILE */
	      ! QVCountRetry(qv))
	    QVResendPacket(qv);
	  break;
	case QV_RecvData:
	  if (qv->PktIn[2]==0x32) /* VENQ */
	    QVSendVNAK(qv);
	  break;
	case QV_RecvDataRetry:
	  if ((qv->PktIn[2]==0x32) && /* VENQ */
	      ! QVCountRetry(qv))
	    QVSendVNAK(qv);
	  break;
      }
      return TRUE;
    }
    Ok = FALSE;
    switch (qv->PktIn[2]) { /* function type */
      case 0x30:
	Ok = QVParseSINIT(qv);
	break;
      case 0x31:
	Ok = QVParseVFILE(qv);
	break;
      case 0x32:
	Ok = QVParseVENQ(qv);
	break;
    }
    if (! Ok)
      QVCancel_(qv);
  }
  else { /* VDAT block */
    if ((qv->QVState != QV_RecvData) &&
	(qv->QVState != QV_RecvDataRetry))
      return TRUE;
    if (qv->PktIn[1] != LOBYTE(qv->SeqNum+1))
      QVSendVACK(qv,qv->PktIn[1]);
    else if (qv->CheckSum==qv->PktIn[qv->PktInPtr-1])
    {
      QVSendVACK(qv,qv->PktIn[1]);
      qv->SeqNum++;
      QVWriteToFile(qv);
    }
    else /* bad checksum */
      QVSendVNAK(qv);
  }

  return TRUE;
}

static void QVSetPacket(PQVVar qv, BYTE Num, BYTE Typ)
{
  int i;

  qv->PktOut[0] = SOH;
  qv->PktOut[1] = Num;
  qv->PktOut[2] = Typ;
  if ((Num ^ Typ) == 0xFF)
    qv->CheckSum = 0;
  else
    qv->CheckSum = SOH + Num + Typ;
  for (i = 3 ; i <= 130 ; i++)
    qv->CheckSum = qv->CheckSum + qv->PktOut[i];
  qv->PktOut[131] = qv->CheckSum;
  qv->PktOutLen = 132;
  qv->PktOutCount = 132;
  qv->PktOutPtr = 0;
}

static void QVSendSINIT(PQVVar qv)
{
  PFileVarProto fv = qv->fv;
  int i;

  qv->PktOut[3] = qv->Ver + 0x30;
  qv->PktOut[4] = 0x30;
  qv->PktOut[5] = qv->WinSize / 10 + 0x30;
  qv->PktOut[6] = qv->WinSize % 10 + 0x30;
  qv->PktOut[7] = 0x30;
  qv->PktOut[8] = 0;
  for (i = 6 ; i <= 127 ; i++)
    qv->PktOut[3+i] = 0;
  /* send SINIT */
  QVSetPacket(qv,0,0x30);
  qv->QVState = QV_SendInit2;
  qv->PktState = QVpktSTX;
  fv->FTSetTimeOut(fv,TimeOutSend);
}

static void QVSendEOT(PQVVar qv)
{
  BYTE b;
  PFileVarProto fv = qv->fv;

  if (qv->QVState==QV_SendEnd)
  {
    if (QVCountRetry(qv))
      return;
  }
  else {
    qv->RetryCount = 10;
    qv->QVState = QV_SendEnd;
  }

  b = EOT;
  QVWrite(qv, &b, 1);
  fv->FTSetTimeOut(fv,TimeOutSend);
}

static void QVPutNum2(PQVVar qv, WORD Num, int *i)
{
    qv->PktOut[*i] = Num / 10 + 0x30;
    (*i)++;
    qv->PktOut[*i] = Num % 10 + 0x30;
    (*i)++;
}

static void QVSendVFILE(PQVVar qv)
{
  PFileVarProto fv = qv->fv;
  int i, j;
  struct stat stbuf;
  struct tm tmbuf;
  BOOL r;
  TFileIO *file = qv->file;
  char *filename;

  filename = fv->GetNextFname(fv);
  if (filename == NULL)
  {
    QVSendEOT(qv);
    return;
  }
  free((void *)qv->FullName);
  qv->FullName = filename;

  /* find file and get file info */
  qv->FileSize = file->GetFSize(file, qv->FullName);
  if (qv->FileSize > 0)
  {
	  qv->FileEnd = (WORD)(qv->FileSize >> 7);
    if ((qv->FileSize & 0x7F) != 0)
      qv->FileEnd++;
  }
  else {
    QVCancel_(qv);
    return;
  }

  /* file open */
  r = file->OpenRead(file, qv->FullName);
  qv->FileOpen = r;
  if (!qv->FileOpen)
  {
    QVCancel_(qv);
    return;
  }
  /* file no. */
  qv->FileNum++;
  qv->ProgStat = 0;
  qv->StartTime = GetTickCount();
  i = 3;
  QVPutNum2(qv,qv->FileNum,&i);
  /* file name */
  fv->InfoOp->SetDlgProtoFileName(fv, qv->FullName);
  filename = file->GetSendFilename(file, qv->FullName, FALSE, TRUE, TRUE);
  strncpy_s(&(qv->PktOut[i]),sizeof(qv->PktOut)-i,filename,_TRUNCATE);
  i = strlen(&(qv->PktOut[i])) + i;
  qv->PktOut[i] = 0;
  i++;
  free(filename);
  /* file size */
  _snprintf_s(&(qv->PktOut[i]), sizeof(qv->PktOut) - i, _TRUNCATE, "%u", qv->FileSize);
  i = strlen(&(qv->PktOut[i])) + i;
  qv->PktOut[i] = 0;
  i++;
  /* date */
  stat(qv->FullName,&stbuf);
  localtime_s(&tmbuf, &stbuf.st_mtime);

  QVPutNum2(qv,(WORD)((tmbuf.tm_year+1900) / 100),&i);
  QVPutNum2(qv,(WORD)((tmbuf.tm_year+1900) % 100),&i);
  QVPutNum2(qv,(WORD)(tmbuf.tm_mon+1),&i);
  QVPutNum2(qv,(WORD)tmbuf.tm_mday,&i);
  /* time */
  QVPutNum2(qv,(WORD)tmbuf.tm_hour,&i);
  QVPutNum2(qv,(WORD)tmbuf.tm_min,&i);
  QVPutNum2(qv,(WORD)tmbuf.tm_sec,&i);
  for (j = i ; j <= 130 ; j++)
    qv->PktOut[j] = 0;

  /* send VFILE */
  QVSetPacket(qv,0,0x31);
  if (qv->FileNum==1)
    qv->QVState = QV_SendInit3;
  else
    qv->QVState = QV_SendNext;
  qv->PktState = QVpktSTX;
  fv->FTSetTimeOut(fv,TimeOutSend);
}

static void QVSendVDATA(PQVVar qv)
{
  PFileVarProto fv = qv->fv;
  int i, C;
  LONG Pos;
  TFileIO *file = qv->file;

  if ((qv->QVState != QV_SendData) &&
      (qv->QVState != QV_SendDataRetry))
    return;

  if ((qv->SeqSent < qv->WinEnd) &&
      (qv->SeqSent < qv->FileEnd) &&
      ! qv->EnqFlag)
  {
    qv->SeqSent++;
    Pos = (LONG)(qv->SeqSent-1) << 7;
    if (qv->SeqSent==qv->FileEnd)
      C = qv->FileSize - Pos;
    else
      C = 128;
    /* read data from file */
    file->Seek(file, Pos);
    file->ReadFile(file,&(qv->PktOut[3]),C);
	qv->ByteCount = Pos + (LONG)C;
    fv->InfoOp->SetDlgPacketNum(fv, qv->SeqSent);
	fv->InfoOp->SetDlgByteCount(fv, qv->ByteCount);
	fv->InfoOp->SetDlgPercent(fv, qv->ByteCount, qv->FileSize, &qv->ProgStat);
	fv->InfoOp->SetDlgTime(fv, qv->StartTime, qv->ByteCount);
    for (i = C ; i <= 127 ; i++)
      qv->PktOut[3+i] = 0;
    /* send VDAT */
    QVSetPacket(qv,LOBYTE(qv->SeqSent),LOBYTE(~ qv->SeqSent));
    if (qv->SeqSent==qv->WinEnd) /* window close */
      fv->FTSetTimeOut(fv,TimeOutSend);
  }
  else if ((qv->SeqSent==qv->FileEnd) &&
	   ! qv->EnqFlag)
  {
    qv->PktOut[3] = LOBYTE(qv->FileEnd);
    qv->PktOut[4] = 0x30;
    for (i = 2 ; i <= 127 ; i++)
      qv->PktOut[3+i] = 0;
    /* send VENQ */
    QVSetPacket(qv,0,0x32);
    fv->FTSetTimeOut(fv,TimeOutSend);
    qv->EnqFlag = TRUE;
  }

}

static void QVParseRINIT(PQVVar qv)
{
  int i;
  BYTE b, n;
  WORD WS;
  BOOL Ok;

  if (qv->PktIn[2] != 0x80) return;

  Ok = TRUE;
  for (i = 0 ; i <= 3 ; i++)
  {
    b = qv->PktIn[3+i];
    if ((b<0x30) || (b>0x39))
      Ok = FALSE;
    n = b - 0x30;
    switch (i) {
      case 0:
	if (n < qv->Ver) qv->Ver = n;
	break;
      case 2:
	WS = n;
	break;
      case 3:
	WS = WS*10 + (WORD)n;
	if (WS < qv->WinSize)
	  qv->WinSize = WS;
	break;
    }
  }
  if (! Ok)
  {
    QVCancel_(qv);
    return;
  }

  /* Send VFILE */
  qv->RetryCount = 10;
  QVSendVFILE(qv);
}

static void QVParseVRPOS(PQVVar qv)
{
  int i;
  BYTE b;
  PFileVarProto fv = qv->fv;

  qv->SeqNum = 0;
  if (qv->PktInPtr-3 >= 3)
    for (i = 3 ; i <= qv->PktInPtr-3 ; i++)
    {
      b = qv->PktIn[i];
      if ((b<0x30) || (b>0x39))
      {
	QVCancel_(qv);
	return;
      }
      qv->SeqNum = qv->SeqNum * 10 + (WORD)(b - 0x30);
    }

  if (qv->SeqNum >= qv->FileEnd)
  {
    QVCancel_(qv);
    return;
  }

  qv->SeqSent = qv->SeqNum;
  qv->WinEnd = qv->SeqNum + qv->WinSize;
  if (qv->WinEnd > qv->FileEnd)
    qv->WinEnd = qv->FileEnd;
  qv->EnqFlag = FALSE;
  qv->QVState = QV_SendData;
  fv->FTSetTimeOut(fv,0);
}

static BOOL QVCheckWindow7(WORD w0, WORD w1, BYTE b, LPWORD w)
{
  WORD i;

  for (i = w0 ; i <= w1 ; i++)
    if ((i & 0x7F)==(b & 0x7F))
    {
      *w = i;
      return TRUE;
    }
  return FALSE;
}

static void QVParseVACK(PQVVar qv)
{
  WORD w;
  PFileVarProto fv = qv->fv;

  if (QVCheckWindow7((WORD)(qv->SeqNum+1),qv->SeqSent,qv->PktIn[2],&w))
  {
    fv->FTSetTimeOut(fv,0);
    qv->SeqNum = w;
    qv->WinEnd = qv->SeqNum + qv->WinSize;
    if (qv->WinEnd > qv->FileEnd)
      qv->WinEnd = qv->FileEnd;
    if (qv->QVState==QV_SendDataRetry)
    {
      qv->RetryCount = 10;
      qv->QVState = QV_SendData;
    }
  }
}

static void QVParseVNAK(PQVVar qv)
{
  WORD w;
  PFileVarProto fv = qv->fv;

  if ((qv->QVState==QV_SendDataRetry) &&
      (qv->PktIn[1]==LOBYTE(qv->SeqNum+1)))
  {
    fv->FTSetTimeOut(fv,0);
    if (QVCountRetry(qv)) return;
    qv->SeqSent = qv->SeqNum;
    qv->WinEnd = qv->SeqNum + qv->WinSize;
    if (qv->WinEnd > qv->FileEnd)
      qv->WinEnd = qv->FileEnd;
    qv->EnqFlag = FALSE;
    return;
  }

  if (QVCheckWindow7((WORD)(qv->SeqNum+1),(WORD)(qv->SeqSent+1),qv->PktIn[2],&w))
  {
    fv->FTSetTimeOut(fv,0);
    qv->SeqNum = w-1;
    qv->SeqSent = qv->SeqNum;
    qv->WinEnd = qv->SeqNum + qv->WinSize;
    if (qv->WinEnd > qv->FileEnd)
      qv->WinEnd = qv->FileEnd;
    qv->EnqFlag = FALSE;
    qv->RetryCount = 10;
    qv->QVState = QV_SendDataRetry;
  }
}

static void QVParseVSTAT(PQVVar qv)
{
  if (qv->EnqFlag && (qv->PktIn[3]==0x30))
  {
	TFileIO *file = qv->file;
    if (qv->FileOpen)
      file->Close(file);
	qv->FileOpen = FALSE;
    qv->EnqFlag = FALSE;
    qv->RetryCount = 10;
    QVSendVFILE(qv);
  }
  else
    QVCancel_(qv);
}

static BOOL QVSendPacket(PQVVar qv)
{
  BYTE b;
  int c, i;
  BOOL GetPkt;
  PFileVarProto fv = qv->fv;

  if (qv->QVState == QV_Close)
    return FALSE;

  c = QVRead1Byte(qv,&b);
  if ((c==0) && qv->CanFlag)
  {
    if ((qv->QVState==QV_SendData) ||
	(qv->QVState==QV_SendDataRetry))
    {
      b = EOT;
	  QVWrite(qv, &b, 1);
    }
    qv->QVState = QV_Close;
    return FALSE;
  }
  qv->CanFlag = FALSE;

  GetPkt = FALSE;
  while ((c>0) && (! GetPkt))
  {
    qv->CanFlag = (b==CAN) && (qv->PktState != QVpktCR);

    if (b==NAK)
      switch (qv->QVState) {
	case QV_SendInit1:
	  qv->RetryCount = 10;
	  QVSendSINIT(qv);
	  break;
	case QV_SendInit2:
	  if (QVCountRetry(qv))
	    return TRUE;
	  QVSendSINIT(qv);
	  break;
      }

    if (qv->QVState==QV_SendEnd)
    {
      if (b==ACK)
      {
	fv->Success = TRUE;
	return FALSE;
      }
      QVSendEOT(qv);
    }

    switch (qv->PktState) {
      case QVpktSTX:
	if (b==STX)
	{
	  qv->PktIn[0] = b;
	  qv->PktInPtr = 1;
	  qv->PktState = QVpktCR;
	}
	break;
      case QVpktCR:
	qv->PktIn[qv->PktInPtr] = b;
	qv->PktInPtr++;
	GetPkt = b==CR;
	if (GetPkt || (qv->PktInPtr>=128))
	  qv->PktState = QVpktSTX;
	break;
      default:
	qv->PktState = QVpktSTX;
    }
    if (! GetPkt) c = QVRead1Byte(qv,&b);
  }

  if (GetPkt)
  {
    qv->CheckSum = 0;
    for (i = 0 ; i <= qv->PktInPtr-3 ; i++)
      qv->CheckSum = qv->CheckSum + qv->PktIn[i];
    GetPkt = (qv->CheckSum | 0x80) == qv->PktIn[qv->PktInPtr-2];
  }
  if (GetPkt)
    switch (qv->QVState) {
      case QV_SendInit2:
	if (qv->PktIn[1]=='R') /* RINIT */
	  QVParseRINIT(qv);
	break;
      case QV_SendInit3:
	switch (qv->PktIn[1]) {
	  case 'P':
	    QVParseVRPOS(qv);
	    break;
	  case 'R': /* RINIT */
	    if (QVCountRetry(qv))
	      return TRUE;
	    QVResendPacket(qv); /* resend VFILE */
	    break;
	}
	break;
      case QV_SendData:
	switch (qv->PktIn[1]) {
	  case 'A':
	    QVParseVACK(qv);
	    break;
	  case 'N':
	    QVParseVNAK(qv);
	    break;
	  case 'T':
	    QVParseVSTAT(qv);
	    break;
	  case 'P': /* VRPOS */
	    if (qv->SeqNum==0)
	    {
	      fv->FTSetTimeOut(fv,0);
	      qv->SeqSent = 0;
	      qv->WinEnd = qv->WinSize;
	      if (qv->WinEnd > qv->FileEnd)
		qv->WinEnd = qv->FileEnd;
	      qv->EnqFlag = FALSE;
	      qv->RetryCount = 10;
	      qv->QVState = QV_SendDataRetry;
	    }
	    break;
	}
	break;
      case QV_SendDataRetry:
	switch (qv->PktIn[1]) {
	  case 'A':
	    QVParseVACK(qv);
	    break;
	  case 'N':
	    QVParseVNAK(qv);
	    break;
	  case 'T':
	    QVParseVSTAT(qv);
	    break;
	  case 'P': /* VRPOS */
	    if (qv->SeqNum==0)
	    {
	      fv->FTSetTimeOut(fv,0);
	      if (QVCountRetry(qv))
		return TRUE;
	      qv->SeqSent = 0;
	      qv->WinEnd = qv->WinSize;
	      if (qv->WinEnd > qv->FileEnd)
		qv->WinEnd = qv->FileEnd;
	      qv->EnqFlag = FALSE;
	    }
	    break;
	}
	break;
      case QV_SendNext:
	switch (qv->PktIn[1]) {
	  case 'P':
	    QVParseVRPOS(qv);
	    break;
	  case 'T':
	    if (QVCountRetry(qv))
	      return TRUE;
	    QVResendPacket(qv); /* resend VFILE */
	    break;
	}
	break;
    }

  /* create packet */
  if (qv->PktOutCount==0)
    QVSendVDATA(qv);

  /* send packet */
  c = 1;
  while ((c>0) && (qv->PktOutCount>0))
  {
	c = QVWrite(qv, &(qv->PktOut[qv->PktOutPtr]), qv->PktOutCount);
    qv->PktOutPtr = qv->PktOutPtr + c;
    qv->PktOutCount = qv->PktOutCount - c;
  }

  return TRUE;
}

static BOOL QVParse(TProto *pv)
{
	PQVVar qv = pv->PrivateData;
	switch (qv->QVMode) {
	case IdQVReceive:
		return QVReadPacket(qv);	// 処理が終わったら FALSE を返す
	case IdQVSend:
		return QVSendPacket(qv);	// 処理が終わったら FALSE を返す
	default:
		return FALSE;
	}
}

static int SetOptV(TProto *pv, int request, va_list ap)
{
	PQVVar qv = pv->PrivateData;
	switch(request) {
	case QUICKVAN_MODE: {
		QV_MODE_T Mode = va_arg(ap, QV_MODE_T);
		qv->QVMode = Mode;
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
	PQVVar qv = pv->PrivateData;
	if (qv->log != NULL) {
		TProtoLog* log = qv->log;
		log->Destory(log);
		qv->log = NULL;
	}
	free((void *)qv->FullName);
	qv->FullName = NULL;
	free(qv);
	pv->PrivateData = NULL;
	free(pv);
}

static const TProtoOp Op = {
	QVInit,
	QVParse,
	QVTimeOutProc,
	QVCancel,
	SetOpt,
	SetOptV,
	Destroy,
};

TProto *QVCreate(PFileVarProto fv)
{
	TProto *pv = malloc(sizeof(*pv));
	if (pv == NULL) {
		return NULL;
	}
	pv->Op = &Op;
	PQVVar qv = malloc(sizeof(*qv));
	pv->PrivateData = qv;
	if (qv == NULL) {
		free(pv);
		return NULL;
	}
	memset(qv, 0, sizeof(*qv));
	qv->FileOpen = FALSE;
	qv->Comm = fv->Comm;
	qv->file = fv->file_fv;
	qv->fv = fv;

	return pv;
}
