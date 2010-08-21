/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTFILE.DLL, routines for file transfer protocol */

#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "ttlib.h"
#include <stdio.h>
#include <string.h>

#include "tt_res.h"

void FTConvFName(PCHAR FName)
{ // replace ' ' by '_' in FName
  int i;

  i = 0;
  while (FName[i]!=0)
  {
    if (FName[i]==' ')
      FName[i] = '_';
    i++;
  }
}

BOOL GetNextFname(PFileVar fv)
{
  /* next file name exists? */

  if (fv->FNCount >= fv->NumFname)
    return FALSE; /* no more file name */

  fv->FNCount++;
  if (fv->NumFname==1) return TRUE;

  GlobalLock(fv->FnStrMemHandle);

  strncpy_s(&fv->FullName[fv->DirLen],sizeof(fv->FullName) - fv->DirLen,
    &fv->FnStrMem[fv->FnPtr],_TRUNCATE);
  fv->FnPtr = fv->FnPtr + strlen(&fv->FnStrMem[fv->FnPtr]) + 1;

  GlobalUnlock(fv->FnStrMemHandle);

  return TRUE;
}


WORD UpdateCRC(BYTE b, WORD CRC)
{
  int i;

  CRC = CRC ^ (WORD)((WORD)b << 8);
  for (i = 1 ; i <= 8 ; i++)
    if ((CRC & 0x8000)!=0)
      CRC = (CRC << 1) ^ 0x1021;
    else
      CRC = CRC << 1;
  return CRC;
}

LONG UpdateCRC32(BYTE b, LONG CRC)
{
  int i;

  CRC = CRC ^ (LONG)b;
  for (i = 1 ; i <= 8 ; i++)
    if ((CRC & 0x00000001)!=0)
      CRC = (CRC >> 1) ^ 0xedb88320;
    else
      CRC = CRC >> 1;
  return CRC;
}

void FTLog1Byte(PFileVar fv, BYTE b)
{
  char d[3];

  if (fv->FlushLogLineBuf) {
	  int rest = 16 - fv->LogCount;
	  int i;

	  for (i = 0 ; i < rest ; i++)
		 _lwrite(fv->LogFile,"   ", 3);
  }

  if (fv->LogCount == 16 || fv->FlushLogLineBuf)
  {
	  int i;

      // ASCII•\Ž¦‚ð’Ç‰Á (2008.6.3 yutaka)
	  _lwrite(fv->LogFile,"    ", 4);
	  for (i = 0 ; i < fv->LogCount ; i++) {
		  char ch[5];
		  if (isprint(fv->LogLineBuf[i])) {
			  _snprintf_s(ch, sizeof(ch), _TRUNCATE, "%c", fv->LogLineBuf[i]);
			  _lwrite(fv->LogFile, ch, 1);

		  } else {
			  _lwrite(fv->LogFile, ".", 1);

		  }

	  }

    fv->LogCount = 0;
    _lwrite(fv->LogFile,"\015\012",2);

	if (fv->FlushLogLineBuf)
		return;
  }

  if (b<=0x9f)
    d[0] = (b >> 4) + 0x30;
  else
    d[0] = (b >> 4) + 0x37;

  if ((b & 0x0f) <= 0x9)
    d[1] = (b & 0x0F) + 0x30;
  else
    d[1] = (b & 0x0F) + 0x37;

  d[2] = 0x20;
  _lwrite(fv->LogFile,d,3);
  fv->LogLineBuf[fv->LogCount] = b;    // add (2008.6.3 yutaka)
  fv->LogCount++;
}

void FTSetTimeOut(PFileVar fv, int T)
{
  KillTimer(fv->HMainWin, IdProtoTimer);
  if (T==0) return;
  SetTimer(fv->HMainWin, IdProtoTimer, T*1000, NULL);
}

void AddNum(PCHAR FName, int n)
{
  char Num[11];
  int i, j, k, dLen;

  _snprintf_s(Num,sizeof(Num),_TRUNCATE,"%u",n);
  GetFileNamePos(FName,&i,&j);
  k = strlen(FName);
  while ((k>j) && (FName[k]!='.'))
    k--;
  if (FName[k]!='.') k = strlen(FName);
  dLen = strlen(Num);

  if (strlen(FName)+dLen > MAX_PATH - 1)
    dLen = MAX_PATH - 1 - strlen(FName);
  memmove(&FName[k+dLen],&FName[k],strlen(FName)-k+1);
  memcpy(&FName[k+dLen-strlen(Num)],Num,strlen(Num));
}

BOOL FTCreateFile(PFileVar fv)
{
  int i;
  char Temp[MAX_PATH];

  FitFileName(&(fv->FullName[fv->DirLen]),sizeof(fv->FullName) - fv->DirLen,NULL);
  if (! fv->OverWrite)
  {
    i = 0;
    strncpy_s(Temp, sizeof(Temp),fv->FullName, _TRUNCATE);
    while (DoesFileExist(Temp))
    {
      i++;
      strncpy_s(Temp, sizeof(Temp),fv->FullName, _TRUNCATE);
      AddNum(Temp,i);
    }
    strncpy_s(fv->FullName, sizeof(fv->FullName),Temp, _TRUNCATE);
  }
  fv->FileHandle = _lcreat(fv->FullName,0);
  fv->FileOpen = fv->FileHandle>0;
  if (! fv->FileOpen && ! fv->NoMsg)
    MessageBox(fv->HMainWin,"Cannot create file",
	       "Tera Term: Error",MB_ICONEXCLAMATION);
  SetDlgItemText(fv->HWin, IDC_PROTOFNAME,&(fv->FullName[fv->DirLen]));
  fv->ByteCount = 0;
  fv->FileSize = 0;

  if (fv->ProgStat != -1) {
    fv->ProgStat = 0;
  }

  return fv->FileOpen;
}
