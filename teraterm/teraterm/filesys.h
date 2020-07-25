/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2020 TeraTerm Project
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
#ifdef __cplusplus
extern "C" {
#endif
typedef BOOL (PASCAL *PGetSetupFname)
  (HWND HWin, WORD FuncId, PTTSet ts);
typedef BOOL (PASCAL *PGetTransFname)
  (PFileVar fv, PCHAR CurDir, WORD FuncId, LPLONG Option);
typedef BOOL (PASCAL *PGetMultiFname)
  (PFileVar fv, PCHAR CurDir, WORD FuncId, LPWORD Option);
typedef BOOL (PASCAL *PGetGetFname)
  (HWND HWin, PFileVar fv);
typedef void (PASCAL *PSetFileVar) (PFileVar fv);
typedef BOOL (PASCAL *PGetXFname)
  (HWND HWin, BOOL Receive, LPLONG Option, PFileVar fv, PCHAR CurDir);
typedef void (PASCAL *PProtoInit)
  (int Proto, PFileVar fv, PCHAR pv, PComVar cv, PTTSet ts);
typedef BOOL (PASCAL *PProtoParse)
  (int Proto, PFileVar fv, PCHAR pv, PComVar cv);
typedef void (PASCAL *PProtoTimeOutProc)
  (int Proto, PFileVar fv, PCHAR pv, PComVar cv);
typedef BOOL (PASCAL *PProtoCancel)
  (int Proto, PFileVar fv, PCHAR pv, PComVar cv);
typedef BOOL (PASCAL *PTTFILESetUILanguageFile)
  (char *file);
typedef BOOL (PASCAL *PTTFILESetFileSendFilter)
  (char *file);

extern PGetSetupFname GetSetupFname;
//extern PGetTransFname GetTransFname;
extern PGetMultiFname GetMultiFname;
extern PGetGetFname GetGetFname;
extern PSetFileVar SetFileVar;
extern PGetXFname GetXFname;
extern PProtoInit ProtoInit;
//extern PProtoParse ProtoParse;
extern PProtoTimeOutProc ProtoTimeOutProc;
extern PProtoCancel ProtoCancel;
extern PTTFILESetUILanguageFile TTFILESetUILanguageFile;
extern PTTFILESetFileSendFilter TTFILESetFileSendFilter;

BOOL LoadTTFILE(void);
BOOL FreeTTFILE(void);
//void ShowFTDlg(WORD OpId);
BOOL NewFileVar(PFileVar *FV);
void FreeFileVar(PFileVar *FV);

void LogPut1(BYTE b);
void LogToFile(void);
BOOL CreateLogBuf(void);
void FreeLogBuf(void);
BOOL CreateBinBuf(void);
void FreeBinBuf(void);

void FileSendStart(void);
void FileSend(void);
//void FLogChangeButton(BOOL Pause);
void FLogPause(BOOL Pause);
void FileTransEnd(WORD OpId);
void FileTransPause(WORD OpId, BOOL Pause);

void ProtoEnd(void);
int ProtoDlgParse(void);
void ProtoDlgTimeOut(void);
void ProtoDlgCancel(void);
void KermitStart(int mode);
void XMODEMStart(int mode);
void YMODEMStart(int mode);
void ZMODEMStart(int mode);
void BPStart(int mode);
void QVStart(int mode);

extern PFileVar SendVar, FileVar;
extern BOOL FileLog, BinLog;

// log
typedef struct {
	wchar_t *filename;		// [in] ファイル名初期値(NULL=default) [out] 入力ファイル名、free()すること
	BOOL append;			// TRUE/FALSE = append/new(overwrite)
	BOOL bom;				// TRUE = BOMあり
	int code;				// 0/1/2 = UTF-8/UTF-16LE/UTF-16BE
} FLogDlgInfo_t;
void logfile_lock_initialize(void);
void FLogRotateSize(size_t size);
void FLogRotateRotate(int step);
void FLogRotateHalt(void);
void FLogAddCommentDlg(HINSTANCE hInst, HWND hWnd);
void FLogClose(void);
BOOL FLogOpen(const char *fname);
BOOL FLogIsOpend(void);
void FLogWriteStr(const char *str);
void FLogInfo(char *param_ptr, size_t param_len);
const char *FLogGetFilename(void);
BOOL FLogOpenDialog(HINSTANCE hInst, HWND hWnd, FLogDlgInfo_t *info);
char *FLogGetLogFilename(const char *log_filename);
BOOL FLogIsPause(void);
void FLogWindow(int nCmdShow);
void FLogShowDlg(void);

#ifdef __cplusplus
}
#endif
