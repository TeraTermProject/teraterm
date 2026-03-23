/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * Windows API type definitions for macOS/POSIX platforms.
 *
 * Provides the fundamental Windows types (HWND, HINSTANCE, BOOL, DWORD, etc.)
 * as POSIX-compatible equivalents so that Tera Term source code can compile
 * on non-Windows platforms.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Basic integer types --- */
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned int        DWORD;
typedef unsigned int        UINT;
typedef int                 INT;
typedef long                LONG;
typedef unsigned long       ULONG;
typedef short               SHORT;
typedef unsigned short      USHORT;
typedef char                CHAR;
typedef unsigned char       UCHAR;
typedef wchar_t             WCHAR;
typedef float               FLOAT;

typedef int64_t             LONGLONG;
typedef uint64_t            ULONGLONG;
typedef intptr_t            LONG_PTR;
typedef uintptr_t           ULONG_PTR;
typedef uintptr_t           DWORD_PTR;
typedef intptr_t            INT_PTR;
typedef uintptr_t           UINT_PTR;

typedef LONG_PTR            LRESULT;
typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;

/* --- Pointer types --- */
typedef char*               LPSTR;
typedef const char*         LPCSTR;
typedef wchar_t*            LPWSTR;
typedef const wchar_t*      LPCWSTR;
typedef void*               LPVOID;
typedef const void*         LPCVOID;
typedef BYTE*               LPBYTE;
typedef WORD*               LPWORD;
typedef DWORD*              LPDWORD;
typedef BOOL*               LPBOOL;

/* --- Handle types (opaque pointers on non-Windows) --- */
typedef void*               HANDLE;
typedef void*               HWND;
typedef void*               HDC;
typedef void*               HINSTANCE;
typedef void*               HMODULE;
typedef void*               HGLOBAL;
typedef void*               HLOCAL;
typedef void*               HBITMAP;
typedef void*               HBRUSH;
typedef void*               HFONT;
typedef void*               HICON;
typedef void*               HCURSOR;
typedef void*               HPEN;
typedef void*               HRGN;
typedef void*               HPALETTE;
typedef void*               HMENU;
typedef void*               HACCEL;
typedef void*               HGDIOBJ;
typedef void*               HKEY;
typedef void*               HRSRC;
typedef void*               HMONITOR;

/* --- Special constants --- */
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL  ((void*)0)
#endif

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

/* --- Calling conventions (no-op on non-Windows) --- */
#define WINAPI
#define CALLBACK
#define APIENTRY
#define PASCAL
#define CDECL
#define __stdcall
#define __cdecl
#define __fastcall
#define DECLSPEC_IMPORT
#define __declspec(x)

/* --- Text macros --- */
#ifdef UNICODE
  typedef WCHAR             TCHAR;
  #define _T(x)             L##x
  #define TEXT(x)            L##x
#else
  typedef CHAR              TCHAR;
  #define _T(x)             x
  #define TEXT(x)            x
#endif

typedef TCHAR*              LPTSTR;
typedef const TCHAR*        LPCTSTR;

/* --- RECT, POINT, SIZE --- */
typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *LPRECT;
typedef const RECT* LPCRECT;

typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT, *LPPOINT;

typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE, *LPSIZE;

/* --- PAINTSTRUCT --- */
typedef struct tagPAINTSTRUCT {
    HDC  hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;

/* --- LOGFONT --- */
#define LF_FACESIZE 32
typedef struct tagLOGFONTW {
    LONG  lfHeight;
    LONG  lfWidth;
    LONG  lfEscapement;
    LONG  lfOrientation;
    LONG  lfWeight;
    BYTE  lfItalic;
    BYTE  lfUnderline;
    BYTE  lfStrikeOut;
    BYTE  lfCharSet;
    BYTE  lfOutPrecision;
    BYTE  lfClipPrecision;
    BYTE  lfQuality;
    BYTE  lfPitchAndFamily;
    WCHAR lfFaceName[LF_FACESIZE];
} LOGFONTW, *LPLOGFONTW;

typedef struct tagLOGFONTA {
    LONG  lfHeight;
    LONG  lfWidth;
    LONG  lfEscapement;
    LONG  lfOrientation;
    LONG  lfWeight;
    BYTE  lfItalic;
    BYTE  lfUnderline;
    BYTE  lfStrikeOut;
    BYTE  lfCharSet;
    BYTE  lfOutPrecision;
    BYTE  lfClipPrecision;
    BYTE  lfQuality;
    BYTE  lfPitchAndFamily;
    CHAR  lfFaceName[LF_FACESIZE];
} LOGFONTA, *LPLOGFONTA;

#ifdef UNICODE
typedef LOGFONTW LOGFONT;
#else
typedef LOGFONTA LOGFONT;
#endif

/* --- COLORREF --- */
typedef DWORD COLORREF;
#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) ((BYTE)((rgb) >> 16))

/* --- MSG --- */
typedef struct tagMSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
} MSG, *LPMSG;

/* --- Common Windows message constants --- */
#define WM_NULL           0x0000
#define WM_CREATE         0x0001
#define WM_DESTROY        0x0002
#define WM_MOVE           0x0003
#define WM_SIZE           0x0005
#define WM_ACTIVATE       0x0006
#define WM_SETFOCUS       0x0007
#define WM_KILLFOCUS      0x0008
#define WM_ENABLE         0x000A
#define WM_PAINT          0x000F
#define WM_CLOSE          0x0010
#define WM_QUIT           0x0012
#define WM_ERASEBKGND     0x0014
#define WM_SHOWWINDOW     0x0018
#define WM_SETTEXT        0x000C
#define WM_GETTEXT        0x000D
#define WM_GETTEXTLENGTH  0x000E
#define WM_TIMER          0x0113
#define WM_HSCROLL        0x0114
#define WM_VSCROLL        0x0115
#define WM_COMMAND        0x0111
#define WM_SYSCOMMAND     0x0112
#define WM_CHAR           0x0102
#define WM_KEYDOWN        0x0100
#define WM_KEYUP          0x0101
#define WM_SYSKEYDOWN     0x0104
#define WM_SYSKEYUP       0x0105
#define WM_MOUSEMOVE      0x0200
#define WM_LBUTTONDOWN    0x0201
#define WM_LBUTTONUP      0x0202
#define WM_LBUTTONDBLCLK  0x0203
#define WM_RBUTTONDOWN    0x0204
#define WM_RBUTTONUP      0x0205
#define WM_RBUTTONDBLCLK  0x0206
#define WM_MBUTTONDOWN    0x0207
#define WM_MBUTTONUP      0x0208
#define WM_MOUSEWHEEL     0x020A
#define WM_DROPFILES      0x0233
#define WM_USER           0x0400
#define WM_COPYDATA       0x004A
#define WM_NOTIFY         0x004E
#define WM_INITDIALOG     0x0110
#define WM_CTLCOLORDLG    0x0136
#define WM_CTLCOLORSTATIC 0x0138
#define WM_SETFONT        0x0030
#define WM_GETFONT        0x0031
#define WM_DRAWITEM       0x002B
#define WM_MEASUREITEM    0x002C
#define WM_DELETEITEM     0x002D
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED  0x0047
#define WM_NCCREATE       0x0081
#define WM_NCDESTROY      0x0082
#define WM_NCHITTEST      0x0084
#define WM_NCPAINT        0x0085
#define WM_GETMINMAXINFO  0x0024
#define WM_ENTERSIZEMOVE  0x0231
#define WM_EXITSIZEMOVE   0x0232
#define WM_SETTINGCHANGE  0x001A
#define WM_DPICHANGED     0x02E0
#define WM_IME_COMPOSITION 0x010F
#define WM_IME_STARTCOMPOSITION 0x010D
#define WM_IME_ENDCOMPOSITION   0x010E
#define WM_IME_NOTIFY     0x0282

/* --- Window Styles --- */
#define WS_OVERLAPPED     0x00000000L
#define WS_POPUP          0x80000000L
#define WS_CHILD          0x40000000L
#define WS_MINIMIZE       0x20000000L
#define WS_VISIBLE        0x10000000L
#define WS_DISABLED       0x08000000L
#define WS_CLIPSIBLINGS   0x04000000L
#define WS_CLIPCHILDREN   0x02000000L
#define WS_MAXIMIZE       0x01000000L
#define WS_CAPTION        0x00C00000L
#define WS_BORDER         0x00800000L
#define WS_DLGFRAME       0x00400000L
#define WS_VSCROLL        0x00200000L
#define WS_HSCROLL        0x00100000L
#define WS_SYSMENU        0x00080000L
#define WS_THICKFRAME     0x00040000L
#define WS_GROUP          0x00020000L
#define WS_TABSTOP        0x00010000L
#define WS_MINIMIZEBOX    0x00020000L
#define WS_MAXIMIZEBOX    0x00010000L
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)

/* --- Extended Window Styles --- */
#define WS_EX_TOPMOST     0x00000008L
#define WS_EX_ACCEPTFILES 0x00000010L
#define WS_EX_TRANSPARENT 0x00000020L

/* --- Window show commands --- */
#define SW_HIDE           0
#define SW_SHOWNORMAL     1
#define SW_NORMAL         1
#define SW_SHOWMINIMIZED  2
#define SW_SHOWMAXIMIZED  3
#define SW_MAXIMIZE       3
#define SW_SHOW           5
#define SW_MINIMIZE       6
#define SW_RESTORE        9

/* --- SetWindowPos flags --- */
#define SWP_NOSIZE        0x0001
#define SWP_NOMOVE        0x0002
#define SWP_NOZORDER      0x0004
#define SWP_NOACTIVATE    0x0010
#define SWP_SHOWWINDOW    0x0040
#define SWP_HIDEWINDOW    0x0080
#define HWND_TOP          ((HWND)0)
#define HWND_TOPMOST      ((HWND)-1)
#define HWND_NOTOPMOST    ((HWND)-2)

/* --- VK_ Virtual Key codes --- */
#define VK_BACK           0x08
#define VK_TAB            0x09
#define VK_RETURN         0x0D
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14
#define VK_ESCAPE         0x1B
#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B

/* --- GDI constants --- */
#define TRANSPARENT       1
#define OPAQUE            2
#define DEFAULT_CHARSET   1
#define SYMBOL_CHARSET    2
#define SHIFTJIS_CHARSET  128
#define ANSI_CHARSET      0
#define FW_NORMAL         400
#define FW_BOLD           700

/* --- Clipboard formats --- */
#define CF_TEXT           1
#define CF_UNICODETEXT    13

/* --- SOCKET types (BSD sockets) --- */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

typedef int SOCKET;
#define INVALID_SOCKET  ((SOCKET)-1)
#define SOCKET_ERROR    (-1)
#define closesocket(s)  close(s)
#define WSAGetLastError() errno
#define WSAEWOULDBLOCK  EWOULDBLOCK
#define WSAEINPROGRESS  EINPROGRESS

/* --- File API compatibility --- */
#define MAX_PATH          260

/* --- Critical section (mapped to pthread mutex) --- */
#include <pthread.h>
typedef pthread_mutex_t CRITICAL_SECTION;
#define InitializeCriticalSection(cs)  pthread_mutex_init(cs, NULL)
#define DeleteCriticalSection(cs)      pthread_mutex_destroy(cs)
#define EnterCriticalSection(cs)       pthread_mutex_lock(cs)
#define LeaveCriticalSection(cs)       pthread_mutex_unlock(cs)

/* --- min/max macros --- */
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

/* --- String functions compatibility --- */
#define _stricmp    strcasecmp
#define _strnicmp   strncasecmp
#define _wcsicmp    wcscasecmp
#define _wcsnicmp   wcsncasecmp
#define _strdup     strdup
#define _wcsdup     wcsdup
#define _snprintf   snprintf
#define _snwprintf  swprintf
#define sprintf_s   snprintf
#define _vsnprintf  vsnprintf
#define _vsnwprintf vswprintf
#define ZeroMemory(p, sz) memset((p), 0, (sz))
#define CopyMemory(d, s, sz) memcpy((d), (s), (sz))
#define MoveMemory(d, s, sz) memmove((d), (s), (sz))
#define FillMemory(p, sz, v) memset((p), (v), (sz))

/* --- HRESULT --- */
typedef LONG HRESULT;
#define S_OK        ((HRESULT)0L)
#define S_FALSE     ((HRESULT)1L)
#define E_FAIL      ((HRESULT)0x80004005L)
#define E_NOTIMPL   ((HRESULT)0x80004001L)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr)    (((HRESULT)(hr)) < 0)

/* --- DLL loading (mapped to dlopen/dlsym) --- */
#include <dlfcn.h>
#define LoadLibraryA(name)       dlopen(name, RTLD_LAZY)
#define LoadLibraryW(name)       dlopen_wide(name)
#define FreeLibrary(h)           dlclose(h)
#define GetProcAddress(h, name)  dlsym(h, name)

/* --- Misc --- */
#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l) ((WORD)(((DWORD_PTR)(l) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w) ((BYTE)(((DWORD_PTR)(w) >> 8) & 0xff))
#define MAKELONG(a, b) ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define MAKEWORD(a, b) ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))

/* Forward declarations for platform-specific functions */
void* dlopen_wide(const wchar_t* name);
DWORD GetTickCount(void);
void Sleep(DWORD dwMilliseconds);
DWORD GetLastError(void);
void SetLastError(DWORD dwErrCode);
DWORD GetCurrentThreadId(void);
DWORD GetCurrentProcessId(void);

#ifdef __cplusplus
}
#endif
