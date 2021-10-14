/*
 * (C) 2021- TeraTerm Project
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

#include <stdio.h>
#include <string.h>

#include "asprintf.h"
#include "codeconv.h"

#include "protolog.h"

typedef struct {
	HANDLE LogFile;
	WORD LogCount;
	BYTE LogLineBuf[16];
	wchar_t *Folder;
} PrivateData_t;

static BOOL OpenW(struct ProtoLog *pv, const wchar_t *file)
{
	PrivateData_t *pdata = (PrivateData_t *)pv->private_data;
	wchar_t *full_path;
	if (pdata->Folder != NULL) {
		aswprintf(&full_path, L"%s\\%s", pdata->Folder, file);
	} else {
		full_path = _wcsdup(file);
	}
	pdata->LogFile = CreateFileW(full_path,
								 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
								 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	free(full_path);
	pdata->LogCount = 0;
	return pdata->LogFile == INVALID_HANDLE_VALUE ? FALSE : TRUE;
}

static BOOL OpenA(TProtoLog *pv, const char *file)
{
	wchar_t *fileW = ToWcharA(file);
	BOOL r = OpenW(pv, fileW);
	free(fileW);
	return r;
}

static BOOL OpenU8(TProtoLog *pv, const char *fileU8)
{
	wchar_t *fileW = ToWcharU8(fileU8);
	BOOL r = OpenW(pv, fileW);
	free(fileW);
	return r;
}

static void Close(TProtoLog *pv)
{
	PrivateData_t *pdata = (PrivateData_t *)pv->private_data;
	if (pdata->LogFile != INVALID_HANDLE_VALUE) {
		if (pdata->LogCount > 0) {
			pv->DumpFlush(pv);
		}
		CloseHandle(pdata->LogFile);
		pdata->LogFile = INVALID_HANDLE_VALUE;
	}
}

static size_t WriteRawData(struct ProtoLog *pv, const void *data, size_t len)
{
	PrivateData_t *pdata = (PrivateData_t *)pv->private_data;
	DWORD NumberOfBytesWritten;
	BOOL result = WriteFile(pdata->LogFile, data, (DWORD)len, &NumberOfBytesWritten, NULL);
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
	PrivateData_t *pdata = (PrivateData_t *)pv->private_data;
	char d[3];

	if (pdata->LogCount == _countof(pdata->LogLineBuf)) {
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
	pdata->LogLineBuf[pdata->LogCount] = b;    // add (2008.6.3 yutaka)
	pdata->LogCount++;
}

static void NewLine(TProtoLog *pv)
{
	pv->WriteRaw(pv,"\015\012",2);	// 0x0d 0x0a "\r\n"
}

static void DumpFlush(TProtoLog *pv)
{
	PrivateData_t *pdata = (PrivateData_t *)pv->private_data;
	int rest = 16 - pdata->LogCount;
	int i;

	for (i = 0 ; i < rest ; i++)
		pv->WriteRaw(pv,"   ", 3);

	// ASCII•\Ž¦‚ð’Ç‰Á (2008.6.3 yutaka)
	pv->WriteRaw(pv,"    ", 4);
	for (i = 0 ; i < pdata->LogCount ; i++) {
		char ch[5];
		if (isprint(pdata->LogLineBuf[i])) {
			_snprintf_s(ch, sizeof(ch), _TRUNCATE, "%c", pdata->LogLineBuf[i]);
			pv->WriteRaw(pv, ch, 1);

		} else {
			pv->WriteRaw(pv, ".", 1);

		}

	}
	pdata->LogCount = 0;

	NewLine(pv);
}

static void SetFolderW(struct ProtoLog *pv, const wchar_t *folder)
{
	PrivateData_t *pdata = (PrivateData_t *)pv->private_data;
	if (pdata->Folder != NULL) {
		free(pdata->Folder);
	}
	if (folder != NULL) {
		pdata->Folder = _wcsdup(folder);
	}
}

static void ProtoLogDestroy(TProtoLog *pv)
{
	PrivateData_t *pdata = (PrivateData_t *)pv->private_data;
	if (pdata->Folder != NULL) {
		free(pdata->Folder);
		pdata->Folder = NULL;
	}
	free(pdata);
	pv->private_data = NULL;
	pv->Close(pv);
	free(pv);
}

TProtoLog *ProtoLogCreate()
{
	TProtoLog *pv = (TProtoLog *)calloc(sizeof(*pv), 1);
	if (pv == NULL) {
		return NULL;
	}

	PrivateData_t *pdata = (PrivateData_t *)calloc(sizeof(*pdata), 1);
	if (pdata == NULL) {
		free(pv);
		return NULL;
	}

	pv->Open = OpenA;
	pv->OpenA = OpenA;
	pv->OpenW = OpenW;
	pv->OpenU8 = OpenU8;
	pv->Close = Close;
	pv->SetFolderW = SetFolderW;
	pv->WriteStr = WriteStr;
	pv->DumpByte = DumpByte;
	pv->DumpFlush = DumpFlush;
	pv->WriteRaw = WriteRawData;
	pv->Destory = ProtoLogDestroy;

	pdata->LogFile = INVALID_HANDLE_VALUE;

	return pv;
}
