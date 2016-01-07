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

typedef struct {
	char *name;
	int csidl;
} SpFolder;

static SpFolder spfolders[] = {
	{ "AllUsersDesktop",   CSIDL_COMMON_DESKTOPDIRECTORY },
	{ "AllUsersStartMenu", CSIDL_COMMON_STARTMENU },
	{ "AllUsersPrograms",  CSIDL_COMMON_PROGRAMS },
	{ "AllUsersStartup",   CSIDL_COMMON_STARTUP },
	{ "Desktop",           CSIDL_DESKTOPDIRECTORY },
	{ "Favorites",         CSIDL_FAVORITES },
	{ "Fonts",             CSIDL_FONTS },
	{ "MyDocuments",       CSIDL_PERSONAL },
	{ "NetHood",           CSIDL_NETHOOD },
	{ "PrintHood",         CSIDL_PRINTHOOD },
	{ "Programs",          CSIDL_PROGRAMS },
	{ "Recent",            CSIDL_RECENT },
	{ "SendTo",            CSIDL_SENDTO },
	{ "StartMenu",         CSIDL_STARTMENU },
	{ "Startup",           CSIDL_STARTUP },
	{ "Templates",         CSIDL_TEMPLATES },
	{ NULL,                -1}
};

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
	OSVERSIONINFO osvi;
	char Path[MAX_PATH] = "";
	LPITEMIDLIST pidl;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if ( (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion == 4) ||
	     (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ) {
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
	SpFolder *p;

	for (p = spfolders; p->name != NULL; p++) {
		if (_stricmp(type, p->name) == 0) {
			return DoGetSpecialFolder(p->csidl, dest, dest_len);
		}
	}
	return 0;
}

int GetMonitorLeftmost(int PosX, int PosY)
{
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if ( (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion == 4) ||
	     (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && osvi.dwMinorVersion < 10) ) {
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
