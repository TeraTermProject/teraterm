/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, Clipboard routines */

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
PCHAR CBOpen(LONG MemSize);
void CBClose();
void CBStartSend(PCHAR DataPtr, int DataSize, BOOL EchoOnly);
void CBStartPaste(HWND HWin, BOOL AddCR, BOOL Bracketed);
void CBStartPasteB64(HWND HWin, PCHAR header, PCHAR footer);
void CBSend();
void CBEcho();
void CBEndPaste();
int CBStartPasteConfirmChange(HWND HWin, BOOL AddCR);

#ifdef __cplusplus
}
#endif
