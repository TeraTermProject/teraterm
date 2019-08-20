/*
 * $Id: Window.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_WINDOWS_H_
#define _YCL_WINDOWS_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <YCL/String.h>

namespace yebisuya {

class Window {
private:
	HWND window;
public:
	Window():window(NULL) {
	}
	Window(HWND window):window(window) {
	}
	void operator<<=(HWND newWindow) {
		window = newWindow;
	}
	operator HWND()const {
		return window;
	}

	LONG_PTR GetWindowLongPtr(int index)const {
		return ::GetWindowLongPtr(window, index);
	}
	LONG_PTR SetWindowLongPtr(int index, LONG_PTR data) {
		return ::SetWindowLongPtr(window, index, data);
	}
	int GetWindowTextLength()const {
		return ::GetWindowTextLength(window);
	}
	int GetWindowText(char* buffer, int size)const {
		return ::GetWindowText(window, buffer, size);
	}
	String GetWindowText()const {
		int length = GetWindowTextLength();
		char* buffer = (char*) alloca(length + 1);
		GetWindowText(buffer, length + 1);
		return buffer;
	}
	bool SetWindowText(const char* text) {
		return ::SetWindowText(window, text) != FALSE;
	}
	LRESULT SendMessage(UINT message, WPARAM wparam = 0, LPARAM lparam = 0)const {
		return ::SendMessage(window, message, wparam, lparam);
	}
	LRESULT PostMessage(UINT message, WPARAM wparam = 0, LPARAM lparam = 0)const {
		return ::PostMessage(window, message, wparam, lparam);
	}
	HWND GetParent()const {
		return ::GetParent(window);
	}
	bool EnableWindow(bool enabled) {
		return ::EnableWindow(window, enabled) != FALSE;
	}
	LRESULT DefWindowProc(int message, int wparam, long lparam) {
		return ::DefWindowProc(window, message, wparam, lparam);
	}
	LRESULT CallWindowProc(WNDPROC proc, int message, int wParam, long lParam) {
		return ::CallWindowProc(proc, window, message, wParam, lParam);
	}
	bool ShowWindow(int command) {
		return ::ShowWindow(window, command) != FALSE;
	}
	HWND SetFocus() {
		return ::SetFocus(window);
	}
	HWND SetCapture() {
		return ::SetCapture(window);
	}
	bool IsWindowEnabled()const {
		return ::IsWindowEnabled(window) != FALSE;
	}
	bool IsWindowVisible()const {
		return ::IsWindowVisible(window) != FALSE;
	}
	bool IsIconic()const {
		return ::IsIconic(window) != FALSE;
	}
	bool IsZoomed()const {
		return ::IsZoomed(window) != FALSE;
	}
	UINT_PTR SetTimer(int id, int elapse, TIMERPROC timerProc = NULL) {
		return ::SetTimer(window, id, elapse, timerProc);
	}
	bool KillTimer(int id) {
		return ::KillTimer(window, id) != FALSE;
	}
	HMENU GetMenu()const {
		return ::GetMenu(window);
	}
	bool SetForegroundWindow() {
		return ::SetForegroundWindow(window) != FALSE;
	}
	void UpdateWindow() {
		::UpdateWindow(window);
	}
	bool GetWindowRect(RECT& rect)const {
		return ::GetWindowRect(window, &rect) != FALSE;
	}
	bool GetClientRect(RECT& rect)const {
		return ::GetClientRect(window, &rect) != FALSE;
	}
	bool SetWindowPos(HWND previous, int x, int y, int cx, int cy, int flags) {
		return ::SetWindowPos(window, previous, x, y, cx, cy, flags) != FALSE;
	}
	bool ClientToScreen(POINT& point)const {
		return ::ClientToScreen(window, &point) != FALSE;
	}
	bool ScreenToClient(POINT& point)const {
		return ::ScreenToClient(window, &point) != FALSE;
	}
	bool ClientToScreen(RECT& rect)const {
		return ::ClientToScreen(window, (POINT*) &rect.left) != FALSE
			&& ::ClientToScreen(window, (POINT*) &rect.right) != FALSE;
	}
	bool ScreenToClient(RECT& rect)const {
		return ::ScreenToClient(window, (POINT*) &rect.left) != FALSE
			&& ::ScreenToClient(window, (POINT*) &rect.right) != FALSE;
	}
	bool InvalidateRect(bool erase = true) {
		return ::InvalidateRect(window, NULL, erase) != FALSE;
	}
	bool InvalidateRect(RECT& rect, bool erase = true) {
		return ::InvalidateRect(window, &rect, erase) != FALSE;
	}
	bool GetWindowPlacement(WINDOWPLACEMENT& wp)const {
		wp.length = sizeof wp;
		return ::GetWindowPlacement(window, &wp) != FALSE;
	}
	bool SetWindowPlacement(const WINDOWPLACEMENT& wp) {
		return ::SetWindowPlacement(window, &wp) != FALSE;
	}
	HWND ChildWindowFromPoint(POINT point, int flags = CWP_ALL)const {
		return ::ChildWindowFromPointEx(window, point, flags);
	}
	HWND GetLastActivePopup()const {
		return ::GetLastActivePopup(window);
	}
	HWND GetWindow(int command)const {
		return ::GetWindow(window, command);
	}
	HWND GetTopWindow()const {
		return ::GetTopWindow(window);
	}
	bool IsWindow()const {
		return ::IsWindow(window) != FALSE;
	}
	bool IsChild(HWND child)const {
		return ::IsChild(window, child) != FALSE;
	}
	bool MoveWindow(int x, int y, int cx, int cy, bool repaint) {
		return ::MoveWindow(window, x, y, cx, cy, repaint) != FALSE;
	}
	bool OpenIcon() {
		return ::OpenIcon(window) != FALSE;
	}
	bool ShowOwnedPopups(bool show) {
		return ::ShowOwnedPopups(window, show) != FALSE;
	}
	bool ShowWindowAsync(int command) {
		return ::ShowWindowAsync(window, command) != FALSE;
	}
	int GetDlgCtrlID()const {
		return ::GetDlgCtrlID(window);
	}
	HWND GetDlgItem(int id)const {
		return ::GetDlgItem(window, id);
	}
	bool DestroyWindow() {
		if (window == NULL || ::DestroyWindow(window) == FALSE)
			return false;
		window = NULL;
		return true;
	}
	bool CloseWindow() {
		return ::CloseWindow(window) != FALSE;
	}
	int SetScrollPos(int type, int position, bool redraw) {
		return ::SetScrollPos(window, type, position, redraw);
	}
	int GetScrollPos(int type)const {
		return ::GetScrollPos(window, type);
	}
	bool SetScrollRange(int type, int minimum, int maximum, bool redraw) {
		return ::SetScrollRange(window, type, minimum, maximum, redraw) != FALSE;
	}
	bool GetScrollRange(int type, int& minimum, int& maximum)const {
		return ::GetScrollRange(window, type, &minimum, &maximum) != FALSE;
	}
	int SetScrollInfo(int type, const SCROLLINFO& info, bool redraw) {
		return ::SetScrollInfo(window, type, &info, redraw);
	}
	bool GetScrollInfo(int type, SCROLLINFO& info)const {
		return ::GetScrollInfo(window, type, &info) != FALSE;
	}
	long SetLayeredWindowAttributes(COLORREF crKey, BYTE bAlpha, DWORD dwFlags) {
		static const char USER32[] = "user32.dll";
		static const char APINAME[] = "SetLayeredWindowAttributes";
		static BOOL (WINAPI* api)(HWND, COLORREF, BYTE, DWORD)
			= (BOOL (WINAPI*)(HWND, COLORREF, BYTE, DWORD)) ::GetProcAddress(::GetModuleHandleA(USER32), APINAME);
		return api != NULL ? (*api)(window, crKey, bAlpha, dwFlags) : 0;
	}
	long SetLayeredWindowAttributes(BYTE bAlpha, DWORD dwFlags) {
		return SetLayeredWindowAttributes(0, bAlpha, dwFlags);
	}
	HWND SetClipboardViewer() {
		return ::SetClipboardViewer(window);
	}
	bool ChangeClipboardChain(HWND newNext) {
		return ::ChangeClipboardChain(window, newNext) != FALSE;
	}
	bool RegisterHotKey(int id, int modifiers, int keycode) {
		return ::RegisterHotKey(window, id, modifiers, keycode) != FALSE;
	}
	bool UnregisterHotKey(int id) {
		return ::UnregisterHotKey(window, id) != FALSE;
	}
	int MessageBox(const char* message, const char* caption, int type) {
		return ::MessageBox(window, message, caption, type);
	}
	int MessageBox(const char* message, int type) {
		Window top(window);
		while ((top.getStyle() & WS_CHILD) != 0) {
			top <<= top.GetParent();
		}
		return MessageBox(message, top.GetWindowText(), type);
	}
	ULONG_PTR GetClassLongPtr(int index)const {
		return ::GetClassLongPtr(window, index);
	}
	ULONG_PTR SetClassLongPtr(int index, LONG_PTR data) {
		return ::SetClassLongPtr(window, index, data);
	}

	bool create(long exStyle, const char* classname, const char* title, long style, const RECT& rect, HWND parent, HMENU menu, void* param = NULL) {
		return create(exStyle, classname, title, style, rect, parent, menu, GetInstanceHandle(), param);
	}
	bool create(long exStyle, const char* classname, const char* title, long style, const RECT& rect, HWND parent, HMENU menu, HINSTANCE instance, void* param = NULL) {
		return (window = ::CreateWindowEx(exStyle, classname, title,
			style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
			parent, menu, instance, param)) != NULL;
	}
	void setRedraw(bool redraw) {
		SendMessage(WM_SETREDRAW, redraw);
	}
	HFONT getFont()const {
		return (HFONT) SendMessage(WM_GETFONT);
	}
	HFONT setFont(HFONT font, bool redraw) {
		return (HFONT) SendMessage(WM_SETFONT, (WPARAM) font, redraw);
	}
	HICON getIcon(bool large) {
		return (HICON) SendMessage(WM_GETICON, large, 0);
	}
	HICON setIcon(HICON icon, bool large) {
		return (HICON) SendMessage(WM_SETICON, large, (LPARAM) icon);
	}
	LONG_PTR getStyle()const {
		return GetWindowLongPtr(GWL_STYLE);
	}
	LONG_PTR getExStyle()const {
		return GetWindowLongPtr(GWL_EXSTYLE);
	}
	LONG_PTR setStyle(LONG_PTR style) {
		return SetWindowLongPtr(GWL_STYLE, style);
	}
	LONG_PTR setExStyle(LONG_PTR exStyle) {
		return SetWindowLongPtr(GWL_EXSTYLE, exStyle);
	}
	WNDPROC getWndProc()const {
		return (WNDPROC) GetWindowLongPtr(GWLP_WNDPROC);
	}
	WNDPROC setWndProc(WNDPROC proc) {
		return (WNDPROC) SetWindowLongPtr(GWLP_WNDPROC, (LONG_PTR) proc);
	}
	HWND getOwner()const {
		return (HWND) GetWindowLongPtr(GWLP_HWNDPARENT);
	}
	HWND setOwner(HWND owner) {
		return (HWND) SetWindowLongPtr(GWLP_HWNDPARENT, (LONG_PTR) owner);
	}
	HWND getChildWindow()const {
		return GetWindow(GW_CHILD);
	}
	HWND getFirstWindow()const {
		return GetWindow(GW_HWNDFIRST);
	}
	HWND getLastWindow()const {
		return GetWindow(GW_HWNDLAST);
	}
	HWND getNextWindow()const {
		return GetWindow(GW_HWNDNEXT);
	}
	HWND getPreviousWindow()const {
		return GetWindow(GW_HWNDPREV);
	}
	HWND getOwnerWindow()const {
		return GetWindow(GW_OWNER);
	}
#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED 0x80000
#endif
	void setAlpha(BYTE alpha) {
		LONG_PTR exStyle = getExStyle();
		if ((exStyle & WS_EX_LAYERED) == 0)
			setExStyle(exStyle | WS_EX_LAYERED);
		SetLayeredWindowAttributes(alpha, 2);
	}
	static const RECT& USEDEFAULT() {
		static RECT rect = {CW_USEDEFAULT, CW_USEDEFAULT, 0, 0};
		return rect;
	}
	ATOM getClassAtom()const {
		return (ATOM) GetClassLongPtr(GCW_ATOM);
	}
	LONG_PTR getClassExtra()const {
		return GetClassLongPtr(GCL_CBCLSEXTRA);
	}
	LONG_PTR getWindowExtra()const {
		return GetClassLongPtr(GCL_CBWNDEXTRA);
	}
	HBRUSH getBackgroundBrush()const {
		return (HBRUSH) GetClassLongPtr(GCLP_HBRBACKGROUND);
	}
	HCURSOR getClassCursor()const {
		return (HCURSOR) GetClassLongPtr(GCLP_HCURSOR);
	}
	HICON getClassIcon()const {
		return (HICON) GetClassLongPtr(GCLP_HICON);
	}
	HICON getClassSmallIcon()const {
		return (HICON) GetClassLongPtr(GCLP_HICONSM);
	}
	HINSTANCE getClassInstance()const {
		return (HINSTANCE) GetClassLongPtr(GCLP_HMODULE);
	}
	LONG_PTR getMenuResourceId()const {
		return GetClassLongPtr(GCLP_MENUNAME);
	}
	LONG_PTR getClassStyle()const {
		return GetClassLongPtr(GCL_STYLE);
	}
	WNDPROC getClassWindowProc()const {
		return (WNDPROC) GetClassLongPtr(GCLP_WNDPROC);
	}
};

#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")
#endif
}

#endif//_YCL_WINDOWS_H_
