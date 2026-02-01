/*
 * (C) 2025- TeraTerm Project
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

/* Raw protocol */

#include "tttypes.h"
#include "filesys_proto.h"
#include "filesys.h"		// for ProtoEnd()

#include "raw.h"

typedef struct {
	const char *FullName;	// Windows上のファイル名 UTF-8
	BOOL FileOpen;
	LONG ByteCount;
	DWORD StartTime;
	PComVar cv;
	enum {
		STATE_FLUSH,		// 処理開始時に受信データを捨てる
		STATE_NORMAL,
		STATE_CANCELED,		// キャンセル通知を受けた
	} state;
	TComm *Comm;
	PFileVarProto fv;
	TFileIO *file;
	int AutoStopSec;		// 自動停止待ち時間(秒)
	int SecLeft;
} TRawVar;
typedef TRawVar *PRawVar;

static int RawRead1Byte(PRawVar rv, LPBYTE b)
{
	if (rv->Comm->op->Read1Byte(rv->Comm, b) == 0) {
		return 0;
	}
	return 1;
}

static void RawFlushReceiveBuf(PRawVar rv)
{
	BYTE b;
	int r;

	do {
		r = RawRead1Byte(rv, &b);
	} while (r != 0);
}

static BOOL RawInit(TProto *pv, PComVar cv, PTTSet ts)
{
	PRawVar rv = pv->PrivateData;
	PFileVarProto fv = rv->fv;
	TFileIO *file = rv->file;

	rv->FullName = fv->GetNextFname(fv);
	rv->FileOpen = file->OpenWrite(file, rv->FullName);
	if (rv->FileOpen == FALSE) {
		if (fv->NoMsg == FALSE) {
			static const TTMessageBoxInfoW mbinfo = {
				"Tera Term",
				"MSG_TT_ERROR", L"Tera Term: Error",
				"MSG_CANTOPEN_FILE_ERROR", L"Cannot open file",
				MB_TASKMODAL | MB_ICONEXCLAMATION };
			TTMessageBoxW(fv->HMainWin, &mbinfo, ts->UILanguageFileW);
		}
		return FALSE;
	}
	fv->InfoOp->SetDlgProtoFileName(fv, rv->FullName);
	fv->InfoOp->SetDlgProtoText(fv, "Raw(binary)");
	rv->StartTime = 0;
	rv->ByteCount = 0;
	rv->state = STATE_FLUSH;
	rv->cv = cv;

	return TRUE;
}

static void RawCancel(TProto *pv)
{
	PRawVar rv = pv->PrivateData;
	PComVar cv = rv->cv;
	TFileIO *file = rv->file;

	rv->state = STATE_CANCELED;	// quit(cancel)
	file->Close(file);
	if (! cv->Ready) {
		// セッション断の場合は RawParse() が呼ばれないため、直接 ProtoEnd() を呼ぶ
		RawFlushReceiveBuf(rv);
		ProtoEnd();
	}
}

static void RawTimeOutProc(TProto *pv)
{
	PRawVar rv = pv->PrivateData;
	PComVar cv = rv->cv;
	TFileIO *file = rv->file;
	PFileVarProto fv = rv->fv;

	rv->SecLeft -= 1;
	if (rv->AutoStopSec > 0 && rv->SecLeft == 0) {
		rv->state = STATE_CANCELED;	// quit(autostop)
		file->Close(file);
		if (! cv->Ready) {
			// セッション断の場合は RawParse() が呼ばれないため、直接 ProtoEnd() を呼ぶ
			RawFlushReceiveBuf(rv);
			ProtoEnd();
		} else {
			fv->Success = TRUE;
		}
	}
	else {
		fv->InfoOp->SetDlgByteCount(fv, rv->ByteCount);
		fv->FTSetTimeOut(fv, 1);
	}
}

static BOOL RawReadPacket(void *arg)
{
	PRawVar rv = (PRawVar)arg;
	TFileIO *file = rv->file;
	PFileVarProto fv = rv->fv;
	PComVar cv = rv->cv;
	BYTE Buff[InBuffSize];
	int BuffCount;
	static DWORD prev_elapsed;
	DWORD elapsed;

	if (cv->InBuffCount > 0) {
		if (rv->StartTime == 0) {
			rv->StartTime = GetTickCount();
			prev_elapsed = rv->StartTime;
			fv->InfoOp->SetDlgPacketNum(fv, 1);
		}
		// クリティカルセクションの時間を短くするため memcpy() してから WriteFile()する
		EnterCriticalSection(&cv->InBuff_lock);
		memcpy(Buff, &(cv->InBuff[cv->InPtr]), cv->InBuffCount);
		BuffCount = cv->InBuffCount;
		cv->InBuffCount = 0;
		cv->InPtr = 0;
		LeaveCriticalSection(&cv->InBuff_lock);
		file->WriteFile(file, Buff, BuffCount);
		rv->ByteCount += BuffCount;
		elapsed = (GetTickCount() - prev_elapsed) / 250;
		if (elapsed != prev_elapsed) {
			prev_elapsed = elapsed;
			fv->InfoOp->SetDlgByteCount(fv, rv->ByteCount);
			fv->InfoOp->SetDlgTime(fv, rv->StartTime, rv->ByteCount);
		}
		rv->SecLeft = rv->AutoStopSec;
		fv->FTSetTimeOut(fv, 1);
	}
	if (rv->state == STATE_CANCELED) {
		return FALSE;
	}
	return TRUE;
}

static BOOL RawParse(TProto *pv)
{
	PRawVar rv = pv->PrivateData;

	switch (rv->state) {
	case STATE_FLUSH:
		RawFlushReceiveBuf(rv);		// 受信データを捨てる
		rv->state = STATE_NORMAL;	// 送受信処理を行う
		return TRUE;
	case STATE_NORMAL:
		return RawReadPacket(rv);
	case STATE_CANCELED:
		RawFlushReceiveBuf(rv);
		return FALSE;
	}
	return FALSE;
}

static int SetOptV(TProto *pv, int request, va_list ap)
{
	PRawVar rv = pv->PrivateData;
	switch(request) {
	case RAW_AUTOSTOP_SEC: {
		const int autostop_sec = va_arg(ap, int);
		rv->AutoStopSec = autostop_sec;
		return 0;
	}
	}
	return -1;
}

static int SetOpt(TProto *pv, int request, ...)
{
	va_list ap;
	va_start(ap, request);
	int r = SetOptV(pv, request, ap);
	va_end(ap);
	return r;
}

static void Destroy(TProto *pv)
{
	PRawVar rv = pv->PrivateData;
	free((void*)rv->FullName);
	rv->FullName = NULL;
	free(rv);
	pv->PrivateData = NULL;
	free(pv);
}

static const TProtoOp Op = {
	RawInit,
	RawParse,
	RawTimeOutProc,
	RawCancel,
	SetOpt,
	SetOptV,
	Destroy,
};

TProto *RawCreate(PFileVarProto fv)
{
	TProto *pv = malloc(sizeof(*pv));
	if (pv == NULL) {
		return NULL;
	}
	pv->Op = &Op;
	PRawVar rv = malloc(sizeof(TRawVar));
	pv->PrivateData = rv;
	if (rv == NULL) {
		free(pv);
		return NULL;
	}
	memset(rv, 0, sizeof(*rv));
	rv->FileOpen = FALSE;
	rv->state = STATE_FLUSH;
	rv->Comm = fv->Comm;
	rv->file = fv->file_fv;
	rv->fv = fv;

	return pv;
}
