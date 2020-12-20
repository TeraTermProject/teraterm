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

typedef struct FileVarProto {
	HWND HMainWin;
	HWND HWin;
	WORD OpId;
	char DlgCaption[40];

	char FullName[MAX_PATH];
	int DirLen;

	int NumFname, FNCount;
	HANDLE FnStrMemHandle;
	PCHAR FnStrMem;
	int FnPtr;

	BOOL FileOpen;
	HANDLE FileHandle;
	LONG FileSize, ByteCount;
	BOOL OverWrite;

	BOOL LogFlag;
	HANDLE LogFile;
	WORD LogState;
	WORD LogCount;

	BOOL Success;
	BOOL NoMsg;

	char LogDefaultPath[MAX_PATH];
	BOOL HideDialog;

	BYTE LogLineBuf[16];
	int FlushLogLineBuf;

	int ProgStat;

	DWORD StartTime;

	DWORD FileMtime;

	// protocol entrys, data
	void (*Init)(struct FileVarProto *fv, PComVar cv, PTTSet ts);
	BOOL (*Parse)(struct FileVarProto *fv, PComVar cv);
	void (*TimeOutProc)(struct FileVarProto *fv, PComVar cv);
	void (*Cancel)(struct FileVarProto *fv, PComVar cv);
	int (*SetOptV)(struct FileVarProto *fv, int request, va_list ap);
	void *data;

	// file I/O
	size_t (*ReadFile)(struct FileVarProto *fv, void *buf, size_t bytes);
	size_t (*WriteFile)(struct FileVarProto *fv, const void *buf, size_t bytes);
	void (*Close)(struct FileVarProto *fv);
	size_t (*GetFSize)(struct FileVarProto *fv, const char *filenameU8);

	// UI
	void (*InitDlgProgress)(struct FileVarProto *fv, int *CurProgStat);
	void (*SetDlgTime)(struct FileVarProto *fv, DWORD elapsed, int bytes);
	void (*SetDlgPaketNum)(struct FileVarProto *fv, LONG Num);
	void (*SetDlgByteCount)(struct FileVarProto *fv, LONG Num);
	void (*SetDlgPercent)(struct FileVarProto *fv, LONG a, LONG b, int *p);
	void (*SetDlgProtoText)(struct FileVarProto *fv, const char *text);
	void (*SetDlgProtoFileName)(struct FileVarProto *fv, const char *text);
} TFileVarProto;
typedef TFileVarProto *PFileVarProto;
