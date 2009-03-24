/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* useful routines */

#ifdef __cplusplus
extern "C" {
#endif

BOOL GetFileNamePos(PCHAR PathName, int far *DirLen, int far *FNPos);
BOOL ExtractFileName(PCHAR PathName, PCHAR FileName, int destlen);
BOOL ExtractDirName(PCHAR PathName, PCHAR DirName);
void FitFileName(PCHAR FileName, int destlen, PCHAR DefExt);
void AppendSlash(PCHAR Path, int destlen);
void Str2Hex(PCHAR Str, PCHAR Hex, int Len, int MaxHexLen, BOOL ConvSP);
BYTE ConvHexChar(BYTE b);
int Hex2Str(PCHAR Hex, PCHAR Str, int MaxLen);
BOOL DoesFileExist(PCHAR FName);
long GetFSize(PCHAR FName);
void uint2str(UINT i, PCHAR Str, int destlen, int len);
#ifdef WIN32
void QuoteFName(PCHAR FName);
#endif
int isInvalidFileNameChar(PCHAR FName);
void deleteInvalidFileNameChar(PCHAR FName);
int isInvalidStrftimeChar(PCHAR FName);
void deleteInvalidStrftimeChar(PCHAR FName);
void ParseStrftimeFileName(PCHAR FName, int destlen);
void ConvFName(PCHAR HomeDir, PCHAR Temp, int templen, PCHAR DefExt, PCHAR FName, int destlen);
void RestoreNewLine(PCHAR Text);
void GetNthString(PCHAR Source, int Nth, int Size, PCHAR Dest);
void GetNthNum(PCHAR Source, int Nth, int far *Num);
void WINAPI GetDefaultFName(char *home, char *file, char *dest, int destlen);
void GetDefaultSetupFName(char *home, char *dest, int destlen);
void GetUILanguageFile(char *buf, int buflen);
void GetOnOffEntryInifile(char *entry, char *buf, int buflen);
void get_lang_msg(PCHAR key, PCHAR buf, int buf_len, PCHAR def, PCHAR iniFile);
int get_lang_font(PCHAR key, HWND dlg, PLOGFONT logfont, HFONT *font, PCHAR iniFile);
void doSelectFolder(HWND hWnd, char *path, int pathlen, char *msg);
void OutputDebugPrintf(char *fmt, ...);
BOOL is_NT4();
int get_OPENFILENAME_SIZE();

#ifdef __cplusplus
}
#endif
