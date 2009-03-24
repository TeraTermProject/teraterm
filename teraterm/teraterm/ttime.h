/* Tera Term */
/* TERATERM.EXE, IME interface */

#ifdef __cplusplus
extern "C" {
#endif

/* proto types */
BOOL LoadIME();
void FreeIME();
BOOL CanUseIME();
void SetConversionWindow(HWND HWin, int X, int Y);
void SetConversionLogFont(PLOGFONT lf);

HGLOBAL GetConvString(UINT wParam, LPARAM lParam);

#ifndef WM_IME_COMPOSITION
#define WM_IME_COMPOSITION              0x010F
#endif

#ifdef __cplusplus
}
#endif
