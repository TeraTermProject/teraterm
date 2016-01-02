// Tera Term
// Copyright(C) 1994-1998 T. Teranishi
// All rights reserved.

// TTMACRO.EXE, misc routines

#include "teraterm.h"
#include "ttlib.h"
#include <string.h>
#include <direct.h>
#include <Shlobj.h>

static char CurrentDir[MAXPATHLEN];

void CalcTextExtent(HDC DC, PCHAR Text, LPSIZE s)
{
  int W, H, i, i0;
  char Temp[512];
  DWORD dwExt;

  W = 0;
  H = 0;
  i = 0;
  do {
    i0 = i;
    while ((Text[i]!=0) &&
	   (Text[i]!=0x0d) &&
	   (Text[i]!=0x0a))
      i++;
    memcpy(Temp,&Text[i0],i-i0);
    Temp[i-i0] = 0;
    if (Temp[0]==0)
    {
     Temp[0] = 0x20;
     Temp[1] = 0;
    }
    dwExt = GetTabbedTextExtent(DC,Temp,strlen(Temp),0,NULL);
    s->cx = LOWORD(dwExt);
    s->cy = HIWORD(dwExt);
    if (s->cx > W) W = s->cx;
    H = H + s->cy;
    if (Text[i]!=0)
    {
      i++;
      if ((Text[i]==0x0a) &&
	  (Text[i-1]==0x0d))
	i++;
    }
  } while (Text[i]!=0);
  if ((i-i0 == 0) && (H > s->cy)) H = H - s->cy;
  s->cx = W;
  s->cy = H;
}

void TTMGetDir(PCHAR Dir, int destlen)
{
  strncpy_s(Dir, destlen, CurrentDir, _TRUNCATE);
}

void TTMSetDir(PCHAR Dir)
{
  char Temp[MAXPATHLEN];

  _getcwd(Temp,sizeof(Temp));
  _chdir(CurrentDir);
  _chdir(Dir);
  _getcwd(CurrentDir,sizeof(CurrentDir));
  _chdir(Temp);
}

BOOL GetAbsPath(PCHAR FName, int destlen)
{
  int i, j;
  char Temp[MAXPATHLEN];

  if (! GetFileNamePos(FName,&i,&j)) {
    return FALSE;
  }
  if (strlen(FName) > 2 && FName[1] == ':') {
    // fullpath
    return TRUE;
  }
  else if (FName[0] == '\\' && FName[1] == '\\') {
    // UNC (\\server\path)
    return TRUE;
  }
  strncpy_s(Temp, sizeof(Temp), FName, _TRUNCATE);
  strncpy_s(FName,destlen,CurrentDir,_TRUNCATE);

  if (Temp[0] == '\\' && destlen > 2) {
    // from drive root (\foo\bar)
    FName[2] = 0;
  }
  else {
    AppendSlash(FName,destlen);
  }

  strncat_s(FName,destlen,Temp,_TRUNCATE);
  return TRUE;
}

int DoGetSpecialFolder(int CSIDL, PCHAR dest, int dest_len)
{
	char Path[MAX_PATH] = "";
	LPITEMIDLIST pidl;

	if (!IsWindowsNTKernel() || IsWindowsNT4()) {
		switch (CSIDL) {
			case CSIDL_COMMON_DESKTOPDIRECTORY:
			case CSIDL_COMMON_STARTMENU:
			case CSIDL_COMMON_PROGRAMS:
			case CSIDL_COMMON_STARTUP:
				return 0;
		}
	}

	if (SHGetSpecialFolderLocation(NULL, CSIDL, &pidl) != S_OK) {
		return 0;
	}

	SHGetPathFromIDList(pidl, Path);
	CoTaskMemFree(pidl);

	strncpy_s(dest, dest_len, Path, _TRUNCATE);
	return 1;
}

int GetSpecialFolder(PCHAR dest, int dest_len, PCHAR type)
{
	int CSIDL;

	if (_stricmp(type, "AllUsersDesktop") == 0) {
		CSIDL = CSIDL_COMMON_DESKTOPDIRECTORY;
	}
	else if (_stricmp(type, "AllUsersStartMenu") == 0) {
		CSIDL = CSIDL_COMMON_STARTMENU;
	}
	else if (_stricmp(type, "AllUsersPrograms") == 0) {
		CSIDL = CSIDL_COMMON_PROGRAMS;
	}
	else if (_stricmp(type, "AllUsersStartup") == 0) {
		CSIDL = CSIDL_COMMON_STARTUP;
	}
	else if (_stricmp(type, "Desktop") == 0) {
		CSIDL = CSIDL_DESKTOPDIRECTORY;
	}
	else if (_stricmp(type, "Favorites") == 0) {
		CSIDL = CSIDL_FAVORITES;
	}
	else if (_stricmp(type, "Fonts") == 0) {
		CSIDL = CSIDL_FONTS;
	}
	else if (_stricmp(type, "MyDocuments") == 0) {
		CSIDL = CSIDL_PERSONAL;
	}
	else if (_stricmp(type, "NetHood") == 0) {
		CSIDL = CSIDL_NETHOOD;
	}
	else if (_stricmp(type, "PrintHood") == 0) {
		CSIDL = CSIDL_PRINTHOOD;
	}
	else if (_stricmp(type, "Programs") == 0) {
		CSIDL = CSIDL_PROGRAMS;
	}
	else if (_stricmp(type, "Recent") == 0) {
		CSIDL = CSIDL_RECENT;
	}
	else if (_stricmp(type, "SendTo") == 0) {
		CSIDL = CSIDL_SENDTO;
	}
	else if (_stricmp(type, "StartMenu") == 0) {
		CSIDL = CSIDL_STARTMENU;
	}
	else if (_stricmp(type, "Startup") == 0) {
		CSIDL = CSIDL_STARTUP;
	}
	else if (_stricmp(type, "Templates") == 0) {
		CSIDL = CSIDL_TEMPLATES;
	}

	if (!DoGetSpecialFolder(CSIDL, dest, dest_len)) {
		return 0;
	}
	return 1;
}

int GetMonitorLeftmost(int PosX, int PosY)
{
	if (IsWindows95() || IsWindowsNT4()) {
		// // NT4.0, 95 はマルチモニタAPIに非対応
		return 0;
	}
	else {
		HMONITOR hm;
		POINT pt;
		MONITORINFO mi;

		pt.x = PosX;
		pt.y = PosY;
		hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hm, &mi);
		return mi.rcWork.left;
	}
}

void BringupWindow(HWND hWnd)
{
	DWORD thisThreadId;
	DWORD fgThreadId;

	thisThreadId = GetWindowThreadProcessId(hWnd, NULL);
	fgThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);

	if (thisThreadId == fgThreadId) {
		SetForegroundWindow(hWnd);
		BringWindowToTop(hWnd);
	} else {
		AttachThreadInput(thisThreadId, fgThreadId, TRUE);
		SetForegroundWindow(hWnd);
		BringWindowToTop(hWnd);
		AttachThreadInput(thisThreadId, fgThreadId, FALSE);
	}
}
