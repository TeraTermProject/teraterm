/*
 * Copyright (C) S.Hayakawa NTT-IT 1998-2002
 * (C) 2002-2020 TeraTerm Project
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

#ifndef WINMISC_H
#define	WINMISC_H

#include	<windows.h>
#include	<tchar.h>

#include	<shlobj.h>
#include	<shellapi.h>

#include	<stdio.h>
#include	<time.h>
#include	"i18n.h"

// Window Position
#define		POSITION_LEFTTOP		0x00
#define		POSITION_LEFTBOTTOM		0x01
#define		POSITION_RIGHTTOP		0x02
#define		POSITION_RIGHTBOTTOM	0x03
#define		POSITION_CENTER			0x04
#define		POSITION_OUTSIDE		0x05

// misc
void	GetTime(TCHAR *cTimeStr);
void	GetTimeEx(LPTSTR lpszTimeStr);
void	SetDlgPos(HWND hWnd, int pos);
void	EncodePassword(TCHAR *cPassword, TCHAR *cEncodePassword);
BOOL	EnableItem(HWND hWnd, int idControl, BOOL flag);
BOOL	OpenFileDlg(HWND hWnd, UINT editCtl, TCHAR *title, TCHAR *filter, TCHAR *defaultDir);
BOOL	SaveFileDlg(HWND hWnd, UINT editCtl, TCHAR *title, TCHAR *filter, TCHAR *defaultDir);
BOOL	BrowseForFolder(HWND hWnd, TCHAR *szTitle, TCHAR *szPath);
BOOL	GetModulePath(TCHAR *szPath, DWORD dwMaxPath);
BOOL	SetForceForegroundWindow(HWND hWnd);
UINT	GetResourceType(LPCTSTR lpszPath);
int		CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
TCHAR	*PathTok(TCHAR *str, TCHAR *separator);
TCHAR	*lstrstri(TCHAR *s1, TCHAR *s2);
void	UTIL_get_lang_msg(PCHAR key, PCHAR buf, int buf_len, PCHAR def, PCHAR iniFile);
int		UTIL_get_lang_font(PCHAR key, HWND dlg, PLOGFONT logfont, HFONT *font, PCHAR iniFile);
LRESULT CALLBACK password_wnd_proc(HWND control, UINT msg,
                                   WPARAM wParam, LPARAM lParam);

#endif
