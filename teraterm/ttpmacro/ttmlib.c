/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// TTMACRO.EXE, misc routines

#include "teraterm.h"
#include "ttlib.h"
#include <string.h>
#include <direct.h>
#include <shlobj.h>

#include "compat_win.h"
#include "ttmlib.h"
#include "codeconv.h"

static char CurrentDir[MAXPATHLEN];

/**
 * 文字を描画した時のサイズを算出する
 *	@param[in]	hWnd
 *	@param[in]	Font	フォントハンドル
 *						NULLの時はhWndに設定されているフォント
 *	@param[in]	Text	描画するテキスト
 *	@param[out]	size	描画サイズ(cx,cy)
 */
void CalcTextExtent(HWND hWnd, HFONT hFont, const char *Text, LPSIZE s)
{
	HDC DC = GetDC(hWnd);
	int W, H, i, i0;
	char Temp[512];
	DWORD dwExt;
	HFONT prevFont;
	if (hFont == NULL) {
		hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	}
	prevFont = (HFONT)SelectObject(DC, hFont);

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
		dwExt = GetTabbedTextExtent(DC,Temp, (int)strlen(Temp),0,NULL);
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
	if (prevFont != NULL) {
		SelectObject(DC, prevFont);
	}
	ReleaseDC(hWnd, DC);
}

void CalcTextExtentW(HWND hWnd, HFONT hFont, const wchar_t *Text, LPSIZE s)
{
	HDC DC = GetDC(hWnd);
	int W, H, i, i0;
	wchar_t Temp[512];
	DWORD dwExt;
	HFONT prevFont;
	if (hFont == NULL) {
		hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	}
	prevFont = (HFONT)SelectObject(DC, hFont);

	W = 0;
	H = 0;
	i = 0;
	do {
		i0 = i;
		while ((Text[i]!=0) &&
			   (Text[i]!=0x0d) &&
			   (Text[i]!=0x0a))
			i++;
		memcpy(Temp,&Text[i0],sizeof(wchar_t) * (i-i0));
		Temp[i-i0] = 0;
		if (Temp[0]==0)
		{
			Temp[0] = 0x20;
			Temp[1] = 0;
		}
		dwExt = GetTabbedTextExtentW(DC,Temp,(int)wcslen(Temp),0,NULL);
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
	if (prevFont != NULL) {
		SelectObject(DC, prevFont);
	}
	ReleaseDC(hWnd, DC);
}

void TTMGetDir(PCHAR Dir, int destlen)
{
  strncpy_s(Dir, destlen, CurrentDir, _TRUNCATE);
}

void TTMSetDir(const char *Dir)
{
	wchar_t Temp[MAX_PATH];
	wchar_t CurrentDirW[MAX_PATH];
	wchar_t *pCurrentDirW = ToWcharU8(CurrentDir);
	wchar_t *DirW = ToWcharU8(Dir);
	char *pCurrentDirU8;
	GetCurrentDirectoryW(_countof(Temp), Temp);
	SetCurrentDirectoryW(pCurrentDirW);
	SetCurrentDirectoryW(DirW);
	GetCurrentDirectoryW(_countof(CurrentDirW), CurrentDirW);
	pCurrentDirU8 = ToU8W(CurrentDirW);
	strncpy_s(CurrentDir, _countof(CurrentDir), pCurrentDirU8, _TRUNCATE);
	SetCurrentDirectoryW(Temp);
	free(pCurrentDirW);
	free(DirW);
	free(pCurrentDirU8);
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

static int GetSpecialFolderAlloc(const char *type, char **dest)
{
	typedef struct {
		const char *name;
		REFKNOWNFOLDERID rfid;
	} SpFolder;

	static const SpFolder spfolders[] = {
		{ "AllUsersDesktop",   &FOLDERID_PublicDesktop },
		{ "AllUsersStartMenu", &FOLDERID_CommonStartMenu },
		{ "AllUsersPrograms",  &FOLDERID_CommonPrograms },
		{ "AllUsersStartup",   &FOLDERID_CommonStartup },
		{ "Desktop",           &FOLDERID_Desktop },
		{ "Favorites",         &FOLDERID_Favorites },
		{ "Fonts",             &FOLDERID_Fonts },
		{ "MyDocuments",       &FOLDERID_Documents },
		{ "NetHood",           &FOLDERID_NetHood },
		{ "PrintHood",         &FOLDERID_PrintHood },
		{ "Programs",          &FOLDERID_Programs },
		{ "Recent",            &FOLDERID_Recent },
		{ "SendTo",            &FOLDERID_SendTo },
		{ "StartMenu",         &FOLDERID_StartMenu },
		{ "Startup",           &FOLDERID_Startup },
		{ "Templates",         &FOLDERID_Templates },
		{ NULL,                NULL }
	};

	const SpFolder *p;

	for (p = spfolders; p->name != NULL; p++) {
		if (_stricmp(type, p->name) == 0) {
			wchar_t *folderW;
			_SHGetKnownFolderPath(p->rfid, 0, NULL, &folderW);
			*dest = ToU8W(folderW);
			free(folderW);
			return 1;
		}
	}
	*dest = NULL;
	return 0;
}

int GetSpecialFolder(PCHAR dest, int dest_len, const char *type)
{
	char *folder;
	GetSpecialFolderAlloc(type, &folder);
	strncpy_s(dest, dest_len, folder, _TRUNCATE);
	free(folder);
	return 1;
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

int MessageBoxHaltScript(HWND hWnd)//, const char *UILanguageFile)
{
	char uimsg[MAX_UIMSG];
	char uimsg2[MAX_UIMSG];
	get_lang_msg("MSG_MACRO_CONF", uimsg, sizeof(uimsg), "MACRO: confirmation", UILanguageFile);
	get_lang_msg("MSG_MACRO_HALT_SCRIPT", uimsg2, sizeof(uimsg2), "Are you sure that you want to halt this macro script?", UILanguageFile);
	return MessageBox(hWnd, uimsg2, uimsg, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
}
