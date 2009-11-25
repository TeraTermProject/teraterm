/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTFILE.DLL, Quick-VAN protocol */
#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys\utime.h>
#include <sys\stat.h>

#include "tt_res.h"
#include "dlglib.h"
#include "ftlib.h"
#include "ttlib.h"
#include "ttcommon.h"

#define TimeOutCAN 1
#define TimeOutCANSend 2
#define TimeOutRecv 20
#define TimeOutSend 60
#define TimeOutEOT 5

#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

int QVRead1Byte(PFileVar fv, PQVVar qv, PComVar cv, LPBYTE b)
{
  if (CommRead1Byte(cv,b) == 0)
    return 0;

  if (fv->LogFlag)
  {
    if (fv->LogState!=1)
    {
      fv->LogState = 1;
      fv->LogCount = 0;
      _lwrite(fv->LogFile,"\015\012<<<\015\012",7);
    }
    FTLog1Byte(fv,*b);
  }
  return 1;
}

int QVWrite(PFileVar fv, PQVVar qv, PComVar cv, PCHAR B, int C)
{
  int i, j;

  i = CommBinaryOut(cv,B,C);

  if (fv->LogFlag && (i>0))
  {
    if (fv->LogState != 0)
    {
      fv->LogState = 0;
      fv->LogCount = 0;
      _lwrite(fv->LogFile,"\015\012>>>\015\012",7);
    }
    for (j=0 ; j <= i-1 ; j++)
      FTLog1Byte(fv,B[j]);
  }
  return i;
}

void QVSendNAK(PFileVar fv, PQVVar qv, PComVar cv)
{
  BYTE b;

  b = NAK;
  QVWrite(fv,qv,cv,&b, 1);
  FTSetTimeOut(fv,TimeOutRecv);
}

void QVSendACK(PFileVar fv, PQVVar qv, PComVar cv)
{
  BYTE b;

  FTSetTimeOut(fv,0);
  b = ACK;
  QVWrite(fv,qv,cv,&b, 1);
  qv->QVState = QV_Close;
}

void QVInit
  (PFileVar fv, PQVVar qv, PComVar cv, PTTSet ts)
{
  char uimsg[MAX_UIMSG];

  qv->WinSize = ts->QVWinSize;
  fv->LogFlag = ((ts->LogFlag & LOG_QV)!=0);

  if (fv->LogFlag)
    fv->LogFile = _lcreat("QUICKVAN.LOG",0);
  fv->LogState = 2;
  fv->LogCount = 0;

  fv->FileOpen = FALSE;
  fv->ByteCount = 0;

  if (qv->QVMode==IdQVReceive)
  {
    strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), "Tera Term: ", _TRUNCATE);
    get_lang_msg("FILEDLG_TRANS_TITLE_QVRCV", uimsg, sizeof(uimsg), TitQVRcv, UILanguageFile);
    strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
  }
  SetWindowText(fv->HWin, fv->DlgCaption);
  SetDlgItemText(fv->HWin, IDC_PROTOPROT, "Quick-VAN");

  InitDlgProgress(fv->HWin, IDC_PROTOPROGRESS, &fv->ProgStat);

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
      FTSetTimeOut(fv,TimeOutSend);
      break;
    case IdQVReceive:
      qv->QVState = QV_RecvInit1;
      qv->PktState = QVpktSOH;
      qv->RetryCount = 10;
      QVSendNAK(fv,qv,cv);
      break;
  }
}

void QVCancel(PFileVar fv, PQVVar qv, PComVar cv)
{
  BYTE b;

  if ((qv->QVState==QV_Close) ||
      (qv->QVState==QV_RecvEOT) ||
      (qv->QVState==QV_Cancel))
    return;

  /* flush send buffer */
  qv->PktOutCount = 0;
  /* send CAN */
  b = CAN;
  QVWrite(fv,qv,cv,&b, 1);

  if ((qv->QVMode==IdQVReceive) &&
      ((qv->QVState==QV_RecvData) ||
       (qv->QVState==QV_RecvDataRetry)))
  {
    FTSetTimeOut(fv,TimeOutEOT);
    qv->QVState = QV_RecvEOT;
    return;
  }
  FTSetTimeOut(fv,TimeOutCANSend);
  qv->QVState = QV_Cancel;
}

BOOL QVCountRetry(PFileVar fv, PQVVar qv, PComVar cv)
{
  qv->RetryCount--;
  if (qv->RetryCount<=0)
  {
    QVCancel(fv,qv,cv);
    return TRUE;
  }
  else
    return FALSE;
}

void QVResendPacket(PFileVar fv, PQVVar qv)
{

  qv->PktOutCount = qv->PktOutLen;
  qv->PktOutPtr = 0;
  if (qv->QVMode==IdQVReceive)
    FTSetTimeOut(fv,TimeOutRecv);
  else
    FTSetTimeOut(fv,TimeOutSend);
}

void QVSetResPacket(PQVVar qv, BYTE Typ, BYTE Num, int DataLen)
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

void QVSendVACK(PFileVar fv, PQVVar qv, BYTE Seq)
{
  FTSetTimeOut(fv,TimeOutRecv);
  qv->RetryCount = 10;
  qv->QVState = QV_RecvData;
  if (((int)qv->SeqNum % (int)qv->AValue) == 0)
    QVSetResPacket(qv,'A',Seq,0);
}

void QVSendVNAK(PFileVar fv, PQVVar qv)
{
  FTSetTimeOut(fv,TimeOutRecv);
  QVSetResPacket(qv,'N',LOBYTE(qv->SeqNum+1),0);
  if (qv->QVState==QV_RecvData)
  {
    qv->RetryCount = 10;
    qv->QVState = QV_RecvDataRetry;
  }
}

void QVSendVSTAT(PFileVar fv, PQVVar qv)
{
  FTSetTimeOut(fv,TimeOutRecv);
  qv->PktOut[3] = 0x30;
  QVSetResPacket(qv,'T',LOBYTE(qv->SeqNum),1);
  qv->RetryCount = 10;
  qv->QVState = QV_RecvNext;
}

void QVTimeOutProc(PFileVar fv, PQVVar qv, PComVar cv)
{
  if ((qv->QVState==QV_Cancel) ||
      (qv->QVState==QV_RecvEOT))
  {
    qv->QVState = QV_Close;
    return;
  }

  if (qv->QVMode==IdQVSend)
  {
    QVCancel(fv,qv,cv);
    return;
  }

  if (qv->CanFlag)
  {
    qv->CanFlag = FALSE;
    qv->QVState = QV_Close;
    return;
  }

  if ((qv->QVState != QV_RecvData) &&
      QVCountRetry(fv,qv,cv)) return;

  qv->PktState = QVpktSOH;
  switch (qv->QVState) {
    case QV_RecvInit1:
      QVSendNAK(fv,qv,cv);
      break;
    case QV_RecvInit2:
      QVResendPacket(fv,qv); /* resend RINIT */
      break;
    case QV_RecvData:
    case QV_RecvDataRetry:
      if (qv->SeqNum==0)
	QVResendPacket(fv,qv); /* resend RPOS */
      else
	QVSendVNAK(fv,qv);
      break;
    case QV_RecvNext:
      QVResendPacket(fv,qv); /* resend VSTAT */
      break;
  }
}

BOOL QVParseSINIT(PFileVar fv, PQVVar qv)
{
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
  FTSetTimeOut(fv,TimeOutRecv);

  return TRUE;
}

  BOOL QVGetNum2(PQVVar qv, int *i, LPWORD w)
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

BOOL QVParseVFILE(PFileVar fv, PQVVar qv)
{
  int i, j;
  WORD w;
  BYTE b;

  if ((qv->QVState != QV_RecvInit2) &&
      (qv->QVState != QV_RecvNext))
    return TRUE;

  /* file name */
  GetFileNamePos(&(qv->PktIn[5]),&i,&j);
  strncpy_s(&(fv->FullName[fv->DirLen]),sizeof(fv->FullName) - fv->DirLen,&(qv->PktIn[5+j]),_TRUNCATE);
  /* file open */
  if (! FTCreateFile(fv)) return FALSE;
  /* file size */
  i = strlen(&(qv->PktIn[5])) + 6;
  do {
    b = qv->PktIn[i];
    if ((b>=0x30) && (b<=0x39))
      fv->FileSize =
	fv->FileSize * 10 + b - 0x30;
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

  SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, 0);
  if (fv->FileSize>0)
    SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
      0, fv->FileSize, &fv->ProgStat);

  /* Send VRPOS */
  QVSetResPacket(qv,'P',0,0);
  qv->QVState = QV_RecvData;
  qv->SeqNum = 0;
  qv->RetryCount = 10;
  FTSetTimeOut(fv,TimeOutRecv);

  return TRUE;
}

BOOL QVParseVENQ(PFileVar fv, PQVVar qv)
{
  struct tm time;
  struct utimbuf timebuf;

  if ((qv->QVState != QV_RecvData) &&
      (qv->QVState != QV_RecvDataRetry))
    return TRUE;

  if (qv->PktIn[3]==LOBYTE(qv->SeqNum))
  {
    if (qv->PktIn[4]==0x30)
    {
      if (fv->FileOpen)
      {
	_lclose(fv->FileHandle);
	fv->FileOpen = FALSE;
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
	  utime(fv->FullName,&timebuf);
	}
      }
      QVSendVSTAT(fv,qv);
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
    QVSendVNAK(fv,qv);
  }

  return TRUE;
}

void QVWriteToFile(PFileVar fv, PQVVar qv)
{
  int C;

  if (fv->FileSize - fv->ByteCount < 128)
    C = fv->FileSize - fv->ByteCount;
  else
    C = 128;
  _lwrite(fv->FileHandle,&(qv->PktIn[3]),C);
  fv->ByteCount = fv->ByteCount + C;

  SetDlgNum(fv->HWin, IDC_PROTOPKTNUM, qv->SeqNum);
  SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
  if (fv->FileSize>0)
    SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
      fv->ByteCount, fv->FileSize, &fv->ProgStat);
}

BOOL QVCheckWindow8(PQVVar qv, WORD w0, WORD w1, BYTE b, LPWORD  w)
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

BOOL QVReadPacket(PFileVar fv, PQVVar qv, PComVar cv)
{
  BYTE b;
  WORD w0, w1, w;
  int c;
  BOOL GetPkt, EOTFlag, Ok;

  if (qv->QVState == QV_Close)
    return FALSE;

  c = 1;
  while ((c>0) && (qv->PktOutCount>0))
  {
    c = QVWrite(fv,qv,cv,&(qv->PktOut[qv->PktOutPtr]),qv->PktOutCount);
    qv->PktOutPtr = qv->PktOutPtr + c;
    qv->PktOutCount = qv->PktOutCount - c;
  }

  c = QVRead1Byte(fv,qv,cv,&b);
  if ((c>0) && qv->CanFlag)
  {
    qv->CanFlag = FALSE;
    FTSetTimeOut(fv,TimeOutRecv);
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
	  if ((qv->SeqNum==0) && (qv->PktIn[1]==1) ||
	      (qv->SeqNum>0) &&
	      QVCheckWindow8(qv,w0,w1,qv->PktIn[1],&w))
	  {
	    qv->CheckSum = 0;
	    qv->PktInPtr = 3;
	    qv->PktInCount = 129;
	    qv->PktState = QVpktDATA;
	  }
	  else {
	    qv->PktState =QVpktSOH;
	    QVSendVNAK(fv,qv);
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
	    FTSetTimeOut(fv,TimeOutRecv);
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

    if (! GetPkt) c = QVRead1Byte(fv,qv,cv,&b);
  }

  if (! GetPkt)
  {
    if (qv->CanFlag)
      FTSetTimeOut(fv,TimeOutCAN);

    if (EOTFlag)
      switch (qv->QVState) {
	case QV_RecvInit2:
	case QV_RecvNext:
	  QVSendACK(fv,qv,cv);
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
	      ! QVCountRetry(fv,qv,cv))
	    QVSendNAK(fv,qv,cv);
	  break;
	case QV_RecvInit2:
	case QV_RecvNext:
	  if ((qv->PktIn[2]==0x31) && /* VFILE */
	      ! QVCountRetry(fv,qv,cv))
	    QVResendPacket(fv,qv);
	  break;
	case QV_RecvData:
	  if (qv->PktIn[2]==0x32) /* VENQ */
	    QVSendVNAK(fv,qv);
	  break;
	case QV_RecvDataRetry:
	  if ((qv->PktIn[2]==0x32) && /* VENQ */
	      ! QVCountRetry(fv,qv,cv))
	    QVSendVNAK(fv,qv);
	  break;
      }
      return TRUE;
    }
    Ok = FALSE;
    switch (qv->PktIn[2]) { /* function type */
      case 0x30:
	Ok = QVParseSINIT(fv,qv);
	break;
      case 0x31:
	Ok = QVParseVFILE(fv,qv);
	break;
      case 0x32:
	Ok = QVParseVENQ(fv,qv);
	break;
    }
    if (! Ok)
      QVCancel(fv,qv,cv);
  }
  else { /* VDAT block */
    if ((qv->QVState != QV_RecvData) &&
	(qv->QVState != QV_RecvDataRetry))
      return TRUE;
    if (qv->PktIn[1] != LOBYTE(qv->SeqNum+1))
      QVSendVACK(fv,qv,qv->PktIn[1]);
    else if (qv->CheckSum==qv->PktIn[qv->PktInPtr-1])
    {
      QVSendVACK(fv,qv,qv->PktIn[1]);
      qv->SeqNum++;
      QVWriteToFile(fv,qv);
    }
    else /* bad checksum */
      QVSendVNAK(fv,qv);
  }

  return TRUE;
}

void QVSetPacket(PQVVar qv, BYTE Num, BYTE Typ)
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

void QVSendSINIT(PFileVar fv, PQVVar qv)
{
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
  FTSetTimeOut(fv,TimeOutSend);
}

void QVSendEOT(PFileVar fv, PQVVar qv, PComVar cv)
{
  BYTE b;

  if (qv->QVState==QV_SendEnd)
  {
    if (QVCountRetry(fv,qv,cv))
      return;
  }
  else {
    qv->RetryCount = 10;
    qv->QVState = QV_SendEnd;
  }

  b = EOT;
  QVWrite(fv,qv,cv,&b, 1);
  FTSetTimeOut(fv,TimeOutSend);
}

  void QVPutNum2(PQVVar qv, WORD Num, int *i)
  {
    qv->PktOut[*i] = Num / 10 + 0x30;
    (*i)++;
    qv->PktOut[*i] = Num % 10 + 0x30;
    (*i)++;
  }

void QVSendVFILE(PFileVar fv, PQVVar qv, PComVar cv)
{
  int i, j;
  struct stat stbuf; 
  struct tm * tmbuf;

  if (! GetNextFname(fv))
  {
    QVSendEOT(fv,qv,cv);
    return;
  }

  /* find file and get file info */
  fv->FileSize = GetFSize(fv->FullName);
  if (fv->FileSize>0)
  {
    qv->FileEnd = (WORD)(fv->FileSize >> 7);
    if ((fv->FileSize & 0x7F) != 0)
      qv->FileEnd++;
  }
  else {
    QVCancel(fv,qv,cv);
    return;
  }

  /* file open */
  fv->FileHandle = _lopen(fv->FullName,OF_READ);
  fv->FileOpen = fv->FileHandle>0;
  if (! fv->FileOpen)
  {
    QVCancel(fv,qv,cv);
    return;
  }
  /* file no. */
  qv->FileNum++;
  fv->ProgStat = 0;
  i = 3;
  QVPutNum2(qv,qv->FileNum,&i);
  /* file name */
  SetDlgItemText(fv->HWin, IDC_PROTOFNAME, &(fv->FullName[fv->DirLen]));
  strncpy_s(&(qv->PktOut[i]),sizeof(qv->PktOut)-i,_strupr(&(fv->FullName[fv->DirLen])),_TRUNCATE);
  FTConvFName(&(qv->PktOut[i]));  // replace ' ' by '_' in FName
  i = strlen(&(qv->PktOut[i])) + i;
  qv->PktOut[i] = 0;
  i++;
  /* file size */
  _snprintf_s(&(qv->PktOut[i]),sizeof(qv->PktOut)-i,_TRUNCATE,"%u",fv->FileSize);
  i = strlen(&(qv->PktOut[i])) + i;
  qv->PktOut[i] = 0;
  i++;
  /* date */
  stat(fv->FullName,&stbuf);
  tmbuf = localtime(&stbuf.st_mtime);

  QVPutNum2(qv,(WORD)((tmbuf->tm_year+1900) / 100),&i);
  QVPutNum2(qv,(WORD)((tmbuf->tm_year+1900) % 100),&i);
  QVPutNum2(qv,(WORD)(tmbuf->tm_mon+1),&i);
  QVPutNum2(qv,(WORD)tmbuf->tm_mday,&i);
  /* time */
  QVPutNum2(qv,(WORD)tmbuf->tm_hour,&i);
  QVPutNum2(qv,(WORD)tmbuf->tm_min,&i);
  QVPutNum2(qv,(WORD)tmbuf->tm_sec,&i);
  for (j = i ; j <= 130 ; j++)
    qv->PktOut[j] = 0;

  /* send VFILE */
  QVSetPacket(qv,0,0x31);
  if (qv->FileNum==1)
    qv->QVState = QV_SendInit3;
  else
    qv->QVState = QV_SendNext;
  qv->PktState = QVpktSTX;
  FTSetTimeOut(fv,TimeOutSend);
}

void QVSendVDATA(PFileVar fv, PQVVar qv)
{
  int i, C;
  LONG Pos;

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
      C = fv->FileSize - Pos;
    else
      C = 128;
    /* read data from file */
    _llseek(fv->FileHandle,Pos,0);
    _lread(fv->FileHandle,&(qv->PktOut[3]),C);
    fv->ByteCount = Pos + (LONG)C;
    SetDlgNum(fv->HWin, IDC_PROTOPKTNUM, qv->SeqSent);
    SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
    SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
		  fv->ByteCount, fv->FileSize, &fv->ProgStat);
    for (i = C ; i <= 127 ; i++)
      qv->PktOut[3+i] = 0;
    /* send VDAT */
    QVSetPacket(qv,LOBYTE(qv->SeqSent),LOBYTE(~ qv->SeqSent));
    if (qv->SeqSent==qv->WinEnd) /* window close */
      FTSetTimeOut(fv,TimeOutSend);
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
    FTSetTimeOut(fv,TimeOutSend);
    qv->EnqFlag = TRUE;
  }

}

void QVParseRINIT(PFileVar fv, PQVVar qv, PComVar cv)
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
    QVCancel(fv,qv,cv);
    return;
  }

  /* Send VFILE */
  qv->RetryCount = 10;
  QVSendVFILE(fv,qv,cv);
}

void QVParseVRPOS(PFileVar fv, PQVVar qv, PComVar cv)
{
  int i;
  BYTE b;

  qv->SeqNum = 0;
  if (qv->PktInPtr-3 >= 3)
    for (i = 3 ; i <= qv->PktInPtr-3 ; i++)
    {
      b = qv->PktIn[i];
      if ((b<0x30) || (b>0x39))
      {
	QVCancel(fv,qv,cv);
	return;
      }
      qv->SeqNum = qv->SeqNum * 10 + (WORD)(b - 0x30);
    }

  if (qv->SeqNum >= qv->FileEnd)
  {
    QVCancel(fv,qv,cv);
    return;
  }

  qv->SeqSent = qv->SeqNum;
  qv->WinEnd = qv->SeqNum + qv->WinSize;
  if (qv->WinEnd > qv->FileEnd)
    qv->WinEnd = qv->FileEnd;
  qv->EnqFlag = FALSE;
  qv->QVState = QV_SendData;
  FTSetTimeOut(fv,0);
}

BOOL QVCheckWindow7(PQVVar qv, WORD w0, WORD w1, BYTE b, LPWORD w)
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

void QVParseVACK(PFileVar fv, PQVVar qv)
{
  WORD w;

  if (QVCheckWindow7(qv,(WORD)(qv->SeqNum+1),qv->SeqSent,qv->PktIn[2],&w))
  {
    FTSetTimeOut(fv,0);
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

void QVParseVNAK(PFileVar fv, PQVVar qv, PComVar cv)
{
  WORD w;

  if ((qv->QVState==QV_SendDataRetry) &&
      (qv->PktIn[1]==LOBYTE(qv->SeqNum+1)))
  {
    FTSetTimeOut(fv,0);
    if (QVCountRetry(fv,qv,cv)) return;
    qv->SeqSent = qv->SeqNum;
    qv->WinEnd = qv->SeqNum + qv->WinSize;
    if (qv->WinEnd > qv->FileEnd)
      qv->WinEnd = qv->FileEnd;
    qv->EnqFlag = FALSE;
    return;
  }

  if (QVCheckWindow7(qv,(WORD)(qv->SeqNum+1),(WORD)(qv->SeqSent+1),qv->PktIn[2],&w))
  {
    FTSetTimeOut(fv,0);
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

void QVParseVSTAT(PFileVar fv, PQVVar qv, PComVar cv)
{

  if (qv->EnqFlag && (qv->PktIn[3]==0x30))
  {
    if (fv->FileOpen)
      _lclose(fv->FileHandle);
    fv->FileOpen = FALSE;
    qv->EnqFlag = FALSE;
    qv->RetryCount = 10;
    QVSendVFILE(fv,qv,cv);
  }
  else
    QVCancel(fv,qv,cv);
}

BOOL QVSendPacket(PFileVar fv, PQVVar qv, PComVar cv)
{
  BYTE b;
  int c, i;
  BOOL GetPkt;

  if (qv->QVState == QV_Close)
    return FALSE;

  c = QVRead1Byte(fv,qv,cv, &b);
  if ((c==0) && qv->CanFlag)
  {
    if ((qv->QVState==QV_SendData) ||
	(qv->QVState==QV_SendDataRetry))
    {
      b = EOT;
      QVWrite(fv,qv,cv,&b, 1);
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
	  QVSendSINIT(fv,qv);
	  break;
	case QV_SendInit2:
	  if (QVCountRetry(fv,qv,cv))
	    return TRUE;
	  QVSendSINIT(fv,qv);
	  break;
      }

    if (qv->QVState==QV_SendEnd)
    {
      if (b==ACK)
      {
	fv->Success = TRUE;
	return FALSE;
      }
      QVSendEOT(fv,qv,cv);
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
    if (! GetPkt) c = QVRead1Byte(fv,qv,cv, &b);
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
	  QVParseRINIT(fv,qv,cv);
	break;
      case QV_SendInit3:
	switch (qv->PktIn[1]) {
	  case 'P':
	    QVParseVRPOS(fv,qv,cv);
	    break;
	  case 'R': /* RINIT */
	    if (QVCountRetry(fv,qv,cv))
	      return TRUE;
	    QVResendPacket(fv,qv); /* resend VFILE */
	    break;
	}
	break;
      case QV_SendData:
	switch (qv->PktIn[1]) {
	  case 'A':
	    QVParseVACK(fv,qv);
	    break;
	  case 'N':
	    QVParseVNAK(fv,qv,cv);
	    break;
	  case 'T':
	    QVParseVSTAT(fv,qv,cv);
	    break;
	  case 'P': /* VRPOS */
	    if (qv->SeqNum==0)
	    {
	      FTSetTimeOut(fv,0);
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
	    QVParseVACK(fv,qv);
	    break;
	  case 'N':
	    QVParseVNAK(fv,qv,cv);
	    break;
	  case 'T':
	    QVParseVSTAT(fv,qv,cv);
	    break;
	  case 'P': /* VRPOS */
	    if (qv->SeqNum==0)
	    {
	      FTSetTimeOut(fv,0);
	      if (QVCountRetry(fv,qv,cv))
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
	    QVParseVRPOS(fv,qv,cv);
	    break;
	  case 'T':
	    if (QVCountRetry(fv,qv,cv))
	      return TRUE;
	    QVResendPacket(fv,qv); /* resend VFILE */
	    break;
	}
	break;
    }

  /* create packet */
  if (qv->PktOutCount==0)
    QVSendVDATA(fv,qv);

  /* send packet */
  c = 1;
  while ((c>0) && (qv->PktOutCount>0))
  {
    c = QVWrite(fv,qv,cv,&(qv->PktOut[qv->PktOutPtr]),qv->PktOutCount);
    qv->PktOutPtr = qv->PktOutPtr + c;
    qv->PktOutCount = qv->PktOutCount - c;
  }

  return TRUE;
}
