/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, Communication routines */
#ifdef __cplusplus
extern "C" {
#endif

void CommInit(PComVar cv);
void CommOpen(HWND HW, PTTSet ts, PComVar cv);
#ifndef NO_I18N
void CommStart(PComVar cv, LONG lParam, PTTSet ts);
#else
void CommStart(PComVar cv, LONG lParam);
#endif
BOOL CommCanClose(PComVar cv);
void CommClose(PComVar cv);
void CommProcRRQ(PComVar cv);
void CommReceive(PComVar cv);
void CommSend(PComVar cv);
void CommSendBreak(PComVar cv);
void CommResetSerial(PTTSet ts, PComVar cv, BOOL ClearBuffer);
void CommLock(PTTSet ts, PComVar cv, BOOL Lock);
int GetCommSerialBaudRate(int id);
BOOL PrnOpen(PCHAR DevName);
int PrnWrite(PCHAR b, int c);
void PrnCancel();
void PrnClose();

extern BOOL TCPIPClosed;

#ifdef __cplusplus
}
#endif
