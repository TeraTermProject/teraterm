/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, file transfer routines */
#ifdef __cplusplus
extern "C" {
#endif
typedef BOOL (FAR PASCAL *PGetSetupFname)
  (HWND HWin, WORD FuncId, PTTSet ts);
typedef BOOL (FAR PASCAL *PGetTransFname)
  (PFileVar fv, PCHAR CurDir, WORD FuncId, LPLONG Option);
typedef BOOL (FAR PASCAL *PGetMultiFname)
  (PFileVar fv, PCHAR CurDir, WORD FuncId, LPWORD Option);
typedef BOOL (FAR PASCAL *PGetGetFname)
  (HWND HWin, PFileVar fv);
typedef void (FAR PASCAL *PSetFileVar) (PFileVar fv);
typedef BOOL (FAR PASCAL *PGetXFname)
  (HWND HWin, BOOL Receive, LPLONG Option, PFileVar fv, PCHAR CurDir);
typedef void (FAR PASCAL *PProtoInit)
  (int Proto, PFileVar fv, PCHAR pv, PComVar cv, PTTSet ts);
typedef BOOL (FAR PASCAL *PProtoParse)
  (int Proto, PFileVar fv, PCHAR pv, PComVar cv);
typedef void (FAR PASCAL *PProtoTimeOutProc)
  (int Proto, PFileVar fv, PCHAR pv, PComVar cv);
typedef BOOL (FAR PASCAL *PProtoCancel)
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

