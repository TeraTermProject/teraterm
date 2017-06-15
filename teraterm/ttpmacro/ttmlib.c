/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

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
	char Path[MAX_PATH] = "";
	LPITEMIDLIST pidl;

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
	if (!HasMultiMonitorSupport()) {
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
