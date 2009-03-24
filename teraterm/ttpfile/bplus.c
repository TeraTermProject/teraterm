/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTFILE.DLL, B-Plus protocol */
#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include <string.h>
#include "tt_res.h"

#include "dlglib.h"
#include "ftlib.h"
#include "ttcommon.h"
#include "ttlib.h"

/* proto type */
BOOL FAR PASCAL GetTransFname
  (PFileVar fv, PCHAR CurDir, WORD FuncId, LPLONG Option);

#define BPTimeOut 10
#define BPTimeOutTCPIP 0

BOOL BPOpenFileToBeSent(PFileVar fv)
{
  if (fv->FileOpen) return TRUE;
  if (strlen(&(fv->FullName[fv->DirLen]))==0) return FALSE;

  fv->FileHandle = _lopen(fv->FullName,OF_READ);
  fv->FileOpen = fv->FileHandle>0;
  if (fv->FileOpen)
  {
    SetDlgItemText(fv->HWin, IDC_PROTOFNAME, &(fv->FullName[fv->DirLen]));
    fv->FileSize = GetFSize(fv->FullName);
  }
  return fv->FileOpen;
}

void BPDispMode(PFileVar fv, PBPVar bv)
{
  strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: B-Plus ", _TRUNCATE);
  switch (bv->BPMode) {
    case IdBPSend:
      strncat_s(fv->DlgCaption,sizeof(fv->DlgCaption),"Send",_TRUNCATE);
      break;
    case IdBPReceive:
      strncat_s(fv->DlgCaption,sizeof(fv->DlgCaption),"Receive",_TRUNCATE);
      break;
  }

  SetWindowText(fv->HWin,fv->DlgCaption);
}

void BPInit
  (PFileVar fv, PBPVar bv, PComVar cv, PTTSet ts)
{
  int i;

  if (bv->BPMode==IdBPAuto)
  {
    CommInsert1Byte(cv,'B');
    CommInsert1Byte(cv,DLE);
  }

  BPDispMode(fv,bv);
  SetDlgItemText(fv->HWin, IDC_PROTOPROT, "B-Plus");

  InitDlgProgress(fv->HWin, IDC_PROTOPROGRESS, &fv->ProgStat);

  /* file name, file size */
  if (bv->BPMode==IdBPSend)
    BPOpenFileToBeSent(fv);

  /* default parameters */
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
    switch (ts->Baud) {
      case IdBaud110:
	bv->TimeOut = BPTimeOut*2;
	bv->MaxBS = 1;
	break;
      case IdBaud300: bv->MaxBS = 1; break;
      case IdBaud600: bv->MaxBS = 2; break;
      case IdBaud1200: bv->MaxBS = 4; break;
      case IdBaud2400: bv->MaxBS = 8; break;
      case IdBaud4800: bv->MaxBS = 12; break;
      default:
	bv->MaxBS = 16;
	break;
    }
  }

  fv->LogFlag = ((ts->LogFlag & LOG_BP)!=0);
  if (fv->LogFlag)
    fv->LogFile = _lcreat("BPLUS.LOG",0);
  fv->LogState = 0;
  fv->LogCount = 0;

}

int BPRead1Byte(PFileVar fv, PBPVar bv, PComVar cv, LPBYTE b)
{
  if (CommRead1Byte(cv,b) == 0)
    return 0;

  if (fv->LogFlag)
  {
    if (fv->LogState==0)
    {
      fv->LogState = 1;
      fv->LogCount = 0;
      _lwrite(fv->LogFile,"\015\012<<<\015\012",7);
    }
    FTLog1Byte(fv,*b);
  }
  return 1;
}

int BPWrite(PFileVar fv, PBPVar bv, PComVar cv, PCHAR B, int C)
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

void BPTimeOutProc(PFileVar fv, PBPVar bv, PComVar cv)
{
  BPWrite(fv,bv,cv,"\005\005",2); /* two ENQ */
  FTSetTimeOut(fv,bv->TimeOut);
  bv->EnqSent = TRUE;
}

void BPUpdateCheck(PBPVar bv, BYTE b)
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

void BPSendACK(PFileVar fv, PBPVar bv, PComVar cv)
{
  char Temp[2];

  if ((bv->BPState != BP_Failure) &&
      (bv->BPState != BP_Close))
  {
    Temp[0] = 0x10; /* DLE */
    Temp[1] = (char)(bv->PktNum % 10 + 0x30);
    BPWrite(fv,bv,cv,Temp,2);
  }
  bv->BPPktState = BP_PktGetDLE;
}

void BPSendNAK(PFileVar fv, PBPVar bv, PComVar cv)
{
  if ((bv->BPState != BP_Failure) &&
      (bv->BPState != BP_Close))
    BPWrite(fv,bv,cv,"\025",1); /* NAK */
  bv->BPPktState = BP_PktGetDLE;
}

void BPPut1Byte(PBPVar bv, BYTE b, int *OutPtr)
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

void BPMakePacket(PBPVar bv, BYTE PktType, int DataLen)
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

void BPSendFailure(PBPVar bv, BYTE b)
{
  int i;

  i = 4;
  BPPut1Byte(bv,b,&i);
  i = i - 4;
  BPMakePacket(bv,'F',i);
  bv->BPState = BP_Failure;
}

void BPSendInit(PBPVar bv)
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

void BPSendTCPacket(PBPVar bv)
{
  int i;

  i = 4;
  BPPut1Byte(bv,'C',&i);
  i = i - 4;
  BPMakePacket(bv,'T',i);
  bv->BPState = BP_SendClose;
}

void BPSendNPacket(PFileVar fv, PBPVar bv)
{
  int i, c;
  BYTE b;

  i = 4;
  c = 1;
  while ((i-4 < bv->PktSize-1) && (c>0))
  {
    c = _lread(fv->FileHandle,&b,1);
    if (c==1)
      BPPut1Byte(bv,b,&i);
    fv->ByteCount = fv->ByteCount + c;
  }
  if (c==0)
  {
    _lclose(fv->FileHandle);
    fv->FileOpen = FALSE;
  }
  i = i - 4;
  BPMakePacket(bv,'N',i);

  SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
  if (fv->FileSize>0)
    SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
      fv->ByteCount, fv->FileSize, &fv->ProgStat);
}

void BPCheckPacket(PFileVar fv, PBPVar bv, PComVar cv)
{
  if (bv->Check != bv->CheckCalc)
  {
    BPSendNAK(fv,bv,cv);
    return;
  }

  /* Sequence number */
  if ((bv->PktNum+1) % 10 + 0x30 != bv->PktIn[0])
  {
    BPSendNAK(fv,bv,cv);
    return;
  }

  bv->PktNum++;
  if (bv->PktNum==10)
  {
    bv->PktNum = 0;
    bv->PktNumOffset = bv->PktNumOffset + 10;
  }
  SetDlgNum(fv->HWin, IDC_PROTOPKTNUM,
	    bv->PktNum + bv->PktNumOffset);

  if (bv->PktIn[1] != '+')
    BPSendACK(fv,bv,cv); /* Send ack */

  bv->GetPacket = TRUE;
}

  int BPGet1(PBPVar bv, int *i, LPBYTE b)
  {
    if (*i < bv->PktInCount)
    {
      *b = bv->PktIn[*i];
      (*i)++;
      return 1;
    }
    return 0;
  }


void BPParseTPacket(PFileVar fv, PBPVar bv)
{
  int i, j, c;
  BYTE b;
//  char Temp[HostNameMaxLength + 1]; // 81(yutaka)
  char Temp[81]; // 81(yutaka)

  switch (bv->PktIn[2]) {
    case 'C': /* Close */
      if (fv->FileOpen)
      {
	_lclose(fv->FileHandle);
	fv->FileOpen = FALSE;
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
      BPDispMode(fv,bv);

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

      GetFileNamePos(Temp,&i,&j);
	  strncpy_s(&(fv->FullName[fv->DirLen]),sizeof(fv->FullName) - fv->DirLen,&(Temp[j]),_TRUNCATE);
      /* file open */
      if (! FTCreateFile(fv))
      {
	BPSendFailure(bv,'E');
	return;
      }
      break;
    case 'I': /* File information */
      i = 5;
      /* file size */
      fv->FileSize = 0;
      do {
	c = BPGet1(bv,&i,&b);
	if ((c==1) &&
	    (b>=0x30) && (b<=0x39))
	  fv->FileSize =
	    fv->FileSize * 10 + b - 0x30;
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
      BPDispMode(fv,bv);

      if (! fv->FileOpen)
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

	GetFileNamePos(Temp,&i,&j);
	FitFileName(&(Temp[j]),sizeof(Temp) - j,NULL);
	strncpy_s(&(fv->FullName[fv->DirLen]),sizeof(fv->FullName) - fv->DirLen,
		&(Temp[j]),_TRUNCATE);

	/* file open */
	if (! BPOpenFileToBeSent(fv))
	{
	  /* if file not found, ask user new file name */
	  fv->FullName[fv->DirLen] = 0;
	  if (! GetTransFname(fv, NULL, GTF_BP, (PLONG)&i)) 
	  {
	    BPSendFailure(bv,'E');
	    return;
	  }
	  /* open retry */
	  if (! BPOpenFileToBeSent(fv))
	  {
	    BPSendFailure(bv,'E');
	    return;
	  }
	}
      }
      fv->ByteCount = 0;

      bv->BPState = BP_SendData;
      BPSendNPacket(fv,bv);
      break;
    default: break;
  }
}

void BPParsePacket(PFileVar fv, PBPVar bv)
{
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
      if (! fv->NoMsg)
	MessageBox(fv->HMainWin,"Transfer failure",
	  "Tera Term: Error",MB_ICONEXCLAMATION);
      bv->BPState = BP_Close;
      break;
    case 'N': /* Data */
      if ((bv->BPState==BP_RecvFile) &&
	  fv->FileOpen)
	bv->BPState = BP_RecvData;
      else if (bv->BPState != BP_RecvData)
      {
	BPSendFailure(bv,'E');
	return;
      }
      _lwrite(fv->FileHandle,&(bv->PktIn[2]),bv->PktInCount-2);
      fv->ByteCount = fv->ByteCount +
		      bv->PktInCount - 2;
      SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
      if (fv->FileSize>0)
	SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
	  fv->ByteCount, fv->FileSize, &fv->ProgStat);
      break;
    case 'T':
      BPParseTPacket(fv,bv); /* File transfer */
      break;
  }
}

void BPParseAck(PFileVar fv, PBPVar bv, BYTE b)
{
  b = (b - 0x30) % 10;
  if (bv->EnqSent)
  {
    FTSetTimeOut(fv,0);
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

  FTSetTimeOut(fv,0);
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
	if (fv->FileOpen)
	  BPSendNPacket(fv,bv);
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
  SetDlgNum(fv->HWin, IDC_PROTOPKTNUM, bv->PktNum + bv->PktNumOffset);
}

  void BPDequote(LPBYTE b)
  {
    if ((*b>=0x40) && (*b<=0x5f))
      *b = *b - 0x40;
    else if ((*b>=0x60) && (*b<=0x7f))
      *b = *b + 0x20;
  }

BOOL BPParse(PFileVar fv, PBPVar bv, PComVar cv)
{
  int c;
  BYTE b;

  do {

    /* Send packet */
    if (bv->BPPktState==BP_PktSending)
    {
      c = 1;
      while ((c>0) && (bv->PktOutCount>0))
      {
	c = BPWrite(fv,bv,cv,&(bv->PktOut[bv->PktOutPtr]),bv->PktOutCount);
	bv->PktOutPtr = bv->PktOutPtr + c;
	bv->PktOutCount = bv->PktOutCount - c;
      }
      if (bv->PktOutCount>0) return TRUE;
      if (cv->OutBuffCount==0)
      {
	bv->BPPktState = BP_PktGetDLE;
	FTSetTimeOut(fv,bv->TimeOut);
      }
      else
	return TRUE;
    }

    /* Get packet */
    c = BPRead1Byte(fv,bv,cv,&b);
    while ((c>0) && (bv->BPPktState!=BP_PktSending) &&
	   ! bv->GetPacket)
    {
      switch (bv->BPPktState) {
	case BP_PktGetDLE:
	  switch (b) {
	    case 0x03: /* ETX */
	      BPSendNAK(fv,bv,cv);
	      break;
	    case 0x05: /* ENQ */
	      if (bv->BPState==BP_Init)
		BPWrite(fv,bv,cv,"\020++\0200",5);
	      else
		BPSendACK(fv,bv,cv);
	      break;
	    case 0x10: /* DLE */
	      bv->BPPktState = BP_PktDLESeen;
	      break;
	    case 0x15: /* NAK */
	      BPWrite(fv,bv,cv,"\005\005",2); /* two ENQs */
	      FTSetTimeOut(fv,bv->TimeOut);
	      bv->EnqSent = TRUE;
	      break;
	  }
	  break;
	case BP_PktDLESeen:
	  switch (b) {
	    case 0x05: /* ENQ */
	      BPSendACK(fv,bv,cv);
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
		BPParseAck(fv,bv,b);
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
	      BPSendACK(fv,bv,cv);
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
		BPCheckPacket(fv,bv,cv);
	      }
	  }
	  break;
      }
      if ((bv->BPPktState != BP_PktSending) &&
	  ! bv->GetPacket)
	c = BPRead1Byte(fv,bv,cv,&b);
      else
	c = 0;
    }

    /* Parse packet */
    if (bv->GetPacket)
      BPParsePacket(fv,bv);

  } while ((c!=0) || (bv->BPPktState==BP_PktSending));

  if (bv->BPState==BP_Close)
    return FALSE;

  return TRUE;
}

void BPCancel(PBPVar bv)
{
  if ((bv->BPState != BP_Failure) &&
      (bv->BPState != BP_Close))
    BPSendFailure(bv,'A');
}
