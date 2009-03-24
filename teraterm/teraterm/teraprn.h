/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, Printing routines */

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
HDC PrnBox(HWND HWin, PBOOL Sel);
BOOL PrnStart(LPSTR DocumentName);
void PrnStop();

#define IdPrnCancel 0
#define IdPrnScreen 1
#define IdPrnSelectedText 2
#define IdPrnScrollRegion 4
#define IdPrnFile 8

int VTPrintInit(int PrnFlag);
void PrnSetAttr(TCharAttr Attr);
void PrnOutText(PCHAR Buff, int Count);
void PrnNewLine();
void VTPrintEnd();

void PrnFileDirectProc();
void PrnFileStart();
void OpenPrnFile();
void ClosePrnFile();
void WriteToPrnFile(BYTE b, BOOL Write);

#ifdef __cplusplus
}
#endif

