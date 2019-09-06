
#include <windows.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagTipWinData TipWin;

TipWin *TipWinCreate(HWND src, int cx, int cy, const TCHAR *str, BOOL resizing_tips);
void TipWinSetText(TipWin *tWin, TCHAR *text);
void TipWinDestroy(TipWin *tWin);

#ifdef __cplusplus
}
#endif
