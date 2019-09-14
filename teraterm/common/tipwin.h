#ifndef _H_TIPWIN
#define _H_TIPWIN
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
void TipWinSetShow(TipWin *tWin);
void TipWinSetHide(TipWin *tWin);
void TipWinSetHideTimer(TipWin *tWin, int ms);
int TipWinIsVisible(TipWin *tWin);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class CTipWin
{
public:
	CTipWin(HWND hWnd, int x, int y, const TCHAR *str);
	CTipWin(HWND hWnd);
	~CTipWin(VOID);
	VOID SetText(TCHAR *str);
	VOID Destroy(VOID);
	VOID GetTextWidthHeight(HWND src, const TCHAR *str, int *width, int *height);
	POINT GetPos(VOID);
	VOID SetPos(int x, int y);
	VOID SetHideTimer(int ms);
	VOID Create(HWND src, int x, int y, const TCHAR *str);
	BOOL IsExists(VOID);
	VOID SetVisible(BOOL bVisible);
	BOOL IsVisible();
private:
	POINT pts;
	UINT timerid;
	TipWin* tWin;
	WNDCLASS wc;
	static ATOM tip_class;
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
	VOID CalcStrRect(VOID);
};
#endif
#endif