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
#include "asprintf.h"
#include "codeconv.h"

#include "filesys.h"

typedef enum {
	FS_BRACKET_NONE,
	FS_BRACKET_START,
	FS_BRACKET_END,
} FileBracketMode_t;

#define FILE_SEND_BUF_SIZE  8192
struct FileSendHandler {
	CHAR buf[FILE_SEND_BUF_SIZE];
	int pos;
	int end;
};

typedef struct {
	wchar_t *FullName;

	HANDLE FileHandle;
	LONG FileSize; // uint64_t FileSize;  TODO
	LONG ByteCount; // uint64_t ByteCount;

	DWORD StartTime;
	BOOL FilePause;

	BOOL FileRetrySend, FileRetryEcho, FileCRSend, FileReadEOF, BinaryMode;

	FileBracketMode_t FileBracketMode;
	int FileBracketPtr;

	PFileTransDlg SendDlg;
	int FileDlgRefresh;		// ?

	wchar_t SendChar;

	struct FileSendHandler FileSendHandler;
} TFileVar;
typedef TFileVar *PFileVar;

static PFileVar SendVar = NULL;
static BOOL FSend = FALSE;

static const char BracketStartStr[] = "\033[200~";
static const char BracketEndStr[] = "\033[201~";

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
	fv->SendDlg = FTDlg; /* File send */

	return TRUE;
}

#if 0
static void ShowFTDlg(WORD OpId)
{
	if (fv->SendDlg != NULL) {
		fv->SendDlg->ShowWindow(SW_SHOWNORMAL);
		SetForegroundWindow(fv->SendDlg->GetSafeHwnd());
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

	SendVar->FileHandle = CreateFileW(SendVar->FullName, GENERIC_READ, FILE_SHARE_READ, NULL,
									   OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (SendVar->FileHandle == INVALID_HANDLE_VALUE) {
		FileSendEnd();
		return FALSE;
	}
	SendVar->ByteCount = 0;
	SendVar->FileSize = GetFSize64H(SendVar->FileHandle);

	TalkStatus = IdTalkFile;
	fv->FileRetrySend = FALSE;
	fv->FileRetryEcho = FALSE;
	fv->FileCRSend = FALSE;
	fv->FileReadEOF = FALSE;
	fv->FileSendHandler.pos = 0;
	fv->FileSendHandler.end = 0;
	fv->FileDlgRefresh = 0;

	if (BracketedPasteMode()) {
		fv->FileBracketMode = FS_BRACKET_START;
		fv->FileBracketPtr = 0;
		fv->BinaryMode = TRUE;
	}
	else {
		fv->FileBracketMode = FS_BRACKET_NONE;
		fv->BinaryMode = ts.TransBin;
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
		PFileVar fv = SendVar;
		FSend = FALSE;
		TalkStatus = IdTalkKeyb;
		if (fv->SendDlg!=NULL)
		{
			fv->SendDlg->DestroyWindow();
			fv->SendDlg = NULL;
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

static int FSOut1(PFileVar fv)
{
	if (fv->BinaryMode) {
		BYTE b = (BYTE)fv->SendChar;
		return CommBinaryOut(&cv, (PCHAR)&b, 1);
	}
	else {
		wchar_t wc = fv->SendChar;
		if ((wc >= 0x20) || (wc == 0x09) || (wc == 0x0A) || (wc == 0x0D)) {
			return CommTextOutW(&cv, &wc, 1);
		}
		else {
			return 1;
		}
	}
}

static int FSEcho1(PFileVar fv)
{
	if (fv->BinaryMode) {
		BYTE b = (BYTE)fv->SendChar;
		return CommBinaryEcho(&cv,(PCHAR)&b,1);
	} else {
		return CommTextEchoW(&cv, &fv->SendChar, 1);
	}
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

	if ((fv->SendDlg == NULL) || (fv->FilePause == TRUE))
		return;

	BCOld = SendVar->ByteCount;

	if (fv->FileRetrySend)
	{
		c = CommRawOut(&cv, &(fv->FileSendHandler.buf[fv->FileSendHandler.pos]),
			fv->FileSendHandler.end - fv->FileSendHandler.pos);
		fv->FileSendHandler.pos += c;
		fv->FileRetrySend = (fv->FileSendHandler.end != fv->FileSendHandler.pos);
		if (fv->FileRetrySend)
			return;
	}

	do {
		if (fv->FileSendHandler.pos == fv->FileSendHandler.end) {
			ReadFile(SendVar->FileHandle, &(fv->FileSendHandler.buf[0]), sizeof(fv->FileSendHandler.buf), &read_bytes, NULL);
			fc = LOWORD(read_bytes);
			fv->FileSendHandler.pos = 0;
			fv->FileSendHandler.end = fc;
		} else {
			fc = fv->FileSendHandler.end - fv->FileSendHandler.end;
		}

		if (fc != 0)
		{
			c = CommRawOut(&cv, &(fv->FileSendHandler.buf[fv->FileSendHandler.pos]),
				fv->FileSendHandler.end - fv->FileSendHandler.pos);
			fv->FileSendHandler.pos += c;
			fv->FileRetrySend = (fv->FileSendHandler.end != fv->FileSendHandler.pos);
			SendVar->ByteCount = SendVar->ByteCount + c;
			if (fv->FileRetrySend)
			{
				if (SendVar->ByteCount != BCOld) {
					fv->SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
				}
				return;
			}
		}
		fv->FileDlgRefresh = SendVar->ByteCount;
		fv->SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
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
		fv->BinaryMode && fv->FileBracketMode == FS_BRACKET_NONE && !cv.TelFlag &&
	    (ts.LocalEcho == 0) && (ts.Baud >= 115200)) {
		return FileSendBinayBoost();
	}

	if ((fv->SendDlg == NULL) || (fv->FilePause == TRUE))
		return;

	BCOld = SendVar->ByteCount;

	if (fv->FileRetrySend)
	{
		fv->FileRetryEcho = (ts.LocalEcho>0);
		c = FSOut1(fv);
		fv->FileRetrySend = (c==0);
		if (fv->FileRetrySend)
			return;
	}

	if (fv->FileRetryEcho)
	{
		c = FSEcho1(fv);
		fv->FileRetryEcho = (c==0);
		if (fv->FileRetryEcho)
			return;
	}

	do {
		if (fv->FileBracketMode == FS_BRACKET_START) {
			fv->SendChar = BracketStartStr[fv->FileBracketPtr++];
			fc = 1;

			if (fv->FileBracketPtr >= sizeof(BracketStartStr) - 1) {
				fv->FileBracketMode = FS_BRACKET_END;
				fv->FileBracketPtr = 0;
				fv->BinaryMode = ts.TransBin;
			}
		}
		else if (!fv->FileReadEOF) {
			DWORD read_bytes;
			BYTE FileByte;
			ReadFile(SendVar->FileHandle, &FileByte, 1, &read_bytes, NULL);
			fc = LOWORD(read_bytes);
			SendVar->ByteCount = SendVar->ByteCount + fc;

			if (FileByte==0x0A && fc == 1 && fv->FileCRSend) {
				// CR を送った直後 0x0A だった -> ファイルから1バイト読む
				ReadFile(SendVar->FileHandle, &FileByte, 1, &read_bytes, NULL);
				fc = LOWORD(read_bytes);
				SendVar->ByteCount = SendVar->ByteCount + fc;
			}

			if (IsDBCSLeadByte(FileByte)) {
				// DBCSの1byte目だった -> ファイルから1バイト読んで Unicode とする
				char dbcs[2];
				dbcs[0] = FileByte;

				ReadFile(SendVar->FileHandle, &dbcs[1], 1, &read_bytes ,NULL);
				fc = LOWORD(read_bytes);
				SendVar->ByteCount = SendVar->ByteCount + fc;

				unsigned int u32;
				MBCPToUTF32(dbcs, 2, CP_ACP, &u32);
				fv->SendChar = u32;
			} else {
				fv->SendChar = FileByte;
			}
		}
		else {
			fc = 0;
		}

		if (fc == 0 && fv->FileBracketMode == FS_BRACKET_END) {
			fv->FileReadEOF = TRUE;
			fv->SendChar = BracketEndStr[fv->FileBracketPtr++];
			fc = 1;
			fv->BinaryMode = TRUE;

			if (fv->FileBracketPtr >= sizeof(BracketEndStr) - 1) {
				fv->FileBracketMode = FS_BRACKET_NONE;
				fv->FileBracketPtr = 0;
			}
		}


		if (fc!=0)
		{
			c = FSOut1(fv);
			fv->FileCRSend = (ts.TransBin == 0) && (fv->SendChar == 0x0D);
			fv->FileRetrySend = (c == 0);
			if (fv->FileRetrySend)
			{
				if (SendVar->ByteCount != BCOld) {
					fv->SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
				}
				return;
			}
			if (ts.LocalEcho>0)
			{
				c = FSEcho1(fv);
				fv->FileRetryEcho = (c==0);
				if (fv->FileRetryEcho)
					return;
			}
		}
		if ((fc==0) || ((SendVar->ByteCount % 100 == 0) && (fv->FileBracketPtr == 0))) {
			fv->SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
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
