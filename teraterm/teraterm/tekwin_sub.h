
#pragma once

#include <windows.h>
#include "tmfc.h"

#if 0
class TTCDC
{
public:
	HDC m_hDC;
	BOOL operator=(const CDC& obj);
	HDC GetSafeHdc() const;
};
#endif

class TTCMenu
{
public:
	HMENU m_hMenu;
};

class TTCFrameWnd : public TTCWnd
{
public:
	TTCFrameWnd();
	virtual ~TTCFrameWnd();
	static TTCFrameWnd *pseudoPtr;
	static LRESULT ProcStub(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
	virtual BOOL Create(HINSTANCE hInstance,
						LPCTSTR lpszClassName,
						LPCTSTR lpszWindowName,
						DWORD dwStyle = WS_OVERLAPPEDWINDOW,
						const RECT& rect = rectDefault,
						HWND pParentWnd = NULL,        // != NULL for popups
						LPCTSTR lpszMenuName = NULL,
						DWORD dwExStyle = 0);//,
						//CCreateContext* pContext = NULL);
	virtual LRESULT Proc(UINT msg, WPARAM wp, LPARAM lp);
	static const RECT rectDefault;
	///
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	///
#if 1
	void OnInitMenuPopup(TTCMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	void OnKillFocus(TTCWnd* pNewWnd);
	void OnDestroy();
	void OnSetFocus(TTCWnd* pOldWnd);
	void OnSysCommand(UINT nID, LPARAM lParam);
	void OnClose();
#endif
	void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
};

class TTCPoint {
public:
	int x;
	int y;
};

