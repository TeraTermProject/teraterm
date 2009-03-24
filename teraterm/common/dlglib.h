/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* Routines for dialog boxes */
#ifdef __cplusplus
extern "C" {
#endif

void EnableDlgItem(HWND HDlg, int FirstId, int LastId);
void DisableDlgItem(HWND HDlg, int FirstId, int LastId);
void ShowDlgItem(HWND HDlg, int FirstId, int LastId);
void SetRB(HWND HDlg, int R, int FirstId, int LastId);
void GetRB(HWND HDlg, LPWORD R, int FirstId, int LastId);
void SetDlgNum(HWND HDlg, int id_Item, LONG Num);
void SetDlgPercent(HWND HDlg, int id_Item, int id_Progress, LONG a, LONG b, int *prog);
void SetDropDownList(HWND HDlg, int Id_Item, PCHAR far *List, int nsel);
LONG GetCurSel(HWND HDlg, int Id_Item);
void InitDlgProgress(HWND HDlg, int id_Progress, int *CurProgStat);

#ifdef __cplusplus
}
#endif
