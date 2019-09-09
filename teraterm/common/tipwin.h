
#include <windows.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	FRAME_WIDTH	6

typedef struct tagTipWinData TipWin;

TipWin *TipWinCreate(HWND src, int cx, int cy, const TCHAR *str);
void TipWinSetText(TipWin *tWin, TCHAR *text);
void TipWinDestroy(TipWin *tWin);
void TipWinGetTextWidthHeight(HWND src, const TCHAR *str, int *width, int *height);
void TipWinSetPos(TipWin *tWin, int x, int y);

#ifdef __cplusplus
}
#endif
