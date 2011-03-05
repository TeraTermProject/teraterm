/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, dialog boxes */

#ifdef __cplusplus
extern "C" {
#endif

void ParseParam(PBOOL IOption, PBOOL VOption);
BOOL GetFileName(HWND HWin);
void SetDlgPos(int x, int y);
void OpenInpDlg(PCHAR Buff, PCHAR Text, PCHAR Caption,
                PCHAR Default, BOOL Paswd);
int OpenErrDlg(PCHAR Msg, PCHAR Line);
int OpenMsgDlg(PCHAR Text, PCHAR Caption, BOOL YesNo);
void OpenStatDlg(PCHAR Text, PCHAR Caption);
void CloseStatDlg();

extern char HomeDir[MAXPATHLEN];
extern char FileName[MAX_PATH];
extern char TopicName[11];
extern char ShortName[MAX_PATH];
extern char Param2[MAXPATHLEN];
extern char Param3[MAXPATHLEN];
extern char Param4[MAXPATHLEN];
extern char Param5[MAXPATHLEN];
extern char Param6[MAXPATHLEN];
extern char Param7[MAXPATHLEN];
extern char Param8[MAXPATHLEN];
extern char Param9[MAXPATHLEN];
extern BOOL SleepFlag;

#ifdef __cplusplus
}
#endif
