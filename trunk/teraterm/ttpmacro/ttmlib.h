// Tera Term
// Copyright(C) 1994-1998 T. Teranishi
// All rights reserved.

// TTMACRO.EXE, misc routines

#ifdef __cplusplus
extern "C" {
#endif

extern char UILanguageFile[MAX_PATH];

void CalcTextExtent(HDC DC, PCHAR Text, LPSIZE s);
void TTMGetDir(PCHAR Dir, int destlen);
void TTMSetDir(PCHAR Dir);
int GetAbsPath(PCHAR FName, int destlen);

#ifdef __cplusplus
}
#endif
