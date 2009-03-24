/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTFILE.DLL, Kermit protocol */
#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include <string.h>

#include "tt_res.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "dlglib.h"
#include "ftlib.h"

  /* kermit parameters */
#define MaxNum 94

#define MinMAXL 10
#define MaxMAXL 94 
#define DefMAXL 90
#define MinTIME 1
#define DefTIME 10
#define DefNPAD 0
#define DefPADC 0
#define DefEOL  0x0D
#define DefQCTL '#'
#define MyQBIN  'Y'
#define DefCHKT 1
#define MyREPT  '~'

BYTE KmtNum(BYTE b)
{
  return (b - 32);
}

BYTE KmtChar(BYTE b)
{
  return (b+32);
}

void KmtCalcCheck(WORD Sum, BYTE CHKT, PCHAR Check)
{
  switch (CHKT) {
    case 1:
      Check[0] = KmtChar((BYTE)((Sum + (Sum & 0xC0) / 0x40) & 0x3F));
      break;
    case 2:
      Check[0] = KmtChar((BYTE)((Sum / 0x40) & 0x3F));
      Check[1] = KmtChar((BYTE)(Sum & 0x3F));
      break;
    case 3: break;
  }
}

void KmtSendPacket(PFileVar fv, PKmtVar kv, PComVar cv)
{
  int C;

  /* padding characters */
  for (C = 1 ; C <= kv->KmtYour.NPAD ; C++)
    CommBinaryOut(cv,&(kv->KmtYour.PADC), 1);

  /* packet */
  C = KmtNum(kv->PktOut[1]) + 2;
  CommBinaryOut(cv,&kv->PktOut[0], C);

  if (fv->LogFlag)
  {
    _lwrite(fv->LogFile,"> ",2);
    _lwrite(fv->LogFile,&(kv->PktOut[1]),C-1);
    _lwrite(fv->LogFile,"\015\012",2);
  }

  /* end-of-line character */
  if (kv->KmtYour.EOL > 0)
    CommBinaryOut(cv,&(kv->KmtYour.EOL), 1);

  FTSetTimeOut(fv,kv->KmtYour.TIME);
}

void KmtMakePacket(PFileVar fv, PKmtVar kv, BYTE SeqNum, BYTE PktType, int DataLen)
{
  int i;
  WORD Sum;

  kv->PktOut[0] = 1; /* MARK */
  kv->PktOut[1] = KmtChar((BYTE)(DataLen + kv->KmtMy.CHKT + 2)); /* LEN */
  kv->PktOut[2] = KmtChar(SeqNum); /* SEQ */
  kv->PktOut[3] = PktType; /* TYPE */

  /* check sum */
  Sum = 0;
  for (i = 1 ; i <= DataLen+3 ; i++)
    Sum = Sum + kv->PktOut[i];
  KmtCalcCheck(Sum, kv->KmtMy.CHKT, &(kv->PktOut[DataLen+4]));
}


void KmtSendInitPkt(PFileVar fv, PKmtVar kv, PComVar cv, BYTE PktType)
{
  int NParam;

  kv->PktNumOffset = 0;
  kv->PktNum = 0;

  NParam = 9; /* num of parameters in Send-init packet */

  /* parameters */

  kv->PktOut[4] = KmtChar(kv->KmtMy.MAXL);
  kv->PktOut[5] = KmtChar(kv->KmtMy.TIME);
  kv->PktOut[6] = KmtChar(kv->KmtMy.NPAD);
  kv->PktOut[7] = kv->KmtMy.PADC ^ 0x40;
  kv->PktOut[8] = KmtChar(kv->KmtMy.EOL);
  kv->PktOut[9] = kv->KmtMy.QCTL;
  kv->PktOut[10] = kv->KmtMy.QBIN;
  kv->PktOut[11] = kv->KmtMy.CHKT + 0x30;
  kv->PktOut[12] = kv->KmtMy.REPT;

  KmtMakePacket(fv,kv,(BYTE)(kv->PktNum - kv->PktNumOffset),PktType,NParam);
  KmtSendPacket(fv,kv,cv);

  switch (PktType) {
    case 'S': kv->KmtState = SendInit; break;
    case 'I': kv->KmtState = ServerInit; break;
  }
}

void KmtSendNack(PFileVar fv, PKmtVar kv, PComVar cv, BYTE SeqChar)
{
  KmtMakePacket(fv,kv,KmtNum(SeqChar),'N',0);
  KmtSendPacket(fv,kv,cv);
}

int KmtCalcPktNum(PKmtVar kv, BYTE b)
{
  int n;

  n = KmtNum(b) + kv->PktNumOffset;
  if (n>kv->PktNum+31) n = n - 64;
  else if (n<kv->PktNum-32) n = n + 64;
  return n;
}

BOOL KmtCheckPacket(PKmtVar kv)
{
  int i;
  WORD Sum;
  BYTE Check[3];

  /* Calc sum */
  Sum = 0;
  for (i = 1 ; i <= kv->PktInLen+1-kv->KmtMy.CHKT ; i++)
    Sum = Sum + kv->PktIn[i];

  /* Calc CHECK */
  KmtCalcCheck(Sum, kv->KmtMy.CHKT, &Check[0]);

  for (i = 1 ; i <= kv->KmtMy.CHKT ; i++)
    if (Check[i-1] !=
        kv->PktIn[ kv->PktInLen +1- kv->KmtMy.CHKT +i ])
      return FALSE;
  return TRUE;
}

  BOOL KmtCheckQuote(BYTE b)
  {
    return (((b>0x20) && (b<0x3f)) ||
            ((b>0x5F) && (b<0x7f)));
  }

void KmtParseInit(PKmtVar kv, BOOL AckFlag)
{
  int i, NParam;
  BYTE b, n;

  NParam = kv->PktInLen - 2 - kv->KmtMy.CHKT;

  for (i=1 ; i <= NParam ; i++)
  {
    b = kv->PktIn[i+3];
    n = KmtNum(b);
    switch (i) {
      case 1:
        if ((MinMAXL<n) && (n<MaxMAXL))
           kv->KmtYour.MAXL = n;
	break;
      case 2:
        if ((MinTIME<n) && (n<MaxNum))
           kv->KmtYour.TIME = n;
        break;
      case 3:
        if (n<MaxNum)
          kv->KmtYour.NPAD = n;
        break;
      case 4:
        kv->KmtYour.PADC = b ^ 0x40;
	break;
      case 5:
        if (n<0x20)
          kv->KmtYour.EOL = n;
        break;
      case 6:
	if (KmtCheckQuote(b))
          kv->KmtYour.QCTL = b;
	break;
      case 7:
        if (AckFlag) /* Ack packet from remote host */
        {
          if ((b=='Y') &&
              KmtCheckQuote(kv->KmtMy.QBIN))
            kv->Quote8 = TRUE;
          else if (KmtCheckQuote(b) &&
                   ((b==kv->KmtMy.QBIN) ||
                    (kv->KmtMy.QBIN=='Y')))
          {
            kv->KmtMy.QBIN = b;
            kv->Quote8 = TRUE;
          }
        }
        else /* S-packet from remote host */
          if ((b=='Y') && KmtCheckQuote(kv->KmtMy.QBIN))
            kv->Quote8 = TRUE;
          else if (KmtCheckQuote(b))
          {
            kv->KmtMy.QBIN = b;
            kv->Quote8 = TRUE;
          }

        if (! kv->Quote8) kv->KmtMy.QBIN = 'N';
        kv->KmtYour.QBIN = kv->KmtMy.QBIN;
        break;

      case 8:
        kv->KmtYour.CHKT = b - 0x30;
        if (AckFlag)
        {
          if (kv->KmtYour.CHKT!=kv->KmtMy.CHKT)
            kv->KmtYour.CHKT = DefCHKT;
        }
        else
          if ((kv->KmtYour.CHKT<1) ||
	      (kv->KmtYour.CHKT>2))
            kv->KmtYour.CHKT = DefCHKT;

        kv->KmtMy.CHKT = kv->KmtYour.CHKT;
        break;

      case 9:
        kv->KmtYour.REPT = b;
        if (! AckFlag &&
            (kv->KmtYour.REPT>0x20) &&
            (kv->KmtYour.REPT<0x7F))
          kv->KmtMy.REPT = kv->KmtYour.REPT;
		/*
		Very old bug:
		Kermit fails to properly negotiate to NOT use "repeat"
		compression when talking to a primitive partner (a
		prominent example of a kermit implementation that does
		not support repeat is the bootloader "U-Boot").

		by Anders Larsen (2007/9/11 yutaka)
		 */
        kv->RepeatFlag = kv->KmtMy.REPT == kv->KmtYour.REPT;
        break;
    }
  }

}

void KmtSendAck(PFileVar fv, PKmtVar kv, PComVar cv)
{
  if (kv->PktIn[3]=='S') /* Send-Init packet */
  {
    KmtParseInit(kv,FALSE);
    KmtSendInitPkt(fv,kv,cv,'Y');
  }
  else {
    KmtMakePacket(fv,kv,KmtNum(kv->PktIn[2]),(BYTE)'Y',0);
    KmtSendPacket(fv,kv,cv);
  }
}

void KmtDecode(PFileVar fv, PKmtVar kv, PCHAR Buff, int *BuffLen)
{
  int i, j, DataLen, BuffPtr;
  BYTE b, b2;
  BOOL CTLflag,BINflag,REPTflag,OutFlag;

  BuffPtr = 0;
  DataLen = kv->PktInLen - kv->KmtMy.CHKT - 2;

  OutFlag = FALSE;
  kv->RepeatCount = 1;
  CTLflag = FALSE;
  BINflag = FALSE;
  REPTflag = FALSE;
  for (i = 1 ; i <= DataLen ; i++)
  {
    b = kv->PktIn[3+i];
    b2 = b & 0x7f;
    if (CTLflag)
    {
      if ((b2 != kv->KmtYour.QCTL) &&
          (! kv->Quote8 || (b2 != kv->KmtYour.QBIN)) &&
          (! kv->RepeatFlag || (b2 != kv->KmtYour.REPT)))
        b = b ^ 0x40;
      CTLflag = FALSE;
      OutFlag = TRUE;
    }
    else if (kv->RepeatFlag && REPTflag)
    {
      kv->RepeatCount = KmtNum(b);
      REPTflag = FALSE;
    }
    else if (b==kv->KmtYour.QCTL) CTLflag = TRUE;
    else if (kv->Quote8 && (b==kv->KmtYour.QBIN))
      BINflag = TRUE;
    else if (kv->RepeatFlag && (b==kv->KmtYour.REPT))
      REPTflag = TRUE;
    else OutFlag = TRUE;

    if (OutFlag)
    {
      if (kv->Quote8 && BINflag) b = b ^ 0x80;        
      for (j = 1 ; j <= kv->RepeatCount ; j++)
      {
        if (Buff==NULL) /* write to file */
          _lwrite(fv->FileHandle,&b,1);
        else /* write to buffer */
          if (BuffPtr < *BuffLen)
          {
            Buff[BuffPtr] = b;
            BuffPtr++;
          }
      }
      fv->ByteCount = fv->ByteCount + kv->RepeatCount;
      OutFlag = FALSE;
      kv->RepeatCount = 1;
      CTLflag = FALSE;
      BINflag = FALSE;
      REPTflag = FALSE;
    }
  }

  if (Buff==NULL)
    SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
  *BuffLen = BuffPtr;
}

BOOL KmtEncode(PFileVar fv, PKmtVar kv)
{
  BYTE b, b2, b7;
  int Len;
  char TempStr[4];

  if ((kv->RepeatCount>0) && (strlen(kv->ByteStr)>0))
  {
    kv->RepeatCount--;
    return TRUE;
  }

  if (kv->NextByteFlag)
  {
    b = kv->NextByte;
    kv->NextByteFlag = FALSE;
  }
  else if (_lread(fv->FileHandle,&b,1)==0)
    return FALSE;
  else fv->ByteCount++;

  Len = 0;

  b7 = b & 0x7f;

  /* 8 bit quoting */
  if (kv->Quote8 && (b != b7))
  {
    TempStr[Len] = kv->KmtMy.QBIN;
    Len++;
    b2 = b7;
  }
  else b2 = b;

  if ((b7<0x20) || (b7==0x7F))
  {
    TempStr[Len] = kv->KmtMy.QCTL;
    Len++;
    b2 = b2 ^ 0x40;
  }
  else if ((b7==kv->KmtMy.QCTL) ||
           (kv->Quote8 && (b7==kv->KmtMy.QBIN)) ||
           (kv->RepeatFlag && (b7==kv->KmtMy.REPT)))
  {
    TempStr[Len] = kv->KmtMy.QCTL;
    Len++;
  }

  TempStr[Len] = b2;
  Len++;

  TempStr[Len] = 0;

  kv->RepeatCount = 1;
  if (_lread(fv->FileHandle,&(kv->NextByte),1)==1)
  {
    fv->ByteCount++;
    kv->NextByteFlag = TRUE;
  }

  while (kv->RepeatFlag && kv->NextByteFlag &&
         (kv->NextByte==b) && (kv->RepeatCount<94))
  {
    kv->RepeatCount++;
    if (_lread(fv->FileHandle,&(kv->NextByte),1)==0)
      kv->NextByteFlag = FALSE;
    else fv->ByteCount++;
  }

  if (Len*kv->RepeatCount > Len+2)
  {
    kv->ByteStr[0] = kv->KmtMy.REPT;
    kv->ByteStr[1] = KmtChar((BYTE)(kv->RepeatCount));
    kv->ByteStr[2] = 0;
    strncat_s(kv->ByteStr,sizeof(kv->ByteStr),TempStr,_TRUNCATE);
    kv->RepeatCount = 1;
  }
  else
    strncpy_s(kv->ByteStr, sizeof(kv->ByteStr),TempStr, _TRUNCATE);

  kv->RepeatCount--;
  return TRUE;
}

void KmtIncPacketNum(PKmtVar kv)
{
  kv->PktNum++;
  if (kv->PktNum >= kv->PktNumOffset+64)
    kv->PktNumOffset = kv->PktNumOffset + 64;
}

void KmtSendEOFPacket(PFileVar fv, PKmtVar kv, PComVar cv)
{
  /* close file */
  if (fv->FileOpen)
    _lclose(fv->FileHandle);
  fv->FileOpen = FALSE;

  KmtIncPacketNum(kv);

  KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'Z',0);
  KmtSendPacket(fv,kv,cv);

  kv->KmtState = SendEOF;
}

void KmtSendNextData(PFileVar fv, PKmtVar kv, PComVar cv)
{
  int DataLen, DataLenNew;
  BOOL NextFlag;

  SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
  SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
                fv->ByteCount, fv->FileSize, &fv->ProgStat);
  DataLen = 0;
  DataLenNew = 0;

  NextFlag = KmtEncode(fv,kv);
  if (NextFlag) DataLenNew = DataLen + strlen(kv->ByteStr);
  while (NextFlag &&
         (DataLenNew < kv->KmtYour.MAXL-kv->KmtMy.CHKT-4))
  {
    strncpy_s(&(kv->PktOut[4+DataLen]),sizeof(kv->PktOut) - (4+DataLen),kv->ByteStr,_TRUNCATE);
    DataLen = DataLenNew;
    NextFlag = KmtEncode(fv,kv);
    if (NextFlag) DataLenNew = DataLen + strlen(kv->ByteStr);
  }
  if (NextFlag) kv->RepeatCount++;

  if (DataLen==0)
  {
    SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
    SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
                  fv->ByteCount, fv->FileSize, &fv->ProgStat);
    KmtSendEOFPacket(fv,kv,cv);
  }
  else {
    KmtIncPacketNum(kv);

    KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'D',DataLen);
    KmtSendPacket(fv,kv,cv);

    kv->KmtState = SendData;
  }
}

void KmtSendEOTPacket(PFileVar fv, PKmtVar kv, PComVar cv)
{
  KmtIncPacketNum(kv);
  KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'B',0);
  KmtSendPacket(fv,kv,cv);

  kv->KmtState = SendEOT;
}

BOOL KmtSendNextFile(PFileVar fv, PKmtVar kv, PComVar cv)
{
  char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];

  if (! GetNextFname(fv))
  {
    KmtSendEOTPacket(fv,kv,cv);
    return TRUE;
  }

  /* file open */
  fv->FileHandle = _lopen(fv->FullName,OF_READ);
  fv->FileOpen = fv->FileHandle>0;
  if (! fv->FileOpen)
  {
    if (! fv->NoMsg)
    {
      get_lang_msg("MSG_TT_ERROR", uimsg2, sizeof(uimsg2), "Tera Term: Error", UILanguageFile);
      get_lang_msg("MSG_CANTOEPN_FILE_ERROR", uimsg, sizeof(uimsg), "Cannot open file", UILanguageFile);
      MessageBox(fv->HWin,uimsg,uimsg,MB_ICONEXCLAMATION);
    }
    return FALSE;
  }
  else
    fv->FileSize = GetFSize(fv->FullName);

  fv->ByteCount = 0;

  SetDlgItemText(fv->HWin, IDC_PROTOFNAME, &(fv->FullName[fv->DirLen]));
  SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
  SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
                fv->ByteCount, fv->FileSize, &fv->ProgStat);

  KmtIncPacketNum(kv);
  strncpy_s(&(kv->PktOut[4]),sizeof(kv->PktOut)-4,&(fv->FullName[fv->DirLen]),_TRUNCATE); // put FName
  FTConvFName(&(kv->PktOut[4]));  // replace ' ' by '_' in FName
  KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'F',
                strlen(&(fv->FullName[fv->DirLen])));
  KmtSendPacket(fv,kv,cv);

  kv->RepeatCount = 0;
  kv->NextByteFlag = FALSE;
  kv->KmtState = SendFile;
  return TRUE;
}

void KmtSendReceiveInit(PFileVar fv, PKmtVar kv, PComVar cv)
{
  kv->PktNum = 0;
  kv->PktNumOffset = 0;

  if ((signed int)strlen(&(fv->FullName[fv->DirLen])) >=
      kv->KmtYour.MAXL - kv->KmtMy.CHKT - 4)
    fv->FullName[fv->DirLen+kv->KmtYour.MAXL-kv->KmtMy.CHKT-4] = 0;

  strncpy_s(&(kv->PktOut[4]),sizeof(kv->PktOut)-4,&(fv->FullName[fv->DirLen]),_TRUNCATE);
  KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'R',
                strlen(&(fv->FullName[fv->DirLen])));
  KmtSendPacket(fv,kv,cv);

  kv->KmtState = GetInit;
}

void KmtSendFinish(PFileVar fv, PKmtVar kv, PComVar cv)
{
  kv->PktNum = 0;
  kv->PktNumOffset = 0;

  kv->PktOut[4] = 'F'; /* Finish */
  KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'G',1);
  KmtSendPacket(fv,kv,cv);

  kv->KmtState = Finish;
}

void KmtInit
  (PFileVar fv, PKmtVar kv, PComVar cv, PTTSet ts)
{
  char uimsg[MAX_UIMSG];

  strncpy_s(fv->DlgCaption,sizeof(fv->DlgCaption),"Tera Term: Kermit ",_TRUNCATE);
  switch (kv->KmtMode) {
    case IdKmtSend:
      get_lang_msg("FILEDLG_TRANS_TITLE_KMTSEND", uimsg, sizeof(uimsg), TitKmtSend, UILanguageFile);
      strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
      break;
    case IdKmtReceive:
      get_lang_msg("FILEDLG_TRANS_TITLE_KMTRCV", uimsg, sizeof(uimsg), TitKmtRcv, UILanguageFile);
      strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
      break;
    case IdKmtGet:
      get_lang_msg("FILEDLG_TRANS_TITLE_KMTGET", uimsg, sizeof(uimsg), TitKmtGet, UILanguageFile);
      strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
      break;
    case IdKmtFinish:
      get_lang_msg("FILEDLG_TRANS_TITLE_KMTFIN", uimsg, sizeof(uimsg), TitKmtFin, UILanguageFile);
      strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
      break;
  }

  SetWindowText(fv->HWin,fv->DlgCaption);
  SetDlgItemText(fv->HWin, IDC_PROTOPROT, "Kermit");

  if (kv->KmtMode == IdKmtSend) {
    InitDlgProgress(fv->HWin, IDC_PROTOPROGRESS, &fv->ProgStat);
  }
  else {
    fv->ProgStat = -1;
  }

  fv->FileOpen = FALSE;

  kv->KmtState = Unknown;

  /* default my parameters */
  kv->KmtMy.MAXL = DefMAXL;
  kv->KmtMy.TIME = DefTIME;
  kv->KmtMy.NPAD = DefNPAD;
  kv->KmtMy.PADC = DefPADC;
  kv->KmtMy.EOL  = DefEOL;
  kv->KmtMy.QCTL = DefQCTL;
  if ((cv->PortType==IdSerial) &&
      (ts->DataBit==IdDataBit7))
    kv->KmtMy.QBIN = '&';
  else
    kv->KmtMy.QBIN = MyQBIN;
  kv->KmtMy.CHKT = DefCHKT;
  kv->KmtMy.REPT = MyREPT;

  /* default your parameters */
  kv->KmtYour = kv->KmtMy;

  kv->Quote8 = FALSE;
  kv->RepeatFlag = FALSE;

  kv->PktNumOffset = 0;
  kv->PktNum = 0;

  fv->LogFlag = ((ts->LogFlag & LOG_KMT)!=0);
  if (fv->LogFlag)
    fv->LogFile = _lcreat("KERMIT.LOG",0);

  switch (kv->KmtMode) {
    case IdKmtSend:
      KmtSendInitPkt(fv,kv,cv,'S');
      break;
    case IdKmtReceive:
      kv->KmtState = ReceiveInit;
      FTSetTimeOut(fv,kv->KmtYour.TIME);
      break;
    case IdKmtGet:
      KmtSendInitPkt(fv,kv,cv,'I');
      break;
    case IdKmtFinish:
      KmtSendInitPkt(fv,kv,cv,'I');
      break;
  }
}

void KmtTimeOutProc(PFileVar fv, PKmtVar kv, PComVar cv)
{
  switch (kv->KmtState) {
    case SendInit:
      KmtSendPacket(fv,kv,cv);
      break;
    case SendFile:
      KmtSendPacket(fv,kv,cv);
      break;
    case SendData:
      KmtSendPacket(fv,kv,cv);
      break;
    case SendEOF:
      KmtSendPacket(fv,kv,cv);
      break;
    case SendEOT:
      KmtSendPacket(fv,kv,cv);
      break;
    case ReceiveInit:
      KmtSendNack(fv,kv,cv,KmtChar(0));
      break;
    case ReceiveFile:
      KmtSendNack(fv,kv,cv,kv->NextSeq);
      break;
    case ReceiveData:
      KmtSendNack(fv,kv,cv,kv->NextSeq);
      break;
    case ServerInit:
      KmtSendPacket(fv,kv,cv);
      break;
    case GetInit:
      KmtSendPacket(fv,kv,cv);
      break;
    case Finish:
      KmtSendPacket(fv,kv,cv);
      break;
  }
}

BOOL KmtReadPacket(PFileVar fv,  PKmtVar kv, PComVar cv)
{
  BYTE b;
  int c, PktNumNew;
  BOOL GetPkt;
  char FNBuff[50];
  int i, j, Len;

  c = CommRead1Byte(cv,&b);

  GetPkt = FALSE;

  while ((c>0) && (! GetPkt))
  {
    if (b==1)
    {
      kv->PktReadMode = WaitLen;
      kv->PktIn[0] = b;
    }
    else
      switch (kv->PktReadMode) {
        case WaitLen:
          kv->PktIn[1] = b;
          kv->PktInLen = KmtNum(b);
          kv->PktInCount = kv->PktInLen;
          kv->PktInPtr = 2;
          kv->PktReadMode = WaitCheck;
          break;
        case WaitCheck:
          kv->PktIn[kv->PktInPtr] = b;
          kv->PktInPtr++;
          kv->PktInCount--;
          GetPkt = (kv->PktInCount==0);
          if (GetPkt) kv->PktReadMode = WaitMark;
          break;  
      }

    if (! GetPkt) c = CommRead1Byte(cv,&b);
  }

  if (! GetPkt) return TRUE;

  if (fv->LogFlag)
  {
    _lwrite(fv->LogFile,"< ",2);
    _lwrite(fv->LogFile,&(kv->PktIn[1]),kv->PktInLen+1);
    _lwrite(fv->LogFile,"\015\012",2);
  }

  PktNumNew = KmtCalcPktNum(kv,kv->PktIn[2]);

  GetPkt = KmtCheckPacket(kv);

  /* Ack or Nack */
  if ((kv->PktIn[3]!='Y') &&
      (kv->PktIn[3]!='N'))
  {
    if (GetPkt) KmtSendAck(fv,kv,cv);
      else KmtSendNack(fv,kv,cv,kv->PktIn[2]);
  }

  if (! GetPkt) return TRUE;

  switch (kv->PktIn[3]) {
    case 'B':
      if (kv->KmtState == ReceiveFile)
      {
	fv->Success = TRUE;
	return FALSE;
      }
    case 'D':
      if ((kv->KmtState == ReceiveData) &&
	  (PktNumNew > kv->PktNum))
	KmtDecode(fv,kv,NULL,&Len);
      break;
    case 'E': return FALSE;
    case 'F':
      if ((kv->KmtState==ReceiveFile) ||
	  (kv->KmtState==GetInit))
      {
	kv->KmtMode = IdKmtReceive;

	Len = sizeof(FNBuff);
	KmtDecode(fv,kv,FNBuff,&Len);
	FNBuff[Len] = 0;
	GetFileNamePos(FNBuff,&i,&j);
	strncpy_s(&(fv->FullName[fv->DirLen]),sizeof(fv->FullName) - fv->DirLen,&FNBuff[j],_TRUNCATE);
	/* file open */
	if (! FTCreateFile(fv)) return FALSE;
	kv->KmtState = ReceiveData;
      }
      break;

    case 'S':
      if ((kv->KmtState == ReceiveInit) ||
          (kv->KmtState == GetInit))
      {
        kv->KmtMode = IdKmtReceive;
        kv->KmtState = ReceiveFile;
      }
      break;

    case 'N':
      switch (kv->KmtState) {
	case SendInit:
	  if (PktNumNew==kv->PktNum)
            KmtSendPacket(fv,kv,cv);
	  break;
	case SendFile:
          if (PktNumNew==kv->PktNum)
	    KmtSendPacket(fv,kv,cv);
	  else if (PktNumNew==kv->PktNum+1)
	    KmtSendNextData(fv,kv,cv);
	  break;
	case SendData:
	  if (PktNumNew==kv->PktNum)
	    KmtSendPacket(fv,kv,cv);
	  else if (PktNumNew==kv->PktNum+1)
	    KmtSendNextData(fv,kv,cv);
	  break;
	case SendEOF:
	  if (PktNumNew==kv->PktNum)
	    KmtSendPacket(fv,kv,cv);
	  else if (PktNumNew==kv->PktNum+1)
	  {
	    if (! KmtSendNextFile(fv,kv,cv))
	      return FALSE;
	  }
	  break;
	case SendEOT:
	  if (PktNumNew==kv->PktNum)
	    KmtSendPacket(fv,kv,cv);
	  break;
	case ServerInit:
	  if (PktNumNew==kv->PktNum)
	    KmtSendPacket(fv,kv,cv);
	  break;
	case GetInit:
	  if (PktNumNew==kv->PktNum)
	    KmtSendPacket(fv,kv,cv);
	  break;
	case Finish:
	  if (PktNumNew==kv->PktNum)
	    KmtSendPacket(fv,kv,cv);
	  break;
      }
      break;

    case 'Y':
      switch (kv->KmtState) {
	case SendInit:
	  if (PktNumNew==kv->PktNum)
	  {
	    KmtParseInit(kv,TRUE);
	    if (! KmtSendNextFile(fv,kv,cv))
	      return FALSE;
	  }
	  break;
	case SendFile:
	  if (PktNumNew==kv->PktNum)
	    KmtSendNextData(fv,kv,cv);
	  break;
	case SendData:
	  if (PktNumNew==kv->PktNum)
	    KmtSendNextData(fv,kv,cv);
	  else if (PktNumNew+1==kv->PktNum)
	    KmtSendPacket(fv,kv,cv);
	  break;
	case SendEOF:
	  if (PktNumNew==kv->PktNum)
	  {
	    if (! KmtSendNextFile(fv,kv,cv))
	      return FALSE;
	  }
	  break;
	case SendEOT:
	  if (PktNumNew==kv->PktNum)
	  {
	    fv->Success = TRUE;
	    return FALSE;
	  }
	  break;
	case ServerInit:
	  if (PktNumNew==kv->PktNum)
	  {
	    KmtParseInit(kv,TRUE);
	    switch (kv->KmtMode) {
	      case IdKmtGet:
		KmtSendReceiveInit(fv,kv,cv);
		break;
	      case IdKmtFinish:
		KmtSendFinish(fv,kv,cv);
                break;
	    }
	  }
	  break;
        case Finish:
	  if (PktNumNew==kv->PktNum)
	  {
	    fv->Success = TRUE;
	    return FALSE;
	  }
	  break;
      }
      break;

    case 'Z':
      if (kv->KmtState == ReceiveData)
      {
	if (fv->FileOpen) _lclose(fv->FileHandle);
	fv->FileOpen = FALSE;
	kv->KmtState = ReceiveFile;
      }
  }

  if (kv->KmtMode == IdKmtReceive)
  {
    kv->NextSeq = KmtChar((BYTE)((KmtNum(kv->PktIn[2])+1) % 64));
    kv->PktNum = PktNumNew;
    if (kv->PktNum > kv->PktNumOffset+63)
      kv->PktNumOffset = kv->PktNumOffset + 64;
  }

  SetDlgNum(fv->HWin, IDC_PROTOPKTNUM, kv->PktNum);

  return TRUE;
}

void KmtCancel(PFileVar fv, PKmtVar kv, PComVar cv)
{
  KmtIncPacketNum(kv);
  strncpy_s(&(kv->PktOut[4]),sizeof(kv->PktOut)-4,"Cancel",_TRUNCATE);
  KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'E',
                strlen(&(kv->PktOut[4])));
  KmtSendPacket(fv,kv,cv);
}
