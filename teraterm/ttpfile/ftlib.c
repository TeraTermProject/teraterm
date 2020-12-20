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

/* TTFILE.DLL, routines for file transfer protocol */

#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "ttlib.h"
#include <stdio.h>
#include <string.h>

#include "ftlib.h"
#include "tt_res.h"

/**
 *	ファイル名を取得
 *
 *	@return		ファイル名
 *				NULLのとき次のファイルはない
 *				不要になったら free() すること
 */
char *GetNextFname(PFileVarProto fv)
{
	char *f = fv->FileNames[fv->FNCount];
	if (f == NULL) {
		/* no more file name */
		return NULL;
	}
	fv->FNCount++;
	f = _strdup(f);
	return f;
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

//
// プロトコル用ログ
//

static BOOL Open(TProtoLog *pv, const char *file)
{
	pv->LogFile = CreateFileA(file,
							  GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
							  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	pv->LogCount = 0;
	pv->LogState = 0;
	return pv->LogFile == INVALID_HANDLE_VALUE ? FALSE : TRUE;
}

static void Close(TProtoLog *pv)
{
	if (pv->LogFile != INVALID_HANDLE_VALUE) {
		if (pv->LogCount > 0) {
			pv->DumpFlush(pv);
		}
		CloseHandle(pv->LogFile);
		pv->LogFile = INVALID_HANDLE_VALUE;
	}
}

static size_t WriteRawData(struct ProtoLog *pv, const void *data, size_t len)
{
	DWORD NumberOfBytesWritten;
	BOOL result = WriteFile(pv->LogFile, data, len, &NumberOfBytesWritten, NULL);
	if (result == FALSE) {
		return 0;
	}
	return NumberOfBytesWritten;
}

static size_t WriteStr(TProtoLog *pv, const char *str)
{
	size_t len = strlen(str);
	size_t r = WriteRawData(pv, str, len);
	return r;
}

static void DumpByte(TProtoLog *pv, BYTE b)
{
	char d[3];

	if (pv->LogCount == _countof(pv->LogLineBuf)) {
		pv->DumpFlush(pv);
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
	pv->WriteRaw(pv,d,3);
	pv->LogLineBuf[pv->LogCount] = b;    // add (2008.6.3 yutaka)
	pv->LogCount++;
}

static void DumpFlush(TProtoLog *pv)
{
	int rest = 16 - pv->LogCount;
	int i;

	for (i = 0 ; i < rest ; i++)
		pv->WriteRaw(pv,"   ", 3);

	// ASCII表示を追加 (2008.6.3 yutaka)
	pv->WriteRaw(pv,"    ", 4);
	for (i = 0 ; i < pv->LogCount ; i++) {
		char ch[5];
		if (isprint(pv->LogLineBuf[i])) {
			_snprintf_s(ch, sizeof(ch), _TRUNCATE, "%c", pv->LogLineBuf[i]);
			pv->WriteRaw(pv, ch, 1);

		} else {
			pv->WriteRaw(pv, ".", 1);

		}

	}

	pv->LogCount = 0;
	pv->WriteRaw(pv,"\015\012",2);
}

static void ProtoLogDestroy(TProtoLog *pv)
{
	pv->Close(pv);
	free(pv);
}

TProtoLog *ProtoLogCreate()
{
	TProtoLog *pv = (TProtoLog *)malloc(sizeof(TProtoLog));
	if (pv == NULL) {
		return NULL;
	}

	memset(pv, 0, sizeof(TProtoLog));
	pv->Open = Open;
	pv->Close = Close;
	pv->WriteStr = WriteStr;
	pv->DumpByte = DumpByte;
	pv->DumpFlush = DumpFlush;
	pv->WriteRaw = WriteRawData;
	pv->Destory = ProtoLogDestroy;
	pv->LogFile = INVALID_HANDLE_VALUE;
	return pv;
}
