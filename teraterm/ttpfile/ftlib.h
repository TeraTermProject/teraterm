/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTFILE.DLL, routines for file transfer protocol */

extern char UILanguageFile[MAX_PATH];

void GetLongFName(PCHAR FullName, PCHAR LongName, int destlen);
void FTConvFName(PCHAR FName);
BOOL GetNextFname(PFileVar fv);
WORD UpdateCRC(BYTE b, WORD CRC);
LONG UpdateCRC32(BYTE b, LONG CRC);
void FTLog1Byte(PFileVar fv, BYTE b);
void FTSetTimeOut(PFileVar fv, int T);
BOOL FTCreateFile(PFileVar fv);
void GetFileSendFilter(PCHAR dest, PCHAR src, int size);

#ifdef __cplusplus
extern "C" {
#endif
void FAR PASCAL TTFILESetUILanguageFile(char *file);
void FAR PASCAL TTFILESetFileSendFilter(char *file);
#ifdef __cplusplus
}
#endif
