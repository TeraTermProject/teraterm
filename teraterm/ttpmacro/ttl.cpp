/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005- TeraTerm Project
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

/* TTMACRO.EXE, Tera Term Language interpreter */

#include "teraterm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "compat_win.h"
#include "tt-version.h"
#include "ttmdlg.h"
#include "ttmbuff.h"
#include "ttmparse.h"
#include "ttmdde.h"
#include "ttmlib.h"
#include "ttlib.h"
#include "ttmenc.h"
#include "ttmenc2.h"
#include "tttypes.h"	// for TitleBuffSize
#include "ttmonig.h"
#include <shellapi.h>
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>

#include "ttl.h"
#include "SFMT.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iptypes.h>
#include <iphlpapi.h>
#include "ttl_gui.h"
#include "codeconv.h"
#include "dllutil.h"
#include "asprintf.h"
#include "win32helper.h"

#define TTERMCOMMAND "TTERMPRO"
#define CYGTERMCOMMAND "cyglaunch -o"

// for 'ExecCmnd' command
static BOOL ParseAgain;
static int IfNest;
static int ElseFlag;
static int EndIfFlag;
// Window handle of the main window
static HWND HMainWin;
// Timeout
static DWORD TimeLimit;
static DWORD TimeStart;
// for 'WaitEvent' command
static int WakeupCondition;

// exit code of TTMACRO
int ExitCode = 0;

// for "FindXXXX" commands
#define NumDirHandle 8
static HANDLE DirHandle[NumDirHandle];
/* for "FileMarkPtr" and "FileSeekBack" commands */
#define NumFHandle 16
static HANDLE FHandle[NumFHandle];
static long FPointer[NumFHandle];

// forward declaration
static int ExecCmnd(void);

static void HandleInit(void)
{
	int i;
	for (i=0; i<_countof(FHandle); i++) {
		FHandle[i] = INVALID_HANDLE_VALUE;
	}
}

/**
 *	@retval	�t�@�C���n���h���C���f�b�N�X(0�`)
 *			-1�̂Ƃ��G���[
 */
static int HandlePut(HANDLE FH)
{
	int i;
	if (FH == INVALID_HANDLE_VALUE) {
		return -1;
	}
	for (i=0; i<_countof(FHandle); i++) {
		if (FHandle[i] == INVALID_HANDLE_VALUE) {
			FHandle[i] = FH;
			FPointer[i] = 0;
			return i;
		}
	}
	return -1;
}

static HANDLE HandleGet(int fhi)
{
	if (fhi < 0 || _countof(FHandle) < fhi) {
		return INVALID_HANDLE_VALUE;
	}
	return FHandle[fhi];
}

static void HandleFree(int fhi)
{
	FHandle[fhi] = INVALID_HANDLE_VALUE;
}

/**
 *	@retval �ǂݍ��݃o�C�g��
 */
static UINT win16_lread(HANDLE hFile, LPVOID lpBuffer, UINT uBytes)
{
	DWORD NumberOfBytesRead;
	BOOL Result = ReadFile(hFile, lpBuffer, uBytes, &NumberOfBytesRead, NULL);
	if (Result == FALSE) {
		return 0;
	}
	return NumberOfBytesRead;
}

/**
 *	@retval �������݃o�C�g��
 */
static UINT win16_lwrite(HANDLE hFile, const char*buf, UINT length)
{
	DWORD NumberOfBytesWritten;
	BOOL result = WriteFile(hFile, buf, length, &NumberOfBytesWritten, NULL);
	if (result == FALSE) {
		return 0;
	}
	return NumberOfBytesWritten;
}

/*
 *	@param[in]	iOrigin
 *				@arg 0(FILE_BEGIN)
 *				@arg 1(FILE_CURRENT)
 *				@arg 2(FILE_END)
 *	@retval �t�@�C���ʒu
 *	@retval HFILE_ERROR((HFILE)-1)	�G���[
 *	@retval INVALID_SET_FILE_POINTER((DWORD)-1) �G���[
 */
static LONG win16_llseek(HANDLE hFile, LONG lOffset, int iOrigin)
{
	DWORD pos = SetFilePointer(hFile, lOffset, NULL, iOrigin);
	if (pos == INVALID_SET_FILE_POINTER) {
		return HFILE_ERROR;
	}
	return pos;
}

BOOL InitTTL(HWND HWin)
{
	int i;
	TVarId ParamsVarId;
	WORD Err;

	HMainWin = HWin;

	if (! InitVar()) return FALSE;
	LockVar();

	// System variables
	NewIntVar("result",0);
	NewIntVar("timeout",0);
	NewIntVar("mtimeout",0);    // �~���b�P�ʂ̃^�C���A�E�g�p (2009.1.23 maya)
	NewStrVar("inputstr","");
	NewStrVar("matchstr","");   // for 'waitregex' command (2005.10.7 yutaka)
	NewStrVar("groupmatchstr1","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr2","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr3","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr4","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr5","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr6","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr7","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr8","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr9","");   // for 'waitregex' command (2005.10.15 yutaka)

	if (ParamCnt == 0) {
		ParamCnt++;
	}
	NewIntVar("paramcnt",ParamCnt);  // �t�@�C�������܂ވ����̌� (2012.4.10 yutaka)

	// ���`���̃p�����[�^�ݒ� (param1 �` param9)
	NewStrVar("param1", (u8)ShortName);
	if (Params) {
		for (i=2; i<=9; i++) {
			char tmpname[10];
			_snprintf_s(tmpname, sizeof(tmpname), _TRUNCATE, "param%d", i);
			if (ParamCnt >= i && Params[i] != NULL) {
				NewStrVar(tmpname, (u8)Params[i]);
			}
			else {
				NewStrVar(tmpname, "");
			}
		}
	}

	// �V�`���̃p�����[�^�ݒ� (params[1�`ParamCnt])
	if (NewStrAryVar("params", ParamCnt+1) == 0) {
		Err = 0;
		GetStrAryVarByName(&ParamsVarId, "params", &Err);
		if (Err == 0) {
			if (ShortName[0] != 0) {
				SetStrValInArray(ParamsVarId, 1, (u8)ShortName, &Err);
			}
			if (Params) {
				for (i=0; i<=ParamCnt; i++) {
					if (i == 1) {
						continue;
					}
					if (Params[i]) {
						SetStrValInArray(ParamsVarId, i, (u8)Params[i], &Err);
						free(Params[i]);
					}
				}
				free(Params);
				Params = NULL;
			}
		}
	}

	ParseAgain = FALSE;
	IfNest = 0;
	ElseFlag = 0;
	EndIfFlag = 0;

	for (i=0; i<NumDirHandle; i++)
		DirHandle[i] = INVALID_HANDLE_VALUE;
	HandleInit();

	// TTL�t�@�C��������J�����g�f�B���N�g����ݒ肷��
	wchar_t *curdir;
	if (!IsRelativePathW(FileName)) {
		// �t���p�X�̂Ƃ�
		// �t�@�C���̃p�X���J�����g�t�H���_�ɐݒ肷��
		curdir = ExtractDirNameW(FileName);
		SetCurrentDirectoryW(curdir);
	}
	else {
		// ���΃p�X�̂Ƃ�
		// (�v���Z�X��)�J�����g�f�B���N�g�����擾����
		hGetCurrentDirectoryW(&curdir);
	}
	// ���݂̃t�H���_��ݒ肷��((�v���Z�X��)�J�����g�f�B���N�g����ݒ肷��)
	TTMSetDir((u8)curdir);
	free(curdir);

	if (! InitBuff(FileName))
	{
		TTLStatus = IdTTLEnd;
		return FALSE;
	}

	UnlockVar();

	if (SleepFlag)
	{ // synchronization for startup macro
		// sleep until Tera Term is ready
		WakeupCondition = IdWakeupInit;
		// WakeupCondition = IdWakeupConnect | IdWakeupDisconn | IdWakeupUnlink;
		TTLStatus = IdTTLSleep;
	}
	else
		TTLStatus = IdTTLRun;

	return TRUE;
}

void EndTTL()
{
	int i;

	CloseStatDlg();

	if (DirHandle[0] != 0) {	// InitTTL() ���ꂸ�� EndTTL() ���΍�
		for (i=0; i<NumDirHandle; i++)
		{
			if (DirHandle[i] != INVALID_HANDLE_VALUE)
				FindClose(DirHandle[i]);
			DirHandle[i] = INVALID_HANDLE_VALUE;
		}
	}

	UnlockVar();
	if (TTLStatus==IdTTLWait)
		KillTimer(HMainWin,IdTimeOutTimer);
	CloseBuff(0);
	EndVar();
}

#if 0
long int CalcTime()
{
	time_t time1;
	struct tm *ptm;

	time1 = time(NULL);
	ptm = localtime(&time1);
	return ((long int)ptm->tm_hour * 3600 +
	        (long int)ptm->tm_min * 60 +
	        (long int)ptm->tm_sec
	       );
}
#endif

//////////////// Beginning of TTL commands //////////////
static WORD TTLCommCmd(char Cmd, int Wait)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	else if (! Linked)
		return ErrLinkFirst;
	else
		return SendCmnd(Cmd,Wait);
}

static WORD TTLCommCmdFile(char Cmd, int Wait)
{
	TStrVal Str;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);

	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		SetFile(Str);
		Err = SendCmnd(Cmd,Wait);
	}
	return Err;
}

static WORD TTLCommCmdBin(char Cmd, int Wait)
{
	int Val;
	WORD Err;

	Err = 0;
	GetIntVal(&Val,&Err);

	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		SetBinary(Val);
		Err = SendCmnd(Cmd,Wait);
	}
	return Err;
}

static WORD TTLCommCmdDeb(void)
{
	int Val;
	WORD Err;

	Err = 0;
	GetIntVal(&Val,&Err);

	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		SetDebug(Val);
	}
	return Err;
}

static WORD TTLCommCmdInt(char Cmd, int Wait)
{
	int Val;
	char Str[21];
	WORD Err;

	Err = 0;
	GetIntVal(&Val,&Err);

	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		_snprintf_s(Str,sizeof(Str),_TRUNCATE,"%d",Val);
		SetFile(Str);
		Err = SendCmnd(Cmd,Wait);
	}
	return Err;
}

static WORD TTLBeep(void)
{
	int val = 0;
	WORD Err = 0;
	UINT type = MB_OK;

	if (CheckParameterGiven()) {
		GetIntVal(&val, &Err);
		if (Err!=0) return Err;

		switch (val) {
		case 0:
			type = -1;
			break;
		case 1:
			type = MB_ICONASTERISK;
			break;
		case 2:
			type = MB_ICONEXCLAMATION;
			break;
		case 3:
			type = MB_ICONHAND;
			break;
		case 4:
			type = MB_ICONQUESTION;
			break;
		case 5:
			type = MB_OK;
			break;
		default:
			return ErrSyntax;
			break;
		}
	}

	MessageBeep(type);

	return 0;
}

static WORD TTLBreak(WORD WId)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	return BreakLoop(WId);
}

static WORD TTLBringupBox(void)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	BringupStatDlg();
	return 0;
}

static WORD TTLCall(void)
{
	TName LabName;
	WORD Err;
	TVariableType VarType;
	TVarId VarId;

	if (GetLabelName(LabName) && (GetFirstChar()==0)) {
		if (CheckVar(LabName, &VarType, &VarId) && (VarType==TypLabel))
			Err = CallToLabel(VarId);
		else
			Err = ErrLabelReq;
	}
	else
	Err = ErrSyntax;

	return Err;
}

static WORD TTLCloseSBox(void)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	CloseStatDlg();
	return 0;
}

static WORD TTLCloseTT(void)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (! Linked)
		return ErrLinkFirst;
	else {
		// Close Tera Term
		SendCmnd(CmdCloseWin,IdTTLWaitCmndEnd);
		EndDDE();
		return 0;
	}
}

static WORD TTLCode2Str(void)
{
	TVarId VarId;
	WORD Err;
	int Num, Len, c, i;
	BYTE d;
	TStrVal Str;

	Err = 0;
	GetStrVar(&VarId, &Err);

	GetIntVal(&Num,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Len = sizeof(Num);
	i = 0;
	for (c=0; c<=Len-1; c++)
	{
		d = (Num >> ((Len-1-c)*8)) & 0xff;
		if ((i>0) || (d!=0))
		{
			Str[i] = d;
			i++;
		}
	}
	Str[i] = 0;
	SetStrVal(VarId, Str);
	return Err;
}

static WORD TTLConnect(WORD mode)
{
	TStrVal Str;
	WORD Err;
	WORD w;

	Str[0] = 0;
	Err = 0;

	if (mode == RsvConnect || CheckParameterGiven()) {
		GetStrVal(Str,&Err);
		if (Err!=0) return Err;
	}

	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (Linked)
	{
		if (ComReady!=0)
		{
			SetResult(2);
			return Err;
		}

		if (mode == RsvConnect) {
			// new connection
			SetFile(Str);
			SendCmnd(CmdConnect,0);

			WakeupCondition = IdWakeupInit;
			TTLStatus = IdTTLSleep;
			return Err;
		}
		else {	// cygwin connection
			TTLCloseTT();
		}
	}

	SetResult(0);
	// link to Tera Term
	if (wcslen(TopicName)==0)
	{
		char TopicNameA[11];
		DWORD e;
		wchar_t *command_line;
		const char *command;
		wchar_t *strW;

		w = HIWORD(HMainWin);
		Word2HexStr(w,TopicNameA);
		w = LOWORD(HMainWin);
		Word2HexStr(w,&(TopicNameA[4]));
		ACPToWideChar_t(TopicNameA, TopicName, _countof(TopicName));

		switch (mode) {
		default:
			assert(FALSE);
			// break;
		case RsvConnect:
			command = TTERMCOMMAND;
			break;
		case RsvCygConnect:
			command = CYGTERMCOMMAND;
			break;
		}

		strW = ToWcharU8(Str);
		aswprintf(&command_line, L"%hs /D=%hs %s", command, TopicNameA, strW);
		e = TTWinExec(command_line);
		free(command_line);
		free(strW);
		if (e != NO_ERROR) {
			return ErrCantConnect;
		}
		TTLStatus = IdTTLInitDDE;
	}
	return Err;
}


/*
 * cf. http://oku.edu.mie-u.ac.jp/~okumura/algo/
 */

enum checksum_type {
	CHECKSUM8,
	CHECKSUM16,
	CHECKSUM32,
	CRC16,
	CRC32
};

static unsigned int checksum32(int n, unsigned char c[])
{
	unsigned long value = 0;
	int i;

	for (i = 0; i < n; i++) {
		value += (c[i] & 0xFF);
	}
	return (value & 0xFFFFFFFF);
}

static unsigned int checksum16(int n, unsigned char c[])
{
	unsigned long value = 0;
	int i;

	for (i = 0; i < n; i++) {
		value += (c[i] & 0xFF);
	}
	return (value & 0xFFFF);
}

static unsigned int checksum8(int n, unsigned char c[])
{
	unsigned long value = 0;
	int i;

	for (i = 0; i < n; i++) {
		value += (c[i] & 0xFF);
	}
	return (value & 0xFF);
}

// CRC-16-CCITT
static unsigned int crc16(int n, unsigned char c[])
{
#define CRC16POLY1  0x1021U  /* x^{16}+x^{12}+x^5+1 */
#define CRC16POLY2  0x8408U  /* ���E�t�] */

	int i, j;
	unsigned long r;

	r = 0xFFFFU;
	for (i = 0; i < n; i++) {
		r ^= c[i];
		for (j = 0; j < CHAR_BIT; j++)
			if (r & 1) r = (r >> 1) ^ CRC16POLY2;
			else       r >>= 1;
	}
	return r ^ 0xFFFFU;
}

static unsigned long crc32(int n, unsigned char c[])
{
#define CRC32POLY1 0x04C11DB7UL
	/* x^{32}+x^{26}+x^{23}+x^{22}+x^{16}+x^{12}+x^{11}+
	   x^{10}+x^8+x^7+x^5+x^4+x^2+x^1+1 */
#define CRC32POLY2 0xEDB88320UL  /* ���E�t�] */
	int i, j;
	unsigned long r;

	r = 0xFFFFFFFFUL;
	for (i = 0; i < n; i++) {
		r ^= c[i];
		for (j = 0; j < CHAR_BIT; j++)
			if (r & 1) r = (r >> 1) ^ CRC32POLY2;
			else       r >>= 1;
	}
	return r ^ 0xFFFFFFFFUL;
}

// �`�F�b�N�T���A���S���Y���E���ʃ��[�`��
static WORD TTLDoChecksum(enum checksum_type type)
{
	TStrVal Str;
	WORD Err;
	TVarId VarId;
	DWORD cksum;
	unsigned char *p;

	Err = 0;
	GetIntVar(&VarId, &Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	if (Str[0]==0) return Err;
	p = (unsigned char *)Str;

	switch (type) {
		case CHECKSUM8:
			cksum = checksum8(strlen(Str), p);
			break;
		case CHECKSUM16:
			cksum = checksum16(strlen(Str), p);
			break;
		case CHECKSUM32:
			cksum = checksum32(strlen(Str), p);
			break;
		case CRC16:
			cksum = crc16(strlen(Str), p);
			break;
		case CRC32:
			cksum = crc32(strlen(Str), p);
			break;
		default:
			cksum = 0;
			break;
	}

	SetIntVal(VarId, cksum);

	return Err;
}

static WORD TTLDoChecksumFile(enum checksum_type type)
{
	TStrVal Str;
	int result = 0;
	WORD Err;
	TVarId VarId;
	HANDLE fh = INVALID_HANDLE_VALUE, hMap = NULL;
	LPBYTE lpBuf = NULL;
	DWORD fsize;
	DWORD cksum;

	Err = 0;
	GetIntVar(&VarId, &Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	if (Str[0]==0) return Err;

	fh = CreateFile(Str,GENERIC_READ,0,NULL,OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,NULL); /* �t�@�C���I�[�v�� */
	if (fh == INVALID_HANDLE_VALUE) {
		result = -1;
		goto error;
	}
	/* �t�@�C���}�b�s���O�I�u�W�F�N�g�쐬 */
	hMap = CreateFileMapping(fh,NULL,PAGE_READONLY,0,0,NULL);
	if (hMap == NULL) {
		result = -1;
		goto error;
	}

	/* �t�@�C�����}�b�v���A�擪�A�h���X��lpBuf�Ɏ擾 */
	lpBuf = (LPBYTE)MapViewOfFile(hMap,FILE_MAP_READ,0,0,0);
	if (lpBuf == NULL) {
		result = -1;
		goto error;
	}

	fsize = GetFileSize(fh,NULL);

	switch (type) {
		case CHECKSUM8:
			cksum = checksum8(fsize, lpBuf);
			break;
		case CHECKSUM16:
			cksum = checksum16(fsize, lpBuf);
			break;
		case CHECKSUM32:
			cksum = checksum32(fsize, lpBuf);
			break;
		case CRC16:
			cksum = crc16(fsize, lpBuf);
			break;
		case CRC32:
			cksum = crc32(fsize, lpBuf);
			break;
		default:
			cksum = 0;
			break;
	}

	SetIntVal(VarId, cksum);

error:
	if (lpBuf != NULL) {
		UnmapViewOfFile(lpBuf);
	}

	if (hMap != NULL) {
		CloseHandle(hMap);
	}
	if (fh != INVALID_HANDLE_VALUE) {
		CloseHandle(fh);
	}

	SetResult(result);

	return Err;
}

static WORD TTLDelPassword(void)
{
	TStrVal Str, Str2;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	if (Str[0]==0) return Err;

	GetAbsPath(Str, sizeof(Str));
	wc ini = wc::fromUtf8(Str);
	DWORD attr = GetFileAttributesW(ini);
	if ((attr == INVALID_FILE_ATTRIBUTES) || (attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		// �t�@�C���͑��݂��Ȃ�
		return Err;
	}
	if (Str2[0] == 0)  // delete all password
		WritePrivateProfileStringW(L"Password", NULL, NULL, ini);
	else  // delete password specified by Str2
		WritePrivateProfileStringW(L"Password", wc::fromUtf8(Str2), NULL, ini);
	return Err;
}

// delpassword2 <filename> <password name>
static WORD TTLDelPassword2(void)
{
	TStrVal FileNameStr, KeyStr;
	WORD Err = 0;

	GetStrVal(FileNameStr, &Err);	// �t�@�C����
	GetStrVal(KeyStr, &Err);		// �L�[��
	if ((Err == 0) && (GetFirstChar() != 0)) {
		Err = ErrSyntax;
	}
	if (Err != 0) {
		return Err;
	}
	if (FileNameStr[0] == 0) {
		return ErrSyntax;
	}

	GetAbsPath(FileNameStr, sizeof(FileNameStr));
	Encrypt2DelPassword(wc::fromUtf8(FileNameStr), KeyStr);
	return 0;
}

static WORD TTLDim(WORD type)
{
	WORD Err, WordId;
	TVariableType VarType;
	TName Name;
	TVarId VarId;
	int size;

	Err = 0;

	if (! GetIdentifier(Name)) return ErrSyntax;
	if (CheckReservedWord(Name, &WordId)) return ErrSyntax;
	if (CheckVar(Name, &VarType, &VarId)) return ErrSyntax;

	GetIntVal(&size, &Err);
	if (Err!=0) return Err;

	if (type == RsvIntDim) {
		Err = NewIntAryVar(Name, size);
	}
	else { // type == RsvStrDim
		Err = NewStrAryVar(Name, size);
	}
	return Err;
}

static WORD TTLDisconnect(void)
{
	WORD Err;
	int Val = 1;
	char Str[21];

	Err = 0;
	// get 1rd arg(optional) if given
	if (CheckParameterGiven()) {
		GetIntVal(&Val, &Err);
	}

	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		_snprintf_s(Str,sizeof(Str),_TRUNCATE,"%d",Val);
		SetFile(Str);
		Err = SendCmnd(CmdDisconnect,0);
	}
	return Err;
}

static WORD TTLDispStr(void)
{
	TStrVal Str, buff;
	WORD Err;
	TVariableType ValType;
	int Val;

	if (! Linked)
		return ErrLinkFirst;

	buff[0] = 0;

	while (TRUE) {
		if (GetString(Str, &Err)) {
			if (Err) return Err;
			strncat_s(buff, MaxStrLen, Str, _TRUNCATE);
		}
		else if (GetExpression(&ValType, &Val, &Err)) {
			if (Err!=0) return Err;
			switch (ValType) {
				case TypInteger:
					Str[0] = LOBYTE(Val);
					Str[1] = 0;
					strncat_s(buff, MaxStrLen, Str, _TRUNCATE);
					/* Falls through. */
				case TypString:
					strncat_s(buff, MaxStrLen, StrVarPtr((TVarId)Val), _TRUNCATE);
					break;
				default:
					return ErrTypeMismatch;
			}
		}
		else {
			break;
		}
	}
	SetFile(buff);
	return SendCmnd(CmdDispStr, 0);
}

static WORD TTLDo(void)
{
	WORD WId, Err;
	int Val = 1;

	Err = 0;
	if (CheckParameterGiven()) {
		if (GetReservedWord(&WId)) {
			switch (WId) {
				case RsvWhile:
					GetIntVal(&Val,&Err);
					break;
				case RsvUntil:
					GetIntVal(&Val,&Err);
					Val = Val == 0;
					break;
				default:
					Err = ErrSyntax;
			}
			if ((Err==0) && (GetFirstChar()!=0))
				Err = ErrSyntax;
		}
		else {
			Err = ErrSyntax;
		}
	}

	if (Err!=0) return Err;

	if (Val!=0)
		return SetWhileLoop();
	else
		EndWhileLoop();
	return Err;
}

static WORD TTLElse(void)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (IfNest<1)
		return ErrInvalidCtl;

	// Skip until 'EndIf'
	IfNest--;
	EndIfFlag = 1;
	return 0;
}

static int CheckElseIf(LPWORD Err)
{
	int Val;
	WORD WId;

	*Err = 0;
	GetIntVal(&Val,Err);
	if (*Err!=0) return 0;
	if (! GetReservedWord(&WId) ||
	    (WId!=RsvThen) ||
	    (GetFirstChar()!=0))
		*Err = ErrSyntax;
	return Val;
}

static WORD TTLElseIf(void)
{
	WORD Err;
	int Val;

	Val = CheckElseIf(&Err);
	if (Err!=0) return Err;

	if (IfNest<1)
		return ErrInvalidCtl;

	// Skip until 'EndIf'
	IfNest--;
	EndIfFlag = 1;
	return Err;
}

static WORD TTLEnd(void)
{
	if (GetFirstChar()==0)
	{
		TTLStatus = IdTTLEnd;
		return 0;
	}
	else
		return ErrSyntax;
}

static WORD TTLEndIf(void)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (IfNest<1)
		return ErrInvalidCtl;

	IfNest--;
	return 0;
}

static WORD TTLEndWhile(BOOL mode)
{
	WORD Err;
	int Val = mode;

	Err = 0;
	if (CheckParameterGiven()) {
		GetIntVal(&Val,&Err);
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	return BackToWhile((Val!=0) == mode);
}

static WORD TTLExec(void)
{
	TStrVal Str,Str2, CurDir;
	int mode = SW_SHOW;
	int wait = 0;
	DWORD ret;
	WORD Err;
	STARTUPINFOW sui;
	PROCESS_INFORMATION pi;
	BOOL bRet;

	memset(CurDir, 0, sizeof(CurDir));

	Err = 0;
	GetStrVal(Str,&Err);

	if (CheckParameterGiven()) {
		GetStrVal(Str2, &Err);
		if (Err!=0) return Err;

		if (_stricmp(Str2, "hide") == 0)
			mode = SW_HIDE;
		else if (_stricmp(Str2, "minimize") == 0)
			mode = SW_MINIMIZE;
		else if (_stricmp(Str2, "maximize") == 0)
			mode = SW_MAXIMIZE;
		else if (_stricmp(Str2, "show") == 0)
			mode = SW_SHOW;
		else
			Err = ErrSyntax;

		// get 3nd arg(optional) if given
		if (CheckParameterGiven()) {
			GetIntVal(&wait, &Err);
			if (Err!=0) return Err;

			// get 4th arg(optional) if given
			if (CheckParameterGiven()) {
				GetStrVal(CurDir, &Err);
				if (Err!=0) return Err;
			}
		}
	}

	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	memset(&sui, 0, sizeof(sui));
	sui.cb = sizeof(STARTUPINFO);
	sui.wShowWindow = mode;
	sui.dwFlags = STARTF_USESHOWWINDOW;
	wc StrW = wc::fromUtf8(Str);
	wc CurDirW = wc::fromUtf8(CurDir);
	const wchar_t *pStrW = StrW;
	const wchar_t *pCurdirW = NULL;
	if (CurDir[0] != 0) {
		pCurdirW = CurDirW;
	}
	bRet = CreateProcessW(NULL, (LPWSTR)pStrW, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, pCurdirW, &sui, &pi);
	if (bRet == FALSE) {
		// ���s�ł��Ȃ������ꍇ�Aresult��-1��Ԃ�
		SetResult(-1);
#if 0
		// �G���[�ɂȂ�
		Err = ErrCantExec;
#endif
	} else {
		if (wait) {
			// ���s & wait�w��
			WaitForSingleObject(pi.hProcess, INFINITE);
			GetExitCodeProcess(pi.hProcess, &ret);
			SetResult(ret);
		} else {
			SetResult(0);
		}
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	return Err;
}

static WORD TTLExecCmnd(void)
{
	WORD Err;
	TStrVal NextLine;
	BYTE b;

	Err = 0;
	GetStrVal(NextLine,&Err);
	if (Err!=0) return Err;
	if (GetFirstChar()!=0)
		return ErrSyntax;

	strncpy_s(LineBuff, sizeof(LineBuff),NextLine, _TRUNCATE);
	LineLen = (WORD)strlen(LineBuff);
	LinePtr = 0;
	b = GetFirstChar();
	LinePtr--;
	ParseAgain = (b!=0) && (b!=':') && (b!=';');
	return Err;
}

static WORD TTLExit(void)
{
	if (GetFirstChar()==0)
	{
		if (! ExitBuffer())
			TTLStatus = IdTTLEnd;
		return 0;
	}
	else
	return ErrSyntax;
}

static WORD TTLExpandEnv(void)
{
	WORD Err;
	TVarId VarId;
	TStrVal deststr, srcptr;

	Err = 0;
	GetStrVar(&VarId, &Err);
	if (Err!=0) return Err;

	if (CheckParameterGiven()) { // expandenv strvar strval
		GetStrVal(srcptr,&Err);
		if ((Err==0) && (GetFirstChar()!=0))
			Err = ErrSyntax;
		if (Err!=0) {
			return Err;
		}

		// �t�@�C���p�X�Ɋ��ϐ����܂܂�Ă���Ȃ�΁A�W�J����B
		ExpandEnvironmentStrings(srcptr, deststr, MaxStrLen);
		SetStrVal(VarId, deststr);
	}
	else { // expandenv strvar
		// �t�@�C���p�X�Ɋ��ϐ����܂܂�Ă���Ȃ�΁A�W�J����B
		ExpandEnvironmentStrings(StrVarPtr(VarId), deststr, MaxStrLen);
		SetStrVal(VarId, deststr);
	}

	return Err;
}

static WORD TTLFileClose(void)
{
	WORD Err;
	int fhi;	// handle index
	HANDLE FH;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	CloseHandle(FH);
	HandleFree(fhi);
	return Err;
}

static WORD TTLFileConcat(void)
{
	WORD Err;
	TStrVal FName1, FName2;

	Err = 0;
	GetStrVal(FName1,&Err);
	GetStrVal(FName2,&Err);
	if ((Err==0) &&
	    ((strlen(FName1)==0) ||
	     (strlen(FName2)==0) ||
	     (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName1,sizeof(FName1))) {
		SetResult(-1);
		return Err;
	}
	if (!GetAbsPath(FName2,sizeof(FName2))) {
		SetResult(-2);
		return Err;
	}
	if (_stricmp(FName1,FName2)==0) {
		SetResult(2);
		return Err;
	}

	wc FName1W = wc::fromUtf8(FName1);
	HANDLE FH1 = CreateFileW(FName1W,
							 GENERIC_WRITE, 0, NULL,
							 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FH1 == INVALID_HANDLE_VALUE) {
		SetResult(3);
		return Err;
	}
	SetFilePointer(FH1, 0, NULL, FILE_END);

	int result = 0;
	wc FName2W = wc::fromUtf8(FName2);
	HANDLE FH2 = CreateFileW(FName2W,
							 GENERIC_READ, FILE_SHARE_READ, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FH2 != INVALID_HANDLE_VALUE)
	{
		DWORD c;
		BYTE buf[13];
		do {
			BOOL Result = ReadFile(FH2, buf, sizeof(buf), &c, NULL);
			if (Result == FALSE) {
				// 0�o�C�g�ǂݍ��݂̂Ƃ��� TRUE ���Ԃ�
				result = -4;
				break;
			}
			if (c > 0) {
				DWORD NumberOfBytesWritten;
				Result = WriteFile(FH1, buf, c, &NumberOfBytesWritten, NULL);
				if (Result == FALSE || c != NumberOfBytesWritten) {
					result = -5;
					break;
				}
			}
		} while (c >= sizeof(buf));
		CloseHandle(FH2);
	}
	CloseHandle(FH1);

	SetResult(result);
	return Err;
}

static WORD TTLFileCopy(void)
{
	WORD Err;
	TStrVal FName1, FName2;
	BOOL ret;

	Err = 0;
	GetStrVal(FName1,&Err);
	GetStrVal(FName2,&Err);
	if ((Err==0) &&
	    ((strlen(FName1)==0) ||
	     (strlen(FName2)==0) ||
	     (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName1,sizeof(FName1))) {
		SetResult(-1);
		return Err;
	}
	if (!GetAbsPath(FName2,sizeof(FName2))) {
		SetResult(-2);
		return Err;
	}
	if (_stricmp(FName1, FName2) == 0) {
		SetResult(-3);
		return Err;
	}

	ret = CopyFileW(wc::fromUtf8(FName1), wc::fromUtf8(FName2), FALSE);
	if (ret == 0) {
		SetResult(-4);
		return Err;
	}

	SetResult(0);
	return Err;
}

static WORD TTLFileCreate(void)
{
	WORD Err;
	TVarId VarId;
	HANDLE FH;
	int fhi;
	TStrVal FName;

	Err = 0;
	GetIntVar(&VarId, &Err);
	GetStrVal(FName, &Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetIntVal(VarId, -1);
		SetResult(-1);
		return Err;
	}
	wc FNameW = wc::fromUtf8(FName);
	// TTL �̃t�@�C���n���h���� filelock �Ń��b�N����̂ŁA
	// dwShareMode �ł̋��L���[�h�� Read/Write �Ƃ��L���ɂ���B
	FH = CreateFileW(FNameW,
					 GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
					 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FH == INVALID_HANDLE_VALUE) {
		SetResult(2);
	}
	else {
		SetResult(0);
	}
	fhi = HandlePut(FH);
	SetIntVal(VarId, fhi);
	if (fhi == -1) {
		CloseHandle(FH);
	}
	return Err;
}

static WORD TTLFileDelete(void)
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetResult(-1);
		return Err;
	}

	if (DeleteFileW(wc::fromUtf8(FName)) == 0) {
		SetResult(-1);
	}
	else {
		SetResult(0);
	}

	return Err;
}

static WORD TTLFileMarkPtr(void)
{
	WORD Err;
	int fhi;
	HANDLE FH;
	LONG pos;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	pos = win16_llseek(FH,0,1);	 /* mark current pos */
	if (pos == INVALID_SET_FILE_POINTER) {
		pos = 0;	// ?
	}
	FPointer[fhi] = pos;
	return Err;
}

static WORD TTLFileOpen(void)
{
	WORD Err;
	TVarId VarId;
	int fhi;
	HANDLE FH;
	int Append, ReadonlyFlag=0;
	TStrVal FName;

	Err = 0;
	GetIntVar(&VarId, &Err);
	GetStrVal(FName, &Err);
	GetIntVal(&Append, &Err);
	// get 4nd arg(optional) if given
	if (CheckParameterGiven()) { // readonly
		GetIntVal(&ReadonlyFlag, &Err);
	}
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetIntVal(VarId, -1);
		return Err;
	}

	wc FNameW = wc::fromUtf8(FName);
	if (ReadonlyFlag) {
		FH = CreateFileW(FNameW,
						 GENERIC_READ, FILE_SHARE_READ, NULL,
						 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	else {
		// �t�@�C�����I�[�v������B
		// ���݂��Ȃ��ꍇ�͍쐬������I�[�v������B
		// TTL �̃t�@�C���n���h���� filelock �Ń��b�N����̂ŁA
		// dwShareMode �ł̋��L���[�h�� Read/Write �Ƃ��L���ɂ���B
		FH = CreateFileW(FNameW,
						 GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
						 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	if (FH == INVALID_HANDLE_VALUE) {
		SetIntVal(VarId, -1);
		return Err;
	}
	fhi = HandlePut(FH);
	if (fhi == -1) {
		SetIntVal(VarId, -1);
		CloseHandle(FH);
		return Err;
	}
	SetIntVal(VarId, fhi);
	if (Append!=0) {
		SetFilePointer(FH, 0, NULL, FILE_END);
	}
	return 0;	// no error
}

// Format: filelock <file handle> [<timeout>]
// (2012.4.19 yutaka)
static WORD TTLFileLock(void)
{
	WORD Err;
	HANDLE FH;
	int fhi;
	int timeoutI;
	DWORD timeout;
	int result;
	BOOL ret;
	DWORD dwStart;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	if (Err!=0) return Err;

	timeout = -1;  // ������
	if (CheckParameterGiven()) {
		GetIntVal(&timeoutI, &Err);
		if (Err!=0) return Err;
	}
	timeout = timeoutI * 1000;

	result = 1;  // error
	dwStart = GetTickCount();
	do {
		ret = LockFile(FH, 0, 0, (DWORD)-1, (DWORD)-1);
		if (ret != 0) { // ���b�N����
			result = 0;  // success
			break;
		}
		Sleep(1);
	} while (GetTickCount() - dwStart < (timeout*1000));

	SetResult(result);
	return Err;
}

// Format: fileunlock <file handle>
// (2012.4.19 yutaka)
static WORD TTLFileUnLock(void)
{
	WORD Err;
	HANDLE FH;
	int fhi;
	BOOL ret;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	ret = UnlockFile(FH, 0, 0, (DWORD)-1, (DWORD)-1);
	if (ret != 0) { // �A�����b�N����
		SetResult(0);
	} else {
		SetResult(1);
	}

	return Err;
}

static WORD TTLFileReadln(void)
{
	WORD Err;
	TVarId VarId;
	int fhi;
	HANDLE FH;
	int i, c;
	TStrVal Str;
	BOOL EndFile, EndLine;
	BYTE b;

	Err = 0;
	GetIntVal(&fhi, &Err);
	FH = HandleGet(fhi);
	GetStrVar(&VarId, &Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	i = 0;
	EndLine = FALSE;
	EndFile = TRUE;
	do {
		c = win16_lread(FH, &b, 1);
		if (c>0) EndFile = FALSE;
		if (c==1) {
			switch (b) {
				case 0x0d:
					c = win16_lread(FH, &b, 1);
					if ((c==1) && (b!=0x0a))
						win16_llseek(FH, -1, 1);
					EndLine = TRUE;
					break;
				case 0x0a: EndLine = TRUE; break;
				default:
					if (i<MaxStrLen-1)
					{
						Str[i] = b;
						i++;
					}
			}
		}
	} while ((c>=1) && (! EndLine));

	if (EndFile)
		SetResult(1);
	else
		SetResult(0);

	Str[i] = 0;
	SetStrVal(VarId, Str);
	return Err;
}


// Format: fileread <file handle> <read byte> <strvar>
// �w�肵���o�C�g�������t�@�C������ǂݍ��ށB
// �������A<read byte>�� 1�`255 �܂ŁB
// (2006.11.1 yutaka)
static WORD TTLFileRead(void)
{
	WORD Err;
	TVarId VarId;
	int fhi;
	HANDLE FH;
	int i, c;
	int ReadByte;   // �ǂݍ��ރo�C�g��
	TStrVal Str;
	BOOL EndFile;
	BYTE b;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	GetIntVal(&ReadByte,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (ReadByte < 1 || ReadByte > MaxStrLen-1))  // �͈̓`�F�b�N
		Err = ErrSyntax;
	if (Err!=0) return Err;

	EndFile = FALSE;
	for (i = 0 ; i < ReadByte ; i++) {
		c = win16_lread(FH,&b,1);
		if (c <= 0) {  // EOF
			EndFile = TRUE;
			break;
		}
		if (i<MaxStrLen-1)
		{
			Str[i] = b;
		}
	}

	if (EndFile)
		SetResult(1);
	else
		SetResult(0);

	Str[i] = 0;
	SetStrVal(VarId,Str);
	return Err;
}


static WORD TTLFileRename(void)
{
	WORD Err;
	TStrVal FName1, FName2;

	Err = 0;
	GetStrVal(FName1,&Err);
	GetStrVal(FName2,&Err);
	if ((Err==0) &&
	    ((strlen(FName1)==0) ||
	     (strlen(FName2)==0) ||
	     (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}
	if (_stricmp(FName1,FName2)==0) {
		SetResult(2);
		return Err;
	}

	if (!GetAbsPath(FName1,sizeof(FName1))) {
		SetResult(-1);
		return Err;
	}
	if (!GetAbsPath(FName2,sizeof(FName2))) {
		SetResult(-2);
		return Err;
	}
	if (MoveFileW(wc::fromUtf8(FName1), wc::fromUtf8(FName2)) == 0) {
		// ���l�[���Ɏ��s������A�G���[�ŕԂ��B
		SetResult(-3);
		return Err;
	}

	SetResult(0);
	return Err;
}

static WORD TTLFileSearch(void)
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(FName,sizeof(FName));
	DWORD attr = GetFileAttributesW(wc::fromUtf8(FName));
	if (attr != INVALID_FILE_ATTRIBUTES)
		// exists file or folder
		SetResult(1);
	else
		SetResult(0);
	return Err;
}

static WORD TTLFileSeek(void)
{
	WORD Err;
	int fhi;
	HANDLE FH;
	int i, j;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	GetIntVal(&i,&Err);
	GetIntVal(&j,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	win16_llseek(FH,i,j);
	return Err;
}

static WORD TTLFileSeekBack(void)
{
	WORD Err;
	int fhi;
	HANDLE FH;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	/* move back to the marked pos */
	win16_llseek(FH,FPointer[fhi],0);
	return Err;
}

/*
 * FILETIME -> time_t
 */
static time_t FileTimeToUnixTime(const FILETIME *ft)
{
	// FILETIME start 1601-01-01 00:00:00 , 100ns
	// Unix epoch     1970-01-01 00:00:00 , 1sec
	static const unsigned long long unix_epoch_offset = 116444736000000000LL;

	unsigned long long ll = ((unsigned long long)ft->dwHighDateTime << 32) + ft->dwLowDateTime;
	return (time_t)((ll - 116444736000000000) / 10000000);
}

static WORD TTLFileStat(void)
{
	WORD Err;
	TStrVal FName;
	int result = -1;
	HANDLE hFile;
	unsigned long long file_size;
	time_t st_mtime;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    (strlen(FName)==0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!GetAbsPath(FName,sizeof(FName))) {
		goto end;
	}

	hFile = CreateFileW(wc::fromUtf8(FName), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		goto end;
	}

	{
		DWORD file_size_hi;
		DWORD file_size_low;
		file_size_low = GetFileSize(hFile, &file_size_hi);
		if (file_size_low == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
			CloseHandle(hFile);
			goto end;
		}
		file_size = ((unsigned long long)file_size_hi << 32) + file_size_low;
	}

	{
		FILETIME last_write_time;
		if (GetFileTime(hFile, NULL, &last_write_time, NULL) == FALSE) {
			CloseHandle(hFile);
			goto end;
		}
		st_mtime = FileTimeToUnixTime(&last_write_time); // �ŏI�C������
	}
	CloseHandle(hFile);

	if (CheckParameterGiven()) {
		TVarId SizeVarId;
		GetIntVar(&SizeVarId,&Err);
		if (Err!=0) return Err;
		SetIntVal(SizeVarId, (int)file_size);
	}

	if (CheckParameterGiven()) {
		TVarId TimeVarId;
		GetStrVar(&TimeVarId,&Err);
		if (Err!=0) return Err;
		struct tm* tmp = localtime(&st_mtime);
		char TimeStr[128];
		strftime(TimeStr, sizeof(TimeStr), "%Y-%m-%d %H:%M:%S", tmp);
		SetStrVal(TimeVarId, TimeStr);
	}

	if (CheckParameterGiven()) {
		TVarId DrvVarId;
		GetStrVar(&DrvVarId,&Err);
		if (Err!=0) return Err;
		char DrvStr[2];
		char d = FName[0];
		DrvStr[0] =
			(d >= 'a' && d <= 'z') ? d - 'a' + 'A' :
			(d >= 'A' && d <= 'Z') ? d :
			'?';
		DrvStr[1] = 0;
		SetStrVal(DrvVarId, DrvStr);
	}

	result = 0;

end:
	SetResult(result);

	return Err;
}

static WORD TTLFileStrSeek(void)
{
	WORD Err;
	int fhi;
	HANDLE FH;
	int Len, i, c;
	TStrVal Str;
	BYTE b;
	long int pos;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	GetStrVal(Str,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	pos = win16_llseek(FH,0,1);
	if (pos == INVALID_SET_FILE_POINTER) return Err;

	Len = strlen(Str);
	i = 0;
	do {
		c = win16_lread(FH,&b,1);
		if (c==1)
		{
			if (b==(BYTE)Str[i])
				i++;
			else if (i>0) {
				i = 0;
				if (b==(BYTE)Str[0])
					i = 1;
			}
		}
	} while ((c>=1) && (i!=Len));
	if (i==Len)
		SetResult(1);
	else {
		SetResult(0);
		win16_llseek(FH,pos,0);
	}
	return Err;
}

static WORD TTLFileStrSeek2(void)
{
	WORD Err;
	int fhi;
	HANDLE FH;
	int Len, i, c;
	TStrVal Str;
	BYTE b;
	long int pos, pos2;
	BOOL Last;

	Err = 0;
	GetIntVal(&fhi,&Err);
	FH = HandleGet(fhi);
	GetStrVal(Str,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	pos = win16_llseek(FH,0,1);
	if (pos == INVALID_SET_FILE_POINTER) return Err;

	Len = strlen(Str);
	i = 0;
	pos2 = pos;
	do {
		Last = (pos2<=0);
		c = win16_lread(FH,&b,1);
		pos2 = win16_llseek(FH,-2,1);
		if (c==1)
		{
			if (b==(BYTE)Str[Len-1-i])
				i++;
			else if (i>0) {
				i = 0;
				if (b==(BYTE)Str[Len-1])
					i = 1;
			}
		}
	} while (!Last && (i!=Len));
	if (i==Len) {
		// �t�@�C����1�o�C�g�ڂ��q�b�g����ƁA�t�@�C���|�C���^���˂��j����
		// INVALID_SET_FILE_POINTER �ɂȂ�̂ŁA
		// �[���I�t�Z�b�g�ɂȂ�悤�ɒ�������B(2008.10.10 yutaka)
		if (pos2 == INVALID_SET_FILE_POINTER)
			win16_llseek(FH, 0, 0);
		SetResult(1);
	} else {
		SetResult(0);
		win16_llseek(FH,pos,0);
	}
	return Err;
}

static WORD TTLFileTruncate(void)
{
	WORD Err = 0;
	TStrVal FName;
	int result = -1;
	int TruncByte;
	BOOL r;
	HANDLE hFile;
	DWORD pos_low;

	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    (strlen(FName)==0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!GetAbsPath(FName,sizeof(FName))) {
		goto end;
	}

	if (CheckParameterGiven()) {
		GetIntVal(&TruncByte,&Err);
		if (Err!=0) return Err;
	} else {
		Err = ErrSyntax;
		goto end;
	}
	Err = 0;

	// �t�@�C���I�[�v���A���݂��Ȃ��ꍇ�͐V�K�쐬
	hFile = CreateFileW(wc::fromUtf8(FName), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		goto end;
	}

	// �t�@�C�����w�肵���T�C�Y�ɂ���A
	// �g�������ꍇ�A�g�������̓��e�͖���`
	pos_low = SetFilePointer(hFile, TruncByte, NULL, FILE_BEGIN );
	if (pos_low == INVALID_SET_FILE_POINTER) {
		goto end_close;
	}
	r = SetEndOfFile(hFile);
	if (r == FALSE) {
		goto end_close;
	}

	result = 0;
	Err = 0;

end_close:
	CloseHandle( hFile );
end:
	SetResult(result);
	return Err;
}

static WORD TTLFileWrite(BOOL addCRLF)
{
	WORD Err, P;
	int fhi;
	HANDLE FH;
	int Val;
	TStrVal Str;

	Err = 0;
	GetIntVal(&fhi, &Err);
	FH = HandleGet(fhi);
	if (Err) return Err;

	P = LinePtr;
	GetStrVal(Str, &Err);
	if (!Err) {
		if (GetFirstChar())
			return ErrSyntax;

		win16_lwrite(FH, Str, strlen(Str));
	}
	else if (Err == ErrTypeMismatch) {
		Err = 0;
		LinePtr = P;
		GetIntVal(&Val, &Err);
		if (Err) return Err;
		if (GetFirstChar())
			return ErrSyntax;

		Str[0] = Val & 0xff;
		win16_lwrite(FH, Str, 1);
	}
	else {
		return Err;
	}

	if (addCRLF) {
		win16_lwrite(FH,"\015\012",2);
	}
	return 0;
}

static WORD TTLFindClose(void)
{
	WORD Err;
	int DH;

	Err = 0;
	GetIntVal(&DH,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if ((DH>=0) && (DH<NumDirHandle) &&
	    (DirHandle[DH]!= INVALID_HANDLE_VALUE))
	{
		FindClose(DirHandle[DH]);
		DirHandle[DH] = INVALID_HANDLE_VALUE;
	}
	return Err;
}

static WORD TTLFindFirst(void)
{
	TVarId DH, Name;
	WORD Err;
	TStrVal Dir;
	int i;
	WIN32_FIND_DATAW data;

	Err = 0;
	GetIntVar(&DH,&Err);
	GetStrVal(Dir,&Err);
	GetStrVar(&Name,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (Dir[0]==0) strncpy_s(Dir, sizeof(Dir),"*.*", _TRUNCATE);
	GetAbsPath(Dir,sizeof(Dir));
	i = 0;
	while ((i<NumDirHandle) &&
	       (DirHandle[i]!= INVALID_HANDLE_VALUE))
		i++;
	if (i<NumDirHandle)
	{
		DirHandle[i] = FindFirstFileW(wc::fromUtf8(Dir),&data);
		if (DirHandle[i]!= INVALID_HANDLE_VALUE)
			SetStrVal(Name,(u8)data.cFileName);
		else
			i = -1;
	}
	else
		i = -1;

	SetIntVal(DH,i);
	if (i<0)
	{
		SetResult(0);
		SetStrVal(Name,"");
	}
	else
		SetResult(1);
	return Err;
}

static WORD TTLFindNext(void)
{
	TVarId Name;
	WORD Err;
	int DH;
	WIN32_FIND_DATAW data;

	Err = 0;
	GetIntVal(&DH,&Err);
	GetStrVar(&Name,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if ((DH>=0) && (DH<NumDirHandle) &&
	    (DirHandle[DH]!= INVALID_HANDLE_VALUE) &&
	    (FindNextFileW(DirHandle[DH],&data) != FALSE))
	{
		SetStrVal(Name,(u8)data.cFileName);
		SetResult(1);
	}
	else {
		SetStrVal(Name,"");
		SetResult(0);
	}
	return Err;
}

static WORD TTLFlushRecv(void)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	FlushRecv();
	return 0;
}

static WORD TTLFolderCreate(void)
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetResult(-1);
		return Err;
	}

	if (CreateDirectoryW(wc::fromUtf8(FName), NULL) == 0) {
		SetResult(2);
	}
	else {
		SetResult(0);
	}
	return Err;
}

static WORD TTLFolderDelete(void)
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetResult(-1);
		return Err;
	}

	if (RemoveDirectoryW(wc::fromUtf8(FName)) == 0) {
		SetResult(2);
	}
	else {
		SetResult(0);
	}
	return Err;
}

static WORD TTLFolderSearch(void)
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(FName,sizeof(FName));
	DWORD attr = GetFileAttributesW(wc::fromUtf8(FName));
	if ((attr != INVALID_FILE_ATTRIBUTES) &&
		(attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		SetResult(1);
	}
	else {
		SetResult(0);
	}
	return Err;
}

static WORD TTLFor(void)
{
	WORD Err;
	TVarId VarId;
	int ValStart, ValEnd, i;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetIntVal(&ValStart,&Err);
	GetIntVal(&ValEnd,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (! CheckNext())
	{ // first time
		Err = SetForLoop();
		if (Err==0)
		{
			SetIntVal(VarId,ValStart);
			i = CopyIntVal(VarId);
			if (i==ValEnd)
				LastForLoop();
		}
	}
	else { // return from 'next'
		i = CopyIntVal(VarId);
		if (i<ValEnd)
			i++;
		else if (i>ValEnd)
			i--;
		SetIntVal(VarId,i);
		if (i==ValEnd)
			LastForLoop();
	}
	return Err;
}

static WORD TTLGetDir(void)
{
	WORD Err;
	TVarId VarId;
	TStrVal Str;

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	TTMGetDir(Str, sizeof(Str));
	SetStrVal(VarId,Str);
	return Err;
}

static WORD TTLGetEnv(void)
{
	WORD Err;
	TVarId VarId;
	TStrVal Str;
	PCHAR Str2;

	Err = 0;
	GetStrVal(Str,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Str2 = getenv(Str);

	if (Str2!=NULL)
		SetStrVal(VarId,Str2);
	else
		SetStrVal(VarId,"");
	return Err;
}

static WORD TTLGetFileAttr(void)
{
	WORD Err;
	TStrVal Filename;

	Err = 0;
	GetStrVal(Filename,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(Filename, sizeof(Filename));
	SetResult(GetFileAttributesW(wc::fromUtf8(Filename)));

	return Err;
}

static WORD TTLGetHostname(void)
{
	WORD Err;
	TVarId VarId;
	char Str[MaxStrLen];

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	Err = GetTTParam(CmdGetHostname,Str,sizeof(Str));
	if (Err==0)
		SetStrVal(VarId,Str);
	return Err;
}

#define MAX_IPADDR 30
/*
 strdim ipaddr 10
 getipv4addr ipaddr num
 */
static WORD TTLGetIPv4Addr(void)
{
	WORD Err;
	TVarId VarId, VarId2, id;
	WSADATA ws;
	INTERFACE_INFO info[MAX_IPADDR];
	SOCKET sock;
	DWORD socknum;
	int num, result, arysize;
	int i, n;
	IN_ADDR addr;

	Err = 0;
	GetStrAryVar(&VarId,&Err);
	GetIntVar(&VarId2, &Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// �������g�̑SIPv4�A�h���X���擾����B
	if (WSAStartup(MAKEWORD(2,2), &ws) != 0) {
		SetResult(-1);
		SetIntVal(VarId2, 0);
		return Err;
	}
	arysize = GetStrAryVarSize(VarId);
	num = 0;
	result = 1;
	sock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
	if (WSAIoctl(sock, SIO_GET_INTERFACE_LIST, NULL, 0, info, sizeof(info), &socknum, NULL, NULL) != SOCKET_ERROR) {
		n = socknum / sizeof(info[0]);
		for (i = 0 ; i < n ; i++) {
			if ((info[i].iiFlags & IFF_UP) == 0)
				continue;
			if ((info[i].iiFlags & IFF_LOOPBACK) != 0)
				continue;
			addr = info[i].iiAddress.AddressIn.sin_addr;

			if (num < arysize) {
				id = GetStrVarFromArray(VarId, num, &Err);
				SetStrVal(id, inet_ntoa(addr));
			}
			else {
				result = 0;
			}
			num++;
		}
	}
	closesocket(sock);
	WSACleanup();

	SetResult(result);
	SetIntVal(VarId2, num);

	return Err;
}


// IPv6�A�h���X�𕶎���ɕϊ�����B
static void myInetNtop(int Family, char *pAddr, char *pStringBuf, size_t StringBufSize)
{
	int i;
	char s[16];
	unsigned int val;

	pStringBuf[0] = '\0';
	for (i = 0 ; i < 16 ; i++) {
		val = (pAddr[i]) & 0xFF;
		_snprintf_s(s, sizeof(s), _TRUNCATE, "%02x", val);
		strncat_s(pStringBuf, StringBufSize, s, _TRUNCATE);
		if (i != 15 && (i & 1))
			strncat_s(pStringBuf, StringBufSize, ":", _TRUNCATE);
	}
}


static WORD TTLGetIPv6Addr(void)
{
	WORD Err;
	TVarId VarId, VarId2, id;
	int num, result, arysize;
	DWORD ret;
	IP_ADAPTER_ADDRESSES addr[256];/* XXX */
	ULONG len = sizeof(addr);
	char ipv6str[64];
	ULONG  (WINAPI *pGetAdaptersAddresses)(ULONG Family, ULONG Flags, PVOID Reserved, PIP_ADAPTER_ADDRESSES AdapterAddresses, PULONG SizePointer);

	Err = 0;
	GetStrAryVar(&VarId,&Err);
	GetIntVar(&VarId2, &Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	ret = DLLGetApiAddress(L"iphlpapi.dll", DLL_LOAD_LIBRARY_SYSTEM, "GetAdaptersAddresses", (void **)&pGetAdaptersAddresses);
	if (ret != NO_ERROR) {
		// GetAdaptersAddresses ���T�|�[�g����Ă��Ȃ� OS �͂����� return
		//   2000 �ȍ~�� IPv6 �ɑΉ����Ă��邪 GetAdaptersAddresses �����݂��Ȃ�
		//   XP �ȍ~�� �T�|�[�g����Ă���
		SetResult(-1);
		SetIntVal(VarId2, 0);
		return Err;
	}

	// �������g�̑SIPv6�A�h���X���擾����B
	arysize = GetStrAryVarSize(VarId);
	num = 0;
	result = 1;
	ret = pGetAdaptersAddresses(AF_INET6, 0, NULL, addr, &len);
	if (ret == ERROR_SUCCESS) {
		IP_ADAPTER_ADDRESSES *padap = &addr[0];

		do {
			IP_ADAPTER_UNICAST_ADDRESS *uni = padap->FirstUnicastAddress;

			if (!uni) {
				continue;
			}
			do {
				SOCKET_ADDRESS addr = uni->Address;
				struct sockaddr_in6 *sa;

				if (!(uni->Flags & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE)) {
					continue;
				}
				sa = (struct sockaddr_in6*)addr.lpSockaddr;
				myInetNtop(AF_INET6, (char*)&sa->sin6_addr, ipv6str, sizeof(ipv6str));

				if (num < arysize) {
					id = GetStrVarFromArray(VarId, num, &Err);
					SetStrVal(id, ipv6str);
				}
				else {
					result = 0;
				}
				num++;

			} while ((uni = uni->Next));
		} while ((padap = padap->Next));
	}

	SetResult(result);
	SetIntVal(VarId2, num);

	return Err;
}

// setpassword 'password.dat' 'mypassword' passowrd
static WORD TTLSetPassword(void)
{
	TStrVal FileNameStr, KeyStr, PassStr;
	char Temp[512];
	WORD Err;
	int result = 0;  /* failure */

	Err = 0;
	GetStrVal(FileNameStr, &Err);   // �t�@�C����
	GetStrVal(KeyStr, &Err);  // �L�[��
	GetStrVal(PassStr, &Err);  // �p�X���[�h
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// �����񂪋�̏ꍇ�̓G���[�Ƃ���B
	if (FileNameStr[0]==0 ||
	    KeyStr[0]==0 ||
	    PassStr[0]==0)   // "getpassword"���l�A��p�X���[�h�������Ȃ��B
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(FileNameStr, sizeof(FileNameStr));

	// �p�X���[�h���Í�������B
	Encrypt(PassStr, Temp);

	if (WritePrivateProfileString("Password", KeyStr, Temp, FileNameStr) != 0)
		result = 1;  /* success */

	SetResult(result);  // �����ۂ�ݒ肷��B
	return Err;
}

// setpassword2 <filename> <password name> <strval> <encryptstr>
static WORD TTLSetPassword2(void)
{
	TStrVal FileNameStr, KeyStr, PassStr, EncryptStr;
	WORD Err = 0;

	GetStrVal(FileNameStr, &Err);	// �t�@�C����
	GetStrVal(KeyStr, &Err);		// �L�[��
	GetStrVal(PassStr, &Err);		// �p�X���[�h
	GetStrVal(EncryptStr, &Err);	// �p�X���[�h��������Í������邽�߂̃p�X���[�h�i���ʌ��j
	if ((Err == 0) && (GetFirstChar() != 0)) {
		Err = ErrSyntax;
	}
	if (Err != 0) {
		return Err;
	}
	if (FileNameStr[0] == 0 ||
		KeyStr[0] == 0 ||
		PassStr[0] == 0 ||
		EncryptStr[0] == 0) {
		return ErrSyntax;
	}

	GetAbsPath(FileNameStr, sizeof(FileNameStr));
	if (Encrypt2SetPassword(wc::fromUtf8(FileNameStr), KeyStr, PassStr, EncryptStr) == 0) {
		SetResult(0);	// ���s
		return 0;
	}
	SetResult(1);		// ����
	return 0;
}

// ispassword 'password.dat' 'username' ; result: 0=false; 1=true
static WORD TTLIsPassword(void)
{
	TStrVal FileNameStr, KeyStr;
	char Temp[512];
	WORD Err;
	int result = 0;

	Err = 0;
	GetStrVal(FileNameStr, &Err);   // �t�@�C����
	GetStrVal(KeyStr, &Err);  // �L�[��
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// �����񂪋�̏ꍇ�̓G���[�Ƃ���B
	if (FileNameStr[0]==0 ||
	    KeyStr[0]==0)
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(FileNameStr, sizeof(FileNameStr));

	Temp[0] = 0;
	GetPrivateProfileString("Password", KeyStr,"",
	                        Temp, sizeof(Temp), FileNameStr);
	if (Temp[0] == 0) { // password not exist
		result = 0;
	} else {
		result = 1;
	}

	SetResult(result);  // �����ۂ�ݒ肷��B
	return Err;
}

// ispassword2 <filename> <password name>
static WORD TTLIsPassword2(void)
{
	TStrVal FileNameStr, KeyStr;
	WORD Err = 0;
	int result = 0;

	GetStrVal(FileNameStr, &Err);	// �t�@�C����
	GetStrVal(KeyStr, &Err);		// �L�[��
	if ((Err == 0) && (GetFirstChar() != 0)) {
		Err = ErrSyntax;
	}
	if (Err != 0) {
		return Err;
	}
	if (FileNameStr[0] == 0 ||
		KeyStr[0] == 0) {
		return ErrSyntax;
	}

	GetAbsPath(FileNameStr, sizeof(FileNameStr));
	if (Encrypt2IsPassword(wc::fromUtf8(FileNameStr), KeyStr) == 0) {
		result = 0;		// �p�X���[�h����
	} else {
		result = 1;		// �p�X���[�h�L��
	}
	SetResult(result);	// �����ۂ�ݒ肷��B
	return 0;
}

static WORD TTLGetSpecialFolder(void)
{
	WORD Err;
	TVarId VarId;
	TStrVal type;
	int result;

	Err = 0;
	GetStrVar(&VarId,&Err);
	if (Err!=0) return Err;

	GetStrVal(type,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) {
		return Err;
	}

	char folder[MaxStrLen];
	result = GetSpecialFolder(folder, sizeof(folder), type);
	SetStrVal(VarId, folder);
	SetResult(result);

	return Err;
}

static WORD TTLGetTime(WORD mode)
{
	WORD Err;
	TVarId VarId;
	TStrVal Str1, Str2, tzStr;
	time_t time1;
	struct tm *ptm;
	const char *format;
	BOOL set_result;
	const char *tz = NULL;
	char tz_copy[128];

	// Save timezone
	tz = getenv("TZ");
	tz_copy[0] = 0;
	if (tz)
		strncpy_s(tz_copy, sizeof(tz_copy), tz, _TRUNCATE);

	Err = 0;
	GetStrVar(&VarId,&Err);

	if (CheckParameterGiven()) {
		GetStrVal(Str1, &Err);
		format = Str1;

		if (isInvalidStrftimeChar(Str1)) {
			SetResult(2);
			return 0;
//			return ErrSyntax;
		}
		set_result = TRUE;

		// �^�C���]�[���̎w�肪����΁Alocaltime()�ɉe��������B(2012.5.2 yutaka)
		if (CheckParameterGiven()) {
			GetStrVal(tzStr, &Err);
			if (Err!=0) return Err;
			_putenv_s("TZ", tzStr);
			_tzset();
		}

	}
	else {
		// mode = RsvGetDate or RsvGetTime
		switch (mode) {
		case RsvGetDate:
		default:
			format = "%Y-%m-%d";
			break;
		case RsvGetTime:
			format = "%H:%M:%S";
			break;
		}
		set_result = FALSE;
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// get current date & time
	time1 = time(NULL);
	ptm = localtime(&time1);

	if (strftime(Str2, sizeof(Str2), format, ptm)) {
		SetStrVal(VarId,Str2);
		if (set_result) SetResult(0);
	}
	else {
		if (set_result) SetResult(1);
	}

	// Restore timezone
	_putenv_s("TZ", tz_copy);
	_tzset();

	return Err;
}

static WORD TTLGetTitle(void)
{
	TVarId VarId;
	WORD Err;
	char Str[TitleBuffSize*2];

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	Err = GetTTParam(CmdGetTitle,Str,sizeof(Str));
	if (Err==0)
		SetStrVal(VarId,Str);
	return Err;
}

static WORD TTLGetTTDir(void)
{
	TVarId VarId;
	WORD Err;
	char Temp[MAX_PATH],HomeDir[MAX_PATH];

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (GetModuleFileName(NULL, Temp, sizeof(Temp)) == 0) {
		SetStrVal(VarId,"");
		SetResult(0);
		return Err;
	}
	ExtractDirName(Temp, HomeDir);
	SetStrVal(VarId,HomeDir);
	SetResult(1);

	return Err;
}

// COM�|�[�g���烌�W�X�^�l��ǂށB
// (2015.1.8 yutaka)
static WORD TTLGetModemStatus(void)
{
	TVarId VarId;
	WORD Err;
	char Str[MaxStrLen];
	int Num;

	Err = 0;
	GetIntVar(&VarId, &Err);
	if ((Err == 0) && (GetFirstChar() != 0))
		Err = ErrSyntax;
	if ((Err == 0) && (!Linked))
		Err = ErrLinkFirst;
	if (Err != 0) return Err;

	memset(Str, 0, sizeof(Str));
	Err = GetTTParam(CmdGetModemStatus, Str, sizeof(Str));
	if (Err == 0) {
		Num = atoi(Str);
		SetIntVal(VarId, Num);
		SetResult(0);
	}
	else {
		SetResult(1);
	}

	return Err;
}

static WORD TTLGetTTPos(void)
{
	WORD Err;
	TVarId showflag;
	TVarId w_x, w_y, w_width, w_height;
	TVarId c_x, c_y, c_width, c_height;
	char Str[MaxStrLen];

	Err = 0;
	GetIntVar(&showflag, &Err); // 0:�ʏ��ԁA1:�ŏ�����ԁA2:�ő剻��ԁA3:������
	GetIntVar(&w_x,      &Err); // w_x, w_y = �E�C���h�E�̈�̍����
	GetIntVar(&w_y,      &Err);
	GetIntVar(&w_width,  &Err);
	GetIntVar(&w_height, &Err);
	GetIntVar(&c_x,      &Err); // c_x, c_y = �N���C�A���g(�e�L�X�g)�̈�̍����
	GetIntVar(&c_y,      &Err);
	GetIntVar(&c_width,  &Err);
	GetIntVar(&c_height, &Err);

	if ((Err == 0) && (GetFirstChar() != 0)) {
		Err = ErrSyntax;
    }
	if ((Err == 0) && (! Linked)) {
		Err = ErrLinkFirst;
	}
	if (Err != 0) {
		return Err;
	}

	Err = GetTTParam(CmdGetTTPos, Str, sizeof(Str));
	if (Err == 0) {
		int tmp_showflag;
		int tmpw_x, tmpw_y, tmpw_width, tmpw_height;
		int tmpc_x, tmpc_y, tmpc_width, tmpc_height;
		HMONITOR hMonitor;
		RECT rc;
		UINT dpi_x, dpi_y;
		float mag = 1;

		if (sscanf_s(Str, "%d %d %d %d %d %d %d %d %d", &tmp_showflag,
					&tmpw_x, &tmpw_y, &tmpw_width, &tmpw_height,
					&tmpc_x, &tmpc_y, &tmpc_width, &tmpc_height) == 9) {
			if (DPIAware == DPI_AWARENESS_CONTEXT_UNAWARE) {
				if (pMonitorFromRect != NULL && pGetDpiForMonitor != NULL) {
					rc.left   = tmpw_x;
					rc.top    = tmpw_y;
					rc.right  = tmpw_x + tmpw_width;
					rc.bottom = tmpw_y + tmpw_height;
					hMonitor = pMonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
					if (hMonitor != NULL) {
						pGetDpiForMonitor(hMonitor, (MONITOR_DPI_TYPE)0 /*0=MDT_EFFECTIVE_DPI*/, &dpi_x, &dpi_y);
						mag = dpi_x / 96.f;
					}
				}
			}
			SetIntVal(showflag, tmp_showflag);
			SetIntVal(w_x,      (int)(tmpw_x      * mag));
			SetIntVal(w_y,      (int)(tmpw_y      * mag));
			SetIntVal(w_width,  (int)(tmpw_width  * mag));
			SetIntVal(w_height, (int)(tmpw_height * mag));
			SetIntVal(c_x,      (int)(tmpc_x      * mag));
			SetIntVal(c_y,      (int)(tmpc_y      * mag));
			SetIntVal(c_width,  (int)(tmpc_width  * mag));
			SetIntVal(c_height, (int)(tmpc_height * mag));
			SetResult(0);
		} else {
			SetResult(-1);
		}
	}

	return Err;
}

//
// Tera Term �̃o�[�W�����擾 & ��r
// �o�[�W�����ԍ��̓R���p�C�����Ɍ��肷��B
// (���݂͎��s�t�@�C���̃o�[�W�������͎Q�Ƃ��Ȃ�)
//
static WORD TTLGetVer(void)
{
	TVarId VarId;
	WORD Err;
	TStrVal Str1, Str2;
	int cur_major = TT_VERSION_MAJOR;
	int cur_minor = TT_VERSION_MINOR;
	int compare = 0;
	int comp_major, comp_minor, ret;
	int cur_ver, comp_ver;

	Err = 0;
	GetStrVar(&VarId, &Err);

	if (CheckParameterGiven()) {
		GetStrVal(Str1, &Err);

		ret = sscanf_s(Str1, "%d.%d", &comp_major, &comp_minor);
		if (ret != 2) {
			SetResult(-2);
			return 0;
			//Err = ErrSyntax;
		}
		compare = 1;

	} else {
		compare = 0;

	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	_snprintf_s(Str2, sizeof(Str2), _TRUNCATE, "%d.%d", cur_major, cur_minor);
	SetStrVal(VarId, Str2);

	if (compare == 1) {
		cur_ver = cur_major * 10000 + cur_minor;
		comp_ver = comp_major * 10000 + comp_minor;

		if (cur_ver < comp_ver) {
			SetResult(-1);
		} else if (cur_ver == comp_ver) {
			SetResult(0);
		} else {
			SetResult(1);
		}
	}

	return Err;
}

static WORD TTLGoto(void)
{
	TName LabName;
	WORD Err;
	TVariableType VarType;
	TVarId VarId;

	if (GetLabelName(LabName) && (GetFirstChar()==0))
	{
		if (CheckVar(LabName,&VarType,&VarId) && (VarType==TypLabel))
		{
			JumpToLabel(VarId);
			Err = 0;
		}
		else
			Err = ErrLabelReq;
	}
	else
		Err = ErrSyntax;

	return Err;
}

// add 'ifdefined' (2006.9.23 maya)
static WORD TTLIfDefined(void)
{
	WORD Err;
	TVariableType VarType;
	int Val;

	GetVarType(&VarType,&Val,&Err);

	SetResult(VarType);

	return Err;
}

static BOOL CheckThen(LPWORD Err)
{
	BYTE b;
	TName Temp;

	do {

		do {
			b = GetFirstChar();
			if (b==0) return FALSE;
		} while (((b<'A') || (b>'Z')) &&
		         (b!='_') &&
		         ((b<'a') || (b>'z')));
		LinePtr--;
		if (! GetIdentifier(Temp)) return FALSE;

	} while (_stricmp(Temp,"then")!=0);

	if (GetFirstChar()!=0)
		*Err = ErrSyntax;
	return TRUE;
}

static WORD TTLIf(void)
{
	WORD Err, Tmp, WId;
	TVariableType ValType;
	int Val;

	if (! GetExpression(&ValType,&Val,&Err))
		return ErrSyntax;

	if (Err!=0) return Err;

	if (ValType!=TypInteger)
		return ErrTypeMismatch;

	Tmp = LinePtr;
	if (GetReservedWord(&WId) &&
	    (WId==RsvThen))
	{  // If then  ... EndIf structure
		if (GetFirstChar()!=0)
			return ErrSyntax;
		IfNest++;
		if (Val==0)
			ElseFlag = 1; // Skip until 'Else' or 'EndIf'
	}
	else { // single line If command
		LinePtr = Tmp;
		if (!CheckParameterGiven())
			return ErrSyntax;
		if (Val==0)
			return 0;
		return ExecCmnd();
	}
	return Err;
}

static WORD TTLInclude(void)
{
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	if (!GetAbsPath(Str,sizeof(Str))) {
		Err = ErrCantOpen;
		return Err;
	}
	wchar_t *StrW = ToWcharU8(Str);
	BOOL r = BuffInclude(StrW);
	free(StrW);
	if (!r) {
		Err = ErrCantOpen;
		return Err;
	}

	return Err;
}

static WORD TTLInt2Str(void)
{
	TVarId VarId;
	WORD Err;
	int Num;
	TStrVal Str2;

	Err = 0;
	GetStrVar(&VarId,&Err);

	GetIntVal(&Num,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	_snprintf_s(Str2,sizeof(Str2),_TRUNCATE,"%d",Num);

	SetStrVal(VarId,Str2);
	return Err;
}

//
// logrotate size value
// logrotate rotate num
// logrotate halt
//
static WORD TTLLogRotate(void)
{
	WORD Err;
	char Str[MaxStrLen];
	char Str2[MaxStrLen];
	char buf[MaxStrLen*2];
	int size, num, len;

	Err = 0;
	GetStrVal(Str, &Err);
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	Err = ErrSyntax;
	if (strcmp(Str, "size") == 0) {   // ���[�e�[�g�T�C�Y
		if (CheckParameterGiven()) {
			Err = 0;
			size = 0;
			GetStrVal(Str2, &Err);
			if (Err == 0) {
				len = strlen(Str2);
				if (isdigit(Str2[len-1])) {
					num = 1;
				} else if (Str2[len-1] == 'K') {
					Str2[len-1] = 0;
					num = 1024;
				} else if (Str2[len-1] == 'M') {
					Str2[len-1] = 0;
					num = 1024*1024;
				} else {
					Err = ErrSyntax;
				}
				size = atoi(Str2) * num;
			}

			if (size < 128)
				Err = ErrSyntax;
			if (Err == 0)
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s %u", Str, size);
		}

	} else if (strcmp(Str, "rotate") == 0) {  // ���[�e�[�g�̐��㐔
		if (CheckParameterGiven()) {
			Err = 0;
			num = 0;
			GetIntVal(&num, &Err);
			if (num <= 0)
				Err = ErrSyntax;
			if (Err == 0)
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s %u", Str, num);
		}

	} else if (strcmp(Str, "halt") == 0) {
		Err = 0;
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s", Str);
	}
	if (Err!=0) return Err;

	SetFile(buf);
	Err = SendCmnd(CmdLogRotate, 0);

	return Err;
}

static WORD TTLLogInfo(void)
{
	WORD Err;
	TVarId VarId;
	char Str[MaxStrLen];

	Err = 0;
	GetStrVar(&VarId, &Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	Err = GetTTParam(CmdLogInfo, Str, sizeof(Str));
	if (Err==0) {
		SetResult(Str[0] - '0');
		SetStrVal(VarId, Str+1);
	}
	return Err;
}

static WORD TTLLogOpen(void)
{
	TStrVal Str;
	WORD Err;
	int LogFlags[LogOptMax+1] = { 0 };
	int TmpFlag;
	char ret[2];

	Err = 0;
	GetStrVal(Str, &Err);
	GetIntVal(&LogFlags[LogOptBinary], &Err);
	GetIntVal(&LogFlags[LogOptAppend], &Err);

	// plain text
	if (!CheckParameterGiven()) goto EndLogOptions;
	GetIntVal(&LogFlags[LogOptPlainText], &Err);
	if (Err!=0) return Err;

	// timestamp
	if (!CheckParameterGiven()) goto EndLogOptions;
	GetIntVal(&LogFlags[LogOptTimestamp], &Err);
	if (Err!=0) return Err;

	// hide file transfer dialog
	if (!CheckParameterGiven()) goto EndLogOptions;
	GetIntVal(&LogFlags[LogOptHideDialog], &Err);
	if (Err!=0) return Err;

	// Include screen buffer
	if (!CheckParameterGiven()) goto EndLogOptions;
	GetIntVal(&LogFlags[LogOptIncScrBuff], &Err);
	if (Err!=0) return Err;

	// Timestamp Type
	if (!CheckParameterGiven()) goto EndLogOptions;
	GetIntVal(&TmpFlag, &Err);
	if (Err!=0) return Err;
	if (TmpFlag < 0 || TmpFlag > 3)
		return ErrSyntax;
	LogFlags[LogOptTimestampType] = TmpFlag;

EndLogOptions:
	if (strlen(Str) == 0 || GetFirstChar() != 0)
		return ErrSyntax;

	SetFile(Str);
	SetLogOption(LogFlags);

	memset(ret, 0, sizeof(ret));
	Err = GetTTParam(CmdLogOpen,ret,sizeof(ret));
	if (Err==0) {
		if (ret[0] == 0x31) {
			SetResult(0);
		}
		else {
			SetResult(1);
		}
	}
	return Err;
}

static WORD TTLLoop(void)
{
	WORD WId, Err;
	int Val = 1;

	Err = 0;
	if (CheckParameterGiven()) {
		if (GetReservedWord(&WId)) {
			switch (WId) {
				case RsvWhile:
					GetIntVal(&Val,&Err);
					break;
				case RsvUntil:
					GetIntVal(&Val,&Err);
					Val = Val == 0;
					break;
				default:
					Err = ErrSyntax;
			}
			if ((Err==0) && (GetFirstChar()!=0))
				Err = ErrSyntax;
		}
		else {
			Err = ErrSyntax;
		}
	}

	if (Err!=0) return Err;

	return BackToWhile(Val!=0);
}

static WORD TTLMakePath(void)
{
	TVarId VarId;
	WORD Err;
	TStrVal Dir, Name;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Dir,&Err);
	GetStrVal(Name,&Err);
	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	AppendSlash(Dir,sizeof(Dir));
	strncat_s(Dir,sizeof(Dir),Name,_TRUNCATE);
	SetStrVal(VarId,Dir);

	return Err;
}

static char *ExtractFileNameU8(const char *PathName)
{
	wchar_t *PathNameW = ToWcharU8(PathName);
	wchar_t *filenameW = ExtractFileNameW(PathNameW);
	free(PathNameW);
	char *filenameU8 = ToU8W(filenameW);
	free(filenameW);
	return filenameU8;
}

static WORD TTLBasename(void)
{
	TVarId VarId;
	WORD Err;
	TStrVal Src;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Src,&Err);
	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	char *basename = ExtractFileNameU8(Src);
	SetStrVal(VarId, basename);
	free(basename);

	return Err;
}

static char *ExtractDirNameU8(const char *PathName)
{
	wchar_t *PathNameW = ToWcharU8(PathName);
	wchar_t *PathNameWD = DeleteSlashW(PathNameW);
	free(PathNameW);
	wchar_t *dirnameW = ExtractDirNameW(PathNameWD);
	free(PathNameWD);
	wchar_t *dirnameWD = DeleteSlashW(dirnameW);
	free(dirnameW);
	char *dirnameU8 = ToU8W(dirnameWD);
	free(dirnameWD);
	return dirnameU8;
}

static WORD TTLDirname(void)
{
	TVarId VarId;
	WORD Err;
	TStrVal Src;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Src,&Err);
	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	char *dir = ExtractDirNameU8(Src);
	SetStrVal(VarId, dir);
	free(dir);

	return Err;
}

static WORD TTLNext(void)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	return NextLoop();
}

static WORD TTLPause(void)
{
	int TimeOut;
	WORD Err;

	Err = 0;
	GetIntVal(&TimeOut,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (TimeOut>0)
	{
		TTLStatus = IdTTLPause;
		TimeLimit = (DWORD)(TimeOut*1000);
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
	}
	return Err;
}

// add 'mpause' command
// SYNOPSIS: mpause millisecoand
// DESCRIPTION: This command would sleep Tera Term macro program by specified millisecond time.
// (2006.2.10 yutaka)
static WORD TTLMilliPause(void)
{
	int TimeOut;
	WORD Err;

	Err = 0;
	GetIntVal(&TimeOut,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (TimeOut > 0) {
		Sleep(TimeOut);
	}

	return Err;
}


// add 'random' command
// SYNOPSIS: random <intvar> <value>
// DESCRIPTION:
// This command generates the random value from 0 to <value> and
// stores the value to <intvar>.
// (2006.2.11 yutaka)
static WORD TTLRandom(void)
{
	static int init_done = 0;
	static sfmt_t sfmt;
	TVarId VarId;
	WORD Err;
	int MaxNum;
	unsigned int Num, RandMin;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetIntVal(&MaxNum,&Err);

	if ( ((Err==0) && (GetFirstChar()!=0)) || MaxNum <= 0)
		Err = ErrSyntax;
	if (Err!=0) return Err;

	MaxNum++;

	/* 2**32 % x == (2**32 - x) % x */
	RandMin = (unsigned int) -MaxNum % MaxNum;

	if (!init_done) {
		init_done = 1;
		sfmt_init_gen_rand(&sfmt, (unsigned int)time(NULL));
	}

	do {
		Num = sfmt_genrand_uint32(&sfmt);
	} while (Num < RandMin);

	SetIntVal(VarId, (int)(Num % MaxNum));

	return Err;
}

static WORD TTLRecvLn(void)
{
	TStrVal Str;
	TVariableType ValType;
	TVarId VarId;
	int TimeOut;

	if (GetFirstChar()!=0)
		return ErrSyntax;
	if (! Linked)
		return ErrLinkFirst;

	ClearWait();

	Str[0] = 0x0a;
	Str[1] = 0;
	SetWait(1,Str);
	SetInputStr("");
	SetResult(1);
	TTLStatus = IdTTLWaitNL;
	TimeOut = 0;
	if (CheckVar("timeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut = CopyIntVal(VarId) * 1000;
	}
	if (CheckVar("mtimeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut += CopyIntVal(VarId);
	}

	if (TimeOut>0)
	{
		TimeLimit = (DWORD)TimeOut;
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
	}

	return 0;
}

static WORD TTLRegexOption(void)
{
	TStrVal Str;
	WORD Err;
	int opt_none_flag = 0;
	OnigOptionType new_opt = ONIG_OPTION_NONE;
	OnigEncoding new_enc = ONIG_ENCODING_UNDEF;
	OnigSyntaxType *new_syntax = NULL;

	Err = 0;

	while (CheckParameterGiven()) {
		GetStrVal(Str, &Err);
		if (Err)
			return Err;

		// Encoding
		if (_stricmp(Str, "ENCODING_ASCII")==0 || _stricmp(Str, "ASCII")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ASCII;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_1")==0 || _stricmp(Str, "ISO_8859_1")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_1;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_2")==0 || _stricmp(Str, "ISO_8859_2")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_2;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_3")==0 || _stricmp(Str, "ISO_8859_3")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_3;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_4")==0 || _stricmp(Str, "ISO_8859_4")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_4;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_5")==0 || _stricmp(Str, "ISO_8859_5")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_5;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_6")==0 || _stricmp(Str, "ISO_8859_6")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_6;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_7")==0 || _stricmp(Str, "ISO_8859_7")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_7;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_8")==0 || _stricmp(Str, "ISO_8859_8")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_8;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_9")==0 || _stricmp(Str, "ISO_8859_9")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_9;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_10")==0 || _stricmp(Str, "ISO_8859_10")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_10;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_11")==0 || _stricmp(Str, "ISO_8859_11")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_11;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_13")==0 || _stricmp(Str, "ISO_8859_13")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_13;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_14")==0 || _stricmp(Str, "ISO_8859_14")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_14;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_15")==0 || _stricmp(Str, "ISO_8859_15")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_15;
		}
		else if (_stricmp(Str, "ENCODING_ISO_8859_16")==0 || _stricmp(Str, "ISO_8859_16")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_ISO_8859_16;
		}
		else if (_stricmp(Str, "ENCODING_UTF8")==0 || _stricmp(Str, "UTF8")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_UTF8;
		}
		else if (_stricmp(Str, "ENCODING_UTF16_BE")==0 || _stricmp(Str, "UTF16_BE")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_UTF16_BE;
		}
		else if (_stricmp(Str, "ENCODING_UTF16_LE")==0 || _stricmp(Str, "UTF16_LE")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_UTF16_LE;
		}
		else if (_stricmp(Str, "ENCODING_UTF32_BE")==0 || _stricmp(Str, "UTF32_BE")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_UTF32_BE;
		}
		else if (_stricmp(Str, "ENCODING_UTF32_LE")==0 || _stricmp(Str, "UTF32_LE")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_UTF32_LE;
		}
		else if (_stricmp(Str, "ENCODING_EUC_JP")==0 || _stricmp(Str, "EUC_JP")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_EUC_JP;
		}
		else if (_stricmp(Str, "ENCODING_EUC_TW")==0 || _stricmp(Str, "EUC_TW")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_EUC_TW;
		}
		else if (_stricmp(Str, "ENCODING_EUC_KR")==0 || _stricmp(Str, "EUC_KR")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_EUC_KR;
		}
		else if (_stricmp(Str, "ENCODING_EUC_CN")==0 || _stricmp(Str, "EUC_CN")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_EUC_CN;
		}
		else if (_stricmp(Str, "ENCODING_SJIS")==0 || _stricmp(Str, "SJIS")==0
		      || _stricmp(Str, "ENCODING_CP932")==0 || _stricmp(Str, "CP932")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_SJIS;
		}
		else if (_stricmp(Str, "ENCODING_KOI8_R")==0 || _stricmp(Str, "KOI8_R")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_KOI8_R;
		}
		else if (_stricmp(Str, "ENCODING_CP1251")==0 || _stricmp(Str, "CP1251")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_CP1251;
		}
		else if (_stricmp(Str, "ENCODING_BIG5")==0 || _stricmp(Str, "BIG5")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_BIG5;
		}
		else if (_stricmp(Str, "ENCODING_GB18030")==0 || _stricmp(Str, "GB18030")==0) {
			if (new_enc != ONIG_ENCODING_UNDEF)
				return ErrSyntax;
			new_enc = ONIG_ENCODING_GB18030;
		}

		// Syntax
		else if (_stricmp(Str, "SYNTAX_DEFAULT")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_DEFAULT;
		}
		else if (_stricmp(Str, "SYNTAX_ASIS")==0 || _stricmp(Str, "ASIS")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_ASIS;
		}
		else if (_stricmp(Str, "SYNTAX_POSIX_BASIC")==0 || _stricmp(Str, "POSIX_BASIC")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_POSIX_BASIC;
		}
		else if (_stricmp(Str, "SYNTAX_POSIX_EXTENDED")==0 || _stricmp(Str, "POSIX_EXTENDED")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_POSIX_EXTENDED;
		}
		else if (_stricmp(Str, "SYNTAX_EMACS")==0 || _stricmp(Str, "EMACS")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_EMACS;
		}
		else if (_stricmp(Str, "SYNTAX_GREP")==0 || _stricmp(Str, "GREP")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_GREP;
		}
		else if (_stricmp(Str, "SYNTAX_GNU_REGEX")==0 || _stricmp(Str, "GNU_REGEX")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_GNU_REGEX;
		}
		else if (_stricmp(Str, "SYNTAX_JAVA")==0 || _stricmp(Str, "JAVA")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_JAVA;
		}
		else if (_stricmp(Str, "SYNTAX_PERL")==0 || _stricmp(Str, "PERL")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_PERL;
		}
		else if (_stricmp(Str, "SYNTAX_PERL_NG")==0 || _stricmp(Str, "PERL_NG")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_PERL_NG;
		}
		else if (_stricmp(Str, "SYNTAX_RUBY")==0 || _stricmp(Str, "RUBY")==0) {
			if (new_syntax != NULL)
				return ErrSyntax;
			new_syntax = ONIG_SYNTAX_RUBY;
		}

		// Option
		else if (_stricmp(Str, "OPTION_NONE")==0) {
			if (new_opt != ONIG_OPTION_NONE || opt_none_flag != 0)
				return ErrSyntax;
			new_opt = ONIG_OPTION_NONE;
			opt_none_flag = 1;
		}
		else if (_stricmp(Str, "OPTION_SINGLELINE")==0 || _stricmp(Str, "SINGLELINE")==0) {
			new_opt |= ONIG_OPTION_SINGLELINE;
		}
		else if (_stricmp(Str, "OPTION_MULTILINE")==0 || _stricmp(Str, "MULTILINE")==0) {
			new_opt |= ONIG_OPTION_MULTILINE;
		}
		else if (_stricmp(Str, "OPTION_IGNORECASE")==0 || _stricmp(Str, "IGNORECASE")==0) {
			new_opt |= ONIG_OPTION_IGNORECASE;
		}
		else if (_stricmp(Str, "OPTION_EXTEND")==0 || _stricmp(Str, "EXTEND")==0) {
			new_opt |= ONIG_OPTION_EXTEND;
		}
		else if (_stricmp(Str, "OPTION_FIND_LONGEST")==0 || _stricmp(Str, "FIND_LONGEST")==0) {
			new_opt |= ONIG_OPTION_FIND_LONGEST;
		}
		else if (_stricmp(Str, "OPTION_FIND_NOT_EMPTY")==0 || _stricmp(Str, "FIND_NOT_EMPTY")==0) {
			new_opt |= ONIG_OPTION_FIND_NOT_EMPTY;
		}
		else if (_stricmp(Str, "OPTION_NEGATE_SINGLELINE")==0 || _stricmp(Str, "NEGATE_SINGLELINE")==0) {
			new_opt |= ONIG_OPTION_NEGATE_SINGLELINE;
		}
		else if (_stricmp(Str, "OPTION_DONT_CAPTURE_GROUP")==0 || _stricmp(Str, "DONT_CAPTURE_GROUP")==0) {
			new_opt |= ONIG_OPTION_DONT_CAPTURE_GROUP;
		}
		else if (_stricmp(Str, "OPTION_CAPTURE_GROUP")==0 || _stricmp(Str, "CAPTURE_GROUP")==0) {
			new_opt |= ONIG_OPTION_CAPTURE_GROUP;
		}

		else {
			return ErrSyntax;
		}
	}


	if (new_enc != ONIG_ENCODING_UNDEF) {
		RegexEnc = new_enc;
	}
	if (new_syntax != NULL) {
		RegexSyntax = new_syntax;
	}
	if (new_opt != ONIG_OPTION_NONE || opt_none_flag != 0) {
		RegexOpt = new_opt;
	}

	return 0;
}

static WORD TTLReturn(void)
{
	if (GetFirstChar()==0)
		return ReturnFromSub();
	else
		return ErrSyntax;
}

// add 'rotateleft' and 'rotateright' (2007.8.19 maya)
#define ROTATE_DIR_LEFT 0
#define ROTATE_DIR_RIGHT 1
static WORD BitRotate(int direction)
{
	TVarId VarId;
	WORD Err;
	int x, n;

	Err = 0;
	GetIntVar(&VarId, &Err);
	GetIntVal(&x, &Err);
	GetIntVal(&n, &Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (direction == ROTATE_DIR_RIGHT)
		n = -n;

	n %= INT_BIT;

	if (n < 0)
		n += INT_BIT;

	if (n == 0) {
		SetIntVal(VarId, x);
	} else {
		SetIntVal(VarId, (x << n) | ((unsigned int)x >> (INT_BIT-n)));
	}
	return Err;
}

static WORD TTLRotateLeft(void)
{
	return BitRotate(ROTATE_DIR_LEFT);
}

static WORD TTLRotateRight(void)
{
	return BitRotate(ROTATE_DIR_RIGHT);
}

/**
 *	�����̕������ DDEOut(), DDEOut1Byte() ����
 */
static WORD GetParamStrings(void)
{
	TStrVal Str;
	WORD Err;
	TVariableType ValType;
	int Val;
	BOOL EndOfLine;

	EndOfLine = FALSE;
	do {
		if (GetString(Str,&Err))
		{
			if (Err!=0) return Err;
			DDEOut(Str);
		}
		else if (GetExpression(&ValType,&Val,&Err))
		{
			if (Err!=0) return Err;
			switch (ValType) {
				case TypInteger: DDEOut1Byte(LOBYTE(Val)); break;
				case TypString: DDEOut(StrVarPtr((TVarId)Val)); break;
				default:
					return ErrTypeMismatch;
			}
		}
		else
			EndOfLine = TRUE;
	} while (! EndOfLine);
	return 0;
}

static WORD TTLSend(void)
{
	if (! Linked)
		return ErrLinkFirst;

	WORD r = GetParamStrings();
	if (r != 0) {
		return r;
	}
	DDESend();
	return 0;
}

static WORD TTLSendText(void)
{
	if (! Linked)
		return ErrLinkFirst;

	WORD r = GetParamStrings();
	if (r != 0) {
		return r;
	}
	DDESendStringU8(NULL);
	return 0;
}

static WORD TTLSendBinary(void)
{
	if (! Linked)
		return ErrLinkFirst;

	WORD r = GetParamStrings();
	if (r != 0) {
		return r;
	}
	DDESendBinary(NULL, 0);
	return 0;
}

/*
 * src �Ɋ܂܂�� 0x01 �� 0x01 0x02 �ɒu�������� dst �ɃR�s�[����B
 * TStrVal �ɂ� 0x00 ���܂܂�鎖������(�I�[�Ƌ�ʂł��Ȃ�)�̂� 0x00 �͍l������K�v�Ȃ��B
 */
static void AddBroadcastString(char *dst, int dstlen, const char *src)
{
	int i;

	i = strlen(dst);
	dst += i;
	dstlen -= i;

	while (*src != 0 && dstlen > 1) {
		if (*src == 0x01) {
			// 0x01 ���i�[����ɂ� 0x01 0x02 ��2�o�C�g + NUL �I�[�p��1�o�C�g���K�v
			if (dstlen < 3) {
				break;
			}
			*dst++ = *src++;
			*dst++ = 0x02;
			dstlen -= 2;
		}
		else {
			*dst++ = *src++;
			dstlen--;
		}
	}

	*dst = 0;
}

/*
 * TTLSendBroadcast / TTLSendMulticast �̉�����
 *
 * �e�p�����[�^��A������������� buff �Ɋi�[���ĕԂ��B
 * crlf �� TRUE �̎��͊e�p�����[�^�̊Ԃ� "\n" �����ށB(�v����)
 *
 * �p�����[�^�� String �̏ꍇ�͂��̂܂܁AInteger �̏ꍇ�� ASCII �R�[�h�Ƃ݂Ȃ��Ă��̕����𑗂�B
 * Tera Term ���ł� send ���Ƌ��ʂ̃��[�`�����g����ׁADDE �ʐM�ׂ̈̃G���R�[�h���s���K�v�L��B
 *   0x00 -> 0x01 0x01
 *   0x01 -> 0x01 0x02
 */
static WORD GetBroadcastString(char *buff, int bufflen, BOOL crlf)
{
	TStrVal Str;
	WORD Err;
	TVariableType ValType;
	int Val;
	char tmp[3];

	buff[0] = '\0';

	while (1) {
		if (GetString(Str, &Err)) {
			if (Err!=0) return Err;
			AddBroadcastString(buff, bufflen, Str);
		}
		else if (GetExpression(&ValType, &Val, &Err)) {
			if (Err!=0) return Err;
			switch (ValType) {
				case TypInteger:
					Val = LOBYTE(Val);
					if (Val == 0 || Val == 1) {
						tmp[0] = 1;
						tmp[1] = Val + 1;
						tmp[2] = 0;
					}
					else {
						tmp[0] = Val;
						tmp[1] = 0;
					}
					strncat_s(buff, bufflen, tmp, _TRUNCATE);
					break;
				case TypString:
					AddBroadcastString(buff, bufflen, StrVarPtr((TVarId)Val));
					break;
				default:
					return ErrTypeMismatch;
			}
		}
		else {
			break;
		}
	}
	if (crlf) {
		strncat_s(buff, bufflen, "\r\n", _TRUNCATE);
	}
	return 0;
}

// sendbroadcast / sendlnbroadcast �̓���痘�p (crlf�̒l�œ����ς���)
static WORD TTLSendBroadcast(BOOL crlf)
{
	TStrVal buf;
	WORD Err;

	if (! Linked)
		return ErrLinkFirst;

	if ((Err = GetBroadcastString(buf, MaxStrLen, crlf)) != 0)
		return Err;

	SetFile(buf);
	return SendCmnd(CmdSendBroadcast, 0);
}

static WORD TTLSetMulticastName(void)
{
	TStrVal Str;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);
	if (Err!=0) return Err;

	SetFile(Str);
	return SendCmnd(CmdSetMulticastName, 0);
}

// sendmulticast / sendlnmulticast �̓���痘�p (crlf�̒l�œ����ς���)
static WORD TTLSendMulticast(BOOL crlf)
{
	TStrVal buf, Str;
	WORD Err;

	if (! Linked)
		return ErrLinkFirst;

	// �}���`�L���X�g���ʗp�̖��O���擾����B
	Err = 0;
	GetStrVal(Str,&Err);
	if (Err!=0) return Err;
	SetFile(Str);

	if ((Err = GetBroadcastString(buf, MaxStrLen, crlf)) != 0)
		return Err;

	SetSecondFile(buf);
	return SendCmnd(CmdSendMulticast, 0);
}

static WORD TTLSendFile(void)
{
	TStrVal Str;
	WORD Err;
	int BinFlag;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&BinFlag,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	SetFile(Str);
	SetBinary(BinFlag);
	return SendCmnd(CmdSendFile,IdTTLWaitCmndEnd);
}

static WORD TTLSendKCode(void)
{
	TStrVal Str;
	WORD Err;
	int KCode, Count;

	Err = 0;
	GetIntVal(&KCode,&Err);
	GetIntVal(&Count,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Word2HexStr((WORD)KCode,Str);
	Word2HexStr((WORD)Count,&Str[4]);
	SetFile(Str);
	return SendCmnd(CmdSendKCode,0);
}

/**
 *	TTLSend() �̉��s�t����
 */
static WORD TTLSendLn(void)
{
	if (! Linked)
		return ErrLinkFirst;

	WORD r = GetParamStrings();
	if (r != 0) {
		return r;
	}
	DDEOut1Byte(0x0d);
	DDESend();
	return 0;
}

static WORD TTLSetDate(void)
{
	WORD Err;
	TStrVal Str;
	int y, m, d;
	SYSTEMTIME Time;

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Str[4] = 0;
	if (sscanf(Str,"%u",&y)!=1) return 0;
	Str[7] = 0;
	if (sscanf(&(Str[5]),"%u",&m)!=1) return 0;
	Str[10] = 0;
	if (sscanf(&(Str[8]),"%u",&d)!=1) return 0;
	GetLocalTime(&Time);
	Time.wYear = y;
	Time.wMonth = m;
	Time.wDay = d;
	SetLocalTime(&Time);
	return Err;
}

static WORD TTLSetDir(void)
{
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	TTMSetDir(Str);

	return Err;
}

static WORD TTLSetDlgPos(void)
{
	WORD Err = 0;

	if (CheckParameterGiven()) {
		// �p�����[�^������΁Ax  y ��2�̃p�����[�^������
		int x, y;
		GetIntVal(&x,&Err);
		GetIntVal(&y,&Err);
		// �ǉ��p�����[�^(�ȗ��\)
		int position = 0, offset_x = 0, offset_y = 0;
		if (CheckParameterGiven()) {
			GetIntVal(&position,&Err);
			if (position < 1 || position > 10) {
				Err = ErrSyntax;
			}
			if (CheckParameterGiven()) {
				GetIntVal(&offset_x,&Err);
				GetIntVal(&offset_y,&Err);
			}
		}
		if ((Err==0) && (GetFirstChar()!=0))
			Err = ErrSyntax;
		if (Err!=0) return Err;
		SetDlgPos(x, y, position, offset_x, offset_y);
	}
	else {
		// �p�����[�^���Ȃ���΃f�t�H���g�ʒu�ɖ߂�
		SetDlgPos(CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0);
	}
	return Err;
}

// reactivate 'setenv' (2007.8.31 maya)
static WORD TTLSetEnv(void)
{
	WORD Err;
	TStrVal Str, Str2;

	Err = 0;
	GetStrVal(Str,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	_putenv_s(Str,Str2);
	return Err;
}

static WORD TTLSetExitCode(void)
{
	WORD Err;
	int Val;

	Err = 0;
	GetIntVal(&Val,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	ExitCode = Val;
	return Err;
}

static WORD TTLSetFileAttr(void)
{
	WORD Err;
	TStrVal Filename;
	int attributes;

	Err = 0;
	GetStrVal(Filename,&Err);
	GetIntVal(&attributes,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!GetAbsPath(Filename,sizeof(Filename))) {
		// ���s
		SetResult(0);
		return Err;
	}

	if (SetFileAttributesW(wc::fromUtf8(Filename), attributes) == 0) {
		// ���s
		SetResult(0);
	}
	else {
		// ����
		SetResult(1);
	}

	return Err;
}

static WORD TTLSetSync(void)
{
	WORD Err;
	int Val;

	Err = 0;
	GetIntVal(&Val,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	if (Val==0)
		SetSync(FALSE);
	else
		SetSync(TRUE);

	return 0;
}

static WORD TTLSetTime(void)
{
	WORD Err;
	TStrVal Str;
	int h, m, s;
	SYSTEMTIME Time;

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Str[2] = 0;
	if (sscanf(Str,"%u",&h)!=1) return 0;
	Str[5] = 0;
	if (sscanf(&(Str[3]),"%u",&m)!=1) return 0;
	Str[8] = 0;
	if (sscanf(&(Str[6]),"%u",&s)!=1) return 0;

	GetLocalTime(&Time);
	Time.wHour = h;
	Time.wMinute = m;
	Time.wSecond = s;
	SetLocalTime(&Time);

	return Err;
}

static WORD TTLShow(void)
{
	WORD Err;
	int Val;

	Err = 0;
	GetIntVal(&Val,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	if (Val==0)
		ShowWindow(HMainWin,SW_MINIMIZE);
	else if (Val>0) {
		ShowWindow(HMainWin,SW_RESTORE);
		PostMessage(HMainWin, WM_USER_MACROBRINGUP, 0, 0);
	}
	else
		ShowWindow(HMainWin,SW_HIDE);
	return Err;
}

void TTLShowMINIMIZE(void)
{
	ShowWindow(HMainWin,SW_MINIMIZE);
}

// 'sprintf': Format a string in the style of sprintf
//
// (2007.5.1 yutaka)
// (2007.5.3 maya)
static WORD TTLSprintf(int getvar)
{
	TStrVal Fmt;
	int Num, NumWidth, NumPrecision;
	TStrVal Str;
	WORD Err = 0, TmpErr;
	TVarId VarId;
	char buf[MaxStrLen];
	char *p, subFmt[MaxStrLen], buf2[MaxStrLen];
	int width_asterisk, precision_asterisk, reg_beg, reg_end, reg_len, i;
	char *match_str;

	enum arg_type {
		INTEGER,
		DOUBLE,
		STRING,
		NONE
	};
	enum arg_type type;

	int r;
	unsigned char *start, *range, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	UChar* pattern, * str;

	if (getvar) {
		GetStrVar(&VarId, &Err);
		if (Err!=0) {
			SetResult(4);
			return Err;
		}
	}

//	pattern = (UChar* )"^%[-+0 #]*(?:[1-9][0-9]*)?(?:\\.[0-9]*)?$";
	pattern = (UChar* )"^%[-+0 #]*([1-9][0-9]*|\\*)?(?:\\.([0-9]*|\\*))?$";
	//               flags--------
	//                       width------------------
	//                                     precision--------------------

	r = onig_new(&reg, pattern, pattern + strlen((char *)pattern),
	             ONIG_OPTION_NONE, ONIG_ENCODING_ASCII, ONIG_SYNTAX_DEFAULT,
	             &einfo);
	if (r != ONIG_NORMAL) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str((OnigUChar *)s, r, &einfo);
		fprintf(stderr, "ERROR: %s\n", s);
		SetResult(-1);
		goto exit2;
	}

	region = onig_region_new();

	GetStrVal(Fmt, &Err);
	if (Err!=0) {
		SetResult(1);
		goto exit2;
	}

	p = Fmt;
	memset(buf, 0, sizeof(buf));
	memset(subFmt, 0, sizeof(subFmt));
	while(*p != '\0') {
		if (strlen(subFmt)>0) {
			type = NONE;
			switch (*p) {
				case '%':
					if (strlen(subFmt) == 1) { // "%%" -> "%"
						strncat_s(buf, sizeof(buf), "%", _TRUNCATE);
						memset(subFmt, 0, sizeof(subFmt));
					}
					else {
						// ���O�܂ł����̂܂� buf �Ɋi�[
						strncat_s(buf, sizeof(buf), subFmt, _TRUNCATE);
						// �d�؂蒼��
						memset(subFmt, 0, sizeof(subFmt));
						strncat_s(subFmt, sizeof(subFmt), p, 1);
					}
					break;

				case 'c':
				case 'd':
				case 'i':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
					type = INTEGER;
					/* Falls through. */

				case 'e':
				case 'E':
				case 'f':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
					if (type == NONE) {
						type = DOUBLE;
					}
					/* Falls through. */

				case 's':
					if (type == NONE) {
						type = STRING;
					}

					// "%" �� *p �̊Ԃ����������`�F�b�N
					str = (UChar* )subFmt;
					end   = str + strlen(subFmt);
					start = str;
					range = end;
					r = onig_search(reg, str, end, start, range, region,
					                ONIG_OPTION_NONE);
					if (r != 0) {
						SetResult(2);
						Err = ErrSyntax;
						goto exit1;
					}

					strncat_s(subFmt, sizeof(subFmt), p, 1);

					// width, precision �� * ���ǂ����`�F�b�N
					width_asterisk = precision_asterisk = 0;
					if (region->num_regs != 3) {
						SetResult(-1);
						goto exit2;
					}
					reg_beg = region->beg[1];
					reg_end = region->end[1];
					reg_len = reg_end - reg_beg;
					match_str = (char*)calloc(reg_len + 1, sizeof(char));
					for (i = 0; i < reg_len; i++) {
						match_str[i] = str[reg_beg + i];
					}
					if (strcmp(match_str, "*") == 0) {
						width_asterisk = 1;
					}
					free(match_str);
					reg_beg = region->beg[2];
					reg_end = region->end[2];
					reg_len = reg_end - reg_beg;
					match_str = (char*)calloc(reg_len + 1, sizeof(char));
					for (i = 0; i < reg_len; i++) {
						match_str[i] = str[reg_beg + i];
					}
					if (strcmp(match_str, "*") == 0) {
						precision_asterisk = 1;
					}
					free(match_str);

					// * �ɑΉ�����������擾
					if (width_asterisk) {
						TmpErr = 0;
						GetIntVal(&NumWidth, &TmpErr);
						if (TmpErr != 0) {
							SetResult(3);
							Err = TmpErr;
							goto exit1;
						}
					}
					if (precision_asterisk) {
						TmpErr = 0;
						GetIntVal(&NumPrecision, &TmpErr);
						if (TmpErr != 0) {
							SetResult(3);
							Err = TmpErr;
							goto exit1;
						}
					}

					if (type == STRING || type == DOUBLE) {
						// ������Ƃ��ēǂ߂邩�g���C
						TmpErr = 0;
						GetStrVal(Str, &TmpErr);
						if (TmpErr == 0) {
							if (type == STRING) {
								if (!width_asterisk && !precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, Str);
								}
								else if (width_asterisk && !precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, Str);
								}
								else if (!width_asterisk && precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumPrecision, Str);
								}
								else { // width_asterisk && precision_asterisk
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, NumPrecision, Str);
								}
							}
							else { // DOUBLE
								if (!width_asterisk && !precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, atof(Str));
								}
								else if (width_asterisk && !precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, atof(Str));
								}
								else if (!width_asterisk && precision_asterisk) {
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumPrecision, atof(Str));
								}
								else { // width_asterisk && precision_asterisk
									_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, NumPrecision, atof(Str));
								}
							}
						}
						else {
							SetResult(3);
							Err = TmpErr;
							goto exit1;
						}
					}
					else {
						// ���l�Ƃ��ēǂ߂邩�g���C
						TmpErr = 0;
						GetIntVal(&Num, &TmpErr);
						if (TmpErr == 0) {
							if (!width_asterisk && !precision_asterisk) {
								_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, Num);
							}
							else if (width_asterisk && !precision_asterisk) {
								_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, Num);
							}
							else if (!width_asterisk && precision_asterisk) {
								_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumPrecision, Num);
							}
							else { // width_asterisk && precision_asterisk
								_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, NumWidth, NumPrecision, Num);
							}
						}
						else {
							SetResult(3);
							Err = TmpErr;
							goto exit1;
						}
					}

					strncat_s(buf, sizeof(buf), buf2, _TRUNCATE);
					memset(subFmt, 0, sizeof(subFmt));
					onig_region_free(region, 0);
					break;

				default:
					strncat_s(subFmt, sizeof(subFmt), p, 1);
			}
		}
		else if (*p == '%') {
			strncat_s(subFmt, sizeof(subFmt), p, 1);
		}
		else if (strlen(buf) < sizeof(buf)-1) {
			strncat_s(buf, sizeof(buf), p, 1);
		}
		else {
			break;
		}
		p++;
	}
	if (strlen(subFmt) > 0) {
		strncat_s(buf, sizeof(buf), subFmt, _TRUNCATE);
	}

	if (getvar) {
		SetStrVal(VarId, buf);
	}
	else {
		// �}�b�`�����s�� inputstr �֊i�[����
		SetInputStr(buf);  // �����Ńo�b�t�@���N���A�����
	}
	SetResult(0);

exit1:
	onig_region_free(region, 1);
exit2:
	onig_free(reg);
	onig_end();

	return Err;
}

static WORD TTLStr2Code(void)
{
	TVarId VarId;
	WORD Err;
	TStrVal Str;
	int Len, c, i;
	unsigned int Num;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	Len = strlen(Str);
	if (Len > sizeof(Num))
		c = sizeof(Num);
	else
		c = Len;
	Num = 0;
	for (i=c; i>=1; i--)
		Num = Num*256 + (BYTE)Str[Len-i];
	SetIntVal(VarId,Num);

	return Err;
}

static WORD TTLStr2Int(void)
{
	TVarId VarId;
	WORD Err;
	TStrVal Str;
	int Num;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	// C����ł�16�i��0x�Ŏn�܂邪�ATTL�d�l�ł� $ �Ŏn�܂邽�߁A��҂��T�|�[�g����B
	if (Str[0] == '$') {
		memmove_s(Str + 2, sizeof(Str) - 2, Str + 1, strlen(Str));
		Str[0] = '0';
		Str[1] = 'x';
	}

	// '%d'����'%i'�֕ύX�ɂ��A10�i�ȊO�̐��l��ϊ��ł���悤�ɂ���B (2007.5.1 yutaka)
	// ���ʌ݊����̂���10�i��16�i�݂̂̃T�|�[�g�Ƃ���B(2007.5.2 yutaka)
	// 10 : decimal
	// 0x10, $10: hex
	if (Str[0] == '0' && tolower(Str[1]) == 'x') {
		if (sscanf(Str,"%i",&Num)!=1)
		{
			Num = 0;
			SetResult(0);
		}
		else {
			SetResult(1);
		}
	} else {
		if (sscanf(Str,"%d",&Num)!=1)
		{
			Num = 0;
			SetResult(0);
		}
		else {
			SetResult(1);
		}
	}
	SetIntVal(VarId,Num);

	return Err;
}

static WORD TTLStrCompare(void)
{
	TStrVal Str1, Str2;
	WORD Err;
	int i;

	Err = 0;
	GetStrVal(Str1,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	i = strcmp(Str1,Str2);
	if (i<0)
		i = -1;
	else if (i>0)
		i = 1;
	SetResult(i);
	return Err;
}

static WORD TTLStrConcat(void)
{
	TVarId VarId;
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	char dest[MaxStrLen];
	const char *src = StrVarPtr(VarId);
	strcpy_s(dest, MaxStrLen, src);
	strncat_s(dest, MaxStrLen, Str, _TRUNCATE);
	SetStrVal(VarId, dest);
	return Err;
}

static WORD TTLStrCopy(void)
{
	WORD Err;
	TVarId VarId;
	int From, Len, SrcLen;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&From,&Err);
	GetIntVal(&Len,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (From<1) From = 1;
	SrcLen = strlen(Str)-From+1;
	if (Len > SrcLen) Len = SrcLen;
	if (Len < 0) Len = 0;
	char dest[MaxStrLen];
	memcpy(dest,&(Str[From-1]),Len);
	dest[Len] = 0;
	SetStrVal(VarId, dest);
	return Err;
}

static WORD TTLStrLen(void)
{
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	SetResult(strlen(Str));
	return Err;
}

/*
  ����: strmatch <������> <���K�\��>
  <������>��<���K�\��>���}�b�`���邩���ׂ�R�}���h(awk��match�֐�����)�B
  result�ɂ́A�}�b�`�����ʒu���Z�b�g(�}�b�`���Ȃ��ꍇ��0)�B
  �}�b�`�����ꍇ�́Awaitregex�Ɠ��l��matchstr,groupmatchstr1-9���Z�b�g�B
 */
static WORD TTLStrMatch(void)
{
	WORD Err;
	TStrVal Str1, Str2;
	int ret, result;

	Err = 0;
	GetStrVal(Str1,&Err);   // target string
	GetStrVal(Str2,&Err);   // regex pattern
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	ret = FindRegexStringOne(Str2, strlen(Str2), Str1, strlen(Str1));
	if (ret > 0) { // matched
		result = ret;
	} else {
		result = 0;
	}

	// FindRegexStringOne�̒���UnlockVar()����Ă��܂��̂ŁALockVar()���Ȃ����B
	LockVar();

	SetResult(result);

	return Err;
}

static WORD TTLStrScan(void)
{
	WORD Err;
	TStrVal Str1, Str2;

	Err = 0;
	GetStrVal(Str1,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if ((Str1[0] == 0) || (Str2[0] == 0)) {
		SetResult(0);
		return Err;
	}

	char *p = strstr(Str1, Str2);
	if (p != NULL) {
		SetResult(p - Str1 + 1);
	}
	else {
		SetResult(0);
	}
	return Err;
}

static void insert_string(char *str, int index, char *addstr)
{
	char *np;
	int srclen;
	int addlen;

	srclen = strlen(str);
	addlen = strlen(addstr);

	// �܂��͑}�������ӏ��ȍ~�̃f�[�^���A���Ɉړ�����B
	np = str + (index - 1);
	memmove_s(np + addlen, MaxStrLen, np, srclen - (index - 1));

	// �������}������
	memcpy(np, addstr, addlen);

	// null-terminate
	str[srclen + addlen] = '\0';
}

static WORD TTLStrInsert(void)
{
	WORD Err;
	TVarId VarId;
	int Index;
	TStrVal Str;
	int srclen, addlen;
	char *srcptr;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetIntVal(&Index,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	char dest[MaxStrLen];
	strcpy_s(dest, sizeof(dest), StrVarPtr(VarId));
	srcptr = dest;
	srclen = strlen(srcptr);
	if (Index <= 0 || Index > srclen+1) {
		Err = ErrSyntax;
	}
	addlen = strlen(Str);
	if (srclen + addlen + 1 > MaxStrLen) {
		Err = ErrSyntax;
	}
	if (Err!=0) return Err;

	insert_string(srcptr, Index, Str);
	SetStrVal(VarId, dest);

	return Err;
}

// ������ str �� index �����ځi1�I���W���j���� len �����폜����
static void remove_string(char *str, int index, int len)
{
	char *np;
	int srclen, copylen;

	srclen = strlen(str);

	if (len <=0 || index <= 0 || (index-1 + len) > srclen) {
		return;
	}

	/*
	   remove_string(str, 6, 4);

	   <------------>srclen
			 <-->len
	   XXXXXX****YYY
	        ^index(np)
			     ^np+len
				 <-->srclen - len - index
		    ��
	   XXXXXXYYY
	 */

	np = str + (index - 1);
	copylen = srclen - len - (index - 1);
	if (copylen > 0)
		memmove_s(np, MaxStrLen, np + len, copylen);

	// null-terminate
	str[srclen - len] = '\0';
}

static WORD TTLStrRemove(void)
{
	WORD Err;
	TVarId VarId;
	int Index, Len;
	int srclen;
	char *srcptr;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetIntVal(&Index,&Err);
	GetIntVal(&Len,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	char dest[MaxStrLen];
	strcpy_s(dest, sizeof(dest), StrVarPtr(VarId));
	srcptr = dest;
	srclen = strlen(srcptr);
	if (Len <=0 || Index <= 0 || (Index-1 + Len) > srclen) {
		Err = ErrSyntax;
	}
	if (Err!=0) return Err;

	remove_string(srcptr, Index, Len);
	SetStrVal(VarId, dest);

	return Err;
}

static WORD TTLStrReplace(void)
{
	WORD Err;
	TVariableType VarType;
	TVarId DestVarId;
	TStrVal oldstr;
	TStrVal newstr;
	TStrVal tmpstr;
	char *p;
	int srclen, oldlen, matchlen;
	int pos, ret;
	int result = 0;

	Err = 0;
	GetStrVar(&DestVarId,&Err);
	GetIntVal(&pos,&Err);
	GetStrVal(oldstr,&Err);
	GetStrVal(newstr,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	const char *srcptr = StrVarPtr(DestVarId);
	srclen = strlen(srcptr);

	if (pos > srclen || pos <= 0) {
		result = 0;
		goto error;
	}
	pos--;

	strncpy_s(tmpstr, MaxStrLen, srcptr, _TRUNCATE);

	oldlen = strlen(oldstr);

	// strptr������� pos �����ڈȍ~�ɂ����āAoldstr ��T���B
	p = tmpstr + pos;
	ret = FindRegexStringOne(oldstr, oldlen, p, strlen(p));
	// FindRegexStringOne�̒���UnlockVar()����Ă��܂��̂ŁALockVar()���Ȃ����B
	LockVar();
	if (ret == 0) {
		// ������Ȃ������ꍇ�́A"0"�Ŗ߂�B
		result = 0;
		goto error;
	}
	else if (ret < 0) {
		// �������Ȃ����K�\�����ŃG���[�̏ꍇ�� -1 ��Ԃ�
		result = -1;
		goto error;
	}
	ret--;

	TVarId MatchVarId;
	if (CheckVar("matchstr",&VarType,&MatchVarId) &&
		(VarType==TypString)) {
		const char *matchptr = StrVarPtr(MatchVarId);
		matchlen = strlen(matchptr);
	} else {
		result = 0;
		goto error;
	}

	char dest[MaxStrLen];
	strncpy_s(dest, sizeof(dest), tmpstr, pos + ret);
	strncat_s(dest, sizeof(dest), newstr, _TRUNCATE);
	strncat_s(dest, sizeof(dest), tmpstr + pos + ret + matchlen, _TRUNCATE);
	SetStrVal(DestVarId, dest);

	result = 1;

error:
	SetResult(result);
	return Err;
}

static WORD TTLStrSpecial(void)
{
	WORD Err;
	TVarId VarId;
	TStrVal srcstr;

	Err = 0;
	GetStrVar(&VarId,&Err);
	if (Err!=0) return Err;

	if (CheckParameterGiven()) { // strspecial strvar strval
		GetStrVal(srcstr,&Err);
		if ((Err==0) && (GetFirstChar()!=0))
			Err = ErrSyntax;
		if (Err!=0) {
			return Err;
		}

		RestoreNewLine(srcstr);
		SetStrVal(VarId, srcstr);
	}
	else { // strspecial strvar
		char dest[MaxStrLen];
		strcpy_s(dest, sizeof(dest), StrVarPtr(VarId));
		RestoreNewLine(dest);
		SetStrVal(VarId, dest);
	}

	return Err;
}

static WORD TTLStrTrim(void)
{
	TStrVal trimchars;
	WORD Err;
	TVarId VarId;
	int srclen;
	int i, start, end;
	char *srcptr, *p;
	char table[256];

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(trimchars,&Err);

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	char dest[MaxStrLen];
	strcpy_s(dest, sizeof(dest), StrVarPtr(VarId));
	srcptr = dest;
	srclen = strlen(srcptr);

	// �폜���镶���̃e�[�u�������B
	memset(table, 0, sizeof(table));
	for (p = trimchars; *p ; p++) {
		table[*p] = 1;
	}

	// ������̐擪���猟������
	for (i = 0 ; i < srclen ; i++) {
		if (table[srcptr[i]] == 0)
			break;
	}
	// �폜����Ȃ��L���ȕ�����̎n�܂�B
	// ���ׂč폜�ΏۂƂȂ�ꍇ�́Astart == srclen �B
	start = i;

	// ������̖������猟������
	for (i = srclen - 1 ; i >= 0 ; i--) {
		if (table[srcptr[i]] == 0)
			break;
	}
	// �폜����Ȃ��L���ȕ�����̏I���B
	// ���ׂč폜�ΏۂƂȂ�ꍇ�́Aend == -1 �B
	end = i;

	// ���������
	srcptr[end + 1] = '\0';

	// ���ɁA�擪������B
	remove_string(srcptr, 1, start);

	SetStrVal(VarId, dest);
	return Err;
}

static WORD TTLStrSplit(void)
{
#define MAXVARNUM 9
	TStrVal src, delimchars, buf;
	WORD Err;
	int maxvar;
	int srclen, len;
	int i, count;
	BOOL ary = FALSE, omit = FALSE;
	char *p;
	char /* *last, */ *tok[MAXVARNUM];

	Err = 0;
	GetStrVal(src,&Err);
	GetStrVal(delimchars,&Err);
	// 3rd arg (optional)
	if (CheckParameterGiven()) {
		GetIntVal(&maxvar,&Err);
		if (Err!=0) {
			// TODO array
#if 0
			Err = 0;
			// Parameter ���� array ���󂯎��
			if (Err==0) {
				ary = TRUE;
			}
#endif
		}
	}
	else {
		maxvar = 9;
		omit = TRUE;
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!ary && (maxvar < 1 || maxvar > MAXVARNUM) )
		return ErrSyntax;

	// �f���~�^��1�����݂̂Ƃ���B
	len = strlen(delimchars);
	if (len != 1)
		return ErrSyntax;

	srclen = strlen(src);
	strcpy_s(buf, MaxStrLen, src);  /* �j�󂳂�Ă������悤�ɁA�R�s�[�o�b�t�@���g���B*/

#if 0
	// �g�[�N���̐؂�o�����s���B
	memset(tok, 0, sizeof(tok));
#if 0
	tok[0] = strtok_s(srcptr, delimchars, &last);
	for (i = 1 ; i < MAXVARNUM ; i++) {
		tok[i] = strtok_s(NULL, delimchars, &last);
		if (tok[i] == NULL)
			break;
	}
#else
	/* strtok���g���ƁA�A��������؂肪1�Ɋۂ߂��邽�߁A���O�Ń|�C���^��
	 * ���ǂ�B�������A��؂蕶����1�݂̂Ƃ���B
	 */
	i = 0;
	for (p = buf; *p == delimchars[0] ; p++) {
		tok[i++] = NULL;
		if (i >= maxvar)
			goto end;
	}

	for (p = strtok_s(p, delimchars, &last); p != NULL ; p = strtok_s(NULL, delimchars, &last) ) {
		tok[i++] = p;
		if (i >= maxvar)
			goto end;
		for (p += strlen(p) + 1 ; *p == delimchars[0] ; p++) {
			tok[i++] = NULL;
			if (i >= maxvar)
				goto end;
		}
	}
#endif
#else
	if (ary) {
		// TODO array
	}
	else {
		p = buf;
		count = 1;
		tok[count-1] = p;
		for (i=0; i < srclen && count < maxvar + omit; i++) { // count �ȗ����ɂ́A���ߕ����̂Ă邽�� 1 �]���ɐi�߂�
			if (*p == *delimchars) {
				*p = '\0';
				count++;
				if (count <= MAXVARNUM) { // tok �̗v�f���𒴂��đ�����Ȃ��悤�ɂ���(count �ȗ����̂���)
					tok[count-1] = p+1;
				}
			}
			p++;
		}
	}
#endif

//end:
	// ���ʂ̊i�[
	for (i = 1 ; i <= count ; i++) {
		SetGroupMatchStr(i, tok[i-1]);
	}
	for (i = count+1 ; i <= MAXVARNUM ; i++) {
		SetGroupMatchStr(i, "");
	}
	SetResult(count);
	return Err;
#undef MAXVARNUM
}

static WORD TTLStrJoin(void)
{
#define MAXVARNUM 9
	TStrVal delimchars, buf;
	WORD Err;
	TVarId TargetVarId;
	TVariableType VarType;
	int maxvar;
	int i;
	BOOL ary = FALSE;
	char *srcptr;
	const char *p;

	Err = 0;
	GetStrVar(&TargetVarId,&Err);
	GetStrVal(delimchars,&Err);
	// 3rd arg (optional)
	if (CheckParameterGiven()) {
		GetIntVal(&maxvar,&Err);
		if (Err!=0) {
			// TODO array
#if 0
			Err = 0;
			// Parameter ���� array ���󂯎��
			if (Err==0) {
				ary = TRUE;
			}
#endif
		}
	}
	else {
		maxvar = 9;
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!ary && (maxvar < 1 || maxvar > MAXVARNUM) )
		return ErrSyntax;

	char dest[MaxStrLen];
	strcpy_s(dest, sizeof(dest), StrVarPtr(TargetVarId));
	srcptr = dest;

	srcptr[0] = '\0';
	if (ary) {
		// TODO array
	}
	else {
		for (i = 0 ; i < maxvar ; i++) {
			TVarId VarId;
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "groupmatchstr%d", i + 1);
			if (CheckVar(buf,&VarType,&VarId)) {
				if (VarType!=TypString)
					return ErrSyntax;
				p = StrVarPtr(VarId);
				strncat_s(srcptr, MaxStrLen, p, _TRUNCATE);
				if (i < maxvar-1) {
					strncat_s(srcptr, MaxStrLen, delimchars, _TRUNCATE);
				}
			}
		}
	}
	SetStrVal(TargetVarId, dest);

	return Err;
#undef MAXVARNUM
}

static WORD TTLTestLink(void)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (! Linked)
		SetResult(0);
	else if (ComReady==0)
		SetResult(1);
	else
		SetResult(2);

	return 0;
}

// added (2007.7.12 maya)
static WORD TTLToLower(void)
{
	WORD Err;
	TVarId VarId;
	TStrVal Str;
	int i=0;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	while (Str[i] != 0) {
		if (Str[i] >= 'A' && Str[i] <= 'Z') {
			Str[i] = Str[i] + 0x20;
		}
		i++;
	}

	SetStrVal(VarId, Str);
	return Err;
}

// added (2007.7.12 maya)
static WORD TTLToUpper(void)
{
	WORD Err;
	TVarId VarId;
	TStrVal Str;
	int i=0;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	while (Str[i] != 0) {
		if (Str[i] >= 'a' && Str[i] <= 'z') {
			Str[i] = Str[i] - 0x20;
		}
		i++;
	}

	SetStrVal(VarId, Str);
	return Err;
}

static WORD TTLUnlink(void)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (Linked)
	{
		EndDDE();
	}
	return 0;
}


static WORD TTLUptime(void)
{
	WORD Err;
	TVarId VarId;
	DWORD tick;

	Err = 0;
	GetIntVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// Windows OS���N�����Ă���̌o�ߎ��ԁi�~���b�j���擾����B�������A49�����o�߂���ƁA0�ɖ߂�B
	// GetTickCount64() API(Vista�ȍ~)���g���ƁA�I�[�o�[�t���[���Ȃ��Ȃ邪�A��������Tera Term�ł�
	// 64bit�ϐ����T�|�[�g���Ă��Ȃ��̂ŁA�Ӗ����Ȃ��B
	tick = GetTickCount();

	SetIntVal(VarId, tick);

	return Err;
}



static WORD TTLWait(BOOL Ln)
{
	TStrVal Str;
	WORD Err;
	TVariableType ValType;
	TVarId VarId;
	int i, Val;
	int TimeOut;

	ClearWait();

	for (i=0; i<10; i++) {
		Err = 0;
		if (GetString(Str, &Err)) {
			SetWait(i+1, Str);
		}
		else if (GetExpression(&ValType, &Val, &Err) && Err == 0) {
			if (ValType == TypString)
				SetWait(i+1, StrVarPtr((TVarId)Val));
			else
				Err = ErrTypeMismatch;
		}
		else
			break;

		if (Err)
			break;
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (! Linked)
		Err = ErrLinkFirst;

	if ((Err==0) && (i>0))
	{
		if (Ln)
			TTLStatus = IdTTLWaitLn;
		else
			TTLStatus = IdTTLWait;
		TimeOut = 0;
		if (CheckVar("timeout",&ValType,&VarId) && (ValType==TypInteger)) {
			TimeOut = CopyIntVal(VarId) * 1000;
		}
		if (CheckVar("mtimeout",&ValType,&VarId) && (ValType==TypInteger)) {
			TimeOut += CopyIntVal(VarId);
		}

		if (TimeOut>0)
		{
			TimeLimit = (DWORD)TimeOut;
			TimeStart = GetTickCount();
			SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
		}
	}
	else
		ClearWait();

	return Err;
}


static WORD TTLWait4all(BOOL Ln)
{
	WORD Err = 0;

	Err = TTLWait(Ln);
	TTLStatus = IdTTLWait4all;

	Wait4allGotIndex = FALSE;
	Wait4allFoundNum = 0;

	return Err;
}


// 'waitregex'(wait regular expression): wait command with regular expression
//
// This command has almost same function of 'wait' command. Additionally 'waitregex' can search
// the keyword with regular expression. Tera Term uses a regex library that is called 'Oniguruma'.
// cf. http://www.geocities.jp/kosako3/oniguruma/
//
// (2005.10.5 yutaka)
static WORD TTLWaitRegex(BOOL Ln)
{
	WORD ret;

	ret = TTLWait(Ln);

	RegexActionType = REGEX_WAIT; // regex enabled

	return (ret);
}



static WORD TTLWaitEvent(void)
{
	WORD Err;
	TVariableType ValType;
	TVarId VarId;
	int TimeOut;

	Err = 0;
	GetIntVal(&WakeupCondition,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	WakeupCondition &= 15;
	TimeOut = 0;
	if (CheckVar("timeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut = CopyIntVal(VarId) * 1000;
	}
	if (CheckVar("mtimeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut += CopyIntVal(VarId);
	}

	if (TimeOut>0)
	{
		TimeLimit = (DWORD)TimeOut;
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
	}

	TTLStatus = IdTTLSleep;
	return Err;
}


static WORD TTLWaitN(void)
{
	WORD Err;
	TVariableType ValType;
	TVarId VarId;
	int TimeOut, WaitBytes;

	ClearWaitN();

	Err = 0;
	GetIntVal(&WaitBytes,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetWaitN(WaitBytes);

	if (! Linked)
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	TTLStatus = IdTTLWaitN;
	TimeOut = 0;
	if (CheckVar("timeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut = CopyIntVal(VarId) * 1000;
	}
	if (CheckVar("mtimeout",&ValType,&VarId) && (ValType==TypInteger)) {
		TimeOut += CopyIntVal(VarId);
	}

	if (TimeOut>0)
	{
		TimeLimit = (DWORD)TimeOut;
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
	}

	return Err;
}


static WORD TTLWaitRecv(void)
{
	TStrVal Str;
	WORD Err;
	int Pos, Len, TimeOut;
	TVariableType VarType;
	TVarId VarId;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&Len,&Err);
	GetIntVal(&Pos,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;
	SetInputStr("");
	SetWait2(Str,Len,Pos);

	TTLStatus = IdTTLWait2;
	TimeOut = 0;
	if (CheckVar("timeout",&VarType,&VarId) && (VarType==TypInteger)) {
		TimeOut = CopyIntVal(VarId) * 1000;
	}
	if (CheckVar("mtimeout",&VarType,&VarId) && (VarType==TypInteger)) {
		TimeOut += CopyIntVal(VarId);
	}

	if (TimeOut>0)
	{
		TimeLimit = (DWORD)TimeOut;
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
	}
	return Err;
}

static WORD TTLWhile(BOOL mode)
{
	WORD Err;
	int Val = mode;

	Err = 0;
	if (CheckParameterGiven()) {
		GetIntVal(&Val,&Err);
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if ((Val!=0) == mode)
		return SetWhileLoop();
	else
		EndWhileLoop();
	return Err;
}

static WORD TTLXmodemRecv(void)
{
	TStrVal Str;
	WORD Err;
	int BinFlag, XOption;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&BinFlag,&Err);
	GetIntVal(&XOption,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	switch (XOption) {
	case XoptCRC:
		// NOP
		break;
	case Xopt1kCRC:
		// for compatibility
		XOption = XoptCRC;
		break;
	default:
		XOption = XoptCheck;
	}

	SetFile(Str);
	SetBinary(BinFlag);
	SetXOption(XOption);
	return SendCmnd(CmdXmodemRecv,IdTTLWaitCmndResult);
}

static WORD TTLXmodemSend(void)
{
	TStrVal Str;
	WORD Err;
	int XOption;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&XOption,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	switch (XOption) {
	case Xopt1kCRC:
		// NOP
		break;
	default:
		XOption = XoptCRC;
	}

	SetFile(Str);
	SetXOption(XOption);
	return SendCmnd(CmdXmodemSend,IdTTLWaitCmndResult);
}

static WORD TTLZmodemSend(void)
{
	TStrVal Str;
	WORD Err;
	int BinFlag;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&BinFlag,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetFile(Str);
	SetBinary(BinFlag);
	return SendCmnd(CmdZmodemSend,IdTTLWaitCmndResult);
}

static WORD TTLYmodemSend(void)
{
	TStrVal Str;
	WORD Err;
//	int BinFlag;

	Err = 0;
	GetStrVal(Str,&Err);
//	GetIntVal(&BinFlag,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetFile(Str);
//	SetBinary(BinFlag);
	return SendCmnd(CmdYmodemSend,IdTTLWaitCmndResult);
}

// SYNOPSIS:
//   scpsend "c:\usr\sample.chm" "doc/sample.chm"
//   scpsend "c:\usr\sample.chm"
static WORD TTLScpSend(void)
{
	TStrVal Str;
	TStrVal Str2;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);

	if ((Err==0) &&
	    ((strlen(Str)==0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetStrVal(Str2,&Err);
	if (Err) {
		Str2[0] = '\0';
		Err = 0;
	}

	if (GetFirstChar() != 0)
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetFile(Str);
	SetSecondFile(Str2);
	return SendCmnd(CmdScpSend, 0);
}

// SYNOPSIS:
//   scprecv "foo.txt"
//   scprecv "src/foo.txt" "c:\foo.txt"
static WORD TTLScpRecv(void)
{
	TStrVal Str;
	TStrVal Str2;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);

	if ((Err==0) &&
	    ((strlen(Str)==0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetStrVal(Str2,&Err);
	if (Err) {
		Str2[0] = '\0';
		Err = 0;
	}

	if (GetFirstChar() != 0)
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetFile(Str);
	SetSecondFile(Str2);
	return SendCmnd(CmdScpRcv, 0);
}

#if defined(OUTPUTDEBUGSTRING_ENABLE)
static WORD TTLOutputDebugstring(void)
{
	TStrVal str;

	WORD Err = 0;
	GetStrVal(str, &Err);
	if (Err !=0) {
		return ErrSyntax;
	}
	OutputDebugStringW(wc::fromUtf8(str));
	return 0;
}
#endif

static int ExecCmnd(void)
{
	WORD WId, Err;
	BOOL StrConst, E, WithIndex, Result;
	TStrVal Str;
	TName Cmnd;
	TVariableType ValType, VarType;
	int Val;

	Err = 0;

	Result = GetReservedWord(&WId);

	if (EndWhileFlag>0) {
		if (Result) {
			switch (WId) {
			case RsvWhile:
			case RsvUntil:
			case RsvDo:
				EndWhileFlag++; break;

			case RsvEndWhile:
			case RsvEndUntil:
			case RsvLoop:
				EndWhileFlag--; break;
			}
		}
		return 0;
	}

	if (BreakFlag>0) {
		if (Result) {
			switch (WId) {
			case RsvIf:
				if (CheckThen(&Err))
					IfNest++;
				break;

			case RsvEndIf:
				if (IfNest<1)
					Err = ErrInvalidCtl;
				else
					IfNest--;
				break;

			case RsvFor:
			case RsvWhile:
			case RsvUntil:
			case RsvDo:
				BreakFlag++; break;

			case RsvNext:
			case RsvEndWhile:
			case RsvEndUntil:
			case RsvLoop:
				BreakFlag--; break;
			}
		}
		if (BreakFlag>0 || !ContinueFlag)
			return Err;
		ContinueFlag = FALSE;
	}

	if (EndIfFlag>0) {
		if (! Result)
			;
		else if ((WId==RsvIf) && CheckThen(&Err))
			EndIfFlag++;
		else if (WId==RsvEndIf)
			EndIfFlag--;
		return Err;
	}

	if (ElseFlag>0) {
		if (! Result)
			;
		else if ((WId==RsvIf) && CheckThen(&Err))
			EndIfFlag++;
		else if (WId==RsvElse)
			ElseFlag--;
		else if (WId==RsvElseIf)
		{
			if (CheckElseIf(&Err)!=0)
				ElseFlag--;
		}
		else if (WId==RsvEndIf)
		{
			ElseFlag--;
			if (ElseFlag==0)
				IfNest--;
		}
		return Err;
	}

	if (Result)
		switch (WId) {
		case RsvBasename:
			Err = TTLBasename(); break;
		case RsvBeep:
			Err = TTLBeep(); break;
		case RsvBPlusRecv:
			Err = TTLCommCmd(CmdBPlusRecv,IdTTLWaitCmndResult); break;
		case RsvBPlusSend:
			Err = TTLCommCmdFile(CmdBPlusSend,IdTTLWaitCmndResult); break;
		case RsvBreak:
		case RsvContinue:
			Err = TTLBreak(WId); break;
		case RsvBringupBox:
			Err = TTLBringupBox(); break;
		case RsvCall:
			Err = TTLCall(); break;
		case RsvCallMenu:
			Err = TTLCommCmdInt(CmdCallMenu,0); break;
		case RsvChangeDir:
			Err = TTLCommCmdFile(CmdChangeDir,0); break;
		case RsvChecksum8:
			Err = TTLDoChecksum(CHECKSUM8); break;
		case RsvChecksum8File:
			Err = TTLDoChecksumFile(CHECKSUM8); break;
		case RsvChecksum16:
			Err = TTLDoChecksum(CHECKSUM16); break;
		case RsvChecksum16File:
			Err = TTLDoChecksumFile(CHECKSUM16); break;
		case RsvChecksum32:
			Err = TTLDoChecksum(CHECKSUM32); break;
		case RsvChecksum32File:
			Err = TTLDoChecksumFile(CHECKSUM32); break;
		case RsvClearScreen:
			Err = TTLCommCmdInt(CmdClearScreen,0); break;
		case RsvClipb2Var:
			Err = TTLClipb2Var(); break;    // add 'clipb2var' (2006.9.17 maya)
		case RsvCloseSBox:
			Err = TTLCloseSBox(); break;
		case RsvCloseTT:
			Err = TTLCloseTT(); break;
		case RsvCode2Str:
			Err = TTLCode2Str(); break;
		case RsvConnect:
		case RsvCygConnect:
			Err = TTLConnect(WId); break;
		case RsvCrc16:
			Err = TTLDoChecksum(CRC16); break;
		case RsvCrc16File:
			Err = TTLDoChecksumFile(CRC16); break;
		case RsvCrc32:
			Err = TTLDoChecksum(CRC32); break;
		case RsvCrc32File:
			Err = TTLDoChecksumFile(CRC32); break;
		case RsvDelPassword:
			Err = TTLDelPassword(); break;
		case RsvDelPassword2:
			Err = TTLDelPassword2(); break;
		case RsvDirname:
			Err = TTLDirname(); break;
		case RsvDirnameBox:
			Err = TTLDirnameBox(); break;
		case RsvDisconnect:
			Err = TTLDisconnect(); break;
		case RsvDispStr:
			Err = TTLDispStr(); break;
		case RsvDo:
			Err = TTLDo(); break;
		case RsvElse:
			Err = TTLElse(); break;
		case RsvElseIf:
			Err = TTLElseIf(); break;
		case RsvEnableKeyb:
			Err = TTLCommCmdBin(CmdEnableKeyb,0); break;
		case RsvEnd:
			Err = TTLEnd(); break;
		case RsvEndIf:
			Err = TTLEndIf(); break;
		case RsvEndUntil:
			Err = TTLEndWhile(FALSE); break;
		case RsvEndWhile:
			Err = TTLEndWhile(TRUE); break;
		case RsvExec:
			Err = TTLExec(); break;
		case RsvExecCmnd:
			Err = TTLExecCmnd(); break;
		case RsvExit:
			Err = TTLExit(); break;
		case RsvExpandEnv:
			Err = TTLExpandEnv(); break;
		case RsvFileClose:
			Err = TTLFileClose(); break;
		case RsvFileConcat:
			Err = TTLFileConcat(); break;
		case RsvFileCopy:
			Err = TTLFileCopy(); break;
		case RsvFileCreate:
			Err = TTLFileCreate(); break;
		case RsvFileDelete:
			Err = TTLFileDelete(); break;
		case RsvFileMarkPtr:
			Err = TTLFileMarkPtr(); break;
		case RsvFilenameBox:
			Err = TTLFilenameBox(); break;  // add 'filenamebox' (2007.9.13 maya)
		case RsvFileOpen:
			Err = TTLFileOpen(); break;
		case RsvFileLock:
			Err = TTLFileLock(); break;
		case RsvFileUnLock:
			Err = TTLFileUnLock(); break;
		case RsvFileReadln:
			Err = TTLFileReadln(); break;
		case RsvFileRead:
			Err = TTLFileRead(); break;    // add 'fileread'
		case RsvFileRename:
			Err = TTLFileRename(); break;
		case RsvFileSearch:
			Err = TTLFileSearch(); break;
		case RsvFileSeek:
			Err = TTLFileSeek(); break;
		case RsvFileSeekBack:
			Err = TTLFileSeekBack(); break;
		case RsvFileStat:
			Err = TTLFileStat(); break;
		case RsvFileStrSeek:
			Err = TTLFileStrSeek(); break;
		case RsvFileStrSeek2:
			Err = TTLFileStrSeek2(); break;
		case RsvFileTruncate:
			Err = TTLFileTruncate(); break;
		case RsvFileWrite:
			Err = TTLFileWrite(FALSE); break;
		case RsvFileWriteLn:
			Err = TTLFileWrite(TRUE); break;
		case RsvFindClose:
			Err = TTLFindClose(); break;
		case RsvFindFirst:
			Err = TTLFindFirst(); break;
		case RsvFindNext:
			Err = TTLFindNext(); break;
		case RsvFlushRecv:
			Err = TTLFlushRecv(); break;
		case RsvFolderCreate:
			Err = TTLFolderCreate(); break;
		case RsvFolderDelete:
			Err = TTLFolderDelete(); break;
		case RsvFolderSearch:
			Err = TTLFolderSearch(); break;
		case RsvFor:
			Err = TTLFor(); break;
		case RsvGetDate:
		case RsvGetTime:
			Err = TTLGetTime(WId); break;
		case RsvGetDir:
			Err = TTLGetDir(); break;
		case RsvGetEnv:
			Err = TTLGetEnv(); break;
		case RsvGetFileAttr:
			Err = TTLGetFileAttr(); break;
		case RsvGetHostname:
			Err = TTLGetHostname(); break;
		case RsvGetIPv4Addr:
			Err = TTLGetIPv4Addr(); break;
		case RsvGetIPv6Addr:
			Err = TTLGetIPv6Addr(); break;
		case RsvGetModemStatus:
			Err = TTLGetModemStatus(); break;
		case RsvGetPassword:
			Err = TTLGetPassword(); break;
		case RsvGetPassword2:
			Err = TTLGetPassword2(); break;
		case RsvSetPassword:
			Err = TTLSetPassword(); break;
		case RsvSetPassword2:
			Err = TTLSetPassword2(); break;
		case RsvIsPassword:
			Err = TTLIsPassword(); break;
		case RsvIsPassword2:
			Err = TTLIsPassword2(); break;
		case RsvGetSpecialFolder:
			Err = TTLGetSpecialFolder(); break;
		case RsvGetTitle:
			Err = TTLGetTitle(); break;
		case RsvGetTTDir:
			Err = TTLGetTTDir(); break;
		case RsvGetTTPos:
			Err = TTLGetTTPos(); break;
		case RsvGetVer:
			Err = TTLGetVer(); break;
		case RsvGoto:
			Err = TTLGoto(); break;
		case RsvIfDefined:
			Err = TTLIfDefined(); break;
		case RsvIf:
			Err = TTLIf(); break;
		case RsvInclude:
			Err = TTLInclude(); break;
		case RsvInputBox:
			Err = TTLInputBox(FALSE); break;
		case RsvInt2Str:
			Err = TTLInt2Str(); break;
		case RsvIntDim:
		case RsvStrDim:
			Err = TTLDim(WId); break;
		case RsvKmtFinish:
			Err = TTLCommCmd(CmdKmtFinish,IdTTLWaitCmndResult); break;
		case RsvKmtGet:
			Err = TTLCommCmdFile(CmdKmtGet,IdTTLWaitCmndResult); break;
		case RsvKmtRecv:
			Err = TTLCommCmd(CmdKmtRecv,IdTTLWaitCmndResult); break;
		case RsvKmtSend:
			Err = TTLCommCmdFile(CmdKmtSend,IdTTLWaitCmndResult); break;
		case RsvListBox:
			Err = TTLListBox(); break;
		case RsvLoadKeyMap:
			Err = TTLCommCmdFile(CmdLoadKeyMap,0); break;
		case RsvLogAutoClose:
			Err = TTLCommCmdBin(CmdLogAutoClose, 0); break;
		case RsvLogClose:
			Err = TTLCommCmd(CmdLogClose,0); break;
		case RsvLogInfo:
			Err = TTLLogInfo(); break;
		case RsvLogOpen:
			Err = TTLLogOpen(); break;
		case RsvLogPause:
			Err = TTLCommCmd(CmdLogPause,0); break;
		case RsvLogRotate:
			Err = TTLLogRotate(); break;
		case RsvLogStart:
			Err = TTLCommCmd(CmdLogStart,0); break;
		case RsvLogWrite:
			Err = TTLCommCmdFile(CmdLogWrite,0); break;
		case RsvLoop:
			Err = TTLLoop(); break;
		case RsvMakePath:
			Err = TTLMakePath(); break;
		case RsvMessageBox:
			Err = TTLMessageBox(); break;
		case RsvNext:
			Err = TTLNext(); break;
		case RsvPasswordBox:
			Err = TTLInputBox(TRUE); break;
		case RsvPause:
			Err = TTLPause(); break;
		case RsvMilliPause:
			Err = TTLMilliPause(); break;    // add 'mpause'
		case RsvQuickVANRecv:
			Err = TTLCommCmd(CmdQVRecv,IdTTLWaitCmndResult); break;
		case RsvQuickVANSend:
			Err = TTLCommCmdFile(CmdQVSend,IdTTLWaitCmndResult); break;
		case RsvRandom:
			Err = TTLRandom(); break;
		case RsvRecvLn:
			Err = TTLRecvLn(); break;
		case RsvRegexOption:
			Err = TTLRegexOption(); break;
		case RsvRestoreSetup:
			Err = TTLCommCmdFile(CmdRestoreSetup,0); break;
		case RsvReturn:
			Err = TTLReturn(); break;
		case RsvRotateL:
			Err = TTLRotateLeft(); break;   // add 'rotateleft' (2007.8.19 maya)
		case RsvRotateR:
			Err = TTLRotateRight(); break;  // add 'rotateright' (2007.8.19 maya)
		case RsvScpSend:
			Err = TTLScpSend(); break;      // add 'scpsend' (2008.1.1 yutaka)
		case RsvScpRecv:
			Err = TTLScpRecv(); break;      // add 'scprecv' (2008.1.4 yutaka)
		case RsvSend:
			Err = TTLSend(); break;
		case RsvSendText:
			Err = TTLSendText(); break;
		case RsvSendBinary:
			Err = TTLSendBinary(); break;
		case RsvSendBreak:
			Err = TTLCommCmd(CmdSendBreak,0); break;
		case RsvSendBroadcast:
			Err = TTLSendBroadcast(FALSE); break;
		case RsvSendlnBroadcast:
			Err = TTLSendBroadcast(TRUE); break;
		case RsvSendlnMulticast:
			Err = TTLSendMulticast(TRUE); break;
		case RsvSendMulticast:
			Err = TTLSendMulticast(FALSE); break;
		case RsvSetMulticastName:
			Err = TTLSetMulticastName(); break;
		case RsvSendFile:
			Err = TTLSendFile(); break;
		case RsvSendKCode:
			Err = TTLSendKCode(); break;
		case RsvSendLn:
			Err = TTLSendLn(); break;
		case RsvSetBaud:
			Err = TTLCommCmdInt(CmdSetBaud,0); break;
		case RsvSetDate:
			Err = TTLSetDate(); break;
		case RsvSetDebug:
			Err = TTLCommCmdDeb(); break;
		case RsvSetDir:
			Err = TTLSetDir(); break;
		case RsvSetDlgPos:
			Err = TTLSetDlgPos(); break;
		case RsvSetDtr:
			Err = TTLCommCmdInt(CmdSetDtr,0); break;
		case RsvSetEcho:
			Err = TTLCommCmdBin(CmdSetEcho,0); break;
		case RsvSetEnv:
			Err = TTLSetEnv(); break;
		case RsvSetExitCode:
			Err = TTLSetExitCode(); break;
		case RsvSetFileAttr:
			Err = TTLSetFileAttr(); break;
		case RsvSetFlowCtrl:
			Err = TTLCommCmdInt(CmdSetFlowCtrl,0); break;
		case RsvSetRts:
			Err = TTLCommCmdInt(CmdSetRts,0); break;
		case RsvSetSync:
			Err = TTLSetSync(); break;
		case RsvSetTime:
			Err = TTLSetTime(); break;
		case RsvSetTitle:
			Err = TTLCommCmdFile(CmdSetTitle,0); break;
		case RsvShow:
			Err = TTLShow(); break;
		case RsvShowTT:
			Err = TTLCommCmdInt(CmdShowTT,0); break;
		case RsvSprintf:
			Err = TTLSprintf(0); break;
		case RsvSprintf2:
			Err = TTLSprintf(1); break;
		case RsvStatusBox:
			Err = TTLStatusBox(); break;
		case RsvStr2Code:
			Err = TTLStr2Code(); break;
		case RsvStr2Int:
			Err = TTLStr2Int(); break;
		case RsvStrCompare:
			Err = TTLStrCompare(); break;
		case RsvStrConcat:
			Err = TTLStrConcat(); break;
		case RsvStrCopy:
			Err = TTLStrCopy(); break;
		case RsvStrInsert:
			Err = TTLStrInsert(); break;
		case RsvStrJoin:
			Err = TTLStrJoin(); break;
		case RsvStrLen:
			Err = TTLStrLen(); break;
		case RsvStrMatch:
			Err = TTLStrMatch(); break;
		case RsvStrRemove:
			Err = TTLStrRemove(); break;
		case RsvStrReplace:
			Err = TTLStrReplace(); break;
		case RsvStrScan:
			Err = TTLStrScan(); break;
		case RsvStrSpecial:
			Err = TTLStrSpecial(); break;
		case RsvStrSplit:
			Err = TTLStrSplit(); break;
		case RsvStrTrim:
			Err = TTLStrTrim(); break;
		case RsvTestLink:
			Err = TTLTestLink(); break;
		case RsvToLower:
			Err = TTLToLower(); break;    // add 'tolower' (2007.7.12 maya)
		case RsvToUpper:
			Err = TTLToUpper(); break;    // add 'toupper' (2007.7.12 maya)
		case RsvUnlink:
			Err = TTLUnlink(); break;
		case RsvUntil:
			Err = TTLWhile(FALSE); break;
		case RsvUptime:
			Err = TTLUptime(); break;
		case RsvVar2Clipb:
			Err = TTLVar2Clipb(); break;    // add 'var2clipb' (2006.9.17 maya)
		case RsvWaitRegex:
			Err = TTLWaitRegex(FALSE); break;    // add 'waitregex' (2005.10.5 yutaka)
		case RsvWait:
			Err = TTLWait(FALSE); break;
		case RsvWait4all:
			Err = TTLWait4all(FALSE); break;
		case RsvWaitEvent:
			Err = TTLWaitEvent(); break;
		case RsvWaitLn:
			Err = TTLWait(TRUE); break;
		case RsvWaitN:
			Err = TTLWaitN(); break;
		case RsvWaitRecv:
			Err = TTLWaitRecv(); break;
		case RsvWhile:
			Err = TTLWhile(TRUE); break;
		case RsvXmodemRecv:
			Err = TTLXmodemRecv(); break;
		case RsvXmodemSend:
			Err = TTLXmodemSend(); break;
		case RsvYesNoBox:
			Err = TTLYesNoBox(); break;
		case RsvZmodemRecv:
			Err = TTLCommCmd(CmdZmodemRecv,IdTTLWaitCmndResult); break;
		case RsvZmodemSend:
			Err = TTLZmodemSend(); break;
		case RsvYmodemRecv:
			Err = TTLCommCmd(CmdYmodemRecv,IdTTLWaitCmndResult); break;
		case RsvYmodemSend:
			Err = TTLYmodemSend(); break;
#if defined(OUTPUTDEBUGSTRING_ENABLE)
		case RsvOutputDebugString:
			Err = TTLOutputDebugstring(); break;
#endif
		default:
			Err = ErrSyntax;
		}
	else if (GetIdentifier(Cmnd)) {
		int Index;
		if (GetIndex(&Index, &Err)) {
			WithIndex = TRUE;
		}
		else {
			WithIndex = FALSE;
		}

		if (!Err && GetFirstChar() == '=') {
			StrConst = GetString(Str,&Err);
			if (StrConst)
				ValType = TypString;
			else
				if (! GetExpression(&ValType,&Val,&Err))
					Err = ErrSyntax;

			if (!Err) {
				TVarId VarId;
				if (CheckVar(Cmnd,&VarType,&VarId)) {
					if (WithIndex) {
						switch (VarType) {
						case TypIntArray:
							VarId = GetIntVarFromArray(VarId, Index, &Err);
							if (!Err) VarType = TypInteger;
							break;
						case TypStrArray:
							VarId = GetStrVarFromArray(VarId, Index, &Err);
							if (!Err) VarType = TypString;
							break;
						default:
							Err = ErrSyntax;
						}
					}
					if (Err) return Err;
					if (VarType==ValType) {
						switch (ValType) {
						case TypInteger: SetIntVal(VarId,Val); break;
						case TypString:
							if (StrConst)
								SetStrVal(VarId,Str);
							else {
								SetStrVal(VarId, StrVarPtr((TVarId)Val));
							}
						break;
						default:
							Err = ErrSyntax;
						}
					}
					else {
						Err = ErrTypeMismatch;
					}
				}
				else if (WithIndex) {
					Err = ErrSyntax;
				}
				else {
					switch (ValType) {
					case TypInteger: E = NewIntVar(Cmnd,Val); break;
					case TypString:
						if (StrConst)
							E = NewStrVar(Cmnd,Str);
						else
							E = NewStrVar(Cmnd,StrVarPtr((TVarId)Val));
						break;
					default:
						E = FALSE;
					}
					if (! E) Err = ErrTooManyVar;
				}
				if (!Err && (GetFirstChar()!=0))
					Err = ErrSyntax;
			}
		}
		else Err = ErrNotSupported;
	}
	else
		Err = ErrSyntax;

	return Err;
}

void Exec(void)
{
	WORD Err;

	// ParseAgain is set by 'ExecCmnd'
	if (! ParseAgain &&
	    ! GetNewLine())
	{
		TTLStatus = IdTTLEnd;
		return;
	}
	ParseAgain = FALSE;

	LockVar();
	Err = ExecCmnd();
	if (Err>0) DispErr(Err);
	UnlockVar();
}

// ���K�\���Ń}�b�`������������L�^����
// (2005.10.7 yutaka)
void SetMatchStr(PCHAR Str)
{
	TVariableType VarType;
	TVarId VarId;

	if (CheckVar("matchstr",&VarType,&VarId) &&
	    (VarType==TypString))
		SetStrVal(VarId,Str);
}

// ���K�\���ŃO���[�v�}�b�`������������L�^����
// (2005.10.15 yutaka)
void SetGroupMatchStr(int no, const char *Str)
{
	TVariableType VarType;
	TVarId VarId;
	char buf[128];
	const char *p;

	if (Str == NULL)
		p = "";
	else
		p = Str;

	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "groupmatchstr%d", no);

	if (CheckVar(buf,&VarType,&VarId) &&
	    (VarType==TypString))
		SetStrVal(VarId,p);
}

void SetInputStr(const char *Str)
{
	TVariableType VarType;
	TVarId VarId;

	if (CheckVar("inputstr",&VarType,&VarId) &&
	    (VarType==TypString))
		SetStrVal(VarId,Str);
}

void SetResult(int ResultCode)
{
	TVariableType VarType;
	TVarId VarId;

	if (CheckVar("result",&VarType,&VarId) &&
	    (VarType==TypInteger))
		SetIntVal(VarId,ResultCode);
}

BOOL CheckTimeout()
{
	BOOL ret;
	DWORD TimeUp = (TimeStart+TimeLimit);

	if (TimeUp > TimeStart) {
		ret = (GetTickCount() > TimeUp);
	}
	else { // for DWORD overflow (49.7 days)
		DWORD TimeTmp = GetTickCount();
		ret = (TimeUp < TimeTmp && TimeTmp >= TimeStart);
	}
	return ret;
}

BOOL TestWakeup(int Wakeup)
{
	return ((Wakeup & WakeupCondition) != 0);
}

void SetWakeup(int Wakeup)
{
	WakeupCondition = Wakeup;
}
