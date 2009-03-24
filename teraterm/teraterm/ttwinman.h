/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, variables, flags related to VT win and TEK win */
#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
void VTActivate();
void ConvertToCP932(char *str, int len);
void ChangeTitle();
void SwitchMenu();
void SwitchTitleBar();
void OpenHelp(HWND HWin, UINT Command, DWORD Data);
void OpenHtmlHelp(HWND HWin, char *filename);

extern HWND HVTWin;
extern HWND HTEKWin;
extern int ActiveWin; /* IdVT, IdTEK */
extern int TalkStatus; /* IdTalkKeyb, IdTalkCB, IdTalkTextFile */
extern BOOL KeybEnabled; /* keyboard switch */
extern BOOL Connecting;

/* 'help' button on dialog box */
extern WORD MsgDlgHelp;
extern LONG HelpId;

extern TTTSet ts;
extern TComVar cv;

/* pointers to window objects */
extern void* pVTWin;
extern void* pTEKWin;
/* instance handle */
extern HINSTANCE hInst;

extern int SerialNo;

#ifdef __cplusplus
}
#endif
