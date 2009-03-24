/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, VT terminal emulation */
extern int MouseReportMode;

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
void ResetTerminal();
void ResetCharSet();
void ResetKeypadMode(BOOL DisabledModeOnly);
void HideStatusLine();
void ChangeTerminalSize(int Nx, int Ny);
int VTParse();
void FocusReport(BOOL Focus);
BOOL MouseReport(int Event, int Button, int Xpos, int Ypos);

#ifdef __cplusplus
}
#endif
