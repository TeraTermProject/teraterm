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
  char Temp[256];
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
  else if (i>0) {
    return TRUE;
  }
  strncpy_s(Temp, sizeof(Temp), FName, _TRUNCATE);
  strncpy_s(FName,destlen,CurrentDir,_TRUNCATE);
  AppendSlash(FName,destlen);
  strncat_s(FName,destlen,Temp,_TRUNCATE);
  return TRUE;
}

int DoGetSpecialFolder(int CSIDL, PCHAR KEY, PCHAR dest, int dest_len)
{
	OSVERSIONINFO osvi;
	LONG result;
	HKEY hKey;
	DWORD disposition, len;

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
		result = RegCreateKeyEx(HKEY_CURRENT_USER,
		                        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
		                        0,
		                        "",
		                        REG_OPTION_NON_VOLATILE,
		                        KEY_READ,
		                        NULL,
		                        &hKey,
		                        &disposition);
		if (result != ERROR_SUCCESS) {
			return 0;
		}

		len = sizeof(Path);
		result = RegQueryValueEx(hKey, KEY, NULL, NULL, Path, &len);
		if (result != ERROR_SUCCESS) {
			return 0;
		}

		RegCloseKey(hKey);
	}
	else {
		if (SHGetSpecialFolderLocation(NULL, CSIDL, &pidl) != NOERROR) {
			return 0;
		}

		SHGetPathFromIDList(pidl, Path);
		CoTaskMemFree(pidl);
	}

	strncpy_s(dest, dest_len, Path, _TRUNCATE);
	return 1;
}

int GetSpecialFolder(PCHAR dest, int dest_len, PCHAR type)
{
	int CSIDL;
	char REGKEY[256];

	if (_stricmp(type, "AllUsersDesktop") == 0) {
		CSIDL = CSIDL_COMMON_DESKTOPDIRECTORY;
		strncpy_s(REGKEY, sizeof(REGKEY), "", _TRUNCATE);
	}
	else if (_stricmp(type, "AllUsersStartMenu") == 0) {
		CSIDL = CSIDL_COMMON_STARTMENU;
		strncpy_s(REGKEY, sizeof(REGKEY), "", _TRUNCATE);
	}
	else if (_stricmp(type, "AllUsersPrograms") == 0) {
		CSIDL = CSIDL_COMMON_PROGRAMS;
		strncpy_s(REGKEY, sizeof(REGKEY), "", _TRUNCATE);
	}
	else if (_stricmp(type, "AllUsersStartup") == 0) {
		CSIDL = CSIDL_COMMON_STARTUP;
		strncpy_s(REGKEY, sizeof(REGKEY), "", _TRUNCATE);
	}
	else if (_stricmp(type, "Desktop") == 0) {
		CSIDL = CSIDL_DESKTOPDIRECTORY;
		strncpy_s(REGKEY, sizeof(REGKEY), "Desktop", _TRUNCATE);
	}
	else if (_stricmp(type, "Favorites") == 0) {
		CSIDL = CSIDL_FAVORITES;
		strncpy_s(REGKEY, sizeof(REGKEY), "Favorites", _TRUNCATE);
	}
	else if (_stricmp(type, "Fonts") == 0) {
		CSIDL = CSIDL_FONTS;
		strncpy_s(REGKEY, sizeof(REGKEY), "Fonts", _TRUNCATE);
	}
	else if (_stricmp(type, "MyDocuments") == 0) {
		CSIDL = CSIDL_PERSONAL;
		strncpy_s(REGKEY, sizeof(REGKEY), "Personal", _TRUNCATE);
	}
	else if (_stricmp(type, "NetHood") == 0) {
		CSIDL = CSIDL_NETHOOD;
		strncpy_s(REGKEY, sizeof(REGKEY), "NetHood", _TRUNCATE);
	}
	else if (_stricmp(type, "PrintHood") == 0) {
		CSIDL = CSIDL_PRINTHOOD;
		strncpy_s(REGKEY, sizeof(REGKEY), "PrintHood", _TRUNCATE);
	}
	else if (_stricmp(type, "Programs") == 0) {
		CSIDL = CSIDL_PROGRAMS;
		strncpy_s(REGKEY, sizeof(REGKEY), "Programs", _TRUNCATE);
	}
	else if (_stricmp(type, "Recent") == 0) {
		CSIDL = CSIDL_RECENT;
		strncpy_s(REGKEY, sizeof(REGKEY), "Recent", _TRUNCATE);
	}
	else if (_stricmp(type, "SendTo") == 0) {
		CSIDL = CSIDL_SENDTO;
		strncpy_s(REGKEY, sizeof(REGKEY), "SendTo", _TRUNCATE);
	}
	else if (_stricmp(type, "StartMenu") == 0) {
		CSIDL = CSIDL_STARTMENU;
		strncpy_s(REGKEY, sizeof(REGKEY), "Start Menu", _TRUNCATE);
	}
	else if (_stricmp(type, "Startup") == 0) {
		CSIDL = CSIDL_STARTUP;
		strncpy_s(REGKEY, sizeof(REGKEY), "Startup", _TRUNCATE);
	}
	else if (_stricmp(type, "Templates") == 0) {
		CSIDL = CSIDL_TEMPLATES;
		strncpy_s(REGKEY, sizeof(REGKEY), "Templates", _TRUNCATE);
	}

	if (!DoGetSpecialFolder(CSIDL, REGKEY, dest, dest_len)) {
		return 0;
	}
	return 1;
}
