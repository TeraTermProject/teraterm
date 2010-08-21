/* Tera Term
Copyright(C) 2008-2010 TeraTerm Project
All rights reserved. */

/* TTFILE.DLL, YMODEM protocol */
#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include <stdio.h>
#include <time.h>
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

// データ転送サイズ。YMODEMでは 128 or 1024 byte をサポートする。
#define SOH_DATALEN	128
#define STX_DATALEN	1024

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
		if (yv->NAKMode==YnakC)
		{
			YSetOpt(fv,yv,XoptCheck);
			yv->NAKMode = YnakC;
			yv->NAKCount = 9;
		}
		else {
			YCancel(fv,yv,cv);
			return;
		}
	}

	if (yv->NAKMode!=YnakC)
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

static void initialize_file_info(PFileVar fv, PYVar yv)
{
	if (yv->YMode == IdYSend) {
		if (fv->FileOpen) {
			_lclose(fv->FileHandle);
		}
		fv->FileHandle = _lopen(fv->FullName,OF_READ);
		fv->FileSize = GetFSize(fv->FullName);
	} else {
		fv->FileHandle = -1;
		fv->FileSize = 0;
	}
	fv->FileOpen = fv->FileHandle>0;

	if (yv->YMode == IdYSend) {
		InitDlgProgress(fv->HWin, IDC_PROTOPROGRESS, &fv->ProgStat);
	} else {
		fv->ProgStat = -1;
	}
	SetDlgItemText(fv->HWin, IDC_PROTOFNAME, &(fv->FullName[fv->DirLen]));

	yv->PktNumOffset = 0;
	yv->PktNum = 0;
	yv->PktNumSent = 0;
	yv->PktBufCount = 0;
	yv->CRRecv = FALSE;
	fv->ByteCount = 0;
	yv->SendFileInfo = 0;
	yv->SendEot = 0;
	yv->LastSendEot = 0;
}

void YInit
(PFileVar fv, PYVar yv, PComVar cv, PTTSet ts)
{
	char inistr[MAX_PATH + 10];

	if (yv->YMode == IdYSend) {
		if (!GetNextFname(fv)) {
			return;
		}
	} 

	fv->LogFlag = ((ts->LogFlag & LOG_Y)!=0);
	if (fv->LogFlag)
		fv->LogFile = _lcreat("YMODEM.LOG",0);
	fv->LogState = 0;
	fv->LogCount = 0;

	SetWindowText(fv->HWin, fv->DlgCaption);

	initialize_file_info(fv, yv);

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

	if (yv->YOpt == Yopt1K)  
	{
		yv->NAKMode = YnakC;
		yv->NAKCount = 10;
	}
	else {
		yv->NAKMode = YnakG;
		yv->NAKCount = 10;
	}

	if (fv->LogFlag) {
		char buf[128];
		time_t tm = time(NULL);

		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "YMODEM %s start: %s\n", 
			yv->YMode == IdYSend ? "Send" : "Recv",
			ctime(&tm) 
			);
		_lwrite(fv->LogFile, buf, strlen(buf));
	}

	switch (yv->YMode) {
	case IdYSend:
		yv->TextFlag = 0;

		// ファイル送信開始前に、"rb ファイル名"を自動的に呼び出す。(2007.12.20 yutaka)
		//strcpy(ts->YModemRcvCommand, "rb");
		if (ts->YModemRcvCommand[0] != '\0') {
			_snprintf_s(inistr, sizeof(inistr), _TRUNCATE, "%s\015", 
				ts->YModemRcvCommand);
			YWrite(fv,yv,cv, inistr , strlen(inistr));
		}

		FTSetTimeOut(fv,TimeOutVeryLong);
		break;

	case IdYReceive:
#if 0   // for debug
		strcpy(inistr, "sb -b svnrev.exe lrzsz-0.12.20.tar.gz\r\n");
//		strcpy(inistr, "sb url3.txt url4.txt url5.txt\r\n");
		YWrite(fv,yv,cv, inistr , strlen(inistr));
#endif
		yv->TextFlag = 0;

		YSendNAK(fv,yv,cv);

		break;
	}
}

void YCancel(PFileVar fv, PYVar yv, PComVar cv)
{
	// five cancels & five backspaces per spec
	BYTE cancel[] = { CAN, CAN, CAN, CAN, CAN, BS, BS, BS, BS, BS };

	YWrite(fv,yv,cv, (PCHAR)&cancel, sizeof(cancel));
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

// YMODEMサーバからファイルを受信する際、ProtoParse()から呼び出される関数。
//
// +-------+-------+--------+---------+-----+
// |Header |Block# |1-Block#| Payload | CRC |
// +-------+-------+--------+---------+-----+
//    1       1        1      128/1024   2      byte
//
// return TRUE: ファイル受信中
//        FALSE: 受信完了
BOOL YReadPacket(PFileVar fv, PYVar yv, PComVar cv)
{
	BYTE b, d;
	int i, c, nak;
	BOOL GetPkt;

	c = YRead1Byte(fv,yv,cv,&b);

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
				  yv->DataLen = SOH_DATALEN;
				  FTSetTimeOut(fv,yv->TOutShort);
			  }
			  else if (b==STX)
			  {
				  yv->PktIn[0] = b;
				  yv->PktReadMode = XpktBLK;
				  yv->DataLen = STX_DATALEN;
				  FTSetTimeOut(fv,yv->TOutShort);
			  }
			  else if (b==EOT)
			  {
				  // EOTが来たら、1つのファイル受信が完了したことを示す。
				  if (fv->FileOpen) {
					  fv->FileOpen = 0;
					  _lclose(fv->FileHandle);
					  fv->FileHandle = -1;

					  // 1回目のEOTに対してNAKを返す
					  b = NAK;
					  YWrite(fv,yv,cv,&b, 1);
					  return TRUE;
				  }

				  initialize_file_info(fv, yv);

				  // EOTに対してACKを返す
				  b = ACK;
				  YWrite(fv,yv,cv,&b, 1);

				  // 次のファイル送信を促すため、'C'を送る。
		  		  YSendNAK(fv,yv,cv);

				  return TRUE;
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

	GetPkt = YCheckPacket(yv);
	if (! GetPkt)
	{
		YSendNAK(fv,yv,cv);
		return TRUE;
	}

	// オールゼロならば、全ファイル受信の完了を示す。
	if (yv->PktIn[1] == 0x00 && yv->PktIn[2] == 0xFF &&
		yv->SendFileInfo == 0
		) {
		c = yv->DataLen;
		while ((c>0) && (yv->PktIn[2+c]==0x00))
			c--;
		if (c == 0) {
		  b = ACK;
		  YWrite(fv,yv,cv,&b, 1);
		  fv->Success = TRUE;
		  return FALSE;
		}
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

		p = &(yv->PktIn[3]);
		name = p;
		strncpy_s(fv->FullName, sizeof(fv->FullName), name, _TRUNCATE);
		nameend = name + 1 + strlen(name);
		if (*nameend) {
			ret = sscanf(nameend, "%ld%lo%o", &bytes_total, &modtime, &mode);
			if (ret >= 1) {
				fv->FileSize = bytes_total;
			}
		}
		fv->FileHandle = _lcreat(fv->FullName,0);
		fv->FileOpen = fv->FileHandle>0;

		fv->DirLen = 0;
		SetDlgItemText(fv->HWin, IDC_PROTOFNAME, &(fv->FullName[fv->DirLen]));

		yv->SendFileInfo = 1;

		// 次のファイル送信を促すため、'C'を送る。
		YSendNAK(fv,yv,cv);

		return TRUE;
	}

	if (yv->PktNum==0)
		yv->PktNumOffset = yv->PktNumOffset + 256;

	c = yv->DataLen;
	if (yv->TextFlag>0)
		while ((c>0) && (yv->PktIn[2+c]==0x1A))
			c--;

	// 最終ブロックの余分なデータを除去する
	if (fv->ByteCount + c > fv->FileSize) {
		c = fv->FileSize - fv->ByteCount;
	}

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
	BYTE firstch, lastrx;

	SendFlag = FALSE;
	if (yv->PktBufCount==0)
	{
		i = YRead1Byte(fv,yv,cv,&b);
		do {
			if (i==0) return TRUE;
			firstch = b;

			switch (b) {
			case ACK:
				// 1回目のEOT送信後のACK受信で、「1ファイル送信」の終わりとする。
				if (yv->SendEot) {
					yv->SendEot = 0;

					// 送信ファイルが残っていない場合は、「全てのファイルを転送終了」を通知する。
					if (!GetNextFname(fv)) {
						yv->LastSendEot = 1;
						break;
					} else {
						initialize_file_info(fv, yv);
					}
				}

				if (! fv->FileOpen) // もう送信するファイルがない場合は、正常終了。
				{
					fv->Success = TRUE;
					return FALSE;
				}
				else if (yv->PktNumSent==(BYTE)(yv->PktNum+1))  // 次のブロックを送る
				{
					// ブロック0（ファイル情報）送信後は、ACK と 'C' を連続して受信することに
					// なっているため、次の'C'を待つ。(2010.6.20 yutaka)
					if ((yv->PktNum==0) && (yv->PktNumOffset==0)) {
						SendFlag = FALSE;
						break;
					}

					yv->PktNum = yv->PktNumSent;
					if (yv->PktNum==0)
						yv->PktNumOffset = yv->PktNumOffset + 256;
					SendFlag = TRUE;
				}
				break;

			case NAK:
				// 1回目のEOT送信後のNAK受信で、最後"EOT"を送る。
				if (yv->SendEot) {
					yv->PktNum = yv->PktNumSent;
					if (yv->PktNum==0)
						yv->PktNumOffset = yv->PktNumOffset + 256;
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
					// ファイル情報送信後、ACK -> 'C' と受信したので、次のブロックを送信する。
					if (yv->SendFileInfo) {
						yv->PktNum = yv->PktNumSent;
						if (yv->PktNum==0)
							yv->PktNumOffset = yv->PktNumOffset + 256;
					}

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

		// 後続のサーバからのデータを読み捨てる。
		do {
			lastrx = firstch;
			i = YRead1Byte(fv,yv,cv,&b);
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

		if (yv->LastSendEot) { // オールゼロのブロックを送信して、もうファイルがないことを知らせる。
			yv->LastSendEot = 0;

			if (yv->DataLen == SOH_DATALEN)
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
			//fv->Success = TRUE;

		} 
		else if (yv->PktNumSent==yv->PktNum) /* make a new packet */
		{
			BYTE *dataptr = &yv->PktOut[3];
			int eot = 0;  // End Of Transfer

			if (yv->DataLen == SOH_DATALEN)
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
				yv->LastSendEot = 0;
			}

		}
		else { /* resend packet */
			yv->PktBufCount = 3 + yv->DataLen + yv->CheckLen;
		}

		yv->PktBufPtr = 0;
	}
	/* a NAK or C could have arrived while we were buffering.  Consume it. */
	// 後続のサーバからのデータを読み捨てる。
	do {
		lastrx = firstch;
		i = YRead1Byte(fv,yv,cv,&b);
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
