/* Tera Term
Copyright(C) 2008 TeraTerm Project
All rights reserved. */

/* TTFILE.DLL, YMODEM protocol */
#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tt_res.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "ftlib.h"
#include "dlglib.h"

#include "ymodem.h"

#define TimeOutInit  10
#define TimeOutC     3
#define TimeOutShort 10
#define TimeOutLong  20
#define TimeOutVeryLong 60

int YRead1Byte(PFileVar fv, PYVar yv, PComVar cv, LPBYTE b)
{
	if (CommRead1Byte(cv,b) == 0)
		return 0;

	if (fv->LogFlag)
	{
		if (fv->LogState==0)
		{
			// 残りのASCII表示を行う
			fv->FlushLogLineBuf = 1;
			FTLog1Byte(fv,0);
			fv->FlushLogLineBuf = 0;

			fv->LogState = 1;
			fv->LogCount = 0;
			fv->FlushLogLineBuf = 0;
			_lwrite(fv->LogFile,"\015\012<<<\015\012",7);
		}
		FTLog1Byte(fv,*b);
	}
	return 1;
}

int YWrite(PFileVar fv, PYVar yv, PComVar cv, PCHAR B, int C)
{
	int i, j;

	i = CommBinaryOut(cv,B,C);
	if (fv->LogFlag && (i>0))
	{
		if (fv->LogState != 0)
		{
			// 残りのASCII表示を行う
			fv->FlushLogLineBuf = 1;
			FTLog1Byte(fv,0);
			fv->FlushLogLineBuf = 0;

			fv->LogState = 0;
			fv->LogCount = 0;
			_lwrite(fv->LogFile,"\015\012>>>\015\012",7);
		}
		for (j=0 ; j <= i-1 ; j++)
			FTLog1Byte(fv,B[j]);
	}
	return i;
}

void YSetOpt(PFileVar fv, PYVar yv, WORD Opt)
{
	char Tmp[21];

	yv->YOpt = Opt;

	strncpy_s(Tmp, sizeof(Tmp),"YMODEM (", _TRUNCATE);
	switch (yv->YOpt) {
	case Yopt1K: /* YMODEM */
		strncat_s(Tmp,sizeof(Tmp),"1k)",_TRUNCATE);
		yv->DataLen = 1024;
		yv->CheckLen = 2;
		break;
	case YoptG: /* YMODEM-g */
		strncat_s(Tmp,sizeof(Tmp),"-g)",_TRUNCATE);
		yv->DataLen = 1024;
		yv->CheckLen = 2;
		break;
	case YoptSingle: /* YMODEM(-g) single mode */
		strncat_s(Tmp,sizeof(Tmp),"single mode)",_TRUNCATE);
		yv->DataLen = 1024;
		yv->CheckLen = 2;
		break;
	}
	SetDlgItemText(fv->HWin, IDC_PROTOPROT, Tmp);
}

void YSendNAK(PFileVar fv, PYVar yv, PComVar cv)
{
	BYTE b;
	int t;

	/* flush comm buffer */
	cv->InBuffCount = 0;
	cv->InPtr = 0;

	yv->NAKCount--;
	if (yv->NAKCount<0)
	{
		if (yv->NAKMode==XnakC)
		{
			YSetOpt(fv,yv,XoptCheck);
			yv->NAKMode = XnakNAK;
			yv->NAKCount = 9;
		}
		else {
			YCancel(fv,yv,cv);
			return;
		}
	}

	if (yv->NAKMode==XnakNAK)
	{
		b = NAK;
		if ((yv->PktNum==0) && (yv->PktNumOffset==0))
			t = TimeOutInit;
		else
			t = yv->TOutLong;
	}
	else {
		b = 'C';
		t = TimeOutC;
	}
	YWrite(fv,yv,cv,&b,1);
	yv->PktReadMode = XpktSOH;
	FTSetTimeOut(fv,t);
}

WORD YCalcCheck(PYVar yv, PCHAR PktBuf)
{
	int i;
	WORD Check;

	if (yv->CheckLen==1) /* CheckSum */
	{
		/* Calc sum */
		Check = 0;
		for (i = 0 ; i <= yv->DataLen-1 ; i++)
			Check = Check + (BYTE)(PktBuf[3+i]);
		return (Check & 0xff);
	}
	else { /* CRC */
		Check = 0;
		for (i = 0 ; i <= yv->DataLen-1 ; i++)
			Check = UpdateCRC(PktBuf[3+i],Check);
		return (Check);
	}
}

BOOL YCheckPacket(PYVar yv)
{
	WORD Check;

	Check = YCalcCheck(yv,yv->PktIn);
	if (yv->CheckLen==1) /* Checksum */
		return ((BYTE)Check==yv->PktIn[yv->DataLen+3]);
	else
		return ((HIBYTE(Check)==yv->PktIn[yv->DataLen+3]) &&
		(LOBYTE(Check)==yv->PktIn[yv->DataLen+4]));  
}

void YInit
(PFileVar fv, PYVar yv, PComVar cv, PTTSet ts)
{
	char inistr[MAXPATHLEN + 10];

	if (! GetNextFname(fv))
	{
		return;
	}
	/* file open */
	fv->FileHandle = _lopen(fv->FullName,OF_READ);
	fv->FileOpen = fv->FileHandle>0;

	fv->LogFlag = ((ts->LogFlag & LOG_Y)!=0);
	if (fv->LogFlag)
		fv->LogFile = _lcreat("YMODEM.LOG",0);
	fv->LogState = 0;
	fv->LogCount = 0;

	fv->FileSize = 0;
	if ((yv->YMode==IdYSend) && fv->FileOpen) {
		fv->FileSize = GetFSize(fv->FullName);
		InitDlgProgress(fv->HWin, IDC_PROTOPROGRESS, &fv->ProgStat);
	}
	else {
		fv->ProgStat = -1;
	}

	SetWindowText(fv->HWin, fv->DlgCaption);
	SetDlgItemText(fv->HWin, IDC_PROTOFNAME, &(fv->FullName[fv->DirLen]));

	yv->PktNumOffset = 0;
	yv->PktNum = 0;
	yv->PktNumSent = 0;
	yv->PktBufCount = 0;
	yv->CRRecv = FALSE;

	fv->ByteCount = 0;

	yv->SendFileInfo = 0;
	yv->SendEot = 0;
	yv->ResendEot = 0;
	yv->LastSendEot = 0;

	if (cv->PortType==IdTCPIP)
	{
		yv->TOutShort = TimeOutVeryLong;
		yv->TOutLong  = TimeOutVeryLong;
	}
	else {
		yv->TOutShort = TimeOutShort;
		yv->TOutLong  = TimeOutLong;
	}  

	YSetOpt(fv,yv,yv->YOpt);

	if (yv->YOpt==XoptCheck)  // TODO
	{
		yv->NAKMode = YnakC;
		yv->NAKCount = 10;
	}
	else {
		yv->NAKMode = YnakG;
		yv->NAKCount = 10;
	}

	switch (yv->YMode) {
	case IdYSend:
		yv->TextFlag = 0;

#if 1
		// ファイル送信開始前に、"rb ファイル名"を自動的に呼び出す。(2007.12.20 yutaka)
		strcpy(ts->YModemRcvCommand, "rb");
		if (ts->YModemRcvCommand[0] != '\0') {
			_snprintf_s(inistr, sizeof(inistr), _TRUNCATE, "%s\015", 
				ts->YModemRcvCommand);
			YWrite(fv,yv,cv, inistr , strlen(inistr));
		}
#endif

		FTSetTimeOut(fv,TimeOutVeryLong);
		break;

	case IdYReceive:
		YSendNAK(fv,yv,cv);
		break;
	}
}

void YCancel(PFileVar fv, PYVar yv, PComVar cv)
{
	BYTE b;

	b = CAN;
	YWrite(fv,yv,cv,&b,1);
	yv->YMode = 0; // quit
}

void YTimeOutProc(PFileVar fv, PYVar yv, PComVar cv)
{
	switch (yv->YMode) {
	case IdXSend:
		yv->YMode = 0; // quit
		break;
	case IdXReceive:
		YSendNAK(fv,yv,cv);
		break;
	}
}

BOOL YReadPacket(PFileVar fv, PYVar yv, PComVar cv)
{
	BYTE b, d;
	int i, c;
	BOOL GetPkt;

	c = YRead1Byte(fv,yv,cv,&b);

	GetPkt = FALSE;

	while ((c>0) && (! GetPkt))
	{
		switch (yv->PktReadMode) {
	  case XpktSOH:
		  if (b==SOH)
		  {
			  yv->PktIn[0] = b;
			  yv->PktReadMode = XpktBLK;
			  if (yv->YOpt==Xopt1K)
				  YSetOpt(fv,yv,XoptCRC);
			  FTSetTimeOut(fv,yv->TOutShort);
		  }
		  else if (b==STX)
		  {
			  yv->PktIn[0] = b;
			  yv->PktReadMode = XpktBLK;
			  YSetOpt(fv,yv,Xopt1K);
			  FTSetTimeOut(fv,yv->TOutShort);
		  }
		  else if (b==EOT)
		  {
			  b = ACK;
			  fv->Success = TRUE;
			  YWrite(fv,yv,cv,&b, 1);
			  return FALSE;
		  }
		  else {
			  /* flush comm buffer */
			  cv->InBuffCount = 0;
			  cv->InPtr = 0;
			  return TRUE;
		  }
		  break;
	  case XpktBLK:
		  yv->PktIn[1] = b;
		  yv->PktReadMode = XpktBLK2;
		  FTSetTimeOut(fv,yv->TOutShort);
		  break;
	  case XpktBLK2:
		  yv->PktIn[2] = b;
		  if ((b ^ yv->PktIn[1]) == 0xff)
		  {
			  yv->PktBufPtr = 3;
			  yv->PktBufCount = yv->DataLen + yv->CheckLen;
			  yv->PktReadMode = XpktDATA;
			  FTSetTimeOut(fv,yv->TOutShort);
		  }
		  else
			  YSendNAK(fv,yv,cv);
		  break;
	  case XpktDATA:
		  yv->PktIn[yv->PktBufPtr] = b;
		  yv->PktBufPtr++;
		  yv->PktBufCount--;
		  GetPkt = yv->PktBufCount==0;
		  if (GetPkt)
		  {
			  FTSetTimeOut(fv,yv->TOutLong);
			  yv->PktReadMode = XpktSOH;
		  }
		  else
			  FTSetTimeOut(fv,yv->TOutShort);
		  break;
		}

		if (! GetPkt) c = YRead1Byte(fv,yv,cv,&b);
	}

	if (! GetPkt) return TRUE;

	if ((yv->PktIn[1]==0) && (yv->PktNum==0) &&
		(yv->PktNumOffset==0))
	{
		if (yv->NAKMode==XnakNAK)
			yv->NAKCount = 10;
		else
			yv->NAKCount = 3;
		YSendNAK(fv,yv,cv);
		return TRUE;
	}

	GetPkt = YCheckPacket(yv);
	if (! GetPkt)
	{
		YSendNAK(fv,yv,cv);
		return TRUE;
	}

	d = yv->PktIn[1] - yv->PktNum;
	if (d>1)
	{
		YCancel(fv,yv,cv);
		return FALSE;
	}

	/* send ACK */
	b = ACK;
	YWrite(fv,yv,cv,&b, 1);
	yv->NAKMode = XnakNAK;
	yv->NAKCount = 10;

	if (d==0) return TRUE;
	yv->PktNum = yv->PktIn[1];
	if (yv->PktNum==0)
		yv->PktNumOffset = yv->PktNumOffset + 256;

	c = yv->DataLen;
	if (yv->TextFlag>0)
		while ((c>0) && (yv->PktIn[2+c]==0x1A))
			c--;

	if (yv->TextFlag>0)
		for (i = 0 ; i <= c-1 ; i++)
		{
			b = yv->PktIn[3+i];
			if ((b==LF) && (! yv->CRRecv))
				_lwrite(fv->FileHandle,"\015",1);
			if (yv->CRRecv && (b!=LF))
				_lwrite(fv->FileHandle,"\012",1);
			yv->CRRecv = b==CR;
			_lwrite(fv->FileHandle,&b,1);
		}
	else
		_lwrite(fv->FileHandle, &(yv->PktIn[3]), c);

	fv->ByteCount = fv->ByteCount + c;

	SetDlgNum(fv->HWin, IDC_PROTOPKTNUM, yv->PktNumOffset+yv->PktNum);
	SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);

	FTSetTimeOut(fv,yv->TOutLong);

	return TRUE;
}

// ファイル送信(local-to-remote)時に、YMODEMサーバからデータが送られてきたときに呼び出される。
BOOL YSendPacket(PFileVar fv, PYVar yv, PComVar cv)
{
	BYTE b;
	int i;
	BOOL SendFlag;
	WORD Check;

	SendFlag = FALSE;
	if (yv->PktBufCount==0)
	{
		i = YRead1Byte(fv,yv,cv,&b);
		do {
			if (i==0) return TRUE;
			switch (b) {
			case ACK:
				if (! fv->FileOpen)
				{
					fv->Success = TRUE;
					return FALSE;
				}
				else if (yv->PktNumSent==(BYTE)(yv->PktNum+1))
				{
					yv->PktNum = yv->PktNumSent;
					if (yv->PktNum==0)
						yv->PktNumOffset = yv->PktNumOffset + 256;
					SendFlag = TRUE;
				}
				break;

			case NAK:
				if (yv->PktNum == 0 && yv->YOpt == Xopt1K)
				{
					/* we wanted 1k with CRC, but the other end specified checksum */
					/* keep the 1k block, but move back to checksum mode.          */
					yv->YOpt = XoptCheck;
					yv->CheckLen = 1;
				}

				// 1回目のEOT送信後のNAK受信で、再度EOTを送る。
				if (yv->SendEot) {
					yv->ResendEot = 1;
				}

				SendFlag = TRUE;
				break;

			case CAN:
				break;

			case 0x43:  // 'C'(43h)
			case 0x47:  // 'G'(47h)
				// 'C'を受け取ると、ブロックの送信を開始する。
				if ((yv->PktNum==0) && (yv->PktNumOffset==0))
				{
					SendFlag = TRUE;
				}
				else if (yv->LastSendEot) {
					SendFlag = TRUE;
				}
				break;
			}
			if (! SendFlag) i = YRead1Byte(fv,yv,cv,&b);
		} while (!SendFlag);
		// reset timeout timer
		FTSetTimeOut(fv,TimeOutVeryLong);

		do {
			i = YRead1Byte(fv,yv,cv,&b);
		} while (i!=0);

		if (yv->LastSendEot) { // オールゼロのブロックを送信して、もうファイルがないことを知らせる。
			if (yv->DataLen==128)
				yv->PktOut[0] = SOH;
			else
				yv->PktOut[0] = STX;
			yv->PktOut[1] = 0;
			yv->PktOut[2] = ~0;

			i = 0;
			while (i < yv->DataLen)
			{
				yv->PktOut[i+3] = 0x00;
				i++;
			}

			Check = YCalcCheck(yv,yv->PktOut);
			if (yv->CheckLen==1) /* Checksum */
				yv->PktOut[yv->DataLen+3] = (BYTE)Check;
			else {
				yv->PktOut[yv->DataLen+3] = HIBYTE(Check);
				yv->PktOut[yv->DataLen+4] = LOBYTE(Check);
			}
			yv->PktBufCount = 3 + yv->DataLen + yv->CheckLen;

		} 
		else if (yv->ResendEot) {  // 2回目のEOT送信
			yv->PktOut[0] = EOT;
			yv->PktBufCount = 1;

			yv->LastSendEot = 1;

		} 
		else if (yv->PktNumSent==yv->PktNum) /* make a new packet */
		{
			BYTE *dataptr = &yv->PktOut[3];
			int eot = 0;  // End Of Transfer

			if (yv->DataLen==128)
				yv->PktOut[0] = SOH;
			else
				yv->PktOut[0] = STX;
			yv->PktOut[1] = yv->PktNumSent;
			yv->PktOut[2] = ~ yv->PktNumSent;

			// ブロック番号のカウントアップ。YMODEMでは"0"から開始する。
			yv->PktNumSent++;

			// ブロック0
			if (yv->SendFileInfo == 0) { // ファイル情報の送信
				struct _stat st;
				int ret, total;
				BYTE buf[1024 + 10];

				yv->SendFileInfo = 1;   // 送信済みフラグon

			   /* timestamp */
			   _stat(fv->FullName, &st);

				ret = _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s",
					&(fv->FullName[fv->DirLen]));
				buf[ret] = 0x00;  // NUL
				total = ret + 1;

				ret = _snprintf_s(&(buf[total]), sizeof(buf) - total, _TRUNCATE, "%lu %lo %o",
					fv->FileSize, (long)st.st_mtime, 0644|_S_IFREG);
				total += ret;

				i = total;
				while (i <= yv->DataLen)
				{
					buf[i] = 0x00;
					i++;
				}

				// データコピー
				memcpy(dataptr, buf, yv->DataLen);

			} else {
				i = 1;
				while ((i<=yv->DataLen) && fv->FileOpen &&
					(_lread(fv->FileHandle,&b,1)==1))
				{
					yv->PktOut[2+i] = b;
					i++;
					fv->ByteCount++;
				}


				if (i>1)
				{
					while (i<=yv->DataLen)
					{
						yv->PktOut[2+i] = 0x1A;
						i++;
					}

				}
				else { /* send EOT */
					if (fv->FileOpen)
					{
						_lclose(fv->FileHandle);
						fv->FileHandle = 0;
						fv->FileOpen = FALSE;
					}

					eot = 1;
				}

			}

			if (eot == 0) {  // データブロック
				Check = YCalcCheck(yv,yv->PktOut);
				if (yv->CheckLen==1) /* Checksum */
					yv->PktOut[yv->DataLen+3] = (BYTE)Check;
				else {
					yv->PktOut[yv->DataLen+3] = HIBYTE(Check);
					yv->PktOut[yv->DataLen+4] = LOBYTE(Check);
				}
				yv->PktBufCount = 3 + yv->DataLen + yv->CheckLen;

			} else {  // EOT
				yv->PktOut[0] = EOT;
				yv->PktBufCount = 1;

				yv->SendEot = 1;  // EOTフラグon。次はNAKを期待する。
				yv->ResendEot = 0;
				yv->LastSendEot = 0;

			}

		}
		else { /* resend packet */
			yv->PktBufCount = 3 + yv->DataLen + yv->CheckLen;
		}

		yv->PktBufPtr = 0;
	}
	/* a NAK or C could have arrived while we were buffering.  Consume it. */
	do {
		i = YRead1Byte(fv,yv,cv,&b);
	} while (i!=0);

	i = 1;
	while ((yv->PktBufCount>0) && (i>0))
	{
		b = yv->PktOut[yv->PktBufPtr];
		i = YWrite(fv,yv,cv,&b, 1);
		if (i>0)
		{
			yv->PktBufCount--;
			yv->PktBufPtr++;
		}
	}

	if (yv->PktBufCount==0)
	{
		SetDlgNum(fv->HWin, IDC_PROTOPKTNUM,
			yv->PktNumOffset+yv->PktNumSent);
		SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, fv->ByteCount);
		SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS,
			fv->ByteCount, fv->FileSize, &fv->ProgStat);
	}

	return TRUE;
}
