/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS GUI abstraction layer.
 * Maps Windows GUI concepts to Cocoa/AppKit equivalents.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../platform_macos_types.h"

/* --- Window management --- */
HWND  tt_CreateWindow(const char* className, const char* title,
                      int x, int y, int width, int height,
                      DWORD style, HWND parent);
HWND  tt_CreateWindowW(const wchar_t* className, const wchar_t* title,
                       int x, int y, int width, int height,
                       DWORD style, HWND parent);
BOOL  tt_DestroyWindow(HWND hWnd);
BOOL  tt_ShowWindow(HWND hWnd, int nCmdShow);
BOOL  tt_UpdateWindow(HWND hWnd);
BOOL  tt_MoveWindow(HWND hWnd, int x, int y, int width, int height, BOOL repaint);
BOOL  tt_SetWindowPos(HWND hWnd, HWND hWndAfter, int x, int y, int cx, int cy, UINT flags);
BOOL  tt_GetWindowRect(HWND hWnd, LPRECT lpRect);
BOOL  tt_GetClientRect(HWND hWnd, LPRECT lpRect);
BOOL  tt_InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase);
BOOL  tt_SetWindowTextW(HWND hWnd, const wchar_t* text);
BOOL  tt_SetWindowTextA(HWND hWnd, const char* text);

/* --- Message loop --- */
BOOL  tt_GetMessage(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
BOOL  tt_PeekMessage(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL  tt_TranslateMessage(const MSG* lpMsg);
LRESULT tt_DispatchMessage(const MSG* lpMsg);
void  tt_PostQuitMessage(int nExitCode);
LRESULT tt_SendMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL  tt_PostMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* --- Timer --- */
UINT_PTR tt_SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, void* lpTimerFunc);
BOOL  tt_KillTimer(HWND hWnd, UINT_PTR uIDEvent);

/* --- Painting (CoreGraphics based) --- */
HDC   tt_BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint);
BOOL  tt_EndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint);
HDC   tt_GetDC(HWND hWnd);
int   tt_ReleaseDC(HWND hWnd, HDC hDC);

/* --- Drawing context operations --- */
COLORREF tt_SetTextColor(HDC hdc, COLORREF color);
COLORREF tt_SetBkColor(HDC hdc, COLORREF color);
int   tt_SetBkMode(HDC hdc, int mode);
BOOL  tt_TextOutW(HDC hdc, int x, int y, const wchar_t* lpString, int c);
BOOL  tt_TextOutA(HDC hdc, int x, int y, const char* lpString, int c);
BOOL  tt_ExtTextOutW(HDC hdc, int x, int y, UINT options, const RECT* lprect,
                     const wchar_t* lpString, UINT c, const int* lpDx);
BOOL  tt_Rectangle(HDC hdc, int left, int top, int right, int bottom);
BOOL  tt_FillRect(HDC hdc, const RECT* lprc, HBRUSH hbr);
HBRUSH tt_CreateSolidBrush(COLORREF color);
HFONT tt_CreateFontIndirectW(const LOGFONTW* lplf);
HGDIOBJ tt_SelectObject(HDC hdc, HGDIOBJ h);
BOOL  tt_DeleteObject(HGDIOBJ ho);
BOOL  tt_GetTextExtentPoint32W(HDC hdc, const wchar_t* lpString, int c, SIZE* psizl);

/* --- Clipboard --- */
BOOL  tt_OpenClipboard(HWND hWndNewOwner);
BOOL  tt_CloseClipboard(void);
BOOL  tt_EmptyClipboard(void);
HANDLE tt_SetClipboardData(UINT uFormat, HANDLE hMem);
HANDLE tt_GetClipboardData(UINT uFormat);
BOOL  tt_IsClipboardFormatAvailable(UINT format);

/* --- Dialog --- */
INT_PTR tt_DialogBoxParamW(HINSTANCE hInstance, const wchar_t* lpTemplateName,
                           HWND hWndParent, void* lpDialogFunc, LPARAM dwInitParam);
BOOL  tt_EndDialog(HWND hDlg, INT_PTR nResult);
int   tt_MessageBoxW(HWND hWnd, const wchar_t* lpText, const wchar_t* lpCaption, UINT uType);
int   tt_MessageBoxA(HWND hWnd, const char* lpText, const char* lpCaption, UINT uType);

/* --- Menu --- */
HMENU tt_CreateMenu(void);
HMENU tt_CreatePopupMenu(void);
BOOL  tt_AppendMenuW(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, const wchar_t* lpNewItem);
BOOL  tt_DestroyMenu(HMENU hMenu);
BOOL  tt_SetMenu(HWND hWnd, HMENU hMenu);

/* --- Cursor --- */
HCURSOR tt_SetCursor(HCURSOR hCursor);
HCURSOR tt_LoadCursorW(HINSTANCE hInstance, const wchar_t* lpCursorName);
BOOL  tt_GetCursorPos(LPPOINT lpPoint);
BOOL  tt_SetCursorPos(int X, int Y);

/* --- Application lifecycle --- */
int   tt_MacOS_RunApp(int argc, char* argv[]);

/* MessageBox types */
#define MB_OK               0x00000000L
#define MB_OKCANCEL         0x00000001L
#define MB_YESNOCANCEL      0x00000003L
#define MB_YESNO            0x00000004L
#define MB_ICONERROR        0x00000010L
#define MB_ICONQUESTION     0x00000020L
#define MB_ICONWARNING      0x00000030L
#define MB_ICONINFORMATION  0x00000040L
#define IDOK     1
#define IDCANCEL 2
#define IDYES    6
#define IDNO     7

#ifdef __cplusplus
}
#endif
