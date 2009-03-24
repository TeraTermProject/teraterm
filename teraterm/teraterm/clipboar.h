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
void CBStartPaste(HWND HWin, BOOL AddCR,
		  int BuffSize, PCHAR DataPtr, int DataSize);
void CBSend();
void CBEndPaste();
int CBStartPasteConfirmChange(HWND HWin);

#ifdef __cplusplus
}
#endif
