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

/* TERATERM.EXE, log routines */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * ログバッファの最低空きサイズ
 *	"[YYYY-MM-DD HH:MM:SS.000]" の文字が入る程度
 */
#define FILESYS_LOG_FREE_SPACE	(30*2)

// log
typedef struct {
	wchar_t *filename;		// [in] ファイル名初期値(NULL=default) [out] 入力ファイル名、free()すること
	BOOL append;			// TRUE/FALSE = append/new(overwrite)
	BOOL bom;				// TRUE = BOMあり
	int code;				// 0/1/2 = UTF-8/UTF-16LE/UTF-16BE
} FLogDlgInfo_t;
BOOL FLogOpenDialog(HINSTANCE hInst, HWND hWnd, FLogDlgInfo_t *info);
void FLogAddCommentDlg(HINSTANCE hInst, HWND hWnd);
wchar_t *FLogGetLogFilename(const wchar_t *log_filename);

void logfile_lock_initialize(void);
void FLogPause(BOOL Pause);
void FLogRotateSize(size_t size);
void FLogRotateRotate(int step);
void FLogRotateHalt(void);
void FLogClose(void);
BOOL FLogOpen(const wchar_t *fname);
BOOL FLogIsOpend(void);
BOOL FLogIsOpendText(void);
BOOL FLogIsOpendBin(void);
void FLogWriteStr(const wchar_t *str);
void FLogInfo(char *param_ptr, size_t param_len);
const wchar_t *FLogGetFilename(void);
BOOL FLogIsPause(void);
void FLogWindow(int nCmdShow);
void FLogShowDlg(void);
int FLogGetCount(void);
int FLogGetFreeCount(void);
void FLogWriteFile(void);
void FLogPutUTF32(unsigned int u32);
void FLogSetCode(int code);
void FLogOutputBOM(void);
//void LogPut1(BYTE b);
void FLogOutputAllBuffer(void);

#ifdef __cplusplus
}
#endif
