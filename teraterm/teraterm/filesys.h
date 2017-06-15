/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
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

extern PGetSetupFname GetSetupFname;
extern PGetTransFname GetTransFname;
extern PGetMultiFname GetMultiFname;
extern PGetGetFname GetGetFname;
extern PSetFileVar SetFileVar;
extern PGetXFname GetXFname;
extern PProtoInit ProtoInit;
extern PProtoParse ProtoParse;
extern PProtoTimeOutProc ProtoTimeOutProc;
extern PProtoCancel ProtoCancel;

BOOL LoadTTFILE();
BOOL FreeTTFILE();
void ShowFTDlg(WORD OpId);
BOOL NewFileVar(PFileVar *FV);
void FreeFileVar(PFileVar *FV);

BOOL LogStart();
void Log1Byte(BYTE b);
void LogToFile();
BOOL CreateLogBuf();
void FreeLogBuf();
BOOL CreateBinBuf();
void FreeBinBuf();

void FileSendStart();
void FileSend();
void FLogChangeButton(BOOL Pause);
void FLogRefreshNum();
void FileTransEnd(WORD OpId);

void ProtoEnd();
int ProtoDlgParse();
void ProtoDlgTimeOut();
void ProtoDlgCancel();
void KermitStart(int mode);
void XMODEMStart(int mode);
void YMODEMStart(int mode);
void ZMODEMStart(int mode);
void BPStart(int mode);
void QVStart(int mode);

extern PFileVar LogVar, SendVar, FileVar;
extern BOOL FileLog, BinLog, DDELog;

void logfile_lock_initialize(void);
void CommentLogToFile(char *buf, int size);

#ifdef __cplusplus
}
#endif

