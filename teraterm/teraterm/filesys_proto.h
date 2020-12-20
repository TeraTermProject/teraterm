/*
 * (C) 2020 TeraTerm Project
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

#pragma once

#include "filesys_io.h"

typedef struct FileVarProto {
	// ↓protosys_proto.cpp内のみ使用
	WORD OpId;

	HWND HMainWin;
	HWND HWin;
	wchar_t *DlgCaption;

	// 送信ファイル名配列
	//	フルパスのファイル名配列(一番最後はNULL)
	char **FileNames;
	int FNCount;		// 送信中ファイル名配列index(0...)

	// 受信
	char *RecievePath;		// 受信フォルダ(終端にパスセパレータ'\\'が付加されている)

	// ↑protosys_proto.cpp内のみ使用

	// ↓各プロトコルで使用するワーク
	BOOL FileOpen;
	LONG FileSize, ByteCount;
	BOOL OverWrite;

	BOOL Success;
	BOOL NoMsg;

	BOOL HideDialog;

	int ProgStat;

	DWORD StartTime;

	DWORD FileMtime;
	// ↑各プロトコルで使用するワーク

	// services
	char *(*GetNextFname)(struct FileVarProto *fv);
	char *(*GetRecievePath)(struct FileVarProto *fv);
	void (*FTSetTimeOut)(struct FileVarProto *fv, int T);
	void (*SetDialogCation)(struct FileVarProto *fv, const char *key, const wchar_t *default_caption);

	// protocol entrys, data
	BOOL (*Init)(struct FileVarProto *fv, PComVar cv, PTTSet ts);
	BOOL (*Parse)(struct FileVarProto *fv, PComVar cv);
	void (*TimeOutProc)(struct FileVarProto *fv, PComVar cv);
	void (*Cancel)(struct FileVarProto *fv, PComVar cv);
	int (*SetOptV)(struct FileVarProto *fv, int request, va_list ap);
	void (*Destroy)(struct FileVarProto *fv);
	void *data;

	// UI
	void (*InitDlgProgress)(struct FileVarProto *fv, int *CurProgStat);
	void (*SetDlgTime)(struct FileVarProto *fv, DWORD elapsed, int bytes);
	void (*SetDlgPaketNum)(struct FileVarProto *fv, LONG Num);
	void (*SetDlgByteCount)(struct FileVarProto *fv, LONG Num);
	void (*SetDlgPercent)(struct FileVarProto *fv, LONG a, LONG b, int *p);
	void (*SetDlgProtoText)(struct FileVarProto *fv, const char *text);
	void (*SetDlgProtoFileName)(struct FileVarProto *fv, const char *text);

	TFileIO *file;
} TFileVarProto;
typedef TFileVarProto *PFileVarProto;
