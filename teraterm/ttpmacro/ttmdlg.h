/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, dialog boxes */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MaxStrLen	// ttmparse.hÇ≈Ç‡íËã`ÇµÇƒÇ¢ÇÈÇΩÇﬂìÒèdíËã`ñhé~
#define MaxStrLen (LONG)512
#endif

void ParseParam(PBOOL IOption, PBOOL VOption);
BOOL GetFileName(HWND HWin);
void SetDlgPos(int x, int y);
void OpenInpDlg(PCHAR Buff, PCHAR Text, PCHAR Caption,
                PCHAR Default, BOOL Paswd);
int OpenErrDlg(PCHAR Msg, PCHAR Line, int lineno, int start, int end, PCHAR FileName);
int OpenMsgDlg(PCHAR Text, PCHAR Caption, BOOL YesNo);
void OpenStatDlg(PCHAR Text, PCHAR Caption);
void CloseStatDlg();
void BringupStatDlg();

int OpenListDlg(PCHAR Text, PCHAR Caption, CHAR **Lists, int Selected);

extern char HomeDir[MAXPATHLEN];
extern char FileName[MAX_PATH];
extern char TopicName[11];
extern char ShortName[MAX_PATH];
extern char Param2[MaxStrLen];
extern char Param3[MaxStrLen];
extern char Param4[MaxStrLen];
extern char Param5[MaxStrLen];
extern char Param6[MaxStrLen];
extern char Param7[MaxStrLen];
extern char Param8[MaxStrLen];
extern char Param9[MaxStrLen];
extern BOOL SleepFlag;
extern int ParamCnt;

#ifdef __cplusplus
}
#endif
