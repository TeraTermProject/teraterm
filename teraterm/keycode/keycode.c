/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* keycode.exe for Tera Term Pro */

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "kc_res.h"
#define ClassName "KeyCodeWin32"

#include "compat_w95.h"

// Prototypes
LRESULT WINAPI MainWndProc( HWND, UINT, WPARAM, LPARAM );

// Global variables;
static HANDLE ghInstance;
static BOOL KeyDown = FALSE;
static BOOL Short;
static WORD Scan;

int PASCAL WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpszCmdLine,
  int nCmdShow)
{
  WNDCLASS wc;
  MSG msg;
  HWND hWnd;

  // インストーラで実行を検出するために mutex を作成する (2006.8.12 maya)
  // 2重起動防止のためではないので、特に返り値は見ない
  HANDLE hMutex;
  hMutex = CreateMutex(NULL, TRUE, "TeraTermProKeycodeAppMutex");

  DoCover_IsDebuggerPresent();

  if(!hPrevInstance)
  {
    wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = ClassName;
    RegisterClass(&wc);
  }

  ghInstance = hInstance;

  hWnd = CreateWindow(ClassName,
    "Key code for Tera Term",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    200,
    100,
    NULL,
    NULL,
    hInstance,
    NULL);

  ShowWindow(hWnd, nCmdShow);

  PostMessage(hWnd,WM_SETICON,ICON_SMALL,
              (LPARAM)LoadImage(hInstance,
                                MAKEINTRESOURCE(IDI_KEYCODE),
                                IMAGE_ICON,16,16,0));
  PostMessage(hWnd,WM_SETICON,ICON_BIG,
              (LPARAM)LoadImage(hInstance,
                                MAKEINTRESOURCE(IDI_KEYCODE),
                                IMAGE_ICON,0,0,0));

  while(GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}

void KeyDownProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  if ((wParam==VK_SHIFT) ||
      (wParam==VK_CONTROL) ||
      (wParam==VK_MENU)) return;

  Scan = HIWORD(lParam) & 0x1ff;
  if ((GetKeyState(VK_SHIFT) & 0x80) != 0)
    Scan = Scan | 0x200;
  if ((GetKeyState(VK_CONTROL) & 0x80) != 0)
    Scan = Scan | 0x400;
  if ((GetKeyState(VK_MENU) & 0x80) != 0)
    Scan = Scan | 0x800;

  if (! KeyDown)
  {
    KeyDown = TRUE;
    Short = TRUE;
    SetTimer(hWnd,1,10,NULL);
    InvalidateRect(hWnd,NULL,TRUE);
  }
}

void KeyUpProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  if (! KeyDown) return;
  if (Short)
    SetTimer(hWnd,2,500,NULL);
  else {
    KeyDown = FALSE;
    InvalidateRect(hWnd,NULL,TRUE);
  }
}

void PaintProc(HWND hWnd)
{
  PAINTSTRUCT ps;
  HDC hDC;
  char OutStr[30];

  hDC = BeginPaint(hWnd, &ps);

  if (KeyDown)
  {
    _snprintf_s(OutStr,sizeof(OutStr),_TRUNCATE,"Key code is %u.",Scan);
    TextOut(hDC,10,10,OutStr,strlen(OutStr));
  }
  else
    TextOut(hDC,10,10,"Push any key.",13);

  EndPaint(hWnd, &ps);
}

void TimerProc(HWND hWnd, WPARAM wParam)
{
  KillTimer(hWnd,wParam);
  if (wParam==1)
    Short = FALSE;
  else if (wParam==2)
  {
    KeyDown = FALSE;
    InvalidateRect(hWnd,NULL,TRUE);
  }
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam,
  LPARAM lParam)
{
  switch( msg ) {
    case WM_KEYDOWN:
      KeyDownProc(hWnd, wParam, lParam);
      break;
    case WM_KEYUP:
      KeyUpProc(hWnd, wParam, lParam);
      break;
    case WM_SYSKEYDOWN:
      if (wParam==VK_F10)
	KeyDownProc(hWnd, wParam, lParam);
      else
	return (DefWindowProc(hWnd, msg, wParam, lParam));
      break;
    case WM_SYSKEYUP:
      if (wParam==VK_F10)
	KeyUpProc(hWnd, wParam, lParam);
      else
	return (DefWindowProc(hWnd, msg, wParam, lParam));
      break;
    case WM_PAINT:
      PaintProc(hWnd);
      break;
    case WM_TIMER:
      TimerProc(hWnd, wParam);
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return (DefWindowProc(hWnd, msg, wParam, lParam));
  }

  return 0;
}
