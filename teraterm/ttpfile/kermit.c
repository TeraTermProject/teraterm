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

/* Kermit protocol */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "tttypes.h"
#include "ttcommon.h"
#include "protolog.h"
#include "filesys_proto.h"

#include "kermit.h"

typedef struct {
	int MAXL;
	int MAXLX;
	BYTE TIME,NPAD,PADC,EOL,QCTL,QBIN,CHKT,REPT,CAPAS,WINDO,MAXLX1,MAXLX2;
} KermitParam;

#define	KMT_DATAMAX		4000
#define	KMT_PKTMAX		(KMT_DATAMAX + 32)
#define	KMT_PKTQUE		4

typedef struct {
	BYTE PktIn[KMT_PKTMAX], PktOut[KMT_PKTMAX];
	int PktInPtr;
	int PktInLen, PktInCount;
	int PktNum, PktNumOffset;
	int PktReadMode;
	KMT_MODE_T KmtMode;
	int KmtState;
	BOOL Quote8, RepeatFlag;
	char ByteStr[6];
	BOOL NextByteFlag;
	int RepeatCount;
	BYTE NextSeq;
	BYTE NextByte;
	KermitParam KmtMy, KmtYour;
	int PktOutCount, PktInLongPacketLen;
	int FileAttrFlag;
	BOOL FileType;
	time_t FileTime;
	int FileMode;
	LONGLONG FileSize;
	TProtoLog *log;
	const char *FullName;		// Windows��̃t�@�C���� UTF-8
	const wchar_t *UILanguageFileW;

	BOOL FileOpen;
	LONG ByteCount;

	int ProgStat;

	DWORD StartTime;

	DWORD FileMtime;
} TKmtVar;
typedef TKmtVar *PKmtVar;

  /* Kermit states */
#define WaitMark  0
#define WaitLen   1
#define WaitCheck 2

#define Unknown 0
#define SendInit 1
#define SendFile 2
#define SendData 3
#define SendEOF 4
#define SendEOT 5
#define SendFileAttr 6

#define ReceiveInit 6
#define ReceiveFile 7
#define ReceiveData 8

#define ServerInit 9
#define GetInit 10
#define Finish 11

/* kermit parameters */
#define MaxNum 94

#define MinMAXL 10
#define MaxMAXL 94
#define DefMAXL 94
#define MinTIME 1
#define DefTIME 10
#define DefNPAD 0
#define DefPADC 0
#define DefEOL  0x0D
#define DefQCTL '#'
#define MyQBIN  'Y'
#define DefCHKT 1
#define MyREPT  '~'

#define	KMT_CAP_LONGPKT	2
#define	KMT_CAP_SLIDWIN	4
#define	KMT_CAP_FILATTR	8
/*
 * Long Packet �̓���
 * - ��M
 *   KmtLongPacket ���L���Ȃ� kv->KmtMy.CAPAS �� Long Packet ��L���ɂ���
 *   kv->KmtMy.CAPAS �������[�g�ɑ��M����ALong Packet ����M�\�ł��邱�Ƃ�`����
 *   Long Packet �����ł���ƁAKmtLongPacket �̐ݒ�ɂ�����炸��M����
 * - ���M
 *   �����[�g����̒ʒm�� kv->KmtYour.CAPAS �ɕۑ������
 *   kv->KmtYour.CAPAS ���L���� kv->KmtMy.CAPAS (KmtLongPacket) ���L���Ȃ璷�����M�f�[�^���쐬����
 *   LEN �� 94 �𒴂���p�P�b�g�� Long Packet �ő��M����
 */

#define	KMT_ATTR_TIME	001
#define	KMT_ATTR_MODE	002
#define	KMT_ATTR_SIZE	003
#define	KMT_ATTR_TYPE	004

/* MARK [LEN+SEQ+TYPE] DATA CHECK */
#define HEADNUM 3
/* MARK [LEN+SEQ+TYPE+LENX1+LENX2+HCHECK] DATA CHECK */
#define LONGPKT_HEADNUM 6

static BYTE KmtNum(BYTE b);


static void KmtOutputCommonLog(PFileVarProto fv, PKmtVar kv, BYTE *buf, int len)
{
	TProtoLog* log = kv->log;
	int i, datalen, n;
	char str[128];
	char type, *s;

	for (i = 0 ; i < len ; i++)
		log->DumpByte(log, buf[i]);

	// �c���ASCII�\�����s��
	log->DumpFlush(log);

	/* �p�P�b�g��l�Ԃɕ�����₷���\������B
	Packet Format
	+------+-------------+-------------+------+------------+-------+
	| MARK | tochar(LEN) | tochar(SEQ) | TYPE | DATA       | CHECK |
	+------+-------------+-------------+------+------------+-------+
	                     <------------ LEN ------------------------>

	Long Packet Format
	+------+-----+-----+------+-------+-------+--------+----- - - - -+-------+
	| MARK |  0  | SEQ | TYPE | LENX1 | LENX2 | HCHECK | DATA        | CHECK |
	+------+-----+-----+------+-------+-------+--------+----- - - - -+-------+
	                                                   <-- 95*LENX1+LENX2 --->
	 */
	if (len >= 4) {
		type = buf[3];
		switch (type) {
			case 'D': s = "Data"; break;
			case 'Y': s = "ACK"; break;
			case 'N': s = "NAK"; break;
			case 'S': s = "SendInitiate"; break;
			case 'B': s = "EOT"; break;
			case 'F': s = "FileHeader"; break;
			case 'Z': s = "EOF"; break;
			case 'E': s = "Error"; break;
			case 'Q': s = "BlockCheckErrorPsuedoPacket"; break;
			case 'T': s = "TimeoutPsuedoPacket"; break;
			case 'I': s = "Initialize"; break;
			case 'X': s = "Text Header"; break;
			case 'A': s = "FileAttributes"; break;
			case 'C': s = "HostCommand"; break;
			case 'K': s = "KermitCommand"; break;
			case 'G': s = "GenericKermitCommand"; break;
			default: s = "UNKNOWN"; break;
		}
		n = KmtNum(buf[1]);
		if (n >= 3)
			datalen = n - 2 - kv->KmtMy.CHKT;
		else
			datalen = KmtNum(buf[4])*95 + KmtNum(buf[5]) - kv->KmtMy.CHKT;

		_snprintf_s(str, sizeof(str), _TRUNCATE, "MARK=%x LEN=%d SEQ#=%d TYPE=%s DATA_LEN=%d\n",
			buf[0], n, KmtNum(buf[2]), s, datalen);
		log->WriteRaw(log, str, strlen(str));

		// Initial Connection
		if (type == 'S' && datalen >= 6) {
			char *p = &buf[4];
			char t[32];

			_snprintf_s(str, sizeof(str), _TRUNCATE,
				"  Data: MAXL=%d TIME=%d NPAD=%d PADC=%x EOL=%x QCTL=%c ",
				KmtNum(p[0]), KmtNum(p[1]), KmtNum(p[2]), p[3]^0x40, p[4], p[5]
				);

			// QBIN �ȍ~�̓I�v�V���������B
			if (datalen >= 7) {
				_snprintf_s(t, sizeof(t), _TRUNCATE, "QBIN=%c ", p[6]);
				strncat_s(str, sizeof(str), t, _TRUNCATE);
			}
			if (datalen >= 8) {
				_snprintf_s(t, sizeof(t), _TRUNCATE, "CHKT=%c ", p[7]);
				strncat_s(str, sizeof(str), t, _TRUNCATE);
			}
			if (datalen >= 9) {
				_snprintf_s(t, sizeof(t), _TRUNCATE, "REPT=%c ", p[8]);
				strncat_s(str, sizeof(str), t, _TRUNCATE);
			}
			if (datalen >= 10) {
				_snprintf_s(t, sizeof(t), _TRUNCATE, "CAPAS=%x ", p[9]);
				strncat_s(str, sizeof(str), t, _TRUNCATE);
			}
			if (datalen >= 11) {
				_snprintf_s(t, sizeof(t), _TRUNCATE, "WINDO=%x ", p[10]);
				strncat_s(str, sizeof(str), t, _TRUNCATE);
			}
			if (datalen >= 12) {
				_snprintf_s(t, sizeof(t), _TRUNCATE, "MAXLX1=%x ", p[11]);
				strncat_s(str, sizeof(str), t, _TRUNCATE);
			}
			if (datalen >= 13) {
				_snprintf_s(t, sizeof(t), _TRUNCATE, "MAXLX2=%x ", p[12]);
				strncat_s(str, sizeof(str), t, _TRUNCATE);
			}

			log->WriteRaw(log, str, strlen(str));
			log->WriteRaw(log, "\015\012", 2);

		}
	}
}

static void KmtReadLog(PFileVarProto fv, PKmtVar kv, BYTE *buf, int len)
{
	if (kv->log != NULL && (len>0))
	{
		TProtoLog* log = kv->log;
		log->WriteRaw(log, "\015\012<<<\015\012",7);
		KmtOutputCommonLog(fv, kv, buf, len);
	}
}

static void KmtWriteLog(PFileVarProto fv, PKmtVar kv, BYTE *buf, int len)
{
	if (kv->log != NULL && (len>0))
	{
		TProtoLog* log = kv->log;
		log->WriteRaw(log, "\015\012>>>\015\012",7);
		KmtOutputCommonLog(fv, kv, buf, len);
	}
}

static void KmtStringLog(PFileVarProto fv, PKmtVar kv, char *fmt, ...)
{
	char tmp[1024];
	int len;
	va_list arg;

	if (kv->log != NULL) {
		TProtoLog* log = kv->log;
		va_start(arg, fmt);
		len = _vsnprintf_s(tmp, sizeof(tmp), _TRUNCATE, fmt, arg);
		va_end(arg);
		log->WriteRaw(log, tmp, len);
		log->WriteRaw(log, "\015\012", 2);
	}
}

static BYTE KmtNum(BYTE b)
{
	return (b - 32);
}

static BYTE KmtChar(BYTE b)
{
	return (b+32);
}

static void KmtCalcCheck(WORD Sum, BYTE CHKT, PCHAR Check)
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

// a single-character type 1 checksum ���v�Z����
static int KmtCheckSumType1(BYTE *buf, int len)
{
	WORD Sum;
	BYTE check;
	int i;

	Sum = 0;
	for (i = 0 ; i < len ; i++) {
		Sum = Sum + buf[i];
	}
	KmtCalcCheck(Sum, 1, &check);
	return (check);
}

static void KmtSendPacket(PFileVarProto fv, PKmtVar kv, PComVar cv)
{
	int C;

	/* padding characters */
	for (C = 1 ; C <= kv->KmtYour.NPAD ; C++)
		CommBinaryOut(cv,&(kv->KmtYour.PADC), 1);

	/* packet */
	C = kv->PktOutCount;
	CommBinaryOut(cv,&kv->PktOut[0], C);

	if (kv->log != NULL) {
		KmtWriteLog(fv, kv, &(kv->PktOut[0]), C);
	}

	/* end-of-line character */
	if (kv->KmtYour.EOL > 0)
		CommBinaryOut(cv,&(kv->KmtYour.EOL), 1);

	fv->FTSetTimeOut(fv,kv->KmtYour.TIME);
}

static void KmtMakePacket(PFileVarProto fv, PKmtVar kv, BYTE SeqNum, BYTE PktType, int DataLen)
{
	int i, nlen, headnum;
	WORD Sum;

	// SEQ����CHECK�܂ł̒����BMARK��LEN�͊܂܂Ȃ��B
	nlen = DataLen + kv->KmtMy.CHKT + 2;

	kv->PktOut[0] = 1; /* MARK */
	kv->PktOut[1] = KmtChar((BYTE)(nlen)); /* LEN */
	kv->PktOut[2] = KmtChar(SeqNum); /* SEQ */
	kv->PktOut[3] = PktType; /* TYPE */

	/* Long Packet�̏ꍇ */
	if (nlen > MaxNum) {
		int k;
		memmove_s(&kv->PktOut[7], KMT_PKTMAX-7, &kv->PktOut[4], DataLen);
		kv->PktOut[1] = KmtChar(0);  /* LEN=0 */
		k =  DataLen + kv->KmtMy.CHKT;
		kv->PktOut[4] = KmtChar(k / 95);
		kv->PktOut[5] = KmtChar(k % 95);
		Sum = KmtCheckSumType1(&kv->PktOut[1], 5);
		kv->PktOut[6] = (BYTE)Sum;  /* HCHECK */

		/* LEN+SEQ+TYPE+LENX1+LENX2+HCHECK */
		headnum = LONGPKT_HEADNUM;

	} else {
		/* LEN+SEQ+TYPE */
		headnum = HEADNUM;
	}

	/* check sum */
	Sum = 0;
	for (i = 1 ; i <= DataLen + headnum ; i++)
		Sum = Sum + kv->PktOut[i];
	KmtCalcCheck(Sum, kv->KmtMy.CHKT, &(kv->PktOut[DataLen + headnum + 1]));

	/* �o�b�t�@�̑S�̃T�C�Y */
	kv->PktOutCount = 1 + headnum + DataLen + kv->KmtMy.CHKT;
}


static void KmtSendInitPkt(PFileVarProto fv, PKmtVar kv, PComVar cv, BYTE PktType)
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

	if (kv->KmtMy.CAPAS > 0) {
		kv->PktOut[13] = KmtChar(kv->KmtMy.CAPAS);
		NParam++;
		if (kv->KmtMy.CAPAS & KMT_CAP_LONGPKT) {
			kv->PktOut[14] = KmtChar(0);
			kv->PktOut[15] = KmtChar(kv->KmtMy.MAXLX / 95);
			kv->PktOut[16] = KmtChar(kv->KmtMy.MAXLX % 95);
			NParam += 3;
		}
	}

	KmtMakePacket(fv,kv,(BYTE)(kv->PktNum - kv->PktNumOffset),PktType,NParam);
	KmtSendPacket(fv,kv,cv);

	switch (PktType) {
	case 'S': kv->KmtState = SendInit; break;
	case 'I': kv->KmtState = ServerInit; break;
	}
}

static void KmtSendNack(PFileVarProto fv, PKmtVar kv, PComVar cv, BYTE SeqChar)
{
	KmtMakePacket(fv,kv,KmtNum(SeqChar),'N',0);
	KmtSendPacket(fv,kv,cv);
}

static int KmtCalcPktNum(PKmtVar kv, BYTE b)
{
	int n;

	n = KmtNum(b) + kv->PktNumOffset;
	if (n>kv->PktNum+31) n = n - 64;
	else if (n<kv->PktNum-32) n = n + 64;
	return n;
}

static BOOL KmtCheckPacket(PKmtVar kv)
{
	int i, len;
	WORD Sum;
	BYTE Check[3];

	/* Long Packet �̏ꍇ�A�܂� HCHECK �����؂���B */
	if (kv->PktInLen == 0) {
		Sum = KmtCheckSumType1(&kv->PktIn[1], 5);
		if ((BYTE)Sum != kv->PktIn[6])
			return FALSE;
		len = kv->PktInCount - 1 - kv->KmtMy.CHKT;

	} else {
		len = kv->PktInLen+1-kv->KmtMy.CHKT;

	}

	/* Calc sum */
	Sum = 0;
	for (i = 1 ; i <= len ; i++)
		Sum = Sum + kv->PktIn[i];

	/* Calc CHECK */
	KmtCalcCheck(Sum, kv->KmtMy.CHKT, &Check[0]);

	for (i = 1 ; i <= kv->KmtMy.CHKT ; i++)
		if (Check[i-1] !=
			kv->PktIn[ len + i ])
			return FALSE;
	return TRUE;
}

static BOOL KmtCheckQuote(BYTE b)
{
	return (((b>0x20) && (b<0x3f)) ||
		((b>0x5F) && (b<0x7f)));
}

static void KmtParseInit(PKmtVar kv, BOOL AckFlag)
{
	int i, NParam, off;
	int maxlen = 0;
	BYTE b, n;

	if (kv->PktInLen == 0) {  /* Long Packet */
		NParam = kv->PktInLongPacketLen - kv->KmtMy.CHKT;
		off = LONGPKT_HEADNUM;

	} else {
		NParam = kv->PktInLen - 2 - kv->KmtMy.CHKT;
		off = HEADNUM;
	}

	for (i=1 ; i <= NParam ; i++)
	{
		b = kv->PktIn[i + off];
		n = KmtNum(b);
		switch (i) {
		  case 1:
			  if ((MinMAXL<=n) && (n<=MaxMAXL))
				  kv->KmtYour.MAXL = n;
			  break;
		  case 2:
			  if ((MinTIME<=n) && (n<=MaxNum))
				  kv->KmtYour.TIME = n;
			  break;
		  case 3:
			  if (n<=MaxNum)
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
			  {
				  if ((b=='Y') && KmtCheckQuote(kv->KmtMy.QBIN))
					  kv->Quote8 = TRUE;
				  else if (KmtCheckQuote(b))
				  {
					  kv->KmtMy.QBIN = b;
					  kv->Quote8 = TRUE;
				  }
			  }

			  if (! kv->Quote8)
				  kv->KmtMy.QBIN = 'N';
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

		  case 10:  /* CAPAS */
			  kv->KmtYour.CAPAS = n;
			  break;

		  case 11:  /* WINDO */
			  // TBD
			  break;

		  case 12:  /* LENX1 */
			  maxlen = n * 95;
			  break;

		  case 13:  /* LENX2 */
			  maxlen += n;
			  break;
		}
	}

	/* Long Packet �̏ꍇ�AMAXLX ���X�V����B*/
	if (kv->KmtYour.CAPAS & KMT_CAP_LONGPKT) {
		kv->KmtYour.MAXLX = maxlen;

		// ������̑��M�o�b�t�@�T�C�Y�𒴂��Ȃ�����
		if (kv->KmtYour.MAXL > KMT_DATAMAX)
			kv->KmtYour.MAXL = KMT_DATAMAX;
	} else {
		/* Capabilities �������Ă���̂ɁALEN=0 �̏ꍇ�́AMAXL �� DefMAXL �̂܂܂Ƃ���B
		 * TODO: �{���̓G���[�Ƃ��ׂ��H
		 */

	}
}

static void KmtSendAck(PFileVarProto fv, PKmtVar kv, PComVar cv)
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

static void KmtDecode(PFileVarProto fv, PKmtVar kv, PCHAR Buff, int *BuffLen)
{
	int i, j, DataLen, BuffPtr, off;
	BYTE b, b2;
	BOOL CTLflag,BINflag,REPTflag,OutFlag;

	BuffPtr = 0;

	if (kv->PktInLen == 0) {  /* Long Packet */
		DataLen = kv->PktInLongPacketLen - kv->KmtMy.CHKT;
		off = 6;
	} else {
		DataLen = kv->PktInLen - kv->KmtMy.CHKT - 2;
		off = 3;
	}

	OutFlag = FALSE;
	kv->RepeatCount = 1;
	CTLflag = FALSE;
	BINflag = FALSE;
	REPTflag = FALSE;
	for (i = 1 ; i <= DataLen ; i++)
	{
		b = kv->PktIn[off + i];
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
				if (Buff==NULL) { /* write to file */
					TFileIO *fileio = fv->file;
					fileio->WriteFile(fileio,&b,1);
				} else /* write to buffer */
					if (BuffPtr < *BuffLen)
					{
						Buff[BuffPtr] = b;
						BuffPtr++;
					}
			}
			kv->ByteCount = kv->ByteCount + kv->RepeatCount;
			OutFlag = FALSE;
			kv->RepeatCount = 1;
			CTLflag = FALSE;
			BINflag = FALSE;
			REPTflag = FALSE;
		}
	}

	if (Buff==NULL)
		fv->InfoOp->SetDlgByteCount(fv, kv->ByteCount);
	*BuffLen = BuffPtr;
	fv->InfoOp->SetDlgTime(fv, kv->StartTime, kv->ByteCount);
}

static void KmtRecvFileAttr(PFileVarProto fv, PKmtVar kv, PCHAR Buff, int *BuffLen)
{
	int DataLen, off, c, n, j;
	BYTE *p, *q;
	char str[256];
	struct tm tm;

	if (kv->PktInLen == 0) {  /* Long Packet */
		DataLen = kv->PktInLongPacketLen - kv->KmtMy.CHKT;
		off = 6;
	} else {
		DataLen = kv->PktInLen - kv->KmtMy.CHKT - 2;
		off = 3;
	}

	kv->FileAttrFlag = 0;
	p = q = &kv->PktIn[1 + off];
	while ((p - q) < DataLen) {
		c = *p++;

		n = KmtNum(*p);  /* 0-255 */
		p++;
		for (j = 0 ; j < n ; j++) {
			str[j] = *p++;
		}
		str[j] = 0;

		switch(c) {
		case '.':	// System ID
		case '*':	// Encoding
		case '!':	// File length in K byte
		case '-':	// Generic "world" protection code
		case '/':	// Record format
		case '(':	// File Block Size
		case '+':	// Disposition
		case '0':	// System-dependent parameters
			break;
		case '"':	// File type
			kv->FileAttrFlag |= KMT_ATTR_TYPE;
			kv->FileType = (str[0] == 'A' ? TRUE : FALSE);
			break;
		case '#':	// File creation date "[yy]yymmdd[ hh:mm[:ss]]"
			kv->FileAttrFlag |= KMT_ATTR_TIME;
			memset(&tm, 0, sizeof(tm));
			switch (strlen(str)) {
				case 17:
					if ( sscanf_s(str, "%04d%02d%02d %02d:%02d:%02d",
								  &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
								  &tm.tm_hour, &tm.tm_min, &tm.tm_sec) < 6 ) {
						kv->FileTime = time(NULL);
					} else {
						tm.tm_year -= 1900;		// 1900-
						tm.tm_mon -= 1;			// 0 - 11
						kv->FileTime = mktime(&tm);
					}
					break;
				case 14:
					if ( sscanf_s(str, "%04d%02d%02d %02d:%02d",
								  &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
								  &tm.tm_hour, &tm.tm_min) < 5 ) {
						kv->FileTime = time(NULL);
					} else {
						tm.tm_year -= 1900;		// 1900-
						tm.tm_mon -= 1;			// 0 - 11
						tm.tm_sec = 0;
						kv->FileTime = mktime(&tm);
					}
					break;
				case 8:
					if ( sscanf_s(str, "%04d%02d%02d",
								  &tm.tm_year, &tm.tm_mon, &tm.tm_mday) < 3 ) {
						kv->FileTime = time(NULL);
					} else {
						tm.tm_year -= 1900;		// 1900-
						tm.tm_mon -= 1;			// 0 - 11
						tm.tm_hour = 0;
						tm.tm_min = 0;
						tm.tm_sec = 0;
						kv->FileTime = mktime(&tm);
					}
					break;
				case 15:
					if ( sscanf_s(str, "%02d%02d%02d %02d:%02d:%02d",
								  &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
								  &tm.tm_hour, &tm.tm_min, &tm.tm_sec) < 6 ) {
						kv->FileTime = time(NULL);
					} else {
						if (tm.tm_year <= 49) {
							tm.tm_year += 100;		// 1900-
						}
						tm.tm_mon -= 1;			// 0 - 11
						kv->FileTime = mktime(&tm);
					}
					break;
				case 12:
					if ( sscanf_s(str, "%02d%02d%02d %02d:%02d",
								  &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
								  &tm.tm_hour, &tm.tm_min) < 5 ) {
						kv->FileTime = time(NULL);
					} else {
						if (tm.tm_year <= 49) {
							tm.tm_year += 100;		// 1900-
						}
						tm.tm_mon -= 1;			// 0 - 11
						tm.tm_sec = 0;
						kv->FileTime = mktime(&tm);
					}
					break;
				case 6:
					if ( sscanf_s(str, "%02d%02d%02d",
								  &tm.tm_year, &tm.tm_mon, &tm.tm_mday) < 3 ) {
						kv->FileTime = time(NULL);
					} else {
						if (tm.tm_year <= 49) {
							tm.tm_year += 100;		// 1900-
						}
						tm.tm_mon -= 1;			// 0 - 11
						tm.tm_hour = 0;
						tm.tm_min = 0;
						tm.tm_sec = 0;
						kv->FileTime = mktime(&tm);
					}
					break;
				default:
					kv->FileTime = time(NULL);
			}
			break;
		case ',':	// File attribute "664"
			kv->FileAttrFlag |= KMT_ATTR_MODE;
			sscanf_s(str, "%03o", &kv->FileMode);
			break;
		case '1':	// File length in bytes
			kv->FileAttrFlag |= KMT_ATTR_SIZE;
			kv->FileSize = _atoi64(str);
			break;
		}
	}

}

static BOOL KmtEncode(PFileVarProto fv, PKmtVar kv)
{
	BYTE b, b2, b7;
	int Len;
	char TempStr[4];
	TFileIO *file = fv->file;

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
	else if (file->ReadFile(file,&b,1)==0)
		return FALSE;
	else
		kv->ByteCount++;

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
	if (file->ReadFile(file,&(kv->NextByte),1)==1)
	{
		kv->ByteCount++;
		kv->NextByteFlag = TRUE;
	}

	while (kv->RepeatFlag && kv->NextByteFlag &&
		(kv->NextByte==b) && (kv->RepeatCount<94))
	{
		kv->RepeatCount++;
		if (file->ReadFile(file,&(kv->NextByte),1)==0)
			kv->NextByteFlag = FALSE;
		else
			kv->ByteCount++;
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

static void KmtIncPacketNum(PKmtVar kv)
{
	kv->PktNum++;
	if (kv->PktNum >= kv->PktNumOffset+64)
		kv->PktNumOffset = kv->PktNumOffset + 64;
}

static void KmtSendEOFPacket(PFileVarProto fv, PKmtVar kv, PComVar cv)
{
	/* close file */
	if (kv->FileOpen) {
		TFileIO *file = fv->file;
		file->Close(file);
	}
	kv->FileOpen = FALSE;

	KmtIncPacketNum(kv);

	KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'Z',0);
	KmtSendPacket(fv,kv,cv);

	kv->KmtState = SendEOF;
}

static void KmtSendNextData(PFileVarProto fv, PKmtVar kv, PComVar cv)
{
	int DataLen, DataLenNew, maxlen;
	BOOL NextFlag;

	fv->InfoOp->SetDlgByteCount(fv, kv->ByteCount);
	fv->InfoOp->SetDlgPercent(fv, kv->ByteCount, kv->FileSize, &kv->ProgStat);
	fv->InfoOp->SetDlgTime(fv, kv->StartTime, kv->ByteCount);
	DataLen = 0;
	DataLenNew = 0;

	// Long Packet
	//   �����[�g�� CAPAS ���L���A���� Tera Term �̐ݒ肪�L��
	if (kv->KmtYour.CAPAS & KMT_CAP_LONGPKT &&
	    kv->KmtMy.CAPAS & KMT_CAP_LONGPKT) {
		// CommBinaryOut() �̐����� 16KB �ŁAKMT_PKTMAX=4032 �𒴂��Ȃ�
		maxlen = kv->KmtYour.MAXLX - kv->KmtMy.CHKT - 7;

	} else {
		maxlen = kv->KmtYour.MAXL - kv->KmtMy.CHKT - 2;
	}

	NextFlag = KmtEncode(fv,kv);
	if (NextFlag) DataLenNew = DataLen + strlen(kv->ByteStr);
	while (NextFlag &&
		(DataLenNew < maxlen))
	{
		strncpy_s(&(kv->PktOut[4+DataLen]),sizeof(kv->PktOut) - (4+DataLen),kv->ByteStr,_TRUNCATE);
		DataLen = DataLenNew;
		NextFlag = KmtEncode(fv,kv);
		if (NextFlag) DataLenNew = DataLen + strlen(kv->ByteStr);
	}
	if (NextFlag) kv->RepeatCount++;

	if (DataLen==0)
	{
		fv->InfoOp->SetDlgByteCount(fv, kv->ByteCount);
		fv->InfoOp->SetDlgPercent(fv, kv->ByteCount, kv->FileSize, &kv->ProgStat);
		fv->InfoOp->SetDlgTime(fv, kv->StartTime, kv->ByteCount);
		KmtSendEOFPacket(fv,kv,cv);
	}
	else {
		KmtIncPacketNum(kv);

		KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'D',DataLen);
		KmtSendPacket(fv,kv,cv);

		kv->KmtState = SendData;
	}
}

static void KmtSendEOTPacket(PFileVarProto fv, PKmtVar kv, PComVar cv)
{
	KmtIncPacketNum(kv);
	KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'B',0);
	KmtSendPacket(fv,kv,cv);

	kv->KmtState = SendEOT;
}

static BOOL KmtSendNextFile(PFileVarProto fv, PKmtVar kv, PComVar cv)
{
	TFileIO *file = fv->file;
	struct _stati64 st;
	BOOL r;
	char *filename;

	filename = fv->GetNextFname(fv);
	if (filename == NULL)
	{
		KmtSendEOTPacket(fv,kv,cv);
		return TRUE;
	}
	free((void *)kv->FullName);
	kv->FullName = filename;

	if (file->stat(file, kv->FullName, &st) == 0) {
		kv->FileAttrFlag = KMT_ATTR_TIME | KMT_ATTR_MODE | KMT_ATTR_SIZE | KMT_ATTR_TYPE;
		kv->FileType = FALSE; // Binary
		kv->FileTime = st.st_mtime;
		kv->FileMode = st.st_mode;
		kv->FileSize = st.st_size;
	} else {
		kv->FileAttrFlag = 0;
	}

	/* file open */
	r = file->OpenRead(file, kv->FullName);
	kv->FileOpen = r;
	if (!kv->FileOpen)
	{
		if (!fv->NoMsg)
		{
			static const TTMessageBoxInfoW info = {
				"Tera Term",
				"MSG_TT_ERROR", L"Tera Term: Error",
				"MSG_CANTOPEN_FILE_ERROR", L"Cannot open file",
				MB_ICONEXCLAMATION };
			TTMessageBoxW(fv->HWin, &info, kv->UILanguageFileW);
		}
		return FALSE;
	}
	else
		kv->FileSize = file->GetFSize(file, kv->FullName);

	kv->ByteCount = 0;
	kv->ProgStat = 0;
	kv->StartTime = GetTickCount();

	fv->InfoOp->SetDlgProtoFileName(fv, kv->FullName);
	fv->InfoOp->SetDlgByteCount(fv, kv->ByteCount);
	fv->InfoOp->SetDlgPercent(fv, kv->ByteCount, kv->FileSize, &kv->ProgStat);
	fv->InfoOp->SetDlgTime(fv, kv->StartTime, kv->ByteCount);

	KmtIncPacketNum(kv);
	filename = file->GetSendFilename(file, kv->FullName, FALSE, TRUE, FALSE);
	strncpy_s(&(kv->PktOut[4]),sizeof(kv->PktOut)-4, filename,_TRUNCATE); // put FName
	KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'F',
				  strlen(filename));
	KmtSendPacket(fv,kv,cv);
	free(filename);

	kv->RepeatCount = 0;
	kv->NextByteFlag = FALSE;
	kv->KmtState = SendFile;
	return TRUE;
}

static BOOL KmtSendNextFileAttr(PFileVarProto fv, PKmtVar kv, PComVar cv)
{
	char buf[512], s[128];
	char t[64];

	buf[0] = '\0';
	if ( (kv->FileAttrFlag & KMT_ATTR_TYPE) != 0 ) {
		_snprintf_s(s, sizeof(s), _TRUNCATE, "\"%c%s8", KmtChar(2), kv->FileType ? "A" : "B");
		strncat_s(buf, sizeof(buf), s, _TRUNCATE);
	}
	if ( (kv->FileAttrFlag & KMT_ATTR_TIME) != 0 ) {
		struct tm date;
		int len;
		localtime_s(&date, &kv->FileTime);
		len = strftime(t, sizeof(t), "%Y%m%d %H:%M:%S", &date);
		_snprintf_s(s, sizeof(s), _TRUNCATE, "#%c%s", KmtChar(len), t);
		strncat_s(buf, sizeof(buf), s, _TRUNCATE);
	}
	if ( (kv->FileAttrFlag & KMT_ATTR_MODE) != 0 ) {
		_snprintf_s(t, sizeof(t), _TRUNCATE, "%03o", kv->FileMode & 0777);
		_snprintf_s(s, sizeof(s), _TRUNCATE, ",%c%s", KmtChar((BYTE)strlen(t)), t);
		strncat_s(buf, sizeof(buf), s, _TRUNCATE);
	}
	if ( (kv->FileAttrFlag & KMT_ATTR_SIZE) != 0 ) {
		_snprintf_s(t, sizeof(t), _TRUNCATE, "%I64d", kv->FileSize);
		_snprintf_s(s, sizeof(s), _TRUNCATE, "1%c%s", KmtChar((BYTE)strlen(t)), t);
		strncat_s(buf, sizeof(buf), s, _TRUNCATE);
	}

	KmtIncPacketNum(kv);
	strncpy_s(&(kv->PktOut[4]),sizeof(kv->PktOut)-4, buf, _TRUNCATE);
	KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'A',
		strlen(buf));
	KmtSendPacket(fv,kv,cv);

	kv->RepeatCount = 0;
	kv->NextByteFlag = FALSE;
	kv->KmtState = SendFileAttr;
	return TRUE;
}

static void KmtSendReceiveInit(PFileVarProto fv, PKmtVar kv, PComVar cv)
{
	TFileIO *file = fv->file;
	char *filename;
	const size_t filename_len_max = kv->KmtYour.MAXL - kv->KmtMy.CHKT - 4;

	kv->PktNum = 0;
	kv->PktNumOffset = 0;

	filename = file->GetRecieveFilename(file, kv->FullName, FALSE, NULL, FALSE);
	if (strlen(filename) >= filename_len_max) {
		filename[filename_len_max] = 0;
	}

	strncpy_s(&(kv->PktOut[4]),sizeof(kv->PktOut)-4,filename,_TRUNCATE);
	KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'R',
		strlen(filename));
	KmtSendPacket(fv,kv,cv);
	free(filename);

	kv->KmtState = GetInit;
}

static void KmtSendFinish(PFileVarProto fv, PKmtVar kv, PComVar cv)
{
	kv->PktNum = 0;
	kv->PktNumOffset = 0;

	kv->PktOut[4] = 'F'; /* Finish */
	KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'G',1);
	KmtSendPacket(fv,kv,cv);

	kv->KmtState = Finish;
}

static BOOL KmtInit(PFileVarProto fv, PComVar cv, PTTSet ts)
{
	PKmtVar kv = fv->data;

	fv->InfoOp->SetDlgProtoText(fv, "Kermit");

	if (kv->KmtMode == IdKmtSend) {
		fv->InfoOp->InitDlgProgress(fv, &kv->ProgStat);
		kv->StartTime = GetTickCount();
	}
	else {
		kv->ProgStat = -1;
		kv->StartTime = 0;
	}

	kv->FileOpen = FALSE;

	kv->UILanguageFileW = ts->UILanguageFileW;
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

	/* CAPAS: a capability of Kermit
	 * (2012/1/22 yutaka)
	 */
	kv->KmtMy.CAPAS = 0x00;
	if (ts->KermitOpt & KmtOptLongPacket) {
		kv->KmtMy.CAPAS |= KMT_CAP_LONGPKT;
		kv->KmtMy.MAXLX = KMT_DATAMAX;
	}
	else {
		// Tera Term ����L�����ƒʒm���Ă��Ȃ��Ă�
		// Long Packet �𑗂��Ă���ꍇ�̂���
		kv->KmtMy.MAXLX = KMT_DATAMAX;
	}
	if (ts->KermitOpt & KmtOptFileAttr)
		kv->KmtMy.CAPAS |= KMT_CAP_FILATTR;

	/* default your parameters */
	kv->KmtYour = kv->KmtMy;
	kv->KmtYour.CAPAS = 0x00;
	kv->KmtYour.MAXLX = 0;

	kv->Quote8 = FALSE;
	kv->RepeatFlag = FALSE;

	kv->PktNumOffset = 0;
	kv->PktNum = 0;

	if ((ts->LogFlag & LOG_KMT)!=0) {
		TProtoLog* log = ProtoLogCreate();
		char buf[128];
		char ctime_str[128];
		time_t tm = time(NULL);
		ctime_s(ctime_str, sizeof(ctime_str), &tm);

		kv->log = log;
		log->SetFolderW(log, ts->LogDirW);
		log->Open(log, "KERMIT.LOG");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "KERMIT %s start: %s\n",
			kv->KmtMode == IdKmtSend ? "Send" :
			kv->KmtMode == IdKmtReceive ? "Receive" :
			kv->KmtMode == IdKmtGet ? "Get" : "Finish",
			ctime_str
			);
		log->WriteRaw(log, buf, strlen(buf));
	}

	switch (kv->KmtMode) {
	case IdKmtSend:
		KmtSendInitPkt(fv,kv,cv,'S');
		break;
	case IdKmtReceive:
		kv->KmtState = ReceiveInit;
		fv->FTSetTimeOut(fv,kv->KmtYour.TIME);
		break;
	case IdKmtGet:
		KmtSendInitPkt(fv,kv,cv,'I');
		break;
	case IdKmtFinish:
		KmtSendInitPkt(fv,kv,cv,'I');
		break;
	default:
		assert(0);
		return FALSE;
	}

	return TRUE;
}

static void KmtTimeOutProc(PFileVarProto fv, PComVar cv)
{
	PKmtVar kv = fv->data;
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

static BOOL FTCreateFile(PFileVarProto fv)
{
	PKmtVar kv = fv->data;
	TFileIO *file = fv->file;

	fv->InfoOp->SetDlgProtoFileName(fv, kv->FullName);
	kv->FileOpen = file->OpenWrite(file, kv->FullName);
	if (!kv->FileOpen) {
		if (fv->NoMsg) {
			MessageBox(fv->HMainWin,"Cannot create file",
					   "Tera Term: Error",MB_ICONEXCLAMATION);
		}
		return FALSE;
	}

	kv->ByteCount = 0;
	kv->FileSize = 0;
	if (kv->ProgStat != -1) {
		kv->ProgStat = 0;
	}
	kv->StartTime = GetTickCount();

	return TRUE;
}

static BOOL KmtReadPacket(PFileVarProto fv,  PComVar cv)
{
	BYTE b;
	int c, PktNumNew;
	BOOL GetPkt;
	char FNBuff[50];
	int Len;
	PKmtVar kv = fv->data;

	if (kv->KmtMode == IdKmtQuit) {
		// �I�����
		return FALSE;
	}

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

				if (kv->PktInLen == 0) {  /* Long Packet */
					kv->PktInCount = 0;
				} else if (kv->PktInLen >= 3) {  /* Normal Packet */
					kv->PktInCount = kv->PktInLen + 2;
					// OutputDebugPrintf("Normal Packet: %d bytes\n", kv->PktInCount);

					// "Initialize �Ń����[�g�������Ă��� MAXL" + 2 (MARK, LEN)
					// �𒴂���T�C�Y�̃p�P�b�g�������[�g�����낤�Ƃ��Ă���
					if (kv->PktInCount > kv->KmtMy.MAXL + 2) {
						KmtStringLog(fv, kv, "Remote is attempting to send %d bytes, but MAXL from remote is %d. Must be less than or equal to %d bytes.",
						             kv->PktInCount, kv->KmtMy.MAXL, kv->KmtMy.MAXL + 2);
						GetPkt = FALSE;
						kv->PktReadMode = WaitMark;
						goto read_end;
					}
				} else {
					/* If unchar(LEN) = 1 or 2, the packet is invalid and should cause an Error. */
					KmtStringLog(fv, kv, "If unchar(LEN) = %d is 1 or 2, the packet is invalid.", kv->PktInLen);
					GetPkt = FALSE;
					kv->PktReadMode = WaitMark;
					goto read_end;
				}

				kv->PktInPtr = 2;
				kv->PktReadMode = WaitCheck;
				break;
			case WaitCheck:
				kv->PktIn[kv->PktInPtr] = b;
				kv->PktInPtr++;
				/* Long Packet */
				// Tera Term ����L�����ƒʒm���Ă��Ȃ��Ă���M����
				if (kv->PktInCount == 0 && kv->PktInPtr == 6) {
					kv->PktInLongPacketLen = KmtNum(kv->PktIn[4])*95 + KmtNum(kv->PktIn[5]);
					kv->PktInCount = kv->PktInLongPacketLen + 7;
					// OutputDebugPrintf("Long Packet: %d bytes\n", kv->PktInCount);

					// "Initialize �Ń����[�g�������Ă��� MAXLX1*95 + MAXLX2" + 7 (MARK ���� HCHECK)
					// �𒴂���T�C�Y�̃p�P�b�g�������[�g�����낤�Ƃ��Ă���
					//   +1 �� C-Kermit 9.0.305 Alpha.05 ���� C-Kermit 10.0 Beta.04 �܂ł� 1 �o�C�g���������Ă��邽��
					if (kv->PktInCount > kv->KmtMy.MAXLX + 7 + 1) {
						KmtStringLog(fv, kv, "Remote is attempting to send %d bytes, but MAXLX from remote is %d. Must be less than or equal to %d bytes.",
						             kv->PktInCount, kv->KmtMy.MAXLX, kv->KmtMy.MAXLX + 7 + 1);
						GetPkt = FALSE;
						kv->PktReadMode = WaitMark;
						goto read_end;
					}
				}

				// ���҂����o�b�t�@�T�C�Y�ɂȂ�����I���B
				if (kv->PktInCount != 0 && kv->PktInPtr >= kv->PktInCount) {
					GetPkt = TRUE;
				}

				if (GetPkt) kv->PktReadMode = WaitMark;
				break;
			}

		if (! GetPkt) c = CommRead1Byte(cv,&b);
	}

read_end:
	if (! GetPkt) return TRUE;

	if (kv->log != NULL)
	{
		KmtReadLog(fv, kv, &(kv->PktIn[0]), kv->PktInCount);
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
		break;
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
			TFileIO *file = fv->file;
			char *RecievePath;
			kv->KmtMode = IdKmtReceive;

			free((void *)kv->FullName);
			kv->FullName = NULL;

			Len = sizeof(FNBuff);
			KmtDecode(fv,kv,FNBuff,&Len);
			FNBuff[Len] = 0;
			RecievePath = fv->GetRecievePath(fv);
			kv->FullName = file->GetRecieveFilename(file, FNBuff, FALSE, RecievePath, !fv->OverWrite);
			free(RecievePath);
			/* file open */
			if (! FTCreateFile(fv)) return FALSE;
			kv->KmtState = ReceiveData;
		}
		break;

	case 'A':   /* File Attribute(Optional) */
		if ((kv->KmtState == ReceiveData) &&
			(PktNumNew > kv->PktNum)) {
			KmtRecvFileAttr(fv,kv,NULL,&Len);
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
			else if (PktNumNew==kv->PktNum+1) {
				if (kv->KmtYour.CAPAS & KMT_CAP_FILATTR)
					KmtSendNextData(fv,kv,cv);
				else
					KmtSendNextData(fv,kv,cv);
			}
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
			if (PktNumNew==kv->PktNum) {
				if (kv->KmtYour.CAPAS & KMT_CAP_FILATTR)
					KmtSendNextFileAttr(fv,kv,cv);
				else
					KmtSendNextData(fv,kv,cv);
			}
			break;
		case SendFileAttr:
			if (PktNumNew==kv->PktNum) {
				KmtSendNextData(fv,kv,cv);
			}
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
				default:
					assert(0);
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
			TFileIO *file = fv->file;
			if (kv->FileOpen)
				file->Close(file);
			kv->FileOpen = FALSE;
			kv->KmtState = ReceiveFile;

			/* �t�@�C��������ݒ肷��B*/
			if (kv->FileAttrFlag & KMT_ATTR_TIME) {
				struct _utimbuf utm;
				memset(&utm, 0, sizeof(utm));
				utm.actime  = kv->FileTime;
				utm.modtime = kv->FileTime;
				file->utime(file, kv->FullName, &utm);
			}
		}
	}

	if (kv->KmtMode == IdKmtReceive)
	{
		kv->NextSeq = KmtChar((BYTE)((KmtNum(kv->PktIn[2])+1) % 64));
		kv->PktNum = PktNumNew;
		if (kv->PktNum > kv->PktNumOffset+63)
			kv->PktNumOffset = kv->PktNumOffset + 64;
	}

	fv->InfoOp->SetDlgPacketNum(fv, kv->PktNum);

	return TRUE;
}

static void KmtCancel(PFileVarProto fv, PComVar cv)
{
	PKmtVar kv = fv->data;
	KmtIncPacketNum(kv);
	strncpy_s(&(kv->PktOut[4]),sizeof(kv->PktOut)-4,"Cancel",_TRUNCATE);
	KmtMakePacket(fv,kv,(BYTE)(kv->PktNum-kv->PktNumOffset),(BYTE)'E',
		strlen(&(kv->PktOut[4])));
	KmtSendPacket(fv,kv,cv);
	kv->KmtMode = IdKmtQuit;
}

static int SetOptV(PFileVarProto fv, int request, va_list ap)
{
	PKmtVar kv = fv->data;
	switch(request) {
	case KMT_MODE: {
		KMT_MODE_T Mode = va_arg(ap, KMT_MODE_T);
		kv->KmtMode = Mode;
		return 0;
	}
	}
	return -1;
}

static void Destroy(PFileVarProto fv)
{
	PKmtVar kv = fv->data;
	if (kv->log != NULL) {
		TProtoLog* log = kv->log;
		log->Destory(log);
		kv->log = NULL;
	}
	free((void *)kv->FullName);
	kv->FullName = NULL;
	free(kv);
	fv->data = NULL;
}

static const TProtoOp Op = {
	KmtInit,
	KmtReadPacket,
	KmtTimeOutProc,
	KmtCancel,
	SetOptV,
	Destroy,
};

BOOL KmtCreate(PFileVarProto fv)
{
	PKmtVar kv;
	kv = malloc(sizeof(TKmtVar));
	if (kv == NULL) {
		return FALSE;
	}
	memset(kv, 0, sizeof(*kv));
	kv->FileOpen = FALSE;
	fv->data = kv;
	fv->ProtoOp = &Op;

	return TRUE;
}
