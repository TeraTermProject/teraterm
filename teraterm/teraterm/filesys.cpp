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

/* TERATERM.EXE, file transfer routines */
#include <stdio.h>
#include <windows.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ftdlg.h"
#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttdde.h"
#include "vtterm.h"
#include "layer_for_unicode.h"
#include "asprintf.h"

#include "filesys.h"

typedef struct {
	wchar_t *FullName;

	HANDLE FileHandle;
	LONG FileSize, ByteCount;

	DWORD StartTime;
	BOOL FilePause;
} TFileVar;
typedef TFileVar *PFileVar;

typedef enum {
	FS_BRACKET_NONE,
	FS_BRACKET_START,
	FS_BRACKET_END,
} FileBracketMode_t;

static PFileVar SendVar = NULL;

static BOOL FileRetrySend, FileRetryEcho, FileCRSend, FileReadEOF, BinaryMode;
static BYTE FileByte;

#define FILE_SEND_BUF_SIZE  8192
struct FileSendHandler {
	CHAR buf[FILE_SEND_BUF_SIZE];
	int pos;
	int end;
};
static struct FileSendHandler FileSendHandler;
static int FileDlgRefresh;

static FileBracketMode_t FileBracketMode = FS_BRACKET_NONE;
static int FileBracketPtr = 0;
static char BracketStartStr[] = "\033[200~";
static char BracketEndStr[] = "\033[201~";

static BOOL FSend = FALSE;

static PFileTransDlg SendDlg = NULL;

static BOOL OpenFTDlg(PFileVar fv)
{
	PFileTransDlg FTDlg;

	FTDlg = new CFileTransDlg();
	if (FTDlg == NULL) {
		return FALSE;
	}

	wchar_t *DlgCaption;
	wchar_t *uimsg = TTGetLangStrW("Tera Term", "FILEDLG_TRANS_TITLE_SENDFILE", L"Send file", ts.UILanguageFile);
	aswprintf(&DlgCaption, L"Tera Term: %s", uimsg);
	free(uimsg);

	CFileTransDlg::Info info;
	info.UILanguageFile = ts.UILanguageFile;
	info.OpId = CFileTransDlg::OpSendFile;
	info.DlgCaption = DlgCaption;
	info.FileName = NULL;
	info.FullName = fv->FullName;
	info.HideDialog = FALSE;
	info.HMainWin = HVTWin;

	FTDlg->Create(hInst, &info);
	FTDlg->RefreshNum(0, fv->FileSize, fv->ByteCount);

	free(DlgCaption);

	fv->FilePause = FALSE;
	fv->StartTime = GetTickCount();
	SendDlg = FTDlg; /* File send */

	return TRUE;
}

#if 0
static void ShowFTDlg(WORD OpId)
{
	if (SendDlg != NULL) {
		SendDlg->ShowWindow(SW_SHOWNORMAL);
		SetForegroundWindow(SendDlg->GetSafeHwnd());
	}
}
#endif

static BOOL NewFileVar(PFileVar *pfv)
{
	if ((*pfv) != NULL) {
		return TRUE;
	}

	PFileVar fv = (PFileVar)malloc(sizeof(TFileVar));
	if (fv == NULL) {
		return FALSE;
	}
	memset(fv, 0, sizeof(TFileVar));
	fv->FileHandle = INVALID_HANDLE_VALUE;

	*pfv = fv;
	return TRUE;
}

static void FreeFileVar(PFileVar *pfv)
{
	if ((*pfv)==NULL) {
		return;
	}

	PFileVar fv = *pfv;
	if (fv->FileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(fv->FileHandle);
		fv->FileHandle = INVALID_HANDLE_VALUE;
	}
	if (fv->FullName != NULL) {
		free(fv->FullName);
		fv->FullName = NULL;
	}
	free(fv);
	*pfv = NULL;
}

/**
 *	ファイル送信
 */
BOOL FileSendStart(const wchar_t *filename, int binary)
{
	if (SendVar != NULL) {
		return FALSE;
	}
	if (! cv.Ready || FSend) {
		return FALSE;
	}
	if (filename == NULL) {
		return FALSE;
	}
	if (ProtoGetProtoFlag())
	{
		FreeFileVar(&SendVar);
		return FALSE;
	}
	if (!NewFileVar(&SendVar)) {
		return FALSE;
	}

	FSend = TRUE;
	PFileVar fv = SendVar;

	SendVar->FullName = _wcsdup(filename);
	ts.TransBin = binary;

	SendVar->FileHandle = _CreateFileW(SendVar->FullName, GENERIC_READ, FILE_SHARE_READ, NULL,
									   OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (SendVar->FileHandle == INVALID_HANDLE_VALUE) {
		FileSendEnd();
		return FALSE;
	}
	SendVar->ByteCount = 0;
	SendVar->FileSize = (LONG)GetFSize64H(SendVar->FileHandle);

	TalkStatus = IdTalkFile;
	FileRetrySend = FALSE;
	FileRetryEcho = FALSE;
	FileCRSend = FALSE;
	FileReadEOF = FALSE;
	FileSendHandler.pos = 0;
	FileSendHandler.end = 0;
	FileDlgRefresh = 0;

	if (BracketedPasteMode()) {
		FileBracketMode = FS_BRACKET_START;
		FileBracketPtr = 0;
		BinaryMode = TRUE;
	}
	else {
		FileBracketMode = FS_BRACKET_NONE;
		BinaryMode = ts.TransBin;
	}

	if (! OpenFTDlg(SendVar)) {
		FileSendEnd();
		return FALSE;
	}

	return TRUE;
}

void FileSendEnd(void)
{
	if (FSend) {
		FSend = FALSE;
		TalkStatus = IdTalkKeyb;
		if (SendDlg!=NULL)
		{
			SendDlg->DestroyWindow();
			SendDlg = NULL;
		}
		FreeFileVar(&SendVar);
	}

	EndDdeCmnd(0);
}

void FileSendPause(BOOL Pause)
{
	PFileVar fv = SendVar;
	if (Pause) {
		fv->FilePause = TRUE;
	}
	else {
		fv->FilePause = FALSE;
	}
}

static int FSOut1(BYTE b)
{
	if (BinaryMode)
		return CommBinaryOut(&cv,(PCHAR)&b,1);
	else if ((b>=0x20) || (b==0x09) || (b==0x0A) || (b==0x0D))
		return CommTextOut(&cv,(PCHAR)&b,1);
	else
		return 1;
}

static int FSEcho1(BYTE b)
{
	if (BinaryMode)
		return CommBinaryEcho(&cv,(PCHAR)&b,1);
	else
		return CommTextEcho(&cv,(PCHAR)&b,1);
}

// 以下の時はこちらの関数を使う
// - BinaryMode == true
// - FileBracketMode == FS_BRACKET_NONE
// - cv.TelFlag == false
// - ts.LocalEcho == 0
static void FileSendBinayBoost(void)
{
	WORD c, fc;
	LONG BCOld;
	DWORD read_bytes;
	PFileVar fv = SendVar;

	if ((SendDlg == NULL) || (fv->FilePause == TRUE))
		return;

	BCOld = SendVar->ByteCount;

	if (FileRetrySend)
	{
		c = CommRawOut(&cv, &(FileSendHandler.buf[FileSendHandler.pos]),
			FileSendHandler.end - FileSendHandler.pos);
		FileSendHandler.pos += c;
		FileRetrySend = (FileSendHandler.end != FileSendHandler.pos);
		if (FileRetrySend)
			return;
	}

	do {
		if (FileSendHandler.pos == FileSendHandler.end) {
			ReadFile(SendVar->FileHandle, &(FileSendHandler.buf[0]), sizeof(FileSendHandler.buf), &read_bytes, NULL);
			fc = LOWORD(read_bytes);
			FileSendHandler.pos = 0;
			FileSendHandler.end = fc;
		} else {
			fc = FileSendHandler.end - FileSendHandler.end;
		}

		if (fc != 0)
		{
			c = CommRawOut(&cv, &(FileSendHandler.buf[FileSendHandler.pos]),
				FileSendHandler.end - FileSendHandler.pos);
			FileSendHandler.pos += c;
			FileRetrySend = (FileSendHandler.end != FileSendHandler.pos);
			SendVar->ByteCount = SendVar->ByteCount + c;
			if (FileRetrySend)
			{
				if (SendVar->ByteCount != BCOld) {
					SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
				}
				return;
			}
		}
		FileDlgRefresh = SendVar->ByteCount;
		SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
		BCOld = SendVar->ByteCount;
		if (fc != 0)
			return;
	} while (fc != 0);

	FileSendEnd();
}

void FileSend(void)
{
	WORD c, fc;
	LONG BCOld;
	PFileVar fv = SendVar;

	if (cv.PortType == IdSerial && ts.FileSendHighSpeedMode &&
	    BinaryMode && FileBracketMode == FS_BRACKET_NONE && !cv.TelFlag &&
	    (ts.LocalEcho == 0) && (ts.Baud >= 115200)) {
		return FileSendBinayBoost();
	}

	if ((SendDlg == NULL) || (fv->FilePause == TRUE))
		return;

	BCOld = SendVar->ByteCount;

	if (FileRetrySend)
	{
		FileRetryEcho = (ts.LocalEcho>0);
		c = FSOut1(FileByte);
		FileRetrySend = (c==0);
		if (FileRetrySend)
			return;
	}

	if (FileRetryEcho)
	{
		c = FSEcho1(FileByte);
		FileRetryEcho = (c==0);
		if (FileRetryEcho)
			return;
	}

	do {
		if (FileBracketMode == FS_BRACKET_START) {
			FileByte = BracketStartStr[FileBracketPtr++];
			fc = 1;

			if (FileBracketPtr >= sizeof(BracketStartStr) - 1) {
				FileBracketMode = FS_BRACKET_END;
				FileBracketPtr = 0;
				BinaryMode = ts.TransBin;
			}
		}
		else if (! FileReadEOF) {
			DWORD read_bytes;
			ReadFile(SendVar->FileHandle, &FileByte, 1, &read_bytes, NULL);
			fc = LOWORD(read_bytes);
			SendVar->ByteCount = SendVar->ByteCount + fc;

			if (FileCRSend && (fc==1) && (FileByte==0x0A)) {
				ReadFile(SendVar->FileHandle, &FileByte, 1, &read_bytes, NULL);
				fc = LOWORD(read_bytes);
				SendVar->ByteCount = SendVar->ByteCount + fc;
			}
		}
		else {
			fc = 0;
		}

		if (fc == 0 && FileBracketMode == FS_BRACKET_END) {
			FileReadEOF = TRUE;
			FileByte = BracketEndStr[FileBracketPtr++];
			fc = 1;
			BinaryMode = TRUE;

			if (FileBracketPtr >= sizeof(BracketEndStr) - 1) {
				FileBracketMode = FS_BRACKET_NONE;
				FileBracketPtr = 0;
			}
		}


		if (fc!=0)
		{
			c = FSOut1(FileByte);
			FileCRSend = (ts.TransBin==0) && (FileByte==0x0D);
			FileRetrySend = (c==0);
			if (FileRetrySend)
			{
				if (SendVar->ByteCount != BCOld) {
					SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
				}
				return;
			}
			if (ts.LocalEcho>0)
			{
				c = FSEcho1(FileByte);
				FileRetryEcho = (c==0);
				if (FileRetryEcho)
					return;
			}
		}
		if ((fc==0) || ((SendVar->ByteCount % 100 == 0) && (FileBracketPtr == 0))) {
			SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
			BCOld = SendVar->ByteCount;
			if (fc!=0)
				return;
		}
	} while (fc!=0);

	FileSendEnd();
}

BOOL IsSendVarNULL()
{
	return SendVar == NULL;
}

/**
 * TODO: IsSendVarNULL() との違いは?
 */
BOOL FileSnedIsSending(void)
{
	return FSend;
}
